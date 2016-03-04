/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 */

//
// This file has been modified as part of the FreeMiNT project. See
// the file Changes.MH for details and dates.
//

// Copyright 1992 Eric R. Smith
// Copyright 1993,1994 Atari Corporation
// All rights reserved.

#if __GNUC__ > 2
#ifndef __mcoldfire__
	.cpu	68030
	.arch	68030
#endif
#endif

#include "magic/magic.i"

//
// interrupt wrapping routines; these should just save registers and call
// the appropriate C handlers, unless speed is a major problem
//
	.text

	.globl	_build_context
	.globl	_restore_context
	.extern	_curproc
	.globl	_leave_kernel
	.globl	_preempt		// C function in proc.c
	.globl	_proc_clock		// controls process' allocation of CPU time
	.extern	_in_kernel
	.globl	__base
	.globl	_calc_load_average
	.globl	_uptimetick
	.globl	_checkbttys_vbl

// AKP: this code is hit once every 5ms
// it updates the time fields of curproc.

	.globl	_mint_5ms
	.globl	_old_5ms
#ifdef PROFILING
	.globl	_profil_on
	.globl	_profil_counter
#endif
	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_5ms:
	dc.l	0

#ifdef __mcoldfire__
_mint_5ms:
#ifdef PROFILING
	tst.w	_profil_on
	beq.s	L_no_profile
	lea	-24(sp),sp
	movem.l	d0-d2/a0-a2,(sp)	// save C registers
	move.l	26(sp),d0		// try to get interrupted PC
	move.l	d0,-(sp)
	jsr	_profil_counter		// call profiler
	addq.l	#0x04,sp		// this is better than lea
	movem.l	(sp),d0-d2/a0-a2	// restore C registers
	lea	24(sp),sp
L_no_profile:
#endif
	move.l	d0,-(sp)		// backup register
	mvz.w	vblcnt,d0
	subq.l	#0x01,d0		// each fourth interrupt makes a "VBL"
	bne.s	L_novbl_0
	moveq	#0x0004,d0
	move.w	d0,vblcnt
	bra.s	L_vbl
L_novbl_0:
	move.w	d0,vblcnt
	bra.s	L_novbl
L_vbl:
	// set up an exception stack frame for VBL emulation
	tst.w	_coldfire_68k_emulation
	bne.s	L_frame060

	// set up a ColdFire stack frame
	subq.l	#8,sp
	move.l	8(sp),(sp)	// move the d0 backup on the top of the stack
	move.w	#0x4000,4(sp)	// format/vector word
	move.w	sr,d0
	move.w	d0,6(sp)	// return sr
	move.l	#_mint_vbl,d0
	move.l	d0,8(sp)	// return pc
	bra.s	L_novbl

L_frame060:
	// set up a 68060 stack frame
	subq.l	#8,sp
	move.l	8(sp),(sp)	// move the d0 backup on the top of the stack
	move.w	sr,d0
	move.w	d0,4(sp)	// return sr
	move.l	#_mint_vbl,d0
	move.l	d0,6(sp)	// return pc
	clr.w	10(sp)		// format/vector word

L_novbl:
	move.l	(sp)+,d0		// restore register
	move.l	_old_5ms.w(pc),-(sp)	// push old vector & save a bit of memory
	move.l	a0,-(sp)
	move.w	_uptimetick,a0
	subq.l	#1,a0
	move.w	a0,_uptimetick
	move.l	_curproc,a0
	tst.w	_in_kernel
	bne.s	L_systime
	addq.l	#5,P_USRTIME(a0)	// update curproc->usrtime
	move.l	(sp)+,a0
	rts				// branch to old vector
L_systime:
	addq.l	#5,P_SYSTIME(a0)	// update curproc->systime
	move.l	(sp)+,a0
	rts
#elif !defined(M68000)
_mint_5ms:
#ifdef PROFILING
	tst.w	_profil_on
	beq.s	L_no_profile
	movem.l	d0-d2/a0-a2,-(sp)	// save C registers
	move.l	26(sp),d0		// try to get interrupted PC
	move.l	d0,-(sp)
	jsr	_profil_counter		// call profiler
	addq.l	#0x04,sp		// this is better than lea
	movem.l	(sp)+,d0-d2/a0-a2	// restore C registers
L_no_profile:
#endif
	subq.w	#0x01,vblcnt		// each fourth interrupt makes a "VBL"
	bne.s	L_novbl
	move.w	#0x0004,vblcnt

	clr.w	-(sp)		// set up a jump to VBL emulation
L_short_1:
	pea	_mint_vbl.w(pc)
	move.w	sr,-(sp)
L_novbl:
	subq.w	#1,_uptimetick
	tst.w	_in_kernel
	bne.s	L_systime
	addq.l	#5,([_curproc],P_USRTIME)
	jmp	([_old_5ms],0.w)
L_systime:
	addq.l	#5,([_curproc],P_SYSTIME)
	jmp	([_old_5ms],0.w)

#else

_mint_5ms:
#ifdef PROFILING
	tst.w	_profil_on
	beq.s	L_no_profile
	movem.l	d0-d2/a0-a2,-(sp)	// save C registers
	move.l	26(sp),d0		// try to get interrupted PC
	move.l	d0,-(sp)
	jsr	_profil_counter		// call profiler
	addq.l	#0x04,sp		// this is better than lea
	movem.l	(sp)+,d0-d2/a0-a2	// restore C registers
L_no_profile:
#endif
	subq.w	#0x01,vblcnt		// each fourth interrupt makes a "VBL"
	bne.s	L_novbl
	move.w	#0x0004,vblcnt

	tst.w	(0x059e).w
	beq.s	L_short_1
	clr.w	-(sp)			// set up a jump to VBL emulation
L_short_1:
	pea	_mint_vbl.w(pc)
	move.w	sr,-(sp)
L_novbl:
	move.l	_old_5ms.w(pc),-(sp)	// push old vector & save a bit of memory
	move.l	a0,-(sp)
	subq.w	#1,_uptimetick
	move.l	_curproc,a0
	tst.w	_in_kernel
	bne.s	L_systime
	addq.l	#5,P_USRTIME(a0)	// update curproc->usrtime
	move.l	(sp)+,a0
	rts				// branch to old vector
L_systime:
	addq.l	#5,P_SYSTIME(a0)	// update curproc->systime
	move.l	(sp)+,a0
	rts
#endif

// Our VBL handler.
// In fact this is not a VBL handler, this is the same 5ms interrupt as above
// just the routine below is called each 4th time (i.e. with 20ms period).
// The name is historical, in earlier versions of MiNT it was hooked onto
// VBL autovector.

	.data

#ifdef FLOPPY_ROUTINES
flopc:	dc.w	0
#endif
vblcnt:	dc.w	4
mod5:	dc.w	5		// initial value matters

	.text

	.globl	_mint_vbl
	.globl	_uptime
	.globl	_uptimetick
	.globl	_c20ms
	.globl	_keyrec
	.globl	_kintr
	.globl	_our_clock
	.globl	_tlist
#ifndef NO_AKP_KEYBOARD
	.globl	_autorepeat_timer
#endif
#ifdef FLOPPY_ROUTINES
	.globl	_flopvbl
#endif

// timeout things go here (50 times per second at IPL 6)

_mint_vbl:
#ifdef __mcoldfire__
	lea	-24(sp),sp
	movem.l	d0-d2/a0-a2,(sp)	// save C registers
#else
	movem.l	d0-d2/a0-a2,-(sp)	// save C registers
#endif
	addq.l	#0x01,_c20ms

#ifdef NO_AKP_KEYBOARD
	move.l	_keyrec,a0
	move.w	6(a0),d0
	cmp.w	8(a0),d0
#ifdef __mcoldfire__
	sne	d0
	move.b	d0,_kintr+1
#else
	sne	_kintr+1
#endif
#endif

	lea	_proc_clock,a0
#ifdef __mcoldfire__
	mvz.w	(a0),d0
#else
	move.w	(a0),d0
#endif
	beq.s	skip
#ifdef __mcoldfire__
	subq.l	#0x01,d0
#else
	subq.w	#0x01,d0
#endif
	move.w	d0,(a0)
skip:
#ifdef __mcoldfire__
	mvz.w	(0x0442).w,d0
	mvz.w	_our_clock,d1
	sub.l	d0,d1
	move.w	d1,_our_clock
#else
	move.w	(0x0442).w,d0
	sub.w	d0,_our_clock
#endif
	move.l	_tlist,d1
	beq.s	txit

// update the `when' field of the first TIMEOUT on the list

	move.l	d1,a0
	move.l	TM_WHEN(a0),a1
#ifdef __mcoldfire__
	sub.l	d0,a1
#else
	sub.w	d0,a1
#endif
	move.l	a1,TM_WHEN(a0)
// keyboard autorepeat

//#ifndef NO_AKP_KEYBOARD
// Ozk: Now keyboard delay/repeat is done via roottimeouts - see keyboard.c
//	jsr	_autorepeat_timer	// in keyboard.c
//#endif

// "VBL" things go here (50 times per second at IPL 3)

txit:
#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	txit_68k

	btst	#2,26(sp)		// don't perform if saved IPL is higher than 3 (bit 2 set)
	bne.s	L_popnout

	bra.s	L_vbl_emu
txit_68k:
#endif
	btst	#2,24(sp)		// don't perform if saved IPL is higher than 3 (bit 2 set)
	bne.s	L_popnout

L_vbl_emu:
	move.w	#0x2400,sr		// lower the current IPL

// floppy stuff

#ifdef FLOPPY_ROUTINES
#ifdef __mcoldfire__
	mvz.w	flopc(pc),d0		// exec each 8th time
	addq.l	#0x01,d0
#else
	move.w	flopc(pc),d0		// exec each 8th time
	addq.w	#0x01,d0
#endif
	move.w	d0,flopc
#ifdef __mcoldfire__
	and.l	#0x07,d0
	subq.l	#0x07,d0
#else
	and.w	#0x07,d0
	subq.w	#0x07,d0
#endif
	bne.s	piramida_hejhopsa
	jsr	_flopvbl
piramida_hejhopsa:
#endif

	jsr	_checkbttys_vbl
	lea	_uptimetick,a0
	tst.w	(a0)
	bgt.s	L_no_uptime
#ifdef __mcoldfire__
	mvz.w	(a0),d0
	add.l	#200,d0
	move.w	d0,(a0)
#else
	add.w	#200,(a0)
#endif
	addq.l	#0x01,_uptime
#ifdef __mcoldfire__
	lea	mod5,a0
	mvz.w	(a0),d0
	subq.l	#0x01,d0
	move.w	d0,(a0)
	bne.s	L_no_uptime
	move.w	#0x0005,(a0)
#else
	subq.w	#0x01,mod5
	bne.s	L_no_uptime
	move.w	#0x0005,mod5
#endif
	jsr	_calc_load_average	// Go test if it's time to update uptime
L_no_uptime:
#ifdef __mcoldfire__
	movem.l	(sp),d0-d2/a0-a2
	lea	24(sp),sp
#else
	movem.l	(sp)+,d0-d2/a0-a2
#endif
	tst.w	_proc_clock		// has time expired yet?
	beq.s	L_expired		// yes -- maybe go switch processes
L_out:	rte			// no -- just return

L_popnout:
#ifdef __mcoldfire__
	movem.l	(sp),d0-d2/a0-a2
	lea	24(sp),sp
#else
	movem.l	(sp)+,d0-d2/a0-a2
#endif
#ifdef M68000
	nop
#endif
	rte

// Process' time has expired, check if we can preempt

L_expired:
#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	L_expired_68k

	btst	#5,2(sp) 		// user mode?
	bne.s	L_out			// no -- switching is not possible

	bra.s	L_expired2
L_expired_68k:
#endif
	btst	#5,(sp) 		// user mode?
	bne.s	L_out			// no -- switching is not possible
L_expired2:
	tst.w	(0x043e).w		// test floppy disk lock variable
	bne.s	L_out			// if locked, can't switch
#ifdef __mcoldfire__
	tst.w	_in_kernel		// are we doing a kernel operation?
	bne.s	L_out
	move.l	d0,-(sp)
	moveq	#0x07,d0
	bset	d0,_in_kernel
	move.l	(sp)+,d0
#else
	bset	#0x07,_in_kernel	// are we doing a kernel operation?
	bne.s	L_out
#endif

L_switch:
	clr.w	-(sp)			// no frame format needed
	move.l	_curproc,-(sp)
	addq.l	#P_SYSCTXT,(sp)		// to get &curproc->ctxt[SYSCALL]
	jsr	_build_context		// build context
	move.l	_curproc,a0
	move.l	(a0),sp 		// use curproc->sysstack
	move.w	P_SYSCTXT+C_SR(a0),d7	// get saved int level

// inlined enter_kernel(0). Notice we can't be in_kernel here (see above),
// so no need to tst.w _in_kernel & stuff. Also, we don't need to increase
// the IPL anymore.
//
	move.w	sr,d0		// 2400 -> d0
#ifdef __mcoldfire__
	eor.l	d0,d7		// 2400 eor 0000 = 2400 -> d7
	and.l	#0x0700,d7	// 2400 and 0700 = 0400 -> d7
	eor.l	d7,d0		// 0400 eor 2400 = 2000 -> d0
#else
	eor.w	d0,d7		// 2400 eor 0000 = 2400 -> d7
	and.w	#0x0700,d7	// 2400 and 0700 = 0400 -> d7
	eor.w	d7,d0		// 0400 eor 2400 = 2000 -> d0
#endif
	move.w	d0,sr		// vbl allowed again

	jsr	_preempt		// yield processor

// inlined leave_kernel()

	move.l	_curproc,a0
#ifdef __mcoldfire__
	move.w	sr,d0
	ori.l	#0x0400,d0		// block VBL
	move.w	d0,sr
#else
	ori.w	#0x0400,sr		// block VBL
#endif
	clr.w	_in_kernel
	pea	4(a0)
	jsr	_restore_context	// back to user (never returns)

//
// routine for doing a reboot
//
	.globl	_reset
	.globl	_old_resvec
	.globl	_restr_intr
	.globl	_reboot

_reboot:
	move.l	(0).w,sp		// get sp after reboot
	move.l	(4).w,a6		// get new reboot address
	bra.s	_reset

// reset routine -- called on a warm boot. Note that TOS sends the
// address to which we should return in register a6. Also note that
// the stack pointer is in an unknown state, so we set up our own.
// There is a free page at 0x0600-0x06ff.

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_resvec:
	dc.l	0

_reset:	move.w	#0x2700,sr		// avoid interruption here
	move.l	sp,0x0600.w		// save A7
	lea	0x06fe.w,sp		// set up temporary stack
#ifdef __mcoldfire__
	lea	-24(sp),sp
	movem.l	d0-d2/a0-a2,(sp)	// save C registers
#else
	movem.l	d0-d2/a0-a2,-(sp)	// save C registers
#endif
	jsr	_restr_intr		// restore interrupts
#ifdef __mcoldfire__
	movem.l	(sp),d0-d2/a0-a2	// restore registers
	lea	24(sp),sp
#else
	movem.l	(sp)+,d0-d2/a0-a2	// restore registers
#endif
	move.l	0x0600.w,sp
	jmp	(a6)		// reset again

//
// routine for mouse packet handling
//
	.globl	_newmvec
	.globl	_newjvec
	.globl	_mouse_handler

// Experimental three button mouse support (by jr@ms.maus.de,
// August 4, 1992
//
// Should work with the mice shipped with Atari's ASV or
// compatible ones (like Golden Image GI-6000). Might not work
// on ST/STE systems with older IKBD's or keyboards. The middle mouse
// button is wired to one of the joystick directions on joystick one.
//
// _newmvec is the same as before with two exceptions:
// 1. the first byte of the packet is saved for the joystick handler
// 2. the bit for the middle mouse button is ored in
//
// _newjvec hooks into the joystick vector and chains to the normal
// handler. The middle mouse button state is saved in a special
// register for _newmvec, and a 'fake' mouse packet is set up
// (by merging the last mouse packet header, or-ing in the
// middle button state and using 0/0 for the x/y increment).
//
	.data

// buffer for faked mouse packet (actually only 3 bytes)

faked_packet:
	dc.l	0

// here we store the additional button state

third_button:
	dc.w	0

	.text

_newmvec:	move.l	a0,-(sp)
	move.b	(a0),d0
	move.b	d0,faked_packet
#ifdef __mcoldfire__
	mvz.b	third_button,d1
	or.l	d1,d0
#else
	or.b	third_button,d0
#endif
	move.b	d0,(a0)
	jsr	_mouse_handler
	move.l	(sp)+,a0
	rts
//
// routine for joystick packet handling (used for three button mice)
//
	.globl	_newjvec
	.globl	_oldjvec

_newjvec:	move.l	a0,-(sp)		// save a0 on the stack
	move.b	2(a0),d0		// joystick direction
#ifdef __mcoldfire__
	and.l	#1,d0		// middle mouse button in lowest bit
#else
	and.b	#1,d0		// middle mouse button in lowest bit
#endif
#ifdef __mcoldfire__
	lsl.l	#0x02,d0
#elif !defined(M68000)
	lsl.b	#0x02,d0
#else
	add.b	d0,d0		// times 4
	add.b	d0,d0
#endif
	move.b	d0,third_button 	// save it for use in newmvec

	lea	faked_packet,a0 	// 'our' faked mouse event
	move.b	(a0),d0
#ifdef __mcoldfire__
	and.l	#0x3,d0		// unmask our mouse button
	or.l	#0xF8,d0 		// or in correct header
	mvz.b	third_button,d1
	or.l	d1,d0 			// or in the current status
#else
	and.b	#0x3,d0		// unmask our mouse button
	or.b	#0xF8,d0 		// or in correct header
	or.b	third_button,d0 	// or in the current status
#endif
	move.b	d0,(a0) 		// write it back

	move.l	a0,-(sp)		// pass pointer to fake packet
	jsr	_mouse_handler		// to /dev/mouse handler
	addq.l	#4,sp		// pop parameter
	move.l	(sp)+,a0		// restore original a0 value
	move.l	_oldjvec,-(sp)		// jump to original joystick handler
	rts

// The routine is called by TOS prior to processing the IKBD
// keyboard scancode. The scancode byte is in d0, IOREC struct
// pointer in a0.
//
// BUG: will not work on TOS < 2.0. The vector used is available
//	since TOS 2.0.

#ifndef NO_AKP_KEYBOARD
#ifdef MILAN
	.globl	_newkeys
	.globl	_oldkeys
	.globl	_ikbd_scan

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_oldkeys:	dc.l	0
_newkeys:
	movem.l	d1-d2/a0-a2,-(sp)
	move.l	_keyrec,-(sp)
	and.w	#0x00ff,d0
	move.w	d0,-(sp)
	jsr	_ikbd_scan
	addq.l	#6,sp
	movem.l	(sp)+,d1-d2/a0-a2
	rts
#endif

// Calling the keyclick routines. This is a helper for keyboard driver.

	.globl	_kbdclick

_kbdclick:
#ifdef __mcoldfire__
	move.l	d0,-(sp)
	moveq	#0,d0
	btst	d0,(0x0484).w	// conterm
	beq.s	L_nocl
	lea	-56(sp),sp
	movem.l	d1-a6,(sp)
#else
	btst	#0,(0x0484).w	// conterm
	beq.s	L_nocl
	movem.l	d0-a6,-(sp)
#endif
	move.l	(0x05b0).w,d0	// vector to the keyclick routine
	beq.s	L_pop
	move.l	d0,a0
	clr.l	d0
	move.w	64(sp),d0	// scancode
	jsr	(a0)
L_pop:
#ifdef __mcoldfire__
	movem.l	(sp),d1-a6
	lea	56(sp),sp
#else
	movem.l	(sp)+,d0-a6
#endif
L_nocl:
#ifdef __mcoldfire__
	move.l	(sp)+,d0
#endif
	rts

// Send mouse packet to mousevec(). This is used to emulate mouse
// movements with keyboard shortcuts (alt/arrow).

	.globl	_send_packet

_send_packet:
#ifdef __mcoldfire__
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)
#else
	movem.l	d0-a6,-(sp)
#endif
	move.l	64(sp),a2
	move.l	68(sp),a0
	move.l	72(sp),a1
	jsr	(a2)
#ifdef __mcoldfire__
	movem.l	(sp),d0-a6
	lea	60(sp),sp
#else
	movem.l	(sp)+,d0-a6
#endif
	rts
#endif

//
// simple signal handlers
// global variables referenced:
// in_kernel: (main.c): flag to indicate that we're in the MiNT kernel
// sig_routine: (signal.c): pointer to which signal catching routine to
//	   call (e.g. for SIGBUS, or whatever)
//
	.globl	_new_bus,_new_addr,_new_ill,_new_divzero,_new_priv,_new_linef
	.globl	_new_trace,_new_chk,_new_trapv,_new_fpcp,_new_mmu,_new_pmmuacc
	.globl	_new_uninit,_new_spurious,_new_format,_new_cpv
	.extern	_in_kernel,_sig_routine
	.globl	_sigbus,_sigaddr,_sigill,_sigfpe,_sigpriv,_sigtrap
	.globl	_haltformat,_haltcpv
	.globl	_sig_exc
	.globl	_mcpu

//
// New bus error handler for memory protection: get the ssp and
// put it in the proc structure before calling
// _sigbus.  When the bus error happens in the kernel we don't save
// any contexts.
// We don't want to mess up any registers here because we might bring the
// page in and RTE.
//

	.globl	_check_bus,_old_bus

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_bus:
	dc.l	0
_new_bus:
#ifndef NO_FAKE_SUPER
#ifdef __mcoldfire__
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)		// check_bus() relies on this!
#else
	movem.l	d0-a6,-(sp)		// check_bus() relies on this!
#endif
	jsr	_check_bus
	tst.l	d0
#ifdef __mcoldfire__
	movem.l	(sp),d0-a6
	lea	60(sp),sp
#else
	movem.l	(sp)+,d0-a6
#endif
#if !(defined(M68000) || defined(__mcoldfire__))
	bne.s	Return_now
#else
	bne	Return_now
#endif
#endif
#ifdef M68000
	move.w	#0x8,_sig_exc
	cmp.l	#30,_mcpu
	bmi.s	noMMU
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_mmu_sigbus,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_mmu_sigbus,_sig_routine
#endif
#ifdef M68000
	bra.s	Do_sig
noMMU:
	move.l	#_nommu_sigbus,_sig_routine
#endif

Do_sig:
#ifndef M68000
	nop			// flush pipes (better safe than sorry)
#endif
	move.l	a0,-(sp)		// save a0
	move.l	_curproc,a0
	move.l	sp,P_EXCSSP(a0)
	addq.l	#4,P_EXCSSP(a0)
#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	do_sig_68k

	move.l	8(sp),P_EXCPC(a0)
	bra.s	do_sig_restore

do_sig_68k:
#endif
	move.l	6(sp),P_EXCPC(a0)
do_sig_restore:
	move.l	(sp)+,a0

	tst.w	_in_kernel		// are we already in the kernel?
	bne.s	Kernel		// yes
	move.w	_sig_exc,-(sp)
	move.l	_curproc,-(sp)
	addq.l	#4,(sp) 		// push offset of save area
	jsr	_build_context
	move.l	_curproc,a4
	move.l	(a4),sp 		// put us in the system stack
#ifdef __mcoldfire__
	moveq	#0x07,d0
	bset	d0,_in_kernel
#else
	bset	#0x07,_in_kernel
#endif
	move.l	_sig_routine,a1 	// get signal handling routine
	jsr	(a1)		// go do it
#ifdef __mcoldfire__
	move.w	sr,d0
	ori.l	#0x0400,d0	// block VBL
	move.w	d0,sr
#else
	ori.w	#0x0400,sr	// block VBL
#endif
	jsr	_leave_kernel		// leave kernel
	addq.l	#4,a4		// get context save area address
	move.l	a4,-(sp)		// push it
	jsr	_restore_context	// restore the context
//
// here's what we do if we already were in the kernel
//
Kernel:
#ifdef __mcoldfire__
	lea	-24(sp),sp
	movem.l	d0-d2/a0-a2,(sp)	// save reggies
#else
	movem.l	d0-d2/a0-a2,-(sp)	// save reggies
#endif
	move.l	_sig_routine,a1 	// get handler
	jsr	(a1)		// go do it
#ifdef __mcoldfire__
	movem.l (sp),d0-d2/a0-a2
	lea	24(sp),sp
#else
	movem.l (sp)+,d0-d2/a0-a2
#endif
// 68000 processor bug: rte right after movem can eat interrupts...
#ifdef M68000
	nop
#endif
Return_now:
	rte

//
// _mmu_sigbus: a pre-handler for _sigbus.  Check the reason for the bus
// error and report if it was a real access fault.
//
_mmu_sigbus:
	move.l	a2,-(sp)
	move.l	_curproc,a0
	move.l	P_EXCSSP(a0),a1 	// a1 is now exception_ssp
#ifdef __mcoldfire__
	tst.w	_coldfire_68k_emulation
	bne.s	mmu_sigbus_68k

	// TODO ColdFire MMU

	bra.s	ms_goon
mmu_sigbus_68k:
#endif
#if !defined(M68040) && !defined(M68060)
	move.w	0xA(a1),d0		// d0 is SSW
	btst	#6,d0			// read or write?
	beq.s	ms_w030
	move.w	#2,P_EXCACCESS(a0)	// it was a read access
	bra.s	ms_aa030
ms_w030:
	move.w	#1,P_EXCACCESS(a0)	// it was a write access
ms_aa030:
	move.l	0x10(a1),a1		// a1 is the access address
	move.l	a1,P_EXCADDR(a0)	// save the access address

	ptestr	d0,(a1),#7,a2		// a2 is the table address
					// (no, it's the descriptor address
					// matching (a1) - Gryf)
	move.l	a2,P_EXCTBL(a0) 	// save table address in curproc
	pmove	psr,P_EXCMMUSR(a0)	// save resulting mmusr in curproc
#else
	move.l	_mcpu,d0
	cmpi.w	#40,d0			// the 060 uses a different stackframe
	bne.s	ms_060
	move.w	0xc(a1),d0		// d0 is SSW
	btst	#8,d0			// read or write?
	beq.s	ms_w040
	move.w	#2,P_EXCACCESS(a0)	// it was a read access
	bra.s	ms_aa040
ms_w040:
	move.w	#1,P_EXCACCESS(a0)	// it was a write access
ms_aa040:
	move.l	0x14(a1),a1		// a1 is the access address
	move.l	a1,P_EXCADDR(a0)	// save the access address
	bra.s	ms_goon
ms_060:
	move.l	0xc(a1),d0		// d0 is FSLW
	swap	d0
#ifdef __mcoldfire__
	lsr.l	#7,d0			// extract the access type
	andi.l	#3,d0
#else
	lsr.w	#7,d0			// extract the access type
	andi.w	#3,d0
#endif
	move.w	d0,P_EXCACCESS(a0)	// save the access type
	move.l	0x8(a1),a1		// a1 is the access address
	move.l	a1,P_EXCADDR(a0)	// save the access address
#endif
ms_goon:
	move.l	(sp)+,a2
	jmp	_sigbus 		// chain to bus-error handler
#ifdef M68000
//
// _nommu_sigbus: handler for bus errors on machines without MMU

_nommu_sigbus:
	move.l	_curproc,a0
	move.l	P_EXCSSP(a0),a1
	lea	0x10(a1),a1		// point to access address
	tst.w	(0x59e).w		// test longframe
	beq.s	NOMMU1
	addq.w	#8,a1		// on 68000, address is 8 bytes further
NOMMU1:
	move.l	(a1),P_EXCADDR(a0)	// save the access address
	jmp	_sigbus
#endif

	.globl	_old_addr

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_addr:
	dc.l	0
_new_addr:
#ifdef M68000
	move.w	#0xc,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigaddr,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigaddr,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_ill

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_ill:
	dc.l	0
_new_ill:
#ifdef M68000
	move.w	#0x10,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigill,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigill,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_divzero

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_divzero:
	dc.l	0
_new_divzero:
#ifdef M68000
	move.w	#0x14,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigfpe,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigfpe,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_linef

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_linef:
	dc.l	0
_new_linef:
#ifdef M68000
	move.w	#0x2c,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigill,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigill,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_chk

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_chk:
	dc.l	0
_new_chk:
#ifdef M68000
	move.w	#0x18,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigfpe,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigfpe,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_trapv

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_trapv:
	dc.l	0
_new_trapv:
#ifdef M68000
	move.w	#0x1c,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigfpe,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigfpe,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_fpcp_0,_old_fpcp_1,_old_fpcp_2,_old_fpcp_3,_old_fpcp_4
	.globl	_old_fpcp_5,_old_fpcp_6

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_fpcp_0:
	dc.l	0
	bra.s	_new_fpcp

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_fpcp_1:
	dc.l	0
	bra.s	_new_fpcp

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_fpcp_2:
	dc.l	0
	bra.s	_new_fpcp

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_fpcp_3:
	dc.l	0
	bra.s	_new_fpcp

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_fpcp_4:
	dc.l	0
	bra.s	_new_fpcp

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_fpcp_5:
	dc.l	0
	bra.s	_new_fpcp

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_fpcp_6:
	dc.l	0
_new_fpcp:
// don't set _sig_exc - only needed for 68000 vectors
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigfpe,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigfpe,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_mmuconf,_new_mmuconf

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_mmuconf:
	dc.l	0
_new_mmuconf:
// don't set _sig_exc - only needed for 68000 vectors
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigill,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigill,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_pmmuill

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_pmmuill:
	dc.l	0
_new_mmu:
// don't set _sig_exc - only needed for 68000 vectors
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigill,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigill,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_pmmuacc

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_pmmuacc:
	dc.l	0

_new_pmmuacc:
// don't set _sig_exc - only needed for 68000 vectors
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigbus,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigbus,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_uninit

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_uninit:
	dc.l	0
_new_uninit:
#ifdef M68000
	move.w	#0x3c,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigbus,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigbus,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_spurious

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_spurious:
	dc.l	0
_new_spurious:
#ifdef M68000
	move.w	#0x60,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigbus,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigbus,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_format

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_format:
	dc.l	0
_new_format:
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_haltformat,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_haltformat,_sig_routine
#endif
	bra	Do_sig

	.globl	_old_cpv

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_cpv:
	dc.l	0
_new_cpv:
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_haltcpv,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_haltcpv,_sig_routine
#endif
	bra	Do_sig

	.globl	_check_priv,_old_priv

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_priv:
	dc.l	0
_new_priv:
#ifdef M68000
	move.w	#0x20,_sig_exc
#endif
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	lea	_sig_routine,a0
	move.l	#_sigpriv,(a0)
	move.l	(sp)+,a0
#else
	move.l	#_sigpriv,_sig_routine
#endif
#ifdef M68000
	tst.w	(0x59e).w		// 68000s always get SIGPRIV
	beq	Do_sig
#endif
#ifndef M68000
#ifdef __mcoldfire__
	lea	-60(sp),sp
	movem.l	d0-a6,(sp)		// check_priv() relies on this!
#else
	movem.l	d0-a6,-(sp)		// check_priv() relies on this!
#endif
	move.l	sp,d0
	move.l	d0,-(sp)
	jsr	_check_priv
	addq.l	#4,sp
	tst.l	d0
#ifdef __mcoldfire__
	movem.l	(sp),d0-a6
	lea	60(sp),sp
#else
	movem.l	(sp)+,d0-a6
#endif
	beq	Do_sig
#endif
	rte

// system entry points from syscall.spp

	.extern	_mint_dos,_mint_bios,_mint_xbios
	.globl	_old_trace

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_trace:
	dc.l	0
_new_trace:
#ifdef __mcoldfire__
	move.l	a0,-(sp)
	move.l	d0,-(sp)

	tst.w	_coldfire_68k_emulation
	bne.s	cf_68k_trace

	lea	10(sp),a0		// native coldfire: sr, pc
	bra.s	cf_trace

cf_68k_trace:
	lea	8(sp),a0		// 68K emulation: sr, pc

cf_trace:
	btst	#5,(a0) 		// only check when called from supervisor mode
	beq.s	S_1
	move.l	2(a0),d0		// pc
	cmp.l	#_mint_dos,d0		// lets not trace the kernel !
	beq.s	S_2
	cmp.l	#_mint_xbios,d0
	beq.s	S_2
	cmp.l	#_mint_bios,d0
	beq.s	S_2
	cmp.l	#_new_divzero,d0
	beq.s	S_2
	cmp.l	#_new_trapv,d0
	beq.s	S_2
	cmp.l	#_new_chk,d0
	beq.s	S_2

// add any other non-traceable entities here...

S_1:	moveq	#0x24,d0
	move.w	d0,_sig_exc
	move.l	#_sigtrap,d0
	move.l	d0,_sig_routine
	move.l	(sp)+,d0
	move.l	(sp)+,a0
	bra	Do_sig

S_2:	move.w	(a0),d0
	and.l	#0x3fff,d0		// clear both trace bits
	move.w	d0,(a0)
	move.l	(sp)+,d0
	move.l	(sp)+,a0
#else
	btst	#5,(sp) 		// only check when called from supervisor mode
	beq.s	S_1
	cmp.l	#_mint_dos,2(sp)	// lets not trace the kernel !
	beq.s	S_2
	cmp.l	#_mint_xbios,2(sp)
	beq.s	S_2
	cmp.l	#_mint_bios,2(sp)
	beq.s	S_2
	cmp.l	#_new_divzero,2(sp)
	beq.s	S_2
	cmp.l	#_new_trapv,2(sp)
	beq.s	S_2
	cmp.l	#_new_chk,2(sp)
	beq.s	S_2

// add any other non-traceable entities here...

S_1:	move.w	#0x24,_sig_exc
	move.l	#_sigtrap,_sig_routine
	bra	Do_sig

S_2:	and.w	#0x3fff,(sp)		// clear both trace bits
#endif
	rte			// and re-start the handler

//
// BIOS disk vectors for pseudo-disks like U: and X:
// these are present
// just in case some program (foolishly) attempts to access these drives
// directly and gets horribly confused
//
	.globl	_old_getbpb		// old Getbpb vector
	.globl	_old_mediach		// old Mediach vector
	.globl	_old_rwabs		// old Rwabs vector
	.globl	_aliasdrv		// array of drive aliases
	.globl	_new_getbpb
	.globl	_new_mediach
	.globl	_new_rwabs

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_getbpb:
	dc.l	0
_new_getbpb:
#ifdef __mcoldfire__
	mvz.w	4(sp),d0		// check the drive
#else
	move.w	4(sp),d0		// check the drive
#endif
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	noalias0		// no
#ifdef __mcoldfire__
	move.l	d0,d1		// get index
	add.l	d0,d1		// convert to index
#else
	move.w	d0,d1		// get index
	add.w	d0,d1		// convert to index
#endif
	lea	_aliasdrv,a0
#ifdef __mcoldfire__
	move.w	0(a0,d1.l),d1		// alias drive?
#else
	move.w	0(a0,d1.w),d1		// alias drive?
#endif
	beq.s	noalias0
	move.w	d1,d0
#ifdef __mcoldfire__
	subq.l	#1,d0		// adjust for aliasdrv base of '@'
#else
	subq.w	#1,d0		// adjust for aliasdrv base of '@'
#endif
	cmp.w	#0x1f,d0 		// is this a legal drive?
	bhi.s	nobpb		// no -- ignore it
noalias0:
	cmp.w	#0x14,d0 		// drive U:?
	beq.s	nobpb		// yes, no BPB available

	move.l	_old_getbpb.w(pc),a0	// not our drive
	jmp	(a0)		// call the old vector for it
nobpb:
	clr.l	d0		// 0 means "no BPB read"
	rts

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_mediach:
	dc.l	0
_new_mediach:
#ifdef __mcoldfire__
	mvz.w	4(sp),d0		// check the drive
#else
	move.w	4(sp),d0		// check the drive
#endif
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	noalias1		// no
#ifdef __mcoldfire__
	move.l	d0,d1		// get index
	add.l	d0,d1		// convert to index
#else
	move.w	d0,d1		// get index
	add.w	d0,d1		// convert to index
#endif
	lea	_aliasdrv,a0
#ifdef __mcoldfire__
	move.w	0(a0,d1.l),d1		// alias drive?
#else
	move.w	0(a0,d1.w),d1		// alias drive?
#endif
	beq.s	noalias1
	move.w	d1,d0
#ifdef __mcoldfire__
	subq.l	#1,d0		// adjust for aliasdrv base
#else
	subq.w	#1,d0		// adjust for aliasdrv base
#endif
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	nobpb		// no -- ignore it
noalias1:
	cmp.w	#0x14,d0 		// drive U:?
	beq.s	nochng		// yes, no change
	move.l	_old_mediach.w(pc),a0	// not our drive
	jmp	(a0)		// call the old vector for it
nochng:
	clr.l	d0		// 0 means "definitely no change"
	rts

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_rwabs:
	dc.l	0
_new_rwabs:
#ifdef __mcoldfire__
	mvz.w	0xe(sp),d0		// check the drive
#else
	move.w	0xe(sp),d0		// check the drive
#endif
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	noalias2		// no
#ifdef __mcoldfire__
	move.l	d0,d1		// get index
	add.l	d0,d1		// convert to index
#else
	move.w	d0,d1		// get index
	add.w	d0,d1		// convert to index
#endif
	lea	_aliasdrv,a0
#ifdef __mcoldfire__
	move.w	0(a0,d1.l),d1		// alias drive?
#else
	move.w	0(a0,d1.w),d1		// alias drive?
#endif
	beq.s	noalias2
	move.w	d1,d0
#ifdef __mcoldfire__
	subq.l	#1,d0		// adjust for aliasdrv base
#else
	subq.w	#1,d0		// adjust for aliasdrv base
#endif
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	nobpb		// no -- ignore it
noalias2:
	cmp.w	#0x14,d0 		// drive U:?
	beq.s	rwdone		// yes, fake it

	move.l	_old_rwabs.w(pc),a0	// not our drive
	jmp	(a0)		// call the old vector for it
rwdone:
	clr.l	d0		// 0 means "successful operation"
	rts

	.globl	_old_criticerr,_new_criticerr
	.extern _mint_criticerr

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_criticerr:
	dc.l	0
_new_criticerr:
	jmp	_mint_criticerr

	.globl	_old_exec_os,_new_exec_os
	.extern _do_exec_os

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_exec_os:
	dc.l	0
_new_exec_os:
	jmp	_do_exec_os
