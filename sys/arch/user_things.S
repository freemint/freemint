/*
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

// The kernel_things is for the kernel itself and its threads.
// It is directly addressed, not copied anywhere.

	.data

	.globl	_kernel_things

_kernel_things:
	.long	0,0			// two useless fields, which must be here, see below

// now the jumptable, with +absolute+ addresses

	.long	terminateme
	.long	sig_return
	.long	pc_valid_return
	.long	0,0,0,0,0		// SLB stuff is currently useless for the kernel/threads
	.long	user_xhdi
# ifdef JAR_PRIVATE
	.long	0
# endif

	.text

// The user_things is for user processes. It is copied to the
// process address space and addressed there.

	.globl	_user_things

_user_things:
	.long	user_end-_user_things	// total number of bytes (to copy)
usr_bp:	.long	0			// user process' basepage pointer (filled in by Slbopen())

// now the jumptable, with relative addresses

	.long	terminateme-_user_things
	.long	sig_return-_user_things
	.long	pc_valid_return-_user_things
	.long	slb_init_and_exit-_user_things
	.long	slb_open-_user_things
	.long	slb_close-_user_things
	.long	slb_close_and_pterm-_user_things
	.long	slb_exec-_user_things
	.long	user_xhdi-_user_things
# ifdef JAR_PRIVATE
	.long	0
# endif

terminateme:
	move.w	4(sp),-(sp)
	move.w	#0x004c,-(sp)
	trap	#1

// sig_return: user signal handlers return to us. At that point, the
// stack looks like this:
//  -4(sp)	(long) sfmt
//    (sp)	(long) signal number -- was a parameter for user routine

sig_return:
	addq.l	#8,sp			// pop signal number and sfmt
	move.w	#0x011a,-(sp)		// Psigreturn() system call
	trap	#1
pc_valid_return:			// tells kernel its us
// we had better not come back
// if we did, something terrible
// happened, and we might as well terminate
	move.w	#-998,-(sp)
	bsr.s	terminateme		// never returns

// SLB stuff

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

slb_open:
	move.l	(sp),a0			// slbhead
	move.l	4(sp),-(sp)		// base
	move.l	SH_OPEN(a0),a1
	jsr	(a1)
	addq.l	#4,sp
	move.l	8(sp),a0		// slb
	tst.l	d0
	bmi.s	failed
	lea	SL_NAME(a0),a0
	clr.l	-(sp)
	clr.l	-(sp)
	clr.l	-(sp)
	clr.l	-(sp)
	move.l	a0,-(sp)
	move.w	#0x0016,-(sp)
	trap	#1
	lea	22+12(sp),sp
	rts

failed:	move.l	d0,-(sp)
	move.l	a0,-(sp)
	move.w	#0x0017,-(sp)
	trap	#1
	addq.l	#6,sp
	move.l	(sp)+,d0
	lea	12(sp),sp
	rts

// slb_close_and_pterm
//
// Like slb_close(), but instead of calling Slbclose() again, Pterm() is called.
// This function is used when a process exited without calling Slbclose().

slb_close_and_pterm:
	move.l	d0,-(sp)
	move.l	8(sp),a0
	move.l	SL_HEAD(a0),a0
	move.l	SH_CLOSE(a0),a0
	move.l	4(sp),-(sp)
	jsr	(a0)
	addq.l	#4,sp
	move.l	(sp)+,d0
	addq.l	#8,sp
	move.w	d0,-(sp)
	bsr.s	terminateme		// never returns

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

slb_close:
	move.l	4(sp),a0
	move.l	SL_HEAD(a0),a0
	move.l	SH_CLOSE(a0),a0
	move.l	(sp),-(sp)
	jsr	(a0)
	addq.l	#4,sp
	move.l	4(sp),-(sp)
	move.w	#0x0017,-(sp)
	trap	#1
	lea	14(sp),sp
	rts

slb_init_and_exit:

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
	addq.l	#4,sp

	moveq	#0x0004,d7
loop:	move.w	d7,-(sp)		// Fclose(x)
	move.w	#0x003e,-(sp)
	trap	#1
	addq.l	#4,sp
#ifdef __mcoldfire__
	subq.l	#1,d7
	bpl.s	loop
#else
	dbra.w	d7,loop
#endif

	clr.l	-(sp)			// Psetpgrp(0,0)
	move.w	#0x010e,-(sp)
	trap	#1
	addq.l	#6,sp

	move.l	4(sp),a0		// basepage pointer
	lea	256(a0),a0		// begin of the TEXT segment

	// Test for original binutils
#ifdef __mcoldfire__
	move.l	#0x283a001a,d0
	cmp.l	(a0),d0
#else
	cmp.l	#0x283a001a,(a0)
#endif
	bne.s	binu2
#ifdef __mcoldfire__
	move.l	#0x4efb48fa,d0
	cmp.l	4(a0),d0
#else
	cmp.l	#0x4efb48fa,4(a0)
#endif
	bne.s	binu2
	bra.s	newfmt

	// Test for binutils >= 2.18-mint-20080209
binu2:
#ifdef __mcoldfire__
	move.l	#0x203a001a,d0
	cmp.l	(a0),d0
#else
	cmp.l	#0x203a001a,(a0)
#endif
	bne.s	oldfmt
#ifdef __mcoldfire__
	move.l	#0x4efb08fa,d0
	cmp.l	4(a0),d0
#else
	cmp.l	#0x4efb08fa,4(a0)
#endif
	bne.s	oldfmt

newfmt:
	lea	228.w(a0),a0		// the header is a bit bigger (new format)

oldfmt:	move.l	a0,-(sp)		// 4 bytes go to the stack

#ifdef __mcoldfire__
	move.l	#0x70004afc,d0
	cmp.l	(a0),d0
#else
	cmp.l	#0x70004afc,(a0)	// a0 = SLB header address
#endif
	beq.s	exec

	move.l	8(sp),a0
#ifdef __mcoldfire__
	moveq	#-1,d0
	move.l	d0,B_CMDLIN(a0)		// put -1 on the bp->p_cmdlin
#else
	move.l	#-1,B_CMDLIN(a0)	// put -1 on the bp->p_cmdlin
#endif

	bsr.s	slb_stop		// Slbopen() will now read the status

	bra.s	exit

exec:	move.l	SH_INIT(a0),a0
	jsr	(a0)			// call the slb_init routine

	move.l	8(sp),a0
	move.l	d0,B_CMDLIN(a0)		// return value

	bsr.s	slb_stop		// Slbopen() will now read the status

	move.l	(sp)+,a0		// header address
	move.l	SH_EXIT(a0),a0
	jsr	(a0)			// call the slb_exit routine

exit:	clr.w	-(sp)
	trap	#1

slb_stop:
	move.w	#0x0011,-(sp)		// SIGSTOP
	clr.w	-(sp)
	move.w	#0x0111,-(sp)
	trap	#1
	addq.l	#6,sp
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

slb_exec:
	move.l	8(sp),d0
	bmi.s	einvfn
	move.l	4(sp),a0
	move.l	SL_HEAD(a0),a0
	cmp.l	SH_NO_FUNCS(a0),d0
	bcc.s	einvfn
#ifndef M68000
	move.l	SH_FUNCTIONS(a0,d0.l*4),d0
#else
	lsl.l	#2,d0
	move.l	SH_FUNCTIONS(a0,d0.l),d0
#endif
	beq.s	einvfn
	move.l	usr_bp.w(pc),4(sp)	// replace the SLB handle with basepage pointer
	move.l	d0,a0
	jmp	(a0)

einvfn:	moveq	#-32,d0
	rts

// The user XHDI call. The XHDI entry in the Cookie Jar points to the begin
// of this code, of course only when each user has own Jar.

	dc.l	0x27011992		// XHDIMAGIC

user_xhdi:
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.w	32(sp),-(sp)
	move.w	#0x0001,-(sp)		// we want XHDI EMU
	move.w	#0x015f,-(sp)		// system EMU
	trap	#1
	lea	34(sp),sp
	rts

user_end:

// EOF
