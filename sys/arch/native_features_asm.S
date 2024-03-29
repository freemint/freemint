/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * 
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2003-12-13
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifdef WITH_NATIVE_FEATURES

#include "mint/asmdefs.h"

	.text

	.globl	SYM(detect_native_features)

/* 
 * NatFeats test (routine retuns TRUE/FALSE (1/0) in D0)
 */
SYM(detect_native_features):

	clr.l	d0			// assume no NatFeats available
	move.l	sp,a1			// save the ssp
	move.l	(0x0010).w,a0		// illegal instruction vector
	move.l	#fail_natfeat,(0x0010).w

	nop				// flush pipelines (for 68040+)

	pea	(nf_version_name).w(pc)
	subq.l	#4,sp
	dc.w	0x7300			// Jump to NATFEAT_ID
	tst.l	d0
	beq.s	fail_natfeat
	moveq	#1,d0			// NatFeats detected

fail_natfeat:
	move.l	a1,sp
	move.l	a0,(0x0010).w

	nop				// flush pipelines (for 68040+)

	rts

nf_version_name:
	.ascii	"NF_VERSION\0"

# endif
