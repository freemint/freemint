/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
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
 * Started:      2000-05-02
 * 
 * Changes:
 * 
 * 2000-05-02:
 * 
 * - inital version
 * 
 */

# ifndef _io_h
# define _io_h

# include "mytypes.h"


int	io_init		(void);

int	io_open		(int64_t dev);
int	io_close	(int handle);
int	io_ioctrl	(int handle, int mode, void *buf);
int32_t	io_read		(int handle, void *buf, int32_t size);
int32_t	io_write	(int handle, void *buf, int32_t size);
int64_t	io_seek		(int handle, int whence, int64_t where);


# endif /* _io_h */
