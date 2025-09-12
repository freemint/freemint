| 
| Low level and interrupt routines for the RTL8012 driver
|
| 2000-08-02 Frank Naumann
| 2000-03-24 Vassilis Papathanassiou
|
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

	.globl SYM(old_vbl_int)
	.globl SYM(vbl_interrupt)
	.extern SYM(rtl8012_int)

	.text

	dc.l	XBRA_MAGIC
	dc.l	0x505a4950		| RTLp
SYM(old_vbl_int):
	ds.l	1
SYM(vbl_interrupt):
	movem.l	a0-a2/d0-d2,-(sp)
	bsr	SYM(rtl8012_int)
	movem.l	(sp)+,a0-a2/d0-d2
	move.l	SYM(old_vbl_int)(pc),-(sp)
	rts
