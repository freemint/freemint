| 
| Low level and interrupt routines for the RTL8012 driver
|
| 2000-08-02 Frank Naumann
| 2000-03-24 Vassilis Papathanassiou
|
|
	.globl _old_vbl_int
	.globl _vbl_interrupt
	.globl _rtl8012_int

	.text

	dc.l	0x58425241		| XBRA
	dc.l	0x505a4950		| RTLp
_old_vbl_int:
	ds.l	1
_vbl_interrupt:
	movem.l	a0-a2/d0-d2,-(sp)
	bsr	_rtl8012_int
	movem.l	(sp)+,a0-a2/d0-d2
	move.l	_old_vbl_int(PC),-(sp)
	rts
