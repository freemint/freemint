/*
 * 	sgetem.sa 3.1 12/10/90
 *
 * 	The entry point sGETEXP returns the exponent portion
 * 	of the input argument.  The exponent bias is removed
 * 	and the exponent value is returned as an extended
 * 	precision number in fp0.  sGETEXPD handles denormalized
 * 	numbers.
 *
 * 	The entry point sGETMAN extracts the mantissa of the
 * 	input argument.  The mantissa is converted to an
 * 	extended precision number and returned in fp0.  The
 * 	range of the result is [1.0 - 2.0).
 *
 *
 * 	Input:  Double-extended number X in the ETEMP space in
 * 		the floating-point save stack.
 *
 * 	Output:	The functions return exp(X) or man(X) in fp0.
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

	.text

/*
 *  This entry point is used by the unimplemented instruction exception
 *  handler.  It points a0 to the input operand.
 *
 *
 *
 * 	SGETEXP
 */
	.globl	sgetexp
sgetexp:
	move.w	LOCAL_EX(a0),d0	/* get the exponent */
	bclr.l	#15,d0		/* clear the sign bit */
	subi.w	#0x3fff,d0	/* subtract off the bias */
	fmove.w  d0,fp0		/* move the exp to fp0 */
	rts

	.globl	sgetexpd
sgetexpd:
	bclr.b	#sign_bit,LOCAL_EX(a0)
	bsr	nrm_set		/* normalize (exp will go negative) */
	move.w	LOCAL_EX(a0),d0	/* load resulting exponent into d0 */
	subi.w	#0x3fff,d0	/* subtract off the bias */
	fmove.w	d0,fp0		/* move the exp to fp0 */
	rts

/*
 *  This entry point is used by the unimplemented instruction exception
 *  handler.  It points a0 to the input operand.
 *
 *
 *
 * 	SGETMAN
 *
 *
 *  For normalized numbers, leave the mantissa alone, simply load
 *  with an exponent of +/- $3fff.
 */
	.globl	sgetman
sgetman:
	move.l	USER_FPCR(a6),d0
	andi.l	#0xffffff00,d0	/* clear rounding precision and mode */
	fmove.l	d0,fpcr		/* this fpcr setting is used by the 882 */
	move.w	LOCAL_EX(a0),d0	/* get the exp (really just want sign bit) */
	ori.w	#0x7fff,d0	/* clear old exp */
	bclr.l	#14,d0		/* make it the new exp +-3fff */
	move.w	d0,LOCAL_EX(a0)	/* move the sign & exp back to fsave stack */
	fmove.x	(a0),fp0	/* put new value back in fp0 */
	rts

/*
 *  For denormalized numbers, shift the mantissa until the j-bit = 1,
 *  then load the exponent with +/1 $3fff.
 */
	.globl	sgetmand
sgetmand:
	move.l	LOCAL_HI(a0),d0	/* load ms mant in d0 */
	move.l	LOCAL_LO(a0),d1	/* load ls mant in d1 */
	bsr.s	shft		/* shift mantissa bits till msbit is set */
	move.l	d0,LOCAL_HI(a0)	/* put ms mant back on stack */
	move.l	d1,LOCAL_LO(a0)	/* put ls mant back on stack */
	bra.s	sgetman

/*
 * 	SHFT
 *
 * 	Shifts the mantissa bits until msbit is set.
 * 	input:
 * 		ms mantissa part in d0
 * 		ls mantissa part in d1
 * 	output:
 * 		shifted bits in d0 and d1
 */
shft:
	tst.l	d0		/* if any bits set in ms mant */
	bne.s	upper		/* then branch else no bits set in ms mant */
	tst.l	d1		/* test if any bits set in ls mant */
	bne.s	cont_getem		/* if set then continue */
	bra.s	shft_end	/* else return */
cont_getem:
	move.l	d3,-(a7)	/* save d3 */
	exg	d0,d1		/* shift ls mant to ms mant */
	bfffo	d0{0:32},d3	/* find first 1 in ls mant to d0 */
	lsl.l	d3,d0		/* shift first 1 to integer bit in ms mant */
	move.l	(a7)+,d3	/* restore d3 */
	bra.s	shft_end
upper:

	movem.l	d3/d5/d6,-(a7)	/* save registers */
	bfffo	d0{0:32},d3	/* find first 1 in ls mant to d0 */
	lsl.l	d3,d0		/* shift ms mant until j-bit is set */
	move.l	d1,d6		/* save ls mant in d6 */
	lsl.l	d3,d1		/* shift ls mant by count */
	move.l	#32,d5
	sub.l	d3,d5		/* sub 32 from shift for ls mant */
	lsr.l	d5,d6		/* shift off all bits but those that will be shifted into ms mant */
	or.l	d6,d0		/* shift the ls mant bits into the ms mant */
	movem.l	(a7)+,d3/d5/d6	/* restore registers */
shft_end:
	rts
