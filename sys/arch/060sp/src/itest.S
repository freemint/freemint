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

/* ******************************************** */
SREGS = -64
IREGS = -128
SCCR = -130
ICCR = -132
TESTCTR = -136
EAMEM = -140
EASTORE = -144
DATA = -160

/* ******************************************** */
TESTTOP:
	bra.l		_060TESTS_

start_str:
	.asciz		"Testing 68060 ISP started:\n"

pass_str:
	.asciz		"passed\n"
fail_str:
	.asciz		" failed\n"
	.even

	/* .balignw    4,0x51fc */
chk_test:
	tst.l		d0
	bne.b		test_fail
test_pass:
	pea			pass_str(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp
	rts
test_fail:
	move.l		d1,-(sp)
	bsr.l		_print_num
	addq.l		#0x4,sp

	pea			fail_str(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp
	rts

/* ******************************************** */
_060TESTS_:
	link		a6,#-160

	movem.l		d2-d7/a2-a5,-(sp)

	pea			start_str(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

/* mul */
	clr.l		TESTCTR(a6)
	pea			(mulul_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		mulul_0

	bsr.l		chk_test

/* div */
	clr.l		TESTCTR(a6)
	pea			(divul_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		divul_0

	bsr.l		chk_test

/* cmp2 */
	clr.l		TESTCTR(a6)
	pea			(cmp2_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		cmp2_1

	bsr.l		chk_test

/* movep */
	clr.l		TESTCTR(a6)
	pea			(movep_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		movep_0

	bsr.l		chk_test

/* ea */
	clr.l		TESTCTR(a6)
	pea			(ea_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	move.l		#0x2,EAMEM(a6)
	bsr.l		ea_0

	bsr.l		chk_test

/* cas */
	clr.l		TESTCTR(a6)
	pea			(cas_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		cas0

	bsr.l		chk_test

/* cas2 */
	clr.l		TESTCTR(a6)
	pea			(cas2_str).l(pc)
	bsr.l		_print_str
	addq.l		#0x4,sp

	bsr.l		cas20

	bsr.l		chk_test

/* */
	movem.l		(sp)+,d2-d7/a2-a5

	unlk		a6
	rts

/* ******************************************** */
/* ******************************************** */

mulul_str:
	.asciz		"\t64-bit multiply..."
	.even

	/* .balignw    4,0x51fc */
	.dc.w 0x51fc
mulul_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d1
	move.l		#0x99999999,d2
	move.l		#0x88888888,d3

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	mulu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	clr.l		IREGS+0x8(a6)
	clr.l		IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x77777777,d1
	move.l		#0x99999999,d2
	moveq.l		#0x00000000,d3

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	mulu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	clr.l		IREGS+0x8(a6)
	clr.l		IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0x00000010,d1
	move.l		#0x66666666,d2

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	mulu.l		d1,d2:d2

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000006,IREGS+0x8(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_3:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x55555555,d1
	moveq.l		#0x00000000,d2
	moveq.l		#0x00000003,d3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	mulu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000000,IREGS+0x8(a6)
	move.l		#0xffffffff,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_4:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x40000000,d1
	moveq.l		#0x00000000,d2
	moveq.l		#0x00000004,d3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	mulu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000001,IREGS+0x8(a6)
	move.l		#0x00000000,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_5:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0xffffffff,d1
	moveq.l		#0x00000000,d2
	moveq.l		#0xffffffff,d3

	move.w		#0x0008,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	mulu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0xfffffffe,IREGS+0x8(a6)
	move.l		#0x00000001,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_6:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x80000000,d1
	moveq.l		#0x00000000,d2
	moveq.l		#0xffffffff,d3

	move.w		#0x00000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	muls.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000000,IREGS+0x8(a6)
	move.l		#0x80000000,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_7:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x80000000,d1
	moveq.l		#0x00000000,d2
	moveq.l		#0x00000001,d3

	move.w		#0x0008,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	muls.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0xffffffff,IREGS+0x8(a6)
	move.l		#0x80000000,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

mulul_8:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0x00000001,d1
	moveq.l		#0x00000000,d2
	move.l		#0x80000000,d3

	move.w		#0x0008,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	muls.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0xffffffff,IREGS+0x8(a6)
	move.l		#0x80000000,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	move.l		TESTCTR(a6),d1
	clr.l		d0
	rts

/* ******************************************** */

movep_str:
	.asciz	"\tmovep..."
	.even

	/* .balignw    4,0x51fc */
/*
 * ******************************
 * movep.w	d0,(0x0,a0)
 * ******************************
 */
movep_0:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.w	#0xaaaa,d0
	clr.b	0x0.w(a0)
	clr.b	0x2(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	d0,(0x0,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x2(a0),d1
	lsl.w	#0x8,d1
	move.b	0x0.w(a0),d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	d0,(0x0,a0)
 * ******************************
 */
movep_1:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA+0x4(a6),a0
	move.w	#0xaaaa,d0
	clr.l	-0x4(a0)
	clr.l	(a0)
	clr.l	0x4(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	d0,(0x0,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	tst.l	-0x4(a0)
	bne.l	error
	tst.l	0x4(a0)
	bne.l	error
	cmpi.l	#0xaa00aa00,(a0)
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ****************************************************
 * movep.w	d0,(0x0,a0)		TESTCTR(a6),
 * - this test has ccr initially equal to zero
 * ****************************************************
 */
movep_2:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.w	#0xaaaa,d0
	clr.b	0x0.w(a0)
	clr.b	0x2(a0)

	move.w	#0x0000,ICCR(a6)
	move.w	#0x0000,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	d0,(0x0,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x2(a0),d1
	lsl.w	#0x8,d1
	move.b	0x0.w(a0),d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	(0x0,a0),d0
 * ******************************
 */
movep_3:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.b	#0xaa,0x0.w(a0)
	move.b	#0xaa,0x2(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	(0x0,a0),d0

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)
	move.w	#0xaaaa,IREGS+0x2(a6)

	move.w	#0xaaaa,d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.l	d0,(0x0,a0)
 * ******************************
 */
movep_4:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.l	#0xaaaaaaaa,d0
	clr.b	0x0.w(a0)
	clr.b	0x2(a0)
	clr.b	0x4(a0)
	clr.b	0x6(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.l	d0,(0x0,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x6(a0),d1
	lsl.l	#0x8,d1
	move.b	0x4(a0),d1
	lsl.l	#0x8,d1
	move.b	0x2(a0),d1
	lsl.l	#0x8,d1
	move.b	0x0.w(a0),d1

	cmp.l	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.l	d0,(0x0,a0)
 * ******************************
 */
movep_5:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA+0x4(a6),a0
	move.l	#0xaaaaaaaa,d0
	clr.l	-0x4(a0)
	clr.l	(a0)
	clr.l	0x4(a0)
	clr.l	0x8(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.l	d0,(0x0,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	tst.l	-0x4(a0)
	bne.l	error
	tst.l	0x8(a0)
	bne.l	error
	cmpi.l	#0xaa00aa00,(a0)
	bne.l	error
	cmpi.l	#0xaa00aa00,0x4(a0)
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.l	(0x0,a0),d0
 * ******************************
 */
movep_6:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.b	#0xaa,0x0.w(a0)
	move.b	#0xaa,0x2(a0)
	move.b	#0xaa,0x4(a0)
	move.b	#0xaa,0x6(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.l	(0x0,a0),d0

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)
	move.l	#0xaaaaaaaa,IREGS(a6)

	move.l	#0xaaaaaaaa,d1

	cmp.l	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	d7,(0x0,a0)
 * ******************************
 */
movep_7:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.w	#0xaaaa,d7
	clr.b	0x0.w(a0)
	clr.b	0x2(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	d7,(0x0,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x2(a0),d1
	lsl.w	#0x8,d1
	move.b	0x0.w(a0),d1

	cmp.w	d1,d7
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	(0x0,a0),d7
 * ******************************
 */
movep_8:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.b	#0xaa,0x0.w(a0)
	move.b	#0xaa,0x2(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	(0x0,a0),d7

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)
	move.w	#0xaaaa,IREGS+30(a6)

	move.w	#0xaaaa,d1

	cmp.w	d1,d7
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	d0,(0x0,a0)
 * ******************************
 */
movep_9:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.w	#0xaaaa,d0
	clr.b	0x0.w(a0)
	clr.b	0x2(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x001f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	d0,(0x0,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x2(a0),d1
	lsl.w	#0x8,d1
	move.b	0x0.w(a0),d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	d0,(0x8,a0)
 * ******************************
 */
movep_10:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.w	#0xaaaa,d0
	clr.b	0x0+0x8(a0)
	clr.b	0x2+0x8(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	d0,(0x8,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x2+0x8(a0),d1
	lsl.w	#0x8,d1
	move.b	0x0+0x8(a0),d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	(0x8,a0),d0
 * ******************************
 */
movep_11:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.b	#0xaa,0x0+0x8(a0)
	move.b	#0xaa,0x2+0x8(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	(0x8,a0),d0

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)
	move.w	#0xaaaa,IREGS+0x2(a6)

	move.w	#0xaaaa,d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.l	d0,(0x8,a0)
 * ******************************
 */
movep_12:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.l	#0xaaaaaaaa,d0
	clr.b	0x0+0x8(a0)
	clr.b	0x2+0x8(a0)
	clr.b	0x4+0x8(a0)
	clr.b	0x6+0x8(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.l	d0,(0x8,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x6+0x8(a0),d1
	lsl.l	#0x8,d1
	move.b	0x4+0x8(a0),d1
	lsl.l	#0x8,d1
	move.b	0x2+0x8(a0),d1
	lsl.l	#0x8,d1
	move.b	0x0+0x8(a0),d1

	cmp.l	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.l	(0x8,a0),d0
 * ******************************
 */
movep_13:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA(a6),a0
	move.b	#0xaa,0x0+0x8(a0)
	move.b	#0xaa,0x2+0x8(a0)
	move.b	#0xaa,0x4+0x8(a0)
	move.b	#0xaa,0x6+0x8(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.l	(0x8,a0),d0

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)
	move.l	#0xaaaaaaaa,IREGS(a6)

	move.l	#0xaaaaaaaa,d1

	cmp.l	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	d0,(-0x8,a0)
 * ******************************
 */
movep_14:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA+0x8(a6),a0
	move.w	#0xaaaa,d0
	clr.b	0x0-0x8(a0)
	clr.b	0x2-0x8(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	d0,(-0x8,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x2-0x8(a0),d1
	lsl.w	#0x8,d1
	move.b	0x0-0x8(a0),d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.w	(-0x8,a0),d0
 * ******************************
 */
movep_15:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA+0x8(a6),a0
	move.b	#0xaa,0x0-0x8(a0)
	move.b	#0xaa,0x2-0x8(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.w	(-0x8,a0),d0

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)
	move.w	#0xaaaa,IREGS+0x2(a6)

	move.w	#0xaaaa,d1

	cmp.w	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.l	d0,(-0x8,a0)
 * ******************************
 */
movep_16:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA+0x8(a6),a0
	move.l	#0xaaaaaaaa,d0
	clr.b	0x0-0x8(a0)
	clr.b	0x2-0x8(a0)
	clr.b	0x4-0x8(a0)
	clr.b	0x8-0x8.w(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.l	d0,(-0x8,a0)

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)

	move.b	0x6-0x8(a0),d1
	lsl.l	#0x8,d1
	move.b	0x4-0x8(a0),d1
	lsl.l	#0x8,d1
	move.b	0x2-0x8(a0),d1
	lsl.l	#0x8,d1
	move.b	0x0-0x8(a0),d1

	cmp.l	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

/*
 * ******************************
 * movep.l	(-0x8,a0),d0
 * ******************************
 */
movep_17:
	addq.l	#0x1,TESTCTR(a6)

	movem.l	(DEF_REGS).l(pc),d0-d7/a0-a5

	lea	DATA+0x8(a6),a0
	move.b	#0xaa,0x0-0x8(a0)
	move.b	#0xaa,0x2-0x8(a0)
	move.b	#0xaa,0x4-0x8(a0)
	move.b	#0xaa,0x8-0x8.w(a0)

	move.w	#0x001f,ICCR(a6)
	move.w	#0x1f,ccr
	movem.l	d0-d7/a0-a6,IREGS(a6)

	movep.l	(-0x8,a0),d0

	move.w	ccr,SCCR(a6)
	movem.l	d0-d7/a0-a6,SREGS(a6)
	move.l	#0xaaaaaaaa,IREGS(a6)

	move.l	#0xaaaaaaaa,d1

	cmp.l	d1,d0
	bne.l	error

	bsr.l	chkregs
	tst.b	d0
	bne.l	error

	move.l	TESTCTR(a6),d1
	clr.l	d0
	rts

/* ******************************************************** */

divul_str:
	.asciz		"\t64-bit divide..."
	.even

	/* .balignw    4,0x51fc */
divul_0:
	addq.l		#0x1,TESTCTR(a6)

	.IFNE 0 /* would generate division by zero */
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d1
	move.l		#0x99999999,d2
	move.l		#0x88888888,d3

	move.w		#0x001e,ICCR(a6)
 	move.w		#0x001f,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error
	.ENDC

divul_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0x00000001,d1
	moveq.l		#0x00000000,d2
	moveq.l		#0x00000000,d3

	move.w		#0x0014,ICCR(a6)
	move.w		#0x001f,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x44444444,d1
	moveq.l		#0x00000000,d2
	move.l		#0x55555555,d3

	move.w		#0x0010,ICCR(a6)
	move.w		#0x001f,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x11111111,IREGS+0x8(a6)
	move.l		#0x00000001,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_3:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x55555555,d1
	moveq.l		#0x00000000,d2
	move.l		#0x44444444,d3

	move.w		#0x0014,ICCR(a6)
	move.w		#0x001f,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x44444444,IREGS+0x8(a6)
	move.l		#0x00000000,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_4:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x11111111,d1
	move.l		#0x44444444,d2
	move.l		#0x44444444,d3

	move.w		#0x001e,ICCR(a6)
	move.w		#0x001d,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_5:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0xfffffffe,d1
	moveq.l		#0x00000001,d2
	moveq.l		#0x00000002,d3

	move.w		#0x001e,ICCR(a6)
	move.w		#0x001d,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divs.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_6:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0xfffffffe,d1
	moveq.l		#0x00000001,d2
	moveq.l		#0x00000000,d3

	move.w		#0x0018,ICCR(a6)
	move.w		#0x001d,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divs.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000000,IREGS+0x8(a6)
	move.l		#0x80000000,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_7:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0x00000002,d1
	moveq.l		#0x00000001,d2
	moveq.l		#0x00000000,d3

	move.w		#0x001e,ICCR(a6)
	move.w		#0x001d,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divs.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_8:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0xffffffff,d1
	moveq.l		#0xfffffffe,d2
	moveq.l		#0xffffffff,d3

	move.w		#0x0008,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_9:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	moveq.l		#0xffffffff,d1
	moveq.l		#0xfffffffe,d2
	moveq.l		#0xffffffff,d3

	move.w		#0x0008,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		#0xffffffff,d2:d2

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0xffffffff,IREGS+0x8(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

divul_10:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x0000ffff,d1
	moveq.l		#0x00000001,d2
	move.l		#0x55555555,d3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	divu.l		d1,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x0000aaab,IREGS+0x8(a6)
	move.l		#0x00015556,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	move.l		TESTCTR(a6),d1
	clr.l		d0
	rts

/* ******************************************************** */

cas_str:
	.asciz		"\tcas..."
	.even

	/* .balignw    4,0x51fc */
cas0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea		DATA+0x1(a6),a0

	move.w		#0xaaaa,(a0)

	move.w		#0xaaaa,d1
	move.w		#0xbbbb,d2

	move.w		#0x0014,ICCR(a6)
	move.w		#0x0010,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas.w		d1,d2,(a0)			/* Dc,Du,<ea> */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d3
	move.w		#0xbbbb,IREGS+0xc+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea		DATA+0x1(a6),a0

	move.w		#0xeeee,(a0)

	move.w		#0x0000aaaa,d1
	move.w		#0x0000bbbb,d2

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas.w		d1,d2,(a0)			/* Dc,Du,<ea> */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d3
	move.w		#0xeeee,IREGS+0x4+0x2(a6)
	move.w		#0xeeee,IREGS+0xc+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea		DATA+0x2(a6),a0

	move.l		#0xaaaaaaaa,(a0)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas.l		d1,d2,(a0)			/* Dc,Du,<ea> */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d3
	move.l		#0xbbbbbbbb,IREGS+0xc(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas3:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x2(a6),a0

	move.l		#0xeeeeeeee,(a0)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas.l		d1,d2,(a0)			/* Dc,Du,<ea> */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d3
	move.l		#0xeeeeeeee,IREGS+0x4(a6)
	move.l		#0xeeeeeeee,IREGS+0xc(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas4:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0

	move.l		#0xaaaaaaaa,(a0)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas.l		d1,d2,(a0)			/* Dc,Du,<ea> */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d3
	move.l		#0xbbbbbbbb,IREGS+0xc(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas5:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0

	move.l		#0x7fffffff,(a0)

	move.l		#0x80000000,d1
	move.l		#0xbbbbbbbb,d2

	move.w		#0x001b,ICCR(a6)
	move.w		#0x0010,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas.l		d1,d2,(a0)			/* Dc,Du,<ea> */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d3
	move.l		#0x7fffffff,IREGS+0x4(a6)
	move.l		#0x7fffffff,IREGS+0xc(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	move.l		TESTCTR(a6),d1
	clr.l		d0
	rts

/* ******************************************************** */

cas2_str:
	.asciz		"\tcas2..."
	.even

	/* .balignw    4,0x51fc */
cas20:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x0(a6),a0
	lea			DATA+0x4(a6),a1

	move.l		#0xaaaaaaaa,(a0)
	move.l		#0xbbbbbbbb,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0014,ICCR(a6)
	move.w		#0x0010,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xcccccccc,IREGS+0x14(a6)
	move.l		#0xdddddddd,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas21:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0
	lea			DATA+0x5(a6),a1

	move.l		#0xaaaaaaaa,(a0)
	move.l		#0xbbbbbbbb,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0014,ICCR(a6)
	move.w		#0x0010,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xcccccccc,IREGS+0x14(a6)
	move.l		#0xdddddddd,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas22:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x2(a6),a0
	lea			DATA+0x6(a6),a1

	move.l		#0xaaaaaaaa,(a0)
	move.l		#0xbbbbbbbb,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0014,ICCR(a6)
	move.w		#0x0010,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xcccccccc,IREGS+0x14(a6)
	move.l		#0xdddddddd,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas23:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x0(a6),a0
	lea			DATA+0x4(a6),a1

	move.l		#0xeeeeeeee,(a0)
	move.l		#0xbbbbbbbb,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xeeeeeeee,IREGS+0x4(a6)
	move.l		#0xbbbbbbbb,IREGS+0x8(a6)
	move.l		#0xeeeeeeee,IREGS+0x14(a6)
	move.l		#0xbbbbbbbb,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas24:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0
	lea			DATA+0x5(a6),a1

	move.l		#0xeeeeeeee,(a0)
	move.l		#0xbbbbbbbb,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xeeeeeeee,IREGS+0x4(a6)
	move.l		#0xbbbbbbbb,IREGS+0x8(a6)
	move.l		#0xeeeeeeee,IREGS+0x14(a6)
	move.l		#0xbbbbbbbb,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas25:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x2(a6),a0
	lea			DATA+0x6(a6),a1

	move.l		#0xeeeeeeee,(a0)
	move.l		#0xbbbbbbbb,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xeeeeeeee,IREGS+0x4(a6)
	move.l		#0xbbbbbbbb,IREGS+0x8(a6)
	move.l		#0xeeeeeeee,IREGS+0x14(a6)
	move.l		#0xbbbbbbbb,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas26:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x0(a6),a0
	lea			DATA+0x4(a6),a1

	move.l		#0xaaaaaaaa,(a0)
	move.l		#0xeeeeeeee,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xaaaaaaaa,IREGS+0x4(a6)
	move.l		#0xeeeeeeee,IREGS+0x8(a6)
	move.l		#0xaaaaaaaa,IREGS+0x14(a6)
	move.l		#0xeeeeeeee,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas27:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0
	lea			DATA+0x5(a6),a1

	move.l		#0xaaaaaaaa,(a0)
	move.l		#0xeeeeeeee,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0xbbbbbbbb,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xaaaaaaaa,IREGS+0x4(a6)
	move.l		#0xeeeeeeee,IREGS+0x8(a6)
	move.l		#0xaaaaaaaa,IREGS+0x14(a6)
	move.l		#0xeeeeeeee,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas28:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x2(a6),a0
	lea			DATA+0x6(a6),a1

	move.l		#0xaaaaaaaa,(a0)
	move.l		#0x7fffffff,(a1)

	move.l		#0xaaaaaaaa,d1
	move.l		#0x80000000,d2
	move.l		#0xcccccccc,d3
	move.l		#0xdddddddd,d4

	move.w		#0x000b,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.l		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.l		(a0),d5
	move.l		(a1),d6
	move.l		#0xaaaaaaaa,IREGS+0x4(a6)
	move.l		#0x7fffffff,IREGS+0x8(a6)
	move.l		#0xaaaaaaaa,IREGS+0x14(a6)
	move.l		#0x7fffffff,IREGS+0x18(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

/* ****************************** */
cas29:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x0(a6),a0
	lea			DATA+0x4(a6),a1

	move.w		#0xaaaa,(a0)
	move.w		#0xbbbb,(a1)

	move.w		#0xaaaa,d1
	move.w		#0xbbbb,d2
	move.w		#0xcccc,d3
	move.w		#0xdddd,d4

	move.w		#0x0014,ICCR(a6)
	move.w		#0x0010,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.w		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d5
	move.w		(a1),d6
	move.w		#0xcccc,IREGS+0x14+0x2(a6)
	move.w		#0xdddd,IREGS+0x18+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas210:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0
	lea			DATA+0x5(a6),a1

	move.w		#0xaaaa,(a0)
	move.w		#0xbbbb,(a1)

	move.w		#0xaaaa,d1
	move.w		#0xbbbb,d2
	move.w		#0xcccc,d3
	move.w		#0xdddd,d4

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.w		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d5
	move.w		(a1),d6
	move.w		#0xcccc,IREGS+0x14+0x2(a6)
	move.w		#0xdddd,IREGS+0x18+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas211:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x0(a6),a0
	lea			DATA+0x4(a6),a1

	move.w		#0xeeee,(a0)
	move.w		#0xbbbb,(a1)

	move.w		#0xaaaa,d1
	move.w		#0xbbbb,d2
	move.w		#0xcccc,d3
	move.w		#0xdddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.w		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d5
	move.w		(a1),d6
	move.w		#0xeeee,IREGS+0x4+0x2(a6)
	move.w		#0xbbbb,IREGS+0x8+0x2(a6)
	move.w		#0xeeee,IREGS+0x14+0x2(a6)
	move.w		#0xbbbb,IREGS+0x18+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas212:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0
	lea			DATA+0x5(a6),a1

	move.w		#0xeeee,(a0)
	move.w		#0xbbbb,(a1)

	move.w		#0xaaaa,d1
	move.w		#0xbbbb,d2
	move.w		#0xcccc,d3
	move.w		#0xdddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.w		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d5
	move.w		(a1),d6
	move.w		#0xeeee,IREGS+0x4+0x2(a6)
	move.w		#0xbbbb,IREGS+0x8+0x2(a6)
	move.w		#0xeeee,IREGS+0x14+0x2(a6)
	move.w		#0xbbbb,IREGS+0x18+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas213:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x0(a6),a0
	lea			DATA+0x4(a6),a1

	move.w		#0xaaaa,(a0)
	move.w		#0xeeee,(a1)

	move.w		#0xaaaa,d1
	move.w		#0xbbbb,d2
	move.w		#0xcccc,d3
	move.w		#0xdddd,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.w		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d5
	move.w		(a1),d6
	move.w		#0xaaaa,IREGS+0x4+0x2(a6)
	move.w		#0xeeee,IREGS+0x8+0x2(a6)
	move.w		#0xaaaa,IREGS+0x14+0x2(a6)
	move.w		#0xeeee,IREGS+0x18+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cas214:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	lea			DATA+0x1(a6),a0
	lea			DATA+0x5(a6),a1

	move.w		#0xaaaa,(a0)
	move.w		#0x7fff,(a1)

	move.w		#0xaaaa,d1
	move.w		#0x8000,d2
	move.w		#0xcccc,d3
	move.w		#0xdddd,d4

	move.w		#0x001b,ICCR(a6)
	move.w		#0x0010,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cas2.w		d1:d2,d3:d4,(a0):(a1)	/* Dc1:Dc2,Du1:Du2,(Rn1):(Rn2) */

	move.w		ccr,SCCR(a6)
	move.w		(a0),d5
	move.w		(a1),d6
	move.w		#0xaaaa,IREGS+0x4+0x2(a6)
	move.w		#0x7fff,IREGS+0x8+0x2(a6)
	move.w		#0xaaaa,IREGS+0x14+0x2(a6)
	move.w		#0x7fff,IREGS+0x18+0x2(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	move.l		TESTCTR(a6),d1
	clr.l		d0
	rts

/* ******************************************************** */

cmp2_str:
	.asciz		"\tcmp2,chk2..."
	.even

	/* .balignw    4,0x51fc */
	.dc.w 0x51fc
/*  unsigned - small,small */
cmp2_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0x2040,DATA(a6)
	move.l		#0x11111120,d1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0x2040,DATA(a6)
	move.l		#0x00000040,a1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_3:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0x2040,DATA(a6)
	move.l		#0x11111130,d1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	chk2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_4:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0x2040,DATA(a6)
	move.l		#0x00000010,a1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_5:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0x2040,DATA(a6)
	move.l		#0x11111150,d1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_6:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0x2040,DATA(a6)
	move.l		#0x00000090,a1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

/*  unsigned - small,large */
cmp2_7:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x2000a000,DATA(a6)
	move.l		#0x11112000,d1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.w		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_8:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x2000a000,DATA(a6)
	move.l		#0xffffa000,a1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.w		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_9:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x2000a000,DATA(a6)
	move.l		#0x11113000,d1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	chk2.w		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_10:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x2000a000,DATA(a6)
	move.l		#0xffff9000,a1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.w		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_11:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x2000a000,DATA(a6)
	move.l		#0x11111000,d1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.w		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_12:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0x2000a000,DATA(a6)
	move.l		#0xffffb000,a1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.w		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

/*  unsigned - large,large */
cmp2_13:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0xa0000000,DATA(a6)
	move.l		#0xc0000000,DATA+0x4(a6)
	move.l		#0xa0000000,d1

	move.w		#0x000c,ICCR(a6)
	move.w		#0x0008,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.l		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_14:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0xa0000000,DATA(a6)
	move.l		#0xc0000000,DATA+0x4(a6)
	move.l		#0xc0000000,a1

	move.w		#0x000c,ICCR(a6)
	move.w		#0x0008,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.l		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_15:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0xa0000000,DATA(a6)
	move.l		#0xc0000000,DATA+0x4(a6)
	move.l		#0xb0000000,d1

	move.w		#0x0008,ICCR(a6)
	move.w		#0x0008,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	chk2.l		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_16:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0xa0000000,DATA(a6)
	move.l		#0xc0000000,DATA+0x4(a6)
	move.l		#0x10000000,a1

	move.w		#0x0009,ICCR(a6)
	move.w		#0x0008,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.l		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_17:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0xa0000000,DATA(a6)
	move.l		#0xc0000000,DATA+0x4(a6)
	move.l		#0x90000000,d1

	move.w		#0x0009,ICCR(a6)
	move.w		#0x0008,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.l		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_18:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		#0xa0000000,DATA(a6)
	move.l		#0xc0000000,DATA+0x4(a6)
	move.l		#0xd0000000,a1

	move.w		#0x0009,ICCR(a6)
	move.w		#0x0008,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.l		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

/*  signed - negative,positive */
cmp2_19:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa040,DATA(a6)
	move.l		#0x111111a0,d1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_20:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa040,DATA(a6)
	move.l		#0x00000040,a1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	chk2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_21:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa040,DATA(a6)
	move.l		#0x111111b0,d1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_22:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa040,DATA(a6)
	move.l		#0x00000010,a1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_23:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa040,DATA(a6)
	move.l		#0x11111190,d1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_24:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa040,DATA(a6)
	move.l		#0x00000050,a1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

/*  signed - negative,negative */
cmp2_25:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa0c0,DATA(a6)
	move.l		#0x111111a0,d1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_26:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa0c0,DATA(a6)
	move.l		#0xffffffc0,a1

	move.w		#0x0004,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_27:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa0c0,DATA(a6)
	move.l		#0x111111b0,d1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	chk2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_28:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa0c0,DATA(a6)
	move.l		#0x11111190,a1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_29:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa0c0,DATA(a6)
	move.l		#0x111111d0,d1

	move.w		#0x0001,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),d1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

cmp2_30:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.w		#0xa0c0,DATA(a6)
	move.l		#0x00000050,a1

	move.w		#0x001b,ICCR(a6)
	move.w		#0x001f,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	cmp2.b		DATA(a6),a1

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	move.l		TESTCTR(a6),d1
	clr.l		d0
	rts

/* ******************************************************** */

ea_str:
	.asciz		"\tEffective addresses..."
	.even

	/* .balignw    4,0x51fc */
ea_0:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a0),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_1:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a0)+,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM+0x4(a6),a0
	move.l		a0,IREGS+0x20(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_2:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x4(a6),a0

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		-(a0),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM(a6),a0
	move.l		a0,IREGS+0x20(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_3:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x1000(a6),a0

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(-0x1000,a0),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_4:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a0

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x1000,a0),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_5:
	addq.l		#0x1,TESTCTR(a6)

	.IFNE 0 /* no portable way to use an absolute address */
 	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		EAMEM.w,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
 	bne.l		error
	.ENDC

ea_6:
	addq.l		#0x1,TESTCTR(a6)

	.IFNE 0 /* no portable way to use an absolute address */
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3

	move.w		#0x0000,ICCR(a6)
 	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		EAMEM.l,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
 	tst.b		d0
 	bne.l		error
	.ENDC


ea_7:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		#0x00000002,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_8:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_8_next
ea_8_mem:
	.dc.l		0x00000002
ea_8_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_8_mem.w,pc),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_9:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x4(a6),a1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		-(a1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM(a6),a0
	move.l		a0,IREGS+0x24(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_10:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x4(a6),a2

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		-(a2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM(a6),a0
	move.l		a0,IREGS+0x28(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_11:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x4(a6),a3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		-(a3),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM(a6),a0
	move.l		a0,IREGS+0x2c(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_12:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x4(a6),a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		-(a4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM(a6),a0
	move.l		a0,IREGS+0x30(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_13:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x4(a6),a5

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		-(a5),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM(a6),a0
	move.l		a0,IREGS+0x34(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_14:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x4(a1),a6

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		-(a6),d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)
	lea			EAMEM(a1),a0
	move.l		a0,IREGS+0x38(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	.ifne 1 /* crashes Hatari before commit 2fdef019fbcfcb6d838786692d17a2d32e7871a3 */
ea_15:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5
	clr.l		d2
	moveq.l		#0x00000002,d3
	move.l		a7,a0
	lea			EAMEM+0x4(a6),a7

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		-(a7),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM(a6),a1
	move.l		a1,IREGS+0x3c(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error
	.ENDC

ea_16:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_17:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.w*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_18:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.w*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_19:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.w*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_20:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.l*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_21:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.l*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_22:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.l*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_23:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,d4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_24:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	move.l		#-0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a0,a4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_25:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	move.l		#0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(-0x10.b,a0,a4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_26:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a1
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a1,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_27:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a2
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a2,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_28:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a3,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_29:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a4
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a4,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_30:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a5
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a5,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_31:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a1),a6
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		(0x10.b,a6,d4.w*1),d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_32:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	move.l		a7,a0
	lea			EAMEM(a6),a7
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x10.b,a7,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_33:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_34:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a2

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_35:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a3),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_36:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_37:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a5

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a5),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_38:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a1),a6

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		(a6),d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_39:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	move.l		a7,a0
	lea			EAMEM(a6),a7

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a7),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_40:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a1)+,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM+0x4(a6),a0
	move.l		a0,IREGS+0x24(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_41:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a2

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a2)+,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM+0x4(a6),a0
	move.l		a0,IREGS+0x28(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_42:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a3)+,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM+0x4(a6),a0
	move.l		a0,IREGS+0x2c(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_43:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a4)+,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM+0x4(a6),a0
	move.l		a0,IREGS+0x30(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_44:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a5

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a5)+,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM+0x4(a6),a0
	move.l		a0,IREGS+0x34(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_45:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a1),a6

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		(a6)+,d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)
	lea			EAMEM+0x4(a1),a0
	move.l		a0,IREGS+0x38(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	.ifne 1 /* crashes Hatari before commit 2fdef019fbcfcb6d838786692d17a2d32e7871a3 */
ea_46:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	move.l		a7,a0
	lea			EAMEM(a6),a7

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(a7)+,d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)
	lea			EAMEM+0x4(a6),a1
	move.l		a1,IREGS+0x3c(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error
	.endc

ea_47:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a1

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x1000,a1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_48:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a2

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x1000,a2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_49:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x1000,a3),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_50:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x1000,a4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_51:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a5

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x1000,a5),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_52:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a1),a6

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		(0x1000,a6),d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_53:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	move.l		a7,a0
	lea			EAMEM-0x1000(a6),a7

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(0x1000,a7),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_54:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x1000(a6),a0

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(-0x1000,a0),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_55:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_55_next

ea_55_data:
	.dc.l		0x00000002
ea_55_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_55_data.w,pc),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_56:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.w*1),d2:d3 ; ZZZ -> translated to: */
	mulu.l		0x10(a3,d4.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_57:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.w*2),d2:d3 ; ZZZ -> translated to: */
	mulu.l		0x10(a3,d4.w*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_58:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.w*4),d2:d3 ; ZZZ -> translated to: */
	mulu.l		0x10(a3,d4.w*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_59:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.w*8),d2:d3 ; ZZZ -> translated to: */
	mulu.l		0x10(a3,d4.w*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_60:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.l*1),d2:d3 */
	mulu.l		0x10(a3,d4.l*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_61:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.l*2),d2:d3 */
	mulu.l		0x10(a3,d4.l*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_62:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.l*4),d2:d3 */
	mulu.l		0x10(a3,d4.l*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_63:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x10.w,a3,d4.l*8),d2:d3 */
	mulu.l		0x10(a3,d4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_64:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	move.l		#0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(-0x10.w,a3,a4.l*8),d2:d3 */
	mulu.l		-0x10(a3,a4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_65:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	move.l		#0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(0x00.w,a3,za4.l*8),d2:d3 */
	/* mulu.l	(a3,zd0.w*8),d2:d3 */
	.dc.w 0x4c33,0x3402,0x0750

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_66:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	move.l		a3,a4
	add.l		#0x10,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		(-0x10.w,za3,a4.l*1),d2:d3 */
	mulu.l		(-0x10.w,za0,a4.l*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_67:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	move.l		#0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(-0x10.l,a3,a4.l*8),d2:d3 */
	mulu.l		-0x10(a3,a4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_68:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_68_next
ea_68_mem:
	.dc.l		0x00000002
ea_68_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(ea_68_mem+0x10.w,pc,d4.w*1),d2:d3 */
	mulu.l		ea_68_mem+0x10(pc,d4.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_69:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_69_next
ea_69_mem:
	.dc.l		0x00000002
ea_69_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	/* mulu.l	(ea_69_mem+0x10.w,pc,d4.w*2),d2:d3 */
	mulu.l		ea_69_mem+0x10(pc,d4.w*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_70:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_70_next
ea_70_mem:
	.dc.l		0x00000002
ea_70_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	/* mulu.l	(ea_70_mem+0x10.w,pc,d4.w*4),d2:d3 */
	mulu.l		ea_70_mem+0x10(pc,d4.w*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_71:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_71_next
ea_71_mem:
	.dc.l		0x00000002
ea_71_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	/* mulu.l	(ea_71_mem+0x10.w,pc,d4.w*8),d2:d3 */
	mulu.l		ea_71_mem+0x10(pc,d4.w*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_72:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_72_next
ea_72_mem:
	.dc.l		0x00000002
ea_72_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	/* mulu.l	(ea_72_mem+0x10.w,pc,d4.l*1),d2:d3 */
	mulu.l		ea_72_mem+0x10(pc,d4.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_73:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_73_next
ea_73_mem:
	.dc.l		0x00000002
ea_73_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	/* mulu.l	(ea_73_mem+0x10.w,pc,d4.l*2),d2:d3 */
	mulu.l		ea_73_mem+0x10(pc,d4.l*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_74:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_74_next
ea_74_mem:
	.dc.l		0x00000002
ea_74_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	/* mulu.l	(ea_74_mem+0x10.w,pc,d4.l*4),d2:d3 */
	mulu.l		ea_74_mem+0x10(pc,d4.l*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_75:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_75_next
ea_75_mem:
	.dc.l		0x00000002
ea_75_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a6,IREGS(a6)

	/* mulu.l	(ea_75_mem+0x10.w,pc,d4.l*8),d2:d3 */
	mulu.l		ea_75_mem+0x10(pc,d4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a6,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_76:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_76_next
ea_76_mem:
	.dc.l		0x00000002
ea_76_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	move.l		#-0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(ea_76_mem+0x10.w,pc,a4.l*8),d2:d3 */
	mulu.l		ea_76_mem+0x10(pc,a4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_77:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_77_next
ea_77_mem:
	.dc.l		0x00000002
ea_77_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	move.l		#0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	(ea_77_mem+0x00.w,pc,za4.l*8),d2:d3 */
	/* mulu.l	ea_77_mem+0x00(pc,zd0.w*8),d2:d3 */
	.dc.w 0x4c3b,0x3402,0x0760,ea_77_mem-.+2

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_78:
	addq.l		#0x1,TESTCTR(a6)

	.IFNE 0 /* no portable way to use an absolute address */
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM,a3
	move.l		a3,a4
	add.l		#0x10,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(EAMEM-0x10.w,zpc,a4.l*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error
	.ENDC


ea_79:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM.l,a3
	move.l		#0x2,a4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l		(ea_79_mem-0x10.l,pc,a4.l*8),d2:d3 */
	mulu.l		ea_79_mem-0x10.l(pc,a4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bra.b		ea_79_next
ea_79_mem:
	.dc.l		0x00000002
ea_79_next:

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_80:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_80_next
ea_80_mem:
	.dc.l		0x00000002
ea_80_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a1
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_80_mem+0x10.b,pc,d4.w*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_81:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_81_next
ea_81_mem:
	.dc.l		0x00000002
ea_81_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_81_mem+0x10.b,pc,d4.w*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_82:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_82_next
ea_82_mem:
	.dc.l		0x00000002
ea_82_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_82_mem+0x10.b,pc,d4.w*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_83:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_83_next
ea_83_mem:
	.dc.l		0x00000002
ea_83_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_83_mem+0x10.b,pc,d4.w*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_84:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_84_next
ea_84_mem:
	.dc.l		0x00000002
ea_84_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_84_mem+0x10.b,pc,d4.l*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_85:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_85_next
ea_85_mem:
	.dc.l		0x00000002
ea_85_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_85_mem+0x10.b,pc,d4.l*2),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_86:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_86_next
ea_86_mem:
	.dc.l		0x00000002
ea_86_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_86_mem+0x10.b,pc,d4.l*4),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_87:
	addq.l		#0x1,TESTCTR(a6)

	bra.b		ea_87_next
ea_87_mem:
	.dc.l		0x00000002
ea_87_next:
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_87_mem+0x10.b,pc,d4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_88:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a0
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		(ea_88_mem+0x10.b,pc,d4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bra.b		ea_88_next
ea_88_mem:
	.dc.l		0x00000002
ea_88_next:

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_89:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.w*1],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_90:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.w*2],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_91:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.w*4],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_92:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.w*8],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_93:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.l*1],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_94:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.l*2],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_95:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.l*4],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_96:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4,d4.l*8],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_97:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.l,a4,d4.l*8],0x1000.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_98:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	([0x00.l,a4,zd4.l*8],0x1000.l),d2:d3 */
	/* mulu.l	([a4,zd0.w*8],0x1000.l),d2:d3 */
	.dc.w 0x4c34,0x3402,0x0753,0x0000,0x1000

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_99:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([a4,zd4.l*8],0x1000.l),d2:d3 */
/*	mulu.l		([a4,zd0.w*8],0x1000.l),d2:d3 */
	.dc.w		0x4c34,0x3402,0x0753,0x0000,0x1000

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_100:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#-0x10,d4
	add.l		a4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([0x10.l,za4,d4.l*1],0x1000.l),d2:d3 */
	mulu.l		([0x10.l,za0,d4.l*1],0x1000.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_101:
	addq.l		#0x1,TESTCTR(a6)

	.IFNE 0 /* no portable way to use an absolute address */
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
 	moveq.l		#-0x10,d4

 	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.l,za4,zd4.l*1]),d2:d3

	move.w		ccr,SCCR(a6)
 	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error
	.ENDC


ea_102:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x1000(a1),a3
	lea			EASTORE(a1),a4
	move.l		a3,(a4)
	move.l		#-0x2,a6

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		([0x10.w,a4,a6.l*8],-0x1000.w),d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_103:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x1000(a1),a3
	lea			EASTORE(a1),a4
	move.l		a3,(a4)
	move.l		#0x2,a6

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		([-0x10.w,a4,a6.l*8],-0x1000.w),d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_104:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.w*1,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_105:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.w*2,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_106:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.w*4,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_107:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.w*8,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_108:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.l*1,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_109:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.w*2,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_110:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.l*4,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_111:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.w,a4],d4.l*8,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_112:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([0x10.l,a4],d4.l*8,0x10.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_113:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a4
	move.l		a3,(a4)
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	/* mulu.l	([0x00.l,a4],zd4.l*8,0x20.l),d2:d3 */
	/* mulu.l	([a4,zd0.w*8],0x20.l),d2:d3 */
	.dc.w		0x4c34,0x3402,0x0753,0x0000,0x0020
	
	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_114:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a7,a0
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(a6),a3
	lea			EASTORE(a6),a7
	move.l		a3,(a7)
	moveq.l		#0x20,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([a7],d4.l*1),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_115:
	addq.l		#0x1,TESTCTR(a6)

	.IFNE 0 /* no portable way to use an absolute address */
	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
 	move.l		a3,(a4)
 	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.l,za4],zd4.l*8,0x20.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error
	.ENDC


ea_116:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a6,a1

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(a1),a3
	lea			EASTORE(a1),a6
	move.l		a3,(a6)
	add.l		#0x10,a6
	move.l		#-0x2,a5

	move.w		#0x0000,ICCR(a1)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a1)

	mulu.l		([-0x10.w,a6],a5.l*8,0x10.l),d2:d3

	move.w		ccr,SCCR(a1)
	movem.l		d0-d7/a0-a7,SREGS(a1)
	move.l		#0x00000004,IREGS+0xc(a1)

	move.l		a1,a6

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	move.l		TESTCTR(a6),d1
	clr.l		d0
	rts

ea_117:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.w*1],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_118:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.w*2],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_119:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.w*4],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_120:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.w*8],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_121:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.l*1],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_122:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.l*2],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_123:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.l*4],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_124:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE+0x10.w,pc,d4.l*8],0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_125:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)
	mulu.l		([EASTORE+0x10.l,pc,d4.l*8],0x1000.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_126:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([EASTORE+0x00.l,pc,zd4.l*8],0x1000.l),d2:d3 */
/*	mulu.l		([EASTORE+0x00.l,pc,zd0.w*8],0x1000.l),d2:d3 */
	.dc.w		0x4c3b,0x3402,0x0773
	.dc.l		EASTORE
	.dc.w		0x0000,0x1000

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_127:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	move.l		a4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([zpc,d4.l*1],0x1000.l),d2:d3 */
	mulu.l		([za0,d4.l*1],0x1000.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_128:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x10,d4
	add.l		a4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([0x10.l,zpc,d4.l*1],0x1000.l),d2:d3 */
	mulu.l		([0x10.l,za0,d4.l*1],0x1000.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_129:
	addq.l		#0x1,TESTCTR(a6)

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#-0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([EASTORE.l,zpc,zd4.l*1]),d2:d3 */
	mulu.l		([EASTORE.l,za0,zd0.w*1]),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_130:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	move.l		#0x2,a6

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE-0x10.w,pc,a6.l*8],-0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_131:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a7,a0
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM+0x1000(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	move.l		#0x2,a7

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE-0x10.w,pc,a7.l*8],-0x1000.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_132:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.w*1,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_133:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.w*2,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_134:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.w*4,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_135:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.w*8,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_136:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x10,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.l*1,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_137:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x8,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.w*2,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_138:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.l*4,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_139:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],d4.l*8,0x10.w),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_140:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	sub.l		#0x10,a4
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.l,pc],d4.l*8,0x10.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_141:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#0x2,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([EASTORE.l,pc],zd4.l*8,0x20.l),d2:d3 */
/*	mulu.l		([EASTORE.l,pc,zd0.w*8],0x20.l),d2:d3 */
	.dc.w		0x4c3b,0x3402,0x0773
	.dc.l		EASTORE
	.dc.w		0x0000,0x0020

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_142:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM-0x20(pc),a3
	lea			EASTORE(pc),a4
	move.l		a3,(a4)
	moveq.l		#0x4,d4

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

/* 	mulu.l		([EASTORE.l,zpc],d4.l*8),d2:d3 */
	mulu.l		([EASTORE.l,za0],d4.l*8),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	bsr.l		chkregs
	tst.b		d0
	bne.l		error

ea_143:
	addq.l		#0x1,TESTCTR(a6)

	movem.l		(DEF_REGS).l(pc),d0-d7/a0-a5

	move.l		a7,a0
	clr.l		d2
	moveq.l		#0x00000002,d3
	lea			EAMEM(pc),a3
	lea			EASTORE(pc),a6
	move.l		a3,(a6)
	add.l		#0x10,a6
	move.l		#-0x2,a7

	move.w		#0x0000,ICCR(a6)
	move.w		#0x0000,ccr
	movem.l		d0-d7/a0-a7,IREGS(a6)

	mulu.l		([EASTORE.w,pc],a7.l*8,0x10.l),d2:d3

	move.w		ccr,SCCR(a6)
	movem.l		d0-d7/a0-a7,SREGS(a6)
	move.l		#0x00000004,IREGS+0xc(a6)

	move.l		a0,a7
	bsr.l		chkregs
	tst.b		d0
	bne.l		error

	clr.l		d0
	rts

/* ******************************************************** */
/* ******************************************************** */
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

	clr.l		d0
	rts

chkregs_error:
	moveq.l		#0x1,d0
	rts

error:
	move.l		TESTCTR(a6),d1
	moveq.l		#0x1,d0
	rts

DEF_REGS:
	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac
	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac

	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac
	.dc.l		0xacacacac, 0xacacacac, 0xacacacac, 0xacacacac

/* ******************************************************** */

_print_str:
	move.l		d0,-(sp)
	move.l		(TESTTOP-0x80+0x0,pc),d0
	pea			((TESTTOP-0x80).l,pc,d0.l)
	move.l		4(sp),d0
	rtd			#4

_print_num:
	move.l		d0,-(sp)
	move.l		(TESTTOP-0x80+0x4,pc),d0
	pea			((TESTTOP-0x80).l,pc,d0.l)
	move.l		0x4(sp),d0
	rtd			#4

	/* .balignw    16,0 */
