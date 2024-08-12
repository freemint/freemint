/*
 *  XDEF ****************************************************************	
 * 	fcmp(): fp compare op routine					
 * 									
 *  XREF ****************************************************************	
 * 	res_qnan() - return QNAN result					
 * 	res_snan() - return SNAN result					
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	a1 = pointer to extended precision destination operand		
 * 	d0 = round prec/mode						
 * 									
 *  OUTPUT ************************************************************** 
 * 	None								
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Handle NANs and denorms as special cases. For everything else,	
 *  just use the actual fcmp instruction to produce the correct condition	
 *  codes.								
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fcmp
fcmp:
	clr.w		d1
	move.b		DTAG(a6),d1
	lsl.b		#0x3,d1
	or.b		STAG(a6),d1
	bne.b		fcmp_not_norm		/*  optimize on non-norm input */

/*
 * COMPARE FP OPs : NORMs, ZEROs, INFs, and "corrected" DENORMs
 */
fcmp_norm:
	fmovem.x		DST.w(a1),fp0		/*  load dst op */

	fcmp.x		SRC.w(a0),fp0		/*  do compare */

	fmove.l		fpsr,d0		/*  save FPSR */
	rol.l		#0x8,d0		/*  extract ccode bits */
	move.b		d0,FPSR_CC(a6)	/*  set ccode bits(no exc bits are set) */

	rts

/*
 * fcmp: inputs are not both normalized; what are they?
 */
fcmp_not_norm:
	move.w		(tbl_fcmp_op.b,pc,d1.w*2),d1
	jmp		(tbl_fcmp_op.b,pc,d1.w*1)

	/* swbeg		#48 */
	.dc.w 0x4afc,48
tbl_fcmp_op:
	.dc.w		fcmp_norm-tbl_fcmp_op /*  NORM - NORM */
	.dc.w		fcmp_norm-tbl_fcmp_op /*  NORM - ZERO */
	.dc.w		fcmp_norm-tbl_fcmp_op /*  NORM - INF */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  NORM - QNAN */
	.dc.w		fcmp_nrm_dnrm-tbl_fcmp_op /*  NORM - DENORM */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  NORM - SNAN */
	.dc.w		tbl_fcmp_op-tbl_fcmp_op
	.dc.w		tbl_fcmp_op-tbl_fcmp_op

	.dc.w		fcmp_norm-tbl_fcmp_op /*  ZERO - NORM */
	.dc.w		fcmp_norm-tbl_fcmp_op /*  ZERO - ZERO */
	.dc.w		fcmp_norm-tbl_fcmp_op /*  ZERO - INF */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  ZERO - QNAN */
	.dc.w		fcmp_dnrm_s-tbl_fcmp_op /*  ZERO - DENORM */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  ZERO - SNAN */
	.dc.w		tbl_fcmp_op-tbl_fcmp_op
	.dc.w		tbl_fcmp_op-tbl_fcmp_op

	.dc.w		fcmp_norm-tbl_fcmp_op /*  INF - NORM */
	.dc.w		fcmp_norm-tbl_fcmp_op /*  INF - ZERO */
	.dc.w		fcmp_norm-tbl_fcmp_op /*  INF - INF */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  INF - QNAN */
	.dc.w		fcmp_dnrm_s-tbl_fcmp_op /*  INF - DENORM */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  INF - SNAN */
	.dc.w		tbl_fcmp_op-tbl_fcmp_op
	.dc.w		tbl_fcmp_op-tbl_fcmp_op

	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  QNAN - NORM */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  QNAN - ZERO */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  QNAN - INF */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  QNAN - QNAN */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  QNAN - DENORM */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  QNAN - SNAN */
	.dc.w		tbl_fcmp_op-tbl_fcmp_op
	.dc.w		tbl_fcmp_op-tbl_fcmp_op

	.dc.w		fcmp_dnrm_nrm-tbl_fcmp_op /*  DENORM - NORM */
	.dc.w		fcmp_dnrm_d-tbl_fcmp_op /*  DENORM - ZERO */
	.dc.w		fcmp_dnrm_d-tbl_fcmp_op /*  DENORM - INF */
	.dc.w		fcmp_res_qnan-tbl_fcmp_op /*  DENORM - QNAN */
	.dc.w		fcmp_dnrm_sd-tbl_fcmp_op /*  DENORM - DENORM */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  DENORM - SNAN */
	.dc.w		tbl_fcmp_op-tbl_fcmp_op
	.dc.w		tbl_fcmp_op-tbl_fcmp_op

	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  SNAN - NORM */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  SNAN - ZERO */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  SNAN - INF */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  SNAN - QNAN */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  SNAN - DENORM */
	.dc.w		fcmp_res_snan-tbl_fcmp_op /*  SNAN - SNAN */
	.dc.w		tbl_fcmp_op-tbl_fcmp_op
	.dc.w		tbl_fcmp_op-tbl_fcmp_op

/*  unlike all other functions for QNAN and SNAN, fcmp does NOT set the */
/*  'N' bit for a negative QNAN or SNAN input so we must squelch it here. */
fcmp_res_qnan:
	bsr.l		res_qnan
	andi.b		#0xf7,FPSR_CC(a6)
	rts
fcmp_res_snan:
	bsr.l		res_snan
	andi.b		#0xf7,FPSR_CC(a6)
	rts

/*
 * DENORMs are a little more difficult.
 * If you have a 2 DENORMs, then you can just force the j-bit to a one
 * and use the fcmp_norm routine.
 * If you have a DENORM and an INF or ZERO, just force the DENORM's j-bit to a one
 * and use the fcmp_norm routine.
 * If you have a DENORM and a NORM with opposite signs, then use fcmp_norm, also.
 * But with a DENORM and a NORM of the same sign, the neg bit is set if the
 * (1) signs are (+) and the DENORM is the dst or
 * (2) signs are (-) and the DENORM is the src
 */

fcmp_dnrm_s:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),d0
	bset		#31,d0			/*  DENORM src; make into small norm */
	move.l		d0,FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	lea		FP_SCR0(a6),a0
	bra.w		fcmp_norm

fcmp_dnrm_d:
	move.l		DST_EX.w(a1),FP_SCR0_EX(a6)
	move.l		DST_HI(a1),d0
	bset		#31,d0			/*  DENORM src; make into small norm */
	move.l		d0,FP_SCR0_HI(a6)
	move.l		DST_LO(a1),FP_SCR0_LO(a6)
	lea		FP_SCR0(a6),a1
	bra.w		fcmp_norm

fcmp_dnrm_sd:
	move.w		DST_EX.w(a1),FP_SCR1_EX(a6)
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		DST_HI(a1),d0
	bset		#31,d0			/*  DENORM dst; make into small norm */
	move.l		d0,FP_SCR1_HI(a6)
	move.l		SRC_HI(a0),d0
	bset		#31,d0			/*  DENORM dst; make into small norm */
	move.l		d0,FP_SCR0_HI(a6)
	move.l		DST_LO(a1),FP_SCR1_LO(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	lea		FP_SCR1(a6),a1
	lea		FP_SCR0(a6),a0
	bra.w		fcmp_norm

fcmp_nrm_dnrm:
	move.b		SRC_EX.w(a0),d0		/*  determine if like signs */
	move.b		DST_EX.w(a1),d1
	eor.b		d0,d1
	bmi.w		fcmp_dnrm_s

/*  signs are the same, so must determine the answer ourselves. */
	tst.b		d0			/*  is src op negative? */
	bmi.b		fcmp_nrm_dnrm_m		/*  yes */
	rts
fcmp_nrm_dnrm_m:
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'Z' ccode bit */
	rts

fcmp_dnrm_nrm:
	move.b		SRC_EX.w(a0),d0		/*  determine if like signs */
	move.b		DST_EX.w(a1),d1
	eor.b		d0,d1
	bmi.w		fcmp_dnrm_d

/*  signs are the same, so must determine the answer ourselves. */
	tst.b		d0			/*  is src op negative? */
	bpl.b		fcmp_dnrm_nrm_m		/*  no */
	rts
fcmp_dnrm_nrm_m:
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'Z' ccode bit */
	rts

