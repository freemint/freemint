/*
 * Ported to FreeMiNT by David Galvez. 2014.
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
	.globl SYM(Find_pci_device)
	.globl SYM(Find_pci_classcode)
	.globl SYM(Read_config_byte)
	.globl SYM(Read_config_word)
	.globl SYM(Read_config_longword)
	.globl SYM(Fast_read_config_byte)
	.globl SYM(Fast_read_config_word)
	.globl SYM(Fast_read_config_longword)
	.globl SYM(Write_config_byte)
	.globl SYM(Write_config_word)
	.globl SYM(Write_config_longword)
	.globl SYM(Hook_interrupt)
	.globl SYM(Unhook_interrupt)
	.globl SYM(Special_cycle)
	.globl SYM(Get_routing)
	.globl SYM(Set_interrupt)
	.globl SYM(Get_resource)
	.globl SYM(Get_card_used)
	.globl SYM(Set_card_used)
	.globl SYM(Read_mem_byte)
	.globl SYM(Read_mem_word)
	.globl SYM(Read_mem_longword)
	.globl SYM(Fast_read_mem_byte)
	.globl SYM(Fast_read_mem_word)
	.globl SYM(Fast_read_mem_longword)
	.globl SYM(Write_mem_byte)
	.globl SYM(Write_mem_word)
	.globl SYM(Write_mem_longword)
	.globl SYM(Read_io_byte)
	.globl SYM(Read_io_word)
	.globl SYM(Read_io_longword)
	.globl SYM(Fast_read_io_byte)
	.globl SYM(Fast_read_io_word)
	.globl SYM(Fast_read_io_longword)
	.globl SYM(Write_io_byte)
	.globl SYM(Write_io_word)
	.globl SYM(Write_io_longword)
	.globl SYM(Get_machine_id)
	.globl SYM(Get_pagesize)
	.globl SYM(Virt_to_bus)
	.globl SYM(Bus_to_virt)
	.globl SYM(Virt_to_phys)
	.globl SYM(Phys_to_virt)

#ifdef PCI_BIOS

SYM(Find_pci_device):
	move.l 4(sp),d0          // ID
	move.l 8(sp),d1          // index
	move.l SYM(tab_funcs_pci),a1
	move.l (a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Find_pci_classcode):
	move.l 4(sp),d0          // class
	move.l 8(sp),d1          // index
	move.l SYM(tab_funcs_pci),a1
	move.l 4(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_config_byte):
	movem.l 4(sp),d0-d1/a0   // handle, PCI register, pointer to space for read data
	move.l SYM(tab_funcs_pci),a1
	move.l 8(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_config_word):
	movem.l 4(sp),d0-d1/a0   // handle, PCI register, pointer to space for read data
	move.l SYM(tab_funcs_pci),a1
	move.l 12(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_config_longword):
	movem.l 4(sp),d0-d1/a0   // handle, PCI register, pointer to space for read data
	move.l SYM(tab_funcs_pci),a1
	move.l 16(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_config_byte):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // PCI register
	move.l SYM(tab_funcs_pci),a1
	move.l 20(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_config_word):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // PCI register
	move.l SYM(tab_funcs_pci),a1
	move.l 24(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_config_longword):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // PCI register
	move.l SYM(tab_funcs_pci),a1
	move.l 28(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_config_byte):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, PCI register, data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 32(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_config_word):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, PCI register, data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 36(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_config_longword):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, PCI register, data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 40(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Hook_interrupt):
	movem.l 4(sp),d0/a0/a1   // handle, pointer to interrupt handler, parameter for interrupt handler
	move.l a2,-(sp)
	move.l SYM(tab_funcs_pci),a2
	move.l 44(a2),a2
	move.l d2,-(sp)
	jsr (a2)
	move.l (sp)+,d2
	move.l (sp)+,a2
	rts

SYM(Unhook_interrupt):
	move.l 4(sp),d0          // handle
	move.l SYM(tab_funcs_pci),a1
	move.l 48(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Special_cycle):
	move.l 4(sp),d0          // bus number
	move.l 8(sp),d1          // special cycle data
	move.l SYM(tab_funcs_pci),a1
	move.l 52(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Get_routing):
	move.l 4(sp),d0          // handle
	move.l SYM(tab_funcs_pci),a1
	move.l 56(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Set_interrupt):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // mode
	move.l SYM(tab_funcs_pci),a1
	move.l 60(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Get_resource):
	move.l 4(sp),d0          // handle
	move.l SYM(tab_funcs_pci),a1
	move.l 64(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Get_card_used):
	move.l 4(sp),d0          // handle
	move.l 8(sp),a0          // address
	move.l SYM(tab_funcs_pci),a1
	move.l 68(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Set_card_used):
	move.l 4(sp),d0          // handle
	move.l 8(sp),a0          // callback
	move.l SYM(tab_funcs_pci),a1
	move.l 72(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_mem_byte):
	movem.l 4(sp),d0-d1/a0    // handle, address to access (in PCI memory address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 76(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_mem_word):
	movem.l 4(sp),d0-d1/a0    // handle, address to access (in PCI memory address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 80(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_mem_longword):
	movem.l 4(sp),d0-d1/a0    // handle, address to access (in PCI memory address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 84(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_mem_byte):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI memory address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 88(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_mem_word):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI memory address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 92(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_mem_longword):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI memory address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 96(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_mem_byte):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, address to access (in PCI memory address space), data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 100(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_mem_word):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, address to access (in PCI memory address space), data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 104(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_mem_longword):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, address to access (in PCI memory address space), data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 108(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_io_byte):
	movem.l 4(sp),d0-d1/a0   // handle, address to access (in PCI I/O address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 112(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_io_word):
	movem.l 4(sp),d0-d1/a0   // handle, address to access (in PCI I/O address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 116(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Read_io_longword):
	movem.l 4(sp),d0-d1/a0   // handle, address to access (in PCI I/O address space), pointer to data in memory
	move.l SYM(tab_funcs_pci),a1
	move.l 120(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_io_byte):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 124(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_io_word):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 128(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Fast_read_io_longword):
	move.l 4(sp),d0          // handle
	move.l 8(sp),d1          // address to access (in PCI I/O address space)
	move.l SYM(tab_funcs_pci),a1
	move.l 132(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_io_byte):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, address to access (in PCI I/O address space), data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 136(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_io_word):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, address to access (in PCI I/O address space), data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 140(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Write_io_longword):
	move.l d2,-(sp)
	movem.l 8(sp),d0-d2      // handle, address to access (in PCI I/O address space), data to write
	move.l SYM(tab_funcs_pci),a1
	move.l 144(a1),a1
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Get_machine_id):
	move.l SYM(tab_funcs_pci),a1
	move.l 148(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Get_pagesize):
	move.l SYM(tab_funcs_pci),a1
	move.l 152(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Virt_to_bus):
	movem.l 4(sp),d0-d1/a0   // handle, address in virtual CPU space, ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 156(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Bus_to_virt):
	movem.l 4(sp),d0-d1/a0   // handle, PCI bus address, ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 160(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Virt_to_phys):
	move.l 4(sp),d0          // address in virtual CPU space  
	move.l 8(sp),a0          // ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 164(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

SYM(Phys_to_virt):
	move.l 4(sp),d0          // physical CPU address 
	move.l 8(sp),a0          // ptr
	move.l SYM(tab_funcs_pci),a1
	move.l 168(a1),a1
	move.l d2,-(sp)
	jsr (a1)
	move.l (sp)+,d2
	rts

#else /* Needed by kentry when PCI_BIOS is undefined */

	.globl SYM(pcibios_installed)

SYM(Find_pci_device):
SYM(Find_pci_classcode):
SYM(Read_config_byte):
SYM(Read_config_word):
SYM(Read_config_longword):
SYM(Fast_read_config_byte):
SYM(Fast_read_config_word):
SYM(Fast_read_config_longword):
SYM(Write_config_byte):
SYM(Write_config_word):
SYM(Write_config_longword):
SYM(Hook_interrupt):
SYM(Unhook_interrupt):
SYM(Special_cycle):
SYM(Get_routing):
SYM(Set_interrupt):
SYM(Get_resource):
SYM(Get_card_used):
SYM(Set_card_used):
SYM(Read_mem_byte):
SYM(Read_mem_word):
SYM(Read_mem_longword):
SYM(Fast_read_mem_byte):
SYM(Fast_read_mem_word):
SYM(Fast_read_mem_longword):
SYM(Write_mem_byte):
SYM(Write_mem_word):
SYM(Write_mem_longword):
SYM(Read_io_byte):
SYM(Read_io_word):
SYM(Read_io_longword):
SYM(Fast_read_io_byte):
SYM(Fast_read_io_word):
SYM(Fast_read_io_longword):
SYM(Write_io_byte):
SYM(Write_io_word):
SYM(Write_io_longword):
SYM(Get_machine_id):
SYM(Get_pagesize):
SYM(Virt_to_bus):
SYM(Bus_to_virt):
SYM(Virt_to_phys):
SYM(Phys_to_virt):
	move.l #0xfffff001, d0     // PCI_BIOS_NOT_INSTALLED
	rts

	.lcomm SYM(pcibios_installed)	4

#endif /* PCI-BIOS */
