/*
 *  XDEF ****************************************************************	
 * 	fsgldiv(): emulates the fsgldiv instruction			
 * 									
 *  XREF ****************************************************************	
 * 	scale_to_zero_src() - scale src exponent to zero		
 * 	scale_to_zero_dst() - scale dst exponent to zero		
 * 	unf_res4() - return default underflow result for sglop		
 * 	ovf_res() - return default overflow result			
 * 	res_qnan() - return QNAN result					
 * 	res_snan() - return SNAN result					
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	a1 = pointer to extended precision destination operand		
 * 	d0  rnd prec,mode						
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = result							
 * 	fp1 = EXOP (if exception occurred)				
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Handle NANs, infinities, and zeroes as special cases. Divide	
 *  norms/denorms into ext/sgl/dbl precision.				
 * 	For norms/denorms, scale the exponents such that a divide	
 *  instruction won't cause an exception. Use the regular fsgldiv to	
 *  compute a result. Check if the regular operands would have taken	
 *  an exception. If so, return the default overflow/underflow result	
 *  and return the EXOP if exceptions are enabled. Else, scale the	
 *  result operand to the proper exponent.				
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fsgldiv
fsgldiv:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */

	clr.w		d1
	move.b		DTAG(a6),d1
	lsl.b		#0x3,d1
	or.b		STAG(a6),d1		/*  combine src tags */

	bne.w		fsgldiv_not_norm	/*  optimize on non-norm input */

/*
 * DIVIDE: NORMs and DENORMs ONLY!
 */
fsgldiv_norm:
	move.w		DST_EX.w(a1),FP_SCR1_EX(a6)
	move.l		DST_HI(a1),FP_SCR1_HI(a6)
	move.l		DST_LO(a1),FP_SCR1_LO(a6)

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	bsr.l		scale_to_zero_src	/*  calculate scale factor 1 */
	move.l		d0,-(sp)		/*  save scale factor 1 */

	bsr.l		scale_to_zero_dst	/*  calculate scale factor 2 */

	neg.l		(sp)			/*  S.F. = scale1 - scale2 */
	add.l		d0,(sp)

	move.w		2+L_SCR3(a6),d1	/*  fetch precision,mode */
	lsr.b		#0x6,d1
	move.l		(sp)+,d0
	cmpi.l		#0x3fff-0x7ffe,d0
	ble.w		fsgldiv_may_ovfl

	cmpi.l		#0x3fff-0x0000,d0	/*  will result underflow? */
	beq.w		fsgldiv_may_unfl	/*  maybe */
	bgt.w		fsgldiv_unfl		/*  yes; go handle underflow */

fsgldiv_normal:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  save FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsgldiv.x	FP_SCR0(a6),fp0	/*  perform sgl divide */

	fmove.l		fpsr,d1		/*  save FPSR */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fsgldiv_normal_exit:
	fmovem.x	fp0,FP_SCR0(a6)	/*  store result on stack */
	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  load {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x	FP_SCR0(a6),fp0	/*  return result in fp0 */
	rts

fsgldiv_may_ovfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  set FPSR */

	fsgldiv.x	FP_SCR0(a6),fp0	/*  execute divide */

	fmove.l		fpsr,d1
	fmove.l		#0x0,fpcr

	or.l		d1,USER_FPSR(a6)	/*  save INEX,N */

	fmovem.x	fp0,-(sp)		/*  save result to stack */
	move.w		(sp),d1		/*  fetch new exponent */
	add.l		#0xc,sp		/*  clear result */
	andi.l		#0x7fff,d1		/*  strip sign */
	sub.l		d0,d1			/*  add scale factor */
	cmpi.l		#0x7fff,d1		/*  did divide overflow? */
	blt.b		fsgldiv_normal_exit

fsgldiv_ovfl_tst:
	ori.w		#ovfl_inx_mask,2+USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fsgldiv_ovfl_ena	/*  yes */

fsgldiv_ovfl_dis:
	btst		#neg_bit,FPSR_CC(a6)	/*  is result negative */
	sne			d1			/*  set sign param accordingly */
	move.l		L_SCR3(a6),d0		/*  pass prec:rnd */
	andi.b		#0x30,d0		/*  kill precision */
	bsr.l		ovf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set INF if applicable */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */
	rts

fsgldiv_ovfl_ena:
	fmovem.x	fp0,FP_SCR0(a6)	/*  move result to stack */

	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	subi.l		#0x6000,d1		/*  subtract new bias */
	andi.w		#0x7fff,d1		/*  clear ms bit */
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x	FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.b		fsgldiv_ovfl_dis

fsgldiv_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		#rz_mode*0x10,fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsgldiv.x	FP_SCR0(a6),fp0	/*  execute sgl divide */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fsgldiv_unfl_ena	/*  yes */

fsgldiv_unfl_dis:
	fmovem.x	fp0,FP_SCR0(a6)	/*  store out result */

	lea			FP_SCR0(a6),a0	/*  pass: result addr */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res4		/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  'Z' bit may have been set */
	fmovem.x	FP_SCR0(a6),fp0	/*  return default result in fp0 */
	rts

/*
 * UNFL is enabled.
 */
fsgldiv_unfl_ena:
	fmovem.x	FP_SCR1(a6),fp1	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsgldiv.x	FP_SCR0(a6),fp1	/*  execute sgl divide */

	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fmovem.x	fp1,FP_SCR0(a6)	/*  save result to stack */
	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	addi.l		#0x6000,d1		/*  add bias */
	andi.w		#0x7fff,d1		/*  clear top bit */
	or.w		d2,d1			/*  concat old sign, new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x	FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.b		fsgldiv_unfl_dis

/*
 * the divide operation MAY underflow:
 */
fsgldiv_may_unfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsgldiv.x	FP_SCR0(a6),fp0	/*  execute sgl divide */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x1,fp1		/*  is |result| > 1.b? */
	fbgt		fsgldiv_normal_exit	/*  no; no underflow occurred */
	fblt		fsgldiv_unfl		/*  yes; underflow occurred */

/*
 * we still don't know if underflow occurred. result is ~ equal to 1. but,
 * we don't know if the result was an underflow that rounded up to a 1
 * or a normalized number that rounded down to a 1. so, redo the entire
 * operation using RZ as the rounding mode to see what the pre-rounded
 * result is. this case should be relatively rare.
 */
	fmovem.x	FP_SCR1(a6),fp1	/*  load dst op into fp1 */

	clr.l		d1			/*  clear scratch register */
	ori.b		#rz_mode*0x10,d1	/*  force RZ rnd mode */

	fmove.l		d1,fpcr		/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsgldiv.x	FP_SCR0(a6),fp1	/*  execute sgl divide */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fabs.x		fp1			/*  make absolute value */
	fcmp.b		#0x1,fp1		/*  is |result| < 1.b? */
	fbge		fsgldiv_normal_exit	/*  no; no underflow occurred */
	bra.w		fsgldiv_unfl		/*  yes; underflow occurred */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * Divide: inputs are not both normalized; what are they?
 */
fsgldiv_not_norm:
	move.w		(tbl_fsgldiv_op.b,pc,d1.w*2),d1
	jmp			(tbl_fsgldiv_op.b,pc,d1.w*1)

	/* swbeg		#48 */
	.dc.w 0x4afc,48
tbl_fsgldiv_op:
	.dc.w		fsgldiv_norm-tbl_fsgldiv_op /*  NORM / NORM */
	.dc.w		fsgldiv_inf_load-tbl_fsgldiv_op /*  NORM / ZERO */
	.dc.w		fsgldiv_zero_load-tbl_fsgldiv_op /*  NORM / INF */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  NORM / QNAN */
	.dc.w		fsgldiv_norm-tbl_fsgldiv_op /*  NORM / DENORM */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  NORM / SNAN */
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op

	.dc.w		fsgldiv_zero_load-tbl_fsgldiv_op /*  ZERO / NORM */
	.dc.w		fsgldiv_res_operr-tbl_fsgldiv_op /*  ZERO / ZERO */
	.dc.w		fsgldiv_zero_load-tbl_fsgldiv_op /*  ZERO / INF */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  ZERO / QNAN */
	.dc.w		fsgldiv_zero_load-tbl_fsgldiv_op /*  ZERO / DENORM */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  ZERO / SNAN */
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op

	.dc.w		fsgldiv_inf_dst-tbl_fsgldiv_op /*  INF / NORM */
	.dc.w		fsgldiv_inf_dst-tbl_fsgldiv_op /*  INF / ZERO */
	.dc.w		fsgldiv_res_operr-tbl_fsgldiv_op /*  INF / INF */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  INF / QNAN */
	.dc.w		fsgldiv_inf_dst-tbl_fsgldiv_op /*  INF / DENORM */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  INF / SNAN */
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op

	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  QNAN / NORM */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  QNAN / ZERO */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  QNAN / INF */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  QNAN / QNAN */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  QNAN / DENORM */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  QNAN / SNAN */
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op

	.dc.w		fsgldiv_norm-tbl_fsgldiv_op /*  DENORM / NORM */
	.dc.w		fsgldiv_inf_load-tbl_fsgldiv_op /*  DENORM / ZERO */
	.dc.w		fsgldiv_zero_load-tbl_fsgldiv_op /*  DENORM / INF */
	.dc.w		fsgldiv_res_qnan-tbl_fsgldiv_op /*  DENORM / QNAN */
	.dc.w		fsgldiv_norm-tbl_fsgldiv_op /*  DENORM / DENORM */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  DENORM / SNAN */
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op

	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  SNAN / NORM */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  SNAN / ZERO */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  SNAN / INF */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  SNAN / QNAN */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  SNAN / DENORM */
	.dc.w		fsgldiv_res_snan-tbl_fsgldiv_op /*  SNAN / SNAN */
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op
	.dc.w		tbl_fsgldiv_op-tbl_fsgldiv_op

fsgldiv_res_qnan:
	bra.l		res_qnan
fsgldiv_res_snan:
	bra.l		res_snan
fsgldiv_res_operr:
	bra.l		res_operr
fsgldiv_inf_load:
	bra.l		fdiv_inf_load
fsgldiv_zero_load:
	bra.l		fdiv_zero_load
fsgldiv_inf_dst:
	bra.l		fdiv_inf_dst
