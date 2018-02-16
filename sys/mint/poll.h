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
 * Started: 2001-03-01
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_poll_h
# define _mint_poll_h

# ifdef __KERNEL__
# include "ktypes.h"
# endif


struct pollfd
{
	long	fd;		/* File descriptor to poll */
	ushort	events;		/* Types of events poller cares about */
	ushort	revents;	/* Types of events that actually occurred */
};

/*
 * Testable events (may be specified in events field).
 */
# define POLLIN		0x001	/* There is data to read */
# define POLLPRI	0x002	/* There is urgent data to read */
# define POLLOUT	0x004	/* Writing now will not block */

# define POLLRDNORM	0x040	/* Normal data may be read */
# define POLLRDBAND	0x080	/* Priority data may be read */
# define POLLWRNORM	0x100	/* Writing now will not block */
# define POLLWRBAND	0x200	/* Priority data may be written */
# define POLLMSG	0x400

/*
 * Non-testable events (may not be specified in events field).
 */
# define POLLERR	0x0008
# define POLLHUP	0x0010
# define POLLNVAL	0x0020


# endif /* _mint_poll_h */
