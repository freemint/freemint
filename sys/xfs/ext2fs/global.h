/*
 * Filename:     global.h
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# ifndef _global_h
# define _global_h

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/file.h"
# include "mint/proc.h"
# include "mint/time.h"

/*
 * Define EXT2FS_DEBUG to produce debug messages
 */
# if 0
# define EXT2FS_DEBUG	1
# endif

# include "ext2.h"


/* debug section
 */

# ifdef EXT2FS_DEBUG

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x

# else

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x

# endif

/* memory allocation
 * 
 * include statistic analysis to detect
 * memory leaks
 */

#ifdef EXT2FS_DEBUG
extern ulong memory;

INLINE void *
own_kmalloc (register long size)
{
	register void *tmp = kmalloc (size);
	if (tmp) memory += size;
	return tmp;
}

INLINE void
own_kfree (void *dst, register long size)
{
	if (dst)
		memory -= size;
	kfree (dst);
}

# undef kmalloc
# undef kfree

# define kmalloc	own_kmalloc
# define kfree		own_kfree
#else
#undef kfree
#define kfree(p, s) (*KERNEL->kfree)(p)
#endif


/* global ext2 specials
 */

extern ushort native_utc;

INLINE long
current_time (void)
{
	if (native_utc)
		return utc.tv_sec;
	
	return unixtime (timestamp, datestamp);
}
# define CURRENT_TIME	current_time ()


typedef struct cookie	COOKIE;	/* */
typedef struct si	SI;	/* */

struct cookie
{
	COOKIE	*next;		/* internal usage */
	
	ulong	links;		/* reference counter */
	ulong 	inode;		/* inode number */
	ushort	dirty;		/* flag to indicate modified inodes */
	ushort	res;		/* reserved */
	
	SI	*s;		/* super info */
	ulong	i_flags;	/* super block mount flags */
	
	ushort	dev;		/* device */
	ushort	rdev;		/* reserved */
	
	FILEPTR	*open;		/* linked list of open file ptr (kernel alloc) */
	LOCK 	*locks;		/* linked list of locks on this file (own alloc) */
	char	*lastlookup;	/* last lookup fail cache (own alloc) */
	long	lastlookupsize;	/* size of the previous string (without '\0') */
	
	ext2_in	in;		/* the inode itself */
	
	ulong	i_size;		/* actual file len - only in xdd valid */
	ulong	i_block_group;
	ulong	i_next_alloc_block;
	ulong	i_next_alloc_goal;
	ulong	i_prealloc_block;
	ulong	i_prealloc_count;
};

INLINE void
clear_lastlookup (COOKIE *c)
{
	if (c->lastlookup)
	{
		kfree (c->lastlookup, c->lastlookupsize + 1);
		c->lastlookup = NULL;
	}
}

INLINE void
update_lastlookup (COOKIE *c, const char *name, long namelen)
{
	if (c->lastlookup)
		kfree (c->lastlookup, c->lastlookupsize + 1);
	
	c->lastlookup = kmalloc (namelen + 1);
	if (c->lastlookup)
	{
		strcpy (c->lastlookup, name);
		c->lastlookupsize = namelen;
	}
}


struct si
{
	DI	*di;		/* device identifikator for this device */
	COOKIE	*root;		/* our root cookie */
	ushort	dev;		/* device this belongs to */
	ushort	rdev;		/* reserved */
	ulong	s_flags;	/* flags */
	ext2_si	sbi;		/* actual super info block */
};

extern SI *super [NUM_DRIVES];


# define EXT2_IFMT		0170000
# define EXT2_IFSOCK		0140000
# define EXT2_IFLNK		0120000
# define EXT2_IFREG		0100000
# define EXT2_IFBLK		0060000
# define EXT2_IFDIR		0040000
# define EXT2_IFCHR		0020000
# define EXT2_IFIFO		0010000

# define EXT2_ISUID		0004000
# define EXT2_ISGID		0002000
# define EXT2_ISVTX		0001000

# define EXT2_ISSOCK(m)		(((m) & EXT2_IFMT) == EXT2_IFSOCK)
# define EXT2_ISLNK(m)		(((m) & EXT2_IFMT) == EXT2_IFLNK)
# define EXT2_ISREG(m)		(((m) & EXT2_IFMT) == EXT2_IFREG)
# define EXT2_ISBLK(m)		(((m) & EXT2_IFMT) == EXT2_IFBLK)
# define EXT2_ISDIR(m)		(((m) & EXT2_IFMT) == EXT2_IFDIR)
# define EXT2_ISCHR(m)		(((m) & EXT2_IFMT) == EXT2_IFCHR)
# define EXT2_ISFIFO(m)		(((m) & EXT2_IFMT) == EXT2_IFIFO)

/*
 * These are the fs-independent mount-flags: up to 16 flags are supported
 */
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

# define IS_APPEND(inode)	(inode->i_flags & S_APPEND)
# define IS_IMMUTABLE(inode)	(inode->i_flags & S_IMMUTABLE)


/*
 * counter for i_version
 */
extern ulong event;


# endif /* _global_h */
