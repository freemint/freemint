//
// XaAES - XaAES Aint the AES (c) 1992 - 1998 C.Graham
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
	.text

	.globl _xa_user_things
	.globl _xa_callout_user

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
	addq.l	#4,sp
nofunc:
	lea	_retcode(pc),a0
	move.l	d0,(a0)
	rts

_userblk: dc.l 0
_retcode: dc.l 0
_parmblk: ds.w 60
xa_user_end:

_xa_callout_user:
	.long	_xa_callout_end		- _xa_callout_user
	.long	callout_user		- _xa_callout_user
	.long	callout_ret		- _xa_callout_user

callout_user:
#ifdef __mcoldfire__
	lea	-60(sp),sp
	movem.l	d0-d7/a0-a6,(sp)
#else
	movem.l	d0-d7/a0-a6,-(sp)
#endif
	lea	callout_func(pc),a0
	move.l	(a0)+,d0
	beq.s	callout_exit
	moveq	#0,d1
	move.w	(a0)+,d1
	beq.s	callout_noparms
	adda.l	d1,a0
	adda.l	d1,a0
#ifdef __mcoldfire__
	subq.l	#1,d1
#else
	subq.w	#1,d1
#endif
callout_gparms:
	move.w	-(a0),-(sp)
#ifdef __mcoldfire__
	subq.l	#1,d1
	bpl.s	callout_gparms
#else
	dbra	d1,callout_gparms
#endif
callout_noparms:
	move.l	d0,a0
	jsr	(a0)
	lea	callout_ret(pc),a0
	move.l	d0,(a0)
	moveq	#0,d1
	move.w	8(a0),d1
	adda.l	d1,sp
	adda.l	d1,sp
callout_exit:
#ifdef __mcoldfire__
	movem.l	(sp),d0-d7/a0-a6
	lea	60(sp),sp
#else
	movem.l	(sp)+,d0-d7/a0-a6
#endif
	rts

callout_ret:	dc.l 0
callout_func:	dc.l 0
callout_plen:	dc.w 0
callout_parm:
_xa_callout_end:
