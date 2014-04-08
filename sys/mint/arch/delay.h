/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 * 
 * Copyright 1994 Hamish Macdonald
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
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _mint_m68k_asm_delay_h
# define _mint_m68k_asm_delay_h

# include "mint/ktypes.h"


# if __KERNEL__ == 1

/* no modul, global variable
 */
extern ulong loops_per_sec;

# else

/* modul, reference
 */
# define loops_per_sec	(*loops_per_sec_ptr)

# endif


INLINE void
__delay (register ulong loops)
{
	__asm__ __volatile__
	(
		"1: subql #1,%0; jcc 1b"
		: "=d" (loops)
		: "0" (loops)
	);
}

# ifdef __M68020__

INLINE void
udelay (register ulong usecs)
{
	register ulong tmp;
	
	usecs *= 4295;		/* 2**32 / 1000000 */
	
	__asm__
	(
		"mulul %2,%0:%1"
		: "=d" (usecs), "=d" (tmp)
		: "d" (usecs), "1" (loops_per_sec)
	);
	
	__delay (usecs);
}

# else

static inline void
udelay (register ulong usecs)
{
	__delay(usecs); /* Sigh */
}

# endif

# ifdef loops_per_sec
# undef loops_per_sec
# endif

# endif /* _mint_m68k_asm_delay_h */
