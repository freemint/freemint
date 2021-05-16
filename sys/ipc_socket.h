/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 2001-01-13
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _ipc_socket_h
# define _ipc_socket_h

# include "mint/mint.h"
# include "mint/net.h"


/* new style pipe */

long _cdecl sys_pipe (short fds[2]);

/* socket system calls */

long _cdecl sys_socket (long domain, long type, long protocol);
long _cdecl sys_socketpair (long domain, long type, long protocol, short fds[2]);
long _cdecl sys_bind (short fd, struct sockaddr *addr, long addrlen);
long _cdecl sys_listen (short fd, long backlog);
long _cdecl sys_accept (short fd, struct sockaddr *addr, long *addrlen);
long _cdecl sys_connect (short fd, const struct sockaddr *addr, long addrlen);
long _cdecl sys_getsockname (short fd, struct sockaddr *addr, long *addrlen);
long _cdecl sys_getpeername (short fd, struct sockaddr *addr, long *addrlen);
long _cdecl sys_sendto (short fd, char *buf, long buflen, long flags, struct sockaddr *addr, long addrlen);
long _cdecl sys_sendmsg (short fd, struct msghdr *msg, long flags);
long _cdecl sys_recvfrom (short fd, char *buf, long buflen, long flags, struct sockaddr *addr, long *addrlen);
long _cdecl sys_recvmsg (short fd, struct msghdr *msg, long flags);
long _cdecl sys_setsockopt (short fd, long level, long optname, void *optval, long optlen);
long _cdecl sys_getsockopt (short fd, long level, long optname, void *optval, long *optlen);
long _cdecl sys_shutdown (short fd, long how);

/* internal socket functions */

long so_setsockopt (struct socket *so, long level, long optname, void *optval, long optlen);
long so_getsockopt (struct socket *so, long level, long optname, void *optval, long *optlen);
long so_shutdown (struct socket *so, long how);


# endif	/* _ipc_socket_h  */
