/*
 * NetUSBee USB driver for FreeMiNT.
 * Modified for USB by David Galvez. 2010 - 2011
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Henrik Gilda.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 * Low level and interrupt routines for the NetUSBee driver
 *
 * 2005-02-12 Henrik Gilda
 * 2000-08-02 Frank Naumann
 * 2000-03-24 Vassilis Papathanassiou
 *
 *
 */

	.globl _set_old_int_lvl
	.globl _set_int_lvl6

	.text

oldSR:	ds.w	1

// Sets interrupt level to what was in the SR
_set_old_int_lvl:
	andi.w	#0x0f00,oldSR		//just keep the int lvl
	move.l	d0,-(sp)
	move.w	sr,d0
	andi.w	#0xf0ff,d0
	or.w	oldSR,d0
	move.w	d0,sr
	move.l	(sp)+,d0
	rts

// Sets interrupt level to 6
_set_int_lvl6:
	move.w	d0,-(sp)
	move.w	sr,d0
	move.w	d0,oldSR
	andi.w	#0xf0ff,d0
	ori.w	#0x0600,d0
	move.w	d0,sr
	move.w	(sp)+,d0
	rts
