	.globl clear_cpu_cf
	.globl clear_cpu_040
	.globl clear_cpu_030

clear_cpu_cf:
	nop					/* synchronize/flush store buffer */
	moveq.l #0,d0		/* initialize way counter */
	moveq.l #0,d1		/* initialize set counter */
	move.l  d0,a0		/* initialize cpushl pointer */

setloop:
	dc.w	0xf4e8	 	/* cpushl bc,(a0) push cache line a0 (both caches) */
	add.l	#0x0010,a0	/* increment set index by 1 */
	addq.l	#1,d1		/* increment set counter */
	cmpi.l	#512,d1 	/* are sets for this way done? */
	bne.s	setloop

	moveq.l #0,d1		/* set counter to zero again */
	addq.l	#1,d0		/* increment to next way */
	move.l	d0,a0		/* set = 0, way = d0 */
	cmpi.l	#4,d0		/* flushed all the ways? */
	bne.s	setloop
	rts

clear_cpu_040:
	move.w	sr,d1
	ori.w	#0x700,sr		; mask interrupts

	.dc.w 0xf478				; cpusha dc for 060
	.dc.w 0xf498				; cinva ic for 060
	move.w	d1,sr		; set back status register
	rts

clear_cpu_030:
	move.w	sr,d1
	ori.w	#0x700,sr		; mask interrupts

	.dc.l 0x4e7a0002		; movec cacr,d0
	or.l	#0x808,d0		; clear data and instruction cache
	.dc.l 0x4e7b0002		; movec d0,cacr
	move.w	d1,sr		; set back status register

	rts	
