/*
 *  XDEF ****************************************************************	
 * 	set_tag_x(): return the optype of the input ext fp number	
 * 									
 *  XREF ****************************************************************	
 * 	None								
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision operand			
 * 									
 *  OUTPUT **************************************************************	
 * 	d0 = value of type tag						
 * 		one of: NORM, INF, QNAN, SNAN, DENORM, UNNORM, ZERO	
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Simply test the exponent, j-bit, and mantissa values to		
 *  determine the type of operand.					
 * 	If it's an unnormalized zero, alter the operand and force it	
 *  to be a normal zero.							
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		set_tag_x
set_tag_x:
	move.w		FTEMP_EX.w(a0), d0	/*  extract exponent */
	andi.w		#0x7fff, d0		/*  strip off sign */
	cmpi.w		#0x7fff,d0		/*  is (EXP == MAX)? */
	beq.b		inf_or_nan_x
not_inf_or_nan_x:
	btst		#0x7,FTEMP_HI(a0)
	beq.b		not_norm_x
is_norm_x:
	move.b		#NORM, d0
	rts
not_norm_x:
	tst.w		d0			/*  is exponent = 0? */
	bne.b		is_unnorm_x
not_unnorm_x:
	tst.l		FTEMP_HI(a0)
	bne.b		is_denorm_x
	tst.l		FTEMP_LO(a0)
	bne.b		is_denorm_x
is_zero_x:
	move.b		#ZERO, d0
	rts
is_denorm_x:
	move.b		#DENORM, d0
	rts
/*  must distinguish now "Unnormalized zeroes" which we */
/*  must convert to zero. */
is_unnorm_x:
	tst.l		FTEMP_HI(a0)
	bne.b		is_unnorm_reg_x
	tst.l		FTEMP_LO(a0)
	bne.b		is_unnorm_reg_x
/*  it's an "unnormalized zero". let's convert it to an actual zero... */
	andi.w		#0x8000,FTEMP_EX.w(a0)	/*  clear exponent */
	move.b		#ZERO, d0
	rts
is_unnorm_reg_x:
	move.b		#UNNORM, d0
	rts
inf_or_nan_x:
	tst.l		FTEMP_LO(a0)
	bne.b		is_nan_x
	move.l		FTEMP_HI(a0), d0
	andi.l		#0x7fffffff, d0	/*  msb is a don't care! */
	bne.b		is_nan_x
is_inf_x:
	move.b		#INF, d0
	rts
is_nan_x:
	btst		#0x6, FTEMP_HI(a0)
	beq.b		is_snan_x
	move.b		#QNAN, d0
	rts
is_snan_x:
	move.b		#SNAN, d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	set_tag_d(): return the optype of the input dbl fp number	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	a0 = points to double precision operand				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	d0 = value of type tag						 */
/* 		one of: NORM, INF, QNAN, SNAN, DENORM, ZERO		 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Simply test the exponent, j-bit, and mantissa values to		 */
/*  determine the type of operand.					 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		set_tag_d
set_tag_d:
	move.l		FTEMP.w(a0), d0
	move.l		d0, d1

	andi.l		#0x7ff00000, d0
	beq.b		zero_or_denorm_d

	cmpi.l		#0x7ff00000,d0
	beq.b		inf_or_nan_d

is_norm_d:
	move.b		#NORM, d0
	rts
zero_or_denorm_d:
	andi.l		#0x000fffff, d1
	bne.l		is_denorm_d
	tst.l		4+FTEMP(a0)
	bne.l		is_denorm_d
is_zero_d:
	move.b		#ZERO, d0
	rts
is_denorm_d:
	move.b		#DENORM, d0
	rts
inf_or_nan_d:
	andi.l		#0x000fffff, d1
	bne.l		is_nan_d
	tst.l		4+FTEMP(a0)
	bne.l		is_nan_d
is_inf_d:
	move.b		#INF, d0
	rts
is_nan_d:
	btst		#19, d1
	bne.l		is_qnan_d
is_snan_d:
	move.b		#SNAN, d0
	rts
is_qnan_d:
	move.b		#QNAN, d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	set_tag_s(): return the optype of the input sgl fp number	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	a0 = pointer to single precision operand			 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	d0 = value of type tag						 */
/* 		one of: NORM, INF, QNAN, SNAN, DENORM, ZERO		 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Simply test the exponent, j-bit, and mantissa values to		 */
/*  determine the type of operand.					 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		set_tag_s
set_tag_s:
	move.l		FTEMP.w(a0), d0
	move.l		d0, d1

	andi.l		#0x7f800000, d0
	beq.b		zero_or_denorm_s

	cmpi.l		#0x7f800000,d0
	beq.b		inf_or_nan_s

is_norm_s:
	move.b		#NORM, d0
	rts
zero_or_denorm_s:
	andi.l		#0x007fffff, d1
	bne.l		is_denorm_s
is_zero_s:
	move.b		#ZERO, d0
	rts
is_denorm_s:
	move.b		#DENORM, d0
	rts
inf_or_nan_s:
	andi.l		#0x007fffff, d1
	bne.l		is_nan_s
is_inf_s:
	move.b		#INF, d0
	rts
is_nan_s:
	btst		#22, d1
	bne.l		is_qnan_s
is_snan_s:
	move.b		#SNAN, d0
	rts
is_qnan_s:
	move.b		#QNAN, d0
	rts

