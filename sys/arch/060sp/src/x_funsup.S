/*
 *  XDEF ****************************************************************	
 * 	_fpsp_unsupp(): 060FPSP entry point for FP "Unimplemented	
 * 		        Data Type" exception.				
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	FP Unimplemented Data Type exception in an operating system.	
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_{word,long}() - read instruction word/longword	
 * 	fix_skewed_ops() - adjust src operand in fsave frame		
 * 	set_tag_x() - determine optype of src/dst operands		
 * 	store_fpreg() - store opclass 0 or 2 result to FP regfile	
 * 	unnorm_fix() - change UNNORM operands to NORM or ZERO		
 * 	load_fpn2() - load dst operand from FP regfile			
 * 	load_fpn1() - load src operand from FP regfile			
 * 	fout() - emulate an opclass 3 instruction			
 * 	tbl_unsupp - add of table of emulation routines for opclass 0,2	
 * 	_real_inex() - "callout" to operating system inexact handler	
 * 	_fpsp_done() - "callout" for exit *  work all done		
 * 	_real_trace() - "callout" for Trace enabled exception		
 * 	funimp_skew() - adjust fsave src ops to "incorrect" value	
 * 	_real_snan() - "callout" for SNAN exception			
 * 	_real_operr() - "callout" for OPERR exception			
 * 	_real_ovfl() - "callout" for OVFL exception			
 * 	_real_unfl() - "callout" for UNFL exception			
 * 	get_packed() - fetch packed operand from memory			
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the "Unimp Data Type" stk frame	
 * 	- The fsave frame contains the ssrc op (for UNNORM/DENORM)	
 * 									
 *  OUTPUT **************************************************************	
 * 	If Inexact exception (opclass 3):				
 * 	- The system stack is changed to an Inexact exception stk frame	
 * 	If SNAN exception (opclass 3):					
 * 	- The system stack is changed to an SNAN exception stk frame	
 * 	If OPERR exception (opclass 3):					
 * 	- The system stack is changed to an OPERR exception stk frame	
 * 	If OVFL exception (opclass 3):					
 * 	- The system stack is changed to an OVFL exception stk frame	
 * 	If UNFL exception (opclass 3):					
 * 	- The system stack is changed to an UNFL exception stack frame	
 * 	If Trace exception enabled:					
 * 	- The system stack is changed to a Trace exception stack frame	
 * 	Else: (normal case)						
 * 	- Correct result has been stored as appropriate			
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Two main instruction types can enter here: (1) DENORM or UNNORM	
 *  unimplemented data types. These can be either opclass 0,2 or 3	
 *  instructions, and (2) PACKED unimplemented data format instructions	
 *  also of opclasses 0,2, or 3.						
 * 	For UNNORM/DENORM opclass 0 and 2, the handler fetches the src	
 *  operand from the fsave state frame and the dst operand (if dyadic)	
 *  from the FP register file. The instruction is then emulated by	
 *  choosing an emulation routine from a table of routines indexed by	
 *  instruction type. Once the instruction has been emulated and result	
 *  saved, then we check to see if any enabled exceptions resulted from	
 *  instruction emulation. If none, then we exit through the "callout"	
 *  _fpsp_done(). If there is an enabled FP exception, then we insert	
 *  this exception into the FPU in the fsave state frame and then exit	
 *  through _fpsp_done().							
 * 	PACKED opclass 0 and 2 is similar in how the instruction is	
 *  emulated and exceptions handled. The differences occur in how the	
 *  handler loads the packed op (by calling get_packed() routine) and	
 *  by the fact that a Trace exception could be pending for PACKED ops.	
 *  If a Trace exception is pending, then the current exception stack	
 *  frame is changed to a Trace exception stack frame and an exit is	
 *  made through _real_trace().						
 * 	For UNNORM/DENORM opclass 3, the actual move out to memory is	
 *  performed by calling the routine fout(). If no exception should occur	
 *  as the result of emulation, then an exit either occurs through	
 *  _fpsp_done() or through _real_trace() if a Trace exception is pending	
 *  (a Trace stack frame must be created here, too). If an FP exception	
 *  should occur, then we must create an exception stack frame of that	
 *  type and jump to either _real_snan(), _real_operr(), _real_inex(),	
 *  _real_unfl(), or _real_ovfl() as appropriate. PACKED opclass 3	
 *  emulation is performed in a similar manner.				
 * 									
 */


/*
 *  (1) DENORM and UNNORM (unimplemented) data types:
 * 
 *                              post-instruction
 *                              *****************
 *                              *      EA       *
 *       pre-instruction        *               *
 *      *****************       *****************
 *      * 0x0 *  0x0dc  *       * 0x3 *  0x0dc  *
 *      *****************       *****************
 *      *     Next      *       *     Next      *
 *      *      PC       *       *      PC       *
 *      *****************       *****************
 *      *      SR       *       *      SR       *
 *      *****************       *****************
 * 
 *  (2) PACKED format (unsupported) opclasses two and three:
 *      *****************
 *      *      EA       *
 *      *               *
 *      *****************
 *      * 0x2 *  0x0dc  *
 *      *****************
 *      *     Next      *
 *      *      PC       *
 *      *****************
 *      *      SR       *
 *      *****************
 */

	.include "hdr.fpu"

	.xref _real_trace
	.xref _real_operr
	.xref _real_inex
	.xref _real_snan
	.xref _real_unfl
	.xref _real_ovfl
	.xref _fpsp_done
	.xref set_tag_x
	.xref tbl_unsupp
	.xref load_fpn1
	.xref load_fpn2
	.xref store_fpreg
	.xref unnorm_fix
	.xref fix_skewed_ops
	.xref fout
	.xref _imem_read_long
	.xref _dmem_write
	.xref dnrm_lp
	.xref norm
	.xref get_packed

	.text

	.globl		_fpsp_unsupp
_fpsp_unsupp:

	link.w		a6,#-LOCAL_SIZE	/*  init stack frame */

	fsave		FP_SRC(a6)		/*  save fp state */

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	/*  save d0-d1/a0-a1 */
	fmovem.l		fpcr/fpsr/fpiar,USER_FPCR(a6) /*  save ctrl regs */
	fmovem.x		fp0-fp1,EXC_FPREGS(a6)	/*  save fp0-fp1 on stack */

	btst		#0x5,EXC_SR(a6)	/*  user or supervisor mode? */
	bne.b		fu_s
fu_u:
	move.l		usp,a0		/*  fetch user stack pointer */
	move.l		a0,EXC_A7(a6)		/*  save on stack */
	bra.b		fu_cont
/*  if the exception is an opclass zero or two unimplemented data type */
/*  exception, then the a7' calculated here is wrong since it doesn't */
/*  stack an ea. however, we don't need an a7' for this case anyways. */
fu_s:
	lea		0x4+EXC_EA(a6),a0	/*  load old a7' */
	move.l		a0,EXC_A7(a6)		/*  save on stack */

fu_cont:

/*  the FPIAR holds the "current PC" of the faulting instruction */
/*  the FPIAR should be set correctly for ALL exceptions passing through */
/*  this point. */
	move.l		USER_FPIAR(a6),EXC_EXTWPTR(a6)
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch the instruction words */
	move.l		d0,EXC_OPWORD(a6)	/*  store OPWORD and EXTWORD */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;; */

	clr.b		SPCOND_FLG(a6)		/*  clear special condition flag */

/*  Separate opclass three (fpn-to-mem) ops since they have a different */
/*  stack frame and protocol. */
	btst		#0x5,EXC_CMDREG(a6)	/*  is it an fmove out? */
	bne.w		fu_out			/*  yes */

/*  Separate packed opclass two instructions. */
	bfextu		EXC_CMDREG(a6){0:6},d0
	cmpi.b		#0x13,d0
	beq.w		fu_in_pack


/*  I'm not sure at this point what FPSR bits are valid for this instruction. */
/*  so, since the emulation routines re-create them anyways, zero exception field */
	andi.l		#0x00ff00ff,USER_FPSR(a6) /*  zero exception field */

	fmove.l		#0x0,fpcr		/*  zero current control regs */
	fmove.l		#0x0,fpsr

/*  Opclass two w/ memory-to-fpn operation will have an incorrect extended */
/*  precision format if the src format was single or double and the */
/*  source data type was an INF, NAN, DENORM, or UNNORM */
	lea		FP_SRC(a6),a0		/*  pass ptr to input */
	bsr.l		fix_skewed_ops

/*  we don't know whether the src operand or the dst operand (or both) is the */
/*  UNNORM or DENORM. call the function that tags the operand type. if the */
/*  input is an UNNORM, then convert it to a NORM, DENORM, or ZERO. */
	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		set_tag_x		/*  tag the operand type */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		fu_op2			/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM,DENORM,or ZERO */

fu_op2:
	move.b		d0,STAG(a6)		/*  save src optype tag */

	bfextu		EXC_CMDREG(a6){6:3},d0 /*  dyadic; load dst reg */

/*  bit five of the fp extension word separates the monadic and dyadic operations */
/*  at this point */
	btst		#0x5,1+EXC_CMDREG(a6)	/*  is operation monadic or dyadic? */
	beq.b		fu_extract		/*  monadic */
	cmpi.b		#0x3a,1+EXC_CMDREG(a6)	/*  is operation an ftst? */
	beq.b		fu_extract		/*  yes, so it's monadic, too */

	bsr.l		load_fpn2		/*  load dst into FP_DST */

	lea		FP_DST(a6),a0		/*  pass: ptr to dst op */
	bsr.l		set_tag_x		/*  tag the operand type */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		fu_op2_done		/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM,DENORM,or ZERO */
fu_op2_done:
	move.b		d0,DTAG(a6)		/*  save dst optype tag */

fu_extract:
	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  fetch rnd mode/prec */

	bfextu		1+EXC_CMDREG(a6){1:7},d1 /*  extract extension */

	lea		FP_SRC(a6),a0
	lea		FP_DST(a6),a1

	move.l		(tbl_unsupp.l,pc,d1.l*4),d1 /*  fetch routine addr */
	jsr		(tbl_unsupp.l,pc,d1.l*1)

/*
 * Exceptions in order of precedence:
 * BSUN	: none
 * SNAN	: all dyadic ops
 * OPERR: fsqrt(-NORM)
 * OVFL	: all except ftst,fcmp
 * UNFL	: all except ftst,fcmp
 * DZ	: fdiv
 * INEX2: all except ftst,fcmp
 * INEX1: none (packed doesn't go through here)
 */

/*  we determine the highest priority exception(if any) set by the */
/*  emulation routine that has also been enabled by the user. */
	move.b		FPCR_ENABLE(a6),d0	/*  fetch exceptions set */
	bne.b		fu_in_ena		/*  some are enabled */

fu_in_cont:
/*  fcmp and ftst do not store any result. */
	move.b		1+EXC_CMDREG(a6),d0	/*  fetch extension */
	andi.b		#0x38,d0		/*  extract bits 3-5 */
	cmpi.b		#0x38,d0		/*  is instr fcmp or ftst? */
	beq.b		fu_in_exit		/*  yes */

	bfextu		EXC_CMDREG(a6){6:3},d0 /*  dyadic; load dst reg */
	bsr.l		store_fpreg		/*  store the result */

fu_in_exit:

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	bra.l		_fpsp_done

fu_in_ena:
	and.b		FPSR_EXCEPT(a6),d0	/*  keep only ones enabled */
	bfffo		d0{24:8},d0		/*  find highest priority exception */
	bne.b		fu_in_exc		/*  there is at least one set */

/*
 * No exceptions occurred that were also enabled. Now:
 *
 *	if (OVFL ## ovfl_disabled ## inexact_enabled) {
 *	    branch to _real_inex() (even if the result was exact!);
 *	} else {
 *	    save the result in the proper fp reg (unless the op is fcmp or ftst);
 *	    return;
 *	}
 */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  was overflow set? */
	beq.b		fu_in_cont		/*  no */

fu_in_ovflchk:
	btst		#inex2_bit,FPCR_ENABLE(a6) /*  was inexact enabled? */
	beq.b		fu_in_cont		/*  no */
	bra.w		fu_in_exc_ovfl		/*  go insert overflow frame */

/*
 * An exception occurred and that exception was enabled:
 *
 *	shift enabled exception field into lo byte of d0;
 *	if (((INEX2 || INEX1) ## inex_enabled ## OVFL ## ovfl_disabled) ||
 *	    ((INEX2 || INEX1) ## inex_enabled ## UNFL ## unfl_disabled)) {
 *		 *
 *		 * this is the case where we must call _real_inex() now or else
 *		 * there will be no other way to pass it the exceptional operand
 *		 *
 *		call _real_inex();
 *	} else {
 *		restore exc state (SNAN||OPERR||OVFL||UNFL||DZ||INEX) into the FPU;
 *	}
 */
fu_in_exc:
	subi.l		#24,d0			/*  fix offset to be 0-8 */
	cmpi.b		#0x6,d0		/*  is exception INEX? (6) */
	bne.b		fu_in_exc_exit		/*  no */

/*  the enabled exception was inexact */
	btst		#unfl_bit,FPSR_EXCEPT(a6) /*  did disabled underflow occur? */
	bne.w		fu_in_exc_unfl		/*  yes */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  did disabled overflow occur? */
	bne.w		fu_in_exc_ovfl		/*  yes */

/*  here, we insert the correct fsave status value into the fsave frame for the */
/*  corresponding exception. the operand in the fsave frame should be the original */
/*  src operand. */
fu_in_exc_exit:
	move.l		d0,-(sp)		/*  save d0 */
	bsr.l		funimp_skew		/*  skew sgl or dbl inputs */
	move.l		(sp)+,d0		/*  restore d0 */

	move.w		(tbl_except.b,pc,d0.w*2),2+FP_SRC(a6) /*  create exc status */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  restore src op */

	unlk		a6

	bra.l		_fpsp_done

tbl_except:
	.dc.w		0xe000,0xe006,0xe004,0xe005
	.dc.w		0xe003,0xe002,0xe001,0xe001

fu_in_exc_unfl:
	move.w		#0x4,d0
	bra.b		fu_in_exc_exit
fu_in_exc_ovfl:
	move.w		#0x03,d0
	bra.b		fu_in_exc_exit

/*  If the input operand to this operation was opclass two and a single */
/*  or double precision denorm, inf, or nan, the operand needs to be */
/*  "corrected" in order to have the proper equivalent extended precision */
/*  number. */
	.globl		fix_skewed_ops
fix_skewed_ops:
	bfextu		EXC_CMDREG(a6){0:6},d0 /*  extract opclass,src fmt */
	cmpi.b		#0x11,d0		/*  is class = 2 # fmt = sgl? */
	beq.b		fso_sgl			/*  yes */
	cmpi.b		#0x15,d0		/*  is class = 2 # fmt = dbl? */
	beq.b		fso_dbl			/*  yes */
	rts					/*  no */

fso_sgl:
	move.w		LOCAL_EX.w(a0),d0	/*  fetch src exponent */
	andi.w		#0x7fff,d0		/*  strip sign */
	cmpi.w		#0x3f80,d0		/*  is |exp| == $3f80? */
	beq.b		fso_sgl_dnrm_zero	/*  yes */
	cmpi.w		#0x407f,d0		/*  no; is |exp| == $407f? */
	beq.b		fso_infnan		/*  yes */
	rts					/*  no */

fso_sgl_dnrm_zero:
	andi.l		#0x7fffffff,LOCAL_HI(a0) /*  clear j-bit */
	beq.b		fso_zero		/*  it's a skewed zero */
fso_sgl_dnrm:
/*  here, we count on norm not to alter a0... */
	bsr.l		norm			/*  normalize mantissa */
	neg.w		d0			/*  -shft amt */
	addi.w		#0x3f81,d0		/*  adjust new exponent */
	andi.w		#0x8000,LOCAL_EX.w(a0)	/*  clear old exponent */
	or.w		d0,LOCAL_EX.w(a0)	/*  insert new exponent */
	rts

fso_zero:
	andi.w		#0x8000,LOCAL_EX.w(a0)	/*  clear bogus exponent */
	rts

fso_infnan:
	andi.b		#0x7f,LOCAL_HI(a0)	/*  clear j-bit */
	ori.w		#0x7fff,LOCAL_EX.w(a0)	/*  make exponent = $7fff */
	rts

fso_dbl:
	move.w		LOCAL_EX.w(a0),d0	/*  fetch src exponent */
	andi.w		#0x7fff,d0		/*  strip sign */
	cmpi.w		#0x3c00,d0		/*  is |exp| == $3c00? */
	beq.b		fso_dbl_dnrm_zero	/*  yes */
	cmpi.w		#0x43ff,d0		/*  no; is |exp| == $43ff? */
	beq.b		fso_infnan		/*  yes */
	rts					/*  no */

fso_dbl_dnrm_zero:
	andi.l		#0x7fffffff,LOCAL_HI(a0) /*  clear j-bit */
	bne.b		fso_dbl_dnrm		/*  it's a skewed denorm */
	tst.l		LOCAL_LO(a0)		/*  is it a zero? */
	beq.b		fso_zero		/*  yes */
fso_dbl_dnrm:
/*  here, we count on norm not to alter a0... */
	bsr.l		norm			/*  normalize mantissa */
	neg.w		d0			/*  -shft amt */
	addi.w		#0x3c01,d0		/*  adjust new exponent */
	andi.w		#0x8000,LOCAL_EX.w(a0)	/*  clear old exponent */
	or.w		d0,LOCAL_EX.w(a0)	/*  insert new exponent */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  fmove out took an unimplemented data type exception. */
/*  the src operand is in FP_SRC. Call _fout() to write out the result and */
/*  to determine which exceptions, if any, to take. */
fu_out:

/*  Separate packed move outs from the UNNORM and DENORM move outs. */
	bfextu		EXC_CMDREG(a6){3:3},d0
	cmpi.b		#0x3,d0
	beq.w		fu_out_pack
	cmpi.b		#0x7,d0
	beq.w		fu_out_pack


/*  I'm not sure at this point what FPSR bits are valid for this instruction. */
/*  so, since the emulation routines re-create them anyways, zero exception field. */
/*  fmove out doesn't affect ccodes. */
	and.l		#0xffff00ff,USER_FPSR(a6) /*  zero exception field */

	fmove.l		#0x0,fpcr		/*  zero current control regs */
	fmove.l		#0x0,fpsr

/*  the src can ONLY be a DENORM or an UNNORM! so, don't make any big subroutine */
/*  call here. just figure out what it is... */
	move.w		FP_SRC_EX(a6),d0	/*  get exponent */
	andi.w		#0x7fff,d0		/*  strip sign */
	beq.b		fu_out_denorm		/*  it's a DENORM */

	lea		FP_SRC(a6),a0
	bsr.l		unnorm_fix		/*  yes; fix it */

	move.b		d0,STAG(a6)

	bra.b		fu_out_cont
fu_out_denorm:
	move.b		#DENORM,STAG(a6)
fu_out_cont:

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  fetch rnd mode/prec */

	lea		FP_SRC(a6),a0		/*  pass ptr to src operand */

	move.l		(a6),EXC_A6(a6)	/*  in case a6 changes */
	bsr.l		fout			/*  call fmove out routine */

/*  Exceptions in order of precedence: */
/* 	BSUN	: none */
/* 	SNAN	: none */
/* 	OPERR	: fmove.{b,w,l} out of large UNNORM */
/* 	OVFL	: fmove.{s,d} */
/* 	UNFL	: fmove.{s,d,x} */
/* 	DZ	: none */
/* 	INEX2	: all */
/* 	INEX1	: none (packed doesn't travel through here) */

/*  determine the highest priority exception(if any) set by the */
/*  emulation routine that has also been enabled by the user. */
	move.b		FPCR_ENABLE(a6),d0	/*  fetch exceptions enabled */
	bne.w		fu_out_ena		/*  some are enabled */

fu_out_done:

	move.l		EXC_A6(a6),(a6)	/*  in case a6 changed */

/*  on extended precision opclass three instructions using pre-decrement or */
/*  post-increment addressing mode, the address register is not updated. is the */
/*  address register was the stack pointer used from user mode, then let's update */
/*  it here. if it was used from supervisor mode, then we have to handle this */
/*  as a special case. */
	btst		#0x5,EXC_SR(a6)
	bne.b		fu_out_done_s

	move.l		EXC_A7(a6),a0		/*  restore a7 */
	move.l		a0,usp

fu_out_done_cont:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	btst		#0x7,(sp)		/*  is trace on? */
	bne.b		fu_out_trace		/*  yes */

	bra.l		_fpsp_done

/*  is the ea mode pre-decrement of the stack pointer from supervisor mode? */
/*  ("fmove.x fpm,-(a7)") if so, */
fu_out_done_s:
	cmpi.b		#mda7_flg,SPCOND_FLG(a6)
	bne.b		fu_out_done_cont

/*  the extended precision result is still in fp0. but, we need to save it */
/*  somewhere on the stack until we can copy it to its final resting place. */
/*  here, we're counting on the top of the stack to be the old place-holders */
/*  for fp0/fp1 which have already been restored. that way, we can write */
/*  over those destinations with the shifted stack frame. */
	fmovem.x		fp0,FP_SRC(a6)	/*  put answer on stack */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.l		(a6),a6		/*  restore frame pointer */

	move.l		LOCAL_SIZE+EXC_SR(sp),LOCAL_SIZE+EXC_SR-0xc(sp)
	move.l		LOCAL_SIZE+2+EXC_PC(sp),LOCAL_SIZE+2+EXC_PC-0xc(sp)

/*  now, copy the result to the proper place on the stack */
	move.l		LOCAL_SIZE+FP_SRC_EX(sp),LOCAL_SIZE+EXC_SR+0x0(sp)
	move.l		LOCAL_SIZE+FP_SRC_HI(sp),LOCAL_SIZE+EXC_SR+0x4(sp)
	move.l		LOCAL_SIZE+FP_SRC_LO(sp),LOCAL_SIZE+EXC_SR+0x8(sp)

	add.l		#LOCAL_SIZE-0x8,sp

	btst		#0x7,(sp)
	bne.b		fu_out_trace

	bra.l		_fpsp_done

fu_out_ena:
	and.b		FPSR_EXCEPT(a6),d0	/*  keep only ones enabled */
	bfffo		d0{24:8},d0		/*  find highest priority exception */
	bne.b		fu_out_exc		/*  there is at least one set */

/*  no exceptions were set. */
/*  if a disabled overflow occurred and inexact was enabled but the result */
/*  was exact, then a branch to _real_inex() is made. */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  was overflow set? */
	beq.w		fu_out_done		/*  no */

fu_out_ovflchk:
	btst		#inex2_bit,FPCR_ENABLE(a6) /*  was inexact enabled? */
	beq.w		fu_out_done		/*  no */
	bra.w		fu_inex			/*  yes */

/*
 * The fp move out that took the "Unimplemented Data Type" exception was
 * being traced. Since the stack frames are similar, get the "current" PC
 * from FPIAR and put it in the trace stack frame then jump to _real_trace().
 *
 *                UNSUPP FRAME             TRACE FRAME
 *              *****************       *****************
 *              *      EA       *       *    Current    *
 *              *               *       *      PC       *
 *              *****************       *****************
 *              * 0x3 *  0x0dc  *       * 0x2 *  0x024  *
 *              *****************       *****************
 *              *     Next      *       *     Next      *
 *              *      PC       *       *      PC       *
 *              *****************       *****************
 *              *      SR       *       *      SR       *
 *              *****************       *****************
 */
fu_out_trace:
	move.w		#0x2024,0x6(sp)
	fmove.l		fpiar,0x8(sp)
	bra.l		_real_trace

/*  an exception occurred and that exception was enabled. */
fu_out_exc:
	subi.l		#24,d0			/*  fix offset to be 0-8 */

/*  we don't mess with the existing fsave frame. just re-insert it and */
/*  jump to the "_real_{}()" handler... */
	move.w		(tbl_fu_out.b,pc,d0.w*2),d0
	jmp		(tbl_fu_out.b,pc,d0.w*1)

	/* swbeg		#0x8 */
	.dc.w 0x4afc,8
tbl_fu_out:
	.dc.w		tbl_fu_out-tbl_fu_out	/*  BSUN can't happen */
	.dc.w		tbl_fu_out-tbl_fu_out	/*  SNAN can't happen */
	.dc.w		fu_operr-tbl_fu_out	/*  OPERR */
	.dc.w		fu_ovfl-tbl_fu_out	/*  OVFL */
	.dc.w		fu_unfl-tbl_fu_out	/*  UNFL */
	.dc.w		tbl_fu_out-tbl_fu_out	/*  DZ can't happen */
	.dc.w		fu_inex-tbl_fu_out	/*  INEX2 */
	.dc.w		tbl_fu_out-tbl_fu_out	/*  INEX1 won't make it here */

/*  for snan,operr,ovfl,unfl, src op is still in FP_SRC so just */
/*  frestore it. */
fu_snan:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30d8,EXC_VOFF(a6)	/*  vector offset = 0xd8 */
	move.w		#0xe006,2+FP_SRC(a6)

	frestore	FP_SRC(a6)

	unlk		a6


	bra.l		_real_snan

fu_operr:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30d0,EXC_VOFF(a6)	/*  vector offset = 0xd0 */
	move.w		#0xe004,2+FP_SRC(a6)

	frestore	FP_SRC(a6)

	unlk		a6


	bra.l		_real_operr

fu_ovfl:
	fmovem.x		fp1,FP_SRC(a6)	/*  save EXOP to the stack */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30d4,EXC_VOFF(a6)	/*  vector offset = 0xd4 */
	move.w		#0xe005,2+FP_SRC(a6)

	frestore	FP_SRC(a6)		/*  restore EXOP */

	unlk		a6

	bra.l		_real_ovfl

/*  underflow can happen for extended precision. extended precision opclass */
/*  three instruction exceptions don't update the stack pointer. so, if the */
/*  exception occurred from user mode, then simply update a7 and exit normally. */
/*  if the exception occurred from supervisor mode, check if */
fu_unfl:
	move.l		EXC_A6(a6),(a6)	/*  restore a6 */

	btst		#0x5,EXC_SR(a6)
	bne.w		fu_unfl_s

	move.l		EXC_A7(a6),a0		/*  restore a7 whether we need */
	move.l		a0,usp		/*  to or not... */

fu_unfl_cont:
	fmovem.x		fp1,FP_SRC(a6)	/*  save EXOP to the stack */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30cc,EXC_VOFF(a6)	/*  vector offset = 0xcc */
	move.w		#0xe003,2+FP_SRC(a6)

	frestore	FP_SRC(a6)		/*  restore EXOP */

	unlk		a6

	bra.l		_real_unfl

fu_unfl_s:
	cmpi.b		#mda7_flg,SPCOND_FLG(a6) /*  was the <ea> mode -(sp)? */
	bne.b		fu_unfl_cont

/*  the extended precision result is still in fp0. but, we need to save it */
/*  somewhere on the stack until we can copy it to its final resting place */
/*  (where the exc frame is currently). make sure it's not at the top of the */
/*  frame or it will get overwritten when the exc stack frame is shifted "down". */
	fmovem.x		fp0,FP_SRC(a6)	/*  put answer on stack */
	fmovem.x		fp1,FP_DST(a6)	/*  put EXOP on stack */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30cc,EXC_VOFF(a6)	/*  vector offset = 0xcc */
	move.w		#0xe003,2+FP_DST(a6)

	frestore	FP_DST(a6)		/*  restore EXOP */

	move.l		(a6),a6		/*  restore frame pointer */

	move.l		LOCAL_SIZE+EXC_SR(sp),LOCAL_SIZE+EXC_SR-0xc(sp)
	move.l		LOCAL_SIZE+2+EXC_PC(sp),LOCAL_SIZE+2+EXC_PC-0xc(sp)
	move.l		LOCAL_SIZE+EXC_EA(sp),LOCAL_SIZE+EXC_EA-0xc(sp)

/*  now, copy the result to the proper place on the stack */
	move.l		LOCAL_SIZE+FP_SRC_EX(sp),LOCAL_SIZE+EXC_SR+0x0(sp)
	move.l		LOCAL_SIZE+FP_SRC_HI(sp),LOCAL_SIZE+EXC_SR+0x4(sp)
	move.l		LOCAL_SIZE+FP_SRC_LO(sp),LOCAL_SIZE+EXC_SR+0x8(sp)

	add.l		#LOCAL_SIZE-0x8,sp

	bra.l		_real_unfl

/*  fmove in and out enter here. */
fu_inex:
	fmovem.x		fp1,FP_SRC(a6)	/*  save EXOP to the stack */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30c4,EXC_VOFF(a6)	/*  vector offset = 0xc4 */
	move.w		#0xe001,2+FP_SRC(a6)

	frestore	FP_SRC(a6)		/*  restore EXOP */

	unlk		a6


	bra.l		_real_inex

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fu_in_pack:


/*  I'm not sure at this point what FPSR bits are valid for this instruction. */
/*  so, since the emulation routines re-create them anyways, zero exception field */
	andi.l		#0x0ff00ff,USER_FPSR(a6) /*  zero exception field */

	fmove.l		#0x0,fpcr		/*  zero current control regs */
	fmove.l		#0x0,fpsr

	bsr.l		get_packed		/*  fetch packed src operand */

	lea		FP_SRC(a6),a0		/*  pass ptr to src */
	bsr.l		set_tag_x		/*  set src optype tag */

	move.b		d0,STAG(a6)		/*  save src optype tag */

	bfextu		EXC_CMDREG(a6){6:3},d0 /*  dyadic; load dst reg */

/*  bit five of the fp extension word separates the monadic and dyadic operations */
/*  at this point */
	btst		#0x5,1+EXC_CMDREG(a6)	/*  is operation monadic or dyadic? */
	beq.b		fu_extract_p		/*  monadic */
	cmpi.b		#0x3a,1+EXC_CMDREG(a6)	/*  is operation an ftst? */
	beq.b		fu_extract_p		/*  yes, so it's monadic, too */

	bsr.l		load_fpn2		/*  load dst into FP_DST */

	lea		FP_DST(a6),a0		/*  pass: ptr to dst op */
	bsr.l		set_tag_x		/*  tag the operand type */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		fu_op2_done_p		/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM,DENORM,or ZERO */
fu_op2_done_p:
	move.b		d0,DTAG(a6)		/*  save dst optype tag */

fu_extract_p:
	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  fetch rnd mode/prec */

	bfextu		1+EXC_CMDREG(a6){1:7},d1 /*  extract extension */

	lea		FP_SRC(a6),a0
	lea		FP_DST(a6),a1

	move.l		(tbl_unsupp.l,pc,d1.l*4),d1 /*  fetch routine addr */
	jsr		(tbl_unsupp.l,pc,d1.l*1)

/*
 * Exceptions in order of precedence:
 *	BSUN	: none
 *	SNAN	: all dyadic ops
 *	OPERR	: fsqrt(-NORM)
 *	OVFL	: all except ftst,fcmp
 *	UNFL	: all except ftst,fcmp
 *	DZ	: fdiv
 *	INEX2	: all except ftst,fcmp
 *	INEX1	: all
 */

/*  we determine the highest priority exception(if any) set by the */
/*  emulation routine that has also been enabled by the user. */
	move.b		FPCR_ENABLE(a6),d0	/*  fetch exceptions enabled */
	bne.w		fu_in_ena_p		/*  some are enabled */

fu_in_cont_p:
/*  fcmp and ftst do not store any result. */
	move.b		1+EXC_CMDREG(a6),d0	/*  fetch extension */
	andi.b		#0x38,d0		/*  extract bits 3-5 */
	cmpi.b		#0x38,d0		/*  is instr fcmp or ftst? */
	beq.b		fu_in_exit_p		/*  yes */

	bfextu		EXC_CMDREG(a6){6:3},d0 /*  dyadic; load dst reg */
	bsr.l		store_fpreg		/*  store the result */

fu_in_exit_p:

	btst		#0x5,EXC_SR(a6)	/*  user or supervisor? */
	bne.w		fu_in_exit_s_p		/*  supervisor */

	move.l		EXC_A7(a6),a0		/*  update user a7 */
	move.l		a0,usp

fu_in_exit_cont_p:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6			/*  unravel stack frame */

	btst		#0x7,(sp)		/*  is trace on? */
	bne.w		fu_trace_p		/*  yes */

	bra.l		_fpsp_done		/*  exit to os */

/*  the exception occurred in supervisor mode. check to see if the */
/*  addressing mode was (a7)+. if so, we'll need to shift the */
/*  stack frame "up". */
fu_in_exit_s_p:
	btst		#mia7_bit,SPCOND_FLG(a6) /*  was ea mode (a7)+ */
	beq.b		fu_in_exit_cont_p	/*  no */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6			/*  unravel stack frame */

/*  shift the stack frame "up". we don't really care about the <ea> field. */
	move.l		0x4(sp),0x10(sp)
	move.l		0x0.w(sp),0xc(sp)
	add.l		#0xc,sp

	btst		#0x7,(sp)		/*  is trace on? */
	bne.w		fu_trace_p		/*  yes */

	bra.l		_fpsp_done		/*  exit to os */

fu_in_ena_p:
	and.b		FPSR_EXCEPT(a6),d0	/*  keep only ones enabled # set */
	bfffo		d0{24:8},d0		/*  find highest priority exception */
	bne.b		fu_in_exc_p		/*  at least one was set */

/*
 * No exceptions occurred that were also enabled. Now:
 *
 *	if (OVFL ## ovfl_disabled ## inexact_enabled) {
 *	    branch to _real_inex() (even if the result was exact!);
 *	} else {
 *	    save the result in the proper fp reg (unless the op is fcmp or ftst);
 *	    return;
 *	}
 */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  was overflow set? */
	beq.w		fu_in_cont_p		/*  no */

fu_in_ovflchk_p:
	btst		#inex2_bit,FPCR_ENABLE(a6) /*  was inexact enabled? */
	beq.w		fu_in_cont_p		/*  no */
	bra.w		fu_in_exc_ovfl_p	/*  do _real_inex() now */

/*
 * An exception occurred and that exception was enabled:
 *
 *	shift enabled exception field into lo byte of d0;
 *	if (((INEX2 || INEX1) ## inex_enabled ## OVFL ## ovfl_disabled) ||
 *	    ((INEX2 || INEX1) ## inex_enabled ## UNFL ## unfl_disabled)) {
 *		 *
 *		 * this is the case where we must call _real_inex() now or else
 *		 * there will be no other way to pass it the exceptional operand
 *		 *
 *		call _real_inex();
 *	} else {
 *		restore exc state (SNAN||OPERR||OVFL||UNFL||DZ||INEX) into the FPU;
 *	}
 */
fu_in_exc_p:
	subi.l		#24,d0			/*  fix offset to be 0-8 */
	cmpi.b		#0x6,d0		/*  is exception INEX? (6 or 7) */
	blt.b		fu_in_exc_exit_p	/*  no */

/*  the enabled exception was inexact */
	btst		#unfl_bit,FPSR_EXCEPT(a6) /*  did disabled underflow occur? */
	bne.w		fu_in_exc_unfl_p	/*  yes */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  did disabled overflow occur? */
	bne.w		fu_in_exc_ovfl_p	/*  yes */

/*  here, we insert the correct fsave status value into the fsave frame for the */
/*  corresponding exception. the operand in the fsave frame should be the original */
/*  src operand. */
/*  as a reminder for future predicted pain and agony, we are passing in fsave the */
/*  "non-skewed" operand for cases of sgl and dbl src INFs,NANs, and DENORMs. */
/*  this is INCORRECT for enabled SNAN which would give to the user the skewed SNAN!!! */
fu_in_exc_exit_p:
	btst		#0x5,EXC_SR(a6)	/*  user or supervisor? */
	bne.w		fu_in_exc_exit_s_p	/*  supervisor */

	move.l		EXC_A7(a6),a0		/*  update user a7 */
	move.l		a0,usp

fu_in_exc_exit_cont_p:
	move.w		(tbl_except_p.b,pc,d0.w*2),2+FP_SRC(a6)

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  restore src op */

	unlk		a6

	btst		#0x7,(sp)		/*  is trace enabled? */
	bne.w		fu_trace_p		/*  yes */

	bra.l		_fpsp_done

tbl_except_p:
	.dc.w		0xe000,0xe006,0xe004,0xe005
	.dc.w		0xe003,0xe002,0xe001,0xe001

fu_in_exc_ovfl_p:
	move.w		#0x3,d0
	bra.w		fu_in_exc_exit_p

fu_in_exc_unfl_p:
	move.w		#0x4,d0
	bra.w		fu_in_exc_exit_p

fu_in_exc_exit_s_p:
	btst		#mia7_bit,SPCOND_FLG(a6)
	beq.b		fu_in_exc_exit_cont_p

	move.w		(tbl_except_p.b,pc,d0.w*2),2+FP_SRC(a6)

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  restore src op */

	unlk		a6			/*  unravel stack frame */

/*  shift stack frame "up". who cares about <ea> field. */
	move.l		0x4(sp),0x10(sp)
	move.l		0x0.w(sp),0xc(sp)
	add.l		#0xc,sp

	btst		#0x7,(sp)		/*  is trace on? */
	bne.b		fu_trace_p		/*  yes */

	bra.l		_fpsp_done		/*  exit to os */

/*
 * The opclass two PACKED instruction that took an "Unimplemented Data Type"
 * exception was being traced. Make the "current" PC the FPIAR and put it in the
 * trace stack frame then jump to _real_trace().
 *
 *                UNSUPP FRAME             TRACE FRAME
 *              *****************       *****************
 *              *      EA       *       *    Current    *
 *              *               *       *      PC       *
 *              *****************       *****************
 *              * 0x2 * 0x0dc   *       * 0x2 *  0x024  *
 *              *****************       *****************
 *              *     Next      *       *     Next      *
 *              *      PC       *       *      PC       *
 *              *****************       *****************
 *              *      SR       *       *      SR       *
 *              *****************       *****************
 */
fu_trace_p:
	move.w		#0x2024,0x6(sp)
	fmove.l		fpiar,0x8(sp)

	bra.l		_real_trace

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fu_out_pack:


/*  I'm not sure at this point what FPSR bits are valid for this instruction. */
/*  so, since the emulation routines re-create them anyways, zero exception field. */
/*  fmove out doesn't affect ccodes. */
	and.l		#0xffff00ff,USER_FPSR(a6) /*  zero exception field */

	fmove.l		#0x0,fpcr		/*  zero current control regs */
	fmove.l		#0x0,fpsr

	bfextu		EXC_CMDREG(a6){6:3},d0
	bsr.l		load_fpn1

/*  unlike other opclass 3, unimplemented data type exceptions, packed must be */
/*  able to detect all operand types. */
	lea		FP_SRC(a6),a0
	bsr.l		set_tag_x		/*  tag the operand type */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		fu_op2_p		/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM,DENORM,or ZERO */

fu_op2_p:
	move.b		d0,STAG(a6)		/*  save src optype tag */

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  fetch rnd mode/prec */

	lea		FP_SRC(a6),a0		/*  pass ptr to src operand */

	move.l		(a6),EXC_A6(a6)	/*  in case a6 changes */
	bsr.l		fout			/*  call fmove out routine */

/*  Exceptions in order of precedence: */
/* 	BSUN	: no */
/* 	SNAN	: yes */
/* 	OPERR	: if ((k_factor > +17) || (dec. exp exceeds 3 digits)) */
/* 	OVFL	: no */
/* 	UNFL	: no */
/* 	DZ	: no */
/* 	INEX2	: yes */
/* 	INEX1	: no */

/*  determine the highest priority exception(if any) set by the */
/*  emulation routine that has also been enabled by the user. */
	move.b		FPCR_ENABLE(a6),d0	/*  fetch exceptions enabled */
	bne.w		fu_out_ena_p		/*  some are enabled */

fu_out_exit_p:
	move.l		EXC_A6(a6),(a6)	/*  restore a6 */

	btst		#0x5,EXC_SR(a6)	/*  user or supervisor? */
	bne.b		fu_out_exit_s_p		/*  supervisor */

	move.l		EXC_A7(a6),a0		/*  update user a7 */
	move.l		a0,usp

fu_out_exit_cont_p:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6			/*  unravel stack frame */

	btst		#0x7,(sp)		/*  is trace on? */
	bne.w		fu_trace_p		/*  yes */

	bra.l		_fpsp_done		/*  exit to os */

/*  the exception occurred in supervisor mode. check to see if the */
/*  addressing mode was -(a7). if so, we'll need to shift the */
/*  stack frame "down". */
fu_out_exit_s_p:
	btst		#mda7_bit,SPCOND_FLG(a6) /*  was ea mode -(a7) */
	beq.b		fu_out_exit_cont_p	/*  no */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.l		(a6),a6		/*  restore frame pointer */

	move.l		LOCAL_SIZE+EXC_SR(sp),LOCAL_SIZE+EXC_SR-0xc(sp)
	move.l		LOCAL_SIZE+2+EXC_PC(sp),LOCAL_SIZE+2+EXC_PC-0xc(sp)

/*  now, copy the result to the proper place on the stack */
	move.l		LOCAL_SIZE+FP_DST_EX(sp),LOCAL_SIZE+EXC_SR+0x0(sp)
	move.l		LOCAL_SIZE+FP_DST_HI(sp),LOCAL_SIZE+EXC_SR+0x4(sp)
	move.l		LOCAL_SIZE+FP_DST_LO(sp),LOCAL_SIZE+EXC_SR+0x8(sp)

	add.l		#LOCAL_SIZE-0x8,sp

	btst		#0x7,(sp)
	bne.w		fu_trace_p

	bra.l		_fpsp_done

fu_out_ena_p:
	and.b		FPSR_EXCEPT(a6),d0	/*  keep only ones enabled */
	bfffo		d0{24:8},d0		/*  find highest priority exception */
	beq.w		fu_out_exit_p

	move.l		EXC_A6(a6),(a6)	/*  restore a6 */

/*  an exception occurred and that exception was enabled. */
/*  the only exception possible on packed move out are INEX, OPERR, and SNAN. */
fu_out_exc_p:
	cmpi.b		#0x1a,d0
	bgt.w		fu_inex_p2
	beq.w		fu_operr_p

fu_snan_p:
	btst		#0x5,EXC_SR(a6)
	bne.b		fu_snan_s_p

	move.l		EXC_A7(a6),a0
	move.l		a0,usp
	bra.w		fu_snan

fu_snan_s_p:
	cmpi.b		#mda7_flg,SPCOND_FLG(a6)
	bne.w		fu_snan

/*  the instruction was "fmove.p fpn,-(a7)" from supervisor mode. */
/*  the strategy is to move the exception frame "down" 12 bytes. then, we */
/*  can store the default result where the exception frame was. */
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30d8,EXC_VOFF(a6)	/*  vector offset = 0xd0 */
	move.w		#0xe006,2+FP_SRC(a6)	/*  set fsave status */

	frestore	FP_SRC(a6)		/*  restore src operand */

	move.l		(a6),a6		/*  restore frame pointer */

	move.l		LOCAL_SIZE+EXC_SR(sp),LOCAL_SIZE+EXC_SR-0xc(sp)
	move.l		LOCAL_SIZE+2+EXC_PC(sp),LOCAL_SIZE+2+EXC_PC-0xc(sp)
	move.l		LOCAL_SIZE+EXC_EA(sp),LOCAL_SIZE+EXC_EA-0xc(sp)

/*  now, we copy the default result to its proper location */
	move.l		LOCAL_SIZE+FP_DST_EX(sp),LOCAL_SIZE+0x4(sp)
	move.l		LOCAL_SIZE+FP_DST_HI(sp),LOCAL_SIZE+0x8(sp)
	move.l		LOCAL_SIZE+FP_DST_LO(sp),LOCAL_SIZE+0xc(sp)

	add.l		#LOCAL_SIZE-0x8,sp


	bra.l		_real_snan

fu_operr_p:
	btst		#0x5,EXC_SR(a6)
	bne.w		fu_operr_p_s

	move.l		EXC_A7(a6),a0
	move.l		a0,usp
	bra.w		fu_operr

fu_operr_p_s:
	cmpi.b		#mda7_flg,SPCOND_FLG(a6)
	bne.w		fu_operr

/*  the instruction was "fmove.p fpn,-(a7)" from supervisor mode. */
/*  the strategy is to move the exception frame "down" 12 bytes. then, we */
/*  can store the default result where the exception frame was. */
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30d0,EXC_VOFF(a6)	/*  vector offset = 0xd0 */
	move.w		#0xe004,2+FP_SRC(a6)	/*  set fsave status */

	frestore	FP_SRC(a6)		/*  restore src operand */

	move.l		(a6),a6		/*  restore frame pointer */

	move.l		LOCAL_SIZE+EXC_SR(sp),LOCAL_SIZE+EXC_SR-0xc(sp)
	move.l		LOCAL_SIZE+2+EXC_PC(sp),LOCAL_SIZE+2+EXC_PC-0xc(sp)
	move.l		LOCAL_SIZE+EXC_EA(sp),LOCAL_SIZE+EXC_EA-0xc(sp)

/*  now, we copy the default result to its proper location */
	move.l		LOCAL_SIZE+FP_DST_EX(sp),LOCAL_SIZE+0x4(sp)
	move.l		LOCAL_SIZE+FP_DST_HI(sp),LOCAL_SIZE+0x8(sp)
	move.l		LOCAL_SIZE+FP_DST_LO(sp),LOCAL_SIZE+0xc(sp)

	add.l		#LOCAL_SIZE-0x8,sp


	bra.l		_real_operr

fu_inex_p2:
	btst		#0x5,EXC_SR(a6)
	bne.w		fu_inex_s_p2

	move.l		EXC_A7(a6),a0
	move.l		a0,usp
	bra.w		fu_inex

fu_inex_s_p2:
	cmpi.b		#mda7_flg,SPCOND_FLG(a6)
	bne.w		fu_inex

/*  the instruction was "fmove.p fpn,-(a7)" from supervisor mode. */
/*  the strategy is to move the exception frame "down" 12 bytes. then, we */
/*  can store the default result where the exception frame was. */
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0/fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.w		#0x30c4,EXC_VOFF(a6)	/*  vector offset = 0xc4 */
	move.w		#0xe001,2+FP_SRC(a6)	/*  set fsave status */

	frestore	FP_SRC(a6)		/*  restore src operand */

	move.l		(a6),a6		/*  restore frame pointer */

	move.l		LOCAL_SIZE+EXC_SR(sp),LOCAL_SIZE+EXC_SR-0xc(sp)
	move.l		LOCAL_SIZE+2+EXC_PC(sp),LOCAL_SIZE+2+EXC_PC-0xc(sp)
	move.l		LOCAL_SIZE+EXC_EA(sp),LOCAL_SIZE+EXC_EA-0xc(sp)

/*  now, we copy the default result to its proper location */
	move.l		LOCAL_SIZE+FP_DST_EX(sp),LOCAL_SIZE+0x4(sp)
	move.l		LOCAL_SIZE+FP_DST_HI(sp),LOCAL_SIZE+0x8(sp)
	move.l		LOCAL_SIZE+FP_DST_LO(sp),LOCAL_SIZE+0xc(sp)

	add.l		#LOCAL_SIZE-0x8,sp


	bra.l		_real_inex

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * if we're stuffing a source operand back into an fsave frame then we
 * have to make sure that for single or double source operands that the
 * format stuffed is as weird as the hardware usually makes it.
 */
	.globl		funimp_skew
funimp_skew:
	bfextu		EXC_EXTWORD(a6){3:3},d0 /*  extract src specifier */
	cmpi.b		#0x1,d0		/*  was src sgl? */
	beq.b		funimp_skew_sgl		/*  yes */
	cmpi.b		#0x5,d0		/*  was src dbl? */
	beq.b		funimp_skew_dbl		/*  yes */
	rts

funimp_skew_sgl:
	move.w		FP_SRC_EX(a6),d0	/*  fetch DENORM exponent */
	andi.w		#0x7fff,d0		/*  strip sign */
	beq.b		funimp_skew_sgl_not
	cmpi.w		#0x3f80,d0
	bgt.b		funimp_skew_sgl_not
	neg.w		d0			/*  make exponent negative */
	addi.w		#0x3f81,d0		/*  find amt to shift */
	move.l		FP_SRC_HI(a6),d1	/*  fetch DENORM hi(man) */
	lsr.l		d0,d1			/*  shift it */
	bset		#31,d1			/*  set j-bit */
	move.l		d1,FP_SRC_HI(a6)	/*  insert new hi(man) */
	andi.w		#0x8000,FP_SRC_EX(a6)	/*  clear old exponent */
	ori.w		#0x3f80,FP_SRC_EX(a6)	/*  insert new "skewed" exponent */
funimp_skew_sgl_not:
	rts

funimp_skew_dbl:
	move.w		FP_SRC_EX(a6),d0	/*  fetch DENORM exponent */
	andi.w		#0x7fff,d0		/*  strip sign */
	beq.b		funimp_skew_dbl_not
	cmpi.w		#0x3c00,d0
	bgt.b		funimp_skew_dbl_not

	tst.b		FP_SRC_EX(a6)		/*  make "internal format" */
	smi.b		0x2+FP_SRC(a6)
	move.w		d0,FP_SRC_EX(a6)	/*  insert exponent with cleared sign */
	clr.l		d0			/*  clear g,r,s */
	lea		FP_SRC(a6),a0		/*  pass ptr to src op */
	move.w		#0x3c01,d1		/*  pass denorm threshold */
	bsr.l		dnrm_lp			/*  denorm it */
	move.w		#0x3c00,d0		/*  new exponent */
	tst.b		0x2+FP_SRC(a6)		/*  is sign set? */
	beq.b		fss_dbl_denorm_done	/*  no */
	bset		#15,d0			/*  set sign */
fss_dbl_denorm_done:
	bset		#0x7,FP_SRC_HI(a6)	/*  set j-bit */
	move.w		d0,FP_SRC_EX(a6)	/*  insert new exponent */
funimp_skew_dbl_not:
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		_mem_write2
_mem_write2:
	btst		#0x5,EXC_SR(a6)
	beq.l		_dmem_write
	move.l		0x0.w(a0),FP_DST_EX(a6)
	move.l		0x4(a0),FP_DST_HI(a6)
	move.l		0x8(a0),FP_DST_LO(a6)
	clr.l		d1
	rts

