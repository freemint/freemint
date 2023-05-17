/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 * set_mmu(crp,tc): called once, the first time a page table is built, to
 * switch away from the MMU setup that came from the ROM and into the setup
 * that we just built.  The CRP is actually on the stack, and it's 8 bytes.
 * The TC is four bytes at sp@(0xc).  "Nulls" is here because we need to
 * shove zeros into a few places.
 *
 */

#if defined(WITH_MMU_SUPPORT) && defined(M68030)

	.arch	68030
	.data
nulltc:
	dc.l	0

	.text

	.globl	_set_mmu
	.globl	_offset_tt_ram
	.globl	_sys_m_xalloc
	.globl	_sys_m_free

_set_mmu:
	movem.l	d0-d2/a0-a2,-(sp)
	tst.l	_offset_tt_ram
	beq.s	normal_set
	move.l	#end_set_mmu_called_st_ram - set_mmu_called_st_ram,d1
	clr.w	-(sp)			// ST-ram
	move.l	d1,-(sp)
//	move.w	#68,-(sp)		// Mxalloc
//	trap	#1
//	addq.l	#8,sp
	jsr	_sys_m_xalloc
 	addq.l	#6,sp
	tst.l	d0
	beq.s	normal_set		// no ST-ram
	move.l	d0,a2
	move.l	28(sp),crp2		// save crp & tc in ST-ram
	move.l	32(sp),crp2+4
	move.l	36(sp),tc2
	lea	set_mmu_called_st_ram.w(pc),a0
	move.l	a2,a1
	lsr.l	#1,d1
	subq.l	#1,d1
copy_st_ram:
	move.w	(a0)+,(a1)+
	dbf	d1,copy_st_ram
	move.w	sr,-(sp)
	or.w	#0x700,sr		// interrupts locked
	movec	cacr,d0
	or.w	#0x808,d0		// clear 030 caches
	movec	d0,cacr
	jsr	(a2)			// set_mmu called in ST-ram
	move.w	(sp)+,sr   		// interrupts unlocked
	pea	(a2)
//	move	#73,-(sp)		// Mfree
//	trap	#1
//	addq.l	#6,sp
	jsr	_sys_m_free
	addq.l	#4,sp
	movem.l	(sp)+,d0-d2/a0-a2
	rts
normal_set:
	movem.l	(sp)+,d0-d2/a0-a2
	pmove	(nulltc).l,tc		// turn off mmu
	pmove	nulltc,tt0
	pmove	nulltc,tt1
	pmove	4(sp),crp		// caution: crp is 8 bytes
	pflusha
	pmove	0xc(sp),tc
	rts

set_mmu_called_st_ram:

	lea	nulltc2(pc),a0
	pmove	(a0),tc			// turn off mmu
	pmove	(a0),tt0
	pmove	(a0),tt1
	lea	crp2(pc),a0
	pmove	(a0),crp		// caution: crp is 8 bytes
	pflusha
	lea	tc2(pc),a0
	pmove	(a0),tc
	rts

nulltc2:
	dc.l	0
crp2:	dc.l	0,0
tc2:	dc.l	0

end_set_mmu_called_st_ram:


//
// save_mmu, restr_mmu: save and restore the MMU setup that came from ROM
//
	.data
oldcrp:	dc.l	0,0
oldtc:	dc.l	0
oldtt0:	dc.l	0
oldtt1:	dc.l	0

	.text

	.globl	_save_mmu

_save_mmu:
	pmove	tc,oldtc
	pmove	tt0,oldtt0
	pmove	tt1,oldtt1
	pmove	crp,oldcrp
	rts


	.globl	_restr_mmu

_restr_mmu:
	pmove	(nulltc).l,tc
	pmove	oldtt0,tt0
	pmove	oldtt1,tt1
	pmove	oldcrp,crp
	pmove	oldtc,tc
	rts


	.globl	_flush_mmu

_flush_mmu:
	pflusha
	rts
#endif
