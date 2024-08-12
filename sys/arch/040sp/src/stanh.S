/*
 * 	stanh.sa 3.1 12/10/90
 *
 * 	The entry point sTanh computes the hyperbolic tangent of
 * 	an input argument; sTanhd does the same except for denormalized
 * 	input.
 *
 * 	Input: Double-extended number X in location pointed to
 * 		by address register a0.
 *
 * 	Output: The value tanh(X) returned in floating-point register Fp0.
 *
 * 	Accuracy and Monotonicity: The returned result is within 3 ulps in
 * 		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
 * 		result is subsequently rounded to double precision. The
 * 		result is provably monotonic in double precision.
 *
 * 	Speed: The program stanh takes approximately 270 cycles.
 *
 * 	Algorithm:
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
 *
 * 		Copyright (C) Motorola, Inc. 1990
 * 			All Rights Reserved
 *
 *        For details on the license for this file, please see the
 *        file, README, in this same directory.
 */

#include "mint/asmdefs.h"
	.include "fpsp.h"

	X = FP_SCR5
	XDCARE = X+2
	XFRAC = X+4

	SGN = L_SCR3

	V = FP_SCR6

	.text

BOUNDS1_stanh:	.dc.l 0x3FD78000,0x3FFFDDCE /*  ... 2^(-40), (5/2)LOG2 */

	.xref	t_frcinx
	.xref	t_extdnrm
	.xref	setox
	.xref	setoxm1

	.globl	stanhd
stanhd:
/*
 * --TANH(X) = X FOR DENORMALIZED X
 */
	bra		t_extdnrm

	.globl	stanh
stanh:
	fmove.x		(a0),fp0	/*  ...LOAD INPUT */

	fmove.x		fp0,X(a6)
	move.l		(a0),d0
	move.w		4(a0),d0
	move.l		d0,X(a6)
	andi.l		#0x7FFFFFFF,d0
	cmp2.l		BOUNDS1_stanh.w(pc),d0	/*  ...2**(-40) < |X| < (5/2)LOG2 ? */
	bcs.s		TANHBORS

/*
 * --THIS IS THE USUAL CASE
 * --Y = 2|X|, Z = EXPM1(Y), TANH(X) = SIGN(X) * Z / (Z+2).
 */
	move.l		X(a6),d0
	move.l		d0,SGN(a6)
	andi.l		#0x7FFF0000,d0
	addi.l		#0x00010000,d0	/*  ...EXPONENT OF 2|X| */
	move.l		d0,X(a6)
	and.l		#0x80000000,SGN(a6)
	fmove.x		X(a6),fp0		/*  ...FP0 IS Y = 2|X| */

	move.l		d1,-(a7)
	clr.l		d1
	fmovem.x	fp0-fp0,(a0)
	bsr		setoxm1		/*  ...FP0 IS Z = EXPM1(Y) */
	move.l		(a7)+,d1

	fmove.x		fp0,fp1
	.IFNE GAS
	fadd.s		#0x40000000,fp1	/*  ...Z+2 */
	.ELSE
	fadd.s		#2.0,fp1	/*  ...Z+2 */
	.ENDC
	move.l		SGN(a6),d0
	fmove.x		fp1,V(a6)
	eor.l		d0,V(a6)

	fmove.l		d1,fpcr		/* restore users exceptions */
	fdiv.x		V(a6),fp0
	bra		t_frcinx

TANHBORS:
	cmpi.l		#0x3FFF8000,d0
	blt.s		TANHSM

	cmpi.l		#0x40048AA1,d0
	bgt		TANHHUGE

/*
 * -- (5/2) LOG2 < |X| < 50 LOG2,
 * --TANH(X) = 1 - (2/[EXP(2X)+1]). LET Y = 2|X|, SGN = SIGN(X),
 * --TANH(X) = SGN -	SGN*2/[EXP(Y)+1].
 */
	move.l		X(a6),d0
	move.l		d0,SGN(a6)
	andi.l		#0x7FFF0000,d0
	addi.l		#0x00010000,d0	/*  ...EXPO OF 2|X| */
	move.l		d0,X(a6)		/*  ...Y = 2|X| */
	and.l		#0x80000000,SGN(a6)
	move.l		SGN(a6),d0
	fmove.x		X(a6),fp0		/*  ...Y = 2|X| */

	move.l		d1,-(a7)
	clr.l		d1
	fmovem.x	fp0-fp0,(a0)
	bsr		setox		/*  ...FP0 IS EXP(Y) */
	move.l		(a7)+,d1
	move.l		SGN(a6),d0
	.IFNE GAS
	fadd.s		#0x3F800000,fp0	/*  ...EXP(Y)+1 */
	.ELSE
	fadd.s		#1.0,fp0	/*  ...EXP(Y)+1 */
	.ENDC

	eori.l		#0xC0000000,d0	/*  ...-SIGN(X)*2 */
	fmove.s		d0,fp1		/*  ...-SIGN(X)*2 IN SGL FMT */
	fdiv.x		fp0,fp1		/*  ...-SIGN(X)2 / [EXP(Y)+1 ] */

	move.l		SGN(a6),d0
	ori.l		#0x3F800000,d0	/*  ...SGN */
	fmove.s		d0,fp0		/*  ...SGN IN SGL FMT */

	fmove.l		d1,fpcr		/* restore users exceptions */
	fadd.x		fp1,fp0

	bra.w		t_frcinx

TANHSM:
	move.w		#0x0000,XDCARE(a6)

	fmove.l		d1,fpcr		/* restore users exceptions */
	fmove.x		X(a6),fp0		/* last inst - possible exception set */

	bra.w		t_frcinx

TANHHUGE:
/*
 * ---RETURN SGN(X) - SGN(X)EPS
 */
	move.l		X(a6),d0
	andi.l		#0x80000000,d0
	ori.l		#0x3F800000,d0
	fmove.s		d0,fp0
	andi.l		#0x80000000,d0
	eori.l		#0x80800000,d0	/*  ...-SIGN(X)*EPS */

	fmove.l		d1,fpcr		/* restore users exceptions */
	fadd.s		d0,fp0

	bra.w		t_frcinx
