/*
 * Filename:     ramfs.c
 * Version:      0.71
 * Author:       Frank Naumann
 * Started:      1998-08-19
 * Last Updated: 2000-08-25
 * Target O/S:   TOS/MiNT
 * Description:  dynamic ramdisk-xfs for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@fishpool.com).
 * 
 * Copying:      Copyright 1998, 1999, 2000 Frank Naumann
 *               <fnaumann@freemint.de>
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
 * 
 * 
 * changes since last version:
 * 
 * 2000-08-25:	(v0.72)
 * 
 * - new: merged into kernel
 *        -> better Dfree() (report now the complete free memory)
 * 
 * 2000-04-01:	(v0.71)
 * 
 * - fix: return correct number of blocks in stat64
 * 
 * 2000-03-16:	(v0.70)
 * 
 * - new: changed completly to UTC mode
 * - new: complete support of new STAT, reorganized internal
 *        data organisation
 * - new: added additional flags and checks
 * - new: added soft write protect mode
 * - fix: lot of code cleanup
 * - fix: bad bug in lseek (SEEK_END incorrect handled)
 * 
 * 1999-07-25:	(v0.61)
 * 
 * - new: changed to extended FILESYS struct
 * - new: removed start.s, now with entry _init
 *        supported by new ld
 * 
 * 1999-07-06:	(v0.60)
 * 
 * - fix: bug in chown: -1 accepted
 * - fix: bug in rename: wrong return value for cross device rename
 * 
 * 1999-05-18:	(v0.59)
 * 
 * - fix: bug in __creat; wrong error behaviour
 * - fix: bug in rename; referenced a freed pointer
 * - fix: bug in __dir_insert; prev ptr was not correct
 * 
 * 1999-05-05:	(v0.58)
 * 
 * - fix: bug in __dir_empty; always true returned
 * 
 * 1999-03-03:	(v0.57)
 * 
 * - fix: bug in rename (casesensitive rename)
 * 
 * 1999-02-04:	(v0.56)
 * 
 * - new: xattr->size information for directories (kmalloc'ed memory)
 * - fix: getxattr: nblocks was not calculated
 * - new: new dir entries are inserted on the tail of the linked list
 * - new: "." and ".." emulation
 * 
 * 1999-01-06:	(v0.55)
 * 
 * - fix: real unix like unlink work now like real unix like unlink
 *        (definitly)
 * - fix: added _cdecl keyword to interface functions
 * 
 * 1998-12-22:	(v0.54)
 * 
 * - fix: real unix like unlink
 *        open files can now be unlinked (inodes are deleted on close)
 *        dir entry is always deleted
 * 
 * 1998-10-22:	(v0.53)
 * 
 * - fix: some changes releated to device number
 * - add: Dpathconf (DP_VOLNAMEMAX)
 * - fix: init() fs_descr is now static
 * 
 * 1998-09-24:	(v0.52b)
 * 
 * - fix: init() - dev number is initialized correct now
 * - new: Dcntl(FS_INFO)
 * - new: Dcntl(FS_USAGE)
 * 
 * 1998-09-08:	(v0.51b)
 * 
 * - new: advisotory filelocking
 * 
 * 1998-08-26:	(v0.5b)
 * 
 * - change: xfs is now casepreserving (previous: casesensitiv)
 *           -> this reduce trouble with tools that run in MiNT-Domain
 *              but can't handle with long and casesensitive names
 * 
 * 1998-08-25:	(v0.4b)
 * 
 * - new: better & faster smallblock handling,
 *        smallblock preallocation
 * - add: __FTRUNCATE
 * 
 * 1998-08-23:	(v0.3b)
 * 
 * - inital revision
 * 
 * 
 * known bugs:
 * 
 * - nothing
 * 
 * todo:
 * 
 * - what about a minimum/maximum limit?
 * - fake x-bit?
 * - no name conversion
 * 
 */

# include "ramfs.h"
# include "global.h"

# include "libkern/libkern.h"

# include "dev-null.h"
# include "dos.h"
# include "filesys.h"
# include "init.h"
# include "kmemory.h"
# include "memory.h"
# include "proc.h"
# include "time.h"


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	72
# define VER_STATUS	

/*
 * startup messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p dynamic RAM filesystem driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\275 " MSG_BUILDDATE " by Frank Naumann.\r\n"

/*
 * default settings
 */

# define DEFAULT_DMODE		(0777)

/*
 * existing FreeMiNT internal memory management
 * works with 8k aligned blocks!
 */
# if 0
# define BLOCK_SIZE	4096L
# define BLOCK_SHIFT	12
# else
# define BLOCK_SIZE	8192L
# define BLOCK_SHIFT	13
# endif
# define BLOCK_MASK	(BLOCK_SIZE - 1)
# define BLOCK_MIN	2	/* preallocate 1/4 smallblock */

# define INITIAL_BLOCKS	16

/*
 * fake for pathconf (no internal limit in real),
 * but some tools doesn't like "Dpathconf(DP_NAMEMAX) == UNLIMITED"
 */
# define MAX_NAME	256	/* include '\0' */

/*
 * debugging stuff
 */

# if 0
# define FS_DEBUG 1
# endif

/****************************************************************************/
/* BEGIN tools */

# define IS_DIR(c)		(((c)->stat.mode & S_IFMT) == S_IFDIR)
# define IS_REG(c)		(((c)->stat.mode & S_IFMT) == S_IFREG)
# define IS_SLNK(c)		(((c)->stat.mode & S_IFMT) == S_IFLNK)

# define IS_SETUID(c)		((c)->stat.mode & S_ISUID)
# define IS_SETGID(c)		((c)->stat.mode & S_ISGID)
# define IS_STICKY(c)		((c)->stat.mode & S_ISVTX)

# define IS_APPEND(c)		((c)->flags & S_APPEND)
# define IS_IMMUTABLE(c)	((c)->flags & S_IMMUTABLE)

static ulong memory = 0;

INLINE void *
ram_kmalloc (register long size)
{
	register void *tmp;
	
	DEBUG  (("fnramfs: kmalloc called: %li", size));
	
	tmp = kmalloc (size);
	if (tmp) memory += size;
	return tmp;
}

INLINE void
ram_kfree (void *dst, register long size)
{
	memory -= size;
	kfree (dst);
}

# undef kmalloc
# undef kfree

# define kmalloc	ram_kmalloc
# define kfree		ram_kfree

/* END tools */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/*
 * filesystem
 */

static long	_cdecl ram_root		(int drv, fcookie *fc);

static long	_cdecl ram_lookup	(fcookie *dir, const char *name, fcookie *fc);
static DEVDRV *	_cdecl ram_getdev	(fcookie *fc, long *devsp);
static long	_cdecl ram_getxattr	(fcookie *fc, XATTR *xattr);
static long	_cdecl ram_stat64	(fcookie *fc, STAT *stat);

static long	_cdecl ram_chattr	(fcookie *fc, int attrib);
static long	_cdecl ram_chown	(fcookie *fc, int uid, int gid);
static long	_cdecl ram_chmode	(fcookie *fc, unsigned mode);

static long	_cdecl ram_mkdir	(fcookie *dir, const char *name, unsigned mode);
static long	_cdecl ram_rmdir	(fcookie *dir, const char *name);
static long	_cdecl ram_creat	(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
static long	_cdecl ram_remove	(fcookie *dir, const char *name);
static long	_cdecl ram_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl ram_rename	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

static long	_cdecl ram_opendir	(DIR *dirh, int flags);
static long	_cdecl ram_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl ram_rewinddir	(DIR *dirh);
static long	_cdecl ram_closedir	(DIR *dirh);

static long	_cdecl ram_pathconf	(fcookie *dir, int which);
static long	_cdecl ram_dfree	(fcookie *dir, long *buf);
static long	_cdecl ram_writelabel	(fcookie *dir, const char *name);
static long	_cdecl ram_readlabel	(fcookie *dir, char *name, int namelen);

static long	_cdecl ram_symlink	(fcookie *dir, const char *name, const char *to);
static long	_cdecl ram_readlink	(fcookie *file, char *buf, int len);
static long	_cdecl ram_hardlink	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
static long	_cdecl ram_fscntl	(fcookie *dir, const char *name, int cmd, long arg);
static long	_cdecl ram_dskchng	(int drv, int mode);

static long	_cdecl ram_release	(fcookie *fc);
static long	_cdecl ram_dupcookie	(fcookie *dst, fcookie *src);
static long	_cdecl ram_sync		(void);

FILESYS ramfs_filesys =
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
	
	root:			ram_root,
	lookup:			ram_lookup,
	creat:			ram_creat,
	getdev:			ram_getdev,
	getxattr:		ram_getxattr,
	chattr:			ram_chattr,
	chown:			ram_chown,
	chmode:			ram_chmode,
	mkdir:			ram_mkdir,
	rmdir:			ram_rmdir,
	remove:			ram_remove,
	getname:		ram_getname,
	rename:			ram_rename,
	opendir:		ram_opendir,
	readdir:		ram_readdir,
	rewinddir:		ram_rewinddir,
	closedir:		ram_closedir,
	pathconf:		ram_pathconf,
	dfree:			ram_dfree,
	writelabel:		ram_writelabel,
	readlabel:		ram_readlabel,
	symlink:		ram_symlink,
	readlink:		ram_readlink,
	hardlink:		ram_hardlink,
	fscntl:			ram_fscntl,
	dskchng:		ram_dskchng,
	release:		ram_release,
	dupcookie:		ram_dupcookie,
	sync:			ram_sync,
	
	/* FS_EXT_1 */
	mknod:			NULL,
	unmount:		NULL,
	
	/* FS_EXT_2
	 */
	
	/* FS_EXT_3 */
	stat64:			ram_stat64,
	res1:			0, 
	res2:			0,
	res3:			0,
	
	lock: 0, sleepers: 0,
	block: NULL, deblock: NULL
};

/*
 * device driver
 */

static long	_cdecl ram_open		(FILEPTR *f);
static long	_cdecl ram_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl ram_read		(FILEPTR *f, char *buf, long bytes);
static long	_cdecl ram_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl ram_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl ram_datime	(FILEPTR *f, ushort *time, int flag);
static long	_cdecl ram_close	(FILEPTR *f, int pid);

static DEVDRV devtab =
{
	open:			ram_open,
	write:			ram_write,
	read:			ram_read,
	lseek:			ram_lseek,
	ioctl:			ram_ioctl,
	datime:			ram_datime,
	close:			ram_close,
	select:			null_select,
	unselect:		null_unselect,
	writeb:			NULL,
	readb:			NULL
};


typedef struct blocks BLOCKS;
typedef struct symlnk SYMLNK;
typedef struct dirlst DIRLST;
typedef struct diroot DIROOT;
typedef struct cookie COOKIE;
typedef struct super  SUPER;


struct blocks
{
	long	size;
	char 	**table;
	long	small_size;
	char 	*small;
};

struct symlnk
{
	char	*name;
	ushort	len;
};

struct dirlst
{
	DIRLST	*prev;
	DIRLST	*next;
	
	ushort	lock;		/* a reference left */
	ushort	len;		/* sizeof name */
	char	*name;		/* name */
	COOKIE	*cookie;	/* cookie for this file */
	
	char	sname [20];	/* short names */
# define SHORT_NAME	20
};

struct diroot
{
	DIRLST	*start;
	DIRLST	*end;
};


struct cookie
{
	COOKIE	*parent;	/* parent directory */
	SUPER	*s;		/* super pointer */
	
	ulong 	links;		/* in use counter */
	ulong	flags;		/* some flags */
	STAT	stat;		/* file attribute */
	
	FILEPTR *open;		/* linked list of open file ptrs */
	LOCK	*locks;		/* locks for this file */
	
	union
	{
		BLOCKS	data;
		SYMLNK	symlnk;
		DIROOT	dir;
	} data;
};

struct super
{
	COOKIE	*root;
	char	*label;		/* FS label */
	ulong	flags;		/* FS flags */
# define MS_RDONLY		   1	/* Mount read-only */
# define MS_NOSUID		   2	/* Ignore suid and sgid bits */
# define MS_NODEV		   4	/* Disallow access to device special files */
# define MS_NOEXEC		   8	/* Disallow program execution */
# define MS_SYNCHRONOUS		  16	/* Writes are synced at once */
# define MS_REMOUNT		  32	/* Alter flags of a mounted FS */
# define MS_MANDLOCK		  64	/* Allow mandatory locks on an FS */
# define S_QUOTA		 128	/* Quota initialized for file/directory/symlink */
# define S_APPEND		 256	/* Append-only file */
# define S_IMMUTABLE		 512	/* Immutable file */
# define MS_NOATIME		1024	/* Do not update access times. */
# define MS_NODIRATIME		2048    /* Do not update directory access times */
# define S_NOT_CLEAN_MOUNTED	4096	/* not cleanly mounted */
};


/*
 * debugging stuff
 */

# ifndef FS_DEBUG

# define RAM_ASSERT(x)		
# define RAM_FORCE(x)		FORCE x
# define RAM_ALERT(x)		ALERT x
# define RAM_DEBUG(x)		
# define RAM_TRACE(x)		

# else

# define RAM_ASSERT(x)		{ assert x; }
# define RAM_FORCE(x)		FORCE x
# define RAM_ALERT(x)		ALERT x
# define RAM_DEBUG(x)		DEBUG (x)
# define RAM_TRACE(x)		TRACE (x)

# endif


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

# define CURRENT_TIME	xtime.tv_sec

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
	
	RAM_DEBUG (("ramfs: __dir_search: search: %s!", name));
	
	while (tmp)
	{
		RAM_DEBUG (("ramfs: __dir_search: compare with: %s!", tmp->name));
		
		if (stricmp (tmp->name, name) == 0)
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
	
	RAM_DEBUG (("ramfs: __creat: enter"));
	
	if (!IS_DIR (d))
	{
		RAM_DEBUG (("ramfs: __creat: dir not a DIR!"));
		return EACCES;
	}
	
	r = __validate_name (name);
	if (r)
	{
		RAM_DEBUG (("ramfs: __creat: not a valid name!"));
		return r;
	}
	
	f = __dir_search (d, name);
	if (f)
	{
		RAM_DEBUG (("ramfs: __creat: object already exist"));
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
		(*new)->flags	= d->flags;
		
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
			RAM_DEBUG (("ramfs: __creat: __dir_insert fail!"));
			kfree (*new, sizeof (**new));
		}
	}
	else
	{
		RAM_DEBUG (("ramfs: __creat: kmalloc fail!"));
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
		RAM_DEBUG (("ramfs: __unlink_cookie: nlink > 0, inode not deleted"));
	}
	else
	{
		if (c->links > 0)
		{
			RAM_DEBUG (("ramfs: __unlink_cookie: inode in use (%li), not deleted", c->links));
		}
		else
		{
			RAM_DEBUG (("ramfs: __unlink_cookie: deleting unlinked inode"));
			
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
	
	if (!IS_DIR (d))
	{
		RAM_DEBUG (("ramfs: __unlink: dir not a DIR!"));
		return EACCES;
	}
	
	f = __dir_search (d, name);
	if (!f)
	{
		RAM_DEBUG (("ramfs: __unlink: object not found!"));
		return EACCES;
	}
	
	t = f->cookie;
	
	if (IS_DIR (t))
	{
		if (!__dir_empty (t))
		{
			RAM_DEBUG (("ramfs: __unlink: directory not clear!"));
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

void
ramfs_init (void)
{
	char initial [] = "dynamic ram-xfs � by [fn]";
	STAT *s;
	
	
	RAM_DEBUG (("ramfs.c: init"));
	
	
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
	s->dev		= RAM_DRV;
	s->ino		= (long) root;
	s->mode		= S_IFDIR | DEFAULT_DMODE;
	s->nlink	= 1;
	/* uid		= 0; */
	/* gid		= 0; */
	s->rdev		= RAM_DRV;
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
		FATAL ("ramfs: out of memory");
	}
	
	RAM_DEBUG (("ramfs: ok (dev_no = %i)", root->stat.dev));
	
	boot_print (MSG_BOOT);
	boot_print (MSG_GREET);
	boot_print ("\r\n");
}

/* END init & configuration part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN filesystem */

static long _cdecl
ram_root (int drv, fcookie *fc)
{
	if (drv == root->stat.dev)
	{
		RAM_DEBUG (("ramfs: ram_root E_OK (%i)", drv));
		
		fc->fs = &ramfs_filesys;
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
ram_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	COOKIE *c = (COOKIE *) dir->index;
	
	/* sanity checks */
	if (!c || !IS_DIR (c))
	{
		RAM_DEBUG (("ramfs: ram_lookup: bad directory"));
		return ENOTDIR;
	}
	
	/* 1 - itself */
	if (!name || (name[0] == '.' && name[1] == '\0'))
	{	
		c->links++;
		*fc = *dir;
		
		RAM_DEBUG (("ramfs: ram_lookup: leave ok, (name = \".\")"));
		return E_OK;
	}
	
	/* 2 - parent dir */
	if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
	{
		if (c->parent)
		{
			c = c->parent;
			
			fc->fs = &ramfs_filesys;
			fc->dev = c->stat.dev;
			fc->aux = 0;
			fc->index = (long) c;
			
			c->links++;
			
			return E_OK;
		}
		
		/* no parent, ROOT */
		
		*fc = *dir;
		RAM_DEBUG (("ramfs: ram_lookup: leave ok, EMOUNT, (name = \"..\")"));
		return EMOUNT;
	}
	
	/* 3 - normal name */
	{
		DIRLST *f = __dir_search (c, name);
		
		if (f)
		{
			fc->fs = &ramfs_filesys;
			fc->dev = f->cookie->stat.dev;
			fc->aux = 0;
			fc->index = (long) f->cookie;
			
			f->cookie->links++;
			
			return E_OK;
		}
	}
	
	RAM_DEBUG (("ramfs: ram_lookup fail (name = %s, return ENOENT)", name));
	return ENOENT;
}

static DEVDRV * _cdecl
ram_getdev (fcookie *fc, long *devsp)
{
	if (fc->fs == &ramfs_filesys)
		return &devtab;
	
	*devsp = ENOSYS;
	return NULL;
}

static long _cdecl
ram_getxattr (fcookie *fc, XATTR *xattr)
{
	COOKIE *c = (COOKIE *) fc->index;
	
	xattr->mode			= c->stat.mode;
	xattr->index			= c->stat.ino;
	xattr->dev			= c->stat.dev;
	xattr->rdev 			= c->stat.rdev;
	xattr->nlink			= c->stat.nlink;
	xattr->uid			= c->stat.uid;
	xattr->gid			= c->stat.gid;
	xattr->size 			= c->stat.size;
	xattr->blksize			= c->stat.blksize;
	xattr->nblocks			= (c->stat.size + BLOCK_SIZE) >> BLOCK_SHIFT;
	*((long *) &(xattr->mtime))	= c->stat.mtime.time;
	*((long *) &(xattr->atime))	= c->stat.atime.time;
	*((long *) &(xattr->ctime))	= c->stat.ctime.time;
	
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
ram_stat64 (fcookie *fc, STAT *stat)
{
	COOKIE *c = (COOKIE *) fc->index;
	
	bcopy (&(c->stat), stat, sizeof (*stat));
	
	stat->blocks = (stat->size + BLOCK_SIZE) >> BLOCK_SHIFT;
	/* blocks is measured in 512 byte blocks */
	stat->blocks <<= (BLOCK_SHIFT - 9);
	
	return E_OK;
}

static long _cdecl
ram_chattr (fcookie *fc, int attrib)
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
ram_chown (fcookie *fc, int uid, int gid)
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
ram_chmode (fcookie *fc, unsigned mode)
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
ram_mkdir (fcookie *dir, const char *name, unsigned mode)
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
ram_rmdir (fcookie *dir, const char *name)
{
	COOKIE *c = (COOKIE *) dir->index;
	
	if (c->s->flags & MS_RDONLY)
		return EROFS;
	
	if (IS_IMMUTABLE (c))
		return EACCES;
	
	/* check for an empty dir is in __unlink */
	return __unlink (c, name);
}

static long _cdecl
ram_creat (fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc)
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
		fc->fs = &ramfs_filesys;
		fc->dev = new->stat.dev;
		fc->aux = 0;
		fc->index = (long) new;
		
		new->links++;
	}
	
	return r;
}

static long _cdecl
ram_remove (fcookie *dir, const char *name)
{
	COOKIE *c = (COOKIE *) dir->index;
	long r;
	
	if (c->s->flags & MS_RDONLY)
		return EROFS;
	
	if (IS_IMMUTABLE (c))
		return EACCES;
	
	r = __unlink (c, name);
	
	return r;
}

static long _cdecl
ram_getname (fcookie *root, fcookie *dir, char *pathname, int size)
{
	COOKIE *r = (COOKIE *) root->index;
	COOKIE *d = (COOKIE *) dir->index;
	
	RAM_DEBUG (("ramfs: ram_getname enter"));
	
	pathname [0] = '\0';
	
	/* sanity checks */
	if (!r || !IS_DIR (r))
	{
		RAM_DEBUG (("ramfs: ram_getname: root not a DIR!"));
		return EACCES;
	}
	if (!d || !IS_DIR (d))
	{
		RAM_DEBUG (("ramfs: ram_getname: dir not a DIR!"));
		return EACCES;
	}
	
	while (d)
	{
		DIRLST *f;
		char *name;
		
		if (r == d)
		{
			strrev (pathname);
			
			RAM_DEBUG (("ramfs: ram_getname: leave E_OK: %s", pathname));
			return E_OK;
		}
		
		f = __dir_searchI (d->parent, d);
		if (!f)
		{
			RAM_DEBUG (("ramfs: ram_getname: __dir_searchI failed!"));
			return EACCES;
		}
		
		name = kmalloc (f->len + 1);
		if (!name)
		{
			RAM_ALERT (("ramfs: kmalloc fail in ram_getname!"));
			return ENOMEM;
		}
		
		name [0] = '\\';
		strcpy (name + 1, f->name);
		strrev (name);
		
		size -= f->len - 1;
		if (size <= 0)
		{
			RAM_DEBUG (("ramfs: ram_getname: name to long"));
			return EBADARG;
		}
		
		strcat (pathname, name);
		kfree (name, f->len + 1);
		
		d = d->parent;
	}
	
	pathname [0] = '\0';
	
	RAM_DEBUG (("ramfs: ram_getname: path not found?"));
	return ENOTDIR;
}

static long _cdecl
ram_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	COOKIE *oldc = (COOKIE *) olddir->index;
	COOKIE *newc = (COOKIE *) newdir->index;
	COOKIE *in;
	DIRLST *old;
	DIRLST *new;
	long r;
	
	RAM_DEBUG (("ramfs: ram_rename: enter (old = %s, new = %s)", oldname, newname));
	
	/* on same device? */
	if (olddir->dev != newdir->dev)
	{
		RAM_DEBUG (("ramfs: ram_rename: cross device rename: [%c] -> [%c]!", olddir->dev+'A', newdir->dev+'A'));
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
			RAM_DEBUG (("ramfs: ram_rename: newname already exist!"));
			return EACCES;
		}
	}
	
	/* search old file */
	old = __dir_search (oldc, oldname);
	if (!old)
	{
		RAM_DEBUG (("ramfs: ram_rename: oldfile not found!"));
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
ram_opendir (DIR *dirh, int flags)
{
	COOKIE *c = (COOKIE *) dirh->fc.index;
	DIRLST *l;
	
	if (!IS_DIR (c))
	{
		RAM_DEBUG (("ramfs: ram_opendir: dir not a DIR!"));
		return EACCES;
	}
	
	l = __dir_next (c, NULL);
	if (l) l->lock = 1;
	
	*(DIRLST **) (&dirh->fsstuff) = l;
	
	c->links++;
	
	dirh->index = 0;
	return E_OK;
}

static long _cdecl
ram_readdir (DIR *dirh, char *nm, int nmlen, fcookie *fc)
{
	DIRLST *l;
	long r = ENMFILES;
	
	l = *(DIRLST **) (&dirh->fsstuff);
	if (l)
	{
		RAM_DEBUG (("ramfs: ram_readdir: %s", l->name));
		
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
			RAM_DEBUG (("ramfs: ram_readdir: TOS_SEARCH!"));
		}
		
		if (l->len <= nmlen)
		{
			strcpy (nm, l->name);
			
			fc->fs = &ramfs_filesys;
			fc->dev = l->cookie->stat.dev;
			fc->aux = 0;
			fc->index = (long) l->cookie;
			
			l->cookie->links++;
			
			RAM_DEBUG (("ramfs: ram_readdir: leave ok: %s", nm));
			
			r = E_OK;
		}
		else
		{
			r = EBADARG;
		}
		
		l = __dir_next ((COOKIE *) dirh->fc.index, l);
		if (l) l->lock = 1;
		
		*(DIRLST **) (&dirh->fsstuff) = l;
	}
	
	return r;
}

static long _cdecl
ram_rewinddir (DIR *dirh)
{
	COOKIE *c = (COOKIE *) dirh->fc.index;
	DIRLST *l;
	
	l = *(DIRLST **) (&dirh->fsstuff);
	if (l) l->lock = 0;
	
	l = __dir_next (c, NULL);
	if (l) l->lock = 1;
	
	*(DIRLST **) (&dirh->fsstuff) = l;
	
	dirh->index = 0;
	return E_OK;
}

static long _cdecl
ram_closedir (DIR *dirh)
{
	COOKIE *c = (COOKIE *) dirh->fc.index;
	DIRLST *l;
	
	l = *(DIRLST **) (&dirh->fsstuff);
	if (l) l->lock = 0;
	
	c->links--;
	
	dirh->index = 0;
	return E_OK;
}

static long _cdecl
ram_pathconf (fcookie *dir, int which)
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
ram_dfree (fcookie *dir, long *buf)
{
	long memfree;
	long memused;
	
	RAM_DEBUG (("ramfs: ram_dfree called"));
	
# define FreeMemory	(tot_rsize (core, 0) + tot_rsize (alt, 0))
	
	memfree = FreeMemory;
	memused = memory + BLOCK_SIZE - 1;
	
	*buf++	= memfree >> BLOCK_SHIFT;
	*buf++	= (memfree + memused) >> BLOCK_SHIFT;
	*buf++	= BLOCK_SIZE;
	*buf	= 1;
	
	return E_OK;
}

static long _cdecl
ram_writelabel (fcookie *dir, const char *name)
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
ram_readlabel (fcookie *dir, char *name, int namelen)
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
ram_symlink (fcookie *dir, const char *name, const char *to)
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
			RAM_ALERT (("ramfs: ram_symlink: kmalloc fail!"));
			
			(void) __unlink (c, name);
			r = ENOMEM;
		}
	}
	
	return r;
}

static long _cdecl
ram_readlink (fcookie *file, char *buf, int len)
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
ram_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	COOKIE *fromc = (COOKIE *) fromdir->index;
	COOKIE *toc = (COOKIE *) todir->index;
	DIRLST *from;
	DIRLST *to;
	long r;
	
	RAM_DEBUG (("ramfs: ram_hardlink: enter (from = %s, to = %s)", fromname, toname));
	
	/* on same device? */
	if (fromdir->dev != todir->dev)
	{
		RAM_DEBUG (("ramfs: ram_hardlink: leave failure (cross device hardlink)!"));
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
		RAM_DEBUG (("ramfs: ram_hardlink: toname already exist!"));
		return EACCES;
	}
	
	/* search from file */
	from = __dir_search (fromc, fromname);
	if (!from)
	{
		RAM_DEBUG (("ramfs: ram_hardlink: fromname not found!"));
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
ram_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "ram-xfs");
			return E_OK;
		}
		case FS_INFO:
		{
			struct fs_info *info = (struct fs_info *) arg;
			if (info)
			{
				strcpy (info->name, "ram-xfs");
				info->version = ((long) VER_MAJOR << 16) | (long) VER_MINOR;
				info->type = FS_RAMFS;
				strcpy (info->type_asc, "ramdisk");
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
				RAM_ALERT (("Ext2-FS [%i]: remounted read-only!", dir->dev));
				
				r = E_OK;
			}
			else if (s->flags & MS_RDONLY)
			{
				s->flags &= ~MS_RDONLY;
				RAM_ALERT (("Ext2-FS [%i]: remounted read/write!", dir->dev));
				
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
ram_dskchng (int drv, int mode)
{
	return 0;
}

static long _cdecl
ram_release (fcookie *fc)
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
ram_dupcookie (fcookie *dst, fcookie *src)
{
	COOKIE *c = (COOKIE *) src->index;
	
	c->links++;
	*dst = *src;
	
	return E_OK;
}

static long _cdecl
ram_sync (void)
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
	
	RAM_DEBUG (("ramfs: __FTRUNCATE: enter (%li)", newsize));
	
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
ram_open (FILEPTR *f)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	RAM_DEBUG (("ramfs: ram_open: enter"));
	
	if (!IS_REG (c))
	{
		RAM_DEBUG (("ramfs: ram_open: leave failure, not a valid file"));
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
		RAM_DEBUG (("ramfs: ram_open: file sharing denied"));
		return EACCES;
	}
	
	if ((f->flags & O_TRUNC) && c->stat.size)
	{
		__free_data (c);
		c->stat.size = 0;
	}
	
	if ((f->flags & O_RWMODE) == O_EXEC)
	{
		f->flags = (f->flags ^ O_EXEC) | O_RDONLY;
	}
	
	f->pos = 0;
	f->devinfo = 0;
	f->next = c->open;
	c->open = f;
	
	c->links++;
	
	RAM_DEBUG (("ramfs: ram_open: leave ok"));
	return E_OK;
}

static long _cdecl
ram_write (FILEPTR *f, const char *buf, long bytes)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	char **table = c->data.data.table;
	long size = c->data.data.size;
	
	long todo;
	long temp = c->stat.size - f->pos;
	long offset;
	
	/* POSIX: mtime/ctime may not change for 0 count */
	if (bytes <= 0)
	{
		RAM_DEBUG (("ramfs: ram_write: ERROR (bytes = %li, return 0)", bytes));
		return 0;
	}
	
	if ((c->s->flags & MS_RDONLY) || IS_IMMUTABLE (c))
		return 0;
	
	todo = bytes;
	
	if (!table)
	{
		RAM_DEBUG (("ramfs: ram_write: set up start table!"));
		
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
			RAM_ALERT (("ramfs: ram_write: kmalloc fail in (1)!"));
			return 0;
		}
	}
	
	while (todo > 0)
	{
		temp = f->pos >> BLOCK_SHIFT;
		if (temp >= size)
		{
			RAM_DEBUG (("ramfs: ram_write: resize block array!"));
			
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
				RAM_DEBUG (("ramfs: ram_write: kmalloc fail in resize array (2)!"));
				goto leave;
			}
		}
		
		offset = f->pos & BLOCK_MASK;
		
		if ((todo >> BLOCK_SHIFT) && (offset == 0))
		{
			char *ptr = table [temp];
			
			RAM_DEBUG (("ramfs: ram_write: aligned (temp = %li)", temp));
			
			if (!ptr)
			{
				ptr = kmalloc (BLOCK_SIZE);
				if (!ptr)
				{
					RAM_DEBUG (("ramfs: ram_write: kmalloc fail (3)!"));
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
			
			RAM_DEBUG (("ramfs: ram_write: BYTES (todo = %li, pos = %li)", todo, f->pos));
			
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
						ALERT (("fnramfs: ram_write: internal error!"));
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
						DEBUG (("fnramfs: ram_write: kmalloc fail (6)!"));
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
ram_read (FILEPTR *f, char *buf, long bytes)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	char **table = c->data.data.table;
	
	register long chunk;
	register long done = 0;		/* processed characters */
	
	if (!table)
	{
		RAM_DEBUG (("ramfs: ram_read: table doesn't exist!"));
		return 0;
	}
	
	bytes = MIN (c->stat.size - f->pos, bytes);
	
	/* At or past EOF */
	if (bytes <= 0)
	{
		RAM_DEBUG (("ramfs: ram_read: At or past EOF (bytes = %li)", bytes));
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
		
		RAM_DEBUG (("ramfs: ram_read: partial to align!"));
		
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
			
			RAM_DEBUG (("ramfs: ram_read: aligned transfer!"));
			
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
		
		RAM_DEBUG (("ramfs: ram_read: small transfer!"));
		
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
ram_lseek (FILEPTR *f, long where, int whence)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	RAM_DEBUG (("ramfs: ram_lseek: enter (where = %li, whence = %i)", where, whence));
	
	switch (whence)
	{
		case SEEK_SET:				break;
		case SEEK_CUR:	where += f->pos;	break;
		case SEEK_END:	where += c->stat.size;	break;
		default:	return EINVAL;
	}
	
	if ((where < 0) || (where > c->stat.size))
	{
		RAM_DEBUG (("ramfs: ram_lseek: leave failure EBADARG (where = %li)", where));
		return EBADARG;
	}
	
	f->pos = where;
	
	RAM_DEBUG (("ramfs: ram_lseek: leave ok (f->pos = %li)", f->pos));
	return where;
}

static long _cdecl
ram_ioctl (FILEPTR *f, int mode, void *buf)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	RAM_DEBUG (("ramfs: ram_ioctl: enter (mode = %i)", mode));
	
	switch (mode)
	{
		case FIONREAD:
		{
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
				(void) ram_lseek (f, 0, SEEK_SET);
				(void) ram_lseek (f, pos, SEEK_SET);
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
					long r = ram_lseek (f, 0L, SEEK_CUR);
					t.l.l_start += r;
					break;
				}
				case SEEK_END:
				{
					long r = ram_lseek (f, 0L, SEEK_CUR);
					t.l.l_start = ram_lseek (f, t.l.l_start, SEEK_END);
					(void) ram_lseek (f, r, SEEK_SET);
					break;
				}
				default:
				{
					RAM_DEBUG (("ram_ioctl: invalid value for l_whence\r\n"));
					return ENOSYS;
				}
			}
			
			if (t.l.l_start < 0) t.l.l_start = 0;
			t.l.l_whence = 0;
			
			if (mode == F_GETLK)
			{
				lck = denylock (c->locks, &t);
				if (lck)
					*fl = lck->l;
				else
					fl->l_type = F_UNLCK;
				
				return E_OK;
			}
			
			cpid = p_getpid ();
			
			if (t.l.l_type == F_UNLCK)
			{
				/* try to find the lock */
				LOCK **lckptr = &(c->locks);
				
				lck = *lckptr;
				while (lck)
				{
					if (lck->l.l_pid == cpid
						&& lck->l.l_start == t.l.l_start
						&& lck->l.l_len == t.l.l_len)
					{
						/* found it -- remove the lock */
						*lckptr = lck->next;
						
						RAM_DEBUG (("ram_ioctl: unlocked %lx: %ld + %ld", c, t.l.l_start, t.l.l_len));
						
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
			
			RAM_DEBUG (("ram_ioctl: lock %lx: %ld + %ld", c, t.l.l_start, t.l.l_len));
			
			/* see if there's a conflicting lock */
			while ((lck = denylock (c->locks, &t)) != 0)
			{
				RAM_DEBUG (("ram_ioctl: lock conflicts with one held by %d", lck->l.l_pid));
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
				RAM_ALERT (("ramfs: kmalloc fail in: ram_ioctl (%lx)", c));
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
ram_datime (FILEPTR *f, ushort *time, int flag)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	switch (flag)
	{
		case 0:
		{
			*(ulong *) time = c->stat.mtime.time;
			
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
ram_close (FILEPTR *f, int pid)
{
	COOKIE *c = (COOKIE *) f->fc.index;
	
	RAM_DEBUG (("ramfs: ram_close: enter (f->links = %i)", f->links));
	
	/* if a lock was made, remove any locks of the process */
	if (f->flags & O_LOCK)
	{
		LOCK *lock;
		LOCK **oldlock;
		
		RAM_DEBUG (("ramfs: ram_close: remove lock (pid = %i)", pid));
		
		oldlock = &c->locks;
		lock = *oldlock;
		
		while (lock)
		{
			if (lock->l.l_pid == pid)
			{
				*oldlock = lock->next;
				/* (void) ram_lock ((int) f->devinfo, 1, lock->l.l_start, lock->l.l_len); */
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
			RAM_ALERT (("ramfs: ram_close: remove open FILEPTR failed"));
		}
		
		c->links--;
	}
	
	RAM_DEBUG (("ramfs: ram_close: leave ok"));
	return E_OK;
}

/* END device driver */
/****************************************************************************/
