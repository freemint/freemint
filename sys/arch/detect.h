/* 
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Author: J”rg Westheide <joerg_westheide@su.maus.de>
 * Started: 1998-12-20
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _m68k_detect_h
# define _m68k_detect_h

extern void *ControlRegA, *DataRegA;
extern void *ControlRegB, *DataRegB;

long detect_hardware (void);

long detect_cpu (void);		/* in detect.spp */
long detect_fpu (void);		/* ibidem */

# endif  /* _m68k_detect_h */
