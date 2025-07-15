/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _shmfs_h
# define _shmfs_h

# include "mint/mint.h"
# include "mint/file.h"
# include "memory.h"


# define SHMNAME_MAX 15

typedef struct shmfile
{
	struct shmfile *next;
	char filename[SHMNAME_MAX+1];
	int uid, gid;
	struct timeval mtime;
	struct timeval ctime;
	unsigned mode;
	int inuse;
	MEMREGION *reg;
} SHMFILE;

void shmfs_init(void);
void shmfs_warp_clock(long diff);

extern FILESYS shm_filesys;


# endif /* _shmfs_h */
