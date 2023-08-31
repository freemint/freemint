/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 * 
 * Copyright 1999, 2000 Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
 * All rights reserved.
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
 * Author: Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
 * Started: 1999-03-21
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * Purpose:
 * Contains code to deal with the MMU found in the MC68040 and MC68060. Most
 * of it is in dc.w-notation, as some assemblers don't know the new calls/
 * registers. Note that pflush on the 040 is a different opcode than on the 030!
 *
 * History:
 * 99/04/14: - Added read_phys, a function to access a long at a given physical
 *             address (Gryf)
 * 99/03/23: - Creation (Gryf)
 * 
 */

#include "mint/asmdefs.h"

#if defined(WITH_MMU_SUPPORT) && (defined(M68040) || defined(M68060))

// set_mmu
//
// Switch to a given MMU tree.
//
// Input:
// 4(sp): New root pointer to write into URP and SRP

	.cpu   68040
	.arch  68040
	.text

	.globl	SYM(set_mmu)

SYM(set_mmu):
	move.l	4(sp),d0
	pflusha			// flush the ATCs
	movec	d0,%urp		// set user root pointer
	movec	d0,%srp		// set supervisor root pointer
	rts

// save_mmu, restr_mmu: save and restore the root pointers that came from ROM

	.data

oldurp:	dc.l	0
oldsrp:	dc.l	0

	.text

	.globl	SYM(save_mmu)

SYM(save_mmu):
	movec	%urp,d0		// save user root pointer
	move.l	d0,oldurp
	movec	%srp,d0		// and supervisor root pointer
	move.l	d0,oldsrp
	rts


	.globl	SYM(restr_mmu)

SYM(restr_mmu):
	pflusha			// flush the ATCs
	move.l	oldurp,d0
	move.l	oldsrp,d1
	movec	d0,%urp		// restore user root pointer
	movec	d1,%srp		// and supervisor root pointer
	rts


	.globl	SYM(flush_mmu)

SYM(flush_mmu):
	pflusha
	rts

// get_mmuregs
//
// Reads the seven configuration registers of the MMU and stores them in an
// array. The order is TC, ITT0, ITT1, DTT0, DTT1, URP, SRP.
//
// Input:
// 4(sp): Pointer to seven longwords.

	.globl	SYM(get_mmuregs)

SYM(get_mmuregs):
	move.l	4(sp),a0
	movec	%tc,d0
	move.l	d0,(a0)+
	movec	%itt0,d0
	move.l	d0,(a0)+
	movec	%itt1,d0
	move.l	d0,(a0)+
	movec	%dtt0,d0
	move.l	d0,(a0)+
	movec	%dtt1,d0
	move.l	d0,(a0)+
	movec	%urp,d0
	move.l	d0,(a0)+
	movec	%srp,d0
	move.l	d0,(a0)+
	rts

// read_phys
//
// Reads a long word from a given physical address, bypassing the current MMU
// layout (used to read the original MMU tree, which might well be hidden from
// physical access).
// Caution: If the address given is invalid (doesn't exist), an access execption
// is raised, but the data transpararent translation register 0 is still
// manipulated, so the system is likely to be in an unstable condition.
//
// Input:
// 4(sp): Physical address to read
//
// Returns:
// d0: Long word found at given address

	.globl	SYM(read_phys)

SYM(read_phys):
	move.l	4(sp),a0		// get address to read
	move	sr,d1
#ifdef __mcoldfire__
	move.w	sr,d0
	ori.l	#0x700,d0		// disable all interrupts
	move.w	d0,sr
#else
	ori	#0x700,sr		// disable all interrupts
#endif
	
	move.l	#0x00ffe060,d0		// value for DTT0: transparently
					// translate all data accesses, not using
					// the cache
	movec	%dtt0,a1			// save DTT0
	movec	d0,%dtt0			// set new DTT0
	move.l	(a0),d0			// read the physical longword
	movec	a1,%dtt0			// restore DTT0
	move	d1,sr			// restore interrupts
	rts
#endif
