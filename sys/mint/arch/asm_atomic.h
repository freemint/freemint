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

# ifndef _mint_m68k_asm_atomic_h
# define _mint_m68k_asm_atomic_h


// TAS
// CAS2

static inline long
compare_and_swap (volatile long *p, long old, long new)
{
	register long read;
	register char ret;
	
	__asm__ __volatile__
	(
		"cas%.l %2,%3,%1\n"
		"seq %0"
		: "=dm" (ret), "=m" (*p), "=d" (read)
		: "d" (new), "m" (*p), "2" (old)
	);
	
	return ret;
}


# endif /* _mint_m68k_asm_atomic_h */
