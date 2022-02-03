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

# include "timeout.h"

# include "arch/timer.h"
# include "mint/asm.h"

# include "dosdir.h"
# include "kmemory.h"
# include "proc.h"
# include "time.h"


# define TIMEOUTS		64	/* # of static timeout structs */
# define TIMEOUT_USED		0x01	/* timeout struct is in use */
# define TIMEOUT_STATIC		0x02	/* this is a static timeout */

/* This gets implizitly initialized to zero, thus the flags are
 * set up correctly.
 */
static TIMEOUT timeouts [TIMEOUTS];
TIMEOUT *tlist = NULL;
TIMEOUT *expire_list = NULL;

/* Number of ticks after that an expired timeout is considered to be old
 * and disposed automatically.
 */
# define TIMEOUT_EXPIRE_LIMIT	400	/* 2 secs */

static TIMEOUT *
newtimeout (short fromlist)
{
	if (!fromlist)
	{
		register TIMEOUT *t;
		
		t = kmalloc (sizeof (*t));
		if (t)
		{
			t->flags = 0;
			t->arg = 0;
			return t;
		}
	}
	
	{
		register long i;
		register short sr;
		
		sr = spl7 ();
		for (i = 0; i < TIMEOUTS; i++)
		{
			if (!(timeouts [i].flags & TIMEOUT_USED))
			{
				timeouts [i].flags |= (TIMEOUT_STATIC|TIMEOUT_USED);
				spl (sr);
				timeouts [i].arg = 0;
				return &timeouts [i];
			}
		}
		spl (sr);
	}
	
	return 0;
}

static void
disposetimeout (TIMEOUT *t)
{
	if (t->flags & TIMEOUT_STATIC) t->flags &= ~TIMEOUT_USED;
	else kfree (t);
}

static void
dispose_old_timeouts (void)
{
	register TIMEOUT *t, **prev, *old;
	register long now = *hz_200;
	register short sr = spl7 ();
	
	for (prev = &expire_list, t = *prev; t; prev = &t->next, t = *prev)
	{
		if (t->when < now)
		{
			/* This and the following timeouts are too old.
			 * Throw them away.
			 */
			
			*prev = 0;
			spl (sr);
			while (t)
			{
				old = t;
				t = t->next;
				disposetimeout (old);
			}
			return;
		}
	}
	
	spl (sr);
}

static void
inserttimeout (TIMEOUT *t, long delta)
{
	register TIMEOUT **prev, *cur;
	register short sr = spl7 ();
	
	cur = tlist;
	prev = &tlist;
	while (cur)
	{
		if (cur->when >= delta)
		{
			cur->when -= delta;
			t->next = cur;
			t->when = delta;
			*prev = t;
			spl(sr);
			return;
		}
		delta -= cur->when;
		prev = &cur->next;
		cur = cur->next;
	}
	
	assert (delta >= 0);
	t->when = delta;
	t->next = cur;
	*prev = t;
	
	spl (sr);
}

/*
 * addtimeout(long delta, void (*func)()): schedule a timeout for the current
 * process, to take place in "delta" milliseconds. "func" specifies a
 * function to be called at that time; the function is passed as a parameter
 * the process for which the timeout was specified (i.e. the value of
 * curproc at the time addtimeout() was called; note that this is probably
 * *not* the current process when the timeout occurs).
 *
 * NOTE: if kernel memory is low, newtimeout() will try to get a statically
 * allocated timeout struct (fallback method).
 */

static TIMEOUT *
__addtimeout (PROC *p, long delta, void _cdecl (*func)(PROC *, long arg), ushort flags)
{
	TIMEOUT *t;
	
	{
		register TIMEOUT **prev;
		register ushort sr;
		
		sr = spl7 ();
		
		/* Try to reuse an already expired timeout that had the
		 * same function attached
		 */
		prev = &expire_list;
		for (t = *prev; t != NULL; prev = &t->next, t = *prev)
		{
			if (t->proc == p && t->func == func)
			{
				*prev = t->next;
				spl(sr);
				inserttimeout(t, delta);
				return t;
			}
		}
		
		spl (sr);
	}
	
	t = newtimeout (flags & 1);
	
	if (t)
	{
		t->proc = p;
		t->func = func;
		inserttimeout (t, delta);
	}
	
	return t;
}

TIMEOUT * _cdecl
addtimeout (PROC *p, long delta, void _cdecl (*func)(PROC *, long arg))
{
	return __addtimeout (p, delta, func, 0);
}

TIMEOUT * _cdecl
addtimeout_curproc (long delta, void _cdecl (*func)(PROC *, long arg))
{
	return __addtimeout (get_curproc(), delta, func, 0);
}

/*
 * addroottimeout(long delta, void (*)(PROC *), short flags);
 * Same as addtimeout(), except that the timeout is attached to Pid 0 (MiNT).
 * This means the timeout won't be cancelled if the process which was
 * running at the time addroottimeout() was called exits.
 *
 * Currently only bit 0 of `flags' is used. Meaning:
 * Bit 0 set: Call from interrupt (cannot use kmalloc, use statically
 *	allocated `struct timeout' instead).
 * Bit 0 clear: Not called from interrupt, can use kmalloc.
 *
 * Thus addroottimeout() can be called from interrupts (bit 0 of flags set),
 * which makes it *extremly* useful for device drivers.
 * A serial device driver would make an addroottimeout(0, check_keys, 1)
 * if some bytes have arrived.
 * check_keys() is then called at the next context switch, can use all
 * the kernel functions and can do time cosuming jobs.
 */

TIMEOUT * _cdecl
addroottimeout (long delta, void _cdecl (*func)(PROC *, long arg), ushort flags)
{
	return __addtimeout (rootproc, delta, func, flags);
}

/*
 * cancelalltimeouts(): cancels all pending timeouts for the current
 * process
 */

void _cdecl
cancelalltimeouts (void)
{
	TIMEOUT *cur, **prev, *old;
	long delta;
	short sr = spl7 ();

	cur = tlist;
	prev = &tlist;
	while (cur)
	{
		if (cur->proc == get_curproc())
		{
			delta = cur->when;
			old = cur;
			*prev = cur = cur->next;
			if (cur) cur->when += delta;
			spl (sr);
			disposetimeout (old);
			sr = spl7();
			
			/* ++kay: just in case an interrupt handler installed a
			 * timeout right after `prev' and before `cur'
			 */
			cur = *prev;
		}
		else
		{
			prev = &cur->next;
			cur = cur->next;
		}
	}
	
	prev = &expire_list;
	for (cur = *prev; cur; cur = *prev)
	{
		if (cur->proc == get_curproc())
		{
			*prev = cur->next;
			spl (sr);
			disposetimeout (cur);
			sr = spl7 ();
		}
		else
			prev = &cur->next;
	}
	
	spl (sr);
}

/*
 * Cancel a specific timeout. If the timeout isn't on the list, or isn't
 * for this process, we do nothing; otherwise, we cancel the time out
 * and then free the memory it used. *NOTE*: it's very possible (indeed
 * likely) that "this" was already removed from the list and disposed of
 * by the timeout processing routines, so it's important that we check
 * for it's presence in the list and do absolutely nothing if we don't
 * find it there!
 */

static void
__canceltimeout (TIMEOUT *this, struct proc *p)
{
	TIMEOUT *cur, **prev;
	short sr = spl7 ();
	
	/* First look at the list of expired timeouts */
	prev = &expire_list;
	for (cur = *prev; cur; cur = *prev)
	{
		if (cur == this && cur->proc == p)
		{
			*prev = cur->next;
			spl (sr);
			disposetimeout (this);
			return;
		}
		prev = &cur->next;
	}

	prev = &tlist;
	for (cur = tlist; cur; cur = cur->next)
	{
		if (cur == this && cur->proc == p)
		{
			*prev = cur->next;
			if (cur->next)
			{
				cur->next->when += this->when;
			}
			spl (sr);
			disposetimeout (this);
			return;
		}
		prev = &cur->next;
	}
	
	spl (sr);
}

void _cdecl
canceltimeout (TIMEOUT *this)
{
	__canceltimeout (this, get_curproc());
}

void _cdecl
cancelroottimeout (TIMEOUT *this)
{
	__canceltimeout (this, rootproc);
}

/*
 * timeout: called every 20 ms or so by GEMDOS, this routine
 * is responsible for maintaining process times and such.
 * it should also decrement the "proc_clock" variable, but
 * should *not* take any action when it reaches 0 (the state of the
 * stack is too uncertain, and time is too critical). Instead,
 * a vbl routine checks periodically and if "proc_clock" is 0
 * suspends the current process
 */

volatile int our_clock = 1000;

/* moved to intr.spp (speed reasons)
 * was called by mint_timer()
 */
# if 0
void _cdecl
timeout (void)
{
	register short ms;
	
	c20ms++;
	
	kintr = keyrec->head != keyrec->tail;
	
	if (proc_clock)
		proc_clock--;
	
	ms = *((short *) 0x442L);
	our_clock -= ms;
	
	if (tlist)
		tlist->when -= ms;
}
# endif

/*
 * sleep() calls this routine to check on alarms and other sorts
 * of time-outs on every context switch.
 */

void
checkalarms (void)
{
	register ushort sr;
	register long delta;
	
	/* do the once per second things */
	while (our_clock < 0)
	{
		our_clock += 1000;
		
		/* Updates timestamp and datestamp. */
		synch_timers ();
		
		searchtime++;
		reset_priorities ();
	}
	
	sr = spl7 ();
	
	/* see if there are outstanding timeout requests to do */
	while (tlist && ((delta = tlist->when) <= 0))
	{
		/* hack: pass an extra long as args, those intrested in it will
		 * need a cast and have to place it in t->arg themselves but
		 * that way everything else still works without change -nox
		 */
		register long args __asm__("d0") = tlist->arg;
		register PROC *p __asm__("a0") = tlist->proc;
		to_func *evnt = tlist->func;
		register TIMEOUT *old = tlist;
		
		tlist = tlist->next;
		
		/* if delta < 0, it's possible that the time has come for the
		 * next timeout to occur.
		 * ++kay: moved this before the timeout fuction is called, in
		 * case the timeout function installes a new timeout.
		 */
		if (tlist)
			tlist->when += delta;
		
		old->next = expire_list;
		old->when = *(long *) 0x4ba + TIMEOUT_EXPIRE_LIMIT;
		expire_list = old;
		
		spl (sr);
		
		/* ++kay: debug output at spl7 hangs the system, so moved it
		 * here
		 */
		TRACE (("doing timeout code for pid %d", p->pid));
		
		/* call the timeout function */
		/*
		 * take care to call it in a way that works both for cdecl
		 * and Pure-C calling conventions, since there seem
		 * to be drivers around that were compiled by it.
		 */
		__asm__ __volatile__(
			"\tmove.l %1,-(%%a7)\n"
			"\tmove.l %0,-(%%a7)\n"
			"\tjsr (%2)\n"
#ifdef __mcoldfire__
			"\taddq.l #8,%%a7\n"
#else
			"\taddq.w #8,%%a7\n"
#endif
		: /* no outputs */
		: "a"(p), "d"(args), "a"(evnt)
		: "d1", "d2", "a1", "a2", "cc", "memory");
		
		sr = spl7 ();
	}
	
	spl (sr);
	
	/* Now look at the expired timeouts if some are getting old */
	dispose_old_timeouts ();
}

/*
 * nap(n): nap for n milliseconds. Used in loops where we're waiting for
 * an event. If we expect the event *very* soon, we should use yield
 * instead.
 * NOTE: we may not sleep for exactly n milliseconds; signals can wake
 * us earlier, and the vagaries of process scheduling may cause us to
 * oversleep...
 */

static void _cdecl
unnapme (PROC *p, long arg)
{
	register short sr = spl7 ();
	
	if (p->wait_q == SELECT_Q && p->wait_cond == (long) nap)
	{
		rm_q (SELECT_Q, p);
		add_q (READY_Q, p);
		p->wait_cond = 0;
	}
	
	spl (sr);
}

void _cdecl 
nap (unsigned n)
{
	TIMEOUT *t;
	
	t = addtimeout (get_curproc(), n, unnapme);
	sleep (SELECT_Q, (long) nap);
	canceltimeout (t);
}
