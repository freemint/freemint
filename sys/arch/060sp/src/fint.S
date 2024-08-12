/*
 *  XDEF ****************************************************************	
 * 	fint(): emulates the fint instruction				
 * 									
 *  XREF ****************************************************************	
 * 	res_{s,q}nan_1op() - set NAN result for monadic operation	
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	d0 = round precision/mode					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = result							
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Separate according to operand type. Unnorms don't pass through	
 *  here. For norms, load the rounding mode/prec, execute a "fint", then	
 *  store the resulting FPSR bits.					
 * 	For denorms, force the j-bit to a one and do the same as for	
 *  norms. Denorms are so low that the answer will either be a zero or a	
 *  one.									
 * 	For zeroes/infs/NANs, return the same while setting the FPSR	
 *  as appropriate.							
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fint
fint:
	move.b		STAG(a6),d1
	bne.b		fint_not_norm		/*  optimize on non-norm input */

/*
 * Norm:
 */
fint_norm:
	andi.b		#0x30,d0		/*  set prec = ext */

	fmove.l		d0,fpcr		/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fint.x		SRC.w(a0),fp0		/*  execute fint */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d0		/*  save FPSR */
	or.l		d0,USER_FPSR(a6)	/*  set exception bits */

	rts

/*
 * input is not normalized; what is it?
 */
fint_not_norm:
	cmpi.b		#ZERO,d1		/*  weed out ZERO */
	beq.b		fint_zero
	cmpi.b		#INF,d1		/*  weed out INF */
	beq.b		fint_inf
	cmpi.b		#DENORM,d1		/*  weed out DENORM */
	beq.b		fint_denorm
	cmpi.b		#SNAN,d1		/*  weed out SNAN */
	beq.l		res_snan_1op
	bra.l		res_qnan_1op		/*  weed out QNAN */

/*
 * Denorm:
 *
 * for DENORMs, the result will be either (+/-)ZERO or (+/-)1.
 * also, the INEX2 and AINEX exception bits will be set.
 * so, we could either set these manually or force the DENORM
 * to a very small NORM and ship it to the NORM routine.
 * I do the latter.
 */
fint_denorm:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6) /*  copy sign, zero exp */
	move.b		#0x80,FP_SCR0_HI(a6)	/*  force DENORM ==> small NORM */
	lea		FP_SCR0(a6),a0
	bra.b		fint_norm

/*
 * Zero:
 */
fint_zero:
	tst.b		SRC_EX.w(a0)		/*  is ZERO negative? */
	bmi.b		fint_zero_m		/*  yes */
fint_zero_p:
	fmove.s		#0x00000000,fp0	/*  return +ZERO in fp0 */
	move.b		#z_bmask,FPSR_CC(a6)	/*  set 'Z' ccode bit */
	rts
fint_zero_m:
	fmove.s		#0x80000000,fp0	/*  return -ZERO in fp0 */
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6) /*  set 'Z','N' ccode bits */
	rts

/*
 * Infinity:
 */
fint_inf:
	fmovem.x	SRC.w(a0),fp0		/*  return result in fp0 */
	tst.b		SRC_EX.w(a0)		/*  is INF negative? */
	bmi.b		fint_inf_m		/*  yes */
fint_inf_p:
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set 'I' ccode bit */
	rts
fint_inf_m:
	move.b		#inf_bmask+neg_bmask,FPSR_CC(a6) /*  set 'N','I' ccode bits */
	rts
