/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2003 Konrad M.Kokoszkiewicz <draco@atari.org>
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author:  Konrad M.Kokoszkiewicz
 * Started: 15-01-2003
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 */

# ifdef BUILTIN_SHELL

# include <stdarg.h>

# include "libkern/libkern.h"

# include "mint/basepage.h"	/* BASEPAGE struct */
# include "mint/mem.h"		/* F_FASTLOAD et contubernales */
# ifdef DEBUG_INFO
# include "mint/proc.h"		/* curproc (for DEBUG) */
# endif
# include "mint/signal.h"	/* SIGCHLD etc. */
# include "mint/stat.h"		/* struct stat */

# include "arch/cpu.h"		/* setstack() */
# include "arch/startup.h"	/* _base */
# include "arch/syscall.h"	/* Pexec(), Cconrs() et cetera */

# include "cnf.h"		/* init_env */
# include "dosmem.h"		/* m_free() */
# include "info.h"		/* national stuff */
# include "k_exec.h"		/* sys_pexec() */

# include "mis.h"

/* Note:
 *
 * 1) the shell is running as separate process, not in kernel context;
 * 2) the shell is running in user mode;
 * 3) the shell is very minimal, so don't expect much ;-)
 *
 */

/* XXX after the code stops changing so quickly, move all the messages to info.c
 */

# define STDIN	0
# define STDOUT	1
# define STDERR	2

# define SHELL_FLAGS	(F_FASTLOAD | F_ALTLOAD | F_ALTALLOC | F_PROT_P)
# define SHELL_UMASK	~(S_ISUID | S_ISGID | S_ISVTX | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

# define SH_VER_MAIOR	0
# define SH_VER_MINOR	1

# define COPYCOPY	"MiS v.%d.%d, the FreeMiNT internal shell,\r\n" \
			"(c) 2003 by Konrad M.Kokoszkiewicz (draco@atari.org)\r\n"

# define MISSING_ARG	"missing argument: "

# define SHELL_STACK	32768L		/* maximum usage is so far about a half ot this */
# define SHELL_ARGS	1024L		/* number of pointers in the argument vector table (i.e. 4K) */

/* this is an average number of seconds in Gregorian year
 * (365 days, 6 hours, 11 minutes, 15 seconds).
 */
# define SEC_OF_YEAR	31558275L

# define LINELEN	126

/* Some help for the optimizer */
static void shell(void) __attribute__((noreturn));

/* Global variables */
static BASEPAGE *shell_base;
static short xcommands;		/* if 1, the extended command set is active */

/* Utility routines */

static void
shell_fprintf(long handle, const char *fmt, ...)
{
	char buf[2048];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, sizeof (buf), fmt, args);
	va_end(args);

	Fwrite(handle, strlen(buf), buf);
}

INLINE void
shell_print(char *text)
{
	shell_fprintf(STDOUT, text);
}

/* Simple conversion of a pathname from the DOS to Unix format */
static void
dos2unix(char *pathname)
{
	char *p;

	p = pathname;

	while (*p)
	{
		if (*p == '\\')
			*p = '/';
		p++;
	}

	p = pathname;

	if (p[1] == ':')
	{
		p[1] = p[0];
		p[0] = '/';
	}
}

/* Helpers for ls:
 * justify_left() - just pads the text with spaces upto given lenght
 * justify_right() - shifts the text right adding spaces on the left
 *		until the given length is reached.
 */
static char *
justify_left(char *p, long spaces)
{
	long s = (spaces - strlen(p));

	while (*p && !isspace(*p))
		p++;

	while (s)
	{
		*p++ = ' ';
		s--;
	}

	return p;
}

static char *
justify_right(char *p, long spaces)
{
	long plen, s;
	char temp[32];

	plen = strlen(p);
	s = (spaces - plen);

	if (s > 0)
	{
		strcpy(temp, p);
		while (s)
		{
			*p++ = ' ';
			s--;
		}
		strcpy(p, temp);
	}

	return (p + plen);
}

/* Helper routines for manipulating environment */
static long
env_size(void)
{
	char *var;
	long count = 0;

	var = shell_base->p_env;

	if (var)
	{
		while (*var)
		{
			while (*var)
			{
				var++;
				count++;
			}
			var++;
			count++;
		}
	}

	return count;
}

static char *
env_append(char *where, char *what)
{
	strcpy(where, what);
	where += strlen(where);
	*where++ = 0;

	return where;
}

static char *
shell_getenv(const char *var)
{
	char *env_str = shell_base->p_env;
	long len;

	if (env_str)
	{
		len = strlen(var);

		while (*env_str)
		{
			if ((strncmp(env_str, var, len) == 0) && (env_str[len] == '='))
				return env_str + len + 1;
			while (*env_str)
				env_str++;
			env_str++;
		}
	}

	return NULL;
}

/* shell_delenv() idea is borrowed from mintlib's del_env() */
static void
shell_delenv(const char *strng)
{
	char *name, *var, *env_str = shell_base->p_env;

	if (!env_str)
		return;

	/* find the tag in the environment */
	var = shell_getenv(strng);

	if (!var)
		return;

	var -= strlen(strng);
	var--;

	/* if it's found, move all the other environment variables down by 1 to
   	 * delete it
         */
	name = var + strlen(var) + 1;

	do
	{
		while (*name)
			*var++ = *name++;
		*var++ = *name++;
	} while (*name);

	*var = 0;

	Mshrink((void *)shell_base->p_env, env_size());
}

/* XXX try to avoid reallocation whenever possible */
static void
shell_setenv(const char *var, char *value)
{
	char *env_str = shell_base->p_env;
	char *new_env, *old_var, *es, *ne;
	long new_size, varlen;

	varlen = strlen(var);
	new_size = env_size();
	new_size += strlen(value);

	old_var = shell_getenv(var);

	if (old_var)
		new_size -= strlen(old_var);	/* this is the VALUE of the var */
	else
		new_size += varlen;		/* this is its NAME */

	new_size += 2;				/* '=' and trailing zero */

	new_env = (char *)Mxalloc(new_size, 3);
	if (new_env == NULL)
		return;

	es = env_str;
	ne = new_env;

	/* If it already exists, delete it */
	shell_delenv(var);

	/* Copy old env to new place */
	while (*es)
	{
		while (*es)
			*ne++ = *es++;
		*ne++ = *es++;
	}

	strcpy(ne, var);
	strcat(ne, "=");
	strcat(ne, value);

	ne += strlen(ne);
	ne++;
	*ne = 0;

	shell_base->p_env = new_env;

	Mfree(env_str);
}

/* Split command line into separate strings, put their pointers
 * into argv[], return argc.
 *
 * XXX add 'quoted arguments' handling.
 * XXX add wildcard expansion (at least the `*'), see fnmatch().
 * XXX also evaluate env variables, if given as arguments
 *
 */
INLINE long
crunch(char *cmdline, char **argv)
{
	char *cmd = cmdline, *start;
	long cmdlen, idx = 0;

	cmdlen = strlen(cmdline);

	do
	{
		while (cmdlen && isspace(*cmd))			/* kill leading spaces */
		{
			cmd++;
			cmdlen--;
		}

		start = cmd;

		while (cmdlen && !isspace(*cmd))
		{
			cmd++;
			cmdlen--;
		}

		if (start == cmd)
			break;
		else if (*cmd)
		{
			*cmd = 0;
			cmd++;
			cmdlen--;
		}

		argv[idx] = start;
		idx++;

	} while (cmdlen > 0);

	argv[idx] = NULL;

	return idx;
}

/* Execute an external program */
INLINE long
execvp(char *oldcmd, char **argv)
{
	char *var, *new_env, *new_var, *t, *path, *np, npath[2048];
	long count = 0, x, r = -1L;

	var = shell_base->p_env;

	/* Check the size of the environment */
	count = env_size();
	count++;			/* trailing zero */
	count += strlen(oldcmd + 1);	/* add some space for the ARGV= strings */
	count += sizeof("ARGV=");	/* this is 6 bytes */

	new_env = (char *)Mxalloc(count, 3);

	if (!new_env)
		return -40;

	var = shell_base->p_env;
	new_var = new_env;

	/* Copy variables from `var' to `new_var', but don't copy ARGV strings
	 */
	if (var)
	{
		while (*var)
		{
			if (strncmp(var, "ARGV=", 5) == 0)
				break;
			while (*var)
				*new_var++ = *var++;
			*new_var++ = *var++;
		}
	}

	/* Append new ARGV strings */
	x = 0;
	new_var = env_append(new_var, "ARGV=");

	while (argv[x])
	{
		new_var = env_append(new_var, argv[x]);
		x++;
	}
	*new_var = 0;

	*oldcmd = 0x7f;

	/* $PATH searching. Don't do it, if the user seems to
	 * have specified the explicit pathname.
	 */
	t = strrchr(argv[0], '/');

	if (t == NULL)
	{
		path = shell_getenv("PATH");
		if (path == NULL)
			path = "./";
	}
	else
		path = "";

	do
	{
		np = npath;

		while (*path && *path != ';' && *path != ',')
			*np++ = *path++;
		if (*path)
			path++;			/* skip the separator */

		*np = 0;
		/* t == NULL means that we really search through $PATH */
		if (t == NULL)
			strcat(npath, "/");
		strcat(npath, argv[0]);

		r = Pexec(0, npath, oldcmd, new_env);

	} while (*path && r < 0);

	Mfree(new_env);

	return r;
}

static void
xcmdstate(void)
{
	shell_fprintf(STDOUT, "Extended commands are %s\r\n", xcommands ? "on" : "off");
}

static void
env(void)
{
	char *var = shell_base->p_env;

	if (var)
	{
		while (*var)
		{
			shell_fprintf(STDOUT, "%s\r\n", var);
			while (*var)
				var++;
			var++;
		}
	}
}

/* End utilities, begin commands */

static long
sh_ver(long argc, char **argv)
{
	shell_fprintf(STDOUT, COPYCOPY, SH_VER_MAIOR, SH_VER_MINOR);

	return 0;
}

static long
sh_help(long argc, char **argv)
{
	shell_fprintf(STDOUT, \
	"	MiS is not intended to be a regular system shell, so don't\r\n" \
	"	expect much. It is only a tool to fix bigger problems that\r\n" \
	"	prevent the system from booting normally. Basic commands:\r\n" \
	"\r\n" \
	"	cd [dir] - change directory\r\n" \
	"	echo text - display `text'\r\n" \
	"	exit - leave and reboot\r\n" \
	"	export [NAME=value] - set an environment variable\r\n" \
	"	help - display this message\r\n" \
	"	setenv [NAME value] - set an environment variable\r\n" \
	"	ver - display version information\r\n" \
	"	xcmd [on|off] - switch the extended command set on/off\r\n"
	"\r\n" \
	"	Extended commands (now %s):\r\n" \
	"\r\n" \
	"	chgrp DEC-GROUP file - change group the file belongs to\r\n" \
	"	chmod OCTAL-MODE file - change access permissions for file\r\n" \
	"	chown DEC-OWNER[:DEC-GROUP] file - change file's ownership\r\n" \
	"	*cp - copy file\r\n" \
	"	ln old new - create a symlink `new' pointing to file `old'\r\n" \
	"	ls [dir] - display directory\r\n" \
	"	*mv - move/rename a file\r\n" \
	"	*rm - delete a file\r\n" \
	"\r\n" \
	"	All other words typed are understood as names of programs\r\n" \
	"	to execute. In case you'd want to execute something, that\r\n" \
	"	has been named like one of the internal commands, use the\r\n" \
	"	full pathname." \
	"\r\n", \
	xcommands ? "on" : "off");

	return 0;
}

/* Display all files in the current directory, with attributes et ceteris.
 * No wilcards filtering what to display, no sorted output, no nothing.
 */
static long
sh_ls(long argc, char **argv)
{
	struct stat st;
	struct timeval tv;
	short year, month, day, hour, minute;
	long r, s, handle;
	char *p, *dir, path[1024], link[1024];
	char entry[256];

	dir = ".";

	if (argc >= 2)
	{
		if (strcmp(argv[1], "--help") == 0)
		{
			shell_fprintf(STDOUT, "%s [dirspec]\r\n", argv[0]);

			return 0;
		}
		else
			dir = argv[1];
	}

	handle = Dopendir(dir, 0);
	if (handle < 0)
		return handle;

	Tgettimeofday(&tv, NULL);

	do
	{
		r = Dreaddir(sizeof(entry), handle, entry);

		if (r == 0)
		{
			strcpy(path, dir);
			strcat(path, "/");
			strcat(path, entry + sizeof(long));

			/* `/bin/ls -l' doesn't dereference links, so we don't either */
			s = Fstat64(1, path, &st);

			if (s == 0)
			{
				if (S_ISLNK(st.mode))
				{
					s = Freadlink(sizeof(link), link, path);
					if (s == 0)
						dos2unix(link);
				}

				/* Reuse the path[] space */
				p = path;

				if (S_ISLNK(st.mode))		/* file type */
					*p++ = 'l';
				else if (S_ISMEM(st.mode))
					*p++ = 'm';
				else if (S_ISFIFO(st.mode))
					*p++ = 'p';
				else if (S_ISREG(st.mode))
					*p++ = '-';
				else if (S_ISBLK(st.mode))
					*p++ = 'b';
				else if (S_ISDIR(st.mode))
					*p++ = 'd';
				else if (S_ISCHR(st.mode))
					*p++ = 'c';
				else if (S_ISSOCK(st.mode))
					*p++ = 's';
				else
					*p++ = '?';

				/* access attibutes: user */
				*p++ = (st.mode & S_IRUSR) ? 'r' : '-';
				*p++ = (st.mode & S_IWUSR) ? 'w' : '-';
				if (st.mode & S_IXUSR)
					*p++ = (st.mode & S_ISUID) ? 's' : 'x';
				else
					*p++ = '-';

				/* ... group */
				*p++ = (st.mode & S_IRGRP) ? 'r' : '-';
				*p++ = (st.mode & S_IWGRP) ? 'w' : '-';
				if (st.mode & S_IXGRP)
					*p++ = (st.mode & S_ISGID) ? 's' : 'x';
				else
					*p++ = '-';

				/* ... others */
				*p++ = (st.mode & S_IROTH) ? 'r' : '-';
				*p++ = (st.mode & S_IWOTH) ? 'w' : '-';
				if (st.mode & S_IXOTH)
					*p++ = (st.mode & S_ISVTX) ? 't' : 'x';
				else
					*p++ = '-';

				*p++ = ' ';

				/* Unfortunately ksprintf() lacks many formatting features ...
				 */
				ksprintf(p, sizeof(path), "%d", (short)st.nlink);	/* XXX */
				p = justify_right(p, 5);
				*p++ = ' ';

				ksprintf(p, sizeof(path), "%ld", st.uid);
				p = justify_left(p, 9);

				ksprintf(p, sizeof(path), "%ld", st.gid);
				p = justify_left(p, 9);

				/* XXX this will cause problems, if st.size > 2GB */
				ksprintf(p, sizeof(path), "%ld", (long)st.size);
				p = justify_right(p, 8);
				*p++ = ' ';

				/* And now recalculate the time stamp */
				unix2calendar(st.mtime.time, &year, &month, &day, &hour, &minute, NULL);

				ksprintf(p, sizeof(path), "%s", months_abbr_3[month - 1]);
				p = justify_left(p, 4);

				ksprintf(p, sizeof(path), "%d", day);
				p = justify_left(p, 3);

				/* Here we decide whether the timestamp displayed should
				 * contain the year or the hour/minute of the modification.
				 *
				 * Basically everything that was modified 6 months ago or
				 * earlier will have the year, more recent stuff will get
				 * hour/minute. This is less or more what /bin/ls does.
				 *
				 * There can be up to 18 hours, 33 minutes and 45 seconds
				 * of error relative to the calendar time, but for this purpose
				 * it does not matter so much (and we avoid calling
				 * unix2calendar again).
				 */
				if ((tv.tv_sec - st.mtime.time) > (SEC_OF_YEAR >> 1))
					ksprintf(p, sizeof(path), "%d", year);
				else
					ksprintf(p, sizeof(path), (minute > 9) ? "%d:%d" : "%d:0%d", hour, minute);

				p = justify_right(p, 5);

				*p = 0;

				shell_fprintf(STDOUT, "%s %s", path, entry + sizeof(long));

				if (S_ISLNK(st.mode) && s == 0)
					shell_fprintf(STDOUT, " -> %s\r\n", link);
				else
					shell_print("\r\n");
			}
		}
	} while (r == 0);

	Dclosedir(handle);

	return 0;
}

/* Change cwd to the given path. If none given, change to $HOME
 *
 * XXX crashes when entering /kern/1
 *
 */
static long
sh_cd(long argc, char **argv)
{
	long r;
	char *newdir, *pwd, cwd[1024];

	if (argc >= 2)
	{
		if (strcmp(argv[1], "--help") == 0)
		{
			shell_fprintf(STDOUT, "%s [newdir]\r\n", argv[0]);

			return 0;
		}
		else
			newdir = argv[1];
	}
	else
	{
		newdir = shell_getenv("HOME");
		if (!newdir)
			newdir = "/";
	}

	r = Dsetpath(newdir);

	if (r == 0)
	{
		Dgetcwd(cwd, 0, sizeof(cwd));

		if (*cwd == 0)
			strcpy(cwd, "/");
		else
			dos2unix(cwd);

		pwd = shell_getenv("PWD");

		if (pwd && strcmp(pwd, cwd))
			shell_setenv("OLDPWD", pwd);

		shell_setenv("PWD", cwd);
	}

	return r;
}

/* chgrp, chown, chmod: change various attributes */
static long
sh_chgrp(long argc, char **argv)
{
	struct stat st;
	long r, gid;
	char *fname;

	if (argc < 3)
	{
		shell_fprintf(STDERR, "%s%s DEC-GROUP file\r\n", MISSING_ARG, argv[0]);

		return 0;
	}

	fname = argv[2];

	r = Fstat64(0, fname, &st);
	if (r < 0)
		return r;

	gid = atol(argv[1]);

	if (gid == st.gid)
		return 0;			/* no change */

	return Fchown(fname, st.uid, gid);
}

static long
sh_chown(long argc, char **argv)
{
	struct stat st;
	long r, uid, gid;
	char *own, *fname, *grp;

	if (argc < 3)
	{
		shell_fprintf(STDERR, "%s%s DEC-OWNER[:DEC-GROUP] file\r\n", MISSING_ARG, argv[0]);

		return 0;
	}

	own = argv[1];
	fname = argv[2];

	r = Fstat64(0, fname, &st);
	if (r < 0)
		return r;

	uid = st.uid;
	gid = st.gid;

	/* cases like `chown 0 filename' and `chown 0. filename' */
	grp = strrchr(own, '.');
	if (!grp)
		grp = strrchr(own, ':');

	if (grp)
	{
		*grp++ = 0;
		if (isdigit(*grp))
			gid = atol(grp);
	}

	/* cases like `chown .0 filename' */
	if (isdigit(*own))
		uid = atol(own);

	if (uid == st.uid && gid == st.gid)
		return 0;			/* no change */

	return Fchown(fname, uid, gid);
}

static long
sh_chmod(long argc, char **argv)
{
	long d = 0;
	short c;
	char *s;

	if (argc < 3)
	{
		shell_fprintf(STDERR, "%s%s OCTAL-MODE file\r\n", MISSING_ARG, argv[0]);

		return 0;
	}

	s = argv[1];

	while ((c = *s++) != 0 && isodigit(c))
	{
		d <<= 3;
		d += (c & 0x0007);
	}

	d &= S_IALLUGO;

	return Fchmod(argv[2], d);
}

static long
sh_xcmd(long argc, char **argv)
{
	if (argc >= 2)
	{
		if (strcmp(argv[1], "on") == 0)
			xcommands = 1;
		else if (strcmp(argv[1], "off") == 0)
			xcommands = 0;
		else if (strcmp(argv[1], "--help") == 0)
			shell_fprintf(STDOUT, "%s [on|off]\r\n", argv[0]);
	}
	xcmdstate();

	return 0;
}

static long
sh_exit(long argc, char **argv)
{
	short y;

	shell_print("Are you sure to exit and reboot (y/n)?");
	y = (short)Cconin();

	if (tolower(y & 0x00ff) == *MSG_init_menu_yes)
	{
		/* Tell the parent to exit */
		(void)Pkill(Pgetppid(), SIGTERM);

		Pterm(0);
	}
	shell_print("\r\n");

	return 0;
}

static long
sh_cp(long argc, char **argv)
{
	shell_fprintf(STDOUT, "%s not implemented yet!\r\n", argv[0]);

	return -1;
}

static long
sh_mv(long argc, char **argv)
{
	shell_fprintf(STDOUT, "%s not implemented yet!\r\n", argv[0]);

	return -1;
}

static long
sh_rm(long argc, char **argv)
{
	shell_fprintf(STDOUT, "%s not implemented yet!\r\n", argv[0]);

	return -1;
}

static long
sh_ln(long argc, char **argv)
{
	char *old, *new;

	if (argc < 3)
	{
		shell_fprintf(STDERR, "%s%s from-file to-link\r\n", MISSING_ARG, argv[0]);

		return 0;
	}

	old = argv[1];
	new = argv[2];

	return Fsymlink(old, new);
}

static long
sh_setenv(long argc, char **argv)
{
	if (argc < 2)
		env();
	else if (argc == 2)
	{
		if (strcmp(argv[1], "--help") == 0)
			shell_fprintf(STDOUT, "%s [NAME value]\r\n", argv[0]);
		else
			shell_delenv(argv[1]);
	}
	else
		shell_setenv(argv[1], argv[2]);

	return 0;
}

static long
sh_export(long argc, char **argv)
{
	char *arg2;

	if (argc < 2)
		env();
	else
	{
		if (strcmp(argv[1], "--help") == 0)
			shell_fprintf(STDOUT, "%s [NAME=value]\r\n", argv[0]);
		else
		{
			arg2 = strrchr(argv[1], '=');

			if (arg2 == NULL)
				shell_delenv(argv[1]);
			else
			{
				*arg2++ = 0;
				argv[2] = arg2;
				argv[3] = NULL;

				sh_setenv(argc + 1, argv);
			}
		}
	}

	return 0;
}

static long
sh_echo(long argc, char **argv)
{
	short x = 1;

	while (--argc)
		shell_fprintf(STDOUT, "%s ", argv[x++]);

	shell_fprintf(STDOUT, "\r\n");

	return 0;
}

/* End of the commands, begin control routines */

# define MAX_BASIC_CMD	8	/* i.e. echo, counting from exit == 1 */

typedef long FUNC();

static FUNC *cmd_routs[] =
{
	sh_exit, sh_ver, sh_cd, sh_help, sh_xcmd, sh_setenv, sh_export, sh_echo, \
	sh_ls, sh_cp, sh_mv, sh_rm, sh_ln, sh_chmod, sh_chown, sh_chgrp
};

/* Add export/setenv */
static const char *commands[] =
{
	"exit", "ver", "cd", "help", "xcmd", "setenv", "export", "echo", \
	"ls", "cp", "mv", "rm", "ln", "chmod", "chown", "chgrp", NULL
};

/* Execute the given command */
INLINE long
execute(char *cmdline)
{
	char *argv[SHELL_ARGS], newcmd[128], *c;
	long cnt, cmdno, argc;

	c = cmdline;

	/* Convert possible control characters to spaces */
	while (*c)
	{
		if (iscntrl(*c))
			*c = ' ';
		c++;
	}

	c = cmdline;

	/* Skip the first word (command) */
	while (*c && isspace(*c))
		c++;
	while (*c && !isspace(*c))
		c++;

	/* The `newcmd' does not have to be zeroed, because the Atari GEMDOS
	 * manual says that the GEMDOS only copies the command line up to
	 * 125 characters or until it encounters a zero byte. Let's hope
	 * this to be true.
	 */
	strncpy(newcmd, c, 127);		/* crunch() destroys the `cmdline', so we need to copy it */

	argc = crunch(cmdline, argv);

	if (!argc)
		return 0;			/* empty command line */

	/* Result a zero if the string given is not an internal command,
	 * or the number of the internal command otherwise (the number is
	 * just their index in the commands[] table, plus one).
	 */
	cmdno = cnt = 0;

	while (commands[cnt])
	{
		if (strcmp(argv[0], commands[cnt]) == 0)
		{
			cmdno = cnt + 1;
			break;
		}
		cnt++;
	}

	/* If xcommands == 1 internal code is used for the commands
	 * below, or external programs are executed otherwise
	 */
	if ((commands[cnt] == NULL) || (!xcommands && cmdno > MAX_BASIC_CMD))
		cmdno = 0;

	return (cmdno == 0) ? execvp(newcmd, argv) : cmd_routs[cnt](argc, argv);
}

/* XXX because of Cconrs() the command line cannot be longer than 254 bytes.
 */
INLINE char *
prompt(uchar *buffer, long buflen)
{
	char *lbuff, cwd[1024];
	short idx;

	buffer[0] = 254;
	buffer[1] = 0;

	Dgetcwd(cwd, 0, sizeof(cwd));

	if (*cwd == 0)
		strcpy(cwd, "/");
	else
		dos2unix(cwd);

	shell_fprintf(STDOUT, "mint:%s#", cwd);
	Cconrs(buffer);

	/* "the string is not guaranteed to be NULL terminated"
	 * (Atari GEMDOS reference manual)
	 */
	lbuff = buffer + 2;
	idx = buffer[1];
	idx--;
	lbuff[idx] = 0;

	return lbuff;
}

static void
shell(void)
{
	uchar linebuf[256], *lbuff;
	long r, s;

	shell_print("\ee\ev\r\n");	/* enable cursor, enable word wrap, make newline */
	sh_ver(0, NULL);
	xcmdstate();
	shell_print("Type `help' for help\r\n\r\n");

	Dsetdrv('U' - 'A');	/* force current drive to u: */

	sh_cd(1, NULL);		/* this sets $HOME as current dir */

	/* This loop restarts the shell when it dies (in case of a bus error for example) */
	for (;;)
	{
		s = Pvfork();

		/* Child here */
		if (s == 0)
		{
			for (;;)
			{
				lbuff = prompt(linebuf, sizeof(linebuf));

				r = execute(lbuff);

				if (r < 0)
					shell_fprintf(STDERR, "mint: %s: error %ld\r\n", lbuff, r);
			}
		}
		else		/* Parent here */
		{
			(void)Pwaitpid(s, 0, NULL);	/* this is to avoid zombies */
		}
	}
}

/* Notice that we cannot define local variables here due to setstack()
 * which changes the stack pointer value.
 */
static void
shell_start(long bp)
{
	/* we must leave some bytes of `pad' behind the (sp)
	 * (see arch/syscall.S), this is why it is `-256L'.
	 */
	setstack(bp + SHELL_STACK - 256L);

	Mshrink((void *)bp, SHELL_STACK);

	(void)Pdomain(1);			/* switch to MiNT domain */
	(void)Pumask(SHELL_UMASK);		/* files created should be rwxr-xr-x */
	(void)Fforce(2, -1);			/* redirect the stderr to console */

	shell();				/* this one doesn't return */
}

/* This below is running in kernel context, called from init.c
 */
long
startup_shell(void)
{
	long r;

	shell_base = (BASEPAGE *)sys_pexec(7, (char *)SHELL_FLAGS, "\0", init_env);

	/* Hope this never happens */
	if (!shell_base)
	{
		DEBUG(("Pexec(7) returned 0!\r\n"));

		return -1;
	}

	/* Start address */
	shell_base->p_tbase = (long)shell_start;

	r = sys_pexec(104, (char *)"shell", shell_base, 0L);

	m_free((long)shell_base);

	return r;
}

# endif /* BUILTIN_SHELL */

/* EOF */
