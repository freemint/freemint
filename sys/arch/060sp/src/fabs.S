/*
 *  XDEF ****************************************************************	
 * 	fabs():  emulates the fabs instruction				
 * 	fsabs(): emulates the fsabs instruction				
 * 	fdabs(): emulates the fdabs instruction				
 * 									
 *  XREF **************************************************************** 
 * 	norm() - normalize denorm mantissa to provide EXOP		
 * 	scale_to_zero_src() - make exponent. = 0 *  get scale factor	
 * 	unf_res() - calculate underflow result				
 * 	ovf_res() - calculate overflow result				
 * 	res_{s,q}nan_1op() - set NAN result for monadic operation	
 * 									
 *  INPUT *************************************************************** 
 * 	a0 = pointer to extended precision source operand		
 * 	d0 = rnd precision/mode						
 * 									
 *  OUTPUT ************************************************************** 
 * 	fp0 = result							
 * 	fp1 = EXOP (if exception occurred)				
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Handle NANs, infinities, and zeroes as special cases. Divide	
 *  norms into extended, single, and double precision.			
 * 	Simply clear sign for extended precision norm. Ext prec denorm	
 *  gets an EXOP created for it since it's an underflow.			
 * 	Double and single precision can overflow and underflow. First,	
 *  scale the operand such that the exponent is zero. Perform an "fabs"	
 *  using the correct rnd mode/prec. Check to see if the original		
 *  exponent would take an exception. If so, use unf_res() or ovf_res()	
 *  to calculate the default result. Also, create the EXOP for the	
 *  exceptional case. If no exception should occur, insert the correct	
 *  result exponent and return.						
 * 	Unnorms don't pass through here.				
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fsabs
fsabs:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#s_mode*0x10,d0	/*  insert sgl precision */
	bra.b		fabs

	.globl		fdabs
fdabs:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#d_mode*0x10,d0	/*  insert dbl precision */

	.globl		fabs
fabs:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */
	move.b		STAG(a6),d1
	bne.w		fabs_not_norm		/*  optimize on non-norm input */

/*
 *  ABSOLUTE VALUE: norms and denorms ONLY!
 */
fabs_norm:
	andi.b		#0xc0,d0		/*  is precision extended? */
	bne.b		fabs_not_ext		/*  no; go handle sgl or dbl */

/*
 * precision selected is extended. so...we can not get an underflow
 * or overflow because of rounding to the correct precision. so...
 * skip the scaling and unscaling...
 */
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	move.w		SRC_EX.w(a0),d1
	bclr		#15,d1			/*  force absolute value */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert exponent */
	fmovem.x		FP_SCR0(a6),fp0	/*  return result in fp0 */
	rts

/*
 * for an extended precision DENORM, the UNFL exception bit is set
 * the accrued bit is NOT set in this instance(no inexactness!)
 */
fabs_denorm:
	andi.b		#0xc0,d0		/*  is precision extended? */
	bne.b		fabs_not_ext		/*  no */

	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	move.w		SRC_EX.w(a0),d0
	bclr		#15,d0			/*  clear sign */
	move.w		d0,FP_SCR0_EX(a6)	/*  insert exponent */

	fmovem.x		FP_SCR0(a6),fp0	/*  return default result in fp0 */

	btst		#unfl_bit,FPCR_ENABLE(a6) /*  is UNFL enabled? */
	bne.b		fabs_ext_unfl_ena
	rts

/*
 * the input is an extended DENORM and underflow is enabled in the FPCR.
 * normalize the mantissa and add the bias of 0x6000 to the resulting negative
 * exponent and insert back into the operand.
 */
fabs_ext_unfl_ena:
	lea		FP_SCR0(a6),a0	/*  pass: ptr to operand */
	bsr.l		norm			/*  normalize result */
	neg.w		d0			/*  new exponent = -(shft val) */
	addi.w		#0x6000,d0		/*  add new bias to exponent */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch old sign,exp */
	andi.w		#0x8000,d1		/*  keep old sign */
	andi.w		#0x7fff,d0		/*  clear sign position */
	or.w		d1,d0			/*  concat old sign, new exponent */
	move.w		d0,FP_SCR0_EX(a6)	/*  insert new exponent */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	rts

/*
 * operand is either single or double
 */
fabs_not_ext:
	cmpi.b		#s_mode*0x10,d0	/*  separate sgl/dbl prec */
	bne.b		fabs_dbl

/*
 * operand is to be rounded to single precision
 */
fabs_sgl:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	bsr.l		scale_to_zero_src	/*  calculate scale factor */

	cmpi.l		#0x3fff-0x3f80,d0	/*  will move in underflow? */
	bge.w		fabs_sd_unfl		/*  yes; go handle underflow */
	cmpi.l		#0x3fff-0x407e,d0	/*  will move in overflow? */
	beq.w		fabs_sd_may_ovfl	/*  maybe; go check */
	blt.w		fabs_sd_ovfl		/*  yes; go handle overflow */

/*
 * operand will NOT overflow or underflow when moved in to the fp reg file
 */
fabs_sd_normal:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fabs.x		FP_SCR0(a6),fp0	/*  perform absolute */

	fmove.l		fpsr,d1		/*  save FPSR */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fabs_sd_normal_exit:
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
fabs_dbl:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	bsr.l		scale_to_zero_src	/*  calculate scale factor */

	cmpi.l		#0x3fff-0x3c00,d0	/*  will move in underflow? */
	bge.b		fabs_sd_unfl		/*  yes; go handle underflow */
	cmpi.l		#0x3fff-0x43fe,d0	/*  will move in overflow? */
	beq.w		fabs_sd_may_ovfl	/*  maybe; go check */
	blt.w		fabs_sd_ovfl		/*  yes; go handle overflow */
	bra.w		fabs_sd_normal		/*  no; ho handle normalized op */

/*
 * operand WILL underflow when moved in to the fp register file
 */
fabs_sd_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	bclr		#0x7,FP_SCR0_EX(a6)	/*  force absolute value */

/*  if underflow or inexact is enabled, go calculate EXOP first. */
	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fabs_sd_unfl_ena	/*  yes */

fabs_sd_unfl_dis:
	lea		FP_SCR0(a6),a0	/*  pass: result addr */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set possible 'Z' ccode */
	fmovem.x	FP_SCR0(a6),fp0	/*  return default result in fp0 */
	rts

/*
 * operand will underflow AND underflow is enabled.
 * Therefore, we must return the result rounded to extended precision.
 */
fabs_sd_unfl_ena:
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
	fmovem.x	FP_SCR1(a6),fp1	/*  return EXOP in fp1 */
	move.l		(sp)+,d2		/*  restore d2 */
	bra.b		fabs_sd_unfl_dis

/*
 * operand WILL overflow.
 */
fabs_sd_ovfl:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fabs.x		FP_SCR0(a6),fp0	/*  perform absolute */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  save FPSR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fabs_sd_ovfl_tst:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fabs_sd_ovfl_ena	/*  yes */

/*
 * OVFL is not enabled; therefore, we must create the default result by
 * calling ovf_res().
 */
fabs_sd_ovfl_dis:
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
fabs_sd_ovfl_ena:
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
	fmovem.x	FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	move.l		(sp)+,d2		/*  restore d2 */
	bra.b		fabs_sd_ovfl_dis

/*
 * the move in MAY underflow. so...
 */
fabs_sd_may_ovfl:
	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fabs.x		FP_SCR0(a6),fp0	/*  perform absolute */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x2,fp1		/*  is |result| >= 2.b? */
	fbge		fabs_sd_ovfl_tst	/*  yes; overflow has occurred */

/*  no, it didn't overflow; we have correct result */
	bra.w		fabs_sd_normal_exit

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * input is not normalized; what is it?
 */
fabs_not_norm:
	cmpi.b		#DENORM,d1		/*  weed out DENORM */
	beq.w		fabs_denorm
	cmpi.b		#SNAN,d1		/*  weed out SNAN */
	beq.l		res_snan_1op
	cmpi.b		#QNAN,d1		/*  weed out QNAN */
	beq.l		res_qnan_1op

	fabs.x		SRC.w(a0),fp0		/*  force absolute value */

	cmpi.b		#INF,d1		/*  weed out INF */
	beq.b		fabs_inf
fabs_zero:
	move.b		#z_bmask,FPSR_CC(a6)	/*  set 'Z' ccode bit */
	rts
fabs_inf:
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set 'I' ccode bit */
	rts
