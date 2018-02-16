/*
 * The Host OS filesystem access driver - filesystem functions.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2002-2006 Standa of ARAnyM dev team.
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Originally taken from the STonX CVS repository.
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
 */

#include "global.h"

#include "hostfs_xfs.h"
#include "hostfs_nfapi.h"

#include "mint/arch/nf_ops.h"


unsigned long nf_hostfs_id = 0;


ulong    _cdecl fs_drive_bits(void)
{
	return nf_call(HOSTFS(GET_DRIVE_BITS));
}

long     _cdecl fs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
									  void *fs, void *fs_dev)
{
	return nf_call(HOSTFS(XFS_INIT), (long)fs_devnum, mountpoint, hostroot, (long)halfsensitive, fs, fs_dev);
}

/* FILESYS routines */

static
long     _cdecl hostfs_fs_root       (int drv, fcookie *fc)
{
	return nf_call(HOSTFS(XFS_ROOT), (long)drv, fc);
}

static
long     _cdecl hostfs_fs_lookup     (fcookie *dir, const char *name, fcookie *fc)
{
	return nf_call(HOSTFS(XFS_LOOKUP), dir, name, fc);
}

static
long     _cdecl hostfs_fs_creat      (fcookie *dir, const char *name,
								   unsigned int mode, int attrib, fcookie *fc)
{
	return nf_call(HOSTFS(XFS_CREATE), dir, name, (long)mode, (long)attrib, fc);
}

static
DEVDRV * _cdecl hostfs_fs_getdev     (fcookie *fc, long *devspecial)
{
	return (DEVDRV*) nf_call(HOSTFS(XFS_GETDEV), fc, devspecial);
}

static
long     _cdecl hostfs_fs_getxattr   (fcookie *file, XATTR *xattr)
{
	return nf_call(HOSTFS(XFS_GETXATTR), file, xattr);
}

static
long     _cdecl hostfs_fs_chattr     (fcookie *file, int attr)
{
	return nf_call(HOSTFS(XFS_CHATTR), file, (long)attr);
}

static
long     _cdecl hostfs_fs_chown      (fcookie *file, int uid, int gid)
{
	return nf_call(HOSTFS(XFS_CHOWN), file, (long)uid, (long)gid);
}

static
long     _cdecl hostfs_fs_chmode     (fcookie *file, unsigned int mode)
{
	return nf_call(HOSTFS(XFS_CHMOD), file, (long)mode);
}

static
long     _cdecl hostfs_fs_mkdir      (fcookie *dir, const char *name, unsigned int mode)
{
	return nf_call(HOSTFS(XFS_MKDIR), dir, name, (long)mode);
}

static
long     _cdecl hostfs_fs_rmdir      (fcookie *dir, const char *name)
{
	return nf_call(HOSTFS(XFS_RMDIR), dir, name);
}

static
long     _cdecl hostfs_fs_remove     (fcookie *dir, const char *name)
{
	return nf_call(HOSTFS(XFS_REMOVE), dir, name);
}

static
long     _cdecl hostfs_fs_getname    (fcookie *relto, fcookie *dir,
								   char *pathname, int size)
{
	return nf_call(HOSTFS(XFS_GETNAME), relto, dir, pathname, (long)size);
}

static
long     _cdecl hostfs_fs_rename     (fcookie *olddir, char *oldname,
								   fcookie *newdir, const char *newname)
{
	return nf_call(HOSTFS(XFS_RENAME), olddir, oldname, newdir, newname);
}

static
long     _cdecl hostfs_fs_opendir    (DIR *dirh, int tosflag)
{
	return nf_call(HOSTFS(XFS_OPENDIR), dirh, (long)tosflag);
}

static
long     _cdecl hostfs_fs_readdir    (DIR *dirh, char *name, int namelen,
								   fcookie *fc)
{
	return nf_call(HOSTFS(XFS_READDIR), dirh, name, (long)namelen, fc);
}

static
long     _cdecl hostfs_fs_rewinddir  (DIR *dirh)
{
	return nf_call(HOSTFS(XFS_REWINDDIR), dirh);
}

static
long     _cdecl hostfs_fs_closedir   (DIR *dirh)
{
	return nf_call(HOSTFS(XFS_CLOSEDIR), dirh);
}

static
long     _cdecl hostfs_fs_pathconf   (fcookie *dir, int which)
{
	return nf_call(HOSTFS(XFS_PATHCONF), dir, (long)which);
}

static
long     _cdecl hostfs_fs_dfree      (fcookie *dir, long *buf)
{
	return nf_call(HOSTFS(XFS_DFREE), dir, buf);
}

static
long     _cdecl hostfs_fs_writelabel (fcookie *dir, const char *name)
{
	return nf_call(HOSTFS(XFS_WRITELABEL), dir, name);
}

static
long     _cdecl hostfs_fs_readlabel  (fcookie *dir, char *name,
								   int namelen)
{
	return nf_call(HOSTFS(XFS_READLABEL), dir, name, (long)namelen);
}

static
long     _cdecl hostfs_fs_symlink    (fcookie *dir, const char *name,
								   const char *to)
{
	return nf_call(HOSTFS(XFS_SYMLINK), dir, name, to);
}

static
long     _cdecl hostfs_fs_readlink   (fcookie *dir, char *buf, int len)
{
	return nf_call(HOSTFS(XFS_READLINK), dir, buf, (long)len);
}

static
long     _cdecl hostfs_fs_hardlink   (fcookie *fromdir,
								   const char *fromname,
								   fcookie *todir, const char *toname)
{
	return nf_call(HOSTFS(XFS_HARDLINK), fromdir, fromname, todir, toname);
}

static
long     _cdecl hostfs_fs_fscntl     (fcookie *dir, const char *name,
								   int cmd, long arg)
{
	return nf_call(HOSTFS(XFS_FSCNTL), dir, name, (long)cmd, arg);
}

static
long     _cdecl hostfs_fs_dskchng    (int drv, int mode)
{
	return nf_call(HOSTFS(XFS_DSKCHNG), (long)drv, (long)mode);
}

static
long     _cdecl hostfs_fs_release    (fcookie *fc)
{
	return nf_call(HOSTFS(XFS_RELEASE), fc);
}

static
long     _cdecl hostfs_fs_dupcookie  (fcookie *new, fcookie *old)
{
	return nf_call(HOSTFS(XFS_DUPCOOKIE), new, old);
}

static
long     _cdecl hostfs_fs_sync       (void)
{
	return nf_call(HOSTFS(XFS_SYNC));
}

static
long     _cdecl hostfs_fs_mknod      (fcookie *dir, const char *name, ulong mode)
{
	return nf_call(HOSTFS(XFS_MKNOD), dir, name, mode);
}

static
long     _cdecl hostfs_fs_unmount    (int drv)
{
	return nf_call(HOSTFS(XFS_UNMOUNT), (long)drv);
}

static
long     _cdecl hostfs_fs_stat64     (fcookie *file, STAT *xattr)
{
	return nf_call(HOSTFS(XFS_STAT64), file, xattr);
}

/*
 * filesystem driver map
 */
FILESYS hostfs_filesys =
{
	(struct filesys *)0,    /* next */
	/*
	 * FS_KNOPARSE         kernel shouldn't do parsing
	 * FS_CASESENSITIVE    file names are case sensitive
	 * FS_NOXBIT           require only 'read' permission for execution
	 *                     (if a file can be read, it can be executed)
	 * FS_LONGPATH         file system understands "size" argument to "getname"
	 * FS_NO_C_CACHE       don't cache cookies for this filesystem
	 * FS_DO_SYNC          file system has a sync function
	 * FS_OWN_MEDIACHANGE  filesystem control self media change (dskchng)
	 * FS_REENTRANT_L1     fs is level 1 reentrant
	 * FS_REENTRANT_L2     fs is level 2 reentrant
	 * FS_EXT_1            extensions level 1 - mknod & unmount
	 * FS_EXT_2            extensions level 2 - additional place at the end
	 * FS_EXT_3            extensions level 3 - stat & native UTC timestamps
	 */
	FS_NOXBIT        |
	FS_CASESENSITIVE |
	/* FS_DO_SYNC       |  not used on the host side (would be
	 *                     called periodically -> commented out) */
	FS_OWN_MEDIACHANGE  |
	FS_LONGPATH      |
	FS_REENTRANT_L1  |
	FS_REENTRANT_L2  |
	FS_EXT_1         |
	FS_EXT_2         |
	FS_EXT_3         ,
	hostfs_fs_root, hostfs_fs_lookup, hostfs_fs_creat, hostfs_fs_getdev, hostfs_fs_getxattr,
	hostfs_fs_chattr, hostfs_fs_chown, hostfs_fs_chmode, hostfs_fs_mkdir, hostfs_fs_rmdir,
	hostfs_fs_remove, hostfs_fs_getname, hostfs_fs_rename, hostfs_fs_opendir,
	hostfs_fs_readdir, hostfs_fs_rewinddir, hostfs_fs_closedir, hostfs_fs_pathconf,
	hostfs_fs_dfree, hostfs_fs_writelabel, hostfs_fs_readlabel, hostfs_fs_symlink,
	hostfs_fs_readlink, hostfs_fs_hardlink, hostfs_fs_fscntl, hostfs_fs_dskchng,
	hostfs_fs_release, hostfs_fs_dupcookie,
	hostfs_fs_sync,
	/* FS_EXT_1 */
	hostfs_fs_mknod, hostfs_fs_unmount,
	/* FS_EXT_2 */
	/* FS_EXT_3 */
	hostfs_fs_stat64,
	0L, 0L, 0L,         /* reserved 1,2,3 */
	0L, 0L,             /* lock, sleepers */
	0L, 0L              /* block(), deblock() */
};


FILESYS *hostfs_init(void)
{
#if __KERNEL__ == 1
	boot_print (MSG_BOOT);
	boot_print (MSG_GREET);
	boot_print ("\r\n");
#endif
	
	if ( !KERNEL->nf_ops ) {
		c_conws("Native Features not present on this system\r\n");
		return NULL;
	}

	/* get the HostFs NatFeat ID */
	nf_hostfs_id = KERNEL->nf_ops->get_id("HOSTFS");
	if (nf_hostfs_id == 0) {
		c_conws(MSG_PFAILURE("hostfs",
					"\r\nThe HOSTFS NatFeat not found\r\n"));
		return NULL;
	}

	nf_call = KERNEL->nf_ops->call;

	/* compare the version */
	if (nf_call(HOSTFS(GET_VERSION)) != HOSTFS_NFAPI_VERSION) {
		c_conws(MSG_PFAILURE("hostfs",
					"\r\nHOSTFS NFAPI version mismatch\n\r"));
		return NULL;
	}

	return &hostfs_filesys;
}
