/*
 *	Device independent network interface routines.
 *
 *	02/24/94, kay roemer.
 */

# include "if.h"

# include "arp.h"
# include "bpf.h"
# include "inetutil.h"
# include "ip.h"
# include "loopback.h"
# include "route.h"

# include "util.h"

# include <mint/asm.h>
# include <mint/file.h>


/*
 * Pending timeout
 */
static TIMEOUT *tmout = 0;

/*
 * List of all registered interfaces, loopback and primary interface.
 */
struct netif *allinterfaces, *if_lo;

/*
 * Stack used while processing incoming packets
 */
char stack[8192];


short
if_enqueue (struct ifq *q, BUF *buf, short pri)
{
	register ushort sr;
	
	sr = spl7 ();
	
	if (q->qlen >= q->maxqlen)
	{
		/*
		 * queue full, dropping packet
		 */
		buf_deref (buf, BUF_ATOMIC);
		
		spl (sr);
		return ENOMEM;
	}
	else
	{
		if ((ushort) pri >= IF_PRIORITIES)
			pri = IF_PRIORITIES-1;
		
		buf->link3 = NULL;
		if (q->qlast[pri])
		{
			q->qlast[pri]->link3 = buf;
			q->qlast[pri] = buf;
		}
		else
			q->qlast[pri] = q->qfirst[pri] = buf;
		
		q->qlen++;
	}
	
	spl (sr);
	
	return 0;
}

short
if_putback (struct ifq *q, BUF *buf, short pri)
{
	register ushort sr;
	
	sr = spl7 ();
	
	if (q->qlen >= q->maxqlen)
	{
		/*
		 * queue full, dropping packet
		 */
		buf_deref (buf, BUF_ATOMIC);
		
		spl (sr);
		return ENOMEM;
	}
	else
	{
		if ((ushort)pri >= IF_PRIORITIES)
			pri = IF_PRIORITIES-1;
		
		buf->link3 = q->qfirst[pri];
		q->qfirst[pri] = buf;
		if (!q->qlast[pri])
			q->qlast[pri] = buf;
		
		q->qlen++;
	}
	
	spl (sr);
	
	return 0;
}

# define GET_QUEUE(q)    (((q)->curr >> IF_PRIORITY_BITS) & (IF_PRIORITIES-1))
# define SET_QUEUE(q, n) ((q)->curr = (n) << IF_PRIORITY_BITS)
# define INC_QUEUE(q, n) ((q)->curr += (IF_PRIORITIES - (n)))

BUF *
if_dequeue (struct ifq *q)
{
	register BUF *buf = NULL;
	register ushort sr;
	register short i;
	
	sr = spl7 ();
	
	if (q->qlen > 0)
	{
		register short que;
		
		que = GET_QUEUE (q);
		for (i = IF_PRIORITIES; i > 0; --i)
		{
			if ((buf = q->qfirst[que]))
			{
				q->qfirst[que] = buf->link3;
				if (!buf->link3)
					q->qlast[que] = NULL;
				
				if (i < IF_PRIORITIES)
					SET_QUEUE (q, que);
				
				INC_QUEUE (q, que);
				q->qlen--;
				
				break;
			}
			que = (que+1) & (IF_PRIORITIES-1);
		}
	}
	
	spl (sr);
	
	return buf;
}

void
if_flushq (struct ifq *q)
{
	register ushort sr;
	register short i;
	
	sr = spl7();
	
	for (i = 0; i < IF_PRIORITIES; ++i)
	{
		register BUF *buf;
		register BUF *next;
		
		for (buf = q->qfirst[i]; buf; buf = next)
		{
			next = buf->link3;
			buf_deref (buf, BUF_NORMAL);
		}
		q->qfirst[i] = q->qlast[i] = 0;
	}
	q->qlen = 0;
	
	spl (sr);
}

static void
if_doinput (long proc)
{
	register struct netif *nif;
	register char *sp;
	short comeagain = 0;
	
	tmout = 0;
	sp = setstack (stack + sizeof (stack));
	
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		register short todo;
		
		if ((nif->flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
			continue;
		
		for (todo = nif->rcv.maxqlen; todo; --todo)
		{
			register BUF *buf;
			
			buf = if_dequeue (&nif->rcv);
			if (!buf)
				break;
			
			switch ((short) buf->info)
			{
				case PKTYPE_IP:
					ip_input (nif, buf);
					break;
				
				case PKTYPE_ARP:
					arp_input (nif, buf);
					break;
				
				case PKTYPE_RARP:
					rarp_input (nif, buf);
					break;
				
				default:
					DEBUG (("if_input: unknown pktype 0x%x",
						(short)buf->info));
					buf_deref (buf, BUF_NORMAL);
					break;
			}
		}
		
		if (!todo)
			comeagain = 1;
	}
	
	if (comeagain)
	{
		/*
		 * Come again at next context switch, since we did
		 * not check all interfaces, there might be packets
		 * waiting for us.
		 */
		if_input (0, 0, 100, 0);
	}
	
	setstack (sp);
}

short
if_input (struct netif *nif, BUF *buf, long delay, short type)
{
	register ushort sr;
	register short r = 0;
	
	sr = spl7 ();
	
	if (buf)
	{
		buf->info = type;
		r = if_enqueue (&nif->rcv, buf, IF_PRIORITIES-1);
	}
	
	if (tmout == 0)
		tmout = addroottimeout (delay, if_doinput, 1);
	
	spl (sr);
	
	return r;
}

static void
if_slowtimeout (long proc)
{
	struct netif *nif;
	
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		if (nif->flags & IFF_UP && nif->timeout)
			(*nif->timeout) (nif);
	}
	
	addroottimeout (IF_SLOWTIMEOUT, if_slowtimeout, 0);
}

long
if_register (struct netif *nif)
{
	static short have_timeout = 0;
	short i;
	
	nif->addrlist = 0;
	nif->snd.qlen = 0;
	nif->rcv.qlen = 0;
	nif->snd.curr = 0;
	nif->rcv.curr = 0;
	
	for (i = 0; i < IF_PRIORITIES; ++i)
	{
		nif->snd.qfirst[i] = nif->snd.qlast[i] = 0;
		nif->rcv.qfirst[i] = nif->rcv.qlast[i] = 0;
	}
	
	if (nif->hwtype >= HWTYPE_NONE)
	{
		nif->hwlocal.len = 0;
		nif->hwbrcst.len = 0;
	}
	
	nif->in_packets = 0;
	nif->in_errors = 0;
	nif->out_packets = 0;
	nif->out_errors = 0;
	nif->collisions = 0;
	
	nif->next = allinterfaces;
	allinterfaces = nif;
	if (nif->timeout && !have_timeout)
	{
		addroottimeout (IF_SLOWTIMEOUT, if_slowtimeout, 0);
		have_timeout = 1;
	}
	
	return 0;
}

/*
 * Get an unused unit number for interface name 'name'
 */
short
if_getfreeunit (char *name)
{
	struct netif *ifp;
	short max = -1;
	
	for (ifp = allinterfaces; ifp; ifp = ifp->next)
	{
		if (!strncmp (ifp->name, name, IF_NAMSIZ) && ifp->unit > max)
			max = ifp->unit;
	}
	
	return max+1;
}

long
if_open (struct netif *nif)
{
	struct ifaddr *ifa;
	long error;
	
	error = (*nif->open) (nif);
	if (error)
	{
		DEBUG (("if_open: cannot open interface %s%d", nif->name, nif->unit));
		return error;
	}
	nif->flags |= (IFF_UP|IFF_RUNNING);
	
	/*
	 * Make sure lo0 is always reachable as 127.0.0.1
	 */
	if (nif->flags & IFF_LOOPBACK)
	{
		route_add (nif, 0x7f000000L, IN_CLASSA_NET, INADDR_ANY,
			RTF_STATIC|RTF_UP|RTF_LOCAL, 999, 0);
	}
	
	ifa = if_af2ifaddr (nif, AF_INET);
	if (!ifa)
	{
		DEBUG (("if_open: warning: iface %s%d has no inet addr",
			nif->name, nif->unit));
		return 0;
	}
	
	/*
	 * This route is necessary for IP to deliver incoming packets
	 * to the local software. It routes the incoming packet and
	 * then compares the packets destination address and the
	 * interfaces' local address. If they match, the packet is
	 * delivered to the local software.
	 */
	if (SIN (&ifa->addr)->sin_addr.s_addr != INADDR_ANY)
	{
		route_add (if_lo, SIN (&ifa->addr)->sin_addr.s_addr, 0xffffffffL,
			INADDR_ANY, RTF_STATIC|RTF_UP|RTF_HOST|RTF_LOCAL, 999, 0);
	}
	
	/*
	 * Want the new interface to become primary one.
	 *
	 * This is usefull to broadcast packets.
	 */
	rt_primary.nif = nif;
	
	DEBUG (("if_open: primary_nif = %s", rt_primary.nif->name));

	return 0;
}

long
if_close (struct netif *nif)
{
	struct ifaddr *ifa;
	long error;
	
	error = (*nif->close) (nif);
	if (error)
	{
		DEBUG (("if_close: cannot close if %s%d", nif->name, nif->unit));
		return error;
	}
	
	if_flushq (&nif->snd);
	if_flushq (&nif->rcv);
	
	route_flush (nif);
	arp_flush (nif);
	
	ifa = if_af2ifaddr (nif, AF_INET);
	if (ifa != 0)
		route_del (SIN (&ifa->addr)->sin_addr.s_addr, 0xffffffff);
	
	if (nif->flags & IFF_LOOPBACK)
		route_del (0x7f000000L, IN_CLASSA_NET);
	
	nif->flags &= ~(IFF_UP|IFF_RUNNING);
	
	/*
	 * Want a running primary interface
	 */
	if (nif == rt_primary.nif)
	{
		for (nif = allinterfaces; nif; nif = nif->next)
		{
			if (nif->flags & IFF_UP)
			{
				rt_primary.nif = nif;
				break;
			}
		}
	}

	DEBUG (("if_close: primary_nif = %s", rt_primary.nif->name));
	
	return 0;
}

long
if_send (struct netif *nif, BUF *buf, ulong nexthop, short isbrcst)
{
	struct arp_entry *are;
	long ret;

	if ((nif->flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
	{
		DEBUG (("if_send: interface %s%d is not UP and RUNNING", nif->name, nif->unit));

		buf_deref (buf, BUF_NORMAL);
		return ENETUNREACH;
	}
	
	if (nif->hwtype >= HWTYPE_NONE)
	{
		DEBUG (("if_send(%s): >= HWTYPE_NONE", nif->name));

		/*
		 * This pseudo hardware type needs no ARP resolving
		 * of the next hop IP address into a hardware address.
		 * We pass the IP address of the next destination instead.
		 */
		return (*nif->output) (nif, buf, (char *) &nexthop,
			sizeof (nexthop), PKTYPE_IP);
	}
	else switch (nif->hwtype)
	{
		case HWTYPE_ETH:
		{
			DEBUG (("if_send(%s): HWTYPE_ETH (brcst=%d)",
				nif->name, isbrcst));

			/*
			 * When broadcast then use interface's broadcast address
			 */
			if (isbrcst)
				return (*nif->output) (nif, buf, nif->hwbrcst.addr,
					nif->hwbrcst.len, PKTYPE_IP);
			/*
			 * Here we must first resolve the IP address into a hardware
			 * address using ARP.
			 */
			are = arp_lookup (0, nif, ARPRTYPE_IP, 4, (char *)&nexthop);
			if (are == 0)
			{
				buf_deref (buf, BUF_NORMAL);
				return ENOMEM;
			}
			
			if (ATF_ISCOM (are))
			{
				ret = (*nif->output) (nif, buf, are->hwaddr.addr,
					are->hwaddr.len, PKTYPE_IP);
			}
			else
				ret = if_enqueue (&are->outq, buf, IF_PRIORITIES-1);
			
			arp_deref (are);
			return ret;
		}
		default:
		{
			DEBUG (("if_send: %d: Invalid hardware type", nif->hwtype));
			buf_deref (buf, BUF_NORMAL);
			return EINVAL;
		}
	}
}

long
if_ioctl (short cmd, long arg)
{
	struct ifreq *ifr;
	struct netif *nif;
	
	switch (cmd)
	{
		case SIOCSIFLINK:
		{
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: SIFLINK: permission denied"));
				return EACCES;
			}
		}
		/* fallback ?????? */
		case SIOCGIFNAME:
		{
			struct iflink *ifl;
			
			ifl = (struct iflink *) arg;
			nif = if_name2if (ifl->ifname);
			if (!nif)
			{
				DEBUG (("if_ioctl: %s: no such if", ifl->ifname));
				return ENOENT;
			}
			return (*nif->ioctl)(nif, cmd, arg);
		}
		case SIOCGIFCONF:
		{
			return if_config ((struct ifconf *) arg);
		}
	}
	
	ifr = (struct ifreq *) arg;
	nif = if_name2if (ifr->ifr_name);
	if (!nif)
	{
		DEBUG (("if_ioctl: %s: no such interface", ifr->ifr_name));
		return ENOENT;
	}
	
	switch (cmd)
	{
		case SIOCGIFHWADDR:
		{
			struct sockaddr_hw *shw =
				(struct sockaddr_hw *)&ifr->ifru.addr;
			
			shw->shw_family = AF_LINK;
			shw->shw_type = nif->hwtype;
			shw->shw_len = nif->hwlocal.len;
			memcpy (shw->shw_addr, nif->hwlocal.addr,
				MIN (shw->shw_len, sizeof (shw->shw_addr)));
			
			return 0;
		}
		case SIOCSLNKFLAGS:
		case SIOCSIFOPT:
		{
			if (p_geteuid ())
				return EACCES;
		}
		case SIOCGLNKFLAGS:
		case SIOCGLNKSTATS:
		{
			return (*nif->ioctl) (nif, cmd, arg);
		}
		case SIOCGIFSTATS:
		{
			ifr->ifru.stats.in_packets  = nif->in_packets;
			ifr->ifru.stats.out_packets = nif->out_packets;
			ifr->ifru.stats.in_errors   = nif->in_errors;
			ifr->ifru.stats.out_errors  = nif->out_errors;
			ifr->ifru.stats.collisions  = nif->collisions;
			return 0;
		}
		case SIOCGIFFLAGS:
		{
			ifr->ifru.flags = nif->flags;
			return 0;
		}
		case SIOCSIFFLAGS:
		{
			short nflags = ifr->ifru.flags & IFF_MASK;
			long error;
			
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: permission denied"));
				return EACCES;
			}
			
			if (nif->flags & IFF_UP && !(nflags & IFF_UP))
			{
				error = if_close (nif);
				if (error)
					return error;
			}
			else if (!(nif->flags & IFF_UP) && nflags & IFF_UP)
			{
				error = if_open (nif);
				if (error)
					return error;
			}
			
			nif->flags &= ~IFF_MASK;
			nif->flags |= nflags;
			
			return (*nif->ioctl) (nif, cmd, arg);
		}
		case SIOCGIFMETRIC:
		{
			ifr->ifru.metric = nif->metric;
			return 0;
		}
		case SIOCSIFMETRIC:
		{
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: permission denied"));
				return EACCES;
			}
			nif->metric = ifr->ifru.metric;
			return 0;
		}
		case SIOCSIFMTU:
		{
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: permission denied"));
				return EACCES;
			}
			nif->mtu = ifr->ifru.mtu;
			(*nif->ioctl) (nif, cmd, 0);
			return 0;
		}
		case SIOCGIFMTU:
		{
			ifr->ifru.mtu = nif->mtu;
			return 0;
		}
		case SIOCSIFDSTADDR:
		{
			struct ifaddr *ifa;
			
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: permission denied"));
				return EACCES;
			}
			
			if (!(nif->flags & IFF_POINTOPOINT))
			{
				DEBUG (("if_ioctl: nif is not p2p"));
				return EACCES;
			}
			
			ifa = if_af2ifaddr (nif, ifr->ifru.dstaddr.sa_family);
			if (!ifa)
			{
				DEBUG (("if_ioctl: %d: interface has no addr "
					"in this AF",
					ifr->ifru.dstaddr.sa_family));
				return EINVAL;
			}
			
			sa_copy (&ifa->ifu.dstaddr, &ifr->ifru.dstaddr);
			return 0;
		}		
		case SIOCGIFDSTADDR:
		{
			struct ifaddr *ifa;
			
			if (!(nif->flags & IFF_POINTOPOINT))
			{
				DEBUG (("if_ioctl: nif is not p2p"));
				return EACCES;
			}
			
			ifa = if_af2ifaddr (nif, ifr->ifru.dstaddr.sa_family);
			if (!ifa)
			{
				DEBUG (("if_ioctl: %d: interface has no addr "
					"in this AF",
					ifr->ifru.dstaddr.sa_family));
				return EINVAL;
			}
			
			sa_copy (&ifr->ifru.dstaddr, &ifa->ifu.dstaddr);
			return 0;
		}
		case SIOCSIFADDR:
		{
			long error;
			
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: permission denied"));
				return EACCES;
			}
			
			error = if_setifaddr (nif, &ifr->ifru.addr);
			if (error)
				return error;
			
			return (*nif->ioctl) (nif, cmd, arg);
		}
		case SIOCGIFADDR:
		{
			struct ifaddr *ifa;
			
			ifa = if_af2ifaddr (nif, ifr->ifru.dstaddr.sa_family);
			if (!ifa)
			{
				DEBUG (("if_ioctl: %d: interface has no addr "
					"in this AF",
					ifr->ifru.dstaddr.sa_family));
				return EINVAL;
			}
			
			sa_copy (&ifr->ifru.addr, &ifa->addr);
			return 0;
		}
		case SIOCSIFBRDADDR:
		{
			struct ifaddr *ifa;
			
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: permission denied"));
				return EACCES;
			}
			
			if (!(nif->flags & IFF_BROADCAST))
			{
				DEBUG (("if_ioctl: nif is not broadcast"));
				return EACCES;
			}
			
			ifa = if_af2ifaddr (nif, ifr->ifru.broadaddr.sa_family);
			if (!ifa)
			{
				DEBUG (("if_ioctl: %d: interface has no addr "
					"in this AF",
					ifr->ifru.broadaddr.sa_family));
				return EINVAL;
			}
			
			sa_copy (&ifa->ifu.broadaddr, &ifr->ifru.broadaddr);
			return 0;
		}
		case SIOCGIFBRDADDR:
		{
			struct ifaddr *ifa;
			
			if (!(nif->flags & IFF_BROADCAST))
			{
				DEBUG (("if_ioctl: nif is not broadcast"));
				return EACCES;
			}
			
			ifa = if_af2ifaddr (nif, ifr->ifru.broadaddr.sa_family);
			if (!ifa)
			{
				DEBUG (("if_ioctl: %d: interface has no addr "
					"in this AF",
					ifr->ifru.broadaddr.sa_family));
				return EINVAL;
			}
			
			sa_copy (&ifr->ifru.broadaddr, &ifa->ifu.broadaddr);
			return 0;
		}
		case SIOCSIFNETMASK:
		{
			struct ifaddr *ifa;
			
			if (p_geteuid ())
			{
				DEBUG (("if_ioctl: permission denied"));
				return EACCES;
			}
			
			if (ifr->ifru.broadaddr.sa_family != AF_INET)
			{
				DEBUG (("if_ioctl: address family != AF_INET"));
				return EAFNOSUPPORT;
			}
			
			ifa = if_af2ifaddr (nif, AF_INET);
			if (!ifa)
			{
				DEBUG (("if_ioctl: %d: interface has no addr "
					"in AF_INET"));
				return EINVAL;
			}
			
			ifa->subnetmask =
				SIN (&ifr->ifru.netmask)->sin_addr.s_addr;
			ifa->subnet = ifa->subnetmask &
				SIN (&ifa->addr)->sin_addr.s_addr;
			
			return (*nif->ioctl) (nif, cmd, arg);
		}
		case SIOCGIFNETMASK:
		{
			struct sockaddr_in sin;
			struct ifaddr *ifa;
			
			ifa = if_af2ifaddr (nif, ifr->ifru.broadaddr.sa_family);
			if (!ifa)
			{
				DEBUG (("if_ioctl: %d: interface has no addr "
					"in this AF",
					ifr->ifru.broadaddr.sa_family));
				return EINVAL;
			}
			
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = ifa->subnetmask;
			sin.sin_port = 0;
			
			sa_copy (&ifr->ifru.netmask, (struct sockaddr *) &sin);
			return 0;
		}
	}
	
	return ENOSYS;
}

struct netif *
if_name2if (char *ifname)
{
	char name[IF_NAMSIZ+1], *cp;
	short i;
	long unit = 0;
	struct netif *nif;
	
	for (i = 0, cp = ifname; i < IF_NAMSIZ && *cp; ++cp, ++i)
	{
		if (*cp >= '0' && *cp <= '9')
		{
			unit = atol (cp);
			break;
		}
		name[i] = *cp;
	}
	
	name[i] = '\0';
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		if (!stricmp (nif->name, name) && nif->unit == unit)
			return nif;
	}
	
	return NULL;
}

/* find an interface which is on the same network as the address `addr' */
struct netif *
if_net2if (ulong addr)
{
	struct netif *nif;
	struct ifaddr *ifa;

	for (nif = allinterfaces; nif; nif = nif->next)
	{
		if (!(nif->flags & IFF_UP))
			continue;
		
		ifa = if_af2ifaddr (nif, AF_INET);
		if (!ifa)
			continue;
		
		if (nif->flags & IFF_POINTOPOINT &&
		    addr == SIN (&ifa->ifu.dstaddr)->sin_addr.s_addr)
		{
			DEBUG(("if_net2if: nif '%s' p2p", nif->name));
			return nif;
		}
		
		if (nif->flags & IFF_BROADCAST &&
		    addr == SIN (&ifa->ifu.broadaddr)->sin_addr.s_addr)
		{
			DEBUG(("if_net2if: nif '%s' broadcast", nif->name));
			return nif;
		}
	}
	
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		if (!(nif->flags & IFF_UP))
			continue;
		
		ifa = if_af2ifaddr (nif, AF_INET);
		if (!ifa)
			continue;
		
		if ((addr & ifa->netmask) == ifa->net)
		{
			DEBUG(("if_net2if: nif '%s' netmask match", nif->name));
			return nif;
		}
	}
	
	DEBUG(("if_net2if: no match"));
	return NULL;
}

long
if_setifaddr (struct netif *nif, struct sockaddr *sa)
{
	struct ifaddr *ifa, *ifa2;
	short error = 0;
	
	ifa = kmalloc (sizeof (*ifa));
	if (!ifa)
	{
		DEBUG (("if_newaddr: out of memory"));
		return ENOMEM;
	}
	
	sa_copy (&ifa->addr, sa);
	ifa->family = sa->sa_family;
	ifa->ifp = nif;
	ifa->flags = 0;

	switch (sa->sa_family)
	{
		case AF_INET:
		{
			struct sockaddr_in *sin = (struct sockaddr_in *)sa;
			ulong netmask;
			
			netmask = ip_netmask (sin->sin_addr.s_addr);
			if (netmask == 0)
			{
				DEBUG (("if_setaddr: Addr not in class A/B/C"));
				error = EADDRNOTAVAIL;
				break;
			}
			ifa->net           =
			ifa->subnet        = sin->sin_addr.s_addr & netmask;
			ifa->netmask       =
			ifa->subnetmask    = netmask;
			ifa->net_broadaddr = ifa->net | ~netmask;
			
			sin = (struct sockaddr_in *)&ifa->ifu.broadaddr;
			sin->sin_family = AF_INET;
			sin->sin_port = 0;
			sin->sin_addr.s_addr = (nif->flags & IFF_BROADCAST)
				? ifa->net_broadaddr
				: INADDR_ANY;
			
			route_flush (nif);
			ifa2 = if_af2ifaddr (nif, AF_INET);
			if (ifa2) route_del (SIN (&ifa2->addr)->sin_addr.s_addr, 0xffffffff);
			
			/*
			 * Make sure lo0 is always reachable as 127.0.0.1
			 */
			if (nif->flags & IFF_LOOPBACK)
				route_add (nif, 0x7f000000L, IN_CLASSA_NET, INADDR_ANY,
					RTF_STATIC|RTF_UP|RTF_LOCAL, 999, 0);

			/*
			 * This route is necessary for IP to deliver incoming packets
			 * to the local software. It routes the incoming packet and
			 * then compares the packets destination address and the
			 * interfaces' local address. If they match, the packet is
			 * delivered to the local software.
			 */
			if (SIN (&ifa->addr)->sin_addr.s_addr != INADDR_ANY) {
				route_add (if_lo, SIN (&ifa->addr)->sin_addr.s_addr,
					0xffffffff, INADDR_ANY,
					RTF_STATIC|RTF_UP|RTF_HOST|RTF_LOCAL, 999, 0);
			} else {
				/* primary broadcast interface */
				rt_primary.nif = nif;
				DEBUG (("if_setifaddr: primary_nif = %s", rt_primary.nif->name));
			}
			break;
		}
		default:
		{
			DEBUG (("if_setifaddr: %d: address family not supported",
				sa->sa_family));
			error = EAFNOSUPPORT;
			break;
		}
	}
	
	if (error)
	{
		kfree (ifa);
		return error;
	}
	
	ifa2 = if_af2ifaddr (nif, ifa->family);
	if (ifa2)
	{
		ifa->next = ifa2->next;
		*ifa2 = *ifa;
		kfree (ifa);
	}
	else
	{
		ifa->next = nif->addrlist;
		nif->addrlist = ifa;
	}
	
	return 0;
}

long
if_config (struct ifconf *ifconf)
{
	struct netif *nif;
	struct ifreq *ifr;
	struct ifaddr *ifa;
	char name[100];
	long len;
	
	len = ifconf->len;
	ifr = ifconf->ifcu.req;
	nif = allinterfaces;
	for (; len >= sizeof (*ifr) && nif; nif = nif->next)
	{
		ksprintf (name, "%s%d", nif->name, nif->unit);
		ifa = nif->addrlist;
		if (!ifa)
		{
			struct sockaddr_in sin;
			
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = INADDR_ANY;
			sin.sin_port = 0;
			strncpy (ifr->ifr_name, name, IF_NAMSIZ);
			sa_copy (&ifr->ifru.addr, (struct sockaddr *)&sin);
			len -= sizeof (*ifr);
			++ifr;
		}
		else
		{
			for (; len >= sizeof (*ifr) && ifa; ifa = ifa->next)
			{
				strncpy (ifr->ifr_name, name, IF_NAMSIZ);
				sa_copy (&ifr->ifru.addr, &ifa->addr);
				len -= sizeof (*ifr);
				++ifr;
			}
		}
	}
	ifconf->len -= len;
	return 0;
}

struct ifaddr *
if_af2ifaddr (struct netif *nif, short family)
{
	struct ifaddr *ifa;
	
	for (ifa = nif->addrlist; ifa; ifa = ifa->next)
		if (ifa->family == family)
			break;
	
	return ifa;
}

long
if_init (void)
{
	struct netif *nif;
	
	if_load ();
	loopback_init (); /* must be last */
	arp_init ();
	
	/*
	 * Look for primary and loopback interface
	 */
	for (nif = allinterfaces; nif; nif = nif->next)
	{
		if (nif->flags & IFF_LOOPBACK)
		{
			if_lo = rt_primary.nif = nif;
			break;
		}
	}
	
	if (!if_lo)
		FATAL ("if_init: no loopback interface");
	
	return 0;
}
