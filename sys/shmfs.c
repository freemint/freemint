/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1992,1993 Eric R. Smith.
 * Copyright 1993,1994 Atari Corporation.
 * All rights reserved.
 * 
 * 
 * Shared memory file system
 * 
 */

# include "shmfs.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/credentials.h"
# include "mint/dcntl.h"
# include "mint/emu_tos.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"
# include "mint/stat.h"

# include "dev-null.h"
# include "kerinfo.h"
# include "kmemory.h"
# include "memory.h"
# include "nullfs.h"
# include "time.h"

# include "proc.h"


static long	_cdecl shm_root		(int drv, fcookie *fc);
static long	_cdecl shm_creat	(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
static long	_cdecl shm_lookup	(fcookie *dir, const char *name, fcookie *fc);
static long	_cdecl shm_stat64	(fcookie *fc, STAT *ptr);
static long	_cdecl shm_chattr	(fcookie *fc, int attrib);
static long	_cdecl shm_chown	(fcookie *fc, int uid, int gid);
static long	_cdecl shm_chmode	(fcookie *fc, unsigned mode);
static long	_cdecl shm_remove	(fcookie *dir, const char *name);
static long	_cdecl shm_getname	(fcookie *root, fcookie *dir, char *pathname, int size);
static long	_cdecl shm_rename	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);
static long	_cdecl shm_readdir	(DIR *dirh, char *nm, int nmlen, fcookie *);
static long	_cdecl shm_pathconf	(fcookie *dir, int which);
static long	_cdecl shm_dfree	(fcookie *dir, long *buf);
static long	_cdecl shm_fscntl 	(fcookie *dir, const char *name, int cmd, long arg);
static DEVDRV *	_cdecl shm_getdev	(fcookie *fc, long *devsp);

static long	_cdecl shm_open		(FILEPTR *f);
static long	_cdecl shm_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl shm_read		(FILEPTR *f, char *buf, long bytes);
static long	_cdecl shm_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl shm_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl shm_datime	(FILEPTR *f, ushort *time, int flag);
static long	_cdecl shm_close	(FILEPTR *f, int pid);

SHMFILE *shmroot = NULL;
struct timeval shmfs_stmp;

void
shmfs_init (void)
{
	shmfs_stmp = xtime;
}


FILESYS shm_filesys =
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
	FS_LONGPATH		|
	FS_NO_C_CACHE		|
	FS_REENTRANT_L1		|
	FS_REENTRANT_L2		|
	FS_EXT_2		|
	FS_EXT_3		,
	
	shm_root,
	shm_lookup, shm_creat, shm_getdev, NULL,
	shm_chattr, shm_chown, shm_chmode,
	null_mkdir, null_rmdir, shm_remove, shm_getname, shm_rename,
	null_opendir, shm_readdir, null_rewinddir, null_closedir,
	shm_pathconf, shm_dfree, null_writelabel, null_readlabel,
	null_symlink, null_readlink, null_hardlink, shm_fscntl, null_dskchng,
	NULL, NULL,
	NULL,
	
	/* FS_EXT_1 */
	NULL, NULL,
	
	/* FS_EXT_2
	 */
	
	/* FS_EXT_3 */
	shm_stat64,
	
	0, 0, 0, 0, 0,
	NULL, NULL
};

DEVDRV shm_device =
{
	shm_open,
	shm_write, shm_read, shm_lseek,
	shm_ioctl, shm_datime,
	shm_close,
	null_select, null_unselect,
	NULL, NULL
};


long _cdecl 
shm_root (int drv, fcookie *fc)
{
	if ((unsigned) drv == SHM_DRV)
	{
		fc->fs = &shm_filesys;
		fc->dev = drv;
		fc->index = 0L;
		return E_OK;
	}
	
	fc->fs = 0;
	return EINTERNAL;
}

static long _cdecl 
shm_lookup (fcookie *dir, const char *name, fcookie *fc)
{
	SHMFILE *s;
	
	if (dir->index != 0)
	{
		DEBUG (("shm_lookup: bad directory"));
		return ENOTDIR;
	}
	
	/* special case: an empty name in a directory means that directory
	 * so does "."
	 */
	if (!*name || (name[0] == '.' && name[1] == 0))
	{
		*fc = *dir;
		return E_OK;
	}
	
	/* another special case: ".." could be a mount point
	 */
	if (!strcmp (name, ".."))
	{
		*fc = *dir;
		return EMOUNT;
	}
	
	for (s = shmroot; s; s = s->next)
	{
		if (!stricmp (s->filename, name))
			break;
	}
	
	if (!s)
	{
		DEBUG (("shm_lookup: name not found"));
		return ENOENT;
	}
	else
	{
		fc->index = (long) s;
		fc->fs = &shm_filesys;
		fc->dev = SHM_DRV;
	}
	
	return E_OK;
}

static long _cdecl
shm_stat64 (fcookie *fc, STAT *ptr)
{
	SHMFILE *s = (SHMFILE *) fc->index;
	
	mint_bzero (ptr, sizeof (*ptr));
	
	if (!s)
	{
		/* the root directory */
		
		ptr->dev = ptr->rdev = SHM_DRV;
		ptr->mode = S_IFDIR | DEFAULT_DIRMODE;
		ptr->nlink = 1;
		
		ptr->atime.high_time = 0;
		ptr->atime.time	= xtime.tv_sec;
		ptr->atime.nanoseconds = 0;
		
		ptr->mtime.high_time = 0;
		ptr->mtime.time	= shmfs_stmp.tv_sec;
		ptr->mtime.nanoseconds = 0;
		
		ptr->ctime.high_time = 0;
		ptr->ctime.time	= rootproc->started.tv_sec;
		ptr->ctime.nanoseconds = 0;
		
		ptr->blocks = 1;
		ptr->blksize = 1;
		
		return E_OK;
	}
	
	ptr->dev = SHM_DRV;
	ptr->ino = (long) s;
	ptr->mode = s->mode;
	ptr->uid = s->uid;
	ptr->gid = s->gid;
	ptr->rdev = PROC_RDEV_BASE | 0;
	
	ptr->atime.high_time = 0;
	ptr->atime.time	= xtime.tv_sec;
	ptr->atime.nanoseconds = 0;
	
	ptr->mtime.high_time = 0;
	ptr->mtime.time	= s->mtime.tv_sec;
	ptr->mtime.nanoseconds = 0;
	
	ptr->ctime.high_time = 0;
	ptr->ctime.time	= s->ctime.tv_sec;
	ptr->ctime.nanoseconds = 0;
	
	if (s->reg)
	{
		ptr->size = ptr->blocks = s->reg->len;
		ptr->nlink = s->reg->links + 1;
	}
	else
	{
		ptr->size = ptr->blocks = 0;
		ptr->nlink = 1;
	}
	
	ptr->blocks = 1;
	ptr->blksize = 1;
	
	return E_OK;
}

static long _cdecl 
shm_chattr (fcookie *fc, int attrib)
{
	SHMFILE *s = (SHMFILE *) fc->index;

	if (!s)
		return EACCES;
	
	if (attrib & FA_RDONLY)
	{
		s->mode &= ~(S_IWUSR|S_IWGRP|S_IWOTH);
	}
	else if (!(s->mode & (S_IWUSR|S_IWGRP|S_IWOTH)))
	{
		s->mode |= (S_IWUSR|S_IWGRP|S_IWOTH);
	}
	
	return E_OK;
}

static long _cdecl 
shm_chown (fcookie *fc, int uid, int gid)
{
	SHMFILE *s = (SHMFILE *) fc->index;

	if (!s)
		return EACCES;
	
	if (uid != -1) s->uid = uid;
	if (gid != -1) s->gid = gid;
	
	return E_OK;
}

static long _cdecl 
shm_chmode (fcookie *fc, unsigned int mode)
{
	SHMFILE *s = (SHMFILE *) fc->index;
	
	if (!s)
		return ENOSYS;
	
	s->mode = mode;
	
	return E_OK;
}

static long _cdecl 
shm_remove (fcookie *dir, const char *name)
{
	SHMFILE *s, **old;
	
	if (dir->index != 0)
		return ENOTDIR;
	
	old = &shmroot;
	for (s = shmroot; s; s = s->next)
	{
		if (!stricmp (s->filename, name))
			break;
		
		old = &s->next;
	}
	
	if (!s)
		return ENOENT;
	
	if (s->inuse)
		return EACCES;
	
	*old = s->next;
	
	s->reg->links--;
	if (s->reg->links <= 0)
		free_region (s->reg);
	
	kfree(s);
	shmfs_stmp = xtime;
	
	return E_OK;
}

static long _cdecl 
shm_getname (fcookie *root, fcookie *dir, char *pathname, int size)
{
	UNUSED (root); UNUSED (dir);
	
	/* BUG: 'size' should be used in a more meaningful way */
	if (size <= 0)
		return EBADARG;
	
	*pathname = 0;
	return E_OK;
}

static long _cdecl 
shm_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	SHMFILE *s;
	
	if (olddir->index != 0 || newdir->index != 0)
		return ENOTDIR;
	
	/* verify that "newname" doesn't exist */
	for (s = shmroot; s; s = s->next)
		if (!strnicmp (s->filename, newname, SHMNAME_MAX))
			return EACCES;
	
	for (s = shmroot; s; s = s->next)
		if (!strnicmp (s->filename, oldname, SHMNAME_MAX))
			break;
	
	if (!s)
		return ENOENT;
	
	strncpy (s->filename, newname, SHMNAME_MAX);
	return E_OK;
}

static long _cdecl 
shm_readdir (DIR *dirh, char *name, int namelen, fcookie *fc)
{
	int i;
	SHMFILE *s;
	
	s = shmroot;
	i = dirh->index++;
	while (i > 0 && s != 0)
	{
		s = s->next;
		--i;
	}
	
	if (!s)
		return ENMFILES;
	
	fc->index = (long) s;
	fc->fs = &shm_filesys;
	fc->dev = SHM_DRV;
	
	if (dirh->flags == 0)
	{
		namelen -= 4;
		if (namelen <= 0)
			return EBADARG;
		
		*(long *) name = (long) s;
		name += 4;
	}
	
	if (namelen <= strlen (s->filename))
		return EBADARG;
	
	strcpy (name, s->filename);
	return E_OK;
}

static long _cdecl 
shm_pathconf (fcookie *dir, int which)
{
	UNUSED (dir);
	
	switch (which)
	{
		case DP_INQUIRE:
			return DP_XATTRFIELDS;
		case DP_IOPEN:
			return UNLIMITED;	/* no internal limit on open files */
		case DP_MAXLINKS:
			return 1;		/* we don't have hard links */
		case DP_PATHMAX:
			return PATH_MAX;	/* max. path length */
		case DP_NAMEMAX:
			return SHMNAME_MAX;	/* max. length of individual name */
		case DP_ATOMIC:
			return UNLIMITED;	/* all writes are atomic */
		case DP_TRUNC:
			return DP_AUTOTRUNC;	/* file names are truncated */
		case DP_CASE:
			return DP_CASEINSENS;	/* case preserved, but ignored */
		case DP_MODEATTR:
			return (0777L << 8)|DP_FT_DIR|DP_FT_MEM;
		case DP_XATTRFIELDS:
			return DP_INDEX|DP_DEV|DP_NLINK|DP_UID|DP_GID|DP_BLKSIZE|DP_SIZE|
				DP_NBLOCKS|DP_MTIME;
		default:
			return ENOSYS;
	}
}

static long _cdecl 
shm_dfree (fcookie *dir, long *buf)
{
	long size;
	
	/* "sector" size is the size of the smallest amount of memory that can
	 * be allocated. see mem.h for the definition of ROUND
	 */
	long secsiz = ROUND (1);
	
	UNUSED (dir);
	
	size = tot_rsize (core, 0) + tot_rsize (alt, 0);
	*buf++ = size / secsiz;			/* number of free clusters */
	size = tot_rsize (core, 1) + tot_rsize(alt, 1);
	*buf++ = size / secsiz;			/* total number of clusters */
	*buf++ = secsiz;			/* sector size (bytes) */
	*buf = 1;				/* cluster size (in sectors) */
	
	return E_OK;
}

static long _cdecl
shm_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{	
	UNUSED (dir);
	UNUSED (name);
	
	switch (cmd)
	{
		case MX_KER_XFSNAME:
		{
			strcpy ((char *) arg, "shmfs");
			return E_OK;
		}
	}
	
	return ENOSYS;
}

static DEVDRV * _cdecl 
shm_getdev (fcookie *fc, long *devsp)
{
	SHMFILE *s = (SHMFILE *) fc->index;
	
	*devsp = (long) s;
	return &shm_device;
}

/*
 * create a shared memory region
 */

static long _cdecl 
shm_creat (fcookie *dir, const char *name, unsigned int mode, int attrib, fcookie *fc)
{
	SHMFILE *s;
	
	UNUSED(attrib);
	
	/* see if the name already exists
	 */
	for (s = shmroot; s; s = s->next)
	{
		if (!stricmp (s->filename, name))
		{
			DEBUG (("shm_creat: file exists"));
			return EACCES;
		}
	}
	
	s = kmalloc (sizeof (*s));
	if (!s)
		return ENOMEM;
	
	s->inuse = 0;
	strncpy (s->filename, name, SHMNAME_MAX);
	s->filename[SHMNAME_MAX] = 0;
	s->uid = get_curproc()->p_cred->ucr->euid;
	s->gid = get_curproc()->p_cred->ucr->egid;
	s->mode = mode;
	s->next = shmroot;
	s->reg = 0;
	s->mtime = s->ctime = xtime;
	shmroot = s;
	
	fc->fs = &shm_filesys;
	fc->index = (long)s;
	fc->dev = dir->dev;
	
	return E_OK;
}

/*
 * Shared memory device driver
 */

/*
 * BUG: file locking and the O_SHMODE restrictions are not implemented
 * for shared memory
 */

static long _cdecl 
shm_open (FILEPTR *f)
{
	SHMFILE *s = (SHMFILE *) f->devinfo;
	
	s->inuse++;
	
	return E_OK;
}

static long _cdecl 
shm_write(FILEPTR *f, const char *buf, long nbytes)
{
	SHMFILE *s = (SHMFILE *) f->devinfo;
	char *where;
	long bytes_written = 0;
	
	if (!s->reg)
		return 0;
	
	if (nbytes + f->pos32 > s->reg->len)
		nbytes = s->reg->len - f->pos32;
	
	where = (char *)s->reg->loc + f->pos32;
	
	/* BUG: memory read/writes should check for valid addresses */
	
	TRACE(("shm_write: %ld bytes to %p", nbytes, where));
	
	while (nbytes-- > 0)
	{
		*where++ = *buf++;
		bytes_written++;
	}
	
	f->pos32 += bytes_written;
	s->mtime = xtime;
	
	return bytes_written;
}

static long _cdecl 
shm_read (FILEPTR *f, char *buf, long nbytes)
{
	SHMFILE *s = (SHMFILE *) f->devinfo;
	char *where;
	long bytes_read = 0;
	
	if (!(s->reg))
		return 0;
	
	if (nbytes + f->pos32 > s->reg->len)
		nbytes = s->reg->len - f->pos32;
	
	where = (char *) s->reg->loc + f->pos32;
	
	TRACE (("shm_read: %ld bytes from %p", nbytes, where));
	
	while (nbytes-- > 0)
	{
		*buf++ = *where++;
		bytes_read++;
	}
	
	f->pos32 += bytes_read;
	return bytes_read;
}

/*
 * shm_ioctl: currently, the only IOCTL's available are:
 * SHMSETBLK:  set the address of the shared memory file. This
 *             call may only be made once per region, and then only
 *	       if the region is open for writing.
 * SHMGETBLK:  get the address of the shared memory region. This
 *             call fails (returns 0) if SHMSETBLK has not been
 *             called yet for this shared memory file.
 */

static long _cdecl 
shm_ioctl (FILEPTR *f, int mode, void *buf)
{
	SHMFILE *s = (SHMFILE *) f->devinfo;
	
	switch (mode)
	{
		case FIONREAD:
		case FIONWRITE:
		{
			long r;
			
			if (s->reg == 0)
			{
				r = 0;
			}
			else
			{
				r = s->reg->len - f->pos32;
				if (r < 0)
					r = 0;
			}
			
			*((long *) buf) = r;
			return E_OK;
		}
		case SHMSETBLK:
		{
			MEMREGION *m;
			
			if (s->reg)
			{
				DEBUG(("Fcntl: SHMSETBLK already performed for %s",
					s->filename));
				return EBADARG;
			}
			
			if ((f->flags & O_RWMODE) == O_RDONLY)
			{
				DEBUG(("Fcntl: SHMSETBLK: %s was opened read-only",
					s->filename));
				return EACCES;
			}
			
			/* find the memory region to be attached */
			m = addr2mem (get_curproc(), (long) buf);
			if (!m || !buf)
			{
				DEBUG(("Fcntl: SHMSETBLK: bad address %p", buf));
				return EFAULT;
			}
			
			if (m->shadow)
			{
				DEBUG(("Fcntl: SHMSETBLK: cannot share forked region"));
				return EACCES;
			}
			
			m->mflags |= M_SHARED;
			m->links++;
			s->reg = m;
			
			return E_OK;
		}
		case SHMGETBLK:
		{
			MEMREGION *m;
			
			if ((m = s->reg) == 0)
			{
				DEBUG(("Fcntl: no address for SHMGETBLK"));
				return E_OK;
			}
			
			/* check for memory limits */
			if (get_curproc()->maxmem)
			{
				if (m->len > get_curproc()->maxmem - memused (get_curproc()))
				{
					DEBUG(("Fcntl: SHMGETBLK would violate memory limits"));
					return E_OK;
				}
			}
			
			return (long) attach_region (get_curproc(), m);
		}
		
		default:
			DEBUG (("shmfs: bad Fcntl command"));
	}
	
	return ENOSYS;
}

static long _cdecl 
shm_lseek (FILEPTR *f, long where, int whence)
{
	SHMFILE *s = (SHMFILE *) f->devinfo;
	long newpos, maxpos;
	
	if (s->reg)
		maxpos = s->reg->len;
	else
		maxpos = 0;
	
	switch (whence)
	{
		case SEEK_SET:
			newpos = where;
			break;
		case SEEK_CUR:
			newpos = f->pos32 + where;
			break;
		case SEEK_END:
			newpos = maxpos + where;
			break;
		default:
			return ENOSYS;
	}
	
	if (newpos < 0 || newpos > maxpos)
		return EBADARG;
	
	f->pos32 = newpos;
	return newpos;
}

static long _cdecl 
shm_datime (FILEPTR *f, ushort *timeptr, int flag)
{
	SHMFILE *s = (SHMFILE *) f->devinfo;
	
	switch (flag)
	{
		case 0:
		{
			*(long *) timeptr = s->mtime.tv_sec;
			break;
		}
		case 1:
		{
			s->mtime.tv_sec = *(long *) timeptr;
			break;
		}
		default:
			return EBADARG;
	}
	
	return E_OK;
}

static long _cdecl 
shm_close (FILEPTR *f, int pid)
{
	UNUSED (pid);
	
	if (f->links <= 0)
	{
		SHMFILE *s = (SHMFILE *) f->devinfo;
		
		s->inuse--;
	}
	
	return E_OK;
}
