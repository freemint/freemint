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

# include "dev-null.h"

# include "libkern/libkern.h"
# include "mint/ioctl.h"
# include "time.h"


DEVDRV null_device =
{
	open:		null_open,
	write:		null_write,
	read:		null_read,
	lseek:		null_lseek,
	ioctl:		null_ioctl,
	datime:		null_datime,
	close:		null_close,
	select:		null_select,
	unselect:	null_unselect,
	writeb:		NULL,
	readb:		NULL
};

DEVDRV zero_device =
{
	open:		null_open,
	write:		null_write,
	read:		zero_read,
	lseek:		null_lseek,
	ioctl:		zero_ioctl,
	datime:		null_datime,
	close:		null_close,
	select:		null_select,
	unselect:	null_unselect,
	writeb:		NULL,
	readb:		NULL
};


/* Driver routines for /dev/null
 */

long _cdecl
null_open (FILEPTR *f)
{
	UNUSED (f);
	
	return E_OK;
}

long _cdecl
null_write (FILEPTR *f, const char *buf, long bytes)
{
	UNUSED (f);
	UNUSED (buf);
	
	return bytes;
}

long _cdecl
null_read (FILEPTR *f, char *buf, long bytes)
{
	UNUSED (f);
	UNUSED (buf);
	UNUSED (bytes);
	
	return 0;
}

long _cdecl
null_lseek (FILEPTR *f, long where, int whence)
{
	UNUSED (f);
	UNUSED (whence);
	
	return (where == 0) ? 0 : EBADARG;
}

long _cdecl
null_ioctl (FILEPTR *f, int mode, void *buf)
{
	UNUSED (f);
	
	switch	(mode)
	{
		case FIONREAD:
			*((long *) buf) = 0;
			break;
		case FIONWRITE:
			*((long *) buf) = 1;
			break;
		case FIOEXCEPT:
			*((long *) buf) = 0;
		default:
			return ENOSYS;
	}
	
	return E_OK;
}

long _cdecl
null_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	UNUSED (f);
	
	if (rwflag)
		return EACCES;
	
	*timeptr++ = timestamp;
	*timeptr = datestamp;
	
	return E_OK;
}

long _cdecl
null_close (FILEPTR *f, int pid)
{
	UNUSED (f);
	UNUSED (pid);
	
	return E_OK;
}

long _cdecl
null_select (FILEPTR *f, long p, int mode)
{
	UNUSED (f);
	UNUSED (p);
	
	if ((mode == O_RDONLY) || (mode == O_WRONLY))
	{
		/* we're always ready to read/write */
		return 1;
	}
	
	/* other things we don't care about */
	return E_OK;
}

void _cdecl
null_unselect (FILEPTR *f, long p, int mode)
{
	UNUSED (f); UNUSED (p); UNUSED (mode);
	/* nothing to do */
}


/* Driver routines for /dev/zero
 */

long _cdecl
zero_read (FILEPTR *f, char *buf, long bytes)
{
	UNUSED (f);
	
	mint_bzero (buf, bytes);
	return bytes;
}

long _cdecl
zero_ioctl (FILEPTR *f, int mode, void *buf)
{
	UNUSED (f);

	switch	(mode)
	{
		case FIONREAD:
			*((long *) buf) = 1;
			break;
		case FIONWRITE:
			*((long *) buf) = 0;
			break;
		case FIOEXCEPT:
			*((long *) buf) = 0;
		default:
			return ENOSYS;
	}
	
	return E_OK;
}
