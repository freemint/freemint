/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2003 Konrad M.Kokoszkiewicz <draco@atari.org>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * Note:
 *
 * 1) the shell is running as separate process, but in kernel context;
 * 2) the shell is running in supervisor mode;
 * 3) the shell is very minimal, so don't expect much ;-)
 * 4) we are on the system stack, which is small;
 *
 * TODO in random order:
 *
 * - filename completion;
 * - make crunch() expand wildcards;
 * - make possible to execute some sort of init script (shellrc or something);
 * - get rid of Cconrs();
 * - make `ls -l |more' work.
 *
 */

# ifdef BUILTIN_SHELL

# include <stdarg.h>

# include "libkern/libkern.h"

# include "mint/errno.h"
# include "mint/stat.h"		/* struct stat */

# include "console.h"		/* c_conin(), c_conrs() */
# include "dos.h"		/* p_umask(), p_domain() */
# include "dosdir.h"		/* d_opendir(), d_readdir(), d_closedir(), f_symlink(), f_stat64(), ...*/
# include "dosfile.h"		/* f_write() */
# include "dosmem.h"		/* m_xalloc(), m_free(), m_shrink() */
# include "global.h"
# include "info.h"		/* national stuff */
# include "init.h"		/* env_size(), _mint_delenv(), _mint_setenv() */
# include "k_exec.h"		/* sys_pexec() */
# include "k_kthread.h"		/* kthread_create(), kthread_exit() */
# include "proc.h"		/* struct proc */
# include "time.h"		/* struct timeval */

# include "mis.h"

# define STDIN		0
# define STDOUT		1
# define STDERR		2

# define SHELL_UMASK	~(S_ISUID | S_ISGID | S_ISVTX | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

# define SH_VER_MAIOR	1
# define SH_VER_MINOR	0

# define COPYCOPY	"(c) 2003-2004 draco@atari.org\n"

# define SHELL_MAXFN	1024L		/* max length of a filename */
# define SHELL_MAXPATH	6144L		/* max length of a pathname */
# define SHELL_MAXLINE	8192L		/* max line for the shell_fprintf() */

/* this is an average number of seconds in Gregorian year
 * (365 days, 5 hours, 48 minutes, 45 seconds).
 */
# define SEC_OF_YEAR	31556925L

# define LINELEN	254

/* Global variables */
static short xcommands;		/* if 1, the extended command set is active */

/* For i/o redirection */
static long stdout = STDOUT;
static long stderr = STDERR;

/* Fake command line to prevent complaints from the kernfs and fork() */
struct kcmd
{
	long length;
	long argvl;
	short dnull;
};

static struct kcmd shell_cmdline = { sizeof(shell_cmdline), 0L, 0 };

/* Utility routines */

__attribute__((format(printf, 2, 3)))
static void
shell_fprintf(long handle, const char *fmt, ...)
{
	char *buf;
	va_list args;

	buf = (char *)sys_m_xalloc(SHELL_MAXLINE, F_ALTPREF);

	if (buf)
	{
		va_start(args, fmt);
		kvsprintf(buf, SHELL_MAXLINE, fmt, args);
		va_end(args);

		sys_f_write(handle, strlen(buf), buf);

		sys_m_free((long)buf);
	}
	else
		sys_f_write(stderr, 13, "Out of RAM!\n");
}

/* Simple conversion of a pathname from the DOS to Unix format,
 * used mainly for displaying symlinks in `correct' (i.e. Unix) form.
 */
static void
dos2unix(char *p)
{
	if (p[1] == ':')
	{
		if (toupper(p[0]) == 'U')
		{
			/* FIXME: overlapping strcpy() */
			strcpy(p, p + 2);	/* eat off a prefix like 'u:' */
		} else
		{
			/* C:/foo/ -> /c/foo/ */
			p[1] = p[0];
			p[0] = '/';
		}
	}

	while (*p)
	{
		if (*p == '\\')
			*p = '/';
		p++;
	}
}

/* shell_delenv() */
static void
shell_delenv(const char *strng)
{
	_mint_delenv(_base, strng);
}

/* shell_setenv() */
static void
shell_setenv(const char *var, char *value)
{
	_mint_setenv(_base, var, value);
}

/* Split command line into separate strings, put their pointers
 * into argv[], return argc.
 *
 * XXX add wildcard expansion (at least the `*'), see fnmatch().
 */
static short
get_arg(char *cmd, char **arg, char **next, short *io)
{
	char *start = NULL, *endquote;
	short special, r = 0;

	*io = 0;

	/* I/O redirection */
	if (isdigit(cmd[0]) && cmd[1] == '>')
	{
		*io = cmd[0] & 0x0f;
		*cmd++ = 0;
		*cmd++ = 0;
		r = 1;
	}
	else if (cmd[0] == '>')
	{
		*io = 1;
		*cmd++ = 0;
		r = 1;
	}
	/* If the argument is an enviroment variable, evaluate it.
	 * It is assumed, that a space or NULL terminates the argument.
	 *
	 * XXX crap code, doesn't evaluate $HOME/dirname properly.
	 */
	else if (cmd[0] == '$' && isupper(cmd[1]))
	{
		start = cmd + 1;

		while (*cmd && *cmd != '>' && !isspace(*cmd))
			cmd++;

		if (cmd[0] == '>')
		{
			if (isdigit(cmd[-1]))
			{
				*io = cmd[-1] & 0x0f;
				cmd[-1] = 0;
			}
			else
				*io = 1;
		}

		if (*cmd)
			*cmd++ = 0;		/* terminate the tag for getenv() */

		start = _mint_getenv(_base, start);

		if (!start)
			r = 1;			/* ignore and try again */
	}
	/* Check if there may be a quoted argument. A quoted argument will begin with
	 * a quote and end with a quote. Quotes do not belong to the argument.
	 * Everything between quotes is litterally taken as argument, without any
	 * internal interpretation.
	 */
	else if (cmd[0] == '\'')
	{
		cmd++;
		endquote = cmd;

		while (*endquote && *endquote != '\'')
			endquote++;

		if (!*endquote)
			return -1000;		/* unmatched quotes */

		start = cmd;
		cmd = endquote;

		if (*cmd)
			*cmd++ = 0;
	}
	else
	{
	/* Search for the ending separator. In a non-quoted argument any space
	 * is understood as a separator except when preceded by the backslash.
	 */
		start = cmd;

		while (*cmd && *cmd != '>' && !isspace(*cmd))
		{
			special = (*cmd == '\\');
			if (special && cmd[1])
				strcpy(cmd, cmd + 1);	/* physically remove the backslash */
			cmd++;
		}

		if (cmd[0] == '>')
		{
			if (isdigit(cmd[-1]))
			{
				*io = cmd[-1] & 0x0f;
				cmd[-1] = 0;
			}
			else
				*io = 1;
		}
		if (*cmd)
			*cmd++ = 0;
	}

	while (*cmd && isspace(*cmd))
		cmd++;

	*arg = start;
	*next = cmd;

	return r;
}

/* Convert MS commandline into ARGV argument vector.
 * If argv argument is NULL, only count arguments, but
 * don't create the argument vector.
 *
 * Notice: crunch() is destructive for the `cmd'.
 */
static long
crunch(char *cmd, char **argv)
{
	char *argument, *tmp = NULL;
	short r, io, saveio;
	long idx = 0, fd;

	/* Save the original commandline */
	if (argv == NULL)
	{
		tmp = (char *)sys_m_xalloc(strlen(cmd) + 1, F_ALTPREF);
		if (!tmp)
			return ENOMEM;
		strcpy(tmp, cmd);
		cmd = tmp;
	}

	while (*cmd && isspace(*cmd))
		cmd++;

	while (*cmd)
	{
		r = get_arg(cmd, &argument, &cmd, &io);
		/* r == 0 means that getting the argument succeeded;
		 * r < 0 means syntax error;
		 * r == 1 means that next argument has to be skipped;
		 */
		if (r == 0)
		{
			if (argv)
				argv[idx] = argument;
			idx++;
		}
		else if (r < 0)
			return r;
		/* else fall through */

		if (io)
		{
			saveio = io;
			r = get_arg(cmd, &argument, &cmd, &io);
			if (r < 0)
				return r;
			/* r == 1 means that the user has supplied an undefined env. variable
			 * as an argument for I/O redirection. We can't tolerate that this
			 * time. Also, syntax like `ls -l 1> 2> out' is not allowed.
			 */
			else if (!io && r != 1)
			{
				if (argv)
				{
					if ((fd = sys_f_create(argument, 0)) < 0)
						return fd;

					stdout = (saveio == 1) ? fd : stdout;
					stderr = (saveio == 2) ? fd : stderr;
				}
			}
			else
				return -1000;
		}
	}

	if (argv)
		argv[idx] = NULL;

	if (tmp)
		sys_m_free((long)tmp);

	return idx;
}

static char *
env_append(char *where, char *what)
{
	strcpy(where, what);
	where += strlen(where);

	return ++where;
}

/* Execute an external program */
static long
internal_execvp(char **argv)
{
	char *oldcmd, *npath, *var, *new_env, *new_var, *t, *path, *np;
	long count, x, y, r = -1L, blanks = 0;
	short dupout, duperr;

	/* Calculate the size of the environment */
	var = _base->p_env;
	count = env_size(var) + sizeof("ARGV=NULL:");

	/* Add space for the ARGV strings.
	 */
	for (x = 0; argv[x]; x++)
		count += strlen(argv[x]) + 1;	/* trailing NULL */

	count <<= 1;				/* make this twice as big (will be shrunk later) */

	/* Allocate one buffer that would satisfy all requirements of this function */
	oldcmd = (char *)sys_m_xalloc(count + 128 + SHELL_MAXPATH + SHELL_MAXFN, F_ALTPREF);
	if (!oldcmd)
		return ENOMEM;

	/* First part is the command line and path buffer.
	 * Last part is the environment buffer. Hope these
	 * will never overlap.
	 */
	new_env = oldcmd + 128 + SHELL_MAXPATH + SHELL_MAXFN;

	var = _base->p_env;
	new_var = new_env;

	/* Copy variables from `var' to `new_var', but don't copy ARGV strings
	 */
	while (var && *var)
	{
		if (strncmp(var, "ARGV=", 5) == 0)
			break;
		while (*var)
			*new_var++ = *var++;
		*new_var++ = *var++;
	}

	/* Append new ARGV strings.
	 *
	 * First check if there are NULL arguments.
	 */
	for (x = 0; !blanks && argv[x]; x++)
		blanks = (argv[x][0] == 0);

	/* Now create the ARGV variable.
	 */
	if (blanks)
	{
		new_var = env_append(new_var, "ARGV=NULL:");
		new_var--;

		for (x = 0; argv[x]; x++)
		{
			if (argv[x][0] == 0)
			{
				ksprintf(new_var, 8, "%ld,", x);
				new_var += strlen(new_var);
			}
		}
		new_var[-1] = 0;	/* kill the last comma */
	}
	else
		new_var = env_append(new_var, "ARGV=");

	/* Append the argument strings.
	 */
	for (x = 0; argv[x]; x++)
		new_var = env_append(new_var, argv[x][0] ? argv[x] : " ");

	*new_var = 0;

	(void)sys_m_shrink(0, (long)oldcmd, env_size(new_env) + SHELL_MAXPATH + SHELL_MAXFN + 128);

	/* Since crunch() evaluates some arguments, we have now to
	 * re-create the old GEMDOS style command line string.
	 */
	npath = oldcmd + 128;
	mint_bzero(oldcmd, 128);

	x = 1;			/* must skip the first argument (program name) */
	y = 0;
	while (y < 126 && argv[x])
	{
		oldcmd[y++] = ' ';
		strncpy_f(oldcmd + y, argv[x][0] ? argv[x] : "''", 126 - y);
		y += strlen(oldcmd + y);
		x++;
	}
	oldcmd[0] = 0x7f;

	/* $PATH searching. Don't do it, if the user seems to
	 * have specified the explicit pathname.
	 */
	t = strrchr(argv[0], '/');

	path = "";

	if (t == NULL)
	{
		path = _mint_getenv(_base, "PATH");
		if (path == NULL)
			path = "./";
	}

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

		/* If stdout/stderr vary from their default values, it means
		 * that crunch() has redirected the output.
		 */
		dupout = duperr = 0;

		if (stdout != STDOUT)
		{
			dupout = sys_f_dup(STDOUT);		/* duplicate the stdout */
			sys_f_force(STDOUT, stdout);
		}

		if (stderr != STDERR)
		{
			duperr = sys_f_dup(STDERR);		/* duplicate the stderr */
			sys_f_force(STDERR, stderr);
		}

		r = sys_pexec(0, npath, oldcmd, new_env);

		if (dupout)
		{
			sys_f_force(STDOUT, dupout);
			sys_f_close(dupout);
		}

		if (duperr)
		{
			sys_f_force(STDERR, duperr);
			sys_f_close(duperr);
		}
	}
	while (*path && r < 0);

	sys_m_free((long)oldcmd);

	return r;
}

static void
xcmdstate(void)
{
	shell_fprintf(stdout, MSG_shell_xcmd_info, xcommands ? MSG_shell_xcmd_on : MSG_shell_xcmd_off);
}

/* End utilities, begin commands */

static long
sh_ver(long argc, char **argv)
{
	shell_fprintf(stdout, MSG_shell_name, SH_VER_MAIOR, SH_VER_MINOR, COPYCOPY);

	return 0;
}

static long
sh_help(long argc, char **argv)
{
	shell_fprintf(stdout, MSG_shell_help, xcommands ? MSG_shell_xcmd_on : MSG_shell_xcmd_off);

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
	short all = 0, full = 0;
	long i, x, r, s, handle;
	char *dir, *path, *link, *entry;

	dir = ".";

	/* Accept options like --help, -a and -l */
	for (x = 1; x < argc; x++)
	{
		if (argv[x][0] == '-')
		{
			if (strcmp(argv[x], "--help") == 0)
			{
				shell_fprintf(stdout, MSG_shell_ls_help, argv[0]);

				return 0;
			}
			else
			{
				for (i = 1; argv[x][i]; i++)
				{
					if (argv[x][i] == 'a')
						all = 1;
					else if (argv[x][i] == 'l')
						full = 1;
				}
			}
		}
		else
		{
			dir = argv[x];
			break;
		}
	}

	handle = sys_d_opendir(dir, 0);
	if (handle < 0)
		return handle;

	sys_t_gettimeofday(&tv, NULL);

	path = (char *)sys_m_xalloc((SHELL_MAXPATH * 2) + SHELL_MAXFN, F_ALTPREF);
	link = path + SHELL_MAXPATH;
	entry = link + SHELL_MAXPATH;

	if (!path)
		return ENOMEM;

	do
	{
		r = sys_d_readdir(SHELL_MAXFN, handle, entry);

		/* Display only names not starting with a dot, unless -a was specified */
		if (r == 0 && (entry[sizeof(long)] != '.' || all))
		{
			strcpy(path, dir);
			strcat(path, "/");
			strcat(path, entry + sizeof(long));

			/* `/bin/ls -l' doesn't dereference links, so we don't either */
			s = sys_f_stat64(1, path, &st);
			if (s == 0)
			{
				if (S_ISLNK(st.mode))
				{
					s = sys_f_readlink(SHELL_MAXPATH, link, path);
					if (s == 0)
						dos2unix(link);
				}

				if (full)
				{
					unsigned short year, month, day, hour, minute;

					/* Reuse the path[] space for attributes */
					strcpy(path, "?---------");

					if (S_ISLNK(st.mode))		/* file type */
						path[0] = 'l';
					else if (S_ISMEM(st.mode))
						path[0] = 'm';
					else if (S_ISFIFO(st.mode))
						path[0] = 'p';
					else if (S_ISREG(st.mode))
						path[0] = '-';
					else if (S_ISBLK(st.mode))
						path[0] = 'b';
					else if (S_ISDIR(st.mode))
						path[0] = 'd';
					else if (S_ISCHR(st.mode))
						path[0] = 'c';
					else if (S_ISSOCK(st.mode))
						path[0] = 's';

					/* access attibutes: user */
					if (st.mode & S_IRUSR)
						path[1] = 'r';
					if (st.mode & S_IWUSR)
						path[2] = 'w';
					if (st.mode & S_IXUSR)
						path[3] = (st.mode & S_ISUID) ? 's' : 'x';

					/* ... group */
					if (st.mode & S_IRGRP)
						path[4] = 'r';
					if (st.mode & S_IWGRP)
						path[5] = 'w';
					if (st.mode & S_IXGRP)
						path[6] = (st.mode & S_ISGID) ? 's' : 'x';

					/* ... others */
					if (st.mode & S_IROTH)
						path[7] = 'r';
					if (st.mode & S_IWOTH)
						path[8] = 'w';
					if (st.mode & S_IXOTH)
						path[9] = (st.mode & S_ISVTX) ? 't' : 'x';

					/* Now recalculate the time stamp */
					unix2calendar(st.mtime.time, &year, &month, &day, &hour, &minute, NULL);

					shell_fprintf(stdout, "%s %5d %8ld %8ld %8ld %s %2d ", \
							path, (short)st.nlink, st.uid, st.gid, \
							(long)st.size, months_abbr_3[month - 1], day);

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
						shell_fprintf(stdout, "%5d ", year);
					else
						shell_fprintf(stdout, "%02d:%02d ", hour, minute);
				}

				shell_fprintf(stdout, "%s", entry + sizeof(long));

				if (S_ISLNK(st.mode))
					shell_fprintf(stdout, " -> %s\n", link);
				else
					shell_fprintf(stdout, "\n");
			}
		}
	} while (r == 0);

	sys_m_free((long)path);

	sys_d_closedir(handle);

	return 0;
}

/* Change cwd to the given path. If none given, change to $HOME
 */
static long
sh_cd(long argc, char **argv)
{
	long r;
	char *newdir, *pwd, *cwd, *opwd;

	if (argc >= 2)
	{
		if (strcmp(argv[1], "--help") == 0)
		{
			shell_fprintf(stdout, MSG_shell_cd_help, argv[0]);

			return 0;
		}
		else
			newdir = argv[1];
	}
	else
	{
		newdir = _mint_getenv(_base, "HOME");
		if (!newdir)
			newdir = "/";
	}

	r = sys_d_setpath(newdir);

	if (r == 0)
	{
		cwd = (char *)sys_m_xalloc(SHELL_MAXPATH, F_ALTPREF);
		if (!cwd)
			return ENOMEM;

		sys_d_getcwd(cwd, 0, SHELL_MAXPATH);

		if (*cwd == 0)
			strcpy(cwd, "/");
		else
			dos2unix(cwd);

		pwd = _mint_getenv(_base, "PWD");

		if (pwd && strcmp(pwd, cwd))
		{
			opwd = (char *)sys_m_xalloc(SHELL_MAXPATH, F_ALTPREF);
			if (opwd)
			{
				strcpy(opwd, pwd);
				shell_setenv("OLDPWD", opwd);
				sys_m_free((long)opwd);
			}
		}

		shell_setenv("PWD", cwd);

		sys_m_free((long)cwd);
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
		shell_fprintf(stderr, "%s%s\n", argv[0], MSG_shell_missing_arg);

		return 0;
	}

	fname = argv[2];

	r = sys_f_stat64(0, fname, &st);
	if (r < 0)
		return r;

	gid = atol(argv[1]);

	if (gid == st.gid)
		return 0;			/* no change */

	return sys_f_chown(fname, st.uid, gid);
}

static long
sh_chown(long argc, char **argv)
{
	struct stat st;
	long r, uid, gid;
	char *own, *fname, *grp;

	if (argc < 3)
	{
		shell_fprintf(stderr, "%s%s\n", argv[0], MSG_shell_missing_arg);

		return 0;
	}

	own = argv[1];
	fname = argv[2];

	r = sys_f_stat64(0, fname, &st);
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

	return sys_f_chown(fname, uid, gid);
}

static long
sh_chmod(long argc, char **argv)
{
	long d = 0;
	short c;
	char *s;

	if (argc < 3)
	{
		shell_fprintf(stderr, "%s%s\n", argv[0], MSG_shell_missing_arg);

		return 0;
	}

	s = argv[1];

	while ((c = *s++) != 0 && isodigit(c))
	{
		d <<= 3;
		d += (c & 0x0007);
	}

	d &= S_IALLUGO;

	return sys_f_chmod(argv[2], d);
}

static long
sh_xcmd(long argc, char **argv)
{
	if (argc >= 2)
	{
		if (strcmp(argv[1], "on") == 0)		/* don't change this to `MSG_shell_xcmd_on' */
			xcommands = 1;
		else if (strcmp(argv[1], "off") == 0)
			xcommands = 0;
		else if (strcmp(argv[1], "--help") == 0)
			shell_fprintf(stdout, MSG_shell_xcmd_help, argv[0]);
	}
	xcmdstate();

	return 0;
}

static long
sh_exit(long argc, char **argv)
{
	short y;

	shell_fprintf(stdout, MSG_shell_exit_q);

	y = (short)sys_c_conin();

	if (tolower(y & 0x00ff) == *MSG_init_menu_yes)
		sys_s_hutdown(SHUT_COLD);

	shell_fprintf(stdout, "\n");

	return 0;
}

/* Variants:
 *
 * file -> file
 * file file file file ... -> directory
 */

# define SHELL_COPYBUF	65536L

static long
copy_from_to(short move, char *from, char *to)
{
	char *buf;
	struct stat st, tost;
	long r, sfd, dfd, bufsize;
	llong size;

	r = sys_f_stat64(0, from, &st);
	if (r < 0)
		return r;

	r = sys_f_stat64(0, to, &tost);
	if (r == 0)
	{
		if (st.ino == tost.ino)
		{
			shell_fprintf(stderr, MSG_shell_cp_the_same, from, to);

			return 0;
		}
	}

	if (move)
	{
		r = sys_f_rename(0, from, to);
		if (r != EXDEV)
			return r;
	}

	sfd = sys_f_open(from, O_RDONLY);
	if (sfd < 0)
		return sfd;
	dfd = sys_f_create(to, 0);
	if (dfd < 0)
	{
		sys_f_close(sfd);
		return dfd;
	}

	buf = (char *)sys_m_xalloc(SHELL_COPYBUF, F_ALTPREF);

	if (buf)
	{
		size = st.size;

		while (size)
		{
			if (size > SHELL_COPYBUF)
				bufsize = SHELL_COPYBUF;
			else
				bufsize = size;
			r = sys_f_read(sfd, bufsize, buf);
			if (r == bufsize)
				r = sys_f_write(dfd, bufsize, buf);
			if (r < 0)
				break;
			size -= bufsize;
		}

		sys_m_free((long)buf);

		if (r >= 0)
		{
			sys_f_chmod(to, st.mode & S_IALLUGO);
			sys_f_chown(to, st.uid, st.gid);
		}
		else
			sys_f_delete(to);

		if (move)
			sys_f_delete(from);

	}
	else
		r = ENOMEM;

	sys_f_close(sfd);
	sys_f_close(dfd);

	return r;
}

static long
sh_cp(long argc, char **argv)
{
	char *path;
	struct stat st;
	long r = 0, x;

	if (argc < 3)
	{
		shell_fprintf(stderr, "%s%s\n", argv[0], MSG_shell_missing_arg);

		return 0;
	}

	path = (char *)sys_m_xalloc(SHELL_MAXPATH, F_ALTPREF);
	if (!path)
		return ENOMEM;

	r = sys_f_stat64(0, argv[argc-1], &st);

	if (r < 0 || (r == 0 && !S_ISDIR(st.mode)))
	{
		if (argc > 3)
			shell_fprintf(stderr, MSG_shell_cp_not_dir, argv[0], argv[argc-1]);
		else
			r = copy_from_to(0, argv[1], argv[2]);
	}
	else
	{
		for (x = 1; x < (argc - 1); x++)
		{
			strcpy(path, argv[argc-1]);
			strcat(path, "/");
			strcat(path, argv[x]);

			r = copy_from_to(0, argv[x], path);

			if (r < 0)
				break;
		}
	}

	sys_m_free((long)path);

	return r;
}

static long
sh_mv(long argc, char **argv)
{
	char *path;
	struct stat st;
	long r = 0, x;

	if (argc < 3)
	{
		shell_fprintf(stderr, "%s%s\n", argv[0], MSG_shell_missing_arg);

		return 0;
	}

	path = (char *)sys_m_xalloc(SHELL_MAXPATH, F_ALTPREF);
	if (!path)
		return ENOMEM;

	r = sys_f_stat64(0, argv[argc-1], &st);

	if (r < 0 || (r == 0 && !S_ISDIR(st.mode)))
	{
		if (argc > 3)
			shell_fprintf(stderr, MSG_shell_cp_not_dir, argv[0], argv[argc-1]);
		else
			r = copy_from_to(1, argv[1], argv[2]);
	}
	else
	{
		for (x = 1; x < (argc - 1); x++)
		{
			strcpy(path, argv[argc-1]);
			strcat(path, "/");
			strcat(path, argv[x]);

			r = copy_from_to(1, argv[x], path);

			if (r < 0)
				break;
		}
	}

	sys_m_free((long)path);

	return r;
}

static long
sh_rm(long argc, char **argv)
{
	long x, r;
	short force = 0;

	if (argc < 2)
	{
		shell_fprintf(stderr, "%s%s\n", argv[0], MSG_shell_missing_arg);

		return 0;
	}

	for (x = 1; x < argc; x++)
	{
		if (argv[x][0] == '-')
		{
			if (strcmp(argv[x], "--help") == 0)
			{
				shell_fprintf(stdout, MSG_shell_rm_help, argv[0]);

				return 0;
			}
			else if (strcmp(argv[x], "-f") == 0)
				force = 1;
		}
		else
			break;
	}

	while (argv[x])
	{
		r = sys_f_delete(argv[x++]);
		if (!force && r < 0)
			return r;
	}

	return 0;
}

static long
sh_ln(long argc, char **argv)
{
	char *old, *new;

	if (argc < 3)
	{
		shell_fprintf(stderr, "%s%s\n", argv[0], MSG_shell_missing_arg);

		return 0;
	}

	old = argv[1];
	new = argv[2];

	return sys_f_symlink(old, new);
}

static long
sh_setenv(long argc, char **argv)
{
	if (argc < 2)
	{
		char *var = _base->p_env;

		while (var && *var)
		{
			shell_fprintf(stdout, "%s\n", var);
			while (*var)
				var++;
			var++;
		}
	}
	else if (argc == 2)
	{
		if (strcmp(argv[1], "--help") == 0)
			shell_fprintf(stdout, MSG_shell_setenv_help, argv[0]);
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
		sh_setenv(argc, argv);
	else
	{
		if (strcmp(argv[1], "--help") == 0)
			shell_fprintf(stdout, MSG_shell_export_help, argv[0]);
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
	short x = 0;

	while (--argc)
	{
		shell_fprintf(stdout, "%s", argv[++x]);
		if (argv[x + 1])
			shell_fprintf(stdout, " ");
	}

	shell_fprintf(stdout, "\n");

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

static const char *commands[] =
{
	/* First eight commands are always active */
	"exit", "ver", "cd", "help", "xcmd", "setenv", "export", "echo", \
	/* The rest is switchable by `xcmd on' or `off' */
	"ls", "cp", "mv", "rm", "ln", "chmod", "chown", "chgrp", NULL
};

/* Execute the given command line */
static long
execute(char *cmdline)
{
	char *c, **argv;
	long r = 0, cnt, cmdno, argc, x, cmde;

	cmde = strlen(cmdline) - 1;

	c = cmdline;

	/* Convert possible control characters to spaces
	 * and double quotes to single quotes.
	 */
	while (*c)
	{
		if (iscntrl(*c))
			*c = ' ';
		if (*c == '"')
			*c = '\'';
		c++;
	}

	/* To allocate the proper amount of memory for the ARGV we first
	 * have to count the arguments. We achieve this by calling crunch()
	 * with NULL as second argument.
	 */
	argc = crunch(cmdline, NULL);

	if (argc <= 0)				/* empty command line or parse error */
	{
		if (argc == -1000)
			shell_fprintf(stderr, MSG_shell_syntax_error, argc);

		return argc;
	}

	/* Construct the proper ARGV stuff now */
	argv = (char **)sys_m_xalloc(argc * sizeof(void *), F_ALTPREF);
	if (!argv)
		return ENOMEM;

	/* I/O redirections occur here */
	crunch(cmdline, argv);

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

	r = (cmdno == 0) ? internal_execvp(argv) : cmd_routs[cnt](argc, argv);

	for (x = 1; x < argc; x++)
	{
		if (argv[x] && !(((long)argv[x] >= (long)cmdline) && ((long)argv[x] <= ((long)cmdline + cmde))))
			sys_m_free((long)argv[x]);
	}

	sys_m_free((long)argv);

	return r;
}

/* XXX because of Cconrs() the command line cannot be longer than 254 bytes.
 */
static char *
prompt(unsigned char *buffer, long buflen)
{
	char *lbuff, *cwd;
	short idx;

	buffer[0] = LINELEN;
	buffer[1] = 0;

	cwd = (char *)sys_m_xalloc(SHELL_MAXPATH, F_ALTPREF);

	if (cwd)
	{
		sys_d_getcwd(cwd, 0, SHELL_MAXPATH);

		if (*cwd == 0)
			strcpy(cwd, "/");
		else
			dos2unix(cwd);
	}

	shell_fprintf(stdout, "%s:%s#", _mint_getenv(_base, "HOSTNAME") ?: "mint", cwd ?: "");

	sys_c_conrs((char *)buffer);

	/* "the string is not guaranteed to be NULL terminated"
	 * (Atari GEMDOS reference manual)
	 */
	lbuff = (char *)buffer + 2;
	idx = buffer[1];
	lbuff[--idx] = 0;

	if (cwd)
		sys_m_free((long)cwd);

	return lbuff;
}

static void
shell(void *arg)
{
	char linebuf[LINELEN + 2];
	char *lbuff;
	long r;

	(void)sys_p_domain(DOM_MINT);		/* switch to MiNT domain */
	(void)sys_f_force(STDERR, -1);		/* redirect the stderr to console */
	(void)sys_p_umask(SHELL_UMASK);		/* files created should be rwxr-xr-x */

	/* If there is no $PATH defined, enable internal commands by default
	 */
	xcommands = (_mint_getenv(_base, "PATH") == NULL);

	shell_fprintf(stdout, "\ee\ev\n");	/* enable cursor, enable word wrap, make newline */
	sh_ver(0, NULL);			/* printout the version number */
	xcmdstate();				/* print 'Extended commands are ...' */
	shell_fprintf(stdout, MSG_shell_type_help);

	sys_d_setdrv('U' - 'A');
	sh_cd(1, NULL);				/* this sets $HOME as current dir */

	for (;;)
	{
		lbuff = prompt((unsigned char*)linebuf, sizeof(linebuf));

		r = execute((char *)lbuff);

		/* Terminate the I/O redirection */
		if (stdout != STDOUT)
		{
			sys_f_close(stdout);
			stdout = STDOUT;
		}

		if (stderr != STDERR)
		{
			sys_f_close(stderr);
			stderr = STDERR;
		}

		if (r < 0)
			shell_fprintf(stderr, MSG_shell_error, lbuff, r);
	}

	kthread_exit (0);
	/* not reached */
}

long
startup_shell(void)
{
	struct proc *p;
	long r;

	r = kthread_create(NULL, shell, NULL, &p, "shell");
	if (r)
	{
		DEBUG(("can't create \"shell\" kernel thread"));
		return -1;
	}

	p->real_cmdline = (char *)&shell_cmdline;	/* fake cmdline */

	return p->pid;
}

# endif /* BUILTIN_SHELL */
