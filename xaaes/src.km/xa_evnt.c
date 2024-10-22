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

#include "xa_evnt.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "draw_obj.h"
#include "k_keybd.h"
#include "k_main.h"
#include "k_mouse.h"
#include "menuwidg.h"
#include "messages.h"
#include "nkcc.h"
#include "rectlist.h"
#include "widgets.h"


static int
pending_critical_msgs(int lock, struct xa_client *client, union msg_buf *buf)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;

	Sema_Up(LOCK_CLIENTS);

	msg = client->crit_msg;
	if (msg)
	{
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

	Sema_Dn(LOCK_CLIENTS);
	return rtn;
}

static int
pending_redraw_msgs(int lock, struct xa_client *client, union msg_buf *buf)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;

	Sema_Up(LOCK_CLIENTS);
	msg = client->rdrw_msg;
	if (msg)
	{
		/* dequeue */
		client->rdrw_msg = msg->next;

		/* write to client */
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending WM_REDRAW (%lx (wind=%d, %d/%d/%d/%d)) for %s",
			(unsigned long)msg, buf->m[3], buf->m[4], buf->m[5], buf->m[6], buf->m[7], c_owner(client) ));

		kfree(msg);
		rtn = 1;
		kick_mousemove_timeout();
	}
	else{
		if (C.redraws )
			yield();
	}

	Sema_Dn(LOCK_CLIENTS);
	return rtn;
}

static int
pending_msgs(int lock, struct xa_client *client, union msg_buf *buf)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;

	Sema_Up(LOCK_CLIENTS);

	msg = client->msg;
	if (msg)
	{
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

	Sema_Dn(LOCK_CLIENTS);
	return rtn;
}

static int
pending_iredraw_msgs(int lock, struct xa_client *client, union msg_buf *buf)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;

	Sema_Up(LOCK_CLIENTS);

	msg = client->irdrw_msg;
	if (msg)
	{
		/* dequeue */
		client->irdrw_msg = msg->next;

		/* write to client */
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending WM_iREDRAW (%lx (wind=%d, %d/%d/%d/%d)) for %s",
			(unsigned long)msg, buf->m[3], buf->m[4], buf->m[5], buf->m[6], buf->m[7], c_owner(client) ));

		kfree(msg);
		rtn = 1;
	}

	Sema_Dn(LOCK_CLIENTS);
	return rtn;
}
void
exec_iredraw_queue(int lock, struct xa_client *client)
{
	struct xa_window *wind;
	GRECT *r;
	long rdrws = -1L;
	union msg_buf ibuf;

	if (pending_iredraw_msgs(lock, client, &ibuf))
	{
		if (!(client->status & CS_NO_SCRNLOCK))
			lock_screen(client->p, false);
		hidem();
		do {
			short xaw = ibuf.irdrw.xaw;
			wind = (struct xa_window *)ibuf.irdrw.ptr;
			r = &ibuf.irdrw.rect;

			if (!xaw && (wind != root_window || (wind == root_window && get_desktop()->owner == client)))
			{
				if (!(r->g_w || r->g_h))
					r = NULL;

				display_window(lock, 14, wind, r);
			}
			else
			{
				struct xa_widget *widg = get_widget(wind, xaw);
				if (widg->m.r.draw)
				{
					struct xa_vdi_settings *v = wind->vdi_settings;
					(*v->api->set_clip)(v, r);
					(*widg->m.r.draw)(wind, widg, r);
					(*v->api->clear_clip)(v);
				}
			}
			rdrws++;
		} while (pending_iredraw_msgs(lock, client, &ibuf));

		showm();

		C.redraws -= rdrws;

		if (!(client->status & CS_NO_SCRNLOCK))
 			unlock_screen(client->p);
		kick_mousemove_timeout();
	}
	if (!client->rdrw_msg && C.redraws)
		yield();
}

#if GENERATE_DIAGS

static const char *const xev[] =
{
	"KBD", "BUT", "M1", "M2", "MSG", "TIM", "WHL", "MX", "NKBD",
	"9", "10", "11", "12", "13", "14", "15"
};

#endif

short
checkfor_mumx_evnt(struct xa_client *client, bool is_locker, short x, short y)
{
	short events = 0;

	/*
	 * Check for activity in Tab list, indicating menu-navigation
	 * in which case we do not report mousemovement events
	 */
	if (!TAB_LIST_START && client->waiting_for & (MU_M1|MU_M2|MU_MX))
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

				wind = find_window(0, x, y, FNDW_NOLIST|FNDW_NORMAL);

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
	struct moose_data *md;

	DIAG((D_button, NULL, "get_mbstate:  md: clicks=%d, head=%lx, tail=%lx, end=%lx",
		client->md_head->clicks, (unsigned long)client->md_head, (unsigned long)client->md_tail, (unsigned long)client->md_end));

	md = client->md_head;
	clicks = md->clicks;

	if (clicks == -1) {
		if (md != client->md_tail) {
			md++;
			if (md > client->md_end)
				md = client->mdb;
			client->md_head = md;
			clicks = 1;
		} else
			clicks = 0;
	}

	if (clicks) {
		clicks = md->clicks;
		mbutts = md->state;
		x = md->x;
		y = md->y;
		ks = md->kstate;
	} else {
		mbutts = md->cstate;
		check_mouse(client, NULL, &x, &y);
		vq_key_s(C.P_handle, &ks);
	}
	md->clicks = -1;

	if (d) {
		d->b	= mbutts;
		d->c	= clicks;
		d->x	= x;
		d->y	= y;
		d->ks	= ks;
		d->empty = (client->md_head == client->md_tail) ? true : false;
	}
}

bool
check_queued_events(struct xa_client *client)
{
	short events = 0;
	short key = 0;
	struct mbs mbs;
	AESPB *pb;
	short *out;
	union msg_buf *m;
	bool multi, to_yield = false;
	unsigned long wevents;
	wevents = client->waiting_for;
	multi = wevents & XAWAIT_MULTI ? true : false;

	if (!(pb = client->waiting_pb)) {
		DIAG((D_appl, NULL, "WARNING: Invalid target message buffer for %s", client->name));
		return false;
	}

	out = pb->intout;

	get_mbstate(client, &mbs);

	if ((wevents & MU_MESAG) && (client->msg || client->rdrw_msg || client->crit_msg)) {
		m = (union msg_buf *)pb->addrin[0];
		if (m) {
			if (!pending_critical_msgs(0, client, m)) {
				if (!pending_redraw_msgs(0, client, m))
					pending_msgs(0, client, m);
			}

			if (multi)
				events |= MU_MESAG;
			else {
				*out = 1;
				goto got_evnt;
			}
		} else {
			DIAG((D_m, NULL, "MU_MESAG and NO PB! for %s", client->name));
			DIAGS(("MU_MESAG and NO PB! for %s", client->name));
			return false;
		}
	}
	/* should MU_BUTTON be sent to a client without any windows? */
	if ((wevents & MU_BUTTON)) {
		bool bev = false;
		const short *in = multi ? pb->intin + 1 : pb->intin;

		DIAG((D_button, NULL, "still_button multi?? o[0,2] "
			"%x,%x, lock %d, Mbase %lx, active.widg %lx",
			pb->intin[1], pb->intin[3],
			mouse_locked() ? mouse_locked()->pid : 0,
			(unsigned long)TAB_LIST_START, (unsigned long)widget_active.widg));

		DIAG((D_button, NULL, " -=- md: clicks=%d, head=%lx, tail=%lx, end=%lx",
			client->md_head->clicks, (unsigned long)client->md_head, (unsigned long)client->md_tail, (unsigned long)client->md_end));

		bev = is_bevent(mbs.b, mbs.c, in, 1);
		if (bev) {
			if (!mbs.c)
				mbs.c++;

			if (multi)
				events |= MU_BUTTON;
			else {
				*out++ = mbs.c;
				*out++ = mbs.x;
				*out++ = mbs.y;
				*out++ = mbs.b;
				*out++ = mbs.ks;
				goto got_evnt;
			}
		}
	}
	if ((wevents & (MU_NORM_KEYBD|MU_KEYBD))) {
		struct rawkey keys;

		if (unqueue_key(client, &keys)) {
			if ((wevents & MU_NORM_KEYBD))
				key = nkc_tconv(keys.raw.bcon);
			else
				key = keys.aes;

			DIAGS((" -- kbstate=%x", mbs.ks));
			mbs.ks = keys.raw.conin.state;

			if (multi)
				events |= (wevents & (MU_NORM_KEYBD|MU_KEYBD));
			else {
				*out = key;
				goto got_evnt;
			}
		}
	}
	{
		short ev;
		bool is_locker = (client->p == mouse_locked() || client->p == update_locked()) ? true : false;

		if ((ev = checkfor_mumx_evnt(client, is_locker, mbs.x, mbs.y))) {
			if (multi)
				events |= ev;
			else {
				*out++ = 1;
				*out++ = mbs.x;
				*out++ = mbs.y;
				*out++ = mbs.b;
				*out   = mbs.ks;
				goto got_evnt;
			}
		}
	}
	/* AES 4.09 */
	if (wevents & MU_WHEEL) {
		/*
		 * Ozk: This has got to be rethinked.
		 * cannot (at leat I really, REALLY!, dont like a different
		 * meanings of the current evnt_multi() parameters.
		 * Perhaps better to add params for WHEEL event?
		 * intout[4] and intout[6] is not to be used for the wheel
		 * I think, as that would rule out normal buttons + wheel events
		 */
		if (client->wheel_md) {
			struct moose_data *md = client->wheel_md;

			DIAG((D_i,client,"    MU_WHEEL"));

			client->wheel_md = NULL;

			if (multi) {
				events |= MU_WHEEL;
				mbs.ks	= md->state;
				mbs.c	= md->clicks;
			}
			kfree(md);
		}
	}
	/* Ozk:
	 *	Check for MU_TIMER must come last!!
	*/
	if ((wevents & MU_TIMER) && !client->timeout) {
		if (multi) {
			if (!events && (wevents & XAWAIT_NTO))
				to_yield = true;
			events |= MU_TIMER;
		} else {
			*out = 1;
			goto got_evnt;
		}
	}
	if (events) {
		cancel_mutimeout(client);
#if GENERATE_DIAGS
		{
			char evtxt[128];
			show_bits(events, "evnt=", xev, evtxt);
			DIAG((D_multi,client,"check_queued_events for %s, %s clks=0x%x, msk=0x%x, bst=0x%x T:%d",
				c_owner(client),
				evtxt,pb->intin[1],pb->intin[2],pb->intin[3], (events&MU_TIMER) ? pb->intin[14] : -1));
			DIAG((D_multi, client, "status %lx, %lx, C.redraws %ld", client->status, (unsigned long)client->rdrw_msg, C.redraws));
			DIAG((D_multi, client, " -- %x, %x, %x, %x, %x, %x, %x",
				events, mbs.x, mbs.y, mbs.b, mbs.ks, key, mbs.c));
		}
#endif
		if (to_yield)
			yield();

		*out++ = events;
		*out++ = mbs.x;
		*out++ = mbs.y;
		*out++ = mbs.b;
		*out++ = mbs.ks;
		*out++ = key;
		*out   = mbs.c;
	} else
	{
		return false;
	}

got_evnt:
	client->status |= CS_CALLED_EVNT;
	cancel_evnt_multi(client);
	return true;
}

static void
wakeme_timeout(struct proc *p, long arg)
{
	struct xa_client *client = (struct xa_client *)arg;

	client->timeout = NULL;

	if (client->blocktype == XABT_SELECT)
		wakeselect(client->p);
	else if (client->blocktype == XABT_SLEEP)
		wake(IO_Q, client->sleeplock);
}

void
cancel_mutimeout(struct xa_client *client)
{
	if (client->timeout) {
		canceltimeout(client->timeout);
		client->timeout = NULL;
		client->timer_val = 0;
	}
}

/* HR 070601: We really must combine events. especially for the button still down situation.
*/

/*
 * The essential evnt_multi() call
 */
unsigned long
XA_evnt_multi(int lock, struct xa_client *client, AESPB *pb)
{
	unsigned long events = (unsigned long)pb->intin[0] | XAWAIT_MULTI;

	CONTROL(16,7,1)

#if GENERATE_DIAGS
	{
		char evtxt[128];
		show_bits(events, "evnt=", xev, evtxt);
		DIAG((D_multi,client,"evnt_multi for %s, %s clks=0x%x, msk=0x%x, bst=0x%x T:%d",
				c_owner(client),
				evtxt,pb->intin[1],pb->intin[2],pb->intin[3], (events&MU_TIMER) ? pb->intin[14] : -1));
		DIAG((D_multi, client, "status %lx, %lx, C.redraws %ld", client->status, (unsigned long)client->rdrw_msg, C.redraws));
	}
#endif
	if (client->status & (CS_LAGGING | CS_MISS_RDRW)) {
		client->status &= ~(CS_LAGGING|CS_MISS_RDRW);
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_multi: %s flagged as lagging! - cleared", client->name));
	}

	/* here we prepare structures necessary to wait for events
	*/
	client->em.flags = 0;
	if (events & (MU_M1|MU_M2|MU_MX)) {
		if (events & MU_M1) {
			const GRECT *r = (const GRECT *)&pb->intin[5];

			client->em.m1 = *r;
			client->em.flags = (pb->intin[4] & 1) | MU_M1;

		}
		if (events & MU_MX) {
			client->em.flags = (pb->intin[4] & 1) | MU_MX;
		}
		if (events & MU_M2) {
			const GRECT *r = (const GRECT *)&pb->intin[10];

			client->em.m2 = *r;
			client->em.flags |= ((pb->intin[9] & 1) << 1) | MU_M2;
		}
	}

	if (events & MU_TIMER) {
		/* The Intel ligent format */
		client->timer_val = ((unsigned long)(unsigned short)pb->intin[15] << 16) | (unsigned long)(unsigned short)pb->intin[14];

		DIAG((D_i,client,"Timer val: %ld(hi=%d,lo=%d)",	client->timer_val, pb->intin[15], pb->intin[14]));
#if SKIP_TEXEL_INTRO
		if( events == 0x10023 && TOP_WINDOW->owner != client && !strcmp( client->name, "  Texel " ) )
			client->timer_val = 0;	/* fake for texel-demo */
#endif
		if (client->timer_val > MIN_TIMERVAL )
		{
			client->timeout = addtimeout(client->timer_val, wakeme_timeout);
			if (client->timeout)
				client->timeout->arg = (long)client;
		}
		else {
			/* Is this the cause of loosing the key's at regular intervals? */
			DIAG((D_i,client, "Done timer for %d", client->p->pid));

			/* If MU_TIMER and no timer (which means, return immediately),
			 * we tell check_queued_events() to yield() if no other events
			 * are pending.
			 */
			events |= XAWAIT_NTO;
			client->timer_val = 0;
		}
	}

	client->waiting_for = events | XAWAIT_MULTI;
	client->waiting_pb = pb;
	(*client->block)(client);
	return XAC_DONE;
}

/*
 * Cancel an event_multi()
 * - Called when any one of the events we were waiting for occurs
 */
void
cancel_evnt_multi(struct xa_client *client)
{
	if (client != C.Aes && client != C.Hlp) {
		client->waiting_for = 0;
		client->em.flags = 0;
		client->waiting_pb = NULL;
	}
	DIAG((D_kern,NULL,"cancel_evnt_multi for %s", c_owner(client)));
}

/*
 * AES message handling
 */
unsigned long
XA_evnt_mesag(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,1)

	if (client->status & (CS_LAGGING | CS_MISS_RDRW)) {
		client->status &= ~(CS_LAGGING|CS_MISS_RDRW);
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_mesag: %s flagged as lagging! - cleared", client->name));
	}

	client->waiting_for = MU_MESAG;	/* Mark the client as waiting for messages */
	client->waiting_pb = pb;

	if (!check_queued_events(client))
	{
		(*client->block)(client);
	}

	return XAC_DONE;
}

/*
 * evnt_button() routine
 */
unsigned long
XA_evnt_button(int lock, struct xa_client *client, AESPB *pb)
{

	CONTROL(3,5,1)

	if (client->status & (CS_LAGGING | CS_MISS_RDRW)) {
		client->status &= ~(CS_LAGGING|CS_MISS_RDRW);
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_button: %s flagged as lagging! - cleared", client->name));
	}

	DIAG((D_button,NULL,"evnt_button for %s; clicks %d mask 0x%x state 0x%x",
	           c_owner(client), pb->intin[0], pb->intin[1], pb->intin[2]));

	client->waiting_for = MU_BUTTON;
	client->waiting_pb = pb;
	if (!check_queued_events(client))
	{
		(*client->block)(client);
	}
	return XAC_DONE;
}

/*
 * evnt_keybd() routine
 */
unsigned long
XA_evnt_keybd(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,0)

	if (client->status & (CS_LAGGING | CS_MISS_RDRW)) {
		client->status &= ~(CS_LAGGING|CS_MISS_RDRW);
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_keybd: %s flagged as lagging! - cleared", client->name));
	}

	client->waiting_for = MU_KEYBD;
	client->waiting_pb = pb;
	if (!check_queued_events(client))
	{
		(*client->block)(client);
	}
	return XAC_DONE;
}

/*
 * Event Mouse
 */
unsigned long
XA_evnt_mouse(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(5,5,0)

	if (client->status & (CS_LAGGING | CS_MISS_RDRW)) {
		client->status &= ~(CS_LAGGING|CS_MISS_RDRW);
		redraw_client_windows(lock, client);
		DIAG((D_multi, client, "evnt_mouse: %s flagged as lagging! - cleared", client->name));
	}

	bzero(&client->em, sizeof(client->em));
	client->em.m1 = *((const GRECT *) &pb->intin[1]);
	client->em.flags = (long)(pb->intin[0]) | MU_M1;

	/* Flag the app as waiting for messages */
	client->waiting_for = MU_M1;
	/* Store a pointer to the AESPB to fill when the event occurs */
	client->waiting_pb = pb;

	if (!check_queued_events(client))
	{
		(*client->block)(client);
	}

	return XAC_DONE;
}

/*
 * Evnt_timer()
 */
unsigned long
XA_evnt_timer(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(2,1,0)

	if ((client->timer_val = ((unsigned long)(unsigned short)pb->intin[1] << 16) | (unsigned long)(unsigned short)pb->intin[0]) <= MIN_TIMERVAL	) {
		client->timer_val = 0;
		yield();
	} else {
		/* Flag the app as waiting for messages */
		client->waiting_pb = pb;
		/* Store a pointer to the AESPB to fill when the event occurs */
		client->waiting_for = MU_TIMER;
		client->timeout = addtimeout(client->timer_val, wakeme_timeout);
		if (client->timeout) {
			client->timeout->arg = (long)client;
		}
		(*client->block)(client);
	}

	return XAC_DONE;
}


/*
 * evnt_dclick() routine
 */
unsigned long
XA_evnt_dclick(int lock, struct xa_client *client, AESPB *pb)
{
	short newval = pb->intin[0];
	short mode = pb->intin[1];

	CONTROL(2,1,0)

	if (mode != 0)
	{
		switch (newval)
		{
			case 0: cfg.double_click_time = 450 / 5; break;
			case 1: cfg.double_click_time = 330 / 5; break;
			case 2: cfg.double_click_time = 275 / 5; break;
			default:
			case 3: cfg.double_click_time = 220 / 5; break; /* DOUBLE_CLICK_TIME */
			case 4: cfg.double_click_time = 165 / 5; break;
		}
		adi_ioctl(G.adi_mouse, MOOSE_DCLICK, cfg.double_click_time);
	}

	if (cfg.double_click_time <= 165 / 5)
		pb->intout[0] = 4;
	else if (cfg.double_click_time <= 220 / 5)
		pb->intout[0] = 3;
	else if (cfg.double_click_time <= 275 / 5)
		pb->intout[0] = 2;
	else if (cfg.double_click_time <= 330 / 5)
		pb->intout[0] = 1;
	else
		pb->intout[0] = 0;
	return XAC_DONE;
}

