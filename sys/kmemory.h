/*
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
 * begin:	1998-09-10
 * last change:	1998-
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
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

void *	_cdecl	_kcore		(ulong size);	/* ST-RAM alloc */
void *	_cdecl	_kmalloc	(ulong size);	/* TT-RAM alloc */
void	_cdecl	_kfree		(void *place);
void *	_cdecl	_umalloc	(ulong size);	/* user space alloc */
void	_cdecl	_ufree		(void *place);	/* user space free */


void	init_kmemory		(void);		/* initalize km allocator */
long	km_config		(long mode, long arg);

# define KM_STAT_DUMP	1


# define kmr_get		_kmr_get
# define kmr_free		_kmr_free


# if 1
# define kcore			_kcore
# else
INLINE void *
__kcore (ulong size, char *file, long line)
{
	void *ptr = _kcore (size);
	TRACE (("%s, %ld: kcore (%lu -> %lx) called!", file, line, size, ptr));
	return ptr;
}
# define kcore			__kcore (size, __FILE__, __LINE__)
# endif

# if 1
# define kmalloc		_kmalloc
# else
INLINE void *
__kmalloc (ulong size, char *file, long line)
{
	void *ptr;
	TRACE (("%s, %ld: kmalloc (%lu) called!", file, line, size));
	ptr = _kmalloc (size);
	TRACE (("%s, %ld: kmalloc return %lx.", file, line, ptr));
	return ptr;
}
# define kmalloc(size)		__kmalloc (size, __FILE__, __LINE__)
# endif


# if 1
# define kfree			_kfree
# else
INLINE void
__kfree (void *place, char *file, long line)
{
	TRACE (("%s, %ld: kfree (%lx) called!", file, line, place));
	return _kfree (place);
}
# define kfree(place)		__kfree (place, __FILE__, __LINE__)
# endif


# if 1
# define umalloc		_umalloc
# else
INLINE void *
__umalloc (ulong size, char *file, long line)
{
	void *ptr = _umalloc (size);
	TRACE (("%s, %ld: umalloc (%lu -> %lx) called!", file, line, size, ptr));
	return ptr;
}
# define umalloc(size)		__umalloc (size, __FILE__, __LINE__)
# endif


# if 1
# define ufree			_ufree
# else
INLINE void
__ufree (void *place, char *file, long line)
{
	TRACE (("%s, %ld: ufree (%lx) called!", file, line, place));
	return _ufree (place);
}
# define ufree(place)		__ufree (place, __FILE__, __LINE__)
# endif


# endif /* _kmemory_h */
