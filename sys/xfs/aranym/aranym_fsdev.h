/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * The u:\host filesystem base - derived from the freemint/sys/ramfs.c
 * file.
 *
 * Copyright (c) 2006 Standa Opichal of ARAnyM dev team.
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
 */

#ifndef _arafs_xfs_h_
#define _arafs_xfs_h_

# include "mint/mint.h"
# include "mint/file.h"

typedef struct blocks BLOCKS;
typedef struct symlnk SYMLNK;
typedef struct dirlst DIRLST;
typedef struct diroot DIROOT;
typedef struct cookie COOKIE;
typedef struct super  SUPER;
typedef struct cops   COPS;


struct blocks
{
	long	size;
	char 	**table;
	long	small_size;
	char 	*small;
};

struct symlnk
{
	char	*name;
	ushort	len;
};

struct dirlst
{
	DIRLST	*prev;
	DIRLST	*next;

	ushort	lock;		/* a reference left */
	ushort	len;		/* sizeof name */
	char	*name;		/* name */
	COOKIE	*cookie;	/* cookie for this file */

	char	sname [20];	/* short names */
# define SHORT_NAME	20
};

struct diroot
{
	DIRLST	*start;
	DIRLST	*end;
};

/* fcookie operations */
struct cops
{
	DEVDRV	*dev_ops;

	long _cdecl (*stat)( struct fcookie *fc, STAT *stat);
};


struct cookie
{
	COOKIE	*parent;	/* parent directory */
	SUPER	*s;		/* super pointer */

	ulong 	links;		/* in use counter */
	ulong	flags;		/* some flags */
	STAT	stat;		/* file attribute */

	FILEPTR *open;		/* linked list of open file ptrs */
	LOCK	*locks;		/* locks for this file */
	COPS	*cops;		/* individual cookie ops */

	union
	{
		BLOCKS	data;
		SYMLNK	symlnk;
		DIROOT	dir;
	} data;
};


struct super
{
	COOKIE	*root;
	char	*label;		/* FS label */
	ulong	flags;		/* FS flags */
# define MS_RDONLY		   1	/* Mount read-only */
# define MS_NOSUID		   2	/* Ignore suid and sgid bits */
# define MS_NODEV		   4	/* Disallow access to device special files */
# define MS_NOEXEC		   8	/* Disallow program execution */
# define MS_SYNCHRONOUS		  16	/* Writes are synced at once */
# define MS_REMOUNT		  32	/* Alter flags of a mounted FS */
# define MS_MANDLOCK		  64	/* Allow mandatory locks on an FS */
# define S_QUOTA		 128	/* Quota initialized for file/directory/symlink */
# define S_APPEND		 256	/* Append-only file */
# define S_IMMUTABLE		 512	/* Immutable file */
# define MS_NOATIME		1024	/* Do not update access times. */
# define MS_NODIRATIME		2048    /* Do not update directory access times */
# define S_NOT_CLEAN_MOUNTED	4096	/* not cleanly mounted */
# define S_PERSISTENT		8192	/* Cannot be deleted */
};


/*
 * existing FreeMiNT internal memory management
 * works with 8k aligned blocks!
 */
# if 0
# define BLOCK_SIZE	4096L
# define BLOCK_SHIFT	12
# else
# define BLOCK_SIZE	8192L
# define BLOCK_SHIFT	13
# endif
# define BLOCK_MASK	(BLOCK_SIZE - 1)
# define BLOCK_MIN	2	/* preallocate 1/4 smallblock */

/* kernel current time value */

INLINE time64_t current_time (void)
{
	if (KERNEL->xtime64)
		return KERNEL->xtime64->tv_sec;
	return (u_int32_t)KERNEL->xtime->tv_sec;
}
# define CURRENT_TIME	current_time ()


extern FILESYS arafs_filesys;
extern DEVDRV arafs_fs_devdrv;

extern short arafs_init( short dev );
FILESYS *aranymfs_init(void);
FILESYS *arafs_mount_drives(FILESYS *fs);

#endif /* _arafs_xfs_h_ */

