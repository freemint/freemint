/*
 * 	scale.sa 3.3 7/30/91
 *
 * 	The entry point sSCALE computes the destination operand
 * 	scaled by the source operand.  If the absolute value of
 * 	the source operand is (>= 2^14) an overflow or underflow
 * 	is returned.
 *
 * 	The entry point sscale is called from do_func to emulate
 * 	the fscale unimplemented instruction.
 *
 * 	Input: Double-extended destination operand in FPTEMP,
 * 		double-extended source operand in ETEMP.
 *
 * 	Output: The function returns scale(X,Y) to fp0.
 *
 * 	Modifies: fp0.
 *
 * 	Algorithm:
 *
 * 		Copyright (C) Motorola, Inc. 1990
 * 			All Rights Reserved
 *
 *        For details on the license for this file, please see the
 *        file, README, in this same directory.
 */

#include "mint/asmdefs.h"
	.include "fpsp.h"

	.xref	t_ovfl2
	.xref	t_unfl
	.xref	round
	.xref	t_resdnrm

	.text

SRC_BNDS: .dc.w	0x3fff,0x400c

/*
 *  This entry point is used by the unimplemented instruction exception
 *  handler.
 *
 *
 *
 * 	FSCALE
 */
	.globl	sscale
sscale:
	fmove.l		#0,fpcr		/* clr user enabled exc */
	clr.l		d1
	move.w		FPTEMP(a6),d1	/* get dest exponent */
	smi		L_SCR1(a6)	/* use L_SCR1 to hold sign */
	andi.l		#0x7fff,d1	/* strip sign */
	move.w		ETEMP(a6),d0	/* check src bounds */
	andi.w		#0x7fff,d0	/* clr sign bit */
	cmp2.w		SRC_BNDS.w(pc),d0
	bcc.s		src_in
	cmpi.w		#0x400c,d0	/* test for too large */
	bge		src_out
/*
 *  The source input is below 1, so we check for denormalized numbers
 *  and set unfl.
 */
src_small:
	move.b		DTAG(a6),d0
	andi.b		#0xe0,d0
	tst.b		d0
	beq.s		no_denorm
	st		STORE_FLG(a6)	/* dest already contains result */
	or.l		#unfl_mask,USER_FPSR(a6) /* set UNFL */
den_done:
	lea.l		FPTEMP(a6),a0
	bra		t_resdnrm
no_denorm:
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		FPTEMP(a6),fp0	/* simply return dest */
	rts


/*
 *  Source is within 2^14 range.  To perform the int operation,
 *  move it to d0.
 */
src_in:
	fmove.x		ETEMP(a6),fp0	/* move in src for int */
	fmove.l		#rz_mode,fpcr	/* force rz for src conversion */
	fmove.l		fp0,d0		/* int src to d0 */
	fmove.l		#0,fpsr		/* clr status from above */
	tst.w		ETEMP(a6)	/* check src sign */
	blt		src_neg

/*
 *  Source is positive.  Add the src to the dest exponent.
 *  The result can be denormalized, if src = 0, or overflow,
 *  if the result of the add sets a bit in the upper word.
 */
src_pos_sc:
	tst.w		d1		/* check for denorm */
	beq		dst_dnrm
	add.l		d0,d1		/* add src to dest exp */
	beq.s		denorm_sc		/* if zero, result is denorm */
	cmpi.l		#0x7fff,d1	/* test for overflow */
	bge.s		ovfl
	tst.b		L_SCR1(a6)
	beq.s		spos_pos
	ori.w		#0x8000,d1
spos_pos:
	move.w		d1,FPTEMP(a6)	/* result in FPTEMP */
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		FPTEMP(a6),fp0	/* write result to fp0 */
	rts
ovfl:
	tst.b		L_SCR1(a6)
	beq.s		sovl_pos
	ori.w		#0x8000,d1
sovl_pos:
	move.w		FPTEMP(a6),ETEMP(a6)	/* result in ETEMP */
	move.l		FPTEMP_HI(a6),ETEMP_HI(a6)
	move.l		FPTEMP_LO(a6),ETEMP_LO(a6)
	bra		t_ovfl2

denorm_sc:
	tst.b		L_SCR1(a6)
	beq.s		den_pos
	ori.w		#0x8000,d1
den_pos:
	tst.l		FPTEMP_HI(a6)	/* check j bit */
	blt.s		nden_exit	/* if set, not denorm */
	move.w		d1,ETEMP(a6)	/* input expected in ETEMP */
	move.l		FPTEMP_HI(a6),ETEMP_HI(a6)
	move.l		FPTEMP_LO(a6),ETEMP_LO(a6)
	or.l		#unfl_bit,USER_FPSR(a6)	/* set unfl */
	lea.l		ETEMP(a6),a0
	bra		t_resdnrm
nden_exit:
	move.w		d1,FPTEMP(a6)	/* result in FPTEMP */
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		FPTEMP(a6),fp0	/* write result to fp0 */
	rts

/*
 *  Source is negative.  Add the src to the dest exponent.
 *  (The result exponent will be reduced).  The result can be
 *  denormalized.
 */
src_neg:
	add.l		d0,d1		/* add src to dest */
	beq.s		denorm_sc	/* if zero, result is denorm */
	blt.s		fix_dnrm	/* if negative, result is */
/* needing denormalization */
	tst.b		L_SCR1(a6)
	beq.s		sneg_pos
	ori.w		#0x8000,d1
sneg_pos:
	move.w		d1,FPTEMP(a6)	/* result in FPTEMP */
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		FPTEMP(a6),fp0	/* write result to fp0 */
	rts


/*
 *  The result exponent is below denorm value.  Test for catastrophic
 *  underflow and force zero if true.  If not, try to shift the
 *  mantissa right until a zero exponent exists.
 */
fix_dnrm:
	cmpi.w		#0xffc0,d1	/* lower bound for normalization */
	blt		fix_unfl	/* if lower, catastrophic unfl */
	move.w		d1,d0		/* use d0 for exp */
	move.l		d2,-(a7)	/* free d2 for norm */
	move.l		FPTEMP_HI(a6),d1
	move.l		FPTEMP_LO(a6),d2
	clr.l		L_SCR2(a6)
fix_loop:
	add.w		#1,d0		/* drive d0 to 0 */
	lsr.l		#1,d1		/* while shifting the */
	roxr.l		#1,d2		/* mantissa to the right */
	bcc.s		no_carry
	st		L_SCR2(a6)	/* use L_SCR2 to capture inex */
no_carry:
	tst.w		d0		/* it is finished when */
	blt.s		fix_loop	/* d0 is zero or the mantissa */
	tst.b		L_SCR2(a6)
	beq.s		tst_zero
	or.l		#unfl_inx_mask,USER_FPSR(a6)
/* set unfl, aunfl, ainex */
/*
 *  Test for zero. If zero, simply use fmove to return +/- zero
 *  to the fpu.
 */
tst_zero:
	clr.w		FPTEMP_EX(a6)
	tst.b		L_SCR1(a6)	/* test for sign */
	beq.s		tst_con
	or.w		#0x8000,FPTEMP_EX(a6) /* set sign bit */
tst_con:
	move.l		d1,FPTEMP_HI(a6)
	move.l		d2,FPTEMP_LO(a6)
	move.l		(a7)+,d2
	tst.l		d1
	bne.s		not_zero_sc
	tst.l		FPTEMP_LO(a6)
	bne.s		not_zero_sc
/*
 *  Result is zero.  Check for rounding mode to set lsb.  If the
 *  mode is rp, and the zero is positive, return smallest denorm.
 *  If the mode is rm, and the zero is negative, return smallest
 *  negative denorm.
 */
	btst.b		#5,FPCR_MODE(a6) /* test if rm or rp */
	beq.s		no_dir
	btst.b		#4,FPCR_MODE(a6) /* check which one */
	beq.s		zer_rm
zer_rp:
	tst.b		L_SCR1(a6)	/* check sign */
	bne.s		no_dir		/* if set, neg op, no inc */
	move.l		#1,FPTEMP_LO(a6) /* set lsb */
	bra.s		sm_dnrm
zer_rm:
	tst.b		L_SCR1(a6)	/* check sign */
	beq.s		no_dir		/* if clr, neg op, no inc */
	move.l		#1,FPTEMP_LO(a6) /* set lsb */
	or.l		#neg_mask,USER_FPSR(a6) /* set N */
	bra.s		sm_dnrm
no_dir:
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		FPTEMP(a6),fp0	/* use fmove to set cc's */
	rts

/*
 *  The rounding mode changed the zero to a smallest denorm. Call
 *  t_resdnrm with exceptional operand in ETEMP.
 */
sm_dnrm:
	move.l		FPTEMP_EX(a6),ETEMP_EX(a6)
	move.l		FPTEMP_HI(a6),ETEMP_HI(a6)
	move.l		FPTEMP_LO(a6),ETEMP_LO(a6)
	lea.l		ETEMP(a6),a0
	bra		t_resdnrm

/*
 *  Result is still denormalized.
 */
not_zero_sc:
	or.l		#unfl_mask,USER_FPSR(a6) /* set unfl */
	tst.b		L_SCR1(a6)	/* check for sign */
	beq.s		fix_exit
	or.l		#neg_mask,USER_FPSR(a6) /* set N */
fix_exit:
	bra.s		sm_dnrm


/*
 *  The result has underflowed to zero. Return zero and set
 *  unfl, aunfl, and ainex.
 */
fix_unfl:
	or.l		#unfl_inx_mask,USER_FPSR(a6)
	btst.b		#5,FPCR_MODE(a6) /* test if rm or rp */
	beq.s		no_dir2
	btst.b		#4,FPCR_MODE(a6) /* check which one */
	beq.s		zer_rm2
zer_rp2:
	tst.b		L_SCR1(a6)	/* check sign */
	bne.s		no_dir2		/* if set, neg op, no inc */
	clr.l		FPTEMP_EX(a6)
	clr.l		FPTEMP_HI(a6)
	move.l		#1,FPTEMP_LO(a6) /* set lsb */
	bra.s		sm_dnrm		/* return smallest denorm */
zer_rm2:
	tst.b		L_SCR1(a6)	/* check sign */
	beq.s		no_dir2		/* if clr, neg op, no inc */
	move.w		#0x8000,FPTEMP_EX(a6)
	clr.l		FPTEMP_HI(a6)
	move.l		#1,FPTEMP_LO(a6) /* set lsb */
	or.l		#neg_mask,USER_FPSR(a6) /* set N */
	bra		sm_dnrm		/* return smallest denorm */

no_dir2:
	tst.b		L_SCR1(a6)
	bge.s		pos_zero_sc
neg_zero:
	clr.l		FP_SCR1(a6)	/* clear the exceptional operand */
	clr.l		FP_SCR1+4(a6)	/* for gen_except. */
	clr.l		FP_SCR1+8(a6)
	.IFNE GAS
	fmove.s		#0x80000000,fp0
	.ELSE
	fmove.s		#-0.0,fp0
	.ENDC
	rts
pos_zero_sc:
	clr.l		FP_SCR1(a6)	/* clear the exceptional operand */
	clr.l		FP_SCR1+4(a6)	/* for gen_except. */
	clr.l		FP_SCR1+8(a6)
	fmove.s		#0x00000000,fp0
	rts

/*
 *  The destination is a denormalized number.  It must be handled
 *  by first shifting the bits in the mantissa until it is normalized,
 *  then adding the remainder of the source to the exponent.
 */
dst_dnrm:
	movem.l		d2/d3,-(a7)
	move.w		FPTEMP_EX(a6),d1
	move.l		FPTEMP_HI(a6),d2
	move.l		FPTEMP_LO(a6),d3
dst_loop:
	tst.l		d2		/* test for normalized result */
	blt.s		dst_norm	/* exit loop if so */
	tst.l		d0		/* otherwise, test shift count */
	beq.s		dst_fin		/* if zero, shifting is done */
	subq.l		#1,d0		/* dec src */
	add.l		d3,d3
	roxl.l		#1,d2
	bra.s		dst_loop
/*
 *  Destination became normalized.  Simply add the remaining
 *  portion of the src to the exponent.
 */
dst_norm:
	add.w		d0,d1		/* dst is normalized; add src */
	tst.b		L_SCR1(a6)
	beq.s		dnrm_pos
	ori.l		#0x8000,d1
dnrm_pos:
	move.w		d1,FPTEMP_EX(a6)
	move.l		d2,FPTEMP_HI(a6)
	move.l		d3,FPTEMP_LO(a6)
	fmove.l		USER_FPCR(a6),fpcr
	fmove.x		FPTEMP(a6),fp0
	movem.l		(a7)+,d2/d3
	rts

/*
 *  Destination remained denormalized.  Call t_excdnrm with
 *  exceptional operand in ETEMP.
 */
dst_fin:
	tst.b		L_SCR1(a6)	/* check for sign */
	beq.s		dst_exit
	or.l		#neg_mask,USER_FPSR(a6) /* set N */
	ori.l		#0x8000,d1
dst_exit:
	move.w		d1,ETEMP_EX(a6)
	move.l		d2,ETEMP_HI(a6)
	move.l		d3,ETEMP_LO(a6)
	or.l		#unfl_mask,USER_FPSR(a6) /* set unfl */
	movem.l		(a7)+,d2/d3
	lea.l		ETEMP(a6),a0
	bra		t_resdnrm

/*
 *  Source is outside of 2^14 range.  Test the sign and branch
 *  to the appropriate exception handler.
 */
src_out:
	tst.b		L_SCR1(a6)
	beq.s		scro_pos
	ori.l		#0x8000,d1
scro_pos:
	move.l		FPTEMP_HI(a6),ETEMP_HI(a6)
	move.l		FPTEMP_LO(a6),ETEMP_LO(a6)
	tst.w		ETEMP(a6)
	blt.s		res_neg
res_pos:
	move.w		d1,ETEMP(a6)	/* result in ETEMP */
	bra		t_ovfl2
res_neg:
	move.w		d1,ETEMP(a6)	/* result in ETEMP */
	lea.l		ETEMP(a6),a0
	bra		t_unfl
