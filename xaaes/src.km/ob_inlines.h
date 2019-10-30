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

#ifndef _ob_inlines_h_
#define _ob_inlines_h_

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
static inline OBSPEC *
aesobj_get_spec(struct xa_aes_object *o) { return object_get_spec(aesobj_ob(o)); }

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
	return	(POPINFO *)object_get_spec(ob)->index;
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

#if AGGRESSIVE_INLINING
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
set_aesobj_uplink(OBJECT **t, struct xa_aes_object *c, struct xa_aes_object *s, struct oblink_spec **oblink)
{
	if ((aesobj_type(c) & 0xff) == G_OBLINK)
	{
		struct oblink_spec *obl = object_get_oblink(aesobj_ob(c));
		if (obl)
		{
			obl->save_to_r = *(RECT *)&obl->to.tree[obl->to.item].ob_x;

			obl->to.tree[obl->to.item].ob_x = aesobj_getx(c);
			obl->to.tree[obl->to.item].ob_y = aesobj_gety(c);

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
#else
bool
disable_object(OBJECT *ob, bool set);
bool
set_aesobj_uplink(OBJECT **t, struct xa_aes_object *c, struct xa_aes_object *s, struct oblink_spec **oblink);
short
ob_offset(OBJECT *obtree, struct xa_aes_object object, short *mx, short *my);
bool
set_aesobj_downlink(OBJECT **t, struct xa_aes_object *c, struct xa_aes_object *s, struct oblink_spec **oblink);
void
clean_aesobj_links(struct oblink_spec **oblink);
#endif
#endif /* _ob_inlines_h_ */
