/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
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
 * Started: 2001-01-12
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_iov_h
# define _mint_iov_h


/* Maximum number of elements in an io vector */
# define IOV_MAX	1024

struct iovec
{
	char	*iov_base;
	long	iov_len;
};

static inline long
iov_size (const struct iovec *iov, long n)
{
	register long size;

	if (n <= 0 || n > IOV_MAX)
		return -1;
	
	for (size = 0; n; ++iov, --n)
	{
		if (iov->iov_len < 0)
			return -1;
		
		size += iov->iov_len;
	}
	
	return size;
}


# endif /* _mint_iov_h */
