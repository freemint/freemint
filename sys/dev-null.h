/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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

# ifndef _dev_null_h
# define _dev_null_h

# include "mint/mint.h"
# include "mint/file.h"


extern DEVDRV null_device;
extern DEVDRV zero_device;

long	_cdecl null_open	(FILEPTR *f);
long	_cdecl null_write	(FILEPTR *f, const char *buf, long bytes);
long	_cdecl null_read	(FILEPTR *f, char *buf, long bytes);
long	_cdecl null_lseek	(FILEPTR *f, long where, int whence);
long	_cdecl null_ioctl	(FILEPTR *f, int mode, void *buf);
long	_cdecl null_datime	(FILEPTR *f, ushort *time, int rwflag);
long	_cdecl null_close	(FILEPTR *f, int pid);
long	_cdecl null_select	(FILEPTR *f, long p, int mode);
void	_cdecl null_unselect	(FILEPTR *f, long p, int mode);

long	_cdecl zero_read	(FILEPTR *f, char *buf, long bytes);
long	_cdecl zero_ioctl	(FILEPTR *f, int mode, void *buf);


# endif /* _dev_null_h */
