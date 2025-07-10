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
 * XBIOS replacement routines
 *
 */

/* Note: most of security code moved to syscall.spp */

# include "xbios.h"
# include "global.h"

# include "mint/arch/mfp.h"
# include "mint/asm.h"
# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/signal.h"

# include "arch/detect.h"
# include "arch/mprot.h"
# include "arch/syscall.h"
# include "arch/timer.h"
# include "arch/tosbind.h"

# include "bios.h"
# include "biosfs.h"
# include "init.h"
# include "k_prot.h"
# include "memory.h"
# include "proc.h"
# include "signal.h"
# include "time.h"	/* xtime */
# include "tty.h"

# include <stddef.h>	/* offsetof */


/* NOTE: has_bconmap is initialized in main.c */

int has_bconmap;	/* flag: set if running under a version
			 * of TOS which supports Bconmap
			 */


/*
 * Supexec() presents a lot of problems for us: for example, the user
 * may be calling the kernel, or may be changing interrupt vectors
 * unexpectedly. So we play some dirty tricks here: the function
 * call is treated like a signal handler, and we take advantage
 * of the fact that no context switches will take place while
 * in supervisor mode. ASSUMPTION: the user will not choose to
 * switch back to user mode, or if s/he does it will be as part
 * of a longjmp().
 *
 * BUG: if the user function switches to user mode, then back to
 * supervisor mode and returns, then the returned value may be
 * inaccurate (this happens if two programs make Supexec calls
 * at the same time).
 */

long _cdecl (*usrcall) (long, long, long, long, long, long);
long usrret;
long usrarg1, usrarg2, usrarg3, usrarg4, usrarg5;

# if 0 /* moved to syscall.spp */

static void _cdecl do_usrcall (void);

static void _cdecl
do_usrcall (void)
{
	usrret = (*usrcall)((long) usrcall, usrarg1, usrarg2, usrarg3, usrarg4, usrarg5);
}
# endif

long _cdecl
sys_b_supexec (Func funcptr, long arg1, long arg2, long arg3, long arg4, long arg5)
{
	PROC *p = get_curproc();
	CONTEXT *syscall = &p->ctxt[SYSCALL];
	ushort savesr;

	if (funcptr == NULL)
	{
		TRACE(("Supexec() error no valid function"));
		return EPERM;
	}
	/* For SECURELEVEL > 1 only the Superuser can set the CPU into supervisor
	 * mode.
	 */

	if ((secure_mode > 1) && !suser (p->p_cred->ucr))
	{
		DEBUG (("Supexec: user call"));
		raise (SIGSYS);
		return EPERM;
	}

	/* set things up so that "signal 0" will be handled by calling the user's
	 * function.
	 */
	
	assert (p->p_sigacts);
	
	usrcall = (long _cdecl (*) (long, long, long, long, long, long)) funcptr;
	usrarg1 = arg1;
	usrarg2 = arg2;
	usrarg3 = arg3;
	usrarg4 = arg4;
	usrarg5 = arg5;
	SIGACTION(p, 0).sa_handler = (long) do_usrcall;
	savesr = syscall->sr;	/* save old super/user mode flag */
	syscall->sr |= 0x2000;	/* set supervisor mode */

	handle_sig (0);		/* actually call out to the user function */

	syscall->sr = savesr;

	/* do_usrcall saves the user's return value in usrret */
	return usrret;
}


/*
 * midiws: we have to replace this, because it's possible that the process'
 * view of what the MIDI port is has been changed by Fforce or Fmidipipe
 */

long _cdecl
sys_b_midiws (int cnt, const char *buf)
{
	FILEPTR *f;
	long towrite = cnt+1;

	f = get_curproc()->p_fd->midiout;	/* MIDI output handle */
	if (!f)
		return EBADF;

	if (is_terminal (f))
	{
		/* see if we can do fast RAW byte IO thru the device driver... */
		if ((f->fc.fs != &bios_filesys ||
			(towrite > 1 &&
			 ((struct bios_file *)f->fc.index)->drvsize >
				offsetof (DEVDRV, writeb))) && f->dev->writeb)
		{
			struct tty *tty = (struct tty *)f->devinfo;

			tty_checkttou (f, tty);
			tty->state &= ~TS_COOKED;
			if ((towrite = (*f->dev->writeb)(f, buf, towrite)) != ENODEV)
				return towrite;
		}

		while (cnt >= 0)
		{
			tty_putchar(f, (long)*buf, RAW);
			buf++; cnt--;
		}

		return towrite;
	}

	return (*f->dev->write)(f, buf, towrite);
}

/*
 * Modem control things: these are replaced because we handle
 * Bconmap ourselves
 */

/* mapin: utility routine, does a Bconmap and keeps track
 * so we call the kernel only when necessary; call this
 * only if has_bconmap is "true".
 * Returns: 0 on failure, 1 on success.
 */
int curbconmap;

static int
mapin (int dev)
{
	long r;

	if (dev == curbconmap)
		return 1;

	if (intr_done)
		r = ROM_Bconmap (dev);
	else
		r = TRAP_Bconmap (dev);
	if (r)
	{
		curbconmap = dev;
		return 1;
	}

	return 0;
}

IOREC_T *_cdecl
sys_b_uiorec (short dev)
{
	TRACE (("Iorec(%d)", dev));

	if (dev == 0 && has_bconmap)
	{
		PROC *p;

		// ALERT ("Iorec(%d) -> NULL", dev);
		return 0;

		/* get around another BIOS Bug:
		 * in (at least) TOS 2.05 Iorec(0) is broken
		 */
		p = get_curproc();
		if ((unsigned) p->p_fd->bconmap - 6 < btty_max)
			return MAPTAB[p->p_fd->bconmap-6].iorec;

		mapin (p->p_fd->bconmap);
	}

	return (IOREC_T *)ROM_Iorec (dev);
}

long _cdecl
rsconf (int baud, int flow, int uc, int rs, int ts, int sc)
{
	PROC *p = get_curproc();
	long rsval;
#ifdef MFP_DEBUG_DIRECT
	static int oldbaud = 0;
#else
	static int oldbaud = -1;
#endif
	unsigned b = 0;
	struct bios_tty *t = bttys;

	TRACE (("Rsconf(%d,%d,%d,%d,%d,%d)", baud, flow, uc, rs, ts, sc));

	if (has_bconmap)
	{
		b = p->p_fd->bconmap - 6;
		if (b < btty_max)
			t += b;
		else
			t = 0;

		/* more bugs...  serial1 is three-wire, requesting hardware
		 * flowcontrol on it can confuse BIOS
		 */
		if ((flow & 0x8002) == 2 && t && t->tty == &ttmfp_tty)
			flow &= ~2;
	}
	else if (tosvers <= 0x0199)
	{
		/*
		 * If this is an old TOS, try to rearrange things to support
		 * the following Rsconf() features:
		 *
		 * 1. Rsconf(-2, ...) does not return current baud (it crashes)
		 *    -> keep track of old speed in static variable
		 * 2. Rsconf(b, ...) sends ASCII DEL to the modem unless b == -1
		 *    -> make speed parameter -1 if new speed matches old speed
		 * 3. Rsconf() discards any buffered output
		 *    -> use Iorec() to ensure all buffered data was sent before call
		 */
		if (baud == -2)
		{
			/* TOS 1.04 Rsconf ignores its other args when asked
			 * for old speed, so can we
			 */
			return oldbaud;
		}
		else if (baud == oldbaud)
			baud = -1;
		else if (baud > -1)
			oldbaud = baud;
	}

	if (t && baud != -2)
	{
		while (t->tty->hup_ospeed)
			sleep (IO_Q, (long) &t->tty->state);
	}

	if (has_bconmap && t)
	{
		rsval = MAPTAB[b].rsconf;

		/* bug # x+1:  at least up to TOS 2.05 SCC Rsconf forgets to or #0x700,sr...
		 * use MAPTAB to call it directly, at ipl7 if it points to ROM
		 *
		 * Note that FireTOS maps ColdFire's PSC0 serial port to BIOS device 7, and
		 * it will hit the condition below, it should be no problem but just in case.
		 */
		if (baud > -2
#ifdef __mcoldfire__
			&& !coldfire_68k_emulation
#endif
			&& (b == 1 || t->tty == &scca_tty)
			&& (b = 1, rsval > 0xe00000L) && rsval < 0xefffffL)
		{
			rsval = callout6spl7 (rsval, baud, flow, uc, rs, ts, sc);
		}
		else
			rsval = callout6 (rsval, baud, flow, uc, rs, ts, sc);
	}
	else
	{
		if (has_bconmap)
			mapin (p->p_fd->bconmap);

		if (intr_done)
			rsval = ROM_Rsconf (baud, flow, uc, rs, ts, sc);
		else
			rsval = TRAP_Rsconf (baud, flow, uc, rs, ts, sc);
	}

	if (!t || baud <= -2)
		return rsval;

	if (baud >= 0)
	{
		t->vticks = 0;
		t->ospeed = t->ispeed = (unsigned)baud < t->maxbaud ?
					t->baudmap[baud] : -1;
	}

# if 1
	if (b == 1 && flow >= 0)
	{
# ifndef MILAN
		/* SCC can observe CD and CTS in hardware (w3 bit 5),
		 * turn on if TF_CAR and T_RTSCTS
		 */
		ushort sr;
		volatile char dummy, *control;
		unsigned char w3;

		control = (volatile char *) (t->tty == &scca_tty ? ControlRegA : ControlRegB);
		w3 = ((((uchar *) t->irec)[0x1d] << 1) & 0xc0) |
			((!t->clocal &&
			 (((uchar *) t->irec)[0x20] & 2)) ? 0x21 : 0x1);

		sr = spl7();
		dummy = _mfpregs->gpip;
		*control = 3;
		dummy = _mfpregs->gpip;
		UNUSED (dummy);
		*control = w3;
		spl (sr);
# endif
	}
# endif
	return rsval;
}

long _cdecl
sys_b_bconmap (short dev)
{
	PROC *p = get_curproc();
	int old = p->p_fd->bconmap;

	TRACE (("Bconmap(%d)", dev));

	if (has_bconmap)
	{
		if (dev == -2)
		{
# if 0
			ALERT ("Bconmap(%d)", dev);
			return 0;
# else
			return ROM_Bconmap(-2);
# endif
		}

		if (dev == -1)
			return old;

		if (dev == 0)
			return E_OK;  /* the user's just testing */

		if (mapin (dev) == 0)
		{
			DEBUG (("Bconmap: mapin(%d) failed", dev));
			return 0;
		}

		if (set_auxhandle (p, dev) == 0)
		{
			DEBUG (("Bconmap: Couldn't change AUX:"));
			return 0;
		}

		p->p_fd->bconmap = dev;
		return old;
	}

	return ENOSYS;	/* no Bconmap available */
}

/*
 * cursconf(): this gets converted into an ioctl() call on
 * the appropriate device
 */

long _cdecl
sys_b_cursconf (int cmd, int op)
{
	FILEPTR *f;
	long r;

	f = get_curproc()->p_fd->control;
	if (!f || !is_terminal(f))
		return ENOSYS;

	r = (*f->dev->ioctl)(f, TCURSOFF + cmd, &op);
	if ((cmd == CURS_GETRATE || cmd == 7 /* undocumented! */) && r == E_OK)
		return op;

	return r;
}


long _cdecl
sys_b_dosound (const char *p)
{
	union { volatile char *vc; const char *cc; long l; } ptr; ptr.cc = p;
# ifdef WITH_MMU_SUPPORT
	if (!no_mem_prot && ptr.l >= 0)
	{
		MEMREGION *r;

		/* check that this process has access to the memory
		 * (if not, the next line will cause a bus error)
		 */
		(void)*ptr.vc;
		//(void)(*((volatile char *) ptr));

		/* OK, now make sure that interrupt routines will have access,
		 * too
		 */
		r = addr2region (ptr.l);// (unsigned long)ptr);
		if (r && get_prot_mode (r) == PROT_P)
		{
			DEBUG (("Dosound: changing protection to Super"));
			mark_region (r, PROT_S, 0);
		}
	}
# endif

	ROM_Dosound (ptr.cc);

	return E_OK;
}

# if 0

/* Draco: patch for Videl monochrome bug in Falcons,
 * posted by Thomas Binder (Gryf)
 */

static void
videl_patch (void)
{
	ushort s;

	s = *(ushort *) 0xffff8266L;
	ROM_Vsync();
	*(ushort *) 0xffff8266L = 0;
	ROM_Vsync();
	*(ushort *) 0xffff8266L = s;
}
# endif

/* Draco: STE shifter patch from Daniel Petersson <tam@dataphone.se>
 */
static void
shifter_patch(void)
{
	*(ushort *) 0xffff8264L = 0;
	ROM_Vsync();
	*(ushort *) 0xffff8264L = 0;
}

# if 0
long _cdecl
vsetmode (int modecode)
{
	long r;

	if (FalconVideo && (modecode != -1) && ((modecode & 0x0007) == 0))
	{
		ROM_Vsync();
		r = ROM_VsetMode(modecode);
		videl_patch();
	}
	else
		r = ROM_VsetMode(modecode);

	return r;
}
# endif

/* this code is not executed on Falcons. see XBIOS kludges section
 * in syscall.spp
 */

long _cdecl
sys_b_vsetscreen (long log, long phys, int rez, int mode)
{
	/* STE shifter ST-high syncbug workaround */
	ROM_VsetScreen (log, phys, rez, mode);
	if (ste_video && rez == 2)
		shifter_patch();

	return E_OK;

}

/* This random number generator was taken from the xscreensaver
 * distribution and adapted to MiNT by Guido Flohr.  The xscreensaver
 * sources have the following copyright notice (but the original
 * file yarandom.c from xscreensaver does _not_ have this message).
 * Thus, I'm not sure if it applies here or not.  Well, I include
 * it anyway.
 */

/* xscreensaver, Copyright (c) 1997 by Jamie Zawinski <jwz@netscape.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

/* yarandom.c -- Yet Another Random Number Generator.
 *
 * The unportable mess that is rand(), random(), drand48() and friends led me
 * to ask Phil Karlton <karlton@netscape.com> what the Right Thing to Do was.
 * He responded with this.  It is non-cryptographically secure, reasonably
 * random (more so than anything that is in any C library), and very fast.
 *
 * I don't understand how it works at all, but he says "look at Knuth,
 * Vol. 2 (original edition), page 26, Algorithm A.  In this case n=55,
 * k=20 and m=2^32."
 *
 * So there you have it.
 *
 * ---------------------------
 * Note: xlockmore 4.03a10 uses this very simple RNG:
 *
 *      if ((seed = seed % 44488 * 48271 - seed / 44488 * 3399) < 0)
 *        seed += 2147483647;
 *      return seed-1;
 *
 * of which it says
 *
 *      ``Dr. Park's algorithm published in the Oct. '88 ACM  "Random Number
 *        Generators: Good Ones Are Hard To Find" His version available at
 *        ftp://cs.wm.edu/pub/rngs.tar Present form by many authors.''
 *
 * Karlton says: ``the usual problem with that kind of RNG turns out to
 * be unexepected short cycles for some word lengths.''
 *
 * Karlton's RNG is faster, since it does three adds and two stores, while the
 * xlockmore RNG does two multiplies, two divides, three adds, and one store.
 *
 * Compiler optimizations make a big difference here:
 *     gcc -O:     difference is 1.2x.
 *     gcc -O2:    difference is 1.4x.
 *     gcc -O3:    difference is 1.5x.
 *     SGI cc -O:  difference is 2.4x.
 *     SGI cc -O2: difference is 2.4x.
 *     SGI cc -O3: difference is 5.1x.
 * Irix 6.2; Indy r5k; SGI cc version 6; gcc version 2.7.2.1.
 */

/* The following 'random' numbers are taken from CRC, 18th Edition, page 622.
 * Each array element was taken from the corresponding line in the table,
 * except that a[0] was from line 100. 8s and 9s in the table were simply
 * skipped. The high order digit was taken mod 4.
 */
# define VectorSize 55
static ulong a[VectorSize] =
{
	035340171546, 010401501101, 022364657325, 024130436022, 002167303062, /*  5 */
	037570375137, 037210607110, 016272055420, 023011770546, 017143426366, /* 10 */
	014753657433, 021657231332, 023553406142, 004236526362, 010365611275, /* 15 */
	007117336710, 011051276551, 002362132524, 001011540233, 012162531646, /* 20 */
	007056762337, 006631245521, 014164542224, 032633236305, 023342700176, /* 25 */
	002433062234, 015257225043, 026762051606, 000742573230, 005366042132, /* 30 */
	012126416411, 000520471171, 000725646277, 020116577576, 025765742604, /* 35 */
	007633473735, 015674255275, 017555634041, 006503154145, 021576344247, /* 40 */
	014577627653, 002707523333, 034146376720, 030060227734, 013765414060, /* 45 */
	036072251540, 007255221037, 024364674123, 006200353166, 010126373326, /* 50 */
	015664104320, 016401041535, 016215305520, 033115351014, 017411670323  /* 55 */
};

static int i1, i2;

long _cdecl
sys_b_random (void)
{
	register long ret = a[i1] + a[i2];
	a[i1] = ret;

	if (++i1 >= VectorSize) i1 = 0;
	if (++i2 >= VectorSize) i2 = 0;

	/* Sigh, we have to return  24-bit random number for compatibility
	 * reasons.  BTW, is it faster to rightshift by 8 bits or do the
	 * logical AND? (Guido)
	 *
	 * Draco: the shift is faster, at least on 68020+.
	 * The cast is needed to force using lsr instead of asr.
	 */
	return (ulong) ret >> 8;
}


static void
init_xrandom (void)
{
	int i;

	/* Ignore overflow when initializing seed on purpose.  The original
	 * algorithm also added (1003 * getpid ()) but this will always
	 * add zero (because that's the kernel's pid.  If there's another
	 * pool of entropy bits hidden in the kernel this should be added
	 * here.
	 */
	ulong seed;

	seed  = ( 999 * xtime.tv_sec);
	seed += (1001 * xtime.tv_usec);
	seed += (1003 * jiffies);

	a[0] += seed;

	for (i = 1; i < VectorSize; i++)
	{
		seed = a[i-1] * 1001 + seed * 999;
		a[i] += seed;
	}

	i1 = a[0] % VectorSize;
	i2 = (i1 + 024) % VectorSize;
}

static void
init_bconmap (void)
{
#ifdef __mcoldfire__
	/* We don't support Bconmap() for ColdFire targets. EmuTOS doesn't
	 * implement Bconmap() for ColdFire targets, FireTOS does it but has
	 * the problem describe below.
	 *
	 * This function calls Rsconf() and there are some bugs in FireTOS
	 * and EmuTOS Rsconf() implementations. FireTOS returns always the value
	 * 5 when inquiring for the baud rate (Rsconf(-2,....)). And EmuTOS
	 * doesn't set the values for the deafult serial port (PSC0) but for
	 * the faulty FireBee's MFP.
	 */

	return;
#else
	int i, oldmap;
	PROC *p = get_curproc();

	curbconmap = (has_bconmap) ? (int) TRAP_Bconmap (-1) : 1;

	oldmap = p->p_fd->bconmap = curbconmap;
	for (i = 0; i < btty_max; i++)
	{
		int r;
		if (has_bconmap)
			p->p_fd->bconmap = i + 6;

		r = (int) rsconf (-2, -1, -1, -1, -1, -1);
		if (r < 0)
		{
			if (has_bconmap)
				mapin (p->p_fd->bconmap);
			TRAP_Rsconf ((r = 0), -1, -1, -1, -1, -1);
		}

		rsconf (r, -1, -1, -1, -1, -1);
	}

	if (has_bconmap)
		mapin (p->p_fd->bconmap = oldmap);
#endif /* __mcoldfire__ */
}

void
init_xbios(void)
{
	/* init XBIOS Random() function */
	init_xrandom();
	
	/* init bconmap stuff */
	init_bconmap();
}
