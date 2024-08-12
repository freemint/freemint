/*
 *  sscale(): computes the destination operand scaled by the source	
 * 	    operand. If the absoulute value of the source operand is	
 * 	    >= 2^14, an overflow or underflow is returned.		
 * 									
 *  INPUT *************************************************************** 
 * 	a0  = pointer to double-extended source operand X		
 * 	a1  = pointer to double-extended destination operand Y		
 * 									
 *  OUTPUT ************************************************************** 
 * 	fp0 =  scale(X,Y)						
 * 									
 */

	.include "hdr.fpu"

	.text

	SIGN = L_SCR1

	.globl		sscale
sscale:
	move.l		d0,-(sp)		/*  store off ctrl bits for now */

	move.w		DST_EX.w(a1),d1		/*  get dst exponent */
	smi.b		SIGN(a6)		/*  use SIGN to hold dst sign */
	andi.l		#0x00007fff,d1		/*  strip sign from dst exp */

	move.w		SRC_EX.w(a0),d0		/*  check src bounds */
	andi.w		#0x7fff,d0		/*  clr src sign bit */
	cmpi.w		#0x3fff,d0		/*  is src ~ ZERO? */
	blt.w		src_small		/*  yes */
	cmpi.w		#0x400c,d0		/*  no; is src too big? */
	bgt.w		src_out			/*  yes */

/*
 * Source is within 2^14 range.
 */
src_ok:
	fintrz.x	SRC.w(a0),fp0		/*  calc int of src */
	fmove.l		fp0,d0		/*  int src to d0 */
/*  don't want any accrued bits from the fintrz showing up later since */
/*  we may need to read the fpsr for the last fp op in t_catch2(). */
	fmove.l		#0x0,fpsr

	tst.b		DST_HI(a1)		/*  is dst denormalized? */
	bmi.b		sok_norm

/*  the dst is a DENORM. normalize the DENORM and add the adjustment to */
/*  the src value. then, jump to the norm part of the routine. */
sok_dnrm:
	move.l		d0,-(sp)		/*  save src for now */

	move.w		DST_EX.w(a1),FP_SCR0_EX(a6) /*  make a copy */
	move.l		DST_HI(a1),FP_SCR0_HI(a6)
	move.l		DST_LO(a1),FP_SCR0_LO(a6)

	lea		FP_SCR0(a6),a0	/*  pass ptr to DENORM */
	bsr.l		norm			/*  normalize the DENORM */
	neg.l		d0
	add.l		(sp)+,d0		/*  add adjustment to src */

	fmovem.x	FP_SCR0(a6),fp0	/*  load normalized DENORM */

	cmpi.w		#-0x3fff,d0		/*  is the shft amt really low? */
	bge.b		sok_norm2		/*  thank goodness no */

/*  the multiply factor that we're trying to create should be a denorm */
/*  for the multiply to work. Therefore, we're going to actually do a */
/*  multiply with a denorm which will cause an unimplemented data type */
/*  exception to be put into the machine which will be caught and corrected */
/*  later. we don't do this with the DENORMs above because this method */
/*  is slower. but, don't fret, I don't see it being used much either. */
	fmove.l		(sp)+,fpcr		/*  restore user fpcr */
	move.l		#0x80000000,d1		/*  load normalized mantissa */
	subi.l		#-0x3fff,d0		/*  how many should we shift? */
	neg.l		d0			/*  make it positive */
	cmpi.b		#0x20,d0		/*  is it > 32? */
	bge.b		sok_dnrm_32		/*  yes */
	lsr.l		d0,d1			/*  no; bit stays in upper lw */
	clr.l		-(sp)			/*  insert zero low mantissa */
	move.l		d1,-(sp)		/*  insert new high mantissa */
	clr.l		-(sp)			/*  make zero exponent */
	bra.b		sok_norm_cont
sok_dnrm_32:
	subi.b		#0x20,d0		/*  get shift count */
	lsr.l		d0,d1			/*  make low mantissa longword */
	move.l		d1,-(sp)		/*  insert new low mantissa */
	clr.l		-(sp)			/*  insert zero high mantissa */
	clr.l		-(sp)			/*  make zero exponent */
	bra.b		sok_norm_cont

/*  the src will force the dst to a DENORM value or worse. so, let's */
/*  create an fp multiply that will create the result. */
sok_norm:
	fmovem.x	DST.w(a1),fp0		/*  load fp0 with normalized src */
sok_norm2:
	fmove.l		(sp)+,fpcr		/*  restore user fpcr */

	addi.w		#0x3fff,d0		/*  turn src amt into exp value */
	swap		d0			/*  put exponent in high word */
	clr.l		-(sp)			/*  insert new exponent */
	move.l		#0x80000000,-(sp)	/*  insert new high mantissa */
	move.l		d0,-(sp)		/*  insert new lo mantissa */

sok_norm_cont:
	fmove.l		fpcr,d0		/*  d0 needs fpcr for t_catch2 */
	move.b		#FMUL_OP,d1		/*  last inst is MUL */
	fmul.x		(sp)+,fp0		/*  do the multiply */
	bra.l		t_catch2		/*  catch any exceptions */

/*
 * Source is outside of 2^14 range.  Test the sign and branch
 * to the appropriate exception handler.
 */
src_out:
	move.l		(sp)+,d0		/*  restore ctrl bits */
	exg		a0,a1			/*  swap src,dst ptrs */
	tst.b		SRC_EX.w(a1)		/*  is src negative? */
	bmi.l		t_unfl			/*  yes; underflow */
	bra.l		t_ovfl_sc		/*  no; overflow */

/*
 * The source input is below 1, so we check for denormalized numbers
 * and set unfl.
 */
src_small:
	tst.b		DST_HI(a1)		/*  is dst denormalized? */
	bpl.b		ssmall_done		/*  yes */

	move.l		(sp)+,d0
	fmove.l		d0,fpcr		/*  no; load control bits */
	move.b		#FMOV_OP,d1		/*  last inst is MOVE */
	fmove.x		DST.w(a1),fp0		/*  simply return dest */
	bra.l		t_catch2
ssmall_done:
	move.l		(sp)+,d0		/*  load control bits into d1 */
	move.l		a1,a0			/*  pass ptr to dst */
	bra.l		t_resdnrm

