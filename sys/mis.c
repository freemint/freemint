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

# define SH_VER_MAIOR	0
# define SH_VER_MINOR	1

# define COPYCOPY	"MiS, the FreeMiNT internal shell.\r\n" \
			"Ver. %d.%d, (c) 2003 by Konrad M.Kokoszkiewicz (draco@atari.org)\r\n" \
			"Type help for help.\r\n"

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

static BASEPAGE *shell_base;

static const char *commands[] =
{
	"exit", "ver", "ls", "cd", "cp", "mv", "rm", "chmod", \
	"help", "ln", "exec", "env", "chown", 0
};

static void
ver(void)
{
	boot_printf(COPYCOPY, SH_VER_MAIOR, SH_VER_MINOR);
}

static void
help(void)
{
	boot_print( \
	"	MiS is not intended to be a regular system shell, so don't\r\n" \
	"	expect much. It is only a tool to fix bigger problems that\r\n" \
	"	prevent the system from booting normally.\r\n" \
	"\r\n" \
	"	Recognized commands are:\r\n\r\n" \
	"	cd - change directory\r\n" \
	"	chmod - change file permissions\r\n" \
	"	chown - change file ownership\r\n" \
	"	cp - copy file\r\n" \
	"	env - display environment\r\n" \
	"	exec - (optional) execute a program\r\n" \
	"	exit - leave and reboot\r\n" \
	"	help - display this message\r\n" \
	"	ln - create a link\r\n" \
	"	ls - display directory\r\n" \
	"	mv - move/rename a file\r\n" \
	"	rm - delete a file\r\n" \
	"	ver - display version information\r\n" \
	"\r\n" \
	"	All other words typed are understood as names of programs\r\n" \
	"	to execute. In case you'd want to execute something, that\r\n" \
	"	is named with one of the words displayed above, use 'exec'\r\n" \
	"	or supply the full pathname." \
	"\r\n");
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

static void
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

static void
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
		boot_printf("%s\r\n", var);
		while (*var)
			var++;
		var++;
	}
}

/* Split command line into separate strings and put their pointers
 * into args[].
 *
 * XXX add 'quoted arguments' handling.
 * XXX add wildcard expansion (at least the `*')
 *
 */
static void
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

/* Return a zero if the string given is not an internal command,
 * or the number of the internal command other wise (the number is
 * just their index in the commands[] table, plus one).
 */
static long
identify_command(char *cmd)
{
	long cnt = 0;

	while (commands[cnt])
	{
		if (strcmp(cmd, commands[cnt]) == 0)
			return (cnt + 1);
		cnt++;
	}

	return 0;
}

/* Execute an external program */
static long
execv(char *oldcmd, char *args[])
{
	char *oc, *var, *new_env, *new_var, *t, *path, *npath = 0, *np;
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

	/* The name of the program still begins the command line,
	 * skip it now.
	 */
	oc = oldcmd;
	while(*oc && *oc == 0x20)
		oc++;
	while(*oc && *oc != 0x20)
		oc++;
	*oc = 0x7f;

	/* $PATH searching. Don't do it, if the user seems to
	 * have specified the explicit pathname.
	 */
	t = strrchr(args[0], '/');
	if (!t)
	{
		path = sh_getenv("PATH=");

		if (path)
		{
			npath = (char *)Mxalloc(2048L, 3);

			if (npath)
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

					r = Pexec(0, npath, oc, new_env);

				} while (*path && r < 0);

				Mfree(npath);
			}
		}
	}
	else
		r = Pexec(0, args[0], oc, new_env);

	Mfree(new_env);

	return r;
}

static long
execute(char *cmdline, char *args[])
{
	char newcmd[128], *c, *home;
	long r;
	short y;

	c = cmdline;

	while (*c)
	{
		if (*c == '\r')
			*c = 0x20;
		if (*c == '\n')
			*c = 0x20;
		c++;
	}

	strncpy(newcmd, cmdline, 127);		/* crunch() destroys the `cmdline', so we need to copy it */

	crunch(cmdline, args);

	if (!args[0])
		return 0;			/* empty command line */

	r = identify_command(args[0]);

	switch (r)
	{
		case	SHCMD_EXIT:
		{
			boot_print("Are you sure to exit and reboot (y/n)?");
			y = (short)Cconin();
			if (tolower (y & 0xff) == *MSG_init_menu_yes)
				Pterm(0);
			r = 0;
			boot_print("\r\n");
			break;
		}
		case	SHCMD_VER:
		{
			ver();
			r = 0;
			break;
		}
		case	SHCMD_CD:
		{
			r = 0;
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
		case	SHCMD_HELP:
		{
			help();
			r = 0;
			break;
		}
		case	SHCMD_ENV:
		{
			env(0);
			r = 0;
			break;
		}
		default:
		{
			r = execv(newcmd, args);
			break;
		}
	}

	return r;
}

/* BUG: because of Cconrs() the command line cannot be longer than 126 bytes.
 */
static void
shell(void)
{
	char **args, cwd[128], linebuf[LINELEN+2], *lb, *home;
	long r;
	short x;

	boot_print("\r\n");
	ver();
	boot_print("\r\n");

	home = sh_getenv("HOME=");
	if (home)
	{
		r = Dsetpath(home);
		if (r < 0)
			boot_printf("mint: %s: can't set home directory, error %ld\r\n\r\n", __FUNCTION__, r);
	}

	args = (char **)Mxalloc(8192L, 3);	/* XXX */

	for (;;)
	{
		bzero(args, 8192L);

		linebuf[0] = (sizeof(linebuf) - 2);
		linebuf[1] = 0;

		r = Dgetcwd(cwd, 0, sizeof(cwd));

		strcat(cwd, "/");

		for (x = 0;x < sizeof(cwd);x++)
		{
			if (cwd[x] == '\\')
				cwd[x] = '/';
		}

		sh_setenv("PWD=", cwd);

		boot_printf("mint:%s#", cwd);
		Cconrs(linebuf);

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
	 * (see arch/syscall.S), this is why it is 7936.
	 */
	setstack(bp + 7936L);

	shell_base->p_hitpa = _base->p_hitpa;
	shell_base->p_tbase = _base->p_tbase;
	shell_base->p_tlen = _base->p_tlen;
	shell_base->p_dbase = _base->p_dbase;
	shell_base->p_dlen = _base->p_dlen;
	shell_base->p_bbase = _base->p_bbase;
	shell_base->p_blen = _base->p_blen;
	shell_base->p_parent = _base;

	Mshrink((void *)bp, 8192L);

	(void)Pdomain(1);
	(void)Pumask(0x022);

	shell();		/* this one should never return */
}

/* This below is running in kernel context, called from init.c
 */
long
startup_shell(void)
{
	shell_base = (BASEPAGE *)sys_pexec(7, (char *)SHELL_FLAGS, "\0", init_env);

	/* Start address */
	shell_base->p_tbase = (long)shell_start;

	return sys_pexec(106, (char *)"shell", shell_base, 0L);
}

/* EOF */
