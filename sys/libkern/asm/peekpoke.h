/*
 * This file belongs to FreeMiNT, it is not a part
 * of the original MiNT distribution.
 */

# ifndef _peekpoke_h
# define _peekpoke_h

# include "mint/mint.h"


long	_cdecl fat12_peek	(char *data, ulong offset);
long	_cdecl fat12_poke	(char *data, ulong offset, long nextcl);


# endif /* _peekpoke_h */
