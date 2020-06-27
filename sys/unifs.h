/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _unifs_h
# define _unifs_h

# include "mint/mint.h"
# include "mint/file.h"


# define UNIFS_MAJOR	1		/* version number for FS_INFO */
# define UNIFS_MINOR	1

extern FILESYS uni_filesys;

const char *fsname(FILESYS *fs);
FILESYS *get_filesys (int);
void unifs_init (void);


# endif /* _unifs_h */
