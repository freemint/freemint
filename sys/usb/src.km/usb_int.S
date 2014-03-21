| 
| Low level and interrupt routines for the USB HUB driver
|
	.globl _old_vbl_int
	.globl _interrupt_vbl
	.globl _usb_int
	.globl _set_old_int_lvl

	.text

oldSR:	ds.w	1

	dc.l	0x58425241		| XBRA
	dc.l	0x534d5343		| _USB
_old_vbl_int:
	ds.l	1
_interrupt_vbl:
	movem.l	a0-a7/d0-d7,-(sp)
	bsr	_usb_int
	movem.l	(sp)+,a0-a7/d0-d7
	move.l	_old_vbl_int(PC),-(sp)
	rts

| Sets interrupt level to what was in the SR
_set_old_int_lvl:
	move.w	(sp),oldSR
	andi.w	#0x0f00,oldSR		|just keep the int lvl
	move.l	d0,-(sp)
	move.w	sr,d0
	andi.w	#0xf0ff,d0
	or.w	oldSR,d0
	move.w	d0,sr
	move.l	(sp)+,d0
	rts

