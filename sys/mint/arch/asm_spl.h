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

# ifndef _m68k_asm_spl_h
# define _m68k_asm_spl_h


static inline __u16
splhigh (void)
{
	register __u16 sr;
	
	__asm__ volatile
	(
		"movew sr, %0;"
		"oriw  #0x0700, sr"
		: "=d" (sr)
	);
	
	return sr; 
}
# define spl7	splhigh

static inline void
spl (register __u16 sr)
{
	__asm__ volatile
	(
		"movew %0, sr"
		:
		: "d" (sr)
	);
}


# endif /* _m68k_asm_spl_h */
