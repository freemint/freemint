/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * begin:	2000-11-07
 * last change:	2000-11-07
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
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
# include "dosfile.h"
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
terminate (PROC *curproc, int code, int que)
{
	PROC *p;
	int  i, wakemint = 0;
	DIR *dirh, *nexth;
	ushort sr;
	FILEPTR *fp;
	
	
	if (bconbsiz)
		(void) bflush();
	
	assert (que == ZOMBIE_Q || que == TSR_Q);
	
	/* Exit all non-closed shared libraries */
	while (slb_close_on_exit(1))
		;
	
	/* Remove structure if curproc is an SLB */
	remove_slb();
	
	if (curproc->pid == 0)
		FATAL("attempt to terminate MiNT");
	
	/* cancel all user-specified interrupt signals */
	cancelsigintrs();
	
	/* cancel all pending timeouts for this process */
	cancelalltimeouts();
	
	/* cancel alarm clock */
	curproc->alarmtim = 0;
	
	/* release any drives locked by Dlock */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		if (dlockproc[i] == curproc)
			dlockproc[i] = NULL;
	}
	
# if 1
	/* release the controlling terminal,
	 * if we're the last member of this pgroup
	 */
	fp = curproc->p_fd->control;
	if (fp && is_terminal(fp))
	{
		struct tty *tty = (struct tty *)fp->devinfo;
		int pgrp = curproc->pgrp;

		if (pgrp == tty->pgrp)
		{
			PROC *p;
			FILEPTR *pfp;

			if (tty->use_cnt > 1)
			{
				for (p = proclist; p; p = p->gl_next)
				{
					if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
						continue;
					
					if (p->pgrp == pgrp
						&& p != curproc
						&& (0 != (pfp = p->p_fd->control))
						&& pfp->fc.index == fp->fc.index
						&& pfp->fc.dev == fp->fc.dev)
					{
						goto found;
					}
				}
			}
			else
			{
				for (p = proclist; p; p = p->gl_next)
				{
					if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
						continue;
					
					if (p->pgrp == pgrp
						&& p != curproc
						&& p->p_fd->control == fp)
					{
						goto found;
					}
				}
			}
			tty->pgrp = 0;
		}
found:
		;
	}

	/* close all files */
	for (i = MIN_HANDLE; i < curproc->p_fd->nfiles; i++)
	{
		if ((fp = curproc->p_fd->ofiles[i]) != 0)
			do_close(fp);
		curproc->p_fd->ofiles[i] = 0;
	}
# else
	free_fd (curproc);
# endif
	
	/* close any unresolved Fsfirst/Fsnext directory searches */
	for (i = 0; i < NUM_SEARCH; i++)
	{
		if (curproc->srchdta[i])
		{
			DIR *dirh = &curproc->srchdir[i];
			xfs_closedir (dirh->fc.fs, dirh);
			release_cookie(&dirh->fc);
			dirh->fc.fs = 0;
		}
	}
	
	/* close pending opendir/readdir searches */
	for (dirh = curproc->searches; dirh; )
	{
		if (dirh->fc.fs)
		{
			xfs_closedir (dirh->fc.fs, dirh);
			release_cookie(&dirh->fc);
		}
		nexth = dirh->next;
		kfree (dirh);
		dirh = nexth;
	}
	
# if 1
	/* release the directory cookies held by the process */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		release_cookie(&curproc->p_cwd->curdir[i]);
		curproc->p_cwd->curdir[i].fs = 0;
		release_cookie(&curproc->p_cwd->root[i]);
		curproc->p_cwd->root[i].fs = 0;
	}
	
	if (curproc->p_cwd->root_dir)
	{
		DEBUG (("free root_dir = %s", curproc->p_cwd->root_dir));
		
		release_cookie (&curproc->p_cwd->rootdir);
		curproc->p_cwd->rootdir.fs = 0;
		kfree (curproc->p_cwd->root_dir);
		curproc->p_cwd->root_dir = NULL;
	}
# else
	free_cwd (curproc);
# endif
	
	/* make sure that any open files that refer to this process are
	 * closed
	 */
	changedrv (PROC_RDEV_BASE | curproc->pid);
	
	/* Free saved commandline */
	if (curproc->real_cmdline)
	{
		kfree (curproc->real_cmdline);
		curproc->real_cmdline = NULL;
	}
	
	/* free exec cookie */
	release_cookie (&curproc->exe);
	
	/* release all semaphores owned by this process */
	free_semaphores (curproc->pid);
	
	if (que == ZOMBIE_Q)
	{
# if 0
		free_mem (curproc);
# else
		MEMREGION **hold_mem;
		virtaddr *hold_addr;
		
		for (i = curproc->p_mem->num_reg - 1; i >= 0; i--)
		{
			MEMREGION *m = curproc->p_mem->mem[i];
			
			curproc->p_mem->mem[i] = 0;
			curproc->p_mem->addr[i] = 0;
			
			if (m)
			{
				/* don't free specially allocated memory */
				if ((m->mflags & M_KEEP) && (m->links <= 1))
					if (curproc != rootproc)
						attach_region(rootproc, m);
				
				m->links--;
				if (m->links == 0)
					free_region(m);
			}
		}

# if 0
		/*
		 * mark the mem & addr arrays as void so the memory
		 * protection code won't try to walk them. Do this before
		 * freeing them so we don't try to walk them when marking
		 * those pages themselves as free!
		 *
		 * Note: when a process terminates, the MMU root pointer
		 * still points to that process' page table, until the next
		 * process is dispatched.  This is OK, since the process'
		 * page table is in system memory, and it isn't going to be
		 * freed.  It is going to wind up on the free process list,
		 * though, after dispose_proc. This might be Not A Good
		 * Thing.
		 */

		hold_addr = curproc->p_mem->addr;
		hold_mem = curproc->p_mem->mem;

		curproc->p_mem->mem = NULL;
		curproc->p_mem->addr = NULL;
		curproc->p_mem->num_reg = 0;

		kfree(hold_addr);
		kfree(hold_mem);
# endif
# endif
	}
/*	else
		 make TSR process non-swappable */
		
	/* find our parent (if parent not found, then use process 0 as parent
	 * since that process is constantly in a wait loop)
	 */
	p = pid2proc (curproc->ppid);
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
	sr = splhigh ();
	if (p->wait_q == WAIT_Q
		&& (p->wait_cond == (long) curproc || p->wait_cond == (long) sys_pwaitpid))
	{
		rm_q (WAIT_Q, p);
		add_q (READY_Q, p);
		
		TRACE (("terminate: waking up parent"));
	}
	spl (sr);
	
	if (curproc->ptracer && curproc->ptracer != p)
	{
		/* BUG: should we ensure curproc->ptracer is awake ? */
		
		/* tell tracing process */
		post_sig (curproc->ptracer, SIGCHLD);
	}
	
	/* inform of process termination */
	post_sig (p, SIGCHLD);
	
	/* find our children, and orphan them
	 * also, check for processes we were tracing, and
	 * cancel the trace
	 */
	i = curproc->pid;
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->ppid == i)
		{
			p->ppid = 0;	/* have the system adopt it */
			if (p->wait_q == ZOMBIE_Q) 
				wakemint = 1;	/* we need to wake proc. 0 */
		}
		if (p->ptracer == curproc)
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
	
	if (wakemint)
	{
		ushort sr;
		
		sr = splhigh ();
		p = rootproc;		/* pid 0 */
		if (p->wait_q == WAIT_Q)
		{
			rm_q(WAIT_Q, p);
			add_q(READY_Q, p);
		}
		spl(sr);
	}

	/* this makes sure that our children are inherited by the system;
	 * plus, it may help avoid problems if somehow a signal gets
	 * through to us
	 */
	for (i = 0; i < NSIG; i++)
# if 1
		SIGACTION(curproc, i).sa_handler = SIG_IGN;
# else
		curproc->sighandle[i] = SIG_IGN;
# endif
	
	/* finally, reset the time/date stamp for /proc and /sys.  */
	procfs_stmp = xtime;
	
	sleep (que, (long)(unsigned)code);
	
	/* we shouldn't ever get here */
	FATAL ("terminate: sleep woke up when it shouldn't have");
	return 0;
}

long
kernel_pterm (PROC *p, int code)
{
	CONTEXT *syscall;
	
	TRACE(("Pterm(%d)", code));
	
	/* call the process termination vector */
	syscall = &p->ctxt[SYSCALL];
	
	if (syscall->term_vec != (long) rts)
	{
		TRACE(("term_vec: user has something to do"));
		
		/* we handle the termination vector just like Supexec(), by
		 * sending signal 0 to the process. See supexec() in xbios.c
		 * for details. Note that we _always_ want to unwind the
		 * signal stack, and setting bit 1 of curproc->p_sigmask tells
		 * handle_sig to do that -- see signal.c.
		 */
		p->p_sigmask |= 1L;
		(void) supexec ((Func) syscall->term_vec, 0L, 0L, 0L, 0L, (long) code);
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
sys_pterm (int code)
{
	/* Exit all non-closed shared libraries */
	for (;;)
	{
		int cont;
		
		cont = slb_close_on_exit (0);
		if (cont == 1)
			return code;
		
		if (cont == 0)
			break;
	}
	
	return (kernel_pterm (curproc, code));
}

long _cdecl
sys_pterm0 (void)
{
	return sys_pterm (0);
}

long _cdecl
sys_ptermres (long save, int code)
{
	PROC *p = curproc;
	struct memspace *mem = p->p_mem;
	
	MEMREGION *m;
	int i;
	
	
	TRACE(("Ptermres(%ld, %d)", save, code));
	
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
				mark_region (m, PROT_G);
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

long _cdecl
sys_pwaitpid (int pid, int nohang, long *rusage)
{
	long r;
	PROC *p;
	int ourpid, ourpgrp;
	int found;
	
	
	TRACE(("Pwaitpid(%d, %d, %lx)", pid, nohang, rusage));
	
	
	ourpid = curproc->pid;
	ourpgrp = curproc->pgrp;
	
	/* if there are terminated children, clean up and return their info;
	 * if there are children, but still running, wait for them;
	 * if there are no children, return an error
	 */
	do {
		/* look for any children */
		found = 0;
		for (p = proclist; p; p = p->gl_next)
		{
			if ((p->ppid == ourpid || p->ptracer == curproc)
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
					TRACE(("Pwaitpid(%d, %d) -> 0 [nohang & 1]", pid, nohang));
					return 0;
				}
				
				if (curproc->pid)
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
	r = (((unsigned long)p->pid) << 16) | (p->wait_cond & 0x0000ffff);
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
	if (curproc->pid == p->ppid)
	{
		/* add child's resource usage to parent's */
		if (p->wait_q == TSR_Q || p->wait_q == ZOMBIE_Q)
		{
			curproc->chldstime += p->systime + p->chldstime;
			curproc->chldutime += p->usrtime + p->chldutime;
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
		if (curproc == p->ptracer)
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
	assert (p != curproc);
	
	/* take the child off both the global and ZOMBIE lists */
	{
		ushort sr;
		
		sr = splhigh ();
		rm_q (ZOMBIE_Q, p);
		spl (sr);
		
		if (proclist == p)
		{
			proclist = p->gl_next;
			p->gl_next = 0;
		}
		else
		{
			PROC *q = proclist;
			
			while (q && q->gl_next != p)
				q = q->gl_next;
			
			assert (q);
			
			q->gl_next = p->gl_next;
			p->gl_next = 0;
		}
	}
	
	if (--p->p_cred->links == 0)
	{
		free_cred (p->p_cred->ucr);
		kfree (p->p_cred);
	}
	
	free_sigacts (p);
	
	kfree (p->p_fd);
	kfree (p->p_cwd);
	free_mem (p);
	
	/* free the PROC structure */
	kfree (p);
	
	TRACE(("Pwaitpid(%d, %d) -> %lx [end]", pid, nohang, r));
	return r;
}

/*
 * p_wait3: BSD process termination primitive, here to maintain
 * compatibility with existing binaries.
 */
long _cdecl
sys_pwait3 (int nohang, long *rusage)
{
	return sys_pwaitpid (-1, nohang, rusage);
}

/*
 * p_wait: block until a child has exited, and don't worry about
 * resource stats. this is provided as a convenience, and to maintain
 * compatibility with existing binaries (yes, I'm lazy...). we could
 * make do with Pwaitpid().
 */
long _cdecl
sys_pwait (void)
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
	return sys_pwaitpid (-1, 2, NULL);
}
