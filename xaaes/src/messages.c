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

#include <mintbind.h>

#include "xa_types.h"
#include "xa_global.h"
#include "xa_evnt.h"
#include "vaproto.h"
#include "xalloc.h"

#if GENERATE_DIAGS
static char xmsgs[][32] =
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

#if 0
/* messages series 0x4700 */
static char va_msgs[][32] =
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
#endif

static char unknown[32];

char *
pmsg(short m)
{
	char *ret;
	if (m >= WM_REDRAW && m <= AP_DRAGDROP)
		ret = xmsgs[m - WM_REDRAW];
#if 0
	else if (m >= AV_PROTOKOLL and m < VA_HIGH)
		ret = va_msgs[m - AV_PROTOKOLL];
#endif
	else
	{
		sdisplay(unknown, "%d(%x)", m, m);
		ret = unknown;
	}

	return ret;
}
#endif

static bool
is_inside(RECT r, RECT o)
{
	if (   (r.x       < o.x      )
	    || (r.y       < o.y      )
	    || (r.x + r.w > o.x + o.w)
	    || (r.y + r.h > o.y + o.h)
	   )
		return false;

	return true;
}

/*
 *	Send an AES message to a client application.
 *  HR: generalized version, which now can be used by appl_write. :-)
 */
void
send_a_message(LOCK lock, short dest, union msg_buf *msg)
{
	XA_CLIENT *dest_client;

	/* Just a precaution - we don't want to send app messages to ourselves...
	 * HR: indeed; messages to the AES itself are switched through the appropriate vector
	 */
	if (dest == C.AESpid) 
		return;

	dest_client = Pid2Client(dest);

	if (dest_client == NULL)
	{
		DIAG((D_appl, NULL, "WARNING: Invalid target pid [%d] for send_a_message()\n", dest));
		return;
	}

	if (dest_client->killed)
		return;

	/* XaAES has left its main loop, so no point queuing messages. */
	if (C.shutdown)
		return;

	Sema_Up(clients);
	
	/* Is the dest client waiting for a message at the moment? */
	if (dest_client->waiting_for & MU_MESAG)
	{
		union msg_buf *clnt_buf;
#if GENERATE_DIAGS
	    if (dest_client->waiting_pb == NULL)
	    {
	    	DIAG((D_m, NULL, "MU_MESAG and NO PB! for %s\n", c_owner(dest_client)));
	    	return;
	    }
#endif
		/* If the client is waiting on a multi, the response is  */
		if (dest_client->waiting_for & XAWAIT_MULTI)
			/* slightly different to the evnt_mesag() response. */
			/* HR: fill out the mouse data!!! */
			multi_intout(dest_client->waiting_pb->intout, MU_MESAG);
		else
			dest_client->waiting_pb->intout[0] = 1;
			
		clnt_buf = (union msg_buf *)(dest_client->waiting_pb->addrin[0]);
		
		if (!clnt_buf)
		{
			DIAG((D_appl, NULL, "WARNING: Invalid target message buffer\n"));
			return;
		}

		/* Fill in the client's message buffer */
		*clnt_buf = *msg;

		DIAG((D_m, NULL, "Send message %s to %s\n", pmsg(msg->s.msg), c_owner(dest_client)));
		/* Write success to client's reply pipe to unblock the process */
		Unblock(dest_client, XA_OK, 22);

	}
	else /* Create a new entry in the destination client's pending messages list */
	{
		XA_AESMSG_LIST *ml = dest_client->msg;
		short *new = msg->m;

#if GENERATE_DIAGS
		if (new[0] == WM_REDRAW)
		{
			DIAG((D_m, NULL, "WM_REDRAW rect %d/%d,%d/%d\n", new[4], new[5], new[6], new[7]));
		}
#endif
		/* There are already some pending messages */
		if (dest_client->msg)
		{
			/* HR 270602: remove engulfed redraw messages. (the simpliest optimization) */
			if (new[0] == WM_REDRAW)
			{
				while (ml)
				{
					short *old = ml->message.m;

					if (   old[0] == WM_REDRAW
					    && old[3] == new[3])
					{
						if (is_inside(*((RECT *)&new[4]), *((RECT *)&old[4])))
						{
							/* new rect inside old rect: forget new. */
							msg = NULL;
							break;
						}
						if (is_inside(*((RECT *)&old[4]), *((RECT *)&new[4])))
						{
							/* old inside new: replace by new. */
							ml->message.s = msg->s;
							msg = NULL;
							break;
						}
					}
					ml = ml->next;
				}
			}
		}

		/* If still there */
		if (msg)
		{
			XA_AESMSG_LIST *new_msg = xmalloc(sizeof(XA_AESMSG_LIST),3);

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

				DIAG((D_m, NULL, "Queued message %s for %s\n", pmsg(msg->s.msg), c_owner(dest_client)));
			}
		}
	}

	Sema_Dn(clients);
}

/* AES internal msgs (All standard) */

void
send_app_message(
	LOCK lock,
	struct xa_window *wind,
	struct xa_client *to, /* if different from wind->owner */
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7)
{
	union msg_buf m;
	short *p = m.m;

	if (to == NULL)
		to = wind->owner;

	*p++ =mp0;			/* message number */
	*p++ =mp1 > 0 ? mp1 : C.AESpid;	/* source pid     */
	*p++ =mp2;
	*p++ =mp3;
	*p++ =mp4;
	*p++ =mp5;
	*p++ =mp6;
	*p++ =mp7;

	send_a_message(lock, to->pid, &m);
}
