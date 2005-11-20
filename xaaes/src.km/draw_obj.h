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

#define DRW_CURSOR	1

void init_objects(void);

void init_ob_render(void);
void exit_ob_render(void);

bool d3_any(OBJECT *ob);
bool d3_indicator(OBJECT *ob);
bool d3_foreground(OBJECT *ob);
bool d3_background(OBJECT *ob);
bool d3_activator(OBJECT *ob);

void	display_object(enum locks lock, XA_TREE *wt, struct xa_vdi_settings *v, short object, short parent_x, short parent_y, short which);
short	draw_object_tree(enum locks lock, XA_TREE *wt, OBJECT *tree, struct xa_vdi_settings *v, short item, short depth, short *xy, short flags);

/* Internal utility routines */
void shadow_object(struct xa_vdi_settings *v, short d, short state, RECT *r, short colour, short border_thick);
void shadow_area(struct xa_vdi_settings *v, short d, short state, RECT *r, short colour, short x, short y);
void draw_2d_box(struct xa_vdi_settings *v, short x, short y, short w, short h, short border_thick, short colour);

void write_menu_line(struct xa_vdi_settings *v, RECT *cl);
long calc_back(const RECT *r, short planes);

/* intermediate level draw functions & misc. */
void adjust_size(short d, RECT *r);
void r2pxy(short *p, short d, const RECT *r);
void rtopxy(short *p, const RECT *r);
void ri2pxy(short *p, short d, short x, short y, short w, short h);
void ritopxy(short *p, short x, short y, short w, short h);
void form_save(short d, RECT r, void **area);
void form_restore(short d, RECT r, void **area);
void form_copy(const RECT *fr, const RECT *to);

// void enable_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v);
// void disable_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl);
void eor_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl);
void draw_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl, bool rdrw);
void undraw_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl, bool rdrw);
void set_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct objc_edit_info *ei);

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
