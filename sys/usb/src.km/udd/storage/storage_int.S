|
| Low level and interrupt routines for the USB mass storage
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

#ifdef TOSONLY
	.globl SYM(old_etv_timer_int)
	.globl SYM(interrupt_storage)
	.globl SYM(lock)
	.extern SYM(storage_int)

	.text

	dc.l	XBRA_MAGIC
	dc.l	0x55535452		| USTR
SYM(old_etv_timer_int):
	ds.l	1
SYM(interrupt_storage):
	subq.l	#1,count
	bne	skip
	tas	SYM(lock)
	bne	skip
#ifdef __mcoldfire__
	| ColdFire V4e: TAS bypasses the D-cache, so the bit-7 it just set
	| in RAM is invisible to any cached reader of lock (e.g. install.c's
	| IN_POLL_ISR check). Sync the dcache line now via a plain cached
	| byte write of any non-zero value.
	moveq	#1,d0
	move.b	d0,SYM(lock)
#endif
#ifdef __mcoldfire__
	lea	-64(sp),sp
	movem.l a0-a7/d0-d7,(sp)
#else
	movem.l	a0-a7/d0-d7,-(sp)
#endif
	move.l	a7,oldSP
	lea	ourSP,a7
	bsr	SYM(storage_int)
	move.l	oldSP,a7
#ifdef __mcoldfire__
	lea	count,a0
	move.l	#100,(a0)
	movem.l (sp),a0-a7/d0-d7
	lea	64(sp),sp
#else
	movem.l	(sp)+,a0-a7/d0-d7
	move.l	#100,count
#endif
#ifdef __mcoldfire__
	clr.l	SYM(lock)
	| ColdFire V4e: push the lock cache line to RAM so the next TAS
	| sees the cleared value. lock is placed alone in its own 16-byte
	| cache line.
	lea	SYM(lock),a0
	nop
	cpushl	%dc,(a0)
	addq.l	#1,a0
	cpushl	%dc,(a0)
	addq.l	#1,a0
	cpushl	%dc,(a0)
	addq.l	#1,a0
	cpushl	%dc,(a0)
#else
	clr.w	SYM(lock)
#endif
skip:	move.l	SYM(old_etv_timer_int)(pc),-(sp)
	rts
#endif

	.data
	| lock lives alone in its own 16-byte D-cache line (MCF547x line
	| size) so the cpushl burst in the ISR tail flushes only lock,
	| never neighbouring state. Only relevant for ColdFire.
	.balign 16
SYM(lock):	ds.w 1
	.balign 16
count:	dc.l 100
	.bss
oldSP:	ds.l 1
	ds.l 1024
ourSP:
	.end
