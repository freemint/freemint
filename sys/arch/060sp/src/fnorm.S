/*
 *  norm(): normalize the mantissa of an extended precision input. the	
 * 	  input operand should not be normalized already.		
 * 									
 *  XDEF ****************************************************************	
 * 	norm()								
 * 									
 *  XREF **************************************************************** 
 * 	none								
 * 									
 *  INPUT *************************************************************** 
 * 	a0 = pointer fp extended precision operand to normalize		
 * 									
 *  OUTPUT ************************************************************** 
 * 	d0 = number of bit positions the mantissa was shifted		
 * 	a0 = the input operand's mantissa is normalized *  the exponent	
 * 	     is unchanged.						
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		norm
norm:
	move.l		d2, -(sp)		/*  create some temp regs */
	move.l		d3, -(sp)

	move.l		FTEMP_HI(a0), d0	/*  load hi(mantissa) */
	move.l		FTEMP_LO(a0), d1	/*  load lo(mantissa) */

	bfffo		d0{0:32}, d2	/*  how many places to shift? */
	beq.b		norm_lo			/*  hi(man) is all zeroes! */

norm_hi:
	lsl.l		d2, d0		/*  left shift hi(man) */
	bfextu		d1{0:d2}, d3	/*  extract lo bits */

	or.l		d3, d0		/*  create hi(man) */
	lsl.l		d2, d1		/*  create lo(man) */

	move.l		d0, FTEMP_HI(a0)	/*  store new hi(man) */
	move.l		d1, FTEMP_LO(a0)	/*  store new lo(man) */

	move.l		d2, d0		/*  return shift amount */

	move.l		(sp)+,d3		/*  restore temp regs */
	move.l		(sp)+,d2

	rts

norm_lo:
	bfffo		d1{0:32}, d2	/*  how many places to shift? */
	lsl.l		d2,d1		/*  shift lo(man) */
	addi.l		#32,d2		/*  add 32 to shft amount */

	move.l		d1, FTEMP_HI(a0)	/*  store hi(man) */
	clr.l		FTEMP_LO(a0)		/*  lo(man) is now zero */

	move.l		d2, d0		/*  return shift amount */

	move.l		(sp)+, d3		/*  restore temp regs */
	move.l		(sp)+, d2

	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  unnorm_fix(): - changes an UNNORM to one of NORM, DENORM, or ZERO	 */
/* 		- returns corresponding optype tag			 */
/* 									 */
/*  XDEF ****************************************************************	 */
/* 	unnorm_fix()							 */
/* 									 */
/*  XREF ****************************************************************  */
/* 	norm() - normalize the mantissa					 */
/* 									 */
/*  INPUT ***************************************************************  */
/* 	a0 = pointer to unnormalized extended precision number		 */
/* 									 */
/*  OUTPUT **************************************************************  */
/* 	d0 = optype tag - is corrected to one of NORM, DENORM, or ZERO	 */
/* 	a0 = input operand has been converted to a norm, denorm, or	 */
/* 	     zero; both the exponent and mantissa are changed.		 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		unnorm_fix
unnorm_fix:
	bfffo		FTEMP_HI(a0){0:32}, d0 /*  how many shifts are needed? */
	bne.b		unnorm_shift		/*  hi(man) is not all zeroes */

/*
 * hi(man) is all zeroes so see if any bits in lo(man) are set
 */
unnorm_chk_lo:
	bfffo		FTEMP_LO(a0){0:32}, d0 /*  is operand really a zero? */
	beq.w		unnorm_zero		/*  yes */

	addi.w		#32,d0		/*  no; fix shift distance */

/*
 * d0 = ; shifts needed for complete normalization
 */
unnorm_shift:
	clr.l		d1			/*  clear top word */
	move.w		FTEMP_EX.w(a0), d1	/*  extract exponent */
	andi.w		#0x7fff, d1		/*  strip off sgn */

	cmp.w		d1,d0		/*  will denorm push exp < 0? */
	bgt.b		unnorm_nrm_zero		/*  yes; denorm only until exp = 0 */

/*
 * exponent would not go < 0. Therefore, number stays normalized
 */
	sub.w		d0, d1		/*  shift exponent value */
	move.w		FTEMP_EX.w(a0), d0	/*  load old exponent */
	andi.w		#0x8000, d0		/*  save old sign */
	or.w		d0, d1		/*  {sgn,new exp} */
	move.w		d1, FTEMP_EX.w(a0)	/*  insert new exponent */

	bsr.l		norm			/*  normalize UNNORM */

	move.b		#NORM, d0		/*  return new optype tag */
	rts

/*
 * exponent would go < 0, so only denormalize until exp = 0
 */
unnorm_nrm_zero:
	cmpi.b		#32,d1		/*  is exp <= 32? */
	bgt.b		unnorm_nrm_zero_lrg	/*  no; go handle large exponent */

	bfextu		FTEMP_HI(a0){d1:32}, d0 /*  extract new hi(man) */
	move.l		d0, FTEMP_HI(a0)	/*  save new hi(man) */

	move.l		FTEMP_LO(a0), d0	/*  fetch old lo(man) */
	lsl.l		d1, d0		/*  extract new lo(man) */
	move.l		d0, FTEMP_LO(a0)	/*  save new lo(man) */

	and.w		#0x8000, FTEMP_EX.w(a0)	/*  set exp = 0 */

	move.b		#DENORM, d0		/*  return new optype tag */
	rts

/*
 * only mantissa bits set are in lo(man)
 */
unnorm_nrm_zero_lrg:
	subi.w		#32,d1				/*  adjust shft amt by 32 */

	move.l		FTEMP_LO(a0), d0	/*  fetch old lo(man) */
	lsl.l		d1, d0		/*  left shift lo(man) */

	move.l		d0, FTEMP_HI(a0)	/*  store new hi(man) */
	clr.l		FTEMP_LO(a0)		/*  lo(man) = 0 */

	and.w		#0x8000, FTEMP_EX.w(a0)	/*  set exp = 0 */

	move.b		#DENORM, d0		/*  return new optype tag */
	rts

/*
 * whole mantissa is zero so this UNNORM is actually a zero
 */
unnorm_zero:
	and.w		#0x8000, FTEMP_EX.w(a0)	/*  force exponent to zero */

	move.b		#ZERO, d0		/*  fix optype tag */
	rts

