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


# define TIMESTEN(x)	((((x) << 2) + (x)) << 1)

/* converts a decimal string to an integer
 */
long _cdecl
_mint_atol (const char *s)
{
	long d = 0;
	int negflag = 0;
	int c;
	
	while (*s && isspace (*s))
		s++;
	
	while (*s == '-' || *s == '+')
	{
		if (*s == '-')
			negflag ^= 1;
		s++;
	}
	
	while ((c = *s++) != 0 && isdigit (c))
	{
		d = TIMESTEN (d) + (c - '0');
	}
	
	if (negflag)
		d = -d;
	
	return d;
}
