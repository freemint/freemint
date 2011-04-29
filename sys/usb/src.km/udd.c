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
#include "udd.h"

struct uddif *alluddifs = NULL;

LIST_HEAD(,usb_driver) usb_driver_list = LIST_HEAD_INITIALIZER(usb_driver_list);


/*
 * Called by udd upon init to register itself
 */
long
udd_register(struct uddif *a, struct usb_driver *new_driver)
{
	DEBUG(("udd_register: Registered device %s (%s)", a->name, a->lname));
	a->next = alluddifs;
	alluddifs = a;

	/* Add it to the list of known drivers */
	LIST_INSERT_HEAD(&usb_driver_list, new_driver, chain);

	return 0;
}

/*
 * Called by udd upon unloading to unregister itself
 */

long
udd_unregister(struct uddif *a, struct usb_driver *driver)
{
	struct uddif **list = &alluddifs;

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

/*
 * Get an unused unit number for interface name 'name'
 */
short
udd_getfreeunit (char *name)
{
	struct uddif *uddp;
	short max = -1;

	for (uddp = alluddifs; uddp; uddp = uddp->next)
	{
		if (!strncmp (uddp->name, name, UDD_NAMSIZ) && uddp->unit > max)
			max = uddp->unit;
	}

	return max+1;
}

struct uddif *
udd_name2udd (char *aname)
{
	char name[UDD_NAMSIZ+1], *cp;
	short i;
	long unit = 0;
	struct uddif *a;

	for (i = 0, cp = aname; i < UDD_NAMSIZ && *cp; ++cp, ++i)
	{
		if (*cp >= '0' && *cp <= '9')
		{
			unit = atol (cp);
			break;
		}
		name[i] = *cp;
	}

	name[i] = '\0';
	for (a = alluddifs; a; a = a->next)
	{
		if (!stricmp (a->name, name) && a->unit == unit)
			return a;
	}

	return NULL;
}


long
udd_ioctl(struct uddif *u, short cmd, long arg)
{
	return (*u->ioctl)(u, cmd, arg);
}

