/*
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

#include "mint/asmdefs.h"

	.globl	SYM(main)
	.extern	SYM(init)
	.extern	SYM(_base)

	.text
	FUNC(main)
SYM(main):
	CFI_STARTPROC()
	move.l	0x04(sp),a0		// basepage address
	move.l	a0,SYM(_base)

	move.l	0x000c(a0),d0		// size of the TEXT segment
	add.l	0x0014(a0),d0		// size of the DATA segment
	add.l	0x001c(a0),d0		// size of the BSS segment
	add.l	#0x00003101,d0		// stack size + basepage size + 1
	bclr	#0,d0			// round
	lea	(a0,d0.l),sp		// new stack pointer

	move.l	d0,-(sp)		// new size
	move.l	a0,-(sp)		// start address
	clr.w	-(sp)
	move.w	#0x004a,-(sp)
	trap	#1
	lea	12(sp),sp

	jmp	SYM(init)
	CFI_ENDPROC()
	END(main)
