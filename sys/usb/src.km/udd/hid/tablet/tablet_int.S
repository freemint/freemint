|
| Low level and interrupt routines for the USB tablet driver
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

#ifdef TOSONLY
	.globl SYM(old_ikbd_int)
	.globl SYM(interrupt_ikbd)
	.extern SYM(tablet_int)

	.text

	dc.l	XBRA_MAGIC
	dc.l	0x554d4f55		| UMOU
SYM(old_ikbd_int):
	ds.l	1
SYM(interrupt_ikbd):
	tas	lock
	bne	skip
	movem.l	a0-a7/d0-d7,registersave
	lea	ourSP,a7
	bsr	SYM(tablet_int)
	movem.l	registersave,a0-a7/d0-d7
	clr.w	lock
skip:	move.l	SYM(old_ikbd_int)(pc),-(sp)
	rts
#endif

	.globl SYM(send_packet)
	.globl SYM(send_data)
	.globl SYM(fake_hwint)

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

SYM(fake_hwint):
	tst.w	0x59e.w
	beq	shortframe
	move.w	#0x118,-(sp)
shortframe:
	pea	retaddr(pc)
	move	sr,d0
	move.w	d0,-(sp)
	move.l	0x118,a0
	jmp	(a0)
retaddr:
	rts

	.bss
lock:	ds.w 1
	ds.l 1024
ourSP:
registersave:
	ds.l 8+8
	.end
