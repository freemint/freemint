| Copyright 1992 Eric R. Smith
| Copyright 1993 Atari Corporation
| All rights reserved.
|
| quickmove(char *dst, char *src, long nbytes):
| quickly copy "nbytes" bytes from src to dst. Assumes that both
| src and dst are word aligned.
|
| quickmovb(char *dst, char *src, long nbytes):
| like memcpy, does unaligned too...  does not check for overlap (memmove).

	.text

	.globl	_bcopy
	.globl	_memcpy
	.globl	__mint_quickcpy

_bcopy:
	movem.l	4(sp),a0/a1
	exg	a0,a1
	bra.s	_quickmovb1
_memcpy:
	movem.l	4(sp),a0/a1		| get dst & src
_quickmovb1:
	move.w	a0,d0
	move.w	a1,d1
	eor.w	d1,d0			| bit 0 == unaligned
	lsr.w	#1,d0			| ...now in x flag
	move.l	12(sp),d0		| get nbytes
	beq	Ldone
	roxr.w	#1,d1			| bit 0 == both odd, msb == unaligned
#ifndef ONLY030
	bmi	bytecopy		| unaligned, do the slow thing...
#else
	bmi	quickmov1		| alignment doesnt matter on 030
#endif
	bcc.s	quickmov1		| both even, ok
	subq.l	#1,d0			| both odd, can be fixed
	move.b	(a1)+,(a0)+
	bra.s	quickmov1

__mint_quickcpy:
	movem.l	4(sp),a0/a1		| get dst & src
	move.l	12(sp),d0		| get nbytes
quickmov1:
	move.w	#0x1ff,d1
	and.w	d0,d1			| d1 = nbytes % 512
	lsr.l	#8,d0			| 
	lsr.l	#1,d0			| d0 = nbytes / 512
	ble.s	Leftover		| if <= 0, skip

blkmv:
	movem.l	d1-d7/a2-a6,-(sp)	| save regs
L1:
	movem.l	(a1)+,d1-d7/a2-a6	| read 12*4 = 48 bytes
	movem.l	d1-d7/a2-a6,(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 2nd read
	movem.l	d1-d7/a2-a6,48(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 3rd read
	movem.l	d1-d7/a2-a6,96(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 4th read
	movem.l	d1-d7/a2-a6,144(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 5th
	movem.l	d1-d7/a2-a6,192(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 6th
	movem.l	d1-d7/a2-a6,240(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 7th
	movem.l	d1-d7/a2-a6,288(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 8th
	movem.l	d1-d7/a2-a6,336(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 9th
	movem.l	d1-d7/a2-a6,384(a0)	|
	movem.l	(a1)+,d1-d7/a2-a6	| 10th
	movem.l	d1-d7/a2-a6,432(a0)	| At this point, 480 bytes done
	movem.l	(a1)+,d1-d7/a2		| Only do 32 more bytes
	movem.l	d1-d7/a2,480(a0)	| for a total of 512

	lea	512(a0),a0
	subq.l	#1,d0
	bgt.s	L1

	movem.l	(sp)+,d1-d7/a2-a6	| pop registers

Leftover:				| do the remaining bytes
	moveq.l	#3,d0
	and.w	d1,d0
	lsr.w	#2,d1
	subq.w	#1,d1			| prepare for dbra loop
	bmi.s	L4done
	lsr.w	#1,d1
	bcc.s	L23
L2l:
	move.l	(a1)+,(a0)+
L23:
	move.l	(a1)+,(a0)+
	dbra	d1,L2l
L4done:
	subq.w	#1,d0			| prepare for dbra loop
	bmi.s	Ldone
L2:
	move.b	(a1)+,(a0)+
	dbra	d0,L2
Ldone:
	rts				| return

#ifndef ONLY030
bytecopy:
	subq.l	#1,d0			| prepare for dbra loop
	move.l	d0,d1
	swap	d1
L3:
	move.b	(a1)+,(a0)+
	dbra	d0,L3
	dbra	d1,L3
	rts				| return
#endif
