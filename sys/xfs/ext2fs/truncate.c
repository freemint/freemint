/*
 * Filename:     truncate.c
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

# include "truncate.h"

# include <mint/endian.h>

# include "balloc.h"
# include "inode.h"


INLINE void
clear_entry (COOKIE *inode, ulong block, ulong offset)
{
	if (block)
	{
		register UNIT *u;
		
		u = bio.read (inode->s->di, block, EXT2_BLOCK_SIZE (inode->s));
		if (u)
		{
			*((__u32 *) u->data + offset) = 0;
			bio_MARK_MODIFIED (&bio, u);
		}
		else
		{
			DEBUG (("clear_entry: #%li: %lu, %lu", inode->inode, block, offset));
		}
	}
	else
	{
		inode->in.i_block [offset] = 0;
		mark_inode_dirty (inode);
	}
}

INLINE long
read_entry (COOKIE *inode, ulong block, ulong offset, ulong *entry)
{
	if (block)
	{
		register UNIT *u;
		
		u = bio.read (inode->s->di, block, EXT2_BLOCK_SIZE (inode->s));
		if (!u)
		{
			DEBUG (("read_entry: #%li: %lu, %lu", inode->inode, block, offset));
			return 1;
		}
		
		*entry = le2cpu32 (*((__u32 *) u->data + offset));
	}
	else
		*entry = le2cpu32 (inode->in.i_block [offset]);
	
	return 0;
}

static ulong
check_block_empty (COOKIE *inode, ulong block, ulong parent, ulong parent_offset)
{
	UNIT *u;
	__u32 *ind;
	int i;
	
	DEBUG (("check_block_empty: #%li: %lu, %lu, %lu", inode->inode, block, parent, parent_offset));
	
	u = bio.read (inode->s->di, block, EXT2_BLOCK_SIZE (inode->s));
	if (!u)
	{
		DEBUG (("check_block_empty: #%li: read failure", inode->inode));
		return 1;
	}
	
	ind = (__u32 *) u->data;
	for (i = 0; i < EXT2_ADDR_PER_BLOCK (inode->s); i++)
	{
		if (*(ind++))
		{
			DEBUG (("check_block_empty: #%li: clear failure", inode->inode));
			return 1;
		}
	}
	
	if (bio.revision > 1)
		bio.remove (u);
	
# ifdef EXT2FS_DEBUG
	{
		ulong tmp;
		
		if (!read_entry (inode, parent, parent_offset, &tmp))
			ASSERT ((tmp == block));
		else
			DEBUG (("read failure in check_block_empty"));
	}
# endif
	
	clear_entry (inode, parent, parent_offset);
	
	inode->in.i_blocks = cpu2le32 (le2cpu32 (inode->in.i_blocks) - (EXT2_BLOCK_SIZE (inode->s) / 512));
	mark_inode_dirty (inode);
	
	ext2_free_blocks (inode, block, 1);
	
	DEBUG (("check_block_empty: #%li: cleared %lu", inode->inode, block));
	return 0;
}


static void
trunc_direct (COOKIE *inode)
{
	ulong block_to_free = 0;
	ulong free_count = 0;
	
	long blocks = EXT2_BLOCK_SIZE (inode->s) / 512;
	long direct_block;
	
	long i;
	
	direct_block   = le2cpu32 (inode->in.i_size);
	direct_block  += EXT2_BLOCK_SIZE_MASK (inode->s);
	direct_block >>= EXT2_BLOCK_SIZE_BITS (inode->s);
	
	for (i = direct_block; i < EXT2_NDIR_BLOCKS; i++)
	{
		__u32 tmp = inode->in.i_block [i];
		
		if (!tmp)
			break;
		
		tmp = le2cpu32 (tmp);
		
		inode->in.i_block [i] = 0;
		inode->in.i_blocks = cpu2le32 (le2cpu32 (inode->in.i_blocks) - blocks);
		mark_inode_dirty (inode);
		
		/* accumulate blocks to free if they're contiguous
		 */
		if (free_count == 0)
		{
			goto free_this;
		}
		else if (block_to_free == (tmp - free_count))
		{
			free_count++;
		}
		else
		{
			ext2_free_blocks (inode, block_to_free, free_count);
free_this:
			block_to_free = tmp;
			free_count = 1;
		}
	}
	
	if (free_count > 0)
		ext2_free_blocks (inode, block_to_free, free_count);
}

static long
trunc_indirect (COOKIE *inode, long offset, ulong parent, ulong parent_offset)
{
	ulong block;
	
	ulong block_to_free = 0;
	ulong free_count = 0;
	
	long blocks;
	long addr_per_block;
	long indirect_block;
	
	long i;
	
	
	if (read_entry (inode, parent, parent_offset, &block))
		return 1;
	
	if (!block)
		return 0;
	
	blocks = EXT2_BLOCK_SIZE (inode->s) / 512;
	addr_per_block = EXT2_ADDR_PER_BLOCK (inode->s);
	
	indirect_block   = le2cpu32 (inode->in.i_size);
	indirect_block  += EXT2_BLOCK_SIZE_MASK (inode->s);
	indirect_block >>= EXT2_BLOCK_SIZE_BITS (inode->s);
	indirect_block  -= offset;
	
	if (indirect_block < 0)
		indirect_block = 0;
	
	for (i = indirect_block; i < addr_per_block; i++)
	{
		UNIT *u;
		__u32 *ind;
		ulong tmp;
		
		u = bio.read (inode->s->di, block, EXT2_BLOCK_SIZE (inode->s));
		if (!u)
		{
			ALERT (("Ext2-FS [%c]: trunc_indirect: "
				"Read failure, inode = %ld, block = %ld",
				DriveToLetter(inode->s->dev), inode->inode, block));
			
			clear_entry (inode, parent, parent_offset);
			return 0;
		}
		
		ind = i + (__u32 *) u->data;
		
		if (!*ind)
			break;
		
		tmp = le2cpu32 (*ind);
		
		*ind = 0;
		bio_MARK_MODIFIED (&bio, u);
		
		inode->in.i_blocks = cpu2le32 (le2cpu32 (inode->in.i_blocks) - blocks);
		mark_inode_dirty (inode);
		
		/* accumulate blocks to free if they're contiguous */
		if (free_count == 0)
		{
			goto free_this;
		}
		else if (block_to_free == tmp - free_count)
		{
			free_count++;
		}
		else
		{
			ext2_free_blocks (inode, block_to_free, free_count);
free_this:
			block_to_free = tmp;
			free_count = 1;
		}
	}
	
	if (free_count > 0)
		ext2_free_blocks (inode, block_to_free, free_count);
	
	/* Check the block and free it if neccessary
	 */
	check_block_empty (inode, block, parent, parent_offset);
	
	return 0;
}

static long
trunc_dindirect (COOKIE *inode, long offset, ulong parent, ulong parent_offset)
{
	ulong block;
	
	long addr_per_block;
	long addr_per_block_bits;
	long dindirect_block;
	
	long i;
	
	DEBUG (("trunc_dindirect: #%li: %li, %lu, %lu", inode->inode, offset, parent, parent_offset));
	
	if (read_entry (inode, parent, parent_offset, &block))
		return 1;
	
	if (!block)
		return 0;
	
	addr_per_block = EXT2_ADDR_PER_BLOCK (inode->s);
	addr_per_block_bits = EXT2_ADDR_PER_BLOCK_BITS (inode->s);
	
	dindirect_block   = le2cpu32 (inode->in.i_size);
	dindirect_block  += EXT2_BLOCK_SIZE_MASK (inode->s);
	dindirect_block >>= EXT2_BLOCK_SIZE_BITS (inode->s);
	dindirect_block  -= offset;
	dindirect_block >>= addr_per_block_bits;
	
	if (dindirect_block < 0)
		dindirect_block = 0;
	
	DEBUG (("trunc_dindirect: %lu, %li, %li", block, dindirect_block, addr_per_block));
	
	for (i = dindirect_block; i < addr_per_block; i++)
	{
		if (trunc_indirect (inode, offset + (i << addr_per_block_bits), block, i))
		{
			clear_entry (inode, parent, parent_offset);
			return 1;
		}
	}
	
	/* Check the block and free it if neccessary
	 */
	check_block_empty (inode, block, parent, parent_offset);
	
	return 0;
}

static long
trunc_tindirect (COOKIE *inode)
{
	ulong block;
	
	long tindirect_block;
	
	long addr_per_block;
	long addr_per_block_bits;
	long offset;
	
	long i;
	
	
	block = inode->in.i_block [EXT2_TIND_BLOCK];
	if (!block)
		return 0;
	
	block = le2cpu32 (block);
	
	addr_per_block = EXT2_ADDR_PER_BLOCK (inode->s);
	addr_per_block_bits = EXT2_ADDR_PER_BLOCK_BITS (inode->s);
	
	offset = EXT2_NDIR_BLOCKS;
	offset += addr_per_block;
	offset += addr_per_block << addr_per_block_bits;
	
	tindirect_block   = le2cpu32 (inode->in.i_size);
	tindirect_block  += EXT2_BLOCK_SIZE_MASK (inode->s);
	tindirect_block >>= EXT2_BLOCK_SIZE_BITS (inode->s);
	tindirect_block  -= offset;
	tindirect_block  /= (addr_per_block << addr_per_block_bits);
	
	if (tindirect_block < 0)
		tindirect_block = 0;
	
	for (i = tindirect_block; i < addr_per_block; i++)
	{
		if (trunc_dindirect (inode, offset + ((i << addr_per_block_bits) << addr_per_block_bits), block, i))
		{
			inode->in.i_block [EXT2_TIND_BLOCK] = 0;
			mark_inode_dirty (inode);
			
			return 1;
		}
	}
	
	/* Check the block and free it if neccessary
	 */
	check_block_empty (inode, block, 0, EXT2_TIND_BLOCK);
	
	return 0;
}

void
ext2_truncate (COOKIE *inode, unsigned long newsize)
{
	ushort i_mode = le2cpu16 (inode->in.i_mode);
	long offset;
	long err;
	
	if (!(EXT2_ISREG (i_mode) || EXT2_ISDIR (i_mode) || EXT2_ISLNK (i_mode)))
		return;
	
	inode->in.i_size = cpu2le32 (newsize);
	mark_inode_dirty (inode);
	
	ext2_discard_prealloc (inode);
	
	/* do truncation
	 */
	trunc_direct (inode);
	trunc_indirect (inode, EXT2_IND_BLOCK, 0, EXT2_IND_BLOCK);
	trunc_dindirect (inode, EXT2_IND_BLOCK + EXT2_ADDR_PER_BLOCK (inode->s), 0, EXT2_DIND_BLOCK);
	trunc_tindirect (inode);
	
	/* If the file is not being truncated to a block boundary, the
	 * contents of the partial block following the end of the file
	 * must be zeroed in case it ever becomes accessible again due
	 * to subsequent file growth.
	 */
	offset = le2cpu32 (inode->in.i_size) & EXT2_BLOCK_SIZE_MASK (inode->s);
	if (offset)
	{
		UNIT *u;
		u = ext2_read (inode,
				le2cpu32 (inode->in.i_size) >> EXT2_BLOCK_SIZE_BITS (inode->s),
				&err);
		if (u)
		{
			bzero (u->data + offset, EXT2_BLOCK_SIZE (inode->s) - offset);
			bio_MARK_MODIFIED (&bio, u);
		}
	}
	
	inode->in.i_mtime = inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
	mark_inode_dirty (inode);
}
