/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _procfs_h
# define _procfs_h

# include "mint/mint.h"
# include "mint/file.h"


extern struct timeval procfs_stmp;

void procfs_init (void);

extern FILESYS proc_filesys;


# endif /* _procfs_h */
