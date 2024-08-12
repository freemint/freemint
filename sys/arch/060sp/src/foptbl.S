/* 
 *  This table holds the offsets of the emulation routines for each individual
 *  math operation relative to the address of this table. Included are
 *  routines like fadd/fmul/fabs as well as the transcendentals.
 *  The location within the table is determined by the extension bits of the
 *  operation longword.
 */

	.include "hdr.fpu"

	.text

	.globl tbl_unsupp
	/* swbeg		#109 */
	.dc.w 0x4afc,109
tbl_unsupp:
	.dc.l		fin-tbl_unsupp		/*  00: fmove */
	.dc.l		fint-tbl_unsupp		/*  01: fint */
	.dc.l		fsinh-tbl_unsupp	/*  02: fsinh */
	.dc.l		fintrz-tbl_unsupp	/*  03: fintrz */
	.dc.l		fsqrt-tbl_unsupp	/*  04: fsqrt */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		flognp1-tbl_unsupp	/*  06: flognp1 */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fetoxm1-tbl_unsupp	/*  08: fetoxm1 */
	.dc.l		ftanh-tbl_unsupp	/*  09: ftanh */
	.dc.l		fatan-tbl_unsupp	/*  0a: fatan */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fasin-tbl_unsupp	/*  0c: fasin */
	.dc.l		fatanh-tbl_unsupp	/*  0d: fatanh */
	.dc.l		fsine-tbl_unsupp	/*  0e: fsin */
	.dc.l		ftan-tbl_unsupp		/*  0f: ftan */
	.dc.l		fetox-tbl_unsupp	/*  10: fetox */
	.dc.l		ftwotox-tbl_unsupp	/*  11: ftwotox */
	.dc.l		ftentox-tbl_unsupp	/*  12: ftentox */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		flogn-tbl_unsupp	/*  14: flogn */
	.dc.l		flog10-tbl_unsupp	/*  15: flog10 */
	.dc.l		flog2-tbl_unsupp	/*  16: flog2 */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fabs-tbl_unsupp		/*  18: fabs */
	.dc.l		fcosh-tbl_unsupp	/*  19: fcosh */
	.dc.l		fneg-tbl_unsupp		/*  1a: fneg */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		facos-tbl_unsupp	/*  1c: facos */
	.dc.l		fcos-tbl_unsupp		/*  1d: fcos */
	.dc.l		fgetexp-tbl_unsupp	/*  1e: fgetexp */
	.dc.l		fgetman-tbl_unsupp	/*  1f: fgetman */
	.dc.l		fdiv-tbl_unsupp		/*  20: fdiv */
	.dc.l		fmod-tbl_unsupp		/*  21: fmod */
	.dc.l		fadd-tbl_unsupp		/*  22: fadd */
	.dc.l		fmul-tbl_unsupp		/*  23: fmul */
	.dc.l		fsgldiv-tbl_unsupp	/*  24: fsgldiv */
	.dc.l		frem-tbl_unsupp		/*  25: frem */
	.dc.l		fscale-tbl_unsupp	/*  26: fscale */
	.dc.l		fsglmul-tbl_unsupp	/*  27: fsglmul */
	.dc.l		fsub-tbl_unsupp		/*  28: fsub */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fsincos-tbl_unsupp	/*  30: fsincos */
	.dc.l		fsincos-tbl_unsupp	/*  31: fsincos */
	.dc.l		fsincos-tbl_unsupp	/*  32: fsincos */
	.dc.l		fsincos-tbl_unsupp	/*  33: fsincos */
	.dc.l		fsincos-tbl_unsupp	/*  34: fsincos */
	.dc.l		fsincos-tbl_unsupp	/*  35: fsincos */
	.dc.l		fsincos-tbl_unsupp	/*  36: fsincos */
	.dc.l		fsincos-tbl_unsupp	/*  37: fsincos */
	.dc.l		fcmp-tbl_unsupp		/*  38: fcmp */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		ftst-tbl_unsupp		/*  3a: ftst */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fsin-tbl_unsupp		/*  40: fsmove */
	.dc.l		fssqrt-tbl_unsupp	/*  41: fssqrt */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fdin-tbl_unsupp		/*  44: fdmove */
	.dc.l		fdsqrt-tbl_unsupp	/*  45: fdsqrt */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fsabs-tbl_unsupp	/*  58: fsabs */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fsneg-tbl_unsupp	/*  5a: fsneg */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fdabs-tbl_unsupp	/*  5c: fdabs */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fdneg-tbl_unsupp	/*  5e: fdneg */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fsdiv-tbl_unsupp	/*  60: fsdiv */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fsadd-tbl_unsupp	/*  62: fsadd */
	.dc.l		fsmul-tbl_unsupp	/*  63: fsmul */
	.dc.l		fddiv-tbl_unsupp	/*  64: fddiv */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fdadd-tbl_unsupp	/*  66: fdadd */
	.dc.l		fdmul-tbl_unsupp	/*  67: fdmul */
	.dc.l		fssub-tbl_unsupp	/*  68: fssub */
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		tbl_unsupp-tbl_unsupp
	.dc.l		fdsub-tbl_unsupp	/*  6c: fdsub */

