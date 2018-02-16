/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation  either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program  if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-03-18
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# define ASM_ONLY
# include "xhdi_emu.h"

	.text

	.globl	_emu_xhdi

	dc.l	0x27011992	// XHDIMAGIC
_emu_xhdi:
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.l	30(sp),-(sp)
	move.w	32(sp),-(sp)
	move.w	#0x0001,-(sp)	// we want XHDI EMU
	move.w	#0x015f,-(sp)	// system EMU
	trap	#1
	lea	34(sp),sp
	rts


# ifdef XHDI_MON

	.globl	_xhdi_mon_dispatcher
	.globl	_xhdi_mon_dispatcher_table

	dc.l	0x27011992	// XHDIMAGIC
_xhdi_mon_dispatcher:
	link	a6,#-4
	movem.l	a0-a5/d1-d7,-(sp)
	move	8(a6),d2	// function number
	cmp	#20,d2		// correct function number?
	bcc.s	out		// no
	lsl.l	#3,d2		// not `mulu #8,d2' :-)
	lea	_xhdi_mon_dispatcher_table,a1
	moveq	#-1,d0		// #ERROR
	move.l	4(a1,d2),d0
	move.l	d0,-4(a6)
	sub.l	d0,sp
	bra.s	push1
push0:
	move.b	10(a6,d0),(sp,d0)
push1:
	dbra	d0,push0
	move.l	(a1,d2),a0
	jsr	(a0)
	move.l	-4(a6),d2
	add.l	d2,sp
out:
	movem.l	(sp)+,a0-a5/d1-d7
	unlk	a6
	rts

# endif
