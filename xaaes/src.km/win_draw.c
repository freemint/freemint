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

#include "xa_global.h"

#include "xa_types.h"
#include "draw_obj.h"
#include "obtree.h"
#include "xaaeswdg.h"
#include "win_draw.h"
/*
 * Draw a window widget
 */
static inline long
pix_2_sl(long p, long s)
{
	/*
	 * Ozk:
	 *      Hmmm.... divide by Zero not handled correctly, at least
	 *	not on my Milan 040 where this crashed if 's' is 0
	 */
	if (s)
		return (p * SL_RANGE) / s;
	else
		return 0L;
}

static inline long
sl_2_pix(long s, long p)
{
	return ((s * p) + (SL_RANGE >> 2)) / SL_RANGE;
}

static void
draw_widg_box(short d, struct xa_wcol_inf *wcoli, short state, RECT *wr, RECT *anch)
{
	struct xa_wcol *wcol;
	struct xa_wtexture *wext;
	bool sel = state & OS_SELECTED;
	short f = wcoli->flags, o = 0;
	RECT r = *wr;

	wr_mode(wcoli->wrm);
	
	if (sel)
		wcol = &wcoli->s;
	else
		wcol = &wcoli->n;

	wext = wcol->texture;

	if (d)
	{
		r.x -= d;
		r.y -= d;
		r.w += d+d;
		r.h += d+d;
	}

	if (f & WCOL_BOXED)
	{
		int i = wcol->box_th;
		l_color(wcol->box_c);
		while (i > 0)
			gbox(o, &r), o--, i--;
	}
		
	if (f & WCOL_DRAW3D)
	{
		if (sel)
		{
			tl_hook(o, &r, wcol->tlc);
			br_hook(o, &r, wcol->brc);
		}
		else
		{
			br_hook(o, &r, wcol->brc);
			tl_hook(o, &r, wcol->tlc);
		}
		o -= 1;
	}

	if (f & WCOL_DRAWBKG)
	{
		f_interior(wcol->i);
		if (wcol->i > 1)
			f_style(wcol->f);
	
		f_color(wcol->c);
		gbar(o, &r);
	}
	
	if (wext)
	{
		short pnt[8];
		short x, y, w, h, sy, sh, dy, dh, width, height;
		MFDB mscreen, *msrc;

		mscreen.fd_addr = NULL;
		msrc = wext->mfdb;

		r.x -= o;
		r.y -= o;
		r.w += o + o;
		r.h += o + o;
		
		x = (long)(r.x - anch->x) % msrc->fd_w;
		w = msrc->fd_w - x;
		sy = (long)(r.y - anch->y) % msrc->fd_h;
		sh = msrc->fd_h - sy;
		dy = r.y;
		dh = r.h;
		
		width = r.w;
		while (width > 0)
		{
			pnt[0] = x;
			pnt[4] = r.x;
			
			width -= w;
			if (width <= 0)
				w += width;
			else
			{
				r.x += w;
				x = 0;
			}
			pnt[2] = pnt[0] + w - 1;
			pnt[6] = pnt[4] + w - 1;
			
			y = sy;
			h = sh;
			r.y = dy;
			r.h = dh;
			height = r.h;
			while (height > 0)
			{
				//pnt[0] = x;
				pnt[1] = y;
				//pnt[4] = r.x;
				pnt[5] = r.y;
				
				height -= h;
				if (height <= 0)
					h += height;
				else
				{
					r.y += h;
					y = 0;
				}
				
				//pnt[2] = x + w - 1;
				pnt[3] = pnt[1] + h - 1;
				
				//pnt[6] = r.x + w - 1;
				pnt[7] = pnt[5] + h - 1;

				vro_cpyfm(C.vh, S_ONLY, pnt, msrc, &mscreen);
				
				h = msrc->fd_h;	
			}
			w = msrc->fd_w;
		}
	}	
}

static void
draw_widget_text(struct xa_widget *widg, struct xa_wtxt_inf *wtxti, char *txt, short xoff, short yoff)
{
	wtxt_output(wtxti, txt, widg->state, &widg->ar, xoff, yoff);
}

static void
draw_widg_icon(struct xa_widget *widg, XA_TREE *wt, short ind)
{
	short x, y, w, h;
	OBJECT *ob = wt->tree + ind;

	if (widg->state & OS_SELECTED)
		ob->ob_state |= OS_SELECTED;
	else
		ob->ob_state &= ~OS_SELECTED;

	x = widg->ar.x, y = widg->ar.y;

	object_spec_wh(ob, &w, &h);
	x += (widg->ar.w - w) >> 1;
	y += (widg->ar.h - h) >> 1;
	display_object(0, wt, (const RECT *)&C.global_clip, ind, x, y, 0);
}

static bool
d_unused(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wc = &wind->colours->win;
	(*wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
	//rp_2_ap(wind, widg, &widg->ar);
	draw_widg_box(0, wc, 0, &widg->ar, &wind->r);
	return true;
}

static bool
d_border(struct xa_window *wind, struct xa_widget *widg)
{
	/* Dummy; no draw, no selection status. */
	return true;
}

static bool
draw_window_borders(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->borders;
	short size = wind->frame;
	RECT r;
	
	if (wind->frame > 0)
	{
		if (wind->frame >= 4)
		{
			/* top-left box */
			r.x = wind->r.x;
			r.y = wind->r.y;
			r.w = size;
			r.h = size;
			draw_widg_box(0, wci, 0, &r, &wind->r);

			/* Left border */
			r.y += size;
			r.h = wind->r.h - (size + size + wind->y_shadow);
			draw_widg_box(0, wci, 0, &r, &wind->r);

			/* bottom-left box */
			r.y = wind->r.y + wind->r.h - (size + wind->y_shadow);
			r.h = size;
			draw_widg_box(0, wci, 0, &r, &wind->r);

			/* Bottom border */
			r.x += size;
			r.w = wind->r.w - (size + size + wind->x_shadow);
			draw_widg_box(0, wci, 0, &r, &wind->r);

			/* right-bottom box */
			r.x = wind->r.x + wind->r.w - (size + wind->x_shadow);
			r.w = size;
			draw_widg_box(0, wci, 0, &r, &wind->r);

			/* right border */
			r.y = wind->r.y + size;
			r.h = wind->r.h - (size + size + wind->y_shadow);
			draw_widg_box(0, wci, 0, &r, &wind->r);

			/* top-right box */
			r.y = wind->r.y;
			r.h = size;
			draw_widg_box(0, wci, 0, &r, &wind->r);

			/* Top border*/
			r.x = wind->r.x + size;
			r.w = wind->r.w - (size + size + wind->x_shadow);
			draw_widg_box(0, wci, 0, &r, &wind->r);
		}
		else
		{
			int i;
			r = wind->r;
			r.w -= wind->x_shadow;
			r.h -= wind->y_shadow;
			l_color(wind->colours->frame_col);
			for (i = 0; i < wind->frame; i++)
			{
				gbox(0, &r);
				r.x++;
				r.y++;
				r.w -= 2;
				r.h -= 2;
			}
		}
	}
	return true;
}

static bool
d_title(struct xa_window *wind, struct xa_widget *widg)
{
	struct options *o = &wind->owner->options;
	struct xa_wcol_inf *wci = &wind->colours->title;
	struct xa_wtxt_inf *wti = &wind->colours->title_txt;
	char tn[256];
	bool dial = (wind->dial & (created_for_FORM_DO|created_for_FMD_START)) != 0;

	/* Convert relative coords and window location to absolute screen location */
	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
	//rp_2_ap(wind, widg, &widg->ar);

	if (MONO)
	{
		f_color(G_WHITE);
		t_color(G_BLACK);
		/* with inside border */
		p_gbar(0, &widg->ar);
	}
	else
	{
		/* no move, no 3D */
		if (wind->active_widgets & MOVER)
		{
			draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
		}
		else
		{
			l_color(screen.dial_colours.shadow_col);
			p_gbar(0, &widg->ar);
			f_color(wci->n.c);
			gbar(-1, &widg->ar);
		}
	}

	wr_mode(MD_TRANS);
	
	if (o->windowner && wind->owner && !wind->winob && !dial)
	{
		char *ow = NULL;

		if (o->windowner == true)
			ow = wind->owner->proc_name;
		/* XXX this can't happen
		else if (o->windowner == 2)
			ow = wind->owner->name; */

		if  (ow)
		{
			char ns[32];
			strip_name(ns, ow);
			if (*ns)
				sprintf(tn, sizeof(tn), "(%s) %s", ns, widg->stuff);
			else
				ow = NULL;
		}

		if (!ow)
			sprintf(tn, sizeof(tn), "(%d) %s", wind->owner->p->pid, widg->stuff);
	}
	else
		strcpy(tn, widg->stuff);

	
	//cramped_name(tn, temp, widg->ar.w/screen.c_max_w);
#if 0
	if (dial)
		effect |= ITALIC;
#endif
	draw_widget_text(widg, wti, tn, 4, 0);
	return true;
}

static bool
d_closer(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->closer;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static bool
d_fuller(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->fuller;
	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static bool
d_info(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->info;
	struct xa_wtxt_inf *wti = &wind->colours->info_txt;

	/* Convert relative coords and window location to absolute screen location */
	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widget_text(widg, wti, widg->stuff, 4, 0); 
	return true;
}

static bool
d_sizer(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->sizer;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static bool
d_uparrow(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->uparrow;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
d_dnarrow(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->dnarrow;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
d_lfarrow(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->lfarrow;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
d_rtarrow(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->rtarrow;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static bool
d_vslide(struct xa_window *wind, struct xa_widget *widg)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	RECT cl;
	struct xa_window_colours *wc = wind->colours;

	sl->flags &= ~SLIDER_UPDATE;

	/* Convert relative coords and window location to absolute screen location */
	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.x = sl->r.y = 0;
		sl->r.w = widg->ar.w;
		sl->r.h = widg->ar.h;
		cl = widg->ar;
		draw_widg_box(0, &wc->slider, 0, &widg->ar, &widg->ar);
		return true;
	}
	len = sl_2_pix(widg->ar.h, sl->length);
	if (len < cfg.widg_h - 3)
		len = cfg.widg_h - 3;
	
	offs = widg->ar.y + sl_2_pix(widg->ar.h - len, sl->position);

	if (offs < widg->ar.y)
		offs = widg->ar.y;
	if (offs + len > widg->ar.y + widg->ar.h)
		len = widg->ar.y + widg->ar.h - offs;

	draw_widg_box(0, &wc->slide, 0, &widg->ar, &widg->ar);	
	
	sl->r.y = offs - widg->ar.y;
	sl->r.h = len;
	sl->r.w = widg->ar.w;

	cl.x = sl->r.x + widg->ar.x;
	cl.y = sl->r.y + widg->ar.y;
	cl.w = sl->r.w;
	cl.h = sl->r.h;
	
	draw_widg_box(-1, &wc->slider, widg->state, &cl, &cl);
	wr_mode(MD_TRANS);
	return true;
}

static bool
d_hslide(struct xa_window *wind, struct xa_widget *widg)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	struct xa_window_colours *wc = wind->colours;
	RECT cl;

	sl->flags &= ~SLIDER_UPDATE;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.x = sl->r.y = 0;
		sl->r.w = widg->ar.w;
		sl->r.h = widg->ar.h;
		draw_widg_box(0, &wc->slider, 0, &widg->ar, &widg->ar/*&wind->r*/);
		return true;
	}
	len = sl_2_pix(widg->ar.w, sl->length);
	if (len < cfg.widg_w - 3)
		len = cfg.widg_w - 3;
	
	offs = widg->ar.x + sl_2_pix(widg->ar.w - len, sl->position);

	if (offs < widg->ar.x)
		offs = widg->ar.x;
	if (offs + len > widg->ar.x + widg->ar.w)
		len = widg->ar.x + widg->ar.w - offs;
	
	draw_widg_box(0, &wc->slide, 0, &widg->ar, &widg->ar/*&wind->r*/);	
	
	sl->r.x = offs - widg->ar.x;
	sl->r.w = len;
	sl->r.h = widg->ar.h;

	cl.x = sl->r.x + widg->ar.x;
	cl.y = sl->r.y + widg->ar.y;
	cl.w = sl->r.w;
	cl.h = sl->r.h;
	
	draw_widg_box(-1, &wc->slider, widg->state, &cl, &cl/*&wind->r*/);
	wr_mode(MD_TRANS);
	return true;
}

static bool
d_iconifier(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->iconifier;

	(wind->widget_theme->rp2ap)(wind, widg, &widg->ar);
// 	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static bool
d_hider(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->hider;

	(*wind->widget_theme->rp2ap)(wind, widg, &widg->ar);	
	//rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static void
s_title_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->ontop_cols->title;
	struct xa_wtxt_inf *wti = &wind->ontop_cols->title_txt;
	short w, h;

	widg->r.w = cfg.widg_w;

	t_font(wti->n.p, wti->n.f);
	vst_effects(C.vh, wti->n.e);
	t_extent("A", &w, &h);
	vst_effects(C.vh, 0);
	
	if ((wci->flags & WCOL_DRAW3D) || (wti->flags & WTXT_DRAW3D))
		h += 4;
	if ((wci->flags & WCOL_ACT3D) || (wti->flags & WTXT_ACT3D))
		h++;

	widg->r.h = h;
};
static void
s_info_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->ontop_cols->info;
	struct xa_wtxt_inf *wti = &wind->ontop_cols->info_txt;
	short w, h;

	widg->r.w = cfg.widg_w;
		
	t_font(wti->n.p, wti->n.f);
	vst_effects(C.vh, wti->n.e);
	t_extent("A", &w, &h);
	vst_effects(C.vh, 0);
	
	if ((wci->flags & WCOL_DRAW3D) || (wti->flags & WTXT_DRAW3D))
		h += 4;
	if ((wci->flags & WCOL_ACT3D) || (wti->flags & WTXT_ACT3D))
		h++;

	widg->r.h = h;
};

static void
s_menu_size(struct xa_window *wind, struct xa_widget *widg)
{
	short w, h;
	t_font(screen.standard_font_point, screen.standard_font_id);
	vst_effects(C.vh, 0);
	t_extent("A", &w, &h);
	
	widg->r.h = h + 1 + 1; //screen.standard_font_height + 1;
	widg->r.w = wind->r.w;
}

static void
set_widg_size(struct xa_window *wind, struct xa_widget *widg, struct xa_wcol_inf *wci, short rsc_ind)
{
	short w, h, f;
	OBJECT *ob = wind->widg_info.tree + rsc_ind;

	f = wci->flags;

	object_spec_wh(ob, &w, &h);

	if (f & WCOL_DRAW3D)
		h += 2, w += 2;
	if (f & WCOL_BOXED)
		h += 2, w += 2;

	widg->r.w = w;
	widg->r.h = h;
}
static void
s_closer_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->closer, widg->loc.rsc_index);
}
static void
s_hider_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->hider, widg->loc.rsc_index);
}
static void
s_iconifier_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->iconifier, widg->loc.rsc_index);
}
static void
s_fuller_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->fuller, widg->loc.rsc_index);
}
static void
s_uparrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->uparrow, widg->loc.rsc_index);
}
static void
s_dnarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->dnarrow, widg->loc.rsc_index);
}
static void
s_lfarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->lfarrow, widg->loc.rsc_index);
}
static void
s_rtarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->rtarrow, widg->loc.rsc_index);
}
static void
s_sizer_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->sizer, widg->loc.rsc_index);
}
/*
 * The vertical slider must get its dimentions from the arrows
 */
static void
s_vslide_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->uparrow, WIDG_UP);
}
static void
s_hslide_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->rtarrow, WIDG_RIGHT);
}

void
init_widget_theme(struct widget_theme *t)
{

	t->exterior.draw	= d_unused;
	t->exterior.setsize	= NULL; //s_unused_size;
	
	t->border.draw		= draw_window_borders;
	t->border.setsize	= NULL; //s_border_size;

	t->title.draw		= d_title;
	t->title.setsize	= s_title_size;

	t->closer.draw		= d_closer;
	t->closer.setsize	= s_closer_size;
	
	t->fuller.draw		= d_fuller;
	t->fuller.setsize	= s_fuller_size;

	t->info.draw		= d_info;
	t->info.setsize		= s_info_size;

	t->sizer.draw		= d_sizer;
	t->sizer.setsize	= s_sizer_size;

	t->uparrow.draw		= d_uparrow;
	t->uparrow.setsize	= s_uparrow_size;

	t->dnarrow.draw		= d_dnarrow;
	t->dnarrow.setsize	= s_dnarrow_size;

	t->vslide.draw		= d_vslide;
	t->vslide.setsize	= s_vslide_size;

	t->lfarrow.draw		= d_lfarrow;
	t->lfarrow.setsize	= s_lfarrow_size;

	t->rtarrow.draw		= d_rtarrow;
	t->rtarrow.setsize	= s_rtarrow_size;

	t->hslide.draw		= d_hslide;
	t->hslide.setsize	= s_hslide_size;

	t->iconifier.draw	= d_iconifier;
	t->iconifier.setsize	= s_iconifier_size;

	t->hider.draw		= d_hider;
	t->hider.setsize	= s_hider_size;

	t->toolbar.draw		= NULL;
	t->toolbar.setsize	= NULL;

	t->menu.draw		= NULL;
	t->menu.setsize		= s_menu_size;

}
#if 0
void
draw_window_borders(struct xa_window *wind)
{
	struct xa_wcol_inf *wci = &wind->colours->borders;
	short size = wind->frame;
	RECT r;

	/* top-left box */
	r.x = wind->r.x;
	r.y = wind->r.y;
	r.w = size;
	r.h = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* Left border */
	r.y += size;
	r.h = wind->r.h - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* bottom-left box */
	r.y = wind->r.y + wind->r.h - (size + 2);
	r.h = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* Bottom border */
	r.x += size;
	r.w = wind->r.w - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* right-bottom box */
	r.x = wind->r.x + wind->r.w - (size + 2);
	r.w = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* right border */
	r.y = wind->r.y + size;
	r.h = wind->r.h - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* top-right box */
	r.y = wind->r.y;
	r.h = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* Top border*/
	r.x = wind->r.x + size;
	r.w = wind->r.w - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

}
#endif

