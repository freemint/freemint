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


# ifndef _mint_pcibios_h
# define _mint_pcibios_h


/* PCI config registers offsets
 */

#define PCIIDR			0x00   /* PCI Configuration ID Register       */
#define PCICSR			0x04   /* PCI Command/Status Register         */
#define PCICR			0x04   /* PCI Command Register                */
#define PCISR			0x06   /* PCI Status Register                 */
#define PCIREV			0x08   /* PCI Revision ID Register            */
#define PCICCR			0x09   /* PCI Class Code Register             */
#define PCICLSR			0x0C   /* PCI Cache Line Size Register        */
#define PCILTR			0x0D   /* PCI Latency Timer Register          */
#define PCIHTR			0x0E   /* PCI Header Type Register            */
#define PCIBISTR		0x0F   /* PCI Build-In Self Test Register     */
#define PCIBAR0			0x10   /* PCI Base Address Register for Memory
					Accesses to Local, Runtime, and DMA */
#define PCIBAR1			0x14   /* PCI Base Address Register for I/O
					Accesses to Local, Runtime, and DMA */
#define PCIBAR2			0x18   /* PCI Base Address Register for Memory
					Accesses to Local Address Space 0   */
#define PCIBAR3			0x1C   /* PCI Base Address Register for Memory
					Accesses to Local Address Space 1   */ 
#define PCIBAR4			0x20   /* PCI Base Address Register, reserved */
#define PCIBAR5			0x24   /* PCI Base Address Register, reserved */
#define PCICIS			0x28   /* PCI Cardbus CIS Pointer, not support*/
#define PCISVID			0x2C   /* PCI Subsystem Vendor ID             */
#define PCISID			0x2E   /* PCI Subsystem ID                    */
#define PCIERBAR		0x30   /* PCI Expansion ROM Base Register     */
#define CAP_PTR			0x34   /* New Capability Pointer              */
#define PCIILR			0x3C   /* PCI Interrupt Line Register         */
#define PCIIPR			0x3D   /* PCI Interrupt Pin Register          */
#define PCIMGR			0x3E   /* PCI Min_Gnt Register                */
#define PCIMLR			0x3F   /* PCI Max_Lat Register                */
#define PMCAPID			0x40   /* Power Management Capability ID      */
#define PMNEXT			0x41   /* Power Management Next Capability
					Pointer                             */
#define PMC			0x42   /* Power Management Capabilities       */
#define PMCSR			0x44   /* Power Management Control/Status     */
#define PMCSR_BSE		0x46   /* PMCSR Bridge Support Extensions     */
#define PMDATA			0x47   /* Power Management Data               */
#define HS_CNTL			0x48   /* Hot Swap Control                    */
#define HS_NEXT			0x49   /* Hot Swap Next Capability Pointer    */
#define HS_CSR			0x4A   /* Hot Swap Control/Status             */
#define PVPDCNTL		0x4C   /* PCI Vital Product Data Control      */
#define PVPD_NEXT		0x4D   /* PCI Vital Product Data Next
					Capability Pointer                  */
#define PVPDAD			0x4E   /* PCI Vital Product Data Address      */
#define PVPDATA			0x50   /* PCI VPD Data                        */


/* the struct definitions
 */

typedef struct				/* structure of resource descriptor    */
{
	unsigned short next;			/* length of the following structure   */
	unsigned short flags;			/* type of resource and misc. flags    */
	unsigned long start;			/* start-address of resource           */
	unsigned long length;			/* length of resource                  */
	unsigned long offset;			/* offset PCI to phys. CPU Address     */
	unsigned long dmaoffset;		/* offset for DMA-transfers            */
} PCI_RSC_DESC;

typedef struct				/* structure of address conversion     */
{
	unsigned long adr;			/* calculated address (CPU<->PCI)      */
	unsigned long len;			/* length of memory range              */
} PCI_CONV_ADR;


/* PCI-BIOS Error Codes
 */

#define PCI_SUCCESSFUL			0  /* everything's fine         */
#define PCI_FUNC_NOT_SUPPORTED		-2  /* function not supported    */
#define PCI_BAD_VENDOR_ID		-3  /* wrong Vendor ID           */
#define PCI_DEVICE_NOT_FOUND		-4  /* PCI-Device not found      */
#define PCI_BAD_REGISTER_NUMBER		-5  /* wrong register number     */
#define PCI_SET_FAILED			-6  /* reserved for later use    */
#define PCI_BUFFER_TOO_SMALL		-7  /* reserved for later use    */
#define PCI_GENERAL_ERROR		-8  /* general BIOS error code   */
#define PCI_BAD_HANDLE			-9  /* wrong/unknown PCI-handle  */


/* Flags used in Resource-Descriptor
 */

#define FLG_IO		0x4000         /* Ressource in IO range               */
#define FLG_ROM		0x2000         /* Expansion ROM */
#define FLG_LAST	0x8000         /* last ressource                      */
#define FLG_8BIT	0x0100         /* 8 bit accesses allowed              */
#define FLG_16BIT	0x0200         /* 16 bit accesses allowed             */
#define FLG_32BIT	0x0400         /* 32 bit accesses allowed             */
#define FLG_ENDMASK	0x000F         /* mask for byte ordering              */


/* Values used in FLG_ENDMASK for Byte Ordering
 */

#define ORD_MOTOROLA	0         /* Motorola (big endian)               */
#define ORD_INTEL_AS	1         /* Intel (little endian), addr.swapped */
#define ORD_INTEL_LS	2         /* Intel (little endian), lane swapped */
#define ORD_UNKNOWN	15         /* unknown (BIOS-calls allowed only)   */


/* Status Info used in Device-Descriptor
 */

#define DEVICE_FREE		0         /* Device is not used                  */
#define DEVICE_USED		1         /* Device is used by another driver    */
#define DEVICE_CALLBACK		2         /* used, but driver can be cancelled   */
#define DEVICE_AVAILABLE	3         /* used, not available                 */
#define NO_DEVICE		-1         /* no device detected                  */


/* Callback-Routine
 */

 #define GET_DRIVER_ID		0        /* CB-Routine 0: Get Driver ID         */
 #define REMOVE_DRIVER		1        /* CB-Routine 1: Remove Driver         */

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

# endif /* _mint_pcibios_h */
