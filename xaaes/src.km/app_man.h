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

#ifndef _app_man_h
#define _app_man_h

#include "global.h"
#include "xa_types.h"

struct xa_window * get_topwind(enum locks lock, struct xa_client *client, bool not, struct xa_window *startw);
struct xa_window * next_wind(enum locks lock);
struct xa_client * next_app(enum locks lock, bool with_window_or_menu, bool no_accessories);
struct xa_client * previous_client(enum locks lock);

void swap_menu(enum locks lock, struct xa_client *, bool, bool, int);
void app_in_front(enum locks lock, struct xa_client *client);
bool is_infront(struct xa_client *client);
struct xa_client * get_app_infront(void);
void set_active_client(enum locks lock, struct xa_client *client);
void hide_app(enum locks lock, struct xa_client *client);
void unhide_app(enum locks lock, struct xa_client *client);
void hide_other(enum locks lock, struct xa_client *client);
void unhide_all(enum locks lock, struct xa_client *client);
void recover(void);

XA_TREE *find_menu_bar(enum locks lock);
struct xa_client *find_desktop (enum locks lock);
struct xa_client *focus_owner  (void);
bool wind_has_focus(struct xa_window *wind);
struct xa_client *find_focus(bool withlocks, bool *waiting, struct xa_client **locked_client, struct xa_window **keywind);

//bool app_is_hidable(struct xa_client *client);
bool any_hidden(enum locks lock, struct xa_client *client);
bool taskbar(struct xa_client *client);

#endif /* _app_man_h */
