/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 1998-09-10
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _kmemory_h
# define _kmemory_h

# include "mint/mint.h"
# include "mint/mem.h"


MEMREGION *	_kmr_get	(void);
void		_kmr_free	(MEMREGION *ptr);

void *	_cdecl	_kcore		(ulong size, const char *func);	/* ST-RAM alloc */
void *	_cdecl	_kmalloc	(ulong size, const char *func);	/* TT-RAM alloc */
void	_cdecl	_kfree		(void *place, const char *func);
void *	_cdecl	_umalloc	(ulong size, const char *func);	/* user space alloc */
void	_cdecl	_ufree		(void *place, const char *func);/* user space free */


void	init_kmemory		(void);		/* initalize km allocator */
long	km_config		(long mode, long arg);

# define KM_STAT_DUMP	1


# define kmr_get		_kmr_get
# define kmr_free		_kmr_free

# define kcore(size)		_kcore (size, __FUNCTION__)
# define kmalloc(size)		_kmalloc (size, __FUNCTION__)
# define kfree(place)		_kfree (place, __FUNCTION__)
# define umalloc(size)		_umalloc (size, __FUNCTION__)
# define ufree(place)		_ufree (place, __FUNCTION__)


# endif /* _kmemory_h */
