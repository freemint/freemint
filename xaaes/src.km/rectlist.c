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

#include "c_window.h"
#include "rectlist.h"

#define max(x,y) (((x)>(y))?(x):(y))
#define min(x,y) (((x)<(y))?(x):(y))

struct xa_rect_list *
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
		*p->area, nrl->r));

	if (nrl)
	{
		short flag, win_x2, win_y2, our_x2, our_y2;
		short w, h;
#if GENERATE_DIAGS
		short i;
#endif

		while (p->getnxtrect(p))
		{
#if GENERATE_DIAGS
			i = 0;
#endif
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
#if GENERATE_DIAGS
				i++;
#endif
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
						DIAGS((" -- 2. new=(%d/%d/%d/%d), remain=(%d/%d/%d/%d)", rl->r, r_ours));
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
							DIAGS((" -- 2. alloc new=%lx", rl));
						}
#if GENERATE_DIAGS
						else
							DIAGS((" -- 2. using orig=%lx", rl));
#endif
						rl->r.x = r_ours.x;
						rl->r.y = r_ours.y;
						rl->r.w = r_ours.w;
						rl->r.h = h;

						r_ours.y += h;
						r_ours.h -= h;
					
						flag = 1;
						DIAGS((" -- 1. new=(%d/%d/%d/%d), remain=(%d/%d/%d/%d)", rl->r, r_ours));
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
							DIAGS((" -- 4. alloc new=%lx", rl));
						}
#if GENERATE_DIAGS
						else
							DIAGS((" -- 4. using orig=%lx", rl));
#endif

						rl->r.x = win_x2;
						rl->r.y = r_ours.y;
						rl->r.w = our_x2 - win_x2;
						rl->r.h = r_ours.h;

						r_ours.w -= rl->r.w;
						flag = 1;
						DIAGS((" -- 4. new=(%d/%d/%d/%d), remain=(%d/%d/%d/%d)", rl->r, r_ours));
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
							DIAGS((" -- 3. alloc new=%lx", rl));
						}
#if GENERATE_DIAGS
						else
							DIAGS((" -- 3. using orig=%lx", rl));
#endif
						rl->r.x = r_ours.x;
						rl->r.y = win_y2;
						rl->r.w = r_ours.w;
						rl->r.h = our_y2 - win_y2;

						r_ours.h -= rl->r.h;
						flag = 1;
						DIAGS((" -- 3. new=(%d/%d/%d/%d), remain=(%d/%d/%d/%d)", rl->r, r_ours));
					}
				}
				else
				{
					flag = 1;
					DIAGS((" -- 1. whole=(%d/%d/%d/%d)", rl->r));
				}

				if (!flag)
				{
					DIAGS((" covered, releasing (nrl=%lx) %lx=(%d/%d/%d/%d) rl_prev=%lx(%lx)",
						nrl, rl, rl->r, rl_prev, rl_prev ? (long)rl_prev->next : 0));
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
#if GENERATE_DIAGS
	else
		DIAGS((" -- RECT is topped"));
	
	{
		short i = 0;
		rl = nrl;
		while (rl)
		{
			i++;
			rl = rl->next;
		}
		DIAGS((" make_rect_list: created %d rectangles, first=%lx", i, nrl));
	}
#endif
	return nrl;
}

static int
nextwind_rect(struct build_rl_parms *p)
{
	int ret = 0;
	struct xa_window *wind = p->ptr1;

	while (wind)
	{
		/*
		 * Lets skip windows whose owner is exiting or is hidden
		 */
		if (!(wind->owner->status & CS_EXITING) &&
		     (wind->window_status & (XAWS_HIDDEN|XAWS_OPEN)) == XAWS_OPEN)
		{
			p->next_r = &wind->r;
			wind = wind->prev;
			ret = 1;
			break;
		}
		wind = wind->prev;
	}
	p->ptr1 = wind;
	return ret;
}

struct xa_rect_list *
make_rect_list(struct xa_window *wind, bool swap, short which)
{
	struct xa_rect_list *nrl = NULL;
	struct build_rl_parms p;

	if (swap)
	{
		DIAGS(("Freeing old rect_list for %d", wind->handle));
		switch (which)
		{
			case RECT_SYS:
			{
				free_rect_list(wind->rect_start);
				wind->rect_list = wind->rect_user = wind->rect_start = NULL;
				/* Fall through */
			}
			case RECT_OPT:
			{
				free_rect_list(wind->rect_opt_start);
				wind->rect_opt = wind->rect_opt_start = NULL;
				break;
			}
		}
	}

	if (wind->owner->status & CS_EXITING)
		return NULL;

	DIAGS(("make_rect_list for wind %d", wind->handle));

	if (wind->r.x > (screen.r.x + screen.r.w) ||
	    wind->r.y > (screen.r.y + screen.r.h) ||
	   (wind->r.x + wind->r.w) < screen.r.x  ||
	   (wind->r.y + wind->r.h) < screen.r.y )
	{
		DIAGS(("make_rect_list: window is outside screen"));
		return NULL;
	}

	switch (which)
	{
		case RECT_SYS:
		{
			p.getnxtrect = nextwind_rect;
			p.area = &wind->r;
			p.ptr1 = wind->prev;
			nrl = build_rect_list(&p);
			if (nrl && swap)
				wind->rect_list = wind->rect_user = wind->rect_start = nrl;
				
			break;
		}
		case RECT_OPT:
		{
			p.getnxtrect = nextwind_rect;
			p.area = &wind->rl_clip;
			p.ptr1 = wind->prev;
			nrl = build_rect_list(&p);
			if (nrl && swap)
				wind->rect_opt_start = wind->rect_opt = nrl;
			break;
		}
	}
	return nrl;
}

int
get_rect(struct xa_window *wind, RECT *clip, bool first, RECT *ret)
{
	struct xa_rect_list *rl;
	int rtn = 0;
	RECT r;

	if (first)
		rl = rect_get_user_first(wind);
	else
		rl = rect_get_user_next(wind);
		
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
			rl = rect_get_user_next(wind);
		}
	}
	else if (rl)
	{
		*ret = rl->r;
		rtn = 1;
	}

	return rtn;
}

void
free_rect_list(struct xa_rect_list *first)
{
	struct xa_rect_list *next;
#if GENERATE_DIAGS
	short i = 0;
#endif

	DIAGS(("free_rect_list: start=%lx", first));
	while (first)
	{
		DIAGS((" -- freeing %lx, next=%lx",
			first, (long)first->next));
#if GENERATE_DIAGS
		i++;
#endif
		next = first->next;
		kfree(first);
		first = next;
	}
	DIAGS((" -- freed %d rectnagles", i));
}

struct xa_rect_list *
rect_get_user_first(struct xa_window *w)
{
	w->rect_user = w->rect_start;
	return w->rect_user;
}

struct xa_rect_list *
rect_get_optimal_first(struct xa_window *w)
{
	w->rect_opt = w->rect_opt_start;
	return w->rect_opt;
}

struct xa_rect_list *
rect_get_system_first(struct xa_window *w)
{
	w->rect_list = w->rect_start;
	return w->rect_list;
}

struct xa_rect_list *
rect_get_optimal_next(struct xa_window *w)
{
	if (w->rect_opt)
		w->rect_opt = w->rect_opt->next;
	return w->rect_opt;
}

struct xa_rect_list *
rect_get_user_next(struct xa_window *w)
{
	if (w->rect_user)
		w->rect_user = w->rect_user->next;
	return w->rect_user;
}

struct xa_rect_list *
rect_get_system_next(struct xa_window *w)
{
	if (w->rect_list)
		w->rect_list = w->rect_list->next;
	return w->rect_list;
}

bool
was_visible(struct xa_window *w)
{
	RECT o, n;
	if (w->rect_prev && w->rect_start)
	{
		o = w->rect_prev->r;
		n = w->rect_start->r;
		DIAG((D_r, w->owner, "was_visible? prev (%d/%d,%d/%d) start (%d/%d,%d/%d)", o, n));
		if (o.x == n.x && o.y == n.y && o.w == n.w && o.h == n.h)
			return true;
	}
	return false;
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
xa_rect_clip(const RECT *s, RECT *d, RECT *r)
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

		return (r->w > 0) && (r->h > 0);
	}
	else
		return false;
}
