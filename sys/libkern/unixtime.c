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


/* unixtime (time, date):
 * 
 * convert a Dos style (time, date) pair into
 * a Unix time (seconds from midnight Jan 1., 1970)
 */
static int const
mth_start[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

long _cdecl
unixtime (ushort time, ushort date)
{
	register int sec, min, hour;
	register int mday, mon, year;
	register long s;
	
	sec	= (time & 31) << 1;
	min	= (time >> 5) & 63;
	hour	= (time >> 11) & 31;
	
	mday	= date & 31;
	mon	= ((date >> 5) & 15) - 1;
	year	= 80 + ((date >> 9) & 127);
	
	/*
	 * Dates after 1.1.2098 would overflow,
	 * because DOS baseyear is 1980,
	 * but unix epoch is 1970
	 */
	if (year >= 198)
		return 4294967296UL - MAX_TZ_OFFSET;

	/* calculate tm_yday here */
	s = (mday - 1) + mth_start[mon] + /* leap year correction */
		(((year % 4) != 0 ) ? 0 : (mon > 1));

	s = (sec) + (min * 60L) + (hour * 3600L) +
		(s * 86400L) + ((year - 70) * 31536000L) +
		((year - 69) / 4) * 86400L;
	
	return s;
}
