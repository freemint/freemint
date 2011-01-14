/*
 * $Id$
 *
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

	.globl	_syskey,_keyrec
	.globl	_old_acia,_new_acia


	dc.l	0x58425241		// XBRA
	dc.l	0x4d694e54		// MiNT
_old_acia:
	dc.l	0

_new_acia:
	movem.l	d0-d3/a0-a3,-(sp)
again:	move.l	_syskey,a3
//	move.l	VMIDISYS(a3),a0
//	jsr	(a0)
	move.l	VIKBDSYS(a3),a0
	jsr	(a0)
	btst	#04,0xfa01.w	//(0xfa01).w
	beq.s	again
	movem.l	(sp)+,d0-d3/a0-a3
	bclr	#6,0xfa11.w	//move.b	#0xbf,(0xfa11).w
#ifdef M68000
	nop				// see intr.S and syscall.S
#endif
	rte

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

	.globl	_ikbdsys_handler

// We only replace ikbdsys, midisys will be handled in ROM.

_ikbdsys_handler:
	move.b	(0xfc00).w,d1		// ACIA control
	move.l	_keyrec,a0
	btst	#0x07,d1		// interrupt request
	beq.s	return
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
	and.w	#0xff,d0
	move.l	_keyrec,-(sp)
	move.w	d0,-(sp)
	jsr	_ikbd_scan
	addq.l	#6,sp
	rts

//	jmp	_newkeys
//	move.l	KBDVEC(a3),a1
//	jmp	(a1)

not_keyboard:
	sub.b	#0xf6,d0

// unlike some documentation states, syskey->ikbdstate contains
// the type of the packet, and not the number of bytes transmitted.

	lea	type,a1
	move.b	(a1,d0.w),IKBDSTATE(a3)
	lea	todo,a1
	move.b	(a1,d0.w),IKBDTODO(a3)
	add.b	#0xf6,d0
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
	cmp.b	#0x06,IKBDSTATE(a3)
	bcs.s	nostk

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

	subq.w	#1,d2

	move.w	d2,d3
	lsl.w	#3,d2			// multiply by 8
	lsl.w	#2,d3			// multiply by 4
	add.w	d3,d2			// multiply by 12

	move.l	(a2,d2.w),a0		// get buffer address for packet
	move.l	4(a2,d2.w),a1		// get buffer end for packet
	move.l	8(a2,d2.w),d3		// routine to call
	move.l	(a3,d3.l),a2

	clr.l	d2
	move.b	IKBDTODO(a3),d2
	sub.l	d2,a1
	move.b	d0,(a1)
	subq.b	#1,IKBDTODO(a3)
	tst.b	IKBDTODO(a3)
	bne.s	xret

call:	move.l	a0,-(sp)
	jsr	(a2)
	addq.l	#4,sp

	move.l	_syskey,a3
	clr.b	IKBDSTATE(a3)

xret:	rts

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
