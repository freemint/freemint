/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 * 
 * Copyright (C) 1994 Hamish Macdonald
 * Copyright (C) 2004 Greg Ungerer <gerg@uclinux.com>
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
# include "arch/timer.h"

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
# ifdef __mcoldfire__

	/* The coldfire runs this loop at significantly different speeds
	 * depending upon long word alignment or not.  We'll pad it to
	 * long word alignment which is the faster version.
	 * The 0x4a8e is of course a 'tstl %fp' instruction.  This is better
	 * than using a NOP (0x4e71) instruction because it executes in one
	 * cycle not three and doesn't allow for an arbitary delay waiting
	 * for bus cycles to finish.  Also fp/a6 isn't likely to cause a
	 * stall waiting for the register to become valid if such is added
	 * to the coldfire at some stage.
	 */
	__asm__ __volatile__ (	".balignw 4, 0x4a8e\n\t"
				"1: subql #1, %0\n\t"
				"jcc 1b"
		: "=d" (loops) : "0" (loops));

# else

	__asm__ __volatile__
	(
		"1: subql #1,%0; jcc 1b"
		: "=d" (loops)
		: "0" (loops)
	);

# endif /* (__mcoldfire__) */
}

static inline void
udelay (register ulong usecs)
{
/*
 *	loops = (usecs * loops_per_sec) / 1000000
 *	loops_per_sec = loops_per_jiffy * HZ
 *
 *	We use integer arithmetic (no floating point).
 *	Because of the (1/1000000) component loops_per_sec easily becomes
 *	zero. To solve this we multiply loops_per_sec by 2^32, product moves
 *	left by 32 bit, we get product most significant 32 bits.
 */

# if defined (__M68020__)

	register ulong tmp;
	
	usecs *= 4295;		/* 2**32 / 1000000 */
	
	__asm__
	(
		"mulul %2,%0:%1"
		: "=d" (usecs), "=d" (tmp)
		: "d" (usecs), "1" (loops_per_sec)
	);

	__delay (usecs);

# else

/*
 *	Ideally we use a 32*32->64 multiply to calculate the number of
 *	loop iterations, but the older standard 68k and ColdFire do not
 *	have this instruction. So for them we have a close approximation
 *	loop using 32*32->32 multiplies only. This calculation based on
 *	the Linux ARM old version of delay.
 *
 *	We want to implement:
 *
 *	loops = (a * b) / 2^32
 *	a = usecs * HZ * SCALE and b = loops_per_jiffy
 *
 *	loops = (usecs * SCALE * HZ * loops_per_jiffy) / 2^32
 *	SCALE = 2^32 / 1000000  ==~ 4294 == 0x10c6
 *
 *	Finally to get the best accuracy for HZ = 200 (MiNT's tick frecuency),
 *	we use 2^28 instead of 2^32.
 */

# define	HZSCALE			(268435456 / (1000000/HZ)) /* 268435456 = 2^28 */
# define	loops_per_jiffy		(loops_per_sec / HZ)

	__delay((((usecs * HZSCALE) >> 11) * (loops_per_jiffy >> 11)) >> 6);

# endif /* __M68020__ */
}

# ifdef loops_per_sec
# undef loops_per_sec
# endif

# endif /* _mint_m68k_asm_delay_h */
