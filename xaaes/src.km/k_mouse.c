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

#include "k_mouse.h"
#include "xa_global.h"
#include "c_mouse.h"

#include "app_man.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "desktop.h"
#include "init.h"
#include "k_init.h"
#include "k_main.h"
#include "k_shutdown.h"
#include "nkcc.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xalloc.h"


#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/signal.h"


/*
 * Ozk: The data in mainmd contains the data from the last button-change
 * and NOT the up-to-date mouse coordinates. Those are in x_mouse and y_mouse
 */
static short x_mouse;
static short y_mouse;
static struct moose_data mainmd;	// ozk: Have to take the most recent moose packet into global space
BUTTON mu_button = { NULL };

struct pending_button pending_button = { 0, 0, { {NULL}, {NULL}, {NULL}, {NULL} } };

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

/* HR 050402: WDIAL: split off as a function for use in ExitForm function */
void
button_event(enum locks lock, struct xa_client *client, const struct moose_data *md)
{
	DIAG((D_button, NULL, "button event for %s", c_owner(client)));

	if (client->waiting_pb)
	{
		short *to = client->waiting_pb->intout;

		if (client->waiting_for & XAWAIT_MULTI)
		{
			/* If the client is waiting on a multi, the response is
			 * slightly different to the evnt_button() response.
			 */
			DIAG((D_button, NULL, " -- XAWAIT_MULTI"));

			if (is_bevent(md->state, md->clicks, client->waiting_pb->intin + 1, 11))
			{
				*to++ = MU_BUTTON;
				*to++ = md->x;
				*to++ = md->y;
				*to++ = md->state;
				*to++ = mu_button.ks;
				*to++ = 0;
				*to++ = md->clicks;

				mu_button.got = true;

				client->usr_evnt = 1;
				Unblock(client, XA_OK, 3);
				
				DIAG((D_button, NULL, " - written"));
			}
		}
		else
		{
			DIAG((D_button, NULL, " -- evnt_button"));
			if (is_bevent(md->state, md->clicks, client->waiting_pb->intin, 12))
			{
				*to++ = md->clicks;
				*to++ = md->x;
				*to++ = md->y;
				*to++ = md->state;
				*to   = mu_button.ks;
				mu_button.got = true;

				client->usr_evnt = 1;
				Unblock(client, XA_OK, 4);
				DIAG((D_button, NULL, " - written"));
			}
		}
	}
}

static void
reset_pending_button(void)
{
	int i;

	pending_button.tail = 0;
	pending_button.head = 0;

	for (i = 0; i < 4; i++)
		pending_button.q[i].client = 0;
}

/* Ozk: Collect up to 4 pending button events -- do we need more? */
void
add_pending_button(enum locks lock, struct xa_client *client)
{
	Sema_Up(pending);

	if (!(pending_button.q[pending_button.head].client == client))
		reset_pending_button();

	pending_button.q[pending_button.tail].client	= client;
	pending_button.q[pending_button.tail].x		= mu_button.x;		/* md->x; */
	pending_button.q[pending_button.tail].y		= mu_button.y;		/* md->y; */
	pending_button.q[pending_button.tail].b		= mu_button.b;		/* md->state; */
	pending_button.q[pending_button.tail].cb	= mu_button.cb;		/* md->cstate; */
	pending_button.q[pending_button.tail].clicks	= mu_button.clicks;	/* md->clicks; */
	pending_button.q[pending_button.tail].ks	= mu_button.ks;

	pending_button.tail++;
	pending_button.tail &= 3;

	Sema_Dn(pending);
}

static bool
do_fmd(enum locks lock, struct xa_client *client, const struct moose_data *md)
{
	if (client && md->state == 1)
	{
		DIAGS(("Classic?  fmd.lock %d, via %lx", client->fmd.lock, client->fmd.mousepress));
		if (client->fmd.lock && client->fmd.mousepress)
		{
			client->fmd.mousepress(lock, client, md);		/* Dead simple (ClassicClick) */
			return true;
		}
	}

	return false;
}


static void
post_cevent(struct xa_client *client,
	void (*func)(enum locks, struct c_event *),
	void *ptr1, void *ptr2,
	int d1, int d2, RECT *r,
	const struct moose_data *md)
{
	int h = client->ce_head;

	client->ce[h].funct = func;
	client->ce[h].client = client;
	client->ce[h].ptr1 = ptr1;
	client->ce[h].ptr2 = ptr2;
	if (r)
		client->ce[h].r = *r;
	if (md)
		client->ce[h].md = *md;

	h++;
	DIAG((D_mouse, client, "added cevnt at %d, nxt %d (tail %d) for %s", client->ce_head, h, client->ce_tail, client->name));
	client->ce_head = h & MAX_CEVENTS;

	if (client != C.Aes)
		Unblock(client, 1, 5000);
	else
		dispatch_cevent(client);
}

/*
 * at the moment widgets is always true.
 */
void
XA_button_event(enum locks lock, const struct moose_data *md, bool widgets)
{
	struct xa_client *client, *locker;
	struct xa_window *wind;

	DIAG((D_button, NULL, "XA_button_event: %d/%d, state=0x%x, clicks=%d",
		md->x, md->y, md->state, md->clicks));

	/* Ozk 040503: Detect a button-released situation, and let active-widget get inactive */

	if (C.menu_base && md->state)
	{
		client = C.menu_base->client;
		DIAG((D_mouse, client, "post button event (menu) to %s", client->name));
		post_cevent(client, cXA_button_event, 0, 0, 0, 0, 0, md);
		return;
	}

	locker = mouse_locked();
	if (!locker)
		locker = update_locked();
	if (locker && md->state)
	{
		if (locker->fmd.lock && locker->fmd.mousepress)
		{
			DIAG((D_mouse, locker, "post form do to %s", locker->name));
			post_cevent(locker, cXA_form_do, 0, 0, 0, 0, 0, md);
			return;
		}
	}

	wind = find_window(lock, md->x, md->y);
	if (wind)
	{
		client = wind->owner;
		DIAG((D_mouse, client, "post button event (wind) to %s", client->name));
		post_cevent(client, cXA_button_event, wind, 0, 0, 0, 0, md);
		return;
	}
}

int
XA_move_event(enum locks lock, const struct moose_data *md)
{
	struct xa_client *client;
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
		widget_active.cb = md->state;
		client = widget_active.wind->owner;
		DIAG((D_mouse, client, "post active widget (move) to %s", client->name));
		post_cevent(client, cXA_active_widget, 0,0, 0,0, 0, md);
		return false;
	}

	if (C.menu_base)
	{
		client = C.menu_base->client;
		DIAG((D_mouse, client, "post menumove to %s", client->name));
		post_cevent(client, cXA_menu_move, 0,0, 0,0, 0, md);
		return false;
	}

	Sema_Up(clients);

	/* Moving the mouse into the menu bar is outside
	 * Tab handling, because the bar itself is not for popping.
	 */
	{
		/* HR: watch the menu bar as a whole */
		struct xa_client *aesp = C.Aes;

		if (   (aesp->waiting_for & XAWAIT_MENU)
		    && (aesp->em.flags & MU_M1))
		{
			if (   cfg.menu_behave != PUSH
			    && !update_locked()
			    && is_rect(x, y, aesp->em.flags & 1, &aesp->em.m1))
			{
				XA_WIDGET *widg = get_widget(root_window, XAW_MENU);
				XA_TREE *menu;

				menu = (XA_TREE *)widg->stuff;
				client = menu->owner;
				DIAG((D_mouse, client, "post widgclick (menustart) to %s", client->name));
				post_cevent(client, cXA_widget_click, widg, 0,0,0,0,md);
				return false;
			}
		}
	}


	/* mouse lock is also for rectangle events! */
	if (mouse_locked())
	{
		client = mouse_locked();
		if (!client)
			return false;
	}
	else
		client = S.client_list;

	/* internalized the client loop */
	while (client)
	{
		if (client->waiting_for & (MU_M1|MU_M2|MU_MX))
		{
			AESPB *pb = client->waiting_pb;

			/* combine mouse events. */
			pb->intout[0] = 0;

			if (   (client->em.flags & MU_M1)
			    && is_rect(x, y, client->em.flags & 1, &client->em.m1))
			{
				if (client->waiting_for & XAWAIT_MULTI)
				{
					DIAG((D_mouse,client,"MU_M1 for %s", c_owner(client))); 
					multi_intout(client, pb->intout, pb->intout[0] | MU_M1);
				}
				else
				{
					multi_intout(client, pb->intout, 0);
					pb->intout[0] = 1;
				}
			}

			if (   (client->em.flags & MU_M2)		/* M2 in evnt_multi only */
			    && is_rect(x, y, client->em.flags & 2, &client->em.m2))
			{
				DIAG((D_mouse,client,"MU_M2 for %s", c_owner(client))); 
				multi_intout(client, pb->intout, pb->intout[0] | MU_M2);
			}

			if (client->em.flags & MU_MX)			/* MX: any movement. */
			{
				DIAG((D_mouse,client,"MU_MX for %s", c_owner(client))); 
				multi_intout(client, pb->intout, pb->intout[0] | MU_MX);
			}

			if (pb->intout[0])
			{
				client->usr_evnt = 1;
				Unblock(client, XA_OK, 5);
			}
		}

		if (mouse_locked())
			break;

		client = client->next;
	}

	Sema_Dn(clients);

	return false;
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
find_focus(bool *waiting, struct xa_client **locked_client)
{
	struct xa_window *top = window_list;
	struct xa_client *client, *locked = NULL;

#if GENERATE_DIAGS
	if (C.focus == root_window)
	{
		DIAGS(("C.focus == root_window"));
	}
#endif
	if (top == C.focus && top->keypress)
	{
		/* this is for windowed form_do which doesn't
		 * set the update lock.
		 */
		*waiting = true;
		DIAGS(("-= 1 =-"));
		return top->owner;
	}

	/* HR 141201: special case, no menu bar, possibly no windows either
	 * but a dialogue on the screen, not governed by form_do. (handled above)
	 * The client must also be waiting.
	 */
	if (update_locked())
	{
		locked = update_locked();

		*locked_client = locked;
		DIAGS(("-= 2 =-"));
	}
	else if (mouse_locked())
	{
		locked = mouse_locked();

		*locked_client = locked;
		DIAGS(("-= 3 =-"));
	}

	if (locked)
	{
		client = locked;
		if (client->fmd.keypress) /* HR 250602 classic (blocked) form_do */
		{
			*waiting = true;
			DIAGS(("-= 4 =-"));
			return client;
		}

		if ((client->waiting_for & (MU_KEYBD|MU_NORM_KEYBD)) != 0 || top->keypress != NULL)
		{
			*waiting = true;
			DIAGS(("-= 5 =-"));
			return client;
		}
	}

	/* HR 131202: removed some spuriosuty and unclear stuff (things got too complex) */
	/* If C.focus == rootwindow, then the top_window owner is not the menu owner;
	                 the menu has prcedence, and the top window isnt drawn bold. */
	client = focus_owner();
	*waiting = (client->waiting_for & (MU_KEYBD|MU_NORM_KEYBD)) != 0 || top->keypress != NULL;
	DIAGS(("-= 9 =-"));

	return client;
}


static XA_WIDGET *
wheel_arrow(struct xa_window *wind, const struct moose_data *md)
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
XA_wheel_event(enum locks lock, const struct moose_data *md)
{
	struct xa_window *wind = window_list;
	struct xa_client *client = NULL;
	XA_WIDGET *widg = wheel_arrow(wind, md);
	int n,c;

	DIAGS(("mouse wheel %d has wheeled %d", md->state, md->clicks));

	client = mouse_locked();

	if (   ( client && widg && wind->send_message && wind->owner == client)
	    || (!client && widg && wind->send_message))
	{
		DIAGS(("found widget %d", widg->type));
		client = wind->owner;
		if (client->wa_wheel || wind->wa_wheel)
		{
			DIAGS(("clwa %d, wiwa %d", client->wa_wheel, wind->wa_wheel));
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
		DIAGS(("wheel event for %s, waiting %d",
				c_owner(client),client->waiting_for));

#if 0
		if (client->fmd)
			 /* Might be a model dialogue; implement at this point . */
		else
#endif
		if (client->waiting_for & MU_WHEEL)
		{
			AESPB *pb = client->waiting_pb;

			if (pb)
			{
				multi_intout(client, pb->intout, MU_WHEEL);
				pb->intout[4] = md->state;
				pb->intout[6] = md->clicks;

				Unblock(client, XA_OK, 3);
				DIAGS((" - written"));
			}
		}
	}
}

static void
new_mu_mouse(struct moose_data *md)
{
	DIAG((D_v, NULL, "new_mu_mouse %d %d,%d/%d", md->state, md->cstate, md->x, md->y));
	mu_button.b		= md->state;
	mu_button.cb		= md->cstate;
	mu_button.x = x_mouse	= md->x;
	mu_button.y = y_mouse	= md->y;
	mu_button.clicks 	= md->clicks;
	vq_key_s(C.vh, &mu_button.ks);
	mu_button.got		= false;

	/*
	 * Copy into the global main moose data if necessary
	 */
	if (md != &mainmd)
		mainmd = *md;

}

static void
new_active_widget_mouse(void)
{
	widget_active.b		= mu_button.b;
	widget_active.cb	= mu_button.cb;
	widget_active.nx	= mu_button.x;
	widget_active.ny	= mu_button.y;
	widget_active.clicks	= mu_button.clicks;
}

/*
 * Here we decide what to do with a new moose packet.
 * Separeted to make it possible to send moose packets from elsewhere...
 */
static int
new_moose_pkt(enum locks lock, int internal, struct moose_data *md)
{
	/* exclusive mouse input */
	if (internal || (S.wait_mouse && (S.wait_mouse->waiting_for & XAWAIT_MOUSE)))
	{
		/* a client wait exclusivly for the mouse */
		short *data = S.wait_mouse->waiting_short;

		if (md->ty == MOOSE_BUTTON_PREFIX)
		{
			new_mu_mouse(md);
			data[0] = md->state;
		}
		else
			data[0] = mu_button.b;
			
		data[1] = md->x;
		data[2] = md->y;

		if (!internal)
			Unblock(S.wait_mouse, XA_OK, 3);

		return true;
	}

	/* Mouse data packet type */
	switch (md->ty)
	{
	case MOOSE_BUTTON_PREFIX:
	{
		DIAG((D_button, NULL, "Button %d, cstate %d on: %d/%d",
			md->state, md->cstate, md->x, md->y));

		/*
		 * Ozk: Moved the checks for fake button-released elsewhere,
		 * new_moose_pkt unconditionally sends only received packet.
		 */
		new_mu_mouse(md);
		new_active_widget_mouse();
		XA_button_event(lock, md, true);

		/* Ozk: button.got is now used as a flag indicating
		 * whether or not a mouse-event packet has been
		 * delivered, queued as pending or not.
		 */
		mu_button.got = true;

		break;
	}
	case MOOSE_MOVEMENT_PREFIX:
	{
		/* mouse rectangle events */
		/* DIAG((D_v, NULL,"mouse move to: %d/%d", mdata.x, mdata.y)); */

		/* Call the mouse movement event handler (doesnt use md->state) */
		x_mouse = md->x;
		y_mouse = md->y;
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
		DIAG((D_mouse, NULL, "Unknown mouse datatype (0x%x)", md->ty));
		DIAG((D_mouse, NULL, " l=0x%x, ty=%d, x=%d, y=%d, state=0x%x, clicks=%d",
			md->l, md->ty, md->x, md->y, md->state, md->clicks));
		DIAG((D_mouse, NULL, " dbg1=0x%x, dbg2=0x%x",
			md->dbg1, md->dbg2));

		return false;
	}
	}
	return true;
}

void
preprocess_mouse(enum locks lock)
{
	/*
	 * Check if a fake button-released packet needs to be sent
	 */
	if (mainmd.state && !mainmd.cstate)
	{
		mainmd.state = mainmd.cstate = mainmd.clicks = 0;
		mainmd.x = mu_button.x;
		mainmd.y = mu_button.y;

		new_moose_pkt(lock, 0, &mainmd);
	}

	/* no_mouse */
	mu_button.have = false;
}

int
mouse_input(enum locks lock, int internal)
{
	struct moose_data md;
	long n;

	/* Read always whole packets, otherwise loose
	 * sync. For now ALL packets are same size,
	 * faster anyhow.
	 * BTW. The record length can still be used
	 * to decide on the amount to read.
	 * 
	 * Ozk:
	 * Added 'cstate', in addition to 'state' to the moose_data structure.
	 *'state' indicates which button(s) triggered the event,
	 *'cstate' contains the button state at the time when
	 * double-click timer expires.
	 * Makes it much easier, yah know ;-)
	 *
	 * Ozk: The moose structure lives in global space now because the
	 * wait_mouse/check_mouse needs to know if a fake button-released
	 * packet is in the works.
	 *
	 * Ozk: If we get a button packet in which the button
	 * state at the time moose sent it is 'released', we
	 * need to fake a 'button-released' event.
	 * If state == cstate != 0, moose will send a 'button-released'
	 * packet.
	 */ 
	if (mainmd.state && !mainmd.cstate)
	{
		DIAG((D_button, NULL, "Sending fake button released"));

		mainmd.state	= 0;
		mainmd.cstate	= 0;
		mainmd.clicks	= 0;
		mainmd.x	= mu_button.x;
		mainmd.y	= mu_button.y;

		return new_moose_pkt(lock, internal, &mainmd);
	}
	else
	{
		n = f_read(C.MOUSE_dev, sizeof(md), &md);
		if (n == sizeof(md))
			return new_moose_pkt(lock, internal, &md);
	}

	/* DIAG((D_mouse, NULL, "Moose channel yielded %ld", n)); */
	return false;
}

/*
 * blocks until mouse input
 * context safe; scheduler called 
 */
void
wait_mouse(struct xa_client *client, short *br, short *xr, short *yr)
{
	short data[3];

	/*
	 * Ozk: Make absolutely sure wether we're the AES kernel or a user
	 * application.
	 */
	if (C.Aes->p == get_curproc())
	{
		/* AESSYS internal -> poll mouse */

		DIAGS(("wait_mouse for XaAES"));

		client->waiting_for |= XAWAIT_MOUSE;
		client->waiting_short = data;

		/* only one client can exclusivly wait for the mouse */
		assert(S.wait_mouse == NULL);

		S.wait_mouse = client;

		while (!mouse_input(NOLOCKS/*XXX*/, true))
			yield();

		S.wait_mouse = NULL;

		client->waiting_for &= ~XAWAIT_MOUSE;
		client->waiting_short = NULL;
	}
	else
	{
		/* wait for input from AESSYS */

		DIAGS(("wait_mouse for %s", client->name));

		client->waiting_for |= XAWAIT_MOUSE;
		client->waiting_short = data;

		/* only one client can exclusivly wait for the mouse */
		assert(S.wait_mouse == NULL);

		S.wait_mouse = client;
		Block(client, 2);
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
void
check_mouse(struct xa_client *client, short *br, short *xr, short *yr)
{
	struct moose_data md;
	//short b, x, y;

	/*
	 * First we check if a fake button-released packet is to be delivered
	 */
	DIAGS(("check_mouse for %s", client->name));

	if (mainmd.state && !mainmd.cstate)
	{
		DIAG((D_button, NULL, "check_mouse: Sending fake button released"));
		mainmd.state = 0;
		mainmd.cstate = 0;
		mainmd.x = mu_button.x;
		mainmd.y = mu_button.y;
		mainmd.clicks = 0;
		new_mu_mouse(&mainmd);
	}
	else
	{
		/*
		 * If no fake packet is need, try to read to get most recent
		 * data. If that fails, the data already in mu_mouse is uptodate.
		 */
		long n;

		n = kernel_read(C.kmoose, &md, sizeof(md));
		if (n == sizeof(md))
		{
			/*
			 * remember that mu_button only changes when button status change
			 * so if only movement, dont call new_mu_mouse();
			 */
			DIAG((D_mouse, NULL, "check_mouse - reading"));
			switch (md.ty)
			{
				case MOOSE_BUTTON_PREFIX:
				{
					new_mu_mouse(&md);
					break;
				}
				case MOOSE_MOVEMENT_PREFIX:
				case MOOSE_WHEEL_PREFIX:
				{
					x_mouse = md.x;
					y_mouse = md.y;
					break;
				}
				default:
				{
					DIAG((D_mouse, NULL, "check_mouse: Unknown mouse datatype (0x%x)", mainmd.ty));
					DIAG((D_mouse, NULL, " l=0x%x, ty=%d, x=%d, y=%d, state=0x%x, clicks=%d",
						mainmd.l, mainmd.ty, mainmd.x, mainmd.y, mainmd.state, mainmd.clicks));
					DIAG((D_mouse, NULL, " dbg1=0x%x, dbg2=0x%x",
						mainmd.dbg1, mainmd.dbg2));
					break;
				}
			}
		}
		else
		{
			/*
			 * No mouse news today...
			 */
			DIAGS(("check_mouse - no new data %lx", n));
		}
	}

	DIAG((D_mouse, NULL, "check_mouse - return %d, %d.%d for %s",
		mu_button.b, x_mouse, y_mouse, client->name));

	if (br)
		*br = mu_button.b;
	if (xr)
		*xr = x_mouse;
	if (yr)
		*yr = y_mouse;
}
