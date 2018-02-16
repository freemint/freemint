/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
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

# include "proc_wakeup.h"

# include "mint/proc.h"
# include "kmemory.h"


void _cdecl
addonprocwakeup(struct proc *p, void _cdecl (*func)(struct proc *, void *), void *arg)
{
	struct procwakeup *w;

	w = kmalloc(sizeof(*w));
	if (w)
	{
		w->next = p->wakeupthings;
		p->wakeupthings = w;

		w->func = func;
		w->p = p;
		w->arg = arg;
	}
}

void
checkprocwakeup(struct proc *p)
{
	struct procwakeup *w;

	w = p->wakeupthings;
	p->wakeupthings = NULL;

	while (w)
	{
		struct procwakeup *next = w->next;

		(*w->func)(w->p, w->arg);
		kfree(w);

		w = next;
	}
}
