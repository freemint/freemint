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

#ifndef _handler_h
#define _handler_h

#include "global.h"


unsigned long XA_appl_init(LOCK lock, XA_CLIENT *client, AESPB *pb);
unsigned long XA_appl_exit(LOCK lock, XA_CLIENT *client, AESPB *pb);
unsigned long XA_appl_yield(LOCK lock, XA_CLIENT *client, AESPB *pb);
unsigned long XA_wind_update(LOCK lock, XA_CLIENT *client, AESPB *pb);

bool   lock_screen(XA_CLIENT *client, long time_out, short *r, int which);
long unlock_screen(XA_CLIENT *client, int which);
bool   lock_mouse(XA_CLIENT *client, long time_out, short *r, int which);
long unlock_mouse(XA_CLIENT *client, int which);

/* the main AES trap handler */
short _cdecl XA_handler(ushort c, AESPB *pb);

void hook_into_vector(void);
void unhook_from_vector(void);

#endif /* _handler_h */
