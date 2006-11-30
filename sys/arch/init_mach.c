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
# include "libkern/libkern.h"

# include "arch/aranym.h"
# include "arch/detect.h"
# include "arch/info_mach.h"
# include "arch/mprot.h"
# include "arch/tosbind.h"

# include "cookie.h"
# include "global.h"
# include "init.h"
# include "kerinfo.h"


/*
 * _MCH cookie is not exact anymore
 * (special hades cookie, special ct60 cookie, special aranym cookie)
 *
 * XXX todo: we should replace the global mch variable with a more
 * accurate enum
 */
enum special_hw
{
	none = 0,
	hades,
	ct60
# ifdef ARANYM
	,
	aranym
# endif
};

static long _getmch (void);
static void identify (enum special_hw);


long
getmch (void)
{
	return TRAP_Supexec(_getmch);
}


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
static long
_getmch (void)
{
	enum special_hw add_info = none;
	struct cookie *jar;

	jar = *CJAR;
	if (jar)
	{
		while (jar->tag != 0)
		{
			switch (jar->tag)
			{
				case COOKIE__MCH:
				{
					mch = jar->value;
# ifdef MILAN
					if (mch != MILAN_C)
					{
						boot_print ("This MiNT version requires a Milan!\r\n");
						return -1;
					}
# else
					if (mch == MILAN_C)
					{
						boot_print ("This MiNT version doesn't run on a Milan!\r\n");
						return -1;
					}
# endif
					break;
				}
				
				case COOKIE__VDO:
				{
					FalconVideo = (jar->value == 0x00030000L);
					ste_video = (jar->value == 0x00010000L);
					break;
				}
				
				case COOKIE_MiNT:
				{
					boot_print ("MiNT is already installed!!\r\n");
					return -1;
				}
				
				case COOKIE__AKP:
				{
					gl_kbd = (short)(jar->value & 0x00ffL);
					gl_lang = (short)((jar->value >> 8) & 0x00ff);
					break;
				}
				
				case COOKIE_PMMU:
				{
					/* jr: if PMMU cookie exists, someone else is
					 * already using the PMMU
					 */
					boot_print ("WARNING: PMMU is already in use!\r\n");
					no_mem_prot = 1;
					break;
				}
				
				case COOKIE_HADES:
				{
					add_info = hades;
					break;
				}
				
				case COOKIE_CT60:
				{
					add_info = ct60;
					break;
				}
				
# ifdef ARANYM
				case COOKIE_NF:
				{
					kernelinfo.nf_ops = nf_init();
					
					add_info = aranym;
					break;
				}
# endif
			}
			
			jar++;
		}
	}
	
	/* own CPU test */
	mcpu = detect_cpu ();
	/* own FPU test; this must be done after the CPU detection */
	fputype = detect_fpu ();
	
	if ((fputype >> 16) > 1)
		fpu = 1;
	
# ifndef M68000

# ifdef M68030
	if( mcpu > 30 )	// we can't use 030 kernel on 040/060 anymore
	{
		boot_print ("\r\nThis version of MiNT requires a 68020-68030.\r\n");
# else /* M68030 */
	if( mcpu < 40 )
	{
		boot_print ("\r\nThis version of MiNT requires a 68040-68060.\r\n");
# endif /* M68030 */

		return -1;
	}
	
	/* Ozk: If protect_page0 == 0, we check if we should tell the pmmu code
	 *	to set SUPER on the first descriptor by placing a value of 1 here.
	 *	The pmmu code checks for a value of 1 here in which case it sets
	 *	SUPER. Any other value will make the pmmu code use existing
	 *	translation tables unmodified. The idea here is to make this
	 *	configurable someway, just setting protect_page0 to 2, for example
	 *	will disable this whole check. That is unimplemented yet. I think
	 *	this must be a boot option, since pmmu code intializes things
	 *	long before mint.cnf is read->parsed... or?
	 */
	if (protect_page0 == 0 && (mch == MILAN_C || add_info == hades))
	{
		boot_print("Hardware needs SUPER on first descriptor!\r\n");
		protect_page0 = 1;
	}
# endif /* !M68000 */
	
	/* initialize the info strings */
	identify (add_info);
	
	DEBUG (("detecting hardware ... "));
	/* at the moment only detection of ST-ESCC */
	if (mcpu < 40 && detect_hardware ())
		boot_print ("ST-ESCC extension detected\r\n");
	DEBUG (("ok!\r\n"));
	
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

	return 0;
}

static void
identify (enum special_hw info)
{
	char buf[64];
	char *_cpu, *_mmu, *_fpu;
	
	machine = "Unknown clone";
	
	switch (info)
	{
		case none:
		{
			switch (mch)
			{
				case ST:
					machine = "Atari ST";
					break;
				case STE:
					machine = "Atari STE";
					break;
				case MEGASTE:
					machine = "Atari MegaSTE";
					break;
				case TT:
					machine = "Atari TT";
					break;
				case FALCON:
					machine = "Atari Falcon";
					break;
				case MILAN_C:
					machine = "Milan";
					break;
			}
			break;
		}
		case hades:
		{
			machine = "Hades";
			break;
		}
		case ct60:
		{
			machine = "Atari Falcon/CT60";
			break;
		}
# ifdef ARANYM
		case aranym:
		{
			machine = nf_name();
			break;
		}
# endif
	}
	
	_fpu = " no ";
	
	if (fpu)
	{
		switch (fputype >> 16)
		{
			case 0x02:
				fpu_type = "68881/82";
				_fpu = " 68881/82 ";
				break;
			case 0x04:
				fpu_type = "68881";
				_fpu = " 68881 ";
				break;
			case 0x06:
				fpu_type = "68882";
				_fpu = " 68882 ";
				break;
			case 0x08:
				fpu_type = "68040";
				_fpu = "/";
				break;
			case 0x10:
				fpu_type = "68060";
				_fpu = "/";
				break;
		}
	}
	
	_cpu = "m68k";
	_mmu = "";
	
	switch (mcpu)
	{
		case 0:
			cpu_type = "68000";
			_cpu = cpu_type;
			_mmu = "";
			break;
		case 10:
			cpu_type = "68010";
			_cpu = cpu_type;
			_mmu = "";
			break;
		case 20:
			cpu_type = "68020";
			_cpu = cpu_type;
			_mmu = "";
			break;
		case 30:
			cpu_type = mmu_type = "68030";
			_cpu = cpu_type;
			_mmu = "/MMU";
			break;
		case 40:
			cpu_type = mmu_type = "68040";
			_cpu = cpu_type;
			_mmu = "/MMU";
			break;
		case 60:
		{
			ulong pcr;
			
			__asm__
			(
				".word 0x4e7a,0x0808;"
				"movl %%d0,%0"
				: "=d"(pcr)
				:
				: "d0"
			);
			
			ksprintf (buf, sizeof (buf), "68%s060 rev.%ld",
					pcr & 0x10000 ? "LC/EC" : "",
					(pcr >> 8) & 0xff);
			
			cpu_type = mmu_type = "68060";
			_cpu = buf;
			_mmu = "/MMU";
			break;
		}
	}
	
	ksprintf (cpu_model, sizeof (cpu_model), "%s (%s CPU%s%sFPU)",
			machine, _cpu, _mmu, _fpu);
	
	boot_printf ("%s\r\n\r\n", cpu_model);
}
