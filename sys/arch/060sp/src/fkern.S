/*
 *  XDEF ****************************************************************	
 * 	t_dz(): Handle DZ exception during transcendental emulation.	
 * 	        Sets N bit according to sign of source operand.		
 * 	t_dz2(): Handle DZ exception during transcendental emulation.	
 * 		 Sets N bit always.					
 * 									
 *  XREF ****************************************************************	
 * 	None								
 * 									
 *  INPUT ***************************************************************	
 * 	a0 = pointer to source operand					
 * 									
 *  OUTPUT **************************************************************	
 * 	fp0 = default result						
 * 									
 *  ALGORITHM ***********************************************************	
 * 	- Store properly signed INF into fp0.				
 * 	- Set FPSR exception status dz bit, ccode inf bit, and		
 * 	  accrued dz bit.						
 * 									
 */

	.include "hdr.fpu"

	.text

qnan:	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff

	.globl		t_dz
t_dz:
	tst.b		SRC_EX.w(a0)		/*  no; is src negative? */
	bmi.b		t_dz2			/*  yes */

dz_pinf:
	fmove.s		#0x7f800000,fp0	/*  return +INF in fp0 */
	ori.l		#dzinf_mask,USER_FPSR(a6) /*  set I/DZ/ADZ */
	rts

	.globl		t_dz2
t_dz2:
	fmove.s		#0xff800000,fp0	/*  return -INF in fp0 */
	ori.l		#dzinf_mask+neg_mask,USER_FPSR(a6) /*  set N/I/DZ/ADZ */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  OPERR exception:						 */
/* 	- set FPSR exception status operr bit, condition code	 */
/* 	  nan bit; Store default NAN into fp0			 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		t_operr
t_operr:
	ori.l		#opnan_mask,USER_FPSR(a6) /*  set NaN/OPERR/AIOP */
	/* fmovem.x		qnan(pc),fp0 */		/*  return default NAN in fp0 */
	.dc.w 0xf23a,0xd080,0xffbe /* rejected by gas */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  Extended DENORM:						 */
/* 	- For all functions that have a denormalized input and	 */
/* 	  that f(x)=x, this is the entry point.			 */
/* 	- we only return the EXOP here if either underflow or	 */
/* 	  inexact is enabled.					 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*  Entry point for scale w/ extended denorm. The function does */
/*  NOT set INEX2/AUNFL/AINEX. */
	.globl		t_resdnrm
t_resdnrm:
	ori.l		#unfl_mask,USER_FPSR(a6) /*  set UNFL */
	bra.b		xdnrm_con

	.globl		t_extdnrm
t_extdnrm:
	ori.l		#unfinx_mask,USER_FPSR(a6) /*  set UNFL/INEX2/AUNFL/AINEX */

xdnrm_con:
	move.l		a0,a1			/*  make copy of src ptr */
	move.l		d0,d1			/*  make copy of rnd prec,mode */
	andi.b		#0xc0,d1		/*  extended precision? */
	bne.b		xdnrm_sd		/*  no */

/*  result precision is extended. */
	tst.b		LOCAL_EX.w(a0)		/*  is denorm negative? */
	bpl.b		xdnrm_exit		/*  no */

	bset		#neg_bit,FPSR_CC(a6)	/*  yes; set 'N' ccode bit */
	bra.b		xdnrm_exit

/*  result precision is single or double */
xdnrm_sd:
	move.l		a1,-(sp)
	tst.b		LOCAL_EX.w(a0)		/*  is denorm pos or neg? */
	smi.b		d1			/*  set d0 accordingly */
	bsr.l		unf_sub
	move.l		(sp)+,a1
xdnrm_exit:
	fmovem.x	(a0),fp0		/*  return default result in fp0 */

	move.b		FPCR_ENABLE(a6),d0
	andi.b		#0x0a,d0		/*  is UNFL or INEX enabled? */
	bne.b		xdnrm_ena		/*  yes */
	rts

/* ;;;;;;;;;;;;;; */
/*  unfl enabled  */
/* ;;;;;;;;;;;;;; */
/*  we have a DENORM that needs to be converted into an EXOP. */
/*  so, normalize the mantissa, add 0x6000 to the new exponent, */
/*  and return the result in fp1. */
xdnrm_ena:
	move.w		LOCAL_EX.w(a1),FP_SCR0_EX(a6)
	move.l		LOCAL_HI(a1),FP_SCR0_HI(a6)
	move.l		LOCAL_LO(a1),FP_SCR0_LO(a6)

	lea		FP_SCR0(a6),a0
	bsr.l		norm			/*  normalize mantissa */
	addi.l		#0x6000,d0		/*  add extra bias */
	andi.w		#0x8000,FP_SCR0_EX(a6)	/*  keep old sign */
	or.w		d0,FP_SCR0_EX(a6)	/*  insert new exponent */

	fmovem.x	FP_SCR0(a6),fp1	/*  return EXOP in fp1 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  UNFL exception:						 */
/* 	- This routine is for cases where even an EXOP isn't	 */
/* 	  large enough to hold the range of this result.	 */
/* 	  In such a case, the EXOP equals zero.			 */
/* 	- Return the default result to the proper precision	 */
/* 	  with the sign of this result being the same as that	 */
/* 	  of the src operand.					 */
/* 	- t_unfl2() is provided to force the result sign to	 */
/* 	  positive which is the desired result for fetox().	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		t_unfl
t_unfl:
	ori.l		#unfinx_mask,USER_FPSR(a6) /*  set UNFL/INEX2/AUNFL/AINEX */

	tst.b		(a0)			/*  is result pos or neg? */
	smi.b		d1			/*  set d1 accordingly */
	bsr.l		unf_sub			/*  calc default unfl result */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */

	fmove.s		#0x00000000,fp1	/*  return EXOP in fp1 */
	rts

/*  t_unfl2 ALWAYS tells unf_sub to create a positive result */
	.globl		t_unfl2
t_unfl2:
	ori.l		#unfinx_mask,USER_FPSR(a6) /*  set UNFL/INEX2/AUNFL/AINEX */

	sf.b		d1			/*  set d0 to represent positive */
	bsr.l		unf_sub			/*  calc default unfl result */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */

	fmove.s		#0x0000000,fp1		/*  return EXOP in fp1 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  OVFL exception:						 */
/* 	- This routine is for cases where even an EXOP isn't	 */
/* 	  large enough to hold the range of this result.	 */
/* 	- Return the default result to the proper precision	 */
/* 	  with the sign of this result being the same as that	 */
/* 	  of the src operand.					 */
/* 	- t_ovfl2() is provided to force the result sign to	 */
/* 	  positive which is the desired result for fcosh().	 */
/* 	- t_ovfl_sc() is provided for scale() which only sets	 */
/* 	  the inexact bits if the number is inexact for the	 */
/* 	  precision indicated.					 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

	.globl		t_ovfl_sc
t_ovfl_sc:
	ori.l		#ovfl_inx_mask,USER_FPSR(a6) /*  set OVFL/AOVFL/AINEX */

	move.b		d0,d1			/*  fetch rnd mode/prec */
	andi.b		#0xc0,d1		/*  extract rnd prec */
	beq.b		ovfl_work		/*  prec is extended */

	tst.b		LOCAL_HI(a0)		/*  is dst a DENORM? */
	bmi.b		ovfl_sc_norm		/*  no */

/*  dst op is a DENORM. we have to normalize the mantissa to see if the */
/*  result would be inexact for the given precision. make a copy of the */
/*  dst so we don't screw up the version passed to us. */
	move.w		LOCAL_EX.w(a0),FP_SCR0_EX(a6)
	move.l		LOCAL_HI(a0),FP_SCR0_HI(a6)
	move.l		LOCAL_LO(a0),FP_SCR0_LO(a6)
	lea		FP_SCR0(a6),a0	/*  pass ptr to FP_SCR0 */
	movem.l		d0-d1/a0,-(sp)		/*  save d0-d1/a0 */
	bsr.l		norm			/*  normalize mantissa */
	movem.l		(sp)+,d0-d1/a0		/*  restore d0-d1/a0 */

ovfl_sc_norm:
	cmpi.b		#0x40,d1		/*  is prec dbl? */
	bne.b		ovfl_sc_dbl		/*  no; sgl */
ovfl_sc_sgl:
	tst.l		LOCAL_LO(a0)		/*  is lo lw of sgl set? */
	bne.b		ovfl_sc_inx		/*  yes */
	tst.b		3+LOCAL_HI(a0)		/*  is lo byte of hi lw set? */
	bne.b		ovfl_sc_inx		/*  yes */
	bra.b		ovfl_work		/*  don't set INEX2 */
ovfl_sc_dbl:
	move.l		LOCAL_LO(a0),d1	/*  are any of lo 11 bits of */
	andi.l		#0x7ff,d1		/*  dbl mantissa set? */
	beq.b		ovfl_work		/*  no; don't set INEX2 */
ovfl_sc_inx:
	ori.l		#inex2_mask,USER_FPSR(a6) /*  set INEX2 */
	bra.b		ovfl_work		/*  continue */

	.globl		t_ovfl
t_ovfl:
	ori.l		#ovfinx_mask,USER_FPSR(a6) /*  set OVFL/INEX2/AOVFL/AINEX */

ovfl_work:
	tst.b		LOCAL_EX.w(a0)		/*  what is the sign? */
	smi.b		d1			/*  set d1 accordingly */
	bsr.l		ovf_res			/*  calc default ovfl result */
	move.b		d0,FPSR_CC(a6)	/*  insert new ccodes */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */

	fmove.s		#0x00000000,fp1	/*  return EXOP in fp1 */
	rts

/*  t_ovfl2 ALWAYS tells ovf_res to create a positive result */
	.globl		t_ovfl2
t_ovfl2:
	ori.l		#ovfinx_mask,USER_FPSR(a6) /*  set OVFL/INEX2/AOVFL/AINEX */

	sf.b		d1			/*  clear sign flag for positive */
	bsr.l		ovf_res			/*  calc default ovfl result */
	move.b		d0,FPSR_CC(a6)	/*  insert new ccodes */
	fmovem.x	(a0),fp0		/*  return default result in fp0 */

	fmove.s		#0x00000000,fp1	/*  return EXOP in fp1 */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  t_catch():							 */
/* 	- the last operation of a transcendental emulation	 */
/* 	  routine may have caused an underflow or overflow.	 */
/* 	  we find out if this occurred by doing an fsave and	 */
/* 	  checking the exception bit. if one did occur, then we	 */
/* 	  jump to fgen_except() which creates the default	 */
/* 	  result and EXOP for us.				 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		t_catch
t_catch:

	fsave		-(sp)
	tst.b		0x2(sp)
	bmi.b		catch
	add.l		#0xc,sp

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  INEX2 exception:						 */
/* 	- The inex2 and ainex bits are set.			 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		t_inx2
t_inx2:
	fblt		t_minx2
	fbeq		inx2_zero

	.globl		t_pinx2
t_pinx2:
	ori.w		#inx2a_mask,2+USER_FPSR(a6) /*  set INEX2/AINEX */
	rts

	.globl		t_minx2
t_minx2:
	ori.l		#inx2a_mask+neg_mask,USER_FPSR(a6) /*  set N/INEX2/AINEX */
	rts

inx2_zero:
	move.b		#z_bmask,FPSR_CC(a6)
	ori.w		#inx2a_mask,2+USER_FPSR(a6) /*  set INEX2/AINEX */
	rts

/*  an underflow or overflow exception occurred. */
/*  we must set INEX/AINEX since the fmul/fdiv/fmov emulation may not! */
catch:
	ori.w		#inx2a_mask,FPSR_EXCEPT(a6)
catch2:
	bsr.l		fgen_except
	add.l		#0xc,sp
	rts

	.globl		t_catch2
t_catch2:

	fsave		-(sp)

	tst.b		0x2(sp)
	bmi.b		catch2
	add.l		#0xc,sp

	fmove.l		fpsr,d0
	or.l		d0,USER_FPSR(a6)

	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  unf_res(): underflow default result calculation for transcendentals	 */
/* 									 */
/*  INPUT:								 */
/* 	d0   : rnd mode,precision					 */
/* 	d1.b : sign bit of result ('11111111 = (-) ; '00000000 = (+))	 */
/*  OUTPUT:								 */
/* 	a0   : points to result (in instruction memory)			 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
unf_sub:
	ori.l		#unfinx_mask,USER_FPSR(a6)

	andi.w		#0x10,d1		/*  keep sign bit in 4th spot */

	lsr.b		#0x4,d0		/*  shift rnd prec,mode to lo bits */
	andi.b		#0xf,d0		/*  strip hi rnd mode bit */
	or.b		d1,d0			/*  concat {sgn,mode,prec} */

	move.l		d0,d1			/*  make a copy */
	lsl.b		#0x1,d1		/*  mult index 2 by 2 */

	move.b		(tbl_unf_cc.b,pc,d0.w*1),FPSR_CC(a6) /*  insert ccode bits */
	lea		(tbl_unf_result.b,pc,d1.w*8),a0 /*  grab result ptr */
	rts

tbl_unf_cc:
	.dc.b		0x4, 0x4, 0x4, 0x0
	.dc.b		0x4, 0x4, 0x4, 0x0
	.dc.b		0x4, 0x4, 0x4, 0x0
	.dc.b		0x0, 0x0, 0x0, 0x0
	.dc.b		0x8+0x4, 0x8+0x4, 0x8, 0x8+0x4
	.dc.b		0x8+0x4, 0x8+0x4, 0x8, 0x8+0x4
	.dc.b		0x8+0x4, 0x8+0x4, 0x8, 0x8+0x4

tbl_unf_result:
	.dc.l		0x00000000, 0x00000000, 0x00000000, 0x0 /*  ZERO;ext */
	.dc.l		0x00000000, 0x00000000, 0x00000000, 0x0 /*  ZERO;ext */
	.dc.l		0x00000000, 0x00000000, 0x00000000, 0x0 /*  ZERO;ext */
	.dc.l		0x00000000, 0x00000000, 0x00000001, 0x0 /*  MIN; ext */

	.dc.l		0x3f810000, 0x00000000, 0x00000000, 0x0 /*  ZERO;sgl */
	.dc.l		0x3f810000, 0x00000000, 0x00000000, 0x0 /*  ZERO;sgl */
	.dc.l		0x3f810000, 0x00000000, 0x00000000, 0x0 /*  ZERO;sgl */
	.dc.l		0x3f810000, 0x00000100, 0x00000000, 0x0 /*  MIN; sgl */

	.dc.l		0x3c010000, 0x00000000, 0x00000000, 0x0 /*  ZERO;dbl */
	.dc.l		0x3c010000, 0x00000000, 0x00000000, 0x0 /*  ZER0;dbl */
	.dc.l		0x3c010000, 0x00000000, 0x00000000, 0x0 /*  ZERO;dbl */
	.dc.l		0x3c010000, 0x00000000, 0x00000800, 0x0 /*  MIN; dbl */

	.dc.l		0x0,0x0,0x0,0x0
	.dc.l		0x0,0x0,0x0,0x0
	.dc.l		0x0,0x0,0x0,0x0
	.dc.l		0x0,0x0,0x0,0x0

	.dc.l		0x80000000, 0x00000000, 0x00000000, 0x0 /*  ZERO;ext */
	.dc.l		0x80000000, 0x00000000, 0x00000000, 0x0 /*  ZERO;ext */
	.dc.l		0x80000000, 0x00000000, 0x00000001, 0x0 /*  MIN; ext */
	.dc.l		0x80000000, 0x00000000, 0x00000000, 0x0 /*  ZERO;ext */

	.dc.l		0xbf810000, 0x00000000, 0x00000000, 0x0 /*  ZERO;sgl */
	.dc.l		0xbf810000, 0x00000000, 0x00000000, 0x0 /*  ZERO;sgl */
	.dc.l		0xbf810000, 0x00000100, 0x00000000, 0x0 /*  MIN; sgl */
	.dc.l		0xbf810000, 0x00000000, 0x00000000, 0x0 /*  ZERO;sgl */

	.dc.l		0xbc010000, 0x00000000, 0x00000000, 0x0 /*  ZERO;dbl */
	.dc.l		0xbc010000, 0x00000000, 0x00000000, 0x0 /*  ZERO;dbl */
	.dc.l		0xbc010000, 0x00000000, 0x00000800, 0x0 /*  MIN; dbl */
	.dc.l		0xbc010000, 0x00000000, 0x00000000, 0x0 /*  ZERO;dbl */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  src_zero(): Return signed zero according to sign of src operand.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		src_zero
src_zero:
	tst.b		SRC_EX.w(a0)		/*  get sign of src operand */
	bmi.b		ld_mzero		/*  if neg, load neg zero */

/*
 * ld_pzero(): return a positive zero.
 */
	.globl		ld_pzero
ld_pzero:
	fmove.s		#0x00000000,fp0	/*  load +0 */
	move.b		#z_bmask,FPSR_CC(a6)	/*  set 'Z' ccode bit */
	rts

/*  ld_mzero(): return a negative zero. */
	.globl		ld_mzero
ld_mzero:
	fmove.s		#0x80000000,fp0	/*  load -0 */
	move.b		#neg_bmask+z_bmask,FPSR_CC(a6) /*  set 'N','Z' ccode bits */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  dst_zero(): Return signed zero according to sign of dst operand.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		dst_zero
dst_zero:
	tst.b		DST_EX.w(a1)		/*  get sign of dst operand */
	bmi.b		ld_mzero		/*  if neg, load neg zero */
	bra.b		ld_pzero		/*  load positive zero */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  src_inf(): Return signed inf according to sign of src operand.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		src_inf
src_inf:
	tst.b		SRC_EX.w(a0)		/*  get sign of src operand */
	bmi.b		ld_minf			/*  if negative branch */

/*
 * ld_pinf(): return a positive infinity.
 */
	.globl		ld_pinf
ld_pinf:
	fmove.s		#0x7f800000,fp0	/*  load +INF */
	move.b		#inf_bmask,FPSR_CC(a6)	/*  set 'INF' ccode bit */
	rts

/*
 * ld_minf():return a negative infinity.
 */
	.globl		ld_minf
ld_minf:
	fmove.s		#0xff800000,fp0	/*  load -INF */
	move.b		#neg_bmask+inf_bmask,FPSR_CC(a6) /*  set 'N','I' ccode bits */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  dst_inf(): Return signed inf according to sign of dst operand.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		dst_inf
dst_inf:
	tst.b		DST_EX.w(a1)		/*  get sign of dst operand */
	bmi.b		ld_minf			/*  if negative branch */
	bra.b		ld_pinf

	.globl		szr_inf
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  szr_inf(): Return +ZERO for a negative src operand or		 */
/* 	            +INF for a positive src operand.		 */
/* 	     Routine used for fetox, ftwotox, and ftentox.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
szr_inf:
	tst.b		SRC_EX.w(a0)		/*  check sign of source */
	bmi.b		ld_pzero
	bra.b		ld_pinf

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  sopr_inf(): Return +INF for a positive src operand or			 */
/* 	      jump to operand error routine for a negative src operand.	 */
/* 	      Routine used for flogn, flognp1, flog10, and flog2.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		sopr_inf
sopr_inf:
	tst.b		SRC_EX.w(a0)		/*  check sign of source */
	bmi.w		t_operr
	bra.b		ld_pinf

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  setoxm1i(): Return minus one for a negative src operand or	 */
/* 	      positive infinity for a positive src operand.	 */
/* 	      Routine used for fetoxm1.				 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		setoxm1i
setoxm1i:
	tst.b		SRC_EX.w(a0)		/*  check sign of source */
	bmi.b		ld_mone
	bra.b		ld_pinf

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  src_one(): Return signed one according to sign of src operand.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		src_one
src_one:
	tst.b		SRC_EX.w(a0)		/*  check sign of source */
	bmi.b		ld_mone

/*
 * ld_pone(): return positive one.
 */
	.globl		ld_pone
ld_pone:
	fmove.s		#0x3f800000,fp0	/*  load +1 */
	clr.b		FPSR_CC(a6)
	rts

/*
 * ld_mone(): return negative one.
 */
	.globl		ld_mone
ld_mone:
	fmove.s		#0xbf800000,fp0	/*  load -1 */
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'N' ccode bit */
	rts

ppiby2:	.dc.l		0x3fff0000, 0xc90fdaa2, 0x2168c235
mpiby2:	.dc.l		0xbfff0000, 0xc90fdaa2, 0x2168c235

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  spi_2(): Return signed PI/2 according to sign of src operand.	 */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		spi_2
spi_2:
	tst.b		SRC_EX.w(a0)		/*  check sign of source */
	bmi.b		ld_mpi2

/*
 * ld_ppi2(): return positive PI/2.
 */
	.globl		ld_ppi2
ld_ppi2:
	fmove.l		d0,fpcr
	fmove.x		ppiby2(pc),fp0	/*  load +pi/2 */
	bra.w		t_pinx2			/*  set INEX2 */

/*
 * ld_mpi2(): return negative PI/2.
 */
	.globl		ld_mpi2
ld_mpi2:
	fmove.l		d0,fpcr
	fmove.x		mpiby2(pc),fp0	/*  load -pi/2 */
	bra.w		t_minx2			/*  set INEX2 */

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/*  The following routines give support for fsincos.  */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * ssincosz(): When the src operand is ZERO, store a one in the
 *	      cosine register and return a ZERO in fp0 w/ the same sign
 *	      as the src operand.
 */
	.globl		ssincosz
ssincosz:
	fmove.s		#0x3f800000,fp1
	tst.b		SRC_EX.w(a0)		/*  test sign */
	bpl.b		sincoszp
	fmove.s		#0x80000000,fp0	/*  return sin result in fp0 */
	move.b		#z_bmask+neg_bmask,FPSR_CC(a6)
	bra.b		sto_cos			/*  store cosine result */
sincoszp:
	fmove.s		#0x00000000,fp0	/*  return sin result in fp0 */
	move.b		#z_bmask,FPSR_CC(a6)
	bra.b		sto_cos			/*  store cosine result */

/*
 * ssincosi(): When the src operand is INF, store a QNAN in the cosine
 *	      register and jump to the operand error routine for negative
 *	      src operands.
 */
	.globl		ssincosi
ssincosi:
	fmove.x		qnan(pc),fp1		/*  load NAN */
	bsr.l		sto_cos			/*  store cosine result */
	bra.w		t_operr

/*
 * ssincosqnan(): When the src operand is a QNAN, store the QNAN in the cosine
 *		 register and branch to the src QNAN routine.
 */
	.globl		ssincosqnan
ssincosqnan:
	fmove.x		LOCAL_EX.w(a0),fp1
	bsr.l		sto_cos
	bra.w		src_qnan

/*
 * ssincossnan(): When the src operand is an SNAN, store the SNAN w/ the SNAN bit set
 *		 in the cosine register and branch to the src SNAN routine.
 */
	.globl		ssincossnan
ssincossnan:
	fmove.x		LOCAL_EX.w(a0),fp1
	bsr.l		sto_cos
	bra.w		src_snan

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 * sto_cos(): store fp1 to the fpreg designated by the CMDREG dst field.
 *	     fp1 holds the result of the cosine portion of ssincos().
 *	     the value in fp1 will not take any exceptions when moved.
 * INPUT:
 *	fp1 : fp value to store
 * MODIFIED:
 *	d0
 *;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 */
	.globl		sto_cos
sto_cos:
	move.b		1+EXC_CMDREG(a6),d0
	andi.w		#0x7,d0
	move.w		(tbl_sto_cos.b,pc,d0.w*2),d0
	jmp		(tbl_sto_cos.b,pc,d0.w*1)

tbl_sto_cos:
	.dc.w		sto_cos_0 - tbl_sto_cos
	.dc.w		sto_cos_1 - tbl_sto_cos
	.dc.w		sto_cos_2 - tbl_sto_cos
	.dc.w		sto_cos_3 - tbl_sto_cos
	.dc.w		sto_cos_4 - tbl_sto_cos
	.dc.w		sto_cos_5 - tbl_sto_cos
	.dc.w		sto_cos_6 - tbl_sto_cos
	.dc.w		sto_cos_7 - tbl_sto_cos

sto_cos_0:
	fmovem.x	fp1,EXC_FP0(a6)
	rts
sto_cos_1:
	fmovem.x	fp1,EXC_FP1(a6)
	rts
sto_cos_2:
	fmove.x		fp1,fp2
	rts
sto_cos_3:
	fmove.x		fp1,fp3
	rts
sto_cos_4:
	fmove.x		fp1,fp4
	rts
sto_cos_5:
	fmove.x		fp1,fp5
	rts
sto_cos_6:
	fmove.x		fp1,fp6
	rts
sto_cos_7:
	fmove.x		fp1,fp7
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		smod_sdnrm
	.globl		smod_snorm
smod_sdnrm:
smod_snorm:
	move.b		DTAG(a6),d1
	beq.l		smod
	cmpi.b		#ZERO,d1
	beq.w		smod_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		smod
	cmpi.b		#SNAN,d1
	beq.l		dst_snan
	bra.l		dst_qnan

	.globl		smod_szero
smod_szero:
	move.b		DTAG(a6),d1
	beq.l		t_operr
	cmpi.b		#ZERO,d1
	beq.l		t_operr
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		t_operr
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		dst_snan

	.globl		smod_sinf
smod_sinf:
	move.b		DTAG(a6),d1
	beq.l		smod_fpn
	cmpi.b		#ZERO,d1
	beq.l		smod_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		smod_fpn
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		dst_snan

smod_zro:
srem_zro:
	move.b		SRC_EX.w(a0),d1		/*  get src sign */
	move.b		DST_EX.w(a1),d0		/*  get dst sign */
	eor.b		d0,d1			/*  get qbyte sign */
	andi.b		#0x80,d1
	move.b		d1,FPSR_QBYTE(a6)
	tst.b		d0
	bpl.w		ld_pzero
	bra.w		ld_mzero

smod_fpn:
srem_fpn:
	clr.b		FPSR_QBYTE(a6)
	move.l		d0,-(sp)
	move.b		SRC_EX.w(a0),d1		/*  get src sign */
	move.b		DST_EX.w(a1),d0		/*  get dst sign */
	eor.b		d0,d1			/*  get qbyte sign */
	andi.b		#0x80,d1
	move.b		d1,FPSR_QBYTE(a6)
	cmpi.b		#DENORM,DTAG(a6)
	bne.b		smod_nrm
	lea		DST.w(a1),a0
	move.l		(sp)+,d0
	bra.l		t_resdnrm
smod_nrm:
	fmove.l		(sp)+,fpcr
	fmove.x		DST.w(a1),fp0
	tst.b		DST_EX.w(a1)
	bmi.b		smod_nrm_neg
	rts

smod_nrm_neg:
	move.b		#neg_bmask,FPSR_CC(a6)	/*  set 'N' ccode */
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		srem_snorm
	.globl		srem_sdnrm
srem_sdnrm:
srem_snorm:
	move.b		DTAG(a6),d1
	beq.l		srem
	cmpi.b		#ZERO,d1
	beq.w		srem_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		srem
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		dst_snan

	.globl		srem_szero
srem_szero:
	move.b		DTAG(a6),d1
	beq.l		t_operr
	cmpi.b		#ZERO,d1
	beq.l		t_operr
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		t_operr
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		dst_snan

	.globl		srem_sinf
srem_sinf:
	move.b		DTAG(a6),d1
	beq.w		srem_fpn
	cmpi.b		#ZERO,d1
	beq.w		srem_zro
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		srem_fpn
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		dst_snan

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
	.globl		sscale_snorm
	.globl		sscale_sdnrm
sscale_snorm:
sscale_sdnrm:
	move.b		DTAG(a6),d1
	beq.l		sscale
	cmpi.b		#ZERO,d1
	beq.l		dst_zero
	cmpi.b		#INF,d1
	beq.l		dst_inf
	cmpi.b		#DENORM,d1
	beq.l		sscale
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		dst_snan

	.globl		sscale_szero
sscale_szero:
	move.b		DTAG(a6),d1
	beq.l		sscale
	cmpi.b		#ZERO,d1
	beq.l		dst_zero
	cmpi.b		#INF,d1
	beq.l		dst_inf
	cmpi.b		#DENORM,d1
	beq.l		sscale
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	bra.l		dst_snan

	.globl		sscale_sinf
sscale_sinf:
	move.b		DTAG(a6),d1
	beq.l		t_operr
	cmpi.b		#QNAN,d1
	beq.l		dst_qnan
	cmpi.b		#SNAN,d1
	beq.l		dst_snan
	bra.l		t_operr

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/*
 * sop_sqnan(): The src op for frem/fmod/fscale was a QNAN.
 */
	.globl		sop_sqnan
sop_sqnan:
	move.b		DTAG(a6),d1
	cmpi.b		#QNAN,d1
	beq.b		dst_qnan
	cmpi.b		#SNAN,d1
	beq.b		dst_snan
	bra.b		src_qnan

/*
 * sop_ssnan(): The src op for frem/fmod/fscale was an SNAN.
 */
	.globl		sop_ssnan
sop_ssnan:
	move.b		DTAG(a6),d1
	cmpi.b		#QNAN,d1
	beq.b		dst_qnan_src_snan
	cmpi.b		#SNAN,d1
	beq.b		dst_snan
	bra.b		src_snan

dst_qnan_src_snan:
	ori.l		#snaniop_mask,USER_FPSR(a6) /*  set NAN/SNAN/AIOP */
	bra.b		dst_qnan

/*
 * dst_qnan(): Return the dst SNAN w/ the SNAN bit set.
 */
	.globl		dst_snan
dst_snan:
	fmove.x		DST.w(a1),fp0		/*  the fmove sets the SNAN bit */
	fmove.l		fpsr,d0		/*  catch resulting status */
	or.l		d0,USER_FPSR(a6)	/*  store status */
	rts

/*
 * dst_qnan(): Return the dst QNAN.
 */
	.globl		dst_qnan
dst_qnan:
	fmove.x		DST.w(a1),fp0		/*  return the non-signalling nan */
	tst.b		DST_EX.w(a1)		/*  set ccodes according to QNAN sign */
	bmi.b		dst_qnan_m
dst_qnan_p:
	move.b		#nan_bmask,FPSR_CC(a6)
	rts
dst_qnan_m:
	move.b		#neg_bmask+nan_bmask,FPSR_CC(a6)
	rts

/*
 * src_snan(): Return the src SNAN w/ the SNAN bit set.
 */
	.globl		src_snan
src_snan:
	fmove.x		SRC.w(a0),fp0		/*  the fmove sets the SNAN bit */
	fmove.l		fpsr,d0		/*  catch resulting status */
	or.l		d0,USER_FPSR(a6)	/*  store status */
	rts

/*
 * src_qnan(): Return the src QNAN.
 */
	.globl		src_qnan
src_qnan:
	fmove.x		SRC.w(a0),fp0		/*  return the non-signalling nan */
	tst.b		SRC_EX.w(a0)		/*  set ccodes according to QNAN sign */
	bmi.b		dst_qnan_m
src_qnan_p:
	move.b		#nan_bmask,FPSR_CC(a6)
	rts
src_qnan_m:
	move.b		#neg_bmask+nan_bmask,FPSR_CC(a6)
	rts

/*
 * fkern2.s:
 *	These entry points are used by the exception handler
 * routines where an instruction is selected by an index into
 * a large jump table corresponding to a given instruction which
 * has been decoded. Flow continues here where we now decode
 * further according to the source operand type.
 */

	.globl		fsinh
fsinh:
	move.b		STAG(a6),d1
	beq.l		ssinh
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		src_inf
	cmpi.b		#DENORM,d1
	beq.l		ssinhd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		flognp1
flognp1:
	move.b		STAG(a6),d1
	beq.l		slognp1
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		sopr_inf
	cmpi.b		#DENORM,d1
	beq.l		slognp1d
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fetoxm1
fetoxm1:
	move.b		STAG(a6),d1
	beq.l		setoxm1
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		setoxm1i
	cmpi.b		#DENORM,d1
	beq.l		setoxm1d
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		ftanh
ftanh:
	move.b		STAG(a6),d1
	beq.l		stanh
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		src_one
	cmpi.b		#DENORM,d1
	beq.l		stanhd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fatan
fatan:
	move.b		STAG(a6),d1
	beq.l		satan
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		spi_2
	cmpi.b		#DENORM,d1
	beq.l		satand
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fasin
fasin:
	move.b		STAG(a6),d1
	beq.l		sasin
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		sasind
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fatanh
fatanh:
	move.b		STAG(a6),d1
	beq.l		satanh
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		satanhd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fsine
fsine:
	move.b		STAG(a6),d1
	beq.l		ssin
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		ssind
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		ftan
ftan:
	move.b		STAG(a6),d1
	beq.l		stan
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		stand
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fetox
fetox:
	move.b		STAG(a6),d1
	beq.l		setox
	cmpi.b		#ZERO,d1
	beq.l		ld_pone
	cmpi.b		#INF,d1
	beq.l		szr_inf
	cmpi.b		#DENORM,d1
	beq.l		setoxd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		ftwotox
ftwotox:
	move.b		STAG(a6),d1
	beq.l		stwotox
	cmpi.b		#ZERO,d1
	beq.l		ld_pone
	cmpi.b		#INF,d1
	beq.l		szr_inf
	cmpi.b		#DENORM,d1
	beq.l		stwotoxd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		ftentox
ftentox:
	move.b		STAG(a6),d1
	beq.l		stentox
	cmpi.b		#ZERO,d1
	beq.l		ld_pone
	cmpi.b		#INF,d1
	beq.l		szr_inf
	cmpi.b		#DENORM,d1
	beq.l		stentoxd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		flogn
flogn:
	move.b		STAG(a6),d1
	beq.l		slogn
	cmpi.b		#ZERO,d1
	beq.l		t_dz2
	cmpi.b		#INF,d1
	beq.l		sopr_inf
	cmpi.b		#DENORM,d1
	beq.l		slognd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		flog10
flog10:
	move.b		STAG(a6),d1
	beq.l		slog10
	cmpi.b		#ZERO,d1
	beq.l		t_dz2
	cmpi.b		#INF,d1
	beq.l		sopr_inf
	cmpi.b		#DENORM,d1
	beq.l		slog10d
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		flog2
flog2:
	move.b		STAG(a6),d1
	beq.l		slog2
	cmpi.b		#ZERO,d1
	beq.l		t_dz2
	cmpi.b		#INF,d1
	beq.l		sopr_inf
	cmpi.b		#DENORM,d1
	beq.l		slog2d
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fcosh
fcosh:
	move.b		STAG(a6),d1
	beq.l		scosh
	cmpi.b		#ZERO,d1
	beq.l		ld_pone
	cmpi.b		#INF,d1
	beq.l		ld_pinf
	cmpi.b		#DENORM,d1
	beq.l		scoshd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		facos
facos:
	move.b		STAG(a6),d1
	beq.l		sacos
	cmpi.b		#ZERO,d1
	beq.l		ld_ppi2
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		sacosd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fcos
fcos:
	move.b		STAG(a6),d1
	beq.l		scos
	cmpi.b		#ZERO,d1
	beq.l		ld_pone
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		scosd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fgetexp
fgetexp:
	move.b		STAG(a6),d1
	beq.l		sgetexp
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		sgetexpd
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fgetman
fgetman:
	move.b		STAG(a6),d1
	beq.l		sgetman
	cmpi.b		#ZERO,d1
	beq.l		src_zero
	cmpi.b		#INF,d1
	beq.l		t_operr
	cmpi.b		#DENORM,d1
	beq.l		sgetmand
	cmpi.b		#QNAN,d1
	beq.l		src_qnan
	bra.l		src_snan

	.globl		fsincos
fsincos:
	move.b		STAG(a6),d1
	beq.l		ssincos
	cmpi.b		#ZERO,d1
	beq.l		ssincosz
	cmpi.b		#INF,d1
	beq.l		ssincosi
	cmpi.b		#DENORM,d1
	beq.l		ssincosd
	cmpi.b		#QNAN,d1
	beq.l		ssincosqnan
	bra.l		ssincossnan

	.globl		fmod
fmod:
	move.b		STAG(a6),d1
	beq.l		smod_snorm
	cmpi.b		#ZERO,d1
	beq.l		smod_szero
	cmpi.b		#INF,d1
	beq.l		smod_sinf
	cmpi.b		#DENORM,d1
	beq.l		smod_sdnrm
	cmpi.b		#QNAN,d1
	beq.l		sop_sqnan
	bra.l		sop_ssnan

	.globl		frem
frem:
	move.b		STAG(a6),d1
	beq.l		srem_snorm
	cmpi.b		#ZERO,d1
	beq.l		srem_szero
	cmpi.b		#INF,d1
	beq.l		srem_sinf
	cmpi.b		#DENORM,d1
	beq.l		srem_sdnrm
	cmpi.b		#QNAN,d1
	beq.l		sop_sqnan
	bra.l		sop_ssnan

	.globl		fscale
fscale:
	move.b		STAG(a6),d1
	beq.l		sscale_snorm
	cmpi.b		#ZERO,d1
	beq.l		sscale_szero
	cmpi.b		#INF,d1
	beq.l		sscale_sinf
	cmpi.b		#DENORM,d1
	beq.l		sscale_sdnrm
	cmpi.b		#QNAN,d1
	beq.l		sop_sqnan
	bra.l		sop_ssnan

