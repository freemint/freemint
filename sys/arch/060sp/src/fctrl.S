/*
 *  XDEF ****************************************************************	
 * 	fmovem_ctrl(): emulate fmovem.l of control registers instr	
 * 									
 *  XREF ****************************************************************	
 * 	_imem_read_long() - read longword from memory			
 * 	iea_iacc() - _imem_read_long() failed *  error recovery		
 * 									
 *  INPUT ***************************************************************	
 * 	None								
 * 									
 *  OUTPUT **************************************************************	
 * 	If _imem_read_long() doesn't fail:				
 * 		USER_FPCR(a6)  = new FPCR value				
 * 		USER_FPSR(a6)  = new FPSR value				
 * 		USER_FPIAR(a6) = new FPIAR value			
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Decode the instruction type by looking at the extension word	
 *  in order to see how many control registers to fetch from memory.	
 *  Fetch them using _imem_read_long(). If this fetch fails, exit through	
 *  the special access error exit handler iea_iacc().			
 * 									
 *  Instruction word decoding:						
 * 									
 * 	fmovem.l #<data>, {FPIAR#|FPCR#|FPSR}				
 * 									
 * 		WORD1			WORD2				
 * 	1111 0010 00 111100	100$ $$00 0000 0000			
 * 									
 * 	$$$ (100): FPCR							
 * 	    (010): FPSR							
 * 	    (001): FPIAR						
 * 	    (000): FPIAR						
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fmovem_ctrl
fmovem_ctrl:
	move.b		EXC_EXTWORD(a6),d0	/*  fetch reg select bits */
	cmpi.b		#0x9c,d0		/*  fpcr # fpsr # fpiar ? */
	beq.w		fctrl_in_7		/*  yes */
	cmpi.b		#0x98,d0		/*  fpcr # fpsr ? */
	beq.w		fctrl_in_6		/*  yes */
	cmpi.b		#0x94,d0		/*  fpcr # fpiar ? */
	beq.b		fctrl_in_5		/*  yes */

/*  fmovem.l #<data>, fpsr/fpiar */
fctrl_in_3:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPSR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPSR(a6)	/*  store new FPSR to stack */
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPIAR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPIAR(a6)	/*  store new FPIAR to stack */
	rts

/*  fmovem.l #<data>, fpcr/fpiar */
fctrl_in_5:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPCR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPCR(a6)	/*  store new FPCR to stack */
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPIAR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPIAR(a6)	/*  store new FPIAR to stack */
	rts

/*  fmovem.l #<data>, fpcr/fpsr */
fctrl_in_6:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPCR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPCR(a6)	/*  store new FPCR to mem */
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPSR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPSR(a6)	/*  store new FPSR to mem */
	rts

/*  fmovem.l #<data>, fpcr/fpsr/fpiar */
fctrl_in_7:
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPCR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPCR(a6)	/*  store new FPCR to mem */
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPSR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPSR(a6)	/*  store new FPSR to mem */
	move.l		EXC_EXTWPTR(a6),a0	/*  fetch instruction addr */
	addq.l		#0x4,EXC_EXTWPTR(a6)	/*  incr instruction ptr */
	bsr.l		_imem_read_long		/*  fetch FPIAR from mem */

	tst.l		d1			/*  did ifetch fail? */
	bne.l		iea_iacc		/*  yes */

	move.l		d0,USER_FPIAR(a6)	/*  store new FPIAR to mem */
	rts

