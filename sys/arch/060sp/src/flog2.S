/*
 *  slog10():  computes the base-10 logarithm of a normalized input	
 *  slog10d(): computes the base-10 logarithm of a denormalized input	
 *  slog2():   computes the base-2 logarithm of a normalized input	
 *  slog2d():  computes the base-2 logarithm of a denormalized input	
 * 									
 *  INPUT *************************************************************** 
 * 	a0 = pointer to extended precision input			
 * 	d0 = round precision,mode					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = log_10(X) or log_2(X)					
 * 									
 *  ACCURACY and MONOTONICITY *******************************************	
 * 	The returned result is within 1.7 ulps in 64 significant bit,	
 * 	i.e. within 0.5003 ulp to 53 bits if the result is subsequently	
 * 	rounded to double precision. The result is provably monotonic	
 * 	in double precision.						
 * 									
 *  ALGORITHM ***********************************************************	
 * 									
 *        slog10d:							
 * 									
 *        Step 0.	If X < 0, create a NaN and raise the invalid operation	
 *                flag. Otherwise, save FPCR in D1 *  set FpCR to default.	
 *        Notes:  Default means round-to-nearest mode, no floating-point	
 *                traps, and precision control = double extended.		
 * 									
 *        Step 1. Call slognd to obtain Y = log(X), the natural log of X.	
 *        Notes:  Even if X is denormalized, log(X) is always normalized.	
 * 									
 *        Step 2.  Compute log_10(X) = log(X) * (1/log(10)).		
 *             2.1 Restore the user FPCR					
 *             2.2 Return ans := Y * INV_L10.				
 * 									
 *        slog10:								
 * 									
 *        Step 0. If X < 0, create a NaN and raise the invalid operation	
 *                flag. Otherwise, save FPCR in D1; set FpCR to default.	
 *        Notes:  Default means round-to-nearest mode, no floating-point	
 *                traps, and precision control = double extended.		
 * 									
 *        Step 1. Call sLogN to obtain Y = log(X), the natural log of X.	
 * 									
 *        Step 2.   Compute log_10(X) = log(X) * (1/log(10)).		
 *             2.1  Restore the user FPCR					
 *             2.2  Return ans := Y * INV_L10.				
 * 									
 *        sLog2d:								
 * 									
 *        Step 0. If X < 0, create a NaN and raise the invalid operation	
 *                flag. Otherwise, save FPCR in D1; set FpCR to default.	
 *        Notes:  Default means round-to-nearest mode, no floating-point	
 *                traps, and precision control = double extended.		
 * 									
 *        Step 1. Call slognd to obtain Y = log(X), the natural log of X.	
 *        Notes:  Even if X is denormalized, log(X) is always normalized.	
 * 									
 *        Step 2.   Compute log_10(X) = log(X) * (1/log(2)).		
 *             2.1  Restore the user FPCR					
 *             2.2  Return ans := Y * INV_L2.				
 * 									
 *        sLog2:								
 * 									
 *        Step 0. If X < 0, create a NaN and raise the invalid operation	
 *                flag. Otherwise, save FPCR in D1; set FpCR to default.	
 *        Notes:  Default means round-to-nearest mode, no floating-point	
 *                traps, and precision control = double extended.		
 * 									
 *        Step 1. If X is not an integer power of two, i.e., X != 2^k,	
 *                go to Step 3.						
 * 									
 *        Step 2.   Return k.						
 *             2.1  Get integer k, X = 2^k.				
 *             2.2  Restore the user FPCR.				
 *             2.3  Return ans := convert-to-double-extended(k).		
 * 									
 *        Step 3. Call sLogN to obtain Y = log(X), the natural log of X.	
 * 									
 *        Step 4.   Compute log_2(X) = log(X) * (1/log(2)).		
 *             4.1  Restore the user FPCR					
 *             4.2  Return ans := Y * INV_L2.				
 * 									
 */

	.include "hdr.fpu"

	.text

INV_L10:
	.dc.l		0x3FFD0000,0xDE5BD8A9,0x37287195,0x00000000

INV_L2:
	.dc.l		0x3FFF0000,0xB8AA3B29,0x5C17F0BC,0x00000000

	.globl		slog10
/* --entry point for Log10(X), X is normalized */
slog10:
	fmove.b		#0x1,fp0
	fcmp.x		(a0),fp0		/*  if operand == 1, */
	fbeq.l		ld_pzero		/*  return an EXACT zero */

	move.l		(a0),d1
	blt.w		invalid
	move.l		d0,-(sp)
	clr.l		d0
	bsr.l		slogn			/*  log(X), X normal. */
	fmove.l		(sp)+,fpcr
	fmul.x		INV_L10(pc),fp0
	bra.l		t_inx2

	.globl		slog10d
/* --entry point for Log10(X), X is denormalized */
slog10d:
	move.l		(a0),d1
	blt.w		invalid
	move.l		d0,-(sp)
	clr.l		d0
	bsr.l		slognd			/*  log(X), X denorm. */
	fmove.l		(sp)+,fpcr
	fmul.x		INV_L10(pc),fp0
	bra.l		t_minx2

	.globl		slog2
/* --entry point for Log2(X), X is normalized */
slog2:
	move.l		(a0),d1
	blt.w		invalid

	move.l		8(a0),d1
	bne.b		continue		/*  X is not 2^k */

	move.l		4(a0),d1
	andi.l		#0x7FFFFFFF,d1
	bne.b		continue

/* --X = 2^k. */
	move.w		(a0),d1
	andi.l		#0x00007FFF,d1
	subi.l		#0x3FFF,d1
	beq.l		ld_pzero
	fmove.l		d0,fpcr
	fmove.l		d1,fp0
	bra.l		t_inx2

continue:
	move.l		d0,-(sp)
	clr.l		d0
	bsr.l		slogn			/*  log(X), X normal. */
	fmove.l		(sp)+,fpcr
	fmul.x		INV_L2(pc),fp0
	bra.l		t_inx2

invalid:
	bra.l		t_operr

	.globl		slog2d
/* --entry point for Log2(X), X is denormalized */
slog2d:
	move.l		(a0),d1
	blt.w		invalid
	move.l		d0,-(sp)
	clr.l		d0
	bsr.l		slognd			/*  log(X), X denorm. */
	fmove.l		(sp)+,fpcr
	fmul.x		INV_L2(pc),fp0
	bra.l		t_minx2

