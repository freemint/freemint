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

#ifndef _c_window_h
#define _c_window_h

#include "global.h"
#include "xa_types.h"

struct xa_window *create_window(enum locks lock,
				SendMessage *message_handler,
				DoWinMesag *message_doer,
				struct xa_client *client,
				bool nolist,
				XA_WIND_ATTR tp,
				WINDOW_TYPE dial,
				int thinframe,
				bool thinwork,
				const RECT r, const RECT *max, RECT *rem);

void change_window_attribs(enum locks lock,
			   struct xa_client *client,
			   struct xa_window *w,
			   XA_WIND_ATTR tp,
			   bool r_is_wa,
			   RECT r, RECT *remember);

struct xa_window *find_window(enum locks lock, short x, short y);
struct xa_window *get_wind_by_handle(enum locks lock, short h);
struct xa_window *pull_wind_to_top(enum locks lock, struct xa_window *w);
struct xa_window *top_w(enum locks lock);
struct xa_window *root_w(enum locks lock);

bool	close_window(enum locks lock, struct xa_window *wind);
int	open_window(enum locks lock, struct xa_window *w, RECT r);
void	send_wind_to_bottom(enum locks lock, struct xa_window *w);
void	move_window(enum locks lock, struct xa_window *wind, bool blit, short newstate, short x, short y, short w, short h);
void	delete_window(enum locks lock, struct xa_window *wind);
void	delayed_delete_window(enum locks lock, struct xa_window *wind);
void	do_delayed_delete_window(enum locks lock);
void	display_window(enum locks lock, int which, struct xa_window *w, RECT *clip);

DoWinMesag	do_rootwind_msg;

void	draw_window(enum locks lock, struct xa_window *wind);
void	update_windows_below(enum locks lock, const RECT *old, RECT *new, struct xa_window *wl);
void	display_windows_below(enum locks lock, const RECT *r, struct xa_window *w);
void	redraw_client_windows(enum locks lock, struct xa_client *client);

RECT	free_icon_pos(enum locks lock, struct xa_window *ignore);
RECT	calc_window(enum locks lock, struct xa_client *client, int request,
		    unsigned long tp, int thinframe, bool thinwork,
		    RECT r);

void	top_window(enum locks lock, struct xa_window *w, struct xa_client *desk_menu_owner);
void	bottom_window(enum locks lock, struct xa_window *w);
void	after_top(enum locks lock, bool untop);
void	remove_windows(enum locks lock, struct xa_client *client);
void	inside_root(RECT *r, struct options *o);

void	iconify_window(enum locks lock, struct xa_window *wind, RECT *r);
void	uniconify_window(enum locks lock, struct xa_window *wind, RECT *r);

void	hide_window(enum locks lock, struct xa_window *wind);
void	unhide_window(enum locks lock, struct xa_window *wind);
void	clear_wind_handles(void);

void	send_moved(enum locks lock, struct xa_window *wind, short amq, RECT *r);
void	send_sized(enum locks lock, struct xa_window *wind, short amq, RECT *r);
void	send_redraw(enum locks lock, struct xa_window *wind, RECT *r);

void	send_ontop(enum locks lock);
void	send_untop(enum locks lock, struct xa_window *wind);

bool	is_topped(struct xa_window *wind);
bool	is_hidden(struct xa_window *wind);
bool	unhide(struct xa_window *w, short *x, short *y);

void	set_and_update_window(struct xa_window *wind, bool blit, RECT *new);

XA_WIND_ATTR hide_move(struct options *o);

#endif /* _c_window_h */
