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
#include "messages.h"
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
	return find_focus(true, NULL, NULL, NULL);
#if 0
	if (C.focus == root_window)
		return menu_owner();

	if (C.focus)
		return C.focus->owner;

	DIAGS(("No focus_owner()???"));
	return NULL;
#endif
}

/*
 * HR: Generalization of focus determination.
 *     Each step checks MU_KEYBD except the first.
 *     The top or focus window can have a keypress handler
 *     instead of the XAWAIT_KEY flag.
 *
 *       first:  check focus keypress handler (no MU_KEYBD or update_lock needed)
 *       second: check update lock
 *       last:   check top or focus window
 *
 *  240401: Interesting bug found and killed:
 *       If the update lock is set, then the key must go to that client,
 *          If that client is not yet waiting, the key must be queued,
 *          the routine MUST pass the client pointer, so there is a pid to be
 *          checked later.
 *       In other words: There can always be a client returned. So we must only know
 *          if that client is already waiting. Hence the ref bool.
 */
struct xa_client *
find_focus(bool withlocks, bool *waiting, struct xa_client **locked_client, struct xa_window **keywind)
{
	struct xa_window *top = window_list;
	struct xa_client *client = NULL, *locked = NULL;

	if (waiting)
		*waiting = false;

	if (keywind)
		*keywind = NULL;

	if (locked_client)
		*locked_client = NULL;

	if (withlocks)
	{
		if (update_locked())
		{
			locked = update_locked();
			DIAGS(("-= 1 =-"));
		}
		else if (mouse_locked())
		{
			locked = mouse_locked();
			DIAGS(("-= 2 =-"));
		}
		if (locked)
		{
			client = locked;
			if (locked_client)
				*locked_client = client;

			if (client->fmd.keypress ||				/* classic (blocked) form_do */
			    client->waiting_for & (MU_KEYBD | MU_NORM_KEYBD) ||
			    (top->owner == client && top->keypress))		/* Windowed form_do() */
			{
				if (waiting)
					*waiting = true;

				if (keywind)
				{
					if (client->fmd.lock && client->fmd.keypress)
						*keywind = client->fmd.wind;
					else if (top->owner == client && top->keypress)
						*keywind = top;
				}
				
				DIAGS(("-= 3 =-"));
				return client;
			}
		}
	}	

	if (is_topped(top))
	{
		if (waiting && ((top->owner->waiting_for & (MU_KEYBD | MU_NORM_KEYBD)) || top->keypress))
			*waiting = true;

		if (keywind)
			*keywind = top;

		DIAGS(("-= 4 =-"));
		client = top->owner;
	}
	else if (get_app_infront()->waiting_for & (MU_KEYBD | MU_NORM_KEYBD))
	{
		DIAGS(("-= 5 =-"));

		if (waiting)
			*waiting = true;

		client = get_app_infront();
	}

	DIAGS(("find_focus: focus = %s, infront = %s", client->name, APP_LIST_START->name));

	return client;
}

/*
 * Attempt to recover a system that has locked up
 */
void
recover(void)
{
	struct xa_client *update_lock = update_locked();
	struct xa_client *mouse_lock = mouse_locked();
	struct xa_client *menu_lock = menustruct_locked();

	DIAG((D_appl, NULL, "Attempting to recover control....."));

	if (update_lock && (update_lock != C.Aes))
	{
		DIAG((D_appl, NULL, "Killing owner of update lock"));
		free_update_lock();
		if (mouse_lock == update_lock)
		{
			mouse_lock = NULL;
			free_mouse_lock();
		}
		if (menu_lock == update_lock)
		{
			menu_lock = NULL;
			free_menustruct_lock();
		}
		ikill(update_lock->p->pid, SIGKILL);
	}
	if (mouse_lock && (mouse_lock != C.Aes))
	{
		free_mouse_lock();
		if (menu_lock == mouse_lock)
		{
			menu_lock = NULL;
			free_menustruct_lock();
		}
		ikill(mouse_lock->p->pid, SIGKILL);
	}
	if (menu_lock && (menu_lock != C.Aes))
	{
		free_menustruct_lock();
		ikill(menu_lock->p->pid, SIGKILL);
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

	last = APP_LIST_START;
	while (NEXT_APP(last))
		last = NEXT_APP(last);

	while (last)
	{
		if (last->std_menu)
		{
			rtn = last->std_menu; //&last->std_menu;
			DIAGS(("found std_menu %lx", rtn));
			break;
		}

		last = PREV_APP(last);
	}

	Sema_Dn(clients);

	return rtn;
}

struct xa_client *
find_desktop(enum locks lock)
{
	struct xa_client *last, *rtn = C.Aes; /* default */

	Sema_Up(clients);

	last = APP_LIST_START;
	while (NEXT_APP(last))
		last = NEXT_APP(last);

	while (last)
	{
		if (last->desktop)
		{
			rtn = last;
			DIAGS(("found desktop %lx", rtn));
			break;
		}

		last = PREV_APP(last);
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

static TIMEOUT *rpi_to = NULL;

static void
repos_iconified(struct proc *p, long arg)
{
	enum locks lock = (enum locks)arg;
	struct xa_window *w = window_list;
	RECT r;

	while (w)
	{
		if ((w->window_status & XAWS_ICONIFIED) && !is_hidden(w))
		{
			r = free_icon_pos(lock, w);
			send_moved(lock, w, AMQ_NORM, &r);
			w->t = r;
		}
		w = w->next;
	}
	rpi_to = NULL;
}

void
hide_app(enum locks lock, struct xa_client *client)
{
	bool reify = false;
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
			RECT r = w->rc, d = root_window->rc;

			if ((w->window_status & XAWS_ICONIFIED))
				reify = true;

			hide_window(lock, w);
#if 0
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
			send_moved(lock|winlist, w, AMQ_NORM, &r);
#endif
		}
		w = w->next;
	}

	DIAG((D_appl, NULL, "   focus now %s", c_owner(focus)));

	if (client == focus)
		app_in_front(lock, next_app(lock, true));

	if (reify && !rpi_to)
		rpi_to = addroottimeout(1000L, repos_iconified, (long)lock);
}

void
hide_other(enum locks lock, struct xa_client *client)
{
	struct xa_client *cl;

	FOREACH_CLIENT(cl)
	{
		if (cl != client)
			hide_app(lock, cl);
	}
	app_in_front(lock, client);
}

void
unhide_all(enum locks lock, struct xa_client *client)
{
	struct xa_client *cl;

	FOREACH_CLIENT(cl)
	{
		unhide_app(lock, cl);
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
		    && (w->window_status & XAWS_OPEN)
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
		  && (wind->window_status & XAWS_OPEN)
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

bool
app_is_hidable(struct xa_client *client)
{
	if (client->std_menu ||
	    client->waiting_for & (MU_KEYBD | MU_NORM_KEYBD) ||
	    any_window(winlist, client))
	    	return true;
	else
		return false;
}

/*
 * wwom true == find a client "with window or menu", else any client
 * will also return clients without window or menu but is listening
 * for kbd input (MU_KEYBD)
 */
struct xa_client *
next_app(enum locks lock, bool wwom)
{
	struct xa_client *client;

	DIAGS(("next_app"));
	client = APP_LIST_START;

	if (client)
	{
		while (NEXT_APP(client))
			client = NEXT_APP(client);
	}

	if (wwom)
	{
		while (client)
		{
			if (client->std_menu ||
			    client->waiting_for & (MU_KEYBD | MU_NORM_KEYBD) ||
			    any_window(lock, client))
				break;

			client = PREV_APP(client);
		}
	}
	else
	{	
		if (client == APP_LIST_START)
			client = NULL;
	}

	DIAGS(("next_app: return %s", client->name));
	return client;
}

struct xa_client *
previous_client(enum locks lock)
{
	struct xa_client *client = APP_LIST_START;

	if (client)
		client = NEXT_APP(client);

	return client;
}

void
app_in_front(enum locks lock, struct xa_client *client)
{
	struct xa_window *wl,*wf,*wp;

	if (client)
	{
		struct xa_client *infront;
		DIAG((D_appl, client, "app_in_front: %s", c_owner(client)));
			
		infront = get_app_infront();
		set_active_client(lock, client);
		swap_menu(lock, client, true, 1);

		wl = root_window->prev;
		wf = window_list;

		while (wl)
		{
			wp = wl->prev;

			if (wl->owner == client && wl != root_window)
			{
				if ((wl->window_status & XAWS_OPEN))
				{
					if (is_hidden(wl))
						unhide_window(lock|winlist, wl);

					top_window(lock|winlist, wl, NULL);
				}
			}

			if (wl == wf)
				break;
			
			wl = wp;
		}

		if (wf != window_list)
		{
			send_untop(lock, wf);
			send_ontop(lock);
		}
		else
		{
			if (!is_topped(wf))
			{
				send_untop(lock, wf);
				display_window(lock, 0, wf, NULL);
			}
			else if (infront != client)
			{
				send_ontop(lock);
				display_window(lock, 0, wf, NULL);
			}
		}
	}
}

bool
is_infront(struct xa_client *client)
{
	return (client == APP_LIST_START ? true : false);
}

struct xa_client *
get_app_infront(void)
{
	return APP_LIST_START;
}

void
set_active_client(enum locks lock, struct xa_client *client)
{
	if (client != APP_LIST_START)
	{
		APP_LIST_REMOVE(client);
		APP_LIST_INSERT_START(client);
	}
}
