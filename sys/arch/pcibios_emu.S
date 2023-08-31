/*
 * Copyright 2014 David Galvez.
 * Based on SCSIDRV sys_emu implementation by Frank Naumann.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifdef PCI_BIOS

#include "mint/asmdefs.h"

	.text

	.globl SYM(emu_pcibios_Find_pci_device)
	.globl SYM(emu_pcibios_Find_pci_classcode)
	.globl SYM(emu_pcibios_Read_config_byte)
	.globl SYM(emu_pcibios_Read_config_word)
	.globl SYM(emu_pcibios_Read_config_longword)
	.globl SYM(emu_pcibios_Fast_read_config_byte)
	.globl SYM(emu_pcibios_Fast_read_config_word)
	.globl SYM(emu_pcibios_Fast_read_config_longword)
	.globl SYM(emu_pcibios_Write_config_byte)
	.globl SYM(emu_pcibios_Write_config_word)
	.globl SYM(emu_pcibios_Write_config_longword)
	.globl SYM(emu_pcibios_Hook_interrupt)
	.globl SYM(emu_pcibios_Unhook_interrupt)
	.globl SYM(emu_pcibios_Special_cycle)
	.globl SYM(emu_pcibios_Get_routing)
	.globl SYM(emu_pcibios_Set_interrupt)
	.globl SYM(emu_pcibios_Get_resource)
	.globl SYM(emu_pcibios_Get_card_used)
	.globl SYM(emu_pcibios_Set_card_used)
	.globl SYM(emu_pcibios_Read_mem_byte)
	.globl SYM(emu_pcibios_Read_mem_word)
	.globl SYM(emu_pcibios_Read_mem_longword)
	.globl SYM(emu_pcibios_Fast_read_mem_byte)
	.globl SYM(emu_pcibios_Fast_read_mem_word)
	.globl SYM(emu_pcibios_Fast_read_mem_longword)
	.globl SYM(emu_pcibios_Write_mem_byte)
	.globl SYM(emu_pcibios_Write_mem_word)
	.globl SYM(emu_pcibios_Write_mem_longword)
	.globl SYM(emu_pcibios_Read_io_byte)
	.globl SYM(emu_pcibios_Read_io_word)
	.globl SYM(emu_pcibios_Read_io_longword)
	.globl SYM(emu_pcibios_Fast_read_io_byte)
	.globl SYM(emu_pcibios_Fast_read_io_word)
	.globl SYM(emu_pcibios_Fast_read_io_longword)
	.globl SYM(emu_pcibios_Write_io_byte)
	.globl SYM(emu_pcibios_Write_io_word)
	.globl SYM(emu_pcibios_Write_io_longword)
	.globl SYM(emu_pcibios_Get_machine_id)
	.globl SYM(emu_pcibios_Get_pagesize)
	.globl SYM(emu_pcibios_Virt_to_bus)
	.globl SYM(emu_pcibios_Bus_to_virt)
	.globl SYM(emu_pcibios_Virt_to_phys)
	.globl SYM(emu_pcibios_Phys_to_virt)

SYM(emu_pcibios_Find_pci_device):
	move.l	d3,-(sp)
	move.l	#2,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Find_pci_classcode):
	move.l	d3,-(sp)
	moveq	#3,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Read_config_byte):
	move.l	d3,-(sp)
	moveq	#4,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Read_config_word):
	move.l	d3,-(sp)
	moveq	#5,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Read_config_longword):
	move.l	d3,-(sp)
	moveq	#6,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Fast_read_config_byte):
	move.l	d3,-(sp)
	moveq	#7,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Fast_read_config_word):
	move.l	d3,-(sp)
	moveq	#8,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Fast_read_config_longword):
	move.l	d3,-(sp)
	moveq	#9,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Write_config_byte):
	move.l	d3,-(sp)
	moveq	#10,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Write_config_word):
	move.l	d3,-(sp)
	moveq	#11,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Write_config_longword):
	move.l	d3,-(sp)
	moveq	#12,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Hook_interrupt):
	move.l	d3,-(sp)
	moveq	#13,d3
	bra	_do_trap_daa

SYM(emu_pcibios_Unhook_interrupt):
	move.l	d3,-(sp)
	moveq	#14,d3
	bra	_do_trap_d

SYM(emu_pcibios_Special_cycle):
	move.l	d3,-(sp)
	moveq	#15,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Get_routing):
	move.l	d3,-(sp)
	moveq	#16,d3
	bra	_do_trap

SYM(emu_pcibios_Set_interrupt):
	move.l	d3,-(sp)
	moveq	#17,d3
	bra	_do_trap

SYM(emu_pcibios_Get_resource):
	move.l	d3,-(sp)
	moveq	#18,d3
	bra	_do_trap_d

SYM(emu_pcibios_Get_card_used):
	move.l	d3,-(sp)
	moveq	#19,d3
	bra	_do_trap_da

SYM(emu_pcibios_Set_card_used):
	move.l	d3,-(sp)
	moveq	#20,d3
	bra	_do_trap_da

SYM(emu_pcibios_Read_mem_byte):
	move.l	d3,-(sp)
	moveq	#21,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Read_mem_word):
	move.l	d3,-(sp)
	moveq	#22,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Read_mem_longword):
	move.l	d3,-(sp)
	moveq	#23,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Fast_read_mem_byte):
	move.l	d3,-(sp)
	moveq	#24,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Fast_read_mem_word):
	move.l	d3,-(sp)
	moveq	#25,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Fast_read_mem_longword):
	move.l	d3,-(sp)
	moveq	#26,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Write_mem_byte):
	move.l	d3,-(sp)
	moveq	#27,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Write_mem_word):
	move.l	d3,-(sp)
	moveq	#28,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Write_mem_longword):
	move.l	d3,-(sp)
	moveq	#29,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Read_io_byte):
	move.l	d3,-(sp)
	moveq	#30,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Read_io_word):
	move.l	d3,-(sp)
	moveq	#31,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Read_io_longword):
	move.l	d3,-(sp)
	moveq	#32,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Fast_read_io_byte):
	move.l	d3,-(sp)
	moveq	#33,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Fast_read_io_word):
	move.l	d3,-(sp)
	moveq	#34,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Fast_read_io_longword):
	move.l	d3,-(sp)
	moveq	#35,d3
	bra	_do_trap_dd

SYM(emu_pcibios_Write_io_byte):
	move.l	d3,-(sp)
	moveq	#36,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Write_io_word):
	move.l	d3,-(sp)
	moveq	#37,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Write_io_longword):
	move.l	d3,-(sp)
	moveq	#38,d3
	bra	_do_trap_ddd

SYM(emu_pcibios_Get_machine_id):
	move.l	d3,-(sp)
	moveq	#39,d3
	bra	_do_trap_d

SYM(emu_pcibios_Get_pagesize):
	move.l	d3,-(sp)
	moveq	#40,d3
	bra	_do_trap

SYM(emu_pcibios_Virt_to_bus):
	move.l	d3,-(sp)
	moveq	#41,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Bus_to_virt):
	move.l	d3,-(sp)
	moveq	#42,d3
	bra	_do_trap_dda

SYM(emu_pcibios_Virt_to_phys):
	move.l	d3,-(sp)
	moveq	#43,d3
	bra	_do_trap_da

SYM(emu_pcibios_Phys_to_virt):
	move.l	d3,-(sp)
	moveq	#44,d3
	bra	_do_trap_da


_do_trap:
	lea	-28(sp),sp	//  ensure that stack space is equal to all
	bra	_call_sys_emu

_do_trap_d:
	lea	-24(sp),sp	//  ensure that stack space is equal to all
				//  arguments of sys_emu
	move.l	d0,-(sp)
	bra	_call_sys_emu

_do_trap_dd:
	lea	-20(sp),sp	//  ensure that stack space is equal to all
				//  arguments of sys_emu
	move.l	d1,-(sp)
	move.l	d0,-(sp)
	bra	_call_sys_emu

_do_trap_ddd:
	lea	-16(sp),sp	//  ensure that stack space is equal to all
				//  arguments of sys_emu
	move.l	d2,-(sp)
	move.l	d1,-(sp)
	move.l	d0,-(sp)
	bra	_call_sys_emu

_do_trap_da:
	lea	-20(sp),sp	//  ensure that stack space is equal to all
				//  arguments of sys_emu
	move.l	a0,-(sp)
	move.l	d0,-(sp)
	bra	_call_sys_emu

_do_trap_dda:
	lea	-16(sp),sp	//  ensure that stack space is equal to all
				//  arguments of sys_emu
	move.l	a0,-(sp)
	move.l	d1,-(sp)
	move.l	d0,-(sp)
	bra	_call_sys_emu

_do_trap_daa:
	lea	-16(sp),sp	//  ensure that stack space is equal to all
				//  arguments of sys_emu
	move.l	a1,-(sp)
	move.l	a0,-(sp)
	move.l	d0,-(sp)
	bra	_call_sys_emu

_call_sys_emu:
	move.w	d3,-(sp)
	move.w	#0x0003,-(sp)	//  we want PCI-BIOS EMU
	move.w	#0x015f,-(sp)	//  system EMU
	trap	#1		//  do the GEMDOS trap
	lea	34(sp),sp	//  correct stack
	move.l	(sp)+,d3
	rts

#endif /* PCI-BIOS */
