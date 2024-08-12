/*
 * 	sasin.sa 3.3 12/19/90
 *
 * 	Description: The entry point sAsin computes the inverse sine of
 * 		an input argument; sAsind does the same except for denormalized
 * 		input.
 *
 * 	Input: Double-extended number X in location pointed to
 * 		by address register a0.
 *
 * 	Output: The value arcsin(X) returned in floating-point register Fp0.
 *
 * 	Accuracy and Monotonicity: The returned result is within 3 ulps in
 * 		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
 * 		result is subsequently rounded to double precision. The
 * 		result is provably monotonic in double precision.
 *
 * 	Speed: The program sASIN takes approximately 310 cycles.
 *
 * 	Algorithm:
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
 *
 * 		Copyright (C) Motorola, Inc. 1990
 * 			All Rights Reserved
 *
 *        For details on the license for this file, please see the
 *        file, README, in this same directory.
 */

#include "mint/asmdefs.h"

	.xref	t_operr
	.xref	t_frcinx
	.xref	t_extdnrm
	.xref	satan

	.xref PIBY2

	.text

	.globl	sasind
sasind:
/* --ASIN(X) = X FOR DENORMALIZED X */

	bra		t_extdnrm

	.globl	sasin
sasin:
	fmove.x		(a0),fp0	/*  ...LOAD INPUT */

	move.l		(a0),d0
	move.w		4(a0),d0
	andi.l		#0x7FFFFFFF,d0
	cmpi.l		#0x3FFF8000,d0
	bge.s		asinbig

/* --THIS IS THE USUAL CASE, |X| < 1 */
/* --ASIN(X) = ATAN( X / SQRT( (1-X)(1+X) ) ) */

	.IFNE GAS
	fmove.s		#0x3F800000,fp1
	.ELSE
	fmove.s		#1.0,fp1
	.ENDC
	fsub.x		fp0,fp1		/*  ...1-X */
	fmovem.x	fp2-fp2,-(a7)
	.IFNE GAS
	fmove.s		#0x3F800000,fp2
	.ELSE
	fmove.s		#1.0,fp2
	.ENDC
	fadd.x		fp0,fp2		/*  ...1+X */
	fmul.x		fp2,fp1		/*  ...(1+X)(1-X) */
	fmovem.x	(a7)+,fp2-fp2
	fsqrt.x		fp1		/*  ...SQRT([1-X][1+X]) */
	fdiv.x		fp1,fp0		/*  ...X/SQRT([1-X][1+X]) */
	fmovem.x	fp0-fp0,(a0)
	bsr			satan
	bra			t_frcinx

asinbig:
	fabs.x		fp0	 /*  ...|X| */
	.IFNE GAS
	fcmp.s		#0x3F800000,fp0
	.ELSE
	fcmp.s		#1.0,fp0
	.ENDC
	fbgt		t_operr		/* cause an operr exception */

/* --|X| = 1, ASIN(X) = +- PI/2. */

	fmove.x		PIBY2.w(pc),fp0
	move.l		(a0),d0
	andi.l		#0x80000000,d0	/*  ...SIGN BIT OF X */
	ori.l		#0x3F800000,d0	/*  ...+-1 IN SGL FORMAT */
	move.l		d0,-(sp)	/*  ...push SIGN(X) IN SGL-FMT */
	fmove.l		d1,fpcr
	fmul.s		(sp)+,fp0
	bra			t_frcinx
