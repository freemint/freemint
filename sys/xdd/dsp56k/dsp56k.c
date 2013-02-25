/*
 * The DSP56001 Device Driver, saviour of the Free World(tm)
 *
 * Authors: Fredrik Noring   <noring@lysator.liu.se>
 *	    lars brinkhoff   <f93labr@dd.chalmers.se>
 *	    Tomas Berndtsson <tobe@lysator.liu.se>
 *
 * First version May 1996
 *
 * History:
 *  97-01-29   Tomas Berndtsson,
 *		 Integrated with Linux 2.1.21 kernel sources.
 *  97-02-15   Tomas Berndtsson,
 *		 Fixed for kernel 2.1.26
 *
 * BUGS:
 *  Hmm... there must be something here :)
 *
 * Copyright (C) 1996,1997 Fredrik Noring, lars brinkhoff & Tomas Berndtsson
 *
 * FreeMiNT port by Konrad M. Kokoszkiewicz
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

# include "mint/mint.h"
# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/errno.h"
# include "mint/falcon.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/signal.h"
# include "mint/ssystem.h"
# include "mint/stat.h"
# include "cookie.h"

# include "dsp56k.h"
# include "atarihw.h"


/* minor devices */
#define DSP56K_DEV_56001	0    /* The only device so far */

#define TIMEOUT			10	/* Host port timeout in number of tries */
#define MAXIO			2048	/* Maximum number of words before sleep */
#define DSP56K_MAX_BINARY_LENGTH (3*64*1024L)

#define DSP56K_TX_INT_ON	dsp56k_host_interface.icr |=  DSP56K_ICR_TREQ
#define DSP56K_RX_INT_ON	dsp56k_host_interface.icr |=  DSP56K_ICR_RREQ
#define DSP56K_TX_INT_OFF	dsp56k_host_interface.icr &= ~DSP56K_ICR_TREQ
#define DSP56K_RX_INT_OFF	dsp56k_host_interface.icr &= ~DSP56K_ICR_RREQ

#define DSP56K_TRANSMIT		(dsp56k_host_interface.isr & DSP56K_ISR_TXDE)
#define DSP56K_RECEIVE		(dsp56k_host_interface.isr & DSP56K_ISR_RXDF)

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))


/* MiNT definitions */

# define BOOT_MSG	"\033p Motorola DSP 56001 device version " __DATE__ " \033q\r\n" \
			" \275 1996-99 F. Noring, L. Brinkhoff, T. Berndtsson\r\n\r\n"

# define MSG_FAILURE	"\7Not installed, Dcntl() failed!\r\n"

# define DEBUG(x)
# define ALERT(x,y)	KERNEL_ALERT (x,y)
# define TRACE(x)

DEVDRV * _cdecl init(struct kerinfo *k);

/* MiNT device driver structure */

static long _cdecl dsp56k_open		(FILEPTR *f);
static long _cdecl dsp56k_write		(FILEPTR *f, const char *buf, long bytes);
static long _cdecl dsp56k_read		(FILEPTR *f, char *buf, long count);
static long _cdecl dsp56k_lseek		(FILEPTR *f, long where, int whence);
static long _cdecl dsp56k_ioctl		(FILEPTR *f, int cmd, void *arg);
static long _cdecl dsp56k_datime	(FILEPTR *f, ushort *timeptr, int wrflag);
static long _cdecl dsp56k_close		(FILEPTR *f, int pid);
static long _cdecl dsp56k_select	(FILEPTR *f, long proc, int mode);
static void _cdecl dsp56k_unselect	(FILEPTR *f, long proc, int mode);

static DEVDRV devtab =
{
	dsp56k_open,
	dsp56k_write, dsp56k_read, dsp56k_lseek, dsp56k_ioctl, dsp56k_datime,
	dsp56k_close,
	dsp56k_select, dsp56k_unselect
};

struct kerinfo *kernel;
static ushort date;
static ushort time;

/* Device descriptor */
static struct dev_descr dev_descriptor =
{
	&devtab,
	DSP56K_DEV_56001,
	0,
	NULL,
	0,
	S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
	NULL,
	0,
	0
};

char name[]="u:\\dev\\dsp56k";

/* Define some replacement for Linux things */

# define _HZ_200	0x4baL

static inline void _cdecl
udelay(ulong ticks)				/* Minimum delay of `ticks' ticks */
{
	ulong hzbeg, hzend;

	hzbeg = *(ulong *)_HZ_200;
	hzend = hzbeg + ticks;

	if (hzend < hzbeg) {
		hzbeg = *(ulong *)_HZ_200;	/* Counter overflow, try again */
		hzend = hzbeg + ticks;
	}

	while (*(ulong *)_HZ_200 < hzend)
		nap(20);	
}

static inline long _cdecl
dsp_lock(void)
{
	return Dsp_Lock();
}

static inline void _cdecl
dsp_unlock(void)
{
	Dsp_Unlock();
}


# define wait_some(n)	nap(n)

#define handshake(count, maxio, timeout, ENABLE, f) \
{ \
	long i, t, m; \
	while (count > 0) { \
		m = min(count, maxio); \
		for (i = 0; i < m; i++) { \
			for (t = 0; t < timeout && !ENABLE; t++) \
				wait_some(2); \
			if (!ENABLE) \
				return EIO; \
			f; \
		} \
		count -= m; \
		if (m == maxio) wait_some(2); \
	} \
}

#define tx_wait(n) \
{ \
	short t; \
	for (t = 0; t < n && !DSP56K_TRANSMIT; t++) \
		wait_some(1); \
	if (!DSP56K_TRANSMIT) { \
		return EIO; \
	} \
}

#define rx_wait(n) \
{ \
	short t; \
	for (t = 0; t < n && !DSP56K_RECEIVE; t++) \
		wait_some(1); \
	if (!DSP56K_RECEIVE) { \
		return EIO; \
	} \
}

/* DSP56001 bootstrap code */
static char bootstrap[] =
{
	0x0c, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x60, 0xf4, 0x00, 0x00, 0x00, 0x4f, 0x61, 0xf4,
	0x00, 0x00, 0x7e, 0xa9, 0x06, 0x2e, 0x80, 0x00, 0x00, 0x47,
	0x07, 0xd8, 0x84, 0x07, 0x59, 0x84, 0x08, 0xf4, 0xa8, 0x00,
	0x00, 0x04, 0x08, 0xf4, 0xbf, 0x00, 0x0c, 0x00, 0x00, 0xfe,
	0xb8, 0x0a, 0xf0, 0x80, 0x00, 0x7e, 0xa9, 0x08, 0xf4, 0xa0,
	0x00, 0x00, 0x01, 0x08, 0xf4, 0xbe, 0x00, 0x00, 0x00, 0x0a,
	0xa9, 0x80, 0x00, 0x7e, 0xad, 0x08, 0x4e, 0x2b, 0x44, 0xf4,
	0x00, 0x00, 0x00, 0x03, 0x44, 0xf4, 0x45, 0x00, 0x00, 0x01,
	0x0e, 0xa0, 0x00, 0x0a, 0xa9, 0x80, 0x00, 0x7e, 0xb5, 0x08,
	0x50, 0x2b, 0x0a, 0xa9, 0x80, 0x00, 0x7e, 0xb8, 0x08, 0x46,
	0x2b, 0x44, 0xf4, 0x45, 0x00, 0x00, 0x02, 0x0a, 0xf0, 0xaa,
	0x00, 0x7e, 0xc9, 0x20, 0x00, 0x45, 0x0a, 0xf0, 0xaa, 0x00,
	0x7e, 0xd0, 0x06, 0xc6, 0x00, 0x00, 0x7e, 0xc6, 0x0a, 0xa9,
	0x80, 0x00, 0x7e, 0xc4, 0x08, 0x58, 0x6b, 0x0a, 0xf0, 0x80,
	0x00, 0x7e, 0xad, 0x06, 0xc6, 0x00, 0x00, 0x7e, 0xcd, 0x0a,
	0xa9, 0x80, 0x00, 0x7e, 0xcb, 0x08, 0x58, 0xab, 0x0a, 0xf0,
	0x80, 0x00, 0x7e, 0xad, 0x06, 0xc6, 0x00, 0x00, 0x7e, 0xd4,
	0x0a, 0xa9, 0x80, 0x00, 0x7e, 0xd2, 0x08, 0x58, 0xeb, 0x0a,
	0xf0, 0x80, 0x00, 0x7e, 0xad
};
static short sizeof_bootstrap = 375;

struct dsp56k_device
{
	short in_use;
	long maxio, timeout;
	short tx_wsize, rx_wsize;
};
static struct dsp56k_device dsp56k;

static long
dsp56k_reset(void)
{
	uchar status;

	/* Power down the DSP */
	sound_ym.rd_data_reg_sel = 14;
	status = sound_ym.rd_data_reg_sel & 0xef;
	sound_ym.wd_data = status;
	sound_ym.wd_data = status | 0x10;

	udelay(10);

	/* Power up the DSP */
	sound_ym.rd_data_reg_sel = 14;
	sound_ym.wd_data = sound_ym.rd_data_reg_sel & 0xef;

	return 0;
}

static long
dsp56k_upload(uchar *bin, long len)
{
	long i;
	uchar *p;

	dsp56k_reset();

	p = (unsigned char *)bootstrap;
	for (i = 0; i < sizeof_bootstrap / 3; i++)
	{
		/* tx_wait(10); */
		dsp56k_host_interface.data.b[1] = *p++;
		dsp56k_host_interface.data.b[2] = *p++;
		dsp56k_host_interface.data.b[3] = *p++;
	}
	for (; i < 512; i++)
	{
		/* tx_wait(10); */
		dsp56k_host_interface.data.b[1] = 0;
		dsp56k_host_interface.data.b[2] = 0;
		dsp56k_host_interface.data.b[3] = 0;
	}

	for (i = 0; i < len; i++)
	{
		tx_wait(10);
		dsp56k_host_interface.data.b[1] = *bin++;
		dsp56k_host_interface.data.b[2] = *bin++;
		dsp56k_host_interface.data.b[3] = *bin++;
	}

	tx_wait(10);
	dsp56k_host_interface.data.l = 3;    /* Magic execute */

	return 0;
}

static long _cdecl
dsp56k_read(FILEPTR *f, char *buf, long count)
{
	short dev = f->fc.aux;
	
	switch(dev)
	{
	case DSP56K_DEV_56001:
	{

		long n;

		/* Don't do anything if nothing is to be done */
		if (!count) return 0;

		n = 0;
		switch (dsp56k.rx_wsize)
		{
		case 1:  /* 8 bit */
		{
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_RECEIVE,
				buf[n++] = dsp56k_host_interface.data.b[3]);
			return n;
		}
		case 2:  /* 16 bit */
		{
			short *data;

			count /= 2;
			data = (short*) buf;
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_RECEIVE,
				data[n++] = dsp56k_host_interface.data.w[1]);
			return 2*n;
		}
		case 3:  /* 24 bit */
		{
			count /= 3;
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_RECEIVE,
				buf[n++] = dsp56k_host_interface.data.b[1];
				buf[n++] = dsp56k_host_interface.data.b[2];
				buf[n++] = dsp56k_host_interface.data.b[3]);
			return 3*n;
		}
		case 4:  /* 32 bit */
		{
			long *data;

			count /= 4;
			data = (long*) buf;
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_RECEIVE,
				data[n++] = dsp56k_host_interface.data.l);
			return 4*n;
		}
		}
		return EFAULT;
	}

	default:
		ALERT("DSP56k driver: Unknown minor device: %d\n", dev);
		return ENXIO;
	}
}

static long _cdecl
dsp56k_write(FILEPTR *f, const char *buf, long count)
{
	short dev = f->fc.aux;

	switch(dev)
	{
	case DSP56K_DEV_56001:
	{
		long n;

		/* Don't do anything if nothing is to be done */
		if (!count) return 0;

		n = 0;
		switch (dsp56k.tx_wsize)
		{
		case 1:  /* 8 bit */
		{
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_TRANSMIT,
				dsp56k_host_interface.data.b[3] = buf[n++]);
			return n;
		}
		case 2:  /* 16 bit */
		{
			const short *data;

			count /= 2;
			data = (const short *) buf;
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_TRANSMIT,
				dsp56k_host_interface.data.w[1] = data[n++]);
			return 2*n;
		}
		case 3:  /* 24 bit */
		{
			count /= 3;
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_TRANSMIT,
				dsp56k_host_interface.data.b[1] = buf[n++];
				dsp56k_host_interface.data.b[2] = buf[n++];
				dsp56k_host_interface.data.b[3] = buf[n++]);
			return 3*n;
		}
		case 4:  /* 32 bit */
		{
			const long *data;

			count /= 4;
			data = (const long *) buf;
			handshake(count, dsp56k.maxio, dsp56k.timeout, DSP56K_TRANSMIT,
				dsp56k_host_interface.data.l = data[n++]);
			return 4*n;
		}
		}

		return EFAULT;
	}
	default:
		ALERT("DSP56k driver: Unknown minor device: %d\n",dev);
		return ENXIO;
	}
}

static long _cdecl
dsp56k_ioctl(FILEPTR *f, int cmd, void *buf)
{
	unsigned long arg = (unsigned long) buf;
	short dev = f->fc.aux;

	switch (dev)
	{
	case DSP56K_DEV_56001:

		switch(cmd)
		{
		case FIONREAD:
		case FIONWRITE:
		case FIOEXCEPT:
		{
			*(long *)arg = 0;
			break;
		}
		case DSP56K_UPLOAD:
		{
			char *bin;
			long r, len;

			struct dsp56k_upload *binary = buf;

			len = binary->len;
			bin = binary->bin;
			if (len == 0)
				return EINVAL;	 /* nothing to upload?!? */
			
			if (len > DSP56K_MAX_BINARY_LENGTH)
				return EINVAL;
			

			r = dsp56k_upload((unsigned char *)bin, len);
			if (r < 0)
				return r;
			
			break;
		}
		case DSP56K_SET_TX_WSIZE:
			if (arg > 4 || arg < 1)
				return EINVAL;
			dsp56k.tx_wsize = (short) arg;
			break;
		case DSP56K_SET_RX_WSIZE:
			if (arg > 4 || arg < 1)
				return EINVAL;
			dsp56k.rx_wsize = (short) arg;
			break;
		case DSP56K_HOST_FLAGS:
		{
			ushort dir, out, status;
			struct dsp56k_host_flags *hf = buf;

			dir = hf->dir;
			out = hf->out;

			if ((dir & 0x1) && (out & 0x1))
				dsp56k_host_interface.icr |= DSP56K_ICR_HF0;
			else if (dir & 0x1)
				dsp56k_host_interface.icr &= ~DSP56K_ICR_HF0;
			if ((dir & 0x2) && (out & 0x2))
				dsp56k_host_interface.icr |= DSP56K_ICR_HF1;
			else if (dir & 0x2)
				dsp56k_host_interface.icr &= ~DSP56K_ICR_HF1;

			status = 0;
			if (dsp56k_host_interface.icr & DSP56K_ICR_HF0) status |= 0x1;
			if (dsp56k_host_interface.icr & DSP56K_ICR_HF1) status |= 0x2;
			if (dsp56k_host_interface.isr & DSP56K_ISR_HF2) status |= 0x4;
			if (dsp56k_host_interface.isr & DSP56K_ISR_HF3) status |= 0x8;
			hf->status = status;
			return 0;
		}
		case DSP56K_HOST_CMD:
			if (arg > 31 || arg < 0)
				return EINVAL;
			dsp56k_host_interface.cvr = (uchar)((arg & DSP56K_CVR_HV_MASK) |
							     DSP56K_CVR_HC);
			break;
		default:
			return EINVAL;
		}
		return 0;

	default:
		ALERT("DSP56k driver: Unknown minor device: %d\n", dev);
		return ENXIO;
	}
}

static long _cdecl
dsp56k_open(FILEPTR *f)
{
	short dev = f->fc.aux;
	
	switch(dev)
	{
	case DSP56K_DEV_56001:

		if (dsp56k.in_use)
			return EBUSY;
		
		if (dsp_lock())
			return EBUSY;
		
		dsp56k.in_use = 1;
		dsp56k.timeout = TIMEOUT;
		dsp56k.maxio = MAXIO;
		dsp56k.rx_wsize = dsp56k.tx_wsize = 4;

		DSP56K_TX_INT_OFF;
		DSP56K_RX_INT_OFF;

		/* Zero host flags */
		dsp56k_host_interface.icr &= ~DSP56K_ICR_HF0;
		dsp56k_host_interface.icr &= ~DSP56K_ICR_HF1;

		break;

	default:
		ALERT("DSP56k driver: Unknown minor device: %d\n", dev);
		return ENXIO;
	}

	return 0;
}

static long _cdecl
dsp56k_close(FILEPTR *f, int pid)
{
	short dev = f->fc.aux;

	switch (dev)
	{
		case DSP56K_DEV_56001:
		{
			if (pid != p_getpid())
				/* kernel cleaning up after a dead person */
				dsp56k_reset();

			dsp_unlock();
			dsp56k.in_use = 0;
			break;
		}
		default:
		{
			ALERT("DSP56k driver: Unknown minor device: %d\n", dev);
			return ENXIO;
		}
	}

	return 0;
}

static long _cdecl
dsp56k_lseek(FILEPTR *f, long where, int whence)
{
	return 0;
}

static long _cdecl
dsp56k_datime(FILEPTR *f, ushort *timeptr, int wrflag)
{
	if (!wrflag)
	{
		*timeptr++ = time;
		*timeptr = date;

		return 0;
	}

	/* rather than ENOSYS */
	return EINVAL;
}

static long _cdecl
dsp56k_select(FILEPTR *f, long proc, int mode)
{
	/* for now only this */
	return 1;
}

static void _cdecl
dsp56k_unselect(FILEPTR *f, long proc, int mode)
{
}


DEVDRV * _cdecl
init(struct kerinfo *k)
{
	long r;
	long snd = 0;

	kernel = k;

	c_conws (BOOT_MSG);

	r = s_system(S_GETCOOKIE, COOKIE__SND, (long) &snd);
	if (r < 0)
	{
		/* MiNT below 1.15.0 or not MiNT at all -> don't bother */
		c_conws("Not installed, too old MiNT version\r\n");
		return NULL;
	}

	if (!(snd & 0x08L))
	{
		c_conws("Not installed, no DSP chip present\r\n");
		return NULL;
	}

	r = d_cntl (DEV_INSTALL, name, (long) &dev_descriptor);
	r = (r >= 0) ? 1L : 0L;
	if (r)
	{
		date = datestamp;
		time = timestamp;
		dsp56k.in_use = 0;
	}
	else
		c_conws(MSG_FAILURE);

	c_conws("\r\n");

	return (DEVDRV *) r;
}
