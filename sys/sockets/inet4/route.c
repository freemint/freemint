/*
 *	Simple IP router.
 *
 *	02/28/94, Kay Roemer.
 */

# include "route.h"

# include "in.h"
# include "ip.h"
# include "route.h"
# include "routedev.h"

# include "mint/socket.h"
# include "mint/sockio.h"


struct route *allroutes[RT_HASH_SIZE];
struct route *defroute;

void
route_init (void)
{
	short i;
	
	for (i = 0; i < RT_HASH_SIZE; ++i)
		allroutes[i] = 0;
	
	defroute = 0;
	routedev_init ();
}

static ushort
route_hash (ulong d)
{
	ushort hash;
	
	if (IN_CLASSA (d))
		hash = (d >> 24);
	else if (IN_CLASSB (d))
		hash = (d >> 16) ^ (d >> 24);
	else if (IN_CLASSC (d))
		hash = (d >> 8) ^ (d >> 16) ^ (d >> 24);
	else
		hash = d ^ (d >> 8) ^ (d >> 16) ^ (d >> 24);
	
	return (hash & 0xff);
}

/*
 * Find a route to destination address `daddr'. We prefer host routes over
 * net routes.
 */
struct route *
route_get (ulong daddr)
{
	struct route *netrt, *rt;
	
	rt = allroutes[route_hash (daddr)];
	for (netrt = 0; rt; rt = rt->next)
	{
		if (rt->ttl <= 0 || !(rt->flags & RTF_UP))
			continue;
		if ((rt->mask & daddr) == rt->net)
		{
			if (rt->flags & RTF_HOST)
				break;
			netrt = rt;
		}
	}
	
	if (!rt)
		rt = netrt ? netrt : defroute;
	
	if (rt && rt->flags & RTF_UP)
	{
		rt->refcnt++;
		rt->usecnt++;
		return rt;
	}
	
	return NULL;
}

struct route *
route_alloc (struct netif *nif, ulong net, ulong mask, ulong gway, short flags, short ttl, long metric)
{
	struct route *rt;

	rt = kmalloc (sizeof (struct route));
	if (!rt)
	{
		DEBUG (("route_alloc: out of mem"));
		return NULL;
	}
	
	rt->net = net;
	rt->mask = mask;
	
	if (flags & RTF_GATEWAY && gway == INADDR_ANY)
	{
		DEBUG (("route_alloc: invalid gateway"));
		kfree (rt);
		return NULL;
	}
	
	rt->gway = (flags & RTF_GATEWAY) ? gway : INADDR_ANY;
	rt->nif = nif;
	rt->flags = flags;
	rt->ttl = ttl;
	rt->metric = metric;
	rt->usecnt = 0;
	rt->refcnt = 1;
	rt->next = 0;
	
	return rt;
}

long
route_add (struct netif *nif, ulong net, ulong mask, ulong gway,
		short flags, short ttl, long metric)
{
	struct route *newrt, *rt, **prevrt;
	
	DEBUG (("route_add: net 0x%lx mask 0x%lx gway 0x%lx", net, mask, gway));
	
	newrt = route_alloc (nif, net, mask, gway, flags, ttl, metric);
	if (!newrt)
	{
		DEBUG (("route_add: no memory for route"));
		return ENOMEM;
	}
	
	if (net == INADDR_ANY)
	{
		DEBUG (("route_add: updating default route"));
		route_deref (defroute);
		defroute = newrt;
		return 0;
	}
	
	prevrt = &allroutes[route_hash (net)];
	for (rt = *prevrt; rt; prevrt = &rt->next, rt = rt->next)
	{
		if (rt->mask == mask && rt->net == net)
		{
			if (!(flags & RTF_STATIC) && rt->flags & RTF_STATIC)
			{
				DEBUG (("route_add: would overwrite "
					"static route with non static ..."));
				kfree (newrt);
				return EACCES;
			}
			DEBUG (("route_add: replacing route"));
			newrt->next = rt->next;
			route_deref (rt);
			break;
		}
	}
	*prevrt = newrt;
	return 0;	
}

long
route_del (ulong net, ulong mask)
{
	struct route **prevrt, *nextrt, *rt;
	
	DEBUG (("route_del: deleting route net %lx mask %lx", net, mask));
	
	if (defroute && net == INADDR_ANY)
	{
		DEBUG (("route_del: freeing default route"));
		route_deref (defroute);
		defroute = 0;
		return 0;
	}
	
	prevrt = &allroutes[route_hash (net)];
	for (rt = *prevrt; rt; rt = nextrt)
	{
		nextrt = rt->next;
		if (rt->mask == mask && rt->net == net)
		{
			DEBUG (("route_del: removing route"));
			*prevrt = nextrt;
			route_deref (rt);
		}
		else
			prevrt = &rt->next;
	
	}
	
	DEBUG (("route_del: no matching route found"));
	return 0;
}

/*
 * Delete all routes referring to interface `nif'
 */
void
route_flush (struct netif *nif)
{
	struct route *rt, *nextrt, **prevrt;
	short i;
	
	if (defroute && defroute->nif == nif)
	{
		route_deref (defroute);
		defroute = 0;
	}
	
	for (i = 0; i < RT_HASH_SIZE; i++)
	{
		prevrt = &allroutes[i];
		for (rt = *prevrt; rt; rt = nextrt)
		{
			nextrt = rt->next;
			if (rt->nif == nif && !(rt->flags & RTF_LOCAL))
			{
				*prevrt = nextrt;
				route_deref (rt);
			}
			else
				prevrt = &rt->next;
		}
	}
}

/*
 * Handle routing ioctl()'s. The only tricky part is to figure out the
 * interface (which is not passed to us!) over which the route should go.
 * But we can find it by looking up the interface which is on the same
 * network as the destination/gateway.
 */
long
route_ioctl (short cmd, long arg)
{
	struct rtentry *rte = (struct rtentry *) arg;
	ulong mask, net, gway = INADDR_ANY;
	short flags;
	
	if (cmd != SIOCADDRT && cmd != SIOCDELRT)
		return ENOSYS;
	
	if (p_geteuid ())
		return EACCES;
	
	if (rte->rt_dst.sa_family != AF_INET)
	{
		DEBUG (("route_ioctl: dst not AF_INET"));
		return EAFNOSUPPORT;
	}
	
	flags = rte->rt_flags;
	net = SIN (&rte->rt_dst)->sin_addr.s_addr;
	
	mask = ip_netmask (net);
	if (mask == 0)
	{
		DEBUG (("if_ioctl: bad dst address"));
		return EADDRNOTAVAIL;
	}
	
	if ((net & ~mask) != 0)
	{
		flags |= RTF_HOST;
		mask = 0xffffffff;
	}
	else
		flags &= ~RTF_HOST;
	
	switch (cmd)
	{
		case SIOCADDRT:
		{
			struct netif *nif;
			
			if (flags & RTF_GATEWAY)
			{
				if (rte->rt_gateway.sa_family != AF_INET)
				{
					DEBUG (("route_ioctl: gateway not AF_INET"));
					return EAFNOSUPPORT;
				}
				gway = SIN (&rte->rt_gateway)->sin_addr.s_addr;
				nif = if_net2if (gway);
			}
			else
				nif = if_net2if (net);
			
			if (!nif)
			{
				DEBUG (("route_ioctl: Dst net unreachable"));
				return ENETUNREACH;
			}
			
			return route_add (nif, net, mask, gway, flags, RT_TTL,
						rte->rt_metric);
		}
		case SIOCDELRT:
		{
			return route_del (net, mask);
		}
		default:
			return ENOSYS;
	}
}
