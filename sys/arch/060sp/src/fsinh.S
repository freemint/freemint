/*
 *  ssinh():  computes the hyperbolic sine of a normalized input		
 *  ssinhd(): computes the hyperbolic sine of a denormalized input	
 * 									
 *  INPUT *************************************************************** 
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT ************************************************************** 
 * 	fp0 = sinh(X)							
 * 									
 *  ACCURACY and MONOTONICITY *******************************************	
 * 	The returned result is within 3 ulps in 64 significant bit,	
 * 	i.e. within 0.5001 ulp to 53 bits if the result is subsequently 
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM *********************************************************** 
 * 									
 *        SINH								
 *        1. If |X| > 16380 log2, go to 3.				
 * 									
 *        2. (|X| <= 16380 log2) Sinh(X) is obtained by the formula	
 *                y = |X|, sgn = sign(X), and z = expm1(Y),		
 *                sinh(X) = sgn*(1/2)*( z + z/(1+z) ).			
 *           Exit.							
 * 									
 *        3. If |X| > 16480 log2, go to 5.				
 * 									
 *        4. (16380 log2 < |X| <= 16480 log2)				
 *                sinh(X) = sign(X) * exp(|X|)/2.				
 *           However, invoking exp(|X|) may cause premature overflow.	
 *           Thus, we calculate sinh(X) as follows:			
 *              Y       := |X|						
 *              sgn     := sign(X)					
 *              sgnFact := sgn * 2**(16380)				
 *              Y'      := Y - 16381 log2					
 *              sinh(X) := sgnFact * exp(Y').				
 *           Exit.							
 * 									
 *        5. (|X| > 16480 log2) sinh(X) must overflow. Return		
 *           sign(X)*Huge*Huge to generate overflow and an infinity with	
 *           the appropriate sign. Huge is the largest finite number in	
 *           extended format. Exit.					
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		ssinh
ssinh:
	fmove.x		(a0),fp0		/*  LOAD INPUT */

	move.l		(a0),d1
	move.w		4(a0),d1
	move.l		d1,a1			/*  save (compacted) operand */
	andi.l		#0x7FFFFFFF,d1
	cmpi.l		#0x400CB167,d1
	bgt.b		SINHBIG

/* --THIS IS THE USUAL CASE, |X| < 16380 LOG2 */
/* --Y = |X|, Z = EXPM1(Y), SINH(X) = SIGN(X)*(1/2)*( Z + Z/(1+Z) ) */

	fabs.x		fp0			/*  Y = |X| */

	movem.l		d0/a1,-(sp)		/*  {a1/d0} */
	fmovem.x	fp0,-(sp)		/*  save Y on stack */
	lea		(sp),a0		/*  pass ptr to Y */
	clr.l		d0
	bsr.l		setoxm1			/*  FP0 IS Z = EXPM1(Y) */
	add.l		#0xc,sp		/*  clear Y from stack */
	fmove.l		#0,fpcr
	movem.l		(sp)+,d0/a1		/*  {a1/d0} */

	fmove.x		fp0,fp1
	fadd.s		#0x3F800000,fp1	/*  1+Z */
	fmove.x		fp0,-(sp)
	fdiv.x		fp1,fp0		/*  Z/(1+Z) */
	move.l		a1,d1
	andi.l		#0x80000000,d1
	ori.l		#0x3F000000,d1
	fadd.x		(sp)+,fp0
	move.l		d1,-(sp)

	fmove.l		d0,fpcr
	move.b		#FMUL_OP,d1		/*  last inst is MUL */
	fmul.s		(sp)+,fp0		/*  last fp inst - possible exceptions set */
	bra.l		t_catch

SINHBIG:
	cmpi.l		#0x400CB2B3,d1
	bgt.l		t_ovfl
	fabs.x		fp0
	fsub.d		T1(pc),fp0		/*  (|X|-16381LOG2_LEAD) */
	move.l		#0,-(sp)
	move.l		#0x80000000,-(sp)
	move.l		a1,d1
	andi.l		#0x80000000,d1
	ori.l		#0x7FFB0000,d1
	move.l		d1,-(sp)		/*  EXTENDED FMT */
	fsub.d		T2(pc),fp0		/*  |X| - 16381 LOG2, ACCURATE */

	move.l		d0,-(sp)
	clr.l		d0
	fmovem.x		fp0,-(sp)		/*  save fp0 on stack */
	lea		(sp),a0		/*  pass ptr to fp0 */
	bsr.l		setox
	add.l		#0xc,sp		/*  clear fp0 from stack */

	move.l		(sp)+,d0
	fmove.l		d0,fpcr
	move.b		#FMUL_OP,d1		/*  last inst is MUL */
	fmul.x		(sp)+,fp0		/*  possible exception */
	bra.l		t_catch

	.globl		ssinhd
/* --SINH(X) = X FOR DENORMALIZED X */
ssinhd:
	bra.l		t_extdnrm

