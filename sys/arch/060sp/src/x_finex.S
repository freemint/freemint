/*
 *  XDEF ****************************************************************	
 * 	_fpsp_inex(): 060FPSP entry point for FP Inexact exception.	
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	FP Inexact exception in an operating system.			
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_long() - read instruction longword			
 * 	fix_skewed_ops() - adjust src operand in fsave frame		
 * 	set_tag_x() - determine optype of src/dst operands		
 * 	store_fpreg() - store opclass 0 or 2 result to FP regfile	
 * 	unnorm_fix() - change UNNORM operands to NORM or ZERO		
 * 	load_fpn2() - load dst operand from FP regfile			
 * 	smovecr() - emulate an "fmovecr" instruction			
 * 	fout() - emulate an opclass 3 instruction			
 * 	tbl_unsupp - add of table of emulation routines for opclass 0,2	
 * 	_real_inex() - "callout" to operating system inexact handler	
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the FP Inexact exception frame	
 * 	- The fsave frame contains the source operand			
 * 									
 *  OUTPUT **************************************************************	
 * 	- The system stack is unchanged					
 * 	- The fsave frame contains the adjusted src op for opclass 0,2	
 * 									
 *  ALGORITHM ***********************************************************	
 * 	In a system where the FP Inexact exception is enabled, the goal	
 *  is to get to the handler specified at _real_inex(). But, on the 060,	
 *  for opclass zero and two instruction taking this exception, the	
 *  hardware doesn't store the correct result to the destination FP	
 *  register as did the '040 and '881/2. This handler must emulate the	
 *  instruction in order to get this value and then store it to the	
 *  correct register before calling _real_inex().				
 * 	For opclass 3 instructions, the 060 doesn't store the default	
 *  inexact result out to memory or data register file as it should.	
 *  This code must emulate the move out by calling fout() before finally	
 *  exiting through _real_inex().						
 * 									
 */

	.include "hdr.fpu"

	.xref _real_trace
	.xref _real_access
	.xref _real_inex
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
	.xref load_fpn2
	.xref fout
	.xref smovecr

	.text

	.globl		_fpsp_inex
_fpsp_inex:

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
	bne.w		finex_out		/*  fmove out */


/*  the hardware, for "fabs" and "fneg" w/ a long source format, puts the */
/*  longword integer directly into the upper longword of the mantissa along */
/*  w/ an exponent value of 0x401e. we convert this to extended precision here. */
	bfextu		d0{19:3},d0		/*  fetch instr size */
	bne.b		finex_cont		/*  instr size is not long */
	cmpi.w		#0x401e,FP_SRC_EX(a6)	/*  is exponent 0x401e? */
	bne.b		finex_cont		/*  no */
	fmove.l		#0x0,fpcr
	fmove.l		FP_SRC_HI(a6),fp0	/*  load integer src */
	fmove.x		fp0,FP_SRC(a6)	/*  store integer as extended precision */
	move.w		#0xe001,0x2+FP_SRC(a6)

finex_cont:
	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		fix_skewed_ops		/*  fix src op */

/*  Here, we zero the ccode and exception byte field since we're going to */
/*  emulate the whole instruction. Notice, though, that we don't kill the */
/*  INEX1 bit. This is because a packed op has long since been converted */
/*  to extended before arriving here. Therefore, we need to retain the */
/*  INEX1 bit from when the operand was first converted. */
	andi.l		#0x00ff01ff,USER_FPSR(a6) /*  zero all but accured field */

	fmove.l		#0x0,fpcr		/*  zero current control regs */
	fmove.l		#0x0,fpsr

	bfextu		EXC_EXTWORD(a6){0:6},d1 /*  extract upper 6 of cmdreg */
	cmpi.b		#0x17,d1		/*  is op an fmovecr? */
	beq.w		finex_fmovecr		/*  yes */

	lea		FP_SRC(a6),a0		/*  pass: ptr to src op */
	bsr.l		set_tag_x		/*  tag the operand type */
	move.b		d0,STAG(a6)		/*  maybe NORM,DENORM */

/*  bits four and five of the fp extension word separate the monadic and dyadic */
/*  operations that can pass through fpsp_inex(). remember that fcmp and ftst */
/*  will never take this exception, but fsincos will. */
	btst		#0x5,1+EXC_CMDREG(a6)	/*  is operation monadic or dyadic? */
	beq.b		finex_extract		/*  monadic */

	btst		#0x4,1+EXC_CMDREG(a6)	/*  is operation an fsincos? */
	bne.b		finex_extract		/*  yes */

	bfextu		EXC_CMDREG(a6){6:3},d0 /*  dyadic; load dst reg */
	bsr.l		load_fpn2		/*  load dst into FP_DST */

	lea		FP_DST(a6),a0		/*  pass: ptr to dst op */
	bsr.l		set_tag_x		/*  tag the operand type */
	cmpi.b		#UNNORM,d0		/*  is operand an UNNORM? */
	bne.b		finex_op2_done		/*  no */
	bsr.l		unnorm_fix		/*  yes; convert to NORM,DENORM,or ZERO */
finex_op2_done:
	move.b		d0,DTAG(a6)		/*  save dst optype tag */

finex_extract:
	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  pass rnd prec/mode */

	move.b		1+EXC_CMDREG(a6),d1
	andi.w		#0x007f,d1		/*  extract extension */

	lea		FP_SRC(a6),a0
	lea		FP_DST(a6),a1

	move.l		(tbl_unsupp.l,pc,d1.w*4),d1 /*  fetch routine addr */
	jsr		(tbl_unsupp.l,pc,d1.l*1)

/*  the operation has been emulated. the result is in fp0. */
finex_save:
	bfextu		EXC_CMDREG(a6){6:3},d0
	bsr.l		store_fpreg

finex_exit:
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)

	unlk		a6
	bra.l		_real_inex

finex_fmovecr:
	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  pass rnd prec,mode */
	move.b		1+EXC_CMDREG(a6),d1
	andi.l		#0x0000007f,d1		/*  pass rom offset */
	bsr.l		smovecr
	bra.b		finex_save

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * the hardware does not save the default result to memory on enabled
 * inexact exceptions. we do this here before passing control to
 * the user inexact handler.
 *
 * byte, word, and long destination format operations can pass
 * through here. so can double and single precision.
 * although packed opclass three operations can take inexact
 * exceptions, they won't pass through here since they are caught
 * first by the unsupported data format exception handler. that handler
 * sends them directly to _real_inex() if necessary.
 */
finex_out:

	move.b		#NORM,STAG(a6)		/*  src is a NORM */

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  pass rnd prec,mode */

	andi.l		#0xffff00ff,USER_FPSR(a6) /*  zero exception field */

	lea		FP_SRC(a6),a0		/*  pass ptr to src operand */

	bsr.l		fout			/*  store the default result */

	bra.b		finex_exit

