/*
 *  XDEF ****************************************************************	
 * 	fout(): move from fp register to memory or data register	
 * 									
 *  XREF ****************************************************************	
 * 	_round() - needed to create EXOP for sgl/dbl precision		
 * 	norm() - needed to create EXOP for extended precision		
 * 	ovf_res() - create default overflow result for sgl/dbl precision
 * 	unf_res() - create default underflow result for sgl/dbl prec.	
 * 	dst_dbl() - create rounded dbl precision result.		
 * 	dst_sgl() - create rounded sgl precision result.		
 * 	fetch_dreg() - fetch dynamic k-factor reg for packed.		
 * 	bindec() - convert FP binary number to packed number.		
 * 	_mem_write() - write data to memory.				
 * 	_mem_write2() - write data to memory unless supv mode -(a7) exc.
 * 	_dmem_write_{byte,word,long}() - write data to memory.		
 * 	store_dreg_{b,w,l}() - store data to data register file.	
 * 	facc_out_{b,w,l,d,x}() - data access error occurred.		
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to extended precision source operand		
 * 	d0 = round prec,mode						
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 : intermediate underflow or overflow result if		
 * 	      OVFL/UNFL occurred for a sgl or dbl operand		
 * 									
 *  ALGORITHM ***********************************************************	
 * 	This routine is accessed by many handlers that need to do an	
 *  opclass three move of an operand out to memory.			
 * 	Decode an fmove out (opclass 3) instruction to determine if	
 *  it's b,w,l,s,d,x, or p in size. b,w,l can be stored to either a data	
 *  register or memory. The algorithm uses a standard "fmove" to create	
 *  the rounded result. Also, since exceptions are disabled, this also	
 *  create the correct OPERR default result if appropriate.		
 * 	For sgl or dbl precision, overflow or underflow can occur. If	
 *  either occurs and is enabled, the EXOP.				
 * 	For extended precision, the stacked <ea> must be fixed along	
 *  w/ the address index register as appropriate w/ _calc_ea_fout(). If	
 *  the source is a denorm and if underflow is enabled, an EXOP must be	
 *  created.								
 * 	For packed, the k-factor must be fetched from the instruction	
 *  word or a data register. The <ea> must be fixed as w/ extended	
 *  precision. Then, bindec() is called to create the appropriate		
 *  packed result.							
 * 	If at any time an access error is flagged by one of the move-	
 *  to-memory routines, then a special exit must be made so that the	
 *  access error can be handled properly.					
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fout
fout:
	bfextu		EXC_CMDREG(a6){3:3},d1 /*  extract dst fmt */
	move.w		(tbl_fout.b,pc,d1.w*2),a1 /*  use as index */
	jmp			(tbl_fout.b,pc,a1.l)	/*  jump to routine */

	/* swbeg		#0x8 */
	.dc.w 0x4afc,8
tbl_fout:
	.dc.w		fout_long	-	tbl_fout
	.dc.w		fout_sgl	-	tbl_fout
	.dc.w		fout_ext	-	tbl_fout
	.dc.w		fout_pack	-	tbl_fout
	.dc.w		fout_word	-	tbl_fout
	.dc.w		fout_dbl	-	tbl_fout
	.dc.w		fout_byte	-	tbl_fout
	.dc.w		fout_pack	-	tbl_fout

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  fmove.b out ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  Only "Unimplemented Data Type" exceptions enter here. The operand */
/*  is either a DENORM or a NORM. */
fout_byte:
	tst.b		STAG(a6)		/*  is operand normalized? */
	bne.b		fout_byte_denorm	/*  no */

	fmovem.x		SRC.w(a0),fp0		/*  load value */

fout_byte_norm:
	fmove.l		d0,fpcr		/*  insert rnd prec,mode */

	fmove.b		fp0,d0		/*  exec move out w/ correct rnd mode */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  fetch FPSR */
	or.w		d1,2+USER_FPSR(a6)	/*  save new exc,accrued bits */

	move.b		1+EXC_OPWORD(a6),d1	/*  extract dst mode */
	andi.b		#0x38,d1		/*  is mode == 0? (Dreg dst) */
	beq.b		fout_byte_dn		/*  must save to integer regfile */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	bsr.l		_dmem_write_byte	/*  write byte */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_b		/*  yes */

	rts

fout_byte_dn:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract Dn */
	andi.w		#0x7,d1
	bsr.l		store_dreg_b
	rts

fout_byte_denorm:
	move.l		SRC_EX.w(a0),d1
	andi.l		#0x80000000,d1		/*  keep DENORM sign */
	ori.l		#0x00800000,d1		/*  make smallest sgl */
	fmove.s		d1,fp0
	bra.b		fout_byte_norm

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  fmove.w out ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  Only "Unimplemented Data Type" exceptions enter here. The operand */
/*  is either a DENORM or a NORM. */
fout_word:
	tst.b		STAG(a6)		/*  is operand normalized? */
	bne.b		fout_word_denorm	/*  no */

	fmovem.x		SRC.w(a0),fp0		/*  load value */

fout_word_norm:
	fmove.l		d0,fpcr		/*  insert rnd prec:mode */

	fmove.w		fp0,d0		/*  exec move out w/ correct rnd mode */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  fetch FPSR */
	or.w		d1,2+USER_FPSR(a6)	/*  save new exc,accrued bits */

	move.b		1+EXC_OPWORD(a6),d1	/*  extract dst mode */
	andi.b		#0x38,d1		/*  is mode == 0? (Dreg dst) */
	beq.b		fout_word_dn		/*  must save to integer regfile */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	bsr.l		_dmem_write_word	/*  write word */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_w		/*  yes */

	rts

fout_word_dn:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract Dn */
	andi.w		#0x7,d1
	bsr.l		store_dreg_w
	rts

fout_word_denorm:
	move.l		SRC_EX.w(a0),d1
	andi.l		#0x80000000,d1		/*  keep DENORM sign */
	ori.l		#0x00800000,d1		/*  make smallest sgl */
	fmove.s		d1,fp0
	bra.b		fout_word_norm

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  fmove.l out ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  Only "Unimplemented Data Type" exceptions enter here. The operand */
/*  is either a DENORM or a NORM. */
fout_long:
	tst.b		STAG(a6)		/*  is operand normalized? */
	bne.b		fout_long_denorm	/*  no */

	fmovem.x		SRC.w(a0),fp0		/*  load value */

fout_long_norm:
	fmove.l		d0,fpcr		/*  insert rnd prec:mode */

	fmove.l		fp0,d0		/*  exec move out w/ correct rnd mode */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  fetch FPSR */
	or.w		d1,2+USER_FPSR(a6)	/*  save new exc,accrued bits */

fout_long_write:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract dst mode */
	andi.b		#0x38,d1		/*  is mode == 0? (Dreg dst) */
	beq.b		fout_long_dn		/*  must save to integer regfile */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	bsr.l		_dmem_write_long	/*  write long */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_l		/*  yes */

	rts

fout_long_dn:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract Dn */
	andi.w		#0x7,d1
	bsr.l		store_dreg_l
	rts

fout_long_denorm:
	move.l		SRC_EX.w(a0),d1
	andi.l		#0x80000000,d1		/*  keep DENORM sign */
	ori.l		#0x00800000,d1		/*  make smallest sgl */
	fmove.s		d1,fp0
	bra.b		fout_long_norm

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  fmove.x out ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  Only "Unimplemented Data Type" exceptions enter here. The operand */
/*  is either a DENORM or a NORM. */
/*  The DENORM causes an Underflow exception. */
fout_ext:

/*  we copy the extended precision result to FP_SCR0 so that the reserved */
/*  16-bit field gets zeroed. we do this since we promise not to disturb */
/*  what's at SRC(a0). */
	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	clr.w		2+FP_SCR0_EX(a6)	/*  clear reserved field */
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	fmovem.x		SRC.w(a0),fp0		/*  return result */

	bsr.l		_calc_ea_fout		/*  fix stacked <ea> */

	move.l		a0,a1			/*  pass: dst addr */
	lea		FP_SCR0(a6),a0	/*  pass: src addr */
	moveq.l		#0xc,d0		/*  pass: opsize is 12 bytes */

/*  we must not yet write the extended precision data to the stack */
/*  in the pre-decrement case from supervisor mode or else we'll corrupt */
/*  the stack frame. so, leave it in FP_SRC for now and deal with it later... */
	cmpi.b		#mda7_flg,SPCOND_FLG(a6)
	beq.b		fout_ext_a7

	bsr.l		_dmem_write		/*  write ext prec number to memory */

	tst.l		d1			/*  did dstore fail? */
	bne.w		fout_ext_err		/*  yes */

	tst.b		STAG(a6)		/*  is operand normalized? */
	bne.b		fout_ext_denorm		/*  no */
	rts

/*  the number is a DENORM. must set the underflow exception bit */
fout_ext_denorm:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set underflow exc bit */

	move.b		FPCR_ENABLE(a6),d0
	andi.b		#0x0a,d0		/*  is UNFL or INEX enabled? */
	bne.b		fout_ext_exc		/*  yes */
	rts

/*  we don't want to do the write if the exception occurred in supervisor mode */
/*  so _mem_write2() handles this for us. */
fout_ext_a7:
	bsr.l		_mem_write2		/*  write ext prec number to memory */

	tst.l		d1			/*  did dstore fail? */
	bne.w		fout_ext_err		/*  yes */

	tst.b		STAG(a6)		/*  is operand normalized? */
	bne.b		fout_ext_denorm		/*  no */
	rts

fout_ext_exc:
	lea		FP_SCR0(a6),a0
	bsr.l		norm			/*  normalize the mantissa */
	neg.w		d0			/*  new exp = -(shft amt) */
	andi.w		#0x7fff,d0
	andi.w		#0x8000,FP_SCR0_EX(a6)	/*  keep only old sign */
	or.w		d0,FP_SCR0_EX(a6)	/*  insert new exponent */
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	rts

fout_ext_err:
	move.l		EXC_A6(a6),(a6)	/*  fix stacked a6 */
	bra.l		facc_out_x

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  fmove.s out ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fout_sgl:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#s_mode*0x10,d0	/*  insert sgl prec */
	move.l		d0,L_SCR3(a6)		/*  save rnd prec,mode on stack */

/*
 * operand is a normalized number. first, we check to see if the move out
 * would cause either an underflow or overflow. these cases are handled
 * separately. otherwise, set the FPCR to the proper rounding mode and
 * execute the move.
 */
	move.w		SRC_EX.w(a0),d0		/*  extract exponent */
	andi.w		#0x7fff,d0		/*  strip sign */

	cmpi.w		#SGL_HI,d0		/*  will operand overflow? */
	bgt.w		fout_sgl_ovfl		/*  yes; go handle OVFL */
	beq.w		fout_sgl_may_ovfl	/*  maybe; go handle possible OVFL */
	cmpi.w		#SGL_LO,d0		/*  will operand underflow? */
	blt.w		fout_sgl_unfl		/*  yes; go handle underflow */

/*
 * NORMs(in range) can be stored out by a simple "fmove.s"
 * Unnormalized inputs can come through this point.
 */
fout_sgl_exg:
	fmovem.x		SRC.w(a0),fp0		/*  fetch fop from stack */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmove.s		fp0,d0		/*  store does convert and round */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d1		/*  save FPSR */

	or.w		d1,2+USER_FPSR(a6)	/*  set possible inex2/ainex */

fout_sgl_exg_write:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract dst mode */
	andi.b		#0x38,d1		/*  is mode == 0? (Dreg dst) */
	beq.b		fout_sgl_exg_write_dn	/*  must save to integer regfile */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	bsr.l		_dmem_write_long	/*  write long */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_l		/*  yes */

	rts

fout_sgl_exg_write_dn:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract Dn */
	andi.w		#0x7,d1
	bsr.l		store_dreg_l
	rts

/*
 * here, we know that the operand would UNFL if moved out to single prec,
 * so, denorm and round and then use generic store single routine to
 * write the value to memory.
 */
fout_sgl_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set UNFL */

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	move.l		a0,-(sp)

	clr.l		d0			/*  pass: S.F. = 0 */

	cmpi.b		#DENORM,STAG(a6)	/*  fetch src optype tag */
	bne.b		fout_sgl_unfl_cont	/*  let DENORMs fall through */

	lea		FP_SCR0(a6),a0
	bsr.l		norm			/*  normalize the DENORM */

fout_sgl_unfl_cont:
	lea		FP_SCR0(a6),a0	/*  pass: ptr to operand */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calc default underflow result */

	lea		FP_SCR0(a6),a0	/*  pass: ptr to fop */
	bsr.l		dst_sgl			/*  convert to single prec */

	move.b		1+EXC_OPWORD(a6),d1	/*  extract dst mode */
	andi.b		#0x38,d1		/*  is mode == 0? (Dreg dst) */
	beq.b		fout_sgl_unfl_dn	/*  must save to integer regfile */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	bsr.l		_dmem_write_long	/*  write long */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_l		/*  yes */

	bra.b		fout_sgl_unfl_chkexc

fout_sgl_unfl_dn:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract Dn */
	andi.w		#0x7,d1
	bsr.l		store_dreg_l

fout_sgl_unfl_chkexc:
	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0a,d1		/*  is UNFL or INEX enabled? */
	bne.w		fout_sd_exc_unfl	/*  yes */
	addq.l		#0x4,sp
	rts

/*
 * it's definitely an overflow so call ovf_res to get the correct answer
 */
fout_sgl_ovfl:
	tst.b		3+SRC_HI(a0)		/*  is result inexact? */
	bne.b		fout_sgl_ovfl_inex2
	tst.l		SRC_LO(a0)		/*  is result inexact? */
	bne.b		fout_sgl_ovfl_inex2
	ori.w		#ovfl_inx_mask,2+USER_FPSR(a6) /*  set ovfl/aovfl/ainex */
	bra.b		fout_sgl_ovfl_cont
fout_sgl_ovfl_inex2:
	ori.w		#ovfinx_mask,2+USER_FPSR(a6) /*  set ovfl/aovfl/ainex/inex2 */

fout_sgl_ovfl_cont:
	move.l		a0,-(sp)

/*  call ovf_res() w/ sgl prec and the correct rnd mode to create the default */
/*  overflow result. DON'T save the returned ccodes from ovf_res() since */
/*  fmove out doesn't alter them. */
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	smi		d1			/*  set if so */
	move.l		L_SCR3(a6),d0		/*  pass: sgl prec,rnd mode */
	bsr.l		ovf_res			/*  calc OVFL result */
	fmovem.x		(a0),fp0		/*  load default overflow result */
	fmove.s		fp0,d0		/*  store to single */

	move.b		1+EXC_OPWORD(a6),d1	/*  extract dst mode */
	andi.b		#0x38,d1		/*  is mode == 0? (Dreg dst) */
	beq.b		fout_sgl_ovfl_dn	/*  must save to integer regfile */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	bsr.l		_dmem_write_long	/*  write long */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_l		/*  yes */

	bra.b		fout_sgl_ovfl_chkexc

fout_sgl_ovfl_dn:
	move.b		1+EXC_OPWORD(a6),d1	/*  extract Dn */
	andi.w		#0x7,d1
	bsr.l		store_dreg_l

fout_sgl_ovfl_chkexc:
	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0a,d1		/*  is UNFL or INEX enabled? */
	bne.w		fout_sd_exc_ovfl	/*  yes */
	addq.l		#0x4,sp
	rts

/*
 * move out MAY overflow:
 * (1) force the exp to 0x3fff
 * (2) do a move w/ appropriate rnd mode
 * (3) if exp still equals zero, then insert original exponent
 *	for the correct result.
 *     if exp now equals one, then it overflowed so call ovf_res.
 */
fout_sgl_may_ovfl:
	move.w		SRC_EX.w(a0),d1		/*  fetch current sign */
	andi.w		#0x8000,d1		/*  keep it,clear exp */
	ori.w		#0x3fff,d1		/*  insert exp = 0 */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert scaled exp */
	move.l		SRC_HI(a0),FP_SCR0_HI(a6) /*  copy hi(man) */
	move.l		SRC_LO(a0),FP_SCR0_LO(a6) /*  copy lo(man) */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fmove.x		FP_SCR0(a6),fp0	/*  force fop to be rounded */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fabs.x		fp0			/*  need absolute value */
	fcmp.b		#0x2,fp0		/*  did exponent increase? */
	fblt		fout_sgl_exg		/*  no; go finish NORM */
	bra.w		fout_sgl_ovfl		/*  yes; go handle overflow */

/* ;;;;;;;;;;;;;; */

fout_sd_exc_unfl:
	move.l		(sp)+,a0

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

	cmpi.b		#DENORM,STAG(a6)	/*  was src a DENORM? */
	bne.b		fout_sd_exc_cont	/*  no */

	lea		FP_SCR0(a6),a0
	bsr.l		norm
	neg.l		d0
	andi.w		#0x7fff,d0
	bfins		d0,FP_SCR0_EX(a6){1:15}
	bra.b		fout_sd_exc_cont

fout_sd_exc:
fout_sd_exc_ovfl:
	move.l		(sp)+,a0		/*  restore a0 */

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)

fout_sd_exc_cont:
	bclr		#0x7,FP_SCR0_EX(a6)	/*  clear sign bit */
	sne.b		2+FP_SCR0_EX(a6)	/*  set internal sign bit */
	lea		FP_SCR0(a6),a0	/*  pass: ptr to DENORM */

	move.b		3+L_SCR3(a6),d1
	lsr.b		#0x4,d1
	andi.w		#0x0c,d1
	swap		d1
	move.b		3+L_SCR3(a6),d1
	lsr.b		#0x4,d1
	andi.w		#0x03,d1
	clr.l		d0			/*  pass: zero g,r,s */
	bsr.l		_round			/*  round the DENORM */

	tst.b		2+FP_SCR0_EX(a6)	/*  is EXOP negative? */
	beq.b		fout_sd_exc_done	/*  no */
	bset		#0x7,FP_SCR0_EX(a6)	/*  yes */

fout_sd_exc_done:
	fmovem.x		FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  fmove.d out ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fout_dbl:
	andi.b		#0x30,d0		/*  clear rnd prec */
	ori.b		#d_mode*0x10,d0	/*  insert dbl prec */
	move.l		d0,L_SCR3(a6)		/*  save rnd prec,mode on stack */

/*
 * operand is a normalized number. first, we check to see if the move out
 * would cause either an underflow or overflow. these cases are handled
 * separately. otherwise, set the FPCR to the proper rounding mode and
 * execute the move.
 */
	move.w		SRC_EX.w(a0),d0		/*  extract exponent */
	andi.w		#0x7fff,d0		/*  strip sign */

	cmpi.w		#DBL_HI,d0		/*  will operand overflow? */
	bgt.w		fout_dbl_ovfl		/*  yes; go handle OVFL */
	beq.w		fout_dbl_may_ovfl	/*  maybe; go handle possible OVFL */
	cmpi.w		#DBL_LO,d0		/*  will operand underflow? */
	blt.w		fout_dbl_unfl		/*  yes; go handle underflow */

/*
 * NORMs(in range) can be stored out by a simple "fmove.d"
 * Unnormalized inputs can come through this point.
 */
fout_dbl_exg:
	fmovem.x		SRC.w(a0),fp0		/*  fetch fop from stack */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	fmove.d		fp0,L_SCR1(a6)	/*  store does convert and round */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		fpsr,d0		/*  save FPSR */

	or.w		d0,2+USER_FPSR(a6)	/*  set possible inex2/ainex */

	move.l		EXC_EA(a6),a1		/*  pass: dst addr */
	lea		L_SCR1(a6),a0		/*  pass: src addr */
	moveq.l		#0x8,d0		/*  pass: opsize is 8 bytes */
	bsr.l		_dmem_write		/*  store dbl fop to memory */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_d		/*  yes */

	rts					/*  no; so we're finished */

/*
 * here, we know that the operand would UNFL if moved out to double prec,
 * so, denorm and round and then use generic store double routine to
 * write the value to memory.
 */
fout_dbl_unfl:
	bset		#unfl_bit,FPSR_EXCEPT(a6) /*  set UNFL */

	move.w		SRC_EX.w(a0),FP_SCR0_EX(a6)
	move.l		SRC_HI(a0),FP_SCR0_HI(a6)
	move.l		SRC_LO(a0),FP_SCR0_LO(a6)
	move.l		a0,-(sp)

	clr.l		d0			/*  pass: S.F. = 0 */

	cmpi.b		#DENORM,STAG(a6)	/*  fetch src optype tag */
	bne.b		fout_dbl_unfl_cont	/*  let DENORMs fall through */

	lea		FP_SCR0(a6),a0
	bsr.l		norm			/*  normalize the DENORM */

fout_dbl_unfl_cont:
	lea		FP_SCR0(a6),a0	/*  pass: ptr to operand */
	move.l		L_SCR3(a6),d1		/*  pass: rnd prec,mode */
	bsr.l		unf_res			/*  calc default underflow result */

	lea		FP_SCR0(a6),a0	/*  pass: ptr to fop */
	bsr.l		dst_dbl			/*  convert to single prec */
	move.l		d0,L_SCR1(a6)
	move.l		d1,L_SCR2(a6)

	move.l		EXC_EA(a6),a1		/*  pass: dst addr */
	lea		L_SCR1(a6),a0		/*  pass: src addr */
	moveq.l		#0x8,d0		/*  pass: opsize is 8 bytes */
	bsr.l		_dmem_write		/*  store dbl fop to memory */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_d		/*  yes */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0a,d1		/*  is UNFL or INEX enabled? */
	bne.w		fout_sd_exc_unfl	/*  yes */
	addq.l		#0x4,sp
	rts

/*
 * it's definitely an overflow so call ovf_res to get the correct answer
 */
fout_dbl_ovfl:
	move.w		2+SRC_LO(a0),d0
	andi.w		#0x7ff,d0
	bne.b		fout_dbl_ovfl_inex2

	ori.w		#ovfl_inx_mask,2+USER_FPSR(a6) /*  set ovfl/aovfl/ainex */
	bra.b		fout_dbl_ovfl_cont
fout_dbl_ovfl_inex2:
	ori.w		#ovfinx_mask,2+USER_FPSR(a6) /*  set ovfl/aovfl/ainex/inex2 */

fout_dbl_ovfl_cont:
	move.l		a0,-(sp)

/*  call ovf_res() w/ dbl prec and the correct rnd mode to create the default */
/*  overflow result. DON'T save the returned ccodes from ovf_res() since */
/*  fmove out doesn't alter them. */
	tst.b		SRC_EX.w(a0)		/*  is operand negative? */
	smi		d1			/*  set if so */
	move.l		L_SCR3(a6),d0		/*  pass: dbl prec,rnd mode */
	bsr.l		ovf_res			/*  calc OVFL result */
	fmovem.x		(a0),fp0		/*  load default overflow result */
	fmove.d		fp0,L_SCR1(a6)	/*  store to double */

	move.l		EXC_EA(a6),a1		/*  pass: dst addr */
	lea		L_SCR1(a6),a0		/*  pass: src addr */
	moveq.l		#0x8,d0		/*  pass: opsize is 8 bytes */
	bsr.l		_dmem_write		/*  store dbl fop to memory */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_d		/*  yes */

	move.b		FPCR_ENABLE(a6),d1
	andi.b		#0x0a,d1		/*  is UNFL or INEX enabled? */
	bne.w		fout_sd_exc_ovfl	/*  yes */
	addq.l		#0x4,sp
	rts

/*
 * move out MAY overflow:
 * (1) force the exp to 0x3fff
 * (2) do a move w/ appropriate rnd mode
 * (3) if exp still equals zero, then insert original exponent
 *	for the correct result.
 *     if exp now equals one, then it overflowed so call ovf_res.
 */
fout_dbl_may_ovfl:
	move.w		SRC_EX.w(a0),d1		/*  fetch current sign */
	andi.w		#0x8000,d1		/*  keep it,clear exp */
	ori.w		#0x3fff,d1		/*  insert exp = 0 */
	move.w		d1,FP_SCR0_EX(a6)	/*  insert scaled exp */
	move.l		SRC_HI(a0),FP_SCR0_HI(a6) /*  copy hi(man) */
	move.l		SRC_LO(a0),FP_SCR0_LO(a6) /*  copy lo(man) */

	fmove.l		L_SCR3(a6),fpcr	/*  set FPCR */

	fmove.x		FP_SCR0(a6),fp0	/*  force fop to be rounded */
	fmove.l		#0x0,fpcr		/*  clear FPCR */

	fabs.x		fp0			/*  need absolute value */
	fcmp.b		#0x2,fp0		/*  did exponent increase? */
	fblt		fout_dbl_exg		/*  no; go finish NORM */
	bra.w		fout_dbl_ovfl		/*  yes; go handle overflow */

/*
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 * XDEF ****************************************************************
 *      dst_dbl(): create double precision value from extended prec.
 *
 * XREF ****************************************************************
 *      None
 *
 * INPUT ***************************************************************
 *      a0 = pointer to source operand in extended precision
 *
 * OUTPUT **************************************************************
 *      d0 = hi(double precision result)
 *      d1 = lo(double precision result)
 *
 * ALGORITHM ***********************************************************
 *
 *  Changes extended precision to double precision.
 *  Note: no attempt is made to round the extended value to double.
 *      dbl_sign = ext_sign
 *      dbl_exp = ext_exp - $3fff(ext bias) + $7ff(dbl bias)
 *      get rid of ext integer bit
 *      dbl_mant = ext_mant{62:12}
 *
 *              ---------------   ---------------    ---------------
 *  extended ->  |s|    exp    |   |1| ms mant   |    | ls mant     |
 *              ---------------   ---------------    ---------------
 *               95         64    63 62       32      31     11   0
 *                                   |                       |
 *                                   |                       |
 *                                   |                       |
 *                                   v                       v
 *                            ---------------   ---------------
 *  double   ->               |s|exp| mant  |   |  mant       |
 *                            ---------------   ---------------
 *                            63     51   32   31              0
 *
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 */
dst_dbl:
	clr.l		d0			/*  clear d0 */
	move.w		FTEMP_EX.w(a0),d0	/*  get exponent */
	subi.w		#EXT_BIAS,d0		/*  subtract extended precision bias */
	addi.w		#DBL_BIAS,d0		/*  add double precision bias */
	tst.b		FTEMP_HI(a0)		/*  is number a denorm? */
	bmi.b		dst_get_dupper		/*  no */
	subq.w		#0x1,d0		/*  yes; denorm bias = DBL_BIAS - 1 */
dst_get_dupper:
	swap		d0			/*  d0 now in upper word */
	lsl.l		#0x4,d0		/*  d0 in proper place for dbl prec exp */
	tst.b		FTEMP_EX.w(a0)		/*  test sign */
	bpl.b		dst_get_dman		/*  if positive, go process mantissa */
	bset		#0x1f,d0		/*  if negative, set sign */
dst_get_dman:
	move.l		FTEMP_HI(a0),d1	/*  get ms mantissa */
	bfextu		d1{1:20},d1		/*  get upper 20 bits of ms */
	or.l		d1,d0			/*  put these bits in ms word of double */
	move.l		d0,L_SCR1(a6)		/*  put the new exp back on the stack */
	move.l		FTEMP_HI(a0),d1	/*  get ms mantissa */
	moveq.l		#21,d0			/*  load shift count */
	lsl.l		d0,d1			/*  put lower 11 bits in upper bits */
	move.l		d1,L_SCR2(a6)		/*  build lower lword in memory */
	move.l		FTEMP_LO(a0),d1	/*  get ls mantissa */
	bfextu		d1{0:21},d0		/*  get ls 21 bits of double */
	move.l		L_SCR2(a6),d1
	or.l		d0,d1			/*  put them in double result */
	move.l		L_SCR1(a6),d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	dst_sgl(): create single precision value from extended prec	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	a0 = pointer to source operand in extended precision		 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	d0 = single precision result					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 									 */
/*  Changes extended precision to single precision.			 */
/* 	sgl_sign = ext_sign						 */
/* 	sgl_exp = ext_exp - $3fff(ext bias) + $7f(sgl bias)		 */
/* 	get rid of ext integer bit					 */
/* 	sgl_mant = ext_mant{62:12}					 */
/* 									 */
/* 		---------------   ---------------    ---------------	 */
/*   extended ->  |s|    exp    |   |1| ms mant   |    | ls mant     |	 */
/* 		---------------   ---------------    ---------------	 */
/* 		 95	    64    63 62	   40 32      31     12	  0	 */
/* 				     |	   |				 */
/* 				     |	   |				 */
/* 				     |	   |				 */
/* 			             v     v				 */
/* 			      ---------------				 */
/*   single   ->		      |s|exp| mant  |				 */
/* 			      ---------------				 */
/* 			      31     22     0				 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

dst_sgl:
	clr.l		d0
	move.w		FTEMP_EX.w(a0),d0	/*  get exponent */
	subi.w		#EXT_BIAS,d0		/*  subtract extended precision bias */
	addi.w		#SGL_BIAS,d0		/*  add single precision bias */
	tst.b		FTEMP_HI(a0)		/*  is number a denorm? */
	bmi.b		dst_get_supper		/*  no */
	subq.w		#0x1,d0		/*  yes; denorm bias = SGL_BIAS - 1 */
dst_get_supper:
	swap		d0			/*  put exp in upper word of d0 */
	lsl.l		#0x7,d0		/*  shift it into single exp bits */
	tst.b		FTEMP_EX.w(a0)		/*  test sign */
	bpl.b		dst_get_sman		/*  if positive, continue */
	bset		#0x1f,d0		/*  if negative, put in sign first */
dst_get_sman:
	move.l		FTEMP_HI(a0),d1	/*  get ms mantissa */
	andi.l		#0x7fffff00,d1		/*  get upper 23 bits of ms */
	lsr.l		#0x8,d1		/*  and put them flush right */
	or.l		d1,d0			/*  put these bits in ms word of single */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fout_pack:
	bsr.l		_calc_ea_fout		/*  fetch the <ea> */
	move.l		a0,-(sp)

	move.b		STAG(a6),d0		/*  fetch input type */
	bne.w		fout_pack_not_norm	/*  input is not NORM */

fout_pack_norm:
	btst		#0x4,EXC_CMDREG(a6)	/*  static or dynamic? */
	beq.b		fout_pack_s		/*  static */

fout_pack_d:
	move.b		1+EXC_CMDREG(a6),d1	/*  fetch dynamic reg */
	lsr.b		#0x4,d1
	andi.w		#0x7,d1

	bsr.l		fetch_dreg		/*  fetch Dn w/ k-factor */

	bra.b		fout_pack_type
fout_pack_s:
	move.b		1+EXC_CMDREG(a6),d0	/*  fetch static field */

fout_pack_type:
	bfexts		d0{25:7},d0		/*  extract k-factor */
	move.l	d0,-(sp)

	lea		FP_SRC(a6),a0		/*  pass: ptr to input */

/*  bindec is currently scrambling FP_SRC for denorm inputs. */
/*  we'll have to change this, but for now, tough luck!!! */
	bsr.l		bindec			/*  convert xprec to packed */

/* 	andi.l		#0xcfff000f,FP_SCR0(a6) ; clear unused fields */
	andi.l		#0xcffff00f,FP_SCR0(a6) /*  clear unused fields */

	move.l	(sp)+,d0

	tst.b		3+FP_SCR0_EX(a6)
	bne.b		fout_pack_set
	tst.l		FP_SCR0_HI(a6)
	bne.b		fout_pack_set
	tst.l		FP_SCR0_LO(a6)
	bne.b		fout_pack_set

/*  add the extra condition that only if the k-factor was zero, too, should */
/*  we zero the exponent */
	tst.l		d0
	bne.b		fout_pack_set
/*  "mantissa" is all zero which means that the answer is zero. but, the '040 */
/*  algorithm allows the exponent to be non-zero. the 881/2 do not. Therefore, */
/*  if the mantissa is zero, I will zero the exponent, too. */
/*  the question now is whether the exponents sign bit is allowed to be non-zero */
/*  for a zero, also... */
	andi.w		#0xf000,FP_SCR0(a6)

fout_pack_set:

	lea		FP_SCR0(a6),a0	/*  pass: src addr */

fout_pack_write:
	move.l		(sp)+,a1		/*  pass: dst addr */
	moveq.l		#0xc,d0		/*  pass: opsize is 12 bytes */

	cmpi.b		#mda7_flg,SPCOND_FLG(a6)
	beq.b		fout_pack_a7

	bsr.l		_dmem_write		/*  write ext prec number to memory */

	tst.l		d1			/*  did dstore fail? */
	bne.w		fout_ext_err		/*  yes */

	rts

/*  we don't want to do the write if the exception occurred in supervisor mode */
/*  so _mem_write2() handles this for us. */
fout_pack_a7:
	bsr.l		_mem_write2		/*  write ext prec number to memory */

	tst.l		d1			/*  did dstore fail? */
	bne.w		fout_ext_err		/*  yes */

	rts

fout_pack_not_norm:
	cmpi.b		#DENORM,d0		/*  is it a DENORM? */
	beq.w		fout_pack_norm		/*  yes */
	lea		FP_SRC(a6),a0
	clr.w		2+FP_SRC_EX(a6)
	cmpi.b		#SNAN,d0		/*  is it an SNAN? */
	beq.b		fout_pack_snan		/*  yes */
	bra.b		fout_pack_write		/*  no */

fout_pack_snan:
	ori.w		#snaniop2_mask,FPSR_EXCEPT(a6) /*  set SNAN/AIOP */
	bset		#0x6,FP_SRC_HI(a6)	/*  set snan bit */
	bra.b		fout_pack_write

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	fetch_dreg(): fetch register according to index in d1		 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	d1 = index of register to fetch from				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	d0 = value of register fetched					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	According to the index value in d1 which can range from zero	 */
/*  to fifteen, load the corresponding register file value (where		 */
/*  address register indexes start at 8). D0/D1/A0/A1/A6/A7 are on the	 */
/*  stack. The rest should still be in their original places.		 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  this routine leaves d1 intact for subsequent store_dreg calls. */
	.globl		fetch_dreg
fetch_dreg:
	move.w		(tbl_fdreg.b,pc,d1.w*2),d0
	jmp		(tbl_fdreg.b,pc,d0.w*1)

tbl_fdreg:
	.dc.w		fdreg0 - tbl_fdreg
	.dc.w		fdreg1 - tbl_fdreg
	.dc.w		fdreg2 - tbl_fdreg
	.dc.w		fdreg3 - tbl_fdreg
	.dc.w		fdreg4 - tbl_fdreg
	.dc.w		fdreg5 - tbl_fdreg
	.dc.w		fdreg6 - tbl_fdreg
	.dc.w		fdreg7 - tbl_fdreg
	.dc.w		fdreg8 - tbl_fdreg
	.dc.w		fdreg9 - tbl_fdreg
	.dc.w		fdrega - tbl_fdreg
	.dc.w		fdregb - tbl_fdreg
	.dc.w		fdregc - tbl_fdreg
	.dc.w		fdregd - tbl_fdreg
	.dc.w		fdrege - tbl_fdreg
	.dc.w		fdregf - tbl_fdreg

fdreg0:
	move.l		EXC_DREGS+0x0(a6),d0
	rts
fdreg1:
	move.l		EXC_DREGS+0x4(a6),d0
	rts
fdreg2:
	move.l		d2,d0
	rts
fdreg3:
	move.l		d3,d0
	rts
fdreg4:
	move.l		d4,d0
	rts
fdreg5:
	move.l		d5,d0
	rts
fdreg6:
	move.l		d6,d0
	rts
fdreg7:
	move.l		d7,d0
	rts
fdreg8:
	move.l		EXC_DREGS+0x8(a6),d0
	rts
fdreg9:
	move.l		EXC_DREGS+0xc(a6),d0
	rts
fdrega:
	move.l		a2,d0
	rts
fdregb:
	move.l		a3,d0
	rts
fdregc:
	move.l		a4,d0
	rts
fdregd:
	move.l		a5,d0
	rts
fdrege:
	move.l		(a6),d0
	rts
fdregf:
	move.l		EXC_A7(a6),d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	store_dreg_l(): store longword to data register specified by d1	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	d0 = longowrd value to store					 */
/* 	d1 = index of register to fetch from				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	(data register is updated)					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	According to the index value in d1, store the longword value	 */
/*  in d0 to the corresponding data register. D0/D1 are on the stack	 */
/*  while the rest are in their initial places.				 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		store_dreg_l
store_dreg_l:
	move.w		(tbl_sdregl.b,pc,d1.w*2),d1
	jmp		(tbl_sdregl.b,pc,d1.w*1)

tbl_sdregl:
	.dc.w		sdregl0 - tbl_sdregl
	.dc.w		sdregl1 - tbl_sdregl
	.dc.w		sdregl2 - tbl_sdregl
	.dc.w		sdregl3 - tbl_sdregl
	.dc.w		sdregl4 - tbl_sdregl
	.dc.w		sdregl5 - tbl_sdregl
	.dc.w		sdregl6 - tbl_sdregl
	.dc.w		sdregl7 - tbl_sdregl

sdregl0:
	move.l		d0,EXC_DREGS+0x0(a6)
	rts
sdregl1:
	move.l		d0,EXC_DREGS+0x4(a6)
	rts
sdregl2:
	move.l		d0,d2
	rts
sdregl3:
	move.l		d0,d3
	rts
sdregl4:
	move.l		d0,d4
	rts
sdregl5:
	move.l		d0,d5
	rts
sdregl6:
	move.l		d0,d6
	rts
sdregl7:
	move.l		d0,d7
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	store_dreg_w(): store word to data register specified by d1	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	d0 = word value to store					 */
/* 	d1 = index of register to fetch from				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	(data register is updated)					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	According to the index value in d1, store the word value	 */
/*  in d0 to the corresponding data register. D0/D1 are on the stack	 */
/*  while the rest are in their initial places.				 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		store_dreg_w
store_dreg_w:
	move.w		(tbl_sdregw.b,pc,d1.w*2),d1
	jmp		(tbl_sdregw.b,pc,d1.w*1)

tbl_sdregw:
	.dc.w		sdregw0 - tbl_sdregw
	.dc.w		sdregw1 - tbl_sdregw
	.dc.w		sdregw2 - tbl_sdregw
	.dc.w		sdregw3 - tbl_sdregw
	.dc.w		sdregw4 - tbl_sdregw
	.dc.w		sdregw5 - tbl_sdregw
	.dc.w		sdregw6 - tbl_sdregw
	.dc.w		sdregw7 - tbl_sdregw

sdregw0:
	move.w		d0,2+EXC_DREGS+0x0(a6)
	rts
sdregw1:
	move.w		d0,2+EXC_DREGS+0x4(a6)
	rts
sdregw2:
	move.w		d0,d2
	rts
sdregw3:
	move.w		d0,d3
	rts
sdregw4:
	move.w		d0,d4
	rts
sdregw5:
	move.w		d0,d5
	rts
sdregw6:
	move.w		d0,d6
	rts
sdregw7:
	move.w		d0,d7
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	store_dreg_b(): store byte to data register specified by d1	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	d0 = byte value to store					 */
/* 	d1 = index of register to fetch from				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	(data register is updated)					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	According to the index value in d1, store the byte value	 */
/*  in d0 to the corresponding data register. D0/D1 are on the stack	 */
/*  while the rest are in their initial places.				 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		store_dreg_b
store_dreg_b:
	move.w		(tbl_sdregb.b,pc,d1.w*2),d1
	jmp		(tbl_sdregb.b,pc,d1.w*1)

tbl_sdregb:
	.dc.w		sdregb0 - tbl_sdregb
	.dc.w		sdregb1 - tbl_sdregb
	.dc.w		sdregb2 - tbl_sdregb
	.dc.w		sdregb3 - tbl_sdregb
	.dc.w		sdregb4 - tbl_sdregb
	.dc.w		sdregb5 - tbl_sdregb
	.dc.w		sdregb6 - tbl_sdregb
	.dc.w		sdregb7 - tbl_sdregb

sdregb0:
	move.b		d0,3+EXC_DREGS+0x0(a6)
	rts
sdregb1:
	move.b		d0,3+EXC_DREGS+0x4(a6)
	rts
sdregb2:
	move.b		d0,d2
	rts
sdregb3:
	move.b		d0,d3
	rts
sdregb4:
	move.b		d0,d4
	rts
sdregb5:
	move.b		d0,d5
	rts
sdregb6:
	move.b		d0,d6
	rts
sdregb7:
	move.b		d0,d7
	rts

