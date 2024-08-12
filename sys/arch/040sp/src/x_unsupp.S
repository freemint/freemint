/*
 * 	x_unsupp.sa 3.3 7/1/91
 * 
 * 	fpsp_unsupp --- FPSP handler for unsupported data type exception
 * 
 *  Trap vector #55	(See table 8-1 Mc68030 User's manual).
 *  Invoked when the user program encounters a data format (packed) that
 *  hardware does not support or a data type (denormalized numbers or un-
 *  normalized numbers).
 *  Normalizes denorms and unnorms, unpacks packed numbers then stores
 *  them back into the machine to let the 040 finish the operation.
 * 
 *  Unsupp calls two routines:
 * 	1. get_op -  gets the operand(s)
 * 	2. res_func - restore the function back into the 040 or
 * 			if fmove.p fpm,<ea> then pack source (fpm)
 * 			and store in users memory <ea>.
 * 
 *   Input: Long fsave stack frame
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

	.xref	get_op
	.xref	res_func
	.xref	gen_except
	.xref	fpsp_fmt_error

	.text

	.globl	fpsp_unsupp
fpsp_unsupp:
	link		a6,#-LOCAL_SIZE
	fsave		-(a7)
	movem.l		d0-d1/a0-a1,USER_DA(a6)
	fmovem.x	fp0-fp3,USER_FP0(a6)
	fmovem.l	fpcr/fpsr/fpiar,USER_FPCR(a6)


	move.b		(a7),VER_TMP(a6) /* save version number */
	move.b		(a7),d0		/* test for valid version num */
	andi.b		#0xf0,d0		/* test for $4x */
	cmpi.b		#VER_4,d0	/* must be $4x or exit */
	bne			fpsp_fmt_error

	fmove.l		#0,fpsr		/* clear all user status bits */
	fmove.l		#0,fpcr		/* clear all user control bits */
/* 
 * 	The following lines are used to ensure that the FPSR
 * 	exception byte and condition codes are clear before proceeding,
 * 	except in the case of fmove, which leaves the cc's intact.
 */
unsupp_con:
	move.l		USER_FPSR(a6),d1
	btst		#5,CMDREG1B(a6)	/* looking for fmove out */
	bne.s		fmove_con
	andi.l		#0xFF00FF,d1	/* clear all but aexcs and qbyte */
	bra.s		end_fix_unsupp
fmove_con:
	andi.l		#0x0FFF40FF,d1	/* clear all but cc's, snan bit, aexcs, and qbyte */
end_fix_unsupp:
	move.l		d1,USER_FPSR(a6)

	st		UFLG_TMP(a6)	/* set flag for unsupp data */

	bsr			get_op		/* everything okay, go get operand(s) */
	bsr			res_func	/* fix up stack frame so can restore it */
	clr.l		-(a7)
	move.b		VER_TMP(a6),(a7) /* move idle fmt word to top of stack */
	bra			gen_except
