/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
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

# ifndef _global_h
# define _global_h

# define __KERNEL_XDD__

# include "mint/mint.h"
# include "libkern/libkern.h"


/* debug section
 */

# if 1
# define DEV_DEBUG	1
# endif

# ifdef DEV_DEBUG

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x

# else

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x

# endif


/* memory allocation
 * 
 * include statistic analysis to detect
 * memory leaks
 */

extern ulong memory;

INLINE void *
own_kmalloc (register long size)
{
	register ulong *tmp;
	
	size += sizeof (*tmp);
	
	tmp = kmalloc (size);
	if (tmp)
	{
		*tmp++ = size;
		memory += size;
	}
	
	return tmp;
}

INLINE void
own_kfree (void *dst)
{
	register ulong *tmp = dst;
	
	tmp--;
	memory -= *tmp;
	
	kfree (tmp);
}

# undef kmalloc
# undef kfree

# define kmalloc	own_kmalloc
# define kfree		own_kfree


# define  COOKIE__SND 	0x5f534e44L


# endif /* _global_h */
