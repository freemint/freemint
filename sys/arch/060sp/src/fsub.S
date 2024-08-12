/*
 *  XDEF ****************************************************************	
 * 	fsub(): emulates the fsub instruction				
 * 	fssub(): emulates the fssub instruction				
 * 	fdsub(): emulates the fdsub instruction				
 * 									
 *  XREF ****************************************************************	
 * 	addsub_scaler2() - scale the operands so they won't take exc	
 * 	ovf_res() - return default overflow result			
 * 	unf_res() - return default underflow result			
 * 	res_qnan() - set QNAN result					
 * 	res_snan() - set SNAN result					
 * 	res_operr() - set OPERR result					
 * 	scale_to_zero_src() - set src operand exponent equal to zero	
 * 	scale_to_zero_dst() - set dst operand exponent equal to zero	
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	a1 = pointer to extended precision destination operand		
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = result							
 * 	fp1 = EXOP (if exception occurred)				
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Handle NANs, infinities, and zeroes as special cases. Divide	
 *  norms into extended, single, and double precision.			
 * 	Do subtraction after scaling exponents such that exception won't
 *  occur. Then, check result exponent to see if exception would have	
 *  occurred. If so, return default result and maybe EXOP. Else, insert	
 *  the correct result exponent and return. Set FPSR bits as appropriate.	
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fssub
fssub:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#s_mode*0x10,d0	/*  insert sgl prec */
	bra.b		fsub

	.globl		fdsub
fdsub:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#d_mode*0x10,d0	/*  insert dbl prec */

	.globl		fsub
fsub:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */

	clr.w		d1
	move.b		DTAG(a6),d1
	lsl.b		#0x3,d1
	or.b		STAG(a6),d1		/*  combine src tags */

	bne.w		fsub_not_norm		/*  optimize on non-norm input */

/*
 * SUB: norms and denorms
 */
fsub_norm:
	bsr.l		addsub_scaler2		/*  scale exponents */

fsub_zero_entry:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		#0x0,fpsr		/*  clear FPSR */
	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fsub.x		FP_SCR0(a6),fp0	/*  execute subtract */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  fetch INEX2, N, Z */

	or.l		d1,USER_FPSR(a6)	/*  save exc and ccode bits */

	fbeq		fsub_zero_exit		/*  if result zero, end now */

	move.l		d2,-(sp)		/*  save d2 */

	fmovem.x	fp0,-(sp)		/*  save result to stack */

	move.w		2+L_SCR3(a6),d1
	lsr.b		#0x6,d1

	move.w		(sp),d2		/*  fetch new exponent */
	andi.l		#0x7fff,d2		/*  strip sign */
	sub.l		d0,d2			/*  add scale factor */

	cmp.l		(tbl_fsub_ovfl.b,pc,d1.w*4),d2 /*  is it an overflow? */
	bge.b		fsub_ovfl		/*  yes */

	cmp.l		(tbl_fsub_unfl.b,pc,d1.w*4),d2 /*  is it an underflow? */
	blt.w		fsub_unfl		/*  yes */
	beq.w		fsub_may_unfl		/*  maybe; go find out */

fsub_normal:
	move.w		(sp),d1
	andi.w		#0x8000,d1		/*  keep sign */
	or.w		d2,d1			/*  insert new exponent */
	move.w		d1,(sp)		/*  insert new exponent */

	fmovem.x	(sp)+,fp0		/*  return result in fp0 */

	move.l		(sp)+,d2		/*  restore d2 */
	rts

fsub_zero_exit:
/* 	fmove.s		#0x00000000,fp0	; return zero in fp0 */
	rts

tbl_fsub_ovfl:
	.dc.l		0x7fff			/*  ext ovfl */
	.dc.l		0x407f			/*  sgl ovfl */
	.dc.l		0x43ff			/*  dbl ovfl */

tbl_fsub_unfl:
	.dc.l	    0x0000			/*  ext unfl */
	.dc.l		0x3f81			/*  sgl unfl */
	.dc.l		0x3c01			/*  dbl unfl */

fsub_ovfl:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fsub_ovfl_ena		/*  yes */

	add.l		#0xc,sp
fsub_ovfl_dis:
	btst		#neg_bit,FPSR_CC(a6)	/*  is result negative? */
	sne			d1			/*  set sign param accordingly */
	move.l		L_SCR3(a6),d0		/*  pass prec:rnd */
	bsr.l		ovf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set INF,N if applicable */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */
	move.l		(sp)+,d2		/*  restore d2 */
	rts

fsub_ovfl_ena:
	move.b		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  is precision extended? */
	bne.b		fsub_ovfl_ena_sd	/*  no */

fsub_ovfl_ena_cont:
	move.w		(sp),d1		/*  fetch {sgn,exp} */
	andi.w		#0x8000,d1		/*  keep sign */
	subi.l		#0x6000,d2		/*  subtract new bias */
	andi.w		#0x7fff,d2		/*  clear top bit */
	or.w		d2,d1			/*  concat sign,exp */
	move.w		d1,(sp)		/*  insert new exponent */

	fmovem.x	(sp)+,fp1		/*  return EXOP in fp1 */
	bra.b		fsub_ovfl_dis

fsub_ovfl_ena_sd:
	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	move.l		L_SCR3(a6),d1
	andi.b		#0x30,d1		/*  clear rnd prec */
	fmove.l		d1,fpcr		/*  set FPCR */

	fsub.x		FP_SCR0(a6),fp0	/*  execute subtract */

	fmove.l		#0x0,fpcr		/*  clear FPCR */

	add.l		#0xc,sp
	fmovem.x	fp0,-(sp)
	bra.b		fsub_ovfl_ena_cont

fsub_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	add.l		#0xc,sp

	fmovem.x	FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		#rz_mode*0x10,fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsub.x		FP_SCR0(a6),fp0	/*  execute subtract */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  save status */

	or.l		d1,USER_FPSR(a6)

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fsub_unfl_ena		/*  yes */

fsub_unfl_dis:
	fmovem.x	fp0,FP_SCR0(a6)	/*  store out result */

	lea			FP_SCR0(a6),a0	/*  pass: result addr */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  'Z' may have been set */
	fmovem.x	FP_SCR0(a6),fp0	/*  return default result in fp0 */
	move.l		(sp)+,d2		/*  restore d2 */
	rts

fsub_unfl_ena:
	fmovem.x	FP_SCR1(a6),fp1

	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  is precision extended? */
	bne.b		fsub_unfl_ena_sd	/*  no */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

fsub_unfl_ena_cont:
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsub.x		FP_SCR0(a6),fp1	/*  execute subtract */

	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fmovem.x	fp1,FP_SCR0(a6)	/*  store result to stack */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	addi.l		#0x6000,d1		/*  subtract new bias */
	andi.w		#0x7fff,d1		/*  clear top bit */
	or.w		d2,d1			/*  concat sgn,exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.w		fsub_unfl_dis

fsub_unfl_ena_sd:
	move.l		L_SCR3(a6),d1
	andi.b		#0x30,d1		/*  clear rnd prec */
	fmove.l		d1,fpcr		/*  set FPCR */

	bra.b		fsub_unfl_ena_cont

/*
 * result is equal to the smallest normalized number in the selected precision
 * if the precision is extended, this result could not have come from an
 * underflow that rounded up.
 */
fsub_may_unfl:
	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  fetch rnd prec */
	beq.w		fsub_normal		/*  yes; no underflow occurred */

	move.l		0x4(sp),d1
	cmpi.l		#0x80000000,d1		/*  is hi(man) = 0x80000000? */
	bne.w		fsub_normal		/*  no; no underflow occurred */

	tst.l		0x8(sp)		/*  is lo(man) = 0x0? */
	bne.w		fsub_normal		/*  no; no underflow occurred */

	btst		#inex2_bit,FPSR_EXCEPT(a6) /*  is INEX2 set? */
	beq.w		fsub_normal		/*  no; no underflow occurred */

/*
 * ok, so now the result has a exponent equal to the smallest normalized
 * exponent for the selected precision. also, the mantissa is equal to
 * 0x8000000000000000 and this mantissa is the result of rounding non-zero
 * g,r,s.
 * now, we must determine whether the pre-rounded result was an underflow
 * rounded "up" or a normalized number rounded "down".
 * so, we do this be re-executing the add using RZ as the rounding mode and
 * seeing if the new result is smaller or equal to the current result.
 */
	fmovem.x	FP_SCR1(a6),fp1	/*  load dst op into fp1 */

	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  keep rnd prec */
	ori.b		#rz_mode*0x10,d1	/*  insert rnd mode */
	fmove.l		d1,fpcr		/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fsub.x		FP_SCR0(a6),fp1	/*  execute subtract */

	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fabs.x		fp0			/*  compare absolute values */
	fabs.x		fp1
	fcmp.x		fp1,fp0		/*  is first result > second? */

	fbgt		fsub_unfl		/*  yes; it's an underflow */
	bra.w		fsub_normal		/*  no; it's not an underflow */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * Sub: inputs are not both normalized; what are they?
 */
fsub_not_norm:
	move.w		(tbl_fsub_op.b,pc,d1.w*2),d1
	jmp		(tbl_fsub_op.b,pc,d1.w*1)

	/* swbeg		#48 */
	.dc.w 0x4afc,48
tbl_fsub_op:
	.dc.w		fsub_norm-tbl_fsub_op /*  NORM - NORM */
	.dc.w		fsub_zero_src-tbl_fsub_op /*  NORM - ZERO */
	.dc.w		fsub_inf_src-tbl_fsub_op /*  NORM - INF */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  NORM - QNAN */
	.dc.w		fsub_norm-tbl_fsub_op /*  NORM - DENORM */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  NORM - SNAN */
	.dc.w		tbl_fsub_op-tbl_fsub_op
	.dc.w		tbl_fsub_op-tbl_fsub_op

	.dc.w		fsub_zero_dst-tbl_fsub_op /*  ZERO - NORM */
	.dc.w		fsub_zero_2-tbl_fsub_op /*  ZERO - ZERO */
	.dc.w		fsub_inf_src-tbl_fsub_op /*  ZERO - INF */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  NORM - QNAN */
	.dc.w		fsub_zero_dst-tbl_fsub_op /*  ZERO - DENORM */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  NORM - SNAN */
	.dc.w		tbl_fsub_op-tbl_fsub_op
	.dc.w		tbl_fsub_op-tbl_fsub_op

	.dc.w		fsub_inf_dst-tbl_fsub_op /*  INF - NORM */
	.dc.w		fsub_inf_dst-tbl_fsub_op /*  INF - ZERO */
	.dc.w		fsub_inf_2-tbl_fsub_op /*  INF - INF */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  NORM - QNAN */
	.dc.w		fsub_inf_dst-tbl_fsub_op /*  INF - DENORM */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  NORM - SNAN */
	.dc.w		tbl_fsub_op-tbl_fsub_op
	.dc.w		tbl_fsub_op-tbl_fsub_op

	.dc.w		fsub_res_qnan-tbl_fsub_op /*  QNAN - NORM */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  QNAN - ZERO */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  QNAN - INF */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  QNAN - QNAN */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  QNAN - DENORM */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  QNAN - SNAN */
	.dc.w		tbl_fsub_op-tbl_fsub_op
	.dc.w		tbl_fsub_op-tbl_fsub_op

	.dc.w		fsub_norm-tbl_fsub_op /*  DENORM - NORM */
	.dc.w		fsub_zero_src-tbl_fsub_op /*  DENORM - ZERO */
	.dc.w		fsub_inf_src-tbl_fsub_op /*  DENORM - INF */
	.dc.w		fsub_res_qnan-tbl_fsub_op /*  NORM - QNAN */
	.dc.w		fsub_norm-tbl_fsub_op /*  DENORM - DENORM */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  NORM - SNAN */
	.dc.w		tbl_fsub_op-tbl_fsub_op
	.dc.w		tbl_fsub_op-tbl_fsub_op

	.dc.w		fsub_res_snan-tbl_fsub_op /*  SNAN - NORM */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  SNAN - ZERO */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  SNAN - INF */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  SNAN - QNAN */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  SNAN - DENORM */
	.dc.w		fsub_res_snan-tbl_fsub_op /*  SNAN - SNAN */
	.dc.w		tbl_fsub_op-tbl_fsub_op
	.dc.w		tbl_fsub_op-tbl_fsub_op

fsub_res_qnan:
	bra.l		res_qnan
fsub_res_snan:
	bra.l		res_snan

/*
 * both operands are ZEROes
 */
fsub_zero_2:
	move.b		SRC_EX.w(a0),d0
	move.b		DST_EX.w(a1),d1
	eor.b		d1,d0
	bpl.b		fsub_zero_2_chk_rm

/*  the signs are opposite, so, return a ZERO w/ the sign of the dst ZERO */
	tst.b		d0			/*  is dst negative? */
	bmi.b		fsub_zero_2_rm		/*  yes */
	fmove.s		#0x00000000,fp0	/*  no; return +ZERO */
	move.b		#z_bmask,FPSR_CC(a6)	/*  set Z */
	rts

/*
 * the ZEROes have the same signs:
 * - Therefore, we return +ZERO if the rounding mode is RN,RZ, or RP
 * - -ZERO is returned in the case of RM.
 */
fsub_zero_2_chk_rm:
	move.b		3+L_SCR3(a6),d1
	andi.b		#0x30,d1		/*  extract rnd mode */
	cmpi.b		#rm_mode*0x10,d1	/*  is rnd mode = RM? */
	beq.b		fsub_zero_2_rm		/*  yes */
	fmove.s		#0x00000000,fp0	/*  no; return +ZERO */
	move.b		#z_bmask,FPSR_CC(a6)	/*  set Z */
	rts

fsub_zero_2_rm:
	fmove.s		#0x80000000,fp0	/*  return -ZERO */
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6)	/*  set Z/NEG */
	rts

/*
 * one operand is a ZERO and the other is a DENORM or a NORM.
 * scale the DENORM or NORM and jump to the regular fsub routine.
 */
fsub_zero_dst:
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	bsr.l		scale_to_zero_src	/*  scale the operand */
	clr.w		FP_SCR1_EX(a6)
	clr.l		FP_SCR1_HI(a6)
	clr.l		FP_SCR1_LO(a6)
	bra.w		fsub_zero_entry		/*  go execute fsub */

fsub_zero_src:
	move.w		DST_EX.w(a1),FP_SCR1_EX(a6)
	move.l		DST_HI(a1),FP_SCR1_HI(a6)
	move.l		DST_LO(a1),FP_SCR1_LO(a6)
	bsr.l		scale_to_zero_dst	/*  scale the operand */
	clr.w		FP_SCR0_EX(a6)
	clr.l		FP_SCR0_HI(a6)
	clr.l		FP_SCR0_LO(a6)
	bra.w		fsub_zero_entry		/*  go execute fsub */

/*
 * both operands are INFs. an OPERR will result if the INFs have the
 * same signs. else,
 */
fsub_inf_2:
	move.b		SRC_EX.w(a0),d0		/*  exclusive or the signs */
	move.b		DST_EX.w(a1),d1
	eor.b		d1,d0
	bpl.l		res_operr		/*  weed out (-INF)+(+INF) */

/*  ok, so it's not an OPERR. but we do have to remember to return */
/*  the src INF since that's where the 881/882 gets the j-bit. */

fsub_inf_src:
	fmovem.x	SRC.w(a0),fp0		/*  return src INF */
	fneg.x		fp0			/*  invert sign */
	fbge		fsub_inf_done		/*  sign is now positive */
	move.b		#neg_bmask+inf_bmask,FPSR_CC(a6) /*  set INF/NEG */
	rts

fsub_inf_dst:
	fmovem.x		DST.w(a1),fp0		/*  return dst INF */
	tst.b		DST_EX.w(a1)		/*  is INF negative? */
	bpl.b		fsub_inf_done		/*  no */
	move.b		#neg_bmask+inf_bmask,FPSR_CC(a6) /*  set INF/NEG */
	rts

fsub_inf_done:
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set INF */
	rts
