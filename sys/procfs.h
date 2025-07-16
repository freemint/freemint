/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _procfs_h
# define _procfs_h

# include "mint/mint.h"
# include "mint/file.h"


extern struct timeval64 procfs_stmp;

void procfs_init(void);
void procfs_warp_clock(long diff);

extern FILESYS proc_filesys;


# endif /* _procfs_h */
