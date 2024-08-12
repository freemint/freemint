/*
 * 	decbin.sa 3.3 12/19/90
 *
 * 	Description: Converts normalized packed bcd value pointed to by
 * 	register A6 to extended-precision value in FP0.
 *
 * 	Input: Normalized packed bcd value in ETEMP(a6).
 *
 * 	Output:	Exact floating-point representation of the packed bcd value.
 *
 * 	Saves and Modifies: D2-D5
 *
 * 	Speed: The program decbin takes ??? cycles to execute.
 *
 * 	Object Size:
 *
 * 	External Reference(s): None.
 *
 * 	Algorithm:
 * 	Expected is a normal bcd (i.e. non-exceptional; all inf, zero,
 * 	and NaN operands are dispatched without entering this routine)
 * 	value in 68881/882 format at location ETEMP(A6).
 *
 * 	A1.	Convert the bcd exponent to binary by successive adds and muls.
 * 	Set the sign according to SE. Subtract 16 to compensate
 * 	for the mantissa which is to be interpreted as 17 integer
 * 	digits, rather than 1 integer and 16 fraction digits.
 * 	Note: this operation can never overflow.
 *
 * 	A2. Convert the bcd mantissa to binary by successive
 * 	adds and muls in FP0. Set the sign according to SM.
 * 	The mantissa digits will be converted with the decimal point
 * 	assumed following the least-significant digit.
 * 	Note: this operation can never overflow.
 *
 * 	A3. Count the number of leading/trailing zeros in the
 * 	bcd string.  If SE is positive, count the leading zeros;
 * 	if negative, count the trailing zeros.  Set the adjusted
 * 	exponent equal to the exponent from A1 and the zero count
 * 	added if SM = 1 and subtracted if SM = 0.  Scale the
 * 	mantissa the equivalent of forcing in the bcd value:
 *
 * 	SM = 0	a non-zero digit in the integer position
 * 	SM = 1	a non-zero digit in Mant0, lsd of the fraction
 *
 * 	this will insure that any value, regardless of its
 * 	representation (ex. 0.1E2, 1E1, 10E0, 100E-1), is converted
 * 	consistently.
 *
 * 	A4. Calculate the factor 10^exp in FP1 using a table of
 * 	10^(2^n) values.  To reduce the error in forming factors
 * 	greater than 10^27, a directed rounding scheme is used with
 * 	tables rounded to RN, RM, and RP, according to the table
 * 	in the comments of the pwrten section.
 *
 * 	A5. Form the final binary number by scaling the mantissa by
 * 	the exponent factor.  This is done by multiplying the
 * 	mantissa in FP0 by the factor in FP1 if the adjusted
 * 	exponent sign is positive, and dividing FP0 by FP1 if
 * 	it is negative.
 *
 * 	Clean up and return.  Check if the final mul or div resulted
 * 	in an inex2 exception.  If so, set inex1 in the fpsr and
 * 	check if the inex1 exception is enabled.  If so, set d7 upper
 * 	word to $0100.  This will signal unimp.sa that an enabled inex1
 * 	exception occurred.  Unimp will fix the stack.
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
RTABLE:
	.dc.b	0,0,0,0
	.dc.b	2,3,2,3
	.dc.b	2,3,3,2
	.dc.b	3,2,2,3

	.globl	decbin
	.globl	calc_e
	.globl	pwrten
	.globl	calc_m
	.globl	norm
	.globl	ap_st_z
	.globl	ap_st_n

	FNIBS = 7
	FSTRT = 0

	ESTRT = 4
	EDIGITS = 2

	.xref FONE
	.xref FTEN

	TEN = 10

/*
 * 	PTENRN, PTENRM, and PTENRP are arrays of powers of 10 rounded
 * 	to nearest, minus, and plus, respectively.  The tables include
 * 	10**{1,2,4,8,16,32,64,128,256,512,1024,2048,4096}.  No rounding
 * 	is required until the power is greater than 27, however, all
 * 	tables include the first 5 for ease of indexing.
 */
	.text
	.globl	PTENRN
PTENRN:
	.dc.l 0x40020000,0xA0000000,0x00000000    /* 10 ^ 1 */
	.dc.l 0x40050000,0xC8000000,0x00000000    /* 10 ^ 2 */
	.dc.l 0x400C0000,0x9C400000,0x00000000    /* 10 ^ 4 */
	.dc.l 0x40190000,0xBEBC2000,0x00000000    /* 10 ^ 8 */
	.dc.l 0x40340000,0x8E1BC9BF,0x04000000    /* 10 ^ 16 */
	.dc.l 0x40690000,0x9DC5ADA8,0x2B70B59E    /* 10 ^ 32 */
	.dc.l 0x40D30000,0xC2781F49,0xFFCFA6D5    /* 10 ^ 64 */
	.dc.l 0x41A80000,0x93BA47C9,0x80E98CE0    /* 10 ^ 128 */
	.dc.l 0x43510000,0xAA7EEBFB,0x9DF9DE8E    /* 10 ^ 256 */
	.dc.l 0x46A30000,0xE319A0AE,0xA60E91C7    /* 10 ^ 512 */
	.dc.l 0x4D480000,0xC9767586,0x81750C17    /* 10 ^ 1024 */
	.dc.l 0x5A920000,0x9E8B3B5D,0xC53D5DE5    /* 10 ^ 2048 */
	.dc.l 0x75250000,0xC4605202,0x8A20979B    /* 10 ^ 4096 */
	.globl	PTENRM
PTENRM:
	.dc.l 0x40020000,0xA0000000,0x00000000    /* 10 ^ 1 */
	.dc.l 0x40050000,0xC8000000,0x00000000    /* 10 ^ 2 */
	.dc.l 0x400C0000,0x9C400000,0x00000000    /* 10 ^ 4 */
	.dc.l 0x40190000,0xBEBC2000,0x00000000    /* 10 ^ 8 */
	.dc.l 0x40340000,0x8E1BC9BF,0x04000000    /* 10 ^ 16 */
	.dc.l 0x40690000,0x9DC5ADA8,0x2B70B59D    /* 10 ^ 32 */
	.dc.l 0x40D30000,0xC2781F49,0xFFCFA6D5    /* 10 ^ 64 */
	.dc.l 0x41A80000,0x93BA47C9,0x80E98CDF    /* 10 ^ 128 */
	.dc.l 0x43510000,0xAA7EEBFB,0x9DF9DE8D    /* 10 ^ 256 */
	.dc.l 0x46A30000,0xE319A0AE,0xA60E91C6    /* 10 ^ 512 */
	.dc.l 0x4D480000,0xC9767586,0x81750C17    /* 10 ^ 1024 */
	.dc.l 0x5A920000,0x9E8B3B5D,0xC53D5DE5    /* 10 ^ 2048 */
	.dc.l 0x75250000,0xC4605202,0x8A20979A    /* 10 ^ 4096 */
	.globl	PTENRP
PTENRP:
	.dc.l 0x40020000,0xA0000000,0x00000000    /* 10 ^ 1 */
	.dc.l 0x40050000,0xC8000000,0x00000000    /* 10 ^ 2 */
	.dc.l 0x400C0000,0x9C400000,0x00000000    /* 10 ^ 4 */
	.dc.l 0x40190000,0xBEBC2000,0x00000000    /* 10 ^ 8 */
	.dc.l 0x40340000,0x8E1BC9BF,0x04000000    /* 10 ^ 16 */
	.dc.l 0x40690000,0x9DC5ADA8,0x2B70B59E    /* 10 ^ 32 */
	.dc.l 0x40D30000,0xC2781F49,0xFFCFA6D6    /* 10 ^ 64 */
	.dc.l 0x41A80000,0x93BA47C9,0x80E98CE0    /* 10 ^ 128 */
	.dc.l 0x43510000,0xAA7EEBFB,0x9DF9DE8E    /* 10 ^ 256 */
	.dc.l 0x46A30000,0xE319A0AE,0xA60E91C7    /* 10 ^ 512 */
	.dc.l 0x4D480000,0xC9767586,0x81750C18    /* 10 ^ 1024 */
	.dc.l 0x5A920000,0x9E8B3B5D,0xC53D5DE6    /* 10 ^ 2048 */
	.dc.l 0x75250000,0xC4605202,0x8A20979B    /* 10 ^ 4096 */

	.text

decbin:
	fmove.l	#0,fpcr		/* clr real fpcr */
	movem.l	d2-d5,-(a7)
/*
 *  Calculate exponent:
 *   1. Copy bcd value in memory for use as a working copy.
 *   2. Calculate absolute value of exponent in d1 by mul and add.
 *   3. Correct for exponent sign.
 *   4. Subtract 16 to compensate for interpreting the mant as all integer digits.
 *      (i.e., all digits assumed left of the decimal point.)
 *
 *  Register usage:
 *
 *   calc_e:
 * 	(*)  d0: temp digit storage
 * 	(*)  d1: accumulator for binary exponent
 * 	(*)  d2: digit count
 * 	(*)  d3: offset pointer
 * 	( )  d4: first word of bcd
 * 	( )  a0: pointer to working bcd value
 * 	( )  a6: pointer to original bcd value
 * 	(*)  FP_SCR1: working copy of original bcd value
 * 	(*)  L_SCR1: copy of original exponent word
 */
calc_e:
	move.l	#EDIGITS,d2		/* # of nibbles (digits) in fraction part */
	moveq.l	#ESTRT,d3		/* counter to pick up digits */
	lea.l	FP_SCR1(a6),a0	/* load tmp bcd storage address */
	move.l	ETEMP(a6),(a0)	/* save input bcd value */
	move.l	ETEMP_HI(a6),4(a0) /* save words 2 and 3 */
	move.l	ETEMP_LO(a6),8(a0) /* and work with these */
	move.l	(a0),d4			/* get first word of bcd */
	clr.l	d1				/* zero d1 for accumulator */
e_gd:
	mulu.l	#TEN,d1			/* mul partial product by one digit place */
	bfextu	d4{d3:4},d0		/* get the digit and zero extend into d0 */
	add.l	d0,d1			/* d1 = d1 + d0 */
	addq.b	#4,d3			/* advance d3 to the next digit */
	dbf	d2,e_gd				/* if we have used all 3 digits, exit loop */
	btst	#30,d4			/* get SE */
	beq.s	e_pos			/* don't negate if pos */
	neg.l	d1				/* negate before subtracting */
e_pos:
	subi.l	#16,d1			/* sub to compensate for shift of mant */
	bge.s	e_save			/* if still pos, do not neg */
	neg.l	d1				/* now negative, make pos and set SE */
	ori.l	#0x40000000,d4	/* set SE in d4, */
	ori.l	#0x40000000,(a0)	/* and in working bcd */
e_save:
	move.l	d1,L_SCR1(a6)	/* save exp in memory */

/*
 *  Calculate mantissa:
 *   1. Calculate absolute value of mantissa in fp0 by mul and add.
 *   2. Correct for mantissa sign.
 *      (i.e., all digits assumed left of the decimal point.)
 *
 *  Register usage:
 *
 *   calc_m:
 * 	(*)  d0: temp digit storage
 * 	(*)  d1: lword counter
 * 	(*)  d2: digit count
 * 	(*)  d3: offset pointer
 * 	( )  d4: words 2 and 3 of bcd
 * 	( )  a0: pointer to working bcd value
 * 	( )  a6: pointer to original bcd value
 * 	(*) fp0: mantissa accumulator
 * 	( )  FP_SCR1: working copy of original bcd value
 * 	( )  L_SCR1: copy of original exponent word
 */
calc_m:
	moveq.l	#1,d1			/* word counter, init to 1 */
	fmove.s	#0,fp0		/* accumulator */

/*
 *   Since the packed number has a long word between the first & second parts,
 *   get the integer digit then skip down & get the rest of the
 *   mantissa.  We will unroll the loop once.
 */
	bfextu	(a0){28:4},d0	/* integer part is ls digit in long word */
	fadd.b	d0,fp0			/* add digit to sum in fp0 */

/*
 *   Get the rest of the mantissa.
 */
loadlw:
	move.l	(a0,d1.l*4),d4	/* load mantissa longword into d4 */
	moveq.l	#FSTRT,d3		/* counter to pick up digits */
	moveq.l	#FNIBS,d2		/* reset number of digits per a0 ptr */
md2b:
	fmul.s	FTEN.w(pc),fp0		/* fp0 = fp0 * 10 */
	bfextu	d4{d3:4},d0		/* get the digit and zero extend */
	fadd.b	d0,fp0			/* fp0 = fp0 + digit */

/*
 *   If all the digits (8) in that long word have been converted (d2=0),
 *   then inc d1 (=2) to point to the next long word and reset d3 to 0
 *   to initialize the digit offset, and set d2 to 7 for the digit count;
 *   else continue with this long word.
 */
	addq.b	#4,d3			/* advance d3 to the next digit */
	dbf	d2,md2b				/* check for last digit in this lw */
nextlw:
	addq.l	#1,d1			/* inc lw pointer in mantissa */
	cmpi.l	#2,d1			/* test for last lw */
	ble.s	loadlw			/* if not, get last one */

/*
 *   Check the sign of the mant and make the value in fp0 the same sign.
 */
m_sign:
	btst	#7,(a0)			/* test sign of the mantissa */
	beq.s	ap_st_z			/* if clear, go to append/strip zeros */
	fneg.x	fp0				/* if set, negate fp0 */

/*
 *  Append/strip zeros:
 *
 *   For adjusted exponents which have an absolute value greater than 27*,
 *   this routine calculates the amount needed to normalize the mantissa
 *   for the adjusted exponent.  That number is subtracted from the exp
 *   if the exp was positive, and added if it was negative.  The purpose
 *   of this is to reduce the value of the exponent and the possibility
 *   of error in calculation of pwrten.
 *
 *   1. Branch on the sign of the adjusted exponent.
 *   2p.(positive exp)
 *    2. Check M16 and the digits in lwords 2 and 3 in descending order.
 *    3. Add one for each zero encountered until a non-zero digit.
 *    4. Subtract the count from the exp.
 *    5. Check if the exp has crossed zero in #3 above; make the exp abs
 * 	   and set SE.
 * 	6. Multiply the mantissa by 10**count.
 *   2n.(negative exp)
 *    2. Check the digits in lwords 3 and 2 in descending order.
 *    3. Add one for each zero encountered until a non-zero digit.
 *    4. Add the count to the exp.
 *    5. Check if the exp has crossed zero in #3 above; clear SE.
 *    6. Divide the mantissa by 10**count.
 *
 *   *Why 27?  If the adjusted exponent is within -28 < expA < 28, than
 *    any adjustment due to append/strip zeros will drive the resultant
 *    exponent towards zero.  Since all pwrten constants with a power
 *    of 27 or less are exact, there is no need to use this routine to
 *    attempt to lessen the resultant exponent.
 *
 *  Register usage:
 *
 *   ap_st_z:
 * 	(*)  d0: temp digit storage
 * 	(*)  d1: zero count
 * 	(*)  d2: digit count
 * 	(*)  d3: offset pointer
 * 	( )  d4: first word of bcd
 * 	(*)  d5: lword counter
 * 	( )  a0: pointer to working bcd value
 * 	( )  FP_SCR1: working copy of original bcd value
 * 	( )  L_SCR1: copy of original exponent word
 *
 *
 *  First check the absolute value of the exponent to see if this
 *  routine is necessary.  If so, then check the sign of the exponent
 *  and do append (+) or strip (-) zeros accordingly.
 *  This section handles a positive adjusted exponent.
 */
ap_st_z:
	move.l	L_SCR1(a6),d1	/* load expA for range test */
	cmpi.l	#27,d1			/* test is with 27 */
	ble	pwrten				/* if abs(expA) <28, skip ap/st zeros */
	btst	#6,(a0)			/* check sign of exp */
	bne.s	ap_st_n			/* if neg, go to neg side */
	clr.l	d1				/* zero count reg */
	move.l	(a0),d4			/* load lword 1 to d4 */
	bfextu	d4{28:4},d0		/* get M16 in d0 */
	bne.s	ap_p_fx			/* if M16 is non-zero, go fix exp */
	addq.l	#1,d1			/* inc zero count */
	moveq.l	#1,d5			/* init lword counter */
	move.l	(a0,d5.L*4),d4	/* get lword 2 to d4 */
	bne.s	ap_p_cl			/* if lw 2 is zero, skip it */
	addq.l	#8,d1			/* and inc count by 8 */
	addq.l	#1,d5			/* inc lword counter */
	move.l	(a0,d5.L*4),d4	/* get lword 3 to d4 */
ap_p_cl:
	clr.l	d3				/* init offset reg */
	moveq.l	#7,d2			/* init digit counter */
ap_p_gd:
	bfextu	d4{d3:4},d0		/* get digit */
	bne.s	ap_p_fx			/* if non-zero, go to fix exp */
	addq.l	#4,d3			/* point to next digit */
	addq.l	#1,d1			/* inc digit counter */
	dbf	d2,ap_p_gd			/* get next digit */
ap_p_fx:
	move.l	d1,d0			/* copy counter to d2 */
	move.l	L_SCR1(a6),d1	/* get adjusted exp from memory */
	sub.l	d0,d1			/* subtract count from exp */
	bge.s	ap_p_fm			/* if still pos, go to pwrten */
	neg.l	d1				/* now its neg; get abs */
	move.l	(a0),d4			/* load lword 1 to d4 */
	ori.l	#0x40000000,d4	/*  and set SE in d4 */
	ori.l	#0x40000000,(a0)	/*  and in memory */
/*
 *  Calculate the mantissa multiplier to compensate for the striping of
 *  zeros from the mantissa.
 */
ap_p_fm:
	lea		PTENRN.w(pc),a1		/* get address of power-of-ten table */
	clr.l	d3				/* init table index */
	fmove.s	FONE.w(pc),fp1		/* init fp1 to 1 */
	moveq.l	#3,d2			/* init d2 to count bits in counter */
ap_p_el:
	asr.l	#1,d0			/* shift lsb into carry */
	bcc.s	ap_p_en			/* if 1, mul fp1 by pwrten factor */
	fmul.x	(a1,d3.w),fp1	/* mul by 10**(d3_bit_no) */
ap_p_en:
	addi.l	#12,d3			/* inc d3 to next rtable entry */
	tst.l	d0				/* check if d0 is zero */
	bne.s	ap_p_el			/* if not, get next bit */
	fmul.x	fp1,fp0			/* mul mantissa by 10**(no_bits_shifted) */
	bra.s	pwrten			/* go calc pwrten */
/*
 *  This section handles a negative adjusted exponent.
 */
ap_st_n:
	clr.l	d1				/* clr counter */
	moveq.l	#2,d5			/* set up d5 to point to lword 3 */
	move.l	(a0,d5.L*4),d4	/* get lword 3 */
	bne.s	ap_n_cl			/* if not zero, check digits */
	subq.l	#1,d5			/* dec d5 to point to lword 2 */
	addq.l	#8,d1			/* inc counter by 8 */
	move.l	(a0,d5.L*4),d4	/* get lword 2 */
ap_n_cl:
	move.l	#28,d3			/* point to last digit */
	moveq.l	#7,d2			/* init digit counter */
ap_n_gd:
	bfextu	d4{d3:4},d0		/* get digit */
	bne.s	ap_n_fx			/* if non-zero, go to exp fix */
	subq.l	#4,d3			/* point to previous digit */
	addq.l	#1,d1			/* inc digit counter */
	dbf	d2,ap_n_gd			/* get next digit */
ap_n_fx:
	move.l	d1,d0			/* copy counter to d0 */
	move.l	L_SCR1(a6),d1	/* get adjusted exp from memory */
	sub.l	d0,d1			/* subtract count from exp */
	bgt.s	ap_n_fm			/* if still pos, go fix mantissa */
	neg.l	d1				/* take abs of exp and clr SE */
	move.l	(a0),d4			/* load lword 1 to d4 */
	andi.l	#0xbfffffff,d4	/*  and clr SE in d4 */
	andi.l	#0xbfffffff,(a0)	/*  and in memory */
/*
 *  Calculate the mantissa multiplier to compensate for the appending of
 *  zeros to the mantissa.
 */
ap_n_fm:
	lea		PTENRN.w(pc),a1		/* get address of power-of-ten table */
	clr.l	d3				/* init table index */
	fmove.s	FONE.w(pc),fp1		/* init fp1 to 1 */
	moveq.l	#3,d2			/* init d2 to count bits in counter */
ap_n_el:
	asr.l	#1,d0			/* shift lsb into carry */
	bcc.s	ap_n_en			/* if 1, mul fp1 by pwrten factor */
	fmul.x	(a1,d3.w),fp1	/* mul by 10**(d3_bit_no) */
ap_n_en:
	addi.l	#12,d3			/* inc d3 to next rtable entry */
	tst.l	d0				/* check if d0 is zero */
	bne.s	ap_n_el			/* if not, get next bit */
	fdiv.x	fp1,fp0			/* div mantissa by 10**(no_bits_shifted) */

/*
 *  Calculate power-of-ten factor from adjusted and shifted exponent.
 *
 *  Register usage:
 *
 *   pwrten:
 * 	(*)  d0: temp
 * 	( )  d1: exponent
 * 	(*)  d2: {FPCR[6:5],SM,SE} as index in RTABLE; temp
 * 	(*)  d3: FPCR work copy
 * 	( )  d4: first word of bcd
 * 	(*)  a1: RTABLE pointer
 *   calc_p:
 * 	(*)  d0: temp
 * 	( )  d1: exponent
 * 	(*)  d3: PWRTxx table index
 * 	( )  a0: pointer to working copy of bcd
 * 	(*)  a1: PWRTxx pointer
 * 	(*) fp1: power-of-ten accumulator
 *
 *  Pwrten calculates the exponent factor in the selected rounding mode
 *  according to the following table:
 *
 * 	Sign of Mant  Sign of Exp  Rounding Mode  PWRTEN Rounding Mode
 *
 * 	ANY	  ANY	RN	RN
 *
 * 	 +	   +	RP	RP
 * 	 -	   +	RP	RM
 * 	 +	   -	RP	RM
 * 	 -	   -	RP	RP
 *
 * 	 +	   +	RM	RM
 * 	 -	   +	RM	RP
 * 	 +	   -	RM	RP
 * 	 -	   -	RM	RM
 *
 * 	 +	   +	RZ	RM
 * 	 -	   +	RZ	RM
 * 	 +	   -	RZ	RP
 * 	 -	   -	RZ	RP
 */
pwrten:
	move.l	USER_FPCR(a6),d3 	/* get user's FPCR */
	bfextu	d3{26:2},d2			/* isolate rounding mode bits */
	move.l	(a0),d4				/* reload 1st bcd word to d4 */
	asl.l	#2,d2				/* format d2 to be */
	bfextu	d4{0:2},d0			/*  {FPCR[6],FPCR[5],SM,SE} */
	add.l	d0,d2				/* in d2 as index into RTABLE */
	lea.l	RTABLE.w(pc),a1		/* load rtable base */
	move.b	(a1,d2.w),d0		/* load new rounding bits from table */
	clr.l	d3					/* clear d3 to force no exc and extended */
	bfins	d0,d3{26:2}			/* stuff new rounding bits in FPCR */
	fmove.l	d3,fpcr				/* write new FPCR */
	asr.l	#1,d0				/* write correct PTENxx table */
	bcc.s	not_rp_db			/* to a1 */
	lea.l	PTENRP.w(pc),a1		/* it is RP */
	bra.s	calc_p				/* go to init section */
not_rp_db:
	asr.l	#1,d0				/* keep checking */
	bcc.s	not_rm
	lea.l	PTENRM.w(pc),a1			/* it is RM */
	bra.s	calc_p				/* go to init section */
not_rm:
	lea.l	PTENRN.w(pc),a1		/* it is RN */
calc_p:
	move.l	d1,d0				/* copy exp to d0;use d0 */
	bpl.s	no_neg				/* if exp is negative, */
	neg.l	d0					/* invert it */
	ori.l	#0x40000000,(a0)	/* and set SE bit */
no_neg:
	clr.l	d3					/* table index */
	fmove.s	FONE.w(pc),fp1			/* init fp1 to 1 */
e_loop_db:
	asr.l	#1,d0				/* shift next bit into carry */
	bcc.s	e_next_db			/* if zero, skip the mul */
	fmul.x	(a1,d3.w),fp1		/* mul by 10**(d3_bit_no) */
e_next_db:
	addi.l	#12,d3				/* inc d3 to next rtable entry */
	tst.l	d0					/* check if d0 is zero */
	bne.s	e_loop_db			/* not zero, continue shifting */

/*
 *   Check the sign of the adjusted exp and make the value in fp0 the
 *   same sign. If the exp was pos then multiply fp1*fp0;
 *   else divide fp0/fp1.
 *
 *  Register Usage:
 *   norm:
 * 	( )  a0: pointer to working bcd value
 * 	(*) fp0: mantissa accumulator
 * 	( ) fp1: scaling factor - 10**(abs(exp))
 */
norm:
	btst	#6,(a0)				/* test the sign of the exponent */
	beq.s	mul					/* if clear, go to multiply */
div:
	fdiv.x	fp1,fp0				/* exp is negative, so divide mant by exp */
	bra.s	end_dec
mul:
	fmul.x	fp1,fp0				/* exp is positive, so multiply by exp */

/*
 *  Clean up and return with result in fp0.
 *
 *  If the final mul/div in decbin incurred an inex exception,
 *  it will be inex2, but will be reported as inex1 by get_op.
 */
end_dec:
	fmove.l	fpsr,d0				/* get status register */
	bclr.l	#inex2_bit+8,d0		/* test for inex2 and clear it */
	fmove.l	d0,fpsr				/* return status reg w/o inex2 */
	beq.s	no_exc_db			/* skip this if no exc */
	or.l	#inx1a_mask,USER_FPSR(a6) /* set inex1/ainex */
no_exc_db:
	movem.l	(a7)+,d2-d5
	rts
