/*
 *	This file implementes /dev/arp, from which the arp table can be
 *	read.
 *
 *	12/15/94, Kay Roemer.
 */

# include "arpdev.h"

# include "arp.h"
# include "if.h"
# include "ifeth.h"
# include "in.h"
# include "inet.h"

# include "dummydev.h"
# include "net.h"
# include "socket.h"
# include "timer.h"
# include "util.h"

# include <mint/file.h>


/*
 * structure that can be read from /dev/arp
 */
struct arp_info
{
	struct sockaddr_hw	praddr;
	struct sockaddr_hw	hwaddr;
	ushort			flags;
	ushort			links;
	ulong			tmout;
};

/*
 * /dev/arp device driver
 */
static long arpdev_read (FILEPTR *, char *, long);

static DEVDRV arpdev =
{
	open:		dummydev_open,
	write:		dummydev_write,
	read:		arpdev_read,
	lseek:		dummydev_lseek,
	ioctl:		dummydev_ioctl,
	datime:		dummydev_datime,
	close:		dummydev_close,
	select:		dummydev_select,
	unselect:	dummydev_unselect
};

static struct dev_descr arpdev_descr =
{
	driver:		&arpdev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};

static char arpdev_name[] = "u:\\dev\\arp";

long
arpdev_init (void)
{
	return dummydev_init (arpdev_name, &arpdev_descr);
}

static long
arpdev_read (FILEPTR *fp, char *buf, long nbytes)
{
	struct arp_info info, *infop = (struct arp_info *) buf;
	struct arp_entry *are = 0;
	long space, t;
	int i, j;
	
	for (space = nbytes; space >= sizeof (info); fp->pos++)
	{
		i = fp->pos;
		for (j = 0; j < ARP_HASHSIZE && i >= 0; j++)
		{
			are = arptab[j];
			for (; are && --i >= 0; are = are->prnext);
		}
		
		if (j >= ARP_HASHSIZE)
			break;
		
		bzero (&info, sizeof (info));
		
		/*
		 * Protocoll address
		 */
		info.praddr.shw_family = AF_LINK;
		info.praddr.shw_len = are->praddr.len;
		info.praddr.shw_type = are->prtype;
		if (are->flags & ATF_PRCOM)
			memcpy (info.praddr.shw_addr, are->praddr.addr,
				are->praddr.len);
		
		/*
		 * Hardware address
		 */
		info.hwaddr.shw_family = AF_LINK;
		info.hwaddr.shw_len = are->hwaddr.len;
		info.hwaddr.shw_type = are->hwtype;
		if (are->flags & ATF_HWCOM)
			memcpy (info.hwaddr.shw_addr, are->hwaddr.addr,
				are->hwaddr.len);
		
		info.flags = are->flags;
		info.links = are->links;
		t = event_delta (&are->tmout);
		info.tmout = (t < 0) ? 0 : t * EVTGRAN;
		
		*infop++ = info;
		space -= sizeof (info);
	}
	
	return (nbytes - space);
}
