
# ifndef _in_h
# define _in_h

#ifdef __KERNEL__
# include <mint/ktypes.h>
#else
# include <sys/types.h>
#endif


/* well-defined IP protocols */
# define IPPROTO_IP	0
# define IPPROTO_ICMP	1
# define IPPROTO_TCP	6
# define IPPROTO_UDP	17
# define IPPROTO_RAW	255
# define IPPROTO_MAX	IPPROTO_RAW

# define IS_INET_PROTO(p) \
	((p) == IPPROTO_ICMP || (p) == IPPROTO_TCP || (p) == IPPROTO_UDP)

/* well-known IP ports */
# define IPPORT_RESERVED	1024
# define IPPORT_USERRESERVED	5000

/* internet address */
struct in_addr
{
	ulong s_addr;
};

/* definitions for classifying an internet address */
# define IN_CLASSA(a)		((((long)(a)) & 0x80000000) == 0)
# define IN_CLASSA_NET		0xff000000ul
# define IN_CLASSA_NSHIFT	24
# define IN_CLASSA_HOST		(0xffffffff & ~IN_CLASSA_NET)
# define IN_CLASSA_MAX		128

# define IN_CLASSB(a)		((((long)(a)) & 0xc0000000) == 0x80000000)
# define IN_CLASSB_NET		0xffff0000ul
# define IN_CLASSB_NSHIFT	16
# define IN_CLASSB_HOST		(0xffffffff & ~IN_CLASSB_NET)
# define IN_CLASSB_MAX		65536

# define IN_CLASSC(a)		((((long)(a)) & 0xe0000000) == 0xc0000000)
# define IN_CLASSC_NET		0xffffff00ul
# define IN_CLASSC_NSHIFT	8
# define IN_CLASSC_HOST		(0xffffffff & ~IN_CLASSC_NET)

# define IN_CLASSD(a)		((((long)(a)) & 0xf0000000) == 0xe0000000)

/* well-defined IP addresses */
# define INADDR_ANY		((ulong) 0x00000000)
# define INADDR_BROADCAST	((ulong) 0xffffffff)
# define INADDR_NONE		((ulong) 0xffffffff)
# define INADDR_LOOPBACK	((ulong) 0x7f000001)

# define IN_LOOPBACKNET		127

/* structure describing an Internet socket address */
struct sockaddr_in
{
	short		sin_family;
	unsigned short	sin_port;
	struct in_addr	sin_addr;
	char		sin_zero[8];
};
# define SIN(x)		((struct sockaddr_in *) x)

/* options for use with [s|g]etsockopt' call at the IPPROTO_IP level */
# define IP_OPTIONS	1
# define IP_HDRINCL	2
# define IP_TOS		3
# define IP_TTL		4
# define IP_RECVOPTS	5
# define IP_RECVRETOPTS	6
# define IP_RECVDSTADDR	7
# define IP_RETOPTS	8

/* structure for use with IP_OPTIONS and IP_RETOPTS */
struct ip_opts
{
	struct in_addr	ip_dst;
	char		ip_opts[40];
};

/* functions to convert between host and network byte order (big endian) */
# define ntohl(x)	(x)
# define ntohs(x)	(x)
# define htonl(x)	(x)
# define htons(x)	(x)


# endif	/* _in_h */
