/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 *
 * a simple unified file system
 *
 */

# include "unifs.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/dcntl.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/stat.h"

# include "bios.h"
# include "filesys.h"
# include "info.h"
# include "k_prot.h"
# include "kerinfo.h"
# include "kmemory.h"
# include "time.h"
# include "dev-null.h"

# include "biosfs.h"
# include "kernfs.h"
# include "nullfs.h"
# include "pipefs.h"
# include "procfs.h"
# include "ramfs.h"
# include "shmfs.h"
# include "fatfs.h"

# include "proc.h"


static long	_cdecl uni_root		(int drv, fcookie *fc);
static long	_cdecl uni_lookup	(fcookie *dir, const char *name, fcookie *fc);
static long	_cdecl uni_getxattr	(fcookie *fc, XATTR *xattr);
static long	_cdecl uni_rmdir	(fcookie *dir, const char *name);
static long	_cdecl uni_remove	(fcookie *dir, const char *name);
static long	_cdecl uni_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl uni_rename	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);
static long	_cdecl uni_opendir	(DIR *dirh, int flags);
static long	_cdecl uni_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl uni_pathconf	(fcookie *dir, int which);
static long	_cdecl uni_dfree	(fcookie *dir, long *buf);
static DEVDRV *	_cdecl uni_getdev	(fcookie *fc, long *devsp);
static long	_cdecl uni_symlink	(fcookie *dir, const char *name, const char *to);
static long	_cdecl uni_readlink	(fcookie *fc, char *buf, int buflen);
static long	_cdecl uni_fscntl	(fcookie *dir, const char *name, int cmd, long arg);

FILESYS uni_filesys =
{
	NULL,

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
	FS_LONGPATH	|
	FS_REENTRANT_L1	|
	FS_REENTRANT_L2	|
	FS_EXT_2	,

	uni_root,
	uni_lookup, null_creat, uni_getdev, uni_getxattr,
	null_chattr, null_chown, null_chmode,
	null_mkdir, uni_rmdir, uni_remove, uni_getname, uni_rename,
	uni_opendir, uni_readdir, null_rewinddir, null_closedir,
	uni_pathconf, uni_dfree, null_writelabel, null_readlabel,
	uni_symlink, uni_readlink, null_hardlink, uni_fscntl, null_dskchng,
	NULL, NULL,
	NULL,

	/* FS_EXT_1 */
	NULL, NULL,

	/* FS_EXT_2
	 */

	/* FS_EXT_3 */
	NULL,

	0, 0, 0, 0, 0,
	NULL, NULL
};

DEVDRV uni_device =
{
	open:		null_open,
	write:		null_write,
	read:		null_read,
	lseek:		null_lseek,
	ioctl:		null_ioctl,
	datime:		null_datime,
	close:		null_close,
	select:		null_select,
	unselect:	null_unselect,
	writeb:		NULL,
	readb:		NULL
};

/*
 * structure that holds files
 * if (S_ISDIR(mode)), then this is an alias for a drive:
 *	"dev" holds the appropriate BIOS device number, and
 *	"data" is meaningless
 * if (S_ISLNK(mode)), then this is a symbolic link:
 *	"dev" holds the user id of the owner, and
 *	"data" points to the actual link data
 */

typedef struct unifile UNIFILE;
struct unifile
{
# define UNINAME_MAX 15
	char	name[UNINAME_MAX + 1];
	ushort	mode;
	ushort	dev;
	FILESYS	*fs;
	void	*data;
	UNIFILE	*next;
	ushort	cdate;
	ushort	ctime;
};

static UNIFILE u_drvs[UNI_NUM_DRVS];
static UNIFILE *u_root = 0;

static long	do_ulookup	(fcookie *, const char *, fcookie *, UNIFILE **);

FILESYS *
get_filesys (int dev)
{
	UNIFILE *u;

	for (u = u_root; u; u = u->next)
		if (u->dev == dev)
			return u->fs;

	return NULL;
}

void
unifs_init (void)
{
	UNIFILE *u = u_drvs;
	int i;

	u_root = u;
	for (i = 0; i < UNI_NUM_DRVS; i++, u++)
	{
		u->next = u + 1;
		u->mode = S_IFDIR | DEFAULT_DIRMODE;
		u->dev = i;
		u->cdate = datestamp;
		u->ctime = timestamp;

		switch (i)
 		{
			case BIOSDRV:
				strcpy (u->name, "dev");
				u->fs = &bios_filesys;
				break;

			case PIPEDRV:
				strcpy (u->name, "pipe");
				u->fs = &pipe_filesys;
				break;

			case PROCDRV:
				strcpy (u->name, "proc");
				u->fs = &proc_filesys;
				break;
# ifndef NO_RAMFS
			case RAM_DRV:
				strcpy (u->name, "ram");
				u->fs = &ramfs_filesys;
				break;
# endif
			case SHM_DRV:
				strcpy (u->name, "shm");
				u->fs = &shm_filesys;
				break;
# if WITH_KERNFS
			case KERNDRV:
				strcpy (u->name, "kern");
				u->fs = &kern_filesys;
				break;
# endif
			case UNIDRV:
				(u-1)->next = u->next;	/* skip this drive */
				break;

			default:
				/* drives A..Z1..6 */
				u->name[0] = i + ((i < 26) ? 'a' : '1' - 26);
				u->name[1] = '\0';
				u->fs = NULL;
				break;
		}
	}

	/* oops, we went too far */
	u--;
	u->next = NULL;
}

static long _cdecl
uni_root (int drv, fcookie *fc)
{
	if (drv == UNIDRV)
	{
		fc->fs = &uni_filesys;
		fc->dev = drv;
		fc->index = 0L;
		return E_OK;
	}

	fc->fs = 0;
	return EINTERNAL;
}

static long _cdecl
uni_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	return do_ulookup (dir, name, fc, NULL);
}

/* worker function for uni_lookup; can also return the UNIFILE
 * pointer for the root directory
 */
static long
do_ulookup (fcookie *dir, const char *nam, fcookie *fc, UNIFILE **up)
{
	union { const char *cc; char *c; } nameptr;
	UNIFILE *u;
	long drvs;
	FILESYS *fs;
	fcookie *tmp;
	long changed;
	
	nameptr.cc = nam;

	TRACE (("uni_lookup(%s)", nam));

	if (dir->index != 0)
	{
		DEBUG (("uni_lookup: bad directory"));
		return ENOTDIR;
	}

	/* special case: an empty name in a directory means that directory
	 * so do "." and ".."
	 */
	if (!*nameptr.c ||
	     (nameptr.c[0] == '.' && nameptr.c[1] == '\0') ||
	     (nameptr.c[0] == '.' && nameptr.c[1] == '.' && nameptr.c[2] == '\0'))
	{
		dup_cookie (fc, dir);
		return E_OK;
	}

	drvs = sys_b_drvmap() | dosdrvs;

	/*
	 * OK, check the list of aliases and special directories
	 */
	for (u = u_root; u; u = u->next)
	{
		if (!strnicmp (nameptr.c, u->name, UNINAME_MAX))
		{
			if (S_ISDIR(u->mode))
			{
				struct cwd *cwd = get_curproc()->p_cwd;

				if (u->dev >= NUM_DRIVES)
				{
					fs = u->fs;
					if ( !fs )
						return ENOTDIR;

					if (up) *up = u;
					return xfs_root (fs, u->dev,fc);
				}
				if ((drvs & (1L << u->dev)) == 0)
				{
					return ENOTDIR;
				}
				tmp = &cwd->root[u->dev];
				changed = disk_changed (tmp->dev);
				if (changed || !tmp->fs)
				{
					/* drive changed? */
					if (!changed)
						changedrv (tmp->dev);

					tmp = &cwd->root[u->dev];
					if (!tmp->fs)
						return ENOTDIR;
				}
				dup_cookie (fc, tmp);
			}
			else
			{
				/* a symbolic link */
				fc->fs = &uni_filesys;
				fc->dev = UNIDRV;
				fc->index = (long) u;
			}
			if (up) *up = u;
			return E_OK;
		}
	}

	DEBUG (("uni_lookup: name (%s) not found", nam));
	return ENOENT;
}

static long _cdecl
uni_getxattr (fcookie *fc, XATTR *xattr)
{
	UNIFILE *u = (UNIFILE *) fc->index;

	if (fc->fs != &uni_filesys)
	{
		ALERT(MSG_unifs_wrong_getxattr);
		return EINTERNAL;
	}

	xattr->index = fc->index;
	xattr->dev = xattr->rdev = fc->dev;
	xattr->nlink = 1;
	xattr->blksize = 1;

	/* If "u" is null, then we have the root directory, otherwise
	 * we use the UNIFILE structure to get the info about it
	 */
	if (!u || S_ISDIR(u->mode))
	{
		xattr->uid = xattr->gid = 0;
		xattr->size = xattr->nblocks = 0;
		xattr->mode = S_IFDIR | DEFAULT_DIRMODE;
		xattr->attr = FA_DIR;
	}
	else
	{
		xattr->uid = u->dev;
		xattr->gid = 0;
		xattr->size = xattr->nblocks = strlen(u->data) + 1;
		xattr->mode = u->mode;
		xattr->attr = 0;
	}

	if (u)
	{
		xattr->mtime = xattr->atime = xattr->ctime = u->ctime;
		xattr->mdate = xattr->adate = xattr->cdate = u->cdate;
	}
	else
	{
		xattr->mtime = xattr->atime = xattr->ctime = timestamp;
		xattr->mdate = xattr->adate = xattr->cdate = datestamp;
	}

	return E_OK;
}

static long _cdecl
uni_rmdir (fcookie *dir, const char *name)
{
	long r;

	r = uni_remove (dir, name);
	if (r == ENOENT)
		r = ENOTDIR;

	return r;
}

static long _cdecl
uni_remove (fcookie *dir, const char *name)
{
	UNIFILE *u, *lastu;
	UNUSED (dir);

	DEBUG (("uni_remove: %s", name));

	lastu = NULL;
	u = u_root;
	while (u)
	{
		if (!strnicmp (u->name, name, UNINAME_MAX))
		{
			if (!S_ISLNK(u->mode))
				return ENOENT;

			if (!suser (get_curproc()->p_cred->ucr) && (u->dev != get_curproc()->p_cred->ucr->euid))
				return EACCES;

			kfree (u->data);

			if (lastu)
				lastu->next = u->next;
			else
				u_root = u->next;

			kfree (u);

			return E_OK;
		}

		lastu = u;
		u = u->next;
	}

	return ENOENT;
}

static long _cdecl
uni_getname (fcookie *root, fcookie *dir, char *pathname, int size)
{
	FILESYS *fs;
	UNIFILE *u;
	char *n;
	fcookie relto;
	char tmppath[PATH_MAX];
	long r;

	UNUSED(root);

	if (size <= 0)
		return EBADARG;

	fs = dir->fs;
	if (dir->dev == UNIDRV)
	{
		*pathname = 0;
		return E_OK;
	}

	for (u = u_root; u; u = u->next)
	{
		if (dir->dev == u->dev && S_ISDIR(u->mode))
		{
			*pathname++ = '\\';
			if (--size <= 0) return EBADARG;
			for (n = u->name; *n; )
			{
				*pathname++ = *n++;
				if (--size <= 0) return EBADARG;
			}
			break;
		}
	}

	if (!u)
	{
		ALERT(MSG_unifs_couldnt_match);
		return ENOTDIR;
	}

	if (dir->dev >= NUM_DRIVES)
	{
		if (xfs_root (fs, dir->dev, &relto) == E_OK)
		{
			if (!(fs->fsflags & FS_LONGPATH))
			{
				r = xfs_getname (fs, &relto, dir, tmppath, PATH_MAX);
				release_cookie (&relto);
				if (r)
				{
					return r;
				}
				if (strlen (tmppath) < size)
				{
					strcpy (pathname, tmppath);
					return E_OK;
				}
				else
				{
					return EBADARG;
				}
			}
			r = xfs_getname (fs, &relto, dir, pathname, size);
			release_cookie(&relto);
			return r;
		}
		else
		{
			*pathname = 0;
			return EINTERNAL;
		}
	}

	if (get_curproc()->p_cwd->root[dir->dev].fs != fs)
	{
		ALERT(MSG_unifs_fs_doesnt_match_dirs);
		return EINTERNAL;
	}

	if (!fs)
	{
		*pathname = 0;
		return E_OK;
	}
	if (!(fs->fsflags & FS_LONGPATH))
	{
		r = xfs_getname (fs, &get_curproc()->p_cwd->root[dir->dev], dir, tmppath, PATH_MAX);
		if (r) return r;
		if (strlen (tmppath) < size)
		{
			strcpy (pathname, tmppath);
			return E_OK;
		}
		else
		{
			return EBADARG;
		}
	}
	return xfs_getname (fs, &get_curproc()->p_cwd->root[dir->dev], dir, pathname, size);
}

static long _cdecl
uni_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	UNIFILE *u;
	fcookie fc;
	long r;

	UNUSED (olddir);

	for (u = u_root; u; u = u->next)
	{
		if (!strnicmp (u->name, oldname, UNINAME_MAX))
			break;
	}

	if (!u)
	{
		DEBUG(("uni_rename: old file not found"));
		return ENOENT;
	}

	/* the new name is not allowed to exist! */
	r = uni_lookup(newdir, newname, &fc);
	if (r == E_OK)
		release_cookie (&fc);

	if (r != ENOENT)
	{
		DEBUG (("uni_rename: error %ld", r));
		return (r == E_OK) ? EACCES : r;
	}

	strncpy (u->name, newname, UNINAME_MAX);

	return E_OK;
}

static long _cdecl
uni_opendir (DIR *dirh, int flags)
{
	UNUSED (flags);

	if (dirh->fc.index != 0)
	{
		DEBUG (("uni_opendir: bad directory"));
		return ENOTDIR;
	}

	dirh->index = 0;
	return E_OK;
}


static long _cdecl
uni_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	long map;
	char *dirname;
	int i;
	int giveindex = (dirh->flags == 0);
	UNIFILE *u;
	long index;
	long r;

	map = dosdrvs | sys_b_drvmap();

	i = dirh->index++;
	u = u_root;
	while (i > 0)
	{
		i--;
		u = u->next;
		if (!u)
			break;
	}

tryagain:
	if (!u) return ENMFILES;

	dirname = u->name;
	index = (long)u;
	if (S_ISDIR(u->mode))
	{
		/* make sure the drive really exists */
		if (u->dev >= NUM_DRIVES)
		{
			r = xfs_root (u->fs, u->dev,fc);
			if (r)
			{
				fc->fs = &uni_filesys;
				fc->index = 0;
				fc->dev = u->dev;
			}
		}
		else
		{
			if ((map & (1L << u->dev)) == 0)
			{
				dirh->index++;
				u = u->next;
				goto tryagain;
			}
			dup_cookie (fc, &get_curproc()->p_cwd->root[u->dev]);
			if (!fc->fs)
			{
				/* drive not yet initialized
				 * use default attributes
				 */
				fc->fs = &uni_filesys;
				fc->index = 0;
				fc->dev = u->dev;
			}
		}
	}
	else
	{
		/* a symbolic link */
		fc->fs = &uni_filesys;
		fc->dev = UNIDRV;
		fc->index = (long)u;
	}

	if (giveindex)
	{
		namelen -= sizeof (long);
		if (namelen <= 0)
		{
			release_cookie (fc);
			return EBADARG;
		}
		*((long * )name) = index;
		name += sizeof (long);
	}
	if (strlen (dirname) < namelen)
	{
		strcpy (name, dirname);
	}
	else
	{
		release_cookie (fc);
		return EBADARG;
	}

	return E_OK;
}

static long _cdecl
uni_pathconf (fcookie *dir, int which)
{
	UNUSED (dir);

	switch (which)
	{
		case DP_INQUIRE:	return DP_XATTRFIELDS;
		case DP_IOPEN:		return 0;		/* no files to open */
		case DP_MAXLINKS:	return 1;		/* no hard links available */
		case DP_PATHMAX:	return PATH_MAX;
		case DP_NAMEMAX:	return UNINAME_MAX;
		case DP_ATOMIC:		return 1;		/* no atomic writes */
		case DP_TRUNC:		return DP_AUTOTRUNC;
		case DP_CASE:		return DP_CASEINSENS;
		case DP_MODEATTR:	return DP_FT_DIR | DP_FT_LNK;
		case DP_XATTRFIELDS:	return DP_INDEX | DP_DEV | DP_NLINK | DP_SIZE;
	}

	return ENOSYS;
}

static long _cdecl
uni_dfree (fcookie *dir, long *buf)
{
	UNUSED (dir);

	buf[0] = 0;	/* number of free clusters */
	buf[1] = 0;	/* total number of clusters */
	buf[2] = 1;	/* sector size (bytes) */
	buf[3] = 1;	/* cluster size (sectors) */

	return E_OK;
}

static DEVDRV * _cdecl
uni_getdev (fcookie *fc, long *devsp)
{
	UNUSED (fc);

	if (fc->fs == &uni_filesys) return &uni_device;

	*devsp = EACCES;

	return NULL;
}

static long _cdecl
uni_symlink (fcookie *dir, const char *name, const char *to)
{
	UNIFILE *u;
	fcookie fc;
	long r;

	r = uni_lookup (dir, name, &fc);
	if (r == E_OK)
	{
		/* file already exists */
		release_cookie (&fc);
		return EACCES;
	}
	if (r != ENOENT)
	{
		/* some other error */
		return r;
	}

	if (get_curproc()->p_cred->ucr->egid)
	{
		/* only members of admin group may do that */
		return EACCES;
	}

	u = kmalloc (sizeof (*u));
	if (!u) return ENOMEM;

	strncpy (u->name, name, UNINAME_MAX);
	u->name[UNINAME_MAX] = '\0';

	u->data = kmalloc ((long) strlen (to) + 1);
	if (!u->data)
	{
		kfree (u);
		return ENOMEM;
	}

	strcpy (u->data, to);
	u->mode = S_IFLNK | DEFAULT_DIRMODE;
	u->dev = get_curproc()->p_cred->ucr->euid;
	u->next = u_root;
	u->fs = &uni_filesys;
	u->cdate = datestamp;
	u->ctime = timestamp;
	u_root = u;

	return E_OK;
}

static long _cdecl
uni_readlink (fcookie *fc, char *buf, int buflen)
{
	UNIFILE *u;

	u = (UNIFILE *) fc->index;

	assert (u);
	assert (S_ISLNK(u->mode));
	assert (u->data);

	if (strlen (u->data) < buflen)
		strcpy (buf, u->data);
	else
		return EBADARG;

	return E_OK;
}




/* uk: use these Dcntl's to install a new filesystem which is only visible
 *     on drive u:
 *
 *     FS_INSTALL:   let the kernel know about the file system; it does NOT
 *                   get a device number.
 *     FS_MOUNT:     use Dcntl(FS_MOUNT, "u:\\foo", &descr) to make a directory
 *                   foo where the filesytem resides in; the file system now
 *                   gets its device number which is also written into the
 *                   dev_no field of the fs_descr structure.
 *     FS_UNMOUNT:   remove a file system's directory; this call closes all
 *                   open files, directory searches and directories on this
 *                   device. Make sure that the FS will not recognise any
 *                   accesses to this device, as fs->root will be called
 *                   during the reinitalisation!
 *     FS_UNINSTALL: remove a file system completely from the kernel list,
 *                   but that will only be possible if there is no directory
 *                   associated with this file system.
 *                   This function allows it to write file systems as demons
 *                   which stay in memory only as long as needed.
 *
 * BUG: it is not possible yet to lock such a filesystem.
 */

/* here we start with gemdos only file system device numbers */
static short curr_dev_no = 0x100;

static long _cdecl
uni_fscntl(fcookie *dir, const char *name, int cmd, long arg)
{
	fcookie fc;
	long r;

	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			char *n = (char *)arg;
			strcpy (n/*(char *) arg*/, "unifs");
			return E_OK;
		}
		case FS_INSTALL: /* install a new filesystem */
		{
			struct fs_descr *d = (struct fs_descr *) arg;
			FILESYS *fs;

			if (!suser (get_curproc()->p_cred->ucr))
				return EPERM;

			/* check if FS is installed already */
			for (fs = active_fs; fs; fs = fs->next)
				if (d->file_system == fs)
					return 0L;

			/* include new file system into chain of file systems */
			xfs_add (d->file_system);

			/* return pointer to kernel info as OK */
			return (long) &kernelinfo;

		}
		case FS_MOUNT: /* install a new gemdos-only device for this FS */
		{
			struct fs_descr *d = (struct fs_descr *) arg;
			FILESYS *fs;
			UNIFILE *u;
			int drvmap_dev_no = -1;

			if (!suser (get_curproc()->p_cred->ucr))
				return EPERM;

			/* first check for existing names */
			r = uni_lookup (dir, name, &fc);
			if (r == E_OK)
			{
				release_cookie (&fc);
				return EACCES; /* name exists already */
			}
			if (r == ENOTDIR && name[0] && !name[1]) {
				/* one letter drive mapping check */
				drvmap_dev_no = DriveFromLetter(name[0]);

				if (drvmap_dev_no < 0)
					return r;

				/* check whether it is not already mounted */
				if (((sys_b_drvmap() | dosdrvs) & (1UL << drvmap_dev_no)) != 0)
					return EACCES; /* already exists */
			}
			else
				if (r != ENOENT) return r; /* some other error */

			if (!d) return EACCES;
			if (!d->file_system) return EACCES;

			/* check if FS is installed */
			for (fs = active_fs; fs; fs = fs->next)
				if (d->file_system == fs)
					break;

			if (!fs) return EACCES; /* not installed, so return an error */

			/* A..Z1..6 mount check */
			if ( drvmap_dev_no == -1 )
			{
				/* new mount point */
				u = kmalloc (sizeof (*u));
				if (!u) return ENOMEM;

				/* chain new entry into unifile list */
				u->next = u_root;
				u_root = u;

				strncpy (u->name, name, UNINAME_MAX);
				u->name[UNINAME_MAX] = 0;
			}
			else
			{
				/* A..Z1..6 mount */
				u = &u_drvs[drvmap_dev_no];
				/* update the _drvbits (the value of sys_b_drvmap() result) */
				*((long *) 0x4c2L) |= 1UL << drvmap_dev_no;
			}

			/* now get the file system its own device number */
			u->dev = d->dev_no = curr_dev_no++;
			u->mode = S_IFDIR|DEFAULT_DIRMODE;
			u->data = 0;
			u->fs = d->file_system;

			DEBUG (("uni_fscntl(FS_MOUNT, %s, drvmap_dev_no %d, dev_no %d)", name, drvmap_dev_no, u->dev));
			return (long) u->dev;
		}
		case FS_UNMOUNT: /* remove a file system's directory */
		{
			struct fs_descr *d = (struct fs_descr *) arg;
			FILESYS *fs;
			UNIFILE *u;

			if (!suser (get_curproc()->p_cred->ucr))
				return EPERM;

			/* first check that directory exists */
			/* use special uni_lookup mode to get the unifile entry */
			r = do_ulookup(dir, name, &fc, &u);
			if (r != E_OK)  return ENOENT; /* name does not exist */

			if (!d) return ENOENT;
			if (!d->file_system) return ENOENT;

			if (d->file_system != fc.fs)
				return ENOENT; /* not the right name! */

			release_cookie (&fc);

			if (!u || (u->fs != d->file_system))
				return ENOENT;

			/* check if FS is installed */
			for (fs = active_fs;  fs;  fs = fs->next)
				if (d->file_system == fs)
					break;

			if (!fs) return EACCES; /* not installed, so return an error */

			/* The FS_MOUNT can mount to e.g. u:\m. It uses the builtin
			 * mountpoint structure for that (u_drvs entry).
			 * If it is the case then the u->dev is >= UNI_NUM_DRVS (we
			 * use this to control the workflow).
			 */
			if (u >= &u_drvs[0] && u <= &u_drvs[sizeof(u_drvs)/sizeof(u_drvs[0]) - 1]) {
				/* cannot unmount the builtin drive */
				if (u->dev < UNI_NUM_DRVS || u->dev != d->dev_no) {
					/* this should never happen, only sanity check */
					DEBUG (("uni_remove: an attempt to remove builtin mountpoint '%s', dev_no = %d", name, u->dev));
					return ENOTDIR;
				}

				/* this mountpoint was made with FS_MOUNT g:\[A-Z1-6] */
				/* set the device back to the builtin number */
				u->dev = ((long)u - (long)&u_drvs[0])/sizeof(u_drvs[0]);
				u->fs = NULL;

				DEBUG (("uni_remove: removing mountpoint '%s', dev_no = %d", name, u->dev));

				/* update the _drvbits (the value of sys_b_drvmap() result) */
				*((long *) 0x4c2L) &= ~(1UL << u->dev);
				return E_OK;
			}

			/* here comes the difficult part: we have to close all files on that
			 * device, so we have to call changedrv(). The file system driver
			 * has to make sure that further calls to fs.root() with this device
			 * number will fail!
			 *
			 * Kludge: mark the directory as a link, so uni_remove will remove it.
			 */
			changedrv (u->dev);
			u->mode &= ~S_IFMT;
			u->mode |= S_IFLNK;
			return uni_remove (dir, name);
		}
		case FS_UNINSTALL: /* remove file system from kernel list */
		{
			struct fs_descr *d = (struct fs_descr *) arg;
			FILESYS *fs, *last_fs;
			UNIFILE *u;

			if (!suser (get_curproc()->p_cred->ucr))
				return EPERM;

			/* first check if there are any files or directories associated with
			 * this file system
			 */
			for (u = u_root;  u;  u = u->next)
				if (u->fs == d->file_system)
					/* we cannot remove it before unmount */
					return EACCES;
			last_fs = 0;
			fs = active_fs;

			/* go through the list and remove the file system */
			while (fs)
			{
				if (fs == d->file_system)
				{
					if (last_fs)
						last_fs->next = fs->next;
					else
						active_fs = fs->next;
					d->file_system->next = 0;
					return E_OK;
				}
				last_fs = fs;
				fs = fs->next;
			}
			return ENOENT;
		}
		case FS_INFO:
		{
			struct fs_info *info = (struct fs_info *) arg;
			if (info)
			{
				char *dst = info->type_asc;

				strcpy(info->name, "uni-xfs");
				info->version = (long) UNIFS_MAJOR << 16;
				info->version |= (long) UNIFS_MINOR;
				info->type = 0;			/* FIXME */
				strcpy(dst, "root");
			}
			return E_OK;
		}
		case VFAT_CNFDFLN:
			{
				FILESYS *fs;

				for (fs = active_fs; fs; fs = fs->next)
					if (fs == &fatfs_filesys)
					{
						fc.dev = 0;
						fc.fs = fs;
						fc.index = 0;
						fc.aux = 0;
						return fs->fscntl(&fc, NULL, cmd, arg);
					}
			}
			break;
		default:
		{
			/* see if we should just pass this along to another file system */
			r = uni_lookup (dir, name, &fc);
			if (r == E_OK)
			{
				if (fc.fs != &uni_filesys)
				{
					r = xfs_fscntl (fc.fs, &fc, ".", cmd, arg);
					release_cookie (&fc);
					return r;
				}
				else if (cmd == FUTIME)
				{
					UNIFILE *u = (UNIFILE *) fc.index;

					if (u)
					{
						u->ctime = timestamp;
						u->cdate = datestamp;
					}

					release_cookie (&fc);
					return E_OK;
				}

				release_cookie (&fc);
			}
		}
	}

	DEBUG (("uni_fscntl(%s, cmd %x, arg %lx) fail!", name, cmd, arg));
	return ENOSYS;
}


const char *fsname(FILESYS *fs)
{
	if (fs == &bios_filesys)
		return "dev";
	if (fs == &pipe_filesys)
		return "pipe";
	if (fs == &proc_filesys)
		return "proc";
#ifndef NO_RAMFS
	if (fs == &ramfs_filesys)
		return "ram";
#endif
	if (fs == &shm_filesys)
		return "shm";
#if WITH_KERNFS
	if (fs == &kern_filesys)
		return "kern";
#endif
	return "???";
}
