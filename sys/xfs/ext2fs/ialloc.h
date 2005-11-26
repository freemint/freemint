/*
 * Filename:     ialloc.h
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@fishpool.com).
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

# ifndef _ialloc_h
# define _ialloc_h

# include "global.h"


void	ext2_free_inode			(COOKIE *inode);
COOKIE *ext2_new_inode			(COOKIE *dir, long mode, long *err);
void	ext2_check_inodes_bitmap	(SI * s);

/* WARNING: all calls modifiy the buffer cache
 */

# endif /* _ialloc_h */
