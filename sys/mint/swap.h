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
 * Started: 2000-04-18
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_swap_h
# define _mint_swap_h

# include "ktypes.h"


static inline __u32
__const_swap32 (register __u32 x)
{
	register __u32 r;
	
	r  = (x << 16) & 0xffff0000;
	r |= (x >> 16) & 0x0000ffff;
	
	return r;
}


# include "arch/swap.h"

static inline __u32
swap32 (register __u32 x)
{
# ifdef HAVE_ASM_SWAP32
	return (__builtin_constant_p (x) ? __const_swap32 (x) : __asm_swap32 (x));
# else
	return (__const_swap32 (x));
# endif
}


# include "bswap.h"

# define SWAP16(x)	(BSWAP16 (x))
# define SWAP32(x)	(swap32 (x))


# endif /* _mint_swap_h */
