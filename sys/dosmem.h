/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosmem_h
# define _dosmem_h

# include "mint/mint.h"
# include "mint/mem.h"


long _cdecl m_addalt (long start, long size);
long _cdecl m_xalloc (long size, int mode);
long _cdecl m_alloc (long size);
long _cdecl m_free (virtaddr block);
long _cdecl m_shrink (int dummy, virtaddr block, long size);


# endif /* _dosmem_h */
