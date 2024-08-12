/*
 * 	scosh.sa 3.1 12/10/90
 *
 * 	The entry point sCosh computes the hyperbolic cosine of
 * 	an input argument; sCoshd does the same except for denormalized
 * 	input.
 *
 * 	Input: Double-extended number X in location pointed to
 * 		by address register a0.
 *
 * 	Output: The value cosh(X) returned in floating-point register Fp0.
 *
 * 	Accuracy and Monotonicity: The returned result is within 3 ulps in
 * 		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
 * 		result is subsequently rounded to double precision. The
 * 		result is provably monotonic in double precision.
 *
 * 	Speed: The program sCOSH takes approximately 250 cycles.
 *
 * 	Algorithm:
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
 * 		However, invoking exp(|X|) may cause premature overflow.
 * 		Thus, we calculate sinh(X) as follows:
 * 		Y	:= |X|
 * 		Fact	:=	2**(16380)
 * 		Y'	:= Y - 16381 log2
 * 		cosh(X) := Fact * exp(Y').
 * 		Exit.
 *
 * 	5. (|X| > 16480 log2) sinh(X) must overflow. Return
 * 		Huge*Huge to generate overflow and an infinity with
 * 		the appropriate sign. Huge is the largest finite number in
 * 		extended format. Exit.
 *
 *
 *
 * 		Copyright (C) Motorola, Inc. 1990
 * 			All Rights Reserved
 *
 *        For details on the license for this file, please see the
 *        file, README, in this same directory.
 */

#include "mint/asmdefs.h"

	.xref	t_ovfl
	.xref	t_frcinx
	.xref	setox

	.text

	.globl T1
T1:	.dc.l 0x40C62D38,0xD3D64634 /* ... 16381 LOG2 LEAD */
	.globl T2
T2:	.dc.l 0x3D6F90AE,0xB1E75CC7 /* ... 16381 LOG2 TRAIL */
TWO16380: .dc.l 0x7FFB0000,0x80000000,0x00000000,0x00000000

	.text

	.globl	scoshd
scoshd:
/*
 * --COSH(X) = 1 FOR DENORMALIZED X
 */
	.IFNE GAS
	fmove.s		#0x3F800000,fp0
	.ELSE
	fmove.s		#1.0,fp0
	.ENDC

	fmove.l		d1,fpcr
	.IFNE GAS
	fadd.s		#0x00800000,fp0
	.ELSE
	fadd.s		#1.17549435082228751e-38,fp0
	.ENDC
	bra			t_frcinx

	.globl	scosh
scosh:
	fmove.x		(a0),fp0			/*  ...LOAD INPUT */

	move.l		(a0),d0
	move.w		4(a0),d0
	andi.l		#0x7FFFFFFF,d0
	cmpi.l		#0x400CB167,d0
	bgt.s		COSHBIG

/*
 * --THIS IS THE USUAL CASE, |X| < 16380 LOG2
 * --COSH(X) = (1/2) * ( EXP(X) + 1/EXP(X) )
 */
	fabs.x		fp0					/*  ...|X| */

	move.l		d1,-(sp)
	clr.l		d1
	fmovem.x	fp0-fp0,(a0)		/* pass parameter to setox */
	bsr		setox					/*  ...FP0 IS EXP(|X|) */
	.IFNE GAS
	fmul.s		#0x3F000000,fp0		/*  ...(1/2)EXP(|X|) */
	.ELSE
	fmul.s		#0.5,fp0		/*  ...(1/2)EXP(|X|) */
	.ENDC
	move.l		(sp)+,d1

	.IFNE GAS
	fmove.s		#0x3E800000,fp1		/*  ...(1/4) */
	.ELSE
	fmove.s		#0.25,fp1		/*  ...(1/4) */
	.ENDC
	fdiv.x		fp0,fp1				/*  ...1/(2 EXP(|X|)) */

	fmove.l		d1,fpcr
	fadd.x		fp1,fp0

	bra		t_frcinx

COSHBIG:
	cmpi.l		#0x400CB2B3,d0
	bgt.s		COSHHUGE

	fabs.x		fp0
	fsub.d		T1.w(pc),fp0		/*  ...(|X|-16381LOG2_LEAD) */
	fsub.d		T2.w(pc),fp0		/*  ...|X| - 16381 LOG2, ACCURATE */

	move.l		d1,-(sp)
	clr.l		d1
	fmovem.x	fp0-fp0,(a0)
	bsr			setox
	fmove.l		(sp)+,fpcr

	fmul.x		TWO16380.w(pc),fp0
	bra		t_frcinx

COSHHUGE:
	fmove.l		#0,fpsr				/* clr N bit if set by source */
	bclr.b		#7,(a0)				/* always return positive value */
	fmovem.x	(a0),fp0-fp0
	bra		t_ovfl
