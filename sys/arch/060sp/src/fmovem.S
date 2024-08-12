/*
 *  XDEF ****************************************************************	
 * 	fmovem_dynamic(): emulate "fmovem" dynamic instruction		
 * 									
 *  XREF ****************************************************************	
 * 	fetch_dreg() - fetch data register				
 * 	{i,d,}mem_read() - fetch data from memory			
 * 	_mem_write() - write data to memory				
 * 	iea_iacc() - instruction memory access error occurred		
 * 	iea_dacc() - data memory access error occurred			
 * 	restore() - restore An index regs if access error occurred	
 * 									
 *  INPUT ***************************************************************	
 * 	None								
 * 									
 *  OUTPUT **************************************************************	
 * 	If instr is "fmovem Dn,-(A7)" from supervisor mode,		
 * 		d0 = size of dump					
 * 		d1 = Dn							
 * 	Else if instruction access error,				
 * 		d0 = FSLW						
 * 	Else if data access error,					
 * 		d0 = FSLW						
 * 		a0 = address of fault					
 * 	Else								
 * 		none.							
 * 									
 *  ALGORITHM ***********************************************************	
 * 	The effective address must be calculated since this is entered	
 *  from an "Unimplemented Effective Address" exception handler. So, we	
 *  have our own fcalc_ea() routine here. If an access error is flagged	
 *  by a _{i,d,}mem_read() call, we must exit through the special		
 *  handler.								
 * 	The data register is determined and its value loaded to get the	
 *  string of FP registers affected. This value is used as an index into	
 *  a lookup table such that we can determine the number of bytes		
 *  involved.								
 * 	If the instruction is "fmovem.x <ea>,Dn", a _mem_read() is used	
 *  to read in all FP values. Again, _mem_read() may fail and require a	
 *  special exit.								
 * 	If the instruction is "fmovem.x DN,<ea>", a _mem_write() is used	
 *  to write all FP values. _mem_write() may also fail.			
 * 	If the instruction is "fmovem.x DN,-(a7)" from supervisor mode,	
 *  then we return the size of the dump and the string to the caller	
 *  so that the move can occur outside of this routine. This special	
 *  case is required so that moves to the system stack are handled	
 *  correctly.								
 * 									
 *  DYNAMIC:								
 * 	fmovem.x	dn, <ea>						
 * 	fmovem.x	<ea>, dn						
 * 									
 * 	      <WORD 1>		      <WORD2>				
 * 	1111 0010 00 |<ea>|	11@# 1000 0$$$ 0000			
 * 									
 * 	# = (0): predecrement addressing mode				
 * 	    (1): postincrement or control addressing mode		
 * 	@ = (0): move listed regs from memory to the FPU		
 * 	    (1): move listed regs from the FPU to memory		
 * 	$$$    : index of data register holding reg select mask		
 * 									
 *  NOTES:								
 * 	If the data register holds a zero, then the			
 * 	instruction is a nop.						
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fmovem_dynamic
fmovem_dynamic:

/*  extract the data register in which the bit string resides... */
	move.b		1+EXC_EXTWORD(a6),d1	/*  fetch extword */
	andi.w		#0x70,d1		/*  extract reg bits */
	lsr.b		#0x4,d1		/*  shift into lo bits */

/*  fetch the bit string into d0... */
	bsr.l		fetch_dreg		/*  fetch reg string */

	andi.l		#0x000000ff,d0		/*  keep only lo byte */

	move.l		d0,-(sp)		/*  save strg */
	move.b		(tbl_fmovem_size.w,pc,d0.l),d0
	move.l		d0,-(sp)		/*  save size */
	bsr.l		fmovem_calc_ea		/*  calculate <ea> */
	move.l		(sp)+,d0		/*  restore size */
	move.l		(sp)+,d1		/*  restore strg */

/*  if the bit string is a zero, then the operation is a no-op */
/*  but, make sure that we've calculated ea and advanced the opword pointer */
	beq.w		fmovem_data_done

/*  separate move ins from move outs... */
	btst		#0x5,EXC_EXTWORD(a6)	/*  is it a move in or out? */
	beq.w		fmovem_data_in		/*  it's a move out */

/* ;;;;;;;;;;; */
/*  MOVE OUT:  */
/* ;;;;;;;;;;; */
fmovem_data_out:
	btst		#0x4,EXC_EXTWORD(a6)	/*  control or predecrement? */
	bne.w		fmovem_out_ctrl		/*  control */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;; */
fmovem_out_predec:
/*  for predecrement mode, the bit string is the opposite of both control */
/*  operations and postincrement mode. (bit7 = FP7 ... bit0 = FP0) */
/*  here, we convert it to be just like the others... */
	move.b		(tbl_fmovem_convert.w,pc,d1.w*1),d1

	btst		#0x5,EXC_SR(a6)	/*  user or supervisor mode? */
	beq.b		fmovem_out_ctrl		/*  user */

fmovem_out_predec_s:
	cmpi.b		#mda7_flg,SPCOND_FLG(a6) /*  is <ea> mode -(a7)? */
	bne.b		fmovem_out_ctrl

/*  the operation was unfortunately an: fmovem.x dn,-(sp) */
/*  called from supervisor mode. */
/*  we're also passing "size" and "strg" back to the calling routine */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;; */
fmovem_out_ctrl:
	move.l		a0,a1			/*  move <ea> to a1 */

	sub.l		d0,sp			/*  subtract size of dump */
	lea		(sp),a0

	tst.b		d1			/*  should FP0 be moved? */
	bpl.b		fmovem_out_ctrl_fp1	/*  no */

	move.l		0x0+EXC_FP0(a6),(a0)+	/*  yes */
	move.l		0x4+EXC_FP0(a6),(a0)+
	move.l		0x8+EXC_FP0(a6),(a0)+

fmovem_out_ctrl_fp1:
	lsl.b		#0x1,d1		/*  should FP1 be moved? */
	bpl.b		fmovem_out_ctrl_fp2	/*  no */

	move.l		0x0+EXC_FP1(a6),(a0)+	/*  yes */
	move.l		0x4+EXC_FP1(a6),(a0)+
	move.l		0x8+EXC_FP1(a6),(a0)+

fmovem_out_ctrl_fp2:
	lsl.b		#0x1,d1		/*  should FP2 be moved? */
	bpl.b		fmovem_out_ctrl_fp3	/*  no */

	fmovem.x		fp2,(a0)		/*  yes */
	add.l		#0xc,a0

fmovem_out_ctrl_fp3:
	lsl.b		#0x1,d1		/*  should FP3 be moved? */
	bpl.b		fmovem_out_ctrl_fp4	/*  no */

	fmovem.x		fp3,(a0)		/*  yes */
	add.l		#0xc,a0

fmovem_out_ctrl_fp4:
	lsl.b		#0x1,d1		/*  should FP4 be moved? */
	bpl.b		fmovem_out_ctrl_fp5	/*  no */

	fmovem.x		fp4,(a0)		/*  yes */
	add.l		#0xc,a0

fmovem_out_ctrl_fp5:
	lsl.b		#0x1,d1		/*  should FP5 be moved? */
	bpl.b		fmovem_out_ctrl_fp6	/*  no */

	fmovem.x		fp5,(a0)		/*  yes */
	add.l		#0xc,a0

fmovem_out_ctrl_fp6:
	lsl.b		#0x1,d1		/*  should FP6 be moved? */
	bpl.b		fmovem_out_ctrl_fp7	/*  no */

	fmovem.x		fp6,(a0)		/*  yes */
	add.l		#0xc,a0

fmovem_out_ctrl_fp7:
	lsl.b		#0x1,d1		/*  should FP7 be moved? */
	bpl.b		fmovem_out_ctrl_done	/*  no */

	fmovem.x		fp7,(a0)		/*  yes */
	add.l		#0xc,a0

fmovem_out_ctrl_done:
	move.l		a1,L_SCR1(a6)

	lea		(sp),a0		/*  pass: supervisor src */
	move.l		d0,-(sp)		/*  save size */
	bsr.l		_dmem_write		/*  copy data to user mem */

	move.l		(sp)+,d0
	add.l		d0,sp			/*  clear fpreg data from stack */

	tst.l		d1			/*  did dstore err? */
	bne.w		fmovem_out_err		/*  yes */

	rts

/* ;;;;;;;;;; */
/*  MOVE IN:  */
/* ;;;;;;;;;; */
fmovem_data_in:
	move.l		a0,L_SCR1(a6)

	sub.l		d0,sp			/*  make room for fpregs */
	lea		(sp),a1

	move.l		d1,-(sp)		/*  save bit string for later */
	move.l		d0,-(sp)		/*  save ; of bytes */

	bsr.l		_dmem_read		/*  copy data from user mem */

	move.l		(sp)+,d0		/*  retrieve ; of bytes */

	tst.l		d1			/*  did dfetch fail? */
	bne.w		fmovem_in_err		/*  yes */

	move.l		(sp)+,d1		/*  load bit string */

	lea		(sp),a0		/*  addr of stack */

	tst.b		d1			/*  should FP0 be moved? */
	bpl.b		fmovem_data_in_fp1	/*  no */

	move.l		(a0)+,0x0+EXC_FP0(a6)	/*  yes */
	move.l		(a0)+,0x4+EXC_FP0(a6)
	move.l		(a0)+,0x8+EXC_FP0(a6)

fmovem_data_in_fp1:
	lsl.b		#0x1,d1		/*  should FP1 be moved? */
	bpl.b		fmovem_data_in_fp2	/*  no */

	move.l		(a0)+,0x0+EXC_FP1(a6)	/*  yes */
	move.l		(a0)+,0x4+EXC_FP1(a6)
	move.l		(a0)+,0x8+EXC_FP1(a6)

fmovem_data_in_fp2:
	lsl.b		#0x1,d1		/*  should FP2 be moved? */
	bpl.b		fmovem_data_in_fp3	/*  no */

	fmovem.x		(a0)+,fp2		/*  yes */

fmovem_data_in_fp3:
	lsl.b		#0x1,d1		/*  should FP3 be moved? */
	bpl.b		fmovem_data_in_fp4	/*  no */

	fmovem.x		(a0)+,fp3		/*  yes */

fmovem_data_in_fp4:
	lsl.b		#0x1,d1		/*  should FP4 be moved? */
	bpl.b		fmovem_data_in_fp5	/*  no */

	fmovem.x		(a0)+,fp4		/*  yes */

fmovem_data_in_fp5:
	lsl.b		#0x1,d1		/*  should FP5 be moved? */
	bpl.b		fmovem_data_in_fp6	/*  no */

	fmovem.x		(a0)+,fp5		/*  yes */

fmovem_data_in_fp6:
	lsl.b		#0x1,d1		/*  should FP6 be moved? */
	bpl.b		fmovem_data_in_fp7	/*  no */

	fmovem.x		(a0)+,fp6		/*  yes */

fmovem_data_in_fp7:
	lsl.b		#0x1,d1		/*  should FP7 be moved? */
	bpl.b		fmovem_data_in_done	/*  no */

	fmovem.x		(a0)+,fp7		/*  yes */

fmovem_data_in_done:
	add.l		d0,sp			/*  remove fpregs from stack */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

fmovem_data_done:
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * table indexed by the operation's bit string that gives the number
 * of bytes that will be moved.
 *
 * number of bytes = (# of 1's in bit string) * 12(bytes/fpreg)
 */
tbl_fmovem_size:
	.dc.b	0x00,0x0c,0x0c,0x18,0x0c,0x18,0x18,0x24
	.dc.b	0x0c,0x18,0x18,0x24,0x18,0x24,0x24,0x30
	.dc.b	0x0c,0x18,0x18,0x24,0x18,0x24,0x24,0x30
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x0c,0x18,0x18,0x24,0x18,0x24,0x24,0x30
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x0c,0x18,0x18,0x24,0x18,0x24,0x24,0x30
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x30,0x3c,0x3c,0x48,0x3c,0x48,0x48,0x54
	.dc.b	0x0c,0x18,0x18,0x24,0x18,0x24,0x24,0x30
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x30,0x3c,0x3c,0x48,0x3c,0x48,0x48,0x54
	.dc.b	0x18,0x24,0x24,0x30,0x24,0x30,0x30,0x3c
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x30,0x3c,0x3c,0x48,0x3c,0x48,0x48,0x54
	.dc.b	0x24,0x30,0x30,0x3c,0x30,0x3c,0x3c,0x48
	.dc.b	0x30,0x3c,0x3c,0x48,0x3c,0x48,0x48,0x54
	.dc.b	0x30,0x3c,0x3c,0x48,0x3c,0x48,0x48,0x54
	.dc.b	0x3c,0x48,0x48,0x54,0x48,0x54,0x54,0x60

/*
 * table to convert a pre-decrement bit string into a post-increment
 * or control bit string.
 * ex:	0x00	==>	0x00
 *	0x01	==>	0x80
 *	0x02	==>	0x40
 *		.
 *		.
 *	0xfd	==>	0xbf
 *	0xfe	==>	0x7f
 *	0xff	==>	0xff
 */
tbl_fmovem_convert:
	.dc.b	0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0
	.dc.b	0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0
	.dc.b	0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8
	.dc.b	0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8
	.dc.b	0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4
	.dc.b	0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4
	.dc.b	0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec
	.dc.b	0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc
	.dc.b	0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2
	.dc.b	0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2
	.dc.b	0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea
	.dc.b	0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa
	.dc.b	0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6
	.dc.b	0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6
	.dc.b	0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee
	.dc.b	0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe
	.dc.b	0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1
	.dc.b	0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1
	.dc.b	0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9
	.dc.b	0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9
	.dc.b	0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5
	.dc.b	0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5
	.dc.b	0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed
	.dc.b	0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd
	.dc.b	0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3
	.dc.b	0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3
	.dc.b	0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb
	.dc.b	0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb
	.dc.b	0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7
	.dc.b	0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7
	.dc.b	0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef
	.dc.b	0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff

	.globl		fmovem_calc_ea
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  _fmovem_calc_ea: calculate effective address  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fmovem_calc_ea:
	move.l		d0,a0			/*  move ; bytes to a0 */

/*  currently, MODE and REG are taken from the EXC_OPWORD. this could be */
/*  easily changed if they were inputs passed in registers. */
	move.w		EXC_OPWORD(a6),d0	/*  fetch opcode word */
	move.w		d0,d1			/*  make a copy */

	andi.w		#0x3f,d0		/*  extract mode field */
	andi.l		#0x7,d1		/*  extract reg  field */

/*  jump to the corresponding function for each {MODE,REG} pair. */
	move.w		(tbl_fea_mode.b,pc,d0.w*2),d0 /*  fetch jmp distance */
	jmp		(tbl_fea_mode.b,pc,d0.w*1) /*  jmp to correct ea mode */

	/* swbeg		#64 */
	.dc.w 0x4afc,64
tbl_fea_mode:
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode

	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode

	.dc.w		faddr_ind_a0	-	tbl_fea_mode
	.dc.w		faddr_ind_a1	-	tbl_fea_mode
	.dc.w		faddr_ind_a2	-	tbl_fea_mode
	.dc.w		faddr_ind_a3	-	tbl_fea_mode
	.dc.w		faddr_ind_a4	-	tbl_fea_mode
	.dc.w		faddr_ind_a5	-	tbl_fea_mode
	.dc.w		faddr_ind_a6	-	tbl_fea_mode
	.dc.w		faddr_ind_a7	-	tbl_fea_mode

	.dc.w		faddr_ind_p_a0	-	tbl_fea_mode
	.dc.w		faddr_ind_p_a1	-	tbl_fea_mode
	.dc.w		faddr_ind_p_a2	-	tbl_fea_mode
	.dc.w		faddr_ind_p_a3	-	tbl_fea_mode
	.dc.w		faddr_ind_p_a4	-	tbl_fea_mode
	.dc.w		faddr_ind_p_a5	-	tbl_fea_mode
	.dc.w		faddr_ind_p_a6	-	tbl_fea_mode
	.dc.w		faddr_ind_p_a7	-	tbl_fea_mode

	.dc.w		faddr_ind_m_a0	-	tbl_fea_mode
	.dc.w		faddr_ind_m_a1	-	tbl_fea_mode
	.dc.w		faddr_ind_m_a2	-	tbl_fea_mode
	.dc.w		faddr_ind_m_a3	-	tbl_fea_mode
	.dc.w		faddr_ind_m_a4	-	tbl_fea_mode
	.dc.w		faddr_ind_m_a5	-	tbl_fea_mode
	.dc.w		faddr_ind_m_a6	-	tbl_fea_mode
	.dc.w		faddr_ind_m_a7	-	tbl_fea_mode

	.dc.w		faddr_ind_disp_a0	-	tbl_fea_mode
	.dc.w		faddr_ind_disp_a1	-	tbl_fea_mode
	.dc.w		faddr_ind_disp_a2	-	tbl_fea_mode
	.dc.w		faddr_ind_disp_a3	-	tbl_fea_mode
	.dc.w		faddr_ind_disp_a4	-	tbl_fea_mode
	.dc.w		faddr_ind_disp_a5	-	tbl_fea_mode
	.dc.w		faddr_ind_disp_a6	-	tbl_fea_mode
	.dc.w		faddr_ind_disp_a7	-	tbl_fea_mode

	.dc.w		faddr_ind_ext	-	tbl_fea_mode
	.dc.w		faddr_ind_ext	-	tbl_fea_mode
	.dc.w		faddr_ind_ext	-	tbl_fea_mode
	.dc.w		faddr_ind_ext	-	tbl_fea_mode
	.dc.w		faddr_ind_ext	-	tbl_fea_mode
	.dc.w		faddr_ind_ext	-	tbl_fea_mode
	.dc.w		faddr_ind_ext	-	tbl_fea_mode
	.dc.w		faddr_ind_ext	-	tbl_fea_mode

	.dc.w		fabs_short	-	tbl_fea_mode
	.dc.w		fabs_long	-	tbl_fea_mode
	.dc.w		fpc_ind		-	tbl_fea_mode
	.dc.w		fpc_ind_ext	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode
	.dc.w		tbl_fea_mode	-	tbl_fea_mode

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Address register indirect: (An)  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
faddr_ind_a0:
	move.l		EXC_DREGS+0x8(a6),a0	/*  Get current a0 */
	rts

faddr_ind_a1:
	move.l		EXC_DREGS+0xc(a6),a0	/*  Get current a1 */
	rts

faddr_ind_a2:
	move.l		a2,a0			/*  Get current a2 */
	rts

faddr_ind_a3:
	move.l		a3,a0			/*  Get current a3 */
	rts

faddr_ind_a4:
	move.l		a4,a0			/*  Get current a4 */
	rts

faddr_ind_a5:
	move.l		a5,a0			/*  Get current a5 */
	rts

faddr_ind_a6:
	move.l		(a6),a0		/*  Get current a6 */
	rts

faddr_ind_a7:
	move.l		EXC_A7(a6),a0		/*  Get current a7 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Address register indirect w/ postincrement: (An)+  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
faddr_ind_p_a0:
	move.l		EXC_DREGS+0x8(a6),d0	/*  Get current a0 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,EXC_DREGS+0x8(a6)	/*  Save incr value */
	move.l		d0,a0
	rts

faddr_ind_p_a1:
	move.l		EXC_DREGS+0xc(a6),d0	/*  Get current a1 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,EXC_DREGS+0xc(a6)	/*  Save incr value */
	move.l		d0,a0
	rts

faddr_ind_p_a2:
	move.l		a2,d0			/*  Get current a2 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,a2			/*  Save incr value */
	move.l		d0,a0
	rts

faddr_ind_p_a3:
	move.l		a3,d0			/*  Get current a3 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,a3			/*  Save incr value */
	move.l		d0,a0
	rts

faddr_ind_p_a4:
	move.l		a4,d0			/*  Get current a4 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,a4			/*  Save incr value */
	move.l		d0,a0
	rts

faddr_ind_p_a5:
	move.l		a5,d0			/*  Get current a5 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,a5			/*  Save incr value */
	move.l		d0,a0
	rts

faddr_ind_p_a6:
	move.l		(a6),d0		/*  Get current a6 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,(a6)		/*  Save incr value */
	move.l		d0,a0
	rts

faddr_ind_p_a7:
	move.b		#mia7_flg,SPCOND_FLG(a6) /*  set "special case" flag */

	move.l		EXC_A7(a6),d0		/*  Get current a7 */
	move.l		d0,d1
	add.l		a0,d1			/*  Increment */
	move.l		d1,EXC_A7(a6)		/*  Save incr value */
	move.l		d0,a0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Address register indirect w/ predecrement: -(An)  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
faddr_ind_m_a0:
	move.l		EXC_DREGS+0x8(a6),d0	/*  Get current a0 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,EXC_DREGS+0x8(a6)	/*  Save decr value */
	move.l		d0,a0
	rts

faddr_ind_m_a1:
	move.l		EXC_DREGS+0xc(a6),d0	/*  Get current a1 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,EXC_DREGS+0xc(a6)	/*  Save decr value */
	move.l		d0,a0
	rts

faddr_ind_m_a2:
	move.l		a2,d0			/*  Get current a2 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,a2			/*  Save decr value */
	move.l		d0,a0
	rts

faddr_ind_m_a3:
	move.l		a3,d0			/*  Get current a3 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,a3			/*  Save decr value */
	move.l		d0,a0
	rts

faddr_ind_m_a4:
	move.l		a4,d0			/*  Get current a4 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,a4			/*  Save decr value */
	move.l		d0,a0
	rts

faddr_ind_m_a5:
	move.l		a5,d0			/*  Get current a5 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,a5			/*  Save decr value */
	move.l		d0,a0
	rts

faddr_ind_m_a6:
	move.l		(a6),d0		/*  Get current a6 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,(a6)		/*  Save decr value */
	move.l		d0,a0
	rts

faddr_ind_m_a7:
	move.b		#mda7_flg,SPCOND_FLG(a6) /*  set "special case" flag */

	move.l		EXC_A7(a6),d0		/*  Get current a7 */
	sub.l		a0,d0			/*  Decrement */
	move.l		d0,EXC_A7(a6)		/*  Save decr value */
	move.l		d0,a0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Address register indirect w/ displacement: (d16, An)  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
faddr_ind_disp_a0:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		EXC_DREGS+0x8(a6),a0	/*  a0 + d16 */
	rts

faddr_ind_disp_a1:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		EXC_DREGS+0xc(a6),a0	/*  a1 + d16 */
	rts

faddr_ind_disp_a2:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		a2,a0			/*  a2 + d16 */
	rts

faddr_ind_disp_a3:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		a3,a0			/*  a3 + d16 */
	rts

faddr_ind_disp_a4:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		a4,a0			/*  a4 + d16 */
	rts

faddr_ind_disp_a5:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		a5,a0			/*  a5 + d16 */
	rts

faddr_ind_disp_a6:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		(a6),a0		/*  a6 + d16 */
	rts

faddr_ind_disp_a7:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		EXC_A7(a6),a0		/*  a7 + d16 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Address register indirect w/ index(8-bit displacement): (d8, An, Xn)  */
/*     "       "         "    w/   "  (base displacement): (bd, An, Xn)   */
/*  Memory indirect postindexed: ([bd, An], Xn, od) */
/*  Memory indirect preindexed: ([bd, An, Xn], od) */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
faddr_ind_ext:
	addq.l		#0x8,d1
	bsr.l		fetch_dreg		/*  fetch base areg */
	move.l		d0,-(sp)

	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word		/*  fetch extword in d0 */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		(sp)+,a0

	btst		#0x8,d0
	bne.w		fcalc_mem_ind

	move.l		d0,L_SCR1(a6)		/*  hold opword */

	move.l		d0,d1
	rol.w		#0x4,d1
	andi.w		#0xf,d1		/*  extract index regno */

/*  count on fetch_dreg() not to alter a0... */
	bsr.l		fetch_dreg		/*  fetch index */

	move.l		d2,-(sp)		/*  save d2 */
	move.l		L_SCR1(a6),d2		/*  fetch opword */

	btst		#0xb,d2		/*  is it word or long? */
	bne.b		faii8_long
	ext.l		d0			/*  sign extend word index */
faii8_long:
	move.l		d2,d1
	rol.w		#0x7,d1
	andi.l		#0x3,d1		/*  extract scale value */

	lsl.l		d1,d0			/*  shift index by scale */

	extb.l		d2			/*  sign extend displacement */
	add.l		d2,d0			/*  index + disp */
	add.l		d0,a0			/*  An + (index + disp) */

	move.l		(sp)+,d2		/*  restore old d2 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Absolute short: (XXX).W  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;; */
fabs_short:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word		/*  fetch short address */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  return <ea> in a0 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Absolute long: (XXX).L  */
/* ;;;;;;;;;;;;;;;;;;;;;;;; */
fabs_long:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch long address */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,a0			/*  return <ea> in a0 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Program counter indirect w/ displacement: (d16, PC)  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fpc_ind:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word		/*  fetch word displacement */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.w		d0,a0			/*  sign extend displacement */

	add.l		EXC_EXTWPTR(a6),a0	/*  pc + d16 */

/*  _imem_read_word() increased the extwptr by 2. need to adjust here. */
	subq.l		#0x2,a0		/*  adjust <ea> */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  PC indirect w/ index(8-bit displacement): (d8, PC, An)  */
/*  "     "     w/   "  (base displacement): (bd, PC, An)   */
/*  PC memory indirect postindexed: ([bd, PC], Xn, od)      */
/*  PC memory indirect preindexed: ([bd, PC, Xn], od)       */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fpc_ind_ext:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word		/*  fetch ext word */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		EXC_EXTWPTR(a6),a0	/*  put base in a0 */
	subq.l		#0x2,a0		/*  adjust base */

	btst		#0x8,d0		/*  is disp only 8 bits? */
	bne.w		fcalc_mem_ind		/*  calc memory indirect */

	move.l		d0,L_SCR1(a6)		/*  store opword */

	move.l		d0,d1			/*  make extword copy */
	rol.w		#0x4,d1		/*  rotate reg num into place */
	andi.w		#0xf,d1		/*  extract register number */

/*  count on fetch_dreg() not to alter a0... */
	bsr.l		fetch_dreg		/*  fetch index */

	move.l		d2,-(sp)		/*  save d2 */
	move.l		L_SCR1(a6),d2		/*  fetch opword */

	btst		#0xb,d2		/*  is index word or long? */
	bne.b		fpii8_long		/*  long */
	ext.l		d0			/*  sign extend word index */
fpii8_long:
	move.l		d2,d1
	rol.w		#0x7,d1		/*  rotate scale value into place */
	andi.l		#0x3,d1		/*  extract scale value */

	lsl.l		d1,d0			/*  shift index by scale */

	extb.l		d2			/*  sign extend displacement */
	add.l		d2,d0			/*  disp + index */
	add.l		d0,a0			/*  An + (index + disp) */

	move.l		(sp)+,d2		/*  restore temp register */
	rts

/*  d2 = index */
/*  d3 = base */
/*  d4 = od */
/*  d5 = extword */
fcalc_mem_ind:
	btst		#0x6,d0		/*  is the index suppressed? */
	beq.b		fcalc_index

	movem.l		d2-d5,-(sp)		/*  save d2-d5 */

	move.l		d0,d5			/*  put extword in d5 */
	move.l		a0,d3			/*  put base in d3 */

	clr.l		d2			/*  yes, so index = 0 */
	bra.b		fbase_supp_ck

/*  index: */
fcalc_index:
	move.l		d0,L_SCR1(a6)		/*  save d0 (opword) */
	bfextu		d0{16:4},d1			/*  fetch dreg index */
	bsr.l		fetch_dreg

	movem.l		d2-d5,-(sp)		/*  save d2-d5 */
	move.l		d0,d2			/*  put index in d2 */
	move.l		L_SCR1(a6),d5
	move.l		a0,d3

	btst		#0xb,d5		/*  is index word or long? */
	bne.b		fno_ext
	ext.l		d2

fno_ext:
	bfextu		d5{21:2},d0
	lsl.l		d0,d2

/*  base address (passed as parameter in d3): */
/*  we clear the value here if it should actually be suppressed. */
fbase_supp_ck:
	btst		#0x7,d5		/*  is the bd suppressed? */
	beq.b		fno_base_sup
	clr.l		d3

/*  base displacement: */
fno_base_sup:
	bfextu		d5{26:2},d0			/*  get bd size */
/* 	beq.l		fmovem_error		; if (size == 0) it's reserved */

	cmpi.b		#0x2,d0
	blt.b		fno_bd
	beq.b		fget_word_bd

	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long

	tst.l		d1			/*  did ifetch fail? */
	bne.l		fcea_iacc		/*  yes */

	bra.b		fchk_ind

fget_word_bd:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		fcea_iacc		/*  yes */

	ext.l		d0			/*  sign extend bd */

fchk_ind:
	add.l		d0,d3			/*  base += bd */

/*  outer displacement: */
fno_bd:
	bfextu		d5{30:2},d0		/*  is od suppressed? */
	beq.w		faii_bd

	cmpi.b		#0x2,d0
	blt.b		fnull_od
	beq.b		fword_od

	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long

	tst.l		d1			/*  did ifetch fail? */
	bne.l		fcea_iacc		/*  yes */

	bra.b		fadd_them

fword_od:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word

	tst.l		d1			/*  did ifetch fail? */
	bne.l		fcea_iacc		/*  yes */

	ext.l		d0			/*  sign extend od */
	bra.b		fadd_them

fnull_od:
	clr.l		d0

fadd_them:
	move.l		d0,d4

	btst		#0x2,d5		/*  pre or post indexing? */
	beq.b		fpre_indexed

	move.l		d3,a0
	bsr.l		_dmem_read_long

	tst.l		d1			/*  did dfetch fail? */
	bne.w		fcea_err		/*  yes */

	add.l		d2,d0			/*  <ea> += index */
	add.l		d4,d0			/*  <ea> += od */
	bra.b		fdone_ea

fpre_indexed:
	add.l		d2,d3			/*  preindexing */
	move.l		d3,a0
	bsr.l		_dmem_read_long

	tst.l		d1			/*  did dfetch fail? */
	bne.w		fcea_err		/*  yes */

	add.l		d4,d0			/*  ea += od */
	bra.b		fdone_ea

faii_bd:
	add.l		d2,d3			/*  ea = (base + bd) + index */
	move.l		d3,d0
fdone_ea:
	move.l		d0,a0

	movem.l		(sp)+,d2-d5		/*  restore d2-d5 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
fcea_err:
	move.l		d3,a0

	movem.l		(sp)+,d2-d5		/*  restore d2-d5 */
	move.w		#0x0101,d0
	bra.l		iea_dacc

fcea_iacc:
	movem.l		(sp)+,d2-d5		/*  restore d2-d5 */
	bra.l		iea_iacc

fmovem_out_err:
	bsr.l		restore
	move.w		#0x00e1,d0
	bra.b		fmovem_err

fmovem_in_err:
	bsr.l		restore
	move.w		#0x0161,d0

fmovem_err:
	move.l		L_SCR1(a6),a0
	bra.l		iea_dacc

