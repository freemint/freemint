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
 
#include "c_keybd.h"
#include "k_keybd.h"
#include "xa_types.h"

void
cXA_fmdkey(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_client *client = ce->client;
	struct rawkey *key = ce->ptr1;

	if (!cancel)
	{
		DIAG((D_keybd, client, "Deliver fmd.keybress to %s", client->name));

		client->fmd.keypress(lock, client, NULL, client->fmd.wt, key);
	}
	kfree(key);
}
void
cXA_keypress(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_client *client = ce->client;
	struct rawkey *key = ce->ptr1;
	struct xa_window *wind = ce->ptr2;

	if (!cancel)
	{
		DIAG((D_keybd, client, "cXA_keypress for %s", client->name));

		wind->keypress(lock, client, wind, NULL, key);
	}
	kfree(key);
}
void
cXA_keybd_event(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_client *client = ce->client;
	struct rawkey *key = ce->ptr1;

	if (!cancel)
		queue_key(client, key);

		//keybd_event(lock, client, key);
	kfree(key);
}
