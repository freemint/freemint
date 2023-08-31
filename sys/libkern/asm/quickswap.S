|
| This file is a modified copy of quickswa.spp, and was created as part
| of the FreeMiNT project. See the file Changes.MH for details and dates.
|
| Copyright 1992 Eric R. Smith
| Copyright 1993 Atari Corporation
| All rights reserved.
|
| quickswap(char *dst, char *src, long nbytes):
| quickly exchanges "nbytes" bytes from src to dst. Assumes that both
| src and dst are word aligned.
|

#include "mint/asmdefs.h"

	.text

	.globl	SYM(_mint_quickswap)

SYM(_mint_quickswap):
	move.l	4(sp),a0		| get dst
	move.l	8(sp),a1		| get src
	move.l	12(sp),d0		| get nbytes
#ifdef __mcoldfire__
	move.l	#0xff,d1
	and.l	d0,d1			| d1 = nbytes % 256
#else
	move.w	#0xff,d1
	and.w	d0,d1			| d1 = nbytes % 256
#endif
	lsr.l	#8,d0			| d0 = nbytes / 256
	ble	Leftover		| if <= 0, skip
 
#ifdef __mcoldfire__
	lea	-48(sp),sp
	movem.l	d1-d7/a2-a6,(sp)	| save regs
#else
	movem.l	d1-d7/a2-a6,-(sp)	| save regs
#endif
L1:
	movem.l	(a1),d1-d6		| swap 6*4 = 24 bytes
	movem.l (a0),d7/a2-a6		|
	movem.l	d1-d6,(a0)		|
	movem.l d7/a2-a6,(a1)		|
	movem.l	24(a1),d1-d6		| 2nd swap
	movem.l 24(a0),d7/a2-a6		|
	movem.l	d1-d6,24(a0)		|
	movem.l d7/a2-a6,24(a1)		|
	movem.l	48(a1),d1-d6		| 3nd swap
	movem.l 48(a0),d7/a2-a6		|
	movem.l	d1-d6,48(a0)		|
	movem.l d7/a2-a6,48(a1)		|
	movem.l	72(a1),d1-d6		| 4th swap
	movem.l 72(a0),d7/a2-a6		|
	movem.l	d1-d6,72(a0)		|
	movem.l d7/a2-a6,72(a1)		|
	movem.l	96(a1),d1-d6		| 5th swap
	movem.l 96(a0),d7/a2-a6		|
	movem.l	d1-d6,96(a0)		|
	movem.l d7/a2-a6,96(a1)		|
	movem.l	120(a1),d1-d6		| 6th swap
	movem.l 120(a0),d7/a2-a6	|
	movem.l	d1-d6,120(a0)		|
	movem.l d7/a2-a6,120(a1)	|
	movem.l	144(a1),d1-d6		| 7th swap
	movem.l 144(a0),d7/a2-a6	|
	movem.l	d1-d6,144(a0)		|
	movem.l d7/a2-a6,144(a1)	|
	movem.l	168(a1),d1-d6		| 8th swap
	movem.l 168(a0),d7/a2-a6	|
	movem.l	d1-d6,168(a0)		|
	movem.l d7/a2-a6,168(a1)	|
	movem.l	192(a1),d1-d6		| 9th swap
	movem.l 192(a0),d7/a2-a6	|
	movem.l	d1-d6,192(a0)		|
	movem.l d7/a2-a6,192(a1)	|
	movem.l	216(a1),d1-d6		| 10th swap
	movem.l 216(a0),d7/a2-a6	|
	movem.l	d1-d6,216(a0)		|
	movem.l d7/a2-a6,216(a1)	|
	movem.l	240(a1),d1-d4		| At this point, 240 bytes done
	movem.l 240(a0),a2-a5		| Only do 16 more bytes
	movem.l	d1-d4,240(a0)		| for a total of 256
	movem.l a2-a5,240(a1)		|

	lea	256(a0),a0
	lea	256(a1),a1
	subq.l	#1,d0
	bgt	L1

#ifdef __mcoldfire__
	movem.l	(sp),d1-d7/a2-a6	| pop registers
	lea	48(sp),sp
#else
	movem.l	(sp)+,d1-d7/a2-a6	| pop registers
#endif

Leftover:				| do the remaining bytes
	moveq.l	#3,d0
#ifdef __mcoldfire__
	and.l	d1,d0
	lsr.l	#2,d1
	subq.l	#1,d1			| prepare for dbra loop
#else
	and.w	d1,d0
	lsr.w	#2,d1
	subq.w	#1,d1			| prepare for dbra loop
#endif
	bmi.s	L4done
#ifdef __mcoldfire__
	move.l	d0,-(sp)		| save register
	lsr.l	#1,d1
#else
	move.w	d0,-(sp)		| save register
	lsr.w	#1,d1
#endif
	bcc.s	L23
L2l:
	move.l	(a1),d0
	move.l  (a0),(a1)+
	move.l	d0,(a0)+
L23:
	move.l	(a1),d0
	move.l  (a0),(a1)+
	move.l	d0,(a0)+
#ifdef __mcoldfire__
	subq.l	#1,d1
	bpl.s	L2l
	move.l	(sp)+,d0		| restore register
#else
	dbra	d1,L2l
	move.w	(sp)+,d0		| restore register
#endif
L4done:
#ifdef __mcoldfire__
	subq.l	#1,d0			| prepare for dbra loop
#else
	subq.w	#1,d0			| prepare for dbra loop
#endif
	bmi.s	Ldone
L2:
	move.b	(a1),d1
	move.b  (a0),(a1)+
	move.b	d1,(a0)+
#ifdef __mcoldfire__
	subq.l	#1,d0
	bpl.s	L2
#else
	dbra	d0,L2
#endif
Ldone:
	rts				| return
