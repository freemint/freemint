/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 1999-07-27
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *  
 */

# ifndef _nullfs_h
# define _nullfs_h

# include "mint/mint.h"
# include "mint/file.h"


long _cdecl null_chattr		(fcookie *fc, int attrib);
long _cdecl null_chown		(fcookie *dir, int uid, int gid);
long _cdecl null_chmode		(fcookie *dir, unsigned int mode);
long _cdecl null_mkdir		(fcookie *dir, const char *name, unsigned mode);
long _cdecl null_rmdir		(fcookie *dir, const char *name);
long _cdecl null_creat		(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
long _cdecl null_remove		(fcookie *dir, const char *name);
long _cdecl null_rename		(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);
long _cdecl null_opendir	(DIR *dirh, int flags);
long _cdecl null_rewinddir	(DIR *dirh);
long _cdecl null_closedir	(DIR *dirh);
long _cdecl null_fscntl		(fcookie *dir, const char *name, int cmd, long arg);
long _cdecl null_symlink	(fcookie *dir, const char *name, const char *to);
long _cdecl null_readlink	(fcookie *dir, char *buf, int buflen);
long _cdecl null_hardlink	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
long _cdecl null_writelabel	(fcookie *dir, const char *name);
long _cdecl null_readlabel	(fcookie *dir, char *name, int namelen);
long _cdecl null_dskchng	(int drv, int mode);
long _cdecl null_mknod		(fcookie *dir, const char *name, ulong mode);
long _cdecl null_unmount	(int drv);


# endif /* _nullfs_h */
