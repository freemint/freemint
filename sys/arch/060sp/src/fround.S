/*
 *  XDEF ****************************************************************	
 * 	_round(): round result according to precision/mode		
 * 									
 *  XREF ****************************************************************	
 * 	None								
 * 									
 *  INPUT ***************************************************************	
 * 	a0	  = ptr to input operand in internal extended format	
 * 	d1(hi)    = contains rounding precision:			
 * 			ext = $0000xxxx					
 * 			sgl = $0004xxxx					
 * 			dbl = $0008xxxx					
 * 	d1(lo)	  = contains rounding mode:				
 * 			RN  = $xxxx0000					
 * 			RZ  = $xxxx0001					
 * 			RM  = $xxxx0002					
 * 			RP  = $xxxx0003					
 * 	d0{31:29} = contains the g,r,s bits (extended)			
 * 									
 *  OUTPUT **************************************************************	
 * 	a0 = pointer to rounded result					
 * 									
 *  ALGORITHM ***********************************************************	
 * 	On return the value pointed to by a0 is correctly rounded,	
 * 	a0 is preserved and the g-r-s bits in d0 are cleared.		
 * 	The result is not typed - the tag field is invalid.  The	
 * 	result is still in the internal extended format.		
 * 									
 * 	The INEX bit of USER_FPSR will be set if the rounded result was	
 * 	inexact (i.e. if any of the g-r-s bits were set).		
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		_round
_round:
/*
 * ext_grs() looks at the rounding precision and sets the appropriate
 * G,R,S bits.
 * If (G,R,S == 0) then result is exact and round is done, else set
 * the inex flag in status reg and continue.
 */
	bsr.l		ext_grs			/*  extract G,R,S */

	tst.l		d0			/*  are G,R,S zero? */
	beq.w		truncate		/*  yes; round is complete */

	ori.w		#inx2a_mask, 2+USER_FPSR(a6) /*  set inex2/ainex */

/*
 * Use rounding mode as an index into a jump table for these modes.
 * All of the following assumes grs != 0.
 */
	move.w		(tbl_mode.b,pc,d1.w*2), a1 /*  load jump offset */
	jmp		(tbl_mode.b,pc,a1.l)	/*  jmp to rnd mode handler */

tbl_mode:
	.dc.w		rnd_near - tbl_mode
	.dc.w		truncate - tbl_mode	/*  RZ always truncates */
	.dc.w		rnd_mnus - tbl_mode
	.dc.w		rnd_plus - tbl_mode

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 	ROUND PLUS INFINITY					 */
/* 								 */
/* 	If sign of fp number = 0 (positive), then add 1 to l.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
rnd_plus:
	tst.b		FTEMP_SGN(a0)		/*  check for sign */
	bmi.w		truncate		/*  if positive then truncate */

	moveq.l		#0xffffffff, d0	/*  force g,r,s to be all f's */
	swap		d1			/*  set up d1 for round prec. */

	cmpi.b		#s_mode,d1		/*  is prec = sgl? */
	beq.w		add_sgl			/*  yes */
	bgt.w		add_dbl			/*  no; it's dbl */
	bra.w		add_ext			/*  no; it's ext */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 	ROUND MINUS INFINITY					 */
/* 								 */
/* 	If sign of fp number = 1 (negative), then add 1 to l.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
rnd_mnus:
	tst.b		FTEMP_SGN(a0)		/*  check for sign */
	bpl.w		truncate		/*  if negative then truncate */

	moveq.l		#0xffffffff, d0	/*  force g,r,s to be all f's */
	swap		d1			/*  set up d1 for round prec. */

	cmpi.b		#s_mode,d1		/*  is prec = sgl? */
	beq.w		add_sgl			/*  yes */
	bgt.w		add_dbl			/*  no; it's dbl */
	bra.w		add_ext			/*  no; it's ext */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* 	ROUND NEAREST						 */
/* 								 */
/* 	If (g=1), then add 1 to l and if (r=s=0), then clear l	 */
/* 	Note that this will round to even in case of a tie.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
rnd_near:
	asl.l		#0x1, d0		/*  shift g-bit to c-bit */
	bcc.w		truncate		/*  if (g=1) then */

	swap		d1			/*  set up d1 for round prec. */

	cmpi.b		#s_mode,d1		/*  is prec = sgl? */
	beq.w		add_sgl			/*  yes */
	bgt.w		add_dbl			/*  no; it's dbl */
	bra.w		add_ext			/*  no; it's ext */

/*  *** LOCAL EQUATES *** */
	ad_1_sgl = 0x00000100	/*  constant to add 1 to l-bit in sgl prec */
	ad_1_dbl = 0x00000800	/*  constant to add 1 to l-bit in dbl prec */

/* ;;;;;;;;;;;;;;;;;;;;;;; */
/* 	ADD SINGLE	 */
/* ;;;;;;;;;;;;;;;;;;;;;;; */
add_sgl:
	add.l		#ad_1_sgl, FTEMP_HI(a0)
	bcc.b		scc_clr			/*  no mantissa overflow */
	roxr.w		FTEMP_HI(a0)		/*  shift v-bit back in */
	roxr.w		FTEMP_HI+2(a0)		/*  shift v-bit back in */
	addq.w		#0x1,FTEMP_EX.w(a0)	/*  and incr exponent */
scc_clr:
	tst.l		d0			/*  test for rs = 0 */
	bne.b		sgl_done
	andi.w		#0xfe00, FTEMP_HI+2(a0) /*  clear the l-bit */
sgl_done:
	andi.l		#0xffffff00, FTEMP_HI(a0) /*  truncate bits beyond sgl limit */
	clr.l		FTEMP_LO(a0)		/*  clear d2 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;; */
/* 	ADD EXTENDED	 */
/* ;;;;;;;;;;;;;;;;;;;;;;; */
add_ext:
	addq.l		#1,FTEMP_LO(a0)	/*  add 1 to l-bit */
	bcc.b		xcc_clr			/*  test for carry out */
	addq.l		#1,FTEMP_HI(a0)	/*  propagate carry */
	bcc.b		xcc_clr
	roxr.w		FTEMP_HI(a0)		/*  mant is 0 so restore v-bit */
	roxr.w		FTEMP_HI+2(a0)		/*  mant is 0 so restore v-bit */
	roxr.w		FTEMP_LO(a0)
	roxr.w		FTEMP_LO+2(a0)
	addq.w		#0x1,FTEMP_EX.w(a0)	/*  and inc exp */
xcc_clr:
	tst.l		d0			/*  test rs = 0 */
	bne.b		add_ext_done
	and.b		#0xfe,FTEMP_LO+3(a0)	/*  clear the l bit */
add_ext_done:
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;; */
/* 	ADD DOUBLE	 */
/* ;;;;;;;;;;;;;;;;;;;;;;; */
add_dbl:
	add.l		#ad_1_dbl, FTEMP_LO(a0) /*  add 1 to lsb */
	bcc.b		dcc_clr			/*  no carry */
	addq.l		#0x1, FTEMP_HI(a0)	/*  propagate carry */
	bcc.b		dcc_clr			/*  no carry */

	roxr.w		FTEMP_HI(a0)		/*  mant is 0 so restore v-bit */
	roxr.w		FTEMP_HI+2(a0)		/*  mant is 0 so restore v-bit */
	roxr.w		FTEMP_LO(a0)
	roxr.w		FTEMP_LO+2(a0)
	addq.w		#0x1,FTEMP_EX.w(a0)	/*  incr exponent */
dcc_clr:
	tst.l		d0			/*  test for rs = 0 */
	bne.b		dbl_done
	andi.w		#0xf000, FTEMP_LO+2(a0) /*  clear the l-bit */

dbl_done:
	and.l		#0xfffff800,FTEMP_LO(a0) /*  truncate bits beyond dbl limit */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Truncate all other bits  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;; */
truncate:
	swap		d1			/*  select rnd prec */

	cmpi.b		#s_mode,d1		/*  is prec sgl? */
	beq.w		sgl_done		/*  yes */
	bgt.b		dbl_done		/*  no; it's dbl */
	rts					/*  no; it's ext */


/*
 * ext_grs(): extract guard, round and sticky bits according to
 *	     rounding precision.
 *
 * INPUT
 *	d0	   = extended precision g,r,s (in d0{31:29})
 *	d1	   = {PREC,ROUND}
 * OUTPUT
 *	d0{31:29}  = guard, round, sticky
 *
 * The ext_grs extract the guard/round/sticky bits according to the
 * selected rounding precision. It is called by the round subroutine
 * only.  All registers except d0 are kept intact. d0 becomes an
 * updated guard,round,sticky in d0{31:29}
 *
 * Notes: the ext_grs uses the round PREC, and therefore has to swap d1
 *	 prior to usage, and needs to restore d1 to original. this
 *	 routine is tightly tied to the round routine and not meant to
 *	 uphold standard subroutine calling practices.
 */

ext_grs:
	swap		d1			/*  have d1.w point to round precision */
	tst.b		d1			/*  is rnd prec = extended? */
	bne.b		ext_grs_not_ext		/*  no; go handle sgl or dbl */

/*
 * d0 actually already hold g,r,s since _round() had it before calling
 * this function. so, as long as we don't disturb it, we are "returning" it.
 */
ext_grs_ext:
	swap		d1			/*  yes; return to correct positions */
	rts

ext_grs_not_ext:
	movem.l		d2-d3, -(sp)		/*  make some temp registers {d2/d3} */

	cmpi.b		#s_mode,d1		/*  is rnd prec = sgl? */
	bne.b		ext_grs_dbl		/*  no; go handle dbl */

/*
 * sgl:
 *      96              64        40    32              0
 *      -----------------------------------------------------
 *      | EXP   |XXXXXXX|         |xx   |               |grs|
 *      -----------------------------------------------------
 *                      <--(24)--->nn\                     /
 *                                 ee ---------------------
 *                                 ww           |
 *                                              v
 *                                 gr      new sticky
 */
ext_grs_sgl:
	bfextu		FTEMP_HI(a0){24:2}, d3 /*  sgl prec. g-r are 2 bits right */
	moveq.l		#30, d2		/*  of the sgl prec. limits */
	lsl.l		d2, d3		/*  shift g-r bits to MSB of d3 */
	move.l		FTEMP_HI(a0),d2	/*  get word 2 for s-bit test */
	andi.l		#0x0000003f,d2		/*  s bit is the or of all other */
	bne.b		ext_grs_st_stky		/*  bits to the right of g-r */
	tst.l		FTEMP_LO(a0)		/*  test lower mantissa */
	bne.b		ext_grs_st_stky		/*  if any are set, set sticky */
	tst.l		d0			/*  test original g,r,s */
	bne.b		ext_grs_st_stky		/*  if any are set, set sticky */
	bra.b		ext_grs_end_sd		/*  if words 3 and 4 are clr, exit */

/*
 * dbl:
 *      96              64              32       11     0
 *      -----------------------------------------------------
 *      | EXP   |XXXXXXX|               |        |xx    |grs|
 *      -----------------------------------------------------
 *                                                nn\       /
 *                                                ee -------
 *                                                ww    |
 *                                                      v
 *                                                gr    new sticky
 */
ext_grs_dbl:
	bfextu		FTEMP_LO(a0){21:2}, d3 /*  dbl-prec. g-r are 2 bits right */
	moveq.l		#30,d2				/*  of the dbl prec. limits */
	lsl.l		d2,d3				/*  shift g-r bits to the MSB of d3 */
	move.l		FTEMP_LO(a0), d2	/*  get lower mantissa  for s-bit test */
	andi.l		#0x000001ff, d2	/*  s bit is the or-ing of all */
	bne.b		ext_grs_st_stky		/*  other bits to the right of g-r */
	tst.l		d0					/*  test word original g,r,s */
	bne.b		ext_grs_st_stky		/*  if any are set, set sticky */
	bra.b		ext_grs_end_sd		/*  if clear, exit */

ext_grs_st_stky:
	bset		#rnd_stky_bit, d3	/*  set sticky bit */
ext_grs_end_sd:
	move.l		d3, d0		/*  return grs to d0 */

	movem.l		(sp)+, d2-d3		/*  restore scratch registers {d2/d3} */

	swap		d1			/*  restore d1 to original */
	rts

