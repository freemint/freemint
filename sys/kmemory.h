/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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

# ifndef _kmemory_h
# define _kmemory_h

# include "mint/mint.h"
# include "mint/mem.h"


MEMREGION *	_kmr_get	(void);
void		_kmr_free	(MEMREGION *ptr);

void *	_cdecl	_kcore		(unsigned long size, const char *func);	/* ST-RAM alloc */
void *	_cdecl	_kmalloc	(unsigned long size, const char *func);	/* TT-RAM alloc */
void	_cdecl	_kfree		(void *place, const char *func);

void *	_cdecl	_dmabuf_alloc	(unsigned long size, short cmode, const char *func);

# define kmr_get		_kmr_get
# define kmr_free		_kmr_free

# define kcore(size)		_kcore(size, FUNCTION)
# define kmalloc(size)		_kmalloc(size, FUNCTION)
# define kfree(place)		_kfree(place, FUNCTION)

# define dmabuf_alloc(size,cm)	_dmabuf_alloc(size, cm, FUNCTION)

void		init_kmemory	(void); /* initalize km allocator */
long		km_config	(long mode, long arg);

# define KM_STAT_DUMP	1
# define KM_TRACE_DUMP	2

long		km_trace_lookup	(void *ptr, char *buf, unsigned long buflen);

# endif /* _kmemory_h */
