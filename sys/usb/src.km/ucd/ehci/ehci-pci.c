/*
 * Adapted for FreeMiNT by David Galvez 2014
 * Ported for Atari by Didier Mequignon
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "mint/pcibios.h"
#include "mint/pci_ids.h"
#include "libkern/libkern.h"
#include "../../global.h"

#include "../../endian/io.h"
#include "mint/endian.h"
#include "mint/mdelay.h"
#include "../../usb.h"
#include "mint/time.h"
#include "arch/timer.h"
#include "../../usb_api.h"

#include "ehci.h"

/*
 * Debug section
 */

#if 0
# define DEV_DEBUG	1
#endif

#ifdef DEV_DEBUG

# define FORCE(x)	KERNEL_FORCE x
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x

#else

# define FORCE(x)	KERNEL_FORCE x
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)
# define TRACE(x)
# define ASSERT(x)	assert x

#endif

extern struct usb_module_api   *api;

extern void ehci_int_handle_asm(void);

/*
 * Function prototypes
 */
long ehci_pci_init	(void *);
void ehci_pci_stop	(struct ehci *);
long ehci_pci_probe	(struct ucdif *);
long ehci_pci_reset	(struct ehci *);
void ehci_pci_error	(struct ehci *);


struct ehci_pci {
	long handle;				/* PCI BIOS */
	const struct pci_device_id *ent;
};

struct pci_device_id ehci_usb_pci_table[] = {
	{ PCI_VENDOR_ID_NEC, PCI_DEVICE_ID_NEC_USB_2, 
	  PCI_ANY_ID, PCI_ANY_ID, PCI_CLASS_SERIAL_USB_EHCI, 0, 0 }, /* NEC PCI EHCI module ids */
	{ PCI_VENDOR_ID_PHILIPS, PCI_DEVICE_ID_PHILIPS_ISP1561_2, 
	  PCI_ANY_ID, PCI_ANY_ID, PCI_CLASS_SERIAL_USB_EHCI, 0, 0 }, /* Philips 1561 PCI EHCI module ids */
	/* Please add supported PCI EHCI controller ids here */
	{ 0, 0, 0, 0, 0, 0, 0 }
};

struct ehci_bus ehci_bus = {
	.init = ehci_pci_init,
	.stop = ehci_pci_stop,
	.probe = ehci_pci_probe,
	.reset = ehci_pci_reset,
	.error = ehci_pci_error,
};

void
ehci_pci_error(struct ehci *gehci)
{
	unsigned short status = Fast_read_config_word(((struct ehci_pci *)gehci->bus)->handle, PCISR);
	ALERT(("EHCI Host System Error, controller usb-%s disabled\r\n(SR:0x%04x%s%s%s%s%s%s)", gehci->slot_name, status & 0xFFFF,
	 status & 0x8000 ? ", Parity error" : "", status & 0x4000 ? ", Signaled system error" : "", status & 0x2000 ? ", Received master abort" : "",
	 status & 0x1000 ? ", Received target abort" : "", status & 0x800 ? ", Signaled target abort" : "", status & 0x100 ? ", Data parity error" : ""));
}

long
ehci_pci_reset(struct ehci *gehci)
{
	if (machine == machine_firebee) 
	{
		if((((struct ehci_pci *)gehci->bus)->ent->vendor == PCI_VENDOR_ID_NEC)
		 && (((struct ehci_pci *)gehci->bus)->ent->device == PCI_DEVICE_ID_NEC_USB_2))
		{
			DEBUG(("ehci_reset set 48MHz clock"));
			Write_config_longword(((struct ehci_pci *)gehci->bus)->handle, 0xE4, 0x20); // oscillator
		}
	}
	return 0;
}

long
ehci_pci_init(void *ucd_priv)
{
	unsigned long usb_base_addr = 0xFFFFFFFF;

	PCI_RSC_DESC *pci_rsc_desc;

	struct ehci *gehci = (struct ehci*)ucd_priv;
	if(!((struct ehci_pci *)gehci->bus)->handle) /* for restart USB cmd */
		return(-1);

	pci_rsc_desc = (PCI_RSC_DESC *)Get_resource(((struct ehci_pci *)gehci->bus)->handle); /* USB OHCI */
	if((long)pci_rsc_desc >= 0)
	{
		unsigned short flags;
		do
		{
			DEBUG(("PCI USB descriptors: flags 0x%08x start 0x%08x \r\n offset 0x%08x dmaoffset 0x%08x length 0x%08x",
			 pci_rsc_desc->flags, pci_rsc_desc->start, pci_rsc_desc->offset, pci_rsc_desc->dmaoffset, pci_rsc_desc->length));
			if(!(pci_rsc_desc->flags & FLG_IO))
			{
				if(usb_base_addr == 0xFFFFFFFF)
				{
					usb_base_addr = pci_rsc_desc->start;
					gehci->hccr = (struct ehci_hccr *)(pci_rsc_desc->offset + pci_rsc_desc->start);
					gehci->dma_offset = pci_rsc_desc->dmaoffset;
					if((pci_rsc_desc->flags & FLG_ENDMASK) == ORD_MOTOROLA)
						gehci->big_endian = 0; /* hardware makes swapping intel -> motorola */
					else
						gehci->big_endian = 1; /* driver must swap intel -> motorola */
				}
			}
			flags = pci_rsc_desc->flags;
			pci_rsc_desc = (PCI_RSC_DESC *)((unsigned long)pci_rsc_desc->next + (unsigned long)pci_rsc_desc);
		}
		while(!(flags & FLG_LAST));
	}
	else
	{
		return(-1); /* get_resource error */
	}
	if(usb_base_addr == 0xFFFFFFFF)
	{
		return(-1);
	}
	if(((struct ehci_pci *)gehci->bus)->handle && ((((struct ehci_pci *)gehci->bus)->ent) != NULL))
	{
		switch(((struct ehci_pci *)gehci->bus)->ent->vendor)
		{
			case PCI_VENDOR_ID_NEC: gehci->slot_name = "uPD720101"; break;
			case PCI_VENDOR_ID_PHILIPS: gehci->slot_name = "isp1561"; break;
			default: gehci->slot_name = "generic"; break;
		}
	}

	/* hook interrupt handler */
	Hook_interrupt(((struct ehci_pci *)gehci->bus)->handle, (void *)ehci_int_handle_asm, (unsigned long *)gehci);

	return 0;
}

void ehci_pci_stop(struct ehci *gehci)
{
	Unhook_interrupt(((struct ehci_pci *)gehci->bus)->handle);
}

/* temporary, need multiple versions and alloc, but the new ucd_register
 * doesn't support more than one at this time.
 */
static struct usb_device *root_hub_dev = NULL;

long
ehci_pci_probe(struct ucdif *ehci_uif)
{
	short index;
	long err;
	int loop_counter = 0;

	long handle;
	struct pci_device_id *board;

	if(pcibios_installed == 0)
	{
		ALERT(("PCI-BIOS not found. You need a PCI-BIOS to use this driver"));
		return -1;
	}

	/* PCI devices detection */
	index = 0;
	do
	{
		do
		{
			handle = Find_pci_device(0x0000FFFFL, index++);
			if(handle >= 0)
			{
				unsigned long id = 0;
				err = Read_config_longword(handle, PCIIDR, &id);
				/* test USB devices */
				if((err >= 0))
				{
					unsigned long class;
					if(Read_config_longword(handle, PCIREV, &class) >= 0
					   && ((class >> 16) == PCI_CLASS_SERIAL_USB))
					{
						if((class >> 8) == PCI_CLASS_SERIAL_USB_EHCI)
						{
							board = ehci_usb_pci_table; /* compare table */
							while(board->vendor)
							{
								if((board->vendor == (id & 0xFFFF))
								    && (board->device == (id >> 16)))
								{
									err = ehci_alloc_ucdif(&ehci_uif);
									if (err < 0)
										break;
									/* assign an interface */
									err = ucd_register(ehci_uif, &root_hub_dev);
									if (err) 
									{
										DEBUG (("%s: ucd register failed!", __FILE__));
										break;
									}
									struct ehci *gehci = (struct ehci *)ehci_uif->ucd_priv;
									gehci->bus = kmalloc (sizeof(struct ehci_pci));
									((struct ehci_pci *)gehci->bus)->handle = handle;
									((struct ehci_pci *)gehci->bus)->ent = board;
									DEBUG (("%s: ucd register ok", __FILE__));
									break;
								}
								board++;
							}
						}
					}
				}
			}
		}
		while(handle >= 0);
		loop_counter++;
	}
	while(loop_counter <= 2); /* Number of card slots */

	return 0;
}

long ehci_interrupt_handle(long param)
{
	struct ehci *ehci = (struct ehci *)param;
	unsigned long status;

	/* flush caches */
	cpush(ehci, -1);

	status = ehci_readl(&ehci->hcor->or_usbsts);
	if(status & STS_PCD) /* port change detect */
	{
		unsigned long reg = ehci_readl(&ehci->hccr->cr_hcsparams);
		unsigned long i = HCS_N_PORTS(reg);
		while(i--)
		{
			unsigned long pstatus = ehci_readl(&ehci->hcor->or_portsc[i-1]);
			if(pstatus & EHCI_PS_PO)
				continue;
			if(ehci->companion & (1 << i))
			{
				/* Low speed device, give up ownership. */
				pstatus |= EHCI_PS_PO;
				ehci_writel(&ehci->hcor->or_portsc[i-1], pstatus);
			}
			else if((pstatus & EHCI_PS_CSC))
				usb_rh_wakeup();
		}
	}

	/* Disable interrupt */
	ehci_writel(&ehci->hcor->or_usbsts, status);

	/* PCI_BIOS specification: if interrupt was for us set D0.0 */
	return 1;
}
