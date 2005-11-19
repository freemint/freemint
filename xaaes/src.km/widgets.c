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

#include "win_draw.h"

#include "mint/signal.h"

// static struct xa_widget_row * get_last_row(struct xa_widget_row *row, short lmask, short lvalid, bool backwards);
// static struct xa_widget_row * get_first_row(struct xa_widget_row *row, short lmask, short lvalid, bool backwards);
static void rp_2_ap_row(struct xa_window *wind);

//static OBJECT *def_widgets;

#if GENERATE_DIAGS
static char *t_widg[] =
{
	"XAW_BORDER",			/* Extended XaAES widget, used for border sizing. */
	"XAW_TITLE",
	"XAW_CLOSE",
	"XAW_FULL",
	"XAW_INFO",
	"XAW_RESIZE",			/* 5 */
	"XAW_UPLN",			/* 6 */
	"XAW_DNLN",			/* 7 */
	"XAW_VSLIDE",
	"XAW_LFLN",			/* 9 */
	"XAW_RTLN",			/* 10 */
	"XAW_HSLIDE",
	"XAW_ICONIFY",
	"XAW_HIDE",			/* 13 */
	
	"XAW_TOOLBAR",			/* 14 Extended XaAES widget */
	"XAW_MENU",			/* 15 Extended XaAES widget, must be drawn last. */

	"XAW_MOVER",			/* Not actually used like the others */
	"XAW_UPPAGE",
	"XAW_DNPAGE",
	"XAW_LFPAGE",
	"XAW_RTPAGE",
};
#endif


static void
setup_widget_theme(struct xa_client *client, struct xa_widget_theme *wtheme)
{
	struct widget_theme *theme = C.Aes->widget_theme->client;
	struct widget_theme *ptheme = C.Aes->widget_theme->popup;
	struct widget_theme *atheme = C.Aes->widget_theme->alert;
				
	if (theme)
	{
		wtheme->client = theme;
		theme->links++;
	}
	else
	{
		(*client->xmwt->new_theme)(client->wtheme_handle, WINCLASS_CLIENT, &theme);
		if ((wtheme->client = theme))
			theme->links++;
	}

	if (ptheme)
	{
		wtheme->popup = ptheme;
		ptheme->links++;
	}
	else
	{
		(*client->xmwt->new_theme)(client->wtheme_handle, WINCLASS_POPUP, &ptheme);
		if ((wtheme->popup = ptheme))
			ptheme->links++;
	}

	if (atheme)
	{
		wtheme->alert = atheme;
		atheme->links++;
	}
	else
	{
		(*client->xmwt->new_theme)(client->wtheme_handle, WINCLASS_ALERT, &atheme);
		if ((wtheme->alert = atheme))
			atheme->links++;
	}
}

void
init_client_widget_theme(struct xa_client *client)
{
	if (!client->widget_theme)
	{
		struct xa_widget_theme *wtheme;
 
 		wtheme = kmalloc(sizeof(*wtheme));
		assert(wtheme);
		bzero(wtheme, sizeof(*wtheme));
		client->widget_theme = wtheme;
		setup_widget_theme(client, wtheme);
	}
}

void
exit_client_widget_theme(struct xa_client *client)
{
	if (client->widget_theme)
	{
		struct widget_theme *theme;
		if ((theme = client->widget_theme->client))
		{
			theme->links--;
			if (!theme->links)
			{
// 				display("free theme %lx", theme);
				(*client->xmwt->free_theme)(client->wtheme_handle, &client->widget_theme->client);
			}
		}
		if ((theme = client->widget_theme->popup))
		{
			theme->links--;
			if (!theme->links)
				(*client->xmwt->free_theme)(client->wtheme_handle, &client->widget_theme->popup);
		}
		if ((theme = client->widget_theme->alert))
		{
			theme->links--;
			if (!theme->links)
				(*client->xmwt->free_theme)(client->wtheme_handle, &client->widget_theme->alert);
		}
		
		kfree(client->widget_theme);
		client->widget_theme = NULL;
	}
}

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

	if (w->active_widgets & widg->m.r.tp)
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

	if (widg->m.r.xaw_idx == XAW_TOOLBAR || widg->m.r.xaw_idx == XAW_MENU)
	{
 		if (!(widg->m.properties & WIP_NOTEXT))
 			widg->r.w = wind->wa.w;
		if ((wt = widg->stuff))
		{
			obtree = wt->tree;
			if (obtree && wind != root_window)
			{
				obtree->ob_x = r->x;
				obtree->ob_y = r->y;
 				obtree->ob_width = widg->r.w;
				if (!wt->zen)
				{
					obtree->ob_x += wt->ox;
					obtree->ob_y += wt->oy;
					obtree->ob_width -= (wt->ox + wt->ox);
				}
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
		t_widg[widg->m.r.xaw_idx], widg, widg->stuff,
		widg->m.r.xaw_idx == XAW_TOOLBAR ? (long)((XA_TREE *)widg->stuff)->tree : -1 ));

	if (widg->m.r.xaw_idx != XAW_TOOLBAR )
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

	ww = wind->frame < 0 ? wind->r.w : wind->r.w - wind->x_shadow;
	wh = wind->frame < 0 ? wind->r.h : wind->r.h - wind->y_shadow;

	switch ((widg->m.r.pos_in_row & (R_BOTTOM|R_RIGHT|R_CENTER)))
	{
	case LT:
		r->x += rtx;
		r->y += rty;
		break;
	case LB:
		r->x += rtx;
		r->y += (wh - rty);
		break;
	case RT:
		r->x += (ww - rtx);
		r->y += rty;
		break;
	case RB:
		r->x += (ww - rtx);
		r->y += (wh - rty);
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
		t_widg[widg->m.r.xaw_idx], widg, widg->stuff,
		widg->m.r.xaw_idx == XAW_TOOLBAR ? (long)((XA_TREE *)widg->stuff)->tree : -1 ));

	if (widg->m.r.xaw_idx != XAW_TOOLBAR)
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

	ww = wind->frame < 0 ? wind->r.w : wind->r.w - wind->x_shadow;
	wh = wind->frame < 0 ? wind->r.h : wind->r.h - wind->y_shadow;

	switch ((widg->m.r.pos_in_row & (R_BOTTOM|R_RIGHT|R_CENTER)))
	{
	case LT:
		r->x += rtx;
		r->y += rty;
		break;
	case LB:
		r->x += rtx;
		r->y += (wh - rty);
		break;
	case RT:
		r->x += (ww - rtx);
		r->y += rty;
		break;
	case RB:
		r->x += (ww - rtx);
		r->y += (wh - rty);
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
static bool
checkfor_xted(OBJECT *obtree, short obj, void *ret)
{
	short *flag = ret;

	if (object_has_tedinfo(obtree + obj))
	{
		XTEDINFO *x;
		(void)object_get_tedinfo(obtree + obj, &x);
		if (x)
			*flag = 1;
		return true;
	}
	return false;
}
void
init_widget_tree(struct xa_client *client, struct widget_tree *wt, OBJECT *obtree)
{
	short sx, sy, xted = 0;
	RECT r;

	bzero(wt, sizeof(*wt));
	
// 	display("init_widget_tree: tree %lx", obtree);
	wt->tree = obtree;
	wt->owner = client;
	
	wt->focus = -1;
	wt->e.obj = -1;
	wt->ei = NULL;

	sx = obtree->ob_x;
	sy = obtree->ob_y;
	obtree->ob_x = 100;
	obtree->ob_y = 100;
	
	ob_area(obtree, 0, &r);
	wt->ox = obtree->ob_x - r.x;
	wt->oy = obtree->ob_y - r.x;
	
	if (wt->ox < 0)
		wt->ox = 0;
	if (wt->oy < 0)
		wt->oy = 0;

	obtree->ob_x = sx;
	obtree->ob_y = sy;

	wt->next = client->wtlist;
	client->wtlist = wt;

	if ((obtree[3].ob_type & 0xff) == G_TITLE)
		wt->is_menu = wt->menu_line = true;

// 	if (!strnicmp(client->proc_name, "taskb", 5))
// 		dforeach_object(obtree, 0, 0, 0, 0, checkfor_xted, &xted);
// 	else
		foreach_object(obtree, 0, 0, 0, 0, checkfor_xted, &xted);
	
	if (xted)
		wt->flags |= WTF_OBJCEDIT;
#if 0
	do
	{
		if (object_has_tedinfo(obtree))
		{
			XTEDINFO *x;
			(void)object_get_tedinfo(obtree, &x);
			if (x)
				wt->flags |= WTF_OBJCEDIT;
			break;
		}
	} while (!(obtree++->ob_flags & OF_LASTOB));
#endif
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
		init_widget_tree(client, new, obtree);
		new->flags |= WTF_ALLOC;		
	}
	DIAGS((" return new wt=%lx", new));
	return new;
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
		display("free_wt: wt=%lx, links=%d, flags=%lx, ismenu=%s, menuline=%s, owner=%s",
			wt, wt->links, wt->flags,
			wt->is_menu ? "Yes" : "No",
			wt->menu_line ? "Yes" : "No",
			wt->owner->name);
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

	if (!(wind->window_status & widg->m.statusmask) && wdg_is_inst(widg))
	{
		struct xa_vdi_settings *v = wind->vdi_settings;

		DIAG((D_wind, wind->owner, "draw_window %d: display widget %d (func=%lx)",
			wind->handle, widg->m.r.xaw_idx, widg->m.r.draw));

		if (!rl)
			rl = wind->rect_list.start;

		hidem();
		while (rl)
		{	
			if (widg->m.properties & WIP_WACLIP)
			{
				if (xa_rect_clip(&rl->r, &wind->wa, &r))
				{
					(*v->api->set_clip)(v, &r);
					widg->m.r.draw(wind, widg, &r);
				}
			}
			else
			{
				(*v->api->set_clip)(v, &rl->r);
				widg->m.r.draw(wind, widg, &rl->r);
			}
			rl = rl->next;
		}
		(*v->api->clear_clip)(v);
		showm();
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
		if (ce->client == mc)
		{
			display_widget(lock, root_window, widg, NULL);
		}
	#if 1
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
	#endif
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

/* Establish iconified window position from a simple ordinal. */
RECT
iconify_grid(int i)
{
	RECT ic = {0,0,ICONIFIED_W,ICONIFIED_H};
	int j, w = screen.r.w/ic.w;

	ic.x = screen.r.x;
	ic.y = screen.r.y + screen.r.h - 1 - ic.h;

	j = i / w;
	i %= w;

	ic.x += i * ic.w;
	ic.y -= j * ic.h;

	return ic;
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
		if (   md->state == MBS_RIGHT /*widg->s == 2*/
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
				
				if (wind->opts & XAWO_WCOWORK)
					r = f2w(&wind->delta, &r, true);
				
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
					
					if (wind->opts & XAWO_WCOWORK)
						r = f2w(&wind->delta, &r, true);
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
	if (wind->nolist)
		return true;

	/* Ozk: If either shifts pressed, unconditionally send the window to bottom */
	if (md->clicks == 1)
	{

		if (md->state == MBS_LEFT)
		{
			if ((md->kstate & 3) && (wind->active_widgets & (STORE_BACK|BACKDROP)) == BACKDROP)
			{
				send_bottomed(lock, wind);
			}
			else
			{
				/* if not on top or on top and not focus */
				if (!is_topped(wind))
				{
					send_topped(lock, wind);
				}
				/* If window is already top, then send it to the back */
				/* Ozk: Always bottom iconified windows... */
				else if ((wind->window_status & XAWS_ICONIFIED))
				{
					send_bottomed(lock, wind);
				}
				else if ((wind->active_widgets & (STORE_BACK|BACKDROP)) == BACKDROP)
				{
					send_bottomed(lock, wind);
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
	else if (md->clicks == 2)
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
	}
	return false;
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
	bool ret = false;

	if (wind->send_message)
	{
		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_CLOSED, 0, 0, wind->handle,
				   0, 0, 0, 0);
		if ((wind->window_status & XAWS_OPEN))
			ret = true;	/* redisplay */
	}
	/* Don't redisplay in the do_widgets() routine */
	return ret;
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
	
	if (!wind->send_message)
		return false;

	if ((wind->window_status & XAWS_OPEN))
	{
		short msg = -1;
		RECT r;

		if ((wind->window_status & XAWS_ICONIFIED))
		{
			/* Window is already iconified - send request to restore it */
			r = wind->ro;
			
			msg = WM_UNICONIFY;
		}
		else
		{
			/* Window is open - send request to iconify it */

			r = free_icon_pos(lock|winlist, NULL);

			/* Could the whole screen be covered by iconified
			 * windows? That would be an achievement, wont it?
			 */
			if (r.y > root_window->wa.y)
				msg = (md->kstate & K_CTRL) ? WM_ALLICONIFY : WM_ICONIFY;
		}
		if (msg != -1)
		{
			if (wind->opts & XAWO_WCOWORK)
				r = f2w(msg == WM_UNICONIFY ? &wind->save_delta : &wind->delta, &r, true);

			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   msg, 0, 0, wind->handle,
				   r.x, r.y, r.w, r.h);
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
	
	if (   md->state == MBS_RIGHT /*widg->s == 2*/
	    || wind->send_message == NULL
	    || wind->owner->options.nolive)
	{
		int xy = sizer ? SE : compass(16, md->x, md->y, r); //compass(16, widg->mx, widg->my, r);
		short maxw, maxh;
		
		if (wind->active_widgets & USE_MAX)
			maxw = wind->max.w, maxh = wind->max.h;
		else
			maxw = maxh = 0;

		/* Always have a nice consistent SIZER when resizing a window */
		graf_mouse(border_mouse[xy], NULL, NULL, false);

		lock_screen(wind->owner->p, 0, 0, 1234);
		rubber_box(wind->owner, xy,
		           r,
			   rect_dist_xy(wind->owner, md->x, md->y, &r, &d),
			   wind->min.w,
			   wind->min.h,
			   maxw, //wind->max.w,
			   maxh, //wind->max.h,
			   &r);
		unlock_screen(wind->owner->p, 1234);

		move = r.x != wind->r.x || r.y != wind->r.y,
		size = r.w != wind->r.w || r.h != wind->r.h;

		if (move || size)
		{
			if (wind->opts & XAWO_WCOWORK)
				r = f2w(&wind->delta, &r, true);

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
				short maxw, maxh;
		
				if (wind->active_widgets & USE_MAX)
					maxw = wind->max.w, maxh = wind->max.h;
				else
					maxw = maxh = 0;
				
				r = widen_rectangle(xy, widget_active.m.x, widget_active.m.y, r, &d);

				check_wh_cp(&r, xy,
					    wind->min.w,
					    wind->min.h,
					    maxw, //wind->max.w,
					    maxh); //wind->max.h);
			}

			move = r.x != wind->r.x || r.y != wind->r.y,
			size = r.w != wind->r.w || r.h != wind->r.h;
			
			if (move || size)
			{
				if (wind->opts & XAWO_WCOWORK)
					r = f2w(&wind->delta, &r, true);
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
	bool reverse = md->state == MBS_RIGHT;
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

				set_widget_active(wind, widg, widg->m.drag, 2);
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
display_object_widget(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	XA_TREE *wt = widg->stuff;
	OBJECT *root;

	/* Convert relative coords and window location to absolute screen location */
	root = rp_2_ap(wind, widg, NULL);

	DIAG((D_form,wind->owner,"display_object_widget(wind=%d), wt=%lx, e.obj=%d, e.pos=%d, form: %d/%d",
		wind->handle, wt, wt->e.obj, wt->e.pos, root->ob_x, root->ob_y));

	if (wind->nolist && (wind->dial & created_for_POPUP))
	{
		/* clip work area */
		(*v->api->set_clip)(v, &wind->wa);
		draw_object_tree(0, wt, NULL, v, widg->start, 100, NULL, 0);
		(*v->api->clear_clip)(v);
	}
	else
		draw_object_tree(0, wt, NULL, v, widg->start, 100, NULL, 0);

	return true;
}

/*
 * Calculate the size of the work area for a window and store it
 * - This is needed because the locations of widgets are relative and
 *   can be modified.
 * Actually, this updates all the dynamic sized elements from the standard widget set...
 * (namely: work area, sliders and title bar)
 */

void
calc_work_area(struct xa_window *wind)
{
// 	struct xa_widget_row *rows = wind->widg_rows;
// 	RECT r = wind->rc;
	int thin = wind->thinwork;
	int frame = wind->frame;
	int t_margin, b_margin, l_margin, r_margin;
	short wa_borders = 0;
	bool shaded = wind->window_status & XAWS_SHADED;
	
// 	wind->wa = r;

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

	wind->outer = wind->rc;
	wind->wadelta = wind->bd;

	if (frame >= 0)
	{
		wind->wadelta.x += frame;
		wind->wadelta.y += frame;
		wind->wadelta.w += (frame << 1) + wind->x_shadow;
		wind->wadelta.h += (frame << 1) + wind->y_shadow;
		
		wind->outer.x += frame;
		wind->outer.y += frame;
		wind->outer.w -= (frame << 1) + wind->x_shadow;
		wind->outer.h -= (frame << 1) + wind->y_shadow;
	}
	if (frame < 0)
		frame = 0;

	wind->inner = f2w(&wind->bd, &wind->outer, true);

	wind->ba.x = wind->r.x + frame;
	wind->ba.y = wind->r.y + frame;
	frame <<= 1;
	wind->ba.w = wind->r.w - frame;
	wind->ba.h = wind->r.h - frame;

	rp_2_ap_row(wind);

	if (wind->inner.y == wind->outer.y && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.y += t_margin;
		wind->wadelta.h += t_margin;
		wa_borders |= WAB_TOP;
	}

	if (wind->inner.x == wind->outer.x && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.x += l_margin;
		wind->wadelta.w += l_margin;
		wa_borders |= WAB_LEFT;
	}

	if ((wind->inner.y + wind->inner.h) == (wind->outer.y + wind->outer.h) && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.h += b_margin;
		wa_borders |= WAB_BOTTOM;
	}

	if ((wind->inner.x + wind->inner.w) == (wind->outer.x + wind->outer.w) && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.w += r_margin;
		wa_borders |= WAB_RIGHT;
	}

	if (wind->frame >= 0 && !wind->thinwork)
	{
		wind->wadelta.x += 2, wind->wadelta.y += 2;
		wind->wadelta.w += 4, wind->wadelta.h += 4;
	}

	wind->wa_borders = wa_borders;
	wind->delta = wind->wadelta;
	wind->wa = f2w(&wind->wadelta, &wind->rc, true);

	if ((wind->active_widgets & TOOLBAR) && !(get_widget(wind, XAW_TOOLBAR)->m.properties & WIP_NOTEXT))
	{
		struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);

		rp_2_ap(wind, widg, &widg->ar);

		wind->delta.y += widg->ar.h;
		wind->delta.h += widg->ar.h;
		wind->rwa = f2w(&wind->delta, &wind->rc, true);
	}
	else
	{
		wind->rwa = wind->wa;
	}

	if (shaded)
	{
		wind->outer = wind->r;
		if ((frame = wind->frame) >= 0)
		{
			wind->outer.x += frame;
			wind->outer.y += frame;
			wind->outer.w -= (frame << 1) + wind->x_shadow;
			wind->outer.h -= (frame << 1) + wind->y_shadow;
		}
		wind->inner = f2w(&wind->rbd, &wind->outer, true);
	}

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
// 				if (d) display(" free_xawidget_re: stuff is wt=%lx (links=%d) in widg=%lx",
// 					wt, wt->links, widg);
				
				if (!remove_wt(wt, false))
				{
					/* Ozk: 
					 * Since the window deletion can happen after a wt is reused for
					 * another window, we must check if it is infact still attached
					 * to the window we remove it from. Fixes gfa_xref problems!
					 */
					if (wt->widg == widg)
						wt->widg = NULL;
					if (wt->wind == widg->wind)
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
					widg->stuff = NULL;
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
	widg->stuff	= NULL;
 	widg->m.r.draw	= NULL;
	widg->m.click	= NULL;
	widg->m.drag	= NULL;
	widg->m.properties = 0;
}

#if 0
/*
 * Define the widget locations using window relative coordinates.
 */

/* needed a dynamic margin (for windowed list boxes) */
/* eliminated both margin and shadow sizes from this table */
/* put some extra data there as well. */
static XA_WIDGET_LOCATION
/*             defaults              index        mask	     status				rsc	      properties  destruct */
/*							      mask									 */
stdl_close   = {LT,			XAW_CLOSE,   CLOSER,    XAWS_ICONIFIED,			WIDG_CLOSER, WIP_ACTIVE, free_xawidget_resources },
stdl_full    = {RT,			XAW_FULL,    FULLER,    XAWS_ICONIFIED,			WIDG_FULL,   WIP_ACTIVE, free_xawidget_resources },
stdl_iconify = {RT,			XAW_ICONIFY, ICONIFIER, 0,				WIDG_ICONIFY,WIP_ACTIVE, free_xawidget_resources },
stdl_hide    = {RT,			XAW_HIDE,    HIDER,     XAWS_ICONIFIED,			WIDG_HIDE,   WIP_ACTIVE, free_xawidget_resources },
stdl_title   = {LT | R_VARIABLE,	XAW_TITLE,   NAME,      0,				0,           WIP_ACTIVE, free_xawidget_resources },
stdl_notitle = {LT | R_VARIABLE,	XAW_TITLE,   NAME,      0,				0,                    0, free_xawidget_resources },
//stdl_resize  = {RB,			XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,	WIDG_SIZE,   WIP_ACTIVE, free_xawidget_resources },
stdl_resize  = {RB,			XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,	WIDG_SIZE,   WIP_ACTIVE, free_xawidget_resources },
stdl_nresize = {RT/*RB*/,		XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,	WIDG_SIZE,            0, free_xawidget_resources },
stdl_uscroll = {LT/*RT*/,		XAW_UPLN,    UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_UP,     WIP_ACTIVE, free_xawidget_resources },
stdl_upage   = {NO,			XAW_UPPAGE,  UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_vslide  = {LT | R_VARIABLE/*RT*/,	XAW_VSLIDE,  VSLIDE,    XAWS_SHADED|XAWS_ICONIFIED, 	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_nvslide = {LT | R_VARIABLE/*RT*/,	XAW_VSLIDE,  VSLIDE,    XAWS_SHADED|XAWS_ICONIFIED, 	0,                    0, free_xawidget_resources },
stdl_dpage   = {NO,			XAW_DNPAGE,  DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_dscroll = {RT/*RB*/,		XAW_DNLN,    DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_DOWN,   WIP_ACTIVE, free_xawidget_resources },
stdl_lscroll = {LB,			XAW_LFLN,    LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_LEFT,   WIP_ACTIVE, free_xawidget_resources },
stdl_lpage   = {NO,			XAW_LFPAGE,  LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_hslide  = {LB | R_VARIABLE,	XAW_HSLIDE,  HSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
//stdl_nhslide = {LB | R_VARIABLE,	XAW_HSLIDE,  HSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,	0,                    0, free_xawidget_resources },
stdl_rpage   = {NO,			XAW_RTPAGE,  RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_rscroll = {RB,			XAW_RTLN,    RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,	WIDG_RIGHT,  WIP_ACTIVE, free_xawidget_resources },
stdl_info    = {LT | R_VARIABLE,	XAW_INFO,    INFO,      XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_menu    = {LT | R_VARIABLE,	XAW_MENU,    XaMENU,    XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_pop     = {LT,			XAW_MENU,    XaPOP,     XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources },
stdl_border  = {0,			XAW_BORDER,  0,         XAWS_SHADED|XAWS_ICONIFIED,	0,           WIP_ACTIVE, free_xawidget_resources }
;

static XA_WIDGET_LOCATION *stdl[] = 
{
	&stdl_close,
	&stdl_full,
	&stdl_iconify,
	&stdl_hide,
	&stdl_title,
	&stdl_notitle,
	//stdl_resize
	&stdl_resize,
	&stdl_nresize,
	&stdl_uscroll,
	&stdl_upage,
	&stdl_vslide,
	&stdl_nvslide,
	&stdl_dpage,
	&stdl_dscroll,
	&stdl_lscroll,
	&stdl_lpage,
	&stdl_hslide,
	//stdl_nhslid
	&stdl_rpage,
	&stdl_rscroll,
	&stdl_info,
	&stdl_menu,
	&stdl_pop,
	&stdl_border,
	NULL
};
#endif

static struct xa_widget_row *
get_widget_row(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_widget_row *row = wind->widg_rows;

	while (row)
	{
		if (row->tp_mask & widg->m.r.tp)
			break;
		row = row->next;
	}
	return row;
}

static struct xa_widget_row *
get_prev_row(struct xa_widget_row *row, short lmask, short lvalid)
{
	struct xa_widget_row *r = row->prev;

	while (r)
	{
		if (r->start && (r->rel & lmask) == lvalid)
			break;
		r = r->prev;
	}

	return r;
}
static struct xa_widget_row *
get_next_row(struct xa_widget_row *row, short lmask, short lvalid)
{
	struct xa_widget_row *r = row->next;

	while (r)
	{
		if (r->start && (r->rel & lmask) == lvalid)
			break;
		r = r->next;
	}

	return r;
}
#if 0
static struct xa_widget_row *
get_last_row(struct xa_widget_row *row, short lmask, short lvalid, bool backwards)
{
	struct xa_widget_row *last = NULL;

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
#endif
/* Dont deleteme yet!! */
#if 0
static struct xa_widget_row *
get_first_row(struct xa_widget_row *row, short lmask, short lvalid, bool backwards)
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
reloc_widgets_in_row(struct xa_widget_row *row)
{
	struct xa_widget *widg = row->start;
	short lxy, rxy;

	if (row->rel & XAR_VERT)
	{
		lxy = rxy = 0;
		while (widg)
		{
			if (widg->m.r.pos_in_row & R_BOTTOM)
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
			widg = widg->next_in_row;
		}
	}
	else
	{
		lxy = rxy = 0;
		while (widg)
		{
			if (widg->m.r.pos_in_row & R_RIGHT)
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
			widg = widg->next_in_row;
		}
	}
}

static void
rp_2_ap_row(struct xa_window *wind)
{
	struct xa_widget_row *row = wind->widg_rows;
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
		r.w -= (frame + wind->x_shadow);
		r.h -= (frame + wind->y_shadow);
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
					if (widg->m.r.pos_in_row & R_VARIABLE)
						v++;

					switch ((widg->m.r.pos_in_row & (R_RIGHT|R_BOTTOM|R_CENTER)))
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
					widg = widg->next_in_row;
				}
				if (v)
				{
					widg = row->start;
					while (widg)
					{
						if (widg->m.r.pos_in_row & R_VARIABLE) //widg->loc.relative_type & R_VARIABLE)
						{
							widg->ar.w = widg->r.w = (l - widg->ar.x);
						}
						if (widg->m.r.xaw_idx == XAW_MENU)
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
						widg = widg->next_in_row;
					}
				}	
			}
			else
			{
				l = y2 - row->rxy;
				while (widg)
				{
					if (widg->m.r.pos_in_row & R_VARIABLE) //widg->loc.relative_type & R_VARIABLE)
						v++;

					switch ((widg->m.r.pos_in_row & (R_RIGHT|R_BOTTOM|R_CENTER))) //((widg->loc.relative_type & (R_RIGHT|R_BOTTOM|R_CENTER)))
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
					widg = widg->next_in_row;
				}
				if (v)
				{
					widg = row->start;
					while (widg)
					{
						if (widg->m.r.pos_in_row & R_VARIABLE) //widg->loc.relative_type & R_VARIABLE)
						{
							widg->ar.h = widg->r.h = (l - widg->ar.y);
						}
						if (widg->m.r.xaw_idx == XAW_MENU)
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
						widg = widg->next_in_row;
					}
				}
			}
		}
		row = row->next;
	}
}


static void
position_widget(struct xa_window *wind, struct xa_widget *widg, RECT *offsets)
{
	struct xa_widget_row *rows = wind->widg_rows;
	struct xa_widget_methods *m = &widg->m;

	if (m->r.pos_in_row & R_NONE)
		return;
	/*
	 * Find row in which this widget belongs, keeping track of Y coord
	 */
	rows = get_widget_row(wind, widg);

	if (rows)
	{
		struct xa_widget *nxt_w = rows->start;
		struct xa_widget_row *nxt_r;
		short diff;
		short orient = rows->rel & XAR_VERT;
		short placement = rows->rel & XAR_PM;
// 		bool d = (!strnicmp(wind->owner->proc_name, "ergo_hlp", 8) && !(wind->dial & created_for_CALC));

		/*
		 * See if this widget is already in this row
		 */
		while (nxt_w && nxt_w->next_in_row && nxt_w != widg && !(nxt_w->m.r.tp & widg->m.r.tp))
			nxt_w = nxt_w->next_in_row;

		if (nxt_w && (nxt_w != widg))
		{
// 			if (d) display("adding %lx to rownr %d", widg->m.r.tp, rows->rownr);

			/*
			 * Widget not in row, link it in
			 */
			if ((!orient && (widg->m.r.pos_in_row & R_RIGHT)) ||
			    ( orient && (widg->m.r.pos_in_row & R_BOTTOM)))
			{
				widg->next_in_row = rows->start;
				rows->start = widg;
			}
			else
			{
				nxt_w->next_in_row = widg;
				widg->next_in_row  = NULL;
			}
		}
		else if (!nxt_w)
		{
// 			if (d) display("row %d, tp = %lx", rows->rownr, widg->m.r.tp);
			/*
			 * This is the first widget in this row, and thus we also init its Y
			 */
			rows->start = widg;
			widg->next_in_row = NULL;
			rows->r.y = rows->r.x = rows->r.w = rows->r.h = rows->rxy = 0;

			if (!orient)	/* horizontal */
			{
				switch (placement)
				{
					case XAR_START:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.y = nxt_r->r.y + nxt_r->r.h;
						else
						{
							rows->r.y = offsets->y;
// 							wind->bd.y -= offsets->y;
// 							wind->bd.h -= offsets->h;
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_START))))
							rows->r.x = nxt_r->r.x + nxt_r->r.w;
						else
							rows->r.x = offsets->x;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_END))))
							rows->rxy = nxt_r->r.x;
						else
							rows->rxy = offsets->w;
						
						wind->ext_borders |= T_BORDER;
						break;
					}
					case XAR_END:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_END))))
							rows->r.y = nxt_r->r.y;
						else
						{
							rows->r.y = offsets->h;
// 							wind->bd.h -= offsets->h;
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_START))))
							rows->r.x = nxt_r->r.x + nxt_r->r.w;
						else
							rows->r.x = offsets->w;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT|XAR_END))))
							rows->rxy = nxt_r->r.x;
						else
							rows->rxy = offsets->w;
						wind->ext_borders |= B_BORDER;
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
						else
						{
							rows->r.x = offsets->x;
// 							wind->bd.x -= offsets->x;
// 							wind->bd.w -= offsets->w;
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.y = nxt_r->r.y + nxt_r->r.h;
						else
							rows->r.y = offsets->y;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), XAR_END)))
							rows->rxy = nxt_r->r.y;
						else
							rows->rxy = offsets->h;
						wind->ext_borders |= L_BORDER;
						break;
					}
					case XAR_END:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_END))))
							rows->r.x = nxt_r->r.x;
						else
						{
							rows->r.x = offsets->w;
// 							wind->bd.w -= offsets->w;
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.y = nxt_r->r.y + nxt_r->r.h;
						else
							rows->r.y = offsets->y;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), XAR_END)))
							rows->rxy = nxt_r->r.y;
						else
							rows->rxy = offsets->h;
						wind->ext_borders |= R_BORDER;
						break;
					}
				}
			}
		}	
		/*
		 * If callback to calc w/h of widget provided...
		 */
		if (m->r.setsize)
		{
			(*m->r.setsize)(wind, widg);
		}
		/*
		 * ... else just use standard w/h
		 */
		else
		{
			widg->r.w = screen.c_max_w; //cfg.widg_w;
			widg->r.h = screen.c_max_h; //cfg.widg_h;
		}
		/*
		 * If Widget height is larger than largest one in this row,
		 * adjust all widgets height, and all following rows Y coord...
		 */
		if (!orient)
		{
			m->r.pos_in_row &= (R_VARIABLE | R_RIGHT);
			if (placement == XAR_END)
				m->r.pos_in_row |= R_BOTTOM;

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
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, XAR_VERT)))
						{
							nxt_r->r.y += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.y += diff;
						wind->bd.h += diff;
						break;
					}
					case XAR_END:
					{
						rows->r.y += diff;
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), XAR_END)))
						{
							nxt_r->r.y += diff;
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, XAR_VERT)))
						{
							nxt_r->rxy += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.h += diff;	
						break;
					}
				}
			}
			else
				widg->r.h = rows->r.h;
			
// 			if (d) display("rownr %d, %d/%d/%d/%d", rows->rownr, rows->r);
		}
		else
		{
			XA_RELATIVE rt = m->r.pos_in_row & R_VARIABLE;

			if (m->r.pos_in_row & R_RIGHT)
				rt |= R_BOTTOM;
			if (placement == XAR_END)
				rt |= R_RIGHT;

			m->r.pos_in_row = rt;

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
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_START))))
						{
							nxt_r->r.x += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.x += diff;
						wind->bd.w += diff;
						break;
					}
					case XAR_END:
					{
						rows->r.x += diff;
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_END))))
						{
							nxt_r->r.x += diff;
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT), 0)))
						{
							nxt_r->rxy += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.w += diff;
						break;
					}
				}
			}
			else
				widg->r.w = rows->r.w;
		}
		reloc_widgets_in_row(rows);
	}
}
static XA_WIDGET *
make_widget(struct xa_window *wind,
	    struct xa_widget_methods *m,
	    struct xa_client *owner, RECT *offsets)
{
	XA_WIDGET *widg = get_widget(wind, m->r.xaw_idx);

	widg->m		= *m;
	widg->owner	 = owner;

	position_widget(wind, widg, offsets);

	return widg;
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
	else if (widg->m.destruct)
		(*widg->m.destruct)(widg);

	bzero(widg, sizeof(XA_WIDGET));

	widg->m.r.xaw_idx = n;

	if (stuff)
	{
		widg->stuff = stuff;
		widg->stufftype = st;
	}
}

static struct xa_widget_row *
create_widg_layout(struct xa_window *wind)
{
	XA_WIND_ATTR tp;
	struct nwidget_row *rows;
// 	struct xa_widget_theme *theme = wind->widget_theme;
	struct widget_theme *theme = wind->active_theme;
	int nrows;
	struct xa_widget_row *xa_rows, *ret = NULL;

	rows = theme->layout;
	nrows = 0;

	while (rows->tp_mask != -1)
		rows++, nrows++;

	if (nrows && (xa_rows = kmalloc( (long)sizeof(*xa_rows) * nrows)))
	{
		int rownr = 0;
		struct xa_widget_row *p = NULL;

		rows = theme->layout;
		ret = xa_rows;

		while ((tp = rows->tp_mask) != -1)
		{
			xa_rows->rel = rows->rel;
			xa_rows->tp_mask = tp;
			
			xa_rows->start = NULL;
			xa_rows->rownr = rownr++;
			xa_rows->r = (RECT){0, 0, 0, 0};
			xa_rows->rxy = 0;
			
			rows++;
			xa_rows->prev = p;
			if (rows->tp_mask != -1)
				xa_rows->next = xa_rows + 1;
			else
				xa_rows->next = NULL;
			
			p = xa_rows;
			xa_rows++;	
		}
	}
	return ret;
}

struct xa_widget_methods def_methods[] =
{
/*Properties, statusmask           render_widget click		drag		release	wheel	key		destruct	*/
 {0,0,					{0},	NULL,		NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* Dummy */
 {0,0,					{0},	NULL,		drag_border,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* Border */
 {0,0,					{0},	click_title,	drag_title,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* title */
 {0,XAWS_ICONIFIED,			{0},	click_close,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* closer */
 {0,XAWS_ICONIFIED,			{0},	click_full,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* fuller */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_title,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* info */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	NULL,		drag_resize,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* resize */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* uparrow */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* dnarrow */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	NULL,		drag_vslide,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* vslide */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* lfarrow */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* rtarrow */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	NULL,		drag_hslide,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* hslide */
 {0,0,					{0},	click_iconify,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* iconify */
 {0,XAWS_ICONIFIED,			{0},	click_hide,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* hide */

 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	NULL,		NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* toolbar */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	NULL,		NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* menu */

/*Properties, statusmask                                render_widget            click		drag		release	wheel	key	  destruct	*/
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{MOVER,	NULL, XAW_MOVER,  NO, NULL,NULL,NULL}, NULL,		NULL,		NULL,	NULL,	NULL,	free_xawidget_resources },/* Mover */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0,	NULL, XAW_UPPAGE, NO, NULL,NULL,NULL}, click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources },/* Mover */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0,	NULL, XAW_DNPAGE, NO, NULL,NULL,NULL}, click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources },/* Mover */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0,	NULL, XAW_LFPAGE, NO, NULL,NULL,NULL}, click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources },/* Mover */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0,	NULL, XAW_RTPAGE, NO, NULL,NULL,NULL}, click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources },/* Mover */

};

static void
init_slider_widget(struct xa_window *wind, struct xa_widget *widg, short slider_idx, RECT *offsets)
{
	struct xa_widget *w;
	XA_SLIDER_WIDGET *sl;

	/* Ozk: Bad mem leak here. If widget got slider info, we keep this
	 */
	if (!widg->stuff)
	{
		sl = kmalloc(sizeof(*sl));
	
		assert(sl);
		widg->stuff = sl;
		sl->length = SL_RANGE;
		widg->flags |= XAWF_STUFFKMALLOC;
							
		if (slider_idx == XAW_VSLIDE)
		{
			DIAGS(("Make vslide (uparrow)"));
			w = make_widget(wind, &def_methods[XAW_UPPAGE + 1], NULL, offsets);
			w->arrowx = WA_UPPAGE;
			w->xarrow = WA_DNPAGE;
			w->limit = 0;
			w->xlimit = SL_RANGE;
			w->slider_type = XAW_VSLIDE;
		
			DIAGS(("Make vslide (dnarrow)"));
			w = make_widget(wind, &def_methods[XAW_DNPAGE + 1], NULL, offsets);
			w->arrowx = WA_DNPAGE;
			w->xarrow = WA_UPPAGE;
			w->limit = SL_RANGE;
			w->xlimit = 0;
			w->slider_type = XAW_VSLIDE;
		}
		else
		{
			DIAGS(("Make hslide (lfarrow)"));
			w = make_widget(wind, &def_methods[XAW_LFPAGE + 1], NULL, offsets);
			w->arrowx = WA_LFPAGE;
			w->xarrow = WA_RTPAGE;
			w->limit = 0;
			w->xlimit = SL_RANGE;
			w->slider_type = XAW_HSLIDE;
		
			DIAGS(("Make hslide (rtarrow)"));
			w = make_widget(wind, &def_methods[XAW_RTPAGE + 1], NULL, offsets);
			w->arrowx = WA_RTPAGE;
			w->xarrow = WA_LFPAGE;
			w->limit = SL_RANGE;
			w->xlimit = 0;
			w->slider_type = XAW_HSLIDE;	
		}
	}
}
							

/* 
 * Setup the required 'standard' widgets for a window. These are the ordinary GEM
 * behaviours. These can be changed for any given window if you want special behaviours.
 */
//position_widget(struct xa_widget *w, struct xa_widget_row *strt_rows, void(*widg_size)(struct xa_widget *widg))
void
standard_widgets(struct xa_window *wind, XA_WIND_ATTR tp, bool keep_stuff)
{
	struct widget_theme *theme = wind->active_theme;
	XA_WIND_ATTR utp = 0;

	DIAGS(("standard_widgets: new(%lx), prev(%lx) on wind %d for %s",
		tp, wind->active_widgets, wind->handle, wind->owner->proc_name));

	if (!wind->widg_rows)
		wind->widg_rows = create_widg_layout(wind);
	else
	{
		struct xa_widget_row *rows = wind->widg_rows;
		while (rows)
		{
			rows->start = NULL;
			rows->r = (RECT){0, 0, 0, 0};
			rows = rows->next;
		}
	}
	{
		struct nwidget_row *rows = theme->layout;
		XA_WIND_ATTR rtp, this_tp, *tp_deps;
		struct xa_widget *widg;
		short xaw_idx;
		struct xa_widget_methods dm;
		RECT *offsets;
		RECT r_ofs;

		wind->ext_borders = 0;
		if (wind->frame >= 0 && (wind->draw_canvas = theme->draw_canvas))
		{
			offsets = &theme->outer;
			wind->bd.x = offsets->x + theme->inner.x;
			wind->bd.y = offsets->y + theme->inner.y;
			wind->bd.w = offsets->x + offsets->w + theme->inner.x + theme->inner.w;
			wind->bd.h = offsets->y + offsets->h + theme->inner.y + theme->inner.h;
			wind->rbd = wind->bd;
		}
		else
		{
			wind->bd = wind->rbd = r_ofs = (RECT){0,0,0,0};
			wind->draw_canvas = NULL;
			offsets = &r_ofs;
		}
		
		wind->draw_waframe = theme->draw_waframe;

		dm = def_methods[XAW_BORDER + 1];
		dm.r = theme->border;
		dm.r.pos_in_row = NO;
		if ((tp & BORDER) || (wind->frame > 0 && (tp & SIZER)))
		{
			dm.properties = WIP_INSTALLED|WIP_ACTIVE;
			utp |= BORDER;
		}
		else
			dm.properties = WIP_INSTALLED;

		(void)make_widget(wind, &dm, NULL, offsets);

		while ((rtp = rows->tp_mask) != -1)
		{
			struct render_widget **rw = rows->w;
			rtp &= THEME_WIDGETS;
			while (rtp && *rw)
			{
				this_tp = (*rw)->tp;
				xaw_idx = (*rw)->xaw_idx;

				if ( (this_tp & tp & rtp) )
				{
					rtp &= ~this_tp;
					utp |= this_tp;
					dm = def_methods[xaw_idx + 1];
					dm.r = *(*rw);
					dm.properties = dm.r.draw ? WIP_ACTIVE|WIP_INSTALLED : 0;
					widg = make_widget(wind, &dm, NULL, offsets);

					switch (xaw_idx)
					{
						case XAW_VSLIDE:
						case XAW_HSLIDE:
						{
							init_slider_widget(wind, widg, xaw_idx, offsets);
							break;
						}
						case XAW_UPLN:
						case XAW_DNLN:
						{
							widg->slider_type = XAW_VSLIDE;
							if (xaw_idx == XAW_UPLN)
							{
								widg->arrowx = WA_UPLINE;
								widg->xarrow = WA_DNLINE;
								widg->limit = 0;
								widg->xlimit = SL_RANGE;
							}
							else
							{
								widg->arrowx = WA_DNLINE;
								widg->xarrow = WA_UPLINE;
								widg->limit = SL_RANGE;
								widg->xlimit = 0;
							}
							break;
						}
						case XAW_LFLN:
						case XAW_RTLN:
						{
							widg->slider_type = XAW_HSLIDE;
							if (xaw_idx == XAW_LFLN)
							{
								widg->arrowx = WA_LFLINE;
								widg->xarrow = WA_RTLINE;
								widg->limit = 0;
								widg->xlimit = SL_RANGE;
							}
							else
							{
								widg->arrowx = WA_RTLINE;
								widg->xarrow = WA_LFLINE;
								widg->limit = SL_RANGE;
								widg->xlimit = 0;
							}
							break;
						}
						default:;
					}
					this_tp = 0;
				}
				else if ((tp_deps = (*rw)->tp_depends))
				{
					while (tp_deps[0])
					{
						if ((tp & tp_deps[0]) && (tp & tp_deps[1]))
						{
							struct render_widget *unused = &theme->exterior;
							dm = def_methods[0];
							dm.r = *(*rw);
							dm.properties = WIP_INSTALLED;
							dm.statusmask = def_methods[xaw_idx + 1].statusmask;
							
							dm.r.draw = unused->draw;
							widg = make_widget(wind, &dm, NULL, offsets);
							this_tp = 0;
// 							if (!(wind->dial & created_for_CALC))
// 								display("install notused widget-drawing for %lx", this_tp);
						}
						tp_deps += 2;
					}
				}
				
				if ((this_tp & rtp))
					wind->widgets[xaw_idx].m.properties &= ~(WIP_INSTALLED|WIP_ACTIVE);
				
				rw++;
			}
			rows++;
		}
	}

	tp &= ~THEME_WIDGETS;
	utp &= THEME_WIDGETS;
	wind->active_widgets = (tp | utp);
}

static bool
display_toolbar(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	XA_TREE *wt = widg->stuff;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, NULL);

	DIAG((D_form,wind->owner,"display_object_widget(wind=%d), wt=%lx, e.obj=%d, e.pos=%d",
		wind->handle, wt, wt->e.obj, wt->e.pos));

	draw_object_tree(0, wt, NULL, wind->vdi_settings, widg->start, 100, NULL, 0);

	return true;
}

/*
 * HR: Direct display of the toolbar widget; HR 260102: over the rectangle list.
 */
void
redraw_toolbar(enum locks lock, struct xa_window *wind, short item)
{
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_rect_list *rl;
	RECT r;

	hidem();
	widg->start = item;

	rl = wind->rect_list.start;
	while (rl)
	{			
		if (xa_rect_clip(&rl->r, &wind->wa, &r))
		{
			(*v->api->set_clip)(v, &r);
			widg->m.r.draw(wind, widg, &r);
		}
		rl = rl->next;
	}
	(*v->api->clear_clip)(v);
	showm();
	widg->start = wind->winitem;
}

void
set_toolbar_coords(struct xa_window *wind, const RECT *r)
{
	struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);

	widg->r.x = wind->wa.x - wind->r.x - (wind->frame <= 0 ? 0 : wind->frame);
	widg->r.y = wind->wa.y - wind->r.y - (wind->frame <= 0 ? 0 : wind->frame);
	if (r)
	{
		widg->r.w = r->w;
		widg->r.h = r->h;
	}
	else
	{
		widg->r.w = wind->wa.w;
		widg->r.h = wind->wa.h;
	}
}

void
set_toolbar_handlers(const struct toolbar_handlers *th, struct xa_window *wind, struct xa_widget *widg, struct widget_tree *wt)
{
	if (widg)
	{
		if (th && th->click)
		{
			if ((long)th->click == -1L)
				widg->m.click	= NULL;
			else
				widg->m.click	= th->click;
		}
		else
			widg->m.click	= Click_windowed_form_do;

		if (th && th->drag)
		{
			if ((long)th->drag == -1L)
				widg->m.drag	= NULL;
			else
				widg->m.drag	= th->drag;
		}
		else
			widg->m.drag	= Click_windowed_form_do;
		
		if (th && th->draw)
		{
			if ((long)th->draw == -1L)
			{
				widg->m.r.draw	= NULL;
				widg->m.properties &= ~WIP_INSTALLED;
			}
			else
			{
				widg->m.r.draw	= th->draw;
				widg->m.properties |= WIP_INSTALLED;
			}
		}
		else
		{
			widg->m.r.draw	= display_toolbar;
			widg->m.properties |= WIP_INSTALLED;
		}
		
		if (th && th->destruct)
		{
			if ((long)th->destruct == -1L)
				widg->m.destruct = NULL;
			else
				widg->m.destruct = th->destruct;
		}
		else
			widg->m.destruct	= free_xawidget_resources;
		
		if (th && th->release)
		{
			if ((long)th->release == -1L)
				widg->m.release = NULL;
			else
				widg->m.release	= th->release;
		}
		else
			widg->m.release	= NULL;
	}

	if (wind)
	{
		if (wt && (wt->ei || wt->e.obj >= 0 || obtree_has_default(wt->tree)))
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
		bool zen,
		const struct toolbar_handlers *th,
		const RECT *r)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	XA_TREE *wt;
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);

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
	wt->zen  = zen;
	wt->links++;
	//display("set_toolbar_widg: link++ on %lx (links=%d)", wt, wt->links);

	/*
	 * Ozk: if edobj == -2, we want to look for an editable and place the
	 * cursor there. Used by wdlg_create() atm
	 */
	if (edobj == -2)
		edobj = ob_find_any_flst(obtree, OF_EDITABLE, 0, 0, OS_DISABLED, 0, 0);

	if (!obj_edit(wt, v, ED_INIT, edobj, 0, -1, NULL, false, NULL, NULL, NULL, &edobj))
		obj_edit(wt, v, ED_INIT, edobj, 0, -1, NULL, false, NULL, NULL, NULL, NULL);

	widg->m.properties = properties | WIP_WACLIP | WIP_ACTIVE;
	set_toolbar_handlers(th, wind, widg, wt);
	
	/* HR 280801: clicks are now put in the widget struct.
	      NB! use this property only when there is very little difference between the 2 */

	widg->state	= OS_NORMAL;
	widg->stuff	= wt;
	widg->stufftype	= STUFF_IS_WT;
	widg->start	= 0;
	wind->tool	= widg;

	widg->m.statusmask = XAWS_SHADED;
	widg->m.r.pos_in_row = LT;
	widg->m.r.tp = TOOLBAR;
	widg->m.r.xaw_idx = XAW_TOOLBAR;
	wind->active_widgets |= TOOLBAR;
	set_toolbar_coords(wind, r);

	return wt;
}

void
remove_widget(enum locks lock, struct xa_window *wind, int tool)
{
	XA_WIDGET *widg = get_widget(wind, tool);

	DIAG((D_form, NULL, "remove_widget %d: 0x%lx", tool, widg->stuff));
	//display("remove_widget %d: 0x%lx", tool, widg->stuff);

	if (widg->m.destruct)
		(*widg->m.destruct)(widg);
	else
	{
		widg->stufftype	= 0;
		widg->stuff   = NULL;
 		widg->m.r.draw = NULL;
		widg->m.click   = NULL;
		widg->m.drag    = NULL;
		widg->m.properties = 0;
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
			return XAW_LFPAGE;

		/* dn arrow */
		if (sl->position < SL_RANGE && click > x2)
			return XAW_RTPAGE;
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
			return XAW_UPPAGE;

		if (sl->position < SL_RANGE && click > y2)
			return XAW_DNPAGE;
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
	WINDOW_STATUS winstatus = w->window_status;
	bool inside = false;

	if (m_inside(x, y, &w->r))
	{
		/* Scan through widgets to find the one we clicked on */
		for (f = 0; f < XA_MAX_WIDGETS; f++)
		{
			if (!is_page(f))
			{
				RECT r;

				widg = w->widgets + f;
				
				if (!(winstatus & widg->m.statusmask) && wdg_is_act(widg))	/* Is the widget in use? */
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
	WINDOW_STATUS winstatus = w->window_status;

	if (!(m_inside(md->x, md->y, &w->r)))
		return false;

	clicks = md->clicks;
	if (clicks > 2)
		clicks = 2;

	/* Scan through widgets to find the one we clicked on */
	for (f = 0; f < XA_MAX_WIDGETS; f++)
	{
		/* HR 060501: just skip page arrows. These are embedded in the slider background together with the slider itself. */
		if (!is_page(f))
		{
			RECT r;

			widg = w->widgets + f;
			
			if (!(winstatus & widg->m.statusmask) && wdg_is_act(widg))	/* Is the widget in use? */
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
					short oldstate = -1;
					bool rtn = false;
					int ax = 0;
	
					widg->x = md->x - r.x; 		/* Mark where the click occurred (relative to the widget) */
					widg->y = md->y - r.y;

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
						widg = w->widgets + ax;
						widg->x = md->x - r.x; 			/* Mark where the click occurred (relative to the widget) */
						widg->y = md->y - r.y;
						if (widg->m.drag) rtn = widg->m.drag(lock, w, widg, md);	/* we know there is only 1 behaviour for these arrows */
						else		  rtn = true;
					}
					else /* normal widget */
					{
						short b = md->cstate, rx = md->x, ry = md->y;
						
						/* We don't auto select & pre-display for a menu or toolbar widget */
						if (f != XAW_MENU && f != XAW_TOOLBAR)
							oldstate = redisplay_widget(lock, w, widg, OS_SELECTED);

						/*
						 * Check if the widget has a dragger function if button still pressed
						*/
						if (b && f == XAW_TOOLBAR && (w->dial & created_for_TOOLBAR))
						{
							/*
							 * Special case; If click goes onto a userinstalled toolbar
							 * we need to give the mouse event to the client if the button
							 * is still being pressed, because a 'touchexit' object may
							 * return control to client immediately, and it may do
							 * graf_mkstate() to check if button is released. If we dont
							 * do this, client will always detect a button released...
							 */
							if (md->cstate)
								button_event(lock, w->owner, md);
							if (widg->m.click)
								rtn = widg->m.click(lock, w, widg, md);
						}
						if (b && (widg->m.properties & WIP_NODRAG))
						{
							if (widg->m.click)
								widg->m.click(lock, w, widg, md);
						}
						else if (b && widg->m.drag)
							rtn = widg->m.drag(lock, w, widg, md);
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
								short tx = rx, ty = ry;
								bool ins = 1;
								check_mouse(w->owner, &b, NULL, NULL);
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
							if (m_inside(rx, ry, &r) && widg->m.click)
							{
								rtn = widg->m.click(lock, w, widg, md);
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
							redisplay_widget(lock, w, widg, OS_NORMAL); //oldstate); //OS_NORMAL);	/* Flag the widget as de-selected */
					}
					else if (w == root_window && f == XAW_TOOLBAR)		/* HR: 280801 a little bit special. */
						return false;		/* pass click to desktop owner */

					/* click devoured by widget */
					return true;
				} /*if m_inside */
			} /* endif (!(winstatus & widg->m.statusmask) && widg->m.r.draw) */
		} /* endif (!is_page(f)) */
	} /* for f */

	/* Button click can be passed on to applications as we didn't use it for a widget */
	return false;
}

short
redisplay_widget(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, short state)
{
	short ret = widg->state;

	if (wdg_is_inst(widg))
	{
		widg->state = state;
		/* No rectangle list. */
		if ((wind->dial & created_for_SLIST) || (wind->nolist && !wind->rect_list.start))
		{
			struct xa_vdi_settings *v = wind->vdi_settings;
			(*v->api->set_clip)(v, &wind->r);
			hidem();
			/* Not on top, but visible */
			widg->m.r.draw(wind, widg, &wind->r);
			showm();
			(*v->api->clear_clip)(v);
		}
		else
			/* Display the selected widget */
			display_widget(lock, wind, widg, NULL);
	}
	return ret;
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
				set_winmouse(-1, -1);
			}
		}
	}
}

void
set_winmouse(short x, short y)
{
// 	short x, y;
	struct xa_window *wind;

	if (x == -1 && y == -1)
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

	if (!update_locked())
	{
	if (wind)
	{
		if (!(wo->status & CS_EXITING))
		{
			if (cfg.widg_auto_highlight)
			{
				struct xa_window *rwind = NULL;
				struct xa_widget *hwidg, *rwidg = NULL;

				checkif_do_widgets(0, wind, 0, x, y, &hwidg);
		
				if (hwidg && !hwidg->owner && wdg_is_inst(hwidg))
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
				{
					redisplay_widget(0, rwind, rwidg, OS_NORMAL);
				}
			}
		
			if (wind->active_widgets & (SIZER|BORDER))
			{
				widg = wind->widgets + XAW_RESIZE;
				if ( /*wind->frame > 0 && */
				    (wind->active_widgets & BORDER) &&
				   !(wind->widgets[XAW_BORDER].m.statusmask & status) &&
				    (!m_inside(x, y, &wind->ba)))
				{
					r = wind->r;
					shape = border_mouse[compass(16, x, y, r)];
				}
				else if (wind->active_widgets & SIZER)
				{
					widg = wind->widgets + XAW_RESIZE;
					if (!(widg->m.statusmask & status))
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
	}

	return shape;
}

