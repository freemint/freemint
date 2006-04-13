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

bool
wind_has_focus(struct xa_window *wind)
{
	struct xa_client *c;
	struct xa_window *w;

	c = find_focus(true, NULL, NULL, &w);

	return wind == w ? true : false;
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
		if (!(C.update_lock && (locked = get_update_locker())))
		{
			if (C.mouse_lock)
				locked = get_mouse_locker();
		}
		if (locked)
		{
			client = locked;
			if (locked_client)
				*locked_client = client;

			if (client->fmd.keypress ||				/* classic (blocked) form_do */
			/* XXX - Ozk:
			 * Find a better solution here! nolist windows that are created for popups (menu's and popups)
			 * should not be taken into account when client wants top_window via wind_get(WF_TOP)!
			 * But the AES may perhaps need to direct some keypresses to those later on..
			 */
			    (nlwind && nlwind->owner == client && !(nlwind->dial & created_for_POPUP)) ||
			    client->waiting_for & (MU_KEYBD | MU_NORM_KEYBD) ||
			    (top->owner == client && top->keypress))		/* Windowed form_do() */
			{
				if (waiting)
					*waiting = true;

				if (keywind)
				{
					if (nlwind && nlwind->owner == client && !(nlwind->dial & created_for_POPUP))
						*keywind = nlwind;
					else if (top->owner == client && top->keypress)
						*keywind = top;
				}
				DIAGS(("-= 3 =-"));
				return client;
			}
		}
	}	

	if (is_topped(top) && !is_hidden(top))
	{
		if (waiting && ((top->owner->waiting_for & (MU_KEYBD | MU_NORM_KEYBD)) || top->keypress))
			*waiting = true;

		if (keywind)
			*keywind = top;

		DIAGS(("-= 4 =-"));
		client = top->owner;
	}
	else
	{
		struct xa_client *c;

		c = get_app_infront();
		
		if (c->blocktype == XABT_NONE || (c->waiting_for & (MU_KEYBD | MU_NORM_KEYBD)))
		{
			client = c;
			if (c->blocktype != XABT_NONE && waiting)
				*waiting = true;
		}
		else if (c == C.Aes)
			client = c;
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
	struct proc *proc;
	struct proc *menu_lock = menustruct_locked();
	struct xa_client *client;

	DIAG((D_appl, NULL, "Attempting to recover control....."));

	swap_menu(0, C.Aes, NULL, SWAPM_DESK); //true, false, 0);

	if ((proc = C.update_lock))
	{
		client = proc2client(proc);

		DIAG((D_appl, NULL, "Killing owner of update lock"));
		free_update_lock();
		if (C.mouse_lock == proc)
		{
			free_mouse_lock();
			C.mouse_lock = NULL;
			C.mouselock_count = 0;
		}
		if (menu_lock == proc)
		{
			menu_lock = NULL;
			free_menustruct_lock();
		}
		C.update_lock = NULL;
		C.updatelock_count = 0;
		ikill(proc->pid, SIGKILL);
	}
	if ((proc = C.mouse_lock))
	{
		free_mouse_lock();
		if (menu_lock == proc)
		{
			menu_lock = NULL;
			free_menustruct_lock();
		}
		C.mouse_lock = NULL;
		C.mouselock_count = 0;
		ikill(proc->pid, SIGKILL);
	}
	if (menu_lock && (menu_lock != C.Aes->p))
	{
		free_menustruct_lock();
		ikill(menu_lock->pid, SIGKILL);
	}
	forcem();
}

void
set_next_menu(struct xa_client *new, bool do_topwind, bool force)
{
	if (new)
	{
		struct xa_client *infront = get_app_infront();
		enum locks lock = 0;
		struct xa_widget *widg = get_menu_widg();
		struct xa_window *top = NULL;

		if (new->nxt_menu)
		{
			new->std_menu = new->nxt_menu;
			new->nxt_menu = NULL;
		}
		
		if (force || (is_infront(new) || (!infront->std_menu && !infront->nxt_menu)))
		{
			if (new->std_menu)
			{
				XA_TREE *wt;
				bool wastop = false;

				DIAG((D_appl, NULL, "swapped to %s",c_owner(new)));
				
				if (new->std_menu != widg->stuff)
				{
					if (do_topwind && (top = window_list != root_window ? root_window : NULL))
						wastop = is_topped(top) ? true : false;
			
					if ((wt = widg->stuff))
					{
						wt->widg = NULL;
						wt->flags &= ~WTF_STATIC;
						wt->links--;
					}
					widg->stuff = wt = new->std_menu;
					wt->flags |= WTF_STATIC;
					wt->widg = widg;
					wt->links++;

					DIAG((D_appl, NULL, "top: %s", w_owner(top)));
			
					if (do_topwind && top)
					{
						if ((wastop && !is_topped(top)) || (!wastop && is_topped(top)))
						{
							if (wastop)
								setwin_untopped(lock, top, true);
							else
								setwin_ontop(lock, true);
				
							send_iredraw(lock, top, 0, NULL);
						}
					}
				}
				set_rootmenu_area(new);
			}
			redraw_menu(lock);
		}
	}
}
/*
 * Swap the main root window's menu-bar to be another application's
 * NOTE: This only swaps the menu-bar, it doesn't swap the topped window.
 *
 * See also click_menu_widget() for APP's
 */

void
swap_menu(enum locks lock, struct xa_client *new, struct widget_tree *new_menu, short flags)
{
	struct proc *p = get_curproc();

	DIAG((D_appl, NULL, "swap_menu: %s, flags=%lx", new->name, flags));

	/* If the new client has no menu bar, no need for a change */
	if (new->std_menu || new_menu || new->nxt_menu)
	{
		if (flags & SWAPM_REMOVE)
		{
			/* Ozk:
			 * Here we mean business!! If SWAPM_REMOVE bit is set,
			 * we make sure here that this clients menu is inaccessible
			 * and freeable when we leave.
			 */
			if (get_menu() == new->std_menu)
			{
				struct xa_client *nm_client;

				if (!new_menu || new_menu->owner == new)
					nm_client = find_menu(lock, new, 3);
				else
					nm_client = new_menu->owner;

				if (menustruct_locked() == new->p)
					popout(TAB_LIST_START);

				set_next_menu(nm_client, ((flags & SWAPM_TOPW) ? true : false), true);
			}

			if (C.next_menu == new)
				C.next_menu = NULL;

			if (new->std_menu)
				remove_wt(new->std_menu, false);
			if (new->nxt_menu && new->nxt_menu != new->std_menu)
				remove_wt(new->nxt_menu, false);
			new->std_menu = new->nxt_menu = NULL;
		}
		else if (lock_menustruct(p, true))
		{
			DIAG((D_appl, NULL, "swap_menu: now. std=%lx, new_menu=%lx, nxt_menu = %lx for %s",
				new->std_menu, new_menu, new->nxt_menu, new->name));
			
		//	display("swap_menu: now. std=%lx, new_menu=%lx, nxt_menu = %lx for %s",
		//		new->std_menu, new_menu, new->nxt_menu, new->name);
			
			if (new_menu)
				new->nxt_menu = new_menu;
			set_next_menu(new, ((flags & SWAPM_TOPW) ? true : false), false);
			unlock_menustruct(p);
		}
		else
		{
			/*
			 * If the menustructure is locked, we set C.next_menu to point
			 * to applications that should have its menu ontop. When the
			 * holder of menustruct lock is done, it checks this variable
			 * and does a "delayed menu swap"
			 */
			DIAG((D_appl, NULL, "swap_menu: later. std=%lx, new_menu=%lx, nxt_menu = %lx for %s",
				new->std_menu, new_menu, new->nxt_menu, new->name));
			
			if (new_menu)
				new->nxt_menu = new_menu;
			C.next_menu = new;
		}
	}

	/* Change desktops? */
	if (  (flags & SWAPM_DESK) /* do_desk */
	    && new->desktop
	    && new->desktop->tree != get_desktop()->tree
	    && new->desktop->tree != get_xa_desktop())
	{
		DIAG((D_appl, NULL, "  --   with desktop=%lx", new->desktop));
		set_desktop(new, false);
	}
	DIAG((D_appl, NULL, "exit ok"));
}

struct xa_client *
find_desktop(enum locks lock, struct xa_client *client, short exclude)
{
	struct xa_client *last, *rtn = C.Aes;

	Sema_Up(clients);

	last = APP_LIST_START;
	while (NEXT_APP(last))
		last = NEXT_APP(last);

	while (last)
	{
		if (last->desktop)
		{
			if ( !((exclude & 2) && (last == client)) &&
			     !((exclude & 1) && (last == C.Aes)) )
			{
				rtn = last;
				DIAGS(("found desktop %lx", rtn));
				break;
			}
		}

		last = PREV_APP(last);
	}

	if (!last)
		last = C.Aes;

	Sema_Dn(clients);
	return rtn;
}
/*
 * Guaranteed to return a client iwth menu
 */
struct xa_client *
find_menu(enum locks lock, struct xa_client *client, short exclude)
{
	struct xa_client *mclient = APP_LIST_START;

	while (mclient)
	{
		if (!(mclient->status & CS_EXITING) && mclient->std_menu)
		{
			if ( !((exclude & 1) && client == mclient)
			  && !((exclude & 2) && client == C.Aes))
			  	return mclient;
		}
		mclient = NEXT_APP(mclient);
	}
	return C.Aes;
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
			unhide_window(lock|winlist, w, false);

		w = w->next;
	}
	
	client->name[1] = ' ';
	
	app_in_front(lock, client, true, true, true);
}

static TIMEOUT *rpi_to = NULL;
static int rpi_block = 0;

static void
repos_iconified(struct proc *p, long arg)
{
	enum locks lock = (enum locks)arg;
	struct xa_window *w = window_list;
	RECT r;

	if (!rpi_block && !S.clients_exiting)
	{
		int i = 0;
		short cx, cy;
		struct xa_window *cw;
		
		while (1)
		{
			r = iconify_grid(i++);
			w = window_list;
			cw = NULL;
			cx = 32000;
			cy = 32000;

			while (w)
			{
				if ((w->window_status & (XAWS_OPEN|XAWS_ICONIFIED|XAWS_HIDDEN)) == (XAWS_OPEN|XAWS_ICONIFIED))
				{
					if (w->t.x == r.x && w->t.y == r.y && !(w->window_status & XAWS_SEMA))
					{
						w->window_status |= XAWS_SEMA;
						if (cw)
						{
							cw->window_status &= ~XAWS_SEMA;
							cw = NULL;
							cx = cy = 32000;
						}
						break;
					}
					else if (!(w->window_status & XAWS_SEMA) && (!cw || ((cfg.icnfy_orient & 2) ? abs((w->t.x - r.x)) <= cx : abs((w->t.y - r.y)) <= cy)))
					{
						if (cw)
							cw->window_status &= ~XAWS_SEMA;
						cx = abs(w->t.x - r.x);
						cy = abs(w->t.y - r.y);
						cw = w;
						cw->window_status |= XAWS_SEMA;
					}
				#if 0
					r = iconify_grid(i++);
					if (w->opts & XAWO_WCOWORK)
						r = f2w(&w->delta, &r, true);
					send_moved(lock, w, AMQ_NORM, &r);
					w->t = r;
				#endif
				}
				
				w = w->next;
			}

			if (cw)
			{
				if (cw->opts & XAWO_WCOWORK)
					r = f2w(&cw->delta, &r, true);
				send_moved(lock, cw, AMQ_NORM, &r);
				w->t = r;
			}
			
			if (!cw && !w)
				break;
		}
		
		w = window_list;
		while (w)
		{
			w->window_status &= ~XAWS_SEMA;
			w = w->next;
		}
	#if 0
		while (w)
		{
			if ((w->window_status & (XAWS_OPEN|XAWS_ICONIFIED|XAWS_HIDDEN)) == (XAWS_OPEN|XAWS_ICONIFIED))
			{
				r = free_icon_pos(lock, w);
				if (w->opts & XAWO_WCOWORK)
					r = f2w(&w->delta, &r, true);
				send_moved(lock, w, AMQ_NORM, &r);
				w->t = r;
			}
			w = w->next;
		}
	#endif
		rpi_to = NULL;
	}
	else
		rpi_to = addroottimeout(1000L, repos_iconified, arg);
}

void
set_reiconify_timeout(enum locks lock)
{
	if (!rpi_to && cfg.icnfy_reorder_to)
		rpi_to = addroottimeout(cfg.icnfy_reorder_to, repos_iconified, (long)lock);
}
void
cancel_reiconify_timeout(void)
{
	if (rpi_to)
	{
		cancelroottimeout(rpi_to);
		rpi_to = NULL;
	}
}
void
block_reiconify_timeout(void)
{
	rpi_block++;
}
void
unblock_reiconify_timeout(void)
{
	rpi_block--;
}

void
hide_app(enum locks lock, struct xa_client *client)
{
	bool reify = false, hidden = false;
	struct xa_client *infocus = focus_owner(), *nxtclient;
	struct xa_window *w;

	DIAG((D_appl, NULL, "hide_app for %s", c_owner(client) ));
	DIAG((D_appl, NULL, "   focus is  %s", c_owner(infocus) ));

	nxtclient = next_app(lock, true, true);
	if ((client->type & APP_SYSTEM) ||
	    (!nxtclient || nxtclient == client) ||
	    (client->swm_newmsg & NM_INHIBIT_HIDE))
		return;
	
	block_reiconify_timeout();

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
			hidden = true;
		}
		w = w->next;
	}

	if (hidden)
		client->name[1] = '*';
	
	DIAG((D_appl, NULL, "   focus now %s", c_owner(infocus)));
	
	if (client == infocus)
		app_in_front(lock, nxtclient, true, true, true);

	unblock_reiconify_timeout();

	if (reify)
		set_reiconify_timeout(lock);

// 	if (reify && !rpi_to)
// 		rpi_to = addroottimeout(1000L, repos_iconified, (long)lock);
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
	app_in_front(lock, client, true, true, true);
}

void
unhide_all(enum locks lock, struct xa_client *client)
{
	struct xa_client *cl;

	FOREACH_CLIENT(cl)
	{
		unhide_app(lock, cl);
	}

	app_in_front(lock, client, true, true, true);
}

void
set_unhidden(enum locks lock, struct xa_client *client)
{
	client->name[1] = ' ';
}

bool
any_hidden(enum locks lock, struct xa_client *client, struct xa_window *exclude)
{
	bool ret = false;
	struct xa_window *w;

	w = window_list;
	while (w)
	{
		if (w != exclude)
		{
			if (w == root_window)
				break;

			if (w->owner == client && is_hidden(w))
			{
				ret = true;
				break;
			}
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
get_topwind(enum locks lock, struct xa_client *client, struct xa_window *startw, bool not, short wsmsk, short wschk)
{
	struct xa_window *w = startw;

	while (w)
	{
		if (w != root_window &&
		    (w->window_status & wsmsk) == wschk)
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
	
	if (!wind || (wind == root_window || wind->next == root_window))
		return NULL;

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
		  && ((wind->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN)
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
 * will also return clients without window or menu but is listening
 * for kbd input (MU_KEYBD)
 */
struct xa_client *
next_app(enum locks lock, bool wwom, bool no_acc)
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
			if (!(client->type & APP_SYSTEM) || !(no_acc && (client->type & APP_ACCESSORY)))
			{
				if (client->std_menu || client->nxt_menu ||
				    client->waiting_for & (MU_KEYBD | MU_NORM_KEYBD) ||
				    any_window(lock, client))
					break;
			}
			client = PREV_APP(client);
		}
	}
	else
	{
		while (client)
		{
			if (!((no_acc && (client->type & APP_ACCESSORY)) || (client->type & APP_SYSTEM)))
				break;
			client = PREV_APP(client);
		}
				
		if (client == APP_LIST_START)
			client = NULL;
	}

	DIAGS(("next_app: return %s", client->name));
	return client;
}

/*
 * If bit 0 in exlude set, only return AES if thats the only
 * process left
 */
struct xa_client *
previous_client(enum locks lock, short exlude)
{
	struct xa_client *client = APP_LIST_START;

	if (client)
		client = NEXT_APP(client);
	
	if (client == C.Aes && (exlude & 1))
	{
		if (!(client = NEXT_APP(client)))
			client = C.Aes;
	}

	return client;
}

void
app_in_front(enum locks lock, struct xa_client *client, bool snd_untopped, bool snd_ontop, bool allwinds)
{
	struct xa_window *wl,*wf,*wp;

	if (client)
	{
		bool was_hidden = false;
		struct xa_client *infront;
		struct xa_window *topped = NULL, *wastop;
		
		DIAG((D_appl, client, "app_in_front: %s", c_owner(client)));

		if (window_list != root_window)
			wastop = is_topped(window_list) ? window_list : NULL;
		else
			wastop = NULL;
		
		infront = get_app_infront();
		if (infront != client)
		{
			set_active_client(lock, client);
			//swap_menu(lock, client, NULL, SWAPM_DESK); //true, false, 1);
		}

		if (client->std_menu != get_menu() || client->nxt_menu)
			swap_menu(lock, client, NULL, SWAPM_DESK);

		if (allwinds)
		{
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
						{
							unhide_window(lock|winlist, wl, false);
							was_hidden = true;
						}

						wi_remove(&S.open_windows, wl);
						wi_put_first(&S.open_windows, wl);
						topped = wl;
					}
				}

				if (wl == wf)
					break;
			
				wl = wp;
			}
		}
		else
		{
			if (!(topped = get_topwind(lock, client, window_list, false, (XAWS_OPEN|XAWS_HIDDEN), XAWS_OPEN)))
			{
				topped = get_topwind(lock, client, window_list, false, XAWS_OPEN, XAWS_OPEN);
				if (topped && is_hidden(topped))
					unhide_window(lock, topped, false);
			}
			if (topped && topped != window_list)
			{
				wi_remove(&S.open_windows, topped);
				wi_put_first(&S.open_windows, topped);
			}
		}
		
		if (was_hidden)
			set_unhidden(lock, client);

		if (topped)
		{
			update_all_windows(lock, window_list);
			set_winmouse(-1, -1);
		}

		if (wastop)
		{
			if (!is_topped(wastop))
			{
				setwin_untopped(lock, wastop, snd_untopped);
				send_iredraw(lock, wastop, 0, NULL);
			}
			if (wastop != window_list && window_list != root_window && is_topped(window_list))
			{
				setwin_ontop(lock, snd_ontop);
				send_iredraw(lock, window_list, 0, NULL);
			}
		}
		else if (window_list != root_window && is_topped(window_list))
		{
			setwin_ontop(lock, snd_ontop);
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

struct xa_client *
get_app_by_procname(char *name)
{
	struct xa_client *client;

	FOREACH_CLIENT(client)
	{
		if (!strnicmp(client->proc_name, name, 8))
			break;
	}
	return client;
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
