
# ifndef _udp_h
# define _udp_h

# include "global.h"


/* Must be enough for IP and ICMP header */
# define UDP_RESERVE	140

/* UDP datagram header */
struct udp_dgram
{
	ushort	srcport;	/* source port */
	ushort	dstport;	/* destination port */
	ushort	length;		/* total dgram length */
	ushort	chksum;		/* optional checksum */
	char	data[0];	/* begin of data */
};

# define UDP_MINLEN		(sizeof (struct udp_dgram))


extern struct in_proto udp_proto;

void	udp_init (void);
ushort	udp_checksum (struct udp_dgram *dgram, ulong srcadr, ulong dstadr);


# endif /* _udp_h */
