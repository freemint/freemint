/* 
 * 	fpsp.h 3.3 3.3
 * 
 *
 * 		Copyright (C) Motorola, Inc. 1990
 * 			All Rights Reserved
 * 
 *        For details on the license for this file, please see the
 *        file, README, in this same directory.
 *
 * 	fpsp.h --- stack frame offsets during FPSP exception handling
 * 
 * 	These equates are used to access the exception frame, the fsave
 * 	frame and any local variables needed by the FPSP package.
 * 
 * 	All FPSP handlers begin by executing:
 * 
 * 		link	a6,#-LOCAL_SIZE
 * 		fsave	-(a7)
 * 		movem.l	d0-d1/a0-a1,USER_DA(a6)
 * 		fmovem.x fp0-fp3,USER_FP0(a6)
 * 		fmove.l	fpsr/fpcr/fpiar,USER_FPSR(a6)
 * 
 * 	After initialization, the stack looks like this:
 * 
 *        A7 ---> +-------------------------------+
 *                |                               |
 *                |       FPU fsave area          |
 *                |                               |
 *                +-------------------------------+
 *                |                               |
 *                |       FPSP Local Variables    |
 *                |            including          |
 *                |         saved registers       |
 *                |                               |
 *                +-------------------------------+
 *        A6 ---> |       Saved A6                |
 *                +-------------------------------+
 *                |                               |
 *                |       Exception Frame         |
 *                |                               |
 *                |                               |
 * 
 * 	Positive offsets from A6 refer to the exception frame.  Negative
 * 	offsets refer to the Local Variable area and the fsave area.
 * 	The fsave frame is also accessible from the top via A7.
 * 
 * 	On exit, the handlers execute:
 * 
 * 		movem.l	USER_DA(a6),d0-d1/a0-a1
 * 		fmovem.x USER_FP0(a6),fp0-fp3
 * 		fmove.l	USER_FPSR(a6),fpsr/fpcr/fpiar
 * 		frestore (a7)+
 * 		unlk	a6
 * 
 * 	and then either "bra fpsp_done" if the exception was completely
 * 	handled	by the package, or "bra real_xxxx" which is an external
 * 	label to a routine that will process a real exception of the
 * 	type that was generated.  Some handlers may omit the "frestore"
 * 	if the FPU state after the exception is idle.
 * 
 * 	Sometimes the exception handler will transform the fsave area
 * 	because it needs to report an exception back to the user.  This
 * 	can happen if the package is entered for an unimplemented float
 * 	instruction that generates (say) an underflow.  Alternatively,
 * 	a second fsave frame can be pushed onto the stack and the
 * 	handler	exit code will reload the new frame and discard the old.
 * 
 * 	The registers d0, d1, a0, a1 and fp0-fp3 are always saved and
 * 	restored from the "local variable" area and can be used as
 * 	temporaries.  If a routine needs to change any
 * 	of these registers, it should modify the saved copy and let
 * 	the handler exit code restore the value.
 * 
 * ----------------------------------------------------------------------
 */
/* 
 * 	Local Variables on the stack
 */
	LOCAL_SIZE = 192		/* bytes needed for local variables */
	LV = -LOCAL_SIZE		/* convenient base value */

	USER_DA = LV+0			/* save space for D0-D1,A0-A1 */
	USER_D0 = LV+0			/* saved user D0 */
	USER_D1 = LV+4			/* saved user D1 */
	USER_A0 = LV+8			/* saved user A0 */
	USER_A1 = LV+12			/* saved user A1 */
	USER_FP0 = LV+16		/* saved user FP0 */
	USER_FP1 = LV+28		/* saved user FP1 */
	USER_FP2 = LV+40		/* saved user FP2 */
	USER_FP3 = LV+52		/* saved user FP3 */
	USER_FPCR = LV+64		/* saved user FPCR */
	FPCR_ENABLE = USER_FPCR+2	/* FPCR exception enable */
	FPCR_MODE = USER_FPCR+3	/* FPCR rounding mode control */
	USER_FPSR = LV+68		/* saved user FPSR */
	FPSR_CC = USER_FPSR+0	/* FPSR condition code */
	FPSR_QBYTE = USER_FPSR+1	/* FPSR quotient */
	FPSR_EXCEPT = USER_FPSR+2	/* FPSR exception */
	FPSR_AEXCEPT = USER_FPSR+3	/* FPSR accrued exception */
	USER_FPIAR = LV+72		/* saved user FPIAR */
	FP_SCR1 = LV+76			/* room for a temporary float value */
	FP_SCR2 = LV+92			/* room for a temporary float value */
	L_SCR1 = LV+108			/* room for a temporary long value */
	L_SCR2 = LV+112			/* room for a temporary long value */
	STORE_FLG = LV+116
	BINDEC_FLG = LV+117		/* used in bindec */
	DNRM_FLG = LV+118		/* used in res_func */
	RES_FLG = LV+119		/* used in res_func */
	DY_MO_FLG = LV+120		/* dyadic/monadic flag */
	UFLG_TMP = LV+121		/* temporary for uflag errata */
	CU_ONLY = LV+122		/* cu-only flag */
	VER_TMP = LV+123		/* temp holding for version number */
	L_SCR3 = LV+124			/* room for a temporary long value */
	FP_SCR3 = LV+128		/* room for a temporary float value */
	FP_SCR4 = LV+144		/* room for a temporary float value */
	FP_SCR5 = LV+160		/* room for a temporary float value */
	FP_SCR6 = LV+176
/*
 * NEXT		equ	LV+192		need to increase LOCAL_SIZE
 *
 * --------------------------------------------------------------------------
 *
 * 	fsave offsets and bit definitions
 * 
 * 	Offsets are defined from the end of an fsave because the last 10
 * 	words of a busy frame are the same as the unimplemented frame.
 */
	CU_SAVEPC = LV-92		/* micro-pc for CU (1 byte) */
	FPR_DIRTY_BITS = LV-91		/* fpr dirty bits */

	WBTEMP = LV-76			/* write back temp (12 bytes) */
	WBTEMP_EX = WBTEMP		/* wbtemp sign and exponent (2 bytes) */
	WBTEMP_HI = WBTEMP+4	/* wbtemp mantissa [63:32] (4 bytes) */
	WBTEMP_LO = WBTEMP+8	/* wbtemp mantissa [31:00] (4 bytes) */

	WBTEMP_SGN = WBTEMP+2	/* used to store sign */

	FPSR_SHADOW = LV-64		/* fpsr shadow reg */

	FPIARCU = LV-60			/* Instr. addr. reg. for CU (4 bytes) */

	CMDREG2B = LV-52		/* cmd reg for machine 2 */
	CMDREG3B = LV-48		/* cmd reg for E3 exceptions (2 bytes) */

	NMNEXC = LV-44			/* NMNEXC (unsup,snan bits only) */
	nmn_unsup_bit = 1
	nmn_snan_bit = 0

	NMCEXC = LV-43			/* NMNEXC & NMCEXC */
	nmn_operr_bit = 7
	nmn_ovfl_bit = 6
	nmn_unfl_bit = 5
	nmc_unsup_bit = 4
	nmc_snan_bit = 3
	nmc_operr_bit = 2
	nmc_ovfl_bit = 1
	nmc_unfl_bit = 0

	STAG = LV-40			/* source tag (1 byte) */
	WBTEMP_GRS = LV-40		/* alias wbtemp guard, round, sticky */
	guard_bit = 1			/* guard bit is bit number 1 */
	round_bit = 0			/* round bit is bit number 0 */
	stag_mask = 0xE0		/* upper 3 bits are source tag type */
	denorm_bit = 7			/* bit determines if denorm or unnorm */
	etemp15_bit = 4			/* etemp exponent bit #15 */
	wbtemp66_bit = 2		/* wbtemp mantissa bit #66 */
	wbtemp1_bit = 1			/* wbtemp mantissa bit #1 */
	wbtemp0_bit = 0			/* wbtemp mantissa bit #0 */

	STICKY = LV-39			/* holds sticky bit */
	sticky_bit = 7

	CMDREG1B = LV-36		/* cmd reg for E1 exceptions (2 bytes) */
	kfact_bit = 12			/* distinguishes static/dynamic k-factor */
							/* on packed move outs.  NOTE: this */
							/* equate only works when CMDREG1B is in */
							/* a register. */

	CMDWORD = LV-35			/* command word in cmd1b */
	direction_bit = 5		/* bit 0 in opclass */
	size_bit2 = 12			/* bit 2 in size field */

	DTAG = LV-32			/* dest tag (1 byte) */
	dtag_mask = 0xE0		/* upper 3 bits are dest type tag */
	fptemp15_bit = 4		/* fptemp exponent bit #15 */

	WB_BYTE = LV-31			/* holds WBTE15 bit (1 byte) */
	wbtemp15_bit = 4		/* wbtemp exponent bit #15 */

	E_BYTE = LV-28			/* holds E1 and E3 bits (1 byte) */
	E1 = 2					/* which bit is E1 flag */
	E3 = 1					/* which bit is E3 flag */
	SFLAG = 0				/* which bit is S flag */

	T_BYTE = LV-27			/* holds T and U bits (1 byte) */
	XFLAG = 7				/* which bit is X flag */
	UFLAG = 5				/* which bit is U flag */
	TFLAG = 4				/* which bit is T flag */

	FPTEMP = LV-24			/* fptemp (12 bytes) */
	FPTEMP_EX = FPTEMP		/* fptemp sign and exponent (2 bytes) */
	FPTEMP_HI = FPTEMP+4	/* fptemp mantissa [63:32] (4 bytes) */
	FPTEMP_LO = FPTEMP+8	/* fptemp mantissa [31:00] (4 bytes) */

	FPTEMP_SGN = FPTEMP+2	/* used to store sign */

	ETEMP = LV-12			/* etemp (12 bytes) */
	ETEMP_EX = ETEMP		/* etemp sign and exponent (2 bytes) */
	ETEMP_HI = ETEMP+4		/* etemp mantissa [63:32] (4 bytes) */
	ETEMP_LO = ETEMP+8		/* etemp mantissa [31:00] (4 bytes) */

	ETEMP_SGN = ETEMP+2		/* used to store sign */

	EXC_SR = 4			/* exception frame status register */
	EXC_PC = 6			/* exception frame program counter */
	EXC_VEC = 10		/* exception frame vector (format+vector#) */
	EXC_EA = 12			/* exception frame effective address */
/*
 * --------------------------------------------------------------------------
 *
 *	FPSR/FPCR bits
 */
	neg_bit = 3		/* negative result */
	z_bit = 2		/* zero result */
	inf_bit = 1		/* infinity result */
	nan_bit = 0		/* not-a-number result */

	q_sn_bit = 7	/* sign bit of quotient byte */

	bsun_bit = 7	/* branch on unordered */
	snan_bit = 6	/* signalling nan */
	operr_bit = 5	/* operand error */
	ovfl_bit = 4	/* overflow */
	unfl_bit = 3	/* underflow */
	dz_bit = 2		/* divide by zero */
	inex2_bit = 1	/* inexact result 2 */
	inex1_bit = 0	/* inexact result 1 */

	aiop_bit = 7	/* accrued illegal operation */
	aovfl_bit = 6	/* accrued overflow */
	aunfl_bit = 5	/* accrued underflow */
	adz_bit = 4		/* accrued divide by zero */
	ainex_bit = 3	/* accrued inexact */
/*
 *	FPSR individual bit masks
 */
	neg_mask = 0x08000000
	z_mask = 0x04000000
	inf_mask = 0x02000000
	nan_mask = 0x01000000

	bsun_mask = 0x00008000
	snan_mask = 0x00004000
	operr_mask = 0x00002000
	ovfl_mask = 0x00001000
	unfl_mask = 0x00000800
	dz_mask = 0x00000400
	inex2_mask = 0x00000200
	inex1_mask = 0x00000100

	aiop_mask = 0x00000080	/* accrued illegal operation */
	aovfl_mask = 0x00000040	/* accrued overflow */
	aunfl_mask = 0x00000020	/* accrued underflow */
	adz_mask = 0x00000010	/* accrued divide by zero */
	ainex_mask = 0x00000008	/* accrued inexact */
/*
 *	FPSR combinations used in the FPSP
 */
	dzinf_mask = inf_mask+dz_mask+adz_mask
	opnan_mask = nan_mask+operr_mask+aiop_mask
	nzi_mask = 0x01ffffff	/* clears N, Z, and I */
	unfinx_mask = unfl_mask+inex2_mask+aunfl_mask+ainex_mask
	unf2inx_mask = unfl_mask+inex2_mask+ainex_mask
	ovfinx_mask = ovfl_mask+inex2_mask+aovfl_mask+ainex_mask
	inx1a_mask = inex1_mask+ainex_mask
	inx2a_mask = inex2_mask+ainex_mask
	snaniop_mask = nan_mask+snan_mask+aiop_mask
	naniop_mask = nan_mask+aiop_mask
	neginf_mask = neg_mask+inf_mask
	infaiop_mask = inf_mask+aiop_mask
	negz_mask = neg_mask+z_mask
	opaop_mask = operr_mask+aiop_mask
	unfl_inx_mask = unfl_mask+aunfl_mask+ainex_mask
	ovfl_inx_mask = ovfl_mask+aovfl_mask+ainex_mask
/*
 * --------------------------------------------------------------------------
 *
 *	FPCR rounding modes
 */
	x_mode = 0x00	/* round to extended */
	s_mode = 0x40	/* round to single */
	d_mode = 0x80	/* round to double */

	rn_mode = 0x00	/* round nearest */
	rz_mode = 0x10	/* round to zero */
	rm_mode = 0x20	/* round to minus infinity */
	rp_mode = 0x30	/* round to plus infinity */
/*
 * --------------------------------------------------------------------------
 *
 *	Miscellaneous equates
 */
	signan_bit = 6	/* signalling nan bit in mantissa */
	sign_bit = 7

	rnd_stky_bit = 29	/* round/sticky bit of mantissa */
				/* this can only be used if in a data register */
	sx_mask = 0x01800000 /* set s and x bits in word $48 */

	LOCAL_EX = 0
	LOCAL_SGN = 2
	LOCAL_HI = 4
	LOCAL_LO = 8
	LOCAL_GRS = 12	/* valid ONLY for FP_SCR1, FP_SCR2 */

	norm_tag = 0x00	/* tag bits in {7:5} position */
	zero_tag = 0x20
	inf_tag = 0x40
	nan_tag = 0x60
	dnrm_tag = 0x80

/* fsave sizes and formats */
	VER_4 = 0x40		/* fpsp compatible version numbers */
						/* are in the $40s {$40-$4f} */
	VER_40 = 0x40		/* original version number */
	VER_41 = 0x41		/* revision version number */

	BUSY_SIZE = 100				/* size of busy frame */
	BUSY_FRAME = LV-BUSY_SIZE	/* start of busy frame */

	UNIMP_40_SIZE = 44			/* size of orig unimp frame */
	UNIMP_41_SIZE = 52			/* size of rev unimp frame */

	IDLE_SIZE = 4				/* size of idle frame */
	IDLE_FRAME = LV-IDLE_SIZE	/* start of idle frame */
/*
 *	exception vectors
 */
	TRACE_VEC = 0x2024		/* trace trap */
	FLINE_VEC = 0x002C		/* real F-line */
	UNIMP_VEC = 0x202C		/* unimplemented */
	INEX_VEC = 0x00C4

	dbl_thresh = 0x3C01
	sgl_thresh = 0x3F81
