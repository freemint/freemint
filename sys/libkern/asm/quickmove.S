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

	.globl	__mint_quickcpy
	.globl	__mint_bcopy
	.globl	_memcpy

__mint_bcopy:
#ifdef __mcoldfire__
	move.l	4(sp),a1
	move.l	8(sp),a0
#else
	movem.l	4(sp),a0/a1
	exg	a0,a1
#endif
	bra.s	_quickmovb1
_memcpy:
	movem.l	4(sp),a0/a1		| get dst & src
_quickmovb1:
	move.l 12(sp),d0
	beq.s	done
#ifdef __mcoldfire__
	move.w	6(sp),d1
	andi.l	#1,d1
#else
	moveq	#1,d1
	and.w	6(sp),d1
#endif
	bne.s	sodd
#ifdef __mcoldfire__
	move.w	10(sp),d1
	andi.l	#1,d1
#else
	moveq	#1,d1
	and.w	10(sp),d1
#endif
	beq.s	quickmov1

#if !defined(__mc68020__) && !defined(__mc68030__) && !defined(__mc68040__) && !defined(__mc68060__) && !defined(__mcoldfire__)
bytecopy:
	move.b	(a1)+,(a0)+
	subq.l	#1,d0
	bne.s	bytecopy
	rts				| return
#else
	bra.s	quickmov1
#endif

sodd:
#ifdef __mcoldfire__
	move.w	10(sp),d1
	andi.l	#1,d1
#else
	and.w	10(sp),d1
#endif
#if !defined(__mc68020__) && !defined(__mc68030__) && !defined(__mc68040__) && !defined(__mc68060__) && !defined(__mcoldfire__)
	beq.s	bytecopy
#else
	beq.s	quickmov1
#endif
	move.b	(a1)+,(a0)+
	subq.l	#1,d0
	bne.s	quickmov1
done:	rts

__mint_quickcpy:
	movem.l	4(sp),a0/a1		| get dst & src
	move.l	12(sp),d0		| get nbytes
	beq.s	done
quickmov1:
#ifdef __mcoldfire__
	move.l	#0x1ff,d1
	and.l	d0,d1			| d1 = nbytes % 512
#else
	move.w	#0x1ff,d1
	and.w	d0,d1			| d1 = nbytes % 512
#endif
	lsr.l	#8,d0			| 
	lsr.l	#1,d0			| d0 = nbytes / 512
#ifdef __mcoldfire__
	ble	Leftover		| if <= 0, skip
#else
	ble.s	Leftover		| if <= 0, skip
#endif

blkmv:
#ifdef __mcoldfire__
	lea     -48(sp),sp
	movem.l	d1-d7/a2-a6,(sp)	| save regs
#else
	movem.l	d1-d7/a2-a6,-(sp)	| save regs
#endif
L1:
#ifdef __mcoldfire__
	movem.l	(a1),d1-d7/a2-a6	| read 12*4 = 48 bytes
	movem.l	d1-d7/a2-a6,(a0)	|
	movem.l	48(a1),d1-d7/a2-a6	| 2nd read
	movem.l	d1-d7/a2-a6,48(a0)	|
	movem.l	96(a1),d1-d7/a2-a6	| 3rd read
	movem.l	d1-d7/a2-a6,96(a0)	|
	movem.l	144(a1),d1-d7/a2-a6	| 4th read
	movem.l	d1-d7/a2-a6,144(a0)	|
	movem.l	192(a1),d1-d7/a2-a6	| 5th
	movem.l	d1-d7/a2-a6,192(a0)	|
	movem.l	240(a1),d1-d7/a2-a6	| 6th
	movem.l	d1-d7/a2-a6,240(a0)	|
	movem.l	288(a1),d1-d7/a2-a6	| 7th
	movem.l	d1-d7/a2-a6,288(a0)	|
	movem.l	336(a1),d1-d7/a2-a6	| 8th
	movem.l	d1-d7/a2-a6,336(a0)	|
	movem.l	384(a1),d1-d7/a2-a6	| 9th
	movem.l	d1-d7/a2-a6,384(a0)	|
	movem.l	432(a1),d1-d7/a2-a6	| 10th
	movem.l	d1-d7/a2-a6,432(a0)	| At this point, 480 bytes done
	movem.l	480(a1),d1-d7/a2	| Only do 32 more bytes
	movem.l	d1-d7/a2,480(a0)	| for a total of 512
	lea	512(a1),a1
#else
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
#endif

	lea	512(a0),a0
	subq.l	#1,d0
#ifdef __mcoldfire__
	bgt	L1
#else
	bgt.s	L1
#endif

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
	bmi.s	L4done
	lsr.l	#1,d1
#else
	and.w	d1,d0
	lsr.w	#2,d1
	subq.w	#1,d1			| prepare for dbra loop
	bmi.s	L4done
	lsr.w	#1,d1
#endif
	bcc.s	L23
L2l:
	move.l	(a1)+,(a0)+
L23:
	move.l	(a1)+,(a0)+
#ifdef __mcoldfire__
	subq.l	#1,d1
	bpl.s	L2l
#else
	dbra	d1,L2l
#endif
L4done:
#ifdef __mcoldfire__
	subq.l	#1,d0			| prepare for dbra loop
#else
	subq.w	#1,d0			| prepare for dbra loop
#endif
	bmi.s	Ldone
L2:
	move.b	(a1)+,(a0)+
#ifdef __mcoldfire__
	subq.l	#1,d0
	bpl.s	L2
#else
	dbra	d0,L2
#endif
Ldone:
	rts				| return
