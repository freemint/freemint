/*
 * $Id$
 *
 * The Host OS filesystem access driver - device IO.
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


#include "hostfs_dev.h"
#include "hostfs_nfapi.h"

/* from ../natfeat/natfeat.c */
extern long __CDECL (*nf_call)(long id, ...);

long _cdecl hostfs_fs_dev_open     (FILEPTR *f);
long _cdecl hostfs_fs_dev_write    (FILEPTR *f, const char *buf, long bytes);
long _cdecl hostfs_fs_dev_read     (FILEPTR *f, char *buf, long bytes);
long _cdecl hostfs_fs_dev_lseek    (FILEPTR *f, long where, int whence);
long _cdecl hostfs_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf);
long _cdecl hostfs_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag);
long _cdecl hostfs_fs_dev_close    (FILEPTR *f, int pid);
long _cdecl hostfs_fs_dev_select   (FILEPTR *f, long proc, int mode);
void _cdecl hostfs_fs_dev_unselect (FILEPTR *f, long proc, int mode);



long _cdecl hostfs_fs_dev_open     (FILEPTR *f) {
	return nf_call(HOSTFS(DEV_OPEN), f);
}

long _cdecl hostfs_fs_dev_write    (FILEPTR *f, const char *buf, long bytes) {
	return nf_call(HOSTFS(DEV_WRITE), f, buf, bytes);
}

long _cdecl hostfs_fs_dev_read     (FILEPTR *f, char *buf, long bytes) {
	return nf_call(HOSTFS(DEV_READ), f, buf, bytes);
}

long _cdecl hostfs_fs_dev_lseek    (FILEPTR *f, long where, int whence) {
	return nf_call(HOSTFS(DEV_LSEEK), f, where, (long)whence);
}

long _cdecl hostfs_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf) {
	return nf_call(HOSTFS(DEV_IOCTL), f, (long)mode, buf);
}

long _cdecl hostfs_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag) {
	return nf_call(HOSTFS(DEV_DATIME), f, timeptr, (long)rwflag);
}

long _cdecl hostfs_fs_dev_close    (FILEPTR *f, int pid) {
	return nf_call(HOSTFS(DEV_CLOSE), f, (long)pid);
}

long _cdecl hostfs_fs_dev_select   (FILEPTR *f, long proc, int mode) {
	return nf_call(HOSTFS(DEV_SELECT), f, proc, (long)mode);
}

void _cdecl hostfs_fs_dev_unselect (FILEPTR *f, long proc, int mode) {
	nf_call(HOSTFS(DEV_UNSELECT), f, proc, (long)mode);
}



DEVDRV hostfs_fs_devdrv =
{
    hostfs_fs_dev_open, hostfs_fs_dev_write, hostfs_fs_dev_read, hostfs_fs_dev_lseek,
    hostfs_fs_dev_ioctl, hostfs_fs_dev_datime, hostfs_fs_dev_close, hostfs_fs_dev_select,
    hostfs_fs_dev_unselect,
    NULL, NULL /* writeb, readb not needed */
};

