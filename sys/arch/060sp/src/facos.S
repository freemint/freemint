/*
 *  sacos():  computes the inverse cosine of a normalized input		
 *  sacosd(): computes the inverse cosine of a denormalized input		
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT ************************************************************** 
 * 	fp0 = arccos(X)							
 * 									
 *  ACCURACY and MONOTONICITY *******************************************	
 * 	The returned result is within 3 ulps in	64 significant bit,	
 * 	i.e. within 0.5001 ulp to 53 bits if the result is subsequently	
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM *********************************************************** 
 * 									
 * 	ACOS								
 * 	1. If |X| >= 1, go to 3.					
 * 									
 * 	2. (|X| < 1) Calculate acos(X) by				
 * 		z := (1-X) / (1+X)					
 * 		acos(X) = 2 * atan( sqrt(z) ).				
 * 		Exit.							
 * 									
 * 	3. If |X| > 1, go to 5.						
 * 									
 * 	4. (|X| = 1) If X > 0, return 0. Otherwise, return Pi. Exit.	
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
	.xref ld_pzero
	.xref TWOBYPI
	.xref PIBY2
	.xref PI

	.text

	.globl		sacos
sacos:
	fmove.x		(a0),fp0		/*  LOAD INPUT */

	move.l		(a0),d1		/*  pack exp w/ upper 16 fraction */
	move.w		4(a0),d1
	andi.l		#0x7FFFFFFF,d1
	cmpi.l		#0x3FFF8000,d1
	bge.b		ACOSBIG

/* --THIS IS THE USUAL CASE, |X| < 1 */
/* --ACOS(X) = 2 * ATAN(	SQRT( (1-X)/(1+X) ) ) */

ACOSMAIN:
	fmove.s		#0x3F800000,fp1
	fadd.x		fp0,fp1		/*  1+X */
	fneg.x		fp0			/*  -X */
	fadd.s		#0x3F800000,fp0	/*  1-X */
	fdiv.x		fp1,fp0		/*  (1-X)/(1+X) */
	fsqrt.x		fp0			/*  SQRT((1-X)/(1+X)) */
	move.l		d0,-(sp)		/*  save original users fpcr */
	clr.l		d0
	fmovem.x		fp0,-(sp)		/*  save SQRT(...) to stack */
	lea		(sp),a0		/*  pass ptr to sqrt */
	bsr.l		satan			/*  ATAN(SQRT([1-X]/[1+X])) */
	add.l		#0xc,sp		/*  clear SQRT(...) from stack */

	fmove.l		(sp)+,fpcr		/*  restore users round prec,mode */
	fadd.x		fp0,fp0		/*  2 * ATAN( STUFF ) */
	bra.l		t_pinx2

ACOSBIG:
	fabs.x		fp0
	fcmp.s		#0x3F800000,fp0
	fbgt.l		t_operr			/*  cause an operr exception */

/* --|X| = 1, ACOS(X) = 0 OR PI */
	tst.b		(a0)			/*  is X positive or negative? */
	bpl.b		ACOSP1

/* --X = -1 */
/* Returns PI and inexact exception */
ACOSM1:
	fmove.x		PI(pc),fp0		/*  load PI */
	fmove.l		d0,fpcr		/*  load round mode,prec */
	fadd.s		#0x00800000,fp0	/*  add a small value */
	bra.l		t_pinx2

ACOSP1:
	bra.l		ld_pzero		/*  answer is positive zero */

	.globl		sacosd
/* --ACOS(X) = PI/2 FOR DENORMALIZED X */
sacosd:
	fmove.l		d0,fpcr		/*  load user's rnd mode/prec */
	fmove.x		PIBY2(pc),fp0
	bra.l		t_pinx2

