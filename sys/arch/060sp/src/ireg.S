/*
 *  XDEF ****************************************************************	
 * 	inc_areg(): increment an address register by the value in d0	
 * 									
 *  XREF ****************************************************************	
 * 	None								
 * 									
 *  INPUT ***************************************************************	
 * 	d0 = amount to increment by					
 * 	d1 = index of address register to increment			
 * 									
 *  OUTPUT **************************************************************	
 * 	(address register is updated)					
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Typically used for an instruction w/ a post-increment <ea>,	
 *  this routine adds the increment value in d0 to the address register	
 *  specified by d1. A0/A1/A6/A7 reside on the stack. The rest reside	
 *  in their original places.						
 * 	For a7, if the increment amount is one, then we have to		
 *  increment by two. For any a7 update, set the mia7_flag so that if	
 *  an access error exception occurs later in emulation, this address	
 *  register update can be undone.					
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		inc_areg
inc_areg:
	move.w		(tbl_iareg.b,pc,d1.w*2),d1
	jmp		(tbl_iareg.b,pc,d1.w*1)

tbl_iareg:
	.dc.w		iareg0 - tbl_iareg
	.dc.w		iareg1 - tbl_iareg
	.dc.w		iareg2 - tbl_iareg
	.dc.w		iareg3 - tbl_iareg
	.dc.w		iareg4 - tbl_iareg
	.dc.w		iareg5 - tbl_iareg
	.dc.w		iareg6 - tbl_iareg
	.dc.w		iareg7 - tbl_iareg

iareg0:	add.l		d0,EXC_DREGS+0x8(a6)
	rts
iareg1:	add.l		d0,EXC_DREGS+0xc(a6)
	rts
iareg2:	add.l		d0,a2
	rts
iareg3:	add.l		d0,a3
	rts
iareg4:	add.l		d0,a4
	rts
iareg5:	add.l		d0,a5
	rts
iareg6:	add.l		d0,(a6)
	rts
iareg7:	move.b		#mia7_flg,SPCOND_FLG(a6)
	cmpi.b		#0x1,d0
	beq.b		iareg7b
	add.l		d0,EXC_A7(a6)
	rts
iareg7b:
	addq.l		#0x2,EXC_A7(a6)
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	dec_areg(): decrement an address register by the value in d0	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	d0 = amount to decrement by					 */
/* 	d1 = index of address register to decrement			 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	(address register is updated)					 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Typically used for an instruction w/ a pre-decrement <ea>,	 */
/*  this routine adds the decrement value in d0 to the address register	 */
/*  specified by d1. A0/A1/A6/A7 reside on the stack. The rest reside	 */
/*  in their original places.						 */
/* 	For a7, if the decrement amount is one, then we have to		 */
/*  decrement by two. For any a7 update, set the mda7_flag so that if	 */
/*  an access error exception occurs later in emulation, this address	 */
/*  register update can be undone.					 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		dec_areg
dec_areg:
	move.w		(tbl_dareg.b,pc,d1.w*2),d1
	jmp		(tbl_dareg.b,pc,d1.w*1)

tbl_dareg:
	.dc.w		dareg0 - tbl_dareg
	.dc.w		dareg1 - tbl_dareg
	.dc.w		dareg2 - tbl_dareg
	.dc.w		dareg3 - tbl_dareg
	.dc.w		dareg4 - tbl_dareg
	.dc.w		dareg5 - tbl_dareg
	.dc.w		dareg6 - tbl_dareg
	.dc.w		dareg7 - tbl_dareg

dareg0:	sub.l		d0,EXC_DREGS+0x8(a6)
	rts
dareg1:	sub.l		d0,EXC_DREGS+0xc(a6)
	rts
dareg2:	sub.l		d0,a2
	rts
dareg3:	sub.l		d0,a3
	rts
dareg4:	sub.l		d0,a4
	rts
dareg5:	sub.l		d0,a5
	rts
dareg6:	sub.l		d0,(a6)
	rts
dareg7:	move.b		#mda7_flg,SPCOND_FLG(a6)
	cmpi.b		#0x1,d0
	beq.b		dareg7b
	sub.l		d0,EXC_A7(a6)
	rts
dareg7b:
	subq.l		#0x2,EXC_A7(a6)
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	load_fpn1(): load FP register value into FP_SRC(a6).		 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	d0 = index of FP register to load				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	FP_SRC(a6) = value loaded from FP register file			 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Using the index in d0, load FP_SRC(a6) with a number from the	 */
/*  FP register file.							 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		load_fpn1
load_fpn1:
	move.w		(tbl_load_fpn1.b,pc,d0.w*2), d0
	jmp		(tbl_load_fpn1.b,pc,d0.w*1)

tbl_load_fpn1:
	.dc.w		load_fpn1_0 - tbl_load_fpn1
	.dc.w		load_fpn1_1 - tbl_load_fpn1
	.dc.w		load_fpn1_2 - tbl_load_fpn1
	.dc.w		load_fpn1_3 - tbl_load_fpn1
	.dc.w		load_fpn1_4 - tbl_load_fpn1
	.dc.w		load_fpn1_5 - tbl_load_fpn1
	.dc.w		load_fpn1_6 - tbl_load_fpn1
	.dc.w		load_fpn1_7 - tbl_load_fpn1

load_fpn1_0:
	move.l		0+EXC_FP0(a6), 0+FP_SRC(a6)
	move.l		4+EXC_FP0(a6), 4+FP_SRC(a6)
	move.l		8+EXC_FP0(a6), 8+FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts
load_fpn1_1:
	move.l		0+EXC_FP1(a6), 0+FP_SRC(a6)
	move.l		4+EXC_FP1(a6), 4+FP_SRC(a6)
	move.l		8+EXC_FP1(a6), 8+FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts
load_fpn1_2:
	fmovem.x		fp2,FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts
load_fpn1_3:
	fmovem.x		fp3,FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts
load_fpn1_4:
	fmovem.x		fp4,FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts
load_fpn1_5:
	fmovem.x		fp5,FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts
load_fpn1_6:
	fmovem.x		fp6,FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts
load_fpn1_7:
	fmovem.x		fp7,FP_SRC(a6)
	lea		FP_SRC(a6), a0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	load_fpn2(): load FP register value into FP_DST(a6).		 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	d0 = index of FP register to load				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	FP_DST(a6) = value loaded from FP register file			 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Using the index in d0, load FP_DST(a6) with a number from the	 */
/*  FP register file.							 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		load_fpn2
load_fpn2:
	move.w		(tbl_load_fpn2.b,pc,d0.w*2), d0
	jmp		(tbl_load_fpn2.b,pc,d0.w*1)

tbl_load_fpn2:
	.dc.w		load_fpn2_0 - tbl_load_fpn2
	.dc.w		load_fpn2_1 - tbl_load_fpn2
	.dc.w		load_fpn2_2 - tbl_load_fpn2
	.dc.w		load_fpn2_3 - tbl_load_fpn2
	.dc.w		load_fpn2_4 - tbl_load_fpn2
	.dc.w		load_fpn2_5 - tbl_load_fpn2
	.dc.w		load_fpn2_6 - tbl_load_fpn2
	.dc.w		load_fpn2_7 - tbl_load_fpn2

load_fpn2_0:
	move.l		0+EXC_FP0(a6), 0+FP_DST(a6)
	move.l		4+EXC_FP0(a6), 4+FP_DST(a6)
	move.l		8+EXC_FP0(a6), 8+FP_DST(a6)
	lea		FP_DST(a6), a0
	rts
load_fpn2_1:
	move.l		0+EXC_FP1(a6), 0+FP_DST(a6)
	move.l		4+EXC_FP1(a6), 4+FP_DST(a6)
	move.l		8+EXC_FP1(a6), 8+FP_DST(a6)
	lea		FP_DST(a6), a0
	rts
load_fpn2_2:
	fmovem.x		fp2,FP_DST(a6)
	lea		FP_DST(a6), a0
	rts
load_fpn2_3:
	fmovem.x		fp3,FP_DST(a6)
	lea		FP_DST(a6), a0
	rts
load_fpn2_4:
	fmovem.x		fp4,FP_DST(a6)
	lea		FP_DST(a6), a0
	rts
load_fpn2_5:
	fmovem.x		fp5,FP_DST(a6)
	lea		FP_DST(a6), a0
	rts
load_fpn2_6:
	fmovem.x		fp6,FP_DST(a6)
	lea		FP_DST(a6), a0
	rts
load_fpn2_7:
	fmovem.x		fp7,FP_DST(a6)
	lea		FP_DST(a6), a0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  XDEF ****************************************************************	 */
/* 	store_fpreg(): store an fp value to the fpreg designated d0.	 */
/* 									 */
/*  XREF ****************************************************************	 */
/* 	None								 */
/* 									 */
/*  INPUT ***************************************************************	 */
/* 	fp0 = extended precision value to store				 */
/* 	d0  = index of floating-point register				 */
/* 									 */
/*  OUTPUT **************************************************************	 */
/* 	None								 */
/* 									 */
/*  ALGORITHM ***********************************************************	 */
/* 	Store the value in fp0 to the FP register designated by the	 */
/*  value in d0. The FP number can be DENORM or SNAN so we have to be	 */
/*  careful that we don't take an exception here.				 */
/* 									 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		store_fpreg
store_fpreg:
	move.w		(tbl_store_fpreg.b,pc,d0.w*2), d0
	jmp		(tbl_store_fpreg.b,pc,d0.w*1)

tbl_store_fpreg:
	.dc.w		store_fpreg_0 - tbl_store_fpreg
	.dc.w		store_fpreg_1 - tbl_store_fpreg
	.dc.w		store_fpreg_2 - tbl_store_fpreg
	.dc.w		store_fpreg_3 - tbl_store_fpreg
	.dc.w		store_fpreg_4 - tbl_store_fpreg
	.dc.w		store_fpreg_5 - tbl_store_fpreg
	.dc.w		store_fpreg_6 - tbl_store_fpreg
	.dc.w		store_fpreg_7 - tbl_store_fpreg

store_fpreg_0:
	fmovem.x		fp0, EXC_FP0(a6)
	rts
store_fpreg_1:
	fmovem.x		fp0, EXC_FP1(a6)
	rts
store_fpreg_2:
	fmovem.x		fp0, -(sp)
	fmovem.x		(sp)+, fp2
	rts
store_fpreg_3:
	fmovem.x		fp0, -(sp)
	fmovem.x		(sp)+, fp3
	rts
store_fpreg_4:
	fmovem.x		fp0, -(sp)
	fmovem.x		(sp)+, fp4
	rts
store_fpreg_5:
	fmovem.x		fp0, -(sp)
	fmovem.x		(sp)+, fp5
	rts
store_fpreg_6:
	fmovem.x		fp0, -(sp)
	fmovem.x		(sp)+, fp6
	rts
store_fpreg_7:
	fmovem.x		fp0, -(sp)
	fmovem.x		(sp)+, fp7
	rts

