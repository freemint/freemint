/*
 * MOTOROLA MICROPROCESSOR & MEMORY TECHNOLOGY GROUP
 * M68000 Hi-Performance Microprocessor Division
 * M68060 Software Package
 * Production Release P1.00 -- October 10, 1994
 * 
 * M68060 Software Package Copyright © 1993, 1994 Motorola Inc.  All rights reserved.
 * 
 * THE SOFTWARE is provided on an "AS IS" basis and without warranty.
 * To the maximum extent permitted by applicable law,
 * MOTOROLA DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
 * INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * and any warranty against infringement with regard to the SOFTWARE
 * (INCLUDING ANY MODIFIED VERSIONS THEREOF) and any accompanying written materials.
 * 
 * To the maximum extent permitted by applicable law,
 * IN NO EVENT SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER
 * (INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
 * BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS)
 * ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.
 * Motorola assumes no responsibility for the maintenance and support of the SOFTWARE.
 * 
 * You are hereby granted a copyright license to use, modify, and distribute the SOFTWARE
 * so long as this entire notice is retained without alteration in any modified and/or
 * redistributed versions, and that such modified versions are clearly identified as such.
 * No licenses are granted by implication, estoppel or otherwise under any patents
 * or trademarks of Motorola, Inc.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
		.text

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
SREGS = -64
IREGS = -128
IFPREGS = -224
SFPREGS = -320
IFPCREGS = -332
SFPCREGS = -344
ICCR = -346
SCCR = -348
TESTCTR = -352
DATA = -384
ERRORCTR = -388
LOCAL_SIZE = -388

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
TESTTOP:
CPUTYPE = TESTTOP-2
	bra.l		_060TESTS_
	.dc.w		0x0000

	bra.l		_060TESTS_unimp
	.dc.w		0x0000

	bra.l		_060TESTS_enable
	.dc.w		0x0000

start_str:
	.asciz		"Testing 68060 FPSP started:\n"

start_str_unimp:
	.asciz		"Testing 68060 FPSP unimplemented instruction started:\n"

start_str_enable:
	.asciz		"Testing 68060 FPSP exception enabled started:\n"

pass_str:
	.asciz		"passed\n"

fail_str:
	.asciz		" failed\n"

	.even
	.balignw    4,0x51fc
chk_test:
	tst.l		ERRORCTR(a6)
	bne.b		test_fail
test_pass:
	pea			pass_str(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp
	rts
test_fail:
    move.l      TESTCTR(a6),d1
	move.l		d1,-(sp)
	bsr.l		_print_num
	addq.l		#0x4,sp

	pea			fail_str(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
_060TESTS_:
	link		a6,#LOCAL_SIZE

	movem.l		d2-d7/a2-a5,-(sp)
	fmovem.x	fp0-fp7,-(sp)

	pea			start_str(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

/* effadd */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(effadd_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		effadd_0

	bsr.l		chk_test

/* unsupp */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(unsupp_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		unsupp_0

	bsr.l		chk_test

/* ovfl non-maskable */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(ovfl_nm_str).l(pc)
	bsr.l		_print_str
	bsr.l		ovfl_nm_0

	bsr.l		chk_test

/* unfl non-maskable */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(unfl_nm_str).l(pc)
	bsr.l		_print_str
	bsr.l		unfl_nm_0

	bsr.l		chk_test

	movem.l		(sp)+,d2-d7/a2-a5
	fmovem.x	(sp)+,fp0-fp7

	unlk		a6
	rts

_060TESTS_unimp:
	link		a6,#LOCAL_SIZE

	movem.l		d2-d7/a2-a5,-(sp)
	fmovem.x	fp0-fp7,-(sp)

	pea			start_str_unimp(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

/* unimp */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(unimp_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		unimp_0

	bsr.l		chk_test

	movem.l		(sp)+,d2-d7/a2-a5
	fmovem.x	(sp)+,fp0-fp7

	unlk		a6
	rts

_060TESTS_enable:
	link		a6,#LOCAL_SIZE

	movem.l		d2-d7/a2-a5,-(sp)
	fmovem.x	fp0-fp7,-(sp)

	pea			start_str_enable(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

/* snan */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(snan_str).l(pc)
	bsr.l		_print_str
	bsr.l		snan_0

	bsr.l		chk_test

/* operr */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(operr_str).l(pc)
	bsr.l		_print_str
	bsr.l		operr_0

	bsr.l		chk_test

/* ovfl */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(ovfl_str).l(pc)
	bsr.l		_print_str
	bsr.l		ovfl_0

	bsr.l		chk_test

/* unfl */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(unfl_str).l(pc)
	bsr.l		_print_str
	bsr.l		unfl_0

	bsr.l		chk_test

/* dz */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(dz_str).l(pc)
	bsr.l		_print_str
	bsr.l		dz_0

	bsr.l		chk_test

/* inexact */
	clr.l		TESTCTR(a6)
	clr.l		ERRORCTR(a6)
	pea			(inex_str).l(pc)
	bsr.l		_print_str
	bsr.l		inex_0

	bsr.l		chk_test

	movem.l		(sp)+,d2-d7/a2-a5
	fmovem.x	(sp)+,fp0-fp7

	unlk		a6
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

unimp_str:
	.asciz		"\tUnimplemented FP instructions..."

	.even
	.balignw    4,0x51fc
unimp_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0x40000000,DATA+0x0(a6)
	move.l		#0xc90fdaa2,DATA+0x4(a6)
	move.l		#0x2168c235,DATA+0x8(a6)

	move.w		#0x0000,ccr
unimp_0_pc:
	fsin.x		DATA(a6),fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0xbfbf0000,IFPREGS+0x0(a6)
	move.l		#0x80000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x08000208,IFPCREGS+0x4(a6)
	lea			unimp_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

unimp_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0x3ffe0000,DATA+0x0(a6)
	move.l		#0xc90fdaa2,DATA+0x4(a6)
	move.l		#0x2168c235,DATA+0x8(a6)

	move.w		#0x0000,ccr
unimp_1_pc:
	ftan.x		DATA(a6),fp0      /* test #2 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x3fff0000,IFPREGS+0x0(a6)
	move.l		#0x80000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x00000208,IFPCREGS+0x4(a6)
	lea			unimp_1_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

/* fmovecr */
unimp_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.w		#0x0000,ccr
unimp_2_pc:
	fmovecr.x	#0x31,fp0      /* test #3 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x40000000,IFPREGS+0x0(a6)
	move.l		#0x935d8ddd,IFPREGS+0x4(a6)
	move.l		#0xaaa8ac17,IFPREGS+0x8(a6)
	move.l		#0x00000208,IFPCREGS+0x4(a6)
	lea			unimp_2_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

/* fscc */
unimp_3:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	fmove.l		#0x0f000000,fpsr
	moveq.l		#0x00,d7

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.w		#0x0000,ccr
unimp_3_pc:
	fsgt		d7      /* test #4 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)
	move.l		#0x0f008080,IFPCREGS+0x4(a6)
	lea			unimp_3_pc(pc),a0
	bsr         zero_fpiar_if_040
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

/* fdbcc */
unimp_4:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	fmove.l		#0x0f000000,fpsr
	moveq.l		#0x2,d7

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.w		#0x0000,ccr
unimp_4_pc:
	fdbgt		d7,unimp_4_pc      /* test #5 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)
	move.w		#0xffff,IREGS+28+2(a6)
	move.l		#0x0f008080,IFPCREGS+0x4(a6)
	lea			unimp_4_pc(pc),a0
	bsr         zero_fpiar_if_040
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

/* ftrapcc */
unimp_5:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	fmove.l		#0x0f000000,fpsr

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.w		#0x0000,ccr
unimp_5_pc:
	ftrapgt.l	#0xabcdef01      /* test #6 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)
	move.l		#0x0f008080,IFPCREGS+0x4(a6)
	lea			unimp_5_pc(pc),a0
	bsr         zero_fpiar_if_040
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

effadd_str:
	.asciz		"\tUnimplemented <ea>..."

	.even
	.balignw    4,0x51fc
effadd_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	fmove.b		#0x2,fp0

	move.w		#0x0000,ccr
effadd_0_pc:
	fmul.x		#0xc00000008000000000000000,fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0xc0010000,IFPREGS+0x0(a6)
	move.l		#0x80000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x08000000,IFPCREGS+0x4(a6)
	lea			effadd_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

effadd_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.w		#0x0000,ccr
effadd_1_pc:
	fabs.p		#0xc12300012345678912345678,fp0      /* test #2 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x3e660000,IFPREGS+0x0(a6)
	move.l		#0xd0ed23e8,IFPREGS+0x4(a6)
	move.l		#0xd14035bc,IFPREGS+0x8(a6)
	move.l		#0x00000108,IFPCREGS+0x4(a6)
	lea			effadd_1_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

fmovml_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	/* fmovem.l	#0xfffffff0ffffffff,fpcr/fpsr */      /* test #3 */
    .dc.l 0xf23c9800,0xfffffff0,0xffffffff /* sometimes miscompiled by gas */
    
	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)
	move.l		#0x0000fff0,IFPCREGS+0x0(a6)
	move.l		#0x0ffffff8,IFPCREGS+0x4(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

fmovml_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	/* fmovem.l	#0xfffffff0ffffffff,fpcr/fpiar */      /* test #4 */
    .dc.l 0xf23c9400,0xfffffff0,0xffffffff /* sometimes miscompiled by gas */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)
	move.l		#0x0000fff0,IFPCREGS+0x0(a6)
	move.l		#0xffffffff,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

fmovml_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	/* fmovem.l	#0xffffffffffffffff,fpsr/fpiar */      /* test #5 */
    .dc.l 0xf23c8c00,0xffffffff,0xffffffff /* sometimes miscompiled by gas */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)
	move.l		#0x0ffffff8,IFPCREGS+0x4(a6)
	move.l		#0xffffffff,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

fmovml_3:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	/* fmovem.l	#0xfffffff0ffffffffffffffff,fpcr/fpsr/fpiar */      /* test #6 */
    .dc.l 0xf23c9c00,0xfffffff0,0xffffffff,0xffffffff /* sometimes miscompiled by gas */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)
	move.l		#0x0000fff0,IFPCREGS+0x0(a6)
	move.l		#0x0ffffff8,IFPCREGS+0x4(a6)
	move.l		#0xffffffff,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

/* fmovmx dynamic */
fmovmx_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	fmove.b		#0x1,fp0
	fmove.b		#0x2,fp1
	fmove.b		#0x3,fp2
	fmove.b		#0x4,fp3
	fmove.b		#0x5,fp4
	fmove.b		#0x6,fp5
	fmove.b		#0x7,fp6
	fmove.b		#0x8,fp7

	fmove.l		#0x0,fpiar
	moveq.l		#0xffffffaa,d0

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a7,IREGS(a6)

	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)

	move.w		#0x0000,ccr

	fmovem.x	d0,-(sp)      /* test #7 */

	move.w		ccr,SCCR(a6)

	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	fmove.s		#0x7f800000,fp1
	fmove.s		#0x7f800000,fp3
	fmove.s		#0x7f800000,fp5
	fmove.s		#0x7f800000,fp7

	fmove.x		(sp)+,fp1
	fmove.x		(sp)+,fp3
	fmove.x		(sp)+,fp5
	fmove.x		(sp)+,fp7

	movem.l		d0-d7/a0-a7,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

fmovmx_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	fmove.b		#0x1,fp0
	fmove.b		#0x2,fp1
	fmove.b		#0x3,fp2
	fmove.b		#0x4,fp3
	fmove.b		#0x5,fp4
	fmove.b		#0x6,fp5
	fmove.b		#0x7,fp6
	fmove.b		#0x8,fp7

	fmove.x		fp6,-(sp)
	fmove.x		fp4,-(sp)
	fmove.x		fp2,-(sp)
	fmove.x		fp0,-(sp)

	fmovem.x	fp0-fp7,IFPREGS(a6)

	fmove.s		#0x7f800000,fp6
	fmove.s		#0x7f800000,fp4
	fmove.s		#0x7f800000,fp2
	fmove.s		#0x7f800000,fp0

	fmove.l		#0x0,fpiar
	fmove.l		#0x0,fpsr
	moveq.l		#0xffffffaa,d0

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a7,IREGS(a6)

	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.w		#0x0000,ccr

	fmovem.x	(sp)+,d0      /* test #8 */

	move.w		ccr,SCCR(a6)

	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	movem.l		d0-d7/a0-a7,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

fmovmx_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	fmove.b		#0x1,fp0
	fmove.b		#0x2,fp1
	fmove.b		#0x3,fp2
	fmove.b		#0x4,fp3
	fmove.b		#0x5,fp4
	fmove.b		#0x6,fp5
	fmove.b		#0x7,fp6
	fmove.b		#0x8,fp7

	fmove.l		#0x0,fpiar
	move.l		#0xffffff00,d0

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a7,IREGS(a6)

	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)

	move.w		#0x0000,ccr

	fmovem.x	d0,-(sp)      /* test #9 */

	move.w		ccr,SCCR(a6)

	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	movem.l		d0-d7/a0-a7,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* This test will take a non-maskable overflow directly. */
ovfl_nm_str:
	.asciz		"\tNon-maskable overflow..."

	.even
	.balignw    4,0x51fc
ovfl_nm_0:
	addq.L		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	fmove.b		#0x2,fp0
	move.l		#0x7ffe0000,DATA+0x0(a6)
	move.l		#0x80000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)

	move.w		#0x0000,ccr
ovfl_nm_0_pc:
	fmul.x		DATA(a6),fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x7fff0000,IFPREGS+0x0(a6)
	move.l		#0x00000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x02001048,IFPCREGS+0x4(a6)
	lea			ovfl_nm_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* This test will take an overflow directly. */
ovfl_str:
	.asciz		"\tEnabled overflow..."

	.even
	.balignw    4,0x51fc
ovfl_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmove.l		#0x00001000,fpcr
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	fmove.b		#0x2,fp0
	move.l		#0x7ffe0000,DATA+0x0(a6)
	move.l		#0x80000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)

	move.w		#0x0000,ccr
ovfl_0_pc:
	fmul.x		DATA(a6),fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x7fff0000,IFPREGS+0x0(a6)
	move.l		#0x00000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x02001048,IFPCREGS+0x4(a6)
	lea			ovfl_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* This test will take an underflow directly. */
unfl_str:
	.asciz		"\tEnabled underflow..."

	.even
	.balignw    4,0x51fc
unfl_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmove.l		#0x00000800,fpcr
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0x00000000,DATA+0x0(a6)
	move.l		#0x80000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)
	fmovem.x	DATA(a6),fp0

	move.w		#0x0000,ccr
unfl_0_pc:
	fdiv.b		#0x2,fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x00000000,IFPREGS+0x0(a6)
	move.l		#0x40000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x00000800,IFPCREGS+0x4(a6)
	lea			unfl_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

/* This test will take a non-maskable underflow directly. */
unfl_nm_str:
	.asciz		"\tNon-maskable underflow..."

	.even
	.balignw    4,0x51fc
unfl_nm_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0x00000000,DATA+0x0(a6)
	move.l		#0x80000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)
	fmovem.x	DATA(a6),fp0

	move.w		#0x0000,ccr
unfl_nm_0_pc:
	fdiv.b		#0x2,fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x00000000,IFPREGS+0x0(a6)
	move.l		#0x40000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x00000800,IFPCREGS+0x4(a6)
	lea			unfl_nm_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

inex_str:
	.asciz		"\tEnabled inexact..."

	.even
	.balignw    4,0x51fc
inex_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmove.l		#0x00000200,fpcr		/* enable inexact */
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0x50000000,DATA+0x0(a6)
	move.l		#0x80000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)
	fmovem.x	DATA(a6),fp0

	move.w		#0x0000,ccr
inex_0_pc:
	fadd.b		#0x2,fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x50000000,IFPREGS+0x0(a6)
	move.l		#0x80000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x00000208,IFPCREGS+0x4(a6)
	lea			inex_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

snan_str:
	.asciz		"\tEnabled SNAN..."

	.even
	.balignw    4,0x51fc
snan_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmove.l		#0x00004000,fpcr		/* enable SNAN */
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0xffff0000,DATA+0x0(a6)
	move.l		#0x00000000,DATA+0x4(a6)
	move.l		#0x00000001,DATA+0x8(a6)
	fmovem.x	DATA(a6),fp0

	move.w		#0x0000,ccr
snan_0_pc:
	fadd.b		#0x2,fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0xffff0000,IFPREGS+0x0(a6)
	move.l		#0x00000000,IFPREGS+0x4(a6)
	move.l		#0x00000001,IFPREGS+0x8(a6)
	move.l		#0x09004080,IFPCREGS+0x4(a6)
	lea			snan_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

operr_str:
	.asciz		"\tEnabled OPERR..."

	.even
	.balignw    4,0x51fc
operr_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmove.l		#0x00002000,fpcr		/* enable OPERR */
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0xffff0000,DATA+0x0(a6)
	move.l		#0x00000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)
	fmovem.x	DATA(a6),fp0

	move.w		#0x0000,ccr
operr_0_pc:
	fadd.s		#0x7f800000,fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0xffff0000,IFPREGS+0x0(a6)
	move.l		#0x00000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x01002080,IFPCREGS+0x4(a6)
	lea			operr_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

dz_str:
	.asciz		"\tEnabled DZ..."

	.even
	.balignw    4,0x51fc
dz_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmove.l		#0x00000400,fpcr		/* enable DZ */
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0x40000000,DATA+0x0(a6)
	move.l		#0x80000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)
	fmovem.x	DATA(a6),fp0

	move.w		#0x0000,ccr
dz_0_pc:
	fdiv.b		#0x0,fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x40000000,IFPREGS+0x0(a6)
	move.l		#0x80000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x02000410,IFPCREGS+0x4(a6)
	lea			dz_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

unsupp_str:
	.asciz		"\tUnimplemented data type/format..."
	.even
	.balignw    4,0x51fc

/* an unnormalized number */
unsupp_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0xc03f0000,DATA+0x0(a6)
	move.l		#0x00000000,DATA+0x4(a6)
	move.l		#0x00000001,DATA+0x8(a6)
	fmove.b		#0x2,fp0
	move.w		#0x0000,ccr
unsupp_0_pc:
	fmul.x		DATA(a6),fp0      /* test #1 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0xc0010000,IFPREGS+0x0(a6)
	move.l		#0x80000000,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x08000000,IFPCREGS+0x4(a6)
	lea			unsupp_0_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

/* a denormalized number */
unsupp_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0x80000000,DATA+0x0(a6)
	move.l		#0x01000000,DATA+0x4(a6)
	move.l		#0x00000000,DATA+0x8(a6)
	fmove.l		#0x7fffffff,fp0

	move.w		#0x0000,ccr
unsupp_1_pc:
	fmul.x		DATA(a6),fp0      /* test #2 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x80170000,IFPREGS+0x0(a6)
	move.l		#0xfffffffe,IFPREGS+0x4(a6)
	move.l		#0x00000000,IFPREGS+0x8(a6)
	move.l		#0x08000000,IFPCREGS+0x4(a6)
	lea			unsupp_1_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

/* packed */
unsupp_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	/* fmovem.x	(DEF_FPREGS).l(pc),fp0-fp7 */ /* not assembled by gas */
	.dc.w		0xf23b,0xd0ff,0x0170
	.dc.l		DEF_FPREGS-.+2
	fmovem.l	(DEF_FPCREGS).l(pc),fpcr/fpsr/fpiar

	move.w		#0x0000,ICCR(a6)
	movem.l		d0-d7/a0-a6,IREGS(a6)
	fmovem.x	fp0-fp7,IFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,IFPCREGS(a6)

	move.l		#0xc1230001,DATA+0x0(a6)
	move.l		#0x23456789,DATA+0x4(a6)
	move.l		#0x12345678,DATA+0x8(a6)

	move.w		#0x0000,ccr
unsupp_2_pc:
	fabs.p		DATA(a6),fp0      /* test #3 */

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	fmovem.x	fp0-fp7,SFPREGS(a6)
	fmovem.l	fpcr/fpsr/fpiar,SFPCREGS(a6)

	move.l		#0x3e660000,IFPREGS+0x0(a6)
	move.l		#0xd0ed23e8,IFPREGS+0x4(a6)
	move.l		#0xd14035bc,IFPREGS+0x8(a6)
	move.l		#0x00000108,IFPCREGS+0x4(a6)
	lea			unsupp_2_pc(pc),a0
	move.l		a0,IFPCREGS+0x8(a6)

	bsr.l		chkregs
	bsr.l		chkfpregs

	clr.l		d0
	rts

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */
/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

chkregs:
	lea			IREGS(a6),a0
	lea			SREGS(a6),a1
	moveq.l		#14,d0
chkregs_loop:
	cmp.l		(a1)+,(a0)+
	bne.l		chkregs_error
	dbra.w		d0,chkregs_loop

	move.w		ICCR(a6),d0
	move.w		SCCR(a6),d1
	cmp.w		d1,d0
	bne.l		chkregs_error
	rts

chkregs_error:
	addq.l		#0x1,ERRORCTR(a6)
	bsr         _real_debug_dregs
	rts

chkfpregs:
	lea			IFPREGS(a6),a0
	lea			SFPREGS(a6),a1
	moveq.l		#23,d0
chkfpregs_loop:
	cmp.l		(a1)+,(a0)+
	bne.l		chkfpregs_error
	dbra.w		d0,chkfpregs_loop

	lea			IFPCREGS(a6),a0
	lea			SFPCREGS(a6),a1
	cmp.l		(a1)+,(a0)+
	bne.l		chkfpregs_error
	cmp.l		(a1)+,(a0)+
	bne.l		chkfpregs_error
	cmp.l		(a1)+,(a0)+
	bne.l		chkfpregs_error
	rts

chkfpregs_error:
	addq.l		#0x1,ERRORCTR(a6)
	bsr         _real_debug_fregs
	rts

DEF_REGS:
	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac
	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac

	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac
	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac

DEF_FPREGS:
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff
	.dc.l		0x7fff0000, 0xffffffff, 0xffffffff

DEF_FPCREGS:
	.dc.l		0x00000000, 0x00000000, 0x00000000

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

_print_str:
	move.l		d0,-(sp)
	move.l		(TESTTOP-0x80+0x0,pc),d0
	pea			((TESTTOP-0x80).l,pc,d0.l)
	move.l		4(sp),d0
	rtd			#0x4

_print_num:
	move.l		d0,-(sp)
	move.l		(TESTTOP-0x80+0x4,pc),d0
	pea			((TESTTOP-0x80).l,pc,d0.l)
	move.l		4(sp),d0
	rtd			#0x4

_real_debug_dregs:
	move.l		d0,-(sp)
	move.l		(TESTTOP-0x80+0x8,pc),d0
	pea			((TESTTOP-0x80).l,pc,d0.l)
	move.l		4(sp),d0
	rtd			#0x4

_real_debug_fregs:
	move.l		d0,-(sp)
	move.l		(TESTTOP-0x80+0xc,pc),d0
	pea			((TESTTOP-0x80).l,pc,d0.l)
	move.l		4(sp),d0
	rtd			#0x4

/* ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; */

zero_fpiar_if_040:
	cmpi.w #40,CPUTYPE.w(pc)
	bne.s   zero_fpiar1
	suba.l  a0,a0
zero_fpiar1:
	rts
