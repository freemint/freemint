/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 2003 Konrad M. Kokoszkiewicz <draco@atari.org>
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
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 * Started: 26.II.2003.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

#include "magic/magic.i"

	.text

// Some sort of a header first

	.globl	_user_header

_user_header:
	.long	0xdeadbabe		// nice magic value first, for validation
	.long	0x00000100		// version number
	.long	user_end-_user_header	// total number of bytes (to copy)
	.long	0			// user process' basepage pointer (to be filled)
	.long	8			// number of valid addresses in the jumptable below

// now the jumptable, with relative addresses

	.long	_user_terminateme-_user_header
	.long	_user_sig_return-_user_header
	.long	_user_pc_valid_return-_user_header
	.long	_user_slb_init_and_exit-_user_header
	.long	_user_slb_open-_user_header
	.long	_user_slb_close-_user_header
	.long	_user_slb_close_and_pterm-_user_header
	.long	_user_slb_exec-_user_header
	.long	0

_user_terminateme:
	clr.w	-(sp)
	trap	#1

// sig_return: user signal handlers return to us. At that point, the
// stack looks like this:
//  -4(sp)	(long) sfmt
//    (sp)	(long) signal number -- was a parameter for user routine

	.globl	_sig_return
	.globl	_pc_valid_return

_user_sig_return:
_sig_return:	addq.w	#8,sp		// pop signal number and sfmt
	move.w	#0x011a,-(sp)		// Psigreturn() system call
	trap	#1
_user_pc_valid_return:
_pc_valid_return:			// tells kernel its us
// we had better not come back
// if we did, something terrible
// happened, and we might as well terminate
	move.w	#-998,-(sp)
	move.w	#0x4c,-(sp)		// Pterm()
	trap	#1

	.globl	_slb_init_and_exit

_user_slb_init_and_exit:
_slb_init_and_exit:

	move.l	4(sp),a0		// basepage pointer
	move.l	B_ENV(a0),d0		// bp->p_env
	beq.s	noenv

	clr.l	B_ENV(a0)		// clear the pointer

	move.l	d0,-(sp)		// free the env variables
	move.w	#0x0049,-(sp)
	trap	#1
	addq.l	#6,sp

noenv:	pea	0x01190001.l		// Pdomain(1)
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
	move.l	#-1,B_CMDLIN(a0)	// put -1 on the bp->p_cmdlin
	bra.s	halt

exec:	move.l	SH_INIT(a0),a0
	jsr	(a0)

	move.l	38(sp),a0
	move.l	d0,B_CMDLIN(a0)		// return value

halt:	move.w	#0x0011,-(sp)
	clr.w	-(sp)
	move.w	#0x0111,-(sp)
	trap	#1
	addq.l	#6,sp

	move.l	(sp)+,a0		// header address
	move.l	SH_EXIT(a0),a0
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

_user_slb_open:
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

_user_slb_close:
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

_user_slb_close_and_pterm:
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

	.globl	_slb_exec
	.globl	_tosbp

_user_slb_exec:
_slb_exec:
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
	move.l	_tosbp,a0
	move.l	(a0),4(sp)
	move.l	d0,a0
	jmp	(a0)

einvfn:	sub.l	a0,a0		// hide the shared lib struct address
	moveq	#-32,d0
	rts

user_end:

// EOF
