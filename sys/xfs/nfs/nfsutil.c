/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

/*
 * File : nfsutil.c
 *        utility functions for the nfs
 */

# include "nfsutil.h"

# include "mint/emu_tos.h"


INLINE enum ftype
nfs_type (int mode, int attrib)
{
	switch (mode & S_IFMT)
	{
		case S_IFCHR:
			return NFCHR;
		case S_IFDIR:
			return NFDIR;
		case S_IFREG:
			return NFREG;
		case S_IFIFO:
			/* Is a fifo the same as a socket? Or a block special file? */
 			return NFNON;
		case S_IFMEM:
			/* What kind of file is this? A block special file? */
			return NFBLK;
		case S_IFLNK:
			return NFLNK;
	}
	
	return NFNON;
}


int
nfs_mode (int mode, int attrib)
{
	int newmode = mode & ~S_IFMT;
	
	switch (mode & S_IFMT)
	{
		case S_IFCHR:
			newmode |= N_IFCHR;
			break;
		case S_IFDIR:
			newmode |= N_IFDIR;
			break;
		case S_IFREG:
			newmode |= N_IFREG;
			break;
		case S_IFIFO:
			/* Is a fifo the same as a socket? Or a block special file? */
			newmode |= N_IFSCK;
			break;
		case S_IFMEM:
			/* What kind of file is this? A block special file? */
			newmode |= N_IFBLK;
			break;
		case S_IFLNK:
			newmode |= N_IFLNK;
			break;
	}
	
	return newmode;
}


INLINE int
mint_mode (int mode, int type)
{
	int newmode = mode & ~N_IFMT;

	switch (mode & N_IFMT)
	{
		case N_IFDIR:
			newmode |= S_IFDIR;
			break;
		case N_IFCHR:
			newmode |= S_IFCHR;
			break;
		case N_IFBLK:
			if (type == NFNON)
				newmode |= S_IFIFO;
			else
				newmode |= S_IFCHR;  /* BUG: we should have a block device type */
			break;
		case N_IFREG:
			newmode |= S_IFREG;
			break;
		case N_IFLNK:
			newmode |= S_IFLNK;
			break;
	}	
	
	return newmode;
}


/* convert an nfs fattr structure into an MiNT xattr structure
 */
void
fattr2xattr (fattr *fa, XATTR *xa)
{
	xa->mode = mint_mode (fa->mode, fa->type);
	xa->attr = 0;
	
	if ((xa->mode & S_IFMT) == S_IFDIR)
		xa->attr |= FA_DIR;
	if ((xa->mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		xa->attr |= FA_RDONLY;
	
	xa->index	= fa->fileid;
	xa->dev		= fa->fsid;
	xa->rdev	= fa->fsid;
	xa->nlink	= fa->nlink;
	xa->uid		= fa->uid;
	xa->gid		= fa->gid;
	xa->size	= fa->size;
	xa->blksize	= fa->blocksize;
	xa->nblocks	= fa->blocks;
	
	if (native_utc)
	{
		*((long *) &(xa->mtime)) = fa->mtime.seconds;
		*((long *) &(xa->atime)) = fa->atime.seconds;
		*((long *) &(xa->ctime)) = fa->ctime.seconds;
	}
	else
	{
		*((long *) &(xa->mtime)) = dostime (fa->mtime.seconds);
		*((long *) &(xa->atime)) = dostime (fa->atime.seconds);
		*((long *) &(xa->ctime)) = dostime (fa->ctime.seconds);
	}
	
# if 0
	if ((xa->mode & S_IFMT) == S_IFLNK)
		/* fix for buffer size when reading symlinks */
		++xa->size;
# endif
	
	xa->reserved2 = 0;
	xa->reserved3 [0] = 0;
	xa->reserved3 [1] = 0;
}

# if 0
void
xattr2fattr (XATTR *xa, fattr *fa)
{
	fa->type	= nfs_type (xa->mode, xa->attr);
	fa->mode	= nfs_mode (xa->mode, xa->attr);
	fa->nlink	= xa->nlink;
	fa->uid		= xa->uid;
	fa->gid		= xa->gid;
	fa->size	= xa->size;
	fa->blocksize	= xa->blksize;
	fa->rdev	= 0;    /* reserved for MiNT */
	fa->blocks	= xa->nblocks;
	fa->fsid	= xa->dev;
	fa->fileid	= xa->index;
	
	if (native_utc)
	{
		fa->atime.seconds  = *((long *) &(xa->atime));
		fa->atime.useconds = 0;
		fa->mtime.seconds  = *((long *) &(xa->mtime));
		fa->mtime.useconds = 0;
		fa->ctime.seconds  = *((long *) &(xa->ctime));
		fa->ctime.useconds = 0;
	}
	else
	{
		fa->atime.seconds  = unixtime (xa->atime, xa->adate);
		fa->atime.useconds = 0;
		fa->mtime.seconds  = unixtime (xa->mtime, xa->mdate);
		fa->mtime.useconds = 0;
		fa->ctime.seconds  = unixtime (xa->ctime, xa->cdate);
		fa->ctime.useconds = 0;
	}
}
# endif
