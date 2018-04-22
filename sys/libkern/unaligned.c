/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
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


void
unaligned_putl(char *addr, long value)
{
#if !defined(__mc68020__) && !defined(__mc68030__) && !defined(__mc68040__) && !defined(__mc68060__) && !defined(__mcoldfire__)
	if ((long)addr & 1)
	{
		addr[0] = (value >> 24) & 0xff;
		addr[1] = (value >> 16) & 0xff;
		addr[2] = (value >>  8) & 0xff;
		addr[3] = (value      ) & 0xff;
	}
	else
#endif
	*(long *)addr = value;
}
