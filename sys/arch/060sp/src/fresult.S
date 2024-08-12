/*
 *  XDEF ****************************************************************	
 * 	unf_res(): routine to produce default underflow result of a	
 * 		   scaled extended precision number *  this is used by	
 * 		   fadd/fdiv/fmul/etc. emulation routines.		
 * 	unf_res4(): same as above but for fsglmul/fsgldiv which use	
 * 		    single round prec and extended prec mode.		
 * 									
 *  XREF ****************************************************************	
 * 	_denorm() - denormalize according to scale factor		
 * 	_round() - round denormalized number according to rnd prec	
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precison operand			
 * 	d0 = scale factor						
 * 	d1 = rounding precision/mode					
 * 									
 *  OUTPUT **************************************************************	
 * 	a0 = pointer to default underflow result in extended precision	
 * 	d0.b = result FPSR_cc which caller may or may not want to save	
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Convert the input operand to "internal format" which means the	
 *  exponent is extended to 16 bits and the sign is stored in the unused	
 *  portion of the extended precison operand. Denormalize the number	
 *  according to the scale factor passed in d0. Then, round the		
 *  denormalized result.							
 * 	Set the FPSR_exc bits as appropriate but return the cc bits in	
 *  d0 in case the caller doesn't want to save them (as is the case for	
 *  fmove out).								
 * 	unf_res4() for fsglmul/fsgldiv forces the denorm to extended	
 *  precision and the rounding mode to single.				
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		unf_res
unf_res:
	move.l		d1, -(sp)		/*  save rnd prec,mode on stack */

	btst		#0x7, FTEMP_EX.w(a0)	/*  make "internal" format */
	sne		FTEMP_SGN(a0)

	move.w		FTEMP_EX.w(a0), d1	/*  extract exponent */
	andi.w		#0x7fff, d1
	sub.w		d0, d1
	move.w		d1, FTEMP_EX.w(a0)	/*  insert 16 bit exponent */

	move.l		a0, -(sp)		/*  save operand ptr during calls */

	move.l		0x4(sp),d0		/*  pass rnd prec. */
	andi.w		#0x00c0,d0
	lsr.w		#0x4,d0
	bsr.l		_denorm			/*  denorm result */

	move.l		(sp),a0
	move.w		0x6(sp),d1		/*  load prec:mode into d1 */
	andi.w		#0xc0,d1		/*  extract rnd prec */
	lsr.w		#0x4,d1
	swap		d1
	move.w		0x6(sp),d1
	andi.w		#0x30,d1
	lsr.w		#0x4,d1
	bsr.l		_round			/*  round the denorm */

	move.l		(sp)+, a0

/*  result is now rounded properly. convert back to normal format */
	bclr		#0x7, FTEMP_EX.w(a0)	/*  clear sgn first; may have residue */
	tst.b		FTEMP_SGN(a0)		/*  is "internal result" sign set? */
	beq.b		unf_res_chkifzero	/*  no; result is positive */
	bset		#0x7, FTEMP_EX.w(a0)	/*  set result sgn */
	clr.b		FTEMP_SGN(a0)		/*  clear temp sign */

/*  the number may have become zero after rounding. set ccodes accordingly. */
unf_res_chkifzero:
	clr.l		d0
	tst.l		FTEMP_HI(a0)		/*  is value now a zero? */
	bne.b		unf_res_cont		/*  no */
	tst.l		FTEMP_LO(a0)
	bne.b		unf_res_cont		/*  no */
/* 	bset		#z_bit, FPSR_CC(a6)	; yes; set zero ccode bit */
	bset		#z_bit, d0		/*  yes; set zero ccode bit */

unf_res_cont:

/*
 * can inex1 also be set along with unfl and inex2???
 *
 * we know that underflow has occurred. aunfl should be set if INEX2 is also set.
 */
	btst		#inex2_bit, FPSR_EXCEPT(a6) /*  is INEX2 set? */
	beq.b		unf_res_end		/*  no */
	bset		#aunfl_bit, FPSR_AEXCEPT(a6) /*  yes; set aunfl */

unf_res_end:
	addq.l		#0x4,sp		/*  clear stack */
	rts

/*  unf_res() for fsglmul() and fsgldiv(). */
	.globl		unf_res4
unf_res4:
	move.l		d1,-(sp)		/*  save rnd prec,mode on stack */

	btst		#0x7,FTEMP_EX.w(a0)	/*  make "internal" format */
	sne		FTEMP_SGN(a0)

	move.w		FTEMP_EX.w(a0),d1	/*  extract exponent */
	andi.w		#0x7fff,d1
	sub.w		d0,d1
	move.w		d1,FTEMP_EX.w(a0)	/*  insert 16 bit exponent */

	move.l		a0,-(sp)		/*  save operand ptr during calls */

	clr.l		d0			/*  force rnd prec = ext */
	bsr.l		_denorm			/*  denorm result */

	move.l		(sp),a0
	move.w		#s_mode,d1		/*  force rnd prec = sgl */
	swap		d1
	move.w		0x6(sp),d1		/*  load rnd mode */
	andi.w		#0x30,d1		/*  extract rnd prec */
	lsr.w		#0x4,d1
	bsr.l		_round			/*  round the denorm */

	move.l		(sp)+,a0

/*  result is now rounded properly. convert back to normal format */
	bclr		#0x7,FTEMP_EX.w(a0)	/*  clear sgn first; may have residue */
	tst.b		FTEMP_SGN(a0)		/*  is "internal result" sign set? */
	beq.b		unf_res4_chkifzero	/*  no; result is positive */
	bset		#0x7,FTEMP_EX.w(a0)	/*  set result sgn */
	clr.b		FTEMP_SGN(a0)		/*  clear temp sign */

/*  the number may have become zero after rounding. set ccodes accordingly. */
unf_res4_chkifzero:
	clr.l		d0
	tst.l		FTEMP_HI(a0)		/*  is value now a zero? */
	bne.b		unf_res4_cont		/*  no */
	tst.l		FTEMP_LO(a0)
	bne.b		unf_res4_cont		/*  no */
/* 	bset		#z_bit,FPSR_CC(a6)	; yes; set zero ccode bit */
	bset		#z_bit,d0		/*  yes; set zero ccode bit */

unf_res4_cont:

/*
 * can inex1 also be set along with unfl and inex2???
 *
 * we know that underflow has occurred. aunfl should be set if INEX2 is also set.
 */
	btst		#inex2_bit,FPSR_EXCEPT(a6) /*  is INEX2 set? */
	beq.b		unf_res4_end		/*  no */
	bset		#aunfl_bit,FPSR_AEXCEPT(a6) /*  yes; set aunfl */

unf_res4_end:
	addq.l		#0x4,sp		/*  clear stack */
	rts

/*
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 * XDEF ****************************************************************
 *	ovf_res(): routine to produce the default overflow result of
 *		   an overflowing number.
 *	ovf_res2(): same as above but the rnd mode/prec are passed
 *		    differently.
 *
 * XREF ****************************************************************
 *	none
 *
 * INPUT ***************************************************************
 *	d1.b	= '-1' => (-); '0' => (+)
 *   ovf_res():
 *	d0	= rnd mode/prec
 *   ovf_res2():
 *	hi(d0)	= rnd prec
 *	lo(d0)	= rnd mode
 *
 * OUTPUT **************************************************************
 *	a0	= points to extended precision result
 *	d0.b	= condition code bits
 *
 * ALGORITHM ***********************************************************
 *	The default overflow result can be determined by the sign of
 * the result and the rounding mode/prec in effect. These bits are
 * concatenated together to create an index into the default result
 * table. A pointer to the correct result is returned in a0. The
 * resulting condition codes are returned in d0 in case the caller
 * doesn't want FPSR_cc altered (as is the case for fmove out).
 *
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 */
	.globl		ovf_res
ovf_res:
	andi.w		#0x10,d1		/*  keep result sign */
	lsr.b		#0x4,d0		/*  shift prec/mode */
	or.b		d0,d1			/*  concat the two */
	move.w		d1,d0			/*  make a copy */
	lsl.b		#0x1,d1		/*  multiply d1 by 2 */
	bra.b		ovf_res_load

	.globl		ovf_res2
ovf_res2:
	andi.w		#0x10, d1		/*  keep result sign */
	or.b		d0, d1		/*  insert rnd mode */
	swap		d0
	or.b		d0, d1		/*  insert rnd prec */
	move.w		d1, d0		/*  make a copy */
	lsl.b		#0x1, d1		/*  shift left by 1 */

/*
 * use the rounding mode, precision, and result sign as in index into the
 * two tables below to fetch the default result and the result ccodes.
 */
ovf_res_load:
	move.b		(tbl_ovfl_cc.b,pc,d0.w*1),d0 /*  fetch result ccodes */
	lea		(tbl_ovfl_result.b,pc,d1.w*8),a0 /*  return result ptr */

	rts

tbl_ovfl_cc:
	.dc.b		0x2, 0x0, 0x0, 0x2
	.dc.b		0x2, 0x0, 0x0, 0x2
	.dc.b		0x2, 0x0, 0x0, 0x2
	.dc.b		0x0, 0x0, 0x0, 0x0
	.dc.b		0x2+0x8, 0x8, 0x2+0x8, 0x8
	.dc.b		0x2+0x8, 0x8, 0x2+0x8, 0x8
	.dc.b		0x2+0x8, 0x8, 0x2+0x8, 0x8

tbl_ovfl_result:
	.dc.l		0x7fff0000,0x00000000,0x00000000,0x00000000 /*  +INF; RN */
	.dc.l		0x7ffe0000,0xffffffff,0xffffffff,0x00000000 /*  +EXT; RZ */
	.dc.l		0x7ffe0000,0xffffffff,0xffffffff,0x00000000 /*  +EXT; RM */
	.dc.l		0x7fff0000,0x00000000,0x00000000,0x00000000 /*  +INF; RP */

	.dc.l		0x7fff0000,0x00000000,0x00000000,0x00000000 /*  +INF; RN */
	.dc.l		0x407e0000,0xffffff00,0x00000000,0x00000000 /*  +SGL; RZ */
	.dc.l		0x407e0000,0xffffff00,0x00000000,0x00000000 /*  +SGL; RM */
	.dc.l		0x7fff0000,0x00000000,0x00000000,0x00000000 /*  +INF; RP */

	.dc.l		0x7fff0000,0x00000000,0x00000000,0x00000000 /*  +INF; RN */
	.dc.l		0x43fe0000,0xffffffff,0xfffff800,0x00000000 /*  +DBL; RZ */
	.dc.l		0x43fe0000,0xffffffff,0xfffff800,0x00000000 /*  +DBL; RM */
	.dc.l		0x7fff0000,0x00000000,0x00000000,0x00000000 /*  +INF; RP */

	.dc.l		0x00000000,0x00000000,0x00000000,0x00000000
	.dc.l		0x00000000,0x00000000,0x00000000,0x00000000
	.dc.l		0x00000000,0x00000000,0x00000000,0x00000000
	.dc.l		0x00000000,0x00000000,0x00000000,0x00000000

	.dc.l		0xffff0000,0x00000000,0x00000000,0x00000000 /*  -INF; RN */
	.dc.l		0xfffe0000,0xffffffff,0xffffffff,0x00000000 /*  -EXT; RZ */
	.dc.l		0xffff0000,0x00000000,0x00000000,0x00000000 /*  -INF; RM */
	.dc.l		0xfffe0000,0xffffffff,0xffffffff,0x00000000 /*  -EXT; RP */

	.dc.l		0xffff0000,0x00000000,0x00000000,0x00000000 /*  -INF; RN */
	.dc.l		0xc07e0000,0xffffff00,0x00000000,0x00000000 /*  -SGL; RZ */
	.dc.l		0xffff0000,0x00000000,0x00000000,0x00000000 /*  -INF; RM */
	.dc.l		0xc07e0000,0xffffff00,0x00000000,0x00000000 /*  -SGL; RP */

	.dc.l		0xffff0000,0x00000000,0x00000000,0x00000000 /*  -INF; RN */
	.dc.l		0xc3fe0000,0xffffffff,0xfffff800,0x00000000 /*  -DBL; RZ */
	.dc.l		0xffff0000,0x00000000,0x00000000,0x00000000 /*  -INF; RM */
	.dc.l		0xc3fe0000,0xffffffff,0xfffff800,0x00000000 /*  -DBL; RP */
