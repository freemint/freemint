/*
 * The Host OS filesystem access driver - NF API definitions.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2003-2006 Petr Stehlik of ARAnyM dev team.
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

#ifndef _HOSTFS_NFAPI_H
#define _HOSTFS_NFAPI_H

/*
 * General versioning rules:
 * The driver version is literaly HOSTFS_XFS_VERSION.HOSTFS_NFAPI_VERSION
 *
 * note: therefore you need to be careful and set the NFAPI_VERSION like
 * 0x or something if needed.
 */

/*
 * general XFS driver version
 */
#define HOSTFS_XFS_VERSION       0
#undef BETA

/* if you change anything in the enum {} below you have to increase
   this HOSTFS_NFAPI_VERSION!
*/
#define HOSTFS_NFAPI_VERSION    04

enum {
	GET_VERSION = 0,	/* subID = 0 */
	GET_DRIVE_BITS,        /* get mapped drive bits */
	/* hostfs_xfs */
	XFS_INIT, XFS_ROOT, XFS_LOOKUP, XFS_CREATE, XFS_GETDEV, XFS_GETXATTR,
	XFS_CHATTR, XFS_CHOWN, XFS_CHMOD, XFS_MKDIR, XFS_RMDIR, XFS_REMOVE,
	XFS_GETNAME, XFS_RENAME, XFS_OPENDIR, XFS_READDIR, XFS_REWINDDIR,
	XFS_CLOSEDIR, XFS_PATHCONF, XFS_DFREE, XFS_WRITELABEL, XFS_READLABEL,
	XFS_SYMLINK, XFS_READLINK, XFS_HARDLINK, XFS_FSCNTL, XFS_DSKCHNG,
	XFS_RELEASE, XFS_DUPCOOKIE, XFS_SYNC, XFS_MKNOD, XFS_UNMOUNT,
	/* hostfs_dev */
	DEV_OPEN, DEV_WRITE, DEV_READ, DEV_LSEEK, DEV_IOCTL, DEV_DATIME,
	DEV_CLOSE, DEV_SELECT, DEV_UNSELECT,
	/* new from 0.04 */
	XFS_STAT64
};

extern unsigned long nf_hostfs_id;
extern long __CDECL (*nf_call)(long id, ...);

#define HOSTFS(a)	(nf_hostfs_id + a)

#endif /* _HOSTFS_NFAPI_H */
