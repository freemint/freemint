
# ifndef _if_h
# define _if_h

# include "global.h"

# include "buf.h"
# include "socket.h"


/* net interface flags */
# define IFF_UP			0x0001	/* if is up */
# define IFF_BROADCAST		0x0002	/* if supports broadcasting */
# define IFF_DEBUG		0x0004	/* if debugging is on */
# define IFF_LOOPBACK		0x0008	/* if is software loopback */
# define IFF_POINTOPOINT	0x0010	/* if for p2p connection */
# define IFF_NOTRAILERS		0x0020	/* if should not use trailer encaps. */
# define IFF_RUNNING		0x0040	/* if ressources are allocated */
# define IFF_NOARP		0x0080	/* if should not use arp */
# define IFF_MASK		(IFF_UP|IFF_DEBUG|IFF_NOTRAILERS|IFF_NOARP)

# define IF_NAMSIZ		16	/* maximum if name len */
# define IF_MAXQ		60	/* maximum if queue len */
# define IF_SLOWTIMEOUT		1000	/* one second */
# define IF_PRIORITY_BITS	1
# define IF_PRIORITIES		(1 << IF_PRIORITY_BITS)

/*
 * socket address carrying a hardware address
 */
struct sockaddr_hw
{
	ushort		shw_family;
	ushort		shw_type;
	ushort		shw_len;
	uchar		shw_addr[8];
};

struct netif;

/* structure for holding address information, assumes internet style */
struct ifaddr
{
	struct sockaddr	addr;		/* local address */
	union {
		struct sockaddr	broadaddr;	/* broadcast address */
		struct sockaddr	dstaddr;	/* point2point dst address */
	} ifu;
	struct netif	*ifp;		/* interface this belongs to */
	struct ifaddr	*next;		/* next ifaddr */
	short		family;		/* address family */
	ushort		flags;		/* flags */

	/* AF_INET specific */
	ulong		net;		/* network id */
	ulong		netmask;	/* network mask */
	ulong		subnet;		/* subnet id */
	ulong		subnetmask;	/* subnet mask */
	ulong		net_broadaddr;	/* logical broadcast addr */
};

/* Interface packet queue */
struct ifq
{
	short		maxqlen;
	short		qlen;
	short		curr;
	BUF		*qfirst[IF_PRIORITIES];
	BUF		*qlast[IF_PRIORITIES];
};

/* Hardware address */
struct hwaddr
{
	short		len;
	uchar		addr[10];
};

/* structure describing a net interface */
struct netif
{
	char		name[IF_NAMSIZ];/* interface name */
	short		unit;		/* interface unit */

	ushort		flags;		/* interface flags */
	ulong		metric;		/* routing metric */
	ulong		mtu;		/* maximum transmission unit */
	ulong		timer;		/* timeout delta in ms */
	short		hwtype;		/* hardware type */
/*
 * These must match the ARP hardware types in arp.h
 */
# define HWTYPE_ETH	1		/* ethernet */
# define HWTYPE_NONE	200		/* pseudo hw type are >= this */
# define HWTYPE_SLIP	201
# define HWTYPE_PPP	202
# define HWTYPE_PLIP	203
	
	struct hwaddr	hwlocal;	/* local hardware address */
	struct hwaddr	hwbrcst;	/* broadcast hardware address */
	
	struct ifaddr	*addrlist;	/* addresses for this interf. */
	struct ifq	snd;		/* send and recv queue */
	struct ifq	rcv;
	
	long		(*open)(struct netif *);
	long		(*close)(struct netif *);
	long		(*output)(struct netif *, BUF *, char *, short, short);
	long		(*ioctl)(struct netif *, short, long);
	void		(*timeout)(struct netif *);

	void		*data;		/* extra data the if may want */
	
	ulong		in_packets;	/* # input packets */
	ulong		in_errors;	/* # input errors */
	ulong		out_packets;	/* # output packets */
	ulong		out_errors;	/* # output errors */
	ulong		collisions;	/* # collisions */
	
	struct netif	*next;		/* next interface */
	
	short		maxpackets;	/* max. number of packets the harware
					 * can receive in fast succession.
					 * 0 means unlimited. (this is used
					 * for ethercards with few receive
					 * buffers and slow io to don't over-
					 * flow them with packets.
					 */
	struct bpf	*bpf;		/* packet filter list */
	uchar		*base_addr;	/* base address of a board (exact meaning
					 * depends on the device driver)
					 */
	long		reserved[3];
};

/* interface statistics */
struct ifstat
{
	ulong		in_packets;	/* # input packets */
	ulong		in_errors;	/* # input errors */
	ulong		out_packets;	/* # output packets */
	ulong		out_errors;	/* # output errors */
	ulong		collisions;	/* # collisions */
};

/* argument structure for the SIOC* ioctl()'s on sockets */
struct ifreq
{
	char	ifr_name[IF_NAMSIZ];		/* interface name */
	union {
		struct	sockaddr addr;		/* local address */
		struct	sockaddr dstaddr;	/* p2p dst address */
		struct	sockaddr broadaddr;	/* broadcast addr */
		struct	sockaddr netmask;	/* network mask */
		short	flags;			/* if flags, IFF_* */
		long	metric;			/* routing metric */
		long	mtu;			/* max transm. unit */
		struct	ifstat stats;		/* interface statistics */
		void	*data;			/* other data */
	} ifru;
};

/* result structure for the SIOCGIFCONF socket ioctl() */
struct ifconf
{
	short	len;				/* buffer len */
	union {
		void 		*buf;		/* the actual buffer */
		struct ifreq 	*req;		/* ifreq structure */
	} ifcu;
};

/* structure used with SIOCSIFLINK */
struct iflink
{
	char	ifname[16];	/* interface to link device to without unit
				 * number, eg 'sl'. On successful return
				 * the actual interface to which the device
				 * was linked, eg 'sl0', can be found here. */
	char	device[128];	/* device name, eg '/dev/ttya' */
};

/* structure used with SIOCSIFOPT */
struct ifopt
{
	char 	option[32];
	short	valtype;
	short	vallen;
	union {
		long v_long;
		char v_string[128];
	} ifou;
};

/* value types for ifopt.valtype */
# define IFO_INT	0	/* integer, uses v_long */
# define IFO_STRING	1	/* string, uses v_string */
# define IFO_HWADDR	2	/* hardware address, v_string[0..5] */


extern struct netif *allinterfaces, *if_lo;

short		if_enqueue	(struct ifq *, BUF *, short pri);
short		if_putback	(struct ifq *, BUF *, short pri);
BUF *		if_dequeue	(struct ifq *);
void		if_flushq	(struct ifq *);
long		if_register	(struct netif *);
long		if_init		(void);
short		if_input	(struct netif *, BUF *, long, short);

/*
 * These must match ethernet protcol types
 */
# define PKTYPE_IP	0x0800
# define PKTYPE_ARP	0x0806
# define PKTYPE_RARP	0x8035

long		if_ioctl	(short cmd, long arg);
long		if_config	(struct ifconf *);

struct netif *	if_name2if	(char *);
struct netif *	if_net2if	(ulong);
long		if_setifaddr	(struct netif *, struct sockaddr *);
struct ifaddr *	if_af2ifaddr	(struct netif *, short fam);
short		if_getfreeunit	(char *);

long		if_open		(struct netif *);
long		if_close	(struct netif *);
long		if_send		(struct netif *, BUF *, ulong, short);
void		if_load		(void);


# endif /* _if_h */
