/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 */

/*
 * Copyright 1993 Linus Torvalds
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
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _mint_mdelay_h
# define _mint_mdelay_h

# include "arch/delay.h"


# ifndef NO_DELAY

/*
 * Using udelay() for intervals greater than a few milliseconds can
 * risk overflow for high loops_per_sec (high bogomips) machines. The
 * mdelay() provides a wrapper to prevent this.  For delays greater
 * than MAX_UDELAY_MS milliseconds, the wrapper is used.  Architecture
 * specific values can be defined in asm-???/delay.h as an override.
 * The 2nd mdelay() definition ensures GCC will optimize away the 
 * while loop for the common cases where n <= MAX_UDELAY_MS  --  Paul G.
 */

# ifndef MAX_UDELAY_MS
# define MAX_UDELAY_MS	5
# endif

static inline void
__mdelay (register ulong msecs)
{
	while (msecs--)
		udelay (1000);
}

# define mdelay(n) (\
	(__builtin_constant_p (n) && (n) <= MAX_UDELAY_MS) ? \
	udelay ((n) * 1000) : __mdelay (n))

# endif


# endif /* _mint_mdelay_h */
