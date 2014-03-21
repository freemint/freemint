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

#include "global.h"
#include "usb.h"
#include "ucd/ucd_defs.h"
#include "udd.h"
#include <mint/osbind.h> /* Setexc */


struct uddif *alluddifs = NULL;

struct usb_driver *alldrivers = NULL;


/*
 * Called by udd upon init to register itself
 */
long
udd_register(struct uddif *a, struct usb_driver *new_driver)
{
	DEBUG(("udd_register: Registered device %s (%s)", a->name, a->lname));

	a->next = alluddifs;
	alluddifs = a;

	new_driver->next = alldrivers;
	alldrivers = new_driver;

	return 0;
}

/*
 * Called by udd upon unloading to unregister itself
 */

long
udd_unregister(struct uddif *a, struct usb_driver *driver)
{
	struct uddif **list = &alluddifs;
	struct usb_driver **listdrv = &alldrivers;

	while (*list)
	{
		if (a == *list)
		{
			*list = a->next;
			break;
		}
		list = &((*list)->next);
	}

	while (*listdrv)
	{
		if (driver == *listdrv)
		{
			*listdrv = driver->next;
			return E_OK;
		}
		listdrv = &((*listdrv)->next);
	}
	return -1L;
}

long
udd_open(struct uddif *a)
{
	long error;

	error = (*a->open)(a);
	if (error)
	{
		DEBUG(("udd_open: Cannot open USB host driver %s%d", a->name, a->unit));
		return error;
	}
	a->flags |= UDD_OPEN;
	return 0;
}

long
udd_close(struct uddif *a)
{
	long error;

	error = (*a->close)(a);
	if (error)
	{
		DEBUG(("udd_close: Cannot close USB host driver %s%d", a->name, a->unit));
		return error;
	}
	a->flags &= ~UDD_OPEN;
	return 0;
}

long
udd_ioctl(struct uddif *u, short cmd, long arg)
{
	return (*u->ioctl)(u, cmd, arg);
}

