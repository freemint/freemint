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
# include "arch/tos_vars.h" /* Address of TOS variables */

# include "arch/init_intr.h"

# include "global.h"
# include "cookie.h"
# include "keyboard.h"		/* has_kbdvec */

/* magic number to show that we have captured the reset vector */
# define RES_MAGIC	0x31415926L

/* structures for keyboard/MIDI interrupt vectors */
KBDVEC *kbdvecs;
static long old_kbdvec;
static KBDVEC old_kbdvecs;

long old_term;
long old_resval;	/* old reset validation */
long old_drvbits;	/* BIOS drive map */

/* New XBRA installer. The XBRA structure must be located
 * directly before the routine it belongs to.
 * old_handler: will return the address of the previous handler for the vector
 * vector: address of the vector to set
 * new_handler: address of the new handler
 */

void
install_vector (long *old_handler, long vector, long _cdecl (*new_handler)())
{
	*old_handler = *(long *)vector;
	*(long *)vector = (long)new_handler;

	/* better to be safe... */
# ifndef M68000
	cpush ((long *) vector, sizeof (vector)); 
	cpush (old_handler, sizeof (old_handler));
# endif
}

/*
 * initialize all interrupt vectors and new trap routines
 * we also get here any TOS variables that we're going to change
 * (e.g. the pointer to the cookie jar) so that rest_intr can
 * restore them.
 */

void
install_TOS_vectors (void)
{
	ushort savesr;

	kbdvecs = (KBDVEC *) TRAP_Kbdvbase ();
	old_kbdvecs = *kbdvecs; /* structure copy */

# ifndef NO_AKP_KEYBOARD
	if (!has_kbdvec) /* TOS versions without the KBDVEC vector */
	{
		/* We need to hook the ikbdsys vector. Our handler will have to deal
		 * with ACIA registers, and to call the appropriate KBDVEC vectors
		 * for keyboard, mouse, joystick, status and time packets. */
		savesr = splhigh();
		kbdvecs->ikbdsys = (long)ikbdsys_handler;
#ifndef M68000
		cpush(&kbdvecs->ikbdsys, sizeof(long));
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
		long *kbdvec = ((long *)kbdvecs)-1;
		install_vector (&old_kbdvec, (long)kbdvec, kbdvec_handler);
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
		kbdvecs->ikbdsys = (long)ikbdsys_handler;
		cpush(&kbdvecs->ikbdsys, sizeof(long));
		install_vector(&old_acia, 0x0118L, new_acia);
		spl(savesr);
	}
# endif /* NO_AKP_KEYBOARD */

	/* Documentation says that we should set etv_term using Setexc (probably to give
	 * the OS a chance to maintain it per-program). We need to save it now, before we
	 * hook TRAP #13 */
	old_term = (long) TRAP_Setexc (ETV_TERM/4, -1UL);

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

	install_vector (&dummy, TRAP0, unused_trap);		/* trap #0 */
	install_vector (&old_dos, TRAP1, mint_dos);		/* trap #1, GEMDOS */	
# if 0	/* we only install this on request yet */
	install_vector (&old_trap2, TRAP2, mint_trap2);	/* trap #2, GEM */
# endif
	install_vector (&dummy, TRAP3, unused_trap);		/* trap #3 */
	install_vector (&dummy, TRAP4, unused_trap);		/* trap #4 */
	install_vector (&dummy, TRAP5, unused_trap);		/* trap #5 */
	install_vector (&dummy, TRAP6, unused_trap);		/* trap #6 */
	install_vector (&dummy, TRAP7, unused_trap);		/* trap #7 */
	install_vector (&dummy, TRAP8, unused_trap);		/* trap #8 */
	install_vector (&dummy, TRAP9, unused_trap);		/* trap #9 */
	install_vector (&dummy, TRAP10, unused_trap);		/* trap #10 */
	install_vector (&dummy, TRAP11, unused_trap);		/* trap #11 */
	install_vector (&dummy, TRAP12, unused_trap);		/* trap #12 */
	install_vector (&old_bios, TRAP13, mint_bios);		/* trap #13, BIOS */
	install_vector (&old_xbios, TRAP14, mint_xbios);	/* trap #14, XBIOS */
# if 0
	install_vector (&dummy, TRAP15, unused_trap);		/* trap #15 */
# endif
	}

	install_vector (&old_criticerr, ETV_CRITIC, (long (*)(void))new_criticerr);

	/* Hook the 200 Hz system timer. Our handler will do its job,
	 * then call the previous handler. Every four interrupts, our handler will
	 * push a fake additional exception stack frame, so when the previous
	 * 200 Hz handler returns with RTE, it will actually call _mint_vbl
	 * to mimic a 50 Hz VBL interrupt.
	 */

	install_vector (&old_5ms, (long)p5msvec, mint_5ms);

#if 0	/* this should really not be necessary ... rincewind */
	install_vector (&old_resvec, 0x042aL, reset);
	old_resval = *((long *)0x426L);
	*((long *) 0x426L) = RES_MAGIC;
#endif

	spl (savesr);

	/* set up signal handlers */
	install_vector (&old_bus, VEC_BUS_ERROR, new_bus);
	install_vector (&old_addr, VEC_ADDRESS_ERROR, new_addr);
	install_vector (&old_ill, VEC_ILLEGAL_INSTRUCTION, new_ill);
	install_vector (&old_divzero, VEC_DIVISION_BY_ZERO, new_divzero);
	install_vector (&old_trace, VEC_TRACE, new_trace);

	install_vector (&old_priv, VEC_PRIVILEGE_VIOLATION, new_priv);

	if (tosvers >= 0x106)
		install_vector (&old_linef, VEC_LINE_F, new_linef);

	install_vector (&old_chk, VEC_CHK, new_chk);
	install_vector (&old_trapv, VEC_TRAPV, new_trapv);

	install_vector (&old_fpcp_0, VEC_FFCP0, new_fpcp);
	install_vector (&old_fpcp_1, VEC_FFCP1, new_fpcp);
	install_vector (&old_fpcp_2, VEC_FFCP2, new_fpcp);
	install_vector (&old_fpcp_3, VEC_FFCP3, new_fpcp);
	install_vector (&old_fpcp_4, VEC_FFCP4, new_fpcp);
	install_vector (&old_fpcp_5, VEC_FFCP5, new_fpcp);
	install_vector (&old_fpcp_6, VEC_FFCP6, new_fpcp);

	install_vector (&old_mmuconf, VEC_MMU_CONFIG_ERROR, new_mmuconf);
	install_vector (&old_pmmuill, VEC_MMU1, new_mmu);
	install_vector (&old_pmmuacc, VEC_MMU2, new_pmmuacc);
	install_vector (&old_format, VEC_FORMAT_ERROR, new_format);
	install_vector (&old_cpv, VEC_COPRO_PROTOCOL_VIOLATION, new_cpv);
	install_vector (&old_uninit, VEC_UNINITIALIZED, new_uninit);
	install_vector (&old_spurious, VEC_SPURIOUS_INTERRUPT, new_spurious);

	/* set up disk vectors */
	install_vector (&old_mediach, HDV_MEDIACH, new_mediach);
	install_vector (&old_rwabs, HDV_RW, new_rwabs);
	install_vector (&old_getbpb, HDV_BPB, new_getbpb);
	old_drvbits = *((long *) _DRVBITS);

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
restore_TOS_vectors (void)
{
	ushort savesr;

	savesr = splhigh();

	*kbdvecs = old_kbdvecs;	/* restore keyboard vectors (structure copy) */

# ifndef NO_AKP_KEYBOARD
	if (tosvers < 0x0200)
	{
		*((long *) 0x0118L) = old_acia;
	}
	else
	{
		long *kbdvec = ((long *)kbdvecs)-1;
		*kbdvec = (long) old_kbdvec;
	}
# endif

	*((long *) VEC_BUS_ERROR) = old_bus;
	*((long *) VEC_ADDRESS_ERROR) = old_addr;
	*((long *) VEC_ILLEGAL_INSTRUCTION) = old_ill;
	*((long *) VEC_DIVISION_BY_ZERO) = old_divzero;
	*((long *) VEC_TRACE) = old_trace;

	if (old_linef)
		*((long *) VEC_LINE_F) = old_linef;

	*((long *) VEC_CHK) = old_chk;
	*((long *) VEC_TRAPV) = old_trapv;

	*((long *) VEC_FFCP0) = old_fpcp_0;
	*((long *) VEC_FFCP1) = old_fpcp_1;
	*((long *) VEC_FFCP2) = old_fpcp_2;
	*((long *) VEC_FFCP3) = old_fpcp_3;
	*((long *) VEC_FFCP4) = old_fpcp_4;
	*((long *) VEC_FFCP5) = old_fpcp_5;
	*((long *) VEC_FFCP6) = old_fpcp_6;

	*((long *) VEC_MMU_CONFIG_ERROR) = old_mmuconf;
	*((long *) VEC_MMU1) = old_pmmuill;
	*((long *) VEC_MMU2) = old_pmmuacc;
	*((long *) VEC_FORMAT_ERROR) = old_format;
	*((long *) VEC_COPRO_PROTOCOL_VIOLATION) = old_cpv;
	*((long *) VEC_UNINITIALIZED) = old_uninit;
	*((long *) VEC_SPURIOUS_INTERRUPT) = old_spurious;

	*((long *) TRAP1) = old_dos;
	*((long *) TRAP13) = old_bios;
	*((long *) TRAP14) = old_xbios;

	*((long *) ETV_TERM) = old_term;
	*((long *) ETV_CRITIC) = old_criticerr;
	*p5msvec = old_5ms;
#if 0	//
	*((long *) RESVALID) = old_resval;
	*((long *) RESVECTOR) = old_resvec;
#endif
	*((long *) HDV_RW) = old_rwabs;
	*((long *) HDV_MEDIACH) = old_mediach;
	*((long *) HDV_BPB) = old_getbpb;
	*((long *) _DRVBITS) = old_drvbits;

	spl (savesr);
}


long _cdecl
register_trap2(long _cdecl (*dispatch)(void *), int mode, int flag, long extra)
{
	long _cdecl (**handler)(void *) = NULL;
	long *x = NULL;
	long ret = EINVAL;

	DEBUG(("register_trap2(0x%p, %i, %i)", dispatch, mode, flag));

	if (flag == 0)
	{
		handler = &aes_handler;
	}
	else if (flag == 1)
	{
		handler = &vdi_handler;
		x = &gdos_version;
	}

	if (mode == 0)
	{
		/* install */

		if (*handler == NULL)
		{
			DEBUG(("register_trap2: installing handler at 0x%p", dispatch));

			*handler = dispatch;
			if (x)
				*x = extra;
			ret = 0;

			/* if trap #2 is not active install it now */
			if (old_trap2 == 0)
				install_vector(&old_trap2, TRAP2, mint_trap2); /* trap #2, GEM */
		}
	}
	else if (mode == 1)
	{
		/* deinstall */

		if (*handler == dispatch)
		{
			DEBUG(("register_trap2: removing handler at 0x%p", dispatch));

			*handler = NULL;
			if (x)
				*x = 0;
			ret = 0;
		}
	}

	return ret;
}

/* EOF */
