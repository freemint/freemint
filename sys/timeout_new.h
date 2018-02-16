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
 * Started: 2000-06-28
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _timeout_new_h
# define _timeout_new_h

# include "mint/mint.h"
# include "mint/lists.h"

# include "arch/timer.h"


/* timer granularity in ms */
# define EVTGRAN	1

# define GETTIME()	(*hz_200)
# define DIFTIME(o,n)	(((n) - (o))/(EVTGRAN / 5))

struct event
{
	long		delta;
	LIST_ENTRY(event) chain;
	ushort		flags;
	ushort		res1;
	void		(*func)(struct proc *, long);
	struct proc	*proc;
	long		arg;
	long		res2[2];
};

void	event_add	(struct event *, long, void (*)(struct proc *, long), long);
void	event_delete	(struct event *);
long	event_delta	(struct event *);
void	event_reset	(struct event *, long);


# endif /* _timeout_new_h */
