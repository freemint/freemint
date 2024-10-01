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

#ifndef _c_window_h
#define _c_window_h

#include "global.h"
#include "xa_types.h"
#include "xa_global.h"

#define FNDW_NOLIST 1
#define FNDW_NORMAL 2

static inline struct xa_window * get_top(void) {return S.open_windows.first;}

struct xa_window * _cdecl create_window(int lock,
				SendMessage *message_handler,
				DoWinMesag *message_doer,
				struct xa_client *client,
				bool nolist,
				XA_WIND_ATTR tp,
				WINDOW_TYPE dial,
				int thinframe,
				bool thinwork,
				const GRECT *r, const GRECT *max, GRECT *rem);

void change_window_attribs(int lock,
			   struct xa_client *client,
			   struct xa_window *w,
			   XA_WIND_ATTR tp,
			   bool r_is_wa,
			   bool insideroot,
			   short noleft,
			   GRECT r, GRECT *remember);

void remove_window_widgets(int lock, int full);
//void wi_remove(struct win_base *b, struct xa_window *w, bool chkfocus);
//void wi_put_first(struct win_base *b, struct xa_window *w);
void wi_move_first(struct win_base *b, struct xa_window *w);
//void wi_move_blast(struct win_base *b, struct xa_window *w);
//void wi_move_belowroot(struct win_base *b, struct xa_window *w);

bool wind_exist(int lock, struct xa_window *wind);
struct xa_window *find_window(int lock, short x, short y, short flags);
struct xa_window *get_wind_by_handle(int lock, short h);
struct xa_window *top_w(int lock);
struct xa_window *root_w(int lock);

//XA_WIND_ATTR fix_wind_kind(XA_WIND_ATTR tp);

bool _cdecl	close_window(int lock, struct xa_window *wind);
int  _cdecl	open_window(int lock, struct xa_window *w, GRECT r);
void _cdecl	send_wind_to_bottom(int lock, struct xa_window *w);
void set_standard_point(struct xa_client *client);
void toggle_menu(int lock, short md);
void _cdecl	move_window(int lock, struct xa_window *wind, bool blit, WINDOW_STATUS newstate, short x, short y, short w, short h);
void _cdecl	delete_window(int lock, struct xa_window *wind);
void _cdecl	delayed_delete_window(int lock, struct xa_window *wind);
void	do_delayed_delete_window(int lock);
void	display_window(int lock, int which, struct xa_window *w, GRECT *clip);

bool clip_off_menu( GRECT *cl );
void	draw_window(int lock, struct xa_window *wind, const GRECT *clip);
void	update_all_windows(int lock, struct xa_window *wl);
void	update_windows_below(int lock, const GRECT *old, GRECT *new, struct xa_window *wl, struct xa_window *wend);
void	redraw_client_windows(int lock, struct xa_client *client);

GRECT	free_icon_pos(int lock, struct xa_window *ignore);

GRECT	w2f(GRECT *delta, const GRECT *in, bool chkwh);
GRECT	f2w(GRECT *delta, const GRECT *in, bool chkwh);
void	delete_wc_cache(struct xa_wc_cache **wcc);
GRECT	calc_window(int lock, struct xa_client *client, int request,
		    XA_WIND_ATTR tp, WINDOW_TYPE dial, int thinframe, bool thinwork,
		    const GRECT *r);

void _cdecl	top_window(int lock, bool snd_untopped, bool snd_ontop, struct xa_window *w);
void	bottom_window(int lock, bool snd_untopped, bool snd_ontop, struct xa_window *w);
void	after_top(int lock, bool untop);
void	remove_windows(int lock, struct xa_client *client);
void	remove_all_windows(int lock, struct xa_client *client);
short	inside_root(GRECT *r, bool noleft);
void	inside_minmax(GRECT *r, struct xa_window *wind);
void	set_winrect(struct xa_window *wind, GRECT *wr, const GRECT *r);

void	iconify_window(int lock, struct xa_window *wind, GRECT *r);
void	uniconify_window(int lock, struct xa_window *wind, GRECT *r);

void	hide_window(int lock, struct xa_window *wind);
void	unhide_window(int lock, struct xa_window *wind, bool check);
void	hide_toolboxwindows(struct xa_client *client);
void	show_toolboxwindows(struct xa_client *client);

void	clear_wind_handles(void);
//void	clear_wind_rectlist(struct xa_window *wind);

void	send_moved	(int lock, struct xa_window *wind, short amq, GRECT *r);
void	send_sized	(int lock, struct xa_window *wind, short amq, GRECT *r);
void	send_reposed	(int lock, struct xa_window *wind, short amq, GRECT *r);
void	send_vslid	(int lock, struct xa_window *wind, short offs);
void	send_hslid	(int lock, struct xa_window *wind, short offs);
void	send_closed	(int lock, struct xa_window *wind);
//void	send_redraw	(int lock, struct xa_window *wind, GRECT *r);
void	send_iredraw	(int lock, struct xa_window *wind, short xaw, GRECT *r);
void	generate_redraws(int lock, struct xa_window *wind, GRECT *r, short flags);

//void	send_ontop(int lock);
//void	send_untop(int lock, struct xa_window *wind);
void	send_topped(int lock, struct xa_window *wind);
void	send_bottomed(int lock, struct xa_window *wind);
void	setwin_untopped(int lock, struct xa_window *wind, bool snd_untopped);
void	setwin_ontop(int lock, struct xa_window *wind, bool snd_ontop);

static inline bool is_topped(struct xa_window *wind){return wind == S.focus ? true : false;}
static inline bool is_toppable(struct xa_window *wind){return (wind->window_status & XAWS_NOFOCUS) ? false : true;}
static inline bool is_hidden(struct xa_window *wind){return (wind->window_status & XAWS_HIDDEN);}
static inline bool is_shaded(struct xa_window *wind){return (wind->window_status & XAWS_SHADED);}
static inline bool is_iconified(struct xa_window *wind){return (wind->window_status & XAWS_ICONIFIED);}
bool	unhide(struct xa_window *w, short *x, short *y);

void	set_window_title(struct xa_window *wind, const char *title, bool redraw);
void	set_window_info(struct xa_window *wind, const char *info, bool redraw);


XA_WIND_ATTR hide_move(struct options *o);

#endif /* _c_window_h */
