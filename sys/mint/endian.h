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
 * begin:	2000-04-18
 * last change:	2000-04-18
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_endian_h
# define _mint_endian_h

# include "ktypes.h"
# include "bswap.h"
# include "arch/endian.h"


# if BYTE_ORDER == BIG_ENDIAN


# define le2cpu16(x)	(bswap16 (x))
# define le2cpu32(x)	(bswap32 (x))
# define le2cpu64(x)	(bswap64 (x))

# define be2cpu16(x)	((__u16) (x))
# define be2cpu32(x)	((__u32) (x))
# define be2cpu64(x)	((__u64) (x))

# define cpu2le16(x)	(bswap16 (x))
# define cpu2le32(x)	(bswap32 (x))
# define cpu2le64(x)	(bswap64 (x))

# define cpu2be16(x)	((__u16) (x))
# define cpu2be32(x)	((__u32) (x))
# define cpu2be64(x)	((__u64) (x))


# elif BYTE_ORDER == LITTLE_ENDIAN


# define le2cpu16(x)	((__u16) (x))
# define le2cpu32(x)	((__u32) (x))
# define le2cpu64(x)	((__u64) (x))

# define be2cpu16(x)	(bswap16 (x))
# define be2cpu32(x)	(bswap32 (x))
# define be2cpu64(x)	(bswap64 (x))

# define cpu2le16(x)	((__u16) (x))
# define cpu2le32(x)	((__u32) (x))
# define cpu2le63(x)	((__u64) (x))

# define cpu2be16(x)	(bswap16 (x))
# define cpu2be32(x)	(bswap32 (x))
# define cpu2be64(x)	(bswap64 (x))


# else

# error unknown BYTE_ORDER

# endif


# define ntohs(x)	(be2cpu16 (x))
# define ntohl(x)	(be2cpu32 (x))
# define htons(x)	(cpu2be16 (x))
# define htonl(x)	(cpu2be32 (x))


# endif /* _mint_endian_h */
