| 
| Low level and interrupt routines for the Unicorn USB driver
|
	.globl _old_vbl_int
	.globl _interrupt_vbl
	.globl _unicorn_int
	.globl _check_flock

	.text

oldSR:  ds.w    1

	dc.l	0x58425241		| XBRA
	dc.l	0x534d5343		| _USB
_old_vbl_int:
	ds.l	1
_interrupt_vbl:
	lea -64(sp),sp
	movem.l	a0-a7/d0-d7,(sp)
	bsr	_unicorn_int
	movem.l	(sp),a0-a7/d0-d7
	lea 64(sp),sp
	move.l	_old_vbl_int(PC),-(sp)
	rts

| Checks & Sets FLOCK
_check_flock:
	tas.b 0x43e
	bne.b done
	move.b #0, D0
	rts
done:
	move.b #0xFF, D0
	rts

