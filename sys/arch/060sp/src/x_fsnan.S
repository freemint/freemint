/*
 *  XDEF ****************************************************************	
 * 	_fpsp_snan(): 060FPSP entry point for FP SNAN exception.	
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	FP Signalling NAN exception in an operating system.		
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_long() - read instruction longword			
 * 	fix_skewed_ops() - adjust src operand in fsave frame		
 * 	_real_snan() - "callout" to operating system SNAN handler	
 * 	_dmem_write_{byte,word,long}() - store data to mem (opclass 3)	
 * 	store_dreg_{b,w,l}() - store data to data regfile (opclass 3)	
 * 	facc_out_{b,w,l,d,x}() - store to mem took acc error (opcl 3)	
 * 	_calc_ea_fout() - fix An if <ea> is -() or ()+ *  also get <ea>	
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the FP SNAN exception frame		
 * 	- The fsave frame contains the source operand			
 * 									
 *  OUTPUT **************************************************************	
 * 	No access error:						
 * 	- The system stack is unchanged					
 * 	- The fsave frame contains the adjusted src op for opclass 0,2	
 * 									
 *  ALGORITHM ***********************************************************	
 * 	In a system where the FP SNAN exception is enabled, the goal	
 *  is to get to the handler specified at _real_snan(). But, on the 060,	
 *  for opclass zero and two instructions taking this exception, the	
 *  input operand in the fsave frame may be incorrect for some cases	
 *  and needs to be corrected. This handler calls fix_skewed_ops() to	
 *  do just this and then exits through _real_snan().			
 * 	For opclass 3 instructions, the 060 doesn't store the default	
 *  SNAN result out to memory or data register file as it should.		
 *  This code must emulate the move out before finally exiting through	
 *  _real_snan(). The move out, if to memory, is performed using		
 *  _mem_write() "callout" routines that may return a failing result.	
 *  In this special case, the handler must exit through facc_out()	
 *  which creates an access error stack frame from the current SNAN	
 *  stack frame.								
 * 	For the case of an extended precision opclass 3 instruction,	
 *  if the effective addressing mode was -() or ()+, then the address	
 *  register must get updated by calling _calc_ea_fout(). If the <ea>	
 *  was -(a7) from supervisor mode, then the exception frame currently	
 *  on the system stack must be carefully moved "down" to make room	
 *  for the operand being moved.						
 * 									
 */

	.include "hdr.fpu"

	.xref _real_trace
	.xref _real_access
	.xref _real_snan
	.xref _fpsp_done
	.xref set_tag_x
	.xref tbl_unsupp
	.xref store_fpreg
	.xref unnorm_fix
	.xref _imem_read_long
	.xref _dmem_write
	.xref _dmem_write_b
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
	.xref facc_out_d
	.xref facc_out_x
	.xref _calc_ea_fout

	.text

	.globl		_fpsp_snan
_fpsp_snan:

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
	bne.w		fsnan_out		/*  fmove out */


/*  here, we simply see if the operand in the fsave frame needs to be "unskewed". */
/*  this would be the case for opclass two operations with a source infinity or */
/*  denorm operand in the sgl or dbl format. NANs also become skewed and must be */
/*  fixed here. */
	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		fix_skewed_ops		/*  fix src op */

fsnan_exit:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)

	unlk		a6
	bra.l		_real_snan

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * the hardware does not save the default result to memory on enabled
 * snan exceptions. we do this here before passing control to
 * the user snan handler.
 *
 * byte, word, long, and packed destination format operations can pass
 * through here. since packed format operations already were handled by
 * fpsp_unsupp(), then we need to do nothing else for them here.
 * for byte, word, and long, we simply need to test the sign of the src
 * operand and save the appropriate minimum or maximum integer value
 * to the effective address as pointed to by the stacked effective address.
 */
fsnan_out:

	bfextu		d0{19:3},d0		/*  extract dst format field */
	move.b		1+EXC_OPWORD(a6),d1	/*  extract <ea> mode,reg */
	move.w		(tbl_snan.b,pc,d0.w*2),a0
	jmp		(tbl_snan.b,pc,a0.l)

tbl_snan:
	.dc.w		fsnan_out_l - tbl_snan /*  long word integer */
	.dc.w		fsnan_out_s - tbl_snan /*  sgl prec shouldn't happen */
	.dc.w		fsnan_out_x - tbl_snan /*  ext prec shouldn't happen */
	.dc.w		tbl_snan    - tbl_snan /*  packed needs no help */
	.dc.w		fsnan_out_w - tbl_snan /*  word integer */
	.dc.w		fsnan_out_d - tbl_snan /*  dbl prec shouldn't happen */
	.dc.w		fsnan_out_b - tbl_snan /*  byte integer */
	.dc.w		tbl_snan    - tbl_snan /*  packed needs no help */

fsnan_out_b:
	move.b		FP_SRC_HI(a6),d0	/*  load upper byte of SNAN */
	bset		#6,d0			/*  set SNAN bit */
	cmpi.b		#0x7,d1		/*  is <ea> mode a data reg? */
	ble.b		fsnan_out_b_dn		/*  yes */
	move.l		EXC_EA(a6),a0		/*  pass: <ea> of default result */
	bsr.l		_dmem_write_byte	/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_b		/*  yes */

	bra.w		fsnan_exit
fsnan_out_b_dn:
	andi.w		#0x0007,d1
	bsr.l		store_dreg_b		/*  store result to regfile */
	bra.w		fsnan_exit

fsnan_out_w:
	move.w		FP_SRC_HI(a6),d0	/*  load upper word of SNAN */
	bset		#14,d0			/*  set SNAN bit */
	cmpi.b		#0x7,d1		/*  is <ea> mode a data reg? */
	ble.b		fsnan_out_w_dn		/*  yes */
	move.l		EXC_EA(a6),a0		/*  pass: <ea> of default result */
	bsr.l		_dmem_write_word	/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_w		/*  yes */

	bra.w		fsnan_exit
fsnan_out_w_dn:
	andi.w		#0x0007,d1
	bsr.l		store_dreg_w		/*  store result to regfile */
	bra.w		fsnan_exit

fsnan_out_l:
	move.l		FP_SRC_HI(a6),d0	/*  load upper longword of SNAN */
	bset		#30,d0			/*  set SNAN bit */
	cmpi.b		#0x7,d1		/*  is <ea> mode a data reg? */
	ble.b		fsnan_out_l_dn		/*  yes */
	move.l		EXC_EA(a6),a0		/*  pass: <ea> of default result */
	bsr.l		_dmem_write_long	/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_l		/*  yes */

	bra.w		fsnan_exit
fsnan_out_l_dn:
	andi.w		#0x0007,d1
	bsr.l		store_dreg_l		/*  store result to regfile */
	bra.w		fsnan_exit

fsnan_out_s:
	cmpi.b		#0x7,d1		/*  is <ea> mode a data reg? */
	ble.b		fsnan_out_d_dn		/*  yes */
	move.l		FP_SRC_EX(a6),d0	/*  fetch SNAN sign */
	andi.l		#0x80000000,d0		/*  keep sign */
	ori.l		#0x7fc00000,d0		/*  insert new exponent,SNAN bit */
	move.l		FP_SRC_HI(a6),d1	/*  load mantissa */
	lsr.l		#0x8,d1		/*  shift mantissa for sgl */
	or.l		d1,d0			/*  create sgl SNAN */
	move.l		EXC_EA(a6),a0		/*  pass: <ea> of default result */
	bsr.l		_dmem_write_long	/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_l		/*  yes */

	bra.w		fsnan_exit
fsnan_out_d_dn:
	move.l		FP_SRC_EX(a6),d0	/*  fetch SNAN sign */
	andi.l		#0x80000000,d0		/*  keep sign */
	ori.l		#0x7fc00000,d0		/*  insert new exponent,SNAN bit */
	move.l		d1,-(sp)
	move.l		FP_SRC_HI(a6),d1	/*  load mantissa */
	lsr.l		#0x8,d1		/*  shift mantissa for sgl */
	or.l		d1,d0			/*  create sgl SNAN */
	move.l		(sp)+,d1
	andi.w		#0x0007,d1
	bsr.l		store_dreg_l		/*  store result to regfile */
	bra.w		fsnan_exit

fsnan_out_d:
	move.l		FP_SRC_EX(a6),d0	/*  fetch SNAN sign */
	andi.l		#0x80000000,d0		/*  keep sign */
	ori.l		#0x7ff80000,d0		/*  insert new exponent,SNAN bit */
	move.l		FP_SRC_HI(a6),d1	/*  load hi mantissa */
	move.l		d0,FP_SCR0_EX(a6)	/*  store to temp space */
	moveq.l		#11,d0			/*  load shift amt */
	lsr.l		d0,d1
	or.l		d1,FP_SCR0_EX(a6)	/*  create dbl hi */
	move.l		FP_SRC_HI(a6),d1	/*  load hi mantissa */
	andi.l		#0x000007ff,d1
	ror.l		d0,d1
	move.l		d1,FP_SCR0_HI(a6)	/*  store to temp space */
	move.l		FP_SRC_LO(a6),d1	/*  load lo mantissa */
	lsr.l		d0,d1
	or.l		d1,FP_SCR0_HI(a6)	/*  create dbl lo */
	lea		FP_SCR0(a6),a0	/*  pass: ptr to operand */
	move.l		EXC_EA(a6),a1		/*  pass: dst addr */
	moveq.l		#0x8,d0		/*  pass: size of 8 bytes */
	bsr.l		_dmem_write		/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_d		/*  yes */

	bra.w		fsnan_exit

/*  for extended precision, if the addressing mode is pre-decrement or */
/*  post-increment, then the address register did not get updated. */
/*  in addition, for pre-decrement, the stacked <ea> is incorrect. */
fsnan_out_x:
	clr.b		SPCOND_FLG(a6)		/*  clear special case flag */

	move.w		FP_SRC_EX(a6),FP_SCR0_EX(a6)
	clr.w		2+FP_SCR0(a6)
	move.l		FP_SRC_HI(a6),d0
	bset		#30,d0
	move.l		d0,FP_SCR0_HI(a6)
	move.l		FP_SRC_LO(a6),FP_SCR0_LO(a6)

	btst		#0x5,EXC_SR(a6)	/*  supervisor mode exception? */
	bne.b		fsnan_out_x_s		/*  yes */

	move.l		usp,a0		/*  fetch user stack pointer */
	move.l		a0,EXC_A7(a6)		/*  save on stack for calc_ea() */
	move.l		(a6),EXC_A6(a6)

	bsr.l		_calc_ea_fout		/*  find the correct ea,update An */
	move.l		a0,a1
	move.l		a0,EXC_EA(a6)		/*  stack correct <ea> */

	move.l		EXC_A7(a6),a0
	move.l		a0,usp		/*  restore user stack pointer */
	move.l		EXC_A6(a6),(a6)

fsnan_out_x_save:
	lea		FP_SCR0(a6),a0	/*  pass: ptr to operand */
	moveq.l		#0xc,d0		/*  pass: size of extended */
	bsr.l		_dmem_write		/*  write the default result */

	tst.l		d1			/*  did dstore fail? */
	bne.l		facc_out_x		/*  yes */

	bra.w		fsnan_exit

fsnan_out_x_s:
	move.l		(a6),EXC_A6(a6)

	bsr.l		_calc_ea_fout		/*  find the correct ea,update An */
	move.l		a0,a1
	move.l		a0,EXC_EA(a6)		/*  stack correct <ea> */

	move.l		EXC_A6(a6),(a6)

	cmpi.b		#mda7_flg,SPCOND_FLG(a6) /*  is <ea> mode -(a7)? */
	bne.b		fsnan_out_x_save	/*  no */

/*  the operation was "fmove.x SNAN,-(a7)" from supervisor mode. */
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)

	move.l		EXC_A6(a6),a6		/*  restore frame pointer */

	move.l		LOCAL_SIZE+EXC_SR(sp),LOCAL_SIZE+EXC_SR-0xc(sp)
	move.l		LOCAL_SIZE+EXC_PC+0x2(sp),LOCAL_SIZE+EXC_PC+0x2-0xc(sp)
	move.l		LOCAL_SIZE+EXC_EA(sp),LOCAL_SIZE+EXC_EA-0xc(sp)

	move.l		LOCAL_SIZE+FP_SCR0_EX(sp),LOCAL_SIZE+EXC_SR(sp)
	move.l		LOCAL_SIZE+FP_SCR0_HI(sp),LOCAL_SIZE+EXC_PC+0x2(sp)
	move.l		LOCAL_SIZE+FP_SCR0_LO(sp),LOCAL_SIZE+EXC_EA(sp)

	add.l		#LOCAL_SIZE-0x8,sp

	bra.l		_real_snan

