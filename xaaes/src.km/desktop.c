/*
 * $Id$
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

#include RSCHNAME

#include "desktop.h"
#include "xa_global.h"

#include "app_man.h"
#include "objects.h"
#include "menuwidg.h"
#include "widgets.h"
#include "xa_rsrc.h"
#include "xalloc.h"

/*
 * Desktop Handling Routines
 */

XA_TREE *
get_desktop(void)
{
	return get_widget(root_window, XAW_TOOLBAR)->stuff;
}

bool
click_desktop_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_client *mowner = menu_owner();
	struct xa_client *client = desktop_owner();

	DIAG((D_button, NULL, "click_desktop_widget, desktop owner: %s", c_owner(client) ));
	DIAG((D_button, NULL, "                         menu owner: %s", c_owner(mowner) ));

	/* HR 280801!!!! menu, desktop and rootwindow ownership are all different!!! */
	/* Ozk:	Trying to get away from vq_mouse() usage. Initial mouse status to act
	 *	upon is always found in mu_button structure.
	*/
	if (!mouse_locked() && mowner != client && (mu_button.b & 1))
	{
		int item, b;

		item = find_object(get_desktop()->tree, 0, 1, widg->mx, widg->my, 0, 0);

		DIAG((D_button, NULL, "  --  item %d", item));

		/* button must be released on the root object. */

		check_mouse(client, &b, 0, 0);

		if (b == 0 && item == 0)
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
	XA_TREE *nt = xcalloc(1, sizeof(XA_TREE), 103);
	XA_WIDGET *wi = get_widget(wind, XAW_TOOLBAR);
	XA_WIDGET_LOCATION loc;

	DIAG((D_widg, NULL, "set_desktop_widget(wind=%d):new@%lx",
		wind->handle, nt));

	if (!nt)
	{
		DIAG((D_widg, NULL, " - unable to allocate widget."));
		return;
	}

	/* desktop widget.tree */
	*nt = *desktop;	

	bzero(&loc, sizeof(loc));
	loc.relative_type = LT;
	loc.r = wind->r;
	loc.r.y += MENU_H + 1;
	loc.r.w -= MENU_H + 1;
	loc.n = XAW_TOOLBAR;
	loc.mask = TOOLBAR;

	wi->display = display_object_widget;
	wi->click = click_desktop_widget;
	wi->dclick = click_desktop_widget;
	wi->drag = click_desktop_widget;
	wi->loc = loc;
	wi->state = OS_NORMAL;
	wi->stuff = nt;
	wi->start = 0;
}


/*
 * Redraw the desktop object tree
 * - Blindingly simple or what?
 * HR 270801: now a widget, so it is drawn in the standard widget way. :-)
 *            this function only for non standard drawing (cleanup) 
 */
int
redraw_desktop(enum locks lock, struct xa_window *wind)
{
	XA_TREE *desktop = get_desktop();

	if (desktop == NULL)
		return false;
	if (desktop->tree == NULL)
		return false;

	draw_object_tree(lock, NULL, desktop->tree, 0, MAX_DEPTH, 10);
	return true;
}

/*
 * Set a new object tree for the desktop 
 */
void
set_desktop(XA_TREE *new_desktop)
{
	OBJECT *ob; RECT r;
	XA_WIDGET *wi = get_widget(root_window, XAW_TOOLBAR);
	XA_TREE *desktop = wi->stuff;

	/* Set the desktop */
	*desktop = *new_desktop;

	ob = desktop->tree;
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
