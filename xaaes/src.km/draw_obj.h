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

#ifndef _draw_obj_h
#define _draw_obj_h

#include "global.h"
#include "xa_types.h"
#include "mt_gem.h"

void init_objects(void);

/* Clipping stuff */
void	set_clip(const RECT *r);
void	save_clip(RECT *s);
void	restore_clip(RECT *s);
void	clear_clip(void);

void	wr_mode(short);
void	l_color(short m);
void	f_color(short m);
void	t_color(short m);
void	t_effect(short m);
void	t_font(short p, short f);
void	f_interior(short m);
void	f_style(short m);

bool d3_any(OBJECT *ob);
bool d3_indicator(OBJECT *ob);
bool d3_foreground(OBJECT *ob);
bool d3_background(OBJECT *ob);
bool d3_activator(OBJECT *ob);
void d3_pushbutton(short d, RECT *r, BFOBSPEC *col, short state, short thick, short mode);

void	display_object(enum locks lock, XA_TREE *wt, const RECT *clip, short object, short parent_x, short parent_y, short which);
short	draw_object_tree(enum locks lock, XA_TREE *wt, OBJECT *tree, short item, short depth, short which);
//void	redraw_object(enum locks lock, XA_TREE *wt, int );

/* Internal utility routines */
void shadow_object(short d, short state, RECT *r, short colour, short border_thick);
void draw_2d_box(short x, short y, short w, short h, short border_thick, short colour);
//void g2d_box(int b, RECT *r, int colour);

void write_menu_line(RECT *cl);
void write_selection(short d, RECT *r);
long calc_back(const RECT *r, short planes);

/* intermediate level draw functions & misc. */
void adjust_size(short d, RECT *r);
void p_gbar(short d, const RECT *r);
void bar(short d,  short x, short y, short w, short h);
void p_bar(short d, short x, short y, short w, short h);	/* for perimeter = 1 */
void gbar(short d, const RECT *r);
void gbox(short d, const RECT *r);
void box(short d, short x, short y, short w, short h);
void line(short x, short y, short x1,short y1, short col);
void tl_hook(short d, const RECT *r, short col);
void br_hook(short d, const RECT *r, short col);
void chiseled_gbox(short d, const RECT *r);
void t_extent(const char *t, short *w, short *h);
void rtopxy(short *p, const RECT *r);
void ritopxy(short *p, short x, short y, short w, short h);
void form_save(short d, RECT r, void **area);
void form_restore(short d, RECT r, void **area);
void form_copy(const RECT *fr, const RECT *to);
void strip_name(char *to, const char *fro);
void cramped_name(const void *s, char *t, short w);
const char *clipped_name(const void *s, char *t, short w);

//int thickness(OBJECT *ob);
//bool is_menu(OBJECT *tree);
//void deselect(OBJECT *tree, short item);
//char *object_type(OBJECT *tree, short t);
//bool is_spec(OBJECT *tree, int item);
void write_disable(RECT *r, short colour);

void enable_objcursor(struct widget_tree *wt);
void disable_objcursor(struct widget_tree *wt, struct xa_rect_list *rl);
void eor_objcursor(struct widget_tree *wt, struct xa_rect_list *rl);
void draw_objcursor(struct widget_tree *wt, struct xa_rect_list *rl);
void undraw_objcursor(struct widget_tree *wt, struct xa_rect_list *rl);
void set_objcursor(struct widget_tree *wt);

ObjectDisplay
	d_g_box,
	d_g_boxchar,
	d_g_boxtext,
	d_g_button,
	d_g_cicon,
	d_g_fboxtext,
	d_g_ftext,
	d_g_ibox,
	d_g_icon,
	d_g_image,
	d_g_progdef,
	d_g_slist,
	d_g_string,
	d_g_text,
	d_g_title;

#endif /* _draw_obj_h */
