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

# ifndef _mint_bswap_h
# define _mint_bswap_h

# include "ktypes.h"


static inline __u16
__const_bswap16 (register __u16 x)
{
	register __u16 r;

	r  = (x << 8) & 0xff00;
	r |= (x >> 8) & 0x00ff;

	return r;
}

static inline __u32
__const_bswap32 (register __u32 x)
{
	register __u32 r;

	r  = (x << 24) & 0xff000000;
	r |= (x <<  8) & 0x00ff0000;
	r |= (x >>  8) & 0x0000ff00;
	r |= (x >> 24) & 0x000000ff;

	return r;
}

static inline __u64
__const_bswap64 (__u64 x)
{
	__u32 *p = (__u32 *)(void *) &x;
	__u32 t;

	t = __const_bswap32 (p[0]);

	p[0] = __const_bswap32 (p[1]);
	p[1] = t;

	return x;
}


# include "arch/bswap.h"

INLINE __u16
bswap16 (register __u16 x)
{
# ifdef HAVE_ASM_BSWAP16
	return (__builtin_constant_p (x) ? __const_bswap16 (x) : __asm_bswap16 (x));
# else
	return (__const_bswap16 (x));
# endif
}

INLINE __u32
bswap32 (register __u32 x)
{
# ifdef HAVE_ASM_BSWAP32
	return (__builtin_constant_p (x) ? __const_bswap32 (x) : __asm_bswap32 (x));
# else
	return (__const_bswap32 (x));
# endif
}

INLINE __u64
bswap64 (register __u64 x)
{
# ifdef HAVE_ASM_BSWAP64
	return (__builtin_constant_p (x) ? __const_bswap64 (x) : __asm_bswap64 (x));
# else
	return (__const_bswap64 (x));
# endif
}


# define BSWAP16(x)	(bswap16 (x))
# define BSWAP32(x)	(bswap32 (x))
# define BSWAP64(x)	(bswap64 (x))


/* compatibility definition */
# define SWAP68_W(x)	(BSWAP16 (x))
# define SWAP68_L(x)	(BSWAP32 (x))


# endif /* _mint_bswap_h */
