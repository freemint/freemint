/*
 * Filename:     bitmap.c
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@fishpool.com).
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@cs.uni-magdeburg.de)
 * 
 * Portions copyright 1992, 1993, 1994, 1995 Remy Card (card@masi.ibp.fr)
 * and 1991, 1992 Linus Torvalds (torvalds@klaava.helsinki.fi)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include "bitmap.h"


ulong
ext2_count_free (char *map, ulong numchars)
{
	static long nibblemap [] = { 4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0 };
	
	register ulong i;
	register ulong sum = 0;
	
	if (!map) 
		return 0;
	
	for (i = 0; i < numchars; i++)
	{
		sum += nibblemap[(map [i]     ) & 0xf];
		sum += nibblemap[(map [i] >> 4) & 0xf];
	}
	
	return sum;
}
