/*
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
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _ksocket_h
# define _ksocket_h

# define __KERNEL_XDD__

# include <libkern/libkern.h>


# define UIO_MAXIOV	1024

/* Structure for scatter/gather I/O */
struct iovec
{
	void *iov_base;	/* Pointer to data */
	long iov_len;	/* Length of data */
};

/* Socket types */
# define SOCK_STREAM	1
# define SOCK_DGRAM	2
# define SOCK_RAW	3
# define SOCK_RDM	4
# define SOCK_SEQPACKET	5

/* Protocol families */
# define PF_UNSPEC	0
# define PF_UNIX	1
# define PF_INET	2
# define PF_APPLETALK	5

/* Address families, same as above */
# define AF_UNSPEC	PF_UNSPEC
# define AF_UNIX	PF_UNIX
# define AF_INET	PF_INET
# define AF_APPLETALK	PF_APPLETALK
# define AF_LINK	200

/* Flags for send/recv */
# define MSG_OOB	0x0001
# define MSG_PEEK	0x0002
# define MSG_DONTROUTE	0x0004

/* Levels for use with [s|g]etsockopt call */
# define SOL_SOCKET	0xffff

/* Options for use with [s|g]etsockopt call */
# define SO_DEBUG	1
# define SO_REUSEADDR	2
# define SO_TYPE	3
# define SO_ERROR	4
# define SO_DONTROUTE	5
# define SO_BROADCAST	6
# define SO_SNDBUF	7
# define SO_RCVBUF	8
# define SO_KEEPALIVE	9
# define SO_OOBINLINE	10
# define SO_LINGER	11
# define SO_CHKSUM	40
# define SO_DROPCONN	41

/* Structure used for SO_LINGER */
struct linger
{
	long	l_onoff;
	long	l_linger;
};

/* Generic socket address */
struct sockaddr
{
	short	sa_family;
	char	sa_data[14];
};

/* Structure describing a message used with sendmsg/recvmsg */
struct msghdr
{
	void		*msg_name;
	long		msg_namelen;
	struct iovec	*msg_iov;
	long		msg_iovlen;
	void		*msg_accrights;
	long		msg_accrightslen;
};

long	socket		(int, int, int);
long	socketpair	(int, int, int, int[2]);
long	bind		(int, struct sockaddr *, long);
long	connect		(int, struct sockaddr *, long);
long	accept		(int, struct sockaddr *, long *); 
long	listen		(int, int);
long	getsockname	(int, struct sockaddr *, long *);
long	getpeername	(int, struct sockaddr *, long *);
long	send		(int, const void *, long, int);
long	recv		(int, void *, long, int);
long	sendto		(int, const void *, long, int, const struct sockaddr *, int);
long	recvfrom	(int, void *, long, int, struct sockaddr *, int *);
long	sendmsg		(int, const struct msghdr *, int);
long	recvmsg		(int, struct msghdr *, int);
long	getsockopt	(int, int, int, void *, long *);
long	setsockopt	(int, int, int, void *, long);
long	shutdown	(int, int);


# define SOCKDEV	"u:\\dev\\socket"
# define SOCKETCALL	(('S' << 8) | 100)

/* socket call command names, passed in the `cmd' field of *_cmd structs
 */

enum so_cmd
{
	SOCKET_CMD = 0,
	SOCKETPAIR_CMD,
	BIND_CMD,
	LISTEN_CMD,
	ACCEPT_CMD,
	CONNECT_CMD,
	GETSOCKNAME_CMD,
	GETPEERNAME_CMD,
	SEND_CMD,
	SENDTO_CMD,
	RECV_CMD,
	RECVFROM_CMD,
	SETSOCKOPT_CMD,
	GETSOCKOPT_CMD,
	SHUTDOWN_CMD,
	SENDMSG_CMD,
	RECVMSG_CMD
};

/* used to extract the `cmd' field from the *_cmd structs */
struct generic_cmd
{
	short	cmd;
	char	data[10];
};

/* structures passed to the SOCKETCALL ioctl() as an argument */
struct socket_cmd
{
	short	cmd;
	short	domain;
	short	type;
	short	protocol;
};

struct socketpair_cmd
{
	short	cmd;
	short	domain;
	short	type;
	short	protocol;
};

struct bind_cmd
{
	short	cmd;
	void	*addr;
	short	addrlen;
};
		
struct listen_cmd
{
	short	cmd;
	short	backlog;
};

struct accept_cmd
{
	short	cmd;
	void	*addr;
	short	*addrlen;
};

struct connect_cmd
{
	short	cmd;
	void	*addr;
	short	addrlen;
};

struct getsockname_cmd
{
	short	cmd;
	void	*addr;
	short	*addrlen;
};

struct getpeername_cmd
{
	short	cmd;
	void	*addr;
	short	*addrlen;
};

struct send_cmd
{
	short	cmd;
	const void* buf;
	long	buflen;
	short	flags;
};

struct sendto_cmd
{
	short	cmd;
	const void *buf;
	long	buflen;
	short	flags;
	const void *addr;
	short	addrlen;
};

struct recv_cmd
{
	short	cmd;
	void	*buf;
	long	buflen;	
	short	flags;
};

struct recvfrom_cmd
{
	short	cmd;
	void	*buf;
	long	buflen;
	short	flags;
	void	*addr;
	short	*addrlen;
};

struct setsockopt_cmd
{
	short	cmd;
	short	level;
	short	optname;
	void	*optval;
	long	optlen;
};

struct getsockopt_cmd
{
	short	cmd;
	short	level;
	short	optname;
	void	*optval;
	long	*optlen;
};

struct shutdown_cmd
{
	short	cmd;
	short	how;
};

struct sendmsg_cmd
{
	short	cmd;
	const void *msg;
	short	flags;
};

struct recvmsg_cmd
{
	short	cmd;
	void	*msg;
	short	flags;
};

# endif /* _ksocket_h */
