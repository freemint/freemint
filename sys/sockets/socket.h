/*
 *	Definitions for dealing with sockets, mostly system independend.
 *	The system dependend part is found in mintsock.h
 *
 *	09/25/93, kay roemer.
 */
 
# ifndef _mint_socket_h
# define _mint_socket_h

# include "iov.h"

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

/* structure used with sendmsg() and recvmsg() */
struct msghdr
{
	struct sockaddr	*msg_name;
	long		msg_namelen;
	struct iovec	*msg_iov;
	long		msg_iovlen;
	void		*msg_accrights;
	long		msg_accrightslen;
};


# endif /* _mint_socket_h */
