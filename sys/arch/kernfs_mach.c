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
 * begin:	2001-05-08
 * last change:	2001-05-08
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "kernfs_mach.h"

# if WITH_KERNFS

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
	char *cpu, *mmu, *fpuname;
	ulong clockfreq, clockfactor;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	cpu = "68000";
	fpuname = mmu = "none";

	clockfactor = 0;

	switch (mcpu)
	{
		case 10:
			cpu = "68010";
			break;
		case 20:
			cpu = "68020";
			clockfactor = 8;
			break;
		case 30:
			cpu = mmu = "68030";
			clockfactor = 8;
			break;
		case 40:
			cpu = mmu = "68040";
			clockfactor = 3;
			break;
		case 60:
			cpu = mmu = "68060";
			clockfactor = 1;
			break;

		/* Add more processors here */

		default:
			cpu = "680x0";
			break;
	}
	
	if (fpu)
	{
		switch (fputype >> 16)
		{
			case 0x02:
				fpuname = "68881/82";
				break;
			case 0x04:
				fpuname = "68881";
				break;
			case 0x06:
				fpuname = "68882";
				break;
			case 0x08:
				fpuname = "68040";
				break;
			case 0x10:
				fpuname = "68060";
				break;
			default:
				fpuname = "680x0";
				break;
		}
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
				cpu, mmu, fpuname,
		  		clockfreq / 1000000, (clockfreq / 100000) % 10,
		   		loops_per_sec / 500000, (loops_per_sec / 5000) % 100,
		   		loops_per_sec
	);
	
	*buffer = info;
	return 0;
}

# endif /* WITH_KERNFS */
