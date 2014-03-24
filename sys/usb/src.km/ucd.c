/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <mint/osbind.h> /* Setexc */

#include "global.h"
#include "usb.h"
#include "ucd.h"
#include "hub.h"

struct ucdif *allucdifs = NULL;

/*
 * Called by ucd upon init to register itself
 */
long
ucd_register(struct ucdif *a)
{
	struct usb_device *dev;
	long result;

	DEBUG(("ucd_register: Registered device %s (%s)", a->name, a->lname));

	a->next = allucdifs;
	allucdifs = a;

	result = (*a->open)(a);
	if (result)
	{
		DEBUG(("ucd_open: Cannot open USB host driver %s%d", a->name, a->unit));
		return -1;
	}

        result = (*a->ioctl)(a, LOWLEVEL_INIT, 0);
        if (result)
        {
                DEBUG (("%s: ucd low level init failed!", __FILE__));
                return -1;
	}

        /* if lowlevel init is OK, scan the bus for devices
         * i.e. search HUBs and configure them 
         */

        dev = usb_alloc_new_device(a);
        if (!dev) 
	{
		return -1;
	}
	
	if (usb_new_device(dev)) 
	{
		return -1;
	}

	usb_hub_events(dev); /* let's look at the hub events immediately */

#ifndef TOSONLY
	// disabled for now.
	usb_hub_init(dev);
#endif

	return 0;
}

/*
 * Called by ucd upon unloading to unregister itself
 */

long
ucd_unregister(struct ucdif *a)
{
	struct ucdif **list = &allucdifs;

	(*a->close)(a);
        (*a->ioctl)(a, LOWLEVEL_STOP, 0);

	while (*list)
	{
		if (a == *list)
		{
			*list = a->next;
			return E_OK;
		}
		list = &((*list)->next);
	}
	return -1L;
}

long
ucd_ioctl(struct ucdif *u, short cmd, long arg)
{
	return (*u->ioctl)(u, cmd, arg);
}

