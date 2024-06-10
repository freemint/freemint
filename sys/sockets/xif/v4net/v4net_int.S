|
| Low level and interrupt routines for the V4SA ethernet driver
|
| Author: Michael Grunditz
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


	#include "v4net.h"
#include "mint/asmdefs.h"

	.globl SYM(v4net_int_old)
	.globl SYM(v4net_int)
	.globl SYM(v4net_recv)
	.globl SYM(v4net_swposcopy)

	.text
	dc.l	0x58425241		| XBRA
	dc.l	0x00000000		| (no cookie)
SYM(v4net_int_old):
	ds.l	1
SYM(v4net_int):

#if V4NET_200HZ

	|
	| 200hz interrupt
	|

	movem.l	a2,-(sp)
	movea.l	(0xde003c),a2				| recieved anything?
	cmpa.l	(SYM(v4net_swposcopy)),a2
	beq.s	.v4net_int_skip
	cmpa.l	#0xfff00000,a2				| offline?
	bcc.s	.v4net_int_skip
	movem.l	d0-d2/a0-a1,-(sp)			| call recv function
	bsr		SYM(v4net_recv)
	movem.l	(sp)+,d0-d2/a0-a2
	move.l  SYM(v4net_int_old),-(sp)		| call old vector
	rts
.v4net_int_skip:
	move.l	(sp),a2
	move.l  SYM(v4net_int_old),(sp)			| call old vector
	rts

#else

	|
	| ethernet interrupt
	|

	btst	#13,(0xdff21e)				| ethernet interrupt requested?
	bne.s	.v4net_int_requested		| no, jump to old vector
	move.l  SYM(v4net_int_old),(sp)
	rts
.v4net_int_requested:
	movem.l	a2,-(sp)
	movea.l	(0xde003c),a2				| recieved anything?
	cmpa.l	(SYM(v4net_swposcopy)),a2
	beq.s	.v4net_int_skip

	cmpa.l	#0xfff00000,a2				| offline?
	bcc.s	.v4net_int_skip

	move.w	#0x2000,0xdff29a			| disable ethernet interrupts
	movem.l	d0-d2/a0-a1,-(sp)
	bsr		SYM(v4net_recv)					| call recv function
	movem.l	(sp)+,d0-d2/a0-a2

	move.w	#0x2000,0xdff29c			| clear interrupt request flag
	move.w	#0xa000,0xdff29a			| enable ethernet interrupts
	move.l  SYM(v4net_int_old),-(sp)		| call old vector
	rts

.v4net_int_skip:
	move.l	(sp),a2
	move.l  SYM(v4net_int_old),(sp)			| call old vector
	rts

#endif
