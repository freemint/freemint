|
| Low level and interrupt routines for the V2ExpEth ethernet driver
|
| Author: Mark Duckworth
|
| 2010-12-17 Torbjorn & Henrik Gilda,
| 2021-12-02 Michael Grunditz
| 2022-10-27 Anders Granlund
|
| This file belongs to FreeMiNT. It's not in the original MiNT 1.12
| distribution. See the file CHANGES for a detailed log of changes.
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


#include "mint/asmdefs.h"

	.globl SYM(v2expeth_int_old)
	.globl SYM(v2expeth_int_handler)
	.globl SYM(v2expeth_int)

	.text
	dc.l	0x58425241		| XBRA
	dc.l	0x00000000		| (no cookie)
SYM(v2expeth_int_old):
	ds.l	1

oldSR:	ds.w	1

SYM(v2expeth_int):

	|
	| 200hz interrupt
	|
	move.w	(sp),oldSR
	movem.l	a0-a7/d0-d7,-(sp)
	bsr	SYM(v2expeth_int_handler)
	movem.l	(sp)+,a0-a7/d0-d7
	move.l	SYM(v2expeth_int_old)(pc),-(sp)	|jump through old handler
	rts