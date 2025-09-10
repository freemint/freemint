/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _arch_init_intr_h
# define _arch_init_intr_h

# include "mint/mint.h"
# include "mint/emu_tos.h"

extern KBDVEC *syskey;

void	install_vector(long *old_handler, long vector, long _cdecl (*new_handler)());

void	install_TOS_vectors	(void);
void	restore_TOS_vectors	(void);
long    _cdecl register_trap2(long _cdecl (*dispatch)(void *), int mode, int flag, long extra);

# endif /* _arch_init_intr_h */
