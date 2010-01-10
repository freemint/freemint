/*
 * $Id$
 *
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

#include "xad.h"
#include "xa_global.h"

struct loaded_xad
{
	struct loaded_xad	*next;
	struct xad		*this;
};

static struct loaded_xad *current_xads = NULL;

/*
 * Called by adi upon init to register itself
 */
long
xad_register(struct xad *a)
{
	DIAGS(("xad_register: Registered device %s (%s)", a->name, a->description));
	struct loaded_xad *lxad = kmalloc(sizeof(*lxad));
	if (lxad)
	{
		lxad->next = current_xads;
		lxad->this = a;
		current_xads = lxad;
		return 0;
	}
	return -1L;
}

/*
 * Called by adi upon unloading to unregister itself
 * XXX this is called from k_main, after xad_close when
 * shutting down. This should be done automatically when
 * unloading the module
 */
long
xad_unregister(struct xad *a)
{
	struct loaded_xad **list = &current_xads;
	struct loaded_xad *this = NULL;

	while (*list)
	{
		if ((*list)->this == a) {
			this = *list;
			*list = this->next;
			break;
		}
		list = (&(*list)->next);
	}

	if (this)
	{
		if (this->this == G.adi_mouse)
			G.adi_mouse = NULL;
		kfree(this);
		return E_OK;
	}

	return -1L;
}

long
xad_open(struct xad *a)
{
	long error;

	error = (*a->open)();
	if (error)
	{
		DIAGS(("xad_open: Cannot open aes device interface %s%d", a->name, a->unit));
		return error;
	}
	a->flags |= XAD_OPEN;
	return 0;
}

long
xad_close(struct xad *a)
{
	long error;

	error = (*a->close)();
	if (error)
	{
		DIAGS(("xad_close: Cannot close AES device interface %s%d", a->name, a->unit));
		return error;
	}
	a->flags &= ~XAD_OPEN;
	return 0;
}

/*
 * Get an unused unit number for interface name 'name'
 */
short
xad_getfreeunit (char *name)
{
	struct loaded_xad *lxad;
	short max = -1;

	for (lxad = current_xads; lxad; lxad = lxad->next)
	{
		if (!strncmp(lxad->this->name, name, XAD_NAMSIZ) && lxad->this->unit > max)
		{
			max = lxad->this->unit;
		}
	}
	return max + 1;
}

struct xad *
xad_name2adi (char *aname)
{
	char name[XAD_NAMSIZ+1], *cp;
	short i;
	long unit = 0;
	struct loaded_xad *lxad;

	for (i = 0, cp = aname; i < XAD_NAMSIZ && *cp; ++cp, ++i)
	{
		if (*cp >= '0' && *cp <= '9')
		{
			unit = atol (cp);
			break;
		}
		name[i] = *cp;
	}

	name[i] = '\0';

	for (lxad = current_xads; lxad; lxad = lxad->next )
	{
		if (!stricmp(lxad->this->name, name) && lxad->this->unit == unit)
			return lxad->this;

	}
	return NULL;
}

long
xad_ioctl(struct xad *a, short cmd, long arg)
{
	return (*a->ioctl)(cmd, arg);
}
