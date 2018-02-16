/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _arch_init_intr_h
# define _arch_init_intr_h

# include "mint/mint.h"
# include "mint/emu_tos.h"

extern KBDVEC *syskey;

void	new_xbra_install(long *xv, long addr, long _cdecl (*func)());

void	init_intr	(void);
void	restr_intr	(void);

# endif /* _arch_init_intr_h */
