/*
 *	Autodial driver.
 *
 *      Please note: Since this driver needs access to the ppp? netif
 *                   structures it has to be loaded AFTER the ppp interface!!!
 *
 *	Started 07/11/94, Torsten Lang.
 */

# include "global.h"
# include "netinfo.h"

# include "buf.h"
# include "inet4/if.h"
# include "inet4/in.h"
# include "inet4/ip.h"

# include <mint/dcntl.h>
# include <mint/file.h>
# include <mint/signal.h>

# include "ppp.h"


# define PPPF_LINKED   0x01 /* interface is linked to device */
# define DIAL_CHANNELS 4
# define PPP_MTU       1500
# if PPP_CHANNELS > 9
# error Number of channels is limited to 9!
# endif

/* Structures from ppp.c. These will go when dial.c will */
/* be integrated into ppp.xif.                           */

struct pppstats
{
	ulong	i_ip;
	ulong	i_other;
	ulong	i_tolong;
	ulong	i_toshort;
	ulong	i_badcheck;
	ulong	i_badcode;
	ulong	i_badvj;
	ulong	i_qfull;
	ulong	i_nomem;
	ulong	o_ip;
	ulong	o_other;
	ulong	o_qfull;
	ulong	o_nomem;
};

/* PPP control block */
struct ppp
{
	short		links;		/* # of open file descr. */
	struct netif	*nif;		/* backlink to interface */
	short		flags;		/* misc flags */
# define PPPF_LINKED	0x01		/* interface is linked to device */
# define PPPF_DROP	0x02		/* drop next packet */

	unsigned long	opts;		/* user settable options */
	char		esc;		/* next char escaped? */

	unsigned long	xmap[8];	/* transmit asynch. char map */
	unsigned long	rmap[1];	/* receive asynch. char map */
	unsigned long	mru;		/* max. recv unit */

	struct ifq	rcvq;		/* /dev/ppp? recv queue */

	long		rsel;		/* selecting processes */
	long		wsel;
	long		xsel;

	short		pgrp;		/* pid/pgrp to send SIGIO to */

	BUF		*snd_buf;	/* BUF currently beeing sent */
	char		*snd_head;	/* head pointer */
	char		*snd_tail;	/* tail pointer */

	BUF		*rcv_buf;	/* BUF currently beeing received */
	char		*rcv_head;	/* head pointer */
	char		*rcv_tail;	/* tail pointer */

	struct slbuf	*slbuf;		/* serial buffer */
	struct slcompress *comp;	/* states for VJ compression */
	struct pppstats	stat;		/* statistics */
};

typedef enum {offline, dialling, online} status;

/* our netif structures */
static struct netif if_dial[DIAL_CHANNELS];
static int if_connect_timeout[DIAL_CHANNELS];
static int if_ref_timeout[DIAL_CHANNELS];
static int if_timeout[DIAL_CHANNELS];
static status if_states[DIAL_CHANNELS];
static int in_dial_output = 0;

/*
 * Prototypes for our service functions
 */
static long	dial_open	(struct netif *);
static long	dial_close	(struct netif *);
static long	dial_output	(struct netif *, BUF *, char *, short, short);
static void	dial_timeout	(struct netif *);
static long	dial_ioctl	(struct netif *, short, long);
static long	dial_config	(struct netif *, struct ifopt *);

/*
 * Prototypes of the device functions
 */
static long	dialdev_open	(FILEPTR *);
static long	dialdev_write	(FILEPTR *, const char *, long);
static long	dialdev_read	(FILEPTR *, char *, long);
static long	dialdev_lseek	(FILEPTR *, long, int);
static long	dialdev_ioctl	(FILEPTR *, int, void *);
static long	dialdev_datime	(FILEPTR *, ushort *, int);
static long	dialdev_close	(FILEPTR *, int);
static long	dialdev_select	(FILEPTR *, long, int);
static void	dialdev_unselect(FILEPTR *, long, int);

static char dial_buffer[4];

static short dial_links[DIAL_CHANNELS];
static long  dial_rsel[DIAL_CHANNELS];

static struct devdrv dial_dev =
{
	dialdev_open, dialdev_write, dialdev_read,
	dialdev_lseek, dialdev_ioctl, dialdev_datime,
	dialdev_close, dialdev_select, dialdev_unselect
};

static struct dev_descr dial_desc =
{
	driver:		&dial_dev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};


/* local functions */

struct ifaddr *
if_af2ifaddr (struct netif *nif, short family)
{
	struct ifaddr *ifa;
	
	for (ifa = nif->addrlist; ifa; ifa = ifa->next)
		if (ifa->family == family)
			break;
	
	return ifa;
}

/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long
dial_open (struct netif *nif)
{
	if (!nif->data)
		return -1;
	return 0;
}

/*
 * Opposite of dial_open(), is called when 'ifconfig down' on this interface
 * is done and the interface was up before.
 */
static long
dial_close (struct netif *nif)
{
	return 0;
}

static void
dial_timeout (struct netif *nif)
{
	if (!in_dial_output && if_timeout[nif->unit])
	{
		struct pppstats *stat = &((struct ppp *)(((struct netif *)nif->data)->data))->stat;
		
		/* Update error counters */
		nif->out_errors = stat->o_qfull    +
		                  stat->o_nomem;
		nif->in_errors  = stat->i_tolong   +
		                  stat->i_toshort  +
		                  stat->i_badcheck +
		                  stat->i_tolong   +
		                  stat->i_badcode  +
		                  stat->i_badvj    +
		                  stat->i_qfull    +
		                  stat->i_nomem;
		
		/* Dequeue any queued packets when ppp is linked */
		if ((((struct ppp *)((struct netif *)nif->data)->data)->flags & PPPF_LINKED)                   &&
		    !(((struct ppp *)((struct netif *)nif->data)->data)->opts & PPPO_IP_DOWN)                  &&
		    ((((struct netif *)nif->data)->flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING)))
		{
			BUF *buf;
			struct ifaddr *ifa1, *ifa2;
			
			/* hack: defer enqueued packets a little bit... */
			if (if_states[nif->unit] == dialling)
			{
				ifa1 = if_af2ifaddr(nif, AF_INET);
				ifa2 = if_af2ifaddr((struct netif *)nif->data, AF_INET);
				if (memcmp(ifa1, ifa2, sizeof(ifa1->addr)))
					memcpy(ifa1, ifa2, sizeof(ifa1->addr));
				else
					if_states[nif->unit] = online;
			}
			else
			{
				if ((ifa1 = if_af2ifaddr((struct netif *)nif->data, AF_INET)))
				{
					while ((buf = if_dequeue (&nif->snd)))
					{
						IP_SADDR(buf) = ((struct sockaddr_in *)&ifa1->addr)->sin_addr.s_addr;
						dial_output(nif, buf, NULL, 0, PKTYPE_IP);
					}
				}
			}
		}
		else if (if_states[nif->unit] == online)
			/* connection was lost before timeout! */
			if_timeout[nif->unit] = 1;
		
		/* Check if ip output counter of corresponding ppp if has changed */
		if (stat->i_ip == nif->in_packets)
		{
			/* counter did not change -> do timeout */
			if (!dial_buffer[nif->unit])
			{
				if (!--if_timeout[nif->unit])
				{
					if ((((struct ppp *)((struct netif *)nif->data)->data)->flags & PPPF_LINKED)                   &&
					    !(((struct ppp *)((struct netif *)nif->data)->data)->opts & PPPO_IP_DOWN)                  &&
					    ((((struct netif *)nif->data)->flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING)))
					{
						dial_buffer[nif->unit] = '0'+nif->unit+0x80;
						if (dial_rsel[nif->unit])
						{
							wakeselect(dial_rsel[nif->unit]);
							if_states[nif->unit] = offline;
						}
					}
				}
			}
		}
		else
		{
			/* counter changed -> reset the timeout and copy counter to ours */
			if_timeout[nif->unit] = if_ref_timeout[nif->unit];
			nif->in_packets = stat->i_ip;
		}
	}
}

/*
 * Output function - initiates dialling if necessary and passes the packets
 * to the corresponding ppp interface.
 */
 
static long
dial_output (struct netif *nif, BUF *buf, char *hwaddr, short hwlen, short pktype)
{
	long retcode;
	
	if (pktype != PKTYPE_IP)
	{
		buf_deref (buf, BUF_NORMAL);
		DEBUG (("dial_output: unsupported packet type"));
		return EINVAL;
	}
	
	in_dial_output++;
	if (!(((struct ppp *)((struct netif *)nif->data)->data)->flags & PPPF_LINKED)                   ||
	    (((struct ppp *)((struct netif *)nif->data)->data)->opts & PPPO_IP_DOWN)                    ||
	    !((((struct netif *)nif->data)->flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING)))
	{
		if (!if_timeout[nif->unit])
		{
			if (!dial_buffer[nif->unit])
			{
				dial_buffer[nif->unit] = '0'+nif->unit;
				if (dial_rsel[nif->unit])
				{
					if_states[nif->unit] = dialling;
 					wakeselect(dial_rsel[nif->unit]);
				}
				
				/* We only reset the timeout value when sending. This is not ok but should not */
				/* cause trouble since we always have some kind of handshaking...              */
				if_timeout[nif->unit] = if_connect_timeout[nif->unit];
			}
		}
do_enqueue:
		retcode = if_enqueue (&nif->snd, buf, buf->info);
		if (retcode)
		{
			nif->out_errors++;
			DEBUG (("dial_output: dial%d: cannot enqueue", nif->unit));
		}
		in_dial_output--;			
		return retcode;
	}
	else if (if_states[nif->unit] == dialling)
	{
		struct ifaddr *ifa1, *ifa2;
		
		ifa1 = if_af2ifaddr (nif, AF_INET);
		ifa2 = if_af2ifaddr ((struct netif *) nif->data, AF_INET);
		if (memcmp (ifa1, ifa2, sizeof (ifa1->addr)))
			memcpy (ifa1, ifa2, sizeof(ifa1->addr));
		else
			if_states[nif->unit] = online;
		
		goto do_enqueue;
	}
	
	if_timeout[nif->unit] = if_ref_timeout[nif->unit];
	in_dial_output--;
	
	retcode = ((struct netif *) nif->data)->output ((struct netif *) nif->data, buf, hwaddr, hwlen, pktype);
	if (!retcode)
		nif->out_packets++;
	
	return retcode;
}

/*
 * MintNet notifies you of some noteable IOCLT's. Usually you don't
 * need to act on them because MintNet already has done so and only
 * tells you that an ioctl happened.
 *
 * One useful thing might be SIOCGLNKFLAGS and SIOCSLNKFLAGS for setting
 * and getting flags specific to your driver. For an example how to use
 * them look at slip.c
 */
static long
dial_ioctl (struct netif *nif, short cmd, long arg)
{
	struct ifreq *ifr;
	
	switch (cmd)
	{
		case SIOCSIFNETMASK:
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
			return 0;
		
		case SIOCSIFOPT:
			/*
			 * Interface configuration, handled by dial_config()
			 */
			ifr = (struct ifreq *)arg;
			return dial_config (nif, ifr->ifru.data);
	}
	
	return ENOSYS;
}

/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that dial_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long
dial_config (struct netif *nif, struct ifopt *ifo)
{
# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))
	
	if (!STRNCMP ("debug"))
	{
		/*
		 * turn debuggin on/off
		 */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		DEBUG (("dial: debug level is %ld", ifo->ifou.v_long));
	}
	else if (!STRNCMP ("log"))
	{
		/*
		 * set log file
		 */
		if (ifo->valtype != IFO_STRING)
			return ENOENT;
		DEBUG (("dial: log file is %s", ifo->ifou.v_string));
	}
	else if (!STRNCMP ("timeout"))
	{
		/*
		 * set timeout value
		 */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		if_ref_timeout[nif->unit] = (int)ifo->ifou.v_long;
		DEBUG (("dial%d: new timeout value is %lds", nif->unit, (long)ifo->ifou.v_long));
	}
	else if (!STRNCMP ("waitconnect"))
	{
		/*
		 * set timeout value
		 */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		if_connect_timeout[nif->unit] = (int)ifo->ifou.v_long;
		DEBUG (("dial%d: new connect timeout value is %lds", nif->unit, (long)ifo->ifou.v_long));
	}
	else
		return ENOSYS;
	
	return 0;
}

/*
 * Initialization. This is called when the driver is loaded. If you
 * link the driver with main.o and init.o then this must be called
 * driver_init() because main() calles a function with this name.
 *
 * You should probe for your hardware here, setup the interface
 * structure and register your interface.
 *
 * This function should return 0 on success and != 0 if initialization
 * fails.
 */
long
driver_init (void)
{
	static char message[100];
	static char my_file_name[128];
	struct netif *ppp_nif;
	short i;
	
	ksprintf (message, "dial?: DIAL driver v0.2 (dial[0-%d]) (w) 1999 by Torsten Lang\n\r", DIAL_CHANNELS-1);
	c_conws (message);
	for (i = 0; i < DIAL_CHANNELS; i++)
	{
		if_ref_timeout[i] = 45;     /* default timeout value in seconds */
		if_connect_timeout[i] = 60; /* default timeout value in seconds */
		if_states[i] = offline;
		strcpy (if_dial[i].name, "dial");
		if_dial[i].unit        = i;
		if_dial[i].metric      = 0;
		if_dial[i].flags       = IFF_POINTOPOINT;
		if_dial[i].mtu         = 1500;
		if_dial[i].timer       = 1000; /* at the moment the timeout routine is always called every second */
		if_dial[i].hwtype      = HWTYPE_PPP;
		if_dial[i].rcv.maxqlen = IF_MAXQ;
		if_dial[i].snd.maxqlen = IF_MAXQ;
		if_dial[i].open        = dial_open;
		if_dial[i].close       = dial_close;
		if_dial[i].output      = dial_output;
		if_dial[i].ioctl       = dial_ioctl;
		if_dial[i].timeout     = dial_timeout;
		if_dial[i].maxpackets  = 0;
		if_dial[i].data        = NULL;
		if_register(if_dial+i);
		ppp_nif = if_dial[i].next;
		while (ppp_nif)
		{
			if (!stricmp (ppp_nif->name, "ppp") && ppp_nif->unit == i)
			{
				if_dial[i].data    = ppp_nif;
				break;
			}
			ppp_nif = ppp_nif->next;
		}
		if (!ppp_nif)
		{
			ksprintf (message, "DIAL%d: Initialization failed, interface ppp%d not found.\r\n", i, i);
			c_conws (message);
		}
		else
		{
			char devname[14];
			
			dial_desc.dinfo = i;
			ksprintf (devname, "u:\\dev\\dial%d", i);
			dial_links[i] = 0;
			d_cntl (DEV_INSTALL, devname, (long) &dial_desc);
		}
	}
	
	if (NETINFO->fname)
	{
		strncpy (my_file_name, NETINFO->fname, sizeof (my_file_name));
		my_file_name[sizeof (my_file_name) - 1] = '\0';
	}	
	
	return 0;
}

/*
 * /dev/dial? device driver.
 *
 * read() expect the following structure:
 *
 * buf[0] -- operation to perform
 *           '0'-'3' for dialling (corresponding ppp interface should be set up
 *           '0'-'3' or'd with 0x80 for hangup
 *
 * fcntl (pppfd, FIONREAD, &size) returns in 'size' the size of the
 * next frame or zero of none is available for reading.
 */

static long
dialdev_open (FILEPTR *fp)
{
	dial_links[fp->fc.aux]++;
	return 0;
}

static long
dialdev_write (FILEPTR *fp, const char *buf, long nbytes)
{
	return EACCES;
}

static long
dialdev_read (FILEPTR *fp, char *buf, long nbytes)
{
	if (!nbytes || !dial_buffer[fp->fc.aux])
		return 0;
	*buf = dial_buffer[fp->fc.aux];
	dial_buffer[fp->fc.aux]='\0';
	return 1;
}

static long
dialdev_lseek (FILEPTR *fp, long where, int whence)
{
	return EACCES;
}

static long
dialdev_ioctl (FILEPTR *fp, int cmd, void *arg)
{
	switch (cmd)
	{
		case FIONREAD:
			if (!dial_buffer[fp->fc.aux])
				*(long *) arg = 0;
			else
				*(long *) arg = 1;
			return 0;
		
		case FIONWRITE:
			*(long *) arg = 0;
			return 0;
		
		case FIOEXCEPT:
			*(long *) arg = 0;
			return 0;
	}
	
	return ENOSYS;
}

static long
dialdev_datime (FILEPTR *fp, ushort *timeptr, int mode)
{
	if (mode == 0)
	{
		timeptr[0] = t_gettime ();
		timeptr[1] = t_getdate ();
		
		return 0;
	}
	
	return EACCES;
}

static long
dialdev_close (FILEPTR *fp, int pid)
{
	if (dial_links[fp->fc.aux])
		dial_links[fp->fc.aux]--;
	return 0;
}

static long
dialdev_select (FILEPTR *fp, long proc, int mode)
{
	switch (mode)
	{
		case O_RDONLY:
			if (dial_buffer[fp->fc.aux])
				return 1;
			if (!dial_rsel[fp->fc.aux])
			{
				dial_rsel[fp->fc.aux] = proc;
				return 0;
			}
			return 2;
		
		case O_WRONLY:
			return 2;
		
		case O_RDWR:
			return 2;
	}
	
	return 0;
}

static void
dialdev_unselect (FILEPTR *fp, long proc, int mode)
{
	switch (mode)
	{
		case O_RDONLY:
			if (proc == dial_rsel[fp->fc.aux])
				dial_rsel[fp->fc.aux] = 0;
			break;
		
		case O_WRONLY:
			break;
		
		case O_RDWR:
			break;
	}
}
