/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 1998-05-01
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _unicode_h
# define _unicode_h

# include "mint/mint.h"


extern uchar *t_uni2atari[256];
extern uchar  t_atari2uni[256];


void init_unicode(void);


/*
 * uni2atari
 * ---------
 * Transform a Unicode character - if possible - in an Atari ST character.
 * Other characters are replaced by a '?'. Also swap bytes from Intel words.
 *
 * off: offset codepage	- wchar[0]
 * cp: codepage		- wchar[1]
 *
 *
 * atari2uni
 * ---------
 * Transform an Atari ST character to an Unicode character. Also swap bytes to
 * Intel words.
 *
 * dst: pointer to 2 bytes free space for the Unicode character
 *
 */

INLINE char
UNI2ATARI (register uchar off, register uchar cp)
{
	register char *codepage = (char *)t_uni2atari[cp];
	return (codepage == NULL) ? '?' : codepage[off];
}

INLINE void
ATARI2UNI (register uchar atari_st, register uchar *dst)
{
	if (atari_st & 0x80)
	{
		atari_st &= 0x7f;
		atari_st <<= 1;
		dst[1] = t_atari2uni[atari_st]; atari_st++;
		dst[0] = t_atari2uni[atari_st];
	}
	else
	{
		dst[0] = atari_st;
		dst[1] = 0x00;
	}
}


# endif /* _unicode_h */
