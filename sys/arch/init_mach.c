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

# include "init_mach.h"

# include "arch/detect.h"
# include "arch/info_mach.h"
# include "arch/mprot.h"
# include "libkern/libkern.h"

# include "cookie.h"
# include "global.h"
# include "init.h"

# include <mint/osbind.h>


static void init_info_mach (void);

/*
 * Get the value of the _MCH cookie, if one exists; also set no_mem_prot if
 * there's a _CPU cookie and you're not on an '030, or if there is none.
 * This must be done in a separate routine because the machine type and CPU
 * type are needed when initializing the system, whereas install_cookies is
 * not called until everything is practically up.
 * 
 * In fact, getmch() should be called before *anything* else is
 * initialized, so that if we find a MiNT cookie already in the
 * jar we can bail out early and painlessly.
 */

long
getmch (void)
{
	COOKIE *jar = *CJAR;
	extern short gl_kbd;
	
	/* own CPU test */
	mcpu = detect_cpu ();
	
	/* own FPU test; this must be done after the CPU detection */
	fputype = detect_fpu ();
	
	if ((fputype >> 16) > 1)
		fpu = 1;
	
	DEBUG (("detecting hardware ... "));
	/* at the moment only detection of ST-ESCC */
	if (mcpu < 40 && detect_hardware ())
		boot_print ("ST-ESCC extension detected\r\n");
	DEBUG (("ok!\r\n"));
	
	if (jar)
	{
		while (jar->tag != 0)
		{
			/* check for machine type */
			if (jar->tag == COOKIE__MCH)
			{
				mch = jar->value;
# ifdef MILAN
				if (mch != MILAN_C)
				{
					boot_print ("This MiNT version requires a Milan!\r\n");
					boot_print ("Hit any key to continue.\r\n");
					(void) Cconin ();
					Pterm0 ();
				}
# else
				if (mch == MILAN_C)
				{
					boot_print ("This MiNT version doesn't run on a Milan!\r\n");
					boot_print ("Hit any key to continue.\r\n");
					(void) Cconin ();
					Pterm0 ();
				}
# endif
			}
			else if (jar->tag == COOKIE__VDO)
			{
				FalconVideo = (jar->value == 0x00030000L);
				ste_video = (jar->value == 0x00010000L);
				if (jar->value & 0xffff0000L)
					screen_boundary = 15;
			}
			else if (jar->tag == COOKIE_MiNT)
			{
				boot_print ("MiNT is already installed!!\r\n");
				Pterm (2);
			}
			else if (jar->tag == COOKIE__AKP)
			{
				gl_lang = (int) ((jar->value >> 8) & 0x00ff);
				gl_kbd = (short)(jar->value & 0x00ffL);
			}
			else if (jar->tag == COOKIE_PMMU)
			{
				/* jr: if PMMU cookie exists, someone else is
				 * already using the PMMU
				 */
				boot_print ("WARNING: PMMU is already in use!\r\n");
				no_mem_prot = 1;
			}
			
			jar++;
		}
	}
	
# ifndef MMU040
	if (mcpu != 30)
		no_mem_prot = 1;
# endif
	
	/*
	 * if no preference found, look at the country code to decide
	 */
	if (gl_lang < 0)
	{
		long *sysbase;
		int i;
		
		sysbase = *((long **)(0x4f2L)); /* gets the RAM OS header */
		sysbase = (long *)sysbase[2];	/* gets the ROM one */
		
		i = (int) ((sysbase[7] & 0x7ffe0000L) >> 17L);
		
		switch (i)
		{
			case 1:		/* Germany */
			case 8:		/* Swiss German */
				gl_lang = 1;
				break;
			case 2:		/* France */
			case 7:		/* Swiss French */
				gl_lang = 2;
				break;
			case 4:		/* Spain */
				gl_lang = 4;
				break;
			case 5:		/* Italy */
				gl_lang = 5;
				break;
			default:
				gl_lang = 0;
				break;
		}
	}
	
	if (gl_lang >= MAXLANG || gl_lang < 0)
		gl_lang = 0;
	
	/* initialize the info strings */
	init_info_mach ();
	
	return 0L;
}

static void
init_info_mach (void)
{
	switch (mch)
	{
		case ST:
			machine = "ATARI ST";
			break;
		case STE:
			machine = "ATARI STE";
			break;
		case MEGASTE:
			machine = "ATARI MegaSTE";
			break;
		case TT:
			machine = "ATARI TT";
			break;
		case FALCON:
			machine = "ATARI Falcon";
			break;
		case MILAN_C:
			machine = "Milan";
			break;
	}
	
	switch (mcpu)
	{
		case 10:
			cpu_model = "68010";
			break;
		case 20:
			cpu_model = "68020";
			break;
		case 30:
			cpu_model = mmu_model = "68030";
			break;
		case 40:
			cpu_model = mmu_model = "68040";
			break;
		case 60:
			cpu_model = mmu_model = "68060";
			break;
	}
	
	if (fpu)
	{
		switch (fputype >> 16)
		{
			case 0x02:
				fpu_model = "68881/82";
				break;
			case 0x04:
				fpu_model = "68881";
				break;
			case 0x06:
				fpu_model = "68882";
				break;
			case 0x08:
				fpu_model = "68040";
				break;
			case 0x10:
				fpu_model = "68060";
				break;
		}
	}
}
