/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
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

# include "semaphores.h"

# include "proc.h"


#if 0

void _cdecl
_semaphore_init(struct semaphore *s, const char *info)
{
	DEBUG(("semaphore_init((0x%lx) from %s", s, info));

	s->lock = 0;
	s->sleepers = 0;
	s->pid = 0;
	s->pad = 0;
}

void _cdecl
_semaphore_lock(struct semaphore *s, const char *info)
{
	short pid = curproc->pid;

	DEBUG(("want semaphore(0x%lx) from %s for %u", s, info, pid));

	if (s->lock && s->pid != pid)
	{
		s->sleepers++;

		while (s->lock)
			sleep(WAIT_Q, (long)s);

		s->sleepers--;
	}

	/* enter semaphore */
	s->lock++;
	s->pid = pid;

	DEBUG(("semaphore(0x%lx) locked from %s for %u", s, info, s->pid));
}

int _cdecl
_semaphore_rel(struct semaphore *s, const char *info)
{
	short pid = curproc->pid;

	DEBUG(("semaphore(0x%lx) released from %s for %u", s, info, pid));

	/* check for correct usage */
	assert(s->lock && s->pid == pid);

	/* exit semaphore */
	s->lock--;

	/* if completed wakup any waiter */
	if (!s->lock)
	{
		if (s->sleepers)
			wake(WAIT_Q, (long)s);

		/* completed, semaphore free */
		return 1;
	}

	/* not completed */
	return 0;
}

int _cdecl
_semaphore_try(struct semaphore *s, const char *info)
{
	short pid = curproc->pid;

	DEBUG(("try semaphore(0x%lx) from %s for %u", s, info, pid));

	if (s->lock && s->pid != pid)
	{
		DEBUG(("semaphore(0x%lx) in use from %s for %u", s, info, s->pid));
		return 0;
	}

	/* enter semaphore */
	s->lock++;

	DEBUG(("semaphore(0x%lx) locked from %s for %u", s, info, s->pid));
	return 1;
}

#endif
