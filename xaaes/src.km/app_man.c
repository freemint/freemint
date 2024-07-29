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
 * Application Manager
 * This module handles various ways of swapping the current application.
 */

#include "app_man.h"
#include "xa_appl.h"
#include "xa_bubble.h"
#include "xa_global.h"
#include "xa_rsrc.h"

#include "semaphores.h"
#include "c_window.h"
#include "desktop.h"
#include "menuwidg.h"
#include "obtree.h"
#include "messages.h"
#include "taskman.h"
#include "widgets.h"

#include "k_mouse.h"

#include "mint/signal.h"

static struct xa_client *	find_menu(int lock, struct xa_client *client, short exclude);

#if INCLUDE_UNUSED
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
#endif

struct xa_client *
focus_owner(void)
{
	return find_focus(true, NULL, NULL, NULL);
}

bool
wind_has_focus(struct xa_window *wind)
{
	struct xa_window *w;

	find_focus(true, NULL, NULL, &w);

	return wind == w ? true : false;
}

struct xa_client *
reset_focus(struct xa_window **new_focus, short flags)
{
	struct xa_client *client = NULL;
	struct xa_window *top = TOP_WINDOW, *fw = NULL;

	if (S.focus && (S.focus->window_status & XAWS_STICKYFOCUS))
		fw = S.focus;

	DIAGA(("reset_focus: topwin is %d of %s, current focus is %d of %s",
		top ? top->handle : -2, top ? top->owner->name : "NoWind",
		S.focus ? S.focus->handle : -2, S.focus ? S.focus->owner->name : "Nowind"));

	if (!fw && 0)
	{
		short x, y;

		check_mouse(NULL, NULL, &x, &y);
		fw = find_window(0, x, y, FNDW_NOLIST|FNDW_NORMAL);
		if (fw && fw != root_window && !(fw->window_status & XAWS_NOFOCUS))
		{
			client = fw->owner;
		}
		else
			fw = NULL;
	}

	if (!fw)
	{
		if (!(flags & 1) && S.focus)
		{
			if (S.focus->nolist) {
				DIAGA(("reset_focus: Current focus is nolist, keep it focused!"));
				fw = S.focus;
			}
		}
		if (!fw && top && !(top->window_status & XAWS_NOFOCUS) && is_topped(top) && !is_hidden(top))
		{
			fw = top;
			DIAGA(("reset_focus: Suggest current TOP to get new focus"));
		}
		else if (!fw)
		{
			struct xa_client *topcl = get_app_infront();

			DIAGA(("reset_focus: look for window of currently topped app...."));
			fw = window_list;
			while (fw) {
				if (fw == root_window) {
					if (get_desktop()->owner != topcl) {
						DIAGA(("reset_focus: Only root_wind present, app ontop have no wind- no focused"));
						fw = NULL;
					} else
					{
						DIAGA(("reset_focus: Suggest root_window to get focus"));
					}
					break;
				} else if (fw->owner == topcl && !(fw->window_status & XAWS_NOFOCUS)) {
					DIAGA(("reset_focus: suggest %d of %s as new focus", fw->handle, fw->owner->name));
					break;
				}

				fw = fw->next;
			}
			if (!fw)
			{
				DIAGA(("reset_focus: app ontop have no winds, find window closest to top..."));
				fw = window_list;
				while (fw)
				{
					if (!(fw->window_status & XAWS_NOFOCUS)) {
						DIAGA(("reset_focus: suggest %d of %s as new focus", fw->handle, fw->owner->name));
						break;
					}
					if (fw == root_window)
					{
						fw = NULL;
						break;
					}
					fw = fw->next;
				}
			}
		}
	}

	if (fw)
		client = fw == root_window ? get_desktop()->owner : fw->owner;
	if (new_focus)
		*new_focus = fw;

	return client;
}

void
setnew_focus(struct xa_window *wind, struct xa_window *unfocus, bool topowner, bool snd_untopped, bool snd_ontop)
{
	DIAGA(("setnew_focus: to %d of %s, unfocus %d of %s",
		wind ? wind->handle : -2, wind ? wind->owner->name : "nowind",
		unfocus ? unfocus->handle : -2, unfocus ? unfocus->owner->name : "nowind"));


	if( C.boot_focus && wind && wind->owner->p != C.boot_focus )
		return;
	if (!unfocus || unfocus == S.focus)
	{
		struct xa_client *owner;	//, *p_owner = 0;

		if (S.focus)
		{
			if( (S.focus->window_status & XAWS_STICKYFOCUS))
				return;
			//p_owner = S.focus->owner;
		}

		if (wind)
		{
			if (!(wind->window_status & XAWS_OPEN))
				return;
			if ((wind->window_status & XAWS_NOFOCUS))
			{
				if (wind->colours != wind->untop_cols || S.focus == wind)
				{
					wind->colours = wind->untop_cols;
					send_iredraw(0, wind, 0, NULL);

					DIAGA(("setnew_focus: wind %d of %s is NOFOCUS, dont give focus!",
						wind->handle, wind->owner->name));
					if (S.focus == wind) {
						DIAGA(("setnew_focus: taking NOFOCUS window out of focus!"));
						S.focus = NULL;
					} else if (S.focus)
					{
						DIAGA(("setnew_focus: Keep focus on %d of %s", S.focus->handle, S.focus->owner->name));
					} else {
						DIAGA(("setnew_focus: Keeping no focused window state"));
					}
				}
				return;
			}
			else {
				owner = wind == root_window ? get_desktop()->owner : wind->owner;
			}
		}
		else
			owner = NULL;
#if 0
		if (topowner && wind && (!S.focus || (owner != S.focus->owner && !is_infront(owner))))
		{
			set_active_client(0, owner);
			swap_menu(0, owner, NULL, SWAPM_DESK);
		}
#endif
		if (wind) {
			if (topowner && (!S.focus || (owner != S.focus->owner && !is_infront(owner)))) {
				set_active_client(0, owner);
				swap_menu(0, owner, NULL, SWAPM_DESK);
			}
			if (wind != S.focus) {

				if (get_app_infront() == owner) {
					setwin_ontop(0, wind, snd_ontop);

					wind->colours = wind->ontop_cols;
					if( wind->tool )
					{
						XA_WIDGET *widg = wind->tool;
						XA_TREE *wt = widg->stuff.wt;
						if( wt )
						{
							if( wind->owner->p == get_curproc() && !(wind->dial & created_for_FMD_START) )
							{
								ob_set_wind( wt->tree, G_SLIST, WM_ONTOP );
							}
						}
					}
					send_iredraw(0, wind, 0, NULL);

				} else
					wind = NULL;
			} else
				wind = NULL;
		}

		if (S.focus) {
			if (wind && wind != S.focus) {
				setwin_untopped(0, S.focus, snd_untopped);
				S.focus->colours = S.focus->untop_cols;
				send_iredraw(0, S.focus, 0, NULL);
				DIAGA(("setnew_focus: Changing focus from %d of %s to %d of %s",
					S.focus->handle, S.focus->owner->name, wind->handle, wind->owner->name));
				S.focus = wind;
			} else if (!wind) {
				setwin_untopped(0, S.focus, snd_untopped);
				S.focus->colours = S.focus->untop_cols;
				send_iredraw(0, S.focus, 0, NULL);
				S.focus = NULL;
			}
		} else {
			S.focus = wind;
			DIAGA(("setnew_focus: Set focus to %d of %s", wind ? wind->handle : -2, wind ? wind->owner->name : "NoWind"));
		}
		DIAGA(("setnew_focus: to %d of '%s', was %d of '%s'",
			(long)wind ? wind->handle : -2, (long)wind ? wind->owner->proc_name : "None",
			(long)S.focus ? S.focus->handle : -2, (long)S.focus ? S.focus->owner->proc_name : "None"));
	}
}

void
unset_focus(struct xa_window *wind)
{
	if (!wind)
		return;

	DIAGA(("unset_focus: on %d of %s which %s focus",
		wind->handle, wind->owner->name, wind == S.focus ? "have":"dont have"));

	if (wind == S.focus)
	{
		struct xa_window *fw;

		S.focus = NULL;
		reset_focus(&fw, 0);
		setnew_focus(fw, NULL, true, true, true);
		wind->colours = wind->untop_cols;
		send_iredraw(0, wind, 0, NULL);
	}
	DIAGA(("unset_focus: !"));
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
	struct xa_window *top = TOP_WINDOW, *nlwind = nolist_list;
	struct xa_client *client = NULL, *locked = NULL;

	DIAGA(("find_focus: topwind = %d of %s, withlocks=%s",
		top ? top->handle : -2, top ? top->owner->name : "NoWind", withlocks ? "yes":"no"));

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
		DIAGA(("find_focus: locked = %s", locked ? locked->name : "Nolocker"));
		if (locked)
		{
			client = locked;
			if (locked_client)
				*locked_client = client;

			if (client->fmd.keypress ||				/* classic (blocked) form_do */
			/* XXX - Ozk:
			 * Find a better solution here! nolist windows that are created for popups (menu's and popups)
			 * should not be taken into account when client wants TOP_WINDOW via wind_get(WF_TOP)!
			 * But the AES may perhaps need to direct some keypresses to those later on..
			 */
				(nlwind && nlwind->owner == client && !(nlwind->dial & created_for_POPUP)) ||
				(client->waiting_for & (MU_KEYBD | MU_NORM_KEYBD)) ||
				(top->owner == client && top->keypress) ||		/* Windowed form_do() */
				(S.focus && S.focus->owner == client))
			{
				if (waiting)
					*waiting = true;

				if (keywind)
				{
					if (nlwind && nlwind->owner == client && !(nlwind->dial & created_for_POPUP))
						*keywind = nlwind;
					else if (top->owner == client && top->keypress)
						*keywind = top;
					else if (S.focus && S.focus->owner == client)
						*keywind = S.focus;
					if (*keywind) {
						DIAGA(("find_focus: keywind handle %d, type %s",
							(*keywind)->handle, (*keywind)->nolist ? "nolist":"normal"));
					} else {
						DIAGA(("find_focus: no window with focus"));
					}
				}
				DIAGA(("-= 3 =-"));
				return client;
			}
		}
	}

	if ((top = S.focus) && !is_hidden(top))
	{
		if (waiting && ((top->owner->waiting_for & (MU_KEYBD | MU_NORM_KEYBD)) || top->keypress))
			*waiting = true;

		if (keywind)
			*keywind = top;

		DIAGA(("-= 4 =-"));
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
	#if 0
		else
		{
			client = menu_owner();
			if (client->blocktype != XABT_NONE && waiting)
				*waiting = true;
		}
	#endif
	}

	DIAGA(("find_focus: focus = %s, infront = %s", client->name, APP_LIST_START->name));

	return client;
}

#if ALT_CTRL_APP_OPS
/*
 * Attempt to recover a system that has locked up
 */
void
recover(void)
{
	struct proc *proc;
	struct proc *menu_lock = menustruct_locked();

	DIAG((D_appl, NULL, "Attempting to recover control....."));

	swap_menu(0, C.Aes, NULL, SWAPM_DESK); //true, false, 0);

	if ((proc = C.update_lock))
	{
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
#endif

void
set_next_menu(struct xa_client *new, bool do_topwind, bool force)
{
	if (new)
	{
		struct xa_client *infront = get_app_infront();
		int lock = 0;
		struct xa_widget *widg = get_menu_widg();
		struct xa_window *top = NULL;

		/* in single-mode display only menu of single-app */
		if( C.SingleTaskPid > 0	&& !(new->p->pid == C.SingleTaskPid) )
			return;
		if (new->nxt_menu)
		{
			new->std_menu = new->nxt_menu;
			new->nxt_menu = NULL;
		}

		set_standard_point(new);

		if (force || (is_infront(new) || (!infront->std_menu && !infront->nxt_menu)))
		{
			if (new->std_menu)
			{
				XA_TREE *wt;
				bool wastop = false;

				DIAG((D_appl, NULL, "swapped to %s",c_owner(new)));

				if (new->std_menu != widg->stuff.wt)
				{
					if (do_topwind && (top = TOP_WINDOW != root_window ? root_window : NULL))
						wastop = is_topped(top) ? true : false;

					if ((wt = widg->stuff.wt) != NULL)
					{
						wt->widg = NULL;
						wt->flags &= ~WTF_STATIC;
						wt->links--;
					}
					widg->stuff.wt = wt = new->std_menu;
					wt->flags |= WTF_STATIC;
					wt->widg = widg;
					wt->links++;

					DIAG((D_appl, NULL, "top: %s", w_owner(top)));

					if (do_topwind && top)
					{
						if ((wastop && !is_topped(top)) || (!wastop && is_topped(top)))
						{
							setnew_focus(TOP_WINDOW, NULL, false, true, true);
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
swap_menu(int lock, struct xa_client *new, struct widget_tree *new_menu, short flags)
{
	struct proc *p = get_curproc();

	DIAG((D_appl, NULL, "swap_menu: %s, flags=%x", new->name, flags));

	/* in single-mode display only menu of single-app */
	if( (C.boot_focus && new->p != C.boot_focus) || (C.SingleTaskPid > 0 && new->p->pid != C.SingleTaskPid) )
		return;

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
			struct proc *update_lock = C.update_lock;
			DIAG((D_appl, NULL, "swap_menu: now. std=%lx, new_menu=%lx, nxt_menu = %lx for %s",
				(unsigned long)new->std_menu, (unsigned long)new_menu, (unsigned long)new->nxt_menu, new->name));

			if (new_menu)
				new->nxt_menu = new_menu;
			C.update_lock = 0;	/* allow main-menu to get redrawn */
			set_next_menu(new, ((flags & SWAPM_TOPW) ? true : false), false);
			unlock_menustruct(p);
			C.update_lock = update_lock;
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
				(unsigned long)new->std_menu, (unsigned long)new_menu, (unsigned long)new->nxt_menu, new->name));

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
		DIAG((D_appl, NULL, "  --   with desktop=%lx", (unsigned long)new->desktop));
		set_desktop(new, false);
	}
	DIAG((D_appl, NULL, "exit ok"));
}

struct xa_client *
find_desktop(int lock, struct xa_client *client, short exclude)
{
	struct xa_client *last, *rtn = C.Aes;

	Sema_Up(LOCK_CLIENTS);

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
				DIAGS(("found desktop %lx", (unsigned long)rtn));
				break;
			}
		}

		last = PREV_APP(last);
	}

	if (!last)
		last = C.Aes;

	Sema_Dn(LOCK_CLIENTS);
	return rtn;
}
/*
 * Guaranteed to return a client iwth menu
 */
struct xa_client *
find_menu(int lock, struct xa_client *client, short exclude)
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
unhide_app(int lock, struct xa_client *client)
{
	struct xa_window *w;

	w = window_list;
	while (w)
	{
		if (w == root_window)
			break;

		if (w->owner == client)
			unhide_window(lock|LOCK_WINLIST, w, false);

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
	int lock = (int)arg;
	struct xa_window *w = window_list;
	GRECT ir, r;

	if (!rpi_block && !S.clients_exiting)
	{
		int i = 0;
		short cx, cy;
		struct xa_window *cw;

		while (1)
		{
			ir = iconify_grid(i++);
			w = window_list;
			cw = NULL;
			cx = 32000;
			cy = 32000;

			while (w && w != root_window)
			{
				if ((w->window_status & (XAWS_OPEN|XAWS_ICONIFIED|XAWS_HIDDEN|XAWS_CHGICONIF)) == (XAWS_OPEN|XAWS_ICONIFIED))
				{
					if (w->opts & XAWO_WCOWORK)
						r = f2w(&w->save_delta, &ir, true);
					else
						r = ir;

					if (w->t.g_x == r.g_x && w->t.g_y == r.g_y && !(w->window_status & XAWS_SEMA))
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
					else if (!(w->window_status & XAWS_SEMA) && (!cw || ((cfg.icnfy_orient & 2) ? abs((w->t.g_x - r.g_x)) <= cx : abs((w->t.g_y - r.g_y)) <= cy)))
					{
						if (cw)
							cw->window_status &= ~XAWS_SEMA;
						cx = abs(w->t.g_x - r.g_x);
						cy = abs(w->t.g_y - r.g_y);
						cw = w;
						cw->window_status |= XAWS_SEMA;
					}
				}
				w = w->next;
			}

			if (cw)
			{
				if (cw->opts & XAWO_WCOWORK)
					r = f2w(&cw->delta, &ir, true);
				else
				{
					if( (cfg.icnfy_orient & 0xff) == 3 )
					{
						r.g_x = (( r.g_x + cfg.icnfy_w / 2 ) / cfg.icnfy_w) * cfg.icnfy_w;
						r.g_y = cw->r.g_y;
						r.g_w = cw->r.g_w;
						r.g_h = cw->r.g_h;
					}
					/* else we loose .. */
				}

				send_moved(lock, cw, AMQ_NORM, &r);
				w->t = r;
			}

			if (!cw && (!w || w == root_window))
				break;
		}

		w = window_list;
		while (w)
		{
			w->window_status &= ~XAWS_SEMA;
			w = w->next;
		}
		rpi_to = NULL;
	}
	else
		rpi_to = addroottimeout(1000L, repos_iconified, arg);
}

void
set_reiconify_timeout(int lock)
{
	if (rpi_to)
	{
		cancelroottimeout(rpi_to);
		rpi_to = NULL;
	}
	if (cfg.icnfy_reorder_to)
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
static void
block_reiconify_timeout(void)
{
	rpi_block++;
}
static void
unblock_reiconify_timeout(void)
{
	rpi_block--;
}

void
hide_app(int lock, struct xa_client *client)
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
			if (is_iconified(w))
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
}

void
hide_other(int lock, struct xa_client *client)
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
unhide_all(int lock, struct xa_client *client)
{
	struct xa_client *cl;

	FOREACH_CLIENT(cl)
	{
		unhide_app(lock, cl);
	}

	app_in_front(lock, client, true, true, true);
}

void
set_unhidden(int lock, struct xa_client *client)
{
	client->name[1] = ' ';
}

bool
any_hidden(int lock, struct xa_client *client, struct xa_window *exclude)
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
any_window(int lock, struct xa_client *client)
{
	struct xa_window *w;
	bool ret = false;

	w = window_list;
	while (w)
	{
		if (   w != root_window
		    && (w->window_status & XAWS_OPEN)
		    && w->r.g_w
		    && w->r.g_h
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
get_topwind(int lock, struct xa_client *client, struct xa_window *startw, bool not, WINDOW_STATUS wsmsk, WINDOW_STATUS wschk)
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

#if ALT_CTRL_APP_OPS
struct xa_window *
next_wind(int lock)
{
	struct xa_window *wind;

	DIAG((D_appl, NULL, "next_window"));
	wind = window_list;

	if (!wind || (wind == root_window || wind->next == root_window))
		return NULL;

	if (wind)
	{
		if( wind != S.focus && (wind->window_status & XAWS_FLOAT) )	// works if 1 XAWS_FLOAT
		{
			return wind;
		}
		wind = wind->next;
	}

	if (wind)
	{
		while (wind->next)
		{
			//if( (wind->window_status & (XAWS_FLOAT|XAWS_FIRST)) == XAWS_FLOAT )
				//return wind;
			wind = wind->next;
		}
	}

	while (wind)
	{
		if ( S.focus != wind && wind != root_window
		  && ((wind->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN )
		  && wind->r.g_w
		  && wind->r.g_h )
		{
			break;
		}
		wind = wind->prev;
	}

	if (wind)
	{
		DIAG((D_appl, NULL, "  --  return %d, owner %s",
			wind->handle, wind->owner->name));
	} else
	{
		DIAG((D_appl, NULL, " -- no next window"));
	}
	return wind;
}
#endif
/*
 * wwom true == find a client "with window or menu", else any client
 * will also return clients without window or menu but is listening
 * for kbd input (MU_KEYBD)
 */
struct xa_client *
next_app(int lock, bool wwom, bool no_acc)
{
	struct xa_client *client;

	DIAGS(("next_app"));
	client = APP_LIST_START;

	if (client)
	{
		while (NEXT_APP(client))
		{
			client = NEXT_APP(client);
		}
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
previous_client(int lock, short exlude)
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
app_in_front(int lock, struct xa_client *client, bool snd_untopped, bool snd_ontop, bool allwinds)
{
	struct xa_window *wl,*wf,*wp;

	if (client)
	{
		bool was_hidden = false, upd = false;
		struct xa_client *infront;
		struct xa_window *topped = NULL;

		if( C.SingleTaskPid > 0 && client->p->pid != C.SingleTaskPid )
			return;

		wakeup_client(client);
		DIAG((D_appl, client, "app_in_front: %s", c_owner(client)));

		infront = get_app_infront();
		if (infront != client){
			set_active_client(lock, client);
		}
		if (client->std_menu != get_menu() || client->nxt_menu)
		{
			swap_menu(lock, client, NULL, SWAPM_DESK);
		}

		if (allwinds)
		{
			wl = root_window->prev;
			wf = window_list;

			while (wl)
			{
				wp = wl->prev;

				if (wl->owner == client && wl != root_window && !(wl->window_status & XAWS_SEMA))
				{
					wl->window_status |= XAWS_SEMA;
					if ((wl->window_status & XAWS_OPEN))
					{
						if (is_hidden(wl))
						{
							unhide_window(lock|LOCK_WINLIST, wl, false);
							was_hidden = true;
						}

						wi_move_first(&S.open_windows, wl);
						upd = true;
						if (!(wl->window_status & XAWS_NOFOCUS))
							topped = wl;
					}
				}
				if (wl == wf)
					break;
				wl = wp;
			}
			wl = window_list;
			while (wl)
			{
				wl->window_status &= ~XAWS_SEMA;
				wl = wl->next;
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
			if (topped && topped != TOP_WINDOW)
			{
				wi_move_first(&S.open_windows, topped);
				upd = true;
			}
		}

		if (was_hidden)
			set_unhidden(lock, client);

		if (topped || upd)
		{
			update_all_windows(lock, window_list);
			set_winmouse(-1, -1);
		}
		if( topped == S.focus )
			S.focus = 0;	/* force focus to new top */
		setnew_focus(topped, S.focus, false, true, true);
#if WITH_BBL_HELP
		if( (!C.boot_focus || client->p == C.boot_focus) )
		{
			if( cfg.menu_bar == 0 && !topped )
				display_launched( lock, client->name );
			else
				xa_bubble( 0, bbl_close_bubble1, 0, 0 );
		}
#endif
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

#if ALT_CTRL_APP_OPS
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
#endif
void
set_active_client(int lock, struct xa_client *client)
{
	if (client != APP_LIST_START)
	{
		hide_toolboxwindows(APP_LIST_START);
		show_toolboxwindows(client);
		APP_LIST_REMOVE(client);
		APP_LIST_INSERT_START(client);
	}
}
