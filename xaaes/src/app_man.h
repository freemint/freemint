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

#ifndef _app_man_h
#define _app_man_h

#include "global.h"


XA_CLIENT *next_app	(LOCK lock);
void swap_menu		(LOCK lock, XA_CLIENT *, bool, int);
void app_in_front	(LOCK lock, XA_CLIENT *client);
void hide_app		(LOCK lock, XA_CLIENT *client);
void unhide_app		(LOCK lock, XA_CLIENT *client);
void hide_other		(LOCK lock, XA_CLIENT *client);
void unhide_all		(LOCK lock, XA_CLIENT *client);
void recover		(void);
void find_dead_clients	(LOCK lock);

XA_TREE   *find_menu_bar(LOCK lock);
XA_CLIENT *find_desktop (LOCK lock);
XA_CLIENT *focus_owner  (void);

bool any_hidden		(LOCK lock, XA_CLIENT *client);
bool taskbar		(XA_CLIENT *client);

#endif /* _app_man_h */
