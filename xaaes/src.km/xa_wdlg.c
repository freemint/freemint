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

#define WDIAL_CALLBACK 1

#include "xa_global.h"
#include "xa_wdlg.h"

#include "k_mouse.h"

#include "app_man.h"
#include "form.h"
#include "obtree.h"
#include "draw_obj.h"
#include "c_window.h"
#include "rectlist.h"
#include "widgets.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_form.h"
#include "scrlobjc.h"

#include "nkcc.h"


/*
 * WDIALOG FUNCTIONS (wdlg)
 *
 * documentation about this can be found in:
 *
 * - the GEMLIB documentation:
 *   http://arnaud.bercegeay.free.fr/gemlib/
 *
 * - the MagiC documentation project:
 *   http://www.bygjohn.fsnet.co.uk/atari/mdp/
 */

#if WDIALOG_WDLG

static inline void
cpy_ev2md(EVNT *e, struct moose_data *m)
{
	m->x = e->mx;
	m->y = e->my;
	m->state = e->mbutton;
	m->cstate = 0;
	m->clicks = e->mclicks;
	m->kstate = e->kstate;
}

/* Called from draw_window via wind->redraw, so clipping
 * is handled elsewhere
 */
static int
wdlg_redraw(enum locks lock, struct xa_window *wind, short start, short depth, RECT *r)
{
	struct wdlg_info *wdlg;

	if ((wdlg = wind->wdlg))
	{
		struct xa_rect_list *rl;
		XA_TREE *wt;
		OBJECT *obtree;
		RECT dr;

		if (wind->window_status == XAWS_ICONIFIED)
		{
			short x = 0, y = 0;

			wt = wdlg->ify_wt;
			obtree = wt->tree;

			if (wdlg->ify_obj >= 0)
			{
				obtree->ob_x = 0;
				obtree->ob_y = 0;
				ob_offset(obtree, wdlg->ify_obj, &x, &y);
			}
			obtree->ob_x = wind->wa.x - x;
			obtree->ob_y = wind->wa.y - y;
			start = wdlg->ify_obj >= 0 ? wdlg->ify_obj : 0;
			depth = 10;
		}
		else
		{
			wt = wdlg->std_wt;
			obtree = wt->tree;
			obtree->ob_x = wind->wa.x;
			obtree->ob_y = wind->wa.y;
		}

		rl = wind->rect_start;
		hidem();
		if (r)
		{
			while (rl)
			{
				if (xa_rect_clip(&rl->r, r, &dr))
				{
					set_clip(&dr);
					draw_object_tree(0, wt, wt->tree, start, depth, 1);
				}
				rl = rl->next;
			}
		}
		else
		{
			while (rl)
			{
				set_clip(&rl->r);
				draw_object_tree(0, wt, wt->tree, start, depth, 1);
				rl = rl->next;
			}
		}
		showm();
		clear_clip();
	}
	return 0;
}

static short
wdlg_mesag(enum locks lock, struct xa_window *wind, XA_TREE *wt, EVNT *ev)
{
	enum locks wlock = lock|winlist;
	struct wdlg_info *wdlg;
	short *msg = ev->msg;
	short wh = wind->handle;
	short mh = msg[3];

	DIAGS(("wdlg_mesag: window %d for %s (%x, %x, %x, %x, %x, %x, %x, %x)",
		wind->handle, wind->owner->name,
		msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]));

	if (!(wdlg = wind->wdlg))
		return -1;

	if (msg[0] >= 20 && msg[0] <= 39 && !wdlg->exit(wdlg->handle, ev, HNDL_MESG, ev->mclicks, wdlg->user_data) )
		return 0;

	switch (msg[0])
	{
		case WM_REDRAW:
		{
			if (wh != mh)
				return -1;
			//display_window(wlock, 1, wind, (RECT *)&msg[4]);
			wdlg_redraw(wlock, wind, 0, 10, (RECT *)&msg[4]);
			break;
		}
		case WM_TOPPED:
		{
			if (wh != mh)
				return -1;
			if (is_hidden(wind))
				unhide_window(wlock, wind);
			top_window(wlock, wind, 0);
			break;
		}
		case WM_CLOSED:
		{
			return wh == mh ? 0 : -1;
		}
		case WM_FULLED:
		{
			if (wh != mh)
				return -1;
			move_window(wlock, wind, -1, wind->max.x, wind->max.y, wind->max.w, wind->max.h);
			break;
		}
		case WM_SIZED:
		{
			RECT r;
			if (wh != mh)
				return -1;

			r.x = wind->r.x;
			r.y = wind->r.y;
			r.w = wind->max.w < msg[6] ? wind->max.w : msg[6];
			r.h = wind->max.h < msg[7] ? wind->max.h : msg[7];

			if (wind->r.w != r.w || wind->r.h != r.h)
			{
				inside_root(&r, &wind->owner->options);
				move_window(wlock, wind, -1, r.x, r.y, r.w, r.h);
			}
			break;
		}
		case WM_MOVED:
		{
			RECT r;

			if (wh != mh)
				return -1;
			if ( !wdlg->exit(wdlg->handle, ev, HNDL_MOVE, ev->mclicks, wdlg->user_data) )
				return 0;

			r.x = msg[4], r.y = msg[5];
			r.w = wind->r.w, r.h = wind->r.h;

			if (wind->r.x != r.x || wind->r.y != r.y)
			{
				inside_root(&r, &wind->owner->options);
				move_window(wlock, wind, -1, r.x, r.y, r.w, r.h);
			}
			break;
		}
		case WM_ONTOP:
		case WM_NEWTOP:
		{
			if (wh != mh)
				return -1;
			return wdlg->exit(wdlg->handle, ev, HNDL_TOPW, ev->mclicks, wdlg->user_data);
		}
		case WM_UNTOPPED:
		{
			if (wh != mh)
				return -1;
			return wdlg->exit(wdlg->handle, ev, HNDL_UNTP, ev->mclicks, wdlg->user_data);
		}
		case WM_BOTTOM:
		{
			if (wh != mh)
				return -1;
			bottom_window(wlock, wind);
			break;
		}
		case AP_TERM:
		{
			return 0;
		}
		default:
		{
			return -1;
		}
	}
	return 1;
}

#if 0
static void
exit_wdial(enum locks lock, struct xa_window *wind, struct xa_widget *widg,
	   struct widget_tree *wt, int f, int os, int dbl, int which,
	   struct rawkey *key)
{
	struct xa_client *client = wt->owner;
	OBJECT *form = wt->tree;
	struct moose_data md;

	md.x = form->ob_x + widg->x;
	md.y = form->ob_y + widg->y;
	md.state = widg->s;
	md.clicks = dbl ? 2 : 1;

	if (os != -1)
	{
		form[f].ob_state = os;
		redraw_object(lock, wt, f);
	}
	wt->tree = form;	/* After redraw of object. :-) */
	wt->current = f|dbl;	/* pass the double click to the internal handlers as well. */
	wt->which = which;	/* pass the event type. */

	if (which == MU_BUTTON)
	{
		DIAG((D_wdlg, client, "button_event: x=%d, y=%d, clicks=%d",
			md.x, md.y, md.clicks));
		button_event(lock, client, &md);
	}
	else
	{
		DIAG((D_wdlg, client, "keybd_event: x=%d, y=%d, clicks=%d",
			md.x, md.y, md.clicks));
		keybd_event(lock, client, key);
	}
}
#endif

unsigned long
XA_wdlg_create(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	XA_TREE *wt;
							
	CONTROL(2,0,4)

	DIAG((D_wdlg, client, "XA_wdlg_create"));
	DIAG((D_wdlg, client, "  --  exit %lx, tree %lx, user_data %lx, data %lx",
		pb->addrin[0], pb->addrin[1], pb->addrin[2], pb->addrin[3]));

	if (pb->addrin[0] && pb->addrin[1] && pb->addrout)
	{
		XA_WIND_ATTR tp = MOVER|NAME;
		RECT r, or;
		OBJECT *obtree = (OBJECT*)pb->addrin[1];

		pb->addrout[0] = 0;

		obtree->ob_state &= ~OS_OUTLINED;

		if (obtree->ob_x <= 0 && obtree->ob_y <= 0)
			form_center(obtree, ICON_H);

		ob_area(obtree, 0, &or);

		r = calc_window(lock, client, WC_BORDER,
				tp,
				MG,
				false, false,
				*(RECT *)&or);		// *(RECT*)&tree->ob_x);

		wind = create_window(lock, send_app_message, NULL, client, false,
				     tp,
				     created_for_WDIAL,
				     MG,
				     false, false,
				     r, 0, 0);
		if (wind)
		{
			short rep;
			struct wdlg_info *wdlg;
			
			wdlg = kmalloc(sizeof(*wdlg));
			if (wdlg)
			{
				bzero(wdlg, sizeof(*wdlg));

				wind->wdlg = wdlg;

				wt = set_toolbar_widget(lock, wind, obtree, 0);
				wt->exit_form = NULL; //exit_wdial;

				wdlg->handle = (void*)(long)wind->handle;
				wdlg->wind = wind;
				wdlg->code = pb->intin[0];			/* Code */
				wdlg->flag = pb->intin[1];			/* Flags */
				wdlg->user_data = (void*)pb->addrin[2];	/* user_data */
				wdlg->data      = (void*)pb->addrin[3];	/* data - is passed to handle_exit() */

				wdlg->std_wt = wt;
				wdlg->ify_wt = wt;

				wdlg->exit      = (void*)pb->addrin[0];
				rep = wdlg->exit(0, 0, HNDL_INIT, wdlg->code, wdlg->data);
				if (rep == 0)
					delete_window(lock, wind);
				else
					(long)pb->addrout[0] = 0xae000000 + wind->handle;
			}
			else
				delete_window(lock, wind);
		}
	}
	return XAC_DONE;
}

unsigned long
XA_wdlg_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;


	CONTROL(4,1,3)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_open, title %lx, data %lx",
		handle, pb->addrin[1],pb->addrin[2]));
	DIAG((D_wdlg, client, "  --  tp=%x, x=%d, y=%d",
		pb->intin[0], pb->intin[1], pb->intin[2]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);

	if (wind && (wdlg = wind->wdlg))
	{
		RECT r = wind->r;
		XA_WIND_ATTR tp = pb->intin[0];
		char *s;

		tp &= CLOSER|MOVER|NAME;

		/* The following if is a result of the clumsiness of the
		 * WDIALOG interface. So dont blame me. */

		/* recreate window with final widget set. */
		if ((short)tp != (short)wind->active_widgets)
		{
			OBJECT *obtree = wdlg->std_wt->tree;
			RECT or;

			ob_area(obtree, 0, &or);

			r = calc_window(lock, client, WC_BORDER,
					tp, MG, false, false,
					*(RECT *)&or); //*(RECT *)&tree->ob_x);

			change_window_attribs(lock, client, wind, tp, r, &r);
		}
	
		r.x = pb->intin[1];
		r.y = pb->intin[2];

		if (r.x == -1 || r.y == -1)
		{
			/* desktop work area */
			r.x = (root_window->wa.w - r.w) / 2;
			r.y = (root_window->wa.h - r.h) / 2;
		}

		if (!(s = (char *)pb->addrin[1]))
			s = client->proc_name;
		{
			char *d = wind->wname;
			char *b = wdlg->std_name;
			int i;

			for (i = 0; i < (sizeof(wind->wname)-1) && (*d++ = *b++ = *s++); i++)
				;
			*d = *b = 0;
			get_widget(wind, XAW_TITLE)->stuff = wind->wname;
		}

		open_window(lock, wind, r);
		wdlg->data = (void*)pb->addrin[2];
		wdlg->exit(wdlg->handle, 0, HNDL_OPEN, pb->intin[3], wdlg->data);

		pb->intout[0] = handle;
	}
	return XAC_DONE;
}

unsigned long
XA_wdlg_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;

	CONTROL(0,3,1)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_close", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind && (wdlg = wind->wdlg))
	{
		pb->intout[1] = wind->r.x;
		pb->intout[2] = wind->r.y;
		close_window(lock, wind);
		pb->intout[0] = 1;
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;

	CONTROL(0,1,1)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_delete", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);

	if (wind && (wdlg = wind->wdlg))
	{
		if (wind->is_open)
			close_window(lock, wind);

		kfree(wdlg);
		wind->wdlg = NULL;
		delete_window(lock, wind);
		pb->intout[0] = 1;
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;
	RECT *r;

	CONTROL(1,0,1)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_get %d", handle, pb->intin[0]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);

	if (wind && (wdlg = wind->wdlg))
	{
		pb->intout[0] = 1;

		switch (pb->intin[0])
		{
			/* wdlg_get_tree */
			case 0:
			{
				r = (RECT*)pb->addrin[2];
				if (r)
					*r = wind->wa;
				if (pb->addrin[1])
					*(OBJECT **)pb->addrin[1] = wdlg->std_wt->tree;
				DIAG((D_wdlg, client, " -- tree %lx (%d/%d,%d/%d)", wdlg->std_wt->tree, *r));		
				break;
			}
			/* wdlg_get_edit */
			case 1:
			{
				if (wdlg->std_wt->e.obj < 0)
				{
					pb->intout[0] = 0;
					pb->intout[1] = 0;
				}
				else
				{
					pb->intout[0] = wdlg->std_wt->e.obj;
					pb->intout[1] = wdlg->std_wt->e.pos;
				}
				DIAG((D_wdlg, client, " ---> e.obj %d, e.pos %d", pb->intout[0], pb->intout[1]));
				break;
			}
			/* wdlg_get_udata */
			case 2:
			{
				if (pb->addrout)
				{
					pb->addrout[0] = (long)wdlg->user_data;
					DIAG((D_wdlg, client, " -- udata %lx", pb->addrout[0]));
				}
				break;
			}
			/* wdlg_get_handle */
			case 3:
			{
				pb->intout[0] = handle;
				DIAG((D_wdlg, client, " -- handle %d", handle));
				break;
			}
			default:
			{
				DIAG((D_wdlg, client, " -- error"));
			}
		}
	}
	else
	{
		DIAG((D_wdlg, client, " -- window gone"));
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	XA_TREE *wt;
	short handle;

	CONTROL(2,1,1)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_set %d", handle, pb->intin[0]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind && (wdlg = wind->wdlg))
	{
		pb->intout[0] = 1;

		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		
		switch(pb->intin[0])
		{
			/* wdlg_set_edit */
			case 0:
			{
				obj_edit(wdlg->std_wt,
					 pb->intin[1] ? ED_INIT : ED_END,
					 pb->intin[1],
					 0,
					 -1,
					 wt == wdlg->std_wt ? true : false,
					 wt == wdlg->std_wt ? wind->rect_start : NULL,
					 NULL,
					 &pb->intout[0]);
				break;
			}
			/* wdlg_set_tree */
			case 1:
			{
#if 1
				/* HR: rubbish! a new tree is a new dialogue is a new window! */
				OBJECT *obtree;

				pb->intout[0] = 0;
				obtree = (OBJECT *)pb->addrin[1];

				if (!obtree)
					return XAC_DONE;
			
				if ( obtree != wdlg->std_wt->tree)
				{
					RECT r, or;

					wt = obtree_to_wt(client, obtree);
					if (!wt)
						wt = new_widget_tree(client, obtree);

					assert(wt);

					/* XXX  - different when iconified?
					 */
					wdlg->std_wt = wt;
					obtree->ob_state &= ~OS_OUTLINED;
					if (wind->window_status != XAWS_ICONIFIED)
					{
						wt = set_toolbar_widget(lock, wind, obtree, 0);
						wt->exit_form = NULL;
						ob_area(obtree, 0, &or);

						r = calc_window(lock, client, WC_BORDER,
							wind->active_widgets, MG, false, false,
							*(RECT *)&or);

						r.x = wind->r.x;
						r.y = wind->r.y;
						move_window(lock, wind, -1, r.x, r.y, r.w, r.h);
						obtree->ob_x = wind->wa.x;
						obtree->ob_y = wind->wa.y;
					}
				}
				else
					display_window(lock, 200, wind, 0);
				break;
			}
#endif
			/* wdlg_set_size */
			case 2:
			{
				RECT *r = (RECT *)pb->addrin[1];

				if (r)
					move_window(lock, wind, -1, r->x, r->y, r->w, r->h);
				break;
			}
			/* wdlg_iconify */
			case 3:
			{
				if (wind->window_status != XAWS_ICONIFIED)
				{
					RECT *nr = (RECT *)pb->addrin[1];
					char *t = (char *)pb->addrin[2];
					OBJECT *obtree = (OBJECT *)pb->addrin[3];
					short obj = pb->intin[1];
					short i;
					RECT r;

					if (obtree)
					{
						wt = obtree_to_wt(client, obtree);
						if (!wt)
							wt = new_widget_tree(client, obtree);
						assert(wt);
						wdlg->ify_wt = wt;
					}
					else
						wdlg->ify_wt = wdlg->std_wt;

					if (obj >= 0)
						wdlg->ify_obj = obj;
					else
						wdlg->ify_obj = -1;

					if (t)
					{
						char *d;
						d = wdlg->ify_name;
						for (i = 0; i < (sizeof(wdlg->ify_name)-1) && (*d++ = *t++); i++)
							;
						*d = 0;
						get_widget(wind, XAW_TITLE)->stuff = wdlg->ify_name;
					}

					if (!nr || (nr && nr->w == -1 && nr->h == -1))
					{
						r = free_icon_pos(lock);
						nr = &r;
					}
				
					wind->redraw = NULL; //wdlg_redraw;
					wind->save_widgets = wind->active_widgets;
					standard_widgets(wind, NAME|MOVER|ICONIFIER, true);
					move_window(lock, wind, XAWS_ICONIFIED, nr->x, nr->y, nr->w, nr->h);
				}
				break;
			}
			/* wdlg_uniconify */
			case 4:
			{
				if (wind->window_status == XAWS_ICONIFIED)
				{
					OBJECT *obtree = (OBJECT *)pb->addrin[3];
					char *t = (char *)pb->addrin[2];
					RECT *nr = (RECT *)pb->addrin[1];
					RECT r;

					wind->redraw = NULL;
					standard_widgets(wind, wind->save_widgets, true);

					if (!t)
						t = wdlg->std_name;
					{
						char *d;
						int i;
						d = wind->wname;
						for (i = 0; i < (sizeof(wind->wname)-1) && (*d++ = *t++); i++)
							;
						get_widget(wind, XAW_TITLE)->stuff = wind->wname;
					}

					if (obtree)
					{
						wt = obtree_to_wt(client, obtree);
						if (!wt)
							wt = new_widget_tree(client, obtree);
						assert(wt);
					}
					else
						wt = wdlg->std_wt;
					
					if (wt != get_widget(wind, XAW_TOOLBAR)->stuff)
					{
						wt = set_toolbar_widget(lock, wind, obtree, 0);
						wt->exit_form = NULL;
					}

					if (!nr)
					{
						nr = &r;
						r.x = obtree->ob_x = wind->wa.x;
						r.y = obtree->ob_y = wind->wa.y;
						r.w = obtree->ob_width;
						r.h = obtree->ob_height;
					}
					move_window(lock, wind, XAWS_OPEN, nr->x, nr->y, nr->w, nr->h);
				}
				pb->intout[0] = 1;
				break;
			}
			default:
			{
				pb->intout[0] = 0;
			}
		}
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_event(enum locks lock, struct xa_client *client, AESPB *pb)
{
	enum locks wlock = lock|winlist;
	struct xa_window *wind, *top;
	struct wdlg_info *wdlg;
	short handle;
	short ret = 1, cont = 1;

	CONTROL(0,1,0)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];
	pb->intout[0] = 1;

	DIAG((D_wdlg, client, "(%d)XA_wdlg_event", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind && (wdlg = wind->wdlg))
	{
		XA_TREE *wt = get_widget(wind, XAW_TOOLBAR)->stuff;

		if (wt)
		{
			EVNT *ev = (EVNT*)pb->addrin[1];
			if (ev)
			{
				OBJECT *obtree;
				struct moose_data md;
				short events, obj, nxtobj, dc;

				events = ev->mwhich;

				if (ev->mwhich & MU_MESAG)
				{
					if ((ret = wdlg_mesag(lock, wind, wt, ev)))
						ev->mwhich &= ~MU_MESAG;
				}

				top = window_list;

				if (ev->mwhich & MU_BUTTON)
					cpy_ev2md(ev, &md);

				if (ret && (ev->mwhich & MU_BUTTON))
				{
					struct xa_window *cwind;
 					cwind = find_window(wlock, ev->mx, ev->my);

 					if (cwind && wind == cwind && (wind == top || (wind->active_widgets & NO_TOPPED)) )
					{
						if ( (obj = ob_find(wt->tree, 0,7, ev->mx, ev->my)) >= 0)
						{
							ev->mwhich &= ~MU_BUTTON;
							if (wind->window_status != XAWS_ICONIFIED)
							{
								DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON): doing form_do on obj=%d for %s",
									obj, client->name));
								if ( !form_button(wt,			/* widget tree	*/
										  obj,			/* Object	*/
										  &md,			/* moose data	*/
										  true,			/* redraw flag	*/
										  wind->rect_start,	/* rect list	*/
										  NULL,			/* new state	*/
										  &nxtobj,		/* next obj	*/
										  &dc))			/* dc mask	*/
								{
									DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON): call wdlg->exit(%lx) with exitobj=%d for %s",
										wdlg->exit, nxtobj, client->name));
									ret = wdlg->exit(wdlg->handle, ev, nxtobj, ev->mclicks, wdlg->user_data);
								}
								else
								{
									if ( object_is_editable(wt->tree + nxtobj) && nxtobj != wt->e.obj)
									{
										if (wt->e.obj >= 0)
											obj_edit(wt, ED_END, 0, 0, 0, true, wind->rect_start, NULL, NULL);
										obj_edit(wt, ED_INIT, nxtobj, 0, -1, true, wind->rect_start, NULL, &nxtobj);
										DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON): Call wdlg->exit(%lx) with new editobj=%d for %s",
											wdlg->exit, nxtobj, client->name));
										ret = wdlg->exit(wdlg->handle, ev, HNDL_EDCH, ev->mclicks, &nxtobj);
									}
								}							
							}
							else if (ev->mclicks == 2)
							{
								DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON): Generate WM_UNICONIFY for %s",
									client->name));
								ev->mclicks = 0;
								ev->mwhich |= MU_MESAG;
								ev->msg[0] = WM_UNICONIFY;
								ev->msg[3] = wind->handle;
								ev->msg[4] = -1;
								ev->msg[5] = -1;
								ev->msg[6] = -1;
								ev->msg[7] = -1;
								ret = wdlg_mesag(lock, wind, wt, ev);
							}
						}
					}
					/* Ozk:
					 * Window needs to be topped
					 */
					else if (cwind && wind == cwind)
					{
						DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON): topping window %d for %s",
							wind->handle, client->name));

						if (is_hidden(wind))
							unhide_window(wlock, wind);
						top_window(wlock, wind, 0);
						swap_menu(wlock, wind->owner, true, 10);
						after_top(wlock, true);
						ret = 1;
						cont = 0;
					}
					DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON) done! (ret=%d, cont=%d)", ret, cont));
				}

				if ( ret && cont && wind == top && (ev->mwhich & MU_KEYBD) )
				{
					unsigned short key = ev->key;

					obtree = wt->tree;
					nxtobj = form_cursor(wt, ev->key, wt->e.obj, true, wind->rect_start);
					
					if (nxtobj >= 0)
					{
						DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): call HNDL_EDCH exit(%lx) with new edobj=%d for %s",
							nxtobj, client->name));
						ret = wdlg->exit(wdlg->handle, ev, HNDL_EDCH, 0, &nxtobj);
					}
					else 
					{
						if (key == 0x1c0d || key == 0x720d)
						{
							nxtobj = ob_find_flst(obtree, OF_DEFAULT, 0, 0, OS_DISABLED, OF_LASTOB, 0);
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): Got RETURN key - default obj=%d for %s",
								nxtobj, client->name));
						}
						else if (key == 0x6100)
						{
							nxtobj = ob_find_cancel(obtree);
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): Got TAB - cancel obj=%d for %s",
								nxtobj, client->name));
						}
						else
						{
							short ks = ev->kstate;
							short nk = normkey(ks, key);
							
							if ( (ks & (K_CTRL|K_ALT)) == K_ALT )
								nxtobj = ob_find_shortcut(obtree, nk);
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): shortcut %d for %s",
								nxtobj, client->name));
						}
						if (nxtobj >= 0)
						{
							if (!(events & MU_BUTTON))
							{
								check_mouse(client, &md.cstate, &md.x, &md.y);
								md.state = MBS_RIGHT;
							}
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): doing form_button on obj=%d for %s",
								nxtobj, client->name));
							if ( !form_button(wt,
									  nxtobj,
									  &md,
									  true,
									  wind->rect_start,
									  NULL,
									  &nxtobj,
									  &dc))
							{
								DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): call exit(%lx) with exitobj=%d for %s",
									wdlg->exit, nxtobj, client->name));
								ret = wdlg->exit(wdlg->handle, ev, nxtobj, 0 /*ev->mclicks*/, wdlg->user_data);
							}
						}
						else if (key != 0x1c0d && key != 0x720d)
						{
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): HNDL_EDIT exit(%lx) with key=%x for %s",
								wdlg->exit, key, client->name));
							obj_edit(wt,
								 ED_CHAR,
								 wt->e.obj,
								 key,
								 0,
								 true,
								 wind->rect_start,
								 NULL,
								 NULL);
							ret = wdlg->exit(wdlg->handle, ev, HNDL_EDIT, 0, &key);
							ev->mwhich &= ~MU_KEYBD;
						}
					}
					if (nxtobj >= 0)
						ev->mwhich &= ~MU_KEYBD;
				} /* end if ( wind == top && (ev->mwhich & MU_KEYBD) ) */
			} /* end if (ev) */
		} /* end if (wt) */
	} /* end if (wind && (wdlg = wind->wdlg)) */

	DIAG((D_wdlg, NULL, "wdlg_event done, return %d", ret));

	pb->intout[0] = ret;

	return XAC_DONE;
}
unsigned long
XA_wdlg_redraw(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	//struct wdlg_info *wdlg;
	short handle;

	CONTROL(2,0,2)

	pb->intout[0] = 1;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_redraw", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);

	wdlg_redraw(lock, wind, pb->intin[0], pb->intin[1], (RECT *)pb->addrin[1]);

	return XAC_DONE;
}

bool
click_wdlg_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	struct xa_client *client = wind->owner;

	if (client->waiting_for & MU_BUTTON)
	{
		button_event(lock, client, md);
	}
	else
	{
		add_pending_button(lock, client);
	}
	return true;
}

#endif /* WDIALOG_WDLG */
