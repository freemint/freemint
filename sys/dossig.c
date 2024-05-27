/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992,1994 Eric R. Smith.
 * All rights reserved.
 *
 */

/* dossig.c: dos signal handling routines */

# include "dossig.h"

# include "mint/arch/mfp.h"
# include "mint/asm.h"
# include "mint/credentials.h"
# include "mint/signal.h"

# include "proc.h"
# include "signal.h"
# include "util.h"


/*
 * send a signal to another process. If pid > 0, send the signal just to
 * that process. If pid < 0, send the signal to all processes whose process
 * group is -pid. If pid == 0, send the signal to all processes with the
 * same process group id.
 *
 * note: post_sig just posts the signal to the process.
 */

long _cdecl
sys_p_kill (short pid, short sig)
{
	PROC *p;
	long r;

	TRACE (("Pkill(%d, %d)", pid, sig));
	if (sig < 0 || sig >= NSIG)
	{
		DEBUG (("Pkill: signal out of range"));
		return EBADARG;
	}

	if (sig == SIGSEGV || sig == SIGBUS)
	{
		PROC *curr = get_curproc();
		curr->exception_pc = curr->ctxt[SYSCALL].pc;
		curr->exception_addr = -1;
	}

	if (pid < 0)
		r = killgroup (-pid, sig, 0);
	else if (pid == 0)
		r = killgroup (get_curproc()->pgrp, sig, 0);
	else
	{
		p = pid2proc (pid);
		if (p == 0 || p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
		{
			DEBUG (("Pkill: pid %d not found", pid));
			return ENOENT;
		}

		if (get_curproc()->p_cred->ucr->euid && get_curproc()->p_cred->ruid != p->p_cred->ruid)
		{
			DEBUG (("Pkill: wrong user"));
			return EACCES;
		}

		/* if the user sends signal 0, don't deliver it -- for users,
		 * signal 0 is a null signal used to test the existence of
		 * a process
		 */
		if (sig != 0)
			post_sig (p, sig);

		r = E_OK;
	}

	if (r == E_OK)
	{
		check_sigs ();
		TRACE (("Pkill: returning OK"));
	}

	return r;
}

/*
 * set a user-specified signal handler, POSIX.1 style
 * "oact", if non-null, gets the old signal handling
 * behaviour; "act", if non-null, specifies new
 * behaviour
 */

long _cdecl
sys_p_sigaction (short sig, const struct sigaction *act, struct sigaction *oact)
{
	PROC *p = get_curproc();

	TRACE (("Psigaction(%d)", sig));
	assert (p->p_sigacts);

	if (sig < 1 || sig >= NSIG)
		return EBADARG;

	if (act && (sig == SIGKILL || sig == SIGSTOP))
		return EACCES;

	if (oact)
	{
		*oact = SIGACTION(p, sig);
		oact->sa_flags &= SAUSER;
	}

	if (act)
	{
		struct sigaction *sigact = & SIGACTION(p, sig);
		ushort flags;

		sigact->sa_handler = act->sa_handler;
		sigact->sa_mask = act->sa_mask & ~UNMASKABLE;

		/* only the flags in SAUSER can be changed by the user */
		flags = sigact->sa_flags & ~SAUSER;
		flags |= act->sa_flags & SAUSER;
		sigact->sa_flags = flags;

		/* various special things that should happen */
		if (act->sa_handler == SIG_IGN)
		{
			/* discard pending signals */
			p->sigpending &= ~(1L << sig);
		}

		/* I dunno if this is right, but bash seems to expect it */
		p->p_sigmask &= ~(1L << sig);
	}

	return E_OK;
}

/*
 * set a user-specified signal handler
 */
long _cdecl
sys_p_signal (short sig, long handler)
{
	PROC *p = get_curproc();
	struct sigaction *sigact;
	long ret;

	TRACE (("Psignal(%u, %lx [%p])", sig, handler, p->p_sigacts));
	assert (p->p_sigacts && p->p_sigacts->links > 0);

	if (sig < 1 || sig >= NSIG)
	{
		ret = EBADARG;
		goto out;
	}

	if (sig == SIGKILL || sig == SIGSTOP)
	{
		ret = EACCES;
		goto out;
	}

	sigact = & SIGACTION(p, sig);
	TRACE (("Psignal() sigact = %p", sigact));

	/* save old value for return */
	ret = sigact->sa_handler;

	sigact->sa_handler = handler;
	sigact->sa_mask = 0;
	sigact->sa_flags = 0;

	/* various special things that should happen */
	if (handler == SIG_IGN)
	{
		/* discard pending signals */
		p->sigpending &= ~(1L<<sig);
	}

	/* I dunno if this is right, but bash seems to expect it */
	p->p_sigmask &= ~(1L<<sig);

out:
	TRACE (("Psignal() ok (%li)", ret));
	return ret;
}

/*
 * block some signals. Returns the old signal mask.
 */
long _cdecl
sys_p_sigblock (ulong mask)
{
	PROC *p = get_curproc();
	ulong oldmask;

	TRACE (("Psigblock(%lx)",mask));

	/* some signals (e.g. SIGKILL) can't be masked */
	mask &= ~(UNMASKABLE);

	oldmask = p->p_sigmask;
	p->p_sigmask |= mask;

	return oldmask;
}

/*
 * set the signals that we're blocking. Some signals (e.g. SIGKILL)
 * can't be masked.
 * Returns the old mask.
 */
long _cdecl
sys_p_sigsetmask (ulong mask)
{
	PROC *p = get_curproc();
	ulong oldmask;

	TRACE (("Psigsetmask(%lx)",mask));

	oldmask = p->p_sigmask;
	p->p_sigmask = mask & ~(UNMASKABLE);

	/* maybe we unmasked something */
	check_sigs ();

	return oldmask;
}

/*
 * p_sigpending: return which signals are pending delivery
 */
long _cdecl
sys_p_sigpending (void)
{
	PROC *p = get_curproc();

	TRACE (("Psigpending()"));

	/* clear out any that are going to be delivered soon */
	check_sigs ();

	/* note that signal #0 is used internally,
	 * so we don't tell the process about it
	 */
	return p->sigpending & ~1L;
}

/*
 * p_sigpause: atomically set the signals that we're blocking, then pause.
 * Some signals (e.g. SIGKILL) can't be masked.
 */
long _cdecl
sys_p_sigpause (ulong mask)
{
	PROC *p = get_curproc();
	ulong oldmask;

	TRACE(("Psigpause(%lx)", mask));

	oldmask = p->p_sigmask;
	p->p_sigmask = mask & ~(UNMASKABLE);

	if (p->sigpending & ~(p->p_sigmask))
		/* a signal is immediately pending */
		check_sigs ();
	else
		sleep (IO_Q, -1L);

	p->p_sigmask = oldmask;

	/* maybe we unmasked something */
	check_sigs();

	TRACE(("Psigpause: returning OK"));
	return E_OK;
}
