/*
 *	This file implements a nonstandard way to transmit IP datagrams
 *	over bidirectional parallel (centronics) lines. I call this PLIP
 *	(Parallel line IP), but it may be incompatible with other such
 *	protocolls called PLIP too.
 *	Read the file README.PLIP for important notes.
 *
 *	03/15/94, Kay Roemer.
 */

# include "global.h"
# include "netinfo.h"

# include "buf.h"
# include "inet4/if.h"
# include "inet4/in.h"

# include "mint/asm.h"
# include "mint/sockio.h"

# include "plip.h"
# include "pl0.h"
# include "slip.h"


struct netif		plip_chan[PLIP_CHANNELS];
struct plip_private	plip_priv[PLIP_CHANNELS] =
{
	{	0, PLIP_TIMEOUT, 0,
		&plip_chan[0],
		pl0_set_strobe,	pl0_get_busy, pl0_set_direction,
		pl0_send_pkt, pl0_recv_pkt,
		pl0_got_ack, pl0_send_ack,
		pl0_cli, pl0_sti,
		pl0_init, pl0_open, pl0_close
	}
};

static long	plip_open	(struct netif *);
static long	plip_close	(struct netif *);
static long	plip_output	(struct netif *, BUF *, const char *, short, short);
static long	plip_ioctl	(struct netif *, short, long);

static void	plip_send_pkt	(struct plip_private *);
static void	plip_recv_pkt	(struct plip_private *);
static void	plip_reset	(struct plip_private *);
static void	plip_kick	(struct plip_private *);
static void	plip_dosend	(PROC *proc, long arg);
static void	inc_timeout	(struct plip_private *);
static short	plip_collision	(struct plip_private *);
extern void	plip_int	(short);

static long
plip_open (struct netif *nif)
{
	struct plip_private *plp = (struct plip_private *) nif->data;
	
	(*plp->open)();
	return 0;
}

static long
plip_close (struct netif *nif)
{
	struct plip_private *plp = (struct plip_private *) nif->data;
	
	(*plp->close)();
	return 0;
}

static long
plip_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	long r;
	
	r = if_enqueue (&nif->snd, buf, buf->info);
	if (r)
	{
		nif->out_errors++;
		return r;
	}
	
	plip_kick (nif->data);
	return 0;
}

static long
plip_ioctl (struct netif *nif, short cmd, long arg)
{
	switch (cmd)
	{
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
		case SIOCSIFNETMASK:
			return 0;
	}
	
	return ENOSYS;
}

long driver_init(void);
long
driver_init (void)
{
	short i;
	
	c_conws (PLIP_VERSION);
	for (i = 0; i < PLIP_CHANNELS; i++)
	{
		strcpy (plip_chan[i].name, "pl");
		plip_chan[i].unit = i;
		plip_chan[i].metric = 0;
		plip_chan[i].flags = IFF_POINTOPOINT;
		plip_chan[i].mtu = PLIP_MTU;
		plip_chan[i].timer = 0;
		plip_chan[i].hwtype = HWTYPE_PLIP;
		
		plip_chan[i].rcv.maxqlen = IF_MAXQ;
		plip_chan[i].snd.maxqlen = IF_MAXQ;
		plip_chan[i].open = plip_open;
		plip_chan[i].close = plip_close;
		plip_chan[i].output = plip_output;
		plip_chan[i].ioctl = plip_ioctl;
		plip_chan[i].timeout = 0;
		
		plip_chan[i].data = &plip_priv[i];
		
		if ((*plip_priv[i].init) () == 0)
			if_register (&plip_chan[i]);
	}
	
	return 0;
}

static void
plip_send_pkt (struct plip_private *plp)
{
	BUF *buf = 0;
	ushort sr;
	long nbytes;
	short i;
	
	if (plp->nif->snd.qlen == 0)
		return;
	
	sr = spl7 ();
	if (plp->busy)
	{
		/* plip channel already in use */
		spl (sr);
		return;
	}
	plp->busy = 1;
	spl (sr);
	
	(*plp->cli)();
	(*plp->set_strobe)(1);
	(*plp->set_direction)(OUT);

restart:
	for (i = PLIP_RETRIES; i > 0; --i)
	{
		(*plp->send_ack)();
		if (RETRY (plp->timeout, (*plp->got_ack)() != 0))
			break;
		inc_timeout (plp);
	}
	
	if (i == 0)
	{
		plip_reset (plp);
		return;
	}
	HARD_WAIT ();
	if ((*plp->get_busy)() != 0 || (*plp->got_ack)())
	{
		plp->nif->collisions++;
		if (plip_collision (plp))
		{
			plip_reset (plp);
			return;
		}
		else
			goto restart;
	}
	(*plp->send_ack)();
	
	buf = if_dequeue (&plp->nif->snd);
	if (!buf)
	{
		plip_reset (plp);
		return;
	}
	nbytes = buf->dend - buf->dstart;
	if ((*plp->send_pkt) (plp->timeout, buf->dstart, nbytes) == 0)
		plp->nif->out_packets++;
	else
	{
		inc_timeout (plp);
		plp->nif->out_errors++;
	}
	if (plp->nif->bpf && BUF_LEAD_SPACE (buf) >= SLIP_HDRLEN)
	{
		buf->dstart -= SLIP_HDRLEN;
		buf->dstart[SLX_DIR] = SLIPDIR_OUT;
		bpf_input (plp->nif, buf);
		buf->dstart += SLIP_HDRLEN;		
	}
	buf_deref (buf, BUF_NORMAL);
	plip_reset (plp);
}

static void
plip_recv_pkt (struct plip_private *plp)
{
	BUF *buf = 0;
	long nbytes, ret;
	ushort sr;
	
	sr = spl7 ();
	if (plp->busy)
	{
		/* plip channel already in use */
		spl (sr);
		return;
	}
	plp->busy = 1;
	spl (sr);
	
	(*plp->cli)();
	(*plp->set_strobe)(1);
	(*plp->set_direction)(IN);
	
	nbytes = plp->nif->mtu + 1;
	buf = buf_alloc (nbytes, 0, BUF_ATOMIC);
	if (!buf)
	{
		plp->nif->in_errors++;
		plip_reset (plp);
		return;
	}
	
	(*plp->set_strobe)(0);
	if (!RETRY (PLIP_MAXTMOUT, (*plp->got_ack) () != 0))
	{
		plip_reset (plp);
		buf_deref (buf, BUF_NORMAL);
		return;
	}
	(*plp->set_strobe)(1);
	(*plp->send_ack)();
	
	ret = (*plp->recv_pkt)(plp->timeout, buf->dstart, nbytes);
	if (ret > 0)
	{
		buf->dend = buf->dstart + ret;
		if (plp->nif->bpf)
		{
			buf->dstart -= SLIP_HDRLEN;
			buf->dstart[SLX_DIR] = SLIPDIR_IN;
			bpf_input (plp->nif, buf);
			buf->dstart += SLIP_HDRLEN;
		}
		if (if_input (plp->nif, buf, 0, PKTYPE_IP))
			plp->nif->in_errors++;
		else
			plp->nif->in_packets++;
	}
	else
	{
		if (ret == -1)
			inc_timeout (plp);
		plp->nif->in_errors++;
		buf_deref (buf, BUF_ATOMIC);
	}
	plip_reset (plp);
}

/* Reset the parallel line */
static void
plip_reset (struct plip_private *plp)
{
	(*plp->set_strobe)(1);
	(*plp->set_direction)(IN);
	(*plp->sti)();
	plp->busy = 0;
}

static void
plip_dosend (PROC *proc, long arg)
{
	struct plip_private *plp;
	short i;
	
	for (i = 0, plp = plip_priv; i < PLIP_CHANNELS; i++, plp++)
	{
		if (plp->timeouts_pending > 0)
		{
			plp->timeouts_pending--;
			plip_kick (plp);
			return;
		}
	}
}

/* Try to send a packet. If any unsent packets set a timeout to check
 * again later. */
static void
plip_kick (struct plip_private *plp)
{
	plip_send_pkt (plp);
	if (plp->nif->snd.qlen > 0 && plp->timeouts_pending == 0)
	{
		TIMEOUT *t = addroottimeout (100, plip_dosend, 0);
		if (t) plp->timeouts_pending++;
	}
}

/* BUSY interrupt routine */
void
plip_int (short channel)
{
	plip_recv_pkt (&plip_priv[channel]);
}

static void
inc_timeout (struct plip_private *plp)
{
	if ((plp->timeout + 1) < PLIP_MAXTMOUT)
		plp->timeout++;
}

static short
plip_collision (struct plip_private *plp)
{
# define SADDR(x)	(x.in.sin_addr.s_addr)
	struct ifaddr *ina;
	
	for (ina = plp->nif->addrlist; ina; ina = ina->next)
	{
		if (ina->family == AF_INET)
			return (SADDR (ina->adr) < SADDR (ina->ifu.dstadr));
	}
	
	return 1;
}
