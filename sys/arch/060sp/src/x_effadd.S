/*
 *  XDEF ****************************************************************	
 * 	_fpsp_effadd(): 060FPSP entry point for FP "Unimplemented	
 * 			effective address" exception.			
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	FP Unimplemented Effective Address exception in an operating	
 * 	system.								
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_long() - read instruction longword			
 * 	fix_skewed_ops() - adjust src operand in fsave frame		
 * 	set_tag_x() - determine optype of src/dst operands		
 * 	store_fpreg() - store opclass 0 or 2 result to FP regfile	
 * 	unnorm_fix() - change UNNORM operands to NORM or ZERO		
 * 	load_fpn2() - load dst operand from FP regfile			
 * 	tbl_unsupp - add of table of emulation routines for opclass 0,2	
 * 	decbin() - convert packed data to FP binary data		
 * 	_real_fpu_disabled() - "callout" for "FPU disabled" exception	
 * 	_real_access() - "callout" for access error exception		
 * 	_mem_read() - read extended immediate operand from memory	
 * 	_fpsp_done() - "callout" for exit *  work all done		
 * 	_real_trace() - "callout" for Trace enabled exception		
 * 	fmovem_dynamic() - emulate dynamic fmovem instruction		
 * 	fmovem_ctrl() - emulate fmovem control instruction		
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the "Unimplemented <ea>" stk frame	
 * 									
 *  OUTPUT **************************************************************	
 * 	If access error:						
 * 	- The system stack is changed to an access error stack frame	
 * 	If FPU disabled:						
 * 	- The system stack is changed to an FPU disabled stack frame	
 * 	If Trace exception enabled:					
 * 	- The system stack is changed to a Trace exception stack frame	
 * 	Else: (normal case)						
 * 	- None (correct result has been stored as appropriate)		
 * 									
 *  ALGORITHM ***********************************************************	
 * 	This exception handles 3 types of operations:			
 *  (1) FP Instructions using extended precision or packed immediate	
 *      addressing mode.							
 *  (2) The "fmovem.x" instruction w/ dynamic register specification.	
 *  (3) The "fmovem.l" instruction w/ 2 or 3 control registers.		
 * 									
 * 	For immediate data operations, the data is read in w/ a		
 *  _mem_read() "callout", converted to FP binary (if packed), and used	
 *  as the source operand to the instruction specified by the instruction	
 *  word. If no FP exception should be reported ads a result of the	
 *  emulation, then the result is stored to the destination register and	
 *  the handler exits through _fpsp_done(). If an enabled exc has been	
 *  signalled as a result of emulation, then an fsave state frame		
 *  corresponding to the FP exception type must be entered into the 060	
 *  FPU before exiting. In either the enabled or disabled cases, we	
 *  must also check if a Trace exception is pending, in which case, we	
 *  must create a Trace exception stack frame from the current exception	
 *  stack frame. If no Trace is pending, we simply exit through		
 *  _fpsp_done().								
 * 	For "fmovem.x", call the routine fmovem_dynamic() which will	
 *  decode and emulate the instruction. No FP exceptions can be pending	
 *  as a result of this operation emulation. A Trace exception can be	
 *  pending, though, which means the current stack frame must be changed	
 *  to a Trace stack frame and an exit made through _real_trace().	
 *  For the case of "fmovem.x Dn,-(a7)", where the offending instruction	
 *  was executed from supervisor mode, this handler must store the FP	
 *  register file values to the system stack by itself since		
 *  fmovem_dynamic() can't handle this. A normal exit is made through	
 *  fpsp_done().								
 * 	For "fmovem.l", fmovem_ctrl() is used to emulate the instruction.	
 *  Again, a Trace exception may be pending and an exit made through	
 *  _real_trace(). Else, a normal exit is made through _fpsp_done().	
 * 									
 * 	Before any of the above is attempted, it must be checked to	
 *  see if the FPU is disabled. Since the "Unimp <ea>" exception is taken	
 *  before the "FPU disabled" exception, but the "FPU disabled" exception	
 *  has higher priority, we check the disabled bit in the PCR. If set,	
 *  then we must create an 8 word "FPU disabled" exception stack frame	
 *  from the current 4 word exception stack frame. This includes		
 *  reproducing the effective address of the instruction to put on the	
 *  new stack frame.							
 * 									
 * 	In the process of all emulation work, if a _mem_read()		
 *  "callout" returns a failing result indicating an access error, then	
 *  we must create an access error stack frame from the current stack	
 *  frame. This information includes a faulting address and a fault-	
 *  status-longword. These are created within this handler.		
 * 									
 */


	.include "hdr.fpu"

	.xref _real_trace
	.xref _real_access
	.xref _real_fpu_disabled
	.xref _fpsp_done
	.xref set_tag_x
	.xref tbl_unsupp
	.xref store_fpreg
	.xref unnorm_fix
	.xref _imem_read_long
	.xref _dmem_write
	.xref _imem_read
	.xref get_packed
	.xref load_fpn2
	.xref decbin
	.xref fmovem_dynamic
	.xref fmovem_ctrl
	.xref fmovem_calc_ea

	.text

	.globl		_fpsp_effadd
_fpsp_effadd:

/*  This exception type takes priority over the "Line F Emulator" */
/*  exception. Therefore, the FPU could be disabled when entering here. */
/*  So, we must check to see if it's disabled and handle that case separately. */
	move.l		d0,-(sp)		/*  save d0 */
	.dc.w 0x4e7a,0x0808	/* movec		pcr,d0		load proc cr */
	btst		#0x1,d0		/*  is FPU disabled? */
	bne.w		iea_disabled		/*  yes */
	move.l		(sp)+,d0		/*  restore d0 */

	link		a6,#-LOCAL_SIZE	/*  init stack frame */

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	/*  save d0-d1/a0-a1 */
	fmovem.l		fpcr/fpsr/fpiar,USER_FPCR(a6) /*  save ctrl regs */
	fmovem.x		fp0-fp1,EXC_FPREGS(a6)	/*  save fp0-fp1 on stack */

/*  PC of instruction that took the exception is the PC in the frame */
	move.l		EXC_PC(a6),EXC_EXTWPTR(a6)

	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch the instruction words */
	move.l		d0,EXC_OPWORD(a6)	/*  store OPWORD and EXTWORD */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	tst.w		d0			/*  is operation fmovem? */
	bmi.w		iea_fmovem		/*  yes */

/*
 * here, we will have:
 *      fabs    fdabs   fsabs           facos           fmod
 *      fadd    fdadd   fsadd           fasin           frem
 *      fcmp                            fatan           fscale
 *      fdiv    fddiv   fsdiv           fatanh          fsin
 *      fint                            fcos            fsincos
 *      fintrz                          fcosh           fsinh
 *      fmove   fdmove  fsmove          fetox           ftan
 *      fmul    fdmul   fsmul           fetoxm1         ftanh
 *      fneg    fdneg   fsneg           fgetexp         ftentox
 *      fsgldiv                         fgetman         ftwotox
 *      fsglmul                         flog10
 *      fsqrt                           flog2
 *      fsub    fdsub   fssub           flogn
 *      ftst                            flognp1
 * which can all use f<op>.{x,p}
 * so, now it's immediate data extended precision AND PACKED FORMAT!
 */
iea_op:
	andi.l		#0x00ff00ff,USER_FPSR(a6)

	btst		#0xa,d0		/*  is src fmt x or p? */
	bne.b		iea_op_pack		/*  packed */


	move.l		EXC_EXTWPTR(a6),a0	/*  pass: ptr to #<data> */
	lea		FP_SRC(a6),a1		/*  pass: ptr to super addr */
	moveq.l		#0xc,d0		/*  pass: 12 bytes */
	bsr.l		_imem_read		/*  read extended immediate */

	tst.l		d1			/*  did ifetch fail? */
	bne.w		iea_iacc		/*  yes */

	bra.b		iea_op_setsrc

iea_op_pack:

	move.l		EXC_EXTWPTR(a6),a0	/*  pass: ptr to #<data> */
	lea		FP_SRC(a6),a1		/*  pass: ptr to super dst */
	moveq.l		#0xc,d0		/*  pass: 12 bytes */
	bsr.l		_imem_read		/*  read packed operand */

	tst.l		d1			/*  did ifetch fail? */
	bne.w		iea_iacc		/*  yes */

/*  The packed operand is an INF or a NAN if the exponent field is all ones. */
	bfextu		FP_SRC(a6){1:15},d0	/*  get exp */
	cmpi.w		#0x7fff,d0		/*  INF or NAN? */
	beq.b		iea_op_setsrc		/*  operand is an INF or NAN */

/*  The packed operand is a zero if the mantissa is all zero, else it's */
/*  a normal packed op. */
	move.b		3+FP_SRC(a6),d0	/*  get byte 4 */
	andi.b		#0x0f,d0		/*  clear all but last nybble */
	bne.b		iea_op_gp_not_spec	/*  not a zero */
	tst.l		FP_SRC_HI(a6)		/*  is lw 2 zero? */
	bne.b		iea_op_gp_not_spec	/*  not a zero */
	tst.l		FP_SRC_LO(a6)		/*  is lw 3 zero? */
	beq.b		iea_op_setsrc		/*  operand is a ZERO */
iea_op_gp_not_spec:
	lea		FP_SRC(a6),a0		/*  pass: ptr to packed op */
	bsr.l		decbin			/*  convert to extended */
	fmovem.x		fp0,FP_SRC(a6)	/*  make this the srcop */

iea_op_setsrc:
	addi.l		#0xc,EXC_EXTWPTR(a6)	/*  update extension word pointer */

/*  FP_SRC now holds the src operand. */
	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		set_tag_x		/*  tag the operand type */
	move.b		d0,STAG(a6)		/*  could be ANYTHING!!! */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		iea_op_getdst		/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM/DENORM/ZERO */
	move.b		d0,STAG(a6)		/*  set new optype tag */
iea_op_getdst:
	clr.b		STORE_FLG(a6)		/*  clear "store result" boolean */

	btst		#0x5,1+EXC_CMDREG(a6)	/*  is operation monadic or dyadic? */
	beq.b		iea_op_extract		/*  monadic */
	btst		#0x4,1+EXC_CMDREG(a6)	/*  is operation fsincos,ftst,fcmp? */
	bne.b		iea_op_spec		/*  yes */

iea_op_loaddst:
	bfextu		EXC_CMDREG(a6){6:3},d0 /*  fetch dst regno */
	bsr.l		load_fpn2		/*  load dst operand */

	lea		FP_DST(a6),a0		/*  pass: ptr to dst op */
	bsr.l		set_tag_x		/*  tag the operand type */
	move.b		d0,DTAG(a6)		/*  could be ANYTHING!!! */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		iea_op_extract		/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM/DENORM/ZERO */
	move.b		d0,DTAG(a6)		/*  set new optype tag */
	bra.b		iea_op_extract

/*  the operation is fsincos, ftst, or fcmp. only fcmp is dyadic */
iea_op_spec:
	btst		#0x3,1+EXC_CMDREG(a6)	/*  is operation fsincos? */
	beq.b		iea_op_extract		/*  yes */
/*  now, we're left with ftst and fcmp. so, first let's tag them so that they don't */
/*  store a result. then, only fcmp will branch back and pick up a dst operand. */
	st		STORE_FLG(a6)		/*  don't store a final result */
	btst		#0x1,1+EXC_CMDREG(a6)	/*  is operation fcmp? */
	beq.b		iea_op_loaddst		/*  yes */

iea_op_extract:
	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  pass: rnd mode,prec */

	move.b		1+EXC_CMDREG(a6),d1
	andi.w		#0x007f,d1		/*  extract extension */

	fmove.l		#0x0,fpcr
	fmove.l		#0x0,fpsr

	lea		FP_SRC(a6),a0
	lea		FP_DST(a6),a1

	move.l		(tbl_unsupp.l,pc,d1.w*4),d1 /*  fetch routine addr */
	jsr		(tbl_unsupp.l,pc,d1.l*1)

/*
 * Exceptions in order of precedence:
 *	BSUN	: none
 *	SNAN	: all operations
 *	OPERR	: all reg-reg or mem-reg operations that can normally operr
 *	OVFL	: same as OPERR
 *	UNFL	: same as OPERR
 *	DZ	: same as OPERR
 *	INEX2	: same as OPERR
 *	INEX1	: all packed immediate operations
 */

/*  we determine the highest priority exception(if any) set by the */
/*  emulation routine that has also been enabled by the user. */
	move.b		FPCR_ENABLE(a6),d0	/*  fetch exceptions enabled */
	bne.b		iea_op_ena		/*  some are enabled */

/*  now, we save the result, unless, of course, the operation was ftst or fcmp. */
/*  these don't save results. */
iea_op_save:
	tst.b		STORE_FLG(a6)		/*  does this op store a result? */
	bne.b		iea_op_exit1		/*  exit with no frestore */

iea_op_store:
	bfextu		EXC_CMDREG(a6){6:3},d0 /*  fetch dst regno */
	bsr.l		store_fpreg		/*  store the result */

iea_op_exit1:
	move.l		EXC_PC(a6),USER_FPIAR(a6) /*  set FPIAR to "Current PC" */
	move.l		EXC_EXTWPTR(a6),EXC_PC(a6) /*  set "Next PC" in exc frame */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6			/*  unravel the frame */

	btst		#0x7,(sp)		/*  is trace on? */
	bne.w		iea_op_trace		/*  yes */

	bra.l		_fpsp_done		/*  exit to os */

iea_op_ena:
	and.b		FPSR_EXCEPT(a6),d0	/*  keep only ones enable and set */
	bfffo		d0{24:8},d0		/*  find highest priority exception */
	bne.b		iea_op_exc		/*  at least one was set */

/*  no exception occurred. now, did a disabled, exact overflow occur with inexact */
/*  enabled? if so, then we have to stuff an overflow frame into the FPU. */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  did overflow occur? */
	beq.b		iea_op_save

iea_op_ovfl:
	btst		#inex2_bit,FPCR_ENABLE(a6) /*  is inexact enabled? */
	beq.b		iea_op_store		/*  no */
	bra.b		iea_op_exc_ovfl		/*  yes */

/*  an enabled exception occurred. we have to insert the exception type back into */
/*  the machine. */
iea_op_exc:
	subi.l		#24,d0			/*  fix offset to be 0-8 */
	cmpi.b		#0x6,d0		/*  is exception INEX? */
	bne.b		iea_op_exc_force	/*  no */

/*  the enabled exception was inexact. so, if it occurs with an overflow */
/*  or underflow that was disabled, then we have to force an overflow or */
/*  underflow frame. */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  did overflow occur? */
	bne.b		iea_op_exc_ovfl		/*  yes */
	btst		#unfl_bit,FPSR_EXCEPT(a6) /*  did underflow occur? */
	bne.b		iea_op_exc_unfl		/*  yes */

iea_op_exc_force:
	move.w		(tbl_iea_except.b,pc,d0.w*2),2+FP_SRC(a6)
	bra.b		iea_op_exit2		/*  exit with frestore */

tbl_iea_except:
	.dc.w		0xe002, 0xe006, 0xe004, 0xe005
	.dc.w		0xe003, 0xe002, 0xe001, 0xe001

iea_op_exc_ovfl:
	move.w		#0xe005,2+FP_SRC(a6)
	bra.b		iea_op_exit2

iea_op_exc_unfl:
	move.w		#0xe003,2+FP_SRC(a6)

iea_op_exit2:
	move.l		EXC_PC(a6),USER_FPIAR(a6) /*  set FPIAR to "Current PC" */
	move.l		EXC_EXTWPTR(a6),EXC_PC(a6) /*  set "Next PC" in exc frame */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  restore exceptional state */

	unlk		a6			/*  unravel the frame */

	btst		#0x7,(sp)		/*  is trace on? */
	bne.b		iea_op_trace		/*  yes */

	bra.l		_fpsp_done		/*  exit to os */

/*
 * The opclass two instruction that took an "Unimplemented Effective Address"
 * exception was being traced. Make the "current" PC the FPIAR and put it in
 * the trace stack frame then jump to _real_trace().
 *
 *               UNIMP EA FRAME            TRACE FRAME
 *              *****************       *****************
 *              * 0x0 *  0x0f0  *       *    Current    *
 *              *****************       *      PC       *
 *              *    Current    *       *****************
 *              *      PC       *       * 0x2 *  0x024  *
 *              *****************       *****************
 *              *      SR       *       *     Next      *
 *              *****************       *      PC       *
 *                                      *****************
 *                                      *      SR       *
 *                                      *****************
 */
iea_op_trace:
	move.l		(sp),-(sp)		/*  shift stack frame "down" */
	move.w		0x8(sp),0x4(sp)
	move.w		#0x2024,0x6(sp)	/*  stk fmt = 0x2; voff = 0x024 */
	fmove.l		fpiar,0x8(sp)		/*  "Current PC" is in FPIAR */

	bra.l		_real_trace

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
iea_fmovem:
	btst		#14,d0			/*  ctrl or data reg */
	beq.w		iea_fmovem_ctrl

iea_fmovem_data:

	btst		#0x5,EXC_SR(a6)	/*  user or supervisor mode */
	bne.b		iea_fmovem_data_s

iea_fmovem_data_u:
	move.l		usp,a0
	move.l		a0,EXC_A7(a6)		/*  store current a7 */
	bsr.l		fmovem_dynamic		/*  do dynamic fmovem */
	move.l		EXC_A7(a6),a0		/*  load possibly new a7 */
	move.l		a0,usp		/*  update usp */
	bra.w		iea_fmovem_exit

iea_fmovem_data_s:
	clr.b		SPCOND_FLG(a6)
	lea		0x2+EXC_VOFF(a6),a0
	move.l		a0,EXC_A7(a6)
	bsr.l		fmovem_dynamic		/*  do dynamic fmovem */

	cmpi.b		#mda7_flg,SPCOND_FLG(a6)
	beq.w		iea_fmovem_data_predec
	cmpi.b		#mia7_flg,SPCOND_FLG(a6)
	bne.w		iea_fmovem_exit

/*  right now, d0 = the size. */
/*  the data has been fetched from the supervisor stack, but we have not */
/*  incremented the stack pointer by the appropriate number of bytes. */
/*  do it here. */
iea_fmovem_data_postinc:
	btst		#0x7,EXC_SR(a6)
	bne.b		iea_fmovem_data_pi_trace

	move.w		EXC_SR(a6),(EXC_SR,a6,d0.l)
	move.l		EXC_EXTWPTR(a6),(EXC_PC,a6,d0.l)
	move.w		#0x00f0,(EXC_VOFF,a6,d0.l)

	lea		(EXC_SR,a6,d0.l),a0
	move.l		a0,EXC_SR(a6)

	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6
	move.l		(sp)+,sp
	bra.l		_fpsp_done

iea_fmovem_data_pi_trace:
	move.w		EXC_SR(a6),(EXC_SR-0x4,a6,d0.l)
	move.l		EXC_EXTWPTR(a6),(EXC_PC-0x4,a6,d0.l)
	move.w		#0x2024,(EXC_VOFF-0x4,a6,d0.l)
	move.l		EXC_PC(a6),(EXC_VOFF+0x2-0x4,a6,d0.l)

	lea		(EXC_SR-0x4,a6,d0.l),a0
	move.l		a0,EXC_SR(a6)

	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6
	move.l		(sp)+,sp
	bra.l		_real_trace

/*  right now, d1 = size and d0 = the strg. */
iea_fmovem_data_predec:
	move.b		d1,EXC_VOFF(a6)	/*  store strg */
	move.b		d0,0x1+EXC_VOFF(a6)	/*  store size */

	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	move.l		(a6),-(sp)		/*  make a copy of a6 */
	move.l		d0,-(sp)		/*  save d0 */
	move.l		d1,-(sp)		/*  save d1 */
	move.l		EXC_EXTWPTR(a6),-(sp)	/*  make a copy of Next PC */

	clr.l		d0
	move.b		0x1+EXC_VOFF(a6),d0	/*  fetch size */
	neg.l		d0			/*  get negative of size */

	btst		#0x7,EXC_SR(a6)	/*  is trace enabled? */
	beq.b		iea_fmovem_data_p2

	move.w		EXC_SR(a6),(EXC_SR-0x4,a6,d0.l)
	move.l		EXC_PC(a6),(EXC_VOFF-0x2,a6,d0.l)
	move.l		(sp)+,(EXC_PC-0x4,a6,d0.l)
	move.w		#0x2024,(EXC_VOFF-0x4,a6,d0.l)

	pea		(a6,d0.l)		/*  create final sp */
	bra.b		iea_fmovem_data_p3

iea_fmovem_data_p2:
	move.w		EXC_SR(a6),(EXC_SR,a6,d0.l)
	move.l		(sp)+,(EXC_PC,a6,d0.l)
	move.w		#0x00f0,(EXC_VOFF,a6,d0.l)

	pea		(0x4,a6,d0.l)		/*  create final sp */

iea_fmovem_data_p3:
	clr.l		d1
	move.b		EXC_VOFF(a6),d1	/*  fetch strg */

	tst.b		d1
	bpl.b		fm_1
	fmovem.x	fp0,(0x4+0x8,a6,d0.l)
	addi.l		#0xc,d0
fm_1:
	lsl.b		#0x1,d1
	bpl.b		fm_2
	fmovem.x	fp1,(0x4+0x8,a6,d0.l)
	addi.l		#0xc,d0
fm_2:
	lsl.b		#0x1,d1
	bpl.b		fm_3
	fmovem.x	fp2,(0x4+0x8,a6,d0.l)
	addi.l		#0xc,d0
fm_3:
	lsl.b		#0x1,d1
	bpl.b		fm_4
	fmovem.x	fp3,(0x4+0x8,a6,d0.l)
	addi.l		#0xc,d0
fm_4:
	lsl.b		#0x1,d1
	bpl.b		fm_5
	fmovem.x	fp4,(0x4+0x8,a6,d0.l)
	addi.l		#0xc,d0
fm_5:
	lsl.b		#0x1,d1
	bpl.b		fm_6
	fmovem.x	fp5,(0x4+0x8,a6,d0.l)
	addi.l		#0xc,d0
fm_6:
	lsl.b		#0x1,d1
	bpl.b		fm_7
	fmovem.x	fp6,(0x4+0x8,a6,d0.l)
	addi.l		#0xc,d0
fm_7:
	lsl.b		#0x1,d1
	bpl.b		fm_end
	fmovem.x	fp7,(0x4+0x8,a6,d0.l)
fm_end:
	move.l		0x4(sp),d1
	move.l		0x8(sp),d0
	move.l		0xc(sp),a6
	move.l		(sp)+,sp

	btst		#0x7,(sp)		/*  is trace enabled? */
	beq.l		_fpsp_done
	bra.l		_real_trace

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
iea_fmovem_ctrl:

	bsr.l		fmovem_ctrl		/*  load ctrl regs */

iea_fmovem_exit:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	btst		#0x7,EXC_SR(a6)	/*  is trace on? */
	bne.b		iea_fmovem_trace		/*  yes */

	move.l		EXC_EXTWPTR(a6),EXC_PC(a6) /*  set Next PC */

	unlk		a6			/*  unravel the frame */

	bra.l		_fpsp_done		/*  exit to os */

/*
 * The control reg instruction that took an "Unimplemented Effective Address"
 * exception was being traced. The "Current PC" for the trace frame is the
 * PC stacked for Unimp EA. The "Next PC" is in EXC_EXTWPTR.
 * After fixing the stack frame, jump to _real_trace().
 *
 *               UNIMP EA FRAME            TRACE FRAME
 *              *****************       *****************
 *              * 0x0 *  0x0f0  *       *    Current    *
 *              *****************       *      PC       *
 *              *    Current    *       *****************
 *              *      PC       *       * 0x2 *  0x024  *
 *              *****************       *****************
 *              *      SR       *       *     Next      *
 *              *****************       *      PC       *
 *                                      *****************
 *                                      *      SR       *
 *                                      *****************
 * this ain't a pretty solution, but it works:
 * -restore a6 (not with unlk)
 * -shift stack frame down over where old a6 used to be
 * -add LOCAL_SIZE to stack pointer
 */
iea_fmovem_trace:
	move.l		(a6),a6		/*  restore frame pointer */
	move.w		EXC_SR+LOCAL_SIZE(sp),0x0+LOCAL_SIZE(sp)
	move.l		EXC_PC+LOCAL_SIZE(sp),0x8+LOCAL_SIZE(sp)
	move.l		EXC_EXTWPTR+LOCAL_SIZE(sp),0x2+LOCAL_SIZE(sp)
	move.w		#0x2024,0x6+LOCAL_SIZE(sp) /*  stk fmt = 0x2; voff = 0x024 */
	add.l		#LOCAL_SIZE,sp		/*  clear stack frame */

	bra.l		_real_trace

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  The FPU is disabled and so we should really have taken the "Line */
/*  F Emulator" exception. So, here we create an 8-word stack frame */
/*  from our 4-word stack frame. This means we must calculate the length */
/*  the faulting instruction to get the "next PC". This is trivial for */
/*  immediate operands but requires some extra work for fmovem dynamic */
/*  which can use most addressing modes. */
iea_disabled:
	move.l		(sp)+,d0		/*  restore d0 */

	link		a6,#-LOCAL_SIZE	/*  init stack frame */

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	/*  save d0-d1/a0-a1 */

/*  PC of instruction that took the exception is the PC in the frame */
	move.l		EXC_PC(a6),EXC_EXTWPTR(a6)
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch the instruction words */
	move.l		d0,EXC_OPWORD(a6)	/*  store OPWORD and EXTWORD */

	tst.w		d0			/*  is instr fmovem? */
	bmi.b		iea_dis_fmovem		/*  yes */
/*  instruction is using an extended precision immediate operand. Therefore, */
/*  the total instruction length is 16 bytes. */
iea_dis_immed:
	moveq.l		#0x10,d0		/*  16 bytes of instruction */
	bra.b		iea_dis_cont
iea_dis_fmovem:
	btst		#0xe,d0		/*  is instr fmovem ctrl */
	bne.b		iea_dis_fmovem_data	/*  no */
/*  the instruction is a fmovem.l with 2 or 3 registers. */
	bfextu		d0{19:3},d1
	moveq.l		#0xc,d0
	cmpi.b		#0x7,d1		/*  move all regs? */
	bne.b		iea_dis_cont
	addq.l		#0x4,d0
	bra.b		iea_dis_cont
/*  the instruction is an fmovem.x dynamic which can use many addressing */
/*  modes and thus can have several different total instruction lengths. */
/*  call fmovem_calc_ea which will go through the ea calc process and, */
/*  as a by-product, will tell us how long the instruction is. */
iea_dis_fmovem_data:
	clr.l		d0
	bsr.l		fmovem_calc_ea
	move.l		EXC_EXTWPTR(a6),d0
	sub.l		EXC_PC(a6),d0
iea_dis_cont:
	move.w		d0,EXC_VOFF(a6)	/*  store stack shift value */

	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

/*  here, we actually create the 8-word frame from the 4-word frame, */
/*  with the "next PC" as additional info. */
/*  the <ea> field is let as undefined. */
	subq.l		#0x8,sp		/*  make room for new stack */
	move.l		d0,-(sp)		/*  save d0 */
	move.w		0xc(sp),0x4(sp)	/*  move SR */
	move.l		0xe(sp),0x6(sp)	/*  move Current PC */
	clr.l		d0
	move.w		0x12(sp),d0
	move.l		0x6(sp),0x10(sp)	/*  move Current PC */
	add.l		d0,0x6(sp)		/*  make Next PC */
	move.w		#0x402c,0xa(sp)	/*  insert offset,frame format */
	move.l		(sp)+,d0		/*  restore d0 */

	bra.l		_real_fpu_disabled

/* ;;;;;;;; */

	.globl iea_iacc
iea_iacc:
	.dc.w 0x4e7a,0x0808	/* movec		pcr,d0 */
	btst		#0x1,d0
	bne.b		iea_iacc_cont
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 on stack */
iea_iacc_cont:
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	subq.w		#0x8,sp		/*  make stack frame bigger */
	move.l		0x8(sp),(sp)		/*  store SR,hi(PC) */
	move.w		0xc(sp),0x4(sp)	/*  store lo(PC) */
	move.w		#0x4008,0x6(sp)	/*  store voff */
	move.l		0x2(sp),0x8(sp)	/*  store ea */
	move.l		#0x09428001,0xc(sp)	/*  store fslw */

iea_acc_done:
	btst		#0x5,(sp)		/*  user or supervisor mode? */
	beq.b		iea_acc_done2		/*  user */
	bset		#0x2,0xd(sp)		/*  set supervisor TM bit */

iea_acc_done2:
	bra.l		_real_access

	.globl iea_dacc
iea_dacc:
	lea		-LOCAL_SIZE(a6),sp

	.dc.w 0x4e7a,0x1808	/* movec		pcr,d1 */
	btst		#0x1,d1
	bne.b		iea_dacc_cont
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 on stack */
	fmovem.l		LOCAL_SIZE+USER_FPCR(sp),fpcr/fpsr/fpiar /*  restore ctrl regs */
iea_dacc_cont:
	move.l		(a6),a6

	move.l		0x4+LOCAL_SIZE(sp),-0x8+0x4+LOCAL_SIZE(sp)
	move.w		0x8+LOCAL_SIZE(sp),-0x8+0x8+LOCAL_SIZE(sp)
	move.w		#0x4008,-0x8+0xa+LOCAL_SIZE(sp)
	move.l		a0,-0x8+0xc+LOCAL_SIZE(sp)
	move.w		d0,-0x8+0x10+LOCAL_SIZE(sp)
	move.w		#0x0001,-0x8+0x12+LOCAL_SIZE(sp)

	movem.l		LOCAL_SIZE+EXC_DREGS(sp),d0-d1/a0-a1 /*  restore d0-d1/a0-a1 */
	add.w		#LOCAL_SIZE-0x4,sp

	bra.b		iea_acc_done

