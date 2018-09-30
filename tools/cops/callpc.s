	.globl rsh_fix
	.globl rsh_fix_wrap

rsh_fix_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	rsh_fix
	addq.l	#4,sp
	rts


	.globl rsh_obfix
	.globl rsh_obfix_wrap

rsh_obfix_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	rsh_obfix
	addq.l	#4,sp
	rts


	.globl Xform_do
	.globl Xform_do_wrap

Xform_do_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	Xform_do
	addq.l	#4,sp
	rts


	.globl GetFirstRect
	.globl GetFirstRect_wrap

GetFirstRect_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	GetFirstRect
	addq.l	#4,sp
	rts


	.globl GetNextRect
	.globl GetNextRect_wrap

GetNextRect_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	GetNextRect
	addq.l	#4,sp
	rts


	.globl Set_Evnt_Mask
	.globl Set_Evnt_Mask_wrap

Set_Evnt_Mask_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	Set_Evnt_Mask
	addq.l	#4,sp
	rts


	.globl CPX_Save
	.globl CPX_Save_wrap

CPX_Save_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	CPX_Save
	addq.l	#4,sp
	rts


	.globl Get_Buffer
	.globl Get_Buffer_wrap

Get_Buffer_wrap:
	move.l	sp,a0
	move.l	a0,-(sp)
	bsr	Get_Buffer
	addq.l	#4,sp
	rts
