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

# ifndef _sockutil_h
# define _sockutil_h

# include "global.h"

# include "net.h"


/* create a new socket */
struct socket *	so_create (void);

/* release socket to unconnecting state */
long		so_release (struct socket *so);

/* make so1 and so2 a connected pair of sockets */
void		so_sockpair (struct socket *so1, struct socket *so2);

/* Put `client' on the queue of incomplete connections of `server'.
 * Blocks until the connection is accepted or it's impossible to
 * establish the connection, unless nonblock != 0.
 * `Backlog' is the number of pending connections allowed for `server'.
 * so_connect() will fail if there are already `backlog' clients on the
 * server queue.
 */
long		so_connect (struct socket *server, struct socket *client,
			   short backlog, short nonblock, short wakeup);

/* Take the first waiting client from `server's incomplete connection
 * queue and connect `newsock' to it.
 * Blocks until a connection request is available, unless nonblock != 0.
 */
long		so_accept (struct socket *server, struct socket *newsock,
			  short nonblock);

/* register/unregister domains */
void		so_register (short dom, struct dom_ops *ops);
void		so_unregister (short dom);

/* add seleting proc's to the socket */
long		so_rselect (struct socket *s, long proc);
long		so_wselect (struct socket *s, long proc);
long		so_xselect (struct socket *s, long proc);

/* wake selecting proc's */
void		so_wakersel (struct socket *s);
void		so_wakewsel (struct socket *s);
void		so_wakexsel (struct socket *s);


# endif /* _sockutil_h */
