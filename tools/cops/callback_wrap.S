|*
|* COPS (c) 1995 - 2003 Sven & Wilfried Behne
|*                 2004 F.Naumann & O.Skancke
|*
|* A XCONTROL compatible CPX manager.
|*
|* This file is part of COPS.
|*
|* COPS is free software; you can redistribute it and/or modify
|* it under the terms of the GNU General Public License as published by
|* the Free Software Foundation; either version 2 of the License, or
|* (at your option) any later version.
|*
|* COPS is distributed in the hope that it will be useful,
|* but WITHOUT ANY WARRANTY; without even the implied warranty of
|* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|* GNU General Public License for more details.
|*
|* You should have received a copy of the GNU General Public License
|* along with XaAES; if not, write to the Free Software
|* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
|*

	.globl _rsh_fix
	.globl _rsh_fix_wrap

_rsh_fix_wrap:
|	move.l	sp,a0
|	move.l	a0,-(sp)
	move.l	sp,-(sp)
	jbsr	_rsh_fix
	addq.l	#4,sp
	rts


	.globl _rsh_obfix
	.globl _rsh_obfix_wrap

_rsh_obfix_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	jbsr	_rsh_obfix
	addq.l	#4,sp
	rts


	.globl _Xform_do
	.globl _Xform_do_wrap

_Xform_do_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	jbsr	_Xform_do
	addq.l	#4,sp
	rts


	.globl _GetFirstRect
	.globl _GetFirstRect_wrap

_GetFirstRect_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	jbsr	_GetFirstRect
	addq.l	#4,sp
	rts


	.globl _GetNextRect
	.globl _GetNextRect_wrap

_GetNextRect_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	jbsr	_GetNextRect
	addq.l	#4,sp
	rts


	.globl _Set_Evnt_Mask
	.globl _Set_Evnt_Mask_wrap

_Set_Evnt_Mask_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	jbsr	_Set_Evnt_Mask
	addq.l	#4,sp
	rts


	.globl _CPX_Save
	.globl _CPX_Save_wrap

_CPX_Save_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	jbsr	_CPX_Save
	addq.l	#4,sp
	rts


	.globl _Get_Buffer
	.globl _Get_Buffer_wrap

_Get_Buffer_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	jbsr	_Get_Buffer
	addq.l	#4,sp
	rts
