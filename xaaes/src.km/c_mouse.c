/*
 * $Id: c_mouse.c
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for MiNT
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

/*
 * This file contains the client side of mouse event processing
*/

#include "c_mouse.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "desktop.h"
#include "init.h"
#include "k_init.h"
#include "k_main.h"
#include "k_mouse.h"
#include "k_shutdown.h"
#include "nkcc.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xalloc.h"


#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/signal.h"

void
cXA_button_event(enum locks lock, struct c_event *ce)
{
	struct xa_client *client = ce->client;
	struct xa_window *wind = (struct xa_window *)ce->ptr1;
	struct moose_data *md = &ce->md;

	DIAG((D_button, client, "cXA_button_event for %s - %d/%d, state=%d, clicks=%d - ptr1=%lx, ptr2=%lx, %lx/%lx",
		client->name, md->x, md->y, md->state, md->clicks,
		ce->ptr1, ce->ptr2, wind, md));

	if (wind == 0)
	{
		if (C.menu_base && md->state && C.menu_base->client == client)
		{
			Tab *tab = C.menu_base;
			DIAG((D_button, client, "cXA_button_event: Menu click"));
			if (tab->ty)
			{
				MENU_TASK *k = &tab->task_data.menu;
				wind = k->popw;

				/* HR 161101: widgets in scrolling popups */
				if (wind)
				{
					if (wind->owner != client)
					{
						DIAG((D_button, client, "cXA_button_event: Wrong client %s, should be %s", wind->owner->name, client->name));
						return;
					}
					if (   (wind->dial&created_for_POPUP) != 0
					    && (wind->active_widgets&V_WIDG) != 0
					   )
					{
						if (do_widgets(lock, wind, XaMENU, md))
							return;
					}
				}

				if (k->entry)
				{
					if (tab->client != client)
					{
						DIAG((D_button, client, "cXA_button_event: Wrong owner of Tab %s, should be %s", tab->client->name, client->name));
						return;
					}
					k->x = md->x;
					k->y = md->y;
					k->entry(tab);
					return;
				}
			}
		}
		return;
	}
	
	if (wind == window_list || (wind != window_list && wind->active_widgets & NO_TOPPED) )
	{
		DIAG((D_button, client, "cXA_button_event: Topped win"));
		if (do_widgets(lock, wind, 0, md))
		{
			return;
		}
		else if (client->waiting_for & MU_BUTTON)
		{
			button_event(lock, client, md);
			return;
		}
		else
		{
			add_pending_button(lock, client);
			return;
		}
	}

	if (wind != window_list)
	{
		DIAG((D_button, client, "cXA_button_event: wind not on top"));
		if (do_widgets(lock, wind, 0, md))
			return;
	}	

	if (md->state)
	{
		DIAG((D_button, client, "cXA_button_event: send click"));
		wind->send_message(lock, wind, NULL, WM_TOPPED, 0, 0, wind->handle, 0, 0, 0, 0);
		return;
	}
}

void
cXA_deliver_button_event(enum locks lock, struct c_event *ce)
{
	DIAG((D_button, ce->client, "cXA_deliver_button_event: to %s", ce->client->name));
	button_event(lock, ce->client, &ce->md);
}

void
cXA_deliver_rect_event(enum locks lock, struct c_event *ce)
{
	struct xa_client *client = ce->client;
	AESPB *pb = client->waiting_pb;
	int events = ce->d0;

	if (pb)
	{
		if (client->waiting_for & XAWAIT_MULTI)
		{
			multi_intout(client, pb->intout, events);
		}
		else
		{
			multi_intout(client, pb->intout, 0);
			pb->intout[0] = 1;
		}
	}
	client->usr_evnt = 1;
}

void
cXA_form_do(enum locks lock, struct c_event *ce)
{
	struct xa_client *client = ce->client;

	DIAG((D_mouse, client, "cXA_form_do for %s", client->name));

	client->fmd.mousepress(lock, client, &ce->md);
}

void
cXA_menu_move(enum locks lock, struct c_event *ce)
{
	if (C.menu_base->client == ce->client)
	{
		MENU_TASK *k = &C.menu_base->task_data.menu;
		int x = ce->md.x;
		int y = ce->md.y;

		DIAG((D_mouse, ce->client, "cXA_menu_move for %s", ce->client->name));

		if (k->em.flags & MU_MX)
		{
			/* XaAES internal flag: report any mouse movement. */

			k->em.flags = 0;
			k->x = x;
			k->y = y;
			k->em.t1(C.menu_base);	/* call the function */
		}
		else if (k->em.flags & MU_M1)
		{
			if (is_rect(x, y, k->em.flags & 1, &k->em.m1))
			{
				k->em.flags = 0;
				k->x = x;
				k->y = y;
				k->em.t1(C.menu_base);	/* call the function */
			}
			else
			/* HR: MU_M2 not used for menu's anymore, replaced by MU_MX */
			/* I leave the text in, because one never knows. */
			if (k->em.flags & MU_M2)
			{
				if (is_rect(x, y, k->em.flags & 2, &k->em.m2))
				{
					k->em.flags = 0;
					k->x = x;
					k->y = y;
					k->em.t2(C.menu_base);
				}
			}
		}
	}
}

void
cXA_active_widget(enum locks lock, struct c_event *ce)
{
	DIAG((D_mouse, ce->client, "cXA_active_widget for %s", ce->client->name));
	do_active_widget(lock, ce->client);
}

/*
 * Also used to open a menu
*/
void
cXA_widget_click(enum locks lock, struct c_event *ce)
{
	XA_WIDGET *widg = ce->ptr1;

	DIAG((D_mouse, ce->client, "cXA_widget_click for %s", ce->client->name));
	widg->click(lock, root_window, widg);
}
