/*
 *  XDEF ****************************************************************	
 * 	_dcalc_ea(): calc correct <ea> from <ea> stacked on exception	
 * 									
 *  XREF ****************************************************************	
 * 	inc_areg() - increment an address register			
 * 	dec_areg() - decrement an address register			
 * 									
 *  INPUT ***************************************************************	
 * 	d0 = number of bytes to adjust <ea> by				
 * 									
 *  OUTPUT **************************************************************	
 * 	None								
 * 									
 *  ALGORITHM ***********************************************************	
 *  "Dummy" CALCulate Effective Address:					
 * 	The stacked <ea> for FP unimplemented instructions and opclass	
 * 	two packed instructions is correct with the exception of...	
 * 									
 * 	1) -(An)   : The register is not updated regardless of size.	
 * 		     Also, for extended precision and packed, the	
 * 		     stacked <ea> value is 8 bytes too big		
 * 	2) (An)+   : The register is not updated.			
 * 	3) #<data> : The upper longword of the immediate operand is	
 * 		     stacked b,w,l and s sizes are completely stacked.	
 * 		     d,x, and p are not.				
 * 									
 */


	.include "hdr.fpu"

	.text

	.globl		_dcalc_ea
_dcalc_ea:
	move.l		d0, a0		/*  move ; bytes to a0 */

	move.b		1+EXC_OPWORD(a6), d0	/*  fetch opcode word */
	move.l		d0, d1		/*  make a copy */

	andi.w		#0x38, d0		/*  extract mode field */
	andi.l		#0x7, d1		/*  extract reg  field */

	cmpi.b		#0x18,d0		/*  is mode (An)+ ? */
	beq.b		dcea_pi			/*  yes */

	cmpi.b		#0x20,d0		/*  is mode -(An) ? */
	beq.b		dcea_pd			/*  yes */

	or.w		d1,d0			/*  concat mode,reg */
	cmpi.b		#0x3c,d0		/*  is mode #<data>? */

	beq.b		dcea_imm		/*  yes */

	move.l		EXC_EA(a6),a0		/*  return <ea> */
	rts

/*  need to set immediate data flag here since we'll need to do */
/*  an imem_read to fetch this later. */
dcea_imm:
	move.b		#immed_flg,SPCOND_FLG(a6)
	lea		([USER_FPIAR,a6],0x4),a0 /*  no; return <ea> */
	rts

/*  here, the <ea> is stacked correctly. however, we must update the */
/*  address register... */
dcea_pi:
	move.l		a0,d0			/*  pass amt to inc by */
	bsr.l		inc_areg		/*  inc addr register */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	rts

/*  the <ea> is stacked correctly for all but extended and packed which */
/*  the <ea>s are 8 bytes too large. */
/*  it would make no sense to have a pre-decrement to a7 in supervisor */
/*  mode so we don't even worry about this tricky case here : ) */
dcea_pd:
	move.l		a0,d0			/*  pass amt to dec by */
	bsr.l		dec_areg		/*  dec addr register */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */

	cmpi.b		#0xc,d0		/*  is opsize ext or packed? */
	beq.b		dcea_pd2		/*  yes */
	rts
dcea_pd2:
	subq.l		#0x8,a0		/*  correct <ea> */
	move.l		a0,EXC_EA(a6)		/*  put correct <ea> on stack */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	_calc_ea_fout(): calculate correct stacked <ea> for extended	 */
/* 			 and packed data opclass 3 operations.		 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	None								 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	a0 = return correct effective address				 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	For opclass 3 extended and packed data operations, the <ea>	 */
/*  stacked for the exception is incorrect for -(an) and (an)+ addressing	 */
/*  modes. Also, while we're at it, the index register itself must get	 */
/*  updated.								 */
/* 	So, for -(an), we must subtract 8 off of the stacked <ea> value	 */
/*  and return that value as the correct <ea> and store that value in An.	 */
/*  For (an)+, the stacked <ea> is correct but we must adjust An by +12.	 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  This calc_ea is currently used to retrieve the correct <ea> */
/*  for fmove outs of type extended and packed. */
	.globl		_calc_ea_fout
_calc_ea_fout:
	move.b		1+EXC_OPWORD(a6),d0	/*  fetch opcode word */
	move.l		d0,d1			/*  make a copy */

	andi.w		#0x38,d0		/*  extract mode field */
	andi.l		#0x7,d1		/*  extract reg  field */

	cmpi.b		#0x18,d0		/*  is mode (An)+ ? */
	beq.b		ceaf_pi			/*  yes */

	cmpi.b		#0x20,d0		/*  is mode -(An) ? */
	beq.w		ceaf_pd			/*  yes */

	move.l		EXC_EA(a6),a0		/*  stacked <ea> is correct */
	rts

/*  (An)+ : extended and packed fmove out */
/* 	: stacked <ea> is correct */
/* 	: "An" not updated */
ceaf_pi:
	move.w		(tbl_ceaf_pi.b,pc,d1.w*2),d1
	move.l		EXC_EA(a6),a0
	jmp		(tbl_ceaf_pi.b,pc,d1.w*1)

	/* swbeg		#0x8 */
	.dc.w 0x4afc,8
tbl_ceaf_pi:
	.dc.w		ceaf_pi0 - tbl_ceaf_pi
	.dc.w		ceaf_pi1 - tbl_ceaf_pi
	.dc.w		ceaf_pi2 - tbl_ceaf_pi
	.dc.w		ceaf_pi3 - tbl_ceaf_pi
	.dc.w		ceaf_pi4 - tbl_ceaf_pi
	.dc.w		ceaf_pi5 - tbl_ceaf_pi
	.dc.w		ceaf_pi6 - tbl_ceaf_pi
	.dc.w		ceaf_pi7 - tbl_ceaf_pi

ceaf_pi0:
	addi.l		#0xc,EXC_DREGS+0x8(a6)
	rts
ceaf_pi1:
	addi.l		#0xc,EXC_DREGS+0xc(a6)
	rts
ceaf_pi2:
	add.l		#0xc,a2
	rts
ceaf_pi3:
	add.l		#0xc,a3
	rts
ceaf_pi4:
	add.l		#0xc,a4
	rts
ceaf_pi5:
	add.l		#0xc,a5
	rts
ceaf_pi6:
	addi.l		#0xc,EXC_A6(a6)
	rts
ceaf_pi7:
	move.b		#mia7_flg,SPCOND_FLG(a6)
	addi.l		#0xc,EXC_A7(a6)
	rts

/*  -(An) : extended and packed fmove out */
/* 	: stacked <ea> = actual <ea> + 8 */
/* 	: "An" not updated */
ceaf_pd:
	move.w		(tbl_ceaf_pd.b,pc,d1.w*2),d1
	move.l		EXC_EA(a6),a0
	subq.l		#0x8,a0
	subq.l		#0x8,EXC_EA(a6)
	jmp		(tbl_ceaf_pd.b,pc,d1.w*1)

	/* swbeg		#0x8 */
	.dc.w 0x4afc,8
tbl_ceaf_pd:
	.dc.w		ceaf_pd0 - tbl_ceaf_pd
	.dc.w		ceaf_pd1 - tbl_ceaf_pd
	.dc.w		ceaf_pd2 - tbl_ceaf_pd
	.dc.w		ceaf_pd3 - tbl_ceaf_pd
	.dc.w		ceaf_pd4 - tbl_ceaf_pd
	.dc.w		ceaf_pd5 - tbl_ceaf_pd
	.dc.w		ceaf_pd6 - tbl_ceaf_pd
	.dc.w		ceaf_pd7 - tbl_ceaf_pd

ceaf_pd0:
	move.l		a0,EXC_DREGS+0x8(a6)
	rts
ceaf_pd1:
	move.l		a0,EXC_DREGS+0xc(a6)
	rts
ceaf_pd2:
	move.l		a0,a2
	rts
ceaf_pd3:
	move.l		a0,a3
	rts
ceaf_pd4:
	move.l		a0,a4
	rts
ceaf_pd5:
	move.l		a0,a5
	rts
ceaf_pd6:
	move.l		a0,EXC_A6(a6)
	rts
ceaf_pd7:
	move.l		a0,EXC_A7(a6)
	move.b		#mda7_flg,SPCOND_FLG(a6)
	rts

