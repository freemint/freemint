/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
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
 */

# ifndef _global_h
# define _global_h

# include "mint/mint.h"
# include "mint/basepage.h"

/*
 * global variables
 */

# if __KERNEL__ == 1

extern struct global global;

#define machine		global.machine
#define fputype		global.fputype
#define sfptype		global.sfptype
#define tosvers		global.tosvers
#define emutos		global.emutos
#define gl_lang		global.gl_lang
#define gl_kbd		global.gl_kbd
#define sysdrv		global.sysdrv
#define sysdir		global.sysdir
#define mchdir		global.mchdir

extern BASEPAGE *_base;	/* pointer to kernel's basepage */


extern long mcpu; /* processor we are running */

extern bool is_apollo_68080; /* Set to true if CPU is Apollo 68080 */

extern short pmmu; /* flag if mmu is present */

/* for proper co-processors we must consider saving their context.
 * This variable when non-zero indicates that the BIOS considers a
 * true coprocessor to be present. We use this variable in the context
 * switch code to decide whether to attempt an FPU context save.
 */
extern short fpu; /* flag if fpu is present */

#ifdef __mcoldfire__
/* When true, we have a ColdFire CPU running a 68K emulation layer.
 * So ColdFire supervisor instructions are not available.
 * We also have 68K exception stack frames, not native ColdFire ones.
 */
extern bool coldfire_68k_emulation;
#endif

/* if this variable is set, then we have "secure mode" enabled; that
 * means only the superuser may access some functions those may be
 * critical for system security.
 */
extern short secure_mode;

extern short allow_setexc;		/* if 0 only kernel-processes or auto-programs may change sys-vectors */

/* linear 20ms counter */
extern unsigned long c20ms;

/* special flags */
extern short falcontos;
extern short FalconVideo;
extern short ste_video;
extern short protect_page0;

/* pointer to vector used by 200 Hz system timer */
extern long *p5msvec;
# endif

# endif /* _global_h */
