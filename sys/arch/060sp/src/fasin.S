/*
 *  sasin():  computes the inverse sine of a normalized input		
 *  sasind(): computes the inverse sine of a denormalized input		
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = arcsin(X)							
 * 									
 *  ACCURACY and MONOTONICITY *******************************************	
 * 	The returned result is within 3 ulps in	64 significant bit,	
 * 	i.e. within 0.5001 ulp to 53 bits if the result is subsequently	
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM ***********************************************************	
 * 									
 * 	ASIN								
 * 	1. If |X| >= 1, go to 3.					
 * 									
 * 	2. (|X| < 1) Calculate asin(X) by				
 * 		z := sqrt( [1-X][1+X] )					
 * 		asin(X) = atan( x / z ).				
 * 		Exit.							
 * 									
 * 	3. If |X| > 1, go to 5.						
 * 									
 * 	4. (|X| = 1) sgn := sign(X), return asin(X) := sgn * Pi/2. Exit.
 * 									
 * 	5. (|X| > 1) Generate an invalid operation by 0 * infinity.	
 * 		Exit.							
 * 									
 */

	.include "hdr.fpu"

	.xref satan
	.xref t_extdnrm
	.xref t_inx2
	.xref t_catch
	.xref t_pinx2
	.xref t_minx2
	.xref t_operr
	.xref TWOBYPI
	.xref PIBY2

	.text

	.globl		sasin
sasin:
	fmove.x		(a0),fp0		/*  LOAD INPUT */

	move.l		(a0),d1
	move.w		4(a0),d1
	andi.l		#0x7FFFFFFF,d1
	cmpi.l		#0x3FFF8000,d1
	bge.b		ASINBIG

/*  This catch is added here for the '060 QSP. Originally, the call to */
/*  satan() would handle this case by causing the exception which would */
/*  not be caught until gen_except(). Now, with the exceptions being */
/*  detected inside of satan(), the exception would have been handled there */
/*  instead of inside sasin() as expected. */
	cmpi.l		#0x3FD78000,d1
	blt.w		ASINTINY

/* --THIS IS THE USUAL CASE, |X| < 1 */
/* --ASIN(X) = ATAN( X / SQRT( (1-X)(1+X) ) ) */

ASINMAIN:
	fmove.s		#0x3F800000,fp1
	fsub.x		fp0,fp1		/*  1-X */
	fmovem.x		fp2,-(sp)		/*   {fp2} */
	fmove.s		#0x3F800000,fp2
	fadd.x		fp0,fp2		/*  1+X */
	fmul.x		fp2,fp1		/*  (1+X)(1-X) */
	fmovem.x		(sp)+,fp2		/*   {fp2} */
	fsqrt.x		fp1			/*  SQRT([1-X][1+X]) */
	fdiv.x		fp1,fp0		/*  X/SQRT([1-X][1+X]) */
	fmovem.x		fp0,-(sp)		/*  save X/SQRT(...) */
	lea		(sp),a0		/*  pass ptr to X/SQRT(...) */
	bsr.l		satan
	add.l		#0xc,sp		/*  clear X/SQRT(...) from stack */
	bra.l		t_inx2

ASINBIG:
	fabs.x		fp0			/*  |X| */
	fcmp.s		#0x3F800000,fp0
	fbgt.l		t_operr			/*  cause an operr exception */

/* --|X| = 1, ASIN(X) = +- PI/2. */
ASINONE:
	fmove.x		PIBY2(pc),fp0
	move.l		(a0),d1
	andi.l		#0x80000000,d1		/*  SIGN BIT OF X */
	ori.l		#0x3F800000,d1		/*  +-1 IN SGL FORMAT */
	move.l		d1,-(sp)		/*  push SIGN(X) IN SGL-FMT */
	fmove.l		d0,fpcr
	fmul.s		(sp)+,fp0
	bra.l		t_inx2

/* --|X| < 2^(-40), ATAN(X) = X */
ASINTINY:
	fmove.l		d0,fpcr		/*  restore users rnd mode,prec */
	move.b		#FMOV_OP,d1		/*  last inst is MOVE */
	fmove.x		(a0),fp0		/*  last inst - possible exception */
	bra.l		t_catch

	.globl		sasind
/* --ASIN(X) = X FOR DENORMALIZED X */
sasind:
	bra.l		t_extdnrm

