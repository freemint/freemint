|
| Low level and interrupt routines for the USB keyboard driver
|

#include "mint/asmdefs.h"

#ifdef TOSONLY
	.globl SYM(old_ikbd_int)
	.globl SYM(interrupt_ikbd)
	.extern SYM(kbd_int)
	.globl SYM(set_old_int_lvl)

	.text

	dc.l	0x58425241		| XBRA
	dc.l	0x554b4244		| UKBD
SYM(old_ikbd_int):
	ds.l	1
SYM(interrupt_ikbd):
	tas	lock
	bne	skip
	movem.l	a0-a7/d0-d7,registersave
	lea	ourSP,a7
	bsr	SYM(kbd_int)
	movem.l	registersave,a0-a7/d0-d7
	clr.w	lock
skip:	move.l	SYM(old_ikbd_int)(pc),-(sp)
	rts
#endif

	.globl SYM(send_data)
	.globl SYM(fake_hwint)

SYM(send_data):
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)
	move.l	64(sp),a2
	move.l	68(sp),a0
	move.l	72(sp),d0
	suba.l	a5,a5 		| needed for TOS 1.x
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
