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


/* Highly specialized strtol.  Convert NAME to a long value and place the
 * result in RESULT.  If NEG is non-zero then allow an optional minus
 * sign to revert the sign.  If ZEROLEAD is non-zero allow optional
 * leading zeros.  Returns 0 on success or -1 on failure.
 */ 
long _cdecl
strtonumber (const char *name, long *result, int neg, int zerolead)
{
	int sign = 1;
	const char* crs = name;
	long r = 0;
	
	if (neg && (name [0] == '-'))
	{
		crs++;
		sign = -1;
		/* "-" is an error */
		if (*crs == '\0')
			return -1;
	}
	
	if (zerolead)
	{
		while (*crs == '0')
			crs++;
		
		/* Avoid reporting success for empty string */
		if (*crs == '\0')
		{
			if (name[0] == '0')
			{
				*result = 0;
				return 0;
			}
			else
				return -1;
		}
	}
	
	if (*crs == '0' && crs[1] == '\0')
	{
		*result = 0;
		return 0;
	}
	
	if (*crs <= '0' || *crs > '9')
		return -1;
	
	while (*crs >= '0' && *crs <= '9')
	{
		r *= 10;
		r += (*crs - '0');
		if (r < 0)
			/* Overflow */
			return -1;
		crs++;
	}
	
	if (*crs != '\0')
		/* Non-digits encountered */
		return -1;
	
	*result = r * sign;
	return 0;
}
