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
is_inside(const RECT *r, const RECT *o)
{
	if (   (r->x        < o->x       )
	    || (r->y        < o->y       )
	    || (r->x + r->w > o->x + o->w)
	    || (r->y + r->h > o->y + o->h)
	   )
		return false;

	return true;
}

STATIC struct xa_rect_list *
build_rect_list(struct build_rl_parms *p)
{
	struct xa_rect_list *rl, *nrl, *rl_next, *rl_prev;
	RECT r_ours, r_win;

	nrl = kmalloc(sizeof(*nrl));
	assert(nrl);
	nrl->next = NULL;
	nrl->r = *p->area;

	{
		short wx2, wy2, sx2, sy2;

		wx2 = nrl->r.x + nrl->r.w;
		wy2 = nrl->r.y + nrl->r.h;
		sx2 = screen.r.x + screen.r.w;
		sy2 = screen.r.y + screen.r.h;

		if (nrl->r.x < screen.r.x)
		{
			nrl->r.w -= screen.r.x - nrl->r.x;
			nrl->r.x = screen.r.x;
		}
		if (wx2 > sx2)
			nrl->r.w -= wx2 - sx2;

		if (nrl->r.y < screen.r.y)
		{
			nrl->r.h -= screen.r.y - nrl->r.y;
			nrl->r.y = screen.r.y;
		}
		if (wy2 > sy2)
			nrl->r.h -= wy2 - sy2;
	}

	DIAGS(("build_rect_list: area=(%d/%d/%d/%d), nrl=(%d/%d/%d/%d)",
		p->area->x, p->area->y, p->area->w, p->area->h, nrl->r.x, nrl->r.y, nrl->r.w, nrl->r.h));

	if (nrl)
	{
		short flag, win_x2, win_y2, our_x2, our_y2;
		short w, h;

		while (p->getnxtrect(p))
		{
			r_win = *p->next_r;
			win_x2 = r_win.x + r_win.w;
			win_y2 = r_win.y + r_win.h;

			for (rl = nrl, rl_prev = NULL; rl; rl = rl_next)
			{
				r_ours = rl->r;

				flag = 0;

				h = r_win.y - r_ours.y;
				w = r_win.x - r_ours.x;

				rl_next = rl->next;

				if ( h < r_ours.h	&&
				     w < r_ours.w	&&
				     win_x2 > r_ours.x	&&
				     win_y2 > r_ours.y)
				{
					our_x2 = r_ours.x + r_ours.w;
					our_y2 = r_ours.y + r_ours.h;

					if (r_win.x > r_ours.x)
					{
						rl->r.x = r_ours.x;
						rl->r.y = r_ours.y;
						rl->r.h = r_ours.h;
						rl->r.w = w;

						r_ours.x += w;
						r_ours.w -= w;
						flag = 1;
					}
					if (r_win.y > r_ours.y)
					{
						if (flag)
						{
							rl_prev = rl;
							rl = kmalloc(sizeof(*rl));
							assert(rl);
							rl->next = rl_prev->next;
							rl_prev->next = rl;
						}
						rl->r.x = r_ours.x;
						rl->r.y = r_ours.y;
						rl->r.w = r_ours.w;
						rl->r.h = h;

						r_ours.y += h;
						r_ours.h -= h;

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

						rl->r.x = win_x2;
						rl->r.y = r_ours.y;
						rl->r.w = our_x2 - win_x2;
						rl->r.h = r_ours.h;

						r_ours.w -= rl->r.w;
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
						rl->r.x = r_ours.x;
						rl->r.y = win_y2;
						rl->r.w = r_ours.w;
						rl->r.h = our_y2 - win_y2;
						if( flag )
						{
#if 1
							/* a_avoid spltting workarea because of menu */
							if( cfg.menu_bar == 1 && menu_window && rl_prev->r.y < menu_window->r.h
								&& rl->r.x + rl->r.w == rl_prev->r.x && rl->r.y + rl->r.h == rl_prev->r.y + rl_prev->r.h )
							{
								rl->r.w += rl_prev->r.w;
								rl_prev->r.h = rl->r.y - rl_prev->r.y;
							}
#endif
						}

						r_ours.h -= rl->r.h;
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
	RECT area;

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
	    area.x > (screen.r.x + screen.r.w) ||
	    area.y > (screen.r.y + screen.r.h) ||
	   (area.x + area.w) < screen.r.x  ||
	   (area.y + area.h) < screen.r.y )
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
get_rect(struct xa_rectlist_entry *rle, RECT *clip, bool first, RECT *ret)
{
	struct xa_rect_list *rl;
	int rtn = 0;
	RECT r;

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
xa_rc_intersect(const RECT s, RECT *d)
{
	if (s.w > 0 && s.h > 0 && d->w > 0 && d->h > 0)
	{
		const short w1 = s.x + s.w;
		const short w2 = d->x + d->w;
		const short h1 = s.y + s.h;
		const short h2 = d->y + d->h;

		d->x = max(s.x, d->x);
		d->y = max(s.y, d->y);
		d->w = min(w1, w2) - d->x;
		d->h = min(h1, h2) - d->y;

		return (d->w > 0) && (d->h > 0);
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
xa_rect_clip(const RECT *s, const RECT *d, RECT *r)
{
	if (s->w > 0 && s->h > 0 && d->w > 0 && d->h > 0)
	{
		const short w1 = s->x + s->w;
		const short w2 = d->x + d->w;
		const short h1 = s->y + s->h;
		const short h2 = d->y + d->h;

		r->x = s->x > d->x ? s->x : d->x;	//max(s->x, d->x);
		r->y = s->y > d->y ? s->y : d->y;	//max(s->y, d->y);
		r->w = (w1 < w2 ? w1 : w2) - r->x; 	//min(w1, w2) - d->x;
		r->h = (h1 < h2 ? h1 : h2) - r->y;	//min(h1, h2) - d->y;

		return ((r->w > 0) && (r->h > 0));
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
xa_rect_chk(const RECT *s, const RECT *d, RECT *r)
{
	int ret = 0;

	if (s->w > 0 && s->h > 0 && d->w > 0 && d->h > 0)
	{
		const short sw = s->x + s->w;
		const short dw = d->x + d->w;
		const short sh = s->y + s->h;
		const short dh = d->y + d->h;

		r->x = s->x < d->x ? d->x : s->x;
		r->y = s->y < d->y ? d->y : s->y;
		r->w = (sw < dw ? sw : dw) - r->x;
		r->h = (sh < dh ? sh : dh) - r->y;

		if (r->x == d->x && r->y == d->y && r->w == d->w && r->h == d->h)
			ret = 2;
		else if ((r->w > 0) && (r->h > 0))
			ret = 1;
	}
	return ret;
}
