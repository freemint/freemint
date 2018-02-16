/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "adi.h"
#include "xa_global.h"


static struct adif *alladifs = NULL;

/*
 * Called by adi upon init to register itself
 */
long
adi_register(struct adif *a)
{
	DIAGS(("adi_register: Registered device %s (%s)", a->name, a->lname));
	a->next = alladifs;
	alladifs = a;

	return 0;
}

/*
 * Called by adi upon unloading to unregister itself
 * XXX this is called from k_main, after adi_close when
 * shutting down. This should be done automatically when
 * unloading the module
 */

long
adi_unregister(struct adif *a)
{
	struct adif **list = &alladifs;

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
adi_open(struct adif *a)
{
	long error;

	error = (*a->open)(a);
	if (error)
	{
		DIAGS(("adi_open: Cannot open aes device interface %s%d", a->name, a->unit));
		return error;
	}
	a->flags |= ADI_OPEN;
	return 0;
}

long
adi_close(struct adif *a)
{
	long error;

	error = (*a->close)(a);
	if (error)
	{
		DIAGS(("adi_close: Cannot close AES device interface %s%d", a->name, a->unit));
		return error;
	}
	a->flags &= ~ADI_OPEN;
	return 0;
}

/*
 * Get an unused unit number for interface name 'name'
 */
short
adi_getfreeunit (char *name)
{
	struct adif *adip;
	short max = -1;

	for (adip = alladifs; adip; adip = adip->next)
	{
		if (!strncmp (adip->name, name, ADI_NAMSIZ) && adip->unit > max)
			max = adip->unit;
	}

	return max+1;
}

struct adif *
adi_name2adi (char *aname)
{
	char name[ADI_NAMSIZ+1], *cp;
	short i;
	short unit = 0;
	struct adif *a;

	for (i = 0, cp = aname; i < ADI_NAMSIZ && *cp; ++cp, ++i)
	{
		if (*cp >= '0' && *cp <= '9')
		{
			unit = atol (cp);
			break;
		}
		name[i] = *cp;
	}

	name[i] = '\0';
	for (a = alladifs; a; a = a->next)
	{
		if (!stricmp (a->name, name) && a->unit == unit)
			return a;
	}

	return NULL;
}

long
adi_ioctl(struct adif *a, short cmd, long arg)
{
	return (*a->ioctl)(a, cmd, arg);
}
