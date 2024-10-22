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

#include "k_mouse.h"
#include "k_keybd.h"
#include "xa_global.h"
#include "c_mouse.h"
#include "xa_appl.h"
#include "xa_graf.h"
#include "xa_evnt.h"

#include "app_man.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "desktop.h"
#include "init.h"
#include "k_init.h"
#include "k_main.h"
#include "k_shutdown.h"
#include "messages.h"
#include "menuwidg.h"
#include "nkcc.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#if WITH_BBL_HELP
#include "xa_bubble.h"
#endif
#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/signal.h"
/*
 * Ozk: When a mouse button event is delivered to a client, or application,
 * the whole button event is copied into the clients private moose_data.
 * This means that if the delivered button event has any of its buttons
 * pressed, we need to remember which client is to get the button released
 * data.
 */
//static struct xa_client *button_waiter = 0;

/*
 * Ozk: The data in mainmd contains the data from the last button-change
 * and NOT the up-to-date mouse coordinates. Those are in x_mouse and y_mouse
 */
static short x_mouse;
static short y_mouse;
struct moose_data mainmd;	// ozk: Have to take the most recent moose packet into global space

#define MDBUF_SIZE 32

/*
 * This moose_data buffer is used when delivering exlusive
 * mouse input.
 */
static int md_buff_head = 0;
static int md_buff_tail = 0;
static int md_lmvalid = 0;
static struct moose_data md_lastmove;
static struct moose_data md_buff[MDBUF_SIZE + 1];

/*
 * This is the queue of incoming moose_data packets.
 * packets coming from moose.adi is stored here immediately
 */
static struct moose_data mdb[];
static struct moose_data *md_head = mdb;
static struct moose_data *md_tail = mdb;
static struct moose_data *md_end = mdb + MDBUF_SIZE;
static struct moose_data mdb[MDBUF_SIZE + 1];

/*
 * Mouse button click handler
 * - MOUSESRV server process passes us click events
 *   No, it's the Moose device driver via the kernel nowadays.
 *
 * The real button click handler is here :)
 * Excerpt from nkcc.doc, courtesy Harald Siegmund:
 *
 * Note: the NKCC button event handler supports the (undocumented)
 * negation flag, which is passed in bit 8 of the parameter <bclicks>
 * (maximum # of mouse clicks to wait for). You don't know this flag?
 * I found an article about it in the c't magazine (I think it was
 * issue 3/90, or maybe 4/90??) - and I damned Atari for their bad
 * documentation. This flag opens the way to check BOTH mouse buttons
 * at the same time without any problems. When set, the return
 * condition is inverted. Let's have a look at an example:
 *
 * mask = evnt_multi(MU_BUTTON,2,3,3,...
 *
 * This doesn't work the way we want: the return condition is
 * "button #0 pressed AND button #1 pressed". But look at this:
 *
 * mask = evnt_multi(MU_BUTTON,0x102,3,0,...
 *
 * Now the condition is "NOT (button #0 released AND button #1
 * released)". Or in other words: "button #0 pressed OR button #1
 * pressed". Nice, isn't it?!
 *
 */

/*
 * Ozk: new_mu_mouse() is now only called by adi_button(). After I
 * get things cleaned up a bit, the mu_button structure will disappear
 * and the latest mouse-button event packet will be stored in mainmd.
 * Then all functions depending on the most uptodate mouse-button event
 * status (notably check_mouse()) will use mainmd instead of mu_button.
 */
static void
new_mu_mouse(struct moose_data *md)
{
	/*
	 * Copy into the global main moose data if necessary
	 */
	if (md != &mainmd)
		mainmd = *md;
}

static void
new_active_widget_mouse(struct moose_data *md)
{
	widget_active.m	= *md;
}

static void
modify_md(struct moose_data *md)
{
	if (cfg.lrmb_swap)
	{
		short ns = 0, ncs = 0;

		if (md->state & MBS_RIGHT)
			ns |= MBS_LEFT;
		if (md->state & MBS_LEFT)
			ns |= MBS_RIGHT;

		if (md->cstate & MBS_RIGHT)
			ncs |= MBS_LEFT;
		if (md->cstate & MBS_LEFT)
			ncs |= MBS_RIGHT;

		md->state = ns, md->cstate = ncs;
	}
}

static bool
add_md(struct moose_data *md)
{
	struct moose_data *n = md_tail;

	n++;
	if (n > md_end)
		n = mdb;
	if (n == md_head)
		return false;

	*md_tail = *md;
	md_tail = n;

	return true;
}

static bool
get_md(struct moose_data *r)
{
	if (md_head != md_tail)
	{
		*r = *md_head;
		md_head++;
		if (md_head > md_end)
			md_head = mdb;
		return true;
	}
	return false;
}

static bool
pending_md(void)
{
	return md_head == md_tail ? false : true;
}

/*
 * Ozk: Store a moose packet in our queue
 * We dont collec more than one MOVEMENT packet, the most recent one.
 */
static void
buffer_moose_pkt(struct moose_data *md)
{
	if (md->ty == MOOSE_BUTTON_PREFIX)
	{
		md_buff[md_buff_head++] = *md;
		md_buff_head &= MDBUF_SIZE - 1;
	}
	else if (md->ty == MOOSE_MOVEMENT_PREFIX)
	{
		md_lmvalid = 1;
		md_lastmove = *md;
	}
}

/*
 * Ozk: Read a packet out of our queue. Button event packets are
 * prioritized.
 */
static int
unbuffer_moose_pkt(struct moose_data *md)
{
	if (md_buff_head == md_buff_tail)
	{
		if (md_lmvalid)
		{
			*md = md_lastmove;
			md_lmvalid = 0;
			return 1;
		}
		return 0;
	}
	*md = md_buff[md_buff_tail++];
	md_buff_tail &= MDBUF_SIZE - 1;
	return 1;
}

bool
is_bevent(int gotbut, int gotcl, const short *o, int which)
{
	bool ev;
	int clks = o[0];
	int msk = o[1];
	int st = o[2];
	int clicks = clks & 0xff;

	if (clks & 0x100)
		ev = (gotbut & msk) != (st & msk);	/* HR 181201: apply mask on requested state as well. :-> */
	else
		ev = (gotbut & msk) == (st & msk);

	ev = ev && (gotcl <= clicks || (gotcl == 1 && clicks == 0));

	DIAG((D_button,NULL,"[%d]is_bevent? %s; gotb %d; gotc %d; clks 0x%x, msk %d, st %d",
		which, ev ? "Yes" : "No", gotbut, gotcl, clks, msk, st));

	return ev;
}

static bool
add_client_md(struct xa_client *client, const struct moose_data *md)
{
	struct moose_data *mdt = client->md_tail;

	mdt++;
	if (mdt > client->md_end)
		mdt = client->mdb;
	if (mdt == client->md_head)
		return false;

	*mdt = *md;
	client->md_tail = mdt;
	return true;
}

/* HR 050402: WDIAL: split off as a function for use in ExitForm function */

void
button_event(int lock, struct xa_client *client, const struct moose_data *md)
{
	bool exiting = client->status & CS_EXITING ? true : false;

	DIAG((D_button, NULL, "button_event: for %s, exiting? %s", c_owner(client), exiting ? "Yes" : "No"));
	DIAG((D_button, NULL, "button_event: md: clicks=%d, head=%lx, tail=%lx, end=%lx",
		client->md_head->clicks, (unsigned long)client->md_head, (unsigned long)client->md_tail, (unsigned long)client->md_end));

	if (!exiting)
	{
		add_client_md(client, md);

		/*
		 * If this is a click-hold event, moose.adi will send a
		 * released event later, and this event belongs to the
		 * receiver of the click-hold.
		 */
		/*
		 * If buttons not released when this packet delivered,
		 * we need to route all packets with any buttons down to
		 * the 'event owner'
		 */
		if (md->state && md->cstate) {
			DIAG((D_button, NULL, "set button_waiter: %s", client->name));
			C.button_waiter = client;
		}
	}

	DIAG((D_button, NULL, "button_event: md: clicks=%d, head=%lx, tail=%lx, end=%lx !",
		client->md_head->clicks, (unsigned long)client->md_head, (unsigned long)client->md_tail, (unsigned long)client->md_end));
}
/*
 * When a client is delivered a button event, we store the event
 * in the clients private moose_data structure. This is the _only_
 * data evnt_multi() and evnt_button() work on, and is only supposed
 * to change when the event is actually delivered to the client and
 * not used by the AES for menu or window gadgets.
*/
static void
deliver_button_event(struct xa_window *wind, struct xa_client *target, const struct moose_data *md)
{
	if (wind && wind->owner != target)
	{
		DIAG((D_mouse, target, "deliver_button_event: Send cXA_button_event (rootwind) to %s", target->name));
		post_cevent(target, cXA_button_event, wind,NULL, 0,0, NULL,md);
	}
	else
	{
		/*
		 * And post a "deliver this button event" client event
		 */
		DIAG((D_mouse, target, "deliver_button_event: Send cXA_deliver_button_event to %s", target->name));
		post_cevent(target, cXA_deliver_button_event, wind,NULL, 0,0, NULL,md);
	}
	mainmd.clicks = 0;
}

static void
dispatch_button_event(int lock, struct xa_window *wind, const struct moose_data *md)
{
	struct xa_client *target = wind->owner;

	/*
	 * Right-button clicks or clicks on no-list windows or topped windows...
	 */
	if ((md->state & MBS_RIGHT) || wind->nolist || is_topped(wind) || (wind->active_widgets & NO_TOPPED))
	{
		/*
		 * Check if click on any windows gadgets, which is AES's task to handle
		 */
		if (checkif_do_widgets(lock, wind, md->x, md->y, NULL))
		{
			DIAG((D_mouse, target, "dispatch_button_event: Send cXA_do_widgets to %s", target->name));
			post_cevent(target, cXA_do_widgets, wind,NULL, 0,0, NULL,md);
		}
		else /* Just deliver the event ... */
			deliver_button_event(wind, target, md);
	}
	else if (!is_topped(wind) && checkif_do_widgets(lock, wind, md->x, md->y, NULL))
	{
		DIAG((D_mouse, target, "dispatch_button_event: Send cXA_do_widgets (untopped widgets) to %s", target->name));
		post_cevent(target, cXA_do_widgets, wind,NULL, 0,0, NULL,md);
	}
	else if (wind->send_message && md->state)
	{
		DIAG((D_mouse, target, "dispatch_button_event: Sending WM_TOPPED to %s", target->name));
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP, WM_TOPPED, 0, 0, wind->handle, 0,0,0,0);
	}
}

/*
 * at the moment widgets is always true.
 */
static void
XA_button_event(int lock, const struct moose_data *md, bool widgets)
{
	struct xa_client *client, *locker, *mw_owner;
	struct xa_window *wind = NULL, *mouse_wind;
	bool chlp_lock = update_locked() == C.Hlp->p;

	DIAGA(("XA_button_event: %d/%d, state=0x%x, clicks=%d",
		md->x, md->y, md->state, md->clicks));

	/* Ozk 040503: Detect a button-released situation, and let active-widget get inactive */

	/*
	 * If menu-task (navigating in a menu) in progress and button
	 * pressed..
	 */

	if (!chlp_lock)
	{
		if (TAB_LIST_START)
		{
			if ((TAB_LIST_START->root->exit_mb && !md->state) || (!TAB_LIST_START->root->exit_mb && md->state))
			{
				client = TAB_LIST_START->client;
				if (!C.ce_menu_click && !(client->status & CS_EXITING))
				{
					C.ce_menu_click = client;
					DIAGA(("XA_button_event: post button event (menu) to %s", client->name));
					post_cevent(client, cXA_button_event, NULL,NULL, 0, 0, NULL, md);
				}
			}
			return;
		}

		/*
		 * If button released and widget_active is set (live movements)...
		 */
		if (widget_active.widg && !md->state)
		{
			widget_active.m = *md;
			client = widget_active.wind->owner;
			if (!(client->status & CS_EXITING))
			{
				DIAGA(("XA_button_event: post active widget (move) to %s", client->name));
				post_cevent(client, cXA_active_widget, NULL,NULL, 0,0, NULL, md);
			}
			return;
		}
	}

	if( cfg.menu_bar != 2 && md->y < 2 )
	{
		short mb = cfg.menu_bar;
		if( md->x > 8 && mb > 0 )
			return;	// dont switch off
		toggle_menu( lock, -1 );
		if( md->x > 8 && !mb )
			cfg.menu_bar = -1;

		return;
	}
	mouse_wind = find_window(lock, md->x, md->y, FNDW_NOLIST|FNDW_NORMAL);
	if (mouse_wind)
		mw_owner = mouse_wind == root_window ? get_desktop()->owner : mouse_wind->owner;
	else
		mw_owner = NULL;

	if ( C.mouse_lock && (locker = get_mouse_locker()))
	{
		DIAGA(("XA_button_event: mouse locked by %s", locker->name));

		if ((wind = nolist_list) && wind->owner == locker)
		{
			dispatch_button_event(lock, wind, md);
			return;
		}
		else if (locker->fmd.mousepress)
		{
			DIAGA(("XA_button_event: post form_do to %s", locker->name));
			post_cevent(locker, cXA_form_do, NULL,NULL, 0, 0, NULL, md);
			return;
		}
		/* not sure if the if 0 is correct but it helps at least taskbar with taskmanager-window below ... */
#if 0
		else if (mouse_wind && mw_owner && (mw_owner->status & CS_NO_SCRNLOCK))
		{
			dispatch_button_event(lock, mouse_wind, md);
			return;
		}
#endif
		/*
		 * Unconditionally deliver the button event to the mouse-lock holder..
		 */
		deliver_button_event(NULL, locker, md);
		return;
	}
	if ( C.update_lock && (locker = get_update_locker()))
	{
		DIAGA(("XA_button_event: screen locked by %s", locker->name));

		if ((wind = nolist_list) && wind->owner == locker)
		{
			dispatch_button_event(lock, wind, md);
			return;
		}
		else if (locker->fmd.mousepress)
		{
			DIAGA(("XA_button_event: post form_do to %s", locker->name));
			post_cevent(locker, cXA_form_do, NULL,NULL, 0, 0, NULL, md);
			return;
		}
		else if (mouse_wind && mw_owner && (mw_owner->status & CS_NO_SCRNLOCK))
		{
			dispatch_button_event(lock, mouse_wind, md);
			return;
		}
	}

	if (mouse_wind && !mouse_wind->nolist && (md->state & MBS_RIGHT) && (md->kstate & (K_CTRL|K_ALT)) == (K_CTRL|K_ALT) )
	{
		if (!(mw_owner->status & CS_EXITING))
			post_cevent(C.Hlp, CE_winctxt, mouse_wind, NULL, 0,0, NULL, md);
		return;
	}

// 	locker = C.mouse_lock ? get_mouse_locker() : NULL;

	/*
	 * check for rootwindow widgets, like the menu-bar, clicks
	 */
	if (md->state && (mouse_wind == root_window || mouse_wind == menu_window) )
	{
		struct xa_widget *widg;

		checkif_do_widgets(lock, mouse_wind, md->x, md->y, &widg);
		if (mouse_wind == menu_window || (widg && widg->m.r.xaw_idx == XAW_MENU) )
		{
			XA_TREE *menu;

			if (C.aesmouse != -1)
				xa_graf_mouse(-1, NULL, NULL, true);

			if( !widg )
				widg = get_menu_widg();
			menu = widg->stuff.wt;
			client = menu->owner;
			if (!(client->status & CS_EXITING))
			{
				DIAGA(("XA_button_event: post widgclick (menustart) to %s", client->name));
				C.ce_open_menu = client;
				post_cevent(client, cXA_open_menu, widg, menu, 0,0, NULL,md);
			}
			return;
		}
	}

	locker = C.mouse_lock ? get_mouse_locker() : NULL;
	/*
	 * Found a window under mouse, and no mouse lock
	 */
	if (mouse_wind && !locker)
	{
		client = mw_owner; //wind == root_window ? get_desktop()->owner : wind->owner;
		/*
		 * Ozk: If root window owner and desktop owner is not the same, the Desktop
		 * should get the clicks made on it. If no desktop is installed, XaAES
		 * XaAES is also the desktop owner (I think)
		 */
		if (mouse_wind->owner != client)
		{
			/*
			 * Ozk: When root-window is clicked, and owned by someone other
			 * than AESSYS, we to let both do_widget(), and possibly send the
			 * click to owner.
			*/
			deliver_button_event(mouse_wind, client, md);
		}
		else
		{
			dispatch_button_event(lock, mouse_wind, md);
		}

		return;
	}
	else if (locker)
	{
		if (mouse_wind)
			client = mw_owner; //wind == root_window ? get_desktop()->owner : wind->owner;
		else
			client = NULL;

		if ((mouse_wind && client && (client->status & CS_NO_SCRNLOCK)) ||
		    (mouse_wind && client == locker && (mouse_wind->active_widgets & TOOLBAR)) )
		{
			dispatch_button_event(lock, mouse_wind, md);
		}
		else
		{
			button_event(lock, locker, md);
			Unblock(locker, 1);
		}
	}
}

static void
dispatch_mu_event(struct xa_client *client, const struct moose_data *md, bool is_locker)
{
	short events;

	if ((events = checkfor_mumx_evnt(client, is_locker, md->x, md->y)))
		post_cevent(client, cXA_deliver_rect_event, (void *)client->status, NULL, events, 0, NULL,NULL);
}

static int
XA_move_event(int lock, const struct moose_data *md)
{
	struct xa_client *client;
	bool chlp_lock = update_locked() == C.Hlp->p;

	DIAG((D_mouse, NULL, "XA_move_event: menulocker = %s, ce_open_menu = %s, tablist = %lx, actwidg = %lx",
		menustruct_locked() ? menustruct_locked()->name : "NONE",
		C.ce_open_menu ? C.ce_open_menu->name : "NONE",
		(unsigned long)TAB_LIST_START, (unsigned long)widget_active.widg));


	/* Ozk 040503: Moved the continuing handling of widgets actions here
	 * so we dont have to msg the client to make real-time stuff
	 * work. Having it here saves time, since it only needs to be
	 * done when the mouse moves.
	 */
	if (!chlp_lock)
	{
		if (widget_active.widg)
		{
			widget_active.m = *md;
			client = widget_active.wind->owner;
			if (!(client->status & CS_EXITING))
			{
				DIAG((D_mouse, client, "post active widget (move) to %s", client->name));
				C.move_block = 1;
				post_cevent(client, cXA_active_widget, NULL,NULL, 0,0, NULL, md);
			}
			return false;
		}
		if (TAB_LIST_START)
		{
			client = TAB_LIST_START->client;

			if (!(client->status & CS_EXITING) && !C.ce_menu_move && !C.ce_menu_click)
			{
				client = TAB_LIST_START->client;
				C.ce_menu_move = client;

				DIAG((D_mouse, client, "post menumove to %s", client->name));
				post_cevent(client, cXA_menu_move, NULL,NULL, 0,0, NULL, md);
			}
			return false;
		}

// 		Sema_Up(LOCK_CLIENTS);
#if 0
		{
			struct xa_window *fw;
			reset_focus(&fw);
			setnew_focus(fw, 0);
		}
#endif
		/* Moving the mouse into the menu bar is outside
		 * Tab handling, because the bar itself is not for popping.
		 */
		if (cfg.menu_behave != PUSH && !menustruct_locked() && !C.ce_open_menu)
		{
			/* HR: watch the menu bar as a whole */
			struct xa_client *aesp = C.Aes;

			DIAG((D_mouse, NULL, "xa_move_event: aes wating for %lx", aesp->waiting_for));

			if ( (aesp->waiting_for & XAWAIT_MENU) && (aesp->em.flags & MU_M1))
			{
				if (/*   cfg.menu_behave != PUSH && */
				    !update_locked() &&
				     is_rect(md->x, md->y, aesp->em.flags & 1, &aesp->em.m1))
				{
					XA_WIDGET *widg = get_widget(root_window, XAW_MENU);
					XA_TREE *menu;

					if (C.aesmouse != -1)
						xa_graf_mouse(-1, NULL, NULL, true);

					menu = widg->stuff.wt;
					client = menu->owner;
					if (!(client->status & CS_EXITING))
					{
						DIAG((D_mouse, client, "post widgclick (menustart) to %s", client->name));
						C.ce_open_menu = client;
						post_cevent(client, cXA_open_menu, widg, menu, 0,0, NULL,md);
					}
					return false;
				}
			}
		}
	}

	client = C.mouse_lock ? get_mouse_locker() : NULL;
	if (client)
	{
		if (client->waiting_for & (MU_M1|MU_M2|MU_MX))
			dispatch_mu_event(client, md, true);
	}
	else if (!chlp_lock)
	{
		struct xa_window *wind = find_window(0, md->x, md->y, FNDW_NOLIST|FNDW_NORMAL);

		wind_mshape(wind, md->x, md->y);

		client = CLIENT_LIST_START;

		/* internalized the client loop */
		while (client)
		{
			dispatch_mu_event(client, md, false);
			client = NEXT_CLIENT(client);
		}
	}
// 	Sema_Dn(LOCK_CLIENTS);

	return false;
}


static void
XA_wheel_event(int lock, const struct moose_data *md)
{
	struct xa_window *wind;
	struct xa_client *client = NULL, *locker = NULL;

	DIAGS(("mouse wheel %d has wheeled %d (x=%d, y=%d)", md->state, md->clicks, md->x, md->y));

	toggle_menu(lock, -2);
	/*
	 * Ozk: For now we send wheel events to the owner of the window underneath the mouse
	 * in all cases except when owner of mouse_lock() is waiting for MU_WHEEL.
	 */

	if ( C.mouse_lock && (locker = get_mouse_locker()))
	{
		DIAG((D_mouse, locker, "XA_button_event - mouse locked by %s", locker->name));

		if ((wind = nolist_list) && wind->owner == locker)
		{
			post_cevent(locker, cXA_wheel_event, wind, NULL, 0, 0, NULL, md);
			return;
		}
		else if (locker->fmd.mousepress)
		{
		//	DIAG((D_mouse, locker, "post form do to %s", locker->name));
		//	post_cevent(locker, cXA_form_do, NULL,NULL, 0, 0, NULL, md);
			return;
		}
	}
	if ( C.update_lock && (locker = get_update_locker()))
	{
		DIAG((D_mouse, locker, "XA_button_event - screen locked by %s", locker->name));

		if ((wind = nolist_list) && wind->owner == locker)
		{
			post_cevent(locker, cXA_wheel_event, wind, NULL, 0, 0, NULL, md);
			//dispatch_button_event(lock, wind, md);
			return;
		}
		else if (locker->fmd.mousepress)
		{
		//	DIAG((D_mouse, locker, "post form do to %s", locker->name));
		//	post_cevent(locker, cXA_form_do, NULL,NULL, 0, 0, NULL, md);
			return;
		}
	}
	locker = C.mouse_lock ? get_mouse_locker() : NULL;
	wind = find_window(lock, md->x, md->y, FNDW_NOLIST|FNDW_NORMAL);
	if( cfg.menu_bar && wind->r.g_y < screen.c_max_h + 2 )
	{
		toggle_menu(lock, -2);
	}


	client = wind == root_window ? get_desktop()->owner : (wind ? wind->owner : NULL);

	if (locker && client != locker)
	{
		if ((locker->waiting_for & MU_WHEEL))
		{
			client = locker;
			wind = NULL;
		}
		else
			client = NULL;
	}

	if (client)
	{
		post_cevent(client, cXA_wheel_event, wind, NULL, 0, 0, NULL, md);

	}
}
static bool
chk_button_waiter(struct moose_data *md)
{
	if (C.button_waiter && md->ty == MOOSE_BUTTON_PREFIX)
	{
		if (C.button_waiter->status & CS_MENU_NAV)
		{
			add_client_md(C.button_waiter, md);
			C.button_waiter = NULL;
		}
		else
		{
			DIAGA(("chk_button_waiter: Got button_waiter %s", C.button_waiter->name));
			add_client_md(C.button_waiter, md);
			Unblock(C.button_waiter, 1);
			if (!(md->state && md->cstate))
				C.button_waiter = NULL;
			return true;
		}
	}
	return false;
}

/*
 * Here we decide what to do with a new moose packet.
 * Separeted to make it possible to send moose packets from elsewhere...
 */
static int
new_moose_pkt(int lock, int internal, struct moose_data *md /*imd*/)
{
	DIAGA(("new_moose_pkt: internal=%s, state %x, cstate %x, %d/%d - %d/%d, ty=%d",
		internal ? "yes":"no", md->state, md->cstate, md->x, md->y, md->sx, md->sy, md->ty));
	/*
	 * Ozk: We cannot distpach anything while a client wants exclusive
	 * right to mouse inputs. So, we need a flag. If this flag is set, it means
	 * a client wants exclusive rights to the mouse, and we never ever dispatch
	 * events normally. If flag is set, but client is not waiting,
	 * i.e., S.wait_mouse == NULL, it means the client is processing mouse input,
	 * and we gotta buffer it, so client receive it at the next wait_mouse() call.
	 */
	if (S.wm_count)
	{
		DIAGA(("new_moose_pkt: Got wm_count = %d", S.wm_count));

		/*
		 * Check if a client is actually waiting. If not, buffer the packet.
		 */
		if (md->ty == MOOSE_WHEEL_PREFIX)
			return true;

		if (internal || (S.wait_mouse && (S.wait_mouse->waiting_for & XAWAIT_MOUSE)))
		{
			/* a client wait exclusivly for the mouse */
			short *data = S.wait_mouse->waiting_short;

			if (C.button_waiter == S.wait_mouse && md->ty == MOOSE_BUTTON_PREFIX)
			{
				add_client_md(C.button_waiter, md);
				if (!(md->state && md->cstate))
					C.button_waiter = NULL;
			}

			data[0] = md->cstate;
			data[1] = md->x;
			data[2] = md->y;

			if (!internal)
				wake(IO_Q, (long)S.wait_mouse->sleeplock);
		}
		else
			buffer_moose_pkt(md);

		return true;
	}
	/*
	 * Ozk: No client is exclusively waiting for mouse, but we check
	 * if the queue contains anything and clear it if it does.
	 */
	if (md_buff_head != md_buff_tail)
		md_buff_head = md_buff_tail = md_lmvalid = 0;

	if (chk_button_waiter(md))
		return true;

	/*
	 * Ozk: now go dispatch the mouse event....
	 */
	switch (md->ty)
	{
		case MOOSE_BUTTON_PREFIX:
		{
			DIAGA(("new_moose_pkt: Button %d, cstate %d on: %d/%d",
				md->state, md->cstate, md->x, md->y));

			XA_button_event(lock, md, true);
			break;
		}
		case MOOSE_MOVEMENT_PREFIX:
		{
			/* mouse rectangle events */
			/* DIAG((D_v, NULL,"mouse move to: %d/%d", mdata.x, mdata.y)); */

			/*
			 * Call the mouse movement event handler
			*/
			md->clicks &= 1;	/* cannot double-click & move (?) */
			XA_move_event(lock, md);

			break;
		}

		case MOOSE_WHEEL_PREFIX:
		{
			XA_wheel_event(lock, md);

			break;
		}
		default:
		{
			DIAGA(("new_moose_pkt: Unknown mouse datatype (0x%x)", md->ty));
			DIAGA(("               l=0x%x, ty=%d, x=%d, y=%d, state=0x%x, clicks=%d",
				md->l, md->ty, md->x, md->y, md->state, md->clicks));
			DIAGA(("               dbg1=0x%x, dbg2=0x%x",
				md->dbg1, md->dbg2));

			return false;
		}
	}
	return true;
}

static short last_x = 0;
static short last_y = 0;
static TIMEOUT *b_to = NULL;
static TIMEOUT *m_to = NULL;
static TIMEOUT *m_rto = NULL;

#if WITH_BBL_HELP

static TIMEOUT *ms_to = NULL;
static short bbl_cnt = 0;
static long bbl_to = BBL_MIN_TO;
static void
m_not_move_timeout(struct proc *p, long arg)
{
	BBL_STATUS b = xa_bubble( 0, bbl_get_status, 0, 3 );
	struct xa_window *wind;

	ms_to = NULL;
	if( bbl_cnt == 0 )
		bbl_cnt = 1;
	if( bbl_to > BBL_MIN_TO )
		bbl_to -= BBL_MIN_TO;
	if( b == bs_open || --bbl_cnt )
		return;

	wind = find_window( 0, last_x, last_y, FNDW_NOLIST | FNDW_NORMAL );
	if( wind && wind->owner && wind->owner->p)
	{
		bubble_request( wind->owner->p->pid, wind->handle, x_mouse, y_mouse );
	}
}
#endif
static void move_timeout(struct proc *, long arg);

/*
 * Ozk: We get here when we've waited some time for WM_REDRAW
 * messages to be processed by clients. If we get here,
 * its either because clients didnt have enough time to
 * process all WM_REDRAWS, or a client (or more) does not
 * respond to AES messages. Clients may be "lagging" due to
 * heavy work or the a client may have crashed.
*/
static void
move_rtimeout(struct proc *p, long arg)
{
	struct xa_client *client;

	FOREACH_CLIENT(client)
	{
		if (client->rdrw_msg || client->irdrw_msg)
		{
			if (client->status & CS_LAGGING)
			{
				DIAGA(("%s lagging - cancelling all events", client->name));
// 				display("%s lagging - cancelling all events", client->name);
				cancel_aesmsgs(&client->rdrw_msg);
				cancel_aesmsgs(&client->msg);
				cancel_aesmsgs(&client->irdrw_msg);
				cancel_cevents(client);
			}
			else
			{
				DIAGA(("%s flagged as lagging", client->name));
// 				display("%s flagged as lagging", client->name);
				client->status |= CS_LAGGING;
			}
		}
	}

	C.redraws = 0;
	C.move_block = 0;
	m_rto = 0;

	/* XXX - Fixme!
	 * Ozk: Figure out, and flag, the client(s)
	 * that does not answer WM_REDRAW messages.
	 * Such a flag should prevent AES from sending it message
	 * until it wakes up (enters evnt_multi again)
	 */
	if (!m_to)
	{
		m_to = addroottimeout(0L, move_timeout, 1);
	}
}

/*
 * Ozk: move_timeout - added by the adi_move() function whenever
 * the mouse moves.
*/
static void
move_timeout(struct proc *p, long arg)
{
// 	bool ur_lock = (C.updatelock_count); // || C.rect_lock);
	struct moose_data md;

	m_to = NULL;

	if (!S.clients_exiting) // && !ur_lock)
	{
		/*
		 * Did mouse move since last time?
		*/
		if (last_x == x_mouse && last_y == y_mouse)
		{
		}
		else
		{
			last_x = x_mouse;
			last_y = y_mouse;

			/*
			 * Construct a moose_data using the the latest
			 * button-event data, only changing the mouse coords.
			*/
			mainmd.sx = x_mouse;
			mainmd.sy = y_mouse;
			md = mainmd;
			md.x = last_x;
			md.y = last_y;
			md.ty = MOOSE_MOVEMENT_PREFIX;
			vq_key_s(C.P_handle, &md.kstate);
			/*
			 * Deliver move event
			*/
			new_moose_pkt(0, 0, &md);

			/*
			 * Did the mouse move while processing last coords?
			 * If so, add a timeout on ourselves.
			*/
			if (last_x != x_mouse || last_y != y_mouse)
			{
// 				ur_lock = (C.updatelock_count); // || C.rect_lock);
				if (C.move_block) // || ur_lock) //C.redraws)
				{
					/*
					 * If redraw messages are still pending,
					 * start a timeout which will handle clients
					 * not responding/too busy to react to WM_REDRAWS
					*/
// 					if (!ur_lock)
// 					{
						if (cfg.redraw_timeout)
						{
							if (!m_rto)
								m_rto = addroottimeout(cfg.redraw_timeout/*400L*/, move_rtimeout, 1);
						}
// 					}
// 					else if (m_rto && !C.move_block)
// 					{
// 						cancelroottimeout(m_rto);
// 						m_rto = NULL;
// 					}
				}
				else
				{
					/*
					 * no redraws; if a pending timeout to handle locked/busy
					 * clients was issued earlier, cancel it
					*/
					if (m_rto)
					{
						cancelroottimeout(m_rto);
						m_rto = NULL;
					}
					/*
					 * new movement...
					*/
					m_to = addroottimeout(0L, move_timeout, 1);
				}
			}
		}

	}
}
#if WITH_BBL_HELP
static void new_bbl_timeout(unsigned long to)
{
	BBL_STATUS s = xa_bubble( 0, bbl_get_status, 0, 2 );
	if( s == bs_open )
	{
		post_cevent(C.Aes, XA_bubble_event, NULL, NULL, BBL_EVNT_CLOSE1, 0, NULL, NULL);
	}

	if (cfg.xa_bubble)
	{
		if (ms_to)
		{
			cancelroottimeout(ms_to);
			ms_to = NULL;
		}
		ms_to = addroottimeout(to, m_not_move_timeout, 1);
	}
}
#endif
/*
 * adi_move() AES Device Interface entry point,
 * taken whenever mouse moves.
 */
void
adi_move(struct adif *a, short x, short y)
{
	/*
	 * Always update these..
	*/
	x_mouse = x;
	y_mouse = y;

	if (C.move_block) //C.redraws)
	{
		/*
		 * If WM_REDRAW messages pending, add timeout
		 * to handle locked/busy clients.
		*/
		if (!m_rto && cfg.redraw_timeout)
			m_rto = addroottimeout(cfg.redraw_timeout, move_rtimeout, 1);
	}
	else
	{
		if (m_rto)
		{
			cancelroottimeout(m_rto);
			m_rto = NULL;
		}
		if (!m_to)
			m_to = addroottimeout(0L, move_timeout, 1);
	}
#if WITH_BBL_HELP
	new_bbl_timeout(bbl_to);
#endif
}

/*
 * WM_REDRAW event dispatchers calls kick_mousemove_timeout
 * when all WM_REDRAW messages have been processed.
 */
void
kick_mousemove_timeout(void)
{
	C.redraws--;

	if (!C.redraws)
	{
		C.move_block = 0;
		if (m_rto)
		{
			cancelroottimeout(m_rto);
			m_rto = NULL;
		}
		if (!m_to)
			m_to = addroottimeout(0L, move_timeout, 1);
	}
	else if (m_rto)
	{
		long newtimeout = cfg.redraw_timeout >> 2;
		if (newtimeout)
		{
			cancelroottimeout(m_rto);
			m_rto = addroottimeout(cfg.redraw_timeout, move_rtimeout, 1);
		}
	}
}

/*
 * button_timeout() - issued by the AES Device Interface
 * entry point for mouse button changes.
 */
static void
button_timeout(struct proc *p, long arg)
{
	b_to = NULL;

	if (!S.clients_exiting)
	{
		if (pending_md())
		{
			struct moose_data md;

			get_md(&md);
			DIAGA(("adi_button_event: type=%d, (%d/%d - %d/%d) state=%d, cstate=%d, clks=%d, l_clks=%d, r_clks=%d (%ld)",
				md.ty, md.x, md.y, md.sx, md.sy, md.state, md.cstate, md.clicks,
				md.iclicks.chars[0], md.iclicks.chars[1], sizeof(struct moose_data) ));
#if WITH_BBL_HELP
			if( xa_bubble( 0, bbl_get_status, 0, 11 ) == bs_open )
			{
				if( md.state == 1 )
					xa_bubble( 0, bbl_close_bubble2, 0, 0 );	/* left click: bubble off */
				bubble_show( 0 );	/* any click: close widget-bubble */
			}
			bbl_cnt = 2;
			if( bbl_to < BBL_MAX_TO )
				bbl_to += BBL_MIN_TO;
#endif
			vq_key_s(C.P_handle, &md.kstate);

			new_moose_pkt(0, 0, &md);

			if ( pending_md() )
				b_to = addroottimeout(0L, button_timeout, 1);
		}
	}
}

/*
 * adi_button() - the entry point taken by moose.adi whenever
 * mouse button packet is ready for delivery.
 */
void
adi_button(struct adif *a, struct moose_data *md)
{
	/*
	 * Ozk: should have obtained the keyboard-shift state here,
	 * but I dont think it is safe. So we get that in button_timeout()
	 * instead. Eventually, moose.adi will provide this info...
	 */

	/*
	 * Modify moose_data according to configuration (swap l/r buttons, etc)
	 */
	modify_md(md);
	/*
	 * Add moose_data to our internal queue
	 */
	add_md(md);
	/*
	 * Update mainmd, always containing the latest BUTTON moose_data
	 */
	new_mu_mouse(md);

	new_active_widget_mouse(md);

	/*
	 * If a movement timeout in progress, cancel it.
	 * Buttons are more important
	 */
	if (m_to)
	{
		cancelroottimeout(m_to);
		m_to = NULL;
	}
	/*
	 * If no button timeout in progress, add one now
	 */
	if (!b_to)
		b_to = addroottimeout(0L, button_timeout, 1);

	C.boot_focus = 0;
}

/*
 * adi_wheel() - The entry point taken by moose.adi when the mouse wheel turns.
 */
void
adi_wheel(struct adif *a, struct moose_data *md)
{
	add_md(md);
	if (!b_to)
	{
		b_to = addroottimeout(0L, button_timeout, 1);
	}
}

#if EIFFEL_SUPPORT
bool
eiffel_wheel(unsigned short scan)
{
	if (scan != 0x5b && scan >= 0x59 && scan <= 0x5d)
	{
		struct moose_data md;

		md.l		= sizeof(md);
		md.ty		= MOOSE_WHEEL_PREFIX;
		md.x = md.sx	= x_mouse;
		md.y = md.sy	= y_mouse;
		md.state	= (scan == 0x59 || scan == 0x5a) ? cfg.ver_wheel_id : cfg.hor_wheel_id;
		md.cstate	= md.state;
		md.clicks	= (scan == 0x59 || scan == 0x5c) ? -1 : 1;
		md.kstate	= 0;
		md.dbg1		= 0;
		md.dbg2		= 0;
		add_md(&md);
		if (!b_to)
			b_to = addroottimeout(0L, button_timeout, 1);
		return true;
	}
	return false;
}
#endif
/*
 * blocks until mouse input
 * context safe; scheduler called
 */
void
wait_mouse(struct xa_client *client, short *br, short *xr, short *yr)
{
	short data[3];
	struct moose_data md;

	/* wait for input from AESSYS */
	DIAGS(("wait_mouse for %s", client->name));

	if (client->md_head != client->md_tail || client->md_head->clicks != -1)
	{
		struct mbs mbs;
		get_mbstate(client, &mbs);
		data[0] = mbs.b;
		data[1] = mbs.x;
		data[2] = mbs.y;
	}
	else if (unbuffer_moose_pkt(&md))
	{
		DIAGS(("wait_mouse - return buffered"));
		data[0] = md.cstate;
		data[1] = md.x;
		data[2] = md.y;
	}
	else
	{
		client->waiting_for |= XAWAIT_MOUSE;
		client->waiting_short = data;

		/* only one client can exclusivly wait for the mouse */
		assert(S.wait_mouse == NULL);

		S.wait_mouse = client;

		/* XXX
		 * Ozk: A hack to make wait_mouse() not return unless wake
		 * really happened because of new mouse data... how else to do this?
		 */
		data[0] = 0xffff;
		while (data[0] == 0xffff)
		{
			do_block(client); //sleep(IO_Q, (long)client);
		}
		S.wait_mouse = NULL;

		client->waiting_for &= ~XAWAIT_MOUSE;
		client->waiting_short = NULL;
	}

	DIAG((D_mouse, NULL, "wait_mouse - return %d, %d.%d for %s",
		data[0], data[1], data[2], client->name));

	if (br)
		*br = data[0];
	if (xr)
		*xr = data[1];
	if (yr)
		*yr = data[2];
}

/*
 * non-blocking and context free mouse input check
 */
/*
 * Ozk: check_mouse() may be called by processes not yet called
 * appl_ini(). So, it must not depend on 'client' being valid!
 */
void
check_mouse(struct xa_client *client, short *br, short *xr, short *yr)
{
	if (client)
	{
		DIAG((D_mouse, NULL, "check_mouse - return %d, %d.%d for %s",
			mainmd.cstate, x_mouse, y_mouse, client->name));
	}
	else
	{
		DIAG((D_mouse, NULL, "check_mouse - return %d, %d.%d for non AES process (pid %ld)",
			mainmd.cstate, x_mouse, y_mouse, p_getpid()));
	}
	if (br)
		*br = mainmd.cstate;
	if (xr)
		*xr = x_mouse;
	if (yr)
		*yr = y_mouse;
}
