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
 * begin:	2000-06-28
 * last change:	2000-06-28
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _dummydev_h
# define _dummydev_h

# include "global.h"

# include <mint/file.h>


long	dummydev_init		(char *, struct dev_descr *);

long	dummydev_open		(FILEPTR *);
long	dummydev_write		(FILEPTR *, const char *, long);
long	dummydev_lseek		(FILEPTR *, long, int);
long	dummydev_ioctl		(FILEPTR *, int, void *);
long	dummydev_datime		(FILEPTR *, ushort *, int);
long	dummydev_close		(FILEPTR *, int);
long	dummydev_select		(FILEPTR *, long, int);
void	dummydev_unselect	(FILEPTR *, long, int);


# endif /* _dummydev_h */
