/*
 *  XDEF ****************************************************************	
 * 	get_packed(): fetch a packed operand from memory and then	
 * 		      convert it to a floating-point binary number.	
 * 									
 *  XREF ****************************************************************	
 * 	_dcalc_ea() - calculate the correct <ea>			
 * 	_mem_read() - fetch the packed operand from memory		
 * 	facc_in_x() - the fetch failed so jump to special exit code	
 * 	decbin()    - convert packed to binary extended precision	
 * 									
 *  INPUT ***************************************************************	
 * 	None								
 * 									
 *  OUTPUT **************************************************************	
 * 	If no failure on _mem_read():					
 * 	FP_SRC(a6) = packed operand now as a binary FP number		
 * 									
 *  ALGORITHM ***********************************************************	
 * 	Get the correct <ea> which is the value on the exception stack	
 *  frame w/ maybe a correction factor if the <ea> is -(an) or (an)+.	
 *  Then, fetch the operand from memory. If the fetch fails, exit		
 *  through facc_in_x().							
 * 	If the packed operand is a ZERO,NAN, or INF, convert it to	
 *  its binary representation here. Else, call decbin() which will	
 *  convert the packed value to an extended precision binary value.	
 * 									
 */

	.include "hdr.fpu"

	.text

/*  the stacked <ea> for packed is correct except for -(An). */
/*  the base reg must be updated for both -(An) and (An)+. */
	.globl		get_packed
get_packed:
	moveq.l		#0xc,d0		/*  packed is 12 .dc.bs */
	bsr.l		_dcalc_ea		/*  fetch <ea>; correct An */

	lea		FP_SRC(a6),a1		/*  pass: ptr to super dst */
	moveq.l		#0xc,d0		/*  pass: 12 bytes */
	bsr.l		_dmem_read		/*  read packed operand */

	tst.l		d1			/*  did dfetch fail? */
	bne.l		facc_in_x		/*  yes */

/*  The packed operand is an INF or a NAN if the exponent field is all ones. */
	bfextu		FP_SRC(a6){1:15},d0	/*  get exp */
	cmpi.w		#0x7fff,d0		/*  INF or NAN? */
	bne.b		gp_try_zero		/*  no */
	rts					/*  operand is an INF or NAN */

/*  The packed operand is a zero if the mantissa is all zero, else it's */
/*  a normal packed op. */
gp_try_zero:
	move.b		3+FP_SRC(a6),d0	/*  get byte 4 */
	andi.b		#0x0f,d0		/*  clear all but last nybble */
	bne.b		gp_not_spec		/*  not a zero */
	tst.l		FP_SRC_HI(a6)		/*  is lw 2 zero? */
	bne.b		gp_not_spec		/*  not a zero */
	tst.l		FP_SRC_LO(a6)		/*  is lw 3 zero? */
	bne.b		gp_not_spec		/*  not a zero */
	rts					/*  operand is a ZERO */
gp_not_spec:
	lea		FP_SRC(a6),a0		/*  pass: ptr to packed op */
	bsr.l		decbin			/*  convert to extended */
	fmovem.x		fp0,FP_SRC(a6)	/*  make this the srcop */
	rts

