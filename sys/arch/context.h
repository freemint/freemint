/* 
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _m68k_context_h
# define _m68k_context_h

# include "mint/mint.h"


long _cdecl	build_context	(CONTEXT *sav, int fmt);
long _cdecl	save_context	(CONTEXT *sav);
void _cdecl	restore_context	(CONTEXT *sav);
void _cdecl	change_context	(CONTEXT *sav);


# endif /* _m68k_context_h */
