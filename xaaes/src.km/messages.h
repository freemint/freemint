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

#ifndef _messages_h
#define _messages_h

#include "global.h"
#include "semaphores.h"
#include "xa_aes.h"

struct xa_client;
struct xa_window;
struct xa_aesmsg_list;

union msg_buf
{
	short m[8];
	struct
	{
		short msg, src, m2, m3;
		void *p1, *p2;
	} s;
	struct
	{
		short msg;
		short xaw;
		void *ptr;
		GRECT rect;
	} irdrw;
	struct
	{
		short m0, m1, m2, m3, m4;
		void *p56;
		short m7;
	} sb;
};

const char *pmsg(short m);

long cancel_aesmsgs(struct xa_aesmsg_list **m);
long cancel_app_aesmsgs(struct xa_client *client);
void cancel_do_winmesag(int lock, struct xa_window *wind);


void send_a_message(int lock, struct xa_client *dest_client, short amq, short qmf, union msg_buf *msg);
//void clip_all_wm_redraws(GRECT *r);
//void deliver_message(int lock, struct xa_client *dest_client, union msg_buf *msg);
//void queue_message(int lock, struct xa_client *dest_client, union msg_buf *msg);

struct xa_window;
struct xa_client;

#define AMQ_NORM	0	/* Normal AES messages queue */
#define AMQ_REDRAW	1	/* AES redraw messages for client */
#define AMQ_CRITICAL	2	/* Critical messages, preceedes the above queue types */
#define AMQ_IREDRAW	3	/* Internal redraw messages */
#define AMQ_LOSTRDRW	4	/* Lost redraw messages */

#define AMQ_ANYCASE	0x8000	/* These messages are queued (on NORMal queue) no matter what state the client is in */

#define QMF_NORM	0
#define QMF_PREPEND	1	/* If set, insert message at start of queue, else add to queue */
#define QMF_CHKDUP	2	/* If set, check for duplicate messages */
#define QMF_NOCOUNT	4	/* If set, do not count */
typedef void
SendMessage(
	int lock,
	struct xa_window *wind,
	struct xa_client *to, /* if different from wind->owner */
	short amq, short flags,
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7);

typedef void
DoWinMesag(struct xa_window *w,
	   struct xa_client *c,
	   short amq, short flags,
	   short *msg);

SendMessage do_winmesag;
SendMessage send_app_message;

#endif /* _messages_h */
