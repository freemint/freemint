/*
 *	Simple IP implementation (missing option processing completely).
 *
 *	01/21/94, kay roemer.
 *      07/01/99, masqerading code by Mario Becroft.
 */

# include "ip.h"

# include "icmp.h"
# include "in.h"
# include "inet.h"
# include "inetutil.h"
# include "masquerade.h"

# include "util.h"


static BUF *	ip_brdcst_copy	(BUF *, struct netif *, struct route *, short);
static long	ip_do_opts	(struct ip_dgram *);

struct in_ip_ops *allipprotos = NULL;

/*
 * Return the local IP address which is 'nearest' to `dstaddr'. Note
 * that one machine can have several IP addresses on several networks,
 * so this routine must know `dstaddr'.
 */
ulong
ip_local_addr (ulong dstaddr)
{
	struct ifaddr *ifa = NULL;
	struct route *rt;

	if (dstaddr == INADDR_ANY)
		ifa = if_af2ifaddr (rt_primary.nif, AF_INET);
	else
	{
		rt = route_get (dstaddr);
		if (rt)
		{
			ifa = if_af2ifaddr (rt->nif, AF_INET);
			route_deref (rt);
		}
	}
	
	return ifa ? SIN (&ifa->addr)->sin_addr.s_addr : INADDR_ANY;
}

short
ip_is_brdcst_addr (ulong addr)
{
	struct netif *nif;
	struct ifaddr *ifa;
	
	if (addr == INADDR_ANY || addr == INADDR_BROADCAST)
		return 1;
	
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		ifa = if_af2ifaddr (nif, AF_INET);
		if (!ifa) continue;
		
		if (addr == ifa->net ||
		    addr == ifa->subnet ||
		    addr == ifa->net_broadaddr)
			return 1;
		
		if (nif->flags & IFF_BROADCAST &&
		    addr == SIN (&ifa->ifu.broadaddr)->sin_addr.s_addr)
			return 1;
	}
	
	return 0;
}
 
short
ip_is_local_addr (ulong addr)
{
	struct netif *nif;
	struct ifaddr *ifa;
	
	if (addr == INADDR_LOOPBACK)
		return 1;
	
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		ifa = if_af2ifaddr (nif, AF_INET);
		if (ifa && addr == SIN (&ifa->addr)->sin_addr.s_addr)
			return 1;
	}
	
	return 0;
}

short
ip_chk_addr (ulong addr, struct route *rt)
{
	struct ifaddr *ifa;
	
	if (addr == INADDR_ANY || addr == INADDR_BROADCAST)
		return IPADDR_BRDCST;
	
	if ((addr & IN_CLASSA_NET) == 0x7f000000ul)
		return IPADDR_LOCAL;
	
	if (rt->flags & RTF_LOCAL)
		return IPADDR_LOCAL;
	
	ifa = if_af2ifaddr (rt->nif, AF_INET);
	if (ifa)
	{
		if (addr == SIN (&ifa->addr)->sin_addr.s_addr)
			return IPADDR_LOCAL;
		
		if (addr == ifa->net ||
		    addr == ifa->subnet ||
		    addr == ifa->net_broadaddr)
			return IPADDR_BRDCST;
		
		if (rt->nif->flags & IFF_BROADCAST &&
		    addr == SIN (&ifa->ifu.broadaddr)->sin_addr.s_addr)
			return IPADDR_BRDCST;
	}
	
	if (!IN_CLASSA (addr) && !IN_CLASSB (addr) && !IN_CLASSC (addr))
		return IPADDR_BADCLASS;

	return IPADDR_NONE;
}

/*
 * Check whether foreign IP address `foreign' matches local IP address
 * `local'. Note that `foreign' might be a broadcast address that matches
 * our network and thus our machines address.
 * This routine requires the subnet portion to be a multiple of 8 bits to
 * recognice subnet broadcasts.
 */
short
ip_same_addr (ulong local, ulong foreign)
{
	ulong mask;
	
	if (local == foreign || !local || !foreign || !~foreign)
		return 1;
	
	for (mask = 0xffffff00ul; mask; mask <<= 8)
	{
		if ((local ^ foreign) & mask)
			continue;
		foreign &= ~mask;
		return (foreign == 0 || foreign == ~mask);
	}
	
	return 0;
}

ulong
ip_dst_addr (ulong addr)
{
	struct ifaddr *ifa = 0;
	
	if (addr == INADDR_ANY)
	{
		/*
		 * Use address of primary interface
		 */
		ifa = if_af2ifaddr (rt_primary.nif, AF_INET);

		DEBUG (("ip_dst_addr: nif = %s any", ifa ? ifa->ifp->name : "??"));

		return ifa ? SIN (&ifa->addr)->sin_addr.s_addr
			   : INADDR_LOOPBACK;
	}
	else if (addr == INADDR_BROADCAST)
	{
		/*
		 * Use broadcast address of primary interface
		 */
		if (rt_primary.nif->flags & IFF_BROADCAST)
			ifa = if_af2ifaddr (rt_primary.nif, AF_INET);

		DEBUG (("ip_dst_addr: nif = %s brcst", ifa ? ifa->ifp->name : "??"));

		return ifa ? SIN (&ifa->ifu.broadaddr)->sin_addr.s_addr
			   : INADDR_BROADCAST;
	}
	
	return addr;
}	

ulong
ip_netmask (ulong addr)
{
	struct netif *nif;
	struct ifaddr *ifa;
	ulong netmask;
	
	if (IN_CLASSA (addr))
		netmask = IN_CLASSA_NET;
	else if (IN_CLASSB (addr))
		netmask = IN_CLASSB_NET;
	else if (IN_CLASSC (addr))
		netmask = IN_CLASSC_NET;
	else
		return 0;
	
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		ifa = if_af2ifaddr (nif, AF_INET);
		if (ifa && ifa->net == (addr & netmask))
			return ifa->subnetmask;
	}
	
	return netmask;
}

short
ip_priority (short pri, uchar tos)
{
	short newpri;
	
	newpri = (tos & IPTOS_LOWDELAY) ? 1 : 0;
	return MAX (pri, newpri);
}

void
ip_register (struct in_ip_ops *proto)
{
	proto->next = allipprotos;
	allipprotos = proto;
}

static BUF *
ip_brdcst_copy (BUF *buf, struct netif *nif, struct route *rt, short addrtype)
{
	if (addrtype != IPADDR_BRDCST || nif == rt->nif)
		return 0;
	
	return buf_clone (buf, BUF_NORMAL);
}

/*
 * Process the IP options in `iph'. For now, do nothing.
 */
static long
ip_do_opts (struct ip_dgram *iph)
{
	return 0;
}

/*
 * Output a fully built IP datagram
 */
long
ip_output (BUF *buf)
{
	struct ip_dgram *iph = (struct ip_dgram *) buf->dstart;
	struct route *rt;
	short addrtype;
	long r;
	BUF *nbuf2;
	
	/*
	 * Route datagram to next interface
	 */
	rt = route_get (iph->daddr);
	if (!rt)
	{
		DEBUG (("ip_output: not route to dst %lx", iph->daddr));
		icmp_send (ICMPT_DSTUR, ICMPC_NETUR, iph->saddr, buf, 0);
		return ENETUNREACH;
	}
	
	addrtype = ip_chk_addr (iph->daddr, rt);
	if (addrtype == IPADDR_BADCLASS)
	{
		DEBUG (("ip_output: Dst addr not in class A/B/C"));
		buf_deref (buf, BUF_NORMAL);
		route_deref (rt);
		return EADDRNOTAVAIL;
	}
	
	/*
	 * Fill in the source of the datagram (if not already done at
	 * higher level)
	 */
	if (iph->saddr == INADDR_ANY)
	{
		struct ifaddr *ifa = if_af2ifaddr (rt->nif, AF_INET);
		if (!ifa)
		{
			DEBUG (("if_output: chosen net if has no inet addr"));
			buf_deref (buf, BUF_NORMAL);
			route_deref (rt);
			return EADDRNOTAVAIL;
		}
		iph->saddr = SIN (&ifa->addr)->sin_addr.s_addr;
	}
	
	nbuf2 = ip_brdcst_copy (buf, rt->nif, rt, addrtype);
	
	r = ip_frag (buf, rt->nif,
		rt->flags & RTF_GATEWAY ? rt->gway : iph->daddr,
		addrtype == IPADDR_BRDCST);
	
	if (nbuf2)
	{
		route_deref (rt);
		rt = route_get (((struct ip_dgram *)(nbuf2->dstart))->daddr = ip_local_addr(((struct ip_dgram *)(nbuf2->dstart))->daddr));
		ip_frag (nbuf2, rt->nif,
			((struct ip_dgram *)(nbuf2->dstart))->daddr,
			0);
	}
	
	route_deref (rt);
	return r;
}

short ip_dgramid = 0;
static struct ip_options def_opts = { 0, IP_DEFAULT_TTL, IP_DEFAULT_TOS, 0 };

long
ip_send (ulong saddr, ulong daddr, BUF *buf, short proto, short flags, struct ip_options *_opts)
{
	BUF *nbuf, *nbuf2;
	struct ip_dgram *iph;
	struct ip_options *opts = _opts ? _opts : &def_opts;
	struct route *rt;
	short addrtype;
	long r;
	
	/*
	 * Allocate and fill in IP header
	 */
	nbuf = buf_reserve (buf, sizeof (*iph), BUF_RESERVE_START);
	if (!nbuf)
	{
		DEBUG (("ip_send: no space for IP header"));
		buf_deref (buf, BUF_NORMAL);
		return ENOMEM;
	}
	
	nbuf->dstart -= sizeof (*iph);
	iph = (struct ip_dgram *)nbuf->dstart;
	iph->version = IP_VERSION;
	iph->hdrlen  = sizeof (*iph) / sizeof (long);
	iph->tos     = opts->tos;
	iph->length  = (short)((long)nbuf->dend - (long)nbuf->dstart);
	iph->id      = ip_dgramid++;
	iph->fragoff = 0;
	iph->ttl     = opts->ttl;
	iph->proto   = proto;
	iph->saddr   = saddr;
	iph->daddr   = daddr;
	iph->chksum  = 0;
	
	nbuf->info = ip_priority (opts->pri, iph->tos);

	/*
	 * Route datagram to next interface
	 */
	rt = route_get (daddr);
	if (!rt)
	{
		DEBUG (("ip_send: no route to dst %lx", daddr));
		icmp_send (ICMPT_DSTUR, ICMPC_NETUR, saddr, nbuf, 0);
		return ENETUNREACH;
	}

	addrtype = ip_chk_addr (daddr, rt);
	if (addrtype == IPADDR_BADCLASS)
	{
		DEBUG (("ip_send: dst addr not in class A/B/C"));
		buf_deref (nbuf, BUF_NORMAL);
		route_deref (rt);
		return EADDRNOTAVAIL;
	}
	
	/*
	 * Check if broadcasts allowed
	 */
	if (addrtype == IPADDR_BRDCST && !(flags & IP_BROADCAST))
	{
		DEBUG (("ip_send: broadcasts not allowed"));
		buf_deref (nbuf, BUF_NORMAL);
		return EACCES;
	}
	
	/*
	 * Fill in the source of the datagram (if not already done at
	 * higher level)
	 */
	if (saddr == INADDR_ANY)
	{
		struct ifaddr *ifa = if_af2ifaddr (rt->nif, AF_INET);
		if (!ifa)
		{
			DEBUG (("if_send: nif %s has no ifaddr", rt->nif->name));
			buf_deref (nbuf, BUF_NORMAL);
			route_deref (rt);
			return EADDRNOTAVAIL;
		}
		iph->saddr = SIN (&ifa->addr)->sin_addr.s_addr;
	}
	
	nbuf2 = ip_brdcst_copy (nbuf, rt->nif, rt, addrtype);
	
	r = ip_frag (nbuf, rt->nif, rt->flags & RTF_GATEWAY ? rt->gway : daddr,
		addrtype == IPADDR_BRDCST);
	
	if (nbuf2)
	{
		route_deref (rt);
		rt = route_get (((struct ip_dgram *)(nbuf2->dstart))->daddr = ip_local_addr(((struct ip_dgram *)(nbuf2->dstart))->daddr));
		ip_frag (nbuf2, rt->nif,
			((struct ip_dgram *)(nbuf2->dstart))->daddr,
			0);
	}
	
	route_deref (rt);
	return r;
}

void
ip_input (struct netif *nif, BUF *buf)
{
	struct ip_dgram *iph = (struct ip_dgram *) buf->dstart;
	struct route *rt;
	short addrtype;
	short pktlen;
	
	/*
	 * Validate incoming datagram
	 */
	pktlen = (long)buf->dend - (long)buf->dstart;
	
	if (pktlen < IP_MINLEN || pktlen != iph->length)
	{
		DEBUG (("ip_input: invalid packet length"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	if (chksum (iph, iph->hdrlen * sizeof (short)))
	{
		DEBUG (("ip_input: bad checksum"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	if (iph->version != IP_VERSION)
	{
		DEBUG (("ip_input: %d: unsupp. IP version", iph->version));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	/*
	 * Process IP options
	 */
	if (ip_do_opts (iph))
	{
		DEBUG (("ip_input: bad IP options"));
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	
	/* 07/01/99 MB */
	buf = masq_ip_input (nif, buf);
	if (!buf)
		return;
	
	/*
	 * Route datagram to next interface
	 */
	rt = route_get (iph->daddr);
	if (!rt)
	{
		DEBUG (("ip_input: not route to dst %lx", iph->daddr));
		icmp_send (ICMPT_DSTUR, ICMPC_NETUR, iph->saddr, buf, 0);
		return;
	}
	
	addrtype = ip_chk_addr (iph->daddr, rt);
	if (addrtype == IPADDR_BADCLASS)
	{
		DEBUG (("ip_input: Dst addr not in class A/B/C"));
		buf_deref (buf, BUF_NORMAL);
		route_deref (rt);
		return;
	}
	
	/*
	 * Check if the datagram is destined to this interface. If so send
	 * the datagram to the local software.
	 */
	if (addrtype == IPADDR_LOCAL || addrtype == IPADDR_BRDCST)
	{
		struct in_ip_ops *p;
		BUF *buf2;
		
		/*
		 * GATEWAY: If datagram is broadcast to a net we are
		 * directly connected to, we must send the datagram to
		 * the local software AND forward it to the network.
		 * buf2 will hold a copy if copying is necessary or zero
		 * if not.
		 */
		buf2 = ip_brdcst_copy (buf, nif, rt, addrtype);
		
		/*
		 * If the protocol input function returns 0, then it has
		 * taken over the packet and we break out of the loop.
		 * If the function returns nonzero then go on.
		 * Because the RAW handler is the last in the chain it
		 * will get all the packets the other protocols don't want.
		 */
		if ((buf = ip_defrag (buf)))
		{
		    for (p = allipprotos; p; p = p->next)
		    {
			if ((p->proto == iph->proto || p->proto == IPPROTO_RAW)
			    && !(*p->input) (nif, buf, iph->saddr, iph->daddr))
				break;
		    }
		    if (!p)
		    {
			DEBUG (("ip_input: %d: No such proto", iph->proto));
			icmp_send (ICMPT_DSTUR,ICMPC_PROTOUR,iph->saddr,buf,0);
		    }
		}
		buf = buf2;
		if (!buf)
		{
			route_deref (rt);
			return;
		}
	}
	
# ifdef DONT_FORWARD
	buf_deref (buf, BUF_NORMAL);
	route_deref (buf, BUF_NORMAL);
# else
	KAYDEBUG (("ip_input: forwarding to dst 0x%lx from 0x%lx",
		iph->daddr, iph->saddr));
	/*
	 * See if the packets times out
	 */
	if (--iph->ttl <= 0)
	{
		DEBUG (("ip_input: ttl exeeded"));
		icmp_send (ICMPT_TIMEX, ICMPC_TTLEX, iph->saddr, buf, 0);
		route_deref (rt);
		return;
	}
	
	/*
	 * Send ICMP redirects if necessary
	 */
	if (rt->nif == nif)
	{
		BUF *gwbuf;
		DEBUG (("ip_input: sending redirect"));
		if (rt->flags & RTF_GATEWAY &&
		    (gwbuf = buf_alloc (sizeof (long), 0, BUF_NORMAL)))
		{
			/*
			 * gwbuf holds the new gateway. both buf and
			 * gwbuf must not be touched after icmp_send ()
			 */
			memcpy (gwbuf->dstart, &rt->gway, sizeof (long));
			gwbuf->dend += sizeof (long);
			icmp_send (ICMPT_REDIR,
				(rt->flags & RTF_HOST)
					? ICMPC_HOSTRD
					: ICMPC_NETRD,
				iph->saddr,
				buf, gwbuf);
		}
		else
			buf_deref (buf, BUF_NORMAL);
		route_deref (rt);
		return;
	}
	
	/*
	 * Set output priority
	 */
	buf->info = ip_priority (0, iph->tos);
	
	ip_frag (buf, rt->nif, rt->flags & RTF_GATEWAY ? rt->gway : iph->daddr,
		addrtype == IPADDR_BRDCST);
	
	route_deref (rt);
# endif /* DONT_FORWARD */
}	

/*
 * Strip off the options from the IP header `iph' which should only be
 * present in the first fragment and update the `hdrlen' field to
 * represent the new header length.
 * 07/01/99 MB make functions public
 */
long
frag_opts (struct ip_dgram *iph, long optlen)
{
	uchar *cp, len, type;
	long i;
	
	for (cp = iph->data, i = 0; i < optlen; )
	{
		type = cp[i];
		switch (type & IPOPT_TYPE)
		{
			case IPOPT_EOL:
				i++;
				goto done;
			
			case IPOPT_NOP:
				i++;
				break;
			
			default:
				len = cp[i+1];
				if (type & IPOPT_COPY)
					i += len;
				else
				{
					memcpy (&cp[i], &cp[i+len], optlen-i-len);
					optlen -= len;
				}
				break;
		}
	}
done:
	while (i & 0x3)
		cp[i++] = IPOPT_NOP;
	i += IP_MINLEN;
	iph->hdrlen = i / sizeof (long);
	
	return i;
}

long
ip_frag (BUF *buf, struct netif *nif, ulong nexthop, short isbrcst)
{
	struct ip_dgram *fragiph, *iph = (struct ip_dgram *)buf->dstart;
	long fraglen, datalen, offset, fragoff, hdrlen, todo, r;
	BUF *fragbuf;
	char *data;
	
	if (iph->length <= nif->mtu)
	{
		iph->chksum = 0;
		iph->chksum = chksum (iph, iph->hdrlen * sizeof (short));
		DEBUG (("ip_frag: short enough -> if_send()"));
		return if_send (nif, buf, nexthop, isbrcst);
	}
	
	fragoff = iph->fragoff;
	hdrlen = iph->hdrlen * sizeof (long);
	fraglen = (nif->mtu - hdrlen) & ~7;
	datalen = iph->length - hdrlen;
	data = IP_DATA (buf);
	offset = 0;
	
	if (fragoff & IP_DF)
	{
		DEBUG (("ip_frag: need fragmentation, but DF set"));
		icmp_send (ICMPT_DSTUR, ICMPC_FNDF, iph->saddr, buf, 0);
		return EOPNOTSUPP;
	}
	
	if ((fragoff & IP_FRAGOFF) + datalen/8 > IP_FRAGOFF)
	{
		DEBUG (("ip_frag: datagram to long"));
		buf_deref (buf, BUF_NORMAL);
		return EINVAL;
	}
	
	while (datalen > 0)
	{
		if (datalen > fraglen)
		{
			todo = fraglen;
			fragbuf = buf_alloc (todo + hdrlen, 0, BUF_NORMAL);
			if (!fragbuf)
			{
				DEBUG (("ip_frag: out of bufs"));
				buf_deref (buf, BUF_NORMAL);
				return ENOMEM;
			}
			memcpy (fragbuf->dend, iph, hdrlen);
			fragbuf->dend += hdrlen;
		}
		else
		{
			todo = datalen;
			buf->dend = IP_DATA (buf);
			fragbuf = buf;
		}
		memcpy (fragbuf->dend, &data[offset], todo);
		fragbuf->dend += todo;
		fragbuf->info = buf->info;
		
		fragiph = (struct ip_dgram *)fragbuf->dstart;
		fragiph->length = hdrlen + todo;
		fragiph->fragoff += offset/8;
		
		datalen -= todo;
		if (datalen > 0) fragiph->fragoff |= IP_MF;
		
		fragiph->chksum = 0;
		fragiph->chksum = chksum (fragiph, hdrlen/2);
		r = if_send (nif, fragbuf, nexthop, isbrcst);
		if (r != 0)
		{
			DEBUG (("ip_frag: if_send failed with %ld", r));
			if (fragbuf != buf)
				buf_deref (buf, BUF_NORMAL);
			return r;
		}
		
		if (offset == 0 && hdrlen > IP_MINLEN)
		{
			/*
			 * after the first fragment has been sent,
			 * we strip off the options of the original
			 * IP header that should only be present in
			 * the first fragment.
			 */
			hdrlen = frag_opts (iph, hdrlen - IP_MINLEN);
		}
		offset += todo;
	}
	
	return 0;
}

struct fragment allfrags[IPFRAG_HEADS];

void
frag_delete (struct fragment *frag)
{
	BUF *buf, *next;
	
	buf = frag->buf;
	frag->buf = 0;
	for (; buf; buf = next)
	{
		next = buf->link3;
		buf_deref (buf, BUF_NORMAL);
	}
	
	event_del (&frag->tmout);
}

void
frag_timeout (long arg)
{
	struct fragment *frag = (struct fragment *)arg;
	
	DEBUG (("frag_timeout: reassambly from id %x saddr 0x%lx timed out",
		frag->id, frag->saddr));
	
	icmp_send (ICMPT_TIMEX, ICMPC_FRAGEX, frag->saddr, frag->buf, 0);
	frag->buf = frag->buf->link3;
	frag_delete (frag);
}

BUF *
frag_pullup (struct fragment *frag)
{
	struct ip_dgram *iph;
	BUF *nbuf, *oldbuf;
	long length, offset;
	
	if (frag->totlen == 0 || frag->totlen != frag->curlen)
		return 0;
	
	TRACE (("frag_pullup: reassembling datagram from id %x saddr 0x%lx",
		frag->id, frag->saddr));
	
	iph = (struct ip_dgram *) frag->buf->dstart;
	length = iph->hdrlen * sizeof (long);
	
	nbuf = buf_alloc (length + frag->totlen, 0, BUF_NORMAL);
	if (!nbuf)
	{
		DEBUG (("frag_pullup: no space for new buf"));
		frag_delete (frag);
		return 0;
	}
	memcpy (nbuf->dend, iph, length);
	nbuf->dend += length;
	
	for (offset = 0; (oldbuf = frag->buf);)
	{
		iph = (struct ip_dgram *)oldbuf->dstart;
		if (offset != (iph->fragoff & IP_FRAGOFF)*8)
		{
			DEBUG (("frag_pullup: maleformed fragment list"));
			frag_delete (frag);
			return 0;
		}
		length = iph->length - iph->hdrlen * sizeof (long);
		memcpy (nbuf->dend, IP_DATA (oldbuf), length);
		nbuf->dend += length;
		offset += length;
		frag->buf = oldbuf->link3;
		buf_deref (oldbuf, BUF_NORMAL);
	}
	iph = (struct ip_dgram *)nbuf->dstart;
	iph->length = offset;
	iph->fragoff = 0;
	
	event_del (&frag->tmout);
	return nbuf;
}

void
frag_insert (struct fragment *frag, BUF *buf)
{
# define OFFSET(x) (((struct ip_dgram *)(x)->dstart)->fragoff & IP_FRAGOFF)
	struct ip_dgram *iph = (struct ip_dgram *) buf->dstart;
	BUF **prev, *curr;
	long offset, datalen;
	
	prev = &frag->buf;
	curr = frag->buf;
	offset = iph->fragoff & IP_FRAGOFF;
	for (; curr; prev = &curr->link3, curr = curr->link3)
	{
		if (OFFSET (curr) > offset)
			break;
		else if (OFFSET (curr) == offset)
		{
			DEBUG (("frag_insert: dropping duplicate fragment"));
			buf_deref (buf, BUF_NORMAL);
			return;
		}
	}
	buf->link3 = curr;
	*prev = buf;
	
	datalen = iph->length - iph->hdrlen * sizeof (long);
	frag->curlen += datalen;
	if (!curr && !(iph->fragoff & IP_MF))
		frag->totlen = offset*8 + datalen;
# if 0
	if (event_delta (&frag->tmout) < iph->ttl*(1000/EVTGRAN))
		event_reset (&frag->tmout, iph->ttl*(1000/EVTGRAN));
# endif
}

BUF *
ip_defrag (BUF *buf)
{
	struct ip_dgram *iph = (struct ip_dgram *) buf->dstart;
	struct fragment *frag, *free;
	short i;
	
	if ((iph->fragoff & (IP_MF|IP_FRAGOFF)) == 0)
		return buf;
	
	free = 0;
	for (i = IPFRAG_HEADS, frag = allfrags; i > 0; --i, frag++)
	{
		if (frag->buf)
		{
			if (iph->id == frag->id && iph->saddr == frag->saddr)
			{
				frag_insert (frag, buf);
				return frag_pullup (frag);
			}
		} else	free = frag;
	}
	
	if (free)
	{
		free->totlen = 0;
		free->curlen = 0;
		free->id = iph->id;
		free->saddr = iph->saddr;
		event_add (&free->tmout,IPFRAG_TMOUT,frag_timeout,(long)free);
		frag_insert (free, buf);
	}
	else
	{
		DEBUG (("ip_defrag: no more fragment slots, dropping dgram"));
		buf_deref (buf, BUF_NORMAL);
	}
	
	return 0;
}

long
ip_setsockopt (struct ip_options *opts, short level, short optname, char *optval, long optlen)
{
	if (level != IPPROTO_IP)
		return EOPNOTSUPP;
	
	switch (optname)
	{
		case IP_OPTIONS:
			break;
		
		case IP_HDRINCL:
			if (optlen != sizeof (long) || !optval)
				return EINVAL;
			opts->hdrincl = *(long *) optval ? 1 : 0;
			return 0;
		
		case IP_TOS:
			if (optlen != sizeof (long) || !optval)
				return EINVAL;
			opts->tos = (char) *(long *) optval;
			return 0;
		
		case IP_TTL:
			if (optlen != sizeof (long) || !optval)
				return EINVAL;
			opts->ttl = (char) *(long *) optval;
			return 0;
		
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
		case IP_RETOPTS:
			break;
	}
	
	return EOPNOTSUPP;
}

long
ip_getsockopt (struct ip_options *opts, short level, short optname, char *optval, long *optlen)
{
	if (level != IPPROTO_IP)
		return EOPNOTSUPP;
	
	switch (optname)
	{
		case IP_OPTIONS:
			break;
		
		case IP_HDRINCL:
			if (!optval || !optlen || *optlen < sizeof (long))
				return EINVAL;
			*(long *) optval = !!opts->hdrincl;
			*optlen = sizeof (long);
			return 0;
		
		case IP_TOS:
			if (!optval || !optlen || *optlen < sizeof (long))
				return EINVAL;
			*(long *) optval = (ulong) opts->tos;
			*optlen = sizeof (long);
			return 0;
		
		case IP_TTL:
			if (!optval || !optlen || *optlen < sizeof (long))
				return EINVAL;
			*(long *) optval = (ulong) opts->ttl;
			*optlen = sizeof (long);
			return 0;
		
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
		case IP_RETOPTS:
			break;
	}
	
	return EOPNOTSUPP;
}
