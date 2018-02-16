/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 * 
 * 
 * Copyright 1991, 1992  Linus Torvalds
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
 * Started: 2000-07-17
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include "delay.h"

# include "libkern/libkern.h"
# include "arch/timer.h"
# include "mint/delay.h"

# include "timeout.h"


/* this should be approx 2 Bo*oMips to start (note initial shift), and will
 * still work even if initially too large, it will just take slightly longer
 */
ulong loops_per_sec;

/* This is the number of bits of precision for the loops_per_second. Each
 * bit takes on average 1.5/HZ seconds.  This (like the original) is a little
 * better than 1%
 */
# define LPS_PREC 8

void
calibrate_delay (void)
{
# ifndef NO_DELAY
	register ulong ticks, loopbit;
	register long lps_precision = LPS_PREC;
	
	loops_per_sec = (1UL << 12);
	
	while (loops_per_sec <<= 1)
	{
		/* wait for "start of" clock tick */
		ticks = jiffies;
		while (ticks == jiffies)
			/* nothing */;
		
		/* Go .. */
		ticks = jiffies;
		__delay (loops_per_sec);
		ticks = jiffies - ticks;
		if (ticks)
			break;
	}
	
	/* Do a binary approximation to get loops_per_second set to equal one
	 * clock (up to lps_precision bits)
	 */
	loops_per_sec >>= 1;
	loopbit = loops_per_sec;
	while (lps_precision-- && (loopbit >>= 1))
	{
		loops_per_sec |= loopbit;
		ticks = jiffies;
		while (ticks == jiffies);
		ticks = jiffies;
		__delay (loops_per_sec);
		if (jiffies != ticks)	/* longer than 1 tick */
			loops_per_sec &= ~loopbit;
	}
	
	/* finally, adjust loops per second in terms of seconds instead of
	 * clocks
	 */
	loops_per_sec *= HZ;
	
# if 0
	/* recalibrate every minute */
	addroottimeout (60 * 1000L, (void _cdecl (*)(PROC *)) calibrate_delay, 0);
	
# ifdef DEBUG_INFO
	{
		char buf[128];
		ksprintf (buf, sizeof (buf), "Recalibrate delay: %lu.%02lu BogoMIPS",
			(loops_per_sec + 2500) / 500000,
			((loops_per_sec + 2500) / 5000) % 100);
		DEBUG ((buf));
	}
# endif
# endif
# endif
}
