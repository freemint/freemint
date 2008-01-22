|
| Ethernat driver for FreeMiNT.
|
| This file belongs to FreeMiNT. It's not in the original MiNT 1.12
| distribution. See the file CHANGES for a detailed log of changes.
|
| Copyright (c) 2007 Henrik Gilda.
|
| This file is free software; you can redistribute it and/or modify
| it under the terms of the GNU General Public License as published by
| the Free Software Foundation; either version 2, or (at your option)
| any later version.
|
| This file is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; if not, write to the Free Software
| Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
|

| 
| Low level and interrupt routines for the Ethernat driver
|
| 2005-02-12 Henrik Gilda
| 2000-08-02 Frank Naumann
| 2000-03-24 Vassilis Papathanassiou
|
|

	.equ	use_200Hz, 1

	.globl _old_200Hz_int
	.globl _interrupt_200Hz
	.globl _old_i6_int
	.globl _interrupt_i6
	.globl _ethernat_int
	.globl _set_old_int_lvl
	.globl _set_int_lvl6

	.text


	dc.l	0x58425241		| XBRA
	dc.l	0x00000000		| (no cookie)
_old_200Hz_int:
	ds.l	1
_interrupt_200Hz:
	move.w	(sp),oldSR
	movem.l	a0-a7/d0-d7,-(sp)
	bsr	_ethernat_int
	movem.l	(sp)+,a0-a7/d0-d7
	move.l	_old_200Hz_int(PC),-(sp)	|jump through old handler
	rts


	dc.l	0x58425241		| XBRA
	dc.l	0x00000000		| (no cookie)
_old_i6_int:
	ds.l	1
_interrupt_i6:
	move.w	(sp),oldSR
	movem.l	a0-a7/d0-d7,-(sp)
	bsr	_ethernat_int
	movem.l	(sp)+,a0-a7/d0-d7
	rte

oldSR:	ds.w	1


| Sets interrupt level to what was in the SR
_set_old_int_lvl:
	move.w	(sp),oldSR
	andi.w	#0x0f00,oldSR		|just keep the int lvl
	move.l	d0,-(sp)
	move.w	sr,d0
	andi.w	#0xf0ff,d0
	or.w	oldSR,d0
	move.w	d0,sr
	move.l	(sp)+,d0
	rts

| Sets interrupt level to 6
_set_int_lvl6:
	move.w	d0,-(sp)
	move.w	sr,d0
	andi.w	#0xf0ff,d0
	ori.w	#0x0600,d0
	move.w	d0,sr
	move.w	(sp)+,d0
	rts
