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

# include "info_mach.h"
# include "arch/native_features.h"
# include "global.h"
# include "mint/ktypes.h"

char *machine_arch = "m68k";

char cpu_model[64];

char *cpu_type     = "m68k";
char *mmu_type     = "none";
char *fpu_type     = "none";

char *machine_str (void)
{
	static char *str = "Unknown clone";

	switch (machine)
	{
		case machine_st:
			str = "Atari ST";
			break;
		case machine_ste:
			str = "Atari STE";
			break;
		case machine_megaste:
			str = "Atari MegaSTE";
			break;
		case machine_tt:
			str = "Atari TT";
			break;
		case machine_falcon:
			str = "Atari Falcon";
			break;
		case machine_firebee:
			str = "FireBee";
			break;
		case machine_milan:
			str = "Milan";
			break;
		case machine_hades:
			str = "Hades";
			break;
		case machine_ct2:
			str = "Atari Falcon/CT2";
			break;
		case machine_ct60:
			str = "Atari Falcon/CT60";
			break;
		case machine_raven:
			str = "Raven";
			break;
# ifdef WITH_NATIVE_FEATURES
		case machine_emulator:
			str = nf_name();
			break;
# endif
		case machine_unknown:
		default:
			/* nothing to set */
			break;
	}

	return str;
}
