/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1998, 1999, 2000, 2001 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 1998-07-02
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 */

# include "global.h"


struct global global =
{
	machine_unknown, 0, 0, -1, 0, 0, "", ""
};

long mcpu = 0;
bool is_apollo_68080 = false;
short pmmu = 0;
short fpu = 0;
#ifdef __mcoldfire__
bool coldfire_68k_emulation = false;
#endif
short secure_mode = 0;
unsigned long c20ms = 0;

BASEPAGE *_base;

/*
 * special flags for workarounds
 */

/* flag for Falcon TOS kludges: TRUE if TOS 4.00 - 4.04
 * what about TOS 4.50 in Milan? see syscall.spp
 */
short falcontos = 0;

/* variable set to 1 if the _VDO cookie indicates Falcon style video
 */
short FalconVideo = 0;

/* variable set to 1 if the _VDO cookie indicates STE style video
 */
short ste_video = 0;

/* Flag that makes init_table() set protectionmode == super on
 * the first logical<->physical page descriptor. This because
 * some hardware dont provide protecting the systemvariables/
 * vectors from userspace accesses
 */
short protect_page0 = 0;
