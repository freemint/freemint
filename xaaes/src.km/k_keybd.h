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

/*
 * Kernal Message Handler
 */

#ifndef _k_keybd_h
#define _k_keybd_h

#include "global.h"
#include "xa_types.h"


#define KEQ_L 64

struct key_q
{
	struct xa_client *client;
	struct xa_client *locked;
	struct rawkey k;
};
struct key_queue
{
	int cur;
	int last;
	struct key_q q[KEQ_L];
};

void cancel_keyqueue	(struct xa_client *client);
void queue_key		(struct xa_client *client, const struct rawkey *key);
bool unqueue_key	(struct xa_client *client, struct rawkey *key);

int switch_keyboard( const char *tbname );
void keyboard_input	(int lock);

#endif /* _k_keybd_h */
