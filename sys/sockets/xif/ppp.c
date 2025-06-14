/*
 *	Simple PPP implementation. The stuff here does de- and encapsulation,
 *	sending and receiving of PPP frames.
 *
 *	There is a simple interface to user level PPP daemons, which uses the
 *	datagram-oriented /dev/pppx devices, where 'x' is the unit number of
 *	the associated network interface.
 *
 *	Lots of options (like functionality for demand dialing and other
 *	nifty things) are missing.
 *
 *	06/25/94, Kay Roemer.
 */

# include "global.h"
# include "netinfo.h"

# include "buf.h"
# include "inet4/if.h"
# include "inet4/in.h"

# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/signal.h"
# include "mint/sockio.h"
# include "mint/stat.h"

# include "ppp.h"
# include "serial.h"
# include "slcompress.h"


# define PPP_VERSION	"0.5"
# define PPP_MTU	1500		/* def. maximum transmission unit */
# define PPP_CHANNELS	4		/* number of PPP units */
# define PPP_RESERVE	100		/* leave that much space in BUF's */

/* PPP special chars */
# define ESC		0x7d		/* escape char */
# define FLAG		0x7e		/* frame start/end */
# define TRANS		0x20		/* XOR with this to (un)esc char */
# define ADDR		0xff		/* 'all station' HDLC address */
# define CTRL		0x03		/* UI with P/F bit cleared */

/* Macros for dealing with asynch. character maps */
# define MAP_ZERO(m)	(memset (m, 0, sizeof (m)))
# define MAP_SET(m,b)	((m)[(uchar)(b) >> 5] |= 1L << ((b) & 0x1f))
# define MAP_CLR(m,b)	((m)[(uchar)(b) >> 5] &= ~(1L << ((b) & 0x1f)))
# define MAP_ISSET(m,b)	((m)[(uchar)(b) >> 5] & (1L << ((b) & 0x1f)))

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

	ulong	opts;		/* user settable options */
	char		esc;		/* next char escaped? */

	ulong	xmap[8];	/* transmit asynch. char map */
	ulong	rmap[1];	/* receive asynch. char map */
	ulong	mru;		/* max. recv unit */

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

static long	ppp_link	(struct iflink *);
static long	ppp_open	(struct netif *);
static long	ppp_close	(struct netif *);
static long	ppp_output	(struct netif *, BUF *, const char *, short, short);
static long	ppp_ioctl	(struct netif *, short, long);
static short	ppp_send	(struct slbuf *);
static short	ppp_recv	(struct slbuf *);
static ushort	ppp_fcs		(const char *, long);
static void	ppp_recv_frame	(struct ppp *, BUF *);
static void	ppp_recv_usr	(struct ppp *, BUF *);
static long	ppp_send_frame	(struct ppp *, BUF *, short);
static void	ppp_build_frame	(struct ppp *, BUF *);

static long	pppdev_open	(FILEPTR *);
static long	pppdev_write	(FILEPTR *, const char *, long);
static long	pppdev_read	(FILEPTR *, char *, long);
static long	pppdev_lseek	(FILEPTR *, long, int);
static long	pppdev_ioctl	(FILEPTR *, int, void *);
static long	pppdev_datime	(FILEPTR *, ushort *, int);
static long	pppdev_close	(FILEPTR *, int);
static long	pppdev_select	(FILEPTR *, long, int);
static void	pppdev_unselect	(FILEPTR *, long, int);

static struct netif	ppp_chan[PPP_CHANNELS];
static struct ppp	ppp_priv[PPP_CHANNELS];

static struct devdrv	ppp_dev =
{
	open:		pppdev_open,
	write:		pppdev_write,
	read:		pppdev_read,
	lseek:		pppdev_lseek,
	ioctl:		pppdev_ioctl,
	datime:		pppdev_datime,
	close:		pppdev_close,
	select:		pppdev_select,
	unselect:	pppdev_unselect
};

static struct dev_descr	ppp_desc =
{
	driver:		&ppp_dev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};

static ushort const fcstab[256] =
{
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

# define FCS_INIT	0xffffu
# define FCS_GOOD	0xf0b8u

static ushort
ppp_fcs (const char *cp, long len)
{
	register ushort fcs = FCS_INIT;
	
	while (--len >= 0)
		fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];
	
	return fcs;
}

static long
ppp_link (struct iflink *ifl)
{
	struct ppp *ppp;
	short i, j;
	
	for (i = 0; i < PPP_CHANNELS; ++i)
	{
		if (!(ppp_priv[i].flags & PPPF_LINKED))
			break;
	}
	
	if (i > PPP_CHANNELS)
	{
		DEBUG (("ppp_link: no free ppp channels"));
		return ENOMEM;
	}
	ppp = &ppp_priv[i];
	
	MAP_ZERO (ppp->rmap);
	MAP_ZERO (ppp->xmap);
	MAP_SET (ppp->xmap, ESC);
	MAP_SET (ppp->xmap, FLAG);
	for (j = 0; j < 32; ++j)
		MAP_SET (ppp->xmap, j);
	
	if (ppp->snd_buf)
		buf_deref (ppp->snd_buf, BUF_NORMAL);
	
	if (ppp->rcv_buf)
		buf_deref (ppp->rcv_buf, BUF_NORMAL);
	
	ppp->esc = 0;
	ppp->snd_buf  = 0;
	ppp->snd_head = 0;
	ppp->snd_tail = 0;
	ppp->rcv_buf  = 0;
	ppp->rcv_head = 0;
	ppp->rcv_tail = 0;
	ppp->comp = slc_init ();
	if (ppp->comp == 0)
	{
		DEBUG (("ppp_link: no space for VJ compression states"));
		return ENOMEM;
	}
	ppp->slbuf = serial_open (&ppp_chan[i],ifl->device,ppp_send,ppp_recv);
	
	if (ppp->slbuf == 0)
	{
		DEBUG (("ppp_link: no free serial channels"));
		slc_free (ppp->comp);
		ppp->comp = 0;
		return ENOMEM;
	}
	ppp->flags |= PPPF_LINKED;
	ksprintf (ifl->ifname, "%s%d", ppp_chan[i].name, ppp_chan[i].unit);
	return 0;
}

static long
ppp_open (struct netif *nif)
{
	struct ppp *ppp = nif->data;
	
	if (!(ppp->flags & PPPF_LINKED))
	{
		DEBUG (("ppp_open: chan %d not linked to device", nif->unit));
		return ENODEV;
	}
	
	return 0;
}

static long
ppp_close (struct netif *nif)
{
	struct ppp *ppp = nif->data;
	long r = 0;
	
	if (ppp->flags & PPPF_LINKED)
	{
		r = serial_close (ppp->slbuf);
		if (r < 0)
		{
			DEBUG (("ppp_close: cannot close serial channel"));
			return r;
		}
		slc_free (ppp->comp);
		ppp->comp = 0;
		ppp->flags &= ~PPPF_LINKED;
	}
	
	return 0;
}

static long
ppp_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	struct ppp *ppp = nif->data;
	
	if (pktype != PKTYPE_IP)
	{
		buf_deref (buf, BUF_NORMAL);
		DEBUG (("ppp_output: unsupported packet type"));
		return EINVAL;
	}
	
	return ppp_send_frame (ppp, buf, PPPPROTO_IP);
}

static long
ppp_ioctl (struct netif *nif, short cmd, long arg)
{
	struct iflink *ifl;
	struct ifreq *ifr;
	struct ppp *ppp = nif->data;
	long *l;
	
	switch (cmd)
	{
		case SIOCSIFLINK:
			return ppp_link ((struct iflink *) arg);
		
		case SIOCGIFNAME:
			ifl = (struct iflink *) arg;
			if (!(ppp->flags & PPPF_LINKED))
			{
				DEBUG (("ppp_ioctl: IFLINK: chan %d not linked",
					nif->mtu));
				return EINVAL;
			}
			strncpy (ifl->device, ppp->slbuf->dev, sizeof (ifl->device));
			return 0;
		
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
		case SIOCSIFNETMASK:
			return 0;
		
		/*
		 * The following to are aliases for PPPIOC[SG]FLAGS. That
		 * means you can use ifconfig ppp? linkNN to modify the
		 * PPP options.
		 */
		case SIOCGLNKFLAGS:
			ifr = (struct ifreq *)arg;
			ifr->ifru.flags = (short)ppp->opts;
			return 0;
		
		case SIOCSLNKFLAGS:
			ifr = (struct ifreq *)arg;
			ppp->opts = (ulong) ifr->ifru.flags;
			return 0;
		
		case SIOCGLNKSTATS:
			ifr = (struct ifreq *)arg;
			l = ifr->ifru.data;
			bzero (l, 40 * sizeof (long));
			memcpy (l, &ppp->stat, sizeof (ppp->stat));
			l += sizeof (ppp->stat) / sizeof (*l);
			if (ppp->comp)
				slc_getstats (ppp->comp, l);
			return 0;
	}
	
	return ENOSYS;
}

long
ppp_init (void)
{
	char devname[100];
	short i;
	
	ksprintf (devname, "PPP  v%s, %d channels\n\r", PPP_VERSION, PPP_CHANNELS);
	c_conws (devname);
	for (i = 0; i < PPP_CHANNELS; ++i)
	{
		strcpy (ppp_chan[i].name, "ppp");
		ppp_chan[i].unit = i;
		ppp_chan[i].metric = 0;
		ppp_chan[i].flags = IFF_POINTOPOINT;
		ppp_chan[i].mtu = PPP_MTU;
		ppp_chan[i].timer = 0;
		ppp_chan[i].hwtype = HWTYPE_PPP;
		
		ppp_chan[i].rcv.maxqlen = IF_MAXQ;
		ppp_chan[i].snd.maxqlen = IF_MAXQ;
		ppp_chan[i].open = ppp_open;
		ppp_chan[i].close = ppp_close;
		ppp_chan[i].output = ppp_output;
		ppp_chan[i].ioctl = ppp_ioctl;
		ppp_chan[i].timeout = 0;
		
		ppp_chan[i].data = &ppp_priv[i];
		ppp_priv[i].nif = &ppp_chan[i];
		ppp_priv[i].rcvq.maxqlen = 20;
		ppp_priv[i].mru = PPP_MTU;
		
		if_register (&ppp_chan[i]);
		
		ppp_desc.dinfo = i;
		ksprintf (devname, "u:\\dev\\ppp%d", i);
		d_cntl (DEV_INSTALL, devname, (long) &ppp_desc);
	}
	
	return 0;
}

static short
ppp_recv (struct slbuf *slb)
{
# define PPP_GETC() ({ char _c = buf[tail++]; tail &= mask; _c; })
	
	struct ppp *ppp = slb->nif->data;
	short tail, mask = slb->isize - 1;
	long space, nbytes;
	uchar c;
	char *cp, *buf;
	BUF *b;
	
	nbytes = SL_IUSED (slb);
	while (nbytes > 0)
	{
		/*
		 * We must be prepared to receive at least MRU bytes plus
		 * 6 bytes (for header and fcs) plus one extra byte so
		 * we don't drop full (MRU+6) sized packets. Use MRU+10
		 * to give it some slope.
		 *
		 * Also make sure we can always receive at least 1500 bytes
		 * long frames as rfc 1331 states.
		 *
		 * The MAX_HDR bytes before start are for uncompressing
		 * vj-compressed TCP frames.
		 */
		if (ppp->rcv_buf == 0)
		{
			long mru = MAX (PPP_MTU, ppp->mru) + 10;
			b = buf_alloc (mru+MAX_HDR, MAX_HDR, BUF_ATOMIC);
			if (!b)
			{
				ppp->stat.i_nomem++;
				return 0;
			}
			ppp->rcv_buf = b;
			ppp->rcv_head = b->dstart + mru;
			ppp->rcv_tail = b->dstart;
		}
		cp = ppp->rcv_tail;
		space = ppp->rcv_head - cp;
		buf = slb->ibuf;
		tail = slb->itail;
		while (space > 0 && nbytes > 0)
		{
			if (nbytes > space)
				nbytes = space;
			
			while (--nbytes >= 0) switch ((c = PPP_GETC ())) {
			case ESC:
				ppp->esc = TRANS;
				break;
			
			case FLAG:
				b = ppp->rcv_buf;
				if (ppp->esc || cp - b->dstart < 3 ||
				    ppp->flags & PPPF_DROP)
				    {
					long l = cp - b->dstart;
					if (ppp->esc)
						++ppp->stat.i_badcode;
					if (l > 0 && l < 4)
						++ppp->stat.i_toshort;
					ppp->flags &= ~PPPF_DROP;
					cp = b->dstart;
					ppp->esc = 0;
					break;
				}
				b->dend = cp;
				ppp_recv_frame (ppp, b);
				ppp->flags &= ~PPPF_DROP;
				ppp->esc = 0;
				ppp->rcv_buf = 0;
				slb->itail = tail;
				nbytes = SL_IUSED (slb);
				goto newpacket;
			
			default:
				if (c >= 0x20 || !MAP_ISSET (ppp->rmap, c))
				{
					*cp++ = c ^ ppp->esc;
					ppp->esc = 0;
				}
				break;
			}
			slb->itail = tail;
			nbytes = SL_IUSED (slb);
			space = ppp->rcv_head - cp;
		}
		if (space <= 0)
		{
			/*
			 * Receive buffer overflow
			 */
			ppp->stat.i_tolong++;
			slb->nif->in_errors++;
			ppp->flags |= PPPF_DROP;
			ppp->rcv_tail = cp = ppp->rcv_buf->dstart;
		}
		else
			ppp->rcv_tail = cp;
newpacket:
		;
	}
	
	return 0;
}

static short
ppp_send (struct slbuf *slb)
{
# define PPP_PUTC(_c) { buf[head++] = _c; head &= mask; }
	
	struct ppp *ppp = slb->nif->data;
	short head, mask = slb->osize - 1;
	long space, nbytes;
	uchar c;
	char *cp, *buf;
	BUF *b;
	
	space = SL_OFREE (slb) >> 1;
	while (space > 0)
	{
		buf = slb->obuf;
		head = slb->ohead;
		if (ppp->snd_buf == 0)
		{
			ppp->snd_buf = b = if_dequeue (&ppp->nif->snd);
			if (!b)
				return 0;
			ppp_build_frame (ppp, b);
			ppp->snd_head = b->dend;
			ppp->snd_tail = b->dstart;
			PPP_PUTC (FLAG);
			space--;
			if (ppp->wsel)
				wakeselect (ppp->wsel);
		}
		cp = ppp->snd_tail;
		nbytes = ppp->snd_head - cp;
		while (nbytes > 0 && space > 0)
		{
			if (space > nbytes)
				space = nbytes;

			while (--space >= 0)
			{
				c = *cp++;
				if (MAP_ISSET (ppp->xmap, c))
				{
					PPP_PUTC (ESC);
					PPP_PUTC (c ^ TRANS);
				}
				else
					PPP_PUTC (c);
			}
			slb->ohead = head;
			space = SL_OFREE (slb) >> 1;
			nbytes = ppp->snd_head - cp;
		}
		
		if (space > 0)
		{
			slb->nif->out_packets++;
			buf_deref (ppp->snd_buf, BUF_ATOMIC);
			ppp->snd_buf = 0;
			PPP_PUTC (FLAG);
			space--;
		}
		slb->ohead = head;
		ppp->snd_tail = cp;
	}
	
	return 0;
}

static long
ppp_send_frame (struct ppp *ppp, BUF *buf, short proto)
{
	BUF *nbuf;
	long r;
	
	nbuf = buf_reserve (buf, 4, BUF_RESERVE_START);
	if (!nbuf)
	{
		ppp->stat.o_nomem++;
		buf_deref (buf, BUF_NORMAL);
		return ENOMEM;
	}
	
	nbuf = buf_reserve (buf = nbuf, 2, BUF_RESERVE_END);
	if (!nbuf)
	{
		ppp->stat.o_nomem++;
		buf_deref (buf, BUF_NORMAL);
		return ENOMEM;
	}
	
	if (proto == PPPPROTO_IP)
		ppp->stat.o_ip++;
	else
		ppp->stat.o_other++;
	
	nbuf->dstart[-2] = (char)(proto >> 8);
	nbuf->dstart[-1] = (char)proto;
	if (proto == PPPPROTO_IP && ppp->opts & PPPO_IP_DOWN)
	{
		nbuf->dstart -= 2;
		*--nbuf->dstart = CTRL;
		*--nbuf->dstart = ADDR;
		ppp_recv_usr (ppp, nbuf);
		return 0;
	}
	
	r = if_enqueue (&ppp->nif->snd, nbuf, nbuf->info);
	if (r)
	{
		ppp->stat.o_qfull++;
		ppp->nif->out_errors++;
		DEBUG (("ppp_output: ppp%d: cannot enqueue", ppp->nif->unit));
		return r;
	}
	
	return serial_send (ppp->slbuf);
}

/*
 * Takes a frame built by ppp_send_frame and constructs a PPP
 * frame out of it according to the options set by PPPIOCSFLAGS.
 */
static void
ppp_build_frame (struct ppp *ppp, BUF *buf)
{
	ushort fcs, proto;
	uchar type;
	char *cp;
	
	cp = buf->dstart;
	proto = (cp[-2] << 8) | cp[-1];
	if (ppp->nif->bpf && BUF_LEAD_SPACE (buf) >= 4)
	{
		buf->dstart -= 4;
		buf->dstart[0] = 1; /* means 'output' */
		buf->dstart[1] = CTRL;
		bpf_input (ppp->nif, buf);
		buf->dstart += 4;
	}
	
	/*
	 * Do VJ header compression for TCP frames if compression is enabled.
	 */
	if (ppp->opts & PPPO_COMPRESS && proto == PPPPROTO_IP
		&& IP_PROTO (buf) == IPPROTO_TCP)
	{
		type = slc_compress (buf, ppp->comp,ppp->flags & PPPO_COMPCID);
		cp = buf->dstart;
		switch (type)
		{
			case TYPE_COMPRESSED_TCP:
				proto = PPPPROTO_COMP_TCP;
				break;
			
			case TYPE_UNCOMPRESSED_TCP:
				proto = PPPPROTO_UNCOMP_TCP;
				break;
		}
	}
	/*
	 * Store PPP protocol, compress if compressible and compression
	 * enabled.
	 */
	*--cp = proto & 0xff;
	if (!(ppp->opts & PPPO_PROT_COMP) || proto & 0xff00)
		*--cp = (char)(proto >> 8);
	
	/*
	 * Store PPP address and control field, compress if compression
	 * enabled.
	 */
	if (!(ppp->opts & PPPO_ADDR_COMP))
	{
		*--cp = CTRL;
		*--cp = ADDR;
	}
	buf->dstart = cp;
	cp = buf->dend;
	
	/*
	 * Calc and store FCS with highest term first.
	 */
	fcs = ~ppp_fcs (buf->dstart, buf->dend - buf->dstart);
	*cp++ = (char)fcs;
	*cp++ = (char)(fcs >> 8);
	buf->dend = cp;
}

/*
 * Pass a frame to /dev/ppp?.
 */

static volatile short havetimeout = 0;

static void
wakeup (PROC *p, long arg)
{
	int i;
	
	havetimeout = 0;
	for (i = 0; i < PPP_CHANNELS; ++i)
	{
		if (ppp_priv[i].rcvq.qfirst[IF_PRIORITIES-1])
			wake (IO_Q, (long)&ppp_priv[i]);
	} 
}


static void
ppp_recv_usr (struct ppp *ppp, BUF *buf)
{
	if (ppp->links <= 0)
	{
		buf_deref (buf, BUF_ATOMIC);
		return;
	}
	
	if (if_enqueue (&ppp->rcvq, buf, IF_PRIORITIES-1))
	{
		ppp->stat.i_qfull++;
		ppp->nif->in_errors++;
	}
	else
	{
		ppp->stat.i_other++;
		ppp->nif->in_packets++;
		/*
		 * The wake() may cause problems, 'cuz we are not in kernel
		 * (called from sld!). However, we are in Supervisor mode
		 * and no context switches will take place, so this should
		 * work ok.
		 */
		if (!havetimeout)
		{
			havetimeout = 1;
			addroottimeout (0, wakeup, 1);
		}
		
		if (ppp->rsel)
			wakeselect (ppp->rsel);
	}
	/*
	 * We signal the user process also in case the queue is full and
	 * no more packets can be enqueued to make sure it reads some
	 * packets soon.
	 */
	if (ikill && ppp->pgrp)
		ikill (ppp->pgrp, SIGIO);
}

/*
 * the memcpy() below is overlapping, but we know that it will work.
 * Just silence the warning.
 */
#pragma GCC diagnostic push
#if __GNUC_PREREQ(8,0)
#pragma GCC diagnostic ignored "-Wrestrict"
#endif

static void
ppp_recv_frame (struct ppp *ppp, BUF *b)
{
	ushort proto;
	uchar *cp;
	long len, r;
	
	cp = (unsigned char *)b->dstart;
	len = b->dend - b->dstart;
	
	if (len < 3 || ppp_fcs((char *)cp, len) != FCS_GOOD)
	{
		buf_deref (b, BUF_ATOMIC);
		if (len < 3)
			ppp->stat.i_toshort++;
		else
			ppp->stat.i_badcheck++;
		
		ppp->nif->in_errors++;
		return;
	}
	
	/*
	 * Check for compressed address and control field
	 */
	if (cp[0] == ADDR && cp[1] == CTRL)
	{
		cp += 2;
		len -= 2;
	}
	
	/*
	 * Check for compressed protocol field
	 */
	if (*cp & 1)
	{
		proto = *cp++;
		len--;
	}
	else
	{
		proto = ((ushort) cp[0] << 8) | cp[1];
		cp += 2;
		len -= 2;
	}
	if (len < 3)
	{
		buf_deref (b, BUF_ATOMIC);
		ppp->stat.i_toshort++;
		ppp->nif->in_errors++;
		return;
	}
	b->dstart = (char *)cp;
	b->dend -= 2;
	
	/*
	 * Check for VJ compressed TCP frames.
	 */
	r = 0;
	switch (proto)
	{
		case PPPPROTO_COMP_TCP:
			if (ppp->opts & PPPO_COMPRESS)
				r = slc_uncompress (b, TYPE_COMPRESSED_TCP, ppp->comp);
			cp = (unsigned char *)b->dstart;
			proto = PPPPROTO_IP;
			break;
		
		case PPPPROTO_UNCOMP_TCP:
			if (ppp->opts & PPPO_COMPRESS)
				r = slc_uncompress (b, TYPE_UNCOMPRESSED_TCP,
					ppp->comp);
			else if (ppp->opts & PPPO_AUTOCOMP &&
				 (r = slc_uncompress (b, TYPE_UNCOMPRESSED_TCP,
					ppp->comp)))
				ppp->opts |= PPPO_COMPRESS;
			cp = (unsigned char *)b->dstart;
			proto = PPPPROTO_IP;
			break;
		
		default:
			r = 1;
			break;
	}
	
	if (r == 0)
	{
		ppp->stat.i_badvj++;
		ppp->nif->in_errors++;
		buf_deref (b, BUF_ATOMIC);
		return;
	}
	
	if ((long) cp & 1)
	{
		/* Shit! -- must be word aligned */
		memcpy (cp-1, cp, len);
		b->dstart--;
		b->dend--;
	}
	
	if (proto == PPPPROTO_IP && ppp->nif->flags & IFF_UP
		&& !(ppp->opts & PPPO_IP_DOWN))
	{
	    	if (ppp->nif->bpf)
	    	{
			b->dstart -= 4;
			b->dstart[0] = 0; /* means 'input' */
			b->dstart[1] = CTRL;
			b->dstart[2] = (char)(proto >> 8);
			b->dstart[3] = (char)proto;
			bpf_input (ppp->nif, b);
			b->dstart += 4;
	    	}
		if (if_input (ppp->nif, b, 0, PKTYPE_IP))
		{
			ppp->stat.i_qfull++;
			ppp->nif->in_errors++;
		}
		else
		{
			ppp->stat.i_ip++;
			ppp->nif->in_packets++;
		}
	}
	else
	{
		/*
		 * Pass it to /dev/ppp?
		 */
		*--b->dstart = (char)proto;
		*--b->dstart = (char)(proto >> 8);
		*--b->dstart = CTRL;
		*--b->dstart = ADDR;
		if (ppp->nif->bpf)
		{
			*b->dstart = 0; /* means 'input' */
			bpf_input (ppp->nif, b);
			*b->dstart = ADDR;
		}
		ppp_recv_usr (ppp, b);
	}
}
#pragma GCC diagnostic pop

/*
 * /dev/ppp? device driver.
 *
 * write() and read() expect the following structure:
 *
 * buf[0] -- PPP address
 * buf[1] -- PPP control
 * buf[2] -- PPP protocol, high byte
 * buf[3] -- PPP protocol, low byte
 * buf[4] ... buf[nbytes - 1] -- "information" - field of the PPP frame
 *
 * fcntl (pppfd, FIONREAD, &size) returns in 'size' the size of the
 * next frame or zero of none is available for reading.
 */

static long
pppdev_open (FILEPTR *fp)
{
	struct ppp *ppp = &ppp_priv[fp->fc.aux];
	
	ppp->links++;
	return 0;
}

static long
pppdev_write (FILEPTR *fp, const char *_buf, long nbytes)
{
	struct ppp *ppp = &ppp_priv[fp->fc.aux];
	const unsigned char *buf = (const unsigned char *)_buf;
	short proto;
	BUF *b;
	long r;
	
	if (nbytes < 4)
	{
		DEBUG (("pppdev_write: must at least have 4 bytes"));
		return EINVAL;
	}
	
	buf += 2;
	proto = ((ushort)buf[0] << 8) | (ushort)buf[1];
	buf += 2;
	nbytes -= 4;
	b = buf_alloc (nbytes + PPP_RESERVE, PPP_RESERVE/2, BUF_NORMAL);
	if (b == 0)
		return ENOMEM;
	
	memcpy (b->dstart, buf, nbytes);
	b->dend += nbytes;
	b->info = IF_PRIORITIES-1;
	
	/*
	 * Can't take long 'til we can write, so just nap().
	 */
	while (ppp->nif->snd.qlen >= ppp->nif->snd.maxqlen)
	{
		if (fp->flags & O_NDELAY)
		{
			buf_deref (b, BUF_NORMAL);
			return 0;
		}
		nap (200);
	}
	r = ppp_send_frame (ppp, b, proto);
	return (r ? r : (nbytes + 4));
}

static long
pppdev_read (FILEPTR *fp, char *buf, long nbytes)
{
	struct ppp *ppp = &ppp_priv[fp->fc.aux];
	long cando;
	BUF *b;
	
	if (nbytes <= 0)
		return 0;
	
	for (;;)
	{
		b = if_dequeue (&ppp->rcvq);
		if (b != 0)
			break;
		
		if (fp->flags & O_NDELAY)
			return 0;
		
		if (isleep (IO_Q, (long) ppp))
			return EINTR;
	}
	cando = MIN (nbytes, b->dend - b->dstart);
	memcpy (buf, b->dstart, cando);
	buf_deref (b, BUF_NORMAL);
	
	return cando;
}

static long
pppdev_lseek (FILEPTR *fp, long where, int whence)
{
	return EACCES;
}

static long
pppdev_ioctl (FILEPTR *fp, int cmd, void *arg)
{
	struct ppp *ppp = &ppp_priv[fp->fc.aux];
	long *l;
	BUF *b;
	
	switch (cmd)
	{
		case FIONREAD:
			b = ppp->rcvq.qfirst[IF_PRIORITIES-1];
			*(long *) arg = b ? b->dend - b->dstart : 0;
			return 0;
		
		case FIONWRITE:
			*(long *) arg = ppp->nif->snd.qlen < ppp->nif->snd.maxqlen
				? 1 : 0;
			return 0;
		
		case FIOEXCEPT:
			*(long *) arg = 0;
			return 0;
		
		case PPPIOCSFLAGS:
			if (p_geteuid ())
				return EACCES;
			ppp->opts = *(long *)arg;
			return 0;
		
		case PPPIOCGFLAGS:
			*(long *) arg = ppp->opts;
			return 0;
		
		case PPPIOCSXMAP:
			if (p_geteuid ())
				return EACCES;
			l = (long *)arg;
			if (l[0] & l[1] || l[2] & l[3] || l[4] & l[5] || l[6] & l[7] ||
			    MAP_ISSET (l, FLAG^TRANS) || MAP_ISSET (l, ESC^TRANS))
			{
				DEBUG (("pppdev_ioctl: invalid xmap"));
				return EINVAL;
			}
			memcpy (ppp->xmap, l, sizeof (ppp->xmap));
			ppp->xmap[1] = 0L;
			MAP_SET (ppp->xmap, ESC);
			MAP_SET (ppp->xmap, FLAG);
			return 0;
		
		case PPPIOCGXMAP:
			l = (long *) arg;
			memcpy (l, ppp->xmap, sizeof (ppp->xmap));
			return 0;
		
		case PPPIOCSRMAP:
			if (p_geteuid ())
				return EACCES;
			ppp->rmap[0] = *(long *) arg;
			return 0;
		
		case PPPIOCGRMAP:
			*(long *) arg = ppp->rmap[0];
			return 0;
		
		case PPPIOCGMTU:
			*(long *) arg = ppp->nif->mtu;
			return 0;
		
		case PPPIOCSMTU:
			if (p_geteuid ())
				return EACCES;
			ppp->nif->mtu = *(long *) arg;
			return 0;
		
		case PPPIOCGMRU:
			*(long *)arg = ppp->mru;
			return 0;
		
		case PPPIOCSMRU:
			if (p_geteuid ())
				return EACCES;
			ppp->mru = *(long *) arg;
			return 0;
		
		case PPPIOCSPGRP:
			ppp->pgrp = (short)*(long *) arg;
			return 0;
		
		case PPPIOCGPGRP:
			*(long *) arg = (long) ppp->pgrp;
			return 0;
		
		case PPPIOCSSLOTS:
			if (p_geteuid ())
				return EACCES;
			return 0;
	}
	
	return ENOSYS;
}

static long
pppdev_datime (FILEPTR *fp, ushort *timeptr, int mode)
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
pppdev_close (FILEPTR *fp, int pid)
{
	struct ppp *ppp = &ppp_priv[fp->fc.aux];
	
	if (fp->links <= 0 && --ppp->links <= 0)
	{
		ppp->pgrp = 0;
		if_flushq (&ppp->rcvq);
	}
	
	return 0;
}

static long
pppdev_select (FILEPTR *fp, long proc, int mode)
{
	struct ppp *ppp = &ppp_priv[fp->fc.aux];
	
	switch (mode)
	{
		case O_RDONLY:
			if (ppp->rcvq.qfirst[IF_PRIORITIES-1])
				return 1;
			if (ppp->rsel == 0)
			{
				ppp->rsel = proc;
				return 0;
			}
			return 2;
		
		case O_WRONLY:
			if (ppp->nif->snd.qlen < ppp->nif->snd.maxqlen)
				return 1;
			if (ppp->wsel == 0)
			{
				ppp->wsel = proc;
				return 0;
			}
			return 2;
		
		case O_RDWR:
			if (ppp->xsel == 0)
			{
				ppp->xsel = proc;
				return 0;
			}
			return 2;
	}
	
	return 0;
}

static void
pppdev_unselect	(FILEPTR *fp, long proc, int mode)
{
	struct ppp *ppp = &ppp_priv[fp->fc.aux];
	
	switch (mode)
	{
		case O_RDONLY:
			if (proc == ppp->rsel)
				ppp->rsel = 0;
			break;
		
		case O_WRONLY:
			if (proc == ppp->wsel)
				ppp->wsel = 0;
			break;
		
		case O_RDWR:
			if (proc == ppp->xsel)
				ppp->xsel = 0;
			break;
	}
}
