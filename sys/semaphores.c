/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
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
 */

# include "semaphores.h"

# include "proc.h"


void
__semaphore_init(struct semaphore *s)
{
	s->count = 0;
}

void
__semaphore_p(struct semaphore *s)
{
	s->count++;
	
	while (s->lock)
		sleep(WAIT_Q, (long)s);
	
	/* enter semaphore */
	s->lock = 1;
}

void
__semaphore_v(struct semaphore *s)
{
	s->count--;
	
	if (s->count)
		wake(WAIT_Q, (long)s);
	
	/* exit semaphore */
	s->lock = 0;
}

# ifdef SEMAPHOREDEBUG

void
_semaphore_init(struct semaphore *s, const char *file, long line)
{
	DEBUG(("semaphore_init((0x%lx) from %s, %li", s, file, line));
	
	__semaphore_init(s);
}

void
_semaphore_p(struct semaphore *s, const char *file, long line)
{
	DEBUG(("want semaphore(0x%lx) from %s, %li", s, file, line));
	
	__semaphore_p(s);
	
	DEBUG(("semaphore(0x%lx) aquired", s));
}

void
_semaphore_v(struct semaphore *s, const char *file, long line)
{
	DEBUG(("semaphore(0x%lx) released from %s, %li", s, file, line));
	
	__semaphore_v(s);
}

# endif /* SEMAPHOREDEBUG */
