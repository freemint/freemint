/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 */

/* miscellaneous DOS functions */

# include "dos.h"
# include "global.h"

# include "arch/halt.h"		/* hw_poweroff, hw_halt, ... */
# include "arch/intr.h"
# include "arch/timer.h"

# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/filedesc.h"
# include "mint/pathconf.h"
# include "mint/signal.h"
# include "mint/stat.h"

# include "console.h"
# include "cookie.h"
# include "filesys.h"
# include "k_prot.h"
# include "memory.h"
# include "proc.h"
# include "proc_help.h"
# include "signal.h"
# include "timeout.h"
# include "util.h"

# ifdef OLDTOSFS
# include "tosfs.h"
# endif


long _cdecl
sys_s_version (void)
{
	TRACE(("Sversion()"));
# ifdef OLDTOSFS
	/* one direct call to ROM less */
	return gemdos_version;
# else
	/* yeah, no GEMDOS call allows our own GEMDOS version */
	return 0x4000;
# endif
}

/*
 * Super(new_ssp): change to supervisor mode.
 */
long _cdecl
sys_s_uper (long new_ssp)
{
	PROC *p = get_curproc();
	register int in_super;
	register long r;

	/* Inappropriate callers will be killed. This is
	 * lotsa safer when you set no memory protection.
	 */
	if ((secure_mode > 1) && (p->p_cred->ucr->euid != 0))
	{
		DEBUG (("Super(%lx) by non privileged process!", new_ssp));
		raise (SIGSYS);
		return new_ssp;
	}

	TRACE (("Super(%lx)", new_ssp));
	in_super = p->ctxt[SYSCALL].sr & 0x2000;
	if (new_ssp == 1)
	{
		r = in_super ? -1L : 0;
	}
	else
	{
		p->ctxt[SYSCALL].sr ^= 0x2000;
		r = p->ctxt[SYSCALL].ssp;
		if (in_super)
		{
			if (new_ssp == 0)
			{
				DEBUG (("bad Super() call"));
				raise (SIGSYS);
			}
			else
			{
				p->ctxt[SYSCALL].usp = p->ctxt[SYSCALL].ssp;
				p->ctxt[SYSCALL].ssp = new_ssp;
			}
		}
		else
		{
			p->ctxt[SYSCALL].ssp = new_ssp ?
				new_ssp : p->ctxt[SYSCALL].usp;
		}
	}
	return r;
}

/*
 * GEMDOS extension: Syield(): give up the processor if any other
 * processes are waiting. Always returns 0.
 */
long _cdecl
sys_s_yield (void)
{
	PROC *p = get_curproc();

	/* reward the nice process */
	p->curpri = p->pri;
	sleep (READY_Q, p->wait_cond);

	return E_OK;
}

/*
 * GEMDOS extensions: routines for getting/setting process i.d.'s and
 * user i.d.'s
 */

long _cdecl sys_p_getpid (void) { return get_curproc()->pid; }
long _cdecl sys_p_getppid (void) { return get_curproc()->ppid; }
long _cdecl sys_p_getpgrp (void) { return get_curproc()->pgrp; }

/*
 * note: Psetpgrp(0, ...) is equivalent to Psetpgrp(Pgetpid(), ...)
 * also note: Psetpgrp(x, 0) is equivalent to Psetpgrp(x, x)
 */

long _cdecl
sys_p_setpgrp (int pid, int newgrp)
{
	PROC *p = get_curproc();
	PROC *t;

	TRACE (("Psetpgrp(%i, %i)", pid, newgrp));

	if (pid == 0)
	{
		t = get_curproc();
	}
	else
	{
		t = pid2proc (pid);
		if (!t) return ENOENT;
	}

	if (p->p_cred->ucr->euid
	    && (t->p_cred->ruid != p->p_cred->ruid)
	    && (t->ppid != p->pid))
	{
		return EACCES;
	}

	if (newgrp >= 0)
	{
		if (newgrp == 0)
			newgrp = t->pid;

		t->pgrp = newgrp;
		DEBUG(("sys_p_setpgrp: assigned t->pgrp = %i", t->pgrp));
	}

	return t->pgrp;
}

/* tesche: audit user id functions, these id's never change once set to != 0
 * and can therefore be used to determine who the initially logged in user was.
 *
 * XXX what's that???
 */
long _cdecl
sys_p_getauid (void)
{
	PROC *p = get_curproc();

	TRACE (("Pgetauid()"));

	return p->auid;
}
long _cdecl
sys_p_setauid (int id)
{
	PROC *p = get_curproc();

	TRACE (("Psetauid(%i)", id));

	if (p->auid)
		return EACCES;	/* this may only be changed once */

	return (p->auid = id);
}

/*
 * a way to get/set process-specific user information. the user information
 * longword is set to "arg", unless arg is -1. In any case, the old
 * value of the longword is returned.
 */
long _cdecl
sys_p_usrval (long arg)
{
	PROC *p = get_curproc();
	long r;

	TRACE (("Pusrval(%lx)", arg));

	r = p->usrdata;
	if (arg != -1L)
		p->usrdata = arg;

	return r;
}

/*
 * set the file creation mask to "mode". Returns the old value of the
 * mask.
 */
long _cdecl
sys_p_umask (ushort mode)
{
	PROC *p = get_curproc();
	long oldmask = p->p_cwd->cmask;

	p->p_cwd->cmask = mode & (~S_IFMT);
	return oldmask;
}

/*
 * get/set the domain of a process. domain 0 is the default (TOS) domain.
 * domain 1 is the MiNT domain. for now, domain affects read/write system
 * calls and filename translation.
 */
long _cdecl
sys_p_domain (int arg)
{
	PROC *p = get_curproc();
	long r;
	TRACE (("Pdomain(%i)", arg));

	r = p->domain;
	if (arg >= 0)
		p->domain = arg ? DOM_MINT : DOM_TOS;

	return r;
}

/*
 * Ppause: just sleeps on IO_Q, with wait_cond == -1. only a signal will
 * wake us up
 */
long _cdecl
sys_p_pause (void)
{
	TRACE (("Pause"));
	sleep (IO_Q, -1L);
	return E_OK;
}

/*
 * helper function for t_alarm: this will be called when the timer goes
 * off, and raises SIGALRM
 */
static void _cdecl
alarmme (PROC *p, long arg)
{
	p->alarmtim = 0;
	post_sig (p, SIGALRM);
}

/*
 * Talarm(x): set the alarm clock to go off in "x" seconds. returns the
 * old value of the alarm clock
 */
long _cdecl
sys_t_alarm (long x)
{
	register long oldalarm;

	oldalarm = sys_t_malarm (x * 1000);
	oldalarm = (oldalarm + 999) / 1000;	/* convert to seconds */

	return oldalarm;
}

/*
 * Tmalarm(x): set the alarm clock to go off in "x" milliseconds. returns
 * the old value ofthe alarm clock
 */
long _cdecl
sys_t_malarm (long x)
{
	PROC *p = get_curproc();
	long oldalarm;
	TIMEOUT *t;

	/* see how many milliseconds there were to the alarm timeout */
	oldalarm = 0;

	if (p->alarmtim)
	{
		ushort sr;

		sr = splhigh ();

		for (t = tlist; t; t = t->next)
		{
			oldalarm += t->when;
			if (t == p->alarmtim)
				goto foundalarm;
		}
		DEBUG (("Talarm: old alarm not found!"));
		oldalarm = 0;
		p->alarmtim = 0;

foundalarm:
		spl (sr);
	}

	/* we were just querying the alarm */
	if (x < 0)
		return oldalarm;

	/* cancel old alarm */
	if (p->alarmtim)
		canceltimeout (p->alarmtim);

	/* add a new alarm, to occur in x milliseconds */
	if (x)
		p->alarmtim = addtimeout (p, x, alarmme);
	else
		p->alarmtim = 0;

	return oldalarm;
}

# define ITIMER_REAL 0
# define ITIMER_VIRTUAL 1
# define ITIMER_PROF 2

/*
 * helper function for t_setitimer: this will be called when the ITIMER_REAL
 * timer goes off
 */
static void _cdecl
itimer_real_me (PROC *p, long arg)
{
	if (p->itimer[ITIMER_REAL].interval)
		p->itimer[ITIMER_REAL].timeout = addtimeout (p, p->itimer[ITIMER_REAL].interval, itimer_real_me);
	else
		p->itimer[ITIMER_REAL].timeout = 0;

	post_sig (p, SIGALRM);
}

/*
 * helper function for t_setitimer: this will be called when the ITIMER_VIRTUAL
 * timer goes off
 */
static void _cdecl
itimer_virtual_me (PROC *p, long arg)
{
	long timeleft;

	timeleft = p->itimer[ITIMER_VIRTUAL].reqtime
			- (p->usrtime - p->itimer[ITIMER_VIRTUAL].startusrtime);
	if (timeleft > 0)
	{
		p->itimer[ITIMER_VIRTUAL].timeout = addtimeout (p, timeleft, itimer_virtual_me);
	}
	else
	{
		timeleft = p->itimer[ITIMER_VIRTUAL].interval;
		if (timeleft == 0)
		{
			p->itimer[ITIMER_VIRTUAL].timeout = 0;
		}
		else
		{
			p->itimer[ITIMER_VIRTUAL].reqtime = timeleft;
			p->itimer[ITIMER_VIRTUAL].startsystime = p->systime;
			p->itimer[ITIMER_VIRTUAL].startusrtime = p->usrtime;
			p->itimer[ITIMER_VIRTUAL].timeout = addtimeout (p, timeleft, itimer_virtual_me);
		}
		post_sig (p, SIGVTALRM);
	}
}

/*
 * helper function for t_setitimer: this will be called when the ITIMER_PROF
 * timer goes off
 */
static void _cdecl
itimer_prof_me (PROC *p, long arg)
{
	long timeleft;

	timeleft = p->itimer[ITIMER_PROF].reqtime
			- (p->usrtime - p->itimer[ITIMER_PROF].startusrtime);
	if (timeleft > 0)
	{
		p->itimer[ITIMER_PROF].timeout = addtimeout (p, timeleft, itimer_prof_me);
	}
	else
	{
		timeleft = p->itimer[ITIMER_PROF].interval;
		if (timeleft == 0)
		{
			p->itimer[ITIMER_PROF].timeout = 0;
		}
		else
		{
			p->itimer[ITIMER_PROF].reqtime = timeleft;
			p->itimer[ITIMER_PROF].startsystime = p->systime;
			p->itimer[ITIMER_PROF].startusrtime = p->usrtime;
			p->itimer[ITIMER_PROF].timeout = addtimeout (p, timeleft, itimer_prof_me);
		}
		post_sig (p, SIGPROF);
	}
}

/*
 * Tsetitimer(which, interval, value, ointerval, ovalue):
 * schedule an interval timer
 * which is ITIMER_REAL (0) for SIGALRM, ITIMER_VIRTUAL (1) for SIGVTALRM,
 * or ITIMER_PROF (2) for SIGPROF.
 * the rest of the parameters are pointers to millisecond values.
 * interval is the value to which the timer will be reset
 * value is the current timer value
 * ointerval and ovalue are the previous values
 */
long _cdecl
sys_t_setitimer (int which, long *interval, long *value, long *ointerval, long *ovalue)
{
	PROC *p = get_curproc();
	long oldtimer;
	TIMEOUT *t;
	void _cdecl (*handler)(PROC *p, long arg) = 0;
	long tmpold;

	if ((which != ITIMER_REAL)
		&& (which != ITIMER_VIRTUAL)
		&& (which != ITIMER_PROF))
	{
			return ENOSYS;
	}

	/* ensure that any addresses specified by the calling process
	 * are in that process's address space
	 */
	if ((interval && (!(valid_address((long) interval))))
		|| (value && (!(valid_address((long) value))))
		|| (ointerval && (!(valid_address((long) ointerval))))
		|| (ovalue && (!(valid_address((long) ovalue)))))
	{
			return EFAULT;
	}

	/* see how many milliseconds there were to the timeout */
	oldtimer = 0;

	if (p->itimer[which].timeout)
	{
		ushort sr;

		sr = splhigh ();

		for (t = tlist; t; t = t->next)
		{
			oldtimer += t->when;
			if (t == p->itimer[which].timeout)
				goto foundtimer;
		}
		DEBUG (("Tsetitimer: old timer not found!"));
		oldtimer = 0;
foundtimer:
		spl (sr);
	}

	if (ointerval)
		*ointerval = p->itimer[which].interval;

	if (ovalue)
	{
		if (which == ITIMER_REAL)
		{
			*ovalue = oldtimer;
		}
		else
		{
			tmpold = p->itimer[which].reqtime
				- (p->usrtime - p->itimer[which].startusrtime);

			if (which == ITIMER_PROF)
				tmpold -= (get_curproc()->systime - p->itimer[which].startsystime);

			if (tmpold <= 0)
				tmpold = 0;

			*ovalue = tmpold;
		}
	}

	if (interval)
		p->itimer[which].interval = MAX (*interval, 10);

	if (value)
	{
		/* cancel old timer */
		if (p->itimer[which].timeout)
			canceltimeout (get_curproc()->itimer[which].timeout);

		p->itimer[which].timeout = NULL;

		/* add a new timer, to occur in x milliseconds */
		if (*value)
		{
			p->itimer[which].reqtime = MAX (*value, 10);
			p->itimer[which].startsystime = get_curproc()->systime;
			p->itimer[which].startusrtime = get_curproc()->usrtime;

			switch (which)
			{
				case ITIMER_REAL:
					handler = itimer_real_me;
					break;
				case ITIMER_VIRTUAL:
					handler = itimer_virtual_me;
					break;
				case ITIMER_PROF:
					handler = itimer_prof_me;
					break;
				default:
					break;
			}
			p->itimer[which].timeout = addtimeout (p, MAX (*value, 10), handler);
		}
		else
			p->itimer[which].timeout = 0;
	}

	return 0;
}

/*
 * Sysconf(which): returns information about system configuration.
 * "which" specifies which aspect of the system configuration is to
 * be returned:
 *	-1	max. value of "which" allowed
 *	 0	max. number of memory regions per proc	{MEMR_MAX}
 *	 1	max. length of Pexec() execution string {ARG_MAX}
 *	 2	max. number of open files per process	{OPEN_MAX}
 *	 3	number of supplementary group id's	{NGROUPS_MAX}
 *	 4	max. number of processes per uid	{CHILD_MAX}
 *	 5	HZ					{CLK_TCK}
 *	 6	pagesize				{PAGE_SIZE}
 *	 7	phys pages				{PHYS_PAGES}
 *	 8      passwd buffer size			{GETPW_R_SIZE}
 *	 9      group buffer size			{GETGR_R_SIZE}
 *
 * unlimited values (e.g. CHILD_MAX) are returned as 0x7fffffffL
 *
 * See also Dpathconf() in dosdir.c.
 */

#define PAGESIZE 8192 /* header file ?? */

long _cdecl
sys_s_ysconf (int which)
{
	switch (which)
	{
		case -1:	return 9;
		case  0:	return UNLIMITED;
		case  1:	return 32767; /* matches ARG_MAX */
		case  2:	return NDFILE;
		case  3:	return NGROUPS_MAX;
		case  4:	return UNLIMITED;
		case  5:	return HZ;
		case  6:	return PAGESIZE;
		case  7:	return freephysmem() / PAGESIZE;
		case  8:	return 1024;
		case  9:	return 1024;
		default:	return ENOSYS;
	}
}

/*
 * Salert: send an ALERT message to the user, via the same mechanism
 * the kernel does (i.e. u:\pipe\alert, if it's available
 */
long _cdecl
sys_s_alert (char *str)
{
	/* how's this for confusing code? _ALERT tries to format the
	 * string as an alert box; if it fails, we let the full-fledged
	 * ALERT function (which will try _ALERT, and fail again)
	 * print the alert to the debugging device
	 */
	if (_ALERT (str) == 0)
		ALERT (str);

	return E_OK;
}

/*
 * Suptime: get time in seconds since boot and current load averages from
 * kernel.
 */
long _cdecl
sys_s_uptime (ulong *cur_uptime, ulong loadaverage[3])
{
	*cur_uptime = uptime;

	loadaverage[0] = avenrun[0];
	loadaverage[1] = avenrun[1];
	loadaverage[2] = avenrun[2];

	return E_OK;
}

#define GEM 0x47454D00L	/* "GEM" */
/*
 * shut down processes; this involves waking them all up, and sending
 * them SIGTERM to give them a chance to clean up after themselves.
 */
static void
shutdown(void)
{
	struct proc *p;
	int posts = 0;
	int i;
	union { long l; char *s; } ls;

	DEBUG(("shutdown() entered"));
	assert(get_curproc()->p_sigacts);

	/* Ignore signals, that could terminate this process */
	SIGACTION(get_curproc(), SIGCHLD).sa_handler = SIG_IGN;
	SIGACTION(get_curproc(), SIGTERM).sa_handler = SIG_IGN;
	SIGACTION(get_curproc(), SIGABRT).sa_handler = SIG_IGN;
	SIGACTION(get_curproc(), SIGQUIT).sa_handler = SIG_IGN;
	SIGACTION(get_curproc(), SIGHUP).sa_handler = SIG_IGN;

	for (p = proclist; p; p = p->gl_next)
	{
		ls.s = p->name;
		FORCE("p->name=%s:%lx pgrp=%d", p->name, ls.l, p->pgrp);
		/* Skip MiNT, curproc and AES, and GEM */
		if (p->pgrp && (p != get_curproc()) && ((p->p_mem->memflags & F_OS_SPECIAL) == 0) && ls.l != GEM )
		{
			if (p->wait_q != ZOMBIE_Q && p->wait_q != TSR_Q)
			{
				if (p->wait_q && p->wait_q != READY_Q)
				{
					ushort sr = spl7();
					rm_q(p->wait_q, p);
					add_q(READY_Q, p);
					spl(sr);
				}

				FORCE("SIGTERM -> %s (pid %i)", p->name, p->pid);
				post_sig(p, SIGTERM);

				posts++;
			}
		}
	}

	while (posts--)
		for (i = 0; i < 16; i++)	/* sleep */
			sys_s_yield();

	sysq[READY_Q].head = sysq[READY_Q].tail = NULL;

	FORCE("Close open files ...");
	close_filesys();
	FORCE("done");

	FORCE("Syncing file systems ...");
	sys_s_ync();
	FORCE("done");

	for (i = 0; i < NUM_DRIVES; i++)
	{
		FILESYS *fs = drives[i];

		if (fs)
		{
			if (fs->fsflags & FS_EXT_1)
			{
				FORCE("Unmounting %c: ...", 'A'+i);
				xfs_unmount(fs, i);
			}
			else
			{
				DEBUG(("Invalidate %c: ...", 'A'+i));
				xfs_dskchng(fs, i, 1);
			}
		}
	}

	FORCE("Syncing file systems ...");
	sys_s_ync();
	FORCE("done");

	/* Wait for the disks to flush their write cache */
#ifdef WITH_NATIVE_FEATURES
	if (machine != machine_emulator)
#endif
		delay_seconds(2);
}

/*
 * where restart is:
 *
 * SHUT_POWER (0) = halt/power off
 * SHUT_BOOT  (1) = warm start
 * SHUT_COLD  (2) = cold start,
 * SHUT_HALT  (3) = halt
 *
 */
long _cdecl
sys_s_hutdown(long restart)
{
	static short shutting_down = 0;
	PROC *p = get_curproc();

	if( shutting_down == 1 )
	{
		FORCE( "sys_s_hutdown reentered!");
		return EPERM;
	}
	shutting_down = 1;
	DEBUG(("sys_s_hutdown: %ld", restart));
	/* only root may shut down the system */
	if (suser(p->p_cred->ucr) || (p->p_cred->ruid == 0))
	{
		shutdown();

		switch (restart)
		{
			case  SHUT_POWER:
			{
				hw_poweroff();
				/* fall through */
			}
			case  SHUT_HALT:
			{
				DEBUG(("Halting system ..."));
				hw_halt();
			}
			case  SHUT_COLD:
			{
				hw_coldboot();
			}
			case  SHUT_BOOT:
			default:
			{
				DEBUG(("Rebooting ..."));
				hw_warmboot();
			}
		}

		/* not reached */
	}
	shutting_down = 0;

	return EPERM;
}
