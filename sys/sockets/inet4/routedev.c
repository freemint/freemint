/*
 *	This file implements /dev/route. It is intended for controlling
 *	the behavior of the IP router and getting information
 *	about it.
 *
 *	02/28/94, kay roemer.
 */

# include "routedev.h"

# include "if.h"
# include "in.h"
# include "route.h"

# include "dummydev.h"
# include "net.h"
# include "socket.h"
# include "util.h"

# include <mint/file.h>


/*
 * read() obtains this structure for every route
 */
struct route_info
{
	char		nif[IF_NAMSIZ];	/* name of the interface */
	struct rtentry	rt;		/* route info */
};

static long routedev_read (FILEPTR *, char *, long);

static DEVDRV routedev =
{
	open:		dummydev_open,
	write:		dummydev_write,
	read:		routedev_read,
	lseek:		dummydev_lseek,
	ioctl:		dummydev_ioctl,
	datime:		dummydev_datime,
	close:		dummydev_close,
	select:		dummydev_select,
	unselect:	dummydev_unselect
};

static struct dev_descr routedev_descr =
{
	driver:		&routedev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};

static char routedev_name[] = "u:\\dev\\route";


long
routedev_init (void)
{
	return dummydev_init (routedev_name, &routedev_descr);
}	

static long
routedev_read (FILEPTR *f, char *buf, long nbytes)
{
	struct route *rt = NULL;
	struct route_info info, *infop = (struct route_info *) buf;
	int i, j;
	long space;
	
	for (space = nbytes; space >= sizeof (info); f->pos++)
	{
		rt = defroute;
		i = rt ? f->pos - 1 : f->pos;
		for (j = 0; j < RT_HASH_SIZE && i >= 0; j++)
		{
			rt = allroutes[j];
			for (; rt && --i >= 0; rt = rt->next);
		}
		
		if (j >= RT_HASH_SIZE)
			break;
		
		bzero (&info, sizeof (info));
		SIN (&info.rt.rt_dst)->sin_family = AF_INET;
		SIN (&info.rt.rt_dst)->sin_addr.s_addr = rt->net;
		
		if (rt->flags & RTF_GATEWAY)
		{
			SIN (&info.rt.rt_gateway)->sin_family = AF_INET;
			SIN (&info.rt.rt_gateway)->sin_addr.s_addr = rt->gway;
		}
		
		info.rt.rt_flags  = rt->flags;
		info.rt.rt_metric = rt->metric;
		info.rt.rt_refcnt = rt->refcnt;
		info.rt.rt_use    = rt->usecnt;
		
		ksprintf (info.nif, "%s%d", rt->nif->name, rt->nif->unit);
		DEBUG (("routedev_read: %s", info.nif));
		
		*infop++ = info;
		space -= sizeof (info);
	}
	
	return (nbytes - space);
}
