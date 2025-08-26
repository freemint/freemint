/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

# include "xfs_xdd.h"

# include "libkern/libkern.h"
# include "mint/emu_tos.h"
# include "mint/file.h"
# include "mint/stat.h"

# include "proc.h"
# include "time.h"


long
getxattr(FILESYS *fs, fcookie *fc, XATTR *xattr)
{
	STAT stat;
	long r;

	assert(fs->fsflags & FS_EXT_3);

	r = xfs_stat64(fs, fc, &stat);
	if (!r)
	{
		xattr->mode	= stat.mode;
		xattr->index	= stat.ino;
		xattr->dev	= stat.dev;
		xattr->rdev	= stat.rdev;
		xattr->nlink	= stat.nlink;
		xattr->uid	= stat.uid;
		xattr->gid	= stat.gid;
		xattr->size	= stat.size;
		xattr->blksize	= stat.blksize;
		xattr->nblocks	= (stat.blksize < 512) ? stat.blocks :
					stat.blocks / (stat.blksize >> 9);

		SET_XATTR_TD(xattr,m,stat.mtime.time);
		SET_XATTR_TD(xattr,a,stat.atime.time);
		SET_XATTR_TD(xattr,c,stat.ctime.time);
#if 0
		*((long *) &(xattr->mtime)) = stat.mtime.time;
		*((long *) &(xattr->atime)) = stat.atime.time;
		*((long *) &(xattr->ctime)) = stat.ctime.time;
#endif
		xattr->attr	= 0;

		/* fake attr field a little bit */
		if (S_ISDIR(stat.mode))
			xattr->attr = FA_DIR;
		else if (!(stat.mode & 0222))
			xattr->attr = FA_RDONLY;

		xattr->reserved2 = 0;
		xattr->reserved3[0] = 0;
		xattr->reserved3[1] = 0;
	}

	return r;
}

long
getstat64(FILESYS *fs, fcookie *fc, STAT *stat)
{
	XATTR xattr;
	long r;

	assert(fs->getxattr);

	r = xfs_getxattr(fs, fc, &xattr);
	if (!r)
	{
		stat->dev	= xattr.dev;
		stat->ino	= xattr.index;
		stat->mode	= xattr.mode;
		stat->nlink	= xattr.nlink;
		stat->uid	= xattr.uid;
		stat->gid	= xattr.gid;
		stat->rdev	= xattr.rdev;

		/* no native UTC extension
		 * -> convert to unix UTC
		 */
		stat->atime.high_time = 0;
		stat->atime.time = unixtime (xattr.atime, xattr.adate) + timezone;
		stat->atime.nanoseconds = 0;

		stat->mtime.high_time = 0;
		stat->mtime.time = unixtime (xattr.mtime, xattr.mdate) + timezone;
		stat->mtime.nanoseconds = 0;

		stat->ctime.high_time = 0;
		stat->ctime.time = unixtime (xattr.ctime, xattr.cdate) + timezone;
		stat->ctime.nanoseconds = 0;

		stat->size	= xattr.size;
		stat->blocks	= (xattr.blksize < 512) ? xattr.nblocks :
					xattr.nblocks * (xattr.blksize >> 9);
		stat->blksize	= xattr.blksize;

		stat->flags	= 0;
		stat->gen	= 0;

		mint_bzero(stat->res, sizeof(stat->res));
	}

	return r;
}


# ifdef NONBLOCKING_DMA

# ifdef DEBUG_INFO
# define DMA_DEBUG(x)	FORCE x
# else
# define DMA_DEBUG(x)
# endif

static void
xfs_block_level_0(FILESYS *fs, ushort dev, const char *func)
{
	while (fs->lock)
	{
		fs->sleepers++;
		DMA_DEBUG(("level 0: sleep on %lx, %c (%s, %i)", fs, DriveToLetter(dev), func, fs->sleepers));
		sleep(IO_Q, (long) fs);
		fs->sleepers--;
	}

	fs->lock = 1;
}

static void
xfs_deblock_level_0(FILESYS *fs, ushort dev, const char *func)
{
	fs->lock = 0;

	if (fs->sleepers)
	{
		DMA_DEBUG(("level 0: wake on %lx, %c (%s, %i)", fs, DriveToLetter(dev), func, fs->sleepers));
		wake(IO_Q, (long) fs);
	}
}

static void
xfs_block_level_1(FILESYS *fs, ushort dev, const char *func)
{
	unsigned long bit = 1UL << dev;

	while (fs->lock & bit)
	{
		fs->sleepers++;
		DMA_DEBUG(("level 1: sleep on %lx, %c (%s, %i)", fs, DriveToLetter(dev), func, fs->sleepers));
		sleep(IO_Q, (long) fs);
		fs->sleepers--;
	}

	fs->lock |= bit;
}

static void
xfs_deblock_level_1(FILESYS *fs, ushort dev, const char *func)
{
	fs->lock &= ~(1UL << dev);

	if (fs->sleepers)
	{
		DMA_DEBUG(("level 1: wake on %lx, %c (%s, %i)", fs, DriveToLetter(dev), func, fs->sleepers));
		wake(IO_Q, (long) fs);
	}
}

static volatile ushort xfs_sema_lock = 0;
static volatile ushort xfs_sleepers = 0;

void _cdecl
xfs_block(FILESYS *fs, ushort dev, const char *func)
{
	if (fs->fsflags & FS_EXT_2)
	{
		(*fs->block)(fs, dev, func);
	}
	else
	{
		while (xfs_sema_lock)
		{
			xfs_sleepers++;
			DMA_DEBUG(("[%c: -> %lx] sleep on xfs_sema_lock (%s, %i)", DriveToLetter(dev), fs, func, xfs_sleepers));
			sleep(IO_Q, (long) &xfs_sema_lock);
			xfs_sleepers--;
		}

		xfs_sema_lock = 1;
	}
}

void _cdecl
xfs_deblock(FILESYS *fs, ushort dev, const char *func)
{
	if (fs->fsflags & FS_EXT_2)
	{
		(*fs->deblock)(fs, dev, func);
	}
	else
	{
		xfs_sema_lock = 0;

		if (xfs_sleepers)
		{
			DMA_DEBUG(("[%c: -> %lx] wake on xfs_sema_lock (%s, %i)", DriveToLetter(dev), fs, func, xfs_sleepers));
			wake(IO_Q, (long) &xfs_sema_lock);
		}
	}
}

# ifdef DEBUG_INFO
# define xfs_lock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_block(fs, dev, func);	\
})
# define xfs_unlock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_deblock(fs, dev, func);	\
})
# else
# define xfs_lock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_block(fs, dev, NULL);	\
})
# define xfs_unlock(fs, dev, func)		\
({						\
	if (!(fs->fsflags & FS_REENTRANT_L2))	\
		xfs_deblock(fs, dev, NULL);	\
})
# endif /* DEBUG_INFO */

# else /* NONBLOCKING_DMA */

void _cdecl xfs_block(FILESYS *fs, ushort dev, const char *func){ return; }
void _cdecl xfs_deblock(FILESYS *fs, ushort dev, const char *func){ return; }
#define xfs_lock(fs, dev, func)
#define xfs_unlock(fs, dev, func)

# endif /* NONBLOCKING_DMA */


long _cdecl
xfs_root(FILESYS *fs, int drv, fcookie *fc)
{
	long r;
	
	xfs_lock(fs, drv, "xfs_root");
	r = (*fs->root)(drv, fc);
	xfs_unlock(fs, drv, "xfs_root");
	
	return r;
}

long _cdecl
xfs_lookup(FILESYS *fs, fcookie *dir, const char *name, fcookie *fc)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_lookup");
	r = (*fs->lookup)(dir, name, fc);
	xfs_unlock(fs, dir->dev, "xfs_lokup");
	
	return r;
}

DEVDRV * _cdecl
xfs_getdev(FILESYS *fs, fcookie *fc, long *devsp)
{
	return (*fs->getdev)(fc, devsp);
}

long _cdecl
xfs_getxattr(FILESYS *fs, fcookie *fc, XATTR *xattr)
{
	if (fs->getxattr)
	{
		long r;
		
		xfs_lock(fs, fc->dev, "xfs_getxattr");
		r = (*fs->getxattr)(fc, xattr);
		xfs_unlock(fs, fc->dev, "xfs_getxattr");
		
		return r;
	}
	
	return getxattr(fs, fc, xattr);
}

long _cdecl
xfs_chattr(FILESYS *fs, fcookie *fc, int attr)
{
	long r;
	
	xfs_lock(fs, fc->dev, "xfs_chattr");
	r = (*fs->chattr)(fc, attr);
	xfs_unlock(fs, fc->dev, "xfs_chatr");
	
	return r;
}
long _cdecl
xfs_chown(FILESYS *fs, fcookie *fc, int uid, int gid)
{
	long r;
	
	xfs_lock(fs, fc->dev, "xfs_chown");
	r = (*fs->chown)(fc, uid, gid);
	xfs_unlock(fs, fc->dev, "xfs_chown");
	
	return r;
}

long _cdecl
xfs_chmode(FILESYS *fs, fcookie *fc, unsigned mode)
{
	long r;
	
	xfs_lock(fs, fc->dev, "xfs_chmod");
	r = (*fs->chmode)(fc, mode);
	xfs_unlock(fs, fc->dev, "xfs_chmod");
	
	return r;
}

long _cdecl
xfs_mkdir(FILESYS *fs, fcookie *dir, const char *name, unsigned mode)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_mkdir");
	r = (*fs->mkdir)(dir, name, mode);
	xfs_unlock(fs, dir->dev, "xfs_mkdir");
	
	return r;
}
long _cdecl
xfs_rmdir(FILESYS *fs, fcookie *dir, const char *name)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_rmdir");
	r = (*fs->rmdir)(dir, name);
	xfs_unlock(fs, dir->dev, "xfs_rmdir");
	
	return r;
}
long _cdecl
xfs_creat(FILESYS *fs, fcookie *dir, const char *name, unsigned mode, int attr, fcookie *fc)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_creat");
	r = (*fs->creat)(dir, name, mode, attr, fc);
	xfs_unlock(fs, dir->dev, "xfs_creat");
	
	return r;
}
long _cdecl
xfs_remove(FILESYS *fs, fcookie *dir, const char *name)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_remove");
	r = (*fs->remove)(dir, name);
	xfs_unlock(fs, dir->dev, "xfs_remove");
	
	return r;
}
long _cdecl
xfs_getname(FILESYS *fs, fcookie *root, fcookie *dir, char *buf, int len)
{
	long r;
	
	xfs_lock(fs, root->dev, "xfs_getname");
	r = (*fs->getname)(root, dir, buf, len);
	xfs_unlock(fs, root->dev, "xfs_getname");
	
	return r;
}
long _cdecl
xfs_rename(FILESYS *fs, fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	long r;
	
	xfs_lock(fs, olddir->dev, "xfs_rename");
	r = (*fs->rename)(olddir, oldname, newdir, newname);
	xfs_unlock(fs, olddir->dev, "xfs_rename");
	
	return r;
}

long _cdecl
xfs_opendir(FILESYS *fs, DIR *dirh, int flags)
{
	long r;
	
	xfs_lock(fs, dirh->fc.dev, "xfs_opendir");
	r = (*fs->opendir)(dirh, flags);
	xfs_unlock(fs, dirh->fc.dev, "xfs_opendir");
	
	return r;
}
long _cdecl
xfs_readdir(FILESYS *fs, DIR *dirh, char *nm, int nmlen, fcookie *fc)
{
	long r;
	
	xfs_lock(fs, dirh->fc.dev, "xfs_readdir");
	r = (*fs->readdir)(dirh, nm, nmlen, fc);
	xfs_unlock(fs, dirh->fc.dev, "xfs_readdir");
	
	return r;
}
long _cdecl
xfs_rewinddir(FILESYS *fs, DIR *dirh)
{
	long r;
	
	xfs_lock(fs, dirh->fc.dev, "xfs_rewinddir");
	r = (*fs->rewinddir)(dirh);
	xfs_unlock(fs, dirh->fc.dev, "xfs_rwinddir");
	
	return r;
}
long _cdecl
xfs_closedir(FILESYS *fs, DIR *dirh)
{
	long r;
	
	xfs_lock(fs, dirh->fc.dev, "xfs_closedir");
	r = (*fs->closedir)(dirh);
	xfs_unlock(fs, dirh->fc.dev, "xfs_closedir");
	
	return r;
}

long _cdecl
xfs_pathconf(FILESYS *fs, fcookie *dir, int which)
{
	return (*fs->pathconf)(dir, which);
}
long _cdecl
xfs_dfree(FILESYS *fs, fcookie *dir, long *buf)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_dfree");
	r = (*fs->dfree)(dir, buf);
	xfs_unlock(fs, dir->dev, "xfs_dfree");
	
	return r;
}
long _cdecl
xfs_writelabel(FILESYS *fs, fcookie *dir, const char *name)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_writelabel");
	r = (*fs->writelabel)(dir, name);
	xfs_unlock(fs, dir->dev, "xfs_writelabel");
	
	return r;
}
long _cdecl
xfs_readlabel(FILESYS *fs, fcookie *dir, char *name, int namelen)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_readlabel");
	r = (*fs->readlabel)(dir, name, namelen);
	xfs_unlock(fs, dir->dev, "xfs_readlabel");
	
	return r;
}

long _cdecl
xfs_symlink(FILESYS *fs, fcookie *dir, const char *name, const char *to)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_symlink");
	r = (*fs->symlink)(dir, name , to);
	xfs_unlock(fs, dir->dev, "xfs_symlink");
	
	return r;
}
long _cdecl
xfs_readlink(FILESYS *fs, fcookie *fc, char *buf, int len)
{
	long r;
	
	xfs_lock(fs, fc->dev, "xfs_readlink");
	r = (*fs->readlink)(fc, buf, len);
	xfs_unlock(fs, fc->dev, "xfs_readlink");
	
	return r;
}
long _cdecl
xfs_hardlink(FILESYS *fs, fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	long r;
	
	xfs_lock(fs, fromdir->dev, "xfs_hardlink");
	r = (*fs->hardlink)(fromdir, fromname, todir, toname);
	xfs_unlock(fs, fromdir->dev, "xfs_hardlink");
	
	return r;
}
long _cdecl
xfs_fscntl(FILESYS *fs, fcookie *dir, const char *name, int cmd, long arg)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_fscntl");
	r = (*fs->fscntl)(dir, name, cmd, arg);
	xfs_unlock(fs, dir->dev, "xfs_fscntl");
	
	return r;
}
long _cdecl
xfs_dskchng(FILESYS *fs, int drv, int mode)
{
	long r;
	
	xfs_lock(fs, drv, "xfs_dskchng");
	r = (*fs->dskchng)(drv, mode);
	xfs_unlock(fs, drv, "xfs_dskchng");
	
	return r;
}

long _cdecl
xfs_release(FILESYS *fs, fcookie *fc)
{
	return (*fs->release)(fc);
}
long _cdecl
xfs_dupcookie(FILESYS *fs, fcookie *dst, fcookie *src)
{
	return (*fs->dupcookie)(dst, src);
}

long _cdecl
xfs_mknod(FILESYS *fs, fcookie *dir, const char *name, ulong mode)
{
	long r;
	
	xfs_lock(fs, dir->dev, "xfs_mknod");
	r = (*fs->mknod)(dir, name, mode);
	xfs_unlock(fs, dir->dev, "xfs_mknod");
	
	return r;
}
long _cdecl
xfs_unmount(FILESYS *fs, int drv)
{
	long r;
	
	xfs_lock(fs, drv, "xfs_unmount");
	r = (*fs->unmount)(drv);
	xfs_unlock(fs, drv, "xfs_unmount");
	
	return r;
}
long _cdecl
xfs_stat64(FILESYS *fs, fcookie *fc, STAT *stat)
{
	if (fs->fsflags & FS_EXT_3)
	{
		long r;
		
		xfs_lock(fs, fc->dev, "xfs_stat64");
		r = (*fs->stat64)(fc, stat);
		xfs_unlock(fs, fc->dev, "xfs_stat64");
		
		return r;
	}
	
	return getstat64(fs, fc, stat);
}


long _cdecl
xdd_open(FILEPTR *f)
{
	long r;
	
	xfs_lock(f->fc.fs, f->fc.dev, "xdd_open");
	r = (f->dev->open)(f);
	xfs_unlock(f->fc.fs, f->fc.dev, "xdd_open");
	
	return r;
}
long _cdecl
xdd_write(FILEPTR *f, const char *buf, long bytes)
{
	long r;
	long newpos, pos, end;

	xfs_lock(f->fc.fs, f->fc.dev, "xdd_write");
	pos = (f->dev->lseek)(f, 0, SEEK_CUR);
	end = (f->dev->lseek)(f, 0, SEEK_END);
	if (pos > end) {
		char zbuf[4096];
		long pad = pos - end;
		long done;
		long prev = end;
	
		memset(zbuf, 0, sizeof(zbuf));

		while (pad > 0) {
			done = (pad >= sizeof(zbuf) ? sizeof(zbuf) : pad);

			r = (f->dev->write)(f, zbuf, done);

			newpos = (f->dev->lseek)(f, 0, SEEK_CUR);
			if (newpos != prev + done) {
				(f->dev->lseek)(f, pos, SEEK_SET);
				xfs_unlock(f->fc.fs, f->fc.dev, "xdd_write");
				return EINVAL;
			}

			if (r != done) {
				(f->dev->lseek)(f, pos, SEEK_SET);
				xfs_unlock(f->fc.fs, f->fc.dev, "xdd_write");
				return EINVAL;
			}

			pad -= done;
			prev += done;
		}
	}
	newpos = (f->dev->lseek)(f, pos, SEEK_SET);
	if (newpos != pos) {
		/* ouch. */
		xfs_unlock(f->fc.fs, f->fc.dev, "xdd_write");
		return EINVAL;
	}

	r = (f->dev->write)(f, buf, bytes);
	xfs_unlock(f->fc.fs, f->fc.dev, "xdd_write");
	
	return r;
}
long _cdecl
xdd_read(FILEPTR *f, char *buf, long bytes)
{
	long r;
	
	xfs_lock(f->fc.fs, f->fc.dev, "xdd_read");
	r = (f->dev->read)(f, buf, bytes);
	xfs_unlock(f->fc.fs, f->fc.dev, "xdd_read");
	
	return r;
}
long _cdecl
xdd_lseek(FILEPTR *f, long where, int whence)
{
	long r;
	
	xfs_lock(f->fc.fs, f->fc.dev, "xdd_lseek");
	r = (f->dev->lseek)(f, where, whence);
	xfs_unlock(f->fc.fs, f->fc.dev, "xdd_lseek");
	
	return r;
}
long _cdecl
xdd_ioctl(FILEPTR *f, int mode, void *buf)
{
	long r;
	
	xfs_lock(f->fc.fs, f->fc.dev, "xdd_ioctl");
	r = (f->dev->ioctl)(f, mode, buf);
	xfs_unlock(f->fc.fs, f->fc.dev, "xdd_ioctl");
	
	return r;
}
long _cdecl
xdd_datime(FILEPTR *f, ushort *timeptr, int rwflag)
{
	long r;
	
	xfs_lock(f->fc.fs, f->fc.dev, "xdd_datime");
	r = (f->dev->datime)(f, timeptr, rwflag);
	xfs_unlock(f->fc.fs, f->fc.dev, "xdd_datime");
	
	return r;
}
long _cdecl
xdd_close(FILEPTR *f, int pid)
{
	long r;
	
	xfs_lock(f->fc.fs, f->fc.dev, "xdd_close");
	r = (f->dev->close)(f, pid);
	xfs_unlock(f->fc.fs, f->fc.dev, "xdd_close");
	
	return r;
}
