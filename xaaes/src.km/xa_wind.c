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

#include "xa_wind.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "menuwidg.h"
#include "obtree.h"
#include "rectlist.h"
#include "version.h"
#include "widgets.h"

#include "xa_form.h"
#include "xa_graf.h"
#include "xa_rsrc.h"


/*
 * AES window handling functions
 * I've tried to support some of the AES4 extensions here as well as the plain
 * single tasking GEM ones.
 */
unsigned long
XA_wind_create(int lock, struct xa_client *client, AESPB *pb)
{
	const GRECT r = *((const GRECT *)&pb->intin[1]);
	struct xa_window *new_window;
	XA_WIND_ATTR kind = (unsigned short)pb->intin[0];

	CONTROL3(5,1,0, 5,5,0, 6,5,0)

	if (pb->control[N_INTIN] >= 6)
	{
		kind |= (unsigned long)(unsigned short)pb->intin[5] << 16;
	}

	if (!client->options.nohide)
		kind |= HIDE;

	DIAG((D_wind, client, "wind_create: kind=%x, tp=%lx",
		pb->intin[0], kind));

	new_window = create_window(lock,
				   send_app_message,
				   NULL,
				   client,
				   false,
				   kind|BACKDROP,
				   created_for_CLIENT,
				   client->options.thinframe,
				   client->options.thinwork,
				   &r,
				   &r,
				   NULL);

	if (new_window)
	{
		if (pb->control[N_INTOUT] >= 5)
		{
			if (new_window->opts & XAWO_WCOWORK)
			{
				*(GRECT *)(pb->intout + 1) = new_window->rwa;
			}
			else
			{
				*(GRECT *)(pb->intout + 1) = new_window->rc;
			}
		}
		/* Return the window handle in intout[0] */
		pb->intout[0] = new_window->handle;
	}
	else
		/* Fail to create window, return -ve number */
		pb->intout[0] = -1;

	/* Allow the kernal to wake up the client - we've done our bit */
	return XAC_DONE;
}

unsigned long
XA_wind_open(int lock, struct xa_client *client, AESPB *pb)
{
	GRECT r;
	struct xa_window *w;

	CONTROL(5,1,0)

	/* Get the window */
	w = get_wind_by_handle(lock, pb->intin[0]);
	if (!w)
	{
		pb->intout[0] = 0;
	}
	else
	{
		if (w->opts & XAWO_WCOWORK)
			r = w2f(&w->delta, (const GRECT *)(pb->intin + 1), true);
		else
			r = *(const GRECT *)(pb->intin + 1);

		/* XXX - ozk:
		 *	Is it correct to adjust the max size of windows here when they are
		 *	opened? If so, why does wind_create() even take rectange as parameter?
		 */
		inside_minmax(&r, w);
#if 0
		if (w->active_widgets & USE_MAX)
		{
			/* for convenience: adjust max */
			if (r.g_w > w->max.g_w)
				w->max.g_w = r.g_w;
			if (r.g_h > w->max.g_h)
				w->max.g_h = r.g_h;
		}
#endif

		pb->intout[0] = open_window(lock, w, r);
	}

	return XAC_DONE;
}

unsigned long
XA_wind_close(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *w;

	CONTROL(1,1,0)

	DIAG((D_wind,client,"XA_wind_close"));

	/* Get the window */
	w = get_wind_by_handle(lock, pb->intin[0]);
	if (w == 0)
	{
		DIAGS(("WARNING:wind_close for %s: Invalid window handle %d", c_owner(client), pb->intin[0]));
		pb->intout[0] = 0;
		return XAC_DONE;
	}

	/* Clients can only close their own windows */
	if (w->owner != client)
	{
		DIAGS(("WARNING: %s cannot close window %d (not owner)", c_owner(client), w->handle));
		pb->intout[0] = 0;
		return XAC_DONE;
	}
	pb->intout[0] = close_window(lock, w) ? 1 : 0;

	return XAC_DONE;
}

unsigned long
XA_wind_find(int lock, struct xa_client *client, AESPB *pb)
{
	/* Is there a window under the mouse? */
	struct xa_window *w = find_window(lock, pb->intin[0], pb->intin[1], FNDW_NOLIST|FNDW_NORMAL);

	CONTROL(2,1,0)

	pb->intout[0] = w ? w->handle : 0;

	DIAG((D_wind, client, "wind_find ---> %d", pb->intout[0]));

	return XAC_DONE;
}


#if GENERATE_DIAGS
static const char *setget(int i)
{
	/* Want to see quickly what app's are doing. */
	static const char *const setget_names[] =
	{
	"0",
	"WF_KIND(1)",
	"WF_NAME(2)",
	"WF_INFO(3)",
	"WF_WORKXYWH(4)",
	"WF_CURRXYWH(5)",
	"WF_PREVXYWH(6)",
	"WF_FULLXYWH(7)",
	"WF_HSLIDE(8)",
	"WF_VSLIDE(9)",
	"WF_TOP(10)",
	"WF_FIRSTXYWH(11)",
	"WF_NEXTXYWH(12)",
	"WF_FIRSTAREAXYWH(13)",
	"WF_NEWDESK(14)",
	"WF_HSLSIZE(15)",
	"WF_VSLSIZE(16)",
	"WF_SCREEN(17)",
	"WF_COLOR(18)",
	"WF_DCOLOR(19)",
	"WF_OWNER(20)",
	"21",
	"22",
	"23",
	"WF_BEVENT(24)",
	"WF_BOTTOM(25)",
	"WF_ICONIFY(26)",
	"WF_UNICONIFY(27)",
	"WF_UNICONIFYXYWH(28)",
	"29",
	"WF_TOOLBAR(30)",
	"WF_FTOOLBAR(31)",
	"WF_NTOOLBAR(32)",
	"WF_MENU(33)",
	"34","35","36","37","38","39",
	"WF_WHEEL(40)",
	"WF_OPTS(41)",
	"      "
	};

	static char unknown[32];

	if (i < 0 || i >= sizeof(setget_names) / sizeof(*setget_names))
	{
		sprintf(unknown, sizeof(unknown), "%d", i);
		return unknown;
	}
	return setget_names[i];
}
#endif /* GENERATE_DIAGS */


unsigned long
XA_wind_set(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *w;
	int wind = pb->intin[0];
	int cmd = pb->intin[1];

	CONTROL(6,1,0)

	w = get_wind_by_handle(lock, wind);

	DIAG((D_wind, client, "wind_set for %s  w%lx, h%d, %s", c_owner(client),
		(unsigned long)w, w ? w->handle : -1, setget(cmd)));

	/* wind 0 is the root window */
	if (!w || wind == 0)
	{
		switch(cmd)
		{
		case WF_TOP:			/* MagiC 5: wind_get(-1, WF_TOP) */
		case WF_NEWDESK:		/* Some functions may be allowed without a real window handle */
		case WF_WHEEL:
		case WF_OPTS:
			break;
		default:
			DIAGS(("WARNING:wind_set for %s: Invalid window handle %d", c_owner(client), wind));
			pb->intout[0] = 0;	/* Invalid window handle, return error */
			return XAC_DONE;
		}
	}
	/*
	 * Ozk: These things one app can do to other processes windows
	 */
	else if (w->owner != client && w != root_window)
	{
		switch(cmd)
		{
			case WF_TOP:
			case WF_BOTTOM:
			{
				break;
			}
			default:
			{
				pb->intout[0] = 0;
				return XAC_DONE;
			}
		}
	}
	else if (w->owner != client		/* Clients can only change their own windows */
		 && (w != root_window || cmd != WF_NEWDESK))
	{
		DIAGS(("WARNING: %s cannot change window %d (not owner)", c_owner(client), w->handle));
		pb->intout[0] = 0;		/* Invalid window handle, return error */
		return XAC_DONE;
	}

	pb->intout[0] = 1;

	switch(cmd)
	{
	default:
	{
		pb->intout[0] = 0;
		break;
	}

	/* */
	case WF_WHEEL:
	{
		unsigned long o = 0, om = ~(XAWO_WHEEL);
		short mode = -1;

		if (pb->intin[2] == 1)	/* Enable */
		{
			o |= XAWO_WHEEL;
			mode = pb->intin[3];
			if (mode < 0 || mode > MAX_WHLMODE)
				mode = DEF_WHLMODE;
		}

		if (wind == -1)
		{
			client->options.wind_opts &= om;
			client->options.wind_opts |= o;
			if (mode != -1)
				client->options.wheel_mode = mode;
		}
		else if (w)
		{
			w->opts &= om;
			w->opts |= o;
			if (mode != -1)
				w->wheel_mode = mode;
		}

		if (mode != -1)
			client->options.app_opts |= XAAO_WF_SLIDE;

		break;
	}

	/* */
	case WF_HSLIDE:
	{
		XA_WIDGET *widg;

		widg = get_widget(w, XAW_HSLIDE);
		if (widg->stuff)
		{
			short newpos = bound_sl(pb->intin[2]);

			XA_SLIDER_WIDGET *slw = widg->stuff;
			if (client->options.app_opts & XAAO_WF_SLIDE)
			{
				short newrpos = bound_sl(pb->intin[3]);
				if (newpos != slw->position || newrpos != slw->rpos)
				{
					slw->position = newpos;
					slw->rpos = newrpos;
					display_widget(lock, w, widg, NULL);
				}
			}
			else
			{
				if (slw->position != newpos)
				{
					slw->position = newpos;
					display_widget(lock, w, widg, NULL);
				}
			}
		}
		break;
	}

	/* */
	case WF_VSLIDE:
	{
		XA_WIDGET *widg;

		widg = get_widget(w, XAW_VSLIDE);

		if (widg->stuff)
		{
			short newpos = bound_sl(pb->intin[2]);

			XA_SLIDER_WIDGET *slw = widg->stuff;
			if (client->options.app_opts & XAAO_WF_SLIDE)
			{
				short newrpos = bound_sl(pb->intin[3]);
				if (newpos != slw->position || newrpos != slw->rpos)
				{
					slw->position = newpos;
					slw->rpos = newrpos;
					display_widget(lock, w, widg, NULL);
				}
			}
			else
			{
				if (slw->position != newpos)
				{
					slw->position = newpos;
					display_widget(lock, w, widg, NULL);
				}
			}
		}
		break;
	}

	/* */
	case WF_HSLSIZE:
	{
		XA_WIDGET *widg;

		widg = get_widget(w, XAW_HSLIDE);
		if (widg->stuff)
		{
			short newlen;
			XA_SLIDER_WIDGET *slw = widg->stuff;
			newlen = bound_sl(pb->intin[2]);
			if (slw->length != newlen)
			{
				slw->length = newlen;
				display_widget(lock, w, widg, NULL);
			}
		}
		break;
	}

	/* */
	case WF_VSLSIZE:
	{
		XA_WIDGET *widg;

		widg = get_widget(w, XAW_VSLIDE);
		if (widg->stuff)
		{
			short newlen;
			XA_SLIDER_WIDGET *slw = widg->stuff;
			newlen = bound_sl(pb->intin[2]);
			if (slw->length != newlen)
			{
				slw->length = newlen;
				display_widget(lock, w, widg, NULL);
			}
		}
		break;
	}

	/* set window name line */
	case WF_NAME:
	{
		set_window_title(w, ptr_from_shorts(pb->intin[2], pb->intin[3]), true);
		break;
	}

	/* set window info line */
	case WF_INFO:
	{
		set_window_info(w, ptr_from_shorts(pb->intin[2], pb->intin[3]), true);
		break;
	}
	/* Move a window, check sizes */
	/* Ozk: We support WF_FULLXYWH and WF_PREVXYWH in XaAES.
	 *
	 */
	case WF_FULLXYWH:
	case WF_PREVXYWH:
	case WF_CURRXYWH:
	case WF_WORKXYWH:
	{
		bool blit = false;
		bool move = (pb->intin[2] == -1 && pb->intin[3] == -1 &&
			     pb->intin[4] == -1 && pb->intin[5] == -1) ? true : false;
		GRECT *ir;
		GRECT r;
		GRECT m;
		WINDOW_STATUS status = -1L, msg = -1;

		if (cmd == WF_PREVXYWH)
		{
			DIAGS(("wind_set: WF_PREVXYWH"));
			set_winrect(w, &w->pr, (const GRECT *)(pb->intin + 2));

			if (pb->control[N_INTOUT] >= 5)
			{
				if (w->opts & XAWO_WCOWORK)
					*(GRECT *)(pb->intout + 1) = f2w(&w->delta, &w->pr, true);
				else
					*(GRECT *)(pb->intout + 1) = w->pr;
				DIAGS(("wind_set: WF_PREVXYWH return %d/%d/%d/%d", pb->intout[1], pb->intout[2], pb->intout[3], pb->intout[4]));
			}
			break;
		}
		else if (cmd == WF_FULLXYWH)
		{
			DIAGS(("wind_set: WF_FULLXYWH"));

			set_winrect(w, &w->max, (const GRECT *)(pb->intin + 2));

			if (pb->control[N_INTOUT] >= 5)
			{
				if (w->opts & XAWO_WCOWORK)
					*(GRECT *)(pb->intout + 1) = f2w(&w->delta, &w->max, true);
				else
					*(GRECT *)(pb->intout + 1) = w->max;
				DIAGS(("wind_set: WF_FULLXYWH return %d/%d/%d/%d", pb->intout[1], pb->intout[2], pb->intout[3], pb->intout[4]));
			}
			break;
		}
		else
		{
			/* A WF_CURRXYWH call with all coords set to -1 makes no sense
			 */
			if (move)
				ir = NULL;
			else
			{
				move = true;
				if (cmd == WF_WORKXYWH && !(w->opts & XAWO_WCOWORK))
				{
					r = w2f(&w->delta, (const GRECT *)(pb->intin + 2), true);
				}
				else
					r = *(const GRECT *)(pb->intin + 2);
				ir = &r;
			}

			DIAGS(("wind_set: WF_CURRXYWH - (%d/%d/%d/%d) blit=%s, ir=%lx",
				pb->intin[2], pb->intin[3], pb->intin[4], pb->intin[5], blit ? "yes":"no", (unsigned long)ir));

			if( !(cfg.menu_bar & 1) || w->rc.g_y >= root_window->wa.g_y )
			{
				blit = true;
			}
		}

		if (!move)
		{
			/* Not all coords have a value of -1 meaning we should
			 * modify some (or all) of the coordinates..
			 */
			if (ir)
			{
				if (pb->intin[2] != -1)
					ir->g_x = pb->intin[2];
				if (pb->intin[3] != -1)
					ir->g_y = pb->intin[3];
				if (pb->intin[4] != -1)
					ir->g_w = pb->intin[4];
				if (pb->intin[5] != -1)
					ir->g_h = pb->intin[5];
			}
		}
		else if (ir)
		{
			if (status == -1 && (w->opts & XAWO_WCOWORK))
			{
				m = w2f(&w->delta, ir, true);
				ir = (GRECT *)&w->rwa;
			}
			else
			{
				m = *ir;
				ir = cmd == WF_WORKXYWH ? (GRECT *)&w->rwa : (GRECT *)&w->rc;
			}
		#if 0
			mx = ir->g_x;
			my = ir->g_y;
			mw = ir->g_w;
			mh = ir->g_h;
			ir = cmd == WF_WORKXYWH ? (GRECT *)&w->rwa : (GRECT *)&w->rc;
		#endif
			if ( (m.g_w != w->rc.g_w && (w->opts & XAWO_NOBLITW)) ||
			     (m.g_h != w->rc.g_h && (w->opts & XAWO_NOBLITH)))
				blit = false;

			DIAGS(("wind_set: move to %d/%d/%d/%d for %s",
				m.g_x, m.g_y, m.g_w, m.g_h, client->name));

			if (!m.g_h)
			{
				if (!(w->window_status & XAWS_SHADED))
				{
					DIAGS(("wind_set: zero heigh, shading window %d for %s",
						w->handle, client->name));

					status = XAWS_SHADED|XAWS_ZWSHADED;
					msg = WM_SHADED;
				}
				else
				{
					DIAGS(("wind_set: zero heigh, window %d already shaded for %s",
						w->handle, client->name));
				}
				m.g_h = w->sh;
			}
			else
			{
				if (!(w->window_status & XAWS_SHADED) && status == -1)
				{
					if (w->max.g_w && w->max.g_h &&
					    m.g_x == w->max.g_x &&
					    m.g_y == w->max.g_y &&
					    m.g_w == w->max.g_w &&
					    m.g_h == w->max.g_h)
					{
						status = XAWS_FULLED;
					}
					else
					{
						if (w->window_status & XAWS_FULLED)
							status = ~XAWS_FULLED;
						/* Ozk: I really, really dont like to do this, but it is
						 *	how N.AES does it; Send a full redraw whenever either
						 *	x,y or w,h both changes.
						 *	We do it slightly different using X1/Y1 & X2/Y2 instead
						 *	so we dont render resizing using left/top border useless!
						 *	Hopefully this will be good enough...
						 */
						if (blit && w->opts & XAWO_FULLREDRAW)
						{
							if (m.g_x != w->rc.g_x && m.g_y != w->rc.g_y &&
							    m.g_w != w->rc.g_w && m.g_h != w->rc.g_h)
								blit = false;
						}
					}
				}
				else if ((w->window_status & XAWS_ZWSHADED) && m.g_h != w->sh)
				{
					DIAGS(("wind_set: window %d for %s shaded - unshade by different height",
						w->handle, client->name));

					status = ~(XAWS_SHADED|XAWS_ZWSHADED);
					msg = WM_UNSHADED;
				}
			}
			inside_minmax(&m, w);
		#if 0
			if (w->active_widgets & USE_MAX)
			{
				if (w->max.g_w && m.g_w > w->max.g_w)
					m.g_w = w->max.g_w;
				if (w->max.g_w && m.g_h > w->max.g_h)
					m.g_h = w->max.g_h;
			}
		#endif

			DIAGS(("wind_set: WF_CURRXYWH %d/%d/%d/%d, status = %lx", pb->intin[2], pb->intin[3], pb->intin[4], pb->intin[5], status));

			move_window(lock, w, blit, status, m.g_x, m.g_y, m.g_w, m.g_h);

			if (msg != -1 && w->send_message)
				w->send_message(lock, w, NULL, AMQ_NORM, QMF_CHKDUP, msg, 0, 0, w->handle, 0,0,0,0);
		}

		if (pb->control[N_INTOUT] >= 5)
		{
			if (!ir)
				ir = (GRECT *)&w->rc;
			*(GRECT *)(pb->intout + 1) = *ir;
			DIAGS(("wind_set: WF_CURRXYWH return %d/%d/%d/%d", ir->g_x, ir->g_y, ir->g_w, ir->g_h));
		}
		break;
	}
	/* */
	case WF_BEVENT:
	{
		if (pb->intin[2] & BEVENT_WORK)
			w->active_widgets |= NO_TOPPED;
		else
			w->active_widgets &= ~NO_TOPPED;
		break;
	}

	/* Extension, send window to the bottom */
	case WF_BOTTOM:
	case WF_M_BACKDROP:
	{
		if (w != root_window && (w->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN)
			bottom_window(lock, false, true, w);
		break;
	}

	/* Top the window */
	case WF_TOP:
	{
		if (w == 0)
		{
			if (wind == -1)
			{
				struct xa_client *target;

				if (pb->intin[2] > 0)
					target = pid2client(pb->intin[2]);
				else
					target = focus_owner();

				app_in_front(lock|LOCK_WINLIST, target, true, false, true);
				DIAG((D_wind, NULL, "-1,WF_TOP: Focus to %s", c_owner(target)));
			}
		}
		else if (w != root_window && (w->window_status & (XAWS_OPEN|XAWS_HIDDEN)) == XAWS_OPEN)
		{
			if ( !wind_has_focus(w))
			{
				top_window(lock|LOCK_WINLIST, true, false, w);
			}
		}
		else
			pb->intout[0] = 0;

		break;
	}

	/* Set a new desktop object tree */
	case WF_NEWDESK:
	{
		OBJECT *ob = ptr_from_shorts(pb->intin[2], pb->intin[3]);

		if (ob)
		{
			XA_TREE *wt = obtree_to_wt(client, ob);

			if (!wt || wt != client->desktop)
			{
				if (!wt)
					wt = new_widget_tree(client, ob);

				if (wt)
				{
					DIAGS(("  desktop for %s to (%d/%d,%d/%d)",
						c_owner(client), ob->ob_x, ob->ob_y, ob->ob_width, ob->ob_height));

					/*
					 * In case app dont remove a previously set desktop
					 */
					if (client->desktop && client->desktop != wt)
						set_desktop(client, true);

					client->desktop = wt;
					set_desktop(client, false);
				}
				else if (client->desktop) set_desktop(client, true);
			}
		}
		else if (client->desktop)
		{
			set_desktop(client, true);
			DIAGS(("  desktop for %s removed", c_owner(client)));
		}
		break;
	}
	/* Ozk: According to 'AES.TXT' in the english MagiC documentation,
	 *	wind_set() should return error if attempting to (un)iconfify
	 *	and already (un)iconified window. so we check and return error
	 *	accordingly...
	 */
	case WF_ICONIFY:
	{
		if (is_iconified(w))
			pb->intout[0] = 0;
		else
		{
			GRECT in;

			if (w->opts & XAWO_WCOWORK)
				in = w2f(&w->delta, (const GRECT *)(pb->intin + 2), true);
			else
				in = *((const GRECT *)(pb->intin + 2));

			iconify_window(lock, w, &in);
		}
		if (pb->control[N_INTOUT] >= 5)
		{
			if (w->opts & XAWO_WCOWORK)
				*(GRECT *)(pb->intout + 1) = w->rwa;
			else
				*(GRECT *)(pb->intout + 1) = w->rc;

			DIAGS(("wind_set: WF_ICONIFY return %d/%d/%d/%d", pb->intout[1], pb->intout[2], pb->intout[3], pb->intout[4]));
		}
		break;
	}

	/* Un-Iconify a window */
	case WF_UNICONIFY:
	{
		if (!is_iconified(w))
			pb->intout[0] = 0;
		else
		{
			GRECT in;
			if (pb->intin[4] == -1 || pb->intin[5] == -1 || !(pb->intin[4] | pb->intin[5]))
			{
				in = w->ro;
			}
			else
			{
				if (w->opts & XAWO_WCOWORK)
				{
					in = w2f(&w->save_delta, (const GRECT *)(pb->intin + 2), true);
				}
				else
					in = *((const GRECT *)(pb->intin + 2));
			}
			uniconify_window(lock, w, &in);
		}
		if (pb->control[N_INTOUT] >= 5)
		{
			if (w->opts & XAWO_WCOWORK)
				*(GRECT *)(pb->intout + 1) = w->rwa;
			else
				*(GRECT *)(pb->intout + 1) = w->rc;

			DIAGS(("wind_set: WF_UNICONIFY return %d/%d/%d/%d", pb->intout[1], pb->intout[2], pb->intout[3], pb->intout[4]));
		}
		break;
	}

	case WF_UNICONIFYXYWH:
	{
		if (w->opts & XAWO_WCOWORK)
			w->ro = w2f(&w->save_delta, (const GRECT *)(pb->intin + 2), true);
		else
			w->ro = *((const GRECT *)(pb->intin+2));
		break;
	}
	/* */
	case WF_TOOLBAR:
	{
		OBJECT *ob = ptr_from_shorts(pb->intin[2], pb->intin[3]);
		XA_WIDGET *widg = get_widget(w, XAW_TOOLBAR);

		DIAGS(("  wind_set(WF_TOOLBAR): obtree=%lx, current wt=%lx", (unsigned long)ob, (unsigned long)widg->stuff));

		if (ob)
		{
			XA_TREE *wt;
			short vslw = 0;
			long vsl = 0;

			/* if (wt || !widg->stuff)*/	/* new or changed toolbar */
			{
				GRECT or;
				int md = widg->stuff ? 1 : 0;
				short d;
				struct xa_vdi_settings *v = w->vdi_settings;

				DIAGS(("  --- Set new toolbar"));

				if( md == 1 /*&& ob != o*/ )	/* changed toolbar */
				{
					OBJECT *o = ((XA_TREE*)widg->stuff)->tree;
					d = o->ob_height - ob->ob_height;
					remove_widget(lock, w, XAW_TOOLBAR);
				}
				else	/* new toolbar */
				{
					d = -ob->ob_height;
				}
				/* correct real work-area and min.h */
				w->min.g_h -= d;
				w->rwa.g_h += d;
				w->rwa.g_y -= d;

				/*
				 * correct vslider to new wa
				 */
				if ( (w->active_widgets & (VS_WIDGETS|SIZER)) )
				{
					XA_WIDGET *widg2 = get_widget(w, XAW_VSLIDE);
					/* disable slider, redraw wa, enable new slider */
					vsl = w->active_widgets & (VS_WIDGETS|SIZER);
					w->active_widgets &= ~(VS_WIDGETS|SIZER);

					widg2->r.g_h += d;
					widg2->ar.g_h += d;
					widg2->r.g_y -= d;
					widg2->ar.g_y -= d;

					vslw = widg2->r.g_w;
					w->wa.g_w += vslw;
					w->rwa.g_w += vslw;

					widg2 = get_widget(w, XAW_UPLN1);
					widg2->r.g_y -= d;
					widg2->ar.g_y -= d;
				}

				if( !w->thinwork && md == 0 )
				{
					w->wa.g_y -= 2;
					w->wa.g_h += 2;
				}
				w->wa.g_x = w->r.g_x;

				wt = obtree_to_wt(client, ob);
				if (!wt)
					wt = new_widget_tree(client, ob);
				assert(wt);

				widg->r.g_w = widg->ar.g_w = ob->ob_width = or.g_w = w->r.g_w;
				widg->ar.g_x = ob->ob_x = w->r.g_x;

				/* if( md == 1 ) */	/* changed */
				{
					/* send redraw for wa anyway! */
					/* w->active_widgets &= ~TOOLBAR; */
					/* redraw window without toolbar&vslider */
					generate_redraws(lock, w, &w->r, RDRW_ALL);
				}

				obj_rectangle(wt, aesobj(ob, 0), &or);
				wt = set_toolbar_widget(lock, w, client, ob, aesobj(ob, pb->intin[5]), 0, STW_ZEN, NULL, &or);

				rp_2_ap_cs(w, widg, NULL);
				if (wt && wt->tree)
				{
					wt->tree->ob_x = w->r.g_x;
					wt->tree->ob_y = w->wa.g_y;
					if (!wt->zen)
					{
						wt->tree->ob_x += wt->ox;
						wt->tree->ob_y += wt->oy;
					}
					w->active_widgets |= TOOLBAR;
				}
				w->dial |= created_for_TOOLBAR;

				if ( md == 1 && (w->window_status & (XAWS_OPEN|XAWS_HIDDEN|XAWS_SHADED)) == XAWS_OPEN)
				{
					struct xa_aes_object edobj = aesobj(wt->tree, pb->intin[5]);
					widg->start = pb->intin[4];
					obj_edit(wt, w->vdi_settings, ED_INIT, edobj, 0,0, NULL, false, NULL,NULL, NULL,NULL);

					ob->ob_width = widg->ar.g_w = w->r.g_w;
					/* kludge: avoid garbage ... todo: draw frame */
					(*v->api->f_color)(v, objc_rend.dial_colours.bg_col);
					(*v->api->f_interior)(v, FIS_SOLID);
					(*v->api->gbar)(v, 0, &widg->ar);
				}

				redraw_toolbar(lock, w, 0);
				widg->start = 0;

				if( vsl )	/* restore vslider */
				{
					w->active_widgets |= vsl;
					w->wa.g_w -= vslw;
					w->rwa.g_w -= vslw;
				}

				if( w->r.g_h < w->min.g_h )	/* height may too small for toolbar */
				{
					move_window(lock, w, true, -1L, w->r.g_x, w->r.g_y, w->r.g_w, w->min.g_h);
				}
			}
		}
		else if (widg->stuff)	/* remove toolbar */
		{
			w->min.g_h -= widg->ar.g_h;
			/*
			 * correct vslider
			 */
			if ( (w->active_widgets & VSLIDE) )
			{
				XA_WIDGET *widg2 = get_widget(w, XAW_VSLIDE);
				short d = widg->r.g_h;

				widg2->r.g_h += d;
				widg2->ar.g_h += d;
				widg2->r.g_y -= d;
				widg2->ar.g_y -= d;

				widg2 = get_widget(w, XAW_UPLN1);
				widg2->r.g_y -= d;
				widg2->ar.g_y -= d;
			}
			DIAGS(("  --- Remove toolbar"));
			remove_widget(lock, w, XAW_TOOLBAR);
			w->active_widgets &= ~TOOLBAR;

			/* correct real work-area */
			w->rwa = w->wa;
			generate_redraws(lock, w, &w->r, RDRW_ALL);

		}
		DIAGS(("  wind_set(WF_TOOLBAR) done"));
		break;
	}

	/* */
	case WF_MENU:
	{
		if (w->handle != 0 && (w->active_widgets & XaMENU))
		{
			OBJECT *ob = ptr_from_shorts(pb->intin[2], pb->intin[3]);
			XA_WIDGET *widg = get_widget(w, XAW_MENU);

			if (ob)
			{
				XA_TREE *wt = obtree_to_wt(client, ob);

				if (!wt || (wt != widg->stuff))
				{

					DIAGS(("  --- install new menu"));
					/* fix_menu(client, ob, false); */
					if (!wt)
						wt = new_widget_tree(client, ob);
					assert(wt);
					fix_menu(client, wt, w, false);
					set_menu_widget(w, client, wt);
					rp_2_ap_cs(w, widg, NULL);
					if (wt && wt->tree)
					{
						wt->tree->ob_x = wt->rdx = widg->ar.g_x;
						wt->tree->ob_y = wt->rdx = widg->ar.g_y;
						if (!wt->zen)
						{
							wt->tree->ob_x += wt->ox;
							wt->tree->ob_y += wt->oy;
						}
					}
				}
			}
			else if (widg->stuff)
			{
				DIAGS(("  --- Remove menu"));
				remove_widget(lock, w, XAW_MENU);
				w->active_widgets &= ~XaMENU;
			}
			DIAGS(("  wind_set(WF_MENU) done"));
		}
		break;
	}
	case WF_MINXYWH:
	{
		/* XXX Ozk: Add some boundary checks...?
		*/
		w->min = *((const GRECT *)pb->intin + 2);
		break;
	}
	case WF_SHADE:
	{
		WINDOW_STATUS status = -1L, msg = -1;

		if (pb->intin[2] == 1)
		{
			if (!(w->window_status & XAWS_SHADED))
			{
				status = XAWS_SHADED;
				msg = WM_SHADED;
			}
		}
		else if (pb->intin[2] == 0 )
		{
			if (w->window_status & XAWS_SHADED)
			{
				status = ~XAWS_SHADED;
				msg = WM_UNSHADED;
			}
		}
		else if (pb->intin[2] == -1 )
		{
			if ( (w->window_status & XAWS_SHADED) )
			{
				status = ~XAWS_SHADED;
				msg = WM_UNSHADED;
			}
			else
			{
				status = XAWS_SHADED;
				msg = WM_SHADED;
			}
		}

		DIAGS(("wind_set: WF_SHADE, wind %d, status %lx for %s",
			w->handle, status, client->name));

		if (msg != -1)
		{
			if (w->send_message)
				w->send_message(lock, w, NULL, AMQ_CRITICAL, QMF_CHKDUP,
					msg, 0, 0, w->handle, 0,0,0,0);

			move_window(lock, w, true, status, w->rc.g_x, w->rc.g_y, w->rc.g_w, w->rc.g_h);
		}
		break;
	}
	case WF_OPTS:
	{
		unsigned long  opts, *optr;
		short mode;

		if (w)
			optr = &w->opts;
		else
			optr = &client->options.wind_opts;

		mode = pb->intin[2];
		opts = ((unsigned long)(unsigned short)pb->intin[3] << 16) | (unsigned long)(unsigned short)pb->intin[4];

		if (!mode)
			*optr &= ~opts;
		else if (mode == 1)
			*optr |= opts;

		break;
	}
	case WF_TOPMOST:
	{
		if (w)
		{
			switch (pb->intin[2])
			{
				case 0:
				{
					if ((w->window_status & (XAWS_FLOAT|XAWS_BINDFOCUS)))
					{
						w->window_status &= ~(XAWS_FLOAT|XAWS_SINK|XAWS_BINDFOCUS|XAWS_NOFOCUS);
						if (w->window_status & XAWS_OPEN)
							top_window(lock, true, true, w);
					}
					break;
				}
				case 1:
				{
					if (!(w->window_status & XAWS_FLOAT))
					{
						w->window_status &= ~(XAWS_SINK|XAWS_BINDFOCUS);
						w->window_status |= (XAWS_FLOAT | XAWS_NOFOCUS);
						if (w->window_status & XAWS_OPEN)
							top_window(lock, true, false, w);
						if (!S.focus)
						{
							struct xa_window *nf;
							reset_focus(&nf, 0);
							if (nf)
								setnew_focus(nf, NULL, true, true, true);
						}
					}
					break;
				}
				case 2:
				{
					if (!(w->window_status & (XAWS_FLOAT|XAWS_BINDFOCUS)))
					{
						w->window_status &= ~XAWS_SINK;
						w->window_status |= (XAWS_FLOAT | XAWS_BINDFOCUS | XAWS_NOFOCUS);
						if (w->window_status & XAWS_OPEN)
						{
							if (get_app_infront() == client)
							{
								struct xa_window *nf;

								top_window(lock, true, false, w);
								nf = get_topwind(lock, client, window_list, false, (XAWS_OPEN|XAWS_HIDDEN|XAWS_NOFOCUS), XAWS_OPEN);
								if (!nf)
									nf = get_topwind(lock, client, window_list, false, (XAWS_OPEN|XAWS_NOFOCUS), XAWS_OPEN);
								if (nf)
									setnew_focus(nf, NULL, false, false, true);
							}
							else
								hide_toolboxwindows(client);
						}
					}
					break;
				}
				default:;
			}
		}
	}

	}
	return XAC_DONE;
}
#if 0
/* Convert GEM widget numbers to XaAES widget indices. */
/* -1 means it is not a GEM object. */
static short GtoX[] =
{
	-1,		/* W_BOX,	*/
	-1, 		/* W_TITLE,	*/
	WIDG_CLOSER,	/* W_CLOSER,	*/
	-1,		/* W_NAME,	*/
	WIDG_FULL,	/* W_FULLER,	*/
	-1,		/* W_INFO,	*/
	-1,		/* W_DATA,	*/
	-1,		/* W_WORK,	*/
	WIDG_SIZE,	/* W_SIZER,	*/
	-1,		/* W_VBAR,	*/
	WIDG_UP,	/* W_UPARROW,	*/
	WIDG_DOWN,	/* W_DNARROW,	*/
	-1,		/* W_VSLlDE,	*/
	-1,		/* W_VELEV,	*/
	-1,		/* W_HBAR,	*/
	WIDG_LEFT,	/* W_LFARROW,	*/
	WIDG_RIGHT,	/* W_RTARROW,	*/
	-1,		/* W_HSLIDE,	*/
	-1,		/* W_HELEV,	*/
	WIDG_ICONIFY,	/* W_SMALLER,	*/
	WIDG_HIDE	/* W_BOTTOMER(W_HIDER?) */
};
#endif

/*
 * AES wind_get() function.
 * This includes support for most of the AES4 / AES4.1 extensions,
 */

unsigned long
XA_wind_get(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *w;
	XA_SLIDER_WIDGET *slw;
	short *o = pb->intout;
	GRECT *ro = (GRECT *)&pb->intout[1];
	int wind = pb->intin[0];
	int cmd = pb->intin[1];

	CONTROL3(2,5,0, 3,5,0, 4,5,0)

	w = get_wind_by_handle(lock, wind);

	DIAG((D_wind, client, "wind_get for %s  w=%lx, h:%d, %s",
		c_owner(client), (unsigned long)w, wind, setget(cmd)));

	if (w == 0)
	{
		switch (cmd)
		{
			case WF_BOTTOM: /* Some functions may be allowed without a real window handle */
			case WF_FULLXYWH:
			case WF_TOP:
			case WF_NEWDESK:
			case WF_SCREEN:
			case WF_WHEEL:
			case WF_OPTS:
			case WF_XAAES: /* 'XA', idee stolen from WINX */
				break;
			default:
				DIAGS(("WARNING:wind_get for %s: Invalid window handle %d", c_owner(client), wind));
				o[0] = 0;	/* Invalid window handle, return error */
				o[1] = 0;	/* HR 020402: clear args */
				o[2] = 0;	/*            Prevents FIRST/NEXTXYWH loops with
						      invalid handle from looping forever. */
				o[3] = 0;
				o[4] = 0;

				return XAC_DONE;
		}
	}

	o[0] = 1;

	switch(cmd)
	{
	case WF_KIND:		/* new since N.Aes */
	{
		o[1] = w->active_widgets;
		break;
	}
	case WF_NAME:		/* new since N.Aes */
	{
		if (w->active_widgets & NAME)
			get_window_title(w, (char *)ptr_from_shorts(pb->intin[2], pb->intin[3]));
		else
			o[0] = 0;

		break;
	}
	case WF_INFO:		/* new since N.Aes */
	{
		if (w->active_widgets & INFO)
			get_window_info(w, (char *)ptr_from_shorts(pb->intin[2], pb->intin[3]));
		else
			o[0] = 0;
		break;
	}
	case WF_WHEEL:
	{
		long opt = w ? w->opts : client->options.wind_opts;
		short mode = w ? w->wheel_mode : client->options.wheel_mode;

		o[1] = (opt & XAWO_WHEEL) ? 1 : 0;
		o[2] = mode;
		break;
	}
	case WF_FIRSTAREAXYWH:
	{
		struct xa_rect_list *rl;

		DIAG((D_wind, client, "wind_xget: N_INTIN=%d, (%d/%d/%d/%d) on wind=%d for %s",
			pb->control[N_INTIN], pb->intin[2], pb->intin[3], pb->intin[4], pb->intin[5], w->handle, client->name));

		if (is_shaded(w))
			ro->g_x = ro->g_y = ro->g_w = ro->g_h = 0;
		else
		{
			w->rl_clip = *(const GRECT *)(pb->intin + 2);
			w->use_rlc = true;
			make_rect_list(w, true, RECT_OPT);
			rl = rect_get_optimal_first(w);

			if (rl)
				*ro = rl->r;
			else
				ro->g_x = ro->g_y = ro->g_w = ro->g_h = 0;

			if ((ro->g_w | ro->g_h) && !w->rect_lock)
			{
				w->rect_lock = 1;
				C.rect_lock++;
			}
		}
		break;
	}
	case WF_FTOOLBAR:	/* suboptimal, but for the moment it is more important that it can be used. */
	case WF_NTOOLBAR:
	{
		struct xa_rect_list *rl = NULL;

		if (!is_shaded(w))
		{
			if (cmd == WF_FTOOLBAR)
			{
				make_rect_list(w, true, RECT_TOOLBAR);
				rl = rect_get_toolbar_first(w);
			}
			else
				rl = rect_get_toolbar_next(w);
		}

		if (rl)
		{
			*ro = rl->r;
		}
		else
		{
			ro->g_x = w->wa.g_x;
			ro->g_y = w->wa.g_y;
			ro->g_w = ro->g_h = 0;
		}
		break;

	}
	case WF_FIRSTXYWH:	/* Generate a rectangle list and return the first entry */
	{
		w->use_rlc = false;
		if (is_shaded(w) || !get_rect(&w->rect_list, &w->rwa, true, ro))
		{
			ro->g_x = w->r.g_x;
			ro->g_y = w->r.g_y;
			ro->g_w = ro->g_h = 0;
		}
		else if (!w->rect_lock)
		{
			w->rect_lock = 1;
			C.rect_lock++;
		}
		break;
	}
	case WF_NEXTXYWH:		/* Get next entry from a rectangle list */
	{
		if (is_shaded(w) || !w->rect_lock)
			ro->g_x = ro->g_y = ro->g_w = ro->g_h = 0;
		else
		{
			if (w->use_rlc)
			{
				struct xa_rect_list *rl = rect_get_optimal_next(w);

				if (rl)
					*ro = rl->r;
				else
				{
					ro->g_x = ro->g_y = ro->g_w = ro->g_h = 0;
				}
			}
			else if (!get_rect(&w->rect_list, &w->rwa, false, ro))
			{
				ro->g_x = w->r.g_x;
				ro->g_y = w->r.g_y;
				ro->g_w = ro->g_h = 0;
			}
			if (!(ro->g_w | ro->g_h))
			{
				w->rect_lock = 0L;
				C.rect_lock--;
			}
		}
		break;
	}
	case WF_TOOLBAR:
	{
		XA_TREE *wt = get_widget(w, XAW_TOOLBAR)->stuff;

		if (wt)
			ptr_to_shorts(wt->tree, pb->intout + 1);
		else
			ptr_to_shorts(NULL, pb->intout + 1);

		break;
	}
	case WF_MENU:
	{
		XA_TREE *wt = get_widget(w, XAW_MENU)->stuff;

		if (wt)
			ptr_to_shorts(wt->tree, pb->intout + 1);
		else
			ptr_to_shorts(NULL, pb->intout + 1);
		break;
	}
	/*
	 * The info given by these function is very important to app's
         * hence the DIAG's
	 */

	/*
	 * Get the current coords of the window
	 */
	case WF_CURRXYWH:
	{
		if (w->opts & XAWO_WCOWORK)
			*ro = w->rwa;
		else
			*ro = w->rc;
		DIAG((D_w, w->owner, "get curr for %d: %d/%d,%d/%d",
			wind ,ro->g_x,ro->g_y,ro->g_w,ro->g_h));
		break;
	}
	/*
	 * Get the current coords of the window's user work area
	 */
	case WF_WORKXYWH:
	{
		*ro = w->rwa;
		DIAG((D_w, w->owner, "get work for %d: %d/%d,%d/%d",
			wind ,ro->g_x,ro->g_y,ro->g_w,ro->g_h));
		break;
	}
	/*
	 * Get previous window position
	 */
	case WF_PREVXYWH:
	{
		if (w->opts  & XAWO_WCOWORK)
			*ro = f2w(&w->delta, &w->pr, true);
		else
			*ro = w->pr;
		DIAG((D_w, w->owner, "get prev for %d: %d/%d,%d/%d",
			wind ,ro->g_x,ro->g_y,ro->g_w,ro->g_h));
		break;
	}
	/*
	 * Get maximum window dimensions specified in wind_create() call
	 */
	case WF_FULLXYWH:
	{
		/* This is the way TosWin2 uses the feature.
		 * Get the info and behave accordingly.
		 * Not directly under AES control.
		 */
		if (!w || w == root_window)
		{
			/* Ensure the windows don't overlay the menu bar */
			ro->g_x = root_window->wa.g_x;
			ro->g_y = root_window->wa.g_y;
			ro->g_w = root_window->wa.g_w;
			ro->g_h = root_window->wa.g_h;
			DIAG((D_w, NULL, "get max full: %d/%d,%d/%d",
				ro->g_x,ro->g_y,ro->g_w,ro->g_h));
		}
		else
		{
			if (w->opts & XAWO_WCOWORK)
				*ro = f2w(&w->delta, &w->max, true);
			else
				*ro = w->max;
			DIAG((D_w, w->owner, "get full for %d: %d/%d,%d/%d",
				wind ,ro->g_x,ro->g_y,ro->g_w,ro->g_h));
		}
		break;
	}
	/*
	 * Return supported window features.
	 */
	case WF_BEVENT:
	{
		o[1] = (w->active_widgets & NO_TOPPED) ? BEVENT_WORK : 0;
		o[2] = o[3] = o[4] = 0;
		break;
	}
	/* Extension, gets the bottom window */
	case WF_BOTTOM:
	{
		if ((w = root_window->prev))
		{
			o[1] = w->handle;
			o[2] = w->owner->p->pid;
		}
		else
		{
			o[1] = -1;
			o[2] = 0;
		}
		break;
	}
	case WF_TOP:
	{
		w = S.focus;
		if (!w)
			w = TOP_WINDOW;

		o[1] = o[2] = 0;
		o[3] = o[4] = -1;

		if (w)
		{
			if (w != root_window)
			{
				if (wind_has_focus(w))
				{
					DIAG((D_w, client, " -- top window=%d, owner='%s' has focus", w->handle, w->owner->name));
					o[1] = w->handle;
					o[2] = w->owner->p->pid;
				} else
				{
					DIAG((D_w, client, " -- top window=%d, owner='%s' NOT in focus", w->handle, w->owner->name));
				}
				if (w->next && w->next != root_window)
				{
					o[3] = w->next->handle;
					o[4] = w->next->owner->p->pid;
				}
				else
					o[3] = -1, o[4] = 0;
			}
		}
		else
		{
			DIAGS(("ERROR: empty window list!!"));
			o[1] = 0;		/* No windows open - return an error */
			o[0] = 0;
		}
		break;
	}
	/* AES4 compatible stuff */
	case WF_OWNER:
	case WF_M_OWNER:
	{
		if (w == root_window)
			/* If it is the root window, things arent that easy. */
			o[1] = get_desktop()->owner->p->pid;
		else
			/* The window owners AESid (==app_id == MiNT pid) */
			o[1] = w->owner->p->pid;

		/* Is the window open? */
		o[2] = (w->window_status & XAWS_OPEN) ? 1 : 0;

		if (w->prev)	/* If there is a window above, return its handle */
			o[3] = w->prev->handle;
		else
			o[3] = -1;		/* HR: dont return a valid window handle (root window is 0) */
						/*     Some programs (Multistrip!!) get in a loop */
		if (w->next)	/* If there is a window below, return its handle */
			o[4] = w->next->handle;
		else
			o[4] = -1;

		DIAG((D_wind, client, "  --  owner: %d(%d), above: %d, below: %d", o[1], o[2], o[3], o[4]));
		break;
	}
	case WF_VSLIDE:
	{
		if (w->active_widgets & VSLIDE)
		{
			slw = get_widget(w, XAW_VSLIDE)->stuff;
			o[1] = slw->position;
			if (client->options.app_opts & XAAO_WF_SLIDE)
				o[2] = slw->rpos;
		}
		else
			o[0] = o[1] = 0;
		break;
	}
	case WF_HSLIDE:
	{
		if (w->active_widgets & HSLIDE)
		{
			slw = get_widget(w, XAW_HSLIDE)->stuff;
			o[1] = slw->position;
			if (client->options.app_opts & XAAO_WF_SLIDE)
				o[2] = slw->rpos;
		}
		else
			o[0] = o[1] = 0;
		break;
	}
	case WF_HSLSIZE:
	{
		if (w->active_widgets & HSLIDE)
		{
			slw = get_widget(w, XAW_HSLIDE)->stuff;
			o[1] = slw->length;
		}
		else
			o[0] = o[1] = 0;
		break;
	}
	case WF_VSLSIZE:
	{
		if (w->active_widgets & VSLIDE)
		{
			slw = get_widget(w, XAW_VSLIDE)->stuff;
			o[1] = slw->length;
		}
		else
			o[0] = o[1] = 0;
		break;
	}
	case WF_SCREEN:
	{
		union { long *lp; short *sp;} ptrs;

		/* HR return a very small area :-) hope app's      */
		/*    then decide to allocate a buffer themselves. */
		/*    This worked for SELECTRIC.  */
		/*   Alas!!!! not PS_CONTRL.ACC	  */
		/* So now its all become official */

		/* HR: make the quarter screen buffer for wind_get(WF_SCREEN) :-( :-( */
		/*    What punishment would be adequate for the bloke (or bird) who invented this? */

		if (client->half_screen_buffer == NULL)
		{
			long sc = ((640L * 400 / 2) * screen.planes) / 8;

			DIAGS(("sc: %ld, cl: %ld", sc, client->options.half_screen));

			if (client->options.half_screen)
				/* Use if specified. whether greater or smaller. */
				client->half_screen_size = client->options.half_screen;
			else
				client->half_screen_size = sc;

			client->half_screen_buffer = umalloc(client->half_screen_size);
			DIAGS(("half_screen_buffer for %s: 0x%lx size %ld use %ld",
				c_owner(client),
				(unsigned long)client->half_screen_buffer,
				client->half_screen_size,
				client->options.half_screen));
		}

		ptrs.sp = &o[1];
		*ptrs.lp++ = (long)client->half_screen_buffer;
		*ptrs.lp   = (long)client->half_screen_size;
		break;
	}
	case WF_DCOLOR:
	{
		DIAGS(("WF_DCOLOR %d for %s, %d,%d", o[1], c_owner(client),o[2],o[3]));
		goto oeps;
	}
	case WF_COLOR:
	{
		DIAGS(("WF_COLOR %d for %s", o[1], c_owner(client)));
oeps:
		if (w->active_theme->get_widgcolor)
		{
			if (o[1] <= W_BOTTOMER)
			{
				union { short c[8]; BFOBSPEC cw[4]; } col;

				(*w->active_theme->get_widgcolor)(w, o[1], col.cw);
				/*
				 * Colorword is the last 16 bits of BFOBSPEC (low word)
				 */
				o[2] = col.c[1];
				o[3] = col.c[3];
			}
			else
			{
				o[2] = o[3] = 0x1178;	/* CAB hack */
			}
			o[4] = 0xf0f; 	/* HR 010402 mask and flags 3d */
		}
		else
			o[0] = 0;

		DIAGS(("   --   %04x", o[2]));
		break;
	}
	case WF_NEWDESK:
	{
		Sema_Up(LOCK_DESK);

		ptr_to_shorts(get_desktop()->tree, o + 1);

		Sema_Dn(LOCK_DESK);
		break;
	}
	case WF_ICONIFY:
	{
		o[1] = (is_iconified(w)) ? 1 : 0;
		o[2] = C.iconify.g_w;
		o[3] = C.iconify.g_h;
		break;
	}
	case WF_UNICONIFY:
	{
		GRECT *sr = (is_iconified(w)) ? &w->ro : &w->r;
		if (w->opts & XAWO_WCOWORK)
			*ro = f2w(&w->delta, sr, true);
		else
			*ro = *sr;
		break;
	}
	case WF_MINXYWH:
		*ro = w->min;
		break;
	case WF_SHADE:
	{
		*o = (w->window_status & XAWS_SHADED) ? 1 : 0;
		o[1] = *o;
		break;
	}
	case WF_XAAES: /* 'XA' */
	{
		o[0] = WF_XAAES;
		o[1] = HEX_VERSION;
		DIAGS(("hex_version = v%04x",o[1]));
		break;
	}
	case WF_OPTS:
	{
		unsigned long *optr = (w ? &w->opts : &client->options.wind_opts);

		o[1] = *optr >> 16;
		o[2] = *optr;
		break;
	}

	case WF_CALCF2W:
	{
		*ro = f2w(&w->delta, (const GRECT *)&pb->intin[2], true);
		break;
	}
	case WF_CALCW2F:
	{
		*ro = w2f(&w->delta, (const GRECT *)&pb->intin[2], true);
		break;
	}
	case WF_CALCF2U:
	{
		*ro = f2w(&w->wadelta, (const GRECT *)&pb->intin[2], true);
		break;
	}
	case WF_CALCU2F:
	{
		*ro = w2f(&w->wadelta, (const GRECT *)&pb->intin[2], true);
		break;
	}
	case WF_MAXWORKXYWH:
	{
		*ro = f2w(&w->delta, (const GRECT *)&root_window->wa, true);
		break;
	}
	default:
	{
		DIAG((D_wind,client,"wind_get: %d",cmd));
		o[0] = 0;
	}
	}

	return XAC_DONE;
}

unsigned long
XA_wind_delete(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *w;

	CONTROL(1,1,0)

	w = get_wind_by_handle(lock, pb->intin[0]);
	if (w && !(w->window_status & XAWS_OPEN))
	{
		delete_window(lock, w);
		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_wind_new(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL2(0,0,0, 0,1,0)

	remove_windows(lock, client);
	if (pb->control[2] > 0 && pb->intout)
		pb->intout[0] = 1;

	return XAC_DONE;
}


/*
 * wind_calc
 * HR: embedded function calc_window()
 */
unsigned long
XA_wind_calc(int lock, struct xa_client *client, AESPB *pb)
{
	XA_WIND_ATTR tp;

	CONTROL(6,5,0)

	DIAG((D_wind, client, "wind_calc: req=%d, kind=%d",
		pb->intin[0], pb->intin[1]));

	if (client->options.wind_opts & XAWO_WCOWORK)
		*(GRECT *)&pb->intout[1] = *(const GRECT *)&pb->intin[2];
	else
	{
		tp = (unsigned short)pb->intin[1];

		if (!client->options.nohide)
			tp |= HIDE;

		*(GRECT *) &pb->intout[1] =
			calc_window(lock,
				    client,
				    pb->intin[0],				/* request */
				    tp, /*(unsigned short)pb->intin[1],*/		/* widget mask */
				    created_for_CLIENT,
				    client->options.thinframe,
				    client->options.thinwork,
				    (const GRECT *)&pb->intin[2]);
	}
	pb->intout[0] = 1;
	return XAC_DONE;
}

/*
 * Wind_update handling
 * This handles locking for the update and mctrl flags.
 *
 * The fmd.lock may only be updated in the real wind_update,
 * otherwise screen writing AES functions that use the internal
 * lock/unlock_screen functions spoil the state.
 * This repairs zBench dialogues.
 * Also changed fmd.lock usage to additive flags.
 */

/*
 * Ozk: XA_wind_update() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */

unsigned long
XA_wind_update(int lock, struct xa_client *client, AESPB *pb)
{
	struct proc *p;

	short op = pb->intin[0];
	bool try = (op & 0x100) ? true : false; /* Test for check-and-set mode */

	CONTROL(1,1,0)

	DIAG((D_sema, NULL, "XA_wind_update for %s %s (%d)",
		c_owner(client), try == true ? "" : "RESPONSE", pb->intin[0]));

	pb->intout[0] = 1;

	if (client)
		p = client->p;
	else
		p = get_curproc();

	if (p->ptracer)
	{
		DIAG((D_sema, NULL, "'%s:wind_update(0x%x) ignored for traced program", p->name, op));
		return XAC_DONE;
	}

	switch (op & 0xff)
	{

	/* Grab the update lock */
	case BEG_UPDATE:
	{
		DIAG((D_sema, NULL, "'%s' BEG_UPDATE", p->name));

		if (lock_screen(p, try))
		{
			if (client)
				client->fmd.lock |= SCREEN_UPD;
			C.update_lock = p;
			C.updatelock_count++;
		}
		else if (try)
			pb->intout[0] = 0;

		DIAG((D_sema, NULL, " -- count %d for %s",
			C.updatelock_count, C.update_lock->name));

		break;
	}
	/* Release the update lock */
	case END_UPDATE:
	{
		if (unlock_screen(p))
		{
			if (client)
				client->fmd.lock &= ~SCREEN_UPD;
		}
		if (C.update_lock == p)
		{
			C.updatelock_count--;
			if (!C.updatelock_count)
				C.update_lock = NULL;
		}

		DIAG((D_sema, NULL, "'%s' END_UPDATE", p->name));
		DIAG((D_sema, NULL, " -- count %d for %s",
			C.updatelock_count, p->name));
		break;
	}

	/* Grab the mouse lock */
	case BEG_MCTRL:
	{
		DIAG((D_sema, NULL, "'%s' BEG_MCTRL", client->name));

		if (lock_mouse(p, try))
		{
			if (client)
				client->fmd.lock |= MOUSE_UPD;
			C.mouse_lock = p;
			C.mouselock_count++;
		}
		else if (try)
			pb->intout[0] = 0;

		DIAG((D_sema, NULL, " -- count %d for %s",
			C.mouselock_count, C.mouse_lock->name));
		break;
	}
	/* Release the mouse lock */
	case END_MCTRL:
	{
		if (unlock_mouse(p))
		{
			if (client)
				client->fmd.lock &= ~MOUSE_UPD;
		}
		if (C.mouse_lock == p)
		{
			C.mouselock_count--;
			if (!C.mouselock_count)
				C.mouse_lock = NULL;
		}

		DIAG((D_sema, NULL, "'%s' END_MCTRL", p->name));

		DIAG((D_sema, NULL, " -- count %d for %s",
			C.mouselock_count, p->name));
		break;
	}

	default:
		DIAG((D_sema, NULL, "WARNING! Invalid opcode for wind_update: 0x%04x", op));
	}

	return XAC_DONE;
}
