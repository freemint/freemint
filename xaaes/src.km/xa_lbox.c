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
new_lbox(struct widget_tree *wt)
{
	struct xa_lbox_info *lbox;

	lbox = kmalloc(sizeof(*lbox));

	if (lbox)
	{
		bzero(lbox, sizeof(*lbox));
			
		lbox->next = wt->lbox;
		wt->lbox = lbox;
		lbox->wt = wt;
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

	for (i = 0, item = lbox->items; (i < lbox->first_a) && (item = item->next); i++)
		;

	DIAGS(("get_first_visible_item: %lx", item));
	return item;
}

/*
 * Redraw an LBOX object
 */
static void
redraw_lbox(struct xa_lbox_info *lbox, short obj, short depth, RECT *r)
{
	struct xa_window *wind;
	struct widget_tree *wt;
	short i, start;
	RECT or;

	wt = lbox->wt;
	wind = wt->wind;

	DIAG((D_lbox, NULL, "redraw_lbox: lbox=%lx, wt=%lx, wind=%lx, parent=%d",
		lbox, wt, wind, lbox->parent));

	ob_area(wt->tree, obj, &or);

	start = obj;
	while (object_is_transparent(wt->tree + start))
	{
		if ((i = ob_get_parent(wt->tree, start)) < 0)
			break;
		start = i;
		depth++;
	}

	if (wind)
	{
		if (wind->is_open)
		{
			struct xa_rect_list *rl;
			RECT dr;

			if (!r)
				r = &or;

			rl = wind->rect_start;
			while (rl)
			{
				if (xa_rect_clip(&rl->r, r, &dr))
				{
					set_clip(&dr);
					draw_object_tree(0, wt, wt->tree, start, depth, 1);
				}
				rl = rl->next;
			}
		}
	}
	else if (r)
	{
		if (xa_rect_clip(r, &or, &or))
		{
			set_clip(&or);
			draw_object_tree(0, wt, wt->tree, start, depth, 1);
		}
	}
	else
	{
		set_clip(&or);
		draw_object_tree(0, wt, wt->tree, start, depth, 1);
	}
	clear_clip();
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
		wt, wt->lbox, wt->lbox ? wt->lbox->entries : -1));

	x = 0;
	y = 0;

	item = lbox->items;

	DIAGS((" --- parent=%d, flags=%x, %d/%d",
		parent, lbox->flags, x, y));

	prev = parent;

	obtree[parent].ob_head = -1;
	obtree[parent].ob_tail = -1;

	for (i = index = ent = 0, first = lbox->first_a; i < (lbox->visible_a + lbox->first_a); i++, ent++)
	{
		if (item)
		{
			if (!first)
			{
				obj = lbox->objs[index];

				DIAG((D_lbox, NULL, " --- obj=%d, index=%d, item=%lx, nxtitem=%lx",
					obj, index, item, item->next));

				if (prev == parent)
				{
					obtree[parent].ob_head = obj;
					obtree[parent].ob_tail = obj;
					obtree[obj   ].ob_next = parent;
				}
				else
				{
					obtree[prev  ].ob_next = obj;
					obtree[parent].ob_tail = obj;
					obtree[obj   ].ob_next = parent;
				}

				prev = obj;

				obtree[obj].ob_x = x;
				obtree[obj].ob_y = y;
				if (lbox->flags & LBOX_VERT)
					y += obtree[obj].ob_height;
				else
					x += obtree[obj].ob_width;

				lbox->set(lbox->lbox_handle, obtree, item, obj, lbox->user_data, NULL, 0);
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

	lbox->entries = ent;

	DIAGS((" --- Got %d entries", ent));

}

static bool
scroll_up(struct xa_lbox_info *lbox, short num)
{
	if (num && (lbox->visible_a + lbox->first_a) < lbox->entries)
	{
		struct lbox_item *item;
		short i;

		i = lbox->first_a + lbox->visible_a + num;

		if (i > lbox->entries)
			num -= i - lbox->entries;

		if (num > 0)
		{
			short idx = 0;

			item = get_first_visible_item(lbox);

			for (i = 0; i < num && item; i++)
			{
				if (idx++ < lbox->visible_a)
				{
					lbox->slct(lbox->lbox_handle,
						   lbox->wt->tree,
						   item,
						   lbox->user_data,
						   0,
						   0);
				}
				item = item->next;
			}
			lbox->first_a += num;
			setup_lbox_objects(lbox);
			return 1;
		}
	}
	return 0;
}

static bool
scroll_down(struct xa_lbox_info *lbox, short num)
{
	if (num && lbox->first_a)
	{
		struct lbox_item *item, *last;
		short i, j;

		item = get_first_visible_item(lbox);
		last = item;

		if (num > lbox->first_a)
			num = lbox->first_a;

		i = lbox->visible_a - num;
		if (i < 0)
			i = 0;

		for (j = i; j > 0 && last; j--)
			last = last->next;

		for (j = lbox->visible_a - i; j > 0 && last; j--)
		{
			lbox->slct(lbox->lbox_handle,
				   lbox->wt->tree,
				   last,
				   lbox->user_data,
				   0, 0);
			last = last->next;
		}
		lbox->first_a -= num;
		setup_lbox_objects(lbox);
		return 1;
	}
	return 0;
}

static bool
page(struct xa_lbox_info *lbox)
{
	short parent, child, x, y, mx, my, dir;
	XA_TREE *wt = lbox->wt;
	OBJECT *obtree = wt->tree;

	check_mouse(wt->owner, NULL, &mx, &my);

	parent = lbox->aslid.slide_bkg;
	child  = lbox->aslid.slider;

	ob_offset(obtree, child, &x, &y);

	if (lbox->flags & LBOX_VERT)
		dir = my > y ? 1 : 0;
	else
		dir = mx > x ? 1 : 0;

	if (dir)
		return scroll_up(lbox, lbox->visible_a - 1);
	else
		return scroll_down(lbox, lbox->visible_a - 1);
}
	
	
static void
set_aslide_size(struct xa_lbox_info *lbox)
{
	long relsize;
	short w, h, parent, child;
	OBJECT *obtree;

	parent = lbox->aslid.slide_bkg;
	child = lbox->aslid.slider;

	if (parent >= 0 && child >= 0)
	{
		obtree = lbox->wt->tree;

		if (lbox->entries)
			relsize = (long)lbox->visible_a * 1000 / lbox->entries;
		else
			relsize = 1000;

		if (relsize > 1000)
			relsize = 1000;

	
		if (lbox->flags & LBOX_VERT)
		{
			w = obtree[parent].ob_width;
			h = ((relsize * obtree[parent].ob_height) + 500) / 1000;
		}
		else
		{
			w = ((relsize * obtree[parent].ob_width) + 500) / 1000;
			h = obtree[parent].ob_height;
		}
		DIAG((D_lbox, NULL, "set_aslide_size: parent=%d, child=%d, entries=%d, visible=%d, relsiz=%ld",
			parent, child, lbox->entries, lbox->visible_a, relsize));
		DIAG((D_lbox, NULL, "old w=%d, old h=%d, new w=%d, new h=%d",
			obtree[child].ob_width, obtree[child].ob_height, w, h));

		obtree[child].ob_width = w + lbox->aslid.ofs.w;
		obtree[child].ob_height = h + lbox->aslid.ofs.h;
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
set_aslide_pos(struct xa_lbox_info *lbox)
{
	short parent, child, total;
	long relpos = 0;
	OBJECT *obtree = lbox->wt->tree;

	parent = lbox->aslid.slide_bkg;
	child = lbox->aslid.slider;
	if (parent >= 0 && child >= 0)
	{
		short l;
		if ((total = lbox->entries - lbox->visible_a) > 0)
		{
			relpos = (long)(((long)lbox->first_a * 1000) / total);
			if (relpos > 1000)
				relpos = 1000;
		}

		if (lbox->flags & LBOX_VERT)
		{
			l = obtree[child].ob_height - lbox->aslid.ofs.h;
			obtree[child].ob_y = ((((long)(obtree[parent].ob_height - l) * relpos) + 500) / 1000) - lbox->aslid.ofs.y;
		}
		else
		{
			l = obtree[child].ob_width - lbox->aslid.ofs.w;
			obtree[child].ob_x = ((((long)(obtree[parent].ob_width -  l) * relpos) + 500) / 1000) - lbox->aslid.ofs.x;
		}
	}
}

static void
drag_aslide(struct xa_lbox_info *lbox)
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

		parent = lbox->aslid.slide_bkg;
		child = lbox->aslid.slider;

		/*
		 * new current object = total_object - visible_objects * slider_relpos / 1000
		 */
		ob_area(obtree, parent, &sl_r);
		ob_area(obtree, lbox->parent, &lb_r);

		first = lbox->first_a;

		if (lbox->flags & LBOX_VERT)
		{
			graf_mouse(XACRS_VERTSIZER, NULL);
			max = obtree[parent].ob_height - (obtree[child].ob_height - lbox->aslid.ofs.h);
		}
		else
		{
			graf_mouse(XACRS_HORSIZER, NULL);
			max = obtree[parent].ob_width - (obtree[child].ob_width - lbox->aslid.ofs.w);
		}

		while (mb)
		{
			check_mouse(lbox->wt->owner, &mb, &mx, &my);

			if (mb && (sx != mx || sy != mx))
			{
				short nc, oc;
				
				if (lbox->flags & LBOX_VERT)
				{
					oc = obtree[child].ob_y + lbox->aslid.ofs.y;
					nc = oc + (my - sy);
					if (nc < 0)
						nc = 0;
					else if (nc > max)
						nc = max;

					if (oc != nc)
					{
						slider_rpos = get_slider_relpos(obtree, nc, parent, child, 1, &lbox->aslid.ofs);
						first = lbox->entries - lbox->visible_a;
						if (first > 0)
							first = ((long)first * slider_rpos + 500) / 1000;
						else
							first = 0;

						if (first != lbox->first_a)
						{
							nc = first - lbox->first_a;
							if (nc < 0)
								scroll_down(lbox, -nc);
							else
								scroll_up(lbox, nc);

							set_aslide_pos(lbox);
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
					oc = obtree[child].ob_x + lbox->aslid.ofs.x;
					nc = oc + (mx - sx);
					if (nc < 0)
						nc = 0;
					else if (nc > max)
						nc = max;

					if (oc != nc)
					{
						slider_rpos = get_slider_relpos(obtree, nc, parent, child, 0, &lbox->aslid.ofs);
						first = lbox->entries - lbox->visible_a;
						if (first > 0)
							first = ((long)first * slider_rpos + 500) / 1000;
						else
							first = 0;

						if (first != lbox->first_a)
						{
							nc = first - lbox->first_a;
							if (nc < 0)
								scroll_down(lbox, -nc);
							else
								scroll_up(lbox, nc);

							set_aslide_pos(lbox);
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
		graf_mouse(ARROW, NULL);
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
		lbox, item, objs, obtree));

	i = -lbox->first_a;

	while (item)
	{
		DIAGS((" --- clear selected on idx=%d (first=%d), item=%lx",
			i, lbox->first_a, item));

		if (i >= 0 && i < lbox->visible_a)
		{
			obj = *objs++;

			DIAGS((" --- inside visible range, idx=%d, obj=%d, item=%lx",
				i, obj, item));

			if (obj != skip && (obtree[obj].ob_state & OS_SELECTED))
			{
				last_state = obtree[obj].ob_state;
				obtree[obj].ob_state &= ~OS_SELECTED;
				item->selected = 0;
				lbox->slct(lbox->lbox_handle,
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
				i, item));
		
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

	for (i = 0; i < lbox->visible_a && item; i++)
	{
		if (*objs++ == obj)
		{
			ret = item;
			break;
		}
		item = item->next;
	}
	DIAGS(("obj_to_item: return %lx", ret));
	return ret;
}

unsigned long
XA_lbox_create(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree;
	
	CONTROL(4,0,8)

	DIAG((D_lbox, client, "XA_lbox_create"));

	pb->intout[0] = 0;

	obtree = (OBJECT *)pb->addrin[0];

	if (obtree)
	{
		XA_TREE *wt;
		struct xa_lbox_info *lbox;

		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		assert(wt);

		lbox = new_lbox(wt);
		
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
			dst = (short *)&lbox->aslid;
			lbox->parent = *in++;
			for (i = 0; i < 4; i++)
				*dst++ = *in++;

			lbox->visible_a	= pb->intin[0];
			lbox->first_a	= pb->intin[1];
			lbox->flags	= pb->intin[2];
			lbox->pause_a	= pb->intin[3];

			/*
			 * Get objects and data for slider B
			 * If only 4 entries in intin, there is no slider B,
			 * and we fill bslid with -1
			 */
			dst = (short *)&lbox->bslid;
			if (pb->control[1] == 8)
			{
				for (i = 0; i < 4; i++)
					*dst++ = *in++;

				lbox->visible_b	= pb->intin[4];
				lbox->first_b	= pb->intin[5];
				lbox->entries_b	= pb->intin[6];
				lbox->pause_b	= pb->intin[7];
			}
			else
			{
				for (i = 0; i < 4; i++)
					*dst++ = -1;
			}

			/*
			 * lbox callback functions...
			 */
			lbox->slct	= (lbox_select *)	pb->addrin[1];
			lbox->set	= (lbox_set *)		pb->addrin[2];

			/*
			 * Address of initial items and the list of object indexes
			 * making up the visible entries in the lbox.
			 */
			lbox->items	= (struct lbox_item *)	pb->addrin[3];
			lbox->objs	= (short *)		pb->addrin[5];
			lbox->user_data	= (void *)		pb->addrin[6];

			DIAG((D_lbox, client, "N_INTIN=%d, flags=%x, visA=%d, firstA=%d, pauseA=%d, visB=%d, firstB=%d, entriesB=%d, pauseB=%d",
				pb->control[1], lbox->flags, lbox->visible_a, lbox->first_a, lbox->pause_a,
				lbox->visible_b, lbox->first_b, lbox->entries_b, lbox->pause_b));
			DIAG((D_lbox, client, "entries=%d, slct=%lx, set=%lx, items=%lx, objs=%lx",
				lbox->entries, (long)lbox->slct, (long)lbox->set, (long)lbox->items, (long)lbox->objs));
			DIAG((D_lbox, client, "ctrl_obj: parent=%d, up=%d, dn=%d, bkg=%d, sld=%d B: lf=%d, rt=%d, bkg=%d, sld=%d",
				lbox->parent, lbox->aslid.up, lbox->aslid.down, lbox->aslid.slide_bkg, lbox->aslid.slider,
				lbox->bslid.left, lbox->bslid.right, lbox->bslid.slide_bkg, lbox->bslid.slider));

			/*
			 * This bit is needed in cases where the sliders background is of
			 * no 3-d type and thus no border offsets while the child is a 3-d
			 * object. We need the difference between parent and child border offset
			 */
			{
				short child, parnt;

				parnt = lbox->aslid.slide_bkg;
				child = lbox->aslid.slider;
				if (child >= 0 && parnt >= 0)
					ob_border_diff(obtree, child, parnt, &lbox->aslid.ofs);

				parnt = lbox->bslid.slide_bkg;
				child = lbox->bslid.slider;
				if (parnt >= 0 && child >= 0)
					ob_border_diff(obtree, child, parnt, &lbox->bslid.ofs);
			}

			setup_lbox_objects(lbox);
			set_aslide_size(lbox);
			set_aslide_pos(lbox);

			pb->addrout[0] = (long)lbox->lbox_handle;
			pb->intout[0] = 1;
		}
	}
	return XAC_DONE;
}

unsigned long
XA_lbox_update(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_lbox_info *lbox;
	CONTROL(0,0,2)

	DIAG((D_lbox, client, "XA_lbox_update"));

	lbox = get_lbox(client, (void *)pb->addrin[0]);

	if (lbox)
	{
		RECT *r = (RECT *)pb->addrin[1];

		setup_lbox_objects(lbox);
		set_aslide_size(lbox);
		set_aslide_pos(lbox);

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

unsigned long
XA_lbox_do(enum locks lock, struct xa_client *client, AESPB *pb)
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
		OBJECT *obtree = wt->tree;
		short obj = pb->intin[0];
		short dc = obj & 0x8000 ? 1 : 0;
		short last_state, ks;
		RECT r;
		
		vq_key_s(C.vh, &ks);
		obj &= ~0x8000;
		ob_area(obtree, lbox->parent, &r);

		if (obj == lbox->aslid.down)
		{
			DIAG((D_lbox, client, "XA_lbox_do: scroll up (arrow down)"));
			if (scroll_up(lbox, 1))
			{
				hidem();
				set_aslide_pos(lbox);
				redraw_lbox(lbox, lbox->parent, 2, &r);
				ob_area(obtree, lbox->aslid.slide_bkg, &r);
				redraw_lbox(lbox, lbox->aslid.slide_bkg, 2, &r);
				showm();
			}
		}
		else if (obj == lbox->aslid.up)
		{
			DIAG((D_lbox, client, "XA_lbox_do: scroll down (arrow up)"));
			if (scroll_down(lbox, 1))
			{
				hidem();
				set_aslide_pos(lbox);
				redraw_lbox(lbox, lbox->parent, 2, &r);
				ob_area(obtree, lbox->aslid.slide_bkg, &r);
				redraw_lbox(lbox, lbox->aslid.slide_bkg, 2, &r);
				showm();
			}
		}
		else if (obj == lbox->aslid.slider)
			drag_aslide(lbox);
		else if (obj == lbox->aslid.slide_bkg)
		{
			DIAG((D_lbox, client, "XA_lbox_do: page.."));
			if (page(lbox))
			{
				hidem();
				set_aslide_pos(lbox);
				redraw_lbox(lbox, lbox->parent, 2, &r);
				ob_area(obtree, lbox->aslid.slide_bkg, &r);
				redraw_lbox(lbox, lbox->aslid.slide_bkg, 2, &r);
				showm();
			}
		}
		else if ((item = obj_to_item(lbox, obj)))
		{			
			last_state = obtree[obj].ob_state;
			hidem();

			if (   lbox->flags & LBOX_SNGL  ||
			     ((lbox->flags & LBOX_SHFT) && !(ks & (K_RSHIFT | K_LSHIFT))) )
			{
				clear_all_selected(lbox, obj, &r);
			}
				

			if (lbox->flags & LBOX_TOGGLE)
				obtree[obj].ob_state ^= OS_SELECTED;
			else
				obtree[obj].ob_state |= OS_SELECTED;

			item->selected = obtree[obj].ob_state & OS_SELECTED ? 1 : 0;

			lbox->slct(lbox->lbox_handle,
				   obtree,
				   item,
				   lbox->user_data,
				   pb->intin[0],
				   last_state);

			redraw_lbox(lbox, lbox->parent, 2, &r);
			showm();

			if (dc)
				pb->intout[0] = -1;
			else
				pb->intout[0] = obj;

			DIAG((D_lbox, client, "XA_lbox_do: lbox=%lx, item=%lx, obj=%d, last=%x",
				lbox, item, obj, last_state));
		}
	}
	DIAG((D_lbox, client, "XA_lbox_do: return %d", pb->intout[0]));
	return XAC_DONE;
}

unsigned long
XA_lbox_delete(enum locks lock, struct xa_client *client, AESPB *pb)
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
	
	for (i = 0; i < lbox->entries && item; i++)
	{
		if (i >= start)
		{
			o = objs[i];
			if (obtree[o].ob_state & OS_SELECTED)
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

	if (ret_obj)
		*ret_obj = obj;
	if (ret_entry)
		*ret_entry = i;
	if (ret_item)
		*ret_item = item;
}

unsigned long
XA_lbox_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_lbox_info *lbox;

	CONTROL(1,0,1)

	DIAG((D_lbox, client, "XA_lbox_get"));

	if (pb->intin[0] == 9)
	{
		struct lbox_item *item, *find;
		short index = -1, i = 1;
				
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
					lbox->entries));
				pb->intout[0] = lbox->entries;
				break;
			}
			case 1:	/* Get obtree					*/
			{
				(void *)pb->addrout[0] = lbox->wt->tree;

				DIAG((D_lbox, client, " --- get obtree = %lx",
					pb->addrout[0]));
				break;
			}
			case 2:	/* Get number of visible items, slider A	*/
			{
				pb->intout[0] = lbox->visible_a;

				DIAG((D_lbox, client, " --- get visible items A %d",
					pb->intout[0]));
				break;
			}
			case 3:	/* Get user data				*/
			{
				(void *)pb->addrout[0] = lbox->user_data;

				DIAG((D_lbox, client, " --- get user data %lx",
					pb->addrout[0]));
				break;
			}
			case 4: /* get first visible item, slider A		*/
			{
				pb->intout[0] = lbox->first_a;

				DIAG((D_lbox, client, " --- get first visible item A %d",
					pb->intout[0]));
				break;
			}
			case 5:	/* Get index of selected item			*/
			{
				get_selected(lbox, 0, (short *)&pb->intout[0], NULL, NULL);

				DIAG((D_lbox, client, " --- get index selected item %d",
					pb->intout[0]));
				break;
			}
			case 6: /* Get items					*/
			{
				(void *)pb->addrout[0] = lbox->items;

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
				(void *)pb->addrout[0] = item;

				DIAG((D_lbox, client, " --- get item at index %d - %lx",
					pb->intin[0], pb->addrout[0]));
				break;
			}
			case 8:	/* Get selected item				*/
			{
				struct lbox_item *item;

				get_selected(lbox, 0, NULL, NULL, &item);
				(void *)pb->addrout[0] = item;

				DIAG((D_lbox, client, " --- get selected item %lx",
					pb->addrout[0]));
				break;
			}
			case 10: /* Get number of visible items, slider B	*/
			{
				pb->intout[0] = lbox->visible_b;

				DIAG((D_lbox, client, " --- visible items B %d",
					pb->intout[0]));
				break;
			}
			case 11: /* Get number of items, slider B		*/
			{
				pb->intout[0] = lbox->entries_b;

				DIAG((D_lbox, client, " --- entries slider B %d",
					pb->intout[0]));
				break;
			}
			case 12: /* get first visible item, slider B		*/
			{
				pb->intout[0] = lbox->first_b;

				DIAG((D_lbox, client, " --- first item B %d",
					pb->intout[0]));
				break;
			}
		}
	}
	return XAC_DONE;
}

unsigned long
XA_lbox_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
	//XA_TREE *wt;
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
				short num = pb->intin[0] - lbox->first_a;

				if (num)
				{
					if (num < 0)
					{
						num = -num;
						scroll_down(lbox, num);
					}
					else if (num > 0)
					{
						scroll_up(lbox, num);
					}
					if (pb->addrin[1])
						redraw_lbox(lbox, lbox->aslid.slide_bkg, 2, (RECT *)pb->addrin[1]);
				}
				break;
			}
			case 1:	/* set items		*/
			{
				DIAG((D_lbox, NULL, " --- set items=%lx", pb->addrin[1]));
				lbox->items = (struct lbox_item *)pb->addrin[1];
				setup_lbox_objects(lbox);
				set_aslide_size(lbox);
				set_aslide_pos(lbox);
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
				short num = pb->intin[0] - lbox->first_a;

				if (num)
				{
					if (num < 0)
					{
						num = -num;
						scroll_down(lbox, num);
					}
					else if (num > 0)
					{
						scroll_up(lbox, num);
					}
					if (pb->addrin[1])
						redraw_lbox(lbox, lbox->parent, 2, (RECT *)pb->addrin[1]);
					if (pb->addrin[2])
						redraw_lbox(lbox, lbox->aslid.slide_bkg, 2, (RECT *)pb->addrin[2]);
				}
				break;
			}
			case 5:	/* set slider B		*/
			{
				break;
			}
			case 6: /* set B entries	*/
			{
				break;
			}
			case 7: /* set B scroll to	*/
			{
				break;
			}
		}
	}
	return XAC_DONE;
}

#endif /* WDIALOG_LBOX */
