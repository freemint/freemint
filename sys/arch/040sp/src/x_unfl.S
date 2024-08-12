/*
 * 	x_unfl.sa 3.4 7/1/91
 *
 * 	fpsp_unfl --- FPSP handler for underflow exception
 *
 *  Trap disabled results
 * 	For 881/2 compatibility, sw must denormalize the intermediate
 *  result, then store the result.  Denormalization is accomplished
 *  by taking the intermediate result (which is always normalized) and
 *  shifting the mantissa right while incrementing the exponent until
 *  it is equal to the denormalized exponent for the destination
 *  format.  After denormalization, the result is rounded to the
 *  destination format.
 *
 *  Trap enabled results
 * 	All trap disabled code applies.	In addition the exceptional
 *  operand needs to made available to the user with a bias of $6000
 *  added to the exponent.
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

	.xref	denorm
	.xref	round
	.xref	store
	.xref	g_rndpr
	.xref	g_opcls
	.xref	g_dfmtou
	.xref	real_unfl
	.xref	real_inex
	.xref	fpsp_done
	.xref	b1238_fix

	.text

	.globl	fpsp_unfl
fpsp_unfl:
	link		a6,#-LOCAL_SIZE
	fsave		-(a7)
	movem.l		d0-d1/a0-a1,USER_DA(a6)
	fmovem.x	fp0-fp3,USER_FP0(a6)
	fmovem.l	fpcr/fpsr/fpiar,USER_FPCR(a6)

	bsr			unf_res	/* denormalize, round & store interm op */
/*
 *  If underflow exceptions are not enabled, check for inexact
 *  exception
 */
	btst.b		#unfl_bit,FPCR_ENABLE(a6)
	beq.s		ck_inex_unfl

	btst.b		#E3,E_BYTE(a6)
	beq.s		no_e3_1_unfl
/*
 *  Clear dirty bit on dest resister in the frame before branching
 *  to b1238_fix.
 */
	bfextu		CMDREG3B(a6){6:3},d0	/* get dest reg no */
	bclr.b		d0,FPR_DIRTY_BITS(a6)	/* clr dest dirty bit */
	bsr			b1238_fix		/* test for bug1238 case */
	move.l		USER_FPSR(a6),FPSR_SHADOW(a6)
	or.l		#sx_mask,E_BYTE(a6)
no_e3_1_unfl:
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			real_unfl
/*
 *  It is possible to have either inex2 or inex1 exceptions with the
 *  unfl.  If the inex enable bit is set in the FPCR, and either
 *  inex2 or inex1 occurred, we must clean up and branch to the
 *  real inex handler.
 */
ck_inex_unfl:
	move.b		FPCR_ENABLE(a6),d0
	and.b		FPSR_EXCEPT(a6),d0
	andi.b		#0x3,d0
	beq.s		unfl_done_unfl

/*
 *  Inexact enabled and reported, and we must take an inexact exception
 */
take_inex_unfl:
	btst.b		#E3,E_BYTE(a6)
	beq.s		no_e3_2_unfl
/*
 *  Clear dirty bit on dest resister in the frame before branching
 *  to b1238_fix.
 */
	bfextu		CMDREG3B(a6){6:3},d0	/* get dest reg no */
	bclr.b		d0,FPR_DIRTY_BITS(a6)	/* clr dest dirty bit */
	bsr			b1238_fix		/* test for bug1238 case */
	move.l		USER_FPSR(a6),FPSR_SHADOW(a6)
	or.l		#sx_mask,E_BYTE(a6)
no_e3_2_unfl:
	move.b		#INEX_VEC,EXC_VEC+1(a6)
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			real_inex

unfl_done_unfl:
	bclr.b		#E3,E_BYTE(a6)
	beq.s		e1_set_unfl		/* if set then branch */
/*
 *  Clear dirty bit on dest resister in the frame before branching
 *  to b1238_fix.
 */
	bfextu		CMDREG3B(a6){6:3},d0		/* get dest reg no */
	bclr.b		d0,FPR_DIRTY_BITS(a6)	/* clr dest dirty bit */
	bsr			b1238_fix		/* test for bug1238 case */
	move.l		USER_FPSR(a6),FPSR_SHADOW(a6)
	or.l		#sx_mask,E_BYTE(a6)
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			fpsp_done
e1_set_unfl:
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	unlk		a6
	bra			fpsp_done
/*
 * 	unf_res --- underflow result calculation
 */
unf_res:
	bsr			g_rndpr		/* returns RND_PREC in d0 0=ext, */
						/* 1=sgl, 2=dbl */
					/* we need the RND_PREC in the */
					/* upper word for round */
	move.w		#0,-(a7)
	move.w		d0,-(a7)	/* copy RND_PREC to stack */

/*
 *  If the exception bit set is E3, the exceptional operand from the
 *  fpu is in WBTEMP; else it is in FPTEMP.
 */
	btst.b		#E3,E_BYTE(a6)
	beq.s		unf_E1
unf_E3:
	lea		WBTEMP(a6),a0	/* a0 now points to operand */
/*
 *  Test for fsgldiv and fsglmul.  If the inst was one of these, then
 *  force the precision to extended for the denorm routine.  Use
 *  the user's precision for the round routine.
 */
	move.w		CMDREG3B(a6),d1	/* check for fsgldiv or fsglmul */
	andi.w		#0x7f,d1
	cmpi.w		#0x30,d1		/* check for sgldiv */
	beq.s		unf_sgl
	cmpi.w		#0x33,d1		/* check for sglmul */
	bne.s		unf_cont	/* if not, use fpcr prec in round */
unf_sgl:
	clr.l		d0
	move.w		#0x1,(a7)	/* override g_rndpr precision */
					/* force single */
	bra.s		unf_cont
unf_E1:
	lea		FPTEMP(a6),a0	/* a0 now points to operand */
unf_cont:
	bclr.b		#sign_bit,LOCAL_EX(a0)	/* clear sign bit */
	sne		LOCAL_SGN(a0)		/* store sign */

	bsr			denorm		/* returns denorm, a0 points to it */
/*
 *  WARNING:
 * 				d0 has guard,round sticky bit
 * 				make sure that it is not corrupted
 * 				before it reaches the round subroutine
 * 				also ensure that a0 isn't corrupted
 */

/*
 *  Set up d1 for round subroutine d1 contains the PREC/MODE
 *  information respectively on upper/lower register halves.
 */
	bfextu		FPCR_MODE(a6){2:2},d1	/* get mode from FPCR */
						/* mode in lower d1 */
	add.l		(a7)+,d1		/* merge PREC/MODE */
/*
 *  WARNING: a0 and d0 are assumed to be intact between the denorm and
 *  round subroutines. All code between these two subroutines
 *  must not corrupt a0 and d0.
 *
 *
 *  Perform Round
 * 	Input:		a0 points to input operand
 * 			d0{31:29} has guard, round, sticky
 * 			d1{01:00} has rounding mode
 * 			d1{17:16} has rounding precision
 * 	Output:		a0 points to rounded operand
 *
 */
	bsr			round		/* returns rounded denorm at (a0) */
/*
 *  Differentiate between store to memory vs. store to register
 */
unf_store:
	bsr			g_opcls		/* returns opclass in d0{2:0} */
	cmpi.b		#0x3,d0
	bne.s		not_opc011_unfl
/*
 *  At this point, a store to memory is pending
 */
opc011_unfl:
	bsr			g_dfmtou
	tst.b		d0
	beq.s		ext_opc011	/* If extended, do not subtract */
				/* If destination format is sgl/dbl, */
	tst.b		LOCAL_HI(a0)	/* If rounded result is normal,don't */
					/* subtract */
	bmi.s		ext_opc011
	subq.w		#1,LOCAL_EX(a0)	/* account for denorm bias vs. */
/* 				normalized bias
 * 				          normalized   denormalized
 * 				single       $7f           $7e
 * 				double       $3ff          $3fe
 */
ext_opc011:
	bsr			store		/* stores to memory */
	bra.s		unf_done	/* finish up */

/*
 *  At this point, a store to a float register is pending
 */
not_opc011_unfl:
	bsr			store	/* stores to float register */
				/* a0 is not corrupted on a store to a */
				/* float register. */
/*
 *  Set the condition codes according to result
 */
	tst.l		LOCAL_HI(a0)	/* check upper mantissa */
	bne.s		ck_sgn
	tst.l		LOCAL_LO(a0)	/* check lower mantissa */
	bne.s		ck_sgn
	bset.b		#z_bit,FPSR_CC(a6) /* set condition codes if zero */
ck_sgn:
	btst.b		#sign_bit,LOCAL_EX(a0)	/* check the sign bit */
	beq.s		unf_done
	bset.b		#neg_bit,FPSR_CC(a6)

/*
 *  Finish.
 */
unf_done:
	btst.b		#inex2_bit,FPSR_EXCEPT(a6)
	beq.s		no_aunfl
	bset.b		#aunfl_bit,FPSR_AEXCEPT(a6)
no_aunfl:
	rts
