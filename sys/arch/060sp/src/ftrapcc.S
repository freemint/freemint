/*
 *  ftrapcc(): routine to emulate the ftrapcc instruction			
 * 									
 *  XDEF ****************************************************************	
 * 	_ftrapcc()							
 * 									
 *  XREF ****************************************************************	
 * 	none								
 * 									
 *  INPUT *************************************************************** 
 * 	none								
 * 									
 *  OUTPUT ************************************************************** 
 * 	none								
 * 									
 *  ALGORITHM *********************************************************** 
 * 	This routine checks which conditional predicate is specified by	
 *  the stacked ftrapcc instruction opcode and then branches to a routine	
 *  for that predicate. The corresponding fbcc instruction is then used	
 *  to see whether the condition (specified by the stacked FPSR) is true	
 *  or false.								
 * 	If a BSUN exception should be indicated, the BSUN and ABSUN	
 *  bits are set in the stacked FPSR. If the BSUN exception is enabled,	
 *  the fbsun_flg is set in the SPCOND_FLG location on the stack. If an	
 *  enabled BSUN should not be flagged and the predicate is true, then	
 *  the ftrapcc_flg is set in the SPCOND_FLG location. These special	
 *  flags indicate to the calling routine to emulate the exceptional	
 *  condition.								
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		_ftrapcc
_ftrapcc:
	move.w		EXC_CMDREG(a6),d0	/*  fetch predicate */

	clr.l		d1			/*  clear scratch reg */
	move.b		FPSR_CC(a6),d1	/*  fetch fp ccodes */
	ror.l		#0x8,d1		/*  rotate to top byte */
	fmove.l		d1,fpsr		/*  insert into FPSR */

	move.w		(tbl_ftrapcc.b,pc,d0.w*2), d1 /*  load table */
	jmp		(tbl_ftrapcc.b,pc,d1.w) /*  jump to ftrapcc routine */

tbl_ftrapcc:
	.dc.w		ftrapcc_f	-	tbl_ftrapcc	/*  00 */
	.dc.w		ftrapcc_eq	-	tbl_ftrapcc	/*  01 */
	.dc.w		ftrapcc_ogt	-	tbl_ftrapcc	/*  02 */
	.dc.w		ftrapcc_oge	-	tbl_ftrapcc	/*  03 */
	.dc.w		ftrapcc_olt	-	tbl_ftrapcc	/*  04 */
	.dc.w		ftrapcc_ole	-	tbl_ftrapcc	/*  05 */
	.dc.w		ftrapcc_ogl	-	tbl_ftrapcc	/*  06 */
	.dc.w		ftrapcc_or	-	tbl_ftrapcc	/*  07 */
	.dc.w		ftrapcc_un	-	tbl_ftrapcc	/*  08 */
	.dc.w		ftrapcc_ueq	-	tbl_ftrapcc	/*  09 */
	.dc.w		ftrapcc_ugt	-	tbl_ftrapcc	/*  10 */
	.dc.w		ftrapcc_uge	-	tbl_ftrapcc	/*  11 */
	.dc.w		ftrapcc_ult	-	tbl_ftrapcc	/*  12 */
	.dc.w		ftrapcc_ule	-	tbl_ftrapcc	/*  13 */
	.dc.w		ftrapcc_neq	-	tbl_ftrapcc	/*  14 */
	.dc.w		ftrapcc_t	-	tbl_ftrapcc	/*  15 */
	.dc.w		ftrapcc_sf	-	tbl_ftrapcc	/*  16 */
	.dc.w		ftrapcc_seq	-	tbl_ftrapcc	/*  17 */
	.dc.w		ftrapcc_gt	-	tbl_ftrapcc	/*  18 */
	.dc.w		ftrapcc_ge	-	tbl_ftrapcc	/*  19 */
	.dc.w		ftrapcc_lt	-	tbl_ftrapcc	/*  20 */
	.dc.w		ftrapcc_le	-	tbl_ftrapcc	/*  21 */
	.dc.w		ftrapcc_gl	-	tbl_ftrapcc	/*  22 */
	.dc.w		ftrapcc_gle	-	tbl_ftrapcc	/*  23 */
	.dc.w		ftrapcc_ngle	-	tbl_ftrapcc	/*  24 */
	.dc.w		ftrapcc_ngl	-	tbl_ftrapcc	/*  25 */
	.dc.w		ftrapcc_nle	-	tbl_ftrapcc	/*  26 */
	.dc.w		ftrapcc_nlt	-	tbl_ftrapcc	/*  27 */
	.dc.w		ftrapcc_nge	-	tbl_ftrapcc	/*  28 */
	.dc.w		ftrapcc_ngt	-	tbl_ftrapcc	/*  29 */
	.dc.w		ftrapcc_sneq	-	tbl_ftrapcc	/*  30 */
	.dc.w		ftrapcc_st	-	tbl_ftrapcc	/*  31 */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 									 */
/*  IEEE Nonaware tests							 */
/* 									 */
/*  For the IEEE nonaware tests, we set the result based on the		 */
/*  floating point condition codes. In addition, we check to see		 */
/*  if the NAN bit is set, in which case BSUN and AIOP will be set.	 */
/* 									 */
/*  The cases EQ and NE are shared by the Aware and Nonaware groups	 */
/*  and are incapable of setting the BSUN exception bit.			 */
/* 									 */
/*  Typically, only one of the two possible branch directions could	 */
/*  have the NAN bit set.							 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * equal:
 *
 *	Z
 */
ftrapcc_eq:
	fbeq		ftrapcc_trap		/*  equal? */
ftrapcc_eq_no:
	rts					/*  do nothing */

/*
 * not equal:
 *	_
 *	Z
 */
ftrapcc_neq:
	fbne		ftrapcc_trap		/*  not equal? */
ftrapcc_neq_no:
	rts					/*  do nothing */

/*
 * greater than:
 *	_______
 *	NANvZvN
 */
ftrapcc_gt:
	fbgt		ftrapcc_trap		/*  greater than? */
ftrapcc_gt_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		ftrapcc_gt_done		/*  no */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_gt_done:
	rts					/*  no; do nothing */

/*
 * not greater than:
 *
 *	NANvZvN
 */
ftrapcc_ngt:
	fbngt		ftrapcc_ngt_yes		/*  not greater than? */
ftrapcc_ngt_no:
	rts					/*  do nothing */
ftrapcc_ngt_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * greater than or equal:
 *	   _____
 *	Zv(NANvN)
 */
ftrapcc_ge:
	fbge		ftrapcc_ge_yes		/*  greater than or equal? */
ftrapcc_ge_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		ftrapcc_ge_done		/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_ge_done:
	rts					/*  no; do nothing */
ftrapcc_ge_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * not (greater than or equal):
 *	       _
 *	NANv(N^Z)
 */
ftrapcc_nge:
	fbnge		ftrapcc_nge_yes		/*  not (greater than or equal)? */
ftrapcc_nge_no:
	rts					/*  do nothing */
ftrapcc_nge_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * less than:
 *	   _____
 *	N^(NANvZ)
 */
ftrapcc_lt:
	fblt		ftrapcc_trap		/*  less than? */
ftrapcc_lt_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		ftrapcc_lt_done		/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_lt_done:
	rts					/*  no; do nothing */

/*
 * not less than:
 *	       _
 *	NANv(ZvN)
 */
ftrapcc_nlt:
	fbnlt		ftrapcc_nlt_yes		/*  not less than? */
ftrapcc_nlt_no:
	rts					/*  do nothing */
ftrapcc_nlt_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * less than or equal:
 *	     ___
 *	Zv(N^NAN)
 */
ftrapcc_le:
	fble		ftrapcc_le_yes		/*  less than or equal? */
ftrapcc_le_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		ftrapcc_le_done		/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_le_done:
	rts					/*  no; do nothing */
ftrapcc_le_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * not (less than or equal):
 *	     ___
 *	NANv(NvZ)
 */
ftrapcc_nle:
	fbnle		ftrapcc_nle_yes		/*  not (less than or equal)? */
ftrapcc_nle_no:
	rts					/*  do nothing */
ftrapcc_nle_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * greater or less than:
 *	_____
 *	NANvZ
 */
ftrapcc_gl:
	fbgl		ftrapcc_trap		/*  greater or less than? */
ftrapcc_gl_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.b		ftrapcc_gl_done		/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_gl_done:
	rts					/*  no; do nothing */

/*
 * not (greater or less than):
 *
 *	NANvZ
 */
ftrapcc_ngl:
	fbngl		ftrapcc_ngl_yes		/*  not (greater or less than)? */
ftrapcc_ngl_no:
	rts					/*  do nothing */
ftrapcc_ngl_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  is NAN set in cc? */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * greater, less, or equal:
 *	___
 *	NAN
 */
ftrapcc_gle:
	fbgle		ftrapcc_trap		/*  greater, less, or equal? */
ftrapcc_gle_no:
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	rts					/*  no; do nothing */

/*
 * not (greater, less, or equal):
 *
 *	NAN
 */
ftrapcc_ngle:
	fbngle		ftrapcc_ngle_yes	/*  not (greater, less, or equal)? */
ftrapcc_ngle_no:
	rts					/*  do nothing */
ftrapcc_ngle_yes:
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 									 */
/*  Miscellaneous tests							 */
/* 									 */
/*  For the IEEE aware tests, we only have to set the result based on the	 */
/*  floating point condition codes. The BSUN exception will not be	 */
/*  set for any of these tests.						 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * false:
 *
 *	False
 */
ftrapcc_f:
	rts					/*  do nothing */

/*
 * true:
 *
 *	True
 */
ftrapcc_t:
	bra.w		ftrapcc_trap		/*  go take trap */

/*
 * signalling false:
 *
 *	False
 */
ftrapcc_sf:
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.b		ftrapcc_sf_done		/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_sf_done:
	rts					/*  no; do nothing */

/*
 * signalling true:
 *
 *	True
 */
ftrapcc_st:
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * signalling equal:
 *
 *	Z
 */
ftrapcc_seq:
	fbseq		ftrapcc_seq_yes		/*  signalling equal? */
ftrapcc_seq_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		ftrapcc_seq_done	/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_seq_done:
	rts					/*  no; do nothing */
ftrapcc_seq_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/*
 * signalling not equal:
 *	_
 *	Z
 */
ftrapcc_sneq:
	fbsne	ftrapcc_sneq_yes	/*  signalling equal? */
ftrapcc_sneq_no:
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		ftrapcc_sneq_no_done	/*  no; go finish */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
ftrapcc_sneq_no_done:
	rts					/*  do nothing */
ftrapcc_sneq_yes:
	btst		#nan_bit, FPSR_CC(a6)	/*  set BSUN exc bit */
	beq.w		ftrapcc_trap		/*  no; go take trap */
	ori.l		#bsun_mask+aiop_mask, USER_FPSR(a6) /*  set BSUN exc bit */
	btst		#bsun_bit, FPCR_ENABLE(a6) /*  was BSUN set? */
	bne.w		ftrapcc_bsun		/*  yes */
	bra.w		ftrapcc_trap		/*  no; go take trap */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 									 */
/*  IEEE Aware tests							 */
/* 									 */
/*  For the IEEE aware tests, we only have to set the result based on the	 */
/*  floating point condition codes. The BSUN exception will not be	 */
/*  set for any of these tests.						 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * ordered greater than:
 *	_______
 *	NANvZvN
 */
ftrapcc_ogt:
	fbogt		ftrapcc_trap		/*  ordered greater than? */
ftrapcc_ogt_no:
	rts					/*  do nothing */

/*
 * unordered or less or equal:
 *	_______
 *	NANvZvN
 */
ftrapcc_ule:
	fbule		ftrapcc_trap		/*  unordered or less or equal? */
ftrapcc_ule_no:
	rts					/*  do nothing */

/*
 * ordered greater than or equal:
 *	   _____
 *	Zv(NANvN)
 */
ftrapcc_oge:
	fboge		ftrapcc_trap		/*  ordered greater than or equal? */
ftrapcc_oge_no:
	rts					/*  do nothing */

/*
 * unordered or less than:
 *	       _
 *	NANv(N^Z)
 */
ftrapcc_ult:
	fbult		ftrapcc_trap		/*  unordered or less than? */
ftrapcc_ult_no:
	rts					/*  do nothing */

/*
 * ordered less than:
 *	   _____
 *	N^(NANvZ)
 */
ftrapcc_olt:
	fbolt		ftrapcc_trap		/*  ordered less than? */
ftrapcc_olt_no:
	rts					/*  do nothing */

/*
 * unordered or greater or equal:
 *
 *	NANvZvN
 */
ftrapcc_uge:
	fbuge		ftrapcc_trap		/*  unordered or greater than? */
ftrapcc_uge_no:
	rts					/*  do nothing */

/*
 * ordered less than or equal:
 *	     ___
 *	Zv(N^NAN)
 */
ftrapcc_ole:
	fbole		ftrapcc_trap		/*  ordered greater or less than? */
ftrapcc_ole_no:
	rts					/*  do nothing */

/*
 * unordered or greater than:
 *	     ___
 *	NANv(NvZ)
 */
ftrapcc_ugt:
	fbugt		ftrapcc_trap		/*  unordered or greater than? */
ftrapcc_ugt_no:
	rts					/*  do nothing */

/*
 * ordered greater or less than:
 *	_____
 *	NANvZ
 */
ftrapcc_ogl:
	fbogl		ftrapcc_trap		/*  ordered greater or less than? */
ftrapcc_ogl_no:
	rts					/*  do nothing */

/*
 * unordered or equal:
 *
 *	NANvZ
 */
ftrapcc_ueq:
	fbueq		ftrapcc_trap		/*  unordered or equal? */
ftrapcc_ueq_no:
	rts					/*  do nothing */

/*
 * ordered:
 *	___
 *	NAN
 */
ftrapcc_or:
	fbor		ftrapcc_trap		/*  ordered? */
ftrapcc_or_no:
	rts					/*  do nothing */

/*
 * unordered:
 *
 *	NAN
 */
ftrapcc_un:
	fbun		ftrapcc_trap		/*  unordered? */
ftrapcc_un_no:
	rts					/*  do nothing */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  the bsun exception bit was not set. */
/*  we will need to jump to the ftrapcc vector. the stack frame */
/*  is the same size as that of the fp unimp instruction. the */
/*  only difference is that the <ea> field should hold the PC */
/*  of the ftrapcc instruction and the vector offset field */
/*  should denote the ftrapcc trap. */
ftrapcc_trap:
	move.b		#ftrapcc_flg,SPCOND_FLG(a6)
	rts

/*  the emulation routine set bsun and BSUN was enabled. have to */
/*  fix stack and jump to the bsun handler. */
/*  let the caller of this routine shift the stack frame up to */
/*  eliminate the effective address field. */
ftrapcc_bsun:
	move.b		#fbsun_flg,SPCOND_FLG(a6)
	rts

