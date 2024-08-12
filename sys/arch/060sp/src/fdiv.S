/*
 *  XDEF ****************************************************************	
 * 	fdiv(): emulates the fdiv instruction				
 * 	fsdiv(): emulates the fsdiv instruction				
 * 	fddiv(): emulates the fddiv instruction				
 * 									
 *  XREF ****************************************************************	
 * 	scale_to_zero_src() - scale src exponent to zero		
 * 	scale_to_zero_dst() - scale dst exponent to zero		
 * 	unf_res() - return default underflow result			
 * 	ovf_res() - return default overflow result			
 * 	res_qnan() - return QNAN result					
 * 	res_snan() - return SNAN result					
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	a1 = pointer to extended precision destination operand		
 * 	d0  rnd prec,mode						
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = result							
 * 	fp1 = EXOP (if exception occurred)				
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Handle NANs, infinities, and zeroes as special cases. Divide	
 *  norms/denorms into ext/sgl/dbl precision.				
 * 	For norms/denorms, scale the exponents such that a divide	
 *  instruction won't cause an exception. Use the regular fdiv to		
 *  compute a result. Check if the regular operands would have taken	
 *  an exception. If so, return the default overflow/underflow result	
 *  and return the EXOP if exceptions are enabled. Else, scale the	
 *  result operand to the proper exponent.				
 * 									
 */

	.include "hdr.fpu"

	.text

	/* .balignw		16,0x51fc */
	.dc.w 0x51fc,0x51fc,0x51fc,0x51fc
tbl_fdiv_unfl:
	.dc.l		0x3fff - 0x0000		/*  ext_unfl */
	.dc.l		0x3fff - 0x3f81		/*  sgl_unfl */
	.dc.l		0x3fff - 0x3c01		/*  dbl_unfl */

tbl_fdiv_ovfl:
	.dc.l		0x3fff - 0x7ffe		/*  ext overflow exponent */
	.dc.l		0x3fff - 0x407e		/*  sgl overflow exponent */
	.dc.l		0x3fff - 0x43fe		/*  dbl overflow exponent */

	.globl		fsdiv
fsdiv:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#s_mode*0x10,d0	/*  insert sgl prec */
	bra.b		fdiv

	.globl		fddiv
fddiv:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#d_mode*0x10,d0	/*  insert dbl prec */

	.globl		fdiv
fdiv:
	move.l		d0,L_SCR3(a6)		/*  store rnd info */

	clr.w		d1
	move.b		DTAG(a6),d1
	lsl.b		#0x3,d1
	or.b		STAG(a6),d1		/*  combine src tags */

	bne.w		fdiv_not_norm		/*  optimize on non-norm input */

/*
 * DIVIDE: NORMs and DENORMs ONLY!
 */
fdiv_norm:
	move.w		DST_EX.w(a1),FP_SCR1_EX(a6)
	move.l		DST_HI(a1),FP_SCR1_HI(a6)
	move.l		DST_LO(a1),FP_SCR1_LO(a6)

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	bsr.l		scale_to_zero_src	/*  scale src exponent */
	move.l		d0,-(sp)		/*  save scale factor 1 */

	bsr.l		scale_to_zero_dst	/*  scale dst exponent */

	neg.l		(sp)			/*  SCALE FACTOR = scale1 - scale2 */
	add.l		d0,(sp)

	move.w		2+L_SCR3(a6),d1	/*  fetch precision */
	lsr.b		#0x6,d1		/*  shift to lo bits */
	move.l		(sp)+,d0		/*  load S.F. */
	cmp.l		(tbl_fdiv_ovfl.b,pc,d1.w*4),d0 /*  will result overflow? */
	ble.w		fdiv_may_ovfl		/*  result will overflow */

	cmp.l		(tbl_fdiv_unfl.w,pc,d1.w*4),d0 /*  will result underflow? */
	beq.w		fdiv_may_unfl		/*  maybe */
	bgt.w		fdiv_unfl		/*  yes; go handle underflow */

fdiv_normal:
	fmovem.x		FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  save FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fdiv.x		FP_SCR0(a6),fp0	/*  perform divide */

	fmove.l		fpsr,d1		/*  save FPSR */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

fdiv_normal_exit:
	fmovem.x		fp0,FP_SCR0(a6)	/*  store result on stack */
	move.l		d2,-(sp)		/*  store d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  load {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factor */
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x		FP_SCR0(a6),fp0	/*  return result in fp0 */
	rts

tbl_fdiv_ovfl2:
	.dc.l		0x7fff
	.dc.l		0x407f
	.dc.l		0x43ff

fdiv_no_ovfl:
	move.l		(sp)+,d0		/*  restore scale factor */
	bra.b		fdiv_normal_exit

fdiv_may_ovfl:
	move.l		d0,-(sp)		/*  save scale factor */

	fmovem.x		FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  set FPSR */

	fdiv.x		FP_SCR0(a6),fp0	/*  execute divide */

	fmove.l		fpsr,d0
	fmove.l		#0x0,fpcr

	or.l		d0,USER_FPSR(a6)	/*  save INEX,N */

	fmovem.x		fp0,-(sp)		/*  save result to stack */
	move.w		(sp),d0		/*  fetch new exponent */
	add.l		#0xc,sp		/*  clear result from stack */
	andi.l		#0x7fff,d0		/*  strip sign */
	sub.l		(sp),d0		/*  add scale factor */
	cmp.l		(tbl_fdiv_ovfl2.b,pc,d1.w*4),d0
	blt.b		fdiv_no_ovfl
	move.l		(sp)+,d0

fdiv_ovfl_tst:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) /*  set ovfl/aovfl/ainex */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x13,d1		/*  is OVFL or INEX enabled? */
	bne.b		fdiv_ovfl_ena		/*  yes */

fdiv_ovfl_dis:
	btst		#neg_bit,FPSR_CC(a6)	/*  is result negative? */
	sne		d1			/*  set sign param accordingly */
	move.l		L_SCR3(a6),d0		/*  pass prec:rnd */
	bsr.l		ovf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  set INF if applicable */
	fmovem.x		(a0),fp0		/*  return default result in fp0 */
	rts

fdiv_ovfl_ena:
	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  is precision extended? */
	bne.b		fdiv_ovfl_ena_sd	/*  no, do sgl or dbl */

fdiv_ovfl_ena_cont:
	fmovem.x		fp0,FP_SCR0(a6)	/*  move result to stack */

	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.w		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	sub.l		d0,d1			/*  add scale factor */
	subi.l		#0x6000,d1		/*  subtract bias */
	andi.w		#0x7fff,d1		/*  clear sign bit */
	andi.w		#0x8000,d2		/*  keep old sign */
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exponent */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.b		fdiv_ovfl_dis

fdiv_ovfl_ena_sd:
	fmovem.x		FP_SCR1(a6),fp0	/*  load dst operand */

	move.l		L_SCR3(a6),d1
	andi.b		#0x30,d1		/*  keep rnd mode */
	fmove.l		d1,fpcr		/*  set FPCR */

	fdiv.x		FP_SCR0(a6),fp0	/*  execute divide */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	bra.b		fdiv_ovfl_ena_cont

fdiv_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set unfl exc bit */

	fmovem.x		FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		#rz_mode*0x10,fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fdiv.x		FP_SCR0(a6),fp0	/*  execute divide */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0b,d1		/*  is UNFL or INEX enabled? */
	bne.b		fdiv_unfl_ena		/*  yes */

fdiv_unfl_dis:
	fmovem.x		fp0,FP_SCR0(a6)	/*  store out result */

	lea		FP_SCR0(a6),a0	/*  pass: result addr */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calculate default result */
	or.b		d0,FPSR_CC(a6)	/*  'Z' may have been set */
	fmovem.x		FP_SCR0(a6),fp0	/*  return default result in fp0 */
	rts

/*
 * UNFL is enabled.
 */
fdiv_unfl_ena:
	fmovem.x		FP_SCR1(a6),fp1	/*  load dst op */

	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  is precision extended? */
	bne.b		fdiv_unfl_ena_sd	/*  no, sgl or dbl */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

fdiv_unfl_ena_cont:
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fdiv.x		FP_SCR0(a6),fp1	/*  execute divide */

	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fmovem.x		fp1,FP_SCR0(a6)	/*  save result to stack */
	move.l		d2,-(sp)		/*  save d2 */
	move.w		FP_SCR0_EX(a6),d1	/*  fetch {sgn,exp} */
	move.l		d1,d2			/*  make a copy */
	andi.l		#0x7fff,d1		/*  strip sign */
	andi.w		#0x8000,d2		/*  keep old sign */
	sub.l		d0,d1			/*  add scale factoer */
	addi.l		#0x6000,d1		/*  add bias */
	andi.w		#0x7fff,d1
	or.w		d2,d1			/*  concat old sign,new exp */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert new exp */
	move.l		(sp)+,d2		/*  restore d2 */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	bra.w		fdiv_unfl_dis

fdiv_unfl_ena_sd:
	move.l		L_SCR3(a6),d1
	andi.b		#0x30,d1		/*  use only rnd mode */
	fmove.l		d1,fpcr		/*  set FPCR */

	bra.b		fdiv_unfl_ena_cont

/*
 * the divide operation MAY underflow:
 */
fdiv_may_unfl:
	fmovem.x		FP_SCR1(a6),fp0	/*  load dst op */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fdiv.x		FP_SCR0(a6),fp0	/*  execute divide */

	fmove.l		fpsr,d1		/*  save status */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	or.l		d1,USER_FPSR(a6)	/*  save INEX2,N */

	fabs.x		fp0,fp1		/*  make a copy of result */
	fcmp.b		#0x1,fp1		/*  is |result| > 1.b? */
	fbgt		fdiv_normal_exit	/*  no; no underflow occurred */
	fblt		fdiv_unfl		/*  yes; underflow occurred */

/*
 * we still don't know if underflow occurred. result is ~ equal to 1. but,
 * we don't know if the result was an underflow that rounded up to a 1
 * or a normalized number that rounded down to a 1. so, redo the entire
 * operation using RZ as the rounding mode to see what the pre-rounded
 * result is. this case should be relatively rare.
 */
	fmovem.x		FP_SCR1(a6),fp1	/*  load dst op into fp1 */

	move.l		L_SCR3(a6),d1
	andi.b		#0xc0,d1		/*  keep rnd prec */
	ori.b		#rz_mode*0x10,d1	/*  insert RZ */

	fmove.l		d1,fpcr		/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fdiv.x		FP_SCR0(a6),fp1	/*  execute divide */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fabs.x		fp1			/*  make absolute value */
	fcmp.b		#0x1,fp1		/*  is |result| < 1.b? */
	fbge		fdiv_normal_exit	/*  no; no underflow occurred */
	bra.w		fdiv_unfl		/*  yes; underflow occurred */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * Divide: inputs are not both normalized; what are they?
 */
fdiv_not_norm:
	move.w		(tbl_fdiv_op.b,pc,d1.w*2),d1
	jmp		(tbl_fdiv_op.b,pc,d1.w*1)

	/* swbeg		#48 */
	.dc.w 0x4afc,48
tbl_fdiv_op:
	.dc.w		fdiv_norm-tbl_fdiv_op /*  NORM / NORM */
	.dc.w		fdiv_inf_load-tbl_fdiv_op /*  NORM / ZERO */
	.dc.w		fdiv_zero_load-tbl_fdiv_op /*  NORM / INF */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  NORM / QNAN */
	.dc.w		fdiv_norm-tbl_fdiv_op /*  NORM / DENORM */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  NORM / SNAN */
	.dc.w		tbl_fdiv_op-tbl_fdiv_op
	.dc.w		tbl_fdiv_op-tbl_fdiv_op

	.dc.w		fdiv_zero_load-tbl_fdiv_op /*  ZERO / NORM */
	.dc.w		fdiv_res_operr-tbl_fdiv_op /*  ZERO / ZERO */
	.dc.w		fdiv_zero_load-tbl_fdiv_op /*  ZERO / INF */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  ZERO / QNAN */
	.dc.w		fdiv_zero_load-tbl_fdiv_op /*  ZERO / DENORM */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  ZERO / SNAN */
	.dc.w		tbl_fdiv_op-tbl_fdiv_op
	.dc.w		tbl_fdiv_op-tbl_fdiv_op

	.dc.w		fdiv_inf_dst-tbl_fdiv_op /*  INF / NORM */
	.dc.w		fdiv_inf_dst-tbl_fdiv_op /*  INF / ZERO */
	.dc.w		fdiv_res_operr-tbl_fdiv_op /*  INF / INF */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  INF / QNAN */
	.dc.w		fdiv_inf_dst-tbl_fdiv_op /*  INF / DENORM */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  INF / SNAN */
	.dc.w		tbl_fdiv_op-tbl_fdiv_op
	.dc.w		tbl_fdiv_op-tbl_fdiv_op

	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  QNAN / NORM */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  QNAN / ZERO */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  QNAN / INF */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  QNAN / QNAN */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  QNAN / DENORM */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  QNAN / SNAN */
	.dc.w		tbl_fdiv_op-tbl_fdiv_op
	.dc.w		tbl_fdiv_op-tbl_fdiv_op

	.dc.w		fdiv_norm-tbl_fdiv_op /*  DENORM / NORM */
	.dc.w		fdiv_inf_load-tbl_fdiv_op /*  DENORM / ZERO */
	.dc.w		fdiv_zero_load-tbl_fdiv_op /*  DENORM / INF */
	.dc.w		fdiv_res_qnan-tbl_fdiv_op /*  DENORM / QNAN */
	.dc.w		fdiv_norm-tbl_fdiv_op /*  DENORM / DENORM */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  DENORM / SNAN */
	.dc.w		tbl_fdiv_op-tbl_fdiv_op
	.dc.w		tbl_fdiv_op-tbl_fdiv_op

	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  SNAN / NORM */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  SNAN / ZERO */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  SNAN / INF */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  SNAN / QNAN */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  SNAN / DENORM */
	.dc.w		fdiv_res_snan-tbl_fdiv_op /*  SNAN / SNAN */
	.dc.w		tbl_fdiv_op-tbl_fdiv_op
	.dc.w		tbl_fdiv_op-tbl_fdiv_op

fdiv_res_qnan:
	bra.l		res_qnan
fdiv_res_snan:
	bra.l		res_snan
fdiv_res_operr:
	bra.l		res_operr

	.globl		fdiv_zero_load		/*  global for fsgldiv */
fdiv_zero_load:
	move.b		SRC_EX.w(a0),d0		/*  result sign is exclusive */
	move.b		DST_EX.w(a1),d1		/*  or of input signs. */
	eor.b		d0,d1
	bpl.b		fdiv_zero_load_p	/*  result is positive */
	fmove.s		#0x80000000,fp0	/*  load a -ZERO */
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6)	/*  set Z/N */
	rts
fdiv_zero_load_p:
	fmove.s		#0x00000000,fp0	/*  load a +ZERO */
	move.b		#z_bmask,FPSR_CC(a6)	/*  set Z */
	rts

/*
 * The destination was In Range and the source was a ZERO. The result,
 * Therefore, is an INF w/ the proper sign.
 * So, determine the sign and return a new INF (w/ the j-bit cleared).
 */
	.globl		fdiv_inf_load		/*  global for fsgldiv */
fdiv_inf_load:
	ori.w		#dz_mask+adz_mask,2+USER_FPSR(a6) /*  no; set DZ/ADZ */
	move.b		SRC_EX.w(a0),d0		/*  load both signs */
	move.b		DST_EX.w(a1),d1
	eor.b		d0,d1
	bpl.b		fdiv_inf_load_p		/*  result is positive */
	fmove.s		#0xff800000,fp0	/*  make result -INF */
	move.b		#inf_bmask+neg_bmask,FPSR_CC(a6) /*  set INF/N */
	rts
fdiv_inf_load_p:
	fmove.s		#0x7f800000,fp0	/*  make result +INF */
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set INF */
	rts

/*
 * The destination was an INF w/ an In Range or ZERO source, the result is
 * an INF w/ the proper sign.
 * The 68881/882 returns the destination INF w/ the new sign(if the j-bit of the
 * dst INF is set, then then j-bit of the result INF is also set).
 */
	.globl		fdiv_inf_dst		/*  global for fsgldiv */
fdiv_inf_dst:
	move.b		DST_EX.w(a1),d0		/*  load both signs */
	move.b		SRC_EX.w(a0),d1
	eor.b		d0,d1
	bpl.b		fdiv_inf_dst_p		/*  result is positive */

	fmovem.x		DST.w(a1),fp0		/*  return result in fp0 */
	fabs.x		fp0			/*  clear sign bit */
	fneg.x		fp0			/*  set sign bit */
	move.b		#inf_bmask+neg_bmask,FPSR_CC(a6) /*  set INF/NEG */
	rts

fdiv_inf_dst_p:
	fmovem.x		DST.w(a1),fp0		/*  return result in fp0 */
	fabs.x		fp0			/*  return positive INF */
	move.b		#inf_bmask,FPSR_CC(a6) /*  set INF */
	rts

