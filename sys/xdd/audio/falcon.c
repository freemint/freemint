/*
 *	/dev/audio for Atari Ste, MegaSte, TT, Falcon running Mint.
 *	(F030 DMA stuff).
 *
 *	Based on audiodev 0.1 by Charles Briscoe-Smith, which is in
 *	turn based on lpdev 0.6 by Thierry Bousch.
 *
 *	10/28/95, Kay Roemer.
 *
 *	9705, John Blakeley - got the it to work on the Falcon.
 *	9802, John Blakeley - implementing 16bit support for F030.
 *	      			Re-wrote bits to make recording easier?!
 */

# include "global.h"
# include "mint/dcntl.h"

# include "dmasnd.h"
# include "device.h"
# include "mfp.h"

# include <mint/falcon.h>


static void		fal_reset (void);
static void		fal_timer (void);
static long		fal_copyin (const char *buf, long len);
static long		fal_copyout (char *buf, long len);
static long		fal_ioctl (short mode, void *arg);
static long		fal_wspace (void);
static long		fal_rspace (void);
static long		fal_mix_ioctl (short cmd, void *arg);

#undef DMActl
#define DMActl (*(volatile unsigned char *)0xffff8901)
#define FalInt (*(volatile unsigned char *)0xffff8900)

/*
 * install things
 */
long
fal_init (struct device *dev)
{
	long snd_cookie;

	if (get_toscookie (COOKIE__SND, &snd_cookie) ||
	    !(snd_cookie & ( SND_16BIT | SND_DSP | SND_MATRIX) ) ) {
		/*
		 * _SND cookie not set or no Falcon
		 * compatible sound hardware.
		 */
		return 1;
	}
	if (MFP_IERA & MFP_IMRA & 0x20) {
		c_conws ("timer A in use!\r\n");
		return 1;
	}

	dev->maxchans = 8;
	dev->reset = fal_reset;
	dev->copyin = fal_copyin;
	dev->copyout = fal_copyout;
	dev->ioctl = fal_ioctl;
	dev->wspace = fal_wspace;
	dev->rspace = fal_rspace;
	dev->format_map = AFMT_U8|AFMT_S8|AFMT_ULAW|AFMT_U16|AFMT_S16;
	
	Devconnect( DMAPLAY, DAC|EXTOUT, CLK25M, CLKOLD, 1 );
	DMActl = 0;
	FalInt = 0;

	Soundcmd( SETPRESCALE, PRE160 );
	Setmode( MONO8 );
	dev->curformat = AFMT_S8;
	dev->curchans = 1;
	dev->status = 0;

	timer_func = fal_timer;

	Jdisint (13);
	Xbtimer(0, 8, 1, new_timera_vector);
	Jenabint (13);
	return 0;
}

static long
fal_wspace (void)
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
fal_rspace (void)
{
	return 0L;
}

static void
fal_reset (void)
{
	short sr = spl7 ();

	thedev.status = 0;
	DMActl = DMA_OFF;
	FalInt = 0;
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
fal_timer (void)
{
	short i, sr;

	sr = spl7 ();
	i = bufidx ^ 1;
	if (thedev.status) {
		if (--thedev.status == 0) {
			dmab[i].used = 0;
			DMActl = DMA_OFF;
			FalInt = 0;
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
fal_copyin (const char *buf, long len)
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
	 * DMA wants even number of samples. If odd, then play the
	 * last sample twice..
	 */
	if ((thedev.curchans & 1) && (cando & 1) && (thedev.ssize == 8)) {
		dmab[i].buf[dmab[i].used] = dmab[i].buf[dmab[i].used - 1];
		++dmab[i].used;
	}
	if (!thedev.status) {
		thedev.status = 1;
		bufidx ^= 1;
		DMASetBase (dmab[i].buf);
		DMASetEnd (dmab[i].buf + dmab[i].used);
		DMActl = DMA_PLAY;
	} else {
		thedev.status = 2;
		DMASetBase (dmab[i].buf);
		DMASetEnd (dmab[i].buf + dmab[i].used);
		DMActl = DMA_PLAY|DMA_LOOP;
		FalInt = 0x4;
	}
	timera_sti ();
	return cando;
}

static long
fal_copyout (char *buf, long len)
{
	return 0L;
}

static long
fal_ioctl (mode, buf)
	short mode;
	void *buf;
{
	long arg = (long)buf;

	switch (mode) {
	case AIOCGSPEED:
		*(long *)arg = thedev.srate;
		break;

	case AIOCSSPEED:
		if (arg < (6285L+8195L)/2)	{
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLKOLD, 1);
			Soundcmd (SETPRESCALE, PREMUTE);
			thedev.srate = 0L;
		} else if (arg < (8195L+9834L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK8K, 1);
			thedev.srate = 8195L;
		} else if (arg < (9834L+12292L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK10K, 1);
			thedev.srate = 9834L;
		} else if (arg < (12292L+12517L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK12K, 1);
			thedev.srate = 12292L;
		} else if (arg < (12517L+16390L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLKOLD, 1);
			Soundcmd (SETPRESCALE, PRE640);
			thedev.srate = 12517L;
		} else if (arg < (16390L+19668L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK16K, 1);
			thedev.srate = 16390L;
		} else if (arg < (19668L+24585L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK20K, 1);
			thedev.srate = 19668L;
		} else if (arg < (24585L+25033L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK25K, 1);
			thedev.srate = 24585L;
		} else if (arg < (25033L+32780L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLKOLD, 1);
			Soundcmd (SETPRESCALE, PRE320);
			thedev.srate = 25033L;
		} else if (arg < (32780L+49170L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK33K, 1);
			thedev.srate = 32780L;
		} else if (arg < (49170L+50066L)/2) {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLK50K, 1);
			thedev.srate = 49170L;
		} else {
			Devconnect (DMAPLAY, DAC|EXTOUT, CLK25M, CLKOLD, 1);
			Soundcmd (SETPRESCALE, PRE160);
			thedev.srate = 50066L;
		}
		break;

	case AIOCGCHAN:
		*(long *)arg = thedev.curchans;
		break;

	case AIOCSCHAN:
		switch (arg) {
			case 1:						/* MONO */
				Settracks (0, 0);
				thedev.curchans = 1;
				if (thedev.ssize & 8)
					Setmode (MONO8);
				else
					Setmode (STEREO16);
				break;
			case 2:						/* STEREO */
				Settracks (0, 0);
				thedev.curchans = 2;
				if (thedev.ssize & 8)
					Setmode (STEREO8);
				else
					Setmode (STEREO16);
				break;
			case 4:						/* QUAD */
				Settracks (1, 1);
				thedev.curchans = 4;
				if (thedev.ssize & 8)
					Setmode (STEREO8);
				else
					Setmode (STEREO16);
				break;
			case 6:						/* HEXAPHONIC?! */
				Settracks (2, 2);
				thedev.curchans = 6;
				if (thedev.ssize & 8)
					Setmode (STEREO8);
				else
					Setmode (STEREO16);
				break;
			case 8:						/* OCTAPHONIC?! */
				Settracks (3, 3);
				thedev.curchans = 8;
				if (thedev.ssize & 8)
					Setmode (STEREO8);
				else
					Setmode (STEREO16);
				break;
			default:
				return EINVAL;
		}
		break;

	default:
		return ENOSYS;
	}
	return 0;
}

#define FAL_SCALE(v)	(((v)*15/100) << 4)
#define FAL_CHECK(v)	((v) >= 0 && (v) <= 100)

long
fal_mix_init (struct device *dev)
{
	long snd_cookie;

	if (get_toscookie (COOKIE__SND, &snd_cookie) ||
	    !(snd_cookie & (SND_16BIT|SND_DSP|SND_MATRIX))) {
		/*
		 * _SND cookie not set or no Falcon compatible
		 * sound hardware
		 */
		return 1;
	}
	Soundcmd(LTATTEN,FAL_SCALE(0));
	Soundcmd(RTATTEN,FAL_SCALE(0));

	dev->mix_ioctl = fal_mix_ioctl;
	return 0;
}

static long
fal_mix_ioctl (mode, buf)
	short mode;
	void *buf;
{
	long arg = (long)buf;

	switch (mode) {
	case AIOCSVOLUME:
		if (!FAL_CHECK (arg))
			return EINVAL;
		arg = 100 - arg;
		Soundcmd(LTATTEN, FAL_SCALE(arg));
		Soundcmd(RTATTEN, FAL_SCALE(arg));
		break;

	case AIOCSLVOLUME:
		if (!FAL_CHECK (arg))
			return EINVAL;
		arg = 100 - arg;
		Soundcmd(LTATTEN, FAL_SCALE(arg));
		break;

	case AIOCSRVOLUME:
		if (!FAL_CHECK (arg))
			return EINVAL;
		arg = 100 - arg;
		Soundcmd(RTATTEN, FAL_SCALE(arg));
		break;

	default:
		return ENOSYS;
	}
	return 0;
}
