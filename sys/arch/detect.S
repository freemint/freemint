/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Author: Jrg Westheide <joerg_westheide@su.maus.de>
 * Started: 1999-06-11
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 *
 * Hardware detection for MiNT.
 * This code _must_ be compiled as -m68030
 *
 * Detection of Harun Scheutzows ST-ESCC (Joerg)
 * Detection of CPU and FPU model (Draco)
 *
 */

#include "mint/asmdefs.h"

#ifndef __mcoldfire__
	.cpu	68030
	.arch	68030
#endif

// Test hardware registers for presence

	.globl	SYM(test_byte_rd)
	.globl	SYM(test_word_rd)
	.globl	SYM(test_long_rd)

	FUNC(test_byte_rd)
SYM(test_byte_rd):
	move.l	a2,-(sp)
	move.l	sp,a0
	move.l	(8).w,a1
	lea	berr.w(pc),a2
	move.l	a2,(8).w
	nop

	clr.l	d0
	move.l	8(sp),a2
	tst.b	(a2)

pass:	moveq	#0x01,d0	// passed

berr:	move.l	a1,(8).w
	move.l	a0,sp
	nop
	move.l	(sp)+,a2
	rts
	END(test_byte_rd)

	
	FUNC(test_word_rd)
SYM(test_word_rd):
	move.l	a2,-(sp)
	move.l	sp,a0
	move.l	(8).w,a1
	lea	berr.w(pc),a2
	move.l	a2,(8).w
	nop

	clr.l	d0
	move.l	8(sp),a2
	tst.w	(a2)

	bra.s	pass
	END(test_word_rd)

	FUNC(test_long_rd)
SYM(test_long_rd):
	move.l	a2,-(sp)
	move.l	sp,a0
	move.l	(8).w,a1
	lea	berr.w(pc),a2
	move.l	a2,(8).w
	nop

	clr.l	d0
	move.l	8(sp),a2
	tst.l	(a2)

	bra.s	pass
	END(test_long_rd)

	.globl	SYM(detect_hardware)

	.text

//
// This function detects peripheral hardware components and returns a
// bit vector of the found components. A set bit indicates, that a
// component was found. The meaning of the bits are:
//
//   bit  component
// --------------------
//    0   ST-ESCC
//  1-31  reserved (=0)
//

	.text

	FUNC(detect_hardware)
SYM(detect_hardware):

#ifdef __mcoldfire__
	move.l	d2,-(sp)
#endif
	moveq	#0,d0				// initialize return value

// *** detect ST-ESCC (Harun Scheutzow's extension to normal ST(E)s providing modem2 and serial2)

	move.l	sp,a1				// save stack pointer
	move.l	(8).w,a0			// save bus error vector
#ifdef __mcoldfire__
	move.l	#no_stescc,d2
	move.l	d2,(8).w			// set temporary new bus error vector
#else
	move.l	#no_stescc,(8).w		// set temporary new bus error vector
#endif
	nop					// flush write pipeline (>= 040)
	move.w	sr,d1

// it was ori.w #0700,SR, but i don't think this was really meant so.
// 700 dec is 0x02bc hex, so it really raised the IPL, but MFP
// interrupts were't disabled (they're IPL 5 IIRC).

#ifdef __mcoldfire__
	move.w	sr,d2
	ori.l	#0x0700,d2			// disable interrupts
	move.w	d2,sr
#else
	ori.w	#0x0700,sr			// disable interrupts
#endif

// The next instruction will cause a bus error if there is no
// ST-compatible MFP, which makes the odd addresses 0xFFFFFA01 -
// 0xFFFFFA3F accessible and is neccessary for the ST-ESCC extension

#ifdef __mcoldfire__
	moveq	#3,d2
	move.b	d2,(0xFFFFFA31).w		// select register 3, channel B
#else
	move.b	#3,(0xFFFFFA31).w		// select register 3, channel B
#endif

// If we reach the following instruction we have a ST-compatible MFP.
// If there is an ST-ESCC we have already selected register 3 of channel B,
// otherwise we wrote the #3 to "nowhere" and will read a 0xFF from there

	tst.b	(0xFFFFFA31).w			// register is always 0 if ST-ESCC present
	bne.s	no_stescc			// no -> no ST-ESCC present
#ifdef __mcoldfire__
	move.w	#0xFA35,d2
	move.w	d2,SYM(ControlRegA)+2		// set register adresses to ST-ESCC's
	move.w	#0xFA37,d2
	move.w	d2,SYM(DataRegA)+2
	move.w	#0xFA31,d2
	move.w	d2,SYM(ControlRegB)+2
	move.w	#0xFA33,d2
	move.w	d2,SYM(DataRegB)+2
#else
	move.w	#0xFA35,SYM(ControlRegA)+2		// set register adresses to ST-ESCC's
	move.w	#0xFA37,SYM(DataRegA)+2
	move.w	#0xFA31,SYM(ControlRegB)+2
	move.w	#0xFA33,SYM(DataRegB)+2
#endif
	bset	#0x00,d0			// mark ST-ESCC present
no_stescc:					// bus error occured or reg 3 != 0 -> ST-ESCC not present
	move.w	d1,sr				// enable interrupts
	move.l	a0,(8).w			// restore  bus error vector
	nop					// flush write pipeline (>= 040)
	move.l	a1,sp				// restore stack pointer

// next detection should start here

#ifdef __mcoldfire__
	move.l	(sp)+,a2
#endif
	rts
	END(detect_hardware)

	.data

// variables to set in ST-ESCC detection ****

	.globl SYM(ControlRegA)
	.globl SYM(DataRegA)
	.globl SYM(ControlRegB)
	.globl SYM(DataRegB)

SYM(ControlRegA):	dc.l 0xFFFF8C81
SYM(DataRegA):	dc.l 0xFFFF8C83
SYM(ControlRegB):	dc.l 0xFFFF8C85
SYM(DataRegB):	dc.l 0xFFFF8C87

	.text
//
// CPU detection, experimental (draco@atari.org).
//
// Return value:
//
// 0x0000 - 68000
// 0x000a - 68010
// 0x0014 - 68020
// 0x001e - 68030
// 0x0028 - 68040
// 0x003c - 68060
//

	.globl	SYM(detect_cpu)

// new routine, does not change the CPU state (esp. caches)
// <draco@atari.org>
//
// It is supposed to detect the ColdFire processor as 68020
//
//

	FUNC(detect_cpu)
SYM(detect_cpu):
#ifdef __mcoldfire__
	moveq	#60,d0		// only true when coldfire_68k_emulation = true
#else
	movem.l	a0-a2/d1-d2,-(sp)

	move.l	(0x10).w,-(sp)	// save the illegal instr. exception vector
	move.l	(0x2c).w,-(sp)	// save the Line-F exception vector (for ColdFire V2)
	move.l	(0xf4).w,-(sp)	// save the unimplemented instr. exception vector

	lea	exit.w(pc),a0	// set all vectors to point to the label `exit'
	move.l	a0,(0x10).w
	move.l	a0,(0x2c).w
	move.l	a0,(0xf4).w

	move.l	sp,a1		// save the ssp

	nop			// flush write pipelines

	clr.l	d0              // assume 68000

// on 68000 we try out a `move from CCR'.

	move.w	%ccr,d1		// legal on 68010+
	moveq	#10,d0

// CACR is present only in 68020+

	movec	%cacr,d1		// get cache control register
	moveq	#20,d0

	lea	no_040.w(pc),a0	// test if 68040 now
	move.l	a0,(0x10).w
	move.l	a0,(0x2c).w
	move.l	a0,(0xf4).w

	nop			// flush write pipelines

// this is CINVA, but the cache field specifies no cache
// (so this is NOP, in fact). According to Motorola, this
// should be illegal on ColdFire processors.

	dc.w	0xf418

	moveq	#40,d0

no_040:	lea	exit.w(pc),a0	// change vectors back to 'exit'
	move.l	a0,(0x10).w
	move.l	a0,(0x2c).w
	move.l	a0,(0xf4).w

	nop			// flush write pipelines

	move.l	a1,sp		// just to be sure

	cmp.w	#40,d0
	beq.s	x040

	movec	%cacr,d1	/* d1 = get cache control register */
	move.l	d1,d2		/* hold a copy for later */
	ori.w	#0x0101,d1	/* enable '020 instr. and '030 data caches */
	movec	d1,%cacr	/* set new cache controls from d1 */
	movec	%cacr,d1	/*   & read it back again to check */
	movec	d2,%cacr	/* restore original CACR */
	btst	#0,d1		/* if '020 instr. cache was not enabled, this is a 68040+ */
	beq.s	x040
	btst	#8,d1		/* check if 68030 data cache was enabled */
	beq.s	exit		/* a zero here means no data cache, i.e. 68020 */

// this idea is borrowed from Linux/m68k, following the suggestion
// of Petr Stehlik <joy@sophics.cz>.
//
// By the way: gas accepts `pmove tt0,d1' which is illegal. Why?
//
	clr.l	-(sp)		/* make some room on the stack */
	move.l  #0x0001001e,d0 /* data cache enabled means 68030 - assume 68ec030 */
	pmove	%tc,(a7)	/* try to access the TC register */
	moveq	#30,d0		/* no fault -> this is a 68030 */

	bra.s	exit

// 68040 or higher.

x040:	moveq	#40,d0		// assume 68040
	dc.l	0x4e7a1808	// attempt to access the PCR

// 68060 or Apollo 68080

	moveq	#60,d0		// assume 68060
	dc.l	0x06d00000	// check for 68080 addiw.l instruction

// Apollo 68080
	move.w	#1,SYM(is_apollo_68080)

exit:	move.l	a1,sp

	move.l	(sp)+,(0xf4).w	// restore stuff and exit
	move.l	(sp)+,(0x2c).w
	move.l	(sp)+,(0x10).w

	nop			// flush write pipelines

	movem.l	(sp)+,a0-a2/d1-d2
#endif
	rts
	END(detect_cpu)

// FPU type detection, experimental (draco@atari.org).
//
// This can only detect the hardware FPU, any software emulation
// will be ignored.
//
// Return value is cookie value for _FPU slot or a zero if no FPU
// is present:
//
// 0x00000000, no FPU
// 0x00020000, 68881 or 68882 [unused]
// 0x00040000, 68881
// 0x00060000, 68882
// 0x00080000, 68040 internal FPU
// 0x00100000, 68060 internal FPU
//
// The low word is supposed to have following meaning:
//
// 0x0000, no Line-F emulator is present
// other, version number of Line-F emulation (assigned by Atari);
//        high bit set means it is aware of 040/060 native instructions
//
// The detection algorithm goes as follows:
//
// - FNOP is executed. If a Line-F exception follows, then there's no FPU
//   in coprocessor mode. If the CPU is >= 68020, no FPU is assumed.
// - if FNOP doesn't take an exception, an FPU attached in coprocessor mode
//   is present. Then if 68040 or 68060 CPU was detected previously, an appropriate
//   FPU type is assumed. Otherwise the stackframe is checked for stack frame size
//   indicating 68881 and if it is different, a 68882 is assumed.

	.globl	SYM(detect_fpu)
	.globl	SYM(mcpu)

	FUNC(detect_fpu)
SYM(detect_fpu):
#ifdef __mcoldfire__
	moveq	#0,d0		// TODO ColdFire FPU
#else
	movem.l	a0-a2/d1,-(sp)
	move.l	sp,a0		// save the ssp
	clr.l	d0		// assume no FPU
	move.l	(0x2c).w,a1	// save the Line-F vector
	move.l	(0x08).w,a2
	move.l	#fexit,d1
	move.l	d1,(0x2c).w	// install temporary Line-F
	move.l	d1,(0x08).w
	nop			// flush pipelines

	move.l	SYM(mcpu),d1	// check if 68000 or 68010
	cmpi.w	#20,d1
	bmi.s	fexit

	cmpi.w	#60,d1		// enable FPU on 68060 before the check
	bmi.s	no60

	dc.l	0x4e7a0808	// movec pcr,d0
	swap	d0
	cmp.w	#0x0431,d0	// "broken" 68LC/EC060
	bne.b	rc60
	clr.l   d0
	bra.s   fexit

rc60:	swap	d0
	bclr	#0x01,d0
	dc.l	0x4e7b0808	// movec d0,pcr
	clr.l   d0

no60:	fnop
	clr.l	-(sp)		// push NULL frame
	clr.l	-(sp)		// extra longs for 68060
	clr.l	-(sp)
	frestore (sp)+		// reset the FPU into NULL state
	fnop			// force it into IDLE state
	fsave	-(sp)		// save the IDLE frame

	moveq	#0x10,d0	// assume 68060 FPU (cookie 0x00100000)
	cmpi.w	#60,d1		// d1 is loaded above the FPU code
	beq.s	fexit
	moveq	#0x08,d0	// if not 060, maybe a 040 (cookie 0x00080000)
	cmpi.w	#40,d1
	beq.s	fexit
	moveq	#0x04,d0	// assume 68881 (cookie 0x00040000)
	move.b	1(sp),d1
	cmpi.b	#0x18,d1
	beq.s	fexit
	moveq	#0x06,d0	// must be 68882 (cookie 0x00060000)

fexit:	move.l	a1,(0x2c).w	// restore Line-F
	move.l	a2,(0x08).w
	move.l	a0,sp
	nop			// flush pipelines
	swap	d0
	movem.l	(sp)+,a0-a2/d1
#endif
	rts
	END(detect_fpu)

// Software FPU emulation detection (SFP-004 or compatible)
//
// Return value is compatible with _FPU cookie in coprocessor mode:
//
// 0x00000000, no FPU
// 0x00020000, 68881 or 68882 [unused]
// 0x00040000, 68881
// 0x00060000, 68882

	.globl	SYM(detect_sfp)

	FUNC(detect_sfp)
SYM(detect_sfp):
#ifdef __mcoldfire__
	moveq	#0,d0		// there is no ColdFire SFP emulation
#else
	movem.l	a0-a1/d1,-(sp)
	move.l	sp,a0		// save the ssp
	clr.l	d0		// assume no FPU
	move.l	(0x08).w,a1	// save the bus error vector
	move.l	#sexit,(0x08).w	// install temporary bus error
	nop			// flush pipelines

sfp:	move.w	(0xfa44).w,d0	// read the save register
	move.w	d0,d1
	andi.w	#0xff00,d1	// isolate the format word
	beq.s	sfp_store	// if null idle, store the format word
	cmpi.w	#0x0100,d1	// if the coprocessor busy
	beq.s	sfp		// keep checking until CP is finished processing
sfp_store:
	move.w	d0,d1

	moveq	#0x04,d0	// 68881 (cookie 0x00040000) by default
	cmpi.b	#0x18,d1	// compare stack frame size
	beq.s	sexit
	moveq	#0x06,d0	// must be 68882 (cookie 0x00060000)

sexit:	move.l	a1,(0x08).w	// restore bus error
	move.l	a0,sp
	nop			// flush pipelines
	swap	d0
	movem.l	(sp)+,a0-a1/d1
#endif
	rts
	END(detect_sfp)

	.globl	SYM(detect_pmmu)

// PMMU detection
//
// A very basic routine. For 000-020 it assumes none,
// for 030 always present and for 040/060 it checks TC's 'E' bit, what is not
// an 100% correct thing to do according to the UM but it seems to work.

	FUNC(detect_pmmu)
SYM(detect_pmmu):
	moveq	#0,d0		// TODO ColdFire PMMU
#ifndef __mcoldfire__
	move.l	d1,-(sp)

	move.l	SYM(mcpu),d1	// check cpu
	cmp.w	#30,d1
	blt.b	pexit		// <030 => no pmmu
	beq.b	pexit1		// 030 => always pmmu for Atari

	dc.w	0x4e7a,0x1003	// movec tc,d1 (040/060)
	and.w	#0x8000,d1	// check 'E' bit
	beq.b	pexit

pexit1:	moveq	#1,d0
pexit:	move.l	(sp)+,d1
#endif
	rts
	END(detect_pmmu)
