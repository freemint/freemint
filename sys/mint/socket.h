/*
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
 
# ifndef _mint_socket_h
# define _mint_socket_h

# include "iov.h"

typedef unsigned short sa_family_t;

/* socket types */
enum so_type
{
	SOCK_STREAM = 1,
	SOCK_DGRAM,
	SOCK_RAW,
	SOCK_RDM,
	SOCK_SEQPACKET
};

/* protocol families */
# define PF_UNSPEC	0
# define PF_UNIX	1
# define PF_INET	2
# define PF_APPLETALK	5

/* address families, same as above */
# define AF_UNSPEC	PF_UNSPEC
# define AF_UNIX	PF_UNIX
# define AF_INET	PF_INET
# define AF_APPLETALK	PF_APPLETALK
# define AF_LINK	200

/* flags for send and recv */
# define MSG_OOB	1
# define MSG_PEEK	2
# define MSG_DONTROUTE	4

/* [s|g]etsockopt() levels */
# define SOL_SOCKET	0xffff

/* [s|g]etsockopt() options */
# define SO_DEBUG	1	/* debugging on/off */
# define SO_REUSEADDR	2	/* duplicate socket addesses on/off */
# define SO_TYPE	3	/* get socket type */
# define SO_ERROR	4	/* reset socket error status */
# define SO_DONTROUTE	5	/* routing of outgoing messages on/off */
# define SO_BROADCAST	6	/* may datagramms be broadcast */
# define SO_SNDBUF	7	/* set/get size of output buffer */
# define SO_RCVBUF	8	/* set/get size of input buffer */
# define SO_KEEPALIVE	9	/* periodically connection checking on/off*/
# define SO_OOBINLINE	10	/* place oob-data in input queue on/off */
# define SO_LINGER	11	/* what to do when closing a socket */
# define SO_ACCEPTCONN  30
# define SO_CHKSUM	40	/* switch checksum generation on/off */
# define SO_DROPCONN	41	/* drop incoming conn. when accept() fails */

/* structure to pass for SO_LINGER */
struct linger
{
	long	l_onoff;	/* when != 0, close() blocks */
	long	l_linger;	/* timeout in seconds */
};

/* generic socket address */
struct sockaddr
{
	short	sa_family;
	char	sa_data[14];
};

#define IS_MULTICAST(x)	(((x) & htonl(0xf0000000)) == htonl(0xe0000000))

/*
 *  Desired design of maximum size and alignment.
 */
#define _SS_MAXSIZE 	128
#define _SS_ALIGNSIZE	sizeof(unsigned short) /* May be unsigned long ?*/

#define _SS_PAD1SIZE   ((2 * _SS_ALIGNSIZE - sizeof (sa_family_t)) % _SS_ALIGNSIZE)
#define _SS_PAD2SIZE   (_SS_MAXSIZE - (sizeof (sa_family_t) + _SS_PAD1SIZE + _SS_ALIGNSIZE))

struct sockaddr_storage {
    sa_family_t	ss_family;
    char		__ss_pad1[_SS_PAD1SIZE];
    unsigned long	__ss_align[_SS_PAD2SIZE / sizeof(unsigned long) + 1];
};

/* structure used with sendmsg() and recvmsg() */
struct msghdr
{
	struct sockaddr	*msg_name;
	long			msg_namelen;
	struct iovec	*msg_iov;
	long			msg_iovlen;
	void			*msg_accrights;
	long			msg_accrightslen;
};


# endif /* _mint_socket_h */
