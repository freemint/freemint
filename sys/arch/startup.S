/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 * Started: 2000-02-08
 *
 *
 * FreeMiNT startup module
 *
 */

	.globl	_main
	.globl	_init
	.globl	__base

#define STACKSIZE 0x00002000

	.bss
__base:
	ds.l	1
__stk:
	ds.b	STACKSIZE

	.text

_main:
	move.l	0x04(sp),a0		// basepage address
	move.l	a0,__base

	lea	__stk+STACKSIZE,sp

	move.l	0x18(a0),d0		// base of the BSS segment
	add.l	0x1c(a0),d0		// size of the BSS segment
	sub.l	a0,d0

	move.l	d0,-(sp)		// new size
	move.l	a0,-(sp)		// start address
	pea	(0x004a0000).l		// Mshrink()
	trap	#1
	lea	0x0c(sp),sp

	jmp	_init
