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

#include "xa_types.h"
#include "xa_lbox.h"
#include "xa_form.h"
#include "xa_global.h"

#include "c_window.h"
#include "obtree.h"
#include "draw_obj.h"
#include "rectlist.h"
#include "widgets.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_form.h"
#include "scrlobjc.h"

#include "k_mouse.h"


#include "nkcc.h"
#include "xa_user_things.h"
#include "mint/signal.h"


/*
 * WDIALOG FUNCTIONS (lbox)
 *
 * documentation about this can be found in:
 *
 * - the GEMLIB documentation:
 *   http://arnaud.bercegeay.free.fr/gemlib/
 *
 * - the MagiC documentation project:
 *   http://www.bygjohn.fsnet.co.uk/atari/mdp/
 */

#if WDIALOG_LBOX

static void
callout_select(struct xa_lbox_info *lbox, OBJECT *tree, struct lbox_item *item, void *user_data, short obj_index, short last_state)
{
	struct sigaction oact, act;
	struct xa_callout_head *u;

	if (lbox->slct)
	{
		u = umalloc(xa_callout_user.len + sizeof(struct co_lboxsel_parms));
		if (u)
		{
			struct xa_callout_parms *p;
			struct co_lboxsel_parms *lp;

			bcopy(&xa_callout_user, u, xa_callout_user.len);

			u->sighand_p	+= (long)u;
			u->parm_p	 = (void *)((char *)u->parm_p + (long)u);

			p 	= u->parm_p;
			p->func	= (long)lbox->slct;
			p->plen	= sizeof(struct co_lboxsel_parms) >> 1;

			lp	= (struct co_lboxsel_parms *)(&p->parms);

			lp->box		= (long)lbox->lbox_handle;
			lp->tree	= (long)tree;
			lp->item	= (long)item;
			lp->user_data	= (long)user_data;
			lp->obj_index	= obj_index;
			lp->last_state	= last_state;

			cpushi(u, xa_callout_user.len);

			act.sa_handler	= u->sighand_p;
			act.sa_mask	= 0xffffffff;
			act.sa_flags	= SA_RESETHAND;

			p_sigaction(SIGUSR2, &act, &oact);
			DIAGS(("raise(SIGUSR2)"));
			raise(SIGUSR2);
			DIAGS(("handled SIGUSR2 lbox->slct callout"));
			/* restore old handler */
			p_sigaction(SIGUSR2, &oact, NULL);

			ufree(u);
		}
	}
}

static short
callout_set(struct xa_lbox_info *lbox,
	OBJECT *tree,
	struct lbox_item *item,
	short obj_index,
	void *user_data,
	GRECT *rect,
	short first)
{
	struct sigaction oact, act;
	struct xa_callout_head *u;
	short ret = 0;

	if (lbox->set)
	{
		u = umalloc(xa_callout_user.len + sizeof(struct co_lboxset_parms));
		if (u)
		{
			struct xa_callout_parms *p;
			struct co_lboxset_parms *lp;

			bcopy(&xa_callout_user, u, xa_callout_user.len);

			u->sighand_p	+= (long)u;
			u->parm_p	 = (void *)((char *)u->parm_p + (long)u);

			p = u->parm_p;
			p->func = (long)lbox->set;
			p->plen = sizeof(struct co_lboxset_parms) >> 1;

			lp = (struct co_lboxset_parms *)(&p->parms);

			lp->box		= (long)lbox->lbox_handle;
			lp->tree	= (long)tree;
			lp->item	= (long)item;
			lp->obj_index	= obj_index;
			lp->user_data	= (long)user_data;
			lp->rect	= (long)rect;
			lp->first	= first;

			cpushi(u, xa_callout_user.len);

			act.sa_handler	= u->sighand_p;
			act.sa_mask	= 0xffffffff;
			act.sa_flags	= SA_RESETHAND;

			p_sigaction(SIGUSR2, &act, &oact);
			DIAGS(("raise(SIGUSR2)"));
			raise(SIGUSR2);
			DIAGS(("handled SIGUSR2 lbox->set callout"));
			/* restore old handler */
			p_sigaction(SIGUSR2, &oact, NULL);

			ret = (short)p->ret;

			ufree(u);
		}
	}
	return ret;
}

static struct xa_lbox_info *
get_lbox(struct xa_client *client, void *lbox_handle)
{
	struct widget_tree *wt;
	struct xa_lbox_info *ret = NULL, *curr;

	wt = client->wtlist;

	while (wt && !ret)
	{
		curr = wt->lbox;
		while (curr)
		{
			if (curr->lbox_handle == lbox_handle)
			{
				ret = curr;
				break;
			}
			curr = curr->next;
		}
		wt = wt->next;
	}

	DIAG((D_lbox, NULL, "get_lbox: return lbox=%lx for %s",
		(long)ret, client->name));
	return ret;
}

/*
 * Get and attatch resources for a new LBOX to a widget tree
 */
static struct xa_lbox_info *
new_lbox(struct widget_tree *wt, struct xa_vdi_settings *v)
{
	struct xa_lbox_info *lbox;

	lbox = kmalloc(sizeof(*lbox));

	if (lbox)
	{
		bzero(lbox, sizeof(*lbox));

		lbox->next = wt->lbox;
		wt->lbox = lbox;
		lbox->wt = wt;
		lbox->vdi_settings = v;
	}
	return lbox;
}

/*
 * Delete resources taken by an LBOX from widget tree
 */
static void
delete_lbox(struct xa_lbox_info *lbox)
{
	struct xa_lbox_info **prev;
	struct widget_tree *wt;

	wt = lbox->wt;
	prev = &wt->lbox;

	while (*prev && *prev != lbox)
		prev = &(*prev)->next;

	if (*prev)
	{
		*prev = (*prev)->next;
		kfree(lbox);
	}
}

/*
 * Find the first visible item
 */
static struct lbox_item *
get_first_visible_item(struct xa_lbox_info *lbox)
{
	int i;
	struct lbox_item *item;

	for (i = 0, item = lbox->items; (i < lbox->aslide.first_visible) && item; i++, item = item->next)
		;

	DIAGS(("get_first_visible_item: %lx", (unsigned long)item));
	return item;
}
static struct lbox_item *
get_last_visible_item(struct xa_lbox_info *lbox)
{
	int i;
	struct lbox_item *item;

	item  = get_first_visible_item(lbox);

	if (item)
	{
		for (i = 0; (i < (lbox->aslide.num_visible - 1)) && item; i++, item = item->next)
			;
	}

	return item;
}
/*
 * Redraw an LBOX object
 */
static void
redraw_lbox(struct xa_lbox_info *lbox, short o, short depth, RECT *r)
{
	struct xa_window *wind;
	struct widget_tree *wt;
	struct xa_vdi_settings *v = lbox->vdi_settings;
	struct xa_aes_object start, object, obj;
	RECT or;

	wt = lbox->wt;
	wind = wt->wind;

	DIAG((D_lbox, NULL, "redraw_lbox: lbox=%lx, wt=%lx, wind=%lx, parent=%d",
		(unsigned long)lbox, (unsigned long)wt, (unsigned long)wind, lbox->parent));

	obj = aesobj(wt->tree, o);

	obj_area(wt, obj, &or);

	start = obj;
	while (obj_is_transparent(wt, aesobj_ob(&start), false))
	{
		object = ob_get_parent(wt->tree, start);
		if (object.item < 0)
			break;
		start = object;
		depth++;
	}

	lock_screen(wt->owner->p, false);

	if (wind)
	{
		if ((wind->window_status & XAWS_OPEN))
		{
			struct xa_rect_list *rl;
			RECT dr;

			if (!r)
				r = &or;

			rl = wind->rect_list.start;
			while (rl)
			{
				if (xa_rect_clip(&rl->r, r, &dr))
				{
					(*v->api->set_clip)(v, &dr);
					draw_object_tree(0, wt, wt->tree, v, start, depth, NULL, 0);
				}
				rl = rl->next;
			}
		}
	}
	else if (r)
	{
		if (xa_rect_clip(r, &or, &or))
		{
			(*v->api->set_clip)(v, &or);
			draw_object_tree(0, wt, wt->tree, v, start, depth, NULL, 0);
		}
	}
	else
	{
		(*v->api->set_clip)(v, &or);
		draw_object_tree(0, wt, wt->tree, v, start, depth, NULL, 0);
	}
	(*v->api->clear_clip)(v);
	unlock_screen(wt->owner->p);
}

/*
 * Setup LBOX objects
 */
static void
setup_lbox_objects(struct xa_lbox_info *lbox)
{
	struct widget_tree *wt = lbox->wt;
	OBJECT *obtree = wt->tree;
	struct lbox_item *item;
	short x, y, i, ent, first, index, obj, prev, parent = lbox->parent;

	DIAG((D_lbox, NULL, "setup_lbox_objects: wt=%lx, wt->lbox=%lx, entries=%d",
		(unsigned long)wt, (unsigned long)wt->lbox, wt->lbox ? wt->lbox->aslide.entries : -1));

	x = 0;
	y = 0;

	item = lbox->items;

	DIAGS((" --- parent=%d, flags=%x, %d/%d",
		parent, lbox->flags, x, y));

	prev = parent;

	obtree[parent].ob_head = -1;
	obtree[parent].ob_tail = -1;

	for (i = index = ent = 0, first = lbox->aslide.first_visible; i < (lbox->aslide.num_visible + lbox->aslide.first_visible); i++, ent++)
	{
		if (item)
		{
			if (!first)
			{
				obj = lbox->objs[index];

				DIAG((D_lbox, NULL, " --- obj=%d, index=%d, item=%lx, nxtitem=%lx",
					obj, index, (unsigned long)item, (unsigned long)item->next));

				if (prev == parent)
				{
					obtree[parent].ob_head = obj;

				}
				else
				{
					obtree[prev  ].ob_next = obj;

				}
				obtree[parent].ob_tail = obj;
				obtree[obj   ].ob_next = parent;

				prev = obj;

				obtree[obj].ob_x = x;
				obtree[obj].ob_y = y;
				if (lbox->flags & LBOX_VERT)
					y += obtree[obj].ob_height;
				else
					x += obtree[obj].ob_width;

				callout_set(lbox,
					    obtree,
					    item,
					    obj,
					    lbox->user_data,
					    NULL,
					    lbox->bslide.first_visible);

				item->selected = obtree[obj].ob_state;
				index++;
			}
			else
			{
				first--;
			}
			item = item->next;
		}
	}

	while (item)
	{
		ent++;
		item = item->next;
	}

	lbox->aslide.entries = ent;

	DIAGS((" --- Got %d entries", ent));

}

static bool
scroll_up(struct xa_lbox_info *lbox, struct lbox_slide *s, short num)
{
	if (num && (s->num_visible + s->first_visible) < s->entries)
	{
		struct lbox_item *item;
		short i;

		i = s->first_visible + s->num_visible + num;

		if (i > s->entries)
			num -= i - s->entries;

		if (num > 0)
		{
			short idx = 0;

			item = get_first_visible_item(lbox);

			for (i = 0; i < num && item; i++)
			{
				if (idx++ < s->num_visible)
				{
					callout_select(lbox,
							lbox->wt->tree,
							item,
							lbox->user_data,
							0,0);
				}
				item = item->next;
			}
			s->first_visible += num;

			item = get_first_visible_item(lbox);
			for (i = 0; i < s->num_visible && item; i++, item = item->next)
			{
				callout_set(lbox,
					    lbox->wt->tree,
					    item,
					    lbox->objs[i],
					    lbox->user_data,
					    NULL,
					    lbox->bslide.first_visible);
			}

			return true;
		}
	}
	return false;
}

static bool
scroll_down(struct xa_lbox_info *lbox, struct lbox_slide *s, short num)
{
	if (num && s->first_visible)
	{
		struct lbox_item *item, *last;
		short i, j;

		item = get_first_visible_item(lbox);
		last = item;

		if (num > s->first_visible)
			num = s->first_visible;

		i = s->num_visible - num;

		for (j = i; j > 0 && last; j--)
			last = last->next;

		for (j = s->num_visible - i; j > 0 && last; j--)
		{
			callout_select(lbox,
					lbox->wt->tree,
					last,
					lbox->user_data,
					0, 0);
			last = last->next;
		}
		s->first_visible -= num;

		item = get_first_visible_item(lbox);
		for (i = 0; i < s->num_visible && item; i++, item = item->next)
		{
			callout_set(lbox,
				    lbox->wt->tree,
				    item,
				    lbox->objs[i],
				    lbox->user_data,
				    NULL,
				    lbox->bslide.first_visible);
		}

		return true;
	}
	return false;
}

static bool
scroll_right(struct xa_lbox_info *lbox, struct lbox_slide *s, short num)
{
	if (num > 0 && s->bkg >= 0 && s->sld >= 0)
	{
		short newfirst = s->first_visible - num;

		if (newfirst < 0)
			newfirst = 0;

		if (newfirst != s->first_visible)
		{
			short *objs = lbox->objs;
			struct lbox_item *item = get_first_visible_item(lbox);
			short i;

			s->first_visible = newfirst;

			for (i = 0; i < lbox->aslide.num_visible && item; i++, item = item->next)
			{
				callout_set(lbox,
					    lbox->wt->tree,
					    item,
					    objs[i],
					    lbox->user_data,
					    NULL,
					    newfirst);
			}
			return true;
		}

	}
	return false;
}
static bool
scroll_left(struct xa_lbox_info *lbox, struct lbox_slide *s, short num)
{
	if (num > 0 && s->bkg >= 0 && s->sld >= 0)
	{
		short newfirst = s->first_visible + num;

		if ((newfirst + s->num_visible) > s->entries)
		{
			newfirst -= (newfirst + s->num_visible) - s->entries;
			if (newfirst < 0)
				newfirst = 0;
		}

		if (newfirst != s->first_visible)
		{
			short *objs = lbox->objs;
			struct lbox_item *item = get_first_visible_item(lbox);
			short i;

			s->first_visible = newfirst;

			for (i = 0; i < lbox->aslide.num_visible && item; i++, item = item->next)
			{
				callout_set(lbox,
					    lbox->wt->tree,
					    item,
					    objs[i],
					    lbox->user_data,
					    NULL,
					    newfirst);
			}
			return true;
		}
	}
	return false;
}


static void
set_slide_size(struct xa_lbox_info *lbox, struct lbox_slide *s)
{
	long relsize;
	short w, h, parent, child;
	OBJECT *obtree;

	parent = s->bkg;
	child = s->sld;

	if (parent >= 0 && child >= 0)
	{
		obtree = lbox->wt->tree;

		if (s->entries)
			relsize = (long)s->num_visible * 1000 / s->entries;
		else
			relsize = 1000;

		if (relsize > 1000)
			relsize = 1000;


		if (s->flags & LBOX_VERT)
		{
			w = obtree[parent].ob_width;
			h = ((relsize * obtree[parent].ob_height) + 500) / 1000;
		}
		else
		{
			w = ((relsize * obtree[parent].ob_width) + 500) / 1000;
			h = obtree[parent].ob_height;
		}
		DIAG((D_lbox, NULL, "set_slide_size: parent=%d, child=%d, entries=%d, firstvis=%d, visible=%d, relsiz=%ld",
			parent, child, s->entries, s->first_visible, s->num_visible, relsize));
		DIAG((D_lbox, NULL, "old w=%d, old h=%d, new w=%d, new h=%d",
			obtree[child].ob_width, obtree[child].ob_height, w, h));

		obtree[child].ob_width = w + s->ofs.w;
		obtree[child].ob_height = h + s->ofs.h;
	}
}
/*
 * if 'sc' (start coordinate) is negative, use childs X or Y to calculate relative position.
 * if 'sc' is positive, use it instead of childs X or Y to calcuate relative position.
 * 'ofs' ignored on X/Y when using 'sc' instead of childs X or Y.
 */
static long
get_slider_relpos(OBJECT *obtree, short sc, short parent, short child, short orient, RECT *ofs)
{
	short tmp;

	if (orient)
	{
		if (!(tmp = obtree[parent].ob_height - (obtree[child].ob_height - ofs->h)))
			return 0;
		else if (sc < 0)
			return (long)(((long)(obtree[child].ob_y + ofs->y) * 1000) / tmp);
		else
			return (long)(((long)sc * 1000) / tmp);

	}
	else
	{
		if (!(tmp = obtree[parent].ob_width - (obtree[child].ob_width - ofs->w)))
			return 0;
		else if (sc < 0)
			return (long)(((long)(obtree[child].ob_x + ofs->x) * 1000) / tmp);
		else
			return (long)(((long)sc * 1000) / tmp);
	}
}

static void
set_slide_pos(struct xa_lbox_info *lbox, struct lbox_slide *s)
{
	short parent, child, total;
	long relpos = 0;
	OBJECT *obtree = lbox->wt->tree;

	parent = s->bkg;
	child = s->sld;
	if (parent >= 0 && child >= 0)
	{
		short l;
		if ((total = s->entries - s->num_visible) > 0)
		{
			relpos = (long)(((long)s->first_visible * 1000) / total);
			if (relpos > 1000)
				relpos = 1000;
		}

		if (s->flags & LBOX_VERT)
		{
			l = obtree[child].ob_height - s->ofs.h;
			obtree[child].ob_y = ((((long)(obtree[parent].ob_height - l) * relpos) + 500) / 1000) - s->ofs.y;
		}
		else
		{
			l = obtree[child].ob_width - s->ofs.w;
			obtree[child].ob_x = ((((long)(obtree[parent].ob_width -  l) * relpos) + 500) / 1000) - s->ofs.x;
		}
		DIAG((D_lbox, NULL, "set_slide_pos: parent=%d, child=%d, entries=%d, firstvis=%d, visible=%d, relsiz=%ld",
			parent, child, s->entries, s->first_visible, s->num_visible, relpos));
	}
}

static void
drag_slide(struct xa_lbox_info *lbox, struct lbox_slide *s)
{
	short mb, sx, sy;

	check_mouse(lbox->wt->owner, &mb, &sx, &sy);

	if (mb)
	{
		short parent, child, first, max, mx, my;
		OBJECT *obtree = lbox->wt->tree;
		long slider_rpos;
		RECT sl_r;
		RECT lb_r;

		parent = s->bkg;
		child = s->sld;

		/*
		 * new current object = total_object - visible_objects * slider_relpos / 1000
		 */
		obj_area(lbox->wt, aesobj(lbox->wt->tree, parent), &sl_r);
		obj_area(lbox->wt, aesobj(lbox->wt->tree, lbox->parent), &lb_r);

		first = s->first_visible;

		if (s->flags & LBOX_VERT)
		{
			xa_graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
			max = obtree[parent].ob_height - (obtree[child].ob_height - s->ofs.h);
		}
		else
		{
			xa_graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
			max = obtree[parent].ob_width - (obtree[child].ob_width - s->ofs.w);
		}

		while (mb)
		{
			check_mouse(lbox->wt->owner, &mb, &mx, &my);

			if (mb && (sx != mx || sy != mx))
			{
				short nc, oc;

				if (s->flags & LBOX_VERT)
				{
					oc = obtree[child].ob_y + s->ofs.y;
					nc = oc + (my - sy);
					if (nc < 0)
						nc = 0;
					else if (nc > max)
						nc = max;

					if (oc != nc)
					{
						slider_rpos = get_slider_relpos(obtree, nc, parent, child, 1, &s->ofs);
						first = s->entries - s->num_visible;
						if (first > 0)
							first = ((long)first * slider_rpos + 500) / 1000;
						else
							first = 0;

						if (first != s->first_visible)
						{
							nc = first - s->first_visible;
							if (nc < 0)
								s->dr_scroll(lbox, s, -nc);
							else
								s->ul_scroll(lbox, s, nc);

							set_slide_pos(lbox, s);
							hidem();
							redraw_lbox(lbox, parent, 2, &sl_r);
							redraw_lbox(lbox, lbox->parent, 2, &lb_r);
							showm();

							sx = mx;
							sy = my;
						}
					}
				}
				else
				{
					oc = obtree[child].ob_x + s->ofs.x;
					nc = oc + (mx - sx);
					if (nc < 0)
						nc = 0;
					else if (nc > max)
						nc = max;

					if (oc != nc)
					{
						slider_rpos = get_slider_relpos(obtree, nc, parent, child, 0, &s->ofs);
						first = s->entries - s->num_visible;
						if (first > 0)
							first = ((long)first * slider_rpos + 500) / 1000;
						else
							first = 0;

						if (first != s->first_visible)
						{
							nc = first - s->first_visible;
							if (nc < 0)
								s->dr_scroll(lbox, s, -nc);
							else
								s->ul_scroll(lbox, s, nc);

							set_slide_pos(lbox, s);
							hidem();
							redraw_lbox(lbox, parent, 2, &sl_r);
							redraw_lbox(lbox, lbox->parent, 2, &lb_r);
							showm();

							sx = mx;
							sy = my;
						}
					}
				}
			} /* if (mb && sx != mx && sy != mx) */
		} /* while (mb) */
		xa_graf_mouse(ARROW, NULL, NULL, false);
	} /* if (mb) */
}
static void
clear_all_selected(struct xa_lbox_info *lbox, short skip, RECT *r)
{
	int i;
	short *objs;
	OBJECT *obtree = lbox->wt->tree;
	struct lbox_item *item;
	short obj, last_state;

	objs = lbox->objs;
	item = lbox->items;

	DIAGS((" Clear_all_selected: lbox=%lx, item=%lx, objs=%lx, obtree=%lx",
		(unsigned long)lbox, (unsigned long)item, (unsigned long)objs, (unsigned long)obtree));

	i = -lbox->aslide.first_visible;

	while (item)
	{
		DIAGS((" --- clear selected on idx=%d (first=%d), item=%lx",
			i, lbox->aslide.first_visible, (unsigned long)item));

		if (i >= 0 && i < lbox->aslide.num_visible)
		{
			obj = *objs++;

			DIAGS((" --- inside visible range, idx=%d, obj=%d, item=%lx",
				i, obj, (unsigned long)item));

			if (obj != skip && (obtree[obj].ob_state & OS_SELECTED))
			{
				last_state = obtree[obj].ob_state;
				obtree[obj].ob_state &= ~OS_SELECTED;
				item->selected = 0;
				callout_select(lbox,
						obtree,
						item,
						lbox->user_data,
						obj,
						last_state);
				if (r)
				{
					redraw_lbox(lbox, obj, 2, r);
				}
			}
		}
		else
		{
			DIAGS((" --- out of visible range idx=%d, item=%lx",
				i, (unsigned long)item));

			item->selected = 0;
		}

		i++;
		item = item->next;
	}

	DIAGS(("done clear_all_selected"));
}

static struct lbox_item *
obj_to_item(struct xa_lbox_info *lbox, short obj)
{
	short i;
	short *objs = lbox->objs;
	struct lbox_item *ret = NULL, *item = get_first_visible_item(lbox);

	for (i = 0; i < lbox->aslide.num_visible && item; i++)
	{
		if (*objs++ == obj)
		{
			ret = item;
			break;
		}
		item = item->next;
	}
	DIAGS(("obj_to_item: return %lx", (unsigned long)ret));
	return ret;
}

static short
item_to_obj(struct xa_lbox_info *lbox, struct lbox_item *item)
{
	struct lbox_item *s = get_first_visible_item(lbox);
	short i, obj = -1;

	for (i = 0; i < lbox->aslide.num_visible && s; i++, s = s->next)
	{
		if (s == item)
		{
			obj = lbox->objs[i];
			break;
		}
	}
	return obj;
}

static bool
move_page(struct xa_lbox_info *lbox, struct lbox_slide *s, bool upd, RECT *lbox_r, RECT *slide_r)
{
	bool ret;
	short x, y, mx, my, dir;
	struct xa_aes_object parent, child;
	XA_TREE *wt = lbox->wt;

	check_mouse(wt->owner, NULL, &mx, &my);

	parent = aesobj(wt->tree, s->bkg);
	child  = aesobj(wt->tree, s->sld);

	obj_offset(wt, child, &x, &y);

	if (s->flags & LBOX_VERT)
		dir = my > y ? 1 : 0;
	else
		dir = mx > x ? 1 : 0;

	if (dir)
		ret = s->ul_scroll(lbox, s, s->num_visible - 1);
	else
		ret = s->dr_scroll(lbox, s, s->num_visible - 1);

	if (ret)
	{
		if (upd)
			set_slide_pos(lbox, s);
		if (lbox_r)
			redraw_lbox(lbox, lbox->parent, 2, lbox_r);
		if (slide_r && s->bkg >= 0)
			redraw_lbox(lbox, aesobj_item(&parent), 2, slide_r);
	}
	return ret;
}

static bool
move_slider(struct xa_lbox_info *lbox, struct lbox_slide *s, short num, bool upd, RECT *lbox_r, RECT *slide_r)
{
	bool ret = false;

	if (num && s->num_visible)
	{
		if (num < 0)
		{
			num = -num;
			ret = s->dr_scroll(lbox, s, num);
		}
		else if (num > 0)
		{
			ret = s->ul_scroll(lbox, s, num);
		}
		if (ret)
		{
			if (upd)
				set_slide_pos(lbox, s);

			if (lbox_r)
				redraw_lbox(lbox, lbox->parent, 2, lbox_r);
			if (slide_r && s->bkg >= 0)
				redraw_lbox(lbox, s->bkg, 2, slide_r);
		}
	}
	return ret;
}


unsigned long
XA_lbox_create(int lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree;

	CONTROL(4,0,8)

	DIAG((D_lbox, client, "XA_lbox_create"));

	pb->intout[0] = 0;

	obtree = (OBJECT *)pb->addrin[0];

	if (validate_obtree(client, obtree, "XA_lbox_create:"))
	{
		XA_TREE *wt;
		struct xa_lbox_info *lbox;

		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		assert(wt);

		lbox = new_lbox(wt, client->vdi_settings);

		if (lbox)
		{
			short *in, *dst;
			short i;

			lbox->wdlg_handle = (void *)pb->addrin[7];	/* dialog structure (handle to us) */
			lbox->lbox_handle = (void *)((ulong)lbox << 16 | (ulong)lbox >> 16);

			/*
			 * Get objects and data for slider A
			 */
			in  = (short *)pb->addrin[4];
			dst = (short *)&lbox->aslide;
			lbox->parent = *in++;
			for (i = 0; i < 4; i++)
				*dst++ = *in++;

			lbox->aslide.num_visible	= pb->intin[0];
			lbox->aslide.first_visible	= pb->intin[1];
			lbox->flags			= pb->intin[2];
			lbox->aslide.pause		= pb->intin[3];
			lbox->aslide.flags		= lbox->flags & 1;
			/*
			 * Ozk: Get objects and data for slider B
			 * If only 4 entries in intin, there is no slider B,
			 * and we fill bslid with -1.
			 *
			 * Some apps have 8 entries in intin, yet does
			 * not use slider B. So, we need to check if
			 * we get a number of visible B slider objects too.
			 */
			dst = (short *)&lbox->bslide;
			if (pb->control[1] == 8 && pb->intin[4] && (lbox->flags & LBOX_2SLDRS))
			{
				for (i = 0; i < 4; i++)
					*dst++ = *in++;

				lbox->bslide.num_visible	= pb->intin[4];
				lbox->bslide.first_visible	= pb->intin[5];
				lbox->bslide.entries		= pb->intin[6];
				lbox->bslide.pause		= pb->intin[7];
				lbox->bslide.flags		= ~(lbox->flags & 1);
			}
			else
			{
				for (i = 0; i < 4; i++)
					*dst++ = -1;
			}

			/*
			 * The scrolling functions..
			 */
			lbox->aslide.dr_scroll		= scroll_down;
			lbox->aslide.ul_scroll		= scroll_up;
			lbox->bslide.dr_scroll		= scroll_right;
			lbox->bslide.ul_scroll		= scroll_left;

			/*
			 * lbox callback functions...
			 */
			lbox->slct	= pb->addrin[1];
			lbox->set	= pb->addrin[2];

			/*
			 * Address of initial items and the list of object indexes
			 * making up the visible entries in the lbox.
			 */
			lbox->items	= (struct lbox_item *)	pb->addrin[3];
			lbox->objs	= (short *)		pb->addrin[5];
			lbox->user_data	= (void *)		pb->addrin[6];

			DIAG((D_lbox, client, "N_INTIN=%d, flags=%x, visA=%d, firstA=%d, pauseA=%d, visB=%d, firstB=%d, entriesB=%d, pauseB=%d",
				pb->control[1], lbox->flags, lbox->aslide.num_visible, lbox->aslide.first_visible, lbox->aslide.pause,
				lbox->bslide.num_visible, lbox->bslide.first_visible, lbox->bslide.entries, lbox->bslide.pause));
			DIAG((D_lbox, client, "entries=%d, slct=%lx, set=%lx, items=%lx, objs=%lx",
				lbox->aslide.entries, (long)lbox->slct, (long)lbox->set, (long)lbox->items, (long)lbox->objs));
			DIAG((D_lbox, client, "ctrl_obj: parent=%d, up=%d, dn=%d, bkg=%d, sld=%d B: lf=%d, rt=%d, bkg=%d, sld=%d",
				lbox->parent, lbox->aslide.ul, lbox->aslide.dr, lbox->aslide.bkg, lbox->aslide.sld,
				lbox->bslide.ul, lbox->bslide.dr, lbox->bslide.bkg, lbox->bslide.sld));

			/*
			 * This bit is needed in cases where the sliders background is of
			 * no 3-d type and thus no border offsets while the child is a 3-d
			 * object. We need the difference between parent and child border offset
			 */
			{
				short child, parnt;

				parnt = lbox->aslide.bkg;
				child = lbox->aslide.sld;
				if (child >= 0 && parnt >= 0)
					obj_border_diff(wt, aesobj(obtree, child), aesobj(obtree, parnt), &lbox->aslide.ofs);

				parnt = lbox->bslide.bkg;
				child = lbox->bslide.sld;
				if (parnt >= 0 && child >= 0)
					obj_border_diff(wt, aesobj(obtree, child), aesobj(obtree, parnt), &lbox->bslide.ofs);
			}

			setup_lbox_objects(lbox);
			set_slide_size(lbox, &lbox->aslide);
			set_slide_pos(lbox, &lbox->aslide);
			set_slide_size(lbox, &lbox->bslide);
			set_slide_pos(lbox, &lbox->bslide);

			pb->addrout[0] = (long)lbox->lbox_handle;
			pb->intout[0] = 1;
		}
	}
	return XAC_DONE;
}

unsigned long
XA_lbox_update(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_lbox_info *lbox;
	CONTROL(0,0,2)

	DIAG((D_lbox, client, "XA_lbox_update"));

	lbox = get_lbox(client, (void *)pb->addrin[0]);

	if (lbox)
	{
		RECT *r = (RECT *)pb->addrin[1];

		setup_lbox_objects(lbox);
		set_slide_size(lbox, &lbox->aslide);
		set_slide_pos(lbox, &lbox->aslide);
		set_slide_size(lbox, &lbox->bslide);
		set_slide_pos(lbox, &lbox->bslide);

		if (r)
		{
			hidem();
			redraw_lbox(lbox, lbox->parent, 10, r);
			showm();
		}
	}

	pb->intout[0] = 0;
	return XAC_DONE;
}

static void
click_lbox_obj(struct xa_lbox_info *lbox, struct lbox_item *item, short obj, short dc, RECT *r)
{
	short ks;
	short last_state;
	OBJECT *obtree = lbox->wt->tree;


	last_state = obtree[obj].ob_state;
	vq_key_s(C.P_handle, &ks);

	if (   lbox->flags & LBOX_SNGL  ||
	     ((lbox->flags & LBOX_SHFT) && !(ks & (K_RSHIFT | K_LSHIFT))) )
	{
		clear_all_selected(lbox, obj, r);
	}


	if (lbox->flags & LBOX_TOGGLE)
		obtree[obj].ob_state ^= OS_SELECTED;
	else
		obtree[obj].ob_state |= OS_SELECTED;

	item->selected = obtree[obj].ob_state & OS_SELECTED ? 1 : 0;

	callout_select(lbox,
			obtree,
			item,
			lbox->user_data,
			obj | dc,
			last_state);
	if (r)
		redraw_lbox(lbox, obj, 2, r);
}

unsigned long
XA_lbox_do(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_lbox_info *lbox;
	CONTROL(1,1,1)

	DIAG((D_lbox, client, "XA_lbox_do, lbox_hand=%lx, obj=%d", pb->addrin[0], pb->intin[0]));

	pb->intout[0] = pb->intin[0] & ~0x8000;

	lbox = get_lbox(client, (void *)pb->addrin[0]);

	if (lbox)
	{
		struct widget_tree *wt = lbox->wt;
		struct lbox_item *item;
		short obj = pb->intin[0];
		short dc = obj & 0x8000;
		short ks, mb;
		RECT r, asr, bsr;

		vq_key_s(C.P_handle, &ks);
		obj &= ~0x8000;
		obj_area(lbox->wt, aesobj(lbox->wt->tree, lbox->parent), &r);

		if (lbox->aslide.bkg > 0)
			obj_area(wt, aesobj(wt->tree, lbox->aslide.bkg), &asr);
		if (lbox->bslide.bkg > 0)
			obj_area(wt, aesobj(wt->tree, lbox->bslide.bkg), &bsr);

		if (obj == lbox->aslide.dr)
		{
			DIAG((D_lbox, client, "XA_lbox_do: scroll up (arrow down)"));
			do
			{
				hidem();
				move_slider(lbox, &lbox->aslide, 1, true, &r, &asr);
				showm();
				check_mouse(client, &mb, NULL, NULL);
				if (mb && lbox->aslide.pause)
				{
					f_select((long)(lbox->aslide.pause), NULL, 0, 0);
					/* nap((long)(lbox->aslide.pause)); */
					check_mouse(client, &mb, NULL, NULL);
				}
			} while (mb);
		}
		else if (obj == lbox->aslide.ul)
		{
			do
			{
				hidem();
				move_slider(lbox, &lbox->aslide, -1, true, &r, &asr);
				showm();
				check_mouse(client, &mb, NULL, NULL);
				if (mb && lbox->aslide.pause)
				{
					f_select((long)(lbox->aslide.pause), NULL, 0, 0);
					/* nap((long)(lbox->aslide.pause)); */
					check_mouse(client, &mb, NULL, NULL);
				}
			} while (mb);
		}
		else if (obj == lbox->aslide.sld)
			drag_slide(lbox, &lbox->aslide);
		else if (obj == lbox->aslide.bkg)
		{
			DIAG((D_lbox, client, "XA_lbox_do: page.."));
			do
			{
				hidem();
				move_page(lbox, &lbox->aslide, true, &r, &asr);
				showm();
				check_mouse(client, &mb, NULL, NULL);
				if (mb && lbox->aslide.pause)
				{
					f_select((long)(lbox->aslide.pause), NULL, 0, 0);
					/* nap((long)(lbox->aslide.pause)); */
					check_mouse(client, &mb, NULL, NULL);
				}
			} while (mb);
		}
		else if (obj == lbox->bslide.dr)
		{
			do
			{
				hidem();
				move_slider(lbox, &lbox->bslide, 1, true, &r, &bsr);
				showm();
				check_mouse(client, &mb, NULL, NULL);
				if (mb && lbox->bslide.pause)
				{
					_f_select((long)(lbox->bslide.pause), NULL, 0, 0);
					/* nap((long)(lbox->aslide.pause)); */
					check_mouse(client, &mb, NULL, NULL);
				}
			} while (mb);
		}
		else if (obj == lbox->bslide.ul)
		{
			do
			{
				hidem();
				move_slider(lbox, &lbox->bslide, -1, true, &r, &bsr);
				showm();
				check_mouse(client, &mb, NULL, NULL);
				if (mb && lbox->bslide.pause)
				{
					_f_select((long)(lbox->bslide.pause), NULL, 0, 0);
					/* nap((long)(lbox->aslide.pause)); */
					check_mouse(client, &mb, NULL, NULL);
				}
			} while (mb);
		}
		else if (obj == lbox->bslide.sld)
			drag_slide(lbox, &lbox->bslide);
		else if (obj == lbox->bslide.bkg)
		{
			do
			{
				hidem();
				move_page(lbox, &lbox->bslide, true, &r, &bsr);
				showm();
				check_mouse(client, &mb, NULL, NULL);
				if (mb && lbox->bslide.pause)
				{
					_f_select((long)(lbox->bslide.pause), NULL, 0, 0);
					/* nap((long)(lbox->aslide.pause)); */
					check_mouse(client, &mb, NULL, NULL);
				}
			} while (mb);
		}
		else if ((item = obj_to_item(lbox, obj)))
		{
			struct lbox_item *nitem;
			short nobj, x2, y2, nx, ny;
			struct lbox_slide *v, *h;
			RECT *vsr, *hsr, or;

			if (lbox->flags & LBOX_VERT)
			{
				v = &lbox->aslide;
				vsr = &asr;
				h = &lbox->bslide;
				hsr = &bsr;
			}
			else
			{
				v = &lbox->bslide;
				vsr = &bsr;
				h = &lbox->aslide;
				hsr = &asr;
			}

			obj_area(lbox->wt, aesobj(lbox->wt->tree, obj), &or);

			check_mouse(client, &mb, &nx, &ny);

			x2 = r.x + r.w;
			y2 = r.y + r.h;

			do
			{
				hidem();
				click_lbox_obj(lbox, item, obj, dc, &r);
				nobj = -1;
				showm();
				S.wm_count++;
				while (!dc && mb && nobj == -1)
				{
					wait_mouse(client, &mb, &nx, &ny);

					if (!mb)
						break;

					if (m_inside(nx, ny, &r))
					{
						struct xa_aes_object nobject;
						nobject = obj_find(wt, aesobj(wt->tree, lbox->parent), 1, nx, ny, NULL);
						nobj = aesobj_item(&nobject);
						if (nobj == obj)
							nobj = -1;

						if (nobj != -1)
						{
							nitem = obj_to_item(lbox, nobj);
							if (nitem)
							{
								obj = nobj;
								item = nitem;
							}
							else
								nobj = -1;
						}
					}
					else if (lbox->flags & LBOX_AUTO)
					{
						if (ny > y2)
						{
							while (mb && ny > y2)
							{
								if (move_slider(lbox, v, 1, true, NULL, NULL) &&
								    (lbox->flags & LBOX_AUTOSLCT))
								{
									short o;
									nitem = get_last_visible_item(lbox);
									o = item_to_obj(lbox, nitem);
									if (o >= 0)
										click_lbox_obj(lbox, nitem, o, dc, NULL);
									hidem();
									redraw_lbox(lbox, lbox->parent, 2, &r);
									redraw_lbox(lbox, v->bkg, 2, vsr);
									showm();
									if (v->pause)
										_f_select(v->pause, NULL, 0, 0);
								}
								check_mouse(client, &mb, &nx, &ny);
							}
						}
						else if (ny < r.y)
						{
							while (mb && ny < r.y)
							{
								if (move_slider(lbox, v, -1, true, NULL, NULL) &&
								    (lbox->flags & LBOX_AUTOSLCT))
								{
									short o;
									nitem = get_first_visible_item(lbox);
										o = item_to_obj(lbox, nitem);
									if (o >= 0)
										click_lbox_obj(lbox, nitem, o, dc, NULL);
									hidem();
									redraw_lbox(lbox, lbox->parent, 2, &r);
									redraw_lbox(lbox, v->bkg, 2, vsr);
									showm();
									if (v->pause)
										_f_select(v->pause, NULL, 0, 0);
								}
								check_mouse(client, &mb, &nx, &ny);
							}
						}
						else if (nx > x2)
						{
							while (mb && nx > x2)
							{
								if (move_slider(lbox, h, 1, true, NULL, NULL))
								{
									hidem();
									redraw_lbox(lbox, lbox->parent, 2, &r);
									redraw_lbox(lbox, h->bkg, 2, hsr);
									showm();
									if (h->pause)
										_f_select(h->pause, NULL, 0, 0);
								}
								check_mouse(client, &mb, &nx, &ny);
							}
						}
						else if (nx < r.x)
						{
							while (mb && nx < r.x)
							{
								if (move_slider(lbox, h, -1, true, NULL, NULL))
								{
									hidem();
									redraw_lbox(lbox, lbox->parent, 2, &r);
									redraw_lbox(lbox, h->bkg, 2, hsr);
									showm();
									if (h->pause)
										_f_select(h->pause, NULL, 0, 0);
								}
								check_mouse(client, &mb, &nx, &ny);
							}
						}
					}
				}
				S.wm_count--;
			} while (!dc && mb);

			if (dc)
				pb->intout[0] = -1;
			else
				pb->intout[0] = obj;

			DIAG((D_lbox, client, "XA_lbox_do: lbox=%lx, item=%lx, obj=%d",
				(unsigned long)lbox, (unsigned long)item, obj));
		}
	}
	DIAG((D_lbox, client, "XA_lbox_do: return %d", pb->intout[0]));
	return XAC_DONE;
}

unsigned long
XA_lbox_delete(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_lbox_info *lbox;
	CONTROL(0,1,1)

	DIAG((D_lbox, client, "XA_lbox_delete"));

	pb->intout[0] = 1;

	lbox = get_lbox(client, (void *)pb->addrin[0]);
	if (lbox)
	{
		delete_lbox(lbox);
	}
	return XAC_DONE;
}

static void
get_selected(struct xa_lbox_info *lbox, short start, short *ret_obj, short *ret_entry, struct lbox_item **ret_item)
{
	OBJECT *obtree = lbox->wt->tree;
	struct lbox_item *item;
	short *objs;
	short i, o, obj = -1;

	item = get_first_visible_item(lbox);
	objs = lbox->objs;

	DIAG((D_lbox, NULL, "get_selected: entries=%d, start=%d, obtree=%lx, item=%lx, objs=%lx",
		lbox->aslide.entries, start, (unsigned long)obtree, (unsigned long)item, (unsigned long)objs));

	for (i = 0; i < lbox->aslide.entries && item; i++)
	{
		if (i >= start)
		{
			o = objs[i];
			DIAGS((" -- -- item=%lx, item->next=%lx, i=%d, o=%d", (unsigned long)item, (unsigned long)item->next, i, o));
			if (o <= 0)
				break;
			else if (o && obtree[o].ob_state & OS_SELECTED)
			{
				obj = o;
				break;
			}
		}
		item = item->next;
	}

	if (obj < 0)
	{
		item = NULL;
		i = -1;
	}

	DIAG((D_lbox, NULL, "get_selected: return obj=%d, entry=%d, item=%lx",
		obj, i, (unsigned long)item));

	if (ret_obj)
		*ret_obj = obj;
	if (ret_entry)
		*ret_entry = i;
	if (ret_item)
		*ret_item = item;
}

unsigned long
XA_lbox_get(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_lbox_info *lbox;

	CONTROL(1,0,1)

	DIAG((D_lbox, client, "XA_lbox_get %d", pb->intin[0]));

	if (pb->intin[0] == 9)
	{
		struct lbox_item *item, *find;
		short index = -1, i = 0;

		item = (struct lbox_item *)pb->addrin[0];
		find = (struct lbox_item *)pb->addrin[1];
		while (item)
		{
			if (item == find)
			{
				index = i;
				break;
			}
			i++;
			item = item->next;
		}
		pb->intout[0] = index;

		DIAG((D_lbox, client, " --- get item index of %lx, start=%lx - found %d",
			pb->addrin[1], pb->addrin[0], pb->intout[0]));
	}
	else if ((lbox = get_lbox(client, (void *)pb->addrin[0])))
	{
		switch (pb->intin[0])
		{
			case 0:	/* Count items					*/
			{
				DIAG((D_lbox, client, " --- count items %d",
					lbox->aslide.entries));
				pb->intout[0] = lbox->aslide.entries;
				break;
			}
			case 1:	/* Get obtree					*/
			{
				pb->addrout[0] = (long)lbox->wt->tree;

				DIAG((D_lbox, client, " --- get obtree = %lx",
					pb->addrout[0]));
				break;
			}
			case 2:	/* Get number of visible items, slider A	*/
			{
				pb->intout[0] = lbox->aslide.num_visible;

				DIAG((D_lbox, client, " --- get visible items A %d",
					pb->intout[0]));
				break;
			}
			case 3:	/* Get user data				*/
			{
				pb->addrout[0] = (long)lbox->user_data;

				DIAG((D_lbox, client, " --- get user data %lx",
					pb->addrout[0]));
				break;
			}
			case 4: /* get first visible item, slider A		*/
			{
				pb->intout[0] = lbox->aslide.first_visible;

				DIAG((D_lbox, client, " --- get first visible item A %d",
					pb->intout[0]));
				break;
			}
			case 5:	/* Get index of selected item			*/
			{
				get_selected(lbox, 0, NULL, (short *)&pb->intout[0], NULL);

				DIAG((D_lbox, client, " --- get index selected item %d",
					pb->intout[0]));
				break;
			}
			case 6: /* Get items					*/
			{
				pb->addrout[0] = (long)lbox->items;

				DIAG((D_lbox, client, " --- get items (start of list) %lx",
					pb->addrout[0]));
				break;
			}
			case 7: /* Get item					*/
			{
				struct lbox_item *item = lbox->items;
				short i = pb->intin[0] - 1;

				if (i < 0)
					i = 0;

				while (item && i)
				{
					item = item->next;
					i--;
				}
				pb->addrout[0] = (long)item;

				DIAG((D_lbox, client, " --- get item at index %d - %lx",
					pb->intin[0], pb->addrout[0]));
				break;
			}
			case 8:	/* Get selected item				*/
			{
				struct lbox_item *item;

				get_selected(lbox, 0, NULL, NULL, &item);
				pb->addrout[0] = (long)item;

				DIAG((D_lbox, client, " --- get selected item %lx",
					pb->addrout[0]));
				break;
			}
			case 10: /* Get number of visible items, slider B	*/
			{
				pb->intout[0] = lbox->bslide.num_visible;

				DIAG((D_lbox, client, " --- visible items B %d",
					pb->intout[0]));
				break;
			}
			case 11: /* Get number of items, slider B		*/
			{
				pb->intout[0] = lbox->bslide.entries;

				DIAG((D_lbox, client, " --- entries slider B %d",
					pb->intout[0]));
				break;
			}
			case 12: /* get first visible item, slider B		*/
			{
				pb->intout[0] = lbox->bslide.first_visible;

				DIAG((D_lbox, client, " --- first item B %d",
					pb->intout[0]));
				break;
			}
		}
	}
	return XAC_DONE;
}

unsigned long
XA_lbox_set(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_lbox_info *lbox;
	CONTROL(2,0,2)

	DIAG((D_lbox, client, "XA_lbox_set %d", pb->intin[0]));

	lbox = get_lbox(client, (void *)pb->addrin[0]);

	if (lbox)
	{
		switch (pb->intin[0])
		{
			case 0: /* set_slider		*/
			{
				short num = pb->intin[1] - lbox->aslide.first_visible;
				DIAGS(("lbox_set_slider: set to %d, curnt=%d, move=%d",
					pb->intin[1], lbox->aslide.first_visible, num));
				move_slider(lbox, &lbox->aslide, num, false, NULL, (RECT *)pb->addrin[1]);
				break;
			}
			case 1:	/* set items		*/
			{
				DIAG((D_lbox, NULL, " --- set items=%lx", pb->addrin[1]));
				lbox->items = (struct lbox_item *)pb->addrin[1];
				lbox->aslide.first_visible = 0;
				lbox->bslide.first_visible = 0;
				setup_lbox_objects(lbox);
				set_slide_size(lbox, &lbox->aslide);
				set_slide_pos(lbox, &lbox->aslide);
				break;
			}
			case 2:	/* free items		*/
			{
				break;
			}
			case 3:	/* free item list	*/
			{
				break;
			}
			case 4: /* scroll to		*/
			{
				short num = pb->intin[1] - lbox->aslide.first_visible;
				DIAGS(("lbox_scroll_to(4): scroll to %d, curnt=%d, move=%d",
					pb->intin[1], lbox->aslide.first_visible, num));
				move_slider(lbox, &lbox->aslide, num, false, (RECT *)pb->addrin[1], (RECT *)pb->addrin[2]);
				break;
			}
			case 5:	/* set slider B		*/
			{
				short num = pb->intin[1] - lbox->bslide.first_visible;
				DIAGS(("lbox_set_Bslider(5): set to %d, curnt=%d, move=%d",
					pb->intin[1], lbox->bslide.first_visible, num));
				move_slider(lbox, &lbox->bslide, num, false, NULL, (RECT *)pb->addrin[1]);
				break;
			}
			case 6: /* set B entries	*/
			{
				DIAGS(("lbox_set_bentries: set to %d, was=%d",
					pb->intin[1], lbox->bslide.entries));
				lbox->bslide.entries = pb->intin[1];
				set_slide_size(lbox, &lbox->bslide);
				set_slide_pos(lbox, &lbox->bslide);
				break;
			}
			case 7: /* set B scroll to	*/
			{
				short num = pb->intin[1] - lbox->bslide.first_visible;
				DIAGS(("lbox_scroll_tob(7): set to %d, curnt=%d, move=%d",
					pb->intin[1], lbox->bslide.first_visible, num));
				move_slider(lbox, &lbox->bslide, num, false, (RECT *)pb->addrin[1], (RECT *)pb->addrin[2]);
				break;
			}
		}
	}
	return XAC_DONE;
}

#endif /* WDIALOG_LBOX */
