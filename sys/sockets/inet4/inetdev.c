/*
 *	This file implements /dev/inet. It is intended for controlling
 *	the behavior of the inet domain layer and getting information
 *	about it. netstat(8) is implemented using this device.
 *
 *	05/27/94, kay roemer.
 */

# include "inetdev.h"

# include "in.h"
# include "inet.h"
# include "tcp.h"
# include "udp.h"

# include "dummydev.h"
# include "net.h"
# include "socket.h"
# include "util.h"

# include <mint/file.h>


/*
 * read() obtains this structure for every inet domain socket
 */
struct inet_info
{
	short		proto;		/* protocol, IPPROTO_* */
	long		sendq;		/* bytes in send queue */
	long		recvq;		/* bytes in recv queue */
	short		state;		/* state */
	struct sockaddr_in laddr;	/* local address */
	struct sockaddr_in faddr;	/* foreign address */
};

struct _datas
{
	void (*getinfo)(struct inet_info *, struct in_data *);
	struct in_proto	*proto;
};

static void tcp_getinfo (struct inet_info *, struct in_data *);
static void udp_getinfo (struct inet_info *, struct in_data *);
static long inetdev_read (FILEPTR *, char *, long);

static struct _datas allindatas[] =
{
	{ tcp_getinfo, &tcp_proto },
	{ udp_getinfo, &udp_proto },
	{ 0, 0 }
};

static DEVDRV inetdev =
{
	open:		dummydev_open,
	write:		dummydev_write,
	read:		inetdev_read,
	lseek:		dummydev_lseek,
	ioctl:		dummydev_ioctl,
	datime:		dummydev_datime,
	close:		dummydev_close,
	select:		dummydev_select,
	unselect:	dummydev_unselect
};

static struct dev_descr inetdev_descr =
{
	driver:		&inetdev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};

static char inetdev_name[] = "u:\\dev\\inet";

long
inetdev_init (void)
{
	return dummydev_init (inetdev_name, &inetdev_descr);
}

/*
 * Fill 'info' with information about TCP socket pointed at by 'data'
 */
static void
tcp_getinfo (struct inet_info *info, struct in_data *data)
{
	struct tcb *tcb = data->pcb;
	
	info->state = tcb->state;
	info->sendq = data->snd.curdatalen;
	info->recvq = tcp_canread (data);
}

/*
 * Fill 'info' with information about UDP socket pointed at by 'data'
 */
static void
udp_getinfo (struct inet_info *info, struct in_data *data)
{
	info->sendq = 0;
	info->recvq = data->rcv.curdatalen;
	info->state = data->flags & IN_ISCONNECTED
		? TCBS_ESTABLISHED : TCBS_CLOSED;
}

static long
inetdev_read (FILEPTR *fp, char *buf, long nbytes)
{
	struct in_data *inp = NULL;
	struct inet_info info, *infop = (struct inet_info *) buf;
	struct _datas *datap;
	long space;
	int i;
	
	for (space = nbytes; space >= sizeof (info); fp->pos++)
	{
		datap = allindatas;
		
		for (i = fp->pos; datap->proto; datap++)
		{
			inp = datap->proto->datas;
			for (; inp && --i >= 0; inp = inp->next)
				;
			
			if (i < 0)
				break;
		}
		
		if (datap->proto == 0)
			break;
		
		info.proto = inp->protonum;
		
		info.laddr.sin_family = AF_INET;
		info.laddr.sin_addr.s_addr = inp->src.addr;
		info.laddr.sin_port = inp->src.port;
		
		info.faddr.sin_family = AF_INET;
		info.faddr.sin_addr.s_addr = inp->dst.addr;
		info.faddr.sin_port = inp->dst.port;
		
		(*datap->getinfo)(&info, inp);
		
		*infop++ = info;
		space -= sizeof (info);
	}
	
	return (nbytes - space);
}
