/* This file belongs to FreeMiNT,
 * it is not a part of the original MiNT distribution.
 */

# ifndef _mmu_h
# define _mmu_h

# include "mint/mint.h"


#ifndef MMU040
void _cdecl	set_mmu		(crp_reg, tc_reg);
#else
void _cdecl	set_mmu		(ulong *);
#endif
void _cdecl	save_mmu	(void);
void _cdecl	restr_mmu	(void);
void _cdecl	flush_mmu	(void);


# endif /* _mmu_h */
