/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992,1994 Eric R. Smith.
 * All rights reserved.
 */

/* dossig.c:: dos signal handling routines */

# include "dossig.h"

# include "mint/asm.h"
# include "mint/signal.h"

# include "bios.h"
# include "kmemory.h"
# include "mfp.h"
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
p_kill (int pid, int sig)
{
	PROC *p;
	long r;
	
	TRACE (("Pkill(%d, %d)", pid, sig));
	if (sig < 0 || sig >= NSIG)
	{
		DEBUG (("Pkill: signal out of range"));
		return EBADARG;
	}
	
	if (pid < 0)
		r = killgroup (-pid, sig, 0);
	else if (pid == 0)
		r = killgroup (curproc->pgrp, sig, 0);
	else
	{
		p = pid2proc (pid);
		if (p == 0 || p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
		{
			DEBUG (("Pkill: pid %d not found", pid));
			return ENOENT;
		}
		
		if (curproc->euid && curproc->ruid != p->ruid)
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
p_sigaction (int sig, const struct sigaction *act, struct sigaction *oact)
{
	TRACE (("Psigaction(%d)", sig));
	
	if (sig < 1 || sig >= NSIG)
		return EBADARG;
	if (act && (sig == SIGKILL || sig == SIGSTOP))
		return EACCES;
	
	if (oact)
	{
		oact->sa_handler = curproc->sighandle[sig];
		oact->sa_mask = curproc->sigextra[sig];
		oact->sa_flags = curproc->sigflags[sig] & SAUSER;
	}
	
	if (act)
	{
		ushort flags;

		curproc->sighandle[sig] = act->sa_handler;
		curproc->sigextra[sig] = act->sa_mask & ~UNMASKABLE;

		/* only the flags in SAUSER can be changed by the user */
		flags = curproc->sigflags[sig] & ~SAUSER;
		flags |= act->sa_flags & SAUSER;
		curproc->sigflags[sig] = flags;
 
		/* various special things that should happen */
		if (act->sa_handler == SIG_IGN)
		{
			/* discard pending signals */
			curproc->sigpending &= ~(1L << sig);
		}

		/* I dunno if this is right, but bash seems to expect it */
 		curproc->sigmask &= ~(1L << sig);
	}
	
	return E_OK;
}

/*
 * set a user-specified signal handler
 */

long _cdecl
p_signal (int sig, long handler)
{
	long oldhandle;

	TRACE (("Psignal(%d, %lx)", sig, handler));
	
	if (sig < 1 || sig >= NSIG)
		return EBADARG;
	if (sig == SIGKILL || sig == SIGSTOP)
		return EACCES;
	
	oldhandle = curproc->sighandle[sig];
	curproc->sighandle[sig] = handler;
	curproc->sigextra[sig] = 0;
	curproc->sigflags[sig] = 0;
	
	/* various special things that should happen */
	if (handler == SIG_IGN)
	{
		/* discard pending signals */
		curproc->sigpending &= ~(1L<<sig);
	}

	/* I dunno if this is right, but bash seems to expect it */
	curproc->sigmask &= ~(1L<<sig);

	return oldhandle;
}

/*
 * block some signals. Returns the old signal mask.
 */

long _cdecl
p_sigblock(ulong mask)
{
	ulong oldmask;

	TRACE (("Psigblock(%lx)",mask));
	
	/* some signals (e.g. SIGKILL) can't be masked */
	mask &= ~(UNMASKABLE);
	
	oldmask = curproc->sigmask;
	curproc->sigmask |= mask;
	
	return oldmask;
}

/*
 * set the signals that we're blocking. Some signals (e.g. SIGKILL)
 * can't be masked.
 * Returns the old mask.
 */

long _cdecl
p_sigsetmask (ulong mask)
{
	ulong oldmask;

	TRACE (("Psigsetmask(%lx)",mask));
	
	oldmask = curproc->sigmask;
	curproc->sigmask = mask & ~(UNMASKABLE);
	
	/* maybe we unmasked something */
	check_sigs ();
	
	return oldmask;
}

/*
 * p_sigpending: return which signals are pending delivery
 */

long _cdecl
p_sigpending (void)
{
	TRACE (("Psigpending()"));
	
	/* clear out any that are going to be delivered soon */
	check_sigs ();

	/* note that signal #0 is used internally,
	 * so we don't tell the process about it
	 */
	return curproc->sigpending & ~1L;
}

/*
 * p_sigpause: atomically set the signals that we're blocking, then pause.
 * Some signals (e.g. SIGKILL) can't be masked.
 */

long _cdecl
p_sigpause (ulong mask)
{
	ulong oldmask;

	TRACE(("Psigpause(%lx)", mask));
	oldmask = curproc->sigmask;
	curproc->sigmask = mask & ~(UNMASKABLE);
	if (curproc->sigpending & ~(curproc->sigmask))
		check_sigs();	/* a signal is immediately pending */
	else
		sleep(IO_Q, -1L);
	curproc->sigmask = oldmask;
	check_sigs();	/* maybe we unmasked something */
	TRACE(("Psigpause: returning OK"));
	return E_OK;
}

/*
 * p_sigintr: Set an exception vector to send us the specified signal.
 *
 * BUG:
 *	does not link vectors using XBRA.
 * CONSOLATION:
 *	XBRA is not so useful in this case anyways... :)
 */

typedef struct usig {
	ushort vec;		/* exception vector number */
	ushort sig;		/* signal to send */
	PROC *proc;		/* process to get signal */
	long oldv;		/* old exception vector value */
	struct usig *next;	/* next entry ... */
} usig;

static usig *usiglst;

long _cdecl
p_sigintr (ushort vec, ushort sig)
{
	extern void new_intr();		/* in intr.spp */
	extern long intr_shadow;
	long vec2;
	usig *new;
	
	/* This function needs long stack frames,
	 * hence it only works on 68020+, sorry.
	 */
	
	if (*(ushort *) 0x0000059e == 0 || !intr_shadow)
		return ENOSYS;
	
	if (sig >= NSIG)		/* ha! */
		return EBADARG;
	
	if (vec && !sig)		/* ignore signal 0 */
		return E_OK;
	
	if (!sig)			/* remove handlers on Psigintr(0,0) */
	{
		cancelsigintrs ();
		return E_OK;
	}
	
	/* only autovectors ($60-$7c), traps ($80-$bc) and user defined
	 * interrupts ($0100-$03fc) are allowed, others already generate
         * signals anyway, no? :)
	 */

	if (vec < 0x0018 || vec > 0x00ff)
		return EBADARG;

	if (vec > 0x002f && vec < 0x0040)
		return EBADARG;

	/* Filter out uninitialized interrupts and odd garbage */

	vec2 = vec<<2;
	vec2 = *(long *)vec2;

	if (!vec2 || (vec2 & 1L))
		return ENXIO;

	/* okay, okay, will install, if you wanna so much... */

	vec2 = (long) new_intr;

	new = kmalloc (sizeof (*new));
	if (!new)			/* hope this never happens...! */
		return ENOMEM;
	new->vec = vec;
	new->sig = sig;
	new->proc = curproc;
	new->next = usiglst;		/* simple unsorted list... */
	usiglst = new;

	new->oldv = setexc(vec, vec2);
	return E_OK;
}

/*
 * Find the process that requested this interrupt, and send it a signal.
 * Called at interrupt time by new_intr() from intr.spp, with interrupt
 * vector number on the stack.
 */

void
sig_user (ushort vec)
{
	usig *ptr;

	for (ptr = usiglst; ptr; ptr=ptr->next)
	{
		if (vec == ptr->vec)
		{
			if (ptr->proc->wait_q != ZOMBIE_Q
				&& ptr->proc->wait_q != TSR_Q)
			{
				post_sig(ptr->proc, ptr->sig);
			}
		}
	}
}

/*
 * cancelsigintrs: remove any interrupts requested by this process,
 * called at process termination.
 */

void
cancelsigintrs (void)
{
	usig *ptr, **old, *nxt;
	short s;
	
	s = spl7();
	for (old = &usiglst, ptr = usiglst; ptr; )
	{
		nxt = ptr->next;
		if (ptr->proc == curproc)
		{
			setexc (ptr->vec, ptr->oldv);
			*old = nxt;
			kfree (ptr);
		}
		else
		{
			old = &(ptr->next);
		}
		ptr = nxt;
	}
	spl(s);
}
