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

#include <mint/mintbind.h>

#include "xa_types.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "menuwidg.h"
#include "nkcc.h"
#include "objects.h"
#include "rectlist.h"
#include "widgets.h"
#include "xalloc.h"
#include "xa_evnt.h"


/* HR 050601: The active widgets are intimately connected to the mouse.
 *            There is only 1 mouse, so there is only need for 1 global structure.
 */
XA_PENDING_WIDGET widget_active = { NULL }; /* Pending active widget (if any) */


/* HR: Keystrokes must be put in a queue if no apps are waiting yet.
 * There also may be an error in the timer event code.
 * A queue like this is pretty anyhow, even if it is almost never needed.
 */

#define KEQ_L 64

struct key_q
{
	XA_CLIENT *client;
	XA_CLIENT *locked;
	KEY k;
};

struct key_queue
{
	int cur;
	int last;
	struct key_q q[KEQ_L];
};

static struct key_queue pending_keys;
static int pending_button_head	= 0;
static int pending_button_tail	= 0;
BUTTON pending_button[4] = { {NULL}, {NULL}, {NULL}, {NULL} };  /* for evnt fall thru */

#if 0
void
cancel_pending_button(void)
{
	if (pending_button.client)
	{
		pending_button.client = NULL;
		DIAG((D_button,NULL,"cancel_pending_button\n"));
	}
}
#endif

/* HR 180601: Now use the global (once) set values in the button structure */
void
multi_intout(short *o, int evnt)
{
	get_mouse(1);

	*o++ = evnt;
	*o++ = button.x;
	*o++ = button.y;
	*o++ = mu_button.b;	/*button.b;*/
	*o++ = button.ks;

	if (evnt)
	{
		*o++ = 0;
		*o++ = 0;
	}
}

#if GENERATE_DIAGS
static char *
em_flag(int f)
{
	static char *mo[6] = { "into", "outof", "into", "outof" };
	return mo[f & 3];
}
#endif

static void
do_widget_repeat(struct task_administration_block *tab)
{


	do_active_widget(tab->lock, tab->client);

	if (widget_active.b)
	{
		tab->timeout = 1;
	}
	else
	{
		tab->timeout = 0;
		tab->task = 0;
	}
		
}
void
set_widget_repeat(LOCK lock, XA_WINDOW *wind)
{
	Tab *t = &C.active_timeout;

	t->timeout = 1;
	t->wind = wind;
	t->client = wind->owner;
	t->task = do_widget_repeat;
	t->lock = lock;
}



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
 */

static bool
is_bevent(int gotbut, int gotcl, short *o, int which)
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

	DIAG((D_button,NULL,"[%d]is_bevent? %s; gotb %d; gotc %d; clks 0x%x, msk %d, st %d\n",
		which, ev ? "Yes" : "No", gotbut, gotcl, clks, msk, st));

	return ev;
}

/* HR 050402: WDIAL: split off as a function for use in ExitForm function */
void
button_event(LOCK lock, XA_CLIENT *client, struct moose_data *md)
{
	DIAG((D_button,NULL,"button event for %s\n", c_owner(client)));

	if (client->waiting_pb && client->client_end)
	{
		short *to = client->waiting_pb->intout;

		if (client->waiting_for & XAWAIT_MULTI)
		{
			/* If the client is waiting on a multi, the response is
			 * slightly different to the evnt_button() response.
			 */
			DIAG((D_button,NULL," -- XAWAIT_MULTI\n"));
			if (is_bevent(md->state, md->clicks, client->waiting_pb->intin + 1, 11))
			{
				*to++ = MU_BUTTON;
				*to++ = md->x;
				*to++ = md->y;
				*to++ = md->state;
				*to++ = button.ks;
				*to++ = 0;
				*to++ = md->clicks;

				button.got = true;
			/* Write success to clients reply pipe to unblock the process */
				Unblock(client, XA_OK, 3);
				DIAG((D_button,NULL," - written\n"));
			}
		} else
		{
			DIAG((D_button,NULL," -- evnt_button\n"));
			if (is_bevent(md->state, md->clicks, client->waiting_pb->intin, 12))
			{
/* HR: exchanged return of clicks & state */
				*to++ = md->clicks;
				*to++ = md->x;
				*to++ = md->y;
				*to++ = md->state;
				*to   = button.ks;
				button.got = true;
			/* Write success to clients reply pipe to unblock the process */
				Unblock(client, XA_OK, 4);
				DIAG((D_button,NULL," - written\n"));
			}
		}
	}
}

/* Ozk 040503: Collect up to 4 pending button events */
static void
add_pending_button(LOCK lock, XA_CLIENT *client, struct moose_data *md)
{

	Sema_Up(pending);

	if (!(pending_button[pending_button_head].client == client))
	{
		pending_button_tail = 0;
		pending_button_head = 0;
		pending_button[1].client = 0;
		pending_button[2].client = 0;
		pending_button[3].client = 0;
	}

	
	pending_button[pending_button_tail].client = client;
	pending_button[pending_button_tail].b = md->state;
	pending_button[pending_button_tail].cb = md->cstate;
	pending_button[pending_button_tail].clicks = md->clicks;

	pending_button_tail++;
	pending_button_tail &= 3;

	Sema_Dn(pending);
}

#if 0
static void
button_pending(LOCK lock, XA_CLIENT *client, struct moose_data *md)
{
	Sema_Up(pending);

		pending_button.client = client;
		pending_button.b = md->state;
		pending_button.cb = md->cstate;
		pending_button.clicks = md->clicks;			/* HR 310501: Oof! bad omission fixed. */
		/* A pending button is a single shot. */
		DIAG((D_button,NULL,"button queued st:%d\n", pending_button.b));

	Sema_Dn(pending);
}
#endif

static bool
do_fmd(LOCK lock, int up, struct moose_data *md)
{
	XA_CLIENT *client = Pid2Client(up);

	if (client && md->state == 1)
	{
		DIAGS(("Classic?  fmd.lock %d, via %lx\n", client->fmd.lock, client->fmd.mousepress));
		if (client->fmd.lock && client->fmd.mousepress)
		{
			client->fmd.mousepress(lock, client, md);		/* Dead simple (ClassicClick) */
			return true;
		}
	}

	return false;
}

void
XA_button_event(LOCK lock, struct moose_data *md, bool widgets)		/* HR at the moment widgets is always true. */
{
	XA_CLIENT *client;
	XA_WINDOW *wind;

/* HR: 19 october 2000; switched over to VMOOSE, the vdi vector based moose. */

	DIAG((D_button,NULL,"XA_button_event: %d/%d, state=0x%x, clicks=%d\n", md->x, md->y, md->state, md->clicks));

	/* Ozk 040503: Detect a button-released situation, and let active-widget get inactive */
	if (!md->state)		/* button released? */
	{
		widget_active.nx = md->x;
		widget_active.ny = md->y;
		widget_active.b  = 0;
		do_active_widget(lock, widget_active.wind->owner);
	}

	if (C.menu_base && md->state)		/* any button down */
	{
		Tab *tab = C.menu_base;
		if (tab->ty)
		{
			MENU_TASK *k = &tab->task_data.menu;
			wind = k->popw;

/* HR 161101: widgets in scrolling popups */
			if (wind)
				if (   (wind->dial&created_for_POPUP) != 0
				    && (wind->active_widgets&V_WIDG) != 0
				   )
					if (do_widgets(lock, wind, XaMENU, md))
						return;

			if (k->entry)
			{
				k->x = md->x;
				k->y = md->y;
				k->entry(tab);
				return;
			}
		}
	}

	/* See if a (classic) blocked form_do is active */
	if (S.mouse_lock)
		if (do_fmd(lock, S.mouse_lock, md))
			return;
	if (S.update_lock)
		if (do_fmd(lock, S.update_lock, md))
			return;

	wind = find_window(lock, md->x, md->y);				/* Try for a window */
	if (!wind)
	{
		DIAG((D_button,NULL,"click not in window\n"));
		return;
	}

/* HR 040401: left click on root object of rootwindow (the desktop). */
/* HR 280801: is now a true widget with behaviours. */

	if (S.mouse_lock == 0)					/* If mouse isn't locked, do a widgets test first */
	{
		if (md->state && widgets)
		{
			DIAG((D_button,NULL,"calling do_widgets\n"));
			if (do_widgets(lock, wind, 0, md))
				return;			/* Window widgets prrocessed. */
		}

		client = wind == root_window ? get_desktop()->owner : wind->owner;
	}
	else
	{
		client = Pid2Client(S.mouse_lock);			/* Mouse is locked - clicks go to owner of mouse */
		DIAG((D_button,NULL,"mouse is locked by %s\n", c_owner(client)));

		DIAG((D_button,NULL,"wind=%lx,st=%d,own=%d,toolbar=%d\n",
				wind, md->state, wind->owner->pid, (wind->active_widgets & TOOLBAR) != 0));
		if (wind->owner == client && (wind->active_widgets & TOOLBAR))
		{
			if (md->state && widgets)
			{
				DIAG((D_button,NULL,"calling do_widgets no_work for %s\n", c_owner(client)));
				if (do_widgets(lock, wind, 0, md))			/* HR 161101: mask */
					return;	/* Process window widgets for toolbar windows (this'll deal with alerts) */
			}
		}
		else
		{
			if (client->waiting_for & MU_BUTTON)
				button_event(lock, client, md);
			else
//				button_pending(lock, client, md);
				add_pending_button(lock, client, md);
			return;
		}
	}

	/* HR 041101: click on work area of iconified window :: send UNICONIFY. */
	if (   md->state == 1
	    && md->clicks == 2
	    && wind->window_status == XAWS_ICONIFIED)
	{
		wind->send_message(lock, wind, NULL,
					WM_UNICONIFY, 0, 0, wind->handle,
					wind->ro.x, wind->ro.y, wind->ro.w, wind->ro.h);
		return;
	}

	Sema_Up(clients);

	/* If the client owning was waiting for a button event, send it */ 
	/* - otherwise forget it, 'coz we don't want delayed clicks (they are confusing to the user [ie. me] ) */

	DIAG((D_button,NULL,"  -- client %s\n", c_owner(client)));
	
	/* HR: Very annoying not getting WM_TOPPED if you click a workarea	*/
	if (   md->state == 1
	    && wind != window_list
	    && wind != root_window
	    && wind->owner == client				/* HR 150601: Mouse lock !!! */
/*	    && (client->waiting_for & MU_MESAG) */
	    && (wind->active_widgets&NO_TOPPED) == 0)		/* WF_BEVENT set */
	{
		DIAG((D_wind,wind->owner,"send WM_TOPPED to %s\n", c_owner(client)));
		wind->send_message(lock|clients, wind, NULL,
					WM_TOPPED, 0, 0, wind->handle,
					0, 0, 0, 0);
	}
	else if (   md->state == 1
		 && wind == window_list
		 && wind != root_window
		 && C.focus == root_window)
	{
		C.focus = window_list;
		client = window_list->owner;
//		display("Click on unfocused top_window of (%d) %s\n", client->pid, client->name);
		DIAG((D_menu,NULL,"Click on unfocused top_window of %s\n", c_owner(client)));
		display_window(lock|clients, 112, window_list, NULL);   /* Redisplay titles */
		send_ontop(lock|clients);
		swap_menu(lock|clients, client, true, 4);
	}
	else if (client->waiting_for & MU_BUTTON)
		button_event(lock, client, md);
	else
//		button_pending(lock, client, md);
		add_pending_button(lock, client, md);

	Sema_Dn(clients);
}

static bool
mouse_ok(XA_CLIENT *client)
{
	IFDIAG(if (S.mouse_lock)
		DIAG((D_sema,client,"Mouse OK? %d pid = %d\n", S.mouse_lock, client->pid));)

	if (S.mouse_lock <= 0 || S.mouse_lock >= MAX_PID)
		return true;

	if (S.mouse_lock == client->pid)
		return true;

	/* mouse locked by another client */
	return false;
}

bool
is_rect(short x, short y, int fl, RECT *o)
{
	bool in = m_inside(x, y, o);
	bool f = (fl == 0);

	return f == in;
}

/* HR: now it is implemented quite equivalent to button events */
int
XA_move_event(LOCK lock, struct moose_data *md)
{
	XA_CLIENT *client;
	short x = md->x;
	short y = md->y;

	/* Ozk 040503: Moved the continuing handling of widgets actions here
	 * so we dont have to msg the client to make real-time stuff
	 * work. Having it here saves time, since it only needs to be
	 * done when the mouse moves.
	 */
	if (widget_active.widg)
	{
		widget_active.nx = md->x;
		widget_active.ny = md->y;
		widget_active.b  = md->state;
		do_active_widget(lock, widget_active.wind->owner);
		return false;
	}

	/* XaAES internal move event handling */
	if (C.menu_base)
	{
		/* Any part of a menu pulled? */

		MENU_TASK *k = &C.menu_base->task_data.menu;

		if (k->em.flags & MU_MX)
		{
			/* XaAES internal flag: report any mouse movement. */

			k->em.flags = 0;
			k->x = x;
			k->y = y;
			k->em.t1(C.menu_base);	/* call the function */
		}
		else if (k->em.flags & MU_M1)
		{
			if (is_rect(x, y, k->em.flags & 1, &k->em.m1))
			{
				k->em.flags = 0;
				k->x = x;
				k->y = y;
				k->em.t1(C.menu_base);	/* call the function */
			}
			else
			/* HR: MU_M2 not used for menu's anymore, replaced by MU_MX */
			/* I leave the text in, because one never knows. */
			if (k->em.flags & MU_M2)
			{
				if (is_rect(x, y, k->em.flags & 2, &k->em.m2))
				{
					k->em.flags = 0;
					k->x = x;
					k->y = y;
					k->em.t2(C.menu_base);
				}
			}
		}
		return false;
	}

	Sema_Up(clients);

	/* Moving the mouse into the menu bar is outside
	 * Tab handling, because the bar itself is not for popping.
	 */
	{
		/* HR: watch the menu bar as a whole */
		XA_CLIENT *aes = C.Aes;

		if (   (aes->waiting_for & XAWAIT_MENU)
		    && (aes->em.flags & MU_M1))
		{
			if (   cfg.menu_behave != PUSH
			    && (S.update_lock == C.AESpid || S.update_lock == 0)
			    && is_rect(x, y, aes->em.flags & 1, &aes->em.m1))
			{
				XA_WIDGET *widg = get_widget(root_window, XAW_MENU);
				cancel_evnt_multi(aes,2);

				Sema_Dn(clients);

				/* This is the root_window function for the menu
				 * tasks. (could be click_menu_widget)
				 */
				widg->click(lock, root_window, widg);
				return false;
			}
		}
	}


	/* HR 150601: mouse lock is also for rectangle events! */
	if (S.mouse_lock)
		client = Pid2Client(S.mouse_lock);
	else
		client = S.client_list;

	/* HR: internalized the client loop */
	while (client)
	{
		if (   client->client_end
		    && (client->waiting_for & (MU_M1|MU_M2|MU_MX)))
		{
			AESPB *pb = client->waiting_pb;
			
			pb->intout[0] = 0;		/* HR 220501: combine mouse events. */

			if (   (client->em.flags & MU_M1)
			    && is_rect(x, y, client->em.flags & 1, &client->em.m1))
			{
				if (client->waiting_for & XAWAIT_MULTI)
				{
					DIAG((D_mouse,client,"MU_M1 for %s\n", c_owner(client))); 
					multi_intout(pb->intout, pb->intout[0] | MU_M1);
				}
				else
				{
					multi_intout(pb->intout, 0);
					pb->intout[0] = 1;
				}
			}

			if (   (client->em.flags & MU_M2)		/* M2 in evnt_multi only */
			    && is_rect(x, y, client->em.flags & 2, &client->em.m2))
			{
				DIAG((D_mouse,client,"MU_M2 for %s\n", c_owner(client))); 
				multi_intout(pb->intout, pb->intout[0] | MU_M2);
			}

			if (client->em.flags & MU_MX)			/* MX: any movement. */
			{
				DIAG((D_mouse,client,"MU_MX for %s\n", c_owner(client))); 
				multi_intout(pb->intout, pb->intout[0] | MU_MX);
			}

			if (pb->intout[0])
				/* Write success to clients reply pipe to unblock the process */
				Unblock(client, XA_OK, 5);
		}

		/* HR 150601: mouse lock is also for rectangle events! */
		if (client->pid == S.mouse_lock)
			break;
		else
			client = client->next;
	}

	Sema_Dn(clients);

	return false;
}

/*
 * HR: Generalization of focus determination.
 *      Each step checks MU_KEYBD except the first.
 *		The top or focus window can have a keypress handler in stead of the XAWAIT_KEY flag.
 *
 *       first:    check focus keypress handler (no MU_KEYBD or update_lock needed)
 *       second:   check update lock
 *       last:     check top or focus window
 *
 *  240401: Interesting bug found and killed:
 *       If the update lock is set, then the key must go to that client,
 *          If that client is not yet waiting, the key must be queued,
 *          the routine MUST pass the client pointer, so there is a pid to be
 *          checked later.
 *       In other words: There can always be a client returned. So we must only know
 *          if that client is already waiting. Hence the ref bool.
 */

static XA_CLIENT *
find_focus(bool *waiting, XA_CLIENT **locked_client)
{
	XA_WINDOW *top = window_list;
	XA_CLIENT *client, *locked = NULL;

#if GENERATE_DIAGS
	if (C.focus == root_window)
	{
		DIAGS(("C.focus == root_window\n"));
	}
#endif
	if (top == C.focus && top->keypress)
	{
		/* this is for windowed form_do which doesn't
		 * set the update lock.
		 */
		*waiting = true;
		TRACE(1);
		return top->owner;
	}

	/* HR 141201: special case, no menu bar, possibly no windows either
	 * but a dialogue on the screen, not governed by form_do. (handled above)
	 * The client must also be waiting.
	 */
	if (S.update_lock > 0 && S.update_lock < MAX_PID)
	{
		locked = Pid2Client(S.update_lock);
		*locked_client = locked;
		TRACE(2);
	}
	else if (S.mouse_lock > 0 && S.mouse_lock < MAX_PID)
	{
		locked = Pid2Client(S.mouse_lock);						
		*locked_client = locked;
		TRACE(3);
	}

	if (locked)
	{
		client = locked;
		if (client->fmd.keypress) /* HR 250602 classic (blocked) form_do */
		{
			*waiting = true;
			TRACE(4);
			return client;
		}

		if ((client->waiting_for & (MU_KEYBD|MU_NORM_KEYBD)) != 0 || top->keypress != NULL)
		{
			*waiting = true;
			TRACE(5);
			return client;
		}
	}

	/* HR 131202: removed some spuriosuty and unclear stuff (things got too complex) */
	/* If C.focus == rootwindow, then the top_window owner is not the menu owner;
	                 the menu has prcedence, and the top window isnt drawn bold. */
	client = focus_owner();
	*waiting = (client->waiting_for & (MU_KEYBD|MU_NORM_KEYBD)) != 0 || top->keypress != NULL;
	TRACE(9);

	return client;
}

/*
   Keyboard input handler
   ======================

   Courtesy Harald Siegmund:

   nkc_tconv: TOS key code converter

   This is the most important function within NKCC: it takes a key code
   returned by TOS and converts it to the sophisticated normalized format.

   Note: the raw converter does no deadkey handling, ASCII input or
         Control key emulation.

   In:   D0.L           key code in TOS format:
                                    0                    1
                        bit 31:     ignored              ignored
                        bit 30:     ignored              ignored
                        bit 29:     ignored              ignored
                        bit 28:     no CapsLock          CapsLock
                        bit 27:     no Alternate         Alternate pressed
                        bit 26:     no Control           Control pressed
                        bit 25:     no left Shift key    left Shift pressed
                        bit 24:     no right Shift key   right Shift pressed

                        bits 23...16: scan code
                        bits 15...08: ignored
                        bits 07...00: ASCII code (or rubbish in most cases
                           when Control or Alternate is pressed ...)

   Out:  D0.W           normalized key code:
                        bits 15...08: flags:
                                    0                    1
                        NKF?_FUNC   printable char       "function key"
                        NKF?_RESVD  ignore it            ignore it
                        NKF?_NUM    main keypad          numeric keypad
                        NKF?_CAPS   no CapsLock          CapsLock
                        NKF?_ALT    no Alternate         Alternate pressed
                        NKF?_CTRL   no Control           Control pressed
                        NKF?_LSH    no left Shift key    left Shift pressed
                        NKF?_RSH    no right Shift key   right Shift pressed

                        bits 07...00: key code
                        function (NKF?_FUNC set):
                           < 32: special key (NK_...)
                           >=32: printable char + Control and/or Alternate
                        no function (NKF?_FUNC not set):
                           printable character (0...255!!!)
 */

/* HR 050402: WDIAL: split off as a function for use in ExitForm functions */
void 
keybd_event(LOCK lock, XA_CLIENT *client, KEY *key)
{
	AESPB *pb = client->waiting_pb;

	if (client->waiting_for & XAWAIT_MULTI)
	{
		/* If the client is waiting on a multi, the response is
		 *  slightly different to the evnt_keybd() response.
		 */
		get_mouse(2);
		button.ks =  key->raw.conin.state;

		/* XaAES extension: return normalized keycode for MU_NORM_KEYBD */
		if (client->waiting_for & MU_NORM_KEYBD)
		{
			/* if (key->norm == 0) */
				key->norm = nkc_tconv(key->raw.bcon);
			multi_intout(pb->intout, MU_NORM_KEYBD);
			pb->intout[5] = key->norm;
			pb->intout[4] = key->norm; /* for convenience */

			DIAG((D_k, NULL, "evnt_multi normkey to %s: 0x%04x\n",
				c_owner(client), key->norm));	
		}
		else
		{
			multi_intout(pb->intout, MU_KEYBD);
			pb->intout[5] = key->aes;

			DIAG((D_k, NULL, "evnt_multi key to %s: 0x%04x\n",
				c_owner(client), key->aes));	
		}
	}
	else
	{
		pb->intout[0] = key->aes;
		DIAG((D_k, NULL, "evnt_keybd keyto %s: 0x%04x\n",
			c_owner(client), key->aes));	
	}

	/* Write success to client's reply pipe to unblock the process */
	Unblock(client, XA_OK, 6);
}


static XA_WIDGET *
wheel_arrow(XA_WINDOW *wind, struct moose_data *md)
{
	XA_WIDGETS which;
	XA_WIDGET  *widg;
	int fac = wind->owner->options.wheel_page;

	if (md->state == 0)
	{
		if (md->clicks < 0)
			which = XAW_UPLN;
		else
			which = XAW_DNLN;
	}
	else if (md->state == 1)
	{
		if (md->clicks < 0)
			which = XAW_LFLN;
		else
			which = XAW_RTLN;
	}
	else
		return NULL;

	if (fac && abs(md->clicks) > abs(fac))
	{
		switch (which)
		{
		case XAW_UPLN: which = XAW_UPPAGE; break;
		case XAW_DNLN: which = XAW_DNPAGE; break;
		case XAW_LFLN: which = XAW_LFPAGE; break;
		case XAW_RTLN: which = XAW_RTPAGE; break;
		default: /* make gcc happy */ break;
		}
	}

	widg = get_widget(wind, which);
	if (widg)
	{
		if (widg->type)
			return widg;
	}

	return NULL;
}

void
XA_wheel_event(LOCK lock, struct moose_data *md)
{
	XA_WINDOW *wind = window_list;
	XA_CLIENT *client = NULL;
	XA_WIDGET *widg = wheel_arrow(wind, md);
	int n,c;

	DIAGS(("mouse wheel %d has wheeled %d\n", md->state, md->clicks));

	if (S.mouse_lock)
		client = Pid2Client(S.mouse_lock);

	if (   ( client && widg && wind->send_message && wind->owner == client)
	    || (!client && widg && wind->send_message))
	{
		DIAGS(("found widget %d\n",widg->type));
		client = wind->owner;
		if (client->wa_wheel || wind->wa_wheel)
		{
			DIAGS(("clwa %d, wiwa %d\n", client->wa_wheel, wind->wa_wheel));
			wind->send_message(lock, wind, NULL,
					WM_ARROWED, 0, 0, wind->handle,
					WA_WHEEL,
					0, md->state, md->clicks);
		}
		else
		{
			n = c = abs(md->clicks);
			while (c)
			{
				wind->send_message(lock, wind, NULL,
						WM_ARROWED, 0, 0, wind->handle,
						client->options.wheel_reverse ? widg->xarrow : widg->arrowx,
						/* 'MW' and 'Mw' */
						c == n ? 0x4d57 : 0x4d77, 0, c);
				c--;
			}
		}
	}
	else if (client)
	{
		DIAGS(("wheel event for %s, waiting %d\n",
				c_owner(client),client->waiting_for));

#if 0
		if (client->fmd)
			 /* Might be a model dialogue; implement at this point . */
		else
#endif
		if (client->waiting_for & MU_WHEEL)
		{
			AESPB *pb = client->waiting_pb;
		
			if (pb && client->client_end)
			{
				multi_intout(pb->intout, MU_WHEEL);
				pb->intout[4] = md->state;
				pb->intout[6] = md->clicks;

			/* Write success to clients reply pipe to unblock the process */
				Unblock(client, XA_OK, 3);
				DIAGS((" - written\n"));
			}
		}
	}
}

void
XA_keyboard_event(LOCK lock, KEY *key)
{
	bool waiting;
	XA_WINDOW *top = window_list;
	XA_CLIENT *client, *locked_client;
	AESPB *pb;

	client = find_focus(&waiting, &locked_client);		/* HR 161201 */
	pb     = client->waiting_pb;

DIAG((D_keybd,client,"XA_keyboard_event: %s; update_lock:%d, focus: %s, window_list: %s\n",
			waiting ? "waiting" : "", S.update_lock, c_owner(client), w_owner(top)));

	if (waiting)		/* Found either (MU_KEYBD|MU_NORM_KEYBD) or keypress handler. */
	{
		/* See if a (classic) blocked form_do is active */
		if (S.update_lock == client->pid)
		{
			DIAGS(("Classic: fmd.lock %d, via %lx\n", client->fmd.lock, client->fmd.keypress));
			if (client->fmd.lock)
				if (client->fmd.keypress)
				{
					client->fmd.keypress(lock, NULL, &client->wt, key->aes, key->norm, *key);
					return;
				}
		}

		if (is_hidden(top))		/* HR 210801 */
		{
			unhide_window(lock, top);
			return;
		}
		
		if (top->keypress)		/* Does the top&focus window have a keypress handler callback? */
		{
			top->keypress(lock, top, NULL, key->aes, key->norm, *key);
			return;
		}
		else if (!pb)
		{
			DIAGS(("XA_keyboard_event: INTERNAL ERROR: No waiting pb.\n"));
			return;
		}

		Sema_Up(clients);
		keybd_event(lock, client, key);
		Sema_Dn(clients);

	}
	else
	{
		int c = pending_keys.cur,
		    e = pending_keys.last;

		Sema_Up(pending);

		DIAG((D_keybd,NULL,"pending key cur=%d\n", c));
		/* If there are pending keys and the top window owner has changed, throw them away. */

		/* HR 041101: FIX! must compare with last queued key!!! */
		if (   c != e
		    && client != pending_keys.q[e-1].client)
		{
			DIAG((D_keybd,NULL," -  clear: cl=%s\n", c_owner(client)));
			DIAG((D_keybd,NULL,"           qu=%s\n", c_owner(pending_keys.q[e-1].client)));
			e = c = 0;
		}

		if (e == KEQ_L)
			e = 0;

		DIAG((D_keybd,NULL," -     key %x to queue position %d\n", key->aes, e));
		pending_keys.q[e].k   = *key;				/* HR 240401: all of key */
		pending_keys.q[e].locked = locked_client;	/* HR 161201 */
		pending_keys.q[e].client = client;			/* HR 240401: see find_focus() */
		e++;
		pending_keys.last = e;
		pending_keys.cur = c;

		Sema_Dn(pending);
	}
}

static int
pending_msgs(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	int rtn;

	/* Is there a widget active (like a scroll arrow)? If so, check with the action first
	 * as it may result in some messages (just in case we've not got any already)
	 */
	/* The reason why it is done in here is that this way it works in sort of a feed back mode.
	   The widget is only actioned in case of a MESAG event wait from the client.
	   Otherwise every pixel slider move would result in a message sent.
	*/

	Sema_Up(clients);

#if 0
	if (!client->msg)
		/* now a function; used in woken_slist as well. */
		do_active_widget(lock|clients, client);

#endif

	rtn = (client->msg != NULL);
	/* Are there any messages pending? */
	if (rtn)
	{
		union msg_buf *buf = pb->addrin[0];
		XA_AESMSG_LIST *msg = client->msg;
		IFDIAG(char *pmsg(short m);)

		client->msg = msg->next;
		/* Copy the message into the clients buffer */
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending message %s for %s from %d\n",
			pmsg(buf->m[0]), c_owner(client), buf->m[1]));

		free(msg);
	}

	Sema_Dn(clients);
	return rtn;
}

/* Note that it might still be necessary to reorganize the kernel loop! */
static bool
pending_key_strokes(LOCK lock, AESPB *pb, XA_CLIENT *client, int type)
{
	bool ok = false, waiting = false;

	Sema_Up(pending);

	/* keys may be queued */
	if (pending_keys.cur != pending_keys.last)
	{
		IFDIAG(XA_CLIENT *qcl = pending_keys.q[pending_keys.cur].client;)
		XA_CLIENT *locked = NULL,
		          *foc = find_focus(&waiting, &locked);

		KEY key;
		DIAG((D_keybd, NULL, "Pending key: cur=%d,end=%d (qcl%d::cl%d::foc%d::lock%d)\n",
			pending_keys.cur, pending_keys.last, qcl->pid, client->pid, foc->pid, locked ? locked->pid : -1));		
		if (client == foc)
		{
			DIAG((D_keybd, NULL, "   --   Gotcha!\n"));

			key = pending_keys.q[pending_keys.cur].k;
	
			if (type)
			{
				get_mouse(3);
				button.ks =  key.raw.conin.state;

				/* XaAES extension: normalized key codes. */
				if (type & MU_NORM_KEYBD)
				{
					/* if (key.norm == 0) */
						key.norm = nkc_tconv(key.raw.bcon);
					pb->intout[5] = key.norm;
					pb->intout[4] = key.norm; /* for convenience */
				}
				else
					pb->intout[5] = key.aes;
			}
			else
				pb->intout[0] = key.aes;
	
			DIAG((D_keybd, NULL, "key 0x%x sent to %s\n",
				key.aes, c_owner(client)));

			pending_keys.cur++;
			if (pending_keys.cur == KEQ_L)
				pending_keys.cur = 0;
			if (pending_keys.cur == pending_keys.last)
				pending_keys.cur = pending_keys.last = 0;

			ok = true;
		}
	}

	Sema_Dn(pending);
	return ok;
}

bool naes12 = false;

static bool
still_button(LOCK lock, XA_CLIENT *client, short *o)
{

	XA_CLIENT *owner;
	XA_WINDOW *wind;
	short b;


	if ( C.menu_base || widget_active.widg || !mouse_ok(client) )
		return false;

	vq_mouse(C.vh, &b, &mu_button.x, &mu_button.y);
	vq_key_s(C.vh, &mu_button.ks);

	wind = find_window(lock, mu_button.x, mu_button.y);
	owner = wind == root_window ? get_desktop()-> owner : wind->owner;

	if (owner != client)
		return false;

	if (	m_inside(mu_button.x, mu_button.y, &wind->wa) &&
		!(client->fmd.lock && client->fmd.mousepress)	)
		return true;

	return false;

#if 0
	/* probably needs more work though. */
	XA_CLIENT *owner;
	XA_WINDOW *wind;

	if (   button.got
	    || C.menu_base
	    || widget_active.widg
	    || !mouse_ok(client))
		return false;

	/* Try for a window */
	wind = find_window(lock, button.x, button.y);
	owner = wind == root_window ? get_desktop()->owner : wind->owner;

	DIAG((D_v, client, "   --   wind %d\n", wind->handle));

	if (owner != client)
		return false;

	/* DIAG((D_v, client, "   --   %d,%d   wa %d/%d,%d/%d\n", button.x, button.y, wind->wa)); */
	/* DIAG((D_v, client, "   --   fmd %d, 0x%lx\n", client->fmd.lock, client->fmd.mousepress)); */

	/* Must call this for return true */
	get_mouse(10);

	if (    m_inside(button.x, button.y, &wind->wa)
	    && !(client->fmd.lock && client->fmd.mousepress))
			return true;

	return false;

#endif

#if 0
	get_mouse(11);
	return
            !button.got			/* XA_button_event() has not been called */
	    && ((o[2] == 0 && (o[0]&0x100) == 0) ||  o[2] != 0)
		&& C.menu_base == NULL
		&& widget_active.widg == NULL
		&& mouse_ok(client);

#endif
}

#if GENERATE_DIAGS
static char *xev[] = {"KBD","BUT","M1","M2","MSG","TIM","WHL","MX","NKBD","9","10","11","12","13","14","15"};

void
evnt_diag_output(void *pb, XA_CLIENT *client, char *which)
{
	if (pb)
	{
		short *o = ((AESPB *)pb)->intout;
		char evx[128];
		show_bits(o[0], "", xev, evx);
		DIAG((D_multi, client, "%sevnt_multi return: %s x%d, y%d, b%d, ks%d\n",
			which, evx, o[1], o[2], o[3], o[4]));
	}
}
#define diag_out(x,c,y) evnt_diag_output(x,c,y);
#else
#define diag_out(x,c,y)
#endif

/* HR 070601: We really must combine events. especially for the button still down situation.
*/

/*
 *	The essential evnt_multi() call
 */

unsigned long
XA_evnt_multi(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	bool mu_butt_p = 0;			/* Ozk 040503: Need to know if MU_BUTTON came from a pending button event
						 * or checking current button state in mu_button */
	short events = pb->intin[0];
	unsigned long ret = XAC_BLOCK;		/* HR: another example of choosing inconvenient default fixed. */
	int new_waiting_for = 0,
	    fall_through = 0,
	    pbi = pending_button_head;
	
	CONTROL(16,7,1)

	pb->intout[0] = 0;
	pb->intout[5] = 0;
	pb->intout[6] = 0;
	client->waiting_for = 0;
	client->waiting_pb = NULL;

#if GENERATE_DIAGS
	{
		char evtxt[128];
		show_bits(events, "evnt=", xev, evtxt);
		DIAG((D_multi,client,"evnt_multi for %s, %s clks=0x%x, msk=0x%x, bst=0x%x T:%d\n",
				c_owner(client),
				evtxt,pb->intin[1],pb->intin[2],pb->intin[3], (events&MU_TIMER) ? pb->intin[14] : -1));
	}
#endif

/*
	Excerpt from nkcc.doc, courtesy Harald Siegmund:

	Note: the NKCC button event handler supports the (undocumented)
	negation flag, which is passed in bit 8 of the parameter <bclicks>
	(maximum # of mouse clicks to wait for). You don't know this flag?
	I found an article about it in the c't magazine (I think it was
	issue 3/90, or maybe 4/90??) - and I damned Atari for their bad
	documentation. This flag opens the way to check BOTH mouse buttons
	at the same time without any problems. When set, the return
	condition is inverted. Let's have a look at an example:
	
	mask = evnt_multi(MU_BUTTON,2,3,3,...
	
	This doesn't work the way we want: the return condition is
	"button #0 pressed AND button #1 pressed". But look at this:
	
	mask = evnt_multi(MU_BUTTON,0x102,3,0,...
	
	Now the condition is "NOT (button #0 released AND button #1
	released)". Or in other words: "button #0 pressed OR button #1
	pressed". Nice, isn't it?!

 */

 /*
	Ozk 040503:
	Now this is how things work; if we're waiting for MU_BUTTON,
	we check if there'a pending button. If there is, we check that.
	If there are no pending buttons, we check with mu_button instead,
	because it contains the last mouse event packet returned from moose.
 */

	if (events & MU_BUTTON)
	{

		Sema_Up(pending);

		if (pending_button[pbi].client	== client && mouse_ok(client))
		{
			DIAG((D_button,NULL,"pending_button multi %d\n", pending_button.b));
			pending_button[pbi].client = NULL;			/* is single shot. */
			pending_button_head++;
			pending_button_head &= 3;

			if (is_bevent(pending_button[pbi].b, pending_button[pbi].clicks, pb->intin + 1, 1))
			{
				fall_through |= MU_BUTTON;
				mu_butt_p = true;		/* pending button used */
DIAG((D_button,NULL,"fall_through |= MU_BUTTON\n"));
			}
		}
		else
		{
DIAG((D_button,NULL,"still_button multi?? o[0,2] %x,%x button.got %d, lock %d, Mbase %lx, active.widg %lx\n",
                                   pb->intin[1], pb->intin[3], button.got, S.mouse_lock, C.menu_base, widget_active.widg));
			if (still_button(lock, client, pb->intin + 1))
			{
DIAG((D_button,NULL,"still_button multi %d,%d/%d\n", button.b, button.x, button.y));
				if (is_bevent(mu_button.b, 0, pb->intin + 1, 2))
				{
DIAG((D_button,NULL,"still button %d: fall_through |= MU_BUTTON\n", button.b));					
					fall_through |= MU_BUTTON;
					button.got = true;				/* Mark button state processed. */
				}
			}
		}

		Sema_Dn(pending);

		if ((fall_through&MU_BUTTON) == 0)
		{
			new_waiting_for |= MU_BUTTON;		/* Flag the app as waiting for button changes */
			pb->intout[0] = 0;
DIAG((D_b,client,"new_waiting_for |= MU_BUTTON\n"));
		}
	}

	if (events & (MU_NORM_KEYBD|MU_KEYBD))		
	{
		short ev = events&(MU_NORM_KEYBD|MU_KEYBD);
		if (pending_key_strokes(lock, pb, client, ev))
			fall_through    |= ev;
		else
			new_waiting_for |= ev;			/* Flag the app as waiting for keypresses */
	}

/* HR: event data are now in the client structure */
/*     040401: Implemented fall thru. */
	if (events & (MU_M1|MU_M2|MU_MX))
	{
		bzero(&client->em, sizeof(XA_MOUSE_RECT));
		if (events & MU_M1)					/* Mouse rectangle tracking */
		{
			RECT *r = (RECT *)&pb->intin[5];
			client->em.m1 = *r;
			client->em.flags = pb->intin[4] | MU_M1;
DIAG((D_multi,client,"    M1 rectangle: %d/%d,%d/%d, flag: 0x%x: %s\n", r->x, r->y, r->w, r->h, client->em.flags, em_flag(client->em.flags)));
			get_mouse(4);
			if (mouse_ok(client) && is_rect(button.x, button.y, client->em.flags & 1, &client->em.m1))
				fall_through    |= MU_M1;
			else
				new_waiting_for |= MU_M1;
		}

		if (events & MU_MX)					/* HR: XaAES extension: any mouse movement. */
		{
			client->em.flags = pb->intin[4] | MU_MX;
DIAG((D_multi,client,"    MX\n"));
			new_waiting_for |= MU_MX;
		}

		if (events & MU_M2)
		{
			RECT *r = (RECT *)&pb->intin[10];
			client->em.m2 = *r;
			client->em.flags |= (pb->intin[9] << 1) | MU_M2;
DIAG((D_multi,client,"    M2 rectangle: %d/%d,%d/%d, flag: 0x%x: %s\n", r->x, r->y, r->w, r->h, client->em.flags, em_flag(client->em.flags)));
			get_mouse(5);
			if (mouse_ok(client) && is_rect(button.x, button.y, client->em.flags & 2, &client->em.m2))
				fall_through    |= MU_M2;
			else
				new_waiting_for |= MU_M2;
		}
	}

	/* AES 4.09 */
	if (events & MU_WHEEL)
	{
		DIAG((D_i,client,"    MU_WHEEL\n"));
		new_waiting_for |= MU_WHEEL;
	}

	if (events & MU_MESAG)
	{
		if (pending_msgs(lock, client, pb))
			fall_through    |= MU_MESAG;
		else
			/* Mark the client as waiting for messages */
			new_waiting_for |= MU_MESAG;
	}

	/* HR: a zero timer (immediate timout) is also catered for in the kernel. */
	/* HR 051201: Unclumsify the timer value passing. */

	if (events & MU_TIMER)
	{
		/* The Intel ligent format */
		client->timer_val = ((long)pb->intin[15] << 16) | pb->intin[14];
		DIAG((D_i,client,"Timer val: %ld\n", client->timer_val));
		if (client->timer_val)
		{
			new_waiting_for |= MU_TIMER;	/* Flag the app as waiting for a timer */
			ret = XAC_TIMER;
		}
		else
		{
			/* Is this the cause of loosing the key's at regular intervals? */
			DIAG((D_i,client,"Done timer for %d\n", client->pid));
			fall_through |= MU_TIMER;
			/* HR 190601: to be able to combine fall thru events. */
			ret = XAC_DONE;
		}
	}

	if (fall_through)
	{
		/* HR: fill out the mouse data */
		multi_intout(pb->intout, 0);
		if ((fall_through&MU_TIMER) == 0)
			ret = XAC_DONE;
		if ((fall_through&MU_BUTTON) != 0)
		{
			/* HR 190601: Pphooo :-( This solves the Thing desk popup missing clicks. */
			/* Ozk 040501: And we need to take the data from the correct place. */
			if (mu_butt_p)
			{
				pb->intout[3] = pending_button[pbi].b;
				pb->intout[6] = pending_button[pbi].clicks;
			}
			else
				pb->intout[6] = mu_button.clicks;
		}
			
		pb->intout[0] = fall_through;
		diag_out(pb,client,"fall_thru ");
	}
	else if (new_waiting_for)
	{
		/* If we actually recognised any of the codes, then set the multi flag */

		/* Flag the app as waiting */
		client->waiting_for = new_waiting_for | XAWAIT_MULTI;

		/* HR 041201(changed place): Store a pointer to the AESPB
		 * to fill when the event(s) finally arrive.
		 */
		client->waiting_pb = pb;
	}

	return ret;
}

/*
 * Cancel an event_multi()
 * - Called when any one of the events we were waiting for occurs
 */
void
cancel_evnt_multi(XA_CLIENT *client, int which)
{
	client->waiting_for = 0;
	client->em.flags = 0;
	client->waiting_pb = NULL;
	DIAG((D_kern,NULL,"[%d]cancel_evnt_multi for %s\n", which, c_owner(client)));
}

/*
 * AES message handling
 */
unsigned long
XA_evnt_mesag(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(0,1,1)

	if (pending_msgs(lock, client, pb))
		return pb->intout[0] = 1, XAC_DONE;

	client->waiting_for = MU_MESAG;	/* Mark the client as waiting for messages */
	client->waiting_pb = pb;

	return XAC_BLOCK;
}

/*
 * evnt_button() routine
 */
unsigned long
XA_evnt_button(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	int pbi = pending_button_head;

	CONTROL(3,5,1)

	DIAG((D_button,NULL,"evnt_button for %s; clicks %d mask 0x%x state 0x%x\n",
	           c_owner(client), pb->intin[0], pb->intin[1], pb->intin[2]));

	Sema_Up(pending);

	if (pending_button[pbi].client == client && mouse_ok(client))
	{
		DIAG((D_button,NULL,"pending_button %d\n", pending_button.b));
		pending_button[pbi].client = NULL;			/* is single shot. */
		pending_button_head++;
		pending_button_head &= 3;

		if (is_bevent(pending_button[pbi].b, pending_button[pbi].clicks, pb->intin, 3))
		{
			multi_intout(pb->intout, 0);
			pb->intout[0] = pending_button[pbi].clicks;	/* Ozk 040503: Take correct data */
			pb->intout[3] = pending_button[pbi].b;
			Sema_Dn(pending);
			return XAC_DONE;
		}
	}
	else
	{
		DIAG((D_button,NULL,"still_button? o[0,2] %x,%x button.got %d, lock %d, Mbase %lx, active.widg %lx\n",
			pb->intin[0], pb->intin[2], button.got, S.mouse_lock, C.menu_base, widget_active.widg));

		if (still_button(lock, client, pb->intin))
		{
			DIAG((D_button,NULL,"still_button %d,%d/%d\n", button.b, button.x, button.y));
			if (is_bevent(mu_button.b, 0, pb->intin, 4))
			{
				DIAG((D_button,NULL,"    --    implicit button %d\n",button.b));
				multi_intout(pb->intout, 0);		/* 0 : for evnt_button */
				pb->intout[0] = 1;
				button.got = true;
				Sema_Dn(pending);
				return XAC_DONE;
			}
		}
	}

	/* Flag the app as waiting for messages */
	client->waiting_for = MU_BUTTON;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_pb = pb;

	Sema_Dn(pending);

	/* Returning this blocks the client app to wait for the event */
	return XAC_BLOCK;
}

/*
 * evnt_keybd() routine
 */
unsigned long
XA_evnt_keybd(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(0,1,0)

	if (pending_key_strokes(lock, pb, client, 0))
		return XAC_DONE;

	/* Flag the app as waiting for messages */
	client->waiting_for = MU_KEYBD;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_pb = pb;

	/* Returning false blocks the client app to wait for the event */
	return XAC_BLOCK;
}

/*
 * Event Mouse
 */
unsigned long
XA_evnt_mouse(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(5,5,0)

	/* Flag the app as waiting for messages */
	client->waiting_for = MU_M1;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_pb = pb;

	bzero(&client->em, sizeof(XA_MOUSE_RECT));
	client->em.m1 = *((RECT *) &pb->intin[1]);
	client->em.flags = (long)(pb->intin[0]) | MU_M1;

	get_mouse(6);
	if (mouse_ok(client) && is_rect(button.x, button.y, client->em.flags & 1, &client->em.m1))
	{
		multi_intout(pb->intout, 0);
		pb->intout[0] = 1;
		return XAC_DONE;
	}

	/* Returning false blocks the client app to wait for the event */
	return XAC_BLOCK;
}

/*
 * Evnt_timer()
 */
unsigned long
XA_evnt_timer(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(2,1,0)

	client->timer_val = ((long)pb->intin[1] << 16) | pb->intin[0];

	/* Flag the app as waiting for messages */
	client->waiting_pb = pb;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_for = MU_TIMER;

	return XAC_TIMER;
}
