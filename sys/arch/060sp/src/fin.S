/*
 *  XDEF ****************************************************************	
 * 	fin(): emulates the fmove instruction				
 * 	fsin(): emulates the fsmove instruction				
 * 	fdin(): emulates the fdmove instruction				
 * 									
 *  XREF ****************************************************************	
 * 	norm() - normalize mantissa for EXOP on denorm			
 * 	scale_to_zero_src() - scale src exponent to zero		
 * 	ovf_res() - return default overflow result			
 * 	unf_res() - return default underflow result			
 * 	res_qnan_1op() - return QNAN result				
 * 	res_snan_1op() - return SNAN result				
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	d0 = round prec/mode						
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = result							
 * 	fp1 = EXOP (if exception occurred)				
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Handle NANs, infinities, and zeroes as special cases. Divide	
 *  norms into extended, single, and double precision.			
 * 	Norms can be emulated w/ a regular fmove instruction. For	
 *  sgl/dbl, must scale exponent and perform an "fmove". Check to see	
 *  if the result would have overflowed/underflowed. If so, use unf_res()	
 *  or ovf_res() to return the default result. Also return EXOP if	
 *  exception is enabled. If no exception, return the default result.	
 * 	Unnorms don't pass through here.				
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fsin
fsin:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#s_mode*0x10,d0	/*  insert sgl precision */
	bra.b		fin

	.globl		fdin
fdin:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#d_mode*0x10,d0	/*  insert dbl precision */

	.globl		fin
fin:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */

	move.b		STAG(a6),d1		/*  fetch src optype tag */
	bne.w		fin_not_norm		/*  optimize on non-norm input */

/*
 * FP MOVE IN: NORMs and DENORMs ONLY!
 */
fin_norm:
	andi.b		#0xc0,d0		/*  is precision extended? */
	bne.w		fin_not_ext		/*  no, so go handle dbl or sgl */

/*
 * precision selected is extended. so...we cannot get an underflow
 * or overflow because of rounding to the correct precision. so...
 * skip the scaling and unscaling...
 */
	tst.b		SRC_EX.w(a0)		/*  is the operand negative? */
	bpl.b		fin_norm_done		/*  no */
	bset		#neg_bit,FPSR_CC(a6)	/*  yes, so set 'N' ccode bit */
fin_norm_done:
	fmovem.x	SRC.w(a0),fp0		/*  return result in fp0 */
	rts

/*
 * for an extended precision DENORM, the UNFL exception bit is set
 * the accrued bit is NOT set in this instance(no inexactness!)
 */
fin_denorm:
	andi.b		#0xc0,d0		/*  is precision extended? */
	bne.w		fin_not_ext		/*  no, so go handle dbl or sgl */

	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */
	tst.b		SRC_EX.w(a0)		/*  is the operand negative? */
	bpl.b		fin_denorm_done		/*  no */
	bset		#neg_bit,FPSR_CC(a6)	/*  yes, so set 'N' ccode bit */
fin_denorm_done:
	fmovem.x	SRC.w(a0),fp0		/*  return result in fp0 */
	btst		#unfl_bit,FPCR_ENABLE(a6) /*  is UNFL enabled? */
	bne.b		fin_denorm_unfl_ena	/*  yes */
	rts

/*
 * the input is an extended DENORM and underflow is enabled in the FPCR.
 * normalize the mantissa and add the bias of 0x6000 to the resulting negative
 * exponent and insert back into the operand.
 */
fin_denorm_unfl_ena:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	lea		FP_SCR0(a6),a0	/*  pass: ptr to operand */
	bsr.l		norm			/*  normalize result */
	neg.w		d0			/*  new exponent = -(shft val) */
	addi.w		#0x6000,d0		/*  add new bias to exponent */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch old sign,exp */
	andi.w		#0x8000,d1		/*  keep old sign */
	andi.w		#0x7fff,d0		/*  clear sign position */
	or.w		d1,d0			/*  concat new exo,old sign */
	move.w		d0,FP_SCR0_EX(a6)	/*  insert new exponent */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	rts

/*
 * operand is to be rounded to single or double precision
 */
fin_not_ext:
	cmpi.b		#s_mode*0x10,d0	/*  separate sgl/dbl prec */
	bne.b		fin_dbl

/*
 * operand is to be rounded to single precision
 */
fin_sgl:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	bsr.l		scale_to_zero_src	/*  calculate scale factor */

	cmpi.l		#0x3fff-0x3f80,d0	/*  will move in underflow? */
	bge.w		fin_sd_unfl		/*  yes; go handle underflow */
	cmpi.l		#0x3fff-0x407e,d0	/*  will move in overflow? */
	beq.w		fin_sd_may_ovfl		/*  maybe; go check */
	blt.w		fin_sd_ovfl		/*  yes; go handle overflow */

/*
 * operand will NOT overflow or underflow when moved into the fp reg file
 */
fin_sd_normal:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fmove.x		FP_SCR0(a6),fp0	/*  perform move */

	fmove.l		fpsr,d1		/*  save FPSR */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fin_sd_normal_exit:
	move.l		d2,-(sp)		/*  save d2 */
	fmovem.x	fp0,FP_SCR0(a6)	/*  store out result */
	move.w		FP_SCR0_EX(a6),d1	/*  load {sgn,exp} */
	move.w		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	sub.l		d0,d1			/*  add scale factor */
	andi.w		#0x8000,d2		/*  keep old sign */
	or.w		d1,d2			/*  concat old sign,new exponent */
	move.w		d2,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x	FP_SCR0(a6),fp0	/*  return result in fp0 */
	rts

/*
 * operand is to be rounded to double precision
 */
fin_dbl:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	bsr.l		scale_to_zero_src	/*  calculate scale factor */

	cmpi.l		#0x3fff-0x3c00,d0	/*  will move in underflow? */
	bge.w		fin_sd_unfl		/*  yes; go handle underflow */
	cmpi.l		#0x3fff-0x43fe,d0	/*  will move in overflow? */
	beq.w		fin_sd_may_ovfl		/*  maybe; go check */
	blt.w		fin_sd_ovfl		/*  yes; go handle overflow */
	bra.w		fin_sd_normal		/*  no; ho handle normalized op */

/*
 * operand WILL underflow when moved in to the fp register file
 */
fin_sd_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	tst.b		FP_SCR0_EX(a6)		/*  is operand negative? */
	bpl.b		fin_sd_unfl_tst
	bset		#neg_bit,FPSR_CC(a6)	/*  set 'N' ccode bit */

/*  if underflow or inexact is enabled, then go calculate the EXOP first. */
fin_sd_unfl_tst:
	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fin_sd_unfl_ena		/*  yes */

fin_sd_unfl_dis:
	lea		FP_SCR0(a6),a0	/*  pass: result addr */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  unf_res may have set 'Z' */
	fmovem.x	FP_SCR0(a6),fp0	/*  return default result in fp0 */
	rts

/*
 * operand will underflow AND underflow or inexact is enabled.
 * Therefore, we must return the result rounded to extended precision.
 */
fin_sd_unfl_ena:
	move.l		FP_SCR0_HI(a6),FP_SCR1_HI(a6)
	move.l		FP_SCR0_LO(a6),FP_SCR1_LO(a6)
	move.w		FP_SCR0_EX(a6),d1	/*  load current exponent */

	move.l		d2,-(sp)		/*  save d2 */
	move.w		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	sub.l		d0,d1			/*  subtract scale factor */
	andi.w		#0x8000,d2		/*  extract old sign */
	addi.l		#0x6000,d1		/*  add new bias */
	andi.w		#0x7fff,d1
	or.w		d1,d2			/*  concat old sign,new exp */
	move.w		d2,FP_SCR1_EX(a6)	/*  insert new exponent */
	fmovem.x	FP_SCR1(a6),fp1	/*  return EXOP in fp1 */
	move.l		(sp)+,d2		/*  restore d2 */
	bra.b		fin_sd_unfl_dis

/*
 * operand WILL overflow.
 */
fin_sd_ovfl:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fmove.x		FP_SCR0(a6),fp0	/*  perform move */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  save FPSR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fin_sd_ovfl_tst:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fin_sd_ovfl_ena		/*  yes */

/*
 * OVFL is not enabled; therefore, we must create the default result by
 * calling ovf_res().
 */
fin_sd_ovfl_dis:
	btst		#neg_bit,FPSR_CC(a6)	/*  is result negative? */
	sne		d1			/*  set sign param accordingly */
	move.l		L_SCR3(a6),d0		/*  pass: prec,mode */
	bsr.l		ovf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set INF,N if applicable */
	fmovem.x		(a0),fp0		/*  return default result in fp0 */
	rts

/*
 * OVFL is enabled.
 * the INEX2 bit has already been updated by the round to the correct precision.
 * now, round to extended(and don't alter the FPSR).
 */
fin_sd_ovfl_ena:
	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	subi.l		#0x6000,d1		/*  subtract bias */
	andi.w		#0x7fff,d1
	or.w		d2,d1
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.b		fin_sd_ovfl_dis

/*
 * the move in MAY overflow. so...
 */
fin_sd_may_ovfl:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fmove.x		FP_SCR0(a6),fp0	/*  perform the move */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x2,fp1		/*  is |result| >= 2.b? */
	fbge		fin_sd_ovfl_tst		/*  yes; overflow has occurred */

/*  no, it didn't overflow; we have correct result */
	bra.w		fin_sd_normal_exit

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * operand is not a NORM: check its optype and branch accordingly
 */
fin_not_norm:
	cmpi.b		#DENORM,d1		/*  weed out DENORM */
	beq.w		fin_denorm
	cmpi.b		#SNAN,d1		/*  weed out SNANs */
	beq.l		res_snan_1op
	cmpi.b		#QNAN,d1		/*  weed out QNANs */
	beq.l		res_qnan_1op

/*
 * do the fmove in; at this point, only possible ops are ZERO and INF.
 * use fmov to determine ccodes.
 * prec:mode should be zero at this point but it won't affect answer anyways.
 */
	fmove.x		SRC.w(a0),fp0		/*  do fmove in */
	fmove.l		fpsr,d0		/*  no exceptions possible */
	rol.l		#0x8,d0		/*  put ccodes in lo byte */
	move.b		d0,FPSR_CC(a6)	/*  insert correct ccodes */
	rts

