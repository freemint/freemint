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

#ifndef _c_window_h
#define _c_window_h

#include "global.h"


struct xa_window *create_window(enum locks lock, SendMessage *message_handler,
				struct xa_client *client, bool nolist, XA_WIND_ATTR tp,
				WINDOW_TYPE dial,
				int frame, int thinframe, bool thinwork,
				const RECT r, const RECT *max, RECT *rem);

struct xa_window *find_window(enum locks lock, int x, int y);
struct xa_window *get_wind_by_handle(enum locks lock, int h);
struct xa_window *pull_wind_to_top(enum locks lock, struct xa_window *w);
struct xa_window *top_w(enum locks lock);
struct xa_window *root_w(enum locks lock);

bool	close_window(enum locks lock, struct xa_window *wind);
int	open_window(enum locks lock, struct xa_window *w, RECT r);
void	send_wind_to_bottom(enum locks lock, struct xa_window *w);
void	move_window(enum locks lock, struct xa_window *wind, int newstate, int x, int y, int w, int h);
void	delete_window(enum locks lock, struct xa_window *wind);
void	display_window(enum locks lock, int which, struct xa_window *w, RECT *clip);
void	draw_window(enum locks lock, struct xa_window *wind);
void	display_windows_below(enum locks lock, const RECT *r, struct xa_window *w);

RECT	free_icon_pos(enum locks lock);
RECT	calc_window(enum locks lock, struct xa_client *client, int request,
		    unsigned long tp, int mg, int thinframe, bool thinwork,
		    RECT r);

void	top_window(enum locks lock, struct xa_window *w, struct xa_client *desk_menu_owner);
void	bottom_window(enum locks lock, struct xa_window *w);
void	after_top(enum locks lock, bool untop);
void	remove_windows(enum locks lock, struct xa_client *client);
void	inside_root(RECT *r, struct options *o);
void	unhide_window(enum locks lock, struct xa_window *wind);
void	clear_wind_handles(void);
void	send_ontop(enum locks lock);
void	send_untop(enum locks lock, struct xa_window *wind);

bool	is_hidden(struct xa_window *wind);
bool	unhide(struct xa_window *w, short *x, short *y);

XA_WIND_ATTR hide_move(struct options *o);

#endif /* _c_window_h */
