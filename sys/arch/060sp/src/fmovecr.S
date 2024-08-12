/*
 *  smovecr(): returns the ROM constant at the offset specified in d1	
 * 	    rounded to the mode and precision specified in d0.		
 * 									
 *  INPUT	***************************************************************	
 * 	d0 = rnd prec,mode						
 * 	d1 = ROM offset							
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = the ROM constant rounded to the user's rounding mode,prec	
 * 									
 */

	.include "hdr.fpu"

	.text

	.globl		smovecr
smovecr:
	move.l		d1,-(sp)		/*  save rom offset for a sec */

	lsr.b		#0x4,d0		/*  shift ctrl bits to lo */
	move.l		d0,d1			/*  make a copy */
	andi.w		#0x3,d1		/*  extract rnd mode */
	andi.w		#0xc,d0		/*  extract rnd prec */
	swap		d0			/*  put rnd prec in hi */
	move.w		d1,d0			/*  put rnd mode in lo */

	move.l		(sp)+,d1		/*  get rom offset */

/*
 * check range of offset
 */
	tst.b		d1			/*  if zero, offset is to pi */
	beq.b		pi_tbl			/*  it is pi */
	cmpi.b		#0x0a,d1		/*  check range $01 - $0a */
	ble.b		z_val			/*  if in this range, return zero */
	cmpi.b		#0x0e,d1		/*  check range $0b - $0e */
	ble.b		sm_tbl			/*  valid constants in this range */
	cmpi.b		#0x2f,d1		/*  check range $10 - $2f */
	ble.b		z_val			/*  if in this range, return zero */
	cmpi.b		#0x3f,d1		/*  check range $30 - $3f */
	ble.b		bg_tbl			/*  valid constants in this range */

z_val:
	bra.l		ld_pzero		/*  return a zero */

/*
 * the answer is PI rounded to the proper precision.
 *
 * fetch a pointer to the answer table relating to the proper rounding
 * precision.
 */
pi_tbl:
	tst.b		d0			/*  is rmode RN? */
	bne.b		pi_not_rn		/*  no */
pi_rn:
	lea.l		(PIRN).l(pc),a0		/*  yes; load PI RN table addr */
	bra.w		set_finx
pi_not_rn:
	cmpi.b		#rp_mode,d0		/*  is rmode RP? */
	beq.b		pi_rp			/*  yes */
pi_rzrm:
	lea.l		(PIRZRM).l(pc),a0		/*  no; load PI RZ,RM table addr */
	bra.b		set_finx
pi_rp:
	lea.l		(PIRP).l(pc),a0		/*  load PI RP table addr */
	bra.b		set_finx

/*
 * the answer is one of:
 *	$0B	log10(2)	(inexact)
 *	$0C	e		(inexact)
 *	$0D	log2(e)		(inexact)
 *	$0E	log10(e)	(exact)
 *
 * fetch a pointer to the answer table relating to the proper rounding
 * precision.
 */
sm_tbl:
	subi.b		#0xb,d1		/*  make offset in 0-4 range */
	tst.b		d0			/*  is rmode RN? */
	bne.b		sm_not_rn		/*  no */
sm_rn:
	lea.l		(SMALRN).l(pc),a0		/*  yes; load RN table addr */
sm_tbl_cont:
	cmpi.b		#0x2,d1		/*  is result log10(e)? */
	ble.b		set_finx		/*  no; answer is inexact */
	bra.b		no_finx			/*  yes; answer is exact */
sm_not_rn:
	cmpi.b		#rp_mode,d0		/*  is rmode RP? */
	beq.b		sm_rp			/*  yes */
sm_rzrm:
	lea.l		(SMALRZRM).l(pc),a0	/*  no; load RZ,RM table addr */
	bra.b		sm_tbl_cont
sm_rp:
	lea.l		(SMALRP).l(pc),a0		/*  load RP table addr */
	bra.b		sm_tbl_cont

/*
 * the answer is one of:
 *	$30	ln(2)		(inexact)
 *	$31	ln(10)		(inexact)
 *	$32	10^0		(exact)
 *	$33	10^1		(exact)
 *	$34	10^2		(exact)
 *	$35	10^4		(exact)
 *	$36	10^8		(exact)
 *	$37	10^16		(exact)
 *	$38	10^32		(inexact)
 *	$39	10^64		(inexact)
 *	$3A	10^128		(inexact)
 *	$3B	10^256		(inexact)
 *	$3C	10^512		(inexact)
 *	$3D	10^1024		(inexact)
 *	$3E	10^2048		(inexact)
 *	$3F	10^4096		(inexact)
 *
 * fetch a pointer to the answer table relating to the proper rounding
 * precision.
 */
bg_tbl:
	subi.b		#0x30,d1		/*  make offset in 0-f range */
	tst.b		d0			/*  is rmode RN? */
	bne.b		bg_not_rn		/*  no */
bg_rn:
	lea.l		(BIGRN).l(pc),a0		/*  yes; load RN table addr */
bg_tbl_cont:
	cmpi.b		#0x1,d1		/*  is offset <= $31? */
	ble.b		set_finx		/*  yes; answer is inexact */
	cmpi.b		#0x7,d1		/*  is $32 <= offset <= $37? */
	ble.b		no_finx			/*  yes; answer is exact */
	bra.b		set_finx		/*  no; answer is inexact */
bg_not_rn:
	cmpi.b		#rp_mode,d0		/*  is rmode RP? */
	beq.b		bg_rp			/*  yes */
bg_rzrm:
	lea.l		(BIGRZRM).l(pc),a0	/*  no; load RZ,RM table addr */
	bra.b		bg_tbl_cont
bg_rp:
	lea.l		(BIGRP).l(pc),a0		/*  load RP table addr */
	bra.b		bg_tbl_cont

/*  answer is inexact, so set INEX2 and AINEX in the user's FPSR. */
set_finx:
	ori.l		#inx2a_mask,USER_FPSR(a6) /*  set INEX2/AINEX */
no_finx:
	mulu.w		#0xc,d1		/*  offset points into tables */
	swap		d0			/*  put rnd prec in lo word */
	tst.b		d0			/*  is precision extended? */

	bne.b		not_ext			/*  if xprec, do not call round */

/*  Precision is extended */
	fmovem.x	(a0,d1.w),fp0	/*  return result in fp0 */
	rts

/*  Precision is single or double */
not_ext:
	swap		d0			/*  rnd prec in upper word */

/*  call round() to round the answer to the proper precision. */
/*  exponents out of range for single or double DO NOT cause underflow */
/*  or overflow. */
	move.w		0x0(a0,d1.w),FP_SCR1_EX(a6) /*  load first word */
	move.l		0x4(a0,d1.w),FP_SCR1_HI(a6) /*  load second word */
	move.l		0x8(a0,d1.w),FP_SCR1_LO(a6) /*  load third word */
	move.l		d0,d1
	clr.l		d0			/*  clear g,r,s */
	lea		FP_SCR1(a6),a0	/*  pass ptr to answer */
	clr.w		LOCAL_SGN(a0)		/*  sign always positive */
	bsr.l		_round			/*  round the mantissa */

	fmovem.x	(a0),fp0		/*  return rounded result in fp0 */
	rts

	/* .balignw		4,0x51fc */
	.dc.w 0x51fc

PIRN:	.dc.l		0x40000000,0xc90fdaa2,0x2168c235	/*  pi */
PIRZRM:	.dc.l		0x40000000,0xc90fdaa2,0x2168c234	/*  pi */
PIRP:	.dc.l		0x40000000,0xc90fdaa2,0x2168c235	/*  pi */

SMALRN:	.dc.l		0x3ffd0000,0x9a209a84,0xfbcff798	/*  log10(2) */
	.dc.l		0x40000000,0xadf85458,0xa2bb4a9a	/*  e */
	.dc.l		0x3fff0000,0xb8aa3b29,0x5c17f0bc	/*  log2(e) */
	.dc.l		0x3ffd0000,0xde5bd8a9,0x37287195	/*  log10(e) */
	.dc.l		0x00000000,0x00000000,0x00000000	/*  0.0 */

SMALRZRM:
	.dc.l		0x3ffd0000,0x9a209a84,0xfbcff798	/*  log10(2) */
	.dc.l		0x40000000,0xadf85458,0xa2bb4a9a	/*  e */
	.dc.l		0x3fff0000,0xb8aa3b29,0x5c17f0bb	/*  log2(e) */
	.dc.l		0x3ffd0000,0xde5bd8a9,0x37287195	/*  log10(e) */
	.dc.l		0x00000000,0x00000000,0x00000000	/*  0.0 */

SMALRP:	.dc.l		0x3ffd0000,0x9a209a84,0xfbcff799	/*  log10(2) */
	.dc.l		0x40000000,0xadf85458,0xa2bb4a9b	/*  e */
	.dc.l		0x3fff0000,0xb8aa3b29,0x5c17f0bc	/*  log2(e) */
	.dc.l		0x3ffd0000,0xde5bd8a9,0x37287195	/*  log10(e) */
	.dc.l		0x00000000,0x00000000,0x00000000	/*  0.0 */

BIGRN:	.dc.l		0x3ffe0000,0xb17217f7,0xd1cf79ac	/*  ln(2) */
	.dc.l		0x40000000,0x935d8ddd,0xaaa8ac17	/*  ln(10) */

	.dc.l		0x3fff0000,0x80000000,0x00000000	/*  10 ^ 0 */
	.dc.l		0x40020000,0xA0000000,0x00000000	/*  10 ^ 1 */
	.dc.l		0x40050000,0xC8000000,0x00000000	/*  10 ^ 2 */
	.dc.l		0x400C0000,0x9C400000,0x00000000	/*  10 ^ 4 */
	.dc.l		0x40190000,0xBEBC2000,0x00000000	/*  10 ^ 8 */
	.dc.l		0x40340000,0x8E1BC9BF,0x04000000	/*  10 ^ 16 */
	.dc.l		0x40690000,0x9DC5ADA8,0x2B70B59E	/*  10 ^ 32 */
	.dc.l		0x40D30000,0xC2781F49,0xFFCFA6D5	/*  10 ^ 64 */
	.dc.l		0x41A80000,0x93BA47C9,0x80E98CE0	/*  10 ^ 128 */
	.dc.l		0x43510000,0xAA7EEBFB,0x9DF9DE8E	/*  10 ^ 256 */
	.dc.l		0x46A30000,0xE319A0AE,0xA60E91C7	/*  10 ^ 512 */
	.dc.l		0x4D480000,0xC9767586,0x81750C17	/*  10 ^ 1024 */
	.dc.l		0x5A920000,0x9E8B3B5D,0xC53D5DE5	/*  10 ^ 2048 */
	.dc.l		0x75250000,0xC4605202,0x8A20979B	/*  10 ^ 4096 */

BIGRZRM:
	.dc.l		0x3ffe0000,0xb17217f7,0xd1cf79ab	/*  ln(2) */
	.dc.l		0x40000000,0x935d8ddd,0xaaa8ac16	/*  ln(10) */

	.dc.l		0x3fff0000,0x80000000,0x00000000	/*  10 ^ 0 */
	.dc.l		0x40020000,0xA0000000,0x00000000	/*  10 ^ 1 */
	.dc.l		0x40050000,0xC8000000,0x00000000	/*  10 ^ 2 */
	.dc.l		0x400C0000,0x9C400000,0x00000000	/*  10 ^ 4 */
	.dc.l		0x40190000,0xBEBC2000,0x00000000	/*  10 ^ 8 */
	.dc.l		0x40340000,0x8E1BC9BF,0x04000000	/*  10 ^ 16 */
	.dc.l		0x40690000,0x9DC5ADA8,0x2B70B59D	/*  10 ^ 32 */
	.dc.l		0x40D30000,0xC2781F49,0xFFCFA6D5	/*  10 ^ 64 */
	.dc.l		0x41A80000,0x93BA47C9,0x80E98CDF	/*  10 ^ 128 */
	.dc.l		0x43510000,0xAA7EEBFB,0x9DF9DE8D	/*  10 ^ 256 */
	.dc.l		0x46A30000,0xE319A0AE,0xA60E91C6	/*  10 ^ 512 */
	.dc.l		0x4D480000,0xC9767586,0x81750C17	/*  10 ^ 1024 */
	.dc.l		0x5A920000,0x9E8B3B5D,0xC53D5DE4	/*  10 ^ 2048 */
	.dc.l		0x75250000,0xC4605202,0x8A20979A	/*  10 ^ 4096 */

BIGRP:
	.dc.l		0x3ffe0000,0xb17217f7,0xd1cf79ac	/*  ln(2) */
	.dc.l		0x40000000,0x935d8ddd,0xaaa8ac17	/*  ln(10) */

	.dc.l		0x3fff0000,0x80000000,0x00000000	/*  10 ^ 0 */
	.dc.l		0x40020000,0xA0000000,0x00000000	/*  10 ^ 1 */
	.dc.l		0x40050000,0xC8000000,0x00000000	/*  10 ^ 2 */
	.dc.l		0x400C0000,0x9C400000,0x00000000	/*  10 ^ 4 */
	.dc.l		0x40190000,0xBEBC2000,0x00000000	/*  10 ^ 8 */
	.dc.l		0x40340000,0x8E1BC9BF,0x04000000	/*  10 ^ 16 */
	.dc.l		0x40690000,0x9DC5ADA8,0x2B70B59E	/*  10 ^ 32 */
	.dc.l		0x40D30000,0xC2781F49,0xFFCFA6D6	/*  10 ^ 64 */
	.dc.l		0x41A80000,0x93BA47C9,0x80E98CE0	/*  10 ^ 128 */
	.dc.l		0x43510000,0xAA7EEBFB,0x9DF9DE8E	/*  10 ^ 256 */
	.dc.l		0x46A30000,0xE319A0AE,0xA60E91C7	/*  10 ^ 512 */
	.dc.l		0x4D480000,0xC9767586,0x81750C18	/*  10 ^ 1024 */
	.dc.l		0x5A920000,0x9E8B3B5D,0xC53D5DE5	/*  10 ^ 2048 */
	.dc.l		0x75250000,0xC4605202,0x8A20979B	/*  10 ^ 4096 */
