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

#include "c_window.h"
#include "xa_global.h"

#include "app_man.h"
#include "taskman.h"
#include "desktop.h"
#include "k_main.h"
#include "k_mouse.h"
#include "menuwidg.h"
#include "draw_obj.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "widgets.h"
#include "xa_bubble.h"
#include "xa_graf.h"
#include "xa_wind.h"

#include "mint/signal.h"

STATIC struct xa_window *pull_wind_to_top(enum locks lock, struct xa_window *w);
STATIC void	fitin_root(RECT *r);
STATIC void	set_and_update_window(struct xa_window *wind, bool blit, bool only_wa, RECT *new);

/*
 * Window Stack Management Functions
 */

//static void draw_window(enum locks lock, struct xa_window *wind);

/* HR:
 * I hated the kind of runaway behaviour (return wind_handle++)
 * So here is my solution:
 * Window handle bit array (used to generate unique window handles)
 * As this fills up, there may be a problem if a user opens
 * more than MAX_WINDOWS windows concurrently :-)
 */

static unsigned long
btst(unsigned long word, unsigned int bit)
{
	unsigned long mask = 1UL << bit;

	return (word & mask);
}
static void
bclr(unsigned long *word, unsigned int bit)
{
	unsigned long mask = 1UL << bit;

	*word &= ~mask;
}
static void
bset(unsigned long *word, unsigned int bit)
{
	unsigned long mask = 1UL << bit;

	*word |= mask;
}

static unsigned long wind_handle[MAX_WINDOWS / LBITS];
static const unsigned int words = MAX_WINDOWS / LBITS;

static int
new_wind_handle(void)
{
	unsigned int word;
	unsigned int l;

	for (word = 0; word < words; word++)
		if (wind_handle[word] != 0xffffffffUL)
			break;

#if XXX
	assert(word < words);
#endif

	l = 0;
	while (btst(wind_handle[word], l))
		l++;

	bset(wind_handle+word, l);
	return word * LBITS + l;
}

static void
free_wind_handle(int h)
{
	unsigned int word = h / LBITS;

	bclr(wind_handle+word, h % LBITS);
}

void
clear_wind_handles(void)
{
	unsigned int f;

	for (f = 0; f < words; f++)
		wind_handle[f] = 0;
}

/*
 * Go through and check that all windows belonging to this client are
 * closed and deleted
 */

/* HR: This is an exemple of the use of the lock system.
 *     The function can be called by both server and client.
 *
 *     If called by the server, lock will be preset to winlist,
 *     resulting in no locking (not needed).
 *
 *     If called by the signal handler, it is unclear, so the
 *     lock is applied. (doesnt harm).
 */
static void
rw(enum locks lock, struct xa_window *wl, struct xa_client *client)
{
	struct xa_window *nwl;

	while (wl)
	{
		DIAGS(("-- RW: %lx, next=%lx, prev=%lx",
			wl, wl->next, wl->prev));

		nwl = wl->next;
		if (wl != root_window && wl != menu_window)
		{
			if (!client || wl->owner == client)
			{
				if ((wl->window_status & XAWS_OPEN))
				{
					DIAGS(("-- RW: closing %lx, client=%lx", wl, client));
					close_window(lock, wl);
				}
				DIAGS(("-- RW: deleting %lx", wl));
				delete_window(lock, wl);
			}
		}
#if GENERATE_DIAGS
		else
			DIAGS((" -- RW: skipping root window"));
#endif
		wl = nwl;
	}
}

void
remove_windows(enum locks lock, struct xa_client *client)
{
	DIAG((D_wind,client,"remove_windows on open_windows list for %s", c_owner(client)));
	if( !client )
		return;
	rw(lock, window_list, client);
	DIAG((D_wind,client,"remove_windows on closed_windows list for %s", c_owner(client)));
	rw(lock, S.closed_windows.first, client);
}

void
remove_all_windows(enum locks lock, struct xa_client *client)
{
	DIAG((D_wind, client,"remove_all_windows for %s", c_owner(client)));

	if( !client )
		return;
	remove_windows(lock, client);
	DIAG((D_wind, client, "remove_all_windows on open_nlwindows for %s", c_owner(client)));
	rw(lock, S.open_nlwindows.first, client);
	DIAG((D_wind, client, "remove_all_windows on closed_nlwindows for %s", c_owner(client)));
	rw(lock, S.closed_nlwindows.first, client);
	rw(lock, S.calc_windows.first, client);
}

inline static void
calc_shade_height(struct xa_window *wind)
{
	if (wind->active_widgets & (CLOSER|NAME|MOVER|ICONIFIER|FULLER))
	{
		RECT t;
		t = w2f(&wind->rbd, &wind->widgets[XAW_TITLE].r, false);
		if( t.h < 6 )
			t.h = 6;
		wind->sh = t.h + wind->y_shadow;
		if (wind->frame > 0)
			wind->sh += (wind->frame << 1);
	}
}

STATIC void
clear_wind_rectlist(struct xa_window *wind)
{
	DIAGS(("clear_wind_rectlist: on %d of %s", wind->handle, wind->owner->name));
	free_rectlist_entry(&wind->rect_list);
	free_rectlist_entry(&wind->rect_opt);
	free_rectlist_entry(&wind->rect_toolbar);
}

/*
 * Find first free position for iconification.
 * Uses the RECT at wind->t.
 */
RECT
free_icon_pos(enum locks lock, struct xa_window *ignore)
{
	RECT ic, r;
	int i = 0;

	for (;;)
	{
		struct xa_window *w = window_list;

		ic = iconify_grid(i++);
// 		display("icgrid %d/%d/%d/%d", ic);

		/* find first unused position. */
		while (w)
		{
			if (w != ignore && (is_iconified(w) || (w->window_status & XAWS_CHGICONIF)))
			{
				r = ic;

				if (w->t.x == r.x && w->t.y == r.y)
					/* position occupied; advance with next position in grid. */
					break;
			}
			w = w->next;
			if (w == root_window)
			{
				w = NULL;
				break;
			}
		}
		if (!w)
			/* position not in list of iconified windows, so it is free. */
			break;
	}
	return ic;
}
static void
move_ctxdep_widgets(struct xa_window *wind)
{
	if (wind->active_widgets & TOOLBAR)
		rp_2_ap_cs(wind, get_widget(wind, XAW_TOOLBAR), NULL);
	if (wind->active_widgets & XaMENU)
	{
		rp_2_ap_cs(wind, get_widget(wind, XAW_MENU), NULL);
	}
}

XA_WIND_ATTR
hide_move(struct options *o)
{
	XA_WIND_ATTR kind = 0;

	if (!o->xa_nohide)
		kind |= HIDE;
	if (!o->xa_nomove)
		kind |= MOVER;

	return kind;
}

void
set_winrect(struct xa_window *wind, RECT *wr, const RECT *new)
{
	RECT r;

	if (wind->opts & XAWO_WCOWORK)
		r = w2f(&wind->delta, new, true);
	else
		r = *new;

	fitin_root(&r);

	*wr = r;
}

/* return 0 if true
 * 1 if too high
 * 2 if too left
 * 3 if both
 */
short
inside_root(RECT *r, bool noleft)
{
	short ret = 0;
	if( !cfg.leave_top_border )
	{
		short min = 0;

		if (cfg.menu_bar && cfg.menu_layout == 0 && cfg.menu_ontop == 0)
		{
			min = get_menu_height();
		}
		if (r->y < min )
		{
			r->y = min;
			ret = 1;
		}
	}

	if (noleft && r->x < root_window->wa.x)
	{
		r->x = root_window->wa.x;
		ret |= 2;
	}
	return ret;
}

STATIC void
fitin_root(RECT *r)
{
	RECT *w = &root_window->wa;

	if (r->x < w->x)
		r->x = w->x;
	if (r->y < w->y)
		r->y = w->y;
	if (r->w > w->w)
		r->w = w->w;
	if (r->h > w->h)
		r->h = w->h;
}

void
inside_minmax(RECT *r, struct xa_window *wind)
{
	bool um = (wind->active_widgets & USE_MAX);

	if (!is_iconified(wind))
	{
		if (um && wind->max.w && r->w > wind->max.w)
			r->w = wind->max.w;
		if (wind->min.w && r->w < wind->min.w)
			r->w = wind->min.w;

		if (um && wind->max.h && r->h > wind->max.h)
			r->h = wind->max.h;
		if (wind->min.h && r->h < wind->min.h)
			r->h = wind->min.h;
	}
}

#if GENERATE_DIAGS
static char *stacks[] =
{
	"HUH!!",
	"Open windows",
	"Closed windows",
	"Open nlwindows",
	"Closed nlwindows",
	"Deleted windows",
	"hidden windows",
	"calc_windows",
};
#endif

STATIC void
wi_remove(struct win_base *b, struct xa_window *w, bool chkfocus)
{
#if GENERATE_DIAGS
	int stack = 0;
	if (b == &S.open_windows)
		stack = 1;
	else if (b == &S.closed_windows)
		stack = 2;
	else if (b == &S.open_nlwindows)
		stack = 3;
	else if (b == &S.closed_nlwindows)
		stack = 4;
	else if (b == &S.deleted_windows)
		stack = 5;
	else if (b == &S.hidden_windows)
		stack = 6;
	else if (b == &S.calc_windows)
		stack = 7;

	DIAGA(("wi_put_first: on stack %s", stacks[stack]));
#endif
	if (w->prev)
		w->prev->next = w->next;
	if (w->next)
		w->next->prev = w->prev;
	if (w == b->first)
		b->first = w->next;
	if (w == b->last)
		b->last = w->prev;
	w->next = NULL;
	w->prev = NULL;

	if (b->top == w)
	{
		w->window_status &= ~XAWS_FIRST;
		b->top = b->first;
		if (b->top)
			b->first->window_status |= XAWS_FIRST;
		DIAGA(("wi_remove: remove top %d of %s, newtop %d of %s",
			w->handle, w->owner->name,
			b->top ? b->top->handle : -2,
			b->top ? b->top->owner->name : "no more winds"));
	}
	if (chkfocus && w == S.focus) {
		DIAGA(("wi_remove: remove focus %d of %s", w->handle, w->owner->name));
		S.focus = NULL;
	}
}

STATIC void
wi_put_first(struct win_base *b, struct xa_window *w)
{
	struct xa_window **wind;
#if GENERATE_DIAGS
	int stack = 0;
	if (b == &S.open_windows)
		stack = 1;
	else if (b == &S.closed_windows)
		stack = 2;
	else if (b == &S.open_nlwindows)
		stack = 3;
	else if (b == &S.closed_nlwindows)
		stack = 4;
	else if (b == &S.deleted_windows)
		stack = 5;
	else if (b == &S.hidden_windows)
		stack = 6;
	else if (b == &S.calc_windows)
		stack = 7;
	DIAGA(("wi_put_first: on stack %s", stacks[stack]));
#endif

	wind = &b->first;

	if (!(w->window_status & XAWS_FLOAT))
	{
		while (*wind)
		{
			if (!((*wind)->window_status & XAWS_FLOAT) || *wind == root_window)
				break;
			wind = &((*wind)->next);
		}
	}
	if (*wind)
	{
		w->next = *wind;
		w->prev = (*wind)->prev;
		(*wind)->prev = w;
	}
	else
	{
		if (b->first)
		{
			w->prev = b->last;
			b->last->next = w;
		}
		else
			w->prev = NULL;

		b->last = w;
		w->next = NULL;
	}
	*wind = w;
	if (b->top) {
		DIAGA(("wi_put_first: detopped %d of %s", b->top->handle, b->top->owner->name));
		b->top->window_status &= ~XAWS_FIRST;
	}
	b->top = w;
	DIAGA(("wi_put_first: new top %d of %s", w->handle, w->owner->name));
	w->window_status |= XAWS_FIRST;
}

extern bool diags_always;

/* This a special version of put last; it puts w just befor last. */
static void
wi_put_blast(struct win_base *b, struct xa_window *w, bool top, bool belowroot)
{
	struct xa_window **wind;
#if GENERATE_DIAGS
	int stack = 0;
	if (b == &S.open_windows)
		stack = 1;
	else if (b == &S.closed_windows)
		stack = 2;
	else if (b == &S.open_nlwindows)
		stack = 3;
	else if (b == &S.closed_nlwindows)
		stack = 4;
	else if (b == &S.deleted_windows)
		stack = 5;
	else if (b == &S.hidden_windows)
		stack = 6;
	else if (b == &S.calc_windows)
		stack = 7;
	DIAGA(("wi_put_first: on stack %s", stacks[stack]));
#endif

	wind = &b->last;

	if (belowroot)
	{
		wind = &b->last;
		(*wind)->next = w;
		w->next = NULL;
		w->prev = *wind;
		*wind = w;
		return;
	}

	wind = b->root ? &(b->root->prev) : &b->last;

	while (*wind)
	{
		if ( *wind != root_window &&
		    ( (!top && (w->window_status & XAWS_SINK)) || !((*wind)->window_status & XAWS_SINK))
		   )
			break;

		wind = &((*wind)->prev);
	}

	if (*wind)
	{
		w->next = (*wind)->next;
		(*wind)->next = w;
		w->prev = *wind;
		if (w->next)
			w->next->prev = w;
		else
			b->last = w;
	}
	else
	{
		if ((w->next = b->first))
			w->next->prev = w;
		else
			b->last = w;

		w->prev = NULL;
		b->first = w;
	}

	if (top || !b->top)
	{
		if (b->top && b->top != w) {
			b->top->window_status &= ~XAWS_FIRST;
			DIAGA(("wi_put_blast: detopped %d of %s", b->top->handle, b->top->owner->name));
		}
		b->top = w;
		w->window_status |= XAWS_FIRST;
		DIAGA(("wi_put_blast: new top %d of %s", b->top->handle, b->top->owner->name));
	}
}

void
wi_move_first(struct win_base *b, struct xa_window *w)
{
	DIAGA(("wi_move_first: %d of %s", w->handle, w->owner->name));

	wi_remove(b, w, false);

	if (!(w->window_status & XAWS_SINK))
		wi_put_first(b, w);
	else
		wi_put_blast(b, w, true, false);
	DIAGA(("wi_move_first: !"));
}

STATIC void
wi_move_blast(struct win_base *b, struct xa_window *w)
{
	DIAGA(("wi_move_blast: %d of %s", w->handle, w->owner->name));

	wi_remove(b, w, false);
	if ((w->window_status & XAWS_FLOAT))
		wi_put_first(b, w);
	else
		wi_put_blast(b, w, false, false);
	DIAGA(("wi_move_blast: !"));
}
STATIC void
wi_move_belowroot(struct win_base *b, struct xa_window *w)
{
	DIAGA(("wi_move_belowroot: %d of %s", w->handle, w->owner->name));
	wi_remove(b, w, false);
	wi_put_blast(b, w, false, true);
	DIAGA(("wi_move_belowroot: !"));
}
/*
 * XXX - Ozk:
 *	Note that the window is not actually hidden until client calls
 *	wind_set(WF_CURRXYWH), but we mark it hidden internally right now,
 *	and it will be skipped in various other places, like rectangle
 *	building, immediately.
 */
void
hide_window(enum locks lock, struct xa_window *wind)
{
	if (!is_hidden(wind))
	{
		RECT r = wind->rc;
		r.x = root_window->rc.x + root_window->rc.w + 16;
		r.y = root_window->rc.y + root_window->rc.h + 16;
		wind->hx = wind->rc.x;
		wind->hy = wind->rc.y;
		if (wind->opts & XAWO_WCOWORK)
			r = f2w(&wind->delta, &r, true);
		send_moved(lock, wind, AMQ_NORM, &r);
		wind->window_status |= XAWS_HIDDEN;
	}
}
void
unhide_window(enum locks lock, struct xa_window *wind, bool check)
{
	if (is_hidden(wind))
	{
		RECT r;

		if (is_iconified(wind))
			r = free_icon_pos(lock, wind);
		else
		{
			r = wind->rc;
			r.x = wind->hx;
			r.y = wind->hy;
		}
		wind->t = r;
		if (wind->opts & XAWO_WCOWORK)
			r = f2w(&wind->delta, &r, true);
		send_moved(lock, wind, AMQ_NORM, &r);
		wind->window_status &= ~XAWS_HIDDEN;
		if (any_hidden(lock, wind->owner, wind))
			set_unhidden(lock, wind->owner);
	}
}

static inline void
movewind_belowroot(struct xa_window *wind)
{
	wi_move_belowroot(&S.open_windows, wind);
	wind->window_status |= XAWS_BELOWROOT;
	clear_wind_rectlist(wind);
}

void
hide_toolboxwindows(struct xa_client *client)
{
	struct xa_window *wind = window_list, *nxt;

	while (wind && wind != root_window)
	{
		nxt = wind->next;
		if (wind->owner == client && (wind->window_status & XAWS_BINDFOCUS))
		{
			struct xa_window *wl = wind->next;
			movewind_belowroot(wind);
// 			wi_move_belowroot(&S.open_windows, wind);
// 			wind->window_status |= XAWS_BELOWROOT;
// 			clear_wind_rectlist(wind);
			update_windows_below(0, &wind->r, NULL, wl, NULL);
		}
		wind = nxt;
	}
	set_winmouse(-1, -1);
}

void
show_toolboxwindows(struct xa_client *client)
{
	struct xa_window *wind = root_window->next, *nxt;
	RECT clip;

	while (wind)
	{
		nxt = wind->next;
		if (wind->owner == client && (wind->window_status & XAWS_BELOWROOT))
		{
			wi_move_first(&S.open_windows, wind);
			wind->window_status &= ~XAWS_BELOWROOT;
			wind->window_status |= XAWS_SEMA;
		}
		wind = nxt;
	}

	wind = window_list;
	while (wind && wind != root_window)
	{
		if (wind->window_status & XAWS_SEMA)
		{
			struct xa_rect_list *rl;

			wind->window_status &= ~XAWS_SEMA;
			make_rect_list(wind, true, RECT_SYS);
			rl = wind->rect_list.start;
			while (rl)
			{
				if (xa_rect_clip(&wind->r, &rl->r, &clip))
					generate_redraws(0, wind, &clip, RDRW_ALL);
				rl = rl->next;
			}
			update_windows_below(0, &wind->r, NULL, wind->next, NULL);
		}
		wind = wind->next;
	}
	set_winmouse(-1, -1);
}

/*
 * ONLY call from from correct context
 */
void
set_window_title(struct xa_window *wind, const char *title, bool redraw)
{
	char *dst = wind->wname;
	XA_WIDGET *widg;


	if (title)
	{
		int i;

		for (i = 0; i < (sizeof(wind->wname)-1) && (*dst++ = *title++); i++)
			;
	}
	*dst = '\0';

	widg = get_widget(wind, XAW_TITLE);
	widg->stuff = wind->wname;

	DIAG((D_wind, wind->owner, "    -   %s", wind->wname));

	/* redraw if necessary */
	if (redraw && (wind->active_widgets & NAME) && ((wind->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN))
	{
		RECT clip;

		rp_2_ap(wind, widg, &clip);
		display_window(0, 45, wind, &clip);
	}
	add_window_to_tasklist( wind, title );
}
void
get_window_title(struct xa_window *wind, char *dst)
{
	if (dst)
	{
		char *src = wind->wname;
		strcpy(dst, src);
	}
}

/*
 * ONLY call from from correct context
 */
void
set_window_info(struct xa_window *wind, const char *info, bool redraw)
{
	char *dst = wind->winfo;
	XA_WIDGET *widg;

	if (info)
	{
		int i;

		for (i = 0; i < (sizeof(wind->winfo)-1) && (*dst++ = *info++); i++)
			;
	}
	*dst = '\0';

	widg = get_widget(wind, XAW_INFO);
	widg->stuff = wind->winfo;

	DIAG((D_wind, wind->owner, "    -   %s", wind->winfo));

	if (redraw && (wind->active_widgets & INFO) && (wind->window_status & (XAWS_OPEN|XAWS_SHADED|XAWS_ICONIFIED|XAWS_HIDDEN)) == XAWS_OPEN)
	{
		RECT clip;

		rp_2_ap(wind, widg, &clip);
		display_window(0, 46, wind, &clip);
	}
}
void
get_window_info(struct xa_window *wind, char *dst)
{
	if (dst)
	{
		char *src = wind->winfo;
		strcpy(dst, src);
	}
}
#if INCLUDE_UNUSED
STATIC void
send_ontop(enum locks lock)
{
	struct xa_window *top = TOP_WINDOW;
	struct xa_client *client = top->owner;

	if (!client->fmd.wind)
	{
		if (top->send_message)
			top->send_message(lock, top, NULL, AMQ_NORM, QMF_CHKDUP,
					  WM_ONTOP, 0, 0, top->handle,
					  0, 0, 0, 0);
	}
}
#endif
void
send_topped(enum locks lock, struct xa_window *wind)
{
	struct xa_client *client = wind->owner;

	if (wind->send_message && (!client->fmd.wind || client->fmd.wind == wind))
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_TOPPED, 0, 0, wind->handle,
				   0, 0, 0, 0);
}
void
send_bottomed(enum locks lock, struct xa_window *wind)
{
	struct xa_client *client = wind->owner;

	if (wind->send_message && (!client->fmd.wind || client->fmd.wind == wind))
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_BOTTOMED, 0, 0, wind->handle,
				   0, 0, 0, 0);
}

void
send_moved(enum locks lock, struct xa_window *wind, short amq, RECT *r)
{
	if (wind->send_message)
	{
		C.move_block = 2;
		wind->send_message(lock, wind, NULL, amq, QMF_CHKDUP,
			WM_MOVED, 0, 0, wind->handle,
			r->x, r->y, r->w, r->h);

	}
}
#define WIND_WMIN	64
#define WIND_HMIN	64
void
send_sized(enum locks lock, struct xa_window *wind, short amq, RECT *r)
{
	if (!(wind->window_status & XAWS_SHADED))
	{
		if (wind->send_message)
		{
			if( r->w < WIND_WMIN )
				r->w = WIND_WMIN;
			if( r->h < WIND_HMIN )
				r->h = WIND_HMIN;
			C.move_block = 2;
			wind->send_message(lock, wind, NULL, amq, QMF_CHKDUP,
				WM_SIZED, 0,0, wind->handle,
				r->x, r->y, r->w, r->h);
		}
	}
}

void
send_reposed(enum locks lock, struct xa_window *wind, short amq, RECT *r)
{
	if (wind->send_message)
	{
		C.move_block = 2;
		wind->send_message(lock, wind, NULL, amq, QMF_CHKDUP,
			WM_REPOSED, 0,0, wind->handle,
			r->x, r->y, r->w, r->h);
	}
}

STATIC void
send_redraw(enum locks lock, struct xa_window *wind, RECT *r)
{
	if (!(wind->window_status & (XAWS_SHADED|XAWS_HIDDEN)) && wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_REDRAW, QMF_CHKDUP,
			WM_REDRAW, 0,0, wind->handle,
			r->x, r->y, r->w, r->h);
	}
}

void
send_vslid(enum locks lock, struct xa_window *wind, short offs)
{
	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
			WM_VSLID, 0,0, wind->handle,
			offs, 0,0,0);
	}
}
void
send_hslid(enum locks lock, struct xa_window *wind, short offs)
{
	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
			WM_HSLID, 0,0, wind->handle,
			offs, 0,0,0);
	}
}

void
send_closed(enum locks lock, struct xa_window *wind)
{
	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_CLOSED, 0, 0, wind->handle,
				   0,0,0,0);

	}
}

void close_window_menu(Tab *tab);
void
setwin_untopped(enum locks lock, struct xa_window *wind, bool snd_untopped)
{
	struct xa_client *client = wind->owner;


	if( wind != root_window && wind->tool && ((XA_WIDGET *)wind->tool)->m.r.xaw_idx == XAW_MENU )
	{
		Tab *tab = TAB_LIST_START;
		if( tab && tab->wind == wind )
		{
			close_window_menu( tab );
			nap(100);
		}
	}

	if (snd_untopped && wind->send_message && (!client->fmd.wind || client->fmd.wind == wind))
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_UNTOPPED, 0, 0, wind->handle,
				   0, 0, 0, 0);
}

void
setwin_ontop(enum locks lock, struct xa_window *wind, bool snd_ontop)
{
	struct xa_client *client = wind->owner;

	if (!client->fmd.wind || (wind == client->fmd.wind))
	{
		if (wind != root_window && snd_ontop && wind->send_message)
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					  WM_ONTOP, 0, 0, wind->handle,
					  0, 0, 0, 0);
	}
}

void
send_iredraw(enum locks lock, struct xa_window *wind, short xaw, RECT *r)
{
	if (wind == root_window)
	{
		if (get_desktop()->owner == C.Aes) {
			display_window(lock, 0, wind, r);
		} else if (r) {
			send_app_message(lock, wind, get_desktop()->owner, AMQ_IREDRAW, QMF_NORM,
					 WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
					 r->x, r->y, r->w, r->h);
		} else {
			send_app_message(lock, wind, get_desktop()->owner, AMQ_IREDRAW, QMF_NORM,
					 WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
					 0,0,0,0);
		}
	}
	else
	{
		if (r)
		{
 			if (!is_inside(r, &wind->rwa))
 			{
				send_app_message(lock, wind, NULL, AMQ_IREDRAW, QMF_NORM,
					WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
					r->x, r->y, r->w, r->h);
			}
		}
		else
		{
			send_app_message(lock, wind, NULL, AMQ_IREDRAW, QMF_NORM,
				WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
				0,0,0,0);
		}
	}
}

void
generate_redraws(enum locks lock, struct xa_window *wind, RECT *r, short flags)
{
	RECT b;

	DIAGS(("generate_redraws: on %d of %s", wind->handle, wind->owner->name));

	if ((wind->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN)
	{
		if (wind != root_window && r && (flags & RDRW_WA) && !is_shaded(wind))
		{
			struct xa_widget *widg;
			if (xa_rect_clip(&wind->rwa, r, &b))
			{
				send_redraw(lock, wind, &b);
			}

			if ((widg = usertoolbar_installed(wind)) )
			{
				if (xa_rect_clip(&widg->ar, r, &b))
				{
					send_app_message(lock, wind, NULL, AMQ_IREDRAW, QMF_NORM,
						WM_REDRAW, widg->m.r.xaw_idx, ((long)wind) >> 16, ((long)wind) & 0xffff,
							b.x, b.y, b.w, b.h);
				}
			}
		}
		if ((flags & RDRW_EXT))
		{
			send_iredraw(lock, wind, 0, r);
		}
	}
	DIAGS(("generate_redraws: !"));
}
static void
remove_from_iredraw_queue(enum locks lock, struct xa_window *wind)
{
	struct xa_aesmsg_list **msg, *free;

	msg = &wind->owner->irdrw_msg;

	while (*msg)
	{
		if ((*msg)->message.irdrw.ptr == wind)
		{
			free = *msg;
			*msg = (*msg)->next;
			kfree(free);
			kick_mousemove_timeout();
		}
		else
			msg = &(*msg)->next;
	}
}

void
iconify_window(enum locks lock, struct xa_window *wind, RECT *r)
{
	if ((r->w == -1 && r->h == -1) || (!r->w && !r->h) || (r->y + r->h + cfg.icnfy_b_y != screen.r.h))
		*r = free_icon_pos(lock, NULL);

	move_window(lock, wind, true, XAWS_ICONIFIED, r->x, r->y, r->w, r->h);
	wind->window_status &= ~XAWS_CHGICONIF;
}

void
uniconify_window(enum locks lock, struct xa_window *wind, RECT *r)
{
	move_window(lock, wind, true, ~XAWS_ICONIFIED, r->x, r->y, r->w, r->h);
	wind->window_status &= ~XAWS_CHGICONIF;

	/* reinstall the menu - maybe theres a better place for this */
	if( wind->active_widgets & XaMENU )
	{
		XA_WIDGET *widg = get_widget( wind, XAW_MENU );
		if( widg && widg->stuff )
			set_menu_widget( wind, wind->owner, widg->stuff );
	}

	/*
	 * re-install toolbar to achieve correct vslider
	 */
	if( (wind->active_widgets & TOOLBAR) && (wind->active_widgets & VSLIDE) )
	{
		XA_WIDGET *widg = get_widget( wind, XAW_TOOLBAR );
		AESPB pb;
		short intin[8], intout[8];

		intin[0] = wind->handle;
		intin[1] = WF_TOOLBAR;
		ptr_to_shorts( ((XA_TREE*)widg->stuff)->tree, intin + 2 );
		pb.intin = intin;
		pb.intout = intout;

		/* install toolbar */
		XA_wind_set( lock, wind->owner, &pb );
	}

	set_reiconify_timeout(lock);
}

void _cdecl
top_window(enum locks lock, bool snd_untopped, bool snd_ontop, struct xa_window *w)
{
	DIAG((D_wind, NULL, "top_window %d for %s",  w->handle, w == root_window ? get_desktop()->owner->proc_name : w->owner->proc_name));
	if (w == NULL || w == root_window || (S.focus == w) )
		return;

	if (w->nolist)
	{
		setnew_focus(w, NULL, false, snd_ontop, false);
	}
	else
	{
#if WITH_BBL_HELP
		if( w != bgem_window && !(w->dial == (created_for_ALERT | created_for_AES)) )
		{
	if( !C.boot_focus )
			xa_bubble( 0, bbl_close_bubble1, 0, 2 );
		}
#endif
		pull_wind_to_top(lock, w);
		setnew_focus(w, NULL, true, snd_untopped, snd_ontop);
	}
	set_winmouse(-1, -1);
}

void _cdecl
bottom_window(enum locks lock, bool snd_untopped, bool snd_ontop, struct xa_window *w)
{
	bool was_top = (is_topped(w) ? true : false);
	struct xa_window *wl = w->next, *new_focus;

	DIAG((D_wind, w->owner, "bottom_window %d", w->handle));

	if (w == w->owner->fmd.wind && w->dial == created_for_FMD_START)
		return;	// modal dialog
	if (w == root_window || wl == root_window)
		/* Don't do anything if already bottom */
		return;

	if (w->nolist)
		unset_focus(w);
	else
	{
		/* Send it to the back */
		send_wind_to_bottom(lock, w);
		if (was_top)
		{
			if (!(TOP_WINDOW->window_status & XAWS_NOFOCUS))
				new_focus = TOP_WINDOW;
			else
			{
				new_focus = window_list;
				while (new_focus && new_focus != root_window)
				{
					if (!(new_focus->window_status & XAWS_NOFOCUS))
						break;
					new_focus = new_focus->next;
				}
			}
			setnew_focus(new_focus, w, true, snd_untopped, snd_ontop);
		}

		DIAG((D_wind, w->owner, " - menu_owner %s, w_list_owner %s",
			t_owner(get_menu()), w_owner(window_list)));

		update_windows_below(lock, &w->r, NULL, wl, w);
	}
}

STATIC XA_WIND_ATTR
fix_wind_kind(XA_WIND_ATTR tp)
{
	/* avoid confusion: if only 1 specified, give both (fail safe!) */
	if (tp & (UPARROW|DNARROW))
		tp |= UPARROW|DNARROW|UPARROW1;
	else
		tp &= ~UPARROW1;

	if (tp & (LFARROW|RTARROW))
		tp |= LFARROW|RTARROW|LFARROW1;
	else
		tp &= ~LFARROW1;

	/* cant hide a window that cannot be moved. */
	if (!(tp & MOVER))
		tp &= ~HIDE;
	/* temporary until solved. */
	if (tp & MENUBAR)
		tp |= XaMENU;

	return tp;
}

/*
 * Create a window
 *
 * introduced pid -1 for temporary windows created basicly only to be able to do
 * calc_work_area and center.
 *
 * needed a dynamiccally sized frame
 *
 * ? what if older prgs set HOTCLOSEBOX which is in fact MENUBAR ?
 */
struct xa_window * _cdecl
create_window(
	enum locks lock,
	SendMessage *message_handler,
	DoWinMesag *message_doer,
	struct xa_client *client,
	bool nolist,
	XA_WIND_ATTR tp,
	WINDOW_TYPE dial,
	int frame,
	bool thinwork,
	const RECT R,
	const RECT *max,
	RECT *remember)
{
	struct xa_window *w;
	struct xa_widget_theme *wtheme;
	RECT r;

	r = R;


#if GENERATE_DIAGS
	if (max)
	{
		DIAG((D_wind, client, "create_window for %s: r:%d,%d/%d,%d  max:%d,%d/%d,%d",
			c_owner(client), r.x,r.y,r.w,r.h,max->x,max->y,max->w,max->h));
	}
	else
	{
		DIAG((D_wind, client, "create_window for %s: r:%d,%d/%d,%d  no max",
			c_owner(client), r.x,r.y,r.w,r.h));
	}

#endif

	w = kmalloc(sizeof(*w));
	if (!w)	/* Unable to allocate memory for window? */
		return NULL;

	init_client_widget_theme(client);

	if (!(wtheme = client->widget_theme))
	{
		kfree(w);
		return NULL;
	}

	bzero(w, sizeof(*w));

	{
		int i;
		for (i = 0; i < XA_MAX_WIDGETS; i++)
			w->widgets[i].wind = w;
	}

	w->vdi_settings = client->vdi_settings;

	w->min.h = w->min.w = WIND_WMIN;
	tp = fix_wind_kind(tp);
	w->requested_widgets = tp;

	/* implement maximum rectangle (needed for at least TosWin2) */
// 	w->max = max ? *max : root_window->wa;

	if (tp & (UPARROW|DNARROW|LFARROW|RTARROW))
	{
		w->min.x = w->min.y = -1;
		w->min.w = 6 * 16; //cfg.widg_w;
		w->min.h = 7 * 16; //cfg.widg_h;
	}
	else
		w->min.w = w->min.h = 0;

	if (tp & (MENUBAR))
		w->min.h += 20;
	if (tp & (INFO))
		w->min.h += 10;

	w->rc = w->r = w->pr = r;

	{
		long opts = client->options.wind_opts;

		if (client->options.naes_ff)
			opts |= XAWO_FULLREDRAW;
		w->opts = opts;
	}

// 	w->widget_theme = client->widget_theme;

	if (dial & created_for_ALERT)
	{
		w->class = WINCLASS_ALERT;
		w->active_theme = client->widget_theme->alert;
		w->window_status |= XAWS_FLOAT;
	}
	else if (dial & created_for_POPUP)
	{
		w->class = WINCLASS_POPUP;
		w->active_theme = client->widget_theme->popup;
		w->window_status |= XAWS_FLOAT|XAWS_NOFOCUS;
	}
	else if (dial & created_for_SLIST)
	{
		w->class = WINCLASS_SLIST;
		w->active_theme = client->widget_theme->slist;
		w->window_status |= XAWS_NOFOCUS;
	}
	else
	{
		w->class = WINCLASS_CLIENT;
		w->active_theme = client->widget_theme->client;
	}

	w->active_theme->links++;

	(*client->xmwt->new_color_theme)(client->wtheme_handle, w->class, &w->ontop_cols, &w->untop_cols);
	w->colours = w->ontop_cols;

	w->wheel_mode = client->options.wheel_mode;
	w->frame = frame;
	w->thinwork = MONO ? true : thinwork;
	w->owner = client;
	w->handle = -1;
	w->nolist = nolist;
	w->dial = dial;
	w->send_message = message_handler;
	w->do_message	= message_doer;
	get_widget(w, XAW_TITLE)->stuff = client->name;

	/*if (dial & created_for_POPUP)
	{
		w->x_shadow = 1;
		w->y_shadow = 1;
	}
	else
	*/
	{
		w->x_shadow = 1; //2;
		w->y_shadow = 1; //2;
	}
	w->wa_frame = true;

// 	if (w->frame > 0)
// 		tp |= BORDER;

	if (nolist)
	{
		if (!(w->dial & (created_for_SLIST | created_for_CALC)))
			wi_put_first(&S.closed_nlwindows, w);
		/* Attach the appropriate widgets to the window */
		DIAGS((" -- nolist window"));
		standard_widgets(w, tp, false);
	}
	else
	{

		tp &= ~STORE_BACK;

		w->handle = new_wind_handle();
		DIAG((D_wind, client, " allocated handle = %d", w->handle));

		wi_put_first(&S.closed_windows, w);
		DIAG((D_wind,client," inserted in closed_windows list"));

		/* Attach the appropriate widgets to the window */
		standard_widgets(w, tp, false);
#if INCLUDE_UNUSED
		/* If STORE_BACK extended attribute is used, window preserves its own background */
		if (tp & STORE_BACK)
		{
			DIAG((D_wind,client," allocating background storage buffer"));

			/* XXX the store back code is not failsafe if kmalloc fail */

			w->background = kmalloc(calc_back(&r, screen.planes));
			assert(w->background);
		}
		else
#endif
			w->background = NULL;

	}
	calc_work_area(w);

	if (max)
	{
		if (w->opts & XAWO_WCOWORK)
		{
			w->max = w2f(&w->delta, max, true);
		}
		else
		{
			w->max = *max;
		}
		fitin_root(&w->max);
	}
	else
		w->max = root_window->wa;

	calc_shade_height(w);
#if 0
	if (tp & (CLOSER|NAME|MOVER|ICONIFIER|FULLER))
	{
		RECT t;
		t = w2f(&w->rbd, &w->widgets[XAW_TITLE].r, true);
		w->sh = t.h + w->y_shadow;
	}
#endif
	if (remember)
		*remember = w->r;

	return w;
}

void
change_window_attribs(enum locks lock,
		struct xa_client *client,
		struct xa_window *w,
		XA_WIND_ATTR tp,
		bool r_is_wa,
		bool insideroot,
		short noleft,
		RECT r, RECT *remember)
{
	XA_WIND_ATTR old_tp = w->active_widgets;

	DIAG((D_wind, client, "change_window_attribs for %s: r:%d,%d/%d,%d  no max",
		c_owner(client), r.x,r.y,r.w,r.h));

	tp = fix_wind_kind(tp);

	standard_widgets(w, tp, false);

	if (r_is_wa)
	{
		w->r = calc_window(lock, client, WC_BORDER,
				 tp, w->dial,
				 client->options.thinframe,
				 client->options.thinwork,
				 *(RECT *)&r);
	}
	else
		w->r = r;

	if (insideroot)
	{
		bool nl;
		switch (noleft)
		{
			case 1:
				nl = false;	break;
			case 2:
				nl = true;	break;
			case 0:
			default:
				nl = w->owner->options.noleft;
				break;
		}
		inside_root(&w->r, nl);
	}

	w->rc = w->r;

	if ((w->window_status & XAWS_SHADED))
		w->r.h = w->sh;

	calc_work_area(w);

	clear_wind_rectlist(w);

	/* If STORE_BACK extended attribute is used, window preserves its own background */
	if (!(tp & STORE_BACK))
	{
		if ((old_tp & STORE_BACK) && w->background)
		{
			DIAG((D_wind,client," allocating background storage buffer"));
			kfree(w->background);
			w->background = NULL;
		}
	}
	else if (!(old_tp & STORE_BACK) && !w->background)
	{
		/* XXX the store back code is not failsafe if kmalloc fail */

		w->background = kmalloc(calc_back(&r, screen.planes));
		assert(w->background);
	}

	calc_shade_height(w);

	/*
	 * Now standard widgets are set and workarea of window
	 * is recalculated.
	 * Its time to do non-standard things depending on correct
	 * workarea coordinates now.
	 */
	{
		struct widget_tree *wt;
		if ((wt = get_widget(w, XAW_TOOLBAR)->stuff))
		{
			set_toolbar_coords(w, NULL);
			if (wt->tree && client->p == get_curproc())
			{
				wt->tree->ob_x = w->wa.x;
				wt->tree->ob_y = w->wa.y;
				if (!wt->zen)
				{
					wt->tree->ob_x += wt->ox;
					wt->tree->ob_y += wt->oy;
				}
			}
		}
	}

	if (remember)
		*remember = w->r;
}

void remove_window_widgets(enum locks lock, int full)
{
	struct xa_window *wind = TOP_WINDOW;
	if( wind && !(wind->window_status & XAWS_ICONIFIED) )
	{
		XA_WIND_ATTR	active_widgets;
		if( !(wind->window_status & XAWS_RM_WDG) )
		{
			wind->save_widgets = wind->active_widgets;
			active_widgets = 0;	// todo: uninstall menu/toolbar
			wind->window_status |= XAWS_RM_WDG;
			if( full )
			{
				wind->ro = wind->r;
				wind->r = screen.r;
			}
		}
		else
		{
			wind->window_status &= ~XAWS_RM_WDG;
			active_widgets = wind->save_widgets;	// todo: re-install menu/toolbar
			if( full )
				wind->r = wind->ro;
		}
		change_window_attribs(lock, wind->owner, wind, active_widgets, false, false, 0, wind->r, NULL);
		if( full )
		{
			wind->send_message(lock, wind, NULL, AMQ_REDRAW, QMF_CHKDUP,
		  	WM_SIZED, 0, 0, wind->handle, wind->r.x, wind->r.y, wind->r.w, wind->r.h);
		}
		if( memcmp( &wind->r, &screen.r, sizeof(RECT) ) )
		{
			update_windows_below(lock, &screen.r, NULL, window_list, NULL);
		}
	}
}

int _cdecl
open_window(enum locks lock, struct xa_window *wind, RECT r)
{
	struct xa_window *wl = window_list;
	RECT clip, our_win;
	int ignorefocus = 0;

	DIAG((D_wind, wind->owner, "open_window %d for %s to %d/%d,%d/%d status %lx",
		wind->handle, c_owner(wind->owner), r.x,r.y,r.w,r.h, wind->window_status));

	if (C.redraws) {
		yield();
	}

	if ((wind->window_status & XAWS_OPEN))
	{
		/* The window is already open, no need to do anything */

		DIAGS(("WARNING: Attempt to open window when it was already open"));
		set_active_client(lock, wind->owner);
		swap_menu(lock|desk, wind->owner, NULL, SWAPM_DESK); // | SWAPM_TOPW);

		return 0;
	}

#if WITH_BBL_HELP
	if( wind != bgem_window && wind->handle >= 0
		&& !(wind->dial == (created_for_ALERT | created_for_AES | created_for_WDIAL))
		&& !C.boot_focus )
	{
		xa_bubble( 0, bbl_close_bubble1, 0, 3 );
	}
#endif
	if (wind->nolist || (wind->dial & created_for_SLIST))
	{
		DIAGS(("open_window: nolist window - SLIST wind? %s",
			(wind->dial & created_for_SLIST) ? "yes":"no"));

		if (wind != root_window && !(wind->dial & (created_for_POPUP | created_for_SLIST)))
		{
			inside_root(&r, wind->owner->options.noleft);
			inside_minmax(&r, wind);
		}

		wind->rc = wind->r = r;

		calc_work_area(wind);

		if ((wind->window_status & XAWS_SHADED))
			wind->r.h = wind->sh;

		wind->window_status |= XAWS_OPEN;

		make_rect_list(wind, true, RECT_SYS);

		if (wind->active_widgets & STORE_BACK)
		{
			(*xa_vdiapi->form_save)(0, wind->r, &(wind->background));
		}

		if (!(wind->dial & created_for_POPUP))
			move_ctxdep_widgets(wind);

		if (!(wind->dial & created_for_SLIST))
		{
			wi_remove(&S.closed_nlwindows, wind, false);
			wi_put_first(&S.open_nlwindows, wind);

			if (!(wind->active_widgets & STORE_BACK))
			{
				our_win = wind->r;

				wl = wind->next;
				if (!wl)
					wl = window_list;

				while (wl)
				{
					clip = wl->r;

					if (xa_rc_intersect(our_win, &clip))
						make_rect_list(wl, true, RECT_SYS);

					if (!wl->next && wl->nolist)
						wl = window_list;
					else
						wl = wl->next;

					if (wl == root_window)
						break;
				}
			}

			if (!(wind->window_status & XAWS_NOFOCUS))
				wind->colours = wind->untop_cols;

			generate_redraws(lock, wind, &wind->r, RDRW_ALL);
		}
		/* dont open unlisted windows */
		return 1;
	}

	wi_remove(&S.closed_windows, wind, false);
	if( (C.boot_focus && wind->owner->p != C.boot_focus))
		ignorefocus = 2;
	else if( (wind->window_status & XAWS_NOFOCUS))
		ignorefocus = 1;

	if (wind != root_window && (wind->window_status & XAWS_BINDFOCUS) && get_app_infront() != wind->owner)
	{
		wi_put_blast(&S.open_windows, wind, false, true);
		wind->window_status |= XAWS_BELOWROOT;
	}
	else {
		wi_put_first(&S.open_windows, wind);
	}
	/* New top window - change the cursor to this client's choice */
	xa_graf_mouse(wind->owner->mouse, wind->owner->mouse_form, wind->owner, false);

	if (is_iconified(wind))
	{
		if (wind != root_window && !(wind->dial & created_for_POPUP))
			inside_root(&r, wind->owner->options.noleft);

		if (r.w != -1 && r.h != -1)
			wind->rc = wind->r = r;

		if ((wind->window_status & XAWS_SHADED))
			wind->r.h = wind->sh;

	}
	else
	{
		/* Change the window coords */
		if (wind != root_window && !(wind->dial & created_for_POPUP))
		{
			inside_root(&r, wind->owner->options.noleft);
			//inside_minmax(&r, wind);
		}
		wind->rc = wind->r = r;
		if ((wind->window_status & XAWS_SHADED))
		{
			wind->r.h = wind->sh;

			if (wind->send_message) {
				wind->send_message(lock, wind, NULL, AMQ_CRITICAL, QMF_CHKDUP,
					WM_SHADED, 0,0, wind->handle, 0,0,0,0);
			}
		}
	}
	wind->window_status |= XAWS_OPEN;

	calc_work_area(wind);

	if (!ignorefocus && wl && (wl == root_window || wl->owner != wind->owner || !is_infront(wind->owner)))
	{
		DIAG((D_appl, NULL, "open_window: Change infront app from %s to %s",
			wl->owner->name, wind->owner->name));

		set_active_client(lock, wind->owner);
		swap_menu(lock|desk, wind->owner, NULL, SWAPM_DESK );
	}
	make_rect_list(wind, true, RECT_SYS);
	/*
	 * Context dependant widgets must always follow the window
	 * movements
	 */
	move_ctxdep_widgets(wind);

	/* Is this a 'preserve own background' window? */
	if (wind->active_widgets & STORE_BACK)
	{
		if (!(wind->window_status & XAWS_BELOWROOT))
		{
			(*xa_vdiapi->form_save)(0, wind->r, &(wind->background));
			/* This is enough, it is only for TOOLBAR windows. */
			generate_redraws(lock, wind, &wind->r, RDRW_ALL);
		}
	}
	else if (!(wind->window_status & XAWS_BELOWROOT))
	{
		struct xa_rect_list *rl;

		wind->colours = wind->untop_cols;

		our_win = wind->r;

		wl = wind->next;
		while (wl)
		{
			clip = wl->r;

			if (xa_rc_intersect(our_win, &clip))
				make_rect_list(wl, true, RECT_SYS);

			if (wl == root_window)
				break;
			wl = wl->next;
		}

		if( ignorefocus != 2 )
			setnew_focus(wind, S.focus, true, true, false);

		/* avoid second redraw (see setnew_focus) */
		if( wind->dial != created_for_FMD_START )
		{
			/* Display the window using clipping rectangles from the rectangle list */
			rl = wind->rect_list.start;
			while (rl)
			{
				if (xa_rect_clip(&wind->r, &rl->r, &clip)) {
					generate_redraws(lock, wind, &clip/*&wind->r*/, RDRW_ALL);
				}
				rl = rl->next;
			}
		}
		if( C.boot_focus )
		{
			struct xa_window *w;
			if( ignorefocus == 2 )
				w = S.focus;
			else
				w = wind;
			S.focus = 0;
			if( w )
				top_window(lock, true, true, w);
				if( w != wind )
					wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP, WM_UNTOPPED, 0, 0, wind->handle, 0,0,0,0);
		}
	}
	else
	{
		setnew_focus(wind, S.focus, true, true, false);
	}

	DIAG((D_wind, wind->owner, "open_window %d for %s exit with 1",
		wind->handle, c_owner(wind->owner)));

	set_winmouse(-1, -1);

	add_window_to_tasklist( wind, NULL );
	return 1;
}

bool clip_off_menu( RECT *cl )
{
	short d = get_menu_height() - cl->y;
	if( cl->h <= d )
		return true;
	cl->y = get_menu_height();
	cl->h -= d;
	return false;
}

void
draw_window(enum locks lock, struct xa_window *wind, const RECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	RECT cl = *clip;

	DIAG((D_wind, wind->owner, "draw_window %d for %s to %d/%d,%d/%d",
		wind->handle, w_owner(wind),
		wind->r.x, wind->r.y, wind->r.w, wind->r.h));

	if (!(wind->window_status & XAWS_OPEN) || (wind->dial & created_for_MENUBAR) )
	{
		DIAG((D_wind, wind->owner, "Dwindow %d for %s not open",
			wind->handle, w_owner(wind)));
		return;
	}

	/* is window below menubar? (better should change rectlists?) */
	if( wind->handle >= 0 && cfg.menu_layout == 0 && cfg.menu_bar && cl.y < get_menu_height() )
	{
		if( clip_off_menu( &cl ) )
			return;
	}

	(*v->api->set_clip)(v, &cl);
	(*v->api->l_color)(v, G_BLACK);

	hidem();

	/* Dont waste precious CRT glass */
	if (wind != root_window)
	{
		(*v->api->f_color)(v, objc_rend.dial_colours.bg_col);
		(*v->api->f_interior)(v, FIS_SOLID);

		if (wind->draw_waframe)
			(*wind->draw_waframe)(wind, clip);

		if (wind->frame >= 0 && (wind->x_shadow || wind->y_shadow))
		{
			//shadow_object(0, OS_SHADOWED, &cl, G_BLACK, wind->shadow/2); //SHADOW_OFFSET/2);
			shadow_area(v, 0, OS_SHADOWED, &wind->r, G_BLACK, wind->x_shadow, wind->y_shadow);
		}
	}

	/* If the window has an auto-redraw function, call it
	 * do this before the widgets. (some programs supply x=0, y=0)
	 */
	/* XXX - Ozk: Extremely not good if draw_window() is to be context indipendant!!!
	 */
	if (wind->redraw)
	{
		wind->redraw(lock, wind);
	}

	{
		int f;
		WINDOW_STATUS status = wind->window_status;
		XA_WIDGET *widg;
		RECT r;

		if (wind->draw_canvas)
		{
			(*wind->draw_canvas)(wind, &wind->outer, &wind->inner, clip);
		}

		for (f = 0; f < XA_MAX_WIDGETS; f++)
		{
			widg = get_widget(wind, f);

//  			if (f == XAW_TOOLBAR && wind != root_window && !(wind->dial & created_for_SLIST))
//  				display("toolbar statusmask %x, properties %x, draw %lx",
//  					widg->m.statusmask, widg->m.properties, widg->m.r.draw);


			if ( (wind != root_window && f == XAW_TOOLBAR) ||
			     (widg->m.properties & WIP_NOTEXT) ||
			     (f == XAW_MENU && wind == root_window))
			{
				continue;
			}

			if ( !(status & widg->m.statusmask) )
			{
				if( wdg_is_inst(widg) )
				{
					DIAG((D_wind, wind->owner, "draw_window %d: display widget %d (func=%lx)",
						wind->handle, f, widg->m.r.draw));

					if (widg->m.properties & WIP_WACLIP)
					{
						if (xa_rect_clip(clip, &wind->wa, &r))
						{
							(*v->api->set_clip)(v, &r);
	 						//if (f == XAW_TOOLBAR) display("drawing toolbar (waclip) for %s,w=%d", wind->owner->name, wind->wa.w);
							(*widg->m.r.draw)(wind, widg, &r);
							(*v->api->set_clip)(v, clip);
						}
					}
					else
					{
	 					//if (f == XAW_TOOLBAR) display("drawing toolbar for %s", wind->owner->name);
						widg->m.r.draw(wind, widg, clip );
					}
				}
				else if( f == XAW_MENU && widg->r.w && widg->r.h )	/* draw a bar to avoid transparence */
				{
						(*v->api->gbar)(v, 0, &widg->ar);
				}
				//else if( wind->wname[0] )
				// BLOG((0,"%s: widget %d not installed", wind->wname, f ));
			}
		}
	}

	showm();

	DIAG((D_wind, wind->owner, "draw_window %d for %s exit ok",
		wind->handle, w_owner(wind)));
}

struct xa_window *
find_window(enum locks lock, short x, short y, short flags)
{
	struct xa_window *w = NULL;

	if (flags & FNDW_NOLIST)
	{
		w = nolist_list;
		while (w)
		{
			if (!(w->owner->status & CS_EXITING) && m_inside(x, y, &w->r))
			{
				return w;
			}
			w = w->next;
		}
	}

	if (flags & FNDW_NORMAL)
	{
		w = window_list;
		while (w)
		{
			if (!(w->owner->status & CS_EXITING) && m_inside(x, y, &w->r))
			{
				break;
			}
			if (w == root_window)
			{
				w = NULL;
				break;
			}
			w = w->next;
		}
	}

	return w;
}

struct xa_window *
get_wind_by_handle(enum locks lock, short h)
{
	struct xa_window *w;

	w = window_list;
	while (w)
	{
		if (w->handle == h)
			break;

		w = w->next;
	}

	if (!w)
	{
		w = S.closed_windows.first;
		while (w)
		{
			if (w->handle == h)
				break;

			w = w->next;
		}
	}

	return w;
}
/*
 * Pull this window to the head of the window list
 */
STATIC struct xa_window *
pull_wind_to_top(enum locks lock, struct xa_window *w)
{
	struct xa_window *wl, *below, *above;
	RECT clip, r;

	DIAG((D_wind, w->owner, "pull_wind_to_top %d for %s", w->handle, w_owner(w)));

	if (w == root_window) /* just a safeguard */
	{
		make_rect_list(w, true, RECT_SYS);
	}
	else if (!(w->owner->status & CS_EXITING))
	{
		below = w->next;
		above = w->prev;
		r = w->r;

		wi_move_first(&S.open_windows, w);
		wl = window_list;

		while (wl)
		{
			if (wl == w)
			{
				if (w->window_status & XAWS_BELOWROOT)
					wl = root_window;
				else
					wl = above;
				while (wl && wl != w)
				{
					clip = wl->r;
					if (xa_rc_intersect(r, &clip))
						make_rect_list(wl, true, RECT_SYS);
					wl = wl->prev;
				}
				set_and_update_window(w, true, false, NULL);
				break;
			}
			else if (wl == below)
			{
				while (wl)
				{
					clip = wl->r;
					if (xa_rc_intersect(r, &clip))
						make_rect_list(wl, true, RECT_SYS);
					if (wl == w)
						break;
					wl = wl->next;
				}
				update_windows_below(lock, &r, NULL, below, w);
				break;
			}
			wl = wl->next;
		}
	}
	return w;
}

void _cdecl
send_wind_to_bottom(enum locks lock, struct xa_window *w)
{
	struct xa_window *wl;
	RECT r, clip;

	if (   w->next == root_window	/* Can't send to the bottom a window that's already there */
	    || w       == root_window	/* just a safeguard */
	    || !(w->window_status & XAWS_OPEN))
		return;

	wl = w->next;
	r = w->r;

	wi_move_blast(&S.open_windows, w);

	while (wl)
	{
		clip = wl->r;
		if (xa_rc_intersect(r, &clip))
			make_rect_list(wl, true, RECT_SYS);
		wl = wl->next;
	}
	make_rect_list(w, true, RECT_SYS);
}

static void redraw_menu_area(void)
{
	RECT r = screen.r;
	popout(TAB_LIST_START);
	r.h = get_menu_height();

	if( cfg.menu_layout || !cfg.menu_bar )
	{
		RECT rc = screen.r;
		short mb = cfg.menu_bar;
		cfg.menu_bar = 0;
		if( mb )
		{
			rc.x = get_menu_widg()->r.w;
			rc.w -= rc.x;
		}
		rc.h = r.h;
		update_windows_below(0, &rc, &rc, window_list, NULL);
		cfg.menu_bar = mb;
		if( mb )
			redraw_menu(0);
	}
}
#if 0
static void print_rect_list( struct xa_window *wind )
{
	struct xa_rect_list *rl = wind->rect_list.start;
	int i;
	BLOG((0, "print_rect_list:%s", wind->wname));
	for( i = 0; rl; i++, rl = rl->next )
	{
		BLOG((0, "%2d:%d/%d/%d/%d", i, rl->r));
	}
}
#endif
/*
 * set point-size for main-menu
 * adjust root-window-size
 * also used to switch menubar on/off
 */
void set_standard_point(struct xa_client *client)
{
	static int old_menu_bar = -1;
	short w, h;
	bool new_menu_sz = true;
	struct xa_widget *xaw = get_menu_widg(), *xat = get_widget(root_window, XAW_TOOLBAR);
	XA_TREE *wt = xat->stuff;
	struct xa_vdi_settings *v = client->vdi_settings;

	if( C.boot_focus && client->p != C.boot_focus)
		return;
	if( (cfg.menu_ontop && !menu_window) )	//|| !client->std_menu )
	{
		new_menu_sz = false;
	}
#if 1
	/* kludge: if < 1 set menu_bar=0, if != -1 set -standard_font_point */
	if( cfg.menu_bar != 2 && client->options.standard_font_point < 0 )
	{
		client->options.standard_font_point = -client->options.standard_font_point;
		if( client->options.standard_font_point == 1 )
			client->options.standard_font_point = cfg.standard_font_point;
		cfg.menu_bar = 0;
	}
#endif
	(*v->api->t_font)(client->vdi_settings, client->options.standard_font_point, cfg.font_id);
	(*v->api->t_extent)(client->vdi_settings, "X", &w, &h );

	screen.c_max_w = w;
	screen.standard_font_point = client->options.standard_font_point;
	if( new_menu_sz == true && cfg.menu_bar != 2 && cfg.menu_layout == 1 && client->std_menu )
	{
		xaw->r.w = xaw->ar.w = client->std_menu->area.w + 1;
		//print_rect_list( root_window );
		if( !cfg.menu_ontop )
			redraw_menu_area();
	}
	if( h == screen.c_max_h && (cfg.menu_bar == 2 || (cfg.menu_bar == old_menu_bar && cfg.menu_layout == 0)) )
	{
		return;
	}

	old_menu_bar = cfg.menu_bar;

	screen.standard_font_height = v->font_h;
	screen.c_max_h = h;
	if( new_menu_sz == true )
	{
		C.Aes->std_menu->tree->ob_height = h + 2;
		xaw->r.h = xaw->ar.h = xat->r.h = xat->ar.h = h + 2;

		if( cfg.menu_bar == 2 || (cfg.menu_bar == 1 && !cfg.menu_layout && !cfg.menu_ontop) )
		{
			root_window->wa.h = screen.r.h - xaw->r.h;
			root_window->wa.y = xaw->r.h;
		}
		else
		{
			root_window->wa.h = screen.r.h;
			root_window->wa.y = 0;
		}
		if( menu_window && cfg.menu_bar != 2 && cfg.menu_ontop && cfg.menu_bar )
		{
			menu_window->r.w = xaw->r.w;
			menu_window->r.h = xaw->r.h;
			if( menu_window->window_status & XAWS_OPEN)
			{
				move_window( 0, menu_window, true, 0, menu_window->r.x, menu_window->r.y, menu_window->r.w, menu_window->r.h );
				redraw_menu_area();
			}
		}

		//RECT rc = screen.r;
		//update_windows_below(0, &rc, &rc, window_list, NULL);
		root_window->rwa = root_window->wa;

		if (get_desktop()->owner == C.Aes)
		{
			wt->tree->ob_height = root_window->wa.h;
			wt->tree->ob_y = root_window->wa.y;
		}
	}
}

/*
 * md=-2: off if was -1
 * md=-1: on temp.
 * else: set md
 */
void toggle_menu(enum locks lock, short md)
{
	struct xa_client *client = menu_owner();

	if( cfg.menu_bar == 2 )
		return;
	if( !client )
		return;
	if( md == -2 )
	{
		if( cfg.menu_bar == -1 )
			md = 0;
		else return;
	}
	if( md != -1 ){
		if( cfg.menu_bar == md )
		{
			redraw_menu(lock);
			return;
		}
		cfg.menu_bar = md;
	}
	else
		if( ++cfg.menu_bar > 1 )
			cfg.menu_bar = 0;

	set_standard_point( client );
	if( cfg.menu_bar )
	{
#if WITH_BBL_HELP
		xa_bubble( 0, bbl_close_bubble1, 0, 4 );
#endif
		if( cfg.menu_ontop )
		{
			open_window(0, menu_window, menu_window->r );
		}
		redraw_menu(lock);
	}
	else
	{
		if( cfg.menu_ontop )
		{
			close_window(0, menu_window);
		}
		redraw_menu_area();
	}

}

/*
 * Change an open window's coordinates, updating rectangle lists as appropriate
 */
void _cdecl
move_window(enum locks lock, struct xa_window *wind, bool blit, WINDOW_STATUS newstate, short X, short Y, short W, short H)
{
	IFDIAG(struct xa_client *client = wind->owner;)
	RECT old, new;

	DIAG((D_wind,client,"move_window(%s) %d for %s from %d/%d,%d/%d to %d/%d,%d,%d",
	      (wind->window_status & XAWS_OPEN) ? "open" : "closed",
	      wind->handle, c_owner(client), wind->r.x,wind->r.y,wind->r.w,wind->r.h, X,Y,W,H));

	if (wind->owner->status & CS_EXITING)
		return;

	new.x = X;
	new.y = Y;
	new.w = W;
	new.h = H;
	old = wind->r;

	if( BELOW_FOREIGN_MENU(old.y) )
	{
		blit = false;
	}

	if (!wind->nolist && C.redraws)
		yield();

	if (newstate != -1L)
	{
		blit = false;
		if (newstate & XAWS_SEMA)
		{
			if (!(newstate & XAWS_ICONIFIED))
			{
				if (is_iconified(wind))
				{
					DIAGS(("move_window: Clear iconified"));
					standard_widgets(wind, wind->save_widgets, true);
#if GENERATE_DIAGS
					if ((wind->window_status & XAWS_SHADED))
					{
						DIAGS(("move_window: %d/%d/%d/%d - uniconify shaded window",
							new));
					}
#endif
				}
			}
#if GENERATE_DIAGS
			if (!(newstate & XAWS_SHADED))
			{
				DIAGS(("move_window: clear shaded"));
			}
#endif
			if (newstate == ~XAWS_FULLED)
			{
				blit = false;
				wind->pr = wind->rc;
				DIAGS(("move_window: unFULLED"));
			}
			wind->window_status &= newstate;
		}
		else
		{
			if ((newstate & XAWS_ICONIFIED))
			{
				DIAGS(("move_window: set iconified"));
				if (!is_iconified(wind))
				{
					wind->save_widgets = wind->active_widgets;
					wind->save_delta = wind->delta;
					wind->save_wadelta = wind->wadelta;
					standard_widgets(wind, (wind->active_widgets & (NO_TOPPED)) | NAME|MOVER|ICONIFIER, true);
					wind->ro = wind->rc;
				}
			}
#if GENERATE_DIAGS
			if ((newstate & XAWS_SHADED))
			{
				DIAGS(("move_window: set shaded"));
			}
#endif
			if (newstate == XAWS_FULLED)
			{
				blit = false;
				wind->pr = wind->rc;
				DIAGS(("move_window: win is FULLED"));
			}
			wind->window_status |= newstate;
		}
	}
	else
	{
#if GENERATE_DIAGS
		if ((wind->window_status & XAWS_SHADED))
		{
			DIAGS(("move_window: win is shaded"));
		}
#endif
		wind->pr = wind->rc;
	}

	inside_root(&new, wind->owner->options.noleft);

	if( !cfg.menu_ontop && cfg.menu_bar && old.y != wind->r.y && old.y < get_menu_height() )
	{
		blit = false;
	}
	set_and_update_window(wind, blit, false, &new);

	if ((wind->window_status & XAWS_OPEN) && !(wind->dial & created_for_SLIST) && !(wind->active_widgets & STORE_BACK))
	{
		struct xa_window *nxt = wind->nolist ? (wind->next ? wind->next : window_list) : wind->next;
		update_windows_below(lock, &old, &new, nxt, NULL);
	}

	/*
	 * If moving the window did not result in any WM_REDRAWS
	 * being generated (C.move_block is set to 3 when WM_REDRAWS
	 * are added to a clients msg queue), we release move_block here
	 */
	if (!C.redraws && C.move_block != 3)
		C.move_block = 0;
	{
		short y = old.y < new.y ? old.y : new.y;
		if( !cfg.menu_ontop && cfg.menu_bar && y < get_menu_height() )
		{
			C.rdm = 1;
		}
	}
}

/*
 * Close an open window and re-display any windows underneath it.
 * Also places window in the closed_windows list but does NOT delete it
 * - the window will still exist after this call.
 */
bool _cdecl
close_window(enum locks lock, struct xa_window *wind)
{
	struct xa_window *wl, *w = NULL;
	struct xa_client *client = wind->owner;
	RECT r;
	bool is_top, ignorefocus = false;
	short dial;

	if (wind == NULL)
	{
		DIAGS(("close_window: null window pointer"));
		/* Invalid window handle, return error */
		return false;
	}

	DIAG((D_wind, wind->owner, "close_window: %d (%s)",
		wind->handle, (wind->window_status & XAWS_OPEN) ? "open" : "closed"));

	if (!(wind->window_status & XAWS_OPEN))
		return false;

	if (wind == C.hover_wind)
	{
#if WITH_BBL_HELP
	//if( !C.boot_focus )
		bubble_show(0);	// cancel pending bubble
#endif
		C.hover_wind = NULL;
		C.hover_widg = NULL;
	}
	if( !C.shutdown )
		add_window_to_tasklist( wind, (char *)-1);
	if (!(wind->dial & (created_for_SLIST|created_for_CALC)))
		cancel_winctxt_popup(lock, wind, NULL);

	if (wind->nolist)
	{
		DIAGS(("close_window: nolist window %d, bkg=%lx",
			wind->handle, wind->background));


		cancel_do_winmesag(lock, wind);

		if (!(wind->dial & created_for_SLIST))
		{
			if (!wind->next && wind->nolist)
			{
				wl = window_list;
			}
			else
			{
				wl = wind->next;
			}

			wi_remove(&S.open_nlwindows, wind, true);
			wi_put_first(&S.closed_nlwindows, wind);

			r = wind->r;
			if (!(wind->active_widgets & STORE_BACK) && !C.shutdown )
				update_windows_below(lock, &r, NULL, wl, NULL);

			unset_focus(wind);
		}

		if ((wind->active_widgets & STORE_BACK))
		{
			(*xa_vdiapi->form_restore)(0, wind->r, &(wind->background));
		}
		clear_wind_rectlist(wind);
		wind->window_status &= ~XAWS_OPEN;
		remove_from_iredraw_queue(lock, wind);
		return true;
	}

	is_top = is_topped(wind);
	ignorefocus = (wind->window_status & XAWS_NOFOCUS) ? true : false;

	wl = wind->next;

	wi_remove(&S.open_windows, wind, true);
	wi_put_first(&S.closed_windows, wind);
	remove_from_iredraw_queue(lock, wind);
	r = wind->r;
	dial = wind->dial;

	clear_wind_rectlist(wind);

	/* Tag window as closed */
	wind->window_status &= ~(XAWS_OPEN);

	if (!(wind->window_status & XAWS_BELOWROOT))
	{
		if (!wl)
			wl = window_list;

		if( !C.shutdown )
			update_windows_below(lock, &r, NULL, wl, NULL);

		if (is_top && !(wind->dial & created_for_AES))
		{
			if (wind->active_widgets & STORE_BACK)
			{
				(*xa_vdiapi->form_restore)(0, wind->r, &(wind->background));
				return true;
			}

			w = window_list;

			while (w && w != root_window)
			{
				if (w->owner == client && !(w->window_status & (XAWS_ICONIFIED|XAWS_HIDDEN|XAWS_NOFOCUS)))
				{
					if (w != TOP_WINDOW)
					{
						top_window(lock|winlist, true, true, w);
					}
					else
					{
						setnew_focus(w, NULL, true, true, true);
// 						setwin_ontop(lock, true);
// 						send_iredraw(lock, w, 0, NULL);
						set_and_update_window(w, true, true, NULL);
					}
					wl = w->next;
					break;
				}
				w = w->next;
			}

			if (w && (w == root_window || (w->owner->status & CS_EXITING)))
				w = NULL;
		}

		if (!ignorefocus && TOP_WINDOW && is_top && !w
			&& !(dial & (created_for_AES|created_for_WDIAL|created_for_FORM_DO|created_for_FMD_START)) )
		{
			switch (client->options.clwtna)
			{
				case 0:		/* Keep active client ontop */
				{
					break;
				}
				default:
				case 1:	/* Top app who owns window below closed one */
				{
					if ((w = TOP_WINDOW) && w != root_window)
					{
						do
						{
							if (!(w->window_status & (XAWS_ICONIFIED|XAWS_HIDDEN|XAWS_NOFOCUS)) && !(w->owner->status & CS_EXITING))
								break;
						} while ((w = w->next) && w != root_window);
					}
					if (w && w != root_window)
					{
// 						if (d) display("topping win for %s", w->owner->name);
						top_window(lock, true, true, w);
					}
					else if (w == root_window)
					{
// 						if (d) display("topping desktop! %s", desktop_owner()->name);
						app_in_front(lock, desktop_owner(), true, true, true);
					}
					else
					{
// 						if (d) display("clwtna 1 last resort - top prev app");
						app_in_front(lock, previous_client(lock, 1), true, true, false);
					}
					break;
				}
				case 2: /* Top the client previously topped */
				{
// 					if (d) display("topping prev client %s", previous_client(lock, 1)->name);
					app_in_front(lock, previous_client(lock, 1), true, true, false);
					break;
				}
			}
		}
	}
	if( !C.shutdown )
		set_winmouse(-1, -1);

	return true;
}

static void
free_standard_widgets(struct xa_window *wind)
{
	int i;

	DIAGS(("free_standard_widgets: wind %d of %s", wind->handle, wind->owner->name));

	for (i = 0; i < XA_MAX_WIDGETS; i++)
	{
		//DIAGS(("call remove_widget for widget %d", i));
		remove_widget(0, wind, i);
	}
	DIAGS(("free_standard_widgets: !"));
}

static void
delete_window1(enum locks lock, struct xa_window *wind)
{
	struct xa_client *client = wind->owner;

	/* Ozk: I dont think this is really necessary here as it should
	 *	should be done when closing the window..
	 */
	if (wind == C.hover_wind)
	{
		C.hover_wind = NULL;
		C.hover_widg = NULL;
		xa_graf_mouse(ARROW, NULL, NULL, false);
	}

//	cancel_do_winmesag(lock, wind);

	if (!wind->nolist)
	{
		DIAG((D_wind, wind->owner, "delete_window1 %d for %s: open? %s",
			wind->handle, w_owner(wind),
			(wind->window_status & XAWS_OPEN) ? "yes" : "no"));

		//assert(!(wind->window_status & XAWS_OPEN));
		if( (wind->window_status & XAWS_OPEN) )
		{
			return;	/* ?? */
		}

		/* Call the window destructor if any */
		if (wind->destructor)
			wind->destructor(lock|winlist, wind);

		free_standard_widgets(wind);
		free_wind_handle(wind->handle);

		if (wind->background)
			kfree(wind->background);

	}
	else
	{
		DIAGS(("delete_window1: nolist window %d, bgk=%lx", wind->handle, wind->background));
		if (wind->destructor)
			wind->destructor(lock, wind);

		free_standard_widgets(wind);

		if (wind->background)
			kfree(wind->background);
	}

	clear_wind_rectlist(wind);

	if (wind->widg_rows)
		kfree(wind->widg_rows);

	if (wind->ontop_cols)
	{
		(*client->xmwt->free_color_theme)(client->wtheme_handle, wind->ontop_cols);
	}
	if (wind->untop_cols)
	{
		(*client->xmwt->free_color_theme)(client->wtheme_handle, wind->untop_cols);
	}
	if (wind->active_theme) //wind->widget_theme && wind->widget_theme->active)
	{
		wind->active_theme->links--;
	}

	free_xa_data_list(&wind->xa_data);

	kfree(wind);
}

static void
CE_delete_window(enum locks lock, struct c_event *ce, short cancel)
{
// 	if (!strnicmp(ce->client->proc_name, "gfa_xref", 8))
// 		display("CE_del_wind %lx", ce->ptr1);

	/*
	 * Since the window in this event already have been removed from
	 * all stacks, we perform  this deletion even when cancel flag is set
	 */
	//if (!cancel)
		delete_window1(lock, ce->ptr1);
}

void _cdecl
delete_window(enum locks lock, struct xa_window *wind)
{
	/* We must be sure it is in the correct list. */
	if ((wind->window_status & XAWS_OPEN))
	{
		DIAG((D_wind, wind->owner, "delete_window %d: not closed", wind->handle));
		/* open window, return error */
		return;
	}
	if (wind->nolist)
	{
		if (wind->dial & created_for_CALC)
			wi_remove(&S.calc_windows, wind, false);
		else if (!(wind->dial & created_for_SLIST))
			wi_remove(&S.closed_nlwindows, wind, false);
	}
	else
		wi_remove(&S.closed_windows, wind, false);

	if (!(wind->dial & (created_for_SLIST|created_for_CALC)))
	{
		cancel_winctxt_popup(lock, wind, NULL);
		remove_from_iredraw_queue(lock, wind);
	}
	cancel_do_winmesag(lock, wind);

	post_cevent(wind->owner, CE_delete_window, wind, NULL, 0,0, NULL, NULL);
	//delete_window1(lock, wind);
}

void _cdecl
delayed_delete_window(enum locks lock, struct xa_window *wind)
{
	DIAG((D_wind, wind->owner, "delayed_delete_window %d for %s: open? %s",
		wind->handle, w_owner(wind),
		(wind->window_status & XAWS_OPEN) ? "yes":"no" ));

	/* We must be sure it is in the correct list. */
	if ((wind->window_status & XAWS_OPEN))
	{
		DIAG((D_wind, wind->owner, "delayed_delete_window %d: not closed", wind->handle));
		/* open window, return error */
		return;
	}
	if (wind->nolist)
	{
		if (!(wind->dial & created_for_SLIST))
			wi_remove(&S.closed_nlwindows, wind, false);
	}
	else
		wi_remove(&S.closed_windows, wind, false);

	if (!(wind->dial & (created_for_SLIST|created_for_CALC)))
	{
		cancel_winctxt_popup(lock, wind, NULL);
		remove_from_iredraw_queue(lock, wind);
	}
	cancel_do_winmesag(lock, wind);

	post_cevent(wind->owner, CE_delete_window, wind, NULL, 0,0, NULL, NULL);
	//wi_put_first(&S.deleted_windows, wind);
}

void
do_delayed_delete_window(enum locks lock)
{
	struct xa_window *wind = S.deleted_windows.first;

	DIAGS(("do_delayed_delete_window"));

	while (wind)
	{
		/* remove from list */
		wi_remove(&S.deleted_windows, wind, false);

		/* final delete */
		if( wind == menu_window || wind == root_window )
			BLOG((0,"do_delayed_delete_window:ERROR:wind=%s", wind == menu_window ? "menu-window" : "root-window" ));
		else
			delete_window1(lock, wind);

		/* anything left? */
		wind = S.deleted_windows.first;
	}
}

/*
 * Context dependant !!!
 *
 * which unused!
 */
void
display_window(enum locks lock, int which, struct xa_window *wind, RECT *clip)
{
	if (!(wind->owner->status & CS_EXITING) && (wind->window_status & XAWS_OPEN))
	{
		{
			struct xa_rect_list *rl;
			RECT d;

			rl = (wind->dial & created_for_SLIST) && wind->parent ? wind->parent->rect_list.start : wind->rect_list.start;

			while (rl)
			{
				if (clip)
				{
					if (xa_rect_clip(clip, &rl->r, &d))
					{
						draw_window(lock, wind, &d);
					}
				}
				else
				{
					draw_window(lock, wind, &rl->r);
				}
				rl = rl->next;
			}
		}

		(*wind->vdi_settings->api->clear_clip)(wind->vdi_settings);
	}
}

void
update_all_windows(enum locks lock, struct xa_window *wl)
{
	while (wl)
	{
		if (wl != root_window && !(wl->owner->status & CS_EXITING) && (wl->window_status & XAWS_OPEN))
		{
			set_and_update_window(wl, true, false, NULL);
		}
		(*wl->vdi_settings->api->clear_clip)(wl->vdi_settings);
		wl = wl->next;
	}
}

/*
 * Display a window that isn't on top, respecting clipping
 * - Pass clip == NULL to redraw whole window, otherwise clip is a pointer to a GRECT that
 * defines the clip rectangle.
 */
void
update_windows_below(enum locks lock, const RECT *old, RECT *new, struct xa_window *wl, struct xa_window *wend)
{
	RECT clip;
	enum locks wlock = lock | winlist;

	DIAGS(("update_windows_below: ..."));

	if (!wl)
		return;
	do
	{
		if (wend && wend == wl)
			break;

		if (!(wl->owner->status & CS_EXITING) && (wl->window_status & XAWS_OPEN))
		{
			/* Check for newly exposed windows */
			if (xa_rect_clip(old, &wl->r, &clip))
			{
				struct xa_rect_list *rl;
				RECT d;

				make_rect_list(wl, true, RECT_SYS);

				rl = wl->rect_list.start;

				while (rl)
				{
					if (xa_rect_clip(&rl->r, &clip, &d))
					{
						generate_redraws(wlock, wl, &d, RDRW_ALL);
					}
					rl = rl->next;
				}
				/* if XaAES-window wait to avoid redraw-error */
				if( wl->owner == C.Hlp )
					yield();
			}
			else if (new)
			{
				/* Check for newly covered windows */
				if (xa_rect_clip(new, &wl->r, &clip))
				{
					/* We don't need to send a redraw to
					 * these windows, we just have to update
					 * their rect lists */
					make_rect_list(wl, true, RECT_SYS);
				}
			}
		}
	} while (wl != root_window && (wl = ((!wl->next && wl->nolist) ? window_list : wl->next)));

	DIAGS(("update_windows_below: !"));
}

/*
 * Typically called after client have been 'lagging',
 * Redraw and send a FULL SET of WM_REDRAWS for all windows owned by
 * the client going non-lagged.
 * Context dependant!
 */
static int
get_lost_redraw_msg(struct xa_client *client, union msg_buf *buf)
{
	struct xa_aesmsg_list *msg;
	int rtn = 0;
	msg = client->lost_rdrw_msg;
	if (msg)
	{
		/* dequeue */
		client->lost_rdrw_msg = msg->next;

		/* write to client */
		*buf = msg->message;

		DIAG((D_m, NULL, "Got pending WM_REDRAW (%lx (wind=%d, %d/%d/%d/%d)) for %s",
			msg, buf->m[3], *(RECT *)&buf->m[4], c_owner(client) ));

		kfree(msg);
		rtn = 1;
// 		kick_mousemove_timeout();
	}
// 	else if (C.redraws)
// 		yield();
	return rtn;
}

void
redraw_client_windows(enum locks lock, struct xa_client *client)
{
	struct xa_window *wl;
	struct xa_rect_list *rl;
	union msg_buf buf;
	bool makerl = true;
	RECT r;

	while (get_lost_redraw_msg(client, &buf))
	{
		if (get_desktop()->owner == client)
		{
			wl = root_window;
			if (makerl) make_rect_list(wl, true, RECT_SYS);
			rl = wl->rect_list.start;
			while (rl)
			{
				if (xa_rect_clip(&rl->r, (RECT *)&buf.m[4], &r))
				{
					generate_redraws(lock, wl, &r, RDRW_ALL);
				}
				rl = rl->next;
			}
		}

		wl = root_window->prev;

		while (wl)
		{
			if (wl->owner == client && (wl->window_status & XAWS_OPEN))
			{
				if (makerl) make_rect_list(wl, true, RECT_SYS);
				rl = wl->rect_list.start;
				while (rl)
				{
					if (xa_rect_clip(&rl->r, (RECT *)&buf.m[4], &r))
					{
						generate_redraws(lock, wl, &r, RDRW_ALL);
					}
					rl = rl->next;
				}
			}
			wl = wl->prev;
		}
		makerl = false;
	}
}

RECT
w2f(RECT *d, const RECT *in, bool chkwh)
{
	RECT r;
	r.x = in->x - d->x; //(wind->rwa.x - wind->r.x);
	r.y = in->y - d->y; //(wind->rwa.y - wind->r.y);
	r.w = in->w + d->w; //(wind->r.w - wind->rwa.w);
	r.h = in->h + d->h; //(wind->r.h - wind->rwa.h);
	if (chkwh && (r.w <= 0 || r.h <= 0))
		r.w = r.h = 0;
	return r;
}

RECT
f2w(RECT *d, const RECT *in, bool chkwh)
{
	RECT r;
	r.x = in->x + d->x; //(wind->rwa.x - wind->r.x);
	r.y = in->y + d->y; //(wind->rwa.y - wind->r.y);
	r.w = in->w - d->w; //(wind->r.w - wind->rwa.w);
	r.h = in->h - d->h; //(wind->r.h - wind->rwa.h);
	if (chkwh && (r.w <= 0 || r.h <= 0))
		r.w = r.h = 0;

	return r;
}

/*
 * Calculate window sizes
 *
 *   HR: first intruduction of the use of a fake window (pid=-1) for calculations.
 *         (heavily used by multistrip, as an example.)
 */
#if 0
static void
Xpolate(RECT *r, RECT *o, RECT *i)
{
	/* If you want to prove the maths here, draw two boxes one inside
	 * the other, then sit and think about it for a while...
	 * HR: very clever :-)
	 */
	r->x = 2 * o->x - i->x;
	r->y = 2 * o->y - i->y;
	r->w = 2 * o->w - i->w;
	r->h = 2 * o->h - i->h;
}
#endif

static struct xa_wc_cache *
add_wcc_entry(struct xa_window *wind)
{
	struct xa_wc_cache *new;
	struct xa_client *client = wind->owner;

	new = kmalloc(sizeof(*new));
	if (new)
	{
		new->next = client->wcc;
		client->wcc = new;
		new->class = wind->class;
// 		new->wtheme_handle = themekey; //client->wtheme_handle;
		new->tp = wind->requested_widgets;
		new->delta = wind->delta;
		new->wadelta = wind->wadelta;
	}
	return new;
}

void
delete_wc_cache(struct xa_wc_cache **wcc)
{
	while (*wcc)
	{
		struct xa_wc_cache *n = *wcc;
		*wcc = n->next;
		kfree(n);
	}
}

static struct xa_wc_cache *
lookup_wcc_entry(struct xa_client *client, short class, XA_WIND_ATTR tp)
{
	struct xa_wc_cache *wcc = client->wcc;

	while (wcc)
	{
		if (wcc->tp == tp && wcc->class == class) //client->wtheme_handle)
			break;
		wcc = wcc->next;
	}
	return wcc;
};

RECT
calc_window(enum locks lock, struct xa_client *client, int request, XA_WIND_ATTR tp, WINDOW_TYPE dial, int thinframe, bool thinwork, RECT r)
{
	struct xa_window *w_temp;
	struct xa_wc_cache *wcc;
	short class;
	RECT o;
	DIAG((D_wind,client,"calc %s from %d/%d,%d/%d", request ? "work" : "border", r));
	tp = fix_wind_kind(tp);
	dial |= created_for_CALC;

	if (dial & created_for_POPUP)
		class = WINCLASS_POPUP;
	else if (dial & created_for_ALERT)
		class = WINCLASS_ALERT;
	else
		class = WINCLASS_CLIENT;

	wcc = lookup_wcc_entry(client, class, tp);
	if (!wcc)
	{
		w_temp = create_window(lock, NULL, NULL, client, true, tp, dial, thinframe, thinwork, r, 0,0);
		wcc = add_wcc_entry(w_temp);
		delete_window(lock, w_temp);
	}

	if (wcc)
	{
		switch (request)
		{
			case WC_BORDER:
			{
				o = w2f(&wcc->delta, &r, false);
				break;
			}
			default:
			{
				o = f2w(&wcc->delta, &r, false);
				break;
			}
		}
	}
	else
	{
		o = r;
	}
	return o;
}

/*
 * two x-adjacend rects; y equal or outside wa: join
 * when generating redraws
 *
 * cannot change window's rl (?)
 */
static bool join_redraws( short wlock, struct xa_window *wind, struct xa_rect_list *newrl, short flags )
{
	short y11 = newrl->r.y, y12 = newrl->r.y + newrl->r.h, y21 = newrl->next->r.y, y22 = newrl->next->r.y + newrl->next->r.h,
		x12 = newrl->r.x + newrl->r.w, x21 = newrl->next->r.x, x22 = newrl->next->r.x + newrl->next->r.w, x11 = newrl->r.x,
		way1 = wind->wa.y, way2 = wind->wa.y + wind->wa.h;

	/* two x-adjacend rects; y equal or outside wa: join */
	if( (y11 == y21 || (y11 < way1 && y21 < way1)) && (y12 == y22 || (y12 >= way2 && y22 >= way2)) && (x21 == x12 || x22 == x11) )
	{
		RECT r;
		r.y = y11 < y21 ? y11 : y21;
		r.h = y12 > y22 ? y12 : y22;
		r.h -= r.y;
		r.x = x11 < x21 ? x11 : x21;
		r.w = newrl->r.w + newrl->next->r.w;

		generate_redraws(wlock, wind, &r, flags );
		return true;
	}
	return false;
}

STATIC void
set_and_update_window(struct xa_window *wind, bool blit, bool only_wa, RECT *new)
{
	short dir, resize, xmove, ymove, wlock = 0;
	struct xa_rect_list *oldrl, *orl, *newrl, *brl, *prev, *next, *rrl, *nrl;
	RECT bs, bd, old, wa, oldw;

	DIAGS(("set_and_update_window:"));

	old = wind->r;
	oldw = wind->wa;

	if (new)
	{
		wind->rc = wind->t = *new;
		if ((wind->window_status & XAWS_SHADED))
		{
			new->h = wind->sh;
		}
		wind->r = *new;
		calc_work_area(wind);
	}
	else
		new = &wind->r;

	wa = wind->wa;

	xmove = new->x - old.x;
	ymove = new->y - old.y;

	resize	= new->w != old.w || new->h != old.h ? 1 : 0;

#if WITH_BBL_HELP
	if( !C.boot_focus )
		bubble_show(0);	// cancel pending bubble
#endif
	if (xmove || ymove || resize)
	{
		/*
		 * Always update the true (actual) screen coordinates
		 * of the widgets that must be redrawn context
		 * dependantly
		 */
		move_ctxdep_widgets(wind);

		/* XXX - This makes set_and_update_window() context dependant
		 *	 when moving windows!!
		 */
		//if (xmove || ymove)
		{
			XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
			XA_TREE *wt = widg->stuff;

			/* Temporary hack 070702 */
			if (wt && wt->tree)
			{
				/* this moves the origin of the window -
					 objects (e.g.buttons) have to be moved by the client
				*/
				wt->tree->ob_x = wind->wa.x;
				wt->tree->ob_y = wind->wa.y;
				if (!wt->zen)
				{
					wt->tree->ob_x += wt->ox;
					wt->tree->ob_y += wt->oy;
				}
			}
		}
	}

	if (!(wind->window_status & XAWS_OPEN))
		return;
#if 1
	if ((wind->dial & created_for_SLIST))
		return;
	if ((wind->nolist && (wind->active_widgets & STORE_BACK))/* || (wind->dial & created_for_SLIST)*/)
	{
		if (xmove || ymove || resize)
		{
			if (wind->active_widgets & STORE_BACK)
				(*xa_vdiapi->form_restore)(0, old, &(wind->background));

			make_rect_list(wind, true, RECT_SYS);

			if (wind->active_widgets & STORE_BACK)
			{
				(*xa_vdiapi->form_save)(0, wind->r, &(wind->background));
			}

			//if (!(wind->dial & created_for_SLIST)) // && !(wind->active_widgets & STORE_BACK))
			{
				generate_redraws(wlock, wind, (RECT *)&wind->r, !only_wa ? RDRW_ALL : RDRW_WA);
			}
		}

		return;
	}
#endif
	oldrl = wind->rect_list.start;
	wind->rect_list.start = wind->rect_list.next = NULL;
	newrl = make_rect_list(wind, false, RECT_SYS);
	wind->rect_list.start = newrl;

	if (blit && oldrl && newrl)
	{
		//DIAGS(("old win=(%d/%d/%d/%d), new win=(%d/%d/%d/%d)",
		//	old, *new));

		if (xmove || ymove)
		{
			if (ymove > 0)
				dir = 1;
			else
			{
				dir = 0;
				ymove = -ymove;
			}
			if (xmove > 0)
			{
				dir |= 2;
			}
			else
				xmove = -xmove;
		}
		else
			dir = 4;

		brl = NULL;

		oldw.x -= old.x;
		oldw.y -= old.y;

		wa.x = wind->wa.x - new->x;
		wa.y = wind->wa.y - new->y;
		wa.w = wind->wa.w;
		wa.h = wind->wa.h;

		/* increase width of workarea by vslider-width if toolbar installed */
		if( (wind->active_widgets & TOOLBAR) && (wind->active_widgets & VSLIDE) )
		{
			XA_WIDGET *widg = get_widget(wind, XAW_VSLIDE);
			wa.w += widg->ar.w;
 		}

		/*
		 * Convert to relative coords
		 */
		orl = oldrl;
		prev = NULL;
		while (orl)
		{
			orl->r.x -= old.x;
			orl->r.y -= old.y;

			if (resize)
			{
				if (!xa_rect_clip(&oldw, &orl->r, &orl->r))
				{
					if (prev)
						prev->next = orl->next;
					else
						oldrl = orl->next;

					nrl = orl;
					orl = orl->next;
					kfree(nrl);
					//DIAGS(("kfree %lx (unused oldrl)", nrl));
				}
				else
				{
					prev = orl;
					orl = orl->next;
				}
			}
			else
				orl = orl->next;
		}
		while (newrl)
		{
			bs.x = newrl->r.x - new->x;
			bs.y = newrl->r.y - new->y;
			bs.w = newrl->r.w;
			bs.h = newrl->r.h;

			if (resize)
			{
				if (!xa_rect_clip(&wa, &bs, &bs))
					orl = NULL;
				else
					orl = oldrl;
			}
			else
				orl = oldrl;

			while (orl)
			{
				//DIAGS(("Check for common areas (newrl=%d/%d/%d/%d, oldrl=%d/%d/%d/%d)", bs, orl->r));
				if (xa_rect_clip(&bs, &orl->r, &bd))
				{
// 					if (dir == 3)
// 						display(" %04d/%04d/%04d/%04d", bd);

					nrl = kmalloc(sizeof(*nrl));
					assert(nrl);

					//DIAGS(("kmalloc %lx (%ld bytes) blitrect", nrl, sizeof(*nrl)));

					//DIAGS(("COMMON brl=%lx, nrl=%lx, brl->nxt=%lx", brl, nrl, brl ? (long)brl->next : 0xFACEDACE));
					if (brl)
					{
						struct xa_rect_list *n, *p;
						short ox2; // = bd.x + bd.w;
						short oy2; // = bd.y + bd.h;

						switch (dir)
						{
							case 0:	// up/left
							{
								ox2 = bd.x + bd.w;
								oy2 = bd.y + bd.h;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( (bd.x < n->r.x && ox2 <= n->r.x) ||
									     ((n->r.x + n->r.w) > bd.x && oy2 <= n->r.y) )
									{
										if (p)
										{
											nrl->next = n;
											p->next = nrl;
										}
										else
										{
											nrl->next = brl;
											brl = nrl;
										}
										break;
									}
									p = n;
									n = n->next;
								}
								if (!n)
								{
									p->next = nrl;
									nrl->next = NULL;
								}
								break;
							}
							case 1: // down/left
							{
								ox2 = bd.x + bd.w;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( (bd.x < n->r.x && ox2 <= n->r.x) ||
									     ((n->r.x + n->r.w) > bd.x && (n->r.y + n->r.h) <= bd.y) )
									{
										if (p)
										{
											nrl->next = n;
											p->next = nrl;
										}
										else
										{
											nrl->next = brl;
											brl = nrl;
										}
										break;
									}
									p = n;
									n = n->next;
								}
								if (!n)
								{
									p->next = nrl;
									nrl->next = NULL;
								}
								break;
							}
							case 2: // up/right
							{
								ox2 = bd.x + bd.w;
								oy2 = bd.y + bd.h;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( (ox2 > (n->r.x + n->r.w) && bd.x >= (n->r.x + n->r.w)) ||
									     ((n->r.x + n->r.w) < bd.x && oy2 <= n->r.y) )
									{
										if (p)
										{
											nrl->next = n;
											p->next = nrl;
										}
										else
										{
											nrl->next = brl;
											brl = nrl;
										}
										break;
									}
									p = n;
									n = n->next;
								}
								if (!n)
								{
									p->next = nrl;
									nrl->next = NULL;
								}
								break;
							}
							case 3: // down/right
							{
								oy2 = bd.y + bd.h;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( oy2 >= n->r.y &&
									     bd.x >= n->r.x
									   )
									{
										if (p)
										{
											nrl->next = n;
											p->next = nrl;
										}
										else
										{
											nrl->next = brl;
											brl = nrl;
										}
										break;
									}
									p = n;
									n = n->next;
								}
								if (!n)
								{
									p->next = nrl;
									nrl->next = NULL;
								}
								break;
							}
							case 4: // No sort - area is not moving
							{
								nrl->next = brl->next;
								brl->next = nrl;
								break;
							}
						}
					}
					else
					{
						brl = next = nrl;
						brl->next = NULL;
					}
					nrl->r = bd;
					//DIAGS(("save blitarea %d/%d/%d/%d", nrl->r));
				}
				orl = orl->next;
			}
			newrl = newrl->next;
		}
		/*
		 * Release previous rectangle list
		 */
		while (oldrl)
		{
			nrl = oldrl;
			oldrl = oldrl->next;
			//DIAGS(("freeing oldrl %lx", nrl));
			kfree(nrl);
			//DIAGS(("kfree %lx (rest of oldrl)", nrl));
		}
		if (brl)
		{
			/*
			 * Do the blitting first...
			 */
			if (xmove || ymove)
			{
				struct xa_rect_list *trl = 0;
				nrl = brl;
				hidem();
				while (nrl)
				{
					/* first blit lower rect if two left, moving down and upper rect partly hidden by menubar (see build_rectlist - surely a hack ..) */
					if( menu_window && cfg.menu_bar && (dir & 1) && nrl->r.y < menu_window->r.h )
						if( nrl->next && !nrl->next->next && nrl->r.y + nrl->r.h == nrl->next->r.y && !trl )
						{
							trl = nrl;
							nrl = nrl->next;
							dir = 0;
						}
					bd = nrl->r;
					bs.x = bd.x + new->x;
					bs.y = bd.y + new->y;
					bs.w = bd.w;
					bs.h = bd.h;
					bd.x += old.x;
					bd.y += old.y;
					//DIAGS(("Blitting from %d/%d/%d/%d to %d/%d/%d/%d (%lx, %lx)",
					//	bd, bs, brl, (long)brl->next));
					(*xa_vdiapi->form_copy)(&bd, &bs);
					if( trl )
					{
						if( nrl == trl )
						{
							nrl = 0;	//nrl->next->next;
							trl = 0;
						}
						nrl = trl;
					}
					else
						nrl = nrl->next;
				}
				showm();
			}
			/*
			 * Calculate rectangles that need to be redrawn...
			 */
			newrl = wind->rect_list.start;
			while (newrl)
			{
				nrl = kmalloc(sizeof(*nrl));
				assert(nrl);

				//DIAGS(("kmalloc %lx (%ld bytes)", nrl, sizeof(*nrl)));
				nrl->next = NULL;
				nrl->r.x = newrl->r.x - new->x;
				nrl->r.y = newrl->r.y - new->y;
				nrl->r.w = newrl->r.w;
				nrl->r.h = newrl->r.h;

				//DIAGS(("NEWRECT (%lx) %d/%d/%d/%d (%d/%d/%d/%d)", nrl, nrl->r, newrl->r));

				if (resize)
				{
					if (!xa_rect_clip(&wa, &nrl->r, &nrl->r))
					{
						kfree(nrl);
						//DIAGS(("kfree %lx", nrl));
						nrl = NULL;
					}
				}

				orl = brl;
				while (orl && nrl)
				{
					short w, h, flag;
					short bx2 = orl->r.x + orl->r.w;
					short by2 = orl->r.y + orl->r.h;

					//DIAGS(("BLITRECT (orl=%lx, nxt=%lx)(nrl=%lx, nxt=%lx) %d/%d/%d/%d, x2/y2=%d/%d",
					//	orl, (long)orl->next, nrl, (long)nrl->next, orl->r, bx2, by2));

					for (rrl = nrl, prev = NULL; rrl; rrl = next)
					{
						RECT our = rrl->r;
						w = orl->r.x - rrl->r.x;
						h = orl->r.y - rrl->r.y;

						//DIAGS(("CLIPPING %d/%d/%d/%d, w/h=%d/%d", our, w, h));
						//DIAGS(("rrl = %lx", rrl));

						next = rrl->next;

						if (   h < our.h   &&
						       w < our.w   &&
						     bx2 > our.x   &&
						     by2 > our.y )
						{
							short nx2 = our.x + our.w;
							short ny2 = our.y + our.h;

							flag = 0;
							if (orl->r.y > our.y)
							{
								rrl->r.x = our.x;
								rrl->r.y = our.y;
								rrl->r.w = our.w;
								rrl->r.h = h;
								our.y += h;
								our.h -= h;
								flag = 1;
								//DIAGS((" -- 1. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
								//	rrl->r, our));
							}
							if (orl->r.x > our.x)
							{
								if (flag)
								{
									prev = rrl;

									rrl = kmalloc(sizeof(*rrl));
									assert(rrl);

									//DIAGS(("kmalloc %lx (%ld bytes)", rrl, sizeof(*rrl)));
									rrl->next = prev->next;
									prev->next = rrl;
									//DIAGS((" -- 2. new (%lx)", rrl));
								}
								rrl->r.x = our.x;
								rrl->r.y = our.y;
								rrl->r.h = our.h;
								rrl->r.w = w;
								our.x += w;
								our.w -= w;
								flag = 1;
								//DIAGS((" -- 2. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
								//	rrl->r, our));
							}
							if (ny2 > by2)
							{
								if (flag)
								{
									prev = rrl;

									rrl = kmalloc(sizeof(*rrl));
									assert(rrl);

									//DIAGS(("kmalloc %lx (%ld bytes)", rrl, sizeof(*rrl)));
									rrl->next = prev->next;
									prev->next = rrl;
									//DIAGS((" -- 3. new (%lx)", rrl));
								}
								rrl->r.x = our.x;
								rrl->r.y = by2;
								rrl->r.w = our.w;
								rrl->r.h = ny2 - by2;
								our.h -= rrl->r.h;
								flag = 1;
								//DIAGS((" -- 3. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
								//	rrl->r, our));
							}
							if (nx2 > bx2)
							{
								if (flag)
								{
									prev = rrl;

									rrl = kmalloc(sizeof(*rrl));
									assert(rrl);

									//DIAGS(("kmalloc %lx (%ld bytes)", rrl, sizeof(*rrl)));
									rrl->next = prev->next;
									prev->next = rrl;
									//DIAGS((" -- 4. new (%lx)", rrl));
								}
								rrl->r.x = bx2;
								rrl->r.y = our.y;
								rrl->r.w = nx2 - bx2;
								rrl->r.h = our.h;
								our.w -= rrl->r.w;
								flag = 1;
								//DIAGS((" -- 4. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
								//	rrl->r, our));
							}
							if (!flag)
							{
								//DIAGS((" ALL BLIT (removed) nrl=%lx, rrl=%lx, prev=%lx, nxt=%lx, %d/%d/%d/%d",
								//	nrl, rrl, prev, (long)rrl->next, rrl->r));

								if (nrl == rrl)
								{
									//if (prev == rrl)
									//	prev = NULL;
									nrl = next;
								}
								else if (nrl->next == rrl)
									nrl->next = next;

								if (prev)
									prev->next = next;

								kfree(rrl);
								//DIAGS(("kfree %lx", rrl));
							}
							else
								prev = rrl;
						}
						else
						{
							//DIAGS((" -- ALL REDRAW %d/%d/%d/%d", rrl->r));
							prev = rrl;
						}
						//DIAGS((" .. .. .."));
					} /* for (rrl = nrl; rrl; rrl = next) */
					//DIAGS(("FROM %lx to %lx", orl, (long)orl->next));
					orl = orl->next;
				} /* while (orl && nrl) */
				//DIAGS(("DONE CLIPPING"));
				/* only blitted: if window has a list-window inform list-window to move its widgets */
				if( !	resize )
				{
					struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
#if 1
					if( wt && wt->extra && (wt->flags & WTF_EXTRA_ISLIST) ) // &&
					{
						SCROLL_INFO *list = wt->extra;
						while( list && list->wi )
						{
							struct xa_window *lwind = list->wi;
							if( !(dir & 1) )
								ymove = -ymove;
							if( !(dir & 2) )
								xmove = -xmove;
							lwind->send_message(wlock, lwind, NULL, AMQ_NORM, QMF_CHKDUP,
								WM_MOVED, 0,0, lwind->handle,
								lwind->r.x + xmove, lwind->r.y + ymove, lwind->r.w, lwind->r.h);
									list = list->next;
						}
					}
#endif
				}
				/*
				 * nrl is the first in a list of rectangles that needs
				 * to be redrawn
				 */
				while (nrl)
				{
					short flags;
					nrl->r.x += new->x;
					nrl->r.y += new->y;
					//DIAGS(("redrawing area (%lx) %d/%d/%d/%d",
					//	nrl, nrl->r));
					/*
					 * we only redraw window borders here if wind moves
					 */

					if( wind->window_status & XAWS_RESIZED )
					{
						flags = 0;	/* redraw later */
					}
					else if (!resize && !only_wa)
					{
						flags = RDRW_ALL;
					}
					else
						flags = RDRW_WA;

					if( flags )
					{
						generate_redraws(wlock, wind, &nrl->r, flags);
					}
					orl = nrl;
					nrl = nrl->next;
					//DIAGS(("Freeing redrawed rect %lx", orl));
					kfree(orl);
					//DIAGS(("kfree %lx", orl));
				}
				newrl = newrl->next;
			} /* while (newrl) */
			/*
			 * free blitting list
			 */
			while (brl)
			{
				nrl = brl;
				brl = brl->next;
				//DIAGS(("Freeing blitrect %lx", nrl));
				kfree(nrl);
				//DIAGS(("kfree %lx", nrl));
			}
			/*
			 * If window was resized, redraw all window borders
			 */
			if (resize && !only_wa)
			{
				generate_redraws(wlock, wind, NULL, RDRW_EXT);
			}

		} /* if (brl) */
		else
		{
			short flags = only_wa ? RDRW_WA : RDRW_ALL;
			newrl = wind->rect_list.start;
			while (newrl)
			{
				generate_redraws(wlock, wind, &newrl->r, flags);
				newrl = newrl->next;
			}
		}
	}
	else if (newrl)
	{
		short flags = only_wa ? RDRW_WA : RDRW_ALL;
		while (newrl)
		{
			bool joined = false;
			if( newrl->next )
			{
				if( (joined = join_redraws( wlock, wind, newrl, flags )) )
					newrl = newrl->next;
			}

			if( !joined )
				generate_redraws(wlock, wind, &newrl->r, flags );
			if( newrl )
				newrl = newrl->next;
		}

		while (oldrl)
		{
			orl = oldrl;
			oldrl = oldrl->next;
			kfree(orl);
			//DIAGS(("kfree %lx", orl));
		}
	}
	DIAGS(("set_and_update_window: DONE!!!"));
}
