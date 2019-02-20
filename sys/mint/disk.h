/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

# ifndef _mint_disk_h
# define _mint_disk_h

# include "ktypes.h"


struct disk
{
	struct disk *next;		/* next disk in system */
	const char *name;		/* disk name */

	u_int64_t blks;			/* total number of blocks */
	u_int32_t blksize;		/* size of a block */

	short blkshift;			/* bytes to blk shift value */
	short res;

	struct disk_driver *driver;	/* the driver for this disk */

	/* statistics */
	u_int64_t transfers;		/* number of transfers */
	u_int64_t blocks;		/* number of transfered blocks */
	struct timeval timestamp;	/* last access time */
	struct timeval mounttime;	/* time of disk mount */
};

struct disk_driver
{
	long _cdecl (*strategy)(struct bunit *);
	long _cdecl (*ioctl)(struct proc *, long cmd, void *buf);

	const char *name;		/* driver name */

	long reserved[7];
};

# endif /* _mint_disk_h */
