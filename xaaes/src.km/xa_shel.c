/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
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
#include "xa_strings.h"

#include "c_window.h"
#include "xa_appl.h"
#include "xa_shel.h"

#include "accstart.h"
#include "init.h"
#include "k_main.h"
#include "sys_proc.h"
#include "taskman.h"
#include "util.h"

#include "mint/signal.h"
#include "mint/stat.h"
#include "k_exec.h"


#define STRINGS 1024 /* number of environment variable allowed in xaaes.cnf */
static char *strings[STRINGS];

const char ** get_raw_env(void) { return (const char **)(const void**)strings; }

#if GENERATE_DIAGS
static void display_env(char **env, int which);
#endif
static int copy_env(char *to, char *s[], const char *without, char **last);
static long count_env(char *s[], const char *without);

static const char ARGV[] = "ARGV=";
static const int argvl = 5;

#if 0
static char *appex[16] = { ".prg", ".app", ".gtp", ".ovl", ".sys", NULL };
#endif
static char *tosex[ 8] = { ".tos", ".ttp", NULL };
static char *accex[ 8] = { ".acc", NULL };

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

	l = count_env(strings, 0);
	DIAG((D_shel, NULL, "count_env: %ld", l));

	argtail = kmalloc(l + 1 + tailsize + 1 + i + 1 + argvl + 1);
	if (argtail)
	{
		char *last;
		int j;

		j = copy_env(argtail, strings, 0, &last);
		DIAG((D_shel, NULL, "copy_env: %d, last-start: %ld", j, last - strings[0]));

		strcpy(last, ARGV);
		strcpy(last + argvl + 1, command);
		parse_tail(last + argvl + 1 + i + 1, p_tail + 1);
		strings[j++] = last;
		strings[j] = 0;

		if (C.env) kfree(C.env);
		C.env = argtail;

		argvtail[0] = 0x7f;
		DIAGS(("ARGV constructed"));
		IFDIAG(display_env(strings, 0);)
	}
	else
		DIAGS(("ARGV: out of memory"));
}

#if GENERATE_DIAGS
static void
print_x_shell(short x_mode, struct xshelw *x_shell)
{
	if (x_mode & SW_PSETLIMIT)
		/* memory limit */
		DIAG((D_shel, NULL, "shel_write: x_mode SW_PSETLIMIT (%li)",
			x_shell->psetlimit));

	if (x_mode & SW_PRENICE)
		/* priority renice */
		DIAG((D_shel, NULL, "shel_write: x_mode SW_PRENICE (%li)",
			x_shell->prenice));

	if (x_mode & SW_DEFDIR)
		/* explicit default dir */
		DIAG((D_shel, NULL, "shel_write: x_mode SW_DEFDIR (%s)",
			x_shell->defdir));

	if (x_mode & SW_UID)
		/* XaAES extension - set user id */
		DIAG((D_shel, NULL, "shel_write: x_mode SW_UID (%i)",
			x_shell->uid));

	if (x_mode & SW_GID)
		/* XaAES extension - set group id */
		DIAG((D_shel, NULL, "shel_write: x_mode SW_GID (%i)",
			x_shell->gid));

	if (x_mode & SW_ENVIRON)
	{
		/* explicit environment */
		char **env = &(x_shell->env);
		DIAG((D_shel, NULL, "shel_write: x_mode SW_ENVIRON"));
		display_env(env, 1);
	}
}
#endif

static int
default_path(struct xa_client *caller, char *cmd, char *path, char *name, char *defdir, struct create_process_opts *cpopts)
{
	int drv;
	/*
	 * Ozk:
	 * I am absolutely NOT SURE about this, but this is what I implemented;
	 *
	 * 1.	If x_shel is used to set default dir, we use that defaultdir,
	 *	ofcourse.
	 * 2.	If the caller's current working directory equeals its home-
	 *	directory, we take the defaultpath for the new process out of
	 *	the 'cmd', which is the path to process callers wants started.
	 * 3.	If the callers current working directory is NOT the same as
	 *	its homepath, we set the new processes default path to, yes,
	 *	the current working directory
	 * 4.	If caller is AESSYS, we always take defaultdir out of cmd.
	 * 5.	IF ANYONE KNOWS BETTER, I'M EXTREMELY INTERESTED IN KNOWING!!!
	 */
	drv = drive_and_path(cmd, path, name, true, caller == C.Aes ? true : false);
	if (!(cpopts->mode & CREATE_PROCESS_OPTS_DEFDIR))
	{
	//	display("defdir '%s'", defdir);
	//	display("apphom '%s'", caller->home_path);
		if (caller == C.Aes || (drv >= 0 && !strcmp(caller->home_path, defdir)))
		{
			defdir[0] = drv + 'a';
			defdir[1] = ':';
			strcpy(defdir + 2, path);
	//		display("use path in cmd as defdir '%s'", defdir);
		}

		cpopts->mode |= CREATE_PROCESS_OPTS_DEFDIR;
		cpopts->defdir = defdir;
	//	display("Set defdir to '%s' for %s", defdir, caller ? caller->name : "no caller");
	//	display("defdir caller '%s'", caller ? caller->home_path : "no caller");
	}
	return drv;
}

static bool is_same_file( char *f1, char *f2 )
{
	struct stat st1, st2;
	long r = f_stat64(0, f1, &st1);
	if( !r )
		r = f_stat64(0, f2, &st2);
	if( !r )
		return st1.dev == st2.dev && st1.ino == st2.ino;
	return false;
}

static int base( char *path, char **ps )
{
	char *p = strrchr( path, '\\' ), c=0;
	if( !p )
		p = strrchr( path, '/' );
	if( p )
	{
		c = *p;
		*p = 0;
	}
	if( ps )
		*ps = p;
	return c;
}

static bool is_launch_path( char *path )
{
	bool ret;
	char *p1, *p2;
	char c1 = base( cfg.launch_path, &p1 );
	char c2 = base( path, &p2 );
	ret = is_same_file( cfg.launch_path, path );
	if( p1 )
		*p1 = c1;
	if( p2 )
		*p2 = c2;
	return ret;
}

int
launch(enum locks lock, short mode, short wisgr, short wiscr,
       const char *parm, char *p_tail, struct xa_client *caller)
{
	char cmd[260]; /* 2 full paths */
	char argvtail[4];
	struct xshelw x_shell = {0};
	struct create_process_opts cpopts;
	short x_mode, real_mode;
	const char *pcmd;
	char *save_tail = NULL, *ext = NULL;
	long longtail = 0, tailsize = 0;
	Path save_cmd;
	char *tail = argvtail;
	int ret = 0;
	int drv = 0;
	Path path, name, defdir;//, cur;
	struct proc *p = NULL;
	int type = 0, follow = 0;

#if GENERATE_DIAGS
	if (caller)
	{
		DIAG((D_shel, caller, "launch for %s: 0x%x,%d,%d,%lx,%lx",
			c_owner(caller), mode, wisgr, wiscr, parm, p_tail));
		DIAG((D_shel, caller, " --- parm=%lx, tail=%lx", parm, tail));
	}
	else
	{
		DIAG((D_shel, caller, "launch for non AES process (pid %ld): 0x%x,%d,%d,%lx,%lx",
			p_getpid(), mode, wisgr, wiscr, parm, p_tail));
	}
#endif

	if (!parm)
		return -1;

	x_mode = mode & 0xff00;
	cpopts.mode = 0;
	if (x_mode)
	{
		x_shell = *(const struct xshelw *)parm;

#if GENERATE_DIAGS
		print_x_shell(x_mode, &x_shell);
#endif
		/* Do some checks before allocating anything. */
		if (!x_shell.newcmd)
			return -1;

		pcmd = x_shell.newcmd;

		/* setup create_process options */
		if (x_mode & SW_PSETLIMIT)
		{
			cpopts.mode |= CREATE_PROCESS_OPTS_MAXCORE;
			cpopts.maxcore = x_shell.psetlimit;
		}
		if (x_mode & SW_PRENICE)
		{
			cpopts.mode |= CREATE_PROCESS_OPTS_NICELEVEL;
			cpopts.nicelevel = x_shell.prenice;
		}
		if (x_mode & SW_DEFDIR)
		{
		//	display("x_modedefdir '%s' for %s", x_shell.defdir, caller ? caller->name : "no caller");
			cpopts.mode |= CREATE_PROCESS_OPTS_DEFDIR;
			cpopts.defdir = x_shell.defdir;
		}
		if (x_mode & SW_UID)
		{
			cpopts.mode |= CREATE_PROCESS_OPTS_UID;
			cpopts.uid = x_shell.uid;
		}
		if (x_mode & SW_GID)
		{
			cpopts.mode |= CREATE_PROCESS_OPTS_GID;
			cpopts.gid = x_shell.gid;
		}
	}
	else{
		pcmd = parm;
	}

	defdir[0] = d_getdrv() + 'a';
	defdir[1] = ':';
	d_getpath(defdir + 2, 0);

	argvtail[0] = 0;
	argvtail[1] = 0;

	if (p_tail)
	{

		if (p_tail[0] == 0x7f || (unsigned char)p_tail[0] == 0xff)
		{
			/* In this case the string CAN ONLY BE null terminated. */
			longtail = strlen(p_tail + 1);
			if (longtail > 124)
				longtail = 124;
			DIAG((D_shel, NULL, "ARGV!  longtail = %ld", longtail));
		}

		if (longtail)
		{
			DIAG((D_shel, NULL, " -- longtailsize=%ld", longtail));
			tailsize = longtail;
			tail = kmalloc(tailsize + 2);
			DIAG((D_shel, NULL, " -- ltail=%lx", tail));
			if (!tail)
				return 0;
			strncpy(tail + 1, p_tail + 1, tailsize);
			*tail = 0x7f;
			tail[tailsize + 1] = '\0';
			DIAG((D_shel, NULL, "long tailsize: %ld", tailsize));
		}
		else
		{
// 			(unsigned long)tailsize = (unsigned char)p_tail[0];
			tailsize = (unsigned long)(unsigned char)p_tail[0];
			DIAG((D_shel, NULL, " -- tailsize1 = %ld", tailsize));
			tail = kmalloc(126); //tail = kmalloc(tailsize + 2);
			DIAG((D_shel, NULL, " -- tail=%lx", tail));
			if (!tail)
				return 0;
			if (tailsize > 124)
				tailsize = 124;
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
		display_env(strings, 0);
	}
#endif

	/* Keep a copy of original for in client structure */
	strcpy(save_cmd, pcmd);

	/* HR: This was a very bad bug, it took me a day to find it, although it was
	 *     right before my eyes all the time.
	 *     Dont mix pascal string processing with C string processing! :-)
	 */
	save_tail = kmalloc(tailsize + 2); /* was: strlen(tail+1) */
	assert(save_tail);
	strncpy(save_tail, tail, tailsize + 1);
	save_tail[tailsize + 1] = '\0';

	if (get_drv(pcmd) >= 0)
	{
		strcpy(cmd, pcmd);
		DIAG((D_shel, 0, "cmd complete: '%s'", cmd));
	}
	else
	{
		if (caller)
		{
			/* no drive, no path, use the caller's */

			DIAG((D_shel, 0, "make cmd: '%s' + '%s'", caller->home_path, pcmd));

			strcpy(cmd, caller->home_path);
			if (*pcmd != '\\' && *(caller->home_path + strlen(caller->home_path) - 1) != '\\')
				strcat(cmd, "\\");
			strcat(cmd, pcmd);
		}
		else
		{
			int driv = d_getdrv();

			cmd[0] = (char)driv + 'A';
			cmd[1] = ':';
			d_getcwd(cmd + 2, driv + 1, sizeof(cmd) - 3);

			DIAG((D_shel, 0, "make cmd: '%s' + '%s'", cmd, pcmd));

			if (*pcmd != '\\' && *(cmd + strlen(cmd) - 1) != '\\')
				strcat(cmd, "\\");
			strcat(cmd, pcmd);
		}
		DIAG((D_shel, 0, "cmd appended: '%s'", cmd));
	}

	ext = cmd + strlen(cmd) - 4; /* Hmmmmm, OK, tos ttp and acc are really specific and always 3 */

	real_mode = mode & 0xff;

	if (real_mode == 0)
	{
		if (is_ext(ext, tosex))
		{
			real_mode = 1; wisgr = 0;
			DIAG((D_shel, NULL, "launch: -= 1,0 =-"));
		}
		else if (is_ext(ext, accex))
		{
			real_mode = 3;
			DIAG((D_shel, NULL, "launch: -= 3,%d =-", wisgr));
		}
		else
		{
			real_mode = 1; wisgr = 1;
			DIAG((D_shel, NULL, "launch: -= 1,1 =-"));
		}
	}

	switch (real_mode)
	{
		default:
		{
			DIAGS(("launch: unhandled shel_write mode %i???", real_mode));
			break;
		}
		case 1:
		{
			long r;
			/* TOS Launch?  */
			if (wisgr == 0)
			{
				const char *tosrun;

				tosrun = get_env(lock, "TOSRUN=");
				if (!tosrun)
				{
					/* check tosrun pipe */
					struct file *f;

					f = kernel_open("u:\\pipe\\tosrun", O_WRONLY, NULL, NULL);
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
					char *new_tail;
					long new_tailsize;

					new_tail = kmalloc(tailsize + 1 + strlen(cmd) + 1);
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
					kfree(tail);
					tail = new_tail;
					tail[new_tailsize + 1] = '\0';
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

			drv = default_path(caller, cmd, path, name, defdir, &cpopts);

			DIAG((D_shel, 0, "[2]drive_and_path %d,'%s','%s'", drv, path,name));

			follow = is_launch_path( cmd );

			make_argv(tail, tailsize, name, argvtail);

			if (wisgr != 0)
			{
				char linkname[PATH_MAX+1], *ps;
				XATTR xat;

				r = f_xattr(1, cmd, &xat);
				if( !r && S_ISLNK( xat.mode ) )
				{
					_f_readlink( PATH_MAX, linkname, cmd );
					if( strcmp( cmd, linkname ) )
					{
						if( follow )
							strcpy( cmd, linkname );
						ps = strrchr( linkname, '\\' );
						if( !ps )
							ps = strrchr( linkname, '/' );

						if( ps )
							*ps = 0;
						strcpy( defdir, linkname );

						/* if started from launch_path set home to link-target path */
						if( follow )
						{
							strcpy( path, linkname );	/* this gets shel_info.home_path in init_client */
						}
					}
				}

				ret = create_process(cmd, *argvtail ? argvtail : tail,
						     (x_mode & SW_ENVIRON) ? x_shell.env : *strings,
						     &p, 0, cpopts.mode ? &cpopts : NULL);

				if (ret == 0)
				{
					assert(p);

					type = APP_APPLICATION;
					ret = p->pid;
					/* switch menu off for single-task (todo: fix it) */
					if( (p->modeflags & M_SINGLE_TASK) )
						toggle_menu( lock, 0 );
				}
				else
				{
					if( ret == EPERM )
					{
						if( C.SingleTaskPid > 0 )
						{
							struct proc *s = pid2proc(C.SingleTaskPid);
							ALERT((xa_strings[AL_STMD]/*"launch: cannot enter single-task-mode: already in single-task-mode: %s(%d)."*/,
								s->name, s->pid));
						}
					}
					ret = -ret;
				}
			}

			/* restore cwd for XaAES */
			d_setdrv(toupper(*C.Aes->home_path) - 'A' );
			d_setpath(C.Aes->home_path);
			break;
		}
		case 3:	/* ACC */
		{
			struct proc *curproc = get_curproc();
			struct memregion *m;
			struct basepage *b;
			long size;

			drv = default_path(caller, cmd, path, name, defdir, &cpopts);

			DIAG((D_shel, 0, "[3]drive_and_path %d,'%s','%s'", drv, path, name));


			ret = create_process(cmd, *argvtail ? argvtail : tail,
					     (x_mode & SW_ENVIRON) ? x_shell.env : *strings,
					     &p, 256, cpopts.mode ? &cpopts : NULL);
			if (ret < 0)
			{
				BLOG((0, "acc launch failed: error %i", ret));
				DIAG((D_shel, 0, "acc launch failed: error %i", ret));
				break;
			}
			assert(p);

			DIAGS(("create_process ok, fixing ACC basepage"));
			m = addr2mem(p, (long)p->p_mem->base);
			assert(m);

			/* map basepage into cuproc */
			b = (void *)attach_region(curproc, m);
			if (!b)
			{
				/* cannot access basepage of the child
				 * -> terminate child
				 */
				ikill(p->pid, SIGKILL);
				p = NULL;
				ret = ENOMEM;
				break;
			}

			/* accsize */
			size = 256 + b->p_tlen + b->p_dlen + b->p_blen;

			DIAGS(("Copy accstart to 0x%lx, size %lu", (char *)b + size, (long)accend - (long)accstart));
			memcpy((char *)b + size, accstart, (long)accend - (long)accstart);
			cpushi((char *)b + size, (long)accend - (long)accstart);

			b->p_dbase = b->p_tbase;
			b->p_tbase = (long)((char *)b + size);

			/* set PC appropriately */
			p->ctxt[CURRENT].pc = b->p_tbase;

			/* remove mapping */
			detach_region(curproc, m);

			type = APP_ACCESSORY;
			ret = p->pid;
			p_setpgrp(p->pid, loader_pgrp);
			break;
		}
	}

	if (p)
	{
		struct shel_info *info;

		/*
		 * reparent child
		 *
		 * This is very, very ugly; we are forced todo so because all
		 * other AES implementations for FreeMiNT (and the old XaAES)
		 * use something like an server that handle most of the
		 * AES syscalls. As the server is a seperate process from
		 * the view of the FreeMiNT kernel newly created processes
		 * have this server process as parent and not the caller of
		 * shel_write(). That's why most AES programs don't handle
		 * this correctly, e.g. not calling Pwaitpid() and zombies
		 * appear (or is there something in AES I oversee here?).
		 *
		 * TODO: add XaAES extension that don't do this.
		 */

// 		p->ppid = C.Aes->p->pid;

		if (x_mode & SW_PRENICE)
		{
			DIAGS((" -- p_renice"));
			p_renice(p->pid, x_shell.prenice);
		}
		if (x_mode & SW_UID)
		{
			DIAGS((" -- proc_setuid"));
			proc_setuid(p, x_shell.uid);
		}
		if (x_mode & SW_GID)
		{
			DIAGS((" -- proc_setgid"));
			proc_setgid(p, x_shell.gid);
		}
		/*
		 * remember shel_write info for appl_init
		 */
		DIAGS((" -- attaching extension"));
		info = attach_extension(p, XAAES_MAGIC_SH, PEXT_COPYONSHARE | PEXT_SHAREONCE, sizeof(*info),
					&info_cb); //xaaes_cb_vector_sh_info);

// 		if (!(strnicmp(p->name, "qed", 3)))
// 		{
// 			struct module_callback *cb = &info_cb; //xaaes_cb_vector_sh_info;
// 			disp_cb(cb);
// 		}

		if (info)
		{
			int i = 0;
			info->type = type;

			if (caller)
				info->rppid = caller->p->pid;
			else
				info->rppid = p_getpid();

			info->reserved = ret;
			info->shel_write = true;

			info->cmd_tail = save_tail;
			info->tail_is_heap = true;
			save_tail = NULL;

			strcpy(info->cmd_name, save_cmd);

			if( !path[0] || path[1] != ':' )
			{
				*(info->home_path) = drv + 'a';
				*(info->home_path + 1) = ':';
				i = 2;
			}
			strcpy(info->home_path+i, path);
		}
		else
		{
			ikill(p->pid, SIGKILL);
			ret = ENOMEM;
		}
	}

out:
	if (tail != argvtail)
		kfree(tail);

	if (save_tail)
		kfree(save_tail);

	DIAG((D_shel, 0, "Launch for %s returns child %d (bp 0x%lx)",
		c_owner(caller), ret, p ? p->p_mem->base : NULL));
	DIAG((D_shel, 0, "Remove ARGV"));

	/* Remove ARGV */
	put_env(lock, ARGV);

	/* and go out */

	return ret;
}

unsigned long
XA_shel_write(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short wdoex = pb->intin[0];
	short wisgr = pb->intin[1];
	short wiscr = pb->intin[2];

	CONTROL(3,1,2)

#if GENERATE_DIAGS
	if (client)
	{
		DIAG((D_shel, NULL, "shel_write(0x%x,%d,%d) for %s",
			wdoex, wisgr, wiscr, client->name));
	}
	else
	{
		DIAG((D_shel, NULL, "shel_write(0x%x,%d,%d) for non AES process (pid %ld)",
			wdoex, wisgr, wiscr, p_getpid()));
	}
#endif
//	display("shel_write(0x%x,%d,%d) for %s",

//		wdoex, wisgr, wiscr, client ? client->name : "no client");

	if ((wdoex & 0xff) < SWM_SHUTDOWN) /* SWM_LANUCH, SWM_LAUNCHNOW or SWM_LAUNCACC */
	{
		Sema_Up(envstr);

		pb->intout[0] = launch(lock|envstr,
				       wdoex,
				       wisgr,
				       wiscr,
				       (char *)pb->addrin[0],
				       (char *)pb->addrin[1],
				       client);

		Sema_Dn(envstr);

		/* let the new process run */
		yield();
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
			case SWM_SHUTDOWN:
			{
				DIAGS(("shutown by shel_write(4, %d,%d)", wiscr, wisgr));

				switch (wiscr)
				{
					/* stop shutdown sequence */
					case 0:
					{
						/* not possible */
						break;
					}
					/* partial shutdown */
					case 1:
					{
					/*
					 * the only difference of partial shutdown
					 * compared to full shutdown is that ACC also
					 * get AP_TERM
					 */
						quit_all_apps(lock, client, AP_TERM);
						pb->intout[0] = 1;
						break;
					}
					/*
					 * and we need also to send status informations back
					 * to the caller
					 */

					/* full shutdown */
					case 2:
					{
						quit_all_apps(lock, client, AP_TERM);
						pb->intout[0] = 1;
						break;
					}
				}
				break;
			}

			/* resolution change */
			case SWM_REZCHANGE:
				if( !(wiscr == 0 || wiscr == 1) )
				{
					pb->intout[0] = 0;
					break;
				}
				next_res = wisgr;
				pb->intout[0] = 1;
				post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, RESOLUTION_CHANGE, 1, NULL, NULL);

				break;

			/* undefined */
			case 6:
				break;

			/* broadcast message */
			case SWM_BROADCAST:
			{
				struct xa_client *cl;

				Sema_Up(clients);

				FOREACH_CLIENT(cl)
				{
					if (is_client(cl) && cl != client)
						send_a_message(lock, cl, AMQ_NORM, 0, (union msg_buf *)cmd);
				}

				Sema_Dn(clients);
				pb->intout[0] = 1;
				break;
			}

			/* Manipulate AES environment */
			case SWM_ENVIRON:
			{
				switch (wisgr)
				{
					case ENVIRON_SIZE:
					{
						long ct = count_env(strings, NULL);
						DIAG((D_shel, 0, "XA_shell_write(wdoex 8, wisgr 0) -- count = %d", ct));
						pb->intout[0] = ct;
						break;
					}
					case ENVIRON_CHANGE:
					{
						DIAGS(("XA_shell_write(wdoex 8, wisgr 1)"));
						DIAGS(("-> env add '%s'", cmd));
						pb->intout[0] = put_env(lock, cmd);
						break;
					}
					case ENVIRON_COPY:
					{
						long ct = count_env(strings, NULL);
						long ret;

						DIAG((D_shel, 0, "XA_shell_write(wdoex 8, wisgr 2) -- copy"));
						memcpy(cmd, strings[0], wiscr > ct ? ct : wiscr);
						ret = ct > wiscr ? ct - wiscr : 0;

						DIAG((D_shel, 0, "XA_shell_write(wdoex 8, wisgr 2) -- & left %ld", ret));
						pb->intout[0] = ret;
						break;
					}
				}
				break;
			}

			/* notify that app understands AP_TERM */
			case SWM_NEWMSG:
			{
				if (client)
				{
					client->swm_newmsg |= wisgr; //client->apterm = (wisgr & 1) != 0;
				}
				pb->intout[0] = 1;
				break;
			}

			/* Send a msg to the AES  */
			case SWM_AESMSG:
			{
				pb->intout[0] = 1;
				break;
			}

			/* create new thread */
			case SWM_THRCREATE:
			{
			#if 0
			#endif
				break;
			}

			/* thread exit */
			case 21:
			{
				break;
			}

			/* terminate thread */
			case 22:
			{
				break;
			}

			default:
				DIAGS(("shel_write: unknown wdoex %i", wdoex));
				break;
		}
	}

	return XAC_DONE;
}


/* I think that the description if shell_read in the compendium is
 * rubbish. At least on Atari GEM the processes own command must be
 * given, NOT that of the parent.
 */
/*
 * Ozk: XA_shel_read() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */
unsigned long
XA_shel_read(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char *name = (char *)pb->addrin[0];
	char *tail = (char *)pb->addrin[1];
	char *myname = NULL;
	char *mytail = NULL;

	CONTROL(0,1,2)
	if (client)
	{
		myname = client->cmd_name;
		mytail = client->cmd_tail;
	}
	else
	{
		struct shel_info *info = NULL;

		info = lookup_extension(NULL, XAAES_MAGIC_SH);
		if (info)
		{
			myname = info->cmd_name;
			mytail = info->cmd_tail;
		}
	}

	if (myname && mytail)
	{
		int f;

		strcpy(name, *myname ? myname : "\\");

		for (f = 0; f < mytail[0] + 1; f++)
			tail[f] = mytail[f];

		tail[f] = '\0';

		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	DIAG((D_shel, client, "shel_read: n='%s', t=%d'%s'",
		pb->addrin[0], *(char *)pb->addrin[1], (char *)pb->addrin[1]+1));

	return XAC_DONE;
}

/*
 * wildcard filename matching:
 *	returns TRUE iff 'name' matches the pattern in 'template'
 *
 * this ought to be already available somewhere in MiNT ...
 */
static int
wc_match (const char *name, const char *template, bool nocase)
{
	char c0, c1;
	const char *p, *q;
	int stop;

	DIAGS(("wc_match '%s' - '%s'", template, name));

	for (p = name, q = template; *p; )
	{
		if (nocase) {
			c0 = tolower(*p);
			c1 = tolower(*q);
		} else
			c0 = *p, c1 = *q;

		if (c0 == c1)
		{
			p++, q++;
			continue;
		}
		stop = (*p == '.');
		switch(*q)
		{
		case '?':
			if (stop)
				return 0;
			p++, q++;
			break;
		case '*':
			if (stop)
				q++;
			else p++;
			break;
		default:
			return 0;
		}
	}
	if (!*q || (*q == '*' && *(q+1) == 0))
	{
		return 1;
	}

	return 0;
}

/*
 * like f_stat64(), but allows wildcard filename spec, and checks for regular file
 * copy found name into result if not 0 (and node != 0)
 */
static long
wc_stat64(int mode, const char *node, char *fn, struct stat *st, char *result)
{
	struct dirstruct dirh;
	int len;
	char *path, *name;
	char fname[256];
	char buf[256];
	long r;

	/*
	 * If path empth, try the filename on its own (->use pwd?)
	 */
	if (!node)
	{
		r = f_stat64(mode, fn, st);
		if (r == 0 && S_ISREG(st->mode))
			return 0;
		else
			return -1;
	}

	path = buf;
	strcpy(buf, node);
	len = strlen(buf);
	if (buf[len] != '\\')
		buf[len++] = '\\';

	buf[len] = '\0';

	r = kernel_opendir(&dirh, buf);

	if (r)
	{
		DIAGS(("Error %lx opening dir %s", r, buf));
		return -1;
	}

	name = buf + len;
	len = sizeof(buf) - len;

	while(!(r = kernel_readdir(&dirh, fname, sizeof(fname))))
	{
		strcpy(name, fname + 4);

		if (wc_match(name, fn, true))
		{
			r = f_stat64(mode, path, st);
			if (r == 0 && S_ISREG(st->mode))
			{
				kernel_closedir(&dirh);
				if( result )
					strcpy( result, name );
				return 0;
			}
		}
	}
	kernel_closedir(&dirh);
	return -1;
}

/*
 * search-strategy:
 *
 * 1. filename has an absolute path -> goto 6.
 *
 * 2. split path from filename -> filepath, filename
 *
 * 3. if usehome is true do wildcard-search for $HOME\[filepath\]filename
 *
 * 4. do wildcard-search for client-home\[filepath\]filename
 *    client-home is the directory where the client was started from
 *
 * 5. if filename does not contain a path-component do a wildcard-search for filename in $PATH
 *
 * 6. look for filename without any processing
 *
 * wildcard-search: case-insensitive, wildcards ? and * possible (DOS-like pattern-matching)
 *
 */
char *
shell_find(enum locks lock, struct xa_client *client, char *fn)
{
	char *path, pathsep;
	char cwd[256];
	char result[PATH_MAX];

	struct stat st;
	long r;

	const char *kh;
	int f = 0, l, n;
	long len = PATH_MAX * 2;

	path = kmalloc(len);
	if (path)
	{
		char *p=0, *pf = fn, c=0;

		if ( !( ( isalpha(*fn) && *(fn + 1) == ':' )
			|| *fn == '/' || *fn == '\\' ) )	/* no absolute path given */
		{
			char fpath[PATH_MAX];
			p = strrchr( fn, '\\');
			if( !p )
				p = strrchr( fn, '/');
			if( p )	/* filename contains directory */
			{
				c = *p;
				*p = 0;
				strcpy( fpath, fn );
				fn = p+1;
			}

			//DIAGS(("shell_find for %s '%s', PATH= '%s'", client->name, fn ? fn : "~", kp ? kp : "~"));
			DIAGS(("shell_find for %s '%s',p='%s'", client->name, fn ? fn : "~", p));

			/* check $HOME directory */
			if (cfg.usehome )
			{
				kh = get_env(lock, "HOME=");
				if( kh )
				{
					if( p )	/* append path from filename */
					{
						sprintf( fpath, sizeof(fpath)-1, "%s%c%s", kh, c, pf );
						kh = fpath;
					}
					r = wc_stat64(0, kh, fn, &st, result);
					DIAGS(("[2]  --   try: '%s\\%s' :: %ld", kh, fn, r));
					if (r == 0)
					{
						sprintf(path, len, "%s\\%s", kh, result);
						return path;
					}
				}
			}

			/* try the file spec in the apps' dir */
			kh = client->home_path;
			if( p )
			{
				sprintf( fpath, sizeof(fpath)-1, "%s%c%s", kh, c, pf );
				kh = fpath;
			}
			r = wc_stat64(0, kh, fn, &st, result);
			DIAGS(("[0]  --    try: '%s' :: %ld", fn, r));
			if (r == 0 )
			{
				sprintf(path, len, "%s\\%s", kh, result);
				return path;
			}

			if( !p )	/* only search PATH if fn does not contain pathsep */
			{
				kh = get_env(lock, "PATH=");
				/* the PATH env could be simply absent */
				if (kh)
				{
					/* Check our PATH environment variable */
					/* l = strlen(cwd); */
					/* cwd uninitialized; after sprintf? or is it kh? or is it sizeof? */
					l = strlen(kh);

						/* eval path seperator */
					if( strchr( kh, ',' ) )
						pathsep = ',';
					else if( strchr( kh, ';' ) )
						pathsep = ';';
					else
						pathsep = ':';

					strcpy(cwd, kh);
					while (f < l)
					{
						n = f;
						while ( cwd[n] && cwd[n] != pathsep )
						{
							if (cwd[n] == '/')
								cwd[n] = '\\';
							n++;
						}
						if (cwd[n-1] == '\\')
							cwd[n-1] = '\0';
						cwd[n] = '\0';

	// 					sprintf(path, sizeof(path), "%s\\%s", cwd + f, fn);
						r = wc_stat64(0, cwd + f, fn, &st, result);
						DIAGS(("[3]  --   try: '%s\\%s' :: %ld", cwd + f, fn, r));
						if (r == 0)
						{
							sprintf(path, len, "%s\\%s", cwd + f, result);
							return path;
						}

						f = n + 1;
					}
				}
			}

			/* Try clients current path: */
			sprintf(cwd, sizeof(cwd), "%c:%s", client->xdrive + 'a', client->xpath);
			r = wc_stat64(0, cwd, fn, &st, result);
			DIAGS(("[4]  --   try: '%s\\%s' :: %ld", cwd, fn, r));
			if (r == 0)
			{
				sprintf(path, len, "%c:%s\\%s", client->xdrive + 'a', client->xpath, result);
				return path;
			}
		}

		if( p )
		{
			fn = pf;
			*p = c;
		}
		/* Last ditch - try the file spec on its own */
		r = wc_stat64(0, NULL, fn, &st, 0);
		DIAGS(("[5]  --    try: '%s' :: %ld", fn, r));
		if (r == 0)
		{
			strncpy(path, fn, len);
			return path;
		}

		kfree(path);
		path = NULL;
	}

	DIAGS((" - %lx", path));
	return path;
}

unsigned long
XA_shel_find(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char *fn = (char *)(pb->addrin[0]);
	char *path;
	short ret = 0; /* default == didnt find file */

	CONTROL(0,1,1)

	path = shell_find(lock, client, fn);
	if (path)
	{
		strcpy(fn, path);
		ret = 1;
		kfree(path);
	}
	pb->intout[0] = ret;

	return XAC_DONE;
}

#if GENERATE_DIAGS

static void
display_env(char **env, int which)
{
	//if (D.debug_level > 2 && D.point[D_shel])
	{
		if (which == 1)
		{
			const char *e = *env;
			DIAGS(("Environment as superstring:\n"));
			while (*e)
			{
				DIAGS((" -- %lx='%s'\n", e, e));
				e += strlen(e)+1;
			}
		}
		else
		{
			DIAGS(("Environment as row of pointers:"));
			while (*env)
			{
				DIAGS((" -- %lx='%s'\n", *env, *env));
				env++;
			}
		}
	}
}
#endif

/* This version finds XX & XX= */
const char *
get_env(enum locks lock, const char *name)
{
	int i;

	Sema_Up(envstr);

	for (i = 0; strings[i]; i++)
	{
		const char *f = strings[i];
		const char *n = name;

		for (;;)
		{
			if (*f != *n)
			{
				if (*f == '=' && *n == '\0')
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
		if (!strncmp(s[i], ARGV, 4))
		{
			DIAGS(("copy_env ARGV: skipped remainder of environment"));
			break;
		}

		if (!l || strncmp(s[i], without, l))
// 		if (l == 0 || (l != 0 && strncmp(s[i], without, l) != 0))
		{
			strcpy(to, s[i]);
			s[j++] = to;
			to += strlen(to) + 1;
		}
		i++;
	}

	strings[j] = '\0';

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
put_env(enum locks lock, const char *cmd)
{
	long ret = 0;

	Sema_Up(envstr);

	if (cmd)
	{
		long ct;
		char *newenv;

		DIAG((D_shel, NULL, " -- change; '%s'", cmd ? cmd : "~~"));
		/* count without */
		ct = count_env(strings, cmd);
		/* ct is including the extra \0 at the end! */

		/* ends with '=': remove */
		if (*(cmd + strlen(cmd) - 1) == '=')
		{
			newenv = kmalloc(ct);
			if (newenv)
				/* copy without */
				copy_env(newenv, strings, cmd, NULL);
		}
		else
		{
			int l = strlen(cmd) + 1;
			newenv = kmalloc(ct + l);
			if (newenv)
			{
				char *last;
				/* copy without */
				int j = copy_env(newenv, strings, cmd, &last);
				strings[j] = last;
				/* add new */
				strcpy(strings[j], cmd);
				*(strings[j] + l) = '\0';
				strings[j + 1] = NULL;
			}
		}
		if (newenv)
		{
			if (C.env) kfree(C.env);
			C.env = newenv;
		}

		DIAGS(("putenv"));
		IFDIAG(display_env(strings, 0);)
	}
	Sema_Dn(envstr);
	return ret;
}

void
init_env(void)
{
	int i = STRINGS;
	char **s = strings;

	for (i = 0; i < STRINGS; i++) *s++ = NULL;
}

/* HR: because everybody can mess around with the AES's environment
 *     (shel_write(8, ...) changing the pointer array strings,
 *     it is necessary to make a permanent copy of the variable.
 */
/*
 * Ozk: XA_shel_envrn() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */

unsigned long
XA_shel_envrn(enum locks lock, struct xa_client *client, AESPB *pb)
{
	char **p = (char **)pb->addrin[0];
	const char *name = (const char *)pb->addrin[1];

	CONTROL(0,1,2)

	if (p && name)
	{
		const char *pf = get_env(lock, name);

		*p = NULL;

		DIAGS(("shell_env for %s: '%s' :: '%s'", client ? c_owner(client) : "non-aes process", name, pf ? pf : "~~~"));
		if (pf)
		{
			*p = umalloc(strlen(pf) + 1);
			if (*p)
				strcpy(*p, pf);
		}
	}

	pb->intout[0] = 1;

	return XAC_DONE;
}

#if INCLUDE_UNUSED
static struct xa_client *
lookup_proc_name(const char *name)
{
	struct xa_client *client;

	FOREACH_CLIENT(client)
	{
		if (stricmp(name, client->proc_name) == 0)
			break;
	}

	return client;
}
#endif
#if INCLUDE_UNUSED
unsigned long
XA_shel_help(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short sh_hmode;
	char *tmp = NULL;

	CONTROL(1,1,2)

	pb->intout[0] = 0;

	sh_hmode = pb->intin[0];
	if (sh_hmode == 0)
	{
		struct helpserver *help = NULL;

		const char *sh_hfile = (const char *)pb->addrin[0];
		const char *sh_hkey = (const char *)pb->addrin[1];

		const char *file;
		const char *ext;

		if (!sh_hfile) sh_hfile = "";
		if (!sh_hkey) sh_hkey = "";

		file = sh_hfile;

		/* search end of string */
		while (*file++)
			;

		/* search first path delimiter */
		do {
			file--;
		}
		while (file >= sh_hfile && *file != '\\' && *file != '/' && *file != ':');

		/* filename starts here */
		file++;

		/* in case someone called without filename on path */
		if (!*file)
		{
			DIAGS(("shel_help: wrong argument, no filename (%s)", sh_hfile));
			goto out;
		}

		/* lookup extension */
		ext = strrchr(file, '.');
		if (!ext)
		{
			/* use first extension */

			help = cfg.helpservers;
			if (!help)
			{
				DIAGS(("shel_help: no helpservers configured"));
				goto out;
			}

			tmp = kmalloc(strlen(sh_hfile) + strlen(help->ext) + 2);
			if (!tmp)
			{
				DIAGS(("shel_help: out of memory"));
				goto out;
			}

			strcpy(tmp, sh_hfile);
			strcat(tmp, ".");
			strcat(tmp, help->ext);

			sh_hfile = tmp;
		}
		else
			ext++;

		if (!help)
		{
			/* search for registered extension */

			help = cfg.helpservers;
			while (help)
			{
				if (stricmp(ext, help->ext) == 0)
					break;

				help = help->next;
			}
		}

		if (help)
		{
			struct xa_client *server;

			server = lookup_proc_name(help->name);
			if (server)
			{
				// if running send VA_START with sh_hfile/sh_hkey
			}
			else if (help->path)
			{
				// if not running start helpserver, argv with sh_hfile/sh_hkey
			}
		}
		else
		{
			DIAGS(("shel_help: no helpserver for '%s' found!", sh_hfile));
		}
	}

out:
	if (tmp) kfree(tmp);

	return XAC_DONE;
}
#endif
