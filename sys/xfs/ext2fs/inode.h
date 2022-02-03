/*
 * Filename:     inode.h
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

# ifndef _inode_h
# define _inode_h

# include "global.h"


void	cookie_install		(COOKIE *c);
COOKIE *get_new_cookie		(void);
COOKIE *new_cookie		(const ulong inode);
void	del_cookie		(COOKIE *c);
void	inv_ctable		(ushort drv);

/* these functions modifiy the buffer cache
 */
long	get_cookie		(SI *s, long inode, COOKIE **in);
long	put_cookie		(COOKIE *c);
void	rel_cookie		(COOKIE *c);

INLINE long
SYNC_COOKIE (COOKIE *c)
{
	register long r = E_OK;
	
	if (c->dirty)
	{
		r = put_cookie (c);
		if (!r)
			c->dirty = 0;
	}
	
	return r;
}

void	sync_cookies		(void);

INLINE void
mark_inode_dirty (COOKIE *c)
{
	c->dirty = 1;
}

# ifdef EXT2FS_DEBUG
void	dump_inode_cache	(char *buf, long bufsize);
# endif


/* these functions modifiy the buffer cache
 */

void	ext2_delete_inode	(COOKIE *inode);

long	ext2_bmap		(COOKIE *inode, long block);
UNIT *	ext2_read		(COOKIE *inode, long block, long *err);

void	ext2_discard_prealloc	(COOKIE *inode);
long	ext2_getblk		(COOKIE *inode, long block, long *err, ushort clear_flag);
UNIT *	ext2_bread		(COOKIE *inode, long block, long *err);


# endif /* _inode_h */
