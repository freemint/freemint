/*
 *	Definitions for the dummy or localhost IP implementation.
 *
 *	01/21/94, kay roemer.
 *	07/01/99, masquerading code by Mario Becroft.
 */

# ifndef _ip_h
# define _ip_h

# include "global.h"

# include "if.h"
# include "route.h"

# include "buf.h"


/* Minimal accepable length of an IP packet */
# define IP_MINLEN	(sizeof (struct ip_dgram))

# define IP_DEFAULT_TTL	255
# define IP_DEFAULT_TOS	0

/* Some macros to access data in the ip header for higher level protocols */
# define IP_HDRLEN(buf)	(((struct ip_dgram *)(buf)->dstart)->hdrlen * 4)
# define IP_DADDR(buf)	(((struct ip_dgram *)(buf)->dstart)->daddr)
# define IP_SADDR(buf)	(((struct ip_dgram *)(buf)->dstart)->saddr)
# define IP_PROTO(buf)	(((struct ip_dgram *)(buf)->dstart)->proto)
# define IP_DATA(buf)	((buf)->dstart + ((struct ip_dgram *)(buf)->dstart)->hdrlen * sizeof (long))

/* IP datagramm */
struct ip_dgram
{
	uchar		version:4;	/* version number */
# define IP_VERSION	4		/* current IP version */
	
	uchar		hdrlen:4;	/* header len */
	uchar		tos;		/* type of service and precedence */
	ushort		length;		/* datagram length */
	ushort		id;		/* datagram id */
	ushort		fragoff;	/* fragment offset */
# define IP_MF		0x2000		/* more fragments bit */
# define IP_DF		0x4000		/* don't fragment bit */
# define IP_FRAGOFF	0x1fff		/* fragment offset */

	uchar		ttl;		/* time to live */
	uchar		proto;		/* next protocol id */
	short		chksum;		/* checksum */
	ulong		saddr;		/* IP source address */
	ulong		daddr;		/* IP destination address */
	char		data[0];	/* options and data */
};

struct ip_options
{
	short		pri;
	uchar		ttl;
	uchar		tos;
	uchar		hdrincl:1;
	ulong		multicast_ip;
	uchar		multicast_loop;
};

/* IP Type Of Service */
# define IPTOS_LOWDELAY	0x10
# define IPTOS_THROUPUT	0x08
# define IPTOS_RELIABLE	0x04

# define IPTOS_PRIORITY(x)	(((x) & 0xe0) >> 5)

/* IP options */
# define IPOPT_COPY	0x80		/* copy on fragmentation flag */
# define IPOPT_CLASS	0x60		/* option class */
# define IPOPT_NUMBER	0x1f		/* option number */
# define IPOPT_TYPE	(IPOPT_CLASS|IPOPT_NUMBER)

# define IPOPT_EOL	0x00		/* end of option list */
# define IPOPT_NOP	0x01		/* no operation */
# define IPOPT_SECURITY	0x02		/* security option */
# define IPOPT_LSRR	0x03		/* loose source and record route */
# define IPOPT_SSRR	0x09		/* strict source and record route */
# define IPOPT_RR	0x07		/* record route option */
# define IPOPT_STREAM	0x08		/* SATNET stream id option */
# define IPOPT_STAMP	0x44		/* internet time stamp option */

/*
 * Return values from ip_chk_addr()
 */
# define IPADDR_NONE		0
# define IPADDR_LOCAL		1
# define IPADDR_BRDCST		2
# define IPADDR_BADCLASS	3
# define IPADDR_MULTICST        4

/*
 * Flags for ip_send()
 */
# define IP_DONTROUTE	0x01
# define IP_BROADCAST	0x02

extern struct in_ip_ops *allipprotos;
extern short ip_dgramid;

short	ip_is_brdcst_addr (ulong);
short	ip_is_local_addr (ulong);

ulong	ip_local_addr (ulong);
short	ip_same_addr (ulong, ulong);
ulong	ip_dst_addr (ulong);
short	ip_chk_addr (ulong, struct route *);
short	ip_priority (short, uchar);

struct in_ip_ops;

void	ip_register (struct in_ip_ops *);
ulong	ip_netmask (ulong);
void	ip_input (struct netif *, BUF *);
long	ip_send (ulong, ulong, BUF *, short, short, struct ip_options *);
long	ip_output (BUF *);

BUF *	ip_defrag (BUF *);

long	ip_setsockopt (struct ip_options *, short, short, char *, long);
long	ip_getsockopt (struct ip_options *, short, short, char *, long *);


# endif /* _ip_h */
