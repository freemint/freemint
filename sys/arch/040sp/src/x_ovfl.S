/*
 * 	x_ovfl.sa 3.5 7/1/91
 *
 * 	fpsp_ovfl --- FPSP handler for overflow exception
 *
 * 	Overflow occurs when a floating-point intermediate result is
 * 	too large to be represented in a floating-point data register,
 * 	or when storing to memory, the contents of a floating-point
 * 	data register are too large to be represented in the
 * 	destination format.
 *
 *  Trap disabled results
 *
 *  If the instruction is move_out, then garbage is stored in the
 *  destination.  If the instruction is not move_out, then the
 *  destination is not affected.  For 68881 compatibility, the
 *  following values should be stored at the destination, based
 *  on the current rounding mode:
 *
 *   RN	Infinity with the sign of the intermediate result.
 *   RZ	Largest magnitude number, with the sign of the
 * 	intermediate result.
 *   RM   For pos overflow, the largest pos number. For neg overflow,
 * 	-infinity
 *   RP   For pos overflow, +infinity. For neg overflow, the largest
 * 	neg number
 *
 *  Trap enabled results
 *  All trap disabled code applies.  In addition the exceptional
 *  operand needs to be made available to the users exception handler
 *  with a bias of $6000 subtracted from the exponent.
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
	.include "fpsp.h"

	.xref	ovf_r_x2
	.xref	ovf_r_x3
	.xref	store
	.xref	real_ovfl
	.xref	real_inex
	.xref	fpsp_done
	.xref	g_opcls
	.xref	b1238_fix

	.text

	.globl	fpsp_ovfl
fpsp_ovfl:
	link		a6,#-LOCAL_SIZE
	fsave		-(a7)
	movem.l		d0-d1/a0-a1,USER_DA(a6)
	fmovem.x	fp0-fp3,USER_FP0(a6)
	fmovem.l	fpcr/fpsr/fpiar,USER_FPCR(a6)

/*
 * 	The 040 doesn't set the AINEX bit in the FPSR, the following
 * 	line temporarily rectifies this error.
 */
	bset.b	#ainex_bit,FPSR_AEXCEPT(a6)

	bsr		ovf_adj		/* denormalize, round & store interm op */
/*
 * 	if overflow traps not enabled check for inexact exception
 */
	btst.b	#ovfl_bit,FPCR_ENABLE(a6)
	beq.s	ck_inex_ovfl

	btst.b		#E3,E_BYTE(a6)
	beq.s		no_e3_1_ovfl
	bfextu		CMDREG3B(a6){6:3},d0	/* get dest reg no */
	bclr.b		d0,FPR_DIRTY_BITS(a6)	/* clr dest dirty bit */
	bsr			b1238_fix
	move.l		USER_FPSR(a6),FPSR_SHADOW(a6)
	or.l		#sx_mask,E_BYTE(a6)
no_e3_1_ovfl:
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			real_ovfl
/*
 *  It is possible to have either inex2 or inex1 exceptions with the
 *  ovfl.  If the inex enable bit is set in the FPCR, and either
 *  inex2 or inex1 occurred, we must clean up and branch to the
 *  real inex handler.
 */
ck_inex_ovfl:
/* 	move.b		FPCR_ENABLE(a6),d0 */
/* 	and.b		FPSR_EXCEPT(a6),d0 */
/* 	andi.b		#$3,d0 */
	btst.b		#inex2_bit,FPCR_ENABLE(a6)
	beq.s		ovfl_exit
/*
 *  Inexact enabled and reported, and we must take an inexact exception.
 */
take_inex_ovfl:
	btst.b		#E3,E_BYTE(a6)
	beq.s		no_e3_2_ovfl
	bfextu		CMDREG3B(a6){6:3},d0	/* get dest reg no */
	bclr.b		d0,FPR_DIRTY_BITS(a6)	/* clr dest dirty bit */
	bsr			b1238_fix
	move.l		USER_FPSR(a6),FPSR_SHADOW(a6)
	or.l		#sx_mask,E_BYTE(a6)
no_e3_2_ovfl:
	move.b		#INEX_VEC,EXC_VEC+1(a6)
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			real_inex

ovfl_exit:
	bclr.b	#E3,E_BYTE(a6)	/* test and clear E3 bit */
	beq.s	e1_set_ovfl
/*
 *  Clear dirty bit on dest resister in the frame before branching
 *  to b1238_fix.
 */
	bfextu		CMDREG3B(a6){6:3},d0	/* get dest reg no */
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
e1_set_ovfl:
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	unlk		a6
	bra			fpsp_done

/*
 * 	ovf_adj
 */
ovf_adj:
/*
 *  Have a0 point to the correct operand.
 */
	btst.b	#E3,E_BYTE(a6)	/* test E3 bit */
	beq.s	ovf_e1

	lea	WBTEMP(a6),a0
	bra.s	ovf_com
ovf_e1:
	lea	ETEMP(a6),a0

ovf_com:
	bclr.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)

	bsr		g_opcls		/* returns opclass in d0 */
	cmpi.w	#3,d0		/* check for opclass3 */
	bne.s	not_opc011_ovfl

/*
 *  FPSR_CC is saved and restored because ovf_r_x3 affects it. The
 *  CCs are defined to be 'not affected' for the opclass3 instruction.
 */
	move.b	FPSR_CC(a6),L_SCR1(a6)
	bsr		ovf_r_x3	/* returns a0 pointing to result */
	move.b	L_SCR1(a6),FPSR_CC(a6)
	bra		store		/* stores to memory or register */

not_opc011_ovfl:
	bsr		ovf_r_x2	/* returns a0 pointing to result */
	bra		store		/* stores to memory or register */
