/* This file belongs to FreeMiNT,
 * it is not a part of the original MiNT distribution.
 */

# ifndef _m68k_cpu_h
# define _m68k_cpu_h

# include "mint/mint.h"


void _cdecl	init_cache	(void);

long _cdecl	ccw_getdmask	(void);
long _cdecl	ccw_get		(void);
long _cdecl	ccw_set		(ulong ctrl, ulong mask);

void _cdecl	cpush		(const void *base, long size);
void _cdecl	cpush000	(const void *base, long size);
void _cdecl	cpush030	(const void *base, long size);
void _cdecl	cpush040	(const void *base, long size);
void _cdecl	cpush060	(const void *base, long size);

void _cdecl	setstack	(long);

void _cdecl	get_superscalar	(void);
short _cdecl	is_superscalar	(void);


# endif /* _m68k_cpu_h */
