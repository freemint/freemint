/*
 * 	sto_res.sa 3.1 12/10/90
 *
 * 	Takes the result and puts it in where the user expects it.
 * 	Library functions return result in fp0.	If fp0 is not the
 * 	users destination register then fp0 is moved to the
 * 	correct floating-point destination register.  fp0 and fp1
 * 	are then restored to the original contents.
 *
 * 	Input:	result in fp0,fp1
 *
 * 		d2 & a0 should be kept unmodified
 *
 * 	Output:	moves the result to the true destination reg or mem
 *
 * 	Modifies: destination floating point register
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

	.globl	sto_cos
sto_cos:
	bfextu		CMDREG1B(a6){13:3},d0	/* extract cos destination */
	cmpi.b		#3,d0		/* check for fp0/fp1 cases */
	ble.s		c_fp0123
	fmovem.x	fp1-fp1,-(a7)
	moveq.l		#7,d1
	sub.l		d0,d1		/* d1 = 7- (dest. reg. no.) */
	clr.l		d0
	bset.l		d1,d0		/* d0 is dynamic register mask */
	fmovem.x	(a7)+,d0
	rts
c_fp0123:
	tst.b		d0
	beq.s		c_is_fp0
	cmpi.b		#1,d0
	beq.s		c_is_fp1
	cmpi.b		#2,d0
	beq.s		c_is_fp2
c_is_fp3:
	fmovem.x	fp1-fp1,USER_FP3(a6)
	rts
c_is_fp2:
	fmovem.x	fp1-fp1,USER_FP2(a6)
	rts
c_is_fp1:
	fmovem.x	fp1-fp1,USER_FP1(a6)
	rts
c_is_fp0:
	fmovem.x	fp1-fp1,USER_FP0(a6)
	rts


	.globl	sto_res
sto_res:
	bfextu		CMDREG1B(a6){6:3},d0	/* extract destination register */
	cmpi.b		#3,d0		/* check for fp0/fp1 cases */
	ble.s		fp0123_sto
	fmovem.x	fp0-fp0,-(a7)
	moveq.l		#7,d1
	sub.l		d0,d1		/* d1 = 7- (dest. reg. no.) */
	clr.l		d0
	bset.l		d1,d0		/* d0 is dynamic register mask */
	fmovem.x	(a7)+,d0
	rts
fp0123_sto:
	tst.b		d0
	beq.s		is_fp0
	cmpi.b		#1,d0
	beq.s		is_fp1
	cmpi.b		#2,d0
	beq.s		is_fp2
is_fp3:
	fmovem.x	fp0-fp0,USER_FP3(a6)
	rts
is_fp2:
	fmovem.x	fp0-fp0,USER_FP2(a6)
	rts
is_fp1:
	fmovem.x	fp0-fp0,USER_FP1(a6)
	rts
is_fp0:
	fmovem.x	fp0-fp0,USER_FP0(a6)
	rts
