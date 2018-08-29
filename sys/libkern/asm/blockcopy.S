| Copyright 1992 Eric R. Smith
| Copyright 1993 Atari Corporation
| All rights reserved.
|
| _mint_blockcpy(char *dst, char *src, long nblocks)|
| 512-byte blocks with 16-byte alignment

	.text

	.globl	__mint_blockcpy

__mint_blockcpy:
	movem.l	4(sp),a0/a1		| dst & src
	move.l	12(sp),d0		| blocks
#if defined(__mc68040__) || defined(__mc68060__)
	move.l	_mcpu,d1
	cmp.w	#40,d1
	bcc.s	L_040			| for 68000-68030 do quickmove
	bra	blkmv
L_040:	dc.l	0xf6218000		| move16 (a1)+,(a0)+
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	dc.l	0xf6218000
	subq.l	#1,d0
	bne	L_040
	rts
#endif

blkmv:
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)	| save regs
#else
	movem.l	d2-d7/a2-a6,-(sp)	| save regs
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
	bne	L1
#else
	bne.s	L1
#endif

#ifdef __mcoldfire__
	movem.l	(sp),d2-d7/a2-a6	| pop registers
	lea	44(sp),sp
#else
	movem.l	(sp)+,d2-d7/a2-a6	| pop registers
#endif

	rts				| return
