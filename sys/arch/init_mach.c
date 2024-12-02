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

# include "arch/detect.h"
# include "arch/info_mach.h"
# include "arch/mprot.h"
# include "arch/tosbind.h"
# include "arch/native_features.h"

# include "cookie.h"
# include "global.h"
# include "init.h"
# include "kerinfo.h"
# include "kentry.h"


/*
 * _MCH cookie is not exact anymore
 * (special hades cookie, special ct2/ct60 cookie, special aranym cookie)
 */
enum special_hw
{
	none = 0,
	hades,
	ct2,
	ct60,
	raven
# ifdef WITH_NATIVE_FEATURES
	,
	emulator
# endif
};

static long _getmch (void);
static void identify (long mch, enum special_hw);


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
	long mch = 0;
	struct cookie *jar;
#ifdef WITH_MMU_SUPPORT
	int old_no_mem_prot = no_mem_prot;
#endif
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

#ifdef WITH_MMU_SUPPORT
				case COOKIE_PMMU:
				{
					/* jr: if PMMU cookie exists, someone else is
					 * already using the PMMU
					 */
					boot_print ("WARNING: PMMU is already in use!\r\n");
					no_mem_prot = 1;
					break;
				}
#endif

				case COOKIE_HADES:
				{
					add_info = hades;
					break;
				}
				
				case COOKIE__CT2:
				{
					add_info = ct2;
					break;
				}

				case COOKIE_CT60:
				{
					add_info = ct60;
					break;
				}

				case COOKIE_RAVN:
				{
					add_info = raven;
				} break;

#ifdef __mcoldfire__
				case COOKIE__CPU:
				{
					coldfire_68k_emulation = true;
					break;
				}
#endif

				case COOKIE__5MS:
				{
					/* pointer to vector used by 200 Hz system timer */
					p5msvec = (long *)jar->value;
					break;
				}
			}

			jar++;
		}
	}
	
# ifdef WITH_NATIVE_FEATURES
	/* unconditionally detect native features
	 * (the emulator / underlying OS doesn't have to provide a __NF cookie)
	 */
	kernelinfo.nf_ops = nf_init();
	kentry.vec_mch.nf_ops = nf_init();

	if (kernelinfo.nf_ops)
		add_info = emulator;
# endif

	/* own CPU test */
	mcpu = detect_cpu();
	/* own FPU test; this must be done after the CPU detection */
	fputype = detect_fpu();
#ifndef WITH_68080
	/* own SFP-004 test */
	/* ignore on machines which cannot have an sfp and also cannot bus error on the tested address */
	if (add_info != raven)
	{
		sfptype = detect_sfp();
	
		if ((sfptype >> 16) > 1)
		    fputype |= 0x00010000;	// update _FPU cookie with the SFP-004 bit
	}
#endif

	if ((fputype >> 16) > 1)	// coprocessor mode only
		fpu = 1;

#ifdef WITH_MMU_SUPPORT
	if ((add_info == ct60 || add_info == ct2) && !old_no_mem_prot)
	{
		// HACK: PMMU cookie is for some reason set on CT2/CT60
		// so make sure we set the old value as intended
		no_mem_prot = 0;
	}

	if (!(mcpu & 0x10000UL))
		pmmu = detect_pmmu ();
	if (!no_mem_prot && !pmmu)
	{
		FORCE ("WARNING: PMMU is requested but not present, disabling.\r\n");
		no_mem_prot = 1;
	}
#endif

	mcpu &= 0xffffUL;

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

	/* Odd Skancke:
	 *	If protect_page0 == 0, we check if we should tell the pmmu code
	 *	to set SUPER on the first descriptor by placing a value of 1 here.
	 *	The pmmu code checks for a value of 1 here in which case it sets
	 *	SUPER. Any other value will make the pmmu code use existing
	 *	translation tables unmodified. The idea here is to make this
	 *	configurable someway, just setting protect_page0 to 2, for example
	 *	will disable this whole check. That is unimplemented yet. I think
	 *	this must be a boot option, since pmmu code intializes things
	 *	long before mint.cnf is read->parsed... or?
	 * Odd Skancke:
	 *	Turns out that my CT63 Falcon also needs to have this protected
	 *	by the pmmu, so we _always_ do it.
	 */
	if (protect_page0 == 0 && (mch == MILAN_C || add_info == hades || add_info == raven))
	{
		boot_print("Hardware needs SUPER on first descriptor!\r\n");
		protect_page0 = 1;
	}
# endif /* !M68000 */

	/* initialize the info strings */
	identify (mch, add_info);

	DEBUG (("detecting hardware ... "));
	/* at the moment only detection of ST-ESCC */
	if (machine != machine_unknown && mcpu < 40 && detect_hardware ())
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
			default:
				gl_lang = i;
				break;
		}
	}

	if (gl_lang < 0)
		gl_lang = 0;

	return 0;
}

static void
identify (long mch, enum special_hw info)
{
	char buf[64];
	char *_cpu, *_mmu, *_fpu;
	short sfp = (short)(sfptype >> 16);

	switch (info)
	{
		case none:
		{
			switch (mch)
			{
				case ST:
					machine = machine_st;
					break;
				case STE:
				case STBOOK: /* Classed as an STE machine */
				case STEIDE: /* STE with IDE */
					machine = machine_ste;
					break;
				case MEGASTE:
					machine = machine_megaste;
					break;
				case TT:
					machine = machine_tt;
					break;
				case FALCON:
#ifdef __mcoldfire__
					machine = machine_firebee;
#else
					machine = machine_falcon;
#endif
					break;
				case MILAN_C:
					machine = machine_milan;
					break;
			}
			break;
		}
		case hades:
			machine = machine_hades;
			break;
		case ct2:
			machine = machine_ct2;
			break;
		case ct60:
			machine = machine_ct60;
			break;
		case raven:
			machine = machine_raven;
			break;
# ifdef WITH_NATIVE_FEATURES
		case emulator:
			machine = machine_emulator;
			break;
# endif
	}

	_fpu = " no FPU";

#ifdef __mcoldfire__
	if (!coldfire_68k_emulation)
	{
		fpu_type = "ColdFire V4e";
		_fpu = "/FPU";
	}
	else
#endif
	if (fpu)
	{
		switch ((fputype >> 16) & ~1)
		{
			case 0x04:
				fpu_type = !sfp ? "68881" : (sfp == 0x04 ?  "68881 & 68881 (SFP-004)" : "68881 & 68882 (SFP-004)");
				_fpu = !sfp ? " 68881 FPU" : (sfp == 0x04 ?  " 68881 & 68881 (SFP-004) FPU" : " 68881 & 68882 (SFP-004) FPU");
				break;
			case 0x06:
				fpu_type = !sfp ? "68882" : (sfp == 0x04 ?  "68882 & 68881 (SFP-004)" : "68882 & 68882 (SFP-004)");
				_fpu = !sfp ? " 68882 FPU" : (sfp == 0x04 ?  " 68882 & 68881 (SFP-004) FPU" : " 68882 & 68882 (SFP-004) FPU");
				break;
			case 0x08:
				fpu_type = !sfp ? "68040" : (sfp == 0x04 ?  "68040 & 68881 (SFP-004)" : "68040 & 68882 (SFP-004)");
				_fpu = !sfp ? "/FPU" : (sfp == 0x04 ?  "/FPU & 68881 (SFP-004) FPU" : "/FPU & 68882 (SFP-004) FPU");
				break;
			case 0x10:
				fpu_type = !sfp ? "68060" : (sfp == 0x04 ?  "68060 & 68881 (SFP-004)" : "68060 & 68882 (SFP-004)");
				_fpu = !sfp ? "/FPU" : (sfp == 0x04 ?  "/FPU & 68881 (SFP-004) FPU" : "/FPU & 68882 (SFP-004) FPU");
				break;
		}
	}
	else if (sfp)
	{
		switch (sfptype >> 16)
		{
			case 0x04:
				fpu_type = "68881 (SFP-004)";
				_fpu = " 68881 (SFP-004) FPU";
				break;
			case 0x06:
				fpu_type = "68882 (SFP-004)";
				_fpu = " 68882 (SFP-004) FPU";
				break;
		}
	}

#ifdef __mcoldfire__
	UNUSED(buf);

	if (!coldfire_68k_emulation)
	{
		cpu_type = mmu_type = "ColdFire V4e";
		_cpu = cpu_type;
		_mmu = "/MMU";
	}
	else
#endif
	{
		_cpu = "m68k";
		_mmu = "";

		if (is_apollo_68080)
		{
			cpu_type = "68080";
			_cpu = cpu_type;
		}
		else switch (mcpu)
		{
			case 0:
				cpu_type = "68000";
				_cpu = cpu_type;
				break;
			case 10:
				cpu_type = "68010";
				_cpu = cpu_type;
				break;
			case 20:
				cpu_type = "68020";
				_cpu = cpu_type;
				break;
			case 30:
				cpu_type = "68030";
				_cpu = cpu_type;
				if (pmmu)
				{
					mmu_type = cpu_type;
					_mmu = "/MMU";
				}
				break;
			case 40:
				cpu_type = "68040";
				_cpu = cpu_type;
				if (pmmu)
				{
					mmu_type = cpu_type;
					_mmu = "/MMU";
				}
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

				cpu_type = "68060";
				_cpu = buf;
				if (pmmu)
				{
					mmu_type = cpu_type;
					_mmu = "/MMU";
				}
				break;
			}
		}
	}

	ksprintf (cpu_model, sizeof (cpu_model), "%s (%s CPU%s%s) (_MCH 0x%lx)",
			machine_str(), _cpu, _mmu, _fpu, mch);

	boot_printf ("%s\r\n\r\n", cpu_model);
}
