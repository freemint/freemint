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

#ifndef _messages_h
#define _messages_h

#include "global.h"
#include "semaphores.h"

struct xa_client;

union msg_buf
{
	short m[8];
	struct
	{
		short msg, src, m2, m3;
		void *p1, *p2;
	} s;
};

const char *pmsg(short m);

void send_a_message(enum locks lock, struct xa_client *dest_client, union msg_buf *msg);
void deliver_message(enum locks lock, struct xa_client *dest_client, union msg_buf *msg);

struct xa_window;
struct xa_client;

typedef void
SendMessage(
	enum locks lock,
	struct xa_window *wind,
	struct xa_client *to, /* if different from wind->owner */
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7);

SendMessage send_app_message;

#endif /* _messages_h */
