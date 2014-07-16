| 
| Low level and interrupt routines for the USB mouse driver
|

#ifdef TOSONLY
	.globl _old_ikbd_int
	.globl _interrupt_ikbd
	.globl _mouse_int
	.globl _set_old_int_lvl

	.text

oldSR:  ds.w    1

	dc.l	0x58425241		| XBRA
	dc.l	0x534d5343		| _USB
_old_ikbd_int:
	ds.l	1
_interrupt_ikbd:
	movem.l	a0-a7/d0-d7,-(sp)
	bsr	_mouse_int
	movem.l	(sp)+,a0-a7/d0-d7
	move.l	_old_ikbd_int(PC),-(sp)
	rts
#endif

	.globl _send_packet

_send_packet:
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)
	move.l	64(sp),a2
	move.l	68(sp),a0
	move.l	72(sp),a1
	jsr	(a2)
	movem.l	(sp),d0-a6
	lea	60(sp),sp
	rts
