/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to
 * the MiNT mailing list.
 * 
 */

# include "mint/mint.h"

# include "mint/asm.h"

# include "arch/acia.h"
# include "arch/cpu.h"		/* cpush() */
# include "arch/intr.h"		/* old_xxx */
# include "arch/kernel.h"	/* enter_gemdos() */
# include "arch/syscall.h"	/* new_xxx */
# include "arch/tosbind.h"	/* TRAP_xxx() */

# include "arch/init_intr.h"

# include "global.h"
# include "cookie.h"
# include "keyboard.h"		/* has_kbdvec */

/* magic number to show that we have captured the reset vector */
# define RES_MAGIC	0x31415926L

/* structures for keyboard/MIDI interrupt vectors */
KBDVEC *syskey;
static KBDVEC oldkey;

long old_term;
long old_resval;	/* old reset validation */
long olddrvs;		/* BIOS drive map */


/* table of processor frame sizes in _words_ (not used on MC68000) */
uchar framesizes[16] =
{
	/*0*/	0,	/* MC68010/M68020/M68030/M68040 short */
	/*1*/	0,	/* M68020/M68030/M68040 throwaway */
	/*2*/	2,	/* M68020/M68030/M68040 instruction error */
	/*3*/	2,	/* M68040 floating point post instruction */
	/*4*/	3,	/* MC68LC040/MC68EC040 unimplemented floating point instruction */
			/* or */
			/* MC68060 access error */
	/*5*/	0,	/* NOTUSED */
	/*6*/	0,	/* NOTUSED */
	/*7*/	26,	/* M68040 access error */
	/*8*/	25,	/* MC68010 long */
	/*9*/	6,	/* M68020/M68030 mid instruction */
	/*A*/	12,	/* M68020/M68030 short bus cycle */
	/*B*/	42,	/* M68020/M68030 long bus cycle */
	/*C*/	8,	/* CPU32 bus error */
	/*D*/	0,	/* NOTUSED */
	/*E*/	0,	/* NOTUSED */
	/*F*/	13	/* 68070 and 9xC1xx microcontroller address error */
};

/* New XBRA installer. The XBRA structure must be located
 * directly before the routine it belongs to.
 */

void
new_xbra_install (long *xv, long addr, long _cdecl (*func)())
{
	*xv = *(long *)addr;
	*(long *)addr = (long)func;

	/* better to be safe... */
# ifndef M68000
	cpush ((long *) addr, sizeof (addr)); 
	cpush (xv, sizeof (xv));
# endif
}

/*
 * initialize all interrupt vectors and new trap routines
 * we also get here any TOS variables that we're going to change
 * (e.g. the pointer to the cookie jar) so that rest_intr can
 * restore them.
 */

void
init_intr (void)
{
	ushort savesr;

	syskey = (KBDVEC *) TRAP_Kbdvbase ();
	oldkey = *syskey;

# ifndef NO_AKP_KEYBOARD
	if (!has_kbdvec) /* TOS versions without the KBDVEC vector */
	{
		/* We need to hook the ikbdsys vector. Our handler will have to deal
		 * with ACIA registers, and to call the appropriate KBDVEC vectors
		 * for keyboard, mouse, joystick, status and time packets. */
		savesr = splhigh();
		syskey->ikbdsys = (long)ikbdsys_handler;
#ifndef M68000
		cpush(&syskey->ikbdsys, sizeof(long));
#endif
		spl(savesr);
	}
	else
	{
		/* Hook the keyboard interrupt to call ikbd_scan() on keyboard data.
		 * There is an undocumented vector just before the KBDVEC structure.
		 * This vector is called by the TOS ikbdsys routine to process
		 * keyboard-only data. It is exactly what we need to hook.
		 * TOS < 2.00 doesn't know about this vector but the new ikdsys
		 * hadler hooked above if we're running over TOS < 2.00 will call it.
		 */
		long *kbdvec = ((long *)syskey)-1;
		new_xbra_install (&oldkeys, (long)kbdvec, newkeys);
	}

	/* Workaround for FireTOS and CT60 TOS 2.xx.
	 * Needed because those TOS doesn't call the undocumented kbdvec vector
	 * from their ikbdsys vector handler, besides they install the ikbdsys
	 * routine as a ACIA interrupt handler, so we can't simply replace their
	 * ikbdsys handler by ours. We need to hook a new ACIA handler which
	 * will call our ikbdsys.
	 */
	unsigned short version = 0;
#ifdef __mcoldfire__
	const unsigned short *FT_TOS_VERSION_ADDR = (unsigned short *)0x00e80000;
	if (coldfire_68k_emulation)
		version = *FT_TOS_VERSION_ADDR;
#else
	const unsigned short *CT60_TOS_VERSION_ADDR = (unsigned short *)0xffe80000;
	if (machine == machine_ct60)
		version = *CT60_TOS_VERSION_ADDR;
#endif
	if (version >= 2)
	{
		savesr = splhigh();
		syskey->ikbdsys = (long)ikbdsys_handler;
		cpush(&syskey->ikbdsys, sizeof(long));
		new_xbra_install(&old_acia, 0x0118L, new_acia);
		spl(savesr);
	}
# endif /* NO_AKP_KEYBOARD */

	old_term = (long) TRAP_Setexc (0x102, -1UL);

	savesr = splhigh();

	/* Take all traps; notice, that the "old" address for any trap
	 * except #1, #13 and #14 (GEMDOS, BIOS, XBIOS) is lost.
	 * It does not matter, because MiNT does not pass these
	 * calls along anyway.
	 * The main problem is, that activating this makes ROM VDI
	 * no more working, and actually any program that takes a
	 * trap before MiNT is loaded.
	 * WARNING: NVDI 5.00 uses trap #15 and isn't even polite
	 * enough to link it with XBRA.
	 */

	{
	long dummy;

	new_xbra_install (&dummy, 0x80L, unused_trap);		/* trap #0 */
	new_xbra_install (&old_dos, 0x84L, mint_dos);		/* trap #1, GEMDOS */	
# if 0	/* we only install this on request yet */
	new_xbra_install (&old_trap2, 0x88L, mint_trap2);	/* trap #2, GEM */
# endif
	new_xbra_install (&dummy, 0x8cL, unused_trap);		/* trap #3 */
	new_xbra_install (&dummy, 0x90L, unused_trap);		/* trap #4 */
	new_xbra_install (&dummy, 0x94L, unused_trap);		/* trap #5 */
	new_xbra_install (&dummy, 0x98L, unused_trap);		/* trap #6 */
	new_xbra_install (&dummy, 0x9cL, unused_trap);		/* trap #7 */
	new_xbra_install (&dummy, 0xa0L, unused_trap);		/* trap #8 */
	new_xbra_install (&dummy, 0xa4L, unused_trap);		/* trap #9 */
	new_xbra_install (&dummy, 0xa8L, unused_trap);		/* trap #10 */
	new_xbra_install (&dummy, 0xacL, unused_trap);		/* trap #11 */
	new_xbra_install (&dummy, 0xb0L, unused_trap);		/* trap #12 */
	new_xbra_install (&old_bios, 0xb4L, mint_bios);		/* trap #13, BIOS */
	new_xbra_install (&old_xbios, 0xb8L, mint_xbios);	/* trap #14, XBIOS */
# if 0
	new_xbra_install (&dummy, 0xbcL, unused_trap);		/* trap #15 */
# endif
	}

	new_xbra_install (&old_criticerr, 0x404L, (long (*)(void))new_criticerr);

	/* Hook the 200 Hz system timer. Our handler will do its job,
	 * then call the previous handler. Every four interrupts, our handler will
	 * push a fake additional exception stack frame, so when the previous
	 * 200 Hz handler returns with RTE, it will actually call _mint_vbl
	 * to mimic a 50 Hz VBL interrupt.
	 */

	new_xbra_install (&old_5ms, (long)p5msvec, mint_5ms);

#if 0	/* this should really not be necessary ... rincewind */
	new_xbra_install (&old_resvec, 0x042aL, reset);
	old_resval = *((long *)0x426L);
	*((long *) 0x426L) = RES_MAGIC;
#endif

	spl (savesr);

	/* set up signal handlers */
	new_xbra_install (&old_bus, 8L, new_bus);
	new_xbra_install (&old_addr, 12L, new_addr);
	new_xbra_install (&old_ill, 16L, new_ill);
	new_xbra_install (&old_divzero, 20L, new_divzero);
	new_xbra_install (&old_trace, 36L, new_trace);

	new_xbra_install (&old_priv, 32L, new_priv);

	if (tosvers >= 0x106)
		new_xbra_install (&old_linef, 44L, new_linef);

	new_xbra_install (&old_chk, 24L, new_chk);
	new_xbra_install (&old_trapv, 28L, new_trapv);

	new_xbra_install (&old_fpcp_0, 192L + (0 * 4), new_fpcp);
	new_xbra_install (&old_fpcp_1, 192L + (1 * 4), new_fpcp);
	new_xbra_install (&old_fpcp_2, 192L + (2 * 4), new_fpcp);
	new_xbra_install (&old_fpcp_3, 192L + (3 * 4), new_fpcp);
	new_xbra_install (&old_fpcp_4, 192L + (4 * 4), new_fpcp);
	new_xbra_install (&old_fpcp_5, 192L + (5 * 4), new_fpcp);
	new_xbra_install (&old_fpcp_6, 192L + (6 * 4), new_fpcp);

	new_xbra_install (&old_mmuconf, 224L, new_mmuconf);
	new_xbra_install (&old_pmmuill, 228L, new_mmu);
	new_xbra_install (&old_pmmuacc, 232L, new_pmmuacc);
	new_xbra_install (&old_format, 56L, new_format);
	new_xbra_install (&old_cpv, 52L, new_cpv);
	new_xbra_install (&old_uninit, 60L, new_uninit);
	new_xbra_install (&old_spurious, 96L, new_spurious);

	/* set up disk vectors */
	new_xbra_install (&old_mediach, 0x47eL, new_mediach);
	new_xbra_install (&old_rwabs, 0x476L, new_rwabs);
	new_xbra_install (&old_getbpb, 0x472L, new_getbpb);
	olddrvs = *((long *) 0x4c2L);

	/* we'll be making GEMDOS calls */
	enter_gemdos ();
}

/* restore all interrupt vectors and trap routines
 *
 * NOTE: This is *not* the approved way of unlinking XBRA trap handlers.
 * Normally, one should trace through the XBRA chain. However, this is
 * a very unusual situation: when MiNT exits, any TSRs or programs running
 * under MiNT will no longer exist, and so any vectors that they have
 * caught will be pointing to never-never land! So we do something that
 * would normally be considered rude, and restore the vectors to
 * what they were before we ran.
 * BUG: we should restore *all* vectors, not just the ones that MiNT caught.
 */

void
restr_intr (void)
{
	ushort savesr;

	savesr = splhigh();

	*syskey = oldkey;	/* restore keyboard vectors */

# ifndef NO_AKP_KEYBOARD
	if (tosvers < 0x0200)
	{
		*((long *) 0x0118L) = old_acia;
	}
	else
	{
		long *kbdvec = ((long *)syskey)-1;
		*kbdvec = (long) oldkeys;
	}
# endif

	*((long *) 0x008L) = old_bus;

	*((long *) 0x00cL) = old_addr;
	*((long *) 0x010L) = old_ill;
	*((long *) 0x014L) = old_divzero;
	*((long *) 0x024L) = old_trace;

	if (old_linef)
		*((long *) 0x2cL) = old_linef;

	*((long *) 0x018L) = old_chk;
	*((long *) 0x01cL) = old_trapv;

	((long *) 0x0c0L)[0] = old_fpcp_0;
	((long *) 0x0c0L)[1] = old_fpcp_1;
	((long *) 0x0c0L)[2] = old_fpcp_2;
	((long *) 0x0c0L)[3] = old_fpcp_3;
	((long *) 0x0c0L)[4] = old_fpcp_4;
	((long *) 0x0c0L)[5] = old_fpcp_5;
	((long *) 0x0c0L)[6] = old_fpcp_6;

	*((long *) 0x0e0L) = old_mmuconf;
	*((long *) 0x0e4L) = old_pmmuill;
	*((long *) 0x0e8L) = old_pmmuacc;
	*((long *) 0x038L) = old_format;
	*((long *) 0x034L) = old_cpv;
	*((long *) 0x03cL) = old_uninit;
	*((long *) 0x060L) = old_spurious;

	*((long *) 0x084L) = old_dos;
	*((long *) 0x0b4L) = old_bios;
	*((long *) 0x0b8L) = old_xbios;
	*((long *) 0x408L) = old_term;
	*((long *) 0x404L) = old_criticerr;
	*p5msvec = old_5ms;
#if 0	//
	*((long *) 0x426L) = old_resval;
	*((long *) 0x42aL) = old_resvec;
#endif
	*((long *) 0x476L) = old_rwabs;
	*((long *) 0x47eL) = old_mediach;
	*((long *) 0x472L) = old_getbpb;
	*((long *) 0x4c2L) = olddrvs;

	spl (savesr);
}

/* EOF */
