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

#ifndef _messages_h
#define _messages_h

/*-----------------------------------------------------------------
 * Lock control
 *-----------------------------------------------------------------*/

enum locks
{
	NOLOCKS   = 0x000,
	appl      = 0x001,
	newclient = 0x002,
	trap      = 0x004,
	winlist   = 0x008,
	desk      = 0x010,
	clients   = 0x020,
	fsel      = 0x040,
	lck_update  = 0x080,
	mouse     = 0x100,
	envstr    = 0x200,
	pending   = 0x400,
	NOLOCKING = -1
};
typedef enum locks LOCK;

union msg_buf
{
	short m[8];
	struct
	{
		short msg, src, m2, m3;
		void *p1, *p2;
	} s;
};

void send_a_message(LOCK lock, short dest, union msg_buf *msg);

struct xa_window;
struct xa_client;

typedef void
SendMessage(
	LOCK lock,
	struct xa_window *wind,
	struct xa_client *to, /* if different from wind->owner */
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7);

SendMessage send_app_message;

#endif /* _messages_h */
