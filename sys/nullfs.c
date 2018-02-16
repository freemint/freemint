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
 * 
 * changes since last version:
 * 
 * 1999-07-27:
 * 
 * initial version; moved from biosfs.c
 * some cleanup
 * 
 * known bugs:
 * 
 * todo:
 * 
 * optimizations to do:
 * 
 */

# include "nullfs.h"


long _cdecl
null_chattr (fcookie *fc, int attrib)
{
	UNUSED (fc);
	UNUSED (attrib);
	
	return EACCES;
}

long _cdecl 
null_chown (fcookie *dir, int uid, int gid)
{
	UNUSED (dir);
	UNUSED (uid);
	UNUSED (gid);
	
	return ENOSYS;
}

long _cdecl 
null_chmode (fcookie *dir, unsigned int mode)
{
	UNUSED (dir);
	UNUSED (mode);
	
	return ENOSYS;
}

long _cdecl
null_mkdir (fcookie *dir, const char *name, unsigned int mode)
{
	UNUSED (dir);
	UNUSED (name);
	UNUSED (mode);
	
	return EACCES;
}

long _cdecl 
null_rmdir (fcookie *dir, const char *name)
{
	UNUSED (dir);
	UNUSED (name);
	
	/* the kernel already checked to see if the file exists */
	return EACCES;
}

long _cdecl
null_creat (fcookie *dir, const char *name, unsigned int mode, int attrib, fcookie *fc)
{
	UNUSED (dir);
	UNUSED (fc);
	UNUSED (name);
	UNUSED (mode);
	UNUSED (attrib);
	
	return EACCES;
}

long _cdecl 
null_remove (fcookie *dir, const char *name)
{
	UNUSED (dir);
	UNUSED (name);
	
	/* the kernel already checked to see if the file exists */
	return EACCES;
}

long _cdecl 
null_rename (fcookie *olddir, char *oldname, fcookie *newdir, const char *newname)
{
	UNUSED (olddir);
	UNUSED (oldname);
	UNUSED (newdir);
	UNUSED (newname);
	
	return EACCES;
}

long _cdecl 
null_opendir (DIR *dirh, int flags)
{
	UNUSED (flags);
	
	dirh->index = 0;
	return E_OK;
}

long _cdecl 
null_rewinddir (DIR *dirh)
{
	dirh->index = 0;
	return E_OK;
}

long _cdecl 
null_closedir (DIR *dirh)
{
	UNUSED (dirh);
	return E_OK;
}

long _cdecl
null_fscntl (fcookie *dir, const char *name, int cmd, long arg)
{
	UNUSED (dir);
	UNUSED (name);
	UNUSED (cmd);
	UNUSED (arg);
	
	return ENOSYS;
}

long _cdecl
null_symlink (fcookie *dir, const char *name, const char *to)
{
	UNUSED (dir);
	UNUSED (name);
	UNUSED (to);
	
	return ENOSYS;
}

long _cdecl
null_readlink (fcookie *dir, char *buf, int buflen)
{
	UNUSED (dir);
	UNUSED (buf);
	UNUSED (buflen);
	
	return ENOSYS;
}

long _cdecl
null_hardlink (fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname)
{
	UNUSED (fromdir);
	UNUSED (todir);
	UNUSED (fromname);
	UNUSED (toname);
	
	return ENOSYS;
}

long _cdecl
null_writelabel (fcookie *dir, const char *name)
{
	UNUSED (dir);
	UNUSED (name);
	
	return EACCES;
}

long _cdecl
null_readlabel (fcookie *dir, char *name, int namelen)
{
	UNUSED (dir);
	UNUSED (name);
	UNUSED (namelen);
	
	return ENOENT;
}

long _cdecl
null_dskchng (int drv, int mode)
{
	UNUSED (drv);
	UNUSED (mode);
	
	return E_OK;
}

long _cdecl
null_mknod (fcookie *dir, const char *name, ulong mode)
{
	UNUSED (dir);
	UNUSED (name);
	UNUSED (mode);
	
	return EACCES;
}

long _cdecl 
null_unmount (int drv)
{
	UNUSED (drv);
	
	return EINVAL;
}
