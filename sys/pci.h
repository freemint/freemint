/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-10-08
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

#ifndef _pci_h_
#define _pci_h_

#define pci_find_device(BusDesc, vendor, id)	\
	
typedef device_t struct device;
typedef driver_t struct driver;
typedef u_int32_t	unsigned long;
typedef u_short		unsigned short;

struct driver
{
	int one;
};

struct device
{
// 	TAILQ_ENTRY(device)	link;
// 	TAILQ_ENTRY(device)	devlink;
	device_t		parent;
	device_list_t		children;

	/*
	 * Detail of this device
	 */
	driver_t		*driver;
	devclass_t		devclass;
	int			unit;
				*unitname;
				*desc;
	int			busy;
// 	device_state_t		state;
	u_int32_t		devflags;
	u_short			flags;
#define DF_ENABLED      1               /* device should be probed/attached */
#define DF_FIXEDCLASS   2               /* devclass specified at create time */
#define DF_WILDCARD     4               /* unit was originally wildcard */
#define DF_DESCMALLOCED 8               /* description was malloced */
#define DF_QUIET        16              /* don't print verbose attach message */
#define DF_DONENOMATCH  32              /* don't execute DEVICE_NOMATCH again */
#define DF_EXTERNALSOFTC 64             /* softc not allocated by us */
#define DF_REBID        128             /* Can rebid after attach */
	u_char  order;                  /* order from device_add_child_ordered() */
	u_char  pad;
	void    *ivars;                 /* instance variables  */
	void    *softc;                 /* current driver's variables  */

// 	struct sysctl_ctx_list sysctl_ctx; /* state for sysctl variables  */
// 	struct sysctl_oid *sysctl_tree; /* state for sysctl variables */
};

struct pci_rsc_desc;		/* structure of resource descriptor    */
{
	unsigned short next;		/* length of the following structure   */
	unsigned short flags;		/* type of resource and misc. flags    */
	unsigned long  start;		/* start-address of resource           */
	unsigned long  length;		/* length of resource                  */
	unsigned long  offset;		/* offset PCI to phys. CPU Address     */
	unsigned long  dmaoffset;	/* offset for DMA-transfers            */
};

struct pci_conv_adr		/* structure of address conversion     */
{
	unsigned long adr;		/* calculated address (CPU<->PCI)      */
	unsigned long len;		/* length of memory range              */
};

struct pcib_f
{
	long		(*find_device)(unsigned long id, unsigned short index);
	long		(*find_classcode)(unsigned long class, unsigned short index);
	long		(*read_cfg_b)(long handle, unsigned char reg, unsigned char *adresse);
	long		(*read_cfg_w)(long handle, unsigned char reg, unsigned short *adresse);
	long		(*read_cfg_l)(long handle, unsigned char reg, unsigned long *adresse);
	unsigned char	(*fast_read_cfg_b)(long handle, unsigned char reg);
	unsigned short	(*fast_read_cfg_w)(long handle, unsigned char reg);
	unsigned long	(*fast_read_cfg_l)(long handle, unsigned char reg);
	long		(*write_cfg_b)(long handle, unsigned char reg, unsigned char val);
	long		(*write_cfg_w)(long handle, unsigned char reg, unsigned short val);
	long		(*write_cfg_l)(long handle, unsigned char reg, unsigned long val);
	long		(*hook_interrupt)(long handle, unsigned long *routine, unsigned long *parameter);
	long		(*unhook_interrupt)(long handle);
	long		(*special_cycle)(unsigned char bus, unsigned long data);
	long		(*get_routing)(long handle);
	long		(*set_interrupt)(long handle);
	long		(*get_resource)(long handle);
	long		(*get_card_used)(long handle, unsigned long *adresse);
	long		(*set_card_used)(long handle, unsigned long *callback);
	long		(*read_mem_b)(long handle, unsigned long offset, unsigned char *adresse);
	long		(*read_mem_w)(long handle, unsigned long offset, unsigned short *adresse);
	long		(*read_mem_l)(long handle, unsigned long offset, unsigned long *adresse);
	unsigned char	(*fast_read_mem_b)(long handle, unsigned long offset);
	unsigned short	(*fast_read_mem_w)(long handle, unsigned long offset);
	unsigned long	(*fast_read_mem_l)(long handle, unsigned long offset);
	long		(*write_mem_b)(long handle, unsigned long offset, unsigned char val);
	long		(*write_mem_w)(long handle, unsigned long offset, unsigned short val);
	long		(*write_mem_l)(long handle, unsigned long offset, unsigned long val);
	long		(*read_io_b)(long handle, unsigned long offset, unsigned char *adresse);
	long		(*read_io_w)(long handle, unsigned long offset, unsigned short *adresse);
	long		(*read_io_l)(long handle, unsigned long offset, unsigned long *adresse);
	unsigned char	(*fast_read_io_b)(long handle, unsigned long offset);
	unsigned short	(*fast_read_io_w)(long handle, unsigned long offset);
	unsigned long	(*fast_read_io_l)(long handle, unsigned long offset);
	long		(*write_io_b)(long handle, unsigned long offset, unsigned char val);
	long		(*write_io_w)(long handle, unsigned long offset, unsigned short val);
	long		(*write_io_l)(long handle, unsigned long offset, unsigned long val);
	long		(*get_machine_id)(void);
	long		(*get_pagesize)(void);
	long		(*virt_to_bus)(long handle, unsigned long adresse, unsigned long *pointer);
	long		(*bus_to_virt)(long handle, unsigned long adresse, unsigned long *pointer);
	long		(*virt_to_phys)(unsigned long adresse, unsigned long *pointer);
	long		(*phys_to_virt)(unsigned long adresse, unsigned long *pointer);
};

struct pci_bios
{
	unsigned long	subcookie;
	unsigned long	version;
	struct pcib_f	f;
};

#define PCI_BUSMAX      255     /* highest supported bus number */
#define PCI_SLOTMAX     31      /* highest supported slot number */
#define PCI_FUNCMAX     7       /* highest supported function number */
#define PCI_REGMAX      255     /* highest supported config register addr. */

#define PCI_MAXMAPS_0   6       /* max. no. of memory/port maps */
#define PCI_MAXMAPS_1   2       /* max. no. of maps for PCI to PCI bridge */
#define PCI_MAXMAPS_2   1       /* max. no. of maps for CardBus bridge */

/* pci_addr_t covers this system's PCI bus address space: 32 or 64 bit */

#ifdef PCI_A64
typedef uint64_t pci_addr_t;    /* uint64_t for system with 64bit addresses */
#else
typedef uint32_t pci_addr_t;    /* uint64_t for system with 64bit addresses */
#endif

/* Interesting values for PCI power management */
struct pcicfg_pp {
    uint16_t    pp_cap;         /* PCI power management capabilities */
    uint8_t     pp_status;      /* config space address of PCI power status reg */
    uint8_t     pp_pmcsr;       /* config space address of PMCSR reg */
    uint8_t     pp_data;        /* config space address of PCI power data reg */
};
 
/* Interesting values for PCI MSI */
struct pcicfg_msi {
    uint16_t    msi_ctrl;       /* Message Control */
    uint8_t     msi_location;   /* Offset of MSI capability registers. */
    uint8_t     msi_msgnum;     /* Number of messages */
    int         msi_alloc;      /* Number of allocated messages. */
    uint64_t    msi_addr;       /* Contents of address register. */
    uint16_t    msi_data;       /* Contents of data register. */
};

/* Interesting values for PCI MSI-X */
struct pcicfg_msix {
    uint16_t    msix_ctrl;      /* Message Control */
    uint8_t     msix_location;  /* Offset of MSI-X capability registers. */
    uint16_t    msix_msgnum;    /* Number of messages */
    int         msix_alloc;     /* Number of allocated messages. */
    uint8_t     msix_table_bar; /* BAR containing vector table. */
    uint8_t     msix_pba_bar;   /* BAR containing PBA. */
    uint32_t    msix_table_offset;
    uint32_t    msix_pba_offset;
    struct resource *msix_table_res;    /* Resource containing vector table. */
    struct resource *msix_pba_res;      /* Resource containing PBA. */
};

/* config header information common to all header types */
typedef struct pcicfg {
	struct device *dev;         /* device which owns this */
	
 	uint32_t    bar[PCI_MAXMAPS_0]; /* BARs */
 	uint32_t    bios;           /* BIOS mapping */
 	
 	uint16_t    subvendor;      /* card vendor ID */
 	uint16_t    subdevice;      /* card device ID, assigned by card vendor */
 	uint16_t    vendor;         /* chip vendor ID */
 	uint16_t    device;         /* chip device ID, assigned by chip vendor */
 	
 	uint16_t    cmdreg;         /* disable/enable chip and PCI options */
 	uint16_t    statreg;        /* supported PCI features and error state */
	
	uint8_t     baseclass;      /* chip PCI class */
	uint8_t     subclass;       /* chip PCI subclass */
	uint8_t     progif;         /* chip PCI programming interface */
	uint8_t     revid;          /* chip revision ID */
	
	uint8_t     hdrtype;        /* chip config header type */
	uint8_t     cachelnsz;      /* cache line size in 4byte units */
	uint8_t     intpin;         /* PCI interrupt pin */
	uint8_t     intline;        /* interrupt line (IRQ for PC arch) */
	
	uint8_t     mingnt;         /* min. useful bus grant time in 250ns units */
	uint8_t     maxlat;         /* max. tolerated bus grant latency in 250ns */
	uint8_t     lattimer;       /* latency timer in units of 30ns bus cycles */
	
	uint8_t     mfdev;          /* multi-function device (from hdrtype reg) */
	uint8_t     nummaps;        /* actual number of PCI maps used */
	
	uint8_t     bus;            /* config space bus address */
	uint8_t     slot;           /* config space slot address */
	uint8_t     func;           /* config space function number */
	
	struct pcicfg_pp pp;        /* pci power management */
	struct pcicfg_msi msi;      /* pci msi */
	struct pcicfg_msix msix;    /* pci msi-x */
} pcicfgregs;

struct bus;
struct bus
{
	struct bus 	*next;
	short		type;
	short		slots;
	short		irq;
	unsigned long	iobase;
	unsigned long	membase;
	unsigned long	cnf;
	unsigned long	bus2mem;
	struct pci_bios *bios;
	unsigned long	bios_handle;
};

#endif /* _pci_h_ */
