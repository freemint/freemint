/*
 * Filename:     inode.c
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               he MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
 *               Copyright 1998, 1999 Axel Kaiser (DKaiser@AM-Gruppe.de)
 * 
 * Portions copyright 1992, 1993, 1994, 1995 Remy Card (card@masi.ibp.fr)
 * and 1991, 1992 Linus Torvalds (torvalds@klaava.helsinki.fi)
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

# include "inode.h"

# include <mint/endian.h>

# include "balloc.h"
# include "ialloc.h"
# include "super.h"
# include "truncate.h"


# if 1
# define COOKIE_HASHBITS	(8)
# else
# define COOKIE_HASHBITS	(12)
# endif
# define COOKIE_SIZE		(1UL << COOKIE_HASHBITS)
# define COOKIE_HASHMASK	(COOKIE_SIZE - 1)

static COOKIE cookies [COOKIE_SIZE];
static COOKIE *ctable [COOKIE_SIZE];

INLINE ulong
cookie_hash (register const ulong n)
{
	register ulong hash;
	
	hash  = n;
	hash ^= (hash >> COOKIE_HASHBITS) ^ (hash >> (COOKIE_HASHBITS << 1));
	
	return hash & COOKIE_HASHMASK;
}

INLINE COOKIE *
cookie_lookup (register const ulong inode, register const ushort dev)
{
	register const ulong hashval = cookie_hash (inode);
	register COOKIE *c;
	
	for (c = ctable [hashval]; c != NULL; c = c->next)
	{
		if ((c->dev == dev) && (c->inode == inode))
		{
			DEBUG (("cookie_lookup: match (%li, #%li)!", hashval, inode));
			return c;
		}
	}
	
	DEBUG (("cookie_lookup: fail (%li, #%li)!", hashval, inode));
	return NULL;
}

void
cookie_install (register COOKIE *c)
{
	register const ulong hashval = cookie_hash (c->inode);
	
	DEBUG (("cookie_install: %li, inode = %li", hashval, c->inode));
	
	c->next = ctable [hashval];
	ctable [hashval] = c;
}

INLINE void
cookie_remove (register COOKIE *c)
{
	register const ulong hashval = cookie_hash (c->inode);
	register COOKIE **temp = & ctable [hashval];
	register long flag = 1;
	
	while (*temp)
	{
		if (*temp == c)
		{
			*temp = c->next;
			flag = 0;
			break;
		}
		temp = &(*temp)->next;
	}
	
	if (flag)
	{
		ALERT (("Ext2-FS: remove from hashtable fail in: cookie_remove (addr = %p, %lu)", c, c->inode));
	}
}

COOKIE *
get_new_cookie (void)
{
	register long i;
	
	for (i = 0; i < COOKIE_SIZE; i++)
	{
		static long count = 0;
		register COOKIE *c;
		
		count++;
		if (count == COOKIE_SIZE) count = 0;
		
		c = &(cookies [count]);
		if (!c->links)
		{
			if (c->inode)
			{
				register long r;
			
				r = SYNC_COOKIE (c);
				if (r)
				{
					ALERT (("Ext2-FS: failed to update inode #%li (r = %li)", c->inode, r));
					continue;
				}
				
				del_cookie (c);
			}
			
			bzero (c, sizeof (*c));
			
			c->links = 1;
			return c;
		}
	}
	
	ALERT (("Ext2-FS: get_new_cookie: no free COOKIE found"));
	
	return NULL;
}

COOKIE *
new_cookie (const ulong inode)
{
	COOKIE *c;
	
	c = get_new_cookie ();
	if (c)
	{
		c->inode = inode;
		
		cookie_install (c);
	}
	
	return c;
}

void
del_cookie (COOKIE *c)
{
	if (c->inode)
	{
		if (c->dirty)
			ALERT (("Ext2-FS [%c]: del_cookie: Inode #%li not written back!", DriveToLetter(c->dev), c->inode));
		
		cookie_remove (c);
	}
	
	if (c->open)
	{
		DEBUG (("Ext2-FS [%c]: open FILEPTR detect in: del_cookie #%li", DriveToLetter(c->dev), c->inode));
		c->open = NULL;
	}
	if (c->locks)
	{
		ALERT (("Ext2-FS [%c]: open LOCKS detect in: del_cookie #%li", DriveToLetter(c->dev), c->inode));
		c->locks = NULL;
	}
	
	c->links = 0;
	c->inode = 0;
	c->dirty = 0;
	
	clear_lastlookup (c);
}

void
inv_ctable (ushort drv)
{
	register long i;
	
	for (i = 0; i < COOKIE_SIZE; i++)
	{
		register COOKIE *c;
		
		for (c = ctable [i]; c != NULL; c = c->next)
		{
			if (c->dev == drv && c->inode)
			{
				del_cookie (c);
			}
		}
	}
}

long
get_cookie (SI *s, long inode, COOKIE **in)
{
	COOKIE *c;
	
	DEBUG (("Ext2-FS [%c]: get_cookie: enter: #%li", DriveToLetter(s->dev), inode));
	
	c = cookie_lookup (inode, s->dev);
	if (c)
	{
		*in = c;
		c->links++;
		
		return E_OK;
	}
	
	c = new_cookie (inode);
	if (c)
	{
		ext2_gd *i_g;
		long group;
		long offset;
		long block;
		UNIT *u;
		
		if ((inode != EXT2_ROOT_INO
				&& inode != EXT2_ACL_IDX_INO
				&& inode != EXT2_ACL_DATA_INO
				&& inode < EXT2_FIRST_INO (s))
			|| inode > (s->sbi.s_inodes_count))
		{
			ALERT (("Ext2-FS: ext2_read_inode: bad inode number: #%li", inode));
			
			del_cookie (c);
			
			DEBUG (("Ext2-FS: get_cookie: leave failure (inode out of range [%li])", inode));
			return EBADARG;
		}
		
		/* inodes are numbered from 1
		 * -> C offset correction
		 */
		inode--;
		
		/* group number */
		group = inode / EXT2_INODES_PER_GROUP (s);
		
		/* index on group */
		offset = inode % EXT2_INODES_PER_GROUP (s) * EXT2_INODE_SIZE (s);
		
		/* ptr to the actual group descriptor */
		i_g = ext2_get_group_desc (s, group, NULL);
		
		/* block of the inode */
		block  = le2cpu32 (i_g->bg_inode_table);
		block += (offset >> EXT2_BLOCK_SIZE_BITS (s));
		
		/* read the target block */
		u = bio.read (s->di, block, EXT2_BLOCK_SIZE (s));
		if (u)
		{
			offset &= EXT2_BLOCK_SIZE_MASK (s);
			
			/* copy inode raw data */
			c->in = *((ext2_in *) (u->data + offset));
			
			
			/* setup cookie
			 */
			
			/* c->dirty = 0; */
			
			c->s = s;
			c->i_flags = s->s_flags;
			
			c->dev = s->dev;
			c->rdev = s->rdev;
			
			/* c->open = NULL;
			c->locks = NULL;
			c->lastlookup = NULL;
			c->lastlookupsize = 0; */
			
			c->i_block_group = group;
			/* c->i_next_alloc_block = 0;
			c->i_next_alloc_goal = 0;
			c->i_prealloc_block = 0;
			c->i_prealloc_count = 0; */
			
			{
				register ulong i_flags;
				
				i_flags = le2cpu32 (c->in.i_flags);
				
				if (i_flags & EXT2_SYNC_FL)
					c->i_flags |= MS_SYNCHRONOUS;
				
				if (i_flags & EXT2_APPEND_FL)
					c->i_flags |= S_APPEND;
				
				if (i_flags & EXT2_IMMUTABLE_FL)
					c->i_flags |= S_IMMUTABLE;
				
				if (i_flags & EXT2_NOATIME_FL)
					c->i_flags |= MS_NOATIME;
			}
			
			*in = c;
			
			DEBUG (("Ext2-FS: get_cookie leave ok"));
			return E_OK;
		}
		
		del_cookie (c);
	}
	
	DEBUG (("Ext2-FS: get_cookie: leave failure (new_cookie fail)"));
	return ENOMEM;
}

long
put_cookie (COOKIE *c)
{
	SI *s = c->s;
	long inode = c->inode;
	
	ext2_gd *i_g;
	long group;
	long offset;
	long block;
	UNIT *u;
	
	
	DEBUG (("Ext2-FS [%c]: put_inode enter (%li)", DriveToLetter(c->dev), inode));
	
	if ((inode != EXT2_ROOT_INO
			&& inode != EXT2_ACL_IDX_INO
			&& inode != EXT2_ACL_DATA_INO
			&& inode < EXT2_FIRST_INO (s))
		|| inode > (s->sbi.s_inodes_count))
	{
		ALERT (("Ext2-FS: ext2_write_inode: bad inode number: #%li", inode));
		
		/* bad inode */
		
		DEBUG (("Ext2-FS: read_inode: leave failure (inode out of range [%li])", inode));
		return EBADARG;
	}
	
	/* inodes are numbered from 1
	 * -> C offset correction
	 */
	inode--;
	
	/* group number */
	group = inode / EXT2_INODES_PER_GROUP (s);
	
	/* index on group */
	offset = inode % EXT2_INODES_PER_GROUP (s) * EXT2_INODE_SIZE (s);
	
	/* ptr to the actual group descriptor */
	i_g = ext2_get_group_desc (s, group, NULL);
	
	/* block of the inode */
	block  = le2cpu32 (i_g->bg_inode_table);
	block += (offset >> EXT2_BLOCK_SIZE_BITS (s));
	
	/* read target block */
	u = bio.read (s->di, block, EXT2_BLOCK_SIZE (s));
	if (u)
	{
		offset &= EXT2_BLOCK_SIZE_MASK (s);
		
		/* copy the inode raw data */
		*((ext2_in *) (u->data + offset)) = c->in;
		
		/* and write back */
		bio_MARK_MODIFIED (&bio, u);
		
		DEBUG (("Ext2-FS: put_inode leave ok"));
		return E_OK;
	}
	
	DEBUG (("Ext2-FS: out_inode: leave failure (bio.read fail)"));
	return EREAD;
}

void
rel_cookie (COOKIE *c)
{
	if (c->links)
	{
		c->links--;
		
		if (!c->links)
		{
			if (!c->in.i_links_count && !c->open)
			{
				DEBUG (("Ext2-FS [%c]: rel_cookie: free deleted inode #%li", DriveToLetter(c->dev), c->inode));
				ext2_delete_inode (c);
			}
		}
	}
	else
	{
		DEBUG (("Ext2-FS [%c]: rel_cookie -> links = 0 (#%li)",
			DriveToLetter(c->dev), c->inode));
	}
}

void
sync_cookies (void)
{
	long i;
	
	for (i = 0; i < COOKIE_SIZE; i++)
	{
		register COOKIE *c = &(cookies [i]);
		
		if (c->inode)
		{
			register long r;
			
			r = SYNC_COOKIE (c);
			if (r)
				ALERT (("Ext2-FS: failed to update inode #%li (r = %li)", c->inode, r));
		}
	}
}

# ifdef EXT2FS_DEBUG

# include "namei.h"

void
dump_inode_cache (char *buf, long bufsize)
{
	char tmp [128 /*SPRINTF_MAX*/];
	long len;
	long i;
	
	bufsize--;
	if (bufsize <= 0)
		return;
	
	ksprintf (tmp, "\n\n--- inode cache ---\n");
	len = MIN (strlen (tmp), bufsize);
	strncpy (buf, tmp, len);
	buf += len;
	bufsize -= len;
	
	for (i = 0; i < COOKIE_SIZE; i++)
	{
		COOKIE *c = &(cookies [i]);
		_DIR *dentry;
		long dentry_nr = 0;
		
# if 0
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
	
	ulong	i_block_group;
	ulong	i_next_alloc_block;
	ulong	i_next_alloc_goal;
	ulong	i_prealloc_block;
	ulong	i_prealloc_count;
};
# endif
		ksprintf (tmp, "%3li: [%c] #%06li - %3lu - %06lx - %06lx - %s - %s",
			i,
			DriveToLetter(c->dev),
			c->inode,
			c->links,
			c->i_flags,
			(long) le2cpu16 (c->in.i_mode),
			c->dirty ? "dirty" : "clean",
			c->open ? "opened" : "closed"
		);
		len = MIN (strlen (tmp), bufsize);
		strncpy (buf, tmp, len);
		buf += len;
		bufsize -= len;
		
# if 1
		dentry = d_lookup_cookie (c, &dentry_nr);
		while (dentry)
		{
			ksprintf (tmp, " - #%06li: %s", (long) dentry->parent, dentry->name);
			len = MIN (strlen (tmp), bufsize);
			strncpy (buf, tmp, len);
			buf += len;
			bufsize -= len;
			
			dentry = d_lookup_cookie (c, &dentry_nr);
		}
# endif
		
		ksprintf (tmp, "\n");
		len = MIN (strlen (tmp), bufsize);
		strncpy (buf, tmp, len);
		buf += len;
		bufsize -= len;
		
		if (bufsize <= 0)
			break;
	}
	
	*buf = '\0';
}
# endif

void
ext2_delete_inode (COOKIE *inode)
{
	if (inode->inode == EXT2_ACL_IDX_INO
		|| inode->inode == EXT2_ACL_DATA_INO)
	{
		return;
	}
	
	inode->in.i_dtime = cpu2le32 (CURRENT_TIME);
	mark_inode_dirty (inode);
	
	if (inode->in.i_blocks)
		ext2_truncate (inode, 0);
	
	ext2_free_inode (inode);
}


static long
inode_bmap (COOKIE *inode, long nr)
{
	register long tmp;
	
	if ((nr < 0) || (nr >= EXT2_N_BLOCKS))
	{
		ALERT (("Ext2-FS: inode_bmap: nr (%li) outside range!", nr));
		return 0;
	}
	
	tmp = le2cpu32 (inode->in.i_block[nr]);
	if (tmp)
	{
		if ((tmp < inode->s->sbi.s_first_data_block) || (tmp >= inode->s->sbi.s_blocks_count))
		{
			ALERT (("Ext2-FS: inode_bmap: tmp (%li), illegal value", tmp));
			return 0;
		}
	}
	
	return tmp;
}

static long
block_bmap (COOKIE *inode, long block, long nr)
{
	register long blocksize = EXT2_BLOCK_SIZE (inode->s);
	register long tmp;
	register UNIT *u;
	
	if (block == 0)
		return 0;
	
	if ((block < inode->s->sbi.s_first_data_block) || (block >= inode->s->sbi.s_blocks_count))
	{
		ALERT (("Ext2-FS: block_bmap: block (%li) outside range!", block));
		return 0;
	}
	
	if ((nr < 0) || (nr >= (blocksize >> 2)))
	{
		ALERT (("Ext2-FS: block_bmap: nr (%li) outside range!", nr));
		return 0;
	}
	
	u = bio.read (inode->s->di, block, blocksize);
	if (!u) return 0;
	
	tmp = le2cpu32 (((ulong *) u->data)[nr]);
	if (tmp)
	{
		if ((tmp < inode->s->sbi.s_first_data_block) || (tmp >= inode->s->sbi.s_blocks_count))
		{
			ALERT (("Ext2-FS: block_bmap: tmp (%li), illegal value", tmp));
			return 0;
		}
	}
	
	return tmp;
}

long
ext2_bmap (COOKIE *inode, long block)
{
	long addr_per_block = EXT2_ADDR_PER_BLOCK (inode->s);
	long addr_per_block_bits = EXT2_ADDR_PER_BLOCK_BITS (inode->s);
	long i;
	
	DEBUG (("Ext2-FS: ext2_bmap enter (%li)", block));
	
	if (block < 0)
	{
		ALERT (("Ext2-FS: ext2_bmap: block < 0"));
		return 0;
	}
	
	if (block >= EXT2_NDIR_BLOCKS + addr_per_block +
		(1UL << (addr_per_block_bits << 1)) +
		((1UL << (addr_per_block_bits << 1)) << addr_per_block_bits))
	{
		ALERT (("Ext2-FS: ext2_bmap: block (%li) > big (%li)",
			block,
			EXT2_NDIR_BLOCKS + addr_per_block +
			(1UL << (addr_per_block_bits << 1)) +
			((1UL << (addr_per_block_bits << 1)) << addr_per_block_bits)
		));
		
		return 0;
	}
	
	/* direct blocks */
	
	if (block < EXT2_NDIR_BLOCKS)
		return inode_bmap (inode, block);
	
	/* indirect blocks */
	
	block -= EXT2_NDIR_BLOCKS;
	if (block < addr_per_block)
	{
		i = inode_bmap (inode, EXT2_IND_BLOCK);
		return block_bmap (inode, i, block);
	}
	
	/* double indirect blocks */
	
	block -= addr_per_block;
	if (block < (1UL << (addr_per_block_bits << 1)))
	{
		i = inode_bmap (inode, EXT2_DIND_BLOCK);
		i = block_bmap (inode, i, block >> addr_per_block_bits);
		return block_bmap (inode, i, block & (addr_per_block - 1));
	}
	
	/* triple indirect blocks */
	
	block -= (1UL << (addr_per_block_bits << 1));
	i = inode_bmap (inode, EXT2_TIND_BLOCK);
	i = block_bmap (inode, i, block >> (addr_per_block_bits << 1));
	i = block_bmap (inode, i, (block >> addr_per_block_bits) & (addr_per_block - 1));
	return block_bmap (inode, i, block & (addr_per_block - 1));
}

UNIT *
ext2_read (COOKIE *inode, long block, long *err)
{
	long tmp;
	UNIT *u = NULL;
	
	tmp = ext2_bmap (inode, block);
	if (tmp)
	{
		u = bio.read (inode->s->di, tmp, EXT2_BLOCK_SIZE (inode->s));
		if (!u && err)
			*err = EREAD;
	}
	else if (err)
	{
		*err = E_OK;
	}
	
	return u;
}


/* ext2_discard_prealloc and ext2_alloc_block are atomic wrt. the
 * superblock in the same manner as are ext2_free_blocks and
 * ext2_new_block.  We just wait on the super rather than locking it
 * here, since ext2_new_block will do the necessary locking and we
 * can't block until then.
 */
void
ext2_discard_prealloc (COOKIE *inode)
{
# ifdef EXT2_PREALLOCATE
	register ushort total;
	
	DEBUG (("ext2_discard_prealloc: enter (#%li)", inode->inode));
	
	if (inode->i_prealloc_count)
	{
		total = inode->i_prealloc_count;
		inode->i_prealloc_count = 0;
		ext2_free_blocks (inode, inode->i_prealloc_block, total);
	}
	
	DEBUG (("ext2_discard_prealloc: leave"));
# endif
}

static long
ext2_alloc_block (COOKIE *inode, ulong goal, long *err)
{
# ifdef EXT2FS_DEBUG
	static ulong alloc_hits = 0;
	static ulong alloc_attempts = 0;
# endif
	ulong result;
	
	
	DEBUG (("ext2_alloc_block: enter"));
	DEBUG (("ext2_alloc_block: inode = %li, goal = %ld", inode->inode, goal));
	
	/* wait_on_super (inode->i_sb); */
	
# ifdef EXT2_PREALLOCATE
	if (inode->i_prealloc_count
		&& (goal == inode->i_prealloc_block
			|| goal + 1 == inode->i_prealloc_block))
	{		
		result = inode->i_prealloc_block++;
		inode->i_prealloc_count--;
		
		DEBUG (("preallocation hit (%lu/%lu)", ++alloc_hits, ++alloc_attempts));
	}
	else
	{
		ext2_discard_prealloc (inode);
		
		DEBUG (("preallocation miss (%lu/%lu)", alloc_hits, ++alloc_attempts));
		
		if (EXT2_ISREG (le2cpu16 (inode->in.i_mode)))
		{
			result = ext2_new_block (inode, goal, 
					&(inode->i_prealloc_count),
					&(inode->i_prealloc_block), err);
		}
		else
		{
			result = ext2_new_block (inode, goal, 0, 0, err);
		}
	}
# else
	result = ext2_new_block (inode, goal, NULL, NULL, err);
# endif
	
	DEBUG (("ext2_alloc_block: leave (result = %ld, *err = %li)", result, *err));
	return result;
}

static long
inode_getblk (COOKIE *inode, long nr, long new_block, long *err, ushort clear_flag)
{
	ulong *p;
	long tmp;
	long goal;
	
	DEBUG (("inode_getblk: enter (#%li, nr = %li, new_block = %li)", inode->inode, nr, new_block));
	
	if ((nr < 0) || (nr >= EXT2_N_BLOCKS))
	{
		ALERT (("Ext2-FS: inode_getblk: nr (%li) outside range!", nr));
		return 0;
	}
	
	p = inode->in.i_block + nr;
	tmp = *p;
	if (tmp)
	{
		tmp = le2cpu32 (tmp);
		
		if ((tmp < inode->s->sbi.s_first_data_block) || (tmp >= inode->s->sbi.s_blocks_count))
		{
			ALERT (("Ext2-FS: inode_getblk: tmp (%li), illegal value", tmp));
			return 0;
		}
		
		return tmp;
	}
	
	*err = EFBIG;
	/* Check file limits */
	{
	}
	
	goal = 0;
	if (inode->i_next_alloc_block == new_block)
		goal = inode->i_next_alloc_goal;
	
	if (goal == 0)
	{
		for (tmp = nr - 1; tmp >= 0; tmp--)
		{
			if (inode->in.i_block[tmp])
			{
				goal = le2cpu32 (inode->in.i_block[tmp]);
				break;
			}
		}
		
		if (!goal)
			goal = (inode->i_block_group * EXT2_BLOCKS_PER_GROUP (inode->s)) +
				inode->s->sbi.s_first_data_block;
	}
	
	tmp = ext2_alloc_block (inode, goal, err);
	if (!tmp)
	{
		DEBUG (("inode_getblk: leave (!tmp -> 0)"));
		return 0;
	}
	
	if ((tmp < inode->s->sbi.s_first_data_block) || (tmp >= inode->s->sbi.s_blocks_count))
	{
		ALERT (("Ext2-FS: ext2_alloc_block = %li, illegal value", tmp));
		return 0;
	}
	
	*p = cpu2le32 (tmp);
	inode->i_next_alloc_block = new_block;
	inode->i_next_alloc_goal = tmp;
	inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
	inode->in.i_blocks = cpu2le32 (le2cpu32 (inode->in.i_blocks) + (EXT2_BLOCK_SIZE (inode->s) >> 9));
	mark_inode_dirty (inode);
	
	if (clear_flag)
	{
		UNIT *u;
		
		u = bio.getunit (inode->s->di, tmp, EXT2_BLOCK_SIZE (inode->s));
		if (u)
		{
			bzero (u->data, EXT2_BLOCK_SIZE (inode->s));
			bio_MARK_MODIFIED (&bio, u);
		}
		else
		{
			ALERT (("Ext2-FS: inode_getblk: getunit fail!"));
			tmp = 0;
		}
	}
	
	DEBUG (("inode_getblk: leave (tmp = %ld)", tmp));
	return tmp;
}

static long
block_getblk (COOKIE *inode, long block, long nr, long new_block, long *err, ushort clear_flag)
{
	long blocksize = EXT2_BLOCK_SIZE (inode->s);
	long tmp;
	long goal;
	UNIT *u;
	
	DEBUG (("block_getblk: enter (#%li, block = %li, nr = %li)", inode->inode, block, nr));
	DEBUG (("block_getblk: blocksize = %li, new_block = %li", blocksize, new_block));
	
	if ((block < inode->s->sbi.s_first_data_block) || (block >= inode->s->sbi.s_blocks_count))
	{
		ALERT (("Ext2-FS: block_getblk: block (%li) outside range!", block));
		return 0;
	}
	
	if ((nr < 0) || (nr >= (blocksize >> 2)))
	{
		ALERT (("Ext2-FS: block_getblk: nr (%li) outside range!", nr));
		return 0;
	}
	
	u = bio.read (inode->s->di, block, blocksize);
	if (!u)
	{
		DEBUG (("block_getblk: leave (!u1 (%li, %li) -> 0)", block, blocksize));
		return 0;
	}
	
	tmp = ((ulong *) u->data)[nr];
	
	/* behave like bmap */
	if (tmp)
	{
		tmp = le2cpu32 (tmp);
		
		if ((tmp < inode->s->sbi.s_first_data_block) || (tmp >= inode->s->sbi.s_blocks_count))
		{
			ALERT (("Ext2-FS: block_getblk: tmp (%li), illegal value", tmp));
			return 0;
		}
		
		return tmp;
	}
	
	*err = EFBIG;
	/* Check file limits */
	{
	}
	
	goal = 0;
	if (inode->i_next_alloc_block == new_block)
		goal = inode->i_next_alloc_goal;
	
	if (goal == 0)
	{
		for (tmp = nr - 1; tmp >= 0; tmp--)
		{
			register ulong i;
			
			i = ((ulong *) u->data)[tmp];
			if (i)
			{
				goal = le2cpu32 (i);
				break;
			}
		}
		
		if (!goal)
			goal = block;
	}
	
	tmp = ext2_alloc_block (inode, goal, err);
	if (tmp == 0)
	{
		DEBUG (("block_getblk: leave (!tmp -> 0)"));
		return 0;
	}
	
	if ((tmp < inode->s->sbi.s_first_data_block) || (tmp >= inode->s->sbi.s_blocks_count))
	{
		ALERT (("Ext2-FS: ext2_alloc_block = %li, illegal value", tmp));
		return 0;
	}
	
	u = bio.read (inode->s->di, block, blocksize);
	if (!u)
	{
		DEBUG (("block_getblk: leave (!u (%li, %li) -> 0)", block, blocksize));
		return 0;
	}
	
	((long *) u->data)[nr] = cpu2le32 (tmp);
	bio_MARK_MODIFIED (&bio, u);
	
	inode->i_next_alloc_block = new_block;
	inode->i_next_alloc_goal = tmp;
	inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
	inode->in.i_blocks = cpu2le32 (le2cpu32 (inode->in.i_blocks) + (blocksize >> 9));
	mark_inode_dirty (inode);
	
	if (clear_flag)
	{
		u = bio.getunit (inode->s->di, tmp, EXT2_BLOCK_SIZE (inode->s));
		if (u)
		{
			bzero (u->data, EXT2_BLOCK_SIZE (inode->s));
			bio_MARK_MODIFIED (&bio, u);
		}
		else
		{
			ALERT (("Ext2-FS: block_getblk: getunit fail!"));
			tmp = 0;
		}
	}
	
	DEBUG (("block_getblk: leave (tmp = %li)", tmp));
	return tmp;
}

long
ext2_getblk (COOKIE *inode, long block, long *err, ushort clear_flag)
{
	long addr_per_block = EXT2_ADDR_PER_BLOCK (inode->s);
	long addr_per_block_bits = EXT2_ADDR_PER_BLOCK_BITS (inode->s);
	long b = block;
	long u;	
	
	*err = EIO;
	
	DEBUG (("ext2_getblk: enter (#%li, block = %li)", inode->inode, block));
	
	if (block < 0)
	{
		ALERT (("Ext2-FS: ext2_getblk: block < 0"));
		return 0;
	}
	
	if (block > EXT2_NDIR_BLOCKS + addr_per_block +
		(1UL << (addr_per_block_bits << 1)) +
		((1UL << (addr_per_block_bits << 1)) << addr_per_block_bits))
	{
		ALERT (("Ext2-FS: ext2_getblk: block > big"));
		return 0;
	}
	
	/* If this is a sequential block allocation, set the next_alloc_block
	 * to this block now so that all the indblock and data block
	 * allocations use the same goal zone
	 */
	
	DEBUG (("block %lu, next %lu, goal %lu", block, 
		inode->i_next_alloc_block,
		inode->i_next_alloc_goal));
	
	if (block == inode->i_next_alloc_block + 1)
	{
		inode->i_next_alloc_block++;
		inode->i_next_alloc_goal++;
	}
	
	*err = ENOSPC;
	
	/* direct blocks */
	
	if (block < EXT2_NDIR_BLOCKS)
	{
		DEBUG (("ext2_getblk: leave (return inode_getblk)"));
		return inode_getblk (inode, block, b, err, clear_flag);
	}
	
	/* indirect blocks */
	
	block -= EXT2_NDIR_BLOCKS;
	if (block < addr_per_block)
	{
		u = inode_getblk (inode, EXT2_IND_BLOCK, b, err, 1);
		
		DEBUG (("ext2_getblk: leave (return block_getblk 1)"));
		return block_getblk (inode, u, block, b, err, clear_flag);
	}
	
	/* double indirect blocks */
	
	block -= addr_per_block;
	if (block < (1UL << (addr_per_block_bits << 1)))
	{
		u = inode_getblk (inode, EXT2_DIND_BLOCK, b, err, 1);
		u = block_getblk (inode, u, block >> addr_per_block_bits, b, err, 1);
		
		DEBUG (("ext2_getblk: leave (return block_getblk 2)"));
		return block_getblk (inode, u, block & (addr_per_block - 1), b, err, clear_flag);
	}
	
	/* triple indirect blocks */
	
	block -= (1UL << (addr_per_block_bits << 1));
	u = inode_getblk (inode, EXT2_TIND_BLOCK, b, err, 1);
	u = block_getblk (inode, u, block >> (addr_per_block_bits << 1), b, err, 1);
	u = block_getblk (inode, u, (block >> addr_per_block_bits) & (addr_per_block - 1), b, err, 1);
	
	DEBUG (("ext2_getblk: leave (return block_getblk 3)"));
	return block_getblk (inode, u, block & (addr_per_block - 1), b, err, clear_flag);
}

UNIT *
ext2_bread (COOKIE *inode, long block, long *err)
{
	long prev_blocks = le2cpu32 (inode->in.i_blocks);
	long nr;
	UNIT *u;
	
	DEBUG (("ext2_bread: enter (#%li, block = %li)", inode->inode, block));
	
	nr = ext2_getblk (inode, block, err, 1);
	if (!nr)
	{
		DEBUG (("ext2_bread: leave (!nr -> NULL)"));
		return NULL;
	}
	
	/* If the inode has grown, and this is a directory, then perform
	 * preallocation of a few more blocks to try to keep directory
	 * fragmentation down.
	 */
	if (EXT2_ISDIR (le2cpu16 (inode->in.i_mode))
		&& le2cpu32 (inode->in.i_blocks) > prev_blocks
		&& EXT2_HAS_COMPAT_FEATURE (inode->s, EXT2_FEATURE_COMPAT_DIR_PREALLOC))
	{
		long tmp;
		long i;
		
		for (i = 1; i < EXT2_SB (inode->s)->s_prealloc_dir_blocks; i++)
		{
			tmp = ext2_getblk (inode, block + i, err, 1);
			if (!tmp)
			{
				ext2_free_blocks (inode, block + i, 1);
				break;
			}
		}
	}
	
	u = bio.read (inode->s->di, nr, EXT2_BLOCK_SIZE (inode->s));
	
	DEBUG (("ext2_bread: leave (%li, %li -> %p)", nr, EXT2_BLOCK_SIZE (inode->s), u));
	return u;
}
