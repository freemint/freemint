
# ifndef _route_h
# define _route_h

# include "global.h"

# include "if.h"


# define RT_HASH_SIZE		256
# define RT_TTL			100

struct route
{
	ulong		net;
	ulong		mask;
	ulong		gway;
	ulong		metric;
	struct netif	*nif;
	short		ttl;
	struct route	*next;
	short		flags;
	long		usecnt;
	short		refcnt;
};

INLINE void
route_deref (struct route *rt)
{
	if (rt && --rt->refcnt <= 0)
		kfree (rt);
}

# define RTF_UP		0x0001
# define RTF_GATEWAY	0x0002
# define RTF_HOST	0x0004
# define RTF_REJECT	0x0008
# define RTF_STATIC	0x0010
# define RTF_DYNAMIC	0x0020
# define RTF_MODIFIED	0x0040
# define RTF_MASK	0x0080
# define RTF_LOCAL	0x0100

/* This BSD struct is used only for ioctl()'s */
struct rtentry
{
	ulong		rt_hash;	/* hash key */
	struct sockaddr	rt_dst;		/* key */
	struct sockaddr	rt_gateway;	/* value */
	short		rt_flags;	/* up/down?, host/net */
	ulong		rt_metric;	/* distance metric */
	short		rt_refcnt;	/* # held references */
	ulong		rt_use;		/* raw # packets forwarded */
	struct netif	*rt_ifp;	/* interface to use */
};

extern struct route *allroutes[RT_HASH_SIZE];
extern struct route *defroute;
extern struct route rt_primary;		/* fake broadcast route */

void		route_init	(void);

void		route_flush	(struct netif *);
struct route *	route_get	(ulong);
long		route_del	(ulong, ulong);
long		route_add	(struct netif *, ulong, ulong, ulong, short, short, long);
struct route *	route_alloc	(struct netif *, ulong, ulong, ulong, short, short, long);
long		route_ioctl	(short, long);


# endif /* _route_h */
