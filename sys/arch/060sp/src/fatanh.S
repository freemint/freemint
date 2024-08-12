/*
 *  satanh():  computes the inverse hyperbolic tangent of a norm input	
 *  satanhd(): computes the inverse hyperbolic tangent of a denorm input	
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = arctanh(X)						
 * 									
 *  ACCURACY and MONOTONICITY *******************************************	
 * 	The returned result is within 3 ulps in	64 significant bit,	
 * 	i.e. within 0.5001 ulp to 53 bits if the result is subsequently	
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM ***********************************************************	
 * 									
 * 	ATANH								
 * 	1. If |X| >= 1, go to 3.					
 * 									
 * 	2. (|X| < 1) Calculate atanh(X) by				
 * 		sgn := sign(X)						
 * 		y := |X|						
 * 		z := 2y/(1-y)						
 * 		atanh(X) := sgn * (1/2) * logp1(z)			
 * 		Exit.							
 * 									
 * 	3. If |X| > 1, go to 5.						
 * 									
 * 	4. (|X| = 1) Generate infinity with an appropriate sign and	
 * 		divide-by-zero by					
 * 		sgn := sign(X)						
 * 		atan(X) := sgn / (+0).					
 * 		Exit.							
 * 									
 * 	5. (|X| > 1) Generate an invalid operation by 0 * infinity.	
 * 		Exit.							
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		satanh
satanh:
	move.l		(a0),d1
	move.w		4(a0),d1
	andi.l		#0x7FFFFFFF,d1
	cmpi.l		#0x3FFF8000,d1
	bge.b		ATANHBIG

/* --THIS IS THE USUAL CASE, |X| < 1 */
/* --Y = |X|, Z = 2Y/(1-Y), ATANH(X) = SIGN(X) * (1/2) * LOG1P(Z). */

	fabs.x		(a0),fp0		/*  Y = |X| */
	fmove.x		fp0,fp1
	fneg.x		fp1			/*  -Y */
	fadd.x		fp0,fp0		/*  2Y */
	fadd.s		#0x3F800000,fp1	/*  1-Y */
	fdiv.x		fp1,fp0		/*  2Y/(1-Y) */
	move.l		(a0),d1
	andi.l		#0x80000000,d1
	ori.l		#0x3F000000,d1		/*  SIGN(X)*HALF */
	move.l		d1,-(sp)

	move.l		d0,-(sp)		/*  save rnd prec,mode */
	clr.l		d0			/*  pass ext prec,RN */
	fmovem.x		fp0,-(sp)		/*  save Z on stack */
	lea		(sp),a0		/*  pass ptr to Z */
	bsr.l		slognp1			/*  LOG1P(Z) */
	add.l		#0xc,sp		/*  clear Z from stack */

	move.l		(sp)+,d0		/*  fetch old prec,mode */
	fmove.l		d0,fpcr		/*  load it */
	move.b		#FMUL_OP,d1		/*  last inst is MUL */
	fmul.s		(sp)+,fp0
	bra.l		t_catch

ATANHBIG:
	fabs.x		(a0),fp0		/*  |X| */
	fcmp.s		#0x3F800000,fp0
	fbgt.l		t_operr
	bra.l		t_dz

	.globl		satanhd
/* --ATANH(X) = X FOR DENORMALIZED X */
satanhd:
	bra.l		t_extdnrm

