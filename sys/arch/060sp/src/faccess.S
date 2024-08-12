/*
 *  XDEF ****************************************************************	
 * 	facc_in_b(): dmem_read_byte failed				
 * 	facc_in_w(): dmem_read_word failed				
 * 	facc_in_l(): dmem_read_long failed				
 * 	facc_in_d(): dmem_read of dbl prec failed			
 * 	facc_in_x(): dmem_read of ext prec failed			
 * 									
 * 	facc_out_b(): dmem_write_byte failed				
 * 	facc_out_w(): dmem_write_word failed				
 * 	facc_out_l(): dmem_write_long failed				
 * 	facc_out_d(): dmem_write of dbl prec failed			
 * 	facc_out_x(): dmem_write of ext prec failed			
 * 									
 *  XREF ****************************************************************	
 * 	_real_access() - exit through access error handler		
 * 									
 *  INPUT ***************************************************************	
 * 	None								
 * 									
 *  OUTPUT **************************************************************	
 * 	None								
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Flow jumps here when an FP data fetch call gets an error	
 *  result. This means the operating system wants an access error frame	
 *  made out of the current exception stack frame.			
 * 	So, we first call restore() which makes sure that any updated	
 *  -(an)+ register gets returned to its pre-exception value and then	
 *  we change the stack to an access error stack frame.			
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl facc_in_b
facc_in_b:
	moveq.l		#0x1,d0			/*  one byte */
	bsr.w		restore				/*  fix An */

	move.w		#0x0121,EXC_VOFF(a6)		/*  set FSLW */
	bra.w		facc_finish

	.globl facc_in_w
facc_in_w:
	moveq.l		#0x2,d0			/*  two bytes */
	bsr.w		restore				/*  fix An */

	move.w		#0x0141,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

	.globl facc_in_l
facc_in_l:
	moveq.l		#0x4,d0			/*  four bytes */
	bsr.w		restore				/*  fix An */

	move.w		#0x0101,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

	.globl facc_in_d
facc_in_d:
	moveq.l		#0x8,d0			/*  eight bytes */
	bsr.w		restore				/*  fix An */

	move.w		#0x0161,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

	.globl facc_in_x
facc_in_x:
	moveq.l		#0xc,d0			/*  twelve bytes */
	bsr.w		restore				/*  fix An */

	move.w		#0x0161,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl facc_out_b
facc_out_b:
	moveq.l		#0x1,d0			/*  one byte */
	bsr.w		restore				/*  restore An */

	move.w		#0x00a1,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

	.globl facc_out_w
facc_out_w:
	moveq.l		#0x2,d0			/*  two bytes */
	bsr.w		restore				/*  restore An */

	move.w		#0x00c1,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

	.globl facc_out_l
facc_out_l:
	moveq.l		#0x4,d0			/*  four bytes */
	bsr.w		restore				/*  restore An */

	move.w		#0x0081,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

	.globl facc_out_d
facc_out_d:
	moveq.l		#0x8,d0			/*  eight bytes */
	bsr.w		restore				/*  restore An */

	move.w		#0x00e1,EXC_VOFF(a6)		/*  set FSLW */
	bra.b		facc_finish

	.globl facc_out_x
facc_out_x:
	moveq.l		#0xc,d0			/*  twelve bytes */
	bsr.w		restore				/*  restore An */

	move.w		#0x00e1,EXC_VOFF(a6)		/*  set FSLW */

/*  here's where we actually create the access error frame from the */
/*  current exception stack frame. */
	.globl facc_finish
facc_finish:
	move.l		USER_FPIAR(a6),EXC_PC(a6) /*  store current PC */

	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	move.l		(sp),-(sp)		/*  store SR, hi(PC) */
	move.l		0x8(sp),0x4(sp)	/*  store lo(PC) */
	move.l		0xc(sp),0x8(sp)	/*  store EA */
	move.l		#0x00000001,0xc(sp)	/*  store FSLW */
	move.w		0x6(sp),0xc(sp)	/*  fix FSLW (size) */
	move.w		#0x4008,0x6(sp)	/*  store voff */

	btst		#0x5,(sp)		/*  supervisor or user mode? */
	beq.b		facc_out2		/*  user */
	bset		#0x2,0xd(sp)		/*  set supervisor TM bit */

facc_out2:
	bra.l		_real_access

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  if the effective addressing mode was predecrement or postincrement, */
/*  the emulation has already changed its value to the correct post- */
/*  instruction value. but since we're exiting to the access error */
/*  handler, then AN must be returned to its pre-instruction value. */
/*  we do that here. */
	.globl restore
restore:
	move.b		EXC_OPWORD+0x1(a6),d1
	andi.b		#0x38,d1		/*  extract opmode */
	cmpi.b		#0x18,d1		/*  postinc? */
	beq.w		rest_inc
	cmpi.b		#0x20,d1		/*  predec? */
	beq.w		rest_dec
	rts

rest_inc:
	move.b		EXC_OPWORD+0x1(a6),d1
	andi.w		#0x0007,d1		/*  fetch An */

	move.w		(tbl_rest_inc.b,pc,d1.w*2),d1
	jmp		(tbl_rest_inc.b,pc,d1.w*1)

tbl_rest_inc:
	.dc.w		ri_a0 - tbl_rest_inc
	.dc.w		ri_a1 - tbl_rest_inc
	.dc.w		ri_a2 - tbl_rest_inc
	.dc.w		ri_a3 - tbl_rest_inc
	.dc.w		ri_a4 - tbl_rest_inc
	.dc.w		ri_a5 - tbl_rest_inc
	.dc.w		ri_a6 - tbl_rest_inc
	.dc.w		ri_a7 - tbl_rest_inc

ri_a0:
	sub.l		d0,EXC_DREGS+0x8(a6)	/*  fix stacked a0 */
	rts
ri_a1:
	sub.l		d0,EXC_DREGS+0xc(a6)	/*  fix stacked a1 */
	rts
ri_a2:
	sub.l		d0,a2			/*  fix a2 */
	rts
ri_a3:
	sub.l		d0,a3			/*  fix a3 */
	rts
ri_a4:
	sub.l		d0,a4			/*  fix a4 */
	rts
ri_a5:
	sub.l		d0,a5			/*  fix a5 */
	rts
ri_a6:
	sub.l		d0,(a6)		/*  fix stacked a6 */
	rts
/*  if it's a fmove out instruction, we don't have to fix a7 */
/*  because we hadn't changed it yet. if it's an opclass two */
/*  instruction (data moved in) and the exception was in supervisor */
/*  mode, then also also wasn't updated. if it was user mode, then */
/*  restore the correct a7 which is in the USP currently. */
ri_a7:
	cmpi.b		#0x30,EXC_VOFF(a6)	/*  move in or out? */
	bne.b		ri_a7_done		/*  out */

	btst		#0x5,EXC_SR(a6)	/*  user or supervisor? */
	bne.b		ri_a7_done		/*  supervisor */
	movec		usp,a0		/*  restore USP */
	sub.l		d0,a0
	movec		a0,usp
ri_a7_done:
	rts

/*  need to invert adjustment value if the <ea> was predec */
rest_dec:
	neg.l		d0
	bra.b		rest_inc

    /* .balignw 16,0 */
