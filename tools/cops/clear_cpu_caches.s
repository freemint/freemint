
	.globl _clear_cpu_caches

_clear_cpu_caches:
	clr.l	-(sp)
	move.l	#'_CPU',d0		| search cpu type
	movea.l	sp,a0
	bsr	get_cookie		| WORD get_cookie(LONG id, LONG *value)
	move.l	(sp)+,d1		| cokie value
	tst.w	d0
	bne.s	clear_cpu040
	moveq	#0,d1
clear_cpu040:
	move.w	sr,-(sp)
	ori	#$700,sr		| mask interrupts

	cmpi.w	#40,d1			| 68040?
	blt.s	clear_cpu030

	cpusha	bc			| clear both caches
	bra.s	clear_cpu_exit

clear_cpu030:
	cmp.w	#20,d1			| 68020 or 68030?
	blt.s	clear_cpu_exit

	movec	cacr,d0
	or.l	#$808,d0		| clear data and instruction cache
	movec	d0,cacr

clear_cpu_exit:
	move.w	(sp)+,sr		| set back status register
	rts	
