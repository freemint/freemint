/*
 *  XDEF ****************************************************************	
 * 	_fpsp_fline(): 060FPSP entry point for "Line F emulator" exc.	
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	"Line F Emulator" exception in an operating system.		
 * 									
 *  XREF ****************************************************************	
 * 	_fpsp_unimp() - handle "FP Unimplemented" exceptions		
 * 	_real_fpu_disabled() - handle "FPU disabled" exceptions		
 * 	_real_fline() - handle "FLINE" exceptions			
 * 	_imem_read_long() - read instruction longword			
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains a "Line F Emulator" exception	
 * 	  stack frame.							
 * 									
 *  OUTPUT **************************************************************	
 * 	- The system stack is unchanged					
 * 									
 *  ALGORITHM ***********************************************************	
 * 	When a "Line F Emulator" exception occurs, there are 3 possible	
 *  exception types, denoted by the exception stack frame format number:	
 * 	(1) FPU unimplemented instruction (6 word stack frame)		
 * 	(2) FPU disabled (8 word stack frame)				
 * 	(3) Line F (4 word stack frame)					
 * 									
 * 	This module determines which and forks the flow off to the	
 *  appropriate "callout" (for "disabled" and "Line F") or to the		
 *  correct emulation code (for "FPU unimplemented").			
 * 	This code also must check for "fmovecr" instructions w/ a	
 *  non-zero <ea> field. These may get flagged as "Line F" but should	
 *  really be flagged as "FPU Unimplemented". (This is a "feature" on	
 *  the '060.								
 * 									
 */

	.include "hdr.fpu"

	.xref _real_fline
	.xref _real_fpu_disabled
	.xref _fpsp_unimp
	.xref _imem_read_long

	.text

	.globl		_fpsp_fline
_fpsp_fline:

/*  check to see if this exception is a "FP Unimplemented Instruction" */
/*  exception. if so, branch directly to that handler's entry point. */
	cmpi.w		#0x202c,0x6(sp)
	beq.l		_fpsp_unimp

/*  check to see if the FPU is disabled. if so, jump to the OS entry */
/*  point for that condition. */
	cmpi.w		#0x402c,0x6(sp)
	beq.l		_real_fpu_disabled

/*  the exception was an "F-Line Illegal" exception. we check to see */
/*  if the F-Line instruction is an "fmovecr" w/ a non-zero <ea>. if */
/*  so, convert the F-Line exception stack frame to an FP Unimplemented */
/*  Instruction exception stack frame else branch to the OS entry */
/*  point for the F-Line exception handler. */
	link.w		a6,#-LOCAL_SIZE	/*  init stack frame */

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	/*  save d0-d1/a0-a1 */

	move.l		EXC_PC(a6),EXC_EXTWPTR(a6)
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch instruction words */

	bfextu		d0{0:10},d1		/*  is it an fmovecr? */
	cmpi.w		#0x03c8,d1
	bne.b		fline_fline		/*  no */

	bfextu		d0{16:6},d1		/*  is it an fmovecr? */
	cmpi.b		#0x17,d1
	bne.b		fline_fline		/*  no */

/*
 * it's an fmovecr w/ a non-zero <ea> that has entered through
 * the F-Line Illegal exception.
 * so, we need to convert the F-Line exception stack frame into an
 * FP Unimplemented Instruction stack frame and jump to that entry
 * point.
 *
 * but, if the FPU is disabled, then we need to jump to the FPU disabled
 * entry point.
 */
	.dc.w 0x4e7a,0x0808 /* movec		pcr,d0 */
	btst		#0x1,d0
	beq.b		fline_fmovecr

	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	subq.l		#0x8,sp		/*  make room for "Next PC", <ea> */
	move.w		0x8(sp),(sp)
	move.l		0xa(sp),0x2(sp)	/*  move "Current PC" */
	move.w		#0x402c,0x6(sp)
	move.l		0x2(sp),0xc(sp)
	addq.l		#0x4,0x2(sp)		/*  set "Next PC" */

	bra.l		_real_fpu_disabled

fline_fmovecr:
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	fmove.l		0x2(sp),fpiar		/*  set current PC */
	addq.l		#0x4,0x2(sp)		/*  set Next PC */

	move.l		(sp),-(sp)
	move.l		0x8(sp),0x4(sp)
	move.b		#0x20,0x6(sp)

	bra.l		_fpsp_unimp

fline_fline:
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	bra.l		_real_fline

