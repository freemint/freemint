/*
 * Filename:     
 * Version:      
 * Author:       Jens Heitmann, Frank Naumann
 * Started:      1999-05
 * Last Updated: 1999-08-18
 * Target O/S:   MiNT
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@freemint.de>
 *               Copyright 1999 Jens Heitmann
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

# define MGW_GETHOSTBYNAME	0x6d676d01UL

	
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
