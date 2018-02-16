/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
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
 * Started: 2001-01-16
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * This file provides a timer with 10 ms granularity. We must
 * have our own routines for this, because the Mint builtin
 * timer does not provide for an argument to the timeout function.
 * 
 */

# include "timeout_new.h"

# include "mint/file.h"
# include "mint/time.h"

# include "timeout.h"


static void	update_head	(void);
static void	check_events	(struct proc *);
static void	event_insert	(struct event *, long);
static void	event_remove	(struct event *);

static LIST_HEAD(, event) allevents = LIST_HEAD_INITIALIZER(allevents);
static struct timeout *nexttimeout = 0;

static void
update_head (void)
{
	static long last = 0;
	static short init = 1;
	long diff, curr = GETTIME ();
	
	if (init || !LIST_FIRST(&allevents) || curr - last < 0)
	{
		last = curr;
		init = 0;
	}
	else
	{
		diff = DIFTIME (last, curr);
		LIST_FIRST(&allevents)->delta -= diff;
		last += diff * (EVTGRAN / 5);
	}
}

static void
check_events (struct proc *p)
{
	register struct event *ep;
	
	update_head ();
	while ((ep = LIST_FIRST(&allevents)) && ep->delta <= 0)
	{
		register void (*func)(struct proc *, long);
		register struct proc *proc;
		register long arg;
		
		arg = ep->arg;
		proc = ep->proc;
		func = ep->func;
		
		/* remove from list */
		LIST_FIRST(&allevents) = LIST_NEXT(ep, chain);
		
		/* update delta */
		if (!LIST_EMPTY(&allevents))
			LIST_FIRST(&allevents)->delta += ep->delta;
		
		/* invalidate */
		ep->func = 0;
		
		/* callout */
		(*func)(proc, arg);
		
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
}

static void
event_insert (struct event *ep, long delta)
{
	struct event *curr;
	
	if (delta <= 0)
	{
		void (*func)(struct proc *, long);
		
		func = ep->func;
		ep->func = 0;
		
		/* callout */
		(*func)(ep->proc, ep->arg);
		
		return;
	}
	
	update_head ();
	
	LIST_FOREACH (curr, &allevents, chain)
	{
		if (curr->delta <= delta)
		{
			delta -= curr->delta;
			if (!LIST_NEXT (curr, chain))
			{
				LIST_INSERT_AFTER (curr, ep, chain);
				break;
			}
		}
		else
		{
			curr->delta -= delta;
			LIST_INSERT_BEFORE (curr, ep, chain);
			break;
		}
	}
	
	ep->delta = delta;
	
	if (LIST_FIRST (&allevents) == ep)
	{
		if (nexttimeout)
			cancelroottimeout (nexttimeout);
		
		nexttimeout = addroottimeout (delta * EVTGRAN, check_events, 0);
		if (!nexttimeout)
			FATAL ("timer: out of kernel memory");
	}
}

static inline void
event_remove (struct event *ep)
{
	LIST_REMOVE (ep, chain);
}

void
event_add (struct event *ep, long delta, void (*func)(struct proc *, long), long arg)
{
	if (ep->func)
		event_remove (ep);
	
	ep->func = func;
	ep->arg = arg;
	
	event_insert (ep, delta);
}

void
event_delete (struct event *ep)
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
	
	LIST_FOREACH (cur, &allevents, chain)
	{
		ticks += cur->delta;
		if (cur == ep)
			return ticks;
	}
	
	DEBUG (("event_delta: event not found"));
	return ticks;
}

/*
 * Reset an event that is already in the queue to a new timeout value.
 * We spot the common case where the old timeout is smaller than the
 * new one and try to be as fast as possible in this case.
 */
void
event_reset (struct event *ep, long delta)
{
	struct event *curr, *next;
	long ticks = 0;
	
	update_head ();
	
	LIST_FOREACH (curr, &allevents, chain)
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
	
	next = LIST_NEXT (curr, chain);
	if (next)
		next->delta += curr->delta;
	
	LIST_REMOVE (curr, chain);
	
	if (delta > ticks)
	{
		delta -= ticks - ep->delta;
		
		for (curr = next; curr; curr = LIST_NEXT (curr, chain))
		{
			if (curr->delta <= delta)
			{
				delta -= curr->delta;
				if (!LIST_NEXT (curr, chain))
				{
					LIST_INSERT_AFTER (curr, ep, chain);
					break;
				}
			}
			else
			{
				curr->delta -= delta;
				LIST_INSERT_BEFORE (curr, ep, chain);
				break;
			}
		}
		
		ep->delta = delta;
	}
	else
		event_insert (ep, delta);
}
