| 
| FreeMiNT memset function
| 
| This file belongs to FreeMiNT,
| it is not a part of the original MiNT distribution
| 
| 8.II.2000, draco@atari.org
| 
| 
| void *memset(void *s, int val, long len)
| 

#include "mint/asmdefs.h"

	.text

	.globl	SYM(memset)

SYM(memset):
	move.l	d2,-(sp)	| save old value
	
	move.l	4+4(sp),a1
	move.l	10+4(sp),d2
	beq.s	L_exit
	move.l	a1,a0
	move.w	8+4(sp),d1
#if defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__) || defined(__mcoldfire__)
	move.b	d1,d0		| expand d1 to longword
#ifdef __mcoldfire__
	lsl.l	#0x8,d1
#else
	lsl.w	#0x8,d1
#endif
	move.b	d0,d1
	move.w	d1,d0
	swap	d1
	move.w	d0,d1

	move.l	d2,d0
	lsr.l	#0x2,d0		| number of longwords
	beq.s	L_nolong
L_long:
	move.l	d1,(a0)+	| fill...
	subq.l	#0x1,d0
	bne.s	L_long
	move.l	d2,d0
#ifdef __mcoldfire__
	and.l	#0xfffffffc,d0
#else
	and.w	#0xfffc,d0
#endif
	sub.l	d0,d2

L_nolong:
	move.l	d2,d0		| fill remaining word...
	lsr.l	#0x1,d0
	beq.s	L_noword
	move.w	d1,(a0)+
	subq.l	#0x2,d2

L_noword:
	tst.l	d2		| ... and byte
	beq.s	L_exit
	move.b	d1,(a0)+
#else
L_byte:
	move.b	d1,(a0)+
	subq.l	#0x1,d2
	bne.s	L_byte
#endif
L_exit:
	move.l	(sp)+,d2	| restore old value
	move.l	a1,d0		| return value
	rts
