|
| Low level and interrupt routines for the Svethlana driver
|
| 2010-12-17 Torbj”rn & Henrik Gild†
|
|

	.globl _old_i6_int
	.globl _interrupt_i6
	.globl _svethlana_int
	.globl _set_old_int_lvl
	.globl _set_int_lvl6
	.globl _int_off
	.globl _int_on

	.text


	dc.l	0x58425241		| XBRA
	dc.l	0x00000000		| (no cookie)
_old_i6_int:
	ds.l	1
_interrupt_i6:
	move.w	(sp),oldSR
	movem.l	a0-a6/d0-d7,-(sp)
	bsr	_svethlana_int
	movem.l	(sp)+,a0-a6/d0-d7
	rte

oldSR:		ds.w	1
save_SR:	ds.w	1

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

| Sets interrupt level to 6
_set_int_lvl6:
	move.w	d0,-(sp)
	move.w	sr,d0
	andi.w	#0xf0ff,d0
	ori.w	#0x0600,d0
	move.w	d0,sr
	move.w	(sp)+,d0
	rts

| Saves SR reg to a variable and turns off interrupts
_int_off:
	move.w	sr,save_SR
	move.w	#0x2700,sr
	rts

| Turns on interrupts by restoring SR. Only call this after having called int_off!
_int_on:
	move.w	save_SR,sr
	rts
