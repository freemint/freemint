/*
 *  XDEF ****************************************************************	
 * 	_fpsp_dz(): 060FPSP entry point for FP DZ exception.		
 * 									
 * 	This handler should be the first code executed upon taking	
 * 	the FP DZ exception in an operating system.			
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_long() - read instruction longword from memory	
 * 	fix_skewed_ops() - adjust fsave operand				
 * 	_real_dz() - "callout" exit point from FP DZ handler		
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the FP DZ exception stack.		
 * 	- The fsave frame contains the source operand.			
 * 									
 *  OUTPUT **************************************************************	
 * 	- The system stack contains the FP DZ exception stack.		
 * 	- The fsave frame contains the adjusted source operand.		
 * 									
 *  ALGORITHM ***********************************************************	
 * 	In a system where the DZ exception is enabled, the goal is to	
 *  get to the handler specified at _real_dz(). But, on the 060, when the	
 *  exception is taken, the input operand in the fsave state frame may	
 *  be incorrect for some cases and need to be adjusted. So, this package	
 *  adjusts the operand using fix_skewed_ops() and then branches to	
 *  _real_dz().								
 * 									
 */

	.include "hdr.fpu"

	.xref _real_dz
	.xref fix_skewed_ops
	.xref _imem_read_long

	.text

	.globl		_fpsp_dz
_fpsp_dz:

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


/*  here, we simply see if the operand in the fsave frame needs to be "unskewed". */
/*  this would be the case for opclass two operations with a source zero */
/*  in the sgl or dbl format. */
	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		fix_skewed_ops		/*  fix src op */

fdz_exit:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)

	unlk		a6
	bra.l		_real_dz
