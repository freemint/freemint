|*
|* $Id$
|* 
|* XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
|*                                 1999 - 2003 H.Robbers
|*                                        2004 F.Naumann
|*
|* A multitasking AES replacement for MiNT
|*
|* This file is part of XaAES.
|*
|* XaAES is free software; you can redistribute it and/or modify
|* it under the terms of the GNU General Public License as published by
|* the Free Software Foundation; either version 2 of the License, or
|* (at your option) any later version.
|*
|* XaAES is distributed in the hope that it will be useful,
|* but WITHOUT ANY WARRANTY; without even the implied warranty of
|* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|* GNU General Public License for more details.
|*
|* You should have received a copy of the GNU General Public License
|* along with XaAES; if not, write to the Free Software
|* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
|*
|-------------------------------------------------------------------------------------
| AES/VDI (Trap 2)  Handler
|-------------------------------------------------------------------------------------
| This mini handler just calls the main handler (written in C) or fields VDI
| commands out to the old vector (which was saved when we hooked trap 2 in the first
| place).
| Perhaps the whole trap handler should be in assembler, but really, it's fast enough
| for me just written in C with this small kludge routine to handle the actual
| exception itself. If anyone wants to recode it totally in assembler, the main Trap
| to pipe interface in in HANDLER.C - I'll happily put the mods into a new release.
| - Self modifying code removed [13/2/96] by Martin koeling.
| - Made XBRA compliant [13/2/96] by Martin Koeling.
| - AES trap code streamlined [980629] by Johan Klockars.
|-------------------------------------------------------------------------------------

	.globl	_XA_handler

	.globl	_old_trap2_vector
	.globl	_handler

	.text

| XBRA structure immediately before the new vector address:
	dc.l    0x58425241		| XBRA magic
	dc.l	0x58614145		| 'XaAE' XBRA id (just a proposal)
_old_trap2_vector:
	dc.l	0

| Exception vector goes to here...
_handler:
	cmp.w	#200,d0         	| Both 0xfffe (vq_gdos) and $73 (vdi calls) are less than 200($c8)
	blt.s	_not_aes

	movem.l	d0-d2/a0-a2,-(sp)	| save regs for jumping to C

	move.l	d1,-(sp)		| push args to stack
	move.w	d0,-(sp)

	jsr 	_XA_handler		| Call the real handler written in C

	addq.w	#6,sp			| correct stack pointer

	tst 	d0
	bge 	done			| XaAES returns -1 here if it is compiled without
					| fileselector and the fileselector is called.
					| This allows fileselectors to be started *Before* Mint & XaAES.
					| E.g. Boxkite
	movem.l	(sp)+,a0-a2/d0-d2	| ******
	bra 	_not_aes	

done:
	movem.l	(sp)+,a0-a2/d0-d2	| ^^^^^^
	clr.w	d0			| Ordinary GEM does this - so we do it as well...
	rte

| New version of the call to the original VDI/AES vector
| [13/2/96] - Martin Koehling
| This is a jump to the old VDI/AES vector. No self-modifying code
| here - _old_trap_vector is data, not code... :-)
_not_aes:
	move.l	_old_trap2_vector,-(sp)
	rts
