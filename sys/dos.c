/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/* miscellaneous DOS functions */

# include "dos.h"
# include "global.h"

# include "mint/asm.h"
# include "mint/signal.h"

# include "arch/intr.h"

# include "dosfile.h"
# include "filesys.h"
# include "memory.h"
# include "proc.h"
# include "signal.h"
# include "timeout.h"
# include "util.h"


long _cdecl 
s_version (void)
{
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
s_uper (long new_ssp)
{
	register int in_super;
	register long r;
	
	/* Inappropriate callers will be killed. This is
	 * lotsa safer when you set no memory protection.
	 */
	if ((secure_mode > 1) && (curproc->euid != 0))
	{
		DEBUG (("Super() by non privileged process!"));
		raise (SIGSYS);
		return new_ssp;
	}
	
	TRACE (("Super"));
	in_super = curproc->ctxt[SYSCALL].sr & 0x2000;
	
	if (new_ssp == 1)
	{
		r = in_super ? -1L : 0;
	}
	else
	{
		curproc->ctxt[SYSCALL].sr ^= 0x2000;
		r = curproc->ctxt[SYSCALL].ssp;
		if (in_super)
		{
			if (new_ssp == 0)
			{
				DEBUG (("bad Super call"));
				raise (SIGSYS);
			}
			else
			{
				curproc->ctxt[SYSCALL].usp = curproc->ctxt[SYSCALL].ssp;
				curproc->ctxt[SYSCALL].ssp = new_ssp;
			}
		}
		else
		{
			curproc->ctxt[SYSCALL].ssp = new_ssp ?
				new_ssp : curproc->ctxt[SYSCALL].usp;
		}
	}
	
	return r;
}

/*
 * GEMDOS extension: Syield(): give up the processor if any other
 * processes are waiting. Always returns 0.
 */

long _cdecl
s_yield (void)
{
	/* reward the nice process */
	curproc->curpri = curproc->pri;
	sleep (READY_Q, curproc->wait_cond);
	
	return E_OK;
}

/*
 * GEMDOS extensions: routines for getting/setting process i.d.'s and
 * user i.d.'s
 */

long _cdecl p_getpid (void) { return curproc->pid; }
long _cdecl p_getppid (void) { return curproc->ppid; }
long _cdecl p_getpgrp (void) { return curproc->pgrp; }

/*
 * note: Psetpgrp(0, ...) is equivalent to Psetpgrp(Pgetpid(), ...)
 * also note: Psetpgrp(x, 0) is equivalent to Psetpgrp(x, x)
 */

long _cdecl
p_setpgrp (int pid, int newgrp)
{
	PROC *p;

	if (pid == 0)
	{
		p = curproc;
	}
	else
	{
		p = pid2proc (pid);
		if (p == 0) return ENOENT;
	}
	
	if (curproc->euid
		&& (p->ruid != curproc->ruid)
		&& (p->ppid != curproc->pid))
	{
		return EACCES;
	}

	if (newgrp < 0)
		return p->pgrp;

	if (newgrp == 0)
		newgrp = p->pid;
	
	return (p->pgrp = newgrp);
}

long _cdecl p_getuid (void) { return curproc->ruid; }
long _cdecl p_getgid (void) { return curproc->rgid; }
long _cdecl p_geteuid (void) { return curproc->euid; }
long _cdecl p_getegid (void) { return curproc->egid; }

long _cdecl
p_setuid (int uid)
{
	if (curproc->euid == 0)
		curproc->ruid = curproc->euid = curproc->suid = uid;
	else if ((uid == curproc->ruid) || (uid == curproc->suid))
		curproc->euid = uid;
	else
		return EACCES;

	return uid;
}

long _cdecl
p_setgid (int gid)
{
	if (curproc->euid == 0)
		curproc->rgid = curproc->egid = curproc->sgid = gid;
	else if ((gid == curproc->rgid) || (gid == curproc->sgid))
		curproc->egid = gid;
	else
		return EACCES;

	return gid;
}

/* uk, blank: set effective uid/gid but leave the real uid/gid unchanged. */
long _cdecl
p_setreuid (int ruid, int euid)
{
	int old_ruid = curproc->ruid;

	if (ruid != -1)
	{
		if (curproc->euid == ruid || old_ruid == ruid || curproc->euid == 0)
			curproc->ruid = ruid;
		else
			return EACCES;
	}

	if (euid != -1)
	{
		if (curproc->euid == euid || old_ruid == euid || curproc->suid == euid || curproc->euid == 0)
			curproc->euid = euid;
		else
		{
			curproc->ruid = old_ruid;
			return EACCES;
		}
	}

	if (ruid != -1 || (euid != -1 && euid != old_ruid))
		curproc->suid = curproc->euid;

	return E_OK;
}
	
long _cdecl
p_setregid (int rgid, int egid)
{
	int old_rgid = curproc->rgid;

	if (rgid != -1)
	{
		if ((curproc->egid == rgid) || (old_rgid == rgid) || (curproc->euid == 0))
			curproc->rgid = rgid;
		else
			return EACCES;
	}

	if (egid != -1)
	{
		if ((curproc->egid == egid) || (old_rgid == egid) || (curproc->sgid == egid) || (curproc->euid == 0))
			curproc->egid = egid;
		else
		{
			curproc->rgid = old_rgid;
			return EACCES;
		}
	}

	if (rgid != -1 || (egid != -1 && egid != old_rgid))
		curproc->sgid = curproc->egid;

	return E_OK;
}

long _cdecl
p_seteuid(int euid)
{
	if (!p_setreuid (-1, euid))
		return euid;

	return EACCES;
}
	
long _cdecl
p_setegid(int egid)
{
	if (!p_setregid (-1, egid))
		return egid;

	return EACCES;
}

/* tesche: audit user id functions, these id's never change once set to != 0
 * and can therefore be used to determine who the initially logged in user was.
 */
long _cdecl
p_getauid (void)
{
	return curproc->auid;
}

long _cdecl
p_setauid (int id)
{
	if (curproc->auid)
		return EACCES;	/* this may only be changed once */

	return (curproc->auid = id);
}

/* tesche: get/set supplemantary group id's.
 */
long _cdecl
p_getgroups (int gidsetlen, int gidset[])
{
	int i;

	if (gidsetlen == 0)
		return curproc->ngroups;

	if (gidsetlen < curproc->ngroups)
		return EBADARG;

	for (i=0; i<curproc->ngroups; i++)
		gidset[i] = curproc->ngroup[i];

	return curproc->ngroups;
}

long _cdecl
p_setgroups (int ngroups, int gidset[])
{
	int i;

	if (curproc->euid)
		return EACCES;	/* only superuser may change this */

	if ((ngroups < 0) || (ngroups > NGROUPS_MAX))
		return EBADARG;

	curproc->ngroups = ngroups;
	for (i=0; i<ngroups; i++)
		curproc->ngroup[i] = gidset[i];

	return ngroups;
}

/*
 * a way to get/set process-specific user information. the user information
 * longword is set to "arg", unless arg is -1. In any case, the old
 * value of the longword is returned.
 */

long _cdecl
p_usrval (long arg)
{
	long r;

	TRACE (("Pusrval"));
	
	r = curproc->usrdata;
	if (arg != -1L)
		curproc->usrdata = arg;
	
	return r;
}

/*
 * set the file creation mask to "mode". Returns the old value of the
 * mask.
 */
long _cdecl
p_umask (unsigned mode)
{
	long oldmask = curproc->umask;

	curproc->umask = mode & (~S_IFMT);
	return oldmask;
}

/*
 * get/set the domain of a process. domain 0 is the default (TOS) domain.
 * domain 1 is the MiNT domain. for now, domain affects read/write system
 * calls and filename translation.
 */

long _cdecl
p_domain (int arg)
{
	long r;
	TRACE (("Pdomain(%d)", arg));
	
	r = curproc->domain;
	if (arg >= 0)
		curproc->domain = arg ? 1 : 0;
	
	return r;
}

/*
 * get process resource usage. 8 longwords are returned, as follows:
 *     r[0] == system time used by process
 *     r[1] == user time used by process
 *     r[2] == system time used by process' children
 *     r[3] == user time used by process' children
 *     r[4] == memory used by process
 *     r[5] - r[7]: reserved for future use
 */

long _cdecl
p_rusage (long *r)
{
	r[0] = curproc->systime;
	r[1] = curproc->usrtime;
	r[2] = curproc->chldstime;
	r[3] = curproc->chldutime;
	r[4] = memused (curproc);
	/* r[5] = ; */
	/* r[6] = ; */
	/* r[7] = ; */
	
	return E_OK;
}

/*
 * get/set resource limits i to value v. The old limit is always returned;
 * if v == -1, the limit is unchanged, otherwise it is set to v. Possible
 * values for i are:
 *    1:  max. cpu time	(milliseconds)
 *    2:  max. core memory allowed
 *    3:  max. amount of malloc'd memory allowed
 */
long _cdecl
p_setlimit (int i, long v)
{
	long oldlimit;

	switch(i)
	{
		case 1:
		{
			oldlimit = curproc->maxcpu;
			if (v >= 0) curproc->maxcpu = v;
			break;
		}
		case 2:
		{
			oldlimit = curproc->maxcore;
			if (v >= 0)
			{
				curproc->maxcore = v;
				recalc_maxmem (curproc);
			}
			break;
		}
		case 3:
		{
			oldlimit = curproc->maxdata;
			if (v >= 0)
			{
				curproc->maxdata = v;
				recalc_maxmem (curproc);
			}
			break;
		}
		default:
		{
			DEBUG (("Psetlimit: invalid mode %d", i));
			return ENOSYS;
		}
	}
	
	TRACE (("p_setlimit(%d, %ld): oldlimit = %ld", i, v, oldlimit));
	return oldlimit;
}

/*
 * Ppause: just sleeps on IO_Q, with wait_cond == -1. only a signal will
 * wake us up
 */

long _cdecl
p_pause (void)
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
alarmme (PROC *p)
{
	p->alarmtim = 0;
	post_sig (p, SIGALRM);
}

/*
 * Talarm(x): set the alarm clock to go off in "x" seconds. returns the
 * old value of the alarm clock
 */

long _cdecl
t_alarm (long x)
{
	register long oldalarm;
	
	oldalarm = t_malarm (x * 1000);
	oldalarm = (oldalarm + 999) / 1000;	/* convert to seconds */
	
	return oldalarm;
}

/*
 * Tmalarm(x): set the alarm clock to go off in "x" milliseconds. returns
 * the old value ofthe alarm clock
 */

long _cdecl
t_malarm (long x)
{
	long oldalarm;
	TIMEOUT *t;

	/* see how many milliseconds there were to the alarm timeout */
	oldalarm = 0;

	if (curproc->alarmtim)
	{
		for (t = tlist; t; t = t->next)
		{
			oldalarm += t->when;
			if (t == curproc->alarmtim)
				goto foundalarm;
		}
		DEBUG (("Talarm: old alarm not found!"));
		oldalarm = 0;
		curproc->alarmtim = 0;
foundalarm:
		;
	}

	/* we were just querying the alarm */
	if (x < 0)
		return oldalarm;

	/* cancel old alarm */
	if (curproc->alarmtim)
		canceltimeout (curproc->alarmtim);

	/* add a new alarm, to occur in x milliseconds */
	if (x)
		curproc->alarmtim = addtimeout (x, alarmme);
	else
		curproc->alarmtim = 0;

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
itimer_real_me (PROC *p)
{
	PROC *real_curproc;

	real_curproc = curproc;
	curproc = p;
	if (p->itimer[ITIMER_REAL].interval)
		p->itimer[ITIMER_REAL].timeout = addtimeout (p->itimer[ITIMER_REAL].interval, itimer_real_me);
	else
		p->itimer[ITIMER_REAL].timeout = 0;

	curproc = real_curproc;
	post_sig (p, SIGALRM);
}

/*
 * helper function for t_setitimer: this will be called when the ITIMER_VIRTUAL
 * timer goes off
 */

static void _cdecl
itimer_virtual_me (PROC *p)
{
	PROC *real_curproc;
	long timeleft;

	real_curproc = curproc;
	curproc = p;
	timeleft = p->itimer[ITIMER_VIRTUAL].reqtime
			- (p->usrtime - p->itimer[ITIMER_VIRTUAL].startusrtime);
	if (timeleft > 0)
	{
		p->itimer[ITIMER_VIRTUAL].timeout = addtimeout (timeleft, itimer_virtual_me);
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
			p->itimer[ITIMER_VIRTUAL].timeout = addtimeout (timeleft, itimer_virtual_me);
		}
		post_sig (p, SIGVTALRM);
	}
	curproc = real_curproc;
}

/*
 * helper function for t_setitimer: this will be called when the ITIMER_PROF
 * timer goes off
 */

static void _cdecl
itimer_prof_me (PROC *p)
{
	PROC *real_curproc;
	long timeleft;

	real_curproc = curproc;
	curproc = p;
	timeleft = p->itimer[ITIMER_PROF].reqtime
			- (p->usrtime - p->itimer[ITIMER_PROF].startusrtime);
	if (timeleft > 0)
	{
		p->itimer[ITIMER_PROF].timeout = addtimeout (timeleft, itimer_prof_me);
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
			p->itimer[ITIMER_PROF].timeout = addtimeout (timeleft, itimer_prof_me);
		}
		post_sig (p, SIGPROF);
	}
	curproc = real_curproc;
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
t_setitimer (int which, long *interval, long *value, long *ointerval, long *ovalue)
{
	long oldtimer;
	TIMEOUT *t;
	void _cdecl (*handler)() = 0;
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

	if (curproc->itimer[which].timeout)
	{
		for (t = tlist; t; t = t->next)
		{
			oldtimer += t->when;
			if (t == curproc->itimer[which].timeout)
				goto foundtimer;
		}
		DEBUG (("Tsetitimer: old timer not found!"));
		oldtimer = 0;
foundtimer:
		;
	}

	if (ointerval)
		*ointerval = curproc->itimer[which].interval;
	if (ovalue)
	{
		if (which == ITIMER_REAL)
		{
			*ovalue = oldtimer;
		}
		else
		{
			tmpold = curproc->itimer[which].reqtime
				- (curproc->usrtime - curproc->itimer[which].startusrtime);
			
			if (which == ITIMER_PROF)
				tmpold -= (curproc->systime - curproc->itimer[which].startsystime);
			
			if (tmpold <= 0)
				tmpold = 0;
			
			*ovalue = tmpold;
		}
	}
	
	if (interval)
		curproc->itimer[which].interval = *interval;
	
	if (value)
	{
		/* cancel old timer */
		if (curproc->itimer[which].timeout)
			canceltimeout(curproc->itimer[which].timeout);
		
		curproc->itimer[which].timeout = 0;

		/* add a new timer, to occur in x milliseconds */
		if (*value)
		{
			curproc->itimer[which].reqtime = *value;
			curproc->itimer[which].startsystime = curproc->systime;
			curproc->itimer[which].startusrtime = curproc->usrtime;
			
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
			curproc->itimer[which].timeout = addtimeout (*value, handler);
		}
		else
			curproc->itimer[which].timeout = 0;
	}
	return 0;
}

/*
 * Sysconf(which): returns information about system configuration.
 * "which" specifies which aspect of the system configuration is to
 * be returned:
 *	-1	max. value of "which" allowed
 *	 0	max. number of memory regions per proc
 *	 1	max. length of Pexec() execution string {ARG_MAX}
 *	 2	max. number of open files per process	{OPEN_MAX}
 *	 3	number of supplementary group id's	{NGROUPS_MAX}
 *	 4	max. number of processes per uid	{CHILD_MAX}
 *
 * unlimited values (e.g. CHILD_MAX) are returned as 0x7fffffffL
 *
 * See also Dpathconf() in dosdir.c.
 */

long _cdecl
s_ysconf (int which)
{
	switch (which)
	{
		case -1:	return 4;
		case  0:	return UNLIMITED;
		case  1:	return 126;
		case  2:	return MAX_OPEN;
		case  3:	return NGROUPS_MAX;
		case  4:	return UNLIMITED;
		default:	return ENOSYS;
	}
}

/*
 * Salert: send an ALERT message to the user, via the same mechanism
 * the kernel does (i.e. u:\pipe\alert, if it's available
 */

long _cdecl
s_alert (char *str)
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
s_uptime (ulong *cur_uptime, ulong loadaverage[3])
{
	*cur_uptime = uptime;
	
	loadaverage[0] = avenrun[0];
	loadaverage[1] = avenrun[1];
	loadaverage[2] = avenrun[2];

	return E_OK;
}

/* uk: shutdown function
 *     if the parameter is nonzero, reboot the machine, otherwise
 *     halt it after syncing the file systems.
 */

/*
 * shut down processes; this involves waking them all up, and sending
 * them SIGTERM to give them a chance to clean up after themselves
 */

static void _cdecl
shutmedown (PROC *p)
{
	wake (WAIT_Q, (long) s_hutdown);
	p->wait_cond = 0;
}

/*
 * This is the code that shuts the system down. It's no longer in s_hutdown(),
 * as it's also called from main.c, where there has been a similar routine,
 * which missed some functionality (mainly invalidating/unmounting of
 * filesystems). Thus, ending the initial process and calling Shutdown() now
 * does exactly the same, and we've also removed some redundant code.
 */
void
shutdown (void)
{
	PROC *p;
	int proc_left = 0;
	int i;
	
	DEBUG (("Shutting processes down..."));
	DEBUG (("This is pid %d", curproc->pid));
	
	/* Ignore signals, that could terminate this process */
	curproc->sighandle[SIGCHLD] = SIG_IGN;
	curproc->sighandle[SIGTERM] = SIG_IGN;
	curproc->sighandle[SIGABRT] = SIG_IGN;
	curproc->sighandle[SIGQUIT] = SIG_IGN;
	curproc->sighandle[SIGHUP] = SIG_IGN;
	
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->pid == 0) continue;
		if (p == curproc) continue;  /* curproc is trapped in this code */
		
		if (p->wait_q != ZOMBIE_Q && p->wait_q != TSR_Q)
		{
			if (p->wait_q != READY_Q)
			{
				short sr = spl7 ();
				rm_q (p->wait_q, p);
				add_q (READY_Q, p);
				spl (sr);
			}
			DEBUG (("Posting SIGTERM for pid %d", p->pid));
			post_sig (p, SIGTERM);
			proc_left++;
		}
	}
	
	if (proc_left)
	{
		/* sleep a little while, to give the other processes
		 * a chance to shut down
		 */
		
		if (addtimeout (1000, shutmedown))
		{
			do {
				DEBUG (("Sleeping..."));
				sleep (WAIT_Q, (long) s_hutdown);
			}
			while (curproc->wait_cond == (long) s_hutdown);
		}
		
		DEBUG (("Killing all processes..."));
		for (p = proclist; p; p = p->gl_next)
		{
			if ((p->pid == 0) || (p == curproc))
				continue;
			
			DEBUG (("Posting SIGKILL for pid %d", p->pid));
			post_sig (p, SIGKILL);
		}
	}
	
	sys_q[READY_Q] = 0;
	
	DEBUG (("Close open files ..."));
	close_filesys ();
	DEBUG (("done"));
	
	DEBUG (("Syncing file systems ..."));
	s_ync ();
	DEBUG (("done"));
	
	for (i = 0; i < NUM_DRIVES; i++)
	{
		FILESYS *fs = drives [i];
		
		if (fs)
		{
			if (fs->fsflags & FS_EXT_1)
			{
				DEBUG (("Unmounting %c: ...", 'A'+i));
				xfs_unmount (fs, i);
			}
			else
			{
				DEBUG (("Invalidate %c: ...", 'A'+i));
				xfs_dskchng (fs, i, 1);
			}
		}
	}
	
	DEBUG (("Syncing file systems ..."));
	s_ync ();
	DEBUG (("done"));
}

long _cdecl
s_hutdown (long restart)
{
	/* The -1 argument returns a longword which indicates
	 * what bits of the `restart' argument are valid
	 * (new as of 1.15.6)
	 */
	if (restart < 0)
		return 0x00000003L;

	/* only root may shut down the system */
	if ((curproc->euid == 0) || (curproc->ruid == 0))
	{
		shutdown ();

		/* 0 = halt, 1 = warm start, 2 = cold start
		 */
		switch (restart)
		{
			case  0:
			{
				DEBUG (("Halting system ..."));
				halt ();		/* does not return */
			}
			case  2:
			{
				/* Invalidate the magic values TOS uses to
				 * detect a warm boot
				 */
				*(long *)0x00000420L = 0L;
				*(long *)0x0000043aL = 0L;
			}
			case  1:
			default:
			{
				DEBUG (("Rebooting ..."));
				reboot ();		/* does not return */
			}
		}
	}
	
	return EPERM;	
}
