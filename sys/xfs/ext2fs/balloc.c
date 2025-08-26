/*
 * Filename:     balloc.c
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
 * balloc.c contains the blocks allocation and deallocation routines
 */

# include "balloc.h"

# include <mint/endian.h>

# include "bitmap.h"
# include "super.h"


INLINE void *
memscan (void *addr, long c, long size)
{
	uchar *p = (uchar *) addr;
	
	while (size)
	{
		if (*p == c)
			return (void *) p;
		p++;
		size--;
	}
	
  	return (void *) p;
}



# define in_range(b, first, len)	((b) >= (first) && (b) < (first) + (len))


INLINE UNIT *
load_block_bitmap (SI *s, ulong block_group)
{
	UNIT *u = NULL;
	ext2_gd *gdp;
	
	gdp = ext2_get_group_desc (s, block_group, NULL);
	if (gdp)
	{
		ulong block = le2cpu32 (gdp->bg_block_bitmap);
		
		u = bio.read (s->di, block, EXT2_BLOCK_SIZE (s));
		if (!u)
		{
			ALERT (("Ext2-FS: load_block_bitmap [%c]: "
				"Cannot read block bitmap - block_group = %ld, block_bitmap = %ld",
				DriveToLetter(s->dev), block_group, block));
		}
	}
	
	return u;
}


void
ext2_free_blocks (COOKIE *inode, ulong block, ulong count)
{
	SI *s = inode->s;
	ext2_gd *gdp;
	
	UNIT *u;
	UNIT *u2;
	
	ulong block_group;
	ulong bit;
	ulong overflow;
	ulong i;
	
	
	/* lock_super (s); */
	
	if (block < s->sbi.s_first_data_block
		|| (block + count) > s->sbi.s_blocks_count)
	{
		ALERT (("Ext2-FS: ext2_free_blocks [%c]: "
			"Freeing blocks not in datazone - "
			"block = %lu, count = %lu",
			DriveToLetter(s->dev), block, count));
		
		goto error_return;
	}
	
	DEBUG (("freeing blocks %lu, count = %lu", block, count));
	
do_more:
	overflow = 0;
	block_group = (block - s->sbi.s_first_data_block) / EXT2_BLOCKS_PER_GROUP (s);
	bit = (block - s->sbi.s_first_data_block) % EXT2_BLOCKS_PER_GROUP (s);
	
	/* Check to see if we are freeing blocks across a group
	 * boundary.
	 */
	if (bit + count > EXT2_BLOCKS_PER_GROUP (s))
	{
		overflow = bit + count - EXT2_BLOCKS_PER_GROUP (s);
		count -= overflow;
	}
	
	u = load_block_bitmap (s, block_group);
	if (!u)
		goto error_return;
	
	gdp = ext2_get_group_desc (s, block_group, &u2);
	if (!gdp)
		goto error_return;
	
	if (test_opt (s, CHECK_STRICT)
		&& (in_range (le2cpu32 (gdp->bg_block_bitmap), block, count)
			|| in_range (le2cpu32 (gdp->bg_inode_bitmap), block, count)
			|| in_range (block, le2cpu32 (gdp->bg_inode_table), s->sbi.s_itb_per_group)
			|| in_range (block + count - 1, le2cpu32 (gdp->bg_inode_table), s->sbi.s_itb_per_group)))
	{
		FATAL ("ext2_free_blocks: "
			"Freeing blocks in system zones - "
			"Block = %lu, count = %lu", block, count);
	}
	
	for (i = 0; i < count; i++)
	{
		if (!ext2_clear_bit (bit + i, u->data))
		{
			ALERT (("Ext2-FS: ext2_free_blocks [%c]: "
				"bit already cleared for block %lu", DriveToLetter(s->dev), block));
		}
		else
		{
			gdp->bg_free_blocks_count =
				cpu2le16 (le2cpu16 (gdp->bg_free_blocks_count) + 1);
			s->sbi.s_sb->s_free_blocks_count =
				cpu2le32 (le2cpu32 (s->sbi.s_sb->s_free_blocks_count) + 1);
		}
	}
	
	bio_MARK_MODIFIED (&bio, u);
	bio_MARK_MODIFIED (&bio, u2);
	bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);
	
	if (overflow)
	{
		block += count;
		count = overflow;
		goto do_more;
	}
	
	s->sbi.s_dirty = 1;
	
error_return:
	/* unlock_super (s); */
	return;
}

/*
 * ext2_new_block uses a goal block to assist allocation.  If the goal is
 * free, or there is a free block within 32 blocks of the goal, that block
 * is allocated.  Otherwise a forward search is made for a free block; within 
 * each block group the search first looks for an entire free byte in the block
 * bitmap, and then for any free bit if that fails.
 */
long
ext2_new_block (COOKIE *inode, ulong goal, ulong *prealloc_count, ulong *prealloc_block, long *err)
{
	SI *s = inode->s;
	ext2_gd *gdp;
	
	UNIT *u;
	UNIT *u2;
	
	char *p, *r;
	long i, j, k, tmp;
	
# ifdef EXT2FS_DEBUG
	static long goal_hits = 0;
	static long goal_attempts = 0;
# endif
	
	
	*err = EACCES;
	
	/* lock_super (s); */
	
	if (le2cpu32 (s->sbi.s_sb->s_free_blocks_count) <= le2cpu32 (s->sbi.s_sb->s_r_blocks_count)
# if 1
		&& ((s->sbi.s_resuid != p_geteuid ())
			&& (s->sbi.s_resgid == 0 || s->sbi.s_resgid != p_getegid ()))
# else
		&& ((s->sbi.s_resuid != g_eteuid ())
			&& (s->sbi.s_resgid == 0 || !in_group_p (s->sbi.s_resgid))
			&& !capable (CAP_SYS_RESOURCE))
# endif
	)
	{
		DEBUG (("ext2_new_block: no free user blocks!"));
		
		/* unlock_super (s); */
		return 0;
	}
	
	if (!s->sbi.s_sb->s_free_blocks_count)
	{
		DEBUG (("ext2_new_block: no free blocks!"));
		
		/* unlock_super (s); */
		return 0;
	}
	
	DEBUG (("goal = %lu", goal));
	
repeat:
	/*
	 * First, test whether the goal block is free.
	 */
	if (goal < s->sbi.s_first_data_block || goal >= s->sbi.s_blocks_count)
	{
		goal = s->sbi.s_first_data_block;
	}
	
	i = (goal - s->sbi.s_first_data_block) / EXT2_BLOCKS_PER_GROUP (s);
	gdp = ext2_get_group_desc (s, i, &u2);
	if (!gdp)
		goto io_error;
	
	if (le2cpu16 (gdp->bg_free_blocks_count) > 0)
	{
		j = (goal - s->sbi.s_first_data_block) % EXT2_BLOCKS_PER_GROUP (s);
# ifdef EXT2FS_DEBUG
		if (j)
			goal_attempts++;
# endif
		u = load_block_bitmap (s, i);
		if (!u)
			goto io_error;
		
		DEBUG (("goal is at %ld:%ld", i, j));
		
		if (!ext2_test_bit (j, u->data))
		{
# ifdef EXT2FS_DEBUG
			goal_hits++;
			DEBUG (("goal bit allocated"));
# endif
			goto got_block;
		}
		
		if (j)
		{
			/*
			 * The goal was occupied; search forward for a free 
			 * block within the next XX blocks.
			 *
			 * end_goal is more or less random, but it has to be
			 * less than EXT2_BLOCKS_PER_GROUP. Aligning up to the
			 * next 64-bit boundary is simple..
			 */
			long end_goal = (j + 63) & ~63;
			j = ext2_find_next_zero_bit (u->data, end_goal, j);
			if (j < end_goal)
				goto got_block;
		}
		
		DEBUG (("Bit not found near goal"));
		
		/*
		 * There has been no free block found in the near vicinity
		 * of the goal: do a search forward through the block groups,
		 * searching in each group first for an entire free byte in
		 * the bitmap and then for any free bit.
		 * 
		 * Search first in the remainder of the current group; then,
		 * cyclicly search through the rest of the groups.
		 */
		p = (char *)u->data + (j >> 3);
		r = memscan (p, 0, (EXT2_BLOCKS_PER_GROUP (s) - j + 7) >> 3);
		k = ((unsigned long)r - (unsigned long)u->data) << 3;
		if (k < EXT2_BLOCKS_PER_GROUP (s))
		{
			j = k;
			goto search_back;
		}
		
		k = ext2_find_next_zero_bit ((ulong *) u->data, EXT2_BLOCKS_PER_GROUP (s), j);
		if (k < EXT2_BLOCKS_PER_GROUP (s))
		{
			j = k;
			goto got_block;
		}
	}
	
	DEBUG (("Bit not found in block group %ld", i));
	
	/*
	 * Now search the rest of the groups.  We assume that 
	 * i and gdp correctly point to the last group visited.
	 */
	for (k = 0; k < s->sbi.s_groups_count; k++)
	{
		i++;
		if (i >= s->sbi.s_groups_count)
			i = 0;
		
		gdp = ext2_get_group_desc (s, i, &u2);
		if (!gdp)
		{
			*err = EREAD;
			/* unlock_super (s); */
			return 0;
		}
		
		if (le2cpu16 (gdp->bg_free_blocks_count) > 0)
			break;
	}
	
	if (k >= s->sbi.s_groups_count)
	{
		/* unlock_super (s); */
		return 0;
	}
	
	u = load_block_bitmap (s, i);
	if (!u)
		goto io_error;
	
	r = memscan (u->data, 0, EXT2_BLOCKS_PER_GROUP (s) >> 3);
	j = ((unsigned long)r - (unsigned long)u->data) << 3;
	if (j < EXT2_BLOCKS_PER_GROUP (s))
	{
		goto search_back;
	}
	else
	{
		j = ext2_find_first_zero_bit ((ulong *) u->data, EXT2_BLOCKS_PER_GROUP (s));
	}
	
	if (j >= EXT2_BLOCKS_PER_GROUP (s))
	{
		ALERT (("Ext2-FS: ext2_new_block [%c]: "
			"Free blocks count corrupted for block group %ld", DriveToLetter(s->dev), i));
		
		/* unlock_super (s); */
		return 0;
	}
	
search_back:
	/* 
	 * We have succeeded in finding a free byte in the block
	 * bitmap.  Now search backwards up to 7 bits to find the
	 * start of this group of free blocks.
	 */
	for (k = 0; k < 7 && j > 0 && !ext2_test_bit (j - 1, u->data); k++, j--);
	
got_block:
	
	DEBUG (("using block group %ld(%d)", i, le2cpu16 (gdp->bg_free_blocks_count)));
	
	tmp = j + i * EXT2_BLOCKS_PER_GROUP (s) + s->sbi.s_first_data_block;
	
	if (test_opt (s, CHECK_STRICT)
		&& (tmp == le2cpu32 (gdp->bg_block_bitmap)
			|| tmp == le2cpu32 (gdp->bg_inode_bitmap)
			|| in_range (tmp, le2cpu32 (gdp->bg_inode_table), s->sbi.s_itb_per_group)))
	{
		FATAL ("ext2_new_block: "
			"Allocating block in system zone - "
			"block = %lu", tmp);
	}
	
	if (ext2_set_bit(j, u->data))
	{
		ALERT (("Ext2-FS: ext2_new_block [%c]: bit already set for block %ld", DriveToLetter(s->dev), j));
		goto repeat;
	}
	
	DEBUG (("found bit %ld", j));
	
	/*
	 * Do block preallocation now if required.
	 */
# ifdef EXT2_PREALLOCATE
	if (prealloc_block)
	{
		long prealloc_goal;
		
		prealloc_goal = s->sbi.s_prealloc_blocks ?
			s->sbi.s_prealloc_blocks : EXT2_DEFAULT_PREALLOC_BLOCKS;
		
		*prealloc_count = 0;
		*prealloc_block = tmp + 1;
		
		for (k = 1; k < prealloc_goal && (j + k) < EXT2_BLOCKS_PER_GROUP (s); k++)
		{
			if (ext2_set_bit (j + k, u->data))
			{
				break;
			}
			(*prealloc_count)++;
		}
		
		gdp->bg_free_blocks_count =
			cpu2le16 (le2cpu16 (gdp->bg_free_blocks_count) - *prealloc_count);
		
		s->sbi.s_sb->s_free_blocks_count =
			cpu2le32 (le2cpu32 (s->sbi.s_sb->s_free_blocks_count) - *prealloc_count);
		
		DEBUG (("Preallocated a further %lu bits", *prealloc_count));
	}
# endif
	
	j = tmp;
	
	bio_MARK_MODIFIED (&bio, u);
	
	if (j >= s->sbi.s_blocks_count)
	{
		ALERT (("Ext2-FS: ext2_new_block [%c]: block >= blocks count - "
			"block_group = %ld, block = %ld", DriveToLetter(s->dev), i, j));
		
		/* unlock_super (s); */
		return 0;
	}
	
	DEBUG (("allocating block %ld: Goal hits %ld of %ld", j, goal_hits, goal_attempts));
	
	gdp->bg_free_blocks_count = cpu2le16 (le2cpu16 (gdp->bg_free_blocks_count) - 1);
	bio_MARK_MODIFIED (&bio, u2);
	
	s->sbi.s_sb->s_free_blocks_count = cpu2le32 (le2cpu32 (s->sbi.s_sb->s_free_blocks_count) - 1);
	bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);
	
	s->sbi.s_dirty = 1;
	*err = E_OK;
	
	/* unlock_super (s); */
	return j;
	
io_error:
	*err = EREAD;
	
	/* unlock_super (s); */
	return 0;
	
}


INLINE long
block_in_use (ulong block, SI *s, uchar *map)
{
	return ext2_test_bit ((block - le2cpu32 (s->sbi.s_sb->s_first_data_block))
		% EXT2_BLOCKS_PER_GROUP (s), map);
}

INLINE long
test_root (long a, long b)
{
	if (a == 0)
		return 1;
	
	while (1)
	{
		if (a == 1)
			return 1;
		
		if (a % b)
			return 0;
		
		a = a / b;
	}
}

long
ext2_group_sparse (long group)
{
	return (test_root (group, 3)
		|| test_root (group, 5)
		|| test_root (group, 7));
}

void
ext2_check_blocks_bitmap (SI *s)
{
	ulong desc_count = 0;
	ulong bitmap_count = 0;
	ulong desc_blocks;
	long i;
	
	
	/* lock_super (s); */
	
	desc_blocks = (s->sbi.s_groups_count + EXT2_DESC_PER_BLOCK_MASK (s)) >> EXT2_DESC_PER_BLOCK_BITS (s);
	for (i = 0; i < s->sbi.s_groups_count; i++)
	{
		ext2_gd *gdp;
		UNIT *u;
		ulong free_blocks;
		long j;
		
		gdp = ext2_get_group_desc (s, i, NULL);
		if (!gdp)
			continue;
		
		free_blocks = le2cpu16 (gdp->bg_free_blocks_count);
		desc_count += free_blocks;
		
		u = load_block_bitmap (s, i);
		if (!u)
			continue;
		
		if (!(s->sbi.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)
			|| ext2_group_sparse (i))
		{
			if (!ext2_test_bit (0, u->data))
			{
				ALERT (("Ext2-FS: ext2_check_blocks_bitmap [%c]: "
					"Superblock in group %ld "
					"is marked free", DriveToLetter(s->dev), i));
			}
			
			for (j = 0; j < desc_blocks; j++)
			{
				if (!ext2_test_bit (j + 1, u->data))
				{
					ALERT (("Ext2-FS: ext2_check_blocks_bitmap [%c]: "
						"Descriptor block #%ld in group "
						"%ld is marked free", DriveToLetter(s->dev), j, i));
				}
			}
		}
		
		if (!block_in_use (le2cpu32 (gdp->bg_block_bitmap), s, u->data))
		{
			ALERT (("Ext2-FS: ext2_check_blocks_bitmap [%c]: "
				"Block bitmap for group %ld is marked free", DriveToLetter(s->dev), i));
		}
		
		if (!block_in_use (le2cpu32 (gdp->bg_inode_bitmap), s, u->data))
		{
			ALERT (("Ext2-FS: ext2_check_blocks_bitmap [%c]: "
				"Inode bitmap for group %ld is marked free", DriveToLetter(s->dev), i));
		}
		
		for (j = 0; j < s->sbi.s_itb_per_group; j++)
		{
			if (!block_in_use (le2cpu32 (gdp->bg_inode_table) + j, s, u->data))
			{
				ALERT (("Ext2-FS: ext2_check_blocks_bitmap [%c]: "
					"Block #%ld of the inode table in "
					"group %ld is marked free", DriveToLetter(s->dev), j, i));
			}
		}
		
		{
			ulong x;
			
			x = ext2_count_free ((char *)u->data, EXT2_BLOCK_SIZE (s));
			if (free_blocks != x)
			{
				ALERT (("Ext2-FS: ext2_check_blocks_bitmap [%c]: Wrong free blocks count for group %ld, "
					"stored = %lu, counted = %lu", DriveToLetter(s->dev), i, free_blocks, x));
			}
			
			bitmap_count += x;
		}
	}
	
	if (le2cpu32 (s->sbi.s_sb->s_free_blocks_count) != bitmap_count)
	{
		ALERT (("Ext2-FS: ext2_check_blocks_bitmap [%c]: "
			"Wrong free blocks count in super block, "
			"stored = %lu, counted = %lu",
			DriveToLetter(s->dev), (ulong) le2cpu32 (s->sbi.s_sb->s_free_blocks_count), bitmap_count));
	}
	
	/* unlock_super (s); */
}
