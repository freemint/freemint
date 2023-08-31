|
| Low level and interrupt routines for the USB mass storage
|

#include "mint/asmdefs.h"

#ifdef TOSONLY
	.globl SYM(old_etv_timer_int)
	.globl SYM(interrupt_storage)
	.extern SYM(storage_int)

	.text

	dc.l	0x58425241		| XBRA
	dc.l	0x55535452		| USTR
SYM(old_etv_timer_int):
	ds.l	1
SYM(interrupt_storage):
	subq.w	#1,count
	bne	skip
	tas	lock
	bne	skip
	movem.l	a0-a7/d0-d7,-(sp)
	move.l	a7,oldSP
	lea	ourSP,a7
	bsr	SYM(storage_int)
	move.l	oldSP,a7
	movem.l	(sp)+,a0-a7/d0-d7
	move.w	#100,count
	clr.w	lock
skip:	move.l	SYM(old_etv_timer_int)(pc),-(sp)
	rts
#endif

	.data
count:	dc.w 100
	.bss
lock:	ds.w 1
oldSP:	ds.l 1
	ds.l 1024
ourSP:
	.end
