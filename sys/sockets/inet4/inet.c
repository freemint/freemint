/*
 *	Inet domain toplevel.
 *
 *	started 01/20/94, kay roemer.
 *	07/01/99 masquerading code by Mario Becroft.
 */

# include "inet.h"

# include "arp.h"
# include "icmp.h"
# include "inetdev.h"
# include "inetutil.h"
# include "masquerade.h"
# include "port.h"
# include "rawip.h"
# include "route.h"
# include "tcp.h"
# include "udp.h"

# include "sockutil.h"
# include "timer.h"
# include "util.h"

# include <mint/dcntl.h>
# include <mint/signal.h>


static void	inet_autobind	(struct in_data *);

static long	inet_attach	(struct socket *, short);
static long	inet_dup	(struct socket *, struct socket *);
static long	inet_abort	(struct socket *, enum so_state);
static long	inet_detach	(struct socket *);
static long	inet_bind	(struct socket *, struct sockaddr *, short);
static long	inet_connect	(struct socket *, struct sockaddr *, short, short);
static long	inet_socketpair	(struct socket *, struct socket *);
static long	inet_accept	(struct socket *, struct socket *, short);
static long	inet_getname	(struct socket *, struct sockaddr *, short *, short);
static long	inet_select	(struct socket *, short, long);
static long	inet_ioctl	(struct socket *, short, void *);
static long	inet_listen	(struct socket *, short);
static long	inet_send	(struct socket *, struct iovec *, short, short, short, struct sockaddr *, short);
static long	inet_recv	(struct socket *, struct iovec *, short, short, short, struct sockaddr *, short *);
static long	inet_shutdown	(struct socket *, short);
static long	inet_setsockopt	(struct socket *, short, short, char *, long);
static long	inet_getsockopt	(struct socket *, short, short, char *, long *);

static struct dom_ops inet_ops =
{
	domain:		AF_INET,
	next:		NULL,
	attach:		inet_attach,
	dup:		inet_dup,
	abort:		inet_abort,
	detach:		inet_detach,
	bind:		inet_bind,
	connect:	inet_connect,
	socketpair:	inet_socketpair,
	accept:		inet_accept,
	getname:	inet_getname,
	select:		inet_select,
	ioctl:		inet_ioctl,
	listen:		inet_listen,
	send:		inet_send,
	recv:		inet_recv,
	shutdown:	inet_shutdown,
	setsockopt:	inet_setsockopt,
	getsockopt:	inet_getsockopt
};

void
inet_init (void)
{
	if_init ();
	route_init ();
	rip_init ();	/* must be first of the protocols */
	icmp_init ();
	udp_init ();
	tcp_init ();
	inetdev_init ();
	masq_init ();
	
	/* initialize lo0 */
	{
		struct sockaddr_in in;
		struct netif *nif;
		struct ifaddr *ifa;
		
		nif = if_name2if ("lo0");
		if (!nif)
		{
			ALERT (("inet4: No such interface lo0 ???"));
			goto error;
		}
		
		/* addr 127.0.0.1 */
		
		in.sin_family = AF_INET;
		in.sin_addr.s_addr = 2130706433UL;
		in.sin_port = 0;
		
		if_setifaddr (nif, (struct sockaddr *) &in);
		
		ifa = if_af2ifaddr (nif, AF_INET);
		if (!ifa)
		{
			ALERT (("inet4: initializing lo0 failed ???"));
        		goto error;
        	}
		
		/* netmask 255.255.255.0 */
		
		ifa->subnetmask = 4294967040UL;
		ifa->subnet = ifa->subnetmask & 4294967040UL;
		
		/* broadaddr 127.0.0.255 */
		
		in.sin_family = AF_INET;
		in.sin_addr.s_addr = 2130706687UL;
		in.sin_port = 0;
		
		sa_copy (&ifa->ifu.broadaddr, (struct sockaddr *) &in);
		
		/* up & running */
		
		nif->ioctl(nif, SIOCSIFADDR, 0);
	}
	
error:
	/* register our domain */
	so_register (AF_INET, &inet_ops);
}

static void
inet_autobind (struct in_data *data)
{
	if (!(data->flags & IN_ISBOUND))
	{
		data->src.port = (data->sock->type == SOCK_RAW)
			? 0 : port_alloc (data);
		data->src.addr = INADDR_ANY;
		data->flags |= IN_ISBOUND;
	}
}

static long
inet_attach (struct socket *so, short proto)
{
	struct in_data *data;
	short handler;
	long r;
	
	switch (so->type)
	{
		case SOCK_STREAM:
			if (proto && proto != IPPROTO_TCP)
			{
				DEBUG (("inet_attach: %d: wrong stream protocol",
					proto));
				return EPROTOTYPE;
			}
			handler = proto = IPPROTO_TCP;
			break;
		
		case SOCK_DGRAM:
			if (proto && proto != IPPROTO_UDP)
			{
				DEBUG (("inet_attach: %d: wrong dgram protocol",
					proto));
				return EPROTOTYPE;
			}
			handler = proto = IPPROTO_UDP;
			break;
		
		case SOCK_RAW:
			handler = IPPROTO_RAW;
			break;
		
		default:
			DEBUG (("inet_attach: %d: unsupported socket type", so->type));
			return ESOCKTNOSUPPORT;
	}
	
	data = in_data_create ();
	if (!data)
	{
		DEBUG (("inet_attach: No mem for socket data"));
		return ENOMEM;
	}
	
	data->protonum = proto;
	data->proto = in_proto_lookup (handler);
	data->sock = so;
	if (!data->proto)
	{
		DEBUG (("inet_attach: %d: No such protocol", handler));
		in_data_destroy (data, 0);
		return EPROTONOSUPPORT;
	}
	
	r = (*data->proto->soops.attach) (data);
	if (r)
	{
		in_data_destroy (data, 0);
		return r;
	}
	
	so->data = data;
	in_data_put (data);
	return 0;
}

static long
inet_dup (struct socket *newso, struct socket *oldso)
{
	struct in_data *data = oldso->data;
	return inet_attach (newso, data->protonum);
}

static long
inet_abort (struct socket *so, enum so_state ostate)
{
	struct in_data *data = so->data;	
	return (*data->proto->soops.abort) (data, ostate);
}

static long
inet_detach (struct socket *so)
{
	struct in_data *data = so->data;
	long r;
	
	r = (*data->proto->soops.detach)(data, 1);
	if (!r) so->data = 0;
	
	return r;
}

static long
inet_bind (struct socket *so, struct sockaddr *addr, short addrlen)
{
	struct in_data *data = so->data;
	struct sockaddr_in *inaddr = (struct sockaddr_in *) addr;
	ulong saddr;
	ushort port;
	
	if (!addr) return EDESTADDRREQ;
	
	if (data->flags & IN_ISBOUND)
	{
		DEBUG (("inet_bind: already bound"));
		return EINVAL;
	}
	
	if (addrlen != sizeof (struct sockaddr_in))
	{
		DEBUG (("inet_bind: invalid address"));
		return EINVAL;
	}
	
	if (addr->sa_family != AF_INET)
	{
		DEBUG (("inet_bind: invalid adr family"));
		return EAFNOSUPPORT;
	}
	
	saddr = inaddr->sin_addr.s_addr;
	if (saddr != INADDR_ANY)
	{
		if (!ip_is_local_addr (saddr))
		{
			DEBUG (("inet_bind: %lx: no such local IP address",
				saddr));
			return EADDRNOTAVAIL;
		}
	}
	
	port = inaddr->sin_port;
	if (so->type == SOCK_RAW)
		port = 0;
	else if (port == 0)
		port = port_alloc (data);
	else
	{
		struct in_data *data2;
		
		if (port < IPPORT_RESERVED && p_geteuid ())
		{
			DEBUG (("inet_bind: Permission denied"));
			return EACCES;
		}
		/*
		 * Reusing of local ports is allowed if:
		 * SOCK_STREAM: All sockets with same local port have
		 *		IN_REUSE set.
		 * SOCK_DGRAM:  All sockets with same local port have
		 *		IN_REUSE set and have different local
		 *		addresses
		 * binding to the same port with different ip addresses is
		 * always allowed, e.g. a local address and INADDR_ANY.
		 * for incoming packets sockets with exact address matches
		 * are preferred over INADDR_ANY matches.
		 */
		data2 = port_find_with_addr (data, port, saddr);
		if (data2)
		{
			if (!(data->flags & IN_REUSE)
				|| !(data2->flags & IN_REUSE))
			{
				DEBUG (("inet_bind: duplicate local address"));
				return EADDRINUSE;
			}
			if (so->type != SOCK_STREAM && (saddr == INADDR_ANY
				|| data2->src.addr == INADDR_ANY
				|| saddr == data2->src.addr))
			{
				DEBUG (("inet_bind: duplicate local address"));
				return EADDRINUSE;
			}
		}
	}
	
	data->src.addr = saddr;
	data->src.port = port;
	data->flags |= IN_ISBOUND;
	
	return 0;
}

static long
inet_connect (struct socket *so, struct sockaddr *addr, short addrlen, short nonblock)
{
	struct in_data *data = so->data;
	
	if (so->state == SS_ISCONNECTING)
		return EALREADY;
	
	if (addrlen != sizeof (struct sockaddr_in) || !addr)
	{
		DEBUG (("inet_connect: invalid address"));
		if (so->type != SOCK_STREAM)
			data->flags &= ~IN_ISCONNECTED;
		
		return EINVAL;
	}
	if (addr->sa_family != AF_INET)
	{
		DEBUG (("inet_connect: invalid adr family"));
		if (so->type != SOCK_STREAM)
			data->flags &= ~IN_ISCONNECTED;
		
		return EAFNOSUPPORT;
	}
	inet_autobind (data);
	return (*data->proto->soops.connect) (data, (struct sockaddr_in *)addr,
		addrlen, nonblock);
}

static long
inet_socketpair (struct socket *so1, struct socket *so2)
{
	return EOPNOTSUPP;
}

static long
inet_accept (struct socket *server, struct socket *newso, short nonblock)
{
	struct in_data *sdata = server->data, *cdata = newso->data;
	return (*sdata->proto->soops.accept)(sdata, cdata, nonblock);
}

static long
inet_getname (struct socket *so, struct sockaddr *addr, short *addrlen, short peer)
{
	struct in_data *data = so->data;
	struct sockaddr_in sin;
	long todo;
	
	if (!addr || !addrlen || *addrlen < 0)
	{
		DEBUG (("inet_getname: invalid addr/addrlen"));
		return EINVAL;
	}
	
	sin.sin_family = AF_INET;
	if (peer == PEER_ADDR)
	{
		if (!(data->flags & IN_ISCONNECTED))
		{
			DEBUG (("inet_getname: not connected"));
			return ENOTCONN;
		}
		sin.sin_port = data->dst.port;
		sin.sin_addr.s_addr = data->dst.addr;
	}
	else
	{
		inet_autobind (data);
		sin.sin_port = data->src.port;
		sin.sin_addr.s_addr = (data->src.addr != INADDR_ANY)
			? data->src.addr
			: ip_local_addr ((data->flags & IN_ISCONNECTED)
				? data->dst.addr
				: INADDR_ANY);
	}
	
	todo = MIN (*addrlen, sizeof (sin));
	bzero (sin.sin_zero, sizeof (sin.sin_zero));
	memcpy (addr, &sin, todo);
	*addrlen = todo;
	
	return 0;
}

static long
inet_select (struct socket *so, short how, long proc)
{
	struct in_data *data = so->data;
	
	if (so->type == SOCK_RAW)
		inet_autobind (data);
	
	return (*data->proto->soops.select)(data, how, proc);
}

static long
inet_ioctl (struct socket *so, short cmd, void *buf)
{
	struct in_data *data = so->data;
	
	switch (cmd)
	{
		case SIOCSIFLINK:
		case SIOCGIFNAME:
		case SIOCGIFCONF:
		case SIOCGIFFLAGS:
		case SIOCSIFFLAGS:
		case SIOCGIFMETRIC:
		case SIOCSIFMETRIC:
		case SIOCSIFMTU:
		case SIOCGIFMTU:
		case SIOCSIFADDR:
		case SIOCGIFADDR:
		case SIOCSIFDSTADDR:
		case SIOCGIFDSTADDR:
		case SIOCSIFNETMASK:
		case SIOCGIFNETMASK:
		case SIOCSIFBRDADDR:
		case SIOCGIFBRDADDR:
		case SIOCGIFSTATS:
		case SIOCGLNKFLAGS:
		case SIOCSLNKFLAGS:
		case SIOCGIFHWADDR:
		case SIOCGLNKSTATS:
		case SIOCSIFOPT:
			return if_ioctl (cmd, (long) buf);
		
		case SIOCADDRT:
		case SIOCDELRT:
			return route_ioctl (cmd, (long) buf);
		
		case SIOCDARP:
		case SIOCGARP:
		case SIOCSARP:
			return arp_ioctl (cmd, buf);
	}
	
	return (*data->proto->soops.ioctl)(data, cmd, buf);
}

static long
inet_listen (struct socket *so, short backlog)
{
	struct in_data *data = so->data;
	
	if (so->type != SOCK_STREAM)
	{
		DEBUG (("inet_listen: Not supp. for datagram sockets"));
		return EOPNOTSUPP;
	}
	inet_autobind (data);
	data->backlog = backlog;
	return (*data->proto->soops.listen)(data);
}

static long
inet_send (struct socket *so, struct iovec *iov, short niov, short nonblock,
		short flags, struct sockaddr *addr, short addrlen)
{
	struct in_data *data = so->data;
	long r;
	
	if (so->state == SS_ISDISCONNECTING || so->state == SS_ISDISCONNECTED)
	{
		DEBUG (("inet_send: Socket shut down"));
		p_kill (p_getpid (), SIGPIPE);
		return EPIPE;
	}
	
	if (data->err)
	{
		r = data->err;
		data->err = 0;
		return r;
	}
	
	if (so->flags & SO_CANTSNDMORE)
	{
		DEBUG (("inet_send: shut down"));
		p_kill (p_getpid (), SIGPIPE);
		return EPIPE;
	}
	
	if (addr)
	{
		if (addrlen != sizeof (struct sockaddr_in))
		{
			DEBUG (("inet_send: invalid address"));
			return EINVAL;
		}
		if (addr->sa_family != AF_INET)
		{
			DEBUG (("inet_send: invalid adr family"));
			return EAFNOSUPPORT;
		}
	}
	
	inet_autobind (data);
	return (*data->proto->soops.send) (data, iov, niov, nonblock, flags,
		(struct sockaddr_in *)addr, addrlen);
}

static long
inet_recv (struct socket *so, struct iovec *iov, short niov, short nonblock,
		short flags, struct sockaddr *addr, short *addrlen)
{
	struct in_data *data = so->data;
	long r;
	
	if (so->state == SS_ISDISCONNECTING || so->state == SS_ISDISCONNECTED)
	{
		DEBUG (("inet_recv: Socket shut down"));
		return 0;
	}
	
	if (data->err)
	{
		r = data->err;
		data->err = 0;
		return r;
	}
	
	inet_autobind (data);
	return (*data->proto->soops.recv)(data, iov, niov, nonblock, flags,
		(struct sockaddr_in *) addr, addrlen);
}

static long
inet_shutdown (struct socket *so, short how)
{
	struct in_data *data = so->data;
	long r;
	
	inet_autobind (data);
	r = (*data->proto->soops.shutdown)(data, how);
	
	if (((SO_CANTRCVMORE|SO_CANTSNDMORE) & so->flags) ==
	    (SO_CANTRCVMORE|SO_CANTSNDMORE))
	{
		DEBUG (("inet_shutdown: releasing socket"));
		so_release (so);
	}
	
	return r;
}

static long
inet_setsockopt (struct socket *so, short level, short optname, char *optval, long optlen)
{
	struct in_data *data = so->data;
	long val;
	
	switch (level)
	{
		case IPPROTO_IP:
			return ip_setsockopt (&data->opts, level, optname, optval,
				optlen);
		
		case SOL_SOCKET:
			break;
		
		default:
			return (*data->proto->soops.setsockopt) (data, level, optname,
				optval, optlen);
	}
	
	if (!optval || optlen < sizeof (long))
	{
		DEBUG (("inet_setsockopt: invalid optval/optlen"));
		return EINVAL;
	}
	
	val = *(long *) optval;
	
	switch (optname)
	{
		case SO_DEBUG:
			break;
		
		case SO_REUSEADDR:
			if (val) data->flags |= IN_REUSE;
			else	 data->flags &= ~IN_REUSE;
			break;
		
		case SO_DONTROUTE:
			if (val) data->flags |= IN_DONTROUTE;
			else	 data->flags &= ~IN_DONTROUTE;
			break;
		
		case SO_BROADCAST:
			if (val) data->flags |= IN_BROADCAST;
			else	 data->flags &= ~IN_BROADCAST;
			break;
		
		case SO_SNDBUF:
			if (val > IN_MAX_WSPACE)
				val = IN_MAX_WSPACE;
			else if (val < IN_MIN_WSPACE)
				val = IN_MIN_WSPACE;
			
			if (so->type == SOCK_STREAM && val < data->snd.curdatalen)
			{
				DEBUG (("inet_setsockopt: sndbuf size invalid"));
				return EINVAL;
			}
			data->snd.maxdatalen = val;
			break;
		
		case SO_RCVBUF:
			if (val > IN_MAX_WSPACE)
				val = IN_MAX_WSPACE;
			else if (val < IN_MIN_WSPACE)
				val = IN_MIN_WSPACE;
			
			if (so->type == SOCK_STREAM && val < data->rcv.curdatalen)
			{
				DEBUG (("inet_setsockopt: rcvbuf size invalid"));
				return EINVAL;
			}
			data->rcv.maxdatalen = val;
			break;
		
		case SO_KEEPALIVE:
			if (val) data->flags |= IN_KEEPALIVE;
			else	 data->flags &= ~IN_KEEPALIVE;
			break;
		
		case SO_OOBINLINE:
			if (val) data->flags |= IN_OOBINLINE;
			else	 data->flags &= ~IN_OOBINLINE;
			break;
		
		case SO_LINGER:
		{
			struct linger l;
			
			if (optlen < sizeof (struct linger))
			{
				DEBUG (("inet_setsockopt: optlen to small"));
				return EINVAL;
			}
			
			l = *(struct linger *)optval;
			if (l.l_onoff)
			{
				data->flags |= IN_LINGER;
				data->linger = l.l_linger;
			}
			else
				data->flags &= ~IN_LINGER;
			break;
		}
		case SO_CHKSUM:
			if (val) data->flags |= IN_CHECKSUM;
			else	 data->flags &= ~IN_CHECKSUM;
			break;
		
		default:
			DEBUG (("inet_setsockopt: %d: invalid option", optval));
			return EOPNOTSUPP;
	}
	
	return 0;
}

static long
inet_getsockopt (struct socket *so, short level, short optname, char *optval, long *optlen)
{
	struct in_data *data = so->data;
	long val;
	
	switch (level)
	{
		case IPPROTO_IP:
			return ip_getsockopt (&data->opts, level, optname, optval,
				optlen);
		
		case SOL_SOCKET:
			break;
		
		default:
			return (*data->proto->soops.getsockopt) (data, level, optname,
				optval, optlen);
	}
	
	if (!optlen || !optval)
	{
		DEBUG (("inet_getsockopt: invalid optval/optlen"));
		return EINVAL;
	}
	
	switch (optname)
	{
		case SO_DEBUG:
			val = 0;
			break;
		
		case SO_TYPE:
			val = so->type;
			break;
		
		case SO_ERROR:
			val = data->err;
			data->err = 0;
			break;
		
		case SO_REUSEADDR:
			val = (data->flags & IN_REUSE) ? 1 : 0;
			break;
		
		case SO_KEEPALIVE:
			val = (data->flags & IN_KEEPALIVE) ? 1 : 0;
			break;
		
		case SO_DONTROUTE:
			val = (data->flags & IN_DONTROUTE) ? 1 : 0;
			break;
		
		case SO_LINGER:
		{
			struct linger l;
			
			if (*optlen < sizeof (struct linger))
			{
				DEBUG (("inet_setsockopt: optlen < sizeof linger"));
				return EINVAL;
			}
			
			if (data->flags & IN_LINGER)
			{
				l.l_onoff = 1;
				l.l_linger = data->linger;
			}
			else
			{
				l.l_onoff = 0;
				l.l_linger = 0;
			}
			
			*(struct linger *) optval = l;
			*optlen = sizeof (struct linger);
			
			return 0;
		}
		case SO_BROADCAST:
			val = (data->flags & IN_BROADCAST) ? 1 : 0;
			break;
		
		case SO_OOBINLINE:
			val = (data->flags & IN_OOBINLINE) ? 1 : 0;
			break;
		
		case SO_SNDBUF:
			val = data->snd.maxdatalen;
			break;
		
		case SO_RCVBUF:
			val = data->rcv.maxdatalen;
			break;
		
		case SO_CHKSUM:
			val = (data->flags & IN_CHECKSUM) ? 1 : 0;
			break;
		
		default:
			return EOPNOTSUPP;
	}
	
	if (*optlen < sizeof (long))
	{
		DEBUG (("inet_getsockopt: optlen < sizeof long"));
		return EINVAL;
	}
	
	*(long *) optval = val;
	*optlen = sizeof (long);
	
	return 0;
}
