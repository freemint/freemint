/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
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
 */

# include "k_exit.h"

# include "libkern/libkern.h"

# include "mint/asm.h"
# include "mint/credentials.h"
# include "mint/signal.h"

# include "arch/mprot.h"
# include "arch/sig_mach.h"

# include "bios.h"
# include "dosdir.h"
# include "filesys.h"
# include "k_exec.h"	/* rts */
# include "k_prot.h"	/* free_cred */
# include "kmemory.h"
# include "memory.h"
# include "proc.h"
# include "proc_help.h"
# include "procfs.h"
# include "rendez.h"
# include "signal.h"
# include "slb.h"
# include "time.h"
# include "timeout.h"
# include "util.h"
# include "xbios.h"


/*
 * terminate a process, with return code "code".
 * If que == ZOMBIE_Q, free all resources attached to the child.
 * If que == TSR_Q, free everything but memory.
 *
 * NOTE: terminate() should be called only when the process is to be
 * "terminated with extreme prejuidice". Most times, p_term or p_termres
 * are the functions to use, since they allow the user to do some cleaning
 * up, etc.
 */

long
terminate(struct proc *pcurproc, short code, short que)
{
	struct proc *p;
	int i, wakemint = 0;

	/* notify proc extensions */
	proc_ext_on_exit(pcurproc, code);

	if (bconbsiz)
		(void) bflush();

	assert (que == ZOMBIE_Q || que == TSR_Q);

	/* Exit all non-closed shared libraries */
	while (slb_close_on_exit (1, 0))
		;

	/* Remove structure if curproc is an SLB */
	remove_slb ();

	if (pcurproc->pid == 0)
		FATAL ("attempt to terminate MiNT");

	/* cancel all pending timeouts for this process */
	cancelalltimeouts();

	/* cancel alarm clock */
	pcurproc->alarmtim = 0;

	/* Free saved commandline */
	if (pcurproc->real_cmdline)
	{
		kfree (pcurproc->real_cmdline);
		pcurproc->real_cmdline = NULL;
	}

	/* free exec cookie */
	release_cookie (&pcurproc->exe);

	/* release all semaphores owned by this process */
	free_semaphores (pcurproc->pid);

	/* make sure that any open files that refer to this process are
	 * closed
	 */
	changedrv (PROC_RDEV_BASE | pcurproc->pid);

	/* release any drives locked by Dlock */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		if (dlockproc[i] == pcurproc)
			dlockproc[i] = NULL;
	}

	free_ext (pcurproc);

	free_fd (pcurproc);

	free_cwd (pcurproc);

	/* attention, this invalidates the MMU table */
	if (que == ZOMBIE_Q)
		free_mem (pcurproc);
	/* else
		 make TSR process non-swappable */

	/* find our parent (if parent not found, then use process 0 as parent
	 * since that process is constantly in a wait loop)
	 */
	p = pid2proc (pcurproc->ppid);
	if (!p)
	{
		TRACE (("terminate: parent not found"));
		p = pid2proc (0);
	}

	/* NOTE: normally just post_sig is sufficient for sending a signal;
	 * but in this particular case, we have to worry about processes
	 * that are blocking all signals because they Vfork'd and are waiting
	 * for us to finish (which is indicated by a wait_cond matching our
	 * PROC structure), and also processes that are ignoring SIGCHLD
	 * but are waiting for us.
	 */
	{
		ushort sr;

		sr = splhigh ();
		if (p->wait_q == WAIT_Q
		    && (p->wait_cond == (long) pcurproc || p->wait_cond == (long) sys_pwaitpid))
		{
			rm_q (WAIT_Q, p);
			add_q (READY_Q, p);

			TRACE (("terminate: waking up parent"));
		}
		spl (sr);
	}

	if (pcurproc->ptracer && pcurproc->ptracer != p)
	{
		/* BUG: should we ensure curproc->ptracer is awake ? */

		/* tell tracing process */
		post_sig (pcurproc->ptracer, SIGCHLD);
	}

	/* inform of process termination */
	post_sig (p, SIGCHLD);

	/* find our children, and orphan them
	 * also, check for processes we were tracing, and
	 * cancel the trace
	 */
	i = pcurproc->pid;
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->ppid == i)
		{
			p->ppid = 0;	/* have the system adopt it */
			if (p->wait_q == ZOMBIE_Q)
				wakemint = 1;	/* we need to wake proc. 0 */
		}

		if (p->ptracer == pcurproc)
		{
			p->ptracer = 0;

			/* `FEATURE': we terminate traced processes when the
			 * tracer terminates. It might plausibly be argued
			 * that it would be better to let them continue,
			 * to let some (new) tracer take them over. On the
			 * other hand, if the tracer terminated normally,
			 * it should have used Fcntl(PTRACESFLAGS) to reset
			 * the trace nicely, so something must be wrong for
			 * us to have reached here.
			 */
			post_sig (p, SIGTERM);	/* arrange for termination */
		}
	}

	if (wakemint || (pcurproc->p_flag & P_FLAG_SLB))
	{
		ushort sr;

		sr = splhigh ();

		p = rootproc;		/* pid 0 */
		if (p->wait_q == WAIT_Q)
		{
			rm_q (WAIT_Q, p);
			add_q (READY_Q, p);
		}

		spl (sr);
	}

	/* this makes sure that our children are inherited by the system;
	 * plus, it may help avoid problems if somehow a signal gets
	 * through to us
	 */
	for (i = 0; i < NSIG; i++)
		SIGACTION(pcurproc, i).sa_handler = SIG_IGN;

	/* finally, reset the time/date stamp for /proc and /sys.  */
	procfs_stmp = xtime64;

	sleep (que, (long)(unsigned)code);

	/* we shouldn't ever get here */
	FATAL ("terminate: sleep woke up when it shouldn't have");
	return 0;
}

long
kernel_pterm(struct proc *p, short code)
{
	long term_vec = p->ctxt[SYSCALL].term_vec;

	TRACE(("Pterm(%d)", code));

	if (term_vec != (long)rts)
	{
		/* call the process termination vector */
		TRACE(("term_vec: user has something to do"));

		/* we handle the termination vector just like Supexec(), by
		 * sending signal 0 to the process. See supexec() in xbios.c
		 * for details. Note that we _always_ want to unwind the
		 * signal stack, and setting bit 1 of curproc->p_sigmask tells
		 * handle_sig to do that -- see signal.c.
		 */
		p->p_sigmask |= 1L;
		sys_b_supexec ((Func)term_vec, 0L, 0L, 0L, 0L, code);
		/*
		 * if we arrive here, continue with the termination...
		 */
	}

	return terminate (p, code, ZOMBIE_Q);
}

/*
 * TOS process termination entry points:
 * p_term terminates the process, freeing its memory
 * p_termres lets the process hang around resident in memory, after
 * shrinking its transient program area to "save" bytes
 *
 * ATTENTION: From within MiNT, use kernel_pterm(), to avoid problems with
 * shared libraries!
 */

long _cdecl
sys_pterm(short code)
{
	/* Exit all non-closed shared libraries */
	for (;;)
	{
		int cont;

		cont = slb_close_on_exit(0, code);
		if (cont == 1)
			return code;

		if (cont == 0)
			break;
	}

	return (kernel_pterm(get_curproc(), code));
}

long _cdecl
sys_pterm0(void)
{
	return sys_pterm(0);
}

long _cdecl
sys_ptermres(long save, short code)
{
	struct proc *p = get_curproc();
	struct memspace *mem = p->p_mem;

	MEMREGION *m;
	int i;


	TRACE(("Ptermres(%ld, %d)", save, code));
	assert (mem);

	m = mem->mem[1];	/* should be the basepage (0 is env.) */
	if (m)
		shrink_region (m, save);

	/* make all of the TSR's private memory globally accessible;
	 * this means that more TSR's will "do the right thing"
	 * without having to have prgflags set.
	 */
	for (i = 0; i < mem->num_reg; i++)
	{
		m = mem->mem[i];
		if (m && m->links == 1)
		{
			/* only the TSR is owner */
			if (get_prot_mode (m) == PROT_P)
				mark_region (m, PROT_G, 0);
		}
	}

	return terminate (p, code, TSR_Q);
}

/*
 * routine for waiting for children to die. Return has the pid of the
 * found child in the high word, and the child's exit code in
 * the low word.
 * If the process was killed by a signal the signal that caused the
 * termination is put into the last seven bits. If no children exist,
 * return "File Not Found".
 *
 * If (nohang & 1) is nonzero, then return a 0 immediately if we have
 * no dead children but some living ones that we still have to wait
 * for. If (nohang & 2) is nonzero, then we return any stopped
 * children; otherwise, only children that have exited or are stopped
 * due to a trace trap are returned.
 * If "rusage" is non-zero and a child is found, put the child's
 * resource usage into it (currently only the user and system time are
 * sent back).
 * The pid argument specifies a set of child processes for which status
 * is requested:
 * 	If pid is equal to -1, status is requested for any child process.
 *
 *	If pid is greater than zero, it specifies the process ID of a
 *	single child process for which status is requested.
 *
 *	If pid is equal to zero, status is requested for any child
 *	process whose process group ID is equal to that of the calling
 *	process.
 *
 *	If pid is less than -1, status is requested for any child process
 *	whose process group ID is equal to the absolute value of pid.
 *
 * Note this call is a real standard crosser... POSIX.1 doesn't have the
 * rusage stuff, BSD doesn't have the pid stuff; both are useful, so why
 * not have it all!
 *
 * NOTE:  There is one important difference to the standard behaviour
 *        of the waitpid function:  If WNOHANG was specified and no
 *        children were found, waitpid should actually return immediately
 *        with no error.  Instead, MiNT will return ENOENT in that
 *        case.  This is probably a bug but we won't change it now
 *        because existing binaries may expect that behavior.  A library
 *        binding should take that into account:  If WNOHANG was specified
 *        and the return value is ENOENT,  don't set errno and return
 *        zero to the process.  If you recompile your `make' program
 *        with that kludge you will never see the
 *
 *        	make: *** wait: file not found.  Stop.
 *
 *        again.
 */
long
pwaitpid(short pid, short nohang, long *rusage, short *retval)
{
	struct proc *p;
	long r;
	int ourpid, ourpgrp;
	int found;


	ourpid = get_curproc()->pid;
	ourpgrp = get_curproc()->pgrp;

	if (ourpid)
		TRACE(("Pwaitpid(%d, %d, %p)", pid, nohang, rusage));

	/* if there are terminated children, clean up and return their info;
	 * if there are children, but still running, wait for them;
	 * if there are no children, return an error
	 */
	do {
		/* look for any children */
		found = 0;
		for (p = proclist; p; p = p->gl_next)
		{
			if ((p->ppid == ourpid || p->ptracer == get_curproc())
			    && (pid == -1
				|| (pid > 0 && pid == p->pid)
# if 0
				|| (pid == 0 && p->pgrp == ourpid)
# else
				|| (pid == 0 && p->pgrp == ourpgrp)
# endif
				|| (pid < -1 && p->pgrp == -pid)))
			{
				found++;
				if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
					break;

				/* p->wait_cond == 0 if a stopped process
				 * has already been waited for
				 */
				if ((p->wait_q == STOP_Q) && p->wait_cond)
				{
					if ((nohang & 2)
# if 0
					    || ((p->wait_cond & 0x1f00) == (SIGTRAP << 8)))
# else
					    || p->ptracer)
# endif
					{
						break;
					}
				}
			}
		}

		if (!p)
		{
			if (found)
			{
				if (nohang & 1)
				{
					if (ourpid)
						TRACE(("Pwaitpid(%d, %d) -> 0 [nohang & 1]", pid, nohang));

					return 0;
				}

				if (get_curproc()->pid)
					TRACE(("Pwaitpid: going to sleep"));

				sleep (WAIT_Q, (long) sys_pwaitpid);
			}
			else
			{
				/* Don't report that for WNOHANG.  */
				if (!(nohang & 1))
					DEBUG(("Pwaitpid: no children found"));

				return ENOENT;
			}
		}
	}
	while (!p);

	/* OK, we've found our child
	 * calculate the return code from the child's exit code and pid and
	 * the possibly fatal signal that caused it to die.
	 */
	r = (((unsigned long)p->pid) << 16) | (p->wait_cond & 0x0000ffffUL);

	if (retval)
		*retval = (unsigned short)r;

	/* This assumes that NSIG is 32, i. e. the highest possible
	   signal is 31.  We have to use 0x9f instead of 0x1f to be
	   prepared for a possible core dump flag.  */
	if (p->signaled)
		r = (r & 0xffff00ff) | ((p->last_sig << 8) & 0x00009f00);
	else if (p->wait_q != STOP_Q && (p->wait_cond & 0x1f00) != SIGTRAP << 8)
		/* Strip down the return code of the child down to 8 bits.
		   Some misbehaving programs exceed this limit which could
		   cause a misinterpretation of the wait status signaled
		   (with possible memory faults if the signal would be
		   > NSIG).  */
		r &= 0xffff00ff;

	/* check resource usage */
	if (rusage)
	{
		*rusage++ = p->usrtime + p->chldutime;
		*rusage = p->systime + p->chldstime;
	}

	/* avoid adding adopted trace processes usage to the foster parent */
	if (get_curproc()->pid == p->ppid)
	{
		/* add child's resource usage to parent's */
		if (p->wait_q == TSR_Q || p->wait_q == ZOMBIE_Q)
		{
			get_curproc()->chldstime += p->systime + p->chldstime;
			get_curproc()->chldutime += p->usrtime + p->chldutime;
		}
	}

	/* if it was stopped, mark it as having been found and again return */
	if (p->wait_q == STOP_Q)
	{
		p->wait_cond = 0;

		TRACE(("Pwaitpid(%d, %d) -> %lx [p->wait_q == STOP_Q]", pid, nohang, r));
		return r;
	}

	/* We have to worry about processes which attach themselves to running
	 * processes which they want to trace. We fix things up so that the
	 * second time the signal gets delivered we will go all the way to the
	 * end of this function.
	 */
 	if (p->ptracer && p->ptracer->pid != p->ppid)
 	{
		if (get_curproc() == p->ptracer)
		{
			/* deliver the signal to the tracing process first */
			TRACE(("Pwaitpid(ptracer): returning status %lx to tracing process", r));

			p->ptracer = NULL;
			if (p->ppid != -1)
				return r;
		}
		else
		{
			/* Hmmm, the real parent got here first */
			TRACE(("Pwaitpid(ptracer): returning status %lx to parent process", r));

			p->ppid = -1;
			return r;
		}
	}

	/* if it was a TSR, mark it as having been found and return */
	if (p->wait_q == TSR_Q)
	{
		p->ppid = -1;

		TRACE(("Pwaitpid(%d, %d) -> %lx [p->wait_q == TSR_Q]", pid, nohang, r));
		return r;
	}

	/* it better have been on the ZOMBIE queue from here on in... */
	assert (p->wait_q == ZOMBIE_Q);
	assert (p != get_curproc());

	/* take the child off both the global and ZOMBIE lists */
	{
		ushort sr = splhigh ();

		rm_q (ZOMBIE_Q, p);

		if (proclist == p)
		{
			proclist = p->gl_next;
			p->gl_next = NULL;
		}
		else
		{
			struct proc *q = proclist;

			while (q && q->gl_next != p)
				q = q->gl_next;

			assert (q);

			q->gl_next = p->gl_next;
			p->gl_next = NULL;
		}

		spl (sr);
	}

	if (--p->p_cred->links == 0)
	{
		free_cred (p->p_cred->ucr);
		kfree (p->p_cred);
	}

	free_sigacts (p);

	/* free the PROC structure */
	kfree (p);

	TRACE(("Pwaitpid(%d, %d) -> %lx [end]", pid, nohang, r));
	return r;
}

long _cdecl
sys_pwaitpid(short pid, short nohang, long *rusage)
{
	return pwaitpid(pid, nohang, rusage, NULL);
}
/*
 * p_wait3: BSD process termination primitive, here to maintain
 * compatibility with existing binaries.
 */
long _cdecl
sys_pwait3(short nohang, long *rusage)
{
	return pwaitpid(-1, nohang, rusage, NULL); // sys_pwaitpid(-1, nohang, rusage);
}

/*
 * p_wait: block until a child has exited, and don't worry about
 * resource stats. this is provided as a convenience, and to maintain
 * compatibility with existing binaries (yes, I'm lazy...). we could
 * make do with Pwaitpid().
 */
long _cdecl
sys_pwait(void)
{
	/* BEWARE:
	 * POSIX says that wait() should be implemented as
	 * Pwaitpid(-1, 0, (long *)0). Pwait is really not
	 * useful for much at all, but we'll keep it around
	 * for a while (with it's old, crufty semantics)
	 * for backwards compatibility. People implementing
	 * POSIX style libraries should use Pwaitpid even
	 * to implement wait().
	 */
	return pwaitpid(-1, 2, NULL, NULL); //sys_pwaitpid(-1, 2, NULL);
}
