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

# include "libkern/libkern.h"

# include "mint/basepage.h"	/* BASEPAGE struct */
# include "mint/mem.h"		/* F_FASTLOAD et contubernales */
# include "mint/stat.h"		/* struct stat */

# include "arch/cpu.h"		/* setstack() */
# include "arch/startup.h"	/* _base */
# include "arch/syscall.h"	/* Pexec(), Cconrs() et cetera */

# include "cnf.h"		/* init_env */
# include "info.h"		/* national stuff */
# include "init.h"		/* boot_printf */
# include "k_exec.h"		/* sys_pexec */
# include "k_exit.h"		/* sys_pwaitpid */

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

# define SH_VER_MAIOR	0
# define SH_VER_MINOR	1

# define COPYCOPY	"MiS v.%d.%d, the FreeMiNT internal shell,\r\n" \
			"(c) 2003 by Konrad M.Kokoszkiewicz (draco@atari.org)\r\n"

# define SHELL_STACK	32768L
# define SHELL_ARGS	2048L
# define SHELL_FLAGS	(F_FASTLOAD | F_ALTLOAD | F_ALTALLOC | F_PROT_P)

# define LINELEN	126

# define SHCMD_EXIT	1
# define SHCMD_VER	2
# define SHCMD_LS	3
# define SHCMD_CD	4
# define SHCMD_CP	5
# define SHCMD_MV	6
# define SHCMD_RM	7
# define SHCMD_CHMOD	8
# define SHCMD_HELP	9
# define SHCMD_LN	10
# define SHCMD_EXEC	11
# define SHCMD_ENV	12
# define SHCMD_CHOWN	13
# define SHCMD_CHGRP	14
# define SHCMD_XCMD	15

/* Global variables */
static BASEPAGE *shell_base;
static short xcommands;

static short current_year;		/* sh_ls() wants to know this */
static struct timeval ctv;

static const char *commands[] =
{
	"exit", "ver", "ls", "cd", "cp", "mv", "rm", "chmod", \
	"help", "ln", "exec", "env", "chown", "chgrp", "xcmd", 0
};

static const char *months[] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", \
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* Used twice, don't INLINE */
static void
xcmdstate(void)
{
	boot_printf("Extended commands are %s\r\n", xcommands ? "on" : "off");
}

/* Idem */
static void
ver(void)
{
	boot_printf(COPYCOPY, SH_VER_MAIOR, SH_VER_MINOR);
}

INLINE void
help(void)
{
	boot_print( \
	"	MiS is not intended to be a regular system shell, so don't\r\n" \
	"	expect much. It is only a tool to fix bigger problems that\r\n" \
	"	prevent the system from booting normally.\r\n" \
	"\r\n" \
	"	Basic commands are:\r\n" \
	"\r\n" \
	"	cd - change directory\r\n" \
	"	exit - leave and reboot\r\n" \
	"	help - display this message\r\n" \
	"	ver - display version information\r\n" \
	"	xcmd - switch the extended command set on/off\r\n"
	"\r\n");

	/* boot_printf() can only expand strings up to 128 bytes :-( */
	boot_printf("	Extended commands (now %s) are:\r\n\r\n", xcommands ? "on" : "off");

	boot_print( \
	"	chgrp - change the group a file belongs to\r\n" \
	"	chmod - change the access permissions for a file\r\n" \
	"	chown - change file's ownership\r\n" \
	"	*cp - copy file\r\n" \
	"	env - display environment\r\n" \
	"	*ln - create a link\r\n" \
	"	ls - display directory\r\n" \
	"	*mv - move/rename a file\r\n" \
	"	*rm - delete a file\r\n" \
	"\r\n" \
	"	All other words typed are understood as names of programs\r\n" \
	"	to execute. In case you'd want to execute something, that\r\n" \
	"	is named with one of the words displayed above, use 'exec'\r\n" \
	"	or supply the full pathname." \
	"\r\n");
}

/* Display all files in the current directory, with attributes et ceteris.
 * No wilcards filtering what to display, no sorted output, no nothing.
 */

/* Helpers for ls:
 * justify_left() - just pads the text with spaces upto given lenght
 * justify_right() - shifts the text right adding spaces on the left
 *		until the given length is reached.
 */
static char *
justify_left(char *p, long spaces)
{
	long s = (spaces - strlen(p));

	while (*p && *p != 0x20)
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
	if (s <= 0)
		return (p + plen);
	strcpy(temp, p);
	while (s)
	{
		*p++ = ' ';
		s--;
	}
	strcpy(p, temp);

	return (p + plen);
}

INLINE long
sh_ls(char *dir)
{
	struct stat st;
	short year, month, day, hour, minute;
	long r, s, handle, gemdos_time;
	char *p, path[1024];
	char entry[256];

	if (!dir || *dir == 0)
		dir = "./";

	r = Dopendir(dir, 0);
	if (r < 0)
		return r;

	handle = r;

	do
	{
		r = Dreaddir(sizeof(entry), handle, entry);

		if (r == 0)
		{
			strcpy(path, dir);
			strcat(path, "/");
			strcat(path, entry + sizeof(long));

			s = Fstat64(0, path, &st);

			if (s == 0)
			{
				/* Reuse the path[] space */
				p = path;

				if (S_ISLNK(st.mode))
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

				/* user */
				*p++ = (st.mode & S_IRUSR) ? 'r' : '-';
				*p++ = (st.mode & S_IWUSR) ? 'w' : '-';
				if (st.mode & S_IXUSR)
					*p++ = (st.mode & S_ISUID) ? 's' : 'x';
				else
					*p++ = '-';

				/* group */
				*p++ = (st.mode & S_IRGRP) ? 'r' : '-';
				*p++ = (st.mode & S_IWGRP) ? 'w' : '-';
				if (st.mode & S_IXGRP)
					*p++ = (st.mode & S_ISGID) ? 's' : 'x';
				else
					*p++ = '-';

				/* others */
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
				p = justify_right(p, 4);
				*p++ = ' ';

				ksprintf(p, sizeof(path), "%ld", st.uid);
				p = justify_left(p, 9);

				ksprintf(p, sizeof(path), "%ld", st.gid);
				p = justify_left(p, 9);

				/* XXX this will cause problems, if st.size > 2GB */
				ksprintf(p, sizeof(path), "%ld", (long)st.size);
				p = justify_right(p, 8);
				*p++ = ' ';

				gemdos_time = unix2xbios(st.mtime.time);
				gemdos_time >>= 5;				/* discard seconds */

				minute = (short)(gemdos_time & 0x0000003fL);
				gemdos_time >>= 6;

				hour = (short)(gemdos_time & 0x0000001fL);
				gemdos_time >>= 5;

				day = (short)(gemdos_time & 0x0000001fL);
				gemdos_time >>= 5;

				month = (short)(gemdos_time & 0x0000000fL);
				gemdos_time >>= 4;

				year = (short)(gemdos_time & 0x0000007fL);
				year += 1980;

				ksprintf(p, sizeof(path), "%s", months[month - 1]);
				p = justify_left(p, 4);

				ksprintf(p, sizeof(path), "%d", day);
				p = justify_left(p, 3);

				if ((ctv.tv_sec - st.mtime.time) > 15768000L)	/* approx. number of seconds in 6 months */
				{
					ksprintf(p, sizeof(path), "%d", year);
					p = justify_right(p, 5);
				}
				else
				{
					ksprintf(p, sizeof(path), "%d", hour);
					p = justify_right(p, 2);
					*p++ = ':';
					if (minute < 10)
						*p++ = '0';
					ksprintf(p, sizeof(path), "%d", minute);
					while (*p) p++;
				}

				*p++ = ' ';

				*p = 0;

				boot_print(path);
				boot_print(entry + sizeof(long));
				boot_print("\r\n");
			}
		}
	} while (r == 0);

	(void)Dclosedir(handle);

	return 0;
}

/* chgrp, chown, chmod: change various attributes */
INLINE long
sh_chgrp(char *grp, char *fname)
{
	struct stat st;
	long r, gid;

	r = Fstat64(0, fname, &st);
	if (r < 0)
		return r;

	gid = atol(grp);

	if (gid == st.gid)
		return 0;			/* no change */

	return Fchown(fname, st.uid, gid);
}

INLINE long
sh_chown(char *own, char *fname)
{
	struct stat st;
	long r, uid, gid;
	char *grp;

	r = Fstat64(0, fname, &st);
	if (r < 0)
		return r;

	grp = strrchr(own, '.');
	if (!grp)
		grp = strrchr(own, ':');

	if (grp)
	{
		*grp++ = 0;
		gid = atol(grp);
	}
	else
		gid = st.gid;

	uid = atol(own);

	if (uid == st.uid && gid == st.gid)
		return 0;			/* no change */

	return Fchown(fname, uid, gid);
}

INLINE long
sh_chmod(char *attr, char *fname)
{
	long d = 0;
	short c;
	char *s;

	s = attr;

	while ((c = *s++) != 0 && (c >= '0' && c <= '7'))
	{
		d <<= 3;
		d += (c & 0x0007);
	}

	d &= 0x00007fffL;

	return Fchmod(fname, d);
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
	where++;

	return where;
}

static char *
sh_getenv(const char *var)
{
	char *es, *env_str = shell_base->p_env;
	long vl, r;

	if (!env_str || *env_str == 0)
	{
		boot_printf("mint: %s: no environment string!\r\n", __FUNCTION__);

		return 0;
	}

	vl = strlen(var);

	while (*env_str)
	{
		r = strncmp(env_str, var, vl);
		if (r == 0)
		{
			es = env_str;

			while (*es != '=')
				es++;
			es++;

			return es;
		}
		while (*env_str)
			env_str++;
		env_str++;
	}

	return 0;
}

/* `static void' when it gets used more often (once for now)
 */
INLINE void
sh_setenv(const char *var, char *value)
{
	char *env_str = shell_base->p_env;
	char *new_env, *old_var, *es, *ne;
	long new_size, varlen;

	varlen = strlen(var);
	new_size = env_size();
	new_size += strlen(value);

	old_var = sh_getenv(var);
	if (old_var)
		new_size -= strlen(old_var);	/* this is the VALUE of the var */
	else
		new_size += varlen;		/* this is its NAME */

	new_size++;				/* trailing zero */

	new_env = (char *)Mxalloc(new_size, 3);
	if (!new_env)
		return;

	bzero(new_env, new_size);

	es = env_str;
	ne = new_env;

	/* Copy old env to new place skipping `var' */
	while (*es)
	{
		if (strncmp(es, var, varlen) == 0)
		{
			while (*es)
				es++;
			es++;
		}
		else
		{
			while (*es)
				*ne++ = *es++;
			*ne++ = *es++;
		}
	}

	strcpy(ne, var);
	strcat(ne, value);

	ne += strlen(ne);
	ne++;
	*ne = 0;

	shell_base->p_env = new_env;

	Mfree(env_str);
}

INLINE void
env(char *envie)
{
	char *var;

	if (envie)
		var = envie;		/* debug reasons */
	else
		var = shell_base->p_env;

	if (!var || *var == 0)
	{
		boot_printf("mint: %s: no environment string!\r\n", __FUNCTION__);

		return;
	}

	while (*var)
	{
		boot_print(var);	/* avoid excessing 128 char limit for boot_printf() */
		boot_print("\r\n");
		while (*var)
			var++;
		var++;
	}
}

/* Split command line into separate strings and put their pointers
 * into args[].
 *
 * XXX add 'quoted arguments' handling.
 * XXX add wildcard expansion (at least the `*'), see fnmatch().
 *
 */
INLINE void
crunch(char *cmdline, char *args[])
{
	char *cmd = cmdline, *start;
	short idx = 0;
	long cmdlen;

	cmdlen = strlen(cmdline);

	do
	{
		while (cmdlen && *cmd == 0x20)			/* kill leading spaces */
		{
			cmd++;
			cmdlen--;
		}

		start = cmd;

		while (cmdlen && *cmd != 0x20)
		{
			cmd++;
			cmdlen--;
		}

		if (start == cmd)
		{
			args[idx] = 0;

			return;
		}
		else if (*cmd)
		{
			*cmd = 0;
			cmd++;
			cmdlen--;
		}

		args[idx] = start;
		idx++;

	} while (cmdlen > 0);

	args[idx] = 0;
}

/* Execute an external program */
INLINE long
execvp(char *oldcmd, char *args[])
{
	char *var, *new_env, *new_var, *t, *path, *np, npath[2048];
	long count = 0, x, r = -1L;

	var = shell_base->p_env;

	/* Check the size of the environment */
	count = env_size();

	count++;			/* trailing zero */

	count += strlen(oldcmd + 1);	/* add some space for the ARGV= strings */
	count += sizeof("ARGV=");	/* this is 6 bytes */

	new_env = (char *)Mxalloc(count + 1, 3);

	if (!new_env)
		return -40;

	bzero(new_env, count + 1);

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

	while (args[x])
	{
		new_var = env_append(new_var, args[x]);
		x++;
	}
	*new_var = 0;

	*oldcmd = 0x7f;

	/* $PATH searching. Don't do it, if the user seems to
	 * have specified the explicit pathname.
	 */
	t = strrchr(args[0], '/');
	if (!t)
	{
		path = sh_getenv("PATH=");

		if (path)
		{
			do
			{
				np = npath;

				while (*path && *path != ';' && *path != ',')
					*np++ = *path++;
				if (*path)
					path++;			/* skip the separator */

				*np = 0;
				strcat(npath, "/");
				strcat(npath, args[0]);

				r = Pexec(0, npath, oldcmd, new_env);

			} while (*path && r < 0);
		}
	}
	else
		r = Pexec(0, args[0], oldcmd, new_env);

	Mfree(new_env);

	return r;
}

/* Execute the given command */
INLINE long
execute(char *cmdline, char *args[])
{
	char newcmd[128], *c, *home;
	long r, cnt, cmdno;
	short y;

	c = cmdline;

	/* Convert possible CR/LF characters to spaces */
	while (*c)
	{
		if (*c == '\r')
			*c = 0x20;
		if (*c == '\n')
			*c = 0x20;
		c++;
	}

	c = cmdline;

	/* Skip the first word (command) */
	while (*c && *c == 0x20)
		c++;
	while (*c && *c != 0x20)
		c++;

	bzero(newcmd, sizeof(newcmd));

	strncpy(newcmd, c, 127);		/* crunch() destroys the `cmdline', so we need to copy it */

	crunch(cmdline, args);

	if (!args[0])
		return 0;			/* empty command line */

	/* Result a zero if the string given is not an internal command,
	 * or the number of the internal command otherwise (the number is
	 * just their index in the commands[] table, plus one).
	 */
	cmdno = cnt = r = 0;

	while (commands[cnt])
	{
		if (strcmp(args[0], commands[cnt]) == 0)
		{
			cmdno = cnt + 1;
			break;
		}
		cnt++;
	}

	/* If xcommands == 1 internal code is used for the commands
	 * below, or external programs are executed otherwise
	 */
	if (!xcommands)
	{
		switch (cmdno)
		{
			case	SHCMD_LS:
			case	SHCMD_CP:
			case	SHCMD_MV:
			case	SHCMD_RM:
			case	SHCMD_CHMOD:
			case	SHCMD_LN:
			case	SHCMD_ENV:
			case	SHCMD_CHOWN:
			case	SHCMD_CHGRP:
			{
				cmdno = 0;
				break;
			}
		}
	}

	switch (cmdno)
	{
		case	SHCMD_EXIT:
		{
			boot_print("Are you sure to exit and reboot (y/n)?");
			y = (short)Cconin();
			if (tolower(y & 0xff) == *MSG_init_menu_yes)
				Pterm(0);
			boot_print("\r\n");
			break;
		}
		case	SHCMD_VER:
		{
			ver();
			break;
		}
		case	SHCMD_LS:
		{
			r = sh_ls(args[1]);
			break;
		}
		case	SHCMD_CD:
		{
			if (args[1])
				r = Dsetpath(args[1]);
			else
			{
				home = sh_getenv("HOME=");
				if (home)
					r = Dsetpath(home);
			}
			break;
		}
		case	SHCMD_CHMOD:
		{
			if (args[1] && args[2])
				r = sh_chmod(args[1], args[2]);
			else
				boot_print("chmod OCTAL-MODE FILE\r\n");
			break;
		}
		case	SHCMD_CHOWN:
		{
			if (args[1] && args[2])
				r = sh_chown(args[1], args[2]);
			else
				boot_print("chown DEC-OWNER[:DEC-GROUP] FILE\r\n");
			break;
		}
		case	SHCMD_CHGRP:
		{
			if (args[1] && args[2])
				r = sh_chgrp(args[1], args[2]);
			else
				boot_print("chgrp DEC-GROUP FILE\r\n");
			break;
		}
		case	SHCMD_HELP:
		{
			help();
			break;
		}
		case	SHCMD_ENV:
		{
			env(0);
			break;
		}
		case	SHCMD_XCMD:
		{
			if (args[1])
			{
				if (strcmp(args[1], "on") == 0)
					xcommands = 1;
				else if (strcmp(args[1], "off") == 0)
					xcommands = 0;
			}
			xcmdstate();
			break;
		}
		default:
		{
			r = execvp(newcmd, args);
			break;
		}
	}

	return r;
}

/* XXX because of Cconrs() the command line cannot be longer than 126 bytes.
 */
static void
shell(void)
{
	char *args[SHELL_ARGS];
	char cwd[1024], linebuf[LINELEN+2], *lb, *home;
	long gemdos_time, r;
	short x;

	/* XXX enable word wrap for the console, cursor etc. */
	boot_print("\r\n");
	ver();
	xcmdstate();
	boot_print("Type `help' for help\r\n\r\n");

	home = sh_getenv("HOME=");
	if (home)
	{
		r = Dsetpath(home);
		if (r < 0)
			boot_printf("mint: %s: can't set home directory, error %ld\r\n\r\n", __FUNCTION__, r);
	}

	for (;;)
	{
		bzero(args, sizeof(args));
		bzero(linebuf, sizeof(linebuf));

		linebuf[0] = (sizeof(linebuf) - 2);
		linebuf[1] = 0;

		r = Dgetcwd(cwd, 0, sizeof(cwd));

		if (*cwd == 0)
			strcpy(cwd, "/");
		else
		{
			for (x = 0;x < sizeof(cwd);x++)
			{
				if (cwd[x] == '\\')
					cwd[x] = '/';
			}
		}

		sh_setenv("PWD=", cwd);

		boot_print("mint:");		/* limits in the boot_printf(), as usual */
		boot_print(cwd);
		boot_print("#");
		Cconrs(linebuf);

		ctv.tv_sec = 0;
		(void)Tgettimeofday(&ctv, 0);

		if (ctv.tv_sec)
		{
			gemdos_time = unix2xbios(ctv.tv_sec);
			current_year = (short)(gemdos_time >> 25);
			current_year += 1980;
		}

		if (linebuf[1] > 1)
		{
			short idx;

			lb = linebuf + 2;

			/* "the string is not guaranteed to be NULL terminated"
			 * (Atari GEMDOS reference manual)
			 */
			idx = linebuf[1];
			idx--;
			lb[idx] = 0;

			r = execute(lb, args);

			if (r < 0)
				boot_printf("mint: %s: error %ld\r\n", lb, r);
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

	(void)Pdomain(1);
	(void)Pumask(0x022);

	shell();			/* this one should never return */
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

	Mfree(shell_base);

	return r;
}

/* EOF */
