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

#include "pci.h"
static struct bus *buses = NULL;

long
init_pci_bios(void)
{
	struct pci_bios *pbios = NULL;
	struct bus *bus;

	if (sys_s_system(S_GETCOOKIE, "_PCI", (long)&pbios)) {
		bus = kmalloc(sizeof(*bus));
		if (bus) {
			bzero(bus, sizeof(*bus));
			bus->type = BUS_PCI;
			bus->bios = pbios;

			bus->next = buses;
			buses = bus;
		}
	}
}


unsigned long _cdecl
pci_get_devid(struct device *dev)
{
	unsigned long id;
	struct bus *b = dev->bus;

	if (b && b->type == BUS_PCI && b->bios) {
		id = (*b->bios->find_device)(dev->devid, dev->);
