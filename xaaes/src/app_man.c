/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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
 * Application Manager
 * This module handles various ways of swapping the current application.
 */

#include <mint/mintbind.h>
#include <signal.h>

#include "xa_types.h"
#include "xa_global.h"

#include "app_man.h"
#include "xa_codes.h"
#include "c_window.h"
#include "desktop.h"
#include "objects.h"
#include "widgets.h"
#include "menuwidg.h"
#include "xa_rsrc.h"
#include "xa_clnt.h"


bool
taskbar(XA_CLIENT *client)
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

XA_CLIENT *
focus_owner(void)
{
	if (C.focus == root_window)
		return menu_owner();

	return C.focus->owner;
}


/*
 * Go round and check whether any clients have died & we didn't notice.
 * - Useful for cleaning up clients that weren't launched via a shell_write.
 */
void
find_dead_clients(LOCK lock)
{
	/* FIX_PC use of r eliminated; int?long? who wants to know? */
	/* HR: use proper loop. */
	XA_CLIENT *client = S.client_list;
	while (client)
	{
		XA_CLIENT *next = client->next; /* HR before FreeClient :-) */
		int pid = client->pid;

		if (pid > 0 && pid != C.AESpid)
		{
			/* We dont look at client_end, because it is
			 * possible that a app is started by shell_write,
			 * but never performed appl_init.
			 */
			if (Pkill(pid, SIGNULL))
			{
				/* Poll to see if the client has died since we last looked... */
				Pwaitpid(-1, 1, 0);

				DIAGS(("Dead client %s - cleaning up\n", c_owner(client)));

				if (client->init)
				{
					remove_refs(client, true);
					/* Run the application exit cleanup */
					XA_client_exit(lock, client, 0);
				}

				if (client->kernel_end)
					close_client(lock, client);
			}
		}

		client = next;
	}
}

/*
 * Attempt to recover a system that has locked up
 */
void
recover(void)
{	
	DIAG((D_appl, NULL, "Attempting to recover control.....\n"));

	if ((S.update_lock) && (S.update_lock != C.AESpid))
	{
		DIAG((D_appl, NULL, "Killing owner of update lock\n"));
		(void) Pkill(S.update_lock, SIGKILL);
	}
	
	if (((S.mouse_lock) && (S.mouse_lock != S.update_lock)) && (S.mouse_lock != C.AESpid))
	{
		DIAG((D_appl, NULL, "Killing owner of mouse lock\n"));
		(void) Pkill(S.mouse_lock, SIGKILL);
	}
	
	forcem();
}

/*
 * Swap the main root window's menu-bar to be another application's
 * NOTE: This only swaps the menu-bar, it doesn't swap the topped window.
 *
 * HR: static pid array.
 * See also click_menu_widget() for APP's
 */
void
swap_menu(LOCK lock, XA_CLIENT *new, bool do_desk, int which)
{
	XA_WINDOW *top;
	XA_TREE *menu_bar = get_menu();

	DIAG((D_appl, NULL, "[%d]swap_menu\n", which));

	/* If the new client has no menu bar, no need for a change */
	if (new->std_menu.tree)	
	{
		DIAG((D_appl, NULL, "  --   to %s\n", c_owner(new)));

		/* menu widget.tree */
		if (new->std_menu.tree != menu_bar->tree) /* Different menu? */
		{
			DIAG((D_appl, NULL, "swapped to %s\n",c_owner(new)));

			*menu_bar = new->std_menu;

			top = window_list;

			DIAG((D_appl, NULL, "top: %s\n", w_owner(top)));

			if (top != root_window && top->owner != new)
			{
				/* untop other pids windows */
				C.focus = root_window;
				DIAG((D_appl, NULL, "Focus to root_window.\n"));
				display_window(lock, 110, top, NULL);   /* Redisplay titles */
				send_untop(lock, top);
			}
			else if (C.focus == root_window && top->owner == new)
			{
				C.focus = top;
				DIAG((D_appl, NULL, "Focus to top_window %s\n", w_owner(top)));
				display_window(lock, 111, top, NULL);   /* Redisplay titles */
				send_ontop(lock);
			}
			else if (top->owner != new)
				C.focus = root_window;
		}
		else
		{
			DIAG((D_appl, NULL, "Same menu %s\n", c_owner(new)));
		}
	}

	/* Change desktops? HR 270801: now widget tree. */
	if (   do_desk
	    && new->desktop.tree
	    && new->desktop.tree != get_desktop()->tree
	    && new->desktop.tree != get_xa_desktop())
	{
		DIAG((D_appl, NULL, "  --   with desktop\n"));
		set_desktop(&new->desktop);
		display_window(lock, 30, root_window, NULL);
	}
	else if (new->std_menu.tree)
		/* No - just change menu bar */
		redraw_menu(lock);
}

XA_TREE *
find_menu_bar(LOCK lock)
{
	XA_TREE *rtn = &C.Aes->std_menu; /* default */
	XA_CLIENT *last;

	Sema_Up(clients);

	last = S.client_list;
	while(last->next)
		last = last->next;

	while (last)
	{
		if (!last->killed)
		{
			if (last->client_end && last->std_menu.tree)
			{
				rtn = &last->std_menu;
				DIAGS(("found std_menu %lx\n", rtn));
				break;
			}
		}
		last = last->prior;
	}

	Sema_Dn(clients);

	return rtn;
}

XA_CLIENT *
find_desktop(LOCK lock)
{
	XA_CLIENT *last, *rtn = C.Aes; /* default */

	Sema_Up(clients);

	last = S.client_list;
	while (last->next)
		last = last->next;

	while (last)
	{
		if (!last->killed)
		{
			if (last->client_end && last->desktop.tree)
			{
				rtn = last;
				DIAGS(("found desktop %lx\n", rtn));
				break;
			}
		}
		last = last->prior;
	}

	Sema_Dn(clients);
	return rtn;
}

void
unhide_app(LOCK lock, XA_CLIENT *client)
{
	XA_WINDOW *w;

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
hide_app(LOCK lock, XA_CLIENT *client)
{
	XA_WINDOW *w;
	XA_CLIENT *focus = focus_owner();

	DIAG((D_appl, NULL, "hide_app for %s\n", c_owner(client) ));
	DIAG((D_appl, NULL, "   focus is  %s\n", c_owner(focus_owner()) ));

	w = window_list;
	while (w)
	{
		if (   w != root_window
		    && w->owner == client
		    && !is_hidden(w) )
		//    && (w->active_widgets&MOVER) != 0	/* fail save */
		//    && (w->active_widgets&HIDE) != 0)	/* fail save */
		{
			RECT r = w->r, d = root_window->r;
#if HIDE_TO_HEIGHT
			/* HR: Dead simple, isnt it? ;-) */
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

	DIAG((D_appl, NULL, "   focus now %s\n", c_owner(focus_owner()) ));

	if (client == focus)
		app_in_front(lock, next_app(lock));
}

void
hide_other(LOCK lock, XA_CLIENT *client)
{
	XA_CLIENT *list;

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
unhide_all(LOCK lock, XA_CLIENT *client)
{
	XA_CLIENT *list;

	list = S.client_list;
	while (list)
	{
		unhide_app(lock, list);
		list = list->next;
	}
	app_in_front(lock, client);
}

bool
any_hidden(LOCK lock, XA_CLIENT *client)
{
	bool ret = false;
	XA_WINDOW *w;

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
any_window(LOCK lock, XA_CLIENT *client)
{
	XA_WINDOW *w;
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

XA_CLIENT *
next_app(LOCK lock)
{
	XA_CLIENT *foc;
	int pid, start_app;

	foc = focus_owner();
	if (!foc)
	{
		DIAGS(("No focus_owner()\n"));
		start_app = 1;
	}
	else
		start_app = foc->pid;

	pid = start_app + 1;
	DIAG((D_appl, NULL, "next_app: %d\n", start_app));
	do
	{
		XA_CLIENT *client;

		if (pid >= MAX_PID)
			pid = 0;

		client = S.Clients[pid];

		if (client)
		{
			DIAG((D_appl, NULL, "client_end %d, pid %d\n", client->client_end, pid));

			/* Valid client ? */
			if (client->client_end
			    || pid == C.AESpid)
			{
				bool anywin = any_window(lock, client);

				DIAG((D_appl, NULL, "anywin %d, menu %lx\n", anywin, client->std_menu.tree));

				if (client->std_menu.tree || anywin)
				{
					XA_CLIENT *ret = Pid2Client(pid);
					DIAG((D_appl, NULL, "  --  return %s\n", c_owner(client)));
					return ret;
				}
			}
			DIAG((D_appl, NULL, "  --  step --> %d\n",pid+1));
		}
		pid++;
	}
	while (pid != start_app);

	DIAG((D_appl, NULL, "  --  fail\n"));
	return NULL;
}

void
app_in_front(LOCK lock, XA_CLIENT *client)
{
	XA_WINDOW *wl,*pr,*wf;

	if (client)
	{
		DIAG((D_appl, client, "app_in_front: %s\n", c_owner(client)));

		swap_menu(lock, client, true, 1);

		wf = root_window->prev;
		while (wf)
		{
			if (wf->owner == client)
				break;
			else
				wf = wf->prev;
		}

		if (wf)
		{
			wl = wf;
			while (wl)
			{
				pr = wl->prev;
				if (wl->owner == client)
				{
					unhide_window(lock|winlist, wl);
					top_window(lock|winlist, wl, client);
				}
				wl = pr;
				if (wl == wf)
					break;
			}
		}
	}
}
