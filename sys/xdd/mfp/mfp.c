/*
 * Filename:     
 * Version:      
 * Author:       Frank Naumann
 * Started:      2000-01-03
 * Last Updated: 2000-02-28
 * Target O/S:   TOS/MiNT
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 *               Portions copyright 1998, 1999 Rainer Mannigel.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

# define __KERNEL_XDD__

# include <mint/mint.h>
# include <mint/dcntl.h>
# include <mint/file.h>
# include <mint/proc.h>
# include <mint/signal.h>
# include <libkern/libkern.h>

# include <osbind.h>
# include "mfp.h"


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	12
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

# ifndef TT_MFP
# define RSVF_MFP	"modem1"
# else
# define RSVF_MFP	"serial1"
# endif

# define TTY_BASENAME	"ttyS"

# define BDEV_START	6
# ifndef TT_MFP
# define BDEV_OFFSET	6
# else
# define BDEV_OFFSET	8
# endif


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

# ifdef MILAN
# define MSG_BOOT	\
	"\033p Milan MFP serial driver version " MSG_VERSION " \033q\r\n"
# else
# define MSG_BOOT	\
	"\033p ST/TT MFP serial driver version " MSG_VERSION " \033q\r\n"
# endif

# define MSG_GREET	\
	"½ 1998, 1999 by Rainer Mannigel.\r\n" \
	"½ " MSG_BUILDDATE " by Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# ifdef MILAN
# define MSG_MACHINE	\
	"\033pThis driver require a Milan!\033q\r\n"
# else
# define MSG_MACHINE	\
	"\033pThis driver require a ST/TT compatible MFP!\033q\r\n"
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
	IOVAR	*next;		/* for Interrupt-Sharing */
	
	ushort	state;		/* receiver state */
# define STATE_HARD		0x1		/* CTS is low */
# define STATE_SOFT		0x2		/* XOFF is set */
	
	ushort	shake;		/* handshake mode */
# define SHAKE_HARD		STATE_HARD	/* RTS/CTS handshake enabled */
# define SHAKE_SOFT		STATE_SOFT	/* XON/XOFF handshake enabled */
	
	uchar	rx_full;	/* input buffer full */
	uchar	tx_empty;	/* output buffer empty */
	
	uchar	sendnow;	/* one byte of control data, bypasses buffer */
	uchar	datmask;	/* AND mask for read from sccdat */
	
	IOREC	input;		/* input buffer */
	IOREC	output;		/* output buffer */
	
	long	baudrate;	/* current baud rate value */
	
	ushort	bdev;		/* BIOS devnumber */
	ushort	lockpid;	/* file locking */
	
	FILEPTR	*open;		/* open FILEPTRs */
	TTY	tty;		/* the tty descriptor */
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
 * locking functions - bottom/top half
 */
INLINE ushort	splhigh		(void);
INLINE void	splx		(ushort old_sr);


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
INLINE int	init_MFP	(IOVAR **iovar, MFP *regs);
INLINE void	init_mfp	(void);
DEVDRV *	_cdecl init	(struct kerinfo *k);


/*
 * interrupt handling - bottom half
 */
INLINE void	notify_top_half	(void);
static void	wr_mfp		(IOVAR *iovar, MFP *regs);
static void	mfp_txempty	(void);
static void	mfp_rxavail	(void);
static void	mfp_dcdint	(void);
static void	mfp_ctsint	(void);
static void	mfp_rxerror	(void);

static void	mfp_intrwrap	(void);
       void	mfp1_txempty	(void);
       void	mfp1_rxavail	(void);
       void	mfp1_dcdint	(void);
       void	mfp1_ctsint	(void);
       void	mfp1_rxerror	(void);


/*
 * interrupt handling - top half
 */
static void	check_event	(void);
static void	soft_dcdchange	(void);


/*
 * control functions - top half
 */
INLINE void	send_byte	(IOVAR *iovar);
static long	ctl_TIOCBAUD	(IOVAR *iovar, long *buf);
static void	ctl_TIOCGFLAGS	(IOVAR *iovar, ushort *buf);
static long	ctl_TIOCSFLAGS	(IOVAR *iovar, ushort flags);


/*
 * start primitives - top half
 */
static void	rx_start	(IOVAR *iovar);
static void	tx_start	(IOVAR *iovar);


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
static long _cdecl	mfp_writeb	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	mfp_readb	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	mfp_twrite	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	mfp_tread	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	mfp_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl	mfp_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl	mfp_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	mfp_close	(FILEPTR *f, int pid);
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

static DEVDRV raw_devtab =
{
	mfp_open,
	mfp_writeb, mfp_readb, mfp_lseek, mfp_ioctl, mfp_datime,
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
	register ushort i;
	
	i = inc_ptr (iorec->head);
	iorec->head = i;
	
	return iorec->buffer[i];
}

INLINE int
iorec_put (IOREC *iorec, uchar data)
{
	register ushort i = inc_ptr (iorec->tail);
	
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
	register long tmp;
	
	tmp = iorec->tail;
	tmp -= iorec->head;
	
	if (tmp < 0)
		tmp += IOBUFSIZE;
	
	return tmp;
}

INLINE long
iorec_free (IOREC *iorec)
{
	register long tmp;
	
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

static IOVAR *iovar_mfp;


static IOVAR *iovar_tab [2];

# define IOVARS(nr)		(iovar_tab [nr])
# define IOVAR_TTY_OFFSET	(1)
# define IOVAR_MAX		(1)

/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN locking functions - bottom/top half */

INLINE ushort
splhigh (void)
{
	ushort old_sr;
	
	__asm__ volatile
	(
		"move.w %%sr,%0;"
		"ori #0x0700,%%sr"
		: "=d" (old_sr)			/* output register */
		: 				/* input registers */
		: "cc"				/* clobbered */
	);
	
	return old_sr;
}

INLINE void
splx (ushort old_sr)
{
	__asm__ volatile
	(
		"move.w %0,%%sr"
		: 				/* output register */
		: "d" (old_sr)			/* input registers */
		: "cc"				/* clobbered */
	);
}

/* END locking functions - bottom/top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN inline assembler primitives - bottom half */

INLINE void
rts_on (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip &= ~0x01; */	/* RTS (bit0) = 0 */
	asm volatile
	(
		"bclr.b #0,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread & 0xf7;
# endif
	DEBUG_I (("MFP: RTS on"));
}

INLINE void
rts_off (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip |= 0x01; */	/* RTS (bit0) = 1 */
	asm volatile
	(
		"bset.b #0,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread | 0x08;
# endif
	DEBUG_I (("MFP: RTS off"));
}


INLINE void
dtr_on (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip &= ~0x8; */
	asm volatile
	(
		"bclr.b #3,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread & 0xef;
# endif
	DEBUG_I (("MFP: DTR on"));
}

INLINE void
dtr_off (MFP *regs)
{
# ifdef MILAN
	/* regs->gpip |= 0x8; */
	asm volatile
	(
		"bset.b #3,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
# else
	*_giselect = 14;
	*_giwrite = *_giread | 0x10;
# endif
	DEBUG_I (("MFP: DTR off"));
}


INLINE void
brk_on (MFP *regs)
{
	regs->tsr |= 0x8;
	DEBUG_I (("MFP BRK on"));
}

INLINE void
brk_off (MFP *regs)
{
	regs->tsr &= ~0x8;
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
	splx (sr);
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
	splx (sr);
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
	splx (sr);
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
	splx (sr);
# endif
}

INLINE void
top_brk_on (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	brk_on (iovar->regs);
	splx (sr);
}

INLINE void
top_brk_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	brk_off (iovar->regs);
	splx (sr);
}

/* END inline assembler primitives - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization - top half */

/* only in one place used -> INLINE
 */
INLINE int
init_MFP (IOVAR **iovar, MFP *regs)
{
	char *buffer;
	
	*iovar = kmalloc (sizeof (**iovar));
	if (!*iovar)
		goto error;
	
	bzero (*iovar, sizeof (**iovar));
	
	buffer = kmalloc (2 * IOBUFSIZE);
	if (!buffer)
		goto error;
	
	(*iovar)->input.buffer = buffer;
	(*iovar)->output.buffer = buffer + IOBUFSIZE;
	
	(*iovar)->input.low_water = (*iovar)->output.low_water = 1 * (IOBUFSIZE / 4);
	(*iovar)->input.high_water = (*iovar)->output.high_water = 3 * (IOBUFSIZE / 4);
	
	(*iovar)->regs = regs;
	
	(*iovar)->datmask = 0xff;
	
	/* soft line always on */
	(*iovar)->state = STATE_SOFT;
	
	/* read current CTS state */
	if (!(regs->gpip & 0x4))
		(*iovar)->state |= STATE_HARD;
	
	/* RTS/CTS handshake */
	(*iovar)->shake = SHAKE_HARD;
	
	/* tx buffer is empty here */
	(*iovar)->tx_empty = 1;
	
	/* reset the MFP to default values */
	regs->ucr = 0x88;
	regs->rsr = 0x00;
	regs->tsr = 0x00;
	
	/* set standard baudrate */
	regs->tcdcr &= 0x70;
	regs->tddr = 0x01;
	regs->tcdcr |= 1;
	(*iovar)->baudrate = 19200;
	
	rts_on (regs);
	
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
init_mfp (void)
{
	uchar reg;
	
	
	init_MFP (&iovar_mfp, _mfpregs);
	
	iovar_mfp->bdev = BDEV_OFFSET;
	
	IOVARS (0) = iovar_mfp;
	IOVARS (0 + IOVAR_TTY_OFFSET) = iovar_mfp;
	
	DEBUG (("init_mfp: installing interrupt handlers ..."));
	Mfpint ( 1, mfp1_dcdint);
	Mfpint ( 2, mfp1_ctsint);
	Mfpint (10, mfp1_txempty);
	Mfpint (11, mfp1_rxerror);
	Mfpint (12, mfp1_rxavail);
	DEBUG (("init_mfp: done"));
	
	
	/* enable interrupts */
	
	reg = iovar_mfp->regs->iera;
	reg |= 0x4 | 0x8 | 0x10;
	iovar_mfp->regs->iera = reg;
	
	reg = iovar_mfp->regs->ierb;
	reg |= 0x2 | 0x4;
	iovar_mfp->regs->ierb = reg;
	
	
	/* set interrupt mask */
	
	reg = iovar_mfp->regs->imra;
	reg |= 0x8 | 0x10;
	iovar_mfp->regs->imra = reg;
	
	reg = iovar_mfp->regs->imrb;
	reg |= 0x2 | 0x4;
	iovar_mfp->regs->imrb = reg;
	
	
	/* enable rx/tx */
	
	iovar_mfp->regs->rsr = 0x01;
	iovar_mfp->regs->tsr = 0x01;
	
	
	DEBUG (("init_mfp: initialization done"));
}

DEVDRV * _cdecl
init (struct kerinfo *k)
{
	long mch;
	int i;
	
	struct dev_descr raw_dev_descriptor =
	{
		&raw_devtab,
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
		44,		/* drvsize */
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
	
# define SSYS_GETCOOKIE	8
# define COOKIE__MCH	0x5f4d4348L
/* values of MCH cookie
 */
# define ST		0
# define STE		0x00010000L
# define MEGASTE	0x00010010L
# define TT		0x00020000L
# define FALCON		0x00030000L
# define MILAN_C	0x00040000L
	if ((s_system (SSYS_GETCOOKIE, COOKIE__MCH, (long) &mch) != 0)
# ifdef MILAN
		|| (mch != MILAN_C))
# else
# ifdef TT_MFP
		|| (mch != TT))
# else
		|| ((mch != ST) && (mch != STE) && (mch != MEGASTE) && (mch != TT)))
# endif
# endif
	{
		c_conws (MSG_MACHINE);
		goto failure;
	}
	
	init_mfp ();
	
	i = 0;
	{
		if (IOVARS (i))
		{
			char name [64];
			
			ksprintf (name, "u:\\dev\\%s", RSVF_MFP);
			
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
					ksprintf (name, "%s", RSVF_MFP);
					add_rsvfentry (name, RSVF_PORT | RSVF_GEMDOS | RSVF_BIOS, raw_dev_descriptor.bdev);
				}
			}
			
			
			ksprintf (name, "u:\\dev\\%s%i", TTY_BASENAME, i + (BDEV_OFFSET - BDEV_START));
			
			tty_dev_descriptor.dinfo = i + IOVAR_TTY_OFFSET;
			tty_dev_descriptor.tty = &(IOVARS (i)->tty);
			if (d_cntl (DEV_INSTALL, name, (long) &tty_dev_descriptor) >= 0)
			{
				DEBUG (("%s: %s installed", __FILE__, name));
				
				IOVARS (i + IOVAR_TTY_OFFSET) = IOVARS (i);
			}
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

static volatile ushort notified = 0;
static volatile ushort dcdchange_notified = 0;

INLINE void
notify_top_half (void)
{
	if (!notified)
	{
		notified = 1;
		addroottimeout (0L, check_event, 0x1);
	}
}

static void
mfp_dcdint (void)
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
	
	if (regs->gpip & 0x02)
	{
		/* DCD is off */
		
		/* Flankenregister (fallende Flanke) */
		regs->aer &= ~0x2;
		
		if (!dcdchange_notified)
		{
			dcdchange_notified = 1;
			addroottimeout (0L, soft_dcdchange, 0x1);
		}
	}
	else
	{
		/* DCD is on */
		
		/* DCD Flanke setzen */
		regs->aer |= 0x2;
	}
	
	/* acknowledge */
	regs->isrb = ~0x02;
}

static void
mfp_ctsint (void)
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
	
	if (regs->gpip & 0x04)
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

static void
wr_mfp (IOVAR *iovar, MFP *regs)
{
	DEBUG_I (("wr_mfp: %lx", regs));
	
	if (!(regs->tsr & 0x80))
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
				DEBUG_I (("wr_scc: buffer empty"));
				
				/* buffer empty */
				iovar->tx_empty = 1;
				
				//txint_off (iovar, regs);
			}
			else
			{
				/* send character from buffer */
				regs->udr = iorec_get (&iovar->output);
			}
		}
	}
}

static void
mfp_txempty (void)
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
				
				notify_top_half ();
			}
		}
	}
	
	/* acknowledge */
	regs->isra = ~0x04;
}

static void
mfp_rxerror (void)
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
	DEBUG_I (("mfp_rxerror: handling MFP at %lx", regs));
	
	/* skip data */
	(void) regs->udr;
	
	/* acknowledge */
	regs->isra = ~0x08;
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
//	while (regs->rsr & 0x80);
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
//	while (regs->rsr & 0x80);
}

static void
mfp_rxavail (void)
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
			rts_off (regs);
		}
		
		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XOFF */
			iovar->sendnow = XOFF;
			wr_mfp (iovar, regs);
		}
	}
	
	notify_top_half ();
	
	/* acknowledge */
	regs->isra = ~0x10;
}

/* interrupt wrappers - call the target C routines
 */
static void
mfp_intrwrap (void)
{
	mfp_dcdint   ();
	mfp_ctsint   ();
	mfp_txempty  ();
	mfp_rxerror  ();
	mfp_rxavail  ();
	
	mfp_intrwrap ();
	
	
	/*
	 * MFP port
	 */
	
	asm volatile
	(
		"_mfp1_dcdint:
		 movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)
		 move.l  _iovar_mfp,%%a0
		 bsr     _mfp_dcdint
		 movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2
		 rte"
		: 			/* output register */
		:  			/* input registers */
		 			/* clobbered */
	);
	
	asm volatile
	(
		"_mfp1_ctsint:
		 movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)
		 move.l  _iovar_mfp,%%a0
		 bsr     _mfp_ctsint
		 movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2
		 rte"
		: 			/* output register */
		:  			/* input registers */
		 			/* clobbered */
	);
	
	asm volatile
	(
		"_mfp1_txempty:
		 movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)
		 move.l  _iovar_mfp,%%a0
		 bsr     _mfp_txempty
		 movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2
		 rte"
		: 			/* output register */
		:  			/* input registers */
		 			/* clobbered */
	);
	
	asm volatile
	(
		"_mfp1_rxerror:
		 movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)
		 move.l  _iovar_mfp,%%a0
		 bsr     _mfp_rxerror
		 movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2
		 rte"
		: 			/* output register */
		:  			/* input registers */
		 			/* clobbered */
	);
	
	asm volatile
	(
		"_mfp1_rxavail:
		 movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)
		 move.l  _iovar_mfp,%%a0
		 bsr     _mfp_rxavail
		 movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2
		 rte"
		: 			/* output register */
		:  			/* input registers */
		 			/* clobbered */
	);
}

/* END interrupt handling - bottom half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN interrupt handling - top half */

static void
check_event (void)
{
	int i;
	
	notified = 0;
	
	for (i = 0; i < IOVAR_MAX && IOVARS (i); i++)
	{
		IOVAR *iovar = IOVARS (i);
		
		if (iovar->tty.rsel)
		{
			if (!iorec_empty (&iovar->input))
				wakeselect (iovar->tty.rsel);
		}
		
		if (iovar->tty.wsel)
		{
			if (iorec_used (&iovar->output) < iovar->output.low_water)
				wakeselect (iovar->tty.wsel);
		}
	}
}

static void
soft_dcdchange (void)
{
	int i;
	
	dcdchange_notified = 0;
	
	for (i = 0; i < IOVAR_MAX && IOVARS (i); i++)
	{
		IOVAR *iovar = IOVARS (i);
		
		if (iovar->regs->gpip & 0x02)
		{
			/* DCD is off */
			
			if (iovar->tty.pgrp)
				killgroup (iovar->tty.pgrp, SIGHUP, 1);
		}
	}
}

/* END interrupt handling - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN control functions - top half */

INLINE void
send_byte (IOVAR *iovar)
{
	if (iovar->regs->tsr & 0x80)
	{
		ushort sr;
		
		sr = splhigh ();
		wr_mfp (iovar, iovar->regs);
		splx (sr);
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
		
		top_dtr_off (iovar);
	}
	else
	{
		/* set baudrate to speed, enable dtr */
		
		static const struct
		{
			ushort	baudrate;
			uchar	timeconst;
			uchar	mode;
		}
		table [] =
		{
			{    50, 0x9a, 2 },
			{    75, 0x66, 2 },
			{   110, 0xAF, 1 },
			{   134, 0x8F, 1 },
			{   150, 0x80, 1 },
			{   200, 0x60, 1 },
			{   300, 0x40, 1 },
			{   600, 0x20, 1 },
			{  1200, 0x10, 1 },
			{  1800, 0x0B, 1 },
			{  2000, 0x0A, 1 },
			{  2400, 0x08, 1 },
			{  3600, 0x05, 1 },
			{  4800, 0x04, 1 },
			{  9600, 0x02, 1 },
			{ 19200, 0x01, 1 },
			{     0,    0, 0 }
		};
		
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
			
			regs->rsr &= 0xfe;
			regs->tsr &= 0xfe;
			
			regs->tcdcr &= 0x70;
			regs->tddr = table [i].timeconst;
			regs->tcdcr |= table [i].mode;
			
			regs->rsr |= 1;
			regs->tsr |= 1;
			
			splx (sr);
			
			iovar->baudrate = baudrate;
		}
		
		/* always enable dtr */
		top_dtr_on (iovar);
	}
	
	return E_OK;
}

static void
ctl_TIOCGFLAGS (IOVAR *iovar, ushort *buf)
{
	ushort flags = 0;
	uchar ucr = iovar->regs->ucr;
	
	switch (ucr & 0x60)
	{
		case 0x00: flags |= TF_8BIT;	break;
		case 0x20: flags |= TF_7BIT;	break;
		case 0x40: flags |= TF_6BIT;	break;
		case 0x60: flags |= TF_5BIT;	break;
	}
	
	switch (ucr & 0x18)
	{
		case 0x00:
		case 0x08: flags |= TF_1STOP;	break;
		case 0x10: flags |= TF_15STOP;	break;
		case 0x18: flags |= TF_2STOP;	break;
	}
	
	if (ucr & 0x4)
	{
		if (ucr & 0x2)	flags |= T_EVENP;
		else		flags |= T_ODDP;
	}
	
	if (iovar->shake & SHAKE_SOFT)
		flags |= T_TANDEM;
	
	if (iovar->shake & SHAKE_HARD)
		flags |= T_RTSCTS;
	
	*buf = flags;
}

static long
ctl_TIOCSFLAGS (IOVAR *iovar, ushort flags)
{
	ushort flag = 0;
	uchar datmask;
	uchar ucr = iovar->regs->ucr;
	
	ucr &= ~0x60;
	switch (flags & TF_CHARBITS)
	{
		case TF_5BIT:	ucr |= 0x60; datmask = 0x1f;	break;
		case TF_6BIT:	ucr |= 0x40; datmask = 0x3f;	break;
		case TF_7BIT:	ucr |= 0x20; datmask = 0x7f;	break;
		case TF_8BIT:	ucr |= 0x00; datmask = 0xff;	break;
		default:	return EBADARG;
	}
	
	ucr &= ~0x18;
	switch (flags & TF_STOPBITS)
	{
		case TF_1STOP:	ucr |= 0x08;			break;
		case TF_15STOP:	ucr |= 0x10;			break;
		case TF_2STOP:	ucr |= 0x18;			break;
		default:	return EBADARG;
	}
	
	if (flags & (T_EVENP | T_ODDP))
	{
		if ((flags & (T_EVENP | T_ODDP)) == (T_EVENP | T_ODDP))
			return EBADARG;
		
		/* enable parity */
		ucr |= 0x4;
		
		/* set even/odd parity */
		if (flags & T_EVENP)	ucr |= 0x2;
		else			ucr &= ~0x2;
	}
	else
	{
		/* disable parity */
		ucr &= ~0x4;
		
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
		if (flags & T_RTSCTS)
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
		DEBUG (("rx_start: buffer not full"));
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
		DEBUG (("tx_start: buffer not empty"));
	}
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
	
	DEBUG (("mfp_instat: leave (%li)", (used ? -1 : 0)));
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
			
			sleep (READY_Q, 0L);
		}
	}
	else
	{
		if (ret < iorec->low_water)
			/* start receiver */
			rx_start (iovar);
	}
	
	ret = iorec_get (iorec);
	
	DEBUG (("mfp_in: leave (%li)", ret));
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
	
	DEBUG (("mfp_outstat: leave (%li)", (free ? -1 : 0)));
	return (free ? -1 : 0);
}

static long _cdecl
mfp_out (int dev, int c)
{
	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	IOREC *iorec = &iovar->output;
	
	while (!iorec_put (iorec, c))
	{
		sleep (READY_Q, 0L);
	}
	
	/* start transfer - interrupt controlled */
	tx_start (iovar);
	
	DEBUG (("mfp_out: leave (E_OK)"));
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
	ctl_TIOCGFLAGS (iovar, &flags);
	
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
		
		if ((flags & TF_CHARBITS) == TF_15STOP)
		{
			flags &= ~TF_STOPBITS;
			flags |= TF_1STOP;
		}
		
		flags &= ~(T_EVENP | T_ODDP);
		if (ucr & 0x4)
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
		if (tsr & 0x8)
			top_brk_on (iovar);
		else
			top_brk_off (iovar);
	}
	
	
	/* now construct the return value
	 */
	
	/* get the current flags */
	ctl_TIOCGFLAGS (iovar, &flags);
	
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
	if (iovar->regs->tsr & 0x8)
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
	
	if (iovar->open)
	{
		if (denyshare (iovar->open, f))
		{
			DEBUG (("mfp_open: file sharing denied"));
			return EACCES;
		}
		
		/* set dtr if the first process open it */
		top_dtr_on (iovar);
	}
	
	f->pos = 0;
	f->next = iovar->open;
	iovar->open = f;
	
	if (dev >= IOVAR_TTY_OFFSET)
	{
		f->flags |= O_TTY;
	}
	else
	{
		/* force nonblocking on HSMODEM devices */
		f->flags |= O_NDELAY;
	}
	
	return E_OK;
}

static long _cdecl
mfp_writeb (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	long done = 0;
	
	DEBUG (("mfp_writeb [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	for (;;)
	{
		/* copy as much as possible */
		while ((bytes > 0) && iorec_put (iorec, *buf))
		{
			buf++; done++;
			bytes--;
		}
		
		/* start transfer - interrupt controlled */
		tx_start (iovar);
		
		if (f->flags & O_NDELAY)
		{
			if (!done)
			{
				/* return EWOULDBLOCK only for non HSMODEM devices */
				if (f->flags & O_TTY)
					done = EWOULDBLOCK;
			}
			
			break;
		}
		
		if (!bytes)
			break;
		
		/* sleep until there is enough room in the buffer
		 * to continue
		 */
		while (iorec_used (iorec) > iorec->low_water)
			sleep (READY_Q, 0L);
	}
	
	DEBUG (("mfp_writeb: leave (%ld)", done));
	return done;
}

static long _cdecl
mfp_readb (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	long done = 0;
	
	DEBUG (("mfp_readb [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	for (;;)
	{
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
		
		if (f->flags & O_NDELAY)
		{
			if (!done)
			{
				/* return EWOULDBLOCK only for non HSMODEM devices */
				if (f->flags & O_TTY)
					done = EWOULDBLOCK;
			}
			
			break;
		}
		
		if (!bytes)
			break;
		
		/* sleep until we received enough data or the buffer
		 * become to full
		 */
		{
			long i;
			
			i = iorec_used (iorec);
			while ((i < bytes) && (i < iorec->high_water))
			{
				sleep (READY_Q, 0L);
				i = iorec_used (iorec);
			}
		}
	}
	
	DEBUG (("mfp_readb: leave (%ld)", done));
	return done;
}

/*
 * Note: when a BIOS device is a terminal (i.e. has the O_TTY flag
 * set), bios_read and bios_write will only ever be called indirectly, via
 * tty_read and tty_write. That's why we can afford to play a bit fast and
 * loose with the pointers ("buf" is really going to point to a long) and
 * why we know that "bytes" is divisible by 4.
 */

static long _cdecl
mfp_twrite (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	long done = 0;
	const long *r = (const long *) buf;
	
	DEBUG (("mfp_twrite [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	for (;;)
	{
		/* copy as much as possible */
		while ((bytes > 0) && iorec_put (iorec, *r++))
		{
			done += 4;
			bytes -= 4;
		}
		
		/* start transfer - interrupt controlled */
		tx_start (iovar);
		
		if (f->flags & O_NDELAY)
		{
			if (!done)
				done = EWOULDBLOCK;
			
			break;
		}
		
		if (!bytes)
			break;
		
		/* sleep until there is enough room in the buffer
		 * to continue
		 */
		while (iorec_used (iorec) > iorec->low_water)
			sleep (READY_Q, 0L);
	}
	
	DEBUG (("mfp_twrite: leave (%ld)", done));
	return done;
}

static long _cdecl
mfp_tread (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	long done = 0;
	long *r = (long *) buf;
	
	DEBUG (("mfp_tread [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	for (;;)
	{
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
		
		if (f->flags & O_NDELAY)
		{
			if (!done)
				done = EWOULDBLOCK;
			
			break;
		}
		
		if (!bytes)
			break;
		
		/* sleep until we received enough data or the buffer
		 * become to full
		 */
		{
			long i;
			
			i = iorec_used (iorec);
			while ((i < bytes) && (i < iorec->high_water))
			{
				sleep (READY_Q, 0L);
				i = iorec_used (iorec);
			}
		}
	}
	
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
			int flushtype;
			
			/* HSMODEM put the value itself in arg
			 * MiNT use arg as a pointer to the value
			 * 
			 * if opened as terminal we assume MiNT convention
			 * otherwise HSMODEM convention
			 * 
			 * ok, or?
			 */
			if (f->fc.aux >= IOVAR_TTY_OFFSET)
				flushtype = *(int *) buf;
			else
				flushtype = (int) ((long) buf);
			
			if (flushtype <= 0)
			{
				r = ENOSYS;
				break;
			}
			
			if (flushtype & 1)
			{
				IOREC *iorec = &(iovar->input);
				uchar sr;
				
				sr = splhigh ();
				iorec->head = iorec->tail = 0;
				splx (sr);
			}
			
			if (flushtype & 2)
			{
				IOREC *iorec = &(iovar->output);
				uchar sr;
				
				sr = splhigh ();
				iorec->head = iorec->tail = 0;
				splx (sr);
			}
			
			break;
		}
		case TIOCSTOP:		/* HSMODEM */
		{
			/* if (iovar->shake)
				rx_stop (iovar); */
			
			r = ENOSYS;
			break;
		}
		case TIOCSTART:		/* HSMODEM */
		{
			rx_start (iovar);
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
			
			ctl_TIOCGFLAGS (iovar, (ushort *) buf);
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
			*(long *) buf = iorec_used (&iovar->output);
			break;
		}
		case TIOCSFLAGSB:	/* anywhere documented? */
		{
			r = ENOSYS;
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
			struct tty *tty = (struct tty *) f->devinfo;
			ushort *v = buf;
			
			if (v [0] > iorec_size (&iovar->input) / 2)
				v [0] = iorec_size (&iovar->input) / 2;
			
			tty->vmin = v [0];
			tty->vtime = v [1];
			
			/* t->vticks = 0; */
			
			break;
		}
		case TIOCWONLINE:
		{
# if 0
			struct tty *tty = (struct tty *) f->devinfo;
			
			while (tty->state & TS_BLIND)
				sleep (IO_Q, (long) &tty->state);
# else
			r = ENOSYS;
# endif
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
				| TIOCMH_DTR
				| TIOCMH_RTS
				| TIOCMH_CTS
				| TIOCMH_CD
				| TIOCMH_RI
				| TIOCMH_DSR
			/*	| TIOCMH_LEI */
			/*	| TIOCMH_TXD */
			/*	| TIOCMH_RXD */
				| TIOCMH_BRK
			/*	| TIOCMH_TER */
			/*	| TIOCMH_RER */
				| TIOCMH_TBE
				| TIOCMH_RBF
				;
			
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
			*_giselect = 14;
			reg = *_giread;
			if (!(reg & 0x10)) val |= TIOCMH_DTR;
			if (!(reg & 0x08)) val |= TIOCMH_RTS;
# endif
			
			reg = iovar->regs->gpip;
# ifdef MILAN
			if (!(reg & 0x08)) val |= TIOCMH_DTR;
			if (!(reg & 0x01)) val |= TIOCMH_RTS;
# endif
			if (!(reg & 0x04)) val |= TIOCMH_CTS;
			if (!(reg & 0x40)) val |= TIOCMH_RI;
			if (!(reg & 0x02)) val |= TIOCMH_CD;
			if (!(reg & 0x10)) val |= TIOCMH_DSR;
			
			/* TIOCMH_LEI */
			/* TIOCMH_TXD */
			/* TIOCMH_RXD */
			
			if (iovar->regs->rsr & 0x08) val |= TIOCMH_BRK;
			
			/* TIOCMH_TER */
			/* TIOCMH_RER */
			
			if (iovar->regs->tsr & 0x80) val |= TIOCMH_TBE;
			if (iovar->regs->rsr & 0x80) val |= TIOCMH_RBF;
			
			*(long *) buf = val;
			break;
		}
		case TIOCCTLSET:	/* HSMODEM */
		{
			long *arg = (long *) buf;
			
			register long mask = *arg++;
			register long val = *arg;
			
			if (mask & (TIOCMH_DTR | TIOCMH_RTS))
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
			*_giselect = 14;
			reg = *_giread;
			if (!(reg & 0x10)) val |= TIOCM_DTR;
			if (!(reg & 0x08)) val |= TIOCM_RTS;
# endif
			
			reg = iovar->regs->gpip;
# ifdef MILAN
			if (!(reg & 0x08)) val |= TIOCM_DTR;
			if (!(reg & 0x01)) val |= TIOCM_RTS;
# endif
			/* TIOCM_ST */
			/* TIOCM_SR */
			
			if (!(reg & 0x04)) val |= TIOCM_CTS;
			if (!(reg & 0x02)) val |= TIOCM_CD;
			if (!(reg & 0x40)) val |= TIOCM_RI;
			if (!(reg & 0x10)) val |= TIOCM_DSR;
			
			*(long *) buf = val;
			break;
		}
		case TIOCCDTR:
		{
			top_dtr_off (iovar);
			break;
		}
		case TIOCSDTR:
		{
			top_dtr_on (iovar);
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
					sleep (IO_Q, (long) iovar);
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
mfp_close (FILEPTR *f, int pid)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	
	DEBUG (("mfp_close [%i]: enter", f->fc.aux));
	
	if ((f->flags & O_LOCK)
	    && ((iovar->lockpid == pid) || (f->links <= 0)))
	{
		/* wake anyone waiting for this lock */
		wake (IO_Q, (long) iovar);
	}
	
	if (f->links <= 0)
	{
		register FILEPTR **temp;
		register long flag = 1;
		
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
		{
			ALERT (("mfp_close: remove open FILEPTR fail!", f->fc.aux));
		}
		
		/* clear dtr if no longer opened */
		if (!(iovar->open))
			top_dtr_off (iovar);
	}
	
	DEBUG (("mfp_close: leave ok"));
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
