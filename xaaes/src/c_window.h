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

#ifndef _c_window_h
#define _c_window_h

#include "global.h"


XA_WINDOW *create_window(LOCK lock, SendMessage *message_handler,
			 XA_CLIENT *client, bool nolist, XA_WIND_ATTR tp,
			 WINDOW_TYPE dial,
			 int frame, int thinframe, bool thinwork,
			 const RECT r, const RECT *max, RECT *rem);

XA_WINDOW *find_window(LOCK lock, int x, int y);
XA_WINDOW *get_wind_by_handle(LOCK lock, int h);
XA_WINDOW *pull_wind_to_top(LOCK lock, XA_WINDOW *w);
XA_WINDOW *top_w(LOCK lock);
XA_WINDOW *root_w(LOCK lock);

bool	close_window(LOCK lock, XA_WINDOW *wind);
int	open_window(LOCK lock, XA_WINDOW *w, RECT r);
void	send_wind_to_bottom(LOCK lock, XA_WINDOW *w);
void	move_window(LOCK lock, XA_WINDOW *wind, int newstate, int x, int y, int w, int h);
void	delete_window(LOCK lock, XA_WINDOW *wind);
void	display_window(LOCK lock, int which, XA_WINDOW *w, RECT *clip);
void	draw_window(LOCK lock, XA_WINDOW *wind);
void	display_windows_below(LOCK lock, const RECT *r, XA_WINDOW *w);

RECT	free_icon_pos(LOCK lock);
RECT	calc_window(LOCK lock, XA_CLIENT *client, int request,
		    unsigned long tp, int mg, int thinframe, bool thinwork,
		    RECT r);

void	top_window(LOCK lock, XA_WINDOW *w, XA_CLIENT *desk_menu_owner);
void	bottom_window(LOCK lock, XA_WINDOW *w);
void	after_top(LOCK lock, bool untop);
void	remove_windows(LOCK lock, XA_CLIENT *client);
void	inside_root(RECT *r, struct options *o);
void	unhide_window(LOCK lock, XA_WINDOW *wind);
void	clear_wind_handles(void);
void	send_ontop(LOCK lock);
void	send_untop(LOCK lock, XA_WINDOW *wind);

bool	is_hidden(XA_WINDOW *wind);
bool	unhide(XA_WINDOW *w, short *x, short *y);

XA_WIND_ATTR hide_move(struct options *o);

#endif /* _c_window_h */
