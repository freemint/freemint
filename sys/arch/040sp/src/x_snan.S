/*
 * 	x_snan.sa 3.3 7/1/91
 *
 *  fpsp_snan --- FPSP handler for signalling NAN exception
 *
 *  SNAN for float -> integer conversions (integer conversion of
 *  an SNAN) is a non-maskable run-time exception.
 *
 *  For trap disabled the 040 does the following:
 *  If the dest data format is s, d, or x, then the SNAN bit in the NAN
 *  is set to one and the resulting non-signaling NAN (truncated if
 *  necessary) is transferred to the dest.  If the dest format is b, w,
 *  or l, then garbage is written to the dest (actually the upper 32 bits
 *  of the mantissa are sent to the integer unit).
 *
 *  For trap enabled the 040 does the following:
 *  If the inst is move_out, then the results are the same as for trap
 *  disabled with the exception posted.  If the instruction is not move_
 *  out, the dest. is not modified, and the exception is posted.
 *
 *
 * 		Copyright (C) Motorola, Inc. 1990
 * 			All Rights Reserved
 *
 *        For details on the license for this file, please see the
 *        file, README, in this same directory.
 */

#include "mint/asmdefs.h"
	.include "fpsp.h"

	.xref	get_fline
	.xref	mem_write
	.xref	real_snan
	.xref	real_inex
	.xref	fpsp_done
	.xref	reg_dest

	.text

	.globl	fpsp_snan
fpsp_snan:
	link		a6,#-LOCAL_SIZE
	fsave		-(a7)
	movem.l		d0-d1/a0-a1,USER_DA(a6)
	fmovem.x	fp0-fp3,USER_FP0(a6)
	fmovem.l	fpcr/fpsr/fpiar,USER_FPCR(a6)

/*
 *  Check if trap enabled
 */
	btst.b		#snan_bit,FPCR_ENABLE(a6)
	bne.s		ena		/* If enabled, then branch */

	bsr			move_out	/* else SNAN disabled */
/*
 *  It is possible to have an inex1 exception with the
 *  snan.  If the inex enable bit is set in the FPCR, and either
 *  inex2 or inex1 occurred, we must clean up and branch to the
 *  real inex handler.
 */
ck_inex_snan:
	move.b	FPCR_ENABLE(a6),d0
	and.b	FPSR_EXCEPT(a6),d0
	andi.b	#0x3,d0
	beq.s	end_snan
/*
 *  Inexact enabled and reported, and we must take an inexact exception.
 */
take_inex_snan:
	move.b		#INEX_VEC,EXC_VEC+1(a6)
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			real_inex
/*
 *  SNAN is enabled.  Check if inst is move_out.
 *  Make any corrections to the 040 output as necessary.
 */
ena:
	btst.b		#5,CMDREG1B(a6) /* if set, inst is move out */
	beq		not_out

	bsr			move_out

report_snan:
	move.b		(a7),VER_TMP(a6)
	cmpi.b		#VER_40,(a7)	/* test for orig unimp frame */
	bne.s		ck_rev_snan
	moveq.l		#13,d0		/* need to zero 14 lwords */
	bra.s		rep_con
ck_rev_snan:
	moveq.l		#11,d0		/* need to zero 12 lwords */
rep_con:
	clr.l		(a7)
loop1_snan:
	clr.l		-(a7)		/* clear and dec a7 */
	dbra		d0,loop1_snan
	move.b		VER_TMP(a6),(a7) /* format a busy frame */
	move.b		#BUSY_SIZE-4,1(a7)
	move.l		USER_FPSR(a6),FPSR_SHADOW(a6)
	or.l		#sx_mask,E_BYTE(a6)
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			real_snan
/*
 *  Exit snan handler by expanding the unimp frame into a busy frame
 */
end_snan:
	bclr.b		#E1,E_BYTE(a6)

	move.b		(a7),VER_TMP(a6)
	cmpi.b		#VER_40,(a7)	/* test for orig unimp frame */
	bne.s		ck_rev2
	moveq.l		#13,d0		/* need to zero 14 lwords */
	bra.s		rep_con2
ck_rev2:
	moveq.l		#11,d0		/* need to zero 12 lwords */
rep_con2:
	clr.l		(a7)
loop2_snan:
	clr.l		-(a7)		/* clear and dec a7 */
	dbra		d0,loop2_snan
	move.b		VER_TMP(a6),(a7) /* format a busy frame */
	move.b		#BUSY_SIZE-4,1(a7) /* write busy size */
	move.l		USER_FPSR(a6),FPSR_SHADOW(a6)
	or.l		#sx_mask,E_BYTE(a6)
	movem.l		USER_DA(a6),d0-d1/a0-a1
	fmovem.x	USER_FP0(a6),fp0-fp3
	fmovem.l	USER_FPCR(a6),fpcr/fpsr/fpiar
	frestore	(a7)+
	unlk		a6
	bra			fpsp_done

/*
 *  Move_out
 */
move_out:
	move.l		EXC_EA(a6),a0	/* get <ea> from exc frame */

	bfextu		CMDREG1B(a6){3:3},d0 /* move rx field to d0{2:0} */
	tst.l		d0		/* check for long */
	beq.s		sto_long	/* branch if move_out long */

	cmpi.l		#4,d0		/* check for word */
	beq.s		sto_word	/* branch if move_out word */

	cmpi.l		#6,d0		/* check for byte */
	beq.s		sto_byte	/* branch if move_out byte */

/*
 *  Not byte, word or long
 */
	rts
/*
 *  Get the 32 most significant bits of etemp mantissa
 */
sto_long:
	move.l		ETEMP_HI(a6),d1
	move.l		#4,d0		/* load byte count */
/*
 *  Set signalling nan bit
 */
	bset.l		#30,d1
/*
 *  Store to the users destination address
 */
	tst.l		a0		/* check if <ea> is 0 */
	beq.s		wrt_dn_snan		/* destination is a data register */

	move.l		d1,-(a7)	/* move the snan onto the stack */
	move.l		a0,a1		/* load dest addr into a1 */
	move.l		a7,a0		/* load src addr of snan into a0 */
	bsr			mem_write	/* write snan to user memory */
	move.l		(a7)+,d1	/* clear off stack */
	rts
/*
 *  Get the 16 most significant bits of etemp mantissa
 */
sto_word:
	move.l		ETEMP_HI(a6),d1
	move.l		#2,d0		/* load byte count */
/*
 *  Set signalling nan bit
 */
	bset.l		#30,d1
/*
 *  Store to the users destination address
 */
	tst.l		a0		/* check if <ea> is 0 */
	beq.s		wrt_dn_snan		/* destination is a data register */

	move.l		d1,-(a7)	/* move the snan onto the stack */
	move.l		a0,a1		/* load dest addr into a1 */
	move.l		a7,a0		/* point to low word */
	bsr			mem_write	/* write snan to user memory */
	move.l		(a7)+,d1	/* clear off stack */
	rts
/*
 *  Get the 8 most significant bits of etemp mantissa
 */
sto_byte:
	move.l		ETEMP_HI(a6),d1
	move.l		#1,d0		/* load byte count */
/*
 *  Set signalling nan bit
 */
	bset.l		#30,d1
/*
 *  Store to the users destination address
 */
	tst.l		a0		/* check if <ea> is 0 */
	beq.s		wrt_dn_snan		/* destination is a data register */
	move.l		d1,-(a7)	/* move the snan onto the stack */
	move.l		a0,a1		/* load dest addr into a1 */
	move.l		a7,a0		/* point to source byte */
	bsr			mem_write	/* write snan to user memory */
	move.l		(a7)+,d1	/* clear off stack */
	rts

/*
 * 	wrt_dn --- write to a data register
 *
 * 	We get here with D1 containing the data to write and D0 the
 * 	number of bytes to write: 1=byte,2=word,4=long.
 */
wrt_dn_snan:
	move.l		d1,L_SCR1(a6)	/* data */
	move.l		d0,-(a7)	/* size */
	bsr			get_fline	/* returns fline word in d0 */
	move.l		d0,d1
	andi.l		#0x7,d1		/* d1 now holds register number */
	move.l		(sp)+,d0	/* get original size */
	cmpi.l		#4,d0
	beq.s		wrt_long
	cmpi.l		#2,d0
	bne.s		wrt_byte
wrt_word:
	ori.l		#0x8,d1
	bra			reg_dest
wrt_long:
	ori.l		#0x10,d1
	bra			reg_dest
wrt_byte:
	bra			reg_dest
/*
 *  Check if it is a src nan or dst nan
 */
not_out:
	move.l		DTAG(a6),d0
	bfextu		d0{0:3},d0	/* isolate dtag in lsbs */

	cmpi.b		#3,d0		/* check for nan in destination */
	bne.s		issrc		/* destination nan has priority */
dst_nan_snan:
	btst.b		#6,FPTEMP_HI(a6) /* check if dest nan is an snan */
	bne.s		issrc		/* no, so check source for snan */
	move.w		FPTEMP_EX(a6),d0
	bra.s		cont_snan
issrc:
	move.w		ETEMP_EX(a6),d0
cont_snan:
	btst.l		#15,d0		/* test for sign of snan */
	beq.s		clr_neg
	bset.b		#neg_bit,FPSR_CC(a6)
	bra			report_snan
clr_neg:
	bclr.b		#neg_bit,FPSR_CC(a6)
	bra			report_snan
