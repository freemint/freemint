/*
 * $Id$
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1992 Eric R. Smith
 * Copyright 1992,1993,1994 Atari Corporation
 * All rights reserved.
 *
 *
 * Interface for system calls
 *
 * The following entry points are defined:
 *
 * _mint_bios:	entry point for the BIOS calls (trap #13)
 * _mint_xbios: entry point for XBIOS calls (trap #14)
 * _mint_dos:	entry point for GEMDOS calls (trap #1)
 * _lineA0:	calls the line A initialize routine
 * _call_aes:	calls the GEM AES
 * _do_usrcall: calls a user supplied function (e.g. for Supexec), with
 *              arguments supplied from global variables usrarg1, usrarg2, etc.
 * _callout:	calls an external function, after first saving all registers,
 *              and restores the registers afterward
 *
 * external variables referenced:
 *
 * _bios_tab, _bios_max:
 *    table of entry points for BIOS routines, max # of routines
 * _xbios_tab, _xbios_max:
 *    ditto for XBIOS
 * _dos_tab, _dos_max:
 *    ditto for GEMDOS
 * _curproc:
 *    pointer to current process table entry, and hence to save area for
 *    context (this is always the first entry in the PROC table).
 *
 * _bconbuf, _bconbsiz, _bconbdev:
 *    256 byte buffer for Bconout() output. If _bconbsiz is non-zero,
 *    there are that many bytes in _bconbuf waiting to be flushed. The
 *    output is for device _bconbdev.
 *
 * The function enter_gemdos() is called while entering
 * the kernel and leave_kernel() while leaving the kernel. These are
 * responsible for managing appropriate semaphores and pointers.
 *
 * we also call certain BIOS functions directly if these are known not to
 * require saving/restoring of context
 *
 * Best view in 800x600 ;-)
 *
 */

#include "magic/magic.i"

	.text

	.globl	_mint_bios,_mint_xbios
	.globl	_mint_dos
	.globl	_old_dos,_old_bios,_old_xbios
	.globl	_build_context
	.globl	_restore_context
	.globl	_proc_clock		// controls process' allocation of CPU time
	.globl	_enter_gemdos
	.globl	_leave_kernel
	.globl	_in_kernel
	.globl	_preempt
	.globl	_unwound_stack
	.globl	_check_sigs

	.globl	_curproc
	.globl	_bios_tab,_bios_max
	.globl	_xbios_tab,_xbios_max,_old_xbios
	.globl	_dos_tab,_dos_max

	.globl	_bconbuf,_bconbsiz,_bconbdev
	.globl	_bflush

	.globl	_sys_b_ubconstat,_do_bconin,_sys_b_ubcostat,_sys_b_kbshift

	.globl	_secure_mode,_falcontos

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_dos:
	dc.l	0
_mint_dos:

// NOTICE: MiNT *no longer ever* traps to ROM GEMDOS, because even
//         the TOSFS *jumps* (not traps) there. So these two lines
//         below only waste space and CPU time. And btw I really hate
//         doing same changes twice! (draco)

#ifndef SYSCALL_REENTRANT
	tst.w	_in_kernel
	bne.s	rom_gemdos
#endif
	clr.w	-(sp)			// no frame format needed
// NOTE: FOR NOW, WE PRESERVE A0 ACROSS GEMDOS CALLS. THIS WILL CHANGE
// SOMEDAY, DON'T RELY ON IT!!!
	move.l	_curproc,d0		// note: preserve all regs but d0
	addq.l	#4,d0			// for compatibility

	move.l	d0,-(sp)		// push pointer to syscall context save
	jsr	_build_context
	lea	_dos_tab,a5		// set syscall_tab
	move.w	_dos_max,d5		// set syscall_max
//
// copy parameters onto process stack. a0 and a1 were set by _build_context
//
	move.l	_curproc,a0
	move.l	(a0),sp 		// this puts us in our private stack
#ifdef NEW_SYSCALL
	move.w (a1),d0
	cmp.w	_dos_max,d0
	bcc	gemdos
// offset 0 is func pointer, 4 is nr of args, 6 is their size
nospr:	move.w	6(a5,d0.w*8),d0		// fetch size in bytes
	lsr.w	#1,d0			// we need size in words
cparg:	move.w	(a1,d0.w*2),-(sp)
	dbra.w	d0,cparg
#else
	move.l	32(a1),-(sp)		// a1 was set by build_context
	move.l	28(a1),-(sp)
	move.l	24(a1),-(sp)
	move.l	20(a1),-(sp)
	move.l	16(a1),-(sp)
	move.l	12(a1),-(sp)
	move.l	8(a1),-(sp)
	move.l	4(a1),-(sp)
	move.l	(a1),-(sp)
#endif
noargs:	jsr	_enter_gemdos		// set up vectors appropriately
#ifdef DEBUG_INFO
	cmpi.w	#4201,(sp)		// special case Sys_Printf
	bne.s	no_sys_printf
	clr.w	(sp)			// manipulate function number
	lea	_sys_mon_tab,a5		// tab
	move.w	#1,d5			// and max
no_sys_printf:
#endif
	bra	_syscall

#ifndef SYSCALL_REENTRANT		// see above
rom_gemdos:	move.l	_old_dos.w(pc),-(sp)
	rts
#endif

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_xbios:
	dc.l	0
_mint_xbios:

// Kludge for Setscreen: under Falcon TOS, this
// calls a GEMDOS function (Srealloc) and so
// we must *not* change any of the vectors!!
// Same kludge for Dsp_LoadProg() and Dsp_LodToBinary.
// They call GEMDOS *a lot* (Fgetdta, Fsetdta, Malloc, Mfree...)

	tst.w	_in_kernel
	bne	LX_rom
	btst	#5,(sp) 		// test for user/super mode
	beq.s	LX_usr
#ifdef ONLY030
	lea	8(sp),a1
#else
	lea	6(sp),a1		// supervisor mode: args on stack
	tst.w	(0x59e).w		// test longframe
	beq.s	LX_check
	addq.w	#2,a1			// stack is a bit bigger
#endif
	bra.s	LX_check
LX_usr:
	move.l	usp,a1			// user mode: args on user stack

// XBIOS is for root, not users - start checking access privileges.

LX_check:
	move.l	_curproc,a0
	cmp.w	#17,(a1)		// no need to forbid Random()
	beq.s	pok
	cmp.w	#21,(a1)		// Cursconf() gets converted to ioctl()
	beq.s	pok
	cmp.w	#38,(a1)		// Supexec() has own protection code
	beq.s	pok
	tst.w	_secure_mode		// the rest is subject to check
	beq.s	pok
	tst.w	P_EUID(a0)		// curproc->euid, is this enough?
	bne.s	ret
pok:
	tst.w	_falcontos		// on TOS 4.0 - 4.04...
	beq.s	sk
	cmp.w	#5,(a1) 		// ...check for Setscreen command
	beq.s	LX_rom			// no -- fall through

// DSP XBIOS is Falcon030 only - am I right?

	cmp.w	#108,(a1)		// Dsp_LoadProg()
	beq.s	LX_rom
	cmp.w	#111,(a1)		// Dsp_LodToBinary()
	beq.s	LX_rom

// _curproc is already in a0 here

sk:	clr.w	-(sp)			// no frame format needed
	pea	4(a0)			// push pointer to syscall context save
	jsr	_build_context
	lea	_xbios_tab,a5		// set syscall_tab
	move.w	_xbios_max,d5		// set syscall_max
//
// copy parameters onto process stack. a0 and a1 were set by _build_context
//
	move.l	_curproc,a0
	move.l	(a0),sp 		// this puts us in our private stack
	move.l	24(a1),-(sp)		// a1 was set by build_context
	move.l	20(a1),-(sp)
	move.l	16(a1),-(sp)
	move.l	12(a1),-(sp)
	move.l	8(a1),-(sp)
	move.l	4(a1),-(sp)
	move.l	(a1),-(sp)
	bset	#0x07,_in_kernel
	bra	_syscall

// For Dsp_LodToBinary(), Dsp_LoadProg()
// jump directly to the ROM vector -- this avoids all hazards.

LX_rom:	move.l	_old_xbios.w(pc),a0	// jump to ROM
	jmp	(a0)

rom_bios:
	move.l	_old_bios.w(pc),-(sp)
	rts

ret:	sub.l	a0,a0			// curproc address is our sweet secret
	moveq	#-36,d0 		// return EACCDN
	rte

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_bios:	dc.l	0
_mint_bios:
//
// Entering the kernel can be very expensive// so, we take a short-cut
// if possible -- we try some BIOS functions out, and if they
// succeed without blocking then we're done// otherwise, we go
// through the long procedure for entering the kernel
//
// These shortcuts aren't done when we're called in supervisor mode,
// because TOS uses very tiny stacks (smaller than we want)// the
// shortcuts operate on the user-supplied ssp, whereas the "full"
// BIOS code works on our (private) system stack
//
// the shortcuts are also turned off if BIOSBUF=n
//
	tst.w	_in_kernel
	bne.s	rom_bios
	bset	#0x07,_in_kernel
	tst.w	_bconbdev		// is BIOS buffering on?
	bmi	L_bios			// no// skip all this

	btst	#5,(sp) 		// test for user/super mode
	bne.s	L_bios			// if super, goto L_bios
	tst.w	_proc_clock		// are we about to be preempted?
	beq.s	L_bios

	move.l	usp,a1			// user mode: args on user stack
L_ubios:
	move.w	(a1),d0 		// get command
	cmp.w	#3,d0			// Bconout?
	beq	do_bconout		// yes -- go do it
//
// most of the remaining functions require BIOS vectors to be properly
// set up
	tst.w	_bconbsiz		// is BIOS output waiting?
	bne.s	L_bios			// yes -- do regular code

// test for various BIOS functions
	cmp.w	#1,d0			// Bconstat?
	bne.s	L_00
	move.w	2(a1),-(sp)		// push arg
	jsr	_sys_b_ubconstat
L_1out:
	addq.l	#2,sp
L_0out:
	ori.w	#0x0400,sr		// spl7()
	jsr	_leave_kernel		// does not change d0
	rte				// return to user
L_00:
	cmp.w	#2,d0			// Bconin?
	bne.s	L_01
	move.w	2(a1),-(sp)		// yes; push argument
	jsr	_do_bconin
	addq.w	#2,sp
	cmp.w	#0xdead,d0		// would Bconin block?
	bne.s	L_0out			// no -- we're done
	bra.s	L_bios			// yes -- do the long stuff
L_01:
	cmp.w	#8,d0			// Bcostat?
	bne.s	L_02
	move.w	2(a1),-(sp)		// push device
	jsr	_sys_b_ubcostat		// get status
	bra.s	L_1out
L_02:
	cmp.w	#11,d0			// Kbshift?
	bne.s	L_bios
	move.w	2(a1),-(sp)		// push arg
	jsr	_sys_b_kbshift
	bra.s	L_1out
L_bios:
	clr.w	-(sp)			// no frame format needed
	move.l	_curproc,a0
	pea	4(a0)			// push pointer to syscall context save
	jsr	_build_context
	lea	_bios_tab,a5		// set syscall_tab
	move.w	_bios_max,d5		// set syscall_max
//
// copy parameters onto process stack. a0 and a1 were set by _build_context
//
	move.l	_curproc,a0
	move.l	(a0),sp 		// this puts us in our private stack
	move.l	24(a1),-(sp)		// a1 was set by build_context
	move.l	20(a1),-(sp)
	move.l	16(a1),-(sp)
	move.l	12(a1),-(sp)
	move.l	8(a1),-(sp)
	move.l	4(a1),-(sp)
	move.l	(a1),-(sp)

_syscall:
//
// check here to see if we need to flush the Bconout() buffer
//
	tst.w	_bconbsiz		// characters in buffer?
	beq.s	L_noflush		// no: OK to proceed
//
// watch out, this could cause a context switch
//
	jsr	_bflush 		// flush the buffer

L_noflush:
//
// figure out which routine to call
//
	move.w	(sp),d0
	cmp.w	d5,d0
	bcc	error
#ifdef ONLY030
#ifdef NEW_SYSCALL
	move.l	0(a5,d0.w*8),d0
#else
	move.l	0(a5,d0.w*4),d0 	// d0 = syscall_tab[d0]
#endif
#else
#ifdef NEW_SYSCALL
	add.w	d0,d0
#endif
	add.w	d0,d0
	add.w	d0,d0			// multiply by 4
	move.l	0(a5,d0.w),d0		// d0 = syscall_tab[d0]
#endif
	beq	error			// null entry means invalid call
	addq.w	#2,sp			// pop function number off stack
	move.l	d0,a0
	jsr	(a0)			// go do the call
out:
	move.l	_curproc,a0
	move.l	d0,P_SYSCTXT+C_D0(a0)	// set d0 in the saved context
	move.w	P_SYSCTXT+C_SR(a0),d0	// get saved status register

	tst.l	P_PTRACER(a0)		// check curproc->ptracer, if not set
	beq.s	notrace 		// then no pending trace// this ensures
	move.w	d0,d1			// we work with non-MiNT debuggers
	and.w	#0xc000,d1		// are either of the trace bits set
	sne	P_SYSCTXT+C_PTRACE(a0)	// mark as trace pending/not
notrace:
	tst.w	_proc_clock		// has process exceeded time slice?
	bne.s	nosleep 		// no -- continue
	btst	#13,d0			// caller in supervisor mode?
	bne.s	nosleep 		// yes -- don't interrupt
	tst.w	(0x43e).w		// test floppy disk lock variable
	bne.s	nosleep 		// if locked, can't switch
sleep:
	tst.l	_unwound_stack		// did we unwind sysstack?
	beq.s	noreload1
	move.l	_curproc,a0		// then reload it before
	move.l	(a0),sp 		// doing anything further
noreload1:
	jsr	_preempt		// does a sleep(READY_Q)
	bra.s	nosig

nosleep:
	move.l	P_SIGMASK(a0),d0	// any unmasked signals left pendig?
	not.l	d0
	and.l	P_SIGPENDING(a0),d0
	beq.s	nosig
	tst.l	_unwound_stack		// did we unwind sysstack?
	beq.s	noreload2
	move.l	_curproc,a0		// then reload it before
	move.l	(a0),sp 		// doing anything further
noreload2:
	jsr	_check_sigs
nosig:
	ori.w	#0x0400,sr		// spl7()
	clr.l	_unwound_stack
	jsr	_leave_kernel		// restore vectors (loads a0)
	pea	4(a0)
	jsr	_restore_context	// never returns

//
// we handle errors by calling through to GEMDOS or the BIOS/XBIOS,
// as appropriate, and letting them handle it -- that way, if the underlying
// system has functions we don't know about, they still work
// to figure out which trap we have to call, we use the system call
// table placed in a5 earlier

error:	cmp.l	#_xbios_tab,a5
	bne.s	enosys
	trap	#14
	bra	out
enosys:	moveq	#-32,d0	// ENOSYS
	bra	out

// Trap wrapper for calling ROM from the inside of the kernel.
// Usage: just replace trap #1 with a jsr _trap_1_emu etc.

#ifdef SYSCALL_REENTRANT

	.globl	_trap_1_emu
#ifdef NOT_YET
	.globl	_trap_13_emu
	.globl	_trap_14_emu
#endif

	TEXT

_trap_1_emu:
#ifndef ONLY030
	tst.w	(0x059e).w		// test longframe
	beq.s	TE_D
#endif
	move.l	(sp)+,d0
	clr.w	-(sp)			// frame format word
	move.l	d0,-(sp)		// return address
TE_D:	move.w	sr,-(sp)
	move.l	_old_dos.w(pc),-(sp)
	rts

#ifdef NOT_YET
_trap_13_emu:
#ifndef ONLY030
	tst.w	(0x059e).w		// test longframe
	beq.s	TE_B
#endif
	move.l	(sp)+,d0
	clr.w	-(sp)			// frame format word
	move.l	d0,-(sp)		// return address
TE_B:	move.w	sr,-(sp)
	move.l	_old_bios.w(pc),-(sp)
	rts

_trap_14_emu:
#ifndef ONLY030
	tst.w	(0x059e).w		// test longframe
	beq.s	TE_X
#endif
	move.l	(sp)+,d0
	clr.w	-(sp)			// frame format word
	move.l	d0,-(sp)		// return address
TE_X:	move.w	sr,-(sp)
	move.l	_old_xbios.w(pc),-(sp)
	rts
#endif
#endif					// SYSCALL_REENTRANT

// Default routine for "unused" traps. Without this, any user (even
// on remote) can draw bombs on the console freely. So this is sorta
// security patch.
// Notice any programs those take these traps should now be loaded
// *after* MiNT. init_intr() decides which traps to take, see there.

	.globl	_unused_trap

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
	dc.l	v_rte

_unused_trap:	move.w	#0x010b,-(sp)		// getpid
	trap	#1
	move.w	#0x000c,-(sp)		// SIGSYS
	move.w	d0,-(sp)		// himself
	move.w	#0x0111,-(sp)		// kill
	trap	#1
	addq.l	#0x08,sp
v_rte:	rte				// and return :-)

//
// bconout special code: on entry, a1 points to the stack the user
// was using. If possible, we just buffer the output until later.
//

do_bconout:
	tst.w	_bconbdev		// is BIOS buffering on?
	bmi	L_bios			// no buffering -- skip this code
	move.w	2(a1),d0		// what device is this for?
	beq	L_bios			// don't buffer the printer
	cmp.w	_bconbdev,d0		// same device as is buffered?
	bne.s	new_dev 		// no -- maybe we can't do this
put_buf:
	move.w	4(a1),d0		// get the character to output
	move.w	_bconbsiz,d1		// get index into buffer table
	cmp.w	#255,d1 		// buffer full?
	beq	L_bios			// yes -- flush it out
	lea	_bconbuf,a0
	add.w	d1,a0
	move.b	d0,(a0) 		// store the character
	addq.w	#1,d1
	move.w	d1,_bconbsiz
	ori.w	#0x0400,sr		// spl7()
	jsr	_leave_kernel		// restore vectors
	moveq.l	#-1,d0			// return character output OK
	rte

new_dev:
	tst.w	_bconbsiz		// characters already in buffer?
	bne	L_bios			// yes: we can't buffer this one
	move.w	d0,_bconbdev		// no: OK, we have a new device
	bra.s	put_buf

//
// _lineA0: MiNT calls this to get the address of the line A variables
//
	.globl	_lineA0

_lineA0:
	movem.l	d2/a2,-(sp)		// save scratch registers
	dc.w	0xa000			// call the line A initialization routine
	movem.l	(sp)+,d2/a2
	rts

//
// _call_aes: calls the GEM AES, using the control block passed as
// a parameter. Used only for doing appl_init(), to see
// if the AES is active yet
//
	.globl	_call_aes

_call_aes:
	move.l	4(sp),d1		// fetch pointer to parameter block
	move.l	#200,d0 		// magic number for AES
	movem.l	d2/a2,-(sp)		// save scratch registers
	trap	#2
	movem.l	(sp)+,d2/a2
	rts

//
// _callout: Call an external function, passing <32 bytes of arguments,
// and return the value from the function. NOTE: we must be careful
// to save all registers here!
//
	.globl	_callout
	.globl	_callout1
	.globl	_callout2
	.globl	_callout6
	.globl	_callout6spl7
//
// _callout is the general purpose one
//
_callout:
	lea	8(sp),a0		// pointer to args
	move.l	4(sp),a1		// pointer to pointer to function
	movem.l	d2-d7/a2-a6,-(sp)	// save registers
	movem.l	(a0),d0-d7		// copy parameters
	movem.l	d0-d7,-(sp)
	suba.l	a5,a5			// the BIOS expects 0 in a5
	jsr	(a1)			// go do it
	lea	32(sp),sp
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
	rts
//
// _callout2 and _callout1 are for functions with just 1 or
// 2 16 bit parameters. We cheat and use the same code for
// both, since passing 32 bits isn't much more expensive than
// passing 16 bits (and since the called function will just
// ignore any extra arg)
//

_callout1:
_callout2:
	movem.l	4(sp),a0/a1		// get function ptr & args
	movem.l	d2-d7/a2-a6,-(sp)	// save reggies
	move.l	a1,-(sp)		// push args
	suba.l	a5,a5			// the BIOS expects 0 in a5
	jsr	(a0)			// do function
	addq.w	#4,sp
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
	rts
//
// _callout6 passes 6 words, saving sr
//
_callout6:
	movem.l	d2-d7/a2-a6,-(sp)	// save registers
	movem.l	4+44(sp),a0-a3		// get function ptr & args
	move.w	sr,-(sp)
	movem.l	a1-a3,-(sp)		// copy args
	suba.l	a5,a5			// the BIOS expects 0 in a5
	jsr	(a0)			// go do it
	lea	12(sp),sp
	move.w	(sp)+,sr
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
	rts
//
// _callout6spl7 passes 6 words at ipl7
//
_callout6spl7:
	movem.l	d2-d7/a2-a6,-(sp)	// save registers
	movem.l	4+44(sp),a0-a3		// get function ptr & args
	move.w	sr,-(sp)
	movem.l	a1-a3,-(sp)		// copy args
	suba.l	a5,a5			// the BIOS expects 0 in a5
	ori.w	#0x0700,sr		// spl7()
	jsr	(a0)			// go do it
	lea	12(sp),sp
	move.w	(sp)+,sr
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
	rts

//
// do_usrcall: call the user supplied function (*usrcall)(), with
// arguments given in the longwords usrarg1..usrarg5. Return value
// is placed in usrret. This function is used by the Supexec code
// in xbios.c.

	.globl	_do_usrcall
	.globl	_usrret
	.globl	_usrcall
	.globl	_usrarg1
	.globl	_usrarg2
	.globl	_usrarg3
	.globl	_usrarg4
	.globl	_usrarg5

_do_usrcall:
	move.l	_usrarg5,-(sp)
	move.l	_usrarg4,-(sp)
	move.l	_usrarg3,-(sp)
	move.l	_usrarg2,-(sp)
	move.l	_usrarg1,-(sp)
	move.l	_usrcall,-(sp)		// the user expects to see this on the stack
	pea	ucret(pc)		// so rts puts us back here
	move.l	_usrcall,-(sp)		// this copy is for us to use
	rts				// to jump to (we don't want to use registers)
ucret:	lea	24(sp),sp		// fix up stack
	move.l	d0,_usrret		// save returned value
	rts				// back to caller
