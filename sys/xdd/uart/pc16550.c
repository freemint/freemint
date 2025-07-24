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
 * Started: 1999-07-28
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * 2001-01-16:	(v0.64)
 * 
 * - fix: bugfix in the file locking code, lockpid was reset to wrong value
 * 
 * 2000-05-24:	(v0.63)
 * 
 * - new: hopefully better behaviour on last close()
 *        (flush buffers, go back to carrier detection & update TS_BLIND)
 * 
 * 2000-05-16:	(v0.62)
 * 
 * - fix: removed unnecessary debug messages that are printed
 * 
 * 2000-05-14:	(v0.61)
 * 
 * - fix: corrected TS_BLIND <-> clocal handling; now hopefully correct
 * - new: bios emulation doesn't poll now, use sleep/wake mechanism
 *        like the read/write functions
 * 
 * 2000-05-12:	(v0.60)
 * 
 * - new: reworked the complete high level I/O routines;
 *        support now (hopefully) true tty functionality
 *        this include carrier handling, vmin/vtime handling,
 *        clocal settings and so on
 * - new: seperate read/write routines for HSMODEM devices;
 *        they never block
 * 
 * 2000-05-09:	(v0.52)
 * 
 * - fix: uart_close doesn't removed device locks
 * 
 * 2000-05-04:	(v0.51)
 * 
 * - new: BIOS output functions never produce debug output
 * 
 * 2000-05-04:	(v0.50)
 * 
 * - fix: last change was not correct, used wrong intr numbers
 * - fix: use correct BIOS device numbers
 * 
 * 2000-05-03:	(v0.49)
 * 
 * - fix: swapped UART1 and UART2 again; hope it match now the HSMODEM defaults
 * 
 * 2000-02-28:	(v0.48)
 * 
 * - new: added CD loss detection -> SIGHUP
 * 
 * 2000-02-20:	(v0.47)
 * 
 * - new: changes related to the final naming scheme
 * - fix: use correct BIOS device for remapping
 * 
 * 2000-01-19:	(v0.46)
 * 
 * - new: optimized IOREC handling a little bit
 * - new: added 76800 baud setting and removed 14400 and 28800 as they are
 *        useless
 * - new: restructured select handling, now in an effective and
 *        CPU friendly way through timeouts
 * - new: better automatic DTR handling
 * - new: added datmask to mask out unsed bits in non 8bit modes
 * - new: added HSMODEM ioctl TIOCSTART
 * 
 * 1999-12-15:	(v0.45)
 * 
 * - new: added new RSVF kernel support
 *        this driver require now the latest FreeMiNT kernel (> 1.15.5)
 *        removed all rsvf stuff (as it's in the kernel now)
 * 
 * 1999-12-15:	(v0.44)
 * 
 * - new: software handshake work now also correct
 * - new: rewritten completly top & bottom half synchronization
 *        it's now hopefully almost ok :-) without any blocking
 *        in the top half and without any negative interaction
 * - new: completed BIOS emulation code (rsconf)
 * 
 * 1999-12-14:	(v0.43)
 * 
 * - new: CTS handling completly rewritten
 *        lot of code reorganisation and movement
 * 
 * 1999-12-01:	(v0.42)
 * 
 * - fix: added CTS checking in the write interrupt routine
 * - fix: forced HSMODEM devices to be opened in non blocking mode
 * - new: added BIOS emulation code
 * - new: support new DEV_INSTALL2 opcode for BIOS remap
 * 
 * 1999-10-04:	(v0.41)
 * 
 * - new: changed to new errno header
 * - new: changed base name of the tty devices from tty to ttyS
 * 
 * 1999-08-21:	(v0.40)
 * 
 * - new: added MiNT version and Milan check
 * - new: added tty tread & twrite functions, ttys work now
 * - new: added automatic tty install (init, open)
 * - new: check & evaluate file sharing modes (open, close)
 * - new: wakeup sleeping processes on close
 * - fix: wrong return value in read & write for N_DELAY
 * 
 * 1999-08-14:	(v0.30b)
 * 
 * - new: optimized read data interrupt routine
 * - new: return EWOULDBLOCK in read & write if N_DELAY is set
 * 
 * 1999-08-09:	(v0.20b)
 * 
 * - inital revision
 * 
 * 
 * bugs/todo:
 * 
 * - tty BREAK handling missing
 * 
 * 
 * UART BIOS and XBIOS Routinen for Milan.
 * Rainer Mannigel, Michael Schwingen
 * 
 * We now support lots (currently 10) of UARTs, and interrupt sharing
 * between them. We install one interrupt handler for each physical
 * interrupt line that belongs to one or more UARTs, and that gets the
 * pointer to the first UART's iovar in its corresponding intr_iovar. The
 * iovars are chained together by the next field, so the interrupt handler
 * services one UART at a time as long as there are events left to be
 * handled and then goes on to the next UART.
 * 
 * Due to the braindamaged design using Bconmap() to access more than one
 * serial port, we do not get a useable device number in the BIOS calls
 * (Bconin/out/stat/ostat), which means that we have to provide one entry
 * point for each of these routines (and for Rsconf, which gets no device
 * numbger at all) for every port we want to handle - this is why there is a 
 * statical limit - these routines are in stubs.s
 */

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/signal.h"
# include "mint/ssystem.h"
# include "mint/stat.h"
# include "cookie.h"

# include <mint/osbind.h>
# include "pc16550.h"


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	64
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

/* onboard ports */
# define UART1		0x03f8L
# define UART1_INTR	4
# define UART2		0x02f8L
# define UART2_INTR	3

# define RSVF_UART1	"modem2"
# define RSVF_UART2	"serial2"

# define TTY_UART1	"ttyS1"
# define TTY_UART2	"ttyS3"

/* additional ports */
# define RSVF_BASENAME	"serial"
# define TTY_BASENAME	"ttyS"

# define RSVF_UARTx	3
# define TTY_UARTx	6

/* BIOS device mappings */
# define BDEV_START	6
# define BDEV_OFFSET	7

# define BDEV_UART1	7
# define BDEV_UART2	8
# define BDEV_UARTx	9


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


# define MAX_PORTS	10
# define MAX_INTS	 5			/* 2 onboard + 3 ISA cards */


/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Milan UART serial driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\275 1998, 1999 by Rainer Mannigel, Michael Schwingen.\r\n" \
	"\275 2000-2010 by Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_MILAN	\
	"\033pThis driver requires a Milan!\033q\r\n"

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
	UART	*regs;		/* UART register base address */
	
	volatile ushort	state;	/* receiver state */
# define STATE_HARD		0x1		/* CTS is low */
# define STATE_SOFT		0x2		/* XOFF is set */
	
	ushort	shake;		/* handshake mode */
# define SHAKE_HARD		STATE_HARD	/* RTS/CTS handshake enabled */
# define SHAKE_SOFT		STATE_SOFT	/* XON/XOFF handshake enabled */
	
	uchar	rx_full;	/* input buffer full */
	uchar	tx_empty;	/* output buffer empty */
	
	uchar	sendnow;	/* one byte of control data, bypasses buffer */
	uchar	datmask;	/* AND mask for read from sccdat */
	
	uchar	iointr;		/* I/O interrupt */
	uchar	stintr;		/* status interrupt */
	ushort	res;		/* padding */
	
	IOREC	input;		/* input buffer */
	IOREC	output;		/* output buffer */
	
	IOVAR	*next;		/* for Interrupt-Sharing */
	long	baudbase;	/* crystal frequency / 16 */
	long	baudrate;	/* current baud rate value */
	
	ushort	intr;		/* intr number */
	
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
 * locking functions - bottom/top half
 */
INLINE ushort	splhigh		(void);
INLINE void	splx		(ushort old_sr);


/*
 * inline assembler primitives - bottom half
 */
INLINE void	dtr_off		(UART *regs);
INLINE void	dtr_on		(UART *regs);
INLINE void	rts_off		(UART *regs);
INLINE void	rts_on		(UART *regs);
INLINE void	brk_off		(UART *regs);
INLINE void	brk_on		(UART *regs);
INLINE uchar	intrs_off	(UART *regs);
INLINE void	intrs_on	(UART *regs, uchar mask);
INLINE void	txint_off	(UART *regs);
INLINE void	txint_on	(UART *regs);


/*
 * inline assembler primitives - top half
 */
INLINE void	top_rts_on	(IOVAR *iovar);
INLINE void	top_rts_off	(IOVAR *iovar);
INLINE void	top_dtr_on	(IOVAR *iovar);
INLINE void	top_dtr_off	(IOVAR *iovar);
INLINE void	top_brk_on	(IOVAR *iovar);
INLINE void	top_brk_off	(IOVAR *iovar);
INLINE uchar	top_intrs_off	(IOVAR *iovar);
INLINE void	top_intrs_on	(IOVAR *iovar, uchar mask);
INLINE void	top_txint_off	(IOVAR *iovar);
INLINE void	top_txint_on	(IOVAR *iovar);


/*
 * initialization - top half
 */
INLINE int	detect_uart	(UART *regs, ulong *baudbase);
INLINE int	init_uart	(IOVAR **iovar, ushort base, int intr, long baudbase);
INLINE void	init_pc16550	(void);
INLINE void	reset_uart	(IOVAR *iovar);


/*
 * interrupt handling - bottom half
 */
INLINE void	notify_top_half	(IOVAR *iovar);
INLINE void	pc16550_read_x	(IOVAR *iovar, UART *regs);
INLINE void	pc16550_read_o	(IOVAR *iovar, UART *regs);
INLINE void	pc16550_read	(IOVAR *iovar, UART *regs);
INLINE void	pc16550_write	(IOVAR *iovar, UART *regs);

static void	pc16550_int0	(void);
static void	pc16550_int1	(void);
static void	pc16550_int2	(void);
static void	pc16550_int3	(void);
static void	pc16550_int4	(void);


/*
 * interrupt handling - top half
 */
static void	check_ioevent	(PROC *p, long arg);
static void	soft_cdchange	(PROC *p, long arg);


/*
 * control functions - top half
 */
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
static long _cdecl	uart_instat	(int dev);
static long _cdecl	uart_in		(int dev);
static long _cdecl	uart_outstat	(int dev);
static long _cdecl	uart_out	(int dev, int c);
static long _cdecl	uart_rsconf	(int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr);


/*
 * device driver routines - top half
 */
static long _cdecl	uart_open	(FILEPTR *f);
static long _cdecl	uart_write	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	uart_read	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	uart_writeb	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	uart_readb	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	uart_twrite	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	uart_tread	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	uart_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl	uart_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl	uart_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	uart_close	(FILEPTR *f, int pid);
static long _cdecl	uart_select	(FILEPTR *f, long proc, int mode);
static void _cdecl	uart_unselect	(FILEPTR *f, long proc, int mode);


/*
 * device driver maps
 */
static BDEVMAP bios_devtab =
{
	uart_instat, uart_in,
	uart_outstat, uart_out,
	uart_rsconf
};

static DEVDRV hsmodem_devtab =
{
	uart_open,
	uart_write, uart_read, uart_lseek, uart_ioctl, uart_datime,
	uart_close,
	uart_select, uart_unselect,
	NULL, NULL
};

static DEVDRV tty_devtab =
{
	uart_open,
	uart_twrite, uart_tread, uart_lseek, uart_ioctl, uart_datime,
	uart_close,
	uart_select, uart_unselect,
	uart_writeb, uart_readb
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

/*
 * global data structures
 */

# define IOVAR_MAX	MAX_PORTS

static IOVAR *iovar_tab [IOVAR_MAX * 2];

# define IOVARS(nr)		(iovar_tab [nr])
# define IOVAR_TTY_OFFSET	(IOVAR_MAX)
# define IOVAR_REAL_MAX		(IOVAR_MAX * 2)

static IOVAR *iovar_bdev [IOVAR_MAX + BDEV_UARTx - BDEV_OFFSET];

# define IOVARS_BDEV(bdev)	(iovar_bdev [bdev - BDEV_OFFSET])


/*
 * interrupt data structures
 */

/* ptr to first UART struct for each interrupt handler
 */
static IOVAR *intr_iovar [MAX_INTS];

/* ptr to interrupt handler routine, used for Setexc
 */
static void (*intr_handler [MAX_INTS])(void) =
{
	pc16550_int0,
	pc16550_int1,
	pc16550_int2,
	pc16550_int3,
	pc16550_int4
};

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
dtr_off (UART *regs)
{
	/* regs->mcr &= ~DTRO; */
	asm volatile
	(
		"bclr.b #0,4(%0)" 	/* delete DTRO in MCR */
		:
		: "a" (regs)
		: "cc"
	);
	DEBUG_I (("PC16550 DTR off"));
}

INLINE void
dtr_on (UART *regs)
{
	/* regs->mcr |= DTRO; */
	asm volatile
	(
		"bset.b #0,4(%0)" 	/* set DTRO in MCR */
		:
		: "a" (regs)
		: "cc"
	);
	DEBUG_I (("PC16550 DTR on"));
}


INLINE void
rts_off (UART *regs)
{
	/* regs->mcr &= ~RTSO; */
	asm volatile
	(
		"bclr.b #1,4(%0)" 	/* delete RTSO in MCR */
		:
		: "a" (regs)
		: "cc"
	);
	DEBUG_I (("PC16550 RTS off"));
}

INLINE void
rts_on (UART *regs)
{
	/* regs->mcr |= RTSO; */
	asm volatile
	(
		"bset.b #1,4(%0)" 	/* set RTSO in MCR */
		:
		: "a" (regs)
		: "cc"
	);
	DEBUG_I (("PC16550 RTS on"));
}


INLINE void
brk_off (UART *regs)
{
	regs->lcr &= ~SBRK;
	DEBUG_I (("PC16550 BRK off"));
}

INLINE void
brk_on (UART *regs)
{
	regs->lcr |= SBRK;
	DEBUG_I (("PC16550 BRK on"));
}


INLINE uchar
intrs_off (UART *regs)
{
	register ushort sr;
	register uchar oldmask;
	
	sr = splhigh ();
	
	/* save old interrupts */
	oldmask = regs->ier;
	
	/* and lock */ 
	regs->ier = 0;
	
	splx (sr);
	
	DEBUG_I (("PC16550 intrs off"));
	return oldmask;
}

INLINE void
intrs_on (UART *regs, uchar mask)
{
	/* set new mask */ 
	regs->ier = mask;
	DEBUG_I (("PC16550 intrs on"));
}


INLINE void
txint_off (UART *regs)
{
	/* regs->ier &= ~TXLDL_IE; */
	asm volatile
	(
		"bclr.b #1,1(%0)" 	/* delete TXLDL_IE in IER */
		:
		: "a" (regs)
		: "cc"
	);
	DEBUG_I (("PC16550 TXINT off"));
}

INLINE void
txint_on (UART *regs)
{
	/* regs->ier |= TXLDL_IE; */
	asm volatile
	(
		"bset.b #1,1(%0)" 	/* set TXLDL_IE in IER */
		:
		: "a" (regs)
		: "cc"
	);
	DEBUG_I (("PC16550 TXINT on"));
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
	rts_on (iovar->regs);
	splx (sr);
}

INLINE void
top_rts_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	rts_off (iovar->regs);
	splx (sr);
}

INLINE void
top_dtr_on (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	dtr_on (iovar->regs);
	splx (sr);
}

INLINE void
top_dtr_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	dtr_off (iovar->regs);
	splx (sr);
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

INLINE uchar
top_intrs_off (IOVAR *iovar)
{
	register uchar oldmask;
	register ushort sr;
	
	sr = splhigh ();
	oldmask = intrs_off (iovar->regs);
	splx (sr);
	
	return oldmask;
}

INLINE void
top_intrs_on (IOVAR *iovar, uchar mask)
{
	ushort sr;
	
	sr = splhigh ();
	intrs_on (iovar->regs, mask);
	splx (sr);
}

INLINE void
top_txint_off (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	txint_off (iovar->regs);
	splx (sr);
}

INLINE void
top_txint_on (IOVAR *iovar)
{
	ushort sr;
	
	sr = splhigh ();
	txint_on (iovar->regs);
	splx (sr);
}

/* END inline assembler primitives - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization - top half */

/*
 * Autodetect a UART.
 * 
 * This routine first does some sanity checks to be sure that there is a
 * UART at the diven address. It then enables the interrupt and toggles the
 * IER to generate an interrupt, to autodetect the interrupt line. The
 * interrupt driver is disabled after this, which means that multiple UARTs
 * using the same interrupt will be detected OK even if the hardware does
 * not support sharing the interrupt line once all UARTs are enabled later.
 * The last thing is to measure the BRG crystal by transmitting some
 * characters (in loopback mode) with disabled FIFOs and measuring the time
 * needed by using MFP timer A.
 * 
 * We time sending 18 characters (or 180 bits) at the highest possible speed
 * (BRG divide by 1, or 115200 bps at 1.8432MHz). At a MFP clock of
 * 2.4576MHz and a :16 prescaler, this gives the following counted values:
 * 
 * Crystal (MHz)   count    BRG=1 rate
 * 1.8432          240      115200
 * 3.6864          120      230400
 * 7.3728           60      460800
 * 
 * 4.0             111      250000 (for MIDI cards)
 * 6.0              74      375000 (for MIDI cards)
 * 
 */

/* only in one place used -> INLINE
 */
INLINE int
detect_uart (UART *regs, ulong *baudbase)
{
	ulong rate = 115200;
	int intr = 0;
	int i;
	
	
	*baudbase = 0L;
	
	/* read LSR */ 
	(void) regs->lsr;
	/* now LSR may not be $FF! */
	if (regs->lsr == 0xff)
		return 0;
	
	regs->scr = 0x55;
	if (regs->scr != 0x55)
		return 0;
	
	regs->scr = 0xAA;
	if (regs->scr != 0xAA)
		return 0;
	
	/* set DLAB */
	regs->lcr = 0x83;
	
	/* set baudrate */
	regs->dlm = (115200 / rate) >> 8;
	regs->dll = (115200 / rate) & 0xFF;
	
	/* 8N1 */
	regs->lcr = 0x03;
	regs->ier = 0;
	
	/* Reset FIFOs */
	regs->fcr = RXFR | TXFR;
	/* Disable FIFOs */
	regs->fcr = 0;
	
	/* enable loopback */
	regs->mcr = LOOP;
	
	/* check loopback */
	if ((regs->msr & 0xf0) != 0)
		return 0;
	
	/* enable loopback */
	regs->mcr = LOOP | 0x0f;
	
	/* check loopback */
	if ((regs->msr & 0xf0) != 0xf0)
		return 0;
	
	DEBUG (("uart: on %lx seems to be a 16550, check intr", regs));
	
# if 0
	int_autoprobe_start ();
	
	regs->mcr = GI_EN;
	
	/* this will create enough INTs */
	regs->ier = 0x00;
	regs->ier = 0x0f;
	regs->ier = 0x00;
	regs->ier = 0x0f;
	
	/* short delay */
	for (i = 255; i >= 0; i--)
		(void) regs->rbr;
	
	regs->ier = 0;
	regs->mcr = 0;
	
	intr = int_autoprobe_end ();
# else
	if ((UART *) (UART1 + 0xc0000000L) == regs)
	{
		/* builtin port 1 */
		intr = UART1_INTR;
	}
	else if ((UART *) (UART2 + 0xc0000000L) == regs)
	{
		/* builtin port 2 */
		intr = UART2_INTR;
	}
	else
	{
		/* no detection available yet */
		intr = 0;
	}
# endif
	
	if (intr > 0)
	{
		volatile uchar *tadr = (volatile uchar *) 0xffffc103L + 15 * 4;
		volatile uchar *tacr = (volatile uchar *) 0xffffc103L + 12 * 4;
		
		ushort sr;
		short time;
		
		
		sr = splhigh ();
		
		/* enable loopback */
		regs->mcr = LOOP; 
		
		*tacr = 0;
		*tadr = 0xff;
		while (*tadr != 0xff)
			;
		
		for (i = 0; i < 19; i++)
		{
			/* send 1 char */
			regs->thr = 0x00;
			
			/* wait until done */
			while ((regs->lsr & TXDE) == 0)
				;
			
			if (i == 0)
				*tacr = 3;
		}
		
		/* stop timer */
		*tacr = 0;
		time = 255 - *tadr;
		
		/* get Rx data */
		while ((regs->lsr & RXDA))
			(void) regs->rbr;
		
		/* disable loopback */
		regs->mcr = GI_EN;
		(void) regs->iir;
		(void) regs->msr;
		
		splx (sr);
		
		
		if      (time > 230 && time <= 250)	*baudbase = 115200L;
		else if (time > 115 && time <= 125)	*baudbase = 230400L;
		else if (time >  55 && time <=  65)	*baudbase = 460800L;
		else if (time > 105 && time <= 115)	*baudbase = 250000L;
		else if (time >  70 && time <=  80)	*baudbase = 375000L;
		
		DEBUG (("detect_clock: t = %d, baudbase = %lu", time, *baudbase));
	}
	
	/* Reset FIFOs */
	regs->fcr = FIFO_EN | RXFR | TXFR;
	regs->fcr = FIFO_EN | RXFTH_8;
	
	if ((regs->iir & 0xc0) != 0xc0)
	{
		DEBUG (("no 16550A at %lx, INT %i", regs, intr));
		return 0;
	}
	
	DEBUG (("detected 16550A at %lx, INT %i", regs, intr));
	return intr;
}

/* only in one place used -> INLINE
 */
INLINE int
init_uart (IOVAR **iovar, ushort base, int intr, long baudbase)
{
	char *buffer;
	
	*iovar = kmalloc (sizeof (**iovar));
	if (!*iovar)
		goto error;
	
	bzero (*iovar, sizeof (**iovar));
	
	buffer = kmalloc (2 * IOBUFSIZE);
	if (!buffer)
		goto error;
	
	(*iovar)->input.buffer = (unsigned char *)buffer;
	(*iovar)->output.buffer = (unsigned char *)buffer + IOBUFSIZE;
	
	(*iovar)->input.low_water = (*iovar)->output.low_water = 1 * (IOBUFSIZE / 4);
	(*iovar)->input.high_water = (*iovar)->output.high_water = 3 * (IOBUFSIZE / 4);
	
	(*iovar)->regs = (UART *) (0xC0000000L | base);
	(*iovar)->baudbase = baudbase;
	(*iovar)->intr = intr;
	(*iovar)->lockpid = 0;
	
	return 1;
	
error:
	if (*iovar)
		kfree (*iovar);
	
	ALERT (("uart.xdd: kmalloc(%li, %li) fail, out of memory?", sizeof (**iovar), IOBUFSIZE));
	return 0;
}

INLINE void
reset_uart (IOVAR *iovar)
{
	UART *regs = iovar->regs;
	long div;
	
	regs->mcr = 0;
	regs->ier = 0;
	
	iovar->baudrate = 9600;
	div = iovar->baudbase / iovar->baudrate;
	
	regs->lcr |= BNKSE;
	regs->dll = (div     ) & 0xFF;
	regs->dlm = (div >> 8) & 0xFF;
	regs->lcr &= ~BNKSE;
	
	/* Reset FIFOs */
	regs->fcr = FIFO_EN | RXFR | TXFR;
	/* Enable FIFOs, set Rcv Threshold */
	regs->fcr = FIFO_EN | RXFTH_8;
	
	/* 8bit char, 1 stopbit, no parity */
	regs->lcr = 3;
	
	/* soft line always on */
	/* why??? iovar->state = STATE_SOFT; */
	
	/* read current CTS state */
	if (!(regs->msr & CTSI))
		/* CTS is off */
		iovar->state |= STATE_HARD;
	
	/* RTS/CTS handshake */
	iovar->shake = SHAKE_HARD;
	
	/* read current CD state */
	if (!(regs->msr & DCDI))
		/* CD is off, no carrier */
		iovar->tty.state |= TS_BLIND;
	
	iovar->rx_full = 1;	/* receiver disabled */
	iovar->tx_empty = 1;	/* transmitter empty */
	
	iovar->sendnow = 0;
	iovar->datmask = 0xff;
	
	iovar->iointr = 0;
	iovar->stintr = 0;
	iovar->res = 0;
	
	iovar->input.head = iovar->input.tail = 0;
	iovar->output.head = iovar->output.tail = 0;
	
	iovar->clocal = 0;
	iovar->brkint = 1;
	
	/* enable interrupts */
	regs->ier = RXHDL_IE | LS_IE | MS_IE;
	
	/* global interrupt enable */
	regs->mcr |= GI_EN;
}

/* only in one place used -> INLINE
 */
INLINE void
init_pc16550 (void)
{
# define NUM_BASES 12
	ushort bases [NUM_BASES] =
	{
		UART1,   UART2,				/* on-board UART1 & UART2 */
		0x03e8L, 0x02e8L,
		0x02a0L, 0x02a8L, 0x02b0, 0x02b8,	/* first AST fourport */
		0x01a0L, 0x01a8L, 0x01b0, 0x01b8	/* second AST fourport */ 
	};
	ulong baudbase[NUM_BASES];
	int intr[NUM_BASES];
	
	int i, j;
	ushort sr;
	
	int current_iovar = 0;
	int intr_num = 0;
	
	
	for (i = 0; i < NUM_BASES; i++)
		intr[i] = detect_uart ((UART *) (bases[i] + 0xc0000000L), &baudbase[i]);
	
	DEBUG (("UARTS found:"));
	
	sr = splhigh ();
	
	for (i = 0, j = 0; i < NUM_BASES; i++)
	{
		if (intr[i] && baudbase[i] && current_iovar < IOVAR_MAX)
		{
			DEBUG (("base %x, intr %i, baudbase %lu", bases[i], intr[i], baudbase[i]));
			
			if (init_uart (&(IOVARS (current_iovar)), bases[i], intr[i], baudbase[i]))
			{
				switch (bases[i])
				{
					case UART1:
					{
						IOVARS (current_iovar)->bdev = BDEV_UART1;
						iovar_bdev[BDEV_UART1-BDEV_OFFSET] = IOVARS (current_iovar);
						
						strcpy (IOVARS (current_iovar)->rsvf_name, RSVF_UART1);
						strcpy (IOVARS (current_iovar)->tty_name, TTY_UART1);
						
						break;
					}
					case UART2:
					{
						IOVARS (current_iovar)->bdev = BDEV_UART2;
						iovar_bdev[BDEV_UART2-BDEV_OFFSET] = IOVARS (current_iovar);
						
						strcpy (IOVARS (current_iovar)->rsvf_name, RSVF_UART2);
						strcpy (IOVARS (current_iovar)->tty_name, TTY_UART2);
						
						break;
					}
					default:
					{
						IOVARS (current_iovar)->bdev = BDEV_UARTx+j;
						iovar_bdev[BDEV_UARTx+j-BDEV_OFFSET] = IOVARS (current_iovar);
						
						ksprintf (IOVARS (current_iovar)->rsvf_name, RSVF_BASENAME "%i", (int) (RSVF_UARTx + j));
						ksprintf (IOVARS (current_iovar)->tty_name, TTY_BASENAME "%i", (int) (TTY_UARTx + j));
						
						break;
					}
				}
				
				current_iovar++;
			}
		}
	}
	
	/* install interrupt handlers
	 * chain iovars into a linked list per shared interrupt
	 */
	for (i = 0; i < current_iovar; i++)
	{
		int chain_int = 0;
		
		DEBUG (("installing INT for UART %d:",i));
		
		for (j = 0; j < i; j++)
			if (IOVARS (i)->intr == IOVARS (j)->intr)
				chain_int = j;
		
		if (chain_int)
		{
			DEBUG (("chaining iovar %d to iovar %d", i, chain_int));
			
			IOVARS (chain_int)->next = IOVARS (i);
			/* set_int_levelmode (IOVARS (i)->intr); */
		}
		else if (intr_num < MAX_INTS)
		{
			DEBUG (("allocating new int handler %d to iovar %d, INT %d", intr_num, i, IOVARS (i)->intr));
			
			intr_iovar[intr_num] = IOVARS (i);
			(void) Setexc (80 + IOVARS (i)->intr, intr_handler[intr_num]);
			DEBUG (("old int handler = %lx", old));
			
			intr_num++;
		}
		else
			ALERT (("ERROR: no free int handler for UART using int %d", IOVARS (i)->intr));
	}
	
	for (i = 0; i < IOVAR_MAX && IOVARS (i); i++)
		reset_uart (IOVARS (i));
	
	splx (sr);
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
	
	if ((s_system (S_GETCOOKIE, COOKIE__MCH, (long) &mch) != 0)
		|| (mch != MILAN_C))
	{
		c_conws (MSG_MILAN);
		goto failure;
	}
	
	init_pc16550 ();
	
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

/*
 * pc16550_read(): uart receive buffer full.
 * Holt Zeichen vom UART und schreibt es in den Puffer.
 * XON/XOFF- oder RTS/CTS-Mode wird beachtet.
 * 
 * only called from pc16550_int
 */

INLINE void
pc16550_read_x (IOVAR *iovar, UART *regs)
{
	while (regs->lsr & RXDA)
	{
		register uchar data = regs->rbr & iovar->datmask;
		
		if (data == XOFF)
		{
			/* otherside can't accept more data */
			
			DEBUG_I (("pc16550_read_x: XOFF"));
			
			iovar->state |= STATE_SOFT;
			txint_off (iovar->regs);
		}
		else if (data == XON)
		{
			/* otherside ready now */
			
			DEBUG_I (("pc16550_read_x: XON"));
			
			iovar->state &= ~STATE_SOFT;
			txint_on (iovar->regs);
		}
		else
		{
			if (!iorec_put (&iovar->input, data))
			{
				DEBUG_I (("pc16550_read_x: buffer full!"));
			}
		}
	}
}

INLINE void
pc16550_read_o (IOVAR *iovar, UART *regs)
{
	while (regs->lsr & RXDA)
	{
		register uchar data = regs->rbr & iovar->datmask;
		
		if (!iorec_put (&iovar->input, data))
		{
			DEBUG_I (("pc16550_read_o: buffer full!"));
		}
	}
}

INLINE void
pc16550_read (IOVAR *iovar, UART *regs)
{
	if (iovar->shake & SHAKE_SOFT)	pc16550_read_x (iovar, regs);
	else				pc16550_read_o (iovar, regs);
	
	/* test the free space
	 */
	if (!iovar->rx_full && (iorec_used (&iovar->input) > iovar->input.high_water))
	{
		DEBUG_I (("pc16550_read: stop rx"));
		
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
			
			/* enable interrupt */
			txint_on (regs);
		}
	}
	
	notify_top_half (iovar);
}

/*
 * pc16550_write(): 
 * 
 * only called from pc16550_int
 */
INLINE void
pc16550_write (IOVAR *iovar, UART *regs)
{
	register int count = 16;
	
	if (iovar->sendnow)
	{
		/* control byte
		 */
		
		DEBUG_I (("pc16550_write: send control %i", iovar->sendnow));
		
		regs->thr = iovar->sendnow;
		iovar->sendnow = 0;
		
		count--;
	}
	
	/* real data
	 */
	
	/* is somebody ready to receive data? */
	if (iovar->shake && (iovar->shake & iovar->state))
	{
		DEBUG_I (("pc16550_write: tx disabled - INT turned off, ier=$%x", regs->ier));
		
		/* disable interrupt */
		txint_off (regs);
	}
	else
	{
		while (count--)
		{
			if (iorec_empty (&iovar->output))
			{
				DEBUG_I (("pc16550_write: buffer empty, ier=$%x", regs->ier));
				
				/* buffer empty */
				iovar->tx_empty = 1;
				
				/* disable interrupt */
				txint_off (regs);
				
				break;
			}
			
			/* send character from buffer */
			regs->thr = iorec_get (&iovar->output);
		}
		
		notify_top_half (iovar);
	}
}

/*
 * Der eigentliche Interrupthandler. Wird vom Assemblerteil aufgerufen.
 * HACK: Der IOVAR-Zeiger wird in A0 uebergeben!
 */
static void
pc16550_int (IOVAR *iovar)
{
	UART *regs;
	ushort sr;
	uchar event;
	
next_uart:
	
	regs = iovar->regs;
	DEBUG_I (("pc16550_int: handling UART at %lx", regs));
	
	sr = splhigh ();
	
	while (!((event = (regs->iir & 0x0f)) & 1))
	{
		/* Int pending */
		
		DEBUG_I (("pc16550_int: event %d ", event));
		
		switch (event)
		{
			case 6:		/* Highest priority */
			{
				/* Parity error, framing error, data overrun
				 * or break event
				 */
				if (regs->lsr & OE)
					pc16550_read (iovar, regs);
				else
					(void) regs->rbr;
				
				break;
			}
			case 4:
			{
				/* Receive Buffer Register (RBR) full, or
				 * reception FIFO level equal to or above
				 * treshold
				 */
			}
			case 12:
			{
				/* At least one character is in the reception
				 * FIFO, and no character has been input to or
				 * read from the reception FIFO for four
				 * character times
				 */
				pc16550_read (iovar, regs);
				
				break;
			}
			case 2:
			{
				/* Transmitter Data Register Empty
				 */
				pc16550_write (iovar, regs);
				
				break;
			}
			case 0:
			{
				/* Any transition on /CTS, /DSR or /DCD or a low to
				 * high transition on /RI
				 */
				register uchar msr = regs->msr;
				
				if (msr & DCTS)
				{
					DEBUG_I (("pc16550_int: CTS state change -> %i", (msr & CTSI)));
					
					if (msr & CTSI)
					{
						/* CTS is on */
						
						iovar->state &= ~STATE_HARD;
						
						if (iovar->shake & SHAKE_HARD)
							txint_on (iovar->regs);
					}
					else
					{
						/* CTS is off */
						
						iovar->state |= STATE_HARD;
						
						if (iovar->shake & SHAKE_HARD)
							txint_off (iovar->regs);
					}
				}
				
				/* (msr & DDSR)
					; */
				
				/* (msr & TERI)
					; */
				
				if (msr & DDCD)
				{
					DEBUG_I (("pc16550_int: DCD state change -> %i", (msr & DCDI)));
					
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
				
				break;
			}
			default:
			{
				/* unknown event
				 */
				
				(void) regs->lsr;
				(void) regs->msr;
				
				DEBUG_I (("pc16550_int: unknown event, ignored"));
				break;
			}
		}
	}
	
	splx (sr);
	
	iovar = iovar->next;
	if (iovar)
	{
		DEBUG_I (("pc16550_int: continuing on next IOVAR!"));
		goto next_uart;
	}
	
	DEBUG_I (("pc16550_int: done"));
}

/* Interrupt-Routinen fuer PC16550 - rufen die eigentlichen C-Routinen auf
 */
static void __attribute__((interrupt))
pc16550_int0 (void)
{
	pc16550_int(intr_iovar[0]);
}

static void __attribute__((interrupt))
pc16550_int1 (void)
{
	pc16550_int(intr_iovar[1]);
}

static void __attribute__((interrupt))
pc16550_int2 (void)
{
	pc16550_int(intr_iovar[2]);
}

static void __attribute__((interrupt))
pc16550_int3 (void)
{
	pc16550_int(intr_iovar[3]);
}

static void __attribute__((interrupt))
pc16550_int4 (void)
{
	pc16550_int(intr_iovar[4]);
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
			DEBUG (("uart.xdd: check_ioevent: waking I/O wait (%lu)", iovar->iosleepers));
			wake (IO_Q, (long) &iovar->tty.state);
		}
	}
	
	if (iovar->tty.rsel)
	{
		if (!iorec_empty (&iovar->input))
		{
			DEBUG (("uart.xdd: wakeselect -> read (%s)", p->fname));
			wakeselect (iovar->tty.rsel);
		}
	}
	
	if (iovar->tty.wsel)
	{
		if (iorec_used (&iovar->output) < iovar->output.low_water)
		{
			DEBUG (("uart.xdd: wakeselect -> write (%s)", p->fname));
			wakeselect (iovar->tty.wsel);
		}
	}
}

static void
soft_cdchange (PROC *p, long arg)
{
	IOVAR *iovar = (IOVAR *) arg;
	
	iovar->stintr = 0;
	
	if (!(iovar->clocal) && !(iovar->regs->msr & DCDI))
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
			
			DEBUG (("TS_BLIND set, lost carrier (p = %lx, tty.pgrp = %i)", p, iovar->tty.pgrp));
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
		
		static const long table [] =
		{
			300, 600,
			1200, 1800, 2400, 3600, 4800, 9600,
			19200, 38400, 57600, 76800,
			115200, 230400, 460800,
			0
		};
		
		long baudrate;
		long div;
		
		div = iovar->baudbase / speed;
		
		if (div < 1)
			div = 1;
		if (div > 65534)
			div = 65534;
		
		baudrate = iovar->baudbase / div;
		
		if ((baudrate != speed) || (iovar->baudbase % div))
		{
			if (speed > iovar->baudbase)
			{
				baudrate = iovar->baudbase;
			}
			else
			{
				int i;
				
				baudrate = table [0];
				
				for (i = 0; table [i] != 0; i++)
				{
					if (speed > table [i] && speed <= iovar->baudbase)
						baudrate = table [i];
				}
			}
			
			*buf = baudrate;
			
			DEBUG (("ctl_TIOCBAUD: EBADARG error -> %li", baudrate));
			return EBADARG;
		}
		
		if (baudrate != iovar->baudrate)
		{
			UART *regs = iovar->regs;
			uchar oldmask;
			
			oldmask = top_intrs_off (iovar);
			
			/* set new daudrate
			 */
			iovar->baudrate = baudrate;
			div = iovar->baudbase / baudrate;
			
			regs->lcr |= BNKSE;
			regs->dll = (div) & 0xFF;
			regs->dlm = (div >> 8) & 0xFF;
			regs->lcr &= ~BNKSE;
			
			/* Reset FIFOs */
			regs->fcr = FIFO_EN | RXFR | TXFR;
			/* Enable FIFOs, set Rcv Threshold */
			regs->fcr = FIFO_EN | RXFTH_8;
			
			top_intrs_on (iovar, oldmask);
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
	uchar lcr = iovar->regs->lcr;
	
	switch (lcr & WLS)
	{
		case 0: flags |= TF_5BIT; break;
		case 1: flags |= TF_6BIT; break;
		case 2: flags |= TF_7BIT; break;
		case 3: flags |= TF_8BIT; break;
	}
	
	if (lcr & STB)	flags |= TF_2STOP;
	else		flags |= TF_1STOP;
	
	if (lcr & PEN)
	{
		if (lcr & EPS)	flags |= T_EVENP;
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
	uchar lcr = iovar->regs->lcr;
	
	lcr &= ~WLS;
	switch (flags & TF_CHARBITS)
	{
		case TF_5BIT:	lcr |= 0; datmask = 0x1f;	break;
		case TF_6BIT:	lcr |= 1; datmask = 0x3f;	break;
		case TF_7BIT:	lcr |= 2; datmask = 0x7f;	break;
		case TF_8BIT:	lcr |= 3; datmask = 0xff;	break;
		default:	return EBADARG;
	}
	
	switch (flags & TF_STOPBITS)
	{
		case TF_1STOP:	lcr &= ~STB;			break;
		case TF_15STOP:	return EBADARG;
		case TF_2STOP:	lcr |= STB;			break;
		default:	return EBADARG;
	}
	
	if (flags & (T_EVENP | T_ODDP))
	{
		if ((flags & (T_EVENP | T_ODDP)) == (T_EVENP | T_ODDP))
			return EBADARG;
		
		/* enable parity */
		lcr |= PEN;
		
		/* set even/odd parity */
		if (flags & T_EVENP)	lcr |= EPS;
		else			lcr &= ~EPS;
	}
	else
	{
		/* disable parity */
		lcr &= ~PEN;
		
		/* even/odd bit ignored in this case */
	}
	
	/* setup in register */
	iovar->regs->lcr = lcr;
	
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
			 */
			if (iovar->regs->msr & CTSI)
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
		top_txint_on (iovar);
	
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
		if (r) ALERT (("uart.xdd: ctl_TIOCSFLAGSB -> set flags failed!"));
		
		DEBUG (("ctl_TIOCSFLAGSB: new flags (%lx) set (= %li)", flags, r));
	}
	
	return flags;
}

/* END control functions - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN start/stop primitives - top half */

static void
rx_start (IOVAR *iovar)
{
	if (iovar->rx_full)
	{
		DEBUG (("rx_start: start receiver"));
		
		if (iovar->shake & SHAKE_HARD)
		{
			/* set rts */
			top_rts_on (iovar);
		}
		
		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XON */
			iovar->sendnow = XON;
			
			/* enable interrupt */
			top_txint_on (iovar);
		}
		
		/* receiver started */
		iovar->rx_full = 0;
	}
	else
	{
		// DEBUG (("rx_start: already started"));
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
		}
		
		if (iovar->shake & SHAKE_SOFT)
		{
			/* send XOFF */
			iovar->sendnow = XOFF;
			
			/* enable interrupt */
			top_txint_on (iovar);
		}
		
		/* receiver stopped */
		iovar->rx_full = 1;
	}
	else
	{
		// DEBUG (("rx_stop: already stopped"));
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
		
		/* enable interrupt */
		top_txint_on (iovar);
	}
	else
	{
		// DEBUG (("tx_start: already started"));
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
		// DEBUG (("tx_start: already stopped"));
	}
}

static void
flush_output (IOVAR *iovar)
{
	IOREC *iorec = &iovar->output;
	uchar oldmask;
	
	oldmask = top_intrs_off (iovar);
	iorec->head = iorec->tail = 0;
	top_intrs_on (iovar, oldmask);
}

static void
flush_input (IOVAR *iovar)
{
	IOREC *iorec = &iovar->input;
	uchar oldmask;
	
	oldmask = top_intrs_off (iovar);
	iorec->head = iorec->tail = 0;
	top_intrs_on (iovar, oldmask);
}

/* END start/stop primitives - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN bios emulation - top half */

static long _cdecl
uart_instat (int dev)
{
	IOVAR *iovar = IOVARS_BDEV (dev);
	IOREC *iorec = &iovar->input;
	long used;
	
	used = iorec_used (iorec);
	return (used ? -1 : 0);
}

static long _cdecl
uart_in (int dev)
{
	IOVAR *iovar = IOVARS_BDEV (dev);
	IOREC *iorec = &iovar->input;
	long ret;
	
	ret = iorec_used (iorec);
	if (!ret)
	{
		while (iorec_empty (iorec))
		{
			/* start receiver */
			rx_start (iovar);
			
			DEBUG (("uart_in: I/O sleep"));
			
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
uart_outstat (int dev)
{
	IOVAR *iovar = IOVARS_BDEV (dev);
	IOREC *iorec = &iovar->output;
	long free;
	
	free = iorec_free (iorec);
	free -= 4;
	if (free < 0)
		free = 0;
	
	return (free ? -1 : 0);
}

static long _cdecl
uart_out (int dev, int c)
{
	IOVAR *iovar = IOVARS_BDEV (dev);
	IOREC *iorec = &iovar->output;
	
	while (!iorec_put (iorec, c))
	{
		/* start transmitter */
		tx_start (iovar);
		
		DEBUG (("uart_out: I/O sleep"));
		
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
uart_rsconf (int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr)
{
	static ulong baud [16] =
	{
		19200, 9600, 4800, 3600, 2400, 2000, 1800, 1200,
		600, 300, 230400, 115200, 57600, 38400, 153600, 76800
	};
	
	IOVAR *iovar = IOVARS_BDEV (dev);
	ushort modified = 0;
	ushort flags;
	ulong ret = 0;
	
	DEBUG (("uart_rsconf: enter %i, %i", dev, speed));
	
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
	if (iovar->regs->lcr & SBRK)
		ret |= (0x8L << 8);
	
	return ret;
}

/* END bios emulation - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver routines - top half */

static long _cdecl
uart_open (FILEPTR *f)
{
	ushort dev = f->fc.aux;
	IOVAR *iovar;
	
	DEBUG (("uart_open [%i]: enter (%lx)", f->fc.aux, f->flags));
	
	if (dev >= IOVAR_REAL_MAX)
		return EACCES;
	
	iovar = IOVARS (dev);
	if (!iovar)
		return EACCES;
	
	if (!iovar->open)
	{
		/* first open */
		
		/* assign dtr line */
		top_dtr_on (iovar);
		
		/* start receiver */
		rx_start (iovar);
	}
	else
	{
		if (denyshare (iovar->open, f))
		{
# if DEV_DEBUG > 0
			FILEPTR *t = iovar->open;
			while (t)
			{
				DEBUG (("t = %lx, t->next = %lx", t, t->next));
				DEBUG (("  links = %i, flags = %x", t->links, t->flags));
				DEBUG (("  pos = %li, devinfo = %lx", t->pos, t->devinfo));
				DEBUG (("  dev = %lx, next = %lx", t->dev, t->next));
				DEBUG (("  fc.index = %li, fc.dev = %i, fc.aux = %i", t->fc.index, t->fc.dev, t->fc.aux));
				
				t = t->next;
			}
# endif
			DEBUG (("uart_open: file sharing denied"));
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
	
	DEBUG (("uart_open: return E_OK (added %lx)", f));
	return E_OK;
}

static long _cdecl
uart_close (FILEPTR *f, int pid)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	
	DEBUG (("uart_close [%i]: enter", f->fc.aux));
	
	if ((f->flags & O_LOCK)
	    && ((iovar->lockpid == pid) || (f->links <= 0)))
	{
		DEBUG (("uart_close: remove lock by %i", pid));
		
		f->flags &= ~O_LOCK;
		iovar->lockpid = 0;
		
		/* wake anyone waiting for this lock */
		wake (IO_Q, (long) iovar);
	}
	
	if (f->links <= 0)
	{
		register FILEPTR **temp;
		register long flag = 1;
		
		DEBUG (("uart_close: freeing FILEPTR %lx", f));
		
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
			ALERT (("uart_close[%d]: remove open FILEPTR fail!", f->fc.aux));
		
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
	
	return E_OK;
}


/* HSMODEM write/read routines
 * 
 * they never block
 * they don't do any carrier check or so
 */
static long _cdecl
uart_write (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	long done = 0;
	
	DEBUG (("uart_write [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
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
	
	DEBUG (("uart_write: leave (%ld)", done));
	return done;
}

static long _cdecl
uart_read (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	long done = 0;
	
	DEBUG (("uart_read [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
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
	
	DEBUG (("uart_read: leave (%ld)", done));
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
uart_writeb (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	
	DEBUG (("uart_writeb [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
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
			
			DEBUG (("uart_writeb: sleeping I/O wait"));
			
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
	
	DEBUG (("uart_writeb: leave (%ld)", done));
	return done;
}

static void
timerwake (PROC *p, long cond)
{
	wake (IO_Q, cond);
}

static long _cdecl
uart_readb (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	int ndelay = (f->flags & O_NDELAY) || iovar->tty.vtime /* ??? */;
	long done = 0;
	
	DEBUG (("uart_readb [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
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
				
				DEBUG (("uart_readb: I/O sleep in vtime == 0"));
				
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
				
				DEBUG (("uart_readb: I/O sleep in vmin == 0"));
				
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
				
				DEBUG (("uart_readb: I/O sleep in vtime && vtime"));
				
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
	
	DEBUG (("uart_readb: leave (%ld)", done));
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
uart_twrite (FILEPTR *f, const char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->output;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	const long *r = (const long *) buf;
	
	DEBUG (("uart_twrite [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
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
			DEBUG (("uart_twrite: sleeping I/O wait"));
			
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
	
	DEBUG (("uart_twrite: leave (%ld)", done));
	return done;
}

static long _cdecl
uart_tread (FILEPTR *f, char *buf, long bytes)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	IOREC *iorec = &iovar->input;
	int ndelay = f->flags & O_NDELAY;
	long done = 0;
	long *r = (long *) buf;
	
	DEBUG (("uart_tread [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
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
			DEBUG (("uart_tread: sleeping I/O wait"));
			
			iovar->iosleepers++;
			sleep (IO_Q, (long) &iovar->tty.state);
			iovar->iosleepers--;
		}
	}
	while (bytes && !ndelay);
	
	if (ndelay && !done)
		done = EWOULDBLOCK;
	
	DEBUG (("uart_tread: leave (%ld)", done));
	return done;
}

static long _cdecl
uart_lseek (FILEPTR *f, long where, int whence)
{
	DEBUG (("uart_lseek [%i]: enter (%ld, %d)", f->fc.aux, where, whence));
	
	/* (terminal) devices are always at position 0 */
	return 0;
}

static long _cdecl
uart_ioctl (FILEPTR *f, int mode, void *buf)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	long r = E_OK;
	
	DEBUG (("uart_ioctl [%i]: (%x, (%c %i), %lx)", f->fc.aux, mode, (char) (mode >> 8), (mode & 0xff), buf));
	
	switch (mode)
	{
		case FIONREAD:
		{
			*(long *) buf = iorec_used (&iovar->input);
			DEBUG (("uart_ioctl [%i]: FIONREAD -> %li", f->fc.aux, *(long *) buf));
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
			DEBUG (("uart_ioctl [%i]: FIONWRITE -> %li", f->fc.aux, *(long *) buf));
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
			DEBUG (("uart_ioctl(TIOCIBAUD) to %li", *(long *) buf));
			
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
			DEBUG (("uart_ioctl(TIOCGFLAGS) [%lx]", (ushort *) buf));
			
			*(ushort *) buf = ctl_TIOCGFLAGS (iovar);
			break;
		}
		case TIOCSFLAGS:
		{
			DEBUG (("uart_ioctl(TIOCSFLAGS) -> %lx", *(ushort *) buf));
			
			r = ctl_TIOCSFLAGS (iovar, *(ushort *) buf);
			break;
		}
		case TIONOTSEND:
		case TIOCOUTQ:
		{
			DEBUG (("uart_ioctl(TIOCOUTQ) -> %li", iorec_used (&iovar->output)));
			
			*(long *) buf = iorec_used (&iovar->output);
			break;
		}
		case TIOCSFLAGSB:
		{
			DEBUG (("uart_ioctl(TIOCSFLAGSB) -> %lx, %lx", ((long *) buf)[0],
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
			{
				DEBUG (("uart_ioctl: TS_BLIND sleep in TIOCWONLINE"));
				sleep (IO_Q, (long) &iovar->tty.state);
			}
			
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
			register char reg;
			
			/* TIOCMH_LE */
			
			/* mcr */
			reg = iovar->regs->mcr;
			if (reg & DTRO) val |= TIOCMH_DTR;
			if (reg & RTSO) val |= TIOCMH_RTS;
			
			/* msr */
			reg = iovar->regs->msr;
			if (reg & CTSI) val |= TIOCMH_CTS;
			if (reg & RII ) val |= TIOCMH_RI;
			if (reg & DCDI) val |= TIOCMH_CD;
			if (reg & DSRI) val |= TIOCMH_DSR;
			
			/* TIOCMH_LEI */
			/* TIOCMH_TXD */
			/* TIOCMH_RXD */
			
			/* lsr */
			reg = iovar->regs->lsr;
			if (reg & BRK ) val |= TIOCMH_BRK;
			/* TIOCMH_TER */
			/* TIOCMH_RER */
			if (reg & TXDE) val |= TIOCMH_TBE;
			if (reg & RXDA) val |= TIOCMH_RBF;
			
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
			register char reg;
			
			/* TIOCM_LE */
			
			/* mcr */
			reg = iovar->regs->mcr;
			if (reg & DTRO) val |= TIOCM_DTR;
			if (reg & RTSO) val |= TIOCM_RTS;
			
			/* TIOCM_ST */
			/* TIOCM_SR */
			
			/* msr */
			reg = iovar->regs->msr;
			if (reg & CTSI) val |= TIOCM_CTS;
			if (reg & DCDI) val |= TIOCM_CD;
			if (reg & RII ) val |= TIOCM_RI;
			if (reg & DSRI) val |= TIOCM_DSR;
			
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
					DEBUG (("uart_ioctl: sleep in SETLKW"));
					sleep (IO_Q, (long) iovar);
				}
				else
					return ELOCKED;
			}
			
			if (lck->l_type == F_UNLCK)
			{
				if (!(f->flags & O_LOCK))
				{
					DEBUG (("uart_ioctl: wrong file descriptor for UNLCK"));
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
				lck->l_type = F_UNLCK;
			
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
	
	DEBUG (("uart_ioctl: return %li", r));
	return r;
}

static long _cdecl
uart_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	DEBUG (("uart_datime [%i]: enter (%i)", f->fc.aux, rwflag));
	
	if (rwflag)
		return EACCES;
	
	*timeptr++ = timestamp;
	*timeptr = datestamp;
	
	return E_OK;
}

static long _cdecl
uart_select (FILEPTR *f, long proc, int mode)
{
	IOVAR *iovar = IOVARS (f->fc.aux);
	struct tty *tty = (struct tty *) f->devinfo;
	
	DEBUG (("uart_select [%i]: enter (%li, %i, %lx)", f->fc.aux, proc, mode, tty));
	
	if (mode == O_RDONLY)
	{
		if (!(tty->state & TS_BLIND) && !iorec_empty (&iovar->input))
		{
			TRACE (("uart_select: data present for device %lx", iovar));
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
			TRACE (("uart_select: ready to output on %lx", iovar));
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
uart_unselect (FILEPTR *f, long proc, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;
	
	DEBUG (("uart_unselect [%i]: enter (%li, %i, %lx)", f->fc.aux, proc, mode, tty));
	
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
