/*
 * Filename:     super.c
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
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

# include "super.h"

# include <mint/endian.h>

# include "balloc.h"
# include "ialloc.h"
# include "inode.h"


INLINE ulong
_log2 (ulong value)
{
	register ulong shift = 0;
	
	while (value > 1)
	{
		shift++;
		value >>= 1;
	}
	
	return shift;
}

INLINE __attribute__ ((const))
bool is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

INLINE void
ext2_setup_super (SI *s)
{
	ext2_sb *sb = s->sbi.s_sb;
	
	DEBUG (("Ext2-FS: ext2_setup_super: enter"));
	
	if (s->sbi.s_rev_level > EXT2_MAX_SUPP_REV)
	{
		ALERT (("Ext2-FS [%c]: WARNING: revision level too high, forcing read only mode", DriveToLetter(s->dev)));
		s->s_flags |= MS_RDONLY;
	}
	
	if (!(s->s_flags & MS_RDONLY))
	{
		if (!(le2cpu16 (sb->s_state) & EXT2_VALID_FS))
		{
			ALERT (("Ext2-FS [%c]: WARNING: mounting unchecked fs, running e2fsck is recommended", DriveToLetter(s->dev)));
		}
		else if (le2cpu16 (sb->s_state) & EXT2_ERROR_FS)
		{
			ALERT (("Ext2-FS [%c]: WARNING: mounting fs with errors, running e2fsck is recommended", DriveToLetter(s->dev)));
		}
		else if (le2cpu16 (sb->s_mnt_count) >= le2cpu16 (sb->s_max_mnt_count))
		{
			ALERT (("Ext2-FS [%c]: WARNING: maximal mount count reached, running e2fsck is recommended", DriveToLetter(s->dev)));
		}
		else if (le2cpu32 (sb->s_checkinterval) &&
			(le2cpu32 (sb->s_lastcheck) + le2cpu32 (sb->s_checkinterval) <= (ulong)CURRENT_TIME))
		{
			ALERT (("Ext2-FS [%c]: WARNING: checktime reached, running e2fsck is recommended", DriveToLetter(s->dev)));
		}
		
		sb->s_state = cpu2le16 (le2cpu16 (sb->s_state) & ~EXT2_VALID_FS);
		
		if (!(le2cpu16 (sb->s_max_mnt_count)))
		{
			sb->s_max_mnt_count = cpu2le16 (EXT2_DFL_MAX_MNT_COUNT);
		}
		
		sb->s_mnt_count = cpu2le16 (le2cpu16 (sb->s_mnt_count) + 1);
		sb->s_mtime = cpu2le32 (CURRENT_TIME);
		
		DEBUG (("2: sb = %p, u = %p, u->data = %p, u->size = %li", sb, s->sbi.s_sb_unit, s->sbi.s_sb_unit->data, s->sbi.s_sb_unit->size));
		bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);
		
		s->sbi.s_dirty = 1;
		
		if (test_opt (s, DEBUG))
		{
			DEBUG (("[EXT II FS %s, %s, bs=%li, fs=%li, gc=%li, "
				"bpg=%li, ipg=%li, mo=%04lx]\n",
				EXT2FS_VERSION,
				EXT2FS_DATE,
				s->sbi.s_blocksize,
				s->sbi.s_fragsize,
				s->sbi.s_groups_count,
				EXT2_BLOCKS_PER_GROUP (s),
				EXT2_INODES_PER_GROUP (s),
				s->sbi.s_mount_opt));
		}
		
		if (test_opt (s, CHECK))
		{
			ext2_check_blocks_bitmap (s);
			ext2_check_inodes_bitmap (s);
		}
	}
}

INLINE long
ext2_check_descriptors (SI *s)
{
	ulong block = s->sbi.s_first_data_block;
	ext2_gd *gdp = NULL;
	long desc_block = 0;
	long i;
	
	DEBUG (("Ext2-FS [%c]: Checking group descriptors", DriveToLetter(s->dev)));
	
	for (i = 0; i < s->sbi.s_groups_count; i++)
	{
		ulong tmp;
		
		if ((i % EXT2_DESC_PER_BLOCK (s)) == 0)
		{
			gdp = s->sbi.s_group_desc [desc_block++];
		}
		
		tmp = le2cpu32 (gdp->bg_block_bitmap);
		if (tmp < block || tmp >= block + EXT2_BLOCKS_PER_GROUP (s))
		{
			ALERT (("Ext2-FS: ext2_check_descriptors: Block bitmap for group %li"
				" not in group (block %li)!", i, tmp));
			
			return 0;
		}
		
		tmp = le2cpu32 (gdp->bg_inode_bitmap);
		if (tmp < block || tmp >= block + EXT2_BLOCKS_PER_GROUP (s))
		{
			ALERT (("Ext2-FS: ext2_check_descriptors: Inode bitmap for group %ld"
				" not in group (block %lu)!", i, tmp));
			
			return 0;
		}
		
		tmp = le2cpu32 (gdp->bg_inode_table);
		if (tmp < block || tmp + s->sbi.s_itb_per_group >= block + EXT2_BLOCKS_PER_GROUP (s))
		{
			ALERT (("Ext2-FS: ext2_check_descriptors: Inode table for group %ld"
				" not in group (block %lu)!", i, tmp));
			
			return 0;
		}
		
		block += EXT2_BLOCKS_PER_GROUP (s);
		gdp++;
	}
	
	return 1;
}

long
read_ext2_sb_info (ushort drv)
{
	SI *s;
	ext2_sb *sb = NULL;
	DI *di;
	UNIT *u;
	
	ulong blocksize;
	ulong sb_block;
	ulong sb_offset;
	
	
	DEBUG (("Ext2-FS [%c]: read_ext2_sb_info enter", DriveToLetter(drv)));
	
	di = bio.get_di (drv);
	if (!di)
	{
		DEBUG (("Ext2-FS [%c]: read_ext2_sb_info: get_di fail", DriveToLetter(drv)));
		return EBUSY;
	}
	
	blocksize = di->pssize;
	if (blocksize <= 1024)
	{
		/* physical sector size is smaller or equal
		 * to our minimum blocksize
		 * 
		 * mapping is set first to our minimum blocksize
		 * superblock starts at block 1 with offset 0
		 */
		blocksize = 1024;
		
		sb_block = 1;
		sb_offset = 0;
	}
	else
	{
		/* physical sector size is greater as our
		 * minimum blocksize
		 * 
		 * mapping is set first to the physical blocksize
		 * superblock starts at block 0 with offset 1024
		 */
		
		sb_block = 0;
		sb_offset = 1024;
	}
	
	/* set our physical/logical mapping */
	bio.set_lshift (di, blocksize);
	
	/* read in superblock sector */
	u = bio.read (di, sb_block, blocksize);
	if (u)
	{
		sb = (ext2_sb *) (u->data + sb_offset);
		
		DEBUG (("sb = %p, u->data = %p, u->size = %lu", sb, u->data, u->size));
		DEBUG (("sb_block = %lu, sb_offset = %lu, blocksize = %lu", sb_block, sb_offset, blocksize));
	}
	else
	{
		DEBUG (("bio.read (%p, %lu, %lu) fail", di, sb_block, blocksize));
	}
	
	if (sb && (sb->s_magic == cpu2le16 (EXT2_SUPER_MAGIC)))
	{
		/* check revision level
		 */
		
		if (le2cpu32 (sb->s_rev_level) > EXT2_GOOD_OLD_REV)
		{
					
			if (le2cpu32 (sb->s_feature_incompat) & ~EXT2_FEATURE_INCOMPAT_SUPP)
			{
				ALERT (("Ext2-FS [%c]: couldn't mount because of "
					"unsupported optional features.", DriveToLetter(drv)));
				
				goto leave;
			}
			
			if (!BIO_WP_CHECK (di) &&
				(le2cpu32 (sb->s_feature_ro_compat) & ~EXT2_FEATURE_RO_COMPAT_SUPP))
			{
				ALERT (("Ext2-FS [%c]: couldn't mount RDWR because of "
					"unsupported optional features.", DriveToLetter(drv)));
				
				goto leave;
			}
			
		}
		
		/* calculate final blocksize */
		blocksize = 1024 << le2cpu32 (sb->s_log_block_size);
		
		/* verify final blocksize */
		if ((blocksize != 1024)
			&& (blocksize != 2048)
			&& (blocksize != 4096))
		{
			ALERT (("Ext2-FS [%c]: Unsupported blocksize on dev!", DriveToLetter(drv)));
			
			goto leave;
		}
		
		/* verify physical blocksize <= logical blocksize */
		if (blocksize < di->pssize)
		{
			ALERT (("Ext2-FS [%c]: Unsupported blocksize on dev!", DriveToLetter(drv)));
			
			goto leave;
		}
		
		/* set up our final physical/logical mapping */
		bio.set_lshift (di, blocksize);
		
		/* change mapping -> sync and invalid the complete partition */
		sb = NULL;
		
		/* recalculate superblock and superblock offset */
		if (blocksize <= 1024)
		{
			sb_block = 1;
			sb_offset = 0;
		}
		else
		{
			sb_block = 0;
			sb_offset = 1024;
		}
		
		/* read in superblock and stay resident */
		u = bio.get_resident (di, sb_block, blocksize);
		if (u)
		{
			sb = (ext2_sb *) (u->data + sb_offset);
			
			DEBUG (("sb = %p, u->data = %p, u->size = %lu", sb, u->data, u->size));
			DEBUG (("sb_block = %lu, sb_offset = %lu, blocksize = %lu", sb_block, sb_offset, blocksize));
		}
		else
		{
			DEBUG (("bio.get_resident (%p, %lu, %lu) fail", di, sb_block, blocksize));
		}
	}	
	else
	{
		if (u)
			DEBUG (("Ext2-FS: read_ext2_sb_info: bad magic in step 1 (%x)", le2cpu16 (sb->s_magic)));
		else
			DEBUG (("Ext2-FS: read_ext2_sb_info: read fail"));
	}
	
	if (sb && (sb->s_magic == cpu2le16 (EXT2_SUPER_MAGIC)))
	{
		ushort resuid = EXT2_DEF_RESUID;
		ushort resgid = EXT2_DEF_RESGID;
		long i;
		
		
		/* allocate memory
		 */
		
		s = kmalloc (sizeof (*s));
		if (!s)
		{
			DEBUG (("Ext2-FS: read_ext2_sb_info: kmalloc fail (2)"));
			goto leave;
		}
		
		bzero (s, sizeof (*s));
		
		DEBUG (("sb = %p, u = %p, u->data = %p, u->size = %li", sb, u, u->data, u->size));
		
		s->sbi.s_sb_unit = u;
		s->sbi.s_sb = sb;
		
		
		/* set up mirrored superblock and
		 * adjust endian
		 */
		
		s->sbi.s_inodes_count		= le2cpu32 (sb->s_inodes_count);
		s->sbi.s_blocks_count		= le2cpu32 (sb->s_blocks_count);
		s->sbi.s_r_blocks_count		= le2cpu32 (sb->s_r_blocks_count);
/* u 		s->sbi.s_free_blocks_count	= le2cpu32 (sb->s_free_blocks_count);	*/
/* u 		s->sbi.s_free_inodes_count	= le2cpu32 (sb->s_free_inodes_count);	*/
		s->sbi.s_first_data_block	= le2cpu32 (sb->s_first_data_block);
/* nn		s->sbi.s_log_block_size		= le2cpu32 (sb->s_log_block_size);	*/
/* nn		s->sbi.s_log_frag_size		= le2cpu32 (sb->s_log_frag_size);	*/
		s->sbi.s_blocks_per_group	= le2cpu32 (sb->s_blocks_per_group);
		s->sbi.s_frags_per_group	= le2cpu32 (sb->s_frags_per_group);
		s->sbi.s_inodes_per_group	= le2cpu32 (sb->s_inodes_per_group);
/* u 		s->sbi.s_mtime			= le2cpu32 (sb->s_mtime);		*/
/* u 		s->sbi.s_wtime			= le2cpu32 (sb->s_wtime);		*/
/* u 		s->sbi.s_mnt_count		= le2cpu16 (sb->s_mnt_count);		*/
/* nn		s->sbi.s_max_mnt_count		= le2cpu16 (sb->s_max_mnt_count);	*/
/* nn		s->sbi.s_magic			= le2cpu16 (sb->s_magic);		*/
/* u 		s->sbi.s_state			= le2cpu16 (sb->s_state);		*/
		s->sbi.s_errors			= le2cpu16 (sb->s_errors);
		s->sbi.s_minor_rev_level	= le2cpu16 (sb->s_minor_rev_level);
/* nn		s->sbi.s_lastcheck		= le2cpu32 (sb->s_lastcheck);		*/
/* nn		s->sbi.s_checkinterval		= le2cpu32 (sb->s_checkinterval);	*/
/* nn		s->sbi.s_creator_os		= le2cpu32 (sb->s_creator_os);		*/
		s->sbi.s_rev_level		= le2cpu32 (sb->s_rev_level);
		
		if (resuid != EXT2_DEF_RESUID)
			s->sbi.s_resuid		= resuid;
		else
			s->sbi.s_resuid		= le2cpu16 (sb->s_def_resuid);
		
		if (resgid != EXT2_DEF_RESGID)
			s->sbi.s_resgid		= resgid;
		else
			s->sbi.s_resgid		= le2cpu16 (sb->s_def_resgid);
		
		
		/* set up values for dynamic
		 * superblock
		 */
		
		if (s->sbi.s_rev_level == EXT2_GOOD_OLD_REV)
		{
			s->sbi.s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
			s->sbi.s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
		}
		else
		{
			s->sbi.s_first_ino = le2cpu32 (sb->s_first_ino);
			s->sbi.s_inode_size = le2cpu16 (sb->s_inode_size);
			
			if ((s->sbi.s_inode_size < EXT2_GOOD_OLD_INODE_SIZE) ||
			     !is_power_of_2(s->sbi.s_inode_size))
			{
				ALERT (("Ext2-FS [%c]: unsupported inode size: %d",
					DriveToLetter(drv), s->sbi.s_inode_size));
				
				kfree (s, sizeof (*s));
				goto leave;
			}
		}
		
/* nn		s->sbi.s_block_group_nr		= le2cpu16 (sb->s_block_group_nr); */
		s->sbi.s_feature_compat		= le2cpu32 (sb->s_feature_compat);
		s->sbi.s_feature_incompat	= le2cpu32 (sb->s_feature_incompat);
		s->sbi.s_feature_ro_compat	= le2cpu32 (sb->s_feature_ro_compat);
/* nn		s->sbi.s_algorithm_use_bitmap	= le2cpu32 (sb->s_algorithm_use_bitmap); */
		s->sbi.s_prealloc_blocks	= sb->s_prealloc_blocks;
		s->sbi.s_prealloc_dir_blocks	= sb->s_prealloc_dir_blocks;
		
		
		/* setup calculated values
		 */
		
		s->sbi.s_blocksize_bits = 10 + le2cpu32 (sb->s_log_block_size);
		s->sbi.s_blocksize = 1UL << s->sbi.s_blocksize_bits;
		s->sbi.s_blocksize_mask = s->sbi.s_blocksize - 1;
		
		s->sbi.s_fragsize_bits = 10 + ABS (le2cpu32 (sb->s_log_frag_size));
		s->sbi.s_fragsize = 1UL << s->sbi.s_fragsize_bits;
		s->sbi.s_fragsize_mask = s->sbi.s_fragsize - 1;
		
		s->sbi.s_frags_per_block  = s->sbi.s_blocksize;
		s->sbi.s_frags_per_block /= s->sbi.s_fragsize;
		
		s->sbi.s_inodes_per_block  = s->sbi.s_blocksize;
		s->sbi.s_inodes_per_block /= sizeof (ext2_in);
		
		s->sbi.s_itb_per_group  = s->sbi.s_inodes_per_group;
		s->sbi.s_itb_per_group /= s->sbi.s_inodes_per_block;
		
		s->sbi.s_desc_per_block  = s->sbi.s_blocksize;
		s->sbi.s_desc_per_block /= sizeof (ext2_gd);
		s->sbi.s_desc_per_block_bits = _log2 (s->sbi.s_desc_per_block);
		s->sbi.s_desc_per_block_mask = s->sbi.s_desc_per_block - 1;
		
		s->sbi.s_groups_count  = s->sbi.s_blocks_count;
		s->sbi.s_groups_count -= s->sbi.s_first_data_block;
		s->sbi.s_groups_count += s->sbi.s_blocks_per_group;
		s->sbi.s_groups_count -= 1;
		s->sbi.s_groups_count /= s->sbi.s_blocks_per_group;
		
		s->sbi.s_db_per_group  = s->sbi.s_groups_count;
		s->sbi.s_db_per_group += s->sbi.s_desc_per_block;
		s->sbi.s_db_per_group -= 1;
		s->sbi.s_db_per_group /= s->sbi.s_desc_per_block;
		
		s->sbi.s_addr_per_block  = s->sbi.s_blocksize;
		s->sbi.s_addr_per_block /= sizeof (__u32);
		s->sbi.s_addr_per_block_bits = _log2 (s->sbi.s_addr_per_block);
		
		
		/* do some sanity checks
		 */
		
		if (!((s->sbi.s_blocksize == 1024)
			|| (s->sbi.s_blocksize == 2048)
			|| (s->sbi.s_blocksize == 4096)))
		{
			ALERT (("Ext2-FS [%c]: Unsupported blocksize on dev!", DriveToLetter(drv)));
			
			kfree (s, sizeof (*s));
			goto leave;
		}
		
		if ((s->sbi.s_blocksize != blocksize)
			|| (s->sbi.s_blocksize < di->pssize))
		{
			ALERT (("Ext2-FS [%c]: Unsupported blocksize on dev!", DriveToLetter(drv)));
			
			kfree (s, sizeof (*s));
			goto leave;
		}
		
		if (s->sbi.s_blocksize != s->sbi.s_fragsize)
		{
			ALERT (("Ext2-FS [%c]: fragsize %ld != blocksize %ld (not supported yet)",
				DriveToLetter(drv), s->sbi.s_fragsize, s->sbi.s_blocksize));
			
			kfree (s, sizeof (*s));
			goto leave;
		}
		
		if (s->sbi.s_blocks_per_group > s->sbi.s_blocksize * 8)
		{
			ALERT (("Ext2-FS [%c]: #blocks per group too big: %ld",
				DriveToLetter(drv), s->sbi.s_blocks_per_group));
			
			kfree (s, sizeof (*s));
			goto leave;
		}
		
		if (s->sbi.s_frags_per_group > s->sbi.s_blocksize * 8)
		{
			ALERT (("Ext2-FS [%c]: #fragments per group too big: %ld",
				DriveToLetter(drv), s->sbi.s_frags_per_group));
			
			kfree (s, sizeof (*s));
			goto leave;
		}
		
		if (s->sbi.s_inodes_per_group > s->sbi.s_blocksize * 8)
		{
			ALERT (("Ext2-FS [%c]: #inodes per group too big: %lu",
				DriveToLetter(drv), s->sbi.s_inodes_per_group));
			
			kfree (s, sizeof (*s));
			goto leave;
		}
		
		
		/* load group descriptor blocks
		 */
		
		s->sbi.s_group_desc_size = s->sbi.s_db_per_group * 2 * sizeof (void *);
		s->sbi.s_group_desc = kmalloc (s->sbi.s_group_desc_size);
		if (!s->sbi.s_group_desc)
		{
			DEBUG (("Ext2-FS: read_ext2_sb_info: kmalloc fail (3)"));
			
			kfree (s, sizeof (*s));
			goto leave;
		}
		
		s->sbi.s_group_desc_units = (UNIT **) (s->sbi.s_group_desc + s->sbi.s_db_per_group);
		
		for (i = 0; i < s->sbi.s_db_per_group; i++)
		{
			u = bio.get_resident (di, sb_block + 1 + i, s->sbi.s_blocksize);
			if (u)
			{
				s->sbi.s_group_desc_units [i] = u;
				s->sbi.s_group_desc [i] = (ext2_gd *) u->data;
			}
			else
			{
				DEBUG (("Ext2-FS: read_ext2_sb_info: read groups fail"));
				
				kfree (s->sbi.s_group_desc, s->sbi.s_group_desc_size);
				kfree (s, sizeof (*s));
				
				goto leave;
			}
		}
		
		s->sbi.s_dirty = 0;
		s->sbi.s_mount_opt = 0;
		
		if (!(le2cpu16 (sb->s_state) & EXT2_VALID_FS) || (le2cpu16 (sb->s_state) & EXT2_ERROR_FS))
			s->s_flags |= S_NOT_CLEAN_MOUNTED;
		
		if (BIO_WP_CHECK (di))
			s->s_flags |= MS_RDONLY;
		
		DEBUG (("Ext2-FS: s->sbi.s_inodes_count = %ld", s->sbi.s_inodes_count));
		DEBUG (("Ext2-FS: s->sbi.s_blocks_count = %ld", s->sbi.s_blocks_count));
		DEBUG (("Ext2-FS: s->sbi.s_r_blocks_count = %ld", s->sbi.s_r_blocks_count));
		DEBUG (("Ext2-FS: s->sbi.s_free_blocks_count = %ld", le2cpu32 (s->sbi.s_sb->s_free_blocks_count)));
		DEBUG (("Ext2-FS: s->sbi.s_free_inodes_count = %ld", le2cpu32 (s->sbi.s_sb->s_free_inodes_count)));
		DEBUG (("Ext2-FS: s->sbi.s_first_data_block = %ld", s->sbi.s_first_data_block));
		DEBUG (("Ext2-FS: s->sbi.s_blocks_per_group = %ld", s->sbi.s_blocks_per_group));
		DEBUG (("Ext2-FS: s->sbi.s_frags_per_group = %ld", s->sbi.s_frags_per_group));
		DEBUG (("Ext2-FS: s->sbi.s_inodes_per_group = %ld", s->sbi.s_inodes_per_group));
		DEBUG (("Ext2-FS: s->sbi.s_mtime = %ld", le2cpu32 (s->sbi.s_sb->s_mtime)));
		DEBUG (("Ext2-FS: s->sbi.s_wtime = %ld", le2cpu32 (s->sbi.s_sb->s_wtime)));
		DEBUG (("Ext2-FS: s->sbi.s_mnt_count = %d", le2cpu16 (s->sbi.s_sb->s_mnt_count)));
		DEBUG (("Ext2-FS: s->sbi.s_max_mnt_count = %d", le2cpu16 (s->sbi.s_sb->s_max_mnt_count)));
		DEBUG (("Ext2-FS: s->sbi.s_magic = %d", le2cpu16 (s->sbi.s_sb->s_magic)));
		DEBUG (("Ext2-FS: s->sbi.s_state = %d", le2cpu16 (s->sbi.s_sb->s_state)));
		DEBUG (("Ext2-FS: s->sbi.s_errors = %d", s->sbi.s_errors));
		DEBUG (("Ext2-FS: s->sbi.s_minor_rev_level = %d", s->sbi.s_minor_rev_level));
		DEBUG (("Ext2-FS: s->sbi.s_lastcheck = %ld", le2cpu32 (s->sbi.s_sb->s_lastcheck)));
		DEBUG (("Ext2-FS: s->sbi.s_checkinterval = %ld", le2cpu32 (s->sbi.s_sb->s_checkinterval)));
		DEBUG (("Ext2-FS: s->sbi.s_creator_os = %ld", le2cpu32 (s->sbi.s_sb->s_creator_os)));
		DEBUG (("Ext2-FS: s->sbi.s_rev_level = %ld", s->sbi.s_rev_level));
		DEBUG (("Ext2-FS: s->sbi.s_def_resuid = %d", le2cpu16 (s->sbi.s_sb->s_def_resuid)));
		DEBUG (("Ext2-FS: s->sbi.s_def_resgid = %d", le2cpu16 (s->sbi.s_sb->s_def_resgid)));
		DEBUG (("Ext2-FS: s->sbi.s_blocksize = %ld", s->sbi.s_blocksize));
		DEBUG (("Ext2-FS: s->sbi.s_blocksize_bits = %ld", s->sbi.s_blocksize_bits));
		DEBUG (("Ext2-FS: s->sbi.s_blocksize_mask = %ld", s->sbi.s_blocksize_mask));
		DEBUG (("Ext2-FS: s->sbi.s_fragsize = %ld", s->sbi.s_fragsize));
		DEBUG (("Ext2-FS: s->sbi.s_fragsize_bits = %ld", s->sbi.s_fragsize_bits));
		DEBUG (("Ext2-FS: s->sbi.s_fragsize_mask = %ld", s->sbi.s_fragsize_mask));
		DEBUG (("Ext2-FS: s->sbi.s_frags_per_block = %ld", s->sbi.s_frags_per_block));
		DEBUG (("Ext2-FS: s->sbi.s_inodes_per_block = %ld", s->sbi.s_inodes_per_block));
		DEBUG (("Ext2-FS: s->sbi.s_itb_per_group = %ld", s->sbi.s_itb_per_group));
		DEBUG (("Ext2-FS: s->sbi.s_db_per_group = %ld", s->sbi.s_db_per_group));
		DEBUG (("Ext2-FS: s->sbi.s_desc_per_block = %ld", s->sbi.s_desc_per_block));
		DEBUG (("Ext2-FS: s->sbi.s_groups_count = %ld", s->sbi.s_groups_count));
		DEBUG (("Ext2-FS: s->sbi.s_addr_per_block = %ld", s->sbi.s_addr_per_block));
		DEBUG (("Ext2-FS: s->sbi.s_addr_per_block_bits = %ld", s->sbi.s_addr_per_block_bits));
		
		DEBUG (("Ext2-FS: s->sbi.s_first_ino = %ld", s->sbi.s_first_ino));
		DEBUG (("Ext2-FS: s->sbi.s_inode_size = %d", s->sbi.s_inode_size));
		DEBUG (("Ext2-FS: s->sbi.s_feature_compat = %lx", s->sbi.s_feature_compat));
		DEBUG (("Ext2-FS: s->sbi.s_feature_incompat = %lx", s->sbi.s_feature_incompat));
		DEBUG (("Ext2-FS: s->sbi.s_feature_ro_compat = %lx", s->sbi.s_feature_ro_compat));
		DEBUG (("Ext2-FS: s->sbi.s_prealloc_blocks = %d", s->sbi.s_prealloc_blocks));
		DEBUG (("Ext2-FS: s->sbi.s_prealloc_dir_blocks = %d", s->sbi.s_prealloc_dir_blocks));
		
		s->di = di;
		s->dev = drv;
		s->rdev = drv;
		
		
		/* check group descriptors
		 */
		if (!ext2_check_descriptors (s))
		{
			DEBUG (("Ext2-FS: read_ext2_sb_info: ext2_check_descriptors fail"));
			
			kfree (s->sbi.s_group_desc, s->sbi.s_group_desc_size);
			kfree (s, sizeof (*s));
			
			goto leave;
		}
		
		
		/* read root cookie
		 */
		i = get_cookie (s, EXT2_ROOT_INO, &(s->root));
		if (i)
		{
			DEBUG (("Ext2-FS: read_ext2_sb_info: read root inode fail"));
			
			kfree (s->sbi.s_group_desc, s->sbi.s_group_desc_size);
			kfree (s, sizeof (*s));
			
			goto leave;
		}
		
		super [drv] = s;
		
		
		/* setup super block
		 */
		ext2_setup_super (s);
		
		
		/* enable write back mode
		 */
		bio.config (s->dev, BIO_WB, ENABLE);
		
		
		/* ok, here we can leave
		 */
		DEBUG (("Ext2-FS: read_ext2_sb_info: leave ok"));
		return E_OK;
	}
	else
	{
		if (u)
			DEBUG (("Ext2-FS: read_ext2_sb_info: bad magic in step 2 (%x)", le2cpu16 (sb->s_magic)));
		else
			DEBUG (("Ext2-FS: read_ext2_sb_info: read fail"));
	}
	
leave:
	/* free our device identifikator */
	bio.free_di (di);
	
	DEBUG (("Ext2-FS: read_ext2_sb_info: leave failure (EMEDIUMTYPE)"));
	return EMEDIUMTYPE;
}
