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

#ifndef _widgets_h
#define _widgets_h

#include "global.h"
#include "xa_types.h"

void CE_winctxt(int lock, struct c_event *ce, short cancel);
void cancel_winctxt_popup(int lock, struct xa_window *wind, struct xa_client *client);

//void	setup_widget_theme(struct xa_client *client, struct xa_widget_theme *xwt);
void init_client_widget_theme(struct xa_client *client);
void exit_client_widget_theme(struct xa_client *client);

#define MI_IGN	32767

//COMPASS compass(short d, short x, short y, RECT r);

void		display_widget(int lock, struct xa_window *wind, XA_WIDGET *widg, struct xa_rect_list *rl);
void		standard_widgets(struct xa_window *wind, XA_WIND_ATTR tp, bool keep_stuff);
void _cdecl	redraw_toolbar(int lock, struct xa_window *wind, short item);
void		set_toolbar_coords(struct xa_window *wind, const RECT *r);

void		set_toolbar_handlers(const struct toolbar_handlers *th, struct xa_window *wind, struct xa_widget *widg, struct widget_tree *wt);

#define STW_ZEN	1
#define STW_GOC	2 /* Get object coordinates */
#define STW_COC 4 /* Center object coordinates */
#define STW_SWC 8 /* Set window coordinates */
XA_TREE *set_toolbar_widget(int lock, struct xa_window *wind, struct xa_client *owner, OBJECT *obj, struct xa_aes_object item, short properties, short flags, const struct toolbar_handlers *th, const RECT *r);

void	remove_widget(int lock, struct xa_window *wind, int tool);
void	rp_2_ap_cs(struct xa_window *wind, XA_WIDGET *widg, RECT *r);
void *	rp_2_ap(struct xa_window *wind, XA_WIDGET *widg, RECT *r);

XA_TREE *	_cdecl	obtree_to_wt(struct xa_client *client, OBJECT *obtree);
void		_cdecl 	init_widget_tree(struct xa_client *client, struct widget_tree *wt, OBJECT *obtree);
XA_TREE *	_cdecl	new_widget_tree(struct xa_client *client, OBJECT *obtree);
void free_wtlist(struct xa_client *client);
//void remove_from_wtlist(XA_TREE *wt);
bool _cdecl	remove_wt(XA_TREE *wt, bool force);

//XA_TREE *check_widget_tree(int lock, struct xa_client *client, OBJECT *obtree);

void	calc_work_area(struct xa_window *wind);
short	checkif_do_widgets(int lock, struct xa_window *w, short x, short y, XA_WIDGET **ret);
int	do_widgets(int lock, struct xa_window *w, XA_WIND_ATTR mask, const struct moose_data *md);
long	pix_to_sl(long p, long s);
long	sl_to_pix(long s, long p);
int	XA_slider(struct xa_window *w, int which, long total, long visible, long start);
bool	m_inside(short x, short y, RECT *o);
void	redraw_menu(int lock);
void	done_widget_active(struct xa_window *wind, int i);
bool iconify_action(int lock, struct xa_window *wind, const struct moose_data *md);

void	free_xawidget_resources(struct xa_widget *widg);

RECT	iconify_grid(int i);

DrawWidg display_object_widget; /* for desktop */

void	remove_widget_active(struct xa_client *client);

void	do_widget_repeat(void);
void	do_active_widget(int lock, struct xa_client *client);
void	set_winmouse(short x, short y);
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

static inline bool
wdg_is_inst(struct xa_widget *widg)
{
	return ((widg->m.properties & WIP_INSTALLED));
}

static inline bool
wdg_is_act(struct xa_widget *widg)
{
	return ( (widg->m.properties & (WIP_ACTIVE|WIP_INSTALLED)) == (WIP_ACTIVE|WIP_INSTALLED) );
}

static inline struct xa_widget *
usertoolbar_installed(struct xa_window *wind)
{
	struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);
	if ( (wind->active_widgets & TOOLBAR) &&
		 !(widg->m.properties & WIP_NOTEXT) &&
		 wdg_is_inst(widg))
		return widg;
	else
		return NULL;
}

#define BELOW_FOREIGN_MENU(y)	((cfg.menu_bar&1) && !cfg.menu_ontop && desktop_owner() != C.Aes && y < get_menu_height() )

#endif /* _widgets_h */
