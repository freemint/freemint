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

/*
 * Desktop Handling Routines
 */

#include "desktop.h"
#include "xa_global.h"

#include "xaaes.h"

#include "app_man.h"
#include "xa_appl.h"
#include "c_window.h"
#include "rectlist.h"
#include "k_main.h"
#include "k_mouse.h"
#include "draw_obj.h"
#include "obtree.h"
#include "menuwidg.h"
#include "widgets.h"
#include "xa_rsrc.h"


XA_TREE *
get_desktop(void)
{
	return get_widget(root_window, XAW_TOOLBAR)->stuff.wt;
}

static bool
click_desktop_widget(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	struct xa_client *mowner = menu_owner();
	struct xa_client *client = desktop_owner();

	DIAG((D_button, NULL, "click_desktop_widget, desktop owner: %s", c_owner(client) ));
	DIAG((D_button, NULL, "                         menu owner: %s", c_owner(mowner) ));


	/* HR 280801!!!! menu, desktop and rootwindow ownership are all different!!! */
	/* Ozk:	Trying to get away from vq_mouse() usage. Initial mouse status to act
	 *	upon is always found in mu_button structure.
	*/
	if (!mouse_locked() && mowner != client && (md->state & 1))
	{
		struct xa_aes_object item;

		item = obj_find(get_desktop(), aesobj(get_desktop()->tree, 0), 1, md->x, md->y, NULL);

		DIAG((D_button, NULL, "  --  item %d", aesobj_item(&item)));

		/* button must be released on the root object. */

		if (md->cstate == 0 && aesobj_item(&item) == 0)
		{
			/* Also unhides the windows. */
			app_in_front(lock, client, true, true, true);

			DIAG((D_button, NULL, "click_desktop_widget done"));

			/* click is used */
			return true;
		}
	}
	/* pass click to be used by evnt manager. */
	return false;
}

/* HR 270801
 * Attach a desktop to a window
 */
void
set_desktop_widget(struct xa_window *wind, XA_TREE *desktop)
{
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	struct xa_widget_methods *m = &widg->m;

	DIAG((D_widg, NULL, "set_desktop_widget(wind = %d):new@0x%lx",
		wind->handle, (unsigned long)desktop));

	if (widg->stuff.wt)
	{
		widg->stuff.wt->widg = NULL;
		widg->stuff.wt->links--;
	}

	desktop->widg = widg;
	desktop->links++;

	widg->r = wind->wa;
#if 0
	widg->r = wind->r;
	widg->r.g_y += get_menu_widg()->r.g_h;
	widg->r.g_h -= get_menu_widg()->r.g_h;
#endif

	bzero(&widg->m, sizeof(*m));

	m->r.xaw_idx = XAW_TOOLBAR;
	m->r.pos_in_row = LT;
	m->r.tp = TOOLBAR;

	m->r.draw = display_object_widget;
	m->r.setsize = NULL;
	m->properties = WIP_INSTALLED|WIP_WACLIP;
	m->click = click_desktop_widget;
	m->drag = click_desktop_widget;
	widg->state = OS_NORMAL;

	/* Ozk;
	 * Set destruct, stufftype and XAWF_STUFFMALLOC and 'nt' is freed.
	 * Set XAWF_STUFFMALLOC and nt is freed by free_xawidget()
	 */
	widg->stuff.wt = desktop;
	widg->stufftype = STUFF_IS_WT;
	m->destruct = free_xawidget_resources;

	widg->start = 0;
}

/*
 * Set a new object tree for the desktop
 */
static void
Set_desktop(XA_TREE *new_desktop)
{
	OBJECT *ob;
	XA_WIDGET *wi = get_widget(root_window, XAW_TOOLBAR);
	short menu_bar = 0;

	/* Set the desktop */

	/* temporarily switch menubar off to set root-object of desktop to secreen-size (works with teradesk) */
	if( cfg.menu_bar != 2 && cfg.menu_bar && new_desktop->owner != C.Aes )
	{
		menu_bar = cfg.menu_bar;
		cfg.menu_bar = 0;
		set_standard_point( new_desktop->owner );
	}
	if (wi->stuff.wt)
	{
		wi->stuff.wt->widg = NULL;
		wi->stuff.wt->flags &= ~WTF_STATIC;
		wi->stuff.wt->links--;
	}

	new_desktop->widg = wi;
	wi->owner = new_desktop->owner;
	new_desktop->flags |= WTF_STATIC;
	new_desktop->links++;

	ob = new_desktop->tree;
	*(GRECT *)&ob->ob_x = root_window->wa;
	/* Now use the root window's auto-redraw function to redraw it
	 *
	 * HR: 110601 fixed erroneous use owner->wt.
         *     The desktop can of course be handled by tool_bar widget stuff!!!
	 */

	DIAGS(("desktop: %d/%d,%d/%d", ob->ob_x, ob->ob_y, ob->ob_width, ob->ob_height));

	wi->stuff.wt = new_desktop;
	wi->stufftype = STUFF_IS_WT;
	wi->m.destruct = free_xawidget_resources;

	if( cfg.menu_bar != 2 && menu_bar && new_desktop->owner != C.Aes )
	{
		cfg.menu_bar = menu_bar;
		set_standard_point( new_desktop->owner );
	}
	//send_iredraw(0, root_window, 0, NULL);
}
static void
CE_set_desktop(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		XA_TREE *newdesk = ce->ptr1;
		DIAGS((" CE_set_desktop: newdesk = %lx (obtree = %lx, owner %s) for %s",
			(unsigned long)newdesk, (unsigned long)newdesk->tree, newdesk->owner->name, ce->client->name));

		Set_desktop(newdesk);
		send_iredraw(0, root_window, 0, NULL);
	}
}

void
set_desktop(struct xa_client *client, bool remove)
{
	struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);

	if (!rc)
		rc = C.Aes;

	if (remove)
	{
		struct xa_client *new = NULL;

		/* find a prev app's desktop. */
		if (get_desktop() == client->desktop)
		{
			/*
			 * Set to AES's desktop, this will unlink current desktop
			 */
			Set_desktop(C.Aes->desktop);
			/* Ozk:
			 * If caller aint the desktop, we try to set the desktop's dektop (uhm.. yes)
			 */
			new = pid2client(C.DSKpid);
			if (!new || client == new || !new->desktop)
				new = find_desktop(0, client, 3);
		}
		client->desktop = NULL;
		client = new;
	}

	if (client && client->desktop)
	{
		if (rc == client || client == C.Aes)
		{
			DIAGS((" set_desktop: Direct from %s to %s",
				rc->name, client->name));

			Set_desktop(client->desktop);
			send_iredraw(0, root_window, 0, NULL);
		}
		else
		{
			DIAGS((" set_desktop: posting cevent (%lx) to %s",
				(long)CE_set_desktop, client->name));

			post_cevent(client,
				    CE_set_desktop,
				    client->desktop,
				    NULL,
				    0, 0,
				    NULL,
				    NULL);
		}
	}
}
struct xa_client *
desktop_owner(void)
{
	return get_desktop()->owner;
}

OBJECT *
get_xa_desktop(void)
{
	return ResourceTree(C.Aes_rsc, DEF_DESKTOP);
}
