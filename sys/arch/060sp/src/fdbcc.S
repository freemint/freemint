/*
 *  fdbcc(): routine to emulate the fdbcc instruction			
 * 									
 *  XDEF **************************************************************** 
 * 	_fdbcc()							
 * 									
 *  XREF **************************************************************** 
 * 	fetch_dreg() - fetch Dn value					
 * 	store_dreg_l() - store updated Dn value				
 * 									
 *  INPUT ***************************************************************	
 * 	d0 = displacement						
 * 									
 *  OUTPUT ************************************************************** 
 * 	none								
 * 									
 *  ALGORITHM ***********************************************************	
 * 	This routine checks which conditional predicate is specified by	
 *  the stacked fdbcc instruction opcode and then branches to a routine	
 *  for that predicate. The corresponding fbcc instruction is then used	
 *  to see whether the condition (specified by the stacked FPSR) is true	
 *  or false.								
 * 	If a BSUN exception should be indicated, the BSUN and ABSUN	
 *  bits are set in the stacked FPSR. If the BSUN exception is enabled,	
 *  the fbsun_flg is set in the SPCOND_FLG location on the stack. If an	
 *  enabled BSUN should not be flagged and the predicate is true, then	
 *  Dn is fetched and decremented by one. If Dn is not equal to -1, add	
 *  the displacement value to the stacked PC so that when an "rte" is	
 *  finally executed, the branch occurs.					
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		_fdbcc
_fdbcc:
	move.l		d0,L_SCR1(a6)		/*  save displacement */

	move.w		EXC_CMDREG(a6),d0	/*  fetch predicate */

	clr.l		d1			/*  clear scratch reg */
	move.b		FPSR_CC(a6),d1	/*  fetch fp ccodes */
	ror.l		#0x8,d1		/*  rotate to top byte */
	fmove.l		d1,fpsr		/*  insert into FPSR */

	move.w		(tbl_fdbcc.b,pc,d0.w*2),d1 /*  load table */
	jmp		(tbl_fdbcc.b,pc,d1.w) /*  jump to fdbcc routine */

tbl_fdbcc:
	.dc.w		fdbcc_f		-	tbl_fdbcc	/*  00 */
	.dc.w		fdbcc_eq	-	tbl_fdbcc	/*  01 */
	.dc.w		fdbcc_ogt	-	tbl_fdbcc	/*  02 */
	.dc.w		fdbcc_oge	-	tbl_fdbcc	/*  03 */
	.dc.w		fdbcc_olt	-	tbl_fdbcc	/*  04 */
	.dc.w		fdbcc_ole	-	tbl_fdbcc	/*  05 */
	.dc.w		fdbcc_ogl	-	tbl_fdbcc	/*  06 */
	.dc.w		fdbcc_or	-	tbl_fdbcc	/*  07 */
	.dc.w		fdbcc_un	-	tbl_fdbcc	/*  08 */
	.dc.w		fdbcc_ueq	-	tbl_fdbcc	/*  09 */
	.dc.w		fdbcc_ugt	-	tbl_fdbcc	/*  10 */
	.dc.w		fdbcc_uge	-	tbl_fdbcc	/*  11 */
	.dc.w		fdbcc_ult	-	tbl_fdbcc	/*  12 */
	.dc.w		fdbcc_ule	-	tbl_fdbcc	/*  13 */
	.dc.w		fdbcc_neq	-	tbl_fdbcc	/*  14 */
	.dc.w		fdbcc_t		-	tbl_fdbcc	/*  15 */
	.dc.w		fdbcc_sf	-	tbl_fdbcc	/*  16 */
	.dc.w		fdbcc_seq	-	tbl_fdbcc	/*  17 */
	.dc.w		fdbcc_gt	-	tbl_fdbcc	/*  18 */
	.dc.w		fdbcc_ge	-	tbl_fdbcc	/*  19 */
	.dc.w		fdbcc_lt	-	tbl_fdbcc	/*  20 */
	.dc.w		fdbcc_le	-	tbl_fdbcc	/*  21 */
	.dc.w		fdbcc_gl	-	tbl_fdbcc	/*  22 */
	.dc.w		fdbcc_gle	-	tbl_fdbcc	/*  23 */
	.dc.w		fdbcc_ngle	-	tbl_fdbcc	/*  24 */
	.dc.w		fdbcc_ngl	-	tbl_fdbcc	/*  25 */
	.dc.w		fdbcc_nle	-	tbl_fdbcc	/*  26 */
	.dc.w		fdbcc_nlt	-	tbl_fdbcc	/*  27 */
	.dc.w		fdbcc_nge	-	tbl_fdbcc	/*  28 */
	.dc.w		fdbcc_ngt	-	tbl_fdbcc	/*  29 */
	.dc.w		fdbcc_sneq	-	tbl_fdbcc	/*  30 */
	.dc.w		fdbcc_st	-	tbl_fdbcc	/*  31 */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 									 */
/*  IEEE Nonaware tests							 */
/* 									 */
/*  For the IEEE nonaware tests, only the false branch changes the	 */
/*  counter. However, the true branch may set bsun so we check to see	 */
/*  if the NAN bit is set, in which case BSUN and AIOP will be set.	 */
/* 									 */
/*  The cases EQ and NE are shared by the Aware and Nonaware groups	 */
/*  and are incapable of setting the BSUN exception bit.			 */
/* 									 */
/*  Typically, only one of the two possible branch directions could	 */
/*  have the NAN bit set.							 */
/*  (This is assuming the mutual exclusiveness of FPSR cc bit groupings	 */
/*   is preserved.)							 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * equal:
 *
 *	Z
 */
fdbcc_eq:
	fbeq		fdbcc_eq_yes		/*  equal? */
fdbcc_eq_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_eq_yes:
	rts

/*
 * not equal:
 *	_
 *	Z
 */
fdbcc_neq:
	fbne		fdbcc_neq_yes		/*  not equal? */
fdbcc_neq_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_neq_yes:
	rts

/*
 * greater than:
 *	_______
 *	NANvZvN
 */
fdbcc_gt:
	fbgt		fdbcc_gt_yes		/*  greater than? */
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fdbcc_false		/*  no;go handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_gt_yes:
	rts					/*  do nothing */

/*
 * not greater than:
 *
 *	NANvZvN
 */
fdbcc_ngt:
	fbngt		fdbcc_ngt_yes		/*  not greater than? */
fdbcc_ngt_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ngt_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		fdbcc_ngt_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_ngt_done:
	rts					/*  no; do nothing */

/*
 * greater than or equal:
 *	   _____
 *	Zv(NANvN)
 */
fdbcc_ge:
	fbge		fdbcc_ge_yes		/*  greater than or equal? */
fdbcc_ge_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fdbcc_false		/*  no;go handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ge_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		fdbcc_ge_yes_done	/*  no;go do nothing */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_ge_yes_done:
	rts					/*  do nothing */

/*
 * not (greater than or equal):
 *	       _
 *	NANv(N^Z)
 */
fdbcc_nge:
	fbnge		fdbcc_nge_yes		/*  not (greater than or equal)? */
fdbcc_nge_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_nge_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		fdbcc_nge_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_nge_done:
	rts					/*  no; do nothing */

/*
 * less than:
 *	   _____
 *	N^(NANvZ)
 */
fdbcc_lt:
	fblt		fdbcc_lt_yes		/*  less than? */
fdbcc_lt_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fdbcc_false		/*  no; go handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_lt_yes:
	rts					/*  do nothing */

/*
 * not less than:
 *	       _
 *	NANv(ZvN)
 */
fdbcc_nlt:
	fbnlt		fdbcc_nlt_yes		/*  not less than? */
fdbcc_nlt_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_nlt_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		fdbcc_nlt_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_nlt_done:
	rts					/*  no; do nothing */

/*
 * less than or equal:
 *	     ___
 *	Zv(N^NAN)
 */
fdbcc_le:
	fble		fdbcc_le_yes		/*  less than or equal? */
fdbcc_le_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fdbcc_false		/*  no; go handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_le_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		fdbcc_le_yes_done	/*  no; go do nothing */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_le_yes_done:
	rts					/*  do nothing */

/*
 * not (less than or equal):
 *	     ___
 *	NANv(NvZ)
 */
fdbcc_nle:
	fbnle		fdbcc_nle_yes		/*  not (less than or equal)? */
fdbcc_nle_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_nle_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fdbcc_nle_done		/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_nle_done:
	rts					/*  no; do nothing */

/*
 * greater or less than:
 *	_____
 *	NANvZ
 */
fdbcc_gl:
	fbgl		fdbcc_gl_yes		/*  greater or less than? */
fdbcc_gl_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		fdbcc_false		/*  no; handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_gl_yes:
	rts					/*  do nothing */

/*
 * not (greater or less than):
 *
 *	NANvZ
 */
fdbcc_ngl:
	fbngl		fdbcc_ngl_yes		/*  not (greater or less than)? */
fdbcc_ngl_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ngl_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		fdbcc_ngl_done		/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_ngl_done:
	rts					/*  no; do nothing */

/*
 * greater, less, or equal:
 *	___
 *	NAN
 */
fdbcc_gle:
	fbgle		fdbcc_gle_yes		/*  greater, less, or equal? */
fdbcc_gle_no:
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_gle_yes:
	rts					/*  do nothing */

/*
 * not (greater, less, or equal):
 *
 *	NAN
 */
fdbcc_ngle:
	fbngle		fdbcc_ngle_yes		/*  not (greater, less, or equal)? */
fdbcc_ngle_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ngle_yes:
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	rts					/*  no; do nothing */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 									 */
/*  Miscellaneous tests							 */
/* 									 */
/*  For the IEEE miscellaneous tests, all but fdbf and fdbt can set bsun.  */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * false:
 *
 *	False
 */
fdbcc_f:					/*  no bsun possible */
	bra.w		fdbcc_false		/*  go handle counter */

/*
 * true:
 *
 *	True
 */
fdbcc_t:					/*  no bsun possible */
	rts					/*  do nothing */

/*
 * signalling false:
 *
 *	False
 */
fdbcc_sf:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set? */
	beq.w		fdbcc_false		/*  no;go handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  go handle counter */

/*
 * signalling true:
 *
 *	True
 */
fdbcc_st:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set? */
	beq.b		fdbcc_st_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_st_done:
	rts

/*
 * signalling equal:
 *
 *	Z
 */
fdbcc_seq:
	fbseq		fdbcc_seq_yes		/*  signalling equal? */
fdbcc_seq_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set? */
	beq.w		fdbcc_false		/*  no;go handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  go handle counter */
fdbcc_seq_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set? */
	beq.b		fdbcc_seq_yes_done	/*  no;go do nothing */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_seq_yes_done:
	rts					/*  yes; do nothing */

/*
 * signalling not equal:
 *	_
 *	Z
 */
fdbcc_sneq:
	fbsne	fdbcc_sneq_yes		/*  signalling not equal? */
fdbcc_sneq_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set? */
	beq.w		fdbcc_false		/*  no;go handle counter */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
	bra.w		fdbcc_false		/*  go handle counter */
fdbcc_sneq_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		fdbcc_sneq_done		/*  no;go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  is BSUN enabled? */
	bne.w		fdbcc_bsun		/*  yes; we have an exception */
fdbcc_sneq_done:
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 									 */
/*  IEEE Aware tests							 */
/* 									 */
/*  For the IEEE aware tests, action is only taken if the result is false. */
/*  Therefore, the opposite branch type is used to jump to the decrement	 */
/*  routine.								 */
/*  The BSUN exception will not be set for any of these tests.		 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * ordered greater than:
 *	_______
 *	NANvZvN
 */
fdbcc_ogt:
	fbogt		fdbcc_ogt_yes		/*  ordered greater than? */
fdbcc_ogt_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ogt_yes:
	rts					/*  yes; do nothing */

/*
 * unordered or less or equal:
 *	_______
 *	NANvZvN
 */
fdbcc_ule:
	fbule		fdbcc_ule_yes		/*  unordered or less or equal? */
fdbcc_ule_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ule_yes:
	rts					/*  yes; do nothing */

/*
 * ordered greater than or equal:
 *	   _____
 *	Zv(NANvN)
 */
fdbcc_oge:
	fboge		fdbcc_oge_yes		/*  ordered greater than or equal? */
fdbcc_oge_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_oge_yes:
	rts					/*  yes; do nothing */

/*
 * unordered or less than:
 *	       _
 *	NANv(N^Z)
 */
fdbcc_ult:
	fbult		fdbcc_ult_yes		/*  unordered or less than? */
fdbcc_ult_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ult_yes:
	rts					/*  yes; do nothing */

/*
 * ordered less than:
 *	   _____
 *	N^(NANvZ)
 */
fdbcc_olt:
	fbolt		fdbcc_olt_yes		/*  ordered less than? */
fdbcc_olt_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_olt_yes:
	rts					/*  yes; do nothing */

/*
 * unordered or greater or equal:
 *
 *	NANvZvN
 */
fdbcc_uge:
	fbuge		fdbcc_uge_yes		/*  unordered or greater than? */
fdbcc_uge_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_uge_yes:
	rts					/*  yes; do nothing */

/*
 * ordered less than or equal:
 *	     ___
 *	Zv(N^NAN)
 */
fdbcc_ole:
	fbole		fdbcc_ole_yes		/*  ordered greater or less than? */
fdbcc_ole_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ole_yes:
	rts					/*  yes; do nothing */

/*
 * unordered or greater than:
 *	     ___
 *	NANv(NvZ)
 */
fdbcc_ugt:
	fbugt		fdbcc_ugt_yes		/*  unordered or greater than? */
fdbcc_ugt_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ugt_yes:
	rts					/*  yes; do nothing */

/*
 * ordered greater or less than:
 *	_____
 *	NANvZ
 */
fdbcc_ogl:
	fbogl		fdbcc_ogl_yes		/*  ordered greater or less than? */
fdbcc_ogl_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ogl_yes:
	rts					/*  yes; do nothing */

/*
 * unordered or equal:
 *
 *	NANvZ
 */
fdbcc_ueq:
	fbueq		fdbcc_ueq_yes		/*  unordered or equal? */
fdbcc_ueq_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_ueq_yes:
	rts					/*  yes; do nothing */

/*
 * ordered:
 *	___
 *	NAN
 */
fdbcc_or:
	fbor		fdbcc_or_yes		/*  ordered? */
fdbcc_or_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_or_yes:
	rts					/*  yes; do nothing */

/*
 * unordered:
 *
 *	NAN
 */
fdbcc_un:
	fbun		fdbcc_un_yes		/*  unordered? */
fdbcc_un_no:
	bra.w		fdbcc_false		/*  no; go handle counter */
fdbcc_un_yes:
	rts					/*  yes; do nothing */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * the bsun exception bit was not set.
 *
 * (1) subtract 1 from the count register
 * (2) if (cr == -1) then
 *	pc = pc of next instruction
 *     else
 *	pc += sign_ext(16-bit displacement)
 */
fdbcc_false:
	move.b		1+EXC_OPWORD(a6), d1	/*  fetch lo opword */
	andi.w		#0x7, d1		/*  extract count register */

	bsr.l		fetch_dreg		/*  fetch count value */
/*  make sure that d0 isn't corrupted between calls... */

	subq.w		#0x1, d0		/*  Dn - 1 -> Dn */

	bsr.l		store_dreg_l		/*  store new count value */

	cmpi.w		#-0x1,d0		/*  is (Dn == -1)? */
	bne.b		fdbcc_false_cont	/*  no; */
	rts

fdbcc_false_cont:
	move.l		L_SCR1(a6),d0		/*  fetch displacement */
	add.l		USER_FPIAR(a6),d0	/*  add instruction PC */
	addq.l		#0x4,d0		/*  add instruction length */
	move.l		d0,EXC_PC(a6)		/*  set new PC */
	rts

/*  the emulation routine set bsun and BSUN was enabled. have to */
/*  fix stack and jump to the bsun handler. */
/*  let the caller of this routine shift the stack frame up to */
/*  eliminate the effective address field. */
fdbcc_bsun:
	move.b		#fbsun_flg,SPCOND_FLG(a6)
	rts

