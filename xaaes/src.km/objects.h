/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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

#ifndef _objects_h 
#define _objects_h

#include "global.h"

void init_objects(void);

/* Externally used routines */
void display_object(enum locks lock, XA_TREE *wt, int object, short parent_x, short parent_y, int which);
int draw_object_tree(enum locks lock, XA_TREE *wt, OBJECT *tree, int item, int depth, int which);
int object_offset(OBJECT *tree, int object, short dx, short dy, short *mx, short *my);
void change_object(enum locks lock, XA_TREE *wt, OBJECT *tree, int i, const RECT *r, int state, bool draw);
int find_object(OBJECT *tree, int object, int depth, short mx, short my, short dx, short dy);
bool ed_char(XA_TREE *wt, TEDINFO *ed_txt, ushort keycode);
XA_TREE *check_widget_tree(enum locks lock, struct xa_client *client, OBJECT *tree);
int edit_object(enum locks lock,
		struct xa_client *client,
		int func,
		XA_TREE *wt, OBJECT *form,
		int ed_obj, int keycode,
		short *newpos);

void set_ob_spec(OBJECT *root, int s_ob, unsigned long cl);
OBSPEC *get_ob_spec(OBJECT *ob);
void redraw_object(enum locks lock, XA_TREE *wt, int );
void wr_mode(int);
void l_color(int m);
void f_color(int m);
void t_color(int m);
void t_effect(int m);
void t_font(int p, int f);
void f_interior(int m);
void f_style(int m);
bool d3_any(OBJECT *ob);
bool d3_indicator(OBJECT *ob);
bool d3_foreground(OBJECT *ob);
bool d3_background(OBJECT *ob);
bool d3_activator(OBJECT *ob);
void d3_pushbutton(int d, RECT *r, BFOBSPEC *col, int state, int thick, int mode);

/* Clipping stuff */
void set_clip(const RECT *r);
void clear_clip(void);

/* Internal utility routines */
void shadow_object(int d, int state, RECT *r, int colour, int border_thick);
void draw_2d_box(short x, short y, short w, short h, int border_thick, int colour);
void g2d_box(int b, RECT *r, int colour);
void object_rectangle(RECT *c, OBJECT *ob, int i, short dx, short dy);
void object_area     (RECT *c, OBJECT *ob, int i, short dx, short dy);
void write_menu_line(RECT *cl);
void write_selection(int d, RECT *r);
long calc_back(const RECT *r, int planes);

/* HR: intermediate level draw functions & misc. */
void adjust_size(int d, RECT *r);
void p_gbar(int d, const RECT *r);
void bar(int d,  short x, short y, short w, short h);
void p_bar(int d, short x, short y, short w, short h);	/* for perimeter = 1 */
void gbar (int d, const RECT *r);
void gbox(int d, const RECT *r);
void box  (int d, short x, short y, short w, short h);
void line (short x, short y, short x1,short y1, int col);
void tl_hook(int d, const RECT *r, int col);
void br_hook(int d, const RECT *r, int col);
void chiseled_gbox(int d, const RECT *r);
void t_extent(char *t, short *w, short *h);
void rtopxy(short *p, const RECT *r);
void ritopxy(short *p, short x, short y, short w, short h);
void *form_save(int d, RECT r, void *area);
void form_restore(int d, RECT r, void *area);
void form_copy(const RECT *fr, const RECT *to);
void strip_name(char *to, char *fro);
void cramped_name(void *s, char *t, int w);
char *clipped_name(void *s, char *t, int w);
int thickness(OBJECT *ob);
bool is_menu(OBJECT *tree);
void deselect(OBJECT *tree, int item);
char *object_type(OBJECT *tree, int t);
bool is_spec(OBJECT *tree, int item);
void write_disable(RECT *r, int colour);

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

/* XXX in my_aes.c */
int get_parent(OBJECT *ob, int item);

#endif /* _objects_h */
