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

#ifndef _widgets_h
#define _widgets_h

#include "global.h"
#include "xa_types.h"

void fix_default_widgets(void *);
OBJECT *get_widgets(void);
void display_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg);
void standard_widgets(struct xa_window *wind, XA_WIND_ATTR tp, bool keep_stuff);
XA_TREE *set_toolbar_widget(enum locks lock, struct xa_window *wind, OBJECT *obj, int item);
void remove_widget(enum locks lock, struct xa_window *wind, int tool);
void *rp_2_ap(struct xa_window *wind, XA_WIDGET *widg, RECT *r);
void calc_work_area(struct xa_window *wind);
int do_widgets(enum locks lock, struct xa_window *w, XA_WIND_ATTR mask, const struct moose_data *md);
int pix_to_sl(long p, int s);
int sl_to_pix(long s, int p);
void XA_slider(struct xa_window *w, int which, int total, int visible, int start);
bool m_inside(short x, short y, RECT *o);
void redraw_menu(enum locks lock);
void redisplay_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, int state);
void done_widget_active(struct xa_window *wind, int i);
RECT iconify_grid(int i);
void do_active_widget(enum locks lock, struct xa_client *client);

WidgetBehaviour display_vslide;  /* For d_g_list, should go! */
WidgetBehaviour display_object_widget;  /* for desktop */

static inline int bound_sl(int p) { return p < 0 ? 0 : (p > SL_RANGE ? SL_RANGE : p); }
static inline XA_WIDGET *get_widget(struct xa_window *wind, int n) { return &wind->widgets[n]; }

static inline bool
is_rect(short x, short y, int fl, RECT *o)
{
	bool in = m_inside(x, y, o);
	bool f = (fl == 0);

	return (f == in);
}

#endif /* _widgets_h */
