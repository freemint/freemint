/*
 *	Implementation of the SLIP protocol for transmission of IP
 *	packets over serial lines.
 *	Read the file README.SLIP for important notes.
 *
 *	03/04/94, kay roemer.
 */

#include <string.h>
#include "config.h"
#include "netinfo.h"
#include "kerbind.h"
#include "atarierr.h"
#include "sockerr.h"
#include "sockios.h"
#include "if.h"
#include "buf.h"
#include "util.h"
#include "osl0.h"

/* this structure is attached to the net interface structure and holds
 * SLIP specific information.
 */
struct slip_private {
	struct netif	*nif;	/* backlink to interface */
	
	BUF	*snd_buf;	/* buffer currently beeing sent */
	char	*snd_head;	/* buffer head */
	char	*snd_tail;	/* buffer tail */
	short	snd_busy;	/* transmitter is busy */
	short	snd_stop;
	void	(*snd_state) (struct slip_private *);
				/* current state of the transmitter */

	BUF	*rcv_buf;	/* buffer currently beeing received */
	char	*rcv_head;	/* buffer head */
	char	*rcv_tail;	/* buffer tail */
	void	(*rcv_state) (struct slip_private *);
				/* current state of the receiver */

	unsigned char	(*recv_byte) (void);
				/* send a byte to the serial line */
	void		(*send_byte) (unsigned char);
				/* receive a byte from the serial line */

	char	(*can_recv) (void);	/* can read byte from serial line ? */
	char	(*can_send) (void);	/* can write byte to serial line ? */
	
	void	(*clr_error) (void);	/* clear accumulated errors */
	short	(*get_error) (void);	/* get accumulated errors */
#define UNDERRUN	0x0001
#define OVERRUN		0x0002
#define FRAME		0x0004
#define PARITY		0x0008
#define BREAK		0x0010

	void	(*set_dtr) (short);	/* set/clear DTR wire state */
	void	(*set_rts) (short);	/* set/clear RTS wire state */
	void	(*set_brk) (short);	/* set/clear BREAK condition */

	char	(*get_dcd) (void);	/* get DCD wire state */
	char	(*get_cts) (void);	/* get CTS wire state */

	short	(*init) (void);		/* initialize the physical line */
	void	(*open) (void);		/* open line for use */
	void	(*close) (void);	/* close line */
};

static long	slip_open	(struct netif *);
static long	slip_close	(struct netif *);
static long	slip_output	(struct netif *, BUF *, char *, short, short);
static long	slip_ioctl	(struct netif *, short, long);

extern void	slip_kick	(short);
extern void	slip_start	(short);
extern void	slip_stop	(short);
extern void	slip_txint	(short);
extern void	slip_rxint	(short);
static short	nif2chan	(struct netif *);

/* transmitter finite state machine */
static void	sstate_newpacket	(struct slip_private *);
static void	sstate_putbyte		(struct slip_private *);
static void	sstate_putesc		(struct slip_private *);

/* receiver finite state machine */
static void	rstate_newpacket	(struct slip_private *);
static void	rstate_getbyte		(struct slip_private *);
static void	rstate_getesc		(struct slip_private *);
static void	rstate_sync		(struct slip_private *);

#define SLIP_MTU	2000		/* maximum transmission unit */
#define SLIP_CHANNELS	1		/* number of SLIP units */

static struct netif slip_chan[SLIP_CHANNELS];
static struct slip_private slip_priv[SLIP_CHANNELS] = {
	{	&slip_chan[0],
		0, 0, 0, 0, 0,
		sstate_newpacket,
		0, 0, 0,
		rstate_newpacket,
		sl0_recv_byte, sl0_send_byte,
		sl0_can_recv, sl0_can_send,
		sl0_clear_errors, sl0_check_errors,
		sl0_set_dtr, sl0_set_rts, sl0_set_brk,
		sl0_get_dcd, sl0_get_cts,
		sl0_init, sl0_open, sl0_close
	},
};

#define ESC		219
#define END		192
#define ESC_ESC		221
#define ESC_END		220

static inline short
nif2chan (nif)
	struct netif *nif;
{
	return nif->unit;
}

static long
slip_open (nif)
	struct netif *nif;
{
	struct slip_private *slp = (struct slip_private *)nif->data;
	short sr;

/* Open the low level part */
	(*slp->open) ();

/* Can only send if Carrier and CTS high */
	sr = spl7 ();
	slp->snd_stop = 0;
	if (!(*slp->get_dcd) ()) ++slp->snd_stop;
	if (!(*slp->get_cts) ()) ++slp->snd_stop;
	spl (sr);

	(*slp->set_dtr) (1);	/* assert DTR */
	(*slp->set_brk) (0);	/* no break condition, please */
	return 0;
}

static long
slip_close (nif)
	struct netif *nif;
{
	struct slip_private *slp = (struct slip_private *)nif->data;

/* Close the low level part */	
	(*slp->close) ();

/* Hang up the line for aprox 1.2 seconds */
	(*slp->set_dtr) (0);
	f_select ((short)1200, 0L, 0L, 0L);
	(*slp->set_dtr) (1);

	(*slp->set_brk) (0);
	return 0;
}

static long
slip_output (nif, buf, hwaddr, hwlen, pktype)
	struct netif *nif;
	BUF *buf;
	char *hwaddr;
	short hwlen, pktype;
{
	struct slip_private *slp = (struct slip_private *)nif->data;
	long r;
	short err;

	err = (*slp->get_error) ();
	if (err) {
		if (err & OVERRUN)
			DEBUG (("SLIP: overrun on channel %d", nif->unit));
		if (err & BREAK)
			TRACE (("SLIP: break on channel %d", nif->unit));
		if (err & PARITY)
			DEBUG (("SLIP: parity error on channel %d", nif->unit));
		if (err & FRAME)
			DEBUG (("SLIP: frame error on channel %d", nif->unit));
	}
	if ((r = if_enqueue (&nif->snd, buf, buf->info))) {
		++nif->out_errors;
		return r;
	}
	slip_kick (nif2chan (nif));
	return 0;
}

static long
slip_ioctl (nif, cmd, arg)
	struct netif *nif;
	short cmd;
	long arg;
{
	switch (cmd) {
	case SIOCSIFFLAGS:
	case SIOCSIFADDR:
	case SIOCSIFNETMASK:
		return 0;
	}
	return EINVFN;
}

/* Try to send data on channel `channel' */
void
slip_kick (channel)
	short channel;
{
	struct slip_private *slp;
	short sr;

	slp = (struct slip_private *)slip_chan[channel].data;
	sr = spl7 ();
	if (!slp->snd_busy && !slp->snd_stop && (*slp->can_send) ()) {
		slp->snd_busy = 1;
		slip_txint (channel);
	}
	spl (sr);
}

/* Stop to send data on channel `channel' (Carrier lost or CTS low).
 * Call at spl7 only. */
void
slip_stop (channel)
	short channel;
{
	struct slip_private *slp;

	slp = (struct slip_private *)slip_chan[channel].data;
	++slp->snd_stop;
	slp->snd_busy = 0;
}

/* Restart transmission on channel `channel' (only if CTS AND DCD high).
 * Call at spl7 only. */
void
slip_start (channel)
	short channel;
{
	struct slip_private *slp;

	slp = (struct slip_private *)slip_chan[channel].data;
	--slp->snd_stop;
	slip_kick (channel);
}

long
driver_init (void)
{
	short i;

	c_conws ("SLIP v0.5 (old), one channel\n\r");
	for (i = 0; i < SLIP_CHANNELS; ++i) {
		strcpy (slip_chan[i].name, "osl");
		slip_chan[i].unit = i;
		slip_chan[i].metric = 0;
		slip_chan[i].flags = IFF_POINTOPOINT;
		slip_chan[i].mtu = SLIP_MTU;
		slip_chan[i].timer = 0;
		slip_chan[i].hwtype = HWTYPE_NONE;

		slip_chan[i].rcv.maxqlen = IF_MAXQ;
		slip_chan[i].snd.maxqlen = IF_MAXQ;
		slip_chan[i].open = slip_open;
		slip_chan[i].close = slip_close;
		slip_chan[i].output = slip_output;
		slip_chan[i].ioctl = slip_ioctl;
		slip_chan[i].timeout = 0;

		slip_chan[i].data = &slip_priv[i];

		if ((*slip_priv[i].init) () == 0) {
			if_register (&slip_chan[i]);
		}
	}
	return 0;
}

/*
 * SLIP transmitter finite state machine
 */

static void
sstate_newpacket (slp)
	struct slip_private *slp;
{
	BUF *buf;
	
	slp->snd_buf = buf = if_dequeue (&slp->nif->snd);
	if (buf) {
		(*slp->send_byte) (END);
		slp->snd_head = buf->dstart;
		slp->snd_tail = buf->dend;
		slp->snd_busy = 1;
		slp->snd_state = sstate_putbyte;
	} else	slp->snd_busy = 0;
}

static void
sstate_putbyte (slp)
	struct slip_private *slp;
{
	unsigned char c;
	
	if (slp->snd_head < slp->snd_tail) {
		c = *slp->snd_head;
		switch (c) {
		case ESC:
		case END:
			(*slp->send_byte) (ESC);
			slp->snd_state = sstate_putesc;
			return;

		default:
			(*slp->send_byte) (c);
			++slp->snd_head;
			return;
		}
	}
	(*slp->send_byte) (END);
	buf_deref (slp->snd_buf, BUF_ATOMIC);
	slp->snd_state = sstate_newpacket;
	++slp->nif->out_packets;
}

static void
sstate_putesc (slp)
	struct slip_private *slp;
{
	if ((unsigned char)*slp->snd_head++ == ESC) {
		(*slp->send_byte) (ESC_ESC);
		slp->snd_state = sstate_putbyte;
	} else {
		(*slp->send_byte) (ESC_END);
		slp->snd_state = sstate_putbyte;
	}
}

/* transmitter interrupt */
void
slip_txint (channel)
	short channel;
{
	struct slip_private *slp;

	slp = (struct slip_private *)slip_chan[channel].data;
	if (slp->snd_stop > 0) return;
	(*slp->snd_state) (slp);
}

/*
 * SLIP receiver finite state machine
 */

static void
rstate_newpacket (slp)
	struct slip_private *slp;
{
	BUF *buf;
	unsigned char c;

	c = (*slp->recv_byte) ();
	if (c == END) return;

	slp->rcv_buf = buf = buf_alloc (slp->nif->mtu, 0, BUF_ATOMIC);
	if (buf) {
		slp->rcv_head = buf->dstart;
		slp->rcv_tail = buf->dstart + slp->nif->mtu;

		if (c != ESC) {
			*slp->rcv_head++ = c;
			slp->rcv_state = rstate_getbyte;
		} else	slp->rcv_state = rstate_getesc;
	} else {
		++slp->nif->in_errors;
		slp->rcv_state = rstate_sync;
	}
}

static void
rstate_getbyte (slp)
	struct slip_private *slp;
{
	unsigned char c;

	c = (*slp->recv_byte) ();
	switch (c) {
	case ESC:
		slp->rcv_state = rstate_getesc;
		return;
		
	case END:
		slp->rcv_buf->dend = slp->rcv_head;
		if_input (slp->nif, slp->rcv_buf, 0, PKTYPE_IP);
		slp->rcv_state = rstate_newpacket;
		++slp->nif->in_packets;
		return;

	default:
		if (slp->rcv_head < slp->rcv_tail) *slp->rcv_head++ = c;
		else {
			++slp->nif->in_errors;
			buf_deref (slp->rcv_buf, BUF_ATOMIC);
			slp->rcv_state = rstate_sync;
		}
		return;
	}
}

static void
rstate_getesc (slp)
	struct slip_private *slp;
{
	unsigned char c;

	c = (*slp->recv_byte) ();
	if (slp->rcv_head < slp->rcv_tail) switch (c) {
	case ESC_END:
		*slp->rcv_head++ = END;
		slp->rcv_state = rstate_getbyte;
		return;
		
	case ESC_ESC:
		*slp->rcv_head++ = ESC;
		slp->rcv_state = rstate_getbyte;
		return;
	}

	++slp->nif->in_errors;
	buf_deref (slp->rcv_buf, BUF_ATOMIC);
	slp->rcv_state = rstate_sync;
}

static void
rstate_sync (slp)
	struct slip_private *slp;
{
	if ((*slp->recv_byte) () == END) {
		slp->rcv_state = rstate_newpacket;
	}
}

/* receiver interrupt */
void
slip_rxint (channel)
	short channel;
{
	struct slip_private *slp;

	slp = (struct slip_private *)slip_chan[channel].data;
	(*slp->rcv_state) (slp);
}
