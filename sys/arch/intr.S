//
// This file has been modified as part of the FreeMiNT project. See
// the file Changes.MH for details and dates.
//

// Copyright 1992 Eric R. Smith
// Copyright 1993,1994 Atari Corporation
// All rights reserved.

#include "magic/magic.i"

//
// interrupt wrapping routines; these should just save registers and call
// the appropriate C handlers, unless speed is a major problem
//
	.text

	.globl	_build_context
	.globl	_restore_context
	.globl	_proc_clock		// controls process' allocation of CPU time
	.globl	_curproc
	.globl	_enter_bios		// see kernel.spp
	.globl	_leave_kernel
	.globl	_preempt		// C function in proc.c
	.globl	_in_kernel
	.globl	_tosbp,__base
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
_old_5ms:	dc.l	0	

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

#ifndef ONLY030
	tst.w	(0x059e).w
	beq.s	L_short_1
#endif
	clr.w	-(sp)		// set up a jump to VBL emulation
L_short_1:
	pea	_mint_vbl.w(pc)
	move.w	sr,-(sp)
L_novbl:
#ifdef ONLY030
	subq.w	#1,_uptimetick
	tst.w	_in_kernel
	bne.s	L_systime
	addq.l	#5,([_curproc],P_USRTIME)
	jmp	([_old_5ms],0.w)
L_systime:
	addq.l	#5,([_curproc],P_SYSTIME)
	jmp	([_old_5ms],0.w)
#else
	move.l	_old_5ms.w(pc),-(sp)	// push old vector & save a bit of memory
	move.l	a0,-(sp)
	subq.w	#1,_uptimetick
	move.l	_curproc,a0
	tst.w	_in_kernel
	bne.s	L_systime
	addq.l	#5,P_USRTIME(a0)	// update curproc->usrtime
	move.l	(sp)+,a0
	rts			// branch to old vector
L_systime:
	addq.l	#5,P_SYSTIME(a0)	// update curproc->systime
	move.l	(sp)+,a0
	rts
#endif

// Our VBL handler.
// Long time ago (just a version back :-)) it was a real VBL handler.
// However, it is not anymore...

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
	.globl	_proc_clock
	.globl	_our_clock
	.globl	_tlist
#ifdef FLOPPY_ROUTINES
	.globl	_flopvbl
#endif

// timeout things go here (50 times per second at IPL 6)

_mint_vbl:	movem.l	d0-d2/a0-a2,-(sp)	// save C registers
	addq.l	#0x01,_c20ms
	move.l	_keyrec,a0
	move.w	6(a0),d0
	cmp.w	8(a0),d0
	sne	_kintr+1
	lea	_proc_clock,a0
	move.w	(a0),d0
	beq.s	skip
	subq.w	#0x01,d0
	move.w	d0,(a0)
skip:	move.w	(0x0442).w,d0
	sub.w	d0,_our_clock
	move.l	_tlist,d1
	beq.s	txit
	move.l	d1,a0
	move.l	8(a0),a1
	sub.w	d0,a1
	move.l	a1,8(a0)

txit:

// floppy stuff

#ifdef FLOPPY_ROUTINES
	move.w	flopc(pc),d0		// exec each 8th time
	addq.w	#0x01,d0
	move.w	d0,flopc
	and.w	#0x07,d0
	subq.w	#0x07,d0
	bne.s	piramida_hejhopsa
	jsr	_flopvbl
piramida_hejhopsa:
#endif

// "VBL" things go here (50 times per second at IPL 3)

	btst	#2,24(sp)		// don't perform if saved IPL is higher than 3 (bit 2 set)
	bne.s	L_popnout
	move.w	#0x2300,sr		// lower the current IPL
	jsr	_checkbttys_vbl
	lea	_uptimetick,a0
	tst.w	(a0)
	bgt.s	L_no_uptime
	add.w	#200,(a0)
	addq.l	#0x01,_uptime
	subq.w	#0x01,mod5
	bne.s	L_no_uptime
	move.w	#0x0005,mod5
	jsr	_calc_load_average	// Go test if it's time to update uptime
L_no_uptime:
	movem.l	(sp)+,d0-d2/a0-a2
	tst.w	_proc_clock		// has time expired yet?
	beq.s	L_expired		// yes -- maybe go switch processes
L_out:	rte			// no -- just return

L_popnout:	movem.l	(sp)+,d0-d2/a0-a2
#ifndef ONLY030
	nop
#endif
	rte

// Process' time has expired, check if we can preempt

L_expired:
	btst	#5,(sp) 		// user mode?
	bne.s	L_out		// no -- switching is not possible
	tst.w	(0x043e).w		// test floppy disk lock variable
	bne.s	L_out		// if locked, can't switch
	bset	#0x07,_in_kernel	// are we doing a kernel operation?
	bne.s	L_out

L_switch:
	clr.w	-(sp)		// no frame format needed
	move.l	_curproc,-(sp)
	addq.l	#P_CTXT0,(sp)		// to get &curproc->ctxt[SYSCALL]
	jsr	_build_context		// build context
	move.l	_curproc,a0
	move.l	(a0),sp 		// use curproc->sysstack
	move.w	P_CTXT0+C_SR(a0),d7	// get saved int level

// inlined enter_kernel(0). Notice we can't be in_kernel here (see above),
// so no need to tst.w _in_kernel & stuff. Also, we don't need to increase
// the IPL anymore.
//
	move.l	_tosbp,a0
	move.l	__base,(a0)
	move.w	sr,d0
	eor.w	d0,d7
	and.w	#0x0700,d7
	eor.w	d7,d0
	move.w	d0,sr		// vbl allowed again

	jsr	_preempt		// yield processor

// inlined leave_kernel()

	move.l	_tosbp,a1
	move.l	_curproc,a0
	move.l	P_BASE(a0),(a1)
//	ori.w	#0x0700,sr
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
	movem.l	d0-d2/a0-a2,-(sp)	// save C registers
	jsr	_restr_intr		// restore interrupts
	movem.l	(sp)+,d0-d2/a0-a2	// restore registers
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
	or.b	third_button,d0
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
	and.b	#1,d0		// middle mouse button in lowest bit
#ifdef ONLY030
	lsl.b	#0x02,d0
#else
	add.b	d0,d0		// times 4
	add.b	d0,d0
#endif
	move.b	d0,third_button 	// save it for use in newmvec

	lea	faked_packet,a0 	// 'our' faked mouse event
	move.b	(a0),d0
	and.b	#0x3,d0		// unmask our mouse button
	or.b	#0xF8,d0 		// or in correct header
	or.b	third_button,d0 	// or in the current status
	move.b	d0,(a0) 		// write it back

	move.l	a0,-(sp)		// pass pointer to fake packet
	jsr	_mouse_handler		// to /dev/mouse handler
	addq.l	#4,sp		// pop parameter
	move.l	(sp)+,a0		// restore original a0 value
	move.l	_oldjvec,-(sp)		// jump to original joystick handler
	rts

// The routine is called by TOS prior to processing the IKBD
// keyboard scancode. The scancode byte is in d0.
// BUG: not sure if this works on TOS < 2.0. The vector used
//      seems not published (although it may be just a flaw
//      in docs).

	.globl	_newkeys
	.globl	_oldkeys
	.globl	_ikbd_scan

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_oldkeys:	dc.l	0
_newkeys:	movem.l	d1-d2/a0-a2,-(sp)
	move.w	d0,-(sp)
	jsr	_ikbd_scan
	addq.l	#0x02,sp
	movem.l	(sp)+,d1-d2/a0-a2
	cmp.w	#0xffff,d0
	beq.s	L_bye
	move.l	_oldkeys.w(pc),-(sp)
L_bye:	rts

// Calling the keyclick routines. This is a helper for keyboard driver.

	.globl	_kbdclick

_kbdclick:	move.l	(0x05b0).w,d0
	beq.s	L_nocl
	move.l	d0,a0
	clr.l	d0
	move.w	4(sp),d0	// scancode
	jmp	(a0)
L_nocl:	rts

// Generic routines for handling any user-specified interrupts.
// These only work on a 68020+ (long frame format is needed),
// psigintr() code in dossig.c prevents this from being ran on
// a 68000.
//
	.globl	_new_intr
	.globl	_intr_shadow
	.globl	_sig_user

	.data

_intr_shadow:	dc.l	0		// pointer to private shadow of exception vectors

	.text

_new_intr:	subq.l	#0x08,sp		// make some room on the stack
	movem.l	d0-d2/a0-a2,-(sp)	// save regs
	move.w	38(sp),d0		// get frame word
	and.w	#0x0fff,d0		// mask out the format code
	move.w	sr,24(sp)		// stackframe for RTE
	move.l	_intr_shadow,a0
	move.l	0(a0,d0.w),26(sp)
	move.w	d0,30(sp)
	lsr.w	#0x02,d0		// convert vector offset to vector number
	move.w	d0,-(sp)		// push vector number
	jsr	_sig_user		// send signal
	addq.l	#0x02,sp
	movem.l	(sp)+,d0-d2/a0-a2
	rte			// jump to original services

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
	.globl	_in_kernel,_sig_routine
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

_new_bus:
#ifndef ONLY030
	move.w	#0x8,_sig_exc
	cmp.l	#30,_mcpu
	bne.s	noMMU
#endif
	move.l	#_mmu_sigbus,_sig_routine
#ifndef ONLY030
	bra.s	Do_sig
noMMU:
	move.l	#_nommu_sigbus,_sig_routine
#endif

Do_sig:	move.l	a0,-(sp)		// save a0
	move.l	_curproc,a0
	move.l	sp,P_EXCSSP(a0)
	addq.l	#4,P_EXCSSP(a0)
	move.l	6(sp),P_EXCPC(a0)
	move.l	(sp)+,a0

	tst.w	_in_kernel		// are we already in the kernel?
	bne.s	Kernel		// yes
	move.w	_sig_exc,-(sp)
	move.l	_curproc,-(sp)
	addq.l	#4,(sp) 		// push offset of save area
	jsr	_build_context
	move.l	_curproc,a4
	move.l	(a4),sp 		// put us in the system stack
	jsr	_enter_bios		// set up kernel vectors
	move.l	_sig_routine,a1 	// get signal handling routine
	jsr	(a1)		// go do it
//	ori.w	#0x0700,sr
	jsr	_leave_kernel		// leave kernel
	addq.w	#4,a4		// get context save area address
	move.l	a4,-(sp)		// push it
	jsr	_restore_context	// restore the context
//
// here's what we do if we already were in the kernel
//
Kernel:	movem.l	d0-d2/a0-a2,-(sp)	// save reggies
	move.l	_sig_routine,a1 	// get handler
	jsr	(a1)		// go do it
	movem.l (sp)+,d0-d2/a0-a2
// 68000 processor bug: rte right after movem can eat interrupts...
#ifndef ONLY030
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
#ifndef MMU040
	move.w	0xA(a1),d0		// d0 is SSW
	btst	#6,d0		// read or write?
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
	cmpi.w	#40,d0		// the 060 uses a different stackframe
	bne.s	ms_060
	move.w	0xc(a1),d0		// d0 is SSW
	btst	#8,d0		// read or write?
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
	lsr.w	#7,d0		// extract the access type
	andi.w	#3,d0
	move.w	d0,P_EXCACCESS(a0)	// save the access type
	move.l	0x8(a1),a1		// a1 is the access address
	move.l	a1,P_EXCADDR(a0)	// save the access address
#endif
ms_goon:
	move.l	(sp)+,a2
	jmp	_sigbus 		// chain to bus-error handler
#ifndef ONLY030
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

_new_addr:
#ifndef ONLY030
	move.w	#0xc,_sig_exc
#endif
	move.l	#_sigaddr,_sig_routine
	bra	Do_sig
_new_ill:
#ifndef ONLY030
	move.w	#0x10,_sig_exc
#endif
	move.l	#_sigill,_sig_routine
	bra	Do_sig
_new_divzero:
#ifndef ONLY030
	move.w	#0x14,_sig_exc
#endif
	move.l	#_sigfpe,_sig_routine
	bra	Do_sig
_new_linef:
#ifndef ONLY030
	move.w	#0x2c,_sig_exc
#endif
	move.l	#_sigill,_sig_routine
	bra	Do_sig
_new_chk:
#ifndef ONLY030
	move.w	#0x18,_sig_exc
#endif
	move.l	#_sigfpe,_sig_routine
	bra	Do_sig
_new_trapv:
#ifndef ONLY030
	move.w	#0x1c,_sig_exc
#endif
	move.l	#_sigfpe,_sig_routine
	bra	Do_sig
_new_fpcp:
// don't set _sig_exc - only needed for 68000 vectors
	move.l	#_sigfpe,_sig_routine
	bra	Do_sig
_new_mmu:
// don't set _sig_exc - only needed for 68000 vectors
	move.l	#_sigill,_sig_routine
	bra	Do_sig
_new_pmmuacc:
// don't set _sig_exc - only needed for 68000 vectors
	move.l	#_sigbus,_sig_routine
	bra	Do_sig
_new_uninit:
#ifndef ONLY030
	move.w	#0x3c,_sig_exc
#endif
	move.l	#_sigbus,_sig_routine
	bra	Do_sig
_new_spurious:
#ifndef ONLY030
	move.w	#0x60,_sig_exc
#endif
	move.l	#_sigbus,_sig_routine
	bra	Do_sig
_new_format:
	move.l	#_haltformat,_sig_routine
	bra	Do_sig
_new_cpv:
	move.l	#_haltcpv,_sig_routine
	bra	Do_sig

	.globl	_old_priv		// old privilege violation vector
_new_priv:
#ifndef ONLY030
	move.w	#0x20,_sig_exc
#endif
	move.l	#_sigpriv,_sig_routine
#ifndef ONLY030
	tst.w	(0x59e).w		// 68000s always get SIGPRIV
	beq	Do_sig
#endif
	movem.l	d0/a0,-(sp)
	move.l	10(sp),a0		// fetch exception address
	move.w	(a0),d0
	and.w	#0xffc0,d0		// partially decode move sr,...
	cmp.w	#0x40c0,d0		// and test it
	movem.l	(sp)+,d0/a0		// preserves the flags
	bne	Do_sig		// doesn't look like sr,...
	move.l	_old_priv+8,-(sp)	// let our parent handle it
	rts

// system entry points from syscall.spp

	.globl	_mint_dos,_mint_bios,_mint_xbios
	.globl	_old_divzero,_old_chk,_old_trapv

_new_trace:
	btst	#5,(sp) 		// only check when called from supervisor mode
	beq.s	S_1
	cmp.l	#_mint_dos,2(sp)	// lets not trace the kernel !
	beq.s	S_2
	cmp.l	#_mint_xbios,2(sp)
	beq.s	S_2
	cmp.l	#_mint_bios,2(sp)
	beq.s	S_2
	cmp.l	#_old_divzero+12,2(sp)
	beq.s	S_2
	cmp.l	#_old_trapv+12,2(sp)
	beq.s	S_2
	cmp.l	#_old_chk+12,2(sp)
	beq.s	S_2

// add any other non-traceable entities here...

S_1:	move.w	#0x24,_sig_exc
	move.l	#_sigtrap,_sig_routine
	bra	Do_sig

S_2:	and.w	#0x3fff,(sp)		// clear both trace bits
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
	move.w	4(sp),d0		// check the drive
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	noalias0		// no
	move.w	d0,d1		// get index
	add.w	d0,d1		// convert to index
	lea	_aliasdrv,a0
	move.w	0(a0,d1.w),d1		// alias drive?
	beq.s	noalias0
	move.w	d1,d0
	subq.w	#1,d0		// adjust for aliasdrv base of '@'
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
	move.w	4(sp),d0		// check the drive
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	noalias1		// no
	move.w	d0,d1		// get index
	add.w	d0,d1		// convert to index
	lea	_aliasdrv,a0
	move.w	0(a0,d1.w),d1		// alias drive?
	beq.s	noalias1
	move.w	d1,d0
	subq.w	#1,d0		// adjust for aliasdrv base
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
	move.w	0xe(sp),d0		// check the drive
	cmp.w	#0x1f,d0 		// legal drive?
	bhi.s	noalias2		// no
	move.w	d0,d1		// get index
	add.w	d0,d1		// convert to index
	lea	_aliasdrv,a0
	move.w	0(a0,d1.w),d1		// alias drive?
	beq.s	noalias2
	move.w	d1,d0
	subq.w	#1,d0		// adjust for aliasdrv base
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
