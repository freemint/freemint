/*
 * window.c
 *
 * Basisfunktionen fuer Fensterverwaltung.
 *
 */

#include <string.h>
#include "global.h"
#include "window.h"
#include "textwin.h"
#include "av.h"
#include "console.h"

#ifndef WM_SHADED
#define WM_SHADED				0x5758
#define WM_UNSHADED			0x5759
#define WF_SHADE				0x575D
#endif

#include "window.h"

WINDOW	*gl_topwin;		/* oberstes Fenster */
WINDOW	*gl_winlist;		/* LIFO Liste der offenen Fenster */
int	 gl_winanz = 0;		/* Anzahl der offenen Fenster */

static void
timer_expired (WINDOW* w, int topped)
{
	(void) w;
	(void) topped;
}

static void noslid(WINDOW *w, short m)
{
	(void) w;
	(void) m;
}

static bool nokey(WINDOW *w, short code, short shft)
{
	(void) w;
	(void) code;
	(void) shft;
	return FALSE;
}

static bool nomouse(WINDOW *w, short clicks, short x, short y, short shift, short mbuttons)
{
	(void) w;
	(void) clicks;
	(void) x;
	(void) y;
	(void) shift;
	(void) mbuttons;
	return FALSE;
}

static void clear_win(WINDOW *v, short x, short y, short w, short h)
{
	short temp[4];

	(void) v;
	set_fillcolor (0);
	temp[0] = x;
	temp[1] = y;
	temp[2] = x + w - 1;
	temp[3] = y + h - 1;
	v_bar(vdi_handle, temp);
}

static WINDOW *get_top(void)
{
	short h, d;
	WINDOW	*v;
	
	wind_get(0, WF_TOP, &h, &d, &d, &d);
	v = get_window(h);
	return v;
}

static void top_win(WINDOW *w)
{
	if (gl_topwin != w)
	{
		gl_topwin = w;
		wind_set(w->handle, WF_TOP, 0, 0, 0, 0);
	}
}

static void ontop_win(WINDOW *w)
{
	gl_topwin = w;
}

static void untop_win(WINDOW *w)
{
	(void) w;
	gl_topwin = get_top();
}
					
static void bottom_win(WINDOW *w)
{
	wind_set(w->handle, WF_BOTTOM, 0, 0, 0, 0);
	gl_topwin = get_top();
}

static void close_win(WINDOW *w)
{
	destroy_window(w);
}

static void full_win(WINDOW *v)
{
	GRECT new;
#ifndef ONLY_XAAES
	if (v->wco)
	{
#endif
		short g = v->flags & WFULLED ? WF_PREVXYWH : WF_FULLXYWH;
		wind_get_grect(v->handle, g, &new);
		wind_xset_grect(v->handle, WF_CURRXYWH, &new, &v->work);
#ifndef ONLY_XAAES
	}
	else
	{
		if (v->flags & WFULLED) 
			wind_get_grect(v->handle, WF_PREVXYWH, &new);
		else 
			wind_get_grect(v->handle, WF_FULLXYWH, &new);

		wind_set_grect(v->handle, WF_CURRXYWH, &new);
		wind_get_grect(v->handle, WF_WORKXYWH, &v->work);
	}
#endif
	v->flags ^= WFULLED;
}

static void move_win(WINDOW *v, short x, short y, short w, short h)
{
	GRECT full;
	GRECT new = (GRECT){x, y, w, h};

	wind_get_grect(v->handle, WF_FULLXYWH, &full);

	if (new.g_w > full.g_w) 
		new.g_w = full.g_w;
	if (new.g_h > full.g_h) 
		new.g_h = full.g_h;

	if (new.g_w != full.g_w || new.g_h != full.g_h)
		v->flags &= ~WFULLED;

#ifndef ONLY_XAAES
	if (v->wco)
#endif
		wind_xset_grect(v->handle, WF_CURRXYWH, &new, &v->work);
#ifndef ONLY_XAAES
	else
	{
		wind_set_grect(v->handle, WF_CURRXYWH, &new);
		wind_get_grect(v->handle, WF_WORKXYWH, &v->work);
	}
#endif
}

static void
repos_win(WINDOW *v, short x, short y, short w, short h)
{
	GRECT new = (GRECT){x, y, w, h};

#ifndef ONLY_XAAES
	if (v->wco)
#endif
		wind_xset_grect(v->handle, WF_CURRXYWH, &new, &v->work);
#ifndef ONLY_XAAES
	else
	{
		wind_set_grect(v->handle, WF_CURRXYWH, &new);
		wind_get_grect(v->handle, WF_WORKXYWH, &v->work);
	}
#endif
}

static void size_win(WINDOW *v, short x, short y, short w, short h)
{
	move_win(v, x, y, w, h);
}

static void iconify_win(WINDOW *v, short x, short y, short w, short h)
{
#ifndef ONLY_XAAES
	if (v->wco)
	{
#endif
		GRECT i = (GRECT){x, y, w, h};
		v->prev = v->work;
		/* XaAES has a bug and it doesn't draw the iconified
		 * window after the wind_set(WF_ICONIFY) call, as a
		 * workaround we open the window first and we do it
		 * outside of the screen to avoid visual artifacts.
		 */
		if (w == -1 && h == -1)
		{
			short x1, y1, w1, h1;
			/* Get the size of the screen */
			wind_get(0, WF_WORKXYWH, &x1, &y1, &w1, &h1);
			/* Draw the window out of the screen boundaries */
			wind_open(v->handle, x1 + w1 + 1, y1 + h1 + 1, 0, 0);
		}
		wind_xset_grect(v->handle, WF_ICONIFY, &i, &v->work);
#ifndef ONLY_XAAES
	}
	else
	{
		if (w == -1 && h == -1)
		{
			wind_close(v->handle);
			wind_set(v->handle, WF_ICONIFY, -1, -1, -1, -1);
			wind_open(v->handle, -1, -1, -1, -1);
		}
		else
			wind_set(v->handle, WF_ICONIFY, x, y, w, h);

	  	v->old_wkind = v->kind;
		v->prev = v->work;
		wind_get_grect(v->handle, WF_WORKXYWH, &v->work);
	}
#endif
	v->oldmouse = v->mouseinp;
	v->mouseinp = nomouse;
	v->flags |= WICONIFIED;
	v->flags &= ~WFULLED;
/*
	(*v->bottomed)(v); 
*/
}

static void uniconify_win(WINDOW *v, short x, short y, short w, short h)
{
#ifndef ONLY_XAAES
	if (v->wco)
	{
#endif
		wind_xset_grect(v->handle, WF_UNICONIFY, &v->prev, &v->work);
#ifndef ONLY_XAAES
	}
	else
	{
		wind_calc(WC_BORDER, v->kind, v->prev.g_x, v->prev.g_y, v->prev.g_w, v->prev.g_h, &x, &y, &w, &h);
  		wind_set(v->handle, WF_UNICONIFY, x, y, w, h);
		wind_get_grect(v->handle, WF_WORKXYWH, &v->work);
	}
#endif
	v->mouseinp = v->oldmouse;
	v->flags &= ~WICONIFIED;
	(*v->topped)(v);
}

void uniconify_all(void)						/* alle Fenster auf */
{
	WINDOW *v;
	
	v = gl_winlist;
	while (v)
	{
		if (v->handle >= 0)
		{
			if (v->flags & WICONIFIED)
				(*v->uniconify)(v, -1, -1, -1, -1);
		}
		v = v->next;
	}
}

static void shade_win(WINDOW *v, short flag)
{
	switch (flag)
	{
		case 1 :
			v->flags |= WSHADED;
			break;

		case 0 :
			v->flags &= ~WSHADED;
			break;

		case -1 :
			wind_set(v->handle, WF_SHADE, 0, 0, 0, 0);
			v->flags &= ~WSHADED;
			(*v->topped)(v);
			break;

		default:
			debug("window.c, shade_win():\n Unbekanntes flag %d\n", flag);
	}		
}

WINDOW *create_window(char *title, short kind, 
		      short wx, short wy, short ww, short wh, 	/* Gre zum ffnen */
		      short max_w, short max_h, long w_limit)	/* max. Gre */
{
	WINDOW *v;
	GRECT full;
#ifndef ONLY_XAAES
	GRECT full_work;
	int centerwin = 0;
#endif

	title = strdup(title);
	v = malloc(sizeof(WINDOW));
	if (!v) 
		return v;
#ifndef ONLY_XAAES
	if (wco)
	{
#endif
		v->kind = kind;
		v->handle = wind_xcreate_grect(v->kind, &gl_desk, &full);
		v->wco = 1;
		
		if (ww == -1 || wh == -1)
		{
			v->work.g_w = full.g_w;
			v->work.g_h = full.g_h;
		}
		else
		{
			v->work.g_w = ww < full.g_w ? ww : full.g_w;
			v->work.g_h = wh < full.g_h ? wh : full.g_h;
		}

		if (w_limit == -1L)
			w_limit = max_w;
		else if (max_w > w_limit)
			max_w = w_limit;
		
		if (max_w > full.g_w)
			max_w = full.g_w;
		if (max_h > full.g_h)
			max_h = full.g_h;
		
		v->max_w = max_w;
		v->max_h = max_h;
		v->w_limit = w_limit;

		if (wx == -1 || wy == -1)
		{
			wx = (gl_desk.g_w - v->work.g_w) >> 1;
			wy = (gl_desk.g_h - v->work.g_h) >> 1;
		}
		if (wx < full.g_x)
			wx = full.g_x;
		if (wy < full.g_y)
			wy = full.g_y;

		v->work.g_x = wx;
		v->work.g_y = wy;
		v->full_work = v->full = full;
// 		wind_set_grect(v->handle, WF_FULLXYWH, &full);
#ifndef ONLY_XAAES
	}
	else
	{
		v->handle = -1;
		v->kind = kind;
		v->wco = 0;

		if (wx == -1 || wy == -1) 
			centerwin = 1;
		if (wx < gl_desk.g_x) 
			wx = gl_desk.g_x;
		if (wy < gl_desk.g_y) 
			wy = gl_desk.g_y;

		v->max_w = max_w;
		v->max_h = max_h;

		if (ww == -1 || wh == -1)
			wind_calc(WC_WORK, v->kind, wx, wy, gl_desk.g_w, gl_desk.g_h, &v->work.g_x, &v->work.g_y, &v->work.g_w, &v->work.g_h);
		else
			wind_calc(WC_WORK, v->kind, wx, wy, ww, wh, &v->work.g_x, &v->work.g_y, &v->work.g_w, &v->work.g_h);

		wind_calc(WC_BORDER, v->kind, v->work.g_x, v->work.g_y, max_w, max_h, &full.g_x, &full.g_y, &full.g_w, &full.g_h);
	
		if (full.g_w > gl_desk.g_w || full.g_w < 0)
			full.g_w = gl_desk.g_w;
		if (full.g_h > gl_desk.g_h || full.g_h < 0)
			full.g_h = gl_desk.g_h;
		wind_calc (WC_WORK, v->kind, 0, 0, full.g_w, full.g_h, 
			   &full_work.g_x, &full_work.g_y, 
			   &full_work.g_w, &full_work.g_h);
		v->full_work = full_work;

		if (0 || centerwin) 
		{
			full.g_x = gl_desk.g_x + (gl_desk.g_w - full.g_w) / 2;
			full.g_y = gl_desk.g_y + (gl_desk.g_h - full.g_h) / 2;
		}
		else
		{
			if ((full.g_x + full.g_w) > (gl_desk.g_x + gl_desk.g_w))
				full.g_x = gl_desk.g_x + gl_desk.g_w - full.g_w;
			if ((full.g_y + full.g_h) > (gl_desk.g_y + gl_desk.g_h))
				full.g_y = gl_desk.g_y + gl_desk.g_h - full.g_h;
		}
		v->full = full;

		if (ww == -1 || wh == -1)
			wind_calc_grect(WC_WORK, v->kind, &full, &v->work);
	}
#endif
	v->title = (char*) title;
	v->extra = NULL;
	v->flags = 0;
	v->redraw = 0;

	v->draw = clear_win;
	v->topped = top_win;
	v->ontopped = ontop_win;
	v->untopped = untop_win;
	v->bottomed = bottom_win;
	v->closed = close_win;
	v->fulled = full_win;
	v->sized = size_win;
	v->moved = move_win;
	v->reposed = repos_win;
	v->iconify = iconify_win;
	v->uniconify = uniconify_win;
	v->shaded = shade_win;
	v->arrowed = noslid;
	v->vslid = noslid;
	v->keyinp = nokey;
	v->mouseinp = nomouse;
	v->oldfulled = v->fulled;
	v->oldmouse = nomouse;
	v->timer = timer_expired;

	v->next = gl_winlist;
	gl_winlist = v;

	return v;
}


void open_window(WINDOW *v, bool as_icon)
{
	static char notitle[] = "Untitled";
#ifndef ONLY_XAAES
	GRECT r;
#endif

#ifndef ONLY_XAAES
	if (v->wco)
	{
#endif
		if (v->flags & WOPEN)
			return;
		if (v->kind & NAME)
		{
			char *t = v->title ? v->title : notitle;
			wind_set_str(v->handle, WF_NAME, t);
		}
		(*v->sized)(v, v->work.g_x, v->work.g_y, v->work.g_w, v->work.g_h);
		if (as_icon)
		{
			iconify_win(v, -1, -1, -1, -1);
		}
		else
		{
			wind_open_grect(v->handle, &v->work);
			gl_topwin = v;
		}
#ifndef ONLY_XAAES
	}
	else
	{
		if (v->handle >= 0)
			return;

		v->handle = wind_create_grect(v->kind, &v->full);
		
		if (v->handle < 0)
			return;

		if (v->kind & NAME) 
		{
			if (v->title)
				wind_set_str(v->handle, WF_NAME, v->title);
			else
				wind_set_str(v->handle, WF_NAME, notitle);
		}

		wind_calc_grect(WC_BORDER, v->kind, &v->work, &r);
		if (as_icon)
		{
			(*v->sized)(v, r.g_x, r.g_y, r.g_w, r.g_h);
			iconify_win(v, -1, -1, -1, -1);
		}
		else
		{
			(*v->sized)(v, r.g_x, r.g_y, r.g_w, r.g_h);	/* macht snapping! */
			wind_calc_grect(WC_BORDER, v->kind, &v->work, &r);
			wind_open_grect(v->handle, &r);
			gl_topwin = v;
		}
	}
#endif
	gl_winanz++;
	send_avwinopen(v->handle);
}

static void unlink_window(WINDOW *v)
{
	WINDOW **ptr, *w;
	short i, dummy;

	/* find v in the window list, and unlink it */
	ptr = &gl_winlist;
	w = *ptr;
	while (w)
	{
		if (w == v)
		{
			*ptr = v->next;
			break;
		}
		ptr = &w->next;
		w = *ptr;
	}
	free(v);

	/* reset gl_topwin */
	gl_topwin = 0;
	wind_get(0, WF_TOP, &i, &dummy, &dummy, &dummy);
	for (v = gl_winlist; v; v = v->next) 
	{
		if (v->handle == i)
			break;
	}
	if (v && !(v->flags & WICONIFIED) && !(v->flags & WISDIAL))
		(*v->topped)(v);
}

void destroy_window(WINDOW *v)
{
	if (v->handle < 0)	/* already closed? */
		return;

	wind_close(v->handle);
	wind_delete(v->handle);
	gl_winanz--;
	send_avwinclose(v->handle);
	v->handle = -1;
	
	if (v->title)
		free(v->title);

	unlink_window(v);
}


void redraw_window(WINDOW *v, short xc, short yc, short wc, short hc)
{
	GRECT t1, t2;
	bool off = FALSE;
	
	v->redraw = 1;
	// XXX wind_update (TRUE);
	
	t2.g_x = xc;
	t2.g_y = yc;
	t2.g_w = wc;
	t2.g_h = hc;
	rc_intersect(&gl_desk, &t2);
	wind_get_grect(v->handle, WF_FIRSTXYWH, &t1);
	while (t1.g_w > 0 && t1.g_h > 0) 
	{
		if (rc_intersect(&t2, &t1)) 
		{
			if (!off)
				off = mouse_hide_if_needed(&t1);
			set_clipping(vdi_handle, t1.g_x, t1.g_y, 
				     t1.g_w, t1.g_h, TRUE);
			(*v->draw)(v, t1.g_x, t1.g_y, t1.g_w, t1.g_h);
		}
		wind_get_grect(v->handle, WF_NEXTXYWH, &t1);
	}
	
	if (off)
		mouse_show();
	
	// XXX wind_update (FALSE);
	v->redraw = 0;
}


WINDOW *get_window(short handle)
{
	WINDOW *w;

	if (handle < 0) 
		return NULL;

	for (w = gl_winlist; w; w = w->next)
		if (w->handle == handle)
			return w;
	return NULL;
}


WINDOW *find_window(short x, short y)
{
	short wx, wy, ww, wh;
	WINDOW *w;

	wind_update(BEG_UPDATE);
	for (w = gl_winlist; w; w = w->next) 
	{
		if (w->handle < 0) 
			continue;
		wind_get(w->handle, WF_FIRSTXYWH, &wx, &wy, &ww, &wh);
		while (ww && wh) 
		{
			if (x >= wx && x <= wx + ww && y >= wy && y <= wy + wh)
				goto found_window;
			wind_get(w->handle, WF_NEXTXYWH, &wx, &wy, &ww, &wh);
		}
	}
found_window:
	wind_update(END_UPDATE);
	return w;	/* w will be null if no window found */
}


bool window_msg(short *msgbuff)
{
	WINDOW	*v;
	bool ret = TRUE;
	
	v = get_window(msgbuff[3]);
	if (!v || (v->flags & WISDIAL))
		return FALSE;

	switch(msgbuff[0]) 
	{
		case WM_REDRAW:
			redraw_window(v, msgbuff[4], msgbuff[5], msgbuff[6], msgbuff[7]);
			break;
		case WM_TOPPED:
			(*v->topped)(v);
			break;
	 	case WM_ONTOP:
		        (*v->ontopped)(v);
		        break;
		case WM_UNTOPPED:
        		(*v->untopped)(v);
		        break;
		case WM_BOTTOMED:
			(*v->bottomed)(v);
         		break;
		case WM_SIZED:
			(*v->sized)(v, msgbuff[4], msgbuff[5], msgbuff[6], msgbuff[7]);
			break;
		case WM_MOVED:
			(*v->moved)(v, msgbuff[4], msgbuff[5], msgbuff[6], msgbuff[7]);
			break;
		case WM_REPOSED:
			(*v->reposed)(v, msgbuff[4], msgbuff[5], msgbuff[6], msgbuff[7]);
			break;
		case WM_FULLED:
			(*v->fulled)(v);
			break;
		case WM_ARROWED:
			(*v->arrowed)(v, msgbuff[4]);
			break;
		case WM_VSLID:
			(*v->vslid)(v, msgbuff[4]);
			break;
		case WM_CLOSED:
			(*v->closed)(v);
			break;
		case WM_ICONIFY:
			(*v->iconify)(v, msgbuff[4], msgbuff[5], msgbuff[6], msgbuff[7]);
			break;
		case WM_UNICONIFY:
			(*v->uniconify)(v, msgbuff[4], msgbuff[5], msgbuff[6], msgbuff[7]);
			break;
		case WM_SHADED :
			(*v->shaded)(v, 1);
			break;
		case WM_UNSHADED :
			(*v->shaded)(v, 0);
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}


bool window_key(short keycode, short shift)
{
	WINDOW	*w;

	w = gl_topwin;
	if (w && !(w->flags & WISDIAL) && !(w->flags & WICONIFIED) && !(w->flags & WSHADED))
		return (*w->keyinp)(w, keycode, shift);
	else
		return FALSE;
}


bool window_click(short clicks, short x, short y, short kshift, short mbutton)
{
	WINDOW *w;

	w = find_window(x, y);
	if (w && !(w->flags & WISDIAL)) 
		return (*w->mouseinp)(w, clicks, x, y, kshift, mbutton);
	return FALSE;
}


void force_redraw(WINDOW *v)
{
	redraw_window(v, v->work.g_x, v->work.g_y, v->work.g_w, v->work.g_h);
}

void change_window_gadgets(WINDOW *w, short newkind)
{
#ifndef ONLY_XAAES
	bool reopen = FALSE;
#endif
	GRECT n;
	
	if (newkind == w->kind) 
		return;

#ifndef ONLY_XAAES
	if (w->wco)
	{
#endif
		if (w->handle >= 0)
		{
			wind_xget_grect(w->handle, WF_CALCW2F, &w->work, &n);
			wind_close(w->handle);
			wind_delete(w->handle);
			send_avwinclose(w->handle);

			w->kind = newkind;
			w->handle = wind_create_grect(w->kind, &gl_desk);
		
			wind_xget_grect(w->handle, WF_CALCF2W, &n, &w->work);
			
			if (w->work.g_w > w->max_w)
				w->work.g_w = w->max_w;
			if (w->work.g_h > w->max_h)
				w->work.g_h = w->max_h;
			
			open_window(w, FALSE);
		}
#ifndef ONLY_XAAES
	}
	else
	{
		if (w->handle >= 0) 
		{
			wind_close(w->handle);
			wind_delete(w->handle);
			send_avwinclose(w->handle);
			w->handle = -1;
			gl_winanz--;
			reopen = TRUE;
		}

	
		wind_calc_grect(WC_WORK, w->kind, &w->full, &n);
		if (n.g_w > w->max_w)
			n.g_w = w->max_w;
		if (n.g_h > w->max_h)
			n.g_h = w->max_h;
	
		wind_calc(WC_BORDER, newkind, n.g_x, n.g_y, n.g_w, n.g_h, &n.g_x, &n.g_y, &w->full.g_w, &w->full.g_h);

		if (w->full.g_w > gl_desk.g_w)
			w->full.g_w = gl_desk.g_w;
		if (w->full.g_h > gl_desk.g_h)
			w->full.g_h = gl_desk.g_h;

		if (w->full.g_x < gl_desk.g_x)
			w->full.g_x = gl_desk.g_x;
		if (w->full.g_y < gl_desk.g_y)
			w->full.g_y = gl_desk.g_y;

		wind_calc_grect(WC_BORDER, w->kind, &w->work, &n);
		if (n.g_w > w->full.g_w)
			n.g_w = w->full.g_w;
		if (n.g_h > w->full.g_h)
			n.g_h = w->full.g_h;

		wind_calc_grect(WC_WORK, newkind, &n, &w->work);
	
		w->kind = newkind;
		if (reopen)
			open_window(w, FALSE);
	}
#endif
}


void title_window(WINDOW *w, char *title)
{
	if (w->title)
		free(w->title);
	w->title = strdup(title);
	if (w->handle >= 0 && (w->kind & NAME)) 
		wind_set_str(w->handle, WF_NAME, w->title);
}


/*
 * new_topwin: reset the top window so that it doesn't match the current
 * top window (if that's possible). If force == 0, then we allow the current
 * window to remain on top, otherwise we pretend that no window is on
 * top
 */
void cycle_window(void)
{
	WINDOW *w;
	short mbuf[8];
	short top, d;
	
	w = gl_topwin;

	if (!w)
	{
		if (gl_winlist)
			w = gl_winlist;
		else
			return;
	}

	if (w->next == NULL)					/* letzes in der Liste */
	{
		if (w == gl_winlist)				/*  -> ist einziges */
		{
			wind_get(0, WF_TOP, &top, &d, &d, &d);
			if (top == w->handle)	/*			ist das auch aktiv? */
				return;
		}
		else
			w = gl_winlist;				/*  -> wrap zum ersten */
	}
	else
		w = w->next;
	if (w)
	{
		mbuf[0] = WM_TOPPED;
		mbuf[1] = gl_apid;
		mbuf[2] = 0;
		mbuf[3] = w->handle;
		mbuf[4] = mbuf[5] = mbuf[6] = mbuf[7] = 0;
		appl_write(gl_apid, (int)sizeof(mbuf), mbuf);
	} 
}

/*
 * Da die Fenster-Dialoge von der CF-Lib verwaltet werden, mssen wir
 * dafr sorgen, da sie auch in unserer Fenster-Liste auftauchen (fr ^W)
*/

static void wdial_c(WINDOW *v)
{
	WDIALOG	*dial = (WDIALOG*)v->extra;

	wdial_close(dial);
	close_wdial(dial);
}

void wdial_open(WDIALOG *dial)
{
	WINDOW	*v;

	v = malloc(sizeof(WINDOW));
	if (v)
	{
		memset(v, 0, sizeof(WINDOW));
		v->handle = dial->win_handle;
		v->flags = WISDIAL;
		v->extra = dial;
		v->next = gl_winlist;

		/* Callbacks */
		v->closed = wdial_c;

		/* in Liste einhngen */
		gl_winlist = v;
		gl_winanz++;
		send_avwinopen(v->handle);
	}
}

void wdial_close(WDIALOG *dial)
{
	WINDOW	*v;
	
	v = get_window(dial->win_handle);
	if (v)
	{
		unlink_window(v);
		send_avwinclose(v->handle);
		gl_winanz--;
	}
}

bool wdial_msg(short *msg)
{
	bool	ret;
	
	ret = message_wdial(msg);
	if (ret && msg[0] == WM_TOPPED)	/* es wurde ein Dialog getopped */
		gl_topwin = get_top();
	return ret;
}


void update_avwin(bool new)
{
	WINDOW *v;

	v = gl_winlist;
	if (new)		/* AV-Cycle war aus, jetzt an */
	{
		while (v)
		{
			send_avwinopen(v->handle);
			v = v->next;
		}
	}
	else			/* AV-Cycle war an, jetzt aus */
	{
		while (v)
		{
			send_avwinclose(v->handle);
			v = v->next;
		}
	}
}

void window_term(void)
{
	WINDOW *v;

	v = gl_winlist;
	while (v) 
	{
		gl_winlist = v->next;
		if ((v->handle >= 0) && !(v->flags & WISDIAL)) 
		{
			wind_close(v->handle);
			wind_delete(v->handle);
			send_avwinclose(v->handle);
		}
		free(v);
		v = gl_winlist;
	}
}

void window_timer (void)
{
	WINDOW* win;
	WINDOW* topwin = get_top ();
	
	for (win = gl_winlist; win != NULL; win = win->next) {
		if (!is_console (win) && win->timer)
			win->timer (win, win == topwin ? 1 : 0);
	}
}

void draw_winicon(WINDOW *win)
{
	OBJECT	*icon;
	GRECT	t1, t2;
	bool	off = FALSE;		

	if (is_console(win))
		icon = get_con_icon();
	else
		icon = winicon;
		
	wind_get_grect(win->handle, WF_WORKXYWH, &t2);
			
	icon[0].ob_x = t2.g_x;
	icon[0].ob_y = t2.g_y;
	icon[0].ob_width = t2.g_w;
	icon[0].ob_height = t2.g_h;
	icon[1].ob_x = (t2.g_w - icon[1].ob_width) / 2;
	icon[1].ob_y = (t2.g_h - icon[1].ob_height) / 2;
	
	rc_intersect(&gl_desk, &t2);
	wind_get_grect(win->handle, WF_FIRSTXYWH, &t1);
	while (t1.g_w && t1.g_h) 
	{
		if (rc_intersect(&t2, &t1)) 
		{
			if (!off)
				off = mouse_hide_if_needed(&t1);
			objc_draw(icon, ROOT, MAX_DEPTH, t1.g_x, t1.g_y, t1.g_w, t1.g_h);
		}
		wind_get_grect(win->handle, WF_NEXTXYWH, &t1);
	}
	if (off)
		mouse_show();
}

void update_window_lock(void) {
	wind_update( TRUE );
}

void update_window_unlock(void) {
	wind_update( FALSE );
}

void get_window_rect(WINDOW *win, short kind, GRECT *rect) {
	wind_get_grect( win->handle, kind, rect );
}

void mouse_show(void) {
	show_mouse();
}

short mouse_hide_if_needed(GRECT *rect) {
	return hide_mouse_if_needed(rect);
}
