/*
 * Filename:     ext2sys.c
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

# include "ext2sys.h"

# include "mint/dcntl.h"
# include "mint/emu_tos.h"
# include "mint/endian.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/stat.h"

# include "ext2dev.h"
# include "inode.h"
# include "ialloc.h"
# include "namei.h"
# include "super.h"
# include "truncate.h"
# include "version.h"


static long	_cdecl e_root		(int dev, fcookie *dir);

static long	_cdecl e_lookup		(fcookie *dir, const char *name, fcookie *fc);
static DEVDRV *	_cdecl e_getdev		(fcookie *fc, long *devsp);
static long	_cdecl e_getxattr	(fcookie *fc, XATTR *xattr);
static long	_cdecl e_stat64		(fcookie *fc, STAT *stat);

static long	_cdecl e_chattr		(fcookie *fc, int attr);
static long	_cdecl e_chown		(fcookie *fc, int uid, int gid);
static long	_cdecl e_chmod		(fcookie *fc, unsigned mode);

static long	_cdecl e_mkdir		(fcookie *dir, const char *name, unsigned mode);
static long	_cdecl e_rmdir		(fcookie *dir, const char *name);
static long	_cdecl e_creat		(fcookie *dir, const char *name, unsigned mode, int attr, fcookie *fc);
static long	_cdecl e_remove		(fcookie *dir, const char *name);
static long	_cdecl e_getname	(fcookie *root, fcookie *dir, char *pathname, int length);
static long	_cdecl e_rename		(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

static long	_cdecl e_opendir	(DIR *dirh, int flag);
static long	_cdecl e_readdir	(DIR *dirh, char *name, int namelen, fcookie *fc);
static long	_cdecl e_rewinddir	(DIR *dirh);
static long	_cdecl e_closedir	(DIR *dirh);

static long	_cdecl e_pathconf	(fcookie *dir, int which);
static long	_cdecl e_dfree		(fcookie *dir, long *buffer);
static long	_cdecl e_wlabel		(fcookie *dir, const char *name);
static long	_cdecl e_rlabel		(fcookie *dir, char *name, int namelen);

static long	_cdecl e_symlink	(fcookie *dir, const char *name, const char *to);
static long	_cdecl e_readlink	(fcookie *fc, char *buf, int len);
static long	_cdecl e_hardlink	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
static long	_cdecl e_fscntl		(fcookie *dir, const char *name, int cmd, long arg);
static long	_cdecl e_dskchng	(int drv, int mode);

static long	_cdecl e_release	(fcookie *fc);
static long	_cdecl e_dupcookie	(fcookie *dst, fcookie *src);
static long	_cdecl e_sync		(void);

static long	_cdecl e_mknod		(fcookie *dir, const char *name, ulong mode);
static long	_cdecl e_unmount	(int drv);


FILESYS ext2_filesys =
{
	next:			NULL,

	fsflags:
	/*
	 * FS_KNOPARSE		kernel shouldn't do parsing
	 * FS_CASESENSITIVE	file names are case sensitive
	 * FS_NOXBIT		if a file can be read, it can be executed
	 * FS_LONGPATH		file system understands "size" argument to "getname"
	 * FS_NO_C_CACHE	don't cache cookies for this filesystem
	 * FS_DO_SYNC		file system has a sync function
	 * FS_OWN_MEDIACHANGE	filesystem control self media change (dskchng)
	 * FS_REENTRANT_L1	fs is level 1 reentrant
	 * FS_REENTRANT_L2	fs is level 2 reentrant
	 * FS_EXT_1		extensions level 1 - mknod & unmount
	 * FS_EXT_2		extensions level 2 - additional place at the end
	 * FS_EXT_3		extensions level 3 - stat & native UTC timestamps
	 */
	FS_CASESENSITIVE	|
	FS_LONGPATH		|
	FS_DO_SYNC		|
	FS_OWN_MEDIACHANGE	|
	FS_EXT_1		|
	FS_EXT_2		|
	FS_EXT_3		,

	root:			e_root,
	lookup:			e_lookup,
	creat:			e_creat,
	getdev:			e_getdev,
	getxattr:		e_getxattr,
	chattr:			e_chattr,
	chown:			e_chown,
	chmode:			e_chmod,
	mkdir:			e_mkdir,
	rmdir:			e_rmdir,
	remove:			e_remove,
	getname:		e_getname,
	rename:			e_rename,
	opendir:		e_opendir,
	readdir:		e_readdir,
	rewinddir:		e_rewinddir,
	closedir:		e_closedir,
	pathconf:		e_pathconf,
	dfree:			e_dfree,
	writelabel:		e_wlabel,
	readlabel:		e_rlabel,
	symlink:		e_symlink,
	readlink:		e_readlink,
	hardlink:		e_hardlink,
	fscntl:			e_fscntl,
	dskchng:		e_dskchng,
	release:		e_release,
	dupcookie:		e_dupcookie,
	sync:			e_sync,

	/* FS_EXT_1 */
	mknod:			e_mknod,
	unmount:		e_unmount,

	/* FS_EXT_2
	 */

	/* FS_EXT_3 */
	stat64:			e_stat64,
	res1:			0,
	res2:			0,
	res3:			0,

	lock: 0, sleepers: 0,
	block: NULL, deblock: NULL
};


static long _cdecl
e_root (int drv, fcookie *fc)
{
	SI *s = super [drv];

	DEBUG (("Ext2-FS [%c]: e_root enter (s = %p, mem = %li)", DriveToLetter(drv), s, memory));

	if (!s)
	{
		long i;

		i = read_ext2_sb_info (drv);
		if (i)
		{
			DEBUG (("Ext2-FS [%c]: e_root leave failure", DriveToLetter(drv)));
			return i;
		}

		s = super [drv];
	}

	fc->fs = &ext2_filesys;
	fc->dev = drv;
	fc->aux = 0;
	fc->index = (long) s->root; s->root->links++;

	DEBUG (("Ext2-FS [%c]: e_root leave ok (mem = %li)", DriveToLetter(drv), memory));
	return E_OK;
}

static long _cdecl
e_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	COOKIE *c = (COOKIE *) dir->index;
	SI *s = super [dir->dev];

	DEBUG (("Ext2-FS [%c]: e_lookup (%s)", DriveToLetter(dir->dev), name));

	*fc = *dir;

	/* 1 - itself */
	if (!*name || (name [0] == '.' && name [1] == '\0'))
	{
		c->links++;

		DEBUG (("Ext2-FS [%c]: e_lookup: leave ok, (name = \".\")", DriveToLetter(dir->dev)));
		return E_OK;
	}

	/* 2 - parent dir */
	if (name [0] == '.' && name [1] == '.' && name [2] == '\0')
	{
		if (c->inode == EXT2_ROOT_INO)
		{
			DEBUG (("Ext2-FS [%c]: e_lookup: leave ok, EMOUNT, (name = \"..\")", DriveToLetter(dir->dev)));
			return EMOUNT;
		}
	}

	/* 3 - normal entry */
	{
		_DIR *dentry;
		long ret;

		dentry = ext2_search_entry (c, name, strlen (name));
		if (!dentry)
		{
			DEBUG (("Ext2-FS [%c]: e_lookup: leave ENOENT", DriveToLetter(dir->dev)));
			return ENOENT;
		}

		ret = get_cookie (s, dentry->inode, &c);
		if (ret)
		{
			DEBUG (("Ext2-FS [%c]: e_lookup: leave ret = %li", DriveToLetter(dir->dev), ret));
			return ret;
		}

		fc->index = (long) c;
	}

	DEBUG (("Ext2-FS [%c]: e_lookup: leave ok", DriveToLetter(dir->dev)));
	return E_OK;
}

static DEVDRV * _cdecl
e_getdev (fcookie *fc, long *devsp)
{
	if (fc->fs == &ext2_filesys)
		return &ext2_dev;

	*devsp = ENOSYS;

	DEBUG (("Ext2-FS: e_getdev: leave failure"));
	return NULL;
}

static long _cdecl
e_getxattr (fcookie *fc, XATTR *ptr)
{
	COOKIE *c = (COOKIE *) fc->index;
	SI *s = super [fc->dev];

	{
		register ushort mode;

		ptr->mode = mode = le2cpu16 (c->in.i_mode);

# if EXT2_IFSOCK != S_IFSOCK
		if (EXT2_ISSOCK (mode))
		{
			ptr->mode &= ~EXT2_IFSOCK;
			ptr->mode |= S_IFSOCK;
		}
# endif
# if EXT2_IFLNK != S_IFLNK
		if (EXT2_ISLNK (mode))
		{
			ptr->mode &= ~EXT2_IFLNK;
			ptr->mode |= S_IFLNK;
		}
# endif
# if EXT2_IFREG != S_IFREG
		if (EXT2_ISREG (mode))
		{
			ptr->mode &= ~EXT2_IFREG;
			ptr->mode |= S_IFREG;
		}
# endif
# if EXT2_IFBLK != S_IFBLK
		if (EXT2_ISBLK (mode))
		{
			ptr->mode &= ~EXT2_IFBLK;
			ptr->mode |= S_IFBLK;
		}
# endif
# if EXT2_IFDIR != S_IFDIR
		if (EXT2_ISDIR (mode))
		{
			ptr->mode &= ~EXT2_IFDIR;
			ptr->mode |= S_IFDIR;
		}
# endif
# if EXT2_IFCHR != S_IFCHR
		if (EXT2_ISCHR (mode))
		{
			ptr->mode &= ~EXT2_IFCHR;
			ptr->mode |= S_IFCHR;
		}
# endif
# if EXT2_IFIFO != S_IFIFO
		if (EXT2_ISFIFO (mode))
		{
			ptr->mode &= ~EXT2_IFIFO;
			ptr->mode |= S_IFIFO;
		}
# endif

		/* fake attr field a little bit */
		if (S_ISDIR (ptr->mode))
		{
			ptr->attr = FA_DIR;
		}
		else
			ptr->attr = (ptr->mode & 0222) ? 0 : FA_RDONLY;
	}

	ptr->index	= c->inode;
	ptr->dev	= c->dev;
	ptr->rdev 	= c->rdev;
	ptr->nlink	= le2cpu16 (c->in.i_links_count);
	ptr->uid	= le2cpu16 (c->in.i_uid);
	ptr->gid	= le2cpu16 (c->in.i_gid);
	ptr->size 	= le2cpu32 (c->in.i_size);
	ptr->blksize	= EXT2_BLOCK_SIZE (s);
	/* nblocks is measured in blksize */
	ptr->nblocks	= le2cpu32 (c->in.i_blocks) / (ptr->blksize >> 9);

	if (native_utc)
	{
		/* kernel recalc to local time & DOS style */
		SET_XATTR_TD(ptr,m,le2cpu32(c->in.i_mtime));
		SET_XATTR_TD(ptr,a,le2cpu32(c->in.i_atime));
		SET_XATTR_TD(ptr,c,le2cpu32(c->in.i_ctime));
	}
	else
	{
		/* the old way */
		SET_XATTR_TD(ptr,m,dostime(le2cpu32(c->in.i_mtime)));
		SET_XATTR_TD(ptr,a,dostime(le2cpu32(c->in.i_atime)));
		SET_XATTR_TD(ptr,c,dostime(le2cpu32(c->in.i_ctime)));
	}

	DEBUG (("Ext2-FS [%c]: e_getxattr: #%li -> ok", DriveToLetter(fc->dev), c->inode));
	return E_OK;
}

static long _cdecl
e_stat64 (fcookie *fc, STAT *ptr)
{
	COOKIE *c = (COOKIE *) fc->index;
	SI *s = super [fc->dev];

	ptr->dev = c->dev;
	ptr->ino = c->inode;
	{
		register ushort mode;

		ptr->mode = mode = le2cpu16 (c->in.i_mode);

# if EXT2_IFSOCK != S_IFSOCK
		if (EXT2_ISSOCK (mode))
		{
			ptr->mode &= ~EXT2_IFSOCK;
			ptr->mode |= S_IFSOCK;
		}
# endif
# if EXT2_IFLNK != S_IFLNK
		if (EXT2_ISLNK (mode))
		{
			ptr->mode &= ~EXT2_IFLNK;
			ptr->mode |= S_IFLNK;
		}
# endif
# if EXT2_IFREG != S_IFREG
		if (EXT2_ISREG (mode))
		{
			ptr->mode &= ~EXT2_IFREG;
			ptr->mode |= S_IFREG;
		}
# endif
# if EXT2_IFBLK != S_IFBLK
		if (EXT2_ISBLK (mode))
		{
			ptr->mode &= ~EXT2_IFBLK;
			ptr->mode |= S_IFBLK;
		}
# endif
# if EXT2_IFDIR != S_IFDIR
		if (EXT2_ISDIR (mode))
		{
			ptr->mode &= ~EXT2_IFDIR;
			ptr->mode |= S_IFDIR;
		}
# endif
# if EXT2_IFCHR != S_IFCHR
		if (EXT2_ISCHR (mode))
		{
			ptr->mode &= ~EXT2_IFCHR;
			ptr->mode |= S_IFCHR;
		}
# endif
# if EXT2_IFIFO != S_IFIFO
		if (EXT2_ISFIFO (mode))
		{
			ptr->mode &= ~EXT2_IFIFO;
			ptr->mode |= S_IFIFO;
		}
# endif
	}
	ptr->nlink		= le2cpu16 (c->in.i_links_count);
	ptr->uid		= le2cpu16 (c->in.i_uid);
	ptr->gid		= le2cpu16 (c->in.i_gid);
	ptr->rdev 		= c->rdev;

	ptr->atime.high_time	= 0;
	ptr->atime.time		= le2cpu32 (c->in.i_atime);
	ptr->atime.nanoseconds	= 0;

	ptr->mtime.high_time	= 0;
	ptr->mtime.time		= le2cpu32 (c->in.i_mtime);
	ptr->mtime.nanoseconds	= 0;

	ptr->ctime.high_time	= 0;
	ptr->ctime.time		= le2cpu32 (c->in.i_ctime);
	ptr->ctime.nanoseconds	= 0;

	ptr->size 		= le2cpu32 (c->in.i_size);
	ptr->blocks		= le2cpu32 (c->in.i_blocks);
	ptr->blksize		= EXT2_BLOCK_SIZE (s);
	ptr->flags		= 0;
	ptr->gen		= 0;

	bzero (ptr->res, sizeof (ptr->res));

	DEBUG (("Ext2-FS [%c]: e_stat: #%li -> ok", DriveToLetter(fc->dev), c->inode));
	return E_OK;
}

/* the only settable attribute is FA_RDONLY; if the bit is set,
 * the mode is changed so that no write permission exists
 */
static long _cdecl
e_chattr (fcookie *fc, int attr)
{
	COOKIE *c = (COOKIE *) fc->index;
	ushort mode;

	DEBUG (("Ext2-FS [%c]: e_chattr: #%li, %x", DriveToLetter(fc->dev), c->inode, attr));

	if (c->s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	if ((attr & FA_RDONLY) || (attr == 0))
	{
		mode = le2cpu16 (c->in.i_mode);

		if (attr)
		{
			/* turn off write permission */
			mode &= ~S_IWUGO;

			DEBUG (("Ext2-FS [%c]: e_chattr: turn off", DriveToLetter(fc->dev)));
			goto write;
		}
		else
		{
			if ((mode & S_IWUGO) == 0)
			{
				/* turn write permission back on */
				mode |= (mode & 0444) >> 1;

				DEBUG (("Ext2-FS [%c]: e_chattr: turn on", DriveToLetter(fc->dev)));
				goto write;
			}
		}
	}

	DEBUG (("Ext2-FS [%c]: e_chattr: return E_OK, nothing done", DriveToLetter(fc->dev)));
	return E_OK;

write:
	c->in.i_mode = cpu2le16 (mode);
	c->in.i_ctime = cpu2le32 (CURRENT_TIME);
	mark_inode_dirty (c);

	bio_SYNC_DRV (&bio, c->s->di);

	DEBUG (("Ext2-FS [%c]: e_chattr: done (%x), return E_OK", DriveToLetter(fc->dev), mode));
	return E_OK;
}

static long _cdecl
e_chown (fcookie *fc, int uid, int gid)
{
	COOKIE *c = (COOKIE *) fc->index;

	DEBUG (("Ext2-FS [%c]: e_chown", DriveToLetter(fc->dev)));

	if (c->s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	if (uid != -1) c->in.i_uid = cpu2le16 (uid);
	if (gid != -1) c->in.i_gid = cpu2le16 (gid);

	c->in.i_ctime = cpu2le32 (CURRENT_TIME);

	mark_inode_dirty (c);

	bio_SYNC_DRV (&bio, c->s->di);
	return E_OK;
}

static long _cdecl
e_chmod (fcookie *fc, unsigned mode)
{
	COOKIE *c = (COOKIE *) fc->index;

	DEBUG (("Ext2-FS [%c]: e_chmod", DriveToLetter(fc->dev)));

	if (c->s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	c->in.i_mode = cpu2le16 ((le2cpu16 (c->in.i_mode) & S_IFMT) | (mode & S_IALLUGO));
	c->in.i_ctime = cpu2le32 (CURRENT_TIME);

	mark_inode_dirty (c);

	bio_SYNC_DRV (&bio, c->s->di);
	return E_OK;
}

static long _cdecl
e_mkdir (fcookie *dir, const char *name, unsigned mode)
{
	SI *s = super [dir->dev];
	COOKIE *dirc = (COOKIE *) dir->index;
	COOKIE *inode;

	long namelen = strlen (name);
	long err = E_OK;


	DEBUG (("Ext2-FS [%c]: e_mkdir", DriveToLetter(dir->dev)));

	if (s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (dirc))
		return EACCES;

	if (le2cpu16 (dirc->in.i_links_count) >= EXT2_LINK_MAX)
		return EMLINK;

	if (ext2_search_entry (dirc, name, namelen))
		return EEXIST;

	inode = ext2_new_inode (dirc, EXT2_IFDIR, &err);
	if (!inode)
		return EIO;

	inode->in.i_size = cpu2le32 (EXT2_BLOCK_SIZE (s));
	inode->in.i_blocks = 0;

	/* setup directory block */
	{
		ext2_d2 *de1;
		ext2_d2 *de2;
		UNIT *u;

		u = ext2_bread (inode, 0, &err);
		if (!u)
		{
			err = EIO;
			goto out_no_entry;
		}

		de1 = (ext2_d2 *) u->data;
		de1->inode = cpu2le32 (inode->inode);
		de1->name_len = 1;
		de1->rec_len = cpu2le16 (EXT2_DIR_REC_LEN (de1->name_len));
		de1->name [0] = '.';
		de1->name [1] = '\0';

		de2 = (ext2_d2 *) ((char *) de1 + le2cpu16 (de1->rec_len));
		de2->inode = cpu2le32 (dirc->inode);
		de2->name_len = 2;
		de2->rec_len = cpu2le16 (EXT2_BLOCK_SIZE (s) - EXT2_DIR_REC_LEN (1));
		de2->name [0] = '.';
		de2->name [1] = '.';
		de2->name [2] = '\0';

		if (EXT2_HAS_INCOMPAT_FEATURE (s, EXT2_FEATURE_INCOMPAT_FILETYPE))
		{
			de1->file_type = EXT2_FT_DIR;
			de2->file_type = EXT2_FT_DIR;
		}

		bio_MARK_MODIFIED (&bio, u);
	}

	inode->in.i_links_count = cpu2le16 (2);
	inode->in.i_mode = S_IFDIR | (mode & (S_IRWXUGO | S_ISVTX) /*& ~current->fs->umask*/);
	if (le2cpu16 (dirc->in.i_mode) & S_ISGID)
		inode->in.i_mode |= S_ISGID;
	inode->in.i_mode = cpu2le16 (inode->in.i_mode);
	mark_inode_dirty (inode);

	/* add name to directory */
	{
		ext2_d2 *de;
		UNIT *u;

		u = ext2_add_entry (dirc, name, namelen, &de, &err);
		if (!u)
			goto out_no_entry;

		de->inode = cpu2le32 (inode->inode);
		if (EXT2_HAS_INCOMPAT_FEATURE (s, EXT2_FEATURE_INCOMPAT_FILETYPE))
			de->file_type = EXT2_FT_DIR;

		bio_MARK_MODIFIED (&bio, u);
	}

	dirc->in.i_version = cpu2le32 (++event);
	dirc->in.i_links_count = cpu2le16 (le2cpu16 (dirc->in.i_links_count) + 1);
	dirc->in.i_flags = cpu2le32 (le2cpu32 (dirc->in.i_flags) & ~EXT2_BTREE_FL);
	mark_inode_dirty (dirc);

	/* update directory cache */
	(void) d_get_dir (dirc, inode->inode, name, namelen);

	err = E_OK;
	goto out;

out_no_entry:

	inode->in.i_links_count = 0;
	mark_inode_dirty (inode);

out:
	/* release the cookie */
	rel_cookie (inode);

	bio_SYNC_DRV (&bio, s->di);

	DEBUG (("Ext2-FS [%c]: e_mkdir: leave (%li)", DriveToLetter(dir->dev), err));
	return err;
}

/* routine to check that the specified directory is empty (for rmdir)
 */
static long
empty_dir (COOKIE *inode)
{
	SI *s = inode->s;
	UNIT *u;
	ext2_d2 *de;
	ext2_d2 *de1;

	long i_size = le2cpu32 (inode->in.i_size);
	long offset;
	long err;


	if (i_size < EXT2_DIR_REC_LEN (1) + EXT2_DIR_REC_LEN (2)
		|| !(u = ext2_read (inode, 0, &err)))
	{
	    	ALERT (("Ext2-FS [%c]: empty_dir: bad directory (dir #%li) - no data block", DriveToLetter(inode->dev), inode->inode));
		return 1;
	}

	de = (ext2_d2 *) u->data;
	de1 = (ext2_d2 *) ((char *) de + le2cpu16 (de->rec_len));

	if (le2cpu32 (de->inode) != inode->inode
		|| !de1->inode
		|| (de->name [0] != '.' || de->name [1] != '\0')
		|| (de1->name [0] != '.' || de1->name [1] != '.' || de1->name [2] != '\0'))
	{
	    	ALERT (("Ext2-FS [%c]: empty_dir: bad directory (dir #%li) - no `.' or `..'", DriveToLetter(inode->dev), inode->inode));
		return 1;
	}

	offset = le2cpu16 (de->rec_len) + le2cpu16 (de1->rec_len);
	de = (ext2_d2 *) ((char *) de1 + le2cpu16 (de1->rec_len));
	while (offset < i_size)
	{
		if (!u || (void *) de >= (void *) (u->data + EXT2_BLOCK_SIZE (s)))
		{
			u = ext2_read (inode, offset >> EXT2_BLOCK_SIZE_BITS (s), &err);
			if (!u)
			{
				ALERT (("Ext2-FS [%c]: empty_dir: directory #%lu contains a hole at offset %lu",
					DriveToLetter(s->dev), inode->inode, offset));

				offset += EXT2_BLOCK_SIZE (s);
				continue;
			}

			de = (ext2_d2 *) u->data;
		}

		if (!ext2_check_dir_entry ("empty_dir", inode, de, u, offset))
			return 1;

		if (de->inode)
			return 0;

		offset += le2cpu16 (de->rec_len);
		de = (ext2_d2 *) ((char *) de + le2cpu16 (de->rec_len));
	}

	return 1;
}

static long _cdecl
e_rmdir (fcookie *dir, const char *name)
{
	SI *s = super [dir->dev];
	COOKIE *dirc = (COOKIE *) dir->index;
	COOKIE *inode;
	_DIR *dentry;

	long namelen = strlen (name);
	long retval = E_OK;


	DEBUG (("Ext2-FS [%c]: e_rmdir (%s)", DriveToLetter(dir->dev), name));

	if (namelen > EXT2_NAME_LEN)
		return ENAMETOOLONG;

	if (s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (dirc))
		return EACCES;

	dentry = ext2_search_entry (dirc, name, namelen);
	if (!dentry)
		return ENOENT;

	retval = get_cookie (s, dentry->inode, &inode);
	if (retval)
		return retval;

	if (IS_IMMUTABLE (inode))
	{
		retval = EACCES;
		goto out;
	}

	if (!EXT2_ISDIR (le2cpu16 (inode->in.i_mode)))
	{
		retval = ENOTDIR;
		goto out;
	}

	if (inode->inode != dentry->inode)
	{
		retval = EIO;
		goto out;
	}

	if (!empty_dir (inode))
	{
		retval = ENOTEMPTY;
		goto out;
	}

	/* remove name from directory */
	{
		ext2_d2 *de;
		UNIT *u;

		u = ext2_find_entry (dirc, name, namelen, &de);
		if (!u)
		{
			retval = ENOENT;
			goto out;
		}

		if (le2cpu32 (de->inode) != inode->inode)
		{
			retval = ENOENT;
			goto out;
		}

		retval = ext2_delete_entry (de, u);
		dirc->in.i_version = cpu2le32 (++event);

		if (retval)
			goto out;

		/* update directory cache
		 */

		/* remove entry itself */
		d_del_dir (dentry);

		/* and possible cached '.' and '..' entries */
		dentry = d_lookup (inode, ".");
		if (dentry)
			d_del_dir (dentry);

		dentry = d_lookup (inode, "..");
		if (dentry)
			d_del_dir (dentry);

# ifdef EXT2FS_DEBUG
		d_verify_clean (s->dev, inode->inode);
# endif
	}

	if (le2cpu16 (inode->in.i_links_count) != 2)
		ALERT (("Ext2-FS [%c]: e_rmdir: empty directory has nlink != 2 (%u)", DriveToLetter(dir->dev), le2cpu16 (inode->in.i_links_count)));

	inode->in.i_version = cpu2le32 (++event);
	inode->in.i_links_count = 0;
	inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
	mark_inode_dirty (inode);

	dirc->in.i_links_count = cpu2le16 (le2cpu16 (dirc->in.i_links_count) - 1);
	dirc->in.i_ctime = dirc->in.i_mtime = inode->in.i_ctime;
	dirc->in.i_flags = cpu2le32 (le2cpu32 (dirc->in.i_flags) & ~EXT2_BTREE_FL);
	mark_inode_dirty (dirc);

out:
	/* release the cookie */
	rel_cookie (inode);

	bio_SYNC_DRV (&bio, s->di);

	DEBUG (("Ext2-FS [%c]: e_rmdir: leave (%li)", DriveToLetter(dir->dev), retval));
	return retval;
}

static long _cdecl
e_creat (fcookie *dir, const char *name, unsigned mode, int attr, fcookie *fc)
{
	COOKIE *dirc = (COOKIE *) dir->index;
	COOKIE *inode;

	long namelen = strlen (name);
	long err = EIO;


	DEBUG (("Ext2-FS [%c]: e_creat enter (%s)", DriveToLetter(dir->dev), name));

	if (dirc->s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (dirc))
		return EACCES;

	if (ext2_search_entry (dirc, name, namelen))
		return EEXIST;

	inode = ext2_new_inode (dirc, mode, &err);
	if (!inode)
		return err;

	/* add name to directory */
	{
		ext2_d2 *de;
		UNIT *u;

		u = ext2_add_entry (dirc, name, namelen, &de, &err);
		if (!u)
		{
			inode->in.i_links_count = cpu2le16 (le2cpu16 (inode->in.i_links_count) - 1);
			mark_inode_dirty (inode);

			/* release cookie (also delete the inode) */
			rel_cookie (inode);

			return err;
		}
		de->inode = cpu2le32 (inode->inode);

		if (EXT2_HAS_INCOMPAT_FEATURE (dirc->s, EXT2_FEATURE_INCOMPAT_FILETYPE))
			de->file_type = EXT2_FT_REG_FILE;

		bio_MARK_MODIFIED (&bio, u);
	}

	dirc->in.i_version = cpu2le32 (++event);
	mark_inode_dirty (dirc);

	/* update directory cache */
	(void) d_get_dir (dirc, inode->inode, name, strlen (name));

	*fc = *dir;
	fc->index = (long) inode;

	bio_SYNC_DRV (&bio, inode->s->di);

	DEBUG (("Ext2-FS [%c]: e_creat leave OK (#%li, uid = %i, gid = %i)", DriveToLetter(dir->dev), inode->inode, le2cpu16 (inode->in.i_uid), le2cpu16 (inode->in.i_gid)));
	return E_OK;
}

static long _cdecl
e_remove (fcookie *dir, const char *name)
{
	COOKIE *dirc = (COOKIE *) dir->index;
	COOKIE *inode;

	_DIR *dentry;

	long namelen = strlen (name);
	long retval;


	DEBUG (("Ext2-FS [%c]: e_remove: enter (%s)", DriveToLetter(dir->dev), name));

	if (namelen > EXT2_NAME_LEN)
		return ENAMETOOLONG;

	if (dirc->s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (dirc))
		return EACCES;

	dentry = ext2_search_entry (dirc, name, namelen);
	if (!dentry)
		return ENOENT;

	retval = get_cookie (dirc->s, dentry->inode, &inode);
	if (retval)
		return retval;

	if (EXT2_ISDIR (le2cpu16 (inode->in.i_mode))
		|| IS_APPEND (inode)
		|| IS_IMMUTABLE (inode))
	{
		retval = EACCES;
		goto out;
	}

	if (dentry->inode != inode->inode)
	{
		retval = EIO;
		goto out;
	}

	if (!inode->in.i_links_count)
	{
		ALERT (("Ext2-FS [%c]: ext2_unlink: Deleting nonexistent file (%lu), %d", DriveToLetter(inode->dev), inode->inode, le2cpu16 (inode->in.i_links_count)));
		inode->in.i_links_count = cpu2le16 (1);
	}

	/* remove name from directory */
	{
		ext2_d2 *de;
		UNIT *u;

		u = ext2_find_entry (dirc, name, namelen, &de);
		if (!u)
		{
			retval = ENOENT;
			goto out;
		}

		retval = ext2_delete_entry (de, u);
		if (retval)
		{
			retval = EACCES;
			goto out;
		}

		/* update directory cache */
		d_del_dir (dentry);
	}

	dirc->in.i_version = cpu2le32 (++event);
	dirc->in.i_ctime = dirc->in.i_mtime = cpu2le32 (CURRENT_TIME);
	dirc->in.i_flags = cpu2le32 (le2cpu32 (dirc->in.i_flags) & ~EXT2_BTREE_FL);
	mark_inode_dirty (dirc);

	inode->in.i_links_count = cpu2le16 (le2cpu16 (inode->in.i_links_count) - 1);
	inode->in.i_ctime = dirc->in.i_ctime;
	mark_inode_dirty (inode);

	retval = E_OK;

out:
	/* release cookie (also delete the inode if neccessary) */
	rel_cookie (inode);

	bio_SYNC_DRV (&bio, dirc->s->di);

	DEBUG (("Ext2-FS [%c]: e_remove: leave (%li)", DriveToLetter(dir->dev), retval));
	return retval;
}

static long _cdecl
e_getname (fcookie *root, fcookie *dir, char *pathname, int length)
{
	SI *s = super [dir->dev];
	ulong inum = ((COOKIE *) dir->index)->inode;

	char *dst = pathname;
	long len = 0;

	DEBUG (("Ext2-FS [%c]: e_getname: #%li -> #%li", DriveToLetter(root->dev), ((COOKIE *) root->index)->inode, ((COOKIE *) dir->index)->inode));
	ASSERT ((((COOKIE *) root->index)->inode == EXT2_ROOT_INO));

	*pathname = '\0';
	length--;

	while (inum != EXT2_ROOT_INO)
	{
		COOKIE *c;
		UNIT *u;
		ext2_d2 *de;
		_DIR *dentry;
		ulong pinum;
		long r;

		r = get_cookie (s, inum, &c);
		if (r)
		{
			DEBUG (("Ext2-FS: e_getname: get_cookie (#%li) fail: r = %li", inum, r));
			return r;
		}

		dentry = ext2_search_entry (c, "..", 2); rel_cookie (c);
		if (!dentry)
		{
			/* If this happens we're in trouble */

			ALERT (("Ext2-FS [%c]: e_getname: no '..' in inode #%li", DriveToLetter(c->dev), c->inode));
			return inum;
		}

		pinum = dentry->inode;

		r = get_cookie (s, pinum, &c);
		if (r)
		{
			DEBUG (("Ext2-FS: e_getname: get_cookie (#%li) fail: r = %li", inum, r));
			return r;
		}

		r = ENOENT;
		u = ext2_search_entry_i (c, inum, &de);
		if (u)
		{
			register long i = de->name_len;
			register char *src;

			len += i + 1;
			if (len < length)
			{
				src = de->name + i - 1;
				while (i--)
				{
					*dst++ = *src--;
				}

				*dst++ = '\\';
				*dst = '\0';

				inum = pinum;

				r = 0;
			}
			else
			{
				DEBUG (("Ext2-FS: e_getname: EBADARG"));
				r = EBADARG;
			}
		}

		rel_cookie (c);

		if (r)
		{
			DEBUG (("Ext2-FS: e_getname: leave failure (r = %li)", r));
			return r;
		}
	}

	strrev (pathname);

	DEBUG (("Ext2-FS: e_getname: leave ok (%s)", pathname));
	return E_OK;
}

static long _cdecl
e_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
# define PARENT_INO(buffer) \
	(((ext2_d2 *) ((char *) (buffer) + le2cpu16 (((ext2_d2 *) (buffer))->rec_len)))->inode)

	SI *s = super [olddir->dev];

	COOKIE *olddirc = (COOKIE *) olddir->index;
	COOKIE *newdirc = (COOKIE *) newdir->index;
	COOKIE *inode = NULL;

	_DIR *olddentry;
	_DIR *tmpdentry;

	UNIT *old_u = NULL;
	UNIT *new_u = NULL;
	UNIT *dir_u = NULL;

	ext2_d2 *old_de;
	ext2_d2 *new_de;

	long retval = ENOENT;


	DEBUG (("Ext2-FS [%c]: e_rename: #%li: %s -> #%li: %s", DriveToLetter(olddir->dev), olddirc->inode, oldname, newdirc->inode, newname));

	/* check cross drives */
	if (olddir->dev != newdir->dev)
	{
		DEBUG (("Ext2-FS [%c]: e_rename: cross device [%c] -> EXDEV!", DriveToLetter(olddir->dev), DriveToLetter(newdir->dev)));
		return EXDEV;
	}
# if 0	/* paranoia check :-) */
	/* check cross drives - redundant here */
	if (olddirc->dev != newdirc->dev)
	{
		ALERT (("Ext2-FS [%c]: e_rename: !!! cross device [%c] -> EXDEV!", DriveToLetter(olddirc->dev), DriveToLetter(newdirc->dev)));
		return EXDEV;
	}
# endif

	if (s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (olddirc) || !EXT2_ISDIR (le2cpu16 (olddirc->in.i_mode))
		|| IS_IMMUTABLE (newdirc) || !EXT2_ISDIR (le2cpu16 (newdirc->in.i_mode)))
	{
		return EACCES;
	}

	olddentry = ext2_search_entry (olddirc, oldname, strlen (oldname));
	if (!olddentry)
		goto end_rename;

	old_u = ext2_find_entry (olddirc, oldname, strlen (oldname), &old_de);
	if (!old_u)
		goto end_rename;

	bio.lock (old_u);

	DEBUG (("olddentry->inode = %li <-> old_de->inode = %li", olddentry->inode, le2cpu32 (old_de->inode)));

	retval = get_cookie (s, olddentry->inode, &inode);
	if (retval)
	{
		inode = NULL;
		goto end_rename;
	}

	retval = EACCES;

# if 0
	if ((old_dir->i_mode & S_ISVTX)
		&& current->fsuid != old_inode->i_uid
		&& current->fsuid != old_dir->i_uid && !capable (CAP_FOWNER))
	{
		goto end_rename;
	}
# endif

	if (IS_APPEND (inode) || IS_IMMUTABLE (inode))
		goto end_rename;

	if (EXT2_ISDIR (le2cpu16 (inode->in.i_mode)))
	{
		COOKIE *check;

		retval = get_cookie (s, newdirc->inode, &check);
		if (retval)
			goto end_rename;

		ASSERT ((check == newdirc));

		for(;;)
		{
			_DIR *tmp;

			if (check->inode == inode->inode)
			{
				DEBUG (("Ext2-FS [%c]: invalid directory move", DriveToLetter(check->dev)));

				rel_cookie (check);

				retval = EINVAL;
				goto end_rename;
			}

			if (check->inode == EXT2_ROOT_INO)
			{
				rel_cookie (check);
				break;
			}

			tmp = ext2_search_entry (check, "..", 2);
			if (!tmp)
			{
				DEBUG (("Ext2-FS [%c]: ext2_search_entry fail in e_rename", DriveToLetter(check->dev)));

				rel_cookie (check);

				retval = EACCES;
				goto end_rename;
			}

			rel_cookie (check);

			retval = get_cookie (s, tmp->inode, &check);
			if (retval)
			{
				DEBUG (("Ext2-FS [%c]: get_cookie fail in e_rename", DriveToLetter(check->dev)));

				goto end_rename;
			}
		}

		if (le2cpu16 (newdirc->in.i_links_count) >= EXT2_LINK_MAX)
		{
			retval = EMLINK;
			goto end_rename;
		}

		dir_u = ext2_read (inode, 0, &retval);
		if (!dir_u)
			goto end_rename;

		bio.lock (dir_u);

		if (le2cpu32 (PARENT_INO (dir_u->data)) != olddirc->inode)
			goto end_rename;
	}

	tmpdentry = ext2_search_entry (newdirc, newname, strlen (newname));
	if (tmpdentry)
		goto end_rename;

	new_u = ext2_add_entry (newdirc, newname, strlen (newname), &new_de, &retval);
	if (!new_u)
		goto end_rename;

	bio.lock (new_u);

	new_de->inode = cpu2le32 (inode->inode);
	if (EXT2_HAS_INCOMPAT_FEATURE (s, EXT2_FEATURE_INCOMPAT_FILETYPE))
		new_de->file_type = old_de->file_type;

	bio_MARK_MODIFIED (&bio, new_u);
	bio.unlock (new_u); new_u = NULL;

	ext2_delete_entry (old_de, old_u);
	bio.unlock (old_u); old_u = NULL;

	if (newdirc != olddirc)
	{
		newdirc->in.i_version = cpu2le32 (++event);
		newdirc->in.i_ctime = newdirc->in.i_mtime = cpu2le32 (CURRENT_TIME);
		newdirc->in.i_flags = cpu2le32 (le2cpu32 (newdirc->in.i_flags) & ~EXT2_BTREE_FL);

		if (EXT2_ISDIR (le2cpu16 (inode->in.i_mode)))
		{
			olddirc->in.i_links_count = cpu2le16 (le2cpu16 (olddirc->in.i_links_count) - 1);
			newdirc->in.i_links_count = cpu2le16 (le2cpu16 (newdirc->in.i_links_count) + 1);

			PARENT_INO (dir_u->data) = cpu2le32 (newdirc->inode);

			bio_MARK_MODIFIED (&bio, dir_u);
			bio.unlock (dir_u); dir_u = NULL;
		}

		mark_inode_dirty (newdirc);
	}

	olddirc->in.i_version = cpu2le32 (++event);
	olddirc->in.i_ctime = olddirc->in.i_mtime = cpu2le32 (CURRENT_TIME);
	olddirc->in.i_flags = cpu2le32 (le2cpu32 (olddirc->in.i_flags) & ~EXT2_BTREE_FL);
	mark_inode_dirty (olddirc);

	/* update directory cache */
	d_del_dir (olddentry);
	(void) d_get_dir (newdirc, inode->inode, newname, strlen (newname));

	retval = E_OK;

end_rename:

	if (inode) rel_cookie (inode);

	if (old_u) bio.unlock (old_u);
	if (new_u) bio.unlock (new_u);
	if (dir_u) bio.unlock (dir_u);

	DEBUG (("Ext2-FS [%c]: e_rename: leave r = %li", DriveToLetter(olddir->dev), retval));
	return retval;
}

struct dirinfo
{
	long pos;
	long size;
	long version;
};
# define DIR_pos(d)	(*(long *) &(d)->fsstuff [0])
# define DIR_size(d)	(*(long *) &(d)->fsstuff [4])
# define DIR_version(d) (*(long *) &(d)->fsstuff [8])

static long _cdecl
e_opendir (DIR *dirh, int flag)
{
	COOKIE *c = (COOKIE *) dirh->fc.index;
	union { char *c; struct dirinfo *dirinfo; } dirptr; dirptr.c = dirh->fsstuff;

	DEBUG (("Ext2-FS [%c]: e_opendir: #%li", DriveToLetter(dirh->fc.dev), c->inode));

	c->links++;

	dirptr.dirinfo->pos = 0;
	dirptr.dirinfo->size = le2cpu32 (c->in.i_size);
	dirptr.dirinfo->version = c->in.i_version;

	DEBUG (("Ext2-FS [%c]: e_opendir: leave ok", DriveToLetter(dirh->fc.dev)));
	return E_OK;
}

static long _cdecl
e_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	COOKIE *c = (COOKIE *) dirh->fc.index;
	SI *s = super [dirh->fc.dev];
	ext2_d2 *de;
	union { char *c; struct dirinfo *dirinfo; } dirptr = {dirh->fsstuff};
	ulong offset = dirptr.dirinfo->pos & EXT2_BLOCK_SIZE_MASK (s);


	DEBUG (("Ext2-FS [%c]: e_readdir: #%li", DriveToLetter(dirh->fc.dev), c->inode));

	while (dirptr.dirinfo->pos < dirptr.dirinfo->size)
	{
		UNIT *u;

		u = ext2_read (c, dirptr.dirinfo->pos >> EXT2_BLOCK_SIZE_BITS (s), NULL);
		if (!u)
		{
			ALERT (("Ext2-FS: ext2_readdir: directory #%li contains a hole at offset %li", c->inode, dirptr.dirinfo->pos));

			dirptr.dirinfo->pos += EXT2_BLOCK_SIZE (s) - offset;
			continue;
		}

		/* If the dir block has changed since the last call to
		 * readdir, then we might be pointing to an invalid
		 * dirent right now. Scan from the start of the block
		 * to make sure.
		 */
		if (dirptr.dirinfo->version != c->in.i_version)
		{
			long i;

			for (i = 0; i < EXT2_BLOCK_SIZE (s) && i < offset; )
			{
				de = (ext2_d2 *) (u->data + i);

				/* It's too expensive to do a full
				 * dirent test each time round this
				 * loop, but we do have to test at
				 * least that it is non-zero. A
				 * failure will be detected in the
				 * dirent test below.
				 */
				if (le2cpu16 (de->rec_len) < EXT2_DIR_REC_LEN (1))
					break;

				i += le2cpu16 (de->rec_len);
			}

			offset = i;
			dirptr.dirinfo->pos = (dirptr.dirinfo->pos & ~(EXT2_BLOCK_SIZE_MASK (s))) | offset;
			dirptr.dirinfo->version = c->in.i_version;
		}

		while (dirptr.dirinfo->pos < dirptr.dirinfo->size && offset < EXT2_BLOCK_SIZE (s))
		{
			de = (ext2_d2 *) (u->data + offset);

			if (!ext2_check_dir_entry ("e_readdir", c, de, u, offset))
			{
				/* On error, skip the DIR_pos to the
                                 * next block.
                                 */
				dirptr.dirinfo->pos += (EXT2_BLOCK_SIZE (s) - (dirptr.dirinfo->pos & EXT2_BLOCK_SIZE_MASK (s)));

				break;
			}

			/* update DIR_pos and offset */
			{
				register ushort tmp = le2cpu16 (de->rec_len);

				dirptr.dirinfo->pos += tmp;

				/* if we found a entry
				 * we must leave here
				 */
				if (de->inode)
					goto found;

				offset += tmp;
			}
		}

		offset = 0;
	}

	if (!((s->s_flags & MS_NODIRATIME) || (s->s_flags & MS_RDONLY) || IS_IMMUTABLE (c)))
	{
		c->in.i_atime = cpu2le32 (CURRENT_TIME);
		mark_inode_dirty (c);
	}

	DEBUG (("Ext2-FS [%c]: e_readdir leave ENMFILES", DriveToLetter(dirh->fc.dev)));
	return ENMFILES;

found:
	/* entry found, copy it */
	{
		COOKIE *new;
		long inode = le2cpu32 (de->inode);
		long r;

		if ((dirh->flags & TOS_SEARCH) == 0)
		{
			unaligned_putl(name, inode);
			namelen -= 4;
			name += 4;
		}

		namelen--;

		r = MIN (namelen, de->name_len);
		strncpy (name, de->name, r);
		name [r] = '\0';

		if (namelen <= de->name_len)
			return EBADARG;

		r = get_cookie (s, inode, &new);
		if (r)
		{
			DEBUG (("Ext2-FS: e_readdir: get_cookie fail (#%li: %li)", inode, r));
			return r;
		}

		/* setup file cookie */
		fc->fs = &ext2_filesys;
		fc->dev = dirh->fc.dev;
		fc->aux = 0;
		fc->index = (long) new;

		if (!((s->s_flags & MS_NODIRATIME) || (s->s_flags & MS_RDONLY) || IS_IMMUTABLE (c)))
		{
			c->in.i_atime = cpu2le32 (CURRENT_TIME);
			mark_inode_dirty (c);
		}

		DEBUG (("Ext2-FS [%c]: e_readdir ok (#%li: %s)", DriveToLetter(dirh->fc.dev), inode, name));
		return E_OK;
	}
}

static long _cdecl
e_rewinddir (DIR *dirh)
{
	union { char *c; struct dirinfo *dirinfo; } dirptr; dirptr.c = dirh->fsstuff;
	DEBUG (("Ext2-FS [%c]: e_rewinddir: #%li", DriveToLetter(dirh->fc.dev), ((COOKIE *) dirh->fc.index)->inode));

	dirptr.dirinfo->pos = 0;

	return E_OK;
}

static long _cdecl
e_closedir (DIR *dirh)
{
	COOKIE *c = (COOKIE *) dirh->fc.index;

	DEBUG (("Ext2-FS [%c]: e_closedir: #%li", DriveToLetter(dirh->fc.dev), c->inode));

	rel_cookie (c);
	return E_OK;
}

static long _cdecl
e_pathconf (fcookie *dir, int which)
{
	DEBUG (("Ext2-FS [%c]: e_pathconf (%i)", DriveToLetter(dir->dev), which));

	switch (which)
	{
		case DP_INQUIRE:	return DP_VOLNAMEMAX;
		case DP_IOPEN:		return UNLIMITED;
		case DP_MAXLINKS:	return EXT2_LINK_MAX;
		case DP_PATHMAX:	return UNLIMITED;
		case DP_NAMEMAX:	return EXT2_NAME_LEN;
		case DP_ATOMIC:		return EXT2_BLOCK_SIZE (super [dir->dev]);
		case DP_TRUNC:		return DP_NOTRUNC;
		case DP_CASE:		return DP_CASESENS;
		case DP_MODEATTR:	return (DP_ATTRBITS | DP_MODEBITS
						| DP_FT_DIR
						| DP_FT_CHR
						| DP_FT_BLK
						| DP_FT_REG
						| DP_FT_LNK
						| DP_FT_SOCK
						| DP_FT_FIFO
					/*	| DP_FT_MEM	*/
					);
		case DP_XATTRFIELDS:	return (DP_INDEX
						| DP_DEV
					/*	| DP_RDEV	*/
						| DP_NLINK
						| DP_UID
						| DP_GID
						| DP_BLKSIZE
						| DP_SIZE
						| DP_NBLOCKS
						| DP_ATIME
						| DP_CTIME
						| DP_MTIME
					);
		case DP_VOLNAMEMAX:	return 0;
	}

	DEBUG (("Ext2-FS: e_pathconf: leave failure"));
	return ENOSYS;
}

static long _cdecl
e_dfree (fcookie *dir, long *buffer)
{
	SI *s = super [dir->dev];

	DEBUG (("Ext2-FS [%c]: e_dfree", DriveToLetter(dir->dev)));

	buffer[0] = le2cpu32 (s->sbi.s_sb->s_free_blocks_count);
	buffer[1] = s->sbi.s_blocks_count;
	buffer[2] = EXT2_BLOCK_SIZE (s);
	buffer[3] = 1;

	/* correct free blocks count for non-privileged user */
	if (p_geteuid ())
	{
		long s_r_blocks_count = le2cpu32 (s->sbi.s_sb->s_r_blocks_count);

		if (buffer[0] > s_r_blocks_count)
			buffer[0] -= s_r_blocks_count;
	}

	return E_OK;
}

static long _cdecl
e_wlabel (fcookie *dir, const char *name)
{
	long namelen = strlen (name);
	long r = E_OK;

	DEBUG (("Ext2-FS [%c]: e_wlabel enter (%s)", DriveToLetter(dir->dev), name));

	if (namelen > 16)
	{
		r = EBADARG;
	}
	else
	{
		SI *s = super [dir->dev];

		if (s->s_flags & MS_RDONLY)
		{
			r = EROFS;
		}
		else
		{
			strncpy (s->sbi.s_sb->s_volume_name, name, 16);
			bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);

			bio_SYNC_DRV (&bio, s->di);
		}
	}

	DEBUG (("Ext2-FS [%c]: e_wlabel leave (ret = %li)", DriveToLetter(dir->dev), r));
	return r;
}

static long _cdecl
e_rlabel (fcookie *dir, char *name, int namelen)
{
	SI *s = super [dir->dev];
	long ret = E_OK;

	long len;
	char *src;

	DEBUG (("Ext2-FS [%c]: e_rlabel enter", DriveToLetter(dir->dev)));

	len = 0;
	src = s->sbi.s_sb->s_volume_name;
	while (len++ < 16 && *src++)
		;

	if (namelen < len)
	{
		len = namelen;
		ret = EBADARG;
	}

	strncpy (name, s->sbi.s_sb->s_volume_name, len);
	name [len] = '\0';

	DEBUG (("Ext2-FS [%c]: e_rlabel leave (ret = %li)", DriveToLetter(dir->dev), ret));
	return ret;
}

static long _cdecl
e_symlink (fcookie *dir, const char *name, const char *to)
{
	COOKIE *dirc = (COOKIE *) dir->index;
	COOKIE *inode;
	UNIT *u = NULL;
	char *link;

	long namelen = strlen (name);
	long tolen = strlen (to);
	long err;


	DEBUG (("Ext2-FS [%c]: e_symlink: enter (%s, %s)", DriveToLetter(dir->dev), name, to));

	if (dirc->s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (dirc))
		return EACCES;

	if (ext2_search_entry (dirc, name, namelen))
		return EEXIST;

	if (tolen > EXT2_BLOCK_SIZE (dirc->s))
		return EACCES;

	inode = ext2_new_inode (dirc, EXT2_IFLNK, &err);
	if (!inode)
		return err;

	inode->in.i_mode = cpu2le16 (EXT2_IFLNK | S_IRWXUGO);

	if (tolen >= sizeof (inode->in.i_block))
	{
		DEBUG (("tolen = %ld, normal symlink", tolen));

		u = ext2_bread (inode, 0, &err);
		if (!u)
			goto out_no_entry;

		link = (char *)u->data;
	}
	else
	{
		DEBUG (("tolen = %ld, fast symlink", tolen));

		link = (char *) inode->in.i_block;
	}

	/* copy target name */
	{
		long i = 0;
		char c;

		while (i < EXT2_BLOCK_SIZE (inode->s) - 1 && (c = *to++))
			link [i++] = c;

		link [i] = 0;

		inode->in.i_size = cpu2le32 (i);
		mark_inode_dirty (inode);

		if (u) bio_MARK_MODIFIED (&bio, u);
	}

	/* add name to directory */
	{
		ext2_d2 *de;

		u = ext2_add_entry (dirc, name, namelen, &de, &err);
		if (!u)
		{
			err = EACCES;
			goto out_no_entry;
		}

		de->inode = cpu2le32 (inode->inode);

		if (EXT2_HAS_INCOMPAT_FEATURE (dirc->s, EXT2_FEATURE_INCOMPAT_FILETYPE))
			de->file_type = EXT2_FT_SYMLINK;

		dirc->in.i_version = cpu2le32 (++event);
		mark_inode_dirty (dirc);

		bio_MARK_MODIFIED (&bio, u);
	}

	/* update directory cache */
	(void) d_get_dir (dirc, inode->inode, name, strlen (name));

	err = E_OK;
	goto out;

out_no_entry:

	inode->in.i_links_count = cpu2le16 (le2cpu16 (inode->in.i_links_count) - 1);
	mark_inode_dirty (inode);

out:
	/* release the cookie */
	rel_cookie (inode);

	bio_SYNC_DRV (&bio, dirc->s->di);

	DEBUG (("Ext2-FS [%c]: e_symlink: leave (%li)", DriveToLetter(dir->dev), err));
	return err;

}

static long _cdecl
e_readlink (fcookie *fc, char *buf, int len)
{
	COOKIE *inode = (COOKIE *) fc->index;
	char *link;
	long i;

	if (len > EXT2_BLOCK_SIZE (inode->s) - 1)
		len = EXT2_BLOCK_SIZE (inode->s) - 1;

	if (inode->in.i_blocks)
	{
		UNIT *u;
		long err;

		u = ext2_read (inode, 0, &err);
		if (!u)
			return err;

		link = (char *)u->data;
	}
	else
	{
		link = (char *) inode->in.i_block;
	}

# if 1
	i = le2cpu32 (inode->in.i_size) + 1;
# else
	/* paranoia check */
	i = strlen (link);
	if (i != le2cpu32 (inode->in.i_size))
	{
		ALERT (("Ext2-FS [%c]: e_readlink: bad inode (#%li -> %li != %li)", DriveToLetter(inode->dev), inode->inode, i, le2cpu32 (inode->in.i_size)));
	}
	i++;
# endif

	strncpy_f (buf, link, MIN (i, len));

	if (i > len)
		return EBADARG;

	return E_OK;
}

long
e_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	COOKIE *fromdirc = (COOKIE *) fromdir->index;
	COOKIE *todirc = (COOKIE *) todir->index;
	COOKIE *inode;

	_DIR *dentry;

	ushort i_mode;
	long err;


	DEBUG (("Ext2-FS [%c]: e_hardlink enter (%s -> %s)", DriveToLetter(fromdir->dev), fromname, toname));

	if (fromdirc->s->s_flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (todirc))
		return EACCES;

	dentry = ext2_search_entry (fromdirc, fromname,  strlen (fromname));
	if (!dentry)
		return EACCES;

	err = get_cookie (fromdirc->s, dentry->inode, &inode);
	if (err)
		return err;

	i_mode = le2cpu16 (inode->in.i_mode);

	if (EXT2_ISDIR (i_mode))
	{
		err = EACCES;
		goto out;
	}

	if (IS_APPEND (inode) || IS_IMMUTABLE (inode))
	{
		err =  EACCES;
		goto out;
	}

	if (le2cpu16 (inode->in.i_links_count) >= EXT2_LINK_MAX)
	{
		err =  EMLINK;
		goto out;
	}

	/* add name to directory */
	{
		long tonamelen = strlen (toname);
		ext2_d2 *de;
		UNIT *u;

		if (ext2_search_entry (todirc, toname, tonamelen))
		{
			err = EEXIST;
			goto out;
		}

		u = ext2_add_entry (todirc, toname, tonamelen, &de, &err);
		if (!u)
			goto out;

		de->inode = cpu2le32 (inode->inode);
		if (EXT2_HAS_INCOMPAT_FEATURE (inode->s, EXT2_FEATURE_INCOMPAT_FILETYPE))
		{
			if (EXT2_ISREG (i_mode))
				de->file_type = EXT2_FT_REG_FILE;
			else if (EXT2_ISDIR (i_mode))
				de->file_type = EXT2_FT_DIR;
			else if (EXT2_ISLNK (i_mode))
				de->file_type = EXT2_FT_SYMLINK;
			else if (EXT2_ISCHR (i_mode))
				de->file_type = EXT2_FT_CHRDEV;
			else if (EXT2_ISBLK (i_mode))
				de->file_type = EXT2_FT_BLKDEV;
			else if (EXT2_ISFIFO (i_mode))
				de->file_type = EXT2_FT_FIFO;
			else if (EXT2_ISSOCK (i_mode))
				de->file_type = EXT2_FT_SOCK;
			else
				de->file_type = EXT2_FT_UNKNOWN;
		}

		bio_MARK_MODIFIED (&bio, u);
	}

	todirc->in.i_version = cpu2le32 (++event);
	mark_inode_dirty (todirc);

	inode->in.i_links_count = cpu2le16 (le2cpu16 (inode->in.i_links_count) + 1);
	inode->in.i_ctime = cpu2le32 (CURRENT_TIME);
	mark_inode_dirty (inode);

	err = E_OK;

out:
	/* release cookie */
	rel_cookie (inode);

	bio_SYNC_DRV (&bio, fromdirc->s->di);

	DEBUG (("Ext2-FS [%c]: e_hardlink: leave (%li)", DriveToLetter(fromdir->dev), err));
	return err;
}

static long _cdecl
e_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
	SI *s = super [dir->dev];

	DEBUG (("Ext2-FS [%c]: e_fscntl (cmd = %i)", DriveToLetter(dir->dev), cmd));

	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "ext2");
			return E_OK;
		}
		case FS_INFO:
		{
			struct fs_info *info;

			info = (struct fs_info *) arg;
			if (info)
			{
				strcpy (info->name, "ext2-xfs");
				info->version = ((long) VER_MAJOR << 16) | (long) VER_MINOR;
				info->type = FS_EXT2;
				strcpy (info->type_asc, "ext2");
			}

			return E_OK;
		}
		case FS_USAGE:
		{
			struct fs_usage *usage;

			usage = (struct fs_usage *) arg;
			if (usage)
			{
				usage->blocksize   = EXT2_BLOCK_SIZE (s);
				usage->blocks      = s->sbi.s_blocks_count;
				usage->free_blocks = le2cpu32 (s->sbi.s_sb->s_free_blocks_count);
				usage->inodes      = s->sbi.s_inodes_count;
				usage->free_inodes = le2cpu32 (s->sbi.s_sb->s_free_inodes_count);
			}

			return E_OK;
		}
		case V_CNTR_WP:
		{
			long r;

			r = bio.config (dir->dev, BIO_WP, arg);
			if (r || (arg == ASK))
				return r;

			r = EINVAL;
			if (BIO_WP_CHECK (s->di) && !(s->s_flags & MS_RDONLY))
			{
				if (!(s->s_flags & S_NOT_CLEAN_MOUNTED))
				{
					s->sbi.s_sb->s_state = cpu2le16 (le2cpu16 (s->sbi.s_sb->s_state) | EXT2_VALID_FS);
					bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);
				}

				bio.sync_drv (s->di);

				s->s_flags |= MS_RDONLY;
				ALERT (("Ext2-FS [%c]: remounted read-only!", DriveToLetter(dir->dev)));

				r = E_OK;
			}
			else if (s->s_flags & MS_RDONLY)
			{
				s->sbi.s_sb->s_state = cpu2le16 (le2cpu16 (s->sbi.s_sb->s_state) & ~EXT2_VALID_FS);
				bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);

				bio.sync_drv (s->di);

				s->s_flags &= ~MS_RDONLY;
				ALERT (("Ext2-FS [%c]: remounted read/write!", DriveToLetter(dir->dev)));

				r = E_OK;
			}

			return r;
		}
		case V_CNTR_WB:
		{
			return bio.config (dir->dev, BIO_WB, arg);
		}
		case FUTIME:
		case FUTIME_UTC:
		{
			fcookie fc;
			COOKIE *c;
			int uid;

			{
				long r;

				r = e_lookup (dir, name, &fc);
				if (r) return r;

				c = (COOKIE *) fc.index;
			}

			DEBUG (("Ext2-FS [%c]: e_fscntl (FUTIME%s) on #%li", DriveToLetter(c->dev), ((cmd == FUTIME) ? "" : "_UTC"), c->inode));

			/* only the owner or super-user can touch
			 */
			uid = p_geteuid ();
			if ((uid && uid != le2cpu16 (c->in.i_uid))
				|| IS_IMMUTABLE (c))
			{
				e_release (&fc);
				return EACCES;
			}

			if (s->s_flags & MS_RDONLY)
			{
				e_release (&fc);
				return EROFS;
			}

			c->in.i_ctime = cpu2le32 (CURRENT_TIME);

			if (arg)
			{
				if (native_utc || (cmd == FUTIME_UTC))
				{
					long *timeptr = (long *) arg;

					c->in.i_atime = cpu2le32 (timeptr[0]);
					c->in.i_mtime = cpu2le32 (timeptr[1]);
				}
				else
				{
					MUTIMBUF *buf = (MUTIMBUF *) arg;

					c->in.i_atime = cpu2le32 (unixtime (buf->actime, buf->acdate));
					c->in.i_mtime = cpu2le32 (unixtime (buf->modtime, buf->moddate));
				}
			}
			else
			{
				c->in.i_atime =
				c->in.i_mtime = c->in.i_ctime;
			}

			mark_inode_dirty (c);
			e_release (&fc);

			bio_SYNC_DRV ((&bio), s->di);
			return E_OK;
		}
		case FTRUNCATE:
		{
			COOKIE *c;
			fcookie fc;

			{
				long r;

				r = e_lookup (dir, name, &fc);
				if (r) return r;

				c = (COOKIE *) fc.index;
			}

			DEBUG (("Ext2-FS [%c]: e_fscntl (FTRUNCATE) on #%li", DriveToLetter(c->dev), c->inode));

			if (s->s_flags & MS_RDONLY)
			{
				e_release (&fc);
				return EROFS;
			}

			if (!EXT2_ISREG (le2cpu16 (c->in.i_mode)))
			{
				e_release (&fc);
				return EACCES;
			}

			if (IS_IMMUTABLE (c)
				|| le2cpu32 (c->in.i_size) < *((unsigned long *) arg))
			{
				e_release (&fc);
				return EACCES;
			}

			ext2_truncate (c, *(unsigned long *)arg);
			e_release (&fc);

			bio_SYNC_DRV ((&bio), s->di);
			return E_OK;
		}
# ifdef EXT2FS_DEBUG
		case (('e' << 8) | 0xf):
		{
			long len;
			struct { char *buf; long bufsize; } *descr = (void *) arg;

			dump_inode_cache (descr->buf, descr->bufsize);
			len = strlen (descr->buf);
			dump_dir_cache (descr->buf + len, descr->bufsize - len);

			return E_OK;
		}
# endif
	}

	DEBUG (("Ext2-FS: e_fscntl: invalid cmd or not supported"));
	return ENOSYS;
}

static long _cdecl
e_dskchng (int drv, int mode)
{
	SI *s = super [drv];
	long change = 1;

	if (mode == 0)
	{
		change = BIO_DSKCHNG (s->di);
	}

	if (change == 0)
	{
		/* no change */
		DEBUG (("Ext2-FS [%c]: e_dskchng (mode = %i): leave no change", DriveToLetter(drv), mode));
		return change;
	}

	DEBUG (("Ext2-FS [%c]: e_dskchng (mode = %i): invalidate drv (change = %li, memory = %li)", DriveToLetter(drv), mode, change, memory));

	/* free the DI (invalidate also the cache units) */
	bio.free_di (s->di);

	/* clear directory cache */
	d_inv_dir (drv);

	/* clear inode cache */
	inv_ctable (drv);

	/* free allocated memory */
	kfree (s->sbi.s_group_desc, s->sbi.s_group_desc_size);
	kfree (s, sizeof (*s));

	super [drv] = NULL;

	DEBUG (("e_dskchng: leave (change = %li, memory = %li)", change, memory));
	return change;
}

static long _cdecl
e_release (fcookie *fc)
{
	register COOKIE *c = (COOKIE *) fc->index;

	DEBUG (("Ext2-FS [%c]: e_release: #%li : %li", DriveToLetter(fc->dev), c->inode, c->links));

	rel_cookie (c);

	return E_OK;
}

static long _cdecl
e_dupcookie (fcookie *dst, fcookie *src)
{
	((COOKIE *) src->index)->links++;
	*dst = *src;

	return E_OK;
}

static long _cdecl
e_sync (void)
{
	/* sync the inode cache */
	sync_cookies ();

	/* buffer cache automatically synced */
	return E_OK;
}

static long _cdecl
e_mknod (fcookie *dir, const char *name, ulong mode)
{
	return ENOSYS;
# if 0
int ext2_mknod (struct inode * dir, struct dentry *dentry, int mode, int rdev)
{
	struct inode * inode;
	struct buffer_head * bh;
	struct ext2_dir_entry_2 * de;
	int err = -EIO;

	if (ext2_search_entry (dirc, name, namelen))
		return EEXIST;

	err = -ENAMETOOLONG;
	if (dentry->d_name.len > EXT2_NAME_LEN)
		goto out;

	inode = ext2_new_inode (dir, mode, &err);
	if (!inode)
		goto out;

	inode->i_uid = current->fsuid;
	inode->i_mode = mode;
	inode->i_op = NULL;
	bh = ext2_add_entry (dir, dentry->d_name.name, dentry->d_name.len, &de, &err);
	if (!bh)
		goto out_no_entry;
	de->inode = cpu_to_le32(inode->i_ino);
	dir->i_version = ++event;
	if (S_ISREG(inode->i_mode)) {
		inode->i_op = &ext2_file_inode_operations;
		if (EXT2_HAS_INCOMPAT_FEATURE(dir->i_sb,
					      EXT2_FEATURE_INCOMPAT_FILETYPE))
			de->file_type = EXT2_FT_REG_FILE;
	} else if (S_ISCHR(inode->i_mode)) {
		inode->i_op = &chrdev_inode_operations;
		if (EXT2_HAS_INCOMPAT_FEATURE(dir->i_sb,
					      EXT2_FEATURE_INCOMPAT_FILETYPE))
			de->file_type = EXT2_FT_CHRDEV;
	} else if (S_ISBLK(inode->i_mode)) {
		inode->i_op = &blkdev_inode_operations;
		if (EXT2_HAS_INCOMPAT_FEATURE(dir->i_sb,
					      EXT2_FEATURE_INCOMPAT_FILETYPE))
			de->file_type = EXT2_FT_BLKDEV;
	} else if (S_ISFIFO(inode->i_mode))  {
		init_fifo(inode);
		if (EXT2_HAS_INCOMPAT_FEATURE(dir->i_sb,
					      EXT2_FEATURE_INCOMPAT_FILETYPE))
			de->file_type = EXT2_FT_FIFO;
	}
	if (S_ISBLK(mode) || S_ISCHR(mode))
		inode->i_rdev = to_kdev_t(rdev);
	mark_inode_dirty(inode);
	mark_buffer_dirty(bh, 1);
	if (IS_SYNC(dir)) {
		ll_rw_block (WRITE, 1, &bh);
		wait_on_buffer (bh);
	}
	d_instantiate(dentry, inode);
	brelse(bh);
	err = 0;
out:
	return err;

out_no_entry:
	inode->i_nlink--;
	mark_inode_dirty(inode);
	iput(inode);
	goto out;
}
# endif
}

static long _cdecl
e_unmount (int drv)
{
	SI *s = super [drv];

	if (!(s->s_flags & MS_RDONLY) && !(s->s_flags & S_NOT_CLEAN_MOUNTED))
	{
		s->sbi.s_sb->s_state = cpu2le16 (le2cpu16 (s->sbi.s_sb->s_state) | EXT2_VALID_FS);
		bio_MARK_MODIFIED (&bio, s->sbi.s_sb_unit);
	}
	else
	{
		DEBUG (("can't unmount cleanly"));
	}

	/* sync the inode cache */
	sync_cookies ();

	/* sync the buffer cache */
	bio.sync_drv (s->di);

	/* free the DI (invalidate also the cache units) */
	bio.free_di (s->di);

	/* clear directory cache */
	d_inv_dir (drv);

	/* clear inode cache */
	inv_ctable (drv);

	/* free allocated memory */
	kfree (s->sbi.s_group_desc, s->sbi.s_group_desc_size);
	kfree (s, sizeof (*s));

	super [drv] = NULL;

	return E_OK;
}
