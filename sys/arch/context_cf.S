/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 */

#ifdef __mcoldfire__

#include "magic/magic.i"
#include "mint/asmdefs.h"

	.text

	.globl	SYM(build_context)
	.globl 	SYM(save_context)
	.globl	SYM(restore_context)
	.globl	SYM(change_context)

	.globl	SYM(framesizes)		// from cpu.S
	.globl	SYM(new_trace)		// from intr.s

	.text

	FUNC(build_context)
SYM(build_context):
	CFI_STARTPROC()
	move.l	a0,-(sp)		// save a0; we'll use it for scratch
	move.l	8(sp),a0		// get address of save area

	movem.l	d0-d7/a0-a6,(a0)	// save registers D0-D7/A0-A6

	clr.b	C_PTRACE(a0)		// no pending traces, thanks!
	lea	12(sp),a1		// start of the interesting stack area
	move.w	(a1)+,d0		// 68000 fake frame format

	tst.w	SYM(coldfire_68k_emulation)
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

	fsave	C_FSTATE(a0)		// save internal state frame (including fpcr and fpsr)
	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq	short1			// skip programmer's model save
	fmovem.d fp0-fp7,C_FREGS(a0)	// save data registers
	fmove.l	fpiar,C_FCTRL+8(a0)	// and control registers
	bra	short1

build_68k:
	move.w	(a1)+,d0		// get SR of context
	move.w	d0,C_SR(a0)		// save it
	move.l	(a1)+,C_PC(a0)		// save PC of context

// save the FPU state
	fsave	C_FSTATE(a0)		// save internal state frame (including fpcr and fpsr)
	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq.s	nofpu			// skip programmer's model save
	fmovem.d fp0-fp7,C_FREGS(a0)	// save data registers
	fmove.l	fpiar,C_FCTRL+8(a0)	// and control registers

nofpu:
	lea	C_SFMT(a0),a2
	moveq	#0,d1
	move.w	(a1)+,d1		// fetch frame format word
	move.w	d1,(a2)+		// and stash it away for later
	lsr.l	#8,d1			// isolate the frame format identifier
	lsr.l	#4,d1
	lea	SYM(framesizes),a3
	move.b	0(a3,d1.l),d1
	beq.s	short1			// if no data to save, skip this
	subq.l	#1,d1			// correct for first time through loop
bcint:
	move.w	(a1)+,(a2)+		// copy CPU internal state
bcover:
	subq.l	#1,d1
	bpl.s	bcint
short1:
	move.l	a1,C_SSP(a0)		// a1 now points above the state frame
	move.l	%usp,a1			// save user stack pointer
	move.l	a1,C_USP(a0)
	btst	#13,d0			// check for supervisor mode
	beq.s	L_CONT1			// user mode// we already have stack in a1
L_SUPER1:
	// moving from the save state buffer
	// means not testing longframe again. (AKP)
	move.l	C_SSP(a0),a1		// was using super stack pointer before interrupt
L_CONT1:
	move.l	(0x408).w,d1
	move.l	d1,C_TERM(a0)		// save GEMDOS terminate vector
	move.l	(sp)+,C_A0(a0)		// save old register a0
	rts
	CFI_ENDPROC()
	END(build_context)

	FUNC(save_context)
SYM(save_context):
	CFI_STARTPROC()
	move.l	a0,-(sp)		// save a0
	move.l	8(sp),a0		// get address of context save area

	movem.l	d0-d7/a0-a6,(a0)	// save D0-D7/A0-A6

// No FPU state to save: the kernel itself does not use the FPU and any
// user FPU activity was already suspended and saved by build_context on
// kernel entry. Mark the state frame as NULL so the restore skips the FPU.

	clr.l	C_FSTATE(a0)

	tst.w	SYM(coldfire_68k_emulation)
	bne.s	save_68k

	move.w	#0x4000,C_SFMT(a0)	// standard frame/vector word

	bra.s	save_ptrace
save_68k:
	clr.w	C_SFMT(a0)		// mark as a 4 word stack frame
save_ptrace:
	clr.b	C_PTRACE(a0)		// no pending traces, thanks!

	lea	8(sp),a1
	move.l	a1,C_SSP(a0)		// save supervisor stack pointer
					// note that it should be pointing above the PC
	move.l	-4(a1),C_PC(a0)		// save PC
	move.l	%usp,a1
	move.l	a1,C_USP(a0)		// save user stack pointer
	move.w	sr,d1
	move.w	d1,C_SR(a0)		// save status register
	move.l	(0x408).w,d1
	move.l	d1,C_TERM(a0)		// save GEMDOS terminate vector
	move.l	(sp)+,C_A0(a0)		// save old a0
	clr.l	d0			// return 0
	rts
	CFI_ENDPROC()
	END(save_context)

// On ColdFire there is no MMU-based memory protection, so change_context
// (which on 68k additionally flushes the ATC and reloads the MMU root
// pointers) is identical to restore_context.

	FUNC(restore_context)
	FUNC(change_context)
SYM(restore_context):
SYM(change_context):
	CFI_STARTPROC()
	move.w	sr,d1
	ori.l	#0x0400,d1		// mask interrupts
	move.w	d1,sr
	move.l	4(sp),a0		// address of context save area

// Switch stacks now - starting now ssp is in the memory space of
// the process we're switching to. Thus, we can change memory context
// to there.

	move.l	C_SSP(a0),a1		// get supervisor stack pointer
	tst.b	(a1)			// touch the page for virtual memory programs
	tst.b	-63(a1)			// make sure stack can grow
	move.l	a1,sp
	move.l	C_USP(a0),a1
	move.l	a1,%usp			// set user stack pointer
	move.l	C_TERM(a0),d1
	move.l	d1,(0x408).w		// restore GEMDOS terminate vector

	tst.w	SYM(coldfire_68k_emulation)
	bne.s	restore_68k

	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq.s	restore_fpu_done	// skip programmer's model restore
	fmove.l	C_FCTRL+8(a0),fpiar	// restore control registers
	fmovem.d C_FREGS(a0),fp0-fp7	// and data registers
restore_fpu_done:
	frestore C_FSTATE(a0)		// finally the internal state (including fpcr and fpsr)

	move.l	C_PC(a0),-(sp)		// push the PC
	move.w	C_SR(a0),-(sp)		// push the status register
	move.w	#0x4000,-(sp)		// push the frame/vector word
	// Since we are just going to return using rte very soon,
	// we can push a standard frame/vector word instead of C_SFMT(a0),
	// so we don't have to add any extra filler byte.

	bra.s	restore_check_trace
restore_68k:
	lea	C_SFMT(a0),a1
	move.w	(a1)+,d0		// fetch frame format word
	moveq	#0,d1
	move.w	d0,d1			// copy it for later
	lsr.l	#8,d1			// isolate the frame format identifier
	lsr.l	#4,d1
	lea	SYM(framesizes),a2
	move.b	0(a2,d1.l),d1
	beq.s	rcovernc		// if no data to copy, skip the copy
	sub.l	d1,sp
	sub.l	d1,sp
	move.l	sp,a2
	subq.l	#1,d1			// correct for first time through loop
rcint:
	move.w	(a1)+,(a2)+
rcover:
	subq.l	#1,d1
	bpl.s	rcint
rcovernc:
	move.w	d0,-(sp)		// frame format identifier

// restore the FPU state
	tst.b	C_FSTATE(a0)		// if NULL frame then the FPU is not in use
	beq.s	restore_fpu_done_68k	// skip programmer's model restore
	fmove.l	C_FCTRL+8(a0),fpiar	// restore control registers
	fmovem.d C_FREGS(a0),fp0-fp7	// and data registers
restore_fpu_done_68k:
	frestore C_FSTATE(a0)		// finally the internal state (including fpcr and fpsr)

	move.l	C_PC(a0),-(sp)		// push the PC
	move.w	C_SR(a0),-(sp)		// push the status register
restore_check_trace:
	tst.b	C_PTRACE(a0)		// check for a pending trace
	movem.l	(a0),d0-d7/a0-a6	// restore registers d0-d7/a0-a6
	beq.s	notrace
	jmp	SYM(new_trace)
notrace:
	rte				// jump back to old context
	CFI_ENDPROC()
	END(restore_context)
	END(change_context)

#endif /* __mcoldfire__ */
