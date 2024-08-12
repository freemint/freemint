/*
 * 	do_func.sa 3.4 2/18/91
 *
 *  Do_func performs the unimplemented operation.  The operation
 *  to be performed is determined from the lower 7 bits of the
 *  extension word (except in the case of fmovecr and fsincos).
 *  The opcode and tag bits form an index into a jump table in
 *  tbldo.sa.  Cases of zero, infinity and NaN are handled in
 *  do_func by forcing the default result.  Normalized and
 *  denormalized (there are no unnormalized numbers at this
 *  point) are passed onto the emulation code.
 *
 *  CMDREG1B and STAG are extracted from the fsave frame
 *  and combined to form the table index.  The function called
 *  will start with a0 pointing to the ETEMP operand.  Dyadic
 *  functions can find FPTEMP at -12(a0).
 *
 *  Called functions return their result in fp0.  Sincos returns
 *  sin(x) in fp0 and cos(x) in fp1.
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

	.xref	t_dz2
	.xref	t_operr
	.xref	t_inx2
	.xref	t_resdnrm
	.xref	dst_nan_ker
	.xref	src_nan_ker
	.xref	nrm_set
	.xref	sto_cos

	.xref	tblpre
	.xref	slognp1,slogn,slog10,slog2
	.xref	slognd,slog10d,slog2d
	.xref	smod,srem
	.xref	sscale
	.xref	smovcr

	.xref PIBY2

	.text
PONE:	.dc.l	0x3fff0000,0x80000000,0x00000000	/* +1 */
MONE:	.dc.l	0xbfff0000,0x80000000,0x00000000	/* -1 */
PZERO:	.dc.l	0x00000000,0x00000000,0x00000000	/* +0 */
MZERO:	.dc.l	0x80000000,0x00000000,0x00000000	/* -0 */
PINF:	.dc.l	0x7fff0000,0x00000000,0x00000000	/* +inf */
MINF:	.dc.l	0xffff0000,0x00000000,0x00000000	/* -inf */
QNAN:	.dc.l	0x7fff0000,0xffffffff,0xffffffff	/* non-signaling nan */
MPIBY2:  .dc.l	0xbFFF0000,0xC90FDAA2,0x2168C235	/* -PI/2 */
	.text

	.globl	do_func
do_func:
	clr.b	CU_ONLY(a6)
/*
 *  Check for fmovecr.  It does not follow the format of fp gen
 *  unimplemented instructions.  The test is on the upper 6 bits;
 *  if they are $17, the inst is fmovecr.  Call entry smovcr
 *  directly.
 */
	bfextu	CMDREG1B(a6){0:6},d0 /* get opclass and src fields */
	cmpi.l	#0x17,d0		/* if op class and size fields are $17, */
 				/* it is FMOVECR; if not, continue */
	bne.s	not_fmovecr_df
	bra	smovcr		/* fmovecr; jmp directly to emulation */

not_fmovecr_df:
	move.w	CMDREG1B(a6),d0
	andi.l	#0x7F,d0
	cmpi.l	#0x38,d0		/* if the extension is >= $38, */
	bge.s	serror		/* it is illegal */
	bfextu	STAG(a6){0:3},d1
	lsl.l	#3,d0		/* make room for STAG */
	add.l	d1,d0		/* combine for final index into table */
	lea.l	tblpre,a1	/* start of monster jump table */
	move.l	(a1,d0.w*4),a1	/* real target address */
	lea.l	ETEMP(a6),a0	/* a0 is pointer to src op */
	move.l	USER_FPCR(a6),d1
	andi.l	#0xFF,d1		/*  discard all but rounding mode/prec */
	fmove.l	#0,fpcr
	jmp	(a1)
/*
 * 	ERROR
 */
	.globl	serror
serror:
	st	STORE_FLG(a6)
	rts
/*
 *  These routines load forced values into fp0.  They are called
 *  by index into tbldo.
 *
 *  Load a signed zero to fp0 and set inex2/ainex
 */
	.globl	snzrinx
snzrinx:
	btst.b	#sign_bit,LOCAL_EX(a0)	/* get sign of source operand */
	bne.s	ld_mzinx	/* if negative, branch */
	bsr	ld_pzero	/* bsr so we can return and set inx */
	bra	t_inx2		/* now, set the inx for the next inst */
ld_mzinx:
	bsr	ld_mzero	/* if neg, load neg zero, return here */
	bra	t_inx2		/* now, set the inx for the next inst */
/*
 *  Load a signed zero to fp0; do not set inex2/ainex
 */
	.globl	szero
szero:
	btst.b	#sign_bit,LOCAL_EX(a0) /* get sign of source operand */
	bne	ld_mzero	/* if neg, load neg zero */
	bra	ld_pzero	/* load positive zero */
/*
 *  Load a signed infinity to fp0; do not set inex2/ainex
 */
	.globl	sinf
sinf:
	btst.b	#sign_bit,LOCAL_EX(a0)	/* get sign of source operand */
	bne	ld_minf			/* if negative branch */
	bra	ld_pinf
/*
 *  Load a signed one to fp0; do not set inex2/ainex
 */
	.globl	sone
sone:
	btst.b	#sign_bit,LOCAL_EX(a0)	/* check sign of source */
	bne	ld_mone
	bra	ld_pone
/*
 *  Load a signed pi/2 to fp0; do not set inex2/ainex
 */
	.globl	spi_2
spi_2:
	btst.b	#sign_bit,LOCAL_EX(a0)	/* check sign of source */
	bne	ld_mpi2
	bra	ld_ppi2
/*
 *  Load either a +0 or +inf for plus/minus operand
 */
	.globl	szr_inf
szr_inf:
	btst.b	#sign_bit,LOCAL_EX(a0)	/* check sign of source */
	bne	ld_pzero
	bra	ld_pinf
/*
 *  Result is either an operr or +inf for plus/minus operand
 *  [Used by slogn, slognp1, slog10, and slog2]
 */
	.globl	sopr_inf
sopr_inf:
	btst.b	#sign_bit,LOCAL_EX(a0)	/* check sign of source */
	bne	t_operr
	bra	ld_pinf

/*
 * 	FLOGNP1
 */
	.globl	sslognp1
sslognp1:
	fmove.x (a0),fp0
	fcmp.b	#-1,fp0
	fbgt	slognp1
	fbeq	t_dz2		/* if = -1, divide by zero exception */
	fmove.l	#0,fpsr		/* clr N flag */
	bra	t_operr		/* take care of operands < -1 */

/*
 * 	FETOXM1
 */
	.globl	setoxm1i
setoxm1i:
	btst.b	#sign_bit,LOCAL_EX(a0)	/* check sign of source */
	bne	ld_mone
	bra	ld_pinf

/*
 * 	FLOGN
 *
 *  Test for 1.0 as an input argument, returning +zero.  Also check
 *  the sign and return operr if negative.
 */
	.globl	sslogn
sslogn:
	btst.b	#sign_bit,LOCAL_EX(a0)
	bne	t_operr		/* take care of operands < 0 */
	cmpi.w	#0x3fff,LOCAL_EX(a0) /* test for 1.0 input */
	bne	slogn
	cmpi.l	#0x80000000,LOCAL_HI(a0)
	bne	slogn
	tst.l	LOCAL_LO(a0)
	bne	slogn
	fmove.x	PZERO.w(pc),fp0
	rts

	.globl	sslognd
sslognd:
	btst.b	#sign_bit,LOCAL_EX(a0)
	beq	slognd
	bra	t_operr		/* take care of operands < 0 */

/*
 * 	FLOG10
 */
	.globl	sslog10
sslog10:
	btst.b	#sign_bit,LOCAL_EX(a0)
	bne	t_operr		/* take care of operands < 0 */
	cmpi.w	#0x3fff,LOCAL_EX(a0) /* test for 1.0 input */
	bne	slog10
	cmpi.l	#0x80000000,LOCAL_HI(a0)
	bne	slog10
	tst.l	LOCAL_LO(a0)
	bne	slog10
	fmove.x	PZERO.w(pc),fp0
	rts

	.globl	sslog10d
sslog10d:
	btst.b	#sign_bit,LOCAL_EX(a0)
	beq	slog10d
	bra	t_operr		/* take care of operands < 0 */

/*
 * 	FLOG2
 */
	.globl	sslog2
sslog2:
	btst.b	#sign_bit,LOCAL_EX(a0)
	bne	t_operr		/* take care of operands < 0 */
	cmpi.w	#0x3fff,LOCAL_EX(a0) /* test for 1.0 input */
	bne	slog2
	cmpi.l	#0x80000000,LOCAL_HI(a0)
	bne	slog2
	tst.l	LOCAL_LO(a0)
	bne	slog2
	fmove.x	PZERO.w(pc),fp0
	rts

	.globl	sslog2d
sslog2d:
	btst.b	#sign_bit,LOCAL_EX(a0)
	beq	slog2d
	bra	t_operr		/* take care of operands < 0 */

/*
 * 	FMOD
 */
pmodt:
				/* $21 fmod */
				/* dtag,stag */
	.dc.l	smod		/*   00,00  norm,norm = normal */
	.dc.l	smod_oper	/*   00,01  norm,zero = nan with operr */
	.dc.l	smod_fpn	/*   00,10  norm,inf  = fpn */
	.dc.l	smod_snan	/*   00,11  norm,nan  = nan */
	.dc.l	smod_zro	/*   01,00  zero,norm = +-zero */
	.dc.l	smod_oper	/*   01,01  zero,zero = nan with operr */
	.dc.l	smod_zro	/*   01,10  zero,inf  = +-zero */
	.dc.l	smod_snan	/*   01,11  zero,nan  = nan */
	.dc.l	smod_oper	/*   10,00  inf,norm  = nan with operr */
	.dc.l	smod_oper	/*   10,01  inf,zero  = nan with operr */
	.dc.l	smod_oper	/*   10,10  inf,inf   = nan with operr */
	.dc.l	smod_snan	/*   10,11  inf,nan   = nan */
	.dc.l	smod_dnan	/*   11,00  nan,norm  = nan */
	.dc.l	smod_dnan	/*   11,01  nan,zero  = nan */
	.dc.l	smod_dnan	/*   11,10  nan,inf   = nan */
	.dc.l	smod_dnan	/*   11,11  nan,nan   = nan */

	.globl	pmod
pmod:
	clr.b	FPSR_QBYTE(a6) /*  clear quotient field */
	bfextu	STAG(a6){0:3},d0 /* stag = d0 */
	bfextu	DTAG(a6){0:3},d1 /* dtag = d1 */

/*
 *  Alias extended denorms to norms for the jump table.
 */
	bclr.l	#2,d0
	bclr.l	#2,d1

	lsl.b	#2,d1
	or.b	d0,d1		/* d1{3:2} = dtag, d1{1:0} = stag */
/*
 * 				;Tag values:
 * 				;00 = norm or denorm
 * 				;01 = zero
 * 				;10 = inf
 * 				;11 = nan
 */
	lea	pmodt.w(pc),a1
	move.l	(a1,d1.w*4),a1
	jmp	(a1)

smod_snan:
	bra	src_nan_ker
smod_dnan:
	bra	dst_nan_ker
smod_oper:
	bra	t_operr
smod_zro:
	move.b	ETEMP(a6),d1	/* get sign of src op */
	move.b	FPTEMP(a6),d0	/* get sign of dst op */
	eor.b	d0,d1		/* get exor of sign bits */
	btst.l	#7,d1		/* test for sign */
	beq.s	smod_zsn	/* if clr, do not set sign big */
	bset.b	#q_sn_bit,FPSR_QBYTE(a6) /* set q-byte sign bit */
smod_zsn:
	btst.l	#7,d0		/* test if + or - */
	beq	ld_pzero	/* if pos then load +0 */
	bra	ld_mzero	/* else neg load -0 */

smod_fpn:
	move.b	ETEMP(a6),d1	/* get sign of src op */
	move.b	FPTEMP(a6),d0	/* get sign of dst op */
	eor.b	d0,d1		/* get exor of sign bits */
	btst.l	#7,d1		/* test for sign */
	beq.s	smod_fsn	/* if clr, do not set sign big */
	bset.b	#q_sn_bit,FPSR_QBYTE(a6) /* set q-byte sign bit */
smod_fsn:
	tst.b	DTAG(a6)	/* filter out denormal destination case */
	bpl.s	smod_nrm	/*  */
	lea.l	FPTEMP(a6),a0	/* a0<- addr(FPTEMP) */
	bra	t_resdnrm	/* force UNFL(but exact) result */
smod_nrm:
	fmove.l USER_FPCR(a6),fpcr /* use user's rmode and precision */
	fmove.x FPTEMP(a6),fp0	/* return dest to fp0 */
	rts

/*
 * 	FREM
 */
premt:
				/* 25 frem */
				/* dtag,stag */
	.dc.l	srem		/*   00,00  norm,norm = normal */
	.dc.l	srem_oper	/*   00,01  norm,zero = nan with operr */
	.dc.l	srem_fpn	/*   00,10  norm,inf  = fpn */
	.dc.l	srem_snan	/*   00,11  norm,nan  = nan */
	.dc.l	srem_zro	/*   01,00  zero,norm = +-zero */
	.dc.l	srem_oper	/*   01,01  zero,zero = nan with operr */
	.dc.l	srem_zro	/*   01,10  zero,inf  = +-zero */
	.dc.l	srem_snan	/*   01,11  zero,nan  = nan */
	.dc.l	srem_oper	/*   10,00  inf,norm  = nan with operr */
	.dc.l	srem_oper	/*   10,01  inf,zero  = nan with operr */
	.dc.l	srem_oper	/*   10,10  inf,inf   = nan with operr */
	.dc.l	srem_snan	/*   10,11  inf,nan   = nan */
	.dc.l	srem_dnan	/*   11,00  nan,norm  = nan */
	.dc.l	srem_dnan	/*   11,01  nan,zero  = nan */
	.dc.l	srem_dnan	/*   11,10  nan,inf   = nan */
	.dc.l	srem_dnan	/*   11,11  nan,nan   = nan */

	.globl	prem
prem:
	clr.b	FPSR_QBYTE(a6)   /* clear quotient field */
	bfextu	STAG(a6){0:3},d0 /* stag = d0 */
	bfextu	DTAG(a6){0:3},d1 /* dtag = d1 */
/*
 *  Alias extended denorms to norms for the jump table.
 */
	bclr	#2,d0
	bclr	#2,d1

	lsl.b	#2,d1
	or.b	d0,d1		/* d1{3:2} = dtag, d1{1:0} = stag */
/*
 * 				;Tag values:
 * 				;00 = norm or denorm
 * 				;01 = zero
 * 				;10 = inf
 * 				;11 = nan
 */
	lea	premt.w(pc),a1
	move.l	(a1,d1.w*4),a1
	jmp	(a1)

srem_snan:
	bra	src_nan_ker
srem_dnan:
	bra	dst_nan_ker
srem_oper:
	bra	t_operr
srem_zro:
	move.b	ETEMP(a6),d1	/* get sign of src op */
	move.b	FPTEMP(a6),d0	/* get sign of dst op */
	eor.b	d0,d1		/* get exor of sign bits */
	btst.l	#7,d1		/* test for sign */
	beq.s	srem_zsn	/* if clr, do not set sign big */
	bset.b	#q_sn_bit,FPSR_QBYTE(a6) /* set q-byte sign bit */
srem_zsn:
	btst.l	#7,d0		/* test if + or - */
	beq	ld_pzero	/* if pos then load +0 */
	bra	ld_mzero	/* else neg load -0 */

srem_fpn:
	move.b	ETEMP(a6),d1	/* get sign of src op */
	move.b	FPTEMP(a6),d0	/* get sign of dst op */
	eor.b	d0,d1		/* get exor of sign bits */
	btst.l	#7,d1		/* test for sign */
	beq.s	srem_fsn	/* if clr, do not set sign big */
	bset.b	#q_sn_bit,FPSR_QBYTE(a6) /* set q-byte sign bit */
srem_fsn:
	tst.b	DTAG(a6)	/* filter out denormal destination case */
	bpl.s	srem_nrm	/*  */
	lea.l	FPTEMP(a6),a0	/* a0<- addr(FPTEMP) */
	bra	t_resdnrm	/* force UNFL(but exact) result */
srem_nrm:
	fmove.l USER_FPCR(a6),fpcr /* use user's rmode and precision */
	fmove.x FPTEMP(a6),fp0	/* return dest to fp0 */
	rts

/*
 * 	FSCALE
 */
pscalet:
				/* $26 fscale */
				/* dtag,stag */
	.dc.l	sscale		/*   00,00  norm,norm = result */
	.dc.l	sscale		/*   00,01  norm,zero = fpn */
	.dc.l	scl_opr		/*   00,10  norm,inf  = nan with operr */
	.dc.l	scl_snan	/*   00,11  norm,nan  = nan */
	.dc.l	scl_zro		/*   01,00  zero,norm = +-zero */
	.dc.l	scl_zro		/*   01,01  zero,zero = +-zero */
	.dc.l	scl_opr		/*   01,10  zero,inf  = nan with operr */
	.dc.l	scl_snan	/*   01,11  zero,nan  = nan */
	.dc.l	scl_inf		/*   10,00  inf,norm  = +-inf */
	.dc.l	scl_inf		/*   10,01  inf,zero  = +-inf */
	.dc.l	scl_opr		/*   10,10  inf,inf   = nan with operr */
	.dc.l	scl_snan	/*   10,11  inf,nan   = nan */
	.dc.l	scl_dnan	/*   11,00  nan,norm  = nan */
	.dc.l	scl_dnan	/*   11,01  nan,zero  = nan */
	.dc.l	scl_dnan	/*   11,10  nan,inf   = nan */
	.dc.l	scl_dnan	/*   11,11  nan,nan   = nan */

	.globl	pscale
pscale:
	bfextu	STAG(a6){0:3},d0 /* stag in d0 */
	bfextu	DTAG(a6){0:3},d1 /* dtag in d1 */
	bclr.l	#2,d0		/* alias  denorm into norm */
	bclr.l	#2,d1		/* alias  denorm into norm */
	lsl.b	#2,d1
	or.b	d0,d1		/* d1{4:2} = dtag, d1{1:0} = stag */
/*
 * 				;dtag values     stag values:
 * 				;000 = norm      00 = norm
 * 				;001 = zero	 01 = zero
 * 				;010 = inf	 10 = inf
 * 				;011 = nan	 11 = nan
 * 				;100 = dnrm
 *
 */
	lea.l	pscalet.w(pc),a1	/* load start of jump table */
	move.l	(a1,d1.w*4),a1	/* load a1 with label depending on tag */
	jmp	(a1)		/* go to the routine */

scl_opr:
	bra	t_operr

scl_dnan:
	bra	dst_nan_ker

scl_zro:
	btst.b	#sign_bit,FPTEMP_EX(a6)	/* test if + or - */
	beq	ld_pzero		/* if pos then load +0 */
	bra	ld_mzero		/* if neg then load -0 */
scl_inf:
	btst.b	#sign_bit,FPTEMP_EX(a6)	/* test if + or - */
	beq	ld_pinf			/* if pos then load +inf */
	bra	ld_minf			/* else neg load -inf */
scl_snan:
	bra	src_nan_ker

/*
 * 	FSINCOS
 */
	.globl	ssincosz
ssincosz:
	btst.b	#sign_bit,ETEMP(a6)	/* get sign */
	beq.s	sincosp
	fmove.x	MZERO.w(pc),fp0
	bra.s	sincoscom
sincosp:
	fmove.x PZERO.w(pc),fp0
sincoscom:
	fmovem.x PONE.w(pc),fp1-fp1	/* do not allow FPSR to be affected */
	bra	sto_cos		/* store cosine result */

	.globl	ssincosi
ssincosi:
	fmove.x QNAN.w(pc),fp1	/* load NAN */
	bsr	sto_cos		/* store cosine result */
	fmove.x QNAN.w(pc),fp0	/* load NAN */
	bra	t_operr

	.globl	ssincosnan
ssincosnan:
	move.l	ETEMP_EX(a6),FP_SCR1(a6)
	move.l	ETEMP_HI(a6),FP_SCR1+4(a6)
	move.l	ETEMP_LO(a6),FP_SCR1+8(a6)
	bset.b	#signan_bit,FP_SCR1+4(a6)
	fmovem.x FP_SCR1(a6),fp1-fp1
	bsr	sto_cos
	bra	src_nan_ker

/*
 *  This code forces default values for the zero, inf, and nan cases
 *  in the transcendentals code.  The CC bits must be set in the
 *  stacked FPSR to be correctly reported.
 */
/* **Returns +PI/2 */
	.globl	ld_ppi2
ld_ppi2:
	fmove.x PIBY2.w(pc),fp0		/* load +pi/2 */
	bra	t_inx2			/* set inex2 exc */

/* **Returns -PI/2 */
	.globl	ld_mpi2
ld_mpi2:
	fmove.x MPIBY2.w(pc),fp0		/* load -pi/2 */
	or.l	#neg_mask,USER_FPSR(a6)	/* set N bit */
	bra	t_inx2			/* set inex2 exc */

/* **Returns +inf */
	.globl	ld_pinf
ld_pinf:
	fmove.x PINF.w(pc),fp0		/* load +inf */
	or.l	#inf_mask,USER_FPSR(a6)	/* set I bit */
	rts

/* **Returns -inf */
	.globl	ld_minf
ld_minf:
	fmove.x MINF.w(pc),fp0		/* load -inf */
	or.l	#neg_mask+inf_mask,USER_FPSR(a6)	/* set N and I bits */
	rts

/* **Returns +1 */
	.globl	ld_pone
ld_pone:
	fmove.x PONE.w(pc),fp0		/* load +1 */
	rts

/* **Returns -1 */
	.globl	ld_mone
ld_mone:
	fmove.x MONE.w(pc),fp0		/* load -1 */
	or.l	#neg_mask,USER_FPSR(a6)	/* set N bit */
	rts

/* **Returns +0 */
	.globl	ld_pzero
ld_pzero:
	fmove.x PZERO.w(pc),fp0		/* load +0 */
	or.l	#z_mask,USER_FPSR(a6)	/* set Z bit */
	rts

/* **Returns -0 */
	.globl	ld_mzero
ld_mzero:
	fmove.x MZERO.w(pc),fp0		/* load -0 */
	or.l	#neg_mask+z_mask,USER_FPSR(a6)	/* set N and Z bits */
	rts
