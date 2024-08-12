/*
 *  XDEF ****************************************************************	
 * 	addsub_scaler2(): scale inputs to fadd/fsub such that no	
 * 			  OVFL/UNFL exceptions will result		
 * 									
 *  XREF ****************************************************************	
 * 	norm() - normalize mantissa after adjusting exponent		
 * 									
 *  INPUT ***************************************************************	
 * 	FP_SRC(a6) = fp op1(src)					
 * 	FP_DST(a6) = fp op2(dst)					
 * 									
 *  OUTPUT **************************************************************	
 * 	FP_SRC(a6) = fp op1 scaled(src)					
 * 	FP_DST(a6) = fp op2 scaled(dst)					
 * 	d0         = scale amount					
 * 									
 *  ALGORITHM ***********************************************************	
 * 	If the DST exponent is > the SRC exponent, set the DST exponent	
 *  equal to 0x3fff and scale the SRC exponent by the value that the	
 *  DST exponent was scaled by. If the SRC exponent is greater or equal,	
 *  do the opposite. Return this scale factor in d0.			
 * 	If the two exponents differ by > the number of mantissa bits	
 *  plus two, then set the smallest exponent to a very small value as a	
 *  quick shortcut.							
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		addsub_scaler2
addsub_scaler2:
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		DST_HI(a1),FP_SCR1_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	move.l		DST_LO(a1),FP_SCR1_LO(a6)
	move.w		SRC_EX.w(a0),d0
	move.w		DST_EX.w(a1),d1
	move.w		d0,FP_SCR0_EX(a6)
	move.w		d1,FP_SCR1_EX(a6)

	andi.w		#0x7fff,d0
	andi.w		#0x7fff,d1
	move.w		d0,L_SCR1(a6)		/*  store src exponent */
	move.w		d1,2+L_SCR1(a6)	/*  store dst exponent */

	cmp.w		d1,d0		/*  is src exp >= dst exp? */
	bge.l		src_exp_ge2

/*  dst exp is >  src exp; scale dst to exp = 0x3fff */
dst_exp_gt2:
	bsr.l		scale_to_zero_dst
	move.l		d0,-(sp)		/*  save scale factor */

	cmpi.b		#DENORM,STAG(a6)	/*  is dst denormalized? */
	bne.b		cmpexp12

	lea		FP_SCR0(a6),a0
	bsr.l		norm			/*  normalize the denorm; result is new exp */
	neg.w		d0			/*  new exp = -(shft val) */
	move.w		d0,L_SCR1(a6)		/*  inset new exp */

cmpexp12:
	move.w		2+L_SCR1(a6),d0
	subi.w		#mantissalen+2,d0	/*  subtract mantissalen+2 from larger exp */

	cmp.w		L_SCR1(a6),d0		/*  is difference >= len(mantissa)+2? */
	bge.b		quick_scale12

	move.w		L_SCR1(a6),d0
	add.w		0x2(sp),d0		/*  scale src exponent by scale factor */
	move.w		FP_SCR0_EX(a6),d1
	andi.w		#0x8000,d1
	or.w		d1,d0			/*  concat {sgn,new exp} */
	move.w		d0,FP_SCR0_EX(a6)	/*  insert new dst exponent */

	move.l		(sp)+,d0		/*  return SCALE factor */
	rts

quick_scale12:
	andi.w		#0x8000,FP_SCR0_EX(a6)	/*  zero src exponent */
	bset		#0x0,1+FP_SCR0_EX(a6)	/*  set exp = 1 */

	move.l		(sp)+,d0		/*  return SCALE factor */
	rts

/*  src exp is >= dst exp; scale src to exp = 0x3fff */
src_exp_ge2:
	bsr.l		scale_to_zero_src
	move.l		d0,-(sp)		/*  save scale factor */

	cmpi.b		#DENORM,DTAG(a6)	/*  is dst denormalized? */
	bne.b		cmpexp22
	lea		FP_SCR1(a6),a0
	bsr.l		norm			/*  normalize the denorm; result is new exp */
	neg.w		d0			/*  new exp = -(shft val) */
	move.w		d0,2+L_SCR1(a6)	/*  inset new exp */

cmpexp22:
	move.w		L_SCR1(a6),d0
	subi.w		#mantissalen+2,d0	/*  subtract mantissalen+2 from larger exp */

	cmp.w		2+L_SCR1(a6),d0	/*  is difference >= len(mantissa)+2? */
	bge.b		quick_scale22

	move.w		2+L_SCR1(a6),d0
	add.w		0x2(sp),d0		/*  scale dst exponent by scale factor */
	move.w		FP_SCR1_EX(a6),d1
	andi.w		#0x8000,d1
	or.w		d1,d0			/*  concat {sgn,new exp} */
	move.w		d0,FP_SCR1_EX(a6)	/*  insert new dst exponent */

	move.l		(sp)+,d0		/*  return SCALE factor */
	rts

quick_scale22:
	andi.w		#0x8000,FP_SCR1_EX(a6)	/*  zero dst exponent */
	bset		#0x0,1+FP_SCR1_EX(a6)	/*  set exp = 1 */

	move.l		(sp)+,d0		/*  return SCALE factor */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	scale_to_zero_src(): scale the exponent of extended precision	 */
/* 			     value at FP_SCR0(a6).			 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	norm() - normalize the mantissa if the operand was a DENORM	 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	FP_SCR0(a6) = extended precision operand to be scaled		 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	FP_SCR0(a6) = scaled extended precision operand			 */
/* 	d0	    = scale value					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Set the exponent of the input operand to 0x3fff. Save the value	 */
/*  of the difference between the original and new exponent. Then,	 */
/*  normalize the operand if it was a DENORM. Add this normalization	 */
/*  value to the previous value. Return the result.			 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		scale_to_zero_src
scale_to_zero_src:
	move.w		FP_SCR0_EX(a6),d1	/*  extract operand's {sgn,exp} */
	move.w		d1,d0			/*  make a copy */

	andi.l		#0x7fff,d1		/*  extract operand's exponent */

	andi.w		#0x8000,d0		/*  extract operand's sgn */
	ori.w		#0x3fff,d0		/*  insert new operand's exponent(=0) */

	move.w		d0,FP_SCR0_EX(a6)	/*  insert biased exponent */

	cmpi.b		#DENORM,STAG(a6)	/*  is operand normalized? */
	beq.b		stzs_denorm		/*  normalize the DENORM */

stzs_norm:
	move.l		#0x3fff,d0
	sub.l		d1,d0			/*  scale = BIAS + (-exp) */

	rts

stzs_denorm:
	lea		FP_SCR0(a6),a0	/*  pass ptr to src op */
	bsr.l		norm			/*  normalize denorm */
	neg.l		d0			/*  new exponent = -(shft val) */
	move.l		d0,d1			/*  prepare for op_norm call */
	bra.b		stzs_norm		/*  finish scaling */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	scale_sqrt(): scale the input operand exponent so a subsequent	 */
/* 		      fsqrt operation won't take an exception.		 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	norm() - normalize the mantissa if the operand was a DENORM	 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	FP_SCR0(a6) = extended precision operand to be scaled		 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	FP_SCR0(a6) = scaled extended precision operand			 */
/* 	d0	    = scale value					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	If the input operand is a DENORM, normalize it.			 */
/* 	If the exponent of the input operand is even, set the exponent	 */
/*  to 0x3ffe and return a scale factor of "(exp-0x3ffe)/2". If the	 */
/*  exponent of the input operand is off, set the exponent to ox3fff and	 */
/*  return a scale factor of "(exp-0x3fff)/2".				 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		scale_sqrt
scale_sqrt:
	cmpi.b		#DENORM,STAG(a6)	/*  is operand normalized? */
	beq.b		ss_denorm		/*  normalize the DENORM */

	move.w		FP_SCR0_EX(a6),d1	/*  extract operand's {sgn,exp} */
	andi.l		#0x7fff,d1		/*  extract operand's exponent */

	andi.w		#0x8000,FP_SCR0_EX(a6)	/*  extract operand's sgn */

	btst		#0x0,d1		/*  is exp even or odd? */
	beq.b		ss_norm_even

	ori.w		#0x3fff,FP_SCR0_EX(a6)	/*  insert new operand's exponent(=0) */

	move.l		#0x3fff,d0
	sub.l		d1,d0			/*  scale = BIAS + (-exp) */
	asr.l		#0x1,d0		/*  divide scale factor by 2 */
	rts

ss_norm_even:
	ori.w		#0x3ffe,FP_SCR0_EX(a6)	/*  insert new operand's exponent(=0) */

	move.l		#0x3ffe,d0
	sub.l		d1,d0			/*  scale = BIAS + (-exp) */
	asr.l		#0x1,d0		/*  divide scale factor by 2 */
	rts

ss_denorm:
	lea		FP_SCR0(a6),a0	/*  pass ptr to src op */
	bsr.l		norm			/*  normalize denorm */

	btst		#0x0,d0		/*  is exp even or odd? */
	beq.b		ss_denorm_even

	ori.w		#0x3fff,FP_SCR0_EX(a6)	/*  insert new operand's exponent(=0) */

	addi.l		#0x3fff,d0
	asr.l		#0x1,d0		/*  divide scale factor by 2 */
	rts

ss_denorm_even:
	ori.w		#0x3ffe,FP_SCR0_EX(a6)	/*  insert new operand's exponent(=0) */

	addi.l		#0x3ffe,d0
	asr.l		#0x1,d0		/*  divide scale factor by 2 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	scale_to_zero_dst(): scale the exponent of extended precision	 */
/* 			     value at FP_SCR1(a6).			 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	norm() - normalize the mantissa if the operand was a DENORM	 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	FP_SCR1(a6) = extended precision operand to be scaled		 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	FP_SCR1(a6) = scaled extended precision operand			 */
/* 	d0	    = scale value					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Set the exponent of the input operand to 0x3fff. Save the value	 */
/*  of the difference between the original and new exponent. Then,	 */
/*  normalize the operand if it was a DENORM. Add this normalization	 */
/*  value to the previous value. Return the result.			 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		scale_to_zero_dst
scale_to_zero_dst:
	move.w		FP_SCR1_EX(a6),d1	/*  extract operand's {sgn,exp} */
	move.w		d1,d0			/*  make a copy */

	andi.l		#0x7fff,d1		/*  extract operand's exponent */

	andi.w		#0x8000,d0		/*  extract operand's sgn */
	ori.w		#0x3fff,d0		/*  insert new operand's exponent(=0) */

	move.w		d0,FP_SCR1_EX(a6)	/*  insert biased exponent */

	cmpi.b		#DENORM,DTAG(a6)	/*  is operand normalized? */
	beq.b		stzd_denorm		/*  normalize the DENORM */

stzd_norm:
	move.l		#0x3fff,d0
	sub.l		d1,d0			/*  scale = BIAS + (-exp) */
	rts

stzd_denorm:
	lea		FP_SCR1(a6),a0	/*  pass ptr to dst op */
	bsr.l		norm			/*  normalize denorm */
	neg.l		d0			/*  new exponent = -(shft val) */
	move.l		d0,d1			/*  prepare for op_norm call */
	bra.b		stzd_norm		/*  finish scaling */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	res_qnan(): return default result w/ QNAN operand for dyadic	 */
/* 	res_snan(): return default result w/ SNAN operand for dyadic	 */
/* 	res_qnan_1op(): return dflt result w/ QNAN operand for monadic	 */
/* 	res_snan_1op(): return dflt result w/ SNAN operand for monadic	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	FP_SRC(a6) = pointer to extended precision src operand		 */
/* 	FP_DST(a6) = pointer to extended precision dst operand		 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	fp0 = default result						 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	If either operand (but not both operands) of an operation is a	 */
/*  nonsignalling NAN, then that NAN is returned as the result. If both	 */
/*  operands are nonsignalling NANs, then the destination operand		 */
/*  nonsignalling NAN is returned as the result.				 */
/* 	If either operand to an operation is a signalling NAN (SNAN),	 */
/*  then, the SNAN bit is set in the FPSR EXC byte. If the SNAN trap	 */
/*  enable bit is set in the FPCR, then the trap is taken and the		 */
/*  destination is not modified. If the SNAN trap enable bit is not set,	 */
/*  then the SNAN is converted to a nonsignalling NAN (by setting the	 */
/*  SNAN bit in the operand to one), and the operation continues as	 */
/*  described in the preceding paragraph, for nonsignalling NANs.		 */
/* 	Make sure the appropriate FPSR bits are set before exiting.	 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		res_qnan
	.globl		res_snan
res_qnan:
res_snan:
	cmpi.b		#SNAN,DTAG(a6)	/*  is the dst an SNAN? */
	beq.b		dst_snan2
	cmpi.b		#QNAN,DTAG(a6)	/*  is the dst a  QNAN? */
	beq.b		dst_qnan2
src_nan:
	cmpi.b		#QNAN,STAG(a6)
	beq.b		src_qnan2
	.globl		res_snan_1op
res_snan_1op:
src_snan2:
	bset		#0x6,FP_SRC_HI(a6)	/*  set SNAN bit */
	ori.l		#nan_mask+aiop_mask+snan_mask, USER_FPSR(a6)
	lea		FP_SRC(a6),a0
	bra.b		nan_comp
	.globl		res_qnan_1op
res_qnan_1op:
src_qnan2:
	ori.l		#nan_mask,USER_FPSR(a6)
	lea		FP_SRC(a6),a0
	bra.b		nan_comp
dst_snan2:
	ori.l		#nan_mask+aiop_mask+snan_mask, USER_FPSR(a6)
	bset		#0x6,FP_DST_HI(a6)	/*  set SNAN bit */
	lea		FP_DST(a6),a0
	bra.b		nan_comp
dst_qnan2:
	lea		FP_DST(a6), a0
	cmpi.b		#SNAN,STAG(a6)
	bne.l		nan_done
	ori.l		#aiop_mask+snan_mask, USER_FPSR(a6)
nan_done:
	ori.l		#nan_mask,USER_FPSR(a6)
nan_comp:
	btst		#0x7,FTEMP_EX.w(a0)	/*  is NAN neg? */
	beq.b		nan_not_neg
	ori.l		#neg_mask,USER_FPSR(a6)
nan_not_neg:
	fmovem.x	(a0),fp0
	rts

/*
 * ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 *  XDEF ****************************************************************
 *      res_operr(): return default result during operand error
 *
 *  XREF ****************************************************************
 *      None
 *
 *  INPUT ***************************************************************
 *      None
 *
 *  OUTPUT **************************************************************
 *      fp0 = default operand error result
 *
 *  ALGORITHM ***********************************************************
 *      An nonsignalling NAN is returned as the default result when
 *  an operand error occurs for the following cases:
 *
 *      Multiply: (Infinity x Zero)
 *      Divide  : (Zero / Zero) || (Infinity / Infinity)
 *
 * ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 */

	.globl		res_operr
res_operr:
	or.l		#nan_mask+operr_mask+aiop_mask,USER_FPSR(a6)
	/* fmovem.x		(nan_return).l(pc),fp0 */
	.dc.w 0xf23b,0xd080,0x0170 /* rejected by gas */
	.dc.l nan_return-.+2
	rts

nan_return:
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff

