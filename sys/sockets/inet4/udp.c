/*
 *	User Datagram Protocol, see RFC 768 for details.
 *
 *	01/22/94, kay roemer.
 */

# include "udp.h"

# include "mint/fcntl.h"
# include "mint/ioctl.h"
# include "mint/net.h"
# include "mint/pathconf.h"

# include "icmp.h"
# include "if.h"
# include "in.h"
# include "inet.h"
# include "inetutil.h"
# include "ip.h"

# include "buf.h"
# include "iov.h"


static long	udp_attach	(struct in_data *);
static long	udp_abort	(struct in_data *, short);
static long	udp_detach	(struct in_data *, short);
static long	udp_connect	(struct in_data *, const struct sockaddr_in *, short, short);
static long	udp_accept	(struct in_data *, struct in_data *, short);
static long	udp_ioctl	(struct in_data *, short, void *);
static long	udp_select	(struct in_data *, short, long);
static long	udp_send	(struct in_data *, const struct iovec *, short, short, short, const struct sockaddr_in *, short);
static long	udp_recv	(struct in_data *, const struct iovec *, short, short, short, struct sockaddr_in *, short *);
static long	udp_shutdown	(struct in_data *, short);
static long	udp_setsockopt	(struct in_data *, short, short, char *, long);
static long	udp_getsockopt	(struct in_data *, short, short, char *, long *);

static long	udp_error	(short, short, BUF *, ulong, ulong);
static long	udp_input	(struct netif *, BUF *, ulong, ulong);

struct in_proto udp_proto =
{
	IPPROTO_UDP,
	0,
	0,
	{	udp_attach, udp_abort, udp_detach, udp_connect, 0, udp_accept,
		udp_ioctl, udp_select, udp_send, udp_recv, udp_shutdown,
		udp_setsockopt, udp_getsockopt
	},
	{	IPPROTO_UDP,
		0,
		udp_error,
		udp_input
	},
	0
};

void
udp_init (void)
{
	in_proto_register (IPPROTO_UDP, &udp_proto);
}


static long
udp_attach (struct in_data *data)
{
	data->pcb = 0;
	return 0;
}

static long
udp_abort (struct in_data *data, short ostate)
{
	UNUSED(data);
	UNUSED(ostate);
	return 0;
}

static long
udp_detach (struct in_data *data, short wait)
{
	in_data_destroy (data, wait);
	return 0;
}

static long
udp_connect (struct in_data *data, const struct sockaddr_in *addr, short addrlen, short nonblock)
{
	UNUSED(addrlen);
	UNUSED(nonblock);
	if (addr->sin_port == 0)
	{
		DEBUG (("udp_connect: port == 0."));
		data->flags &= ~IN_ISCONNECTED;
		return EADDRNOTAVAIL;
	}
	
	data->dst.addr = ip_dst_addr (addr->sin_addr.s_addr);
	data->dst.port = addr->sin_port;
	data->flags |= IN_ISCONNECTED;
	
	return 0;
}

static long
udp_accept (struct in_data *data, struct in_data *newdata, short nonblock)
{
	UNUSED(data);
	UNUSED(newdata);
	UNUSED(nonblock);
	return EOPNOTSUPP;
}

static long
udp_ioctl (struct in_data *data, short cmd, void *buf)
{
	switch (cmd)
	{
		case FIONREAD:
		{
			struct udp_dgram *uh;
			
			if ((data->sock->flags & SO_CANTRCVMORE) || data->err)
			{
				*(long *)buf = UNLIMITED;
				return 0;
			}
			
			if (!data->rcv.qfirst)
			{
				*(long *)buf = 0;
				return 0;
			}
			
			uh = (struct udp_dgram *)IP_DATA (data->rcv.qfirst);
			*(long *) buf = uh->length - sizeof (struct udp_dgram);
			
			return 0;
		}
		case FIONWRITE:
		{
			*(long *) buf = UNLIMITED;
			return 0;
		}
	}
	
	return ENOSYS;
}

static long
udp_select (struct in_data *data, short mode, long proc)
{
	switch (mode)
	{
		case O_WRONLY:
			return 1;
		
		case O_RDONLY:
			if ((data->sock->flags & SO_CANTRCVMORE) || data->err)
				return 1;
			return (data->rcv.qfirst ? 1 : so_rselect (data->sock, proc));
	}
	
	return 0;
}

static long
udp_send (struct in_data *data, const struct iovec *iov, short niov, short nonblock,
		short  flags, const struct sockaddr_in *addr, short addrlen)
{
	struct udp_dgram *uh;
	BUF *buf;
	long size, r, copied;
	ulong dstaddr, srcaddr;
	ushort dstport;
	short ipflags = 0;
	
	UNUSED(nonblock);
	UNUSED(addrlen);
	if (flags & ~MSG_DONTROUTE)
	{
		DEBUG (("udp_send: invalid flags"));
		return EOPNOTSUPP;
	}
	
	size = iov_size (iov, niov);
	if (size == 0)
		return 0;
	
	if (size < 0)
	{
		DEBUG (("udp_send: Invalid iovec"));
		return EINVAL;
	}
	
	if (size > data->snd.maxdatalen)
	{
		DEBUG (("udp_send: Message too long"));
		return EMSGSIZE;
	}
	
	if (data->flags & IN_ISCONNECTED)
	{
		if (addr) return EISCONN;
		dstaddr = data->dst.addr;
		dstport = data->dst.port;
	}
	else
	{
		if (!addr) return EDESTADDRREQ;

		DEBUG (("udp_send: s_addr = 0x%lx", addr->sin_addr.s_addr));

		dstaddr = ip_dst_addr (addr->sin_addr.s_addr);
		dstport = addr->sin_port;
		if (dstport == 0)
		{
			DEBUG (("udp_send: dstport == 0"));
			return EADDRNOTAVAIL;
		}
	}
	buf = buf_alloc (size + sizeof (struct udp_dgram) + UDP_RESERVE,
		UDP_RESERVE/2, BUF_NORMAL);
	if (!buf)
	{
		DEBUG (("udp_send: Out of mem"));
		return ENOMEM;
	}
	
	uh = (struct udp_dgram *)buf->dstart;
	uh->srcport = data->src.port;
	uh->dstport = dstport;
	uh->length = sizeof (struct udp_dgram) + size;
	uh->chksum = 0;
	copied = iov2buf_cpy (uh->data, size, iov, niov, 0);
	
	if (data->flags & IN_CHECKSUM)
	{
		srcaddr = data->src.addr;
		if (srcaddr == INADDR_ANY)
			srcaddr = ip_local_addr (dstaddr);
		if ((dstaddr & 0xf0000000ul) == INADDR_MULTICAST) {
			srcaddr = data->opts.multicast_ip;
		}
		uh->chksum = udp_checksum (uh, srcaddr, dstaddr);
		if (!uh->chksum) uh->chksum = ~0;
	}
	buf->dend += sizeof (struct udp_dgram) + size;
	
	if (data->flags & IN_BROADCAST)
		ipflags |= IP_BROADCAST;
	
	if ((data->flags & IN_DONTROUTE) || (flags & MSG_DONTROUTE))
		ipflags |= IP_DONTROUTE;
	
	DEBUG (("udp_send: dstaddr = 0x%lx", dstaddr));

	r = ip_send (data->src.addr, dstaddr, buf, IPPROTO_UDP, ipflags,
		&data->opts);
	
	if (r == 0)
		r = copied;
	
	return r;
}

static long
udp_recv (struct in_data *data, const struct iovec *iov, short niov, short nonblock,
		short flags, struct sockaddr_in *addr, short *addrlen)
{
	struct udp_dgram *uh;
	struct socket *so = data->sock;
	BUF *buf;
	long size, todo, copied;
	
	size = iov_size (iov, niov);
	if (size == 0)
		return 0;
	
	if (size < 0)
	{
		DEBUG (("udp_recv: invalid iovec"));
		return EINVAL;
	}
	
	if (addr && (!addrlen || *addrlen < 0))
	{
		DEBUG (("udp_recv: invalid address len"));
		return EINVAL;
	}
	
	while (!data->rcv.qfirst)
	{
		if (nonblock)
		{
			DEBUG (("udp_recv: EAGAIN"));
			return EAGAIN;
		}
		
		if (so->flags & SO_CANTRCVMORE)
		{
			DEBUG (("udp_recv: shut down"));
			return 0;
		}
		
		if (isleep (IO_Q, (long)so))
		{
			DEBUG (("udp_recv: interrupted"));
			return EINTR;
		}
		
		if (so->state != SS_ISUNCONNECTED)
		{
			DEBUG (("udp_recv: Socket shut down while sleeping"));
			return 0;
		}
		
		if (data->err)
		{
			copied = data->err;
			data->err = 0;
			return copied;
		}
	}
	
	buf = data->rcv.qfirst;
	uh = (struct udp_dgram *) IP_DATA (buf);
	todo = uh->length - sizeof (struct udp_dgram);
	copied = buf2iov_cpy (uh->data, todo, iov, niov, 0);
	
	if (addr)
	{
		struct sockaddr_in in;
		
		*addrlen = MIN ((ushort)*addrlen, sizeof (in));
		in.sin_family = AF_INET;
		in.sin_addr.s_addr = IP_SADDR (buf);
		in.sin_port = uh->srcport;
		memcpy (addr, &in, *addrlen);
	}
	
	if (!(flags & MSG_PEEK))
	{
		if (!buf->next)
		{
			data->rcv.qfirst = data->rcv.qlast = 0;
			data->rcv.curdatalen = 0;
		}
		else
		{
			data->rcv.qfirst = buf->next;
			data->rcv.curdatalen -= todo;
			buf->next->prev = 0;
		}
		
		buf_deref (buf, BUF_NORMAL);
	}
	
	return copied;
}

static long
udp_shutdown (struct in_data *data, short how)
{
	UNUSED(data);
	UNUSED(how);
	return 0;
}

static long
udp_setsockopt (struct in_data *data, short level, short optname, char *optval, long optlen)
{
	/*
	 * No special UDP socket options
	 */
	UNUSED(data);
	UNUSED(level);
	UNUSED(optname);
	UNUSED(optval);
	UNUSED(optlen);
	return EOPNOTSUPP;
}

static long
udp_getsockopt (struct in_data *data, short level, short optname, char *optval, long *optlen)
{
	/*
	 * No special UDP socket options
	 */
	UNUSED(data);
	UNUSED(level);
	UNUSED(optname);
	UNUSED(optval);
	UNUSED(optlen);
	return EOPNOTSUPP;
}

/*
 * If destination non existant then don't free buf and return != 0.
 * Otherwise always take over (or free) buf and return zero.
 */
static long
udp_input (struct netif *iface, BUF *buf, ulong saddr, ulong daddr)
{
	struct udp_dgram *uh = (struct udp_dgram *) IP_DATA (buf);
	struct in_data *data;
	ulong pktlen;
	
	UNUSED(iface);
	pktlen = (long)buf->dend - (long)uh;
	if (pktlen < UDP_MINLEN || pktlen != uh->length)
	{
		DEBUG (("udp_input: invalid packet length"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	if (uh->chksum && udp_checksum (uh, saddr, daddr))
	{
		DEBUG (("udp_input: Bad checksum"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	
	data = in_data_lookup (udp_proto.datas, saddr, uh->srcport,
		daddr, uh->dstport);
	if (!data)
	{
		BUF *nbuf;
		
		DEBUG (("udp_input: Destination port %d non existant",
			uh->dstport));
		
		/* 07/14/1999 TL
		 * It would violate RFC1122 to send an ICMP error when the
		 * daddr is a broadcast address!!!
		 */
		if (!ip_is_brdcst_addr (daddr))
		{
			nbuf = buf_clone (buf, BUF_NORMAL);
			if (nbuf != 0)
				icmp_send (ICMPT_DSTUR, ICMPC_PORTUR, saddr, nbuf, 0);
		}
		
		return -1;
	}
	pktlen -= sizeof (struct udp_dgram);
	if (pktlen + data->rcv.curdatalen > (ulong)data->rcv.maxdatalen)
	{
		DEBUG (("udp_input: Input queue full"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	if (data->sock->flags & SO_CANTRCVMORE)
	{
		DEBUG (("udp_input: Dropping packet, receiver shutdown"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	buf->next = 0;
	
	if (data->rcv.qlast)
	{
		data->rcv.qlast->next = buf;
		buf->prev = data->rcv.qlast;
		data->rcv.qlast = buf;
	}
	else
	{
		buf->prev = 0;
		data->rcv.qfirst = data->rcv.qlast = buf;
	}
	data->rcv.curdatalen += pktlen;
	so_wakersel (data->sock);
	wake (IO_Q, (long) data->sock);
	
	return 0;
}

/*
 * This is called from the icmp module on dst unreachable messages.
 * Note that `saddr' is our address, ie the source address of the packet
 * that caused the icmp reply.
 */
static long
udp_error (short type, short code, BUF *buf, ulong saddr, ulong daddr)
{
	struct in_data *data;
	struct udp_dgram *uh = (struct udp_dgram *)IP_DATA (buf);
	
	data = in_data_lookup (udp_proto.datas, daddr, uh->dstport,
		saddr, uh->srcport);
	if (!data || !(data->flags & IN_ISCONNECTED))
	{
		DEBUG (("udp_error: no such local socket (%lx, %x)",
			saddr, uh->srcport));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	DEBUG (("udp_error: destination (%lx, %x) unreachable",
		daddr, uh->dstport));
	
	data->err = icmp_errno (type, code);
	so_wakersel (data->sock);
	so_wakewsel (data->sock);
	wake (IO_Q, (long)data->sock);
	buf_deref (buf, BUF_NORMAL);
	
	return 0;
}

ushort
udp_checksum (struct udp_dgram *dgram, ulong srcadr, ulong dstadr)
{
	ulong sum = 0;
	ushort len = dgram->length;
	
	/*
	 * Pseudo IP header checksum
	 */
	__asm__(
		"\tmoveq	#0, d0		\n"
		"\tmovel	%3, %0		\n"
		"\taddl	%1, %0		\n"
		"\taddxl	%2, %0		\n"
#ifdef __mcoldfire__
		"\tmvzw	%4, d1		\n"
		"\taddxl	d1, %0		\n"
		"\taddxl	d0, %0		\n"
#else
		"\taddxw	%4, %0		\n"
		"\taddxw	d0, %0		\n"
#endif
		: "=d"(sum):"g"(srcadr),
		    "d"(dstadr), "i"(IPPROTO_UDP), "d"(len), "0"(sum)
#ifdef __mcoldfire__
		: "d0", "d1", "cc"
#else
		: "d0", "cc"
#endif
		);

	/*
	 * UDP datagram & header checksum
	 */
	__asm__(
		"\tclrl	d0		\n"
#ifdef __mcoldfire__
		"\tmvzw	%2, d1		\n"
		"\tlsrl	#4, d1		\n"
#else
		"\tmovew	%2, d1	\n"
		"\tlsrw	#4, d1		\n"
#endif
		"\tbeq	4f		\n"
#ifdef __mcoldfire__
		"\tsubql	#1, d1\n"	/* clears X bit */
#else
		"\tsubqw	#1, d1\n"		/* clears X bit */
#endif
		"1:\n"
#ifdef __mcoldfire__
		"\tmoveml	%4@, d0/d2-d4\n"	/* 16 byte loop */
		"\tlea	%4@(16), %4	\n"
#else
		"\tmoveml	%4@+, d0/d2-d4\n"	/* 16 byte loop */
#endif
		"\taddxl	d0, %0\n"	/* ~5 clock ticks per byte */
		"\taddxl	d2, %0\n"
		"\taddxl	d3, %0\n"
		"\taddxl	d4, %0\n"
#ifdef __mcoldfire__
		"\tmoveq	#0, d0\n"	/* X not affected */
		"\taddxl	d0, %0\n"
		"\tsubql	#1, d1\n"	/* X cloberred */
		"\tbpls	1b		\n"		/* X not affected */
#else
		"\tdbra	d1, 1b\n"
		"\tclrl	d0\n"
		"\taddxl	d0, %0\n"
#endif
		"4:\n"
		"\tmovew	%2, d1\n"
#ifdef __mcoldfire__
		"\tandil	#0xf, d1\n"
		"\tlsrl	#2, d1\n"
#else
		"\tandiw	#0xf, d1\n"
		"\tlsrw	#2, d1\n"
#endif
		"\tbeq	2f\n"
#ifdef __mcoldfire__
		"\tsubql	#1, d1\n"
#else
		"\tsubqw	#1, d1\n"
#endif
		"3:\n"
		"\taddl	%4@+, %0\n"	/* 4 byte loop */
		"\taddxl	d0, %0\n"	/* ~10 clock ticks per byte */
#ifdef __mcoldfire__
		"\tsubql	#1, d1\n" "bpls	3b		\n\t"
#else
		"\tdbra	d1, 3b\n"
#endif
		"2:\n"
		: "=d"(sum), "=a"(dgram)
		: "g"(len), "0"(sum), "1"(dgram)
		: "d0", "d1", "d2", "d3", "d4", "cc");

	/*
	 * Add in extra word, byte (if len not multiple of 4).
	 * Convert to short
	 */
	__asm__(
		"\tclrl	d0\n"
		"\tbtst	#1, %2\n"
		"\tbeq	5f\n"
#ifdef __mcoldfire__
		"\tmvz.w	%4@+, d2\n"
		"\taddl	d2, %0\n"	/* no, add in extra word */
		"\taddxl	d0, %0\n"
#else
		"\taddw	%4@+, %0\n"	/* extra word */
		"\taddxw	d0, %0\n"
#endif
		"5:\n"
		"\tbtst	#0, %2\n"
		"\tbeq	6f\n"
#ifdef __mcoldfire__
		"\tmvzb	%4@+, d1\n"	/* extra byte */
		"\tlsll	#8, d1\n"
		"\taddl	d1, %0\n"
		"\taddxl	d0, %0\n"
#else
		"\tmoveb	%4@+, d1\n"	/* extra byte */
		"\tlslw	#8, d1\n"
		"\taddw	d1, %0\n"
		"\taddxw	d0, %0\n"
#endif
		"6:\n\t"
#ifdef __mcoldfire__
		"\tswap	%0		\n"		/* convert to short */
		"\tmvzw	%0, d1\n"
		"\tclr.w	%0\n"
		"\tswap	%0\n"
		"\taddl	d1, %0\n"
		"\tswap	%0\n"
		"\tmvzw	%0, d1\n"
		"\tclr.w	%0\n"
		"\tswap	%0\n"
		"\taddl	d1, %0\n"
#else
		"\tmovel	%0, d1\n"	/* convert to short */
		"\tswap	d1\n"
		"\taddw	d1, %0\n"
		"\taddxw	d0, %0\n"
#endif
		: "=d"(sum), "=a"(dgram)
		: "d"(len), "0"(sum), "1"(dgram)
#ifdef __mcoldfire__
		: "d0", "d1", "d2", "cc"
#else
		: "d0", "d1", "cc"
#endif
		);

	return (short)(~sum & 0xffff);
}
