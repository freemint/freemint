/*
 *  XDEF ****************************************************************	
 * 	_denorm(): denormalize an intermediate result			
 * 									
 *  XREF ****************************************************************	
 * 	None								
 * 									
 *  INPUT *************************************************************** 
 * 	a0 = points to the operand to be denormalized			
 * 		(in the internal extended format)			
 * 									
 * 	d0 = rounding precision						
 * 									
 *  OUTPUT **************************************************************	
 * 	a0 = pointer to the denormalized result				
 * 		(in the internal extended format)			
 * 									
 * 	d0 = guard,round,sticky						
 * 									
 *  ALGORITHM ***********************************************************	
 * 	According to the exponent underflow threshold for the given	
 *  precision, shift the mantissa bits to the right in order raise the	
 *  exponent of the operand to the threshold value. While shifting the	
 *  mantissa bits right, maintain the value of the guard, round, and	
 *  sticky bits.								
 *  other notes:								
 * 	(1) _denorm() is called by the underflow routines		
 * 	(2) _denorm() does NOT affect the status register		
 * 									
 */

	.include "hdr.fpu"

	.text

/*
 * table of exponent threshold values for each precision
 */
tbl_thresh:
	.dc.w		0x0
	.dc.w		sgl_thresh
	.dc.w		dbl_thresh

	.globl		_denorm
_denorm:
/*
 * Load the exponent threshold for the precision selected and check
 * to see if (threshold - exponent) is > 65 in which case we can
 * simply calculate the sticky bit and zero the mantissa. otherwise
 * we have to call the denormalization routine.
 */
	lsr.b		#0x2,d0		/*  shift prec to lo bits */
	move.w		(tbl_thresh.b,pc,d0.w*2), d1 /*  load prec threshold */
	move.w		d1, d0		/*  copy d1 into d0 */
	sub.w		FTEMP_EX.w(a0), d0	/*  diff = threshold - exp */
	cmpi.w		#66,d0		/*  is diff > 65? (mant + g,r bits) */
	bpl.b		denorm_set_stky		/*  yes; just calc sticky */

	clr.l		d0			/*  clear g,r,s */
	btst		#inex2_bit, FPSR_EXCEPT(a6) /*  yes; was INEX2 set? */
	beq.b		denorm_call		/*  no; don't change anything */
	bset		#29, d0		/*  yes; set sticky bit */

denorm_call:
	bsr.l		dnrm_lp			/*  denormalize the number */
	rts

/*
 * all bit would have been shifted off during the denorm so simply
 * calculate if the sticky should be set and clear the entire mantissa.
 */
denorm_set_stky:
	move.l		#0x20000000, d0	/*  set sticky bit in return value */
	move.w		d1, FTEMP_EX.w(a0)	/*  load exp with threshold */
	clr.l		FTEMP_HI(a0)		/*  set d1 = 0 (ms mantissa) */
	clr.l		FTEMP_LO(a0)		/*  set d2 = 0 (ms mantissa) */
	rts

/* 									 */
/*  dnrm_lp(): normalize exponent/mantissa to specified threshold		 */
/* 									 */
/*  INPUT:								 */
/* 	a0	   : points to the operand to be denormalized		 */
/* 	d0{31:29} : initial guard,round,sticky				 */
/* 	d1{15:0}  : denormalization threshold				 */
/*  OUTPUT:								 */
/* 	a0	   : points to the denormalized operand			 */
/* 	d0{31:29} : final guard,round,sticky				 */
/* 									 */

/*  *** Local Equates ***  */
	GRS = L_SCR2			/*  g,r,s temp storage */
	FTEMP_LO2 = L_SCR1			/*  FTEMP_LO copy */

	.globl		dnrm_lp
dnrm_lp:

/*
 * make a copy of FTEMP_LO and place the g,r,s bits directly after it
 * in memory so as to make the bitfield extraction for denormalization easier.
 */
	move.l		FTEMP_LO(a0), FTEMP_LO2(a6) /*  make FTEMP_LO copy */
	move.l		d0, GRS(a6)		/*  place g,r,s after it */

/*
 * check to see how much less than the underflow threshold the operand
 * exponent is.
 */
	move.l		d1, d0		/*  copy the denorm threshold */
	sub.w		FTEMP_EX.w(a0), d1	/*  d1 = threshold - uns exponent */
	ble.b		dnrm_no_lp		/*  d1 <= 0 */
	cmpi.w		#0x20,d1		/*  is ( 0 <= d1 < 32) ? */
	blt.b		case_1			/*  yes */
	cmpi.w		#0x40,d1		/*  is (32 <= d1 < 64) ? */
	blt.b		case_2			/*  yes */
	bra.w		case_3			/*  (d1 >= 64) */

/*
 * No normalization necessary
 */
dnrm_no_lp:
	move.l		GRS(a6), d0		/*  restore original g,r,s */
	rts

/*
 * case (0<d1<32)
 *
 * d0 = denorm threshold
 * d1 = "n" = amt to shift
 *
 *      ---------------------------------------------------------
 *      |     FTEMP_HI    |     FTEMP_LO     |grs000.........000|
 *      ---------------------------------------------------------
 *      <-(32 - n)-><-(n)-><-(32 - n)-><-(n)-><-(32 - n)-><-(n)->
 *      \          \                  \                  \
 *       \          \                  \                  \
 *        \          \                  \                  \
 *         \          \                  \                  \
 *          \          \                  \                  \
 *           \          \                  \                  \
 *            \          \                  \                  \
 *             \          \                  \                  \
 *      <-(n)-><-(32 - n)-><------(32)-------><------(32)------->
 *      ---------------------------------------------------------
 *      |0.....0| NEW_HI  |  NEW_FTEMP_LO     |grs              |
 *      ---------------------------------------------------------
 */
case_1:
	move.l		d2, -(sp)		/*  create temp storage */

	move.w		d0, FTEMP_EX.w(a0)	/*  exponent = denorm threshold */
	moveq.l		#32, d0
	sub.w		d1, d0		/*  d0 = 32 - d1 */

	cmpi.w		#29,d1		/*  is shft amt >= 29 */
	blt.b		case1_extract		/*  no; no fix needed */
	move.b		GRS(a6), d2
	or.b		d2, 3+FTEMP_LO2(a6)

case1_extract:
	bfextu		FTEMP_HI(a0){0:d0}, d2 /*  d2 = new FTEMP_HI */
	bfextu		FTEMP_HI(a0){d0:32}, d1 /*  d1 = new FTEMP_LO */
	bfextu		FTEMP_LO2(a6){d0:32}, d0 /*  d0 = new G,R,S */

	move.l		d2, FTEMP_HI(a0)	/*  store new FTEMP_HI */
	move.l		d1, FTEMP_LO(a0)	/*  store new FTEMP_LO */

	bftst		d0{2:30}		/*  were bits shifted off? */
	beq.b		case1_sticky_clear	/*  no; go finish */
	bset		#rnd_stky_bit, d0	/*  yes; set sticky bit */

case1_sticky_clear:
	andi.l		#0xe0000000,d0	/*  clear all but G,R,S */
	move.l		(sp)+, d2		/*  restore temp register */
	rts

/*
 * case (32<=d1<64)
 *
 * d0 = denorm threshold
 * d1 = "n" = amt to shift
 *
 *      ---------------------------------------------------------
 *      |     FTEMP_HI    |     FTEMP_LO     |grs000.........000|
 *      ---------------------------------------------------------
 *      <-(32 - n)-><-(n)-><-(32 - n)-><-(n)-><-(32 - n)-><-(n)->
 *      \          \                  \
 *       \          \                  \
 *        \          \                  -------------------
 *         \          --------------------                 \
 *          -------------------           \                 \
 *                             \           \                 \
 *                              \           \                 \
 *                               \           \                 \
 *      <-------(32)------><-(n)-><-(32 - n)-><------(32)------->
 *      ---------------------------------------------------------
 *      |0...............0|0....0| NEW_LO     |grs              |
 *      ---------------------------------------------------------
 */
case_2:
	move.l		d2, -(sp)		/*  create temp storage */

	move.w		d0, FTEMP_EX.w(a0)	/*  exponent = denorm threshold */
	subi.w		#0x20, d1		/*  d1 now between 0 and 32 */
	moveq.l		#0x20, d0
	sub.w		d1, d0		/*  d0 = 32 - d1 */

/*  subtle step here; or in the g,r,s at the bottom of FTEMP_LO to minimize */
/*  the number of bits to check for the sticky detect. */
/*  it only plays a role in shift amounts of 61-63. */
	move.b		GRS(a6), d2
	or.b		d2, 3+FTEMP_LO2(a6)

	bfextu		FTEMP_HI(a0){0:d0}, d2 /*  d2 = new FTEMP_LO */
	bfextu		FTEMP_HI(a0){d0:32}, d1 /*  d1 = new G,R,S */

	bftst		d1{2:30}		/*  were any bits shifted off? */
	bne.b		case2_set_sticky	/*  yes; set sticky bit */
	bftst		FTEMP_LO2(a6){d0:31}	/*  were any bits shifted off? */
	bne.b		case2_set_sticky	/*  yes; set sticky bit */

	move.l		d1, d0		/*  move new G,R,S to d0 */
	bra.b		case2_end

case2_set_sticky:
	move.l		d1, d0		/*  move new G,R,S to d0 */
	bset		#rnd_stky_bit, d0	/*  set sticky bit */

case2_end:
	clr.l		FTEMP_HI(a0)		/*  store FTEMP_HI = 0 */
	move.l		d2, FTEMP_LO(a0)	/*  store FTEMP_LO */
	andi.l		#0xe0000000, d0	/*  clear all but G,R,S */

	move.l		(sp)+,d2		/*  restore temp register */
	rts

/*
 * case (d1>=64)
 *
 * d0 = denorm threshold
 * d1 = amt to shift
 */
case_3:
	move.w		d0, FTEMP_EX.w(a0)	/*  insert denorm threshold */

	cmpi.w		#65,d1		/*  is shift amt > 65? */
	blt.b		case3_64		/*  no; it's == 64 */
	beq.b		case3_65		/*  no; it's == 65 */

/*
 * case (d1>65)
 *
 * Shift value is > 65 and out of range. All bits are shifted off.
 * Return a zero mantissa with the sticky bit set
 */
	clr.l		FTEMP_HI(a0)		/*  clear hi(mantissa) */
	clr.l		FTEMP_LO(a0)		/*  clear lo(mantissa) */
	move.l		#0x20000000, d0	/*  set sticky bit */
	rts

/*
 * case (d1 == 64)
 *
 *      ---------------------------------------------------------
 *      |     FTEMP_HI    |     FTEMP_LO     |grs000.........000|
 *      ---------------------------------------------------------
 *      <-------(32)------>
 *      \                  \
 *       \                  \
 *        \                  \
 *         \                  ------------------------------
 *          -------------------------------                 \
 *                                         \                 \
 *                                          \                 \
 *                                           \                 \
 *                                            <-------(32)------>
 *      ---------------------------------------------------------
 *      |0...............0|0................0|grs               |
 *      ---------------------------------------------------------
 */
case3_64:
	move.l		FTEMP_HI(a0), d0	/*  fetch hi(mantissa) */
	move.l		d0, d1		/*  make a copy */
	andi.l		#0xc0000000, d0	/*  extract G,R */
	andi.l		#0x3fffffff, d1	/*  extract other bits */

	bra.b		case3_complete

/*
 * case (d1 == 65)
 *
 *      ---------------------------------------------------------
 *      |     FTEMP_HI    |     FTEMP_LO     |grs000.........000|
 *      ---------------------------------------------------------
 *      <-------(32)------>
 *      \                  \
 *       \                  \
 *        \                  \
 *         \                  ------------------------------
 *          --------------------------------                \
 *                                          \                \
 *                                           \                \
 *                                            \                \
 *                                             <-------(31)----->
 *      ---------------------------------------------------------
 *      |0...............0|0................0|0rs               |
 *      ---------------------------------------------------------
 */
case3_65:
	move.l		FTEMP_HI(a0), d0	/*  fetch hi(mantissa) */
	andi.l		#0x80000000, d0	/*  extract R bit */
	lsr.l		#0x1, d0		/*  shift high bit into R bit */
	andi.l		#0x7fffffff, d1	/*  extract other bits */

case3_complete:
/*  last operation done was an "and" of the bits shifted off so the condition */
/*  codes are already set so branch accordingly. */
	bne.b		case3_set_sticky	/*  yes; go set new sticky */
	tst.l		FTEMP_LO(a0)		/*  were any bits shifted off? */
	bne.b		case3_set_sticky	/*  yes; go set new sticky */
	tst.b		GRS(a6)		/*  were any bits shifted off? */
	bne.b		case3_set_sticky	/*  yes; go set new sticky */

/*
 * no bits were shifted off so don't set the sticky bit.
 * the guard and
 * the entire mantissa is zero.
 */
	clr.l		FTEMP_HI(a0)		/*  clear hi(mantissa) */
	clr.l		FTEMP_LO(a0)		/*  clear lo(mantissa) */
	rts

/*
 * some bits were shifted off so set the sticky bit.
 * the entire mantissa is zero.
 */
case3_set_sticky:
	bset		#rnd_stky_bit,d0	/*  set new sticky bit */
	clr.l		FTEMP_HI(a0)		/*  clear hi(mantissa) */
	clr.l		FTEMP_LO(a0)		/*  clear lo(mantissa) */
	rts

