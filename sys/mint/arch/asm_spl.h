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

# ifndef _mint_m68k_asm_spl_h
# define _mint_m68k_asm_spl_h

/*
 * Normally we'd include global.h, but there are multiple global.h files.
 * This needs cleaning up. So just define directly.
 *
 * #include "global.h"
 */
extern bool coldfire_68k_emulation;

/* Called inside init.c */

static inline void
cpu_stop (void)
{
#ifdef __mcoldfire__
	if (coldfire_68k_emulation)
	{
		/* The stop instruction is currently buggy with FireTOS */
		return;
	}
#endif

	__asm__ volatile
	(
		"stop  #0x2000"
	);
}

static inline void
cpu_lpstop (void)
{
#ifndef __mcoldfire__
	/* 68060's lpstop #$2000 instruction */
	__asm__ volatile
	(
		"dc.w	0xf800,0x01c0,0x2000"
	);
#endif
}

static inline __u16
splhigh (void)
{
	register __u16 sr;
#ifdef __mcoldfire__
	register __u16 tempo;
#endif
	
	__asm__ volatile
	(
#ifdef __mcoldfire__
		"movew   %%sr,%0\n\t"
		"movew   %0,%1\n\t"
		"oril    #0x0700,%1\n\t"
		"movew   %1,%%sr"
		: "=d" (sr), "=d" (tempo)
#else
		"movew   %%sr,%0\n\t"
		"oriw    #0x0700,%%sr"
		: "=d" (sr)
#endif
	);
	
	return sr; 
}
# define spl7	splhigh

static inline void
spl (register __u16 sr)
{
	__asm__ volatile
	(
		"movew   %0,%%sr"
		:
		: "d" (sr)
	);
}


# endif /* _mint_m68k_asm_spl_h */
