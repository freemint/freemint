/*
 * 	x_unimp.sa 3.3 7/1/91
 *
 * 	fpsp_unimp --- FPSP handler for unimplemented instruction
 * 	exception.
 *
 *  Invoked when the user program encounters a floating-point
 *  op-code that hardware does not support.  Trap vector# 11
 *  (See table 8-1 MC68030 User's Manual).
 *
 *
 *  Note: An fsave for an unimplemented inst. will create a short
 *  fsave stack.
 *
 *   Input: 1. Six word stack frame for unimplemented inst, four word
 *             for illegal
 *             (See table 8-7 MC68030 User's Manual).
 *          2. Unimp (short) fsave state frame created here by fsave
 *             instruction.
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

	.xref	get_op
	.xref	do_func
	.xref	sto_res
	.xref	gen_except
	.xref	fpsp_fmt_error

	.text

	.globl	fpsp_unimp
	.globl	uni_2
fpsp_unimp:
	link		a6,#-LOCAL_SIZE
	fsave		-(a7)
uni_2:
	movem.l		d0-d1/a0-a1,USER_DA(a6)
	fmovem.x	fp0-fp3,USER_FP0(a6)
	fmovem.l	fpcr/fpsr/fpiar,USER_FPCR(a6)
	move.b		(a7),d0		/* test for valid version num */
	andi.b		#0xf0,d0		/* test for $4x */
	cmpi.b		#VER_4,d0	/* must be $4x or exit */
	bne			fpsp_fmt_error
/*
 * 	Temporary D25B Fix
 * 	The following lines are used to ensure that the FPSR
 * 	exception byte and condition codes are clear before proceeding
 */
	move.l		USER_FPSR(a6),d0
	andi.l		#0xFF00FF,d0	/* clear all but accrued exceptions */
	move.l		d0,USER_FPSR(a6)
	fmove.l		#0,fpsr /* clear all user bits */
	fmove.l		#0,fpcr	/* clear all user exceptions for FPSP */

	clr.b		UFLG_TMP(a6)	/* clr flag for unsupp data */

	bsr			get_op		/* go get operand(s) */
	clr.b		STORE_FLG(a6)
	bsr			do_func		/* do the function */
	fsave		-(a7)		/* capture possible exc state */
	tst.b		STORE_FLG(a6)
	bne.s		no_store	/* if STORE_FLG is set, no store */
	bsr			sto_res		/* store the result in user space */
no_store:
	bra			gen_except	/* post any exceptions and return */
