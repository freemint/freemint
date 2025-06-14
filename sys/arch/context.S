/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1992 Eric R. Smith
 * Copyright 1992,1993,1994 Atari Corporation
 * All rights reserved.
 *
 * Routines for saving/restoring user contexts:
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
	.cpu	68030
#endif

#include "magic/magic.i"
#include "mint/asmdefs.h"

	.text

	.globl	SYM(build_context)
	.globl 	SYM(save_context)
	.globl	SYM(restore_context)
	.globl	SYM(change_context)

	.globl	SYM(mcpu)
	.globl	SYM(fpu)
	.globl	SYM(framesizes)
	.globl	SYM(new_trace)		// from intr.s
#ifdef WITH_MMU_SUPPORT
	.globl	SYM(no_mem_prot)
#endif

	.text

// Stack frame format Macro
.macro SWITCH_STACKS
	movem.l	C_PC(a0),d1/a1		// Load PC and SSP together (d1=PC, a1=SSP)
	tst.b	(a1)			// Touch page for VM
	tst.b	-63(a1)			// Check stack growth space
	move.l	a1,sp			// Restore SSP (a1 = SSP from movem)
	move.l	C_USP(a0),a1	// Load USP separately (not adjacent)
	move.l	a1,%usp			// Restore USP
#ifdef __mcoldfire__
	move.l	C_TERM(a0),d1
	move.l	d1,(0x408).w		// Restore GEMDOS terminate vector
#else
	move.l	C_TERM(a0),(0x408).w	// Restore GEMDOS terminate vector
#endif
.endm

.macro SETUP_FRAME_RESTORATION
	lea	C_SFMT(a0),a1
	move.w	(a1)+,d1		// Get frame format word directly into d1
#ifdef __mcoldfire__
	lsr.l	#8,d1			// ColdFire: First shift by 8
	lsr.l	#4,d1			// ColdFire: Then shift by 4 (total 12)
	add.l	d1,d1			// ColdFire: Convert to word offset (long operation)
	lea	framesizes_table(pc),a2	// ColdFire: Load table address
	move.w	0(a2,d1.l),d1		// ColdFire: Lookup with long index
#else
	lsr.w	#8,d1			// 68K: First shift by 8
	lsr.w	#4,d1			// 68K: Then shift by 4 (total 12)
	add.w	d1,d1			// 68K: Convert to word offset
	lea	framesizes_table(pc),a2	// 68K: Load table address
	move.w	0(a2,d1.w),d1		// 68K: Lookup with word index
#endif
	beq.s	99f			// Skip if no extra data (local label)
#ifdef __mcoldfire__
	sub.l	d1,sp			// Allocate space on stack
	sub.l	d1,sp
#else
	sub.w	d1,sp			// Allocate space on stack
	sub.w	d1,sp
#endif
	move.l	sp,a2
#ifdef __mcoldfire__
	subq.l	#1,d1			// Adjust loop counter
98:	move.w	(a1)+,(a2)+		// Copy internal state to stack
	subq.l	#1,d1
	bpl.s	98b
#else
	subq.w	#1,d1			// Adjust loop counter
98:	move.w	(a1)+,(a2)+		// Copy internal state to stack
	dbf	d1,98b
#endif
99:	move.w	-2(a1),-(sp)		// Push frame format word (reload from saved location)
								// a1 was incremented past the format word, so use -2(a1)
.endm

	FUNC(build_context)
SYM(build_context):
	CFI_STARTPROC()
	move.l	a0,-(sp)		// Save a0 (scratch use)
	move.l	8(sp),a0		// Get context save area address

	movem.l	d0-d7/a0-a6,(a0)	// Save registers D0-D7/A0-A6

#ifdef WITH_MMU_SUPPORT
	tst.w	SYM(no_mem_prot)	// Memory protection enabled?
	bne.s	noprot1
#if defined (M68040) || defined (M68060)
	dc.w	0x4e7a,0x0806		// movec urp,d0
	move.l	d0,C_CRP(a0)		// Save CRP
#else
	pmove	%crp,C_CRP(a0)		// Save MMU CRP
	pmove	%tc,C_TC(a0)		// Save MMU TC
#endif
noprot1:
#endif
	clr.b	C_PTRACE(a0)		// Clear pending trace flag
	lea	12(sp),a1		// Stack frame start
	move.w	(a1)+,d0		// 68000 fake frame format

#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	build_68k

	move.w	(a1)+,C_SFMT(a0)	// Save format/vector word
	move.w	(a1)+,d0		// Get SR
	move.w	d0,C_SR(a0)		// Save SR
	move.l	(a1)+,C_PC(a0)		// Save PC

	move.w	C_SFMT(a0),d1
	andi.l	#0x3000,d1		// Extract filler size bits
	beq.s	build_stack_fixed
	addq.l	#2,a1			// Skip 2-byte frame filler
build_stack_fixed:

	pea	short1(pc)		// Set return address
	bra	save_coldfire_fpu

build_68k:
#endif
#ifdef M68000
	move.w	(0x59e).w,d1		// Get process frame flag
	bne.s	nojunk			// Skip if no stack junk
	move.w	d0,C_SFMT(a0)		// Save fake frame format
	subq.w	#0x8,d0			// Check for bus error
	beq.s	group0			// (faster than cmp)
	subq.w	#0x4,d0			// Check for address error
	bne.s	nojunk
group0:
	move.l	(a1)+,C_INTERNAL(a0)	// Save internal state part 1
	move.l	(a1)+,C_INTERNAL+4(a0)	// Save internal state part 2
nojunk:
#endif
	move.w	(a1)+,d0		// Get SR
	move.w	d0,C_SR(a0)		// Save SR
	move.l	(a1)+,C_PC(a0)		// Save PC
#ifdef M68000
	tst.w	d1			// Check longframe flag (AKP)
	beq.s	short1			// Skip if short frame
#endif

// Save FPU state
#ifdef	__mcoldfire__
	pea	nofpu(pc)		// Set return address
	bra	save_coldfire_fpu
#else
	bsr	save_fpu_for_build
#endif

nofpu:
	lea	C_SFMT(a0),a2
#ifdef	__mcoldfire__
	moveq	#0,d1
#endif
	move.w	(a1)+,d1		// Fetch frame format word
	move.w	d1,(a2)+		// Store frame format
#ifdef	__mcoldfire__
	lsr.l	#8,d1			// Isolate frame format ID
	lsr.l	#4,d1
#else
	lsr.w	#8,d1			// Isolate frame format ID
	lsr.w	#4,d1
#endif
	lea	SYM(framesizes),a3
#ifdef	__mcoldfire__
	move.b	0(a3,d1.l),d1
#else
	move.b	0(a3,d1.w),d1
#endif
	beq.s	short1			// Skip if no extra data
#ifdef	__mcoldfire__
	subq.l	#1,d1			// Adjust loop counter
#else
	subq.w	#1,d1			// Adjust loop counter
#endif
#ifdef M68000
	// 68000: Use simple loop (optimized for code size)
bcint:
	move.w	(a1)+,(a2)+		// Copy CPU internal state
bcover:
	dbra	d1,bcint
#elif defined(__mcoldfire__)
	// ColdFire: Use simple loop (ColdFire restrictions)
bcint:
	move.w	(a1)+,(a2)+		// Copy CPU internal state
bcover:
	subq.l	#1,d1
	bpl.s	bcint	
#else
	// 68020+: Optimize for small frames using jump table
	cmp.w	#3,d1			// Check if ≤3 words (most common)
	bhi.s	bcint_loop		// Use loop for larger frames
	add.w	d1,d1			// Convert to word offset
	jmp	build_copy_table(pc,d1.w)
build_copy_table:
	bra.s	short1			// 0 words - done
	bra.s	build_copy_1w		// 1 word
	bra.s	build_copy_2w		// 2 words
	bra	build_copy_3w		// 3 words
build_copy_3w: move.w	(a1)+,(a2)+
build_copy_2w: move.w	(a1)+,(a2)+
build_copy_1w: move.w	(a1)+,(a2)+
	bra.s	short1
bcint_loop:
	move.w	(a1)+,(a2)+		// Copy CPU internal state
#ifdef	__mcoldfire__
	subq.l	#1,d1
	bpl.s	bcint_loop
#else
	dbra	d1,bcint_loop
#endif
#endif
short1:
	move.l	a1,C_SSP(a0)		// Save SSP position
	move.l	%usp,a1			// Get USP
	move.l	a1,C_USP(a0)		// Save USP
	btst	#13,d0			// Check supervisor mode
	beq.s	L_CONT1			// User mode: USP already saved
L_SUPER1:
	move.l	C_SSP(a0),a1		// Use saved SSP for supervisor mode
L_CONT1:
#ifdef	__mcoldfire__
	move.l	(0x408).w,d1
	move.l	d1,C_TERM(a0)		// Save GEMDOS terminate vector
#else
	move.l	(0x408).w,C_TERM(a0) 	// Save GEMDOS terminate vector
#endif
	move.l	(sp)+,C_A0(a0)		// Restore original a0
	rts
	CFI_ENDPROC()
	END(build_context)

	FUNC(save_context)
SYM(save_context):
	CFI_STARTPROC()
	move.l	a0,-(sp)		// Save a0
	move.l	8(sp),a0		// Get context save area address

	movem.l	d0-d7/a0-a6,(a0)	// Save registers D0-D7/A0-A6

#ifdef WITH_MMU_SUPPORT
	tst.w	SYM(no_mem_prot)	// Memory protection enabled?
	bne.s	noprot2
	move.w	sr,d1
#if defined(M68040) || defined(M68060)
	dc.w	0x4e7a,0x0806		// movec urp,d0
	move.l	d0,C_CRP(a0)		// Save CRP
#else
	pmove	%crp,C_CRP(a0)		// Save MMU CRP
	pmove	%tc,C_TC(a0)		// Save MMU TC
#endif
	move.w	d1,sr
noprot2:
#endif

// Save FPU state
#ifdef	__mcoldfire__
	pea	nofpu2(pc)		// Set return address
	bra	save_coldfire_fpu
#else
	bsr	save_fpu_for_save
#endif

nofpu2:
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	save_68k

	move.w	#0x4000,C_SFMT(a0)	// Standard frame/vector word
	bra.s	save_ptrace
save_68k:
#endif
	clr.w	C_SFMT(a0)		// Mark as 4-word stack frame
save_ptrace:
	clr.b	C_PTRACE(a0)		// Clear pending trace flag

	lea	8(sp),a1
	move.l	a1,C_SSP(a0)		// Save current SSP
	move.l	-4(a1),C_PC(a0)		// Save return address as PC
	move.l	%usp,a1
	move.l	a1,C_USP(a0)		// Save USP
#ifdef	__mcoldfire__
	move.w	sr,d1
	move.w	d1,C_SR(a0)		// Save SR
	move.l	(0x408).w,d1
	move.l	d1,C_TERM(a0)		// Save GEMDOS terminate vector
#else
	move.w	sr,C_SR(a0)		// Save SR
	move.l	(0x408).w,C_TERM(a0)	// Save GEMDOS terminate vector
#endif
	move.l	(sp)+,C_A0(a0)		// Restore original a0
	clr.l	d0			// Return 0 (first call)
	rts
	CFI_ENDPROC()
	END(save_context)

	FUNC(restore_context)
SYM(restore_context):
	CFI_STARTPROC()
#ifdef	__mcoldfire__
	move.w	sr,d1
	ori.l	#0x0400,d1
	move.w	d1,sr			// Disable interrupts
#else
	ori.w	#0x0400,sr		// Disable interrupts
#endif
	move.l	4(sp),a0		// Get context save area address

	SWITCH_STACKS			// Common stack switching (macro)

#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	restore_68k

	bsr	restore_coldfire_fpu

	move.l	C_PC(a0),-(sp)		// Push return PC
	move.w	C_SR(a0),-(sp)		// Push SR
	move.w	#0x4000,-(sp)		// Push standard frame marker
	bra.s	restore_check_trace
restore_68k:
#endif
#ifdef M68000
	tst.w	(0x59e).w		// Check longframe flag (AKP)
	beq.s	short3
#endif
	SETUP_FRAME_RESTORATION		// Common frame setup (macro)

// Restore FPU state
#ifdef __mcoldfire__
	bsr	restore_coldfire_fpu
#else
	bsr	restore_fpu_for_restore
#endif

short3:
	move.l	C_PC(a0),-(sp)		// Push return PC (load fresh, don't reuse d1)
	move.w	C_SR(a0),-(sp)		// Push SR
restore_check_trace:
	tst.b	C_PTRACE(a0)		// Check for pending trace
	movem.l	(a0),d0-d7/a0-a6	// Restore general registers
	beq.s	notrace
	jmp	SYM(new_trace)		// Handle trace exception
notrace:
	rte				// Return to saved context
	CFI_ENDPROC()
	END(restore_context)


	FUNC(change_context)
SYM(change_context):
	CFI_STARTPROC()
#ifdef	__mcoldfire__
	move.w	sr,d1
	ori.l	#0x0400,d1
	move.w	d1,sr			// Disable interrupts
#else
	ori.w	#0x0400,sr		// Disable interrupts
#endif
	move.l	4(sp),a0		// Get context save area address

	SWITCH_STACKS			// Common stack switching (macro)

#ifdef WITH_MMU_SUPPORT
// Set memory context
	tst.w	SYM(no_mem_prot)	// Memory protection enabled?
	bne.s	noprot4
	move.w	sr,d1
#if defined(M68040) || defined(M68060)
	move.l	C_CRP(a0),d0
	dc.w	0xf518			// pflusha (flush ATC)
	dc.w	0x4e7b,0x0806		// movec d0,urp
	dc.w	0x4e7b,0x0807		// movec d0,srp
#else
	pflusha				// Flush ATC
	pmove	C_CRP(a0),%crp		// Restore MMU CRP
	pmove	C_TC(a0),%tc		// Restore MMU TC
#endif
	move.w	d1,sr
noprot4:
#endif

#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	change_68k

	bsr	restore_coldfire_fpu

	move.l	C_PC(a0),-(sp)		// Push return PC
	move.w	C_SR(a0),-(sp)		// Push SR
	move.w	#0x4000,-(sp)		// Push standard frame marker
	bra.s	change_check_trace
change_68k:
#endif
#ifdef M68000
	tst.w	(0x59e).w		// Check longframe flag (AKP)
	beq.s	short6
#endif


// Restore FPU state
#ifdef __mcoldfire__
	bsr	restore_coldfire_fpu
#else
	bsr	restore_fpu_for_change
#endif
	SETUP_FRAME_RESTORATION		// Common frame setup (macro)

short6:
	move.l	C_PC(a0),-(sp)		// Push return PC
	move.w	C_SR(a0),-(sp)		// Push SR
change_check_trace:
	tst.b	C_PTRACE(a0)		// Check for pending trace
	movem.l	(a0),d0-d7/a0-a6	// Restore general registers
	beq.s	notrace2
	jmp	SYM(new_trace)		// Handle trace exception
notrace2:
	rte				// Return to saved context
	CFI_ENDPROC()
	END(change_context)

// Frame size lookup table (PC-relative access)
// Index by format field (bits 15-12 of format word) * 2 (word offset)
framesizes_table:
	.word	0	// Format 0: Normal 4-word frame (0 extra bytes)
	.word	0	// Format 1: Throwaway frame (0 extra bytes)  
	.word	4	// Format 2: 6-word frame (4 extra bytes)
	.word	4	// Format 3: Coprocessor frame (4 extra bytes)
	.word	8	// Format 4: 8-word frame (8 extra bytes)
	.word	0	// Format 5: Undefined (0 extra bytes)
	.word	0	// Format 6: Undefined (0 extra bytes)
	.word	0	// Format 7: Undefined (0 extra bytes)
	.word	58	// Format 8: 68010 bus error (58 extra bytes)
	.word	20	// Format 9: Coprocessor mid-instruction (20 extra bytes)
	.word	32	// Format A: 68020/030 short bus error (32 extra bytes)
	.word	92	// Format B: 68020/030 long bus error (92 extra bytes)
	.word	12	// Format C: 68040 bus error (12 extra bytes)
	.word	84	// Format D: 68060 bus error (84 extra bytes)
	.word	0	// Format E: Undefined (0 extra bytes)
	.word	0	// Format F: Undefined (0 extra bytes)

// Macro for common FPU save code
.macro SAVE_FPU_COMMON label_skip, label_cf
	tst.w	SYM(fpu)		// FPU present?
	beq.s	\label_skip
#ifdef __mcoldfire__
	bra	\label_cf
#else
	fsave	C_FSTATE(a0)		// Save FPU internal state
#ifndef M68000
	cmp.w	#60,SYM(mcpu)+2		// 68060 has different frame format
	bne.s	check_null_frame_\@
	tst.b	C_FSTATE+2(a0)		// Check for null frame
	beq.s	\label_skip		// Skip if FPU not in use
	bra.s	save_fpu_regs_\@
check_null_frame_\@:
#endif
	tst.b	C_FSTATE(a0)		// Check for null frame
	beq	\label_skip		// Skip if FPU not in use
save_fpu_regs_\@:	fmovem.x fp0-fp7,C_FREGS(a0)		// Save FP data registers
	fmovem.l fpcr/fpsr/fpiar,C_FCTRL(a0)	// Save FP control registers
#endif
.endm

// Macro for common FPU restore code
.macro RESTORE_FPU_COMMON label_skip, label_cf
	tst.w	SYM(fpu)		// FPU present?
	beq	\label_skip
#ifdef __mcoldfire__
	bra	\label_cf
#else
#ifndef M68000
	cmp.w	#60,SYM(mcpu)+2		// 68060 has different frame format
	bne.s	check_null_frame_\@
	tst.b	C_FSTATE+2(a0)		// Check for null frame
	beq	\label_skip		// Skip if FPU not in use
	bra.s	restore_fpu_regs_\@
check_null_frame_\@:
#endif
	tst.b	C_FSTATE(a0)		// Check for null frame
	beq	\label_skip		// Skip if FPU not in use
restore_fpu_regs_\@:	fmovem.l C_FCTRL(a0),fpcr/fpsr/fpiar	// Restore FP control registers
	fmovem.x C_FREGS(a0),fp0-fp7		// Restore FP data registers
	frestore C_FSTATE(a0)			// Restore FP internal state
#endif
.endm

save_fpu_for_build:
	SAVE_FPU_COMMON fpu_build_done, save_cf_build
save_cf_build:	
fpu_build_done:
	rts

save_fpu_for_save:
	SAVE_FPU_COMMON fpu_save_done, save_cf_save
save_cf_save:	
fpu_save_done:
	rts

restore_fpu_for_restore:
	RESTORE_FPU_COMMON fpu_restore_skip, restore_cf_restore
restore_cf_restore:	
fpu_restore_skip:
fpu_restore_done:
	rts

restore_fpu_for_change:
	RESTORE_FPU_COMMON fpu_change_skip, restore_cf_change
restore_cf_change:	
fpu_change_skip:
fpu_change_done:
	rts

#ifdef	__mcoldfire__
save_coldfire_fpu:
	fsave	C_FSTATE(a0)		// Save FPU state (includes control regs)
	tst.b	C_FSTATE(a0)		// Check for null frame
	beq.s	save_coldfire_fpu_end	// Skip if FPU not in use
	fmovem.d fp0-fp7,C_FREGS(a0)	// Save FP data registers
	fmove.l	fpiar,C_FCTRL+8(a0)	// Save FP instruction address
save_coldfire_fpu_end:
	rts

restore_coldfire_fpu:
	tst.b	C_FSTATE(a0)		// Check for null frame
	beq.s	restore_coldfire_fpu_internal // Skip if FPU not in use
	fmove.l	C_FCTRL+8(a0),fpiar	// Restore FP instruction address
	fmovem.d C_FREGS(a0),fp0-fp7	// Restore FP data registers
restore_coldfire_fpu_internal:
	frestore C_FSTATE(a0)		// Restore FP state (includes control regs)
	rts
#endif