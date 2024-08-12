/*
 *  fscc(): routine to emulate the fscc instruction			
 * 									
 *  XDEF **************************************************************** 
 * 	_fscc()								
 * 									
 *  XREF **************************************************************** 
 * 	store_dreg_b() - store result to data register file		
 * 	dec_areg() - decrement an areg for -(an) mode			
 * 	inc_areg() - increment an areg for (an)+ mode			
 * 	_dmem_write_byte() - store result to memory			
 * 									
 *  INPUT ***************************************************************	
 * 	none								
 * 									
 *  OUTPUT ************************************************************** 
 * 	none								
 * 									
 *  ALGORITHM ***********************************************************	
 * 	This routine checks which conditional predicate is specified by	
 *  the stacked fscc instruction opcode and then branches to a routine	
 *  for that predicate. The corresponding fbcc instruction is then used	
 *  to see whether the condition (specified by the stacked FPSR) is true	
 *  or false.								
 * 	If a BSUN exception should be indicated, the BSUN and ABSUN	
 *  bits are set in the stacked FPSR. If the BSUN exception is enabled,	
 *  the fbsun_flg is set in the SPCOND_FLG location on the stack. If an	
 *  enabled BSUN should not be flagged and the predicate is true, then	
 *  the result is stored to the data register file or memory		
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		_fscc
_fscc:
	move.w		EXC_CMDREG(a6),d0	/*  fetch predicate */

	clr.l		d1			/*  clear scratch reg */
	move.b		FPSR_CC(a6),d1	/*  fetch fp ccodes */
	ror.l		#0x8,d1		/*  rotate to top byte */
	fmove.l		d1,fpsr		/*  insert into FPSR */

	move.w		(tbl_fscc.b,pc,d0.w*2),d1 /*  load table */
	jmp		(tbl_fscc.b,pc,d1.w)	/*  jump to fscc routine */

tbl_fscc:
	.dc.w		fscc_f		-	tbl_fscc	/*  00 */
	.dc.w		fscc_eq		-	tbl_fscc	/*  01 */
	.dc.w		fscc_ogt	-	tbl_fscc	/*  02 */
	.dc.w		fscc_oge	-	tbl_fscc	/*  03 */
	.dc.w		fscc_olt	-	tbl_fscc	/*  04 */
	.dc.w		fscc_ole	-	tbl_fscc	/*  05 */
	.dc.w		fscc_ogl	-	tbl_fscc	/*  06 */
	.dc.w		fscc_or		-	tbl_fscc	/*  07 */
	.dc.w		fscc_un		-	tbl_fscc	/*  08 */
	.dc.w		fscc_ueq	-	tbl_fscc	/*  09 */
	.dc.w		fscc_ugt	-	tbl_fscc	/*  10 */
	.dc.w		fscc_uge	-	tbl_fscc	/*  11 */
	.dc.w		fscc_ult	-	tbl_fscc	/*  12 */
	.dc.w		fscc_ule	-	tbl_fscc	/*  13 */
	.dc.w		fscc_neq	-	tbl_fscc	/*  14 */
	.dc.w		fscc_t		-	tbl_fscc	/*  15 */
	.dc.w		fscc_sf		-	tbl_fscc	/*  16 */
	.dc.w		fscc_seq	-	tbl_fscc	/*  17 */
	.dc.w		fscc_gt		-	tbl_fscc	/*  18 */
	.dc.w		fscc_ge		-	tbl_fscc	/*  19 */
	.dc.w		fscc_lt		-	tbl_fscc	/*  20 */
	.dc.w		fscc_le		-	tbl_fscc	/*  21 */
	.dc.w		fscc_gl		-	tbl_fscc	/*  22 */
	.dc.w		fscc_gle	-	tbl_fscc	/*  23 */
	.dc.w		fscc_ngle	-	tbl_fscc	/*  24 */
	.dc.w		fscc_ngl	-	tbl_fscc	/*  25 */
	.dc.w		fscc_nle	-	tbl_fscc	/*  26 */
	.dc.w		fscc_nlt	-	tbl_fscc	/*  27 */
	.dc.w		fscc_nge	-	tbl_fscc	/*  28 */
	.dc.w		fscc_ngt	-	tbl_fscc	/*  29 */
	.dc.w		fscc_sneq	-	tbl_fscc	/*  30 */
	.dc.w		fscc_st		-	tbl_fscc	/*  31 */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*                                                                       */
/*  IEEE Nonaware tests                                                  */
/*                                                                       */
/*  For the IEEE nonaware tests, we set the result based on the          */
/*  floating point condition codes. In addition, we check to see         */
/*  if the NAN bit is set, in which case BSUN and AIOP will be set.      */
/*                                                                       */
/*  The cases EQ and NE are shared by the Aware and Nonaware groups      */
/*  and are incapable of setting the BSUN exception bit.                 */
/*                                                                       */
/*  Typically, only one of the two possible branch directions could      */
/*  have the NAN bit set.                                                */
/*                                                                       */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * equal:
 *
 *	Z
 */
fscc_eq:
	fbeq		fscc_eq_yes		/*  equal? */
fscc_eq_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_eq_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * not equal:
 *	_
 *	Z
 */
fscc_neq:
	fbne		fscc_neq_yes		/*  not equal? */
fscc_neq_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_neq_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * greater than:
 *	_______
 *	NANvZvN
 */
fscc_gt:
	fbgt		fscc_gt_yes		/*  greater than? */
fscc_gt_no:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_gt_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * not greater than:
 *
 *	NANvZvN
 */
fscc_ngt:
	fbngt		fscc_ngt_yes		/*  not greater than? */
fscc_ngt_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ngt_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * greater than or equal:
 *	   _____
 *	Zv(NANvN)
 */
fscc_ge:
	fbge		fscc_ge_yes		/*  greater than or equal? */
fscc_ge_no:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_ge_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * not (greater than or equal):
 *	       _
 *	NANv(N^Z)
 */
fscc_nge:
	fbnge		fscc_nge_yes		/*  not (greater than or equal)? */
fscc_nge_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_nge_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * less than:
 *	   _____
 *	N^(NANvZ)
 */
fscc_lt:
	fblt		fscc_lt_yes		/*  less than? */
fscc_lt_no:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_lt_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * not less than:
 *	       _
 *	NANv(ZvN)
 */
fscc_nlt:
	fbnlt		fscc_nlt_yes		/*  not less than? */
fscc_nlt_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_nlt_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * less than or equal:
 *	     ___
 *	Zv(N^NAN)
 */
fscc_le:
	fble		fscc_le_yes		/*  less than or equal? */
fscc_le_no:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_le_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * not (less than or equal):
 *	     ___
 *	NANv(NvZ)
 */
fscc_nle:
	fbnle		fscc_nle_yes		/*  not (less than or equal)? */
fscc_nle_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_nle_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * greater or less than:
 *	_____
 *	NANvZ
 */
fscc_gl:
	fbgl		fscc_gl_yes		/*  greater or less than? */
fscc_gl_no:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_gl_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * not (greater or less than):
 *
 *	NANvZ
 */
fscc_ngl:
	fbngl		fscc_ngl_yes		/*  not (greater or less than)? */
fscc_ngl_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ngl_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * greater, less, or equal:
 *	___
 *	NAN
 */
fscc_gle:
	fbgle		fscc_gle_yes		/*  greater, less, or equal? */
fscc_gle_no:
	clr.b		d0			/*  set false */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_gle_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * not (greater, less, or equal):
 *
 *	NAN
 */
fscc_ngle:
	fbngle		fscc_ngle_yes	/*  not (greater, less, or equal)? */
fscc_ngle_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ngle_yes:
	st		d0			/*  set true */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*                                                                       */
/*  Miscellaneous tests                                                  */
/*                                                                       */
/*  For the IEEE aware tests, we only have to set the result based on    */
/*  the floating point condition codes. The BSUN exception will not be   */
/*  set for any of these tests.                                          */
/*                                                                       */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * false:
 *
 *	False
 */
fscc_f:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */

/*
 * true:
 *
 *	True
 */
fscc_t:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * signalling false:
 *
 *	False
 */
fscc_sf:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * signalling true:
 *
 *	True
 */
fscc_st:
	st		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * signalling equal:
 *
 *	Z
 */
fscc_seq:
	fbseq		fscc_seq_yes		/*  signalling equal? */
fscc_seq_no:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_seq_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/*
 * signalling not equal:
 *	_
 *	Z
 */
fscc_sneq:
	fbsne	fscc_sneq_yes		/*  signalling equal? */
fscc_sneq_no:
	clr.b		d0			/*  set false */
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */
fscc_sneq_yes:
	st		d0			/*  set true */
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		fscc_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	bra.w		fscc_chk_bsun		/*  go finish */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*                                                                       */
/*  IEEE Aware tests                                                     */
/*                                                                       */
/*  For the IEEE aware tests, we only have to set the result based on    */
/*  the floating point condition codes. The BSUN exception will not be   */
/*  set for any of these tests.                                          */
/*                                                                       */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * ordered greater than:
 *	_______
 *	NANvZvN
 */
fscc_ogt:
	fbogt		fscc_ogt_yes		/*  ordered greater than? */
fscc_ogt_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ogt_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * unordered or less or equal:
 *	_______
 *	NANvZvN
 */
fscc_ule:
	fbule		fscc_ule_yes		/*  unordered or less or equal? */
fscc_ule_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ule_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * ordered greater than or equal:
 *	   _____
 *	Zv(NANvN)
 */
fscc_oge:
	fboge		fscc_oge_yes		/*  ordered greater than or equal? */
fscc_oge_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_oge_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * unordered or less than:
 *	       _
 *	NANv(N^Z)
 */
fscc_ult:
	fbult		fscc_ult_yes		/*  unordered or less than? */
fscc_ult_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ult_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * ordered less than:
 *	   _____
 *	N^(NANvZ)
 */
fscc_olt:
	fbolt		fscc_olt_yes		/*  ordered less than? */
fscc_olt_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_olt_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * unordered or greater or equal:
 *
 *	NANvZvN
 */
fscc_uge:
	fbuge		fscc_uge_yes		/*  unordered or greater than? */
fscc_uge_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_uge_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * ordered less than or equal:
 *	     ___
 *	Zv(N^NAN)
 */
fscc_ole:
	fbole		fscc_ole_yes		/*  ordered greater or less than? */
fscc_ole_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ole_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * unordered or greater than:
 *	     ___
 *	NANv(NvZ)
 */
fscc_ugt:
	fbugt		fscc_ugt_yes		/*  unordered or greater than? */
fscc_ugt_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ugt_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * ordered greater or less than:
 *	_____
 *	NANvZ
 */
fscc_ogl:
	fbogl		fscc_ogl_yes		/*  ordered greater or less than? */
fscc_ogl_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ogl_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * unordered or equal:
 *
 *	NANvZ
 */
fscc_ueq:
	fbueq		fscc_ueq_yes		/*  unordered or equal? */
fscc_ueq_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_ueq_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * ordered:
 *	___
 *	NAN
 */
fscc_or:
	fbor		fscc_or_yes		/*  ordered? */
fscc_or_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_or_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/*
 * unordered:
 *
 *	NAN
 */
fscc_un:
	fbun		fscc_un_yes		/*  unordered? */
fscc_un_no:
	clr.b		d0			/*  set false */
	bra.w		fscc_done		/*  go finish */
fscc_un_yes:
	st		d0			/*  set true */
	bra.w		fscc_done		/*  go finish */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * the bsun exception bit was set. now, check to see is BSUN
 * is enabled. if so, don't store result and correct stack frame
 * for a bsun exception.
 */
fscc_chk_bsun:
	btst		#bsun_bit,FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		fscc_bsun

/*
 * the bsun exception bit was not set.
 * the result has been selected.
 * now, check to see if the result is to be stored in the data register
 * file or in memory.
 */
fscc_done:
	move.l		d0,a0			/*  save result for a moment */

	move.b		1+EXC_OPWORD(a6),d1	/*  fetch lo opword */
	move.l		d1,d0			/*  make a copy */
	andi.b		#0x38,d1		/*  extract src mode */

	bne.b		fscc_mem_op		/*  it's a memory operation */

	move.l		d0,d1
	andi.w		#0x7,d1		/*  pass index in d1 */
	move.l		a0,d0			/*  pass result in d0 */
	bsr.l		store_dreg_b		/*  save result in regfile */
	rts

/*
 * the stacked <ea> is correct with the exception of:
 *	-> Dn : <ea> is garbage
 *
 * if the addressing mode is post-increment or pre-decrement,
 * then the address registers have not been updated.
 */
fscc_mem_op:
	cmpi.b		#0x18,d1		/*  is <ea> (An)+ ? */
	beq.b		fscc_mem_inc		/*  yes */
	cmpi.b		#0x20,d1		/*  is <ea> -(An) ? */
	beq.b		fscc_mem_dec		/*  yes */

	move.l		a0,d0			/*  pass result in d0 */
	move.l		EXC_EA(a6),a0		/*  fetch <ea> */
	bsr.l		_dmem_write_byte	/*  write result byte */

	tst.l		d1			/*  did dstore fail? */
	bne.w		fscc_err		/*  yes */

	rts

/*
 * addressing mode is post-increment. write the result byte. if the write
 * fails then don't update the address register. if write passes then
 * call inc_areg() to update the address register.
 */
fscc_mem_inc:
	move.l		a0,d0			/*  pass result in d0 */
	move.l		EXC_EA(a6),a0		/*  fetch <ea> */
	bsr.l		_dmem_write_byte	/*  write result byte */

	tst.l		d1			/*  did dstore fail? */
	bne.w		fscc_err		/*  yes */

	move.b		0x1+EXC_OPWORD(a6),d1	/*  fetch opword */
	andi.w		#0x7,d1		/*  pass index in d1 */
	moveq.l		#0x1,d0		/*  pass amt to inc by */
	bsr.l		inc_areg		/*  increment address register */

	rts

/*  addressing mode is pre-decrement. write the result byte. if the write */
/*  fails then don't update the address register. if the write passes then */
/*  call dec_areg() to update the address register. */
fscc_mem_dec:
	move.l		a0,d0			/*  pass result in d0 */
	move.l		EXC_EA(a6),a0		/*  fetch <ea> */
	bsr.l		_dmem_write_byte	/*  write result byte */

	tst.l		d1			/*  did dstore fail? */
	bne.w		fscc_err		/*  yes */

	move.b		0x1+EXC_OPWORD(a6),d1	/*  fetch opword */
	andi.w		#0x7,d1		/*  pass index in d1 */
	moveq.l		#0x1,d0		/*  pass amt to dec by */
	bsr.l		dec_areg		/*  decrement address register */

	rts

/*  the emulation routine set bsun and BSUN was enabled. have to */
/*  fix stack and jump to the bsun handler. */
/*  let the caller of this routine shift the stack frame up to */
/*  eliminate the effective address field. */
fscc_bsun:
	move.b		#fbsun_flg,SPCOND_FLG(a6)
	rts

/*  the byte write to memory has failed. pass the failing effective address */
/*  and a FSLW to funimp_dacc(). */
fscc_err:
	move.w		#0x00a1,EXC_VOFF(a6)
	bra.l		facc_finish

