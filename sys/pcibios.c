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
# include "pcibios.h"
# include "global.h"

# include "arch/pcibios_emu.h"
# include "mint/proc.h"
# include "libkern/libkern.h"

# include "cookie.h"	/* cookie handling */
# include "init.h"	/* boot_printf */
# include "k_prot.h"

# include "proc.h"

static PCIBIOS emu_pcibios = {
	0,
	0,
	emu_pcibios_Find_pci_device,
	emu_pcibios_Find_pci_classcode,
	emu_pcibios_Read_config_byte,
	emu_pcibios_Read_config_word,
	emu_pcibios_Read_config_longword,
	emu_pcibios_Fast_read_config_byte,
	emu_pcibios_Fast_read_config_word,
	emu_pcibios_Fast_read_config_longword,
	emu_pcibios_Write_config_byte,
	emu_pcibios_Write_config_word,
	emu_pcibios_Write_config_longword,
	emu_pcibios_Hook_interrupt,
	emu_pcibios_Unhook_interrupt,
	emu_pcibios_Special_cycle,
	emu_pcibios_Get_routing,
	emu_pcibios_Set_interrupt,
	emu_pcibios_Get_resource,
	emu_pcibios_Get_card_used,
	emu_pcibios_Set_card_used,
	emu_pcibios_Read_mem_byte,
	emu_pcibios_Read_mem_word,
	emu_pcibios_Read_mem_longword,
	emu_pcibios_Fast_read_mem_byte,
	emu_pcibios_Fast_read_mem_word,
	emu_pcibios_Fast_read_mem_longword,
	emu_pcibios_Write_mem_byte,
	emu_pcibios_Write_mem_word,
	emu_pcibios_Write_mem_longword,
	emu_pcibios_Read_io_byte,
	emu_pcibios_Read_io_word,
	emu_pcibios_Read_io_longword,
	emu_pcibios_Fast_read_io_byte,
	emu_pcibios_Fast_read_io_word,
	emu_pcibios_Fast_read_io_longword,
	emu_pcibios_Write_io_byte,
	emu_pcibios_Write_io_word,
	emu_pcibios_Write_io_longword,
	emu_pcibios_Get_machine_id,
	emu_pcibios_Get_pagesize,
	emu_pcibios_Virt_to_bus,
	emu_pcibios_Bus_to_virt,
	emu_pcibios_Virt_to_phys,
	emu_pcibios_Phys_to_virt,
};


# ifdef DEBUG_INFO
# define PCIBIOS_DEBUG(x)	DEBUG (x)
# else
# define SCSIDRV_DEBUG(x)
# endif


ulong pcibios_installed = 0;

PCIBIOS *pcibios = NULL;
PCIBIOS pcibios_ct60tos;
void *tab_funcs_pci;

long
pcibios_init (void)
{
	long r;
	unsigned long t = 0, dummy = 0;

	r = get_toscookie(COOKIE__PCI, &t);
	pcibios = (PCIBIOS *)t;

	if (!r && pcibios)
	{
		pcibios_installed = pcibios->version;
		/* First function in jump table */
		tab_funcs_pci = &pcibios->Find_pci_device;
		emu_pcibios.version = pcibios_installed;
		r = set_toscookie (COOKIE__PCI, (long) &emu_pcibios);
	}
	else
	{
		pcibios_installed = 0;
		pcibios = NULL;
		return r;
	}

	/* Some PCI-BIOS functions in FireTOS and CT60TOS PCI-BIOS get the
	 * cookie value every time they're called to calculate the place
	 * in memory where descriptors are stored, as the cookie value
	 * is replaced by MiNT the memory addresses calculated are wrong.
	 * To avoid crashes we need to make this hack. Resource and status
	 * descriptors are store following the PCI-BIOS functions jump table.
	 */
#define JUMPTABLESIZE	sizeof(PCIBIOS) - 8

	r = get_toscookie(COOKIE__CPU, &t);
	if (!r && t == 60)	/* cf68klib in FireTOS emulates 68060 CPU! */
		if (!get_toscookie(COOKIE__CF_, &dummy) ||	/* Test for FireTOS */
		    !get_toscookie(COOKIE_CT60, &dummy))	/* Test for CT60 (CTPCI) */
		{
			/* Save the old jumptable to a new memory loaction */
			memcpy(&pcibios_ct60tos.Find_pci_device, &pcibios->Find_pci_device, JUMPTABLESIZE);

			/* Make the wrapper point to the new location */
			tab_funcs_pci = &pcibios_ct60tos.Find_pci_device;

			/* Copy the wrapper's jumptable to the memory pointed by the old _PCI cookie's value */
			memcpy(&pcibios->Find_pci_device, &emu_pcibios.Find_pci_device, JUMPTABLESIZE);

			/* Restore the old cookie value */
			r = set_toscookie(COOKIE__PCI, (long)pcibios);
		}

	return r;
}

long _cdecl
sys_pcibios (ushort op,
	     long a1, long a2, long a3, long a4,
	     long a5, long a6, long a7)
{
	typedef long (*wrap0)();
	typedef long (*wrap1)(long);
	typedef long (*wrap2)(long, long);
	typedef long (*wrap3)(long, long, long);

	/* only superuser can use this interface */
	if (!suser (get_curproc()->p_cred->ucr))
		return EPERM;

	if (!pcibios)
		return ENOSYS;

	switch (op)
	{
		/* PCI_BIOS exist */
		case 0:
		case 1:
		{
			return E_OK;
		}
		/* Find_pci_device */
		case 2:
		{
			wrap2 f = (wrap2)Find_pci_device;
			return (*f)(a1, a2);
		}
		/* Find_pci_classcode */
		case 3:
		{
			wrap2 f = (wrap2)Find_pci_classcode;
			return (*f)(a1, a2);
		}
		/* Read_config_byte */
		case 4:
		{
			wrap3 f = (wrap3)Read_config_byte;
			return (*f)(a1, a2, a3);
		}
		/* Read_config_word */
		case 5:
		{
			wrap3 f = (wrap3)Read_config_word;
			return (*f)(a1, a2, a3);
		}
		/* Read_config_longword */
		case 6:
		{
			wrap3 f = (wrap3)Read_config_longword;
			return (*f)(a1, a2, a3);
		}
		/* Fast_read_config_byte */
		case 7:
		{
			wrap2 f = (wrap2)Fast_read_config_byte;
			return (*f)(a1, a2);
		}
		/* Fast_read_config_word */
		case 8:
		{
			wrap2 f = (wrap2)Fast_read_config_word;
			return (*f)(a1, a2);
		}
		/* Fast_read_config_longword */
		case 9:
		{
			wrap2 f = (wrap2)Fast_read_config_longword;
			return (*f)(a1, a2);
		}
		/* Write_config_byte */
		case 10:
		{
			wrap3 f = (wrap3)Write_config_byte;
			return (*f)(a1, a2, a3);
		}
		/* Write_config_word */
		case 11:
		{
			wrap3 f = (wrap3)Write_config_word;
			return (*f)(a1, a2, a3);
		}
		/* Write_config_longword */
		case 12:
		{
			wrap3 f = (wrap3)Write_config_longword;
			return (*f)(a1, a2, a3);
		}
		/* Hook_interrupt */
		case 13:
		{
			wrap3 f = (wrap3)Hook_interrupt;
			return (*f)(a1, a2, a3);
		}
		/* Unhook_interrupt */
		case 14:
		{
			wrap1 f = (wrap1)Unhook_interrupt;
			return (*f)(a1);
		}
		/* Special_cycle */
		case 15:
		{
			wrap2 f = (wrap2)Special_cycle;
			return (*f)(a1, a2);
		}
		/* Get_routing */
		/* Set_interrupt */
		case 16:
		case 17:
		{
			return ENOSYS;
		}
		/* Get_resource */
		case 18:
		{
			wrap1 f = (wrap1)Get_resource;
			return (*f)(a1);
		}
		/* Get_card_used */
		case 19:
		{
			wrap2 f = (wrap2)Get_card_used;
			return (*f)(a1, a2);
		}
		/* Set_card_used */
		case 20:
		{
			wrap2 f = (wrap2)Set_card_used;
			return (*f)(a1, a2);
		}
		/* Read_mem_byte */
		case 21:
		{
			wrap3 f = (wrap3)Read_mem_byte;
			return (*f)(a1, a2, a3);
		}
		/* Read_mem_word */
		case 22:
		{
			wrap3 f = (wrap3)Read_mem_word;
			return (*f)(a1, a2, a3);
		}
		/* Read_mem_longword */
		case 23:
		{
			wrap3 f = (wrap3)Read_mem_longword;
			return (*f)(a1, a2, a3);
		}
		/* Fast_read_mem_byte */
		case 24:
		{
			wrap2 f = (wrap2)Fast_read_mem_byte;
			return (*f)(a1, a2);
		}
		/* Fast_read_mem_word */
		case 25:
		{
			wrap2 f = (wrap2)Fast_read_mem_word;
			return (*f)(a1, a2);
		}
		/* Fast_read_mem_longword */
		case 26:
		{
			wrap2 f = (wrap2)Fast_read_mem_longword;
			return (*f)(a1, a2);
		}
		/* Write_mem_byte */
		case 27:
		{
			wrap3 f = (wrap3)Write_mem_byte;
			return (*f)(a1, a2, a3);
		}
		/* Write_mem_word */
		case 28:
		{
			wrap3 f = (wrap3)Write_mem_word;
			return (*f)(a1, a2, a3);
		}
		/* Write_mem_longword */
		case 29:
		{
			wrap3 f = (wrap3)Write_mem_longword;
			return (*f)(a1, a2, a3);
		}
		/* Read_io_byte */
		case 30:
		{
			wrap3 f = (wrap3)Read_io_byte;
			return (*f)(a1, a2, a3);
		}
		/* Read_io_word */
		case 31:
		{
			wrap3 f = (wrap3)Read_io_word;
			return (*f)(a1, a2, a3);
		}
		/* Read_io_longword */
		case 32:
		{
			wrap3 f = (wrap3)Read_io_longword;
			return (*f)(a1, a2, a3);
		}
		/* Fast_read_io_byte */
		case 33:
		{
			wrap2 f = (wrap2)Fast_read_io_byte;
			return (*f)(a1, a2);
		}
		/* Fast_read_io_word */
		case 34:
		{
			wrap2 f = (wrap2)Fast_read_io_word;
			return (*f)(a1, a2);
		}
		/* Fast_read_io_longword */
		case 35:
		{
			wrap2 f = (wrap2)Fast_read_io_longword;
			return (*f)(a1, a2);
		}
		/* Write_io_byte */
		case 36:
		{
			wrap3 f = (wrap3)Write_io_byte;
			return (*f)(a1, a2, a3);
		}
		/* Write_io_word */
		case 37:
		{
			wrap3 f = (wrap3)Write_io_word;
			return (*f)(a1, a2, a3);;
		}
		/* Write_io_longword */
		case 38:
		{
			wrap3 f = (wrap3)Write_io_longword;
			return (*f)(a1, a2, a3);
		}
		/* Get_machine_id */
		case 39:
		{
			wrap1 f = (wrap1)Get_machine_id;
			return (*f)(a1);
		}
		/* Get_pagesize */
		case 40:
		{
			wrap0 f = (wrap0)Get_pagesize;
			return (*f)();
		}
		/* Virt_to_bus */
		case 41:
		{
			wrap3 f = (wrap3)Virt_to_bus;
			return (*f)(a1, a2, a3);
		}
		/* Bus_to_virt */
		case 42:
		{
			wrap3 f = (wrap3)Bus_to_virt;
			return (*f)(a1, a2, a3);
		}
		/* Virt_to_phys */
		case 43:
		{
			wrap2 f = (wrap2)Virt_to_phys;
			return (*f)(a1, a2);
		}
		/* Phys_to_virt */
		case 44:
		{
			wrap2 f = (wrap2)Phys_to_virt;
			return (*f)(a1, a2);
		}
	}
	return EBADARG;
}
#endif /* PCI-BIOS */
