/*
 *  XDEF ****************************************************************	
 * 	_load_fop(): load operand for unimplemented FP exception	
 * 									
 *  XREF ****************************************************************	
 * 	set_tag_x() - determine ext prec optype tag			
 * 	set_tag_s() - determine sgl prec optype tag			
 * 	set_tag_d() - determine dbl prec optype tag			
 * 	unnorm_fix() - convert normalized number to denorm or zero	
 * 	norm() - normalize a denormalized number			
 * 	get_packed() - fetch a packed operand from memory		
 * 	_dcalc_ea() - calculate <ea>, fixing An in process		
 * 									
 * 	_imem_read_{word,long}() - read from instruction memory		
 * 	_dmem_read() - read from data memory				
 * 	_dmem_read_{byte,word,long}() - read from data memory		
 * 									
 * 	facc_in_{b,w,l,d,x}() - mem read failed *  special exit point	
 * 									
 *  INPUT ***************************************************************	
 * 	None								
 * 									
 *  OUTPUT **************************************************************	
 * 	If memory access doesn't fail:					
 * 		FP_SRC(a6) = source operand in extended precision	
 * 		FP_DST(a6) = destination operand in extended precision	
 * 									
 *  ALGORITHM ***********************************************************	
 * 	This is called from the Unimplemented FP exception handler in	
 *  order to load the source and maybe destination operand into		
 *  FP_SRC(a6) and FP_DST(a6). If the instruction was opclass zero, load	
 *  the source and destination from the FP register file. Set the optype	
 *  tags for both if dyadic, one for monadic. If a number is an UNNORM,	
 *  convert it to a DENORM or a ZERO.					
 * 	If the instruction is opclass two (memory->reg), then fetch	
 *  the destination from the register file and the source operand from	
 *  memory. Tag and fix both as above w/ opclass zero instructions.	
 * 	If the source operand is byte,word,long, or single, it may be	
 *  in the data register file. If it's actually out in memory, use one of	
 *  the mem_read() routines to fetch it. If the mem_read() access returns	
 *  a failing value, exit through the special facc_in() routine which	
 *  will create an access error exception frame from the current exception 
 *  frame.								
 * 	Immediate data and regular data accesses are separated because	
 *  if an immediate data access fails, the resulting fault status		
 *  longword stacked for the access error exception must have the		
 *  instruction bit set.							
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		_load_fop
_load_fop:

/*
 *  15     13 12 10  9 7  6       0
 * /        \ /   \ /  \ /         \
 * ---------------------------------
 * | opclass | RX  | RY | EXTENSION |  (2nd word of general FP instruction)
 * ---------------------------------
 */

/* 	bfextu		EXC_CMDREG(a6){0:3}, d0 ; extract opclass */
/* 	cmpi.b		#0x2,d0		; which class is it? ('000,'010,'011) */
/* 	beq.w		op010			; handle <ea> -> fpn */
/* 	bgt.w		op011			; handle fpn -> <ea> */

/*  we're not using op011 for now... */
	btst		#0x6,EXC_CMDREG(a6)
	bne.b		op010

/* ;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  OPCLASS '000: reg -> reg  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;; */
op000:
	move.b		1+EXC_CMDREG(a6),d0	/*  fetch extension word lo */
	btst		#0x5,d0		/*  testing extension bits */
	beq.b		op000_src		/*  (bit 5 == 0) => monadic */
	btst		#0x4,d0		/*  (bit 5 == 1) */
	beq.b		op000_dst		/*  (bit 4 == 0) => dyadic */
	andi.w		#0x007f,d0		/*  extract extension bits {6:0} */
	cmpi.w		#0x0038,d0		/*  is it an fcmp (dyadic) ? */
	bne.b		op000_src		/*  it's an fcmp */

op000_dst:
	bfextu		EXC_CMDREG(a6){6:3}, d0 /*  extract dst field */
	bsr.l		load_fpn2		/*  fetch dst fpreg into FP_DST */

	bsr.l		set_tag_x		/*  get dst optype tag */

	cmpi.b		#UNNORM,d0		/*  is dst fpreg an UNNORM? */
	beq.b		op000_dst_unnorm	/*  yes */
op000_dst_cont:
	move.b		d0, DTAG(a6)		/*  store the dst optype tag */

op000_src:
	bfextu		EXC_CMDREG(a6){3:3}, d0 /*  extract src field */
	bsr.l		load_fpn1		/*  fetch src fpreg into FP_SRC */

	bsr.l		set_tag_x		/*  get src optype tag */

	cmpi.b		#UNNORM,d0		/*  is src fpreg an UNNORM? */
	beq.b		op000_src_unnorm	/*  yes */
op000_src_cont:
	move.b		d0, STAG(a6)		/*  store the src optype tag */
	rts

op000_dst_unnorm:
	bsr.l		unnorm_fix		/*  fix the dst UNNORM */
	bra.b		op000_dst_cont
op000_src_unnorm:
	bsr.l		unnorm_fix		/*  fix the src UNNORM */
	bra.b		op000_src_cont

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  OPCLASS '010: <ea> -> reg  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;; */
op010:
	move.w		EXC_CMDREG(a6),d0	/*  fetch extension word */
	btst		#0x5,d0		/*  testing extension bits */
	beq.b		op010_src		/*  (bit 5 == 0) => monadic */
	btst		#0x4,d0		/*  (bit 5 == 1) */
	beq.b		op010_dst		/*  (bit 4 == 0) => dyadic */
	andi.w		#0x007f,d0		/*  extract extension bits {6:0} */
	cmpi.w		#0x0038,d0		/*  is it an fcmp (dyadic) ? */
	bne.b		op010_src		/*  it's an fcmp */

op010_dst:
	bfextu		EXC_CMDREG(a6){6:3}, d0 /*  extract dst field */
	bsr.l		load_fpn2		/*  fetch dst fpreg ptr */

	bsr.l		set_tag_x		/*  get dst type tag */

	cmpi.b		#UNNORM,d0		/*  is dst fpreg an UNNORM? */
	beq.b		op010_dst_unnorm	/*  yes */
op010_dst_cont:
	move.b		d0, DTAG(a6)		/*  store the dst optype tag */

op010_src:
	bfextu		EXC_CMDREG(a6){3:3}, d0 /*  extract src type field */

	bfextu		EXC_OPWORD(a6){10:3}, d1 /*  extract <ea> mode field */
	bne.w		fetch_from_mem		/*  src op is in memory */

op010_dreg:
	clr.b		STAG(a6)		/*  either NORM or ZERO */
	bfextu		EXC_OPWORD(a6){13:3},d1 /*  extract src reg field */

	move.w		(tbl_op010_dreg.b,pc,d0.w*2), d0 /*  jmp based on optype */
	jmp		(tbl_op010_dreg.b,pc,d0.w*1) /*  fetch src from dreg */

op010_dst_unnorm:
	bsr.l		unnorm_fix		/*  fix the dst UNNORM */
	bra.b		op010_dst_cont

	/* swbeg		#0x8 */
	.dc.w 0x4afc,8
tbl_op010_dreg:
	.dc.w		opd_long-tbl_op010_dreg
	.dc.w		opd_sgl-tbl_op010_dreg
	.dc.w		tbl_op010_dreg-tbl_op010_dreg
	.dc.w		tbl_op010_dreg-tbl_op010_dreg
	.dc.w		opd_word-tbl_op010_dreg
	.dc.w		tbl_op010_dreg-tbl_op010_dreg
	.dc.w		opd_byte-tbl_op010_dreg
	.dc.w		tbl_op010_dreg-tbl_op010_dreg

/*
 * LONG: can be either NORM or ZERO...
 */
opd_long:
	bsr.l		fetch_dreg		/*  fetch long in d0 */
	fmove.l		d0, fp0		/*  load a long */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */
	fbeq		opd_long_zero		/*  long is a ZERO */
	rts
opd_long_zero:
	move.b		#ZERO, STAG(a6)	/*  set ZERO optype flag */
	rts

/*
 * WORD: can be either NORM or ZERO...
 */
opd_word:
	bsr.l		fetch_dreg		/*  fetch word in d0 */
	fmove.w		d0, fp0		/*  load a word */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */
	fbeq		opd_word_zero		/*  WORD is a ZERO */
	rts
opd_word_zero:
	move.b		#ZERO, STAG(a6)	/*  set ZERO optype flag */
	rts

/*
 * BYTE: can be either NORM or ZERO...
 */
opd_byte:
	bsr.l		fetch_dreg		/*  fetch word in d0 */
	fmove.b		d0, fp0		/*  load a byte */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */
	fbeq		opd_byte_zero		/*  byte is a ZERO */
	rts
opd_byte_zero:
	move.b		#ZERO, STAG(a6)	/*  set ZERO optype flag */
	rts

/*
 * SGL: can be either NORM, DENORM, ZERO, INF, QNAN or SNAN but not UNNORM
 *
 * separate SNANs and DENORMs so they can be loaded w/ special care.
 * all others can simply be moved "in" using fmove.
 */
opd_sgl:
	bsr.l		fetch_dreg		/*  fetch sgl in d0 */
	move.l		d0,L_SCR1(a6)

	lea		L_SCR1(a6), a0	/*  pass: ptr to the sgl */
	bsr.l		set_tag_s		/*  determine sgl type */
	move.b		d0, STAG(a6)		/*  save the src tag */

	cmpi.b		#SNAN,d0		/*  is it an SNAN? */
	beq.w		get_sgl_snan		/*  yes */

	cmpi.b		#DENORM,d0		/*  is it a DENORM? */
	beq.w		get_sgl_denorm		/*  yes */

	fmove.s		(a0), fp0		/*  no, so can load it regular */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 * fetch_from_mem():
 * - src is out in memory. must:
 *	(1) calc ea - must read AFTER you know the src type since
 *		      if the ea is -() or ()+, need to know ; of bytes.
 *	(2) read it in from either user or supervisor space
 *	(3) if (b || w || l) then simply read in
 *	    if (s || d || x) then check for SNAN,UNNORM,DENORM
 *	    if (packed) then punt for now
 * INPUT:
 *	d0 : src type field
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 */
fetch_from_mem:
	clr.b		STAG(a6)		/*  either NORM or ZERO */

	move.w		(tbl_fp_type.b,pc,d0.w*2), d0 /*  index by src type field */
	jmp		(tbl_fp_type.b,pc,d0.w*1)

	/* swbeg		#0x8 */
	.dc.w 0x4afc,8
tbl_fp_type:
	.dc.w		load_long-tbl_fp_type
	.dc.w		load_sgl-tbl_fp_type
	.dc.w		load_ext-tbl_fp_type
	.dc.w		load_packed-tbl_fp_type
	.dc.w		load_word-tbl_fp_type
	.dc.w		load_dbl-tbl_fp_type
	.dc.w		load_byte-tbl_fp_type
	.dc.w		tbl_fp_type-tbl_fp_type

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  load a LONG into fp0:		 */
/* 	-number can't fault		 */
/* 	(1) calc ea			 */
/* 	(2) read 4 bytes into L_SCR1	 */
/* 	(3) fmove.l into fp0		 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
load_long:
	moveq.l		#0x4, d0		/*  pass: 4 (bytes) */
	bsr.l		_dcalc_ea		/*  calc <ea>; <ea> in a0 */

	cmpi.b		#immed_flg,SPCOND_FLG(a6)
	beq.b		load_long_immed

	bsr.l		_dmem_read_long		/*  fetch src operand from memory */

	tst.l		d1			/*  did dfetch fail? */
	bne.l		facc_in_l		/*  yes */

load_long_cont:
	fmove.l		d0, fp0		/*  read into fp0;convert to xprec */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */

	fbeq		load_long_zero		/*  src op is a ZERO */
	rts
load_long_zero:
	move.b		#ZERO, STAG(a6)	/*  set optype tag to ZERO */
	rts

load_long_immed:
	bsr.l		_imem_read_long		/*  fetch src operand immed data */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		funimp_iacc		/*  yes */
	bra.b		load_long_cont

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  load a WORD into fp0:		 */
/* 	-number can't fault		 */
/* 	(1) calc ea			 */
/* 	(2) read 2 bytes into L_SCR1	 */
/* 	(3) fmove.w into fp0		 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
load_word:
	moveq.l		#0x2, d0		/*  pass: 2 (bytes) */
	bsr.l		_dcalc_ea		/*  calc <ea>; <ea> in a0 */

	cmpi.b		#immed_flg,SPCOND_FLG(a6)
	beq.b		load_word_immed

	bsr.l		_dmem_read_word		/*  fetch src operand from memory */

	tst.l		d1			/*  did dfetch fail? */
	bne.l		facc_in_w		/*  yes */

load_word_cont:
	fmove.w		d0, fp0		/*  read into fp0;convert to xprec */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */

	fbeq		load_word_zero		/*  src op is a ZERO */
	rts
load_word_zero:
	move.b		#ZERO, STAG(a6)	/*  set optype tag to ZERO */
	rts

load_word_immed:
	bsr.l		_imem_read_word		/*  fetch src operand immed data */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		funimp_iacc		/*  yes */
	bra.b		load_word_cont

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  load a BYTE into fp0:		 */
/* 	-number can't fault		 */
/* 	(1) calc ea			 */
/* 	(2) read 1 byte into L_SCR1	 */
/* 	(3) fmove.b into fp0		 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
load_byte:
	moveq.l		#0x1, d0		/*  pass: 1 (byte) */
	bsr.l		_dcalc_ea		/*  calc <ea>; <ea> in a0 */

	cmpi.b		#immed_flg,SPCOND_FLG(a6)
	beq.b		load_byte_immed

	bsr.l		_dmem_read_byte		/*  fetch src operand from memory */

	tst.l		d1			/*  did dfetch fail? */
	bne.l		facc_in_b		/*  yes */

load_byte_cont:
	fmove.b		d0, fp0		/*  read into fp0;convert to xprec */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */

	fbeq		load_byte_zero		/*  src op is a ZERO */
	rts
load_byte_zero:
	move.b		#ZERO, STAG(a6)	/*  set optype tag to ZERO */
	rts

load_byte_immed:
	bsr.l		_imem_read_word		/*  fetch src operand immed data */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		funimp_iacc		/*  yes */
	bra.b		load_byte_cont

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  load a SGL into fp0:			 */
/* 	-number can't fault		 */
/* 	(1) calc ea			 */
/* 	(2) read 4 bytes into L_SCR1	 */
/* 	(3) fmove.s into fp0		 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
load_sgl:
	moveq.l		#0x4, d0		/*  pass: 4 (bytes) */
	bsr.l		_dcalc_ea		/*  calc <ea>; <ea> in a0 */

	cmpi.b		#immed_flg,SPCOND_FLG(a6)
	beq.b		load_sgl_immed

	bsr.l		_dmem_read_long		/*  fetch src operand from memory */
	move.l		d0, L_SCR1(a6)	/*  store src op on stack */

	tst.l		d1			/*  did dfetch fail? */
	bne.l		facc_in_l		/*  yes */

load_sgl_cont:
	lea		L_SCR1(a6), a0	/*  pass: ptr to sgl src op */
	bsr.l		set_tag_s		/*  determine src type tag */
	move.b		d0, STAG(a6)		/*  save src optype tag on stack */

	cmpi.b		#DENORM,d0		/*  is it a sgl DENORM? */
	beq.w		get_sgl_denorm		/*  yes */

	cmpi.b		#SNAN,d0		/*  is it a sgl SNAN? */
	beq.w		get_sgl_snan		/*  yes */

	fmove.s		L_SCR1(a6), fp0	/*  read into fp0;convert to xprec */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */
	rts

load_sgl_immed:
	bsr.l		_imem_read_long		/*  fetch src operand immed data */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		funimp_iacc		/*  yes */
	move.l		d0, L_SCR1(a6)	/*  store src op on stack */
	bra.b		load_sgl_cont

/*  must convert sgl denorm format to an Xprec denorm fmt suitable for */
/*  normalization... */
/*  a0 : points to sgl denorm */
get_sgl_denorm:
	clr.w		FP_SRC_EX(a6)
	bfextu		(a0){9:23}, d0	/*  fetch sgl hi(_mantissa) */
	lsl.l		#0x8, d0
	move.l		d0, FP_SRC_HI(a6)	/*  set ext hi(_mantissa) */
	clr.l		FP_SRC_LO(a6)		/*  set ext lo(_mantissa) */

	clr.w		FP_SRC_EX(a6)
	btst		#0x7, (a0)		/*  is sgn bit set? */
	beq.b		sgl_dnrm_norm
	bset		#0x7, FP_SRC_EX(a6)	/*  set sgn of xprec value */

sgl_dnrm_norm:
	lea		FP_SRC(a6), a0
	bsr.l		norm			/*  normalize number */
	move.w		#0x3f81, d1		/*  xprec exp = 0x3f81 */
	sub.w		d0, d1		/*  exp = 0x3f81 - shft amt. */
	or.w		d1, FP_SRC_EX(a6)	/*  {sgn,exp} */

	move.b		#NORM, STAG(a6)	/*  fix src type tag */
	rts

/*  convert sgl to ext SNAN */
/*  a0 : points to sgl SNAN */
get_sgl_snan:
	move.w		#0x7fff, FP_SRC_EX(a6) /*  set exp of SNAN */
	bfextu		(a0){9:23}, d0
	lsl.l		#0x8, d0		/*  extract and insert hi(man) */
	move.l		d0, FP_SRC_HI(a6)
	clr.l		FP_SRC_LO(a6)

	btst		#0x7, (a0)		/*  see if sign of SNAN is set */
	beq.b		no_sgl_snan_sgn
	bset		#0x7, FP_SRC_EX(a6)
no_sgl_snan_sgn:
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  load a DBL into fp0:			 */
/* 	-number can't fault		 */
/* 	(1) calc ea			 */
/* 	(2) read 8 bytes into L_SCR(1,2) */
/* 	(3) fmove.d into fp0		 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
load_dbl:
	moveq.l		#0x8, d0		/*  pass: 8 (bytes) */
	bsr.l		_dcalc_ea		/*  calc <ea>; <ea> in a0 */

	cmpi.b		#immed_flg,SPCOND_FLG(a6)
	beq.b		load_dbl_immed

	lea		L_SCR1(a6), a1	/*  pass: ptr to input dbl tmp space */
	moveq.l		#0x8, d0		/*  pass: ; bytes to read */
	bsr.l		_dmem_read		/*  fetch src operand from memory */

	tst.l		d1			/*  did dfetch fail? */
	bne.l		facc_in_d		/*  yes */

load_dbl_cont:
	lea		L_SCR1(a6), a0	/*  pass: ptr to input dbl */
	bsr.l		set_tag_d		/*  determine src type tag */
	move.b		d0, STAG(a6)		/*  set src optype tag */

	cmpi.b		#DENORM,d0		/*  is it a dbl DENORM? */
	beq.w		get_dbl_denorm		/*  yes */

	cmpi.b		#SNAN,d0		/*  is it a dbl SNAN? */
	beq.w		get_dbl_snan		/*  yes */

	fmove.d		L_SCR1(a6), fp0	/*  read into fp0;convert to xprec */
	fmovem.x	fp0,FP_SRC(a6)	/*  return src op in FP_SRC */
	rts

load_dbl_immed:
	lea		L_SCR1(a6), a1	/*  pass: ptr to input dbl tmp space */
	moveq.l		#0x8, d0		/*  pass: ; bytes to read */
	bsr.l		_imem_read		/*  fetch src operand from memory */

	tst.l		d1				/*  did ifetch fail? */
	bne.l		funimp_iacc		/*  yes */
	bra.b		load_dbl_cont

/*  must convert dbl denorm format to an Xprec denorm fmt suitable for */
/*  normalization... */
/*  a0 : loc. of dbl denorm */
get_dbl_denorm:
	clr.w		FP_SRC_EX(a6)
	bfextu		(a0){12:31}, d0	/*  fetch hi(_mantissa) */
	move.l		d0, FP_SRC_HI(a6)
	bfextu		4(a0){11:21}, d0	/*  fetch lo(_mantissa) */
	moveq.l		#0xb,d1
	lsl.l		d1,d0
	move.l		d0,FP_SRC_LO(a6)

	btst		#0x7, (a0)		/*  is sgn bit set? */
	beq.b		dbl_dnrm_norm
	bset		#0x7, FP_SRC_EX(a6)	/*  set sgn of xprec value */

dbl_dnrm_norm:
	lea		FP_SRC(a6), a0
	bsr.l		norm			/*  normalize number */
	move.w		#0x3c01, d1		/*  xprec exp = 0x3c01 */
	sub.w		d0, d1		/*  exp = 0x3c01 - shft amt. */
	or.w		d1, FP_SRC_EX(a6)	/*  {sgn,exp} */

	move.b		#NORM, STAG(a6)	/*  fix src type tag */
	rts

/*  convert dbl to ext SNAN */
/*  a0 : points to dbl SNAN */
get_dbl_snan:
	move.w		#0x7fff, FP_SRC_EX(a6) /*  set exp of SNAN */

	bfextu		(a0){12:31}, d0	/*  fetch hi(_mantissa) */
	move.l		d0, FP_SRC_HI(a6)
	bfextu		4(a0){11:21}, d0	/*  fetch lo(_mantissa) */
	moveq.l		#0xb,d1
	lsl.l		d1, d0
	move.l		d0, FP_SRC_LO(a6)

	btst		#0x7, (a0)		/*  see if sign of SNAN is set */
	beq.b		no_dbl_snan_sgn
	bset		#0x7, FP_SRC_EX(a6)
no_dbl_snan_sgn:
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  load a Xprec into fp0:			 */
/* 	-number can't fault			 */
/* 	(1) calc ea				 */
/* 	(2) read 12 bytes into L_SCR(1,2)	 */
/* 	(3) fmove.x into fp0			 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
load_ext:
	moveq.l		#0xc,d0		/*  pass: 12 (bytes) */
	bsr.l		_dcalc_ea		/*  calc <ea> */

	lea		FP_SRC(a6), a1	/*  pass: ptr to input ext tmp space */
	moveq.l		#0xc,d0		/*  pass: ; of bytes to read */
	bsr.l		_dmem_read		/*  fetch src operand from memory */

	tst.l		d1			/*  did dfetch fail? */
	bne.l		facc_in_x		/*  yes */

	lea		FP_SRC(a6), a0	/*  pass: ptr to src op */
	bsr.l		set_tag_x		/*  determine src type tag */

	cmpi.b		#UNNORM,d0		/*  is the src op an UNNORM? */
	beq.b		load_ext_unnorm		/*  yes */

	move.b		d0, STAG(a6)		/*  store the src optype tag */
	rts

load_ext_unnorm:
	bsr.l		unnorm_fix		/*  fix the src UNNORM */
	move.b		d0, STAG(a6)		/*  store the src optype tag */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  load a packed into fp0:			 */
/* 	-number can't fault			 */
/* 	(1) calc ea				 */
/* 	(2) read 12 bytes into L_SCR(1,2,3)	 */
/* 	(3) fmove.x into fp0			 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
load_packed:
	bsr.l		get_packed

	lea		FP_SRC(a6),a0		/*  pass ptr to src op */
	bsr.l		set_tag_x		/*  determine src type tag */
	cmpi.b		#UNNORM,d0		/*  is the src op an UNNORM ZERO? */
	beq.b		load_packed_unnorm	/*  yes */

	move.b		d0,STAG(a6)		/*  store the src optype tag */
	rts

load_packed_unnorm:
	bsr.l		unnorm_fix		/*  fix the UNNORM ZERO */
	move.b		d0,STAG(a6)		/*  store the src optype tag */
	rts
