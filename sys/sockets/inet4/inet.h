/*
 *	Definitions and the like for the INET layer, protocol independet.
 *
 *	01/15/94, kay roemer.
 */

# ifndef _inet_h
# define _inet_h

# include "global.h"

# include "if.h"
# include "in.h"
# include "ip.h"

# include "buf.h"
# include "net.h"


struct in_dataq
{
	long		maxdatalen;	/* upper limit of bytes in this q */
	long		curdatalen;	/* current # of bytes in this q */
# define IN_DEFAULT_RSPACE	(64240)
# define IN_DEFAULT_WSPACE	(64240)
# define IN_MAX_RSPACE		(65535)
# define IN_MAX_WSPACE		(65535)
# define IN_MIN_RSPACE		(8192/2)
# define IN_MIN_WSPACE		(8192/2)
	long		lowat;		/* low watermark */
	long		hiwat;		/* high watermark */
	short		flags;		/* misc flags */
	BUF		*qfirst;	/* start of buffer queue */
	BUF		*qlast;		/* last buf in queue */
};

struct in_sockadr
{
	ulong	addr;		/* IP address */
	ushort	port;		/* port number */
};

struct in_data;

/* Interface to the socket layer */
struct in_sock_ops
{
	long	(*attach)	(struct in_data *);
	long	(*abort)	(struct in_data *, short ostate);
	long	(*detach)	(struct in_data *, short wait);
	long	(*connect)	(struct in_data *, struct sockaddr_in *,
				short addrlen, short flags);
	long	(*listen)	(struct in_data *);
	long	(*accept)	(struct in_data *, struct in_data *,
				short flags);
	long	(*ioctl)	(struct in_data *, short, void *);
	long	(*select)	(struct in_data *, short, long);
	long	(*send)		(struct in_data *, struct iovec *,
				short niov, short block, short flags,
				struct sockaddr_in *addr, short addrlen);

	long	(*recv)		(struct in_data *, struct iovec *,
				short niov, short block, short flags,
				struct sockaddr_in *addr, short *addrlen);

	long	(*shutdown)	(struct in_data *, short how);
	long	(*setsockopt)	(struct in_data *, short level, short optname,
				char *optval, long optlen);

	long	(*getsockopt)	(struct in_data *, short level, short optname,
				char *optval, long *optlen);
};

/* Interface to IP */
struct in_ip_ops
{
	short			proto;
	struct in_ip_ops	*next;
	long	(*error)	(short type, short code, BUF *,
				ulong sadr, ulong dadr);
	long	(*input) 	(struct netif *, BUF *, ulong sadr,
				ulong dadr);
};	

struct in_proto
{
	short			proto;	/* protocol number */
	short			flags;	/* gobal protocol flags */
	struct in_proto		*next;	/* next IP protocol */
	struct in_sock_ops	soops;	/* sock layer <-> proto ops */
	struct in_ip_ops	ipops;	/* proto <-> IP ops */
	struct in_data		*datas;	/* sockets belonging to this proto */
};
	
struct in_data
{
	short			protonum; /* protocol number, IPPROTO_* */
	struct in_proto		*proto;	  /* the associated protocol */
	struct socket		*sock;	  /* socket this data belongs to */
	struct in_data		*next;	  /* next in_data in list */
	struct ip_options	opts;	  /* IP per packet options */
	void			*pcb;	  /* protocol control block */
	struct in_dataq		snd;	  /* send queue */
	struct in_dataq		rcv;	  /* receive queue */
	struct in_sockadr	src;	  /* source address */
	struct in_sockadr	dst;	  /* destination address */
	short			flags;	  /* misc flags */
# define IN_ISBOUND	0x0001		  /* bind() was done on the socket */
# define IN_HASPORT	IN_ISBOUND	  /* socket has local port */
# define IN_ISCONNECTED	0x0002		  /* socket is connected */
# define IN_REUSE	0x0004		  /* reuse local port numebers */
# define IN_KEEPALIVE	0x0008		  /* keep tcp line open */
# define IN_OOBINLINE	0x0010		  /* handle oob data inline */
# define IN_CHECKSUM	0x0020		  /* generate checksums */
# define IN_DONTROUTE	0x0040		  /* don't route outgoing dgrams */
# define IN_BROADCAST	0x0080		  /* toggle broadcast permission */
# define IN_LINGER	0x0100		  /* lingering active */
# define IN_DEAD	0x0200		  /* socket is dead (TCP timewait) */
	short			backlog;  /* backlog limit */
	long			linger;	  /* lingering period */
	volatile long		err;	  /* asyncronous error */
};


void inet_init (void);


# endif /* _inet_h */
