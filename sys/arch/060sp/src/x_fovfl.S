/*
 *  XDEF ****************************************************************	
 * 	_fpsp_ovfl(): 060FPSP entry point for FP Overflow exception.	
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	FP Overflow exception in an operating system.			
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_long() - read instruction longword			
 * 	fix_skewed_ops() - adjust src operand in fsave frame		
 * 	set_tag_x() - determine optype of src/dst operands		
 * 	store_fpreg() - store opclass 0 or 2 result to FP regfile	
 * 	unnorm_fix() - change UNNORM operands to NORM or ZERO		
 * 	load_fpn2() - load dst operand from FP regfile			
 * 	fout() - emulate an opclass 3 instruction			
 * 	tbl_unsupp - add of table of emulation routines for opclass 0,2	
 * 	_fpsp_done() - "callout" for 060FPSP exit (all work done!)	
 * 	_real_ovfl() - "callout" for Overflow exception enabled code	
 * 	_real_inex() - "callout" for Inexact exception enabled code	
 * 	_real_trace() - "callout" for Trace exception code		
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the FP Ovfl exception stack frame
 * 	- The fsave frame contains the source operand
 *
 *  OUTPUT **************************************************************
 * 	Overflow Exception enabled:
 * 	- The system stack is unchanged
 * 	- The fsave frame contains the adjusted src op for opclass 0,2
 * 	Overflow Exception disabled:
 * 	- The system stack is unchanged
 * 	- The "exception present" flag in the fsave frame is cleared
 *
 *  ALGORITHM ***********************************************************
 * 	On the 060, if an FP overflow is present as the result of any
 *  instruction, the 060 will take an overflow exception whether the
 *  exception is enabled or disabled in the FPCR. For the disabled case,
 *  This handler emulates the instruction to determine what the correct
 *  default result should be for the operation. This default result is
 *  then stored in either the FP regfile, data regfile, or memory.
 *  Finally, the handler exits through the "callout" _fpsp_done()
 *  denoting that no exceptional conditions exist within the machine.
 * 	If the exception is enabled, then this handler must create the
 *  exceptional operand and place it in the fsave state frame, and store
 *  the default result (only if the instruction is opclass 3). For
 *  exceptions enabled, this handler must exit through the "callout"
 *  _real_ovfl() so that the operating system enabled overflow handler
 *  can handle this case.
 * 	Two other conditions exist. First, if overflow was disabled
 *  but the inexact exception was enabled, this handler must exit
 *  through the "callout" _real_inex() regardless of whether the result
 *  was inexact.
 * 	Also, in the case of an opclass three instruction where
 *  overflow was disabled and the trace exception was enabled, this
 *  handler must exit through the "callout" _real_trace().
 */

	.include "hdr.fpu"

	.xref _real_trace
	.xref _real_inex
	.xref _real_ovfl
	.xref _fpsp_done
	.xref set_tag_x
	.xref tbl_unsupp
	.xref load_fpn2
	.xref store_fpreg
	.xref unnorm_fix
	.xref fix_skewed_ops
	.xref fout
	.xref _imem_read_long

	.globl		_fpsp_ovfl

	.text
_fpsp_ovfl:

/* 	sub.l		#24,sp			; make room for src/dst */

	link.w		a6,#-LOCAL_SIZE	/*  init stack frame */

	fsave		FP_SRC(a6)		/*  grab the "busy" frame */

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	/*  save d0-d1/a0-a1 */
	fmovem.l		fpcr/fpsr/fpiar,USER_FPCR(a6) /*  save ctrl regs */
	fmovem.x		fp0-fp1,EXC_FPREGS(a6)	/*  save fp0-fp1 on stack */

/*  the FPIAR holds the "current PC" of the faulting instruction */
	move.l		USER_FPIAR(a6),EXC_EXTWPTR(a6)
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch the instruction words */
	move.l		d0,EXC_OPWORD(a6)

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	btst		#0x5,EXC_CMDREG(a6)	/*  is instr an fmove out? */
	bne.w		fovfl_out


	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		fix_skewed_ops		/*  fix src op */

/*  since, I believe, only NORMs and DENORMs can come through here, */
/*  maybe we can avoid the subroutine call. */
	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		set_tag_x		/*  tag the operand type */
	move.b		d0,STAG(a6)		/*  maybe NORM,DENORM */

/*  bit five of the fp extension word separates the monadic and dyadic operations */
/*  that can pass through fpsp_ovfl(). remember that fcmp, ftst, and fsincos */
/*  will never take this exception. */
	btst		#0x5,1+EXC_CMDREG(a6)	/*  is operation monadic or dyadic? */
	beq.b		fovfl_extract		/*  monadic */

	bfextu		EXC_CMDREG(a6){6:3},d0 /*  dyadic; load dst reg */
	bsr.l		load_fpn2		/*  load dst into FP_DST */

	lea		FP_DST(a6),a0		/*  pass: ptr to dst op */
	bsr.l		set_tag_x		/*  tag the operand type */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		fovfl_op2_done		/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM,DENORM,or ZERO */
fovfl_op2_done:
	move.b		d0,DTAG(a6)		/*  save dst optype tag */

fovfl_extract:

/* $;	move.l		FP_SRC_EX(a6),TRAP_SRCOP_EX(a6) */
/* $;	move.l		FP_SRC_HI(a6),TRAP_SRCOP_HI(a6) */
/* $;	move.l		FP_SRC_LO(a6),TRAP_SRCOP_LO(a6) */
/* $;	move.l		FP_DST_EX(a6),TRAP_DSTOP_EX(a6) */
/* $;	move.l		FP_DST_HI(a6),TRAP_DSTOP_HI(a6) */
/* $;	move.l		FP_DST_LO(a6),TRAP_DSTOP_LO(a6) */

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  pass rnd prec/mode */

	move.b		1+EXC_CMDREG(a6),d1
	andi.w		#0x007f,d1		/*  extract extension */

	andi.l		#0x00ff01ff,USER_FPSR(a6) /*  zero all but accured field */

	fmove.l		#0x0,fpcr		/*  zero current control regs */
	fmove.l		#0x0,fpsr

	lea		FP_SRC(a6),a0
	lea		FP_DST(a6),a1

/*  maybe we can make these entry points ONLY the OVFL entry points of each routine. */
	move.l		(tbl_unsupp.l,pc,d1.w*4),d1 /*  fetch routine addr */
	jsr		(tbl_unsupp.l,pc,d1.l*1)

/*  the operation has been emulated. the result is in fp0. */
/*  the EXOP, if an exception occurred, is in fp1. */
/*  we must save the default result regardless of whether */
/*  traps are enabled or disabled. */
	bfextu		EXC_CMDREG(a6){6:3},d0
	bsr.l		store_fpreg

/*  the exceptional possibilities we have left ourselves with are ONLY overflow */
/*  and inexact. and, the inexact is such that overflow occurred and was disabled */
/*  but inexact was enabled. */
	btst		#ovfl_bit,FPCR_ENABLE(a6)
	bne.b		fovfl_ovfl_on

	btst		#inex2_bit,FPCR_ENABLE(a6)
	bne.b		fovfl_inex_on

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6
/* $;	add.l		#24,sp */
	bra.l		_fpsp_done

/*  overflow is enabled AND overflow, of course, occurred. so, we have the EXOP */
/*  in fp1. now, simply jump to _real_ovfl()! */
fovfl_ovfl_on:
	fmovem.x		fp1,FP_SRC(a6)	/*  save EXOP (fp1) to stack */

	move.w		#0xe005,2+FP_SRC(a6)	/*  save exc status */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  do this after fmovem,other f<op>s! */

	unlk		a6

	bra.l		_real_ovfl

/*  overflow occurred but is disabled. meanwhile, inexact is enabled. Therefore, */
/*  we must jump to real_inex(). */
fovfl_inex_on:

	fmovem.x		fp1,FP_SRC(a6)	/*  save EXOP (fp1) to stack */

	move.b		#0xc4,1+EXC_VOFF(a6)	/*  vector offset = 0xc4 */
	move.w		#0xe001,2+FP_SRC(a6)	/*  save exc status */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  do this after fmovem,other f<op>s! */

	unlk		a6

	bra.l		_real_inex

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fovfl_out:


/* $;	move.l		FP_SRC_EX(a6),TRAP_SRCOP_EX(a6) */
/* $;	move.l		FP_SRC_HI(a6),TRAP_SRCOP_HI(a6) */
/* $;	move.l		FP_SRC_LO(a6),TRAP_SRCOP_LO(a6) */

/*  the src operand is definitely a NORM(!), so tag it as such */
	move.b		#NORM,STAG(a6)		/*  set src optype tag */

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  pass rnd prec/mode */

	and.l		#0xffff00ff,USER_FPSR(a6) /*  zero all but accured field */

	fmove.l		#0x0,fpcr		/*  zero current control regs */
	fmove.l		#0x0,fpsr

	lea		FP_SRC(a6),a0		/*  pass ptr to src operand */

	bsr.l		fout

	btst		#ovfl_bit,FPCR_ENABLE(a6)
	bne.w		fovfl_ovfl_on

	btst		#inex2_bit,FPCR_ENABLE(a6)
	bne.w		fovfl_inex_on

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6
/* $;	add.l		#24,sp */

	btst		#0x7,(sp)		/*  is trace on? */
	beq.l		_fpsp_done		/*  no */

	fmove.l		fpiar,0x8(sp)		/*  "Current PC" is in FPIAR */
	move.w		#0x2024,0x6(sp)	/*  stk fmt = 0x2; voff = 0x024 */
	bra.l		_real_trace

