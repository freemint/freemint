/*
 * 	smovecr.sa 3.1 12/10/90
 *
 * 	The entry point sMOVECR returns the constant at the
 * 	offset given in the instruction field.
 *
 * 	Input: An offset in the instruction word.
 *
 * 	Output:	The constant rounded to the user's rounding
 * 		mode unchecked for overflow.
 *
 * 	Modified: fp0.
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

	.xref	nrm_set
	.xref	round

	.text

PIRN:
	.dc.l 0x40000000,0xc90fdaa2,0x2168c235    /* pi */
PIRZRM:
	.dc.l 0x40000000,0xc90fdaa2,0x2168c234    /* pi */
PIRP:
	.dc.l 0x40000000,0xc90fdaa2,0x2168c235    /* pi */
/* round to nearest */
SMALRN:
	.dc.l 0x3ffd0000,0x9a209a84,0xfbcff798    /* log10(2) */
	.dc.l 0x40000000,0xadf85458,0xa2bb4a9a    /* e */
	.dc.l 0x3fff0000,0xb8aa3b29,0x5c17f0bc    /* log2(e) */
	.dc.l 0x3ffd0000,0xde5bd8a9,0x37287195    /* log10(e) */
	.dc.l 0x00000000,0x00000000,0x00000000    /* 0.0 */
/*  round to zero;round to negative infinity */
SMALRZRM:
	.dc.l 0x3ffd0000,0x9a209a84,0xfbcff798    /* log10(2) */
	.dc.l 0x40000000,0xadf85458,0xa2bb4a9a    /* e */
	.dc.l 0x3fff0000,0xb8aa3b29,0x5c17f0bb    /* log2(e) */
	.dc.l 0x3ffd0000,0xde5bd8a9,0x37287195    /* log10(e) */
	.dc.l 0x00000000,0x00000000,0x00000000    /* 0.0 */
/*  round to positive infinity */
SMALRP:
	.dc.l 0x3ffd0000,0x9a209a84,0xfbcff799    /* log10(2) */
	.dc.l 0x40000000,0xadf85458,0xa2bb4a9b    /* e */
	.dc.l 0x3fff0000,0xb8aa3b29,0x5c17f0bc    /* log2(e) */
	.dc.l 0x3ffd0000,0xde5bd8a9,0x37287195    /* log10(e) */
	.dc.l 0x00000000,0x00000000,0x00000000    /* 0.0 */

/* round to nearest */
BIGRN:
	.dc.l 0x3ffe0000,0xb17217f7,0xd1cf79ac    /* ln(2) */
	.dc.l 0x40000000,0x935d8ddd,0xaaa8ac17    /* ln(10) */
	.dc.l 0x3fff0000,0x80000000,0x00000000    /* 10 ^ 0 */

/* round to minus infinity */
BIGRZRM:
	.dc.l 0x3ffe0000,0xb17217f7,0xd1cf79ab    /* ln(2) */
	.dc.l 0x40000000,0x935d8ddd,0xaaa8ac16    /* ln(10) */
	.dc.l 0x3fff0000,0x80000000,0x00000000    /* 10 ^ 0 */
/* round to positive infinity */
BIGRP:
	.dc.l 0x3ffe0000,0xb17217f7,0xd1cf79ac    /* ln(2) */
	.dc.l 0x40000000,0x935d8ddd,0xaaa8ac17    /* ln(10) */
	.dc.l 0x3fff0000,0x80000000,0x00000000    /* 10 ^ 0 */

	.text

/*
 * 	FMOVECR
 */
	.globl	smovcr
smovcr:
	bfextu	CMDREG1B(a6){9:7},d0 /* get offset */
	bfextu	USER_FPCR(a6){26:2},d1 /* get rmode */
/*
 *  check range of offset
 */
	tst.b	d0		/* if zero, offset is to pi */
	beq.s	PI_TBL		/* it is pi */
	cmpi.b	#0x0a,d0		/* check range $01 - $0a */
	ble.s	Z_VAL		/* if in this range, return zero */
	cmpi.b	#0x0e,d0		/* check range $0b - $0e */
	ble.s	SM_TBL		/* valid constants in this range */
	cmpi.b	#0x2f,d0		/* check range $10 - $2f */
	ble.s	Z_VAL		/* if in this range, return zero */
	cmpi.b	#0x3f,d0		/* check range $30 - $3f */
	ble.s	BG_TBL		/* valid constants in this range */
Z_VAL:
	fmove.s	#0,fp0
	rts
PI_TBL:
	tst.b	d1		/* offset is zero, check for rmode */
	beq.s	PI_RN		/* if zero, rn mode */
	cmpi.b	#0x3,d1		/* check for rp */
	beq.s	PI_RP		/* if 3, rp mode */
PI_RZRM:
	lea.l	PIRZRM.w(pc),a0	/* rmode is rz or rm, load PIRZRM in a0 */
	bra	set_finx
PI_RN:
	lea.l	PIRN.w(pc),a0		/* rmode is rn, load PIRN in a0 */
	bra	set_finx
PI_RP:
	lea.l	PIRP.w(pc),a0		/* rmode is rp, load PIRP in a0 */
	bra	set_finx
SM_TBL:
	subi.l	#0xb,d0		/* make offset in 0 - 4 range */
	tst.b	d1		/* check for rmode */
	beq.s	SM_RN		/* if zero, rn mode */
	cmpi.b	#0x3,d1		/* check for rp */
	beq.s	SM_RP		/* if 3, rp mode */
SM_RZRM:
	lea.l	SMALRZRM.w(pc),a0	/* rmode is rz or rm, load SMRZRM in a0 */
	cmpi.b	#0x2,d0		/* check if result is inex */
	ble.s	set_finx	/* if 0 - 2, it is inexact */
	bra.s	no_finx		/* if 3, it is exact */
SM_RN:
	lea.l	SMALRN.w(pc),a0	/* rmode is rn, load SMRN in a0 */
	cmpi.b	#0x2,d0		/* check if result is inex */
	ble.s	set_finx	/* if 0 - 2, it is inexact */
	bra.s	no_finx		/* if 3, it is exact */
SM_RP:
	lea.l	SMALRP.w(pc),a0	/* rmode is rp, load SMRP in a0 */
	cmpi.b	#0x2,d0		/* check if result is inex */
	ble.s	set_finx	/* if 0 - 2, it is inexact */
	bra.s	no_finx		/* if 3, it is exact */
BG_TBL:
	subi.l	#0x30,d0		/* make offset in 0 - f range */
	tst.b	d1		/* check for rmode */
	beq.s	BG_RN		/* if zero, rn mode */
	cmpi.b	#0x3,d1		/* check for rp */
	beq.s	BG_RP		/* if 3, rp mode */
BG_RZRM:
	lea.l	BIGRZRM.w(pc),a0	/* rmode is rz or rm, load BGRZRM in a0 */
	cmpi.b	#0x1,d0		/* check if result is inex */
	ble.s	set_finx	/* if 0 - 1, it is inexact */
	cmpi.b	#0x7,d0		/* second check */
	ble.s	no_finx		/* if 0 - 7, it is exact */
	bra.s	set_finx	/* if 8 - f, it is inexact */
BG_RN:
	lea.l	BIGRN.w(pc),a0	/* rmode is rn, load BGRN in a0 */
	cmpi.b	#0x1,d0		/* check if result is inex */
	ble.s	set_finx	/* if 0 - 1, it is inexact */
	cmpi.b	#0x7,d0		/* second check */
	ble.s	no_finx		/* if 0 - 7, it is exact */
	bra.s	set_finx	/* if 8 - f, it is inexact */
BG_RP:
	lea.l	BIGRP.w(pc),a0	/* rmode is rp, load SMRP in a0 */
	cmpi.b	#0x1,d0		/* check if result is inex */
	ble.s	set_finx	/* if 0 - 1, it is inexact */
	cmpi.b	#0x7,d0		/* second check */
	ble.s	no_finx		/* if 0 - 7, it is exact */
/* 	bra.s	set_finx	 if 8 - f, it is inexact */
set_finx:
	or.l	#inx2a_mask,USER_FPSR(a6) /* set inex2/ainex */
no_finx:
	mulu.l	#12,d0			/* use offset to point into tables */
	move.l	d1,L_SCR1(a6)		/* load mode for round call */
	bfextu	USER_FPCR(a6){24:2},d1	/* get precision */
	tst.l	d1			/* check if extended precision */
/*
 *  Precision is extended
 */
	bne.s	not_ext_sm			/* if extended, do not call round */
	fmovem.x (a0,d0.w),fp0-fp0		/* return result in fp0 */
	rts
/*
 *  Precision is single or double
 */
not_ext_sm:
	swap	d1			/* rnd prec in upper word of d1 */
	add.l	L_SCR1(a6),d1		/* merge rmode in low word of d1 */
	move.l	(a0,d0.w),FP_SCR1(a6)	/* load first word to temp storage */
	move.l	4(a0,d0.w),FP_SCR1+4(a6)	/* load second word */
	move.l	8(a0,d0.w),FP_SCR1+8(a6)	/* load third word */
	clr.l	d0			/* clear g,r,s */
	lea	FP_SCR1(a6),a0
	btst.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)		/* convert to internal ext. format */

	bsr	round			/* go round the mantissa */

	bfclr	LOCAL_SGN(a0){0:8}	/* convert back to IEEE ext format */
	beq.s	fin_fcr
	bset.b	#sign_bit,LOCAL_EX(a0)
fin_fcr:
	fmovem.x (a0),fp0-fp0
	rts
