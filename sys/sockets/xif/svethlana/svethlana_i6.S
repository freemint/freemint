|
| Interrupt entry for the SVEthlana MAC (vector 0xc5, Linux IRQ 141).
|
| This file belongs to FreeMiNT. It's not in the original MiNT 1.12
| distribution. See the file CHANGES for a detailed log of changes.
|
| This is what request_irq(IRQF_SHARED) gives the ethoc driver on Linux:
| the C handler is called with the scratch registers saved, and when it
| reports IRQ_NONE the interrupt is passed on to whoever owned the vector
| before us.
|
| This file is free software; you can redistribute it and/or modify
| it under the terms of the GNU General Public License as published by
| the Free Software Foundation; either version 2, or (at your option)
| any later version.
|

#include "mint/asmdefs.h"
#include "mint/xbra.h"

	.globl	SYM(svethlana_old_vector)
	.globl	SYM(svethlana_interrupt)
	.extern	SYM(ethoc_interrupt)

	.text

	dc.l	XBRA_MAGIC
	dc.l	MINT_MAGIC
SYM(svethlana_old_vector):
	ds.l	1
SYM(svethlana_interrupt):
	movem.l	d0-d2/a0-a2,-(sp)
	jsr	SYM(ethoc_interrupt)
	tst.l	d0
	movem.l	(sp)+,d0-d2/a0-a2	| movem does not touch the condition codes
	beq.s	chain
	rte
chain:
	move.l	SYM(svethlana_old_vector)(pc),-(sp)
	rts
