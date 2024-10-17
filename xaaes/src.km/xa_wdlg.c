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

#define WDIAL_CALLBACK 1

#include "xa_global.h"
#include "xa_wdlg.h"
#include "xa_pdlg.h"

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

#include "xa_user_things.h"
#include "nkcc.h"
#include "keycodes.h"
#include "mint/signal.h"

static WidgetBehaviour click_wdlg_widget;



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


static short
callout_exit(struct xa_client *client, struct wdlg_info *wdlg, void *ev, short nxtobj, short mclicks, void *udata, void *feedback)
{
	struct sigaction oact, act;
	struct xa_callout_head *u;
	short ret = 0;

	if (wdlg->exit)
	{
		u = umalloc(xa_callout_user.len + sizeof(struct co_wdlgexit_parms) + 16);
		if (u)
		{
			struct xa_callout_parms *p;
			struct co_wdlgexit_parms *wp;

			bcopy(&xa_callout_user, u, xa_callout_user.len);

			u->sighand_p	+= (long)u;
			u->parm_p	 = (void *)((char *)u->parm_p + (long)u);

			p	= u->parm_p;
			p->func	= (long)wdlg->exit;
			p->plen	= sizeof(struct co_wdlgexit_parms) >> 1;

			wp	= (struct co_wdlgexit_parms *)(&p->parms);

			wp->dialog	= (long)wdlg->handle;
			wp->evnt	= (long)ev;
			wp->obj		= nxtobj;
			wp->clicks	= mclicks;

			if (feedback)
				wp->data	= (long)wp + sizeof(struct co_wdlgexit_parms);
			else
				wp->data	= (long)udata;

			cpushi(u, xa_callout_user.len);

			act.sa_handler	= u->sighand_p;
			act.sa_mask	= 0xffffffff;
			act.sa_flags	= SA_RESETHAND;

			p_sigaction(SIGUSR2, &act, &oact);
			DIAGS(("raise(SIGUSR2)"));
			raise(SIGUSR2);
			DIAGS(("handled SIGUSR2 wdialog->exit callout"));
			/* restore old handler */
			p_sigaction(SIGUSR2, &oact, NULL);

			if (feedback)
			{
				*(short *)feedback = *(short *)((long)wp + sizeof(struct co_wdlgexit_parms));
				DIAGS(("callout_exit: return %d from %lx(%lx) to feedbackptr %lx",
					*(short *)((long)wp + sizeof(struct co_wdlgexit_parms)), (unsigned long)wp->data, (long)wp + sizeof(struct co_wdlgexit_parms),
					(unsigned long)feedback));
			}
			ret = p->ret;

			ufree(u);
		}
	}
	return ret;
}

static inline void
cpy_ev2md(EVNT *e, struct moose_data *m)
{
	m->x = e->mx;
	m->y = e->my;
	m->state = e->mbutton;

	check_mouse(NULL, &m->cstate, &m->sx, &m->sy);

	m->clicks = e->mclicks;
	m->kstate = e->kstate;
}

void
wdialog_redraw(int lock, struct xa_window *wind, struct xa_aes_object start, short depth, GRECT *r)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_rect_list *rl;
	struct widget_tree *wt;

	if (wind && (wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt) && (rl = wind->rect_list.start))
	{
		OBJECT *obtree;
		GRECT dr;
		bool drwcurs = (!wt->ei && edit_set(&wt->e));

		obtree = wt->tree;

		if (!aesobj_tree(&start))
		{
			short item = aesobj_item(&start);
			if (item == -1)
				start = aesobj(obtree, 0);
			else
				start = aesobj(obtree, item);
		}

		lock_screen(wind->owner->p, false);
		hidem();

		if (drwcurs)
		{
			obj_edit(wt, v, ED_END, aesobj(wt->tree, 0), 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);
		}

		if (r)
		{
			while (rl)
			{
				if (xa_rect_clip(&rl->r, r, &dr))
				{
					(*v->api->set_clip)(v, &dr);
					draw_object_tree(0, wt, wt->tree, v, start, depth, NULL, 0);
				}
				rl = rl->next;
			}
		}
		else
		{
			while (rl)
			{
				(*v->api->set_clip)(v, &rl->r);
				draw_object_tree(0, wt, wt->tree, v, start, depth, NULL, 0);
				rl = rl->next;
			}
		}

		if (drwcurs)
			obj_edit(wt, v, ED_END, aesobj(wt->tree, 0), 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);

		showm();
		(*v->api->clear_clip)(v);
		unlock_screen(wind->owner->p);
	}
}

/* Called from draw_window via wind->redraw, so clipping
 * is handled elsewhere
 */
static void
wdlg_redraw(int lock, struct xa_window *wind, struct xa_aes_object start, short depth, GRECT *r)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct wdlg_info *wdlg;
	struct xa_rect_list *rl;

	if ((wdlg = wind->wdlg) && (rl = wind->rect_list.start))
	{
		XA_TREE *wt;
		OBJECT *obtree;
		GRECT dr;
		bool drwcurs;

		if (is_iconified(wind))
		{
			short x = 0, y = 0;

			wt = wdlg->ify_wt;
			obtree = wt->tree;

			if (wdlg->ify_obj >= 0)
			{
				obtree->ob_x = 0;
				obtree->ob_y = 0;
				obj_offset(wt, aesobj(wt->tree, wdlg->ify_obj), &x, &y);
			}
			obtree->ob_x = wind->wa.g_x - x;
			obtree->ob_y = wind->wa.g_y - y;
			start = wdlg->ify_obj >= 0 ? aesobj(obtree, wdlg->ify_obj) : aesobj(obtree, 0);
			depth = 10;
		}
		else
		{
			wt = wdlg->std_wt;
			obtree = wt->tree;
		}

		if (!aesobj_tree(&start))
		{
			short item = aesobj_item(&start);

			if (item == -1)
				start = aesobj(obtree, 0);
			else
				start = aesobj(obtree, item);
		}

		drwcurs = (!wt->ei && edit_set(&wt->e));

		lock_screen(wind->owner->p, false);
		hidem();

		if (drwcurs)
			obj_edit(wt, v, ED_END, aesobj(wt->tree, 0), 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);

		if (r)
		{
			while (rl)
			{
				if (xa_rect_clip(&rl->r, r, &dr))
				{
					(*v->api->set_clip)(v, &dr);
					draw_object_tree(0, wt, wt->tree, v, start, depth, NULL, 0);
				}
				rl = rl->next;
			}
		}
		else
		{
			while (rl)
			{
				(*v->api->set_clip)(v, &rl->r);
				draw_object_tree(0, wt, wt->tree, v, start, depth, NULL, 0);
				rl = rl->next;
			}
		}

		if (drwcurs)
			obj_edit(wt, v, ED_END, aesobj(wt->tree, 0), 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);

		showm();
		(*v->api->clear_clip)(v);
		unlock_screen(wind->owner->p);
	}
}

static short
wdialog_message(int lock, struct xa_client *client, struct wdlg_evnt_parms *wep)
{
	int wlock = lock|LOCK_WINLIST;
	struct xa_window *wind = wep->wind;
	struct wdlg_info *wdlg = wep->wdlg;
	EVNT *ev = wep->ev;
	short *msg = ev->msg;
	short wh = wind->handle;
	short mh = msg[3];

	DIAGS(("wdlg_mesag: window %d for %s (%x, %x, %x, %x, %x, %x, %x, %x)",
		wind->handle, wind->owner->name,
		msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]));

	if (wdlg && !(wdlg = wind->wdlg))
		return -1;

	if (msg[0] >= 20 && msg[0] <= 39)
	{
		if (wep->callout && !(*wep->callout)(wind->owner, wdlg, ev, HNDL_MESG, ev->mclicks, wdlg->user_data, NULL) )
			return 0;
	}

	switch (msg[0])
	{
		case WM_REDRAW:
		{
			if (wh != mh)
				return -1;
			(*wep->redraw)(wlock, wind, aesobj(NULL, 0), 10, (GRECT *)&msg[4]);
			break;
		}
		case WM_TOPPED:
		{
			if (wh != mh)
				return -1;
			if (is_hidden(wind))
				unhide_window(wlock, wind, true);
			top_window(wlock, true, false, wind);
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
			move_window(wlock, wind, true, XAWS_FULLED, wind->max.g_x, wind->max.g_y, wind->max.g_w, wind->max.g_h);
			break;
		}
		case WM_SIZED:
		{
			GRECT mr;
			GRECT r;

			if (wh != mh)
				return -1;

			if (wind->opts & XAWO_WCOWORK)
			{
				mr = w2f(&wind->delta, (GRECT *)(msg + 4), true);
			}
			else
				mr = *(GRECT *)(msg + 4);

			r.g_x = wind->r.g_x;
			r.g_y = wind->r.g_y;
			r.g_w = wind->max.g_w < mr.g_w ? wind->max.g_w : mr.g_w;
			r.g_h = wind->max.g_h < mr.g_h ? wind->max.g_h : mr.g_h;
		#if 0
			r.g_w = wind->max.g_w < msg[6] ? wind->max.g_w : msg[6];
			r.g_h = wind->max.g_h < msg[7] ? wind->max.g_h : msg[7];
		#endif
			if (wind->r.g_w != r.g_w || wind->r.g_h != r.g_h)
			{
				inside_root(&r, wind->owner->options.noleft);
				move_window(wlock, wind, true, -1, r.g_x, r.g_y, r.g_w, r.g_h);
			}
			break;
		}
		case WM_MOVED:
		{
			GRECT mr;
			GRECT r;

			if (wh != mh)
				return -1;

			if (wep->callout)
			{
				if ( !(*wep->callout)(wind->owner, wdlg, ev, HNDL_MOVE, ev->mclicks, wdlg->user_data, NULL))
					return 0;
			}
			if (wind->opts & XAWO_WCOWORK)
			{
				mr = w2f(&wind->delta, (GRECT *)(msg + 4), true);
			}
			else
				mr = *(GRECT *)(msg + 4);

			r.g_x = mr.g_x, r.g_y = mr.g_y;

			if (wind->r.g_x != r.g_x || wind->r.g_y != r.g_y)
			{
				r.g_w = wind->rc.g_w, r.g_h = wind->rc.g_h;
				inside_root(&r, wind->owner->options.noleft);
				move_window(wlock, wind, true, -1, r.g_x, r.g_y, r.g_w, r.g_h);
			}
			break;
		}
		case WM_ONTOP:
		case WM_NEWTOP:
		{
			if (wh != mh)
				return -1;
			if (wep->callout)
				return (*wep->callout)(wind->owner, wdlg, ev, HNDL_TOPW, ev->mclicks, wdlg->user_data, NULL);

			break;
		}
		case WM_UNTOPPED:
		{
			XA_WIDGET *widg = wind->tool;
			if( widg )
			{
				XA_TREE *wt = widg->stuff.wt;
				if( wt )
					ob_set_wind( wt->tree, G_SLIST, msg[0] );
			}

			if (wh != mh)
				return -1;
			if (wep->callout)
				return (*wep->callout)(wind->owner, wdlg, ev, HNDL_UNTP, ev->mclicks, wdlg->user_data, NULL);
			break;
		}
		case WM_BOTTOM:
		{
			if (wh != mh)
				return -1;
			bottom_window(wlock, false, true, wind);
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

struct toolbar_handlers wdlg_th =
{
	(void *)-1L,			/* FormExit		*exitform;	*/
	(void *)-1L,			/* FormKeyInput		*keypress;	*/

	(void *)0,				/* DisplayWidget	*display;	*/
	click_wdlg_widget,		/* WidgetBehaviour	*click;		*/
	click_wdlg_widget,		/* WidgetBehaviour	*drag;		*/
	(void *)-1L,			/* WidgetBehaviour	*release;	*/
	NULL,				/* void (*destruct)(struct xa_widget *w); */
};

unsigned long
XA_wdlg_create(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	XA_TREE *wt;

	CONTROL(2,0,4)

	DIAG((D_wdlg, client, "XA_wdlg_create: exit %lx, tree %lx, user_data %lx, data %lx",
		pb->addrin[0], pb->addrin[1], pb->addrin[2], pb->addrin[3]));

	if (pb->addrin[0] && validate_obtree(client, (OBJECT *)pb->addrin[1], "XA_wdlg_create:") && pb->addrout)
	{
		short swtflags = STW_ZEN|STW_GOC|STW_SWC;
		XA_WIND_ATTR tp = 0;
		GRECT r, or = (GRECT){100,100,100,100};
		OBJECT *obtree = (OBJECT*)pb->addrin[1];

		pb->addrout[0] = 0;

		obtree->ob_state &= ~OS_OUTLINED;

		DIAGS(("XA_wdlg_create: ob=%lx, obx=%d, oby=%d", (unsigned long)obtree, obtree->ob_x, obtree->ob_y));

		if (obtree->ob_x <= 0 && obtree->ob_y <= 0)
			swtflags |= STW_COC;

		r = calc_window(lock, client, WC_BORDER,
				tp, created_for_WDIAL,
				client->options.thinframe,
				client->options.thinwork,
				&or);

		wind = create_window(lock, send_app_message, NULL, client, false,
				     tp,
				     created_for_WDIAL,
				     client->options.thinframe,
				     client->options.thinwork,
				     &r, NULL, NULL);

		if (wind)
		{
			short rep;
			struct wdlg_info *wdlg;

			wdlg = kmalloc(sizeof(*wdlg));
			if (wdlg)
			{
				bzero(wdlg, sizeof(*wdlg));

				wind->wdlg = wdlg;

				wt = set_toolbar_widget(lock, wind, client, obtree, aesobj(obtree, -2), WIP_NOTEXT, swtflags, &wdlg_th, &or);
				obj_init_focus(wt, OB_IF_RESET);

				wdlg->handle = (void *)((long)0xae000000 + wind->handle);
				wdlg->wind = wind;
				wdlg->code = pb->intin[0];		/* Code */
				wdlg->flag = pb->intin[1];		/* Flags */
				wdlg->user_data = (void*)pb->addrin[2];	/* user_data */
				wdlg->data      = (void*)pb->addrin[3];	/* data - is passed to handle_exit() */

				wdlg->std_wt = wt;
				wdlg->ify_wt = wt;

				wdlg->exit      = (void*)pb->addrin[0];
				rep = callout_exit(client, wdlg, NULL, HNDL_INIT, wdlg->code, wdlg->data, NULL);
				if (rep == 0)
					delete_window(lock, wind);
				else
					pb->addrout[0] = (long)wdlg->handle;
			}
			else
				delete_window(lock, wind);
		}
	}
	return XAC_DONE;
}

unsigned long
XA_wdlg_open(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;

	CONTROL(4,1,3)

	pb->intout[0] = 0;
	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "XA_wdlg_open: handle=(%d)%lx, title %lx, data %lx",
		handle, (unsigned long)pb->addrin[0], (unsigned long)pb->addrin[1], (unsigned long)pb->addrin[2]));
	DIAG((D_wdlg, client, "XA_wdlg_open: tp=%x, x=%d, y=%d",
		pb->intin[0], pb->intin[1], pb->intin[2]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);

	if (wind && (wdlg = wind->wdlg))
	{
		GRECT r = wind->wa;
		XA_WIND_ATTR tp = (unsigned short)pb->intin[0];
		char *s;

		/*
		 * work around a bug in ArcView: if you close a dialog,
		 * then open it again, the 2nd time it is created without a NAME widget
		 */
		if (wind->active_widgets & NAME)
			tp |= NAME;

		/* The following if is a result of the clumsiness of the
		 * WDIALOG interface. So dont blame me. */

		/* recreate window with final widget set. */

		DIAG((D_wdlg, client, "XA_wdlg_open: ob=%lx, obx=%d, oby=%d",
			(unsigned long)wdlg->std_wt->tree, wdlg->std_wt->tree->ob_x, wdlg->std_wt->tree->ob_y));

		if (pb->intin[1] >= root_window->wa.g_x)
			r.g_x = pb->intin[1];
		else
			r.g_x = (root_window->wa.g_w - r.g_w) >> 1;

		if (pb->intin[2] >= root_window->wa.g_y)
			r.g_y = pb->intin[2];
		else
			r.g_y = (root_window->wa.g_h - r.g_h) >> 1;

		{
			GRECT or;

			obj_area(wdlg->std_wt, aesobj(wdlg->std_wt->tree, 0), &or);
			or.g_x = r.g_x;
			or.g_y = r.g_y;
			change_window_attribs(lock, client, wind, tp, true, true, 2, or, NULL);
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
			get_widget(wind, XAW_TITLE)->stuff.name = wind->wname;
		}

		obj_init_focus(wdlg->std_wt, OB_IF_RESET);

		DIAG((D_wdlg, client, "XA_wdlg_open: wa.x=%d, wa=%d", wind->wa.g_x, wind->wa.g_y));

		wdlg->std_wt->tree->ob_x = wind->wa.g_x;
		wdlg->std_wt->tree->ob_y = wind->wa.g_y;
		open_window(lock, wind, wind->rc);
		wdlg->data = (void *)pb->addrin[2];
		callout_exit(client, wdlg, NULL, HNDL_OPEN, pb->intin[3], wdlg->data, NULL);

		pb->intout[0] = handle;
	}
	return XAC_DONE;
}

unsigned long
XA_wdlg_close(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;

	CONTROL2(0,3,1, 0,1,1)

	pb->intout[0] = 0;
	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_close, handle=%lx", handle, pb->addrin[0]));
	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind && (wdlg = wind->wdlg))
	{
		if (pb->control[N_INTOUT] >= 3)
		{
			pb->intout[1] = wind->r.g_x;
			pb->intout[2] = wind->r.g_y;
		}
		close_window(lock, wind);
		pb->intout[0] = 1;
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_delete(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;

	CONTROL(0,1,1)

	pb->intout[0] = 0;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_delete, handle=%lx",
		handle, pb->addrin[0]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);

	if (wind && (wdlg = wind->wdlg))
	{
		if ((wind->window_status & XAWS_OPEN))
			close_window(lock, wind);

		kfree(wdlg);
		wind->wdlg = NULL;
		delete_window(lock, wind);
		pb->intout[0] = 1;
	}

	return XAC_DONE;
}

unsigned long
XA_wdlg_get(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;
	GRECT *r;

	CONTROL(1,0,1)

	pb->intout[0] = 0;
	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_get %d, handle=%lx",
		handle, pb->intin[0], pb->addrin[0]));

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
				r = (GRECT*)pb->addrin[2];
				if (r)
					*r = wind->wa;
				if (pb->addrin[1])
					*(OBJECT **)pb->addrin[1] = wdlg->std_wt->tree;
				DIAG((D_wdlg, client, " -- tree %lx (%d/%d,%d/%d)", (unsigned long)wdlg->std_wt->tree, r->g_x, r->g_y, r->g_w, r->g_h));
				break;
			}
			/* wdlg_get_edit */
			case 1:
			{
				struct objc_edit_info *ei;

				if (wdlg->std_wt->ei)
					ei = wdlg->std_wt->ei;
				else
					ei = &wdlg->std_wt->e;

				if (!ei || !edit_set(ei))
				{
					pb->intout[0] = 0;
					pb->intout[1] = 0;
				}
				else
				{
					pb->intout[0] = edit_item(ei);
					pb->intout[1] = ei->pos;
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
				pb->intout[0] = 0;
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
XA_wdlg_set(int lock, struct xa_client *client, AESPB *pb)
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

	DIAG((D_wdlg, client, "(%d)XA_wdlg_set %d, handle=%lx",
		handle, pb->intin[0], pb->addrin[0]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);

	if (wind && (wdlg = wind->wdlg))
	{
		pb->intout[0] = 1;

		wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;

		switch(pb->intin[0])
		{
			/* wdlg_set_edit */
			case 0:
			{
				struct xa_aes_object out;
				obj_edit(wdlg->std_wt,
					 wind->vdi_settings,
					 pb->intin[1] ? ED_INIT : ED_END,
					 aesobj(wdlg->std_wt->tree, pb->intin[1]),
					 0,
					 -1,
					 NULL,
					 wt == wdlg->std_wt ? true : false,
					 &wind->wa,
					 wt == wdlg->std_wt ? wind->rect_list.start : NULL,
					 NULL,
					 &out);

				pb->intout[0] = aesobj_item(&out);
				break;
			}
			/* wdlg_set_tree */
			case 1:
			{
				OBJECT *obtree;

				pb->intout[0] = 0;
				obtree = (OBJECT *)pb->addrin[1];

				if (!validate_obtree(client, obtree, "XA_wdlg_set_tree:"))
					return XAC_DONE;

				if ( obtree != wdlg->std_wt->tree)
				{
					GRECT r, or;

					wt = obtree_to_wt(client, obtree);
					if (!wt)
						wt = new_widget_tree(client, obtree);

					assert(wt);

					/* XXX  - different when iconified?
					 */
					wdlg->std_wt = wt;
					obtree->ob_state &= ~OS_OUTLINED;
					if (!is_iconified(wind))
					{
						obj_rectangle(wt, aesobj(obtree, 0), &or);
						set_toolbar_widget(lock, wind, client, obtree, aesobj(obtree, 0), WIP_NOTEXT, STW_ZEN, &wdlg_th, &or);
						wt->exit_form = NULL;
						r = calc_window(lock, client, WC_BORDER,
							wind->active_widgets, wind->dial,
							client->options.thinframe,
							client->options.thinwork,
							&or);

						r.g_x = wind->r.g_x;
						r.g_y = wind->r.g_y;
						move_window(lock, wind, true, -1, r.g_x, r.g_y, r.g_w, r.g_h);
						obtree->ob_x = wind->wa.g_x;
						obtree->ob_y = wind->wa.g_y;
					}
				}
				else
				{
					display_window(lock, 200, wind, NULL);
				}
				break;
			}
			/* wdlg_set_size */
			case 2:
			{
				GRECT *r = (GRECT *)pb->addrin[1];

				if (r)
				{
					GRECT nr = *r;

					nr.g_x -= wind->wa.g_x - wind->rc.g_x;
					nr.g_y -= wind->wa.g_y - wind->rc.g_y;
					nr.g_w += wind->rc.g_w - wind->wa.g_w;
					nr.g_h += wind->rc.g_h - wind->wa.g_h;
					move_window(lock, wind, true, -1, nr.g_x, nr.g_y, nr.g_w, nr.g_h);
				}
				break;
			}
			/* wdlg_iconify */
			case 3:
			{
				if (!is_iconified(wind))
				{
					GRECT *nr = (GRECT *)pb->addrin[1];
					char *t = (char *)pb->addrin[2];
					OBJECT *obtree = (OBJECT *)pb->addrin[3];
					short obj = pb->intin[1];
					short i;
					GRECT r;

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
						get_widget(wind, XAW_TITLE)->stuff.name = wdlg->ify_name;
					}

					if (!nr || (nr && nr->g_w == -1 && nr->g_h == -1))
					{
						r = free_icon_pos(lock, NULL);
						nr = &r;
					}

					wind->redraw = NULL;
					move_window(lock, wind, true, XAWS_ICONIFIED, nr->g_x, nr->g_y, nr->g_w, nr->g_h);
				}
				break;
			}
			/* wdlg_uniconify */
			case 4:
			{
				if (is_iconified(wind))
				{
					OBJECT *obtree = (OBJECT *)pb->addrin[3];
					char *t = (char *)pb->addrin[2];
					GRECT *nr = (GRECT *)pb->addrin[1];
					GRECT r;

					wind->redraw = NULL;

					if (!t)
						t = wdlg->std_name;
					{
						char *d;
						int i;
						d = wind->wname;
						for (i = 0; i < (sizeof(wind->wname)-1) && (*d++ = *t++); i++)
							;
						get_widget(wind, XAW_TITLE)->stuff.name = wind->wname;
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

					if (wt != get_widget(wind, XAW_TOOLBAR)->stuff.wt)
					{
						wt = set_toolbar_widget(lock, wind, client, obtree, aesobj(obtree, 0), WIP_NOTEXT, STW_ZEN|STW_GOC, NULL, NULL);
						wt->exit_form = NULL;
					}

					if (!nr)
					{
						nr = &r;
						r.g_x = /*obtree->ob_x =*/ wind->wa.g_x;
						r.g_y = /*obtree->ob_y =*/ wind->wa.g_y;
						r.g_w = obtree->ob_width;
						r.g_h = obtree->ob_height;
					}

					move_window(lock, wind, true, ~XAWS_ICONIFIED, nr->g_x, nr->g_y, nr->g_w, nr->g_h);
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

short
wdialog_event(int lock, struct xa_client *client, struct wdlg_evnt_parms *wep)
{
	short ret = 1, cont = 1;
	struct widget_tree *wt;

	if ((wt = wep->wt))
	{
		EVNT *ev = wep->ev;
		if (ev)
		{
			struct xa_window *top, *wind = wep->wind;
			struct xa_vdi_settings *v = wind->vdi_settings;
			OBJECT *obtree;
			struct objc_edit_info *ei;
			struct moose_data md;
			short events, dc;
			struct xa_aes_object nxtobj, obj = {0};

			events = ev->mwhich;

			if (ev->mwhich & MU_MESAG)
			{
				if ((ret = wdialog_message(lock, client, wep)) > 0)
					ev->mwhich &= ~MU_MESAG;
			}

			top = TOP_WINDOW;

			if (ev->mwhich & MU_BUTTON)
				cpy_ev2md(ev, &md);

			if (ret && (ev->mwhich & MU_BUTTON))
			{
				struct xa_window *cwind;
				cwind = find_window(lock, ev->mx, ev->my, FNDW_NOLIST|FNDW_NORMAL);

				if (cwind && wep->wind == cwind && (wind == top || (wind->active_widgets & NO_TOPPED)) )
				{
					obj = obj_find(wt, aesobj(wt->tree, 0),7, ev->mx, ev->my, NULL);

					if (valid_aesobj(&obj))
					{
						ev->mwhich &= ~MU_BUTTON;
						if (!is_iconified(wind))
						{
							DIAG((D_wdlg, NULL, "wdialog_event(MU_BUTTON): doing Form_Button on obj=%d for %s",
								aesobj_item(&obj), client->name));
							if ( !Form_Button(wt,			/* widget tree	*/
									  v,			/* VDI settings & api */
									  obj,			/* Object	*/
									  &md,			/* moose data	*/
									  FBF_REDRAW|FBF_DO_SLIST|FBF_CHANGE_FOCUS,/* redraw flag	*/
									  &wind->rect_list.start,	/* rect list	*/
									  NULL,			/* new state	*/
									  &nxtobj,		/* next obj	*/
									  &dc))			/* dc mask	*/
							{
								DIAG((D_wdlg, NULL, "wdialog_event(MU_BUTTON): call wdlg->exit(%lx) with exitobj=%d for %s",
									(unsigned long)(wep->wdlg ? wep->wdlg->exit : NULL), aesobj_item(&nxtobj), client->name));
								wep->obj = nxtobj;
								if (wep->callout)
									ret = (*wep->callout)(client, wep->wdlg, ev, aesobj_item(&nxtobj), ev->mclicks, wep->wdlg->user_data, NULL);
								else
									ret = 0;
							}
							else
							{
								ei = &wt->e;

								if (valid_aesobj(&nxtobj) && aesobj_is_editable(&nxtobj, 0, 0) && !same_aesobj(&nxtobj, &ei->o))
								{
									if (edit_set(ei))
										obj_edit(wt, v, ED_END, aesobj(wt->tree, 0), 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);

									obj_edit(wt, v, ED_INIT, nxtobj, 0, -1, NULL, true, &wind->wa, wind->rect_list.start, NULL, &nxtobj);
									DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON): Call wdlg->exit(%lx) with new editobj=%d for %s",
										(unsigned long)(wep->wdlg ? wep->wdlg->exit : NULL), aesobj_item(&nxtobj), client->name));
									if (wep->callout)
									{
										short no = aesobj_item(&nxtobj);

										ret = (*wep->callout)(client, wep->wdlg, ev, HNDL_EDCH, ev->mclicks, NULL, &no);
									}
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
							ret = wdialog_message(lock, client, wep);
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
						unhide_window(lock, wind, true);
					top_window(lock, true, true, wind);
					ret = 1;
					cont = 0;
				}
				DIAG((D_wdlg, NULL, "wdlg_event(MU_BUTTON) done! (ret=%d, cont=%d)", ret, cont));
			}

			if ( ret && cont && wind == top && (ev->mwhich & MU_KEYBD) )
			{
				unsigned short key = ev->key;
				unsigned short keystate;
				struct xa_aes_object new_focus = wt->focus;

				vq_key_s(C.P_handle, (short *)&keystate);

				obtree = wt->tree;

				if (valid_aesobj(&new_focus))
				{
					switch (aesobj_type(&new_focus) & 0xff)
					{
						case G_SLIST:
						{
							struct scroll_info *list = object_get_slist(aesobj_ob(&new_focus));
							if (list && list->keypress)
							{
								if ((*list->keypress)(list, key, keystate) == 0)
								{
									goto kbd_done;
								}
							}
							break;
						}
						default:;
					}
				}

				ei = wt->ei ? wt->ei : &wt->e;

				nxtobj = Form_Cursor(wt, v, ev->key, keystate, ei->o, true, &wind->rect_list.start, &new_focus, &key);

				if (valid_aesobj(&nxtobj))
				{
					DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): call HNDL_EDCH exit(%lx) with new edobj=%d for %s",
						(unsigned long)(wep->wdlg ? wep->wdlg->exit : NULL), nxtobj.item, client->name));

					if (!same_aesobj(&nxtobj, &ei->o))
					{
						short no = aesobj_item(&nxtobj);
						obj_edit(wt, v, ED_END, nxtobj, 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);
						obj_edit(wt, v, ED_INIT, nxtobj, 0, -1, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);
						if (wep->callout)
							ret = (*wep->callout)(client, wep->wdlg, ev, HNDL_EDCH, 0, NULL, &no);
						ei = wt->ei ? wt->ei : &wt->e;
					}
				}
				else if (key)
				{
					if (key == SC_SPACE && focus_set(wt) && !(focus_ob(wt)->ob_flags & OF_EDITABLE))
					{
						nxtobj = wt->focus;
					}
					else if (key == SC_RETURN || key == SC_NMPAD_ENTER)
					{
						nxtobj = ob_find_flst(obtree, OF_DEFAULT, 0, 0, OS_DISABLED, 0, 0);

						DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): Got RETURN key - default obj=%d for %s",
							aesobj_item(&nxtobj), client->name));
					}
					else if (key == SC_UNDO)
					{
						nxtobj = ob_find_cancel(obtree);
						DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): Got UNDO - cancel obj=%d for %s",
							aesobj_item(&nxtobj), client->name));
					}
					else
					{
						short ks = ev->kstate;
						short nk = normkey(ks, key);

						if ( (ks & (K_CTRL|K_ALT)) == K_ALT )
						{
							nxtobj = ob_find_shortcut(obtree, nk, 0);
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): shortcut %d for %s",
								aesobj_item(&nxtobj), client->name));
						}
					}
					if (valid_aesobj(&nxtobj))
					{
						if (!(events & MU_BUTTON))
						{
							check_mouse(client, &md.cstate, &md.x, &md.y);
							md.state = MBS_LEFT;
						}
						DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): doing Form_Button on obj=%d for %s",
							aesobj_item(&nxtobj), client->name));
						if ( !Form_Button(wt,
								  v,
								  nxtobj,
								  &md,
								  FBF_REDRAW|FBF_KEYBD,
								  &wind->rect_list.start,
								  NULL,
								  &nxtobj,
								  &dc))
						{
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): call exit(%lx) with exitobj=%d for %s",
								(unsigned long)(wep->wdlg ? wep->wdlg->exit : NULL), aesobj_item(&nxtobj), client->name));
							wep->obj = nxtobj;
							if (wep->callout)
								ret = (*wep->callout)(client, wep->wdlg, ev, aesobj_item(&nxtobj), 0, wep->wdlg->user_data, NULL);
							else
								ret = 0;
						}
					}
					else if (key != SC_RETURN && key != SC_NMPAD_ENTER)
					{
						/* new: call obj_edit if same obj also */
						/* if (!same_aesobj(&wt->focus, &ei->o)) */
						{
							DIAG((D_wdlg, NULL, "wdlg_event(MU_KEYBD): HNDL_EDIT exit(%lx) with key=%x for %s",
								(unsigned long)(wep->wdlg ? wep->wdlg->exit : NULL), key, client->name));
							ei = &wt->e;
							obj_edit(wt,
							 v,
							 ED_CHAR,
							 ei->o,
							 key,
							 0,
							 NULL,
							 true,
							 &wind->wa, wind->rect_list.start,
							 NULL,
							 NULL);
						}
						if (wep->callout)
						{
							ret = (*wep->callout)(client, wep->wdlg, ev, HNDL_EDIT, 0, NULL, &key);
						}

						ev->mwhich &= ~MU_KEYBD;
					}
				}
				if (valid_aesobj(&nxtobj))
					ev->mwhich &= ~MU_KEYBD;
kbd_done:
				if (valid_aesobj(&new_focus) && !same_aesobj(&wt->focus, &new_focus))
				{
					struct xa_aes_object pf = wt->focus;
					wt->focus = new_focus;
					if (valid_aesobj(&pf))
						obj_draw(wt, v, pf, -2, NULL, wind->rect_list.start, DRW_CURSOR);
					obj_draw(wt, v, new_focus, -2, NULL, wind->rect_list.start, DRW_CURSOR);
				}
			} /* end if ( wind == top && (ev->mwhich & MU_KEYBD) ) */
		} /* end if (ev) */
	} /* end if (wt) */
	return ret;
}

unsigned long
XA_wdlg_event(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	struct wdlg_info *wdlg;
	short handle;
	short ret = 1;

	CONTROL(0,1,0)

	pb->intout[0] = 1;

	if (*(const unsigned char *)pb->addrin != 0xae)
		return XAC_DONE;

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_event, handle=%lx",
		handle, pb->addrin[0]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind && (wdlg = wind->wdlg))
	{
		struct wdlg_evnt_parms wep;

		wep.wind	= wind;
		wep.wt		= get_widget(wind, XAW_TOOLBAR)->stuff.wt;
		wep.ev		= (EVNT *)pb->addrin[1];
		wep.wdlg	= wdlg;
		wep.callout	= callout_exit;
		wep.redraw	= wdlg_redraw;
		wep.obj		= inv_aesobj();

		ret = wdialog_event(lock, client, &wep);
	}
	DIAG((D_wdlg, NULL, "wdlg_event done, return %d", ret));

	pb->intout[0] = ret;

	return XAC_DONE;
}
/*
 * Ozk: Hmm ... should wdlg_redraw() be able to redraw pdlg_xx() windows too?
 *	According to the pdlg sample sources, yes, so we do that here.
 */
unsigned long
XA_wdlg_redraw(int lock, struct xa_client *client, AESPB *pb)
{
	struct xa_window *wind;
	short handle;

	CONTROL(2,0,2)

	pb->intout[0] = 1;

	if (*(const unsigned char *)pb->addrin != 0xae)
	{
#if WDIALOG_PDLG
		struct xa_pdlg_info *pdlg;
		pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
		wind = get_pdlg_wind(client, pdlg);
		if (wind)
		{
			if (pdlg->current_subdlg)
			{
				wdialog_redraw(lock, wind, aesobj(pdlg->current_subdlg->tree, pb->intin[0]), pb->intin[1], (GRECT *)&pdlg->current_subdlg->tree->ob_x);
			}
		}
#endif
		return XAC_DONE;
	}

	handle = (short)pb->addrin[0];

	DIAG((D_wdlg, client, "(%d)XA_wdlg_redraw, handle=%lx",
		handle, pb->addrin[0]));

	/* Get the window */
	wind = get_wind_by_handle(lock, handle);
	if (wind)
		wdlg_redraw(lock, wind, aesobj(NULL, pb->intin[0]), pb->intin[1], (GRECT *)pb->addrin[1]);

	return XAC_DONE;
}

static bool
click_wdlg_widget(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	struct xa_client *client = wind->owner;

	button_event(lock, client, md);
	return true;
}

#endif /* WDIALOG_WDLG */
