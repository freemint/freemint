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

#ifndef _widgets_h
#define _widgets_h

void fix_default_widgets(void *);
OBJECT *get_widgets(void);
void display_widget(LOCK lock, XA_WINDOW *wind, XA_WIDGET *widg);
void standard_widgets(XA_WINDOW *wind, XA_WIND_ATTR tp, bool keep_stuff);
XA_TREE *set_toolbar_widget(LOCK lock, XA_WINDOW *wind, OBJECT *obj, int item);
void remove_widget(LOCK lock, XA_WINDOW *wind, int tool);
void *rp_2_ap(XA_WINDOW *wind, XA_WIDGET *widg, RECT *r);
void calc_work_area(XA_WINDOW *wind);
int do_widgets(LOCK lock, XA_WINDOW *w, XA_WIND_ATTR mask, struct moose_data *md);
int pix_to_sl(long p, int s);
int sl_to_pix(long s, int p);
void XA_slider(XA_WINDOW *w, int which, int total, int visible, int start);
bool m_inside(short x, short y, RECT *o);
void redraw_menu(LOCK lock);
void redisplay_widget(LOCK lock, XA_WINDOW *wind, XA_WIDGET *widg, int state);
void done_widget_active(XA_WINDOW *wind, int i);
RECT iconify_grid(int i);
void do_active_widget(LOCK lock, XA_CLIENT *client);

WidgetBehaviour display_vslide;  /* For d_g_list, should go! */
WidgetBehaviour display_object_widget;  /* for desktop */

#ifdef __GNUC__
static inline int bound_sl(int p) { return p < 0 ? 0 : (p > SL_RANGE ? SL_RANGE : p); }
#else
int bound_sl(int p);
#endif

#ifdef __GNUC__
static inline XA_WIDGET *get_widget(XA_WINDOW *wind, int n) { return &wind->widgets[n]; }
#else
#define get_widget(w,n) (&(w)->widgets[n])
#endif

#endif /* _widgets_h */
