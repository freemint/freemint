/*
 *	Internet control message protocol. as per RFC 792.
 *
 *	02/05/94, Kay Roemer.
 *	07/14/99, T. Lang - Changes to handling of ICMP echo requests
 */

# include "icmp.h"

# include "if.h"
# include "inetutil.h"
# include "ip.h"
# include "route.h"


static long icmp_input (struct netif *, BUF *, ulong, ulong);
static long icmp_error (short, short, BUF *, ulong, ulong);

struct in_ip_ops icmp_ops =
{
	proto:	IPPROTO_ICMP,
	next:	NULL,
	error:	icmp_error,
	input:	icmp_input
};

static BUF	*do_send (short, short, BUF *, BUF *);

static long	do_unreach  (BUF *, struct netif *, long);
static long	do_erreport (BUF *, struct netif *, long);
static long	do_redir    (BUF *, struct netif *, long);
static long	do_echo     (BUF *, struct netif *, long);
static long	do_info     (BUF *, struct netif *, long);
static long	do_time     (BUF *, struct netif *, long);
static long	do_mask     (BUF *, struct netif *, long);


static long
icmp_input (struct netif *nif, BUF *buf, ulong saddr, ulong daddr)
{
	struct icmp_dgram *icmph;
	short datalen;
	
	UNUSED(saddr);
	UNUSED(daddr);
	icmph = (struct icmp_dgram *) IP_DATA (buf);
	
	datalen = (long) buf->dend - (long)icmph;
	if (datalen & 1)
		*buf->dend = 0;
	
	if (chksum (icmph, (datalen+1)/2))
	{
		DEBUG (("icmp_input: bad checksum from 0x%lx", saddr));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}
	
	switch (icmph->type)
	{
		case ICMPT_ECHORP:
			TRACE (("icmp_input: ECHO REPLY from 0x%lx", saddr));
			return -1;
		
		case ICMPT_DSTUR:
			TRACE (("icmp_input: DESTINATION UNREACH. from 0x%lx", saddr));
			do_unreach (buf, nif, datalen);
			do_erreport (buf, nif, datalen);
		return -1;
		
		case ICMPT_TIMEX:
			TRACE (("icmp_input: TIME EXCEEDED from 0x%lx", saddr));
			do_erreport (buf, nif, datalen);
			return -1;
		
		case ICMPT_SRCQ:
			TRACE (("icmp_input: SOURCE QUENCH from 0x%lx", saddr));
			do_erreport (buf, nif, datalen);
			return -1;
		
		case ICMPT_REDIR:
			TRACE (("icmp_input: REDIRECT from 0x%lx", saddr));
			do_redir (buf, nif, datalen);
			return -1;
		
		case ICMPT_ECHORQ:
			TRACE (("icmp_input: ECHO REQUEST from 0x%lx", saddr));
			if (do_echo (buf, nif, datalen))
				buf_deref (buf, BUF_NORMAL);
			return 0;
		
		case ICMPT_PARAMP:
			TRACE (("icmp_input: PARAMETER PROBLEM from 0x%lx", saddr));
			do_erreport (buf, nif, datalen);
			return -1;
		
		case ICMPT_TIMERQ:
			TRACE (("icmp_input: TIME REQUEST from 0x%lx", saddr));
			if (do_time (buf, nif, datalen)) 
				buf_deref (buf, BUF_NORMAL);
			return 0;
		
		case ICMPT_TIMERP:
			TRACE (("icmp_input: TIME REPLY from 0x%lx", saddr));
			return -1;
		
		case ICMPT_INFORQ:
			TRACE (("icmp_input: INFO REQUEST from 0x%lx", saddr));
			if (do_info (buf, nif, datalen)) 
				buf_deref (buf, BUF_NORMAL);
			return 0;
		
		case ICMPT_INFORP:
			TRACE (("icmp_input: INFO REPLY from 0x%lx", saddr));
			return -1;
		
		case ICMPT_MASKRQ:
			TRACE (("icmp_input: MASK REQUEST from 0x%lx", saddr));
			if (do_mask (buf, nif, datalen)) 
				buf_deref (buf, BUF_NORMAL);
			return 0;
	
		case ICMPT_MASKRP:
			TRACE (("icmp_input: MASK REPLY from 0x%lx", saddr));
			return -1;
	}
	
	return -1;
}

static long
icmp_error (short type, short code, BUF *buf, ulong saddr, ulong daddr)
{
	UNUSED(type);
	UNUSED(code);
	UNUSED(saddr);
	UNUSED(daddr);
	TRACE (("icmp_error: cannot send icmp error message to %lx", daddr));
	buf_deref (buf, BUF_NORMAL);
	return 0;
}

long
icmp_errno (short type, short code)
{
	static short icmp_errors[] =
	{
		ENETUNREACH,	/* ICMPC_NETUR, net unreachable */
		EHOSTUNREACH,	/* ICMPC_HOSTUR, host unreachable */
		ENOPROTOOPT,	/* ICMPC_PROTOUR, protocol unreachable */
		ECONNREFUSED,	/* ICMPC_PORTUR, port unreachable */
		EMSGSIZE,	/* ICMPC_FNDF, frag needed but DF set */
		EOPNOTSUPP,	/* ICMPC_SRCRT, source soute failed */
		ENETUNREACH, /* ICMPC_NET_UNKNOWN, Destination network unknown */
		EHOSTDOWN, /* ICMPC_HOST_UNKNOWN, Destination host unknown */
		ENONET, /* ICMPC_HOST_ISOLATED, Source host isolated */
		ENETUNREACH, /* ICMPC_NET_ANO, Network administratively prohibited */
		EHOSTUNREACH, /* ICMPC_HOST_ANO, Host administratively prohibited */
		ENETUNREACH,	/* ICMPC_NET_UNR_TOS Network unreachable for ToS */
		EHOSTUNREACH,	/* ICMPC_HOST_UNR_TOS, Host unreachable for ToS */
		EHOSTUNREACH, /* ICMPC_PKT_FILTERED, Communication administratively filtered */
		EHOSTUNREACH,	/* ICMPC_PREC_VIOLATION, Host Precedence violation */
		EHOSTUNREACH,	/* ICMP_PREC_CUTOFF, Precedence cut off in effect */
	};
	
	if (type != ICMPT_DSTUR)
		return EOPNOTSUPP;
	
	if ((ushort)code >= sizeof (icmp_errors) / sizeof (*icmp_errors))
	{
		DEBUG (("icmp_errno: %d: no such code", code));
		return EHOSTUNREACH;
	}
	
	return icmp_errors[code];
}

short
icmp_dontsend (short type, BUF *buf)
{
	struct ip_dgram *iph = (struct ip_dgram *) buf->dstart;
	
	if (iph->fragoff & IP_FRAGOFF)
		return 1;
	
	if (iph->proto == IPPROTO_ICMP) switch (type)
	{
		case ICMPT_DSTUR:
		case ICMPT_REDIR:
		case ICMPT_SRCQ:
		case ICMPT_TIMEX:
		case ICMPT_PARAMP:
			return 1;
		case ICMPT_ECHORP:
			/* We explicitly allow ICMP echo requests to
			 * broadcast addresses!
			 */
			return 0;
	}
	
	if (ip_is_brdcst_addr (iph->daddr))
		return 1;
	
	return 0;
}

/*
 * send an ICMP message.
 *
 * the parameters:
 *	buf1 holds the original IP datagram that caused the ICMP request.
 *	buf2 holds optional data specific for ICMP type/code, 0 if none.
 */
long
icmp_send (short type, short code, ulong daddr, BUF *buf1, BUF *buf2)
{
	BUF *nbuf;
	struct icmp_dgram *icmph;
	ulong saddr;
	short datalen;
	
	/* T. Lang:
	 * Since we may answer packets with broadcast addresses as dstaddr
	 * we have to replace such an address by a valid interface address
	 * so we have some new code here...
	 */
	
	if (icmp_dontsend (type, buf1))
	{
		DEBUG (("icmp_send: error message about error message"));
		buf_deref (buf1, BUF_NORMAL);
		if (buf2) buf_deref (buf2, BUF_NORMAL);
		return 0;
	}
	
	/* T. Lang: New - replace a broadcast address in the received packet
	 * by a local addr
	 */
	if (ip_is_brdcst_addr (IP_DADDR (buf1)))
		saddr = ip_local_addr (IP_DADDR (buf1));
	else
		saddr = IP_DADDR (buf1);
	
	if (!ip_is_local_addr (saddr))
		saddr = INADDR_ANY;
	
	nbuf = do_send (type, code, buf1, buf2);
	if (!nbuf)
	{
		buf_deref (buf1, BUF_NORMAL);
		if (buf2) buf_deref (buf2, BUF_NORMAL);
		return 0;
	}
	
	/* T. Lang: New - if destination address is a broadcast address then
	 * reflect datagram to local ip system
	 */
	if (ip_is_brdcst_addr (daddr))
	{
		buf1 = buf_clone (nbuf, BUF_NORMAL);
		if (buf1)
			ip_send (saddr, 0x7F000001L, buf1, IPPROTO_ICMP, 0, 0);
	}
	
	icmph = (struct icmp_dgram *) nbuf->dstart;
	icmph->code = code;
	icmph->type = type;
	icmph->chksum = 0;
	
	datalen = (long) nbuf->dend - (long) nbuf->dstart;
	if (datalen & 1)
		*nbuf->dend = 0;
	icmph->chksum = chksum (icmph, (datalen+1)/2);
	
	ip_send (saddr, daddr, nbuf, IPPROTO_ICMP, 0, 0);
	if (buf2) buf_deref (buf2, BUF_NORMAL);
	return 0;
}

/*
 * build the outgoing ICMP packet from supplied data
 */
static BUF *
do_send (short type, short code, BUF *buf1, BUF *buf2)
{
	BUF *nbuf;
	struct icmp_dgram *icmph;
	
	UNUSED(code);
	switch (type)
	{
		case ICMPT_ECHORP:
		case ICMPT_TIMERP:
		case ICMPT_INFORP:
		case ICMPT_MASKRP:
			buf1->dstart = IP_DATA (buf1);
			return buf1;
		
		case ICMPT_DSTUR:
		case ICMPT_TIMEX:
			/*
			 * Keep only the first 64 bits of the original datagram
			 */
			buf1->dend = IP_DATA (buf1) + 2 * sizeof (long);
			
			nbuf = buf_reserve (buf1, sizeof (*icmph), BUF_RESERVE_START);
			if (!nbuf)
			{
				DEBUG (("do_send: no space for ICMP header"));
				return 0;
			}
			nbuf->dstart -= sizeof (*icmph);
			icmph = (struct icmp_dgram *)nbuf->dstart;
			icmph->u.zero = 0;
			return nbuf;
		
		case ICMPT_REDIR:
			/*
			 * Keep only the first 64 bits of the original datagram
			 */
			buf1->dend = IP_DATA (buf1) + 2 * sizeof (long);
			
			nbuf = buf_reserve (buf1, sizeof (*icmph), BUF_RESERVE_START);
			if (!nbuf)
			{
				DEBUG (("do_send: no space for ICMP header"));
				return 0;
			}
			nbuf->dstart -= sizeof (*icmph);
			icmph = (struct icmp_dgram *)nbuf->dstart;
			memcpy (&icmph->u.redir_gw, buf2->dstart, sizeof (long));
			return nbuf;
		
		case ICMPT_SRCQ:
		case ICMPT_ECHORQ:
		case ICMPT_PARAMP:
		case ICMPT_TIMERQ:
		case ICMPT_INFORQ:
		case ICMPT_MASKRQ:
			break;
	}
	
	TRACE (("do_send: ICMP message type %d not implemented", type));
	return 0;
}

/*
 * handle ICMP unreachable message. We use this to update our routing table
 */
static long
do_unreach (BUF *buf, struct netif *nif, long len)
{
	struct icmp_dgram *icmph;
	struct ip_dgram *iph;
	
	UNUSED(nif);
	if ((ulong)len < sizeof (*icmph) + sizeof (*iph) + 2*sizeof (long))
	{
		DEBUG (("do_unreach: packet too short"));
		return -1;
	}
	
	icmph = (struct icmp_dgram *) IP_DATA (buf);
	iph = (struct ip_dgram *)(icmph+1);
	
	if (icmph->code == ICMPC_HOSTUR || icmph->code == ICMPC_NETUR)
	{
		struct route *rt = route_get (iph->daddr);
		if (rt)
		{
			DEBUG (("do_unreach: bad route to %lx", rt->net));
			rt->flags |= RTF_REJECT;
			route_deref (rt);
		}
	}
	
	return 0;
}

/*
 * Handle ICMP redirect message. We use this to update our routing table.
 */
static long
do_redir (BUF *b, struct netif *nif, long len)
{
	struct icmp_dgram *icmph;
	struct ip_dgram *iph;
	struct route *rt;
	long mask;

	if ((ulong)len < sizeof (*icmph) + sizeof (*iph) + 2*sizeof (long))
	{
		DEBUG (("do_redir: packet too short"));
		return -1;
	}
	
	icmph = (struct icmp_dgram *) IP_DATA (b);
	iph = (struct ip_dgram *)(icmph+1);
	
	if (icmph->code != ICMPC_NETRD && icmph->code != ICMPC_HOSTRD)
		return 0;
	/*
    	 * validate new gw address
	 */
	rt = route_get (icmph->u.redir_gw);
	if (!rt || (rt->flags & RTF_GATEWAY))
	{
		DEBUG (("do_redir: bad new gway 0x%lx", icmph->u.redir_gw));
		if (rt) route_deref (rt);
		return 0;
	}
	route_deref (rt);
	
	/*
	 * validate sending gateway
	 */
	rt = route_get (iph->daddr);
	if (!rt || !(rt->flags & RTF_GATEWAY) || rt->gway != IP_SADDR (b))
	{
		DEBUG (("do_redir: bad sending gway 0x%lx",
		IP_SADDR (b)));
		if (rt) route_deref (rt);
		return 0;
	}
	route_deref (rt);
	
	switch (icmph->code)
	{
		case ICMPC_NETRD:
			/*
			 * XXX: we should kill all routes to nets that
			 * are subnets of the destination net.
			 */
			mask = ip_netmask (iph->daddr);
			DEBUG (("do_redir: net 0x%lx over gw 0x%lx",
				iph->daddr & mask, icmph->u.redir_gw));
			route_add (nif, iph->daddr, mask, icmph->u.redir_gw,
				RTF_DYNAMIC|RTF_MODIFIED|RTF_GATEWAY,
				RT_TTL, 1);
			break;
		
		case ICMPC_HOSTRD:
			DEBUG (("do_redir: host 0x%lx over gw 0x%lx",
				iph->daddr, icmph->u.redir_gw));
			route_add (nif, iph->daddr, 0xffffffffL, icmph->u.redir_gw,
				RTF_DYNAMIC|RTF_MODIFIED|RTF_HOST|RTF_GATEWAY,
				RT_TTL, 1);
			break;
	}
	
	return 0;
}

/*
 * Handle ICMP time request. Used to obtain hosts time
 */
static long
do_time (BUF *b, struct netif *nif, long len)
{
	struct icmp_dgram *icmph;
	long tm;
	
	UNUSED(nif);
	if ((ulong)len < sizeof (*icmph) + 3*sizeof (long))
	{
		DEBUG (("do_redir: packet too short"));
		return -1;
	}
	icmph = (struct icmp_dgram *) IP_DATA (b);
	
	/*
	 * Time in ms since midnight UCT. What if the
	 * system clock is not set to UCT? Perhaps we should
	 * always set the high order bit to indicate a nonstandard
	 * time as RFC 792 says?
	 */
	tm = (unixtime (t_gettime (), t_getdate ()) % (60*60*24L)) * 1000L;
	icmph->data.longs[1] = icmph->data.longs[2] = tm;
	icmp_send (ICMPT_TIMERP, 0, IP_SADDR (b), b, 0);
	
	return 0;
}

/*
 * Handle ICMP echo request. Used by ping
 */
static long
do_echo (BUF *b, struct netif *nif, long len)
{
	UNUSED(nif);
	if ((ulong)len < sizeof (struct icmp_dgram))
	{
		DEBUG (("do_echo: packet too short"));
		return -1;
	}
	
	icmp_send (ICMPT_ECHORP, 0, IP_SADDR (b), b, 0);
	
	return 0;
}

/*
 * handle ICMP mask request. Used to obtain netmask of an IP address.
 */
static long
do_mask (BUF *b, struct netif *nif, long len)
{
	struct icmp_dgram *icmph;
	struct ifaddr *ifa;
	
	if ((ulong)len < sizeof (*icmph) + sizeof (long))
	{
		DEBUG (("do_mask: packet too short"));
		return -1;
	}
	icmph = (struct icmp_dgram *) IP_DATA (b);

	ifa = if_af2ifaddr (nif, AF_INET);
	if (!ifa)
	{
		DEBUG (("do_mask: if_af2ifaddr returned NULL"));
		return -1;
	}
	
	icmph->data.longs[0] = ifa->subnetmask;
	icmp_send (ICMPT_MASKRP, 0, IP_SADDR (b), b, 0);
	
	return 0;
}

/*
 * handle ICMP information request. Used to obtain IP address.
 */
static long
do_info (BUF *b, struct netif *nif, long len)
{
	UNUSED(nif);
	if ((ulong)len < sizeof (struct icmp_dgram))
	{
		DEBUG (("do_info: packet too short"));
		return -1;
	}
	
	icmp_send (ICMPT_INFORP, 0, IP_SADDR (b), b, 0);
	
	return 0;
}

/*
 * report ICMP error to higher level protocols
 */
static long
do_erreport (BUF *b, struct netif *nif, long len)
{
	struct in_ip_ops *p;
	struct icmp_dgram *icmph;
	struct ip_dgram *iph;
	BUF *buf;
	
	UNUSED(nif);
	if ((ulong)len < sizeof (*icmph) + sizeof (*iph) + 2*sizeof (long))
	{
		DEBUG (("do_erreport: packet too short"));
		return -1;
	}
	buf = buf_clone (b, BUF_NORMAL);
	if (!buf)
		return 0;
	
	/*
	 * strip IP header
	 */
	buf->dstart = IP_DATA (buf);
	icmph = (struct icmp_dgram *) buf->dstart;
	
	/*
	 * strip ICMP header
	 */
	buf->dstart += sizeof (struct icmp_dgram);
	iph = (struct ip_dgram *)buf->dstart;
	
	for (p = allipprotos; p; p = p->next)
	{
		if (p->proto == iph->proto)
		{
			(*p->error)(icmph->type, icmph->code, buf,
				iph->saddr, iph->daddr);
			return 0;
		}
	}
	DEBUG (("icmp_erreport: %d: no such proto", iph->proto));
	buf_deref (buf, BUF_NORMAL);
	return -1;
}

void
icmp_init (void)
{
	ip_register (&icmp_ops);
}
