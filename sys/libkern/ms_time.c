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


/* ms_time (ms, timeptr):
 * 
 * convert a time in milliseconds to a GEMDOS style date/time
 * timeptr[0] gets the time, timeptr[1] the date.
 * BUG/FEATURE: in the conversion, it is assumed that all months have
 * 30 days and all years have 360 days.
 */

void _cdecl
ms_time (ulong ms, short int *timeptr)
{
	register ulong secs = ms;
	register short tsec, tmin, thour;
	register short tday, tmonth, tyear;

	secs /= 1000;
	tsec = secs % 60;
	
	secs /= 60;		/* secs now contains # of minutes */
	tmin = secs % 60;
	
	secs /= 60;		/* secs now contains # of hours */
	thour = secs % 24;
	
	secs /= 24;		/* secs now contains # of days */
	tday = secs % 30;
	
	secs /= 30;
	tmonth = secs % 12;
	tyear = secs / 12;
	
	*timeptr++ = (thour << 11) | (tmin << 5) | (tsec >> 1);
	*timeptr = (tyear << 9) | ((tmonth + 1) << 5) | (tday + 1);
}
