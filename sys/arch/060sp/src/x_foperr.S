/*
 *  XDEF ****************************************************************	
 * 	_fpsp_operr(): 060FPSP entry point for FP Operr exception.	
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	FP Operand Error exception in an operating system.		
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_long() - read instruction longword			
 * 	fix_skewed_ops() - adjust src operand in fsave frame		
 * 	_real_operr() - "callout" to operating system operr handler	
 * 	_dmem_write_{byte,word,long}() - store data to mem (opclass 3)	
 * 	store_dreg_{b,w,l}() - store data to data regfile (opclass 3)	
 * 	facc_out_{b,w,l}() - store to memory took access error (opcl 3)	
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the FP Operr exception frame	
 * 	- The fsave frame contains the source operand			
 * 									
 *  OUTPUT **************************************************************	
 * 	No access error:						
 * 	- The system stack is unchanged					
 * 	- The fsave frame contains the adjusted src op for opclass 0,2	
 * 									
 *  ALGORITHM ***********************************************************	
 * 	In a system where the FP Operr exception is enabled, the goal	
 *  is to get to the handler specified at _real_operr(). But, on the 060,	
 *  for opclass zero and two instruction taking this exception, the	
 *  input operand in the fsave frame may be incorrect for some cases	
 *  and needs to be corrected. This handler calls fix_skewed_ops() to	
 *  do just this and then exits through _real_operr().			
 * 	For opclass 3 instructions, the 060 doesn't store the default	
 *  operr result out to memory or data register file as it should.	
 *  This code must emulate the move out before finally exiting through	
 *  _real_inex(). The move out, if to memory, is performed using		
 *  _mem_write() "callout" routines that may return a failing result.	
 *  In this special case, the handler must exit through facc_out()	
 *  which creates an access error stack frame from the current operr	
 *  stack frame.								
 * 									
 */

	.include "hdr.fpu"

	.xref _real_trace
	.xref _real_access
	.xref _real_operr
	.xref _fpsp_done
	.xref set_tag_x
	.xref tbl_unsupp
	.xref store_fpreg
	.xref unnorm_fix
	.xref _imem_read_long
	.xref _dmem_write_byte
	.xref _dmem_write_word
	.xref _dmem_write_long
	.xref store_dreg_b
	.xref store_dreg_w
	.xref store_dreg_l
	.xref _imem_read
	.xref fix_skewed_ops
	.xref facc_out_b
	.xref facc_out_w
	.xref facc_out_l

	.text

	.globl		_fpsp_operr
_fpsp_operr:

	link.w		a6,#-LOCAL_SIZE	/*  init stack frame */

	fsave		FP_SRC(a6)		/*  grab the "busy" frame */

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	/*  save d0-d1/a0-a1 */
	fmovem.l		fpcr/fpsr/fpiar,USER_FPCR(a6) /*  save ctrl regs */
	fmovem.x		fp0-fp1,EXC_FPREGS(a6)	/*  save fp0-fp1 on stack */

/*  the FPIAR holds the "current PC" of the faulting instruction */
	move.l		USER_FPIAR(a6),EXC_EXTWPTR(a6)

	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch the instruction words */
	move.l		d0,EXC_OPWORD(a6)

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	btst		#13,d0			/*  is instr an fmove out? */
	bne.b		foperr_out		/*  fmove out */


/*  here, we simply see if the operand in the fsave frame needs to be "unskewed". */
/*  this would be the case for opclass two operations with a source infinity or */
/*  denorm operand in the sgl or dbl format. NANs also become skewed, but can't */
/*  cause an operr so we don't need to check for them here. */
	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		fix_skewed_ops		/*  fix src op */

foperr_exit:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)

	unlk		a6
	bra.l		_real_operr

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * the hardware does not save the default result to memory on enabled
 * operand error exceptions. we do this here before passing control to
 * the user operand error handler.
 *
 * byte, word, and long destination format operations can pass
 * through here. we simply need to test the sign of the src
 * operand and save the appropriate minimum or maximum integer value
 * to the effective address as pointed to by the stacked effective address.
 *
 * although packed opclass three operations can take operand error
 * exceptions, they won't pass through here since they are caught
 * first by the unsupported data format exception handler. that handler
 * sends them directly to _real_operr() if necessary.
 */
foperr_out:

	move.w		FP_SRC_EX(a6),d1	/*  fetch exponent */
	andi.w		#0x7fff,d1
	cmpi.w		#0x7fff,d1
	bne.b		foperr_out_not_qnan
/*  the operand is either an infinity or a QNAN. */
	tst.l		FP_SRC_LO(a6)
	bne.b		foperr_out_qnan
	move.l		FP_SRC_HI(a6),d1
	andi.l		#0x7fffffff,d1
	beq.b		foperr_out_not_qnan
foperr_out_qnan:
	move.l		FP_SRC_HI(a6),L_SCR1(a6)
	bra.b		foperr_out_jmp

foperr_out_not_qnan:
	move.l		#0x7fffffff,d1
	tst.b		FP_SRC_EX(a6)
	bpl.b		foperr_out_not_qnan2
	addq.l		#0x1,d1
foperr_out_not_qnan2:
	move.l		d1,L_SCR1(a6)

foperr_out_jmp:
	bfextu		d0{19:3},d0		/*  extract dst format field */
	move.b		1+EXC_OPWORD(a6),d1	/*  extract <ea> mode,reg */
	move.w		(tbl_operr.b,pc,d0.w*2),a0
	jmp		(tbl_operr.b,pc,a0.l)

tbl_operr:
	.dc.w		foperr_out_l - tbl_operr /*  long word integer */
	.dc.w		tbl_operr    - tbl_operr /*  sgl prec shouldn't happen */
	.dc.w		tbl_operr    - tbl_operr /*  ext prec shouldn't happen */
	.dc.w		foperr_exit  - tbl_operr /*  packed won't enter here */
	.dc.w		foperr_out_w - tbl_operr /*  word integer */
	.dc.w		tbl_operr    - tbl_operr /*  dbl prec shouldn't happen */
	.dc.w		foperr_out_b - tbl_operr /*  byte integer */
	.dc.w		tbl_operr    - tbl_operr /*  packed won't enter here */

foperr_out_b:
	move.b		L_SCR1(a6),d0		/*  load positive default result */
	cmpi.b		#0x7,d1		/*  is <ea> mode a data reg? */
	ble.b		foperr_out_b_save_dn	/*  yes */
	move.l		EXC_EA(a6),a0		/*  pass: <ea> of default result */
	bsr.l		_dmem_write_byte	/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_b		/*  yes */

	bra.w		foperr_exit
foperr_out_b_save_dn:
	andi.w		#0x0007,d1
	bsr.l		store_dreg_b		/*  store result to regfile */
	bra.w		foperr_exit

foperr_out_w:
	move.w		L_SCR1(a6),d0		/*  load positive default result */
	cmpi.b		#0x7,d1		/*  is <ea> mode a data reg? */
	ble.b		foperr_out_w_save_dn	/*  yes */
	move.l		EXC_EA(a6),a0		/*  pass: <ea> of default result */
	bsr.l		_dmem_write_word	/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_w		/*  yes */

	bra.w		foperr_exit
foperr_out_w_save_dn:
	andi.w		#0x0007,d1
	bsr.l		store_dreg_w		/*  store result to regfile */
	bra.w		foperr_exit

foperr_out_l:
	move.l		L_SCR1(a6),d0		/*  load positive default result */
	cmpi.b		#0x7,d1		/*  is <ea> mode a data reg? */
	ble.b		foperr_out_l_save_dn	/*  yes */
	move.l		EXC_EA(a6),a0		/*  pass: <ea> of default result */
	bsr.l		_dmem_write_long	/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_l		/*  yes */

	bra.w		foperr_exit
foperr_out_l_save_dn:
	andi.w		#0x0007,d1
	bsr.l		store_dreg_l		/*  store result to regfile */
	bra.w		foperr_exit

