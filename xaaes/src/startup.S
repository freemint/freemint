/*
 * $Id$
 * 
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 * Started: 2000-02-08
 * 
 * 
 * Startup module
 *
 */

	.globl	__base
	.globl	__start
	.globl	__crtinit

	.bss
__base:
	ds.l	1

	.text

__start:
	move.l	0x4(sp),a0		// basepage address
	move.l	a0,__base

	movel	a0@(4),d0		// get _base->p_hitpa
	bclr	#0,d0			// round off
	movel	d0,sp			// set stack (temporarily)
	jmp	__crtinit		// in crtinit.c
