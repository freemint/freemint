/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _shmfs_h
# define _shmfs_h

# include "mint/mint.h"
# include "mint/file.h"
# include "memory.h"


void shmfs_init(void);
void shmfs_warp_clock(long diff);

extern FILESYS shm_filesys;


# endif /* _shmfs_h */
