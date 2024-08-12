/*
 *  XDEF ****************************************************************	
 * 	fsglmul(): emulates the fsglmul instruction			
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
 * 	For norms/denorms, scale the exponents such that a multiply	
 *  instruction won't cause an exception. Use the regular fsglmul to	
 *  compute a result. Check if the regular operands would have taken	
 *  an exception. If so, return the default overflow/underflow result	
 *  and return the EXOP if exceptions are enabled. Else, scale the	
 *  result operand to the proper exponent.				
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fsglmul
fsglmul:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */

	clr.w		d1
	move.b		DTAG(a6),d1
	lsl.b		#0x3,d1
	or.b		STAG(a6),d1

	bne.w		fsglmul_not_norm	/*  optimize on non-norm input */

fsglmul_norm:
	move.w		DST_EX.w(a1),FP_SCR1_EX(a6)
	move.l		DST_HI(a1),FP_SCR1_HI(a6)
	move.l		DST_LO(a1),FP_SCR1_LO(a6)

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	bsr.l		scale_to_zero_src	/*  scale exponent */
	move.l		d0,-(sp)		/*  save scale factor 1 */

	bsr.l		scale_to_zero_dst	/*  scale dst exponent */

	add.l		(sp)+,d0		/*  SCALE_FACTOR = scale1 + scale2 */

	cmpi.l		#0x3fff-0x7ffe,d0	/*  would result ovfl? */
	beq.w		fsglmul_may_ovfl	/*  result may rnd to overflow */
	blt.w		fsglmul_ovfl		/*  result will overflow */

	cmpi.l		#0x3fff+0x0001,d0	/*  would result unfl? */
	beq.w		fsglmul_may_unfl	/*  result may rnd to no unfl */
	bgt.w		fsglmul_unfl		/*  result will underflow */

fsglmul_normal:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsglmul.x	FP_SCR0(a6),fp0	/*  execute sgl multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fsglmul_normal_exit:
	fmovem.x	fp0,FP_SCR0(a6)	/*  store out result */
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

fsglmul_ovfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsglmul.x	FP_SCR0(a6),fp0	/*  execute sgl multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fsglmul_ovfl_tst:

/*  save setting this until now because this is where fsglmul_may_ovfl may jump in */
	ori.l		#ovfl_inx_mask, USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fsglmul_ovfl_ena	/*  yes */

fsglmul_ovfl_dis:
	btst		#neg_bit,FPSR_CC(a6)	/*  is result negative? */
	sne			d1			/*  set sign param accordingly */
	move.l		L_SCR3(a6),d0		/*  pass prec:rnd */
	andi.b		#0x30,d0		/*  force prec = ext */
	bsr.l		ovf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set INF,N if applicable */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */
	rts

fsglmul_ovfl_ena:
	fmovem.x		fp0,FP_SCR0(a6)	/*  move result to stack */

	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	sub.l		d0,d1			/*  add scale factor */
	subi.l		#0x6000,d1		/*  subtract bias */
	andi.w		#0x7fff,d1
	andi.w		#0x8000,d2		/*  keep old sign */
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x	FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.b		fsglmul_ovfl_dis

fsglmul_may_ovfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsglmul.x	FP_SCR0(a6),fp0	/*  execute sgl multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x2,fp1		/*  is |result| >= 2.b? */
	fbge		fsglmul_ovfl_tst	/*  yes; overflow has occurred */

/*  no, it didn't overflow; we have correct result */
	bra.w		fsglmul_normal_exit

fsglmul_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		#rz_mode*0x10,fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsglmul.x	FP_SCR0(a6),fp0	/*  execute sgl multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fsglmul_unfl_ena	/*  yes */

fsglmul_unfl_dis:
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
fsglmul_unfl_ena:
	fmovem.x	FP_SCR1(a6),fp1	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsglmul.x	FP_SCR0(a6),fp1	/*  execute sgl multiply */

	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fmovem.x	fp1,FP_SCR0(a6)	/*  save result to stack */
	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	addi.l		#0x6000,d1		/*  add bias */
	andi.w		#0x7fff,d1
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x	FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.w		fsglmul_unfl_dis

fsglmul_may_unfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsglmul.x	FP_SCR0(a6),fp0	/*  execute sgl multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x2,fp1		/*  is |result| > 2.b? */
	fbgt		fsglmul_normal_exit	/*  no; no underflow occurred */
	fblt		fsglmul_unfl		/*  yes; underflow occurred */

/*
 * we still don't know if underflow occurred. result is ~ equal to 2. but,
 * we don't know if the result was an underflow that rounded up to a 2 or
 * a normalized number that rounded down to a 2. so, redo the entire operation
 * using RZ as the rounding mode to see what the pre-rounded result is.
 * this case should be relatively rare.
 */
	fmovem.x	FP_SCR1(a6),fp1	/*  load dst op into fp1 */

	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  keep rnd prec */
	ori.b		#rz_mode*0x10,d1	/*  insert RZ */

	fmove.l		d1,fpcr		/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsglmul.x	FP_SCR0(a6),fp1	/*  execute sgl multiply */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fabs.x		fp1			/*  make absolute value */
	fcmp.b		#0x2,fp1		/*  is |result| < 2.b? */
	fbge		fsglmul_normal_exit	/*  no; no underflow occurred */
	bra.w		fsglmul_unfl		/*  yes, underflow occurred */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * Single Precision Multiply: inputs are not both normalized; what are they?
 */
fsglmul_not_norm:
	move.w		(tbl_fsglmul_op.b,pc,d1.w*2),d1
	jmp		(tbl_fsglmul_op.b,pc,d1.w*1)

	/* swbeg		#48 */
	.dc.w 0x4afc,48
tbl_fsglmul_op:
	.dc.w		fsglmul_norm-tbl_fsglmul_op /*  NORM x NORM */
	.dc.w		fsglmul_zero-tbl_fsglmul_op /*  NORM x ZERO */
	.dc.w		fsglmul_inf_src-tbl_fsglmul_op /*  NORM x INF */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  NORM x QNAN */
	.dc.w		fsglmul_norm-tbl_fsglmul_op /*  NORM x DENORM */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  NORM x SNAN */
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op

	.dc.w		fsglmul_zero-tbl_fsglmul_op /*  ZERO x NORM */
	.dc.w		fsglmul_zero-tbl_fsglmul_op /*  ZERO x ZERO */
	.dc.w		fsglmul_res_operr-tbl_fsglmul_op /*  ZERO x INF */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  ZERO x QNAN */
	.dc.w		fsglmul_zero-tbl_fsglmul_op /*  ZERO x DENORM */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  ZERO x SNAN */
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op

	.dc.w		fsglmul_inf_dst-tbl_fsglmul_op /*  INF x NORM */
	.dc.w		fsglmul_res_operr-tbl_fsglmul_op /*  INF x ZERO */
	.dc.w		fsglmul_inf_dst-tbl_fsglmul_op /*  INF x INF */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  INF x QNAN */
	.dc.w		fsglmul_inf_dst-tbl_fsglmul_op /*  INF x DENORM */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  INF x SNAN */
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op

	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  QNAN x NORM */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  QNAN x ZERO */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  QNAN x INF */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  QNAN x QNAN */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  QNAN x DENORM */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  QNAN x SNAN */
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op

	.dc.w		fsglmul_norm-tbl_fsglmul_op /*  NORM x NORM */
	.dc.w		fsglmul_zero-tbl_fsglmul_op /*  NORM x ZERO */
	.dc.w		fsglmul_inf_src-tbl_fsglmul_op /*  NORM x INF */
	.dc.w		fsglmul_res_qnan-tbl_fsglmul_op /*  NORM x QNAN */
	.dc.w		fsglmul_norm-tbl_fsglmul_op /*  NORM x DENORM */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  NORM x SNAN */
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op

	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  SNAN x NORM */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  SNAN x ZERO */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  SNAN x INF */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  SNAN x QNAN */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  SNAN x DENORM */
	.dc.w		fsglmul_res_snan-tbl_fsglmul_op /*  SNAN x SNAN */
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op
	.dc.w		tbl_fsglmul_op-tbl_fsglmul_op

fsglmul_res_operr:
	bra.l		res_operr
fsglmul_res_snan:
	bra.l		res_snan
fsglmul_res_qnan:
	bra.l		res_qnan
fsglmul_zero:
	bra.l		fmul_zero
fsglmul_inf_src:
	bra.l		fmul_inf_src
fsglmul_inf_dst:
	bra.l		fmul_inf_dst

