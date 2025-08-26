/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 *
 * BIOS replacement routines
 *
 */

# include "bios.h"
# include "global.h"

# include "libkern/libkern.h"

# include "mint/arch/mfp.h"
# include "mint/asm.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/signal.h"
# include "mint/xbra.h"

# include "arch/detect.h"
# include "arch/intr.h"
# include "arch/mprot.h"
# include "arch/syscall.h"
# include "arch/timer.h"
# include "arch/tosbind.h"
# include "arch/user_things.h"

# include "biosfs.h"
# include "console.h"
# include "dev-mouse.h"
# include "dosdir.h"
# include "filesys.h"
# include "info.h"
# include "keyboard.h"
# include "k_prot.h"
# include "memory.h"
# include "proc.h"
# include "random.h"
# include "signal.h"
# include "timeout.h"
# include "tosfs.h"
# include "tty.h"
# include "xbios.h"

# include <stddef.h>


/* some BIOS vectors; note that the routines at these vectors may do nasty
 * things to registers!
 */

# define RWABS		*((long *) 0x476L)
# define MEDIACH	*((long *) 0x47eL)
# define GETBPB 	*((long *) 0x472L)


/* tickcal: return milliseconds per system clock tick
 */
long _cdecl
sys_b_tickcal (void)
{
	return (long) (*((ushort *) 0x0442L));
}

/* drvmap: return drives connected to system
 */
long _cdecl
sys_b_drvmap (void)
{
	return *((long *) 0x4c2L);
}

/* kbshift: return (and possibly change) keyboard shift key status
 *
 * WARNING: syscall.spp assumes that kbshift never blocks, and never
 * calls any underlying TOS functions
 */

char *kbshft;		/* set in main.c */

long _cdecl
sys_b_kbshift (int mode)
{
	int oldshft;

	oldshft = *((uchar *) kbshft);
	if (mode >= 0)
		*kbshft = mode;

	return oldshft;
}

/* mediach: check for media change
 */
long _cdecl
sys_b_mediach (int dev)
{
	long r;

	r = callout1 (MEDIACH, dev);
	return r;
}

/* getbpb: get BIOS parameter block
 */
struct bpb *_cdecl
sys_b_getbpb (int dev)
{
	union { long r; short *ptr; struct bpb *bpb; } r;

	/* we can't trust the Getbpb routine to accurately save all registers,
	 * so we do it ourselves
	 */
	r.r = callout1 (GETBPB, dev);

	/* There is a bug in the TOS disk handling routines (well, several
	 * actually). If the directory size of Getbpb() is returned as zero
	 * then the drive 'dies' and won't read any new disks even with the
	 * 'ESC' enforced disk change. This is present even in TOS 1.06
	 * (not sure about 1.62 though). This small routine changes the dir
	 * size to '1' if it is zero. It may make some non-TOS disks look
	 * a bit weird but that's better than killing the drive.
	 */
	if (r.r)
	{
		if (r.ptr[3] == 0)
			r.ptr[3] = 1;
#ifdef OLDTOSFS
		if (dev >= 0 && dev < NUM_DRIVES)
			clsizb[dev] = (long)((unsigned long)r.ptr[0] * r.ptr[1]);
#endif
#if 0
		if (((short *) r)[3] == 0)	/* 0 directory size? */
			((short *) r)[3] = 1;

# ifdef OLDTOSFS
		/* jr: save cluster size in area */
		if (dev >= 0 && dev < NUM_DRIVES)
			/* Falcon TOS works with cluster size 65536? */
			clsizb[dev] = (long)(((ushort *) r)[0]) * (long)(((ushort *) r)[1]);
# endif
#endif
	}

	return r.bpb;
}

/* rwabs: various disk stuff
 */
long _cdecl
sys_b_rwabs (int rwflag, void *buffer, int number, int recno, int dev, long lrecno)
{
	PROC *p = get_curproc();
	long r;

	/* jr: inspect bit 3 of rwflag!!!
	 */
	if (!(rwflag & 8) && dev >= 0 && dev < NUM_DRIVES)
	{
		if (aliasdrv [dev])
			dev = aliasdrv [dev] - 1;

		if (dlockproc [dev] && dlockproc [dev] != p)
		{
			DEBUG (("Rwabs: device %c is locked", DriveToLetter(dev)));
			return ELOCKED;
		}
	}

	/* only the superuser can make Rwabs calls directly
	 */
	if (secure_mode)
	{
		if (p->in_dos || suser (p->p_cred->ucr))
		{
			/* Note that some (most?) Rwabs device drivers don't
			 * bother saving registers, whereas our compiler
			 * expects politeness. So we go via callout(), which
			 * will save registers for us.
			 */
			TRACE (("calling RWABS buffer:%p",buffer));
			r = callout (RWABS, rwflag, buffer, number, recno, dev, lrecno);
			TRACE (("returning from RWABS"));
		}
		else
		{
			DEBUG (("RWABS by non privileged process!"));
			r = EPERM;
		}
	}
	else
	{
		TRACE (("calling RWABS buffer:%p",buffer));
		r = callout (RWABS, rwflag, buffer, number, recno, dev, lrecno);
		TRACE (("returning from RWABS"));
	}

	return r;
}

/* setexc: set exception vector
 */
long _cdecl
sys_b_setexc (int number, long vector)
{
# ifdef JAR_PRIVATE
	struct user_things *ut;
# endif
	PROC *p = get_curproc();
	long *place;
	long old;

# ifdef JAR_PRIVATE
	ut = p->p_mem->tp_ptr;
# endif
	/* If the caller has no root privileges, we'll attempt
	 * to terminate it. We allow to change the critical error handler
	 * and the GEMDOS terminate vector (and the cookie jar pointer too),
	 * because these are private for each process
	 */
# ifdef JAR_PRIVATE
	if (vector != -1 && number != 0x0101 && number != 0x0102 && number != 0x0168 && \
		secure_mode && p->in_dos == 0)
# else
	if (vector != -1 && number != 0x0101 && number != 0x0102 && \
		secure_mode && p->in_dos == 0)
# endif
	{
# ifdef WITH_SINGLE_TASK_SUPPORT
		if( !(get_curproc()->modeflags & M_SINGLE_TASK) )
# endif
			if( vector != -1 && allow_setexc < 2 && !(get_curproc()->pid <= 1 || get_curproc()->ppid == 0) )
			{
				if( allow_setexc == 0 || number <= 15 )
				{
					DEBUG (("Setexc (%d,%lx) not permitted", number,vector));
					return EPERM;
				}
			}
		if (!suser (p->p_cred->ucr))
		{
			DEBUG (("Setexc by non privileged process!"));
			raise (SIGSYS);
			return EPERM;
		}
# ifndef M68000
		else
		{
			/* What about this: a program can only change an
			 * exception vector, if its memory is F_PROT_G or
			 * F_PROT_S? Perhaps it could save us a crash, or
			 * even two... (draco)
			 *
			 * Unfortunately some programs go Super() then change
			 * vectors directly. Common practice in games/demos :(
			 */
			if ((p->p_mem->memflags & F_OS_SPECIAL) == 0)
			{
				if (((p->p_mem->memflags & F_PROTMODE) == F_PROT_P) ||
					((p->p_mem->memflags & F_PROTMODE) == F_PROT_PR))
				{
					ALERT (MSG_bios_kill);
					/* Avoid an additional alert,
					 * be f**cking efficient.
					 */
					raise (SIGKILL);

					return EACCES;
				}
			}
		}
# endif
	}

	place = (long *)(((long) number) << 2);

	/* Note: critical error handler (0x0101) is kernel private now
	 * and programs cannot really use it.
	 */
	if (number == 0x102)
		/* GEMDOS term vector */
		old = p->ctxt[SYSCALL].term_vec;
# ifdef JAR_PRIVATE
	else if (number == 0x0168)
		/* The cookie jar pointer */
		old = (long)ut->user_jar_p;
# endif
	else
		old = *place;

	if (vector > 0)
	{
		/* validate vector; this will cause a bus error if mem
		 * protection is on and the current process doesn't have
		 * access to the memory
		 *
		 * XXX fna: this *NOT* the recommended way to terminate a
		 *	    process from kernel
		 *
		 *	    rewrite to check if the address is inside a
		 *	    memregion of the process
		 */
		if (*((long *) vector) == 0xDEADBEEFL)
			return old;

		if (number == 0x102)
		{
			p->ctxt[SYSCALL].term_vec = vector;
		}
# ifdef JAR_PRIVATE
		else if (number == 0x0168)
			ut->user_jar_p = (struct cookie *)vector;
# endif
		else if (number == 0x101)
		{
			/* problem:
			 * lots of TSR's look for the Setexc (0x101,...)
			 * that the AES does at startup time; so we have
			 * to pass it along.
			 */
			long mintcerr;

			mintcerr = (long)ROM_Setexc (0x101, (void (*)()) vector);
			*place = mintcerr;
		}
		else
		{
# if 0
			/* This code below is a nonsense. Of course, an exception vector
			 * should point to a supervisor or global memory, but an exception
			 * handler rarely consists of a code, that does not read/write
			 * memory. So, if a program installs an exception handler, this
			 * code below converts the handler code to Super, but does not
			 * convert the protection status of the DATA/BSS segments, which
			 * are possibly accessed by the handler. If they are, and they are
			 * `private' --> instant crash (draco)
			 */
			if (!no_mem_prot)
			{
				/* if memory protection is on, the vector
				 * should be pointing at supervisor or
				 * global memory
				 */
				MEMREGION *r;

				r = addr2region (vector);
				if (r && get_prot_mode (r) == PROT_P)
				{
					DEBUG (("Changing protection to Supervisor because of Setexc"));
					mark_region (r, PROT_S, 0);
				}
			}
# endif

			/* We would do just *place = vector except that
			 * someone else might be intercepting Setexc looking
			 * for something in particular...
			 *
			 * psigintr() exception shadow area.
			 * if curproc->in_dos varies from a zero,
			 * it may be a call from psigintr() and shadow
			 * must not be updated that time.
			 */
# if 0
			if (intr_shadow && number < 0x0100 && p->in_dos == 0)
				intr_shadow[number] = vector;
# endif
			old = (long)ROM_Setexc (number, (void (*)()) vector);
		}
	}

	TRACE (("Setexc %d, %lx -> %lx", number, vector, old));
	return old;
}

# define BDEVMAP_MAX	16
static BDEVMAP bdevmap [BDEVMAP_MAX];

static long _cdecl	_ubconstat (int dev);
static long _cdecl	_ubcostat (int dev);
static long _cdecl	_ubconout (int dev, int c);
static long _cdecl	_ubconin (int dev);

/* BIOS initialization routine: gets keyboard buffer pointers, for the
 * interrupt routine below
 */
void
init_bios(void)
{
	int i;

	keyrec = (IOREC_T *)TRAP_Iorec(1);

	for (i = 0; i < BDEVMAP_MAX; i++)
	{
		BDEVMAP *map = &(bdevmap[i]);

		map->instat	= _ubconstat;
		map->in 	= _ubconin;
		map->outstat	= _ubcostat;
		map->out	= _ubconout;
		map->rsconf	= NULL;
	}
}

long
overlay_bdevmap (int dev, BDEVMAP *newmap)
{
	DEBUG (("overlay_bdevmap: %i", dev));

	if ((ushort) dev < BDEVMAP_MAX)
	{
		int i;

		if ((long) newmap->instat == 1)
			bdevmap [dev].instat = NULL;
		else if (newmap->instat)
			bdevmap [dev].instat = newmap->instat;

		if ((long) newmap->in == 1)
			bdevmap [dev].in = NULL;
		else if (newmap->in)
			bdevmap [dev].in = newmap->in;

		if ((long) newmap->outstat == 1)
			bdevmap [dev].outstat = NULL;
		else if (newmap->outstat)
			bdevmap [dev].outstat = newmap->outstat;

		if ((long) newmap->out == 1)
			bdevmap [dev].out = NULL;
		else if (newmap->out)
			bdevmap [dev].out = newmap->out;

		if ((long) newmap->rsconf == 1)
			bdevmap [dev].rsconf = NULL;
		else if (newmap->rsconf)
			bdevmap [dev].rsconf = newmap->rsconf;

# if 1
		/* and, at last, disable original bttys :-) */
		for (i = 0; i < btty_max; i++)
		{
			if (bttys[i].bdev == dev)
			{
				int j;

				for (j = i + 1; j < btty_max; j++, i++)
					bttys[i] = bttys[j];

				btty_max = i;

				break;
			}
		}
# endif

		return E_OK;
	}

	return EINVAL;
}

long _cdecl
sys_b_ubconstat (int dev)
{
	if ((ushort) dev < BDEVMAP_MAX)
	{
		if (bdevmap [dev].instat)
			return (*bdevmap [dev].instat)(dev);
	}

	return ENOSYS;
}

long _cdecl
sys_b_ubconin (int dev)
{
	if ((ushort) dev < BDEVMAP_MAX)
	{
		if (bdevmap [dev].in)
			return (*bdevmap [dev].in)(dev);
	}

	return ENOSYS;
}

long _cdecl
sys_b_ubcostat (int dev)
{
	if ((ushort) dev < BDEVMAP_MAX)
	{
		if (bdevmap [dev].outstat)
			return (*bdevmap [dev].outstat)(dev);
	}

	return ENOSYS;
}

long _cdecl
sys_b_ubconout (int dev, int c)
{
	if ((ushort) dev < BDEVMAP_MAX)
	{
		if (bdevmap [dev].out)
			return (*bdevmap [dev].out)(dev, c);
	}

	return ENOSYS;
}

long _cdecl
sys_b_ursconf (int baud, int flow, int uc, int rs, int ts, int sc)
{
	if (has_bconmap)
	{
		ushort dev = get_curproc()->p_fd->bconmap;

		if ((ushort) dev < BDEVMAP_MAX)
		{
			DEBUG (("rsconf: %i -> %p", dev, bdevmap [dev].rsconf));

			if (bdevmap [dev].rsconf)
				return (*bdevmap [dev].rsconf)(dev, baud, flow, uc, rs, ts, sc);
		}
	}

	return rsconf (baud, flow, uc, rs, ts, sc);
}

/* BIOS device definitions */
#define CONSDEV 2
#define AUXDEV	1
#define PRNDEV	0
#define SERDEV	6	/* First serial port */

/* BIOS devices 0..MAX_BHANDLE-1 can be redirected to GEMDOS files */
# define MAX_BHANDLE	4

/* BIOS redirection maps */
const short binput[MAX_BHANDLE] = { -3, -2, -1, -4 };
const short boutput[MAX_BHANDLE] = { -3, -2, -1, -5 };

/* these are supposed to be tables holding the addresses of the
 * first 8 BconXXX functions, but in fact only the first 5 are
 * placed here (and device 5 only has Bconout implemented;
 * we don't use that device (raw console) anyway).
 */

# define xconstat	((long *) 0x51eL)
# define xconin 	((long *) 0x53eL)
# define xcostat	((long *) 0x55eL)
# define xconout	((long *) 0x57eL)

/* if the system has Bconmap the ones >= 6 are in a table available
 * thru Bconmap(-2)...
 */
#define MAPTAB (bconmap2->maptab)

/* and then do BCOSTAT ourselves, the BIOS SCC ones are often broken */
#define BCOSTAT(dev) \
	(((unsigned)dev <= 4) ? ((tosvers >= 0x0102) ? \
	   (int)callout1(xcostat[dev], dev) : ROM_Bcostat(dev)) : \
	   ((has_bconmap && (unsigned)dev-SERDEV < btty_max) ? \
		bcxstat(MAPTAB[dev-SERDEV].iorec+1) : ROM_Bcostat(dev)))
#define BCONOUT(dev, c) \
	(((unsigned)dev <= 4) ? ((tosvers >= 0x0102) ? \
	   callout2(xconout[dev], dev, c) : ROM_Bconout(dev, c)) : \
	   ((has_bconmap && (unsigned)dev-SERDEV < btty_max) ? \
		callout2(MAPTAB[dev-SERDEV].bconout, dev, c) : ROM_Bconout(dev, c)))
#define BCONSTAT(dev) \
	(((unsigned)dev <= 4) ? ((tosvers >= 0x0102) ? \
	   (int)callout1(xconstat[dev], dev) : ROM_Bconstat(dev)) : \
	   ((has_bconmap && (unsigned)dev-SERDEV < btty_max) ? \
		(int)callout1(MAPTAB[dev-SERDEV].bconstat, dev) : ROM_Bconstat(dev)))
#define BCONIN(dev) \
	(((unsigned)dev <= 4) ? ((tosvers >= 0x0102) ? \
	   callout1(xconin[dev], dev) : ROM_Bconin(dev)) : \
	   ((has_bconmap && (unsigned)dev-SERDEV < btty_max) ? \
		callout1(MAPTAB[dev-SERDEV].bconin, dev) : ROM_Bconin(dev)))

/* variables for monitoring the keyboard */
IOREC_T *keyrec;		/* keyboard i/o record pointer */
BCONMAP2_T *bconmap2;		/* bconmap struct */
short	kintr = 0;		/* keyboard interrupt pending (see intr.s) */

/* replacement *costat for BCOSTAT above
 */
INLINE int
bcxstat (IOREC_T *wrec)
{
	long s;

	s = wrec->head;
	s -= wrec->tail;
	if (s <= 0)
		s += wrec->buflen;

	return s < 3 ? 0 : -1;
}

INLINE int
isonline (struct bios_tty *b)
{
	if (b->tty == &aux_tty)
	{
		/* modem1 */

		if (machine == machine_unknown)
		{
			/* unknown hardware, assume CD always on. */
			return 1;
		}
		else
		{
			/* CD is !bit 1 on the 68901 GPIP port */
			return (1 << 1) & ~(_mfpregs->gpip);
		}
	}
	else if (b->tty == &sccb_tty)
	{
		/* modem2 */

		short sr = spl7 ();
		register uchar r;

# ifndef MILAN
		volatile uchar dummy;
		dummy = _mfpregs->gpip;
		UNUSED (dummy);

		/* CD is bit 3 of read register 0 on SCC port B */
		r = (1 << 3) & *((volatile char *) ControlRegB);
# else
		/* CD is bit 7 of msr register on UART port 1	*/
		r = (1 << 7) & *((volatile char *) ((0x800003f8L + 6L) ^ 3L));
# endif
		spl (sr);
		return r;
	}
	else if (b->tty == &scca_tty)
	{
		/* serial2 */

		short sr = spl7 ();
		register uchar r;

# ifndef MILAN
		volatile uchar dummy;
		dummy = _mfpregs->gpip;
		UNUSED (dummy);

		/* CD is bit 3 of read register 0 on SCC port A */
		r = (1 << 3) & *((volatile char *) ControlRegA);
# else
		/* CD is bit 7 of msr register on UART port 2	*/
		r = (1 << 7) & *((volatile char *) ((0x800002f8L + 6L) ^ 3L));
# endif
		spl (sr);
		return r;
	}

	/* unknown port, assume CD always on. */
	return 1;
}

INLINE int
isbrk (struct bios_tty *b)
{
	if (b->tty == &aux_tty)
	{
		/* modem1 */

		if (machine == machine_unknown)
		{
			/* unknown hardware, cannot detect breaks... */
			return 0;
		}
		else
		{
			/* break is bit 3 in the 68901 RSR */
			return (1 << 3) & _mfpregs->rsr;
		}
	}
	else if (b->tty == &sccb_tty)
	{
		/* modem2 */

		/* break is bit 7 of read register 0 on SCC port B */
		short sr = spl7();
		register uchar r;

		volatile uchar dummy;
		dummy = _mfpregs->gpip;
		UNUSED (dummy);

# ifndef MILAN
		r = (1 << 7) & *((volatile char *) ControlRegB);
# else
		/* break is bit 4 of lsr register on UART port 1 */
		r = (1 << 4) & *((volatile char *) ((0x800002f8L + 5L) ^ 3L));
# endif
		spl (sr);
		return r;
	}
	else if (b->tty == &scca_tty)
	{
		/* serial2 */

		/* like modem2, only port A */
		short sr = spl7();
		register uchar r;

		volatile uchar dummy;
		dummy = _mfpregs->gpip;
		UNUSED (dummy);

# ifndef MILAN
		r = (1 << 7) & *((volatile char *) ControlRegA);
# else
		/* break is bit 4 of lsr register on UART port 2 */
		r = (1 << 4) & *((volatile char *) ((0x800003f8L + 5L) ^ 3L));
# endif
		spl(sr);
		return r;
	}
	else if (b->tty == &ttmfp_tty)
	{
# ifndef MILAN
		/* serial1 */
		return ((1 << 3) & _ttmfpregs->rsr);
# endif
	}

	/* unknown port, cannot detect breaks... */
	return 0;
}

ushort
ionwrite (IOREC_T *wrec)
{
	ushort s;

	s = wrec->head;
	s -= wrec->tail;
	if ((int)s <= 0)
		s += wrec->buflen;

	if ((int)(s -= 2) < 0)
		s = 0;

	return s;
}

ushort
ionread (IOREC_T *irec)
{
	ushort r;

	r = irec->tail;
	r -= irec->head;
	if ((int) r < 0)
		r += irec->buflen;

	return r;
}

ushort
btty_ionread (struct bios_tty *b)
{
	return ionread (b->irec);
}

static void
checkbtty (struct bios_tty *b, int sig)
{
	void **l;

	if (!b->irec)
		return;

	if (!b->clocal && !isonline (b))
	{
		b->vticks = 0;
		b->bticks = jiffies + 0x80000000L;
		if (!(b->tty->state & TS_BLIND))
		{
			/* just lost carrier...  set TS_BLIND,
			 * let reads and writes return
			 */
			b->tty->state |= TS_BLIND;
			iocsbrk (b->bdev, TIOCCBRK, b);
			if (sig)
			{
				b->orec->tail = b->orec->head;
				DEBUG (("checkbtty: bdev %d disconnect", b->bdev));
				if (!(b->tty->sg.sg_flags & T_NOFLSH))
					iread (b->bdev, (char *) NULL, 0, 1, 0);
				if (b->tty->pgrp)
					/* ...and here is the long missed :) SIGHUP  */
					killgroup (b->tty->pgrp, SIGHUP, 1);
			}
			wake (IO_Q, (long) b);
			wake (IO_Q, (long) &b->tty->state);
		}
		return;
	}
	if (b->tty->state & TS_BLIND)
	{
		/* just got carrier (or entered local mode), clear TS_BLIND and
		 * wake whoever waits for it
		 */
		b->tty->state &= ~(TS_BLIND | TS_HOLD);
		wake (IO_Q, (long) &b->tty->state);
	}
	if (sig)
	{
		if (b->brkint && isbrk (b))
		{
			if (!b->bticks)
			{
				/* the break should last for more than 200 ms
				 * or the equivalent of 48 10-bit chars at
				 * ispeed (then its probably not line noise...)
				 */
				if ((ulong) b->ispeed <= 2400)
					b->bticks = jiffies + 40L;
				else
					b->bticks = jiffies + (480L * 200L / (ulong) b->ispeed);
				if (!b->bticks)
					b->bticks = 1;
			}
			else if (jiffies - b->bticks > 0)
			{
				/* every break only one interrupt please
				 */
				b->bticks += 0x80000000L;

				DEBUG (("checkbtty: bdev %d break(int)", b->bdev));

				if (!(b->tty->sg.sg_flags & T_NOFLSH))
					iread (b->bdev, (char *) NULL, 0, 1, 0);

				if (b->tty->pgrp)
				{
					DEBUG (("checkbtty: killgroup (%i, SIGINT)", b->tty->pgrp));
					killgroup (b->tty->pgrp, SIGINT, 1);
				}
			}
		}
		else
			b->bticks = 0;
	}

	if (!b->vticks || jiffies - b->vticks > 0)
	{
		long r;

		if ((r = (long) b->tty->vmin - btty_ionread (b)) <= 0)
		{
			b->vticks = 0;
			wake (IO_Q, (long)b);
			l = (void **) b->rsel;
			if (*l)
				wakeselect (*l);
		}
		else if ((--r, r *= 2000L) > (ulong) b->ispeed)
		{
			b->vticks = jiffies + (r / (ulong) b->ispeed);
			if (!b->vticks)
				b->vticks = 1;
		}
		else
			b->vticks = 0;
	}

	if (b->tty->state & TS_HOLD)
		return;

	l = (void **) b->wsel;
	if (*l)
	{
		long i;

		i = b->orec->tail;
		i -= b->orec->head;
		if (i < 0)
			i += b->orec->buflen;

		if (i < b->orec->hi_water)
			wakeselect (*l);
	}
}

void
checkbttys (void)
{
	struct bios_tty *b;

	for (b = bttys; b < bttys + btty_max; b++)
		checkbtty (b, 1);

	b = &midi_btty;
	if (!b->vticks || jiffies - b->vticks > 0)
	{
		long r;
		void **l;

		if ((r = (long) b->tty->vmin - ionread (b->irec)) <= 0)
		{
			b->vticks = 0;
			wake (IO_Q, (long) b);
			l = (void **) b->rsel;
			if (*l)
				wakeselect ((PROC *) *l);
		}
		else if ((--r, r *= 2000L) > 31250UL)
		{
			b->vticks = jiffies + (r / 31250UL);
			if (!b->vticks)
				b->vticks = 1;
		}
		else
			b->vticks = 0;
	}
}

void
checkbttys_vbl (void)
{
	struct bios_tty *b;

	for (b = bttys; b < bttys + btty_max; b++)
	{
		if (!b->clocal && b->orec->tail != b->orec->head && !isonline (b))
			b->orec->tail = b->orec->head;
	}
}

/* check 1 tty without raising sigs, needed after turning off local mode
 * (to avoid getting SIGHUP'd immediately...)
 */
void
checkbtty_nsig (struct bios_tty *b)
{
	checkbtty (b, 0);
}

/*
 * Note that BIOS handles 0 - MAX_BHANDLE now reference file handles;
 * to get the physical devices, go through u:\dev\
 *
 * A note on translation: all of the bco[n]XXX functions have a "u"
 * variant that is actually what the user calls. For example,
 * ubconstat is the function that gets control after the user does
 * a Bconstat. It figures out what device or file handle is
 * appropriate. Typically, it will be a biosfs file handle; a
 * request is sent to biosfs, and biosfs in turn figures out
 * the "real" device and calls bconstat.
 */

# define SLICES(pri)	(((pri) >= 0) ?  0 : -(pri))

/*
 * WARNING: syscall.spp assumes that ubconstat never blocks.
 */
static long _cdecl
_ubconstat (int dev)
{
	if (dev < MAX_BHANDLE)
	{
		FILEPTR *f = get_curproc()->p_fd->ofiles[binput [dev]];
		if (file_instat(f))
			goto reset;
		else
			goto punish;
	}
	else
	{
		if (bconstat (dev))
		{
			/* Data is coming - quick! We need some CPU!
			 */
reset:
			get_curproc()->slices = SLICES (get_curproc()->curpri);
			get_curproc()->curpri = get_curproc()->pri;
			return -1;

		}
		else
		{
			/* Process is polling like mad - punish it!
			 */
punish:
			if (get_curproc()->curpri > MIN_NICE)
				get_curproc()->curpri -= 1;

			return 0;
		}
	}
}

long
bconstat (int dev)
{
	if (dev == CONSDEV)
	{
		if (checkkeys ())
			return 0;

		return (keyrec->head != keyrec->tail) ? -1 : 0;
	}

	if (dev == AUXDEV && has_bconmap)
		dev = get_curproc()->p_fd->bconmap;

	return BCONSTAT (dev);
}

/* bconin: input a character
 *
 * WARNING: syscall.spp assumes that ubconin never
 * blocks if ubconstat returns non-zero.
 */
static long _cdecl
_ubconin (int dev)
{
	if (dev < MAX_BHANDLE)
	{
		FILEPTR *f = get_curproc()->p_fd->ofiles [binput [dev]];

		return file_getchar (f, RAW);
	}
	else
		return bconin (dev);
}

/* wait condition for console input
 */
static short console_in;

long
bconin (int dev)
{
	short h;

	if (dev == CONSDEV)
	{
		IOREC_T *k = keyrec;
		long r;

again:
		while (k->tail == k->head)
		{
			if (sleep (IO_Q, (long) &console_in))
			   return EINTR;
		}

		if (checkkeys ())
			goto again;

		h = k->head + 4;
		if (h >= k->buflen)
			h = 0;

		r = *((long *) (k->bufaddr + h));
		k->head = h;

		return r;
	}
	else
	{
		if (dev == AUXDEV)
		{
			if (has_bconmap)
			{
				dev = get_curproc()->p_fd->bconmap;
				h = dev - SERDEV;
			}
			else
				h = 0;
		}
		else
			h = dev - SERDEV;

		if ((unsigned) h < btty_max || dev == 3)
		{
			if (has_bconmap && dev != 3)
			{
				/* help the compiler... :) */
				long *statc;

				while (!callout1 (*(statc = &MAPTAB [dev - SERDEV].bconstat), dev)) {
					if (sleep (IO_Q, (long) &bttys [h]))
						return EINTR;
				}

				return callout1 (statc[1], dev);
			}

			while (!BCONSTAT (dev)) {
				if (sleep (IO_Q, (long) (dev == 3 ? &midi_btty : &bttys[h])))
					return EINTR;
			}
		}
		else if (dev > 0)
		{
			ulong tick;

			tick = jiffies;
			while (!BCONSTAT (dev))
			{
				/* make blocking (for longer) reads eat
				 * less CPU...
				 *
				 * if yield()ed > 2 seconds and still no
				 * data continue with nap
				 */
				if ((jiffies - tick) > 400)
					nap (60);
				else
					yield ();
			}
		}
	}

	return BCONIN (dev);
}

/* bconout: output a character.
 *
 * returns 0 for failure, nonzero for success
 */

static long _cdecl
_ubconout (int dev, int c)
{
	FILEPTR *f;
	char outp;

	if (dev < MAX_BHANDLE)
	{
		f = get_curproc()->p_fd->ofiles [boutput [dev]];
		if (!f)
			return 0;

		if (is_terminal (f))
			return tty_putchar (f, ((long) c) & 0x00ff, RAW);

		outp = c;
		return (*f->dev->write)(f, &outp, 1L);
	}
	else if (dev == 5)
	{
		c &= 0x00ff;
		f = get_curproc()->p_fd->control;
		if (!f)
			return 0;

		if (is_terminal (f))
		{
			if (c < ' ')
			{
				/* MW hack for quoted characters */
				tty_putchar (f, (long) '\033', RAW);
				tty_putchar (f, (long) 'Q', RAW);
			}

			return tty_putchar (f, ((long) c) & 0x00ff, RAW);
		}

		/* note: we're assuming sizeof(int) == 2 here!
		 */
		outp = c;
		return (*f->dev->write)(f, &outp, 1L);
	}
	else
		return bconout (dev, c);
}

long
bconout (int dev, int c)
{
	int statdev;

	if (dev == AUXDEV && has_bconmap)
		dev = get_curproc()->p_fd->bconmap;

	/* compensate for a known BIOS bug: MIDI and IKBD are switched
	 */
	if	(dev == 3)	statdev = 4;
	else if (dev == 4)	statdev = 3;
	else			statdev = dev;

	/* provide a 10 second time out for the printer
	 */
	if (!BCOSTAT (statdev))
	{
		if (dev != PRNDEV)
		{
			do {
				/* BUG: Speedo GDOS isn't re-entrant;
				 * so printer output to the serial port
				 * could cause problems
				 */
				yield ();
			}
			while (!BCOSTAT (statdev));
		}
		else
		{
			long endtime = jiffies + 10 * 200L;
			do {
# if 1
				/* Speedo GDOS isn't re-entrant,
				 * so we can't give up CPU time here :-(
				 */
				yield ();
# endif
			}
			while (!BCOSTAT (statdev) && jiffies < endtime);

			if (jiffies >= endtime)
				return 0;
		}
	}

	/* special case: many text accelerators return a bad value from
	 * Bconout, so we ignore the returned value for the console
	 * Sigh. serptch2 and hsmodem1 also screw this up, so for now let's
	 * only count on it being correct for the printer.
	 */
	{
		long r = BCONOUT (dev, c);

		if (dev == PRNDEV)
			return r;
		else
			return 1;
	}
}

/* bcostat: return output device status
 *
 * WARNING: syscall.spp assumes that ubcostat never blocks.
 */
static long _cdecl
_ubcostat (int dev)
{
	FILEPTR *f;

	/* the BIOS switches MIDI (3) and IKBD (4)
	 * (a bug, but it can't be corrected)
	 */
	if (dev == 4)
	{
		/* really the MIDI port */
		f = get_curproc()->p_fd->ofiles [boutput [3]];
		return file_outstat (f) ? -1 : 0;
	}

	if (dev == 3)
		return BCOSTAT (dev);

	if (dev < MAX_BHANDLE)
	{
		f = get_curproc()->p_fd->ofiles [boutput [dev]];
		return file_outstat (f) ? -1 : 0;
	}
	else
		return bcostat (dev);
}

long
bcostat(int dev)
{

	if (dev == CONSDEV)
	{
		return -1;
	}
	else if (dev == AUXDEV && has_bconmap)
	{
		dev = get_curproc()->p_fd->bconmap;
	}
	/* compensate here for the BIOS bug, so that the MIDI and IKBD
	 * files work correctly
	 */
	else if (dev == 3) dev = 4;
	else if (dev == 4) dev = 3;

	return BCOSTAT (dev);
}


/* special Bconout buffering code:
 *
 * Because system call overhead is so high, programs that do output
 * with Bconout suffer in performance. To compensate for this,
 * Bconout is special-cased in syscall.s, and if possible characters
 * are placed in the 256 byte bconbuf buffer. This buffer is flushed
 * when any system call other than Bconout happens, or when a context
 * switch occurs.
 */

char bconbuf[256];	/* buffer contents */
short bconbsiz = 0;	/* number of characters in buffer */
short bconbdev = -1;	/* BIOS device for which the buffer is valid */
			/* (-1 means no buffering is active) */

/* flush pending BIOS output. Return 0 if some bytes were not successfully
 * written, non-zero otherwise (just like bconout)
 */
long _cdecl
bflush (void)
{
	long ret = 0, bsiz;
	char *s;
	FILEPTR *f;
	short dev;
	short statdev;
	long lbconbuf[256];

	if ((dev = bconbdev) < 0)
		return 0;

	/*
	 * Here we lock the BIOS buffering mechanism by setting bconbdev to -1
	 * This is necessary because if two or more programs try to do
	 * buffered BIOS output at the same time, they can get seriously
	 * mixed up. We unlock by setting bconbdev to 0.
	 *
	 * NOTE: some code (e.g. in sleep()) checks for bconbsiz != 0 in
	 * order to see if we need to do a bflush; if one is already in
	 * progress, it's pointless to do this, so we save a bit of
	 * time by setting bconbsiz to 0 here.
	 */
	bconbdev = -1;
	bsiz = bconbsiz;
	if (bsiz == 0)
		return 0;
	bconbsiz = 0;

	/* BIOS handles 0..MAX_BHANDLE-1 are aliases for special GEMDOS files
	 */
	if (dev < MAX_BHANDLE || dev == 5)
	{
		if (dev == 5)
			f = get_curproc()->p_fd->ofiles [-1];
		else
			f = get_curproc()->p_fd->ofiles [boutput [dev]];

		if (!f)
		{
			bconbdev = 0;
			return 0;
		}

		if (is_terminal (f))
		{
			int oldflags = f->flags;

			s = bconbuf;

			/* turn off NDELAY for this write... */
			f->flags &= ~O_NDELAY;

			if (dev == 5)
			{
				while (bsiz-- > 0)
				{
					if (*s < ' ')
					{
						/* use ESC-Q to quote control character */
						(void)tty_putchar(f, (long)'\033', RAW);
						(void)tty_putchar(f, (long)'Q', RAW);
					}
					(void) tty_putchar (f, (long) *s++, RAW);
				}
			}
			else
			{
				long *where, nbytes;
# if 1

				/* see if we can do fast RAW byte IO thru the device driver... */
			    if ((f->fc.fs != &bios_filesys ||
				    (bsiz > 1 &&
				     ((struct bios_file *)f->fc.index)->drvsize >
					    offsetof (DEVDRV, writeb))) && f->dev->writeb) {
			        struct tty *tty = (struct tty *)f->devinfo;

				tty_checkttou (f, tty);
				tty->state &= ~TS_COOKED;
				if ((ret = (*f->dev->writeb)(f, (char *)s, bsiz)) != ENODEV) {
					f->flags = oldflags;
					bconbdev = 0;
					if (ret < E_OK)
					{
						return 0;
					}
					return -1;
				}
			    }
#endif
/* the tty_putchar should set up terminal modes correctly */
			    (void) tty_putchar(f, (long)*s++, RAW);
			    where = lbconbuf;
			    nbytes = 0;
			    while (--bsiz > 0) {
						*where++ = *s++; nbytes+=4;
			    }
			    if (nbytes) {
						ret = (*f->dev->write)(f, (char *)lbconbuf, nbytes);
			    }
			}
			if (ret < E_OK)
			{
				/* some bytes not successfully flushed */
				ret = 0;
			}
			/* all bytes flush */
			ret = -1;
			f->flags = oldflags;
		} else {
			ret = (*f->dev->write)(f, (char *)bconbuf, bsiz);
			if (ret < E_OK)
			{
				ret = 0;
			}
			ret = -1;
		}
		bconbdev = 0;
		return ret;
	}

	/* Otherwise, we have a real BIOS device
	 */
	if (dev == AUXDEV && has_bconmap)
	{
		dev = get_curproc()->p_fd->bconmap;
		statdev = dev;
	}

	if ((ret = iwrite (dev, bconbuf, bsiz, 0, 0)) != ENODEV)
	{
		bconbdev = 0;
		return ret;
	}
	/* compensate for a known BIOS bug:
	 * MIDI and IKBD are switched
	 */
	else if (dev == 3)
	{
		statdev = 4;
	}
	else if (dev == 4)
	{
		statdev = 3;
	}
	else
		statdev = dev;

	s = bconbuf;
	while (bsiz-- > 0)
	{
		while (!BCOSTAT (statdev))
			yield();

		(void) BCONOUT (dev,*s);
		s++;
	}

	bconbdev = 0;
	return 1L;
}

/*
 * do_bconin: try to do a bconin function quickly, without
 * blocking. If we can't do it without blocking, we return
 * 0x0123dead and the calling trap #13 code falls through
 * to the normal bconin stuff. We can't block here because
 * the trap #13 code hasn't yet saved registers or other
 * context bits, so sleep() wouldn't work properly.
 */

#define WOULDBLOCK 0x0123deadL

/* WARNING: syscall.spp assumes that do_bconin never blocks */

long _cdecl
do_bconin(int dev)
{
	FILEPTR *f;
	long r, nread;
	uchar c;

	if (dev < MAX_BHANDLE)
	{
		f = get_curproc()->p_fd->ofiles [binput[dev]];
		if (!f) return 0;
		nread = 0;
		(void)(*f->dev->ioctl)(f, FIONREAD, &nread);
		if (!nread) return WOULDBLOCK;	/* data not ready */
		if (is_terminal(f)) {
			r = tty_getchar(f, RAW);
		}
		else
		{
			r = (*f->dev->read)(f, (char *)&c, 1L);
			if (r == ENODEV)
			{
				return MiNTEOF;
			}
			else if (r < E_OK)
			{
				return r;
			}
			r = (r == 1) ? c : MiNTEOF;
		}
	}
	else
	{
		if (!bconstat (dev))
			r = WOULDBLOCK;
		else
			r = bconin(dev);
	}

	return r;
}

/*
 * routine for checking keyboard (called by sleep() on any context
 * switch where a keyboard event occured). returns 1 if a special
 * control character was eaten, 0 if not
 */
int
checkkeys (void)
{
	char ch;
	short shift;
	int sig, ret;
	struct tty *tty = &con_tty;
	static short oldktail = 0;

	ret = 0;
	while (oldktail != keyrec->tail)
	{

/* BUG: we really should check the shift status _at the time the key was
 * pressed_, not now!
 */
		sig = 0;
		shift = mshift;
		oldktail += 4;
		if (oldktail >= keyrec->buflen)
			oldktail = 0;

/* check for special control keys, etc. */
/* BUG: this doesn't exactly match TOS' behavior, particularly for
 * ^S/^Q
 * XXX: so what?
 */
		/* Ozk:
		 * Guessing wildly here, but shouldn't jobcontrol
		 * be skipped when T_RAW is set?
		 */
		if (!(tty->sg.sg_flags & T_RAW) && ((tty->state & TS_COOKED) || (shift & MM_CTRLALT) == MM_CTRLALT))
		{
			ch = (keyrec->bufaddr + keyrec->tail)[3];
			if (ch == UNDEF)
				;	/* do nothing */
			else if (ch == tty->tc.t_intrc)
				sig = SIGINT;
			else if (ch == tty->tc.t_quitc)
				sig = SIGQUIT;
			else if (ch == tty->ltc.t_suspc)
				sig = SIGTSTP;
			else if (ch == tty->tc.t_stopc)
			{
				tty->state |= TS_HOLD;
				ret = 1;
				keyrec->head = oldktail;
				continue;
			}
			else if (ch == tty->tc.t_startc)
			{
				tty->state &= ~TS_HOLD;
				ret = 1;
				keyrec->head = oldktail;
				continue;
			}

			if (sig)
			{
				tty->state &= ~TS_HOLD;
				if (!(tty->sg.sg_flags & T_NOFLSH))
				    oldktail = keyrec->head = keyrec->tail;

				DEBUG(("checkkeys: killgroup(%i, %i, 1)", tty->pgrp, sig));
				killgroup(tty->pgrp, sig, 1);
				ret = 1;
			}
			else if (tty->state & TS_HOLD)
			{
				keyrec->head = oldktail;
				ret = 1;
			}
		}
	}

	/* XXX: move this later to ikbd_scan()
	 */
	if (keyrec->head != keyrec->tail)
	{
	/* wake up any processes waiting in bconin() */
		wake(IO_Q, (long)&console_in);
	/* wake anyone that did a select() on the keyboard */
		if (tty->rsel)
			wakeselect((PROC *)tty->rsel);
	}

	return ret;
}

/* EOF */
