/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation  either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program  if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-03-31
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

	.text

	.globl	_emu_scsidrv_In
	.globl	_emu_scsidrv_Out
	.globl	_emu_scsidrv_InquireSCSI
	.globl	_emu_scsidrv_InquireBUS
	.globl	_emu_scsidrv_CheckDev
	.globl	_emu_scsidrv_RescanBus
	.globl	_emu_scsidrv_Open
	.globl	_emu_scsidrv_Close
	.globl	_emu_scsidrv_Error
	.globl	_emu_scsidrv_Install
	.globl	_emu_scsidrv_Deinstall
	.globl	_emu_scsidrv_GetCmd
	.globl	_emu_scsidrv_SendData
	.globl	_emu_scsidrv_GetData
	.globl	_emu_scsidrv_SendStatus
	.globl	_emu_scsidrv_SendMsg
	.globl	_emu_scsidrv_GetMsg

_emu_scsidrv_In:
	moveq	#1,d0
	bra.s	_do_trap

_emu_scsidrv_Out:
	moveq	#2,d0
	bra.s	_do_trap
	
_emu_scsidrv_InquireSCSI:
	moveq	#3,d0
	bra.s	_do_trap

_emu_scsidrv_InquireBUS:
	moveq	#4,d0
	bra.s	_do_trap

_emu_scsidrv_CheckDev:
	moveq	#5,d0
	bra.s	_do_trap

_emu_scsidrv_RescanBus:
	moveq	#6,d0
	bra.s	_do_trap

_emu_scsidrv_Open:
	moveq	#7,d0
	bra.s	_do_trap

_emu_scsidrv_Close:
	moveq	#8,d0
	bra.s	_do_trap

_emu_scsidrv_Error:
	moveq	#9,d0
	bra.s	_do_trap

_emu_scsidrv_Install:
	moveq	#10,d0
	bra.s	_do_trap

_emu_scsidrv_Deinstall:
	moveq	#11,d0
	bra.s	_do_trap

_emu_scsidrv_GetCmd:
	moveq	#12,d0
	bra.s	_do_trap

_emu_scsidrv_SendData:
	moveq	#13,d0
	bra.s	_do_trap

_emu_scsidrv_GetData:
	moveq	#14,d0
	bra.s	_do_trap

_emu_scsidrv_SendStatus:
	moveq	#15,d0
	bra.s	_do_trap

_emu_scsidrv_SendMsg:
	moveq	#16,d0
	bra.s	_do_trap

_emu_scsidrv_GetMsg:
	moveq	#17,d0
 	/* bra.s	_do_trap */

_do_trap:
	move.l	sp,a0
	move.l	a2,-(sp)	//  we must preserve a2, PureC convention
	lea	-12(sp),sp	//  ensure that stack space is equal to all
	move.l	16(a0),-(sp)	//  arguments of sys_emu
	move.l	12(a0),-(sp)
	move.l	8(a0),-(sp)
	move.l	4(a0),-(sp)
	move.w	d0,-(sp)	//  number of the emulation call
	move.w	#0x0002,-(sp)	//  we want SCSIDRV EMU
	move.w	#0x015f,-(sp)	//  system EMU
	trap	#1		//  do the GEMDOS trap
	lea	34(sp),sp	//  correct stack
	move.l	(sp)+,a2	//  restore a2
	rts
