/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000, 2001 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 1999-09-14
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * 2001-03-05:	(v0.27)
 * 
 * - new: on LAN port RTS is mapped to DTR line;
 *        the xdd behaves like HSMODEM now
 * 
 * 2001-01-16:	(v0.26)
 * 
 * - fix: bugfix in the file locking code, lockpid was reset to wrong value
 * 
 * 2000-06-12:	(v0.25)
 * 
 * - new: exact delay routine for SCC register access delay
 * 
 * 2000-06-06:	(v0.24)
 * 
 * - fix: tt_scca baudtable had a typo for 115200 setting; also
 *        removed falcon_scca table as it's the same as the tt_scca
 * 
 * 2000-05-24:	(v0.23)
 * 
 * - new: hopefully better behaviour on last close()
 *        (flush buffers, go back to carrier detection & update TS_BLIND)
 * 
 * 2000-05-16:	(v0.22)
 * 
 * - fix: removed unnecessary debug messages that are printed
 * 
 * 2000-05-14:	(v0.21)
 * 
 * - fix: corrected TS_BLIND <-> clocal handling; now hopefully correct
 * - new: bios emulation doesn't poll now, use sleep/wake mechanism
 *        like the read/write functions
 * 
 * 2000-05-12:	(v0.20)
 * 
 * - new: reworked the complete high level I/O routines;
 *        support now (hopefully) true tty functionality
 *        this include carrier handling, vmin/vtime handling,
 *        clocal settings and so on
 * - new: seperate read/write routines for HSMODEM devices;
 *        they never block
 * - fix: scc_close doesn't removed device locks
 * 
 * 2000-05-05:	(v0.16)
 * 
 * - fix: every SCC register write access is now delayed a fixed time
 * 
 * 2000-05-04:	(v0.15)
 * 
 * - fix: write register access with delay to verify SCC register integrity
 * 
 * 2000-01-32:	(v0.14)
 * 
 * - new: added CD loss detection -> SIGHUP
 * 
 * 2000-01-32:	(v0.13)
 * 
 * - new: changed naming scheme; now hopefully more logical
 * - fix: corrected BIOS device numbers that are overlayed
 * 
 * 2000-01-24:	(v0.12)
 * 
 * - new: added LAN port handling
 * - fix: changed SCC base address from 0x00ff8c80 to 0xffff8c80
 * 
 * 2000-01-20:	(v0.11)
 * 
 * - fix: bug in SCC register declaration; missing dummy chars
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
# include "mint/arch/delay.h"
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
# include "scc.h"


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	27
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
# define RSVF_SCCA_1	"serial2"
# define RSVF_SCCA_2	"lan"
# define RSVF_SCCB	"modem2"

# define TTY_BASENAME	"ttyS"

# define BDEV_START	6
# define BDEV_OFFSET	7


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
	"\033p SCC serial driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\275 1998, 1999 by Rainer Mannigel.\r\n" \
	"\275 2000-2010 by Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_MACHINE	\
	"\033pThis driver requires a MegaSTE/TT/Falcon/Hades!\033q\r\n"

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
	ushort	timeconst;
	uchar	clockdivisor;
	uchar	clockmode;
	uchar	brgenmode;
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
	SCC	*regs;		/* UART register base address */
	
	uchar	wr3;		/* content of write register 3 */
	uchar	wr4;		/* content of write register 4 */
	uchar	wr5;		/* content of write register 5 */
	uchar	lan;		/* LAN port flag */
	
	volatile ushort	state;	/* receiver state */
# define STATE_HARD		0x1		/* CTS is low */
# define STATE_SOFT		0x2		/* XOFF is set */
	
	ushort	shake;		/* handshake mode */
# define SHAKE_HARD		STATE_HARD	/* RTS/CTS handshake enabled */
# define SHAKE_SOFT		STATE_SOFT	/* XON/XOFF handshake enabled */
	
	uchar	rx_full;	/* input buffer full */
	uchar	tx_empty;	/* output buffer empty */
	
	uchar	sendnow;	/* control data, bypasses buffer */
	uchar	datmask;	/* AND mask for read from sccdat */
	
	uchar	iointr;		/* I/O interrupt */
	uchar	stintr;		/* status interrupt */
	ushort	lan_dev;	/* device number of LAN port */
# define LANDEV(iovar)		(iovar->lan_dev)
	
	IOREC	input;		/* input buffer */
	IOREC	output;		/* output buffer */
	
	BAUDS	*table;		/* baudrate vector */
	long	baudrate;	/* current baud rate value */
	
	ushort	res1;		/* padding */
	
	ushort	bdev;		/* BIOS devnumber */
	ushort	iosleepers;	/* number of precesses that are sleeping in I/O */
	ushort	lockpid;	/* file locking */
	
	ushort	clocal;		/* */
	ushort	brkint;		/* */
	
	FILEPTR	*open;		/* */
	TTY	tty;		/* */
};


# define XON	0x11
# define XOFF	0x13


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
INLINE void	rts_on		(IOVAR *iovar, SCC *regs);
INLINE void	rts_off		(IOVAR *iovar, SCC *regs);
INLINE void	dtr_on		(IOVAR *iovar, SCC *regs);
INLINE void	dtr_off		(IOVAR *iovar, SCC *regs);
INLINE void	brk_on		(IOVAR *iovar, SCC *regs);
INLINE void	brk_off		(IOVAR *iovar, SCC *regs);
INLINE void	txint_off	(IOVAR *iovar, SCC *regs);


/*
 * inline assembler primitives - top half
 */
INLINE void	top_rts_on	(IOVAR *iovar);
INLINE void	top_rts_off	(IOVAR *iovar);
INLINE void	top_dtr_on	(IOVAR *iovar);
INLINE void	top_dtr_off	(IOVAR *iovar);
INLINE void	top_brk_on	(IOVAR *iovar);
INLINE void	top_brk_off	(IOVAR *iovar);
INLINE void	top_txint_off	(IOVAR *iovar);


/*
 * initialization - top half
 */
static int	init_SCC	(IOVAR **iovar, SCC *regs);
INLINE void	init_scc	(void);


/*
 * interrupt handling - bottom half
 */
INLINE void	notify_top_half	(IOVAR *iovar);
static void	wr_scc 		(IOVAR *iovar, SCC *regs);

static void	scca_txempty_asm(void);
static void scca_rxavail_asm(void);
static void scca_stchange_asm(void);
static void scca_special_asm(void);
static void sccb_txempty_asm(void);
static void sccb_rxavail_asm(void);
static void sccb_stchange_asm(void);
static void sccb_special_asm(void);


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
static long _cdecl	scc_instat	(int dev);
static long _cdecl	scc_in		(int dev);
static long _cdecl	scc_outstat	(int dev);
static long _cdecl	scc_out		(int dev, int c);
static long _cdecl	scc_rsconf	(int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr);


/*
 * device driver routines - top half
 */
static long _cdecl	scc_open	(FILEPTR *f);
static long _cdecl	scc_close	(FILEPTR *f, int pid);
static long _cdecl	scc_write	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	scc_read	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	scc_writeb	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	scc_readb	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	scc_twrite	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	scc_tread	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	scc_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl	scc_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl	scc_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	scc_select	(FILEPTR *f, long proc, int mode);
static void _cdecl	scc_unselect	(FILEPTR *f, long proc, int mode);


/*
 * device driver maps
 */
static BDEVMAP bios_devtab =
{
	scc_instat, scc_in,
	scc_outstat, scc_out,
	scc_rsconf
};

static DEVDRV hsmodem_devtab =
{
	scc_open,
	scc_write, scc_read, scc_lseek, scc_ioctl, scc_datime,
	scc_close,
	scc_select, scc_unselect,
	NULL, NULL
};

static DEVDRV tty_devtab =
{
	scc_open,
	scc_twrite, scc_tread, scc_lseek, scc_ioctl, scc_datime,
	scc_close,
	scc_select, scc_unselect,
	scc_writeb, scc_readb
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

/* values of MCH cookie
 */
# define ST		0
# define STE		0x00010000L
# define MEGASTE	0x00010010L
# define TT		0x00020000L
# define FALCON		0x00030000L
# define MILAN_C	0x00040000L

static long mch;
static ushort flag_14_7456_mhz = 0;

static IOVAR *iovar_scca;
static IOVAR *iovar_sccb;

# define IOVAR_MAX	4

static IOVAR *iovar_tab [IOVAR_MAX * 2];

# define IOVARS(nr)		(iovar_tab [nr])
# define IOVAR_TTY_OFFSET	(IOVAR_MAX)
# define IOVAR_REAL_MAX		(IOVAR_MAX * 2)


static BAUDS baudtable_tt_scca [] =
{
	{     50,  2293,  TAKT_16x,  0x50,  0x1  },
	{     75,  1528,  TAKT_16x,  0x50,  0x1  },
//	{    110,  2286,  TAKT_16x,  0x50,  0x3  },
//	{    134,  1876,  TAKT_16x,  0x50,  0x3  },
//	{    150,   763,  TAKT_16x,  0x50,  0x1  },
//	{    200,  1256,  TAKT_16x,  0x50,  0x3  },
	{    300,   837,  TAKT_16x,  0x50,  0x3  },
	{    600,   417,  TAKT_16x,  0x50,  0x3  },
	{   1200,   208,  TAKT_16x,  0x50,  0x3  },
	{   1800,   138,  TAKT_16x,  0x50,  0x3  },
	{   2000,   124,  TAKT_16x,  0x50,  0x3  },
	{   2400,   103,  TAKT_16x,  0x50,  0x3  },
	{   3600,    68,  TAKT_16x,  0x50,  0x3  },
	{   4800,    22,  TAKT_16x,  0x50,  0x1  },
	{   9600,    10,  TAKT_16x,  0x50,  0x1  },
	{  19200,     4,  TAKT_16x,  0x50,  0x1  },
	{  38400,     1,  TAKT_16x,  0x50,  0x1  },
	{  57600,     0,  TAKT_16x,  0x50,  0x1  },
	{ 115200,     0,  TAKT_32x,  0x00,  0x0  },
	{ 230400,     0,  TAKT_16x,  0x00,  0x0  },
	{      0,     0,         0,     0,    0  }
};

static BAUDS baudtable_tt_sccb [] =
{
//	{     50,   190,  TAKT_16x,  0x50,  0x1  },
//	{     75,   126,  TAKT_16x,  0x50,  0x1  },
//	{    110,  2286,  TAKT_16x,  0x50,  0x3  },
	{    134,  1876,  TAKT_16x,  0x50,  0x3  },
	{    150,    62,  TAKT_16x,  0x50,  0x1  },
	{    200,  1256,  TAKT_16x,  0x50,  0x3  },
	{    300,    30,  TAKT_16x,  0x50,  0x1  },
	{    600,   417,  TAKT_16x,  0x50,  0x3  },
	{   1200,     6,  TAKT_16x,  0x50,  0x1  },
	{   1800,   138,  TAKT_16x,  0x50,  0x3  },
	{   2000,   124,  TAKT_16x,  0x50,  0x3  },
	{   2400,     2,  TAKT_16x,  0x50,  0x1  },
	{   3600,    68,  TAKT_16x,  0x50,  0x3  },
	{   4800,     0,  TAKT_16x,  0x50,  0x1  },
	{   9600,     0,  TAKT_32x,  0x00,  0x0  },
	{  19200,     0,  TAKT_16x,  0x00,  0x0  },
	{  38400,     0,  TAKT_64x,  0x28,  0x0  },
	{  76800,     0,  TAKT_32x,  0x28,  0x0  },
	{ 153600,     0,  TAKT_16x,  0x28,  0x0  },
	{      0,     0,         0,     0,    0  }
};

# if 0
static BAUDS baudtable_falcon_scca [] =
{
	{     50,  2293,  TAKT_16x,  0x50,  0x1  },
	{     75,  1528,  TAKT_16x,  0x50,  0x1  },
//	{    110,  2286,  TAKT_16x,  0x50,  0x3  },
//	{    134,  1876,  TAKT_16x,  0x50,  0x3  },
//	{    150,   763,  TAKT_16x,  0x50,  0x1  },
//	{    200,  1256,  TAKT_16x,  0x50,  0x3  },
	{    300,   837,  TAKT_16x,  0x50,  0x3  },
	{    600,   417,  TAKT_16x,  0x50,  0x3  },
	{   1200,   208,  TAKT_16x,  0x50,  0x3  },
	{   1800,   138,  TAKT_16x,  0x50,  0x3  },
	{   2000,   124,  TAKT_16x,  0x50,  0x3  },
	{   2400,   103,  TAKT_16x,  0x50,  0x3  },
	{   3600,    68,  TAKT_16x,  0x50,  0x3  },
	{   4800,    22,  TAKT_16x,  0x50,  0x1  },
	{   9600,    10,  TAKT_16x,  0x50,  0x1  },
	{  19200,     4,  TAKT_16x,  0x50,  0x1  },
	{  38400,     1,  TAKT_16x,  0x50,  0x1  },
	{  57600,     0,  TAKT_16x,  0x50,  0x1  },
	{ 115200,     0,  TAKT_32x,  0x00,  0x0  },
	{ 230400,     0,  TAKT_16x,  0x00,  0x0  },
	{      0,     0,         0,     0,    0  }
};
# endif

static BAUDS baudtable_falcon_sccb [] =
{
//	{     50,  2293,  TAKT_16x,  0x50,  0x1  },
//	{     75,  1528,  TAKT_16x,  0x50,  0x1  },
//	{    110,  2286,  TAKT_16x,  0x50,  0x3  },
//	{    134,  1876,  TAKT_16x,  0x50,  0x3  },
//	{    150,   763,  TAKT_16x,  0x50,  0x1  },
//	{    200,  1256,  TAKT_16x,  0x50,  0x3  },
	{    300,   837,  TAKT_16x,  0x50,  0x3  },
	{    600,   417,  TAKT_16x,  0x50,  0x3  },
	{   1200,   208,  TAKT_16x,  0x50,  0x3  },
	{   1800,   138,  TAKT_16x,  0x50,  0x3  },
	{   2000,   124,  TAKT_16x,  0x50,  0x3  },
	{   2400,   103,  TAKT_16x,  0x50,  0x3  },
	{   3600,    68,  TAKT_16x,  0x50,  0x3  },
	{   4800,    22,  TAKT_16x,  0x50,  0x1  },
	{   9600,    10,  TAKT_16x,  0x50,  0x1  },
	{  19200,     4,  TAKT_16x,  0x50,  0x1  },
	{  38400,     0,  TAKT_64x,  0x28,  0x0  },
	{  57600,     0,  TAKT_16x,  0x50,  0x1  },
	{  76800,     0,  TAKT_32x,  0x28,  0x0  },
	{ 115200,     0,  TAKT_32x,  0x00,  0x0  },
	{ 153600,     0,  TAKT_16x,  0x28,  0x0  },
	{ 230400,     0,  TAKT_16x,  0x00,  0x0  },
	{      0,     0,         0,     0,    0  }
};

/* PCLK 14.7456 MHZ
 * 
 * TT		(modified)
 * Hades
 */
static BAUDS baudtable_14_7456_mhz [] =
{
//	{      50,  9214,  TAKT_16x,  0x50,  0x3  },
//	{      75,  6142,  TAKT_16x,  0x50,  0x3  },
//	{     110,  4187,  TAKT_16x,  0x50,  0x3  },
//	{     134,   854,  TAKT_16x,  0x50,  0x1  },
//	{     150,  3070,  TAKT_16x,  0x50,  0x3  },
//	{     200,  2302,  TAKT_16x,  0x50,  0x3  },
	{     300,  1534,  TAKT_16x,  0x50,  0x3  },
	{     600,   766,  TAKT_16x,  0x50,  0x3  },
	{    1200,   382,  TAKT_16x,  0x50,  0x3  },
	{    1800,   254,  TAKT_16x,  0x50,  0x3  },
	{    2000,   228,  TAKT_16x,  0x50,  0x3  },
	{    2400,   190,  TAKT_16x,  0x50,  0x3  },
	{    3600,   126,  TAKT_16x,  0x50,  0x3  },
	{    4800,    94,  TAKT_16x,  0x50,  0x3  },
	{    9600,    46,  TAKT_16x,  0x50,  0x3  },
	{   19200,    22,  TAKT_16x,  0x50,  0x3  },
	{   38400,    10,  TAKT_16x,  0x50,  0x3  },
	{   57600,     6,  TAKT_16x,  0x50,  0x3  },
	{   76800,     4,  TAKT_16x,  0x50,  0x3  },
	{  115200,     2,  TAKT_16x,  0x50,  0x3  },
	{  153600,     1,  TAKT_16x,  0x50,  0x3  },
	{  230400,     0,  TAKT_16x,  0x50,  0x3  },
	{       0,     0,         0,     0,    0  }
};

/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN SCC register access */

# define ZS_PAUSE	udelay (10)

INLINE void
ZS_WRITE_0 (SCC *regs, uchar data)
{
	regs->sccctl = data;
	ZS_PAUSE;
}

INLINE void
ZS_WRITE (SCC *regs, uchar reg, uchar data)
{
	regs->sccctl = reg;
	ZS_PAUSE;
	
	regs->sccctl = data;
	ZS_PAUSE;
}

INLINE void
ZS_WRITE_DATA (SCC *regs, uchar data)
{
	regs->sccdat = data;
	ZS_PAUSE;
}

INLINE uchar
ZS_READ_0 (SCC *regs)
{
	return regs->sccctl;
}

INLINE uchar
ZS_READ (SCC *regs, uchar reg)
{
	regs->sccctl = reg;
	ZS_PAUSE;
	
	return regs->sccctl;
}

INLINE uchar
ZS_READ_DATA (SCC *regs)
{
	return regs->sccdat;
}

/* END SCC register access */
/****************************************************************************/

/****************************************************************************/
/* BEGIN inline assembler primitives - bottom half */

INLINE void
rts_on (IOVAR *iovar, SCC *regs)
{
	iovar->wr5 |= RTS;
	ZS_WRITE (regs, WR5, iovar->wr5);
}

INLINE void
rts_off (IOVAR *iovar, SCC *regs)
{
	iovar->wr5 &= ~RTS;
	ZS_WRITE (regs, WR5, iovar->wr5);
}


INLINE void
dtr_on (IOVAR *iovar, SCC *regs)
{
	iovar->wr5 |= DTR;
	ZS_WRITE (regs, WR5, iovar->wr5);
}

INLINE void
dtr_off (IOVAR *iovar, SCC *regs)
{
	iovar->wr5 &= ~DTR;
	ZS_WRITE (regs, WR5, iovar->wr5);
}


INLINE void
brk_on (IOVAR *iovar, SCC *regs)
{
	iovar->wr5 |= SBRK;
	ZS_WRITE (regs, WR5, iovar->wr5);
}

INLINE void
brk_off (IOVAR *iovar, SCC *regs)
{
	iovar->wr5 &= ~SBRK;
	ZS_WRITE (regs, WR5, iovar->wr5);
}

INLINE void
txint_off (IOVAR *iovar, SCC *regs)
{
	ZS_WRITE_0 (regs, RESINT);
}

/* END inline assembler primitives - bottom half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN inline assembler primitives - top half */

INLINE void
top_rts_on (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	rts_on (iovar, iovar->regs);
	spl (sr);
}

INLINE void
top_rts_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	rts_off (iovar, iovar->regs);
	spl (sr);
}

INLINE void
top_dtr_on (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	dtr_on (iovar, iovar->regs);
	spl (sr);
}

INLINE void
top_dtr_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	dtr_off (iovar, iovar->regs);
	spl (sr);
}

INLINE void
top_brk_on (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	brk_on (iovar, iovar->regs);
	spl (sr);
}

INLINE void
top_brk_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	brk_off (iovar, iovar->regs);
	spl (sr);
}

INLINE void
top_txint_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	ZS_WRITE_0 (iovar->regs, RESINT);
	spl (sr);
}

/* END inline assembler primitives - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization - top half */

static int
init_SCC (IOVAR **iovar, SCC *regs)
{
	char *buffer = NULL;
	
	*iovar = kmalloc (sizeof (**iovar));
//	*iovar = m_xalloc (sizeof (**iovar), 0x20|0);
	if (!*iovar)
		goto error;
	
	bzero (*iovar, sizeof (**iovar));
	
	buffer = kmalloc (2 * IOBUFSIZE);
//	buffer = m_xalloc (2 * IOBUFSIZE, 0x20|0);
	if (!buffer)
		goto error;
	
	(*iovar)->input.buffer		= (unsigned char *)buffer;
	(*iovar)->input.low_water	= 1 * (IOBUFSIZE / 4);
	(*iovar)->input.high_water	= 3 * (IOBUFSIZE / 4);
	
	(*iovar)->output.buffer		= (unsigned char *)buffer + IOBUFSIZE;
	(*iovar)->output.low_water	= 1 * (IOBUFSIZE / 4);
	(*iovar)->output.high_water	= 3 * (IOBUFSIZE / 4);
	
	(*iovar)->regs = regs;
	
	{
		static const uchar init_reglist [] =
		{
			WR4,	STB_ASYNC1|TAKT_16x,
			WR1,	0x04,
			WR2,	0x60,
			WR3,	0xC0,
			WR5,	TX8BIT,
			WR6,	0x00,
			WR7,	0x00,
			WR9,	0x01,
			WR10,	0x00,
			WR11,	0x50,
			WR12,	0x18,
			WR13,	0x00,
			WR14,	0x02,
			WR14,	0x03,
			WR3,	0xC1,
			WR5,	TXEN|TX8BIT,
			WR15,	0x08|0x20,	/* CD, RTS intr enable */
			WR0,	0x10,
			WR0,	0x10,
			WR1,	0x17,
			WR9,	0x09,
			
			0xff	/* end */
		};
		
		const uchar *src = init_reglist;
		ushort sr;
		uchar ctlreg;
		
		sr = splhigh ();
		
		while (*src != 0xff)
		{
			ZS_WRITE (regs, src[0], src[1]);
			src += 2;
		}
		
		spl (sr);
		
		(*iovar)->wr3 = 0xC1;
		(*iovar)->wr4 = STB_ASYNC1|TAKT_16x;
		(*iovar)->wr5 = TXEN|TX8BIT;
		(*iovar)->lan = 0;
		(*iovar)->datmask = 0xff;
		(*iovar)->baudrate = 9600;
		
		/* soft line always on */
		/* why??? (*iovar)->state = STATE_SOFT; */
		
		ctlreg = ZS_READ_0 (regs);
		
		/* read current CTS state */
		if (!(ctlreg & CTS))
			/* CTS is off */
			(*iovar)->state |= STATE_HARD;
		
		/* RTS/CTS handshake */
		(*iovar)->shake = SHAKE_HARD;
		
		/* read current CD state */
		if (!(ctlreg & DCD))
			/* CD is off, no carrier */
			(*iovar)->tty.state |= TS_BLIND;
		
		(*iovar)->rx_full = 1;	/* receiver disabled */
		(*iovar)->tx_empty = 1;	/* transmitter empty */
		
		(*iovar)->sendnow = 0;
		(*iovar)->datmask = 0xff;
		
		(*iovar)->iointr = 0;
		(*iovar)->stintr = 0;
		(*iovar)->lan_dev = 0;
		
		(*iovar)->input.head = (*iovar)->input.tail = 0;
		(*iovar)->output.head = (*iovar)->output.tail = 0;
		
		(*iovar)->clocal = 0;
		(*iovar)->brkint = 1;
	}
	
	return 1;
	
error:
	if (*iovar)
		kfree (*iovar);
	
	if (buffer)
		kfree (buffer);
	
	ALERT (("scc.xdd: kmalloc(%li, %li) fail, out of memory?", sizeof (**iovar), IOBUFSIZE));
	return 0;
}
	
INLINE void
init_scc (void)
{
	{
		SCC *regs = scca;
		
		static const uchar init_reglist [] =
		{
			WR9,	0xc0,	/* 11000000 - Hardware reset */
			WR12,	0xff,	/* 11111111 - Time constant low byte */
			WR13,	0xff,	/* 11111111 - Time constant hi byte */
			WR14,	0x03,	/* 00000011 - BRG clocksource = RTxC, BRG enable */
			WR15,	0x02,	/* 00000010 - Zero Count IE */
			WR1,	0x01,	/* 00000001 - Ext Int Enable */
			WR9,	0x20,	/* 00100000 - Software INTACK enable */
			
			0xff	/* end */
		};
		
		const uchar *src = init_reglist;
		ulong time;
		ushort sr;
		int i;
		
		sr = splhigh ();
		
		while (*src != 0xff)
		{
			ZS_WRITE (regs, src[0], src[1]);
			src += 2;
		}
		
		spl (sr);
		
		i = 40;
		time = *_hz_200;
		while (i--)
		{
			uchar status;
			
			do {
				sr = splhigh ();
				status = ZS_READ (regs, RR3);
				spl (sr);
			}
			while (!(status & 0x8));
			
			sr = splhigh ();
			(void) ZS_READ (regs, RR2);
			spl (sr);
			
			ZS_WRITE_0 (regs, RESESI);
			ZS_WRITE_0 (regs, RHIUS);
		}
		time = *_hz_200 - time;
		
		if (time < 50)
		{
			flag_14_7456_mhz = 1;
			c_conws ("PCLK is 14.7456 MHz\r\n\r\n");
		}
		else
			c_conws ("PCLK is 8.053976 MHz\r\n\r\n");
	}
	
	/* interrupt vectors
	 * 
	 * 0x180 - SCC Port B Transmit Buffer Empty
	 * 0x184 - unused
	 * 0x188 - SCC Port B External Status Change
	 * 0x18c - unused
	 * 0x190 - SCC Port B Receive Character Available
	 * 0x194 - unused
	 * 0x198 - SCC Port B Special Receive Condition
	 * 0x19c - unused
	 * 0x1a0 - SCC Port A Transmit Buffer Empty
	 * 0x1a4 - unused
	 * 0x1a8 - SCC Port A External Status Change
	 * 0x1ac - unused
	 * 0x1b0 - SCC Port A Receive Character Available
	 * 0x1b4 - unused
	 * 0x1b8 - SCC Port A Special Receive Condition
	 * 0x1bc - unused
	 */
	
	init_SCC (&iovar_sccb, sccb);
	if (iovar_sccb)
	{
		if (flag_14_7456_mhz)
			iovar_sccb->table = baudtable_14_7456_mhz;
		else if (mch == FALCON)
			iovar_sccb->table = baudtable_falcon_sccb;
		else
			iovar_sccb->table = baudtable_tt_sccb;
		
		/* SCC-B is always modem2 aka BIOS 7 */
		iovar_sccb->bdev = BDEV_OFFSET;
		
		/* and the first device in our device list */
		IOVARS (0) = iovar_sccb;
		
# define vector(x)	(x / 4)
		
		(void) Setexc (vector (0x180), sccb_txempty_asm);
		(void) Setexc (vector (0x188), sccb_stchange_asm);
		(void) Setexc (vector (0x190), sccb_rxavail_asm);
		(void) Setexc (vector (0x198), sccb_special_asm);
		
# undef vector
	}
	
	init_SCC (&iovar_scca, scca);
	if (iovar_scca)
	{
		if (flag_14_7456_mhz)
			iovar_scca->table = baudtable_14_7456_mhz;
		/* else if (mch == FALCON)
			iovar_scca->table = baudtable_falcon_scca; */
		else
			iovar_scca->table = baudtable_tt_scca;
		
		/* SCC-A is serial2/lan BIOS 8 except TT there it is BIOS 9
		 * as BIOS 8 is TT-MFP
		 */
		if (mch != TT)
			iovar_scca->bdev = BDEV_OFFSET + 1;
		else
			iovar_scca->bdev = BDEV_OFFSET + 2;
		
		/* for non Falcons we support switchable serial2 */
		if (mch != FALCON)
			IOVARS (2) = iovar_scca;
		
		if ((mch == MEGASTE) || (mch == TT) || (mch == FALCON))
		{
			IOVARS (3) = iovar_scca;
			
			if (mch == FALCON)
				/* fixed lan port */
				iovar_scca->lan = 1;
			else
				/* switchable port
				 * the lan flag is updated in open()
				 */
				iovar_scca->lan_dev = 3;
		}
		
# define vector(x)	(x / 4)
		
		(void) Setexc (vector (0x1a0), scca_txempty_asm);
		(void) Setexc (vector (0x1a8), scca_stchange_asm);
		(void) Setexc (vector (0x1b0), scca_rxavail_asm);
		(void) Setexc (vector (0x1b8), scca_special_asm);
		
# undef vector
	}
	
}

DEVDRV * _cdecl
init_xdd (struct kerinfo *k)
{
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
	
	if (!loops_per_sec_ptr)
	{
		c_conws (MSG_MINT);
		goto failure;
	}
	
	if ((s_system (S_GETCOOKIE, COOKIE__MCH, (long) &mch) != 0)
		|| ((mch != MEGASTE) && (mch != TT) && (mch != FALCON)))
	{
		c_conws (MSG_MACHINE);
		goto failure;
	}
	
	init_scc ();
	
	for (i = 0; i < IOVAR_MAX; i++)
	{
		if (IOVARS (i))
		{
			char name [64];
			
			ksprintf (name, "u:\\dev\\%s", (i == 0) ? RSVF_SCCB : (i == 2) ? RSVF_SCCA_1 : RSVF_SCCA_2);
			
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
					ksprintf (name, "%s", (i == 0) ? RSVF_SCCB : (i == 2) ? RSVF_SCCA_1 : RSVF_SCCA_2);
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
wr_scc (IOVAR *iovar, SCC *regs)
{
	DEBUG_I (("wr_scc: %lx", regs));
	
	if (!(ZS_READ_0 (regs) & TXBE))
		return;
	
	if (iovar->sendnow)
	{
		ZS_WRITE_DATA (regs, iovar->sendnow);
		iovar->sendnow = 0;
	}
	else
	{
		/* is somebody ready to receive data? */
		if (iovar->shake && (iovar->shake & iovar->state))
		{
			DEBUG_I (("wr_scc: tx disabled - no action"));
		}
		else
		{
			if (iorec_empty (&iovar->output))
			{
				DEBUG_I (("wr_scc: buffer empty"));
				
				/* buffer empty */
				iovar->tx_empty = 1;
				
				txint_off (iovar, regs);
			}
			else
			{
				/* send character from buffer */
				ZS_WRITE_DATA (regs, iorec_get (&iovar->output));
			}
		}
	}
}

static void scc_txempty (void) __asm__("scc_txempty") __attribute((used));
static void scc_txempty (void)
{
	IOVAR *iovar;
	SCC *regs;
	
	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
		: "cc"			/* clobbered */
	);
	
	regs = iovar->regs;
	DEBUG_I (("scc_txempty: %lx", regs));
	
	
	ZS_WRITE_0 (regs, RESINT);
	
	if (iovar->sendnow)
	{
		ZS_WRITE_DATA (regs, iovar->sendnow);
		iovar->sendnow = 0;
	}
	else
	{
		/* is somebody ready to receive data? */
		if (iovar->shake && (iovar->shake & iovar->state))
		{
			DEBUG_I (("scc_txempty: tx disabled - no action"));
		}
		else
		{
			if (iorec_empty (&iovar->output))
			{
				DEBUG_I (("scc_txempty: buffer empty"));
				
				/* buffer empty */
				iovar->tx_empty = 1;
			}
			else
			{
				/* send character from buffer */
				ZS_WRITE_DATA (regs, iorec_get (&iovar->output));
				
				notify_top_half (iovar);
			}
		}
	}
	
	ZS_WRITE_0 (regs, RHIUS);
}

static void scc_rxavail (void) __asm__("scc_rxavail") __attribute__((used));
static void scc_rxavail (void)
{
	IOVAR *iovar;
	SCC *regs;
	register uchar data;
	register ushort sr;
	
	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
		: "cc"			/* clobbered */
	);
	
	sr = splhigh ();
	
	regs = iovar->regs;
	DEBUG_I (("scc_rxavail: %lx", regs));
	
	
	data = ZS_READ_DATA (regs) & iovar->datmask;
	
	if (iovar->shake & SHAKE_SOFT)
	{
		if (data == XOFF)
		{
			/* otherside can't accept more data */
			
			DEBUG_I (("scc_rxavail: XOFF"));
			
			iovar->state |= STATE_SOFT;
			txint_off (iovar, regs);
			
			goto out;
		}
		
		if (data == XON)
		{
			/* otherside ready now */
			
			DEBUG_I (("scc_rxavail: XON"));
			
			iovar->state &= ~STATE_SOFT;
			wr_scc (iovar, regs);
			
			goto out;
		}
	}
	
	if (!iorec_put (&iovar->input, data))
	{
		DEBUG_I (("scc_rxavail: buffer full!"));
	}
	
	if (!iovar->rx_full && (iorec_used (&iovar->input) > iovar->input.high_water))
	{
		DEBUG_I (("scc_rxavail: stop rx"));
		
		iovar->rx_full = 1;
		
		if (iovar->shake & SHAKE_HARD)
		{
			/* set rts off */
			rts_off (iovar, regs);
			
			if (iovar->lan)
				/* on lan: RTS emulation on DTR line */
				dtr_off (iovar, regs);
		}
		
		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XOFF */
			iovar->sendnow = XOFF;
			
			wr_scc (iovar, regs);
		}
	}
	
	notify_top_half (iovar);
	
out:
	ZS_WRITE_0 (regs, RHIUS);
	
	spl (sr);
}

static void scc_stchange (void) asm("scc_stchange") __attribute__((used));
static void scc_stchange (void)
{
	IOVAR *iovar;
	SCC *regs;
	register uchar ctlreg;
	
	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
		: "cc"			/* clobbered */
	);
	
	regs = iovar->regs;
	DEBUG_I (("scc_stchange: %lx", regs));
	
	ctlreg = ZS_READ_0 (regs);
	
	if (ctlreg & CTS)
	{
		/* CTS is on */
		
		iovar->state &= ~STATE_HARD;
		
		if (iovar->shake & SHAKE_HARD)
			wr_scc (iovar, regs);
	}
	else
	{
		/* CTS is off */
		
		iovar->state |= STATE_HARD;
		
		if (iovar->shake & SHAKE_HARD)
			txint_off (iovar, regs);
	}
	
	/* if (!(ctlreg & DCD)) */
	{
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
	}
	
	ZS_WRITE_0 (regs, RESESI);
	ZS_WRITE_0 (regs, RHIUS);
}

static void scc_special (void) __asm__("scc_special") __attribute__((used));
static void scc_special (void)
{
	IOVAR *iovar;
	SCC *regs;
	
	asm volatile
	(
		"move.l %%a0,%0"
		: "=da" (iovar)		/* output register */
		: 			/* input registers */
		: "cc"			/* clobbered */
	);
	
	regs = iovar->regs;
	DEBUG (("scc_special: %lx", (unsigned long)regs));
	
	
	ZS_READ (regs, RR1);
	ZS_READ (regs, RR8);
	
	ZS_WRITE_0 (regs, ERRRES);
	ZS_WRITE_0 (regs, RHIUS);
}

/* Interrupt-Routinen
 * - rufen die eigentlichen C-Routinen auf
 * 
 * HACK: Der IOVAR-Zeiger wird in A0 uebergeben!
 */

/*
 * SCC port A
 */
	
static void scca_txempty_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t"
		 "move.l  %0,%%a0\n\t"
		 "bsr     scc_txempty\n\t"
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t"
		 "rte"
		: 			/* output register */
		: "m"(iovar_scca)  			/* input registers */
		:  			/* input registers */
		 			/* clobbered */
	);
}
	
static void scca_rxavail_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t" \
		 "move.l  %0,%%a0\n\t" \
		 "bsr     scc_rxavail\n\t" \
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t" \
		 "rte"
		: 			/* output register */
		: "m"(iovar_scca)  			/* input registers */
		 			/* clobbered */
	);
}
	
static void scca_stchange_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t" \
		 "move.l  %0,%%a0\n\t" \
		 "bsr     scc_stchange\n\t" \
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t" \
		 "rte"
		: 			/* output register */
		: "m"(iovar_scca)  			/* input registers */
		 			/* clobbered */
	);
}
	
static void scca_special_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t" \
		 "move.l  %0,%%a0\n\t" \
		 "bsr     scc_special\n\t" \
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t" \
		 "rte"
		: 			/* output register */
		: "m"(iovar_scca)  			/* input registers */
		 			/* clobbered */
	);
}
	
/*
 * SCC port B
 */
	
static void sccb_txempty_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t"
		 "move.l  %0,%%a0\n\t"
		 "bsr     scc_txempty\n\t"
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t"
		 "rte"
		: 			/* output register */
		: "m"(iovar_sccb)  			/* input registers */
		:  			/* input registers */
		 			/* clobbered */
	);
}
	
static void sccb_rxavail_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t" \
		 "move.l  %0,%%a0\n\t" \
		 "bsr     scc_rxavail\n\t" \
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t" \
		 "rte"
		: 			/* output register */
		: "m"(iovar_sccb)  			/* input registers */
		 			/* clobbered */
	);
}
	
static void sccb_stchange_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t" \
		 "move.l  %0,%%a0\n\t" \
		 "bsr     scc_stchange\n\t" \
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t" \
		 "rte"
		: 			/* output register */
		: "m"(iovar_sccb)  			/* input registers */
		 			/* clobbered */
	);
}
	
static void sccb_special_asm(void)
{
	asm volatile
	(
		 "movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)\n\t" \
		 "move.l  %0,%%a0\n\t" \
		 "bsr     scc_special\n\t" \
		 "movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2\n\t" \
		 "rte"
		: 			/* output register */
		: "m"(iovar_sccb)  			/* input registers */
		 			/* clobbered */
	);
}
	
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
			DEBUG (("scc.xdd: check_ioevent: waking I/O wait (%lu)", (unsigned long)iovar->iosleepers));
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
	uchar ctlreg;
	
	iovar->stintr = 0;
	
	ctlreg = ZS_READ_0 (iovar->regs);
	
	if (!(iovar->clocal) && !(ctlreg & DCD))
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
			
			DEBUG (("TS_BLIND set, lost carrier (p = %lx)", (unsigned long)p));
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
	register SCC *regs = iovar->regs;
	
	if (ZS_READ_0 (regs) & TXBE)
	{
		ushort sr;
		
		sr = splhigh ();
		wr_scc (iovar, regs);
		spl (sr);
	}
}

static long
ctl_TIOCBAUD (IOVAR *iovar, long *buf)
{
	long speed = *(long *) buf;
	
	if (speed == -1)
	{
		/* current baudrate */
		
		*(long *) buf = iovar->baudrate;
	}
	else if (speed == 0)
	{
		/* clear dtr */
		
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
			SCC *regs = iovar->regs;
			uchar low, high;
			ushort sr;
			
			sr = splhigh ();
			
			ZS_WRITE (regs, WR14, 0);
			
			iovar->wr4 &= ~TAKT_MASK;
			iovar->wr4 |= table [i].clockdivisor;
			ZS_WRITE (regs, WR4, iovar->wr4);
			
			ZS_WRITE (regs, WR11, table [i].clockmode);
			
			low = table [i].timeconst & 0xff;
			high = (table [i].timeconst >> 8) & 0xff;
			
			ZS_WRITE (regs, WR12, low);
			ZS_WRITE (regs, WR13, high);
			ZS_WRITE (regs, WR14, table [i].brgenmode);
			
			spl (sr);
			
			iovar->baudrate = baudrate;
		}
		
		/* always enable dtr */
		top_dtr_on (iovar);
	}
	
	return E_OK;
}

static ushort
ctl_TIOCGFLAGS (IOVAR *iovar)
{
	ushort flags = 0;
	
	switch (iovar->wr5 & TX8BIT)
	{
		case TX5BIT:	flags |= TF_5BIT;	break;
		case TX6BIT:	flags |= TF_6BIT;	break;
		case TX7BIT:	flags |= TF_7BIT;	break;
		case TX8BIT:	flags |= TF_8BIT;	break;
	}
	
	switch (iovar->wr4 & STB_MASK)
	{
		case STB_ASYNC1:	flags |= TF_1STOP;	break;
		case STB_ASYNC15:	flags |= TF_15STOP;	break;
		case STB_ASYNC2:	flags |= TF_2STOP;	break;
	}
	
	if (iovar->wr4 & PARITY_ENABLE)
	{
		if (iovar->wr4 & PARITY_EVEN)	flags |= T_EVENP;
		else				flags |= T_ODDP;
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
	uchar wr3 = iovar->wr3;
	uchar wr4 = iovar->wr4;
	uchar wr5 = iovar->wr5;
	uchar datmask;
	int flag = 0;
	
	wr3 &= ~(TX8BIT << 1);
	wr5 &= ~(TX8BIT     );
	switch (flags & TF_CHARBITS)
	{
		case TF_5BIT:
		{
			wr3 |= (TX5BIT << 1);
			wr5 |= (TX5BIT     );
			datmask = 0x1f;
			break;
		}
		case TF_6BIT:
		{
			wr3 |= (TX6BIT << 1);
			wr5 |= (TX6BIT     );
			datmask = 0x3f;
			break;
		}
		case TF_7BIT:
		{
			wr3 |= (TX7BIT << 1);
			wr5 |= (TX7BIT     );
			datmask = 0x7f;
			break;
		}
		case TF_8BIT:
		{
			wr3 |= (TX8BIT << 1);
			wr5 |= (TX8BIT     );
			datmask = 0xff;
			break;
		}
		default:
		{
			return EBADARG;
			break;
		}
	}
	
	wr4 &= ~STB_MASK;
	switch (flags & TF_STOPBITS)
	{
		case TF_1STOP:	wr4 |= STB_ASYNC1;	break;
		case TF_15STOP:	wr4 |= STB_ASYNC15;	break;
		case TF_2STOP:	wr4 |= STB_ASYNC2;	break;
		default:	return EBADARG;		break;
	}
	
	if (flags & (T_EVENP | T_ODDP))
	{
		if ((flags & (T_EVENP | T_ODDP)) == (T_EVENP | T_ODDP))
			return EBADARG;
		
		/* enable parity */
		wr4 |= PARITY_ENABLE;
		
		/* set even/odd parity */
		if (flags & T_EVENP)	wr4 |= PARITY_EVEN;
		else			wr4 &= ~PARITY_EVEN;
	}
	else
	{
		/* disable parity */
		wr4 &= ~PARITY_ENABLE;
		
		/* even/odd bit ignored in this case */
	}
	
	/* setup in register */
	ZS_WRITE (iovar->regs, WR3, wr3);
	ZS_WRITE (iovar->regs, WR4, wr4);
	ZS_WRITE (iovar->regs, WR5, wr5);
	
	/* save for later */
	iovar->wr3 = wr3;
	iovar->wr4 = wr4;
	iovar->wr5 = wr5;
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
			 */
			if (ZS_READ_0 (iovar->regs) & CTS)
				iovar->state &= ~STATE_HARD;
			else
				iovar->state = STATE_HARD;
			
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
		if (r) ALERT (("scc.xdd: ctl_TIOCSFLAGSB -> set flags failed!"));
		
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
			top_rts_on (iovar);
			
			if (iovar->lan)
				/* on lan: RTS emulation on DTR line */
				top_dtr_on (iovar);
		}
		
		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XON */
			iovar->sendnow = XON;
			
			send_byte (iovar);
		}
		
		/* receiver started */
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
			top_rts_off (iovar);
			
			if (iovar->lan)
				/* on lan: RTS emulation on DTR line */
				top_dtr_off (iovar);
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
		
		/* transmitter started */
		iovar->tx_empty = 0;
		
		send_byte (iovar);
	}
	else
	{
		DEBUG (("tx_start: already started"));
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
		top_txint_off (iovar);
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

/* END start/stop primitives - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN bios emulation - top half */

static long _cdecl
scc_instat (int dev)
{
	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	IOREC *iorec = &iovar->input;
	long used;
	
	used = iorec_used (iorec);
	return (used ? -1 : 0);
}

static long _cdecl
scc_in (int dev)
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
scc_outstat (int dev)
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
scc_out (int dev, int c)
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
scc_rsconf (int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr)
{
	static const ulong baud [16] =
	{
		19200, 9600, 4800, 3600, 2400, 2000, 1800, 1200,
		600, 300, 230400, 115200, 57600, 38400, 153600, 76800
	};
	
	IOVAR *iovar = IOVARS (dev - BDEV_OFFSET);
	ushort modified = 0;
	ushort flags;
	ulong ret = 0;
	
	DEBUG (("scc_rsconf: enter %i, %i", dev, speed));
	
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
	if (iovar->wr5 & SBRK)
		ret |= (0x8L << 8);
	
	return ret;
}

/* END bios emulation - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver routines - top half */

static long _cdecl
scc_open (FILEPTR *f)
{
	ushort dev = f->fc.aux;
	IOVAR *iovar;
	
	DEBUG (("scc_open [%i]: enter (%x)", f->fc.aux, f->flags));
	
	if (dev > IOVAR_REAL_MAX)
		return EACCES;
	
	iovar = IOVARS (dev);
	if (!iovar)
		return EACCES;
	
	if (iovar->open && (iovar->open->fc.aux != dev))
		return EACCES;
	
	if (!iovar->open)
	{
		/* first open */
		
		DEBUG (("scc_open [%i]: first open, %lx (%x, %i)", f->fc.aux, (unsigned long)f, f->flags, f->links));
		
		if (LANDEV (iovar))
		{
			/* select the right port */
			
# define _giselect	((volatile uchar *) 0x00ff8800)
# define _giread	((volatile uchar *) 0x00ff8800)
# define _giwrite	((volatile uchar *) 0x00ff8802)
		
			ushort sr = splhigh ();
			
			if (dev == LANDEV (iovar))
			{
				*_giselect = 14;
				*_giwrite = *_giread & 0x7f;
				
				iovar->lan = 1;
			}
			else
			{
				*_giselect = 14;
				*_giwrite = *_giread | 0x80;
				
				iovar->lan = 0;
			}
			
			spl (sr);
		}
		
		/* assign dtr line */
		top_dtr_on (iovar);
		
		/* start receiver */
		rx_start (iovar);
	}
	else
	{
		DEBUG (("scc_open [%i]: next open, %lx (%x, %i)", f->fc.aux, (unsigned long)f, f->flags, f->links));
		{
			FILEPTR *t = iovar->open;
			
			while (t)
			{
				DEBUG (("  chain t = 0x%lx, t->next = 0x%lx", (unsigned long)t, (unsigned long)t->next));
				DEBUG (("    links = %i, flags = 0x%x", t->links, t->flags));
				
				t = t->next;
			}
		}
		
		if (denyshare (iovar->open, f))
		{
# if DEV_DEBUG > 0
			FILEPTR *t = iovar->open;
			
			DEBUG (("f = 0x%lx, f->next = 0x%lx", (unsigned long)f, (unsigned long)f->next));
			DEBUG (("  links = %i, flags = 0x%x", f->links, f->flags));
			DEBUG (("  pos = %li, devinfo = 0x%lx, dev = 0x%lx", f->pos, f->devinfo, (unsigned long)f->dev));
			DEBUG (("  fc.index = %li, fc.dev = 0x%x, fc.aux = %i", f->fc.index, f->fc.dev, f->fc.aux));
			
			while (t)
			{
				DEBUG (("chain t = 0x%lx, t->next = 0x%lx", (unsigned long)t, (unsigned long)t->next));
				DEBUG (("  links = %i, flags = 0x%x", t->links, t->flags));
				DEBUG (("  pos = %li, devinfo = 0x%lx, dev = 0x%lx", t->pos, t->devinfo, (unsigned long)t->dev));
				DEBUG (("  fc.index = %li, fc.dev = 0x%x, fc.aux = %i", t->fc.index, t->fc.dev, t->fc.aux));
				
				t = t->next;
			}
# endif
			DEBUG (("scc_open: file sharing denied"));
			return EACCES;
		}
	}
	
	f->pos = 0;
	f->next = iovar->open;
	iovar->open = f;
	
	if (dev >= IOVAR_TTY_OFFSET)
		f->flags |= O_TTY;
	else
		/* force nonblocking mode on HSMODEM devices */
		f->flags |= O_NDELAY;
	
	DEBUG (("scc_open: return E_OK (added %lx)", (unsigned long)f));
	return E_OK;
}

static long _cdecl
scc_close (FILEPTR *f, int pid)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	
	DEBUG (("scc_close [%i]: enter for %lx", f->fc.aux, (unsigned long)f));
	
	if ((f->flags & O_LOCK)
	    && ((iovar->lockpid == pid) || (f->links <= 0)))
	{
		DEBUG (("scc_close: remove lock by %i", pid));
		
		f->flags &= ~O_LOCK;
		iovar->lockpid = 0;
		
		/* wake anyone waiting for this lock */
		wake (IO_Q, (long) iovar);
	}
	
	if (f->links <= 0)
	{
		register FILEPTR **temp;
		register long flag = 1;
		
		DEBUG (("scc_close: freeing FILEPTR %lx", (unsigned long)f));
		
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
			ALERT (("scc_close[%d]: remove open FILEPTR fail!", f->fc.aux));
		
		if (!(iovar->open))
		{
			/* stop receiver */
			rx_stop (iovar);
			
			/* clear dtr line */
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
	
	DEBUG (("scc_close: leave ok"));
	return E_OK;
}


/* HSMODEM write/read routines
 * 
 * they never block
 * they don't do any carrier check or so
 */
static long _cdecl
scc_write (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	long done = 0;
	
	DEBUG (("scc_write [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));
	
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
	
	DEBUG (("scc_write: leave (%ld)", done));
	return done;
}

static long _cdecl
scc_read (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	long done = 0;
	
	DEBUG (("scc_read [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));
	
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
	
	DEBUG (("scc_read: leave (%ld)", done));
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
scc_writeb (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	
	DEBUG (("scc_writeb [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));
	
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
			
			DEBUG (("scc_writeb: sleeping I/O wait"));
			
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
	
	DEBUG (("scc_writeb: leave (%ld)", done));
	return done;
}

static void
timerwake (PROC *p, long cond)
{
	wake (IO_Q, cond);
}

static long _cdecl
scc_readb (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	int ndelay = (f->flags & O_NDELAY) || iovar->tty.vtime /* ??? */;
	long done = 0;
	
	DEBUG (("scc_readb [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));
	
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
	
	DEBUG (("scc_readb: leave (%ld)", done));
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
scc_twrite (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	const long *r = (const long *) buf;
	
	DEBUG (("scc_twrite [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));
	
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
			DEBUG (("scc_twrite: sleeping I/O wait"));
			
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
	
	DEBUG (("scc_twrite: leave (%ld)", done));
	return done;
}

static long _cdecl
scc_tread (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	long *r = (long *) buf;
	
	DEBUG (("scc_tread [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));
	
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
			DEBUG (("scc_tread: sleeping I/O wait"));
			
			iovar->iosleepers++;
			sleep (IO_Q, (long) &iovar->tty.state);
			iovar->iosleepers--;
		}
	}
	while (bytes && !ndelay);
	
	if (ndelay && !done)
		done = EWOULDBLOCK;
	
	DEBUG (("scc_tread: leave (%ld)", done));
	return done;
}

static long _cdecl
scc_lseek (FILEPTR *f, long where, int whence)
{
	DEBUG (("scc_lseek [%i]: enter (%ld, %d)", f->fc.aux, where, whence));
	
	/* (terminal) devices are always at position 0 */
	return 0;
}

static long _cdecl
scc_ioctl (FILEPTR *f, int mode, void *buf)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	long r = E_OK;
	
	DEBUG (("scc_ioctl [%i]: (%x, (%c %i), %lx)", f->fc.aux, mode, (char) (mode >> 8), (mode & 0xff), (unsigned long)buf));
	
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
			bytes -= 2;
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
			DEBUG (("scc_ioctl(TIOCIBAUD) to %li", *(long *) buf));
			
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
			DEBUG (("scc_ioctl(TIOCGFLAGS) [%lx]", (unsigned long)buf));
			
			*(ushort *) buf = ctl_TIOCGFLAGS (iovar);
			break;
		}
		case TIOCSFLAGS:
		{
			DEBUG (("scc_ioctl(TIOCSFLAGS) -> %x", *(ushort *) buf));
			
			r = ctl_TIOCSFLAGS (iovar, *(ushort *) buf);
			break;
		}
		case TIONOTSEND:
		case TIOCOUTQ:
		{
			DEBUG (("scc_ioctl(TIOCOUTQ) -> %li", iorec_used (&iovar->output)));
			
			*(long *) buf = iorec_used (&iovar->output);
			break;
		}
		case TIOCSFLAGSB:
		{
			DEBUG (("scc_ioctl(TIOCSFLAGSB) -> %lx, %lx", ((long *) buf)[0],
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
			/*	| TIOCMH_LE  */
				| TIOCMH_DTR
				| TIOCMH_RTS
				| TIOCMH_CTS
				| TIOCMH_CD
			/*	| TIOCMH_RI  */
			/*	| TIOCMH_DSR */
			/*	| TIOCMH_LEI */
			/*	| TIOCMH_TXD */
			/*	| TIOCMH_RXD */
				| TIOCMH_BRK
			/*	| TIOCMH_TER */
			/*	| TIOCMH_RER */
			/*	| TIOCMH_TBE */
			/*	| TIOCMH_RBF */
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
			register uchar reg = ZS_READ_0 (iovar->regs);
			
			/* TIOCMH_LE */
			if (iovar->wr5 & DTR) val |= TIOCMH_DTR;
			if (iovar->wr5 & RTS) val |= TIOCMH_RTS;
			if (reg & CTS) val |= TIOCMH_CTS;
			/* TIOCMH_RI */
			if (reg & DCD) val |= TIOCMH_CD;
			/* TIOCMH_DSR */
			/* TIOCMH_LEI */
			/* TIOCMH_TXD */
			/* TIOCMH_RXD */
			if (reg & BRK) val |= TIOCMH_BRK;
			/* TIOCMH_TER */
			/* TIOCMH_RER */
			/* TIOCMH_TBE */
			/* TIOCMH_RBF */
			
			/* on lan: RTS emulation on DTR line */
			if (iovar->lan && (iovar->wr5 & DTR))
				val |= TIOCMH_RTS;
			
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
				ushort sr;
				
				if (mask & TIOCMH_DTR)
				{
					if (val & TIOCMH_DTR)	iovar->wr5 |= DTR;
					else			iovar->wr5 &= ~DTR;
				}
				
				if (mask & TIOCMH_RTS)
				{
					if (iovar->lan)
					{
						/* on lan: RTS emulation on DTR line */
						if (val & TIOCMH_RTS)	iovar->wr5 |= DTR;
						else			iovar->wr5 &= ~DTR;
					}
					else
					{
						if (val & TIOCMH_RTS)	iovar->wr5 |= RTS;
						else			iovar->wr5 &= ~RTS;
					}
				}
				
				sr = splhigh ();
				ZS_WRITE (iovar->regs, WR5, iovar->wr5);
				spl (sr);
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
			register uchar reg = ZS_READ_0 (iovar->regs);
			
			/* TIOCM_LE */
			
			if (iovar->wr5 & DTR) val |= TIOCM_DTR;
			if (iovar->wr5 & RTS) val |= TIOCM_RTS;
			
			/* TIOCM_ST */
			/* TIOCM_SR */
			
			if (reg & CTS) val |= TIOCM_CTS;
			if (reg & DCD) val |= TIOCM_CD;
			
			/* TIOCM_RI */
			/* TIOCM_DSR */
			
			/* on lan: RTS emulation on DTR line */
			if (iovar->lan && (iovar->wr5 & DTR))
				val |= TIOCMH_RTS;
			
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
				{
					DEBUG (("scc_ioctl: sleep in SETLKW"));
					sleep (IO_Q, (long) iovar);
				}
				else
					return ELOCKED;
			}
			
			if (lck->l_type == F_UNLCK)
			{
				if (!(f->flags & O_LOCK))
				{
					DEBUG (("scc_ioctl: wrong file descriptor for UNLCK"));
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
	
	DEBUG (("scc_ioctl: return %li", r));
	return r;
}

static long _cdecl
scc_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	DEBUG (("scc_datime [%i]: enter (%i)", f->fc.aux, rwflag));
	
	if (rwflag)
		return EACCES;
	
	*timeptr++ = timestamp;
	*timeptr = datestamp;
	
	return E_OK;
}

static long _cdecl
scc_select (FILEPTR *f, long proc, int mode)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	struct tty *tty = (struct tty *) f->devinfo;
	
	DEBUG (("scc_select [%i]: enter (%li, %i, %lx)", f->fc.aux, proc, mode, (unsigned long)tty));
	
	if (mode == O_RDONLY)
	{
		if (!iorec_empty (&iovar->input))
		{
			TRACE (("scc_select: data present for device %lx", (unsigned long)iovar));
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
			TRACE (("scc_select: ready to output on %lx", (unsigned long)iovar));
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
scc_unselect (FILEPTR *f, long proc, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;
	
	DEBUG (("scc_unselect [%i]: enter (%li, %i, %lx)", f->fc.aux, (unsigned long)proc, mode, (unsigned long)tty));
	
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
