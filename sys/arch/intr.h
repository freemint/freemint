/* This file belongs to FreeMiNT,
 * it is not a part of the original MiNT distribution.
 */

# ifndef _intr_h
# define _intr_h

# include "mint/mint.h"


/* interrupt vectors are linked some other way, see new_xbra_install() */
extern long old_timer, old_vbl, old_5ms;

/* old reset vector */
extern long old_resvec;

/* old ikbd vector */
extern long old_ikbd;

/* BIOS disk vectors */
extern long old_mediach, old_getbpb, old_rwabs;

extern long 	*intr_shadow;

long _cdecl	reset		(void);
void _cdecl	reboot		(void);
void _cdecl	newmvec		(void);
void _cdecl	newjvec		(void);
long _cdecl	new_rwabs	(void);
long _cdecl	new_mediach	(void);
long _cdecl	new_getbpb	(void);


# endif /* _intr_h */
