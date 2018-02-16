/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
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
 * Started: 2001-05-08
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "kernfs_mach.h"

# if WITH_KERNFS

# include "arch/info_mach.h"
# include "libkern/libkern.h"

# include "delay.h"
# include "global.h"
# include "kmemory.h"


/* This is mostly stolen from linux-m68k and should be adapted to MiNT
 */
long 
kern_get_cpuinfo (SIZEBUF **buffer)
{
	SIZEBUF *info;
	int len = 256;
	ulong clockfreq, clockfactor;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	clockfactor = 0;

#ifdef __mcoldfire__
	if (!coldfire_68k_emulation)
	{
		clockfactor = 2; // Experimentally almost accurate
	}
	else
#endif	
	if (is_apollo_68080)
	{
		clockfactor = 2; // Experimentally almost accurate
	}
	else switch (mcpu)
	{
		case 20:
			clockfactor = 8;
			break;
		case 30:
			clockfactor = 8;
			break;
		case 40:
			clockfactor = 3;
			break;
		case 60:
			clockfactor = 1;
			break;

		/* Add more processors here */
	}
	
	clockfreq = loops_per_sec * clockfactor;

	if (mcpu <= 10)
	{
		/* Assume 8 MHz ST
		 */
		
		clockfreq = 8 * 1000000UL;
		loops_per_sec = 83 * 5000UL;
	}

	info->len = ksprintf (info->buf, len,
				"CPU:\t\t%s\n"
				"MMU:\t\t%s\n"
				"FPU:\t\t%s\n"
		   		"Clocking:\t%lu.%1luMHz\n"
				"BogoMIPS:\t%lu.%02lu\n"
		   		"Calibration:\t%lu loops\n",
				cpu_type, mmu_type, fpu_type,
		  		clockfreq / 1000000, (clockfreq / 100000) % 10,
		   		loops_per_sec / 500000, (loops_per_sec / 5000) % 100,
		   		loops_per_sec
	);
	
	*buffer = info;
	return 0;
}

# endif /* WITH_KERNFS */
