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

#include "mint/asmdefs.h"
#include "mint/xbra.h"

	.equ	use_200Hz, 1

	.globl SYM(old_200Hz_int)
	.globl SYM(interrupt_200Hz)
	.globl SYM(old_i6_int)
	.globl SYM(interrupt_i6)
	.extern SYM(ethernat_int)
	.globl SYM(set_old_int_lvl)
	.globl SYM(set_int_lvl6)
	.globl SYM(ethernat_probe_asm)
	.extern SYM(ethernat_probe_c)

	.text


	dc.l	XBRA_MAGIC
	dc.l	0x00000000		| (no cookie)
SYM(old_200Hz_int):
	ds.l	1
SYM(interrupt_200Hz):
	move.w	(sp),oldSR
	movem.l	a0-a7/d0-d7,-(sp)
	bsr	SYM(ethernat_int)
	movem.l	(sp)+,a0-a7/d0-d7
	move.l	SYM(old_200Hz_int)(pc),-(sp)	|jump through old handler
	rts


	dc.l	XBRA_MAGIC
	dc.l	0x00000000		| (no cookie)
SYM(old_i6_int):
	ds.l	1
SYM(interrupt_i6):
	move.w	(sp),oldSR
	movem.l	a0-a7/d0-d7,-(sp)
	bsr	SYM(ethernat_int)
	movem.l	(sp)+,a0-a7/d0-d7
	rte

oldSR:	ds.w	1


| Sets interrupt level to what was in the SR
SYM(set_old_int_lvl):
	andi.w	#0x0f00,oldSR		|just keep the int lvl
	move.l	d0,-(sp)
	move.w	sr,d0
	andi.w	#0xf0ff,d0
	or.w	oldSR,d0
	move.w	d0,sr
	move.l	(sp)+,d0
	rts

| Sets interrupt level to 6
SYM(set_int_lvl6):
	move.w	d0,-(sp)
	move.w	sr,d0
	move.w	d0,oldSR
	andi.w	#0xf0ff,d0
	ori.w	#0x0600,d0
	move.w	d0,sr
	move.w	(sp)+,d0
	rts


| Change bus error exception handler
| Call probe function
| Enable again the old bus error handler
SYM(ethernat_probe_asm):
	movem.l	a0-a7/d0-d7,-(sp)
	move.w	sr,-(sp)
	move.l	8,old_berr
	lea	berr,a0
	move.l	a0,8
	move.l	sp,old_stack
	bsr	SYM(ethernat_probe_c)
berr:
	move.l	old_berr,8
	move.l	old_stack,sp
	move.w	(sp)+,sr
	movem.l	(sp)+,d0-d7/a0-a7
	rts

old_stack:
	ds.l	1
old_berr:
	ds.l	1
