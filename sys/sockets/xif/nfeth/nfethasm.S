|
|  Interrupt routine for the Native Features Ethernet driver
| 
|  This file belongs to FreeMiNT. It's not in the original MiNT 1.12
|  distribution. See the file CHANGES for a detailed log of changes.
| 
|  Copyright (c) 2002-2006 Standa and Petr of ARAnyM dev team.
| 
|  This file is free software; you can redistribute it and/or modify
|  it under the terms of the GNU General Public License as published by
|  the Free Software Foundation; either version 2, or (at your option)
|  any later version.
| 
|  This file is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|  GNU General Public License for more details.
| 
|  You should have received a copy of the GNU General Public License
|  along with this program; if not, write to the Free Software
|  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
|

#include "mint/asmdefs.h"

	.globl SYM(old_interrupt)
	.globl SYM(my_interrupt)
	.extern SYM(nfeth_interrupt)

	.text

	dc.l	0x58425241		| XBRA
	dc.l	0x41524165		| ARAe
SYM(old_interrupt):
	ds.l	1
SYM(my_interrupt):
	movem.l	a0-a2/d0-d2,-(sp)
	bsr	SYM(nfeth_interrupt)
	movem.l	(sp)+,a0-a2/d0-d2
	move.l	SYM(old_interrupt)(pc),-(sp)
	rts
