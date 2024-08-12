/*
 * 	stwotox.sa 3.1 12/10/90
 *
 * 	stwotox  --- 2**X
 * 	stwotoxd --- 2**X for denormalized X
 * 	stentox  --- 10**X
 * 	stentoxd --- 10**X for denormalized X
 *
 * 	Input: Double-extended number X in location pointed to
 * 		by address register a0.
 *
 * 	Output: The function values are returned in Fp0.
 *
 * 	Accuracy and Monotonicity: The returned result is within 2 ulps in
 * 		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
 * 		result is subsequently rounded to double precision. The
 * 		result is provably monotonic in double precision.
 *
 * 	Speed: The program stwotox takes approximately 190 cycles and the
 * 		program stentox takes approximately 200 cycles.
 *
 * 	Algorithm:
 *
 * 	twotox
 * 	1. If |X| > 16480, go to ExpBig.
 *
 * 	2. If |X| < 2**(-70), go to ExpSm.
 *
 * 	3. Decompose X as X = N/64 + r where |r| <= 1/128. Furthermore
 * 		decompose N as
 * 		 N = 64(M + M') + j,  j = 0,1,2,...,63.
 *
 * 	4. Overwrite r := r * log2. Then
 * 		2**X = 2**(M') * 2**(M) * 2**(j/64) * exp(r).
 * 		Go to expr to compute that expression.
 *
 * 	tentox
 * 	1. If |X| > 16480*log_10(2) (base 10 log of 2), go to ExpBig.
 *
 * 	2. If |X| < 2**(-70), go to ExpSm.
 *
 * 	3. Set y := X*log_2(10)*64 (base 2 log of 10). Set
 * 		N := round-to-int(y). Decompose N as
 * 		 N = 64(M + M') + j,  j = 0,1,2,...,63.
 *
 * 	4. Define r as
 * 		r := ((X - N*L1)-N*L2) * L10
 * 		where L1, L2 are the leading and trailing parts of log_10(2)/64
 * 		and L10 is the natural log of 10. Then
 * 		10**X = 2**(M') * 2**(M) * 2**(j/64) * exp(r).
 * 		Go to expr to compute that expression.
 *
 * 	expr
 * 	1. Fetch 2**(j/64) from table as Fact1 and Fact2.
 *
 * 	2. Overwrite Fact1 and Fact2 by
 * 		Fact1 := 2**(M) * Fact1
 * 		Fact2 := 2**(M) * Fact2
 * 		Thus Fact1 + Fact2 = 2**(M) * 2**(j/64).
 *
 * 	3. Calculate P where 1 + P approximates exp(r):
 * 		P = r + r*r*(A1+r*(A2+...+r*A5)).
 *
 * 	4. Let AdjFact := 2**(M'). Return
 * 		AdjFact * ( Fact1 + ((Fact1*P) + Fact2) ).
 * 		Exit.
 *
 * 	ExpBig
 * 	1. Generate overflow by Huge * Huge if X > 0; otherwise, generate
 * 		underflow by Tiny * Tiny.
 *
 * 	ExpSm
 * 	1. Return 1 + X.
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

	.text
L2TEN64:	.dc.l 0x406A934F,0x0979A371 /* ... 64LOG10/LOG2 */
L10TWO1:	.dc.l 0x3F734413,0x509F8000 /* ... LOG2/64LOG10 */

L10TWO2:	.dc.l 0xBFCD0000,0xC0219DC1,0xDA994FD2

LOG10:		.dc.l 0x40000000,0x935D8DDD,0xAAA8AC17

LOG2_stwotox:	.dc.l 0x3FFE0000,0xB17217F7,0xD1CF79AC

EXPA5_stwotox:	.dc.l 0x3F56C16D,0x6F7BD0B2
EXPA4_stwotox:	.dc.l 0x3F811112,0x302C712C
EXPA3_stwotox:	.dc.l 0x3FA55555,0x55554CC1
EXPA2_stwotox:	.dc.l 0x3FC55555,0x55554A54
EXPA1_stwotox:	.dc.l 0x3FE00000,0x00000000

EXPTBL_stwotox:
	.dc.l	0x3FFF0000,0x80000000,0x00000000,0x3F738000
	.dc.l	0x3FFF0000,0x8164D1F3,0xBC030773,0x3FBEF7CA
	.dc.l	0x3FFF0000,0x82CD8698,0xAC2BA1D7,0x3FBDF8A9
	.dc.l	0x3FFF0000,0x843A28C3,0xACDE4046,0x3FBCD7C9
	.dc.l	0x3FFF0000,0x85AAC367,0xCC487B15,0xBFBDE8DA
	.dc.l	0x3FFF0000,0x871F6196,0x9E8D1010,0x3FBDE85C
	.dc.l	0x3FFF0000,0x88980E80,0x92DA8527,0x3FBEBBF1
	.dc.l	0x3FFF0000,0x8A14D575,0x496EFD9A,0x3FBB80CA
	.dc.l	0x3FFF0000,0x8B95C1E3,0xEA8BD6E7,0xBFBA8373
	.dc.l	0x3FFF0000,0x8D1ADF5B,0x7E5BA9E6,0xBFBE9670
	.dc.l	0x3FFF0000,0x8EA4398B,0x45CD53C0,0x3FBDB700
	.dc.l	0x3FFF0000,0x9031DC43,0x1466B1DC,0x3FBEEEB0
	.dc.l	0x3FFF0000,0x91C3D373,0xAB11C336,0x3FBBFD6D
	.dc.l	0x3FFF0000,0x935A2B2F,0x13E6E92C,0xBFBDB319
	.dc.l	0x3FFF0000,0x94F4EFA8,0xFEF70961,0x3FBDBA2B
	.dc.l	0x3FFF0000,0x96942D37,0x20185A00,0x3FBE91D5
	.dc.l	0x3FFF0000,0x9837F051,0x8DB8A96F,0x3FBE8D5A
	.dc.l	0x3FFF0000,0x99E04593,0x20B7FA65,0xBFBCDE7B
	.dc.l	0x3FFF0000,0x9B8D39B9,0xD54E5539,0xBFBEBAAF
	.dc.l	0x3FFF0000,0x9D3ED9A7,0x2CFFB751,0xBFBD86DA
	.dc.l	0x3FFF0000,0x9EF53260,0x91A111AE,0xBFBEBEDD
	.dc.l	0x3FFF0000,0xA0B0510F,0xB9714FC2,0x3FBCC96E
	.dc.l	0x3FFF0000,0xA2704303,0x0C496819,0xBFBEC90B
	.dc.l	0x3FFF0000,0xA43515AE,0x09E6809E,0x3FBBD1DB
	.dc.l	0x3FFF0000,0xA5FED6A9,0xB15138EA,0x3FBCE5EB
	.dc.l	0x3FFF0000,0xA7CD93B4,0xE965356A,0xBFBEC274
	.dc.l	0x3FFF0000,0xA9A15AB4,0xEA7C0EF8,0x3FBEA83C
	.dc.l	0x3FFF0000,0xAB7A39B5,0xA93ED337,0x3FBECB00
	.dc.l	0x3FFF0000,0xAD583EEA,0x42A14AC6,0x3FBE9301
	.dc.l	0x3FFF0000,0xAF3B78AD,0x690A4375,0xBFBD8367
	.dc.l	0x3FFF0000,0xB123F581,0xD2AC2590,0xBFBEF05F
	.dc.l	0x3FFF0000,0xB311C412,0xA9112489,0x3FBDFB3C
	.dc.l	0x3FFF0000,0xB504F333,0xF9DE6484,0x3FBEB2FB
	.dc.l	0x3FFF0000,0xB6FD91E3,0x28D17791,0x3FBAE2CB
	.dc.l	0x3FFF0000,0xB8FBAF47,0x62FB9EE9,0x3FBCDC3C
	.dc.l	0x3FFF0000,0xBAFF5AB2,0x133E45FB,0x3FBEE9AA
	.dc.l	0x3FFF0000,0xBD08A39F,0x580C36BF,0xBFBEAEFD
	.dc.l	0x3FFF0000,0xBF1799B6,0x7A731083,0xBFBCBF51
	.dc.l	0x3FFF0000,0xC12C4CCA,0x66709456,0x3FBEF88A
	.dc.l	0x3FFF0000,0xC346CCDA,0x24976407,0x3FBD83B2
	.dc.l	0x3FFF0000,0xC5672A11,0x5506DADD,0x3FBDF8AB
	.dc.l	0x3FFF0000,0xC78D74C8,0xABB9B15D,0xBFBDFB17
	.dc.l	0x3FFF0000,0xC9B9BD86,0x6E2F27A3,0xBFBEFE3C
	.dc.l	0x3FFF0000,0xCBEC14FE,0xF2727C5D,0xBFBBB6F8
	.dc.l	0x3FFF0000,0xCE248C15,0x1F8480E4,0xBFBCEE53
	.dc.l	0x3FFF0000,0xD06333DA,0xEF2B2595,0xBFBDA4AE
	.dc.l	0x3FFF0000,0xD2A81D91,0xF12AE45A,0x3FBC9124
	.dc.l	0x3FFF0000,0xD4F35AAB,0xCFEDFA1F,0x3FBEB243
	.dc.l	0x3FFF0000,0xD744FCCA,0xD69D6AF4,0x3FBDE69A
	.dc.l	0x3FFF0000,0xD99D15C2,0x78AFD7B6,0xBFB8BC61
	.dc.l	0x3FFF0000,0xDBFBB797,0xDAF23755,0x3FBDF610
	.dc.l	0x3FFF0000,0xDE60F482,0x5E0E9124,0xBFBD8BE1
	.dc.l	0x3FFF0000,0xE0CCDEEC,0x2A94E111,0x3FBACB12
	.dc.l	0x3FFF0000,0xE33F8972,0xBE8A5A51,0x3FBB9BFE
	.dc.l	0x3FFF0000,0xE5B906E7,0x7C8348A8,0x3FBCF2F4
	.dc.l	0x3FFF0000,0xE8396A50,0x3C4BDC68,0x3FBEF22F
	.dc.l	0x3FFF0000,0xEAC0C6E7,0xDD24392F,0xBFBDBF4A
	.dc.l	0x3FFF0000,0xED4F301E,0xD9942B84,0x3FBEC01A
	.dc.l	0x3FFF0000,0xEFE4B99B,0xDCDAF5CB,0x3FBE8CAC
	.dc.l	0x3FFF0000,0xF281773C,0x59FFB13A,0xBFBCBB3F
	.dc.l	0x3FFF0000,0xF5257D15,0x2486CC2C,0x3FBEF73A
	.dc.l	0x3FFF0000,0xF7D0DF73,0x0AD13BB9,0xBFB8B795
	.dc.l	0x3FFF0000,0xFA83B2DB,0x722A033A,0x3FBEF84B
	.dc.l	0x3FFF0000,0xFD3E0C0C,0xF486C175,0xBFBEF581

	N = L_SCR1

	X = FP_SCR1
	XDCARE = X+2
	XFRAC = X+4

	ADJFACT = FP_SCR2

	FACT1 = FP_SCR3
	FACT1HI = FACT1+4
	FACT1LOW = FACT1+8

	FACT2 = FP_SCR4
	FACT2HI = FACT2+4
	FACT2LOW = FACT2+8

	.xref	t_unfl
	.xref	t_ovfl
	.xref	t_frcinx

	.text

	.globl	stwotoxd
stwotoxd:
/*
 * --ENTRY POINT FOR 2**(X) FOR DENORMALIZED ARGUMENT
 */
	fmove.l		d1,fpcr		/* ...set user's rounding mode/precision */
	.IFNE GAS
	fmove.s		#0x3F800000,fp0  /* ...RETURN 1 + X */
	.ELSE
	fmove.s		#1.0,fp0  /* ...RETURN 1 + X */
	.ENDC
	move.l		(a0),d0
	ori.l		#0x00800001,d0
	fadd.s		d0,fp0
	bra		t_frcinx

	.globl	stwotox
stwotox:
/*
 * --ENTRY POINT FOR 2**(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
 */
	fmovem.x	(a0),fp0-fp0	/* ...LOAD INPUT, do not set cc's */

	move.l		(a0),d0
	move.w		4(a0),d0
	fmove.x		fp0,X(a6)
	andi.l		#0x7FFFFFFF,d0

	cmpi.l		#0x3FB98000,d0		/* ...|X| >= 2**(-70)? */
	bge.s		TWOOK1
	bra		EXPBORS

TWOOK1:
	cmpi.l		#0x400D80C0,d0		/* ...|X| > 16480? */
	ble.s		TWOMAIN
	bra		EXPBORS


TWOMAIN:
/*
 * --USUAL CASE, 2^(-70) <= |X| <= 16480
 */
	fmove.x		fp0,fp1
	.IFNE GAS
	fmul.s		#0x42800000,fp1  /* ...64 * X */
	.ELSE
	fmul.s		#64.0,fp1  /* ...64 * X */
	.ENDC

	fmove.l		fp1,N(a6)		/* ...N = ROUND-TO-INT(64 X) */
	move.l		d2,-(sp)
	lea		EXPTBL_stwotox.w(pc),a1	/* ...LOAD ADDRESS OF TABLE OF 2^(J/64) */
	fmove.l		N(a6),fp1		/* ...N --> FLOATING FMT */
	move.l		N(a6),d0
	move.l		d0,d2
	andi.l		#0x3F,d0		/* ...D0 IS J */
	asl.l		#4,d0		/* ...DISPLACEMENT FOR 2^(J/64) */
	adda.l		d0,a1		/* ...ADDRESS FOR 2^(J/64) */
	asr.l		#6,d2		/* ...d2 IS L, N = 64L + J */
	move.l		d2,d0
	asr.l		#1,d0		/* ...D0 IS M */
	sub.l		d0,d2		/* ...d2 IS M', N = 64(M+M') + J */
	addi.l		#0x3FFF,d2
	move.w		d2,ADJFACT(a6)	/* ...ADJFACT IS 2^(M') */
	move.l		(sp)+,d2
/*
 * --SUMMARY: a1 IS ADDRESS FOR THE LEADING PORTION OF 2^(J/64),
 * --D0 IS M WHERE N = 64(M+M') + J. NOTE THAT |M| <= 16140 BY DESIGN.
 * --ADJFACT = 2^(M').
 * --REGISTERS SAVED SO FAR ARE (IN ORDER) FPCR, D0, FP1, a1, AND FP2.
 */
 	.IFNE GAS
	fmul.s		#0x3C800000,fp1  /* ...(1/64)*N */
	.ELSE
	fmul.s		#0.015625,fp1  /* ...(1/64)*N */
	.ENDC
	move.l		(a1)+,FACT1(a6)
	move.l		(a1)+,FACT1HI(a6)
	move.l		(a1)+,FACT1LOW(a6)
	move.w		(a1)+,FACT2(a6)
	clr.w		FACT2+2(a6)

	fsub.x		fp1,fp0		/* ...X - (1/64)*INT(64 X) */

	move.w		(a1)+,FACT2HI(a6)
	clr.w		FACT2HI+2(a6)
	clr.l		FACT2LOW(a6)
	add.w		d0,FACT1(a6)

	fmul.x		LOG2_stwotox.w(pc),fp0	/* ...FP0 IS R */
	add.w		d0,FACT2(a6)

	bra		expr

EXPBORS:
/*
 * --FPCR, D0 SAVED
 */
	cmpi.l		#0x3FFF8000,d0
	bgt.s		EXPBIG_stwotox

EXPSM_stwotox:
/*
 * --|X| IS SMALL, RETURN 1 + X
 */
	fmove.l		d1,fpcr		/* restore users exceptions */
 	.IFNE GAS
	fadd.s		#0x3F800000,fp0  /* ...RETURN 1 + X */
	.ELSE
	fadd.s		#1.0,fp0  /* ...RETURN 1 + X */
	.ENDC

	bra		t_frcinx

EXPBIG_stwotox:
/*
 * --|X| IS LARGE, GENERATE OVERFLOW IF X > 0; ELSE GENERATE UNDERFLOW
 * --REGISTERS SAVE SO FAR ARE FPCR AND  D0
 */
	move.l		X(a6),d0
	tst.l		d0
	blt.s		EXPNEG

	bclr.b		#7,(a0)		/* t_ovfl expects positive value */
	bra		t_ovfl

EXPNEG:
	bclr.b		#7,(a0)		/* t_unfl expects positive value */
	bra		t_unfl

	.globl	stentoxd
stentoxd:
/*
 * --ENTRY POINT FOR 10**(X) FOR DENORMALIZED ARGUMENT
 */
	fmove.l		d1,fpcr		/* ...set user's rounding mode/precision */
	.IFNE GAS
	fmove.s		#0x3F800000,fp0  /* ...RETURN 1 + X */
	.ELSE
	fmove.s		#1.0,fp0  /* ...RETURN 1 + X */
	.ENDC
	move.l		(a0),d0
	ori.l		#0x00800001,d0
	fadd.s		d0,fp0
	bra		t_frcinx

	.globl	stentox
stentox:
/*
 * --ENTRY POINT FOR 10**(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
 */
	fmovem.x	(a0),fp0-fp0	/* ...LOAD INPUT, do not set cc's */

	move.l		(a0),d0
	move.w		4(a0),d0
	fmove.x		fp0,X(a6)
	andi.l		#0x7FFFFFFF,d0

	cmpi.l		#0x3FB98000,d0		/* ...|X| >= 2**(-70)? */
	blt		EXPBORS

	cmpi.l		#0x400B9B07,d0		/* ...|X| <= 16480*log2/log10 ? */
	bgt		EXPBORS

TENMAIN:
/*
 * --USUAL CASE, 2^(-70) <= |X| <= 16480 LOG 2 / LOG 10
 */
	fmove.x		fp0,fp1
	fmul.d		L2TEN64.w(pc),fp1	/* ...X*64*LOG10/LOG2 */

	fmove.l		fp1,N(a6)		/* ...N=INT(X*64*LOG10/LOG2) */
	move.l		d2,-(sp)
	lea		EXPTBL_stwotox.w(pc),a1	/* ...LOAD ADDRESS OF TABLE OF 2^(J/64) */
	fmove.l		N(a6),fp1		/* ...N --> FLOATING FMT */
	move.l		N(a6),d0
	move.l		d0,d2
	andi.l		#0x3F,d0		/* ...D0 IS J */
	asl.l		#4,d0		/* ...DISPLACEMENT FOR 2^(J/64) */
	adda.l		d0,a1		/* ...ADDRESS FOR 2^(J/64) */
	asr.l		#6,d2		/* ...d2 IS L, N = 64L + J */
	move.l		d2,d0
	asr.l		#1,d0		/* ...D0 IS M */
	sub.l		d0,d2		/* ...d2 IS M', N = 64(M+M') + J */
	addi.l		#0x3FFF,d2
	move.w		d2,ADJFACT(a6)	/* ...ADJFACT IS 2^(M') */
	move.l		(sp)+,d2

/*
 * --SUMMARY: a1 IS ADDRESS FOR THE LEADING PORTION OF 2^(J/64),
 * --D0 IS M WHERE N = 64(M+M') + J. NOTE THAT |M| <= 16140 BY DESIGN.
 * --ADJFACT = 2^(M').
 * --REGISTERS SAVED SO FAR ARE (IN ORDER) FPCR, D0, FP1, a1, AND FP2.
 */
	fmove.x		fp1,fp2

	fmul.d		L10TWO1.w(pc),fp1	/* ...N*(LOG2/64LOG10)_LEAD */
	move.l		(a1)+,FACT1(a6)

	fmul.x		L10TWO2.w(pc),fp2	/* ...N*(LOG2/64LOG10)_TRAIL */

	move.l		(a1)+,FACT1HI(a6)
	move.l		(a1)+,FACT1LOW(a6)
	fsub.x		fp1,fp0		/* ...X - N L_LEAD */
	move.w		(a1)+,FACT2(a6)

	fsub.x		fp2,fp0		/* ...X - N L_TRAIL */

	clr.w		FACT2+2(a6)
	move.w		(a1)+,FACT2HI(a6)
	clr.w		FACT2HI+2(a6)
	clr.l		FACT2LOW(a6)

	fmul.x		LOG10.w(pc),fp0	/* ...FP0 IS R */

	add.w		d0,FACT1(a6)
	add.w		d0,FACT2(a6)

expr:
/*
 * --FPCR, FP2, FP3 ARE SAVED IN ORDER AS SHOWN.
 * --ADJFACT CONTAINS 2**(M'), FACT1 + FACT2 = 2**(M) * 2**(J/64).
 * --FP0 IS R. THE FOLLOWING CODE COMPUTES
 * --	2**(M'+M) * 2**(J/64) * EXP(R)
 */
	fmove.x		fp0,fp1
	fmul.x		fp1,fp1		/* ...FP1 IS S = R*R */

	fmove.d		EXPA5_stwotox.w(pc),fp2	/* ...FP2 IS A5 */
	fmove.d		EXPA4_stwotox.w(pc),fp3	/* ...FP3 IS A4 */

	fmul.x		fp1,fp2		/* ...FP2 IS S*A5 */
	fmul.x		fp1,fp3		/* ...FP3 IS S*A4 */

	fadd.d		EXPA3_stwotox.w(pc),fp2	/* ...FP2 IS A3+S*A5 */
	fadd.d		EXPA2_stwotox.w(pc),fp3	/* ...FP3 IS A2+S*A4 */

	fmul.x		fp1,fp2		/* ...FP2 IS S*(A3+S*A5) */
	fmul.x		fp1,fp3		/* ...FP3 IS S*(A2+S*A4) */

	fadd.d		EXPA1_stwotox.w(pc),fp2	/* ...FP2 IS A1+S*(A3+S*A5) */
	fmul.x		fp0,fp3		/* ...FP3 IS R*S*(A2+S*A4) */

	fmul.x		fp1,fp2		/* ...FP2 IS S*(A1+S*(A3+S*A5)) */
	fadd.x		fp3,fp0		/* ...FP0 IS R+R*S*(A2+S*A4) */

	fadd.x		fp2,fp0		/* ...FP0 IS EXP(R) - 1 */


/*
 * --FINAL RECONSTRUCTION PROCESS
 * --EXP(X) = 2^M*2^(J/64) + 2^M*2^(J/64)*(EXP(R)-1)  -  (1 OR 0)
 */
	fmul.x		FACT1(a6),fp0
	fadd.x		FACT2(a6),fp0
	fadd.x		FACT1(a6),fp0

	fmove.l		d1,fpcr		/* restore users exceptions */
	clr.w		ADJFACT+2(a6)
	move.l		#0x80000000,ADJFACT+4(a6)
	clr.l		ADJFACT+8(a6)
	fmul.x		ADJFACT(a6),fp0	/* ...FINAL ADJUSTMENT */

	bra			t_frcinx
