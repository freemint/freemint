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

#define ONLY_OPENED 1
#define NO_DOWN     2

static void
slist_msg_handler(struct xa_window *wind, struct xa_client *to, short amq, short qmf, short *msg);


static struct xa_wtxt_inf default_fnt =
{
 WTXT_NOCLIP,
/* id  pnts efx   fgc      bgc */
 {  1,  10,   0, G_BLACK, G_WHITE },	/* Normal */
 {  1,  10,   0, G_WHITE, G_BLACK },	/* Selected */
 {  1,  10,   0, G_BLACK, G_WHITE },	/* Highlighted */

};
static struct xa_wcol_inf default_col =
{
	WCOL_DRAWBKG, MD_REPLACE,
      /* color    inter     style   boxcol   boxsize   tl       br        texture */
	{G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK,   NULL},
	{G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK,   NULL},
	{G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK,   NULL},
};

static SCROLL_ENTRY *
next_entry(SCROLL_ENTRY *this, short flags, short maxlevel, short *level)
{
	short lev;

	if (level)
		lev = *level;
	else
		lev = 0;

//	display("next_entry %lx (n=%lx, p=%lx, u=%lx, d=%lx)",
//		this, this->next, this->prev, this->up, this->down);
	
	if (this)
	{
		if (flags & ENT_VISIBLE)
		{
			if (this->down && (this->xstate & OS_OPENED) && (maxlevel == -1 || maxlevel > lev))
				this = this->down, lev++;
			else if (this->next)
				this = this->next;
			else
			{
				if (flags & ENT_ISROOT)
				{
					if (!lev)
						this = this->next;
					else
					{
						lev--;
						this = this->up;
						while (this)
						{
							if (this->next)
							{
								this = this->next;
								break;
							}
							else if (lev)
							{
								this = this->up;
								lev--;
							}
							else
								this = NULL;
						}
					}
				}
				else
				{
					this = this->up;
					lev--;
					while (this)
					{
						if (this->next)
						{
							this = this->next;
							break;
						}
						else
							this = this->up, lev--;
					}
				}
				
			}
		}
		else
		{
			if (this->down && (maxlevel == -1 || maxlevel > lev))
				this = this->down, lev++;
			else if (this->next)
				this = this->next;
			else
			{
				if (flags & ENT_ISROOT)
				{
					if (!lev)
						this = this->next;
					else
					{
						lev--;
						this = this->up;
						while (this)
						{
							if (this->next)
							{
								this = this->next;
								break;
							}
							else if (lev)
							{
								this = this->up;
								lev--;
							}
							else
								this = NULL;
						}
					}
				}
				else
				{
					this = this->up;
					lev--;
					while (this)
					{
						if (this->next)
						{
							this = this->next;
							break;
						}
						else
							this = this->up, lev--;
					}
				}
			}
		}
	}
//	display("--- return %lx (n=%lx, p=%lx, u=%lx, d=%lx)",
//		this, this->next, this->prev, this->up, this->down);
	if (level)
		*level = lev;

	return this;
}

static SCROLL_ENTRY *
prev_entry(SCROLL_ENTRY *this, short flags)
{
	if (this->prev)
	{
		this = this->prev;
		while (this->down && (this->xstate & OS_OPENED))
		{
			this = this->down;
			while (this->next)
				this = this->next;
		}
	}
	else if (this->up)
		this = this->up;

	return this;
}

static bool
is_in_list(SCROLL_INFO *list, SCROLL_ENTRY *this)
{
	SCROLL_ENTRY *e = list->start;

	while (e)
	{
		if (e == this)
			return true;
		e = next_entry(e, 0, -1, NULL);
	}
	return false;
}

static void
change_entry(SCROLL_INFO *list, SCROLL_ENTRY *entry, bool select, bool rdrw)
{
	short state = entry->state;

	if (select)
		state |= OS_SELECTED;
	else	
		state &= ~OS_SELECTED;

	if (entry->state != state)
	{
		entry->state = state;
		if (rdrw) list->redraw(list, entry);
	}
}

static struct se_tab se_tabs[] =
{
	{ 0, {0, 0, -1/*32*8*/, 0}},
	{ SETAB_RJUST|SETAB_REL, {0, 0, -1/*6*8*/, 0} },
	{ SETAB_END, {0, 0, 0, 0}},
};

/*
 * Work out width/height of each individual string in a SETYPE_TEXT entry
 */
static bool
entry_text_wh(SCROLL_INFO *list, SCROLL_ENTRY *this)
{
	bool fullredraw = false;
	struct se_tab *stabs = list->se_tabs;
	struct se_text_tabulator *tabs = list->tabs;
	struct se_text *setext = this->c.td.text.text;
	int i;
	short tw = 0, h = 0;
	char *s;

	for (i = 0; i < this->c.td.text.n_strings; i++)
	{
		s = setext->text;
		text_extent(s, &this->c.td.text.fnt->n, &setext->w, &setext->h);
		
		if (setext->h > h)
			h = setext->h;

		setext->w += 4;
		if (stabs->r.w == -1)
		{
			if (tabs->r.w < setext->w)
			{
				tabs->r.w = setext->w;
				fullredraw = true;
			}
		}
		tw += tabs->r.w;

		setext = setext->next;
		stabs++, tabs++;
	}

	this->r.w += tw;
	
	if (h > this->r.h)
		this->r.h = h;

	return fullredraw;
}

static void
reset_tabs(SCROLL_INFO *list)
{
	struct se_text_tabulator *tabs = list->tabs;
	struct se_tab *stabs = list->se_tabs;
	while (stabs)
	{
		tabs->flags = stabs->flags;
		tabs->r = stabs->r;
		tabs->widest = 0;
		tabs->highest = 0;

		if (stabs->r.w == -1)
			tabs->r.w = 0;
		if (stabs->r.h == -1)
			tabs->r.h = 0;
		if (stabs->flags & SETAB_END)
			stabs = NULL;
		else
			tabs++, stabs++;
	}
}

static short
draw_nesticon(RECT *xy, SCROLL_ENTRY *this)
{
	int i;
	RECT r;
	short x_center, y_center, width = 16;
	short pnt[4];

	x_center = (width >> 1);
	y_center = (xy->h >> 1);

	r.x = xy->x + x_center - 4;
	r.y = xy->y + y_center - 4;
	r.w = 9;
	r.h = 9;

	if (this->down || (this->xstate & OS_NESTICON))
	{
		f_interior(FIS_SOLID);
		f_color(G_WHITE);
		gbar(0, &r);

		l_color(G_BLACK);
		gbox(0, &r);

		//if (this->prev || this->up)
		{
			l_color(G_LBLACK);
			pnt[0] = xy->x + x_center;
			pnt[1] = xy->y;
			pnt[2] = pnt[0];
			pnt[3] = r.y;
			v_pline(C.vh, 2, pnt);
		}
		//if (this->next || (this->xstate & OS_OPENED))
		{
			pnt[0] = xy->x + x_center;
			pnt[1] = r.y + 9;
			pnt[2] = pnt[0];
			pnt[3] = xy->y + xy->h - 1;
			v_pline(C.vh, 2, pnt);
		}

		l_color(G_BLACK);
		pnt[0] = r.x + 2;
		pnt[1] = r.y + 4;
		pnt[2] = pnt[0] + 4;
		pnt[3] = pnt[1];
	
		v_pline(C.vh, 2, pnt);
	
		if (!(this->xstate & OS_OPENED))
		{
			pnt[0] = r.x + 4;
			pnt[1] = r.y + 2;
			pnt[2] = pnt[0];
			pnt[3] = r.y + 6;
			v_pline(C.vh, 2, pnt);
		}
	}
	else
	{
		l_color(G_LBLACK);
		//if (this->prev || this->up)
		{
			pnt[0] = xy->x + x_center;
			pnt[1] = xy->y;
			pnt[2] = pnt[0];
			pnt[3] = xy->y + y_center;
			v_pline(C.vh, 2, pnt);
		}
		//if (this->next)
		{
			pnt[0] = xy->x + x_center;
			pnt[1] = xy->y + y_center;
			pnt[2] = pnt[0];
			pnt[3] = xy->y + xy->h - 1;
			v_pline(C.vh, 2, pnt);
		}
		
		pnt[0] = xy->x + x_center;
		pnt[1] = xy->y + y_center;
		pnt[2] = xy->x + width;
		pnt[3] = pnt[1];
		v_pline(C.vh, 2, pnt);
	}

	if (this->level)
	{
		short x = xy->x;
		l_color(G_LBLACK);
		for (i = 0; i < this->level; i++)
		{
			x -= width;
			pnt[0] = x + x_center;
			pnt[1] = xy->y;
			pnt[2] = pnt[0];
			pnt[3] = xy->y + xy->h - 1;
			v_pline(C.vh, 2, pnt);
		}
	}
	return width;
}

static void
display_list_element(enum locks lock, SCROLL_INFO *list, SCROLL_ENTRY *this, RECT *xy, const RECT *clip)
{
	bool sel = this->state & OS_SELECTED;
	XA_TREE tr = nil_tree;
	
	if (this)
	{
		bool ii = (list->flags & SIF_ICONINDENT) ? true : false;
		short indent = this->indent;
		struct se_text_tabulator *tabs = list->tabs;
		struct se_text *tetext;
		struct xa_fnt_info *wtxt;
		short x, y, w = 0, h = 0, f;
		
		f_color(sel ? G_BLACK : G_WHITE);
		bar(0, xy->x, xy->y, xy->w, this->r.h);
		
		xy->x += indent;

		if (list->flags & SIF_TREEVIEW)
		{
			draw_nesticon(xy, this);
			x = draw_nesticon(xy, this);
			xy->x += x;
			indent += x;
		}

		if (sel)
			wtxt = &this->c.td.text.fnt->s;
		else
			wtxt = &this->c.td.text.fnt->n;

		f = this->c.td.text.fnt->flags;

		wr_mode(MD_TRANS);
		t_font(wtxt->p, wtxt->f);
		vst_effects(C.vh, wtxt->e);
		x = xy->x;
		if (ii)
		{
			x += list->icon_w;
			indent += list->icon_w;
		}
		y = xy->y;

		tetext = this->c.td.text.text;

		while (tetext && tabs)
		{
			short x2, y2, dx, dy;
			char t[200];

			dx = x + tabs->r.x;
			x2 = dx + tabs->r.w - 1 - indent;
			dy = y + tabs->r.y;
			y2 = dy + tabs->r.h - 1;

			x = x2;
			y = y2;
			
			prop_clipped_name(tetext->text, t, tabs->r.w - indent, &w, &h, 0);
			indent = 0;
			
			if (tabs->flags & SETAB_RJUST)
			{
				dx = x2 - w;
			}
			else if (tabs->flags & SETAB_CJUST)
			{
				dx = x2 - (w >> 1);
			}

			dy += ((this->r.h - h) >> 1);
			
			if (f & WTXT_DRAW3D)
			{
				if (sel && (f & WTXT_ACT3D))
					dx++, dy++;
	
				t_color(wtxt->bgc);
				dx++;
				dy++;
				v_gtext(C.vh, dx, dy, t);
				dx--;
				dy--;
			}
			t_color(wtxt->fgc);
			v_gtext(C.vh, dx, dy, t);

			if (tabs->flags & SETAB_END)
				tabs = NULL;
			else
				tabs++;
			tetext = tetext->next;
		}
		/* normal */
		vst_effects(C.vh, 0);

		//wtxt_output(this->c.fnt, this->c.text, this->state, xy, ii ? list->icon_w : 0, 0);
		
		f_color(G_WHITE);
		
		if (ii && this->c.icon)
		{
			if (sel)
				this->c.icon->ob_state |= OS_SELECTED;
			else
				this->c.icon->ob_state &= ~OS_SELECTED;

			tr.tree = this->c.icon;
			tr.owner = list->wt->owner;
			display_object(lock, &tr, clip, 0, xy->x + 1, xy->y + 1, 12);
		}
	}
}

void
draw_slist(enum locks lock, SCROLL_INFO *list, SCROLL_ENTRY *entry, const RECT *clip)
{
	struct xa_window *wind = list->wi;
	struct scroll_entry *this = list->top;
	RECT r, xy;

	if (xa_rect_clip(clip, &wind->wa, &r))
	{
		set_clip(&r);

		wr_mode(MD_TRANS);

		xy.x = wind->wa.x - list->start_x;
		xy.y = wind->wa.y - list->off_y;
		xy.w = wind->wa.w + list->start_x;
		xy.h = wind->wa.h + list->off_y;

		while (this && xy.y < (wind->wa.y + wind->wa.h))
		{
			RECT ar;

			ar.x = xy.x;
			ar.y = xy.y;
			ar.w = xy.w; //wind->wa.w;
			ar.h = this->r.h;
			
			if (!entry || entry == this)
			{
				display_list_element(lock, list, this, &ar, &r);
			}
			
			xy.y += this->r.h;
			xy.h -= this->r.h;
			this = next_entry(this, ENT_VISIBLE, -1, NULL);
		}
		
		if (!entry && xy.h > 0)
		{
			f_color(G_WHITE);
			bar(0, xy.x, xy.y, xy.w, xy.h);
		}
		set_clip(clip);
	}
}

static void
check_movement(SCROLL_INFO *list)
{
	struct xa_window *wind = list->wi;
	OBJECT *obtree = list->wt->tree;
	short x, y;

	x = obtree->ob_x + list->rel_x;
	y = obtree->ob_y + list->rel_y;

	if (x != wind->rc.x ||
	    y != wind->rc.y )
	{
		wind->r.x = wind->rc.x = x;
		wind->r.y = wind->rc.y = y;
		calc_work_area(list->wi);
	}
}
static bool
canredraw(SCROLL_INFO *list)
{
	struct xa_window *wind;

	if (!(wind = list->pw))
		wind = list->wt->wind;
	if (wind && (wind->window_status & (XAWS_OPEN|XAWS_SHADED|XAWS_HIDDEN)) == XAWS_OPEN)
	{
		check_movement(list);
		return true;
	}
	return false;
}
/*
 * Reset listwindow's widgets according to contents
 */
static bool
reset_listwi_widgets(SCROLL_INFO *list, short redraw)
{
	XA_WIND_ATTR tp = list->wi->active_widgets;
	bool rdrw = false;
	bool as = list->flags & SIF_AUTOSLIDERS;
	if (list->wi)
	{
		if (list->total_w > list->wi->wa.w)
		{
			if (as)
				tp |= (LFARROW|RTARROW|HSLIDE);
		}
		else
		{
			if (as)
				tp &= ~(LFARROW|RTARROW|HSLIDE);
			if ((list->start_x | list->off_x))
			{
				list->start_x = 0;
				list->off_x = 0;
				rdrw = true;
			}
		}
			
		if (list->total_h > list->wi->wa.h)
		{
			if (as)
				tp |= (UPARROW|DNARROW|VSLIDE);
		}
		else
		{
			if (as)
				tp &= ~(UPARROW|DNARROW|VSLIDE);
			if ((list->start_y | list->off_y))
			{
				list->start_y = 0;
				list->off_y = 0;
				list->top = list->start;
				rdrw = true;
			}
		}

		if (rdrw || (tp != list->wi->active_widgets))
		{
			change_window_attribs(0, list->wi->owner, list->wi, tp, false, list->wi->r, NULL);
			if (canredraw(list))
			{
				draw_window(0, list->wi, &list->wi->r);
				if (redraw)
				{
					list->redraw(list, NULL);
				}
			}
			return true;
		}
	}
	return false;
}

static void
sliders(struct scroll_info *list, bool rdrw)
{
	int rm = 0;

	if (XA_slider(list->wi, XAW_VSLIDE,
		  list->total_h,
		  list->wi->wa.h,
		  list->start_y))
		rm |= 1;

	if (XA_slider(list->wi, XAW_HSLIDE,
		  list->total_w,
		  list->wi->wa.w,
		  list->start_x))
		rm |= 2;

	if (rm && rdrw)
	{
		struct xa_window *wind = list->pw;
		if (!wind)
			wind = list->wt->wind;
		if (wind && wind->rect_start)
		{
			check_movement(list);
			if (rm & 1)
				display_widget(0, list->wi, get_widget(list->wi, XAW_VSLIDE), wind->rect_start);
			if (rm & 2)
				display_widget(0, list->wi, get_widget(list->wi, XAW_HSLIDE), wind->rect_start);
		}
	}
}

static struct scroll_entry *
get_last_entry(SCROLL_INFO *list)
{
	struct scroll_entry *this = list->start;

	while (this)
		this = next_entry(this, 0, -1, NULL);
	
	return this;
}

static struct scroll_entry *
get_next_selected(struct scroll_info *list, struct scroll_entry *this)
{
	this = next_entry(this, 0, -1, NULL);
	while (this)
	{
		if (this->state & OS_SELECTED)
			break;
		this = next_entry(this, 0, -1, NULL);
	}
	return this;
}

static struct scroll_entry *
get_first_selected(SCROLL_INFO *list)
{
	struct scroll_entry *this = list->start;

	while (this)
	{
		if (this->state & OS_SELECTED)
			break;
		this = next_entry(this, 0, -1, NULL);
	}
	return this;
}

static bool
hidden_entry(SCROLL_ENTRY *this)
{
	bool hidden = false;

	this = this->up;
	while (this)
	{
		if (!(this->xstate & OS_OPENED))
		{
			hidden =  true;
			break;
		}
		this = this->up;
	}
	return hidden;
}

static void
unselect_all(SCROLL_INFO *list, short redraw)
{
	SCROLL_ENTRY *this;
	this = get_first_selected(list);
	while (this)
	{
		change_entry(list, this, false, redraw ? !hidden_entry(this) : redraw);
		this = get_next_selected(list, this);
	}
}

static short
find_widest(SCROLL_INFO *list, SCROLL_ENTRY *start)
{
	short widest = 0;
	
	if (!start)
		start = list->start;

	while (start)
	{
		if (start->r.w > widest)
			widest = start->r.w;
		start = next_entry(start, ENT_VISIBLE, -1, NULL);
	}
	return widest;
}


static int
get_entry_lrect(struct scroll_info *l, struct scroll_entry *e, short flags, LRECT *r)
{
	long x = 0, y = 0, w = 0, h = 0;
	struct scroll_entry *this = l->start;
	short level = 0;

	while (this)
	{
		if (e == this)
			break;
		y += this->r.h;
		this = next_entry(this, ENT_VISIBLE, -1, NULL);
	}
	if (this)
	{
		w = this->r.w;
		h = this->r.h;
		if ((flags & 1))
		{
			this = next_entry(this, (ENT_VISIBLE|ENT_ISROOT), -1, &level);
			if (this && level)
			{
				level = 0;
				while (this)
				{
					w += this->r.w, h += this->r.h;
					this = next_entry(this, (ENT_VISIBLE|ENT_ISROOT), -1, &level);
				}
			}
		}
		r->x = x;
		r->y = y;
		r->w = w;
		r->h = h;
		return 1;
	}
	else
	{
		r->w = r->h = 0L;
		return 0;
	}
}
/*
 * If entyr == NULL, redraw whole box,
 * else redraw only the give entry.
 * If scroll_list have no parent window, nothing is redraw,
 * as then we do not have a rectanlge list to use.
 */
static void
slist_redraw(SCROLL_INFO *list, SCROLL_ENTRY *entry)
{
	struct xa_window *wind = list->pw;
	struct xa_rect_list *rl;

	if (!wind)
		wind = list->wt->wind;

	if (entry)
	{
		LRECT r;
		get_entry_lrect(list, entry, 0, &r);
		if (!(r.w | r.h) || ((r.y + r.h) < list->start_y) || (r.y > (list->start_y + list->wi->wa.h)))
		{
			return;
		}
	}
	if (wind && ((wind->window_status & (XAWS_OPEN|XAWS_SHADED|XAWS_HIDDEN)) == XAWS_OPEN) && (rl = wind->rect_start))
	{
		check_movement(list);
		
		hidem();
		while (rl)
		{
			draw_slist(0, list, entry, &rl->r);
			rl = rl->next;
		}
		clear_clip();
		showm();
	}
}

static bool
canblit(SCROLL_INFO *list)
{
	bool ret = false;
	struct xa_window *wind = list->pw;
	struct xa_rect_list *rl = NULL;

	if (!wind)
		wind = list->wt->wind;
	if (wind)
	{
		RECT r;
		rl = wind->rect_start;
		while (rl)
		{
			if (xa_rect_chk(&rl->r, &list->wi->wa, &r) == 2)
			{
				ret = true;
				break;
			}
			rl = rl->next;
		}
	}
	return ret;
}

/*
 * Check if entry has its own wtxt structure or if its using the global
 * default one. If using default, allocate mem for and copy the default
 * one over. Need this so that chaning one entry's fnt info dont affect
 * all others
 */
static void
alloc_entry_wtxt(struct scroll_entry *entry, struct xa_wtxt_inf *newwtxt)
{
	if (!(entry->iflags & SEF_WTXTALLOC))
	{
		struct xa_wtxt_inf *old = entry->c.td.text.fnt;
		entry->c.td.text.fnt = kmalloc(sizeof(struct xa_wtxt_inf));
		if (entry->c.td.text.fnt)
		{
			entry->iflags |= SEF_WTXTALLOC;
			
			if (newwtxt)
				*entry->c.td.text.fnt = *newwtxt;
			else if (old)
				*entry->c.td.text.fnt = *old;
			else
				*entry->c.td.text.fnt = *(struct xa_wtxt_inf *)&default_fnt;
		}
		else
		{
			entry->c.td.text.fnt = old;
			return;
		}
	}
	else if (newwtxt)
		*entry->c.td.text.fnt = *newwtxt;
}
	
static int
set(SCROLL_INFO *list,
    SCROLL_ENTRY *entry,
    short what,
    unsigned long arg,
    short rdrw)
{
	bool redrw = false;
	int ret = 0;

	switch (what)
	{
		case SESET_STATE:
		{
			short state = entry->state, newstate = arg, mask = arg >> 16;
			
			state &= mask;
			state |= newstate;
			
			if (entry->state != state)
			{
				entry->state = state;
				redrw = true;
				ret = 1;
			}
			break;
		}
		case SESET_NFNT:
		{
			struct xa_fnt_info *f = (void*)arg;
			alloc_entry_wtxt(entry, NULL);
			if (entry->iflags & SEF_WTXTALLOC)
			{
				entry->c.td.text.fnt->n = *f;
			}
				else ret = 0;
			break;
		}
		case SESET_SFNT:
		{
			struct xa_fnt_info *f = (void*)arg;
			alloc_entry_wtxt(entry, NULL);
			if (entry->iflags & SEF_WTXTALLOC)
			{
				entry->c.td.text.fnt->s = *f;
			}
				else ret = 0;
			break;
		}
		case SESET_HFNT:
		{
			struct xa_fnt_info *f = (void*)arg;
			alloc_entry_wtxt(entry, NULL);
			if (entry->iflags & SEF_WTXTALLOC)
			{
				entry->c.td.text.fnt->h = *f;
			}
				else ret = 0;
			break;
		}
		case SESET_SELECTED:
		{
			if (!(list->flags & SIF_MULTISELECT))
			{
				unselect_all(list, rdrw);
			}
			change_entry(list, entry, true, rdrw);
			break;
		}
		case SESET_UNSELECTED:
		{
			if (arg == UNSELECT_ALL)
				unselect_all(list, rdrw);
			else if (arg == UNSELECT_ONE)
				change_entry(list, entry, false, rdrw);
			break;
		}
		case SESET_MULTISEL:
		{
			if (arg)
				list->flags |= SIF_MULTISELECT;
			else
				list->flags &= ~SIF_MULTISELECT;
			break;
		}
		case SESET_OPEN:
		{
			short xstate = entry->xstate;
			LRECT r;
			RECT s, d;

			if (arg)
			{
				xstate |= OS_OPENED;

				if (xstate != entry->xstate)
				{
					entry->xstate = xstate;
		
					if (entry->down)
					{
						get_entry_lrect(list, entry, 1, &r);
						if ((r.w | r.h))
						{
							list->total_h += (r.h - entry->r.h);
							list->total_w = list->widest = find_widest(list, NULL);
							if (!reset_listwi_widgets(list, rdrw))
							{
								if ((r.y + entry->r.h) < list->start_y)
									list->start_y += (r.h - entry->r.h);
								else if (rdrw && canredraw(list) && (r.y < (list->start_y + list->wi->wa.h)))
								{
									if (canblit(list))
									{
										s = d = list->wi->wa;
										s.y += ((r.y + entry->r.h) - list->start_y);
										d.y += ((r.y + r.h) - list->start_y);
										d.h -= ((r.y + r.h) - list->start_y);
										s.h = d.h;
										hidem();
										if (d.h > 4)
										{
											//display("form copy");
											form_copy(&s, &d);
											s.h = d.y - s.y;
										}
										else
										{
											//display("all redraw");
											s.h += (d.y - s.y);
										}
										draw_slist(0, list, NULL, &s);
										showm();
										clear_clip();
									}
									else
										list->redraw(list, NULL);
								}
							}
						}
						list->slider(list, rdrw);
					}
				}
			}
			else
			{
				xstate &= ~OS_OPENED;
				if (xstate != entry->xstate)
				{
					get_entry_lrect(list, entry, 1, &r);
					entry->xstate = xstate;
					if (entry->down)
					{
						if ((r.w | r.h))
						{
							list->total_h -= (r.h - entry->r.h);
							list->total_w = list->widest = find_widest(list, NULL);
							if (!reset_listwi_widgets(list, rdrw))
							{
								if ((r.y + r.h) < list->start_y)
									list->start_y -= (r.h - entry->r.h);
								else if (rdrw && canredraw(list) && r.y < (list->start_y + list->wi->wa.h))
								{
									if (canblit(list))
									{
										s = d = list->wi->wa;
										s.y += ((r.y + r.h) - list->start_y);
										s.h -= ((r.y + r.h) - list->start_y);
										hidem();
										if (s.h > 4)
										{
											d.y += ((r.y + entry->r.h) - list->start_y);
											d.h = s.h;
											form_copy(&s, &d);
											d.h = s.y - d.y;
											d.y += s.h;
										}
										else
										{
											d.y += ((r.y + entry->r.h) - list->start_y);
											d.h -= ((r.y + entry->r.h) - list->start_y); //(s.y - d.y);
										}
									
										if (d.h > 0)
											draw_slist(0, list, NULL, &d);
									
										showm();
										clear_clip();
									}
									else
										list->redraw(list, NULL);
								}
							}
						}
						list->slider(list, rdrw);
					}
				}
			}
			list->redraw(list, entry);
			break;
		}
		case SESET_TEXTTAB:
		{
			struct seset_txttab *tab = (void *)arg;
			struct se_tab *setabs = list->se_tabs;
			short idx = tab->index;

			while (setabs)
			{
				if (!idx)
					break;
				if (setabs->flags & SETAB_END)
					setabs = NULL;
				setabs++;
				idx--;
			}
			if (setabs)
			{
				setabs->flags &= SETAB_END;
				setabs->flags |= tab->flags & ~SETAB_END;
				setabs->r = tab->r;
				reset_tabs(list);
			}
			else
				ret = 0;
			break;
		}
		case SESET_USRFLAGS:
		{
			if (entry)
				entry->c.usr_flags = arg;
			break;
		}
		case SESET_USRDATA:
		{
			if (entry)
				entry->c.data	= (void *)arg;
			break;
		}
		case SESET_TREEVIEW:
		{
			if (arg)
				list->flags |= SIF_TREEVIEW;
			else
				list->flags &= ~SIF_TREEVIEW;
			if (rdrw)
				list->redraw(list, NULL);
			break;
		}
	}

	if (redrw)
	{
		LRECT r;
		get_entry_lrect(list, entry, 0, &r);
		if (((r.y + r.h) > list->start_y) && (r.y < (list->start_y + list->wi->wa.h)))
			list->redraw(list, entry);
	}
	return ret;
}

static struct se_text *
get_entry_setext(struct scroll_entry *this, short idx)
{
	struct se_text *setext = NULL;

	if (this)
	{
		setext = this->c.td.text.text;
		while (setext && idx--)
		{
			setext = setext->next;
		}
	}	
	return setext;
}

static int
get(SCROLL_INFO *list, SCROLL_ENTRY *entry, short what, void *arg)
{
	int ret = 0;

	if (arg)
	{
		ret = 1;
		
		switch (what)
		{
			case SEGET_STATE:
			{
				*(short *)arg = entry->state;
				break;
			}
			case SEGET_XSTATE:
			{
				*(short *)arg = entry->xstate;
				break;
			}
			case SEGET_NFNT:
			{
				struct xa_fnt_info *f = arg;
				*f = entry->c.td.text.fnt->n;
				break;
			}
			case SEGET_SFNT:
			{
				struct xa_fnt_info *f = arg;
				*f = entry->c.td.text.fnt->s;
				break;
			}
			case SEGET_HFNT:
			{
				struct xa_fnt_info *f = arg;
				*f = entry->c.td.text.fnt->h;
				break;
			}
			case SEGET_WTXT:
			{
				struct xa_wtxt_inf *wtxt = arg;
				*wtxt = *(struct xa_wtxt_inf *)&default_fnt;
				break;
			}
			/*
			 * Return entry at index 'idx', starting at level below 'entry'
			 * If 'entry' is NULL, search start at root-level.
			 * If 'idx' entry is not reached, a NULL is returned as 'e' while
			 * the number of entries are returned in 'idx'
			 */
			case SEGET_ENTRYBYIDX:
			{
				struct seget_entrybyarg *p = arg;
				struct scroll_entry *this = entry;				
				int idx = p->idx;

				if (!this)
					this = list->start;
				else
					this = this->down;
				
				if (this)
				{
					idx--;
					while (idx && this->next)
					{
						this = this->next;
						idx--;
					}
					if (idx)
						this = NULL;
				}
				p->idx -= idx;
				p->e = this;
				break;
			}
			case SEGET_ENTRYBYTEXT:
			{
				struct seget_entrybyarg *p = arg;
				struct se_text *setext;
				struct scroll_entry *this = entry;

				if (!this)
					this = list->start;
				while (this)
				{
					setext = get_entry_setext(this, p->idx);
					if (setext)
					{
						if (!strcmp(p->arg.typ.txt, setext->text))
							break;
					}
					this = next_entry(this, p->arg.flags, p->arg.maxlevel, &p->arg.curlevel);
				}
				if (this)
					p->e = this;
				else
					ret = 0;
				break;
			}
			case SEGET_LISTXYWH:
			{
				check_movement(list);
				*(RECT *)arg = list->wi->wa;
				break;
			}
			case SEGET_TEXTTAB:
			{
				struct seset_txttab *tab = arg;
				struct se_tab *setabs = list->se_tabs;
				short idx = tab->index;

				while (setabs)
				{
					if (!idx)
						break;
					if (setabs->flags & SETAB_END)
						setabs = NULL;
					setabs++;
					idx--;
				}
				if (setabs)
				{
					tab->flags = setabs->flags;
					tab->r = setabs->r;
				}
				else
					ret = 0;
				break;
			}
			case SEGET_SELECTED:
			{
				if (entry)
					*(struct scroll_entry **)arg = get_next_selected(list, entry);
				else
					*(struct scroll_entry **)arg = get_first_selected(list);
				break;
			}
			case SEGET_USRFLAGS:
			{
				if (arg && entry)
					*(long *)arg = entry->c.usr_flags;
				break;
			}
			case SEGET_USRDATA:
			{
				if (arg && entry)
					*(void **)arg = entry->c.data;
				break;
			}
			case SEGET_NEXTENT:
			{
				struct seget_entrybyarg *p = arg;
				
				if (entry)
				{
					p->e = next_entry(entry, p->arg.flags, p->arg.maxlevel, &p->arg.curlevel);
				}
				break;
			}
			case SEGET_PREVENT:
			{
				struct seget_entrybyarg *p = arg;
				
				if (entry)
				{
					if (!p->arg.maxlevel)
						p->e = entry->prev;
					else
						p->e = prev_entry(entry, p->arg.flags);
				}
				break;
			}
			case SEGET_TEXTCPY:
			{
				struct seget_entrybyarg *p = arg;
				struct se_text *setext;

				if ((setext = get_entry_setext(entry, p->idx)))
					strcpy(p->arg.typ.txt, setext->text);
				else
					ret = 0;
				break;
			}
			case SEGET_TEXTCMP:
			{
				struct seget_entrybyarg *p = arg;
				struct se_text *setext;

				if ((setext = get_entry_setext(entry, p->idx)))
					p->ret.ret = strcmp(p->arg.typ.txt, setext->text);
				else
					ret = 0;
				break;
			}
			case SEGET_TEXTPTR:
			{
				struct seget_entrybyarg *p = arg;
				struct se_text *setext;

				if ((setext = get_entry_setext(entry, p->idx)))
					p->ret.ptr = setext->text;
				else
					ret = 0;
				break;
			}
			default:
			{
				ret = 0;
				break;
			}
		}
	}
	return ret;
}

static void
sort_entry(SCROLL_INFO *list, SCROLL_ENTRY **start, SCROLL_ENTRY *new, scrl_compare *greater)
{
	if (*start)
	{
		SCROLL_ENTRY *c = *start, *here;
		
		/* new > c */
		if (greater(new, c))
		{
			here = c;
			c = c->next;
			while (c)
			{
				/* while new < c */
				if (greater(new, c))
				{
					here = c;
					c = c->next;
				}
				else /* new < c */
				{
					/* insert before c */
					break;
				}
			}
		}
		else /* new < c */
		{
			c = c->prev;
			while (c)
			{
				/* while new < c */
				if (greater(c, new))
				{
					c = c->prev;
				}
				else /* new > c */
				{
					/* insert after c */
					break;
				}
			}
			here = c;
		}
		*start = here;
	}
}

/* better control over the content of scroll_entry. */
static int
add_scroll_entry(SCROLL_INFO *list,
		 SCROLL_ENTRY *parent,
		 scrl_compare *sort,
		 struct scroll_content *sc, /*OBJECT *icon, void *text,*/
		 short addmode,
		 SCROLL_ENTRY_TYPE type,
		 short redraw)
{
	struct scroll_entry *here, *new;
	LRECT r;

	if (!parent || is_in_list(list, parent))
	{
		bool fullredraw = false;
		int i;
		short ns = sc->n_strings;
		/*
		 * mem for the new entry...
		 */
		if (type & SETYP_AMAL)
			new = kmalloc(sizeof(*new));
		else
			new = kmalloc(sizeof(*new));
	
		if (!new)
			return 0;

		bzero(new, sizeof(*new));

		{
			char *s = sc->text;
			struct se_text *setext, **prev;

			prev = &new->c.td.text.text;
			new->c.type = SECONTENT_TEXT;

			for (i = 0; i < ns; i++)
			{
				if (type & SETYP_AMAL)
				{
					setext = kmalloc(sizeof(*setext) + strlen(s) + 1);
					if (setext)
					{
						setext->flags = SETEXT_TXTSTR;
						setext->text = setext->txtstr;
						strcpy(setext->text, s);
						*prev = setext;
						prev = &((*prev)->next);
						*prev = NULL;
					}
				}
				else
				{
					setext = kmalloc(sizeof(*setext));
					if (setext)
					{
						setext->flags = 0;
						setext->text = s;
						*prev = setext;
						prev = &((*prev)->next);
						*prev = NULL;
						if (type & SETYP_MAL)
							setext->flags |= SETEXT_ALLOC;
					}
				}
				setext->tblen = strlen(setext->text);
				s += strlen(s) + 1;
			}
		}

		new->c.td.text.n_strings = sc->n_strings;
		
		new->c.icon = sc->icon;
		new->c.data = sc->data;
		new->c.usr_flags = sc->usr_flags;

		new->xstate = sc->xstate;
		new->xflags = sc->xflags;

		if (sc->fnt)
			alloc_entry_wtxt(new, sc->fnt);
		else
			new->c.td.text.fnt = &default_fnt;
		
		if (!(new->c.col = sc->col))
			new->c.col = &default_col;

		new->type = type;

		if (new->c.icon)
		{
			object_spec_wh(new->c.icon, &new->r.w, &new->r.h);
			new->c.icon->ob_x = new->c.icon->ob_y = 0;
			new->r.w += 2, new->r.h += 2;
			new->icon_w = new->r.w;
			new->icon_h = new->r.h;
			if (new->r.w > list->icon_w)
				list->icon_w = new->r.w;
			if (new->r.h > list->icon_h)
				list->icon_h = new->r.h;
		}

		if (new->c.td.text.text)
			fullredraw = entry_text_wh(list, new);

		new->indent = list->indent;
		new->r.w += new->indent;
		
		if (parent)
		{
			if ((addmode & SEADD_CHILD))
				here = parent->down;
			else
				here = parent;
		}
		else
			here = list->start;

		if (!here)
		{
			if ((addmode & SEADD_CHILD))
			{
				parent->down = new;
				new->up = parent;
			}
			else
				list->start = list->top = new;	
		}
		else
		{
			if (sort)
			{
				sort_entry(list, &here, new, sort);
				if (!here)
				{
					if (parent)
						here = parent->down;
					else
						here = list->start;
					addmode |= SEADD_PRIOR;
				}
				else
					addmode &= ~SEADD_PRIOR;
			}
			else if (parent)
			{
				/*
				 * If we have a parent but 'here' is not equal to it,
				 * 'here' points to 'parent's first child - we now need
				 * to find the last entry at that level and append 'new'
				 * there.
				 */
				if (here != parent && !(addmode & SEADD_PRIOR))
				{
					while (here->next)
						here = here->next;
				}
				/* else if here == parent means that 'here' points to the entry after
				 * which we will insert the 'new' entry.
				 */
			}
			else if (!(addmode & SEADD_PRIOR))
			{
				while (here->next)
					here = here->next;
			}

			new->up = here->up;
			if (addmode & SEADD_PRIOR)
			{
				new->next = here;
				if (!(new->prev = here->prev))
				{
					if (here->up)
						here->up->down = new;
					else
						list->start = new;
				}
				else
					new->prev->next = new;
				
				here->prev = new;
			}
			else
			{
				if (here->next)
					here->next->prev = new;
				new->next = here->next;
				here->next = new;
				new->prev = here;
			}
		}
		
		if (new->up)
		{
			new->level = new->up->level + 1;
			new->indent += (new->level * list->nesticn_w);
			if (list->flags & SIF_TREEVIEW)
			{
				if (new->r.h < list->nesticn_h)
					new->r.h = list->nesticn_h;
				if (new->r.w < list->nesticn_w)
					new->r.w = list->nesticn_w;
			}
			//new->r.w += (new->level * 16);
		}

		get_entry_lrect(list, new, 0, &r);
			
		if ((r.h | r.w))
		{

			if (new->r.w > list->widest)
				list->widest = new->r.w;
			if (new->r.h > list->highest)
				list->highest = new->r.h;
			list->total_w = list->widest;
			list->total_h += new->r.h;

			
			if (r.y < (list->start_y - list->off_y))
			{
				list->start_y += r.h;
			}
			else if (r.y <  (list->start_y + list->wi->wa.h))
			{
				if (r.y == (list->start_y - list->off_y))
				{
					if (list->off_y)
						list->start_y += r.h;
					else
						list->top = new;
				}
				if (!reset_listwi_widgets(list, redraw/*true*/) && redraw)
				{
					if (canredraw(list))
					{
						if (!canblit(list) || fullredraw)
						{
							list->redraw(list, NULL);
							fullredraw = false;
						}
						else
						{
							short sy = (r.y - list->start_y);
							if (sy >= 0)
							{
								struct scroll_entry *next;
								hidem();
								if ((next = next_entry(new, ENT_VISIBLE, -1, NULL)))
								{
									RECT d, s = list->wi->wa;
							
									s.y += sy;
									s.h -= (sy + r.h);
									d = s;
									d.y += r.h;
									if (d.h > 0)
										form_copy(&s, &d);
								}
								list->redraw(list, new);
								showm();
							}
						}
					}
					else
						redraw = 0;
				}
			}
			if (fullredraw)
			{
				if (!reset_listwi_widgets(list, redraw) && canredraw(list) && redraw)
					list->redraw(list, NULL);
			}
			list->slider(list, redraw);
		}
		return 1;
	}
	return 0;
}

/*
 * Delete scroll entry
 */
static SCROLL_ENTRY *
free_scroll_entry(struct scroll_info *list, struct scroll_entry *this, long *height)
{
	struct scroll_entry *stop = this, *next;
	short level = 1;
	long h = 0;

	this = this->down;

	while (this && level) //this != stop)
	{
		if (this->down)
		{
			this = this->down;
			level++;
		}
		else
		{
			next = this->next;
			if ((this->type & SETYP_STATIC))
			{
				if (!next)
				{
					next = this->up;
					level--;
					while (next && level)
					{
						if (next->next)
						{
							next = next->next;
							break;
						}
						next = next->up;
						level--;
					}
				}
			}
			else
			{
				if (next)
					next->prev = this->prev;
				if (this->prev)
					this->prev->next = next;
				else
				{
					if (this->up)
						this->up->down = next;
					else
						list->start = next;
				}
				if (!next)
				{
					next = this->up;
					level--;
				}

				if (this == list->cur)
				{
					list->cur = NULL;
				}
				if (this == list->top)
				{
					list->top = NULL;
				}

				if (this->c.type == SECONTENT_TEXT)
				{
					struct se_text *setext = this->c.td.text.text, *n;
					while (setext)
					{
						n = setext->next;
						if (setext->flags & SETEXT_ALLOC)
							kfree(setext->text);
						kfree(setext);
						setext = n;
					}
					if ((this->iflags & SEF_WTXTALLOC))
						kfree(this->c.td.text.fnt);
				}
				h += this->r.h;
				kfree(this);
			
			}			
			this = next;
		}
	}

	this = stop;
	next = this->next;
	if (!(this->type & SETYP_STATIC))
	{
		if (next)
			next->prev = this->prev;
		if (this->prev)
			this->prev->next = next;
		else
		{
			if (this->up)
				this->up->down = next;
			else
				list->start = next;
		}
	}
	if (!next)
	{
		next = this->up;
		level--;
		while (next)
		{
			if (next->next)
			{
				next = next->next;
				break;
			}
			next = next->up; //this = this->up;
			level--;
		}
	//	if (next)
	//		next = next->next;
	}

	if (!(this->type & SETYP_STATIC))
	{
		if (this == list->cur)
			list->cur = NULL;
		if (this == list->top)
			list->top = NULL;

		if (this->c.type == SECONTENT_TEXT)
		{
			struct se_text *setext = this->c.td.text.text, *n;
			while (setext)
			{
				n = setext->next;
				if (setext->flags & SETEXT_ALLOC)
					kfree(setext->text);
				kfree(setext);
				setext = n;
			}
			if ((this->iflags & SEF_WTXTALLOC))
				kfree(this->c.td.text.fnt);
		}
		h += this->r.h;
		kfree(this);
	}
	if (height)
		*height = h;

	return next;
}

static struct scroll_entry *
del_scroll_entry(struct scroll_info *list, struct scroll_entry *e, short redraw)
{

	struct scroll_entry *next, *prev;
	short sy = 0, dy = 0;
	long del_h;
	LRECT lr, r;

	get_entry_lrect(list, e, 1, &lr);
	
	if (!(prev = e->prev))
		prev = e->up;
	
	next = free_scroll_entry(list, e, &del_h);

	if (lr.y < (list->start_y - list->off_y))
	{
		list->total_h -= lr.h;
		
		if (list->total_h <= list->wi->wa.h)
		{
			list->top = list->start;

			if (!list->start_y && !list->off_y)
			{
				sy = lr.y + lr.h;
				dy = lr.y;
			}
			else
			{
				list->start_y = list->off_y = 0;
				sy = -1L;
			}
		}
		else
		{
			if ((lr.y + lr.h) < (list->start_y - list->off_y))
			{
				list->start_y -= lr.h;
			}
			else
			{
				if (next)
				{
					get_entry_lrect(list, next, 0, &r);
					list->top = next;
					sy = (lr.h + r.y) - list->start_y;
					if (sy > list->start_y + list->wi->wa.h)
						sy = -1L;
					dy = 0;
					list->start_y = r.y;
					list->off_y = 0;
				}
				else if (prev)
				{
					get_entry_lrect(list, prev, 0, &r);
					list->top = prev;
					list->start_y = r.y;
					list->off_y = 0;
					sy = -1L;
				}
				/* else the list is empty */
			}
		}
	}
	else //if (lr.y >= list->start_y)
	{
		list->total_h -= lr.h;
		if (list->total_h <= list->wi->wa.h)
		{
			list->top = list->start;
			if (!list->start_y && !list->off_y)
			{
				sy = lr.y + lr.h;
				dy = lr.y;
			}
			else
			{
				list->start_y = list->off_y = 0;
				sy = -1L;
			}
		}
		else
		{
			if (lr.y == (list->start_y - list->off_y))
			{
				if (next)
				{
					list->top = next;
					get_entry_lrect(list, next, 0, &r);
					sy = (r.y + lr.h) - list->start_y;
					if (sy > list->wi->wa.h)
						sy = -1L;
					dy = 0;
					list->start_y = r.y;
					list->off_y = 0;
				}
				else if (prev)
				{
					list->top = prev;
					get_entry_lrect(list, prev, 0, &r);
					list->start_y = r.y;
					list->off_y = 0;
					sy = -1L;
				}
				/* else list is empty */
			}
			else
			{
				if (next)
				{
					get_entry_lrect(list, next, 0, &r);
					sy = (r.y + lr.h) - list->start_y;
					if (sy > list->wi->wa.h)
						sy = -1l;
					dy = lr.y - list->start_y;
				}
			}
		}
	}
	if (lr.w == list->widest)
	{
		list->widest = list->total_w = find_widest(list, NULL);
	}

	if (!reset_listwi_widgets(list, redraw/*true*/) && redraw && sy)
	{
		short h;
		RECT s, d;

		hidem();
		if (!canblit(list) || sy == -1L)
			list->redraw(list, NULL);
		else
		{			
			s = list->wi->wa;
			h = s.h - sy;
			if (h < 8)
			{
				s.y += dy;
				s.h -= s.h - dy;
				draw_slist(0, list, NULL, &s);
			}
			else
			{
				s.h = h;
				d = s;
				s.y += sy;
				d.y += dy;
				form_copy(&s, &d);
				d.y += (d.h - 1);
				d.h = list->wi->wa.h - (d.y - list->wi->wa.y) + 1;
				draw_slist(0, list, NULL, &d);
			}
			clear_clip();
		}
		showm();
	}

	list->slider(list, redraw);
	return next;
}

/* Modified such that a choise can be made. */
static void
empty_scroll_list(SCROLL_INFO *list, SCROLL_ENTRY *this, SCROLL_ENTRY_TYPE type)
{
	//short level = 0;
	if (!this)
	{
		this = list->start;
		while (this)
		{
			if (type == -2)
				this->type &= ~SETYP_STATIC;

			this = del_scroll_entry(list, this, false);
#if 0
			if (type == -2 || ((this->type & type) || type == -1))
				this = del_scroll_entry(list, this, false);
			else
				this = next_entry(this, ENT_ISROOT, -1, &level);
#endif
		}
	}
	else
	{
		SCROLL_ENTRY *next = this->next;
		this = this->down;
		while (this && this != next)
			this = del_scroll_entry(list, this, false);
	}
}

static void
scroll_up(SCROLL_INFO *list, long num, bool rdrw)
{
	long n, max, h;
	struct scroll_entry *this, *next;

	DIAG((D_fnts, NULL, "scroll_up: %d lines, top = %lx",
		num, list->top));

	n = list->total_h - (list->start_y + list->wi->wa.h);
	
	if (n <= 0)
		return;
	
	max = n > num ? num : n;
	n = max;

	this = list->top;
		
	while (n > 0 && (next = next_entry(this, ENT_VISIBLE, -1, NULL))) //this->next)
	{
		h = this->r.h;
		if (list->off_y)
			h -= list->off_y;

		n -= h;

		if (n < 0)
		{
			list->off_y = n + this->r.h;
			list->start_y += (n + h); //num;
		}
		else
		{
			list->off_y = 0;
			list->start_y += h;
			this = next; //this->next;
		}
	}
	list->top = this;

	if (rdrw)
	{
		if (!canblit(list) || max > (list->wi->wa.h - 8))
			list->redraw(list, NULL);
		else
		{
		/*
		 * XXX - Ozk: This assumes there's nothing covering our slist window"
		 */
			RECT s, d;

			s = list->wi->wa;
			s.h -= max;
			d = s;
			s.y += max;
			hidem();
			form_copy(&s, &d);
			d.y += (d.h - 1);
			d.h = max + 1;
			draw_slist(0, list, NULL, &d);
			clear_clip();
			showm();
		}
		list->slider(list, true);
	}
}
static void
scroll_down(SCROLL_INFO *list, long num, bool rdrw)
{
	SCROLL_ENTRY *prev;
	long max = num, n, h;

	DIAG((D_fnts, NULL, "scroll_down: %d lines, top = %lx",
		num, list->top));

	if (max > list->start_y)
		max = list->start_y;
	
	n = max;

	if (list->top)
	{
		while (n > 0 && ((prev = prev_entry(list->top, ENT_VISIBLE))/*list->top->prev*/ || list->off_y))
		{
			if (list->off_y)
			{
				h = list->off_y;
				n -= h;
				if (n < 0)
				{
					list->off_y -= (n + h);
					list->start_y -= (n + h);
				}
				else
				{
					list->off_y = 0;
					list->start_y -= h;
				}
			}
			else
			{
				list->top = prev; //list->top->prev;
				h = (list->top->r.h - list->off_y);

				n -= h;
				if (n < 0)
				{
					list->off_y = list->top->r.h - (n + h);
					list->start_y -= (n + h);
				}
				else
				{
					list->off_y = 0;
					list->start_y -= h;
				}
			}
		}
	}
	if (rdrw)
	{
		if (!canblit(list) || max > (list->wi->wa.h - 8))
			list->redraw(list, NULL);
		else
		{
			RECT s, d;

			s = list->wi->wa;

			s.h -= max;
			d = s;
			d.y += max;
			hidem();
			form_copy(&s, &d);
			s.h = max;
			draw_slist(0, list, NULL, &s);
			clear_clip();
			showm();
		}
		list->slider(list, true);
	}
}

static void
scroll_left(SCROLL_INFO *list, long num, bool rdrw)
{
	long n, max;

	if (list->total_w < list->wi->wa.w)
		return;

	if ((num + list->start_x + list->wi->wa.w) > list->total_w)
	{
		max = n = list->total_w - (list->start_x + list->wi->wa.w);
		if (max < 0)
			return;
	}
	else
		max = n = num;

	list->start_x += n;

	if (rdrw)
	{
		if (!canblit(list) || max > list->wi->wa.w - 8)
			list->redraw(list, NULL);
		else
		{
			RECT s, d;

			s = list->wi->wa;
			s.w -= max;
			d = s;
			s.x += max;
			hidem();
			form_copy(&s, &d);
			d.x += d.w;
			d.w = max;
			draw_slist(0, list, NULL, &d);
			clear_clip();
			showm();
		}
		list->slider(list, true);
	}
}
static void
scroll_right(SCROLL_INFO *list, long num, bool rdrw)
{
	long n, max;

	if (list->total_w < list->wi->wa.w)
		return;

	if (num > list->start_x)
		max = n = list->start_x;
	else
		max = n = num;

	list->start_x -= n;

	if (rdrw)
	{
		if (!canblit(list) || max > list->wi->wa.w - 8)
			list->redraw(list, NULL);
		else
		{
			RECT s, d;

			s = list->wi->wa;

			s.w -= max;
			d = s;
			d.x += max;
			hidem();
			form_copy(&s, &d);
			s.w = max;
			draw_slist(0, list, NULL, &s);
			clear_clip();
			showm();
		}
		list->slider(list, true);
	}
}
	
		
static void
visible(SCROLL_INFO *list, SCROLL_ENTRY *s, short redraw)
{
	LRECT r;

	DIAGS(("scrl_visible: start_y=%ld, height=%d, total =%ld",
		list->start_y, list->wi->wa.h, list->total_h));

	if (get_entry_lrect(list, s, 0, &r))
	{
		bool rdrw = false;
		if ((r.w | r.h))
		{
			check_movement(list);
		
			if (redraw == NORMREDRAW)
				rdrw = true;
				
			if ((r.y + r.h) <= list->start_y || r.y > (list->start_y + list->wi->wa.h))
			{
				/*
				 * If the entry to make visible is the next-up, we only scroll enough
				 * to make that visisble, i.e., do not scroll so that it is placed
				 * in at the center.
				 */
				if ((r.y + r.h) == list->start_y)
				{
					scroll_down(list, r.h, rdrw);
				}
				else
				{
					/*
					 * We want to place the entry as  close as possible to the center
					 * of the list (vertical center, that is)
					 */
					r.y -= list->start_y;
					if (r.y < 0)
						scroll_down(list, -r.y + (list->wi->wa.h >> 1), rdrw);
					else
					{
						r.y -= list->wi->wa.h;
						scroll_up(list, r.y + (list->wi->wa.h >> 1), rdrw);
					}
				}
				rdrw = false;
			}
			else if (r.y < list->start_y)
			{
				/*
				 * The entry is partially visible, scroll just enough to make it the first
				 * visible in the list
				 */
				scroll_down(list, list->off_y, rdrw);
			}
			else if ((r.y + r.h) > (list->start_y + list->wi->wa.h))
			{
				/*
				 * The entry is partially visible at the end of the list, scroll just
				 * enough to make it visible as the last line in the list
				 */
				scroll_up(list, (r.y + r.h) - (list->start_y + list->wi->wa.h), rdrw);
			}
			if (rdrw)
				list->redraw(list, s);
			
			if (redraw == FULLREDRAW)
				list->redraw(list, NULL);
		
			list->slider(list, redraw);
		}
	}	
}

static SCROLL_ENTRY *
search(SCROLL_INFO *list, SCROLL_ENTRY *start, short mode, void *data)
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
				if (start->c.data == data)
					break;
				start = next_entry(start, 0, -1, NULL);
			}
			ret = start;
			break;
		}
		case SEFM_BYTEXT:
		{
			while (start)
			{
				if (!strcmp(start->c.td.text.text->text, data))
					break;
				start = next_entry(start, 0, -1, NULL);
			}
			ret = start;
			break;
		}
		case SEFM_LAST:
		{
			ret = get_last_entry(list);
			break;
		}
	}
	return ret;
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
	check_movement(list);
	if (!do_widgets(lock, list->wi, 0, md))
	{
		long y;

		cy -= list->wi->wa.y;

		if ((this = list->top))
		{
			y = list->top->r.h - list->off_y;
		
			while (this && y < cy)
			{
				this = next_entry(this, ENT_VISIBLE, -1, NULL);
				if (this) y += this->r.h;
			}
		}
		
		if (this)
		{
			short ni_x = (list->wi->wa.x + this->indent) - list->off_x;
			LRECT r;

			if ((list->flags & SIF_TREEVIEW) && md->x > ni_x && md->x < (ni_x + list->nesticn_w))
			{
				if ((this->xflags & OF_AUTO_OPEN))
				{
					if (this->xstate & OS_OPENED)
						list->set(list, this, SESET_OPEN, 0, NORMREDRAW);
					else
						list->set(list, this, SESET_OPEN, 1, NORMREDRAW);
				}
				if (list->click_nesticon)
					list->click_nesticon(list, this, md);
			}
			else
			{
				list->cur = this;
				if (list->flags & SIF_AUTOSELECT)
				{
					if (!(list->flags & SIF_MULTISELECT))
						list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
					list->set(list, this, SESET_SELECTED, 0, NOREDRAW); //NORMREDRAW);
					get_entry_lrect(list, this, 0, &r);
					list->vis(list, this, NORMREDRAW);
				}
				if (list->click)			/* Call the new object selected function */
					(*list->click)(list, this, md);
			}
		}
		else
			list->click(list, this, md);
	}
}

void
dclick_scroll_list(enum locks lock, OBJECT *form, int item, const struct moose_data *md)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	OBJECT *ob = form + item;
	long y;
	short cx = md->x, cy = md->y;	

	list = (SCROLL_INFO *)object_get_spec(ob)->index;
	check_movement(list);
	if (!do_widgets(lock, list->wi, 0, md))		/* HR 161101: mask */
	{
		if (!m_inside(cx, cy, &list->wi->wa))
			return;
	
		cy -= list->wi->wa.y;
	
		if ((this = list->top))
		{
			y = list->top->r.h - list->off_y;
			while(this && y < cy)
			{
				this = next_entry(this, ENT_VISIBLE, -1, NULL);
				if (this) y += this->r.h;
			}
		}
	
		if (this)
			list->cur = this;
		
		if (list->dclick)
			(*list->dclick)(list, this, md);
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

static void
unset_G_SLIST(struct scroll_info *list)
{
	struct scroll_entry *this;
	OBJECT *ob = list->wt->tree + list->item;

	DIAG((D_objc, NULL, "unset_G_SLIST: list=%lx, obtree=%lx, index=%d",
		list, list->wt->tree, list->item));

	this = list->start;
	while (this)
	{
		this->type &= ~SETYP_STATIC;
		this = free_scroll_entry(list, this, NULL);
	}
	//list->empty(list, -2);

	if (list->wi)
	{
		close_window(0, list->wi);
		delete_window(0, list->wi);
		list->wi = NULL;
	}

	if (list->wt)
	{
		list->wt = NULL;
	}

	if (list->se_tabs)
		kfree(list->se_tabs);

	*ob = list->prev_ob;
	if (list->flags & SIF_KMALLOC)
		kfree(list);
	else
		ufree(list);
}

/* preparations for windowed list box widget;
 * most important is to get the drawing smooth and simple.
 * get rid of all those small (confolded) constant value's.
 */
SCROLL_INFO *
set_slist_object(enum locks lock,
		 XA_TREE *wt,
		 struct xa_window *parentwind,
		 short item,
		 short flags,

		 scrl_widget *closer,
		 scrl_widget *fuller,
		 scrl_click *dclick,
		 scrl_click *click,
		 scrl_click *click_nesticon,

		 scrl_add	*add,
		 scrl_del	*del,
		 scrl_empty	*empty,
		 scrl_list	*destroy,

		 char *title,
		 char *info,
		 void *data,
		 short lmax)	/* Used to determine whether a horizontal slider is needed. */
{
	struct scroll_info *list;
	RECT r;
	XA_WIND_ATTR wkind = UPARROW|VSLIDE|DNARROW;
	OBJECT *ob = wt->tree + item;

	DIAG((D_rsrc, NULL, "set_slist_object"));

	list = kmalloc(sizeof(*list));

	if (!list)
		return NULL;

	bzero(list, sizeof(*list));

	{
		int i = 1;
		long len;
		struct se_tab *setabs = se_tabs;

		while (!(setabs->flags & SETAB_END))
			i++, setabs++;
		
		len = sizeof(struct se_tab) + sizeof(struct se_text_tabulator);
		
		if (!(list->se_tabs = kmalloc(len * i) ))
		{
			kfree(list);
			return NULL;
		}
		(long)list->tabs = (long)list->se_tabs + (sizeof(struct se_tab) * i);
		memcpy(list->se_tabs, &se_tabs, (sizeof(struct se_tab) * i));
		reset_tabs(list);
	}

	list->flags |= SIF_KMALLOC;

	/* colours are those for windows */
	list->prev_ob = *ob;

	object_set_spec(wt->tree + item, (unsigned long)list);
	ob->ob_type = G_SLIST;
	ob->ob_flags |= OF_TOUCHEXIT;
	
	if (flags & SIF_SELECTABLE)
		ob->ob_flags |= OF_SELECTABLE;
	else
		ob->ob_flags &= ~OF_SELECTABLE;

	list->flags |= (flags & ~SIF_KMALLOC);

	//list->tree = wt->tree;
	list->item = item;

	list->pw = parentwind;
	
	list->wt = wt;

	list->data = data;

	list->dclick = dclick;			/* Pass the scroll list's double click function */
	list->click = click;			/* Pass the scroll list's click function */
	list->click_nesticon = click_nesticon;

	list->title = title;
	obj_area(wt, item, &r);
	list->rel_x = r.x - wt->tree->ob_x;
	list->rel_y = r.y - wt->tree->ob_y;

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
				 0, false, r, NULL, NULL);
	if (list->wi)
	{
		int dh;

		if (title)
			set_window_title(list->wi, title, false);
		if (info)
			set_window_info(list->wi, info, false);
		
		get_widget(list->wi, XAW_VSLIDE)->drag = drag_vslide;
		get_widget(list->wi, XAW_HSLIDE)->drag = drag_hslide;

		list->wi->winob	= wt->tree;		/* The parent object of the windowed list box */
		list->wi->winitem = item;
		r = list->wi->wa;
		dh = list->wi->wa.h - r.h;
		ob->ob_height -= dh;
		list->wi->r.h -= dh;

		list->slider	= sliders;
		list->closer	= closer;

		if (add)	list->add	= add;
		else		list->add	= add_scroll_entry;
		if (del)	list->del	= del;
		else		list->del	= del_scroll_entry;
		if (empty)	list->empty	= empty;
		else		list->empty	= empty_scroll_list;
		if (destroy)	list->destroy	= destroy;
		else		list->destroy	= unset_G_SLIST;

		list->fuller	= fuller;
		list->vis	= visible;
		list->search	= search;
		list->set	= set;
		list->get	= get;
		list->redraw	= slist_redraw;
		list->lock	= lock;

		list->indent = 2;
		list->nesticn_w = 16;
		list->nesticn_h = 16;

		open_window(lock, list->wi, list->wi->r);
	}
	DIAGS((" -- return new slist %lx", list));
	return list;
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
	//enum locks lock = 0;
	SCROLL_INFO *list;
	SCROLL_ENTRY *top, *n;
	OBJECT *ob;
	long p, oldp;
	short amount, msgt;

	ob = wind->winob + wind->winitem;
	list = object_get_slist(ob); //(SCROLL_INFO *)object_get_spec(ob)->index;
	top = list->top;
	oldp = list->start_y;
	check_movement(list);
	//display("msg...");	
	switch (msg[0])		/* message number */
	{
	case WM_ARROWED:
	{
		if (top)
		{
			if ((msgt = msg[4] & 7) < WA_LFPAGE)
			{
				p = list->wi->wa.h;
				if (!(amount = (msg[4] >> 8) & 0xff))
					amount++;

				if (p < list->total_h)
				{
					switch (msgt)
					{
					case WA_UPLINE:
					{
						if ((n = prev_entry(top, ENT_VISIBLE)) || list->off_y)
						{
							if (list->off_y)
								scroll_down(list, list->off_y, true);
							else
							{
								long h = n->r.h;
								amount--;
								while (amount)
								{
									if (!(n = prev_entry(n, ENT_VISIBLE)))
										break;
									h += n->r.h;
									amount--;
								}
								scroll_down(list, h, true);
							}
						}
						break;
					}
					case WA_DNLINE:
					{
						if ((n = next_entry(top, ENT_VISIBLE, -1, NULL)))
						{
							if (list->off_y)
								scroll_up(list, top->r.h - list->off_y, true);
							else if (n)
							{
								long h = n->r.h;
								amount--;
								while (amount)
								{
									if (!(n = next_entry(n, ENT_VISIBLE, -1, NULL)))
										break;
									h += n->r.h;
									amount--;
								}
								scroll_up(list, h, true);
							}
						}
						break;
					}
					case WA_UPPAGE:
					{
						scroll_down(list, list->wi->wa.h - top->r.h, true);
						break;
					}
					case WA_DNPAGE:
					{
						scroll_up(list, list->wi->wa.h - top->r.h, true);
						break;
					}
					}
				}
			}
			else
			{
				{
					switch (msg[4])
					{
						case WA_LFLINE:
							scroll_right(list, 4, true);
							break;
						case WA_RTLINE:
							scroll_left(list, 4, true);
							break;
						case WA_LFPAGE:
							scroll_right(list, list->wi->wa.w - 8, true);
							break;
						case WA_RTPAGE:
							scroll_left(list, list->wi->wa.w - 8, true);
					}
				}
			}
		}
		break;
	}
	case WM_VSLID:
	{
		if (top)
		{
			p = list->wi->wa.h;
			if (p < list->total_h)
			{
				long new = sl_to_pix(list->total_h - p, msg[4]); //(list->n - (p-1), msg[4]);

				//new -= list->top->n;
				new -= list->start_y;
				if (new < 0)
					scroll_down(list, -new, true);
				else
					scroll_up(list, new, true);
			}
		}
		break;
	}
	case WM_HSLID:
	{
		if (top)
		{
			p = list->wi->wa.w;
			if (p < list->total_w)
			{
				long new = sl_to_pix(list->total_w - p, msg[4]);
				new -= list->start_x;
				if (new < 0)
					scroll_right(list, -new, true);
				else
					scroll_left(list, new, true);
			}
		}
		break;
	}
	case WM_CLOSED:
	{
		if (list->closer)
			list->closer(list, true);
		break;
	}
	case WM_FULLED:
	{
		if (list->fuller)
			list->fuller(list, true);
	}
	default:
	{
		return;
	}
	}
}

int
scrl_cursor(SCROLL_INFO *list, ushort keycode)
{
	switch (keycode)
	{
	case 0x4800:			/* up arrow */
	{
		if (!list->cur)
		{
			short msg[8] = { WM_ARROWED,0,0,list->wi->handle,WA_UPLINE,0,0,0 };
			slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		}
		else
		{
			SCROLL_ENTRY *n = prev_entry(list->cur, ENT_VISIBLE);
			if (n)
			{
				list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
				list->cur = n;
				list->set(list, n, SESET_SELECTED, 0, NOREDRAW);
				list->vis(list, n, NORMREDRAW);
			}
		}
		break;
	}
	case 0x5000:			/* down arrow */
	{
		if (!list->cur)
		{
			short msg[8] = { WM_ARROWED,0,0,list->wi->handle,WA_DNLINE,0,0,0 };
			slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		}
		else
		{
			SCROLL_ENTRY *n = next_entry(list->cur, ENT_VISIBLE, -1, NULL);
			if (n)
			{
				list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
				list->cur = n;
				list->set(list, n, SESET_SELECTED, 0, NOREDRAW);
				list->vis(list, n, NORMREDRAW);
			}
		}			
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
		scroll_down(list, list->start_y, true);
		break;
	}
	case 0x4737:			/* shift + home */
	{
		long n = list->total_h - (list->start_y + list->wi->wa.h);
		if (n > 0)
			scroll_up(list, n, true);
		break;
	}
	default:
	{
		return -1;
	}
	}
	return keycode;
}
