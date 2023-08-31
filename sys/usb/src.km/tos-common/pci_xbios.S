/* pci_xbios.S - PCI bios calls from _PCI cookie
 *
 * Reworked for USB TOS drivers by David Galvez 2019
 *
 * Didier Mequignon 2005-2007, e-mail: aniplay@wanadoo.fr
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mint/asmdefs.h"

	.globl SYM(tab_funcs_pci)
	.globl SYM(x_Find_pci_device)
	.globl SYM(x_Find_pci_classcode)
	.globl SYM(x_Read_config_byte)
	.globl SYM(x_Read_config_word)
	.globl SYM(x_Read_config_longword)
	.globl SYM(x_Fast_read_config_byte)
	.globl SYM(x_Fast_read_config_word)
	.globl SYM(x_Fast_read_config_longword)
	.globl SYM(x_Write_config_byte)
	.globl SYM(x_Write_config_word)
	.globl SYM(x_Write_config_longword)
	.globl SYM(x_Hook_interrupt)
	.globl SYM(x_Unhook_interrupt)
	.globl SYM(x_Special_cycle)
	.globl SYM(x_Get_routing)
	.globl SYM(x_Set_interrupt)
	.globl SYM(x_Get_resource)
	.globl SYM(x_Get_card_used)
	.globl SYM(x_Set_card_used)
	.globl SYM(x_Read_mem_byte)
	.globl SYM(x_Read_mem_word)
	.globl SYM(x_Read_mem_longword)
	.globl SYM(x_Fast_read_mem_byte)
	.globl SYM(x_Fast_read_mem_word)
	.globl SYM(x_Fast_read_mem_longword)
	.globl SYM(x_Write_mem_byte)
	.globl SYM(x_Write_mem_word)
	.globl SYM(x_Write_mem_longword)
	.globl SYM(x_Read_io_byte)
	.globl SYM(x_Read_io_word)
	.globl SYM(x_Read_io_longword)
	.globl SYM(x_Fast_read_io_byte)
	.globl SYM(x_Fast_read_io_word)
	.globl SYM(x_Fast_read_io_longword)
	.globl SYM(x_Write_io_byte)
	.globl SYM(x_Write_io_word)
	.globl SYM(x_Write_io_longword)
	.globl SYM(x_Get_machine_id)
	.globl SYM(x_Get_pagesize)
	.globl SYM(x_Virt_to_bus)
	.globl SYM(x_Bus_to_virt)
	.globl SYM(x_Virt_to_phys)
	.globl SYM(x_Phys_to_virt)

SYM(x_Find_pci_device):
	move.l 4(sp),d0          // ID
	move.w 8(sp),d1          // index
	move.l SYM(tab_funcs_pci),a1
	move.l (a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Find_pci_classcode):
	move.l 4(sp),d0          // class
	move.w 8(sp),d1          // index
	move.l SYM(tab_funcs_pci),a1
	move.l 4(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_config_byte):
	move.l 4(sp),d0          // handle
	move.w 8(sp),d1          // PCI register
	move.l 10(sp),a0         // pointer to space for read data
	move.l SYM(tab_funcs_pci),a1
	move.l 8(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_config_word):
	move.l 4(sp),d0          // handle
	move.w 8(sp),d1          // PCI register
	move.l 10(sp),a0         // pointer to space for read data
	move.l SYM(tab_funcs_pci),a1
	move.l 12(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_config_longword):
	move.l 4(sp),d0          // handle
	move.w 8(sp),d1          // PCI register
	move.l 10(sp),a0         // pointer to space for read data
	move.l SYM(tab_funcs_pci),a1
	move.l 16(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_config_byte):
	move.l 4(sp),d0          // handle
	move.w 8(sp),d1          // PCI register
	move.l SYM(tab_funcs_pci),a1
	move.l 20(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_config_word):
	move.l 4(sp),d0          // handle
	move.w 8(sp),d1          // PCI register
	move.l SYM(tab_funcs_pci),a1
	move.l 24(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_config_longword):
	move.l 4(sp),d0          // handle
	move.w 8(sp),d1          // PCI register
	move.l SYM(tab_funcs_pci),a1
	move.l 28(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_config_byte):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.w 12(sp),d1         // PCI register
	move.w 14(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 32(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_config_word):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.w 12(sp),d1         // PCI register
	move.w 14(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 36(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_config_longword):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.w 12(sp),d1         // PCI register
	move.l 14(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 40(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Hook_interrupt):
	move.l 4(sp),d0          // handle
	move.l 8(sp),a0          // pointer to interrupt handler
	move.l 12(sp),a1         // parameter for interrupt handler
	move.l a2,-(sp)
	move.l SYM(tab_funcs_pci),a2
	move.l 44(a2),a2
	move.l d2,-(sp)
	jsr (a2)
	move.l (sp)+,d2
	move.l (sp)+,a2
	rts

SYM(x_Unhook_interrupt):
	move.l 4(sp),d0          // handle
	move.l SYM(tab_funcs_pci),a1
	move.l 48(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Special_cycle):
	move.w 2(sp),d0          // bus number
	move.l 6(sp),d1          // special cycle data
	move.l SYM(tab_funcs_pci),a1
	move.l 52(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Get_routing):
	move.l 4(sp),d0          // handle
	move.l SYM(tab_funcs_pci),a1
	move.l 56(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Set_interrupt):
	move.l 4(sp),d0          // handle
	move.l SYM(tab_funcs_pci),a1
	move.l 60(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Get_resource):
	move.l 4(sp),d0          // handle
	move.l SYM(tab_funcs_pci),a1
	move.l 64(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Get_card_used):
	move.l 4(sp),d0          // handle
	move.l 8(sp),a0          // address
	move.l SYM(tab_funcs_pci),a1
	move.l 68(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Set_card_used):
	move.l 4(sp),d0          // handle
	move.l 8(sp),a0          // callback
	move.l SYM(tab_funcs_pci),a1
	move.l 72(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_mem_byte):
	movem.l 4(sp),d0-d1/a0    // handle, address to access (in PCI memory address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 76(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_mem_word):
	movem.l 4(sp),d0-d1/a0    // handle, address to access (in PCI memory address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 80(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_mem_longword):
	movem.l 4(sp),d0-d1/a0    // handle, address to access (in PCI memory address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 84(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_mem_byte):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI memory address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 88(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_mem_word):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI memory address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 92(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_mem_longword):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI memory address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 96(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_mem_byte):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.l 12(sp),d1         // address to access (in PCI memory address space)
	move.w 16(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 100(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_mem_word):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.l 12(sp),d1         // address to access (in PCI memory address space)
	move.w 16(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 104(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_mem_longword):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.l 12(sp),d1         // address to access (in PCI memory address space)
	move.l 16(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 108(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_io_byte):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l 12(sp),a0         // pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 112(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_io_word):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l 12(sp),a0         // pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 116(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Read_io_longword):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l 12(sp),a0         // pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 120(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_io_byte):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 124(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_io_word):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 128(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Fast_read_io_longword):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 132(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_io_byte):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.l 12(sp),d1         // address to access (in PCI I/O address space)
	move.w 16(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 136(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_io_word):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.l 12(sp),d1         // address to access (in PCI I/O address space)
	move.w 16(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 140(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Write_io_longword):
	move.l d2,-(sp)
	move.l 8(sp),d0          // handle
	move.l 12(sp),d1         // address to access (in PCI I/O address space)
	move.l 16(sp),d2         // data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 144(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Get_machine_id):
	move.l SYM(tab_funcs_pci),a1
	move.l 148(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Get_pagesize):
	move.l SYM(tab_funcs_pci),a1
	move.l 152(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Virt_to_bus):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address in virtual CPU space
	move.l 12(sp),a0         // ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 156(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Bus_to_virt):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // PCI bus address
	move.l 12(sp),a0         // ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 160(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Virt_to_phys):
	move.l 4(sp),d0          // address in virtual CPU space
	move.l 8(sp),a0          // ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 164(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(x_Phys_to_virt):
	move.l 4(sp),d0          // physical CPU address
	move.l 8(sp),a0          // ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 168(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts
