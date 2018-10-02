	.globl clear_cpu_caches
	.globl Getcookie

clear_cpu_caches:
	clr.l	-(sp)
	lea		(sp),a0
	move.l	#0x5f435055,d0		; search cpu type
	jsr		Getcookie		; WORD Getcookie(LONG id, LONG *value)
	move.l	(sp)+,d1		; cookie value

	move.w	sr,-(sp)
	ori.w	#0x700,sr		; mask interrupts

	cmpi.w	#40,d1			; 68040?
	blt		clear_cpu030

	.dc.w 0xf478				; cpusha dc for 060
	.dc.w 0xf498				; cinva ic for 060
	bra	clear_cpu_exit

clear_cpu030:
	cmp.w	#20,d1			; 68020 or 68030?
	blt	clear_cpu_exit

	.dc.l 0x4e7a0002		; movec cacr,d0
	or.l	#0x808,d0		; clear data and instruction cache
	.dc.l 0x4e7b0002		; movec d0,cacr

clear_cpu_exit:
	move.w	(sp)+,sr		; set back status register
	rts	
