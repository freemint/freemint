/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
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
 * Author: Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
 * Started: 1999-08-17
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 *
 * Purpose:
 * Contains assembler subroutines needed for Slbopen(), Slbclose(), ...
 *
 * History:
 * 00/05/02: - slb_exec now reads *tosbp to get the basepage address (Gryf)
 * 99/08/18: - Changed slb_exec to pass the basepage of the calling process,
 *             instead of its PID (Gryf)
 * 99/08/11-
 * 99/08/17: - Creation (Gryf)
 *
 */

#include "magic/magic.i"

	.text

	.globl	_slb_init_and_exit

_slb_init_and_exit:

	pea	0x01190001.l		// Pdomain(1)
	trap	#1

	move.w	#0x0004,d7

loop:	move.w	d7,-(sp)		// Fclose(x)
	move.w	#0x003e,-(sp)
	trap	#1

	dbra.w	d7,loop

	clr.l	-(sp)			// Psetpgrp(0,0)
	move.w	#0x010e,-(sp)
	trap	#1

// 30 bytes of junk on the stack here

	move.l	34(sp),a0		// basepage pointer
	lea	256(a0),a0		// begin of the TEXT segment
	cmp.l	#0x283a001a,(a0)
	bne.s	oldfmt
	cmp.l	#0x4efb48fa,4(a0)
	bne.s	oldfmt

	lea	228.w(a0),a0		// the header is a bit bigger (new format)

oldfmt:	move.l	a0,-(sp)

	cmp.l	#0x70004afc,(a0)	// a0 = SLB header address
	beq.s	exec

	move.l	38(sp),a0
	move.l	#-1,128(a0)		// put -1 on the bp->p_cmdlin
	bra.s	halt

exec:	move.l	16(a0),a0
	jsr	(a0)

	move.l	38(sp),a0
	move.l	d0,128(a0)		// return value

halt:	move.w	#0x0011,-(sp)
	clr.w	-(sp)
	move.w	#0x0111,-(sp)
	trap	#1
	addq.l	#6,sp

	move.l	(sp)+,a0		// header address
	move.l	20(a0),a0
	jsr	(a0)

	clr.w	-(sp)
	trap	#1

// slb_open
//
// Fake routine for returning from Slbopen(). Calls the library's open()
// function in user mode and in the context of the calling process.
//
// Input:
// 0(sp): Pointer to shared library header
// 4(sp): Pointer to basepage of current process
// 8(sp): Pointer to name of shared library structure
//
// Returns:
// d0: Version number of the shared library, or GEMDOS error code

	.globl	_slb_open

_slb_open:
	move.l	(sp),a0
	move.l	4(sp),d0
	move.l	d0,-(sp)
	move.l	SH_OPEN(a0),a1
	jsr	(a1)
	addq.l	#4,sp
	tst.l	d0
	bmi.s	failed
	move.l	8(sp),a0
	lea	SL_NAME(a0),a0
	clr.l	-(sp)
	clr.l	-(sp)
	clr.l	-(sp)
	clr.l	-(sp)
	move.l	a0,-(sp)
	move.w	#0x0016,-(sp)
	trap	#1
	lea	34(sp),sp
	rts

failed:	move.l	8(sp),a0
	move.l	d0,-(sp)
	move.l	a0,-(sp)
	move.w	#0x0017,-(sp)
	trap	#1
	addq.l	#6,sp
	move.l	(sp)+,d0
	lea	12(sp),sp
	rts

// slb_close
//
// Fake routine for returning from Slbclose(). Calls the library's close()
// function in user mode and in the context of the calling process.
//
// Input:
// 0(sp): Pointer to basepage of current process
// 4(sp): Pointer to shared library structure, as passed to Slbclose()
//
// Returns:
// d0: Version number of the shared library, or GEMDOS error code

	.globl	_slb_close

_slb_close:
	move.l	4(sp),a0
	move.l	SL_HEAD(a0),a0
	move.l	SH_CLOSE(a0),a0
	move.l	(sp),d0
	move.l	d0,-(sp)
	jsr	(a0)
	addq.l	#4,sp
	move.l	4(sp),a0
	move.l	a0,-(sp)
	move.w	#0x0017,-(sp)
	trap	#1
	lea	14(sp),sp
	rts

// slb_close_and_pterm
//
// Like slb_close(), but instead of calling Slbclose() again, Pterm() is called.
// This function is used when a process exited without calling Slbclose().

	.globl	_slb_close_and_pterm

_slb_close_and_pterm:
	move.l	d0,-(sp)
	move.l	8(sp),a0
	move.l	SL_HEAD(a0),a0
	move.l	SH_CLOSE(a0),a0
	move.l	4(sp),d0
	move.l	d0,-(sp)
	jsr	(a0)
	addq.l	#4,sp
	move.l	(sp)+,d0
	addq.l	#8,sp
	move.w	d0,-(sp)
	move.w	#0x004c,-(sp)
	trap	#1
	addq.l	#4,sp
	rts

// slb_exec
//
// Helper function to call an SLB's function from within an application.
//
// Input:
// 4(sp): Pointer to shared library structure
// 8(sp): Function number (long)
// 12(sp): Number of arguments (short)
// 14(sp): arguments (if any)
//
// Returns:
// d0: EINVFN: Function not implemented (or function number out of range)
//     Otherwise: Return code of function
//
// slb_fast is an alternative stub which avoids trapping to
// GEMDOS at each library call.

	.globl	_slb_exec
	.globl	_tosbp

_slb_exec:
	move.l	_tosbp,a0
	move.l	(a0),d1
	move.l	8(sp),d0
	bmi.s	einvfn
	move.l	4(sp),a0
	move.l	SL_HEAD(a0),a0
	cmp.l	SH_NO_FUNCS(a0),d0
	bcc.s	einvfn
#ifdef ONLY030
	move.l	SH_FUNCTIONS(a0,d0.l*4),d0
#else
	lsl.l	#2,d0
	move.l	SH_FUNCTIONS(a0,d0.l),d0
#endif
	beq.s	einvfn
	move.l	d0,a0
	move.l	d1,4(sp)
	jmp	(a0)

einvfn:
	moveq	#-32,d0
	rts

// EOF