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
 * begin:	2001-01-13
 * last change:	2001-01-13
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _ipc_socket_h
# define _ipc_socket_h

# include "mint/mint.h"
# include "mint/net.h"


/* socket system calls */

long _cdecl sys_socket (short domain, enum so_type type, short protocol);
long _cdecl sys_socketpair (short domain, enum so_type type, short protocol, short fds[2]);
long _cdecl sys_bind (short fd, struct sockaddr *addr, short addrlen);
long _cdecl sys_listen (short fd, short backlog);
long _cdecl sys_accept (short fdp, struct sockaddr *addr, short *addrlen);
long _cdecl sys_connect (short fd, struct sockaddr *addr, short addrlen);
long _cdecl sys_getsockname (short fd, struct sockaddr *addr, short *addrlen);
long _cdecl sys_getpeername (short fd, struct sockaddr *addr, short *addrlen);
long _cdecl sys_send (short fd, char *buf, long buflen, short flags);
long _cdecl sys_sendto (short fd, char *buf, long buflen, short flags, struct sockaddr *addr, short addrlen);
long _cdecl sys_sendmsg (short fd, struct msghdr *msg, short flags);
long _cdecl sys_recv (short fd, char *buf, long buflen, short flags);
long _cdecl sys_recvfrom (short fd, char *buf, long buflen, short flags, struct sockaddr *addr, short *addrlen);
long _cdecl sys_recvmsg (short fd, struct msghdr *msg, short flags);
long _cdecl sys_setsockopt (short fd, short level, short optname, void *optval, long optlen);
long _cdecl sys_getsockopt (short fd, short level, short optname, void *optval, long *optlen);
long _cdecl sys_shutdown (short fd, short how);


# endif	/* _ipc_socket_h  */
