/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosmem_h
# define _dosmem_h

# include "mint/mint.h"
# include "mint/mem.h"

long _cdecl sys_m_addalt (long start, long size);
long _cdecl sys_m_xalloc (long size, short mode);
long _cdecl sys_m_alloc (long size);
long _cdecl sys_m_free (long block);
long _cdecl sys_m_shrink (short dummy, unsigned long block, long size);
long _cdecl sys_m_validate (short pid, unsigned long addr, long size, long *flags);
long _cdecl sys_m_access (unsigned long addr, long size, short mode);


# endif /* _dosmem_h */
