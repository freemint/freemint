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

#ifndef _obtree_h
#define _obtree_h

#include "xa_types.h"
#include "mt_gem.h"

bool		object_have_spec(OBJECT *ob);
OBSPEC *	object_get_spec(OBJECT *ob);
void		object_set_spec(OBJECT *ob, unsigned long cl);
bool		object_is_editable(OBJECT *ob);
TEDINFO *	object_get_tedinfo(OBJECT *ob);
void		object_deselect(OBJECT *ob);
bool		object_is_transparent(OBJECT *ob);
short		object_thickness(OBJECT *ob);
void		object_offsets(OBJECT *ob, RECT *r);

CICON *		getbest_cicon(CICONBLK *ciconblk);

OBJECT *	duplicate_obtree(struct xa_client *client, OBJECT *obtree, short start);
void		free_obtree_resources(struct xa_client *client, OBJECT *obtree);
void		free_object_tree(struct xa_client *client, OBJECT *obtree);

short	ob_get_parent(OBJECT *t, short object);
short	ob_remove(OBJECT *obtree, short object);
short	ob_add(OBJECT *obtree, short parent, short aobj);
void	ob_order(OBJECT *obtree, short object, ushort pos);
short	ob_find_flag(OBJECT *obtree, short f, short mf, short stopf);
short	ob_find_any_flag(OBJECT *obtree, short f, short mf, short stopf);
short	ob_count_flag(OBJECT *obtree, short f, short mf, short stopf, short *count);
short	ob_count_any_flag(OBJECT *obtree, short f, short mf, short stopf, short *count);
short	ob_find_flst(OBJECT *obtree, short f, short s, short mf, short ms, short stopf, short stops);
short	ob_find_any_flst(OBJECT *obtree, short f, short s, short mf, short ms, short stopf, short stops);
short	ob_find_next_any_flag(OBJECT *obtree, short start, short f);
short	ob_find_prev_any_flag(OBJECT *obtree, short start, short f);
short	ob_find_cancel(OBJECT *ob);
short	ob_find_shortcut(OBJECT *tree, ushort nk);

short	ob_offset(OBJECT *obtree, short object, short *mx, short *my);
void	ob_rectangle(OBJECT *obtree, short obj, RECT *c);
void	ob_area(OBJECT *obtree, short obj, RECT *c);
void	ob_border_diff(OBJECT *obtree, short obj1, short obj2, RECT *r);
int	ob_spec_xywh(OBJECT *obtree, short obj, RECT *r);
short	ob_find(OBJECT *obtree, short object, short depth, short mx, short my);

bool	obtree_is_menu(OBJECT *tree);
bool	obtree_has_default(OBJECT *obtree);
bool	obtree_has_exit(OBJECT *obtree);
bool	obtree_has_touchexit(OBJECT *obtree);

short	obj_offset(XA_TREE *wt, short object, short *mx, short *my);
void	obj_rectangle(XA_TREE *wt, short object, RECT *c);
void	obj_area(XA_TREE *wt, short object, RECT *c);
short	obj_find(XA_TREE *wt, short object, short depth, short mx, short my, RECT *c);

void	obj_change(XA_TREE *wt, short obj, short state, short flags, bool redraw, const RECT *clip, struct xa_rect_list *r);
void	obj_draw(XA_TREE *wt, short obj, const RECT *clip, struct xa_rect_list *r);
short	obj_edit(XA_TREE *wt, short func, short obj, short keycode, short pos, bool redraw, const RECT *clip, struct xa_rect_list *rl, short *ret_pos, short *ret_obj);
void	obj_set_radio_button(XA_TREE *wt, short obj, bool redraw, const RECT *clip, struct xa_rect_list *rl);
short	obj_watch(XA_TREE *wt, short obj, short in_state, short out_state, const RECT *clip, struct xa_rect_list *rl);

#endif /* _obtree_h */
