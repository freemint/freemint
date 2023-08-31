//
// XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
//                                 1999 - 2003 H.Robbers
//                                        2004 F.Naumann & O.Skancke
//
// A multitasking AES replacement for FreeMiNT
//
// This file is part of XaAES.
//
// XaAES is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// XaAES is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XaAES; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)

	.globl	SYM(accstart)
	.globl	SYM(accend)

	.text

| Accessory startup code poached from oAESis

SYM(accstart):
	move.l	4(%sp),%a0		| pointer to basepage
	clr.l	36(%a0)			| HR: clear pointer to parent basepage
	move.l	16(%a0),%a1		| data seg
	move.l	%a1,8(%a0)		| --> text seg (reverse xa_shell action)
	add.l	12(%a0),%a1		| text + textlen
	move.l	%a1,16(%a0)		| --> data seg
	move.l	8(%a0),%a1		| start of text segment
	jmp	(%a1)			| jump to text
SYM(accend):
