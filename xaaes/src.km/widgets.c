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

static OBJECT *def_widgets;

//#if GENERATE_DIAGS
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
//#endif

/*
 * WINDOW WIDGET SET HANDLING ROUTINES
 * This module handles the behaviour of the window widgets.
 */
inline int
pix_to_sl(long p, int s)
{
	return (p * SL_RANGE) / s;
}

inline int
sl_to_pix(long s, int p)
{
	return (s * p) / SL_RANGE;
}

void
XA_slider(struct xa_window *w, int which, int total, int visible, int start)
{
	XA_WIDGET *widg = get_widget(w, which);

	if (w->active_widgets & widg->loc.mask) //widg->loc.mask)
	{
		XA_SLIDER_WIDGET *sl = widg->stuff;

		if (visible < total)
			sl->length = pix_to_sl(visible, total);
		else
			sl->length = SL_RANGE;

		if (total - visible < 0 || start == 0)
			sl->position = 0;
		else
		{
			if (start + visible >= total)
				sl->position = SL_RANGE;
			else
				sl->position = pix_to_sl(start, total - visible);
		}
	}
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
	graf_mouse(wind->owner->mouse, wind->owner->mouse_form, false);
}


#ifndef __GNUC__
/* check slider value */
int
bound_sl(int p)
{
	return p < 0 ? 0 : (p > SL_RANGE ? SL_RANGE : p);
}
#endif

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

	switch (widg->loc.relative_type)
	{
	case LT:
		r->x += rtx;
		r->y += rty;
		break;
	case LB:
		r->x += rtx;
		r->y += (wh - r->h - rty);
		break;
	case RT:
		r->x += (ww - r->w - rtx);
		r->y += rty;
		break;
	case RB:
		r->x += (ww - r->w - rtx);
		r->y += (wh - r->h - rty);
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
	}

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
}

/* Ozk:
 * Context safe rp_2_ap - does not return an object tree
 * Used by AESSYS, checkif_do_widgets(), when determining
 * which widget is under the mouse.
 */
void
rp_2_ap_cs(struct xa_window *wind, XA_WIDGET *widg, RECT *r)
{
	RECT dr;
	short rtx, rty, ww, wh;
	int frame = wind->frame;

	DIAG((D_form, NULL, "rp_2_ap: type=%s, widg=%lx, wt=%lx, obtree=%lx",
		t_widg[widg->type], widg, widg->stuff,
		widg->type == XAW_TOOLBAR ? (long)((XA_TREE *)widg->stuff)->tree : -1 ));

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

	switch (widg->loc.relative_type)
	{
	case LT:
		r->x += rtx;
		r->y += rty;
		break;
	case LB:
		r->x += rtx;
		r->y += (wh - r->h - rty);
		break;
	case RT:
		r->x += (ww - r->w - rtx);
		r->y += rty;
		break;
	case RB:
		r->x += (ww - r->w - rtx);
		r->y += (wh - r->h - rty);
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
	}
}

XA_TREE *
obtree_to_wt(struct xa_client *client, OBJECT *obtree)
{
	struct xa_window *wind = client->fmd.wind;
	XA_TREE *wt = NULL;

	DIAGS(("obtree_to_wt: look for wt with obtree=%lx for %s",
		obtree, client->name));

	if (client->fmd.wt && obtree == client->fmd.wt->tree)
	{
		DIAGS((" -- found in client->wt"));
		wt = client->fmd.wt;
	}
	else if (wind)
	{
		XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);

		if (obtree == ((XA_TREE *)widg->stuff)->tree)
		{
			DIAGS((" -- found in XAW_TOOLBAR fmd wind %d - %s",
				wind->handle, client->name));
			wt = widg->stuff;
		}
	}
	else
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

		obtree->ob_x = sx;
		obtree->ob_y = sy;

		new->next = client->wtlist;
		client->wtlist = new;
		
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
		DIAGS(("  --- freed obtree %lx", wt->tree));
		kfree(wt->tree);
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

void
remove_wt(XA_TREE *wt)
{
	remove_from_wtlist(wt);
	free_wt(wt);
}


void
display_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg)
{
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
}
static void
CE_redraw_menu(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		struct xa_client *mc;
		struct xa_widget *widg = get_menu_widg();

		mc = ((XA_TREE *)widg->stuff)->owner;
		if (ce->client == mc )
		{
			DIAGS(("CE_redraw_menu: for %s", ce->client->name));
			display_widget(lock, root_window, widg);
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

	DIAGS(("redraw_menu: yeehaa"));

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
		DIAGS(("Display MENU (same client) for %s", rc->name));
		display_widget(lock, root_window, widg);
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
calc_work_area(struct xa_window *wi)
{
	RECT r = wi->rc, *rt;
	XA_SLIDER_WIDGET *sl;
	XA_WIDGET *widg;
	XA_WIND_ATTR k = wi->active_widgets;
	int thin = wi->thinwork;
	int frame = wi->frame;
	int bottom = 0;
	int right = 0;
	int tl_margin, br_margin, wa_margins;
	int winside /* , hinside */;

	wi->wa = r;

	/*
	 * Any widgets at all?
	 */
	if (!(k & V_WIDG))
	{
		/* XXX
		 * Ozk: Why isn't frame used to calculate worarea here??
		 * Investigate it later!
		 */
		wi->ba = r;
		if (frame >= 0)
		{
			wi->wa.x += frame; //1;
			wi->wa.y += frame; //1;
			wi->wa.w -= frame + frame + SHADOW_OFFSET; //2 + SHADOW_OFFSET;
			wi->wa.h -= frame + frame + SHADOW_OFFSET; //2 + SHADOW_OFFSET;
		}
	}
	else
	{
		/* a colour work area frame is larger to allow for the
		 * fancy borders :-) unless thinwork has been specified
		 * a color work area frame consists of a grey, a light and
		 * a dark line.
		 */

		/* in color aligning is on the top & left light line of tha wa */
		//tl_margin = (MONO || thin) ? 1 : 2;
		tl_margin = (MONO || thin) ? 0 : 2;

		/* The visual thing :-) */
		//br_margin = (MONO || thin) ? 1 : 3;
		br_margin = (MONO || thin) ? 0 : 3;

		wa_margins = tl_margin + br_margin;

		/* This is the largest work area possible */
		if (frame >= 0)
		{
			wi->wa.x +=    frame + tl_margin;
			wi->wa.y +=    frame + tl_margin;
			wi->wa.w -= 2 * frame + wa_margins + SHADOW_OFFSET;
			wi->wa.h -= 2 * frame + wa_margins + SHADOW_OFFSET;
		}

		/* For use by border sizing. */
		wi->ba = wi->wa;

		/*
		 * calculate correct width for windows title line
		 */
		winside = wi->wa.w + wa_margins;
		/* hinside = wi->wa.h + wa_margins; */

		if (k & (CLOSER|NAME|MOVER|ICONIFIER|FULLER))	/* top bar */
			wi->wa.y += cfg.widg_h + 2,
			wi->wa.h -= cfg.widg_h + 2;

		rt = &get_widget(wi, XAW_TITLE)->r;
		rt->w = winside;

		if (k & CLOSER)
		{
			/* + space between name and closer
			 *   name has a inside border
			 */
			rt->w -= cfg.widg_w + 1;
		}

		if (k & (ICONIFIER|FULLER|HIDE))
		{
			/* Due to pushbutton behaviour in color, in mono its a rectangle. */
			rt->w -=  MONO ? 1 : 2;
			if (k & FULLER)
				rt->w -= cfg.widg_w;
			if (k & ICONIFIER)
				rt->w -= cfg.widg_w;
			if (k & HIDE)
				rt->w -= cfg.widg_w;
		}

		if (!(wi->window_status & XAWS_ICONIFIED))
		{
			/* fatter topbar */
			if (k & INFO)
			{
				wi->wa.y += cfg.widg_h;
				wi->wa.h -= cfg.widg_h;
			}

			if (k & XaMENU)
			{
				/* menu is a standard widget!
				 * 
				 * Now I can specify the desktop window w/o dummy name. :-)
				 * the +1 is for the menu_line
				 */
				wi->wa.y += MENU_H + 1;
				wi->wa.h -= MENU_H + 1;
			}

			if (k & (UPARROW|DNARROW|VSLIDE))
			{
				/* right bar */
				right = 1;
			}

			if (k & (LFARROW|RTARROW|HSLIDE))
			{
				/* bottom bar */
				wi->wa.h -= cfg.widg_h + WASP; /* + spacer */
				bottom = 1;
			}

			if (right || ((k & SIZER) && !(right || bottom)))
				/* sizer only retain right bar */
				wi->wa.w-= cfg.widg_w + WASP; /* + spacer */

			/* Update info bar */
			if (k & INFO)
				/* full width inside standard borders */
				get_widget(wi, XAW_INFO)->r.w = winside;

			if (k & XaMENU)
				get_widget(wi, XAW_MENU)->r.w = winside;	


			/* Sliders are completely oriented on the work area */

			/* Update horizontal slider */
			if (k & HSLIDE)
			{
				widg = get_widget(wi, XAW_HSLIDE);
				sl = widg->stuff;
				/* initializes slider draw */
				sl->r.w = 0;

				rt = &widg->r;
				rt->w = wi->wa.w + wa_margins;

				if (k & LFARROW)
					/* includes RTARROW */
					rt->w -= 2*cfg.widg_w;

				if ((k & SIZER) && !right)
					rt->w -= cfg.widg_h;
			}

			/* Update vertical slider */
			if (k & VSLIDE)
			{
				widg = get_widget(wi, XAW_VSLIDE);
				sl = widg->stuff;
				/* initializes slider draw */
				sl->r.h = 0;

				rt = &widg->r;
				rt->h = wi->wa.h + wa_margins;

				if (k & DNARROW)
					/* includes UPARROW */
					rt->h -= 2 * cfg.widg_h;

				if ((k & SIZER) && !bottom)
					rt->h -= cfg.widg_h;
			}
		}
	}

	if (wi->wa.w < 0 || wi->wa.h < 0)
		wi->wa.w = wi->wa.h = 0;
	
	/* border displacement */
	wi->bd.x = wi->rc.x - wi->wa.x;
	wi->bd.y = wi->rc.y - wi->wa.y;
	wi->bd.w = wi->rc.w - wi->wa.w;
	wi->bd.h = wi->rc.h - wi->wa.h;

	/* Add bd to toolbar->r to get window rectangle for create_window
	 * Anyhow, always convenient, especially when snapping the workarea.
	 */

	DIAG((D_widg, wi->owner, "workarea %d: %d/%d,%d/%d", wi->handle, wi->wa));
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
				//remove_wt(wt);
				wt->widg = NULL;
				wt->wind = NULL;
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
/*             defaults              index        mask	     status			rsc           top     destruct */
/*							      mask					 */
stdl_close   = {LT, { 0, 0, 1, 1 },  XAW_CLOSE,   CLOSER,    XAWS_ICONIFIED,				WIDG_CLOSER,  false, free_xawidget_resources },
stdl_full    = {RT, { 0, 0, 1, 1 },  XAW_FULL,    FULLER,    XAWS_ICONIFIED,				WIDG_FULL,    false, free_xawidget_resources },
stdl_iconify = {RT, { 0, 0, 1, 1 },  XAW_ICONIFY, ICONIFIER, 0,				WIDG_ICONIFY, false, free_xawidget_resources },
stdl_hide    = {RT, { 0, 0, 1, 1 },  XAW_HIDE,    HIDER,     XAWS_ICONIFIED,				WIDG_HIDE,    false, free_xawidget_resources },
stdl_title   = {LT, { 0, 0, 1, 1 },  XAW_TITLE,   NAME,      0,				0,            false, free_xawidget_resources },
stdl_notitle = {LT, { 0, 0, 1, 1 },  XAW_TITLE,   0,         0,				0,            false, free_xawidget_resources },
stdl_resize  = {RB, { 0, 0, 1, 1 },  XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,		WIDG_SIZE,    false, free_xawidget_resources },
stdl_uscroll = {RT, { 0, 0, 1, 1 },  XAW_UPLN,    UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,		WIDG_UP,      false, free_xawidget_resources },
stdl_upage   = {RT, { 0, 1, 1, 1 },  XAW_UPPAGE,  UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_vslide  = {RT, { 0, 1, 1, 1 },  XAW_VSLIDE,  VSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_dpage   = {RT, { 0, 1, 1, 1 },  XAW_DNPAGE,  DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_dscroll = {RB, { 0, 1, 1, 1 },  XAW_DNLN,    DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,		WIDG_DOWN,    false, free_xawidget_resources },
stdl_lscroll = {LB, { 0, 0, 1, 1 },  XAW_LFLN,    LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,		WIDG_LEFT,    false, free_xawidget_resources },
stdl_lpage   = {LB, { 1, 0, 1, 1 },  XAW_LFPAGE,  LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_hslide  = {LB, { 1, 0, 1, 1 },  XAW_HSLIDE,  HSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_rpage   = {LB, { 1, 0, 1, 1 },  XAW_RTPAGE,  RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_rscroll = {RB, { 1, 0, 1, 1 },  XAW_RTLN,    RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,		WIDG_RIGHT,   false, free_xawidget_resources },
stdl_info    = {LT, { 0, 0, 1, 1 },  XAW_INFO,    INFO,      XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_menu    = {LT, { 0, 0, 0, 0 },  XAW_MENU,    XaMENU,    XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_pop     = {LT, { 0, 0, 0, 0 },  XAW_MENU,    XaPOP,     XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources },
stdl_border  = {0,  { 0, 0, 0, 0 },  XAW_BORDER,  0,         XAWS_SHADED|XAWS_ICONIFIED,		0,            false, free_xawidget_resources }
;

static void
set_widget_coords(struct xa_widget *w)
{
	XA_WIDGET_LOCATION *l = &w->loc;

	w->r.x = l->r.x * cfg.widg_w;
	w->r.y = l->r.y * cfg.widg_h;
	w->r.w = l->r.w * cfg.widg_w;
	w->r.h = l->r.h * cfg.widg_h;
	
	if (l->rsc_index && (cfg.widg_dw || cfg.widg_dh))
	{
		switch(l->relative_type)
		{
		case LT:
			w->r.x -= cfg.widg_dw;
			w->r.y -= cfg.widg_dh;
			break;
		case RT:
			w->r.x += cfg.widg_dw;
			w->r.y -= cfg.widg_dh;
			break;
		case LB:
			w->r.x -= cfg.widg_dw;
			w->r.y += cfg.widg_dh;
			break;
		case RB:
			w->r.x += cfg.widg_dw;
			w->r.y += cfg.widg_dh;
			break;
		default:;
		}
	}
}
static XA_WIDGET *
make_widget(struct xa_window *wind, const XA_WIDGET_LOCATION *loc,
	    DisplayWidget *disp, WidgetBehaviour *click, WidgetBehaviour *drag)
{
	XA_WIDGET *widg = get_widget(wind, loc->n);

	widg->display	= disp;
	widg->click	= click;
	widg->drag	= drag;
	widg->loc	= *loc;
	widg->type	= loc->n;
	widg->destruct	= loc->destruct;

	set_widget_coords(widg);

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

static bool
display_def_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	RECT r; int i = widg->loc.rsc_index;

	rp_2_ap(wind, widg, &r);

	if (widg->state & OS_SELECTED)
		def_widgets[i].ob_state |= OS_SELECTED;
	else
		def_widgets[i].ob_state &= ~OS_SELECTED;

/*
DIAG((D_widg, wind->owner, "display_def_widget %d: state %d, global clip: %d:%d,%d:%d r: %d:%d/%d:%d",
				wind->handle, (widg->state & SELECTED) != 0,
				C.global_clip[0],C.global_clip[1],C.global_clip[2],C.global_clip[3],
				r.x,r.y,r.x+r.w-1,r.y+r.h-1
				));
*/
	display_object(lock, &wind->widg_info, i, r.x, r.y, 0);

	return true;
}

/*======================================================
	TITLE WIDGET BEHAVIOUR
========================================================*/
static bool
display_title(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct options *o = &wind->owner->options;
	char temp[256];
	char tn[256];
	RECT r;
	int effect = 0;
	bool dial = (wind->dial & (created_for_FORM_DO|created_for_FMD_START)) != 0;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &r);

	wr_mode(MD_TRANS);

#if DISPLAY_LOGO_IN_TITLE
	/* It ressembles too much a closer */
	if (widg->stat & SELECTED)
		def_widgets[WIDG_LOGO].ob_state |= OS_SELECTED;
	else
		def_widgets[WIDG_LOGO].ob_state &= ~OS_SELECTED;

	wind->wt.tree = def_widgets;
	display_object(lock, wind->wt, WIDG_LOGO, r.x, r.y, 1);

	/* tiny pixel correction (better spacing) */
	r.x += cfg.widg_w + 2;
	r.w -= cfg.widg_w + 2;
#endif

	if MONO
	{
		f_color(G_WHITE);
		t_color(G_BLACK);

		if (is_topped(wind))
			/* Highlight the title bar of the top window */
			effect = cfg.topname;
		else
			effect = cfg.backname;

		/* with inside border */
		p_gbar(0, &r);
	}
	else
	{
		if (is_topped(wind))
			/* Highlight the title bar of the top window */
			t_color(G_BLACK);
		else
			t_color(G_WHITE);

		/* no move, no 3D */
		if (wind->active_widgets & MOVER)
			d3_pushbutton(-2, &r, NULL, widg->state, 0, 0);
		else
		{
			f_color(screen.dial_colours.bg_col);
			l_color(screen.dial_colours.shadow_col);
			p_gbar(0, &r);
		}
	}

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

	cramped_name(tn, temp, r.w/screen.c_max_w);

	/* avoid inner border. */
	if (*temp != ' ')
		++r.x;

	if (dial)
		effect |= ITALIC;

	vst_effects(C.vh, effect);
	r.y += (r.h - screen.c_max_h) / 2;
	v_gtext(C.vh, r.x, r.y, temp);

	/* normal */
	vst_effects(C.vh, 0);

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
			graf_mouse(XACRS_MOVER, NULL, false);
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

			lock_screen(wind->owner, 0, 0, 1234);
			drag_box(wind->owner, r, &bound, rect_dist(wind->owner, &r, &d), &r);
			unlock_screen(wind->owner, 1235);

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
				rect_dist(wind->owner, &r, &d);
			}

			//if (widget_active.m.cstate)	/*(mb)*/
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
		if ((widg->k & 3) && (!((wind->active_widgets & STORE_BACK) !=0 ) || !((wind->active_widgets & BACKDROP) == 0)))
		{
			if (wind->send_message)
				wind->send_message(lock, wind, NULL, AMQ_NORM,
						   WM_BOTTOMED, 0, 0, wind->handle, 0, 0, 0, 0);
			else
				bottom_window(lock, wind);
		}
		else
		{
			/* if not on top or on top and not focus */
			//if ( wind != window_list ||
			//    (wind == window_list && wind != C.focus))
			if (!is_topped(wind))
			{
				if (wind->send_message)
					wind->send_message(lock, wind, NULL, AMQ_NORM,
							   WM_TOPPED, 0, 0, wind->handle,
							   0, 0, 0, 0);
				else
					/* Just top these windows, they can handle it... */
					/* Top the window */
					top_window(lock, wind, NULL);
			}
			/* If window is already top, then send it to the back */

			/* Ozk: Always bottom iconified windows... */
			else if ((wind->window_status & XAWS_ICONIFIED))
			{
				if (wind->send_message)
					wind->send_message(lock, wind, NULL, AMQ_NORM, WM_BOTTOMED, 0, 0, wind->handle, 0,0,0,0);
			}
			else if (!((wind->active_widgets & STORE_BACK) != 0		/* Don't bottom STORE_BACK windows */
				    || (wind->active_widgets & BACKDROP) == 0) )	/* Don/t bottom NO BACKDROP windows */
			{
					if (wind->send_message)
						wind->send_message(lock, wind, NULL, AMQ_NORM,
								   WM_BOTTOMED, 0, 0, wind->handle,
								   0, 0, 0, 0);
					else
						/* Just bottom these windows, they can handle it... */
						bottom_window(lock, wind);
			}
		}
	}
	else if (md->state == MBS_RIGHT)
	{
		if ((wind->window_status & XAWS_SHADED))
		{
			if (wind->send_message)
			{
				DIAGS(("Click_title: unshading window %d for %s",
					wind->handle, wind->owner->name));

				wind->send_message(lock, wind, NULL, AMQ_CRITICAL,
					WM_UNSHADED, 0, 0,wind->handle, 0, 0, 0, 0);

				move_window(lock, wind, ~(XAWS_SHADED|XAWS_ZWSHADED), wind->rc.x, wind->rc.y, wind->rc.w, wind->rc.h);
			}
		}
		else
		{
			if (wind->send_message)
			{
				DIAGS(("Click_title: shading window %d for %s",
					wind->handle, wind->owner->name));

				move_window(lock, wind, XAWS_SHADED, wind->rc.x, wind->rc.y, wind->rc.w, wind->rc.h);
				wind->send_message(lock, wind, NULL, AMQ_CRITICAL,
					WM_SHADED, 0, 0, wind->handle, 0,0,0,0);
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
			wind->send_message(lock, wind, NULL, AMQ_NORM,
					   WM_UNICONIFY, 0, 0, wind->handle,
					   wind->pr.x, wind->pr.y, wind->pr.w, wind->pr.h);
		}
		else if (wind->active_widgets & FULLER)
		{
			/* Ozk 100503: Double click on title now sends WM_FULLED,
			 * as N.AES does it */
			wind->send_message(lock, wind, NULL, AMQ_NORM,
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
	char t[160];
	RECT r;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &r);

	/* in mono just leave unbordered */
	if (!MONO)
	{
		tl_hook(0, &r, screen.dial_colours.lit_col);
		br_hook(0, &r, screen.dial_colours.shadow_col);
	}
	t_color(G_BLACK);

	r.y += (r.h-screen.c_max_h)/2;
	v_gtext(C.vh, r.x + 4, r.y, clipped_name(widg->stuff, t, (r.w/screen.c_max_w)-1));

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
		wind->send_message(lock, wind, NULL, AMQ_NORM,
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
		wind->send_message(lock, wind, NULL, AMQ_NORM,
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

			wind->send_message(lock, wind, NULL, AMQ_NORM,
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
				wind->send_message(lock|winlist, wind, NULL, AMQ_NORM,
					   WM_ICONIFY, 0, 0, wind->handle,
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
size_window(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, bool sizer, WidgetBehaviour next)
{
	bool move, size;
	RECT r = wind->r, d;
	
	bool use_max = (wind->active_widgets&USE_MAX) != 0;

	if (   widg->s == 2
	    || wind->send_message == NULL
	    || wind->owner->options.nolive)
	{
		int xy = sizer ? SE : compass(16, widg->mx, widg->my, r);

		/* Always have a nice consistent SIZER when resizing a window */
		graf_mouse(border_mouse[xy], NULL, false);

		lock_screen(wind->owner, 0, 0, 1234);
		rubber_box(wind->owner, xy,
		           r,
			   rect_dist(wind->owner, &r, &d),
			   6 * cfg.widg_w,
			   6 * cfg.widg_h,
			   use_max ? wind->max.w : root_window->wa.w,
			   use_max ? wind->max.h : root_window->wa.h,
			   &r);
		unlock_screen(wind->owner, 1234);

		move = r.x != wind->r.x || r.y != wind->r.y,
		size = r.w != wind->r.w || r.h != wind->r.h;

		if (move || size)
		{
			if (move)
				send_moved(lock, wind, AMQ_NORM, &r);
			if (size)
				send_sized(lock, wind, AMQ_NORM, &r);
		}
	}
	else if (widget_active.m.cstate)
	{
		short pmx, pmy /*, mx, my, mb*/;
		COMPASS xy;

		/* need to do this anyhow, for mb */
		/* vq_mouse(C.vh, &mb, &pmx, &pmy); */

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
			xy  = sizer ? SE : compass(10, pmx, pmy, r);
			rect_dist(wind->owner, &r, &d);
		}

		/* Drag border */
		if (widget_active.m.cstate)	/*(mb)*/
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
				graf_mouse(border_mouse[xy], NULL, false);
			}

			/* Has the mouse moved? */
			if (widget_active.m.x != pmx || widget_active.m.y != pmy)
			{
				r = widen_rectangle(xy, widget_active.m.x, widget_active.m.y, r, &d);	/*(xy, mx, my, r, &d);*/

				check_wh_cp(&r, xy,
					    6 * cfg.widg_w,
					    6 * cfg.widg_h,
					    use_max ? wind->max.w : root_window->wa.w,
					    use_max ? wind->max.h : root_window->wa.h);
			}

			move = r.x != wind->r.x || r.y != wind->r.y,
			size = r.w != wind->r.w || r.h != wind->r.h;
			
			if (move || size)
			{
				if (move)
					send_moved(lock, wind, AMQ_NORM, &r);
				if (size)
					send_sized(lock, wind, AMQ_NORM, &r);
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
	return size_window(lock, wind, widg, true, drag_resize);
}
static inline bool
drag_border(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	return size_window(lock, wind, widg, false, drag_border);
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

static struct xa_client *do_widget_repeat_client = NULL;
static enum locks do_widget_repeat_lock;

void
do_widget_repeat(void)
{
	if (do_widget_repeat_client)
	{
		do_active_widget(do_widget_repeat_lock, do_widget_repeat_client);

		/*
		 * Ozk: The action functions clear the active widget by clearing the 
		 * widg member when the mouse button is released, so we check that
		 * instead of the button state here.
		 */
		if (!widget_active.widg)
		{
			aessys_timeout = 0;
			do_widget_repeat_client = NULL;
		}
	}
}

static void
set_widget_repeat(enum locks lock, struct xa_window *wind)
{
	if (!do_widget_repeat_client)
	{
		aessys_timeout = 1;
		do_widget_repeat_client = wind->owner;
		do_widget_repeat_lock = lock;

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
			RECT r;

			/* XXX 
			 * Center sliders at the clicked position.
			 * Wait for mousebutton release
			 */
			if (mb == md->cstate)
			{
				graf_mouse(XACRS_POINTSLIDE, NULL, false);
				check_mouse(wind->owner, &mb, 0, 0);

				S.wm_count++;
				while (mb == md->cstate)
					wait_mouse(wind->owner, &mb, &mx, &my);
				S.wm_count--;
			}

			/* Convert relative coords and window location to absolute screen location */
			rp_2_ap(wind, slider, &r);

			inside = m_inside(mx, my, &r);
			if (inside)
			{
				XA_SLIDER_WIDGET *ssl = slider->stuff;
				int offs;

				rp_2_ap(wind, widg, &r);

				widg->x = mx - r.x;
				widg->y = my - r.y;
				widg->mx = mx;
				widg->my = my;
				widg->cs = mb;
				widg->k = md->kstate;
				widg->clicks = 0;

				if (widg->slider_type == XAW_VSLIDE)
					offs = bound_sl(pix_to_sl(widg->y - (ssl->r.h >> 1),
								  slider->r.h - ssl->r.h));
				else
					offs = bound_sl(pix_to_sl(widg->x - (ssl->r.w >> 1),
								  slider->r.w - ssl->r.w));

				if (wind->send_message)
					wind->send_message(lock, wind, NULL, AMQ_NORM,
							   widg->slider_type == XAW_VSLIDE ? WM_VSLID : WM_HSLID,
							   0, 0, wind->handle, offs, 0, 0, 0);
			}

			cancel_widget_active(wind, 2);
			return true;
		}

		if (md->clicks > 1)
		{
			if (reverse)
			{
				wind->send_message(lock, wind, NULL, AMQ_NORM,
						   widg->slider_type == XAW_VSLIDE ? WM_VSLID : WM_HSLID,
						   0, 0, wind->handle, widg->xlimit, 0, 0, 0);
			}
			else
			{
				wind->send_message(lock, wind, NULL, AMQ_NORM,
						   widg->slider_type == XAW_VSLIDE ? WM_VSLID : WM_HSLID, 
						   0, 0, wind->handle, widg->limit, 0, 0, 0);
			}
		}
		else
		{
			wind->send_message(lock, wind, NULL, AMQ_NORM,
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
#if 0

			if (md->cstate)
			{
				check_mouse(wind->owner, &mb, &mx, &my);

				if (mb)
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
#endif
		}
	}

	cancel_widget_active(wind, 2);
	return true;
}

static void
slider_back(RECT *cl)
{
	f_color(screen.dial_colours.bg_col);
	f_interior(FIS_SOLID);
	gbar(0, cl);
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

/* Display now includes the background */
bool
display_vslide(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	RECT r, cl;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &r);

	if (sl->length >= SL_RANGE)
	{
		sl->r = r;
		slider_back(&r);
		return true;
	}

	wr_mode(MD_REPLACE);

	len = sl_to_pix(r.h, sl->length);
	if (len < cfg.widg_h - 3)
		/* not too small please */
		len = cfg.widg_h - 3;

	offs = r.y + sl_to_pix(r.h - len, sl->position);

	if (offs < r.y)
		offs = r.y;
	if (offs + len > r.y + r.h)
		len = r.y + r.h - offs;

	if (   sl->r.h == 0		/* if initial, or after sizing or off top */
	    || wind != window_list)
	{
		sl->r = r;
		slider_back(&r);	/* whole background */
	}
	else
	{				/* should really use xa_rc_intersect */
		cl = sl->r;		/* remove old slider or parts */
		if (   sl->r.h == len	/* parts work only when physical length didnt change */
		    && !(offs + len < sl->r.y || offs > sl->r.y + sl->r.h)) /* if overlap */ 		
		{
			if (sl->r.y < offs)		/* down */
				cl.h = offs - sl->r.y;
			else				/* up */
			{
				cl.y = offs + len;
				cl.h = sl->r.y - offs;
			}
		}
		if (cl.h)
			slider_back(&cl);
	}

	/* this rectangle to be used for click detection and drawing */
	sl->r.y = offs;
	sl->r.h = len;

	/* HR: there are so much nice functions, use them!!! :-) */
	/* The small corrections are needed because the slider sits in a enclosure,
	 * the function is normally used for buttons that may be drawn a little larger
	 * than there official size. the small negative numbers makes the button
	 * exactly that amount smaller than the standard. (The standard is 1 outline border)
	 */
	{
		int thick = 1, mode = 3;
		if (MONO)
			mode = 1;
		else if (wind != window_list)
			thick = 0;

		d3_pushbutton(-2, &sl->r, NULL, widg->state, thick, mode);
	}

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
	RECT r; int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	RECT cl;

	rp_2_ap(wind, widg, &r);

	if (sl->length >= SL_RANGE)
	{
		sl->r = r;
		slider_back(&r);
		return true;
	}

	wr_mode(MD_REPLACE);

	len = sl_to_pix(r.w, sl->length);
	if (len < cfg.widg_w-3)
		len = cfg.widg_w-3;
	offs = r.x + sl_to_pix(r.w - len, sl->position);

	if (offs < r.x)
		offs = r.x;
	if (offs + len > r.x + r.w)
		len = r.x + r.w - offs;

	if (sl->r.w == 0 || wind != window_list)
	{
		sl->r = r;
		slider_back(&r);
	}
	else
	{
		cl = sl->r;
		if (sl->r.w == len
		    && !(offs + len < sl->r.x || offs > sl->r.x + sl->r.w))
		{
			if (sl->r.x < offs)
			{
				cl.w = offs - sl->r.x;
			}
			else
			{
				cl.x = offs + len;
				cl.w = sl->r.x - offs;
			}
		}
		if (cl.w)
			slider_back(&cl);
	}

	sl->r.x = offs;
	sl->r.w = len;

	{
		int thick = 1, mode = 3;
		if (MONO)
			mode = 1;
		else if (wind != window_list)
			thick = 0;

		d3_pushbutton(-2, &sl->r, NULL, widg->state, thick, mode);
	}

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
	short ny;
	int offs;

	/* need to do this anyhow, for mb */
	/* Ozk: No, we dont! */
	/* vq_mouse(C.vh, &mb, &pmx, &pmy);*/

	if (widget_active.m.cstate) //if (widg->s & 1)
	{
		if (!widget_active.cont)
		{
			widget_active.cont = true;
			/* Always have a nice consistent sizer when dragging a box */
			graf_mouse(XACRS_VERTSIZER, NULL, false);
		}

		if (widget_active.widg)
		{
			/* pending widget: take that */

			ny = widget_active.y;
			offs = widget_active.offs;
		}
		else
		{
			offs = sl->position;
			ny = widget_active.m.y;
		}

		//if (widget_active.m.cstate)
		{
			/* Drag slider */

			if (widget_active.m.y != ny)
				/* Has the mouse moved? */
				offs = bound_sl(offs + pix_to_sl(widget_active.m.y - ny, widg->r.h - sl->r.h) );

			set_widget_active(wind, widg, drag_vslide, 3);
			widget_active.y = widget_active.m.y;
			widget_active.offs = offs;

			if (wind->send_message)
				wind->send_message(lock, wind, NULL, AMQ_NORM,
						   WM_VSLID, 0, 0, wind->handle,
						   offs,     0, 0, 0);

			/* We return false here so the widget display status stays
			 * selected whilst it repeats
			 */
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
	short nx;
	int offs;

	/* need to do this anyhow, for mb */
	/* Ozk: No, we dont now either */
	/* vq_mouse(C.vh, &mb, &pmx, &pmy); */

	if (widget_active.m.cstate)
	{
		if (!widget_active.cont)
		{
			widget_active.cont = true;
			/* Always have a nice consistent sizer when dragging a box */
			graf_mouse(XACRS_HORSIZER, NULL, false);
		}

		if (widget_active.widg)
		{
			/* pending widget: take that */

			nx = widget_active.x;
			offs = widget_active.offs;
		}
		else
		{
			nx = widget_active.m.x;
			offs = sl->position;
		}

		//if (widget_active.m.cstate)
		{
			/* Drag slider */

			/* Has the mouse moved? */
			if (widget_active.m.x != nx)
				offs = bound_sl(offs + pix_to_sl(widget_active.m.x - nx, widg->r.w - sl->r.w) );

			set_widget_active(wind, widg, drag_hslide, 4);
			widget_active.x = widget_active.m.x;
			widget_active.offs = offs;

			if (wind->send_message)
				wind->send_message(lock, wind, NULL, AMQ_NORM,
						   WM_HSLID, 0, 0, wind->handle,
						   offs, 0, 0, 0);

			/* We return false here so the widget display status stays
			 * selected whilst it repeats
			 */
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
		draw_object_tree(lock, wt, NULL, widg->start, 100, 123);
		clear_clip();
	}
	else
		draw_object_tree(lock, wt, NULL, widg->start, 100, 3);

	return true;
}

static bool
display_arrow(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	return true;
}


/* HR : moved the following to the end of the file, this way all function that are referenced, are already defined */
/*      removed a very annoying & confusing connection between bit number of a mask and a enum value */
static void
shift_from_top(struct xa_window *wind, XA_WIDGET *widg, long tp, int xtra)
{
	int wd;

	/* Shift any widgets we might interfere with */
	for (wd = 0; wd < XA_MAX_WIDGETS; wd++)
	{
		XA_WIDGET *tw = wind->widgets + wd;
		if (tp & tw->loc.mask)
		{
			if (   tw->loc.relative_type == LT
			    || tw->loc.relative_type == RT
			    || tw->loc.relative_type == CT)
			{
				tw->r.y += widg->r.h + xtra;
			}
		}
	}
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

/* 
 * Setup the required 'standard' widgets for a window. These are the ordinary GEM
 * behaviours. These can be changed for any given window if you want special behaviours.
 */
void
standard_widgets(struct xa_window *wind, XA_WIND_ATTR tp, bool keep_stuff)
{
	XA_WIDGET *widg;
	int /*wd,*/ bottom = 0, right = 0;
	XA_WIND_ATTR old_tp = wind->active_widgets;
	//void *stuff;

	DIAGS(("standard_widgets: new(%lx), prev(%lx) on wind %d for %s",
		tp, old_tp, wind->handle, wind->owner->proc_name));
	
	/* Fill in the active widgets summary */
	wind->active_widgets = tp;

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
		if (!(old_tp & CLOSER))
		{
			DIAGS(("Make CLOSER"));
			make_widget(wind, &stdl_close, display_def_widget, click_close, 0);
		}
#if GENERATE_DIAGS
		else
			DIAGS(("Nothing to do for closer"));
#endif
	}
	if (old_tp & CLOSER)
	{
		DIAGS(("Clear closer"));
		zwidg(wind, XAW_CLOSE, keep_stuff);
	}

	if (tp & FULLER)
	{
		if (!(old_tp & FULLER))
		{
			DIAGS(("Make fuller"));
			make_widget(wind, &stdl_full, display_def_widget, click_full, 0);
		}
#if GENERATE_DIAGS
		else
			DIAGS(("Nothing to do for fuller"));
#endif
	}
	if (old_tp & FULLER)
	{
		DIAGS(("Clear fuller"));
		zwidg(wind, XAW_FULL, keep_stuff);
	}

	if (tp & ICONIFIER)
	{
		if (!(old_tp & ICONIFIER))
		{
			DIAGS(("Make iconifier"));
			widg = make_widget(wind, &stdl_iconify, display_def_widget, click_iconify, 0);
		}
		else
		{		
			DIAGS(("iconifier already made"));
			widg = get_widget(wind, XAW_ICONIFY);
			set_widget_coords(widg);
		}
		if (tp & FULLER)
			widg->r.x += get_widget(wind, XAW_FULL)->r.w;
	}
	else if (old_tp & ICONIFIER)
	{
		DIAGS(("clear iconifier"));
		zwidg(wind, XAW_ICONIFY, keep_stuff);
	}

	if (tp & HIDE)
	{
		if (!(old_tp & HIDE))
		{
			DIAGS(("Make hider"));
			widg = make_widget(wind, &stdl_hide, display_def_widget, click_hide, 0);
		}
		else
		{
			DIAGS(("hider already made"));
			widg = get_widget(wind, XAW_HIDE);
			set_widget_coords(widg);
		}
		if (tp & FULLER)
			widg->r.x += get_widget(wind, XAW_FULL)->r.w;
		if (tp & ICONIFIER)
			widg->r.x += get_widget(wind, XAW_ICONIFY)->r.w;
	}
	else if (old_tp & HIDE)
		zwidg(wind, XAW_HIDE, keep_stuff);

	if (tp & SIZER)
	{
		if (!(old_tp & SIZER))
		{
			DIAGS(("Make sizer"));
			make_widget(wind, &stdl_resize, display_def_widget, 0, drag_resize);
		}
#if GENERATE_DIAGS
		else
			DIAGS(("sizer already made"));
#endif
	}
	else if (old_tp & SIZER)
	{
		DIAGS(("clear sizer"));
		zwidg(wind, XAW_RESIZE, keep_stuff);
	}

	if ( (tp & (SIZER|MOVER)) == (SIZER|MOVER) )
	{
		if ( (old_tp & (SIZER|MOVER)) != (SIZER|MOVER) )
		{
			DIAGS(("Make border"));
			make_widget(wind, &stdl_border, display_border, drag_border, drag_border);
		}
#if GENERATE_DIAGS
		else
			DIAGS(("border already made"));
#endif
	}
	else if ( (old_tp & (SIZER|MOVER)) == (SIZER|MOVER) )
	{
		DIAGS(("clear border"));
		zwidg(wind, XAW_BORDER, keep_stuff);
	}

	if (tp & UPARROW)
	{
		if (!(old_tp & UPARROW))
		{
			DIAGS(("Make uparrow"));
			widg = make_widget(wind, &stdl_uscroll, display_def_widget, click_scroll, click_scroll);
			widg->dclick = click_scroll;
			widg->arrowx = WA_UPLINE;
			widg->xarrow = WA_DNLINE;
			widg->xlimit = SL_RANGE;
			widg->slider_type = XAW_VSLIDE;

			if (tp & VSLIDE)
			{
				if (!(old_tp & VSLIDE))
				{
					struct xa_widget *w;
					DIAGS(("Make uppage"));
					w = make_widget(wind, &stdl_upage, display_arrow, click_scroll, click_scroll);
					w->arrowx = WA_UPPAGE;
					w->xarrow = WA_DNPAGE;
					w->xlimit = SL_RANGE;
					w->slider_type = XAW_VSLIDE;
				}
#if GENERATE_DIAGS
				else
					DIAGS(("uppage already made"));
#endif
			}
			else if (old_tp & VSLIDE)
			{
				DIAGS(("clear uppage"));
				zwidg(wind, XAW_UPPAGE, keep_stuff);
			}
		}
#if GENERATE_DIAGS
		else
			DIAGS(("uparrow already made"));
#endif
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
		if (!(old_tp & DNARROW))
		{
			widg = make_widget(wind, &stdl_dscroll, display_def_widget, click_scroll, click_scroll);
			widg->dclick = click_scroll;
			widg->arrowx = WA_DNLINE;
			widg->xarrow = WA_UPLINE;
			widg->limit = SL_RANGE;
			widg->slider_type = XAW_VSLIDE;
			if (tp & VSLIDE)
			{
				if (!(old_tp & VSLIDE))
				{
					struct xa_widget *w;
					w = make_widget(wind, &stdl_dpage, display_arrow, click_scroll, click_scroll);
					w->arrowx = WA_DNPAGE;
					w->xarrow = WA_UPPAGE;
					w->limit = SL_RANGE;
					w->slider_type = XAW_VSLIDE;
				}
			}
			else if (old_tp & VSLIDE)
				zwidg(wind, XAW_DNPAGE, keep_stuff);
		}
		else
		{
			widg = get_widget(wind, XAW_DNLN);
			set_widget_coords(widg);
		}
		if ( !(tp & SIZER) && !bottom)
			widg->r.y -= cfg.widg_h;
		else if (bottom)
			widg->r.y += WASP;	
	}
	else if (old_tp & DNARROW)
	{
		zwidg(wind, XAW_DNLN, keep_stuff);
		if (old_tp & VSLIDE)
			zwidg(wind, XAW_DNPAGE, keep_stuff);
	}

	if (tp & LFARROW)
	{
		if (!(old_tp & LFARROW))
		{
			widg = make_widget(wind, &stdl_lscroll, display_def_widget, click_scroll, click_scroll);
			widg->dclick = click_scroll;
			widg->arrowx = WA_LFLINE;
			widg->xarrow = WA_RTLINE;
			widg->xlimit = SL_RANGE;
			widg->slider_type = XAW_HSLIDE;
			if (tp & HSLIDE)
			{
				if (!(old_tp & HSLIDE))
				{
					struct xa_widget *w;
					w = make_widget(wind, &stdl_lpage, display_arrow, click_scroll, click_scroll);
					w->arrowx = WA_LFPAGE;
					w->xarrow = WA_RTPAGE;
					w->xlimit = SL_RANGE;
					w->slider_type = XAW_HSLIDE;
				}
			}
			else if (old_tp & HSLIDE)
				zwidg(wind, XAW_LFPAGE, keep_stuff);
		}
	}
	else if (old_tp & LFARROW)
	{
		zwidg(wind, XAW_LFLN, keep_stuff);
		if (old_tp & HSLIDE)
			zwidg(wind, XAW_LFPAGE, keep_stuff);
	}

	if (tp & RTARROW)
	{
		if (!(old_tp & RTARROW))
		{
			widg = make_widget(wind, &stdl_rscroll, display_def_widget, click_scroll, click_scroll);
			widg->dclick = click_scroll;
			widg->arrowx = WA_RTLINE;
			widg->xarrow = WA_LFLINE;
			widg->limit = SL_RANGE;
			widg->slider_type = XAW_HSLIDE;
			if (tp & HSLIDE)
			{
				if (!(old_tp & HSLIDE))
				{
					struct xa_widget *w;
					w = make_widget(wind, &stdl_rpage, display_arrow, click_scroll, click_scroll);
					w->arrowx = WA_RTPAGE;
					w->xarrow = WA_LFPAGE;
					w->limit = SL_RANGE;
					w->slider_type = XAW_HSLIDE;
				}
			}
			else if (old_tp & HSLIDE)
				zwidg(wind, XAW_RTPAGE, keep_stuff);
		}
		else
		{
			widg = get_widget(wind, XAW_RTPAGE);
			set_widget_coords(widg);
		}
		if (!(tp & SIZER) && !right)
			widg->r.x -= cfg.widg_h;
		else if (right)
			widg->r.x += WASP;
	}
	else if (old_tp & RTARROW)
	{
		zwidg(wind, XAW_RTLN, keep_stuff);
		if (old_tp & HSLIDE)
			zwidg(wind, XAW_RTPAGE, keep_stuff);
	}

	if (tp & VSLIDE)
	{
		if (!(old_tp & VSLIDE))
		{
			widg = make_widget(wind, &stdl_vslide, display_vslide, 0, drag_vslide);
			if (!keep_stuff)
			{
				XA_SLIDER_WIDGET *sl = kmalloc(sizeof(*sl));
				assert(sl);
				widg->stuff = sl;
				sl->length = SL_RANGE;
				widg->flags |= XAWF_STUFFKMALLOC;
			}
		}
	}
	else if (old_tp & VSLIDE)
		zwidg(wind, XAW_VSLIDE, keep_stuff);

	if (tp & HSLIDE)
	{
		if (!(old_tp & HSLIDE))
		{
			widg = make_widget(wind, &stdl_hslide, display_hslide, 0, drag_hslide);
			if (!keep_stuff)
			{
				XA_SLIDER_WIDGET *sl = kmalloc(sizeof(*sl));
				assert(sl);
				widg->stuff = sl;
				sl->length = SL_RANGE;
				widg->flags |= XAWF_STUFFKMALLOC;
			}
		}
	}
	else if (old_tp & HSLIDE)
		zwidg(wind, XAW_HSLIDE, keep_stuff);

	if (tp & XaPOP) /* popups in a window */
	{
		if (!(old_tp & XaPOP))
			make_widget(wind, &stdl_pop, 0, 0, 0);
	}
	else if (old_tp & XaPOP)
		zwidg(wind, XAW_MENU, keep_stuff);

	if (tp & XaMENU)
	{
		if (!(old_tp & XaMENU))
		{
			widg = make_widget(wind, &stdl_menu, 0, 0, 0);
		}
		else
		{
			widg = get_widget(wind, XAW_MENU);
			set_widget_coords(widg);
		}
		widg->r.h = screen.c_max_h + 3;
		/* Remainder of widg is provided by set_menu_widget() */		
		if (tp & (NAME|CLOSER|FULLER|ICONIFIER|HIDE))
			widg->r.y += cfg.widg_h + 1;
		if (tp & INFO)
			widg->r.y += cfg.widg_h + 1;

		shift_from_top(wind, widg, tp & (~(XaMENU|INFO|NAME|CLOSER|FULLER|ICONIFIER|HIDE)), 3);
	}
	else if (old_tp & XaMENU)
		zwidg(wind, XAW_MENU, keep_stuff);

	if (tp & INFO)
	{
		if (!(old_tp & INFO))
		{
			widg = make_widget(wind, &stdl_info, display_info, click_title, 0);
			if (!keep_stuff)
				/* Give the window a default info line until the client changes it */
				widg->stuff = "Info Bar";

		}
		else
		{
			widg = get_widget(wind, XAW_INFO);
			set_widget_coords(widg);
		}
		if (tp & (NAME|CLOSER|FULLER|ICONIFIER))
			widg->r.y += cfg.widg_h + 1;

		shift_from_top(wind, widg, tp & (~(XaMENU|INFO|NAME|CLOSER|FULLER|ICONIFIER|HIDE)), 2);
	}
	else if (old_tp & INFO)
		zwidg(wind, XAW_INFO, keep_stuff);

	if (tp & NAME)
	{
		if (!(old_tp & NAME))
		{
			widg = make_widget(wind, &stdl_title, display_title, click_title, drag_title);
			widg->dclick = dclick_title;
			if (widg->stuff == NULL && !keep_stuff)
				/* Give the window a default title if not already set */
				widg->stuff = "XaAES Window";

		}
		else
		{
			widg = get_widget(wind, XAW_TITLE);
			set_widget_coords(widg);
		}
		/* else the widget already exists */
		if (tp & CLOSER)
			widg->r.x += cfg.widg_w + 1;
		shift_from_top(wind, widg, tp & (~(XaMENU|INFO|NAME|CLOSER|FULLER|ICONIFIER|HIDE)), (tp & INFO) ? 0 : 2);
		widg->r.h = cfg.widg_h;
	}
	else
	{
		if (old_tp & NAME)
			zwidg(wind, XAW_TITLE, keep_stuff);
		widg = make_widget(wind, &stdl_notitle, 0,0,0);
		widg->r.h = cfg.widg_h;
	}

	if ((tp & VSLIDE) && !(tp & UPARROW))
	{
		get_widget(wind, XAW_UPPAGE)->r.y -= cfg.widg_h;
		get_widget(wind, XAW_VSLIDE)->r.y -= cfg.widg_h;
	}

	if ((tp & HSLIDE) && !(tp & LFARROW))
	{
		get_widget(wind, XAW_LFPAGE)->r.x -= cfg.widg_w;
		get_widget(wind, XAW_HSLIDE)->r.x -= cfg.widg_w;
	}
}

/*
 * HR: Direct display of the toolbar widget; HR 260102: over the rectangle list.
 */
void
display_toolbar(enum locks lock, struct xa_window *wind, short item)
{
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	struct xa_rect_list *rl;

	hidem();
	widg->start = item;

	rl = rect_get_system_first(wind);
	while (rl)
	{			
		set_clip(&rl->r);
		widg->display(lock, wind, widg);
		rl = rect_get_system_next(wind);
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

/*
 * Attach a toolbar to a window...probably let this be accessed via wind_set one day
 * This is also used to setup windowed form_do sessions()
 */
XA_TREE *
set_toolbar_widget(enum locks lock, struct xa_window *wind, OBJECT *obtree, short edobj)
{
	XA_TREE *wt;
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	XA_WIDGET_LOCATION loc;

	DIAG((D_wind, wind->owner, "set_toolbar_widget for %d (%s): obtree %lx, %d",
		wind->handle, wind->owner->name, obtree, edobj));

	if (widg->stuff)
	{
		((XA_TREE *)widg->stuff)->widg = NULL;
		((XA_TREE *)widg->stuff)->wind = NULL;
		((XA_TREE *)widg->stuff)->zen  = false;
	}

	
	wt = obtree_to_wt(wind->owner, obtree);
	if (!wt)
		wt = new_widget_tree(wind->owner, obtree);

	assert(wt);

	wt->widg = widg;
	wt->wind = wind;
	wt->zen  = true;

	/*
	 * Ozk: if edobj == -2, we want to look for an editable and place the
	 * cursor there. Used by wdlg_create() atm
	 */
	if (edobj == -2)
		edobj = ob_find_any_flst(obtree, OF_EDITABLE, 0, 0, OS_DISABLED, OF_LASTOB, 0);

	if (!obj_edit(wt, ED_INIT, edobj, 0, -1, false, NULL, NULL, &edobj))
		obj_edit(wt, ED_INIT, edobj, 0, -1, false, NULL, NULL, NULL);

		
#if WDIALOG_WDLG
	if ((wind->dial & created_for_WDIAL))
	{
		wt->exit_form	= NULL;
		widg->click	= click_wdlg_widget;
		widg->dclick	= click_wdlg_widget;
		widg->drag	= click_wdlg_widget;
		widg->display	= NULL;
	}
	else
	{
		if (wt->e.obj >= 0 || obtree_has_default(obtree))
			wind->keypress = Key_form_do;

		wt->exit_form	= Exit_form_do;
		widg->click	= Click_windowed_form_do;
		widg->dclick	= Click_windowed_form_do;
		widg->drag	= Click_windowed_form_do;
		widg->display	= display_object_widget;
	}
#else
	if (wt->e.obj >= 0 || obtree_has_default(obtree))
		wind->keypress = Key_form_do;

	wt->exit_form	= Exit_form_do;
	widg->click	= Click_windowed_form_do;
	widg->dclick	= Click_windowed_form_do;
	widg->drag	= Click_windowed_form_do;
	widg->display	= display_object_widget;
#endif

	widg->destruct	= free_xawidget_resources;
	
	/* HR 280801: clicks are now put in the widget struct.
	      NB! use this property only when there is very little difference between the 2 */

	widg->state	= OS_NORMAL;
	widg->stuff	= wt;
	widg->stufftype	= STUFF_IS_WT;
	widg->start	= 0;
	wind->tool	= widg;

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

	if (click > sl->r.x && click < sl->r.x + sl->r.w)
		return 0;

	/* outside slider, arrows must be defined */
	if (w->active_widgets & LFARROW)
	{
		/* outside has 2 sides :-) */

		/* up arrow */
		if (sl->position > 0 && click < sl->r.x)
			/* add to slider index */
			return 1;

		/* dn arrow */
		if (sl->position < SL_RANGE && click > sl->r.x + sl->r.w)
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

	if (click > sl->r.y && click < sl->r.y + sl->r.h)
		return 0;

 	if (w->active_widgets & UPARROW)
	{
		if (sl->position > 0 && click < sl->r.y)	
			return 1;

		if (sl->position < SL_RANGE && click > sl->r.y + sl->r.h)
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
checkif_do_widgets(enum locks lock, struct xa_window *w, XA_WIND_ATTR mask, const struct moose_data *md)
{
	XA_WIDGET *widg;
	int f, clicks;
	short winstatus = w->window_status;
	bool inside = false;

	clicks = md->clicks;
	if (clicks > 2)
		clicks = 2;

	/* Scan through widgets to find the one we clicked on */
	for (f = 0; f < XA_MAX_WIDGETS; f++)
	{
		RECT r;

		while (is_page(f))
			f++;

		widg = w->widgets + f;

#if 0
		display("rp_2_ap: type=%s, widg=%lx, wt=%lx, obtree=%lx",
			t_widg[widg->type], widg, widg->stuff,
			widg->type == XAW_TOOLBAR ? (long)((XA_TREE *)widg->stuff)->tree : -1 );
		display(" -- statusmask %x, window_status %x", widg->loc.statusmask, winstatus);
		display(" -- widg->display=%lx", widg->display);
#endif

		if (!(winstatus & widg->loc.statusmask) && widg->display)					/* Is the widget in use? */
		{
			if (    widg->loc.mask         == 0		/* not maskable */
			    || (widg->loc.mask & mask) == 0)		/* masked */
			{

				if (f != XAW_BORDER)			/* HR 280102: implement border sizing. */
				{					/* Normal widgets */
					rp_2_ap_cs(w, widg, &r);	/* Convert relative coords and window location to absolute screen location */
					inside = m_inside(md->x, md->y, &r);
					//display("%d/%d - %d/%d/%d/%d", md->x, md->y, r);
				}
				else
				{
					r = w->r;			/* Inside window and outside border area = border. */
					inside = !m_inside(md->x, md->y, &w->ba);
				}
				if (inside)
				{
					//display("checkif_do_widgets: true");
					return true;
				}
			}
		}
		//else
		//	display("masked");
	}
	//display("checkif_do_widgets: false");
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
			if (    widg->loc.mask         == 0		/* not maskable */
			    || (widg->loc.mask & mask) == 0)		/* masked */
			{
				bool inside;

				if (f != XAW_BORDER)			/* HR 280102: implement border sizing. */
				{					/* Normal widgets */
					rp_2_ap(w, widg, &r);		/* Convert relative coords and window location to absolute screen location */
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
						ax = is_V_arrow(w,widg,md->y);
					else if (f == XAW_HSLIDE)
						ax = is_H_arrow(w,widg,md->x);

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
		
						//check_mouse( w->owner, &b, &rx, &ry);

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
		display_widget(lock, wind, widg);
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
				/* Flag the widget as de-selected */
				redisplay_widget(lock, wind, widg, OS_NORMAL);
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
		if (wind->active_widgets & SIZER)
		{
			widg = wind->widgets + XAW_RESIZE;
			if (wind->frame > 0 && !(wind->widgets[XAW_BORDER].loc.statusmask & status) && (!m_inside(x, y, &wind->ba)))
			{
				r = wind->r;
				shape = border_mouse[compass(16, x, y, r)];
			}
			else
			{
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
				graf_mouse(shape, NULL, true);
		}
		else
		{
			if (C.aesmouse != -1)
				graf_mouse(-1, NULL, true);
			if (C.mouse_form != wind->owner->mouse_form)
				graf_mouse(wo->mouse, wo->mouse_form, false);
		}
	}
	else if (C.aesmouse != -1)
		graf_mouse(-1, NULL, true);

	return shape;
}

