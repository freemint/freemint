/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 * Started: 1998-09-05
 *
 *
 * This replaces enter_kernel() and leave_kernel()
 * previously defined in main.c. Purpose: speed optimization.
 *
 * NOTICE: these routines are mainly responsible for the fact that
 * the kernel is not reentrant. Some day we will have to fix it.
 *
 */

#include "magic/magic.i"

	.globl	_enter_gemdos
	.globl	_enter_kernel
	.globl	_leave_kernel

	.extern	_curproc
	.globl	_in_kernel
	.globl	__base

// Formerly, the enter_kernel() was called with an argument, that was
// copied to the curproc->in_dos. We are compatible with this behaviour
// splitting the function to enter_gemdos() and enter_bios() called
// when necessary. Notice we aren't playing with trap vectors anymore,
// calls get redirected to ROM by the MiNT trap handlers according to the
// in_kernel state.
// Also, the in_kernel flag is now responsible for preventing any two
// (or more) routines from entering the kernel at the same time. As the
// flag is locked in one continuous read-modify-write cycle by
// trap handlers and the VBL routine, there's no need to manipulate the
// IPL before entering and leaving the kernel.
// Checking the flag introduces some additional CPU cycles into trap
// handlers, but this is balanced by removal of approx. 10 instructions
// (incl. 9 memory-to-memory long moves) from each enter/leave sequence.

	.data

_in_kernel:
	dc.w	0

	.text

_enter_gemdos:
#ifdef __mcoldfire__
	moveq	#0x07,d0
	bset	d0,_in_kernel
#else
	bset	#0x07,_in_kernel
#endif
	bne.s	exit
	move.l	_curproc,a0
	move.w	#0x0001,P_INDOS(a0)
	move.w	#1,P_INKERN(a0)
exit:	rts

_enter_kernel:
#ifdef __mcoldfire__
	moveq	#0x7,d0
	bset	d0,_in_kernel
#else
	bset	#0x7,_in_kernel
#endif
	bne.s	exit
	move.l	_curproc,a0
	move.w	#1,P_INKERN(a0)
	bra	exit

//
// leave_kernel: called before leaving the kernel, either back to
// user mode or when calling a signal handler or the GEMDOS
// terminate vector.
//
// Notice: the rest of assembler modules rely on the fact, that
// this function loads the _curproc address to the register a0,
// and additionally doesn't change the d0. See syscall.spp.
//
_leave_kernel:
	move.l	_curproc,a0
	clr.w	P_INDOS(a0)
	clr.w	P_INKERN(a0)
	clr.w	_in_kernel
	rts
