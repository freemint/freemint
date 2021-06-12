/*
 *	This file provides a timer with 10 ms granularity. We must
 *	have our own routines for this, because the Mint builtin
 *	timer does not provide for an argument to the timeout function.
 *
 *	03/22/94, Kay Roemer.
 */

# include "global.h"

# include "timer.h"
# include "mint/time.h"


static struct event *allevents = 0;
static struct timeout *nexttimeout = 0;

static void
update_head (void)
{
	static long last = 0;
	static short init = 1;
	long diff, curr = GETTIME ();
	
	if (init || !allevents || curr - last < 0)
	{
		last = curr;
		init = 0;
	}
	else
	{
		diff = DIFTIME (last, curr);
		allevents->delta -= diff;
		last += diff * (EVTGRAN / 5);
	}
}

# if 0
/* I don't see a reason for that */

static char stack[8192];

INLINE void *
setstack (register void *sp)
{
	register void *osp __asm__("d0") = 0;
	
	__asm__ volatile
	(
		"movel sp,%0;"
		"movel %2,sp;"
		: "=a" (osp)
		: "0" (osp), "a" (sp)
	);
	
	return osp;
}
# endif

static void
check_events (PROC *proc, long arg2)
{
	register struct event *ep;
	register void (*func)(long);
	register long arg;
# if 0
	register void *sp;
	
	sp = setstack (stack + sizeof (stack));
# endif
	
	update_head ();
	while ((ep = allevents) && ep->delta <= 0)
	{
		arg = ep->arg;
		func = ep->func;
		allevents = ep->next;
		if (allevents)
			allevents->delta += ep->delta;
		ep->func = 0;
		(*func)(arg);
		update_head ();
	}
	
	if (ep)
	{
		nexttimeout = addroottimeout (ep->delta * EVTGRAN, check_events, 0);
		if (!nexttimeout)
			FATAL ("timer: out of kernel memory");
	}
	else
		nexttimeout = 0;
	
# if 0
	setstack (sp);
# endif
}

static void
event_insert (struct event *ep, long delta)
{
	struct event **prev, *curr;
	
	if (delta <= 0)
	{
		void (*func)(long) = ep->func;
		ep->func = 0;
		(*func)(ep->arg);
		return;
	}
	
	update_head ();
	prev = &allevents;
	curr = allevents;
	for (; curr; prev = &curr->next, curr = curr->next)
	{
		if (curr->delta <= delta)
			delta -= curr->delta;
		else
		{
			curr->delta -= delta;
			break;
		}
	}
	ep->delta = delta;
	ep->next = curr;
	*prev = ep;
	
	if (allevents == ep)
	{
		if (nexttimeout)
			cancelroottimeout (nexttimeout);
		
		nexttimeout = addroottimeout (delta * EVTGRAN, check_events, 0);
		if (!nexttimeout)
			FATAL ("timer: out of kernel memory");
	}
}

static short
event_remove (struct event *ep)
{
	struct event **prev, *curr;
	
	prev = &allevents;
	curr = allevents;
	for (; curr; prev = &curr->next, curr = curr->next)
	{
		if (ep == curr)
		{
			*prev = curr->next;
			if (curr->next)
				curr->next->delta += curr->delta;
			
			return 1;
		}
	}
	
	return 0;
}

void
event_add (struct event *ep, long delta, void (*func)(long), long arg)
{
	if (ep->func)
		event_remove (ep);
	
	ep->func = func;
	ep->arg = arg;
	
	event_insert (ep, delta);
}

void
event_del (struct event *ep)
{
	if (ep->func)
	{
		event_remove (ep);
		ep->func = 0;
	}
}

/*
 * return the time in ms until the event `ep' happens
 */
long
event_delta (struct event *ep)
{
	struct event *cur;
	long ticks = 0;
	
	update_head ();
	for (cur = allevents; cur; cur = cur->next)
	{
		ticks += cur->delta;
		if (cur == ep)
			return ticks;
	}
	
	DEBUG (("event_delta: event not found"));
	return 0;
}

/*
 * Reset an event that is already in the queue to a new timeout value.
 * We spot the common case where the old timeout is smaller than the
 * new one and try to be as fast as possible in this case.
 */
void
event_reset (struct event *ep, long delta)
{
	struct event *curr, **prev;
	long ticks = 0;
	
	update_head ();
	prev = &allevents;
	for (curr = *prev; curr; prev = &curr->next, curr = *prev)
	{
		ticks += curr->delta;
		if (curr == ep)
		{
			if (delta == ticks)
				return;
			break;
		}
	}
	
	if (!curr)
		FATAL ("event_reset: event not found");

	*prev = curr->next;
	if (curr->next)
		curr->next->delta += curr->delta;

	if (delta > ticks)
	{
		curr = *prev;
		delta -= ticks - ep->delta;
		for (; curr; prev = &curr->next, curr = curr->next)
		{
			if (curr->delta <= delta)
				delta -= curr->delta;
			else
			{
				curr->delta -= delta;
				break;
			}
		}
		ep->delta = delta;
		ep->next = curr;
		*prev = ep;
	}
	else
		event_insert (ep, delta);
}
