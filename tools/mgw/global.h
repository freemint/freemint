/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999 Frank Naumann <fnaumann@freemint.de>
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

# ifndef _global_h
# define _global_h

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>
# include <fcntl.h>
# include <netdb.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <errno.h>

# include <mint/mintbind.h>

# if __MINTLIB_MAJOR__ == 0 && __MINTLIB_MINOR__ < 57
# error Require at least MiNTLib 0.57
# endif

# ifndef _cdecl
# define _cdecl		__CDECL
# endif

# define str(x)		_stringify (x)
# define _stringify(x)	#x


# define TCPCOOKIE	0x49434950L	/* ICIP */
# define FREECOOKIE	0x46524545L	/* FREE */
# define JAR		0x5A0

/* struct for Pmsg() */
typedef struct
{
	long msg1;
	long msg2;
	short pid;
} PMSG;

# define MGW_LIBSOCKETCALL	0x6d676d01UL
# define MGW_GETHOSTBYNAME	1
# define MGW_GETHOSTBYADDR	2
# define MGW_GETHOSTNAME	3
# define MGW_GETSERVBYNAME	4
# define MGW_GETSERVBYPORT	5
# define MGW_GETUNLOCK 		6
# define MGW_NEWCMDLINE		7

/*
 * Adapted part of STSOCKET.H
 */

# define ST_SETFL	83
# define ST_GETFL	84

/* Extended File Options */
# define ST_O_NONBLOCK	04000

/* Ioctl options */
# define ST_FIONBIO	126

# define ST_EAGAIN	 11	/* Do it again */
# define ST_EPROTO	 71	/* Protocol error */
# define ST_ENOTSOCK	 88	/* Not a valid socket */
# define ST_EOPNOTSUPP	 95	/* Operation not supported */
# define ST_EADDRINUSE	 98	/* address is already in use */
# define ST_ENOBUFS	105
# define ST_EISCONN	106	/* socket is already connected */
# define ST_ENOTCONN	107	/* socket is not connected */
# define ST_EALREADY	114	/* operation in progress */
# define ST_EINPROGRESS	115	/* operation started */

/* Message header */
struct st_msghdr
{
	void *msg_name;
	short msg_namelen;
	struct iovec *msg_iov;
	long msg_iovlen;
	void *msg_accrights;
	long msg_accrightslen;
};

struct st_servent
{
	char *s_name;
	char **s_aliases;
	short s_port;
	char *s_proto;
};

struct st_hostent
{
	char *h_name;
	char **h_aliases;
	short h_addrtype;
	short h_length;
	char **h_addr_list;
};


# define ST_FDSET_LONGS 8

typedef struct st_fd_set st_fd_set;
struct st_fd_set 
{
	ulong fds_bits [ST_FDSET_LONGS];
};

# define ST_NFDBITS		(8 * sizeof(unsigned long))
# define ST_FD_SETSIZE		(ST_FDSET_LONGS * ST_NFDBITS)

# define ST_FDELT(d)		(d / ST_NFDBITS)
# define ST_FDMASK(d)		(1L << (d % ST_NFDBITS))
# define ST_FD_SET(d, set)	((set)->fds_bits[ST_FDELT(d)] |= ST_FDMASK(d))
# define ST_FD_CLR(d, set)	((set)->fds_bits[ST_FDELT(d)] &= ~ST_FDMASK(d))
# define ST_FD_ISSET(d, set)	((set)->fds_bits[ST_FDELT(d)] & ST_FDMASK(d))
# define ST_FD_ZERO(set) 	bzero (set, sizeof(st_fd_set))


# endif /* _global_h */
