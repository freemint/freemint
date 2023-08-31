|
| Daynaport SCSI/Link driver for FreeMiNT.
| GNU C conversion by Miro Kropacek, <miro.kropacek@gmail.com>
|
| This file belongs to FreeMiNT. It's not in the original MiNT 1.12
| distribution. See the file CHANGES for a detailed log of changes.
|
| Copyright (c) 2007 Roger Burrows.
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
| low level routines for the SCSILINK driver
|
| march/2007, roger burrows
|	original, based on moribund MagxNet code
|

#include "mint/asmdefs.h"

	.globl	SYM(install_interrupts)
	.globl	SYM(interrupt_handler)
	
	.globl	SYM(set_lock)
	.globl	SYM(release_lock)
	.globl	SYM(test_lock)
	
	.text
|
| VBL interrupt handler
|
	.ascii	"XBRA"
	.ascii	"SLnk"
old_vbl_interrupt:
	ds.l	1
vbl_interrupt:
	movem.l	d0-d7/a0-a6,-(sp)			|save some regs
	bsr.w	SYM(interrupt_handler)		|call C code
	movem.l	(sp)+,d0-d7/a0-a6			|restore regs
	move.l	old_vbl_interrupt(pc),-(sp)	|then go to old handler
	rts

|
| install interrupt handler
|
SYM(install_interrupts):
	movem.l	d2/a2,-(sp)					|save some regs
	pea		vbl_interrupt
	move.w	#0x70/4,-(sp)
	move.w	#0x05,-(sp)					|issue Setexc()
	trap	#13
	addq.l	#8,sp
	move.l	d0,old_vbl_interrupt		|save ptr to old handler
	movem.l	(sp)+,d2/a2					|restore regs
	rts

| int set_lock(FASTLOCK *lockptr);
SYM(set_lock):
	movea.l	4(sp),a0
	tas		(a0)
	seq		d0
	rts
	
| int release_lock(FASTLOCK *lockptr);
SYM(release_lock):
	movea.l	4(sp),a0
	tst.b	(a0)
	smi		d0
	clr.b	(a0)
	rts
	
| int test_lock(FASTLOCK *lockptr);
SYM(test_lock):
	movea.l	4(sp),a0
	tst.b	(a0)
	smi		d0
	rts
