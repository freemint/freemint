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
	.globl _xa_co_wdlgexit

	.globl _xa_co_lboxselect
	.globl _xa_co_lboxset
	.globl _xa_co_lboxscroll

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

_xa_co_wdlgexit:
	.long xa_co_we_end	- _xa_co_wdlgexit	| len
	.long co_wdlg_exit	- _xa_co_wdlgexit	| sighand_p
	.long _wdlgexit		- _xa_co_wdlgexit	| wdlg exit function
	.long _userdata		- _xa_co_wdlgexit
	.long _mclicks		- _xa_co_wdlgexit
	.long _nxtobj		- _xa_co_wdlgexit
	.long _ev		- _xa_co_wdlgexit
	.long _handle		- _xa_co_wdlgexit
	.long _wdlgexit_ret	- _xa_co_wdlgexit
	.long _wdlgexit_fb	- _xa_co_wdlgexit

co_wdlg_exit:
	movem.l	d1-d7/a0-a6,-(sp)
	lea	_wdlgexit(pc),a0
	move.l	(a0)+,d0
	beq.s	no_wexit
	move.l	(a0)+,-(sp)
	move.w	(a0)+,-(sp)
	move.w	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0),-(sp)
	move.l	d0,a0
	jsr	(a0)
	lea	16(sp),sp
no_wexit:
	lea	_wdlgexit_ret(pc),a0
	move.w	d0,(a0)
	movem.l (sp)+,d1-d7/a0-a6
	rts
	
_wdlgexit:	dc.l 0
_userdata:	dc.l 0
_mclicks:	dc.w 0
_nxtobj:	dc.w 0
_ev:		dc.l 0
_handle:	dc.l 0
_wdlgexit_ret:	dc.w 0
_wdlgexit_fb:	dc.l 4

xa_co_we_end:
| --------------------------------------------------------------|
| --------------------  LBOX CALLOUTS --------------------------|
|---------------------------------------------------------------|
_xa_co_lboxselect:
	.long _lboxsel_end	- _xa_co_lboxselect	| len
	.long co_lboxselect	- _xa_co_lboxselect	| sighand
	.long _lboxsel_func	- _xa_co_lboxselect	| parms

co_lboxselect:
	movem.l	d0-d7/a0-a6,-(sp)
	lea	_lboxsel_func(pc),a0
	move.l	(a0)+,d0
	beq.s	lboxsel_exit	
	move.w	(a0)+,-(sp)
	move.w	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0),-(sp)
	move.l	d0,a0
	jsr	(a0)
	lea	20(sp),sp	
lboxsel_exit:
	movem.l	(sp)+,d0-d7/a0-a6
	rts
_lboxsel_func:		dc.l 0
_lboxsel_lstate:	dc.w 0
_lboxsel_obj_ind:	dc.w 0
_lboxsel_usrdata:	dc.l 0
_lboxsel_item:		dc.l 0
_lboxsel_tree:		dc.l 0
_lboxsel_box:		dc.l 0
_lboxsel_end:

| --------- set --------- |
_xa_co_lboxset:
	.long _lboxset_end	- _xa_co_lboxset
	.long co_lboxset	- _xa_co_lboxset
	.long _lboxset_func	- _xa_co_lboxset

co_lboxset:
	movem.l d0-d7/a0-a6,-(sp)
	lea	_lboxset_func(pc),a0
	move.l	(a0)+,d0
	beq.s	exit_lboxset
	move.w	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.w	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0),-(sp)
	move.l	d0,a0
	jsr	(a0)
	lea	24(sp),sp
exit_lboxset:
	lea	_lboxset_ret(pc),a0
	move.l	d0,(a0)
	movem.l	(sp)+,d0-d7/a0-a6
	rts
_lboxset_func:		dc.l 0
_lboxset_first:		dc.w 0
_lboxset_rect:		dc.l 0
_lboxset_usrdata:	dc.l 0
_lboxset_obj_ind:	dc.w 0
_lboxset_item:		dc.l 0
_lboxset_tree:		dc.l 0
_lboxset_box:		dc.l 0
_lboxset_ret:		dc.l 0
_lboxset_end:

| ------------------ scroll ---------------- |
_xa_co_lboxscroll:
	.long _lboxscrl_end	- _xa_co_lboxscroll
	.long co_lboxscrl	- _xa_co_lboxscroll
	.long _lboxscrl_func	- _xa_co_lboxscroll
co_lboxscrl:
	movem.l	d0-d7/a0-a6,-(sp)
	lea	_lboxscrl_func(pc),a0
	move.l	(a0)+,d0
	beq.s	exit_lboxscrl
	move.w	(a0)+,-(sp)
	move.l	(a0)+,-(sp)
	move.l	(a0),-(sp)
	move.l	d0,a0
	jsr	(a0)
	lea	10(sp),sp
exit_lboxscrl:
	lea	_lboxscrl_ret(pc),a0
	move.l	d0,(a0)
	movem.l	(sp)+,d0-d7/a0-a6
	rts
_lboxscrl_func:		dc.l 0
_lboxscrl_n:		dc.w 0
_lboxscrl_lboxslide:	dc.l 0
_lboxscrl_box:		dc.l 0
_lboxscrl_ret:		dc.l 0
_lboxscrl_end:

|typedef	void  _cdecl lbox_select(LIST_BOX *box,
|				OBJECT *tree,
|				struct lbox_item *item,
|				void *user_data,
|				short obj_index,
|				short last_state);
|typedef	short _cdecl lbox_set	(LIST_BOX *box,
|				OBJECT *tree,
|				struct lbox_item *item,
|				short obj_index,
|				void *user_data,
|				GRECT *rect,
|				short first);
|
|typedef bool _cdecl lbox_scroll	(struct xa_lbox_info *lbox,
|				 struct lbox_slide *s,
|				 short n);
|
