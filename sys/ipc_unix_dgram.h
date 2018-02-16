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
 * Started: 2000-06-28
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _ipc_unix_dgram_h
# define _ipc_unix_dgram_h

# include "mint/mint.h"
# include "mint/net.h"


long	unix_dgram_socketpair	(struct socket *, struct socket *);
long	unix_dgram_connect	(struct socket *, struct sockaddr *, short, short);
long	unix_dgram_send		(struct socket *, const struct iovec *, short, short, short, struct sockaddr *, short);
long	unix_dgram_recv		(struct socket *, const struct iovec *, short, short, short, struct sockaddr *, short *);
long	unix_dgram_select	(struct socket *, short, long);
long	unix_dgram_ioctl	(struct socket *, short, void *);
long	unix_dgram_getname	(struct socket *, struct sockaddr *, short *, short);
					

# endif /* _ipc_unix_dgram_h */
