/*
 *  stanh():  computes the hyperbolic tangent of a normalized input	
 *  stanhd(): computes the hyperbolic tangent of a denormalized input	
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = tanh(X)							
 * 									
 *  ACCURACY and MONOTONICITY *******************************************	
 * 	The returned result is within 3 ulps in 64 significant bit,	
 * 	i.e. within 0.5001 ulp to 53 bits if the result is subsequently 
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM ***********************************************************	
 * 									
 * 	TANH								
 * 	1. If |X| >= (5/2) log2 or |X| <= 2**(-40), go to 3.		
 * 									
 * 	2. (2**(-40) < |X| < (5/2) log2) Calculate tanh(X) by		
 * 		sgn := sign(X), y := 2|X|, z := expm1(Y), and		
 * 		tanh(X) = sgn*( z/(2+z) ).				
 * 		Exit.							
 * 									
 * 	3. (|X| <= 2**(-40) or |X| >= (5/2) log2). If |X| < 1,		
 * 		go to 7.						
 * 									
 * 	4. (|X| >= (5/2) log2) If |X| >= 50 log2, go to 6.		
 * 									
 * 	5. ((5/2) log2 <= |X| < 50 log2) Calculate tanh(X) by		
 * 		sgn := sign(X), y := 2|X|, z := exp(Y),			
 * 		tanh(X) = sgn - [ sgn*2/(1+z) ].			
 * 		Exit.							
 * 									
 * 	6. (|X| >= 50 log2) Tanh(X) = +-1 (round to nearest). Thus, we	
 * 		calculate Tanh(X) by					
 * 		sgn := sign(X), Tiny := 2**(-126),			
 * 		tanh(X) := sgn - sgn*Tiny.				
 * 		Exit.							
 * 									
 * 	7. (|X| < 2**(-40)). Tanh(X) = X.	Exit.			
 * 									
 */

	.include "hdr.fpu"

	X = FP_SCR0
	XFRAC = X+4

	SGN = L_SCR3

	V = FP_SCR0

	.text

	.globl		stanh
stanh:
	fmove.x		(a0),fp0		/*  LOAD INPUT */

	fmove.x		fp0,X(a6)
	move.l		(a0),d1
	move.w		4(a0),d1
	move.l		d1,X(a6)
	andi.l		#0x7FFFFFFF,d1
	cmpi.l		#0x3fd78000,d1	/*  is |X| < 2^(-40)? */
	blt.w		TANHBORS		/*  yes */
	cmpi.l		#0x3fffddce,d1	/*  is |X| > (5/2)LOG2? */
	bgt.w		TANHBORS		/*  yes */

/* --THIS IS THE USUAL CASE */
/* --Y = 2|X|, Z = EXPM1(Y), TANH(X) = SIGN(X) * Z / (Z+2). */

	move.l		X(a6),d1
	move.l		d1,SGN(a6)
	andi.l		#0x7FFF0000,d1
	addi.l		#0x00010000,d1		/*  EXPONENT OF 2|X| */
	move.l		d1,X(a6)
	andi.l		#0x80000000,SGN(a6)
	fmove.x		X(a6),fp0		/*  FP0 IS Y = 2|X| */

	move.l		d0,-(sp)
	clr.l		d0
	fmovem.x		fp0,-(sp)		/*  save Y on stack */
	lea		(sp),a0		/*  pass ptr to Y */
	bsr.l		setoxm1			/*  FP0 IS Z = EXPM1(Y) */
	add.l		#0xc,sp		/*  clear Y from stack */
	move.l		(sp)+,d0

	fmove.x		fp0,fp1
	fadd.s		#0x40000000,fp1	/*  Z+2 */
	move.l		SGN(a6),d1
	fmove.x		fp1,V(a6)
	eor.l		d1,V(a6)

	fmove.l		d0,fpcr		/*  restore users round prec,mode */
	fdiv.x		V(a6),fp0
	bra.l		t_inx2

TANHBORS:
	cmpi.l		#0x3FFF8000,d1
	blt.w		TANHSM

	cmpi.l		#0x40048AA1,d1
	bgt.w		TANHHUGE

/* -- (5/2) LOG2 < |X| < 50 LOG2, */
/* --TANH(X) = 1 - (2/[EXP(2X)+1]). LET Y = 2|X|, SGN = SIGN(X), */
/* --TANH(X) = SGN -	SGN*2/[EXP(Y)+1]. */

	move.l		X(a6),d1
	move.l		d1,SGN(a6)
	andi.l		#0x7FFF0000,d1
	addi.l		#0x00010000,d1		/*  EXPO OF 2|X| */
	move.l		d1,X(a6)		/*  Y = 2|X| */
	andi.l		#0x80000000,SGN(a6)
	move.l		SGN(a6),d1
	fmove.x		X(a6),fp0		/*  Y = 2|X| */

	move.l		d0,-(sp)
	clr.l		d0
	fmovem.x		fp0,-(sp)		/*  save Y on stack */
	lea		(sp),a0		/*  pass ptr to Y */
	bsr.l		setox			/*  FP0 IS EXP(Y) */
	add.l		#0xc,sp		/*  clear Y from stack */
	move.l		(sp)+,d0
	move.l		SGN(a6),d1
	fadd.s		#0x3F800000,fp0	/*  EXP(Y)+1 */

	eori.l		#0xC0000000,d1		/*  -SIGN(X)*2 */
	fmove.s		d1,fp1		/*  -SIGN(X)*2 IN SGL FMT */
	fdiv.x		fp0,fp1		/*  -SIGN(X)2 / [EXP(Y)+1 ] */

	move.l		SGN(a6),d1
	ori.l		#0x3F800000,d1		/*  SGN */
	fmove.s		d1,fp0		/*  SGN IN SGL FMT */

	fmove.l		d0,fpcr		/*  restore users round prec,mode */
	move.b		#FADD_OP,d1		/*  last inst is ADD */
	fadd.x		fp1,fp0
	bra.l		t_inx2

TANHSM:
	fmove.l		d0,fpcr		/*  restore users round prec,mode */
	move.b		#FMOV_OP,d1		/*  last inst is MOVE */
	fmove.x		X(a6),fp0		/*  last inst - possible exception set */
	bra.l		t_catch

/* ---RETURN SGN(X) - SGN(X)EPS */
TANHHUGE:
	move.l		X(a6),d1
	andi.l		#0x80000000,d1
	ori.l		#0x3F800000,d1
	fmove.s		d1,fp0
	andi.l		#0x80000000,d1
	eori.l		#0x80800000,d1		/*  -SIGN(X)*EPS */

	fmove.l		d0,fpcr		/*  restore users round prec,mode */
	fadd.s		d1,fp0
	bra.l		t_inx2

	.globl		stanhd
/* --TANH(X) = X FOR DENORMALIZED X */
stanhd:
	bra.l		t_extdnrm

