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

	}
	else if (C.redraws)
		yield();

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

#if GENERATE_DIAGS

static char *xev[] =
{
	"KBD", "BUT", "M1", "M2", "MSG", "TIM", "WHL", "MX", "NKBD",
	"9", "10", "11", "12", "13", "14", "15"
};

#if 0
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
#endif

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

short
checkfor_mumx_evnt(struct xa_client *client, bool is_locker, short x, short y)
{
	short events = 0;

	if (client->waiting_for & (MU_M1|MU_M2|MU_MX))
	{
		if (   (client->em.flags & MU_M1)
		    && is_rect(x, y, client->em.flags & 1, &client->em.m1))
		{
			DIAG((D_mouse, client, "%s have M1 event", client->name));
			events |= MU_M1;
		}

		if (   (client->em.flags & MU_M2)		/* M2 in evnt_multi only */
		    && is_rect(x, y, client->em.flags & 2, &client->em.m2))
		{
			DIAG((D_mouse, client, "%s have M2 event", client->name));
			events |= MU_M2;
		}

		if (client->em.flags & MU_MX)			/* MX: any movement. */
		{
			DIAG((D_mouse, client, "%s have MX event", client->name));
			events |= MU_MX;
		}

		if (events)
		{
			
			DIAG((D_mouse, client, "Post deliver M1/M2 events %d to %s", events, client->name));

			if (!is_locker)
			{
				struct xa_window *wind;
				struct xa_client *wo = NULL;

				wind = find_window(0, x, y);
			
				if (wind)
					wo = wind == root_window ? get_desktop()->owner : wind->owner;

				if (!(is_infront(client) || !wo || (wo && wo == client && (is_topped(wind) || wind->active_widgets & NO_TOPPED))) )
					events = 0;
			}
		}
	}
	return events;
}

void
get_mbstate(struct xa_client *client, struct mbs *d)
{
	short mbutts, clicks, x, y, ks;

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
		}
		else
			clicks = 0;
	}

	if (clicks)
	{
		mbutts = client->md_head->state;
		client->md_head->clicks = 0;
		x = client->md_head->x;
		y = client->md_head->y;
		ks = client->md_head->kstate;
	}
	else
	{
		mbutts = client->md_head->cstate;
		client->md_head->clicks = -1;
		check_mouse(client, NULL, &x, &y);
		vq_key_s(C.vh, &ks);
	}

	if (d)
	{
		d->b	= mbutts;
		d->c	= clicks;
		d->x	= x;
		d->y	= y;
		d->ks	= ks;
	}
}

bool
check_queued_events(struct xa_client *client)
{
	short events = 0, wevents = client->waiting_for;
	short key = 0;
	struct mbs mbs;
	bool multi = wevents & XAWAIT_MULTI ? true : false;
	AESPB *pb;
	short *out;
	
	if (!(pb = client->waiting_pb))
	{
		DIAG((D_appl, NULL, "WARNING: Invalid target message buffer for %s", client->name));
		return false;
	}

	out = pb->intout;

	get_mbstate(client, &mbs);

	if ((wevents & MU_MESAG) && (client->msg || client->rdrw_msg || client->crit_msg))
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

			if (multi)
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
	if ((wevents & MU_BUTTON))
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

		bev = is_bevent(mbs.b, mbs.c, in, 1);	
		if (bev)
		{
			if (!mbs.c)
				mbs.c++;

			if (multi)
				events |= MU_BUTTON;
			else
			{
				*out++ = mbs.c;
				*out++ = mbs.x;
				*out++ = mbs.y;
				*out++ = mbs.b;
				*out++ = mbs.ks;
				goto got_evnt;
			}
		}
	}
	if ((wevents & (MU_NORM_KEYBD|MU_KEYBD)))
	{
		struct rawkey keys;

		if (unqueue_key(client, &keys))
		{
			if ((wevents & MU_NORM_KEYBD))
				key = nkc_tconv(keys.raw.bcon);
			else
				key = keys.aes;

			DIAGS((" -- kbstate=%x", mbs.ks));

			mbs.ks = keys.raw.conin.state;

			if (multi)
				events |= wevents & (MU_NORM_KEYBD|MU_KEYBD);
			else
			{
				*out = key;
				goto got_evnt;
			}
		}
	}
	{
		short ev;
		bool is_locker = (client == mouse_locked() || client == update_locked()) ? true : false;
		
		if ((ev = checkfor_mumx_evnt(client, is_locker, mbs.x, mbs.y)))
		{
			if (multi)
				events |= ev;
			else
			{
				*out++ = 1;
				*out++ = mbs.x;
				*out++ = mbs.y;
				*out++ = mbs.b;
				*out   = mbs.ks;
				goto got_evnt;
			}
		}
	}
	if (wevents & MU_TIMER)
	{
		if (!client->timeout)
		{
			if (multi)
				events |= MU_TIMER;
			else
			{
				*out = 1;
				goto got_evnt;
			}
		}
	}
	/* AES 4.09 */
	if (wevents & MU_WHEEL)
	{
		/*
		 * Ozk: This has got to be rethinked.
		 * cannot (at leat I really, REALLY!, dont like a different
		 * meanings of the current evnt_multi() parameters.
		 * Perhaps better to add params for WHEEL event?
		 * intout[4] and intout[6] is not to be used for the wheel
		 * I think, as that would rule out normal buttons + wheel events
		 */
#if 0
		DIAG((D_i,client,"    MU_WHEEL"));
		if (multi)
			events |= MU_WHEEL;
#endif
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
			DIAG((D_multi, client, " -- %x, %x, %x, %x, %x, %x, %x",
				events, mbs.x, mbs.y, mbs.b, mbs.ks, key, mbs.c));
		}
#endif
		if (client->timeout)
		{
			canceltimeout(client->timeout);
			client->timeout = NULL;
		}

		*out++ = events;
		*out++ = mbs.x;
		*out++ = mbs.y;
		*out++ = mbs.b;
		*out++ = mbs.ks;
		*out++ = key;
		*out   = mbs.c;
	}
	else
		return false;

got_evnt:
	client->usr_evnt = 1;
	cancel_evnt_multi(client, 222);
	return true;
}

static void
wakeme_timeout(struct proc *p, struct xa_client *client)
{
	client->timeout = NULL;
	wake(IO_Q, (long)client);
}

/* HR 070601: We really must combine events. especially for the button still down situation.
*/

/*
 * The essential evnt_multi() call
 */
unsigned long
XA_evnt_multi(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short events = pb->intin[0] | XAWAIT_MULTI;
	
	CONTROL(16,7,1)

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
	
	/* here we prepare structures necessary to wait for events
	*/
	if (events & (MU_M1|MU_M2|MU_MX))
	{
		if (events & MU_M1)
		{
			const RECT *r = (const RECT *)&pb->intin[5];

			client->em.m1 = *r;
			client->em.flags = pb->intin[4] | MU_M1;

		}
		if (events & MU_MX)
		{
			client->em.flags = pb->intin[4] | MU_MX;
		}
		if (events & MU_M2)
		{
			const RECT *r = (const RECT *)&pb->intin[10];

			client->em.m2 = *r;
			client->em.flags |= (pb->intin[9] << 1) | MU_M2;
		}
	}

	if (events & MU_TIMER)
	{
		/* The Intel ligent format */
		client->timer_val = ((long)pb->intin[15] << 16) | pb->intin[14];
		DIAG((D_i,client,"Timer val: %ld(hi=%d,lo=%d)",
			client->timer_val, pb->intin[15], pb->intin[14]));
		if (client->timer_val)
		{
			client->timeout = addtimeout(client->timer_val, wakeme_timeout);
			if (client->timeout)
				client->timeout->arg = (long)client;
		}
		else
		{
			/* Is this the cause of loosing the key's at regular intervals? */
			DIAG((D_i,client, "Done timer for %d", client->p->pid));

			/* If MU_TIMER and no timer (which means, return immediately),
			 * we yield()
			 */
			yield();
		}
	}
	

	client->waiting_for = events | XAWAIT_MULTI;
	client->waiting_pb = pb;
	
	if (check_cevents(client) || check_queued_events(client))
		return XAC_DONE;

	Block(client, 1);
	return XAC_DONE;
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

	client->waiting_for = MU_MESAG;	/* Mark the client as waiting for messages */
	client->waiting_pb = pb;

	if (!check_queued_events(client))
		Block(client, 1);

	return XAC_DONE;
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

	client->waiting_for = MU_BUTTON;
	client->waiting_pb = pb;
	if (!check_queued_events(client))
		Block(client, 1);
	return XAC_DONE;
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

	client->waiting_for = MU_KEYBD;
	client->waiting_pb = pb;
	if (!check_queued_events(client))
		Block(client, 1);
	return XAC_DONE;
}

/*
 * Event Mouse
 */
unsigned long
XA_evnt_mouse(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(5,5,0)

	if (client->status & CS_LAGGING)
	{
		client->status &= ~CS_LAGGING;
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_mouse: %s flagged as lagging! - cleared", client->name));
	}

	bzero(&client->em, sizeof(client->em));
	client->em.m1 = *((const RECT *) &pb->intin[1]);
	client->em.flags = (long)(pb->intin[0]) | MU_M1;

	/* Flag the app as waiting for messages */
	client->waiting_for = MU_M1;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_pb = pb;

	if (!check_queued_events(client))
		Block(client, 1);

	return XAC_DONE;
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
	}
	else
	{
		/* Flag the app as waiting for messages */
		client->waiting_pb = pb;
		/* Store a pointer to the AESPB to fill when the event occurs */
		client->waiting_for = MU_TIMER;
		client->timeout = addtimeout(client->timer_val, wakeme_timeout);
		if (client->timeout)
			client->timeout->arg = (long)client;

		Block(client, 1);
	}
	return XAC_DONE;
}
