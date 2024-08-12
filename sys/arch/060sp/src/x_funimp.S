/*
 *  XDEF ****************************************************************	
 * 	_fpsp_unimp(): 060FPSP entry point for FP "Unimplemented	
 * 		       Instruction" exception.				
 * 									
 * 	This handler should be the first code executed upon taking the	
 * 	FP Unimplemented Instruction exception in an operating system.	
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_{word,long}() - read instruction word/longword	
 * 	load_fop() - load src/dst ops from memory and/or FP regfile	
 * 	store_fpreg() - store opclass 0 or 2 result to FP regfile	
 * 	tbl_trans - addr of table of emulation routines for trnscndls	
 * 	_real_access() - "callout" for access error exception		
 * 	_fpsp_done() - "callout" for exit *  work all done		
 * 	_real_trace() - "callout" for Trace enabled exception		
 * 	smovecr() - emulate "fmovecr" instruction			
 * 	funimp_skew() - adjust fsave src ops to "incorrect" value	
 * 	_ftrapcc() - emulate an "ftrapcc" instruction			
 * 	_fdbcc() - emulate an "fdbcc" instruction			
 * 	_fscc() - emulate an "fscc" instruction				
 * 	_real_trap() - "callout" for Trap exception			
 * 	_real_bsun() - "callout" for enabled Bsun exception		
 * 									
 *  INPUT ***************************************************************	
 * 	- The system stack contains the "Unimplemented Instr" stk frame	
 * 									
 *  OUTPUT **************************************************************	
 * 	If access error:						
 * 	- The system stack is changed to an access error stack frame	
 * 	If Trace exception enabled:					
 * 	- The system stack is changed to a Trace exception stack frame	
 * 	Else: (normal case)						
 * 	- Correct result has been stored as appropriate			
 * 									
 *  ALGORITHM ***********************************************************	
 * 	There are two main cases of instructions that may enter here to	
 *  be emulated: (1) the FPgen instructions, most of which were also	
 *  unimplemented on the 040, and (2) "ftrapcc", "fscc", and "fdbcc".	
 * 	For the first set, this handler calls the routine load_fop()	
 *  to load the source and destination (for dyadic) operands to be used	
 *  for instruction emulation. The correct emulation routine is then	
 *  chosen by decoding the instruction type and indexing into an		
 *  emulation subroutine index table. After emulation returns, this	
 *  handler checks to see if an exception should occur as a result of the
 *  FP instruction emulation. If so, then an FP exception of the correct	
 *  type is inserted into the FPU state frame using the "frestore"	
 *  instruction before exiting through _fpsp_done(). In either the	
 *  exceptional or non-exceptional cases, we must check to see if the	
 *  Trace exception is enabled. If so, then we must create a Trace	
 *  exception frame from the current exception frame and exit through	
 *  _real_trace().							
 * 	For "fdbcc", "ftrapcc", and "fscc", the emulation subroutines	
 *  _fdbcc(), _ftrapcc(), and _fscc() respectively are used. All three	
 *  may flag that a BSUN exception should be taken. If so, then the	
 *  current exception stack frame is converted into a BSUN exception	
 *  stack frame and an exit is made through _real_bsun(). If the		
 *  instruction was "ftrapcc" and a Trap exception should result, a Trap	
 *  exception stack frame is created from the current frame and an exit	
 *  is made through _real_trap(). If a Trace exception is pending, then	
 *  a Trace exception frame is created from the current frame and a jump	
 *  is made to _real_trace(). Finally, if none of these conditions exist,	
 *  then the handler exits though the callout _fpsp_done().		
 * 									
 * 	In any of the above scenarios, if a _mem_read() or _mem_write()	
 *  "callout" returns a failing value, then an access error stack frame	
 *  is created from the current stack frame and an exit is made through	
 *  _real_access().							
 * 									
 */

/*
 *  FP UNIMPLEMENTED INSTRUCTION STACK FRAME:
 * 
 *      *****************
 *      *               * => <ea> of fp unimp instr.
 *      -      EA       -
 *      *               *
 *      *****************
 *      * 0x2 *  0x02c  * => frame format and vector offset(vector #11)
 *      *****************
 *      *               *
 *      -    Next PC    - => PC of instr to execute after exc handling
 *      *               *
 *      *****************
 *      *      SR       * => SR at the time the exception was taken
 *      *****************
 * 
 *  Note: the !NULL bit does not get set in the fsave frame when the
 *  machine encounters an fp unimp exception. Therefore, it must be set
 *  before leaving this handler.
 */

	.include "hdr.fpu"

	.xref _real_trace
	.xref _real_access
	.xref _real_bsun
	.xref _real_trap
	.xref _fpsp_done
	.xref _fdbcc
	.xref _fscc
	.xref _ftrapcc
	.xref _imem_read_word
	.xref _imem_read_long
	.xref sop_sqnan
	.xref sop_ssnan
	.xref ssincosd
	.xref ssincosi
	.xref ssincosd
	.xref ssincosz
	.xref stentoxd
	.xref stwotox
	.xref stentox
	.xref slog10
	.xref sacosd
	.xref satand
	.xref satanh
	.xref sasind
	.xref slog2d
	.xref stwotoxd
	.xref scoshd
	.xref setoxd
	.xref slognd
	.xref stanhd
	.xref ssinhd
	.xref sacos
	.xref satan
	.xref sasin
	.xref sgetmand
	.xref slog2
	.xref sasin
	.xref scosd
	.xref scosh
	.xref sgetexpd
	.xref setox
	.xref slogn
	.xref stand
	.xref stanh
	.xref ssind
	.xref ssinh
	.xref slog10d
	.xref satanhd
	.xref setoxm1d
	.xref setoxm1i
	.xref sgetman
	.xref scos
	.xref sgetexp
	.xref stan
	.xref ssin
	.xref setoxm1
	.xref slognp1
	.xref slognp1d
	.xref ssincos
	.xref szr_inf
	.xref sopr_inf
	.xref sscale_sdnrm
	.xref sscale_snorm
	.xref sscale_szero
	.xref smod_sdnrm
	.xref smod_szero
	.xref smod_snorm
	.xref _load_fop
	.xref funimp_skew
	.xref sscale_sinf
	.xref ld_ppi2
	.xref ld_pinf
	.xref ld_pone
	.xref t_dz2
	.xref t_operr
	.xref spi_2
	.xref smod_sinf
	.xref store_fpreg
	.xref src_snan
	.xref src_qnan
	.xref src_zero
	.xref src_inf
	.xref src_one
	.xref srem_sdnrm
	.xref srem_snorm
	.xref srem_sinf
	.xref srem_szero
	.xref ssincosqnan
	.xref ssincossnan
	.xref smovecr

	.text

	.globl		_fpsp_unimp
_fpsp_unimp:

	link.w		a6,#-LOCAL_SIZE	/*  init stack frame */

	movem.l		d0-d1/a0-a1,EXC_DREGS(a6)	/*  save d0-d1/a0-a1 */
	fmovem.l		fpcr/fpsr/fpiar,USER_FPCR(a6) /*  save ctrl regs */
	fmovem.x		fp0-fp1,EXC_FPREGS(a6)	/*  save fp0-fp1 */

	btst		#0x5,EXC_SR(a6)	/*  user mode exception? */
	bne.b		funimp_s		/*  no; supervisor mode */

/*  save the value of the user stack pointer onto the stack frame */
funimp_u:
	move.l		usp,a0		/*  fetch user stack pointer */
	move.l		a0,EXC_A7(a6)		/*  store in stack frame */
	bra.b		funimp_cont

/*  store the value of the supervisor stack pointer BEFORE the exc occurred. */
/*  old_sp is address just above stacked effective address. */
funimp_s:
	lea		4+EXC_EA(a6),a0	/*  load old a7' */
	move.l		a0,EXC_A7(a6)		/*  store a7' */
	move.l		a0,OLD_A7(a6)		/*  make a copy */

funimp_cont:

/*  the FPIAR holds the "current PC" of the faulting instruction. */
	move.l		USER_FPIAR(a6),EXC_EXTWPTR(a6)

	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch the instruction words */
	move.l		d0,EXC_OPWORD(a6)

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	fmove.l		#0x0,fpcr		/*  clear FPCR */
	fmove.l		#0x0,fpsr		/*  clear FPSR */

	clr.b		SPCOND_FLG(a6)		/*  clear "special case" flag */

/*  Divide the fp instructions into 8 types based on the TYPE field in */
/*  bits 6-8 of the opword(classes 6,7 are undefined). */
/*  (for the '060, only two types  can take this exception) */
/* 	bftst		d0{7:3}		; test TYPE */
	btst		#22,d0			/*  type 0 or 1 ? */
	bne.w		funimp_misc		/*  type 1 */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  TYPE == 0: General instructions	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
funimp_gen:

	clr.b		STORE_FLG(a6)		/*  clear "store result" flag */

/*  clear the ccode byte and exception status byte */
	andi.l		#0x00ff00ff,USER_FPSR(a6)

	bfextu		d0{16:6},d1		/*  extract upper 6 of cmdreg */
	cmpi.b		#0x17,d1		/*  is op an fmovecr? */
	beq.w		funimp_fmovecr		/*  yes */

funimp_gen_op:
	bsr.l		_load_fop		/*  load */

	clr.l		d0
	move.b		FPCR_MODE(a6),d0	/*  fetch rnd mode */

	move.b		1+EXC_CMDREG(a6),d1
	andi.w		#0x003f,d1		/*  extract extension bits */
	lsl.w		#0x3,d1		/*  shift right 3 bits */
	or.b		STAG(a6),d1		/*  insert src optag bits */

	lea		FP_DST(a6),a1		/*  pass dst ptr in a1 */
	lea		FP_SRC(a6),a0		/*  pass src ptr in a0 */

	move.w		(tbl_trans.w,pc,d1.w*2),d1
	jsr		(tbl_trans.w,pc,d1.w*1) /*  emulate */

funimp_fsave:
	move.b		FPCR_ENABLE(a6),d0	/*  fetch exceptions enabled */
	bne.w		funimp_ena		/*  some are enabled */

funimp_store:
	bfextu		EXC_CMDREG(a6){6:3},d0 /*  fetch Dn */
	bsr.l		store_fpreg		/*  store result to fp regfile */

funimp_gen_exit:
	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

funimp_gen_exit_cmp:
	cmpi.b		#mia7_flg,SPCOND_FLG(a6) /*  was the ea mode (sp)+ ? */
	beq.b		funimp_gen_exit_a7	/*  yes */

	cmpi.b		#mda7_flg,SPCOND_FLG(a6) /*  was the ea mode -(sp) ? */
	beq.b		funimp_gen_exit_a7	/*  yes */

funimp_gen_exit_cont:
	unlk		a6

funimp_gen_exit_cont2:
	btst		#0x7,(sp)		/*  is trace on? */
	beq.l		_fpsp_done		/*  no */

/*  this catches a problem with the case where an exception will be re-inserted */
/*  into the machine. the frestore has already been executed...so, the fmove.l */
/*  alone of the control register would trigger an unwanted exception. */
/*  until I feel like fixing this, we'll sidestep the exception. */
	fsave		-(sp)
	fmove.l		fpiar,0x14(sp)	/*  "Current PC" is in FPIAR */
	frestore	(sp)+
	move.w		#0x2024,0x6(sp)	/*  stk fmt = 0x2; voff = 0x24 */
	bra.l		_real_trace

funimp_gen_exit_a7:
	btst		#0x5,EXC_SR(a6)	/*  supervisor or user mode? */
	bne.b		funimp_gen_exit_a7_s	/*  supervisor */

	move.l		a0,-(sp)
	move.l		EXC_A7(a6),a0
	move.l		a0,usp
	move.l		(sp)+,a0
	bra.b		funimp_gen_exit_cont

/*  if the instruction was executed from supervisor mode and the addressing */
/*  mode was (a7)+, then the stack frame for the rte must be shifted "up" */
/*  "n" bytes where "n" is the size of the src operand type. */
/*  f<op>.{b,w,l,s,d,x,p} */
funimp_gen_exit_a7_s:
	move.l		d0,-(sp)		/*  save d0 */
	move.l		EXC_A7(a6),d0		/*  load new a7' */
	sub.l		OLD_A7(a6),d0		/*  subtract old a7' */
	move.l		0x2+EXC_PC(a6),(0x2+EXC_PC,a6,d0.l) /*  shift stack frame */
	move.l		EXC_SR(a6),(EXC_SR,a6,d0.l) /*  shift stack frame */
	move.w		d0,EXC_SR(a6)		/*  store incr number */
	move.l		(sp)+,d0		/*  restore d0 */

	unlk		a6

	add.w		(sp),sp		/*  stack frame shifted */
	bra.b		funimp_gen_exit_cont2

/* ;;;;;;;;;;;;;;;;;;;; */
/*  fmovecr.x #ccc,fpn */
/* ;;;;;;;;;;;;;;;;;;;; */
funimp_fmovecr:
	clr.l		d0
	move.b		FPCR_MODE(a6),d0
	move.b		1+EXC_CMDREG(a6),d1
	andi.l		#0x0000007f,d1		/*  pass rom offset in d1 */
	bsr.l		smovecr
	bra.w		funimp_fsave

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * the user has enabled some exceptions. we figure not to see this too
 * often so that's why it gets lower priority.
 */
funimp_ena:

/*  was an exception set that was also enabled? */
	and.b		FPSR_EXCEPT(a6),d0	/*  keep only ones enabled and set */
	bfffo		d0{24:8},d0		/*  find highest priority exception */
	bne.b		funimp_exc		/*  at least one was set */

/*  no exception that was enabled was set BUT if we got an exact overflow */
/*  and overflow wasn't enabled but inexact was (yech!) then this is */
/*  an inexact exception; otherwise, return to normal non-exception flow. */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  did overflow occur? */
	beq.w		funimp_store		/*  no; return to normal flow */

/*  the overflow w/ exact result happened but was inexact set in the FPCR? */
funimp_ovfl:
	btst		#inex2_bit,FPCR_ENABLE(a6) /*  is inexact enabled? */
	beq.w		funimp_store		/*  no; return to normal flow */
	bra.b		funimp_exc_ovfl		/*  yes */

/*  some exception happened that was actually enabled. */
/*  we'll insert this new exception into the FPU and then return. */
funimp_exc:
	subi.l		#24,d0			/*  fix offset to be 0-8 */
	cmpi.b		#0x6,d0		/*  is exception INEX? */
	bne.b		funimp_exc_force	/*  no */

/*  the enabled exception was inexact. so, if it occurs with an overflow */
/*  or underflow that was disabled, then we have to force an overflow or */
/*  underflow frame. the eventual overflow or underflow handler will see that */
/*  it's actually an inexact and act appropriately. this is the only easy */
/*  way to have the EXOP available for the enabled inexact handler when */
/*  a disabled overflow or underflow has also happened. */
	btst		#ovfl_bit,FPSR_EXCEPT(a6) /*  did overflow occur? */
	bne.b		funimp_exc_ovfl		/*  yes */
	btst		#unfl_bit,FPSR_EXCEPT(a6) /*  did underflow occur? */
	bne.b		funimp_exc_unfl		/*  yes */

/*  force the fsave exception status bits to signal an exception of the */
/*  appropriate type. don't forget to "skew" the source operand in case we */
/*  "unskewed" the one the hardware initially gave us. */
funimp_exc_force:
	move.l		d0,-(sp)		/*  save d0 */
	bsr.l		funimp_skew		/*  check for special case */
	move.l		(sp)+,d0		/*  restore d0 */
	move.w		(tbl_funimp_except.b,pc,d0.w*2),2+FP_SRC(a6)
	bra.b		funimp_gen_exit2	/*  exit with frestore */

tbl_funimp_except:
	.dc.w		0xe002, 0xe006, 0xe004, 0xe005
	.dc.w		0xe003, 0xe002, 0xe001, 0xe001

/*  insert an overflow frame */
funimp_exc_ovfl:
	bsr.l		funimp_skew		/*  check for special case */
	move.w		#0xe005,2+FP_SRC(a6)
	bra.b		funimp_gen_exit2

/*  insert an underflow frame */
funimp_exc_unfl:
	bsr.l		funimp_skew		/*  check for special case */
	move.w		#0xe003,2+FP_SRC(a6)

/*  this is the general exit point for an enabled exception that will be */
/*  restored into the machine for the instruction just emulated. */
funimp_gen_exit2:
	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  insert exceptional status */

	bra.w		funimp_gen_exit_cmp

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * TYPE == 1: FDB<cc>, FS<cc>, FTRAP<cc>
 *
 * These instructions were implemented on the '881/2 and '040 in hardware but
 * are emulated in software on the '060.
 */
funimp_misc:
	bfextu		d0{10:3},d1		/*  extract mode field */
	cmpi.b		#0x1,d1		/*  is it an fdb<cc>? */
	beq.w		funimp_fdbcc		/*  yes */
	cmpi.b		#0x7,d1		/*  is it an fs<cc>? */
	bne.w		funimp_fscc		/*  yes */
	bfextu		d0{13:3},d1
	cmpi.b		#0x2,d1		/*  is it an fs<cc>? */
	blt.w		funimp_fscc		/*  yes */

/* ;;;;;;;;;;;;;;;;;;;;;;; */
/*  ftrap<cc>		 */
/*  ftrap<cc>.w #<data>	 */
/*  ftrap<cc>.l #<data>	 */
/* ;;;;;;;;;;;;;;;;;;;;;;; */
funimp_ftrapcc:

	bsr.l		_ftrapcc		/*  FTRAP<cc>() */

	cmpi.b		#fbsun_flg,SPCOND_FLG(a6) /*  is enabled bsun occurring? */
	beq.w		funimp_bsun		/*  yes */

	cmpi.b		#ftrapcc_flg,SPCOND_FLG(a6) /*  should a trap occur? */
	bne.w		funimp_done		/*  no */

/*
 *       FP UNIMP FRAME            TRAP  FRAME
 *      *****************       *****************
 *      **    <EA>     **       **  Current PC **
 *      *****************       *****************
 *      * 0x2 *  0x02c  *       * 0x2 *  0x01c  *
 *      *****************       *****************
 *      **   Next PC   **       **   Next PC   **
 *      *****************       *****************
 *      *      SR       *       *      SR       *
 *      *****************       *****************
 *          (6 words)               (6 words)
 *
 * the ftrapcc instruction should take a trap. so, here we must create a
 * trap stack frame from an unimplemented fp instruction stack frame and
 * jump to the user supplied entry point for the trap exception
 */
funimp_ftrapcc_tp:
	move.l		USER_FPIAR(a6),EXC_EA(a6) /*  Address = Current PC */
	move.w		#0x201c,EXC_VOFF(a6)	/*  Vector Offset = 0x01c */

	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6
	bra.l		_real_trap

/* ;;;;;;;;;;;;;;;;;;;;;;; */
/*  fdb<cc> Dn,<label>	 */
/* ;;;;;;;;;;;;;;;;;;;;;;; */
funimp_fdbcc:

	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x2,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_word		/*  read displacement */

	tst.l		d1			/*  did ifetch fail? */
	bne.w		funimp_iacc		/*  yes */

	ext.l		d0			/*  sign extend displacement */

	bsr.l		_fdbcc			/*  FDB<cc>() */

	cmpi.b		#fbsun_flg,SPCOND_FLG(a6) /*  is enabled bsun occurring? */
	beq.w		funimp_bsun

	bra.w		funimp_done		/*  branch to finish */

/* ;;;;;;;;;;;;;;; */
/*  fs<cc>.b <ea>	 */
/* ;;;;;;;;;;;;;;; */
funimp_fscc:

	bsr.l		_fscc			/*  FS<cc>() */

/*  I am assuming here that an "fs<cc>.b -(An)" or "fs<cc>.b (An)+" instruction */
/*  does not need to update "An" before taking a bsun exception. */
	cmpi.b		#fbsun_flg,SPCOND_FLG(a6) /*  is enabled bsun occurring? */
	beq.w		funimp_bsun

	btst		#0x5,EXC_SR(a6)	/*  yes; is it a user mode exception? */
	bne.b		funimp_fscc_s		/*  no */

funimp_fscc_u:
	move.l		EXC_A7(a6),a0		/*  yes; set new USP */
	move.l		a0,usp
	bra.w		funimp_done		/*  branch to finish */

/*
 * remember, I'm assuming that post-increment is bogus...(it IS!!!)
 * so, the least significant WORD of the stacked effective address got
 * overwritten by the "fs<cc> -(An)". We must shift the stack frame "down"
 * so that the rte will work correctly without destroying the result.
 * even though the operation size is byte, the stack ptr is decr by 2.
 *
 * remember, also, this instruction may be traced.
 */
funimp_fscc_s:
	cmpi.b		#mda7_flg,SPCOND_FLG(a6) /*  was a7 modified? */
	bne.w		funimp_done		/*  no */

	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	btst		#0x7,(sp)		/*  is trace enabled? */
	bne.b		funimp_fscc_s_trace	/*  yes */

	subq.l		#0x2,sp
	move.l		0x2(sp),(sp)		/*  shift SR,hi(PC) "down" */
	move.l		0x6(sp),0x4(sp)	/*  shift lo(PC),voff "down" */
	bra.l		_fpsp_done

funimp_fscc_s_trace:
	subq.l		#0x2,sp
	move.l		0x2(sp),(sp)		/*  shift SR,hi(PC) "down" */
	move.w		0x6(sp),0x4(sp)	/*  shift lo(PC) */
	move.w		#0x2024,0x6(sp)	/*  fmt/voff = $2024 */
	fmove.l		fpiar,0x8(sp)		/*  insert "current PC" */

	bra.l		_real_trace

/*
 * The ftrap<cc>, fs<cc>, or fdb<cc> is to take an enabled bsun. we must convert
 * the fp unimplemented instruction exception stack frame into a bsun stack frame,
 * restore a bsun exception into the machine, and branch to the user
 * supplied bsun hook.
 *
 *       FP UNIMP FRAME            BSUN FRAME
 *      *****************       *****************
 *      **    <EA>     **       * 0x0 * 0x0c0   *
 *      *****************       *****************
 *      * 0x2 *  0x02c  *       ** Current PC  **
 *      *****************       *****************
 *      **   Next PC   **       *      SR       *
 *      *****************       *****************
 *      *      SR       *       *    (4 words)
 *      *****************
 *          (6 words)
 */
funimp_bsun:
	move.w		#0x00c0,2+EXC_EA(a6)	/*  Fmt = 0x0; Vector Offset = 0x0c0 */
	move.l		USER_FPIAR(a6),EXC_VOFF(a6) /*  PC = Current PC */
	move.w		EXC_SR(a6),2+EXC_PC(a6) /*  shift SR "up" */

	move.w		#0xe000,2+FP_SRC(a6)	/*  bsun exception enabled */

	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	frestore	FP_SRC(a6)		/*  restore bsun exception */

	unlk		a6

	addq.l		#0x4,sp		/*  erase sludge */

	bra.l		_real_bsun		/*  branch to user bsun hook */

/*
 * all ftrapcc/fscc/fdbcc processing has been completed. unwind the stack frame
 * and return.
 *
 * as usual, we have to check for trace mode being on here. since instructions
 * modifying the supervisor stack frame don't pass through here, this is a
 * relatively easy task.
 */
funimp_done:
	fmovem.x		EXC_FP0(a6),fp0-fp1	/*  restore fp0-fp1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */

	unlk		a6

	btst		#0x7,(sp)		/*  is trace enabled? */
	bne.b		funimp_trace		/*  yes */

	bra.l		_fpsp_done

/*
 *       FP UNIMP FRAME           TRACE  FRAME
 *      *****************       *****************
 *      **    <EA>     **       **  Current PC **
 *      *****************       *****************
 *      * 0x2 *  0x02c  *       * 0x2 *  0x024  *
 *      *****************       *****************
 *      **   Next PC   **       **   Next PC   **
 *      *****************       *****************
 *      *      SR       *       *      SR       *
 *      *****************       *****************
 *          (6 words)               (6 words)
 *
 * the fscc instruction should take a trace trap. so, here we must create a
 * trace stack frame from an unimplemented fp instruction stack frame and
 * jump to the user supplied entry point for the trace exception
 */
funimp_trace:
	fmove.l		fpiar,0x8(sp)		/*  current PC is in fpiar */
	move.b		#0x24,0x7(sp)		/*  vector offset = 0x024 */

	bra.l		_real_trace

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		tbl_trans
	/* swbeg		#0x1c0 */
	.dc.w 0x4afc,0x1c0
tbl_trans:
	.dc.w		tbl_trans-tbl_trans	/*  $00-0 fmovecr all */
	.dc.w		tbl_trans-tbl_trans	/*  $00-1 fmovecr all */
	.dc.w		tbl_trans-tbl_trans	/*  $00-2 fmovecr all */
	.dc.w		tbl_trans-tbl_trans	/*  $00-3 fmovecr all */
	.dc.w		tbl_trans-tbl_trans	/*  $00-4 fmovecr all */
	.dc.w		tbl_trans-tbl_trans	/*  $00-5 fmovecr all */
	.dc.w		tbl_trans-tbl_trans	/*  $00-6 fmovecr all */
	.dc.w		tbl_trans-tbl_trans	/*  $00-7 fmovecr all */

	.dc.w		tbl_trans-tbl_trans	/*  $01-0 fint norm */
	.dc.w		tbl_trans-tbl_trans	/*  $01-1 fint zero */
	.dc.w		tbl_trans-tbl_trans	/*  $01-2 fint inf */
	.dc.w		tbl_trans-tbl_trans	/*  $01-3 fint qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $01-5 fint denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $01-4 fint snan */
	.dc.w		tbl_trans-tbl_trans	/*  $01-6 fint unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $01-7 ERROR */

	.dc.w		ssinh-tbl_trans		/*  $02-0 fsinh norm */
	.dc.w		src_zero-tbl_trans	/*  $02-1 fsinh zero */
	.dc.w		src_inf-tbl_trans	/*  $02-2 fsinh inf */
	.dc.w		src_qnan-tbl_trans	/*  $02-3 fsinh qnan */
	.dc.w		ssinhd-tbl_trans	/*  $02-5 fsinh denorm */
	.dc.w		src_snan-tbl_trans	/*  $02-4 fsinh snan */
	.dc.w		tbl_trans-tbl_trans	/*  $02-6 fsinh unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $02-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $03-0 fintrz norm */
	.dc.w		tbl_trans-tbl_trans	/*  $03-1 fintrz zero */
	.dc.w		tbl_trans-tbl_trans	/*  $03-2 fintrz inf */
	.dc.w		tbl_trans-tbl_trans	/*  $03-3 fintrz qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $03-5 fintrz denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $03-4 fintrz snan */
	.dc.w		tbl_trans-tbl_trans	/*  $03-6 fintrz unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $03-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $04-0 fsqrt norm */
	.dc.w		tbl_trans-tbl_trans	/*  $04-1 fsqrt zero */
	.dc.w		tbl_trans-tbl_trans	/*  $04-2 fsqrt inf */
	.dc.w		tbl_trans-tbl_trans	/*  $04-3 fsqrt qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $04-5 fsqrt denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $04-4 fsqrt snan */
	.dc.w		tbl_trans-tbl_trans	/*  $04-6 fsqrt unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $04-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $05-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $05-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $05-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $05-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $05-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $05-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $05-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $05-7 ERROR */

	.dc.w		slognp1-tbl_trans	/*  $06-0 flognp1 norm */
	.dc.w		src_zero-tbl_trans	/*  $06-1 flognp1 zero */
	.dc.w		sopr_inf-tbl_trans	/*  $06-2 flognp1 inf */
	.dc.w		src_qnan-tbl_trans	/*  $06-3 flognp1 qnan */
	.dc.w		slognp1d-tbl_trans	/*  $06-5 flognp1 denorm */
	.dc.w		src_snan-tbl_trans	/*  $06-4 flognp1 snan */
	.dc.w		tbl_trans-tbl_trans	/*  $06-6 flognp1 unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $06-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $07-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $07-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $07-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $07-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $07-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $07-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $07-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $07-7 ERROR */

	.dc.w		setoxm1-tbl_trans	/*  $08-0 fetoxm1 norm */
	.dc.w		src_zero-tbl_trans	/*  $08-1 fetoxm1 zero */
	.dc.w		setoxm1i-tbl_trans	/*  $08-2 fetoxm1 inf */
	.dc.w		src_qnan-tbl_trans	/*  $08-3 fetoxm1 qnan */
	.dc.w		setoxm1d-tbl_trans	/*  $08-5 fetoxm1 denorm */
	.dc.w		src_snan-tbl_trans	/*  $08-4 fetoxm1 snan */
	.dc.w		tbl_trans-tbl_trans	/*  $08-6 fetoxm1 unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $08-7 ERROR */

	.dc.w		stanh-tbl_trans		/*  $09-0 ftanh norm */
	.dc.w		src_zero-tbl_trans	/*  $09-1 ftanh zero */
	.dc.w		src_one-tbl_trans	/*  $09-2 ftanh inf */
	.dc.w		src_qnan-tbl_trans	/*  $09-3 ftanh qnan */
	.dc.w		stanhd-tbl_trans	/*  $09-5 ftanh denorm */
	.dc.w		src_snan-tbl_trans	/*  $09-4 ftanh snan */
	.dc.w		tbl_trans-tbl_trans	/*  $09-6 ftanh unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $09-7 ERROR */

	.dc.w		satan-tbl_trans		/*  $0a-0 fatan norm */
	.dc.w		src_zero-tbl_trans	/*  $0a-1 fatan zero */
	.dc.w		spi_2-tbl_trans		/*  $0a-2 fatan inf */
	.dc.w		src_qnan-tbl_trans	/*  $0a-3 fatan qnan */
	.dc.w		satand-tbl_trans	/*  $0a-5 fatan denorm */
	.dc.w		src_snan-tbl_trans	/*  $0a-4 fatan snan */
	.dc.w		tbl_trans-tbl_trans	/*  $0a-6 fatan unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $0a-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $0b-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $0b-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $0b-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $0b-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $0b-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $0b-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $0b-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $0b-7 ERROR */

	.dc.w		sasin-tbl_trans		/*  $0c-0 fasin norm */
	.dc.w		src_zero-tbl_trans	/*  $0c-1 fasin zero */
	.dc.w		t_operr-tbl_trans	/*  $0c-2 fasin inf */
	.dc.w		src_qnan-tbl_trans	/*  $0c-3 fasin qnan */
	.dc.w		sasind-tbl_trans	/*  $0c-5 fasin denorm */
	.dc.w		src_snan-tbl_trans	/*  $0c-4 fasin snan */
	.dc.w		tbl_trans-tbl_trans	/*  $0c-6 fasin unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $0c-7 ERROR */

	.dc.w		satanh-tbl_trans	/*  $0d-0 fatanh norm */
	.dc.w		src_zero-tbl_trans	/*  $0d-1 fatanh zero */
	.dc.w		t_operr-tbl_trans	/*  $0d-2 fatanh inf */
	.dc.w		src_qnan-tbl_trans	/*  $0d-3 fatanh qnan */
	.dc.w		satanhd-tbl_trans	/*  $0d-5 fatanh denorm */
	.dc.w		src_snan-tbl_trans	/*  $0d-4 fatanh snan */
	.dc.w		tbl_trans-tbl_trans	/*  $0d-6 fatanh unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $0d-7 ERROR */

	.dc.w		ssin-tbl_trans		/*  $0e-0 fsin norm */
	.dc.w		src_zero-tbl_trans	/*  $0e-1 fsin zero */
	.dc.w		t_operr-tbl_trans	/*  $0e-2 fsin inf */
	.dc.w		src_qnan-tbl_trans	/*  $0e-3 fsin qnan */
	.dc.w		ssind-tbl_trans		/*  $0e-5 fsin denorm */
	.dc.w		src_snan-tbl_trans	/*  $0e-4 fsin snan */
	.dc.w		tbl_trans-tbl_trans	/*  $0e-6 fsin unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $0e-7 ERROR */

	.dc.w		stan-tbl_trans		/*  $0f-0 ftan norm */
	.dc.w		src_zero-tbl_trans	/*  $0f-1 ftan zero */
	.dc.w		t_operr-tbl_trans	/*  $0f-2 ftan inf */
	.dc.w		src_qnan-tbl_trans	/*  $0f-3 ftan qnan */
	.dc.w		stand-tbl_trans		/*  $0f-5 ftan denorm */
	.dc.w		src_snan-tbl_trans	/*  $0f-4 ftan snan */
	.dc.w		tbl_trans-tbl_trans	/*  $0f-6 ftan unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $0f-7 ERROR */

	.dc.w		setox-tbl_trans		/*  $10-0 fetox norm */
	.dc.w		ld_pone-tbl_trans	/*  $10-1 fetox zero */
	.dc.w		szr_inf-tbl_trans	/*  $10-2 fetox inf */
	.dc.w		src_qnan-tbl_trans	/*  $10-3 fetox qnan */
	.dc.w		setoxd-tbl_trans	/*  $10-5 fetox denorm */
	.dc.w		src_snan-tbl_trans	/*  $10-4 fetox snan */
	.dc.w		tbl_trans-tbl_trans	/*  $10-6 fetox unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $10-7 ERROR */

	.dc.w		stwotox-tbl_trans	/*  $11-0 ftwotox norm */
	.dc.w		ld_pone-tbl_trans	/*  $11-1 ftwotox zero */
	.dc.w		szr_inf-tbl_trans	/*  $11-2 ftwotox inf */
	.dc.w		src_qnan-tbl_trans	/*  $11-3 ftwotox qnan */
	.dc.w		stwotoxd-tbl_trans	/*  $11-5 ftwotox denorm */
	.dc.w		src_snan-tbl_trans	/*  $11-4 ftwotox snan */
	.dc.w		tbl_trans-tbl_trans	/*  $11-6 ftwotox unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $11-7 ERROR */

	.dc.w		stentox-tbl_trans	/*  $12-0 ftentox norm */
	.dc.w		ld_pone-tbl_trans	/*  $12-1 ftentox zero */
	.dc.w		szr_inf-tbl_trans	/*  $12-2 ftentox inf */
	.dc.w		src_qnan-tbl_trans	/*  $12-3 ftentox qnan */
	.dc.w		stentoxd-tbl_trans	/*  $12-5 ftentox denorm */
	.dc.w		src_snan-tbl_trans	/*  $12-4 ftentox snan */
	.dc.w		tbl_trans-tbl_trans	/*  $12-6 ftentox unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $12-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $13-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $13-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $13-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $13-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $13-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $13-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $13-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $13-7 ERROR */

	.dc.w		slogn-tbl_trans		/*  $14-0 flogn norm */
	.dc.w		t_dz2-tbl_trans		/*  $14-1 flogn zero */
	.dc.w		sopr_inf-tbl_trans	/*  $14-2 flogn inf */
	.dc.w		src_qnan-tbl_trans	/*  $14-3 flogn qnan */
	.dc.w		slognd-tbl_trans	/*  $14-5 flogn denorm */
	.dc.w		src_snan-tbl_trans	/*  $14-4 flogn snan */
	.dc.w		tbl_trans-tbl_trans	/*  $14-6 flogn unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $14-7 ERROR */

	.dc.w		slog10-tbl_trans	/*  $15-0 flog10 norm */
	.dc.w		t_dz2-tbl_trans		/*  $15-1 flog10 zero */
	.dc.w		sopr_inf-tbl_trans	/*  $15-2 flog10 inf */
	.dc.w		src_qnan-tbl_trans	/*  $15-3 flog10 qnan */
	.dc.w		slog10d-tbl_trans	/*  $15-5 flog10 denorm */
	.dc.w		src_snan-tbl_trans	/*  $15-4 flog10 snan */
	.dc.w		tbl_trans-tbl_trans	/*  $15-6 flog10 unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $15-7 ERROR */

	.dc.w		slog2-tbl_trans		/*  $16-0 flog2 norm */
	.dc.w		t_dz2-tbl_trans		/*  $16-1 flog2 zero */
	.dc.w		sopr_inf-tbl_trans	/*  $16-2 flog2 inf */
	.dc.w		src_qnan-tbl_trans	/*  $16-3 flog2 qnan */
	.dc.w		slog2d-tbl_trans	/*  $16-5 flog2 denorm */
	.dc.w		src_snan-tbl_trans	/*  $16-4 flog2 snan */
	.dc.w		tbl_trans-tbl_trans	/*  $16-6 flog2 unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $16-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $17-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $17-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $17-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $17-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $17-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $17-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $17-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $17-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $18-0 fabs norm */
	.dc.w		tbl_trans-tbl_trans	/*  $18-1 fabs zero */
	.dc.w		tbl_trans-tbl_trans	/*  $18-2 fabs inf */
	.dc.w		tbl_trans-tbl_trans	/*  $18-3 fabs qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $18-5 fabs denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $18-4 fabs snan */
	.dc.w		tbl_trans-tbl_trans	/*  $18-6 fabs unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $18-7 ERROR */

	.dc.w		scosh-tbl_trans		/*  $19-0 fcosh norm */
	.dc.w		ld_pone-tbl_trans	/*  $19-1 fcosh zero */
	.dc.w		ld_pinf-tbl_trans	/*  $19-2 fcosh inf */
	.dc.w		src_qnan-tbl_trans	/*  $19-3 fcosh qnan */
	.dc.w		scoshd-tbl_trans	/*  $19-5 fcosh denorm */
	.dc.w		src_snan-tbl_trans	/*  $19-4 fcosh snan */
	.dc.w		tbl_trans-tbl_trans	/*  $19-6 fcosh unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $19-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $1a-0 fneg norm */
	.dc.w		tbl_trans-tbl_trans	/*  $1a-1 fneg zero */
	.dc.w		tbl_trans-tbl_trans	/*  $1a-2 fneg inf */
	.dc.w		tbl_trans-tbl_trans	/*  $1a-3 fneg qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $1a-5 fneg denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $1a-4 fneg snan */
	.dc.w		tbl_trans-tbl_trans	/*  $1a-6 fneg unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $1a-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $1b-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $1b-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $1b-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $1b-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $1b-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $1b-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $1b-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $1b-7 ERROR */

	.dc.w		sacos-tbl_trans		/*  $1c-0 facos norm */
	.dc.w		ld_ppi2-tbl_trans	/*  $1c-1 facos zero */
	.dc.w		t_operr-tbl_trans	/*  $1c-2 facos inf */
	.dc.w		src_qnan-tbl_trans	/*  $1c-3 facos qnan */
	.dc.w		sacosd-tbl_trans	/*  $1c-5 facos denorm */
	.dc.w		src_snan-tbl_trans	/*  $1c-4 facos snan */
	.dc.w		tbl_trans-tbl_trans	/*  $1c-6 facos unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $1c-7 ERROR */

	.dc.w		scos-tbl_trans		/*  $1d-0 fcos norm */
	.dc.w		ld_pone-tbl_trans	/*  $1d-1 fcos zero */
	.dc.w		t_operr-tbl_trans	/*  $1d-2 fcos inf */
	.dc.w		src_qnan-tbl_trans	/*  $1d-3 fcos qnan */
	.dc.w		scosd-tbl_trans		/*  $1d-5 fcos denorm */
	.dc.w		src_snan-tbl_trans	/*  $1d-4 fcos snan */
	.dc.w		tbl_trans-tbl_trans	/*  $1d-6 fcos unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $1d-7 ERROR */

	.dc.w		sgetexp-tbl_trans	/*  $1e-0 fgetexp norm */
	.dc.w		src_zero-tbl_trans	/*  $1e-1 fgetexp zero */
	.dc.w		t_operr-tbl_trans	/*  $1e-2 fgetexp inf */
	.dc.w		src_qnan-tbl_trans	/*  $1e-3 fgetexp qnan */
	.dc.w		sgetexpd-tbl_trans	/*  $1e-5 fgetexp denorm */
	.dc.w		src_snan-tbl_trans	/*  $1e-4 fgetexp snan */
	.dc.w		tbl_trans-tbl_trans	/*  $1e-6 fgetexp unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $1e-7 ERROR */

	.dc.w		sgetman-tbl_trans	/*  $1f-0 fgetman norm */
	.dc.w		src_zero-tbl_trans	/*  $1f-1 fgetman zero */
	.dc.w		t_operr-tbl_trans	/*  $1f-2 fgetman inf */
	.dc.w		src_qnan-tbl_trans	/*  $1f-3 fgetman qnan */
	.dc.w		sgetmand-tbl_trans	/*  $1f-5 fgetman denorm */
	.dc.w		src_snan-tbl_trans	/*  $1f-4 fgetman snan */
	.dc.w		tbl_trans-tbl_trans	/*  $1f-6 fgetman unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $1f-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $20-0 fdiv norm */
	.dc.w		tbl_trans-tbl_trans	/*  $20-1 fdiv zero */
	.dc.w		tbl_trans-tbl_trans	/*  $20-2 fdiv inf */
	.dc.w		tbl_trans-tbl_trans	/*  $20-3 fdiv qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $20-5 fdiv denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $20-4 fdiv snan */
	.dc.w		tbl_trans-tbl_trans	/*  $20-6 fdiv unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $20-7 ERROR */

	.dc.w		smod_snorm-tbl_trans	/*  $21-0 fmod norm */
	.dc.w		smod_szero-tbl_trans	/*  $21-1 fmod zero */
	.dc.w		smod_sinf-tbl_trans		/*  $21-2 fmod inf */
	.dc.w		sop_sqnan-tbl_trans		/*  $21-3 fmod qnan */
	.dc.w		smod_sdnrm-tbl_trans	/*  $21-5 fmod denorm */
	.dc.w		sop_ssnan-tbl_trans		/*  $21-4 fmod snan */
	.dc.w		tbl_trans-tbl_trans		/*  $21-6 fmod unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $21-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $22-0 fadd norm */
	.dc.w		tbl_trans-tbl_trans	/*  $22-1 fadd zero */
	.dc.w		tbl_trans-tbl_trans	/*  $22-2 fadd inf */
	.dc.w		tbl_trans-tbl_trans	/*  $22-3 fadd qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $22-5 fadd denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $22-4 fadd snan */
	.dc.w		tbl_trans-tbl_trans	/*  $22-6 fadd unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $22-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $23-0 fmul norm */
	.dc.w		tbl_trans-tbl_trans	/*  $23-1 fmul zero */
	.dc.w		tbl_trans-tbl_trans	/*  $23-2 fmul inf */
	.dc.w		tbl_trans-tbl_trans	/*  $23-3 fmul qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $23-5 fmul denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $23-4 fmul snan */
	.dc.w		tbl_trans-tbl_trans	/*  $23-6 fmul unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $23-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $24-0 fsgldiv norm */
	.dc.w		tbl_trans-tbl_trans	/*  $24-1 fsgldiv zero */
	.dc.w		tbl_trans-tbl_trans	/*  $24-2 fsgldiv inf */
	.dc.w		tbl_trans-tbl_trans	/*  $24-3 fsgldiv qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $24-5 fsgldiv denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $24-4 fsgldiv snan */
	.dc.w		tbl_trans-tbl_trans	/*  $24-6 fsgldiv unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $24-7 ERROR */

	.dc.w		srem_snorm-tbl_trans	/*  $25-0 frem norm */
	.dc.w		srem_szero-tbl_trans	/*  $25-1 frem zero */
	.dc.w		srem_sinf-tbl_trans		/*  $25-2 frem inf */
	.dc.w		sop_sqnan-tbl_trans		/*  $25-3 frem qnan */
	.dc.w		srem_sdnrm-tbl_trans	/*  $25-5 frem denorm */
	.dc.w		sop_ssnan-tbl_trans		/*  $25-4 frem snan */
	.dc.w		tbl_trans-tbl_trans		/*  $25-6 frem unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $25-7 ERROR */

	.dc.w		sscale_snorm-tbl_trans	/*  $26-0 fscale norm */
	.dc.w		sscale_szero-tbl_trans	/*  $26-1 fscale zero */
	.dc.w		sscale_sinf-tbl_trans	/*  $26-2 fscale inf */
	.dc.w		sop_sqnan-tbl_trans		/*  $26-3 fscale qnan */
	.dc.w		sscale_sdnrm-tbl_trans	/*  $26-5 fscale denorm */
	.dc.w		sop_ssnan-tbl_trans		/*  $26-4 fscale snan */
	.dc.w		tbl_trans-tbl_trans		/*  $26-6 fscale unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $26-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $27-0 fsglmul norm */
	.dc.w		tbl_trans-tbl_trans	/*  $27-1 fsglmul zero */
	.dc.w		tbl_trans-tbl_trans	/*  $27-2 fsglmul inf */
	.dc.w		tbl_trans-tbl_trans	/*  $27-3 fsglmul qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $27-5 fsglmul denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $27-4 fsglmul snan */
	.dc.w		tbl_trans-tbl_trans	/*  $27-6 fsglmul unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $27-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $28-0 fsub norm */
	.dc.w		tbl_trans-tbl_trans	/*  $28-1 fsub zero */
	.dc.w		tbl_trans-tbl_trans	/*  $28-2 fsub inf */
	.dc.w		tbl_trans-tbl_trans	/*  $28-3 fsub qnan */
	.dc.w		tbl_trans-tbl_trans	/*  $28-5 fsub denorm */
	.dc.w		tbl_trans-tbl_trans	/*  $28-4 fsub snan */
	.dc.w		tbl_trans-tbl_trans	/*  $28-6 fsub unnorm */
	.dc.w		tbl_trans-tbl_trans	/*  $28-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $29-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $29-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $29-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $29-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $29-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $29-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $29-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $29-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $2a-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2a-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2a-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2a-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2a-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2a-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2a-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2a-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $2b-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2b-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2b-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2b-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2b-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2b-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2b-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2b-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $2c-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2c-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2c-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2c-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2c-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2c-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2c-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2c-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $2d-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2d-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2d-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2d-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2d-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2d-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2d-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2d-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $2e-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2e-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2e-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2e-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2e-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2e-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2e-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2e-7 ERROR */

	.dc.w		tbl_trans-tbl_trans	/*  $2f-0 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2f-1 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2f-2 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2f-3 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2f-4 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2f-5 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2f-6 ERROR */
	.dc.w		tbl_trans-tbl_trans	/*  $2f-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $30-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $30-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $30-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $30-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $30-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $30-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $30-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $30-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $31-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $31-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $31-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $31-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $31-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $31-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $31-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $31-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $32-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $32-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $32-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $32-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $32-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $32-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $32-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $32-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $33-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $33-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $33-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $33-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $33-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $33-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $33-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $33-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $34-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $34-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $34-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $34-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $34-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $34-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $34-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $34-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $35-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $35-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $35-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $35-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $35-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $35-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $35-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $35-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $36-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $36-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $36-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $36-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $36-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $36-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $36-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $36-7 ERROR */

	.dc.w		ssincos-tbl_trans		/*  $37-0 fsincos norm */
	.dc.w		ssincosz-tbl_trans		/*  $37-1 fsincos zero */
	.dc.w		ssincosi-tbl_trans		/*  $37-2 fsincos inf */
	.dc.w		ssincosqnan-tbl_trans	/*  $37-3 fsincos qnan */
	.dc.w		ssincosd-tbl_trans		/*  $37-5 fsincos denorm */
	.dc.w		ssincossnan-tbl_trans	/*  $37-4 fsincos snan */
	.dc.w		tbl_trans-tbl_trans		/*  $37-6 fsincos unnorm */
	.dc.w		tbl_trans-tbl_trans		/*  $37-7 ERROR */

/* ;;;;;;;; */

/*  the instruction fetch access for the displacement word for the */
/*  fdbcc emulation failed. here, we create an access error frame */
/*  from the current frame and branch to _real_access(). */
	.globl funimp_iacc
funimp_iacc:
	movem.l		EXC_DREGS(a6),d0-d1/a0-a1	/*  restore d0-d1/a0-a1 */
	fmovem.l		USER_FPCR(a6),fpcr/fpsr/fpiar /*  restore ctrl regs */
	fmovem.x		EXC_FPREGS(a6),fp0-fp1	/*  restore fp0-fp1 */

	move.l		USER_FPIAR(a6),EXC_PC(a6) /*  store current PC */

	unlk		a6

	move.l		(sp),-(sp)		/*  store SR,hi(PC) */
	move.w		0x8(sp),0x4(sp)	/*  store lo(PC) */
	move.w		#0x4008,0x6(sp)	/*  store voff */
	move.l		0x2(sp),0x8(sp)	/*  store EA */
	move.l		#0x09428001,0xc(sp)	/*  store FSLW */

	btst		#0x5,(sp)		/*  user or supervisor mode? */
	beq.b		funimp_iacc_end		/*  user */
	bset		#0x2,0xd(sp)		/*  set supervisor TM bit */

funimp_iacc_end:
	bra.l		_real_access

