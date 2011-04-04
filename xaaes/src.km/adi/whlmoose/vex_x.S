		.globl	_motv
		.globl	_cmotv
		.globl	_butv
		.globl	_cbutv
		.globl	_whlv
		.globl	_cwhlv
		.globl	_timv
	
		.globl _sample_x
		.globl _sample_y
		.globl _sample_butt
		.globl _sample_wheel
		.globl _sample_wclicks
		.globl _th_wrapper
		
		.text
|** These are exchanged VDI change vectors ****************************************************
_motv:		move.w	d0,_sample_x
		move.w	d1,_sample_y
		movem.l	d0-d2/a0-a2,-(sp)
		jsr	_cmotv
		movem.l	(sp)+,d0-d2/a0-a2
		rts

_butv:
		movem.l	d0-d2/a0-a2,-(sp)
		move.w	d0,_sample_butt
		jsr	_cbutv
		movem.l	(sp)+,d0-d2/a0-a2
		rts

_whlv:
		movem.l	d0-d2/a0-a2,-(sp)
		move.w	d0,_sample_wheel
		move.w	d1,_sample_wclicks
		jsr	_cwhlv
		movem.l	(sp)+,d0-d2/a0-a2
		rts

_timv:		rts

_th_wrapper:	movem.l	d0-d7/a0-a6,-(sp)
		jsr	_timer_handler
		movem.l	(sp)+,d0-d7/a0-a6
		rts
