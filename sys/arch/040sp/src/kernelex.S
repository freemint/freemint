/*
 * 	kernel_ex.sa 3.3 12/19/90
 *
 *  This file contains routines to force exception status in the
 *  fpu for exceptional cases detected or reported within the
 *  transcendental functions.  Typically, the t_xx routine will
 *  set the appropriate bits in the USER_FPSR word on the stack.
 *  The bits are tested in gen_except.sa to determine if an exceptional
 *  situation needs to be created on return from the FPSP.
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
mns_inf:  .dc.l 0xffff0000,0x00000000,0x00000000
pls_inf:  .dc.l 0x7fff0000,0x00000000,0x00000000
nan:      .dc.l 0x7fff0000,0xffffffff,0xffffffff

	.xref	 ovf_r_k
	.xref	 unf_sub
	.xref	 nrm_set

	.text

	.globl	 t_dz
	.globl   t_dz2
	.globl   t_operr
	.globl   t_unfl
	.globl   t_ovfl
	.globl   t_ovfl2
	.globl   t_inx2
	.globl	 t_frcinx
	.globl	 t_extdnrm
	.globl	 t_resdnrm
	.globl	 dst_nan_ker
	.globl	 src_nan_ker
/*
 * 	DZ exception
 *
 *
 * 	if dz trap disabled
 * 		store properly signed inf (use sign of etemp) into fp0
 * 		set FPSR exception status dz bit, condition code
 * 		inf bit, and accrued dz bit
 * 		return
 * 		frestore the frame into the machine (done by unimp_hd)
 *
 * 	else dz trap enabled
 * 		set exception status bit & accrued bits in FPSR
 * 		set flag to disable sto_res from corrupting fp register
 * 		return
 * 		frestore the frame into the machine (done by unimp_hd)
 *
 *  t_dz2 is used by monadic functions such as flogn (from do_func).
 *  t_dz is used by monadic functions such as satanh (from the
 *  transcendental function).
 */
t_dz2:
	bset.b	#neg_bit,FPSR_CC(a6)	/* set neg bit in FPSR */
	fmove.l	#0,fpsr			/* clr status bits (Z set) */
	btst.b	#dz_bit,FPCR_ENABLE(a6)	/* test FPCR for dz exc enabled */
	bne.s	dz_ena_end
	bra.s	m_inf			/* flogx always returns -inf */
t_dz:
	fmove.l	#0,fpsr			/* clr status bits (Z set) */
	btst.b	#dz_bit,FPCR_ENABLE(a6)	/* test FPCR for dz exc enabled */
	bne.s	dz_ena
/*
 * 	dz disabled
 */
	btst.b	#sign_bit,ETEMP_EX(a6)	/* check sign for neg or pos */
	beq.s	p_inf			/* branch if pos sign */

m_inf:
	fmovem.x mns_inf.w(pc),fp0-fp0		/* load -inf */
	bset.b	#neg_bit,FPSR_CC(a6)	/* set neg bit in FPSR */
	bra.s	set_fpsr
p_inf:
	fmovem.x pls_inf.w(pc),fp0-fp0		/* load +inf */
set_fpsr:
	or.l	#dzinf_mask,USER_FPSR(a6) /* set I,DZ,ADZ */
	rts
/*
 * 	dz enabled
 */
dz_ena:
	btst.b	#sign_bit,ETEMP_EX(a6)	/* check sign for neg or pos */
	beq.s	dz_ena_end
	bset.b	#neg_bit,FPSR_CC(a6)	/* set neg bit in FPSR */
dz_ena_end:
	or.l	#dzinf_mask,USER_FPSR(a6) /* set I,DZ,ADZ */
	st	STORE_FLG(a6)
	rts
/*
 * 	OPERR exception
 *
 * 	if (operr trap disabled)
 * 		set FPSR exception status operr bit, condition code
 * 		nan bit; Store default NAN into fp0
 * 		frestore the frame into the machine (done by unimp_hd)
 *
 * 	else (operr trap enabled)
 * 		set FPSR exception status operr bit, accrued operr bit
 * 		set flag to disable sto_res from corrupting fp register
 * 		frestore the frame into the machine (done by unimp_hd)
 */
t_operr:
	or.l	#opnan_mask,USER_FPSR(a6) /* set NaN, OPERR, AIOP */

	btst.b	#operr_bit,FPCR_ENABLE(a6) /* test FPCR for operr enabled */
	bne.s	op_ena

	fmovem.x nan.w(pc),fp0-fp0		/* load default nan */
	rts
op_ena:
	st	STORE_FLG(a6)		/* do not corrupt destination */
	rts

/*
 * 	t_unfl --- UNFL exception
 *
 *  This entry point is used by all routines requiring unfl, inex2,
 *  aunfl, and ainex to be set on exit.
 *
 *  On entry, a0 points to the exceptional operand.  The final exceptional
 *  operand is built in FP_SCR1 and only the sign from the original operand
 *  is used.
 */
t_unfl:
	clr.l	FP_SCR1(a6)		/* set exceptional operand to zero */
	clr.l	FP_SCR1+4(a6)
	clr.l	FP_SCR1+8(a6)
	tst.b	(a0)			/* extract sign from caller's exop */
	bpl.s	unfl_signok
	bset	#sign_bit,FP_SCR1(a6)
unfl_signok:
	lea.l	FP_SCR1(a6),a0
	or.l	#unfinx_mask,USER_FPSR(a6)	/* set UNFL, INEX2, AUNFL, AINEX */
unfl_con:
	btst.b	#unfl_bit,FPCR_ENABLE(a6)
	beq.s	unfl_dis

unfl_ena:
	bfclr	STAG(a6){5:3}		/* clear wbtm66,wbtm1,wbtm0 */
	bset.b	#wbtemp15_bit,WB_BYTE(a6) /* set wbtemp15 */
	bset.b	#sticky_bit,STICKY(a6)	/* set sticky bit */

	bclr.b	#E1,E_BYTE(a6)

unfl_dis:
	bfextu	FPCR_MODE(a6){0:2},d0	/* get round precision */

	bclr.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)		/* convert to internal ext format */

	bsr	unf_sub			/* returns IEEE result at a0, and sets FPSR_CC accordingly */

	bfclr	LOCAL_SGN(a0){0:8}	/* convert back to IEEE ext format */
	beq.s	unfl_fin

	bset.b	#sign_bit,LOCAL_EX(a0)
	bset.b	#sign_bit,FP_SCR1(a6)	/* set sign bit of exc operand */

unfl_fin:
	fmovem.x (a0),fp0-fp0		/* store result in fp0 */
	rts


/*
 * 	t_ovfl2 --- OVFL exception (without inex2 returned)
 *
 *  This entry is used by scale to force catastrophic overflow.  The
 *  ovfl, aovfl, and ainex bits are set, but not the inex2 bit.
 */
t_ovfl2:
	or.l	#ovfl_inx_mask,USER_FPSR(a6)
	move.l	ETEMP(a6),FP_SCR1(a6)
	move.l	ETEMP_HI(a6),FP_SCR1+4(a6)
	move.l	ETEMP_LO(a6),FP_SCR1+8(a6)
/*
 *  Check for single or double round precision.  If single, check if
 *  the lower 40 bits of ETEMP are zero; if not, set inex2.  If double,
 *  check if the lower 21 bits are zero; if not, set inex2.
 */
	move.b	FPCR_MODE(a6),d0
	andi.b	#0xc0,d0
	beq.s	t_work		/* if extended, finish ovfl processing */
	cmpi.b	#0x40,d0		/* test for single */
	bne.s	t_dbl
t_sgl:
	tst.b	ETEMP_LO(a6)
	bne.s	t_setinx2
	move.l	ETEMP_HI(a6),d0
	andi.l	#0xff,d0		/* look at only lower 8 bits */
	bne.s	t_setinx2
	bra.s	t_work
t_dbl:
	move.l	ETEMP_LO(a6),d0
	andi.l	#0x7ff,d0	/* look at only lower 11 bits */
	beq.s	t_work
t_setinx2:
	or.l	#inex2_mask,USER_FPSR(a6)
	bra.s	t_work
/*
 * 	t_ovfl --- OVFL exception
 *
 * Note: the exc operand is returned in ETEMP.
 */
t_ovfl:
	or.l	#ovfinx_mask,USER_FPSR(a6)
t_work:
	btst.b	#ovfl_bit,FPCR_ENABLE(a6) /* test FPCR for ovfl enabled */
	beq.s	ovf_dis

ovf_ena:
	clr.l	FP_SCR1(a6)		/* set exceptional operand */
	clr.l	FP_SCR1+4(a6)
	clr.l	FP_SCR1+8(a6)

	bfclr	STAG(a6){5:3}		/* clear wbtm66,wbtm1,wbtm0 */
	bclr.b	#wbtemp15_bit,WB_BYTE(a6) /* clear wbtemp15 */
	bset.b	#sticky_bit,STICKY(a6)	/* set sticky bit */

	bclr.b	#E1,E_BYTE(a6)

/* fall through to disabled case */

/*
 *  For disabled overflow call 'ovf_r_k'.  This routine loads the
 *  correct result based on the rounding precision, destination
 *  format, rounding mode and sign.
 */
ovf_dis:
	bsr	ovf_r_k			/* returns unsigned ETEMP_EX, and sets FPSR_CC accordingly. */
	bfclr	ETEMP_SGN(a6){0:8}	/* fix sign */
	beq.s	ovf_pos
	bset.b	#sign_bit,ETEMP_EX(a6)
	bset.b	#sign_bit,FP_SCR1(a6)	/* set exceptional operand sign */
ovf_pos:
	fmovem.x ETEMP(a6),fp0-fp0		/* move the result to fp0 */
	rts


/*
 * 	INEX2 exception
 *
 *  The inex2 and ainex bits are set.
 */
t_inx2:
	or.l	#inx2a_mask,USER_FPSR(a6) /* set INEX2, AINEX */
	rts

/*
 * 	Force Inex2
 *
 *  This routine is called by the transcendental routines to force
 *  the inex2 exception bits set in the FPSR.  If the underflow bit
 *  is set, but the underflow trap was not taken, the aunfl bit in
 *  the FPSR must be set.
 */
t_frcinx:
	or.l	#inx2a_mask,USER_FPSR(a6) /* set INEX2, AINEX */
	btst.b	#unfl_bit,FPSR_EXCEPT(a6) /* test for unfl bit set */
	beq.s	no_uacc1		/* if clear, do not set aunfl */
	bset.b	#aunfl_bit,FPSR_AEXCEPT(a6)
no_uacc1:
	rts

/*
 * 	DST_NAN
 *
 *  Determine if the destination nan is signalling or non-signalling,
 *  and set the FPSR bits accordingly.  See the MC68040 User's Manual
 *  section 3.2.2.5 NOT-A-NUMBERS.
 */
dst_nan_ker:
	btst.b	#sign_bit,FPTEMP_EX(a6) /* test sign of nan */
	beq.s	dst_pos			/* if clr, it was positive */
	bset.b	#neg_bit,FPSR_CC(a6)	/* set N bit */
dst_pos:
	btst.b	#signan_bit,FPTEMP_HI(a6) /* check if signalling */
	beq.s	dst_snan		/* branch if signalling */

	fmove.l	d1,fpcr			/* restore user's rmode/prec */
	fmove.x FPTEMP(a6),fp0		/* return the non-signalling nan */
/*
 *  Check the source nan.  If it is signalling, snan will be reported.
 */
	move.b	STAG(a6),d0
	andi.b	#0xe0,d0
	cmpi.b	#0x60,d0
	bne.s	no_snan
	btst.b	#signan_bit,ETEMP_HI(a6) /* check if signalling */
	bne.s	no_snan
	or.l	#snaniop_mask,USER_FPSR(a6) /* set NAN, SNAN, AIOP */
no_snan:
	rts

dst_snan:
	btst.b	#snan_bit,FPCR_ENABLE(a6) /* check if trap enabled */
	beq.s	dst_dis			/* branch if disabled */

	or.b	#nan_tag,DTAG(a6)	/* set up dtag for nan */
	st	STORE_FLG(a6)		/* do not store a result */
	or.l	#snaniop_mask,USER_FPSR(a6) /* set NAN, SNAN, AIOP */
	rts

dst_dis:
	bset.b	#signan_bit,FPTEMP_HI(a6) /* set SNAN bit in sop */
	fmove.l	d1,fpcr			/* restore user's rmode/prec */
	fmove.x FPTEMP(a6),fp0		/* load non-sign. nan */
	or.l	#snaniop_mask,USER_FPSR(a6) /* set NAN, SNAN, AIOP */
	rts

/*
 * 	SRC_NAN
 *
 *  Determine if the source nan is signalling or non-signalling,
 *  and set the FPSR bits accordingly.  See the MC68040 User's Manual
 *  section 3.2.2.5 NOT-A-NUMBERS.
 */
src_nan_ker:
	btst.b	#sign_bit,ETEMP_EX(a6) /* test sign of nan */
	beq.s	src_pos_ker			/* if clr, it was positive */
	bset.b	#neg_bit,FPSR_CC(a6)	/* set N bit */
src_pos_ker:
	btst.b	#signan_bit,ETEMP_HI(a6) /* check if signalling */
	beq.s	src_snan		/* branch if signalling */
	fmove.l	d1,fpcr			/* restore user's rmode/prec */
	fmove.x ETEMP(a6),fp0		/* return the non-signalling nan */
	rts

src_snan:
	btst.b	#snan_bit,FPCR_ENABLE(a6) /* check if trap enabled */
	beq.s	src_dis			/* branch if disabled */
	bset.b	#signan_bit,ETEMP_HI(a6) /* set SNAN bit in sop */
	or.b	#norm_tag,DTAG(a6)	/* set up dtag for norm */
	or.b	#nan_tag,STAG(a6)	/* set up stag for nan */
	st	STORE_FLG(a6)		/* do not store a result */
	or.l	#snaniop_mask,USER_FPSR(a6) /* set NAN, SNAN, AIOP */
	rts

src_dis:
	bset.b	#signan_bit,ETEMP_HI(a6) /* set SNAN bit in sop */
	fmove.l	d1,fpcr			/* restore user's rmode/prec */
	fmove.x ETEMP(a6),fp0		/* load non-sign. nan */
	or.l	#snaniop_mask,USER_FPSR(a6) /* set NAN, SNAN, AIOP */
	rts

/*
 *  For all functions that have a denormalized input and that f(x)=x,
 *  this is the entry point
 */
t_extdnrm:
	or.l	#unfinx_mask,USER_FPSR(a6) /* set UNFL, INEX2, AUNFL, AINEX */
	bra.s	xdnrm_con
/*
 *  Entry point for scale with extended denorm.  The function does
 *  not set inex2, aunfl, or ainex.
 */
t_resdnrm:
	or.l	#unfl_mask,USER_FPSR(a6)

xdnrm_con:
	btst.b	#unfl_bit,FPCR_ENABLE(a6)
	beq.s	xdnrm_dis

/*
 *  If exceptions are enabled, the additional task of setting up WBTEMP
 *  is needed so that when the underflow exception handler is entered,
 *  the user perceives no difference between what the 040 provides vs.
 *  what the FPSP provides.
 */
xdnrm_ena:
	move.l	a0,-(a7)

	move.l	LOCAL_EX(a0),FP_SCR1(a6)
	move.l	LOCAL_HI(a0),FP_SCR1+4(a6)
	move.l	LOCAL_LO(a0),FP_SCR1+8(a6)

	lea	FP_SCR1(a6),a0

	bclr.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)		/* convert to internal ext format */
	tst.w	LOCAL_EX(a0)		/* check if input is denorm */
	beq.s	xdnrm_dn		/* if so, skip nrm_set */
	bsr	nrm_set			/* normalize the result (exponent will be negative) */
xdnrm_dn:
	bclr.b	#sign_bit,LOCAL_EX(a0)	/* take off false sign */
	bfclr	LOCAL_SGN(a0){0:8}	/* change back to IEEE ext format */
	beq.s	xdep
	bset.b	#sign_bit,LOCAL_EX(a0)
xdep:
	bfclr	STAG(a6){5:3}		/* clear wbtm66,wbtm1,wbtm0 */
	bset.b	#wbtemp15_bit,WB_BYTE(a6) /* set wbtemp15 */
	bclr.b	#sticky_bit,STICKY(a6)	/* clear sticky bit */
	bclr.b	#E1,E_BYTE(a6)
	move.l	(a7)+,a0
xdnrm_dis:
	bfextu	FPCR_MODE(a6){0:2},d0	/* get round precision */
	bne.s	not_ext_ker			/* if not round extended, store IEEE defaults */
is_ext:
	btst.b	#sign_bit,LOCAL_EX(a0)
	beq.s	xdnrm_store

	bset.b	#neg_bit,FPSR_CC(a6)	/* set N bit in FPSR_CC */

	bra.s	xdnrm_store

not_ext_ker:
	bclr.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)		/* convert to internal ext format */
	bsr	unf_sub			/* returns IEEE result pointed by a0; sets FPSR_CC accordingly */
	bfclr	LOCAL_SGN(a0){0:8}	/* convert back to IEEE ext format */
	beq.s	xdnrm_store
	bset.b	#sign_bit,LOCAL_EX(a0)
xdnrm_store:
	fmovem.x (a0),fp0-fp0		/* store result in fp0 */
	rts

/*
 *  This subroutine is used for dyadic operations that use an extended
 *  denorm within the kernel. The approach used is to capture the frame,
 *  fix/restore.
 */
	.globl	t_avoid_unsupp
t_avoid_unsupp:
	link	a2,#-LOCAL_SIZE		/* so that a2 fpsp.h negative offsets may be used */
	fsave	-(a7)
	tst.b	1(a7)			/* check if idle, exit if so */
	beq.s	idle_end
	btst.b	#E1,E_BYTE(a2)		/* check for an E1 exception if enabled, there is an unsupp */
	beq.s	end_avun		/* else, exit */
	btst.b	#7,DTAG(a2)		/* check for denorm destination */
	beq.s	src_den			/* else, must be a source denorm */
/*
 *  handle destination denorm
 */
	lea	FPTEMP(a2),a0
	btst.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)		/* convert to internal ext format */
	bclr.b	#7,DTAG(a2)		/* set DTAG to norm */
	bsr	nrm_set			/* normalize result, exponent will become negative */
	bclr.b	#sign_bit,LOCAL_EX(a0)	/* get rid of fake sign */
	bfclr	LOCAL_SGN(a0){0:8}	/* convert back to IEEE ext format */
	beq.s	ck_src_den		/* check if source is also denorm */
	bset.b	#sign_bit,LOCAL_EX(a0)
ck_src_den:
	btst.b	#7,STAG(a2)
	beq.s	end_avun
src_den:
	lea	ETEMP(a2),a0
	btst.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)		/* convert to internal ext format */
	bclr.b	#7,STAG(a2)		/* set STAG to norm */
	bsr	nrm_set			/* normalize result, exponent will become negative */
	bclr.b	#sign_bit,LOCAL_EX(a0)	/* get rid of fake sign */
	bfclr	LOCAL_SGN(a0){0:8}	/* convert back to IEEE ext format */
	beq.s	den_com
	bset.b	#sign_bit,LOCAL_EX(a0)
den_com:
	move.b	#0xfe,CU_SAVEPC(a2)	/* set continue frame */
	clr.w	NMNEXC(a2)		/* clear NMNEXC */
	bclr.b	#E1,E_BYTE(a2)
/* 	fmove.l	fpsr,FPSR_SHADOW(a2) */
/* 	bset.b	#SFLAG,E_BYTE(a2) */
/* 	bset.b	#XFLAG,T_BYTE(a2) */
end_avun:
	frestore (a7)+
	unlk	a2
	rts
idle_end:
	add.l	#4,a7
	unlk	a2
	rts
