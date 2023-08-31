| 
| Low level and interrupt routines for the Unicorn USB driver
|

#include "mint/asmdefs.h"

	.globl SYM(old_mfp_int)
	.globl SYM(interrupt_mfp)
	.extern SYM(unicorn_int)
	.globl SYM(check_flock)

	.text

oldSR:  ds.w    1

	dc.l	0x58425241		| XBRA
	dc.l	0x554e4943		| UNIC
SYM(old_mfp_int):
	ds.l	1
SYM(interrupt_mfp):
	lea -64(sp),sp
	movem.l	a0-a7/d0-d7,(sp)
	bsr	SYM(unicorn_int)
	movem.l	(sp),a0-a7/d0-d7
	lea 64(sp),sp
	move.l	SYM(old_mfp_int)(pc),-(sp)
	rts

| Checks & Sets FLOCK
SYM(check_flock):
	tst.w 0x43e
	bne.w done
	dc.w 0x50f9
	dc.l 0x43e
	move.b #0, d0
	rts
done:
	move.b #0xFF, d0
	rts
