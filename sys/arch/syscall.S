/*
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
 * mint_dos:	entry point for GEMDOS calls (trap #1)
 * mint_bios:	entry point for BIOS calls (trap #13)
 * mint_xbios: entry point for XBIOS calls (trap #14)
 * _lineA0:	calls the line A initialize routine
 * _call_aes:	calls the GEM AES
 * _do_usrcall: calls a user supplied function (e.g. for Supexec), with
 *              arguments supplied from global variables usrarg1, usrarg2, etc.
 * _callout:	calls an external function, after first saving all registers,
 *              and restores the registers afterward
 *
 * external variables referenced:
 *
 * bios_tab, BIOS_MAX:
 *    table of entry points for BIOS routines
 * xbios_tab, XBIOS_MAX:
 *    ditto for XBIOS
 * dos_tab, DOS_MAX:
 *    ditto for GEMDOS
 * curproc:
 *    pointer to current process table entry, and hence to save area for
 *    context (this is always the first entry in the PROC table).
 *
 * bconbuf, bconbsiz, bconbdev:
 *    256 byte buffer for Bconout() output. If _bconbsiz is non-zero,
 *    there are that many bytes in _bconbuf waiting to be flushed. The
 *    output is for device bconbdev.
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

#ifndef __mcoldfire__
	.cpu	68030
	.arch	68030
#endif

#include "magic/magic.i"
#include "mint/asmdefs.h"

	.text

	.globl	SYM(mint_bios)
	.globl	SYM(mint_xbios)
	.globl	SYM(mint_dos)
	.globl	SYM(old_dos),SYM(old_bios),SYM(old_xbios)
	.extern	SYM(build_context)
	.extern	SYM(restore_context)
	.extern	SYM(proc_clock)		// controls process' allocation of CPU time
	.extern	SYM(enter_gemdos)
	.extern	SYM(leave_kernel)
	.extern	SYM(in_kernel)
	.extern	SYM(preempt)
	.extern	SYM(unwound_stack)
	.globl	SYM(check_sigs)

	.globl	SYM(old_trap2)
	.globl	SYM(mint_trap2)
	.globl	SYM(gdos_version),SYM(vdi_handler),SYM(aes_handler)

	.extern	SYM(curproc)
	.globl	SYM(bios_tab)
	.globl	SYM(xbios_tab),SYM(old_xbios)
	.globl	SYM(dos_tab)

	.extern	SYM(bconbuf),SYM(bconbsiz),SYM(bconbdev)
	.extern	SYM(bflush)

	.extern	SYM(sys_b_ubconstat),SYM(do_bconin),SYM(sys_b_ubcostat),SYM(sys_b_kbshift)

	.extern	SYM(secure_mode)
	.extern SYM(falcontos)
	.extern SYM(fpu)
	.extern SYM(mcpu)

	.extern	SYM(stop)

	BIOS_MAX = 0x20
	XBIOS_MAX = 0x80
	DOS_MAX = 0x190

// ***********************
// * GEMDOS trap handler *
// ***********************
//
// NOTICE: MiNT *no longer ever* traps to ROM GEMDOS, because even
//         the TOSFS *jumps* (not traps) there. So these two lines
//         below only waste space and CPU time. And btw I really hate
//         doing same changes twice! (draco)

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
SYM(old_dos):
	dc.l	0
SYM(mint_dos):

//
// see at symbol reentrant_trap for a detailed explanation
//
	tst.w	SYM(in_kernel)
	beq.s	norm_gemdos

	movea.l	sp,a0
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)
#else
	movem.l	d2-d7/a2-a6,-(sp)
#endif
	lea	SYM(dos_tab),a5
	move.w	#DOS_MAX,d5
	bra	reentrant_trap
norm_gemdos:
	clr.w	-(sp)			// no frame format needed
// NOTE: FOR NOW, WE PRESERVE A0 ACROSS GEMDOS CALLS. THIS WILL CHANGE
// SOMEDAY, DON'T RELY ON IT!!!
	move.l	SYM(curproc),-(sp)		// push pointer to syscall context save
	addq.l	#P_SYSCTXT,(sp)

	jsr	SYM(build_context)		// build_context sets A1 to stack used before we get here
	lea	SYM(dos_tab),a5		// set syscall_tab
	move.w	#DOS_MAX,d5		// set syscall_max
//
// copy parameters onto process stack. a0 and a1 were set by build_context
//
	move.l	SYM(curproc),a0
	move.l	(a0),sp 		// this puts us in our private stack
#ifdef NEW_SYSCALL
#ifdef __mcoldfire__
	mvz.w  (a1),d1
#else
	move.w (a1),d1
#endif
	cmp.w	#DOS_MAX,d1
	bcc	gemdos
// offset 0 is func pointer, 4 is nr of args, 6 is their size
nospr:
#ifdef __mcoldfire__
	move.w	6(a5,d1.l*8),d1		// fetch size in bytes
	lsr.l	#1,d1			// we need size in words
cparg:	move.w	(a1,d1.l*2),-(sp)
	subq.l	#1,d1
	bpl.s	cparg
#else
	move.w	6(a5,d1.w*8),d1		// fetch size in bytes
	lsr.w	#1,d1			// we need size in words
cparg:	move.w	(a1,d1.w*2),-(sp)
	dbra.w	d1,cparg
#endif
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
noargs:	jsr	SYM(enter_gemdos)		// set up vectors appropriately
#ifdef DEBUG_INFO
#ifdef __mcoldfire__
	move.w	#4201,d1
	cmp.w	(sp),d1			// special case Sys_Printf
#else
	cmpi.w	#4201,(sp)		// special case Sys_Printf
#endif
	bne.s	no_sys_printf
	clr.w	(sp)			// manipulate function number
	lea	sys_mon_tab(pc),a5		// tab
	move.w	#1,d5			// and max
no_sys_printf:
#endif
	bra	_syscall

sys_mon_tab:
	.dc.l	SYM(Debug)

// **********************
// * XBIOS trap handler *
// **********************
//
// Kludge for Setscreen: under Falcon TOS, this
// calls a GEMDOS function (Srealloc) and so
// we must *not* change any of the vectors!!
// Same kludge for Dsp_LoadProg() and Dsp_LodToBinary.
// They call GEMDOS *a lot* (Fgetdta, Fsetdta, Malloc, Mfree...)

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
SYM(old_xbios):
	dc.l	0
SYM(mint_xbios):

//
// see at symbol reentrant_trap for a detailed explanation
//
	tst.w	SYM(in_kernel)
	beq	norm_xbios

	movea.l	sp,a0
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)
#else
	movem.l	d2-d7/a2-a6,-(sp)
#endif
	lea	SYM(xbios_tab),a5
	move.w	#XBIOS_MAX,d5
	bra	reentrant_trap

norm_xbios:
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	LX_68k

	btst	#5,2(sp) 		// test for user/super mode
	beq.s	LX_usr

	move.w	(sp),d1			// format/vector word
	andi.l	#0x3000,d1		// keep the filler size bits
	beq.s	LX_stack_fixed
	moveq	#2,d1			// assume 2-byte filler after the exception frame
LX_stack_fixed:
	lea	8(sp,d1.l),a1
	bra.s	LX_check
LX_68k:
#endif
	btst	#5,(sp) 		// test for user/super mode
	beq.s	LX_usr
#ifndef M68000
	lea	8(sp),a1
#else
	lea	6(sp),a1		// supervisor mode: args on stack
	tst.w	(0x59e).w		// test longframe
	beq.s	LX_check
	addq.l	#2,a1			// stack is a bit bigger
#endif
	bra.s	LX_check
LX_usr:
	move.l	%usp,a1			// user mode: args on user stack

// XBIOS is for root, not users - start checking access privileges.

LX_check:
	move.l	SYM(curproc),a0
#ifdef __mcoldfire__
	moveq	#17,d1
	cmp.w	(a1),d1			// no need to forbid Random()
#else
	cmp.w	#17,(a1)		// no need to forbid Random()
#endif
	beq.s	pok
#ifdef __mcoldfire__
	moveq	#21,d1
	cmp.w	(a1),d1			// Cursconf() gets converted to ioctl()
#else
	cmp.w	#21,(a1)		// Cursconf() gets converted to ioctl()
#endif
	beq.s	pok
#ifdef __mcoldfire__
	moveq	#38,d1
	cmp.w	(a1),d1			// Supexec() has own protection code
#else
	cmp.w	#38,(a1)		// Supexec() has own protection code
#endif
	beq.s	pok
	tst.w	SYM(secure_mode)		// the rest is subject to check
	beq.s	pok
	tst.w	P_EUID(a0)		// curproc->euid, is this enough?
	bne.s	ret
pok:
#ifndef M68000
	tst.w	SYM(falcontos)		// on TOS 4.0 - 4.04...
	beq.s	sk
#ifdef __mcoldfire__
	moveq	#5,d1
	cmp.w	(a1),d1 		// ...check for Setscreen command
#else
	cmp.w	#5,(a1) 		// ...check for Setscreen command
#endif
	beq.s	LX_rom			// no -- fall through

// DSP XBIOS is Falcon030 only - am I right?

#ifdef __mcoldfire__
	moveq	#108,d1
	cmp.w	(a1),d1			// Dsp_LoadProg()
#else
	cmp.w	#108,(a1)		// Dsp_LoadProg()
#endif
	beq.s	LX_rom
#ifdef __mcoldfire__
	moveq	#111,d1
	cmp.w	(a1),d1			// Dsp_LodToBinary()
#else
	cmp.w	#111,(a1)		// Dsp_LodToBinary()
#endif
	beq.s	LX_rom
#endif
// curproc is already in a0 here

sk:
#ifdef __mcoldfire__
	moveq	#0x7,d0
	bset	d0,SYM(in_kernel)
#else
	bset	#0x7,SYM(in_kernel)
#endif
	clr.w	-(sp)			// no frame format needed
	pea	P_SYSCTXT(a0)		// push pointer to syscall context save
	jsr	SYM(build_context)
	lea	SYM(xbios_tab),a5		// set syscall_tab
	move.w	#XBIOS_MAX,d5		// set syscall_max

//
// copy parameters onto process stack. a0 and a1 were set by build_context
//
	move.l	SYM(curproc),a0
	move.l	(a0),sp 		// this puts us in our private stack
	move.l	24(a1),-(sp)		// a1 was set by build_context
	move.l	20(a1),-(sp)
	move.l	16(a1),-(sp)
	move.l	12(a1),-(sp)
	move.l	8(a1),-(sp)
	move.l	4(a1),-(sp)
	move.l	(a1),-(sp)
	bra	_syscall

// For Dsp_LodToBinary(), Dsp_LoadProg()
// jump directly to the ROM vector -- this avoids all hazards.
#ifndef M68000
LX_rom:	move.l	SYM(old_xbios).w(pc),a0	// jump to ROM
	jmp	(a0)
#endif

ret:	sub.l	a0,a0			// curproc address is our sweet secret
	moveq	#-36,d0 		// return EACCDN
	rte

// *********************
// * BIOS trap handler *
// *********************
//
// Entering the kernel can be very expensive so, we take a short-cut
// if possible -- we try some BIOS functions out, and if they
// succeed without blocking then we're done otherwise, we go
// through the long procedure for entering the kernel
//
// These shortcuts aren't done when we're called in supervisor mode,
// because TOS uses very tiny stacks (smaller than we want) the
// shortcuts operate on the user-supplied ssp, whereas the "full"
// BIOS code works on our (private) system stack
//
// the shortcuts are also turned off if BIOSBUF=n

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
SYM(old_bios):	dc.l	0
SYM(mint_bios):

//
// see at symbol reentrant_trap for a detailed explanation
//
	tst.w	SYM(in_kernel)
	beq	norm_bios

	movea.l	sp,a0
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)
#else
	movem.l	d2-d7/a2-a6,-(sp)
#endif
	lea	SYM(bios_tab),a5
	move.w	#BIOS_MAX,d5
	bra	reentrant_trap

norm_bios:
#ifdef __mcoldfire__
	moveq	#0x7,d0
	bset	d0,SYM(in_kernel)
#else
	bset	#0x7,SYM(in_kernel)
#endif
	tst.w	SYM(bconbdev)		// is BIOS buffering on?
	bmi	L_bios			// no, skip all this

#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	L_68k

	btst	#5,2(sp) 		// test for user/super mode
	bne	L_bios			// if super, goto L_bios

	bra.s	L_user
L_68k:
#endif
	btst	#5,(sp) 		// test for user/super mode
	bne.s	L_bios			// if super, goto L_bios
L_user:
	tst.w	SYM(proc_clock)		// are we about to be preempted?
	beq.s	L_bios

	move.l	%usp,a1			// user mode: args on user stack
L_ubios:
	move.w	(a1),d0 		// get command
	cmp.w	#3,d0			// Bconout?
	beq	do_bconout		// yes -- go do it
//
// most of the remaining functions require BIOS vectors to be properly
// set up
	tst.w	SYM(bconbsiz)		// is BIOS output waiting?
	bne.s	L_bios			// yes -- do regular code

// test for various BIOS functions
	cmp.w	#1,d0			// Bconstat?
	bne.s	L_00
	move.w	2(a1),-(sp)		// push arg
	jsr	SYM(sys_b_ubconstat)
L_1out:
	addq.l	#2,sp
L_0out:
#ifdef __mcoldfire__
	move.w	sr,d1
	ori.l	#0x0400,d1		// spl7()
	move.w	d1,sr
#else
	ori.w	#0x0400,sr		// spl7()
#endif
	jsr	SYM(leave_kernel)		// does not change d0
	rte				// return to user
L_00:
	cmp.w	#2,d0			// Bconin?
	bne.s	L_01
	move.w	2(a1),-(sp)		// yes; push argument
	jsr	SYM(do_bconin)
	addq.l	#2,sp
	cmp.w	#0xdead,d0		// would Bconin block?
	bne.s	L_0out			// no -- we're done
	bra.s	L_bios			// yes -- do the long stuff
L_01:
	cmp.w	#8,d0			// Bcostat?
	bne.s	L_02
	move.w	2(a1),-(sp)		// push device
	jsr	SYM(sys_b_ubcostat)		// get status
	bra.s	L_1out
L_02:
	cmp.w	#11,d0			// Kbshift?
	bne.s	L_bios
	move.w	2(a1),-(sp)		// push arg
	jsr	SYM(sys_b_kbshift)
	bra.s	L_1out
L_bios:
	clr.w	-(sp)			// no frame format needed
	move.l	SYM(curproc),a0
	pea	P_SYSCTXT(a0)			// push pointer to syscall context save
	jsr	SYM(build_context)
	lea	SYM(bios_tab),a5		// set syscall_tab
	move.w	#BIOS_MAX,d5		// set syscall_max
//
// copy parameters onto process stack. a0 and a1 were set by build_context
//
	move.l	SYM(curproc),a0
	move.l	(a0),sp 		// this puts us in our private stack
	move.l	24(a1),-(sp)		// a1 was set by build_context
	move.l	20(a1),-(sp)
	move.l	16(a1),-(sp)
	move.l	12(a1),-(sp)
	move.l	8(a1),-(sp)
	move.l	4(a1),-(sp)
	move.l	(a1),-(sp)

_syscall:
// Ozk: From now on we preserve cpu register D0.. it seems to be important
//      for XBIOS, since some functions return shorts while other returns
//      longs. And apps apparently have ways of checking this...
//

//
// check here to see if we need to flush the Bconout() buffer
//
	tst.w	SYM(bconbsiz)		// characters in buffer?
	beq.s	L_noflush		// no: OK to proceed
//
// watch out, this could cause a context switch
//
	jsr	SYM(bflush) 		// flush the buffer

L_noflush:

// Code for ptrace mode PT_SYSCALL
	move.l	SYM(curproc),a0
	tst.l	P_PTRACER(a0)		// is someone tracing us?
	beq.s	pt_ende
	tst.w	P_PTRACEFLAGS(a0)	// are system calls being traced?
	beq.s	pt_ende
	move.w	#5,-(sp)		// move.w #SIGTRAP,-(sp)
	jsr	SYM(stop)			// send SIGTRAP to the process
	addq.l	#2,sp
pt_ende:
// end of code for ptrace mode PT_SYSCALL


//
// figure out which routine to call
//
#ifdef __mcoldfire__
	mvz.w	(sp),d1
#else
	move.w	(sp),d1
#endif
	cmp.w	d5,d1
	bcc	error
#ifndef M68000
#ifdef NEW_SYSCALL
#ifdef __mcoldfire__
	move.l	0(a5,d1.l*8),d1
#else
	move.l	0(a5,d1.w*8),d1
#endif
#else
#ifdef __mcoldfire__
	move.l	0(a5,d1.l*4),d1 	// d0 = syscall_tab[d0]
#else
	move.l	0(a5,d1.w*4),d1 	// d0 = syscall_tab[d0]
#endif
#endif
#else
#ifdef NEW_SYSCALL
	add.w	d1,d1
#endif
	add.w	d1,d1
	add.w	d1,d1			// multiply by 4
	move.l	0(a5,d1.w),d1		// d0 = syscall_tab[d0]
#endif
	beq	error			// null entry means invalid call
	addq.l	#2,sp			// pop function number off stack
	move.l	d1,a0
	jsr	(a0)			// go do the call

out:	moveq	#0,d2
out_select:
	move.l	SYM(curproc),a0
	move.l	d0,P_SYSCTXT+C_D0(a0)	// set d0 in the saved context
	move.w	P_SYSCTXT+C_SR(a0),d0	// get saved status register

	tst.l	P_PTRACER(a0)		// check curproc->ptracer, if not set
	beq.s	notrace 		// then no pending trace this ensures
	move.w	d0,d1			// we work with non-MiNT debuggers
#ifdef __mcoldfire__
	and.l	#0xc000,d1		// are either of the trace bits set
	sne	d1
	move.b	d1,P_SYSCTXT+C_PTRACE(a0) // mark as trace pending/not
#else
	and.w	#0xc000,d1		// are either of the trace bits set
	sne	P_SYSCTXT+C_PTRACE(a0)	// mark as trace pending/not
#endif
notrace:
	tst.w	d2			// If D2 set, dont preemtp
	bne.s	nosleep
	tst.w	SYM(proc_clock)		// has process exceeded time slice?
	bne.s	nosleep 		// no -- continue
	btst	#13,d0			// caller in supervisor mode?
	bne.s	nosleep 		// yes -- don't interrupt
	tst.w	(0x43e).w		// test floppy disk lock variable
	bne.s	nosleep 		// if locked, can't switch
sleep:
	tst.l	SYM(unwound_stack)		// did we unwind sysstack?
	beq.s	noreload1
	move.l	SYM(curproc),a0		// then reload it before
	move.l	(a0),sp 		// doing anything further
noreload1:
	jsr	SYM(preempt)		// does a sleep(READY_Q)
	bra.s	nosig

nosleep:
	move.l	P_SIGMASK(a0),d0	// any unmasked signals left pendig?
	not.l	d0
	and.l	P_SIGPENDING(a0),d0
	beq.s	nosig
	tst.l	SYM(unwound_stack)		// did we unwind sysstack?
	beq.s	noreload2
	move.l	SYM(curproc),a0		// then reload it before
	move.l	(a0),sp 		// doing anything further
noreload2:
	jsr	SYM(check_sigs)
nosig:
#ifdef __mcoldfire__
	move.w	sr,d1
	ori.l	#0x0400,d1		// spl7()
	move.w	d1,sr
#else
	ori.w	#0x0400,sr		// spl7()
#endif
	clr.l	SYM(unwound_stack)
	jsr	SYM(leave_kernel)		// restore vectors (loads a0)
	pea	P_SYSCTXT(a0)
	jsr	SYM(restore_context)	// never returns

rom_bios:
	move.l	SYM(old_bios).w(pc),-(sp)
	rts

//
// bconout special code: on entry, a1 points to the stack the user
// was using. If possible, we just buffer the output until later.
//

do_bconout:
	tst.w	SYM(bconbdev)		// is BIOS buffering on?
	bmi	L_bios			// no buffering -- skip this code
	move.w	2(a1),d0		// what device is this for?
	beq	L_bios			// don't buffer the printer
	cmp.w	SYM(bconbdev),d0		// same device as is buffered?
	bne.s	new_dev 		// no -- maybe we can't do this
put_buf:
	move.w	4(a1),d0		// get the character to output
#ifdef __mcoldfire__
	mvz.w	SYM(bconbsiz),d1		// get index into buffer table
#else
	move.w	SYM(bconbsiz),d1		// get index into buffer table
#endif
	cmp.w	#255,d1 		// buffer full?
	beq	L_bios			// yes -- flush it out
	lea	SYM(bconbuf),a0
#ifdef __mcoldfire__
	add.l	d1,a0
#else
	add.w	d1,a0
#endif
	move.b	d0,(a0) 		// store the character
#ifdef __mcoldfire__
	addq.l	#1,d1
#else
	addq.w	#1,d1
#endif
	move.w	d1,SYM(bconbsiz)
#ifdef __mcoldfire__
	move.w	sr,d1
	ori.l	#0x0400,d1		// spl7()
	move.w	d1,sr
#else
	ori.w	#0x0400,sr		// spl7()
#endif
	jsr	SYM(leave_kernel)		// restore vectors
	moveq.l	#-1,d0			// return character output OK
	rte

new_dev:
	tst.w	SYM(bconbsiz)		// characters already in buffer?
	bne	L_bios			// yes: we can't buffer this one
	move.w	d0,SYM(bconbdev)		// no: OK, we have a new device
	bra.s	put_buf

//
// we handle errors by calling through to GEMDOS or the BIOS/XBIOS,
// as appropriate, and letting them handle it -- that way, if the underlying
// system has functions we don't know about, they still work
// to figure out which trap we have to call, we use the system call
// table placed in a5 earlier

error:	cmp.l	#SYM(xbios_tab),a5
	bne.s	enosys
	jsr	SYM(trap_14_emu)
	bra	out
enosys:	moveq	#-32,d0		// ENOSYS
	bra	out

SYM(vdi_handler):
	dc.l	0
SYM(aes_handler):
	dc.l	0
SYM(gdos_version):
	dc.l	0

// ***********************
// * AES/VDI trap handler *
// ***********************
//
// The VDI intitialization needs to put the GDOS version at symbol
// gdos_version, because this trap handler returns what is at that
// symbol when a handler is installed in vdi_handler
//
	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
SYM(old_trap2):
	dc.l	0
SYM(mint_trap2):

//
// see at symbol reentrant_trap for a detailed explanation
//
	tst.w	SYM(in_kernel)
	beq.s	norm_trap2

#ifdef __mcoldfire__
	lea	-56(sp),sp
	movem.l	d1-d7/a0-a6,(sp)
#else
	movem.l	d1-d7/a0-a6,-(sp)
#endif
	lea	((7+7)*4)(sp),a0
	bra	reentrant_trap2

norm_trap2:
#ifdef M68000
	bset	#0x07,SYM(in_kernel)	// speed optimization: 'in_vdi' is not used for the 000 build
#else
	move.l	a0,-(sp)		// backup a0

#ifdef __mcoldfire__
	lea	SYM(in_kernel),a0
	bset	#0x07,(a0)
#else
	bset	#0x07,SYM(in_kernel)
#endif
	move.l	SYM(curproc),a0
	move.w	#0x0001,P_INVDI(a0)

	move.l	(sp)+,a0		// restore a0
#endif	// M68000

// The AES/VDI passes parameters in D0/D1, so we do not touch
// them registers..
	clr.w	-(sp)
	move.l	SYM(curproc),-(sp)
	addq.l	#P_SYSCTXT,(sp)
	jsr	SYM(build_context)

	// A1 set to callers stack by build_context
	move.l	SYM(curproc),a0
	move.l	(a0),sp			// fetch our systemstack
	move.l	P_SYSCTXT+C_D0(a0),d0		// D0.l saved by build_context, function code (AES or VDI)
	move.l	P_SYSCTXT+C_D1(a0),d1		// AES/VDI pb
	cmp.w	#115,d0			// VDI?
	beq	enter_vdi
	cmp.w	#200,d0			// AES?
	beq	enter_aes
	cmp.w	#201,d0			// appl_yield()?
	beq	enter_aes		// or should this be its own function ?

	// some VDI special cases, must be prepared by any VDI module ..
	cmp.w	#-2,d0			// Query GDOS version
	beq	trap2_gdosvers
	cmp.w	#-1,d0			// This returns the address of the VDI handler
	beq	trap2_vdihandler

	// What to do with trap 2 calls we dont know?
	// for now just noting..
trap2_out:
	bra	out_select

	// Call the VDI
	// long VDI_dispach( VDIpb *pb );
enter_vdi:
	move.l	SYM(vdi_handler),d2
	beq.s	do_old_trap2
	move.l	d2,a0
	move.l	d1,-(sp)		// VDI parameter block address
	jsr	(a0)
	addq.l	#4,sp
	moveq	#1,d2			// nonzero D2 prevents preempt
	bra.s	trap2_out

	// Call the AES
	// long AES_dispatch ( AESpb *pb );
enter_aes:
	move.l	SYM(aes_handler),d2
	beq.s	do_old_trap2
	move.l	d2,a0
	move.l	d1,-(sp)
	jsr	(a0)
	addq.l	#4,sp
	moveq	#0,d2			// zero D2 allows preempt
	bra.s	trap2_out

	// Return the GDOS version if VDI is installed
	// as a kernel module.
trap2_gdosvers:
	move.l	SYM(vdi_handler),d2
	beq.s	do_old_trap2
	move.l	SYM(gdos_version),d0
	moveq	#1,d2
	bra.s	trap2_out

	// Deliver the address of the VDI function dispatcher
	// if we have it.
trap2_vdihandler:
	move.l	SYM(vdi_handler),d0
	moveq	#1,d2
	bne.s	trap2_out

	// Call old trap 2, when no function registered
do_old_trap2:
	bsr	SYM(trap_2_emu)
	moveq	#1,d2
	bra.s	trap2_out

rom_trap2:
	move.l	SYM(old_trap2),-(sp)
	rts

// *********************************
// * ROM GEMDOS/BIOS/XBIOS callout *
// *********************************
//
// Trap wrapper for calling ROM from the inside of the kernel.
// Usage: just replace trap #1 with a jsr trap_1_emu etc.

	.globl	SYM(trap_1_emu)
	.globl	SYM(trap_2_emu)
	.globl	SYM(trap_13_emu)
	.globl	SYM(trap_14_emu)

	.text

SYM(trap_1_emu):
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	TE_D_68K

	move.w	sr,d0
	move.w	d0,-(sp)	// return sr
	move.w	#0x4000,-(sp)	// format/vector word

	bra.s	TE_D_JMP_OLD
TE_D_68K:
#endif
#ifdef M68000
	tst.w	(0x059e).w		// test longframe
	beq.s	TE_D
#endif
	move.w	(sp),-(sp)
	move.w	4(sp),2(sp)
	clr.w	4(sp)
TE_D:
#ifdef __mcoldfire__
	move.w	sr,d0
	move.w	d0,-(sp)
#else
	move.w	sr,-(sp)
#endif
TE_D_JMP_OLD:
#if !(defined(M68000) || defined(__mcoldfire__))
	jmp	([SYM(old_dos)])
#else
	move.l	SYM(old_dos).w(pc),-(sp)
	rts
#endif

	// Ozk: we cannot use D0/D1 becaues the ugly VDI and AES
	// passes paremters in these registers.
SYM(trap_2_emu):
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	TE_AV_68K

	subq.l	#6,sp
	move.w	d0,(sp)		// backup d0
	move.w	#0x4000,2(sp)	// format/vector word
	move.w	sr,d0
	move.w	d0,4(sp)	// return sr
	move.w	(sp)+,d0	// restore d0

	bra.s	TE_AV_JMP_OLD
TE_AV_68K:
#endif
#ifdef	M68000
	tst.w	(0x059e).w
	beq.s	TE_AV
#endif
	move.w	(sp),-(sp)
	move.w	4(sp),2(sp)
	clr.w	4(sp)
TE_AV:
#ifdef __mcoldfire__
	subq.l	#4,sp
	move.w	d0,(sp)
	move.w	sr,d0
	move.w	d0,2(sp)
	move.w	(sp)+,d0
#else
	move.w	sr,-(sp)
#endif
TE_AV_JMP_OLD:
#if !(defined(M68000) || defined(__mcoldfire__))
	jmp	([SYM(old_trap2)])
#else
	move.l	SYM(old_trap2).w(pc),-(sp)
	rts
#endif

SYM(trap_13_emu):
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	TE_B_68K

	move.w	sr,d0
	move.w	d0,-(sp)	// return sr
	move.w	#0x4000,-(sp)	// format/vector word

	bra.s	TE_B_JMP_OLD
TE_B_68K:
#endif
#ifdef M68000
	tst.w	(0x059e).w		// test longframe
	beq.s	TE_B
#endif
	move.w	(sp),-(sp)
	move.w	4(sp),2(sp)
	clr.w	4(sp)
TE_B:
#ifdef __mcoldfire__
	move.w	sr,d0
	move.w	d0,-(sp)
#else
	move.w	sr,-(sp)
#endif
TE_B_JMP_OLD:
#if !(defined(M68000) || defined(__mcoldfire__))
	jmp	([SYM(old_bios)])
#else
	move.l	SYM(old_bios).w(pc),-(sp)
	rts
#endif

SYM(trap_14_emu):
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	TE_X_68K

	move.w	sr,d0
	move.w	d0,-(sp)	// return sr
	move.w	#0x4000,-(sp)	// format/vector word

	bra.s	TE_X_JMP_OLD
TE_X_68K:
#endif
#ifdef M68000
	tst.w	(0x059e).w		// test longframe
	beq.s	TE_X
#endif
	move.w	(sp),-(sp)
	move.w	4(sp),2(sp)
	clr.w	4(sp)
TE_X:
#ifdef __mcoldfire__
	move.w	sr,d0
	move.w	d0,-(sp)
#else
	move.w	sr,-(sp)
#endif
TE_X_JMP_OLD:
#if !(defined(M68000) || defined(__mcoldfire__))
	jmp	([SYM(old_xbios)])
#else
	move.l	SYM(old_xbios).w(pc),-(sp)
	rts
#endif

// ******************************
// * unused trap's trap handler *
// ******************************
//
// Default routine for "unused" traps. Without this, any user (even
// on remote) can draw bombs on the console freely. So this is sorta
// security patch.
// Notice any programs those take these traps should now be loaded
// *after* MiNT. init_intr() decides which traps to take, see there.

	.globl	SYM(unused_trap)

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
	dc.l	v_rte

SYM(unused_trap):
	move.w	#0x010b,-(sp)		// getpid
	trap	#1
	move.w	#0x000c,-(sp)		// SIGSYS
	move.w	d0,-(sp)		// himself
	move.w	#0x0111,-(sp)		// kill
	trap	#1
	addq.l	#0x08,sp
v_rte:	rte				// and return :-)

// ************************
// * miscellaneous things *
// ************************

//
// _lineA0: MiNT calls this to get the address of the line A variables
//
	.globl	SYM(lineA0)

SYM(lineA0):
#ifdef __mcoldfire__
	move.l	a2,-(sp)		// save scratch registers
	move.l	d2,-(sp)
	dc.w	0xa920			// call the line A initialization routine
	move.l	(sp)+,d2
	move.l	(sp)+,a2
#else
	movem.l	d2/a2,-(sp)		// save scratch registers
	dc.w	0xa000			// call the line A initialization routine
	movem.l	(sp)+,d2/a2
#endif
	rts

//
// _call_aes: calls the GEM AES, using the control block passed as
// a parameter. Used only for doing appl_init(), to see
// if the AES is active yet
//
	.globl	SYM(call_aes)

SYM(call_aes):
	move.l	4(sp),d1		// fetch pointer to parameter block
	move.l	#200,d0 		// magic number for AES
#ifdef __mcoldfire__
	move.l	a2,-(sp)		// save scratch registers
	move.l	d2,-(sp)
#else
	movem.l	d2/a2,-(sp)		// save scratch registers
#endif
	trap	#2
#ifdef __mcoldfire__
	move.l	(sp)+,d2
	move.l	(sp)+,a2
#else
	movem.l	(sp)+,d2/a2
#endif
	rts

//
// _callout: Call an external function, passing <32 bytes of arguments,
// and return the value from the function. NOTE: we must be careful
// to save all registers here!
//
	.globl	SYM(callout)
	.globl	SYM(callout1)
	.globl	SYM(callout2)
	.globl	SYM(callout6)
	.globl	SYM(callout6spl7)
//
// callout is the general purpose one
//
SYM(callout):
	lea	8(sp),a0		// pointer to args
	move.l	4(sp),a1		// pointer to pointer to function
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)	// save registers
#else
	movem.l	d2-d7/a2-a6,-(sp)	// save registers
#endif
	movem.l	(a0),d0-d7		// copy parameters
#ifdef __mcoldfire__
	lea	-32(sp),sp
	movem.l	d0-d7,(sp)
#else
	movem.l	d0-d7,-(sp)
#endif
	suba.l	a5,a5			// the BIOS expects 0 in a5
	jsr	(a1)			// go do it
	lea	32(sp),sp
#ifdef __mcoldfire__
	movem.l	(sp),d2-d7/a2-a6	// restore reggies
	lea	44(sp),sp
#else
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
#endif
	rts
//
// callout2 and callout1 are for functions with just 1 or
// 2 16 bit parameters. We cheat and use the same code for
// both, since passing 32 bits isn't much more expensive than
// passing 16 bits (and since the called function will just
// ignore any extra arg)
//

SYM(callout1):
SYM(callout2):
	movem.l	4(sp),a0/a1		// get function ptr & args
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)	// save reggies
#else
	movem.l	d2-d7/a2-a6,-(sp)	// save reggies
#endif
	move.l	a1,-(sp)		// push args
	suba.l	a5,a5			// the BIOS expects 0 in a5
	jsr	(a0)			// do function
	addq.l	#4,sp
#ifdef __mcoldfire__
	movem.l	(sp),d2-d7/a2-a6	// restore reggies
	lea	44(sp),sp
#else
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
#endif
	rts
//
// callout6 passes 6 words, saving sr
//
SYM(callout6):
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)	// save registers
#else
	movem.l	d2-d7/a2-a6,-(sp)	// save registers
#endif
	movem.l	4+44(sp),a0-a3		// get function ptr & args
#ifdef __mcoldfire__
	move.w	sr,d2
	move.w	d2,-(sp)
	lea	-12(sp),sp
	movem.l	a1-a3,(sp)		// copy args
#else
	move.w	sr,-(sp)
	movem.l	a1-a3,-(sp)		// copy args
#endif
	suba.l	a5,a5			// the BIOS expects 0 in a5
	jsr	(a0)			// go do it
	lea	12(sp),sp
#ifdef __mcoldfire__
	move.w	(sp)+,d2
	move.w	d2,sr
	movem.l	(sp),d2-d7/a2-a6	// restore reggies
	lea	44(sp),sp
#else
	move.w	(sp)+,sr
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
#endif
	rts
//
// callout6spl7 passes 6 words at ipl7
//
SYM(callout6spl7):
#ifdef __mcoldfire__
	lea	-44(sp),sp
	movem.l	d2-d7/a2-a6,(sp)	// save registers
#else
	movem.l	d2-d7/a2-a6,-(sp)	// save registers
#endif
	movem.l	4+44(sp),a0-a3		// get function ptr & args
#ifdef __mcoldfire__
	move.w	(sp)+,d2
	move.w	d2,sr
	lea	-12(sp),sp
	movem.l	a1-a3,(sp)		// copy args
#else
	move.w	sr,-(sp)
	movem.l	a1-a3,-(sp)		// copy args
#endif
	suba.l	a5,a5			// the BIOS expects 0 in a5
#ifdef __mcoldfire__
	move.w	sr,d2
	ori.l	#0x0700,d2		// spl7()
	move.w	d2,sr
#else
	ori.w	#0x0700,sr		// spl7()
#endif
	jsr	(a0)			// go do it
	lea	12(sp),sp
#ifdef __mcoldfire__
	move.w	(sp)+,d2
	move.w	d2,sr
	movem.l	(sp),d2-d7/a2-a6	// restore reggies
	lea	44(sp),sp
#else
	move.w	(sp)+,sr
	movem.l	(sp)+,d2-d7/a2-a6	// restore reggies
#endif
	rts

//
// do_usrcall: call the user supplied function (*usrcall)(), with
// arguments given in the longwords usrarg1..usrarg5. Return value
// is placed in usrret. This function is used by the Supexec code
// in xbios.c.

	.globl	SYM(do_usrcall)
	.globl	SYM(usrret)
	.globl	SYM(usrcall)
	.globl	SYM(usrarg1)
	.globl	SYM(usrarg2)
	.globl	SYM(usrarg3)
	.globl	SYM(usrarg4)
	.globl	SYM(usrarg5)

	FUNC(do_usrcall)
SYM(do_usrcall):
	CFI_STARTPROC()
	move.l	SYM(usrarg5),-(sp)
	move.l	SYM(usrarg4),-(sp)
	move.l	SYM(usrarg3),-(sp)
	move.l	SYM(usrarg2),-(sp)
	move.l	SYM(usrarg1),-(sp)
	move.l	SYM(usrcall),-(sp)		// the user expects to see this on the stack
	pea	ucret(pc)		// so rts puts us back here
	move.l	SYM(usrcall),-(sp)		// this copy is for us to use
	rts				// to jump to (we don't want to use registers)
ucret:	lea	24(sp),sp		// fix up stack
	move.l	d0,SYM(usrret)		// save returned value
	rts				// back to caller
	CFI_ENDPROC()
	END(do_usrcall)

// *****************************
// * Reentrant syscall handler *
// *****************************
//
// Implemented by Odd Skancke.
//
// Reentrant means here that we can enter the kernel again through
// the trap handler from inside kernel space.
// This is not neccessary for the kernel itself but rather if you want
// to use other subsystems which use the trap#1 to enter the kernel again.
// You can, for example,use the TOS-VDI subsystem safely and the VDI can
// reenter the kernel through the trap handler.
//
// This piece of code needs more testing, NEW_SYSCALL not tested,
// 68000 not tested (with M68000)
//
reentrant_trap:
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	reent_68k

	move.w	(a0),d1			// format/vector word
	addq.l	#2+2+4,a0		// skip format/vector word, sr and pc
	andi.l	#0x3000,d1		// keep the filler size bits
	beq.s	reent_fpu
	addq.l	#2,a0			// assume 2-byte filler after the exception frame
	bra.s	reent_fpu
reent_68k:
#endif
#ifdef M68000
	addq.l	#2+4,a0
	tst.w	(0x59e).w
	beq.s	reent_shortframe
	addq.l	#2,a0
reent_shortframe:
#else
	addq.l	#2+4+2,a0
#endif
reent_fpu:
	// Save hardware registers (CPU/FPU)
#ifdef __mcoldfire__
	lea	-16(sp),sp
	fsave	(sp)		// save internal state frame (including fpcr and fpsr)
	tst.b	(sp)		// if NULL frame then the FPU is not in use
	beq.s	reent_fpu_notused // skip programmer's model restore
	lea	-64(sp),sp
	fmovem.d fp0-fp7,(sp)	// save data registers
	fmove.l	fpiar,-(sp)	// and control registers
	move.w	#1,-(sp)	// data registers are saved
	bra.s	reent_fpu_saved
#else
	tst.w	SYM(fpu)
	beq.s	reent_fpu_saved
	fsave	-(sp)

#ifndef M68000
	cmp.w	#60,SYM(mcpu)+2
	bne.s	reent_no_060
	tst.b	2(sp)
	beq.s	reent_fpu_notused
	bra.s	reent_save_fpu
reent_no_060:
#endif

	tst.b	(sp)
	beq.s	reent_fpu_notused
reent_save_fpu:
	fmovem.x fp0-fp7,-(sp)
	fmovem.l fpcr/fpsr/fpiar,-(sp)
	move.w	#1,-(sp)
	bra.s	reent_fpu_saved
#endif /* __mcoldfire__ */
reent_fpu_notused:
	clr.w	-(sp)
reent_fpu_saved:

	moveq	#0,d1
	move.w	(a0),d1			// Function number
	cmp.w	d5,d1			// check if within bounds
	bcc.s	reent_erange		// Out of range

	// D0 will hold the address of function to call
	// D1 function number when staring here, indexing function tab
	// D5 will hold byte size of parameters
	// A0 will be placed at param_start + D5 (param_size)
	// A5 contains address of function tab

// ---------- BEGIN without NEW_SYSCALL ---------------
#ifndef NEW_SYSCALL
	// Fetch address of function, and copy 24 bytes of the parameters
	// (or 32 bytes if function table == gemdos) from the original place.
 #ifndef M68000
	move.l	(a5,d1.l*4),d2
 #else
	add.w	d1,d1
	add.w	d1,d1
	move.l	(a5,d1.l),d2
 #endif
	beq.s	reent_nofunc
	moveq	#24,d5
	cmpa.l	#SYM(dos_tab),a5
	bne.s	reent_24
 #ifdef __mcoldfire__
	addq.l	#8,d5
 #else
	addq.w	#8,d5
 #endif
reent_24:

// ---------- END without NEW_SYSCALL ---------------
#else
// ---------- BEGIN with NEW_SYSCALL ------------------
// Ozk:	When writing this routine, I didnt know if the
//	size of params in the function table includes
//	the function number. If not, this code needs
//	to be modified to reflect that, it now thinks
//	param size includes function number!
 #ifndef M68000
	move.l	(a5,d1.l*8),d2
 #else
	add.w	d1,d1
	add.w	d1,d1
	add.w	d1,d1
	move.l	(a5,d1.l),d2
 #endif
	beq.s	reent_nofunc
 #ifdef __mcoldfire__
	mvz.w	6(a5,d1.l*8),d5
 #elif !defined(M68000)
	move.w	6(a5,d1.l*8),d5
 #else
 	move.w	6(a5,d1.l),d5
 #endif
#endif	//NEW_SYSCALL
// ------------ END with NEW_SYSCALL ------------------
#ifdef __mcoldfire__
	add.l	d5,a0		// A0 now points to the last parameter (first one pushed)
	subq.l	#2,d5		// Skip the Function number
	move.l	d5,d1
#else
	add.w	d5,a0		// A0 now points to the last parameter (first one pushed)
	subq.w	#2,d5		// Skip the Function number
	move.w	d5,d1
#endif
	beq.s	reent_noargs	// No args (only function number)
#ifdef __mcoldfire__
	lsr.l	#1,d1		// Turn bytes into words
	subq.l	#1,d1
#else
	lsr.w	#1,d1		// Turn bytes into words
	subq.w	#1,d1
#endif
reent_cpyargs:
	move.w	-(a0),-(sp)
#ifdef __mcoldfire__
	subq.l	#1,d1
	bpl.s	reent_cpyargs
#else
	dbra	d1,reent_cpyargs
#endif
reent_noargs:

	move.l	d2,a0		// Function address
	jsr	(a0)
	moveq	#0,d1		// Clear D1 -- we're not gonna call anything else
reent_out:
#ifdef __mcoldfire__
	add.l	d5,sp		// Correct stack
#else
	add.w	d5,sp		// Correct stack
#endif
	// Unstack all cpu/fpu registers...
#ifdef __mcoldfire__
	tst.w	(sp)+		// data registers saved?
	beq.s	reent_no_fpu_saved
	fmove.l	(sp)+,fpiar	// restore control registers
	fmovem.d (sp),fp0-fp7	// and data registers
	lea	64(sp),sp
reent_no_fpu_saved:
	frestore (sp)		// finally the internal state (including fpcr and fpsr)
	lea	16(sp),sp
#else
	tst.w	SYM(fpu)
	beq.s	reent_out_no_fpu
	tst.w	(sp)+
	beq.s	reent_no_fpu_saved
	fmovem.l (sp)+,fpcr/fpsr/fpiar
	fmovem.x (sp)+,fp0-fp7
reent_no_fpu_saved:
	frestore (sp)+
#endif /* __mcoldfire__ */
reent_out_no_fpu:
#ifdef __mcoldfire__
	movem.l	(sp),d2-d7/a2-a6
	lea	44(sp),sp
#else
	movem.l	(sp)+,d2-d7/a2-a6
#endif
	tst.l	d1		// If D1 is not NULL, it is the address of some old trap
	bne.s	reent_call_rom
	rte
reent_call_rom:
	move.l	d1,a0
	jmp	(a0)

// ---- What to do when function number is outside function range
reent_erange:
	moveq	#0,d5
	cmpa.l	#SYM(xbios_tab),a5
	beq.s	reent_rom_xbios
	bra.s	reent_enosys

// ---- What to do when there is no funcion installed
// ---- in the function table
reent_nofunc:
	moveq	#0,d5
	cmpa.l	#SYM(bios_tab),a5
	beq.s	reent_rom_bios
	cmpa.l	#SYM(xbios_tab),a5
	beq.s	reent_rom_xbios
reent_enosys:
	moveq	#-32,d0
	moveq	#0,d1
	bra.s	reent_out

// ---- set D1 to point to the old trap entry
reent_rom_bios:
	move.l	SYM(old_bios),d1
	bra.s	reent_out
reent_rom_xbios:
	move.l	SYM(old_xbios),d1
	bra.s	reent_out

|---------------------------------------------------------------------------------

// Because of different parameter passing, we need a whole different
// scheme with AES/VDI traps... ARGH!! :-)
reentrant_trap2:
	// Since we're not interested in the stack now,
	// we wont bother with it. VDI/AES passes args in D0/D1.
#ifdef	TRAP2_STACK_PARAMS
#ifdef __mcoldfire__
	tst.w	SYM(coldfire_68k_emulation)
	bne.s	reent2_68k

	move.w	(a0),d1			// format/vector word
	addq.l	#2+2+4,a0		// skip format/vector word, sr and pc
	andi.l	#0x3000,d1		// keep the filler size bits
	beq.s	reent2_fpu
	addq.l	#2,a0			// assume 2-byte filler after the exception frame
	bra.s	reent2_fpu
reent2_68k:
#endif
#ifdef M68000
	addq.l	#2+4,a0
	tst.w	(0x59e).w
	beq.s	reent2_shortframe
	addq.l	#2,a0
reent2_shortframe:
#else
	addq.l	#2+4+2,a0
#endif
#endif
reent2_fpu:
	// Save hardware registers (CPU/FPU)
#ifdef __mcoldfire__
	lea	-16(sp),sp
	fsave	(sp)		// save internal state frame (including fpcr and fpsr)
	tst.b	(sp)		// if NULL frame then the FPU is not in use
	beq.s	reent2_fpu_notused // skip programmer's model restore
	lea	-64(sp),sp
	fmovem.d fp0-fp7,(sp)	// save data registers
	fmove.l	fpiar,-(sp)	// and control registers
	move.w	#1,-(sp)	// data registers are saved
	bra.s	reent2_fpu_saved
#else
	tst.w	SYM(fpu)
	beq.s	reent2_fpu_saved
	fsave	-(sp)

#ifndef M68000
	cmp.w	#60,SYM(mcpu)+2
	bne.s	reent2_no_060
	tst.b	2(sp)
	beq.s	reent2_fpu_notused
	bra.s	reent2_save_fpu
reent2_no_060:
#endif

	tst.b	(sp)
	beq.s	reent2_fpu_notused
reent2_save_fpu:
	fmovem.x fp0-fp7,-(sp)
	fmovem.l fpcr/fpsr/fpiar,-(sp)
	move.w	#1,-(sp)
	bra.s	reent2_fpu_saved
#endif /* __mcoldfire__ */
reent2_fpu_notused:
	clr.w	-(sp)
reent2_fpu_saved:

|-----------------------
/// FUNCTION CALLING HERE
	cmp.w	#115,d0			// VDI?
	beq	reent2_enter_vdi
	cmp.w	#200,d0			// AES?
	beq	reent2_enter_aes
	cmp.w	#201,d0			// appl_yield()?
	beq	reent2_enter_aes	// or should this be its own function ?

	// some VDI special cases, must be prepared by any VDI module ..
	cmp.w	#-2,d0			// Query GDOS version
	beq	reent2_trap2_gdosvers
	cmp.w	#-1,d0			// This returns the address of the VDI handler
	beq	reent2_trap2_vdihandler

	// What to do with trap 2 calls we dont know?
	// for now just noting..
	moveq	#0,d2

reent2_trap2_out:
	bra.s	reent2_out

	// Call the VDI
	// long VDI_dispach( VDIpb *pb );
reent2_enter_vdi:
	move.l	SYM(vdi_handler),d2
	beq.s	reent2_old_trap2
	move.l	d2,a0
	move.l	d1,-(sp)
	jsr	(a0)
	addq.l	#4,sp
	moveq	#0,d2
	bra.s	reent2_trap2_out

	// Call the AES
	// long AES_dispatch ( AESpb *pb );
reent2_enter_aes:
	move.l	SYM(aes_handler),d2
	beq.s	reent2_old_trap2
	move.l	d2,a0
	move.l	d1,-(sp)
	jsr	(a0)
	addq.l	#4,sp
	moveq	#0,d2
	bra.s	reent2_trap2_out

	// REturn the GDOS version if VDI is installed
	// as a kernel module.
reent2_trap2_gdosvers:
	move.l	SYM(vdi_handler),d2
	beq.s	reent2_old_trap2
	move.l	SYM(gdos_version),d0
	moveq	#0,d2
	bra.s	reent2_trap2_out

	// Deliver the address of the VDI function dispatcher
	// if we have it.
reent2_trap2_vdihandler:
	moveq	#0,d2
	move.l	SYM(vdi_handler),d0
	bne.s	reent2_trap2_out

reent2_rom_trap2:
reent2_old_trap2:
	moveq	#1,d2
	bra.s	reent2_trap2_out

/// OUT begins here
|-----------------------
	// If D2 is NULL, we exit normally, because we did something
	// If D2 is not NULL, we call the old trap2
reent2_out:
	// Unstack all cpu/fpu registers...
#ifdef __mcoldfire__
	tst.w	(sp)+		// data registers saved?
	beq.s	reent2_no_fpu_saved
	fmove.l	(sp)+,fpiar	// restore control registers
	fmovem.d (sp),fp0-fp7	// and data registers
	lea	64(sp),sp
reent2_no_fpu_saved:
	frestore (sp)		// finally the internal state (including fpcr and fpsr)
	lea	16(sp),sp
#else
	tst.w	SYM(fpu)
	beq.s	reent2_out_no_fpu
	tst.w	(sp)+
	beq.s	reent2_no_fpu_saved
	fmovem.l (sp)+,fpcr/fpsr/fpiar
	fmovem.x (sp)+,fp0-fp7
reent2_no_fpu_saved:
	frestore (sp)+
#endif /* __mcoldfire__ */
reent2_out_no_fpu:
	tst.l	d2
	bne.s	reent2_call_old
#ifdef __mcoldfire__
	movem.l	(sp),d1-d7/a0-a6
	lea	56(sp),sp
#else
	movem.l	(sp)+,d1-d7/a0-a6
#endif
	rte
reent2_call_old:
#ifdef __mcoldfire__
	movem.l	(sp),d1-d7/a0-a6
	lea	56(sp),sp
#else
	movem.l	(sp)+,d1-d7/a0-a6
#endif
	move.l	SYM(old_trap2),-(sp)
	rts
