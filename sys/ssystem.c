/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 * Copyright by jerry g geiger
 * <jerry@zedat.fu-berlin.de> or <jerry@merlin.abacus.de>
 *
 * almost completely rewritten by Draco, draco@mi.com.pl,
 * Warszawa, 4.XII.1997.
 *
 * added time related stuff, Guido, gufl0000@stud.uni-sb.de Mar/Apr 1998.
 *
 *
 * General purpose: access vital system variables and constants without need
 * to switch to Supervisor mode. Prototype:
 *
 * long _cdecl s_system (int mode, ulong arg1, ulong arg2);
 *
 */

# include "ssystem.h"
# include "global.h"

# include "buildinfo/version.h"
# include "libkern/libkern.h"

# include "arch/cpu.h"
# include "arch/mprot.h"
# include "arch/user_things.h"

# include "mint/file.h"
# include "mint/filedesc.h"
# include "mint/signal.h"

# include "bios.h"
# include "block_IO.h"
# include "cookie.h"
# include "info.h"
# include "keyboard.h"
# include "kmemory.h"
# include "memory.h"
# include "k_prot.h"
# include "proc.h"
# include "time.h"
# include "update.h"


# if 0
short run_level = 1;		/* default runlevel */
# endif
short disallow_single = 0;


long _cdecl
sys_s_system (int mode, ulong arg1, ulong arg2)
{
	ushort isroot = suser (curproc->p_cred->ucr);
	ulong *lpointer;
	ushort *wpointer;
	uchar *bpointer;
	ulong *sysbase;
	short *mfp;

	register long r = E_OK;

	TRACE(("enter s_system(): mode %04x, arg1 %08x, arg2 %08x", mode, arg1, arg2));

	switch (mode)
	{
		case -1:
		{
			TRACE(("exit s_system(): call exists"));
			break;
		}
		/*
		 * Kernel information
		 */
		case S_OSNAME:
		{
			r = 0x4d694e54L; 	/* MiNT */
			break;
		}
		case S_OSXNAME:
		{
			r = 0x46726565;		/* Free */
			break;
		}
		case S_OSVERSION:
		{
			r = MiNT_version;
			break;
		}
		case S_OSHEADER:
		{
			/* We return a complete and corrected OSHEADER. */
			sysbase = *((ulong **)(0x4f2L));
			arg1 &= 0x000000fe;	/* 256 bytes allowed */
			if (((ushort *)sysbase)[1] < 0x102 && arg1 >= 0x20 && arg1 <= 0x2c)
			{
				/* BUG: RAM-TOS before 1986 could differ! */
				static const long sysbase0x100[2][4] =
				     {{0x56FA, 0xE1B, 0x602C, 0},
				      {0x7E0A, 0xE1B, 0x873C, 0}};
				r = sysbase0x100[(((ushort *)sysbase)[14] >> 1) == 4][(arg1 - 0x20) / sizeof(ulong)];
			}
			else
			{
				/* Correct a bug in TOS >= 1.02 during AUTO folder
				 * or with AHDI, correcting an AHDI bug.
				 * Only necessary for os_conf!
				 */
				if (arg1 == 0x1a)	/* special to read os_conf */
				{
					sysbase = (ulong *)(sysbase[2]);
					r = ((ushort *)sysbase)[14];
				}
				else if (arg1 == 0x1c)
				{
					r = ((ushort *)sysbase)[15];
					sysbase = (ulong *)(sysbase[2]);
					r |= (ulong)(((ushort *)sysbase)[14]) << 16;
				}
				else
					r = sysbase[arg1 / sizeof(ulong)];
			}
			break;
		}
		case S_OSBUILDDATE:
		{
			r = MiNT_date;
			break;
		}
		case S_OSBUILDTIME:
		{
			r = MiNT_time;
			break;
		}
		case S_OSCOMPILE:
		{
# ifdef CPU060
			r = 0x0000003cL;	/* 060 kernel */
# endif
# ifdef CPU040
			r = 0x00000028L;	/* 040 kernel */
# endif
# ifdef CPU030
			r = 0x0000001eL;	/* 030 kernel */
# endif
			break;			/* generic 68000 */
		}

/* A bitfield specifying the built-in system features.
 * Each '1' means that the feature is present:
 * ---
 * 0 - memory protection support (1 = enabled)
 * 1 - virtual memory support (1 = enabled)
 * 2 - storage encryption (1 = supported)
 * 3-31 reserved bits
 * ---
 */
		case S_OSFEATURES:
		{
# ifdef CRYPTO_CODE
			r =  ((!no_mem_prot) & 0x01) | 0x04;
# else
			r = ((!no_mem_prot) & 0x01);
# endif
			break;
		}
		/*
		 * GEMDOS variables
		 */
		case S_GETLVAL:
		{
			arg1 &= 0xfffffffeUL;
			lpointer = (ulong *) arg1;
			if (arg1 < 0x0420 || arg1 > 0xfffcUL)
				DEBUG (("GET_LVAL: address out of range"));
# ifdef JAR_PRIVATE
			else if (arg1 == 0x5a0)
				r = sys_b_setexc(0x0168, -1L);
# endif
			else
				r = *lpointer;
			break;
		}
		case S_GETWVAL:
		{
			arg1 &= 0xfffffffeUL;
			wpointer = (ushort *) arg1;
			if (arg1 < 0x0420 || arg1 > 0xfffeUL)
				DEBUG (("GET_WVAL: address out of range"));
			else
				r = *wpointer;
			break;
		}
		case S_GETBVAL:
		{
			bpointer = (uchar *) arg1;
			if (arg1 < 0x0420 || arg1 > 0xffffUL)
				DEBUG (("GET_BVAL: address out of range"));
			else
				r = *bpointer;
			break;
		}
		case S_SETLVAL:
		{
			if (isroot == 0)
			{
				DEBUG (("SET_LVAL: access denied"));
				r = EPERM;
				break;
			}
			arg1 &= 0xfffffffeUL;
			lpointer = (ulong *) arg1;
			if (arg1 < 0x0420 || arg1 > 0xfffc)
			{
				DEBUG (("SET_LVAL: address out of range"));
				r = EBADARG;
				break;
			}
# ifdef JAR_PRIVATE
			if (arg1 == 0x5a0)
				r = sys_b_setexc(0x0168, arg2);
			else
				*lpointer = arg2;
# else
			*lpointer = arg2;
# endif
			break;
		}
		case S_SETWVAL:
		{
			if (isroot == 0)
			{
				DEBUG (("SET_WVAL: access denied"));
				r = EPERM;
				break;
			}
			arg1 &= 0xfffffffeUL;
			wpointer = (ushort *) arg1;
			if (arg1 < 0x0420 || arg1 > 0xfffe)
			{
				DEBUG (("SET_WVAL: address out of range"));
				r = EBADARG;
				break;
			}
			*wpointer = arg2;
			break;
		}
		case S_SETBVAL:
		{
			if (isroot == 0)
			{
				DEBUG (("SET_BVAL: access denied"));
				r = EPERM;
				break;
			}
			bpointer = (uchar*) arg1;
			if (arg1 < 0x0420 || arg1 > 0xffff)
			{
				DEBUG (("SET_BVAL: address out of range"));
				r = EBADARG;
				break;
			}
			*bpointer = arg2;
			break;
		}
		/*
		 * Cookie Jar functions
		 */
		case S_GETCOOKIE:
		{
			r = get_cookie (NULL, arg1, (long *) arg2);

			break;
		}
		case S_SETCOOKIE:
		{
# ifndef JAR_PRIVATE
			if (isroot == 0)
			{
				DEBUG (("SET_COOKIE: access denied"));
				r = EPERM;
			}
			else	r = set_cookie (NULL, arg1, arg2);
# else
			/* Gluestik kludges, part I */
			if (isroot && (arg1 == COOKIE_STiK || arg1 == COOKIE_ICIP))
			{
				USER_THINGS *ut;
				PROC *p;

				for (p = proclist; p; p = p->gl_next)
				{
					if (p != curproc && p->wait_q != ZOMBIE_Q && p->wait_q != TSR_Q)
					{
						if (p->p_mem->tp_reg)
						{
							attach_region(curproc, p->p_mem->tp_reg);
							ut = (USER_THINGS *)p->p_mem->tp_ptr;
							set_cookie((COOKIE *)ut->user_jar_p, arg1, arg2);
							detach_region(curproc, p->p_mem->tp_reg);
						}
						else if (p->pid == 0)
							set_cookie((COOKIE *)kernel_things.user_jar_p, arg1, arg2);
					}
				}
			}

			r = set_cookie(NULL, arg1, arg2);
# endif
			break;
		}
		case S_DELCOOKIE:
		{
# ifndef JAR_PRIVATE
			if (isroot == 0)
			{
				DEBUG (("DEL_COOKIE: access denied"));
				r = EPERM;
			}
			else	r = del_cookie (NULL, arg1);
# else
			/* Gluestik kludges, part II */
			if (isroot && (arg1 == COOKIE_STiK || arg1 == COOKIE_ICIP))
			{
				USER_THINGS *ut;
				PROC *p;

				for (p = proclist; p; p = p->gl_next)
				{
					if (p != curproc && p->wait_q != ZOMBIE_Q && p->wait_q != TSR_Q)
					{
						if (p->p_mem->tp_reg)
						{
							attach_region(curproc, p->p_mem->tp_reg);
							ut = (USER_THINGS *)p->p_mem->tp_ptr;
							del_cookie((COOKIE *)ut->user_jar_p, arg1);
							detach_region(curproc, p->p_mem->tp_reg);
						}
						else if (p->pid == 0)
							del_cookie((COOKIE *)kernel_things.user_jar_p, arg1);
					}
				}
			}

			r = del_cookie(NULL, arg1);
# endif
			break;
		}

		/* Hack (dirty one) for MiNTLibs */
		case S_TIOCMGET:
		{
			mfp = (short *) arg1;
			r = ((*mfp) & 0x00ff);

			break;
		}

		case S_SECLEVEL:
		{
			if (isroot == 0)	r = EPERM;
			else if (arg1 == -1)	r = secure_mode;
			else if (arg1 > 2)	r = EBADARG;
			else			secure_mode = arg1;

			break;
		}
# if 0 /* bogus concept & code */
		case RUN_LEVEL:
		{
			if (arg1 == -1)				r = run_level;
			if (isroot == 0 || disallow_single)	r = EPERM;
			if (arg1 == 0 || arg1 == 1)		run_level = arg1;
			else					r = EACCES;

			break;
		}
# endif
		case S_CLOCKUTC:
		{
			/* I think we shouldn't allow that if the real
			 * user-id only is root.  I think that all calls
			 * that require super-user privileges should
			 * return EACCES if only the real user-id is
			 * root.  This does not only apply to Ssystem
			 * but to a lot of other calls too like
			 * Pkill.  (Personal opinion of Guido).
			 * Agreed. Done. (Draco)
			 */
 			if (arg1 == -1)		r = clock_mode;
			else if (isroot == 0)	r = EPERM;
			else			warp_clock (arg1 ? 1 : 0);

 			break;
		}
		case S_FASTLOAD:
		{
			if (isroot == 0)	r = EPERM;
			else if (arg1 == -1)	r = forcefastload;
			else 			forcefastload = arg1 ? 1 : 0;

			break;
		}
		case S_SYNCTIME:
		{
			if (isroot == 0)	r = EPERM;
			else if (arg1 == -1)	r = sync_time;
			else if (arg1 > 0)	sync_time = arg1;
			else			r = EBADARG;

			break;
		}
		case S_BLOCKCACHE:
		{
			if (isroot == 0)	r = EPERM;
			else			r = bio_set_percentage (arg1);

			break;
		}
		case S_TSLICE:
		{
			if (isroot == 0)	r = EPERM;
			else if (arg1 == -1)	r = time_slice;
			else if (arg1 >= 0 && arg1 <= 20) time_slice = arg1;
			else			r = EBADARG;

			break;
		}
		case S_FLUSHCACHE:
		{
			cpush ((void *) arg1, (long) arg2);
			break;
		}
		case S_CTRLCACHE:
		{
			if (arg1 == -1 && arg2 == -1)	r = E_OK;
			else if (arg1 == -1)		r = ccw_get ();
			else if (arg2 == -1)		r = ccw_getdmask ();
			else if (isroot == 0)		r = EPERM;
			else				r = ccw_set (arg1, arg2);

			break;
		}
		case S_INITIALTPA:
		{
			if (isroot == 0)	r = EPERM;
			else if (arg1 == -1)	r = initialmem;
			else if (arg1 > 0)	initialmem = arg1;
			else			r = EBADARG;

			break;
		}

		case S_CAD:
		{
			if (!isroot)		r = EPERM;

			/* Arguments:
			 *
			 * if arg1 == 100, then revert to defaults;
			 * arg2 provides information which keys to reset (0, 1, 2)
			 *
			 * if arg1 == 101, then it is setting a signal;
			 * arg2 is bitfield:
			 * 31-16: pid to be signalled
			 * 8-15: signal number
			 * 7-0 : which keys to redefine (0, 1, 2)
			 *
			 * if arg1 == 102, then it is setting an exec;
			 * arg2 is a pointer to the following:
			 * short key, which keys to redefine
			 * char *exe, to the executable file
			 * char *cmd, to the command line
			 * char *env, to the environment
			 */
			/* XXX: I don't understand what to do with the definition in
			 * unix/reboot.c (in the mintlib, that is), so arg1 values
			 * used in there are reserved for now.
			 */
			else if (arg1 == 100)
			{
				ushort which = (ushort)arg2;

				if (which > 2)
					r = EBADARG;
				else
				{
					if ((cad[which].action == 2) && cad[which].par.path)
						kfree(cad[which].par.path);
					cad[which].action = 0;
					cad[which].par.pid = 0;
					cad[which].aux.arg = 0;
					cad[which].env = NULL;
				}
			}
			else if (arg1 == 101)
			{
				ushort which, sig, pid;

				pid = (ushort)(arg2 >> 16);
				sig = (ushort)(arg2 >> 8);
				sig &= 0x00ff;
				which = (ushort)(arg2);
				which &= 0x00ff;

				/* XXX: Perhaps SIGKILL etc should also be filtered out?
				 * What about pid verification?
				 */
				if (which > 2 || pid > MAXPID || sig >= NSIG)
					r = EBADARG;
				else
				{
					if ((cad[which].action == 2) && cad[which].par.path)
						kfree(cad[which].par.path);
					cad[which].action = 1;
					cad[which].par.pid = pid;
					cad[which].aux.arg = sig;
				}
			}
			else if (arg1 == 102)
			{
				typedef struct
				{
					ushort key;
					char *exe;
					char *cmd;
					char *env;
				} CADPROC;

				CADPROC *cp;
				char *bbuf;
				long exel, cmdl, envl;

				cp = (CADPROC *)arg2;
				if (cp->key > 2)
					r = EBADARG;
				else
				{
					/* If any of the pointers is not valid,
					 * a bus error will kill the caller now
					 */
					exel = strlen(cp->exe) + 1;
					cmdl = strlen(cp->cmd) + 1;
					envl = strlen(cp->env) + 1;
					bbuf = kmalloc(exel + cmdl + envl);
					if (!bbuf)
						r = ENOMEM;
					else
					{
						if ((cad[cp->key].action == 2) && cad[cp->key].par.path)
							kfree(cad[cp->key].par.path);
						cad[cp->key].action = 2;
						cad[cp->key].par.path = bbuf;
						strcpy(bbuf, cp->exe);
						bbuf += exel;
						cad[cp->key].aux.cmd = bbuf;
						strcpy(bbuf, cp->cmd);
						bbuf += cmdl;
						cad[cp->key].env = bbuf;
						strcpy(bbuf, cp->env);
					}
				}
			}
			break;
		}

		case S_LOADKBD:
		{
			if (isroot == 0)
				r = EPERM;
			else
				r = load_keyboard_table((char *)arg1, 1);
			break;
		}

		/* experimental section
		 */
		case S_KNAME:
		{
			strncpy_f ((char *) arg1, version, arg2);
			break;
		}
		case S_CNAME:
		{
			strncpy_f ((char *) arg1, COMPILER_NAME, arg2);
			break;
		}
		case S_CVERSION:
		{
			strncpy_f ((char *) arg1, COMPILER_VERS, arg2);
			break;
		}
		case S_CDEFINES:
		{
			strncpy_f ((char *) arg1, COMPILER_DEFS, arg2);
			break;
		}
		case S_COPTIM:
		{
			strncpy_f ((char *) arg1, COMPILER_OPTS, arg2);
			break;
		}

		/* debug section
		 */
		case S_DEBUGLEVEL:
		{
			if (isroot == 0)	r = EPERM;
			else if (arg1 == -1)	r = debug_level;
			else if (arg1 >= 0)	debug_level = arg1;
			else			r = EBADARG;

			break;
		}
		case S_DEBUGDEVICE:
		{
			if (isroot == 0)	r = EPERM;
			else if (arg1 == -1)	r = out_device;
			else if (arg1 >= 0 && arg1 <= 9) out_device = arg1;
			else			r = EBADARG;

			break;
		}
		default:
		{
			DEBUG (("s_system(): invalid mode %d", mode));
			r = ENOSYS;

			break;
		}
	}

	TRACE (("s_system() returned %ld", r));
	return r;
}
