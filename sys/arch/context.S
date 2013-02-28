/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1992 Eric R. Smith
 * Copyright 1992,1993,1994 Atari Corporation
 * All rights reserved.
 *
 *
 * routines for saving/restoring user contexts
 *
 * long build_context(struct context *sav, short fmt):
 *	Called from an interrupt handler (such as the trap #1 routine
 *	for system calls) saves the context of the interrupted
 *	routine. Assumes that no user registers have been changed
 *	since the interrupt, and that the PC and status register
 *	are still on the stack. Returns the stack pointer being used
 *	at the time of the interrupt **in register a1**.
 *	The fmt parameter is used on the 68000 to communicate the exception
 *	vector number; on >=68010 we use the vector offset from the frame.
 *
 * long save_context(struct context *sav):
 *	Saves the context of the calling routine in the area pointed
 *	to by sav. Save_context always returns 0 when initially called;
 *	this is so processes can (by suitably manipulating the
 *	saved registers) tell when the return from save_context is
 *	actually caused by restoring the context, e.g.:
 *		if (save_context(sav) == 0) {		<<-- L1
 *			/-* do some stuff *-/
 *			sav.regs[D0] = 1;	/-* for restore context *-/
 *			restore_context(sav);	/-* goes back to L1 *-/
 *		}
 *		else /-* this is the second time through *-/
 *
 * void restore_context(struct context *sav):
 *	Restores a context previously saved by build_context or save_context.
 *	Since the program counter is part of the context, this function
 *	will never return (it's like longjmp()). NOTE: this function should
 *	be used only to change contexts _within_ the same program, since
 *	it does NOT flush the ATC. See change_context
 *
 * void change_context(struct context *sav):
 *	Restores a context previously saved by build_context or save_context
 *	for a different process. Unlike restore_context, this one *does*
 *	flush the ATC.
 */

#ifndef __mcoldfire__
	.arch	68030
#endif

#include "magic/magic.i"

	.text

	.globl	_build_context
	.globl 	_save_context
	.globl	_restore_context
	.globl	_change_context

	.globl	_mcpu
	.globl	_fpu
	.globl	_framesizes
	.globl	_new_trace		// from intr.s
#ifdef WITH_MMU_SUPPORT
	.globl	_no_mem_prot
#endif

	.text

_build_context:
	move.l	a0,-(sp)		// save a0; we'll use it for scratch
	move.l	8(sp),a0		// get address of save area

	movem.l	d0-d7/a0-a6,(a0)	// save registers D0-D7/A0-A6

#ifdef WITH_MMU_SUPPORT
	tst.w	_no_mem_prot		// is there memory protection?
	bne.s	noprot1
#if defined (M68040) || defined (M68060)
	dc.w	0x4e7a,0x0806		// movec urp,d0
	move.l	d0,C_CRP(a0)
#else
	pmove	crp,C_CRP(a0)		// save CRP from MMU
	pmove	tc,C_TC(a0)		// save TC from MMU
#endif
noprot1:
#endif
	clr.b	C_PTRACE(a0)		// no pending traces, thanks!
	lea	12(sp),a1		// start of the interesting stack area
	move.w	(a1)+,d0		// 68000 fake frame format

#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	build_68k

	move.w	(a1)+,C_SFMT(a0)	// save format/vector word of context
	move.w	(a1)+,d0		// get SR of context
	move.w	d0,C_SR(a0)		// save it
	move.l	(a1)+,C_PC(a0)		// save PC of context

	move.w	C_SFMT(a0),d1
	andi.l	#0x3000,d1		// keep the filler size bits
	beq.s	build_stack_fixed
	addq.l	#2,a1			// assume 2-byte filler after the exception frame
build_stack_fixed:

	pea	short1(pc)		// return address
	bra	save_coldfire_fpu

build_68k:
#endif
#ifdef M68000
	move.w	(0x59e).w,d1		// get process frame flag
	bne.s	nojunk			// we have some junk on the stack
	move.w	d0,C_SFMT(a0)		// save fake frame format
	subq.w	#0x8,d0			// if bus error (note: subq is faster than
	beq.s	group0			// cmp, and we won't need d0 later)
	subq.w	#0x4,d0			// or address error (0xC==0x8+0x4)
	bne.s	nojunk
group0:
	move.l	(a1)+,C_INTERNAL(a0)	// stash it in the internal area
	move.l	(a1)+,C_INTERNAL+4(a0)	// if a debugger's interested
nojunk:
#endif
	move.w	(a1)+,d0		// get SR of context
	move.w	d0,C_SR(a0)		// save it
	move.l	(a1)+,C_PC(a0)		// save PC of context
#ifdef M68000
	tst.w	d1			// test longframe (AKP)
	beq.s	short1			// short
#endif

// save the FPU state
#ifdef	__mcoldfire__
	pea	nofpu(pc)		// return address
	bra	save_coldfire_fpu
#else
	tst.w	_fpu			// is there a true FPU in the system
	beq.s	nofpu

	fsave	C_FSTATE(a0)		// save internal state frame
#ifndef M68000
	cmp.w	#60,_mcpu+2		// on 68060 frame format is different
	bne.s	no60
	tst.b	C_FSTATE+2(a0)		// if NULL frame then the FPU is not in use
	beq.s	nofpu			// skip programmer's model save
	bra.s	save
no60:
#endif
	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq.s	nofpu			// skip programmer's model save
save:	fmovem.x fp0-fp7,C_FREGS(a0)		// save data registers
	fmovem.l fpcr/fpsr/fpiar,C_FCTRL(a0)	// and control registers
#endif /* __mcoldfire__ */

nofpu:
	lea	C_SFMT(a0),a2
#ifdef	__mcoldfire__
	moveq	#0,d1
#endif
	move.w	(a1)+,d1		// fetch frame format word
	move.w	d1,(a2)+		// and stash it away for later
#ifdef	__mcoldfire__
	lsr.l	#8,d1			// isolate the frame format identifier
	lsr.l	#4,d1
#else
	lsr.w	#8,d1			// isolate the frame format identifier
	lsr.w	#4,d1
#endif
	lea	_framesizes,a3
#ifdef	__mcoldfire__
	move.b	0(a3,d1.l),d1
#else
	move.b	0(a3,d1.w),d1
#endif
	beq.s	short1			// if no data to save, skip this
#ifdef	__mcoldfire__
	subq.l	#1,d1			// correct for first time through loop
#else
	subq.w	#1,d1			// correct for first time through loop
#endif
bcint:
	move.w	(a1)+,(a2)+		// copy CPU internal state
bcover:
#ifdef	__mcoldfire__
	subq.l	#1,d1
	bpl.s	bcint
#else
	dbra	d1,bcint
#endif
short1:
	move.l	a1,C_SSP(a0)		// a1 now points above the state frame
	move.l	usp,a1			// save user stack pointer
	move.l	a1,C_USP(a0)
	btst	#13,d0			// check for supervisor mode
	beq.s	L_CONT1			// user mode// we already have stack in a1
L_SUPER1:
	// moving from the save state buffer
	// means not testing longframe again. (AKP)
	move.l	C_SSP(a0),a1		// was using super stack pointer before interrupt
L_CONT1:
#ifdef	__mcoldfire__
	move.l	(0x408).w,d1
	move.l	d1,C_TERM(a0)		// save GEMDOS terminate vector
#else
	move.l	(0x408).w,C_TERM(a0) 	// save GEMDOS terminate vector
#endif
	move.l	(sp)+,C_A0(a0)		// save old register a0
	rts

_save_context:
	move.l	a0,-(sp)		// save a0
	move.l	8(sp),a0		// get address of context save area

	movem.l	d0-d7/a0-a6,(a0)	// save D0-D7/A0-A6

#ifdef WITH_MMU_SUPPORT
	tst.w	_no_mem_prot
	bne.s	noprot2
	move.w	sr,d1
#if defined(M68040) || defined(M68060)
	dc.w	0x4e7a,0x0806		// movec urp,d0
	move.l	d0,C_CRP(a0)
#else
	pmove	crp,C_CRP(a0)		// save the CRP from the MMU
	pmove	tc,C_TC(a0)		// save the TC from the MMU
#endif
	move.w	d1,sr
noprot2:
#endif

// save the FPU state
#ifdef	__mcoldfire__
	pea	nofpu2(pc)		// return address
	bra	save_coldfire_fpu
#else
// if running with a true coprocessor we need to save the FPU state

	tst.w	_fpu			// is there a true FPU in the system
	beq.s	nofpu2
	fsave	C_FSTATE(a0)		// save internal state frame
#ifndef M68000
	cmp.w	#60,_mcpu+2		// on 68060 frame format is different
	bne.s	no60_2
	tst.b	C_FSTATE+2(a0)		// if NULL frame then the FPU is not in use
	beq.s	nofpu2			// skip programmer's model save
	bra.s	save2
no60_2:
#endif
	tst.b	C_FSTATE(a0)			// if NULL frame then the FPU is not in use
	beq.s	nofpu2				// skip programmer's model save
save2:	fmovem.x fp0-fp7,C_FREGS(a0)		// save data registers
	fmovem.l fpcr/fpsr/fpiar,C_FCTRL(a0)	// and control registers
#endif /* __mcoldfire__ */

nofpu2:
// note: I am somewhat unsure of this assumption, viz that save_context
// can never be called in a situation where a co-processor
// mid-instruction stack frame would be required. I suspect this is a
// valid assumption, in which case the above FPU code is redundant, the
// next line is not however!

#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	save_68k

	move.w	#0x4000,C_SFMT(a0)	// standard frame/vector word

	bra.s	save_ptrace
save_68k:
#endif
	clr.w	C_SFMT(a0)		// mark as a 4 word stack frame
save_ptrace:
	clr.b	C_PTRACE(a0)		// no pending traces, thanks!

	lea	8(sp),a1
	move.l	a1,C_SSP(a0)		// save supervisor stack pointer
					// note that it should be pointing above the PC
	move.l	-4(a1),C_PC(a0)		// save PC
	move.l	usp,a1
	move.l	a1,C_USP(a0)		// save user stack pointer
#ifdef	__mcoldfire__
	move.w	sr,d1
	move.w	d1,C_SR(a0)		// save status register
	move.l	(0x408).w,d1
	move.l	d1,C_TERM(a0)		// save GEMDOS terminate vector
#else
	move.w	sr,C_SR(a0)		// save status register
	move.l	(0x408).w,C_TERM(a0)	// save GEMDOS terminate vector
#endif
	move.l	(sp)+,C_A0(a0)		// save old a0
	clr.l	d0			// return 0
	rts
_restore_context:
#ifdef	__mcoldfire__
	move.w	sr,d1
	ori.l	#0x0400,d1
	move.w	d1,sr
#else
	ori.w	#0x0400,sr
#endif
	move.l	4(sp),a0		// address of context save area

// Switch stacks now - starting now ssp is in the memory space of
// the process we're switching to. Thus, we can change memory context
// to there.

	move.l	C_SSP(a0),a1		// get supervisor stack pointer
	tst.b	(a1)			// touch the page for virtual memory programs
	tst.b	-63(a1)			// make sure stack can grow
	move.l	a1,sp
	move.l	C_USP(a0),a1
	move.l	a1,usp			// set user stack pointer
#ifdef	__mcoldfire__
	move.l	C_TERM(a0),d1
	move.l	d1,(0x408).w		// restore GEMDOS terminate vector
#else
	move.l	C_TERM(a0),(0x408).w	// restore GEMDOS terminate vector
#endif

// Set memory context now: actually, this isn't necessary, since
// we're coming back to a context in the same process as is running
// now.
//	tst.w	_no_mem_prot
//	bne.s	noprot3
//	pmove	C_CRP(a0),crp		// restore MMU root pointer
//	pmove	C_TC(a0),tc		// restore MMU control register
//noprot3:

#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	restore_68k

	bsr	restore_coldfire_fpu

	move.l	C_PC(a0),-(sp)		// push the PC
	move.w	C_SR(a0),-(sp)		// push the status register
	move.w	#0x4000,-(sp)		// push the frame/vector word
	// Since we are just going to return using rte very soon,
	// we can push a standard frame/vector word instead of C_SFMT(a0),
	// so we don't have to add any extra filler byte.

	bra.s	restore_check_trace
restore_68k:
#endif
#ifdef M68000
	tst.w	(0x59e).w		// test longframe (AKP)
	beq.s	short3
#endif
	lea	C_SFMT(a0),a1
	move.w	(a1)+,d0		// fetch frame format word
#ifdef	__mcoldfire__
	moveq	#0,d1
#endif
	move.w	d0,d1			// copy it for later
#ifdef	__mcoldfire__
	lsr.l	#8,d1			// isolate the frame format identifier
	lsr.l	#4,d1
#else
	lsr.w	#8,d1			// isolate the frame format identifier
	lsr.w	#4,d1
#endif
	lea	_framesizes,a2
#ifdef	__mcoldfire__
	move.b	0(a2,d1.l),d1
#else
	move.b	0(a2,d1.w),d1
#endif
	beq.s	rcovernc		// if no data to copy, skip the copy
#ifdef	__mcoldfire__
	sub.l	d1,sp
	sub.l	d1,sp
#else
	sub.w	d1,sp
	sub.w	d1,sp
#endif
	move.l	sp,a2
#ifdef	__mcoldfire__
	subq.l	#1,d1			// correct for first time through loop
#else
	subq.w	#1,d1			// correct for first time through loop
#endif
rcint:
	move.w	(a1)+,(a2)+
rcover:
#ifdef	__mcoldfire__
	subq.l	#1,d1
	bpl.s	rcint
#else
	dbf	d1,rcint
#endif
rcovernc:
	move.w	d0,-(sp)		// frame format identifier

// restore the FPU state
#ifdef __mcoldfire__
	bsr	restore_coldfire_fpu
#else
// if running with a true coprocessor we need to restore the FPU state
	tst.w	_fpu			// is there a true FPU in the system
	beq.s	short3
#ifndef M68000
	cmp.w	#60,_mcpu+2		// on 68060 frame format is different
	bne.s	no60_3
	tst.b	C_FSTATE+2(a0)
	beq.s	short4
	bra.s	save3
no60_3:
#endif
	tst.b	C_FSTATE(a0)			// if NULL frame then the FPU is not in use
	beq.s	short4				// skip programmer's model restore
save3:	fmovem.l C_FCTRL(a0),fpcr/fpsr/fpiar	// restore control registers
	fmovem.x C_FREGS(a0),fp0-fp7		// and data registers
short4:
	frestore C_FSTATE(a0)			// finally the internal state
#endif /* __mcoldfire__ */

short3:
	move.l	C_PC(a0),-(sp)		// push the PC
	move.w	C_SR(a0),-(sp)		// push the status register
restore_check_trace:
	tst.b	C_PTRACE(a0)		// check for a pending trace
	movem.l	(a0),d0-d7/a0-a6	// restore registers d0-d7/a0-a6
	beq.s	notrace
	jmp	_new_trace
notrace:
	rte				// jump back to old context


_change_context:
#ifdef	__mcoldfire__
	move.w	sr,d1
	ori.l	#0x0400,d1		// mask interrupts
	move.w	d1,sr
#else
	ori.w	#0x0400,sr		// mask interrupts
#endif
	move.l	4(sp),a0		// address of context save area

// Switch stacks now - starting now ssp is in the memory space of
// the process we're switching to. Thus, we can change memory context
// to there.

	move.l	C_SSP(a0),a1		// get supervisor stack pointer
	tst.b	(a1)			// touch the page for virtual memory programs
	tst.b	-63(a1)			// make sure stack can grow
	move.l	a1,sp
	move.l	C_USP(a0),a1
	move.l	a1,usp			// set user stack pointer
#ifdef	__mcoldfire__
	move.l	C_TERM(a0),d1
	move.l	d1,(0x408).w		// restore GEMDOS terminate vector
#else
	move.l	C_TERM(a0),(0x408).w	// restore GEMDOS terminate vector
#endif

#ifdef WITH_MMU_SUPPORT
// Set memory context now
	tst.w	_no_mem_prot
	bne.s	noprot4
	move.w	sr,d1
#if defined(M68040) || defined(M68060)
	move.l	C_CRP(a0),d0
	dc.w	0xf518			// pflusha
	dc.w	0x4e7b,0x0806		// movec d0,urp
	dc.w	0x4e7b,0x0807		// movec d0,srp
#else
	pflusha
	pmove	C_CRP(a0),crp		// restore MMU root pointer
	pmove	C_TC(a0),tc		// restore MMU control register
#endif
	move.w	d1,sr
noprot4:
#endif

#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	change_68k

	bsr	restore_coldfire_fpu

	move.l	C_PC(a0),-(sp)		// push the PC
	move.w	C_SR(a0),-(sp)		// push the status register
	move.w	#0x4000,-(sp)		// push the frame/vector word
	// Since we are just going to return using rte very soon,
	// we can push a standard frame/vector word instead of C_SFMT(a0),
	// so we don't have to add any extra filler byte.

	bra.s	change_check_trace
change_68k:
#endif
#ifdef M68000
	tst.w	(0x59e).w		// test longframe (AKP)
	beq.s	short6
#endif
	lea	C_SFMT(a0),a1
	move.w	(a1)+,d0		// fetch frame format word
#ifdef	__mcoldfire__
	moveq	#0,d1
#endif
	move.w	d0,d1			// copy it for later
#ifdef	__mcoldfire__
	lsr.l	#8,d1			// isolate the frame format identifier
	lsr.l	#4,d1
#else
	lsr.w	#8,d1			// isolate the frame format identifier
	lsr.w	#4,d1
#endif
	lea	_framesizes,a2
#ifdef	__mcoldfire__
	move.b	0(a2,d1.l),d1
#else
	move.b	0(a2,d1.w),d1
#endif
	beq.s	rcover2nc		// if no data to copy, skip it
#ifdef	__mcoldfire__
	sub.l	d1,sp
	sub.l	d1,sp
#else
	sub.w	d1,sp
	sub.w	d1,sp
#endif
	move.l	sp,a2
#ifdef	__mcoldfire__
	subq.l	#1,d1			// correct for first time through loop
#else
	subq.w	#1,d1			// correct for first time through loop
#endif
rcint2:
	move.w	(a1)+,(a2)+
rcover2:
#ifdef	__mcoldfire__
	subq.l	#1,d1
	bpl.s	rcint2
#else
	dbf	d1,rcint2
#endif
rcover2nc:
	move.w	d0,-(sp)		// frame format identifier

// restore the FPU state
#ifdef __mcoldfire__
	bsr	restore_coldfire_fpu
#else
// if running with a true coprocessor we need to restore the FPU state

	tst.w	_fpu			// is there a true FPU in the system
	beq.s	short6

#ifndef M68000
	cmp.w	#60,_mcpu+2		// on 68060 frame format is different
	bne.s	no60_4
	tst.b	C_FSTATE+2(a0)
	beq.s	short5
	bra.s	save5
no60_4:
#endif
	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq.s	short5			// skip programmer's model restore
save5:	fmovem.l C_FCTRL(a0),fpcr/fpsr/fpiar	// restore control registers
	fmovem.x C_FREGS(a0),fp0-fp7		// and data registers
short5:
	frestore C_FSTATE(a0)			// finally the internal state
#endif /* __mcoldfire__ */

short6:
	move.l	C_PC(a0),-(sp)		// push the PC
	move.w	C_SR(a0),-(sp)		// push status register
change_check_trace:
	tst.b	C_PTRACE(a0)		// check for a pending trace
	movem.l	(a0),d0-d7/a0-a6	// restore registers d0-d7/a0-a6
	beq.s	notrace2
	jmp	_new_trace
notrace2:
	rte				// jump back to old context

#ifdef	__mcoldfire__
save_coldfire_fpu:
	fsave	C_FSTATE(a0)		// save internal state frame (including fpcr and fpsr)
	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq.s	save_coldfire_fpu_end	// skip programmer's model restore
	fmovem.d fp0-fp7,C_FREGS(a0)	// save data registers
	fmove.l	fpiar,C_FCTRL+8(a0)	// and control registers
save_coldfire_fpu_end:
	rts

restore_coldfire_fpu:
	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq.s	restore_coldfire_fpu_internal	// skip programmer's model restore
	fmove.l	C_FCTRL+8(a0),fpiar	// restore control registers
	fmovem.d C_FREGS(a0),fp0-fp7	// and data registers
restore_coldfire_fpu_internal:
	frestore C_FSTATE(a0)		// finally the internal state (including fpcr and fpsr)
	rts
#endif
