/*
 * $Id$
 * 
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-10-08
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * known bugs:
 * 
 * todo:
 * 
 */

# include "ptrace.h"

# include "libkern/libkern.h"

# include "mint/arch/register.h"
# include "mint/asm.h"
# include "mint/proc.h"
# include "mint/signal.h"

# include "arch/cpu.h"
# include "arch/process_reg.h"
# include "arch/mprot.h"

# include "memory.h"
# include "proc.h"
# include "signal.h"
# include "util.h"


/* 
 * NAME
 *      ptrace - process tracing and debugging
 * 
 * SYNOPSIS
 *      #include <sys/types.h>
 *      #include <sys/ptrace.h>
 * 
 *      int
 *      ptrace(int request, pid_t pid, caddr_t addr, int data);
 * 
 * DESCRIPTION
 *      ptrace() provides tracing and debugging facilities.  It allows one pro-
 *      cess (the tracing process) to control another (the traced process).  Most
 *      of the time, the traced process runs normally, but when it receives a
 *      signal (see sigaction(2)), it stops.  The tracing process is expected to
 *      notice this via wait(2) or the delivery of a SIGCHLD signal, examine the
 *      state of the stopped process, and cause it to terminate or continue as
 *      appropriate.  ptrace() is the mechanism by which all this happens.
 * 
 *      The request argument specifies what operation is being performed; the
 *      meaning of the rest of the arguments depends on the operation, but except
 *      for one special case noted below, all ptrace() calls are made by the
 *      tracing process, and the pid argument specifies the process ID of the
 *      traced process.  request can be:
 * 
 *      PT_TRACE_ME   This request is the only one used by the traced process; it
 *                    declares that the process expects to be traced by its par-
 *                    ent.  All the other arguments are ignored.  (If the parent
 *                    process does not expect to trace the child, it will proba-
 *                    bly be rather confused by the results; once the traced pro-
 *                    cess stops, it cannot be made to continue except via
 *                    ptrace().)  When a process has used this request and calls
 *                    execve(2) or any of the routines built on it (such as
 *                    execv(3)), it will stop before executing the first instruc-
 *                    tion of the new image.  Also, any setuid or setgid bits on
 *                    the executable being executed will be ignored.
 * 
 *      PT_READ_I, PT_READ_D
 *                    These requests read a single int of data from the traced
 *                    process' address space.  Traditionally, ptrace() has al-
 *                    lowed for machines with distinct address spaces for in-
 *                    struction and data, which is why there are two requests:
 *                    conceptually, PT_READ_I reads from the instruction space
 *                    and PT_READ_D reads from the data space.  In the current
 *                    NetBSD implementation, these two requests are completely
 *                    identical.  The addr argument specifies the address (in the
 *                    traced process' virtual address space) at which the read is
 *                    to be done.  This address does not have to meet any align-
 *                    ment constraints.  The value read is returned as the return
 *                    value from ptrace().
 * 
 *      PT_WRITE_I, PT_WRITE_D
 *                    These requests parallel PT_READ_I and PT_READ_D, except
 *                    that they write rather than read.  The data argument sup-
 *                    plies the value to be written.
 * 
 *      PT_CONTINUE   The traced process continues execution.  addr is an address
 *                    specifying the place where execution is to be resumed (a
 *                    new value for the program counter), or (caddr_t)1 to indi-
 *                    cate that execution is to pick up where it left off.  data
 *                    provides a signal number to be delivered to the traced pro-
 *                    cess as it resumes execution, or 0 if no signal is to be
 *                    sent.
 * 
 *      PT_KILL       The traced process terminates, as if PT_CONTINUE had been
 *                    used with SIGKILL given as the signal to be delivered.
 * 
 *      PT_ATTACH     This request allows a process to gain control of an other-
 *                    wise unrelated process and begin tracing it.  It does not
 *                    need any cooperation from the to-be-traced process.  In
 *                    this case, pid specifies the process ID of the to-be-traced
 *                    process, and the other two arguments are ignored.  This re-
 *                    quest requires that the target process must have the same
 *                    real UID as the tracing process, and that it must not be
 *                    executing a setuid or setgid executable.  (If the tracing
 *                    process is running as root, these restrictions do not ap-
 *                    ply.)  The tracing process will see the newly-traced pro-
 *                    cess stop and may then control it as if it had been traced
 *                    all along.
 * 
 *                    Two other restrictions apply to all tracing processes, even
 *                    those running as root.  First, no process may trace the
 *                    process running init(8).  Second, if a process has its root
 *                    directory set with chroot(2), it may not trace another pro-
 *                    cess unless that process's root directory is at or below
 *                    the tracing process's root.
 * 
 *      PT_DETACH     This request is like PT_CONTINUE, except that it does not
 *                    allow specifying an alternative place to continue execu-
 *                    tion, and after it succeeds, the traced process is no
 *                    longer traced and continues execution normally.
 * 
 *      Additionally, machine-specific requests can exist.  On the SPARC, these
 *      are:
 * 
 *      PT_GETREGS    This request reads the traced process' machine registers
 *                    into the ``struct reg'' (defined in <machine/reg.h>) point-
 *                    ed to by addr.
 * 
 *      PT_SETREGS    This request is the converse of PT_GETREGS; it loads the
 *                    traced process' machine registers from the ``struct reg''
 *                    (defined in <machine/reg.h>) pointed to by addr.
 * 
 *      PT_GETFPREGS  This request reads the traced process' floating-point reg-
 *                    isters into the ``struct fpreg'' (defined in
 *                    <machine/reg.h>) pointed to by addr.
 * 
 *      PT_SETFPREGS  This request is the converse of PT_GETFPREGS; it loads the
 *                    traced process' floating-point registers from the ``struct
 *                    fpreg'' (defined in <machine/reg.h>) pointed to by addr.
 * 
 * ERRORS
 *      Some requests can cause ptrace() to return -1 as a non-error value; to
 *      disambiguate, errno can be set to 0 before the call and checked after-
 *      wards.  The possible errors are:
 * 
 *      [ESRCH]
 *            No process having the specified process ID exists.
 * 
 *      [EINVAL]
 *            o   A process attempted to use PT_ATTACH on itself.
 *            o   The request was not one of the legal requests.
 *            o   The signal number (in data) to PT_CONTINUE was neither 0 nor a
 *                legal signal number.
 *            o   PT_GETREGS, PT_SETREGS, PT_GETFPREGS, or PT_SETFPREGS was at-
 *                tempted on a process with no valid register set.  (This is nor-
 *                mally true only of system processes.)
 * 
 *      [EBUSY]
 *            o   PT_ATTACH was attempted on a process that was already being
 *                traced.
 *            o   A request attempted to manipulate a process that was being
 *                traced by some process other than the one making the request.
 *            o   A request (other than PT_ATTACH) specified a process that
 *                wasn't stopped.
 * 
 *      [EPERM]
 *            o   A request (other than PT_ATTACH) attempted to manipulate a pro-
 *                cess that wasn't being traced at all.
 *            o   An attempt was made to use PT_ATTACH on a process in violation
 *                of the requirements listed under PT_ATTACH above.
 */


long _cdecl
p_trace (short request, short pid, void *addr, long data)
{
	PROC *p = curproc;
	PROC *t;
	
	int write;
	long ret;
	
	
	DEBUG (("Ptrace(%i, %i, %lx, %li)", request, pid, (long) addr, data));
	
	
	if (request != PT_TRACE_ME)
	{
		t = pid2proc (pid);
		if (!t)
		{
			DEBUG (("Ptrace(%i, pid %i) - no such process.", request, pid));
			return ESRCH;
		}
	}
	else
		t = p;
	
	/* consistency checks */
	switch (request)
	{
		case PT_TRACE_ME:
		{
			/* tracing itself is always legal */
			break;
		}
		case PT_ATTACH:
		{
			/*
			 * You can't attach to a process if:
			 *	(1) it's the process that's doing the attaching,
			 */
			if (t == p)
			{
				DEBUG (("Ptrace(%i, pid %i) - can't attach itself.", request, pid));
				return EINVAL;
			}
			
			/*
			 *	(2) it's already being traced, or
			 */
			if (t->ptracer)
			{
				DEBUG (("Ptrace(%i, pid %i) - process is already beeing traced.", request, pid));
				return EBUSY;
			}
			
			/*
			 *	(3) it's not owned by you, or is set-id on exec
			 *	    (unless you're root), or...
			 */
			if ((t->ruid != p->ruid) && p->euid)
				return EPERM;
			
			/*
			 *	(4) ...it's init, which controls the security level
			 *	    of the entire system, and the system was not
			 *          compiled with permanently insecure mode turned
			 *	    on.
			 */
			if (t == rootproc)
			{
				DEBUG (("Ptrace(%i, pid %i) - rootproc can't be traced.", request, pid));
				return EPERM;
			}
			
			/*
			 * (4) the tracer is chrooted, and its root directory is
			 * not at or above the root directory of the tracee
			 */
			
			/* if (!proc_isunder (t, p))
				return EPERM; */
			
			break;
		}
		case PT_READ_I:
		case PT_READ_D:
		case PT_WRITE_I:
		case PT_WRITE_D:
		case PT_CONTINUE:
		case PT_KILL:
		case PT_DETACH:
		case PT_STEP:
		case PT_GETREGS:
		case PT_SETREGS:
		case PT_GETFPREGS:
		case PT_SETFPREGS:
		case PT_BASEPAGE:
		{
			/*
			 * You can't do what you want to the process if:
			 *	(1) It's not being traced at all,
			 */
			if (!t->ptracer)
			{
				DEBUG (("Ptrace(%i, pid %i) - process isn't beeing traced.", request, pid));
				return EPERM;
			}
			
			/*
			 *	(2) it's not being traced by you, or
			 */
			if (t->ptracer != p)
			{
				DEBUG (("Ptrace(%i, pid %i) - not the tracer process.", request, pid));
				return EBUSY;
			}
			
			/*
			 *	(3) it's not currently stopped.
			 */
			if (t->wait_q != STOP_Q)
			{
				DEBUG (("Ptrace(%i, pid %i) - child not stopped.", request, pid));
				return EBUSY;
			}
			
			break;
		}
		default:
		{
			/* not a legal request */
			DEBUG (("Ptrace(%i, pid %i) - no legal request.", request, pid));
			return EINVAL;
		}
	}
	
	write = 0;
	
	switch (request)
	{
		case PT_TRACE_ME:
		{
			t->ptracer = pid2proc (t->ppid);
			if (t->ptracer)
				return 0;
			
			DEBUG (("Ptrace(%i, pid %i) - ppid invalid?.", request, pid));
			return EINVAL;
		}
		case PT_WRITE_I:
		case PT_WRITE_D:
			write = 1;
		case PT_READ_I:
		case PT_READ_D:
			/* write is 0 */
		{
			MEMREGION *m;
			
			m = proc_addr2region (t, (long) addr);
			if (m)
			{
				int prot_hold;
				
				/* we should tell the memory protection that
				 * we need access to the region. if we are
				 * reading from a saveplace region, the
				 * kernel always has access to the memory,
				 * so it suffices to get access to the
				 * "real" region.
				 */
				//attach_region (p, m);
				prot_hold = prot_temp (m->loc, m->len, -1);
				assert (prot_hold < 0);
				
				/* XXX - what about forked regions? */
				if (write)
				{
					*(long *) addr = data;
					
					/* flush write-back cache */
					cpush(addr, 4);
				}
				else
					*(long *) data = *(long *) addr;
				
				//detach_region (p, m);
				if (prot_hold != -1)
					prot_temp (m->loc, m->len, prot_hold);
				
				return 0;
			}
			
			DEBUG (("Ptrace(%i, pid %i) - invalid addr.", request, pid));
			return EIO;
		}
		case PT_ATTACH:
		{
			t->ptracer = p;
			post_sig (t, SIGSTOP);
			return 0;
		}
		case PT_DETACH:
		{
			t->ptracer = NULL;
			t->ptraceflags = 0;
			t->ctxt[SYSCALL].ptrace = 0;
			t->ctxt[CURRENT].ptrace = 0;
			
			/* if the process is stopped, restart it */
			if (t->wait_q == STOP_Q)
			{
				t->sigpending &= ~STOPSIGS;
				post_sig (t, SIGCONT);
			}
			
			return 0;
		}
		case PT_CONTINUE:
		case PT_SYSCALL:
		case PT_STEP:
		{
			if (data < 0 || data >= NSIG)
			{
				DEBUG (("Ptrace(%i, pid %i) - data out of range.", request, pid));
				return EINVAL;
			}
			
			ret = process_single_step (t, (request == PT_STEP));
			if (ret)
				return ret;
			
			if (addr != (void *) 1L)
			{
				ret = process_set_pc (t, (long) addr);
				if (ret)
					return ret;
			}
			
# define PF_TRACESYS 0x1
			if (request == PT_SYSCALL)
				t->ptraceflags |= PF_TRACESYS;
			else
				t->ptraceflags &= ~PF_TRACESYS;
			
			/* Discard the saved frame */
			t->ctxt[SYSCALL].sfmt = 0;
			t->sigpending = 0;
			
			if (data != 0)
			{
				post_sig (t, data);
				
				/* another SIGNULL hack... within check_sigs()
				 * we watch for a pending SIGNULL, if we see
				 * this then we allow delivery of a signal to
				 * the process, rather than telling the
				 * parent.
				 */
				t->sigpending |= 1L;
			}
			
			/* wake the process up */
			{
				ushort sr = splhigh ();
				if (t->wait_q == STOP_Q)
				{
					rm_q (t->wait_q, t);
					add_q (READY_Q, t);
				}
				spl (sr);
			}
			
			return 0;
		}
		case PT_KILL:
		{
			t->ptracer = NULL;
			t->ptraceflags = 0;
			t->ctxt[SYSCALL].ptrace = 0;
			t->ctxt[CURRENT].ptrace = 0;
			
			post_sig (t, SIGKILL);
			
			return 0;
		}
		
		case  PT_SETREGS:
			write = 1;
		case  PT_GETREGS:
			/* write is 0 */
		{
			if (!addr)
				ret = EINVAL;
			else if (write)
				ret = process_setregs (t, addr);
			else
				ret = process_getregs (t, addr);
			
			return ret;
		}
		
		case  PT_SETFPREGS:
			write = 1;
		case  PT_GETFPREGS:
			/* write is 0 */
		{
			if (!addr)
				ret = EINVAL;
			else if (write)
				ret = process_setfpregs (t, addr);
			else
				ret = process_getfpregs (t, addr);
			
			return ret;
		}
		
		case PT_BASEPAGE:
		{
			*(long *) data = (long) t->base;
			return 0;
		}
	}
	
	return EINVAL;
}
