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

#ifndef _isofs_global_h
#define _isofs_global_h

#include "mint/mint.h"

#include "libkern/libkern.h"
#include "mint/file.h"
#include "mint/proc.h"
#include "mint/time.h"

#include "iso.h"
#include "iso_rrip.h"


/* debug section
 */

#if 1
#define FS_DEBUG	1
#endif

#ifdef FS_DEBUG

#define FORCE(x)	
#define ALERT(x)	KERNEL_ALERT x
#define DEBUG(x)	KERNEL_DEBUG x
#define TRACE(x)	KERNEL_TRACE x
#define ASSERT(x)	assert x

#else

#define FORCE(x)	
#define ALERT(x)	KERNEL_ALERT x
#define DEBUG(x)	
#define TRACE(x)	
#define ASSERT(x)	assert x

#endif

/* memory allocation
 * 
 * include statistic analysis to detect
 * memory leaks
 */

extern unsigned long memory;

INLINE void *
own_kmalloc(register long size)
{
	register void *tmp = kmalloc(size);
	if (tmp) memory += size;
	return tmp;
}

INLINE void
own_kfree(void *dst, register long size)
{
	memory -= size;
	kfree(dst);
}

#undef kmalloc
#undef kfree

#define kmalloc	own_kmalloc
#define kfree	own_kfree


INLINE long
current_time(void)
{
	return utc.tv_sec;
}
# define CURRENT_TIME	current_time()


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
	
//	ext2_in	in;		/* the inode itself */
	
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
		kfree(c->lastlookup, c->lastlookupsize + 1);
		c->lastlookup = NULL;
	}
}

INLINE void
update_lastlookup(COOKIE *c, const char *name, long namelen)
{
	if (c->lastlookup)
		kfree(c->lastlookup, c->lastlookupsize + 1);
	
	c->lastlookup = kmalloc(namelen + 1);
	if (c->lastlookup)
	{
		strcpy(c->lastlookup, name);
		c->lastlookupsize = namelen;
	}
}

enum ISO_TYPE
{
	ISO_TYPE_DEFAULT,
	ISO_TYPE_9660,
	ISO_TYPE_RRIP,
	ISO_TYPE_ECMA
};

struct iso_super
{
	DI *di;			/* device identifikator for this device */
//	COOKIE *root;		/* our root cookie */
	ushort dev;		/* device this belongs to */
	ushort rdev;		/* reserved */
	ulong s_flags;		/* flags */

	short im_flags;
	short im_joliet_level;

	long logical_block_size;
	long im_bshift;
	long im_bmask;

	long volume_space_size;

	char root[ISODCL (157, 190)];
	long root_extent;
	long root_size;
	enum ISO_TYPE iso_type;

	long rr_skip;
	long rr_skip0;
};

# endif /* _isofs_global_h */
