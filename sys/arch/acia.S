/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * This is the IKBD/MIDI ACIA interrupt service routine. We use it
 * on TOS < 2.0 to replace the original handler, which provides
 * too few vectors to handle our AKP routines.
 *
 * Routines are based on what TOS 4.04 does.
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

#include "mint/asmdefs.h"

# ifndef NO_AKP_KEYBOARD

KBDVEC		=	-4
MIDIVEC		=	KBDVEC+4
VKBDERR		=	MIDIVEC+4
VMIDERR		=	VKBDERR+4
STATVEC		=	VMIDERR+4
MOUSEVEC	=	STATVEC+4
CLOCKVEC	=	MOUSEVEC+4
JOYVEC		=	CLOCKVEC+4
VMIDISYS	=	JOYVEC+4
VIKBDSYS	=	VMIDISYS+4
IKBDSTATE	=	VIKBDSYS+4
IKBDTODO	=	IKBDSTATE+1

	.text

	.globl	SYM(kbdvecs),SYM(kbd_iorec)
	.globl	SYM(old_acia),SYM(new_acia)
	.extern SYM(has_kbdvec)
	.extern SYM(newkeys)

	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
SYM(old_acia):
	dc.l	0

// This handler is installed only when running on FireTOS and CT60 TOS 2, this
// is needed to workaround a problem with keyboard handling. Those TOS install
// directly in the ACIA vector the ikbdsys handler which never calls the
// undocumented kbdvec vector handler which take care of the keyboard scancode
// processing.
//
// For the rest of architectures this handler is disabled, the main reason is
// because we don't have a midisys vector handler and using the one from ROM
// crashes the system. Some ROM kbdvecs interrupt hadlers expect to have in an
// address register a value to reference its internal variables, they expect
// ROM's ACIA handler to put that adrress there for them. So while we depend
// on calling those ROM vectors handlers we can't use our own ACIA routine.
// FireTOS and CT60 TOS 2 ACIA handler doesn't process MIDI data so this
// problem doesn't occur with them, midisys handler in those TOS just return.

	
	FUNC(new_acia)
SYM(new_acia):
	CFI_STARTPROC()
#ifdef __mcoldfire__
	lea	-32(sp),sp
	movem.l	d0-d3/a0-a3,(sp)
#else
	movem.l	d0-d3/a0-a3,-(sp)
#endif
again:	move.l	SYM(kbdvecs),a3
	move.l	VMIDISYS(a3),a0
	jsr	(a0)
	move.l	VIKBDSYS(a3),a0
	jsr	(a0)
#ifdef __mcoldfire__
	moveq.l	#04,d0
	btst	d0,0xfa01.w	//(0xfa01).w
#else
	btst	#04,0xfa01.w	//(0xfa01).w
#endif
	beq.s	again
#ifdef __mcoldfire__
	moveq.l	#6,d0
	bclr	d0,0xfa11.w	//move.b	#0xbf,(0xfa11).w
	movem.l	(sp),d0-d3/a0-a3
	lea	32(sp),sp
#else
	movem.l	(sp)+,d0-d3/a0-a3
	bclr	#6,0xfa11.w	//move.b	#0xbf,(0xfa11).w
#endif
#ifdef M68000
	nop				// see intr.S and syscall.S
#endif
	rte
	CFI_ENDPROC()
	END(new_acia)

// TOS passes the following data to the routines pointed by
// kbdvec and midivec:
//
// d0 - data byte sent by ACIA
// a0 - a pointer to the appropriate IOREC_T structure
//
// So-do-we.

	.data

type:	dc.b	1,2,3,3,3,3,4,5,6,7
todo:	dc.b	7,5,2,2,2,2,6,2,1,1

	.text

	.globl	SYM(ikbdsys_handler)

// We only replace ikbdsys, midisys will be handled in ROM.

	FUNC(kbdsys_handler)
SYM(ikbdsys_handler):
	CFI_STARTPROC()
	move.l	SYM(kbdvecs),a3		// if ACIA handler is from ROM get kbdvecs
	move.b	(0xfc00).w,d1		// ACIA control
	move.l	SYM(kbd_iorec),a0
	btst	#0x07,d1		// interrupt request
#ifdef __mcoldfire__
	beq	return
#else
	beq.s	return
#endif
	btst	#0x00,d1		// Receiver full
	beq.s	skip
	move.l	d1,-(sp)
	bsr.s	no_error
	move.l	(sp)+,d1
skip:	btst	#0x05,d1		// Receiver overrun
	beq.s	return
	move.b	(0xfc02).w,d0		// ACIA data
	move.l	VKBDERR(a3),a1
	jmp	(a1)			// error

no_error:
	clr.l	d0
	move.b	(0xfc02).w,d0		// ACIA data
	tst.b	IKBDSTATE(a3)
	bne.s	mid_multiple
	cmp.b	#0xf6,d0		// $f6-$ff are multibyte packets
	bcc.s	not_keyboard
	tst.w	SYM(has_kbdvec)		// if kvdvec not present call new_keys directly
	beq	SYM(newkeys)
	move.l	KBDVEC(a3),a1
	jmp	(a1)

not_keyboard:
#ifdef __mcoldfire__
	sub.l	#0xf6,d0
#else
	sub.b	#0xf6,d0
#endif

// unlike some documentation states, kbdvecs->ikbdstate contains
// the type of the packet, and not the number of bytes transmitted.

	lea	type,a1
#ifdef __mcoldfire__
	move.b	(a1,d0.l),d1
	move.b	d1,IKBDSTATE(a3)	
#else
	move.b	(a1,d0.w),IKBDSTATE(a3)	
#endif
	lea	todo,a1
#ifdef __mcoldfire__
	move.b	(a1,d0.l),d1
	move.b	d1,IKBDTODO(a3)
	add.l	#0xf6,d0
#else
	move.b	(a1,d0.w),IKBDTODO(a3)
	add.b	#0xf6,d0
#endif
	cmp.b	#0xf8,d0
	blt.s	joy
	cmp.b	#0xfb,d0
	bgt.s	joy
	move.b	d0,mouse_2_buf
return:	rts

joy:	cmp.b	#0xfd,d0
	blt.s	ret
	move.b	d0,joy_buf
ret:	rts

mid_multiple:
#ifdef __mcoldfire__
	moveq.l	#0x06,d1
	cmp.b	IKBDSTATE(a3),d1
	bcc.s	nostk
#else
	cmp.b	#0x06,IKBDSTATE(a3)
	bcs.s	nostk
#endif

	clr.l	d1
	move.b	IKBDSTATE(a3),d1
	add.l	#joy_buf+1,d1
	subq.l	#6,d1
	move.l	d1,a2
	move.b	d0,(a2)
	move.l	JOYVEC(a3),a2
	lea	joy_buf,a0
	bra.s	call

nostk:	lea	buffers,a2

	clr.l	d2
	move.b	IKBDSTATE(a3),d2

#ifdef __mcoldfire__
	subq.l	#1,d2

	move.l	d2,d3
	lsl.l	#3,d2			// multiply by 8
	lsl.l	#2,d3			// multiply by 4
	add.l	d3,d2			// multiply by 12

	move.l	(a2,d2.l),a0		// get buffer address for packet
	move.l	4(a2,d2.l),a1		// get buffer end for packet
	move.l	8(a2,d2.l),d3		// routine to call
#else
	subq.w	#1,d2

	move.w	d2,d3
	lsl.w	#3,d2			// multiply by 8
	lsl.w	#2,d3			// multiply by 4
	add.w	d3,d2			// multiply by 12

	move.l	(a2,d2.w),a0		// get buffer address for packet
	move.l	4(a2,d2.w),a1		// get buffer end for packet
	move.l	8(a2,d2.w),d3		// routine to call
#endif
	move.l	(a3,d3.l),a2

	clr.l	d2
	move.b	IKBDTODO(a3),d2
	sub.l	d2,a1
	move.b	d0,(a1)
#ifdef __mcoldfire__
	moveq.l	#0,d0
	move.b	IKBDTODO(a3),d0
	subq.l	#1,d0
	move.b	d0,IKBDTODO(a3)
#else
	subq.b	#1,IKBDTODO(a3)
#endif
	tst.b	IKBDTODO(a3)
	bne.s	xret

call:	move.l	a0,-(sp)
	jsr	(a2)
	addq.l	#4,sp

	move.l	SYM(kbdvecs),a3
	clr.b	IKBDSTATE(a3)

xret:	rts
	CFI_ENDPROC()
	END(ikbdsys_handler)

	.bss

stat_buf:
	ds.b	7 + 1
mouse_1_buf:
	ds.b	5 + 1
mouse_2_buf:
	ds.b	3 + 1
clock_buf:
	ds.b	6
joy_buf:
	ds.b	2
dump_buf:			// universal buffer
	ds.b	8

	.data

buffers:
	dc.l	stat_buf,       stat_buf + 7, STATVEC
	dc.l	mouse_1_buf, mouse_1_buf + 5, MOUSEVEC
	dc.l	mouse_2_buf, mouse_2_buf + 3, MOUSEVEC
	dc.l	clock_buf,     clock_buf + 6, CLOCKVEC
	dc.l	joy_buf,         joy_buf + 2, JOYVEC
# endif
