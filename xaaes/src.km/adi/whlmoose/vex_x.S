#include "mint/asmdefs.h"

		.globl	SYM(motv)
		.extern	SYM(cmotv)
		.globl	SYM(butv)
		.extern	SYM(cbutv)
		.globl	SYM(whlv)
		.extern	SYM(cwhlv)
		.globl	SYM(timv)
	
		.extern SYM(sample_x)
		.extern SYM(sample_y)
		.extern SYM(sample_butt)
		.extern SYM(sample_wheel)
		.extern SYM(sample_wclicks)
		.globl SYM(th_wrapper)
		.extern SYM(timer_handler)

		.text
|** These are exchanged VDI change vectors ****************************************************
SYM(motv):		move.w	d0,SYM(sample_x)
		move.w	d1,SYM(sample_y)
#ifdef __mcoldfire__
		lea	-24(sp),sp
		movem.l	d0-d2/a0-a2,(sp)
#else
		movem.l	d0-d2/a0-a2,-(sp)
#endif
		jsr	SYM(cmotv)
#ifdef __mcoldfire__
		movem.l	(sp),d0-d2/a0-a2
		lea	24(sp),sp
#else
		movem.l	(sp)+,d0-d2/a0-a2
#endif
		rts

SYM(butv):
#ifdef __mcoldfire__
		lea	-24(sp),sp
		movem.l	d0-d2/a0-a2,(sp)
#else
		movem.l	d0-d2/a0-a2,-(sp)
#endif
		move.w	d0,SYM(sample_butt)
		jsr	SYM(cbutv)
#ifdef __mcoldfire__
		movem.l	(sp),d0-d2/a0-a2
		lea	24(sp),sp
#else
		movem.l	(sp)+,d0-d2/a0-a2
#endif
		rts

SYM(whlv):
#ifdef __mcoldfire__
		lea	-24(sp),sp
		movem.l	d0-d2/a0-a2,(sp)
#else
		movem.l	d0-d2/a0-a2,-(sp)
#endif
		move.w	d0,SYM(sample_wheel)
		move.w	d1,SYM(sample_wclicks)
		jsr	SYM(cwhlv)
#ifdef __mcoldfire__
		movem.l	(sp),d0-d2/a0-a2
		lea	24(sp),sp
#else
		movem.l	(sp)+,d0-d2/a0-a2
#endif
		rts

SYM(timv):		rts

SYM(th_wrapper):
#ifdef __mcoldfire__
		lea	-60(sp),sp
		movem.l	d0-d7/a0-a6,(sp)
#else
		movem.l	d0-d7/a0-a6,-(sp)
#endif
		jsr	SYM(timer_handler)
#ifdef __mcoldfire__
		movem.l	(sp),d0-d7/a0-a6
		lea	60(sp),sp
#else
		movem.l	(sp)+,d0-d7/a0-a6
#endif
		rts
