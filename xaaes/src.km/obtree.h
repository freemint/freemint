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

#ifndef _obtree_h
#define _obtree_h

#include "xa_types.h"
#include "mt_gem.h"
#include "ob_inlines.h"

bool			validate_obtree(struct xa_client *c, OBJECT *o, char *f);
bool			object_have_spec(OBJECT *ob);
void			object_set_spec(OBJECT *ob, unsigned long cl);
//void			aesobj_set_spec(struct xa_aes_object *o, unsigned long cl);
bool			object_has_tedinfo(OBJECT *ob);
//bool			aesobj_has_tedinfo(struct xa_aes_object *o);
bool			object_has_freestr(OBJECT *ob);
//bool			aesobj_has_freestr(struct xa_aes_object *o);
bool			object_is_editable(OBJECT *ob, short flags, short state);
bool			aesobj_is_editable(struct xa_aes_object *o, short flags, short state);
TEDINFO *		object_get_tedinfo(OBJECT *ob, XTEDINFO **x);
//TEDINFO *		aesobj_get_tedinfo(struct xa_aes_object *o, XTEDINFO **x);
char *			object_get_string(OBJECT *ob);


bool			obj_is_transparent(struct widget_tree *wt, OBJECT *ob, bool progdef_is_transparent);
// short			object_thickness(OBJECT *ob);
// void			object_offsets(OBJECT *ob, RECT *r);
void			object_spec_wh(OBJECT *ob, short *w, short *h);


CICON *			getbest_cicon(CICONBLK *ciconblk);

OBJECT * _cdecl	duplicate_obtree(struct xa_client *client, OBJECT *obtree, short start);
void		free_obtree_resources(struct xa_client *client, OBJECT *obtree);
void _cdecl	free_object_tree(struct xa_client *client, OBJECT *obtree);

OBJECT *	create_popup_tree(struct xa_client *client, short type, short nobjs, short mw, short mh, void *(*cb)(short item, void **data), void **data);

//void	foreach_object(OBJECT *tree, struct xa_aes_object parent, struct xa_aes_object start, short stopf, short stops, bool(*f)(OBJECT *obtree, short obj, void *ret), void *data);

short			ob_count_objs(OBJECT *obtree, short start, short depth);
// struct xa_aes_object	ob_get_parent(OBJECT *t, struct xa_aes_object object);
struct xa_aes_object	ob_get_parent(OBJECT *t, struct xa_aes_object object);
short			ob_remove(OBJECT *obtree, short object);
short			ob_add(OBJECT *obtree, short parent, short aobj);
void			ob_order(OBJECT *obtree, short object, ushort pos);
struct xa_aes_object	ob_find_flag(OBJECT *obtree, short f, short mf, short stopf);
void      ob_set_wind(OBJECT *tree, short f, short t );
short			ob_count_flag(OBJECT *obtree, short f, short mf, short stopf, short *count);
short			ob_count_any_flag(OBJECT *obtree, short f, short mf, short stopf, short *count);
struct xa_aes_object	ob_find_flst(OBJECT *obtree, short f, short s, short mf, short ms, short stopf, short stops);
struct xa_aes_object	ob_find_any_flst(OBJECT *obtree, short f, short s, short mf, short ms/*, short stopf, short stops*/);
/* Definitions for 'flags' parameter to ob_find_next_any_flagstate() */
#define OBFIND_VERT		0x0000	/* To make source more readable */
#define OBFIND_HOR		0x0001	/* horizontal search, else vertical search */
#define OBFIND_UP		0x0000	/* To make source more readable */
#define OBFIND_DOWN		0x0002  /* down/right search, else up/left search */
#define OBFIND_EXACTFLAG	0x0004	/* All bits in 'f' must match */
#define OBFIND_EXACTSTATE	0x0008	/* All bits in 's' must match */
#define OBFIND_FIRST		0x0010	/* Find object closes to top, ignores OBFIND_[HOR/DOWN] */
#define OBFIND_LAST		0x0020	/* Find object closes to bottom, ignores OBFIND_[HOR/DOWN/FIRST] */
#define OBFIND_HIDDEN		0x0040	/* Dont skip hidden trees */
#define OBFIND_NOWRAP		0x0080
struct xa_aes_object	ob_find_next_any_flagstate(struct widget_tree *wt, struct xa_aes_object parent, struct xa_aes_object start, short f, short mf, short s, short ms, short stopf, short stops, short flags);
//struct xa_aes_object	ob_find_next_any_flag(OBJECT *obtree, short start, short f);
short	ob_find_prev_any_flag(OBJECT *obtree, short start, short f);
struct xa_aes_object	ob_find_cancel(OBJECT *ob);
struct xa_aes_object ob_find_type(OBJECT *tree, short t );

struct sc
{
	short o;
	char c;
};
struct sc2
{
	short objs;
	struct sc *sc;
};

void			ob_fix_shortcuts(OBJECT *obtree, bool not_hidden, struct sc2 *scp);
struct xa_aes_object 	ob_find_shortcut(OBJECT *tree, ushort nk, short stop_type);

short	ob_offset(OBJECT *obtree, struct xa_aes_object object, short *mx, short *my);
// void	ob_rectangle(OBJECT *obtree, struct xa_aes_object obj, RECT *c);
// void	ob_area(OBJECT *obtree, struct xa_aes_object obj, RECT *c);
void	obj_border_diff(struct widget_tree *wt, struct xa_aes_object obj1, struct xa_aes_object obj2, RECT *r);
void	ob_spec_xywh(OBJECT *obtree, short obj, RECT *r);
// short	ob_find(OBJECT *obtree, short object, short depth, short mx, short my);

bool	obtree_is_menu(OBJECT *tree);
bool	obtree_has_default(OBJECT *obtree);
bool	obtree_has_exit(OBJECT *obtree);
bool	obtree_has_touchexit(OBJECT *obtree);

#define OB_IF_RESET	1
#define OB_IF_ONLY_EDITS 2
void	obj_init_focus(XA_TREE *wt, short flags);
void	obj_set_g_popup(XA_TREE *swt, struct xa_aes_object sobj, POPINFO *pinf);
void	obj_unset_g_popup(XA_TREE *swt, struct xa_aes_object sobj, char *txt);
void	obj_change_popup_entry(struct xa_aes_object obj, short obnum, char *s);
short	obj_offset(XA_TREE *wt, struct xa_aes_object object, short *mx, short *my);
void	obj_rectangle(XA_TREE *wt, struct xa_aes_object object, RECT *c);
void	obj_orectangle(XA_TREE *wt, struct xa_aes_object object, RECT *c);
bool	obj_area(XA_TREE *wt, struct xa_aes_object object, RECT *c);
struct xa_aes_object	obj_find(XA_TREE *wt, struct xa_aes_object object, short depth, short mx, short my, RECT *c);

void	obj_change(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object obj, int transdepth, short state, short flags, bool redraw, const RECT *clip, struct xa_rect_list *r, short dflags);
void	obj_draw(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object obj, int transdepth, const RECT *clip, struct xa_rect_list *r, short flags);
short	obj_edit(XA_TREE *wt, struct xa_vdi_settings *v, short func, struct xa_aes_object obj, short keycode, short pos, char *string, bool redraw, const RECT *clip, struct xa_rect_list *rl, short *ret_pos, struct xa_aes_object *ret_obj);
void _cdecl obj_set_radio_button(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object obj, bool redraw, const RECT *clip, struct xa_rect_list *rl);
struct xa_aes_object _cdecl obj_get_radio_button(XA_TREE *wt, struct xa_aes_object parent, short state);
short	obj_watch(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object obj, short in_state, short out_state, const RECT *clip, struct xa_rect_list *rl);

int copy_string_to_clipbrd( char *text );

#if 0
static inline bool
issel(OBJECT *ob)
{	return (ob->ob_state & OS_SELECTED); }

static inline void
setsel(OBJECT *ob, bool sel)
{
	short state;

	state = ob->ob_state & ~OS_SELECTED;
	if (sel)
		state |= OS_SELECTED;
	ob->ob_state = state;
}
static inline void
setchecked(OBJECT *ob, bool set)
{
	short state = ob->ob_state & ~OS_CHECKED;
	state = ob->ob_state & ~OS_CHECKED;
	if (set)
		state |= OS_CHECKED;
	ob->ob_state = state;
}

static inline bool
disable_object(OBJECT *ob, bool set)
{
	if (set)
	{
		if (!(ob->ob_state & OS_DISABLED)) {
			ob->ob_state |= OS_DISABLED;
			return true;
		}
		else
			return false;
	}
	else
	{
		if ((ob->ob_state & OS_DISABLED)) {
			ob->ob_state &= ~OS_DISABLED;
			return true;
		}
		else
			return false;
	}
}

static inline bool
object_disabled(OBJECT *ob)
{ return (ob->ob_state & OS_DISABLED); }


static inline OBSPEC *
object_get_spec(OBJECT *ob)
{
	if (ob->ob_flags & OF_INDIRECT)
	{
		return ob->ob_spec.indirect;
	}
	else
	{
		return &ob->ob_spec;
	}
}

static inline struct scroll_info *
object_get_slist(OBJECT *ob)
{
	return (struct scroll_info *)object_get_spec(ob)->index;
}

static inline struct oblink_spec *
object_get_oblink(OBJECT *ob)
{
	struct oblink_spec *obl = NULL;

	if ((ob->ob_type & 0xff) == G_OBLINK)
		obl = (struct oblink_spec *)object_get_spec(ob)->index;
	return obl;
}

static inline POPINFO *
object_get_popinfo(OBJECT *ob)
{
	return	(POPINFO *)object_get_spec(ob)->popinfo;
}

static inline char *
object_get_freestr(OBJECT *ob)
{
	return object_get_spec(ob)->free_string;
}

static inline void
object_deselect(OBJECT *ob)
{
	ob->ob_state &= ~OS_SELECTED;
}

static inline bool
set_aesobj_uplink(OBJECT **t, struct xa_aes_object *c, struct xa_aes_object *s, struct oblink_spec **oblink)
{
	if ((aesobj_ob(c)->ob_type & 0xff) == G_OBLINK)
	{
		struct oblink_spec *obl = object_get_oblink(aesobj_ob(c));
		if (obl)
		{
			obl->save_to_r = *(RECT *)&obl->to.tree[obl->to.item].ob_x;

			obl->to.tree[obl->to.item].ob_x = aesobj_ob(c)->ob_x;
			obl->to.tree[obl->to.item].ob_y = aesobj_ob(c)->ob_y;

			obl->d.pmisc[1] = *oblink;
			*oblink = obl;

			obl->savestop = *s;
			*t = obl->to.tree;
			*c = *s = aesobj(obl->to.tree, obl->to.item);
			return true;
		}
	}
	return false;
}
static inline bool
set_aesobj_downlink(OBJECT **t, struct xa_aes_object *c, struct xa_aes_object *s, struct oblink_spec **oblink)
{
	if (*oblink)
	{
		OBJECT *tree = (*oblink)->to.tree + (*oblink)->to.item;

		tree->ob_x = (*oblink)->save_to_r.x;
		tree->ob_y = (*oblink)->save_to_r.y;
		tree->ob_width = (*oblink)->save_to_r.w;
		tree->ob_height = (*oblink)->save_to_r.h;

		*t = (*oblink)->from.tree;
		*c = aesobj((*oblink)->from.tree, (*oblink)->from.item);
		*s = (*oblink)->savestop;
		*oblink = (*oblink)->d.pmisc[1];
		return true;
	}
	return false;
}

static inline void
clean_aesobj_links(struct oblink_spec **oblink)
{
	while (*oblink)
	{
		OBJECT *tree = (*oblink)->to.tree + (*oblink)->to.item;

		tree->ob_x = (*oblink)->save_to_r.x;
		tree->ob_y = (*oblink)->save_to_r.y;
		tree->ob_width = (*oblink)->save_to_r.w;
		tree->ob_height = (*oblink)->save_to_r.h;

		*oblink = (*oblink)->d.pmisc[1];
	}
}
#endif

#endif /* _obtree_h */
