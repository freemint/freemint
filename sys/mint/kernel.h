/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 2000-05-14
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_kernel_h
# define _mint_kernel_h

# if !defined(__KERNEL__) && \
     !defined(__KERNEL_XFS__) && !defined(__KERNEL_XDD__) && \
     !defined(__KERNEL_MODULE__)
# error __KERNEL__ not defined!
# endif

# undef __KERNEL__

# if defined(__KERNEL_MODULE__)
# define __KERNEL__	3
# elif defined(__KERNEL_XFS__) || defined(__KERNEL_XDD__)
# define __KERNEL__	2
# else
# define __KERNEL__	1
# endif

# endif /* _mint_kernel_h */
