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

#ifndef _xa_evnt_h
#define _xa_evnt_h

#include "global.h"
#include "xa_types.h"

void exec_iredraw_queue(int lock, struct xa_client *client);
short checkfor_mumx_evnt(struct xa_client *client, bool is_locker, short x, short y);
void get_mbstate(struct xa_client *client, struct mbs *mbs);
bool check_queued_events(struct xa_client *client);
void cancel_mutimeout(struct xa_client *client);

#define MIN_TIMERVAL	5	/* ms */

AES_function
	XA_xevnt_multi,
	XA_evnt_button,
	XA_evnt_keybd,
	XA_evnt_mesag,
	XA_evnt_mouse,
	XA_evnt_multi,
	XA_evnt_timer;

#endif /* _xa_evnt_h */
