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
#include "ucd.h"

struct ucdif *allucdifs = NULL;

/*
 * Called by ucd upon init to register itself
 */
long
ucd_register(struct ucdif *a)
{
	DEBUG(("ucd_register: Registered device %s (%s)", a->name, a->lname));
	a->next = allucdifs;
	allucdifs = a;

	return 0;
}

/*
 * Called by ucd upon unloading to unregister itself
 */

long
ucd_unregister(struct ucdif *a)
{
	struct ucdif **list = &allucdifs;

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
ucd_open(struct ucdif *a)
{
	long error;

	error = (*a->open)(a);
	if (error)
	{
		DEBUG(("ucd_open: Cannot open USB host driver %s%d", a->name, a->unit));
		return error;
	}
	a->flags |= UCD_OPEN;
	return 0;
}

long
ucd_close(struct ucdif *a)
{
	long error;

	error = (*a->close)(a);
	if (error)
	{
		DEBUG(("ucd_close: Cannot close USB host driver %s%d", a->name, a->unit));
		return error;
	}
	a->flags &= ~UCD_OPEN;
	return 0;
}

/*
 * Get an unused unit number for interface name 'name'
 */
short
ucd_getfreeunit (char *name)
{
	struct ucdif *ucdp;
	short max = -1;

	for (ucdp = allucdifs; ucdp; ucdp = ucdp->next)
	{
		if (!strncmp (ucdp->name, name, UCD_NAMSIZ) && ucdp->unit > max)
			max = ucdp->unit;
	}

	return max+1;
}

struct ucdif *
ucd_name2ucd (char *aname)
{
	char name[UCD_NAMSIZ+1], *cp;
	short i;
	long unit = 0;
	struct ucdif *a;

	for (i = 0, cp = aname; i < UCD_NAMSIZ && *cp; ++cp, ++i)
	{
		if (*cp >= '0' && *cp <= '9')
		{
			unit = atol (cp);
			break;
		}
		name[i] = *cp;
	}

	name[i] = '\0';
	for (a = allucdifs; a; a = a->next)
	{
		if (!stricmp (a->name, name) && a->unit == unit)
			return a;
	}

	return NULL;
}


long
ucd_ioctl(struct ucdif *u, short cmd, long arg)
{
	return (*u->ioctl)(u, cmd, arg);
}

