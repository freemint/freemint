/*
 *  XDEF ****************************************************************	
 * 	fgen_except(): catch an exception during transcendental		
 * 		       emulation					
 * 									
 *  XREF ****************************************************************	
 * 	fmul() - emulate a multiply instruction				
 * 	fadd() - emulate an add instruction				
 * 	fin() - emulate an fmove instruction				
 * 									
 *  INPUT ***************************************************************	
 * 	fp0 = destination operand					
 * 	d0  = type of instruction that took exception			
 * 	fsave frame = source operand					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = result							
 * 	fp1 = EXOP							
 * 									
 *  ALGORITHM ***********************************************************	
 * 	An exception occurred on the last instruction of the		
 *  transcendental emulation. hopefully, this won't be happening much	
 *  because it will be VERY slow.						
 * 	The only exceptions capable of passing through here are		
 *  Overflow, Underflow, and Unsupported Data Type.			
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		fgen_except
fgen_except:
	cmpi.b		#0x7,0x3(sp)		/*  is exception UNSUPP? */
	beq.b		fge_unsupp		/*  yes */

	move.b		#NORM,STAG(a6)

fge_cont:
	move.b		#NORM,DTAG(a6)

/*  ok, I have a problem with putting the dst op at FP_DST. the emulation */
/*  routines aren't supposed to alter the operands but we've just squashed */
/*  FP_DST here... */

/*  8/17/93 - this turns out to be more of a "cleanliness" standpoint */
/*  then a potential bug. to begin with, only the dyadic functions */
/*  frem,fmod, and fscale would get the dst trashed here. But, for */
/*  the 060SP, the FP_DST is never used again anyways. */
	fmovem.x	fp0,FP_DST(a6)	/*  dst op is in fp0 */

	lea		0x4(sp),a0		/*  pass: ptr to src op */
	lea		FP_DST(a6),a1		/*  pass: ptr to dst op */

	cmpi.b		#FMOV_OP,d1
	beq.b		fge_fin			/*  it was an "fmov" */
	cmpi.b		#FADD_OP,d1
	beq.b		fge_fadd		/*  it was an "fadd" */
fge_fmul:
	bsr.l		fmul
	rts
fge_fadd:
	bsr.l		fadd
	rts
fge_fin:
	bsr.l		fin
	rts

fge_unsupp:
	move.b		#DENORM,STAG(a6)
	bra.b		fge_cont
