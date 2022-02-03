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

# ifndef _m68k_pcibios_emu_h
# define _m68k_pcibios_emu_h

# include "mint/mint.h"
# include "pcibios.h"

/* Although we declare this functions as standard gcc functions (cdecl),
 * they expect paramenters inside registers (fastcall), which is unsupported by gcc m68k.
 * Don't worry caller will take care of parameters passing convention.
 */
long emu_pcibios_Find_pci_device	(unsigned long id, unsigned short index);
long emu_pcibios_Find_pci_classcode	(unsigned long class, unsigned short index);
long emu_pcibios_Read_config_byte	(long handle, unsigned short reg, unsigned char *address);
long emu_pcibios_Read_config_word	(long handle, unsigned short reg, unsigned short *address);
long emu_pcibios_Read_config_longword	(long handle, unsigned short reg, unsigned long *address);
unsigned char emu_pcibios_Fast_read_config_byte	(long handle, unsigned short reg);
unsigned short emu_pcibios_Fast_read_config_word	(long handle, unsigned short reg);
unsigned long emu_pcibios_Fast_read_config_longword	(long handle, unsigned short reg);
long emu_pcibios_Write_config_byte	(long handle, unsigned short reg, unsigned short val);
long emu_pcibios_Write_config_word	(long handle, unsigned short reg, unsigned short val);
long emu_pcibios_Write_config_longword	(long handle, unsigned short reg, unsigned long val);
long emu_pcibios_Hook_interrupt	(long handle, unsigned long *routine, unsigned long *parameter);
long emu_pcibios_Unhook_interrupt	(long handle);
long emu_pcibios_Special_cycle	(unsigned short bus, unsigned long data);
long emu_pcibios_Get_routing	(long handle);
long emu_pcibios_Set_interrupt	(long handle);
long emu_pcibios_Get_resource	(long handle);
long emu_pcibios_Get_card_used	(long handle, unsigned long *address);
long emu_pcibios_Set_card_used	(long handle, unsigned long *callback);
long emu_pcibios_Read_mem_byte	(long handle, unsigned long offset, unsigned char *address);
long emu_pcibios_Read_mem_word	(long handle, unsigned long offset, unsigned short *address);
long emu_pcibios_Read_mem_longword	(long handle, unsigned long offset, unsigned long *address);
unsigned char emu_pcibios_Fast_read_mem_byte	(long handle, unsigned long offset);
unsigned short emu_pcibios_Fast_read_mem_word	(long handle, unsigned long offset);
unsigned long emu_pcibios_Fast_read_mem_longword	(long handle, unsigned long offset);
long emu_pcibios_Write_mem_byte	(long handle, unsigned long offset, unsigned short val);
long emu_pcibios_Write_mem_word	(long handle, unsigned long offset, unsigned short val);
long emu_pcibios_Write_mem_longword	(long handle, unsigned long offset, unsigned long val);
long emu_pcibios_Read_io_byte	(long handle, unsigned long offset, unsigned char *address);
long emu_pcibios_Read_io_word	(long handle, unsigned long offset, unsigned short *address);
long emu_pcibios_Read_io_longword	(long handle, unsigned long offset, unsigned long *address);
unsigned char emu_pcibios_Fast_read_io_byte	(long handle, unsigned long offset);
unsigned short emu_pcibios_Fast_read_io_word	(long handle, unsigned long offset);
unsigned long emu_pcibios_Fast_read_io_longword	(long handle, unsigned long offset);
long emu_pcibios_Write_io_byte	(long handle, unsigned long offset, unsigned short val);
long emu_pcibios_Write_io_word	(long handle, unsigned long offset, unsigned short val);
long emu_pcibios_Write_io_longword	(long handle, unsigned long offset, unsigned long val);
long emu_pcibios_Get_machine_id	(void);
long emu_pcibios_Get_pagesize	(void);
long emu_pcibios_Virt_to_bus	(long handle, unsigned long address, PCI_CONV_ADR *pointer);
long emu_pcibios_Bus_to_virt	(long handle, unsigned long address, PCI_CONV_ADR *pointer);
long emu_pcibios_Virt_to_phys	(unsigned long address, PCI_CONV_ADR *pointer);
long emu_pcibios_Phys_to_virt	(unsigned long address, PCI_CONV_ADR *pointer);

# endif /* _m68k_pcibios_emu_h */
