/*
 * 	util.sa 3.7 7/29/91
 *
 * 	This file contains routines used by other programs.
 *
 * 	ovf_res: used by overflow to force the correct
 * 		 result. ovf_r_k, ovf_r_x2, ovf_r_x3 are
 * 		 derivatives of this routine.
 * 	get_fline: get user's opcode word
 * 	g_dfmtou: returns the destination format.
 * 	g_opcls: returns the opclass of the float instruction.
 * 	g_rndpr: returns the rounding precision.
 * 	reg_dest: write byte, word, or long data to Dn
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

	.xref	mem_read

	.globl	g_dfmtou
	.globl	g_opcls
	.globl	g_rndpr
	.globl	get_fline
	.globl	reg_dest

	.text

/*
 *  Final result table for ovf_res. Note that the negative counterparts
 *  are unnecessary as ovf_res always returns the sign separately from
 *  the exponent.
 */
/* +inf */
EXT_PINF:	.dc.l	0x7fff0000,0x00000000,0x00000000,0x00000000
/* largest +ext */
EXT_PLRG:	.dc.l	0x7ffe0000,0xffffffff,0xffffffff,0x00000000
/* largest magnitude +sgl in ext */
SGL_PLRG:	.dc.l	0x407e0000,0xffffff00,0x00000000,0x00000000
/* largest magnitude +dbl in ext */
DBL_PLRG:	.dc.l	0x43fe0000,0xffffffff,0xfffff800,0x00000000

tblovfl:
	.dc.l	EXT_RN
	.dc.l	EXT_RZ
	.dc.l	EXT_RM
	.dc.l	EXT_RP
	.dc.l	SGL_RN
	.dc.l	SGL_RZ
	.dc.l	SGL_RM
	.dc.l	SGL_RP
	.dc.l	DBL_RN
	.dc.l	DBL_RZ
	.dc.l	DBL_RM
	.dc.l	DBL_RP
	.dc.l	error_util
	.dc.l	error_util
	.dc.l	error_util
	.dc.l	error_util


/*
 * 	ovf_r_k --- overflow result calculation
 */
/*
 *  This entry point is used by kernel_ex.
 *
 *  This forces the destination precision to be extended
 *
 *  Input:	operand in ETEMP
 *  Output:	a result is in ETEMP (internal extended format)
 */
	.globl	ovf_r_k
ovf_r_k:
	lea	ETEMP(a6),a0	/* a0 points to source operand */
	bclr.b	#sign_bit,ETEMP_EX(a6)
	sne	ETEMP_SGN(a6)	/* convert to internal IEEE format */

/*
 * 	ovf_r_x2 --- overflow result calculation
 *
 *  This entry point used by x_ovfl.  (opclass 0 and 2)
 *
 *  Input		a0  points to an operand in the internal extended format
 *  Output	a0  points to the result in the internal extended format
 *
 *  This sets the round precision according to the user's FPCR unless the
 *  instruction is fsgldiv or fsglmul or fsadd, fdadd, fsub, fdsub, fsmul,
 *  fdmul, fsdiv, fddiv, fssqrt, fsmove, fdmove, fsabs, fdabs, fsneg, fdneg.
 *  If the instruction is fsgldiv of fsglmul, the rounding precision must be
 *  extended.  If the instruction is not fsgldiv or fsglmul but a force-
 *  precision instruction, the rounding precision is then set to the force
 *  precision.
 */
	.globl	ovf_r_x2
ovf_r_x2:
	btst.b	#E3,E_BYTE(a6)		/* check for nu exception */
	beq.s	ovf_e1_exc		/* it is cu exception */
ovf_e3_exc:
	move.w	CMDREG3B(a6),d0		/* get the command word */
	andi.w	#0x00000060,d0		/* clear all bits except 6 and 5 */
	cmpi.l	#0x00000040,d0
	beq.s	ovff_sgl		/* force precision is single */
	cmpi.l	#0x00000060,d0
	beq.s	ovff_dbl		/* force precision is double */
	move.w	CMDREG3B(a6),d0		/* get the command word again */
	andi.l	#0x7f,d0			/* clear all except operation */
	cmpi.l	#0x33,d0
	beq.s	ovf_fsgl		/* fsglmul or fsgldiv */
	cmpi.l	#0x30,d0
	beq.s	ovf_fsgl
	bra.s	ovf_fpcr		/* instruction is none of the above use FPCR */
ovf_e1_exc:
	move.w	CMDREG1B(a6),d0		/* get command word */
	andi.l	#0x00000044,d0		/* clear all bits except 6 and 2 */
	cmpi.l	#0x00000040,d0
	beq.s	ovff_sgl		/* the instruction is force single */
	cmpi.l	#0x00000044,d0
	beq.s	ovff_dbl		/* the instruction is force double */
	move.w	CMDREG1B(a6),d0		/* again get the command word */
	andi.l	#0x0000007f,d0		/* clear all except the op code */
	cmpi.l	#0x00000027,d0
	beq.s	ovf_fsgl		/* fsglmul */
	cmpi.l	#0x00000024,d0
	beq.s	ovf_fsgl		/* fsgldiv */
	bra.s	ovf_fpcr		/* none of the above, use FPCR */

/*
 *  Inst is either fsgldiv or fsglmul.  Force extended precision.
 */
ovf_fsgl:
	clr.l	d0
	bra.s	ovf_res

ovff_sgl:
	moveq.l	#0x00000001,d0		/* set single */
	bra.s	ovf_res
ovff_dbl:
	moveq.l	#0x00000002,d0		/* set double */
	bra.s	ovf_res
/*
 *  The precision is in the fpcr.
 */
ovf_fpcr:
	bfextu	FPCR_MODE(a6){0:2},d0 /* set round precision */
	bra.s	ovf_res

/*
 * 	ovf_r_x3 --- overflow result calculation
 *
 *  This entry point used by x_ovfl. (opclass 3 only)
 *
 *  Input		a0  points to an operand in the internal extended format
 *  Output	a0  points to the result in the internal extended format
 *
 *  This sets the round precision according to the destination size.
 */
	.globl	ovf_r_x3
ovf_r_x3:
	bsr	g_dfmtou
/*
 * get dest fmt in d0{1:0}
 * 				for fmovout, the destination format
 * 				is the rounding precision
 */

/*
 * 	ovf_res --- overflow result calculation
 *
 *  Input:
 * 	a0	points to operand in internal extended format
 *  Output:
 * 	a0	points to result in internal extended format
 */
	.globl	ovf_res
ovf_res:
	lsl.l	#2,d0		/* move round precision to d0{3:2} */
	bfextu	FPCR_MODE(a6){2:2},d1 /* set round mode */
	or.l	d1,d0		/* index is fmt:mode in d0{3:0} */
	lea.l	tblovfl.w(pc),a1	/* load a1 with table address */
	move.l	0(a1,d0.w*4),a1	/* use d0 as index to the table */
	jmp	(a1)		/* go to the correct routine */

/*
 * case DEST_FMT = EXT
 */
EXT_RN:
	lea.l	EXT_PINF.w(pc),a1	/* answer is +/- infinity */
	bset.b	#inf_bit,FPSR_CC(a6)
	bra	set_sign	/* now go set the sign */
EXT_RZ:
	lea.l	EXT_PLRG.w(pc),a1	/* answer is +/- large number */
	bra	set_sign	/* now go set the sign */
EXT_RM:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	e_rm_pos
e_rm_neg:
	lea.l	EXT_PINF.w(pc),a1	/* answer is negative infinity */
	or.l	#neginf_mask,USER_FPSR(a6)
	bra	end_ovfr
e_rm_pos:
	lea.l	EXT_PLRG.w(pc),a1	/* answer is large positive number */
	bra	end_ovfr
EXT_RP:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	e_rp_pos
e_rp_neg:
	lea.l	EXT_PLRG.w(pc),a1	/* answer is large negative number */
	bset.b	#neg_bit,FPSR_CC(a6)
	bra	end_ovfr
e_rp_pos:
	lea.l	EXT_PINF.w(pc),a1	/* answer is positive infinity */
	bset.b	#inf_bit,FPSR_CC(a6)
	bra	end_ovfr

/*
 * case DEST_FMT = DBL
 */
DBL_RN:
	lea.l	EXT_PINF.w(pc),a1	/* answer is +/- infinity */
	bset.b	#inf_bit,FPSR_CC(a6)
	bra	set_sign
DBL_RZ:
	lea.l	DBL_PLRG.w(pc),a1	/* answer is +/- large number */
	bra	set_sign	/* now go set the sign */
DBL_RM:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	d_rm_pos
d_rm_neg:
	lea.l	EXT_PINF.w(pc),a1	/* answer is negative infinity */
	or.l	#neginf_mask,USER_FPSR(a6)
	bra	end_ovfr	/* inf is same for all precisions (ext,dbl,sgl) */
d_rm_pos:
	lea.l	DBL_PLRG.w(pc),a1	/* answer is large positive number */
	bra	end_ovfr
DBL_RP:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	d_rp_pos
d_rp_neg:
	lea.l	DBL_PLRG.w(pc),a1	/* answer is large negative number */
	bset.b	#neg_bit,FPSR_CC(a6)
	bra.s	end_ovfr
d_rp_pos:
	lea.l	EXT_PINF.w(pc),a1	/* answer is positive infinity */
	bset.b	#inf_bit,FPSR_CC(a6)
	bra.s	end_ovfr

/*
 * case DEST_FMT = SGL
 */
SGL_RN:
	lea.l	EXT_PINF.w(pc),a1	/* answer is +/-  infinity */
	bset.b	#inf_bit,FPSR_CC(a6)
	bra.s	set_sign
SGL_RZ:
	lea.l	SGL_PLRG.w(pc),a1	/* answer is +/- large number */
	bra.s	set_sign
SGL_RM:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	s_rm_pos
s_rm_neg:
	lea.l	EXT_PINF.w(pc),a1	/* answer is negative infinity */
	or.l	#neginf_mask,USER_FPSR(a6)
	bra.s	end_ovfr
s_rm_pos:
	lea.l	SGL_PLRG.w(pc),a1	/* answer is large positive number */
	bra.s	end_ovfr
SGL_RP:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	s_rp_pos
s_rp_neg:
	lea.l	SGL_PLRG.w(pc),a1	/* answer is large negative number */
	bset.b	#neg_bit,FPSR_CC(a6)
	bra.s	end_ovfr
s_rp_pos:
	lea.l	EXT_PINF.w(pc),a1	/* answer is positive infinity */
	bset.b	#inf_bit,FPSR_CC(a6)
	bra.s	end_ovfr

set_sign:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	end_ovfr
neg_sign:
	bset.b	#neg_bit,FPSR_CC(a6)

end_ovfr:
	move.w	LOCAL_EX(a1),LOCAL_EX(a0) /* do not overwrite sign */
	move.l	LOCAL_HI(a1),LOCAL_HI(a0)
	move.l	LOCAL_LO(a1),LOCAL_LO(a0)
	rts


/*
 * 	ERROR
 */
error_util:
	rts

/*
 * 	get_fline --- get f-line opcode of interrupted instruction
 *
 * 	Returns opcode in the low word of d0.
 */
get_fline:
	move.l	USER_FPIAR(a6),a0	/* opcode address */
	move.l	#0,-(a7)	/* reserve a word on the stack */
	lea.l	2(a7),a1	/* point to low word of temporary */
	move.l	#2,d0		/* count */
	bsr		mem_read
	move.l	(a7)+,d0
	rts

/*
 * 	g_rndpr --- put rounding precision in d0{1:0}
 *
 * 	valid return codes are:
 * 		00 - extended
 * 		01 - single
 * 		10 - double
 *
 *  begin
 *  get rounding precision (cmdreg3b{6:5})
 *  begin
 *   case	opclass = 011 (move out)
 * 	get destination format - this is the also the rounding precision
 *
 *   case	opclass = 0x0
 * 	if E3
 * 	    *case RndPr(from cmdreg3b{6:5} = 11  then RND_PREC = DBL
 * 	    *case RndPr(from cmdreg3b{6:5} = 10  then RND_PREC = SGL
 * 	     case RndPr(from cmdreg3b{6:5} = 00  01
 * 		use precision from FPCR{7:6}
 * 			case 00 then RND_PREC = EXT
 * 			case 01 then RND_PREC = SGL
 * 			case 10 then RND_PREC = DBL
 * 	else E1
 * 	     use precision in FPCR{7:6}
 * 	     case 00 then RND_PREC = EXT
 * 	     case 01 then RND_PREC = SGL
 * 	     case 10 then RND_PREC = DBL
 *  end
 */
g_rndpr:
	bsr		g_opcls		/* get opclass in d0{2:0} */
	cmpi.w	#0x0003,d0	/* check for opclass 011 */
	bne.s	op_0x0

/*
 *  For move out instructions (opclass 011) the destination format
 *  is the same as the rounding precision.  Pass results from g_dfmtou.
 */
	bsr	g_dfmtou
	rts
op_0x0:
	btst.b	#E3,E_BYTE(a6)
	beq.s	unf_e1_exc	/* branch to e1 underflow */
unf_e3_exc:
	move.l	CMDREG3B(a6),d0	/* rounding precision in d0{10:9} */
	bfextu	d0{9:2},d0	/* move the rounding prec bits to d0{1:0} */
	cmpi.l	#0x2,d0
	beq.s	unff_sgl	/* force precision is single */
	cmpi.l	#0x3,d0		/* force precision is double */
	beq.s	unff_dbl
	move.w	CMDREG3B(a6),d0	/* get the command word again */
	andi.l	#0x7f,d0		/* clear all except operation */
	cmpi.l	#0x33,d0
	beq.s	unf_fsgl	/* fsglmul or fsgldiv */
	cmpi.l	#0x30,d0
	beq.s	unf_fsgl	/* fsgldiv or fsglmul */
	bra.s	unf_fpcr
unf_e1_exc:
	move.l	CMDREG1B(a6),d0	/* get 32 bits off the stack, 1st 16 bits are the command word */
	andi.l	#0x00440000,d0	/* clear all bits except bits 6 and 2 */
	cmpi.l	#0x00400000,d0
	beq.s	unff_sgl	/* force single */
	cmpi.l	#0x00440000,d0	/* force double */
	beq.s	unff_dbl
	move.l	CMDREG1B(a6),d0	/* get the command word again */
	andi.l	#0x007f0000,d0	/* clear all bits except the operation */
	cmpi.l	#0x00270000,d0
	beq.s	unf_fsgl	/* fsglmul */
	cmpi.l	#0x00240000,d0
	beq.s	unf_fsgl	/* fsgldiv */
	bra.s	unf_fpcr

/*
 *  Convert to return format.  The values from cmdreg3b and the return
 *  values are:
 * 	cmdreg3b	return	     precision
 * 	--------	------	     ---------
 * 	  00,01		  0		ext
 * 	   10		  1		sgl
 * 	   11		  2		dbl
 *  Force single
 */
unff_sgl:
	move.l	#1,d0		/* return 1 */
	rts
/*
 *  Force double
 */
unff_dbl:
	move.l	#2,d0		/* return 2 */
	rts
/*
 *  Force extended
 */
unf_fsgl:
	move.l	#0,d0
	rts
/*
 *  Get rounding precision set in FPCR{7:6}.
 */
unf_fpcr:
	move.l	USER_FPCR(a6),d0 /* rounding precision bits in d0{7:6} */
	bfextu	d0{24:2},d0	/* move the rounding prec bits to d0{1:0} */
	rts
/*
 * 	g_opcls --- put opclass in d0{2:0}
 */
g_opcls:
	btst.b	#E3,E_BYTE(a6)
	beq.s	opc_1b		/* if set, go to cmdreg1b */
opc_3b:
	clr.l	d0		/* if E3, only opclass 0x0 is possible */
	rts
opc_1b:
	move.l	CMDREG1B(a6),d0
	bfextu	d0{0:3},d0	/* shift opclass bits d0{31:29} to d0{2:0} */
	rts

/*
 * 	g_dfmtou --- put destination format in d0{1:0}
 *
 * 	If E1, the format is from cmdreg1b{12:10}
 * 	If E3, the format is extended.
 *
 * 	Dest. Fmt.
 * 		extended  010 -> 00
 * 		single    001 -> 01
 * 		double    101 -> 10
 */
g_dfmtou:
	btst.b	#E3,E_BYTE(a6)
	beq.s	op011
	clr.l	d0		/* if E1, size is always ext */
	rts
op011:
	move.l	CMDREG1B(a6),d0
	bfextu	d0{3:3},d0	/* dest fmt from cmdreg1b{12:10} */
	cmpi.b	#1,d0		/* check for single */
	bne.s	not_sgl
	move.l	#1,d0
	rts
not_sgl:
	cmpi.b	#5,d0		/* check for double */
	bne.s	not_dbl
	move.l	#2,d0
	rts
not_dbl:
	clr.l	d0		/* must be extended */
	rts

/*
 * Final result table for unf_sub. Note that the negative counterparts
 * are unnecessary as unf_sub always returns the sign separately from
 * the exponent.
 */
	.text
EXT_PZRO:	.dc.l	0x00000000,0x00000000,0x00000000,0x00000000 /* +zero */
SGL_PZRO:	.dc.l	0x3f810000,0x00000000,0x00000000,0x00000000 /* +zero */
DBL_PZRO:	.dc.l	0x3c010000,0x00000000,0x00000000,0x00000000 /* +zero */
EXT_PSML:	.dc.l	0x00000000,0x00000000,0x00000001,0x00000000 /* smallest +ext denorm */
SGL_PSML:	.dc.l	0x3f810000,0x00000100,0x00000000,0x00000000 /* smallest +sgl denorm */
DBL_PSML:	.dc.l	0x3c010000,0x00000000,0x00000800,0x00000000 /* smallest +dbl denorm */

	.text

/*
 * 	UNF_SUB --- underflow result calculation
 *
 *  Input:
 * 	d0	contains round precision
 * 	a0	points to input operand in the internal extended format
 *
 *  Output:
 * 	a0	points to correct internal extended precision result.
 */

tblunf:
	.dc.l	uEXT_RN
	.dc.l	uEXT_RZ
	.dc.l	uEXT_RM
	.dc.l	uEXT_RP
	.dc.l	uSGL_RN
	.dc.l	uSGL_RZ
	.dc.l	uSGL_RM
	.dc.l	uSGL_RP
	.dc.l	uDBL_RN
	.dc.l	uDBL_RZ
	.dc.l	uDBL_RM
	.dc.l	uDBL_RP
	.dc.l	uDBL_RN
	.dc.l	uDBL_RZ
	.dc.l	uDBL_RM
	.dc.l	uDBL_RP

	.globl	unf_sub
unf_sub:
	lsl.l	#2,d0		/* move round precision to d0{3:2} */
	bfextu	FPCR_MODE(a6){2:2},d1 /* set round mode */
	or.l	d1,d0		/* index is fmt:mode in d0{3:0} */
	lea.l	tblunf.w(pc),a1	/* load a1 with table address */
	move.l	0(a1,d0.w*4),a1	/* use d0 as index to the table */
	jmp	(a1)		/* go to the correct routine */

/*
 * case DEST_FMT = EXT
 */
uEXT_RN:
	lea.l	EXT_PZRO.w(pc),a1	/* answer is +/- zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra	uset_sign	/* now go set the sign */
uEXT_RZ:
	lea.l	EXT_PZRO.w(pc),a1	/* answer is +/- zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra	uset_sign	/* now go set the sign */
uEXT_RM:
	tst.b	LOCAL_SGN(a0)	/* if negative underflow */
	beq.s	ue_rm_pos
ue_rm_neg:
	lea.l	EXT_PSML.w(pc),a1	/* answer is negative smallest denorm */
	bset.b	#neg_bit,FPSR_CC(a6)
	bra	end_unfr
ue_rm_pos:
	lea.l	EXT_PZRO.w(pc),a1	/* answer is positive zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra	end_unfr
uEXT_RP:
	tst.b	LOCAL_SGN(a0)	/* if negative underflow */
	beq.s	ue_rp_pos
ue_rp_neg:
	lea.l	EXT_PZRO.w(pc),a1	/* answer is negative zero */
	ori.l	#negz_mask,USER_FPSR(a6)
	bra	end_unfr
ue_rp_pos:
	lea.l	EXT_PSML.w(pc),a1	/* answer is positive smallest denorm */
	bra	end_unfr

/*
 * case DEST_FMT = DBL
 */
uDBL_RN:
	lea.l	DBL_PZRO.w(pc),a1	/* answer is +/- zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra	uset_sign
uDBL_RZ:
	lea.l	DBL_PZRO.w(pc),a1	/* answer is +/- zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra	uset_sign	/* now go set the sign */
uDBL_RM:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	ud_rm_pos
ud_rm_neg:
	lea.l	DBL_PSML.w(pc),a1	/* answer is smallest denormalized negative */
	bset.b	#neg_bit,FPSR_CC(a6)
	bra	end_unfr
ud_rm_pos:
	lea.l	DBL_PZRO.w(pc),a1	/* answer is positive zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra	end_unfr
uDBL_RP:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	ud_rp_pos
ud_rp_neg:
	lea.l	DBL_PZRO.w(pc),a1	/* answer is negative zero */
	ori.l	#negz_mask,USER_FPSR(a6)
	bra.s	end_unfr
ud_rp_pos:
	lea.l	DBL_PSML.w(pc),a1	/* answer is smallest denormalized negative */
	bra.s	end_unfr

/*
 * case DEST_FMT = SGL
 */
uSGL_RN:
	lea.l	SGL_PZRO.w(pc),a1	/* answer is +/- zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra.s	uset_sign
uSGL_RZ:
	lea.l	SGL_PZRO.w(pc),a1	/* answer is +/- zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra.s	uset_sign
uSGL_RM:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	us_rm_pos
us_rm_neg:
	lea.l	SGL_PSML.w(pc),a1	/* answer is smallest denormalized negative */
	bset.b	#neg_bit,FPSR_CC(a6)
	bra.s	end_unfr
us_rm_pos:
	lea.l	SGL_PZRO.w(pc),a1	/* answer is positive zero */
	bset.b	#z_bit,FPSR_CC(a6)
	bra.s	end_unfr
uSGL_RP:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	us_rp_pos
us_rp_neg:
	lea.l	SGL_PZRO.w(pc),a1	/* answer is negative zero */
	ori.l	#negz_mask,USER_FPSR(a6)
	bra.s	end_unfr
us_rp_pos:
	lea.l	SGL_PSML.w(pc),a1	/* answer is smallest denormalized positive */
	bra.s	end_unfr

uset_sign:
	tst.b	LOCAL_SGN(a0)	/* if negative overflow */
	beq.s	end_unfr
uneg_sign:
	bset.b	#neg_bit,FPSR_CC(a6)

end_unfr:
	move.w	LOCAL_EX(a1),LOCAL_EX(a0) /* be careful not to overwrite sign */
	move.l	LOCAL_HI(a1),LOCAL_HI(a0)
	move.l	LOCAL_LO(a1),LOCAL_LO(a0)
	rts

/*
 * 	reg_dest --- write byte, word, or long data to Dn
 *
 *
 *  Input:
 *        L_SCR1: Data
 *        d1:     data size and dest register number formatted as:
 *
 *        32              5    4     3     2     1     0
 *        -----------------------------------------------
 *        |        0        |    Size   |  Dest Reg #   |
 *        -----------------------------------------------
 *
 *        Size is:
 *                0 - Byte
 *                1 - Word
 *                2 - Long/Single
 */
pregdst:
	.dc.l	byte_d0
	.dc.l	byte_d1
	.dc.l	byte_d2
	.dc.l	byte_d3
	.dc.l	byte_d4
	.dc.l	byte_d5
	.dc.l	byte_d6
	.dc.l	byte_d7
	.dc.l	word_d0
	.dc.l	word_d1
	.dc.l	word_d2
	.dc.l	word_d3
	.dc.l	word_d4
	.dc.l	word_d5
	.dc.l	word_d6
	.dc.l	word_d7
	.dc.l	long_d0
	.dc.l	long_d1
	.dc.l	long_d2
	.dc.l	long_d3
	.dc.l	long_d4
	.dc.l	long_d5
	.dc.l	long_d6
	.dc.l	long_d7

reg_dest:
	lea.l	pregdst.w(pc),a0
	move.l	0(a0,d1.w*4),a0
	jmp	(a0)

byte_d0:
	move.b	L_SCR1(a6),USER_D0+3(a6)
	rts
byte_d1:
	move.b	L_SCR1(a6),USER_D1+3(a6)
	rts
byte_d2:
	move.b	L_SCR1(a6),d2
	rts
byte_d3:
	move.b	L_SCR1(a6),d3
	rts
byte_d4:
	move.b	L_SCR1(a6),d4
	rts
byte_d5:
	move.b	L_SCR1(a6),d5
	rts
byte_d6:
	move.b	L_SCR1(a6),d6
	rts
byte_d7:
	move.b	L_SCR1(a6),d7
	rts

word_d0:
	move.w	L_SCR1(a6),USER_D0+2(a6)
	rts
word_d1:
	move.w	L_SCR1(a6),USER_D1+2(a6)
	rts
word_d2:
	move.w	L_SCR1(a6),d2
	rts
word_d3:
	move.w	L_SCR1(a6),d3
	rts
word_d4:
	move.w	L_SCR1(a6),d4
	rts
word_d5:
	move.w	L_SCR1(a6),d5
	rts
word_d6:
	move.w	L_SCR1(a6),d6
	rts
word_d7:
	move.w	L_SCR1(a6),d7
	rts

long_d0:
	move.l	L_SCR1(a6),USER_D0(a6)
	rts
long_d1:
	move.l	L_SCR1(a6),USER_D1(a6)
	rts
long_d2:
	move.l	L_SCR1(a6),d2
	rts
long_d3:
	move.l	L_SCR1(a6),d3
	rts
long_d4:
	move.l	L_SCR1(a6),d4
	rts
long_d5:
	move.l	L_SCR1(a6),d5
	rts
long_d6:
	move.l	L_SCR1(a6),d6
	rts
long_d7:
	move.l	L_SCR1(a6),d7
	rts
