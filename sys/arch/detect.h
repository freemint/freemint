/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * begin:	1998-11
 * last change: 1998-12-20
 * 
 * Author: J”rg Westheide <joerg_westheide@su.maus.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _detect_h
# define _detect_h

extern void *ControlRegA, *DataRegA;
extern void *ControlRegB, *DataRegB;

long detect_hardware (void);

long detect_cpu (void);		/* in detect.spp */
long detect_fpu (void);		/* ibidem */


# endif  /* _detect_h */
