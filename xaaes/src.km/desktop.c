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

/*
 * Desktop Handling Routines
 */

#include RSCHNAME

#include "desktop.h"
#include "xa_global.h"

#include "app_man.h"
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
	return get_widget(root_window, XAW_TOOLBAR)->stuff;
}

bool
click_desktop_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
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
		int item;

		item = obj_find(get_desktop(), 0, 1, md->x, md->y, NULL);

		DIAG((D_button, NULL, "  --  item %d", item));

		/* button must be released on the root object. */

		if (md->cstate == 0 && item == 0)
		{
			/* Also unhides the windows. */
			app_in_front(lock, client);

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
	XA_WIDGET *wi = get_widget(wind, XAW_TOOLBAR);
	XA_WIDGET_LOCATION loc;

	DIAG((D_widg, NULL, "set_desktop_widget(wind = %d):new@0x%lx",
		wind->handle, desktop));

	if (wi->stuff)
		((XA_TREE *)wi->stuff)->widg = NULL;

	desktop->widg = wi;

	bzero(&loc, sizeof(loc));
	loc.relative_type = LT;
	loc.r = wind->r;
	loc.r.y += MENU_H + 1;
	loc.r.w -= MENU_H + 1;
	loc.n = XAW_TOOLBAR;
	loc.mask = TOOLBAR;

	wi->r = loc.r;

	wi->display = display_object_widget;
	wi->click = click_desktop_widget;
	wi->dclick = click_desktop_widget;
	wi->drag = click_desktop_widget;
	wi->loc = loc;
	wi->state = OS_NORMAL;

	/* Ozk;
	 * Set destruct, stufftype and XAWF_STUFFMALLOC and 'nt' is freed.
	 * Set XAWF_STUFFMALLOC and nt is freed by free_xawidget()
	 */
	wi->stuff = desktop;
	wi->stufftype = STUFF_IS_WT;
	wi->destruct = free_xawidget_resources;
	
	wi->start = 0;
}

/*
 * Set a new object tree for the desktop 
 */
static void
Set_desktop(XA_TREE *new_desktop)
{
	OBJECT *ob; RECT r;
	XA_WIDGET *wi = get_widget(root_window, XAW_TOOLBAR);

	/* Set the desktop */

	if (wi->stuff)
	{
		((XA_TREE *)wi->stuff)->widg = NULL;
		((XA_TREE *)wi->stuff)->flags &= ~WTF_STATIC;
	}
	new_desktop->widg = wi;
	wi->owner = new_desktop->owner;
	new_desktop->flags |= WTF_STATIC;

	ob = new_desktop->tree;
	r = *(RECT*)&ob->ob_x;

	/* Now use the root window's auto-redraw function to redraw it
	 * 
	 * HR: 110601 fixed erroneous use owner->wt.
         *     The desktop can of course be handled by tool_bar widget stuff!!!
	 */

	DIAGS(("desktop: %d/%d,%d/%d",r.x,r.y,r.w,r.h));

	/* HR 010501: general fix */
	if (root_window->r.h > r.h)
		wi->loc.r.y = ob->ob_y = root_window->r.h - r.h;

	wi->stuff = new_desktop;
	wi->stufftype = STUFF_IS_WT;
	wi->destruct = free_xawidget_resources;

	{
		send_iredraw(0, root_window, 0, NULL);
#if 0
		struct xa_window *wl = root_window;
		struct xa_rect_list *rl;

		make_rect_list(wl, true);
		rl = wl->rect_start;
		while (rl)
		{
			//set_clip(&rl->r);
			//draw_window(0, wl);
			generate_redraws(0, wl, &rl->r, RDRW_ALL);
			rl = rl->next;
		}
#endif
	}
}
static void
CE_set_desktop(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		XA_TREE *newdesk = ce->ptr1;
		DIAGS((" CE_set_desktop: newdesk = %lx (obtree = %lx, owner %s) for %s",
			newdesk, newdesk->tree, newdesk->owner->name, ce->client->name));

		Set_desktop(newdesk);
		//display_window(lock, 1, root_window, NULL);
	}
}
void
set_desktop(XA_TREE *new_desktop)
{
	struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);

	if (!rc)
		rc = C.Aes;

	if (rc == new_desktop->owner || new_desktop->owner == C.Aes)
	{
		DIAGS((" set_desktop: Direct from %s to %s",
			rc->name, new_desktop->owner->name));

		Set_desktop(new_desktop);
		//display_window(0, 2, root_window, NULL);
	}
	else
	{
		DIAGS((" set_desktop: posting cevent (%lx) to %s",
			(long)CE_set_desktop, new_desktop->owner->name));

		post_cevent(new_desktop->owner,
			    CE_set_desktop,
			    new_desktop,
			    NULL,
			    0, 0,
			    NULL,
			    NULL);
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
