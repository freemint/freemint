/*
 * Filename:     ialloc.c
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
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

/*
 * ialloc.c contains the inodes allocation and deallocation routines
 */

# include "ialloc.h"

# include <mint/endian.h>

# include "bitmap.h"
# include "inode.h"
# include "super.h"


INLINE UNIT *
load_inode_bitmap (SI *s, ulong block_group)
{
	UNIT *u = NULL;
	ext2_gd *gdp;
	
	gdp = ext2_get_group_desc (s, block_group, NULL);
	if (gdp)
	{
		ulong block = le2cpu32 (gdp->bg_inode_bitmap);
		
		u = bio.read (s->di, block, s->sbi.s_blocksize);
		if (!u)
		{
			ALERT (("Ext2-FS: read_inode_bitmap [%c]: "
				"Cannot read inode bitmap - block_group = %ld, inode_bitmap = %ld",
				DriveToLetter(s->dev), block_group, block));
		}
	}
	
	return u;
}
static void
donothing(void)
{}
void
ext2_free_inode (COOKIE *inode)
{
	SI *s = inode->s;
	UNIT *u;
	
	ulong ino = inode->inode;
	ulong block_group;
	ulong bit;
	
	
	DEBUG (("ext2_free_inode [%c]: enter #%li", DriveToLetter(inode->dev), ino));
	
	if (SYNC_COOKIE (inode))
	{
		ALERT (("Ext2-FS: ext2_free_inode [%c]: can't sync inode #%li", DriveToLetter(inode->dev), ino));
		return;
	}
	if (inode->links > 0)
	{
		ALERT (("Ext2-FS: ext2_free_inode [%c]: inode #%li has count = %ld\n", DriveToLetter(inode->dev), ino, inode->links));
		return;
	}
	if (inode->in.i_links_count)
	{
		ALERT (("Ext2-FS: ext2_free_inode [%c]: inode #%li has nlink = %d\n", DriveToLetter(inode->dev), ino, le2cpu16 (inode->in.i_links_count)));
		return;
	}
	if (ino < EXT2_FIRST_INO (s) || ino > s->sbi.s_inodes_count)
	{
		ALERT (("Ext2-FS: ext2_free_inode [%c]: reserved or nonexistent inode #%li", DriveToLetter(inode->dev), ino));
		return;
	}
	
	block_group = (ino - 1) / EXT2_INODES_PER_GROUP (s);
	bit = (ino - 1) % EXT2_INODES_PER_GROUP (s);
	
	/* lock_super (s); */
	
	u = load_inode_bitmap (s, block_group);
	if (!u)
		goto error_return;
	
	if (!ext2_clear_bit (bit, u->data))
	{
		ALERT (("Ext2-FS: ext2_free_inode [%c]: bit already cleared for inode #%li: %p", DriveToLetter(s->dev), ino, inode));
	}
	else
	{
		ext2_gd *gdp;
		
		bio_MARK_MODIFIED (&bio, u);
		
		gdp = ext2_get_group_desc (s, block_group, &u);
		if (gdp)
		{
			gdp->bg_free_inodes_count =
				cpu2le16 (le2cpu16 (gdp->bg_free_inodes_count) + 1);
			
			if (EXT2_ISDIR (le2cpu16 (inode->in.i_mode)))
			{
				gdp->bg_used_dirs_count =
					cpu2le16 (le2cpu16 (gdp->bg_used_dirs_count) - 1);
			}
		}
		
		bio_MARK_MODIFIED (&bio, u);
		
		s->sbi.s_sb->s_free_inodes_count =
			cpu2le32 (le2cpu32 (s->sbi.s_sb->s_free_inodes_count) + 1);
		
		bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);
		
		s->sbi.s_dirty = 1;
	}
	
	/* remove from inode cache */
	del_cookie (inode);

error_return:
	donothing();
	/* unlock_super (s); */
}

/* This function increments the inode version number
 * This may be used one day by the NFS server
 */
INLINE void
inc_inode_version (COOKIE *inode, ext2_gd *gdp, long mode)
{
	inode->in.i_version = cpu2le32 (le2cpu32 (inode->in.i_version) + 1);
	mark_inode_dirty (inode);
}

/* There are two policies for allocating an inode.  If the new inode is
 * a directory, then a forward search is made for a block group with both
 * free space and a low directory-to-inode ratio; if that fails, then of
 * the groups with above-average free space, that group with the fewest
 * directories already is chosen.
 *
 * For other inodes, search forward from the parent directory's block
 * group to find a free inode.
 */
COOKIE *
ext2_new_inode (COOKIE *dir, long mode, long *err)
{
	SI *s = dir->s;
	ext2_gd *gdp;
	ext2_gd *tmp;
	
	UNIT *u;
	UNIT *u2 = 0;
	
	COOKIE *inode;
	
	long i;
	long j;
	long avefreei;
	
	
	/* Cannot create files in a deleted directory
	 */
	if (!dir || !dir->in.i_links_count)
	{
		*err = EACCES;
		return NULL;
	}
	
	inode = get_new_cookie ();
	if (!inode)
	{
		*err = ENOMEM;
		return NULL;
	}
	
	inode->s = s;
	inode->i_flags = 0;
	
	/* lock_super (s); */
	
repeat:
	gdp = NULL; i = 0;
	
	*err = ENOSPC;
	if (EXT2_ISDIR (mode))
	{
		avefreei = le2cpu32 (s->sbi.s_sb->s_free_inodes_count) / s->sbi.s_groups_count;
		if (!gdp)
		{
			for (j = 0; j < s->sbi.s_groups_count; j++)
			{
				tmp = ext2_get_group_desc (s, j, &u2);
				if (tmp
					&& le2cpu16 (tmp->bg_free_inodes_count)
					&& le2cpu16 (tmp->bg_free_inodes_count) >= avefreei)
				{
					if (!gdp || (le2cpu16 (tmp->bg_free_blocks_count) > le2cpu16 (gdp->bg_free_blocks_count)))
					{
						i = j;
						gdp = tmp;
					}
				}
			}
		}
	}
	else 
	{
		/* Try to place the inode in its parent directory
		 */
		i = dir->i_block_group;
		tmp = ext2_get_group_desc (s, i, &u2);
		if (tmp && le2cpu16 (tmp->bg_free_inodes_count))
		{
			gdp = tmp;
		}
		else
		{
			/* Use a quadratic hash to find a group with a
			 * free inode
			 */
			for (j = 1; j < s->sbi.s_groups_count; j <<= 1)
			{
				i += j;
				if (i >= s->sbi.s_groups_count)
					i -= s->sbi.s_groups_count;
				
				tmp = ext2_get_group_desc (s, i, &u2);
				if (tmp && le2cpu16 (tmp->bg_free_inodes_count))
				{
					gdp = tmp;
					break;
				}
			}
		}
		
		if (!gdp)
		{
			/* That failed: try linear search for a free inode
			 */
			i = dir->i_block_group + 1;
			for (j = 2; j < s->sbi.s_groups_count; j++)
			{
				if (++i >= s->sbi.s_groups_count)
					i = 0;
				
				tmp = ext2_get_group_desc (s, i, &u2);
				if (tmp && le2cpu16 (tmp->bg_free_inodes_count))
				{
					gdp = tmp;
					break;
				}
			}
		}
	}
	
	if (!gdp)
		goto error;
	
	u = load_inode_bitmap (s, i);
	if (!u)
	{
		*err = EREAD;
		goto error;
	}
	
	if ((j = ext2_find_first_zero_bit ((ulong *) u->data,
			EXT2_INODES_PER_GROUP (s))) < EXT2_INODES_PER_GROUP(s))
	{
		if (ext2_set_bit (j, u->data))
		{
			ALERT (("Ext2-FS: ext2_new_inode [%c]: "
				"bit already set for inode %ld", DriveToLetter(dir->dev), j));
			
			goto repeat;
		}
		
		bio_MARK_MODIFIED (&bio, u);
	}
	else
	{
		if (le2cpu16 (gdp->bg_free_inodes_count) != 0)
		{
			ALERT (("Ext2-FS: ext2_new_inode [%c]: "
				"Free inodes count corrupted in group %ld", DriveToLetter(dir->dev), i));
			
			goto error;
		}
		
		goto repeat;
	}
	
	j += i * EXT2_INODES_PER_GROUP (s) + 1;
	if (j < EXT2_FIRST_INO (s) || j > s->sbi.s_inodes_count)
	{
		ALERT (("Ext2-FS: ext2_new_inode [%c]: "
			"reserved inode or inode > inodes count - "
			"block_group = %ld, inode = %ld", DriveToLetter(dir->dev), i, j));
		
		goto error;
	}
	
	gdp->bg_free_inodes_count =
		cpu2le16 (le2cpu16 (gdp->bg_free_inodes_count) - 1);
	
	if (EXT2_ISDIR (mode))
		gdp->bg_used_dirs_count =
			cpu2le16 (le2cpu16 (gdp->bg_used_dirs_count) + 1);
	
	bio_MARK_MODIFIED (&bio, u2);
	
	s->sbi.s_sb->s_free_inodes_count =
		cpu2le32 (le2cpu32 (s->sbi.s_sb->s_free_inodes_count) - 1);
	
	bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);
	
	s->sbi.s_dirty = 1;
	
	
	inode->inode = j;
	/* inode->s = s; already set */
	inode->dev = s->dev;
	inode->rdev = s->rdev;
	
	inode->in.i_mode = cpu2le16 (mode);
	inode->in.i_links_count = cpu2le16 (1);
	inode->in.i_uid = cpu2le16 (p_geteuid ());
	
	if (test_opt (s, GRPID))
	{
		inode->in.i_gid = dir->in.i_gid;
	}
	else if (le2cpu16 (dir->in.i_mode) & EXT2_ISGID)
	{
		inode->in.i_gid = dir->in.i_gid;
		if (EXT2_ISDIR (mode))
			mode |= EXT2_ISGID;
	}
	else
	{
		inode->in.i_gid = cpu2le16 (p_getegid ());
	}
	
	inode->in.i_blocks = 0;
	inode->in.i_mtime = inode->in.i_atime = inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
	inode->in.i_flags = le2cpu32 (dir->in.i_flags);
	
	if (EXT2_ISLNK (mode))
		inode->in.i_flags &= ~(EXT2_IMMUTABLE_FL | EXT2_APPEND_FL);
	
	if (inode->in.i_flags & EXT2_SYNC_FL)
		inode->i_flags |= MS_SYNCHRONOUS;
	
	if (inode->in.i_flags & EXT2_APPEND_FL)
		inode->i_flags |= S_APPEND;
	
	if (inode->in.i_flags & EXT2_IMMUTABLE_FL)
		inode->i_flags |= S_IMMUTABLE;
	
	if (inode->in.i_flags & EXT2_NOATIME_FL)
		inode->i_flags |= MS_NOATIME;
	
	inode->in.i_flags = cpu2le32 (inode->in.i_flags);
	
	inode->i_block_group = i;
	inode->i_next_alloc_block = 0;
	inode->i_next_alloc_goal = 0;
	inode->i_prealloc_block = 0;
	inode->i_prealloc_count = 0;
	
	cookie_install (inode);
	inc_inode_version (inode, gdp, mode);
	
	mark_inode_dirty (inode);
	
	/* unlock_super (s); */
	DEBUG (("ext2_new_inode: allocating inode #%lu", inode->inode));
	
	*err = E_OK;
	return inode;

error:
	/* unlock_super (s); */
	
	del_cookie (inode);
	return NULL;
}

void
ext2_check_inodes_bitmap (SI * s)
{
	ulong desc_count = 0;
	ulong bitmap_count = 0;
	long i;
	
	
	/* lock_super (s); */
	
	for (i = 0; i < s->sbi.s_groups_count; i++)
	{
		ext2_gd *gdp;
		UNIT *u;
		ulong free_count;
		
		gdp = ext2_get_group_desc (s, i, NULL);
		if (!gdp)
			continue;
		
		free_count = le2cpu16 (gdp->bg_free_inodes_count);
		desc_count += free_count;
		
		u = load_inode_bitmap (s, i);
		if (!u)
			continue;
		
		{
			ulong x;
			
			x = ext2_count_free ((char *)u->data, EXT2_INODES_PER_GROUP (s) / 8);
			if (x != free_count)
			{
				ALERT (("Ext2-FS: ext2_check_inodes_bitmap [%c]: "
					"Wrong free inodes count in group %ld, "
					"stored = %lu, counted = %lu",
					DriveToLetter(s->dev), i, free_count, x));
			}
			
			bitmap_count += x;
		}
	}
	
	if (le2cpu32 (s->sbi.s_sb->s_free_inodes_count) != bitmap_count)
	{
		ALERT (("Ext2-FS: ext2_check_inodes_bitmap [%c]: "
			"Wrong free inodes count in super block, "
			"stored = %ld, counted = %ld",
			DriveToLetter(s->dev), le2cpu32 (s->sbi.s_sb->s_free_inodes_count), bitmap_count));
	}
	
	/* unlock_super (s); */
}
