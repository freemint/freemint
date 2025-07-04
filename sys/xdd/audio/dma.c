/*
 *	/dev/audio for Atari Ste, MegaSte, TT, Falcon running Mint.
 *	(Ste DMA stuff).
 *
 *	Based on audiodev 0.1 by Charles Briscoe-Smith, which is in
 *	turn based on lpdev 0.6 by Thierry Bousch.
 *
 *	10/28/95, Kay Roemer.
 *
 *	9802, John Blakeley - brought in to line with the falcon approach.
 */

# include "global.h"
# include "mint/ioctl.h"

# include "dmasnd.h"
# include "device.h"
# include "mfp.h"

# include <mint/osbind.h>


static void		dma_reset (void);
static void		dma_timer (void);
static long		dma_copyin (const char *buf, long len);
static long		dma_copyout (char *buf, long len);
static long		dma_ioctl (short mode, void *arg);
static long		dma_wspace (void);
static long		dma_rspace (void);


/*
 * install things
 */
long dma_init(struct device *dev);
long
dma_init (struct device *dev)
{
	long snd_cookie;

	if (get_toscookie (COOKIE__SND, (unsigned long *)&snd_cookie) ||
	    !(snd_cookie & 2)) {
		/*
		 * _SND cookie not set or no Ste
		 * compatible sound hardware.
		 */
		return 1;
	}
	if (MFP_IERA & MFP_IMRA & 0x20) {
		c_conws ("timer A in use!\r\n");
		return 1;
	}

	dev->maxchans = 2;
	dev->reset = dma_reset;
	dev->copyin = dma_copyin;
	dev->copyout = dma_copyout;
	dev->ioctl = dma_ioctl;
	dev->wspace = dma_wspace;
	dev->rspace = dma_rspace;
	dev->format_map = AFMT_U8|AFMT_S8|AFMT_ULAW;

	DMActl = DMA_OFF;
	dev->status = 0;
	DMAsmc = DMA_12KHZ|DMA_MONO;
	dev->curchans = 1;
	dev->curformat = AFMT_S8;
	dev->srate = 12517;

	timer_func = dma_timer;

	Jdisint (13);
	Xbtimer (0, 8, 1, new_timera_vector);
	Jenabint (13);
	return 0;
}

static long
dma_wspace (void)
{
	short sr = spl7 ();
	long avail;

	avail = BUFSIZE - dmab[bufidx].used;
	if (!thedev.status)
		avail += BUFSIZE;
	spl (sr);
	return avail;
}

static long
dma_rspace (void)
{
	return 0L;
}

static void
dma_reset (void)
{
	short sr = spl7 ();

	thedev.status = 0;
	DMActl = DMA_OFF;
	dmab[0].used = 0;
	dmab[1].used = 0;
	spl (sr);
	if (audio_rsel)
		wakeselect (audio_rsel);
}

/*
 * timer A (frame done) interrupt
 */
static void
dma_timer (void)
{
	short i, sr;

	sr = spl7 ();
	i = bufidx ^ 1;
	if (thedev.status) {
		if (--thedev.status == 0) {
			dmab[i].used = 0;
			DMActl = DMA_OFF;
		} else {
			dmab[i].used = 0;
			DMActl = DMA_PLAY;
			bufidx = i;
		}
		spl (sr);
		if (audio_rsel)
			wakeselect (audio_rsel);
	} else
		spl (sr);
}

/*
 * copy (and convert) samples from user space
 */
static long
dma_copyin (const char *buf, long len)
{
	long cando;
	short i;

	timera_cli ();
	i = bufidx;
	cando = BUFSIZE - dmab[i].used;
	if (len < cando)
		cando = len;
	if (cando <= 0) {
		timera_sti ();
		return 0;
	}
	(*thedev.copyfn) (dmab[i].buf + dmab[i].used, buf, cando);
	dmab[i].used += cando;
	/*
	 * DMA wants event number of samples. If odd, then play the
	 * last sample twice..
	 */
	if ((thedev.curchans ==1) && (cando & 1)) {
		dmab[i].buf[dmab[i].used] = dmab[i].buf[dmab[i].used - 1];
		++dmab[i].used;
	}
	if (thedev.status == 0) {
		thedev.status = 1;
		bufidx ^= 1;
		DMASetBase (dmab[i].buf);
		DMASetEnd  (dmab[i].buf + dmab[i].used);
		DMActl = DMA_PLAY;
	} else {
		thedev.status = 2;
		DMASetBase (dmab[i].buf);
		DMASetEnd  (dmab[i].buf + dmab[i].used);
		DMActl = DMA_PLAY|DMA_LOOP;
	}
	timera_sti ();
	return cando;
}

static long
dma_copyout (char *buf, long len)
{
	return 0L;
}

static long
dma_ioctl (short mode, void *buf)
{
	long arg = (long)buf;

	switch (mode) {
	case AIOCGSPEED:
		*(long *)arg = thedev.srate;
		break;

	case AIOCSSPEED:
		if (arg < (6258L+12517L)/2) {
			arg = DMA_6KHZ;
			thedev.srate = 6258L;
		}
		else if (arg < (12517L+25033L)/2) {
			arg = DMA_12KHZ;
			thedev.srate = 12517L;
		}
		else if (arg < (25033L+50066L)/2) {
			arg = DMA_25KHZ;
			thedev.srate = 25033L;
		}
		else {
			arg = DMA_50KHZ;
			thedev.srate = 50066L;
		}
		DMAsmc = (DMAsmc & DMA_MONO) | arg;
		break;

	case AIOCGCHAN:
		*(long *)arg = thedev.curchans;
		break;

	case AIOCSCHAN:
		if ((arg > thedev.maxchans) || (!arg))	
			return EINVAL;
		else if (arg == thedev.maxchans) {		/* STE so stereo */
			thedev.curchans = 2;
			DMAsmc = (DMAsmc & 3);
		} else {
			thedev.curchans = 1;
			DMAsmc = (DMAsmc & 3) | DMA_MONO;
		}
		break;

	default:
		return ENOSYS;
	}
	return 0;
}

