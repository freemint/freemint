/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 * 
 * 
 * machine dependant signal handling
 * 
 */

# include "sig_mach.h"

# include "mint/asm.h"
# include "mint/basepage.h"
# include "mint/proc.h"
# include "mint/signal.h"

# include "bios.h"
# include "kmemory.h"
# include "global.h"
# include "signal.h"

# include "context.h"	/* save_context, restore_context */
# include "kernel.h"
# include "mprot.h"
# include "syscall.h"
# include "user_things.h"

void (*sig_routine)();	/* used in intr.S */
short sig_exc = 0;	/* used in intr.S */

long unwound_stack = 0;	/* used in syscall.S */

struct sigcontext
{
	ulong	sc_pc;
	ulong	sc_usp;
	ushort	sc_sr;
};

int
sendsig(ushort sig)
{
	struct sigaction *sigact = &(SIGACTION(curproc, sig));
	unsigned long oldstack, newstack;
	unsigned long *stack;
	struct user_things *ut;
	struct sigcontext *sigctxt;
	CONTEXT *call, contexts[2];

# define newcurrent (contexts[0])
# define oldsysctxt (contexts[1])


	assert(curproc->stack_magic == STACK_MAGIC);
	
	/* another kludge: there is one case in which the p_sigreturn
	 * mechanism is invoked by the kernel, namely when the user
	 * calls Supexec() or when s/he installs a handler for the
	 * GEMDOS terminate vector (#0x102) and the program terminates.
	 * MiNT fakes the call to user code with signal 0 (SIGNULL);
	 * programs that longjmp out of the user function and are
	 * later sent back to it again (e.g. if ^C keeps getting
	 * pressed and a terminate vector has been installed) will
	 * grow the stack without bound unless we watch for this case.
	 *
	 * Solution (sort of): whenever Pterm() is called, we unwind
	 * the stack; otherwise, we let it grow, so that nested
	 * Supexec() calls work.
	 *
	 * Note that SIGNULL is thrown away when sent by user
	 * processes,  and the user can't mask it (it's UNMASKABLE),
	 * so there is is no possibility of confusion with anything
	 * the user does.
	 */
	
	if (sig == 0)
	{
		/* kernel_pterm() sets p_sigmask to let us know to do
		 * Psigreturn
		 */
		
		if (curproc->p_sigmask & 1L)
		{
			sys_psigreturn();
			curproc->p_sigmask &= ~1L;
		}
		else
		{
			unwound_stack = 0;
		}
	}
	
	++curproc->nsigs;
	
	if (curproc->p_flag & P_FLAG_SYS)
	{
		/* This is a system process, e.g. a kernel thread. We can't
		 * go into user mode for signal handling. As we know we are
		 * already in kernel and a kernel thread must rts from the
		 * signal handler we can simply callout the signal handler
		 * as function.
		 */
		
		DEBUG(("system process, calling signal handler 0x%lx (%d)(%s) directly", sigact->sa_handler, sig, curproc->name));
		((void (*)(short)) sigact->sa_handler)(sig);
		
		if (sigact->sa_flags & SA_RESETHAND)
		{
			TRACE(("resetting sa_handler"));
			
			sigact->sa_handler = SIG_DFL;
			sigact->sa_flags &= ~SA_RESETHAND;
		}
		
		return 0;
	}

	call = &(curproc->ctxt[SYSCALL]);
	
	/* what we do is build two fake stack frames; the top one is
	 * for a call to the user function, with (long)parameter being the
	 * signal number; the bottom one is for sig_return.
	 * When the user function returns, it returns to sig_return, which
	 * calls into the kernel to restore the context in prev_ctxt
	 * (thus putting us back here). We can then continue on our way.
	 */
	
	/* set a new system stack, with a bit of buffer space */
	oldstack = curproc->sysstack;
	newstack = ((unsigned long) &newcurrent) - 0x40UL - 12UL - 0x100UL;
	if (newstack < (unsigned long) curproc->stack + ISTKSIZE + 256)
	{
		ALERT("stack overflow");
		return 1;
	}
	else if ((unsigned long) curproc->stack + STKSIZE < newstack)
	{
		FATAL("system stack not in proc structure");
	}
	
	oldsysctxt = *call;
	stack = (unsigned long *)(call->sr & 0x2000 ? call->ssp : call->usp);
	
	/* Hmmm... here's another potential problem for the signal 0
	 * terminate vector: if the program keeps returning back to
	 * user mode without worrying about the supervisor stack,
	 * we'll eventually overflow it. However, if the program is
	 * in supervisor mode itself, then we don't want to stop on
	 * its stack. Temporary solution: ignore the problem, the
	 * stack's only growing 12 bytes at a time.
	 * 
	 * in addition to the signal number we stuff the vector offset
	 * on the stack; if the user is interested they can sniff it,
	 * if not ignoring it needs no action on their part. Why do we
	 * need this? So that a single SIGFPE handler (for example)
	 * can discriminate amongst the multiple things which may get
	 * thrown its way
	 */
	ut = curproc->p_mem->tp_ptr;

	stack -= (sizeof(struct sigcontext) + 3) / 4;
// 	stack -= 3;
	sigctxt = (struct sigcontext *)stack;
	sigctxt->sc_pc = oldsysctxt.pc;
	sigctxt->sc_usp = oldsysctxt.usp;
	sigctxt->sc_sr = oldsysctxt.sr;
	*(--stack) = (unsigned long) sigctxt;
	*(--stack) = (unsigned long) call->sfmt & 0xfff;
	*(--stack) = (unsigned long) sig;
	*(--stack) = ut->sig_return_p;
	if (call->sr & 0x2000)
		call->ssp = ((unsigned long)stack);
	else
		call->usp = ((unsigned long)stack);
	
	call->pc = sigact->sa_handler;
	/* don't restart FPU communication */
	call->sfmt = call->fstate.bytes[0] = 0;
	
	if (sigact->sa_flags & SA_RESETHAND)
	{
		TRACE(("resetting sa_handler"));
		
		sigact->sa_handler = SIG_DFL;
		sigact->sa_flags &= ~SA_RESETHAND;
	}
	
	if (save_context(&newcurrent) == 0)
	{
		/* go do the signal; eventually, we'll restore this
		 * context (unless the user longjmp'd out of his
		 * signal handler). while the user is handling the
		 * signal, it's masked out to prevent race conditions.
		 * p_sigreturn() will unmask it for us when the user
		 * is finished.
		 */
		
		/* set D0 so next return is different */
		newcurrent.regs[0] = CTXT_MAGIC;
		assert(curproc->magic == CTXT_MAGIC);
		
		/* unwound_stack is set by p_sigreturn() */
		if (sig == 0 && unwound_stack)
			stack = (unsigned long *)unwound_stack;
		else
			/* newstack points just below our current sp,
			 * much less than ISTKSIZE away so better set
			 * it up with interrupts off...  -nox
			 */
			stack = (unsigned long *)newstack;
		
		splhigh();
		
		unwound_stack = 0;
		curproc->sysstack = (unsigned long)stack;
		stack++;
		*stack++ = FRAME_MAGIC;
		*stack++ = oldstack;
		*stack = sig ;

		leave_kernel();

		restore_context(call);
	}
	
	/* OK, we get here from p_sigreturn, via the user returning
	 * from the handler to sig_return. Restoring the stack and
	 * unmasking the signal have been done already for us by
	 * p_sigreturn. We should just restore the old system call
	 * context and continue with whatever it was we were doing.
	 */
	
	TRACE(("done handling signal"));
	
	oldsysctxt.pc = sigctxt->sc_pc;
	oldsysctxt.usp = sigctxt->sc_usp;
	oldsysctxt.sr &= 0xff00;
	oldsysctxt.sr |= sigctxt->sc_sr & 0xff;
	
	curproc->ctxt[SYSCALL] = oldsysctxt;
	assert(curproc->magic == CTXT_MAGIC);

# undef oldsysctxt
# undef newcurrent
	
	return 0;
}

/*
 * the Psigreturn system call
 * When called by the user from inside a signal handler, it indicates a
 * desire to restore the old stack frame prior to a longjmp() out of
 * the handler.
 * When called from the sig_return module, it indicates that the user
 * is finished a handler, and we should not only restore the stack
 * frame but also the old context we were working in (which is on the
 * system call stack -- see handle_sig).
 * The syscall pc is "pc_valid_return" in the second case.
 */
long _cdecl
sys_psigreturn (void)
{
	CONTEXT *oldctxt;
	unsigned long *frame;
	long sig;
	struct user_things *ut = curproc->p_mem->tp_ptr;

	unwound_stack = 0;
top:
	frame = (unsigned long *)curproc->sysstack;
	frame++;	/* frame should point at FRAME_MAGIC, now */
	sig = frame[2];
	if (*frame != FRAME_MAGIC || (sig < 0) || (sig >= NSIG))
	{
		FATAL("Psigreturn: system stack corrupted");
	}
	if (frame[1] == 0)
	{
		DEBUG (("Psigreturn: frame at %p points to 0", frame-1));
		return E_OK;
	}
	unwound_stack = curproc->sysstack;
	TRACE (("Psigreturn(%d)", (int) sig));
	
	curproc->sysstack = frame[1];	/* restore frame */
	curproc->p_sigmask &= ~(1L<<sig); /* unblock signal */
	
	if (curproc->ctxt[SYSCALL].pc != ut->pc_valid_return_p)
	{
		/* here, the user is telling us that a longjmp out of a signal
		 * handler is about to occur; so we should unwind *all* the
		 * signal frames
		 */
		goto top;
	}
	else
	{
		unwound_stack = 0;
		oldctxt = (CONTEXT *) (((long) &frame[2]) + 0x40 + 0x100);
		if (oldctxt->regs[0] != CTXT_MAGIC)
		{
			FATAL ("p_sigreturn: corrupted context");
		}
		assert (curproc->magic == CTXT_MAGIC);
		
		restore_context (oldctxt);
		
		/* dummy -- this isn't reached */
		return E_OK;
	}
}

/*
 * exception numbers corresponding to signals
 */
static char excep_num[NSIG] =
{
	0,
	0,
	0,
	0,
	4,		/* SIGILL  == illegal instruction */
	9,		/* SIGTRAP == trace trap */
	4,		/* pretend SIGABRT is also illegal instruction */
	8,		/* SIGPRIV == privileged instruction exception */
	5,		/* SIGFPE  == divide by zero */
	0,		
	2,		/* SIGBUS  == bus error */
	3		/* SIGSEGV == address error */
	
	/* everything else gets zeros */
};

/*
 * a "0" means we don't print a message when it happens -- typically the
 * user is expecting a synchronous signal, so we don't need to report it
 */
static const char *signames[NSIG] =
{
	0,
	0,
	0,
	0,
	"ILLEGAL INSTRUCTION",
	"TRACE TRAP",
	0 /* "SIGABRT" */,
	"PRIVILEGE VIOLATION",
	"DIVISION BY ZERO",
	0,
	"BUS ERROR",
	"ADDRESS ERROR",
	"BAD SYSTEM CALL",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	"CPU TIME EXHAUSTED",
	"FILE TOO BIG",
	0,
	0,
	0,
	0,
	0,
	/* Don't be smart and put "POWER FAILURE RESTART" here.  The init
	 * daemon will already take care of informing our users before the
	 * lights go out
	 */
	0
};

/*
 * replaces the TOS "show bombs" routine: for now, print the name of the
 * interrupt on the console, and save info on the crash in the appropriate
 * system area
 */
void
bombs(ushort sig)
{
	long *procinfo = (long *)0x380L;
	int i;
	CONTEXT *crash;

	if (sig >= NSIG)
	{
		ALERT("bombs(%d): sig out of range", sig);
	}
	else if (signames[sig])
	{
# ifdef WITH_MMU_SUPPORT
		if (!no_mem_prot && sig == SIGBUS)
		{
		    /* already reported by report_buserr */
		}
		else
# endif
		{
			/* uk: give some more information in case of a crash, so that a
			 *     progam which shared text can be debugged better.
			 */
			BASEPAGE *base = curproc->p_mem->base;
			long ptext = 0;
			long pdata = 0;
			long pbss = 0;
			
			/* can it happen, that base == NULL???? */
			if (base)
			{
				ptext = base->p_tbase;
				pdata = base->p_dbase;
				pbss = base->p_bbase;
			}
			
			if (sig == SIGSEGV || sig == SIGBUS)
			{
				ALERT("%s: User PC=%lx, Address: %lx (basepage=%lx, text=%lx, data=%lx, bss=%lx)",
					signames[sig],
					curproc->exception_pc, curproc->exception_addr,
					(unsigned long)base, ptext, pdata, pbss);
			}
			else
			{
				ALERT("%s: User PC=%lx (basepage=%lx, text=%lx, data=%lx, bss=%lx)",
					signames[sig],
					curproc->exception_pc,
					(unsigned long)base, ptext, pdata, pbss);
			}
		}
		
		/* save the processor state at crash time
		 * 
		 * assumes that "crash time" is the context
		 * curproc->ctxt[SYSCALL]
		 * 
		 * BUG: this is not true if the crash happened in the kernel;
		 * in the latter case, the crash context wasn't saved anywhere.
		 */
		crash = &curproc->ctxt[SYSCALL];
		*procinfo++ = 0x12345678L; /* magic flag for valid info */
		
		for (i = 0; i < 15; i++)
			*procinfo++ = crash->regs[i];
		
		*procinfo++ = curproc->exception_ssp;
		*procinfo++ = ((long)excep_num[sig]) << 24L;
		*procinfo = crash->usp;

		/* we're also supposed to save some info from the supervisor
		 * stack. it's not clear what we should do for MiNT, since
		 * most of the stuff that used to be on the stack has been
		 * put in the CONTXT struct. Moreover, we don't want to crash
		 * because of an attempt to access illegal memory. Hence, we
		 * do nothing here...
		 */
	}
	else
	{
		TRACE(("bombs(%d)", sig));
	}
}

/*
 * interrupt handlers to raise SIGBUS, SIGSEGV, etc. Note that for
 * really fatal errors we reset the handler to SIG_DFL, so that
 * a second such error kills us
 */

static void
exception(ushort sig)
{
	assert(curproc->stack_magic == STACK_MAGIC);
	assert(curproc->p_sigacts);
	
	DEBUG(("signal #%d raised [syscall_pc 0x%lx, exception_pc 0x%lx]",
		sig, curproc->ctxt[SYSCALL].pc, curproc->exception_pc));
	
	SIGACTION(curproc, sig).sa_flags |= SA_RESETHAND;
	raise(sig);
}

void
sigbus(void)
{
	assert(curproc->stack_magic == STACK_MAGIC);
	assert(curproc->p_sigacts);
	
	if (SIGACTION(curproc, SIGBUS).sa_handler == SIG_DFL)
		report_buserr();
	
	exception(SIGBUS);
}

void
sigaddr(void)
{
	assert(curproc->stack_magic == STACK_MAGIC);
	exception(SIGSEGV);
}

void
sigill(void)
{
	assert(curproc->stack_magic == STACK_MAGIC);
	exception(SIGILL);
}

void
sigpriv(void)
{
	assert(curproc->stack_magic == STACK_MAGIC);
	
	DEBUG(("signal SIGPRIV raised [syscall_pc 0x%lx, exception_pc 0x%lx]",
		curproc->ctxt[SYSCALL].pc, curproc->exception_pc));
	
	raise(SIGPRIV);
}

void
sigfpe(void)
{
	assert(curproc->stack_magic == STACK_MAGIC);
	
	DEBUG(("signal SIGFPE raised [syscall_pc 0x%lx, exception_pc 0x%lx]",
		curproc->ctxt[SYSCALL].pc, curproc->exception_pc));
	
	if (fpu)
	{
		CONTEXT *ctxt = &curproc->ctxt[SYSCALL];
		
		/* 0x38 is the stack frame size value for 68882 */
		if ((ctxt->fstate.words[0] & 0xff) == 0x38
			&& ((ctxt->sfmt & 0xfff) >= 0xc0L)
			&& ((ctxt->sfmt & 0xfff) <= 0xd8L))
		{
			/* fix a bug in the 68882 - Motorola call it a
			 * feature :-)
			 */
			ctxt->fstate.bytes[ctxt->fstate.bytes[1]] |= 1 << 3;
		}
	}
	
	raise(SIGFPE);
}

void
sigtrap(void)
{
	assert(curproc->stack_magic == STACK_MAGIC);
	
	DEBUG(("signal SIGTRAP raised [syscall_pc 0x%lx, exception_pc 0x%lx]",
		curproc->ctxt[SYSCALL].pc, curproc->exception_pc));
	
	raise(SIGTRAP);
}

void
haltformat(void)
{
	FATAL("halt: invalid stack frame format");
}

void
haltcpv(void)
{
	FATAL("halt: coprocessor protocol violation");
}
