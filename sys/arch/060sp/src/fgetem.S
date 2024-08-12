/*
 *  sgetexp():  returns the exponent portion of the input argument.	
 * 	      The exponent bias is removed and the exponent value is	
 * 	      returned as an extended precision number in fp0.		
 *  sgetexpd(): handles denormalized numbers.				
 * 									
 *  sgetman():  extracts the mantissa of the input argument. The		
 * 	      mantissa is converted to an extended precision number w/	
 * 	      an exponent of $3fff and is returned in fp0. The range of 
 * 	      the result is [1.0 - 2.0).				
 *  sgetmand(): handles denormalized numbers.				
 * 									
 *  INPUT *************************************************************** 
 * 	a0  = pointer to extended precision input			
 * 									
 *  OUTPUT ************************************************************** 
 * 	fp0 = exponent(X) or mantissa(X)				
 * 									
 */

	.include "hdr.fpu"

	.xref norm

	.text

	.globl		sgetexp
sgetexp:
	move.w		SRC_EX.w(a0),d0		/*  get the exponent */
	bclr		#0xf,d0		/*  clear the sign bit */
	subi.w		#0x3fff,d0		/*  subtract off the bias */
	fmove.w		d0,fp0		/*  return exp in fp0 */
	blt.b		sgetexpn		/*  it's negative */
	rts

sgetexpn:
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'N' ccode bit */
	rts

	.globl		sgetexpd
sgetexpd:
	bsr.l		norm			/*  normalize */
	neg.w		d0			/*  new exp = -(shft amt) */
	subi.w		#0x3fff,d0		/*  subtract off the bias */
	fmove.w		d0,fp0		/*  return exp in fp0 */
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'N' ccode bit */
	rts

	.globl		sgetman
sgetman:
	move.w		SRC_EX.w(a0),d0		/*  get the exp */
	ori.w		#0x7fff,d0		/*  clear old exp */
	bclr		#0xe,d0		/*  make it the new exp +-3fff */

/*  here, we build the result in a tmp location so as not to disturb the input */
	move.l		SRC_HI(a0),FP_SCR0_HI(a6) /*  copy to tmp loc */
	move.l		SRC_LO(a0),FP_SCR0_LO(a6) /*  copy to tmp loc */
	move.w		d0,FP_SCR0_EX(a6)	/*  insert new exponent */
	fmove.x		FP_SCR0(a6),fp0	/*  put new value back in fp0 */
	bmi.b		sgetmann		/*  it's negative */
	rts

sgetmann:
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'N' ccode bit */
	rts

/*
 * For denormalized numbers, shift the mantissa until the j-bit = 1,
 * then load the exponent with +/1 $3fff.
 */
	.globl		sgetmand
sgetmand:
	bsr.l		norm			/*  normalize exponent */
	bra.b		sgetman

