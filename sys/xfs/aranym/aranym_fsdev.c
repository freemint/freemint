/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * The u:\host filesystem base - derived from the freemint/sys/ramfs.c
 * file.
 *
 * Copyright 2006 Standa Opichal <opichals@seznam.cz>
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this progara; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

# ifndef NO_ARAFS

# include "global.h"

# include "mint/dcntl.h"
# include "mint/emu_tos.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/stat.h"

# include "libkern/libkern.h"

# include "aranym_fsdev.h"


# define INITIAL_BLOCKS	16

/*
 * fake for pathconf (no internal limit in real),
 * but some tools doesn't like "Dpathconf(DP_NAMEMAX) == UNLIMITED"
 */
# define MAX_NAME	256	/* include '\0' */

/*
 * debugging stuff
 */

# if 1
# define FS_DEBUG 1
# endif

/****************************************************************************/
/* BEGIN tools */

# define IS_DIR(c)		S_ISDIR((c)->stat.mode)
# define IS_REG(c)		S_ISREG((c)->stat.mode)
# define IS_SLNK(c)		S_ISLNK((c)->stat.mode)

# define IS_SETUID(c)		((c)->stat.mode & S_ISUID)
# define IS_SETGID(c)		((c)->stat.mode & S_ISGID)
# define IS_STICKY(c)		((c)->stat.mode & S_ISVTX)

# define IS_APPEND(c)		((c)->flags & S_APPEND)
# define IS_IMMUTABLE(c)	((c)->flags & S_IMMUTABLE)
# define IS_PERSISTENT(c)	((c)->flags & S_PERSISTENT)

# define S_INHERITED_MASK	(~ S_PERSISTENT)


static long memory = 0;

static void *
ara_kmalloc (register long size, const char *func)
{
	register void *tmp;

	DEBUG  (("fnarafs: kmalloc called: %li (used mem %li)", size, memory));

	tmp = kmalloc (size);
	if (tmp) memory += size;
	return tmp;
}

static void
ara_kfree (void *dst, register long size, const char *func)
{
	memory -= size;
	kfree (dst);
}

# undef kmalloc
# undef kfree

# undef FUNCTION
# define FUNCTION str(MODULE_NAME)":"__FILE__","str(__LINE__)

# define kmalloc(size)		ara_kmalloc (size, FUNCTION)
# define kfree(place,size)	ara_kfree (place, size, FUNCTION)

/* END tools */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/*
 * filesystem
 */

static long	_cdecl ara_root		(int drv, fcookie *fc);

static long	_cdecl ara_lookup	(fcookie *dir, const char *name, fcookie *fc);
static DEVDRV *	_cdecl ara_getdev	(fcookie *fc, long *devsp);
static long	_cdecl ara_getxattr	(fcookie *fc, XATTR *xattr);
static long	_cdecl ara_stat64	(fcookie *fc, STAT *stat);

static long	_cdecl ara_chattr	(fcookie *fc, int attrib);
static long	_cdecl ara_chown	(fcookie *fc, int uid, int gid);
static long	_cdecl ara_chmode	(fcookie *fc, unsigned mode);

static long	_cdecl ara_mkdir	(fcookie *dir, const char *name, unsigned mode);
static long	_cdecl ara_rmdir	(fcookie *dir, const char *name);
static long	_cdecl ara_creat	(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
static long	_cdecl ara_remove	(fcookie *dir, const char *name);
static long	_cdecl ara_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl ara_rename	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

static long	_cdecl ara_opendir	(DIR *dirh, int flags);
static long	_cdecl ara_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl ara_rewinddir	(DIR *dirh);
static long	_cdecl ara_closedir	(DIR *dirh);

static long	_cdecl ara_pathconf	(fcookie *dir, int which);
static long	_cdecl ara_dfree	(fcookie *dir, long *buf);
static long	_cdecl ara_writelabel	(fcookie *dir, const char *name);
static long	_cdecl ara_readlabel	(fcookie *dir, char *name, int namelen);

static long	_cdecl ara_symlink	(fcookie *dir, const char *name, const char *to);
static long	_cdecl ara_readlink	(fcookie *file, char *buf, int len);
static long	_cdecl ara_hardlink	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
static long	_cdecl ara_fscntl	(fcookie *dir, const char *name, int cmd, long arg);
static long	_cdecl ara_dskchng	(int drv, int mode);

static long	_cdecl ara_release	(fcookie *fc);
static long	_cdecl ara_dupcookie	(fcookie *dst, fcookie *src);
static long	_cdecl ara_sync		(void);

FILESYS arafs_filesys =
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
	FS_NO_C_CACHE		|
	FS_OWN_MEDIACHANGE	|
	FS_REENTRANT_L1		|
	FS_REENTRANT_L2		|
	FS_EXT_2		|
	FS_EXT_3		,

	root:			ara_root,
	lookup:			ara_lookup,
	creat:			ara_creat,
	getdev:			ara_getdev,
	getxattr:		ara_getxattr,
	chattr:			ara_chattr,
	chown:			ara_chown,
	chmode:			ara_chmode,
	mkdir:			ara_mkdir,
	rmdir:			ara_rmdir,
	remove:			ara_remove,
	getname:		ara_getname,
	rename:			ara_rename,
	opendir:		ara_opendir,
	readdir:		ara_readdir,
	rewinddir:		ara_rewinddir,
	closedir:		ara_closedir,
	pathconf:		ara_pathconf,
	dfree:			ara_dfree,
	writelabel:		ara_writelabel,
	readlabel:		ara_readlabel,
	symlink:		ara_symlink,
	readlink:		ara_readlink,
	hardlink:		ara_hardlink,
	fscntl:			ara_fscntl,
	dskchng:		ara_dskchng,
	release:		ara_release,
	dupcookie:		ara_dupcookie,
	sync:			ara_sync,

	/* FS_EXT_1 */
	mknod:			NULL,
	unmount:		NULL,

	/* FS_EXT_2
	 */

	/* FS_EXT_3 */
	stat64:			ara_stat64,
	res1:			0,
	res2:			0,
	res3:			0,

	lock: 0, sleepers: 0,
	block: NULL, deblock: NULL
};

/*
 * device driver
 */

static long	_cdecl ara_open		(FILEPTR *f);
static long	_cdecl ara_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl ara_read		(FILEPTR *f, char *buf, long bytes);
static long	_cdecl ara_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl ara_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl ara_datime	(FILEPTR *f, ushort *time, int flag);
static long	_cdecl ara_close	(FILEPTR *f, int pid);

static long _cdecl
null_select (FILEPTR *f, long p, int mode)
{
	UNUSED (f);
	UNUSED (p);
	
	if ((mode == O_RDONLY) || (mode == O_WRONLY))
	{
		/* we're always ready to read/write */
		return 1;
	}
	
	/* other things we don't care about */
	return E_OK;
}

static void _cdecl
null_unselect (FILEPTR *f, long p, int mode)
{
	UNUSED (f); UNUSED (p); UNUSED (mode);
	/* nothing to do */
}


static DEVDRV devtab =
{
	open:			ara_open,
	write:			ara_write,
	read:			ara_read,
	lseek:			ara_lseek,
	ioctl:			ara_ioctl,
	datime:			ara_datime,
	close:			ara_close,
	select:			null_select,
	unselect:		null_unselect,
	writeb:			NULL,
	readb:			NULL
};


/* directory manipulation */

INLINE long	__dir_empty	(COOKIE *root);
INLINE DIRLST *	__dir_next	(COOKIE *root, DIRLST *actual);
static DIRLST *	__dir_search	(COOKIE *root, const char *name);
INLINE DIRLST *	__dir_searchI	(COOKIE *root, register const COOKIE *searched);
static long	__dir_remove	(COOKIE *root, DIRLST *f);
static long	__dir_insert	(COOKIE *root, COOKIE *cookie, const char *name);


static void	__free_data	(COOKIE *rc);

static long	__creat		(COOKIE *c, const char *name, COOKIE **new, unsigned mode, int attrib);
INLINE long	__unlink_cookie	(COOKIE *c);
static long	__unlink	(COOKIE *d, const char *name);


static long	__FUTIME	(COOKIE *rc, ulong *timeptr);
static long	__FTRUNCATE	(COOKIE *rc, long size);

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

static SUPER super_struct;
static SUPER *super = &super_struct;

static COOKIE root_inode;
static COOKIE *root = &root_inode;

/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN DIR part */

INLINE long
__dir_empty (COOKIE *dir)
{
	DIRLST *list = dir->data.dir.start;

	while (list)
	{
		if (!((list->name [0] == '.' && list->name [1] == '\0')
			|| (list->name [0] == '.' && list->name [1] == '.' && list->name [2] == '\0')))
		{
			return 0;
		}

		list = list->next;
	}

	return 1;
}

INLINE DIRLST *
__dir_next (COOKIE *dir, DIRLST *actual)
{
	if (actual)
		return actual->next;

	return dir->data.dir.start;
}

static DIRLST *
__dir_search (COOKIE *dir, const char *name)
{
	register DIRLST *tmp = dir->data.dir.start;

	DEBUG (("arafs: __dir_search: search: '%s'", name));

	/* fix trailing pathsep (appends an empty name, so return found) */
	if(!name[0])
		return tmp;
	while (tmp)
	{
		DEBUG (("arafs: __dir_search: compare '%s' with: '%s',next=%p", name, tmp->name, tmp->next));

		if ( stricmp (tmp->name, name) == 0)
		{
			return tmp;
		}

		tmp = tmp->next;
	}

	return NULL;
}

INLINE DIRLST *
__dir_searchI (COOKIE *dir, register const COOKIE *searched)
{
	register DIRLST *tmp = dir->data.dir.start;

	while (tmp)
	{
		if (tmp->cookie == searched)
		{
			return tmp;
		}

		tmp = tmp->next;
	}

	return NULL;
}

static long
__dir_remove (COOKIE *dir, DIRLST *element)
{
	if (element)
	{
		if (element->lock)
		{
			return EACCES;
		}

		if (element->prev)
		{
			element->prev->next = element->next;
		}
		if (element->next)
		{
			element->next->prev = element->prev;
		}

		if (dir->data.dir.start == element)
		{
			dir->data.dir.start = element->next;
		}
		if (dir->data.dir.end == element)
		{
			dir->data.dir.end = element->prev;
		}

		if (element->len > SHORT_NAME)
		{
			dir->stat.size -= element->len;
			kfree (element->name, element->len);
		}

		dir->stat.size -= sizeof (*element);
		kfree (element, sizeof (*element));
	}

	return E_OK;
}

static long
__dir_insert (COOKIE *dir, COOKIE *cookie, const char *name)
{
	DIRLST *new;
	long r = ENOMEM;

	new = kmalloc (sizeof (*new));
	if (new)
	{
		new->lock = 0;
		new->cookie = cookie;
		new->len = strlen (name) + 1;

		if (new->len > SHORT_NAME)
		{
			dir->stat.size += new->len;
			new->name = kmalloc (new->len);
		}
		else
			new->name = new->sname;

		if (new->name)
		{
			strcpy (new->name, name);

			dir->stat.size += sizeof (*new);

			if (dir->data.dir.end)
			{
				dir->data.dir.end->next = new;
				new->prev = dir->data.dir.end;
			}
			else
			{
				dir->data.dir.start = new;
				new->prev = NULL;
			}

			new->next = NULL;
			dir->data.dir.end = new;

			r = E_OK;
		}
		else
		{
			kfree (new, sizeof (*new));
		}
	}

	return r;
}

/* END DIR part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN misc part */

static void
__free_data (COOKIE *c)
{
	char **table = c->data.data.table;

	if (table)
	{
		register long size = c->data.data.size;
		register long i = 0;

		while (table [i] && i < size)
		{
			kfree (table [i], BLOCK_SIZE);
			i++;
		}

		kfree (table, size * sizeof (*table));
		c->data.data.table = NULL;
	}

	if (c->data.data.small)
	{
		kfree (c->data.data.small, c->data.data.small_size);
		c->data.data.small = NULL;
	}

	c->stat.size = 0;
}

INLINE long
__validate_name (register const char *name)
{
	ushort len = 0;

	if (!name
		|| (name [0] == '\0')
		|| (name [0] == '.' && name [1] == '\0')
		|| (name [0] == '.' && name [1] == '.' && name [2] == '\0'))
	{
		return EACCES;
	}

	while (*name)
	{
		register char c = *name++;

		len++;
		if (len >= MAX_NAME)
			/* name exceed maximum limit */
			return EBADARG;

		if (c < 32 || c > 126)
			/* Non-ASCII character met */
			return EACCES;

		if (c == '/' || c == '\\' || c == ':')
			/* Directory separator */
			return EACCES;
	}

	return E_OK;
}

static long
__creat (COOKIE *d, const char *name, COOKIE **new, unsigned mode, int attrib)
{
	DIRLST *f;
	long r;

	DEBUG (("arafs: __creat: enter"));

	if (!IS_DIR (d))
	{
		DEBUG (("arafs: __creat: dir not a DIR!"));
		return EACCES;
	}

	r = __validate_name (name);
	if (r)
	{
		DEBUG (("arafs: __creat: not a valid name!"));
		return r;
	}

	f = __dir_search (d, name);
	if (f)
	{
		DEBUG (("arafs: __creat: object already exist"));
		return EACCES;
	}

	*new = kmalloc (sizeof (**new));
	if (*new)
	{
		STAT *s = &((*new)->stat);

		/* clear all */
		bzero (*new, sizeof (**new));

		s->dev		= d->stat.dev;
		s->ino		= (long) *new;
		s->mode		= mode;
		s->nlink	= 1;
		s->uid		= IS_SETUID (d) ? d->stat.uid : p_getuid ();
		s->gid		= IS_SETGID (d) ? d->stat.gid : p_getgid ();
		s->rdev		= d->stat.rdev;
		s->atime.time	= CURRENT_TIME;
		s->mtime.time	= CURRENT_TIME;
		s->ctime.time	= CURRENT_TIME;
		/* size		= 0; */
		/* blocks	= 0; */
		s->blksize	= BLOCK_SIZE;
		/* flags	= 0; */
		/* gen		= 0; */
		/* res [0]..[6]	= 0; */

		(*new)->s	= d->s;
		(*new)->flags	= d->flags & S_INHERITED_MASK;

		if (IS_DIR (*new))
		{
			(*new)->parent = d;

			r = __dir_insert (*new, *new, ".");

			if (!r)
				r = __dir_insert (*new, d, "..");

			if (r && (*new)->data.dir.start)
				__dir_remove (*new, (*new)->data.dir.start);
		}

		if (!r)
			r = __dir_insert (d, *new, name);

		if (r)
		{
			DEBUG (("arafs: __creat: __dir_insert fail!"));
			kfree (*new, sizeof (**new));
		}
	}
	else
	{
		DEBUG (("arafs: __creat: kmalloc fail!"));
		r = ENOMEM;
	}

	return r;
}

INLINE long
__unlink_cookie (COOKIE *c)
{
	/* if no references left, free the memory */
	if (c->stat.nlink > 0)
	{
		DEBUG (("arafs: __unlink_cookie: nlink > 0, inode not deleted"));
	}
	else
	{
		if (c->links > 0)
		{
			DEBUG (("arafs: __unlink_cookie: inode in use (%li), not deleted", c->links));
		}
		else
		{
			DEBUG (("arafs: __unlink_cookie: deleting unlinked inode"));

			if (IS_REG (c))
			{
				__free_data (c);
			}
			else if (IS_DIR (c))
			{
				while (c->data.dir.start)
					__dir_remove (c, c->data.dir.start);
			}
			else if (IS_SLNK (c))
			{
				kfree (c->data.symlnk.name, c->data.symlnk.len);
			}

			kfree (c, sizeof (*c));
		}
	}

	return E_OK;
}

static long
__unlink (COOKIE *d, const char *name)
{
	register DIRLST *f;
	register COOKIE *t;
	long r;

	DEBUG (("arafs: __unlink: %s", name));
	if (!IS_DIR (d))
	{
		DEBUG (("arafs: __unlink: dir not a DIR!"));
		return EACCES;
	}

	f = __dir_search (d, name);
	if (!f)
	{
		DEBUG (("arafs: __unlink: object not found!"));
		return EACCES;
	}

	t = f->cookie;

	if (IS_PERSISTENT (t)) {
		DEBUG (("arafs: __unlink: object is persistent!"));
		return EACCES;
	}

	if (IS_DIR (t))
	{
		if (!__dir_empty (t))
		{
			DEBUG (("arafs: __unlink: directory not clear!"));
			return EACCES;
		}
	}

	/* remove from dir */
	r = __dir_remove (d, f);
	if (r == E_OK)
	{
		/* decrement link counter */
		t->stat.nlink--;

		/* unlink cookie if possible */
		r = __unlink_cookie (t);
	}

	return r;
}

/* END misc part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN init & configuration part */

short arafs_init(short dev)
{
	char initial [] = "dynamic ara-xfs \275 by [fn]";
	STAT *s;


	DEBUG (("arafs.c: init"));


	/*
	 * init internal data:
	 */

	/* clear super struct */
	bzero (super, sizeof (*super));

	super->root	= root;

	/* initial label */
	super->label = kmalloc (strlen (initial) + 1);
	if (super->label)
		strcpy (super->label, initial);

	/* clear root cookie */
	bzero (root, sizeof (*root));

	root->s		= super;
	root->links	= 1;

	/* setup root stat data */
	s		= &(root->stat);
	s->dev		= dev;
	s->ino		= (long) root;
	s->mode		= S_IFDIR | DEFAULT_DMODE;
	s->nlink	= 1;
	/* uid		= 0; */
	/* gid		= 0; */
	s->rdev		= dev;
	s->atime.time	= CURRENT_TIME;
	s->mtime.time	= CURRENT_TIME;
	s->ctime.time	= CURRENT_TIME;
	/* size		= 0; */
	/* blocks	= 0; */
	s->blksize	= BLOCK_SIZE;
	/* flags	= 0; */
	/* gen		= 0; */
	/* res [0]..[6]	= 0; */

	if (__dir_insert (root, root, ".")
		|| __dir_insert (root, root, ".."))
	{
		FATAL ("arafs: out of memory");
	}

	DEBUG (("arafs: ok (dev_no = %i)", (int)root->stat.dev));
	return dev;
}

/* END init & configuration part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN filesystem */

static long _cdecl
ara_root (int drv, fcookie *fc)
{
	if (drv == root->stat.dev)
	{
		DEBUG (("arafs: ara_root E_OK (%i)", drv));

		fc->fs = &arafs_filesys;
		fc->dev = root->stat.dev;
		fc->aux = 0;
		fc->index = (long) root;

		root->links++;

		return E_OK;
	}

	fc->fs = NULL;
	return ENXIO;
}

static long _cdecl
ara_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	COOKIE *c = (COOKIE *) dir->index;

	DEBUG (("arafs: ara_lookup: '%s'", name));
	/* sanity checks */
	if (!c || !IS_DIR (c))
	{
		DEBUG (("arafs: ara_lookup: bad directory"));
		return ENOTDIR;
	}

	/* 1 - itself */
	if (!name || (name[0] == '.' && name[1] == '\0'))
	{
		c->links++;
		*fc = *dir;

		DEBUG (("arafs: ara_lookup: leave ok, (name = \".\")"));
		return E_OK;
	}

	/* 2 - parent dir */
	if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
	{
		if (c->parent)
		{
			c = c->parent;

			fc->fs = &arafs_filesys;
			fc->dev = c->stat.dev;
			fc->aux = 0;
			fc->index = (long) c;

			c->links++;

			return E_OK;
		}

		/* no parent, ROOT */

		*fc = *dir;
		DEBUG (("arafs: ara_lookup: leave ok, EMOUNT, (name = \"..\")"));
		return EMOUNT;
	}

	/* 3 - normal name */
	{
		DIRLST *f = __dir_search (c, name);

		if (f)
		{
			fc->fs = &arafs_filesys;
			fc->dev = f->cookie->stat.dev;
			fc->aux = 0;
			fc->index = (long) f->cookie;

			f->cookie->links++;

			return E_OK;
		}
	}

	DEBUG (("arafs: ara_lookup fail (name = %s, return ENOENT)", name));
	return ENOENT;
}

static DEVDRV * _cdecl
ara_getdev (fcookie *fc, long *devsp)
{
	if (fc->fs == &arafs_filesys)
		return &devtab;

	*devsp = ENOSYS;
	return NULL;
}

static long _cdecl
ara_getxattr (fcookie *fc, XATTR *xattr)
{
	STAT stat;
	long r = ara_stat64( fc, &stat);
	if ( r < 0 ) {
		return r;
	}

	xattr->mode			= stat.mode;
	xattr->index			= stat.ino;
	xattr->dev			= stat.dev;
	xattr->rdev 			= stat.rdev;
	xattr->nlink			= stat.nlink;
	xattr->uid			= stat.uid;
	xattr->gid			= stat.gid;
	xattr->size 			= stat.size;
	xattr->blksize			= stat.blksize;
	xattr->nblocks			= (stat.size + BLOCK_SIZE) >> BLOCK_SHIFT;
	SET_XATTR_TD(xattr,m,stat.mtime.time);
	SET_XATTR_TD(xattr,a,stat.atime.time);
	SET_XATTR_TD(xattr,c,stat.ctime.time);
#if 0
	*((long *) &(xattr->mtime))	= stat.mtime.time;
	*((long *) &(xattr->atime))	= stat.atime.time;
	*((long *) &(xattr->ctime))	= stat.ctime.time;
#endif
	/* fake attr field a little bit */
	if (S_ISDIR (xattr->mode))
	{
		xattr->attr = FA_DIR;
	}
	else
		xattr->attr = (xattr->mode & 0222) ? 0 : FA_RDONLY;

	return E_OK;
}

static long _cdecl
ara_stat64 (fcookie *fc, STAT *stat)
{
	COOKIE *c = (COOKIE *) fc->index;

	long r = c->cops && c->cops->stat ? 
		c->cops->stat( fc, stat) : 0;
	if (r < 0)
	{
		DEBUG (("arafs: arafs_stat: returned = %ld", r));
		return r;
	}

	bcopy (&(c->stat), stat, sizeof (*stat));

	stat->blocks = (stat->size + BLOCK_SIZE) >> BLOCK_SHIFT;
	/* blocks is measured in 512 byte blocks */
	stat->blocks <<= (BLOCK_SHIFT - 9);

	return E_OK;
}

static long _cdecl
ara_chattr (fcookie *fc, int attrib)
{
	COOKIE *c = (COOKIE *) fc->index;

	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	if (attrib & FA_RDONLY)
	{
		c->stat.mode &= ~ (S_IWUSR | S_IWGRP | S_IWOTH);
	}
	else if (!(c->stat.mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
	{
		c->stat.mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
	}

	return E_OK;
}

static long _cdecl
ara_chown (fcookie *fc, int uid, int gid)
{
	COOKIE *c = (COOKIE *) fc->index;

	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	if (uid != -1) c->stat.uid = uid;
	if (gid != -1) c->stat.gid = gid;

	c->stat.ctime.time = CURRENT_TIME;

	return E_OK;
}

static long _cdecl
ara_chmode (fcookie *fc, unsigned mode)
{
	COOKIE *c = (COOKIE *) fc->index;

	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	c->stat.mode = (c->stat.mode & S_IFMT) | (mode & ~S_IFMT);

	return E_OK;
}

static long _cdecl
ara_mkdir (fcookie *dir, const char *name, unsigned mode)
{
	COOKIE *c = (COOKIE *) dir->index;
	COOKIE *new;

	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	return __creat (c, name, &new, mode | S_IFDIR, FA_DIR);
}

static long _cdecl
ara_rmdir (fcookie *dir, const char *name)
{
	COOKIE *c = (COOKIE *) dir->index;

	DEBUG (("arafs: ara_rmdir '%s' enter,flags=%lx,IS_IMMUTABLE=%lx", name, c->s->flags & MS_RDONLY, IS_IMMUTABLE (c) ));
	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	/* check for an empty dir is in __unlink */
	long r = __unlink (c, name);
	DEBUG (("arafs: ara_rmdir '%s' return %ld", name, r));
	return r;
}

static long _cdecl
ara_creat (fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc)
{
	COOKIE *c = (COOKIE *) dir->index;
	COOKIE *new;
	long r;

	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	r = __creat (c, name, &new, mode | S_IFREG, attrib);
	if (r == E_OK)
	{
		fc->fs = &arafs_filesys;
		fc->dev = new->stat.dev;
		fc->aux = 0;
		fc->index = (long) new;

		new->links++;
	}

	return r;
}

static long _cdecl
ara_remove (fcookie *dir, const char *name)
{
	COOKIE *c = (COOKIE *) dir->index;
	long r;

	DEBUG (("arafs: ara_remove '%s' enter", name));
	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	r = __unlink (c, name);

	DEBUG (("arafs: ara_remove '%s' return %ld", name, r));
	return r;
}

static long _cdecl
ara_getname (fcookie *rootc, fcookie *dir, char *pathname, int size)
{
	COOKIE *r = (COOKIE *) rootc->index;
	COOKIE *d = (COOKIE *) dir->index;

	DEBUG (("arafs: ara_getname enter"));

	pathname [0] = '\0';

	/* sanity checks */
	if (!r || !IS_DIR (r))
	{
		DEBUG (("arafs: ara_getname: root not a DIR!"));
		return EACCES;
	}
	if (!d || !IS_DIR (d))
	{
		DEBUG (("arafs: ara_getname: dir not a DIR!"));
		return EACCES;
	}

	while (d)
	{
		DIRLST *f;
		char *name;

		if (r == d)
		{
			strrev (pathname);

			DEBUG (("arafs: ara_getname: leave E_OK: %s", pathname));
			return E_OK;
		}

		f = __dir_searchI (d->parent, d);
		if (!f)
		{
			DEBUG (("arafs: ara_getname: __dir_searchI failed!"));
			return EACCES;
		}

		name = kmalloc (f->len + 1);
		if (!name)
		{
			ALERT (("arafs: kmalloc fail in ara_getname!"));
			return ENOMEM;
		}

		name [0] = '\\';
		strcpy (name + 1, f->name);
		strrev (name);

		size -= f->len - 1;
		if (size <= 0)
		{
			DEBUG (("arafs: ara_getname: name to long"));
			return EBADARG;
		}

		strcat (pathname, name);
		kfree (name, f->len + 1);

		d = d->parent;
	}

	pathname [0] = '\0';

	DEBUG (("arafs: ara_getname: path not found?"));
	return ENOTDIR;
}

static long _cdecl
ara_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	COOKIE *oldc = (COOKIE *) olddir->index;
	COOKIE *newc = (COOKIE *) newdir->index;
	COOKIE *in;
	DIRLST *old;
	DIRLST *new;
	long r;

	DEBUG (("arafs: ara_rename: enter (old = %s, new = %s)", oldname, newname));

	/* on same device? */
	if (olddir->dev != newdir->dev)
	{
		DEBUG (("arafs: ara_rename: cross device rename: [%c] -> [%c]!", DriveToLetter(olddir->dev), DriveToLetter(newdir->dev)));
		return EXDEV;
	}

	if (oldc->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (oldc) || IS_IMMUTABLE (newc))
		return EACCES;

	/* check if file exist */
	new = __dir_search (newc, newname);
	if (new)
	{
		/* check for rename same file (casepreserving) */
		if (!((oldc == newc) && (stricmp (oldname, newname) == 0)))
		{
			DEBUG (("arafs: ara_rename: newname already exist!"));
			return EACCES;
		}
	}

	/* search old file */
	old = __dir_search (oldc, oldname);
	if (!old)
	{
		DEBUG (("arafs: ara_rename: oldfile not found!"));
		return ENOENT;
	}

	in = old->cookie;

	r = __dir_remove (oldc, old);
	if (r == E_OK)
	{
		r = __dir_insert (newc, in, newname);
		if (r)
		{
			(void) __dir_insert (oldc, in, oldname);
		}
	}

	return r;
}

static long _cdecl
ara_opendir (DIR *dirh, int flags)
{
	union { char *c; DIRLST **dir; } ptr; ptr.c = dirh->fsstuff;
	COOKIE *c = (COOKIE *) dirh->fc.index;
	DIRLST *l;

	if (!IS_DIR (c))
	{
		DEBUG (("arafs: ara_opendir: dir not a DIR!"));
		return EACCES;
	}

	l = __dir_next (c, NULL);
	if (l) l->lock = 1;

	*ptr.dir = l;

	c->links++;

	dirh->index = 0;
	return E_OK;
}

static long _cdecl
ara_readdir (DIR *dirh, char *nm, int nmlen, fcookie *fc)
{
	union { char *c; DIRLST **dir; } dirptr;
	DIRLST *l;
	long r = ENMFILES;

	if (!dirh)
	{
		DEBUG (("arafs: ara_readdir: ERROR: dirh=0"));
		return r;
	}
	dirptr.c = dirh->fsstuff;
	if (!dirptr.dir)
	{
		DEBUG (("arafs: ara_readdir: ERROR: dirptr.dir=0"));
		return r;
	}
	l = *dirptr.dir;
	if (l)
	{
		DEBUG (("arafs: ara_readdir: %s", l->name));

		l->lock = 0;

		if ((dirh->flags & TOS_SEARCH) == 0)
		{
			nmlen -= sizeof (long);
			if (nmlen <= 0) return EBADARG;
			*(long *) nm = (long) l->cookie;
			nm += sizeof (long);
		}
		else
		{
			DEBUG (("arafs: ara_readdir: TOS_SEARCH!"));
		}

		if (l->len <= nmlen)
		{
			strcpy (nm, l->name);

			fc->fs = &arafs_filesys;
			fc->dev = l->cookie->stat.dev;
			fc->aux = 0;
			fc->index = (long) l->cookie;

			l->cookie->links++;

			DEBUG (("arafs: ara_readdir: leave ok: %s", nm));

			r = E_OK;
		}
		else
		{
			r = EBADARG;
		}

		l = __dir_next ((COOKIE *) dirh->fc.index, l);
		if (l) l->lock = 1;

		*dirptr.dir = l;
	}

	return r;
}

static long _cdecl
ara_rewinddir (DIR *dirh)
{
	union { char *c; DIRLST **dir; } ptr; ptr.c = dirh->fsstuff;
	COOKIE *c = (COOKIE *) dirh->fc.index;
	DIRLST *l;

	l = *ptr.dir;
	if (l) l->lock = 0;

	l = __dir_next (c, NULL);
	if (l) l->lock = 1;

	*ptr.dir = l;

	dirh->index = 0;
	return E_OK;
}

static long _cdecl
ara_closedir (DIR *dirh)
{
	union { char *c; DIRLST **dir; } ptr; ptr.c = dirh->fsstuff;
	COOKIE *c = (COOKIE *) dirh->fc.index;
	DIRLST *l;

	l = *ptr.dir;
	if (l) l->lock = 0;

	c->links--;

	dirh->index = 0;
	return E_OK;
}

static long _cdecl
ara_pathconf (fcookie *dir, int which)
{
	switch (which)
	{
		case DP_INQUIRE:	return DP_VOLNAMEMAX;
		case DP_IOPEN:		return UNLIMITED;
		case DP_MAXLINKS:	return UNLIMITED;
		case DP_PATHMAX:	return UNLIMITED;
		case DP_NAMEMAX:	return MAX_NAME - 1;
		case DP_ATOMIC:		return BLOCK_SIZE;
		case DP_TRUNC:		return DP_NOTRUNC;
		case DP_CASE:		return DP_CASEINSENS;
		case DP_MODEATTR:	return (DP_ATTRBITS
						| DP_MODEBITS
						| DP_FT_DIR
						| DP_FT_REG
						| DP_FT_LNK
					);
		case DP_XATTRFIELDS:	return (DP_INDEX
						| DP_DEV
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
		case DP_VOLNAMEMAX:	return MAX_NAME - 1;
	}

	return ENOSYS;
}

static long _cdecl
ara_dfree (fcookie *dir, long *buf)
{
	long memfree;
	long memused;

	DEBUG (("arafs: ara_dfree called"));

	memfree = FreeMemory;
	memused = memory + BLOCK_SIZE - 1;

	*buf++	= memfree >> BLOCK_SHIFT;
	*buf++	= (memfree + memused) >> BLOCK_SHIFT;
	*buf++	= BLOCK_SIZE;
	*buf	= 1;

	return E_OK;
}

static long _cdecl
ara_writelabel (fcookie *dir, const char *name)
{
	char *new;

	new = kmalloc (strlen (name) + 1);
	if (new)
	{
		strcpy (new, name);

		kfree (super->label, strlen (super->label) + 1);
		super->label = new;

		return E_OK;
	}

	return EACCES;
}

static long _cdecl
ara_readlabel (fcookie *dir, char *name, int namelen)
{
	ushort len = strlen (super->label);

	if (len < namelen)
	{
		strcpy (name, super->label);
		return E_OK;
	}

	return EBADARG;
}

static long _cdecl
ara_symlink (fcookie *dir, const char *name, const char *to)
{
	COOKIE *c = (COOKIE *) dir->index;
	COOKIE *new;
	long r;

	if (c->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (c))
		return EACCES;

	r = __creat (c, name, &new, S_IFLNK, 0);
	if (r == E_OK)
	{
		ushort len = strlen (to) + 1;

		new->data.symlnk.len = len;
		new->data.symlnk.name = kmalloc (len);
		if (new->data.symlnk.name)
		{
			strcpy (new->data.symlnk.name, to);
		}
		else
		{
			ALERT (("arafs: ara_symlink: kmalloc fail!"));

			(void) __unlink (c, name);
			r = ENOMEM;
		}
	}

	return r;
}

static long _cdecl
ara_readlink (fcookie *file, char *buf, int len)
{
	COOKIE *c = (COOKIE *) file->index;
	long r = EACCES;

	if (IS_SLNK (c))
	{
		if (c->data.symlnk.len <= len)
		{
			strcpy (buf, c->data.symlnk.name);
			r = E_OK;
		}
		else
		{
			r = EBADARG;
		}
	}

	return r;
}

static long _cdecl
ara_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	COOKIE *fromc = (COOKIE *) fromdir->index;
	COOKIE *toc = (COOKIE *) todir->index;
	DIRLST *from;
	DIRLST *to;
	long r;

	DEBUG (("arafs: ara_hardlink: enter (from = %s, to = %s)", fromname, toname));

	/* on same device? */
	if (fromdir->dev != todir->dev)
	{
		DEBUG (("arafs: ara_hardlink: leave failure (cross device hardlink)!"));
		return EACCES;
	}

	if (toc->s->flags & MS_RDONLY)
		return EROFS;

	if (IS_IMMUTABLE (toc))
		return EACCES;

	/* check if name exist */
	to = __dir_search (toc, toname);
	if (to)
	{
		DEBUG (("arafs: ara_hardlink: toname already exist!"));
		return EACCES;
	}

	/* search from file */
	from = __dir_search (fromc, fromname);
	if (!from)
	{
		DEBUG (("arafs: ara_hardlink: fromname not found!"));
		return ENOENT;
	}

	/* create new name */
	r = __dir_insert (toc, from->cookie, toname);
	if (r == E_OK)
	{
		from->cookie->stat.nlink++;
	}

	return r;
}

static long _cdecl
ara_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "arafs");
			return E_OK;
		}
		case FS_INFO:
		{
			struct fs_info *info = (struct fs_info *) arg;
			if (info)
			{
				strcpy (info->name, "ara-xfs");
				info->version = ((long) VER_MAJOR << 16) | (long) VER_MINOR;
				info->type = FS_RAMFS;
				strcpy (info->type_asc, "aradisk");
			}
			return E_OK;
		}
		case FS_USAGE:
		{
			struct fs_usage *usage = (struct fs_usage *) arg;
			if (usage)
			{
				usage->blocksize = BLOCK_SIZE;
				usage->blocks = (FreeMemory + (memory + BLOCK_SIZE - 1)) >> BLOCK_SHIFT;
				usage->free_blocks = usage->blocks - ((memory + BLOCK_SIZE - 1) >> BLOCK_SHIFT);
				usage->inodes = FS_UNLIMITED;
				usage->free_inodes = FS_UNLIMITED;
			}
			return E_OK;
		}
		case V_CNTR_WP:
		{
			SUPER *s = ((COOKIE *) dir->index)->s;
			long r;

			if (arg == ASK)
				return (s->flags & MS_RDONLY);

			r = EINVAL;
			if (!(s->flags & MS_RDONLY))
			{
				s->flags |= MS_RDONLY;
				ALERT (("arafs [%i]: remounted read-only!", dir->dev));

				r = E_OK;
			}
			else if (s->flags & MS_RDONLY)
			{
				s->flags &= ~MS_RDONLY;
				ALERT (("arafs [%i]: remounted read/write!", dir->dev));

				r = E_OK;
			}

			return r;
		}
		case FUTIME:
		case FUTIME_UTC:
		{
			COOKIE *d = (COOKIE *) dir->index;
			COOKIE *c;
			DIRLST *l;
			int uid;
			long r;

			if (!IS_DIR (d))
				return EACCES;

			l = __dir_search (d, name);
			if (!l) return EACCES;

			c = l->cookie;

			/* only the owner or super-user can touch
			 */
			uid = p_geteuid ();
			if ((uid && uid != c->stat.uid)
				|| IS_IMMUTABLE (c))
			{
				return EACCES;
			}

			if (c->s->flags & MS_RDONLY)
			{
				return EROFS;
			}

			r = __FUTIME (c, (ulong *) arg);
			return r;
		}
		case FTRUNCATE:
		{
			COOKIE *d = (COOKIE *) dir->index;
			DIRLST *l;
			long r = EACCES;

			if (!IS_DIR (d)) return EACCES;

			l = __dir_search (d, name);
			if (l) r = __FTRUNCATE (l->cookie, arg);

			return r;
		}
	}

	return ENOSYS;
}

static long _cdecl
ara_dskchng (int drv, int mode)
{
	return 0;
}

static long _cdecl
ara_release (fcookie *fc)
{
	COOKIE *c = (COOKIE *) fc->index;

	c->links--;
	if (!c->links)
	{
		__unlink_cookie (c);
	}

	return E_OK;
}

static long _cdecl
ara_dupcookie (fcookie *dst, fcookie *src)
{
	COOKIE *c = (COOKIE *) src->index;

	c->links++;
	*dst = *src;

	return E_OK;
}

static long _cdecl
ara_sync (void)
{
	return E_OK;
}

/* END filesystem */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver */

/*
 * internal
 */

/* Check for access 'access' for given uid/gid/mode
 * return 0 if access allowed.
 */
INLINE int
check_mode (COOKIE *rc, int euid, int egid, int access)
{
	if (!euid)
		return 0;

	if (euid == rc->stat.uid && (rc->stat.mode & access))
		return 0;

	if (egid == rc->stat.gid && (rc->stat.mode & (access >> 3)))
		return 0;

	if (rc->stat.mode & (access >> 6))
		return 0;

	return 1;
}

static long
__FUTIME (COOKIE *rc, ulong *timeptr)
{
	int uid = p_geteuid ();
	int gid = p_getegid ();

	/*
	 * The owner or super-user can always touch,
	 * others only if timeptr == 0 and write
	 * permission.
	 */
	if (uid
		&& uid != rc->stat.uid
		&& (timeptr || check_mode (rc, uid, gid, S_IWUSR)))
	{
		return EACCES;
	}

	rc->stat.ctime.time = CURRENT_TIME;

	if (timeptr)
	{
		rc->stat.atime.time = timeptr[0];
		rc->stat.mtime.time = timeptr[1];
	}
	else
	{
		register long time = rc->stat.ctime.time;

		rc->stat.atime.time = time;
		rc->stat.mtime.time = time;
	}

	return E_OK;
}

static long
__FTRUNCATE (COOKIE * c, long newsize)
{
	char **table = c->data.data.table;

	DEBUG (("arafs: __FTRUNCATE: enter (%li)", newsize));

	/* sanity checks */
	if (!IS_REG (c))
	{
		return EACCES;
	}
	if (c->stat.size <= newsize)
	{
		return E_OK;
	}

	/* simple check */
	if (newsize == 0)
	{
		__free_data (c);
		return E_OK;
	}

	if (c->data.data.small
		&& (c->stat.size - newsize) > c->data.data.small_size)
	{
		kfree (c->data.data.small, c->data.data.small_size);
		c->data.data.small = NULL;
	}

	c->stat.size = newsize;
	newsize >>= BLOCK_SHIFT;
	newsize++;

	if (table)
	{
		register long size = c->data.data.size;
		register long i = newsize;

		while (table [i] && i < size)
		{
			kfree (table [i], BLOCK_SIZE);
			i++;
		}
	}

	return E_OK;
}


/*
 * external
 */

static long _cdecl
ara_open (FILEPTR *f)
{
	COOKIE *c = (COOKIE *) f->fc.index;

	DEBUG (("arafs: ara_open: enter"));

	if (!IS_REG (c))
	{
		DEBUG (("arafs: ara_open: leave failure, not a valid file"));
		return EACCES;
	}

	if (((f->flags & O_RWMODE) == O_WRONLY)
		|| ((f->flags & O_RWMODE) == O_RDWR))
	{
		if (c->s->flags & MS_RDONLY)
			return EROFS;

		if (IS_IMMUTABLE (c))
			return EACCES;
	}

	if (c->open && denyshare (c->open, f))
	{
		DEBUG (("arafs: ara_open: file sharing denied"));
		return EACCES;
	}

	if ((f->flags & O_TRUNC) && c->stat.size)
	{
		__free_data (c);
	}

	if ((f->flags & O_RWMODE) == O_EXEC)
	{
		f->flags = (f->flags ^ O_EXEC) | O_RDONLY;
	}

	{
		long r = c->cops && c->cops->dev_ops->open ?
			 c->cops->dev_ops->open( f) : 0;
		if (r < 0)
		{
			DEBUG (("arafs: arafs_open: failed (flags = %i)", f->flags));
			return r;
		}
	}
	
	f->pos = 0;
	f->devinfo = 0;
	f->next = c->open;
	c->open = f;

	c->links++;

	DEBUG (("arafs: ara_open: leave ok"));
	return E_OK;
}

static long _cdecl
ara_write (FILEPTR *f, const char *buf, long bytes)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	char **table = c->data.data.table;
	long size = c->data.data.size;

	long todo;
	long offset;

	/* POSIX: mtime/ctime may not change for 0 count */
	if (bytes <= 0)
	{
		DEBUG (("arafs: ara_write: ERROR (bytes = %li, return 0)", bytes));
		return 0;
	}

	if ((c->s->flags & MS_RDONLY) || IS_IMMUTABLE (c))
		return 0;

	todo = bytes;

	if (!table)
	{
		DEBUG (("arafs: ara_write: set up start table!"));

		size = bytes >> BLOCK_SHIFT;
		size += size >> 1;
		size = MAX (size, INITIAL_BLOCKS);

		table = kmalloc (size * sizeof (*table));
		if (table)
		{
			c->data.data.table = table;
			c->data.data.size = size;

			/* clear it! */
			bzero (table, size * sizeof (*table));
		}
		else
		{
			ALERT (("arafs: ara_write: kmalloc fail in (1)!"));
			return 0;
		}
	}

	while (todo > 0)
	{
		long temp = f->pos >> BLOCK_SHIFT;
		if (temp >= size)
		{
			DEBUG (("arafs: ara_write: resize block array!"));

			size <<= 1;
			table = kmalloc (size * sizeof (*table));
			if (table)
			{
				/* clear it! */
				bzero (table, size * sizeof (*table));

				/* save old data */
				bcopy (c->data.data.table, table, c->data.data.size * sizeof (*table));
				/* free old data */
				kfree (c->data.data.table, c->data.data.size * sizeof (*table));

				/* setup new table */
				c->data.data.table = table;
				c->data.data.size = size;
			}
			else
			{
				DEBUG (("arafs: ara_write: kmalloc fail in resize array (2)!"));
				goto leave;
			}
		}

		offset = f->pos & BLOCK_MASK;

		if ((todo >> BLOCK_SHIFT) && (offset == 0))
		{
			char *ptr = table [temp];

			DEBUG (("arafs: ara_write: aligned (temp = %li)", temp));

			if (!ptr)
			{
				ptr = kmalloc (BLOCK_SIZE);
				if (!ptr)
				{
					DEBUG (("arafs: ara_write: kmalloc fail (3)!"));
					goto leave;
				}

				table [temp] = ptr;

				if (c->data.data.small)
				{
					kfree (c->data.data.small, c->data.data.small_size);
					c->data.data.small = NULL;
				}
			}

			bcopy (buf, ptr, BLOCK_SIZE);

			buf += BLOCK_SIZE;
			todo -= BLOCK_SIZE;
			f->pos += BLOCK_SIZE;
		}
		else
		{
			char *ptr = table [temp];
			long data;

			DEBUG (("arafs: ara_write: BYTES (todo = %li, pos = %li)", todo, f->pos));

			data = BLOCK_SIZE - offset;
			data = MIN (todo, data);

			if (!ptr)
			{
				register long s;

				ptr = c->data.data.small;

				if (!ptr)
				{
					if (offset)
					{
						ALERT (("fnarafs: ara_write: internal error!"));
						goto leave;
					}

					c->data.data.small_size = 0;
					s = data;
				}
				else
				{
					s = data + offset;
					s = MAX (s, c->data.data.small_size);
				}

				if (s > c->data.data.small_size)
				{
					s += s >> BLOCK_MIN;
					s = MIN (s, BLOCK_SIZE);

					ptr = kmalloc (s);
					if (!ptr)
					{
						DEBUG (("fnarafs: ara_write: kmalloc fail (6)!"));
						goto leave;
					}

					if (c->data.data.small_size)
					{
						bcopy (c->data.data.small, ptr, c->data.data.small_size);

						kfree (c->data.data.small, c->data.data.small_size);
						c->data.data.small = NULL;
					}
				}

				if (s != BLOCK_SIZE)
				{
					c->data.data.small_size = s;
					c->data.data.small = ptr;
				}
				else
					table [temp] = ptr;
			}

			bcopy (buf, (ptr + offset), data);

			buf += data;
			todo -= data;
			f->pos += data;
		}

		if (f->pos > c->stat.size)
		{
			c->stat.size = f->pos;
		}
	}

leave:
	c->stat.mtime.time = CURRENT_TIME;

	return (bytes - todo);
}

static long _cdecl
ara_read (FILEPTR *f, char *buf, long bytes)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	char **table = c->data.data.table;

	register long chunk;
	register long done = 0;		/* processed characters */

	if (!table)
	{
		DEBUG (("arafs: ara_read: table doesn't exist!"));
		return 0;
	}

	bytes = MIN (c->stat.size - f->pos, bytes);

	/* At or past EOF */
	if (bytes <= 0)
	{
		DEBUG (("arafs: ara_read: At or past EOF (bytes = %li)", bytes));
		return 0;
	}

	chunk = f->pos >> BLOCK_SHIFT;

	/* Are we block aligned? */
	if (f->pos & BLOCK_MASK)
	{
		char *ptr = table [chunk++];

		register long off = f->pos & BLOCK_MASK;
		register long data;

		data = BLOCK_SIZE - off;
		data = MIN (bytes, data);

		DEBUG (("arafs: ara_read: partial to align!"));

		if (!ptr)
			ptr = c->data.data.small;

		if (ptr)
			bcopy (ptr + off, buf, data);
		else
			bzero (buf, data);

		buf += data;
		bytes -= data;
		done += data;
		f->pos += data;
	}

	/* Any full blocks to read ? */
	if (bytes >> BLOCK_SHIFT)
	{
		register long end = bytes;

		while (end >> BLOCK_SHIFT)
		{
			char *ptr = table [chunk++];

			DEBUG (("arafs: ara_read: aligned transfer!"));

			if (!ptr)
				ptr = c->data.data.small;

			if (ptr)
				bcopy (ptr, buf, BLOCK_SIZE);
			else
				bzero (buf, BLOCK_SIZE);

			buf += BLOCK_SIZE;
			end -= BLOCK_SIZE;
			done += BLOCK_SIZE;
			f->pos += BLOCK_SIZE;
		}

		bytes &= BLOCK_MASK;
	}

	/* Anything left ? */
	if (bytes)
	{
		char *ptr = table [chunk];

		DEBUG (("arafs: ara_read: small transfer!"));

		if (!ptr)
			ptr = c->data.data.small;

		if (ptr)
			bcopy (ptr, buf, bytes);
		else
			bzero (buf, bytes);

		done += bytes;
		f->pos += bytes;
	}

	if (!((c->s->flags & MS_NOATIME)
		|| (c->s->flags & MS_RDONLY)
		|| IS_IMMUTABLE (c)))
	{
		/* update time/datestamp */
		c->stat.atime.time = CURRENT_TIME;
	}

	return done;
}

static long _cdecl
ara_lseek (FILEPTR *f, long where, int whence)
{
	COOKIE *c = (COOKIE *) f->fc.index;

	DEBUG (("arafs: ara_lseek: enter (where = %li, whence = %i)", where, whence));

	switch (whence)
	{
		case SEEK_SET:				break;
		case SEEK_CUR:	where += f->pos;	break;
		case SEEK_END:	where += c->stat.size;	break;
		default:	return EINVAL;
	}

	if (where < 0)
	{
		DEBUG (("arafs: ara_lseek: leave failure EBADARG (where = %li)", where));
		return EBADARG;
	}

	f->pos = where;

	DEBUG (("arafs: ara_lseek: leave ok (f->pos = %li)", f->pos));
	return where;
}

static long _cdecl
ara_ioctl (FILEPTR *f, int mode, void *buf)
{
	COOKIE *c = (COOKIE *) f->fc.index;

	DEBUG (("arafs: ara_ioctl: enter (mode = %i)", mode));

	switch (mode)
	{
		case FIONREAD:
		{
			if (c->stat.size - f->pos < 0) 
				*(long *) buf = 0;
			else
				*(long *) buf = c->stat.size - f->pos;

			return E_OK;
		}
		case FIONWRITE:
		{
			*(long *) buf = 1;
			return E_OK;
		}
		case FUTIME:
		case FUTIME_UTC:
		{
			if (c->s->flags & MS_RDONLY)
				return EROFS;

			if (IS_IMMUTABLE (c))
				return EACCES;

			return __FUTIME (c, buf);
		}
		case FTRUNCATE:
		{
			long r;

			if ((f->flags & O_RWMODE) == O_RDONLY)
				return EACCES;

			r = __FTRUNCATE (c, *(long *) buf);
			if (r == E_OK)
			{
				long pos = f->pos;
				(void) ara_lseek (f, 0, SEEK_SET);
				(void) ara_lseek (f, pos, SEEK_SET);
			}

			return r;
		}
		case FIOEXCEPT:
		{
			*(long *) buf = 0;
			return E_OK;
		}
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			struct flock *fl = (struct flock *) buf;

			LOCK t;
			LOCK *lck;

			int cpid;

			t.l = *fl;

			switch (t.l.l_whence)
			{
				case SEEK_SET:
				{
					break;
				}
				case SEEK_CUR:
				{
					long r = ara_lseek (f, 0L, SEEK_CUR);
					t.l.l_start += r;
					break;
				}
				case SEEK_END:
				{
					long r = ara_lseek (f, 0L, SEEK_CUR);
					t.l.l_start = ara_lseek (f, t.l.l_start, SEEK_END);
					(void) ara_lseek (f, r, SEEK_SET);
					break;
				}
				default:
				{
					DEBUG (("ara_ioctl: invalid value for l_whence\r\n"));
					return ENOSYS;
				}
			}

			if (t.l.l_start < 0) t.l.l_start = 0;
			t.l.l_whence = 0;

			cpid = p_getpid ();

			if (mode == F_GETLK)
			{
				lck = denylock (cpid, c->locks, &t);
				if (lck)
					*fl = lck->l;
				else
					fl->l_type = F_UNLCK;

				return E_OK;
			}

			if (t.l.l_type == F_UNLCK)
			{
				/* try to find the lock */
				LOCK **lckptr = &(c->locks);

				lck = *lckptr;
				while (lck)
				{
					if (lck->l.l_pid == cpid
		                                && ((lck->l.l_start == t.l.l_start
						     && lck->l.l_len == t.l.l_len) ||
						    (lck->l.l_start >= t.l.l_start
						     && t.l.l_len == 0)))
					{
						/* found it -- remove the lock */
						*lckptr = lck->next;

						DEBUG (("ara_ioctl: unlocked %p: %ld + %ld", c, t.l.l_start, t.l.l_len));

						/* wake up anyone waiting on the lock */
						wake (IO_Q, (long) lck);
						kfree (lck, sizeof (*lck));

						return E_OK;
					}

					lckptr = &(lck->next);
					lck = lck->next;
				}

				return ENSLOCK;
			}

			DEBUG (("ara_ioctl: lock %p: %ld + %ld", c, t.l.l_start, t.l.l_len));

			/* see if there's a conflicting lock */
			while ((lck = denylock (cpid, c->locks, &t)) != 0)
			{
				DEBUG (("ara_ioctl: lock conflicts with one held by %d", lck->l.l_pid));
				if (mode == F_SETLKW)
				{
					/* sleep a while */
					sleep (IO_Q, (long) lck);
				}
				else
					return ELOCKED;
			}

			/* if not, add this lock to the list */
			lck = kmalloc (sizeof (*lck));
			if (!lck)
			{
				ALERT (("arafs: kmalloc fail in: ara_ioctl (%p)", c));
				return ENOMEM;
			}

			lck->l = t.l;
			lck->l.l_pid = cpid;

			lck->next = c->locks;
			c->locks = lck;

			/* mark the file as being locked */
			f->flags |= O_LOCK;
			return E_OK;
		}
	}

	return ENOSYS;
}

static long _cdecl
ara_datime (FILEPTR *f, ushort *time, int flag)
{
	COOKIE *c = (COOKIE *) f->fc.index;

	/* this makes sure the custom stat is called */
	STAT stat;
	long r = ara_stat64( &f->fc, &stat);
	if ( r < 0 ) {
		return r;
	}

	switch (flag)
	{
		case 0:
		{
			*(ulong *) time = stat.mtime.time;

			break;
		}
		case 1:
		{
			if (c->s->flags & MS_RDONLY)
				return EROFS;

			if (IS_IMMUTABLE (c))
				return EACCES;

			c->stat.mtime.time = *(ulong *) time;
			c->stat.atime.time = c->stat.mtime.time;
			c->stat.ctime.time = CURRENT_TIME;

			break;
		}
		default:
			return EBADARG;
	}

	return E_OK;
}

static long _cdecl
ara_close (FILEPTR *f, int pid)
{
	COOKIE *c = (COOKIE *) f->fc.index;

	DEBUG (("arafs: ara_close: enter (f->links = %i)", f->links));

	{
		long r = c->cops && c->cops->dev_ops->close ? 
			 c->cops->dev_ops->close( f, pid) : 0;
		if (r < 0)
		{
			DEBUG (("arafs: arafs_close: cannot close the file (pid = %i)", pid));
			return r;
		}
	}

	/* if a lock was made, remove any locks of the process */
	if (f->flags & O_LOCK)
	{
		LOCK *lock;
		LOCK **oldlock;

		DEBUG (("arafs: ara_close: remove lock (pid = %i)", pid));

		oldlock = &c->locks;
		lock = *oldlock;

		while (lock)
		{
			if (lock->l.l_pid == pid)
			{
				*oldlock = lock->next;
				/* (void) ara_lock ((int) f->devinfo, 1, lock->l.l_start, lock->l.l_len); */
				wake (IO_Q, (long) lock);
				kfree (lock, sizeof (*lock));
			}
			else
			{
				oldlock = &lock->next;
			}

			lock = *oldlock;
		}
	}

	if (f->links <= 0)
	{
		/* remove the FILEPTR from the linked list */
		register FILEPTR **temp = &c->open;
		register long flag = 1;

		while (*temp)
		{
			if (*temp == f)
			{
				*temp = f->next;
				f->next = NULL;
				flag = 0;
				break;
			}
			temp = &(*temp)->next;
		}

		if (flag)
		{
			ALERT (("arafs: ara_close: remove open FILEPTR failed"));
		}

		c->links--;
	}

	DEBUG (("arafs: ara_close: leave ok"));
	return E_OK;
}

/* END device driver */
/****************************************************************************/

# endif
