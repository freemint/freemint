/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for MiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "xa_types.h"
#include "xa_global.h" 

#include "xa_appl.h"
#include "xa_shel.h"

#include "accstart.h"
#include "init.h"
#include "taskman.h"
#include "util.h"
#include "xalloc.h"

#include "mint/signal.h"
#include "mint/stat.h"


static char *get_env(enum locks lock, const char *name);
static int copy_env(char *to, char *s[], const char *without, char **last);
static long count_env(char *s[], const char *without);

static const char ARGV[] = "ARGV=";
static const int argvl = 5;

#if 0
static char *appex[16] = { ".prg", ".app", ".gtp", ".ovl", ".sys", 0 };
#endif
static char *tosex[ 8] = { ".tos", ".ttp", 0 };
static char *accex[ 8] = { ".acc", 0 };

static bool
is_ext(char *s, char **app)
{
	while (*app)
	{
		if (stricmp(s, *app) == 0)
			return true;
		app++;
	}
	return false;
}

/* Parse comand tail, while removing separators. */
static long
parse_tail(char *to, char *ti)
{
	char sep;
	char *t = ti;

	while (*t)
	{
		if (*t == '\'' || *t == '\"')	/* arg between quote or apostrophe? */
		{
			if (t == ti)		/* if first arg advance start */
				ti++;
			sep = *t++;
			*t = '\0';
		}
		else
			sep = ' ';
	
		while(*t && *t != sep)
			*to++ = *t++;		/* transfer (in place) between seperator */

		*to++ = 0;			/* terminate 1 arg */
		if (*t)
		{
			/* skip seperator */
			t++;
			if (*t && sep != ' ' && *t == ' ')
				t++;
		}
	}

	/* double 0 terminate */
	*to++ = 0;

	/* return space needed (end - start) */
	return to - ti;
}

static void
make_argv(char *p_tail, long tailsize, char *command, char *argvtail)
{
	int i = strlen(command);
	long l;
	char *argtail;
	
	DIAGS(("make_argv: %lx, %ld, %lx, %lx", p_tail, tailsize, command, argvtail));
	
	l = count_env(C.strings, 0);
	DIAG((D_shel, NULL, "count_env: %ld", l));
	argtail = xmalloc(l + 1 + tailsize + 1 + i + 1 + argvl + 1, 2012);
	if (argtail)
	{
		int j; char *last;
		j = copy_env(argtail, C.strings, 0, &last);
		DIAG((D_shel, NULL, "copy_env: %d, last-start: %ld", j, last - C.strings[0]));
		strcpy(last, ARGV);
		strcpy(last + argvl + 1, command);
		parse_tail(last + argvl + 1 + i + 1, p_tail + 1);
		C.strings[j++] = last;
		C.strings[j] = 0;
#if XXX
		// XXX raise with initialization in bootup
		// free(C.env);
#endif
		C.env = argtail;
		argvtail[0] = 0x7f;
		DIAGS(("ARGV constructed"));
		IFDIAG (display_env(C.strings, 0);)
	}
	else
		DIAGS(("ARGV: out of memory"));
}

#if 0
static int
xa_fork_exec(short x_mode, const struct xshelw *xsh, const char *fname, char *tail)
{
	struct xshelw x_shell = *xsh;
	Path defdir, shelldir;
	int defdrive = -1;
	int child;
	long oldmask;

#if GENERATE_DIAGS
	{
		char ppp[128];

		*ppp = 0;

		if (x_mode & SW_PSETLIMIT)
			/* Limit child's memory? */
			strcat(ppp,"SW_PSETLIMIT,");
		if (x_mode & SW_UID)
			/* XaAES extension - set user id */
			strcat(ppp, "SW_UID,");
		if (x_mode & SW_GID)
			/* XaAES extension - set group id */
			strcat(ppp, "SW_GID,");
		if (x_mode & SW_PRENICE)
			/* Was the child spawned as 'nice'? */
			strcat(ppp, "SW_PRENICE,");
		if (x_mode & SW_ENVIRON)
			strcat(ppp, "SW_ENVIRON");
		if (x_mode & SW_DEFDIR)
			/* Was the a default dir passed? */
			strcat(ppp, "SW_DEFDIR");

		DIAG((D_shel, NULL, "Extended shell_write bits: %s", ppp));

		if (x_mode & SW_DEFDIR)
			/* Was the a default dir passed? */
			DIAG((D_shel, NULL, "def dir: '%s'", x_shell.defdir));
	}

	if ((x_mode & SW_ENVIRON))
		display_env(&x_shell.env, 1);
#endif

	/* Make it local (for memory protection) */
	if (x_mode & SW_DEFDIR)
		strcpy(shelldir, x_shell.defdir);
	else
		*shelldir = 0;


	DIAG((D_shel, NULL, "Pexec(200) '%s', tail: %ld(%lx) %d '%s'",
		fname, tail, tail, *tail, tail + 1));
	DIAGS(("Normal Pexec"));
#if GENERATE_DIAGS
	display_env(C.strings, 1);
#endif

	/* block SIGCHLD until we setup our data structures */
	oldmask = p_sigblock(1UL << SIGCHLD);

	/* Fork off a new process */
	child = p_vfork();
	if (child == 0)
	{
		/* In child here */
		long rep;

		/* restore old sigmask */
		p_sigsetmask(oldmask);

#if GENERATE_DIAGS
		{
			Path havedir;
			int drv;

			DIAG((D_shel, NULL, "*****   in Fork for '%s' *****", fname));
			drv = d_getdrv();
			d_getpath(havedir, 0);
			DIAG((D_shel, NULL, "havedir(0) '%s'", havedir));
			d_getpath(havedir, drv + 1);
			DIAG((D_shel, NULL, "havedir(%d) '%s'", drv, havedir));
			DIAG((D_shel, NULL, "Parent = %d", p_getppid()));
		}
#endif

		if (x_mode)
		{
			/* Limit child's memory? */
			if (x_mode & SW_PSETLIMIT)
				p_setlimit(2, x_shell.psetlimit);

			/* XaAES extension - set user id */
			if (x_mode & SW_UID)
				p_setuid(x_shell.uid);

			/* XaAES extension - set group id */
			if (x_mode & SW_GID)
				p_setgid(x_shell.gid);

			/* Was the child spawned as 'nice'? */
			if ((x_mode & SW_PRENICE) && x_shell.prenice)
				p_nice(x_shell.prenice);

			if ((x_mode & SW_DEFDIR) && *shelldir)
			{
				/* no name, set */
				defdrive = drive_and_path(shelldir, defdir, NULL, false, true);

				DIAG((D_shel, NULL, "x_mode drive: %d", defdrive));
				DIAG((D_shel, NULL, "       path: '%s'", defdir));
			}
		}

		/* It took me a few hours to realize that C.strings must be dereferenced,
		 * to get to the original type of string (which I call a 'superstring'
		 */

		/* exec */
		rep = p_exec(200, fname, tail,
				(x_mode & SW_ENVIRON) ? x_shell.env : *C.strings);

		DIAG((D_shel, NULL, "Pexec replied %ld(%lx)", rep, rep));

		/* If we Pterm, we've failed to execute */
		p_term(rep);
	}

	/* restore old sigmask */
	p_sigsetmask(oldmask);

	return child;
}
#endif

int
launch(enum locks lock, short mode, short wisgr, short wiscr, const char *parm, char *p_tail, struct xa_client *caller)
{
	char cmd[260];		/* 2 full paths */
	char argvtail[4];
	struct xshelw x_shell;
	short x_mode, real_mode;
	const char *pcmd;
	char *save_tail = NULL, *ext = NULL;
	long longtail = 0, tailsize = 0;
	Path save_cmd;
	char *tail = argvtail;
	int ret;
	int drv = 0;
	Path path,name;
	struct proc *p = NULL;
	int type;

	DIAG((D_shel, caller, "launch for %s: 0x%x,%d,%d,%lx,%lx",
		c_owner(caller), mode, wisgr, wiscr, parm, p_tail));

	if (!parm)
		return -1;

	x_shell = *(const struct xshelw *) parm;

	real_mode = mode & 0xff;
	x_mode = mode & 0xff00;

	if (x_mode)
	{
		/* Do some checks before allocating anything. */
		if (!x_shell.newcmd)
			return -1;
	}

	if (x_mode)
		pcmd = x_shell.newcmd;
	else
		pcmd = parm;

	argvtail[0] = 0;
	argvtail[1] = 0;

	if (p_tail)
	{
		if (p_tail[0] == 0x7f || (unsigned char)p_tail[0] == 0xff)
		{
			/* In this case the string CAN ONLY BE null terminated. */
			longtail = strlen(p_tail + 1);
			DIAG((D_shel, NULL, "ARGV!  longtail = %ld", longtail));
			if (longtail < 126)
			{
				p_tail[0] = longtail;
				longtail = 0;
			}
		}
	
		if (longtail)
		{
			tailsize = longtail;
			tail = xmalloc(tailsize + 2, 1012);
			if (!tail)
				return 0;
			strcpy(tail + 1, p_tail + 1);
			*tail = 0x7f;
			DIAG((D_shel, NULL, "long tailsize: %ld", tailsize));
		}
		else
		{
			tailsize = p_tail[0];
			tail = xmalloc(tailsize + 2, 12);
			if (!tail)
				return 0;
			strncpy(tail, p_tail, tailsize + 1);
			tail[tailsize + 1] = '\0';
			DIAG((D_shel, NULL, "int tailsize: %ld", tailsize));
		}
	}

	DIAG((D_shel, NULL, "Launch(0x%x): wisgr:%d, wiscr:%d\r\n cmd='%s'\r\n tail=%d'%s'",
		mode, wisgr, wiscr, pcmd, *tail, tail+1));

#if GENERATE_DIAGS
	if (wiscr == 1)
	{
		DIAGS(("wiscr == 1"));
		display_env(C.strings, 0);
	}
#endif

	/* Keep a copy of original for in client structure */
	strcpy(save_cmd, pcmd);

	/* HR: This was a very bad bug, it took me a day to find it, although it was
	 *     right before my eyes all the time.
	 *     Dont mix pascal string processing with C string processing! :-)
	 */
	save_tail = xmalloc(tailsize + 2, 16);	/* was: strlen(tail+1) */
	strncpy(save_tail, tail, tailsize + 1);
	save_tail[tailsize + 1] = '\0';

	if (get_drv(pcmd) >= 0)
	{
		strcpy(cmd, pcmd);
		DIAG((D_shel, 0, "cmd complete: '%s'", cmd));
	}
	else
	{
		/* no drive, no path, use the caller's */

		DIAG((D_shel, 0, "make cmd: '%s' + '%s'", caller->home_path, pcmd));

		strcpy(cmd, caller->home_path);
		if (*pcmd != '\\' && *(caller->home_path + strlen(caller->home_path) - 1) != '\\')
			strcat(cmd, "\\");
		strcat(cmd, pcmd);	

		DIAG((D_shel, 0, "cmd appended: '%s'", cmd));
	}

	ext = cmd + strlen(cmd) - 4; /* Hmmmmm, OK, tos ttp and acc are really specific and always 3 */

	if (real_mode == 0)
	{
		if (is_ext(ext, tosex))
		{
			real_mode = 1, wisgr = 0;
			DIAG((D_shel, 0, " -= 1,0 =-"));
		}
		else if (is_ext(ext, accex))
		{
			real_mode = 3;
			DIAG((D_shel, 0, " -= 3,%d =-", wisgr));
		}
		else
		{
			real_mode = 1, wisgr = 1;
			DIAG((D_shel, 0, " -= 1,1 =-"));
		}
	}

	switch (real_mode)
	{
		case 1:
		{
			/* TOS Launch?  */
			if (wisgr == 0)
			{
				char *tosrun;

				tosrun = get_env(lock, "TOSRUN=");
				if (!tosrun)
				{
					/* check tosrun pipe */
					struct file *f;

					f = kernel_open("u:\\pipe\\tosrun", O_WRONLY, NULL);
					if (f)
					{		
						kernel_write(f, cmd, strlen(cmd));
						kernel_write(f, " ", 1);
						kernel_write(f, tail + 1, *tail);
						kernel_close(f);
					}
					else
					{
						DIAGS(("Tosrun pipe not opened!!  nn: '%s', nt: %d'%s'",
							cmd, tail[0], tail+1));
						DIAGS(("Raw run."));
						wisgr = 1;
					}
				}
				else
				{
					char *new_tail = xmalloc(tailsize + 1 + strlen(cmd) + 1, 17);
					long new_tailsize;

					if (!new_tail)
					{
						ret = ENOMEM;
						goto out;
					}

					/* command --> tail */
					strcpy(new_tail + 1, cmd);
					strcat(new_tail + 1, " ");
					DIAGS(("TOSRUN: '%s'", tosrun));
					wisgr = 1;
					strcpy(cmd,tosrun);
					new_tailsize = strlen(new_tail + 1);
					strncpy(new_tail + new_tailsize + 1, tail + 1, tailsize);
					new_tailsize += tailsize;
					free(tail);
					tail = new_tail;
					tail[new_tailsize + 1] = 0;
					tailsize = new_tailsize;
					if (tailsize > 126)
					{
						longtail = tailsize;
						tail[0] = 0x7f;
						DIAGS(("long tosrun nn: '%s', tailsize: %ld", cmd, tailsize));
					}
					else
					{
						longtail = 0;
						tail[0] = tailsize;
						DIAGS(("tosrun nn: '%s', nt: %ld'%s'", cmd, tailsize, tail+1));
					}
				}
			}

			drv = drive_and_path(cmd, path, name, true, true);

			DIAG((D_shel, 0, "[2]drive_and_path %d,'%s','%s'", drv, path,name));

			if (tailsize && (wiscr == 1 || longtail))
				make_argv(tail, tailsize, name, argvtail);

			if (wisgr != 0)
			{
				ret = create_process(cmd, *argvtail ? argvtail : tail,
						     (x_mode & SW_ENVIRON) ? x_shell.env : *C.strings,
						     &p, 0);
				if (ret == 0)
				{
					assert(p);

					if (((x_mode & SW_PRENICE) == 0)
					    || ((x_mode & SW_PRENICE) != 0 && x_shell.prenice == 0))
						p_renice(p->pid, -4);

					DIAG((D_appl, 0, "Alloc client; APP %d", p->pid));

					type = APP_APPLICATION;
					ret = p->pid;
				}
			}

			break;
		}
		case 3:
		{
			struct basepage *b;
			long size, p_rtn;

			drv = drive_and_path(save_cmd, path, name, true, true);

			DIAG((D_shel, 0, "[3]drive_and_path %d,'%s','%s'", drv, path, name));

			p_rtn = create_process(cmd, *argvtail ? argvtail : tail, *C.strings, &p, 128);
			if (p_rtn < 0)
			{
				DIAG((D_shel, 0, "acc launch failed:error=%ld", p_rtn));
				ret = p_rtn;
				break;
			}

			b = p->p_mem->base;

			/* accsize */
			size = 256 + b->p_tlen + b->p_dlen + b->p_blen;

			DIAGS(("Copy accstart to 0x%lx, size %lu", (char *)b + size, (long)accend - (long)accstart));
			memcpy((char *)b + size, accstart, (long)accend - (long)accstart);

			b->p_dbase = b->p_tbase;
			b->p_tbase = (char *)b + size;

			/* set PC appropriately */
			p->ctxt[CURRENT].pc = b->p_tbase;

			p_renice(p->pid, -4);

			type = APP_ACCESSORY;
			ret = p->pid;

			break;
		}
	}

	if (p)
	{
		struct shel_info *info;

		info = attach_extension(p, XAAES_MAGIC_SH, sizeof(*info), NULL);
		if (info)
		{
			info->type = type;

			info->cmd_tail = save_tail;
			info->tail_is_heap = true;

			strcpy(info->cmd_name, save_cmd);

			*(info->home_path) = drv + 'a';
			*(info->home_path + 1) = ':';
			strcpy(info->home_path+2, path);
		}
		else
			ikill(SIGKILL, p->pid);
	}

out:
	if (tail != argvtail)
		free(tail);

	DIAG((D_shel, 0, "Launch for %s returns child %d (bp 0x%lx)",
		c_owner(caller), ret, p ? p->p_mem->base : NULL));
	DIAG((D_shel, 0, "Remove ARGV"));

	/* Remove ARGV */
	put_env(lock, 1, 0, ARGV);

	/* and go out */
	return ret;
}

unsigned long
XA_shell_write(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short wdoex = pb->intin[0];
	short wisgr = pb->intin[1];
	short wiscr = pb->intin[2];

	CONTROL(3,1,2)

	DIAG((D_shel, NULL, "shel_write(0x%d,%d,%d) for %s",
		wdoex, wisgr, wiscr, client->name));

	if ((wdoex & 0xff) < 4)
	{
		Sema_Up(envstr);

		pb->intout[0] = launch(lock|envstr,
				       pb->intin[0],
				       wisgr,
				       wiscr,
				       (char *)pb->addrin[0],
				       (char *)pb->addrin[1],
				       client);

		// XXX wait until appl_init from new child???

		Sema_Dn(envstr);
	}
	else
	{
		char *cmd = (char*)pb->addrin[0];

		DIAG((D_shel, NULL, " -- 0x%x, wisgr %d, wiscr %d",
			wdoex, wisgr, wiscr));

		pb->intout[0] = 0;

		switch (wdoex)
		{
			/* shutdown system */
			case 4:
			{
				DIAGS(("shutown by shel_write(4, %d,%d)", wiscr, wisgr));
				shutdown(lock);
				break;
			}

			/* resolution change */
			case 5:
				break;

			/* undefined */
			case 6:
				break;

			/* broadcast message */
			case 7:	
			{
				struct xa_client *cl;

				Sema_Up(clients);

				cl = S.client_list;
				while (cl)
				{
					if (is_client(cl) && cl != client)
						send_a_message(lock, cl, (union msg_buf *)cmd);

					cl = cl->next;
				}

				Sema_Dn(clients);
				break;
			}

			/* Manipulate AES environment */
			case 8:
			{
				pb->intout[0] = put_env(lock, wisgr, wiscr, cmd);
				break;
			}

			/* currently: tell AES app understands AP_TERM */
			case 9:
			{
				client->apterm = (wisgr & 1) != 0;
				break;
			}

			/* Send a msg to the AES  */
			case 10:

			/* create new thread */
			case 20:

			/* thread exit */
			case 21:

			/* terminate thread */
			case 22:

			default:
				break;
		}
	}

	return XAC_DONE;
}


/* I think that the description if shell_read in the compendium is 
 * rubbish. At least on Atari GEM the processes own command must be
 * given, NOT that of the parent.
 */
unsigned long
XA_shell_read(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char *name = (char *)pb->addrin[0];
	char *tail = (char *)pb->addrin[1];
	int f;

	CONTROL(0,1,2)

	strcpy(name, *client->cmd_name ? client->cmd_name : "\\");
	
	for (f = 0; f < client->cmd_tail[0] + 1; f++)
		tail[f] = client->cmd_tail[f];

	tail[f] = 0;
	
	pb->intout[0] = 1;

	DIAG((D_shel, client, "shel_read: n='%s', t=%d'%s'",
		pb->addrin[0], *(char *)pb->addrin[1], (char *)pb->addrin[1]+1));

	return XAC_DONE;
}

char *
shell_find(enum locks lock, struct xa_client *client, char *fn)
{
	char cwd[256];
	static Path path; /* XXX */

	struct stat st;
	long r;

	char *kp, *kh;
	int f = 0, l, n;


	kp = get_env(lock, "PATH=");
	kh = get_env(lock, "HOME=");

	DIAGS(("shell_find for %s '%s', PATH= '%s'", client->name, fn ? fn : "~", kp ? kp : "~"));

	if (!(isalpha(*fn) && *(fn+1) == ':'))
	{
		/* Check the clients home path */
		sprintf(path, sizeof(path), "%s\\%s", client->home_path, fn);
		r = f_stat64(0, path, &st);
		DIAGS(("[1]  --   try: '%s' :: %ld", path, r));
		if (r == 0 && S_ISREG(st.mode))
			return path;

		/* check $HOME directory */
		if (cfg.usehome && kh)
		{
			sprintf(path, sizeof(path), "%s\\%s", kh, fn);
			r = f_stat64(0, path, &st);
			DIAGS(("[2]  --   try: '%s' :: %ld", path, r));
			if (r == 0 && S_ISREG(st.mode))
				return path;
		}

		/* the PATH env could be simply absent */
		if (kp)	
		{
			/* Check our PATH environment variable */
			/* l = strlen(cwd); */
			/* cwd uninitialized; after sprintf? or is it kp? or is it sizeof? */
			l = strlen(kp);
			strcpy(cwd, kp);
			while (f < l)
			{
				/* We understand ';' and ',' as path seperators */
				n = f;
				while (   cwd[n]
				       && cwd[n] != ';'
				       && cwd[n] != ',')
				{
					if (cwd[n] == '/')
						cwd[n] = '\\';
					n++;
				}
				if (cwd[n-1] == '\\')
					cwd[n-1] = 0;
				cwd[n] = '\0';

				sprintf(path, sizeof(path), "%s\\%s", cwd + f, fn);
				r = f_stat64(0, path, &st);
				DIAGS(("[3]  --   try: '%s' :: %ld", path, r));
				if (r == 0 && S_ISREG(st.mode))
					return path;

				f = n + 1;
			}
		}

		/* Try clients current path: */
		sprintf(path, sizeof(path), "%c:%s\\%s",
			client->xdrive + 'a', client->xpath, fn);
		r = f_stat64(0, path, &st);
		DIAGS(("[4]  --   try: '%s' :: %ld", path, r));
		if (r == 0 && S_ISREG(st.mode))
			return path;
	}

	/* Last ditch - try the file spec on its own */
	r = f_stat64(0, path, &st);
	DIAGS(("[5]  --    try: '%s' :: %ld", fn, r));
	if (r == 0 && S_ISREG(st.mode))
		return fn;

	DIAGS((" - 0"));
	return 0;
}

unsigned long
XA_shell_find(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char *fn = (char *)(pb->addrin[0]);
	char *path;

	CONTROL(0,1,1)

	path = shell_find(lock, client, fn);
	if (path)
	{
		strcpy(fn, path);
		pb->intout[0] = 1;
	}
	else
		/* Didn't find the file */
		pb->intout[0] = 0;

	return XAC_DONE;
}

/* This version finds XX & XX= */
static char *
get_env(enum locks lock, const char *name)
{
	int i = 0;

	Sema_Up(envstr);

	while (C.strings[i])
	{
		char *f = C.strings[i];
		const char *n = name;
		do {
			if (*f != *n)
			{
				if (*f == '=' && *n == 0)
				{
					Sema_Dn(envstr);
					/* if used XX return pointer to '=' */
					return f;
				}
				else
					break;
			}

			if (*f == '=' && *n == '=')
			{
				Sema_Dn(envstr);
				/* if used XX= return pointer after '=' */
				return f+1;
			}

			if (*f == '=')
				break;

			f++;
			n++;
		}
		while (1);

		i++;
	}

	Sema_Dn(envstr);
	return 0;
}

static int
copy_env(char *to, char *s[], const char *without, char **last)
{
	int i = 0, j = 0, l = 0;

	if (without)
		while (without[l] != '=')
			l++;

	while (s[i])
	{
		if (strncmp(s[i], ARGV, 4) == 0)
		{
			DIAGS(("copy_env ARGV: skipped remainder of environment"));
			break;
		}

		if (l == 0 || (l != 0 && strncmp(s[i], without, l) != 0) )
		{
			strcpy(to, s[i]);
			s[j++] = to;
			to += strlen(to) + 1;
		}
		i++;
	}

	C.strings[j] = '\0';

	*to = '\0';		/* the last extra \0 */
	if (last)
		*last = to;

	return j;
}

static long
count_env(char *s[], const char *without)
{
	long ct = 0;
	int i = 0, l = 0;

	if (without)
		while (without[l] != '=')
			l++;

	while (s[i])
	{
		if (strncmp(s[i], ARGV, 4) == 0)
		{
			DIAGS(("count_env ARGV: skipped remainder of environment"));
			break;
		}
		if (l == 0 || (l != 0 && strncmp(s[i], without, l) != 0) )
			ct += strlen(s[i]) + 1;
		i++;
	}
	return ct + 1;
}

long
put_env(enum locks lock, short wisgr, short wiscr, char *cmd)
{
	long ret = 0;

	Sema_Up(envstr);

	if (wisgr == 0)
	{
		long ct = count_env(C.strings, 0);
		DIAG((D_shel, 0, " -- count = %d", ct));
		ret = ct;
	}
	else if (wisgr == 1)
	{
		if (cmd)
		{
			long ct;
			char *newenv;

			DIAG((D_shel, 0, " -- change; '%s'", cmd ? cmd : "~~"));

			/* count without */
			ct = count_env(C.strings, cmd);	
			/* ct is including the extra \0 at the end! */

			/* ends with '=': remove */
			if (*(cmd + strlen(cmd) - 1) == '=')
			{
				newenv = xmalloc(ct, 19);
				if (newenv)
					/* copy without */
					copy_env(newenv, C.strings, cmd, 0);
			}
			else
			{
				int l = strlen(cmd) + 1;
				newenv = xmalloc(ct + l, 20);
				if (newenv)
				{
					char *last;
					/* copy without */
					int j = copy_env(newenv, C.strings, cmd, &last);
					C.strings[j] = last;
					/* add new */
					strcpy(C.strings[j], cmd);
					*(C.strings[j] + l) = 0;
					C.strings[j + 1] = 0;
				}
			}

			if (newenv)
			{
#if XXX
				// XXX raise with initialization in bootup
				// free(C.env);
#endif
				C.env = newenv;
			}

			DIAGS(("putenv"));
			IFDIAG (display_env(C.strings, 0);)
		}
	}
	else if (wisgr == 2)
	{
		long ct = count_env(C.strings, 0);
		DIAG((D_shel, 0, " -- copy"));
		memcpy(cmd, C.strings[0], wiscr > ct ? ct : wiscr);
		ret = ct > wiscr ? ct - wiscr : 0;
		DIAG((D_shel, 0, " -- & left %ld", ret));
	}

	Sema_Dn(envstr);
	return ret;
}

/* HR: because everybody can mess around with the AES's environment
 *     (shel_write(8, ...) changing the pointer array strings,
 *     it is necessary to make a permanent copy of the variable.
 */
unsigned long
XA_shell_envrn(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char **p = (char**)pb->addrin[0];
	char *name = (char *)pb->addrin[1];

	CONTROL(0,1,2)

	if (p && name)
	{
		char *pf = get_env(lock, name);

		*p = NULL;

		DIAGS(("shell_env for %s: '%s' :: '%s'", c_owner(client), name, pf? pf: "~~~"));	
		if (pf)
		{
			*p = proc_malloc(strlen(pf) + 1);
			if (*p)
				strcpy(*p, pf);
		}
	}

	pb->intout[0] = 1;
	return XAC_DONE;
}
