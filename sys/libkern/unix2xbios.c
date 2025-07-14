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

# if 0
# define LEAPS_THRU_END_OF(y)	((y) / 4)
# else
# define LEAPS_THRU_END_OF(y)	((y) / 4 - (y) / 100 + (y) / 400)
# endif

# ifdef __isleap
# undef __isleap
# endif

# define __isleap(year)		(((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0)))

/* Convert Unix time into calendar format
 */
void _cdecl
unix2calendar64(time64_t tv_sec, ushort *year, ushort *month, ushort *day, ushort *hour, ushort *minute, ushort *second)
{
	long days, rem, y;

	days = tv_sec / SECS_PER_DAY;
	rem = tv_sec % SECS_PER_DAY;

	if (hour)
		*hour = (ushort)(rem / SECS_PER_HOUR);

	rem %= SECS_PER_HOUR;

	if (minute)
		*minute = (ushort)(rem / 60);
	if (second)
		*second = (ushort)(rem % 60);

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

	if (year)
		*year = (ushort)y;

	{
		const ushort *ip;

		ip = __mon_yday [__isleap (y)];
		for (y = 11; days < ip [y]; --y)
			continue;

		days -= ip [y];
	}

	if (month)
		*month = (ushort)y + 1;
	if (day)
		*day = (ushort)days + 1;
}

long _cdecl
unix2xbios(time32_t tv_sec)
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

	DOSTIME xtm_struct;
	DOSTIME *xtm = &xtm_struct;
	ushort year, month, day, hour, minute, second;

	/*
	 * do not sign extend it, to allow for years >= 2038,
	 * since this is a file timestamp from old DOS-style filesystems
	 * and therefor will never be from before 1970
	 */
	unix2calendar64((__u32)tv_sec, &year, &month, &day, &hour, &minute, &second);

	xtm->hour = hour;
	xtm->minute = minute;
	xtm->sec2 = (second >> 1);
	xtm->year = (year - 1980);
	xtm->month = month;
	xtm->day = day;

	return *(long *)xtm;
}

/* EOF */
