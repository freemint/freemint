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
#include "xa_global.h"

#include "xa_evnt.h"
#include "xa_graf.h"
#include "k_mouse.h"

#include "scrlobjc.h"
#include "form.h"
#include "rectlist.h"
#include "draw_obj.h"
#include "obtree.h"
#include "widgets.h"
#include "xa_fsel.h"
#include "xa_form.h"
#include "c_window.h"

#if 0
static void
slist_msg_handler(
	enum locks lock,
	struct xa_window *wind,
	struct xa_client *to,
	short mp0, short mp1, short mp2, short mp3,
	short mp4, short mp5, short mp6, short mp7);
#endif
static void
slist_msg_handler(struct xa_window *wind, struct xa_client *to, short amq, short qmf, short *msg);

static void
sliders(struct scroll_info *list)
{
	XA_slider(list->wi, XAW_VSLIDE,
		  list->n - 1,
		  list->s,
		  list->top ? list->top->n - 1 : 0);

	XA_slider(list->wi, XAW_HSLIDE,
		  list->max,
		  list->v,
		  list->left);
}

void
free_scrollist(SCROLL_INFO *list)
{
	while (list->start)
	{
		SCROLL_ENTRY *next = list->start->next;
		kfree(list->start);
		list->start = next;
	}
	list->start = NULL;
	list->cur = NULL;
	list->top = NULL;
	list->last = NULL;
}

static void
scroll_up(SCROLL_INFO *list, int num)
{
	DIAG((D_fnts, NULL, "scroll_up: %d lines, top = %lx (%s), bot = %lx (%s)",
		num, list->top, list->top->text, list->bot, list->bot->text));

	while (num && list->bot->next)
	{
		list->top = list->top->next;
		list->bot = list->bot->next;
		num--;
	}
	
	DIAG((D_fnts, NULL, " --- top = %lx (%s), bot = %lx (%s)",
		list->top, list->top->text, list->bot, list->bot->text));
	
}
static void
scroll_down(SCROLL_INFO *list, int num)
{
	DIAG((D_fnts, NULL, "scroll_down: %d lines, top = %lx (%s), bot = %lx (%s)",
		num, list->top, list->top->text, list->bot, list->bot->text));
	
	while (num && list->top->prev)
	{
		list->top = list->top->prev;
		
		while (list->bot->prev && ((list->bot->n - list->top->n + 1) > list->s))
		{
			list->bot = list->bot->prev;
		}
		num--;
	}
	DIAG((D_fnts, NULL, " --- top = %lx (%s), bot = %lx (%s)",
		list->top, list->top->text, list->bot, list->bot->text));
}

static void
visible(SCROLL_INFO *list, SCROLL_ENTRY *s, bool redraw)
{
	int n;

	DIAGS(("scrl_visible: start-n=%d, top-n=%d, bot-n=%d, last-n=%d, vis-n=%d",
		list->start->n, list->top->n, list->bot->n, list->last->n, s->n));

	n = s->n - list->top->n;

	if (n < 0)
	{
		/* scroll down */
		n = -n + (list->s >> 1);
		scroll_down(list, n);
	}
	else if (n >= list->s)
	{
		/* scroll up */
		n -= list->s >> 1;
		scroll_up(list, n);
	}
	
	list->slider(list);

	if (redraw)
	{
		/* HR 290702 (do without parent window; A listbox is a OBJECT and part of a object tree.) */
		hidem();
		draw_object_tree(list->lock, list->wt, list->wi->winob, list->wi->winitem, 2, 200);
		showm();
	}
}

static SCROLL_ENTRY *
search(SCROLL_INFO *list, SCROLL_ENTRY *start, void *data, short mode)
{
	SCROLL_ENTRY *ret = NULL;
	
	if (!start)
		start = list->start;

	switch (mode)
	{
		case SEFM_BYDATA:
		{
			/*
			 * Find entry based on data ptr
			 */
			while (start)
			{
				if (start->data == data)
					break;
				start = start->next;
			}
			ret = start;
			break;
		}
		case SEFM_BYTEXT:
		{
			while (start)
			{
				if (!strcmp(start->text, data))
					break;
				start = start->next;
			}
			ret = start;
			break;
		}
	}
	return ret;
}

/*
 * Setup a scrolling list structure for an object
 * - I've provided this as I don't expect any resource editor to support
 * XaAES' extensions to the object types...
 */

/* title set by set_slist_object() */
int
set_scroll(struct xa_client *client, OBJECT *form, int item, bool selectable)
{
	OBJECT *ob = form + item;
	SCROLL_INFO *sinfo;

	if (client == C.Aes)
		sinfo = kmalloc(sizeof(*sinfo));
	else
		sinfo = umalloc(sizeof(*sinfo));

	if (!sinfo)
		return false;

	bzero(sinfo, sizeof(*sinfo));

	/* colours are those for windows */
	sinfo->prev_ob = *ob;
	
	object_set_spec(form + item, (unsigned long)sinfo);
	ob->ob_type = G_SLIST;
	ob->ob_flags |= OF_TOUCHEXIT;
	
	if (selectable)
		ob->ob_flags |= OF_SELECTABLE;
	else
		ob->ob_flags &= ~ OF_SELECTABLE;

	sinfo->tree = form;
	sinfo->item = item;

	return true;
}

void
unset_G_SLIST(struct xa_client *client, OBJECT *form, short item)
{
	struct scroll_info *list;
	OBJECT *ob = form + item;

	DIAG((D_objc, NULL, "unset_G_SLIST: obtree=%lx, index=%d",
		form, item));
	
	DIAGS(("unset_G_SLIST: obtree=%lx, index=%d",
		form, item));

	if (ob->ob_type == G_SLIST && (list = (struct scroll_info *)object_get_spec(ob)->index))
	{
		DIAGS((" --- emptying..."));
		empty_scroll_list(form, item, -1);
		*ob = list->prev_ob;
		if (client == C.Aes)
			kfree(list);
		else
			ufree(list);
	}
}

static bool
drag_vslide(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *imd)
{
	XA_SLIDER_WIDGET *sl = widg->stuff;

	DIAGS(("scroll object vslide drag!"));
	
	if (imd->cstate)
	{
		short offs;
		struct moose_data m = *imd;
		struct moose_data *md = &m;
		short msg[8] = { WM_VSLID, 0, 0, wind->handle, 0, 0,0,0};
		
		if (md->cstate & MBS_RIGHT)
		{
			RECT s, b, d, r;

			rp_2_ap(wind, widg, &widg->ar);
			
			b = s = widg->ar;
			
			s.x += sl->r.x;
			s.y += sl->r.y;
			s.w = sl->r.w;
			s.h = sl->r.h;

			lock_screen(wind->owner->p, 0, 0, 0);
			graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			unlock_screen(wind->owner->p, 0);

			offs = bound_sl(pix_to_sl(r.y - widg->ar.y, widg->r.h - sl->r.h));

			if (offs != sl->position)
			{
				sl->rpos = msg[4] = offs;
				slist_msg_handler(wind, NULL, 0,0, msg);
			}
		}
		else
		{
			short y, old_y = 0, old_offs = 0, pmx, pmy;
			short cont = 0;

			S.wm_count++;
			while (md->cstate)
			{
				if (!cont)
				{
					cont = true;
					/* Always have a nice consistent sizer when dragging a box */
					graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
					rp_2_ap(wind, widg, &widg->ar);
					old_offs = md->y - (widg->ar.y + sl->r.y);
					old_y = md->y;
					y = md->sy;
				}
				else
					y = md->y;

				if (old_y != y)
				{
					offs = bound_sl(pix_to_sl((y - old_offs) - widg->ar.y, widg->r.h - sl->r.h));
		
					if (offs != sl->position && wind->send_message)
					{
						sl->rpos = msg[4] = offs;
						slist_msg_handler(wind, NULL, 0,0, msg);
					}
					old_y = y;
				}
				pmx = md->x, pmy = md->y;
				while (md->cstate && md->x == pmx && md->y == pmy)
				{
					wait_mouse(wind->owner, &md->cstate, &md->x, &md->y);
				}
			}
			S.wm_count--;
		}
		graf_mouse(wind->owner->mouse, NULL, NULL, false);
	}
	return true;
}
static bool
drag_hslide(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *imd)
{
	XA_SLIDER_WIDGET *sl = widg->stuff;

	DIAGS(("scroll object vslide drag!"));
	
	if (imd->cstate)
	{
		short offs;
		struct moose_data m = *imd;
		struct moose_data *md = &m;
		short msg[8] = { WM_HSLID, 0, 0, wind->handle, 0, 0,0,0};
		
		if (md->cstate & MBS_RIGHT)
		{
			RECT s, b, d, r;

			rp_2_ap(wind, widg, &widg->ar);
			
			b = s = widg->ar;
			
			s.x += sl->r.x;
			s.y += sl->r.y;
			s.w = sl->r.w;
			s.h = sl->r.h;

			lock_screen(wind->owner->p, 0, 0, 0);
			graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			unlock_screen(wind->owner->p, 0);

			offs = bound_sl(pix_to_sl(r.x - widg->ar.x, widg->r.w - sl->r.w));

			if (offs != sl->position)
			{
				sl->rpos = msg[4] = offs;
				slist_msg_handler(wind, NULL, 0,0, msg);
			}
		}
		else
		{
			short x, old_x = 0, old_offs = 0, pmx;
			short cont = 0;

			S.wm_count++;
			while (md->cstate)
			{
				if (!cont)
				{
					cont = true;
					/* Always have a nice consistent sizer when dragging a box */
					graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
					rp_2_ap(wind, widg, &widg->ar);
					old_offs = md->x - (widg->ar.x + sl->r.x);
					old_x = md->x;
					x = md->sx;
				}
				else
					x = md->x;

				if (old_x != x)
				{
					offs = bound_sl(pix_to_sl((x - old_offs) - widg->ar.x, widg->r.w - sl->r.w));
		
					if (offs != sl->position && wind->send_message)
					{
						sl->rpos = msg[4] = offs;
						slist_msg_handler(wind, NULL, 0,0, msg);
					}
					old_x = x;
				}
				pmx = md->x;
				while (md->cstate && md->x == pmx)
				{
					wait_mouse(wind->owner, &md->cstate, &md->x, &md->y);
				}
			}
			S.wm_count--;
		}
		graf_mouse(wind->owner->mouse, NULL, NULL, false);
	}
	return true;
}

/* preparations for windowed list box widget;
 * most important is to get the drawing smooth and simple.
 * get rid of all those small (confolded) constant value's.
 */
SCROLL_INFO *
set_slist_object(enum locks lock,
        	 XA_TREE *wt,
		 OBJECT *form,
		 struct xa_window *parentwind,
		 short item,
		 scrl_widget *closer,
		 scrl_widget *fuller,
		 scrl_click *dclick,
		 scrl_click *click,
		 char *title,
		 char *info,
		 void *data,
		 short lmax)	/* Used to determine whether a horizontal slider is needed. */
{
	RECT r;
	XA_WIND_ATTR wkind = UPARROW|VSLIDE|DNARROW;
	OBJECT *ob = form + item;
	SCROLL_INFO *list = (SCROLL_INFO *)object_get_spec(ob)->index;

	list->pw = parentwind;
	list->wt = wt;
	list->data = data;

	list->dclick = dclick;			/* Pass the scroll list's double click function */
	list->click = click;			/* Pass the scroll list's click function */

	list->title = title;
	/* rp_2_ap; it is an OBJECT, not a widget */
	//r = *(RECT*)&ob->ob_x;
	obj_area(wt, item, &r);
	/* We want to use the space normally occupied by the shadow;
	    so we do a little cheat here. */
	//r.w += SHADOW_OFFSET;
	//r.h += SHADOW_OFFSET;
	//ob_offset(form, item, &r.x, &r.y);
	//obj_offset(wt, item, &r.x, &r.y);
	if (title)
		wkind |= NAME;
	if (info)
		wkind |= INFO;
	if (closer)
		wkind |= CLOSER;
	if (fuller)
		wkind |= FULLER;
	if (lmax * screen.c_max_w + ICON_W > r.w - 24)
		wkind |= LFARROW|HSLIDE|RTARROW;
	
	wkind |= TOOLBAR;

	list->wi = create_window(lock,
				 do_winmesag,
				 slist_msg_handler,
				 wt->owner,
				 true,			/* nolist */
				 wkind,
				 created_for_AES | created_for_SLIST,
				 0, false,r,NULL,NULL);
	if (list->wi)
	{
		int dh;
		get_widget(list->wi, XAW_TITLE)->stuff = title;
		if (info)
			get_widget(list->wi, XAW_INFO)->stuff = info;

		get_widget(list->wi, XAW_VSLIDE)->drag = drag_vslide;
		get_widget(list->wi, XAW_HSLIDE)->drag = drag_hslide;

		//list->wi->data = list;
		
		list->wi->winob	= form;		/* The parent object of the windowed list box */
		list->wi->winitem = item;
		r = list->wi->wa;
		r.h /= screen.c_max_h;
		r.h *= screen.c_max_h;		/* snap the workarea hight */
		list->s = r.h / screen.c_max_h;
		dh = list->wi->wa.h - r.h;
		ob->ob_height -= dh;
		list->wi->r.h -= dh;

		list->slider	= sliders;
		list->closer	= closer;
		list->fuller	= fuller;
		list->vis	= visible;
		list->search	= search;
		list->lock	= lock;

		list->v = (r.w - cfg.widg_w) / screen.c_max_w;
		list->max = lmax;

		open_window(lock, list->wi, list->wi->r);
	}
	return list;
}

/* better control over the content of scroll_entry. */
bool
add_scroll_entry(OBJECT *form, int item,
		 OBJECT *icon, void *text,
		 SCROLL_ENTRY_TYPE flag,
		 void *data)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *last, *new;
	OBJECT *ob = form + item;

	list = (SCROLL_INFO *)object_get_spec(ob)->index;

	if (flag & FLAG_AMAL)
		new = kmalloc(sizeof(*new) + strlen(text) + 1);
	else
		new = kmalloc(sizeof(*new));
	
	if (!new)
		return false;

	new->next = NULL;
	last = list->start;
	new->data = data;

	if (last)
	{
		while (last->next)
			last = last->next;
		last->next = new;
		new->prev = last;
		new->n = last->n + 1;
	}
	else
	{
		new->prev = NULL;
		list->start = list->top = list->bot = list->last = new;	/* cur is left zero, which means no current : no selection. */
		new->n = 1;
	}

	if (new->n <= list->s)
		list->bot = new;

	if (flag & FLAG_AMAL)
	{
		new->text = new->the_text;
		strcpy(new->the_text, text);
	}
	else	
		new->text = text;

	new->icon = icon;
	new->flag = flag;
	
	if (icon)
		icon->ob_x = icon->ob_y = 0;

	list->n = new->n;
	
	return true;
}

/* Modified such that a choise can be made. */
void
empty_scroll_list(OBJECT *form, int item, SCROLL_ENTRY_TYPE flag)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *this, *next, *prior = NULL;
	OBJECT *ob = form + item;
	int n = 0;
	list = (SCROLL_INFO *)object_get_spec(ob)->index;
	this = next = list->start;

	DIAGS(("empty_scroll_list: list=%lx, obtree=%lx, ind=%d",
		list, form, item));

	while (this)
	{
		next = this->next;
		
		DIAGS((" --- this=%lx, next=%lx, this->flag=%x",
			this, this->flag));

		if ((this->flag & flag) || flag == -1)
		{
			if ((this->flag & FLAG_MAL) && this->text)
			{
				kfree(this->text);
				this->text = NULL;
			}
			
			if (this == list->start)
				list->start = next;
			
			if (this == list->last)
				list->last = prior;

			if (prior)
				prior->next = next;

			DIAGS(("empty_scroll_list: free %lx", this));
			kfree(this);
		}
		else
		{
			this->n = ++n;
			prior = this;
		}
		this = next;
	}

	list->n = n;
	list->top = list->start;
	list->cur = NULL;		/* HR: This does nothing more than inhibit the selection of a line. */
}
		
/* HR: The application point of view of the list box */
/* HR: Immensely cleanup of the code by using a window and it's mechanisms for the list box.
       Including fully functional sliders and all arrows.
*/
static void
slist_msg_handler(
	struct xa_window *wind,
	struct xa_client *to,
	short amq, short qmf,
	short *msg)
{
	enum locks lock = 0;
	SCROLL_INFO *list;
	SCROLL_ENTRY *old;
	OBJECT *ob;
	int p,ol;

	ob = wind->winob + wind->winitem;
	list = (SCROLL_INFO *)object_get_spec(ob)->index;
	old = list->top;
	ol  = list->left;

	switch (msg[0])		/* message number */
	{
	case WM_ARROWED:
	{
		if (msg[4] < WA_LFPAGE)
		{
			p = list->s;
			if (p < list->n)
			{
				switch (msg[4])
				{
				case WA_UPLINE:
				{
					scroll_down(list, 1);
					break;
				}
			#if 0
					if (list->top->prev)
						list->state = SCRLSTAT_UP,
						list->top = list->top->prev;
				break;
			#endif
				case WA_DNLINE:
				{
					scroll_up(list, 1);
					break;
				}
			#if 0
					if (list->bot->next)
						list->state = SCRLSTAT_DOWN,
						list->top = list->top->next;
				break;
			#endif
				case WA_UPPAGE:
				{
					scroll_down(list, list->s - 1);
					break;
				}
			#if 0
					while (--p && list->top->prev) 
						list->top = list->top->prev;
				break;
			#endif
				case WA_DNPAGE:
				{
					scroll_up(list, list->s - 1);
					break;
				}
			#if 0
					while (--p && list->bot->next)
						list->bot = list->bot->next,
						list->top = list->top->next;
			#endif
				}
			}
		}
		else
		{
			p = list->v;
			if (p < list->max)
			{
				switch (msg[4])
				{
				case WA_LFLINE:
					list->left--;
				break;
				case WA_RTLINE:
					list->left++;
				break;
				case WA_LFPAGE:
					list->left -= p-1;
				break;
				case WA_RTPAGE:
					list->left += p-1;
				}
				if (list->left < 0)
					list->left = 0;
				if (list->left > list->max - p)
					list->left = list->max - p;
			}
		}
		break;
	}
	case WM_VSLID:
	{
		p = list->s;
		if (p < list->n)
		{
			int new = sl_to_pix(list->n - (p-1), msg[4]);

			new -= list->top->n;
			if (new < 0)
				scroll_down(list, -new);
			else
				scroll_up(list, new);
		#if 0
			if (list->top->n > new) /* go up */
			{
				while (list->top->n > new && list->top->prev)
					list->top = list->top->prev;
			}
			else /* go down */
			{
				while (list->top->n < new && list->bot->next)
					list->bot = list->bot->next,
					list->top = list->top->next;
			}
		#endif
		}
		break;
	}
	case WM_HSLID:
	{
		p = list->v;
		if (p < list->max)
			list->left = sl_to_pix(list->max - p, msg[4]);
		break;
	}
	case WM_CLOSED:
	{
		if (list->closer)
			list->closer(list);
		break;
	}
	case WM_FULLED:
	{
		if (list->fuller)
			list->fuller(list);
	}
	default:
	{
		return;
	}
	}

	/* next step is to take the scrolling out of d_g_slist and make it a seperate
	 * generalized function, using the difference between old and list->top.
	 * list->state would disappear.
	 */
	if (old != list->top || ol != list->left)
	{
		list->slider(list);
		/* do without parent window; A listbox is a OBJECT and part of a object tree */
		hidem();
		draw_object_tree(lock, list->wt, list->wi->winob, list->wi->winitem, 2, 200);
		showm();
	}
}
int
scrl_cursor(SCROLL_INFO *list, ushort keycode)
{
	SCROLL_ENTRY *save = list->cur;

	if (!list->cur)
		list->cur = list->top;

	switch (keycode)
	{
	case 0x4800:			/* up arrow */
	{
		if (list->cur->prev)
			list->cur = list->cur->prev;

		break;
	}
	case 0x5000:			/* down arrow */
	{
		if (list->cur->next)
			list->cur = list->cur->next;
		break;
	}
	case 0x4900:			/* page up */
	case 0x4838:			/* shift + up arrow */
	{
		short msg[8] = { WM_ARROWED,0,0,list->wi->handle,WA_UPPAGE,0,0,0 };
		slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		return keycode;
	}
	case 0x5032:			/* shift + down arrow */
	case 0x5100:			/* page down */
	{
		short msg[8] = { WM_ARROWED,0,0,list->wi->handle,WA_DNPAGE,0,0,0 };
		slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		return keycode;
	}
	case 0x4700:			/* home */
	{
		list->cur = list->start;
		break;
	}
	case 0x4737:			/* shift + home */
	{
		list->cur = list->last;
		break;
	}
	default:
	{
		list->cur = save;	/* Only change cur if a valid cursor key
		                       was pressed. See fs_dclick() */
		return -1;
	}
	}

	list->vis(list,list->cur, true);
	return keycode;
}

/* HR 181201: pass all mouse data to these functions using struct moose_data. */
void
click_scroll_list(enum locks lock, OBJECT *form, int item, const struct moose_data *md)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	OBJECT *ob = form + item;
	short cy = md->y;

	list = (SCROLL_INFO *)object_get_spec(ob)->index;
	
	DIAGS(("click_scroll_list: state=%d, cstate=%d", md->state, md->cstate));
	
	{
		RECT r;
		ob_area(form, item, &r);
		list->wi->r = list->wi->rc = r;
		calc_work_area(list->wi);
	}
	
	if (!do_widgets(lock, list->wi, 0, md))
	{
		short y = screen.c_max_h;

		cy -= list->wi->wa.y;

		this = list->top;
		while(this && y < cy)
		{
			this = this->next;
			y += screen.c_max_h;
		}

		if (this)
		{
			list->cur = this;			

/* HR 290702 (do without parent window; A listbox is a OBJECT and part of a object tree.) */
/* HR 111102 Nasty bug :-(  list->click can close the whole window containing the list.
            So the below draw is moved from the end of this function to here. */

			hidem();
			draw_object_tree(lock, list->wt, form, item, 2, 200);
			showm();

			if (list->click)			/* Call the new object selected function */
				(*list->click)(lock, list, form, item);
		}
	}
}

void
dclick_scroll_list(enum locks lock, OBJECT *form, int item, const struct moose_data *md)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	OBJECT *ob = form + item;
	short y = screen.c_max_h, cx = md->x, cy = md->y;	

	list = (SCROLL_INFO *)object_get_spec(ob)->index;

	{
		RECT r;
		ob_area(form, item, &r);
		list->wi->r = list->wi->rc = r;
		calc_work_area(list->wi);
	}

	if (!do_widgets(lock, list->wi, 0, md))		/* HR 161101: mask */
	{
		if (!m_inside(cx, cy, &list->wi->wa))
			return;
	
		cy -= list->wi->wa.y;
	
		this = list->top;
		while(this && y < cy)
		{
			this = this->next;
			y += screen.c_max_h;
		}
	
		if (this)
			list->cur = this;
		
		if (list->dclick)
			(*list->dclick)(lock, list, form, item);
	}
}
