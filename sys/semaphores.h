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

# ifndef _semaphores_h
# define _semaphores_h

# include "mint/mint.h"

# ifdef DEBUG_INFO
# define SEMAPHOREDEBUG
# endif

struct semaphore
{
	volatile short lock;
	short count;
};

void	__semaphore_init(struct semaphore *s);
void	__semaphore_p	(struct semaphore *s);
void	__semaphore_v	(struct semaphore *s);

# ifdef SEMAPHOREDEBUG

void	_semaphore_init	(struct semaphore *s, const char *, long);
void	_semaphore_p	(struct semaphore *s, const char *, long);
void	_semaphore_v	(struct semaphore *s, const char *, long);

# define semaphore_init(s)	_semaphore_init(s, __FILE__, __LINE__)
# define semaphore_p(s)		_semaphore_p(s, __FILE__, __LINE__)
# define semaphore_v(s)		_semaphore_v(s, __FILE__, __LINE__)

# else /* !SEMAPHOREDEBUG */

# define semaphore_init(s)	__semaphore_init(s)
# define semaphore_p(s)		__semaphore_p(s)
# define semaphore_v(s)		__semaphore_v(s)

# endif /* SEMAPHOREDEBUG */

# endif /* _semaphores_h */
