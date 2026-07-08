/*
 * Copyright (c) 2026 The FreeMiNT development team.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "mint/asmdefs.h"

	.extern SYM(ohci_interrupt_handle)
	.globl SYM(ohci_int_handle_asm)

/* PCI_BIOS specification: only a0 and d0 are allowed
 * to be changed in interrupt handler, so we need to
 * preserve a1 and d1 since GCC use them too as scratch
 * registers.
 */
#if !defined(OHCI_POLL) && !defined(TOSONLY)
SYM(ohci_int_handle_asm):
	subq.l #8,sp
	movem.l d1/a1,(sp)
	move.l d0,-(sp)		| PCI_BIOS specification: INT not for us leave d0 untouched
	move.l a0,-(sp)		| PCI_BIOS specification: parameter is in a0
	jsr SYM(ohci_interrupt_handle)
	addq.l #8,sp
	movem.l (sp),d1/a1
	addq.l #8,sp
	rts
#endif

#ifdef TOSONLY
/* fake_ikbd_int - Synthesize an IKBD/ACIA (MFP source 6) interrupt by
 * building an exception frame and jumping through vector 0x118 to drain
 * the IKBD FIFO before it overruns and packets from legacy Atari IKBD
 * mice are lost.
 */
	.globl SYM(fake_ikbd_int)

SYM(fake_ikbd_int):
#ifdef __mcoldfire__
	move.l	sp,d0
	and.l	#3,d0
	lsl.l	#8,d0
	lsl.l	#4,d0
	or.l	#0x4118,d0		| format[3:2]=01 | VEC=70
	move.l	sp,d2
	andi.l	#0xfffffffc,d2		| align sp down to 4-byte boundary
	movea.l	d2,sp
	pea	fake_ikbd_ret(pc)	| PC -> [SP+4..7]
	move.w	sr,d1
	move.w	d1,-(sp)		| SR -> [SP+2..3]
	move.w	d0,-(sp)		| format word -> [SP+0..1]
#else
	tst.w	0x59e.w
	beq	fake_ikbd_short
	move.w	#0x118,-(sp)
fake_ikbd_short:
	pea	fake_ikbd_ret(pc)
	move	sr,d0
	move.w	d0,-(sp)
#endif
	move.l	0x118,a0
	jmp	(a0)
fake_ikbd_ret:
	rts
#endif /* TOSONLY */
