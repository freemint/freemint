| 
| Low level and interrupt routines for the USB mouse driver
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

#ifdef TOSONLY
	.globl SYM(old_ikbd_int)
	.globl SYM(interrupt_ikbd)
	.extern SYM(mouse_int)

	.text

	dc.l	XBRA_MAGIC
	dc.l	0x554d4f55		| UMOU
SYM(old_ikbd_int):
	ds.l	1
SYM(interrupt_ikbd):
	tas	lock
	bne	skip
#ifdef __mcoldfire__
	lea	-64(sp),sp
	movem.l	a0-a7/d0-d7,(sp)
	move.l	a7,oldSP
	lea	ourSP,a7
	bsr	SYM(mouse_int)
	move.l	oldSP,a7
	movem.l	(sp),a0-a7/d0-d7
	lea	64(sp),sp
#else
	movem.l	a0-a7/d0-d7,registersave
	lea	ourSP,a7
	bsr	SYM(mouse_int)
	movem.l	registersave,a0-a7/d0-d7
#endif
	clr.w	lock
#ifdef __mcoldfire__
	| clr.w is a cached write; ColdFire tas is non-cacheable and reads
	| directly from RAM. Push the cache line containing `lock` so
	| it's visible to the next tas.
	move.l	d0,-(sp)
	move.l	a0,-(sp)
	lea	lock,a0
	move.l	a0,d0
	andi.l	#-16,d0
	move.l	d0,a0
	nop
	cpushl	%dc,(a0)		| way 0
	addq.l	#1,a0
	cpushl	%dc,(a0)		| way 1
	addq.l	#1,a0
	cpushl	%dc,(a0)		| way 2
	addq.l	#1,a0
	cpushl	%dc,(a0)		| way 3
	move.l	(sp)+,a0
	move.l	(sp)+,d0
#endif
skip:	move.l	SYM(old_ikbd_int)(pc),-(sp)
	rts
#endif

	.globl SYM(send_packet)
	.globl SYM(send_data)

SYM(send_packet):
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)
	move.l	64(sp),a2
	move.l	68(sp),a0
	move.l	72(sp),a1
	jsr	(a2)
	movem.l	(sp),d0-a6
	lea	60(sp),sp
	rts

SYM(send_data):
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)
	move.l	64(sp),a2
	move.l	68(sp),a0
	move.l	72(sp),d0
	jsr	(a2)
	movem.l	(sp),d0-a6
	lea	60(sp),sp
rts

	.bss
lock:	ds.w 1
oldSP:	ds.l 1
	.balign 4			| ensure ourSP is 4-byte aligned
	ds.l 1024
ourSP:
registersave:
	ds.l 8+8
	.end
