/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _tosfs_h
# define _tosfs_h

# include "mint/mint.h"
# include "mint/file.h"

# ifdef OLDTOSFS

extern int flk;
extern long gemdos_version;

extern FILESYS tos_filesys;
extern long clsizb[NUM_DRIVES];

# endif

# endif /* _tosfs_h */
