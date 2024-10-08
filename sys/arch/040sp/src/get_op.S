/*
 * 	get_op.sa 3.6 5/19/92
 *
 * 	get_op.sa 3.5 4/26/91
 *
 *   Description: This routine is called by the unsupported format/data
 *  type exception handler ('unsupp' - vector 55) and the unimplemented
 *  instruction exception handler ('unimp' - vector 11).  'get_op'
 *  determines the opclass (0, 2, or 3) and branches to the
 *  opclass handler routine.  See 68881/2 User's Manual table 4-11
 *  for a description of the opclasses.
 *
 *  For UNSUPPORTED data/format (exception vector 55) and for
 *  UNIMPLEMENTED instructions (exception vector 11) the following
 *  applies:
 *
 *  - For unnormalized numbers (opclass 0, 2, or 3) the
 *  number(s) is normalized and the operand type tag is updated.
 *
 *  - For a packed number (opclass 2) the number is unpacked and the
 *  operand type tag is updated.
 *
 *  - For denormalized numbers (opclass 0 or 2) the number(s) is not
 *  changed but passed to the next module.  The next module for
 *  unimp is do_func, the next module for unsupp is res_func.
 *
 *  For UNSUPPORTED data/format (exception vector 55) only the
 *  following applies:
 *
 *  - If there is a move out with a packed number (opclass 3) the
 *  number is packed and written to user memory.  For the other
 *  opclasses the number(s) are written back to the fsave stack
 *  and the instruction is then restored back into the '040.  The
 *  '040 is then able to complete the instruction.
 *
 *  For example:
 *  fadd.x fpm,fpn where the fpm contains an unnormalized number.
 *  The '040 takes an unsupported data trap and gets to this
 *  routine.  The number is normalized, put back on the stack and
 *  then an frestore is done to restore the instruction back into
 *  the '040.  The '040 then re-executes the fadd.x fpm,fpn with
 *  a normalized number in the source and the instruction is
 *  successful.
 *
 *  Next consider if in the process of normalizing the un-
 *  normalized number it becomes a denormalized number.  The
 *  routine which converts the unnorm to a norm (called mk_norm)
 *  detects this and tags the number as a denorm.  The routine
 *  res_func sees the denorm tag and converts the denorm to a
 *  norm.  The instruction is then restored back into the '040
 *  which re_executes the instruction.
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

	.xref	nrm_zero
	.xref	decbin
	.xref	round

	.text
	.globl    get_op
	.globl    uns_getop
	.globl    uni_getop
get_op:
	clr.b	DY_MO_FLG(a6)
	tst.b	UFLG_TMP(a6)	/* test flag for unsupp/unimp state */
	beq.s	uni_getop

uns_getop:
	btst.b	#direction_bit,CMDREG1B(a6)
	bne	opclass3_go	/* branch if a fmove out (any kind) */
	btst.b	#6,CMDREG1B(a6)
	beq.s	uns_notpacked

	bfextu	CMDREG1B(a6){3:3},d0
	cmpi.b	#3,d0
	beq	pack_source	/* check for a packed src op, branch if so */
uns_notpacked:
	bsr	chk_dy_mo	/* set the dyadic/monadic flag */
	tst.b	DY_MO_FLG(a6)
	beq.s	src_op_ck	/* if monadic, go check src op */
				/* else, check dst op (fall through) */

	btst.b	#7,DTAG(a6)
	beq.s	src_op_ck	/* if dst op is norm, check src op */
	bra.s	dst_ex_dnrm	/* else, handle destination unnorm/dnrm */

uni_getop:
	bfextu	CMDREG1B(a6){0:6},d0 /* get opclass and src fields */
	cmpi.l	#0x17,d0		/* if op class and size fields are $17, */
				/* it is FMOVECR; if not, continue */
/*
 *  If the instruction is fmovecr, exit get_op.  It is handled
 *  in do_func and smovecr.sa.
 */
	beq.s	fmovecr_done

not_fmovecr_go:
	btst.b	#E1,E_BYTE(a6)	/* if set, there is a packed operand */
	bne	pack_source	/* check for packed src op, branch if so */

/*  The following lines of are coded to optimize on normalized operands */
	move.b	STAG(a6),d0
	or.b	DTAG(a6),d0	/* check if either of STAG/DTAG msb set */
	bmi.s	dest_op_ck	/* if so, some op needs to be fixed */
fmovecr_done:
	rts

dest_op_ck:
	btst.b	#7,DTAG(a6)	/* check for unsupported data types in */
	beq.s	src_op_ck	/* the destination, if not, check src op */
	bsr	chk_dy_mo	/* set dyadic/monadic flag */
	tst.b	DY_MO_FLG(a6)	/*  */
	beq.s	src_op_ck	/* if monadic, check src op */
/*
 *  At this point, destination has an extended denorm or unnorm.
 */
dst_ex_dnrm:
	move.w	FPTEMP_EX(a6),d0 /* get destination exponent */
	andi.w	#0x7fff,d0	/* mask sign, check if exp = 0000 */
	beq.s	src_op_ck	/* if denorm then check source op. */
/*
 * 				denorms are taken care of in res_func
 * 				(unsupp) or do_func (unimp)
 * 				else unnorm fall through
 */
	lea.l	FPTEMP(a6),a0	/* point a0 to dop - used in mk_norm */
	bsr	mk_norm /* go normalize */
/*
 * mk_norm returns:
 * 				L_SCR1{7:5} = operand tag
 * 					(000 = norm, 100 = denorm)
 * 				L_SCR1{4} = fpte15 or ete15
 * 					0 = exp >  $3fff
 * 					1 = exp <= $3fff
 * 				and puts the normalized num back
 * 				on the fsave stack
 */
	move.b L_SCR1(a6),DTAG(a6) /* write the new tag & fpte15 */
 				/* to the fsave stack and fall */
				/* through to check source operand */

src_op_ck:
	btst.b	#7,STAG(a6)
	beq	end_getop	/* check for unsupported data types on the */
				/* source operand */
	btst.b	#5,STAG(a6)
	bne.s	src_sd_dnrm	/* if bit 5 set, handle sgl/dbl denorms */
/*
 *  At this point only unnorms or extended denorms are possible.
 */
src_ex_dnrm:
	move.w	ETEMP_EX(a6),d0 /* get source exponent */
	andi.w	#0x7fff,d0	/* mask sign, check if exp = 0000 */
	beq	end_getop	/* if denorm then exit, denorms are handled in do_func */
	lea.l	ETEMP(a6),a0	/* point a0 to sop - used in mk_norm */
	bsr	mk_norm		/* go normalize */
/*
 * mk_norm returns:
 * 				L_SCR1{7:5} = operand tag
 * 					(000 = norm, 100 = denorm)
 * 				L_SCR1{4} = fpte15 or ete15
 * 					0 = exp >  $3fff
 * 					1 = exp <= $3fff
 * 				and puts the normalized num back
 * 				on the fsave stack
 */
	move.b	L_SCR1(a6),STAG(a6) /* write the new tag & ete15 */
	rts			/* end_getop */

/*
 *  At this point, only single or double denorms are possible.
 *  If the inst is not fmove, normalize the source.  If it is,
 *  do nothing to the input.
 */
src_sd_dnrm:
	btst.b	#4,CMDREG1B(a6)	/* differentiate between sgl/dbl denorm */
	bne.s	is_double
is_single:
	move.w	#0x3f81,d1	/* write bias for sgl denorm */
	bra.s	common		/* goto the common code */
is_double:
	move.w	#0x3c01,d1	/* write the bias for a dbl denorm */
common:
	btst.b	#sign_bit,ETEMP_EX(a6) /* grab sign bit of mantissa */
	beq.s	pos
	bset	#15,d1		/* set sign bit because it is negative */
pos:
	move.w	d1,ETEMP_EX(a6) /* put exponent on stack */

	move.w	CMDREG1B(a6),d1
	andi.w	#0xe3ff,d1	/* clear out source specifier */
	ori.w	#0x0800,d1	/* set source specifier to extended prec */
	move.w	d1,CMDREG1B(a6)	/* write back to the command word in stack */
				/* this is needed to fix unsupp data stack */
	lea.l	ETEMP(a6),a0	/* point a0 to sop */

	bsr	mk_norm		/* convert sgl/dbl denorm to norm */
	move.b	L_SCR1(a6),STAG(a6) /* put tag into source tag reg - d0 */
	rts			/* end_getop */
/*
 *  At this point, the source is definitely packed, whether
 *  instruction is dyadic or monadic is still unknown
 */
pack_source:
	move.l	FPTEMP_LO(a6),ETEMP(a6)	/* write ms part of packed */
				/* number to etemp slot */
	bsr.s	chk_dy_mo	/* set dyadic/monadic flag */
	bsr	unpack

	tst.b	DY_MO_FLG(a6)
	beq.s	end_getop	/* if monadic, exit; else, fix FPTEMP */
pack_dya:
	bfextu	CMDREG1B(a6){6:3},d0 /* extract dest fp reg */
	move.l	#7,d1
	sub.l	d0,d1
	clr.l	d0
	bset.l	d1,d0		/* set up d0 as a dynamic register mask */
	fmovem.x d0,FPTEMP(a6)	/* write to FPTEMP */

	btst.b	#7,DTAG(a6)	/* check dest tag for unnorm or denorm */
	bne	dst_ex_dnrm	/* else, handle the unnorm or ext denorm */
/*
 *  Dest is not denormalized.  Check for norm, and set fpte15
 *  accordingly.
 */
	move.b	DTAG(a6),d0
	andi.b	#0xf0,d0		/* strip to only dtag:fpte15 */
	tst.b	d0		/* check for normalized value */
	bne.s	end_getop	/* if inf/nan/zero leave get_op */
	move.w	FPTEMP_EX(a6),d0
	andi.w	#0x7fff,d0
	cmpi.w	#0x3fff,d0	/* check if fpte15 needs setting */
	bge.s	end_getop	/* if >= $3fff, leave fpte15=0 */
	or.b	#0x10,DTAG(a6)
	bra.s	end_getop

/*
 *  At this point, it is either an fmoveout packed, unnorm or denorm
 */
opclass3_go:
	clr.b	DY_MO_FLG(a6)	/* set dyadic/monadic flag to monadic */
	bfextu	CMDREG1B(a6){4:2},d0
	cmpi.b	#3,d0
	bne	src_ex_dnrm	/* if not equal, must be unnorm or denorm */
  				/* else it is a packed move out */
end_getop:
	rts

/*
 *  Sets the DY_MO_FLG correctly. This is used only on if it is an
 *  unsupported data type exception.  Set if dyadic.
 */
chk_dy_mo:
	move.w	CMDREG1B(a6),d0
	btst.l	#5,d0		/* testing extension command word */
	beq.s	set_mon		/* if bit 5 = 0 then monadic */
	btst.l	#4,d0		/* know that bit 5 = 1 */
	beq.s	set_dya		/* if bit 4 = 0 then dyadic */
	andi.w	#0x007f,d0	/* get rid of all but extension bits {6:0} */
	cmpi.w	#0x0038,d0	/* if extension = $38 then fcmp (dyadic) */
	bne.s	set_mon
set_dya:
	st	DY_MO_FLG(a6)	/* set the inst flag type to dyadic */
	rts
set_mon:
	clr.b	DY_MO_FLG(a6)	/* set the inst flag type to monadic */
	rts
/*
 * 	MK_NORM
 *
 *  Normalizes unnormalized numbers, sets tag to norm or denorm, sets unfl
 *  exception if denorm.
 *
 *  CASE opclass 0x0 unsupp
 * 	mk_norm till msb set
 * 	set tag = norm
 *
 *  CASE opclass 0x0 unimp
 * 	mk_norm till msb set or exp = 0
 * 	if integer bit = 0
 * 	   tag = denorm
 * 	else
 * 	   tag = norm
 *
 *  CASE opclass 011 unsupp
 * 	mk_norm till msb set or exp = 0
 * 	if integer bit = 0
 * 	   tag = denorm
 * 	   set unfl_nmcexe = 1
 * 	else
 * 	   tag = norm
 *
 *  if exp <= $3fff
 *    set ete15 or fpte15 = 1
 *  else set ete15 or fpte15 = 0
 *
 *  input:
 * 	a0 = points to operand to be normalized
 *  output:
 * 	L_SCR1{7:5} = operand tag (000 = norm, 100 = denorm)
 * 	L_SCR1{4}   = fpte15 or ete15 (0 = exp > $3fff, 1 = exp <=$3fff)
 * 	the normalized operand is placed back on the fsave stack
 */
mk_norm:
	clr.l	L_SCR1(a6)
	bclr.b	#sign_bit,LOCAL_EX(a0)
	sne	LOCAL_SGN(a0)	/* transform into internal extended format */

	cmpi.b	#0x2c,1+EXC_VEC(a6) /* check if unimp */
	bne.s	uns_data	/* branch if unsupp */
	bsr.s	uni_inst	/* call if unimp (opclass 0x0) */
	bra.s	reload
uns_data:
	btst.b	#direction_bit,CMDREG1B(a6) /* check transfer direction */
	bne.s	bit_set		/* branch if set (opclass 011) */
	bsr.s	uns_opx		/* call if opclass 0x0 */
	bra.s	reload
bit_set:
	bsr.s	uns_op3		/* opclass 011 */
reload:
	cmp.w	#0x3fff,LOCAL_EX(a0) /* if exp > $3fff */
	bgt.s	end_mk		/*    fpte15/ete15 already set to 0 */
	bset.b	#4,L_SCR1(a6)	/* else set fpte15/ete15 to 1 */
/*
 * 				calling routine actually sets the
 * 				value on the stack (along with the
 * 				tag), since this routine doesn't
 * 				know if it should set ete15 or fpte15
 * 				ie, it doesn't know if this is the
 * 				src op or dest op.
 */
end_mk:
	bfclr	LOCAL_SGN(a0){0:8}
	beq.s	end_mk_pos
	bset.b	#sign_bit,LOCAL_EX(a0) /* convert back to IEEE format */
end_mk_pos:
	rts
/*
 *      CASE opclass 011 unsupp
 */
uns_op3:
	bsr	nrm_zero	/* normalize till msb = 1 or exp = zero */
	btst.b	#7,LOCAL_HI(a0)	/* if msb = 1 */
	bne.s	no_unfl		/* then branch */
set_unfl:
	or.w	#dnrm_tag,L_SCR1(a6) /* set denorm tag */
	bset.b	#unfl_bit,FPSR_EXCEPT(a6) /* set unfl exception bit */
no_unfl:
	rts
/*
 *      CASE opclass 0x0 unsupp
 */
uns_opx:
	bsr	nrm_zero	/* normalize the number */
	btst.b	#7,LOCAL_HI(a0)	/* check if integer bit (j-bit) is set */
	beq.s	uns_den		/* if clear then now have a denorm */
uns_nrm:
	or.b	#norm_tag,L_SCR1(a6) /* set tag to norm */
	rts
uns_den:
	or.b	#dnrm_tag,L_SCR1(a6) /* set tag to denorm */
	rts
/*
 *      CASE opclass 0x0 unimp
 */
uni_inst:
	bsr	nrm_zero
	btst.b	#7,LOCAL_HI(a0)	/* check if integer bit (j-bit) is set */
	beq.s	uni_den		/* if clear then now have a denorm */
uni_nrm:
	or.b	#norm_tag,L_SCR1(a6) /* set tag to norm */
	rts
uni_den:
	or.b	#dnrm_tag,L_SCR1(a6) /* set tag to denorm */
	rts

/*
 * 	Decimal to binary conversion
 *
 *  Special cases of inf and NaNs are completed outside of decbin.
 *  If the input is an snan, the snan bit is not set.
 *
 *  input:
 * 	ETEMP(a6)	- points to packed decimal string in memory
 *  output:
 * 	fp0	- contains packed string converted to extended precision
 * 	ETEMP	- same as fp0
 */
unpack:
	move.w	CMDREG1B(a6),d0	/* examine command word, looking for fmove's */
	andi.w	#0x3b,d0
	beq	move_unpack	/* special handling for fmove: must set FPSR_CC */

	move.w	ETEMP(a6),d0	/* get word with inf information */
	bfextu	d0{20:12},d1	/* get exponent into d1 */
	cmpi.w	#0x0fff,d1	/* test for inf or NaN */
	bne.s	try_zero	/* if not equal, it is not special */
	bfextu	d0{17:3},d1	/* get SE and y bits into d1 */
	cmpi.w	#7,d1		/* SE and y bits must be on for special */
	bne.s	try_zero	/* if not on, it is not special */
/* input is of the special cases of inf and NaN */
	tst.l	ETEMP_HI(a6)	/* check ms mantissa */
	bne.s	fix_nan		/* if non-zero, it is a NaN */
	tst.l	ETEMP_LO(a6)	/* check ls mantissa */
	bne.s	fix_nan		/* if non-zero, it is a NaN */
	bra	finish_go		/* special already on stack */
fix_nan:
	btst.b	#signan_bit,ETEMP_HI(a6) /* test for snan */
	bne	finish_go
	or.l	#snaniop_mask,USER_FPSR(a6) /* always set snan if it is so */
	bra	finish_go
try_zero:
	move.w	ETEMP_EX+2(a6),d0 /* get word 4 */
	andi.w	#0x000f,d0	/* clear all but last ni(y)bble */
	tst.w	d0		/* check for zero. */
	bne.s	not_spec
	tst.l	ETEMP_HI(a6)	/* check words 3 and 2 */
	bne.s	not_spec
	tst.l	ETEMP_LO(a6)	/* check words 1 and 0 */
	bne.s	not_spec
	tst.l	ETEMP(a6)	/* test sign of the zero */
	bge.s	pos_zero_go
	move.l	#0x80000000,ETEMP(a6) /* write neg zero to etemp */
	clr.l	ETEMP_HI(a6)
	clr.l	ETEMP_LO(a6)
	bra	finish_go
pos_zero_go:
	clr.l	ETEMP(a6)
	clr.l	ETEMP_HI(a6)
	clr.l	ETEMP_LO(a6)
	bra	finish_go

not_spec:
	fmovem.x fp0-fp1,-(a7)	/* save fp0 - decbin returns in it */
	bsr	decbin
	fmove.x fp0,ETEMP(a6)	/* put the unpacked sop in the fsave stack */
	fmovem.x (a7)+,fp0-fp1
	fmove.l	#0,fpsr		/* clr fpsr from decbin */
	bra	finish_go

/*
 *  Special handling for packed move in:  Same results as all other
 *  packed cases, but we must set the FPSR condition codes properly.
 */
move_unpack:
	move.w	ETEMP(a6),d0	/* get word with inf information */
	bfextu	d0{20:12},d1	/* get exponent into d1 */
	cmpi.w	#0x0fff,d1	/* test for inf or NaN */
	bne.s	mtry_zero	/* if not equal, it is not special */
	bfextu	d0{17:3},d1	/* get SE and y bits into d1 */
	cmpi.w	#7,d1		/* SE and y bits must be on for special */
	bne.s	mtry_zero	/* if not on, it is not special */
/* input is of the special cases of inf and NaN */
	tst.l	ETEMP_HI(a6)	/* check ms mantissa */
	bne.s	mfix_nan		/* if non-zero, it is a NaN */
	tst.l	ETEMP_LO(a6)	/* check ls mantissa */
	bne.s	mfix_nan		/* if non-zero, it is a NaN */
/* input is inf */
	or.l	#inf_mask,USER_FPSR(a6) /* set I bit */
	tst.l	ETEMP(a6)	/* check sign */
	bge	finish_go
	or.l	#neg_mask,USER_FPSR(a6) /* set N bit */
	bra	finish_go		/* special already on stack */
mfix_nan:
	or.l	#nan_mask,USER_FPSR(a6) /* set NaN bit */
	move.b	#nan_tag,STAG(a6)	/* set stag to NaN */
	btst.b	#signan_bit,ETEMP_HI(a6) /* test for snan */
	bne.s	mn_snan
	or.l	#snaniop_mask,USER_FPSR(a6) /* set snan bit */
	btst.b	#snan_bit,FPCR_ENABLE(a6) /* test for snan enabled */
	bne.s	mn_snan
	bset.b	#signan_bit,ETEMP_HI(a6) /* force snans to qnans */
mn_snan:
	tst.l	ETEMP(a6)	/* check for sign */
	bge.s	finish_go		/* if clr, go on */
	or.l	#neg_mask,USER_FPSR(a6) /* set N bit */
	bra.s	finish_go

mtry_zero:
	move.w	ETEMP_EX+2(a6),d0 /* get word 4 */
	andi.w	#0x000f,d0	/* clear all but last ni(y)bble */
	tst.w	d0		/* check for zero. */
	bne.s	mnot_spec
	tst.l	ETEMP_HI(a6)	/* check words 3 and 2 */
	bne.s	mnot_spec
	tst.l	ETEMP_LO(a6)	/* check words 1 and 0 */
	bne.s	mnot_spec
	tst.l	ETEMP(a6)	/* test sign of the zero */
	bge.s	mpos_zero_go
	or.l	#neg_mask+z_mask,USER_FPSR(a6) /* set N and Z */
	move.l	#0x80000000,ETEMP(a6) /* write neg zero to etemp */
	clr.l	ETEMP_HI(a6)
	clr.l	ETEMP_LO(a6)
	bra.s	finish_go
mpos_zero_go:
	or.l	#z_mask,USER_FPSR(a6) /* set Z */
	clr.l	ETEMP(a6)
	clr.l	ETEMP_HI(a6)
	clr.l	ETEMP_LO(a6)
	bra.s	finish_go

mnot_spec:
	fmovem.x fp0-fp1,-(a7)	/* save fp0 ,fp1 - decbin returns in fp0 */
	bsr	decbin
	fmove.x fp0,ETEMP(a6)	/* put the unpacked sop in the fsave stack */
	fmovem.x (a7)+,fp0-fp1

finish_go:
	move.w	CMDREG1B(a6),d0	/* get the command word */
	andi.w	#0xfbff,d0	/* change the source specifier field to */
				/* extended (was packed). */
	move.w	d0,CMDREG1B(a6)	/* write command word back to fsave stack */
				/* we need to do this so the 040 will */
				/* re-execute the inst. without taking */
				/* another packed trap. */

fix_stag:
/*
 * Converted result is now in etemp on fsave stack, now set the source
 * tag (stag)
 * 	if (ete =$7fff) then INF or NAN
 * 		if (etemp = $x.0----0) then
 * 			stag = INF
 * 		else
 * 			stag = NAN
 * 	else
 * 		if (ete = $0000) then
 * 			stag = ZERO
 * 		else
 * 			stag = NORM
 *
 *  Note also that the etemp_15 bit (just right of the stag) must
 *  be set accordingly.
 */
	move.w		ETEMP_EX(a6),d1
	andi.w		#0x7fff,d1   /* strip sign */
	cmpi.w		#0x7fff,d1
	bne.s		z_or_nrm
	move.l		ETEMP_HI(a6),d1
	bne.s		is_nan
	move.l		ETEMP_LO(a6),d1
	bne.s		is_nan
is_inf:
	move.b		#0x40,STAG(a6)
	move.l		#0x40,d0
	rts
is_nan:
	move.b		#0x60,STAG(a6)
	move.l		#0x60,d0
	rts
z_or_nrm:
	tst.w		d1
	bne.s		is_nrm
is_zro:
/*  For a zero, set etemp_15 */
	move.b		#0x30,STAG(a6)
	move.l		#0x20,d0
	rts
is_nrm:
/*  For a norm, check if the exp <= $3fff; if so, set etemp_15 */
	cmpi.w		#0x3fff,d1
	ble.s		set_bit15
	move.b		#0,STAG(a6)
	bra.s		end_is_nrm
set_bit15:
	move.b		#0x10,STAG(a6)
end_is_nrm:
	move.l		#0,d0
end_fix_go:
end_get:
	rts
