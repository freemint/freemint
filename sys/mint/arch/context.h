/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 2000-10-08
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_m68k_context_h
# define _mint_m68k_context_h

# include "mmu.h"

/*
 * a process context consists, for now, of its registers
 */

struct context
{
	unsigned long	regs[15];	/* registers d0-d7, a0-a6 */
	unsigned long	usp;		/* user stack pointer (a7) */
	unsigned short	sr;		/* status register */
	unsigned long	pc;		/* program counter */
	unsigned long	ssp;		/* supervisor stack pointer */
	unsigned long	term_vec;	/* GEMDOS terminate vector (0x102) */
	
	/* AGK: if running on a TT and the user is playing with the FPU then we
	 * must save and restore the context. We should also consider this for
	 * I/O based co-processors, although this may be difficult due to
	 * possibility of a context switch in the middle of an I/O handshaking
	 * exchange.
	 */
	union {
		unsigned char	bytes[216];	/* FPU internal state */
		unsigned short  words[108];
		unsigned long	longs[54];
	} fstate;
	unsigned long	fregs[3*8];	/* registers fp0-fp7 */
	unsigned long	fctrl[3];	/* FPCR/FPSR/FPIAR */
	char		ptrace;		/* trace exception is pending */
	unsigned char	pad1;		/* junk */
	unsigned long	iar;		/* unused */
	unsigned long	res[2];		/* unused, reserved */
	
	/* Saved CRP and TC values. These are necessary for memory protection.
	 */
	crp_reg	crp;		/* 64 bits */
	tc_reg	tc;		/* 32 bits */
	
	/* AGK: for long (time-wise) co-processor instructions (FMUL etc.), the
	 * FPU returns NULL, come-again with interrupts allowed primitives. It
	 * is highly likely that a context switch will occur in one of these if
	 * running a mathematically intensive application, hence we must handle
	 * the mid-instruction interrupt stack. We do this by saving the extra
	 * 3 long words and the stack format word here.
	 */
	unsigned short	sfmt;		/* stack frame format identifier */
	unsigned short	internal[42];	/* internal state -- see framesizes[] for size */
};

# define PROC_CTXTS	2
# define SYSCALL	0	/* saved context from system call */
# define CURRENT	1	/* current saved context */

# define FRAME_MAGIC	0xf4a3e000UL
				/* magic for signal call stack */
# define CTXT_MAGIC	0xabcdef98UL
# define CTXT2_MAGIC	0x87654321UL
				/* magic #'s for contexts */


# endif /* _mint_m68k_context_h */
