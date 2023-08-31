|*
|* Copyright 2000 Konrad M. Kokoszkiewicz <draco@atari.org>
|* All rights reserved.
|* 
|* This program is free software; you can redistribute it and/or modify
|* it under the terms of the GNU General Public License as published by
|* the Free Software Foundation; either version 2, or (at your option)
|* any later version.
|* 
|* This file is distributed in the hope that it will be useful,
|* but WITHOUT ANY WARRANTY; without even the implied warranty of
|* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|* GNU General Public License for more details.
|* 
|* You should have received a copy of the GNU General Public License
|* along with this program; if not, write to the Free Software
|* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
|* 
|* 
|* Simple startup module
|* 

#include "symbols.h"

	.globl	C_SYMBOL_NAME(main)
	.globl	C_SYMBOL_NAME(_base)
	.globl __start

#ifndef STACKSIZE
#define STACKSIZE 0x20000
#endif

	.data
__dummy:
	dc.l	0x11111918

	.bss
C_SYMBOL_NAME(_base):
	ds.l	1

	.text
__start:
	move.l	0x04(%sp),%a0		| basepage address
	move.l	%a0,C_SYMBOL_NAME(_base)
#ifndef BESTFIT
	move.l	0x000c(%a0),%d0		| size of the TEXT segment
	add.l	0x0014(%a0),%d0		| size of the DATA segment
	add.l	0x001c(%a0),%d0		| size of the BSS segment
	add.l	#STACKSIZE+0x0100,%d0	| stack + basepage
	lea	0(%a0,%d0.l),%sp		| new stack pointer

	move.l	%d0,-(%sp)		| new size
	move.l	%a0,-(%sp)		| start address
	pea	(0x004a0000).l		| Mshrink()
	trap	#1
	lea	12(%sp),%sp
#endif
#ifdef GETHOME_BUILTIN
	move.l	C_SYMBOL_NAME(_base),-(%sp)
	bsr	C_SYMBOL_NAME(gethome)
	addq.l	#0x04,%sp
	lea	C_SYMBOL_NAME(home)(%pc),%a0
	beq.s	__loc
	move.l	%d0,(%a0)
	bra.s	__xmain
__loc:
	lea	_vlocal(%pc),%a1
	move.l	%a1,(%a0)
#endif
#ifdef SMALL_MODEL
__xmain:
	bsr	C_SYMBOL_NAME(main)
#else	
__xmain:
	jsr	C_SYMBOL_NAME(main)
#endif
	move.w	%d0,-(%sp)
	move.w	#0x004c,-(%sp)
	trap	#1

	.globl	C_SYMBOL_NAME(__main)
C_SYMBOL_NAME(__main):
	rts

#ifdef GETHOME_BUILTIN
	.globl	C_SYMBOL_NAME(home)
	.globl	C_SYMBOL_NAME(gethome)

	.bss
C_SYMBOL_NAME(home):	ds.l	1

	.text
C_SYMBOL_NAME(gethome):
	move.l	4(%sp),%a0
	move.l	0x002c(%a0),%a0
L_loop:
	lea	_vhome(%pc),%a1
	moveq	#0x05,%d0
L_strncmp:
	cmpm.b	(%a0)+,(%a1)+
	bne.s	L_nope
	subq.w	#0x01,%d0
	bne.s	L_strncmp
	move.l	%a0,%d0
	bne.s	L_got
L_nope:
	clr.l	%d0
L_zs:
	tst.b	(%a0)+
	bne.s	L_zs
	tst.b	(%a0)
	bne.s	L_loop
L_got:
	rts

_vhome:
	dc.b	0x48,0x4f,0x4d,0x45,0x3d,0x00
_vlocal:
	dc.b	0x2e,0x2f,0x00

#endif
