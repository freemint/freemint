/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _filesys_h
# define _filesys_h

# include "mint/mint.h"
# include "mint/file.h"


# define DIRSEP(p)	(((p) == '\\') || ((p) == '/'))
# define MAX_LINKS	4
# define follow_links	((char *) -1L)

/* external variables
 */

extern FILESYS *active_fs;
extern FILESYS *drives[NUM_DRIVES];
extern int aliasdrv[NUM_DRIVES];
extern long dosdrvs;


/* exported functions
 */

void xfs_block (FILESYS *fs, ushort dev, const char *func);
void xfs_deblock (FILESYS *fs, ushort dev, const char *func);

long getxattr (FILESYS *fs, fcookie *fc, XATTR *xattr);

void kill_cache (fcookie *dir, char *name);
long cache_lookup (fcookie *dir, char *name, fcookie *res);
void cache_init (void);
void clobber_cookie (fcookie *fc);
void init_filesys (void);
char *xfs_name (fcookie *fc);
void xfs_add (FILESYS *fs);
void load_modules (long type);
void close_filesys (void);
long _s_ync (void);
long _cdecl s_ync (void);
long _cdecl sys_fsync (short fh);
void _cdecl changedrv (ushort drv);
long disk_changed (ushort drv);
long relpath2cookie (fcookie *dir, const char *path, char *lastnm, fcookie *res, int depth);
long path2cookie (const char *path, char *lastname, fcookie *res);
void release_cookie (fcookie *fc);
void dup_cookie (fcookie *new, fcookie *old);
FILEPTR *new_fileptr (void);
void dispose_fileptr (FILEPTR *f);
int _cdecl denyshare (FILEPTR *list, FILEPTR *newfileptr);
int denyaccess (XATTR *, ushort);
LOCK * _cdecl denylock (LOCK *list, LOCK *newlock);
long dir_access (fcookie *, ushort, ushort *);
int has_wild (const char *name);
void copy8_3 (char *dest, const char *src);
int pat_match (const char *name, const char *template);
int samefile (fcookie *, fcookie *);


# ifndef NONBLOCKING_DMA
# define xfs_lock(fs, dev, func)
# define xfs_unlock(fs, dev, func)
# else
# ifdef DEBUG_INFO
# define xfs_lock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_block (fs, dev, func);	\
})
# define xfs_unlock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_deblock (fs, dev, func);	\
})
# else
# define xfs_lock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_block (fs, dev, NULL);	\
})
# define xfs_unlock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_deblock (fs, dev, NULL);	\
})
# endif /* DEBUG_INFO */
# endif

INLINE long
xfs_root (FILESYS *fs, int drv, fcookie *fc)
{
	register long r;
	
	xfs_lock (fs, drv, "xfs_root");
	r = (*fs->root)(drv, fc);
	xfs_unlock (fs, drv, "xfs_root");
	
	return r;
}

INLINE long
xfs_lookup (FILESYS *fs, fcookie *dir, const char *name, fcookie *fc)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_lookup");
	r = (*fs->lookup)(dir, name, fc);
	xfs_unlock (fs, dir->dev, "xfs_lokup");
	
	return r;
}

INLINE DEVDRV *
xfs_getdev (FILESYS *fs, fcookie *fc, long *devsp)
{
	return (*fs->getdev)(fc, devsp);
}

INLINE long
xfs_getxattr (FILESYS *fs, fcookie *fc, XATTR *xattr)
{
	if (fs->getxattr)
	{
		register long r;
		
		xfs_lock (fs, fc->dev, "xfs_getxattr");
		r = (*fs->getxattr)(fc, xattr);
		xfs_unlock (fs, fc->dev, "xfs_getxattr");
		
		return r;
	}
	
	return getxattr (fs, fc, xattr);
}

INLINE long
xfs_chattr (FILESYS *fs, fcookie *fc, int attr)
{
	register long r;
	
	xfs_lock (fs, fc->dev, "xfs_chattr");
	r = (*fs->chattr)(fc, attr);
	xfs_unlock (fs, fc->dev, "xfs_chatr");
	
	return r;
}
INLINE long
xfs_chown (FILESYS *fs, fcookie *fc, int uid, int gid)
{
	register long r;
	
	xfs_lock (fs, fc->dev, "xfs_chown");
	r = (*fs->chown)(fc, uid, gid);
	xfs_unlock (fs, fc->dev, "xfs_chown");
	
	return r;
}
INLINE long
xfs_chmode (FILESYS *fs, fcookie *fc, unsigned mode)
{
	register long r;
	
	xfs_lock (fs, fc->dev, "xfs_chmod");
	r = (*fs->chmode)(fc, mode);
	xfs_unlock (fs, fc->dev, "xfs_chmod");
	
	return r;
}

INLINE long
xfs_mkdir (FILESYS *fs, fcookie *dir, const char *name, unsigned mode)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_mkdir");
	r = (*fs->mkdir)(dir, name, mode);
	xfs_unlock (fs, dir->dev, "xfs_mkdir");
	
	return r;
}
INLINE long
xfs_rmdir (FILESYS *fs, fcookie *dir, const char *name)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_rmdir");
	r = (*fs->rmdir)(dir, name);
	xfs_unlock (fs, dir->dev, "xfs_rmdir");
	
	return r;
}
INLINE long
xfs_creat (FILESYS *fs, fcookie *dir, const char *name, unsigned mode, int attr, fcookie *fc)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_creat");
	r = (*fs->creat)(dir, name, mode, attr, fc);
	xfs_unlock (fs, dir->dev, "xfs_creat");
	
	return r;
}
INLINE long
xfs_remove (FILESYS *fs, fcookie *dir, const char *name)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_remove");
	r = (*fs->remove)(dir, name);
	xfs_unlock (fs, dir->dev, "xfs_remove");
	
	return r;
}
INLINE long
xfs_getname (FILESYS *fs, fcookie *root, fcookie *dir, char *buf, int len)
{
	register long r;
	
	xfs_lock (fs, root->dev, "xfs_getname");
	r = (*fs->getname)(root, dir, buf, len);
	xfs_unlock (fs, root->dev, "xfs_getname");
	
	return r;
}
INLINE long
xfs_rename (FILESYS *fs, fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	register long r;
	
	xfs_lock (fs, olddir->dev, "xfs_rename");
	r = (*fs->rename)(olddir, oldname, newdir, newname);
	xfs_unlock (fs, olddir->dev, "xfs_rename");
	
	return r;
}

INLINE long
xfs_opendir (FILESYS *fs, DIR *dirh, int flags)
{
	register long r;
	
	xfs_lock (fs, dirh->fc.dev, "xfs_opendir");
	r = (*fs->opendir)(dirh, flags);
	xfs_unlock (fs, dirh->fc.dev, "xfs_opendir");
	
	return r;
}
INLINE long
xfs_readdir (FILESYS *fs, DIR *dirh, char *nm, int nmlen, fcookie *fc)
{
	register long r;
	
	xfs_lock (fs, dirh->fc.dev, "xfs_readdir");
	r = (*fs->readdir)(dirh, nm, nmlen, fc);
	xfs_unlock (fs, dirh->fc.dev, "xfs_readdir");
	
	return r;
}
INLINE long
xfs_rewinddir (FILESYS *fs, DIR *dirh)
{
	register long r;
	
	xfs_lock (fs, dirh->fc.dev, "xfs_rewinddir");
	r = (*fs->rewinddir)(dirh);
	xfs_unlock (fs, dirh->fc.dev, "xfs_rwinddir");
	
	return r;
}
INLINE long
xfs_closedir (FILESYS *fs, DIR *dirh)
{
	register long r;
	
	xfs_lock (fs, dirh->fc.dev, "xfs_closedir");
	r = (*fs->closedir)(dirh);
	xfs_unlock (fs, dirh->fc.dev, "xfs_closedir");
	
	return r;
}

INLINE long
xfs_pathconf (FILESYS *fs, fcookie *dir, int which)
{
	return (*fs->pathconf)(dir, which);
}
INLINE long
xfs_dfree (FILESYS *fs, fcookie *dir, long *buf)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_dfree");
	r = (*fs->dfree)(dir, buf);
	xfs_unlock (fs, dir->dev, "xfs_dfree");
	
	return r;
}
INLINE long
xfs_writelabel (FILESYS *fs, fcookie *dir, const char *name)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_writelabel");
	r = (*fs->writelabel)(dir, name);
	xfs_unlock (fs, dir->dev, "xfs_writelabel");
	
	return r;
}
INLINE long
xfs_readlabel (FILESYS *fs, fcookie *dir, char *name, int namelen)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_readlabel");
	r = (*fs->readlabel)(dir, name, namelen);
	xfs_unlock (fs, dir->dev, "xfs_readlabel");
	
	return r;
}

INLINE long
xfs_symlink (FILESYS *fs, fcookie *dir, const char *name, const char *to)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_symlink");
	r = (*fs->symlink)(dir, name , to);
	xfs_unlock (fs, dir->dev, "xfs_symlink");
	
	return r;
}
INLINE long
xfs_readlink (FILESYS *fs, fcookie *fc, char *buf, int len)
{
	register long r;
	
	xfs_lock (fs, fc->dev, "xfs_readlink");
	r = (*fs->readlink)(fc, buf, len);
	xfs_unlock (fs, fc->dev, "xfs_readlink");
	
	return r;
}
INLINE long
xfs_hardlink (FILESYS *fs, fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	register long r;
	
	xfs_lock (fs, fromdir->dev, "xfs_hardlink");
	r = (*fs->hardlink)(fromdir, fromname, todir, toname);
	xfs_unlock (fs, fromdir->dev, "xfs_hardlink");
	
	return r;
}
INLINE long
xfs_fscntl (FILESYS *fs, fcookie *dir, const char *name, int cmd, long arg)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_fscntl");
	r = (*fs->fscntl)(dir, name, cmd, arg);
	xfs_unlock (fs, dir->dev, "xfs_fscntl");
	
	return r;
}
INLINE long
xfs_dskchng (FILESYS *fs, int drv, int mode)
{
	register long r;
	
	xfs_lock (fs, drv, "xfs_dskchng");
	r = (*fs->dskchng)(drv, mode);
	xfs_unlock (fs, drv, "xfs_dskchng");
	
	return r;
}

INLINE long
xfs_release (FILESYS *fs, fcookie *fc)
{
	return (*fs->release)(fc);
}
INLINE long
xfs_dupcookie (FILESYS *fs, fcookie *dst, fcookie *src)
{
	return (*fs->dupcookie)(dst, src);
}

INLINE long
xfs_mknod (FILESYS *fs, fcookie *dir, const char *name, ulong mode)
{
	register long r;
	
	xfs_lock (fs, dir->dev, "xfs_mknod");
	r = (*fs->mknod)(dir, name, mode);
	xfs_unlock (fs, dir->dev, "xfs_mknod");
	
	return r;
}
INLINE long
xfs_unmount (FILESYS *fs, int drv)
{
	register long r;
	
	xfs_lock (fs, drv, "xfs_unmount");
	r = (*fs->unmount)(drv);
	xfs_unlock (fs, drv, "xfs_unmount");
	
	return r;
}
INLINE long
xfs_stat64 (FILESYS *fs, fcookie *fc, STAT *stat)
{
	register long r;
	
	xfs_lock (fs, fc->dev, "xfs_stat64");
	r = (*fs->stat64)(fc, stat);
	xfs_unlock (fs, fc->dev, "xfs_stat64");
	
	return r;
}


INLINE long
xdd_open (FILEPTR *f)
{
	register long r;
	
	xfs_lock (f->fc.fs, f->fc.dev, "xdd_open");
	r = (f->dev->open)(f);
	xfs_unlock (f->fc.fs, f->fc.dev, "xdd_open");
	
	return r;
}
INLINE long
xdd_write (FILEPTR *f, const char *buf, long bytes)
{
	register long r;
	
	xfs_lock (f->fc.fs, f->fc.dev, "xdd_write");
	r = (f->dev->write)(f, buf, bytes);
	xfs_unlock (f->fc.fs, f->fc.dev, "xdd_write");
	
	return r;
}
INLINE long
xdd_read (FILEPTR *f, char *buf, long bytes)
{
	register long r;
	
	xfs_lock (f->fc.fs, f->fc.dev, "xdd_read");
	r = (f->dev->read)(f, buf, bytes);
	xfs_unlock (f->fc.fs, f->fc.dev, "xdd_read");
	
	return r;
}
INLINE long
xdd_lseek (FILEPTR *f, long where, int whence)
{
	register long r;
	
	xfs_lock (f->fc.fs, f->fc.dev, "xdd_lseek");
	r = (f->dev->lseek)(f, where, whence);
	xfs_unlock (f->fc.fs, f->fc.dev, "xdd_lseek");
	
	return r;
}
INLINE long
xdd_ioctl (FILEPTR *f, int mode, void *buf)
{
	register long r;
	
	xfs_lock (f->fc.fs, f->fc.dev, "xdd_ioctl");
	r = (f->dev->ioctl)(f, mode, buf);
	xfs_unlock (f->fc.fs, f->fc.dev, "xdd_ioctl");
	
	return r;
}
INLINE long
xdd_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	register long r;
	
	xfs_lock (f->fc.fs, f->fc.dev, "xdd_datime");
	r = (f->dev->datime)(f, timeptr, rwflag);
	xfs_unlock (f->fc.fs, f->fc.dev, "xdd_datime");
	
	return r;
}
INLINE long
xdd_close (FILEPTR *f, int pid)
{
	register long r;
	
	xfs_lock (f->fc.fs, f->fc.dev, "xdd_close");
	r = (f->dev->close)(f, pid);
	xfs_unlock (f->fc.fs, f->fc.dev, "xdd_close");
	
	return r;
}


# endif /* _filesys_h */
