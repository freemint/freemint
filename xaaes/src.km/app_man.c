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
	struct xa_window *top = window_list, *nlwind = nolist_list;
	struct xa_client *client = NULL, *locked = NULL;

	if (waiting)
		*waiting = false;

	if (keywind)
		*keywind = NULL;

	if (locked_client)
		*locked_client = NULL;

	if (withlocks)
	{
		if (C.update_lock)
		{
			locked = C.update_lock;
			DIAGS(("-= 1 =-"));
		}
		else if (C.mouse_lock)
		{
			locked = C.mouse_lock;
			DIAGS(("-= 2 =-"));
		}
		if (locked)
		{
			client = locked;
			if (locked_client)
				*locked_client = client;

			if (client->fmd.keypress ||				/* classic (blocked) form_do */
			    (nlwind && nlwind->owner == client) ||
			    client->waiting_for & (MU_KEYBD | MU_NORM_KEYBD) ||
			    (top->owner == client && top->keypress))		/* Windowed form_do() */
			{
				if (waiting)
					*waiting = true;

				if (keywind)
				{
					if (nlwind && nlwind->owner == client)
						*keywind = nlwind;
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
	struct xa_client *client;
	struct xa_client *menu_lock = menustruct_locked();

	DIAG((D_appl, NULL, "Attempting to recover control....."));

	swap_menu(0, C.Aes, true, false, 0);

	if ((client = C.update_lock))
	{
		DIAG((D_appl, NULL, "Killing owner of update lock"));
		free_update_lock();
		if (C.mouse_lock == client)
		{
			free_mouse_lock();
			C.mouse_lock = NULL;
			C.mouselock_count = 0;
		}
		if (menu_lock == client)
		{
			menu_lock = NULL;
			free_menustruct_lock();
		}
		C.update_lock = NULL;
		C.updatelock_count = 0;
		ikill(client->p->pid, SIGKILL);
	}
	if ((client = C.mouse_lock))
	{
		free_mouse_lock();
		if (menu_lock == client)
		{
			menu_lock = NULL;
			free_menustruct_lock();
		}
		C.mouse_lock = NULL;
		C.mouselock_count = 0;
		ikill(client->p->pid, SIGKILL);
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
swap_menu(enum locks lock, struct xa_client *new, bool do_desk, bool do_topwind, int which)
{
	//struct xa_window *top = NULL;
	//struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);
	//XA_WIDGET *widg = get_menu_widg();

	DIAG((D_appl, NULL, "[%d]swap_menu", which));

	/* If the new client has no menu bar, no need for a change */
	if (new->std_menu)
	{
	
		DIAG((D_appl, NULL, "  --   to %s", c_owner(new)));
		set_rootmenu(new, do_topwind);
#if 0
		/* menu widget.tree */
		if (new->std_menu != widg->stuff) /* Different menu? */
		{
			XA_TREE *wt;
			bool wastop = false;

			DIAG((D_appl, NULL, "swapped to %s",c_owner(new)));

			if (!rc)
				rc = C.Aes;

			if (do_topwind && (top = window_list != root_window ? root_window : NULL))
				wastop = is_topped(top) ? true : false;
			
			new->status |= CS_WAIT_MENU;
			lock_menustruct(rc, false);
			if ((wt = widg->stuff))
			{
				wt->widg = NULL;
				wt->flags &= ~WTF_STATIC;
			}
			widg->stuff = wt = new->std_menu;
			wt->flags |= WTF_STATIC;
			wt->widg = widg;
			unlock_menustruct(rc);
			new->status &= ~CS_WAIT_MENU;

			DIAG((D_appl, NULL, "top: %s", w_owner(top)));

			if (do_topwind && top)
			{
				if ((wastop && !is_topped(top)) || (!wastop && is_topped(top)))
				{
					if (wastop)
						send_untop(lock, top);
					else
						send_ontop(lock);
				
					send_iredraw(lock, top, 0, NULL);
					//display_window(lock, 110, top, NULL);
				}
			}
		}
		else
		{
			DIAG((D_appl, NULL, "Same menu %s", c_owner(new)));
		}
#endif
	}

	/* Change desktops? */
	if (   do_desk
	    && new->desktop
	    && new->desktop->tree != get_desktop()->tree
	    && new->desktop->tree != get_xa_desktop())
	{
		DIAG((D_appl, NULL, "  --   with desktop=%lx", new->desktop));
		set_desktop(new->desktop);
		//redraw_menu(lock);
	}
#if 0
	else if (new->std_menu)
	{
		/* No - just change menu bar */
		DIAG((D_appl, NULL, "redrawing menu..."));
		set_rootmenu(new->desktop);
		redraw_menu(lock);
	}
#endif
	DIAG((D_appl, NULL, "exit ok"));
}

XA_TREE *
find_menu_bar(enum locks lock)
{
	XA_TREE *rtn = C.Aes->std_menu;  /* default */
	struct xa_client *last;

	Sema_Up(clients);

	last = APP_LIST_START;
	while (NEXT_APP(last))
		last = NEXT_APP(last);

	while (last)
	{
		if (last->std_menu)
		{
			rtn = last->std_menu;
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
			if ((w->window_status & XAWS_ICONIFIED))
				reify = true;

			hide_window(lock, w);
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
get_topwind(enum locks lock, struct xa_client *client, bool not, struct xa_window *startw)
{
	struct xa_window *w = startw;

	while (w)
	{
		if (w != root_window &&
		    (w->window_status & XAWS_OPEN) &&
		    !is_hidden(w))
		{
			if (client)
			{
				if (!not)
				{
					if (w->owner == client)
						break;
				}
				else if (w->owner != client)
					break;
			}
			else
				break;
		}
		w = w->next;
	}
	return w;
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
		struct xa_window *topped = NULL, *wastop;
		DIAG((D_appl, client, "app_in_front: %s", c_owner(client)));

		if (window_list != root_window)
			wastop = is_topped(window_list) ? window_list : NULL;
		else
			wastop = NULL;
		
		infront = get_app_infront();
		set_active_client(lock, client);
		swap_menu(lock, client, true, false, 1);

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

					wi_remove(&S.open_windows, wl);
					wi_put_first(&S.open_windows, wl);
					topped = wl;
				}
			}

			if (wl == wf)
				break;
			
			wl = wp;
		}

		if (topped)
		{
			update_all_windows(lock, window_list);
			set_winmouse();
		}

		if (wastop)
		{
			if (!is_topped(wastop))
			{
				send_untop(lock, wastop);
				send_iredraw(lock, wastop, 0, NULL);
			}
			if (wastop != window_list && window_list != root_window && is_topped(window_list))
			{
				send_ontop(lock);
				send_iredraw(lock, window_list, 0, NULL);
			}
		}
		else if (window_list != root_window && is_topped(window_list))
		{
			send_ontop(lock);
			send_iredraw(lock, window_list, 0, NULL);
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
