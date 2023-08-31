/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * cannot include mint/asmdefs.h here,
 * because this source is also used by programs that link to
 * mintlib, and thus cannot add any kernel include directories
 * because they conflict.
 *
 * Redefine a minimal set of macros here
 */
 
#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)

/*
 * _setstack - changes the stack pointer
 *
 * Prototype:
 * void _setstack(void *newsp);
 *
 * WARNING: after this local variables may no longer be valid!
 *          destroys a0 and a7 too
 */

	.globl	SYM(_setstack)

	.text

SYM(_setstack):
	movel	%sp@+,%a0		// save return
	movel	%sp@,%sp		// new stack pointer
	subql	#4,%sp		// fixup
	jmp	%a0@		// and jump back
