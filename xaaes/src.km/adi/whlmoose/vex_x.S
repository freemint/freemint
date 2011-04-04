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
#ifdef __mcoldfire__
		lea	-24(sp),sp
		movem.l	d0-d2/a0-a2,(sp)
#else
		movem.l	d0-d2/a0-a2,-(sp)
#endif
		jsr	_cmotv
#ifdef __mcoldfire__
		movem.l	(sp),d0-d2/a0-a2
		lea	24(sp),sp
#else
		movem.l	(sp)+,d0-d2/a0-a2
#endif
		rts

_butv:
#ifdef __mcoldfire__
		lea	-24(sp),sp
		movem.l	d0-d2/a0-a2,(sp)
#else
		movem.l	d0-d2/a0-a2,-(sp)
#endif
		move.w	d0,_sample_butt
		jsr	_cbutv
#ifdef __mcoldfire__
		movem.l	(sp),d0-d2/a0-a2
		lea	24(sp),sp
#else
		movem.l	(sp)+,d0-d2/a0-a2
#endif
		rts

_whlv:
#ifdef __mcoldfire__
		lea	-24(sp),sp
		movem.l	d0-d2/a0-a2,(sp)
#else
		movem.l	d0-d2/a0-a2,-(sp)
#endif
		move.w	d0,_sample_wheel
		move.w	d1,_sample_wclicks
		jsr	_cwhlv
#ifdef __mcoldfire__
		movem.l	(sp),d0-d2/a0-a2
		lea	24(sp),sp
#else
		movem.l	(sp)+,d0-d2/a0-a2
#endif
		rts

_timv:		rts

_th_wrapper:
#ifdef __mcoldfire__
		lea	-60(sp),sp
		movem.l	d0-d7/a0-a6,(sp)
#else
		movem.l	d0-d7/a0-a6,-(sp)
#endif
		jsr	_timer_handler
#ifdef __mcoldfire__
		movem.l	(sp),d0-d7/a0-a6
		lea	60(sp),sp
#else
		movem.l	(sp)+,d0-d7/a0-a6
#endif
		rts
