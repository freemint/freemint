/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file for other projects without my
 * explicit permission.
 */

/*
 * begin:	1999-10-24
 * last change: 2000-04-08
 * 
 * Author: Guido Flohr <guido@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * Kernel system info filesystem.
 * 
 */

# ifndef _kernfs_h
# define _kernfs_h

# include "mint/mint.h"
# include "mint/file.h"


# if WITH_KERNFS

extern FILESYS kern_filesys;

int kern_follow_link (fcookie* fc, int depth);

# endif


# endif /* _kernfs_h */
