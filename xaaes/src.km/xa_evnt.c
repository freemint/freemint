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
#include "objects.h"
#include "rectlist.h"
#include "widgets.h"

//long redraws = 0;

static int
pending_redraw_msgs(enum locks lock, struct xa_client *client, AESPB *pb)
{
	int rtn = 0;
	struct xa_aesmsg_list *msg = client->rdrw_msg;

	Sema_Up(clients);

	if (msg)
	{
		union msg_buf *buf = (union msg_buf *)pb->addrin[0];

		client->rdrw_msg = msg->next;
		*buf = msg->message;
		DIAG((D_m, NULL, "Got pending WM_REDRAW for %s", c_owner(client) ));
		kfree(msg);
		rtn = 1;
		C.redraws--;
	}
	Sema_Dn(clients);
	return rtn;
}

static int
pending_msgs(enum locks lock, struct xa_client *client, AESPB *pb)
{
	int rtn = 0;
	struct xa_aesmsg_list *msg = client->msg;

	Sema_Up(clients);

	if (msg)
	{
		union msg_buf *buf = (union msg_buf *)pb->addrin[0];

		client->msg = msg->next;
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending message %s for %s from %d",
			pmsg(buf->m[0]), c_owner(client), buf->m[1]));

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
		struct xa_client *locked = NULL,
		          *foc = find_focus(&waiting, &locked);

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

				// Ozk: Why poll mouse here?
				//get_mouse(3);
				//button.ks =  key.raw.conin.state;
				mu_button.ks = key.raw.conin.state;

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
	IFDIAG(if (mouse_locked())
		DIAG((D_sema,client,"Mouse OK? %d pid = %d",
			mouse_locked()->p->pid, client->p->pid));)

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

static char *
em_flag(int f)
{
	static char *mo[6] = { "into", "outof", "into", "outof" };
	return mo[f & 3];
}

#else /* GENERATE_DIAGS */

#define diag_out(x,c,y)

#endif

/* HR 070601: We really must combine events. especially for the button still down situation.
*/

/*
 * The essential evnt_multi() call
 */
unsigned long
XA_evnt_multi(enum locks lock, struct xa_client *client, AESPB *pb)
{
	bool mu_butt_p = 0;			/* Ozk 040503: Need to know if MU_BUTTON came from a pending button event
						 * or checking current button state in mu_button */
	short events = pb->intin[0];
	short x, y;
	unsigned long ret = XAC_BLOCK;		/* HR: another example of choosing inconvenient default fixed. */
	int new_waiting_for = 0,
	    fall_through = 0,
	    pbi = pending_button.head,
	    clicks = 0;
	
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
		DIAG((D_multi,client,"evnt_multi for %s, %s clks=0x%x, msk=0x%x, bst=0x%x T:%d",
				c_owner(client),
				evtxt,pb->intin[1],pb->intin[2],pb->intin[3], (events&MU_TIMER) ? pb->intin[14] : -1));
		DIAG((D_multi, client, "status %lx, %lx, C.redraws %ld", client->status, client->rdrw_msg, C.redraws));
	}
#endif

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		DIAG((D_multi, client, "evnt_multi: %s flagged as lagging! - cleared", client->name));
	}

	/*
	 * Ozk: We absolutely prioritize WM_REDRAW messages, which are
	 * queued in a separate message queue.
	*/
	if (events & MU_MESAG)
	{
		if ( pending_redraw_msgs(lock, client, pb) )
		{
			multi_intout(client, pb->intout, 0);
			pb->intout[0] = MU_MESAG;
			/*
			 * Ozk: If this was the last redraw message,
			 * reenable delivery of mouse-movement events.
			*/
			if (!C.redraws)
				kick_mousemove_timeout();
			return XAC_DONE;
		}
	}

	/*
	 * Ozk: If there are still redraw messages (for other clients than "us"),
	 * we yield() so that redraws are processed as soon as possible.
	*/
	if (C.redraws)
		yield();

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

		if (pending_button.q[pbi].client == client && mouse_ok(client))
		{
			DIAG((D_button,NULL,"pending_button multi %d", pending_button.q[pbi].b));
			pending_button.q[pbi].client = NULL;			/* is single shot. */
			pending_button.head++;
			pending_button.head &= 3;

			if (is_bevent(pending_button.q[pbi].b, pending_button.q[pbi].clicks, pb->intin + 1, 1))
			{
				fall_through |= MU_BUTTON;
				mu_butt_p = true;		/* pending button used */

				DIAG((D_button, NULL, "fall_through |= MU_BUTTON"));
			}
		}
		else
		{
			bool bev = false;

			DIAG((D_button, NULL, "still_button multi?? o[0,2] "
				"%x,%x newc/newr %d/%d, lock %d, Mbase %lx, active.widg %lx\n",
				pb->intin[1], pb->intin[3], mu_button.newc, mu_button.newr,
				mouse_locked() ? mouse_locked()->p->pid : 0,
				C.menu_base, widget_active.widg));

			//DIAG((D_button, NULL, "evnt_multi: Check if button event"));

			{
				struct moose_data *md = &client->md;
				DIAG((D_button, NULL, "evnt_multi: check button evnt on private"));
				if (md->clicks)
				{
					bev = is_bevent(md->state, md->clicks, pb->intin + 1, 1);
					clicks = md->clicks;
					md->clicks = 0;
				}
				else
				{
 					if ((bev = is_bevent(md->cstate, 0, pb->intin + 1, 1)))
						clicks = 1;
				}
			}
			if (bev == true)
				fall_through |= MU_BUTTON;
		}

		Sema_Dn(pending);

		if ((fall_through & MU_BUTTON) == 0)
		{
			new_waiting_for |= MU_BUTTON;		/* Flag the app as waiting for button changes */
			pb->intout[0] = 0;

			DIAG((D_b, client, "new_waiting_for |= MU_BUTTON"));
		}
	}

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
		bzero(&client->em, sizeof(client->em));
		if (events & MU_M1)
		{
			/* Mouse rectangle tracking */

			const RECT *r = (const RECT *)&pb->intin[5];
			client->em.m1 = *r;
			client->em.flags = pb->intin[4] | MU_M1;

			DIAG((D_multi,client,"    M1 rectangle: %d/%d,%d/%d, flag: 0x%x: %s",
				r->x, r->y, r->w, r->h, client->em.flags, em_flag(client->em.flags)));

			check_mouse(client, NULL, &x, &y);

			if (mouse_ok(client) && is_rect(x, y, client->em.flags & 1, &client->em.m1))
				fall_through    |= MU_M1;
			else
				new_waiting_for |= MU_M1;
		}

		if (events & MU_MX)
		{
			/* XaAES extension: any mouse movement. */
			client->em.flags = pb->intin[4] | MU_MX;
			DIAG((D_multi,client,"    MX"));
			new_waiting_for |= MU_MX;
		}

		if (events & MU_M2)
		{
			const RECT *r = (const RECT *)&pb->intin[10];
			client->em.m2 = *r;
			client->em.flags |= (pb->intin[9] << 1) | MU_M2;

			DIAG((D_multi,client,"    M2 rectangle: %d/%d,%d/%d, flag: 0x%x: %s",
				r->x, r->y, r->w, r->h, client->em.flags, em_flag(client->em.flags)));

			check_mouse(client, NULL, &x, &y);

			if (mouse_ok(client) && is_rect(x, y, client->em.flags & 2, &client->em.m2))
				fall_through    |= MU_M2;
			else
				new_waiting_for |= MU_M2;
		}
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
		DIAG((D_i,client,"Timer val: %ld", client->timer_val));
		if (client->timer_val)
		{
			new_waiting_for |= MU_TIMER;	/* Flag the app as waiting for a timer */
			ret = XAC_TIMER;
		}
		else
		{
			/* Is this the cause of loosing the key's at regular intervals? */
			DIAG((D_i,client, "Done timer for %d", client->p->pid));
			fall_through |= MU_TIMER;
			/* HR 190601: to be able to combine fall thru events. */
			ret = XAC_DONE;
		}
	}

	if (fall_through)
	{
		/* HR: fill out the mouse data */
		multi_intout(client, pb->intout, 0);
		if ((fall_through&MU_TIMER) == 0)
			ret = XAC_DONE;
		if ((fall_through&MU_BUTTON) != 0)
		{
			pb->intout[6] = clicks;

			/* HR 190601: Pphooo :-( This solves the Thing desk popup missing clicks. */
			/* Ozk 040501: And we need to take the data from the correct place. */
			if (mu_butt_p)
			{
				pb->intout[1] = pending_button.q[pbi].x;
				pb->intout[2] = pending_button.q[pbi].y;
				pb->intout[3] = pending_button.q[pbi].b;
				pb->intout[6] = pending_button.q[pbi].clicks;
			}
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
		DIAG((D_multi, client, "evnt_mesag: %s flagged as lagging! - cleared", client->name));
	}
	/*
	 * Ozk: look at XA_evnt_multi() for explanations..
	*/
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
	int pbi = pending_button.head;
	int clicks = 0;

	CONTROL(3,5,1)

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		DIAG((D_multi, client, "evnt_button: %s flagged as lagging! - cleared", client->name));
	}

	DIAG((D_button,NULL,"evnt_button for %s; clicks %d mask 0x%x state 0x%x",
	           c_owner(client), pb->intin[0], pb->intin[1], pb->intin[2]));

	Sema_Up(pending);

	if (pending_button.q[pbi].client == client && mouse_ok(client))
	{
		DIAG((D_button,NULL,"pending_button %d", pending_button.q[pbi].b));
		pending_button.q[pbi].client = NULL;			/* is single shot. */
		pending_button.head++;
		pending_button.head &= 3;

		if (is_bevent(pending_button.q[pbi].b, pending_button.q[pbi].clicks, pb->intin, 3))
		{
			pb->intout[0] = pending_button.q[pbi].clicks;	/* Ozk 040503: Take correct data */
			pb->intout[1] = pending_button.q[pbi].x;
			pb->intout[2] = pending_button.q[pbi].y;
			pb->intout[3] = pending_button.q[pbi].b;
			pb->intout[4] = pending_button.q[pbi].ks;
			pb->intout[5] = 0;
			pb->intout[6] = 0;
			Sema_Dn(pending);
			return XAC_DONE;
		}
	}
	else
	{
		bool bev = false;

		DIAG((D_button,NULL,"still_button? o[0,2] %x,%x newc/newr %d/%d, lock %d, Mbase %lx, active.widg %lx",
			pb->intin[0], pb->intin[2], mu_button.newc, mu_button.newr,
			mouse_locked() ? mouse_locked()->p->pid : 0,
			C.menu_base, widget_active.widg));

		{
			struct moose_data *md = &client->md;
			DIAG((D_button, NULL, "evnt_button: check event on private"));
			if (md->clicks)
			{
				bev = is_bevent(md->state, md->clicks, pb->intin, 1);
				clicks = md->clicks;
				if (md->state && !md->cstate)
					md->clicks = 0;
			}
			else
			{
				if ((bev = is_bevent(md->cstate, 0, pb->intin, 1)))
					clicks = 1;
			}
		}
		if (bev == true)
		{
			DIAG((D_button, NULL, "evnt_multi: Check if button event"));
			multi_intout(client, pb->intout, 0);
			pb->intout[0] = clicks; //1;
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
	CONTROL(5,5,0)

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		DIAG((D_multi, client, "evnt_mouse: %s flagged as lagging! - cleared", client->name));
	}
	/* Flag the app as waiting for messages */
	client->waiting_for = MU_M1;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_pb = pb;

	bzero(&client->em, sizeof(client->em));
	client->em.m1 = *((const RECT *) &pb->intin[1]);
	client->em.flags = (long)(pb->intin[0]) | MU_M1;

	check_mouse(client, NULL, &x, &y);
	if (mouse_ok(client) && is_rect(x, y, client->em.flags & 1, &client->em.m1))
	{
		multi_intout(client, pb->intout, 0);
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
XA_evnt_timer(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(2,1,0)

	client->timer_val = ((long)pb->intin[1] << 16) | pb->intin[0];

	/* Flag the app as waiting for messages */
	client->waiting_pb = pb;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_for = MU_TIMER;

	return XAC_TIMER;
}
