/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file on other projects without my explicit
 * permission.
 */

/*
 * begin:	2000-03-26
 * last change: 2000-03-26
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */


# ifndef _sys_emu_h
# define _sys_emu_h

# include "mint/mint.h"


long _cdecl sys_emu (ushort which, ushort op, long a1, long a2, long a3, long a4, long a5, long a6, long a7);


# endif /* _sys_emu_h */
