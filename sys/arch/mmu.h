/* 
 * $Id$
 * 
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _m68k_mmu_h
# define _m68k_mmu_h

# include "mint/mint.h"


#ifndef MMU040
void _cdecl	set_mmu		(crp_reg, tc_reg);
#else
void _cdecl	set_mmu		(ulong *);
#endif
void _cdecl	save_mmu	(void);
void _cdecl	restr_mmu	(void);
void _cdecl	flush_mmu	(void);


# endif /* _m68k_mmu_h */
