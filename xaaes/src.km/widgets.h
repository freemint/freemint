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

#ifndef _widgets_h
#define _widgets_h

#include "global.h"
#include "xa_types.h"

COMPASS compass(short d, short x, short y, RECT r);

void	fix_default_widgets(void *);
OBJECT *get_widgets(void);
void	display_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg);
void	standard_widgets(struct xa_window *wind, XA_WIND_ATTR tp, bool keep_stuff);
void	redraw_toolbar(enum locks lock, struct xa_window *wind, short item);
void	set_toolbar_coords(struct xa_window *wind);

XA_TREE *set_toolbar_widget(enum locks lock, struct xa_window *wind, struct xa_client *owner, OBJECT *obj, short item, short properties, const struct toolbar_handlers *th);

void	remove_widget(enum locks lock, struct xa_window *wind, int tool);
void	rp_2_ap_cs(struct xa_window *wind, XA_WIDGET *widg, RECT *r);
void *	rp_2_ap(struct xa_window *wind, XA_WIDGET *widg, RECT *r);

XA_TREE * obtree_to_wt(struct xa_client *client, OBJECT *obtree);
XA_TREE * new_widget_tree(struct xa_client *client, OBJECT *obtree);
XA_TREE * set_client_wt(struct xa_client *client, OBJECT *obtree);
void free_wtlist(struct xa_client *client);
void remove_from_wtlist(XA_TREE *wt);
void copy_wt(XA_TREE *d, XA_TREE *s);
void free_wt(XA_TREE *wt);
bool remove_wt(XA_TREE *wt, bool force);

//XA_TREE *check_widget_tree(enum locks lock, struct xa_client *client, OBJECT *obtree);

void	calc_work_area(struct xa_window *wind);
bool	checkif_do_widgets(enum locks lock, struct xa_window *w, XA_WIND_ATTR mask, short x, short y, XA_WIDGET **ret);
int	do_widgets(enum locks lock, struct xa_window *w, XA_WIND_ATTR mask, const struct moose_data *md);
int	pix_to_sl(long p, int s);
int	sl_to_pix(long s, int p);
void	XA_slider(struct xa_window *w, int which, int total, int visible, int start);
bool	m_inside(short x, short y, RECT *o);
void	redraw_menu(enum locks lock);
void	redisplay_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, int state);
void	done_widget_active(struct xa_window *wind, int i);

void	free_xawidget_resources(struct xa_widget *widg);

RECT	iconify_grid(int i);

DisplayWidget display_vslide; /* For d_g_list, should go! */
DisplayWidget display_object_widget; /* for desktop */

void	remove_widget_active(struct xa_client *client);

void	do_widget_repeat(void);
void	do_active_widget(enum locks lock, struct xa_client *client);
void	set_winmouse(void);
short	wind_mshape(struct xa_window *wind, short x, short y);

/*
 * inline some very simple functions
 */

static inline XA_WIDGET *get_widget(struct xa_window *wind, int n) { return &(wind->widgets[n]); }
static inline int bound_sl(int p) { return ((p < 0) ? 0 : ((p > SL_RANGE) ? SL_RANGE : p)); }

static inline bool
is_rect(short x, short y, int fl, RECT *o)
{
	bool in = m_inside(x, y, o);
	bool f = (fl == 0);

	return (f == in);
}

#endif /* _widgets_h */
