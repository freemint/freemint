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
#include "k_mouse.h"
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
 * Uses the RECT at wind->t.
 */
RECT
free_icon_pos(enum locks lock, struct xa_window *ignore)
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
			if (w != ignore && (w->window_status & XAWS_ICONIFIED))
			{
				if (w->t.x == ic.x && w->t.y == ic.y)
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

static void
move_ctxdep_widgets(struct xa_window *wind)
{		
	if (wind->active_widgets & TOOLBAR)
		rp_2_ap_cs(wind, get_widget(wind, XAW_TOOLBAR), NULL);
	if (wind->active_widgets & XaMENU)
		rp_2_ap_cs(wind, get_widget(wind, XAW_MENU), NULL);
}


/*
 * Check if a new menu and/or desktop is needed
 */
static void
check_menu_desktop(enum locks lock, struct xa_window *old_top, struct xa_window *new_top)
{
	/* If we're getting a new top window, we may need to swap menu bars... */
	if (old_top && (old_top == root_window || old_top->owner != new_top->owner || !is_infront(new_top->owner)))
	{
		Sema_Up(desk);

		DIAG((D_appl, NULL, "check_menu_desktop from %s to %s",
			old_top->owner->name, new_top->owner->name));

		set_active_client(lock, new_top->owner);
		swap_menu(lock|desk, new_top->owner, true, true, 2);

		Sema_Dn(desk);
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
inside_root(RECT *r, struct options *o)
{
	if (r->y < root_window->wa.y)
		r->y = root_window->wa.y;

	if (o->noleft)
		if (r->x < root_window->wa.x)
			r->x = root_window->wa.x;
}

static void
inside_minmax(RECT *r, struct xa_window *wind)
{
	if (wind->max.w && r->w > wind->max.w)
		r->w = wind->max.w;
	if (wind->min.w && r->w < wind->min.w)
		r->w = wind->min.w;
	
	if (wind->max.h && r->h > wind->max.h)
		r->h = wind->max.h;
	if (wind->min.h && r->h < wind->min.h)
		r->h = wind->min.h;
}

void
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
}

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
}

void
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
}

bool
is_topped(struct xa_window *wind)
{

	DIAGS(("is_topped: app infront %s, window_list owner %s",
		APP_LIST_START->name, window_list->owner->name));

	if (wind->owner != APP_LIST_START)
		return false;

	if (wind != window_list)
		return false;

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
	return ( wind->rc.x == (root_window->rc.x + root_window->rc.w + 16) && wind->rc.y == (root_window->rc.y + root_window->rc.h + 16)) ? true : false;
}

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
		send_moved(lock, wind, AMQ_NORM, &r);
	}
}

void
unhide_window(enum locks lock, struct xa_window *wind)
{
	if (is_hidden(wind))
	{
		RECT r = wind->rc;

		r.x = wind->hx;
		r.y = wind->hy;
		send_moved(lock, wind, AMQ_NORM, &r);
		wind->t = r;
	}
}

/* SendMessage */
void
send_untop(enum locks lock, struct xa_window *wind)
{
	struct xa_client *client = wind->owner;

	if (wind->send_message && !client->fmd.wind)
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_UNTOPPED, 0, 0, wind->handle,
				   0, 0, 0, 0);
}

void
send_ontop(enum locks lock)
{
	struct xa_window *top = window_list;
	struct xa_client *client = top->owner;

	if (is_topped(top) && top->send_message && !client->fmd.wind)
		top->send_message(lock, top, NULL, AMQ_NORM, QMF_CHKDUP,
				  WM_ONTOP, 0, 0, top->handle,
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
void
send_sized(enum locks lock, struct xa_window *wind, short amq, RECT *r)
{
	if (!(wind->window_status & XAWS_SHADED))
	{
		if (wind->send_message)
		{
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

void
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
send_iredraw(enum locks lock, struct xa_window *wind, short xaw, RECT *r)
{
	if (wind == root_window)
	{
		if (get_desktop()->owner == C.Aes)
			display_window(lock, 0, wind, r);
		else if (r)
			send_app_message(lock, wind, get_desktop()->owner, AMQ_IREDRAW, QMF_NORM,
					 WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
					 r->x, r->y, r->w, r->h);
		else
			send_app_message(lock, wind, get_desktop()->owner, AMQ_IREDRAW, QMF_NORM,
					 WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
					 0,0,0,0);
	}
	else
	{
		if (r)
			send_app_message(lock, wind, NULL, AMQ_IREDRAW, QMF_NORM,
				WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
				r->x, r->y, r->w, r->h);
		else
			send_app_message(lock, wind, NULL, AMQ_IREDRAW, QMF_NORM,
				WM_REDRAW, xaw, ((long)wind) >> 16, ((long)wind) & 0xffff,
				0,0,0,0);
	}
}

void
generate_redraws(enum locks lock, struct xa_window *wind, RECT *r, short flags)
{
	RECT b;
	
	if ((wind->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN)
	{
		if (wind != root_window && r && (flags & RDRW_WA) && xa_rect_clip(&wind->wa, r, &b))
			send_redraw(lock, wind, &b);
			
		if ((flags & RDRW_EXT))
			send_iredraw(lock, wind, 0, r);
	}
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
	if ((r->w == -1 && r->h == -1) || (!r->w && !r->h))
		*r = free_icon_pos(lock, NULL);
	move_window(lock, wind, true, XAWS_ICONIFIED, r->x, r->y, r->w, r->h);
}

void
uniconify_window(enum locks lock, struct xa_window *wind, RECT *r)
{
	move_window(lock, wind, true, ~XAWS_ICONIFIED, r->x, r->y, r->w, r->h);
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
	bool thinwork,
	const RECT R,
	const RECT *max,
	RECT *remember)
{
	struct xa_window *w;
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

	if (tp & (UPARROW|DNARROW|LFARROW|RTARROW))
	{
		w->min.x = w->min.y = -1;
		w->min.w = 6 * cfg.widg_w;
		w->min.h = 6 * cfg.widg_h;
	}
	else
		w->min.w = w->min.h = 0;

	w->rc = w->r = w->pr = r;

	{
		long opts = client->options.wind_opts;
		
		if (client->options.naes_ff)
			opts |= XAWO_FULLREDRAW;
		w->opts = opts;
	}

	w->vdi_handle = C.vh;	
	w->wheel_mode = client->options.wheel_mode;
	w->frame = frame;
	w->thinwork = thinwork;
	w->owner = client;
	w->rect_user = w->rect_list = w->rect_start = NULL;
	w->handle = -1;
	w->remember = remember;
	w->nolist = nolist;
	w->dial = dial;
	w->send_message = message_handler;
	w->do_message	= message_doer;
	get_widget(w, XAW_TITLE)->stuff = client->name;

	if (nolist)
	{
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

		/* If STORE_BACK extended attribute is used, window preserves its own background */
		if (tp & STORE_BACK)
		{
			DIAG((D_wind,client," allocating background storage buffer"));

			/* XXX the store back code is not failsafe if kmalloc fail */

			w->background = kmalloc(calc_back(&r, screen.planes));
			assert(w->background);
		}
		else
			w->background = NULL;

	}

	calc_work_area(w);

	if (tp & (CLOSER|NAME|MOVER|ICONIFIER|FULLER))
	{
		RECT t;
		rp_2_ap_cs(w, w->widgets + XAW_TITLE, &t);
		w->sh = t.y + t.h - w->r.y + (w->frame > 0 ? w->frame : 0);
	}

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
		RECT r, RECT *remember)
{
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

	//w->rc = w->r = r;
	standard_widgets(w, tp, false);

	if (r_is_wa)
	{
		w->r = calc_window(lock, client, WC_BORDER,
				 tp,
				 client->options.thinframe,
				 client->options.thinwork,
				 *(RECT *)&r);
	}
	else
		w->r = r;
	
	w->rc = w->r;

	if ((w->window_status & XAWS_SHADED))
		w->r.h = w->sh;

	calc_work_area(w);

	free_rect_list(w->rect_start);
	free_rect_list(w->rect_opt_start);
	w->rect_user = w->rect_list = w->rect_start = w->rect_opt = w->rect_opt_start = NULL;

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

	if (tp & (CLOSER|NAME|MOVER|ICONIFIER|FULLER))
	{
		RECT t;
		rp_2_ap_cs(w, w->widgets + XAW_TITLE, &t);
		w->sh = t.y + t.h - w->r.y + (w->frame > 0 ? w->frame : 0);
	}

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
	
	if ( w->active_widgets & TOOLBAR )
	{
		OBJECT *obtree = ((XA_TREE *)get_widget(w, XAW_TOOLBAR)->stuff)->tree;

		obtree->ob_x = w->wa.x;
		obtree->ob_y = w->wa.y;
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

	if ((wind->window_status & XAWS_OPEN))
	{
		/* The window is already open, no need to do anything */

		DIAGS(("WARNING: Attempt to open window when it was already open"));
		return 0;
	}

	if (wind->nolist || (wind->dial & created_for_SLIST))
	{
		DIAGS(("open_window: nolist window - SLIST wind? %s",
			(wind->dial & created_for_SLIST) ? "yes":"no"));

		if (wind != root_window && !(wind->dial & (created_for_POPUP | created_for_SLIST)))
		{
			inside_root(&r, &wind->owner->options);
			inside_minmax(&r, wind);
		}

		wind->rc = wind->r = r;

		if ((wind->window_status & XAWS_SHADED))
			wind->r.h = wind->sh;

		wind->window_status |= XAWS_OPEN;
		calc_work_area(wind);
		make_rect_list(wind, true, RECT_SYS);

		if (wind->active_widgets & STORE_BACK)
		{
			form_save(0, wind->r, &(wind->background));
		}
		wi_remove(&S.closed_nlwindows, wind);
		wi_put_first(&S.open_nlwindows, wind);
		move_ctxdep_widgets(wind);
		//draw_window(lock|winlist, wind);

		if (!(wind->dial & created_for_SLIST))
			generate_redraws(lock, wind, &wind->r, RDRW_ALL);
		//send_redraw(lock|winlist, wind, &wind->wa);
		/* dont open unlisted windows */
		return 1;
	}

	wi_remove(&S.closed_windows, wind);
	wi_put_first(&S.open_windows, wind);

	/* New top window - change the cursor to this client's choice */
	graf_mouse(wind->owner->mouse, wind->owner->mouse_form, wind->owner, false);

	if ((wind->window_status & XAWS_ICONIFIED))
	{
		if (wind != root_window && !(wind->dial & created_for_POPUP))
			inside_root(&r, &wind->owner->options);

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
			inside_root(&r, &wind->owner->options);
			//inside_minmax(&r, wind);
		}
		wind->rc = wind->r = r;
		if ((wind->window_status & XAWS_SHADED))
		{
			wind->r.h = wind->sh;

			if (wind->send_message)
				wind->send_message(lock, wind, NULL, AMQ_CRITICAL, QMF_CHKDUP,
					WM_SHADED, 0,0, wind->handle, 0,0,0,0);
		}
	}
	wind->window_status |= XAWS_OPEN;

	calc_work_area(wind);

	check_menu_desktop(lock|winlist, wl, wind);

	make_rect_list(wind, true, RECT_SYS);
	
	/*
	 * Context dependant widgets must always follow the window
	 * movements
	 */
	move_ctxdep_widgets(wind);

	/* Is this a 'preserve own background' window? */
	if (wind->active_widgets & STORE_BACK)
	{
		form_save(0, wind->r, &(wind->background));
		/* This is enough, it is only for TOOLBAR windows. */
		redraw_menu(lock);
		generate_redraws(lock, wind, &wind->r, RDRW_ALL);
	}
	else
	{
		our_win = wind->r;

		wl = wind->next;
		while (wl)
		{
			clip = wl->r;

			if (xa_rc_intersect(our_win, &clip))
				make_rect_list(wl, true, RECT_SYS);

			wl = wl->next;
		}

		/* streamlining the topping */
		after_top(lock|winlist, true);

		/* Display the window using clipping rectangles from the rectangle list */
		redraw_menu(lock);
		generate_redraws(lock, wind, &wind->r, RDRW_ALL);
	}

	DIAG((D_wind, wind->owner, "open_window %d for %s exit with 1",
		wind->handle, c_owner(wind->owner)));

	set_winmouse();

	return 1;
}

static void
if_bar(short pnt[4])
{
	if ((pnt[2] - pnt[0]) >= 0 && (pnt[3] - pnt[1]) >= 0)
		v_bar(C.vh, pnt);
}

/* 
 *About to remove this..
 */
void
do_rootwind_msg(
	struct xa_window *wind,
	struct xa_client *to,			/* if different from wind->owner */
	short amq, short qmflags,
	short *msg)
{

	DIAG((D_form, wind->owner, "do_rootwind_msg: wown %s, to %s, msg %d, %d, %d, %d, %d, %d, %d, %d",
		wind->owner->name, to->name, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]));

	return;

	switch (msg[0])
	{
		case WM_REDRAW:
		{
			XA_WIDGET *widg = get_widget(root_window, XAW_TOOLBAR);
			
			if (get_desktop()->owner == to && widg->display)
			{
				hidem();
				set_clip((RECT *)&msg[4]);
				widg->display(0, root_window, widg);
				clear_clip();
				showm();
			}
			break;
		}
	}
}

void
draw_window(enum locks lock, struct xa_window *wind)
{
	
	DIAG((D_wind, wind->owner, "draw_window %d for %s to %d/%d,%d/%d",
		wind->handle, w_owner(wind),
		wind->r.x, wind->r.y, wind->r.w, wind->r.h));

	if (!(wind->window_status & XAWS_OPEN))
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
	/* XXX - Ozk: Extremely not good if draw_window() is to be context indipendant!!!
	 */
	if (wind->redraw) /* && !wind->owner->killed */
		wind->redraw(lock, wind);

#if 0
	/* Go through and display the window widgets using their display behaviour */
	if ((wind->window_status & XAWS_ICONIFIED))
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
#endif
	{
		int f;
		short status = wind->window_status;
		XA_WIDGET *widg;

		for (f = 0; f < XA_MAX_WIDGETS; f++)
		{
			widg = get_widget(wind, f);

			if ((widg->properties & WIDG_NOTEXT) || (f == XAW_MENU && wind == root_window))
				continue;

			if (!(status & widg->loc.statusmask) && widg->display)
			{
				DIAG((D_wind, wind->owner, "draw_window %d: display widget %d (func=%lx)",
					wind->handle, f, widg->display));
				widg->display(lock, wind, widg);
			}
		}
#if 0
		for (f = XA_MAX_CF_WIDGETS; f < XA_MAX_WIDGETS; f++)
		{
			widg = get_widget(wind, f);

			if ( !(status & widg->loc.statusmask) && widg->display)
			{
				if (!widg->owner || (widg->owner && widg->owner == rc))
					widg->display(lock, wind, widg);
			}
		}
#endif
	}

	showm();

	DIAG((D_wind, wind->owner, "draw_window %d for %s exit ok",
		wind->handle, w_owner(wind)));
}

struct xa_window *
find_window(enum locks lock, short x, short y)
{
	struct xa_window *w;

	w = window_list;
	while(w)
	{
		if (!(w->owner->status & CS_EXITING) && m_inside(x, y, &w->r))
			break;

		w = w->next;
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
		send_iredraw(lock, below, 0, NULL);

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
	struct xa_window *wl;
	RECT clip, r;

	DIAG((D_wind, w->owner, "pull_wind_to_top %d for %s", w->handle, w_owner(w)));

	if (w == root_window) /* just a safeguard */
	{
		make_rect_list(w, true, RECT_SYS);
	}
	else if (!(w->owner->status & CS_EXITING))
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
						make_rect_list(wl, true, RECT_SYS);
				}
				wl = wl->prev;
			}
		}
		set_and_update_window(w, true, false, NULL);
	}
	return w;
}

void
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

	wi_remove   (&S.open_windows, w);
	wi_put_blast(&S.open_windows, w);	/* put before last; last = root_window. */

	while (wl)
	{
		clip = wl->r;
		if (xa_rc_intersect(r, &clip))
			make_rect_list(wl, true, RECT_SYS);
		wl = wl->next;
	}

	make_rect_list(w, true, RECT_SYS);
}

/*
 * Change an open window's coordinates, updating rectangle lists as appropriate
 */
void
move_window(enum locks lock, struct xa_window *wind, bool blit, short newstate, short X, short Y, short W, short H)
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

	if (!wind->nolist && C.redraws)
		yield();

	if (newstate != -1)
	{
		blit = false;
		if (newstate < -1)
		{
			if (!(newstate & XAWS_ICONIFIED))
			{
				DIAGS(("move_window: Clear iconified"));
				if ((wind->window_status & XAWS_ICONIFIED))
				{
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
				if (!(wind->window_status & XAWS_ICONIFIED))
				{
					wind->save_widgets = wind->active_widgets;
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

	inside_root(&new, &wind->owner->options);

	set_and_update_window(wind, blit, false, &new);

	if ((wind->window_status & XAWS_OPEN))
	{
		update_windows_below(lock, &old, &new, wind->next, NULL);
	}
	
	if (wind->remember)
		*wind->remember = wind->r;

	/*
	 * If moving the window did not result in any WM_REDRAWS
	 * being generated (C.move_block is set to 3 when WM_REDRAWS
	 * are added to a clients msg queue), we release move_block here
	 */
	if (!C.redraws && C.move_block != 3)
		C.move_block = 0;
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
		wind->handle, (wind->window_status & XAWS_OPEN) ? "open" : "closed"));

	if (!(wind->window_status & XAWS_OPEN))
		return false;

	if (wind == C.hover_wind)
	{
		C.hover_wind = NULL;
		C.hover_widg = NULL;
	}

	if (wind->nolist)
	{
		DIAGS(("close_window: nolist window %d, bkg=%lx",
			wind->handle, wind->background));

		wi_remove(&S.open_nlwindows, wind);
		wi_put_first(&S.closed_nlwindows, wind);

		if (wind->active_widgets & STORE_BACK)
			form_restore(0, wind->r, &(wind->background));
		
		free_rect_list(wind->rect_start);
		free_rect_list(wind->rect_opt_start);
		wind->rect_user = wind->rect_list = wind->rect_start = wind->rect_opt = wind->rect_opt_start = NULL;
		wind->window_status &= ~XAWS_OPEN;
		remove_from_iredraw_queue(lock, wind);
		return true;
	}	

	is_top = is_topped(wind);

	wl = wind->next;

	wi_remove(&S.open_windows, wind);
	wi_put_first(&S.closed_windows, wind);
	remove_from_iredraw_queue(lock, wind);
	r = wind->r;

	free_rect_list(wind->rect_start);
	free_rect_list(wind->rect_opt_start);

	wind->rect_user = wind->rect_list = wind->rect_start = wind->rect_opt = wind->rect_opt_start = NULL;

	/* Tag window as closed */
	wind->window_status &= ~(XAWS_OPEN);

	if (wind->remember)
		*wind->remember = wind->rc;

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
				if (w != window_list)
					top_window(lock|winlist, true, w, NULL, NULL);
				else
				{
					send_iredraw(lock, w, 0, NULL);
					set_and_update_window(w, true, true, NULL);
					send_ontop(lock);
				}
				wl = w->next;
				break;
			}
			w = w->next;
		}

		if (w && (w == root_window || (w->owner->status & CS_EXITING)))
			w = NULL;
	}

	/* Redisplay any windows below the one we just have closed
	 * or just have topped
	 */
	update_windows_below(lock, &r, NULL, wl, NULL);

	if (window_list && is_top && !w)
	{
		switch (cfg.last_wind)
		{
			case 0: /* Put owner of window ontop infront */
			{
				if (!(window_list->owner->status & CS_EXITING) && window_list != root_window)
				{
					top_window(lock, true, window_list, NULL, NULL);
				}
				break;
			}
			case 1: /* Keep this app infront */
			{
				break;
			}
		}
	}

	set_winmouse();

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
	/* Ozk: I dont think this is really necessary here as it should
	 *	should be done when closing the window..
	 */
	if (wind == C.hover_wind)
	{
		C.hover_wind = NULL;
		C.hover_widg = NULL;
		graf_mouse(ARROW, NULL, NULL, false);
	}

	if (!wind->nolist)
	{
		DIAG((D_wind, wind->owner, "delete_window1 %d for %s: open? %s",
			wind->handle, w_owner(wind),
			(wind->window_status & XAWS_OPEN) ? "yes" : "no"));

		assert(!(wind->window_status & XAWS_OPEN));

		free_standard_widgets(wind);

		/* Call the window destructor if any */
		if (wind->destructor)
			wind->destructor(lock|winlist, wind);

		free_wind_handle(wind->handle);

		if (wind->background)
			kfree(wind->background);

	}
	else
	{
		DIAGS(("delete_window1: nolist window %d, bgk=%lx", wind->handle, wind->background));
		free_standard_widgets(wind);
		if (wind->background)
			kfree(wind->background);
		if (wind->destructor)
			wind->destructor(lock, wind);
	}

	free_rect_list(wind->rect_start);
	free_rect_list(wind->rect_opt_start);
	wind->rect_user = wind->rect_list = wind->rect_start = wind->rect_opt = wind->rect_opt_start = NULL;

	kfree(wind);
}

void
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
		wi_remove(&S.closed_nlwindows, wind);
	else
		wi_remove(&S.closed_windows, wind);
	
	remove_from_iredraw_queue(lock, wind);
	delete_window1(lock, wind);
}

void
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
		wi_remove(&S.closed_nlwindows, wind);
	else
		wi_remove(&S.closed_windows, wind);

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

/*
 * Context dependant !!!
 */
void
display_window(enum locks lock, int which, struct xa_window *wind, RECT *clip)
{
	if (!(wind->owner->status & CS_EXITING) && (wind->window_status & XAWS_OPEN))
	{
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

void
update_all_windows(enum locks lock, struct xa_window *wl)
{
	while (wl)
	{
		if (wl != root_window && !(wl->owner->status & CS_EXITING) && (wl->window_status & XAWS_OPEN))
		{
			set_and_update_window(wl, true, false, NULL);
		}
		wl = wl->next;
	}
	clear_clip();
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

	while (wl)
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

				rl = wl->rect_start;
				while (rl)
				{
					if (xa_rect_clip(&rl->r, &clip, &d))
					{
						generate_redraws(wlock, wl, &d, RDRW_ALL);
					}
					rl = rl->next;
				}
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
		wl = wl->next;
	}
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
		make_rect_list(wl, true, RECT_SYS);
		rl = wl->rect_start;
		while (rl)
		{
			generate_redraws(lock, wl, &rl->r, RDRW_ALL);
			
			rl = rl->next;
		}
	}

	wl = root_window->prev;

	while (wl)
	{
		if (wl->owner == client && (wl->window_status & XAWS_OPEN))
		{
			make_rect_list(wl, true, RECT_SYS);

			{
				rl = wl->rect_start;
				while (rl)
				{
					generate_redraws(lock, wl, &rl->r, RDRW_ALL);
					rl = rl->next;
				}
			}
		}
		wl = wl->prev;
	}
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
calc_window(enum locks lock, struct xa_client *client, int request, ulong tp, int thinframe, bool thinwork, RECT r)
{
	struct xa_window *w_temp;
	RECT o;

	DIAG((D_wind,client,"calc %s from %d/%d,%d/%d", request ? "work" : "border", r));

	/* Create a temporary window with the required widgets */
	w_temp = create_window(lock, NULL, NULL, client, true, tp, 0, thinframe, thinwork, r, 0, 0);

	switch(request)
	{
		case WC_BORDER:
		{
			/* We have to work out the border size ourselves here */
			Xpolate(&o, &w_temp->r, &w_temp->wa);
			break;
		}
		case WC_WORK:
		{
			/* Work area was calculated when the window was created */
			o = w_temp->wa;
			break;
		}
		default:
		{
			DIAG((D_wind, client, "wind_calc request %d", request));
			o = w_temp->wa;	/* HR: return something usefull*/
		}
	}
	DIAG((D_wind,client,"calc returned: %d/%d,%d/%d", o));

	/* Dispose of the temporary window we created */
	delete_window(lock, w_temp);

	return o;
}

void
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
			new->h = wind->sh;
		wind->r = *new;
		calc_work_area(wind);
	}
	else
		new = &wind->r;

	wa = wind->wa;

	xmove = new->x - old.x;
	ymove = new->y - old.y;
	
	resize	= new->w != old.w || new->h != old.h ? 1 : 0;

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
		if (xmove || ymove)
		{
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
	}
		
	if (!(wind->window_status & XAWS_OPEN))
		return;

	oldrl = wind->rect_start;
	wind->rect_start = NULL;
	newrl = make_rect_list(wind, false, RECT_SYS);
	wind->rect_start = newrl;

	if (wind->nolist)
	{
		if (xmove || ymove || resize)
		{
			if (wind->active_widgets & STORE_BACK)
				form_restore(0, old, &(wind->background));
		
			make_rect_list(wind, true, RECT_SYS);

			if (wind->active_widgets & STORE_BACK)
				form_save(0, wind->r, &(wind->background));

			generate_redraws(wlock, wind, (RECT *)&wind->r, !only_wa ? RDRW_ALL : RDRW_WA);
		}
		return;
	}


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
					DIAGS(("kfree %lx (unused oldrl)", nrl));
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
				//DIAGS(("Check for common areas (newrl=%d/%d/%d/%d, oldrl=%d/%d/%d/%d)",
				//	bs, orl->r));
				if (xa_rect_clip(&bs, &orl->r, &bd))
				{
					nrl = kmalloc(sizeof(*nrl));
					assert(nrl);

					DIAGS(("kmalloc %lx (%ld bytes) blitrect", nrl, sizeof(*nrl)));
					
					//DIAGS(("COMMON brl=%lx, nrl=%lx, brl->nxt=%lx", brl, nrl, brl ? (long)brl->next : 0xFACEDACE));
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
			DIAGS(("kfree %lx (rest of oldrl)", nrl));
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
					//DIAGS(("Blitting from %d/%d/%d/%d to %d/%d/%d/%d (%lx, %lx)",
					//	bd, bs, brl, (long)brl->next));
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
				assert(nrl);

				DIAGS(("kmalloc %lx (%ld bytes)", nrl, sizeof(*nrl)));
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
						DIAGS(("kfree %lx", nrl));
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

									DIAGS(("kmalloc %lx (%ld bytes)", rrl, sizeof(*rrl)));
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

									DIAGS(("kmalloc %lx (%ld bytes)", rrl, sizeof(*rrl)));
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

									DIAGS(("kmalloc %lx (%ld bytes)", rrl, sizeof(*rrl)));
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
								DIAGS(("kfree %lx", rrl));
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
				/*
				 * nrl is the first in a list of rectangles that needs
				 * to be redrawn
				 */
				while (nrl)
				{
					nrl->r.x += new->x;
					nrl->r.y += new->y;
					//DIAGS(("redrawing area (%lx) %d/%d/%d/%d",
					//	nrl, nrl->r));
					/*
					 * we only redraw window borders here if wind moves
					 */
					generate_redraws(wlock, wind, &nrl->r, (!resize && !only_wa) ? RDRW_ALL : RDRW_WA);
					orl = nrl;
					nrl = nrl->next;
					//DIAGS(("Freeing redrawed rect %lx", orl));
					kfree(orl);
					DIAGS(("kfree %lx", orl));
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
				DIAGS(("kfree %lx", nrl));
			}
			/*
			 * If window was resized, redraw all window borders
			 */
			if (resize && !only_wa)
				generate_redraws(wlock, wind, NULL, RDRW_EXT);
						
		} /* if (brl) */
		else
		{
			newrl = wind->rect_start;
			while (newrl)
			{
				generate_redraws(wlock, wind, &newrl->r, !only_wa ? RDRW_ALL : RDRW_WA);
				newrl = newrl->next;
			}
		}
	}
	else if (newrl)
	{
		while (newrl)
		{
			generate_redraws(wlock, wind, &newrl->r, !only_wa ? RDRW_ALL : RDRW_WA);
			newrl = newrl->next;
		}

		while (oldrl)
		{
			orl = oldrl;
			oldrl = oldrl->next;
			kfree(orl);
			DIAGS(("kfree %lx", orl));
		}
	}
			
	DIAGS(("set_and_update_window: DONE!!!"));
}
