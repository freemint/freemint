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

static struct xa_window *pull_wind_to_top(int lock, struct xa_window *w);
static void	fitin_root(GRECT *r);
static void	set_and_update_window(struct xa_window *wind, bool blit, bool only_wa, GRECT *new);

/*
 * Window Stack Management Functions
 */

//static void draw_window(int lock, struct xa_window *wind);

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
 *     If called by the server, lock will be preset to LOCK_WINLIST,
 *     resulting in no locking (not needed).
 *
 *     If called by the signal handler, it is unclear, so the
 *     lock is applied. (doesnt harm).
 */
static void
rw(int lock, struct xa_window *wl, struct xa_client *client)
{
	struct xa_window *nwl;

	while (wl)
	{
		DIAGS(("-- RW: %lx, next=%lx, prev=%lx",
			(unsigned long)wl, (unsigned long)wl->next, (unsigned long)wl->prev));

		nwl = wl->next;
		if (wl != root_window && wl != menu_window)
		{
			if (!client || wl->owner == client)
			{
				if (wl->window_status & XAWS_OPEN)
				{
					DIAGS(("-- RW: closing %lx, client=%lx", (unsigned long)wl, (unsigned long)client));
					close_window(lock, wl);
				}
				DIAGS(("-- RW: deleting %lx", (unsigned long)wl));
				delete_window(lock, wl);
			}
		} else
		{
			DIAGS((" -- RW: skipping root window"));
		}
		wl = nwl;
	}
}

void
remove_windows(int lock, struct xa_client *client)
{
	DIAG((D_wind,client,"remove_windows on open_windows list for %s", c_owner(client)));
	if( !client )
		return;
	rw(lock, window_list, client);
	DIAG((D_wind,client,"remove_windows on closed_windows list for %s", c_owner(client)));
	rw(lock, S.closed_windows.first, client);
}

void
remove_all_windows(int lock, struct xa_client *client)
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
		GRECT t;
		t = w2f(&wind->rbd, &wind->widgets[XAW_TITLE].r, false);
		if( t.g_h < 6 )
			t.g_h = 6;
		wind->sh = t.g_h + wind->y_shadow;
		if (wind->frame > 0)
			wind->sh += (wind->frame << 1);
	}
}

static void
clear_wind_rectlist(struct xa_window *wind)
{
	DIAGS(("clear_wind_rectlist: on %d of %s", wind->handle, wind->owner->name));
	free_rectlist_entry(&wind->rect_list);
	free_rectlist_entry(&wind->rect_opt);
	free_rectlist_entry(&wind->rect_toolbar);
}

/*
 * Find first free position for iconification.
 * Uses the GRECT at wind->t.
 */
GRECT
free_icon_pos(int lock, struct xa_window *ignore)
{
	GRECT ic, r;
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

				if (w->t.g_x == r.g_x && w->t.g_y == r.g_y)
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
set_winrect(struct xa_window *wind, GRECT *wr, const GRECT *new)
{
	GRECT r;

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
inside_root(GRECT *r, bool noleft)
{
	short ret = 0;
	if( !cfg.leave_top_border )
	{
		short min = 0;

		if (cfg.menu_bar && cfg.menu_layout == 0 && cfg.menu_ontop == 0)
		{
			min = get_menu_height();
		}
		if (r->g_y < min )
		{
			r->g_y = min;
			ret = 1;
		}
	}

	if (noleft && r->g_x < root_window->wa.g_x)
	{
		r->g_x = root_window->wa.g_x;
		ret |= 2;
	}
	return ret;
}

static void
fitin_root(GRECT *r)
{
	GRECT *w = &root_window->wa;

	if (r->g_x < w->g_x)
		r->g_x = w->g_x;
	if (r->g_y < w->g_y)
		r->g_y = w->g_y;
	if (r->g_w > w->g_w)
		r->g_w = w->g_w;
	if (r->g_h > w->g_h)
		r->g_h = w->g_h;
}

void
inside_minmax(GRECT *r, struct xa_window *wind)
{
	bool um = (wind->active_widgets & USE_MAX);

	if (!is_iconified(wind))
	{
		if (um && wind->max.g_w && r->g_w > wind->max.g_w)
			r->g_w = wind->max.g_w;
		if (wind->min.g_w && r->g_w < wind->min.g_w)
			r->g_w = wind->min.g_w;

		if (um && wind->max.g_h && r->g_h > wind->max.g_h)
			r->g_h = wind->max.g_h;
		if (wind->min.g_h && r->g_h < wind->min.g_h)
			r->g_h = wind->min.g_h;
	}
}

#if GENERATE_DIAGS
static const char *const stacks[] =
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

static void
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

static void
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

static void
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
static void
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
hide_window(int lock, struct xa_window *wind)
{
	if (!is_hidden(wind))
	{
		GRECT r = wind->rc;
		r.g_x = root_window->rc.g_x + root_window->rc.g_w + 16;
		r.g_y = root_window->rc.g_y + root_window->rc.g_h + 16;
		wind->hx = wind->rc.g_x;
		wind->hy = wind->rc.g_y;
		if (wind->opts & XAWO_WCOWORK)
			r = f2w(&wind->delta, &r, true);
		send_moved(lock, wind, AMQ_NORM, &r);
		wind->window_status |= XAWS_HIDDEN;
	}
}
void
unhide_window(int lock, struct xa_window *wind, bool check)
{
	if (is_hidden(wind))
	{
		GRECT r;

		if (is_iconified(wind))
			r = free_icon_pos(lock, wind);
		else
		{
			r = wind->rc;
			r.g_x = wind->hx;
			r.g_y = wind->hy;
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
	GRECT clip;

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
	widg->stuff.name = wind->wname;

	DIAG((D_wind, wind->owner, "    -   %s", wind->wname));

	/* redraw if necessary */
	if (redraw && (wind->active_widgets & NAME) && ((wind->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN))
	{
		GRECT clip;

		rp_2_ap(wind, widg, &clip);
		display_window(0, 45, wind, &clip);
	}
	add_window_to_tasklist( wind, title );
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
	widg->stuff.name = wind->winfo;

	DIAG((D_wind, wind->owner, "    -   %s", wind->winfo));

	if (redraw && (wind->active_widgets & INFO) && (wind->window_status & (XAWS_OPEN|XAWS_SHADED|XAWS_ICONIFIED|XAWS_HIDDEN)) == XAWS_OPEN)
	{
		GRECT clip;

		rp_2_ap(wind, widg, &clip);
		display_window(0, 46, wind, &clip);
	}
}

#if INCLUDE_UNUSED
static void
send_ontop(int lock)
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
send_topped(int lock, struct xa_window *wind)
{
	struct xa_client *client = wind->owner;

	if (wind->send_message && (!client->fmd.wind || client->fmd.wind == wind))
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_TOPPED, 0, 0, wind->handle,
				   0, 0, 0, 0);
}
void
send_bottomed(int lock, struct xa_window *wind)
{
	struct xa_client *client = wind->owner;

	if (wind->send_message && (!client->fmd.wind || client->fmd.wind == wind))
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_BOTTOMED, 0, 0, wind->handle,
				   0, 0, 0, 0);
}

void
send_moved(int lock, struct xa_window *wind, short amq, GRECT *r)
{
	if (wind->send_message)
	{
		C.move_block = 2;
		wind->send_message(lock, wind, NULL, amq, QMF_CHKDUP,
			WM_MOVED, 0, 0, wind->handle,
			r->g_x, r->g_y, r->g_w, r->g_h);

	}
}
#define WIND_WMIN	64
#define WIND_HMIN	64
void
send_sized(int lock, struct xa_window *wind, short amq, GRECT *r)
{
	if (!(wind->window_status & XAWS_SHADED))
	{
		if (wind->send_message)
		{
			if( r->g_w < WIND_WMIN )
				r->g_w = WIND_WMIN;
			if( r->g_h < WIND_HMIN )
				r->g_h = WIND_HMIN;
			C.move_block = 2;
			wind->send_message(lock, wind, NULL, amq, QMF_CHKDUP,
				WM_SIZED, 0,0, wind->handle,
				r->g_x, r->g_y, r->g_w, r->g_h);
		}
	}
}

void
send_reposed(int lock, struct xa_window *wind, short amq, GRECT *r)
{
	if (wind->send_message)
	{
		C.move_block = 2;
		wind->send_message(lock, wind, NULL, amq, QMF_CHKDUP,
			WM_REPOSED, 0,0, wind->handle,
			r->g_x, r->g_y, r->g_w, r->g_h);
	}
}

static void
send_redraw(int lock, struct xa_window *wind, GRECT *r)
{
	if (!(wind->window_status & (XAWS_SHADED|XAWS_HIDDEN)) && wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_REDRAW, QMF_CHKDUP,
			WM_REDRAW, 0,0, wind->handle,
			r->g_x, r->g_y, r->g_w, r->g_h);
	}
}

void
send_vslid(int lock, struct xa_window *wind, short offs)
{
	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
			WM_VSLID, 0,0, wind->handle,
			offs, 0,0,0);
	}
}
void
send_hslid(int lock, struct xa_window *wind, short offs)
{
	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
			WM_HSLID, 0,0, wind->handle,
			offs, 0,0,0);
	}
}

void
send_closed(int lock, struct xa_window *wind)
{
	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_CLOSED, 0, 0, wind->handle,
				   0,0,0,0);

	}
}

void
setwin_untopped(int lock, struct xa_window *wind, bool snd_untopped)
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
setwin_ontop(int lock, struct xa_window *wind, bool snd_ontop)
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
send_iredraw(int lock, struct xa_window *wind, short xaw, GRECT *r)
{
	if (wind == root_window)
	{
		if (get_desktop()->owner == C.Aes) {
			display_window(lock, 0, wind, r);
		} else if (r) {
			send_app_message(lock, wind, get_desktop()->owner, AMQ_IREDRAW, QMF_NORM,
					 WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
					 r->g_x, r->g_y, r->g_w, r->g_h);
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
					r->g_x, r->g_y, r->g_w, r->g_h);
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
generate_redraws(int lock, struct xa_window *wind, GRECT *r, short flags)
{
	GRECT b;

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
							b.g_x, b.g_y, b.g_w, b.g_h);
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
remove_from_iredraw_queue(int lock, struct xa_window *wind)
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
iconify_window(int lock, struct xa_window *wind, GRECT *r)
{
	if ((r->g_w == -1 && r->g_h == -1) || (!r->g_w && !r->g_h) || (r->g_y + r->g_h + cfg.icnfy_b_y != screen.r.g_h))
		*r = free_icon_pos(lock, NULL);

	move_window(lock, wind, true, XAWS_ICONIFIED, r->g_x, r->g_y, r->g_w, r->g_h);
	wind->window_status &= ~XAWS_CHGICONIF;
}

void
uniconify_window(int lock, struct xa_window *wind, GRECT *r)
{
	move_window(lock, wind, true, ~XAWS_ICONIFIED, r->g_x, r->g_y, r->g_w, r->g_h);
	wind->window_status &= ~XAWS_CHGICONIF;

	/* reinstall the menu - maybe theres a better place for this */
	if( wind->active_widgets & XaMENU )
	{
		XA_WIDGET *widg = get_widget( wind, XAW_MENU );
		if( widg && widg->stuff.wt )
			set_menu_widget( wind, wind->owner, widg->stuff.wt );
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
		ptr_to_shorts(widg->stuff.wt->tree, intin + 2);
		pb.intin = intin;
		pb.intout = intout;

		/* install toolbar */
		XA_wind_set( lock, wind->owner, &pb );
	}

	set_reiconify_timeout(lock);
}

void _cdecl
top_window(int lock, bool snd_untopped, bool snd_ontop, struct xa_window *w)
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
bottom_window(int lock, bool snd_untopped, bool snd_ontop, struct xa_window *w)
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

static XA_WIND_ATTR
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
	int lock,
	SendMessage *message_handler,
	DoWinMesag *message_doer,
	struct xa_client *client,
	bool nolist,
	XA_WIND_ATTR tp,
	WINDOW_TYPE dial,
	int frame,
	bool thinwork,
	const GRECT *R,
	const GRECT *max,
	GRECT *remember)
{
	struct xa_window *w;
	struct xa_widget_theme *wtheme;
	GRECT r;

	r = *R;

	if (max)
	{
		DIAG((D_wind, client, "create_window for %s: r:%d,%d/%d,%d  max:%d,%d/%d,%d",
			c_owner(client), r.g_x,r.g_y,r.g_w,r.g_h,max->g_x,max->g_y,max->g_w,max->g_h));
	}
	else
	{
		DIAG((D_wind, client, "create_window for %s: r:%d,%d/%d,%d  no max",
			c_owner(client), r.g_x,r.g_y,r.g_w,r.g_h));
	}

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

	w->min.g_h = w->min.g_w = WIND_WMIN;
	tp = fix_wind_kind(tp);
	w->requested_widgets = tp;

	if (tp & (UPARROW|DNARROW|LFARROW|RTARROW))
	{
		w->min.g_x = w->min.g_y = -1;
		w->min.g_w = 6 * 16; //cfg.widg_w;
		w->min.g_h = 7 * 16; //cfg.widg_h;
	}
	else
		w->min.g_w = w->min.g_h = 0;

	if (tp & (MENUBAR))
		w->min.g_h += 20;
	if (tp & (INFO))
		w->min.g_h += 10;

	w->rc = w->r = w->pr = r;

	{
		long opts = client->options.wind_opts;

		if (client->options.naes_ff)
			opts |= XAWO_FULLREDRAW;
		w->opts = opts;
	}

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
	w->do_message = message_doer;
	get_widget(w, XAW_TITLE)->stuff.name = client->name;

	w->x_shadow = 1;
	w->y_shadow = 1;
	w->wa_frame = true;

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
		GRECT t;
		t = w2f(&w->rbd, &w->widgets[XAW_TITLE].r, true);
		w->sh = t.g_h + w->y_shadow;
	}
#endif
	if (remember)
		*remember = w->r;

	return w;
}

void
change_window_attribs(int lock,
		struct xa_client *client,
		struct xa_window *w,
		XA_WIND_ATTR tp,
		bool r_is_wa,
		bool insideroot,
		short noleft,
		GRECT r, GRECT *remember)
{
	XA_WIND_ATTR old_tp = w->active_widgets;

	DIAG((D_wind, client, "change_window_attribs for %s: r:%d,%d/%d,%d  no max",
		c_owner(client), r.g_x,r.g_y,r.g_w,r.g_h));

	tp = fix_wind_kind(tp);

	standard_widgets(w, tp, false);

	if (r_is_wa)
	{
		w->r = calc_window(lock, client, WC_BORDER,
				 tp, w->dial,
				 client->options.thinframe,
				 client->options.thinwork,
				 &r);
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
		w->r.g_h = w->sh;

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
		if ((wt = get_widget(w, XAW_TOOLBAR)->stuff.wt) != NULL)
		{
			set_toolbar_coords(w, NULL);
			if (wt->tree && client->p == get_curproc())
			{
				wt->tree->ob_x = w->wa.g_x;
				wt->tree->ob_y = w->wa.g_y;
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

void remove_window_widgets(int lock, int full)
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
		  	WM_SIZED, 0, 0, wind->handle, wind->r.g_x, wind->r.g_y, wind->r.g_w, wind->r.g_h);
		}
		if( memcmp( &wind->r, &screen.r, sizeof(GRECT) ) )
		{
			update_windows_below(lock, &screen.r, NULL, window_list, NULL);
		}
	}
}

int _cdecl
open_window(int lock, struct xa_window *wind, GRECT r)
{
	struct xa_window *wl = window_list;
	GRECT clip, our_win;
	int ignorefocus = 0;

	DIAG((D_wind, wind->owner, "open_window %d for %s to %d/%d,%d/%d status %lx",
		wind->handle, c_owner(wind->owner), r.g_x,r.g_y,r.g_w,r.g_h, wind->window_status));

	if (C.redraws) {
		yield();
	}

	if ((wind->window_status & XAWS_OPEN))
	{
		/* The window is already open, no need to do anything */

		DIAGS(("WARNING: Attempt to open window when it was already open"));
		set_active_client(lock, wind->owner);
		swap_menu(lock|LOCK_DESK, wind->owner, NULL, SWAPM_DESK); // | SWAPM_TOPW);

		return 0;
	}

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
			wind->r.g_h = wind->sh;

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
	if( (wind->window_status & XAWS_NOFOCUS))
		ignorefocus = 1;
	else if( (C.boot_focus && wind->owner->p != C.boot_focus))
		ignorefocus = 2;

#if WITH_BBL_HELP
	if( wind != bgem_window && wind->handle >= 0 && !ignorefocus
		&& !(wind->dial && (created_for_ALERT | created_for_AES | created_for_WDIAL)) )
	{
		xa_bubble( 0, bbl_close_bubble1, 0, 3 );
	}
#endif
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

		if (r.g_w != -1 && r.g_h != -1)
			wind->rc = wind->r = r;

		if ((wind->window_status & XAWS_SHADED))
			wind->r.g_h = wind->sh;

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
			wind->r.g_h = wind->sh;

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
		swap_menu(lock|LOCK_DESK, wind->owner, NULL, SWAPM_DESK );
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

		if( !C.boot_focus )
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
		if( C.boot_focus && wind->send_message )
		{
			int m = 0;
			if( ignorefocus == 0 )
				m = WM_TOPPED;
			else if( ignorefocus == 2 )
				m = WM_UNTOPPED;
			if( m )
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP, m, 0, 0, wind->handle, 0,0,0,0);
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

bool clip_off_menu( GRECT *cl )
{
	short d = get_menu_height() - cl->g_y;
	if( cl->g_h <= d )
		return true;
	cl->g_y = get_menu_height();
	cl->g_h -= d;
	return false;
}

void
draw_window(int lock, struct xa_window *wind, const GRECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	GRECT cl = *clip;

	DIAG((D_wind, wind->owner, "draw_window %d for %s to %d/%d,%d/%d",
		wind->handle, w_owner(wind),
		wind->r.g_x, wind->r.g_y, wind->r.g_w, wind->r.g_h));

	if (!(wind->window_status & XAWS_OPEN) || (wind->dial & created_for_MENUBAR) )
	{
		DIAG((D_wind, wind->owner, "Dwindow %d for %s not open",
			wind->handle, w_owner(wind)));
		return;
	}

	/* is window below menubar? (better should change rectlists?) */
	if( wind->handle >= 0 && cfg.menu_layout == 0 && cfg.menu_bar && cl.g_y < get_menu_height() )
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
		GRECT r;

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
						wind->handle, f, (unsigned long)widg->m.r.draw));

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
				else if( f == XAW_MENU && widg->r.g_w && widg->r.g_h )	/* draw a bar to avoid transparence */
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
find_window(int lock, short x, short y, short flags)
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
get_wind_by_handle(int lock, short h)
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

bool
wind_exist(int lock, struct xa_window *wind)
{
	struct xa_window *w;

	w = window_list;
	while (w)
	{
		if (w == wind)
			break;

		w = w->next;
	}

	if (!w)
	{
		w = S.closed_windows.first;
		while (w)
		{
			if (w == wind)
				break;

			w = w->next;
		}
	}

	return !!w;
}
/*
 * Pull this window to the head of the window list
 */
static struct xa_window *
pull_wind_to_top(int lock, struct xa_window *w)
{
	struct xa_window *wl, *below, *above;
	GRECT clip, r;

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
send_wind_to_bottom(int lock, struct xa_window *w)
{
	struct xa_window *wl;
	GRECT r, clip;

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
	GRECT r = screen.r;
	popout(TAB_LIST_START);
	r.g_h = get_menu_height();

	if( cfg.menu_layout || !cfg.menu_bar )
	{
		GRECT rc = screen.r;
		short mb = cfg.menu_bar;
		cfg.menu_bar = 0;
		if( mb )
		{
			rc.g_x = get_menu_widg()->r.g_w;
			rc.g_w -= rc.g_x;
		}
		rc.g_h = r.g_h;
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
	XA_TREE *wt = xat->stuff.wt;
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
		xaw->r.g_w = xaw->ar.g_w = client->std_menu->area.g_w + 1;
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
		xaw->r.g_h = xaw->ar.g_h = xat->r.g_h = xat->ar.g_h = h + 2;

		if( cfg.menu_bar == 2 || (cfg.menu_bar == 1 && !cfg.menu_layout && !cfg.menu_ontop) )
		{
			root_window->wa.g_h = screen.r.g_h - xaw->r.g_h;
			root_window->wa.g_y = xaw->r.g_h;
		}
		else
		{
			root_window->wa.g_h = screen.r.g_h;
			root_window->wa.g_y = 0;
		}
		if( menu_window && cfg.menu_bar != 2 && cfg.menu_ontop && cfg.menu_bar )
		{
			menu_window->r.g_w = xaw->r.g_w;
			menu_window->r.g_h = xaw->r.g_h;
			if( menu_window->window_status & XAWS_OPEN)
			{
				move_window( 0, menu_window, true, 0, menu_window->r.g_x, menu_window->r.g_y, menu_window->r.g_w, menu_window->r.g_h );
				redraw_menu_area();
			}
		}

		//GRECT rc = screen.r;
		//update_windows_below(0, &rc, &rc, window_list, NULL);
		root_window->rwa = root_window->wa;

		if (get_desktop()->owner == C.Aes)
		{
			wt->tree->ob_height = root_window->wa.g_h;
			wt->tree->ob_y = root_window->wa.g_y;
		}
	}
}

/*
 * md=-2: off if was -1
 * md=-1: on temp.
 * else: set md
 */
void toggle_menu(int lock, short md)
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
move_window(int lock, struct xa_window *wind, bool blit, WINDOW_STATUS newstate, short X, short Y, short W, short H)
{
	IFDIAG(struct xa_client *client = wind->owner;)
	GRECT old, new;

	DIAG((D_wind,client,"move_window(%s) %d for %s from %d/%d,%d/%d to %d/%d,%d,%d",
	      (wind->window_status & XAWS_OPEN) ? "open" : "closed",
	      wind->handle, c_owner(client), wind->r.g_x,wind->r.g_y,wind->r.g_w,wind->r.g_h, X,Y,W,H));


	if (wind->owner->status & CS_EXITING)
		return;

	new.g_x = X;
	new.g_y = Y;
	new.g_w = W;
	new.g_h = H;
	old = wind->r;

	if( BELOW_FOREIGN_MENU(old.g_y) )
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
					if (wind->window_status & XAWS_SHADED)
					{
						DIAGS(("move_window: %d/%d/%d/%d - uniconify shaded window",
							new.g_x, new.g_y, new.g_w, new.g_h));
					}
				}
			}
			if (!(newstate & XAWS_SHADED))
			{
				DIAGS(("move_window: clear shaded"));
			}
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
			if (newstate & XAWS_SHADED)
			{
				DIAGS(("move_window: set shaded"));
			}
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
		if (wind->window_status & XAWS_SHADED)
		{
			DIAGS(("move_window: win is shaded"));
		}
		wind->pr = wind->rc;
	}

	inside_root(&new, wind->owner->options.noleft);

	if( !cfg.menu_ontop && (cfg.menu_bar&1) && old.g_y != wind->r.g_y && old.g_y < get_menu_height() )
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
		short y = old.g_y < new.g_y ? old.g_y : new.g_y;
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
close_window(int lock, struct xa_window *wind)
{
	struct xa_window *wl, *w = NULL;
	struct xa_client *client = wind->owner;
	GRECT r;
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
			wind->handle, (unsigned long)wind->background));


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

		if (is_top)
		{
			if (wind->active_widgets & STORE_BACK)
			{
				(*xa_vdiapi->form_restore)(0, wind->r, &(wind->background));
				return true;
			}

			w = window_list;

			while (w && w != root_window)
			{
				if ((w->owner == client || (!(w->dial & created_for_AES) && (client == C.Hlp)))
					&& !(w->window_status & (XAWS_ICONIFIED|XAWS_HIDDEN|XAWS_NOFOCUS)) )
				{
					if (w != TOP_WINDOW)
					{
						top_window(lock|LOCK_WINLIST, true, true, w);
					}
					else
					{
						setnew_focus(w, NULL, true, true, true);
						set_and_update_window(w, true, true, NULL);
					}
					wl = w->next;
					break;
				}
				w = w->next;
			}

			if (w == root_window || (w->owner->status & CS_EXITING))
				w = NULL;
		}

		if (!C.shutdown && !ignorefocus && TOP_WINDOW && is_top && !w
			&& !(dial & (created_for_WDIAL|created_for_FORM_DO|created_for_FMD_START)) )
		{
			switch (client->options.clwtna)
			{
				case 0: 	/* Keep active client ontop */
				{
					break;
				}
				default:
				case 1: /* Top app who owns window below closed one */
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
						top_window(lock, true, true, w);
					}
					else if (w == root_window)
					{
						app_in_front(lock, desktop_owner(), true, true, true);
					}
					else
					{
						app_in_front(lock, previous_client(lock, 1), true, true, false);
					}
					break;
				}
				case 2: /* Top the client previously topped */
				{
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
delete_window1(int lock, struct xa_window *wind)
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
			wind->destructor(lock|LOCK_WINLIST, wind);

		free_standard_widgets(wind);
		free_wind_handle(wind->handle);

		if (wind->background)
			kfree(wind->background);

	}
	else
	{
		DIAGS(("delete_window1: nolist window %d, bgk=%lx", wind->handle, (unsigned long)wind->background));
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
CE_delete_window(int lock, struct c_event *ce, short cancel)
{
	/*
	 * Since the window in this event already have been removed from
	 * all stacks, we perform  this deletion even when cancel flag is set
	 */
#if 0
	if (!cancel)
#endif
		delete_window1(lock, ce->ptr1);
}

void _cdecl
delete_window(int lock, struct xa_window *wind)
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
delayed_delete_window(int lock, struct xa_window *wind)
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
do_delayed_delete_window(int lock)
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
display_window(int lock, int which, struct xa_window *wind, GRECT *clip)
{
	if (!(wind->owner->status & CS_EXITING) && (wind->window_status & XAWS_OPEN))
	{
		{
			struct xa_rect_list *rl;
			GRECT d;

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
update_all_windows(int lock, struct xa_window *wl)
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
update_windows_below(int lock, const GRECT *old, GRECT *new, struct xa_window *wl, struct xa_window *wend)
{
	GRECT clip;
	int wlock = lock | LOCK_WINLIST;

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
				GRECT d;

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
			(unsigned long)msg, buf->m[3], buf->m[4], buf->m[5], buf->m[6], buf->m[7], c_owner(client) ));

		kfree(msg);
		rtn = 1;
// 		kick_mousemove_timeout();
	}
// 	else if (C.redraws)
// 		yield();
	return rtn;
}

void
redraw_client_windows(int lock, struct xa_client *client)
{
	struct xa_window *wl;
	struct xa_rect_list *rl;
	union msg_buf buf;
	bool makerl = true;
	GRECT r;

	while (get_lost_redraw_msg(client, &buf))
	{
		if (get_desktop()->owner == client)
		{
			wl = root_window;
			if (makerl) make_rect_list(wl, true, RECT_SYS);
			rl = wl->rect_list.start;
			while (rl)
			{
				if (xa_rect_clip(&rl->r, (GRECT *)&buf.m[4], &r))
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
					if (xa_rect_clip(&rl->r, (GRECT *)&buf.m[4], &r))
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

GRECT
w2f(GRECT *d, const GRECT *in, bool chkwh)
{
	GRECT r;
	r.g_x = in->g_x - d->g_x; //(wind->rwa.g_x - wind->r.g_x);
	r.g_y = in->g_y - d->g_y; //(wind->rwa.g_y - wind->r.g_y);
	r.g_w = in->g_w + d->g_w; //(wind->r.g_w - wind->rwa.g_w);
	r.g_h = in->g_h + d->g_h; //(wind->r.g_h - wind->rwa.g_h);
	if (chkwh && (r.g_w <= 0 || r.g_h <= 0))
		r.g_w = r.g_h = 0;
	return r;
}

GRECT
f2w(GRECT *d, const GRECT *in, bool chkwh)
{
	GRECT r;
	r.g_x = in->g_x + d->g_x; //(wind->rwa.g_x - wind->r.g_x);
	r.g_y = in->g_y + d->g_y; //(wind->rwa.g_y - wind->r.g_y);
	r.g_w = in->g_w - d->g_w; //(wind->r.g_w - wind->rwa.g_w);
	r.g_h = in->g_h - d->g_h; //(wind->r.g_h - wind->rwa.g_h);
	if (chkwh && (r.g_w <= 0 || r.g_h <= 0))
		r.g_w = r.g_h = 0;

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
Xpolate(GRECT *r, GRECT *o, GRECT *i)
{
	/* If you want to prove the maths here, draw two boxes one inside
	 * the other, then sit and think about it for a while...
	 * HR: very clever :-)
	 */
	r->g_x = 2 * o->g_x - i->g_x;
	r->g_y = 2 * o->g_y - i->g_y;
	r->g_w = 2 * o->g_w - i->g_w;
	r->g_h = 2 * o->g_h - i->g_h;
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

GRECT
calc_window(int lock, struct xa_client *client, int request, XA_WIND_ATTR tp, WINDOW_TYPE dial, int thinframe, bool thinwork, const GRECT *r)
{
	struct xa_window *w_temp;
	struct xa_wc_cache *wcc;
	short class;
	GRECT o;
	DIAG((D_wind,client,"calc %s from %d/%d,%d/%d", request ? "work" : "border", r->g_x, r->g_y, r->g_w, r->g_h));
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
				o = w2f(&wcc->delta, r, false);
				break;
			}
			default:
			{
				o = f2w(&wcc->delta, r, false);
				break;
			}
		}
	}
	else
	{
		o = *r;
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
	short y11 = newrl->r.g_y, y12 = newrl->r.g_y + newrl->r.g_h, y21 = newrl->next->r.g_y, y22 = newrl->next->r.g_y + newrl->next->r.g_h,
		x12 = newrl->r.g_x + newrl->r.g_w, x21 = newrl->next->r.g_x, x22 = newrl->next->r.g_x + newrl->next->r.g_w, x11 = newrl->r.g_x,
		way1 = wind->wa.g_y, way2 = wind->wa.g_y + wind->wa.g_h;

	/* two x-adjacend rects; y equal or outside wa: join */
	if( (y11 == y21 || (y11 < way1 && y21 < way1)) && (y12 == y22 || (y12 >= way2 && y22 >= way2)) && (x21 == x12 || x22 == x11) )
	{
		GRECT r;
		r.g_y = y11 < y21 ? y11 : y21;
		r.g_h = y12 > y22 ? y12 : y22;
		r.g_h -= r.g_y;
		r.g_x = x11 < x21 ? x11 : x21;
		r.g_w = newrl->r.g_w + newrl->next->r.g_w;

		generate_redraws(wlock, wind, &r, flags );
		return true;
	}
	return false;
}

static void
set_and_update_window(struct xa_window *wind, bool blit, bool only_wa, GRECT *new)
{
	short dir, resize, xmove, ymove, wlock = 0;
	struct xa_rect_list *oldrl, *orl, *newrl, *brl, *prev, *next, *rrl, *nrl;
	GRECT bs, bd, old, wa, oldw;

	DIAGS(("set_and_update_window:"));

	old = wind->r;
	oldw = wind->wa;

	if (new)
	{
		wind->rc = wind->t = *new;
		if ((wind->window_status & XAWS_SHADED))
		{
			new->g_h = wind->sh;
		}
		wind->r = *new;
		calc_work_area(wind);
	}
	else
		new = &wind->r;

	wa = wind->wa;

	xmove = new->g_x - old.g_x;
	ymove = new->g_y - old.g_y;

	resize	= new->g_w != old.g_w || new->g_h != old.g_h ? 1 : 0;

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
			XA_TREE *wt = widg->stuff.wt;

			/* Temporary hack 070702 */
			if (wt && wt->tree)
			{
				/* this moves the origin of the window -
					 objects (e.g.buttons) have to be moved by the client
				*/
				wt->tree->ob_x = wind->wa.g_x;
				wt->tree->ob_y = wind->wa.g_y;
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
				generate_redraws(wlock, wind, (GRECT *)&wind->r, !only_wa ? RDRW_ALL : RDRW_WA);
			}
		}

		return;
	}
#endif
	oldrl = wind->rect_list.start;
	wind->rect_list.start = wind->rect_list.next = NULL;
	newrl = make_rect_list(wind, false, RECT_SYS);
	wind->rect_list.start = newrl;

	//blit = 1;
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

		oldw.g_x -= old.g_x;
		oldw.g_y -= old.g_y;

		wa.g_x = wind->wa.g_x - new->g_x;
		wa.g_y = wind->wa.g_y - new->g_y;
		wa.g_w = wind->wa.g_w;
		wa.g_h = wind->wa.g_h;

		/* increase width of workarea by vslider-width if toolbar installed */
		if( (wind->active_widgets & TOOLBAR) && (wind->active_widgets & VSLIDE) )
		{
			XA_WIDGET *widg = get_widget(wind, XAW_VSLIDE);
			wa.g_w += widg->ar.g_w;
 		}

		/*
		 * Convert to relative coords
		 */
		orl = oldrl;
		prev = NULL;
		while (orl)
		{
			orl->r.g_x -= old.g_x;
			orl->r.g_y -= old.g_y;

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
			bs.g_x = newrl->r.g_x - new->g_x;
			bs.g_y = newrl->r.g_y - new->g_y;
			bs.g_w = newrl->r.g_w;
			bs.g_h = newrl->r.g_h;

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
								ox2 = bd.g_x + bd.g_w;
								oy2 = bd.g_y + bd.g_h;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( (bd.g_x < n->r.g_x && ox2 <= n->r.g_x) ||
									     ((n->r.g_x + n->r.g_w) > bd.g_x && oy2 <= n->r.g_y) )
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
								ox2 = bd.g_x + bd.g_w;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( (bd.g_x < n->r.g_x && ox2 <= n->r.g_x) ||
									     ((n->r.g_x + n->r.g_w) > bd.g_x && (n->r.g_y + n->r.g_h) <= bd.g_y) )
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
								ox2 = bd.g_x + bd.g_w;
								oy2 = bd.g_y + bd.g_h;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( (ox2 > (n->r.g_x + n->r.g_w) && bd.g_x >= (n->r.g_x + n->r.g_w)) ||
									     ((n->r.g_x + n->r.g_w) < bd.g_x && oy2 <= n->r.g_y) )
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
								oy2 = bd.g_y + bd.g_h;
								n = brl;
								p = NULL;
								while (n)
								{
									if ( oy2 >= n->r.g_y &&
									     bd.g_x >= n->r.g_x
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
					if( menu_window && cfg.menu_bar && (dir & 1) && nrl->r.g_y < menu_window->r.g_h )
						if( nrl->next && !nrl->next->next && nrl->r.g_y + nrl->r.g_h == nrl->next->r.g_y && !trl )
						{
							trl = nrl;
							nrl = nrl->next;
							dir = 0;
						}
					bd = nrl->r;
					bs.g_x = bd.g_x + new->g_x;
					bs.g_y = bd.g_y + new->g_y;
					bs.g_w = bd.g_w;
					bs.g_h = bd.g_h;
					bd.g_x += old.g_x;
					bd.g_y += old.g_y;
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
				nrl->r.g_x = newrl->r.g_x - new->g_x;
				nrl->r.g_y = newrl->r.g_y - new->g_y;
				nrl->r.g_w = newrl->r.g_w;
				nrl->r.g_h = newrl->r.g_h;


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
					short bx2 = orl->r.g_x + orl->r.g_w;
					short by2 = orl->r.g_y + orl->r.g_h;

					//DIAGS(("BLITRECT (orl=%lx, nxt=%lx)(nrl=%lx, nxt=%lx) %d/%d/%d/%d, x2/y2=%d/%d",
					//	orl, (long)orl->next, nrl, (long)nrl->next, orl->r, bx2, by2));

					for (rrl = nrl, prev = NULL; rrl; rrl = next)
					{
						GRECT our = rrl->r;
						w = orl->r.g_x - rrl->r.g_x;
						h = orl->r.g_y - rrl->r.g_y;

						//DIAGS(("CLIPPING %d/%d/%d/%d, w/h=%d/%d", our, w, h));
						//DIAGS(("rrl = %lx", rrl));

						next = rrl->next;

						if (   h < our.g_h   &&
						       w < our.g_w   &&
						     bx2 > our.g_x   &&
						     by2 > our.g_y )
						{
							short nx2 = our.g_x + our.g_w;
							short ny2 = our.g_y + our.g_h;

							flag = 0;
							if (orl->r.g_y > our.g_y)
							{
								rrl->r.g_x = our.g_x;
								rrl->r.g_y = our.g_y;
								rrl->r.g_w = our.g_w;
								rrl->r.g_h = h;
								our.g_y += h;
								our.g_h -= h;
								flag = 1;
								//DIAGS((" -- 1. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
								//	rrl->r, our));
							}
							if (orl->r.g_x > our.g_x)
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
								rrl->r.g_x = our.g_x;
								rrl->r.g_y = our.g_y;
								rrl->r.g_h = our.g_h;
								rrl->r.g_w = w;
								our.g_x += w;
								our.g_w -= w;
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
								rrl->r.g_x = our.g_x;
								rrl->r.g_y = by2;
								rrl->r.g_w = our.g_w;
								rrl->r.g_h = ny2 - by2;
								our.g_h -= rrl->r.g_h;
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
								rrl->r.g_x = bx2;
								rrl->r.g_y = our.g_y;
								rrl->r.g_w = nx2 - bx2;
								rrl->r.g_h = our.g_h;
								our.g_w -= rrl->r.g_w;
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
					struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
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
								lwind->r.g_x + xmove, lwind->r.g_y + ymove, lwind->r.g_w, lwind->r.g_h);
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
					nrl->r.g_x += new->g_x;
					nrl->r.g_y += new->g_y;
					//DIAGS(("redrawing area (%lx) %d/%d/%d/%d",
					//	nrl, nrl->r));
					DIAGS(("redrawing area (%lx) %d/%d/%d/%d", (unsigned long)nrl, nrl->r.g_x, nrl->r.g_y, nrl->r.g_w, nrl->r.g_h));
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
