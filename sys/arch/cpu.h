/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _m68k_cpu_h
# define _m68k_cpu_h

# include "mint/mint.h"

void _cdecl	init_cache	(void);

long _cdecl	ccw_getdmask	(void);
long _cdecl	ccw_get		(void);
long _cdecl	ccw_set		(ulong ctrl, ulong mask);

void _cdecl	cpush		(const void *base, long size);
void _cdecl	cpushi		(const void *base, long size);

void _cdecl	setstack	(long);

# ifdef DEBUG_INFO
long _cdecl	get_ssp		(void);
# endif
long _cdecl	get_usp		(void);

void _cdecl	get_superscalar	(void);
short _cdecl	is_superscalar	(void);

# endif /* _m68k_cpu_h */
