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

#include "xa_evnt.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "k_keybd.h"
#include "k_main.h"
#include "k_mouse.h"
#include "menuwidg.h"
#include "messages.h"
#include "nkcc.h"
#include "rectlist.h"
#include "widgets.h"


static int
pending_critical_msgs(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;

	Sema_Up(clients);

	msg = client->crit_msg;
	if (msg)
	{
		union msg_buf *buf = (union msg_buf *)pb->addrin[0];

		/* dequeue */
		client->crit_msg = msg->next;

		/* write to client */
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending critical message %s for %s from %d - %d,%d,%d,%d,%d",
			pmsg(buf->m[0]), c_owner(client), buf->m[1],
			buf->m[3], buf->m[4], buf->m[5], buf->m[6], buf->m[7]));

		kfree(msg);
		rtn = 1;
	}

	Sema_Dn(clients);
	return rtn;
}

static int
pending_redraw_msgs(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;

	Sema_Up(clients);

	msg = client->rdrw_msg;
	if (msg)
	{
		union msg_buf *buf = (union msg_buf *)pb->addrin[0];

		/* dequeue */
		client->rdrw_msg = msg->next;

		/* write to client */
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending WM_REDRAW (%lx (wind=%d, %d/%d/%d/%d)) for %s",
			msg, buf->m[3], *(RECT *)&buf->m[4], c_owner(client) ));

		kfree(msg);
		rtn = 1;

		if (--C.redraws)
			kick_mousemove_timeout();

		//C.redraws--;		
	}

	Sema_Dn(clients);
	return rtn;
}

static int
pending_msgs(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;

	Sema_Up(clients);

	msg = client->msg;
	if (msg)
	{
		union msg_buf *buf = (union msg_buf *)pb->addrin[0];

		/* dequeue */
		client->msg = msg->next;

		/* write to client */
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending message %s for %s from %d - %d,%d,%d,%d,%d",
			pmsg(buf->m[0]), c_owner(client), buf->m[1],
			buf->m[3], buf->m[4], buf->m[5], buf->m[6], buf->m[7]));

		kfree(msg);
		rtn = 1;
	}

	Sema_Dn(clients);
	return rtn;
}

/* Note that it might still be necessary to reorganize the kernel loop! */
static bool
pending_key_strokes(enum locks lock, AESPB *pb, struct xa_client *client, int type)
{
	bool ok = false, waiting = false;

	Sema_Up(pending);

	/* keys may be queued */
	if (pending_keys.cur != pending_keys.last)
	{
		IFDIAG(struct xa_client *qcl = pending_keys.q[pending_keys.cur].client;)
		struct xa_client *locked = NULL;
		struct xa_client *foc = find_focus(true, &waiting, &locked, NULL);
		struct rawkey key;

		DIAG((D_keybd, NULL, "Pending key: cur=%d,end=%d (qcl%d::cl%d::foc%d::lock%d)",
			pending_keys.cur, pending_keys.last,
			qcl->p->pid, client->p->pid, foc->p->pid, locked ? locked->p->pid : -1));		

		if (client == foc)
		{
			DIAG((D_keybd, NULL, "   --   Gotcha!"));

			key = pending_keys.q[pending_keys.cur].k;
	
			if (type)
			{
				mainmd.kstate = key.raw.conin.state;

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
	
			DIAG((D_keybd, NULL, "key 0x%x sent to %s",
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

static bool
mouse_ok(struct xa_client *client)
{
#if GENERATE_DIAGS
	if (mouse_locked())
	{
		DIAG((D_sema,client,"Mouse OK? %d pid = %d",
			mouse_locked()->p->pid, client->p->pid));
	}
#endif

	if (!mouse_locked())
		return true;

	if (mouse_locked() == client)
		return true;

	/* mouse locked by another client */
	return false;
}

#if GENERATE_DIAGS
static char *xev[] = {"KBD","BUT","M1","M2","MSG","TIM","WHL","MX","NKBD","9","10","11","12","13","14","15"};

static void
evnt_diag_output(void *pb, struct xa_client *client, char *which)
{
	if (pb)
	{
		short *o = ((AESPB *)pb)->intout;
		char evx[128];
		show_bits(o[0], "", xev, evx);
		DIAG((D_multi, client, "%sevnt_multi return: %s x%d, y%d, b%d, ks%d, kc%d, mc%d",
			which, evx, o[1], o[2], o[3], o[4], o[5], o[6]));
	}
}
#define diag_out(x,c,y) evnt_diag_output(x,c,y);

#if 0
static char *
em_flag(int f)
{
	static char *mo[6] = { "into", "outof", "into", "outof" };
	return mo[f & 3];
}
#endif

#else /* GENERATE_DIAGS */

#define diag_out(x,c,y)

#endif

bool
check_queued_events(struct xa_client *client)
{
	short events = 0;
	short clicks = 0, mbutts = 0, mx, my, ks;
	bool multi = client->waiting_for & XAWAIT_MULTI ? true : false;
	AESPB *pb;
	short *out;
	
	if (!(pb = client->waiting_pb))
	{
		DIAG((D_appl, NULL, "WARNING: Invalid target message buffer for %s", client->name));
		return false;
	}

	out = pb->intout;

	check_mouse(client, NULL, &mx, &my);
	vq_key_s(C.vh, &ks);

	if ((client->waiting_for & MU_MESAG) && (client->msg || client->rdrw_msg || client->crit_msg))
	{
		union msg_buf *cbuf;

		cbuf = (union msg_buf *)pb->addrin[0];
		if (cbuf)
		{
			if (!pending_critical_msgs(0, client, pb))
			{
				if (!pending_redraw_msgs(0, client, pb))
					pending_msgs(0, client, pb);
			}

			if (client->waiting_for & XAWAIT_MULTI)
			{
				events |= MU_MESAG;
			}
			else
			{
				*out = 1;
				goto got_evnt;
			}
		}
		else
		{
			DIAG((D_m, NULL, "MU_MESAG and NO PB! for %s", client->name));
			return false;
		}
	}

	if ((client->waiting_for & MU_BUTTON))
	{
		bool bev = false;
		const short *in = multi ? pb->intin + 1 : pb->intin;

		DIAG((D_button, NULL, "still_button multi?? o[0,2] "
			"%x,%x, lock %d, Mbase %lx, active.widg %lx",
			pb->intin[1], pb->intin[3],
			mouse_locked() ? mouse_locked()->p->pid : 0,
			C.menu_base, widget_active.widg));

		DIAG((D_button, NULL, " -=- md: clicks=%d, head=%lx, tail=%lx, end=%lx",
			client->md_head->clicks, client->md_head, client->md_tail, client->md_end));

		clicks = client->md_head->clicks;
				
		if (clicks == -1)
		{
			if (client->md_head != client->md_tail)
			{
				client->md_head++;
				if (client->md_head > client->md_end)
					client->md_head = client->mdb;
				clicks = client->md_head->clicks;
				DIAG((D_button, NULL, "Next packet %lx (s=%d, cs=%d)",
					client->md_head, client->md_head->state, client->md_head->cstate));
			}
			else
			{
				DIAG((D_button, NULL, "Used packet %lx (s=%d, cs=%d)",
					client->md_head, client->md_head->state, client->md_head->cstate));
				clicks = 0;
			}
		}

		if (clicks)
		{
			DIAG((D_button, NULL, "New event %lx (s=%d, cs=%d)",
				client->md_head, client->md_head->state, client->md_head->cstate));
			if ((bev = is_bevent(client->md_head->state, clicks, in, 1)))
			{
				mbutts = client->md_head->state;
				mx = client->md_head->x;
				my = client->md_head->y;
				ks = client->md_head->kstate;
			}
			client->md_head->clicks = 0;
		}
		else
		{
			DIAG((D_button, NULL, "old event %lx (s=%d, cs=%d)",
				client->md_head, client->md_head->state, client->md_head->cstate));
			if ((bev = is_bevent(client->md_head->cstate, clicks, in, 1)))
			{
				mbutts = client->md_head->cstate;
				clicks = 1;			/* Turns out we have to lie about clicks .. ARGHHH! */
			}
			client->md_head->clicks = -1;
		}
		if (bev)
		{
			if (multi)
				events = MU_BUTTON;
			else
			{
				*out++ = clicks;
				*out++ = mx;
				*out++ = my;
				*out++ = mbutts;
				*out++ = ks;
				goto got_evnt;
			}
		}
	}

	if (events)
	{

#if GENERATE_DIAGS
		{
			char evtxt[128];
			show_bits(events, "evnt=", xev, evtxt);
			DIAG((D_multi,client,"check_queued_events for %s, %s clks=0x%x, msk=0x%x, bst=0x%x T:%d",
				c_owner(client),
				evtxt,pb->intin[1],pb->intin[2],pb->intin[3], (events&MU_TIMER) ? pb->intin[14] : -1));
			DIAG((D_multi, client, "status %lx, %lx, C.redraws %ld", client->status, client->rdrw_msg, C.redraws));
		}
#endif
		*out++ = events;
		*out++ = mx;
		*out++ = my;
		*out++ = mbutts;
		*out++ = ks;
		*out++ = 0;
		*out++ = clicks;
	}
	else
		return false;

got_evnt:
	client->usr_evnt = 1;
	cancel_evnt_multi(client, 222);
	return true;
}


/* HR 070601: We really must combine events. especially for the button still down situation.
*/

/*
 * The essential evnt_multi() call
 */
unsigned long
XA_evnt_multi(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short events = pb->intin[0];
	short x, y, mx, my;
	unsigned long ret = XAC_BLOCK;		/* HR: another example of choosing inconvenient default fixed. */
	short new_waiting_for = 0,
	    fall_through = 0,
	    clicks = 0, ks, mbutts = 0;
	
	CONTROL(16,7,1)

	pb->intout[0] = 0;
	pb->intout[5] = 0;
	pb->intout[6] = 0;

#if GENERATE_DIAGS
	{
		char evtxt[128];
		show_bits(events, "evnt=", xev, evtxt);
		DIAG((D_multi,client,"evnt_multi for %s, %s clks=0x%x, msk=0x%x, bst=0x%x T:%d",
				c_owner(client),
				evtxt,pb->intin[1],pb->intin[2],pb->intin[3], (events&MU_TIMER) ? pb->intin[14] : -1));
		DIAG((D_multi, client, "status %lx, %lx, C.redraws %ld", client->status, client->rdrw_msg, C.redraws));
	}
#endif

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_multi: %s flagged as lagging! - cleared", client->name));
	}
	
	/*
	 * Ozk: We absolutely prioritize WM_REDRAW messages, which are
	 * queued in a separate message queue.
	*/
	if (events & MU_MESAG)
	{
		short e = 0;

		if ( pending_critical_msgs(lock, client, pb) )
		{
			e = MU_MESAG;
		}
		else if ( pending_redraw_msgs(lock, client, pb) )
		{
			e = MU_MESAG;
			/*
			 * Ozk: If this was the last redraw message,
			 * reenable delivery of mouse-movement events.
			*/
			if (!C.redraws)
				kick_mousemove_timeout();
		}
		if (e)
		{
			multi_intout(client, pb->intout, 0);
			pb->intout[0] = e;
			return XAC_DONE;
		}
	}

	/*
	 * Ozk: If there are still redraw messages (for other clients than "us"),
	 * we yield() so that redraws are processed as soon as possible.
	*/
	if (C.redraws)
		yield();

	client->waiting_for = events | XAWAIT_MULTI;
	client->waiting_pb = pb;
	if (check_cevents(client))
		return XAC_DONE;
	client->waiting_for = 0;
	client->waiting_pb = NULL;

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

	check_mouse(client, NULL, &x, &y);
	vq_key_s(C.vh, &ks);
	mx = x, my = y;

	if (events & MU_BUTTON)
	{
		{
			bool bev = false;

			DIAG((D_button, NULL, "still_button multi?? o[0,2] "
				"%x,%x, lock %d, Mbase %lx, active.widg %lx",
				pb->intin[1], pb->intin[3],
				mouse_locked() ? mouse_locked()->p->pid : 0,
				C.menu_base, widget_active.widg));

			DIAG((D_button, NULL, " -=- md: clicks=%d, head=%lx, tail=%lx, end=%lx",
				client->md_head->clicks, client->md_head, client->md_tail, client->md_end));
			{

				clicks = client->md_head->clicks;
				
				if (clicks == -1)
				{
					if (client->md_head != client->md_tail)
					{
						client->md_head++;
						if (client->md_head > client->md_end)
							client->md_head = client->mdb;
						clicks = client->md_head->clicks;
						DIAG((D_button, NULL, "Next packet %lx (s=%d, cs=%d)",
							client->md_head, client->md_head->state, client->md_head->cstate));
					}
					else
					{
						DIAG((D_button, NULL, "Used packet %lx (s=%d, cs=%d)",
							client->md_head, client->md_head->state, client->md_head->cstate));
						clicks = 0;
					}
				}

				if (clicks)
				{
					DIAG((D_button, NULL, "New event %lx (s=%d, cs=%d)",
						client->md_head, client->md_head->state, client->md_head->cstate));
					if ((bev = is_bevent(client->md_head->state, clicks, pb->intin + 1, 1)))
					{
						mbutts = client->md_head->state;
						mx = client->md_head->x;
						my = client->md_head->y;
						ks = client->md_head->kstate;
					}
					client->md_head->clicks = 0;
				}
				else
				{
					DIAG((D_button, NULL, "old event %lx (s=%d, cs=%d)",
						client->md_head, client->md_head->state, client->md_head->cstate));
					if ((bev = is_bevent(client->md_head->cstate, clicks, pb->intin + 1, 1)))
					{
						mbutts = client->md_head->cstate;
						mx = x, my = y;
						clicks = 1;			/* Turns out we have to lie about clicks .. ARGHHH! */
					}
					client->md_head->clicks = -1;
				}
			}
			if (bev)
				fall_through |= MU_BUTTON;
			else
			{
				new_waiting_for |= MU_BUTTON;
				//mx = x, my = y;
				DIAG((D_b, client, "new_waiting_for |= MU_BUTTON"));
			}
		}
	}
#if 0
	else
	{
		mx = x;
		my = y;
	}
#endif
	if (events & (MU_NORM_KEYBD|MU_KEYBD))		
	{
		short ev = events&(MU_NORM_KEYBD|MU_KEYBD);
		if (pending_key_strokes(lock, pb, client, ev))
			fall_through    |= ev;
		else
			/* Flag the app as waiting for keypresses */
			new_waiting_for |= ev;
	}

	if (events & (MU_M1|MU_M2|MU_MX))
	{
		struct xa_window *wind;
		struct xa_client *wo = NULL, *locker;

		bzero(&client->em, sizeof(client->em));

		/* Ozk:
		 *   MU_Mx events are delivered if;
		 *   1. Client is the holder of either update or mouse lock
		 *   2. The event happens over one of its windows and that window
		 *      is either ontop or WF_BEVENT'ed.
		 *
		 */
		locker = mouse_locked();
		if (!locker)
			locker = update_locked();

		wind = find_window(0, x, y);
		if (wind)
			wo = wind == root_window ? get_desktop()->owner : wind->owner;

		if ( (locker && locker == client) ||
		     (is_infront(client) || !wo || (wo && wo == client && (is_topped(wind) || wind->active_widgets & NO_TOPPED))) )
		{
			if (events & MU_M1)
			{
				const RECT *r = (const RECT *)&pb->intin[5];

				client->em.m1 = *r;
				client->em.flags = pb->intin[4] | MU_M1;

				if (mouse_ok(client) && is_rect(x, y, client->em.flags & 1, &client->em.m1))
					fall_through |= MU_M1;
				else
					new_waiting_for |= MU_M1;
			}
			if (events & MU_MX)
			{
				client->em.flags = pb->intin[4] | MU_MX;
				new_waiting_for |= MU_MX;
			}
			if (events & MU_M2)
			{
				const RECT *r = (const RECT *)&pb->intin[10];

				client->em.m2 = *r;
				client->em.flags |= (pb->intin[9] << 1) | MU_M2;

				if (mouse_ok(client) && is_rect(x, y, client->em.flags & 2, &client->em.m2))
					fall_through |= MU_M2;
				else
					new_waiting_for |= MU_M2;
			}
		}
		else
			new_waiting_for |= (events & (MU_M1 | MU_M2 | MU_MX));
	}

	/* AES 4.09 */
	if (events & MU_WHEEL)
	{
		DIAG((D_i,client,"    MU_WHEEL"));
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
		DIAG((D_i,client,"Timer val: %ld(hi=%d,lo=%d)",
			client->timer_val, pb->intin[15], pb->intin[14]));
		if (client->timer_val)
		{
			new_waiting_for |= MU_TIMER;	/* Flag the app as waiting for a timer */
			ret = XAC_TIMER;
		}
		else
		{
			/* Is this the cause of loosing the key's at regular intervals? */
			DIAG((D_i,client, "Done timer for %d", client->p->pid));

			/* If MU_TIMER and no timer (which means, return immediately),
			 * we yield()
			 */
			yield();
			fall_through |= MU_TIMER;
			/* HR 190601: to be able to combine fall thru events. */
			ret = XAC_DONE;
		}
	}

	if (fall_through)
	{
		if (!(fall_through & MU_TIMER))
			ret = XAC_DONE;
		
		pb->intout[0] = fall_through;
		pb->intout[1] = mx;
		pb->intout[2] = my;
		pb->intout[3] = mbutts;
		pb->intout[4] = ks;
	     /* pb->intout[5] is AES key if MU_KEYBD/MU_NORM_KEYBD - filled by pending_key_strokes above */
		pb->intout[6] = clicks;
		diag_out(pb,client,"fall_thru ");
	}
	else if (new_waiting_for)
	{
		pb->intout[0] = 0;

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
cancel_evnt_multi(struct xa_client *client, int which)
{
	client->waiting_for = 0;
	client->em.flags = 0;
	client->waiting_pb = NULL;
	DIAG((D_kern,NULL,"[%d]cancel_evnt_multi for %s", which, c_owner(client)));
}

/*
 * AES message handling
 */
unsigned long
XA_evnt_mesag(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,1)

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_mesag: %s flagged as lagging! - cleared", client->name));
	}
	/*
	 * Ozk: look at XA_evnt_multi() for explanations..
	*/
	if (pending_critical_msgs(lock, client, pb))
	{
		pb->intout[0] = 1;
		return XAC_DONE;
	}
	if (pending_redraw_msgs(lock, client, pb))
	{
		pb->intout[0] = 1;
		if (!C.redraws)
			kick_mousemove_timeout();

		return XAC_DONE;
	}
	if (C.redraws)
		yield();

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
XA_evnt_button(enum locks lock, struct xa_client *client, AESPB *pb)
{

	CONTROL(3,5,1)

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_button: %s flagged as lagging! - cleared", client->name));
	}

	DIAG((D_button,NULL,"evnt_button for %s; clicks %d mask 0x%x state 0x%x",
	           c_owner(client), pb->intin[0], pb->intin[1], pb->intin[2]));

	Sema_Up(pending);

	{
		short clicks, ks, mbutts, mx, my;
		bool bev = false;

		DIAG((D_button, NULL, "still_button? o[0,2] "
			"%x,%x, lock %d, Mbase %lx, active.widg %lx",
			pb->intin[0], pb->intin[2],
			mouse_locked() ? mouse_locked()->p->pid : 0,
			C.menu_base, widget_active.widg));

		DIAG((D_button, NULL, " -=- md: clicks=%d, head=%lx, tail=%lx, end=%lx",
			client->md_head->clicks, client->md_head, client->md_tail, client->md_end));

		{
			clicks = client->md_head->clicks;
				
			if (clicks == -1)
			{
				if (client->md_head != client->md_tail)
				{
					client->md_head++;
					if (client->md_head > client->md_end)
						client->md_head = client->mdb;
					clicks = client->md_head->clicks;
				}
				else
					clicks = 0;
			}

			if (clicks)
			{
				if ((bev = is_bevent(client->md_head->state, clicks, pb->intin, 1)))
				{
					mbutts = client->md_head->state;
					mx = client->md_head->x;
					my = client->md_head->y;
					ks = client->md_head->kstate;
				}
				client->md_head->clicks = 0;
			}
			else
			{
				if ((bev = is_bevent(client->md_head->cstate, clicks, pb->intin, 1)))
				{
					mbutts = client->md_head->cstate;
					check_mouse(client, NULL, &mx, &my);
					vq_key_s(C.vh, &ks);
					clicks = 1;			/* Turns out we have to lie about clicks ... ARGHH! */
				}
				client->md_head->clicks = -1;
			}
		}
		if (bev)
		{
			DIAG((D_button, NULL, "evnt_multi: Check if button event"));
			pb->intout[0] = clicks;
			pb->intout[1] = mx;
			pb->intout[2] = my;
			pb->intout[3] = mbutts;
			pb->intout[4] = ks;
			Sema_Dn(pending);
			return XAC_DONE;
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
XA_evnt_keybd(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,0)

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_keybd: %s flagged as lagging! - cleared", client->name));
	}
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
XA_evnt_mouse(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short x, y;
	struct xa_window *wind;
	struct xa_client *wo = NULL, *locker;

	CONTROL(5,5,0)

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_mouse: %s flagged as lagging! - cleared", client->name));
	}
	/* Flag the app as waiting for messages */
	client->waiting_for = MU_M1;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_pb = pb;

	bzero(&client->em, sizeof(client->em));
	client->em.m1 = *((const RECT *) &pb->intin[1]);
	client->em.flags = (long)(pb->intin[0]) | MU_M1;

	locker = mouse_locked();
	if (!locker)
		locker = update_locked();

	check_mouse(client, NULL, &x, &y);
	wind = find_window(0, x, y);
	if (wind)
		wo = wind == root_window ? get_desktop()->owner : wind->owner;
	if ( (locker && locker == client) ||
	     (is_infront(client) || !wo || (wo && wo == client && (is_topped(wind) || wind->active_widgets & NO_TOPPED))) )
	{
		if (mouse_ok(client) && is_rect(x, y, client->em.flags & 1, &client->em.m1))
		{
			multi_intout(client, pb->intout, 0);
			pb->intout[0] = 1;
			return XAC_DONE;
		}
	}

	/* Returning false blocks the client app to wait for the event */
	return XAC_BLOCK;
}

/*
 * Evnt_timer()
 */
unsigned long
XA_evnt_timer(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(2,1,0)

	if (!(client->timer_val = ((long)pb->intin[1] << 16) | pb->intin[0]))
	{
		yield();
		return XAC_DONE;
	}
	else
	{
		/* Flag the app as waiting for messages */
		client->waiting_pb = pb;
		/* Store a pointer to the AESPB to fill when the event occurs */
		client->waiting_for = MU_TIMER;

		return XAC_TIMER;
	}
}
