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
 * begin:	2000-04-17
 * last change:	2000-04-17
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * known bugs:
 * 
 * todo:
 * 
 * optimizations:
 * 
 */

# include "libkern.h"


/*
 * zero out a block of memory, quickly; the block must be word-aligned,
 * and should be long-aligned for speed reasons
 */

void
_mint_bzero (void *dst, unsigned long size)
{
	register char *place = dst;
	register ulong cruft;
	register ulong blocksize;

	if ((cruft = (unsigned long)((long)place & 3))) {
		size -= cruft;
		while (cruft--)
			*place++ = '\0';
	}

	cruft = size % 256;	/* quickzero does 256 byte blocks */
	blocksize = size / 256;	/* divide by 256 */
	if (blocksize)
	{
		quickzero (place, blocksize);
		place += (blocksize * 256);
	}
	
	while (cruft)
	{
		*place++ = '\0';
		cruft--;
	}
}
