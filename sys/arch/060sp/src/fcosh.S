/*
 *  scosh():  computes the hyperbolic cosine of a normalized input	
 *  scoshd(): computes the hyperbolic cosine of a denormalized input	
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = cosh(X)							
 * 									
 *  ACCURACY and MONOTONICITY *******************************************	
 * 	The returned result is within 3 ulps in 64 significant bit,	
 * 	i.e. within 0.5001 ulp to 53 bits if the result is subsequently	
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM ***********************************************************	
 * 									
 * 	COSH								
 * 	1. If |X| > 16380 log2, go to 3.				
 * 									
 * 	2. (|X| <= 16380 log2) Cosh(X) is obtained by the formulae	
 * 		y = |X|, z = exp(Y), and				
 * 		cosh(X) = (1/2)*( z + 1/z ).				
 * 		Exit.							
 * 									
 * 	3. (|X| > 16380 log2). If |X| > 16480 log2, go to 5.		
 * 									
 * 	4. (16380 log2 < |X| <= 16480 log2)				
 * 		cosh(X) = sign(X) * exp(|X|)/2.				
 * 		However, invoking exp(|X|) may cause premature		
 * 		overflow. Thus, we calculate sinh(X) as follows:	
 * 		Y	:= |X|						
 * 		Fact	:=	2**(16380)				
 * 		Y'	:= Y - 16381 log2				
 * 		cosh(X) := Fact * exp(Y').				
 * 		Exit.							
 * 									
 * 	5. (|X| > 16480 log2) sinh(X) must overflow. Return		
 * 		Huge*Huge to generate overflow and an infinity with	
 * 		the appropriate sign. Huge is the largest finite number	
 * 		in extended format. Exit.				
 * 									
 */

	.include "hdr.fpu"

	.text

TWO16380:
	.dc.l		0x7FFB0000,0x80000000,0x00000000,0x00000000

	.globl		scosh
scosh:
	fmove.x		(a0),fp0		/*  LOAD INPUT */

	move.l		(a0),d1
	move.w		4(a0),d1
	andi.l		#0x7FFFFFFF,d1
	cmpi.l		#0x400CB167,d1
	bgt.b		COSHBIG

/* --THIS IS THE USUAL CASE, |X| < 16380 LOG2 */
/* --COSH(X) = (1/2) * ( EXP(X) + 1/EXP(X) ) */

	fabs.x		fp0			/*  |X| */

	move.l		d0,-(sp)
	clr.l		d0
	fmovem.x		fp0,-(sp)		/*  save |X| to stack */
	lea		(sp),a0		/*  pass ptr to |X| */
	bsr.l		setox			/*  FP0 IS EXP(|X|) */
	add.l		#0xc,sp		/*  erase |X| from stack */
	fmul.s		#0x3F000000,fp0	/*  (1/2)EXP(|X|) */
	move.l		(sp)+,d0

	fmove.s		#0x3E800000,fp1	/*  (1/4) */
	fdiv.x		fp0,fp1		/*  1/(2 EXP(|X|)) */

	fmove.l		d0,fpcr
	move.b		#FADD_OP,d1		/*  last inst is ADD */
	fadd.x		fp1,fp0
	bra.l		t_catch

COSHBIG:
	cmpi.l		#0x400CB2B3,d1
	bgt.b		COSHHUGE

	fabs.x		fp0
	fsub.d		T1(pc),fp0		/*  (|X|-16381LOG2_LEAD) */
	fsub.d		T2(pc),fp0		/*  |X| - 16381 LOG2, ACCURATE */

	move.l		d0,-(sp)
	clr.l		d0
	fmovem.x		fp0,-(sp)		/*  save fp0 to stack */
	lea		(sp),a0		/*  pass ptr to fp0 */
	bsr.l		setox
	add.l		#0xc,sp		/*  clear fp0 from stack */
	move.l		(sp)+,d0

	fmove.l		d0,fpcr
	move.b		#FMUL_OP,d1		/*  last inst is MUL */
	fmul.x		TWO16380(pc),fp0
	bra.l		t_catch

COSHHUGE:
	bra.l		t_ovfl2

	.globl		scoshd
/* --COSH(X) = 1 FOR DENORMALIZED X */
scoshd:
	fmove.s		#0x3F800000,fp0

	fmove.l		d0,fpcr
	fadd.s		#0x00800000,fp0
	bra.l		t_pinx2

