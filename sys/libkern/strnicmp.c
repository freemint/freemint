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
 * Case insensitive string comparison. Note that this only returns
 * 0 (match) or nonzero (no match), and that the returned value
 * is not a reliable indicator of any "order".
 */

long _cdecl
_mint_strnicmp (const char *str1, const char *str2, long len)
{
	register char c1, c2;

	do {
		c1 = *str1++; if (isupper(c1)) c1 = _tolower(c1);
		c2 = *str2++; if (isupper(c2)) c2 = _tolower(c2);
	}
	while (--len >= 0 && c1 && c1 == c2);
	
	if (len < 0 || c1 == c2)
		return 0;
	
	return (long) (c1 - c2);
}
