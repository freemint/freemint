/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * from Henry Spencer's stringlib, modified by ERS, by Guido.
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
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
 */

# include "libkern.h"


/*
 * strchr - find first occurrence of a character in a string
 * 
 * return found char, or NULL if none
 */
char * _cdecl
_mint_strchr(const char *s, long charwanted)
{
	char c;

	/*
	 * The odd placement of the two tests is so NUL is findable.
	 */
	while ((c = *s++) != (char) charwanted)
		if (c == 0)
			return NULL;

	return ((char *) --s);
}
