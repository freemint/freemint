/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-01-03
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 *
 * changes since last version:
 *
 * 2000-02-28:	(v0.12)
 *
 * - new: added CD loss detection -> SIGHUP
 *
 * 2000-02-20:	(v0.11)
 *
 * - new: implemented final naming scheme
 * - new: started the TT MFP version
 *
 * 2000-01-19:	(v0.10)
 *
 * - inital revision
 *
 *
 * known bugs:
 *
 *
 * todo:
 *
 *
 */

# include "mint/mint.h"

# include "arch/timer.h"
# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/signal.h"
# include "mint/ssystem.h"
# include "mint/stat.h"
# include "cookie.h"

# include <mint/osbind.h>
# include "mfp.h"


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	31
# define VER_STATUS


/*
 * debugging stuff
 */

# if 0
# define DEV_DEBUG	1
# endif

# if 0
# define INT_DEBUG	1
# endif


/*
 * default settings
 */

# define RSVF_MFP	"modem1"
# define RSVF_MFP_TT	"serial1"

# define TTY_MFP	"ttyS0"
# define TTY_MFP_TT	"ttyS2"

# define BDEV_START	6
# define BDEV_OFFSET	6

# define BDEV_MFP	6
# define BDEV_MFP_TT	8


/*
 * size of each buffer, compile time option,
 * allocated through kmalloc on initialization time
 *
 * default to 12 bit = 4kb => 8kb per line (one page)
 * maximum is 15 (limited range of ushort data types)
 */
# define IOBUFBITS	12			/* 4096 */
# define IOBUFSIZE	(1UL << IOBUFBITS)
# define IOBUFMASK	(IOBUFSIZE - 1)


# if 1
/* don't return EWOULDBLOCK as nobody support it at the moment
 */
# undef EWOULDBLOCK
# define EWOULDBLOCK	0
# endif


/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p MFP serial driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\275 1998, 1999 by Rainer Mannigel.\r\n" \
	"\275 2000-2010 by Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# ifdef MILAN
# define MSG_MACHINE	\
	"\033pThis driver requires a Milan!\033q\r\n"
# else
# define MSG_MACHINE	\
	"\033pThis driver requires a ST/TT compatible MFP!\033q\r\n"
# endif

# define MSG_FAILURE	\
	"\7\r\nSorry, driver NOT installed - initialization failed!\r\n\r\n"


/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *kernel;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

typedef struct bauds BAUDS;
struct bauds
{
	ulong	baudrate;
	uchar	timeconst;
	uchar	mode;
};

typedef struct iorec IOREC;
struct iorec
{
	uchar	*buffer;	/* the buffer itself */
	volatile ushort head;	/* next data byte */
	volatile ushort tail;	/* next free byte */
	ushort	low_water;	/* xon mark */
	ushort	high_water;	/* xoff mark */
};

typedef struct iovar IOVAR;
struct iovar
{
	MFP	*regs;		/* MFP register base address */

	volatile ushort	state;		/* receiver state */
# define STATE_HARD		0x1		/* CTS is low */
# define STATE_SOFT		0x2		/* XOFF is set */

	ushort	shake;		/* handshake mode */
# define SHAKE_HARD		STATE_HARD	/* RTS/CTS handshake enabled */
# define SHAKE_SOFT		STATE_SOFT	/* XON/XOFF handshake enabled */

	uchar	rx_full;	/* input buffer full */
	uchar	tx_empty;	/* output buffer empty */

	uchar	sendnow;	/* one byte of control data, bypasses buffer */
	uchar	datmask;	/* AND mask for read from data */

	uchar	iointr;		/* I/O interrupt */
	uchar	stintr;		/* status interrupt */

	ushort	tt_port;	/* TT-MFP */

	IOREC	input;		/* input buffer */
	IOREC	output;		/* output buffer */

	BAUDS	*table;		/* baudrate vector */
	long	baudrate;	/* current baud rate value */

	ushort	res;		/* padding */

	ushort	bdev;		/* BIOS devnumber */
	ushort	iosleepers;	/* number of precesses that are sleeping in I/O */
	ushort	lockpid;	/* file locking */

	ushort	clocal;		/* */
	ushort	brkint;		/* */

	FILEPTR	*open;		/* open FILEPTRs */
	TTY	tty;		/* the tty descriptor */

	char	rsvf_name[10];
	char	tty_name[10];
};


# define XON		0x11
# define XOFF		0x13


/*
 * buffer manipulation - mixed
 */
INLINE int	iorec_empty	(IOREC *iorec);
INLINE int	iorec_full	(IOREC *iorec);
INLINE uchar	iorec_get	(IOREC *iorec);
INLINE int	iorec_put	(IOREC *iorec, uchar data);
INLINE long	iorec_used	(IOREC *iorec);
INLINE long	iorec_free	(IOREC *iorec);
INLINE ushort	iorec_size	(IOREC *iorec);


/*
 * inline assembler primitives - bottom half
 */
INLINE void	rts_on		(MFP *regs);
INLINE void	rts_off		(MFP *regs);
INLINE void	dtr_on		(MFP *regs);
INLINE void	dtr_off		(MFP *regs);
INLINE void	brk_on		(MFP *regs);
INLINE void	brk_off		(MFP *regs);


/*
 * inline assembler primitives - top half
 */
INLINE void	top_rts_on	(IOVAR *iovar);
INLINE void	top_rts_off	(IOVAR *iovar);
INLINE void	top_dtr_on	(IOVAR *iovar);
INLINE void	top_dtr_off	(IOVAR *iovar);
INLINE void	top_brk_on	(IOVAR *iovar);
INLINE void	top_brk_off	(IOVAR *iovar);


/*
 * initialization - top half
 */
INLINE int	init_MFP	(IOVAR **iovar, MFP *regs, ushort tt_port);
INLINE void	init_mfp	(long mch);


/*
 * interrupt handling - bottom half
 */
INLINE void	notify_top_half	(IOVAR *iovar);
static void	wr_mfp		(IOVAR *iovar, MFP *regs);

static void	mfp_dcdint_asm(void);
static void mfp_ctsint_asm(void);
static void mfp_txerror_asm(void);
static void mfp_txempty_asm(void);
static void mfp_rxerror_asm(void);
static void mfp_rxavail_asm(void);

#ifndef MILAN
static void ttmfp_txerror_asm(void);
static void ttmfp_txempty_asm(void);
static void ttmfp_rxerror_asm(void);
static void	ttmfp_rxavail_asm(void);
#endif


/*
 * interrupt handling - top half
 */
static void	check_ioevent	(PROC *p, long arg);
static void	soft_cdchange	(PROC *p, long arg);


/*
 * control functions - top half
 */
INLINE void	send_byte	(IOVAR *iovar);
static long	ctl_TIOCBAUD	(IOVAR *iovar, long *buf);
static ushort	ctl_TIOCGFLAGS	(IOVAR *iovar);
static long	ctl_TIOCSFLAGS	(IOVAR *iovar, ushort flags);
static long	ctl_TIOCSFLAGSB	(IOVAR *iovar, long flags, long mask);


/*
 * start/stop primitives - top half
 */
static void	rx_start	(IOVAR *iovar);
static void	rx_stop		(IOVAR *iovar);
static void	tx_start	(IOVAR *iovar);
static void	tx_stop		(IOVAR *iovar);
static void	flush_output	(IOVAR *iovar);
static void	flush_input	(IOVAR *iovar);


/*
 * bios emulation - top half
 */
static long _cdecl	mfp_instat	(int dev);
static long _cdecl	mfp_in		(int dev);
static long _cdecl	mfp_outstat	(int dev);
static long _cdecl	mfp_out		(int dev, int c);
static long _cdecl	mfp_rsconf	(int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr);


/*
 * device driver routines - top half
 */
static long _cdecl	mfp_open	(FILEPTR *f);
static long _cdecl	mfp_close	(FILEPTR *f, int pid);
static long _cdecl	mfp_write	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	mfp_read	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	mfp_writeb	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	mfp_readb	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	mfp_twrite	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	mfp_tread	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	mfp_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl	mfp_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl	mfp_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	mfp_select	(FILEPTR *f, long proc, int mode);
static void _cdecl	mfp_unselect	(FILEPTR *f, long proc, int mode);


/*
 * device driver maps
 */
static BDEVMAP bios_devtab =
{
	mfp_instat, mfp_in,
	mfp_outstat, mfp_out,
	mfp_rsconf
};

static DEVDRV hsmodem_devtab =
{
	mfp_open,
	mfp_write, mfp_read, mfp_lseek, mfp_ioctl, mfp_datime,
	mfp_close,
	mfp_select, mfp_unselect,
	NULL, NULL
};

static DEVDRV tty_devtab =
{
	mfp_open,
	mfp_twrite, mfp_tread, mfp_lseek, mfp_ioctl, mfp_datime,
	mfp_close,
	mfp_select, mfp_unselect,
	mfp_writeb, mfp_readb
};


/*
 * debugging stuff
 */

# ifdef DEV_DEBUG
#  define DEBUG(x)	KERNEL_DEBUG x
#  define TRACE(x)	KERNEL_TRACE x
#  define ALERT(x)	KERNEL_ALERT x
# else
#  define DEBUG(x)
#  define TRACE(x)
#  define ALERT(x)	KERNEL_ALERT x
# endif

# ifdef INT_DEBUG
#  define DEBUG_I(x)	KERNEL_DEBUG x
#  define TRACE_I(x)	KERNEL_TRACE x
#  define ALERT_I(x)	KERNEL_ALERT x
# else
#  define DEBUG_I(x)
#  define TRACE_I(x)
#  define ALERT_I(x)	KERNEL_ALERT x
# endif

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN buffer manipulation - mixed */

/* input:
 *
 * - iorec_empty : top
 * - iorec_full  : -
 * - iorec_get   : top
 * - iorec_put   : bottom
 * - iorec_used  : bottom & top - uncritical
 * - iorec_free  : -
 * - iorec_size  : uncritical
 *
 * -> interrupt safe interaction
 *
 *
 * output:
 *
 * - iorec_empty : bottom
 * - iorec_full  : -
 * - iorec_get   : bottom
 * - iorec_put   : top
 * - iorec_used  : top
 * - iorec_free  : top
 * - iorec_size  : uncritical
 *
 * -> interrupt safe interaction
 */

INLINE ushort
inc_ptr (ushort ptr)
{
	return (++ptr & IOBUFMASK);
}

INLINE int
iorec_empty (IOREC *iorec)
{
	return (iorec->head == iorec->tail);
}

INLINE int
iorec_full (IOREC *iorec)
{
	return (iorec->head == inc_ptr (iorec->tail));
}

INLINE uchar
iorec_get (IOREC *iorec)
{
	ushort i;

	i = inc_ptr (iorec->head);
	iorec->head = i;

	return iorec->buffer[i];
}

INLINE int
iorec_put (IOREC *iorec, uchar data)
{
	ushort i = inc_ptr (iorec->tail);

	if (i == iorec->head)
	{
		/* buffer full */
		return 0;
	}

	iorec->buffer[i] = data;
	iorec->tail = i;

	return 1;
}

INLINE long
iorec_used (IOREC *iorec)
{
	long tmp;

	tmp = iorec->tail;
	tmp -= iorec->head;

	if (tmp < 0)
		tmp += IOBUFSIZE;

	return tmp;
}

INLINE long
iorec_free (IOREC *iorec)
{
	long tmp;

	tmp = iorec->head;
	tmp -= iorec->tail;

	if (tmp <= 0)
		tmp += IOBUFSIZE;

	return tmp;
}

INLINE ushort
iorec_size (IOREC *iorec)
{
	return IOBUFSIZE;
}

/* END buffer manipulation - mixed */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

/* for interrupt wrapper */
static IOVAR *iovar_mfp;
# ifndef MILAN
static IOVAR *iovar_mfp_tt;
# endif


# define IOVAR_MAX	2

static IOVAR *iovar_tab [IOVAR_MAX * 2];

# define IOVARS(nr)		(iovar_tab [nr])
# define IOVAR_TTY_OFFSET	(IOVAR_MAX)
# define IOVAR_REAL_MAX		(IOVAR_MAX * 2)


static BAUDS baudtable_st [] =
{
	{     50, 0x9a, 2 },
	{     75, 0x66, 2 },
	{    110, 0xAF, 1 },
	{    134, 0x8F, 1 },
	{    150, 0x80, 1 },
	{    200, 0x60, 1 },
	{    300, 0x40, 1 },
	{    600, 0x20, 1 },
	{   1200, 0x10, 1 },
	{   1800, 0x0B, 1 },
	{   2000, 0x0A, 1 },
	{   2400, 0x08, 1 },
	{   3600, 0x05, 1 },
	{   4800, 0x04, 1 },
	{   9600, 0x02, 1 },
	{  19200, 0x01, 1 },
	{      0,    0, 0 }
};

# ifndef MILAN
static BAUDS baudtable_rsve [] =
{
	{     50, 0x9a, 2 },
	{     75, 0x66, 2 },
	{    200, 0x60, 1 },
	{    300, 0x40, 1 },
	{    600, 0x20, 1 },
	{   1200, 0x10, 1 },
	{   1800, 0x0B, 1 },
	{   2000, 0x0A, 1 },
	{   2400, 0x08, 1 },
	{   3600, 0x05, 1 },
	{   4800, 0x04, 1 },
	{   9600, 0x02, 1 },
	{  19200, 0x01, 1 },
	{  38400, 0xAF, 1 },
	{  57600, 0x8F, 1 },
	{ 115200, 0x80, 1 },
	{      0,    0, 0 }
};
# endif

/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN inline assembler primitives - bottom half */

INLINE void
rts_on (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip &= ~GPIP_RTS; */	/* RTS (bit0) = 0 */
	asm volatile
	(
		"bclr.b #0,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread & ~GI_RTS;
# endif
	DEBUG_I (("MFP: RTS on"));
}

INLINE void
rts_off (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip |= GPIP_RTS; */	/* RTS (bit0) = 1 */
	asm volatile
	(
		"bset.b #0,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread | GI_RTS;
# endif
	DEBUG_I (("MFP: RTS off"));
}


INLINE void
dtr_on (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip &= ~GPIP_DTR; */
	asm volatile
	(
		"bclr.b #3,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread & ~GI_DTR;
# endif
	DEBUG_I (("MFP: DTR on"));
}

INLINE void
dtr_off (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip |= GPIP_DTR; */
	asm volatile
	(
		"bset.b #3,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread | GI_DTR;
# endif
	DEBUG_I (("MFP: DTR off"));
}


INLINE void
brk_on (MFP *regs)
{
	regs->tsr |= TSR_SEND_BREAK;
	DEBUG_I (("MFP BRK on"));
}

INLINE void
brk_off (MFP *regs)
{
	regs->tsr &= ~TSR_SEND_BREAK;
	DEBUG_I (("MFP BRK off"));
}

/* END inline assembler primitives - bottom half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN inline assembler primitives - top half */

INLINE void
top_rts_on (IOVAR *iovar)
{
# ifdef MILAN
	rts_on (iovar->regs);
# else
	ushort sr;

	sr = splhigh ();
	rts_on (iovar->regs);
	spl (sr);
# endif
}

INLINE void
top_rts_off (IOVAR *iovar)
{
# ifdef MILAN
	rts_off (iovar->regs);
# else
	ushort sr;

	sr = splhigh ();
	rts_off (iovar->regs);
	spl (sr);
# endif
}

INLINE void
top_dtr_on (IOVAR *iovar)
{
# ifdef MILAN
	dtr_on (iovar->regs);
# else
	ushort sr;

	sr = splhigh ();
	dtr_on (iovar->regs);
	spl (sr);
# endif
}

INLINE void
top_dtr_off (IOVAR *iovar)
{
# ifdef MILAN
	dtr_off (iovar->regs);
# else
	ushort sr;

	sr = splhigh ();
	dtr_off (iovar->regs);
	spl (sr);
# endif
}

INLINE void
top_brk_on (IOVAR *iovar)
{
	ushort sr;

	sr = splhigh ();
	brk_on (iovar->regs);
	spl (sr);
}

INLINE void
top_brk_off (IOVAR *iovar)
{
	ushort sr;

	sr = splhigh ();
	brk_off (iovar->regs);
	spl (sr);
}

/* END inline assembler primitives - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization - top half */

# ifndef MILAN

/* try to autodetect RSVE, RSFI or similiar RS232 speeders
 *
 *  (c) Harun Scheutzow
 *	    developer of RSVE, RSFI, ST_ESCC and author of hsmoda-package
 *
 *  integrated by Juergen Orschiedt
 *
 * noise-free detection of Tx/Rx baudrate
 *  - set MFP to loopback, syncmode, 8bpc, syncchar=0xff
 *  - enable transmitter and measure time between syncdetect
 * depending on the relationship between measured time and
 * timer-d setting we can tell which (if any) speeder we have.
 *
 * returncodes:
 *	0	something wrong (1200 too slow)
 *	1	no speeder detected
 *	2	RSVE detected
 *	3	RSFI detected
 *	4	PLL or fixed Baudrate (Hardware-hack)
 */

static void
set_timer_D (MFP *regs, uchar baud, uchar prescale)
{
	/* set timer d to given value, prescale 4
	 * allow PLL-settling (3 bit-times)
	 */

	int count;

	regs->tcdcr &= 0xf8;		/* disable timer D */
	regs->tddr = baud;		/* preset baudrate */
	regs->tcdcr |= prescale;	/* enable timer D, prescale N */

	for (count = 6; count; count--)
	{
		regs->iprb = ~0x10;
		while (!(regs->iprb & 0x10))
			;
	}
}

INLINE int
detect_MFP_speeder (IOVAR *iovar)
{
	MFP *regs = iovar->regs;
	long count, speeder;
	char imra, imrb;
	ushort sr;

	/* prepare IRQ registers for measurement */

	sr = splhigh ();

	imra = regs->imra;
	imrb = regs->imrb;

	regs->imra = imra & 0xe1;	/* mask off all Rx/Tx ints */
	regs->imrb = imrb & 0xef;	/* disable timer d int in IMRB */
	regs->ierb |= 0x10;		/* enable in IERB (to see pending ints) */

	spl (sr);

	(void) regs->gpip;		/* consume some cycles */
	(void) regs->gpip;

	/* initialize MFP */
	regs->rsr = 0x00;		/* disable Rx */
	regs->tsr = TSR_SOMODE_HIGH;	/* disable Tx, output-level high */
	regs->scr = 0xff;		/* syncchar = 0xff */
	regs->ucr = (UCR_PARITY_OFF | UCR_SYNC_MODE | UCR_CHSIZE_8);
	regs->tsr = (TSR_TX_ENAB | TSR_SOMODE_LOOP);

	/* look at 1200 baud setting (== effective 19200 in syncmode) */
	set_timer_D (regs, 0x10, 0x01);

	sr = splhigh ();

	/* check for fixed speed / bad speed */
	regs->rsr = RSR_RX_ENAB;
	count = -1;
	do {
continue_outer:
		count++;
		regs->iprb = ~0x10;
		do {
			if (regs->iprb & 0x10)
				goto continue_outer;
		}
		while (!(regs->rsr & RSR_SYNC_SEARCH) && count <= 22);
	}
	while (0);

	spl (sr);

	/* for RSxx or standard MFP we have 8 bittimes (count=16) */
	DEBUG (("mfp: detect_MFP_speeder: count[1200] = %ld\n", count));

	if (count < 10)
		/* less than 5 bittimes: primitive speeder */
		speeder = MFP_WITH_PLL;
	else if (count > 22)
		/* something wrong - too slow! */
		speeder = MFP_WITH_WEIRED_CLOCK;
	else
	{
		/* 1200 baud is working, we neither have fixed clock nor simple PLL */
		set_timer_D (regs, 0xaf, 0x01);

		sr = splhigh ();

		/* check for RSxx or Standard MFP with 110 baud
		 * timer D toggles each 290us
		 *	      bps	sync char count
		 * RSVE:    614400	 22.33		 13uS
		 * RSFI:     38400	  1.39		208us
		 * Standard:  1720	  0.06
		 *
		 */
		regs->iprb = ~0x10;

		/* syncronize to timer D */
		while (!(regs->iprb & 0x10))
			;

		regs->iprb = ~0x10;
		count = -1;
		do
		{
continue_outer2:
			regs->rsr = 0;			/* disable Rx */
			count++;
			(void) regs->gpip;		/* delay */
			regs->rsr = RSR_RX_ENAB;	/* enable Rx */
			// nop ();
			do {
				/* increment counter if sync char detected */
				if (regs->rsr & RSR_SYNC_SEARCH)
					goto continue_outer2;
			}
			while (!(regs->iprb & 0x10));
		}
		while (0);

		DEBUG (("mfp: detect_MFP_speeder: count[110] = %ld\n", count));

		if (count < 1)
			/* no speeder detected */
			speeder = MFP_STANDARD;
		else if (count > 4)
			speeder = MFP_WITH_RSVE;
		else
			speeder = MFP_WITH_RSFI;
	}

	spl (sr);

	regs->rsr   = 0x00;		/* disable Rx */
	regs->tsr   = TSR_SOMODE_HIGH;	/* disable Tx, output-level high */
	regs->ucr   = (UCR_PARITY_OFF | UCR_ASYNC_2 | UCR_CHSIZE_8);
	regs->imra  = imra;
	regs->imrb  = imrb;
	regs->ierb &= 0xef;
	regs->ipra  = 0xe1;		/* mask off pending Rx/Tx */
	regs->iprb  = 0xef;		/* mask off pending Timer D */

	return speeder;
}

# endif

/* only in one place used -> INLINE
 */
INLINE int
init_MFP (IOVAR **iovar, MFP *regs, ushort tt_port)
{
	uchar *buffer;

	*iovar = kmalloc (sizeof (**iovar));
	if (!*iovar)
		goto error;

	bzero (*iovar, sizeof (**iovar));

	buffer = kmalloc (2 * IOBUFSIZE);
	if (!buffer)
		goto error;

	(*iovar)->input.buffer		= buffer;
	(*iovar)->input.low_water	= 1 * (IOBUFSIZE / 4);
	(*iovar)->input.high_water	= 3 * (IOBUFSIZE / 4);

	(*iovar)->output.buffer		= buffer + IOBUFSIZE;
	(*iovar)->output.low_water	= 1 * (IOBUFSIZE / 4);
	(*iovar)->output.high_water	= 3 * (IOBUFSIZE / 4);

	(*iovar)->regs = regs;


	/* reset the MFP to default values */
	regs->ucr = UCR_PREDIV | UCR_ASYNC_1;
	regs->rsr = 0x00;
	regs->tsr = 0x00;

	/* set standard baudrate */
	regs->tcdcr &= 0x70;
	regs->tddr = 0x01;
	regs->tcdcr |= 1;

	(*iovar)->datmask = 0xff;
	(*iovar)->baudrate = 9600;

	/* soft line always on for TT-MFP */
	/* ??? if (tt_port)
		(*iovar)->state = STATE_SOFT; */

	/* read current CTS state */
	if (!tt_port && !(regs->gpip & GPIP_CTS))
		/* CTS is on */
		(*iovar)->state |= STATE_HARD;

	/* RTS/CTS handshake */
	if (!tt_port)
		(*iovar)->shake = SHAKE_HARD;

	/* read current CD state */
	if (!tt_port && (regs->gpip & GPIP_DCD))
		/* CD is off, no carrier */
		(*iovar)->tty.state |= TS_BLIND;

	(*iovar)->rx_full = 1;	/* receiver disabled */
	(*iovar)->tx_empty = 1;	/* transmitter empty */

	(*iovar)->sendnow = 0;
	(*iovar)->datmask = 0xff;

	(*iovar)->iointr = 0;
	(*iovar)->stintr = 0;
	(*iovar)->tt_port = tt_port;

	(*iovar)->input.head = (*iovar)->input.tail = 0;
	(*iovar)->output.head = (*iovar)->output.tail = 0;

	(*iovar)->clocal = 0;
	(*iovar)->brkint = 1;


	DEBUG (("init_MFP: iovar initialized for MFP at %lx", regs));
	return 1;

error:
	if (*iovar)
		kfree (*iovar);

	ALERT (("mfp.xdd: kmalloc(%li, %li) fail, out of memory?", sizeof (**iovar), IOBUFSIZE));
	return 0;
}

/* only in one place used -> INLINE
 */
INLINE void
init_mfp (long mch)
{
	/* init our MFP */
	init_MFP (&iovar_mfp, _mfpregs, 0);
	if (iovar_mfp)
	{
		IOVAR *iovar = iovar_mfp;
		uchar reg;
# ifndef MILAN
		int speeder = detect_MFP_speeder (iovar);

		switch (speeder)
		{
			case MFP_WITH_WEIRED_CLOCK:
				iovar->table = baudtable_st;
				c_conws ("original MFP with weired clock (unsupported)\r\n");
				break;
			case MFP_STANDARD:
				iovar->table = baudtable_st;
				c_conws ("original MFP (supported)\r\n");
				break;
			case MFP_WITH_PLL:
				iovar->table = baudtable_st;
				c_conws ("PLL accelerated MFP (unsupported)\r\n");
				break;
			case MFP_WITH_RSVE:
				iovar->table = baudtable_rsve;
				c_conws ("RSVE accelerated MFP (supported)\r\n");
				break;
			case MFP_WITH_RSFI:
				iovar->table = baudtable_st;
				c_conws ("RSVF accelerated MFP (unsupported)\r\n");
				break;
			default:
				FATAL ("mfp: unknown return");
				break;
		}
# else
		iovar->table = baudtable_st;
		c_conws ("Milan MFP (supported)\r\n");
# endif

		iovar->bdev = BDEV_MFP;

		IOVARS (0) = iovar;
		IOVARS (0 + IOVAR_TTY_OFFSET) = iovar;

		strcpy (IOVARS (0)->rsvf_name, RSVF_MFP);
		strcpy (IOVARS (0)->tty_name, TTY_MFP);

		DEBUG (("init_mfp (MFP): installing interrupt handlers ..."));

		Mfpint ( 1, mfp_dcdint_asm);	/* 0x104 */
		Mfpint ( 2, mfp_ctsint_asm);	/* 0x108 */
		Mfpint ( 9, mfp_txerror_asm);	/* 0x124 */
		Mfpint (10, mfp_txempty_asm);	/* 0x128 */
		Mfpint (11, mfp_rxerror_asm);	/* 0x12c */
		Mfpint (12, mfp_rxavail_asm);	/* 0x130 */

		DEBUG (("init_mfp (MFP): done"));


		/* enable interrupts */

		reg = iovar->regs->iera;
		reg |= 0x2 | 0x4 | 0x8 | 0x10;
		iovar->regs->iera = reg;

		reg = iovar->regs->ierb;
		reg |= 0x2 | 0x4;
		iovar->regs->ierb = reg;


		/* set interrupt mask */

		reg = iovar->regs->imra;
		reg |= 0x2 | 0x4 | 0x8 | 0x10;
		iovar->regs->imra = reg;

		reg = iovar->regs->imrb;
		reg |= 0x2 | 0x4;
		iovar->regs->imrb = reg;


		/* enable rx/tx */

		iovar->regs->rsr = RSR_RX_ENAB;
		iovar->regs->tsr = TSR_TX_ENAB;
	}

# ifndef MILAN
# define TT		0x00020000L
	if (mch != TT)
	{
		DEBUG (("init_mfp: initialization done"));
		return;
	}

	/* init TT MFP */
	init_MFP (&iovar_mfp_tt, _mfpregs_tt, 1);
	if (iovar_mfp_tt)
	{
		IOVAR *iovar = iovar_mfp_tt;
		uchar reg;

		iovar->table = baudtable_st;
		c_conws ("TT MFP (supported)\r\n");

		iovar->bdev = BDEV_MFP_TT;

		IOVARS (1) = iovar_mfp_tt;
		IOVARS (1 + IOVAR_TTY_OFFSET) = iovar_mfp_tt;

		strcpy (IOVARS (1)->rsvf_name, RSVF_MFP_TT);
		strcpy (IOVARS (1)->tty_name, TTY_MFP_TT);

		DEBUG (("init_mfp (TT-MFP): installing interrupt handlers ..."));

# define vector(x)	(x / 4)

		(void) Setexc (vector (0x164), ttmfp_txerror_asm);
		(void) Setexc (vector (0x168), ttmfp_txempty_asm);
		(void) Setexc (vector (0x16c), ttmfp_rxerror_asm);
		(void) Setexc (vector (0x170), ttmfp_rxavail_asm);

# undef vector

		DEBUG (("init_mfp (TT_MFP): done"));


		/* enable interrupts */

		reg = iovar->regs->iera;
		reg |= 0x2 | 0x4 | 0x8 | 0x10;
		iovar->regs->iera = reg;


		/* set interrupt mask */

		reg = iovar->regs->imra;
		reg |= 0x2 | 0x4 | 0x8 | 0x10;
		iovar->regs->imra = reg;


		/* enable rx/tx */

		iovar->regs->rsr = RSR_RX_ENAB;
		iovar->regs->tsr = TSR_TX_ENAB;
	}
# endif

	DEBUG (("init_mfp: initialization done"));
}

DEVDRV * _cdecl
init_xdd (struct kerinfo *k)
{
	long mch;
	int i;

	struct dev_descr raw_dev_descriptor =
	{
		&hsmodem_devtab,
		0,		/* dinfo -> fc.aux */
		0,		/* flags */
		NULL,		/* struct tty * */
		0,		/* drvsize */
		S_IFCHR |
		S_IRUSR |
		S_IWUSR |
		S_IRGRP |
		S_IWGRP |
		S_IROTH |
		S_IWOTH,	/* fmode */
		&bios_devtab,	/* bdevmap */
		0,		/* bdev */
		0		/* reserved */
	};

	struct dev_descr tty_dev_descriptor =
	{
		&tty_devtab,
		0,		/* dinfo -> fc.aux */
		O_TTY,		/* flags */
		NULL,		/* struct tty * */
		sizeof(DEVDRV),		/* drvsize */
		S_IFCHR |
		S_IRUSR |
		S_IWUSR |
		S_IRGRP |
		S_IWGRP |
		S_IROTH |
		S_IWOTH,	/* fmode */
		NULL,		/* bdevmap */
		0,		/* bdev */
		0		/* reserved */
	};


	kernel = k;

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);

	DEBUG (("%s: enter init", __FILE__));

	if ((MINT_MAJOR == 0)
		|| ((MINT_MAJOR == 1) && ((MINT_MINOR < 15) || (MINT_KVERSION < 2))))
	{
		c_conws (MSG_MINT);
		goto failure;
	}

	if ((s_system (S_GETCOOKIE, COOKIE__MCH, (long) &mch) != 0))
		mch = 0;

# ifdef MILAN
	if ((mch != MILAN_C))
# else
	if (((mch != ST) && (mch != STE) && (mch != MEGASTE) && (mch != TT)))
# endif
	{
		/* raven clone has an ST compatible mfp */
		if ((s_system(S_GETCOOKIE, COOKIE_RAVN, (long) &mch) == 0))
		{
			mch = ST;
		}
		else
		{
			c_conws (MSG_MACHINE);
			goto failure;
		}
	}

	init_mfp (mch);
	c_conws ("\r\n");

	for (i = 0; i < IOVAR_MAX && IOVARS (i); i++)
	{
		char name [32];


		ksprintf (name, "u:\\dev\\%s", IOVARS (i)->rsvf_name);

		raw_dev_descriptor.dinfo = i;
		raw_dev_descriptor.tty = &(IOVARS (i)->tty);
		raw_dev_descriptor.bdev = IOVARS (i)->bdev;
		if (d_cntl (DEV_INSTALL2, name, (long) &raw_dev_descriptor) >= 0)
		{
			DEBUG (("%s: %s installed with BIOS remap", __FILE__, name));

			if (add_rsvfentry)
			{
# define RSVF_PORT	0x80
# define RSVF_GEMDOS	0x40
# define RSVF_BIOS	0x20
				add_rsvfentry (IOVARS (i)->rsvf_name, RSVF_PORT | RSVF_GEMDOS | RSVF_BIOS, raw_dev_descriptor.bdev);
			}
		}


		ksprintf (name, "u:\\dev\\%s", IOVARS (i)->tty_name);

		tty_dev_descriptor.dinfo = i + IOVAR_TTY_OFFSET;
		tty_dev_descriptor.tty = &(IOVARS (i)->tty);
		if (d_cntl (DEV_INSTALL, name, (long) &tty_dev_descriptor) >= 0)
		{
			DEBUG (("%s: %s installed", __FILE__, name));

			IOVARS (i + IOVAR_TTY_OFFSET) = IOVARS (i);
		}
	}

	return (DEVDRV *) 1;

failure:
	c_conws (MSG_FAILURE);
	return NULL;
}

/* END initialization - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN interrupt handling - bottom half */

INLINE void
notify_top_half (IOVAR *iovar)
{
	if (!iovar->iointr)
	{
		TIMEOUT *t;

		t = addroottimeout (0L, check_ioevent, 0x1);
		if (t)
		{
			t->arg = (long) iovar;
			iovar->iointr = 1;
		}
	}
}

static void
wr_mfp (IOVAR *iovar, MFP *regs)
{
	DEBUG_I (("wr_mfp: %lx", regs));

	if (!(regs->tsr & TSR_BUF_EMPTY))
		return;

	if (iovar->sendnow)
	{
		regs->udr = iovar->sendnow;
		iovar->sendnow = 0;
	}
	else
	{
		/* is somebody ready to receive data? */
		if (iovar->shake && (iovar->shake & iovar->state))
		{
			DEBUG_I (("wr_mfp: tx disabled - no action"));
		}
		else
		{
			if (iorec_empty (&iovar->output))
			{
				DEBUG_I (("wr_mfp: buffer empty"));

				/* buffer empty */
				iovar->tx_empty = 1;

				// txint_off (iovar, regs);
			}
			else
			{
				/* send character from buffer */
				regs->udr = iorec_get (&iovar->output);
			}
		}
	}
}

static void mfp_dcdint (void) __asm__("mfp_dcdint") __attribute__((used));
static void mfp_dcdint (void)
{
	IOVAR *iovar;
	MFP *regs;

	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
	);

	regs = iovar->regs;
	DEBUG_I (("mfp_dcdint: handling MFP at %lx", regs));

	if (regs->gpip & GPIP_DCD)
	{
		/* DCD is off */

		/* Flankenregister (fallende Flanke) */
		regs->aer &= ~0x2;
	}
	else
	{
		/* DCD is on */

		/* DCD Flanke setzen */
		regs->aer |= 0x2;
	}

	if (!iovar->stintr)
	{
		TIMEOUT *t;

		t = addroottimeout (0L, soft_cdchange, 0x1);
		if (t)
		{
			t->arg = (long) iovar;
			iovar->stintr = 1;
		}
	}

	/* acknowledge */
	regs->isrb = ~0x02;
}


static void mfp_ctsint (void) __asm__("mfp_ctsint") __attribute__((used));
static void mfp_ctsint (void)
{
	IOVAR *iovar;
	MFP *regs;

	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
	);

	regs = iovar->regs;
	DEBUG_I (("mfp_ctsint: handling MFP at %lx", regs));

	if (regs->gpip & GPIP_CTS)
	{
		/* CTS is off */

		/* Flankenregister (fallende Flanke) */
		regs->aer &= ~0x4;

		iovar->state |= STATE_HARD;

		//if (iovar->shake & SHAKE_HARD)
		//	txint_off (iovar->regs);
	}
	else
	{
		/* CTS is on */

		/* CTS Flanke setzen */
		regs->aer |= 0x4;

		iovar->state &= ~STATE_HARD;

		if (iovar->shake & SHAKE_HARD)
			wr_mfp (iovar, iovar->regs);
	}

	/* acknowledge */
	regs->isrb = ~0x04;
}

static void mfp_txerror (void) __asm__("mfp_txerror") __attribute__((used));
static void mfp_txerror (void)
{
	IOVAR *iovar;
	MFP *regs;

	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
	);

	regs = iovar->regs;
	DEBUG_I (("mfp_txerror: handling MFP at %lx", regs));

	(void) regs->tsr;

	/* acknowledge */
	regs->isra = ~0x02;
}

static void mfp_txempty (void) __asm__("mfp_txempty") __attribute__((used));
static void mfp_txempty (void)
{
	IOVAR *iovar;
	MFP *regs;

	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
	);

	regs = iovar->regs;
	DEBUG_I (("mfp_txempty: %lx", regs));


	if (iovar->sendnow)
	{
		regs->udr = iovar->sendnow;
		iovar->sendnow = 0;
	}
	else
	{
		/* is somebody ready to receive data? */
		if (iovar->shake && (iovar->shake & iovar->state))
		{
			DEBUG_I (("mfp_txempty: tx disabled - no action"));
		}
		else
		{
			if (iorec_empty (&iovar->output))
			{
				DEBUG_I (("mfp_txempty: buffer empty"));

				/* buffer empty */
				iovar->tx_empty = 1;
			}
			else
			{
				/* send character from buffer */
				regs->udr = iorec_get (&iovar->output);

				notify_top_half (iovar);
			}
		}
	}

	/* acknowledge */
	regs->isra = ~0x04;
}

INLINE void
mfp_read_x (IOVAR *iovar, MFP *regs)
{
//	do {
		register uchar data = regs->udr & iovar->datmask;

		if (data == XOFF)
		{
			/* otherside can't accept more data */

			DEBUG_I (("mfp_read_x: XOFF"));

			iovar->state |= STATE_SOFT;
			//txint_off (iovar, regs);
		}
		else if (data == XON)
		{
			/* otherside ready now */

			DEBUG_I (("mfp_read_x: XON"));

			iovar->state &= ~STATE_SOFT;
			wr_mfp (iovar, iovar->regs);
		}
		else
		{
			if (!iorec_put (&iovar->input, data))
			{
				DEBUG_I (("mfp_read_x: buffer full!"));
			}
		}
//	}
//	while (regs->rsr & RSR_CHAR_AVAILABLE);
}

INLINE void
mfp_read_o (IOVAR *iovar, MFP *regs)
{
//	do {
		register uchar data = regs->udr & iovar->datmask;

		if (!iorec_put (&iovar->input, data))
		{
			DEBUG_I (("mfp_read_o: buffer full!"));
		}
//	}
//	while (regs->rsr & RSR_CHAR_AVAILABLE);
}


static void mfp_rxavail (void) __asm__("mfp_rxavail") __attribute__((used));
static void mfp_rxavail (void)
{
	IOVAR *iovar;
	MFP *regs;

	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
	);

	regs = iovar->regs;
	DEBUG_I (("mfp_rxavail: handling MFP at %lx", regs));

	if (iovar->shake & SHAKE_SOFT)	mfp_read_x (iovar, regs);
	else				mfp_read_o (iovar, regs);

	/* test the free space
	 */
	if (!iovar->rx_full && (iorec_used (&iovar->input) > iovar->input.high_water))
	{
		DEBUG_I (("mfp_rxavail: stop rx"));

		iovar->rx_full = 1;

		if (iovar->shake & SHAKE_HARD)
		{
			/* set rts off */
			if (!iovar->tt_port)
				rts_off (regs);
		}

		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XOFF */
			iovar->sendnow = XOFF;
			wr_mfp (iovar, regs);
		}
	}

	notify_top_half (iovar);

	/* acknowledge */
	regs->isra = ~0x10;
}

static void mfp_rxerror (void) __asm("mfp_rxerror") __attribute__((used));
static void mfp_rxerror (void)
{
	IOVAR *iovar;
	MFP *regs;
	register char rsr;

	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
	);

	regs = iovar->regs;
	DEBUG_I (("mfp_rxerror: handling MFP at %lx", regs));

	rsr = regs->rsr;
	if (rsr & RSR_OVERRUN_ERR)
	{
		if (iovar->shake & SHAKE_SOFT)	mfp_read_x (iovar, regs);
		else				mfp_read_o (iovar, regs);

		/* test the free space
		 */
		if (!iovar->rx_full && (iorec_used (&iovar->input) > iovar->input.high_water))
		{
			DEBUG_I (("mfp_rxerror: stop rx"));

			iovar->rx_full = 1;

			if (iovar->shake & SHAKE_HARD)
			{
				/* set rts off */
				if (!iovar->tt_port)
					rts_off (regs);
			}

			if (iovar->shake & SHAKE_SOFT)
			{
				/* send XOFF */
				iovar->sendnow = XOFF;
				wr_mfp (iovar, regs);
			}
		}

		notify_top_half (iovar);
	}
	else
		/* skip data */
		(void) regs->udr;

	/* acknowledge */
	regs->isra = ~0x08;
}

/* interrupt wrappers - call the target C routines
 */

/*
 * MFP port
 */
static void mfp_dcdint_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_dcdint\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp)  			/* input registers */
		/* clobbered */
	);
}

static void mfp_ctsint_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_ctsint\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp)  			/* input registers */
		/* clobbered */
	);
}

static void mfp_txerror_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_txerror\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp)  			/* input registers */
		/* clobbered */
	);
}

static void mfp_txempty_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_txempty\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp) 			/* input registers */
		/* clobbered */
	);
}

static void mfp_rxerror_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_rxerror\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp) 			/* input registers */
		/* clobbered */
	);
}

static void mfp_rxavail_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_rxavail\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp)  			/* input registers */
		/* clobbered */
	);
}

#ifndef MILAN
/*
 * TT-MFP port
 */
static void ttmfp_txerror_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_txerror\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp_tt)  			/* input registers */
		/* clobbered */
	);
}

static void ttmfp_txempty_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_txempty\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp_tt)  			/* input registers */
		/* clobbered */
	);
}

static void ttmfp_rxerror_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_rxerror\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp_tt)  			/* input registers */
		/* clobbered */
	);
}

static void ttmfp_rxavail_asm(void)
{
	__asm__ __volatile__
	(
		PUSH_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"move.l  %0,%%a0\n\t"
		"bsr     mfp_rxavail\n\t"
		POP_SP("%%a0-%%a2/%%d0-%%d2", 24)
		"rte"
	: 			/* output register */
	: "m"(iovar_mfp_tt)  			/* input registers */
		/* clobbered */
	);
}

#endif

/* END interrupt handling - bottom half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN interrupt handling - top half */

static void
check_ioevent (PROC *p, long arg)
{
	IOVAR *iovar = (IOVAR *) arg;

	iovar->iointr = 0;

	if (iovar->iosleepers)
	{
		if (iorec_used (&iovar->output) < iovar->output.low_water
			|| !iorec_empty (&iovar->input))
		{
			DEBUG (("mfp.xdd: check_ioevent: waking I/O wait (%lu)", iovar->iosleepers));
			wake (IO_Q, (long) &iovar->tty.state);
		}
	}

	if (iovar->tty.rsel)
		if (!iorec_empty (&iovar->input))
			wakeselect (iovar->tty.rsel);

	if (iovar->tty.wsel)
		if (iorec_used (&iovar->output) < iovar->output.low_water)
			wakeselect (iovar->tty.wsel);
}

static void
soft_cdchange (PROC *p, long arg)
{
	IOVAR *iovar = (IOVAR *) arg;

	iovar->stintr = 0;

	if (!(iovar->clocal) && (iovar->regs->gpip & GPIP_DCD))
	{
		/* CD is off and no local mode */

		if (!(iovar->tty.state & TS_BLIND))
		{
			/* lost carrier */
			iovar->tty.state |= TS_BLIND;

			/* clear break */
			top_brk_off (iovar);

			/* sent SIGHUP */
			if (p && iovar->tty.pgrp)
			{
				DEBUG (("soft_cdchange: SIGHUP -> %i", iovar->tty.pgrp));
				killgroup (iovar->tty.pgrp, SIGHUP, 1);
			}

			/* let reads and writes return */
			wake (IO_Q, (long) &iovar->tty.state);

			DEBUG (("TS_BLIND set, lost carrier (p = %lx)", p));
		}

		return;
	}

	if (iovar->tty.state & TS_BLIND)
	{
		/* got carrier (or entered local mode),
		 * clear TS_BLIND and wake whoever waits for it
		 */
		iovar->tty.state &= ~(TS_BLIND | TS_HOLD);
		wake (IO_Q, (long) &iovar->tty.state);

		DEBUG (("TS_BLIND cleared, clocal = %i", iovar->clocal));
	}
}

/* END interrupt handling - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN control functions - top half */

INLINE void
send_byte (IOVAR *iovar)
{
	if (iovar->regs->tsr & TSR_BUF_EMPTY)
	{
		ushort sr;

		sr = splhigh ();
		wr_mfp (iovar, iovar->regs);
		spl (sr);
	}
}

static long
ctl_TIOCBAUD (IOVAR *iovar, long *buf)
{
	long speed = *buf;

	if (speed == -1)
	{
		/* current baudrate */

		*buf = iovar->baudrate;
	}
	else if (speed == 0)
	{
		/* clear dtr */

		if (!iovar->tt_port)
			top_dtr_off (iovar);
	}
	else
	{
		/* set baudrate to speed, enable dtr */

		BAUDS *table = iovar->table;
		ulong baudrate = 0;
		int i;

		for (i = 0; table [i].baudrate != 0; i++)
		{
			if (speed == table [i].baudrate)
			{
				baudrate = table [i].baudrate;
				break;
			}
		}

		if (baudrate != speed)
		{
			baudrate = table [0].baudrate;

			for (i = 0; table [i].baudrate != 0; i++)
			{
				if (speed > table [i].baudrate)
					baudrate = table [i].baudrate;
			}

			*(long *) buf = baudrate;
			DEBUG (("ctl_TIOCBAUD: EBADARG error -> %li", baudrate));

			return EBADARG;
		}

		if (baudrate != iovar->baudrate)
		{
			MFP *regs = iovar->regs;
			ushort sr;

			sr = splhigh ();

			regs->rsr &= ~RSR_RX_ENAB;
			regs->tsr &= ~TSR_TX_ENAB;

			regs->tcdcr &= 0x70;
			regs->tddr = table [i].timeconst;
			regs->tcdcr |= table [i].mode;

			regs->rsr |= 1;
			regs->tsr |= 1;

			spl (sr);

			iovar->baudrate = baudrate;
		}

		/* always enable dtr */
		if (!iovar->tt_port)
			top_dtr_on (iovar);
	}

	return E_OK;
}

static ushort
ctl_TIOCGFLAGS (IOVAR *iovar)
{
	ushort flags = 0;
	uchar ucr = iovar->regs->ucr;

	switch (ucr & UCR_CHSIZE_MASK)
	{
		case UCR_CHSIZE_8: flags |= TF_8BIT;	break;
		case UCR_CHSIZE_7: flags |= TF_7BIT;	break;
		case UCR_CHSIZE_6: flags |= TF_6BIT;	break;
		case UCR_CHSIZE_5: flags |= TF_5BIT;	break;
	}

	switch (ucr & UCR_MODE_MASK)
	{
		case UCR_SYNC_MODE:
		case UCR_ASYNC_1:  flags |= TF_1STOP;	break;
		case UCR_ASYNC_15: flags |= TF_15STOP;	break;
		case UCR_ASYNC_2:  flags |= TF_2STOP;	break;
	}

	if (ucr & UCR_PARITY_ODD)
	{
		if (ucr & 0x2)	flags |= T_EVENP;
		else		flags |= T_ODDP;
	}

	if (iovar->shake & SHAKE_SOFT)
		flags |= T_TANDEM;

	if (iovar->shake & SHAKE_HARD)
		flags |= T_RTSCTS;

	return flags;
}

static long
ctl_TIOCSFLAGS (IOVAR *iovar, ushort flags)
{
	ushort flag = 0;
	uchar datmask;
	uchar ucr = iovar->regs->ucr;

	ucr &= ~UCR_CHSIZE_MASK;
	switch (flags & TF_CHARBITS)
	{
		case TF_5BIT:	ucr |= UCR_CHSIZE_5; datmask = 0x1f; break;
		case TF_6BIT:	ucr |= UCR_CHSIZE_6; datmask = 0x3f; break;
		case TF_7BIT:	ucr |= UCR_CHSIZE_7; datmask = 0x7f; break;
		case TF_8BIT:	ucr |= UCR_CHSIZE_8; datmask = 0xff; break;
		default:	return EBADARG;
	}

	ucr &= ~UCR_MODE_MASK;
	switch (flags & TF_STOPBITS)
	{
		case TF_1STOP:	ucr |= UCR_ASYNC_1;	break;
		case TF_15STOP:	ucr |= UCR_ASYNC_15;	break;
		case TF_2STOP:	ucr |= UCR_ASYNC_2;	break;
		default:	return EBADARG;
	}

	if (flags & (T_EVENP | T_ODDP))
	{
		if ((flags & (T_EVENP | T_ODDP)) == (T_EVENP | T_ODDP))
			return EBADARG;

		/* enable parity */
		ucr |= UCR_PARITY_ODD;

		/* set even/odd parity */
		if (flags & T_EVENP)	ucr |= 0x2;
		else			ucr &= ~0x2;
	}
	else
	{
		/* disable parity */
		ucr &= ~UCR_PARITY_ODD;

		/* even/odd bit ignored in this case */
	}

	/* setup in register */
	iovar->regs->ucr = ucr;

	/* save for later */
	iovar->datmask = datmask;

	/* soft handshake */
	if (iovar->shake & SHAKE_SOFT)
	{
		if (!(flags & T_TANDEM))
		{
			iovar->shake &= ~SHAKE_SOFT;
			flag = 1;
		}
	}
	else
	{
		if (flags & T_TANDEM)
		{
			iovar->shake |= SHAKE_SOFT;
			iovar->state &= ~STATE_SOFT;
			flag = 1;
		}
	}

	/* hard handshake */
	if (iovar->shake & SHAKE_HARD)
	{
		if (!(flags & T_RTSCTS))
		{
			iovar->shake &= ~SHAKE_HARD;
			flag = 1;
		}
	}
	else
	{
		if (!iovar->tt_port && (flags & T_RTSCTS))
		{
			iovar->shake |= SHAKE_HARD;

			/* update the CTS state bit manually
			 * to be safe, or???
			 *
			if (iovar->regs->msr & CTSI)
				iovar->state &= ~STATE_HARD;
			else
				iovar->state = STATE_HARD;
			 */

			flag = 1;
		}
	}

	/* enable transmitter
	 * otherwise we end in deadlocks because
	 * tx_empty && handshake interaction
	 */
	if (flag)
		send_byte (iovar);

	return E_OK;
}

/* flags - new flags, hi bit set == read only
 * mask  - what flags to change/read
 */
long
ctl_TIOCSFLAGSB (IOVAR *iovar, long flags, long mask)
{
	ulong oflags = ctl_TIOCGFLAGS (iovar);

	DEBUG (("ctl_TIOCSFLAGSB: flags = %lx, mask = %lx, oflags = %lx", flags, mask, oflags));

	/* TF_BRKINT|TF_CAR are only handled if masked */
	if (mask & (TF_BRKINT|TF_CAR))
	{
		if (iovar->brkint)
			oflags |= TF_BRKINT;

		if (!iovar->clocal)
			oflags |= TF_CAR;
	}

	/* read only */
	if (flags < 0)
		return oflags;

	/* clear unused bits */
	flags &= (TF_STOPBITS|TF_CHARBITS|TF_BRKINT|TF_CAR|T_RTSCTS|T_TANDEM|T_EVENP|T_ODDP);

	/* TF_BRKINT|TF_CAR are only handled if masked */
	if (mask & (TF_BRKINT|TF_CAR))
	{
		if (!(mask & TF_CAR))
		{
			flags &= ~TF_CAR;
			flags |= (oflags & TF_CAR);
		}
		else
		{
# ifdef DEV_DEBUG
			ushort old = iovar->clocal;
			iovar->clocal = !(flags & TF_CAR);
			if (old != iovar->clocal)
				DEBUG (("CHANGE iovar->clocal to %i", iovar->clocal));
# else
			iovar->clocal = !(flags & TF_CAR);
# endif

			/* update TS_BLIND */
			soft_cdchange (NULL, (long) iovar);
		}

		if (!(mask & TF_BRKINT))
		{
			flags &= ~TF_BRKINT;
			flags |= (oflags & TF_BRKINT);
		}
		else
			iovar->brkint = flags & TF_BRKINT;
	}
	else
		flags &= ~(TF_BRKINT|TF_CAR);

	if (!((mask & (T_RTSCTS|T_TANDEM)) == (T_RTSCTS|T_TANDEM)))
	{
		flags &= ~(T_RTSCTS|T_TANDEM);
		flags |= (oflags & (T_RTSCTS|T_TANDEM));
	}

	if (!((mask & (T_EVENP|T_ODDP)) == (T_EVENP|T_ODDP)))
	{
		flags &= ~(T_EVENP|T_ODDP);
		flags |= (oflags & (T_EVENP|T_ODDP));
	}

	if (!((mask & TF_STOPBITS) == TF_STOPBITS && (flags & TF_STOPBITS)))
	{
		flags &= ~TF_STOPBITS;
		flags |= (oflags & TF_STOPBITS);
	}

	if (!((mask & TF_CHARBITS) == TF_CHARBITS))
	{
		flags &= ~TF_CHARBITS;
		flags |= (oflags & TF_CHARBITS);
	}

	if (flags != oflags)
	{
		long r;

		r = ctl_TIOCSFLAGS (iovar, flags);
		if (r) ALERT (("mfp.xdd: ctl_TIOCSFLAGSB -> set flags failed!"));

		DEBUG (("ctl_TIOCSFLAGSB: new flags (%lx) set (= %li)", flags, r));
	}

	return flags;
}

/* END control functions - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN start primitives - top half */

static void
rx_start (IOVAR *iovar)
{
	if (iovar->rx_full)
	{
		DEBUG (("rx_start: start receiver"));

		if (iovar->shake & SHAKE_HARD)
		{
			/* set rts on */
			if (!iovar->tt_port)
				top_rts_on (iovar);
		}

		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XON */
			iovar->sendnow = XON;

			send_byte (iovar);
		}

		/* buffer no longer full */
		iovar->rx_full = 0;
	}
	else
	{
		DEBUG (("rx_start: already started"));
	}
}

static void
rx_stop (IOVAR *iovar)
{
	if (!iovar->rx_full)
	{
		DEBUG (("rx_stop: stop receiver"));

		if (iovar->shake & SHAKE_HARD)
		{
			/* drop rts */
			if (!iovar->tt_port)
				top_rts_off (iovar);
		}

		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XON */
			iovar->sendnow = XON;

			send_byte (iovar);
		}

		/* receiver stopped */
		iovar->rx_full = 1;
	}
	else
	{
		DEBUG (("rx_stop: already stopped"));
	}
}

static void
tx_start (IOVAR *iovar)
{
	if (iovar->tx_empty)
	{
		DEBUG (("tx_start: start transmitter"));

		/* buffer contain data */
		iovar->tx_empty = 0;

		send_byte (iovar);
	}
	else
	{
		DEBUG (("tx_start: buffer empty"));
	}
}

static void
tx_stop (IOVAR *iovar)
{
	if (!iovar->tx_empty)
	{
		DEBUG (("tx_stop: stop transmitter"));

		/* transmitter stopped */
		iovar->tx_empty = 1;

		/* disable interrupts */
		//top_txint_off (iovar);
	}
	else
	{
		DEBUG (("tx_start: already stopped"));
	}
}

static void
flush_output (IOVAR *iovar)
{
	IOREC *iorec = &iovar->output;
	ushort sr;

	sr = splhigh ();
	iorec->head = iorec->tail = 0;
	spl (sr);
}

static void
flush_input (IOVAR *iovar)
{
	IOREC *iorec = &iovar->input;
	ushort sr;

	sr = splhigh ();
	iorec->head = iorec->tail = 0;
	spl (sr);
}


/* END start primitives - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN bios emulation - top half */

static long _cdecl
mfp_instat (int dev)
{
	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	IOREC *iorec = &iovar->input;
	long used;

	used = iorec_used (iorec);
	return (used ? -1 : 0);
}

static long _cdecl
mfp_in (int dev)
{
	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	IOREC *iorec = &iovar->input;
	long ret;

	ret = iorec_used (iorec);
	if (!ret)
	{
		while (iorec_empty (iorec))
		{
			/* start receiver */
			rx_start (iovar);

			iovar->iosleepers++;
			sleep (IO_Q, (long) &iovar->tty.state);
			iovar->iosleepers--;
		}
	}
	else
	{
		if (ret < iorec->low_water)
			/* start receiver */
			rx_start (iovar);
	}

	ret = iorec_get (iorec);
	return ret;
}

static long _cdecl
mfp_outstat (int dev)
{
	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	IOREC *iorec = &iovar->output;
	long free;

	free = iorec_free (iorec);
	free -= 4;
	if (free < 0)
		free = 0;

	return (free ? -1 : 0);
}

static long _cdecl
mfp_out (int dev, int c)
{
	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	IOREC *iorec = &iovar->output;

	while (!iorec_put (iorec, c))
	{
		/* start transmitter */
		tx_start (iovar);

		iovar->iosleepers++;
		sleep (IO_Q, (long) &iovar->tty.state);
		iovar->iosleepers--;
	}

	/* start transmitter */
	tx_start (iovar);

	return E_OK;
}

/*
 * rsconf modes
 *
 * speed:
 *
 * -2 - return current baudrate
 * -1 - no change
 *  0 - 19200
 *  1 - 9600
 *  2 - 4800
 *  3 - 3600
 *  4 - 2400
 *  5 - 2000
 *  6 - 1800
 *  7 - 1200
 *  8 - 600
 *  9 - 300
 * 10 - 200	-> 230400	(HSMODEM)
 * 11 - 150	-> 115200	(HSMODEM)
 * 12 - 134	-> 57600	(HSMODEM)
 * 13 - 110	-> 38400	(HSMODEM)
 * 14 - 75	-> 153600	(HSMODEM)
 * 15 - 50	-> 76800	(HSMODEM)
 *
 * flowctl:
 *
 * -1 - no change
 *  0 - no handshake
 *  1 - soft (XON/XOFF)
 *  2 - hard (RTS/CTS)
 *  3 - hard & soft
 *
 * ucr:
 * bit 5..6: wordlength: 00: 8, 01: 7, 10: 6, 11: 5
 * bit 3..4: stopbits: 01: 1, 10: 1.5, 11: 2, 00: invalid
 * bit 2   : 0: parity off  1: parity on
 * bit 1   : 0: odd parity, 1: even parity - only valid if parity is enabled
 *
 * rsr: - not used, 0
 * tsr: - bit 3 break on/off
 * scr: - not used, 0
 */

static long _cdecl
mfp_rsconf (int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr)
{
	static ulong baud [16] =
	{
		19200, 9600, 4800, 3600, 2400, 2000, 1800, 1200,
		600, 300, 230400, 115200, 57600, 38400, 153600, 76800
	};

	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	ushort modified = 0;
	ushort flags;
	ulong ret = 0;

	DEBUG (("mfp_rsconf: enter %i, %i", dev, speed));

	if (speed != -1)
	{
		if (speed == -2)
			return iovar->baudrate;

		if ((ushort) speed < 16)
		{
			long baudrate = baud [(ushort) speed];

			ctl_TIOCBAUD (iovar, &baudrate);
		}
	}

	/* get the current flags */
	flags = ctl_TIOCGFLAGS (iovar);

	if (flowctl != -1)
	{
		switch (flowctl)
		{
			case 0:
			{
				flags &= ~T_TANDEM;
				flags &= ~T_RTSCTS;
				break;
			}
			case 1:
			{
				flags |= T_TANDEM;
				flags &= ~T_RTSCTS;
				break;
			}
			case 2:
			{
				flags &= ~T_TANDEM;
				flags |= T_RTSCTS;
				break;
			}
			case 3:
			{
				flags |= T_TANDEM;
				flags |= T_RTSCTS;
				break;
			}
		}

		modified = 1;
	}

	if (ucr != -1)
	{
		flags &= ~TF_CHARBITS;
		flags |= (ucr & 0x60) >> 3;

		flags &= ~TF_STOPBITS;
		flags |= (ucr & 0x18) >> 3;

		if ((flags & TF_STOPBITS) == TF_15STOP)
		{
			flags &= ~TF_STOPBITS;
			flags |= TF_1STOP;
		}

		flags &= ~(T_EVENP | T_ODDP);
		if (ucr & UCR_PARITY_ODD)
		{
			if (ucr & 0x2)
				flags |= T_EVENP;
			else
				flags |= T_ODDP;
		}

		modified = 1;
	}

	if (modified)
	{
		/* set the new flags */
		ctl_TIOCSFLAGS (iovar, flags);
	}

	if (tsr != -1)
	{
		if (tsr & 0x08)
			top_brk_on (iovar);
		else
			top_brk_off (iovar);
	}


	/* now construct the return value
	 */

	/* get the current flags */
	flags = ctl_TIOCGFLAGS (iovar);

	/* ucr */
	ret |= ((((long) (flags & TF_CHARBITS)) << 3) << 24);
	ret |= ((((long) (flags & TF_STOPBITS)) << 3) << 24);

	if (flags & (T_EVENP | T_ODDP))
	{
		ret |= (0x4L << 24);
		if (flags & T_EVENP)
			ret |= (0x2L << 24);
	}

	/* tsr */
	if (iovar->regs->tsr & TSR_SEND_BREAK)
		ret |= (0x8L << 8);

	return ret;
}

/* END bios emulation - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver routines - top half */

static long _cdecl
mfp_open (FILEPTR *f)
{
	ushort dev = f->fc.aux;
	IOVAR *iovar;

	DEBUG (("mfp_open [%i]: enter (%lx)", f->fc.aux, f->flags));

	if (dev > IOVAR_MAX)
		return EACCES;

	iovar = IOVARS (dev);
	if (!iovar)
		return EACCES;

	if (!iovar->open)
	{
		/* first open */

		/* assign dtr line */
		if (!iovar->tt_port)
			top_dtr_on (iovar);

		/* start receiver */
		rx_start (iovar);
	}
	else
	{
		if (denyshare (iovar->open, f))
		{
			DEBUG (("mfp_open: file sharing denied"));
			return EACCES;
		}
	}

	f->pos = 0;
	f->next = iovar->open;
	iovar->open = f;

	if (dev >= IOVAR_TTY_OFFSET)
		f->flags |= O_TTY;
	else
		/* force nonblocking on HSMODEM devices */
		f->flags |= O_NDELAY;

	DEBUG (("mfp_open: return E_OK (added %lx)", f));
	return E_OK;
}

static long _cdecl
mfp_close (FILEPTR *f, int pid)
{
	IOVAR *iovar = IOVARS (f->fc.aux);

	DEBUG (("mfp_close [%i]: enter", f->fc.aux));

	if ((f->flags & O_LOCK)
	    && ((iovar->lockpid == pid) || (f->links <= 0)))
	{
		DEBUG (("mfp_close: remove lock by %i", pid));

		f->flags &= ~O_LOCK;
		iovar->lockpid = 0;

		/* wake anyone waiting for this lock */
		wake (IO_Q, (long) iovar);
	}

	if (f->links <= 0)
	{
		register FILEPTR **temp;
		register long flag = 1;

		DEBUG (("mfp_close: freeing FILEPTR %lx", f));

		/* remove the FILEPTR from the linked list */
		temp = &iovar->open;
		while (*temp)
		{
			if (*temp == f)
			{
				*temp = f->next;
				f->next = NULL;
				flag = 0;

				break;
			}

			temp = &(*temp)->next;
		}

		if (flag)
			ALERT (("mfp_close: remove open FILEPTR fail!", f->fc.aux));

		if (!(iovar->open))
		{
			/* stop receiver */
			rx_stop (iovar);

			/* clear dtr line */
			if (!iovar->tt_port)
				top_dtr_off (iovar);

			if (iovar->clocal)
			{
				/* clear local carrier */
				iovar->clocal = 0;

				/* update TS_BLIND */
				soft_cdchange (NULL, (long) iovar);
			}

			/* always flush receiver buffer, nobody can read */
			flush_input (iovar);

			if (iovar->tty.state & TS_BLIND)
				/* if blind flush transmitter buffer */
				flush_output (iovar);
		}
	}

	DEBUG (("mfp_close: leave ok"));
	return E_OK;
}

/* HSMODEM write/read routines
 *
 * they never block
 * they don't do any carrier check or so
 */
static long _cdecl
mfp_write (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	long done = 0;

	DEBUG (("mfp_write [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));

	/* copy as much as possible */
	while ((bytes > 0) && iorec_put (iorec, *buf))
	{
		buf++; done++;
		bytes--;
	}

	/* start transmitter */
	tx_start (iovar);

	if (!done && (f->flags & O_TTY))
	{
		/* return EWOULDBLOCK only for
		 * non HSMODEM devices
		 */
		done = EWOULDBLOCK;
	}

	DEBUG (("mfp_write: leave (%ld)", done));
	return done;
}

static long _cdecl
mfp_read (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	long done = 0;

	DEBUG (("mfp_read [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));

	/* copy as much as possible */
	while ((bytes > 0) && !iorec_empty (iorec))
	{
		*buf = iorec_get (iorec);

		buf++; done++;
		bytes--;
	}

	if (iorec_used (iorec) < iorec->low_water)
		/* start receiver */
		rx_start (iovar);

	if (!done && (f->flags & O_TTY))
	{
		/* return EWOULDBLOCK only for
		 * non HSMODEM devices
		 */
		done = EWOULDBLOCK;
	}

	DEBUG (("mfp_read: leave (%ld)", done));
	return done;
}


/* terminal fast raw I/O routines
 *
 * they must implement all the nice tty features
 * like O_NDELAY handling, vmin/vtime support and so on
 *
 * they are called in non-canonical mode
 */
static long _cdecl
mfp_writeb (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;

	DEBUG (("mfp_writeb [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));

	if (bytes)
	do {
		long free;

		if (iovar->tty.state & TS_BLIND)
			/* line disconnected */
			break;

		free = iorec_free (iorec);
		if ((free < 2L)
 			|| (!ndelay && (free < bytes) && (iorec_used (iorec) > iorec->high_water)))
		{
			if (ndelay)
				break;

			DEBUG (("mfp_writeb: sleeping I/O wait"));

			/* start transmitter */
			tx_start (iovar);

			iovar->iosleepers++;
			sleep (IO_Q, (long) &iovar->tty.state);
			iovar->iosleepers--;

			/* restart */
			continue;
		}

		/* copy as much as possible */
		while ((bytes > 0) && iorec_put (iorec, *buf))
		{
			buf++; done++;
			bytes--;
		}

		/* start transmitter */
		tx_start (iovar);
	}
	while (bytes && !ndelay);

	if (ndelay && !done && (f->flags & O_TTY))
	{
		/* return EWOULDBLOCK only for
		 * non HSMODEM devices
		 */
		done = EWOULDBLOCK;
	}

	DEBUG (("mfp_writeb: leave (%ld)", done));
	return done;
}

static void
timerwake (PROC *p, long cond)
{
	wake (IO_Q, cond);
}

static long _cdecl
mfp_readb (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	int ndelay = (f->flags & O_NDELAY) || iovar->tty.vtime /* ??? */;
	long done = 0;

	DEBUG (("mfp_readb [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));

	if (!bytes)
		/* nothing to do... */
		return 0;

	if (iovar->tty.state & TS_BLIND)
		/* line disconnected */
		return 0;

	if (!ndelay)
	{
		/* blocking read
		 * handle all vmin and vtime combinations
		 */

		if (iovar->tty.vtime == 0)
		{
			/* case D: vmin = 0, vtime = 0
			 * -> doesn't enter while() and proceed as it's
			 *    expected
			 *
			 * case B: vmin > 0, vtime = 0
			 * -> enter while() and block until vmin is ready
			 *    (no timer, wait until vmin chars received)
			 *
			 * BUG: if vmin > bufsize state is undefined
			 */

			while (!(iovar->tty.state & TS_BLIND)
				&& (iorec_used (iorec) < (long) iovar->tty.vmin))
			{
				/* start receiver */
				rx_start (iovar);

				iovar->iosleepers++;
				sleep (IO_Q, (long) &iovar->tty.state);
				iovar->iosleepers--;
			}
		}
		else if (iovar->tty.vmin == 0)
		{
			/* case C: vmin = 0, vtime > 0
			 * -> vtime is a absolut read timer
			 */
			if (iorec_empty (iorec))
			{
				TIMEOUT *t;

				/* start receiver */
				rx_start (iovar);

				t = addtimeout (iovar->tty.vtime * 100, timerwake);
				if (!t) return ENOMEM;

				t->arg = (long) &iovar->tty.state;

				iovar->iosleepers++;
				sleep (IO_Q, t->arg);
				iovar->iosleepers--;

				canceltimeout (t);
			}
		}
		else
		{
			/* case A: vmin > 0, vtime > 0
			 * -> vtime is an inter byte timer
			 */
			while (!(iovar->tty.state & TS_BLIND)
				&& (iorec_used (iorec) < (long) iovar->tty.vmin))
			{
				TIMEOUT *t;

				/* start receiver */
				rx_start (iovar);

				t = addtimeout (iovar->tty.vtime * 100, timerwake);
				if (!t) return ENOMEM;

				t->arg = (long) &iovar->tty.state;

				iovar->iosleepers++;
				sleep (IO_Q, t->arg);
				iovar->iosleepers--;

				canceltimeout (t);
			}
		}
	}

	/* copy as much as possible */
	while ((bytes > 0) && !iorec_empty (iorec))
	{
		*buf = iorec_get (iorec);

		buf++; done++;
		bytes--;
	}

	if (iorec_used (iorec) < iorec->low_water)
		/* start receiver */
		rx_start (iovar);

	DEBUG (("mfp_readb: leave (%ld)", done));
	return done;
}


/* slow terminal I/O routines
 *
 * they must implement only N_DELAY correctly
 *
 * they are called in canonical mode and only from kernel level
 * with kernel buffers
 *
 * Note: buf doesn't point to a byte stream; it's rather a int32 array
 *       with one 32bit int for every char; bytes is the array size in _bytes_
 */
static long _cdecl
mfp_twrite (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	const long *r = (const long *) buf;

	DEBUG (("mfp_twrite [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));

	if (bytes)
	do {
		/* copy as much as possible */
		while ((bytes > 0) && iorec_put (iorec, *r++))
		{
			done += 4;
			bytes -= 4;
		}

		if (bytes && !ndelay)
		{
			DEBUG (("mfp_twrite: sleeping I/O wait"));

			/* start transmitter */
			tx_start (iovar);

			iovar->iosleepers++;
			sleep (IO_Q, (long) &iovar->tty.state);
			iovar->iosleepers--;
		}

		/* start transmitter */
		tx_start (iovar);
	}
	while (bytes && !ndelay);

	if (ndelay && !done)
		done = EWOULDBLOCK;

	DEBUG (("mfp_twrite: leave (%ld)", done));
	return done;
}

static long _cdecl
mfp_tread (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	long *r = (long *) buf;

	DEBUG (("mfp_tread [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));

	if (bytes)
	do {
		/* copy as much as possible */
		while ((bytes > 0) && !iorec_empty (iorec))
		{
			*r = (long) iorec_get (iorec);

			r++; done += 4;
			bytes -= 4;
		}

		if (iorec_used (iorec) < iorec->low_water)
			/* start receiver */
			rx_start (iovar);

		if (bytes && !ndelay)
		{
			DEBUG (("mfp_tread: sleeping I/O wait"));

			iovar->iosleepers++;
			sleep (IO_Q, (long) &iovar->tty.state);
			iovar->iosleepers--;
		}
	}
	while (bytes && !ndelay);

	if (ndelay && !done)
		done = EWOULDBLOCK;

	DEBUG (("mfp_tread: leave (%ld)", done));
	return done;
}

static long _cdecl
mfp_lseek (FILEPTR *f, long where, int whence)
{
	DEBUG (("mfp_lseek [%i]: enter (%ld, %d)", f->fc.aux, where, whence));

	/* (terminal) devices are always at position 0 */
	return 0;
}

static long _cdecl
mfp_ioctl (FILEPTR *f, int mode, void *buf)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	long r = E_OK;

	DEBUG (("mfp_ioctl [%i]: (%x, (%c %i), %lx)", f->fc.aux, mode, (char) (mode >> 8), (mode & 0xff), buf));

	switch (mode)
	{
		case FIONREAD:
		{
			*(long *) buf = iorec_used (&iovar->input);
			break;
		}
		case FIONWRITE:
		{
			long bytes;

			bytes = iorec_free (&iovar->output);
			bytes -= 4;
			if (bytes < 0)
				bytes = 0;

			*(long *) buf = bytes;
			break;
		}
		case FIOEXCEPT:		/* anywhere documented? */
		{
			*(long *) buf = 0;
			break;
		}
		case TIOCFLUSH:
		{
			long flushtype;

			/* HSMODEM put the value itself in arg
			 * MiNT use arg as a pointer to the value
			 *
			 * if opened as terminal we assume MiNT convention
			 * otherwise HSMODEM convention
			 *
			 * ok, or?
			 */
			if (f->fc.aux >= IOVAR_TTY_OFFSET)
			{
				long *ptr = buf;
				if ((!ptr) || (!(*ptr & 3)))
					flushtype = 3;
				else
					flushtype = (int) *ptr;
			}
			else
				flushtype = (long) buf;

			if (flushtype <= 0)
			{
				r = ENOSYS;
				break;
			}

			if (flushtype & 1)
				flush_input (iovar);

			if (flushtype & 2)
				flush_output (iovar);

			break;
		}
		case TIOCSTOP:		/* HSMODEM */
		{
			tx_stop (iovar);	/* stop transmitter */
			rx_stop (iovar);	/* stop receiver */
			break;
		}
		case TIOCSTART:		/* HSMODEM */
		{
			tx_start (iovar);	/* start transmitter */
			rx_start (iovar);	/* start receiver */
			break;
		}
		case TIOCIBAUD:
		case TIOCOBAUD:
		{
			DEBUG (("mfp_ioctl(TIOCIBAUD) to %li", *(long *) buf));

			r = ctl_TIOCBAUD (iovar, (long *) buf);
			break;
		}
		case TIOCCBRK:
		{
			top_brk_off (iovar);
			break;
		}
		case TIOCSBRK:
		{
			top_brk_on (iovar);
			break;
		}
		case TIOCGFLAGS:
		{
			DEBUG (("mfp_ioctl(TIOCGFLAGS) [%lx]", (ushort *) buf));

			*(ushort *) buf = ctl_TIOCGFLAGS (iovar);
			break;
		}
		case TIOCSFLAGS:
		{
			DEBUG (("mfp_ioctl(TIOCSFLAGS) -> %lx", *(ushort *) buf));

			r = ctl_TIOCSFLAGS (iovar, *(ushort *) buf);
			break;
		}
		case TIONOTSEND:
		case TIOCOUTQ:
		{
			DEBUG (("mfp_ioctl(TIOCOUTQ) -> %li", iorec_used (&iovar->output)));

			*(long *) buf = iorec_used (&iovar->output);
			break;
		}
		case TIOCSFLAGSB:
		{
			DEBUG (("mfp_ioctl(TIOCSFLAGSB) -> %lx, %lx", ((long *) buf)[0],
									((long *) buf)[1]));

			*(long *) buf = ctl_TIOCSFLAGSB (iovar, ((long *) buf)[0],
								((long *) buf)[1]);
			break;
		}
		case TIOCGVMIN:
		{
			struct tty *tty = (struct tty *) f->devinfo;
			ushort *v = buf;

			v [0] = tty->vmin;
			v [1] = tty->vtime;

			break;
		}
		case TIOCSVMIN:
		{
			ushort *v = buf;

			if (v[0] > iorec_size (&iovar->input) / 2)
				v[0] = iorec_size (&iovar->input) / 2;

			iovar->tty.vmin = v[0];
			iovar->tty.vtime = v[1];

			break;
		}
		case TIOCWONLINE:
		{
			while (iovar->tty.state & TS_BLIND)
				sleep (IO_Q, (long) &iovar->tty.state);

			break;
		}
		case TIOCBUFFER:	/* HSMODEM special */
		{
			long *ptr = (long *) buf;

			/* input buffer size */
			if (*ptr == -1)	*ptr = iorec_size (&iovar->input);
			else		*ptr = -1;

			ptr++;

			/* input low watermark */
			if (*ptr == -1)	*ptr = iovar->input.low_water;
			else		*ptr = -1;

			ptr++;

			/* input high watermark */
			if (*ptr == -1)	*ptr = iovar->input.high_water;
			else		*ptr = -1;

			ptr++;

			/* output buffer size */
			if (*ptr == -1)	*ptr = iorec_size (&iovar->output);
			else		*ptr = -1;

			break;
		}
		case TIOCCTLMAP:	/* HSMODEM */
		{
			long *ptr = (long *) buf;

			ptr [0] = 0
			/*	| TIOCMH_LE */
			/*	| TIOCMH_DTR */
			/*	| TIOCMH_RTS */
			/*	| TIOCMH_CTS */
			/*	| TIOCMH_CD */
			/*	| TIOCMH_RI  */
# ifdef MILAN
				| TIOCMH_DSR
# endif
			/*	| TIOCMH_LEI */
			/*	| TIOCMH_TXD */
			/*	| TIOCMH_RXD */
				| TIOCMH_BRK
			/*	| TIOCMH_TER */
			/*	| TIOCMH_RER */
				| TIOCMH_TBE
				| TIOCMH_RBF
				;

			if (!iovar->tt_port)
				ptr [0] |= 0
					| TIOCMH_DTR
					| TIOCMH_RTS
					| TIOCMH_CTS
					| TIOCMH_CD
					| TIOCMH_RI;

			ptr [1] = 0;	/* will be never supported */
			ptr [2] = 0;	/* will be never supported */
			ptr [3] = 0;	/* reserved */
			ptr [4] = 0;	/* reserved */
			ptr [5] = 0;	/* reserved */

			break;
		}
		case TIOCCTLGET:	/* HSMODEM */
		{
			/* register long mask = *(long *) buf; */
			register long val = 0;
			register uchar reg;

			/* TIOCMH_LE */

# ifndef MILAN
			if (!iovar->tt_port)
			{
				*_giselect = 14;
				reg = *_giread;

				if (!(reg & GI_DTR)) val |= TIOCMH_DTR;
				if (!(reg & GI_RTS)) val |= TIOCMH_RTS;
			}
# endif

			reg = iovar->regs->gpip;
# ifdef MILAN
			if (!(reg & GPIP_DTR)) val |= TIOCMH_DTR;
			if (!(reg & GPIP_RTS)) val |= TIOCMH_RTS;
			if (!(reg & GPIP_DSR)) val |= TIOCMH_DSR;
# endif
			if (!iovar->tt_port)
			{
				if (!(reg & GPIP_CTS)) val |= TIOCMH_CTS;
				if (!(reg & GPIP_DCD)) val |= TIOCMH_CD;
				if (!(reg & GPIP_RI )) val |= TIOCMH_RI;
			}

			/* TIOCMH_LEI */
			/* TIOCMH_TXD */
			/* TIOCMH_RXD */

			if (iovar->regs->rsr & RSR_BREAK_DETECT)
				val |= TIOCMH_BRK;

			/* TIOCMH_TER */
			/* TIOCMH_RER */

			if (iovar->regs->tsr & TSR_BUF_EMPTY)
				val |= TIOCMH_TBE;

			if (iovar->regs->rsr & RSR_CHAR_AVAILABLE)
				val |= TIOCMH_RBF;

			*(long *) buf = val;
			break;
		}
		case TIOCCTLSET:	/* HSMODEM */
		{
			long *arg = (long *) buf;

			register long mask = *arg++;
			register long val = *arg;

			if (!iovar->tt_port &&
			    (mask & (TIOCMH_DTR | TIOCMH_RTS)))
			{
				if (mask & TIOCMH_DTR)
				{
					if (val & TIOCMH_DTR)	top_dtr_on (iovar);
					else			top_dtr_off (iovar);
				}

				if (mask & TIOCMH_RTS)
				{
					if (val & TIOCMH_RTS)	top_rts_on (iovar);
					else			top_rts_off (iovar);
				}
			}

			break;
		}
		case TIOCERROR:		/* HSMODEM */
		{
			r = EBADARG;
			break;
		}
		case TIOCMGET:
		{
			register long val = 0;
			register uchar reg;

			/* TIOCM_LE */
# ifndef MILAN
			if (!iovar->tt_port)
			{
				*_giselect = 14;
				reg = *_giread;

				if (!(reg & GI_DTR)) val |= TIOCM_DTR;
				if (!(reg & GI_RTS)) val |= TIOCM_RTS;
			}
# endif

			reg = iovar->regs->gpip;
# ifdef MILAN
			if (!(reg & GPIP_DTR)) val |= TIOCM_DTR;
			if (!(reg & GPIP_RTS)) val |= TIOCM_RTS;
			if (!(reg & GPIP_DSR)) val |= TIOCM_DSR;
# endif
			/* TIOCM_ST */
			/* TIOCM_SR */

			if (!iovar->tt_port)
			{
				if (!(reg & GPIP_CTS)) val |= TIOCM_CTS;
				if (!(reg & GPIP_DCD)) val |= TIOCM_CD;
				if (!(reg & GPIP_RI )) val |= TIOCM_RI;
			}

			*(long *) buf = val;
			break;
		}
		case TIOCCDTR:
		{
			if (!iovar->tt_port)
				top_dtr_off (iovar);
			else
				r = EINVAL;
			break;
		}
		case TIOCSDTR:
		{
			if (!iovar->tt_port)
				top_dtr_on (iovar);
			else
				r = EINVAL;
			break;
		}

		case F_SETLK:
		case F_SETLKW:
		{
			struct flock *lck = (struct flock *) buf;
			int cpid = p_getpid ();

			while (iovar->lockpid && iovar->lockpid != cpid)
			{
				if (mode == F_SETLKW && lck->l_type != F_UNLCK)
				{
					DEBUG (("mfp_ioctl: sleep in SETLKW"));
					sleep (IO_Q, (long) iovar);
				}
				else
					return ELOCKED;
			}

			if (lck->l_type == F_UNLCK)
			{
				if (!(f->flags & O_LOCK))
				{
					DEBUG (("mfp_ioctl: wrong file descriptor for UNLCK"));
					return ENSLOCK;
				}

				if (iovar->lockpid != cpid)
					return ENSLOCK;

				iovar->lockpid = 0;
				f->flags &= ~O_LOCK;

				/* wake anyone waiting for this lock */
				wake (IO_Q, (long) iovar);
			}
			else
			{
				iovar->lockpid = cpid;
				f->flags |= O_LOCK;
			}

			break;
		}
		case F_GETLK:
		{
			struct flock *lck = (struct flock *) buf;

			if (iovar->lockpid)
			{
				lck->l_type = F_WRLCK;
				lck->l_start = lck->l_len = 0;
				lck->l_pid = iovar->lockpid;
			}
			else
			{
				lck->l_type = F_UNLCK;
			}

			break;
		}

		default:
		{
			/* Fcntl will automatically call tty_ioctl to handle
			 * terminal calls that we didn't deal with
			 */
			r = ENOSYS;
			break;
		}
	}

	DEBUG (("mfp_ioctl: return %li", r));
	return r;
}

static long _cdecl
mfp_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	DEBUG (("mfp_datime [%i]: enter (%i)", f->fc.aux, rwflag));

	if (rwflag)
		return EACCES;

	*timeptr++ = timestamp;
	*timeptr = datestamp;

	return E_OK;
}

static long _cdecl
mfp_select (FILEPTR *f, long proc, int mode)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	struct tty *tty = (struct tty *) f->devinfo;

	DEBUG (("mfp_select [%i]: enter (%li, %i, %lx)", f->fc.aux, proc, mode, tty));

	if (mode == O_RDONLY)
	{
		if (!iorec_empty (&iovar->input))
		{
			TRACE (("mfp_select: data present for device %lx", iovar));
			return 1;
		}

		if (tty)
		{
			/* avoid collisions with other processes
			 */

			if (tty->rsel)
				/* collision */
				return 2;

			tty->rsel = proc;
		}

		return 0;
	}
	else if (mode == O_WRONLY)
	{
		if ((!tty || !(tty->state & (TS_BLIND | TS_HOLD))) && !iorec_empty (&iovar->output))
		{
			TRACE (("mfp_select: ready to output on %lx", iovar));
			return 1;
		}

		if (tty)
		{
			/* avoid collisions with other processes
			 */

			if (tty->wsel)
				/* collision */
				return 2;

			tty->wsel = proc;
		}

		return 0;
	}

	/* default -- we don't know this mode, return 0 */
	return 0;
}

static void _cdecl
mfp_unselect (FILEPTR *f, long proc, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;

	DEBUG (("mfp_unselect [%i]: enter (%li, %i, %lx)", f->fc.aux, proc, mode, tty));

	if (tty)
	{
		if (mode == O_RDONLY && tty->rsel == proc)
			tty->rsel = 0;
		else if (mode == O_WRONLY && tty->wsel == proc)
			tty->wsel = 0;
	}
}

/* END device driver routines - top half */
/****************************************************************************/
