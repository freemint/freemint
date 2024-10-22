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

#include "messages.h"

#include "desktop.h"
#include "draw_obj.h"
#include "k_main.h"
#include "k_mouse.h"
#include "vaproto.h"
#include "xa_types.h"
#include "xa_global.h"
#include "xa_evnt.h"
#include "rectlist.h"


static void queue_message(int lock, struct xa_client *dest_client, short amq, short qmf, union msg_buf *msg);

#if GENERATE_DIAGS
static const char *xmsgs[] =
{
	"WM_REDRAW",		/* 0x0014    20	*/
	"WM_TOPPED",
	"WM_CLOSED",
	"WM_FULLED",
	"WM_ARROWED",
	"WM_HSLID",
	"WM_VSLID",
	"WM_SIZED",
	"WM_MOVED",
	"WM_NEWTOP",
	"WM_UNTOPPED",		/* 30	*/
	"WM_ONTOP",
	"WM_OFFTOP",
	"WM_BOTTOMED",
	"WM_ICONIFY	",
	"WM_UNICONIFY",
	"WM_ALLICONIFY",
	"WM_TOOLBAR",
	"38",
	"39",
	"AC_OPEN",		/* 40 */
	"AC_CLOSE",
	"42",
	"43",
	"44",
	"45",
	"46",
	"47",
	"48",
	"49",
	"AP_TERM",		/* 50 */
	"AP_TFAIL",
	"52",
	"53",
	"54",
	"55",
	"56",
	"AP_RESCHG",
	"58",
	"59",
	"SHUT_COMPLETED",	/* 60 */
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

#define WINX_HIGH (WM_UNSHADED + 1)

static const char *winx_msgs[] =
{
	"WM_SHADED",
	"WM_UNSHADED"
};

static char unknown[32];


#if GENERATE_DIAGS
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
	else if (m >= WM_SHADED && m < WINX_HIGH)
	{
		ret = winx_msgs[m - WM_SHADED];
	}
	else
	{
		sprintf(unknown, sizeof(unknown), "%d(%x)", m, m);
		ret = unknown;
	}

	return ret;
}
#endif
#endif

long
cancel_aesmsgs(struct xa_aesmsg_list **m)
{
	long redraws = 0;

	while (*m)
	{
		struct xa_aesmsg_list *nm;

		if ((*m)->message.m[0] == WM_REDRAW)
			redraws++;

		nm = (*m)->next;
		kfree(*m);
		*m = nm;
	}
	return redraws;
}

long
cancel_app_aesmsgs(struct xa_client *client)
{
	long redraws = 0;

	redraws =  cancel_aesmsgs(&client->irdrw_msg);
	redraws += cancel_aesmsgs(&client->rdrw_msg);
	redraws += cancel_aesmsgs(&client->msg);
	redraws += cancel_aesmsgs(&client->crit_msg);
	cancel_aesmsgs(&client->lost_rdrw_msg);
	return redraws;
}

/* Ozk:
 * If we want to do direct handling of the message sent to a window
 * via wind->send_message, we need to install the real handler in
 * wind->do_message and place the address of do_winmesag()
 * in wind->send_message(). This is because direct handling of window
 * messages requires the handler to run in the context of windows
 * owner.
 * XXX - not dealing with cases where to is different from wind->owner!
 */

struct CE_do_winmesag_data
{
	struct xa_window *wind;
	struct xa_client *client;
	short amq;
	short qmf;
	short msg[8];
};

static void
CE_do_winmesag(int lock, struct c_event *ce, short cancel)
{
	struct CE_do_winmesag_data *args = ce->ptr1;
	if (!cancel)
	{
		struct xa_window *wind = args->wind;
		/*
		 * Clear args->wind before calling do_message, incase
		 * a call to cancel_do_winmesag is done and the 'args'
		 * is freed there, causing havoc when freed again here!!
		 */
		args->wind = NULL;
		wind->do_message(wind, args->client, args->amq, args->qmf, args->msg);
	}
	kfree(args);
}

static bool
cdwm(struct c_event *ce, long arg)
{
	struct CE_do_winmesag_data *args = ce->ptr1;
	struct xa_window *wind = (struct xa_window *)arg;

	if (args->wind == wind)
	{
		if (args->msg[0] == WM_REDRAW)
			kick_mousemove_timeout();

		kfree(args);
		return true;
	}
	return false;
}

void
cancel_do_winmesag(int lock, struct xa_window *wind)
{
	cancel_CE(wind->owner, CE_do_winmesag, cdwm, (long)wind);
}

void
do_winmesag(int lock,
	struct xa_window *wind,
	struct xa_client *to,			/* if different from wind->owner */
	short amq, short qmf,
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7)
{
	if (wind->do_message)
	{
		struct xa_client *wo;
		short msg[8] = { mp0,mp1,mp2,mp3,mp4,mp5,mp6,mp7 };
		wo = wind == root_window ? get_desktop()->owner : wind->owner;

		if ((wind->dial & created_for_SLIST))
			wind->do_message(wind, wo, amq, qmf, msg);
		else if (!(wind->owner->status & CS_EXITING))
		{
			struct CE_do_winmesag_data *p;

			if (mp0 == WM_REDRAW)
			{
				/* BLOGif( mp3 < 50 && mp3 >= 0,(0,"do_winmesag: WM_REDRAW %s %d/%d/%d/%d/%d/%d/%d/%d", wind->wname, mp0,mp1,mp2,mp3,mp4,mp5,mp6,mp7)); */
				C.redraws++, C.move_block = 3;
			}

			p = kmalloc(sizeof(*p));
			if (p)
			{
				int i;

				p->wind = wind;
				p->client = wo;
				p->amq = amq;
				p->qmf = qmf;
				for (i = 0; i < 8; i++)
					p->msg[i] = msg[i];


				post_cevent(wo, CE_do_winmesag, p, NULL, 0,0, NULL, NULL);
			}
		}
	} else
	{
		DIAGS((" --==-- do_winmesag: no do_message!"));
	}
}

static void
#if GENERATE_DIAGS
add_msg_2_queue(struct xa_client *client, struct xa_aesmsg_list **queue, union msg_buf *msg, short qmflags)
#else
add_msg_2_queue(struct xa_aesmsg_list **queue, union msg_buf *msg, short qmflags)
#endif
{
	short *new = msg->m;
	struct xa_aesmsg_list **next, *new_msg;

	if (new[0] == WM_REDRAW)
	{
		short *old;

		DIAG((D_m, NULL, "WM_REDRAW for %s, rect %d/%d,%d/%d", client->name, new[4], new[5], new[6], new[7]));

		next = queue;

		if (qmflags & QMF_CHKDUP)
		{
			while (*next)
			{
				old = (*next)->message.m;

				if (old[3] == new[3] && old[0] == new[0] )
				{
					if ( !memcmp((GRECT *)&(old[4]), (GRECT *)&(new[4]),sizeof(GRECT)) || is_inside((GRECT *)&(new[4]), (GRECT *)&(old[4])))
					{
						msg = NULL;
						break;
					}
					if (is_inside((GRECT *)&(old[4]), (GRECT *)&(new[4])))
					{
						/* old inside new: replace by new. */
						(*next)->message.s = msg->s;
						msg = NULL;
						break;
					}
				}
				next = &((*next)->next);
			}
		}
		else
		{
			while (*next)
				next = &((*next)->next);
		}

		if (msg)
		{
			new_msg = kmalloc(sizeof(*new_msg));
			DIAG((D_m, NULL, "new WM_REDRAW message %lx for %s", (unsigned long)new_msg, client->name));
			if (new_msg)
			{
				*next = new_msg;
				new_msg->message = *msg;
				new_msg->next = NULL;
				/* BLOGif( new[3] < 50 && new[3] >= 0, (0, "add_msg_2_queue:WM_REDRAW added for %d rect %d/%d,%d/%d", new_msg->message.m[3], new_msg->message.m[4], new_msg->message.m[5], new_msg->message.m[6], new[7])); */
				if (!(qmflags & QMF_NOCOUNT))
				{
					C.redraws++;
					C.move_block = 3;
				}
			}
		}
		return;
	}

	/* There are already some pending messages */
	next = queue;

	if (*queue && (qmflags & QMF_CHKDUP))
	{

		if (new[0] == WM_MOVED)
		{
			while (*next)
			{
				short *old = (*next)->message.m;

				if (old[0] == new[0] && old[3] == new[3])
				{
					old[4] = new[4];
					old[5] = new[5];
					old[6] = new[6];
					old[7] = new[7];
					msg = NULL;
					break;
				}
				next = &((*next)->next);
			}
		}
		else if (new[0] == WM_SIZED)
		{
			while (*next)
			{
				short *old = (*next)->message.m;

				if (old[0] == new[0] && old[3] == new[3])
				{
					old[4] = new[4];
					old[5] = new[5];
					old[6] = new[6];
					old[7] = new[7];
					msg = NULL;
					break;
				}
				next = &((*next)->next);
			}
		}
		else if (new[0] == WM_VSLID)
		{
			while (*next)
			{
				short *old = (*next)->message.m;

				if (old[0] == new[0] && old[3] == new[3])
				{
					old[4] = new[4];
					msg = NULL;
					break;
				}
				next = &((*next)->next);
			}
		}
		else if (new[0] == WM_HSLID)
		{
			while (*next)
			{
				short *old = (*next)->message.m;

				if (old[0] == new[0] && old[3] == new[3])
				{
					old[4] = new[4];
					msg = NULL;
					break;
				}
				next = &((*next)->next);
			}
		}
		else if (new[0] == WM_ARROWED)
		{
			while (*next)
			{
				short *old = (*next)->message.m;

				if (old[0] == new[0] && old[3] == new[3])
				{
					short old_t = old[4] & 0xf, new_t = new[4] & 0xf;

					if (old_t == new_t)
					{
						short old_a = old[4] >> 8, new_a = new[4] >> 8;

						if ((old_a | new_a))
						{
							if (!old_a)
								old_a++;
							if (!new_a)
								new_a++;
							new_a += old_a;
							old[4] = (old_t | (new_a << 8));
							old[5] = new[5];
							old[6] = new[6];
							old[7] = new[7];
						}
						msg = NULL;
						break;
					}
				}
				next = &((*next)->next);
			}
		}
		else
		{
			while (*next)
				next = &((*next)->next);
		}
	}
	else
	{
		while (*next)
			next = &((*next)->next);
	}

	/* If still there */
	if (msg)
	{
		new_msg = kmalloc(sizeof(*new_msg));
		if (new_msg)
		{
			new_msg->message = *msg;
			if (qmflags & QMF_PREPEND)
			{
				new_msg->next = *queue;
				*queue = new_msg;
			}
			else
			{
				/* Fill in the new pending list entry with the message */
				*next = new_msg;
				new_msg->next = NULL;
			}
			DIAG((D_m, NULL, "Queued message %s for %s", pmsg(msg->s.msg), c_owner(client)));
		}
	}
}

/*
 * Context independant.
 * queue a message for dest_client
 */
static void
queue_message(int lock, struct xa_client *client, short amq, short qmf, union msg_buf *msg)
{

	amq &= ~AMQ_ANYCASE;

	switch (amq)
	{
		case AMQ_NORM:
#if GENERATE_DIAGS
			add_msg_2_queue(client, &client->msg, msg, qmf);
#else
			add_msg_2_queue(&client->msg, msg, qmf);
#endif
			break;
		case AMQ_REDRAW:
#if GENERATE_DIAGS
			add_msg_2_queue(client, &client->rdrw_msg, msg, qmf);
#else
			add_msg_2_queue(&client->rdrw_msg, msg, qmf);
#endif
			break;
		case AMQ_LOSTRDRW:
#if GENERATE_DIAGS
			add_msg_2_queue(client, &client->lost_rdrw_msg, msg, qmf);
#else
			add_msg_2_queue(&client->lost_rdrw_msg, msg, qmf);
#endif
			break;
		case AMQ_CRITICAL:
#if GENERATE_DIAGS
			add_msg_2_queue(client, &client->crit_msg, msg, qmf);
#else
			add_msg_2_queue(&client->crit_msg, msg, qmf);
#endif
			break;
		case AMQ_IREDRAW:
#if GENERATE_DIAGS
			add_msg_2_queue(client, &client->irdrw_msg, msg, qmf);
#else
			add_msg_2_queue(&client->irdrw_msg, msg, qmf);
#endif
			break;
	}
}

#if 0
static void
add_lost_rect(struct xa_client *client, GRECT *r)
{
	struct xa_rect_list **rl = client->lost_redraws.start;

	while (rl)
	{

}
#endif

/*
 * Context indipendant.
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
send_a_message(int lock, struct xa_client *dest_client, short amq, short qmf, union msg_buf *msg)
{
	if (dest_client == NULL || (dest_client->status & CS_EXITING))
	{
		DIAG((D_appl, NULL, "WARNING: Invalid target for send_a_message()"));
		return;
	}

	if (amq != AMQ_IREDRAW  && !(amq & AMQ_ANYCASE))
	{
		if (dest_client->status & (CS_LAGGING | CS_FORM_ALERT | CS_FORM_DO | CS_FSEL_INPUT))
		{
			if (msg->m[0] == WM_REDRAW)
			{
				struct xa_vdi_settings *v = dest_client->vdi_settings;
				GRECT *r = (GRECT *)&msg->m[4];

#if 0
				add_lost_rect(client, r);
#endif
				queue_message(lock, dest_client, AMQ_LOSTRDRW, qmf|QMF_NOCOUNT|QMF_CHKDUP, msg);

				dest_client->status |= CS_MISS_RDRW;
 				/* if (dest_client->status & (CS_LAGGING)) */
				{
					hidem();
					(*v->api->set_clip)(v, r);

					(*v->api->f_color)(v, 9);
					(*v->api->wr_mode)(v, MD_REPLACE);
					(*v->api->f_interior)(v, FIS_SOLID);
					(*v->api->bar)(v, 0, r->g_x, r->g_y, r->g_w, r->g_h);

					(*v->api->clear_clip)(v);
					showm();
					C.redraws++;
					kick_mousemove_timeout();
				}
			}
			else
			{
				queue_message(lock, dest_client, amq, qmf, msg);
			}

			if (dest_client->status & CS_FORM_ALERT)
			{
				DIAG((D_appl, dest_client, "send_a_message: Client %s is in form_alert "
					"- AES message discarded!", dest_client->name));
			}
			if (dest_client->status & CS_LAGGING)
			{
				DIAG((D_appl, dest_client, "send_a_message: Client %s is lagging - "
					"AES message discarded!", dest_client->name));
			}
			return;
		}
	}
	queue_message(lock, dest_client, amq, qmf, msg);
	Unblock(dest_client, 1);
}

/*
 * AES internal msgs
 */
void
send_app_message(
	int lock,
	struct xa_window *wind,
	struct xa_client *to, /* if different from wind->owner */
	short amq, short qmf,
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7)
{
	union msg_buf m;
	short *p = m.m;

	if (to == NULL)
		to = wind->owner;

	*p++ = mp0;				/* message number */
	*p++ = amq != AMQ_IREDRAW ? (mp1 > 0 ? mp1 : C.AESpid) : mp1;	/* source pid     */
	*p++ = mp2;
	*p++ = mp3;
	*p++ = mp4;
	*p++ = mp5;
	*p++ = mp6;
	*p++ = mp7;

	send_a_message(lock, to, amq, qmf, &m);
}
