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
 * Application Manager
 * This module handles various ways of swapping the current application.
 */

#include "app_man.h"
#include "xa_global.h"
#include "xa_rsrc.h"

#include "semaphores.h"
#include "c_window.h"
#include "desktop.h"
#include "menuwidg.h"
#include "widgets.h"

#include "mint/signal.h"


bool
taskbar(struct xa_client *client)
{
#if AP_TASKBAR
	if (strcmp(client->proc_name, "TASKBAR ") == 0
	    || strcmp(client->proc_name, "MLTISTRP") == 0)
		return true;

	return false;
#else
	return true;
#endif
}

struct xa_client *
focus_owner(void)
{
	if (C.focus == root_window)
		return menu_owner();

	if (C.focus)
		return C.focus->owner;

	DIAGS(("No focus_owner()???"));
	return NULL;
}

/*
 * Attempt to recover a system that has locked up
 */
void
recover(void)
{
	struct xa_client *update_lock = update_locked();
	struct xa_client *mouse_lock = mouse_locked();

	DIAG((D_appl, NULL, "Attempting to recover control....."));

	if (update_lock && (update_lock != C.Aes))
	{
		DIAG((D_appl, NULL, "Killing owner of update lock"));
		ikill(update_lock->p->pid, SIGKILL);
	}

	if ((mouse_lock && (mouse_lock != update_lock)) && (mouse_lock != C.Aes))
	{
		DIAG((D_appl, NULL, "Killing owner of mouse lock"));
		ikill(mouse_lock->p->pid, SIGKILL);
	}

	forcem();
}

/*
 * Swap the main root window's menu-bar to be another application's
 * NOTE: This only swaps the menu-bar, it doesn't swap the topped window.
 *
 * See also click_menu_widget() for APP's
 */
void
swap_menu(enum locks lock, struct xa_client *new, bool do_desk, int which)
{
	struct xa_window *top;
	struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);
	XA_WIDGET *widg = get_menu_widg();

	DIAG((D_appl, NULL, "[%d]swap_menu", which));

	/* If the new client has no menu bar, no need for a change */
	if (new->std_menu)
	{
		DIAG((D_appl, NULL, "  --   to %s", c_owner(new)));

		/* menu widget.tree */
		if (new->std_menu != widg->stuff) /* Different menu? */
		{
			DIAG((D_appl, NULL, "swapped to %s",c_owner(new)));

			if (!rc)
				rc = C.Aes;

			new->status |= CS_WAIT_MENU;
			lock_menustruct(rc, false);
			if (widg->stuff)
			{
				((XA_TREE *)widg->stuff)->widg = NULL;
				((XA_TREE *)widg->stuff)->flags &= ~WTF_STATIC;
			}
			widg->stuff = new->std_menu;
			new->std_menu->flags |= WTF_STATIC;
			new->std_menu->widg = widg;
			unlock_menustruct(rc);
			new->status &= ~CS_WAIT_MENU;

			top = window_list;

			DIAG((D_appl, NULL, "top: %s", w_owner(top)));

			if (top != root_window && top->owner != new)
			{
				/* untop other pids windows */
				C.focus = root_window;
				DIAG((D_appl, NULL, "Focus to root_window."));
				display_window(lock, 110, top, NULL);   /* Redisplay titles */
				send_untop(lock, top);
			}
			else if (C.focus == root_window && top->owner == new)
			{
				C.focus = top;
				DIAG((D_appl, NULL, "Focus to top_window %s", w_owner(top)));
				display_window(lock, 111, top, NULL);   /* Redisplay titles */
				send_ontop(lock);
			}
			else if (top->owner != new)
			{
				DIAG((D_appl, NULL, "Set focus to root_window."));
				C.focus = root_window;
			}
		}
		else
		{
			DIAG((D_appl, NULL, "Same menu %s", c_owner(new)));
		}
	}

	/* Change desktops? */
	if (   do_desk
	    && new->desktop
	    && new->desktop->tree != get_desktop()->tree
	    && new->desktop->tree != get_xa_desktop())
	{
		DIAG((D_appl, NULL, "  --   with desktop=%lx", new->desktop));
		set_desktop(new->desktop); //set_desktop(&new->desktop);
		//display_window(lock, 30, root_window, NULL);
		redraw_menu(lock);
	}
	else if (new->std_menu)
	{
		/* No - just change menu bar */
		DIAG((D_appl, NULL, "redrawing menu..."));
		redraw_menu(lock);
	}
	DIAG((D_appl, NULL, "exit ok"));
}

XA_TREE *
find_menu_bar(enum locks lock)
{
	XA_TREE *rtn = C.Aes->std_menu; //&(C.Aes->std_menu); /* default */
	struct xa_client *last;

	Sema_Up(clients);

	last = S.client_list;
	while (last->next)
		last = last->next;

	while (last)
	{
		if (last->std_menu)
		{
			rtn = last->std_menu; //&last->std_menu;
			DIAGS(("found std_menu %lx", rtn));
			break;
		}

		last = last->prior;
	}

	Sema_Dn(clients);

	return rtn;
}

struct xa_client *
find_desktop(enum locks lock)
{
	struct xa_client *last, *rtn = C.Aes; /* default */

	Sema_Up(clients);

	last = S.client_list;
	while (last->next)
		last = last->next;

	while (last)
	{
		if (last->desktop)
		{
			rtn = last;
			DIAGS(("found desktop %lx", rtn));
			break;
		}
		last = last->prior;
	}

	Sema_Dn(clients);
	return rtn;
}

void
unhide_app(enum locks lock, struct xa_client *client)
{
	struct xa_window *w;

	w = window_list;
	while (w)
	{
		if (w == root_window)
			break;

		if (w->owner == client)
			unhide_window(lock|winlist, w);

		w = w->next;
	}

	app_in_front(lock, client);
}

void
hide_app(enum locks lock, struct xa_client *client)
{
	struct xa_client *focus = focus_owner();
	struct xa_window *w;

	DIAG((D_appl, NULL, "hide_app for %s", c_owner(client) ));
	DIAG((D_appl, NULL, "   focus is  %s", c_owner(focus) ));

	w = window_list;
	while (w)
	{
		if (w != root_window
		    && w->owner == client
		    && !is_hidden(w))
		{
			RECT r = w->r, d = root_window->r;
#if HIDE_TO_HEIGHT
			r.y += d.h;
#else
			if (r.x > 0)
				while (r.x < d.w)
					r.x += d.w;
			else
				while (r.x + r.w > 0)
					r.x -= d.w;
#endif
			if (w->send_message)
				w->send_message(lock|winlist, w, NULL,
						WM_MOVED, 0, 0, w->handle,
						r.x, r.y, r.w, r.h);
			else
				move_window(lock|winlist, w, -1, r.x, r.y, r.w, r.h);
		}
		w = w->next;
	}

	DIAG((D_appl, NULL, "   focus now %s", c_owner(focus)));

	if (client == focus)
		app_in_front(lock, next_app(lock, true));
}

void
hide_other(enum locks lock, struct xa_client *client)
{
	struct xa_client *list;

	list = S.client_list;
	while (list)
	{
		if (list != client)
			hide_app(lock, list);

		list = list->next;
	}
	app_in_front(lock, client);
}

void
unhide_all(enum locks lock, struct xa_client *client)
{
	struct xa_client *list;

	list = S.client_list;
	while (list)
	{
		unhide_app(lock, list);
		list = list->next;
	}
	app_in_front(lock, client);
}

bool
any_hidden(enum locks lock, struct xa_client *client)
{
	bool ret = false;
	struct xa_window *w;

	w = window_list;
	while (w)
	{
		if (w == root_window)
			break;

		if (w->owner == client && is_hidden(w))
		{
			ret = true;
			break;
		}

		w = w->next;
	}

	return ret;
}

static bool
any_window(enum locks lock, struct xa_client *client)
{
	struct xa_window *w;
	bool ret = false;

	w = window_list;
	while (w)
	{
		if (   w != root_window
		    && w->is_open
		    && w->r.w
		    && w->r.h
		    && w->owner == client)
		{
			ret = true;
			break;
		}
		w = w->next;
	}

	return ret;
}

struct xa_window *
next_wind(enum locks lock)
{
	struct xa_window *w, *wind;

	DIAG((D_appl, NULL, "next_window"));
	wind = window_list;

	if (wind)
		wind = wind->next;

	w = wind;

	if (wind)
	{
		while (wind->next)
			wind = wind->next;
	}

	while (wind)
	{
		if ( wind != root_window
		  && wind->is_open
		  && !is_hidden(wind)
		  && wind->r.w
		  && wind->r.h )
		{
			break;
		}
		wind = wind->prev;
	}
#if GENERATE_DIAGS
	if (wind)
		DIAG((D_appl, NULL, "  --  return %d, owner %s",
			wind->handle, wind->owner->name));
	else
		DIAG((D_appl, NULL, " -- no next window"));
#endif
	return wind;
}

/*
 * wwom true == find a client "with window or menu", else any client
 */
struct xa_client *
next_app(enum locks lock, bool wwom)
{
	struct xa_client *client;

	client = S.client_list;

	while (client->next)
		client = client->next;

	if (wwom)
	{
		while (client)
		{
			if (client->std_menu || any_window(lock, client))
				break;

			client = client->prior;
		}
	}
	else
	{	
		if (client == S.client_list)
			client = NULL;
	}
	return client;
}

#if 0
	fc = focus_owner();
	client = NULL;

	if (fc)
		client = fc->next;

	if (!client)
	{
		if (!(client = S.client_list))
			return NULL;
	}
	fc = client;
	do
	{
		bool anywin = any_window(lock, client);

		DIAG((D_appl, NULL, "anywin %d, menu %lx", anywin, client->std_menu->tree));

		if (client->std_menu || anywin)
		{
			DIAG((D_appl, NULL, "  --  return %s", c_owner(client)));
			return client;
		}
		if (!(client = client->next))
			client = S.client_list;

	} while (client != fc);

	DIAG((D_appl, NULL, "  --  fail"));
	return NULL;
}
#endif

struct xa_client *
previous_client(enum locks lock)
{
	return S.client_list->next;
}

void
app_in_front(enum locks lock, struct xa_client *client)
{
	struct xa_window *wl,*wf;

	if (client)
	{
		DIAG((D_appl, client, "app_in_front: %s", c_owner(client)));
			
		set_active_client(lock, client);

		swap_menu(lock, client, true, 1);

		wl = window_list;
		wf = NULL;

		while (wl)
		{
			if (wl->owner == client && wl != root_window)
			{
				if (wl->is_open)
				{
					if (!wf)
						wf = wl;
					if (is_hidden(wl))
						unhide_window(lock|winlist, wl);
				}
			}
			wl = wl->next;
		}
		if (wf)
			top_window(lock|winlist, wf, client);
	}
}

void
set_active_client(enum locks lock, struct xa_client *client)
{
	if (client != S.client_list)
	{
		if (client->prior)
			client->prior->next = client->next;
		if (client->next)
			client->next->prior = client->prior;
		if (S.client_list)
		{
			S.client_list->prior = client;
			client->next = S.client_list;
			client->prior = NULL;
			S.client_list = client;
		}
		else
		{
			S.client_list = client;
			client->next = client->prior = NULL;
		}
	}
}
