/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _nalloc2_h
# define _nalloc2_h

# ifdef __TURBOC__
# include "include\mint.h"
# else
# include "include/mint.h"
# endif


void nalloc_arena_add	(void *start, long len);
void *nalloc		(long size);
void nfree		(void *start);
void NALLOC_DUMP	(void);

# endif /* _nalloc2_h */
