|*
|* $Id$
|* 
|* XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
|*                                 1999 - 2003 H.Robbers
|*                                        2004 F.Naumann & O.Skancke
|*
|* A multitasking AES replacement for FreeMiNT
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
	.text

	.globl _xa_user_things

_xa_user_things:
	.long xa_user_end	- _xa_user_things	| Size of xa user things
	.long progdef_callout	- _xa_user_things	| address of progdef_callout function
	.long _userblk		- _xa_user_things	| store userblk address here
	.long _retcode		- _xa_user_things	| return code
	.long _parmblk		- _xa_user_things	| address of local parmblk

| This is all that is needed to test this with existing signal
| handling scheme I think...
progdef_callout:
	lea	_userblk(pc),a2		| address of userblk
	move.l	(a2),d0			| get address of the usrblk
	beq.s	nofunc			| exit if NULL
	move.l	d0,a2			| Address off A2
	move.l	(a2),d0			| Here is the callout function address
	beq.s	nofunc
	move.l	d0,a0
	pea	_parmblk(pc)
	jsr	(a0)
	addq.w	#4,sp
nofunc:
	lea	_retcode(pc),a0
	move.l	d0,(a0)
	rts

_userblk: dc.l 0
_retcode: dc.l 0
_parmblk: ds.w 60

xa_user_end:
