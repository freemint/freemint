
#define MOVEC_CACR_D0       .dc.l 0x4e7a0002        /* 68020-68060 */
#define MOVEC_D0_CACR       .dc.l 0x4e7b0002        /* 68020-68060 */
#define CPUSHA_BC			.dc.w 0xf4f8
#define CPUSHA_DC			.dc.w 0xf478			/* 68040-68060 */
#define CINVA_IC			.dc.w 0xf498			/* 68040-68060 */

	.globl _clear_cpu_cf
	.globl _clear_cpu_040
	.globl _clear_cpu_030

_clear_cpu_cf:
#ifdef __mcoldfire__
	nop					/* synchronize/flush store buffer */
	moveq.l #0,d0		/* initialize way counter */
	moveq.l #0,d1		/* initialize set counter */
	move.l  d0,a0		/* initialize cpushl pointer */

setloop:
	cpushl	bc,(a0) 	/* push cache line a0 (both caches) */
	add.l	#0x0010,a0	/* increment set index by 1 */
	addq.l	#1,d1		/* increment set counter */
	cmpi.l	#512,d1 	/* are sets for this way done? */
	bne.s	setloop

	moveq.l #0,d1		/* set counter to zero again */
	addq.l	#1,d0		/* increment to next way */
	move.l	d0,a0		/* set = 0, way = d0 */
	cmpi.l	#4,d0		/* flushed all the ways? */
	bne.s	setloop
#endif
	rts

_clear_cpu_040:
	move.w	%sr,%d1
#ifdef __mcoldfire__
	move.l	%d1,%d0
	ori.l	#0x700,d0		| mask interrupts
	move.w	%d0,%sr
#else
	ori.w	#0x700,%sr		| mask interrupts
#endif

	CPUSHA_DC
	CINVA_IC

	move.w	%d1,%sr			| set back status register
	rts

_clear_cpu_030:
	move.w	%sr,%d1
#ifdef __mcoldfire__
	move.l	%d1,%d0
	ori.l	#0x700,d0		| mask interrupts
	move.w	%d0,%sr
#else
	ori.w	#0x700,%sr		| mask interrupts
#endif

	MOVEC_CACR_D0
	or.l	#0x808,%d0		| clear data and instruction cache
	MOVEC_D0_CACR

	move.w	%d1,%sr			| set back status register

	rts	
