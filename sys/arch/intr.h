/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _m68k_intr_h
# define _m68k_intr_h

# include "mint/mint.h"


/* interrupt vectors are linked some other way, see new_xbra_install() */
extern long old_timer, old_vbl, old_5ms;

/* old reset vector */
extern long old_resvec;

/* old ikbd vector */
extern long old_ikbd;

/* old ikbd vector (other way) */
extern long oldkeys;

/* BIOS disk vectors */
extern long old_mediach, old_getbpb, old_rwabs;

# if 0
extern long 	*intr_shadow;
# endif

long _cdecl	reset		(void);
void _cdecl	reboot		(void) NORETURN;
void _cdecl	newmvec		(void);
void _cdecl	newjvec		(void);
long _cdecl	newkeys		(void);
void _cdecl	kbdclick	(short scancode);
long _cdecl	new_rwabs	(void);
long _cdecl	new_mediach	(void);
long _cdecl	new_getbpb	(void);

# endif /* _m68k_intr_h */
