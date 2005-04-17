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

#include WIDGHNAME

#include "widgets.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "form.h"
#include "k_main.h"
#include "k_mouse.h"
#include "menuwidg.h"
#include "draw_obj.h"
#include "obtree.h"
#include "rectlist.h"

#include "xa_evnt.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_wdlg.h"

#include "mint/signal.h"


static struct widget_row * get_last_row(struct widget_row *row, short lmask, short lvalid, bool backwards);
//static struct widget_row * get_first_row(struct widget_row *row, short lmask, short lvalid, bool backwards);
static void rp_2_ap_row(struct xa_window *wind);

static OBJECT *def_widgets;

#if GENERATE_DIAGS
static char *t_widg[] =
{
	"XAW_TITLE",
	"XAW_CLOSE",
	"XAW_FULL",
	"XAW_MOVER",			/* Not actually used like the others */
	"XAW_INFO",
	"XAW_RESIZE",
	"XAW_UPLN",			/* 6 */
	"XAW_DNLN",			/* 7 */
	"XAW_VSLIDE",
	"XAW_UPPAGE",
	"XAW_DNPAGE",
	"XAW_LFLN",			/* 11 */
	"XAW_RTLN",			/* 12 */
	"XAW_HSLIDE",
	"XAW_LFPAGE",
	"XAW_RTPAGE",
	"XAW_ICONIFY",
	"XAW_HIDE",
	"XAW_TOOLBAR",			/* Extended XaAES widget */
	"XAW_BORDER",			/* Extended XaAES widget, used for border sizing. */
	"XAW_MENU"			/* Extended XaAES widget, must be drawn last. */
};
#endif

/*
 * WINDOW WIDGET SET HANDLING ROUTINES
 * This module handles the behaviour of the window widgets.
 */
inline long
pix_to_sl(long p, long s)
{
	/*
	 * Ozk:
	 *      Hmmm.... divide by Zero not handled correctly, at least
	 *	not on my Milan 040 where this crashed if 's' is 0
	 */
	if (s)
		return (p * SL_RANGE) / s;
	else
		return 0L;
}

inline long
sl_to_pix(long s, long p)
{
	return ((s * p) + (SL_RANGE >> 2)) / SL_RANGE;
}

int
XA_slider(struct xa_window *w, int which, long total, long visible, long start)
{
	int ret = 0;
	XA_WIDGET *widg = get_widget(w, which);

	if (w->active_widgets & widg->loc.mask)
	{
		XA_SLIDER_WIDGET *sl = widg->stuff;
		short old_pos = sl->position;
		short old_len = sl->length;


		if (visible < total)
			sl->length = pix_to_sl(visible, total);
		else
			sl->length = SL_RANGE;

		if (total - visible < 0 || start == 0)
			sl->position = sl->rpos = 0;
		else
		{
			if (start + visible >= total)
				sl->position = sl->rpos = SL_RANGE;
			else
				sl->position = sl->rpos = pix_to_sl(start, total - visible);
		}
		if (sl->position != old_pos || sl->length != old_len)
			sl->flags |= SLIDER_UPDATE;

		ret = (sl->flags & SLIDER_UPDATE) ? 1 : 0; //(sl->position != old_pos || sl->length != old_len);
	}
	return ret;
}

static inline void
fix_widg(OBJECT *widg)
{
	widg->ob_x = 0;
	widg->ob_y = 0;
}

/* Standard Widget Set from widget resource file */
void
fix_default_widgets(void *rsc)
{
	int i;	

	def_widgets = ResourceTree(rsc, WIDGETS);
	DIAGS(("widget set at %lx", def_widgets));

	for (i = 1; i < WIDG_CFG; i++)
		fix_widg(def_widgets + i);
}

OBJECT *
get_widgets(void)
{
	return def_widgets;
}

bool
m_inside(short x, short y, RECT *o)
{
	return (   x >= o->x
		&& y >= o->y
		&& x <  o->x+o->w
		&& y <  o->y+o->h);
}


/*
 * Set the active/pending widget behaviour for a client
 */
static void
set_widget_active(struct xa_window *wind, XA_WIDGET *widg, WidgetBehaviour *wc, int i)
{
	widget_active.wind = wind;
	widget_active.widg = widg;
	widget_active.action = wc;
}

/*
 * Cancel the active/pending widget for a client
 */
static void
cancel_widget_active(struct xa_window *wind, int i)
{
	widget_active.widg = NULL;
	widget_active.cont = false;

	/* Restore the mouse now we've finished the action */
	graf_mouse(wind->owner->mouse, wind->owner->mouse_form, wind->owner, false);
}

void
remove_widget_active(struct xa_client *client)
{
	if (widget_active.widg && widget_active.wind->owner == client)
	{
		widget_active.widg = NULL;
		widget_active.cont = false;
		C.do_widget_repeat_client = NULL;
		aessys_timeout = 0;
	}
}


#ifndef __GNUC__
/* check slider value */
int
bound_sl(int p)
{
	return p < 0 ? 0 : (p > SL_RANGE ? SL_RANGE : p);
}
#endif

static OBJECT *
rp2ap_obtree(struct xa_window *wind, struct xa_widget *widg, RECT *r)
{
	XA_TREE *wt;
	OBJECT *obtree = NULL;

	if (widg->type == XAW_TOOLBAR || widg->type == XAW_MENU)
	{
		wt = widg->stuff;
		obtree = wt->tree;

		if (wind != root_window)
		{
			obtree->ob_x = r->x;
			obtree->ob_y = r->y;
			if (!wt->zen)
			{
				obtree->ob_x += wt->ox;
				obtree->ob_y += wt->oy;
			}
		}
	}
	/* If want to get informed */
	return obtree;
}


/*
 * Convert window relative widget coords to absolute screen coords
 */
void *
rp_2_ap(struct xa_window *wind, XA_WIDGET *widg, RECT *r)
{
	RECT dr;
	short rtx, rty, ww, wh;
	int frame = wind->frame;

	DIAG((D_form, NULL, "rp_2_ap: type=%s, widg=%lx, wt=%lx, obtree=%lx",
		t_widg[widg->type], widg, widg->stuff,
		widg->type == XAW_TOOLBAR ? (long)((XA_TREE *)widg->stuff)->tree : -1 ));

	if (widg->type != XAW_TOOLBAR )
	{
		if (r && (long)r != (long)&widg->ar)
			*r = widg->ar;
		return rp2ap_obtree(wind, widg, &widg->ar);
	}

	if (frame < 0)
		frame = 0;

	if (r == NULL)
		r = &dr;

	/* HR: window frame size dynamic
	 *     thanks to the good design these 2 additions are all there is :-)
	 */
	rtx = widg->r.x + frame;
	rty = widg->r.y + frame;

	r->w = widg->r.w;
	r->h = widg->r.h;
	r->x = wind->r.x;
	r->y = wind->r.y;

	ww = wind->frame < 0 ? wind->r.w : wind->r.w - SHADOW_OFFSET;
	wh = wind->frame < 0 ? wind->r.h : wind->r.h - SHADOW_OFFSET;

	switch ((widg->loc.relative_type & (R_BOTTOM|R_RIGHT|R_CENTER)))
	{
	case LT:
		r->x += rtx;
		r->y += rty;
		break;
	case LB:
		r->x += rtx;
		r->y += (wh - rty); //(wh - r->h - rty);
		break;
	case RT:
		r->x += (ww - rtx); //(ww - r->w - rtx);
		r->y += rty;
		break;
	case RB:
		r->x += (ww - rtx); //(ww - r->w - rtx);
		r->y += (wh - rty); //(wh - r->h - rty);
		break;
	case CT:
		r->x += (ww - r->w) / 2;
		r->y += rty;
		break;
	case CR:
		r->x += (ww - r->w - rtx);
		r->y += (wh - r->h) / 2;
	case CB:
		r->x += (ww - r->w) / 2;
		r->y += (wh - r->h - rty);
		break;
	case CL:
		r->x += rtx;
		r->y += (wh - r->h - rty);
		break;
	default:;
	}

	return rp2ap_obtree(wind, widg, r);
#if 0	
	/* HR: for after moving: set form's x & y, otherwise find_object
	 *     fails.
	 */
	if (widg->type == XAW_TOOLBAR || widg->type == XAW_MENU)
	{
		XA_TREE *wt;
		OBJECT *form;

		wt = widg->stuff;
		form = wt->tree;

		if (wind != root_window)
		{
			form->ob_x = r->x;
			form->ob_y = r->y;
			if (!wt->zen)
			{
				form->ob_x += wt->ox;
				form->ob_y += wt->oy;
			}
		}

		/* If want to get informed */
		return form;
	}
	return NULL;
#endif
}

/* Ozk:
 * Context safe rp_2_ap - does not return an object tree
 * Used by AESSYS, checkif_do_widgets(), when determining
 * which widget is under the mouse.
 */
void
rp_2_ap_cs(struct xa_window *wind, XA_WIDGET *widg, RECT *r)
{
	short rtx, rty, ww, wh;
	int frame = wind->frame;

	DIAG((D_form, NULL, "rp_2_ap: type=%s, widg=%lx, wt=%lx, obtree=%lx",
		t_widg[widg->type], widg, widg->stuff,
		widg->type == XAW_TOOLBAR ? (long)((XA_TREE *)widg->stuff)->tree : -1 ));

	if (widg->type != XAW_TOOLBAR)
	{
		if (r && (long)r != (long)&widg->ar)
			*r = widg->ar;
		return;
	}

	if (frame < 0)
		frame = 0;

	if (r == NULL)
		r = &widg->ar;

	/* HR: window frame size dynamic
	 *     thanks to the good design these 2 additions are all there is :-)
	 */
	rtx = widg->r.x + frame;
	rty = widg->r.y + frame;

	r->w = widg->r.w;
	r->h = widg->r.h;
	r->x = wind->r.x;
	r->y = wind->r.y;

	ww = wind->frame < 0 ? wind->r.w : wind->r.w - SHADOW_OFFSET;
	wh = wind->frame < 0 ? wind->r.h : wind->r.h - SHADOW_OFFSET;

	switch ((widg->loc.relative_type & (R_BOTTOM|R_RIGHT|R_CENTER)))
	{
	case LT:
		r->x += rtx;
		r->y += rty;
		break;
	case LB:
		r->x += rtx;
		r->y += (wh - rty); //(wh - r->h - rty);
		break;
	case RT:
		r->x += (ww - rtx); //(ww - r->w - rtx);
		r->y += rty;
		break;
	case RB:
		r->x += (ww - rtx); //(ww - r->w - rtx);
		r->y += (wh - rty); //(wh - r->h - rty);
		break;
	case CT:
		r->x += (ww - r->w) / 2;
		r->y += rty;
		break;
	case CR:
		r->x += (ww - r->w - rtx);
		r->y += (wh - r->h) / 2;
	case CB:
		r->x += (ww - r->w) / 2;
		r->y += (wh - r->h - rty);
		break;
	case CL:
		r->x += rtx;
		r->y += (wh - r->h - rty);
		break;
	default:;
	}
}

XA_TREE *
obtree_to_wt(struct xa_client *client, OBJECT *obtree)
{
	XA_TREE *wt = NULL;

	DIAGS(("obtree_to_wt: look for wt with obtree=%lx for %s",
		obtree, client->name));

	if (client->fmd.wt && obtree == client->fmd.wt->tree)
	{
		DIAGS((" -- found in client->wt"));
		wt = client->fmd.wt;
	}

	if (!wt && client->fmd.wind)
	{
		XA_WIDGET *widg = get_widget(client->fmd.wind, XAW_TOOLBAR);

		if (obtree == ((XA_TREE *)widg->stuff)->tree)
		{
			DIAGS((" -- found in XAW_TOOLBAR fmd wind %d - %s",
				client->fmd.wind->handle, client->name));
			wt = widg->stuff;
		}
	}

	if (!wt && client->alert)
	{
		XA_WIDGET *widg = get_widget(client->alert, XAW_TOOLBAR);

		if (obtree == ((XA_TREE *)widg->stuff)->tree)
		{
			DIAGS((" -- found in XAW_TOOLBAR fmd wind %d - %s",
				client->alert->handle, client->name));
			wt = widg->stuff;
		}
	}

	if (!wt)
	{
		wt = client->wtlist;
		DIAGS((" -- lookup in wtlist for %s", client->name));
		while (wt)
		{
			if (obtree == wt->tree)
				break;
			wt = wt->next;
		}
#if GENERATE_DIAGS
		if (!wt)
			DIAGS((" -- lookup failed"));
#endif
	}
	DIAGS((" obtree_to_wt: return %lx for %s",
		wt, client->name));

	return wt;
}

XA_TREE *
new_widget_tree(struct xa_client *client, OBJECT *obtree)
{
	XA_TREE *new;

	DIAGS((" === Create new widget tree - obtree=%lx, for %s",
		obtree, client->name));

	new = kmalloc(sizeof(*new));

	if (new)
	{
		short sx, sy;
		RECT r;

		bzero(new, sizeof(*new));
		new->flags |= WTF_ALLOC;
		new->tree = obtree;
		new->owner = client;
		new->e.obj = -1;

		sx = obtree->ob_x;
		sy = obtree->ob_y;
		obtree->ob_x = 100;
		obtree->ob_y = 100;
		
		ob_area(obtree, 0, &r);
		new->ox = obtree->ob_x - r.x;
		new->oy = obtree->ob_y - r.x;
		
		if (new->ox < 0)
			new->ox = 0;
		if (new->oy < 0)
			new->oy = 0;

		obtree->ob_x = sx;
		obtree->ob_y = sy;

		new->next = client->wtlist;
		client->wtlist = new;

		if ((obtree[3].ob_type & 0xff) == G_TITLE)
			new->is_menu = new->menu_line = true;

	}
	DIAGS((" return new wt=%lx", new));
	return new;
}

/* Ozk:
 * This is the last resort, when all else fails trying to
 * obtain a widget tree for a obtree. Uses the widget tree
 * embedded in the client structure.
 */
XA_TREE *
set_client_wt(struct xa_client *client, OBJECT *obtree)
{
#if 0
	XA_TREE *wt = &client->wt;

	DIAGS(("set_client_wt: obtree %lx for %s",
		obtree, client->name));
	if (wt->tree != obtree)
	{
		bzero(wt, sizeof(*wt));
		wt->tree = obtree;
		wt->owner = client;
		wt->e.obj = -1;
		wt->e.c_state = 0;
		wt->c = *(RECT *)&obtree->ob_x;
	}
	return wt;
#endif
	return NULL;
}

/* Ozk:
 * This will also free statically declared widget_tree's
 */
void
free_wtlist(struct xa_client *client)
{
	XA_TREE *wt;

	DIAGS(("free_wtlist for %s", client->name));
	while (client->wtlist)
	{
		wt = client->wtlist;
		client->wtlist = wt->next;
		wt->flags &= ~WTF_STATIC;
		free_wt(wt);
	}
}
/* Unhook a widget_tree from its owners wtlist
 */
void
remove_from_wtlist(XA_TREE *wt)
{
	XA_TREE **nwt;

	if (wt->flags & WTF_STATIC)
	{
		DIAGS(("remove_from_wtlist: wt=%lx declared as static - not removed",
			wt));
		return;
	}

	nwt = &wt->owner->wtlist;

	while (*nwt)
	{
		if (*nwt == wt)
		{
			*nwt = wt->next;
			DIAGS(("remove_from_wtlist: removed wt=%lx from %s list",
				wt, wt->owner->name));
			break;
		}
		nwt = &(*nwt)->next;
	}
	wt->next = NULL;
	DIAGS(("remove_from_wtlist: wt=%lx not in %s wtlist",
		wt, wt->owner->name));
}

/* Ozk:
 * Not all things should be duplicated in a widget_tree
 */
void
copy_wt(XA_TREE *d, XA_TREE *s)
{
	ulong f = d->flags;
	XA_TREE *n = d->next;

	*d = *s;

	d->flags = f;
	d->next = n;
}
	
void
free_wt(XA_TREE *wt)
{

	DIAGS(("free_wt: wt=%lx", wt));
#if 1
	if (wt->links)
	{
		display("free_wt: links not NULL!!!!!");
		display("free_wt: wt=%lx, links=%d, flags=%lx, owner=%s", wt, wt->links, wt->flags, wt->owner->name);
	}
#endif
	if (wt->flags & WTF_STATIC)
	{
		DIAGS(("free_wt: Declared as static!"));
		return;
	}

	if (wt->extra && (wt->flags & WTF_XTRA_ALLOC))
	{
		DIAGS(("  --- freed extra %lx", wt->extra));
		kfree(wt->extra);
	}
#if GENERATE_DIAGS
	else
		DIAGS(("  --- extra not alloced"));
#endif
	if (wt->tree && (wt->flags & WTF_TREE_ALLOC))
	{
		if (wt->flags & WTF_TREE_CALLOC)
		{
			DIAGS(("  --- kfreed obtree %lx", wt->tree));
		
			free_object_tree(wt->owner, wt->tree);
		}
		else
		{
			DIAGS(("  --- ufreed obtree %lx", wt->tree));
		
			free_object_tree(C.Aes, wt->tree);
		}
		wt->tree = NULL;
	}
#if GENERATE_DIAGS
	else
		DIAGS(("  --- obtree not alloced"));
#endif
	if (wt->lbox)
	{
		kfree(wt->lbox);
		wt->lbox = NULL;
	}
	if (wt->flags & WTF_ALLOC)
	{
		DIAGS(("  --- freed wt=%lx", wt));
		kfree(wt);
	}
	else
	{
		struct xa_client *client = wt->owner;
		DIAGS(("  --- wt=%lx not alloced", wt));
		bzero(wt, sizeof(*wt));
		wt->owner = client;
	}
}

bool
remove_wt(XA_TREE *wt, bool force)
{
	if (force || (wt->flags & (WTF_STATIC|WTF_AUTOFREE)) == WTF_AUTOFREE)
	{
		if (force || wt->links == 0)
		{
			//display("remove_wt: removing %lx (links=%d, force = %d)", wt, wt->links, force);
			remove_from_wtlist(wt);
			free_wt(wt);
			return true;
		}
		//else
		//	display("remove_wt: links = %d", wt->links);
	}
	//else
	//	display("remove_wt: flags =%lx, links = %d", wt->flags, wt->links);

	return false;
}

/*
 * If rl is not NULL, we use that rectangle list instead of the rectlist
 * of the window containing the widget. Used when redrawing widgets of 
 * SLIST windows, for example, that are considered to be objects along
 * the lines of AES Object trees,
 */
void
display_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, struct xa_rect_list *rl)
{
	RECT r;

	if (!(wind->window_status & widg->loc.statusmask) && widg->display)
	{
		DIAG((D_wind, wind->owner, "draw_window %d: display widget %d (func=%lx)",
			wind->handle, widg->type, widg->display));

		if (!rl)
			rl = wind->rect_start;

		hidem();
		while (rl)
		{	
			if (widg->loc.properties & WIP_WACLIP)
			{
				if (xa_rect_clip(&rl->r, &wind->wa, &r))
				{
					set_clip(&r);
					widg->display(lock, wind, widg);
				}
			}
			else
			{
				set_clip(&rl->r);
				widg->display(lock, wind, widg);
			}
			rl = rl->next;
		}
		clear_clip();
		showm();
	}
#if 0	
	/* if removed or lost */
	if (!(wind->window_status & widg->loc.statusmask) && widg->display)
	{
		/* Some programs do this! */
		if (!((wind->window_status & XAWS_ICONIFIED)
		      && widg->type != XAW_TITLE
		      && widg->type != XAW_ICONIFY))
		{
			struct xa_rect_list *rl;

			hidem();
			rl = rect_get_system_first(wind);
			while (rl)
			{
				/* Walk the rectangle list */
				set_clip(&rl->r);
				widg->display(lock|winlist, wind, widg);
				rl = rect_get_system_next(wind);
			}
			clear_clip();
			showm();
		}
	}
#endif
}

static void
CE_redraw_menu(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		struct xa_client *mc;
		struct xa_widget *widg = get_menu_widg();

		mc = ((XA_TREE *)widg->stuff)->owner;
		if (ce->client == mc)
		{
		#if 0
			DIAGS(("CE_redraw_menu: for %s", ce->client->name));
			hidem();
			widg->display(lock|winlist, root_window, widg);
			showm();
		#endif
			display_widget(lock, root_window, widg, NULL);
		}
		else
		{
			DIAGS(("CE_redraw_menu: ownership change, forwarding..."));
			post_cevent(mc,
				    CE_redraw_menu,
				    NULL,
				    NULL,
				    0, 0,
				    NULL,
				    NULL);
			Unblock(mc, 1, 1);
		}
	}
}
/*
 * Ozk: redraw menu need to check the owner of the menu object tree
 * and draw it in the right context. 
 */
void
redraw_menu(enum locks lock)
{
	struct xa_client *rc, *mc;
	struct xa_widget *widg;

	rc = lookup_extension(NULL, XAAES_MAGIC);
	if (!rc)
		rc = C.Aes;

	DIAGS(("redraw_menu: rc = %lx, %s", rc, rc->name));

	widg = get_menu_widg();
	DIAGS(("redaw_menu: widg = %lx", widg));
	mc = ((XA_TREE *)widg->stuff)->owner;
	DIAGS(("redaw_menu: widg owner = %s", mc->name));

	if (mc == rc || mc == C.Aes)
	{
		if (C.update_lock && C.update_lock != rc->p)
			return;
	#if 0
		DIAGS(("Display MENU (same client) for %s", rc->name));
		hidem();
		widg->display(lock|winlist, root_window, widg);
		showm();
	#endif
		display_widget(lock, root_window, widg, NULL);
	}
	else
	{
		DIAGS(("Display MENU: post cevnt (%lx) to owner %s by %s", CE_redraw_menu, mc->name, rc->name));
		post_cevent(mc,
			    CE_redraw_menu,
			    NULL,
			    NULL,
			    0, 0,
			    NULL,
			    NULL);
	}
	DIAGS(("Display MENU - exit OK"));
}


/*
 * Calculate the size of the work area for a window and store it
 * - This is needed because the locations of widgets are relative and
 *   can be modified.
 * Actually, this updates all the dynamic sized elements from the standard widget set...
 * (namely: work area, sliders and title bar)
 */

#define WASP 1	/* space between workarea and other widgets */

void
calc_work_area(struct xa_window *wind)
{
	struct widget_row *rows = wind->widg_rows;

	RECT r = wind->rc;
	//XA_WIND_ATTR k = wind->active_widgets;
	int thin = wind->thinwork;
	int frame = wind->frame;
	int t_margin, b_margin, l_margin, r_margin;
	short wa_borders = 0;
	wind->wa = r;

	{
		/* a colour work area frame is larger to allow for the
		 * fancy borders :-) unless thinwork has been specified
		 * a color work area frame consists of a grey, a light and
		 * a dark line.
		 */

		/* in color aligning is on the top & left light line of tha wa */
		t_margin = (MONO || thin) ? 1 : 2;
		b_margin = (MONO || thin) ? 1 : 2;
		l_margin = (MONO || thin) ? 1 : 2;
		r_margin = (MONO || thin) ? 1 : 2;
		
		/* This is the largest work area possible */
		if (frame >= 0)
		{
			wind->wa.x += frame;
			wind->wa.y += frame;
			wind->wa.w -= frame << 1;
			wind->wa.h -= frame << 1;
			
			wind->wa.w -= SHADOW_OFFSET;
			wind->wa.h -= SHADOW_OFFSET;

		//	if (frame < 4)
		//		frame = 4;
		}
		if (frame < 0)
			frame = 0;

		wind->ba.x = wind->r.x + frame;
		wind->ba.y = wind->r.y + frame;
		frame <<= 1;
		wind->ba.w = wind->r.w - frame;
		wind->ba.h = wind->r.h - frame;

		rp_2_ap_row(wind);

		if ((rows = get_last_row(wind->widg_rows, (XAR_VERT | XAR_PM), XAR_START, false)))
		{
			wind->wa.y += (rows->r.y + rows->r.h);
			wind->wa.h -= (rows->r.y + rows->r.h);
			
		}
		else if (wind->frame >= 0 && wind->thinwork && wind->wa_frame)
		{
			wind->wa.y += t_margin;
			wind->wa.h -= t_margin;
			wa_borders |= WAB_TOP;
		}

		if ((rows = get_last_row(wind->widg_rows, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_START), false)))
		{
			wind->wa.x += (rows->r.x + rows->r.w);
			wind->wa.w -= (rows->r.x + rows->r.w);
		}
		else if (wind->frame >= 0 && wind->thinwork && wind->wa_frame)
		{
			wind->wa.x += l_margin;
			wind->wa.w -= l_margin;
			wa_borders |= WAB_LEFT;
		}

		if ((rows = get_last_row(wind->widg_rows, (XAR_VERT | XAR_PM), XAR_END, false)))
			wind->wa.h -= rows->r.y;
		else if (wind->frame >= 0 && wind->thinwork && wind->wa_frame)
			wind->wa.h -= b_margin, wa_borders |= WAB_BOTTOM;

		if ((rows = get_last_row(wind->widg_rows, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_END), false)))
			wind->wa.w -= rows->r.x;
		else if (wind->frame >= 0 && wind->thinwork && wind->wa_frame)
			wind->wa.w -= r_margin, wa_borders |= WAB_RIGHT;

		if (wind->frame >= 0 && !wind->thinwork)
		{
			wind->wa.x += 2, wind->wa.y += 2;
			wind->wa.w -= 4, wind->wa.h -= 4;
		}
		wind->wa_borders = wa_borders;
	}

	/* border displacement */
	wind->bd.x = wind->rc.x - wind->wa.x;
	wind->bd.y = wind->rc.y - wind->wa.y;
	wind->bd.w = wind->rc.w - wind->wa.w;
	wind->bd.h = wind->rc.h - wind->wa.h;

	/* Add bd to toolbar->r to get window rectangle for create_window
	 * Anyhow, always convenient, especially when snapping the workarea.
	 */

	DIAG((D_widg, wind->owner, "workarea %d: %d/%d,%d/%d", wind->handle, wind->wa));
}

/*
 * Ozk: Any resources taken by a widget should be released here
 */
void
free_xawidget_resources(struct xa_widget *widg)
{
	DIAGS(("free_xawidget_resources: widg=%lx", widg));
	if (widg->stuff)
	{
		switch (widg->stufftype)
		{
			case STUFF_IS_WT:
			{					
				XA_TREE *wt = widg->stuff;
				DIAGS(("  --- stuff is wt=%lx in widg=%lx",
					wt, widg));

				wt->links--;
			//	display(" free_xawidget_re: stuff is wt=%lx (links=%d) in widg=%lx",
			//		wt, wt->links, widg);
				
				if (!remove_wt(wt, false))
				{
					wt->widg = NULL;
					wt->wind = NULL;
				}
				break;
			}
			default:
			{
				if (widg->flags & XAWF_STUFFKMALLOC)
				{
					DIAGS(("  --- release stuff=%lx in widg=%lx",
						widg->stuff, widg));
					kfree(widg->stuff);
				}
			}
		}
		widg->flags    &= ~XAWF_STUFFKMALLOC;
	}
#if GENERATE_DIAGS
	else
		DIAGS(("  --- stuff=%lx not alloced in widg=%lx",
			widg->stuff, widg));
#endif
	widg->stufftype	= 0;
	widg->stuff   = NULL;
 	widg->display = NULL;
	widg->click   = NULL;
	widg->dclick  = NULL;
	widg->drag    = NULL;
}

/*
 * Define the widget locations using window relative coordinates.
 */

/* needed a dynamic margin (for windowed list boxes) */
/* eliminated both margin and shadow sizes from this table */
/* put some extra data there as well. */
static XA_WIDGET_LOCATION
/*             defaults              index        mask	     status				rsc	      properties  destruct */
/*							      mask									 */
stdl_close   = {LT, { 0, 0, 1, 1 },  XAW_CLOSE,   CLOSER,    XAWS_ICONIFIED,			WIDG_CLOSER,  WIP_ACTIVE, free_xawidget_resources },
stdl_full    = {RT, { 0, 0, 1, 1 },  XAW_FULL,    FULLER,    XAWS_ICONIFIED,			WIDG_FULL,    WIP_ACTIVE, free_xawidget_resources },
stdl_iconify = {RT, { 0, 0, 1, 1 },  XAW_ICONIFY, ICONIFIER, 0,					WIDG_ICONIFY, WIP_ACTIVE, free_xawidget_resources },
stdl_hide    = {RT, { 0, 0, 1, 1 },  XAW_HIDE,    HIDER,     XAWS_ICONIFIED,			WIDG_HIDE,    WIP_ACTIVE, free_xawidget_resources },
stdl_title   = {LT | R_VARIABLE, { 0, 0, 1, 1 },  XAW_TITLE,   NAME,      0,					0,            WIP_ACTIVE, free_xawidget_resources },
stdl_notitle = {LT | R_VARIABLE, { 0, 0, 1, 1 },  XAW_TITLE,   NAME,      0,					0,                     0, free_xawidget_resources },
//stdl_resize  = {RB, { 0, 0, 1, 1 },  XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,	WIDG_SIZE,    WIP_ACTIVE, free_xawidget_resources },
stdl_resize  = {RB, { 0, 0, 1, 1 },  XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,	WIDG_SIZE,    WIP_ACTIVE, free_xawidget_resources },
stdl_nresize = {RT/*RB*/, { 0, 0, 1, 1 },  XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,	WIDG_SIZE,             0, free_xawidget_resources },
stdl_uscroll = {LT/*RT*/, { 0, 0, 1, 1 },  XAW_UPLN,    UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_UP,      WIP_ACTIVE, free_xawidget_resources },
stdl_upage   = {NO, { 0, 1, 1, 1 },  XAW_UPPAGE,  UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_vslide  = {LT | R_VARIABLE/*RT*/, { 0, 1, 1, 1 },  XAW_VSLIDE,  VSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_nvslide = {LT | R_VARIABLE/*RT*/, { 0, 1, 1, 1 },  XAW_VSLIDE,  VSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,	0,                     0, free_xawidget_resources },
stdl_dpage   = {NO, { 0, 1, 1, 1 },  XAW_DNPAGE,  DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_dscroll = {RT/*RB*/, { 0, 1, 1, 1 },  XAW_DNLN,    DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_DOWN,    WIP_ACTIVE, free_xawidget_resources },
stdl_lscroll = {LB, { 0, 0, 1, 1 },  XAW_LFLN,    LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_LEFT,    WIP_ACTIVE, free_xawidget_resources },
stdl_lpage   = {NO, { 1, 0, 1, 1 },  XAW_LFPAGE,  LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_hslide  = {LB | R_VARIABLE, { 1, 0, 1, 1 },  XAW_HSLIDE,  HSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
//stdl_nhslide = {LB | R_VARIABLE, { 1, 0, 1, 1 },  XAW_HSLIDE,  HSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,	0,                     0, free_xawidget_resources },
stdl_rpage   = {NO, { 1, 0, 1, 1 },  XAW_RTPAGE,  RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_rscroll = {RB, { 1, 0, 1, 1 },  XAW_RTLN,    RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_RIGHT,   WIP_ACTIVE, free_xawidget_resources },
stdl_info    = {LT | R_VARIABLE, { 0, 0, 1, 1 },  XAW_INFO,    INFO,      XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_menu    = {LT | R_VARIABLE, { 0, 0, 0, 0 },  XAW_MENU,    XaMENU,    XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_pop     = {LT, { 0, 0, 0, 0 },  XAW_MENU,    XaPOP,     XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources },
stdl_border  = {0,  { 0, 0, 0, 0 },  XAW_BORDER,  0,         XAWS_SHADED|XAWS_ICONIFIED,	0,            WIP_ACTIVE, free_xawidget_resources }
;


static struct widget_row *
get_widget_row(struct xa_window *wind, struct xa_widget *widg)
{
	struct widget_row *row = wind->widg_rows;

	while (row)
	{
		if (row->tp_mask & widg->loc.mask)
			break;
		row = row->next;
	}
	return row;
}

static struct widget_row *
get_prev_row(struct widget_row *row, short lmask, short lvalid)
{
	struct widget_row *r = row->prev;

	while (r)
	{
		if (r->start && (r->rel & lmask) == lvalid)
			break;
		r = r->prev;
	}

	return r;
}
static struct widget_row *
get_next_row(struct widget_row *row, short lmask, short lvalid)
{
	struct widget_row *r = row->next;

	while (r)
	{
		if (r->start && (r->rel & lmask) == lvalid)
			break;
		r = r->next;
	}

	return r;
}

static struct widget_row *
get_last_row(struct widget_row *row, short lmask, short lvalid, bool backwards)
{
	struct widget_row *last = NULL;

	if (row->start && (row->rel & lmask) == lvalid)
		last = row;

	if (backwards)
	{
		while ((row = get_prev_row(row, lmask, lvalid)))
			last = row;
	}
	else
	{
		while ((row = get_next_row(row, lmask, lvalid)))
			last = row;
	}
	return last;
}

/* Dont deleteme yet!! */
#if 0
static struct widget_row *
get_first_row(struct widget_row *row, short lmask, short lvalid, bool backwards)
{
	if (!(row->start && (row->rel & lmask) == lvalid))
	{
		if (backwards)
			row = get_prev_row(row, lmask, lvalid);
		else
			row = get_next_row(row, lmask, lvalid);
	}
	return row;
}
#endif
	
static void
reloc_widget_row(struct widget_row *row)
{
	struct xa_widget *widg = row->start;
	short lxy, rxy;

	if (row->rel & XAR_VERT)
	{
		lxy = rxy = 0;
		while (widg)
		{
			if (widg->loc.relative_type & R_BOTTOM)
			{
				widg->r.y = row->rxy + widg->r.h + rxy;
				rxy += widg->r.h;
			}
			else
			{
				widg->r.y = row->r.y + lxy;
				lxy += widg->r.h;
			}
			widg->r.x = row->r.x;
			widg->r.w = row->r.w;
			widg = widg->next;
		}
	}
	else
	{
		lxy = rxy = 0;
		while (widg)
		{
			if (widg->loc.relative_type & R_RIGHT)
			{
				widg->r.x = row->rxy + widg->r.w + rxy;
				rxy += widg->r.w;
			}
			else
			{
				widg->r.x = row->r.x + lxy;
				lxy += widg->r.w;
			}
			widg->r.y = row->r.y;
			widg->r.h = row->r.h;
			widg = widg->next;
		}
	}
}

static void
rp_2_ap_row(struct xa_window *wind)
{
	struct widget_row *row = wind->widg_rows;
	struct xa_widget *widg;
	RECT r;
	short v, x2, y2, l;

	r = wind->r;

	if (wind->frame >= 0)
	{
		short frame = wind->frame;

		r.x += frame;
		r.y += frame;
		frame <<= 1;
		r.w -= (frame + SHADOW_OFFSET);
		r.h -= (frame + SHADOW_OFFSET);
	}
	
	x2 = r.x + r.w;
	y2 = r.y + r.h;

	while (row)
	{
		v = 0;
		if ((widg = row->start))
		{
			if (!(row->rel & XAR_VERT))
			{
				l = x2 - row->rxy;
				while (widg)
				{
					if (widg->loc.relative_type & R_VARIABLE)
						v++;

					switch ((widg->loc.relative_type & (R_RIGHT|R_BOTTOM|R_CENTER)))
					{
						case 0:
						{
							widg->ar.x = r.x + widg->r.x;
							widg->ar.y = r.y + widg->r.y;
							break;
						}
						case R_RIGHT:
						{
							widg->ar.x = x2 - widg->r.x;
							widg->ar.y = r.y + widg->r.y;
							if (l > widg->ar.x)
								l = widg->ar.x;
							break;
						}
						case R_BOTTOM:
						{
							widg->ar.x = r.x + widg->r.x;
							widg->ar.y = y2 - widg->r.y;
							break;
						}
						case (R_BOTTOM|R_RIGHT):
						{
							widg->ar.x = x2 - widg->r.x;
							widg->ar.y = y2 - widg->r.y;
							if (l > widg->ar.x)
								l = widg->ar.x;
							break;
						}
						default:;
					}
					widg->ar.w = widg->r.w;
					widg->ar.h = widg->r.h;
					widg = widg->next;
				}
				if (v)
				{
					widg = row->start;
					while (widg)
					{
						if (widg->loc.relative_type & R_VARIABLE)
						{
							widg->ar.w = widg->r.w = (l - widg->ar.x);
						}
						if (widg->type == XAW_MENU)
						{
							XA_TREE *wt = widg->stuff;
							if (wt && wt->tree)
							{
								wt->tree->ob_x = widg->ar.x;
								wt->tree->ob_y = widg->ar.y;
								if (!wt->zen)
								{
									wt->tree->ob_x += wt->ox;
									wt->tree->ob_y += wt->oy;
								}
							}
						}
						widg = widg->next;
					}
				}
				
			}
			else
			{
				l = y2 - row->rxy;
				while (widg)
				{
					if (widg->loc.relative_type & R_VARIABLE)
						v++;

					switch ((widg->loc.relative_type & (R_RIGHT|R_BOTTOM|R_CENTER)))
					{
						case 0:
						{
							widg->ar.x = r.x + widg->r.x;
							widg->ar.y = r.y + widg->r.y;
							break;
						}
						case R_RIGHT:
						{
							widg->ar.x = x2 - widg->r.x;
							widg->ar.y = r.y + widg->r.y;
							break;
						}
						case R_BOTTOM:
						{
							widg->ar.x = r.x + widg->r.x;
							widg->ar.y = y2 - widg->r.y;
							if (l > widg->ar.y)
								l = widg->ar.y;
							break;
						}
						case (R_BOTTOM|R_RIGHT):
						{
							widg->ar.x = x2 - widg->r.x;
							widg->ar.y = y2 - widg->r.y;
							if (l > widg->ar.y)
								l = widg->ar.y;
							break;
						}
						default:;
					}
					widg->ar.w = widg->r.w;
					widg->ar.h = widg->r.h;
					widg = widg->next;
				}
				if (v)
				{
					widg = row->start;
					while (widg)
					{
						if (widg->loc.relative_type & R_VARIABLE)
						{
							widg->ar.h = widg->r.h = (l - widg->ar.y);
						}
						if (widg->type == XAW_MENU)
						{
							XA_TREE *wt = widg->stuff;
							if (wt && wt->tree)
							{
								wt->tree->ob_x = widg->ar.x;
								wt->tree->ob_y = widg->ar.y;
								if (!wt->zen)
								{
									wt->tree->ob_x += wt->ox;
									wt->tree->ob_y += wt->oy;
								}
							}
						}
						widg = widg->next;
					}
				}
			}
		}
		row = row->next;
	}
}
		

static void
position_widget(struct xa_window *wind, struct xa_widget *widg, void(*widg_size)(struct xa_window *wind, struct xa_widget *widg))
{
	struct widget_row *rows = wind->widg_rows;

	if (widg->loc.relative_type & R_NONE)
		return;

	/*
	 * Find row in which this widget belongs, keeping track of Y coord
	 */
	rows = get_widget_row(wind, widg);

	if (rows)
	{
		struct xa_widget *nxt_w = rows->start;
		struct widget_row *nxt_r;
		short diff;
		short orient = rows->rel & XAR_VERT;
		short placement = rows->rel & XAR_PM;

		/*
		 * See if this widget is already in this row
		 */
		while (nxt_w && nxt_w->next && nxt_w != widg && !(nxt_w->loc.mask & widg->loc.mask))
			nxt_w = nxt_w->next;

		if (nxt_w && (nxt_w != widg))
		{
			/*
			 * Widget not in row, link it in
			 */
			nxt_w->next = widg;
			widg->next = NULL;
		}
		else if (!nxt_w)
		{
			/*
			 * This is the first widget in this row, and thus we also init its Y
			 */
			rows->start = widg;
			widg->next = NULL;
			rows->r.y = rows->r.x = rows->r.w = rows->r.h = rows->rxy = 0;

			if (!orient)	/* horizontal */
			{
				switch (placement)
				{
					case XAR_START:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.y = nxt_r->r.y + nxt_r->r.h;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_START))))
							rows->r.x = nxt_r->r.x + nxt_r->r.w;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_END))))
							rows->rxy = nxt_r->r.x;
						
						break;
					}
					case XAR_END:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_END))))
							rows->r.y = nxt_r->r.y;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_START))))
							rows->r.x = nxt_r->r.x + nxt_r->r.w;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT|XAR_END))))
							rows->rxy = nxt_r->r.x;
						break;
					}
					
					default:;
				}
			}
			else		/* vertical */
			{
				switch (placement)
				{
					case XAR_START:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_START))))
							rows->r.x = nxt_r->r.x + nxt_r->r.w;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.y = nxt_r->r.y + nxt_r->r.h;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), XAR_END)))
							rows->rxy = nxt_r->r.y;
						break;
					}
					case XAR_END:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_END))))
							rows->r.x = nxt_r->r.x;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.y = nxt_r->r.y + nxt_r->r.h;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), XAR_END)))
							rows->rxy = nxt_r->r.y;
						break;
					}
				}
			}
		}	
		/*
		 * If callback to calc w/h of widget provided...
		 */
		if (widg_size)
		{
			(*widg_size)(wind, widg);
		}
		/*
		 * ... else just use standard w/h
		 */
		else
		{
			widg->r.w = cfg.widg_w;
			widg->r.h = cfg.widg_h;
		}
		/*
		 * If Widget height is larger than largest one in this row,
		 * adjust all widgets height, and all following rows Y coord...
		 */
		if (!orient)
		{
			widg->loc.relative_type &= (R_VARIABLE | R_RIGHT);
			if (placement == XAR_END)
				widg->loc.relative_type |= R_BOTTOM;

			if ((diff = widg->r.h - rows->r.h) > 0)
			{
				rows->r.h = widg->r.h;
				
				switch (placement)
				{
					case XAR_START:
					{
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT|XAR_PM), XAR_START)))
						{
							nxt_r->r.y += diff;
							reloc_widget_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, XAR_VERT)))
						{
							nxt_r->r.y += diff;
							reloc_widget_row(nxt_r);
						}
						break;
					}
					case XAR_END:
					{
						rows->r.y += diff;
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), XAR_END)))
						{
							nxt_r->r.y += diff;
							reloc_widget_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, XAR_VERT)))
						{
							nxt_r->rxy += diff;
							reloc_widget_row(nxt_r);
						}	
						break;
					}
				}
			}
			else
				widg->r.h = rows->r.h;
		}
		else
		{
			XA_RELATIVE rt = widg->loc.relative_type & R_VARIABLE;

			if (widg->loc.relative_type & R_RIGHT)
				rt |= R_BOTTOM;
			if (placement == XAR_END)
				rt |= R_RIGHT;

			widg->loc.relative_type = rt;

			if ((diff = widg->r.w - rows->r.w) > 0)
			{
				rows->r.w = widg->r.w;
				switch (placement)
				{
					case XAR_START:
					{
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, 0)))
						{
							nxt_r->r.x += diff;
							reloc_widget_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_START))))
						{
							nxt_r->r.x += diff;
							reloc_widget_row(nxt_r);
						}
						break;
					}
					case XAR_END:
					{
						rows->r.x += diff;
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_END))))
						{
							nxt_r->r.x += diff;
							reloc_widget_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT), 0)))
						{
							nxt_r->rxy += diff;
							reloc_widget_row(nxt_r);
						}
						break;
					}
				}
			}
			else
				widg->r.w = rows->r.w;
		}
		reloc_widget_row(rows);
	}
}

static XA_WIDGET *
make_widget(struct xa_window *wind,
	    const XA_WIDGET_LOCATION *loc,
	    DisplayWidget *disp,
	    WidgetBehaviour *click,
	    WidgetBehaviour *drag,
	    struct xa_client *owner,
	    void (*widg_size)(struct xa_window *wind, struct xa_widget *widg))
{
	XA_WIDGET *widg = get_widget(wind, loc->n);

	widg->owner	= owner;
	widg->display	= disp;
	widg->click	= click;
	widg->drag	= drag;
	widg->loc	= *loc;
	widg->type	= loc->n;
	widg->destruct	= loc->destruct;

	position_widget(wind, widg, widg_size);

	return widg;
}

/* Establish iconified window position from a simple ordinal. */
RECT
iconify_grid(int i)
{
	RECT ic = {0,0,ICONIFIED_W,ICONIFIED_H};
	int j, w = screen.r.w/ic.w;

	ic.x = screen.r.x;
	ic.y = screen.r.y + screen.r.h - 1 - ic.h;

	j = i/w;
	i %= w;

	ic.x += i*ic.w;
	ic.y -= j*ic.h;

	return ic;
}
/*
 * Draw a window widget
 */
 
static void
draw_widg_box(short d, struct xa_wcol_inf *wcoli, short state, RECT *wr, RECT *anch)
{
	struct xa_wcol *wcol;
	struct xa_wtexture *wext;
	bool sel = state & OS_SELECTED;
	short f = wcoli->flags, o = 0;
	RECT r = *wr;

	wr_mode(wcoli->wrm);
	
	if (sel)
		wcol = &wcoli->s;
	else
		wcol = &wcoli->n;

	wext = wcol->texture;

	if (d)
	{
		r.x -= d;
		r.y -= d;
		r.w += d+d;
		r.h += d+d;
	}

	if (f & WCOL_BOXED)
	{
		int i = wcol->box_th;
		l_color(wcol->box_c);
		while (i > 0)
			gbox(o, &r), o--, i--;
	}
		
	if (f & WCOL_DRAW3D)
	{
		if (sel)
		{
			tl_hook(o, &r, wcol->tlc);
			br_hook(o, &r, wcol->brc);
		}
		else
		{
			br_hook(o, &r, wcol->brc);
			tl_hook(o, &r, wcol->tlc);
		}
		o -= 1;
	}

	if (f & WCOL_DRAWBKG)
	{
		f_interior(wcol->i);
		if (wcol->i > 1)
			f_style(wcol->f);
	
		f_color(wcol->c);
		gbar(o, &r);
	}
	
	if (wext)
	{
		short pnt[8];
		short x, y, w, h, sy, sh, dy, dh, width, height;
		MFDB mscreen, *msrc;

		mscreen.fd_addr = NULL;
		msrc = wext->mfdb;

		r.x -= o;
		r.y -= o;
		r.w += o + o;
		r.h += o + o;
		
		x = (long)(r.x - anch->x) % msrc->fd_w;
		w = msrc->fd_w - x;
		sy = (long)(r.y - anch->y) % msrc->fd_h;
		sh = msrc->fd_h - sy;
		dy = r.y;
		dh = r.h;
		
		width = r.w;
		while (width > 0)
		{
			pnt[0] = x;
			pnt[4] = r.x;
			
			width -= w;
			if (width <= 0)
				w += width;
			else
			{
				r.x += w;
				x = 0;
			}
			pnt[2] = pnt[0] + w - 1;
			pnt[6] = pnt[4] + w - 1;
			
			y = sy;
			h = sh;
			r.y = dy;
			r.h = dh;
			height = r.h;
			while (height > 0)
			{
				//pnt[0] = x;
				pnt[1] = y;
				//pnt[4] = r.x;
				pnt[5] = r.y;
				
				height -= h;
				if (height <= 0)
					h += height;
				else
				{
					r.y += h;
					y = 0;
				}
				
				//pnt[2] = x + w - 1;
				pnt[3] = pnt[1] + h - 1;
				
				//pnt[6] = r.x + w - 1;
				pnt[7] = pnt[5] + h - 1;

				vro_cpyfm(C.vh, S_ONLY, pnt, msrc, &mscreen);
				
				h = msrc->fd_h;	
			}
			w = msrc->fd_w;
		}
	}	
}

	
	
static void
draw_widget_text(struct xa_widget *widg, struct xa_wtxt_inf *wtxti, char *txt, short xoff, short yoff)
{
	wtxt_output(wtxti, txt, widg->state, &widg->ar, xoff, yoff);
}

static void
draw_widg_icon(struct xa_widget *widg, XA_TREE *wt, short ind)
{
	short x, y, w, h;
	OBJECT *ob = wt->tree + ind;

	if (widg->state & OS_SELECTED)
		ob->ob_state |= OS_SELECTED;
	else
		ob->ob_state &= ~OS_SELECTED;

	x = widg->ar.x, y = widg->ar.y;

	object_spec_wh(ob, &w, &h);
	x += (widg->ar.w - w) >> 1;
	y += (widg->ar.h - h) >> 1;
	display_object(0, wt, (const RECT *)&C.global_clip, ind, x, y, 0);
}

void
draw_window_borders(struct xa_window *wind)
{
	struct xa_wcol_inf *wci = &wind->colours->borders;
	short size = wind->frame;
	RECT r;

	/* top-left box */
	r.x = wind->r.x;
	r.y = wind->r.y;
	r.w = size;
	r.h = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* Left border */
	r.y += size;
	r.h = wind->r.h - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* bottom-left box */
	r.y = wind->r.y + wind->r.h - (size + 2);
	r.h = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* Bottom border */
	r.x += size;
	r.w = wind->r.w - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* right-bottom box */
	r.x = wind->r.x + wind->r.w - (size + 2);
	r.w = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* right border */
	r.y = wind->r.y + size;
	r.h = wind->r.h - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* top-right box */
	r.y = wind->r.y;
	r.h = size;
	draw_widg_box(0, wci, 0, &r, &wind->r);

	/* Top border*/
	r.x = wind->r.x + size;
	r.w = wind->r.w - (size + size + 2);
	draw_widg_box(0, wci, 0, &r, &wind->r);

}

static bool
display_closer(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->closer;

	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
display_hider(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->hider;
	
	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
display_fuller(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->fuller;
	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static bool
display_iconifier(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->iconifier;

	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
display_sizer(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->sizer;

	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

static bool
display_uparrow(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->uparrow;

	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
display_dnarrow(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->dnarrow;

	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
display_lfarrow(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->lfarrow;

	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}
static bool
display_rtarrow(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->colours->rtarrow;

	rp_2_ap(wind, widg, &widg->ar);	
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(widg, &wind->widg_info, widg->loc.rsc_index);
	return true;
}

/*======================================================
	TITLE WIDGET BEHAVIOUR
========================================================*/
static bool
display_title(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct options *o = &wind->owner->options;
	struct xa_wcol_inf *wci = &wind->colours->title;
	struct xa_wtxt_inf *wti = &wind->colours->title_txt;
	char tn[256];
	bool dial = (wind->dial & (created_for_FORM_DO|created_for_FMD_START)) != 0;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &widg->ar);

	if (MONO)
	{
		f_color(G_WHITE);
		t_color(G_BLACK);
#if 0
		if (wind->nolist || is_topped(wind))
			/* Highlight the title bar of the top window */
			effect = cfg.topname;
		else
			effect = cfg.backname;
#endif
		/* with inside border */
		p_gbar(0, &widg->ar);
	}
	else
	{
		/* no move, no 3D */
		if (wind->active_widgets & MOVER)
		{
			draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
		}
		else
		{
			l_color(screen.dial_colours.shadow_col);
			p_gbar(0, &widg->ar);
			f_color(wci->n.c);
			gbar(-1, &widg->ar);
		}
	}

	wr_mode(MD_TRANS);
	
	if (o->windowner && wind->owner && !wind->winob && !dial)
	{
		char *ow = NULL;

		if (o->windowner == true)
			ow = wind->owner->proc_name;
		/* XXX this can't happen
		else if (o->windowner == 2)
			ow = wind->owner->name; */

		if  (ow)
		{
			char ns[32];
			strip_name(ns, ow);
			if (*ns)
				sprintf(tn, sizeof(tn), "(%s) %s", ns, widg->stuff);
			else
				ow = NULL;
		}

		if (!ow)
			sprintf(tn, sizeof(tn), "(%d) %s", wind->owner->p->pid, widg->stuff);
	}
	else
		strcpy(tn, widg->stuff);

	
	//cramped_name(tn, temp, widg->ar.w/screen.c_max_w);
#if 0
	if (dial)
		effect |= ITALIC;
#endif
	draw_widget_text(widg, wti, tn, 4, 0);
	return true;
}
/*
 * Click & drag on the title bar - does a move window
 */
static bool
drag_title(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	/* You can only move a window if its MOVE attribute is set */
	if (wind->active_widgets & MOVER)
	{
		RECT r = wind->r, d;

#if PRESERVE_DIALOG_BGD
		{
			struct xa_window *scan_wind;

			/* Don't allow windows below a STORE_BACK to move */
			if (wind != window_list)
			{
				for (scan_wind = wind->prev; scan_wind; scan_wind = scan_wind->prev)
				{
					if (scan_wind->active_widgets & STORE_BACK)
					{
						return true;
					}
				}
			}
		}
#endif
		if (!widget_active.cont)
		{
			widget_active.cont = true;
			/* Always have a nice consistent MOVER when dragging a box */
			graf_mouse(XACRS_MOVER, NULL, NULL, false);
		}

		/* If right button is used, do a classic outline drag. */
		if (   widg->s == 2
		    || wind->send_message == NULL
		    || wind->owner->options.nolive)
		{
			RECT bound;
			
			bound.x = wind->owner->options.noleft ?
					0 : -root_window->r.w;
			bound.y = root_window->wa.y;
			bound.w = root_window->r.w*3;
			bound.h = root_window->r.h*2;

			lock_screen(wind->owner->p, 0, 0, 1234);
			drag_box(wind->owner, r, &bound, rect_dist_xy(wind->owner, md->x, md->y, &r, &d), &r);
			unlock_screen(wind->owner->p, 1235);

			if (r.x != wind->rc.x || r.y != wind->rc.y)
			{
				r.h = wind->rc.h;
				send_moved(lock, wind, AMQ_NORM, &r);
			}
		}
		else if (widget_active.m.cstate)
		{
			short pmx, pmy, /*mx, my,*/ mb;

			/* need to do this anyhow, for mb */
			/* vq_mouse(C.vh, &mb, &pmx, &pmy);*/

			if (widget_active.widg)
			{
				/* pending widget: take that */
				pmx = widget_active.x;
				pmy = widget_active.y;
				d   = widget_active.d;
			}
			else
			{
				pmx = widget_active.m.x;
				pmy = widget_active.m.y;
				mb  = widget_active.m.cstate;
				rect_dist_xy(wind->owner, pmx, pmy, &r, &d);
				widget_active.m.x = md->sx;
				widget_active.m.y = md->sy;
			}

			{
				/* Drag title */

				set_widget_active(wind, widg, drag_title, 1);

				widget_active.x = widget_active.m.x;
				widget_active.y = widget_active.m.y;

				widget_active.d.x = d.x;
				widget_active.d.y = d.y;

				if (widget_active.x != pmx || widget_active.y != pmy)
					r = move_rectangle(widget_active.x, widget_active.y, r, &d);

				if (wind->owner->options.noleft)
					if (r.x < 0)
						r.x = 0;

				if (r.y < root_window->wa.y)
					r.y = root_window->wa.y;

				if (r.x != wind->r.x || r.y != wind->r.y)
				{
					r.h = wind->rc.h;
					send_moved(lock, wind, AMQ_NORM, &r);
				}

				/* We return false here so the widget display
				 * status stays selected whilst it repeats
				 */
				return false;
			}
		}
	}

	cancel_widget_active(wind, 1);
	return true;
}

/*
 * Single click title bar sends window to the back
 */
static bool
click_title(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	/* Ozk: If either shifts pressed, unconditionally send the window to bottom */
	if (md->state == MBS_LEFT)
	{
		if ((widg->k & 3) && (wind->active_widgets & (STORE_BACK|BACKDROP)) == BACKDROP)
		{
			send_bottomed(lock, wind);
#if 0
			if (wind->send_message)
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
						   WM_BOTTOMED, 0, 0, wind->handle, 0, 0, 0, 0);
			else
				bottom_window(lock, wind);
#endif
		}
		else
		{
			/* if not on top or on top and not focus */
			if (!is_topped(wind))
			{
				send_topped(lock, wind);
#if 0
				if (wind->send_message)
					wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
							   WM_TOPPED, 0, 0, wind->handle,
							   0, 0, 0, 0);
				else
					/* Just top these windows, they can handle it... */
					/* Top the window */
					top_window(lock, true, false, wind, (void *)-1L);
#endif
			}
			/* If window is already top, then send it to the back */

			/* Ozk: Always bottom iconified windows... */
			else if ((wind->window_status & XAWS_ICONIFIED))
			{
				send_bottomed(lock, wind);
#if 0
				if (wind->send_message)
					wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
							   WM_BOTTOMED, 0, 0, wind->handle, 0,0,0,0);
#endif
			}
			else if ((wind->active_widgets & (STORE_BACK|BACKDROP)) == BACKDROP)
			{
				send_bottomed(lock, wind);
#if 0
					if (wind->send_message)
						wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
								   WM_BOTTOMED, 0, 0, wind->handle,
								   0, 0, 0, 0);
					else
						/* Just bottom these windows, they can handle it... */
						bottom_window(lock, wind);
#endif
			}
		}
	}
	else if (md->state == MBS_RIGHT && !(wind->dial & created_for_SLIST))
	{
		if (!(wind->window_status & XAWS_ZWSHADED))
		{
			if ((wind->window_status & XAWS_SHADED))
			{
				if (wind->send_message)
				{
					DIAGS(("Click_title: unshading window %d for %s",
						wind->handle, wind->owner->name));

					wind->send_message(lock, wind, NULL, AMQ_CRITICAL, QMF_CHKDUP,
						WM_UNSHADED, 0, 0,wind->handle, 0, 0, 0, 0);

					move_window(lock, wind, true, ~(XAWS_SHADED|XAWS_ZWSHADED), wind->rc.x, wind->rc.y, wind->rc.w, wind->rc.h);
				}
			}
			else
			{
				if (wind->send_message)
				{
					DIAGS(("Click_title: shading window %d for %s",
						wind->handle, wind->owner->name));

					move_window(lock, wind, true, XAWS_SHADED, wind->rc.x, wind->rc.y, wind->rc.w, wind->rc.h);
					wind->send_message(lock, wind, NULL, AMQ_CRITICAL, QMF_CHKDUP,
						WM_SHADED, 0, 0, wind->handle, 0,0,0,0);
				}
			}
		}
	}
	return true;
}

/*
 * Double click title bar of iconified window - sends a restore message
 */
static bool
dclick_title(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	if (wind->send_message)
	{
		/* If window is iconified - send request to restore it */

		if ((wind->window_status & XAWS_ICONIFIED))
		{
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					   WM_UNICONIFY, 0, 0, wind->handle,
					   wind->pr.x, wind->pr.y, wind->pr.w, wind->pr.h);
		}
		else if (wind->active_widgets & FULLER)
		{
			/* Ozk 100503: Double click on title now sends WM_FULLED,
			 * as N.AES does it */
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					   WM_FULLED, 0, 0, wind->handle,
					   0, 0, 0, 0);
		}
		return true;
	}
	return false;
}

/*======================================================
	INFO BAR WIDGET BEHAVIOUR
========================================================*/
static bool
display_info(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	//char t[160];
	//short x, y;
	struct xa_wcol_inf *wci = &wind->colours->info;
	struct xa_wtxt_inf *wti = &wind->colours->info_txt;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &widg->ar);
	draw_widg_box(0, wci, widg->state, &widg->ar, &wind->r);
	draw_widget_text(widg, wti, widg->stuff, 4, 0); 
	return true;
}

/*======================================================
	CLOSE WIDGET BEHAVIOUR
========================================================*/
/* Displayed by display_def_widget */

/* 
 * Default close widget behaviour - just send a WM_CLOSED message to the client that
 * owns the window.
 */
static bool
click_close(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_CLOSED, 0, 0, wind->handle,
				   0, 0, 0, 0);

		/* Redisplay.... */
		return true;
	}

	/* Just close these windows, they can handle it... */
	close_window (lock, wind);
	/* delete on the next possible time */
	delayed_delete_window(lock, wind);

	/* Don't redisplay in the do_widgets() routine */
	return false;
}

/*======================================================
	FULLER WIDGET BEHAVIOUR
========================================================*/
/* Displayed by display_def_widget */

/* Default full widget behaviour - Just send a WM_FULLED message to the client that */
/* owns the window. */
static bool
click_full(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	if (wind->send_message)
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_FULLED, 0, 0, wind->handle,
				   0, 0, 0, 0);
	return true;
}

/*======================================================
	ICONIFY WIDGET BEHAVIOUR
========================================================*/
/* Displayed by display_def_widget */

/*
 * click the iconify widget
 */
static bool
click_iconify(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	DIAGS(("click_iconify:"));
	
	if (wind->send_message == NULL)
		return false;

	if ((wind->window_status & XAWS_OPEN))
	{
		if ((wind->window_status & XAWS_ICONIFIED))
		{
			/* Window is already iconified - send request to restore it */

			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					   WM_UNICONIFY, 0, 0, wind->handle,
					   wind->ro.x, wind->ro.y, wind->ro.w, wind->ro.h);
		}
		else
		{
			/* Window is open - send request to iconify it */

			RECT ic = free_icon_pos(lock|winlist, NULL);

			/* Could the whole screen be covered by iconified
			 * windows? That would be an achievement, wont it?
			 */
			if (ic.y > root_window->wa.y)
			{
				short msg = (md->kstate & K_CTRL) ? WM_ALLICONIFY : WM_ICONIFY;

				wind->send_message(lock|winlist, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					   msg, 0, 0, wind->handle,
					   ic.x, ic.y, ic.w, ic.h);
			}
		}
	}
	/* Redisplay.... */
	return true;
}

/*======================================================
	HIDER WIDGET BEHAVIOUR
========================================================*/
/* Displayed by display_def_widget */

/*
 * click the hider widget
 */
static bool
click_hide(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	hide_app(lock, wind->owner);

	/* Redisplay.... (Unselect basically.) */
	return true;
}

/* 
 * helper functions for sizing.
 */
static bool
display_border(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	/* Dummy; no draw, no selection status. */
	return true;
}

COMPASS
compass(short d, short x, short y, RECT r)
{
	r.x += d;
	r.y += d;
	r.w -= d+d;
	r.h -= d+d;

	r.w += r.x - 1;
	r.h += r.y - 1;

	if (x < r.x)
	{
		if (y < r.y)	return NW;
		if (y > r.h)	return SW;
		return W_;
	}
	if (x > r.w)
	{
		if (y < r.y)	return NE;
		if (y > r.h)	return SE;
		return E_;
	}
	if (y < r.y)	return N_;
	return S_;
}

/* HR 150202: make rubber_box omnidirectional
 *            and use it here for outline sizing.
 */
static bool
size_window(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, bool sizer, WidgetBehaviour next, const struct moose_data *md)
{
	bool move, size;
	RECT r = wind->r, d;
	
	if (   widg->s == 2
	    || wind->send_message == NULL
	    || wind->owner->options.nolive)
	{
		int xy = sizer ? SE : compass(16, widg->mx, widg->my, r);

		/* Always have a nice consistent SIZER when resizing a window */
		graf_mouse(border_mouse[xy], NULL, NULL, false);

		lock_screen(wind->owner->p, 0, 0, 1234);
		rubber_box(wind->owner, xy,
		           r,
			   rect_dist_xy(wind->owner, md->x, md->y, &r, &d),
			   wind->min.w,
			   wind->min.h,
			   wind->max.w,
			   wind->max.h,
			   &r);
		unlock_screen(wind->owner->p, 1234);

		move = r.x != wind->r.x || r.y != wind->r.y,
		size = r.w != wind->r.w || r.h != wind->r.h;

		if (move || size)
		{
			if (move && size && (wind->opts & XAWO_SENDREPOS))
				send_reposed(lock, wind, AMQ_NORM, &r);
			else
			{
				if (move)
					send_moved(lock, wind, AMQ_NORM, &r);
				if (size)
					send_sized(lock, wind, AMQ_NORM, &r);
			}
		}
	}
	else if (widget_active.m.cstate)
	{
		short pmx, pmy;
		COMPASS xy;

		if (widget_active.widg)
		{
			/* pending widget: take that */
			pmx = widget_active.x;
			pmy = widget_active.y;
			d   = widget_active.d;
			xy  = widget_active.xy;
		}
		else
		{
			pmx = widget_active.m.x;
			pmy = widget_active.m.y;
			xy  = sizer ? SE : compass(16, pmx, pmy, r);
			rect_dist_xy(wind->owner, md->x, md->y, &r, &d);
			widget_active.m.x = md->sx;
			widget_active.m.y = md->sy;
		}

		/* Drag border */
		if (widget_active.m.cstate)
		{
			set_widget_active(wind, widg, next, 6);

			widget_active.x = widget_active.m.x;
			widget_active.y = widget_active.m.y;

			widget_active.d = d;
			widget_active.xy = xy;

			if (!widget_active.cont)
			{
				widget_active.cont = true;
				/* Always have a nice consistent SIZER when resizing a window */
				graf_mouse(border_mouse[xy], NULL, NULL, false);
			}

			/* Has the mouse moved? */
			if (widget_active.m.x != pmx || widget_active.m.y != pmy)
			{
				r = widen_rectangle(xy, widget_active.m.x, widget_active.m.y, r, &d);

				check_wh_cp(&r, xy,
					    wind->min.w,
					    wind->min.h,
					    wind->max.w,
					    wind->max.h);
			}

			move = r.x != wind->r.x || r.y != wind->r.y,
			size = r.w != wind->r.w || r.h != wind->r.h;
			
			if (move || size)
			{
				if (move && size && (wind->opts & XAWO_SENDREPOS))
					send_reposed(lock, wind, AMQ_NORM, &r);
				else
				{
					if (move)
						send_moved(lock, wind, AMQ_NORM, &r);
					if (size)
						send_sized(lock, wind, AMQ_NORM, &r);
				}
			}
			/* We return false here so the widget display status
			 * stays selected whilst it repeats */
			return false;
		}
	}

	cancel_widget_active(wind, 5);
	return true;
}

/*======================================================
	RESIZE WIDGET BEHAVIOUR
========================================================*/
/* Displayed by display_def_widget */

/* HR: use the distance from the mouse to the lower right corner.
	just like WINX & MagiC */
/* HR 150202: make rubber_box omnidirectional. */

static inline bool
drag_resize(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	return size_window(lock, wind, widg, true, drag_resize, md);
}
static inline bool
drag_border(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	return size_window(lock, wind, widg, false, drag_border, md);
}


/*
 * Scroll bar & Slider handlers
 */

/*======================================================
 *	SCROLL WIDGET BEHAVIOUR
 * =====================================================
 * Displayed by display_def_widget
 * 
 * HR 181201: use widg->state (right/left click) to determine the direction of the arrow effect.
 *            double click on an arrow sends WM_xSLIDE with the apropriate limit.
 */

//static struct xa_client *do_widget_repeat_client = NULL;
//static enum locks do_widget_repeat_lock;

void
do_widget_repeat(void)
{
	if (C.do_widget_repeat_client)
	{
		do_active_widget(C.do_widget_repeat_lock, C.do_widget_repeat_client);

		/*
		 * Ozk: The action functions clear the active widget by clearing the 
		 * widg member when the mouse button is released, so we check that
		 * instead of the button state here.
		 */
		if (!widget_active.widg)
		{
			aessys_timeout = 0;
			C.do_widget_repeat_client = NULL;
		}
	}
}

static void
set_widget_repeat(enum locks lock, struct xa_window *wind)
{
	if (!C.do_widget_repeat_client)
	{
		aessys_timeout = 1;
		C.do_widget_repeat_client = wind->owner;
		C.do_widget_repeat_lock = lock;

		/* wakup wakeselect */
		wakeselect(C.Aes->p);
	}
}

static bool
click_scroll(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	XA_WIDGET *slider = &(wind->widgets[widg->slider_type]);
	bool reverse = (widg->s == 2);
	short mx = md->x;
	short my = md->y;
	short mb = md->state;

	if (!(   widget_active.widg
	      && slider
	      && slider->stuff
	      && ((XA_SLIDER_WIDGET *)slider->stuff)->position == (reverse ? widg->xlimit : widg->limit)))
	{
		if (md->kstate & 3)
		{
			int inside;

			/* XXX 
			 * Center sliders at the clicked position.
			 * Wait for mousebutton release
			 */
			if (mb == md->cstate)
			{
				graf_mouse(XACRS_POINTSLIDE, NULL, NULL, false);
				check_mouse(wind->owner, &mb, NULL, NULL);

				S.wm_count++;
				while (mb == md->cstate)
					wait_mouse(wind->owner, &mb, &mx, &my);
				S.wm_count--;
			}
			
			/* Convert relative coords and window location to absolute screen location */
			rp_2_ap(wind, slider, &slider->ar);

			inside = m_inside(mx, my, &slider->ar);
			if (inside)
			{
				XA_SLIDER_WIDGET *sl = slider->stuff;
				int offs;
				
				if (widg->slider_type == XAW_VSLIDE)
					offs = bound_sl(pix_to_sl(my - slider->ar.y - (sl->r.h >> 1), slider->r.h - sl->r.h));
				else
					offs = bound_sl(pix_to_sl(mx - slider->ar.x - (sl->r.w >> 1), slider->r.w - sl->r.w));

				if (offs != sl->position)
				{
					sl->rpos = offs;				
					if (wind->send_message)
						wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
								   widg->slider_type == XAW_VSLIDE ? WM_VSLID : WM_HSLID,
								   0, 0, wind->handle, offs, 0, 0, 0);
				}
			}

			cancel_widget_active(wind, 2);
			return true;
		}

		if (md->clicks > 1)
		{
			if (reverse)
			{
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
						   widg->slider_type == XAW_VSLIDE ? WM_VSLID : WM_HSLID,
						   0, 0, wind->handle, widg->xlimit, 0, 0, 0);
			}
			else
			{
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
						   widg->slider_type == XAW_VSLIDE ? WM_VSLID : WM_HSLID, 
						   0, 0, wind->handle, widg->limit, 0, 0, 0);
			}
		}
		else
		{
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					   WM_ARROWED, 0, 0, wind->handle,
					   reverse ? widg->xarrow : widg->arrowx, 0, 0, 0);

			if (md->cstate)
			{
				/* If the button has been held down, set a
				 * pending/active widget for the client */

				set_widget_active(wind, widg, widg->drag, 2);
				set_widget_repeat(lock, wind);

				/* We return false here so the widget display
				 * status stays selected whilst it repeats */
				return false;
			}
		}
	}

	cancel_widget_active(wind, 2);
	return true;
}

/*======================================================
	VERTICAL SLIDER WIDGET BEHAVIOUR
	The slider widgets are slightly more complex than other widgets
	as they have three separate 'widgets' inside them.
	(I know GEM doesn't have these, but I think they're cool)

	HR: NO NO NO, they're NOT cool, they're unnecessarily ugly and complex!!
		They would have been cool though, if the widget structure had been nested,
		and the widget handling code recursive.
		Maybe once I'll implement such a thing.

	HR: code to safeguard the actual boundaries of the whole slider widget.
		We want to be independent of arithmatic rounding.

	Also: introduction of a slider rectangle in XA_SLIDER_WIDGET,
		  saves some arithmetic, but more important,
		  this was necessary since the introducion of a mimimum size.
	Optimization of drawing.
========================================================*/
static bool
display_unused(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wc = &wind->colours->win;
	rp_2_ap(wind, widg, &widg->ar);
	draw_widg_box(0, wc, 0, &widg->ar, &wind->r);
	return true;
}

/* Display now includes the background */
bool
display_vslide(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	RECT cl;
	struct xa_window_colours *wc = wind->colours;

	sl->flags &= ~SLIDER_UPDATE;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.x = sl->r.y = 0;
		sl->r.w = widg->ar.w;
		sl->r.h = widg->ar.h;
		cl = widg->ar;
		draw_widg_box(0, &wc->slider, 0, &widg->ar, &widg->ar/*&wind->r*/);
		return true;
	}
	len = sl_to_pix(widg->ar.h, sl->length);
	if (len < cfg.widg_h - 3)
		len = cfg.widg_h - 3;
	
	offs = widg->ar.y + sl_to_pix(widg->ar.h - len, sl->position);

	if (offs < widg->ar.y)
		offs = widg->ar.y;
	if (offs + len > widg->ar.y + widg->ar.h)
		len = widg->ar.y + widg->ar.h - offs;

	draw_widg_box(0, &wc->slide, 0, &widg->ar, &widg->ar/*&wind->r*/);	
	
	sl->r.y = offs - widg->ar.y;
	sl->r.h = len;
	sl->r.w = widg->ar.w;

	cl.x = sl->r.x + widg->ar.x;
	cl.y = sl->r.y + widg->ar.y;
	cl.w = sl->r.w;
	cl.h = sl->r.h;
	
	draw_widg_box(-1, &wc->slider, widg->state, &cl, &cl/*&wind->r*/);
	wr_mode(MD_TRANS);
	return true;
}

/*======================================================
	HORIZONTAL SLIDER WIDGET BEHAVIOUR
	same as vertical, except for x,w & y,h
========================================================*/

static bool
display_hslide(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	struct xa_window_colours *wc = wind->colours;
	RECT cl;

	sl->flags &= ~SLIDER_UPDATE;

	rp_2_ap(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.x = sl->r.y = 0;
		sl->r.w = widg->ar.w;
		sl->r.h = widg->ar.h;
		draw_widg_box(0, &wc->slider, 0, &widg->ar, &widg->ar/*&wind->r*/);
		return true;
	}
	len = sl_to_pix(widg->ar.w, sl->length);
	if (len < cfg.widg_w - 3)
		len = cfg.widg_w - 3;
	
	offs = widg->ar.x + sl_to_pix(widg->ar.w - len, sl->position);

	if (offs < widg->ar.x)
		offs = widg->ar.x;
	if (offs + len > widg->ar.x + widg->ar.w)
		len = widg->ar.x + widg->ar.w - offs;
	
	draw_widg_box(0, &wc->slide, 0, &widg->ar, &widg->ar/*&wind->r*/);	
	
	sl->r.x = offs - widg->ar.x;
	sl->r.w = len;
	sl->r.h = widg->ar.h;

	cl.x = sl->r.x + widg->ar.x;
	cl.y = sl->r.y + widg->ar.y;
	cl.w = sl->r.w;
	cl.h = sl->r.h;
	
	draw_widg_box(-1, &wc->slider, widg->state, &cl, &cl/*&wind->r*/);
	wr_mode(MD_TRANS);
	return true;
}

/*
 * HR 050601: Heavily cleaned up the code.
 * As a reward the sliders do NOT behave sticky at the edges.
 * I immensely hate that behaviour, as it seems to be the standard for certain
 * operating systems nowadays.
 * It is unnatural and unintelligent.
 * 
 * Note, that the stuff for clicking on page arrows has completely vanished,
 * as that is mow unified with the line arrows.
 */

static bool
drag_vslide(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	XA_SLIDER_WIDGET *sl = widg->stuff;

	if (widget_active.m.cstate)
	{
		short offs;
		
		if (widget_active.m.cstate & MBS_RIGHT)
		{
			RECT s, b, d, r;
			rp_2_ap(wind, widg, &widg->ar);
			
			b = s = widg->ar;
			
			s.x += sl->r.x;
			s.y += sl->r.y;
			s.w = sl->r.w;
			s.h = sl->r.h;

			lock_screen(wind->owner->p, 0, 0, 0);
			graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			unlock_screen(wind->owner->p, 0);

			offs = bound_sl(pix_to_sl(r.y - widg->ar.y, widg->r.h - sl->r.h));

			if (offs != sl->position)
			{
				sl->rpos = offs;
				send_vslid(lock, wind, offs);
			}
		}
		else
		{
			short y;

			if (!widget_active.cont)
			{
				widget_active.cont = true;
				/* Always have a nice consistent sizer when dragging a box */
				graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
				rp_2_ap(wind, widg, &widg->ar);
				widget_active.offs = md->y - (widg->ar.y + sl->r.y);
				widget_active.y = md->y;
				y = md->sy;
			}
			else
				y = md->y;

			if (widget_active.y != y)
			{
				offs = bound_sl(pix_to_sl((y - widget_active.offs) - widg->ar.y, widg->r.h - sl->r.h));
		
				if (offs != sl->position && wind->send_message)
				{
					sl->rpos = offs;
					send_vslid(lock, wind, offs);
				}
				widget_active.y = y;
			}
			set_widget_active(wind, widg, drag_vslide, 3);
			return false;
		}
	}
	cancel_widget_active(wind, 3);
	return true;
}

static bool
drag_hslide(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	XA_SLIDER_WIDGET *sl = widg->stuff;

	if (widget_active.m.cstate)
	{
		short offs;

		if (widget_active.m.cstate & MBS_RIGHT)
		{
			RECT s, b, d, r;

			rp_2_ap(wind, widg, &widg->ar);
			
			b = s = widg->ar;
			
			s.x += sl->r.x;
			s.y += sl->r.y;
			s.w = sl->r.w;
			s.h = sl->r.h;

			lock_screen(wind->owner->p, 0, 0, 0);
			graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			unlock_screen(wind->owner->p, 0);
			
			offs = bound_sl(pix_to_sl(r.x - widg->ar.x, widg->r.w - sl->r.w));

			if (offs != sl->position)
			{
				sl->rpos = offs;
				send_hslid(lock, wind, offs);
			}
		}
		else
		{
			short x;
			
			if (!widget_active.cont)
			{
				widget_active.cont = true;
				/* Always have a nice consistent sizer when dragging a box */
				graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
				rp_2_ap(wind, widg, &widg->ar);
				widget_active.offs = md->x - (widg->ar.x + sl->r.x);
				widget_active.x = md->x;
				x = md->sx;
			}
			else
				x = md->x;
			
			if (widget_active.x != x)
			{
			
				offs = bound_sl(pix_to_sl((x - widget_active.offs) - widg->ar.x, widg->r.w - sl->r.w));
		
				if (offs != sl->position && wind->send_message)
				{
					sl->rpos = offs;
					send_hslid(lock, wind, offs);
				}
				widget_active.x = x;
			}
			set_widget_active(wind, widg, drag_hslide, 4);
			return false;
		}
	}

	cancel_widget_active(wind, 4);
	return true;
}

/*
 * Generic Object Tree Widget display
 */
bool
display_object_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	XA_TREE *wt = widg->stuff;
	OBJECT *root;

	/* Convert relative coords and window location to absolute screen location */
	root = rp_2_ap(wind, widg, NULL);

	DIAG((D_form,wind->owner,"display_object_widget(wind=%d), wt=%lx, e.obj=%d, e.pos=%d, form: %d/%d",
		wind->handle, wt, wt->e.obj, wt->e.pos, root->ob_x, root->ob_y));

	if (wind->nolist && (wind->dial & created_for_POPUP))
	{
		/* clip work area */
		set_clip(&wind->wa);
		draw_object_tree(lock, wt, NULL, widg->start, 100, NULL);
		clear_clip();
	}
	else
		draw_object_tree(lock, wt, NULL, widg->start, 100, NULL);

	return true;
}

static bool
display_arrow(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	return true;
}

static inline void
zwidg(struct xa_window *wind, short n, bool keepstuff)
{
	XA_WIDGET *widg = wind->widgets + n;
	void *stuff = NULL;
	short st = 0;

	if (keepstuff)
	{
		stuff = widg->stuff;
		st = widg->stufftype;
	}
	else if (widg->destruct)
		(*widg->destruct)(widg);

	bzero(widg, sizeof(XA_WIDGET));

	widg->type = n;

	if (stuff)
	{
		widg->stuff = stuff;
		widg->stufftype = st;
	}
}

struct wdglo_desc
{
	XA_RELATIVE rel;
	XA_WIND_ATTR tp;
};

static struct wdglo_desc widglayout[] =
{
	{(XAR_START),			(NAME | CLOSER | FULLER | ICONIFIER | HIDE)},
	{(XAR_START),			INFO},
	{(XAR_START),			XaMENU},
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER)},
	{(XAR_END),			(LFARROW | HSLIDE | RTARROW)},
	{0, -1},
};

static struct widget_row *
create_widg_layout(struct xa_window *wind, struct wdglo_desc *wld)
{
	XA_WIND_ATTR tp;
	struct wdglo_desc *w;
	int nrows;
	struct widget_row *rows, *ret = NULL;

	w = wld;
	nrows = 0;

	while (w->tp != -1)
		w++, nrows++;

	if (nrows && (rows = kmalloc( (long)sizeof(*rows) * nrows)))
	{
		int rownr = 0;
		struct widget_row *p = NULL;

		ret = rows;

		while ((tp = wld->tp) != -1)
		{
			rows->start = NULL;
			rows->tp_mask = tp;
			rows->rel = wld->rel;
			rows->rownr = rownr++;
			rows->r = (RECT){0, 0, 0, 0};
			
			wld++;
			rows->prev = p;
			if (wld->tp != -1)
				rows->next = rows + 1;
			else
				rows->next = NULL;
			
			p = rows;
			rows++;	
		}
	}
	return ret;
}

static void
set_title_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->ontop_cols->title;
	struct xa_wtxt_inf *wti = &wind->ontop_cols->title_txt;
	short w, h;

	widg->r.w = cfg.widg_w;

	t_font(wti->n.p, wti->n.f);
	vst_effects(C.vh, wti->n.e);
	t_extent("A", &w, &h);
	vst_effects(C.vh, 0);
	
	if ((wci->flags & WCOL_DRAW3D) || (wti->flags & WTXT_DRAW3D))
		h += 4;
	if ((wci->flags & WCOL_ACT3D) || (wti->flags & WTXT_ACT3D))
		h++;

	widg->r.h = h;
};
static void
set_info_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &wind->ontop_cols->info;
	struct xa_wtxt_inf *wti = &wind->ontop_cols->info_txt;
	short w, h;

	widg->r.w = cfg.widg_w;
		
	t_font(wti->n.p, wti->n.f);
	vst_effects(C.vh, wti->n.e);
	t_extent("A", &w, &h);
	vst_effects(C.vh, 0);
	
	if ((wci->flags & WCOL_DRAW3D) || (wti->flags & WTXT_DRAW3D))
		h += 4;
	if ((wci->flags & WCOL_ACT3D) || (wti->flags & WTXT_ACT3D))
		h++;

	widg->r.h = h;
};
static void
set_menu_size(struct xa_window *wind, struct xa_widget *widg)
{
#if 1
	short w, h;
	t_font(screen.standard_font_point, screen.standard_font_id);
	vst_effects(C.vh, 0);
	t_extent("A", &w, &h);
	
	widg->r.h = h + 1 + 1; //screen.standard_font_height + 1;
	widg->r.w = wind->r.w;
#else
	widg->r.h = MENU_H + 1;
	widg->r.w = wind->r.w;
#endif

}

static void
set_widg_size(struct xa_window *wind, struct xa_widget *widg, struct xa_wcol_inf *wci, short rsc_ind)
{
	short w, h, f;
	OBJECT *ob = wind->widg_info.tree + rsc_ind;

	f = wci->flags;

	object_spec_wh(ob, &w, &h);

	if (f & WCOL_DRAW3D)
		h += 2, w += 2;
	if (f & WCOL_BOXED)
		h += 2, w += 2;

	widg->r.w = w;
	widg->r.h = h;
}
static void
set_closer_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->closer, widg->loc.rsc_index);
}
static void
set_hider_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->hider, widg->loc.rsc_index);
}
static void
set_iconifier_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->iconifier, widg->loc.rsc_index);
}
static void
set_fuller_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->fuller, widg->loc.rsc_index);
}
static void
set_uparrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->uparrow, widg->loc.rsc_index);
}
static void
set_dnarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->dnarrow, widg->loc.rsc_index);
}
static void
set_lfarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->lfarrow, widg->loc.rsc_index);
}
static void
set_rtarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->rtarrow, widg->loc.rsc_index);
}
static void
set_sizer_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->sizer, widg->loc.rsc_index);
}
/*
 * The vertical slider must get its dimentions from the arrows
 */
static void
set_vslide_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &wind->ontop_cols->uparrow, WIDG_UP);
}

/* 
 * Setup the required 'standard' widgets for a window. These are the ordinary GEM
 * behaviours. These can be changed for any given window if you want special behaviours.
 */
//position_widget(struct xa_widget *w, struct widget_row *strt_rows, void(*widg_size)(struct xa_widget *widg))
void
standard_widgets(struct xa_window *wind, XA_WIND_ATTR tp, bool keep_stuff)
{
	XA_WIDGET *widg;
	int bottom = 0, right = 0;
	XA_WIND_ATTR old_tp = wind->active_widgets;
	XA_WIND_ATTR utp;

	DIAGS(("standard_widgets: new(%lx), prev(%lx) on wind %d for %s",
		tp, old_tp, wind->handle, wind->owner->proc_name));

	if (!wind->widg_rows)
		wind->widg_rows = create_widg_layout(wind, (struct wdglo_desc *)&widglayout);
	else
	{
		struct widget_row *rows = wind->widg_rows;
		while (rows)
		{
			rows->start = NULL;
			rows->r = (RECT){0, 0, 0, 0};
			rows = rows->next;
		}
	}

	/* Fill in the active widgets summary */
	wind->active_widgets = utp = tp;

	/* HR 300801: Increases stability (a permenent widget tree of its own). */
	wind->widg_info.tree = def_widgets;
	wind->widg_info.owner = wind->owner;

	if (tp & (LFARROW|HSLIDE|RTARROW))
		bottom = 1;
	if (tp & (UPARROW|VSLIDE|DNARROW))
		right = 1;

	/* 
	 * function make_widget()
	 * 
	 * The function is not only to spare some bytes.
	 * Most important is to keep the organization and the access mechanisms
	 * of the widgets out of sight and centralized.
	 * And, of course, remove duplication of trivial text.
	 * Such things make life easier for a maintainer.
	 * widgets_on_top has become obsolete and is removed.
	 */

	if (tp & CLOSER)
	{
		DIAGS(("Make CLOSER"));
		make_widget(wind, &stdl_close, display_closer, click_close, NULL, NULL, set_closer_size);
	}
	else if (old_tp & CLOSER)
	{
		DIAGS(("Clear closer"));
		zwidg(wind, XAW_CLOSE, keep_stuff);
	}

	if (tp & FULLER)
	{
		DIAGS(("Make fuller"));
		make_widget(wind, &stdl_full, display_fuller, click_full, NULL, NULL, set_fuller_size);
	}
	else if (old_tp & FULLER)
	{
		DIAGS(("Clear fuller"));
		zwidg(wind, XAW_FULL, keep_stuff);
	}

	if (tp & ICONIFIER)
	{
		DIAGS(("Make iconifier"));
		widg = make_widget(wind, &stdl_iconify, display_iconifier, click_iconify, NULL, NULL, set_iconifier_size);
	}
	else if (old_tp & ICONIFIER)
	{
		DIAGS(("clear iconifier"));
		zwidg(wind, XAW_ICONIFY, keep_stuff);
	}

	if (tp & HIDE)
	{
		DIAGS(("Make hider"));
		widg = make_widget(wind, &stdl_hide, display_hider, click_hide, NULL, NULL, set_hider_size);
	}
	else if (old_tp & HIDE)
		zwidg(wind, XAW_HIDE, keep_stuff);

	if (tp & SIZER)
	{
		DIAGS(("Make sizer"));
		make_widget(wind, &stdl_resize, display_sizer, NULL, drag_resize, NULL, set_sizer_size);
	}
	else if (bottom && right)
	{
		DIAGS(("Unused sizer"));
		make_widget(wind, &stdl_nresize, display_unused, NULL, NULL, NULL, set_sizer_size);
	}
	else // if ((old_tp & SIZER) || ((old_tp & (LFARROW|RTARROW|HSLIDE)) && (old_tp & (UPARROW|DNARROW|VSLIDE))))
	{
		DIAGS(("clear sizer"));
		zwidg(wind, XAW_RESIZE, keep_stuff);
	}

	if ( (tp & BORDER) || ((tp & (SIZER|MOVER)) == (SIZER|MOVER)) )
	{
		tp |= BORDER;
 		if (!(old_tp & BORDER))
 		{
			DIAGS(("Make border"));
			make_widget(wind, &stdl_border, display_border, drag_border, drag_border, NULL, NULL);
 		}
#if GENERATE_DIAGS
 		else
			DIAGS(("border already made"));
#endif
	}
	else if ((old_tp & BORDER))
	{
		DIAGS(("clear border"));
		zwidg(wind, XAW_BORDER, keep_stuff);
	}

	if (tp & UPARROW)
	{
		DIAGS(("Make uparrow"));
		widg = make_widget(wind, &stdl_uscroll, display_uparrow, click_scroll, click_scroll, NULL, set_uparrow_size);
		widg->dclick = click_scroll;
		widg->arrowx = WA_UPLINE;
		widg->xarrow = WA_DNLINE;
		widg->xlimit = SL_RANGE;
		widg->slider_type = XAW_VSLIDE;

		if (tp & VSLIDE)
		{
			struct xa_widget *w;
			DIAGS(("Make uppage"));
			w = make_widget(wind, &stdl_upage, display_arrow, click_scroll, click_scroll, NULL, NULL);
			w->arrowx = WA_UPPAGE;
			w->xarrow = WA_DNPAGE;
			w->xlimit = SL_RANGE;
			w->slider_type = XAW_VSLIDE;
		}
		else if (old_tp & VSLIDE)
		{
			DIAGS(("clear uppage"));
			zwidg(wind, XAW_UPPAGE, keep_stuff);
		}
	}
	else if (old_tp & UPARROW)
	{
		DIAGS(("clear uparrow"));
		zwidg(wind, XAW_UPLN, keep_stuff);
		if (old_tp & VSLIDE)
			zwidg(wind, XAW_UPPAGE, keep_stuff);
	}

	if (tp & DNARROW)
	{
		DIAGS(("Make dnarrow"));
		widg = make_widget(wind, &stdl_dscroll, display_dnarrow, click_scroll, click_scroll, NULL, set_dnarrow_size);
		widg->dclick = click_scroll;
		widg->arrowx = WA_DNLINE;
		widg->xarrow = WA_UPLINE;
		widg->limit = SL_RANGE;
		widg->slider_type = XAW_VSLIDE;
		if (tp & VSLIDE)
		{
			struct xa_widget *w;
			DIAGS(("Make vslide"));
			w = make_widget(wind, &stdl_dpage, display_arrow, click_scroll, click_scroll, NULL, NULL);
			w->arrowx = WA_DNPAGE;
			w->xarrow = WA_UPPAGE;
			w->limit = SL_RANGE;
			w->slider_type = XAW_VSLIDE;
		}
		else if (old_tp & VSLIDE)
		{
			DIAGS(("Clear vslide"));
			zwidg(wind, XAW_DNPAGE, keep_stuff);
		}
	}
	else if (old_tp & DNARROW)
	{
		DIAGS(("Clear dnarrow"));
		zwidg(wind, XAW_DNLN, keep_stuff);
		if (old_tp & VSLIDE)
			zwidg(wind, XAW_DNPAGE, keep_stuff);
	}

	if (tp & LFARROW)
	{
		DIAGS(("Make lfarrow"));
		widg = make_widget(wind, &stdl_lscroll, display_lfarrow, click_scroll, click_scroll, NULL, set_lfarrow_size);
		widg->dclick = click_scroll;
		widg->arrowx = WA_LFLINE;
		widg->xarrow = WA_RTLINE;
		widg->xlimit = SL_RANGE;
		widg->slider_type = XAW_HSLIDE;
		if (tp & HSLIDE)
		{
			struct xa_widget *w;
			DIAGS(("Make hslide (lfarrow)"));
			w = make_widget(wind, &stdl_lpage, display_arrow, click_scroll, click_scroll, NULL, NULL);
			w->arrowx = WA_LFPAGE;
			w->xarrow = WA_RTPAGE;
			w->xlimit = SL_RANGE;
			w->slider_type = XAW_HSLIDE;
		}
		else if (old_tp & HSLIDE)
		{
			DIAGS(("Clear hslide (lfarrow)"));
			zwidg(wind, XAW_LFPAGE, keep_stuff);
		}
	}
	else if (old_tp & LFARROW)
	{
		DIAGS(("Clear lfarrow"));
		zwidg(wind, XAW_LFLN, keep_stuff);
		if (old_tp & HSLIDE)
			zwidg(wind, XAW_LFPAGE, keep_stuff);
	}

	if (tp & RTARROW)
	{
		DIAGS(("Make rtarrow"));
		widg = make_widget(wind, &stdl_rscroll, display_rtarrow, click_scroll, click_scroll, NULL, set_rtarrow_size);
		widg->dclick = click_scroll;
		widg->arrowx = WA_RTLINE;
		widg->xarrow = WA_LFLINE;
		widg->limit = SL_RANGE;
		widg->slider_type = XAW_HSLIDE;
		if (tp & HSLIDE)
		{
			struct xa_widget *w;
			DIAGS(("Make hslide (rtarrow)"));
			w = make_widget(wind, &stdl_rpage, display_arrow, click_scroll, click_scroll, NULL, NULL);
			w->arrowx = WA_RTPAGE;
			w->xarrow = WA_LFPAGE;
			w->limit = SL_RANGE;
			w->slider_type = XAW_HSLIDE;
		}
		else if (old_tp & HSLIDE)
		{
			DIAGS(("Clear hslide (rtarrow)"));
			zwidg(wind, XAW_RTPAGE, keep_stuff);
		}
	}
	else if (old_tp & RTARROW)
	{
		DIAGS(("Clear rtarrow"));
		zwidg(wind, XAW_RTLN, keep_stuff);
		if (old_tp & HSLIDE)
			zwidg(wind, XAW_RTPAGE, keep_stuff);
	}

	if (tp & VSLIDE)
	{
		DIAGS(("Make VSLIDE"));
		widg = make_widget(wind, &stdl_vslide, display_vslide, NULL, drag_vslide, NULL, set_vslide_size);
		if (!keep_stuff)
		{
			XA_SLIDER_WIDGET *sl = kmalloc(sizeof(*sl));
			assert(sl);
			widg->stuff = sl;
			sl->length = SL_RANGE;
			widg->flags |= XAWF_STUFFKMALLOC;
		}
	}
	else if (old_tp & VSLIDE)
	{
		DIAGS(("Clear VSLIDE"));
		zwidg(wind, XAW_VSLIDE, keep_stuff);
	}
	
	if (!(tp & VSLIDE) && (tp & (SIZER|UPARROW|DNARROW)))
	{
		DIAGS(("Make unused VSLIDE"));
		widg = make_widget(wind, &stdl_nvslide, display_unused, NULL, NULL, NULL, set_vslide_size);
		utp |= VSLIDE;
	}

	if (tp & HSLIDE)
	{
		DIAGS(("Make HSLIDE"));
		widg = make_widget(wind, &stdl_hslide, display_hslide, NULL, drag_hslide, NULL, NULL);
		if (!keep_stuff)
		{
			XA_SLIDER_WIDGET *sl = kmalloc(sizeof(*sl));
			assert(sl);
			widg->stuff = sl;
			sl->length = SL_RANGE;
			widg->flags |= XAWF_STUFFKMALLOC;
		}
	}
	else if (old_tp & HSLIDE)
	{
		DIAGS(("Clear HSLIDE"));
		zwidg(wind, XAW_HSLIDE, keep_stuff);
	}

	if (tp & XaPOP) /* popups in a window */
	{
		DIAGS(("Make XaPOP"));
		make_widget(wind, &stdl_pop, NULL, NULL, NULL, NULL, NULL);
	}
	else if (old_tp & XaPOP)
	{
		DIAGS(("Clear XaPOP"));
		zwidg(wind, XAW_MENU, keep_stuff);
	}

	if (tp & XaMENU)
	{
		DIAGS(("Make XaMENU"));
		widg = make_widget(wind, &stdl_menu, NULL, NULL, NULL, NULL, set_menu_size);
	}
	else if (old_tp & XaMENU)
	{
		DIAGS(("Clear XaMENU"));
		zwidg(wind, XAW_MENU, keep_stuff);
	}

	if (tp & INFO)
	{
		DIAGS(("Make INFO"));
		widg = make_widget(wind, &stdl_info, display_info, click_title, NULL, NULL, set_info_size);
		if (!keep_stuff)
			/* Give the window a default info line until the client changes it */
			widg->stuff = "Info Bar";
	}
	else if (old_tp & INFO)
	{
		DIAGS(("Clear INFO"));
		zwidg(wind, XAW_INFO, keep_stuff);
	}

	if (tp & NAME)
	{
		DIAGS(("Make NAME"));
		widg = make_widget(wind, &stdl_title, display_title, click_title, drag_title, NULL, set_title_size);
		widg->dclick = dclick_title;
		if (widg->stuff == NULL && !keep_stuff)
			/* Give the window a default title if not already set */
			widg->stuff = "XaAES Window";
	}
	else
	{
		if (old_tp & NAME)
		{
			DIAGS(("Clear NAME"));
			zwidg(wind, XAW_TITLE, keep_stuff);
		}
		if (tp & widglayout[0].tp)
		{
			DIAGS(("Make unused NAME"));
			widg = make_widget(wind, &stdl_notitle, NULL, NULL, NULL, NULL, NULL);
		}
	}

	old_tp &= ~STD_WIDGETS;
	//tp &= STD_WIDGETS;
	wind->active_widgets = (tp | old_tp);
}

static bool
display_toolbar(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	XA_TREE *wt = widg->stuff;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, NULL);

	DIAG((D_form,wind->owner,"display_object_widget(wind=%d), wt=%lx, e.obj=%d, e.pos=%d",
		wind->handle, wt, wt->e.obj, wt->e.pos));

	draw_object_tree(lock, wt, NULL, widg->start, 100, NULL);

	return true;
}

/*
 * HR: Direct display of the toolbar widget; HR 260102: over the rectangle list.
 */
void
redraw_toolbar(enum locks lock, struct xa_window *wind, short item)
{
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	struct xa_rect_list *rl;
	RECT r;

	hidem();
	widg->start = item;

	rl = wind->rect_start;
	while (rl)
	{			
		if (xa_rect_clip(&rl->r, &wind->wa, &r))
		{
			set_clip(&r);
			widg->display(lock, wind, widg);
		}
		rl = rl->next;
	}
	clear_clip();
	showm();
	widg->start = wind->winitem;
}

void
set_toolbar_coords(struct xa_window *wind)
{
	struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);
	XA_WIDGET_LOCATION *loc = &widg->loc;
	OBJECT *form = ((XA_TREE *)widg->stuff)->tree;

	loc->r.x  = wind->wa.x - wind->r.x - (wind->frame <= 0 ? 0 : wind->frame);
	loc->r.y  = wind->wa.y - wind->r.y - (wind->frame <= 0 ? 0 : wind->frame);
	loc->r.w  = form->ob_width;
	loc->r.h  = form->ob_height;
	widg->r = loc->r;
}

void
set_toolbar_handlers(const struct toolbar_handlers *th, struct xa_window *wind, struct xa_widget *widg, struct widget_tree *wt)
{
	if (widg)
	{
		if (th && th->click)
		{
			if ((long)th->click == -1L)
				widg->click	= NULL;
			else
				widg->click	= th->click;
		}
		else
			widg->click	= Click_windowed_form_do;

		if (th && th->dclick)
		{
			if ((long)th->dclick == -1L)
				widg->dclick	= NULL;
			else
				widg->dclick	= th->dclick;
		}
		else
			widg->dclick	= Click_windowed_form_do;

		if (th && th->drag)
		{
			if ((long)th->drag == -1L)
				widg->drag	= NULL;
			else
				widg->drag	= th->drag;
		}
		else
			widg->drag	= Click_windowed_form_do;
		
		if (th && th->display)
		{
			if ((long)th->display == -1L)
				widg->display	= NULL;
			else
				widg->display	= th->display;
		}
		else
			widg->display	= display_toolbar; //display_object_widget;
		
		if (th && th->destruct)
		{
			if ((long)th->destruct == -1L)
				widg->destruct = NULL;
			else
				widg->destruct	= th->destruct;
		}
		else
			widg->destruct	= free_xawidget_resources;
		
		if (th && th->release)
		{
			if ((long)th->release == -1L)
				widg->release = NULL;
			else
				widg->release	= th->release;
		}
		else
			widg->release	= NULL;
	}

	if (wind)
	{
		if (wt && (wt->e.obj >= 0 || obtree_has_default(wt->tree)))
		{
			if (th && th->keypress)
			{
				if ((long)th->keypress == -1L)
					wind->keypress = NULL;
				else
					wind->keypress = th->keypress;
			}
			else
				wind->keypress = Key_form_do;
		}
		else
			wind->keypress = NULL;
	}
	
	if (wt)
	{
		if (th && th->exitform)
		{
			if ((long)th->exitform == -1L)
				wt->exit_form = NULL;
			else
				wt->exit_form = th->exitform;
		}
		else
			wt->exit_form = Exit_form_do;
	}
}
	
/*
 * Attach a toolbar to a window...probably let this be accessed via wind_set one day
 * This is also used to setup windowed form_do sessions()
 */

XA_TREE *
set_toolbar_widget(enum locks lock,
		struct xa_window *wind,
		struct xa_client *owner,
		OBJECT *obtree,
		short edobj,
		short properties,
		const struct toolbar_handlers *th)
{
	XA_TREE *wt;
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	XA_WIDGET_LOCATION loc;

	DIAG((D_wind, wind->owner, "set_toolbar_widget for %d (%s): obtree %lx, %d",
		wind->handle, wind->owner->name, obtree, edobj));

	if (widg->stuff)
	{
		struct widget_tree *owt = widg->stuff;

		set_toolbar_handlers(NULL, NULL, NULL, owt);
		owt->widg = NULL;
		owt->wind = NULL;
		owt->zen  = false;
		owt->links--;
	//	display("set_toolbar_widg: links-- on %lx (links=%d)", owt, owt->links);
	}

	if (owner)
		widg->owner = owner;
	else
		widg->owner = wind->owner;
	
	wt = obtree_to_wt(wind->owner, obtree);
	if (!wt)
		wt = new_widget_tree(wind->owner, obtree);

	assert(wt);

	wt->widg = widg;
	wt->wind = wind;
	wt->zen  = true;
	wt->links++;
	//display("set_toolbar_widg: link++ on %lx (links=%d)", wt, wt->links);

	/*
	 * Ozk: if edobj == -2, we want to look for an editable and place the
	 * cursor there. Used by wdlg_create() atm
	 */
	if (edobj == -2)
		edobj = ob_find_any_flst(obtree, OF_EDITABLE, 0, 0, OS_DISABLED, OF_LASTOB, 0);

	if (!obj_edit(wt, ED_INIT, edobj, 0, -1, false, NULL, NULL, NULL, &edobj))
		obj_edit(wt, ED_INIT, edobj, 0, -1, false, NULL, NULL, NULL, NULL);

	set_toolbar_handlers(th, wind, widg, wt);
	
	/* HR 280801: clicks are now put in the widget struct.
	      NB! use this property only when there is very little difference between the 2 */

	widg->state	= OS_NORMAL;
	widg->stuff	= wt;
	widg->stufftype	= STUFF_IS_WT;
	widg->start	= 0;
	wind->tool	= widg;

	loc.properties = properties | WIP_WACLIP | WIP_ACTIVE;
	loc.relative_type = LT;
	set_toolbar_coords(wind);
	loc.n	 = XAW_TOOLBAR;
	loc.mask = TOOLBAR;
	loc.statusmask = XAWS_SHADED;
	widg->loc	= loc;
	return wt;
}

void
remove_widget(enum locks lock, struct xa_window *wind, int tool)
{
	XA_WIDGET *widg = get_widget(wind, tool);

	DIAG((D_form, NULL, "remove_widget %d: 0x%lx", tool, widg->stuff));

	if (widg->destruct)
		widg->destruct(widg);
	else
	{
		widg->stufftype	= 0;
		widg->stuff   = NULL;
 		widg->display = NULL;
		widg->click   = NULL;
		widg->dclick  = NULL;
		widg->drag    = NULL;
	}
	wind->keypress = NULL;
}

static inline bool
is_page(int f)
{
	switch (f)
	{
	case XAW_LFPAGE:
	case XAW_RTPAGE:
	case XAW_UPPAGE:
	case XAW_DNPAGE:
		return true;
	} 
	return false;
}

/*
 * The mouse is inside the large slider bar
 */
static inline int
is_H_arrow(struct xa_window *w, XA_WIDGET *widg, int click)
{
	/* but are we in the slider itself, the slidable one :-) */
	XA_SLIDER_WIDGET *sl = widg->stuff;
	short x  = widg->ar.x + sl->r.x;
	short x2 = x + sl->r.w;

	if (click > x && click < x2)
		return 0;

	/* outside slider, arrows must be defined */
	if (w->active_widgets & LFARROW)
	{
		/* outside has 2 sides :-) */

		/* up arrow */
		if (sl->position > 0 && click < x)
			/* add to slider index */
			return 1;

		/* dn arrow */
		if (sl->position < SL_RANGE && click > x2)
			return 2;
	}

	/* no active widget, skip */
	return -1;
}

/*
 * same as above, but for vertical slider
 */
static inline int
is_V_arrow(struct xa_window *w, XA_WIDGET *widg, int click)
{
	XA_SLIDER_WIDGET *sl = widg->stuff;
	short y  = widg->ar.y + sl->r.y;
	short y2 = y + sl->r.h;

	if (click > y && click < y2)
		return 0;

 	if (w->active_widgets & UPARROW)
	{
		if (sl->position > 0 && click < y)
			return 1;

		if (sl->position < SL_RANGE && click > y2)
			return 2;
	}

	return -1;
}

/*
 * Process widget clicks, and call the appropriate handler routines
 * This is the main routine for handling window interaction from a users perspective.
 * Each widget has a set of behaviours (display, drag, click, etc) for each of it's widgets.
 * - these can be changed on an individual basis, so the close widget of one window might
 *   call a that sends a 'go back up a directory' message to the desktop, where-as on another
 *   it may just take the GEM default and send a WM_CLOSED message to the application.
 * NOTE: If a widget has no XACB_DISPLAY behaviour, it isnt there.   HR 060601
 */
 
/* HR!!! This is NOT called when widget_active is handled in pending_msg()!!!
            that is making the stuff so weird.
         It is all about the sliders in the list boxes, who are not sending messages to a app,
         but call a XaAES internal routine.
   The problems arose when I removed the active_widget pointers out of the client and window
   structures and made it global.
   Another reason to have the active_widget handling in the kernel. */

bool
checkif_do_widgets(enum locks lock, struct xa_window *w, XA_WIND_ATTR mask, short x, short y, XA_WIDGET **ret)
{
	XA_WIDGET *widg;
	int f;
	short winstatus = w->window_status;
	bool inside = false;

	/* Scan through widgets to find the one we clicked on */
	for (f = 0; f < XA_MAX_WIDGETS; f++)
	{
		RECT r;

		while (is_page(f))
			f++;

		widg = w->widgets + f;

		if (!(winstatus & widg->loc.statusmask) && widg->display)	/* Is the widget in use? */
		{
			//if (    widg->loc.mask         == 0		/* not maskable */
			//    || (widg->loc.mask & mask) == 0)		/* masked */
			if (widg->loc.properties & WIP_ACTIVE)
			{

				if (f != XAW_BORDER)			/* HR 280102: implement border sizing. */
				{					/* Normal widgets */
					rp_2_ap_cs(w, widg, &r);	/* Convert relative coords and window location to absolute screen location */
					inside = m_inside(x, y, &r);
				}
				else
				{
					r = w->r;			/* Inside window and outside border area = border. */
					inside = !m_inside(x, y, &w->ba);
				}
				if (inside)
				{
					if (ret)
						*ret = widg;
					return true;
				}
			}
		}
	}
	if (ret)
		*ret = NULL;
	
	return false;
}


/* HR 161101: possibility to mask out certain widgets. */
int
do_widgets(enum locks lock, struct xa_window *w, XA_WIND_ATTR mask, const struct moose_data *md)
{
	XA_WIDGET *widg;
	int f, clicks;
	short winstatus = w->window_status;

	clicks = md->clicks;
	if (clicks > 2)
		clicks = 2;

	/* Scan through widgets to find the one we clicked on */
	for (f = 0; f < XA_MAX_WIDGETS; f++)
	{
		RECT r;

/* HR 060501: just skip page arrows. These are embedded in the slider background together with the slider itself. */
		while (is_page(f))
			f++;

		widg = w->widgets + f;

		if (!(winstatus & widg->loc.statusmask) && widg->display)					/* Is the widget in use? */
		{
			//if (    widg->loc.mask         == 0		/* not maskable */
			//    || (widg->loc.mask & mask) == 0)		/* masked */
			if (widg->loc.properties & WIP_ACTIVE)
			{
				bool inside;

				if (f != XAW_BORDER)			/* HR 280102: implement border sizing. */
				{					/* Normal widgets */
					rp_2_ap(w, widg, &r);		/* Convert relative coords and window location to absolute screen location */
					widg->ar = r;
					inside = m_inside(md->x, md->y, &r);
				}
				else
				{
					r = w->r;			/* Inside window and outside border area = border. */
					inside = !m_inside(md->x, md->y, &w->ba);
				}

				if (inside)
				{
					bool rtn = false;
					int ax = 0;
		
					widg->x = md->x - r.x; 		/* Mark where the click occurred (relative to the widget) */
					widg->y = md->y - r.y;
					widg->mx = md->x;
					widg->my = md->y;
					widg->s = md->state;		/* HR 280801: we need the state also some time (desktop widget) */
					widg->cs = md->cstate;
					widg->k = md->kstate;
					widg->clicks = clicks;

	/* In this version page arrows are separate widgets,
		they are overlapped by the slider widget, hence this kind of a hack.
		The real solution would be nested widgets and recursive handling. */
	
					if (f == XAW_VSLIDE)
						ax = is_V_arrow(w, widg, md->y);
					else if (f == XAW_HSLIDE)
						ax = is_H_arrow(w, widg, md->x);

					/*
					 * Ozk: If this is a button released click, we just return here..
					 * same if inside a page arrow thats not active.
					*/
					if (ax < 0 || (!md->state && !md->cstate) )
						/* inside a page arrow, but not active */
						return true;

					if (ax)
					{
						widg = w->widgets + f + ax;
						widg->x = md->x - r.x; 			/* Mark where the click occurred (relative to the widget) */
						widg->y = md->y - r.y;
						widg->mx = md->x;
						widg->my = md->y;
						widg->s = md->state;			/* HR 280801: we need the state also some time (desktop widget) */
						widg->cs = md->cstate;
						widg->k = md->kstate;
						widg->clicks = clicks;
						rtn = widg->drag(lock, w, widg, md);	/* we know there is only 1 behaviour for these arrows */
					}
					else /* normal widget */
					{
						short b = md->cstate, rx = md->x, ry = md->y;
	
						/* We don't auto select & pre-display for a menu or toolbar widget */
						if (f != XAW_MENU && f != XAW_TOOLBAR)
							redisplay_widget(lock, w, widg, OS_SELECTED);
		

						/*
						 * Check if the widget has a dragger function if button still pressed
						*/
						if (b && widg->drag) //(widget_active.cb) && widg->drag) 
						{
							/* If the mouse button is still down
							 * do a drag (if the widget has a drag
							 * behaviour) */
							rtn = widg->drag(lock, w, widg, md);
						}
						else
						{
						/*
						 * otherwise, process as a mouse click(s)
						*/
							if (b)
							{
								/*
								 * If button is still being held down, hang around
								 * waiting for button release. Animate the widget clicked as mouse
								 * moves on/off...
								*/
								short tx = rx, ty = ry; //tx = widget_active.nx, ty = widget_active.ny;
								bool ins = 1;
								check_mouse(w->owner, &b, 0, 0);
								S.wm_count++;
								while (b)
								{
									/* Wait for the mouse to be released */
									wait_mouse(w->owner, &b, &rx, &ry);

									if (tx != rx || ty != ry)
									{
										if (m_inside(rx, ry, &r))
										{
											if (!ins)
											{
												redisplay_widget(lock, w, widg, OS_SELECTED);
												ins = 1;
											}
										}
										else if (ins)
										{
												redisplay_widget(lock, w, widg, OS_NORMAL);
											ins = 0;
										}
										tx = rx;
										ty = ry;
									}
								}
								S.wm_count--;
							}
							if (m_inside(rx, ry, &r))
							{
								/* Ozk: added check for number of clicks and call the dclick function if apropriate */
								if (md->clicks == 1) //widget_active.clicks == 1)
								{
									if (widg->click)
										rtn = widg->click(lock, w, widg, md);
									else
										rtn = true;
								}
								else if (md->clicks == 2) //widget_active.clicks == 2)
								{
									if (widg->dclick)
										rtn = widg->dclick(lock, w, widg, md);
									else
										rtn = true;
								}
							}
							else
								rtn = true;		/* HR 060601: released outside widget: reset its state. */
	
							if (w->winob)		/* HR 060601: a little bit of hack, until checking widget_active
							                                  is moved from pending_msg() to the kernel. */
								cancel_widget_active(w, 5);
						}
					}
	
					if (rtn)	/* If the widget click/drag function returned true we reset the state of the widget */
					{
						DIAG((D_button, NULL, "Deselect widget"));
						if (f != XAW_MENU && f != XAW_TOOLBAR)
							redisplay_widget(lock, w, widg, OS_NORMAL);	/* Flag the widget as de-selected */
					}
					else if (w == root_window && f == XAW_TOOLBAR)		/* HR: 280801 a little bit special. */
						return false;		/* pass click to desktop owner */

					/* click devoured by widget */
					return true;
				} /*if m_inside */
			} /* if not masked */
		} /* if there */
	} /* for f */

	/* Button click can be passed on to applications as we didn't use it for a widget */
	return false;
}

void
redisplay_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, int state)
{
	if (widg->display)
	{
		widg->state = state;

		/* No rectangle list. */
		if (wind->nolist)
		{
			hidem();
			/* Not on top, but visible */
			widg->display(lock, wind, widg);
			showm();
		}
		else
			/* Display the selected widget */
			display_widget(lock, wind, widg, NULL);
	}
}

void
do_active_widget(enum locks lock, struct xa_client *client)
{
	if (widget_active.widg)
	{
		if (widget_active.wind->owner == client)
		{
			struct xa_window *wind = widget_active.wind;
			XA_WIDGET *widg = widget_active.widg;
			int rtn;

			/* Call the pending action */
			rtn = (*widget_active.action)(lock, wind, widg, &widget_active.m);

			/* HR: 050601: if the pending widget is canceled, its state is undefined!!!!!! */
			/* If the widget click/drag function returned true we reset the state of the widget */
			if (rtn)
			{
				/* Flag the widget as de-selected */
				redisplay_widget(lock, wind, widg, OS_NORMAL);
				set_winmouse();
			}
		}
	}
}

void
set_winmouse(void)
{
	short x, y;
	struct xa_window *wind;

	check_mouse(NULL, NULL, &x, &y);

	wind = find_window(0, x, y);

	wind_mshape(wind, x, y);
}

short
wind_mshape(struct xa_window *wind, short x, short y)
{
	short shape = -1;
	short status = wind->window_status;
	struct xa_client *wo = wind == root_window ? get_desktop()->owner : wind->owner;
	XA_WIDGET *widg;
	RECT r;

	if (wind)
	{
		if (!(wo->status & CS_EXITING))
		{
			if (cfg.widg_auto_highlight)
			{
				struct xa_window *rwind = NULL;
				struct xa_widget *hwidg, *rwidg = NULL;

				checkif_do_widgets(0, wind, 0, x, y, &hwidg);
		
				if (hwidg && !hwidg->owner && hwidg->display)
				{
					if (wind != C.hover_wind || hwidg != C.hover_widg)
					{
						rwind = C.hover_wind;
						rwidg = C.hover_widg;
						redisplay_widget(0, wind, hwidg, OS_SELECTED);
						C.hover_wind = wind;
						C.hover_widg = hwidg;
					}
				}
				else if ((rwind = C.hover_wind))
				{
					rwidg = C.hover_widg;
					C.hover_wind = NULL, C.hover_widg = NULL;
				}
		
				if (rwind)
					redisplay_widget(0, rwind, rwidg, OS_NORMAL);
			}
		
			if (wind->active_widgets & (SIZER|BORDER))
			{
				widg = wind->widgets + XAW_RESIZE;
				if ( /*wind->frame > 0 && */
				    (wind->active_widgets & BORDER) &&
				   !(wind->widgets[XAW_BORDER].loc.statusmask & status) &&
				    (!m_inside(x, y, &wind->ba)))
				{
					r = wind->r;
					shape = border_mouse[compass(16, x, y, r)];
				}
				else if (wind->active_widgets & SIZER)
				{
					widg = wind->widgets + XAW_RESIZE;
					if (!(widg->loc.statusmask & status))
					{
						rp_2_ap_cs(wind, widg, &r);
						if (m_inside(x, y, &r))
							shape = border_mouse[SE];
					}
				}
			}
			if (shape != -1)
			{
				if (C.aesmouse == -1 || (C.aesmouse != -1 && C.aesmouse != shape))
					graf_mouse(shape, NULL, NULL, true);
			}
			else
			{
				if (C.aesmouse != -1)
					graf_mouse(-1, NULL, NULL, true);
				if (C.mouse_form != wind->owner->mouse_form)
					graf_mouse(wo->mouse, wo->mouse_form, wo, false);
			}
		}
		else
		{
			C.hover_wind = NULL;
			C.hover_widg = NULL;
			graf_mouse(ARROW, NULL, NULL, false);
		}	
	}
	else if (C.aesmouse != -1)
		graf_mouse(-1, NULL, NULL, true);

	return shape;
}

