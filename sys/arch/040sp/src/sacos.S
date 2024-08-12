/*
 * 	sacos.sa 3.3 12/19/90
 *
 * 	Description: The entry point sAcos computes the inverse cosine of
 * 		an input argument; sAcosd does the same except for denormalized
 * 		input.
 *
 * 	Input: Double-extended number X in location pointed to
 * 		by address register a0.
 *
 * 	Output: The value arccos(X) returned in floating-point register Fp0.
 *
 * 	Accuracy and Monotonicity: The returned result is within 3 ulps in
 * 		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
 * 		result is subsequently rounded to double precision. The
 * 		result is provably monotonic in double precision.
 *
 * 	Speed: The program sCOS takes approximately 310 cycles.
 *
 * 	Algorithm:
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
	.xref	satan

	.xref PI
	.xref PIBY2

	.text

PI:	.dc.l 0x40000000,0xC90FDAA2,0x2168C235
	.globl PIBY2
PIBY2:	.dc.l 0x3FFF0000,0xC90FDAA2,0x2168C235	/* +PI/2 */

	.text

	.globl	sacosd
sacosd:
/* --ACOS(X) = PI/2 FOR DENORMALIZED X */
	fmove.l		d1,fpcr		/*  ...load user's rounding mode/precision */
	fmove.x		PIBY2.w(pc),fp0
	bra		t_frcinx

	.globl	sacos
sacos:
	fmove.x		(a0),fp0	/*  ...LOAD INPUT */

	move.l		(a0),d0		/*  ...pack exponent with upper 16 fraction */
	move.w		4(a0),d0
	andi.l		#0x7FFFFFFF,d0
	cmpi.l		#0x3FFF8000,d0
	bge.s		ACOSBIG

/* --THIS IS THE USUAL CASE, |X| < 1 */
/* --ACOS(X) = 2 * ATAN(	SQRT( (1-X)/(1+X) )	) */

	.IFNE GAS
	fmove.s		#0x3F800000,fp1
	.ELSE
	fmove.s		#1.0,fp1
	.ENDC
	fadd.x		fp0,fp1		/*  ...1+X */
	fneg.x		fp0		/*  ... -X */
	.IFNE GAS
	fadd.s		#0x3F800000,fp0	/*  ...1-X */
	.ELSE
	fadd.s		#1.0,fp0	/*  ...1-X */
	.ENDC
	fdiv.x		fp1,fp0		/*  ...(1-X)/(1+X) */
	fsqrt.x		fp0		/*  ...SQRT((1-X)/(1+X)) */
	fmovem.x	fp0-fp0,(a0)	/*  ...overwrite input */
	move.l		d1,-(sp)	/* save original users fpcr */
	clr.l		d1
	bsr			satan		/*  ...ATAN(SQRT([1-X]/[1+X])) */
	fmove.l		(sp)+,fpcr	/* restore users exceptions */
	fadd.x		fp0,fp0		/*  ...2 * ATAN( STUFF ) */
	bra			t_frcinx

ACOSBIG:
	fabs.x		fp0
	.IFNE GAS
	fcmp.s		#0x3F800000,fp0
	.ELSE
	fcmp.s		#1.0,fp0
	.ENDC
	fbgt		t_operr		/* cause an operr exception */

/* --|X| = 1, ACOS(X) = 0 OR PI */
	move.l		(a0),d0		/*  ...pack exponent with upper 16 fraction */
	move.w		4(a0),d0
	tst.l		d0		/* D0 has original exponent+fraction */
	bgt.s		ACOSP1

/* --X = -1 */
/* Returns PI and inexact exception */
	fmove.x		PI.w(pc),fp0
	fmove.l		d1,fpcr
	.IFNE GAS
	fadd.s		#0x00800000,fp0
	.ELSE
	fadd.s		#1.17549435082228751e-38,fp0
	.ENDC
/*
 * cause an inexact exception to be put
 * 					into the 040 - will not trap until next
 * 					fp inst.
 */
	bra		t_frcinx

ACOSP1:
	fmove.l		d1,fpcr
	fmove.s		#0x00000000,fp0
	rts				/* Facos ; of +1 is exact */
