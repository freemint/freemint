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


/* unix2xbios (tv_sec):
 * 
 * convert a Unix time into a DOS/XBIOS time
 * the date word first, then the time word as it's expected by the XBIOS
 */

/* How many days come before each month (0-11) */
static const ushort __mon_yday [2][12] =
{
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};

# define SECS_PER_HOUR		(60L * 60L)
# define SECS_PER_DAY		(SECS_PER_HOUR * 24L)

# define LEAPS_THRU_END_OF(y)	((y) / 4)

# ifdef __isleap
# undef __isleap
# endif
/* Lucky enough that 2000 is a leap year.  */
# define __isleap(year)		((year % 4) == 0)

long _cdecl
unix2xbios (long tv_sec)
{
	typedef struct dostime DOSTIME;
	struct dostime
	{
		unsigned year: 7;
		unsigned month: 4;
		unsigned day: 5;
		unsigned hour: 5;
		unsigned minute: 6;
		unsigned sec2: 5;
	};
	
	DOSTIME dostime;
	DOSTIME *xtm = &dostime;
	long days, rem, y;
	
	
	days = tv_sec / SECS_PER_DAY;
	rem = tv_sec % SECS_PER_DAY;
	
	xtm->hour = rem / SECS_PER_HOUR;
	
	rem %= SECS_PER_HOUR;
	xtm->minute = rem / 60;
	xtm->sec2 = (rem % 60) >> 1;
	
	
	y = 1970;
	while (days < 0 || days >= (__isleap (y) ? 366 : 365))
	{
		/* Guess a corrected year, assuming 365 days per year */
		long yg = y + days / 365 - (days % 365 < 0);
		
		/* Adjust DAYS and Y to match the guessed year */
		days -= ((yg - y) * 365
			+ LEAPS_THRU_END_OF (yg - 1)
			- LEAPS_THRU_END_OF (y - 1));
		y = yg;
	}
	
	xtm->year = y - 1980;
	
	{	register const ushort *ip;
		
		ip = __mon_yday [__isleap (y)];
		for (y = 11; days < ip [y]; --y)
			continue;
		
		days -= ip [y];
	}
	
	xtm->month = y + 1;
	xtm->day = days + 1;
	
	return *(long *) xtm;
}
