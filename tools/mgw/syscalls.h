/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1999 Jens Heitmann
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann, Jens Heitmann
 * Started: 1999-05
 * 
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _syscalls_h
# define _syscalls_h

/*
 * raw prototypes
 */

struct st_socket_param
{
	char *fnc;
	short domain;
	short type;
	short proto;
};

struct st_close_param
{
	char *fnc;
	short sfd;
};

struct st_seek_param
{
	char *fnc;
	short sfd;
	size_t offset;
	short whence;
};

struct st_fcntl_param
{
	char *fnc;
	short sfd;
	short cmd;
	long args;
};

struct st_read_param
{
	char *fnc;
	short sfd;
	void *buf;
	size_t len;
};

struct st_write_param
{
	char *fnc;
	short sfd;
	void *buf;
	size_t len;
};

struct st_recv_param
{
	char *fnc;
	short sfd;
	void *buf;
	size_t buflen;
	unsigned short flags;
};

struct st_recvfrom_param
{
	char *fnc;
	short sfd;
	void *buf;
	size_t buflen;
	unsigned short flags;
	struct sockaddr *addr;
	short *addrlen;
};

struct st_recvmsg_param
{
	char *fnc;
	short sfd;
	struct st_msghdr *msg;
	unsigned short flags;
};

struct st_send_param
{
	char *fnc;
	short sfd;
	void *buf;
	size_t buflen;
	unsigned short flags;
};

struct st_sendto_param
{
	char *fnc;
	short sfd;
	void *buf;
	size_t buflen;
	unsigned short flags;
	struct sockaddr *addr;
	short addrlen;
};

struct st_sendmsg_param
{
	char *fnc;
	short sfd;
	struct st_msghdr *msg;
	unsigned short flags;
};

struct st_accept_param
{
	char *fnc;
	short sfd;
	struct sockaddr *addr;
	short *addrlen;
};

struct st_bind_param
{
	char *fnc;
	short sfd;
	struct sockaddr *address;
	short addrlen;
};

struct st_connect_param
{
	char *fnc;
	short sfd;
	struct sockaddr *addr;
	short addrlen;
};

struct st_listen_param
{
	char *fnc;
	short sfd;
	short backlog;
};

struct st_shutdown_param
{
	char *fnc;
	short sfd;
	short how;
};

struct st_getpeername_param
{
	char *fnc;
	short sfd;
	struct sockaddr *addr;
	short *addrlen;
};

struct st_getsockname_param
{
	char *fnc;
	short sfd;
	struct sockaddr *addr;
	short *addrlen;
};

struct st_getsockopt_param
{
	char *fnc;
	short sfd;
	short level;
	short optname;
	char *optval;
	short *optlen;
};

struct st_setsockopt_param
{
	char *fnc;
	short sfd;
	short level;
	short optname;
	char *optval;
	short optlen;
};

struct st_select_param
{
	char *fnc;
	short nfds;
	st_fd_set *readfds;
	st_fd_set *writefds;
	st_fd_set *exceptfds;
	struct timeval *timeout;
};


/*
 * prototypes
 */
short	_cdecl st_socket	(struct st_socket_param p);
short	_cdecl st_close 	(struct st_close_param p);
short	_cdecl st_seek		(struct st_seek_param p);
short 	_cdecl st_fcntl 	(struct st_fcntl_param p);

size_t 	_cdecl st_read		(struct st_read_param p);
size_t 	_cdecl st_write 	(struct st_write_param p);

size_t 	_cdecl st_recv		(struct st_recv_param p);
size_t 	_cdecl st_recvfrom	(struct st_recvfrom_param p);
size_t 	_cdecl st_recvmsg 	(struct st_recvmsg_param p); 				 
size_t 	_cdecl st_send		(struct st_send_param p);
size_t 	_cdecl st_sendto	(struct st_sendto_param p);
size_t 	_cdecl st_sendmsg 	(struct st_sendmsg_param p);

short	_cdecl st_accept 	(struct st_accept_param p);
short	_cdecl st_bind		(struct st_bind_param p);
short	_cdecl st_connect 	(struct st_connect_param p);
short	_cdecl st_listen 	(struct st_listen_param p);
short	_cdecl st_shutdown	(struct st_shutdown_param p);

short	_cdecl st_getsockname 	(struct st_getsockname_param p);
short	_cdecl st_getpeername 	(struct st_getpeername_param p);

short	_cdecl st_getsockopt	(struct st_getsockopt_param p);
short	_cdecl st_setsockopt	(struct st_setsockopt_param p);

short	_cdecl st_select 	(struct st_select_param p); 								


# endif /* _syscalls_h */
