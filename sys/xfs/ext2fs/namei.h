/*
 * Filename:     namei.h
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

# ifndef _namei_h
# define _namei_h

# include "global.h"


typedef struct dir	_DIR;	/* */
struct dir
{
	_DIR	*next;		/* next in hash list */
	char	*name;		/* name */
	ushort	namelen;	/* size of name */
	ushort	hash;		/* hash value of name */
	ulong	parent;		/* parent inode */
	ulong	inode;		/* inode number */
	ushort	dev;		/* device */
	ushort	res;		/* reserved */
};


_DIR *	d_lookup	(COOKIE *dir, const char *name);
_DIR *	d_get_dir	(COOKIE *dir, ulong inode, const char *name, long namelen);
void	d_del_dir	(_DIR *dentry);
void	d_inv_dir	(ushort dev);

# ifdef EXT2FS_DEBUG
void	d_verify_clean	(ushort dev, ulong parent);
_DIR *	d_lookup_cookie	(COOKIE *c, long *i);
void	dump_dir_cache	(char *buf, long bufsize);
# endif

long	ext2_check_dir_entry	(const char *caller, COOKIE *dir, ext2_d2 *de, UNIT *u, ulong offset);
_DIR *	ext2_search_entry	(COOKIE *dir, const char *name, long namelen);
UNIT *	ext2_search_entry_i	(COOKIE *dir, long inode, ext2_d2 **res_dir);
UNIT *	ext2_find_entry		(COOKIE *dir, const char *name, long namelen, ext2_d2 **res_dir);
UNIT *	ext2_add_entry		(COOKIE *dir, const char *name, long namelen, ext2_d2 **res_dir, long *err);
long	ext2_delete_entry	(ext2_d2 *dir, UNIT *u);


# endif /* _namei_h */
