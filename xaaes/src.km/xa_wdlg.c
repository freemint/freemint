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

#include "xa_wdlg.h"

#include "c_window.h"
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
		RECT r;
		OBJECT *tree = (OBJECT*)pb->addrin[1];

		pb->addrout[0] = 0;

		tree->ob_state &= ~OS_OUTLINED;
		if (tree->ob_x <= 0 && tree->ob_y <= 0)
			center_form(tree, ICON_H);

		r = calc_window(lock, client, WC_BORDER,
				tp,
				MG,
				false, false,
				*(RECT*)&tree->ob_x);

		wind = create_window(lock, send_app_message, client, false,
				     tp,
				     created_for_WDIAL,
				     MG,
				     false, false,
				     r, 0, 0);
		if (wind)
		{
			short rep;

			wt = set_toolbar_widget(lock, wind, tree, 0);
			wt->exit_form = exit_wdial;
			wt->exit_handler = 0;
			wt->wdlg.handle = (void*)(long)wind->handle;
			wt->wdlg.wind = wind;
			wt->wdlg.code = pb->intin[0];			/* Code */
			wt->wdlg.flag = pb->intin[1];			/* Flags */
			wt->wdlg.user_data = (void*)pb->addrin[2];	/* user_data */
			wt->wdlg.data      = (void*)pb->addrin[3];	/* data - is passed to handle_exit() */

#if WDIAL_CALLBACK
			wt->wdlg.exit      = (void*)pb->addrin[0];
			rep = wt->wdlg.exit(0, 0, HNDL_INIT, wt->wdlg.code, wt->wdlg.data);
			if (rep == 0)
				delete_window(lock, wind);
			else
				(long)pb->addrout[0] = 0xae000000 + wind->handle;
#else
				(long)pb->addrout[0] = 0xae000000 + wind->handle;

			wt->wdlg.exit = 0;
#endif
		}			
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	XA_TREE *wt;
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
	if (wind)
	{
		RECT r = wind->r;
		XA_WIND_ATTR tp = pb->intin[0];

		tp &= CLOSER|MOVER|NAME;

		/* The following if is a result of the clumsiness of the
		 * WDIALOG interface. So dont blame me. */

		/* recreate window with final widget set. */
		if ((short)tp != (short)wind->active_widgets)
		{
			OBJECT *tree;

			r = calc_window(lock, client, WC_BORDER,
					tp, MG, false, false,
					*(RECT *)&tree->ob_x);

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

		if (pb->addrin[1])
			get_widget(wind, XAW_TITLE)->stuff = (void*)pb->addrin[1];

		open_window(lock, wind, r);
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		wt->wdlg.data = (void*)pb->addrin[2];

#if WDIAL_CALLBACK
		wt->wdlg.exit(wt->wdlg.handle, 0, HNDL_OPEN, pb->intin[3], wt->wdlg.data);
#endif
		pb->intout[0] = handle;
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	short handle;

	CONTROL(0,3,1)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_close", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind)
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
	short handle;

	CONTROL(0,1,1)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_delete", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind)
	{
		delete_window(lock, wind);
		pb->intout[0] = 1;
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	XA_TREE *wt;
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
	if (wind)
	{
		pb->intout[0] = 1;
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		if (wt == 0)
		{
			/* Not the correct window */
			DIAG((D_wdlg, client, " -- widget gone"));
			pb->intout[0] = 0;
		}
		else
		{
			switch (pb->intin[0])
			{

			/* wdlg_get_tree */
			case 0:
			{
				r = (RECT*)pb->addrin[2];
				if (r)
					*r = wind->wa;
				if (pb->addrin[1])
					*(OBJECT **)pb->addrin[1] = wt->tree;
				DIAG((D_wdlg, client, " -- tree %lx (%d/%d,%d/%d)", wt->tree, *r));		
				break;
			}
			/* wdlg_get_edit */
			case 1:
			{
				pb->intout[0] = wt->edit_obj;
				if (wt->edit_obj < 0)
					pb->intout[0] = 0;
				pb->intout[1] = wt->edit_pos;
				DIAG((D_wdlg, client, " ---> edit_obj %d, edit_pos %d", pb->intout[0], pb->intout[1]));
				break;
			}
			/* wdlg_get_udata */
			case 2:
			{
				if (pb->addrout)
				{
					pb->addrout[0] = (long)wt->wdlg.user_data;
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
	if (wind)
	{
		pb->intout[0] = 1;
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		if (wt)
		{
			switch(pb->intin[0])
			{

			/* wdlg_set_edit */
			case 0:
			{
				edit_object(lock, client,
					    pb->intin[1] ? ED_INIT : ED_END,
					    wt, wt->tree, pb->intin[1], 0, &pb->intout[0]);
				break;
			}
			/* wdlg_set_tree */
			case 1:
			{
#if 1
				/* HR: rubbish! a new tree is a new dialogue is a new window! */

				tree = pb->addrin[1];
				if (tree && tree != wt->tree)
				{
					RECT r = tree->r;
					struct wdlg_info keep = wt->wdlg;
					wt = set_toolbar_widget(lock, wind, tree, 0);
					wt->wdlg = keep;
					wt->exit_form = exit_edial;
					wt->exit_handler = 0;
			
					tree->ob_state &= ~OS_OUTLINED;

					move_window(lock, wind, -1, r.x, r.y, r.w, r.h);
				}
				else
					display_window(lock, 200, wind, 0);
				break;
			}
#endif
			/* wdlg_set_size */
			case 2:

			/* wdlg_iconify */
			case 3:
		
			/* wdlg_uniconify */
			case 4:
				/* Not implemented. Will be, but by proper widget settings! */
				pb->intout[0] = 0;
			break;

			default:
				pb->intout[0] = 0;
			}
		}
	}

	return XAC_DONE;
}
static short
wdlg_mesag(enum locks lock, struct xa_window *wind, XA_TREE *wt, EVNT *ev)
{
	enum locks wlock = lock|winlist;
	short *msg = evnt->msg;
	short wh = wind->handle;
	short mh = msg[3];

	DIAGS(("wdlg_mesag: window %d for %s (%x, %x, %x, %x, %x, %x, %x, %x)",
		wind->handle, wind->owner->name,
		msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]));

	if (msg[0] >= 20 && msg[0] <= 39 && !wt->wdlg.exit(wt->wdlg.handle, ev, HNDL_MESG, ev->mclicks, wt->wdlg.user_data) )
		return 0;

	switch (msg[0])
	{
		case WM_REDRAW:
		{
			if (wh != mh)
				return -1;
			display_window(wlock, 1, wind, (RECT *)&msg[4]);
			break;
		}
		case WM_TOPPED:
		{
			if (wh != mh)
				return -1;
			if (is_hidden(wlock, wind))
				unhide_window(wlock, wind;
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
			if ( !wi->wdlg.exit(wt->wdlg.handle, ev, HNDL_MOVE, ev->mclicks, wt->wdlg.user_data) )
				return 0;

			r.x = msg[4], r.y = msg[5];
			r.w = wind->r.w, r.h = wind->r.h;

			if (wind.r.x != r.x || wind.r.y != r.y)
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
			return wi->wdlg.exit(wt->wdlg.handle, ev, HNDL_TOPW, ev->mclicks, wt->wdlg.user_data);
		}
		case WM_UNTOPPED:
		{
			if (wh != mh)
				return -1;
			return wi->wdlg.exit(wt->wdlg.handle, ev, HNDL_UNTP, ev->mclicks, wt->wdlg.user_data);
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
}

static inline void
cpyev2md(EVNT *e, struct moose_data *m)
{
	m->x = e->mx;
	m->y = e->my;
	m->state = e->mbutton;
	m->cstate = 0;
	m->clicks = e->mclicks;
	m->kstate = e->kstate;
}

unsigned long
XA_wdlg_event(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind, *top;
	short handle;
	short ret = 1;

	CONTROL(0,1,0)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];
	pb->intout[0] = 1;

	DIAG((D_wdlg, client, "(%d)XA_wdlg_event", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind)
	{
		XA_TREE *wt = get_widget(wind, XAW_TOOLBAR)->stuff;

		if (wt)
		{
			EVNT *ev = (EVNT*)pb->addrin[1];
			if (ev)
			{
				struct moose_data md;

				if (ev->mwhich & MU_MESAG)
				{
					if ((ret = wdlg_mesag(lock, wind, wt, ev))
						ev->mwhich &= ~MU_MESAG;
				}

				top = window_list;

				if (ret && (ev->mwhich & MU_BUTTON) && (top == wind || wind->active_widget & NO_TOPPED) )
				{
					struct xa_window *cwind;
					short obj, nxtobj;

					cwind = find_window(wlock, ev->mx, ev->my);

					if (cwind && cwind == wind && (object = find_object(wt->tree, 0,7, ev->mx, ev->my,0,0) >= 0) )
					{
						ev->mwhich &= ~MU_BUTTON;
						if (wind->window_status != XAWS_ICONIFIED)
						{
							cpyev2md(ev, &md);
							if ( !form_button(wlock, wt, obj, &md, NULL, &nxtobj)
							{
								ret = wt->wdlg.exit(wt->wdlg.handle, ev, obj, ev->mclicks, wt->wdlg.user_data);
							}
							else
							{
								if ( is_editobj(wt->tree, nxtobj) && nxtobj != wt->edit_obj)
								{
									Nedit_object(lock, wt, ED_END, 0, 0, NULL);
									Nedit_object(lock, wt, ED_INIT, nxtobj, 0, NULL);
									ret = wt->wdlg.exit(wt->wdlg.handle, ev, HNDL_EDCH, ev->mclicks, &nxtobj);
								}
							}							
						}
						else if (ev->mclicks == 2)
						{
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
						else if (wind != top)
						{
							enum locks wlock = lock | winlist;
							if (is_hidden(wind))
								unhide_window(wlock, wind);
							top_window(wlock, wind, 0);
							swap_menu(wlock, w->owner, true, 10);
							after_top(wlock, true);
						}
					}
				}

				if ( wind == top && (ev->mwhich & MU_KEYBD) )
				{
					unsigned short key = ev->key;
					short obj = form_cursor(lock, wt, ev->key, wt->edit_obj);

					if (obj >= 0)
						ret = wt->wdlg.>exit(wt->wdlg.handle, ev, HNDL_EDCH, ev, &obj);
					else if (key 
					{
						

				/* I wnat to see if the programs react. */
				short rep = 999;
	
				wt->wdlg.evnt = ev;
				DIAG((D_wdlg, client, "  --  mwhich %d", ev->mwhich));
				/* There must have been a exit condition. See exit_form() in XA_FORM.C */
				if (wt->which)
				{
#if WDIAL_CALLBACK
					short ob = wt->current&0xff,
					      dbl = wt->current < 0 ? 2 : 1;

					rep = wt->wdlg.exit(	wt->wdlg.handle,
								ev,
								ob,
								dbl,
								wt->wdlg.user_data);
					DIAG((D_wdlg, client, " obj ---> %d",rep));
#endif
					ev->mwhich &= ~wt->which;
					wt->which = 0;
				}
#if WDIAL_CALLBACK
				else if (   (ev->mwhich & MU_MESAG)
				         && (ev->msg[0] == WM_CLOSED))
				{
					rep = wt->wdlg.exit(wt->wdlg.handle,
										ev,
										HNDL_CLSD,
										0,
										wt->wdlg.data);

					DIAG((D_wdlg, client, " HNDL_CLSD ---> %d",rep));
					ev->mwhich &= ~MU_MESAG;
				}

				if (rep == 0)
				{
					close_window(lock, wt->wdlg.wind);
					delete_window(lock, wt->wdlg.wind);
					pb->intout[0] = 0;
				}
#endif
			}
		}
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_redraw(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	short handle;

	CONTROL(2,0,2)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_redraw", handle));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind)
	{
		XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);

		widg->start = pb->intin[0];
		/* We ignore the additional rectangle of the wdlg interface. */
		display_widget(lock, wind, widg);
		widg->start = 0;
		pb->intout[0] = 1;
	}

	return XAC_DONE;
}

#endif /* WDIALOG_WDLG */
