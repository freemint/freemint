/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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

#include <mint/basepage.h>
#include <mint/mintbind.h>
#include <signal.h>

#include "xa_types.h"
#include "xa_global.h" 

#include "accstart.h"
#include "bootup.h"
#include "taskman.h"
#include "util.h"
#include "xalloc.h"
#include "xa_clnt.h"
#include "xa_shel.h"


static char ARGV[] = "ARGV=";
static int argvl = 5;

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

/* HR: 210201 Parse comand tail, while removing separators. */
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
	
	DIAGS(("make_argv: %lx, %ld, %lx, %lx\n", p_tail, tailsize, command, argvtail));
	
	l = count_env(C.strings, 0);
	DIAG((D_shel, NULL, "count_env: %ld\n", l));
	argtail = xmalloc(l + 1 + tailsize + 1 + i + 1 + argvl + 1, 2012);
	if (argtail)
	{
		int j; char *last;
		j = copy_env(argtail, C.strings, 0, &last);
		DIAG((D_shel, NULL, "copy_env: %d, last-start: %ld\n", j, last - C.strings[0]));
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
		DIAGS(("ARGV constructed\n"));
		IFDIAG (display_env(C.strings, 0);)
	}
	else
		DIAGS(("ARGV: out of memory\n"));
}

/* HR new_client --> new;  use NewClient() */
/* HR only called now for real_mode < 4 */
int
launch(LOCK lock, short mode, short wisgr, short wiscr, char *parm, char *p_tail, XA_CLIENT *caller)
{
	XSHELW x_shell;
	short x_mode, real_mode;
	char *pcmd, *save_tail = NULL, *ext = NULL;
	long longtail = 0, tailsize = 0;
	char argvtail[4];
	XA_CLIENT *new = NULL;
	char cmd[260];		/* 2 full paths */
	Path save_cmd;
	char *tail = argvtail;
	int ret;

	long fl;

	DIAG((D_shel, caller, "launch for %s: 0x%x,%d,%d,%lx,%lx\n",
		c_owner(caller), mode, wisgr, wiscr, parm, p_tail));

	if (!parm)
		return 0;

	x_shell = *(XSHELW *) parm;

	real_mode = mode & 0xff;
	x_mode = mode & 0xff00;

	if (x_mode)
	{
		/* Do some checks before allocating anything. */
		if (!x_shell.newcmd)
			return 0;
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
			DIAG((D_shel, NULL, "ARGV!  longtail = %ld\n", longtail));
			if (longtail < 126)
			{
				p_tail[0] = longtail;
				longtail = 0;
			}
		}
	
		if (longtail)
		{
			tailsize = longtail;
			tail = xmalloc(tailsize + 2,1012);
			if (!tail)
				return 0;
			strcpy(tail + 1, p_tail + 1);
			*tail = 0x7f;
			DIAG((D_shel, NULL, "long tailsize: %ld\n", tailsize));
		}
		else
		{
			tailsize = p_tail[0];
			tail = xmalloc(tailsize + 2,12);
			if (!tail)
				return 0;
			strncpy(tail, p_tail, tailsize + 1);
			tail[tailsize + 1] = '\0';
			DIAG((D_shel, NULL, "int tailsize: %ld\n", tailsize));
		}
	}

	DIAG((D_shel, NULL, "Launch(0x%x): wisgr:%d, wiscr:%d\r\n cmd='%s'\r\n tail=%d'%s'\n",
		mode, wisgr, wiscr, pcmd, *tail, tail+1));

#if GENERATE_DIAGS
	if (wiscr == 1)
	{
		DIAGS(("wiscr == 1\n"));
		display_env(C.strings, 0);
	}
#endif

	/* Keep a copy of oroginal for in client structure */
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
		DIAG((D_shel,0,"cmd complete: '%s'\n", cmd));
	}
	else
	{
		/* no drive, no path, use the caller's */

		DIAG((D_shel, 0, "make cmd: '%s' + '%s'\n", caller->home_path, pcmd));

		strcpy(cmd, caller->home_path);
		if (*pcmd != '\\' && *(caller->home_path + strlen(caller->home_path) - 1) != '\\')
			strcat(cmd, "\\");
		strcat(cmd, pcmd);	

		DIAG((D_shel, 0, "cmd appended: '%s'\n", cmd));
	}

	ext = cmd + strlen(cmd) - 4; /* Hmmmmm, OK, tos ttp and acc are really specific and always 3 */

	if (real_mode == 0)
	{
		if (is_ext(ext, tosex))
		{
			real_mode = 1, wisgr = 0;
			DIAG((D_shel, 0, " -= 1,0 =-\n"));
		}
		else
		if (is_ext(ext, accex))
		{
			real_mode = 3;
			DIAG((D_shel, 0, " -= 3,%d =-\n", wisgr));
		}
		else
		{
			real_mode = 1, wisgr = 1;
			DIAG((D_shel, 0, " -= 1,1 =-\n"));
		}
	}

	fl = Fopen(cmd, 0);
	DIAG((D_shel, 0, "Fopen try: '%s' to %ld, real_mode %d\n", cmd, fl, real_mode));
	if (fl > 0)
	{
		int drv = 0;
		Path path,name;

		Fclose(fl);
		DIAG((D_shel, 0, "OK Fclose %ld\n", fl));

		switch (real_mode)
		{
		case 1:
			/* TOS Launch?  */
			if (wisgr == 0)
			{
				char *tosrun = get_env(lock, "TOSRUN=");
				if (tosrun == 0)
				{
					/* HR check tosrun pipe */
					long fd = Fopen("u:\\pipe\\tosrun", 2);
					if (fd > 0)
					{		
						Fwrite(fd, strlen(cmd), cmd);
						Fwrite(fd, 1, " ");
						Fwrite(fd, *tail, tail + 1);
						Fclose(fd);
					}
					else
					{
						DIAGS(("Tosrun pipe not opened!!  nn: '%s', nt: %d'%s'\n",
							cmd, tail[0], tail+1));
						DIAGS(("Raw run.\n"));
						wisgr = 1;

					}
				}
				else
				{
					char *new_tail = xmalloc(tailsize + 1 + strlen(cmd) + 1, 17);
					long new_tailsize;

					/* command --> tail */
					strcpy(new_tail + 1, cmd);
					strcat(new_tail + 1, " ");
					DIAGS(("TOSRUN: '%s'\n", tosrun));
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
						DIAGS(("long tosrun nn: '%s', tailsize: %ld\n", cmd, tailsize));
					}
					else
					{
						longtail = 0;
						tail[0] = tailsize;
						DIAGS(("tosrun nn: '%s', nt: %ld'%s'\n", cmd, tailsize, tail+1));
					}
				}
			}

			/* HR 260202: 'save_cmd' changed to 'cmd' */
			drv = drive_and_path(/* save_ */ cmd, path, name, true, true);

			DIAG((D_shel,0,"[2]drive_and_path %d,'%s','%s'\n",drv,path,name));

			if (tailsize && (wiscr == 1 || longtail))
				make_argv(tail, tailsize, name, argvtail);

			if (wisgr != 0)
			{
				Sema_Up(clients);

				new = xa_fork_exec(x_mode, &x_shell, cmd, *argvtail ? argvtail : tail);
				if (new)
				{
					if (((x_mode & SW_PRENICE) == 0)
					    || ((x_mode & SW_PRENICE) != 0 && x_shell.prenice == 0))
						(void) Prenice(new->pid, -4);

					DIAG((D_appl,0,"Alloc client; APP %d\n",new->pid));
					new->type = APP_APPLICATION;
				}

				Sema_Dn(clients);
/*
 * MASSIVE KLUDGE
 * - for some reason, you MUST re-open the moose after a Pvfork()
 */
#if MOUSE_KLUDGE
				if (C.MOUSE_dev)
					reopen_moose();
#endif
			}

			break;
		case 3:
#if MOUSE_KLUDGE
			if (C.MOUSE_dev)
				/* Kludge to get round a bug in MiNT (or moose?) */
				Fclose(C.MOUSE_dev);
#endif
			{
				long p_rtn;
				long shrink;
				int child = 0;
				int shrinked;
				BASEPAGE* b;
				long oldmask;

				drv = drive_and_path(save_cmd, path, name, true, true);

				DIAG((D_shel, 0, "[3]drive_and_path %d,'%s','%s'\n",drv,path,name));

				p_rtn = Pexec(3, cmd, *argvtail ? argvtail : tail, 0);
				if (p_rtn < 0)
				{
					DIAG((D_shel, 0, "acc launch failed:error=%ld\n", p_rtn));
					break;
				}

				b = (BASEPAGE *)p_rtn;
				shrink = 256 + b->p_tlen + b->p_dlen + b->p_blen;
				shrinked = Mshrink(b, shrink);

				DIAGS(("Shrinked accessory@%lx to %ld: %d\n",b, shrink, shrinked));

				b->p_dbase = b->p_tbase;
				/* This code basically puts the basepage address in a0. */
				b->p_tbase = (char *)accstart;

				Sema_Up(clients);

				/* block SIGCHLD until we setup our data structures */
				oldmask = Psigblock(1UL << SIGCHLD);

				DIAG((D_shel, 0, "Pexec(106) '%s'\n",cmd));
				child = Pexec(106, cmd, b, *C.strings);		/* HR 104 --> 106 */
				if (child < 0)
					; /* XXX failure */
				DIAG((D_shel, 0, "child = %d\n", child));

				(void) Prenice(child, -4);

				new = NewClient(child);
				if (new)
				{
					DIAG((D_appl, NULL, "Alloc client; ACC %d\n", new->pid));
					new->type = APP_ACCESSORY;
				}

				/* restore old sigmask */
				Psigsetmask(oldmask);

				Sema_Dn(clients);
			}

#if MOUSE_KLUDGE
			if (C.MOUSE_dev)
				/* Kludge - for some reason, you MUST re-open the moose after a Pvfork() */
				reopen_moose();
#endif
			break;

		}

		if (new)
		{
			strcpy(new->cmd_name, save_cmd);

			new->cmd_tail = save_tail;
			new->tail_is_heap = true;
			new->parent = Pgetpid();

			/* As we now unambiguously know the path from which the client is loaded,
			 * why not fill it out here? :-)
			 */
			* new->home_path = drv + 'a';
			*(new->home_path + 1) = ':';
			strcpy(new->home_path+2, path);

			update_tasklist(lock);
		}
	}

#if 0
	Dsetdrv(C.home_drv);
	Dsetpath(C.home);
#endif

	free(tail);

	DIAG((D_shel,0,"Launch for %s returns child %d\n", c_owner(caller), new ? new->pid : -1));
	DIAG((D_shel,0,"Remove ARGV\n"));

	/* Remove ARGV */
	put_env(lock, 1, 0, ARGV);

	/*
	 * return pid of new process
	 * 
	 * remember this before we unblock SIGCHLD; if the client died
	 * in the meantime the signal handler run for sure before
	 * Psigsetmask() return :-)
	 */
	ret = new ? new->pid : 0;

	/* and go out */
	return ret;
}

unsigned long
XA_shell_write(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	XA_CLIENT *child;
	short wisgr = pb->intin[1];
	short wiscr = pb->intin[2];

	CONTROL(3,1,2)

	DIAG((D_shel, NULL, "shel_write(0x%x,%d,%d) for %s\n",
		pb->intin[0], wisgr, wiscr, client->name));

	if ((pb->intin[0] & 0xff) < 4)
	{
		int child_id;

		Sema_Up(envstr);

		child_id = launch(lock|envstr,
				  pb->intin[0],
				  wisgr,
				  wiscr,
				  pb->addrin[0],
				  pb->addrin[1],
				  client);

		pb->intout[0] = child_id;

		if (child_id)
		{
			child = Pid2Client(child_id);
			if (child)
			{
				child->parent = client->pid;
#if SUSPEND_PARENT
				if (pb->intin[0] == 1 && wisgr == 1)
				{
					client->waiting_for = XAWAIT_CHILD;

					Sema_Dn(envstr);
					return XAC_BLOCK;
				}
#endif
			}
		}

		Sema_Dn(envstr);
	}
	else
	{
		char *cmd = pb->addrin[0];

		DIAG((D_shel, NULL, " -- 0x%x, wisgr %d, wiscr %d\n",
			pb->intin[0], pb->intin[1], pb->intin[2]));

		pb->intout[0] = 0;

		switch (pb->intin[0])
		{
			/* Shutdown system */
			case 4:
			{
				DIAGS(("shutown by shel_write(4, %d,%d)\n", wiscr, wisgr));
				shutdown(lock);
				break;
			}

			/* Res change */
			case 5:
				break;

			/* undefined */
			case 6:
				break;

			/* broadcast message */
			case 7:	
			{
				XA_CLIENT *cl;

				Sema_Up(clients);
	
				cl = S.client_list;
				while (cl)
				{
					if (is_client(cl) && cl->pid != client->pid)
						send_a_message(lock, cl->pid, (union msg_buf *)cmd);

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
				client->apterm = (wisgr&1) != 0;
				break;
			}

			/* Send a msg to the AES  */
			case 10:
			default:
				break;
		}
	}

	return XAC_DONE;
}


/* I think that the description if shell_read in the compendium is 
     rubbish. At least on Atari GEM the processes own command must be
     given, NOT that of the parent.
*/
unsigned long
XA_shell_read(LOCK lock, XA_CLIENT *client, AESPB *pb)
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

	DIAG((D_shel, client, "shel_read: n='%s', t=%d'%s'\n",
		pb->addrin[0], *(char *)pb->addrin[1], (char *)pb->addrin[1]+1));

	return XAC_DONE;
}

char *
shell_find(LOCK lock, XA_CLIENT *client, char *fn)
{
	char *kp, *kh;
	static
	Path path;
	char cwd[256];
	long handle;
	int f = 0, l, n;

	kp = get_env(lock, "PATH=");
	kh = get_env(lock, "HOME=");

	DIAGS(("shell_find for %s '%s', PATH= '%s'\n", client->name, fn ? fn : "~", kp ? kp : "~"));

	if ( ! (isalpha(*fn) && *(fn+1) == ':'))
	{
		/* Check the clients home path */
		sdisplay(path, "%s\\%s", client->home_path, fn);
		handle = Fopen(path, 0);
		DIAGS(("[1]  --   try: '%s' :: %ld\n",path,handle));
		if (handle > 0)
		{
			Fclose(handle);
			return path;
		}

		/* HR 051002: check $HOME directory */
		if (cfg.usehome && kh)
		{
			sdisplay(path, "%s\\%s", kh, fn);
			handle = Fopen(path, 0);
			DIAGS(("[2]  --   try: '%s' :: %ld\n", path, handle));
			if (handle > 0)
			{
				Fclose(handle);
				return path;
			}
		}

		/* HR: the PATH env could be simply absent */
		if (kp)	
		{
			/* Check our PATH environment variable */
			/* l = strlen(cwd); */
			/* HR: cwd uninitialized; after sdisplay? or is it kp? or is it sizeof? */
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
				sdisplay(path, "%s\\%s", cwd + f, fn);
				handle = Fopen(path, 0);
				DIAGS(("[3]  --   try: '%s' :: %ld\n", path, handle));
				if (handle > 0)
				{
					Fclose(handle);
					return path;
				}
				f = n + 1;
			}
		}

		/* Try clients current path: */
		sdisplay(path, "%c:%s\\%s",
					client->xdrive + 'a', client->xpath, fn);
		handle = Fopen(path, 0);
		DIAGS(("[4]  --   try: '%s' :: %ld\n", path, handle));
		if (handle > 0)
		{
			Fclose(handle);
			return path;
		}

	}

	/* Last ditch - try the file spec on its own */
	handle = Fopen(fn, 0);
	DIAGS(("[5]  --    try: '%s' :: %ld\n",fn,handle));
	if (handle > 0)
	{
		Fclose(handle);
		return fn;
	}

	DIAGS((" - 0\n"));
	return 0;
}

unsigned long
XA_shell_find(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	char *fn = pb->addrin[0],
	     *path;

	CONTROL(0,1,1)

	path = shell_find(lock, client, pb->addrin[0]);
	if (path)
	{
		strcpy(fn, path);
		pb->intout[0] = 1;
	}
	else
		/* Didn't find the file :( */
		pb->intout[0] = 0;

	return XAC_DONE;
}

/* This version finds XX & XX= */
char *
get_env(LOCK lock, const char *name)
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
					/* HR: if used XX return pointer to '=' */
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

/* HR: The importance of usefull functions */
int
copy_env(char *to, char *s[], char *without, char **last)
{
	int i = 0, j = 0, l = 0;

	if (without)
		while (without[l] != '=') l++;

	while (s[i])
	{
		if (strncmp(s[i],ARGV,4) == 0)
		{
			DIAGS(("copy_env ARGV: skipped remainder of environment\n"));
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

	C.strings[j] = 0;

	*to = 0;		/* the last extra \0 */
	if (last)
		*last = to;

	return j;
}

long
count_env(char *s[], char *without)
{
	long ct = 0;
	int i = 0, l = 0;

	if (without)
		while (without[l] != '=') l++;
	while (s[i])
	{
		if (strncmp(s[i],ARGV,4) == 0)
		{
			DIAGS(("count_env ARGV: skipped remainder of environment\n"));
			break;
		}
		if (l == 0 || (l != 0 && strncmp(s[i], without, l) != 0) )
			ct += strlen(s[i]) + 1;
		i++;
	}
	return ct + 1;
}

long
put_env(LOCK lock, short wisgr, short wiscr, char *cmd)
{
	long ret = 0;

	Sema_Up(envstr);

	if (wisgr == 0)
	{
		long ct = count_env(C.strings, 0);
		DIAG((D_shel, 0, " -- count = %d\n", ct));
		ret = ct;
	}
	else if (wisgr == 1)
	{
		if (cmd)
		{
			long ct;
			char *newenv;

			DIAG((D_shel, 0, " -- change; '%s'\n", cmd ? cmd : "~~"));

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

			DIAGS(("putenv\n"));
			IFDIAG (display_env(C.strings, 0);)
		}
	}
	else if (wisgr == 2)
	{
		long ct = count_env(C.strings, 0);
		DIAG((D_shel, 0, " -- copy\n"));
		memcpy(cmd, C.strings[0], wiscr > ct ? ct : wiscr);
		ret = ct > wiscr ? ct - wiscr : 0;
		DIAG((D_shel, 0, " -- & left %ld\n", ret));
	}

	Sema_Dn(envstr);
	return ret;
}


/* HR: because everybody can mess around with the AES's environment
 *     (shel_write(8, ...) changing the pointer array strings,
 *     it is necessary to make a permanent copy of the variable.
 */
unsigned long
XA_shell_envrn(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	char **p = pb->addrin[0], *pf;
	char *name = (char *)pb->addrin[1];

	CONTROL(0,1,2)

	if (p && name)
	{
		*p = 0;
		pf = get_env(lock, name);

		DIAGS(("shell_env for %s: '%s' :: '%s'\n", c_owner(client), name, pf? pf: "~~~"));	
		if (pf)
		{
			*p = XA_alloc(&client->base, strlen(pf) + 1, 0, 0);
			if (*p)
				strcpy(*p, pf);
		}
	}

	pb->intout[0] = 1;

	return XAC_DONE;
}
