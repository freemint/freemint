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

#ifndef _app_man_h
#define _app_man_h

#include "global.h"
#include "xa_types.h"

struct xa_window * get_topwind(enum locks lock, struct xa_client *client, struct xa_window *startw, bool not, WINDOW_STATUS wsmask, WINDOW_STATUS wsvalue);
struct xa_window * next_wind(enum locks lock);
struct xa_client * previous_client(enum locks lock, short exlude);

void set_next_menu(struct xa_client *new, bool do_topwind, bool force);

/* Definition of bits in 'flags' for swap_menu */
#define SWAPM_DESK	0x0001
#define SWAPM_TOPW	0x0002
#define SWAPM_REMOVE	0x0004
void swap_menu(enum locks lock, struct xa_client *, struct widget_tree *, short); // bool, bool, int);
void app_in_front(enum locks lock, struct xa_client *client, bool snd_untopped, bool snd_ontop, bool all_winds);
bool is_infront(struct xa_client *client);
struct xa_client * get_app_infront(void);
struct xa_client * get_app_by_procname(char *name);
void set_active_client(enum locks lock, struct xa_client *client);

void set_reiconify_timeout(enum locks lock);
void cancel_reiconify_timeout(void);
//void block_reiconify_timeout(void);
//void unblock_reiconify_timeout(void);

void hide_app(enum locks lock, struct xa_client *client);
void unhide_app(enum locks lock, struct xa_client *client);
void hide_other(enum locks lock, struct xa_client *client);
void unhide_all(enum locks lock, struct xa_client *client);
void set_unhidden(enum locks lock, struct xa_client *client);
void recover(void);

//XA_TREE *find_menu_bar(enum locks lock);
struct xa_client * next_app(enum locks lock, bool with_window_or_menu, bool no_accessories);
struct xa_client *	find_desktop (enum locks lock, struct xa_client *client, short exlude);
struct xa_client *	focus_owner  (void);
bool wind_has_focus(struct xa_window *wind);
struct xa_client *reset_focus(struct xa_window **new_focus, short flags);

void setnew_focus(struct xa_window *wind, struct xa_window *depend, bool topowner, bool snd_untopped, bool snd_ontop);
void unset_focus(struct xa_window *wind);
struct xa_client *find_focus(bool withlocks, bool *waiting, struct xa_client **locked_client, struct xa_window **keywind);

//bool app_is_hidable(struct xa_client *client);
bool any_hidden(enum locks lock, struct xa_client *client, struct xa_window *exclude);
bool taskbar(struct xa_client *client);

#endif /* _app_man_h */
