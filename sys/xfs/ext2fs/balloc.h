/*
 * Filename:     balloc.h
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

# ifndef _balloc_h
# define _balloc_h

# include "global.h"


void	ext2_free_blocks		(COOKIE *inode, ulong block, ulong count);
long	ext2_new_block			(COOKIE *inode, ulong goal, ulong *prealloc_count, ulong *prealloc_block, long *err);
long	ext2_group_sparse		(long group);
void	ext2_check_blocks_bitmap	(SI * s);

/* WARNING: all calls modifiy the buffer cache
 */

# endif /* _balloc_h */
