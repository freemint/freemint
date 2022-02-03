| Copyright 1992 Eric R. Smith
| All rights reserved
|
| quickzero(char *place, ulong size): zero an area of memory. "size" is
| the number of 256 byte blocks in the area, and must be > 0.
| registers changed: d0 and a0
|
	.text

	.globl	__mint_quickzero

__mint_quickzero:
	move.l	4(sp),a0		| place to zero at
	move.l	8(sp),d0		| number of blocks
#ifdef __mcoldfire__
	lea	-32(sp),sp
	movem.l	d1-d7/a1,(sp)		| save registers
#else
	movem.l	d1-d7/a1,-(sp)		| save registers
#endif
	moveq.l	#0,d1			| zero them out
	moveq.l	#0,d2
	moveq.l	#0,d3
	moveq.l	#0,d4
	moveq.l	#0,d5
	moveq.l	#0,d6
	moveq.l	#0,d7
	move.l	d1,a1
loop:
	movem.l	d1-d7/a1,(a0)		| zero the memory, 32 bytes at a time
	movem.l	d1-d7/a1,32(a0)
	movem.l	d1-d7/a1,64(a0)
	movem.l	d1-d7/a1,96(a0)
	movem.l	d1-d7/a1,128(a0)
	movem.l	d1-d7/a1,160(a0)
	movem.l	d1-d7/a1,192(a0)
	movem.l	d1-d7/a1,224(a0)
	lea	256(a0),a0
	subq.l	#1,d0
	bne.s	loop
#ifdef __mcoldfire__
	movem.l	(sp),d1-d7/a1		| restore regs
	lea	32(sp),sp
#else
	movem.l	(sp)+,d1-d7/a1		| restore regs
#endif
	rts				| and leave
