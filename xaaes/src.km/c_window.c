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
#include "desktop.h"
#include "k_main.h"
#include "menuwidg.h"
#include "draw_obj.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "widgets.h"
#include "xa_graf.h"

#include "mint/signal.h"

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
 * Find first free position for iconification.
 */
RECT
free_icon_pos(enum locks lock)
{
	RECT ic;
	int i = 0;

	for (;;)
	{
		struct xa_window *w = window_list;
		ic = iconify_grid(i++);

		/* find first unused position. */
		while (w)
		{
			if (w->window_status == XAWS_ICONIFIED)
			{
				
				if (w->r.x == ic.x && w->r.y == ic.y)
					/* position occupied; advance with next position in grid. */
					break;
			}
			w = w->next;
		}

		if (!w)
			/* position not in list of iconified windows, so it is free. */
			break;
	}

	return ic;
}

/*
 * Check if a new menu and/or desktop is needed
 */
static void
check_menu_desktop(enum locks lock, struct xa_window *old_top, struct xa_window *new_top)
{
	/* If we're getting a new top window, we may need to swap menu bars... */
	if (old_top && (old_top == root_window || old_top->owner != new_top->owner))
	{
		Sema_Up(desk);

		DIAG((D_appl, NULL, "check_menu_desktop from %s to %s",
			old_top->owner->name, new_top->owner->name));

		set_active_client(lock, new_top->owner);
		swap_menu(lock|desk, new_top->owner, true, 2);

		Sema_Dn(desk);
	}
}

#if 0
/* not used??? */

static RECT
wa_to_curr(struct xa_window *wi, int d, RECT r)
{
	r.x += wi->bd.x - d;
	r.y += wi->bd.y - d;
	r.w += wi->bd.w + d+d;
	r.h += wi->bd.h + d+d;
	return r;
}

static RECT
bound_inside(RECT r, RECT o)
{
	if (r.x       < o.x      ) r.x  = o.x;
	if (r.y       < o.y      ) r.y  = o.y;
	if (r.x + r.w > o.x + o.w) r.w -= (r.x + r.w) - (o.x + o.w);
	if (r.y + r.h > o.y + o.h) r.h -= (r.y + r.h) - (o.y + o.h);
	return r;
}
#endif

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
inside_root(RECT *r, struct options *o)
{
	if (r->y < root_window->wa.y)
		r->y = root_window->wa.y;

	if (o->noleft)
		if (r->x < root_window->wa.x)
			r->x = root_window->wa.x;
}

static void
wi_put_first(struct win_base *b, struct xa_window *w)
{
	if (b->first)
	{
		w->next = b->first;
		w->prev = NULL;
		b->first->prev = w;
	}
	else
	{
		b->last = w;
		w->next = w->prev = NULL;
	}
	b->first = w;
#if 0
	w = b->first;
	while (w)
	{
		DIAGS(("pf: %d pr:%d", w->handle, w->prev ? w->prev->handle : -1));
		w = w->next;
	}
#endif
}

#if 0
/* not used??? */

static void
wi_put_last(WIN_BASE *b, struct xa_window *w)
{
	if (b->last)
	{
		w->prev = b->last;
		b->last->next = w;
		b->last = w;
		w->next = NULL;
	}
	else
	{
		b->first = b->last = w;
		w->next = w->prev = NULL;
	}
#if 0
	w = b->first;
	while (w)
	{
		DIAGS(("pl: %d pr:%d", w->handle, w->prev ? w->prev->handle : -1));
		w = w->next;
	}
#endif
}
#endif

/* This a special version of put last; it puts w just befor last. */
static void
wi_put_blast(struct win_base *b, struct xa_window *w)
{
	if (b->last)
	{
		struct xa_window *l = b->last;
		w->next = l;
		w->prev = l->prev;
		if (l->prev)
			l->prev->next = w;
		l->prev = w;
	}
	else
	{
		b->first = b->last = w;
		w->next = w->prev = NULL;
	}
#if 0
	w = b->first;
	while (w)
	{
		DIAGS(("pl: %d pr:%d", w->handle, w->prev ? w->prev->handle : -1));
		w = w->next;
	}
#endif
}

static void
wi_remove(struct win_base *b, struct xa_window *w)
{
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
#if 0
	w = b->first;
	while (w)
	{
		DIAGS(("rem: %d pr:%d", w->handle, w->prev ? w->prev->handle : -1));
		w = w->next;
	}
#endif
}

#if 0
/* not used??? */

static void
wi_insert(WIN_BASE *b, struct xa_window *w, struct xa_window *after)
{
	if (after)
	{
		if (after != b->last)
		{
			w->next = after->next;
			w->prev = after;
			if (after->next)
			{
				if (after->next == b->last)
					b->last = w;
				after->next->prev = w;
			}
			after->next = w;
		}
	}
	else
		wi_put_first(b, w);
#if 0
	w = b->first;
	while (w)
	{
		DIAGS(("rem: %d pr:%d", w->handle, w->prev ? w->prev->handle : -1)i);
		w = w->next;
	}
#endif
}
#endif


bool
is_topped(struct xa_window *wind)
{

	DIAGS(("is_topped: app infront %s, window_list owner %s",
		APP_LIST_START->name, window_list->owner->name));

	if (wind->owner != APP_LIST_START)
		return false;

	if (wind != window_list)
		return false;

#if 0
	if (wind != C.focus)
		return false;
#endif

	return true;
}

struct xa_window *
get_top(void)
{
	return S.open_windows.first;
}

bool
is_hidden(struct xa_window *wind)
{
	RECT d = root_window->r;
	return !xa_rc_intersect(wind->r, &d);
}

bool
unhide(struct xa_window *wind, short *x, short *y)
{
	RECT r = wind->r;
	bool done = false;

	if (is_hidden(wind))
	{
		RECT d = root_window->r;

		while (r.x + r.w < d.x)
			r.x += d.w;
		while (r.y + r.h < d.y)
			r.y += d.h;
		if (r.y < root_window->wa.y)
			r.y = root_window->wa.y;	/* make shure the mover becomes visible. */
		while (r.x > d.x + d.w)
			r.x -= d.w;
		while (r.y > d.y + d.h)
			r.y -= d.h;
		done = true;
	}
	*x = r.x;
	*y = r.y;
	return done;
}

void
unhide_window(enum locks lock, struct xa_window *wind)
{
	RECT r = wind->r;
	if (unhide(wind, &r.x, &r.y))
	{
		//if (wind->window_status == XAWS_ICONIFIED)
		//	r = free_icon_pos(lock);

		if (wind->send_message)
			wind->send_message(lock, wind, NULL,
					   WM_MOVED, 0, 0, wind->handle,
					   r.x, r.y, r.w, r.h);
		else
			move_window(lock, wind, -1, r.x, r.y, r.w, r.h);
	}
}

/* SendMessage */
void
send_untop(enum locks lock, struct xa_window *wind)
{
	struct xa_client *client = wind->owner;

	if (wind->send_message && !client->fmd.wind)
		wind->send_message(lock, wind, NULL,
				   WM_UNTOPPED, 0, 0, wind->handle,
				   0, 0, 0, 0);
}

void
send_ontop(enum locks lock)
{
	struct xa_window *top = window_list;
	struct xa_client *client = top->owner;

	if (is_topped(top) && top->send_message && !client->fmd.wind)
		top->send_message(lock, top, NULL,
				  WM_ONTOP, 0, 0, top->handle,
				  0, 0, 0, 0);
}

/*
 * Create a window
 *
 * introduced pid -1 for temporary windows created basicly only to be able to do
 * calc_work_area and center.
 *
 * needed a dynamiccally sized frame
 */
struct xa_window *
create_window(
	enum locks lock,
	SendMessage *message_handler,
	DoWinMesag *message_doer,
	struct xa_client *client,
	bool nolist,
	XA_WIND_ATTR tp,
	WINDOW_TYPE dial,
	int frame,
	int thinframe,
	bool thinwork,
	const RECT r,
	const RECT *max,
	RECT *remember)
{
	struct xa_window *w;

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
	if (!w)
		/* Unable to allocate memory for window? */
		return NULL;

	bzero(w, sizeof(*w));

	/*
	 * Initialize the widget types values
	 */
	{
		int i;

		for (i = 0; i < XA_MAX_WIDGETS; i++)
			w->widgets[i].type = i;
	}

	/* avoid confusion: if only 1 specified, give both (fail safe!) */
	if ((tp & UPARROW) || (tp & DNARROW))
		tp |= UPARROW|DNARROW;
	if ((tp & LFARROW) || (tp & RTARROW))
		tp |= LFARROW|RTARROW;
	/* cant hide a window that cannot be moved. */
	if ((tp & MOVER) == 0)
		tp &= ~HIDE;
	/* temporary until solved. */
	if (tp & MENUBAR)
		tp |= XaMENU;

	/* implement maximum rectangle (needed for at least TosWin2) */
	w->max = max ? *max : root_window->wa;
		
	w->r  = r;
	w->pr = w->r;

	frame = thinframe;
#if 0
	if (!MONO && frame > 0)
	{
		if (thinframe < 0)
			frame = 1;
	}
#endif

#if 0
	/* implement border sizing. */
	if ((tp & (SIZE|MOVE)) == (SIZE|MOVE))
#endif
#if 0
		if (frame > 0 && thinframe > 0)
			/* see how well it performs. */
			frame += thinframe;
#endif

	w->frame = frame;
	w->thinwork = thinwork;
	w->owner = client;
	w->rect_user = w->rect_list = w->rect_start = NULL;
	w->handle = -1;
	w->window_status = XAWS_CLOSED;
	w->remember = remember;
	w->nolist = nolist;
	w->dial = dial;
	w->send_message = message_handler;
	w->do_message	= message_doer;
	get_widget(w, XAW_TITLE)->stuff = client->name;

	if (nolist)
	{
		/* Dont put in the windowlist */

		/* Attach the appropriate widgets to the window */
		DIAGS((" -- nolist window"));
		standard_widgets(w, tp, false);
	}
	else
	{
		w->handle = new_wind_handle();
		DIAG((D_wind, client, " allocated handle = %d", w->handle));

		wi_put_first(&S.closed_windows, w);
		DIAG((D_wind,client," inserted in closed_windows list"));

		/* Attach the appropriate widgets to the window */
		standard_widgets(w, tp, false);

		/* If STORE_BACK extended attribute is used, window preserves its own background */
		if (tp & STORE_BACK)
		{
			DIAG((D_wind,client," allocating background storage buffer"));
			w->background = kmalloc(calc_back(&r, screen.planes));
		}
		else
			w->background = NULL;

	}

	calc_work_area(w);

	if (remember)
		*remember = w->r;

	return w;
}

void
change_window_attribs(enum locks lock,
		struct xa_client *client,
		struct xa_window *w,
		XA_WIND_ATTR tp,
		RECT r, RECT *remember)
{
	//struct xa_window *w;
	XA_WIND_ATTR old_tp = w->active_widgets;

	DIAG((D_wind, client, "change_window_attribs for %s: r:%d,%d/%d,%d  no max",
		c_owner(client), r.x,r.y,r.w,r.h));

	/* avoid confusion: if only 1 specified, give both (fail safe!) */
	if ((tp & UPARROW) || (tp & DNARROW))
		tp |= UPARROW|DNARROW;
	if ((tp & LFARROW) || (tp & RTARROW))
		tp |= LFARROW|RTARROW;
	/* cant hide a window that cannot be moved. */
	if ((tp & MOVER) == 0)
		tp &= ~HIDE;
	/* temporary until solved. */
	if (tp & MENUBAR)
		tp |= XaMENU;

	w->r = r;

	w->rect_user = w->rect_list = w->rect_start = NULL;

	/* Attach the appropriate widgets to the window */
	standard_widgets(w, tp, false);

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
		w->background = kmalloc(calc_back(&r, screen.planes));
	}

	calc_work_area(w);

	/*
	 * Now standard widgets are set and workarea of window
	 * is recalculated.
	 * Its time to do non-standard things depending on correct
	 * workarea coordinates now.
	 */

	if ( get_widget(w, XAW_TOOLBAR)->stuff )
	{
		set_toolbar_coords(w);
	}

	if (remember)
		*remember = w->r;
}
int
open_window(enum locks lock, struct xa_window *wind, RECT r)
{
	struct xa_window *wl = window_list;
	RECT clip, our_win;

	DIAG((D_wind, wind->owner, "open_window %d for %s to %d/%d,%d/%d status %x",
		wind->handle, c_owner(wind->owner), r.x,r.y,r.w,r.h, wind->window_status));

	if (C.redraws)
		yield();

#if 0
	if (r.w <= 0) r.w = 6 * cfg.widg_w;
	if (r.h <= 0) r.h = 6 * cfg.widg_h;
#endif

	if (wind->is_open == true)
	{
		/* The window is already open, no need to do anything */

		DIAGS(("WARNING: Attempt to open window when it was already open"));
		return 0;
	}

	if (wind->nolist)
	{
		DIAGS(("open_window: nolist window"));
		C.focus = wind;
		wind->r = r;
		wind->is_open = true;
		wind->window_status = XAWS_OPEN;
		calc_work_area(wind);
		make_rect_list(wind, true);

		if (wind->active_widgets & STORE_BACK)
		{
			form_save(0, wind->r, &(wind->background));
		}

		draw_window(lock|winlist, wind);

		if (wind->send_message)
			wind->send_message(lock|winlist, wind, NULL,
					   WM_REDRAW, 0, 0, wind->handle,
					   wind->r.x, wind->r.y, wind->r.w, wind->r.h);
		/* dont open unlisted windows */
		return 1;
	}

	wi_remove(&S.closed_windows, wind);
	wi_put_first(&S.open_windows, wind);

	C.focus = wind;

	/* New top window - change the cursor to this client's choice */
	graf_mouse(wind->owner->mouse, wind->owner->mouse_form, false);

	if (wind->window_status == XAWS_ICONIFIED)
	{
		if (r.w != -1 && r.h != -1)
			wind->r = r;

		if (wind != root_window)
			inside_root(&wind->r, &wind->owner->options);
	}
	else
	{
		/* Change the window coords */
		wind->r = r;
		if (wind != root_window)
			inside_root(&wind->r, &wind->owner->options);

		wind->window_status = XAWS_OPEN;
	}

	/* Flag window as open */
	wind->is_open = true;

	calc_work_area(wind);

	check_menu_desktop(lock|winlist, wl, wind);

	make_rect_list(wind, true);
	
	/* Is this a 'preserve own background' window? */
	if (wind->active_widgets & STORE_BACK)
	{
		form_save(0, wind->r, &(wind->background));
		/* This is enough, it is only for TOOLBAR windows. */
		draw_window(lock|winlist, wind);
		redraw_menu(lock);
	}
	else
	{
		our_win = wind->r;
	
		wl = wind;
		while (wl)
		{
			clip = wl->r;

			if (xa_rc_intersect(our_win, &clip))
				make_rect_list(wl, 1);

			wl = wl->next;
		}

		/* streamlining the topping */
		after_top(lock|winlist, true);

		/* Display the window using clipping rectangles from the rectangle list */
		display_window(lock|winlist, 10, wind, NULL);
		redraw_menu(lock);

		if (wind->send_message)
			wind->send_message(lock|winlist, wind, NULL,
					   WM_REDRAW, 0, 0, wind->handle,
					   wind->r.x, wind->r.y, wind->r.w, wind->r.h);
	}

	DIAG((D_wind, wind->owner, "open_window %d for %s exit with 1",
		wind->handle, c_owner(wind->owner)));
	return 1;
}

static void
if_bar(short pnt[4])
{
	//if ((pnt[2] - pnt[0]) !=0 && (pnt[3] - pnt[1]) != 0)
	if ((pnt[2] - pnt[0]) >= 0 && (pnt[3] - pnt[1]) >= 0)
		v_bar(C.vh, pnt);
}

void
do_rootwind_msg(
	struct xa_window *wind,
	struct xa_client *to,			/* if different from wind->owner */
	short *msg)
{
	DIAG((D_form, wind->owner, "do_rootwind_msg: wown %s, to %s, msg %d, %d, %d, %d, %d, %d, %d, %d",
		wind->owner->name, to->name, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]));

	switch (msg[0])
	{
		case WM_REDRAW:
		{
			XA_WIDGET *widg = get_widget(root_window, XAW_TOOLBAR);
			
			if (get_desktop()->owner == to && widg->display)
			{
				//if (to != C.Aes)
				//	lock_screen(to, false, NULL, 3);
				hidem();
				set_clip((RECT *)&msg[4]);
				widg->display(0, root_window, widg);
				clear_clip();
				showm();
				//if (to != C.Aes)
				//	unlock_screen(to, 4);
			}
			break;
		}
	}
}

void
draw_window(enum locks lock, struct xa_window *wind)
{
	//struct xa_window *wind = (struct xa_window *)ce->ptr1;

	DIAG((D_wind, wind->owner, "draw_window %d for %s to %d/%d,%d/%d",
		wind->handle, w_owner(wind),
		wind->r.x, wind->r.y, wind->r.w, wind->r.h));

	if (!wind->is_open)
	{
		DIAG((D_wind, wind->owner, "Dwindow %d for %s not open",
			wind->handle, w_owner(wind)));
		return;
	}

	l_color(G_BLACK);
	hidem();

	/* Dont waste precious CRT glass */
	if (wind != root_window)
	{
		RECT cl = wind->r;
		RECT wa = wind->wa;

		/* Display the window backdrop (borders only, GEM style) */

		cl.w-=SHADOW_OFFSET;
		cl.h-=SHADOW_OFFSET;
#if 0
		if (wind->active_widgets & TOOLBAR)
		{
			/* whole plane */

			if (wind->frame > 0)
				if (MONO)
					p_gbar(0, &cl);
				else
					gbar  (0, &cl);		
		}
		else
#endif
		if ((wind->active_widgets & V_WIDG) == 0)
		{
			if (wind->frame > 0)
				gbox(0, &cl);
		}
		else
		{
			short pnt[8];
			RECT tcl;

			f_color(screen.dial_colours.bg_col);
			f_interior(FIS_SOLID);

			tcl = cl;
			if (wind->frame > 0)
			{
				l_color(9);
				gbox(0, &cl);
				tcl.x++;
				tcl.y++;
				tcl.w -= 2;
				tcl.h -= 2;
			}
			
			pnt[0] = tcl.x;
			pnt[1] = tcl.y;
			pnt[2] = tcl.x + tcl.w - 1;
			pnt[3] = wa.y - 1;
			if_bar(pnt); 			/* top */
			pnt[1] = wa.y + wa.h;
			pnt[3] = tcl.y + tcl.h - 1;
			if_bar(pnt);			/* bottom */
			pnt[0] = tcl.x;
			pnt[1] = tcl.y;
			pnt[2] = wa.x - 1;
			if_bar(pnt);			/* left */
			pnt[0] = wa.x + wa.w;
			pnt[2] = tcl.x + tcl.w - 1;
			if_bar(pnt);			/* right */

			/* Display the work area */

			if (MONO)
			{
				if (wind->frame > 0)
					gbox(0, &cl);
				//gbox(1, &wa);
			}
			else
			{
				if (wind->thinwork)
				{
					//l_color(6);
					//gbox(1, &wa);
				}
				else
				{
					RECT nw = wa;
					nw.w++;
					nw.h++;
					br_hook(2, &nw, screen.dial_colours.shadow_col);
					tl_hook(2, &nw, screen.dial_colours.lit_col);
					br_hook(1, &nw, screen.dial_colours.lit_col);
					tl_hook(1, &nw, screen.dial_colours.shadow_col);
				}
			}
		}

#if 0
		/* for outlined windowed objects */
		if (wind->outline_adjust)
		{
			l_color(screen.dial_colours.bg_col);
			gbox( 0, &wa);
			gbox(-1, &wa);
			gbox(-2, &wa);
		}
#endif
		if (wind->frame >= 0)
		{
			shadow_object(0, OS_SHADOWED, &cl, G_BLACK, SHADOW_OFFSET/2);

#if 0
			/* only usefull if we can distinguish between
			 * top left margin and bottom right margin, which currently is
			 * not the case
			 */
			if (!MONO)
				tl_hook(0, &cl, screen.dial_colours.lit_col);
#endif
		}
	}

	/* If the window has an auto-redraw function, call it
	 * do this before the widgets. (some programs supply x=0, y=0)
	 */
	if (wind->redraw) /* && !wind->owner->killed */
		wind->redraw(lock, wind);

	/* Go through and display the window widgets using their display behaviour */
	if (wind->window_status == XAWS_ICONIFIED)
	{
		XA_WIDGET *widg;

			
		widg = get_widget(wind, XAW_TITLE);
		DIAG((D_wind, wind->owner, "draw_window %d: display widget %d (func: %lx)",
			wind->handle, XAW_TITLE, widg->display));
		widg->display(lock, wind, widg);

		widg = get_widget(wind, XAW_ICONIFY);
		DIAG((D_wind, wind->owner, "draw_window %d: display widget %d (func: %lx)",
			wind->handle, XAW_ICONIFY, widg->display));
		widg->display(lock, wind, widg);
	}
	else
	{
		int f;
		XA_WIDGET *widg;

		for (f = 0; f < XA_MAX_CF_WIDGETS; f++)
		{
			widg = get_widget(wind, f);
			if (widg->display)
			{
				DIAG((D_wind, wind->owner, "draw_window %d: display widget %d (func=%lx)",
					wind->handle, f, widg->display));
				widg->display(lock, wind, widg);
			}
		}
	}

	showm();

	DIAG((D_wind, wind->owner, "draw_window %d for %s exit ok",
		wind->handle, w_owner(wind)));
}

struct xa_window *
find_window(enum locks lock, int x, int y)
{
	struct xa_window *w;

	w = window_list;
	while(w)
	{
		if (m_inside(x, y, &w->r))
			break;

		w = w->next;
	}

	return w;
}

struct xa_window *
get_wind_by_handle(enum locks lock, int h)
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
 * Handle windows after topping
 */
void
after_top(enum locks lock, bool untop)
{
	struct xa_window *below;

	below = window_list->next;

	/* Refresh the previous top window as being 'non-topped' */
	if (below && below != root_window)
	{
		display_window(lock, 11, below, NULL);

		if (untop)
			send_untop(lock, below);
	}
}

/*
 * Pull this window to the head of the window list
 */
struct xa_window *
pull_wind_to_top(enum locks lock, struct xa_window *w)
{
	enum locks wlock = lock|winlist;
	struct xa_window *wl;
	RECT clip, r;

	DIAG((D_wind, w->owner, "pull_wind_to_top %d for %s", w->handle, w_owner(w)));

	check_menu_desktop(wlock, window_list, w);

	if (w == root_window) /* just a safeguard */
	{
		make_rect_list(w, 1);
	}
	else
	{
		wl = w->prev;
		r = w->r;

		/* Very small change in logic for was_visible() optimization.
	         * It eliminates a spurious generate_rect_list() call, which
	         * spoiled the setting of rect_prev. */

		if (w != window_list)	
		{
			wi_remove   (&S.open_windows, w);
			wi_put_first(&S.open_windows, w);

			while(wl)
			{
				clip = wl->r;
				DIAG((D_r, wl->owner, "wllist %d", wl->handle));
				if (xa_rc_intersect(r, &clip))
				{
					if (wl != w)
						make_rect_list(wl, 1);
				}
				wl = wl->prev;
			}
		}
		set_and_update_window(w, true, NULL);
	}
	return w;
}

void
send_wind_to_bottom(enum locks lock, struct xa_window *w)
{
	struct xa_window *wl;
	struct xa_window *old_top = window_list;

	RECT r, clip;

	if (   w->next == root_window	/* Can't send to the bottom a window that's already there */
	    || w       == root_window	/* just a safeguard */
	    || w->is_open == false)
		return;

	wl = w->next;
	r = w->r;

	wi_remove   (&S.open_windows, w);
	wi_put_blast(&S.open_windows, w);	/* put before last; last = root_window. */

	while (wl)
	{
		clip = wl->r;
		if (xa_rc_intersect(r, &clip))
			make_rect_list(wl, 1);
		wl = wl->next;
	}

	make_rect_list(w, 1);

	check_menu_desktop(lock|winlist, old_top, window_list);
}

/*
 * Change an open window's coordinates, updating rectangle lists as appropriate
 */
void
move_window(enum locks lock, struct xa_window *wind, int newstate, int X, int Y, int W, int H)
{
	IFDIAG(struct xa_client *client = wind->owner;)
	RECT old, new;
	bool blit = true;

	DIAG((D_wind,client,"move_window(%s) %d for %s from %d/%d,%d/%d to %d/%d,%d,%d",
	      wind->is_open ? "open" : "closed",
	      wind->handle, c_owner(client), wind->r.x,wind->r.y,wind->r.w,wind->r.h, X,Y,W,H));

	if (C.redraws)
		yield();

	new.x = X;
	new.y = Y;
	new.w = W;
	new.h = H;

	old = wind->r;

	if (newstate >= 0)
	{
		wind->window_status = newstate;
		if (newstate == XAWS_ICONIFIED)
			wind->ro = wind->r;
		blit = false;
	}
	else
		wind->pr = wind->r;

	inside_root(&new, &wind->owner->options);
	set_and_update_window(wind, blit, &new);

	if (wind->is_open)
	{
		update_windows_below(lock, &old, &new, wind->next); //wl = wind->next;
	}
	
	if (wind->remember)
		*wind->remember = wind->r;
}

/*
 * Close an open window and re-display any windows underneath it.
 * Also places window in the closed_windows list but does NOT delete it
 * - the window will still exist after this call.
 */
bool
close_window(enum locks lock, struct xa_window *wind)
{
	struct xa_window *wl, *w = NULL;
	struct xa_client *client = wind->owner;
	RECT r;
	bool is_top;

	if (wind == NULL)
	{
		DIAGS(("close_window: null window pointer"));
		/* Invalid window handle, return error */
		return false;
	}

	DIAG((D_wind, wind->owner, "close_window: %d (%s)",
		wind->handle, wind->is_open ? "open" : "closed"));

	if (wind->is_open == false)
		return false;

	if (wind->nolist)
	{
		DIAGS(("close_window: nolist window %d, bkg=%lx",
			wind->handle, wind->background));

		if (wind->active_widgets & STORE_BACK)
			form_restore(0, wind->r, &(wind->background));
		if (C.focus == wind)
			C.focus = window_list;
		
		free_rect_list(wind->rect_start);
		wind->rect_user = wind->rect_list = wind->rect_start = NULL;
		wind->is_open = false;
		wind->window_status = XAWS_CLOSED;

		return true;
	}	

	is_top = is_topped(wind);

	wl = wind->next;

	wi_remove(&S.open_windows, wind);
	wi_put_first(&S.closed_windows, wind);

	if (C.focus == wind)
	{
		C.focus = window_list;
		if (!C.focus)
			DIAGS(("no focus owner, no windows open???"));
	}

	r = wind->r;

	if (wind->rect_start)
		free_rect_list(wind->rect_start);

	wind->rect_user = wind->rect_list = wind->rect_start = NULL;

	/* Tag window as closed */
	wind->is_open = false;
	wind->window_status = XAWS_CLOSED;

	if (!wl)
		wl = window_list;

	if (is_top)
	{
		if (wind->active_widgets & STORE_BACK)
		{
			form_restore(0, wind->r, &(wind->background));
			return true;
		}

 		w = window_list;

		while (w && w != root_window)
		{
			if (w->owner == client)
			{
				top_window(lock|winlist, w, client);
				send_ontop(lock);
				wl = w->next;
				break;
			}
			w = w->next;
		}

		if (w && w == root_window)
			w = NULL;
	}

	/* Redisplay any windows below the one we just have closed
	 * or just have topped
	 */
	update_windows_below(lock, &r, NULL, wl);

	if (window_list && is_top && !w)
	{
		switch (cfg.last_wind)
		{
			case 0: /* Put owner of window ontop infront */
			{
				if (window_list != root_window)
				{
					set_active_client(lock, window_list->owner);
					swap_menu(lock, window_list->owner, true, 0);
					top_window(lock, window_list, NULL);
					send_ontop(lock);
				}
				break;
			}
			case 1: /* Keep this app infront */
			{
				break;
			}
		}
	}
	return true;
}

static void
free_standard_widgets(struct xa_window *wind)
{
	int i;
	//struct xa_widget *widg;
	
	DIAGS(("free_standard_widgets for window %d, owner %s", wind->handle, wind->owner->name));
	for (i = 0; i < XA_MAX_WIDGETS; i++)
	{
		//DIAGS(("call remove_widget for widget %d", i));
		remove_widget(0, wind, i);
	}
}

static void
delete_window1(enum locks lock, struct xa_window *wind)
{
	if (!wind->nolist)
	{
		DIAG((D_wind, wind->owner, "delete_window1 %d for %s: open? %d",
			wind->handle, w_owner(wind), wind->is_open));

		assert(!wind->is_open);

		free_standard_widgets(wind);

		/* Call the window destructor if any */
		if (wind->destructor)
			wind->destructor(lock|winlist, wind);

		free_wind_handle(wind->handle);

		if (wind->background)
			kfree(wind->background);

		free_rect_list(wind->rect_start);
		wind->rect_user = wind->rect_list = wind->rect_start = NULL;
	}
	else
	{
		DIAGS(("delete_window1: nolist window %d, bgk=%lx", wind->handle, wind->background));
		free_standard_widgets(wind);
		free_rect_list(wind->rect_start);
		wind->rect_user = wind->rect_list = wind->rect_start = NULL;
		if (wind->destructor)
			wind->destructor(lock, wind);
	}

	kfree(wind);
}

void
delete_window(enum locks lock, struct xa_window *wind)
{
	if (!wind->nolist)
	{
		/* We must be sure it is in the correct list. */
		if (wind->is_open)
		{
			DIAG((D_wind, wind->owner, "delete_window %d: not closed", wind->handle));
			/* open window, return error */
			return;
		}

		wi_remove(&S.closed_windows, wind);
	}

	delete_window1(lock, wind);
}

void
delayed_delete_window(enum locks lock, struct xa_window *wind)
{
	if (!wind->nolist)
	{
		DIAG((D_wind, wind->owner, "delayed_delete_window %d for %s: open? %d",
			wind->handle, w_owner(wind), wind->is_open));

		/* We must be sure it is in the correct list. */
		if (wind->is_open)
		{
			DIAG((D_wind, wind->owner, "delayed_delete_window %d: not closed", wind->handle));
			/* open window, return error */
			return;
		}

		wi_remove(&S.closed_windows, wind);
	}

	wi_put_first(&S.deleted_windows, wind);
}

void
do_delayed_delete_window(enum locks lock)
{
	struct xa_window *wind = S.deleted_windows.first;

	DIAGS(("do_delayed_delete_window"));

	while (wind)
	{
		/* remove from list */
		wi_remove(&S.deleted_windows, wind);

		/* final delete */
		delete_window1(lock, wind);

		/* anything left? */
		wind = S.deleted_windows.first;
	}
}

//static void
//diswin(enum locks lock, struct xa_window *wind, RECT *clip)
void display_window(enum locks lock, int which, struct xa_window *wind, RECT *clip)
{
	if (wind->is_open)
	{
		//if (wind->nolist)
		//	draw_window(lock, wind);
		//else
		{
			struct xa_rect_list *rl;
			RECT d;

			rl = wind->rect_start;
			while (rl)
			{
				if (clip)
				{
					if (xa_rect_clip(clip, &rl->r, &d))
					{
						set_clip(&d);
						draw_window(lock, wind);
					}
				}
				else
				{
					set_clip(&rl->r);
					draw_window(lock, wind);
				}
				rl = rl->next;
			}
		}
		clear_clip();
	}
}

/*
 * Display a window that isn't on top, respecting clipping
 * - Pass clip == NULL to redraw whole window, otherwise clip is a pointer to a GRECT that
 * defines the clip rectangle.
 */
void
update_windows_below(enum locks lock, RECT *old, RECT *new, struct xa_window *wl)
{
	RECT clip;
	enum locks wlock = lock | winlist;

	while (wl)
	{
		if (wl->is_open)
		{
			/* Check for newly exposed windows */
			if (xa_rect_clip(old, &wl->r, &clip))
			{			
				struct xa_rect_list *rl;
				RECT d;

				make_rect_list(wl, true);

				rl = wl->rect_start;
				while (rl)
				{
					if (xa_rect_clip(&rl->r, &clip, &d))
					{
						set_clip(&d);
						draw_window(wlock, wl);
						if (wl->send_message && xa_rect_clip(&wl->wa, &d, &d))
						{
							wl->send_message(wlock, wl, NULL,
								WM_REDRAW, 0, 0, wl->handle,
								d.x, d.y, d.w, d.h);
						}
					}
					rl = rl->next;
				}
				clear_clip();
			}
			else if (new)
			{
				/* Check for newly covered windows */
				if (xa_rect_clip(new, &wl->r, &clip))
				{
					/* We don't need to send a redraw to
					 * these windows, we just have to update
					 * their rect lists */
					make_rect_list(wl, 1);
				}
			}
		}
		wl = wl->next;
	}
}	
/*
 * Display windows below a given rectangle, starting with window w.
 * HR: at the time only called for form_dial(FMD_FINISH)
 */
void
display_windows_below(enum locks lock, const RECT *r, struct xa_window *wl)
{
	struct xa_rect_list *rl;
	RECT clip;

	DIAG((D_wind, wl->owner, "display_windows_below:"));

	while (wl)
	{
		rl = wl->rect_start;
		while (rl)
		{
			if (xa_rect_clip(r, &rl->r, &clip))
			{
				set_clip(&clip);

				draw_window(lock, wl);
				
				if (wl->send_message && xa_rect_clip(&wl->wa, &clip, &clip))
				{
					wl->send_message(lock, wl, NULL,
						WM_REDRAW, 0, 0, wl->handle,
						clip.x,
						clip.y,
						clip.w,
						clip.h);
				}
			}
			rl = rl->next;
		}
		wl = wl->next;
	}
	clear_clip();
	DIAG((D_wind, NULL, "display_windows_below: exit"));
}

/*
 * Typically called after client have been 'lagging',
 * Redraw and send a FULL SET of WM_REDRAWS for all windows owned by
 * the client going non-lagged.
 * Context dependant!
 */
void
redraw_client_windows(enum locks lock, struct xa_client *client)
{
	struct xa_window *wl;
	struct xa_rect_list *rl;

	if (get_desktop()->owner == client)
	{
		wl = root_window;
		make_rect_list(wl, true);
		rl = wl->rect_start;
		while (rl)
		{
			set_clip(&rl->r);
			draw_window(lock, wl);
			if (wl->send_message)
			{
				wl->send_message(lock, wl, NULL,
					WM_REDRAW, 0, 0, wl->handle,
					rl->r.x, rl->r.y, rl->r.w, rl->r.h);
			}
			rl = rl->next;
		}
	}

	wl = root_window->prev;

	while (wl)
	{
		if (wl->owner == client && wl->is_open)
		{
			make_rect_list(wl, true);

			//if (wl->nolist)
			//	draw_window(lock, wl);
			//else
			{
				rl = wl->rect_start;
				while (rl)
				{
					set_clip(&rl->r);
					draw_window(lock, wl);
					if (wl->send_message)
					{
						wl->send_message(lock, wl, NULL,
							WM_REDRAW, 0, 0, wl->handle,
							rl->r.x, rl->r.y, rl->r.w, rl->r.h);
					}
					rl = rl->next;
				}
			}
		}
		wl = wl->prev;
	}
	clear_clip();
}

/*
 * Calculate window sizes
 *
 *   HR: first intruduction of the use of a fake window (pid=-1) for calculations.
 *         (heavily used by multistrip, as an example.)
 */

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

RECT
calc_window(enum locks lock, struct xa_client *client, int request, ulong tp, int mg, int thinframe, bool thinwork, RECT r)
{
	struct xa_window *w_temp;
	RECT o;

	DIAG((D_wind,client,"calc %s from %d/%d,%d/%d", request ? "work" : "border", r));

	/* Create a temporary window with the required widgets */
	w_temp = create_window(lock, NULL, NULL, client, true, tp, 0, mg, thinframe, thinwork, r, 0, 0);

	switch(request)
	{
		case WC_BORDER:	
			/* We have to work out the border size ourselves here */
			Xpolate(&o, &w_temp->r, &w_temp->wa);
			break;
		case WC_WORK:
			/* Work area was calculated when the window was created */
			o = w_temp->wa;
			break;
		default:
			DIAG((D_wind, client, "wind_calc request %d", request));
			o = w_temp->wa;	/* HR: return something usefull*/
	}
	DIAG((D_wind,client,"calc returned: %d/%d,%d/%d", o));

	/* Dispose of the temporary window we created */
	delete_window(lock, w_temp);

	return o;
}

void
set_and_update_window(struct xa_window *wind, bool blit, RECT *new)
{
	short dir, resize, xmove, ymove, wlock = 0;
	struct xa_rect_list *oldrl, *orl, *newrl, *brl, *prev, *next, *rrl, *nrl;
	RECT bs, bd, old, wa, oldw;

	DIAGS(("set_and_update_window:"));

	old = wind->r;
	oldw = wind->wa;

	if (new)
	{
		wind->r = *new;
		calc_work_area(wind);
	}
	else
		new = &wind->r;

	wa = wind->wa;

	if (!wind->is_open)
		return;

	oldrl = wind->rect_start;
	wind->rect_start = NULL;
	newrl = make_rect_list(wind, 0);
	wind->rect_start = newrl;

	xmove = new->x - old.x;
	ymove = new->y - old.y;
	
	resize	= new->w != old.w || new->h != old.h ? 1 : 0;

	if (xmove || ymove)
	{
		/* XXX - This makes set_and_update_window() context dependant
		 *	 when moving windows!!
		 */

		XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
		XA_TREE *wt = widg->stuff;

		/* Temporary hack 070702 */
		if (wt && wt->tree)
		{
			
			wt->tree->ob_x = wind->wa.x;
			wt->tree->ob_y = wind->wa.y;
			if (!wt->zen)
			{
				wt->tree->ob_x += wt->ox;
				wt->tree->ob_y += wt->oy;
			}
		}
	}
		
	if (blit && oldrl && newrl)
	{
		DIAGS(("old win=(%d/%d/%d/%d), new win=(%d/%d/%d/%d)",
			old, *new));

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
				DIAGS(("Check for common areas (newrl=%d/%d/%d/%d, oldrl=%d/%d/%d/%d)",
					bs, orl->r));
				if (xa_rect_clip(&bs, &orl->r, &bd))
				{
					nrl = kmalloc(sizeof(*nrl));
					DIAGS(("COMMON brl=%lx, nrl=%lx, brl->nxt=%lx", brl, nrl, brl ? (long)brl->next : 0xFACEDACE));
					if (brl)
					{
						struct xa_rect_list *n, *p;
						short ox2 = bd.x + bd.w;
						short oy2 = bd.y + bd.h;
						
						switch (dir)
						{
							case 0:	// up/left
							{
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
								n = brl;
								p = NULL;
								while (n)
								{
									//if (bd.x > n->r.x && bd.y < (n->r.y + n->r.h))
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
								n = brl;
								p = NULL;
								while (n)
								{
									//if (bd.x > n->r.x || bd.y >= (n->r.y + n->r.h))
									if ( ( ox2 > (n->r.x + n->r.w) && bd.x >= (n->r.x + n->r.w)) ||
									     ( bd.y >= (n->r.y + n->r.h) && (n->r.x + n->r.w) > bd.x) )
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
					DIAGS(("save blitarea %d/%d/%d/%d", nrl->r));
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
			DIAGS(("freeing oldrl %lx", nrl));
			kfree(nrl);
		}
		if (brl)
		{
			/*
			 * Do the blitting first...
			 */
			if (xmove || ymove)
			{
				nrl = brl;
				while (nrl)
				{
					bd = nrl->r;
					bs.x = bd.x + new->x;
					bs.y = bd.y + new->y;
					bs.w = bd.w;
					bs.h = bd.h;
					bd.x += old.x;
					bd.y += old.y;
					DIAGS(("Blitting from %d/%d/%d/%d to %d/%d/%d/%d (%lx, %lx)",
						bd, bs, brl, (long)brl->next));
					form_copy(&bd, &bs);
					nrl = nrl->next;
				}
			}
			/*
			 * Calculate rectangles that needs to be redrawn...
			 */
			newrl = wind->rect_start;
			while (newrl)
			{
				nrl = kmalloc(sizeof(*nrl));
				nrl->next = NULL;
				nrl->r.x = newrl->r.x - new->x;
				nrl->r.y = newrl->r.y - new->y;
				nrl->r.w = newrl->r.w;
				nrl->r.h = newrl->r.h;
				
				DIAGS(("NEWRECT (%lx) %d/%d/%d/%d (%d/%d/%d/%d)", nrl, nrl->r, newrl->r));

				if (resize)
				{
					if (!xa_rect_clip(&wa, &nrl->r, &nrl->r))
					{
						kfree(nrl);
						nrl = NULL;
					}
				}

				orl = brl;
				while (orl && nrl)
				{
					short w, h, flag;
					short bx2 = orl->r.x + orl->r.w;
					short by2 = orl->r.y + orl->r.h;

					DIAGS(("BLITRECT (orl=%lx, nxt=%lx)(nrl=%lx, nxt=%lx) %d/%d/%d/%d, x2/y2=%d/%d",
						orl, (long)orl->next, nrl, (long)nrl->next, orl->r, bx2, by2));

					for (rrl = nrl, prev = NULL; rrl; rrl = next)
					{
						RECT our = rrl->r;
						w = orl->r.x - rrl->r.x;
						h = orl->r.y - rrl->r.y;

						DIAGS(("CLIPPING %d/%d/%d/%d, w/h=%d/%d", our, w, h));
						DIAGS(("rrl = %lx", rrl));
						
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
								DIAGS((" -- 1. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
									rrl->r, our));
							}
							if (orl->r.x > our.x)
							{
								if (flag)
								{
									prev = rrl;
									rrl = kmalloc(sizeof(*rrl));
									rrl->next = prev->next;
									prev->next = rrl;
									DIAGS((" -- 2. new (%lx)", rrl));
								}
								rrl->r.x = our.x;
								rrl->r.y = our.y;
								rrl->r.h = our.h;
								rrl->r.w = w;
								our.x += w;
								our.w -= w;
								flag = 1;
								DIAGS((" -- 2. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
									rrl->r, our));
							}
							if (ny2 > by2)
							{
								if (flag)
								{
									prev = rrl;
									rrl = kmalloc(sizeof(*rrl));
									rrl->next = prev->next;
									prev->next = rrl;
									DIAGS((" -- 3. new (%lx)", rrl));
								}
								rrl->r.x = our.x;
								rrl->r.y = by2;
								rrl->r.w = our.w;
								rrl->r.h = ny2 - by2;
								our.h -= rrl->r.h;
								flag = 1;
								DIAGS((" -- 3. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
									rrl->r, our));
							}
							if (nx2 > bx2)
							{
								if (flag)
								{
									prev = rrl;
									rrl = kmalloc(sizeof(*rrl));
									rrl->next = prev->next;
									prev->next = rrl;
									DIAGS((" -- 4. new (%lx)", rrl));
								}	
								rrl->r.x = bx2;
								rrl->r.y = our.y;
								rrl->r.w = nx2 - bx2;
								rrl->r.h = our.h;
								our.w -= rrl->r.w;
								flag = 1;
								DIAGS((" -- 4. redraw part %d/%d/%d/%d, remain(blit) %d/%d/%d/%d",
									rrl->r, our));
							}
							if (!flag)
							{
								DIAGS((" ALL BLIT (removed) nrl=%lx, rrl=%lx, prev=%lx, nxt=%lx, %d/%d/%d/%d",
									nrl, rrl, prev, (long)rrl->next, rrl->r));

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
							}
							else
								prev = rrl;
						}
						else
						{
							DIAGS((" -- ALL REDRAW %d/%d/%d/%d", rrl->r));
							prev = rrl;
						}
						DIAGS((" .. .. .."));
					} /* for (rrl = nrl; rrl; rrl = next) */
					DIAGS(("FROM %lx to %lx", orl, (long)orl->next));
					orl = orl->next;
				} /* while (orl && nrl) */
				DIAGS(("DONE CLIPPING"));
				/*
				 * nrl is the first in a list of rectangles that needs
				 * to be redrawn
				 */
				while (nrl)
				{
					nrl->r.x += new->x;
					nrl->r.y += new->y;
					DIAGS(("redrawing area (%lx) %d/%d/%d/%d",
						nrl, nrl->r));
					/*
					 * we only redraw window borders here if wind moves
					 */
					if (!resize)
						display_window(wlock, 2, wind, &nrl->r);
					if (xa_rect_clip(&wind->wa, &nrl->r, &bs))
					{
						wind->send_message(wlock, wind, NULL,
							WM_REDRAW, 0, 0, wind->handle,
							bs.x,
							bs.y,
							bs.w,
							bs.h);
					}
					orl = nrl;
					nrl = nrl->next;
					DIAGS(("Freeing redrawed rect %lx", orl));
					kfree(orl);
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
				DIAGS(("Freeing blitrect %lx", nrl));
				kfree(nrl);
			}
			/*
			 * If window was resized, redraw all window borders
			 */
			if (resize)
				display_window(wlock, 2, wind, NULL);
						
		} /* if (brl) */
		else
		{
			while (newrl)
			{
				display_window(wlock, 12, wind, &newrl->r);
				wind->send_message(wlock, wind, NULL,
					WM_REDRAW, 0, 0, wind->handle,
					newrl->r.x,
					newrl->r.y,
					newrl->r.w,
					newrl->r.h);
				newrl = newrl->next;
			}
		}
	}
	else if (newrl)
	{
		while (newrl)
		{
			display_window(wlock, 12, wind, &newrl->r);
			wind->send_message(wlock, wind, NULL,
				WM_REDRAW, 0, 0, wind->handle,
				newrl->r.x,
				newrl->r.y,
				newrl->r.w,
				newrl->r.h);
			newrl = newrl->next;
		}

		while (oldrl)
		{
			orl = oldrl;
			oldrl = oldrl->next;
			kfree(orl);
		}
	}
}
