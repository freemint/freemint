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

#include "messages.h"

#include "c_message.h"
#include "k_main.h"
#include "k_mouse.h"
#include "vaproto.h"
#include "xa_types.h"
#include "xa_global.h"
#include "xa_evnt.h"

#if GENERATE_DIAGS
static const char *xmsgs[] =
{
	"WM_REDRAW",
	"WM_TOPPED",
	"WM_CLOSED",
	"WM_FULLED",
	"WM_ARROWED",
	"WM_HSLID",
	"WM_VSLID",
	"WM_SIZED",
	"WM_MOVED",
	"WM_NEWTOP",
	"WM_UNTOPPED",
	"WM_ONTOP",
	"WM_OFFTOP",
	"WM_BOTTOMED",
	"WM_ICONIFY	",
	"WM_UNICONIFY",
	"WM_ALLICONIFY",
	"37",
	"38",
	"39",
	"AC_OPEN",
	"AC_CLOSE",
	"42",
	"43",
	"44",
	"45",
	"46",
	"47",
	"48",
	"49",
	"AP_TERM",
	"AP_TFAIL",
	"52",
	"53",
	"54",
	"55",
	"56",
	"AP_RESCHG",
	"58",
	"59",
	"SHUT_COMPLETED",
	"RESCH_COMPLETED",
	"62",
	"AP_DRAGDROP",
	"64                           ",
	"                             ",
	"                             "
};

/* messages series 0x4700 */
static const char *va_msgs[] =
{
	"AV_PROTOKOLL",			/* 0x4700 */
	"VA_PROTOSTATUS",		/* 0x4701 */
	"4702",
	"AV_GETSTATUS",			/* 0x4703 */
	"AV_STATUS",			/* 0x4704 */
	"VA_SETSTATUS",			/* 0x4705 */
	"4706",
	"4707",
	"4708",
	"4709",
	"470a",				/* sigh... */
	"470b",
	"470c",
	"470d",
	"470e",
	"470f",
	"AV_SENDKEY",			/* 0x4710 */
	"VA_START",			/* 0x4711 */
	"AV_ASKFILEFONT",		/* 0x4712 */
	"VA_FILEFONT",			/* 0x4713 */
	"AV_ASKCONFONT",		/* 0x4714 */
	"VA_CONFONT",			/* 0x4715 */
	"AV_ASKOBJECT",			/* 0x4716 */
	"VA_OBJECT",			/* 0x4717 */
	"AV_OPENCONSOLE",		/* 0x4718 */
	"VA_CONSOLEOPEN",		/* 0x4719 */
	"471a",				/* sigh... */
	"471b",
	"471c",
	"471d",
	"471e",
	"471f",
	"AV_OPENWIND",			/* 0x4720 */
	"VA_WINDOPEN",			/* 0x4721 */
	"AV_STARTPROG",			/* 0x4722 */
	"VA_PROGSTART",			/* 0x4723 */
	"AV_ACCWINDOPEN",		/* 0x4724 */
	"VA_DRAGACCWIND",		/* 0x4725 */
	"AV_ACCWINDCLOSED",		/* 0x4726 */
	"4727",
	"AV_COPY_DRAGGED",		/* 0x4728 */
	"VA_COPY_COMPLETE",		/* 0x4729 */
	"472a",				/* sigh... */
	"472b",
	"472c",
	"472d",
	"472e",
	"472f",
	"AV_PATH_UPDATE",		/* 0x4730 */
	"4731",
	"AV_WHAT_IZIT",			/* 0x4732 */
	"VA_THAT_IZIT",			/* 0x4733 */
	"AV_DRAG_ON_WINDOW",		/* 0x4734 */
	"VA_DRAG_COMPLETE",		/* 0x4735 */
	"AV_EXIT",			/* 0x4736 */
	"4737",
	"AV_STARTED",			/* 0x4738 */
	"VA_FONTCHANGED",		/* 0x4739 */
	"473a",				/* sigh... */
	"473b",
	"473c",
	"473d",
	"473e",
	"473f",
	"AV_XWIND",			/* 0x4740 */
	"VA_XOPEN",			/* 0x4741 */
	"4742",
	"4743",
	"4744",
	"4745",
	"4746",
	"4747",
	"4748",
	"4749",
	"474a",				/* sigh... */
	"474b",
	"474c",
	"474d",
	"474e",
	"474f",
	
	/* new messages since 26.06.1995 */
	
	"4750",
	"AV_VIEW",			/* 0x4751 */
	"VA_VIEWED",			/* 0x4752 */
	"AV_FILEINFO",			/* 0x4753 */
	"VA_FILECHANGED",		/* 0x4754 */
	"AV_COPYFILE",			/* 0x4755 */
	"VA_FILECOPIED",		/* 0x4756 */
	"AV_DELFILE",			/* 0x4757 */
	"VA_FILEDELETED",		/* 0x4758 */
	"AV_SETWINDPOS",		/* 0x4759 */
	"475a",				/* sigh... */
	"475b",
	"475c",
	"475d",
	"475e",
	"475f",
	"VA_PATH_UPDATE",		/* 0x4760 */
	"4761                  "
};

static char unknown[32];

const char *
pmsg(short m)
{
	const char *ret;

	if (m >= WM_REDRAW && m <= AP_DRAGDROP)
	{
		ret = xmsgs[m - WM_REDRAW];
	}
	else if (m >= AV_PROTOKOLL && m < VA_HIGH)
	{
		ret = va_msgs[m - AV_PROTOKOLL];
	}
	else
	{
		sprintf(unknown, sizeof(unknown), "%d(%x)", m, m);
		ret = unknown;
	}

	return ret;
}
#endif

void
cancel_aesmsgs(struct xa_aesmsg_list **m)
{
	while (*m)
	{
		struct xa_aesmsg_list *nm;

		nm = (*m)->next;
		kfree(*m);
		*m = nm;
	}
}

/* Ozk:
 * If we want to do direct handling of the message sendt to a window
 * via wind->send_message, we need to install the real handler in
 * wind->do_message and place the address of do_winmesag()
 * in wind->send_message(). This is because direct handling of window
 * messages requires the handler to run in the context of windows
 * owner.
 * xxx - not dealing with cases where to is different from wind->owner!
 */
static void
Phfw(void *_parm)
{
	long *parms = _parm;
	struct xa_window *w = (struct xa_window *)parms[0];
	
	w->do_message(w,
			 (struct xa_client *)parms[1],
			 (short *)parms[2]);

	wake(IO_Q, (long)_parm);
	kfree(_parm);
	kthread_exit(0);
}

void
do_winmesag(enum locks lock,
	struct xa_window *wind,
	struct xa_client *to,			/* if different from wind->owner */
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7)
{
	if (wind->do_message)
	{
		struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);
		short msg[8] = { mp0,mp1,mp2,mp3,mp4,mp5,mp6,mp7 };

		if (!rc)
			rc = C.Aes;

		if (wind->owner == rc)
		{
			DIAGS((" --==-- do_winmesag: Doing direct handle_form_wind"));
			wind->do_message(wind, to, msg);
		}
		else
		{
			long *p = kmalloc(16);

			if (p)
			{
				p[0] = (long)wind;
				p[1] = (long)to;
				p[2] = (long)msg;

				DIAGS((" --==-- do_winmesag: Doing kthread handle_form_wind"));

				kthread_create(wind->owner->p,
						Phfw,
						p,
						NULL, "k%s", wind->owner->name);
				sleep(IO_Q, (long)p);
			}
		}
	}
#if GENERATE_DIAGS
	else
		DIAGS((" --==-- do_winmesag: no do_message!"));
#endif
}


/* Ozk:
 * context dependant
 * deliver_message is guaranteed to run in the dest_client's context
 */
void
deliver_message(enum locks lock, struct xa_client *dest_client, union msg_buf *msg)
{
	/* Is the dest client waiting for a message at the moment? */
	if (dest_client->waiting_for & MU_MESAG)
	{
		union msg_buf *clnt_buf;

#if GENERATE_DIAGS
		if (dest_client->waiting_pb == NULL)
		{
			DIAG((D_m, NULL, "MU_MESAG and NO PB! for %s", c_owner(dest_client)));
			return;
		}
#endif
		/* If the client is waiting on a multi, the response is  */
		if (dest_client->waiting_for & XAWAIT_MULTI)
			/* slightly different to the evnt_mesag() response. */
			/* HR: fill out the mouse data!!! */
			multi_intout(dest_client, dest_client->waiting_pb->intout, MU_MESAG);
		else
			dest_client->waiting_pb->intout[0] = 1;

		clnt_buf = (union msg_buf *)(dest_client->waiting_pb->addrin[0]);
		if (!clnt_buf)
		{
			DIAG((D_appl, NULL, "WARNING: Invalid target message buffer"));
			return;
		}

		/* Fill in the client's message buffer */
		*clnt_buf = *msg;

		DIAG((D_m, NULL, "Send message %s to %s", pmsg(msg->s.msg), c_owner(dest_client)));
		/* Write success to client's reply pipe to unblock the process */

		dest_client->usr_evnt = 1;
		cancel_evnt_multi(dest_client, 22);
		if (msg->m[0] == WM_REDRAW)
		{
			C.redraws--;
			dest_client->status &= ~CS_CE_REDRAW_SENT;
			if (!C.redraws)
				kick_mousemove_timeout();
		}
	}
	else
	{
		queue_message(lock, dest_client, msg);
		/*
		 * Ozk: This is a client event message, and such messages are also
		 * counted. Since queue_message() also counted this message, we "uncount"
		 * the client event here.
		 */
		if (msg->m[0] == WM_REDRAW)
		{
			C.redraws--;
			dest_client->status &= ~CS_CE_REDRAW_SENT;
		}
	}
}

static bool inline
is_inside(const RECT *r, const RECT *o)
{
	if (   (r->x        < o->x       )
	    || (r->y        < o->y       )
	    || (r->x + r->w > o->x + o->w)
	    || (r->y + r->h > o->y + o->h)
	   )
		return false;

	return true;
}

/*
 * Context independant.
 * queue a message for dest_client
 */
void
queue_message(enum locks lock, struct xa_client *dest_client, union msg_buf *msg)
{
	struct xa_aesmsg_list *ml;
	short *new = msg->m;

	if (new[0] == WM_REDRAW)
	{
		struct xa_aesmsg_list **next;
		struct xa_aesmsg_list *new_msg;
		short *old = ml->message.m;

		DIAG((D_m, NULL, "WM_REDRAW rect %d/%d,%d/%d", new[4], new[5], new[6], new[7]));

		next = &(dest_client->rdrw_msg);
		ml = *next;

		while (ml)
		{
			old = ml->message.m;

			if (old[3] == new[3])
			{
				if (is_inside((RECT *)&(new[4]), (RECT *)&(old[4])))
					return;

				if (is_inside((RECT *)&(old[4]), (RECT *)&(new[4])))
				{
					/* old inside new: replace by new. */
					ml->message.s = msg->s;
					return;
				}
			}
			next	= &(ml->next);
			ml	= *next;
		}

		new_msg = kmalloc(sizeof(*new_msg));
		if (new_msg)
		{
			*next = new_msg;
			new_msg->message = *msg;
			new_msg->next = NULL;	
			C.redraws++;
		}

		return;
	}

	/* There are already some pending messages */
	if (dest_client->msg)
	{
		ml = dest_client->msg;

		if (new[0] == WM_MOVED)
		{
			while (ml)
			{
				short *old = ml->message.m;

				if (old[0] == WM_MOVED && old[3] == new[3])
				{
					old[4] = new[4];
					old[5] = new[5];
					old[6] = new[6];
					old[7] = new[7];
					msg = NULL;
					break;
				}

				ml = ml->next;
			}
		}
		else if (new[0] == WM_SIZED)
		{
			while (ml)
			{
				short *old = ml->message.m;
					
				if (old[0] == WM_SIZED && old[3] == new[3])
				{
					old[4] = new[4];
					old[5] = new[5];
					old[6] = new[6];
					old[7] = new[7];
					msg = NULL;
					break;
				}

				ml = ml->next;
			}
		}
		else if (new[0] == WM_VSLID)
		{
			while (ml)
			{
				short *old = ml->message.m;
					
				if (old[0] == WM_VSLID && old[3] == new[3])
				{
					old[4] = new[4];
					msg = NULL;
					break;
				}

				ml = ml->next;
			}
		}
		else if (new[0] == WM_HSLID)
		{
			while (ml)
			{
				short *old = ml->message.m;
					
				if (old[0] == WM_HSLID && old[3] == new[3])
				{
					old[4] = new[4];
					msg = NULL;
					break;
				}

				ml = ml->next;
			}
		}
		else if (new[0] == WM_ARROWED)
		{
			while (ml)
			{
				short *old = ml->message.m;
					
				if (old[0] == WM_ARROWED && old[3] == new[3] && old[4] == new[4])
				{
					msg = NULL;
					break;
				}

				ml = ml->next;
			}
		}
	}

	/* If still there */
	if (msg)
	{
		struct xa_aesmsg_list *new_msg;

		new_msg = kmalloc(sizeof(*new_msg));
		if (new_msg)
		{
			/* Fill in the new pending list entry with the message */
			new_msg->message = *msg;
			new_msg->next = NULL;

			if (dest_client->msg)
			{
				/* There are already some pending messages */

				ml = dest_client->msg;
				while (ml->next)
					 ml = ml->next;

				/* Append the new message to the list */
				ml->next = new_msg;
			}
			else
				/* First entry in the client's pending message list */
				dest_client->msg = new_msg;

			DIAG((D_m, NULL, "Queued message %s for %s", pmsg(msg->s.msg), c_owner(dest_client)));
		}
	}
}

/*
 * Send an AES message to a client application.
 * generalized version, which now can be used by appl_write. :-)
 * 
 * Ozk: Okie, new semantics now; If the caller of send_a_message is NOT the receiver
 * of the message, ALWAYS queue it on behalf of the receiver. Then Unblock() receiver.
 * The receiver, when it is woken up, will then call check_queued_events() and detect
 * the pending message. No need to use client events for this, plus it is easier to
 * combine messages in the queue (like keeping only one WM_ARROWED message, combining
 * WM_REDRAW rectangles, etc.).
 */
void
send_a_message(enum locks lock, struct xa_client *dest_client, union msg_buf *msg)
{
	struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);

	if (dest_client == NULL)
	{
		DIAG((D_appl, NULL, "WARNING: Invalid target for send_a_message()"));
		return;
	}

	if (dest_client->status & (CS_LAGGING | CS_FORM_ALERT | CS_WAIT_MENU))
	{
#if GENERATE_DIAGS
		if (dest_client->status & CS_FORM_ALERT)
			DIAG((D_appl, dest_client, "send_a_message: Client %s is in form_alert "
				"- AES message discarded!", dest_client->name));

		if (dest_client->status & CS_LAGGING)
			DIAG((D_appl, dest_client, "send_a_message: Client %s is lagging - "
				"AES message discarded!", dest_client->name));
#endif
		return;
	}

	if (rc == dest_client)
	{
		if (msg->m[0] == WM_REDRAW)
		{
			dest_client->status |= CS_CE_REDRAW_SENT;
			C.redraws++;
		}
		deliver_message(lock, dest_client, msg);
	}
	else
	{
		queue_message(lock, dest_client, msg);
		Unblock(dest_client, 1, 123);
	}
}

/*
 * AES internal msgs
 */
void
send_app_message(
	enum locks lock,
	struct xa_window *wind,
	struct xa_client *to, /* if different from wind->owner */
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7)
{
	union msg_buf m;
	short *p = m.m;

	if (to == NULL)
		to = wind->owner;

	*p++ = mp0;				/* message number */
	*p++ = mp1 > 0 ? mp1 : C.AESpid;	/* source pid     */
	*p++ = mp2;
	*p++ = mp3;
	*p++ = mp4;
	*p++ = mp5;
	*p++ = mp6;
	*p++ = mp7;

	send_a_message(lock, to, &m);
}
