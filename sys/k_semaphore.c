/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
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
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include "k_semaphore.h"

# include "proc.h"


# ifdef DEBUG_INFO
# define SEMA_DEBUG(x)   FORCE x
# else
# define SEMA_DEBUG(x)
# endif


void _cdecl
sema_init(struct sema *s)
{
	s->lock = 0;
	s->sleepers = 0;
}

void _cdecl
sema_lock(struct sema *s)
{
	SEMA_DEBUG(("sema_lock: enter"));
	
	while (s->lock)
	{
		SEMA_DEBUG(("sema_lock: semaphore locked, sleeping"));
		
		s->sleepers++;
		sleep(IO_Q, (long) s);
		s->sleepers--;
	}
	
	s->lock = 1;
}

void _cdecl
sema_unlock(struct sema *s)
{
	SEMA_DEBUG(("sema_unlock: enter"));
	
	assert(s->lock);
	
	s->lock = 0;
	
	if (s->sleepers)
		wake(IO_Q, (long) s);
}
