/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 */

//
// CPU tricks
//
//
// this below is only called from init.c on 68060 or newer

#include "mint/asmdefs.h"

#ifndef M68000

	.globl	SYM(get_superscalar)
	.globl	SYM(mcpu)			// in main.c

	FUNC(get_superscalar)
SYM(get_superscalar):
	CFI_STARTPROC()
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	get_superscalar_68k
	rts				// nothing to do
get_superscalar_68k:
#endif
#ifdef __mcoldfire__
	moveq	#60,d1
	cmp.w	SYM(mcpu)+2,d1
	bhi.s	ssret
#else
	cmp.w	#60,SYM(mcpu)+2
	bmi.s	ssret
#endif
	dc.l	0x4e7a0808		// movec pcr,d0
#ifdef __mcoldfire__
	ori.l	#0x0001,d0		// enable the superscalar dispatch
#else
	ori.w	#0x0001,d0		// enable the superscalar dispatch
#endif
	dc.l	0x4e7b0808		// movec d0,pcr
ssret:	rts
	CFI_ENDPROC()
	END(get_superscalar)

	.globl	SYM(is_superscalar)

	FUNC(is_superscalar)
SYM(is_superscalar):
	CFI_STARTPROC()
	clr.w	d0
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	beq.s	ssret			// not superscalar
#endif
#ifdef __mcoldfire__
	moveq	#60,d1
	cmp.w	SYM(mcpu)+2,d1
	bpl.s	ssret
#else
	cmp.w	#60,SYM(mcpu)+2
	bmi.s	ssret
#endif
	dc.l	0x4e7a0808		// movec pcr,d0
#ifdef __mcoldfire__
	andi.l	#0x0001,d0
#else
	andi.w	#0x0001,d0
#endif
	rts
	CFI_ENDPROC()
	END(is_superscalar)

//
// Cache tricks
//
	.globl	SYM(init_cache)

// this is a vector for jumping into appropriate cache
// routine. initialized at startup.

	.data
_cachevec:
	dc.l	_cache000
_cacheveci:
	dc.l	_cache000

// vectors for CCW/CACR conversions

l2cv:	dc.l	clq			// CCW -> CACR
c2lv:	dc.l	clq			// CACR -> CCW

// flag: cache present

hascaches:
	dc.w	0x00

#ifdef __mcoldfire__
// this is a variable to save the cacr value in the ColdFire
// initial value with BaS GCC          0xA4088020
// initial value with BaS Code Warrior 0xA008A020
// (values in November 2019)
cacr_saved:	dc.l	0xA4088020
#endif
	.text

// init cache routines

	FUNC(init_cache)
SYM(init_cache):
	CFI_STARTPROC()
#ifdef __mcoldfire__
	lea	-24(sp),sp
	movem.l	d0-d1/a0-a3,(sp)
#else
	movem.l	d0-d1/a0-a3,-(sp)
#endif
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	init_cache_68k

	lea	_cache_coldfire,a0
	movea.l	a0,a3
	lea	l2cv4e,a1
	lea	c2lv4e,a2

	bra.s	icset
init_cache_68k:
#endif
	move.l	SYM(mcpu),d0
	beq	icquit
	cmp.w	#10,d0
	beq	icquit
	lea	_cache030,a0
	movea.l	a0,a3
	lea	l2c020,a1
	lea	c2l020,a2
	cmp.w	#20,d0
	beq.s	icset
	lea	l2c030,a1
	lea	c2l030,a2
	cmp.w	#30,d0
	beq.s	icset
	lea	_cache040,a0
	lea	_cache040_i,a3
	lea	l2c040,a1
	lea	c2l040,a2
	cmp.w	#40,d0
	beq.s	icset
	lea	_cache060(pc),a0
	lea	_cache060_i(pc),a3
	lea	l2c060,a1
	lea	c2l060,a2
	cmp.w	#60,d0
	bne.s	icquit
icset:	move.l	a0,_cachevec
	move.l	a1,l2cv
	move.l	a2,c2lv
	move.l	a3,_cacheveci
#ifdef __mcoldfire__
	moveq	#-1,d0
	move.b	d0,hascaches
#else
	st	hascaches
#endif
	nop
icquit:
#ifdef __mcoldfire__
	movem.l	(sp),d0-d1/a0-a3
	lea	24(sp),sp
#else
	movem.l	(sp)+,d0-d1/a0-a3
#endif
	rts
	CFI_ENDPROC()
	END(init_cache)

// Cache control routines.
//
// These are intended to serve for (super) user programs so that
// they needn't to get into supervisor mode for controlling caches.
// Also, this code automagically handles the differences between
// 020, 030, 040 and 060 cache controls.
//
// The idea is that you use a logical Cache Control Word, which
// is automatically converted to and from physical CACR register.
// Bits of the CCW are defined as follows:
//
//                                                 020  030  040  060  CFv4e
// 0 - instruction cache enable                     x    x    x    x     x
// 1 - data cache enable                            -    x    x    x     x
// 2 - branch cache enable                          -    -    -    x     x
// 3 - instruction cache freeze                     x    x    -    -     -
// 4 - data cache freeze                            -    x    -    -     -
// 5 - instruction burst enable                     -    x    -    -     -
// 6 - data burst enable                            -    x    -    -     -
// 7 - data write allocate enable                   -    x    -    -     -
// 8 - instruction cache full mode enable           -    -    -    x     -
// 9 - instruction cache read/write allocate        -    -    -    x     -
// 10 - data cache full mode enable                 -    -    -    x     -
// 11 - data cache read/write allocate enable       -    -    -    x     -
// 12 - branch cache invalidate all                 -    -    -    x     x
// 13 - branch cache invalidate user entries        -    -    -    x     -
// 14 - CPUSH invalidate enable (data)              -    -    -    x     x
// 15 - store buffer enable                         -    -    -    x     x
// 16 - instruction cache invalidate all            -    -    -    -     x
// 17 - instruction cache mode default              -    -    -    -     x
// 18 - instruction cache half lock enable          -    -    -    -     x
// 19 - CPUSH invalidate enable (instruction)       -    -    -    -     x
// 20 - default cache-inhibited fill buffer         -    -    -    -     x
// 21 - data cache invalidate all                   -    -    -    -     x
// 22 - default data cache mode (bit 0)             -    -    -    -     x
// 23 - default data cache mode (bit 1)             -    -    -    -     x
// 24 - data cache half lock enable                 -    -    -    -     x
//
// This is translated into the physical CACR
// independently for each processor (as they differ).
//
// ssystem(S_CTRLCACHE, control, mask);
//
// where control is the actual Cache Control Word
// and mask indicates, what bits of CCW should be taken into account.
//

// table of default CCW masks for processors

ccw_dmasks:
	dc.l	0x00000000			// 68000
	dc.l	0x00000000			// 68010
	dc.l	0x00000009			// 68020
	dc.l	0x000000fb			// 68030
	dc.l	0x00000003			// 68040
	dc.l	0x00000000			// 68050 (does not exist)
	dc.l	0x0000ff07			// 68060

	.globl	SYM(ccw_getdmask)
	.globl	SYM(ccw_get)
	.globl	SYM(ccw_set)

// Set the CACR according to the Cache Control Word and Mask

	FUNC(ccw_set)
SYM(ccw_set):
	CFI_STARTPROC()
	tst.w	hascaches
	beq	clq
#ifdef __mcoldfire__
	lea	-20(sp),sp
	movem.l	d1-d2/a0-a2,(sp)
#else
	movem.l	d1-d2/a0-a2,-(sp)
#endif
	jbsr	SYM(ccw_get)		// get the current CCW
	move.l	d0,-(sp)		// save it for later reference
	move.l	28(sp),d1		// user specified CCW
	move.l	32(sp),d2		// user specified CCM
	and.l	d2,d1			// clear "unused" user-CCW bits
	not.l	d2			// negate all bits in CCM
	and.l	d2,d0			// clear bits to change in actual CCW
	or.l	d0,d1			// apply user CCW bits
	jbsr	SYM(ccw_getdmask)
#ifdef __mcoldfire__
	and.l	d1,d0
#else
	and.w	d1,d0
#endif
	move.l	d0,-(sp)		// save new CCW
	move.l	l2cv,a0			// convert new CCW ...
	jsr	(a0)
	move.l	(sp)+,d2		// restore new CCW
	move.l	(sp)+,d1		// restore old CCW
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne	cf_68K_emu
	btst	#0,d2			// instruction cache going to be enabled?
	beq.s	ic_off_cf
	btst	#0,d1			// if yes, is it currently disabled?
	bne.s	no_icache_cf
	or.l	#0x00000100,d0		// yes, so invalidate it (cinva ic)
	jbra	no_icache_cf
ic_off_cf:
	btst	#0,d1			// if it's going to be disabled,
	beq.s	no_icache_cf		// is it currently enabled?
	jbsr	_cpusha_ic_cf		// yes, so write it back (cpusha ic)
no_icache_cf:
	btst	#1,d2			// same as above, but for the data cache
	beq.s	dc_off_cf
	btst	#1,d1
	jbne	write_cacr
	or.l	#0x01000000,d0		// cinva dc
	jbra	write_cacr
dc_off_cf:
	btst	#1,d1
	jbeq	write_cacr
	jbsr	_cpusha_dc_cf		// cpusha dc
	jbra	write_cacr
cf_68K_emu:
	move.w	SYM(mcpu)+2,a0
	cmpa.l	#40,a0			// the following is only necessary
#else
	cmpi.w	#40,SYM(mcpu)+2		// the following is only necessary
#endif
	blt.s	lt040			// on 68040 and 68060 (copyback cache!)
	btst	#0,d2			// instruction cache going to be enabled?
	beq.s	no_icache
	btst	#0,d1			// if yes, is it currently disabled?
	bne.s	no_icache
	dc.w	0xF498			// yes, so invalidate it (cinva ic)
no_icache:
	btst	#1,d2			// same as above, but for the data cache
	beq.s	dc_off
	btst	#1,d1
	bne.s	write_cacr
	dc.w	0xF458			// cinva dc
	bra.s	lt040
dc_off:
	btst	#1,d1			// if it's going to be disabled,			
	beq.s	write_cacr		// is it currently enabled?
	dc.w	0xF478			// yes, so write it back (cpusha dc)
lt040:					// 020/030 have write-through cache but we
#ifdef __mcoldfire__			// need to invalidate it before enable it
	move.w	SYM(mcpu)+2,a0
	cmpa.l	#30,a0			// it's a 68020?
#else
	cmpi.w	#30,SYM(mcpu)+2		// it's a 68020?
#endif
	blt.s	inv_020ic		// 68020 has only intruction cache
	btst	#1,d2			// 030 data cache going to be enabled?
	jbeq	inv_020ic
	btst	#1,d1			// if yes, is it currently disabled?
	bne.s	inv_020ic
	or.l	#0b100000000000,d0	// cd bit (clear d-cache)
inv_020ic:				// invalidate 020 or 030 instruction cache
	btst	#0,d2			// 020/030 instruction cache going to be enabled?
	jbeq	write_cacr
	btst	#0,d1			// if yes, is it currently disabled?
	bne.s	write_cacr
	or.l	#0b000000001000,d0	// ci bit (clear i-cache)
write_cacr:
	movec	d0,%cacr		// write CACR
#ifdef __mcoldfire__
	movem.l	(sp),d1-d2/a0-a2
	lea	20(sp),sp
	tst.w	SYM(coldfire_68k_emulation)
	bne clq
	and.l	#0xfefffeff,d0		// clear cinva bits before saving
	move.l	d0,cacr_saved		// save new CACR value
#else
	movem.l	(sp)+,d1-d2/a0-a2
#endif
clq:	clr.l	d0			// E_OK
	rts
	CFI_ENDPROC()
	END(ccw_get)

// Extract Cache Control Word from the CACR

	FUNC(ccw_get)
SYM(ccw_get):
	CFI_STARTPROC()
	tst.w	hascaches
	beq.s	clq
#ifdef __mcoldfire__
	lea	-20(sp),sp
	movem.l	d1-d2/a0-a2,(sp)
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	ccw_get_cacr
	move.l	cacr_saved,d0
	bra.s	ccw_get_ccw
#else
	movem.l	d1-d2/a0-a2,-(sp)
#endif
ccw_get_cacr:
	dc.w	0x4e7a,0x0002		// movec cacr,d0
ccw_get_ccw:
	move.l	c2lv,a0			// convert
	jsr	(a0)
	move.l	d0,d1
	jbsr	SYM(ccw_getdmask)
	and.l	d1,d0
#ifdef __mcoldfire__
	movem.l	(sp),d1-d2/a0-a2
	lea	20(sp),sp
#else
	movem.l	(sp)+,d1-d2/a0-a2
#endif
	rts
	CFI_ENDPROC()
	END(ccw_get)

// get default Cache Control Mask for currently used CPU

	FUNC(ccw_getdmask)
SYM(ccw_getdmask):
	CFI_STARTPROC()
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	ccw_getdmask_68k
	move.l	#0x01ffd007,d0		// ColdFire CCW mask
	rts
ccw_getdmask_68k:
#endif
	move.l	SYM(mcpu),d0		// high word assumed 0
	divu.w	#5,d0
	and.l	#0x0000ffff,d0		// better to be safe...
	lsl.l	#1,d0
	lea	ccw_dmasks,a0
#ifdef __mcoldfire__
	move.l	0(a0,d0.l),d0		// _not_ a (a0,d0.w*2), see divu
#else
	move.l	0(a0,d0.w),d0		// _not_ a (a0,d0.w*2), see divu
#endif
	rts
	CFI_ENDPROC()
	END(ccw_getdmask)

// These are called by routines above. Notice CACR is ulong,
// while CCW is in fact ushort, but the user sees it as ulong too.

// 68020, bit 0 enables the cache, bit 1 freezes it.

l2c020:
#ifdef __mcoldfire__
	mvz.w	d0,d1
	and.l	#0x0001,d0		// leave the CE bit
	lsr.l	#0x02,d1		// shift the CF bit left
	or.l	d1,d0
#else
	move.w	d0,d1
	and.w	#0x0001,d0		// leave the CE bit
	lsr.w	#0x02,d1		// shift the CF bit left
	or.w	d1,d0
#endif
	rts

c2l020:
	move.l	d0,d1
#ifdef __mcoldfire__
	and.l	#0x0001,d0
	lsl.l	#0x02,d1
	or.l	d1,d0
#else
	and.w	#0x0001,d0
	lsl.w	#0x02,d1
	or.w	d1,d0
#endif
	rts

// 68030

cacr30:	dc.w	0x0001,0x0100,0x0000,0x0002
	dc.w	0x0200,0x0010,0x1000,0x2000

ccw30:	dc.w	0x0001,0x0008,0x0000,0x0000
	dc.w	0x0020,0x0000,0x0000,0x0000
	dc.w	0x0002,0x0010,0x0000,0x0000
	dc.w	0x0040,0x0080,0x0000,0x0000

l2c030:	
#ifdef __mcoldfire__
	move.l	d3,-(sp)
#endif
	lea	cacr30,a0
	clr.l	d1
	moveq	#0x07,d2
l30:
#ifdef __mcoldfire__
	lsr.l	#0x01,d0
#else
	lsr.w	#0x01,d0
#endif
	bcc.s	s30
#ifdef __mcoldfire__
	mvz.w	(a0),d3
	or.l	d3,d1
#else
	or.w	(a0),d1
#endif
s30:	addq.l	#0x02,a0
#ifdef __mcoldfire__
	subq.l	#1,d2
	bpl.s	l30
#else
	dbra.w	d2,l30
#endif
	move.l	d1,d0
#ifdef __mcoldfire__
	move.l	(sp)+,d3
#endif
	rts

c2l030:
#ifdef __mcoldfire__
	move.l	d3,-(sp)
#endif
	lea	ccw30,a0
	clr.l	d1
	moveq	#0x0f,d2
c30:
#ifdef __mcoldfire__
	lsr.l	#0x01,d0
#else
	lsr.w	#0x01,d0
#endif
	bcc.s	s31
#ifdef __mcoldfire__
	mvz.w	(a0),d3
	or.l	d3,d1
#else
	or.w	(a0),d1
#endif
s31:	addq.l	#0x02,a0
#ifdef __mcoldfire__
	subq.l	#1,d2
	bpl.s	c30
#else
	dbra.w	d2,c30
#endif
	move.l	d1,d0
#ifdef __mcoldfire__
	move.l	(sp)+,d3
#endif
	rts

// 68040, bit 15 enables the instruction cache, bit 31 enables the data cache.

l2c040:
#ifdef __mcoldfire__
	move.l	d1,-(sp)
	moveq	#0,d1
	btst	#0,d0			// IE bit
	beq.s	l2c040ieok
	bset	#15,d1
l2c040ieok:
	btst	#1,d0			// DE bit
	beq.s	l2c040deok
	bset	#31,d1
l2c040deok:
	move.l	d1,d0
	move.l	(sp)+,d1
#else
	ror.l	#0x01,d0		// move IE bit to position 31
	ror.w	#0x01,d0		// move DE bit to position 15
	swap	d0			// swap them
#endif
	rts

c2l040:
#ifdef __mcoldfire__
	move.l	d1,-(sp)
	moveq	#0,d1
	btst	#15,d0			// IE bit
	beq.s	c2l040ieok
	bset	#0,d1
c2l040ieok:
	btst	#31,d0			// DE bit
	beq.s	c2l040deok
	bset	#1,d1
c2l040deok:
	move.l	d1,d0
	move.l	(sp)+,d1
#else
	swap	d0
	rol.w	#0x01,d0
	rol.l	#0x01,d0
#endif
	rts

// 68060
//
// This is some special case, because function of the bits in CACR
// has been reverted in the CCW for consistency, so that each '1'
// means 'enable' rather than a 'disable' (in real 68060 CACR this
// varies).

cacr60:	dc.l	0x00008000,0x80000000,0x00800000,0x00000000
	dc.l	0x00000000,0x00000000,0x00000000,0x00000000
	dc.l	0x00002000,0x00004000,0x08000000,0x40000000
	dc.l	0x00400000,0x00200000,0x10000000,0x20000000

ccw60:	dc.w	0x0000,0x0100,0x0200,0x0001
	dc.w	0x0000,0x0000,0x0000,0x0000
	dc.w	0x0000,0x1000,0x2000,0x0004
	dc.w	0x0000,0x0000,0x0000,0x0400
	dc.w	0x4000,0x8000,0x0800,0x0002

l2c060:	lea	cacr60,a0
	clr.l	d1
	moveq	#0x0f,d2
l60:
#ifdef __mcoldfire__
	lsr.l	#0x01,d0
#else
	lsr.w	#0x01,d0
#endif
	bcc.s	s60
	or.l	(a0),d1
s60:	addq.l	#0x04,a0
#ifdef __mcoldfire__
	subq.l	#1,d2
	bpl.s	l60
#else
	dbra.w	d2,l60
#endif
	move.l	d1,d0
	eori.l	#0x58006000,d0		// adjust reverted bits
	rts

c2l060:
#ifdef __mcoldfire__
	move.l	d3,-(sp)
#endif
	lea	ccw60,a0
	lsr.l	#0x08,d0		// first 13 bits of CACR aren't used
	lsr.l	#0x04,d0
	clr.l	d1
	moveq	#0x13,d2
c60:	lsr.l	#0x01,d0
	bcc.s	s61
#ifdef __mcoldfire__
	mvz.w	(a0),d3
	or.l	d3,d1
#else
	or.w	(a0),d1
#endif
s61:	addq.l	#0x02,a0
#ifdef __mcoldfire__
	subq.l	#1,d2
	bpl.s	c60
#else
	dbra.w	d2,c60
#endif
	move.l	d1,d0
#ifdef __mcoldfire__
	eor.l	#0x4f00,d0		// adjust reverted bits
#else
	eor.w	#0x4f00,d0		// adjust reverted bits
#endif
#ifdef __mcoldfire__
	move.l	(sp)+,d3
#endif
	rts

#endif

// ColdFire v4e
#ifdef __mcoldfire__
cacrv4e:dc.l	0x00008000,0x80000000,0x00080000,0x00000000
	dc.l	0x00000000,0x00000000,0x00000000,0x00000000
	dc.l	0x00000000,0x00000000,0x00000000,0x00000000
	dc.l	0x00040000,0x00000000,0x10000000,0x20000000
	dc.l	0x00000100,0x00000400,0x00000800,0x00001000
	dc.l	0x00002000,0x01000000,0x02000000,0x04000000
	dc.l	0x08000000,0x00000000,0x00000000,0x00000000
	dc.l	0x00000000,0x00000000,0x00000000,0x00000000

ccwv4e:	dc.l	0x00000000,0x00000000,0x00000000,0x00000000
	dc.l	0x00010000,0x00000000,0x00020000,0x00040000
	dc.l	0x00080000,0x00100000,0x00000000,0x00000001
	dc.l	0x00000000,0x00000000,0x00001000,0x00000004
	dc.l	0x00000000,0x00000000,0x00000000,0x00000000
	dc.l	0x00200000,0x00400000,0x00800000,0x01000000
	dc.l	0x00004000,0x00008000,0x00000000,0x00000002

l2cv4e:
	lea	cacrv4e,a0
	clr.l	d1
	moveq	#0x1f,d2
lv4e:
	lsr.l	#0x01,d0
	bcc.s	sv4e
	or.l	(a0),d1
sv4e:
	addq.l	#0x04,a0
	subq.l	#1,d2
	bpl.s	lv4e

	move.l	d1,d0
	eori.l	#0x10001000,d0		// adjust reverted bits
	or.l	#0x00000020,d0		// always set EUSP bit
	rts

c2lv4e:
	move.l	d3,-(sp)
	lea	ccwv4e,a0
	lsr.l	#0x04,d0		// first 4 bits of CACR aren't used
	clr.l	d1
	moveq	#0x1c,d2
cv4e:
	lsr.l	#0x01,d0
	bcc.s	ssv4e
	mov.l	(a0),d3
	or.l	d3,d1

ssv4e:	addq.l	#0x04,a0
	subq.l	#1,d2
	bpl.s	cv4e

	move.l	d1,d0
	eor.l	#0x00084000,d0		// adjust reverted bits
	move.l	(sp)+,d3

	rts
#endif

	.globl	SYM(cpush)
	.globl	SYM(cpushi)

// Ozk:
// cpushi(void *base, long length):
// flush data cache and invalidate instruction cache from base over a distance of length.
// If length is -1 then the entire cache is flushed/invalidated
// Usage: When modifying/setting up process text area.

	FUNC(cpushi)
SYM(cpushi):
#ifndef M68000
	move.l	4(sp),a0
	move.l	8(sp),d0
	nop
	move.l	_cacheveci,a1
	jmp	(a1)
#endif
//
// cpush(void *base, long length):
// flush both caches from base over a distance of length. If length is -1
// then the entire cache is flushed
//
SYM(cpush):
#ifndef M68000
	move.l	4(sp),a0
	move.l	8(sp),d0
	nop				// flush pipelines
	move.l	_cachevec,a1
	jmp	(a1)


_cache030:				// 68030 cache
	dc.w	0x4e7a,0x1002		// movec cacr,d1
	move.l	d1,-(sp)
	addq.l	#1,d0			// if was -1
	beq.s	abc030			// then flush everything
	addq.l	#2,d0			// round up to long boundary
	lsr.l	#2,d0			// convert to number of longs
	cmp.l	#64,d0
	bcs.s	fls030			// dump selectively

abc030:
#ifdef __mcoldfire__
	or.l	#0x0808,d1
#else
	or.w	#0x0808,d1
#endif
	movec	d1,%cacr
	bra.s	rescacr

fls030:
#ifdef __mcoldfire__
	or.l	#0x0404,d1		// clear DC/IC entries
#else
	or.w	#0x0404,d1		// clear DC/IC entries
#endif
// run through d0+1 times (since a0 may not be on a long boundary)
do030:	dc.w	0x4e7b,0x8802		// movec a0,caar
	movec	d1,%cacr
	addq.l	#4,a0
#ifdef __mcoldfire__
	subq.l	#1,d0
	bpl.s	do030
#else
	dbf	d0,do030
#endif
rescacr:
	move.l	(sp)+,d0
	movec	d0,%cacr

_cache000:				// 68000 cache (i.e. no cache)
#endif
	rts
	END(cpushi)

#ifndef M68000
_cache040:				// 68040 cache
	move.l	d0,d1			// If no bytes to push, exit
	beq.s	_c060rts
	addq.l	#1,d1			// If was -1
	beq.s	abc040			// then flush everything
	move.l	a0,d1			// extract the position within the cacheline
	and.l	#0xf,d1			// which we use to calculate
	subq.l	#1,d1			// one byte at address 0xf in cacheline should still
	add.l	d1,d0			// only push this one cacheline
	lsr.l	#4,d0			// how many cachelines we need to push
	cmp.l	#256,d0
	bcs.s	fls040
abc040:	dc.w	0xF478			// this is "cpusha dc"
	rts
fls040:
// run through d0+1 times (since a0 may not be on a line boundary)
	move.l	a0,d1
#ifdef __mcoldfire__
	and.l	#0xfffffff0,d1
#else
	and.w	#0xfff0,d1
#endif
	movea.l	d1,a1
	moveq	#16,d1
do040:
	// As the MC68040 (and the MC68060, which uses the following code as
	// well) needs physical addresses for cpushl, we have to convert the
	// address(es) to push. This has to be done for each line, as memory
	// may not be linearly mapped (i.e. there may be gaps in the physical
	// memory used to form the logical address space). Of course, the
	// conversion would actually only be necessary when the address to push
	// crosses a page boundary, but I don't think the gain this could mean
	// would be enough to judge the additional overhead needed.
	move.l	a1,a0
	bsr	log2phys
	cmpa.l	#-1,a0
	beq.s	abc040			// if there's no physical address, flush all
	dc.w	0xF468			// this is "cpushl dc,(a0)"
	add.l	d1,a1
#ifdef __mcoldfire__
	subq.l	#1,d0
	bpl.s	do040
#else
	dbf	d0,do040
#endif
_c060rts:
	rts
	
_cache060:
	move.l	d0,d1			// If no bytes to push, exit
	beq.s	_c060rts
#ifndef __mcoldfire__
	addq.l	#1,d1			// If was -1
	beq.s	abc040			// then flush everything
	move.l	a0,d1			// extract the position within the cacheline
	and.l	#0xf,d1			// which we use to calculate
	subq.l	#1,d1			// one byte at address 0xf in cacheline should still
	add.l	d1,d0			// only push this one cacheline
	lsr.l	#4,d0			// how many cachelines we need to push
	cmp.l	#512,d0
	bcs.s	fls040
#endif
	dc.w	0xF478			// this is "cpusha dc"
	rts
// ---------------------------------------------------
// Ozk:
// This is 'cpushi' which flushes only the datacache and then
// invalidates the intruction cache. Intended to be used
// where text area of processes are modified/prepared
_cache040_i:				// 68040 cache
	move.l	d0,d1			// If no bytes to push, exit
	beq.s	_c060rtsi
	addq.l	#1,d1			// If was -1
	beq.s	abc040i			// then fulsh everything
	move.l	a0,d1			// extract the position within the cacheline
	and.l	#0xf,d1			// which we use to calculate
	subq.l	#1,d1			// one byte at address 0xf in cacheline should still
	add.l	d1,d0			// only push this one cacheline
	lsr.l	#4,d0			// how many cachelines we need to push
	cmp.l	#256,d0
	bcs.s	fls040i
abc040i:
	dc.w	0xF4F8			// this is "cpusha bc"
	rts
fls040i:
	// run through d0+1 times (since a0 may not be on a line boundary)
	move.l	a0,d1
#ifdef __mcoldfire__
	and.l	#0xfffffff0,d1
#else
	and.w	#0xfff0,d1
#endif
	movea.l	d1,a1
	moveq	#16,d1
do040i:
	// As the MC68040 (and the MC68060, which uses the following code as
	// well) needs physical addresses for cpushl, we have to convert the
	// address(es) to push. This has to be done for each line, as memory
	// may not be linearly mapped (i.e. there may be gaps in the physical
	// memory used to form the logical address space). Of course, the
	// conversion would actually only be necessary when the address to push
	// crosses a page boundary, but I don't think the gain this could mean
	// would be enough to judge the additional overhead needed.
	move.l	a1,a0
	jbsr	log2phys
	cmpa.l	#-1,a0
	beq.s	abc040i			// if there's no physical address, flush all
	dc.w	0xF468			// this is "cpushl dc,(a0)"
	dc.w	0xF4C8			// this is "cinvl bc,(a0)" (BC instead of just IC
					// as DC might not have been invalidated on 060)
	add.l	d1,a1
#ifdef __mcoldfire__
	subq.l	#1,d0
	bpl.s	do040i
#else
	dbf	d0,do040i
#endif
_c060rtsi:
	rts

_cache060_i:
	move.l	d0,d1			// If no bytes to push, exit
	beq.s	_c060rtsi
#ifndef __mcoldfire__
	addq.l	#1,d1			// If was -1
	beq.s	abc040i			// then fulsh everything
	move.l	a0,d1			// extract the position within the cacheline
	and.l	#0xf,d1			// which we use to calculate
	subq.l	#1,d1			// one byte at address 0xf in cacheline should still
	add.l	d1,d0			// only push this one cacheline
	lsr.l	#4,d0			// how many cachelines we need to push
	cmp.l	#512,d0
	bcs.s	fls040i
#endif
	dc.w	0xF478			// this is "cpusha dc"
	dc.w	0xF4D8			// this is "cinva bc"
	rts

#ifdef __mcoldfire__
_cache_coldfire:			// ColdFire cache
        // This code comes from the MCF547x Reference Manual
        // Section 7.11 Cache Management
        //
        // The ColdFire has no cinva instruction.
        // Instead, cpushl writes the modified cache data to the RAM
        // then invalidates the caches (data + instruction) except if
        // the bits DDPI and IDPI have been set in the CACR.
        //
        // The ColdFire V4e core has a 32 kB instruction cache
        // and a 32 kB data cache. Both caches have the same structure.
        // The data is stored in "Line" elements of 16 bytes.
        // The Lines are stored in a 2D array of 4 Ways * 512 Sets.
        //
        // The following changes have been made to the original code:
        // - flush both caches with "cpushl bc" instead of "cpushl dc"
        // - flush the 512 Sets (original code forgot the last one)
        
	nop			// synchronize/flush store buffer
	moveq.l #0,d0		// initialize way counter
	moveq.l #0,d1		// initialize set counter
	move.l  d0,a0		// initialize cpushl pointer

setloop:
	cpushl	%bc,(a0) 	// push cache line a0 (both caches)
	add.l	#0x0010,a0	// increment set index by 1
	addq.l	#1,d1		// increment set counter
	cmpi.l	#512,d1 	// are sets for this way done?
	bne.s	setloop

	moveq.l #0,d1		// set counter to zero again
	addq.l	#1,d0		// increment to next way
	move.l	d0,a0		// set = 0, way = d0
	cmpi.l	#4,d0		// flushed all the ways?
	bne.s	setloop
	rts

_cpusha_dc_cf:
	nop			// synchronize/flush store buffer
	lea	-12(sp),sp
	movem.l	d0-d1/a0,(sp)
	moveq.l #0,d0		// initialize way counter
	moveq.l #0,d1		// initialize set counter
	move.l  d0,a0		// initialize cpushl pointer

setloop_dc:
	cpushl	%dc,(a0) 	// push cache line a0
	add.l	#0x0010,a0	// increment set index by 1
	addq.l	#1,d1		// increment set counter
	cmpi.l	#512,d1 	// are sets for this way done?
	bne.s	setloop_dc

	moveq.l #0,d1		// set counter to zero again
	addq.l	#1,d0		// increment to next way
	move.l	d0,a0		// set = 0, way = d0
	cmpi.l	#4,d0		// flushed all the ways?
	bne.s	setloop_dc
	movem.l	(sp),d0-d1/a0
	lea	12(sp),sp
	rts

_cpusha_ic_cf:
	nop			// synchronize/flush store buffer
	lea	-12(sp),sp
	movem.l	d0-d1/a0,(sp)
	moveq.l #0,d0		// initialize way counter
	moveq.l #0,d1		// initialize set counter
	move.l  d0,a0		// initialize cpushl pointer

setloop_ic:
	cpushl	%ic,(a0) 	// push cache line a0
	add.l	#0x0010,a0	// increment set index by 1
	addq.l	#1,d1		// increment set counter
	cmpi.l	#512,d1 	// are sets for this way done?
	bne.s	setloop_ic

	moveq.l #0,d1		// set counter to zero again
	addq.l	#1,d0		// increment to next way
	move.l	d0,a0		// set = 0, way = d0
	cmpi.l	#4,d0		// flushed all the ways?
	bne.s	setloop_ic
	movem.l	(sp),d0-d1/a0
	lea	12(sp),sp
	rts
#endif

// ---------------------------------------------------

// log2phys
//
// Convert logical to physical address on 040 and 060. The method necessary
// for the 060 is a bit braindead, but Motorola decided to remove the ptest
// opcode in favor of plpa, which has the big disadvantage of triggering an
// access exception ("bus error") for logical addresses that do not have a
// (valid) physical counterpart - very clever :-/
//
// Bugs:
// - Assumes that the logical address to convert modulo pagesize is never
//   (pagesize - 1), as -1 (which is used to signal an unmapped address) could
//   be a valid answer in this case.
//
// Input:
// a0: logical address// use even addresses only to be on the safe side
//     (see "Bugs" above)
//
// Returns:
// a0: physical address (-1 if the address is not currently mapped)

log2phys:
#ifdef __mcoldfire__
	lea	-16(sp),sp
	movem.l	d0-d2/a1,(sp)		// save some registers
	move.w	SYM(mcpu)+2,d0
	cmpi.w	#60,d0			// is this a 060?
#else
	movem.l	d0-d2/a1,-(sp)		// save some registers
	cmpi.w	#60,SYM(mcpu)+2		// is this a 060?
#endif
	beq.s	l2p_060			// yes, so use plpa
	dc.w	0x4e7a,0x0003		// movec tc,d0
	btst	#15,d0
	beq.s	l2p_exit		// MMU is disabled, logical == physical
	// determine the current pagesize - 1
	move.l	#8191,d1
	btst	#14,d0
	bne.s	l2p_goon
	move.l	#4095,d1
l2p_goon:
	move.l	d1,d2
	not.l	d2			// ~(pagesize - 1)
	dc.w	0xf568			// let the MMU translate the address (ptestr (a0))
	dc.w	0x4e7a,0x0805		// movec mmusr,d0
	btst	#1,d0			// match with a transparent translation register?
	bne.s	l2p_exit		// yes, logical == physical
	btst	#0,d0			// was a valid desriptor found?
	bne.s	l2p_resident
	moveq	#-1,d0			// no, so return -1
	move.l	d0,a0
	bra.s	l2p_exit
l2p_resident:
	and.l	d2,d0			// extract the physical page address
#ifdef __mcoldfire__
	move.l	d0,d2
	move.l	a0,d0
	move.l	d2,a0
#else
	exg	d0,a0
#endif
	and.l	d1,d0			// extract the offset from the logical address
	add.l	d0,a0			// and build the effective phyiscal address
l2p_exit:
#ifdef __mcoldfire__
	movem.l	(sp),d0-d2/a1		// restore registers
	lea	16(sp),sp
#else
	movem.l	(sp)+,d0-d2/a1		// restore registers
#endif
	rts

	// Here's the code for the MC68060. Note that it needs a temporary
	// access exception handler because plpa may trigger such an exception
	// if the logical address doesn't have a valid physical counterpart
l2p_060:
	dc.w	0x4e7a,0x0003		// movec tc,d0
	btst	#15,d0
	beq.s	l2p_exit
	// Install the temporary access exception handler
	move.w	sr,d1
#ifdef __mcoldfire__
	move.w	sr,d0
	ori.l	#0x700,d0
	move.w	d0,sr
#else
	ori	#0x700,sr
#endif
	dc.w	0x4e7a,0x9801		// movec vbr,a1
#ifdef __mcoldfire__
	move.l	8(a1),d0
	move.l	d0,old_access
	move.l	#temp_access,d0
	move.l	d0,8(a1)
#else
	move.l	8(a1),old_access
	move.l	#temp_access,8(a1)
#endif
	move.l	sp,a1
	dc.w	0xf5c8			// translate the address (plpar (a0))
lp2_rte:
	dc.w	0x4e7a,0x9801		// movec vbr,a1
#ifdef __mcoldfire__
	move.l	old_access,d0
	move.l	d0,8(a1)
#else
	move.l	old_access,8(a1)
#endif
	move.w	d1,sr
	bra.s	l2p_exit
// This is the temporary access exception handler. If it gets called, there's
// no valid physical address for the logical address we want to convert.

old_access:
	dc.l	0
temp_access:
	movea.l	a1,sp
	moveq	#-1,d0
	move.l	d0,a0
	bra.s	lp2_rte
#endif

//
// Set the stack pointer to a new value
// Called when we're starting GEM from the exec_os vector
//
	.globl	SYM(setstack)
	FUNC(setstack)
SYM(setstack):
	CFI_STARTPROC()
	move.l	(sp)+,a0		// pop return address
	move.l	(sp)+,sp		// set stack pointer
	jmp	(a0)			// return
	CFI_ENDPROC()
	END(setstack)

// used for debugging

#ifdef DEBUG_INFO
	.globl	SYM(get_ssp)

	FUNC(get_ssp)
SYM(get_ssp):
	CFI_STARTPROC()
	move.l	sp,d0
	rts
	CFI_ENDPROC()
	END(get_ssp)
#endif

	.globl	SYM(get_usp)

	FUNC(get_usp)
SYM(get_usp):
	CFI_STARTPROC()
	move.l	%usp,a0
	move.l	a0,d0
	rts
	CFI_ENDPROC()
	END(get_usp)
