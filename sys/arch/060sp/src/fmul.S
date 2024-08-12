/*
 *  XDEF ****************************************************************	
 * 	fmul(): emulates the fmul instruction				
 * 	fsmul(): emulates the fsmul instruction				
 * 	fdmul(): emulates the fdmul instruction				
 * 									
 *  XREF ****************************************************************	
 * 	scale_to_zero_src() - scale src exponent to zero		
 * 	scale_to_zero_dst() - scale dst exponent to zero		
 * 	unf_res() - return default underflow result			
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
 *  instruction won't cause an exception. Use the regular fmul to		
 *  compute a result. Check if the regular operands would have taken	
 *  an exception. If so, return the default overflow/underflow result	
 *  and return the EXOP if exceptions are enabled. Else, scale the	
 *  result operand to the proper exponent.				
 * 									
 */

	.include "hdr.fpu"

	.text

	/* .balignw		16,0x51fc */
tbl_fmul_ovfl:
	.dc.l		0x3fff - 0x7ffe		/*  ext_max */
	.dc.l		0x3fff - 0x407e		/*  sgl_max */
	.dc.l		0x3fff - 0x43fe		/*  dbl_max */
tbl_fmul_unfl:
	.dc.l		0x3fff + 0x0001		/*  ext_unfl */
	.dc.l		0x3fff - 0x3f80		/*  sgl_unfl */
	.dc.l		0x3fff - 0x3c00		/*  dbl_unfl */

	.globl		fsmul
fsmul:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#s_mode*0x10,d0	/*  insert sgl prec */
	bra.b		fmul

	.globl		fdmul
fdmul:
	andi.b		#0x30,d0
	ori.b		#d_mode*0x10,d0	/*  insert dbl prec */

	.globl		fmul
fmul:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */

	clr.w		d1
	move.b		DTAG(a6),d1
	lsl.b		#0x3,d1
	or.b		STAG(a6),d1		/*  combine src tags */
	bne.w		fmul_not_norm		/*  optimize on non-norm input */

fmul_norm:
	move.w		DST_EX.w(a1),FP_SCR1_EX(a6)
	move.l		DST_HI(a1),FP_SCR1_HI(a6)
	move.l		DST_LO(a1),FP_SCR1_LO(a6)

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	bsr.l		scale_to_zero_src	/*  scale src exponent */
	move.l		d0,-(sp)		/*  save scale factor 1 */

	bsr.l		scale_to_zero_dst	/*  scale dst exponent */

	add.l		d0,(sp)		/*  SCALE_FACTOR = scale1 + scale2 */

	move.w		2+L_SCR3(a6),d1	/*  fetch precision */
	lsr.b		#0x6,d1		/*  shift to lo bits */
	move.l		(sp)+,d0		/*  load S.F. */
	cmp.l		(tbl_fmul_ovfl.b,pc,d1.w*4),d0 /*  would result ovfl? */
	beq.w		fmul_may_ovfl		/*  result may rnd to overflow */
	blt.w		fmul_ovfl		/*  result will overflow */

	cmp.l		(tbl_fmul_unfl.b,pc,d1.w*4),d0 /*  would result unfl? */
	beq.w		fmul_may_unfl		/*  result may rnd to no unfl */
	bgt.w		fmul_unfl		/*  result will underflow */

/*
 * NORMAL:
 * - the result of the multiply operation will neither overflow nor underflow.
 * - do the multiply to the proper precision and rounding mode.
 * - scale the result exponent using the scale factor. if both operands were
 * normalized then we really don't need to go through this scaling. but for now,
 * this will do.
 */
fmul_normal:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst operand */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmul.x		FP_SCR0(a6),fp0	/*  execute multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fmul_normal_exit:
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
	fmovem.x	FP_SCR0(a6),fp0	/*  return default result in fp0 */
	rts

/*
 * OVERFLOW:
 * - the result of the multiply operation is an overflow.
 * - do the multiply to the proper precision and rounding mode in order to
 * set the inexact bits.
 * - calculate the default result and return it in fp0.
 * - if overflow or inexact is enabled, we need a multiply result rounded to
 * extended precision. if the original operation was extended, then we have this
 * result. if the original operation was single or double, we have to do another
 * multiply using extended precision and the correct rounding mode. the result
 * of this operation then has its exponent scaled by -0x6000 to create the
 * exceptional operand.
 */
fmul_ovfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst operand */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmul.x		FP_SCR0(a6),fp0	/*  execute multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

/*  save setting this until now because this is where fmul_may_ovfl may jump in */
fmul_ovfl_tst:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fmul_ovfl_ena		/*  yes */

/*  calculate the default result */
fmul_ovfl_dis:
	btst		#neg_bit,FPSR_CC(a6)	/*  is result negative? */
	sne		d1			/*  set sign param accordingly */
	move.l		L_SCR3(a6),d0		/*  pass rnd prec,mode */
	bsr.l		ovf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set INF,N if applicable */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */
	rts

/*
 * OVFL is enabled; Create EXOP:
 * - if precision is extended, then we have the EXOP. simply bias the exponent
 * with an extra -0x6000. if the precision is single or double, we need to
 * calculate a result rounded to extended precision.
 */
fmul_ovfl_ena:
	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  test the rnd prec */
	bne.b		fmul_ovfl_ena_sd	/*  it's sgl or dbl */

fmul_ovfl_ena_cont:
	fmovem.x	fp0,FP_SCR0(a6)	/*  move result to stack */

	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.w		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	sub.l		d0,d1			/*  add scale factor */
	subi.l		#0x6000,d1		/*  subtract bias */
	andi.w		#0x7fff,d1		/*  clear sign bit */
	andi.w		#0x8000,d2		/*  keep old sign */
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.b		fmul_ovfl_dis

fmul_ovfl_ena_sd:
	fmovem.x		FP_SCR1(a6),fp0	/*  load dst operand */

	move.l		L_SCR3(a6),d1
	andi.b		#0x30,d1		/*  keep rnd mode only */
	fmove.l		d1,fpcr		/*  set FPCR */

	fmul.x		FP_SCR0(a6),fp0	/*  execute multiply */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	bra.b		fmul_ovfl_ena_cont

/*
 * may OVERFLOW:
 * - the result of the multiply operation MAY overflow.
 * - do the multiply to the proper precision and rounding mode in order to
 * set the inexact bits.
 * - calculate the default result and return it in fp0.
 */
fmul_may_ovfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmul.x		FP_SCR0(a6),fp0	/*  execute multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x2,fp1		/*  is |result| >= 2.b? */
	fbge		fmul_ovfl_tst		/*  yes; overflow has occurred */

/*  no, it didn't overflow; we have correct result */
	bra.w		fmul_normal_exit

/*
 * UNDERFLOW:
 * - the result of the multiply operation is an underflow.
 * - do the multiply to the proper precision and rounding mode in order to
 * set the inexact bits.
 * - calculate the default result and return it in fp0.
 * - if overflow or inexact is enabled, we need a multiply result rounded to
 * extended precision. if the original operation was extended, then we have this
 * result. if the original operation was single or double, we have to do another
 * multiply using extended precision and the correct rounding mode. the result
 * of this operation then has its exponent scaled by -0x6000 to create the
 * exceptional operand.
 */
fmul_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

/*  for fun, let's use only extended precision, round to zero. then, let */
/*  the unf_res() routine figure out all the rest. */
/*  will we get the correct answer. */
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst operand */

	fmove.l		#rz_mode*0x10,fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmul.x		FP_SCR0(a6),fp0	/*  execute multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fmul_unfl_ena		/*  yes */

fmul_unfl_dis:
	fmovem.x	fp0,FP_SCR0(a6)	/*  store out result */

	lea		FP_SCR0(a6),a0	/*  pass: result addr */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  unf_res2 may have set 'Z' */
	fmovem.x	FP_SCR0(a6),fp0	/*  return default result in fp0 */
	rts

/*
 * UNFL is enabled.
 */
fmul_unfl_ena:
	fmovem.x		FP_SCR1(a6),fp1	/*  load dst op */

	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  is precision extended? */
	bne.b		fmul_unfl_ena_sd	/*  no, sgl or dbl */

/*  if the rnd mode is anything but RZ, then we have to re-do the above */
/*  multiplication because we used RZ for all. */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

fmul_unfl_ena_cont:
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmul.x		FP_SCR0(a6),fp1	/*  execute multiply */

	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fmovem.x		fp1,FP_SCR0(a6)	/*  save result to stack */
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
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.w		fmul_unfl_dis

fmul_unfl_ena_sd:
	move.l		L_SCR3(a6),d1
	andi.b		#0x30,d1		/*  use only rnd mode */
	fmove.l		d1,fpcr		/*  set FPCR */

	bra.b		fmul_unfl_ena_cont

/*  MAY UNDERFLOW: */
/*  -use the correct rounding mode and precision. this code favors operations */
/*  that do not underflow. */
fmul_may_unfl:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst operand */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmul.x		FP_SCR0(a6),fp0	/*  execute multiply */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x2,fp1		/*  is |result| > 2.b? */
	fbgt		fmul_normal_exit	/*  no; no underflow occurred */
	fblt		fmul_unfl		/*  yes; underflow occurred */

/*
 * we still don't know if underflow occurred. result is ~ equal to 2. but,
 * we don't know if the result was an underflow that rounded up to a 2 or
 * a normalized number that rounded down to a 2. so, redo the entire operation
 * using RZ as the rounding mode to see what the pre-rounded result is.
 * this case should be relatively rare.
 */
	fmovem.x		FP_SCR1(a6),fp1	/*  load dst operand */

	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  keep rnd prec */
	ori.b		#rz_mode*0x10,d1	/*  insert RZ */

	fmove.l		d1,fpcr		/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmul.x		FP_SCR0(a6),fp1	/*  execute multiply */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fabs.x		fp1			/*  make absolute value */
	fcmp.b		#0x2,fp1		/*  is |result| < 2.b? */
	fbge		fmul_normal_exit	/*  no; no underflow occurred */
	bra.w		fmul_unfl		/*  yes, underflow occurred */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * Multiply: inputs are not both normalized; what are they?
 */
fmul_not_norm:
	move.w		(tbl_fmul_op.b,pc,d1.w*2),d1
	jmp		(tbl_fmul_op.b,pc,d1.w)

	/* swbeg		#48 */
	.dc.w 0x4afc,48
tbl_fmul_op:
	.dc.w		fmul_norm-tbl_fmul_op /*  NORM x NORM */
	.dc.w		fmul_zero-tbl_fmul_op /*  NORM x ZERO */
	.dc.w		fmul_inf_src-tbl_fmul_op /*  NORM x INF */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  NORM x QNAN */
	.dc.w		fmul_norm-tbl_fmul_op /*  NORM x DENORM */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  NORM x SNAN */
	.dc.w		tbl_fmul_op-tbl_fmul_op
	.dc.w		tbl_fmul_op-tbl_fmul_op

	.dc.w		fmul_zero-tbl_fmul_op /*  ZERO x NORM */
	.dc.w		fmul_zero-tbl_fmul_op /*  ZERO x ZERO */
	.dc.w		fmul_res_operr-tbl_fmul_op /*  ZERO x INF */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  ZERO x QNAN */
	.dc.w		fmul_zero-tbl_fmul_op /*  ZERO x DENORM */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  ZERO x SNAN */
	.dc.w		tbl_fmul_op-tbl_fmul_op
	.dc.w		tbl_fmul_op-tbl_fmul_op

	.dc.w		fmul_inf_dst-tbl_fmul_op /*  INF x NORM */
	.dc.w		fmul_res_operr-tbl_fmul_op /*  INF x ZERO */
	.dc.w		fmul_inf_dst-tbl_fmul_op /*  INF x INF */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  INF x QNAN */
	.dc.w		fmul_inf_dst-tbl_fmul_op /*  INF x DENORM */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  INF x SNAN */
	.dc.w		tbl_fmul_op-tbl_fmul_op
	.dc.w		tbl_fmul_op-tbl_fmul_op

	.dc.w		fmul_res_qnan-tbl_fmul_op /*  QNAN x NORM */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  QNAN x ZERO */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  QNAN x INF */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  QNAN x QNAN */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  QNAN x DENORM */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  QNAN x SNAN */
	.dc.w		tbl_fmul_op-tbl_fmul_op
	.dc.w		tbl_fmul_op-tbl_fmul_op

	.dc.w		fmul_norm-tbl_fmul_op /*  NORM x NORM */
	.dc.w		fmul_zero-tbl_fmul_op /*  NORM x ZERO */
	.dc.w		fmul_inf_src-tbl_fmul_op /*  NORM x INF */
	.dc.w		fmul_res_qnan-tbl_fmul_op /*  NORM x QNAN */
	.dc.w		fmul_norm-tbl_fmul_op /*  NORM x DENORM */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  NORM x SNAN */
	.dc.w		tbl_fmul_op-tbl_fmul_op
	.dc.w		tbl_fmul_op-tbl_fmul_op

	.dc.w		fmul_res_snan-tbl_fmul_op /*  SNAN x NORM */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  SNAN x ZERO */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  SNAN x INF */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  SNAN x QNAN */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  SNAN x DENORM */
	.dc.w		fmul_res_snan-tbl_fmul_op /*  SNAN x SNAN */
	.dc.w		tbl_fmul_op-tbl_fmul_op
	.dc.w		tbl_fmul_op-tbl_fmul_op

fmul_res_operr:
	bra.l		res_operr
fmul_res_snan:
	bra.l		res_snan
fmul_res_qnan:
	bra.l		res_qnan

/*
 * Multiply: (Zero x Zero) || (Zero x norm) || (Zero x denorm)
 */
	.globl		fmul_zero		/*  global for fsglmul */
fmul_zero:
	move.b		SRC_EX.w(a0),d0		/*  exclusive or the signs */
	move.b		DST_EX.w(a1),d1
	eor.b		d0,d1
	bpl.b		fmul_zero_p		/*  result ZERO is pos. */
fmul_zero_n:
	fmove.s		#0x80000000,fp0	/*  load -ZERO */
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6) /*  set Z/N */
	rts
fmul_zero_p:
	fmove.s		#0x00000000,fp0	/*  load +ZERO */
	move.b		#z_bmask,FPSR_CC(a6)	/*  set Z */
	rts

/*
 * Multiply: (inf x inf) || (inf x norm) || (inf x denorm)
 *
 * Note: The j-bit for an infinity is a don't-care. However, to be
 * strictly compatible w/ the 68881/882, we make sure to return an
 * INF w/ the j-bit set if the input INF j-bit was set. Destination
 * INFs take priority.
 */
	.globl		fmul_inf_dst		/*  global for fsglmul */
fmul_inf_dst:
	fmovem.x	DST.w(a1),fp0		/*  return INF result in fp0 */
	move.b		SRC_EX.w(a0),d0		/*  exclusive or the signs */
	move.b		DST_EX.w(a1),d1
	eor.b		d0,d1
	bpl.b		fmul_inf_dst_p		/*  result INF is pos. */
fmul_inf_dst_n:
	fabs.x		fp0			/*  clear result sign */
	fneg.x		fp0			/*  set result sign */
	move.b		#inf_bmask+neg_bmask,FPSR_CC(a6) /*  set INF/N */
	rts
fmul_inf_dst_p:
	fabs.x		fp0			/*  clear result sign */
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set INF */
	rts

	.globl		fmul_inf_src		/*  global for fsglmul */
fmul_inf_src:
	fmovem.x	SRC.w(a0),fp0		/*  return INF result in fp0 */
	move.b		SRC_EX.w(a0),d0		/*  exclusive or the signs */
	move.b		DST_EX.w(a1),d1
	eor.b		d0,d1
	bpl.b		fmul_inf_dst_p		/*  result INF is pos. */
	bra.b		fmul_inf_dst_n

