/*
 *  XDEF ****************************************************************	
 * 	fsqrt(): emulates the fsqrt instruction				
 * 	fssqrt(): emulates the fssqrt instruction			
 * 	fdsqrt(): emulates the fdsqrt instruction			
 * 									
 *  XREF ****************************************************************	
 * 	scale_sqrt() - scale the source operand				
 * 	unf_res() - return default underflow result			
 * 	ovf_res() - return default overflow result			
 * 	res_qnan_1op() - return QNAN result				
 * 	res_snan_1op() - return SNAN result				
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	d0  rnd prec,mode						
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = result							
 * 	fp1 = EXOP (if exception occurred)				
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Handle NANs, infinities, and zeroes as special cases. Divide	
 *  norms/denorms into ext/sgl/dbl precision.				
 * 	For norms/denorms, scale the exponents such that a sqrt		
 *  instruction won't cause an exception. Use the regular fsqrt to	
 *  compute a result. Check if the regular operands would have taken	
 *  an exception. If so, return the default overflow/underflow result	
 *  and return the EXOP if exceptions are enabled. Else, scale the	
 *  result operand to the proper exponent.				
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fssqrt
fssqrt:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#s_mode*0x10,d0	/*  insert sgl precision */
	bra.b		fsqrt

	.globl		fdsqrt
fdsqrt:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#d_mode*0x10,d0	/*  insert dbl precision */

	.globl		fsqrt
fsqrt:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */
	clr.w		d1
	move.b		STAG(a6),d1
	bne.w		fsqrt_not_norm		/*  optimize on non-norm input */

/*
 * SQUARE ROOT: norms and denorms ONLY!
 */
fsqrt_norm:
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	bmi.l		res_operr		/*  yes */

	andi.b		#0xc0,d0		/*  is precision extended? */
	bne.b		fsqrt_not_ext		/*  no; go handle sgl or dbl */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsqrt.x		(a0),fp0		/*  execute square root */

	fmove.l		fpsr,d1
	or.l		d1,USER_FPSR(a6)	/*  set N,INEX */

	rts

fsqrt_denorm:
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	bmi.l		res_operr		/*  yes */

	andi.b		#0xc0,d0		/*  is precision extended? */
	bne.b		fsqrt_not_ext		/*  no; go handle sgl or dbl */

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	bsr.l		scale_sqrt		/*  calculate scale factor */

	bra.w		fsqrt_sd_normal

/*
 * operand is either single or double
 */
fsqrt_not_ext:
	cmpi.b		#s_mode*0x10,d0	/*  separate sgl/dbl prec */
	bne.w		fsqrt_dbl

/*
 * operand is to be rounded to single precision
 */
fsqrt_sgl:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	bsr.l		scale_sqrt		/*  calculate scale factor */

	cmpi.l		#0x3fff-0x3f81,d0	/*  will move in underflow? */
	beq.w		fsqrt_sd_may_unfl
	bgt.w		fsqrt_sd_unfl		/*  yes; go handle underflow */
	cmpi.l		#0x3fff-0x407f,d0	/*  will move in overflow? */
	beq.w		fsqrt_sd_may_ovfl	/*  maybe; go check */
	blt.w		fsqrt_sd_ovfl		/*  yes; go handle overflow */

/*
 * operand will NOT overflow or underflow when moved in to the fp reg file
 */
fsqrt_sd_normal:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fsqrt.x		FP_SCR0(a6),fp0	/*  perform absolute */

	fmove.l		fpsr,d1		/*  save FPSR */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fsqrt_sd_normal_exit:
	move.l		d2,-(sp)		/*  save d2 */
	fmovem.x	fp0,FP_SCR0(a6)	/*  store out result */
	move.w		FP_SCR0_EX(a6),d1	/*  load sgn,exp */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	sub.l		d0,d1			/*  add scale factor */
	andi.w		#0x8000,d2		/*  keep old sign */
	or.w		d1,d2			/*  concat old sign,new exp */
	move.w		d2,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x	FP_SCR0(a6),fp0	/*  return result in fp0 */
	rts

/*
 * operand is to be rounded to double precision
 */
fsqrt_dbl:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	bsr.l		scale_sqrt		/*  calculate scale factor */

	cmpi.l		#0x3fff-0x3c01,d0	/*  will move in underflow? */
	beq.w		fsqrt_sd_may_unfl
	bgt.b		fsqrt_sd_unfl		/*  yes; go handle underflow */
	cmpi.l		#0x3fff-0x43ff,d0	/*  will move in overflow? */
	beq.w		fsqrt_sd_may_ovfl	/*  maybe; go check */
	blt.w		fsqrt_sd_ovfl		/*  yes; go handle overflow */
	bra.w		fsqrt_sd_normal		/*  no; ho handle normalized op */

/*  we're on the line here and the distinguising characteristic is whether */
/*  the exponent is 3fff or 3ffe. if it's 3ffe, then it's a safe number */
/*  elsewise fall through to underflow. */
fsqrt_sd_may_unfl:
	btst		#0x0,1+FP_SCR0_EX(a6)	/*  is exponent 0x3fff? */
	bne.w		fsqrt_sd_normal		/*  yes, so no underflow */

/*
 * operand WILL underflow when moved in to the fp register file
 */
fsqrt_sd_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	fmove.l		#rz_mode*0x10,fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsqrt.x		FP_SCR0(a6),fp0	/*  execute square root */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

/*  if underflow or inexact is enabled, go calculate EXOP first. */
	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fsqrt_sd_unfl_ena	/*  yes */

fsqrt_sd_unfl_dis:
	fmovem.x		fp0,FP_SCR0(a6)	/*  store out result */

	lea		FP_SCR0(a6),a0	/*  pass: result addr */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set possible 'Z' ccode */
	fmovem.x		FP_SCR0(a6),fp0	/*  return default result in fp0 */
	rts

/*
 * operand will underflow AND underflow is enabled.
 * Therefore, we must return the result rounded to extended precision.
 */
fsqrt_sd_unfl_ena:
	move.l		FP_SCR0_HI(a6),FP_SCR1_HI(a6)
	move.l		FP_SCR0_LO(a6),FP_SCR1_LO(a6)
	move.w		FP_SCR0_EX(a6),d1	/*  load current exponent */

	move.l		d2,-(sp)		/*  save d2 */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  subtract scale factor */
	addi.l		#0x6000,d1		/*  add new bias */
	andi.w		#0x7fff,d1
	or.w		d2,d1			/*  concat new sign,new exp */
	move.w		d1,FP_SCR1_EX(a6)	/*  insert new exp */
	fmovem.x		FP_SCR1(a6),fp1	/*  return EXOP in fp1 */
	move.l		(sp)+,d2		/*  restore d2 */
	bra.b		fsqrt_sd_unfl_dis

/*
 * operand WILL overflow.
 */
fsqrt_sd_ovfl:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fsqrt.x		FP_SCR0(a6),fp0	/*  perform square root */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  save FPSR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fsqrt_sd_ovfl_tst:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fsqrt_sd_ovfl_ena	/*  yes */

/*
 * OVFL is not enabled; therefore, we must create the default result by
 * calling ovf_res().
 */
fsqrt_sd_ovfl_dis:
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
fsqrt_sd_ovfl_ena:
	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	subi.l		#0x6000,d1		/*  subtract bias */
	andi.w		#0x7fff,d1
	or.w		d2,d1			/*  concat sign,exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	move.l		(sp)+,d2		/*  restore d2 */
	bra.b		fsqrt_sd_ovfl_dis

/*
 * the move in MAY underflow. so...
 */
fsqrt_sd_may_ovfl:
	btst		#0x0,1+FP_SCR0_EX(a6)	/*  is exponent 0x3fff? */
	bne.w		fsqrt_sd_ovfl		/*  yes, so overflow */

	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fsqrt.x		FP_SCR0(a6),fp0	/*  perform absolute */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fmove.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x1,fp1		/*  is |result| >= 1.b? */
	fbge		fsqrt_sd_ovfl_tst	/*  yes; overflow has occurred */

/*  no, it didn't overflow; we have correct result */
	bra.w		fsqrt_sd_normal_exit

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * input is not normalized; what is it?
 */
fsqrt_not_norm:
	cmpi.b		#DENORM,d1		/*  weed out DENORM */
	beq.w		fsqrt_denorm
	cmpi.b		#ZERO,d1		/*  weed out ZERO */
	beq.b		fsqrt_zero
	cmpi.b		#INF,d1		/*  weed out INF */
	beq.b		fsqrt_inf
	cmpi.b		#SNAN,d1		/*  weed out SNAN */
	beq.l		res_snan_1op
	bra.l		res_qnan_1op

/*
 *	fsqrt(+0) = +0
 *	fsqrt(-0) = -0
 *	fsqrt(+INF) = +INF
 *	fsqrt(-INF) = OPERR
 */
fsqrt_zero:
	tst.b		SRC_EX.w(a0)		/*  is ZERO positive or negative? */
	bmi.b		fsqrt_zero_m		/*  negative */
fsqrt_zero_p:
	fmove.s		#0x00000000,fp0	/*  return +ZERO */
	move.b		#z_bmask,FPSR_CC(a6)	/*  set 'Z' ccode bit */
	rts
fsqrt_zero_m:
	fmove.s		#0x80000000,fp0	/*  return -ZERO */
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6)	/*  set 'Z','N' ccode bits */
	rts

fsqrt_inf:
	tst.b		SRC_EX.w(a0)		/*  is INF positive or negative? */
	bmi.l		res_operr		/*  negative */
fsqrt_inf_p:
	fmovem.x		SRC.w(a0),fp0		/*  return +INF in fp0 */
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set 'I' ccode bit */
	rts
