/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
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

# ifndef _xfs_xdd_h
# define _xfs_xdd_h

# include "mint/mint.h"


long getxattr (FILESYS *fs, fcookie *fc, XATTR *xattr);
long getstat64 (FILESYS *fs, fcookie *fc, STAT *ptr);


void _cdecl xfs_block (FILESYS *fs, ushort dev, const char *func);
void _cdecl xfs_deblock (FILESYS *fs, ushort dev, const char *func);


long _cdecl xfs_root(FILESYS *fs, int drv, fcookie *fc);
long _cdecl xfs_lookup(FILESYS *fs, fcookie *dir, const char *name, fcookie *fc);

DEVDRV * _cdecl xfs_getdev(FILESYS *fs, fcookie *fc, long *devsp);

long _cdecl xfs_getxattr(FILESYS *fs, fcookie *fc, XATTR *xattr);

long _cdecl xfs_chattr(FILESYS *fs, fcookie *fc, int attr);
long _cdecl xfs_chown(FILESYS *fs, fcookie *fc, int uid, int gid);
long _cdecl xfs_chmode(FILESYS *fs, fcookie *fc, unsigned mode);

long _cdecl xfs_mkdir(FILESYS *fs, fcookie *dir, const char *name, unsigned mode);
long _cdecl xfs_rmdir(FILESYS *fs, fcookie *dir, const char *name);
long _cdecl xfs_creat(FILESYS *fs, fcookie *dir, const char *name, unsigned mode, int attr, fcookie *fc);
long _cdecl xfs_remove(FILESYS *fs, fcookie *dir, const char *name);
long _cdecl xfs_getname(FILESYS *fs, fcookie *root, fcookie *dir, char *buf, int len);
long _cdecl xfs_rename(FILESYS *fs, fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

long _cdecl xfs_opendir(FILESYS *fs, DIR *dirh, int flags);
long _cdecl xfs_readdir(FILESYS *fs, DIR *dirh, char *nm, int nmlen, fcookie *fc);
long _cdecl xfs_rewinddir(FILESYS *fs, DIR *dirh);
long _cdecl xfs_closedir(FILESYS *fs, DIR *dirh);

long _cdecl xfs_pathconf(FILESYS *fs, fcookie *dir, int which);
long _cdecl xfs_dfree(FILESYS *fs, fcookie *dir, long *buf);
long _cdecl xfs_writelabel(FILESYS *fs, fcookie *dir, const char *name);
long _cdecl xfs_readlabel(FILESYS *fs, fcookie *dir, char *name, int namelen);

long _cdecl xfs_symlink(FILESYS *fs, fcookie *dir, const char *name, const char *to);
long _cdecl xfs_readlink(FILESYS *fs, fcookie *fc, char *buf, int len);
long _cdecl xfs_hardlink(FILESYS *fs, fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
long _cdecl xfs_fscntl(FILESYS *fs, fcookie *dir, const char *name, int cmd, long arg);
long _cdecl xfs_dskchng(FILESYS *fs, int drv, int mode);

long _cdecl xfs_release(FILESYS *fs, fcookie *fc);
long _cdecl xfs_dupcookie(FILESYS *fs, fcookie *dst, fcookie *src);

long _cdecl xfs_mknod(FILESYS *fs, fcookie *dir, const char *name, ulong mode);
long _cdecl xfs_unmount(FILESYS *fs, int drv);
long _cdecl xfs_stat64(FILESYS *fs, fcookie *fc, STAT *stat);


long _cdecl xdd_open(FILEPTR *f);
long _cdecl xdd_write(FILEPTR *f, const char *buf, long bytes);
long _cdecl xdd_read(FILEPTR *f, char *buf, long bytes);
long _cdecl xdd_lseek(FILEPTR *f, long where, int whence);
long _cdecl xdd_ioctl(FILEPTR *f, int mode, void *buf);
long _cdecl xdd_datime(FILEPTR *f, ushort *timeptr, int rwflag);
long _cdecl xdd_close(FILEPTR *f, int pid);

# endif /* _xfs_xdd_h */
