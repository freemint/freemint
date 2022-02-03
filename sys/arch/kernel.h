/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 */

# ifndef _m68k_kernel_h
# define _m68k_kernel_h

# include "mint/mint.h"

extern short in_kernel;

void enter_gemdos (void);
void leave_kernel (void);

# endif /* _m68k_kernel_h */
