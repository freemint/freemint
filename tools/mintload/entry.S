/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)

	.globl	__start
	.globl	SYM(_base)
	.globl	SYM(_crtinit)

	.bss
SYM(_base):
	ds.l	1

	.text

/*
 * not a C symbol: this is directly referenced in the Makefile
 */
__start:
	move.l	0x4(%sp),%a0		// basepage address
	move.l	%a0,SYM(_base)

	movel	%a0@(4),%d0		// get _base->p_hitpa
	bclr	#0,%d0			// round off
	movel	%d0,%sp			// set stack (temporarily)
	jmp	SYM(_crtinit)		// in crtinit.c
