/*
 *	/dev/audio for Atari Ste, MegaSte, TT, Falcon running Mint.
 *	(Yamaha PSG stuff).
 *
 *	Based on audiodev 0.1 by Charles Briscoe-Smith, which is in
 *	turn based on lpdev 0.6 by Thierry Bousch.
 *
 *	10/28/95, Kay Roemer.
 *
 *  9802, John Blakeley - brought in to line with the falcon approach.
 */

# include "global.h"
# include "mint/ioctl.h"

# include "psgsnd.h"
# include "device.h"
# include "mfp.h"

# include <mint/osbind.h>


static void		psg_reset (void);
static void		psg_timer (void);
static long		psg_copyin (const char *buf, long len);
static long		psg_copyout (char *buf, long len);
static long		psg_ioctl (short mode, void *arg);
static long		psg_wspace (void);
static long		psg_rspace (void);
static void		gi_init (void);

static unsigned char ta_ctrl = 0;
static TIMEOUT *tmout = 0;

unsigned char *playptr = 0;
long playlen = 0;

/*
 * convert a play rate (in Hz) into values for timer B control
 * and data register
 */
static long
psg_freq (long *speed, unsigned char *control, unsigned char *data)
{
	if (*speed > 50000L || *speed < 4000L)
		return EINVAL;

	*control = MFP_DIV4;
	*data    = MFP_CLOCK/(*speed * 4);
	*speed   = MFP_CLOCK/((unsigned long)*data * 4);
	ta_ctrl  = *control;
	return 0;
}

/*
 * install things
 */
long
psg_init (struct device *dev)
{
	unsigned char ctrl, data;
	long snd_cookie;

	if (!get_toscookie (COOKIE__SND, &snd_cookie) &&
	    !(snd_cookie & 1)) {
		/*
		 * _SND cookie set and bit 0 (PSG sound
		 * not set
		 */
		return 1;
	}
	if (MFP_IERA & MFP_IMRA & 0x20) {
		c_conws ("timer A in use!\r\n");
		return 1;
	}

	dev->maxchans = 1;
	dev->curchans = 1;
	dev->reset = psg_reset;
	dev->copyin = psg_copyin;
	dev->copyout = psg_copyout;
	dev->ioctl = psg_ioctl;
	dev->wspace = psg_wspace;
	dev->rspace = psg_rspace;
	dev->format_map = AFMT_U8|AFMT_S8|AFMT_ULAW;
	dev->status = 0;
	
	timer_func = psg_timer;

	dev->srate = 8000;
	psg_freq (&dev->srate, &ctrl, &data);
	Jdisint (13);
	Xbtimer (0, ctrl, data, psg_player);
	Jenabint (13);
	timera_disable ();

	gi_init ();
	return 0;
}

static long
psg_wspace (void)
{
	short sr = spl7 ();
	long space = 0;

	if (thedev.status <= 1) {
		space = BUFSIZE - dmab[bufidx].used;
		if (!thedev.status)
			space += BUFSIZE;
	}
	spl (sr);
	return space;
}

static long
psg_rspace (void)
{
	return 0L;
}

static void
psg_reset (void)
{
	short sr = spl7 ();

	thedev.status = 0;
	playlen = 0;
	timera_disable ();
	dmab[0].used = 0;
	dmab[1].used = 0;
	spl (sr);
	if (tmout) {
		cancelroottimeout (tmout);
		tmout = 0;
	}
	if (audio_rsel)
		wakeselect (audio_rsel);
}

static void
gi_init (void)
{
	short sr = spl7 ();
	PSGselect = PSG_MIX;
	PSGwrite = PSGread | PSG_ALL_OFF;

	PSGselect = PSG_AMPA;
	PSGwrite = 0;

	PSGselect = PSG_AMPB;
	PSGwrite = 0;

	PSGselect = PSG_AMPC;
	PSGwrite = 0;
	spl (sr);
}

/*
 * timer A (frame done) interrupt
 */
static void
psg_timer (void)
{
	struct dmabuf *db;

	if (thedev.status) {
		--thedev.status;
		db = &dmab[bufidx];
		dmab[(bufidx ^= 1)].used = 0;
		if (thedev.status > 0) {
			playptr = db->buf;
			playlen = db->used;
		} else
			timera_disable ();
		if (audio_rsel)
			wakeselect (audio_rsel);
	}
}

static void
flushme (long proc)
{
	short sr = spl7 ();
	if (thedev.status == 1)
		thedev.status = 2;
	spl (sr);
}

/*
 * copy (and convert) samples from user space
 */
static long
psg_copyin (const char *buf, long len)
{
	long cando;
	short i, sr;

	sr = spl7 ();
	if (thedev.status > 1) {
		spl (sr);
		return 0;
	}
	i = bufidx;
	spl (sr);

	cando = BUFSIZE - dmab[i].used;
	if (len < cando)
		cando = len;
	if (cando <= 0)
		return 0;
	(*thedev.copyfn) (dmab[i].buf + dmab[i].used, buf, cando);
	dmab[i].used += cando;
	sr = spl7 ();
	if (thedev.status == 0) {
		thedev.status = 1;
		bufidx ^= 1;
		playptr = dmab[i].buf;
		playlen = dmab[i].used;
		gi_init ();
		timera_enable (ta_ctrl);
	} else {
		long timeleft;
		/*
		 * one buffer is currenlty beeing played, the
		 * other is currently beeing filled. If at least
		 * two thirds of this buffer full, then "flush" it,
		 * otherwise set a timer that "flushes" the buffer
		 * before playing of the other buffer is finished.
		 *
		 * "flushing" means setting playing to 2, i.e.
		 * allowing psg_timer() to play the buffer.
		 */
		if (dmab[i].used >= BUFSIZE*2L/3 ||
		    (timeleft = playlen*1000/thedev.srate - 10) < 10) {
			thedev.status = 2;
			spl (sr);
			if (tmout) {
				cancelroottimeout (tmout);
				tmout = 0;
			}
			return cando;
		}
		if (!tmout) {
			spl (sr);
			tmout = addroottimeout (timeleft, flushme, 0);
			return cando;
		}
	}
	spl (sr);
	return cando;
}

static long
psg_copyout (char *buf, long len)
{
	return 0L;
}

static long
psg_ioctl (mode, buf)
	short mode;
	void *buf;
{
	unsigned char ctrl, data;
	long r, arg = (long)buf;
	short sr;

	switch (mode) {
	case AIOCGSPEED:
		*(long *)arg = thedev.srate;
		break;

	case AIOCSSPEED:
		r = psg_freq (&arg, &ctrl, &data);
		if (r < 0)
			return r;
		thedev.srate = arg;
		sr = spl7 ();
		timera_disable ();
		MFP_TADR = data;
		if (thedev.status)
			timera_enable (ta_ctrl);
		spl (sr);
		break;

	case AIOCGCHAN:
		*(long *)arg = thedev.curchans;
		break;

	case AIOCSCHAN:
		if (arg != thedev.maxchans)
			return EINVAL;
		break;
	default:
		return ENOSYS;
	}
	return 0;
}

