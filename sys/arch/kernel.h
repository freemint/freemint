/*
 * This file belongs to FreeMiNT, it is not a part
 * of the original MiNT distribution.
 */

# ifndef _m68k_kernel_h
# define _m68k_kernel_h

# include "mint/mint.h"


void enter_gemdos (void);
void enter_bios   (void);
void leave_kernel (void);


# endif /* _m68k_kernel_h */
