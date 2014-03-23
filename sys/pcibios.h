/*
 * Copyright 2014 David Galvez.
 * Based on SCSIDRV sys_emu implementation by Frank Naumann.
 * Some code chunks taken from FireTOS sources:
 * Didier Mequignon 2005-2007, e-mail: aniplay@wanadoo.fr
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

/* Header only for PCI-BIOS system call emulation.
 * IMPORTANT: Drivers must use /mint/pcibios.h
 */

# ifndef _pcibios_h
# define _pcibios_h

# include "mint/mint.h"

/* forward definitions
 */

typedef struct pcibios	PCIBIOS;

/* exported data
 */

extern ulong pcibios_installed;


/* exported functions
 */

long	pcibios_init		(void);

long	_cdecl sys_pcibios	(ushort op,
				     long a1, long a2, long a3, long a4,
				     long a5, long a6, long a7);

/* the struct definitions
 */

typedef struct				/* structure of address conversion */
{
	unsigned long adr;			/* calculated address (CPU<->PCI) */
	unsigned long len;			/* length of memory range */
} PCI_CONV_ADR;

/* structure of resource descriptor */
typedef struct				/* structure of resource descriptor */
{
	unsigned short next;			/* length of the following structure */
	unsigned short flags;			/* type of resource and misc. flags */
	unsigned long start;			/* start-address of resource */
	unsigned long length;			/* length of resource */
	unsigned long offset;			/* offset PCI to phys. CPU Address */
	unsigned long dmaoffset;		/* offset for DMA-transfers */
	long		error;			/* internal error code */
} PCI_RSC_DESC;

/* structure of status descriptor */
typedef struct
{
	unsigned long status;		/* Status PCI */
	unsigned long callback;	/* Address of Callback Routine */
	unsigned long handler;	/* Address of Interrupt Handlers */
	unsigned long parameter;	/* Parameter for Interrupt Handler */
	unsigned long start_IRQ;	/* Routine Start IRQ */
} PCI_STS_DESC;

/* PCI_BIOS structure */

struct pcibios {
	unsigned long subjar;
	unsigned long version;
	/* Although we declare this functions as standard gcc functions (cdecl),
	 * they expect paramenters inside registers (fastcall) unsupported by gcc m68k.
	 * Caller will take care of parameters passing convention.
	 */
	long (*Find_pci_device)	(unsigned long id, unsigned short index);
	long (*Find_pci_classcode)	(unsigned long class, unsigned short index);
	long (*Read_config_byte)	(long handle, unsigned short reg, unsigned char *address);
	long (*Read_config_word)	(long handle, unsigned short reg, unsigned short *address);
	long (*Read_config_longword)	(long handle, unsigned short reg, unsigned long *address);
	unsigned char (*Fast_read_config_byte)	(long handle, unsigned short reg);
	unsigned short (*Fast_read_config_word)	(long handle, unsigned short reg);
	unsigned long (*Fast_read_config_longword)	(long handle, unsigned short reg);
	long (*Write_config_byte)	(long handle, unsigned short reg, unsigned short val);
	long (*Write_config_word)	(long handle, unsigned short reg, unsigned short val);
	long (*Write_config_longword)	(long handle, unsigned short reg, unsigned long val);
	long (*Hook_interrupt)	(long handle, unsigned long *routine, unsigned long *parameter);
	long (*Unhook_interrupt)	(long handle);
	long (*Special_cycle)	(unsigned short bus, unsigned long data);
	long (*Get_routing)	(long handle);
	long (*Set_interrupt)	(long handle);
	long (*Get_resource)	(long handle);
	long (*Get_card_used)	(long handle, unsigned long *address);
	long (*Set_card_used)	(long handle, unsigned long *callback);
	long (*Read_mem_byte)	(long handle, unsigned long offset, unsigned char *address);
	long (*Read_mem_word)	(long handle, unsigned long offset, unsigned short *address);
	long (*Read_mem_longword)	(long handle, unsigned long offset, unsigned long *address);
	unsigned char (*Fast_read_mem_byte)	(long handle, unsigned long offset);
	unsigned short (*Fast_read_mem_word)	(long handle, unsigned long offset);
	unsigned long (*Fast_read_mem_longword)	(long handle, unsigned long offset);
	long (*Write_mem_byte)	(long handle, unsigned long offset, unsigned short val);
	long (*Write_mem_word)	(long handle, unsigned long offset, unsigned short val);
	long (*Write_mem_longword)	(long handle, unsigned long offset, unsigned long val);
	long (*Read_io_byte)	(long handle, unsigned long offset, unsigned char *address);
	long (*Read_io_word)	(long handle, unsigned long offset, unsigned short *address);
	long (*Read_io_longword)	(long handle, unsigned long offset, unsigned long *address);
	unsigned char (*Fast_read_io_byte)	(long handle, unsigned long offset);
	unsigned short (*Fast_read_io_word)	(long handle, unsigned long offset);
	unsigned long (*Fast_read_io_longword)	(long handle, unsigned long offset);
	long (*Write_io_byte)	(long handle, unsigned long offset, unsigned short val);
	long (*Write_io_word)	(long handle, unsigned long offset, unsigned short val);
	long (*Write_io_longword)	(long handle, unsigned long offset, unsigned long val);
	long (*Get_machine_id)	(void);
	long (*Get_pagesize)	(void);
	long (*Virt_to_bus)	(long handle, unsigned long address, PCI_CONV_ADR *pointer);
	long (*Bus_to_virt)	(long handle, unsigned long address, PCI_CONV_ADR *pointer);
	long (*Virt_to_phys)	(unsigned long address, PCI_CONV_ADR *pointer);
	long (*Phys_to_virt)	(unsigned long address, PCI_CONV_ADR *pointer);
	long reserved[2];
};

typedef struct pcibios_mint {
	PCIBIOS emu_pcibios;
	/* Only for FireTOS and CTPCI */
#define PCI_MAX_HANDLE		5 	/* 4 slots on the CTPCI + host bridge PLX9054 */
#define PCI_MAX_FUNCTION	4	/* 4 functions per PCI slot */
#define PCI_MAX_RSC_DESC	6	/* 6 resource descriptors */
	PCI_RSC_DESC pci_rsc_desc[PCI_MAX_HANDLE][PCI_MAX_FUNCTION][PCI_MAX_RSC_DESC];
	PCI_STS_DESC pci_sts_desc[PCI_MAX_HANDLE][PCI_MAX_FUNCTION];
} PCIBIOS_MINT;


/* PCI bios calls from _PCI cookie
 */

long Find_pci_device(unsigned long id, unsigned short index);
long Find_pci_classcode(unsigned long class, unsigned short index);
long Read_config_byte(long handle, unsigned short reg, unsigned char *address);
long Read_config_word(long handle, unsigned short reg, unsigned short *address);
long Read_config_longword(long handle, unsigned short reg, unsigned long *address);
unsigned char Fast_read_config_byte(long handle, unsigned short reg);
unsigned short Fast_read_config_word(long handle, unsigned short reg);
unsigned long Fast_read_config_longword(long handle, unsigned short reg);
long Write_config_byte(long handle, unsigned short reg, unsigned short val);
long Write_config_word(long handle, unsigned short reg, unsigned short val);
long Write_config_longword(long handle, unsigned short reg, unsigned long val);
long Hook_interrupt(long handle, unsigned long *routine, unsigned long *parameter);
long Unhook_interrupt(long handle);
long Special_cycle(unsigned short bus, unsigned long data);
long Get_routing(long handle);
long Set_interrupt(long handle);
long Get_resource(long handle);
long Get_card_used(long handle, unsigned long *address);
long Set_card_used(long handle, unsigned long *callback);
long Read_mem_byte(long handle, unsigned long offset, unsigned char *address);
long Read_mem_word(long handle, unsigned long offset, unsigned short *address);
long Read_mem_longword(long handle, unsigned long offset, unsigned long *address);
unsigned char Fast_read_mem_byte(long handle, unsigned long offset);
unsigned short Fast_read_mem_word(long handle, unsigned long offset);
unsigned long Fast_read_mem_longword(long handle, unsigned long offset);
long Write_mem_byte(long handle, unsigned long offset, unsigned short val);
long Write_mem_word(long handle, unsigned long offset, unsigned short val);
long Write_mem_longword(long handle, unsigned long offset, unsigned long val);
long Read_io_byte(long handle, unsigned long offset, unsigned char *address);
long Read_io_word(long handle, unsigned long offset, unsigned short *address);
long Read_io_longword(long handle, unsigned long offset, unsigned long *address);
unsigned char Fast_read_io_byte(long handle, unsigned long offset);
unsigned short Fast_read_io_word(long handle, unsigned long offset);
unsigned long Fast_read_io_longword(long handle, unsigned long offset);
long Write_io_byte(long handle, unsigned long offset, unsigned short val);
long Write_io_word(long handle, unsigned long offset, unsigned short val);
long Write_io_longword(long handle, unsigned long offset, unsigned long val);
long Get_machine_id(void);
long Get_pagesize(void);
long Virt_to_bus(long handle, unsigned long address, PCI_CONV_ADR *pointer);
long Bus_to_virt(long handle, unsigned long address, PCI_CONV_ADR *pointer);
long Virt_to_phys(unsigned long address, PCI_CONV_ADR *pointer);
long Phys_to_virt(unsigned long address, PCI_CONV_ADR *pointer);

# endif /* _pcibios_h */
