/*
 *	New hardware independet SLIP interface implementation using
 *	the pseudo line discipline stuff in serial.c. This way SLIP
 *	can use the Mint device drivers. A device driver is linked
 *	to a SLIP interface using the SIOCSIFLINK ioctl, the device
 *	driver linked to an interface can be queried with SIOCGIFNAME.
 *	Iflink(8) implements this behavior.
 *
 *	06/12/94, Kay Roemer.
 */

# include "global.h"
# include "netinfo.h"

# include "buf.h"
# include "inet4/if.h"
# include "inet4/in.h"
# include "util.h"

# include <mint/config.h>
# include <mint/dcntl.h>

# include "slip.h"
# include "serial.h"
# include "slcompress.h"


# define SLIP_VERSION	"0.5"
# define SLIP_MTU	2000		/* def. maximum transmission unit */
# define SLIP_CHANNELS	4		/* number of SLIP units */

# define ESC		219		/* escape char */
# define END		192		/* end of frame char */
# define ESC_ESC	221		/* escaped esc */
# define ESC_END	220		/* escaped end */

struct slstats
{
	ulong	i_packets;
	ulong	i_toshort;
	ulong	i_tolong;
	ulong	i_badcode;
	ulong	i_badvj;
	ulong	i_nomem;
	ulong	i_qfull;
	ulong	o_packets;
	ulong	o_qfull;
};

struct slip
{
	struct netif	*nif;		/* backlink to interface */
	short		flags;		/* misc flags, see slip.h */
	BUF		*snd_buf;	/* BUF currently beeing sent */
	char		*snd_head;	/* head pointer */
	char		*snd_tail;	/* tail pointer */
	
	BUF		*rcv_buf;	/* BUF currently beeing received */
	char		*rcv_head;	/* head pointer */
	char		*rcv_tail;	/* tail pointer */
	
	struct slbuf	*slbuf;		/* serial buffer */
	struct slcompress *comp;	/* states for VJ compression */
	struct slstats	stat;
};

static struct netif	slip_chan[SLIP_CHANNELS];
static struct slip	slip_priv[SLIP_CHANNELS];

static long	slip_link	(struct iflink *);
static long	slip_open	(struct netif *);
static long	slip_close	(struct netif *);
static long	slip_output	(struct netif *, BUF *, const char *, short, short);
static long	slip_ioctl	(struct netif *, short, long);
static short	slip_send	(struct slbuf *);
static short	slip_recv	(struct slbuf *);

long
driver_init (void)
{
	extern long ppp_init (void);
	extern long slip_init (void);
	
	serial_init ();
	return (slip_init () || ppp_init ());
}

static long
slip_link (struct iflink *ifl)
{
	int i;
	
	for (i = 0; i < SLIP_CHANNELS; i++)
	{
		if (!(slip_priv[i].flags & SLF_LINKED))
			break;
	}
	
	if (i > SLIP_CHANNELS)
	{
		DEBUG (("slip_link: no free slip channels"));
		return ENOMEM;
	}
	
	if (slip_priv[i].snd_buf)
		buf_deref (slip_priv[i].snd_buf, BUF_NORMAL);
	
	if (slip_priv[i].rcv_buf)
		buf_deref (slip_priv[i].rcv_buf, BUF_NORMAL);
	
	slip_priv[i].snd_buf  = 0;
	slip_priv[i].snd_head = 0;
	slip_priv[i].snd_tail = 0;
	slip_priv[i].rcv_buf  = 0;
	slip_priv[i].rcv_head = 0;
	slip_priv[i].rcv_tail = 0;
	slip_priv[i].comp = slc_init ();
	
	if (slip_priv[i].comp == 0)
	{
		DEBUG (("slip_link: no space for VJ compression states"));
		return ENOMEM;
	}
	
	slip_priv[i].slbuf = serial_open (&slip_chan[i], ifl->device,
		slip_send, slip_recv);
	
	if (slip_priv[i].slbuf == 0)
	{
		DEBUG (("slip_link: no free serial channels"));
		slc_free (slip_priv[i].comp);
		slip_priv[i].comp = 0;
		return ENOMEM;
	}
	
	slip_priv[i].flags |= SLF_LINKED;
	ksprintf (ifl->ifname, "%s%d", slip_chan[i].name, slip_chan[i].unit);
	return 0;
}

static long
slip_open (struct netif *nif)
{
	struct slip *slp = nif->data;
	
	if (!(slp->flags & SLF_LINKED))
	{
		DEBUG (("slip_open: chan %d not linked to device", nif->unit));
		return ENODEV;
	}
	
	return 0;
}

static long
slip_close (struct netif *nif)
{
	struct slip *slp = nif->data;
	long r = 0;
	
	if (slp->flags & SLF_LINKED)
	{
		r = serial_close (slp->slbuf);
		if (r < 0)
		{
			DEBUG (("slip_close: cannot close serial channel"));
			return r;
		}
		slc_free (slp->comp);
		slp->comp = 0;
		slp->flags &= ~SLF_LINKED;
	}
	return 0;
}

static long
slip_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	struct slip *slp = nif->data;
	long r;
	
	r = if_enqueue (&nif->snd, buf, buf->info);
	if (r)
	{
		slp->stat.o_qfull++;
		nif->out_errors++;
		
		DEBUG (("slip_output: chan %d: cannot enqueue", nif->unit));
		return r;
	}
	
	return serial_send (slp->slbuf);
}

static long
slip_ioctl (struct netif *nif, short cmd, long arg)
{
	struct slip *slp = nif->data;
	struct iflink *ifl;
	struct ifreq *ifr;
	long *l;
	
	switch (cmd)
	{
		case SIOCSIFLINK:
			return slip_link ((struct iflink *)arg);
		
		case SIOCGIFNAME:
			ifl = (struct iflink *) arg;
			if (!(slp->flags & SLF_LINKED))
			{
				DEBUG (("slip_ioctl: IFLINK: chan %d not linked",
					nif->mtu));
				return EINVAL;
			}
			strncpy (ifl->device, slp->slbuf->dev, sizeof (ifl->device));
			return 0;
		
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
		case SIOCSIFNETMASK:
			return 0;
		
		case SIOCGLNKFLAGS:
			ifr = (struct ifreq *) arg;
			ifr->ifru.flags = slp->flags & SLF_USRMASK;
			return 0;
		
		case SIOCSLNKFLAGS:
			ifr = (struct ifreq *) arg;
			slp->flags &= ~SLF_USRMASK;
			slp->flags |= ifr->ifru.flags & SLF_USRMASK;
			return 0;
		
		case SIOCGLNKSTATS:
			ifr = (struct ifreq *) arg;
			l = ifr->ifru.data;
			bzero (l, 40 * sizeof (long));
			memcpy (l, &slp->stat, sizeof (slp->stat));
			l += sizeof (slp->stat) / sizeof (*l);
			if (slp->comp)
				slc_getstats (slp->comp, l);
			return 0;
	}
	
	return ENOSYS;
}

long
slip_init (void)
{
	char buf[100];
	short i;
	
	ksprintf (buf, "SLIP v%s, %d channels\n\r", SLIP_VERSION, SLIP_CHANNELS);
	c_conws (buf);
	for (i = 0; i < SLIP_CHANNELS; ++i)
	{
		strcpy (slip_chan[i].name, "sl");
		slip_chan[i].unit = i;
		slip_chan[i].metric = 0;
		slip_chan[i].flags = IFF_POINTOPOINT;
		slip_chan[i].mtu = SLIP_MTU;
		slip_chan[i].timer = 0;
		slip_chan[i].hwtype = HWTYPE_SLIP;

		slip_chan[i].rcv.maxqlen = IF_MAXQ;
		slip_chan[i].snd.maxqlen = IF_MAXQ;
		slip_chan[i].open = slip_open;
		slip_chan[i].close = slip_close;
		slip_chan[i].output = slip_output;
		slip_chan[i].ioctl = slip_ioctl;
		slip_chan[i].timeout = 0;

		slip_chan[i].data = &slip_priv[i];
		slip_priv[i].nif = &slip_chan[i];

		if_register (&slip_chan[i]);
	}
	
	return 0;
}

INLINE short
uncompress (BUF *b, struct slip *slp)
{
	uchar type;
	
	type = slc_type (b);
	switch (type)
	{
		case TYPE_IP:
			return 0;
		
		case TYPE_COMPRESSED_TCP:
			if (slp->flags & SLF_COMPRESS)
				return !slc_uncompress (b, type, slp->comp);
			break;
		
		case TYPE_UNCOMPRESSED_TCP:
			if (slp->flags & SLF_COMPRESS)
				return !slc_uncompress (b, type, slp->comp);
			
			if (slp->flags & SLF_AUTOCOMP &&
				slc_uncompress (b, type, slp->comp))
			{
				slp->flags |= SLF_COMPRESS;
				return 0;
			}
			break;
	}
	
	return 1;
}

static short
slip_recv (struct slbuf *slb)
{
# define SL_GETC() ({ char _c = buf[tail++]; tail &= mask; _c; })
	
	struct slip *slp = slb->nif->data;
	short tail, mask = slb->isize - 1;
	long space, nbytes, l;
	unsigned char c;
	char *cp, *buf;
	BUF *b;
	
	nbytes = SL_IUSED (slb);
	while (nbytes > 0)
	{
		if (slp->rcv_buf == 0)
		{
			b = buf_alloc (slb->nif->mtu+MAX_HDR+1, MAX_HDR, BUF_ATOMIC);
			if (!b)
			{
				slp->stat.i_nomem++;
				return 0;
			}
			
			slp->rcv_buf = b;
			slp->rcv_head = b->dstart + slb->nif->mtu + 1;
			slp->rcv_tail = b->dstart;
		}
		
		cp = slp->rcv_tail;
		space = slp->rcv_head - cp;
		buf = slb->ibuf;
		tail = slb->itail;
		while (space > 0 && nbytes > 0)
		{
			if (nbytes > space)
				nbytes = space;
			
			while (--nbytes >= 0) switch ((c = SL_GETC ())) {
			case ESC:
				slp->flags |= SLF_ESC;
				break;

			case END:
				b = slp->rcv_buf;
				l = cp - b->dstart;
				if (slp->flags & SLF_DROP || l < 3)
				{
					if (l > 0 && l < 3)
						slp->stat.i_toshort++;
					slp->flags &= ~(SLF_ESC|SLF_DROP);
					cp = b->dstart;
					break;
				}
				b->dend = cp;
				if (uncompress (b, slp))
				{
					slp->stat.i_badvj++;
					slb->nif->in_errors++;
					slp->flags &= ~(SLF_ESC|SLF_DROP);
					cp = b->dstart;
					break;
				}
				if (slp->nif->bpf)
				{
					b->dstart -= SLIP_HDRLEN;
					b->dstart[SLX_DIR] = SLIPDIR_IN;
					bpf_input (slp->nif, b);
					b->dstart += SLIP_HDRLEN;
				}
				if (if_input (slb->nif, b, 0, PKTYPE_IP))
				{
					slp->stat.i_qfull++;
					slb->nif->in_errors++;
				}
				else
				{
					slp->stat.i_packets++;
					slb->nif->in_packets++;
				}
				
				slp->flags &= ~(SLF_ESC|SLF_DROP);
				slp->rcv_buf = 0;
				slb->itail = tail;
				nbytes = SL_IUSED (slb);
				goto newpacket;
			
			case ESC_ESC:
				*cp++ = (slp->flags & SLF_ESC) ? ESC : c;
				slp->flags &= ~SLF_ESC;
				break;
			
			case ESC_END:
				*cp++ = (slp->flags & SLF_ESC) ? END : c;
				slp->flags &= ~SLF_ESC;
				break;
			
			default:
				*cp++ = c;
				slp->flags &= ~SLF_ESC;
				break;
			}
			slb->itail = tail;
			nbytes = SL_IUSED (slb);
			space = slp->rcv_head - cp;
		}
		if (space <= 0)
		{
			slp->stat.i_tolong++;
			slb->nif->in_errors++;
			slp->flags |= SLF_DROP;
			slp->rcv_tail = cp = slp->rcv_buf->dstart;
		}
		else
			slp->rcv_tail = cp;
newpacket:
	}
	
	return 0;
}

static short
slip_send (struct slbuf *slb)
{
# define SL_PUTC(_c) { buf[head++] = _c; head &= mask; }
	
	struct slip *slp = slb->nif->data;
	short head, mask = slb->osize - 1;
	long space, nbytes;
	unsigned char c;
	char *cp, *buf;
	BUF *b;
	
	space = SL_OFREE (slb) / 2;
	while (space > 0)
	{
		buf = slb->obuf;
		head = slb->ohead;
		if (slp->snd_buf == 0)
		{
			slp->snd_buf = b = if_dequeue (&slp->nif->snd);
			if (b == 0)
				return 0;
			
			if (slp->nif->bpf && BUF_LEAD_SPACE(b) >= SLIP_HDRLEN)
			{
				b->dstart -= SLIP_HDRLEN;
				b->dstart[SLX_DIR] = SLIPDIR_OUT;
				bpf_input (slp->nif, b);
				b->dstart += SLIP_HDRLEN;
			}
			
			if (slp->flags & SLF_COMPRESS &&
			    IP_PROTO (b) == IPPROTO_TCP)
				*b->dstart |= slc_compress (b, slp->comp,
					slp->flags & SLF_COMPCID);
			
			slp->snd_head = b->dend;
			slp->snd_tail = b->dstart;
			SL_PUTC (END);
			space--;
		}
		
		cp = slp->snd_tail;
		nbytes = slp->snd_head - cp;
		while (nbytes > 0 && space > 0)
		{
			if (space > nbytes)
				space = nbytes;
			
			while (--space >= 0) switch ((c = *cp++)) {
			case ESC:
				SL_PUTC (ESC);
				SL_PUTC (ESC_ESC);
				break;
			
			case END:
				SL_PUTC (ESC);
				SL_PUTC (ESC_END);
				break;
			
			default:
				SL_PUTC (c);
				break;
			}
			slb->ohead = head;
			space = SL_OFREE (slb) / 2;
			nbytes = slp->snd_head - cp;
		}
		if (space > 0)
		{
			slp->stat.o_packets++;
			slb->nif->out_packets++;
			buf_deref (slp->snd_buf, BUF_ATOMIC);
			slp->snd_buf = 0;
			SL_PUTC (END);
			space--;
		}
		slb->ohead = head;
		slp->snd_tail = cp;
	}
	
	return 0;
}
