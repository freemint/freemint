/*
 *  XDEF ****************************************************************	
 * 	ftst(): emulates the ftest instruction				
 * 									
 *  XREF ****************************************************************	
 * 	res{s,q}nan_1op() - set NAN result for monadic instruction	
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 									
 *  OUTPUT **************************************************************	
 * 	none								
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Check the source operand tag (STAG) and set the FPCR according	
 *  to the operand type and sign.						
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		ftst
ftst:
	move.b		STAG(a6),d1
	bne.b		ftst_not_norm		/*  optimize on non-norm input */

/*
 * Norm:
 */
ftst_norm:
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	bmi.b		ftst_norm_m		/*  yes */
	rts
ftst_norm_m:
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'N' ccode bit */
	rts

/*
 * input is not normalized; what is it?
 */
ftst_not_norm:
	cmpi.b		#ZERO,d1		/*  weed out ZERO */
	beq.b		ftst_zero
	cmpi.b		#INF,d1		/*  weed out INF */
	beq.b		ftst_inf
	cmpi.b		#SNAN,d1		/*  weed out SNAN */
	beq.l		res_snan_1op
	cmpi.b		#QNAN,d1		/*  weed out QNAN */
	beq.l		res_qnan_1op

/*
 * Denorm:
 */
ftst_denorm:
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	bmi.b		ftst_denorm_m		/*  yes */
	rts
ftst_denorm_m:
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'N' ccode bit */
	rts

/*
 * Infinity:
 */
ftst_inf:
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	bmi.b		ftst_inf_m		/*  yes */
ftst_inf_p:
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set 'I' ccode bit */
	rts
ftst_inf_m:
	move.b		#inf_bmask+neg_bmask,FPSR_CC(a6) /*  set 'I','N' ccode bits */
	rts

/*
 * Zero:
 */
ftst_zero:
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	bmi.b		ftst_zero_m		/*  yes */
ftst_zero_p:
	move.b		#z_bmask,FPSR_CC(a6)	/*  set 'N' ccode bit */
	rts
ftst_zero_m:
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6)	/*  set 'Z','N' ccode bits */
	rts

