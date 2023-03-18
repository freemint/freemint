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
#include "xa_global.h"

#include "c_window.h"
#include "rectlist.h"
#include "widgets.h"

#define max(x,y) (((x)>(y))?(x):(y))
#define min(x,y) (((x)<(y))?(x):(y))

bool inline
is_inside(const GRECT *r, const GRECT *o)
{
	if (   (r->g_x        < o->g_x       )
	    || (r->g_y        < o->g_y       )
	    || (r->g_x + r->g_w > o->g_x + o->g_w)
	    || (r->g_y + r->g_h > o->g_y + o->g_h)
	   )
		return false;

	return true;
}

static struct xa_rect_list *
build_rect_list(struct build_rl_parms *p)
{
	struct xa_rect_list *rl, *nrl, *rl_next, *rl_prev;
	GRECT r_ours, r_win;

	nrl = kmalloc(sizeof(*nrl));
	assert(nrl);
	nrl->next = NULL;
	nrl->r = *p->area;

	{
		short wx2, wy2, sx2, sy2;

		wx2 = nrl->r.g_x + nrl->r.g_w;
		wy2 = nrl->r.g_y + nrl->r.g_h;
		sx2 = screen.r.g_x + screen.r.g_w;
		sy2 = screen.r.g_y + screen.r.g_h;

		if (nrl->r.g_x < screen.r.g_x)
		{
			nrl->r.g_w -= screen.r.g_x - nrl->r.g_x;
			nrl->r.g_x = screen.r.g_x;
		}
		if (wx2 > sx2)
			nrl->r.g_w -= wx2 - sx2;

		if (nrl->r.g_y < screen.r.g_y)
		{
			nrl->r.g_h -= screen.r.g_y - nrl->r.g_y;
			nrl->r.g_y = screen.r.g_y;
		}
		if (wy2 > sy2)
			nrl->r.g_h -= wy2 - sy2;
	}

	DIAGS(("build_rect_list: area=(%d/%d/%d/%d), nrl=(%d/%d/%d/%d)",
		p->area->g_x, p->area->g_y, p->area->g_w, p->area->g_h, nrl->r.g_x, nrl->r.g_y, nrl->r.g_w, nrl->r.g_h));

	if (nrl)
	{
		short flag, win_x2, win_y2, our_x2, our_y2;
		short w, h;

		while (p->getnxtrect(p))
		{
			r_win = *p->next_r;
			win_x2 = r_win.g_x + r_win.g_w;
			win_y2 = r_win.g_y + r_win.g_h;

			for (rl = nrl, rl_prev = NULL; rl; rl = rl_next)
			{
				r_ours = rl->r;

				flag = 0;

				h = r_win.g_y - r_ours.g_y;
				w = r_win.g_x - r_ours.g_x;

				rl_next = rl->next;

				if ( h < r_ours.g_h	&&
				     w < r_ours.g_w	&&
				     win_x2 > r_ours.g_x	&&
				     win_y2 > r_ours.g_y)
				{
					our_x2 = r_ours.g_x + r_ours.g_w;
					our_y2 = r_ours.g_y + r_ours.g_h;

					if (r_win.g_x > r_ours.g_x)
					{
						rl->r.g_x = r_ours.g_x;
						rl->r.g_y = r_ours.g_y;
						rl->r.g_h = r_ours.g_h;
						rl->r.g_w = w;

						r_ours.g_x += w;
						r_ours.g_w -= w;
						flag = 1;
					}
					if (r_win.g_y > r_ours.g_y)
					{
						if (flag)
						{
							rl_prev = rl;
							rl = kmalloc(sizeof(*rl));
							assert(rl);
							rl->next = rl_prev->next;
							rl_prev->next = rl;
						}
						rl->r.g_x = r_ours.g_x;
						rl->r.g_y = r_ours.g_y;
						rl->r.g_w = r_ours.g_w;
						rl->r.g_h = h;

						r_ours.g_y += h;
						r_ours.g_h -= h;

						flag = 1;
					}
					if (our_x2 > win_x2)
					{
						if (flag)
						{
							rl_prev = rl;
							rl = kmalloc(sizeof(*rl));
							assert(rl);
							rl->next = rl_prev->next;
							rl_prev->next = rl;
						}

						rl->r.g_x = win_x2;
						rl->r.g_y = r_ours.g_y;
						rl->r.g_w = our_x2 - win_x2;
						rl->r.g_h = r_ours.g_h;

						r_ours.g_w -= rl->r.g_w;
						flag = 1;
					}
					if (our_y2 > win_y2)
					{
						if (flag)
						{
							rl_prev = rl;
							rl = kmalloc(sizeof(*rl));
							assert(rl);
							rl->next = rl_prev->next;
							rl_prev->next = rl;
						}
						rl->r.g_x = r_ours.g_x;
						rl->r.g_y = win_y2;
						rl->r.g_w = r_ours.g_w;
						rl->r.g_h = our_y2 - win_y2;
						if( flag )
						{
#if 1
							/* a_avoid spltting workarea because of menu */
							if( cfg.menu_bar == 1 && menu_window && rl_prev->r.g_y < menu_window->r.g_h
								&& rl->r.g_x + rl->r.g_w == rl_prev->r.g_x && rl->r.g_y + rl->r.g_h == rl_prev->r.g_y + rl_prev->r.g_h )
							{
								rl->r.g_w += rl_prev->r.g_w;
								rl_prev->r.g_h = rl->r.g_y - rl_prev->r.g_y;
							}
#endif
						}

						r_ours.g_h -= rl->r.g_h;
						flag = 1;
					}
				}
				else
				{
					flag = 1;
				}

				if (!flag)
				{
					if (rl == nrl)
					{
						nrl = rl_next;
					}
					else if (nrl->next == rl)
						nrl->next = rl_next;

					if (rl_prev)
						rl_prev->next = rl_next;
					kfree(rl);
				}
				else
				{
					rl_prev = rl;
				}
			} /* for (rl = nrl; rl; rl = rl_next) */
		} /* while (wl) */
	} /* if (nrl && w->prev) */
	return nrl;
}

static int
nextwind_rect(struct build_rl_parms *p)
{
	int ret = 0;
	struct xa_window *wind = p->ptr1;


	while (wind && !ret)
	{
		/*
		 * Lets skip windows whose owner is exiting or is hidden
		 */
		if (!(wind->owner->status & CS_EXITING) && !(wind->active_widgets & STORE_BACK) &&
		     (wind->window_status & (XAWS_HIDDEN|XAWS_OPEN)) == XAWS_OPEN)
		{
			p->next_r = &wind->r;
			//wind = wind->prev;
			ret = 1;
			//break;
		}
		if (!wind->prev && !wind->nolist)
			wind = S.open_nlwindows.last;
		else
			wind = wind->prev;
	}
	p->ptr1 = wind;
	return ret;
}

struct xa_rect_list *
make_rect_list(struct xa_window *wind, bool swap, short which)
{
	struct xa_rect_list *nrl = NULL;
	struct xa_rectlist_entry *rle;
	struct build_rl_parms p;
	GRECT area;

	if ((wind->owner->status & CS_EXITING))
		return NULL;

	DIAGS(("Freeing old rect_list for %d", wind->handle));
	switch (which)
	{
		case RECT_SYS:
		{
			if (swap)
			{
				free_rectlist_entry(&wind->rect_list);
				free_rectlist_entry(&wind->rect_user);
				free_rectlist_entry(&wind->rect_opt);
			}
			area = wind->r;
			rle = &wind->rect_list;
			break;
		}
		case RECT_OPT:
		{
			if (swap)
				free_rectlist_entry(&wind->rect_opt);
			area = wind->rl_clip;
			rle = &wind->rect_opt;
			break;
		}
		case RECT_TOOLBAR:
		{
			if (swap)
				free_rectlist_entry(&wind->rect_toolbar);
			if (!usertoolbar_installed(wind) || !xa_rect_clip(&wind->r, &wind->widgets[XAW_TOOLBAR].ar, &area))
				return NULL;
			rle = &wind->rect_toolbar;
			break;
		}
		default:;
			return NULL;
	}

	DIAGS(("make_rect_list for wind %d", wind->handle));

	if ((wind->window_status & (XAWS_HIDDEN|XAWS_BELOWROOT)) ||
	    area.g_x > (screen.r.g_x + screen.r.g_w) ||
	    area.g_y > (screen.r.g_y + screen.r.g_h) ||
	   (area.g_x + area.g_w) < screen.r.g_x  ||
	   (area.g_y + area.g_h) < screen.r.g_y )
	{
		DIAGS(("make_rect_list: window is outside screen"));
		return NULL;
	}

	p.getnxtrect = nextwind_rect;
	p.area = &area;
	if (!wind->prev && !wind->nolist)
		p.ptr1 = S.open_nlwindows.last;
	else
		p.ptr1 = wind->prev;

	nrl = build_rect_list(&p);
	if (swap)
		rle->start = rle->next = nrl;

	return nrl;
}

static struct xa_rect_list *
get_rect_first(struct xa_rectlist_entry *rle)
{
	rle->next = rle->start;
	return rle->next;
}
static struct xa_rect_list *
get_rect_next(struct xa_rectlist_entry *rle)
{
	if (rle->next)
		rle->next = rle->next->next;
	return rle->next;
}

/*
 * return 1 if found else 0
 */
int
get_rect(struct xa_rectlist_entry *rle, GRECT *clip, bool first, GRECT *ret)
{
	struct xa_rect_list *rl;
	int rtn = 0;
	GRECT r;

	if (first)
		rl = get_rect_first(rle);
	else
		rl = get_rect_next(rle);

	if (clip)
	{
		while (rl)
		{
			if (xa_rect_clip(&rl->r, clip, &r))
			{
				*ret = r;
				rtn = 1;
				break;
			}
			rl = get_rect_next(rle);
		}
	}
	else if (rl)
	{
		*ret = rl->r;
		rtn = 1;
	}

	return rtn;
}

#if INCLUDE_UNUSED
void
free_rect_list(struct xa_rect_list *first)
{
	struct xa_rect_list *next;

	DIAGS(("free_rect_list: start=%lx", first));
	while (first)
	{
		next = first->next;
		kfree(first);
		first = next;
	}
}
#endif
void
free_rectlist_entry(struct xa_rectlist_entry *rle)
{
	struct xa_rect_list *rl;

	while ((rl = rle->start))
	{
		rle->start = rl->next;
		kfree(rl);
	}
	rle->next = NULL;
}

#if INCLUDE_UNUSED
struct xa_rect_list *
rect_get_user_first(struct xa_window *w)
{
	w->rect_user.next = w->rect_user.start;
	return w->rect_user.next;
}
#endif
struct xa_rect_list *
rect_get_optimal_first(struct xa_window *w)
{
	w->rect_opt.next = w->rect_opt.start;
	return w->rect_opt.next;
}

#if INCLUDE_UNUSED
struct xa_rect_list *
rect_get_system_first(struct xa_window *w)
{
	w->rect_list.next = w->rect_list.start;
	return w->rect_list.next;
}
#endif
struct xa_rect_list *
rect_get_optimal_next(struct xa_window *w)
{
	if (w->rect_opt.next)
		w->rect_opt.next = w->rect_opt.next->next;
	return w->rect_opt.next;
}

#if INCLUDE_UNUSED
struct xa_rect_list *
rect_get_user_next(struct xa_window *w)
{
	if (w->rect_user.next)
		w->rect_user.next = w->rect_user.next->next;
	return w->rect_user.next;
}
#endif
#if INCLUDE_UNUSED
struct xa_rect_list *
rect_get_system_next(struct xa_window *w)
{
	if (w->rect_list.next)
		w->rect_list.next = w->rect_list.next->next;
	return w->rect_list.next;
}
#endif
struct xa_rect_list *
rect_get_toolbar_first(struct xa_window *w)
{
	w->rect_toolbar.next = w->rect_toolbar.start;
	return w->rect_toolbar.next;
}
struct xa_rect_list *
rect_get_toolbar_next(struct xa_window *w)
{
	if (w->rect_toolbar.next)
		w->rect_toolbar.next = w->rect_toolbar.next->next;
	return w->rect_toolbar.next;
}

/*
 * Compute intersection of two rectangles; put result rectangle
 * into *d; return true if intersection is nonzero.
 *
 * (Original version of this function taken from Digital Research's
 * GEM sample application `DEMO' [aka `DOODLE'],  Version 1.1,
 * March 22, 1985)
 */
bool
xa_rc_intersect(const GRECT s, GRECT *d)
{
	if (s.g_w > 0 && s.g_h > 0 && d->g_w > 0 && d->g_h > 0)
	{
		const short w1 = s.g_x + s.g_w;
		const short w2 = d->g_x + d->g_w;
		const short h1 = s.g_y + s.g_h;
		const short h2 = d->g_y + d->g_h;

		d->g_x = max(s.g_x, d->g_x);
		d->g_y = max(s.g_y, d->g_y);
		d->g_w = min(w1, w2) - d->g_x;
		d->g_h = min(h1, h2) - d->g_y;

		return (d->g_w > 0) && (d->g_h > 0);
	}
	else
		return false;
}
/* Ozk:
 * This is my (ozk) version of the xa_rc_intersect.
 * Takes pointers to source, destination and result
 * rectangle structures.
 */
bool
xa_rect_clip(const GRECT *s, const GRECT *d, GRECT *r)
{
	if (s->g_w > 0 && s->g_h > 0 && d->g_w > 0 && d->g_h > 0)
	{
		const short w1 = s->g_x + s->g_w;
		const short w2 = d->g_x + d->g_w;
		const short h1 = s->g_y + s->g_h;
		const short h2 = d->g_y + d->g_h;

		r->g_x = s->g_x > d->g_x ? s->g_x : d->g_x;	//max(s->x, d->g_x);
		r->g_y = s->g_y > d->g_y ? s->g_y : d->g_y;	//max(s->y, d->g_y);
		r->g_w = (w1 < w2 ? w1 : w2) - r->g_x; 	//min(w1, w2) - d->g_x;
		r->g_h = (h1 < h2 ? h1 : h2) - r->g_y;	//min(h1, h2) - d->g_y;

		return ((r->g_w > 0) && (r->g_h > 0));
	}
	else
		return false;
}
/*
 * return
 *
 * 0	s or d has w=0 or h=0 or no intersection
 * 1	d not inside s
 * 2	d inside s
 *
 */
int
xa_rect_chk(const GRECT *s, const GRECT *d, GRECT *r)
{
	int ret = 0;

	if (s->g_w > 0 && s->g_h > 0 && d->g_w > 0 && d->g_h > 0)
	{
		const short sw = s->g_x + s->g_w;
		const short dw = d->g_x + d->g_w;
		const short sh = s->g_y + s->g_h;
		const short dh = d->g_y + d->g_h;

		r->g_x = s->g_x < d->g_x ? d->g_x : s->g_x;
		r->g_y = s->g_y < d->g_y ? d->g_y : s->g_y;
		r->g_w = (sw < dw ? sw : dw) - r->g_x;
		r->g_h = (sh < dh ? sh : dh) - r->g_y;

		if (r->g_x == d->g_x && r->g_y == d->g_y && r->g_w == d->g_w && r->g_h == d->g_h)
			ret = 2;
		else if ((r->g_w > 0) && (r->g_h > 0))
			ret = 1;
	}
	return ret;
}
