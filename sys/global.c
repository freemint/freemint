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


ulong c20ms = 0;


long mch = 0;		/* machine we are are running */
long mcpu;		/* processor we are running */
long fputype = 0;	/* value for cookie jar */
/*
 * AGK: for proper co-processors we must consider saving their context.
 * This variable when non-zero indicates that the BIOS considers a true
 * coprocessor to be present. We use this variable in the context switch
 * code to decide whether to attempt an FPU context save.
 */
short fpu = 0;		/* flag */

int tosvers;		/* version of TOS we're running over */

/* flag for Falcon TOS kludges: TRUE if TOS 4.00 - 4.04
 * what about TOS 4.50 in Milan? see syscall.spp
 */
short falcontos;

/*
 * if this variable is set, then we have "secure mode" enabled; that
 * means only the superuser may access some functions those may be critical
 * for system security.
 *
 */

int secure_mode = 0;

/*
 * "screen_boundary+1" tells us how screens must be positioned
 * (to a 256 byte boundary on STs, a 16 byte boundary on other
 * machines; actually, 16 bytes is conservative, 4 is probably
 * OK, but it doesn't hurt to be cautious). The +1 is because
 * we're using this as a mask in the ROUND() macro in mem.h.
 */
int screen_boundary = 255;

/*
 * variable set if someone has already installed an flk cookie
 */
# if OLDTOSFS
int flk = 0;
# endif

/*
 * variable set to 1 if the _VDO cookie indicates Falcon style video
 */
int FalconVideo = 0;

/*
 * variable set to 1 if the _VDO cookie indicates STE style video
 */
short ste_video = 0;

/*
 * variable holds language preference
 */
int gl_lang = -1;

/* version of GEMDOS in ROM.
 * dos.c no longer directly calls Sversion()
 */
# ifdef OLDTOSFS
long gemdos_version;
# endif
