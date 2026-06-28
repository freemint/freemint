|
| Low level and interrupt routines for the USB mass storage
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

#ifdef TOSONLY
	.globl SYM(old_etv_timer_int)
	.globl SYM(interrupt_storage)
    .globl SYM(lock)
	.extern SYM(storage_int)

	.text

	dc.l	XBRA_MAGIC
	dc.l	0x55535452		| USTR
SYM(old_etv_timer_int):
	ds.l	1
SYM(interrupt_storage):
#ifdef __mcoldfire__
	subq.l	#1,count
#else
	subq.w	#1,count
#endif
	bne	skip
	tas	SYM(lock)
	bne	skip
#ifdef __mcoldfire__
	lea	-64(sp),sp
	movem.l a0-a7/d0-d7,(sp)
#else
	movem.l	a0-a7/d0-d7,-(sp)
#endif
	move.l	a7,oldSP
	lea	ourSP,a7
	bsr	SYM(storage_int)
	move.l	oldSP,a7
#ifdef __mcoldfire__
	lea	count,a0
	move.w	#100,(a0)
	movem.l (sp),a0-a7/d0-d7
	lea	64(sp),sp
#else
	movem.l	(sp)+,a0-a7/d0-d7
	move.w	#100,count
#endif
	clr.w	SYM(lock)
skip:	move.l	SYM(old_etv_timer_int)(pc),-(sp)
	rts
#endif

	.data
SYM(lock):	ds.w 1
count:	dc.w 100
	.bss
oldSP:	ds.l 1
	ds.l 1024
ourSP:
	.end
