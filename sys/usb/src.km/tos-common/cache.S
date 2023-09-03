/*
 * cache.S - cache control functions for USB TOS drivers
 *
 * David Galvez     2019
 * Didier Mequignon 2005-2007, e-mail: aniplay@wanadoo.fr
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mint/asmdefs.h"

/* TODO: Check whether this routine is valid for the 68040 and ColdFire CPUs */

	.globl SYM(cpush)

SYM(cpush):
	move.l 8(sp),d0          // size
	beq.s .csh1
	move.l d0,d1
	addq.l #1,d1
	beq.s .csh3              // -1 then flush all
	move.l 4(sp),a0          // base
	move.l a0,d1
	and.l #15,d1
	sne.b d1
	and.l #1,d1
	lsr.l #4,d0              // 16
	add.l d1,d0
	cmp.l #256,d0            // cache lines
	bcc.s .csh3              // flush all
	move.l a0,d1             // line alignment
	and.b #0xF0,d1
	move.l d1,a0
	subq.l #1,d0
.csh2:                           // flush lines loop
	cpushl %dc,(a0)
	lea 16(a0),a0
	dbf d0,.csh2
	rts
.csh3:
	cpusha %dc                // flush all
.csh1:
	rts
