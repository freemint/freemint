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

#include "xa_types.h"

#include "widgets.h"
#include "xa_global.h"

#include "xaaeswdg.h"
#undef NUM_STRINGS
#undef NUM_IMAGES
#undef NUM_UD
#undef NUM_BB
#undef NUM_IB
#undef NUM_CIB
#undef NUM_TI
#undef NUM_OBS
#undef NUM_TREE
#undef NUM_FRSTR
#undef NUM_FRIMG
#include "xaaes.h"

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
#include "taskman.h"

#if WITH_BBL_HELP
#include "xa_bubble.h"
#endif
#include "xa_evnt.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_menu.h"
#include "xa_wdlg.h"

#include "win_draw.h"

#include "mint/signal.h"

/* The active widgets are intimately connected to the mouse.
 * There is only 1 mouse, so there is only need for 1 global structure.
 */
XA_PENDING_WIDGET widget_active = { NULL }; /* Pending active widget (if any) */

static void rp_2_ap_row(struct xa_window *wind);
static void free_wt(XA_TREE *wt);
static short	redisplay_widget(int lock, struct xa_window *wind, XA_WIDGET *widg, short state);

#if GENERATE_DIAGS
static const char *const t_widg[] =
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
	struct widget_theme *stheme = C.Aes->widget_theme->slist;

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

	if (stheme)
	{
		wtheme->slist = stheme;
		stheme->links++;
	}
	else
	{
		(*client->xmwt->new_theme)(client->wtheme_handle, WINCLASS_SLIST, &stheme);
		if ((wtheme->slist = stheme))
			stheme->links++;
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
		if ((theme = client->widget_theme->slist))
		{
			theme->links--;
			if (!theme->links)
				(*client->xmwt->free_theme)(client->wtheme_handle, &client->widget_theme->slist);
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
		XA_SLIDER_WIDGET *sl = widg->stuff.sl;
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

		ret = (sl->flags & SLIDER_UPDATE) ? 1 : 0;
	}
	return ret;
}

/* MI_IGN: don't care */
bool
m_inside(short x, short y, GRECT *o)
{
	if( x != MI_IGN && y != MI_IGN )
		return x >= o->g_x && y >= o->g_y && x < o->g_x+o->g_w && y < o->g_y+o->g_h;

	if( y != MI_IGN )
		return y >= o->g_y && y < o->g_y+o->g_h;

	if( x != MI_IGN )
		return x >= o->g_x && x < o->g_x+o->g_w;

	return false;
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
	xa_graf_mouse(wind->owner->mouse, wind->owner->mouse_form, wind->owner, false);
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
rp2ap_obtree(struct xa_window *wind, struct xa_widget *widg, GRECT *r)
{
	XA_TREE *wt;
	OBJECT *obtree = NULL;

	if (widg->m.r.xaw_idx == XAW_TOOLBAR || widg->m.r.xaw_idx == XAW_MENU)
	{
 		if (!(widg->m.properties & WIP_NOTEXT))
 		{
			if( wind != root_window || (cfg.menu_layout == 0 && widg->m.r.xaw_idx == XAW_MENU) )
 				widg->r.g_w = wind->r.g_w;
 		}
		if ((wt = widg->stuff.wt) != NULL)
		{
			obtree = wt->tree;
			if (obtree && wind != root_window)
			{
				obtree->ob_x = r->g_x;
				obtree->ob_y = r->g_y;

 				obtree->ob_width = widg->r.g_w;
 				obtree->ob_height = widg->r.g_h;

 				/* is it a good idea to change the object-coords incrementally? */
				if (!wt->zen)
				{
					obtree->ob_x += wt->ox;
					obtree->ob_y += wt->oy;
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
rp_2_ap(struct xa_window *wind, XA_WIDGET *widg, GRECT *r)
{
	GRECT dr;
	short rtx, rty, ww, wh;
	int frame = wind->frame;

	DIAG((D_form, NULL, "rp_2_ap: type=%s, widg=%lx, wt=%lx, obtree=%lx",
		t_widg[widg->m.r.xaw_idx], (unsigned long)widg, (unsigned long)widg->stuff.wt,
		widg->m.r.xaw_idx == XAW_TOOLBAR ? (unsigned long)widg->stuff.wt->tree : -1 ));

	if (widg->m.r.xaw_idx != XAW_TOOLBAR )
	{
		if (r && r != &widg->ar)
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
	rtx = widg->r.g_x + frame;
	rty = widg->r.g_y + frame;

	r->g_w = widg->r.g_w;
	r->g_h = widg->r.g_h;
	r->g_x = wind->r.g_x;
	r->g_y = wind->r.g_y;

	ww = wind->frame < 0 ? wind->r.g_w : wind->r.g_w - wind->x_shadow;
	wh = wind->frame < 0 ? wind->r.g_h : wind->r.g_h - wind->y_shadow;

	switch ((widg->m.r.pos_in_row & (R_BOTTOM|R_RIGHT|R_CENTER)))
	{
	case LT:
		r->g_x += rtx;
		r->g_y += rty;
		break;
	case LB:
		r->g_x += rtx;
		r->g_y += (wh - rty);
		break;
	case RT:
		r->g_x += (ww - rtx);
		r->g_y += rty;
		break;
	case RB:
		r->g_x += (ww - rtx);
		r->g_y += (wh - rty);
		break;
	case CT:
		r->g_x += (ww - r->g_w) / 2;
		r->g_y += rty;
		break;
	case CR:
		r->g_x += (ww - r->g_w - rtx);
		r->g_y += (wh - r->g_h) / 2;
	case CB:
		r->g_x += (ww - r->g_w) / 2;
		r->g_y += (wh - r->g_h - rty);
		break;
	case CL:
		r->g_x += rtx;
		r->g_y += (wh - r->g_h - rty);
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
rp_2_ap_cs(struct xa_window *wind, XA_WIDGET *widg, GRECT *r)
{
	short rtx, rty, ww, wh;
	int frame = wind->frame;

	DIAG((D_form, NULL, "rp_2_ap: type=%s, widg=%lx, wt=%lx, obtree=%lx",
		t_widg[widg->m.r.xaw_idx], (unsigned long)widg, (unsigned long)widg->stuff.wt,
		widg->m.r.xaw_idx == XAW_TOOLBAR ? (unsigned long)widg->stuff.wt->tree : -1 ));

	if (widg->m.r.xaw_idx != XAW_TOOLBAR)
	{
		if (r && r != &widg->ar)
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
	rtx = widg->r.g_x + frame;
	rty = widg->r.g_y + frame;

	r->g_w = widg->r.g_w;
	r->g_h = widg->r.g_h;
	r->g_x = wind->r.g_x;
	r->g_y = wind->r.g_y;

	ww = wind->frame < 0 ? wind->r.g_w : wind->r.g_w - wind->x_shadow;
	wh = wind->frame < 0 ? wind->r.g_h : wind->r.g_h - wind->y_shadow;

	switch ((widg->m.r.pos_in_row & (R_BOTTOM|R_RIGHT|R_CENTER)))
	{
	case LT:
		r->g_x += rtx;
		r->g_y += rty;
		break;
	case LB:
		r->g_x += rtx;
		r->g_y += (wh - rty);
		break;
	case RT:
		r->g_x += (ww - rtx);
		r->g_y += rty;
		break;
	case RB:
		r->g_x += (ww - rtx);
		r->g_y += (wh - rty);
		break;
	case CT:
		r->g_x += (ww - r->g_w) / 2;
		r->g_y += rty;
		break;
	case CR:
		r->g_x += (ww - r->g_w - rtx);
		r->g_y += (wh - r->g_h) / 2;
	case CB:
		r->g_x += (ww - r->g_w) / 2;
		r->g_y += (wh - r->g_h - rty);
		break;
	case CL:
		r->g_x += rtx;
		r->g_y += (wh - r->g_h - rty);
		break;
	default:;
	}
}

XA_TREE * _cdecl
obtree_to_wt(struct xa_client *client, OBJECT *obtree)
{
	XA_TREE *wt = NULL;

	if (!client) client = C.Aes;

	DIAGS(("obtree_to_wt: look for wt with obtree=%lx for %s",
		(unsigned long)obtree, client->name));

	if (client->fmd.wt && obtree == client->fmd.wt->tree)
	{
		DIAGS((" -- found in client->wt"));
		wt = client->fmd.wt;
	}

	if (!wt && client->fmd.wind)
	{
		XA_WIDGET *widg = get_widget(client->fmd.wind, XAW_TOOLBAR);

		if (obtree == widg->stuff.wt->tree)
		{
			DIAGS((" -- found in XAW_TOOLBAR fmd wind %d - %s",
				client->fmd.wind->handle, client->name));
			wt = widg->stuff.wt;
		}
	}

	if (!wt && client->alert)
	{
		XA_WIDGET *widg = get_widget(client->alert, XAW_TOOLBAR);

		if (obtree == widg->stuff.wt->tree)
		{
			DIAGS((" -- found in XAW_TOOLBAR fmd wind %d - %s",
				client->alert->handle, client->name));
			wt = widg->stuff.wt;
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
		if (!wt)
		{
			DIAGS((" -- lookup failed"));
		}
	}
	DIAGS((" obtree_to_wt: return %lx for %s",
		(unsigned long)wt, client->name));


	return wt;
}

void _cdecl
init_widget_tree(struct xa_client *client, struct widget_tree *wt, OBJECT *obtree)
{
	short sx, sy;
	GRECT r;
	struct xa_data_hdr *h;

	if (!client) client = C.Aes;

	bzero(wt, sizeof(*wt));

	wt->tree = obtree;
	wt->owner = client;

	wt->objcr_api = client->objcr_api;

	h = client->objcr_theme;
	wt->objcr_theme = h;

	clear_focus(wt);
	clear_edit(&wt->e);
	wt->ei = NULL;

	sx = obtree->ob_x;
	sy = obtree->ob_y;
	obtree->ob_x = 100;
	obtree->ob_y = 100;

	obj_area(wt, aesobj(obtree, 0), &r);

	wt->ox = obtree->ob_x - r.g_x;
	wt->oy = obtree->ob_y - r.g_x;

	if (wt->ox < 0)
		wt->ox = 0;
	if (wt->oy < 0)
		wt->oy = 0;

	obtree->ob_x = sx;
	obtree->ob_y = sy;

	wt->next = client->wtlist;
	client->wtlist = wt;


	if ((obtree[3].ob_type & 0xff) == G_TITLE)
		wt->is_menu = true;
}

XA_TREE * _cdecl
new_widget_tree(struct xa_client *client, OBJECT *obtree)
{
	XA_TREE *new;

	if (!client) client = C.Aes;

	new = kmalloc(sizeof(*new));

	DIAGS((" === Create new widget tree - obtree=%lx, for %s",
		(unsigned long)obtree, client->name));

	if (new)
	{
		init_widget_tree(client, new, obtree);
		new->flags |= WTF_ALLOC;
	}
	DIAGS((" return new wt=%lx", (unsigned long)new));
	return new;
}

/* Ozk:
 * This will also free statically declared widget_tree's
 */
void
free_wtlist(struct xa_client *client)
{
	XA_TREE *wt;


	while (client->wtlist)
	{
		wt = client->wtlist;
#if 0
		if (wt->owner != client)
			BLOG((false, "THIS IS PERVERSE! wt=%lx, cl='%s', wtown='%s'", wt, client->name, wt->owner->name));
		if (client == C.Hlp || client == C.Aes)
			BLOG((false, "free_wtlist: %lx for %s", wt, client->name));
#endif
		client->wtlist = wt->next;
		wt->flags &= ~WTF_STATIC;
		free_wt(wt);
#if 0
		if (client == C.Hlp || client == C.Aes)
			BLOG((false, " done...(wtlist = %lx)", client->wtlist));
#endif
	}
}
/* Unhook a widget_tree from its owners wtlist
 */
static void
remove_from_wtlist(XA_TREE *wt)
{
	XA_TREE **nwt;

	if (wt->flags & WTF_STATIC)
	{
		DIAGS(("remove_from_wtlist: wt=%lx declared as static - not removed",
			(unsigned long)wt));
		return;
	}

	nwt = &wt->owner->wtlist;

	while (*nwt)
	{
		if (*nwt == wt)
		{
			*nwt = wt->next;
			wt->next = NULL;

			DIAGS(("remove_from_wtlist: removed wt=%lx from %s list",
				(unsigned long)wt, wt->owner->name));

			return;
		}
		nwt = &(*nwt)->next;
	}

	DIAGS(("remove_from_wtlist: wt=%lx not in %s wtlist",
		(unsigned long)wt, wt->owner->name));
}

static void
free_wt(XA_TREE *wt)
{
	DIAGS(("free_wt: wt=%lx", (unsigned long)wt));
#if 0
	if (wt->links)
	{
		BLOG(( 0, "free_wt: links not NULL!! on wt %lx", (unsigned long)wt));
		BLOG((0, "free_wt: links=%d, flags=%lx, ismenu=%s, owner=%s",
			wt->links, wt->flags,
			wt->is_menu ? "Yes" : "No",
			wt->owner->name));
		BLOG((0, "free_wt: C.Aes = %lx, C.Hlp = %lx", (unsigned long)C.Aes, (unsigned long)C.Hlp));
	}
#endif
	if (wt->flags & WTF_STATIC)
	{
		DIAGS(("free_wt: Declared as static!"));
		return;
	}

	if (wt->extra && (wt->flags & WTF_XTRA_ALLOC))
	{
		DIAGS(("  --- freed extra %lx", (unsigned long)wt->extra));
		kfree(wt->extra);
	}
	else {
		DIAGS(("  --- extra not alloced"));
	}
	if (wt->tree && (wt->flags & WTF_TREE_ALLOC))
	{
		if (wt->flags & WTF_TREE_CALLOC)
		{
			DIAGS(("  --- kfreed obtree %lx", (unsigned long)wt->tree));

			free_object_tree(wt->owner, wt->tree);
		}
		else
		{
			DIAGS(("  --- ufreed obtree %lx", (unsigned long)wt->tree));
			free_object_tree(C.Aes, wt->tree);
		}
		wt->tree = NULL;
	}
	else {
		DIAGS(("  --- obtree not alloced"));
	}
	if (wt->lbox)
	{
		kfree(wt->lbox);
		wt->lbox = NULL;
	}
	if (wt->flags & WTF_ALLOC)
	{
		DIAGS(("  --- freed wt=%lx", (unsigned long)wt));
	#if 0
		if (wt->objcr_api)
		{
			struct xa_data_hdr *h = wt->objcr_theme;

		}
	#endif
		kfree(wt);
	}
	else
	{
		struct xa_client *client = wt->owner;
		void *s[2];
		DIAGS(("  --- wt=%lx not alloced", (unsigned long)wt));
		s[0] = wt->objcr_api;
		s[1] = wt->objcr_theme;
		wt->next = NULL;
		wt->objcr_api = s[0];
		wt->objcr_theme = s[1];
		wt->owner = client;
	}
}

bool _cdecl
remove_wt(XA_TREE *wt, bool force)
{

	if (force || (wt->flags & (WTF_STATIC|WTF_AUTOFREE)) == WTF_AUTOFREE)
	{
		if (force || wt->links == 0)
		{
			remove_from_wtlist(wt);
			free_wt(wt);
			return true;
		}
	}

	return false;
}

/*
 * If rl is not NULL, we use that rectangle list instead of the rectlist
 * of the window containing the widget. Used when redrawing widgets of
 * SLIST windows, for example, that are considered to be objects along
 * the lines of AES Object trees,
 */
static void
draw_widget(struct xa_window *wind, XA_WIDGET *widg, struct xa_rect_list *rl)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	GRECT r;

	DIAG((D_wind, wind->owner, "draw_widget: wind handle %d, widget %d (func=%lx)",
		wind->handle, widg->m.r.xaw_idx, (unsigned long)widg->m.r.draw));

	if (!rl)
		rl = wind->rect_list.start;

	if (rl)
	{
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
				if( wind == root_window || !cfg.menu_bar || cfg.menu_layout != 0 || ( rl->r.g_y > get_menu_height() || !clip_off_menu( &rl->r )) )
				{
					if (xa_rect_clip(&rl->r, &widg->ar, &r))
					{
						(*v->api->set_clip)(v, &r);
						if( !cfg.menu_ontop && widg->m.r.xaw_idx != XAW_MENU && rl->r.g_y < get_menu_height() &&  rl->r.g_x < get_menu_widg()->r.g_w )
						{
							C.rdm = 1;
						}
						widg->m.r.draw(wind, widg, &rl->r);
					}
				}
			}
			rl = rl->next;
		}
		(*v->api->clear_clip)(v);
		showm();
	}
}

void
display_widget(int lock, struct xa_window *wind, XA_WIDGET *widg, struct xa_rect_list *rl)
{
	if (!(wind->window_status & widg->m.statusmask) && wdg_is_inst(widg))
	{
		draw_widget(wind, widg, rl);
#if 0
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
#endif
	}
}

static void
CE_redraw_menu(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		struct xa_client *mc;
		struct xa_widget *widg = get_menu_widg();

		mc = widg->stuff.wt->owner;

		/* in single-mode display only menu of single-app */
		if( C.SingleTaskPid > 0 && mc->p->pid != C.SingleTaskPid )
			return;

		if (ce->client == mc)
		{
			struct xa_rect_list rl = {0, {0, 0, 0, 0} };
			rl.r.g_w = get_menu_widg()->r.g_w;
			rl.r.g_h = get_menu_widg()->r.g_h;
			display_widget(lock, root_window, widg, &rl);
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
			Unblock(mc, 1, 11);
		}
	#endif
	}
}

/*
 * Ozk: redraw menu need to check the owner of the menu object tree
 * and draw it in the right context.
 */
void
redraw_menu(int lock)
{
	struct xa_client *rc, *mc;
	struct xa_widget *widg;

	if( cfg.menu_bar == 0 )
	{
		return;
	}
	rc = lookup_extension(NULL, XAAES_MAGIC);
	if (!rc)
		rc = C.Aes;

	DIAGS(("redraw_menu: rc = %lx, %s", (unsigned long)rc, rc->name));

	widg = get_menu_widg();
	DIAGS(("redaw_menu: widg = %lx", (unsigned long)widg));
	mc = widg->stuff.wt->owner;
	if( !mc )
	{
		mc = C.Aes;
	}
	DIAGS(("redaw_menu: widg owner = %s", mc->name));

	/* in single-mode display only menu of single-app */
	if( (C.SingleTaskPid > 0 && mc->p->pid != C.SingleTaskPid) )
		return;

	if (mc == rc || mc == C.Aes)
	{
		struct xa_rect_list rl = {0, {0, 0, 0, 0} };
		if (C.update_lock && C.update_lock != rc->p)
		{
			return;
		}
		/* menubar always top */
		rl.r.g_w = get_menu_widg()->r.g_w;
		rl.r.g_h = get_menu_widg()->r.g_h;

		display_widget(lock, root_window, widg, &rl);
	}
	else
	{
		DIAGS(("redraw_menu: post cevnt (%lx) to owner %s by %s", (unsigned long)CE_redraw_menu, mc->name, rc->name));
		post_cevent(mc,
			    CE_redraw_menu,
			    NULL,
			    NULL,
			    0, 0,
			    NULL,
			    NULL);
	}
	DIAGS(("redraw_menu - exit OK"));
}

/* Establish iconified window position from a simple ordinal. */
GRECT
iconify_grid(int i)
{
	short v_num = (root_window->wa.g_w - cfg.icnfy_l_x - cfg.icnfy_r_x) / cfg.icnfy_w;
	short h_num = (root_window->wa.g_h - cfg.icnfy_t_y - cfg.icnfy_b_y) / cfg.icnfy_h;
	short start_x, start_y, column, row;
	GRECT ic = {0,0,cfg.icnfy_w, cfg.icnfy_h};

	switch ((cfg.icnfy_orient & 0xff))
	{
		case 0:
		{
			ic.g_x = start_x = root_window->wa.g_x + cfg.icnfy_l_x;
			column = i / h_num;
			if (column)
			{
				ic.g_x += (column * cfg.icnfy_w);
				if ((ic.g_x + cfg.icnfy_w) > root_window->wa.g_w)
					ic.g_x = start_x;
			}
			row = i % h_num;
			if ((cfg.icnfy_orient & 0x100))
				ic.g_y = ((root_window->wa.g_y + root_window->wa.g_h) - cfg.icnfy_b_y - cfg.icnfy_h) - (row * cfg.icnfy_h);
			else
				ic.g_y = (root_window->wa.g_y + cfg.icnfy_t_y) + (row * cfg.icnfy_h);
			break;
		}
		case 1:
		{
			ic.g_x = start_x = root_window->wa.g_w - cfg.icnfy_w - cfg.icnfy_r_x;

			column = i / h_num;

			if (column)
			{
				ic.g_x -= (column * cfg.icnfy_w);
				if (ic.g_x < cfg.icnfy_l_x)
					ic.g_x = start_x;
			}
			row = i % h_num ;

			if ((cfg.icnfy_orient & 0x100))
				ic.g_y = ((root_window->wa.g_y + root_window->wa.g_h) - cfg.icnfy_b_y - cfg.icnfy_h) - (row * cfg.icnfy_h);
			else
				ic.g_y = (root_window->wa.g_y + cfg.icnfy_t_y) + (row * cfg.icnfy_h);

			break;
		}
		case 2:
		{
			ic.g_y = start_y = root_window->wa.g_y + cfg.icnfy_t_y;
			column = i % v_num;
			if ((cfg.icnfy_orient & 0x100))
			{
				ic.g_x = ((root_window->wa.g_x + root_window->wa.g_w) - cfg.icnfy_r_x - cfg.icnfy_w) - (column * cfg.icnfy_w);
			}
			else
			{
				ic.g_x = start_x = root_window->wa.g_x + cfg.icnfy_l_x + (column * cfg.icnfy_w);
			}
			row = i / v_num;
			if (row)
			{
				ic.g_y += (row * cfg.icnfy_h);
				if ((ic.g_y + cfg.icnfy_h + cfg.icnfy_b_y) > root_window->wa.g_h)
					ic.g_y = start_y;
			}
			break;
		}
		case 3:
		{
			ic.g_y = start_y = (root_window->wa.g_y + root_window->wa.g_h) - cfg.icnfy_h - cfg.icnfy_b_y;
			column = i % v_num;

			if ((cfg.icnfy_orient & 0x100))
			{
				ic.g_x = ((root_window->wa.g_x + root_window->wa.g_w) - cfg.icnfy_r_x - cfg.icnfy_w) - (column * cfg.icnfy_w);
			}
			else
			{
				ic.g_x = root_window->wa.g_x + cfg.icnfy_l_x + (column * cfg.icnfy_w);
			}

			row = i / v_num;
			if (row)
			{
				ic.g_y -= (row * cfg.icnfy_h);
				if (ic.g_y < (root_window->wa.g_y + cfg.icnfy_t_y))
					ic.g_y = start_y;
			}
			break;
		}

	}
	return ic;
}

/*
 * Click & drag on the title bar - does a move window
 */
static bool
drag_title(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	/* You can only move a window if its MOVE attribute is set */
	if (wind->active_widgets & MOVER)
	{
		GRECT r = wind->r, d;

#if PRESERVE_DIALOG_BGD
		{
			struct xa_window *scan_wind;

			/* Don't allow windows below a STORE_BACK to move */
			if (wind != TOP_WINDOW)
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
			xa_graf_mouse(XACRS_MOVER, NULL, NULL, false);
		}

		/* If right button is used, do a classic outline drag. */
		if (   md->state == MBS_RIGHT /*widg->s == 2*/
		    || wind->send_message == NULL
		    || wind->owner->options.nolive)
		{
			GRECT bound;

			bound.g_x = wind->owner->options.noleft ?
					0 : -root_window->r.g_w;
			if (cfg.menu_bar && cfg.menu_ontop == 0)
				bound.g_y = get_menu_height();
			else
				bound.g_y = root_window->wa.g_y;
			bound.g_w = root_window->r.g_w*3;
			bound.g_h = root_window->r.g_h*2;

			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				lock_screen(wind->owner->p, false);
			drag_box(wind->owner, r, &bound, rect_dist_xy(wind->owner, md->x, md->y, &r, &d), &r);
			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				unlock_screen(wind->owner->p);

			if (r.g_x != wind->rc.g_x || r.g_y != wind->rc.g_y)
			{
				r.g_h = wind->rc.g_h;

				if (wind->opts & XAWO_WCOWORK)
					r = f2w(&wind->delta, &r, true);
				send_moved(lock, wind, AMQ_NORM, &r);
			}
		}
		else if (widget_active.m.cstate)
		{
			short pmx, pmy/*, mx, my, mb*/;

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
				rect_dist_xy(wind->owner, pmx, pmy, &r, &d);
				widget_active.m.x = md->sx;
				widget_active.m.y = md->sy;
			}

			{
				/* Drag title */

				set_widget_active(wind, widg, drag_title, 1);

				widget_active.x = widget_active.m.x;
				widget_active.y = widget_active.m.y;

				widget_active.d.g_x = d.g_x;
				widget_active.d.g_y = d.g_y;

				if (widget_active.x != pmx || widget_active.y != pmy)
					r = move_rectangle(widget_active.x, widget_active.y, r, &d);

				if (wind->owner->options.noleft)
					if (r.g_x < 0)
						r.g_x = 0;

				if (r.g_y < root_window->wa.g_y)
					r.g_y = root_window->wa.g_y;

				if (r.g_x != wind->r.g_x || r.g_y != wind->r.g_y)
				{
					r.g_h = wind->rc.g_h;

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

/* This must run in the context of the window owner!! */
static void
shade_action(int lock, struct xa_window *wind)
{
	if (wind->active_widgets & (MOVER|NAME))
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

					/* if window was opened shaded fix rc.h */
					if( wind->rc.g_h == wind->r.g_h && wind->pr.g_h > wind->rc.g_h )
						wind->rc.g_h = wind->pr.g_h;
					move_window(lock, wind, true, ~(XAWS_SHADED|XAWS_ZWSHADED), wind->rc.g_x, wind->rc.g_y, wind->rc.g_w, wind->rc.g_h);
				}
			}
			else
			{
				if (wind->send_message)
				{
					DIAGS(("Click_title: shading window %d for %s",
						wind->handle, wind->owner->name));

					move_window(lock, wind, true, XAWS_SHADED, wind->rc.g_x, wind->rc.g_y, wind->rc.g_w, wind->rc.g_h);
					wind->send_message(lock, wind, NULL, AMQ_CRITICAL, QMF_CHKDUP,
						WM_SHADED, 0, 0, wind->handle, 0,0,0,0);
				}
			}
		}
	}
}
/*
 * Single click title bar sends window to the back
 */
static bool
click_title(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	bool nolist = wind->nolist;

	if (nolist && (wind->window_status & XAWS_NOFOCUS))
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
				if (nolist)
				{
					if (wind_has_focus(wind)) {
						send_bottomed(lock, wind);
					} else {
						send_topped(lock, wind);
					}
				}
				else
				{
					/* if not on top or on top and not focus */
					if (!is_topped(wind))
					{
						send_topped(lock, wind);
					}
					else if (!wind_has_focus(wind) && !(wind->window_status & XAWS_NOFOCUS))
						setnew_focus(wind, NULL, true, true, true);

					/* If window is already top, then send it to the back */
					/* Ozk: Always bottom iconified windows... */
					else if (is_iconified(wind))
					{
						send_bottomed(lock, wind);
					}
					else if ((wind->active_widgets & (STORE_BACK|BACKDROP)) == BACKDROP)
					{
						send_bottomed(lock, wind);
					}
				}
			}
		}
		else if (md->state == MBS_RIGHT && !(wind->dial & created_for_SLIST))
		{
			shade_action(lock, wind);
		}
		return true;
	}
	else if (md->clicks == 2)
	{
		if (wind->send_message)
		{
			/* If window is iconified - send request to restore it */

			if (is_iconified(wind))
			{
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
						   WM_UNICONIFY, 0, 0, wind->handle,
						   wind->pr.g_x, wind->pr.g_y, wind->pr.g_w, wind->pr.g_h);
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
static bool
click_wcontext(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	return false;
}
static bool
click_wappicn(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
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
click_close(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	bool ret = false;

	if (md->state & MBS_RIGHT)
	{
		post_cevent(C.Hlp, CE_winctxt, wind, NULL, 0,0, NULL,md);
		ret = true;
	}
	else
	{
		send_closed(lock, wind);
		if ((wind->window_status & XAWS_OPEN))
			ret = true;
#if 0
		if (wind->send_message)
		{
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					   WM_CLOSED, 0, 0, wind->handle,
					   0,0,0,0);

			if ((wind->window_status & XAWS_OPEN))
				ret = true;	/* redisplay */
		}
#endif
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
click_full(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
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
bool
iconify_action(int lock, struct xa_window *wind, const struct moose_data *md)
{
	if (!wind->send_message)
		return false;

	if ((wind->window_status & XAWS_OPEN))
	{
		short msg = -1;
		GRECT r;

		if (is_iconified(wind))
		{
			/* Window is already iconified - send request to restore it */
			r = wind->ro;

			msg = WM_UNICONIFY;
			wind->window_status |= XAWS_CHGICONIF;
		}
		else
		{
			/* Window is open - send request to iconify it */
			r = free_icon_pos(lock|LOCK_WINLIST, NULL);

			/* Could the whole screen be covered by iconified
			 * windows? That would be an achievement, wont it?
			 */
			if (r.g_y >= root_window->wa.g_y)
			{
				msg = (md && md->kstate & K_CTRL) ? WM_ALLICONIFY : WM_ICONIFY;
				wind->window_status |= XAWS_CHGICONIF;
			}
		}
		if (msg != -1)
		{
			if (wind->opts & XAWO_WCOWORK)
				r = f2w(msg == WM_UNICONIFY ? &wind->save_delta : &wind->delta, &r, true);

			wind->t = r;
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   msg, 0, 0, wind->handle,
				   r.g_x, r.g_y, r.g_w, r.g_h);
		}
	}
	/* Redisplay.... */
	return true;
}
/*
 * click the iconify widget
 */
static bool
click_iconify(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	DIAGS(("click_iconify:"));
	iconify_action(lock, wind, md);
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
click_hide(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	hide_app(lock, wind->owner);

	/* Redisplay.... (Unselect basically.) */
	return true;
}
static COMPASS
compass(short d, short x, short y, GRECT r)
{
	r.g_x += d;
	r.g_y += d;
	r.g_w -= d+d;
	r.g_h -= d+d;

	r.g_w += r.g_x - 1;
	r.g_h += r.g_y - 1;

	if (x < r.g_x)
	{
		if (y < r.g_y)	return NW;
		if (y > r.g_h)	return SW;
		return W_;
	}
	if (x > r.g_w)
	{
		if (y < r.g_y)	return NE;
		if (y > r.g_h)	return SE;
		return E_;
	}
	if (y < r.g_y)	return N_;
	return S_;
}

/* HR 150202: make rubber_box omnidirectional
 *            and use it here for outline sizing.
 */
static bool
size_window(int lock, struct xa_window *wind, XA_WIDGET *widg, bool sizer, WidgetBehaviour next, const struct moose_data *md)
{
	bool move, size;
	GRECT r = wind->r, d;

	if (   md->state == MBS_RIGHT /*widg->s == 2*/
	    || wind->send_message == NULL
	    || wind->owner->options.nolive)
	{
		int xy = sizer ? SE : compass(16, md->x, md->y, r);
		short maxw, maxh;

		if (wind->active_widgets & USE_MAX)
			maxw = wind->max.g_w, maxh = wind->max.g_h;
		else
			maxw = maxh = 0;

		/* Always have a nice consistent SIZER when resizing a window */
		xa_graf_mouse(border_mouse[xy], NULL, NULL, false);

		if (!(wind->owner->status & CS_NO_SCRNLOCK))
			lock_screen(wind->owner->p, false);
		rubber_box(wind->owner, xy,
		           r,
			   rect_dist_xy(wind->owner, md->x, md->y, &r, &d),
			   wind->min.g_w,
			   wind->min.g_h,
			   maxw,
			   maxh,
			   &r);
		if (!(wind->owner->status & CS_NO_SCRNLOCK))
			unlock_screen(wind->owner->p);

		size = r.g_w != wind->r.g_w || r.g_h != wind->r.g_h;
		if( !size )		/* size precedes */
			move = r.g_x != wind->r.g_x || r.g_y != wind->r.g_y;
		else
			move = false;

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
				{
					send_sized(lock, wind, AMQ_NORM, &r);
				}
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
				xa_graf_mouse(border_mouse[xy], NULL, NULL, false);
			}

			/* Has the mouse moved? */
			if (widget_active.m.x != pmx || widget_active.m.y != pmy)
			{
				short maxw, maxh;

				if (wind->active_widgets & USE_MAX)
					maxw = wind->max.g_w, maxh = wind->max.g_h;
				else
					maxw = maxh = 0;

				r = widen_rectangle(xy, widget_active.m.x, widget_active.m.y, r, &d);
				check_wh_cp(&r, xy,
					    wind->min.g_w,
					    wind->min.g_h,
					    maxw,
					    maxh);
			}

			size = r.g_w != wind->r.g_w || r.g_h != wind->r.g_h;
			if( !size && xy != N_ )		/* size precedes - move&size? */
				move = r.g_x != wind->r.g_x || r.g_y != wind->r.g_y;
			else
				move = false;


			if (move || size)
			{
				if (wind->opts & XAWO_WCOWORK)
					r = f2w(&wind->delta, &r, true);

				if (move && size && (wind->opts & XAWO_SENDREPOS))
					send_reposed(lock, wind, AMQ_NORM, &r);
				else
				{
					if (move)
					{
						send_moved(lock, wind, AMQ_NORM, &r);
					}
					if (size)
					{
						send_sized(lock, wind, AMQ_NORM, &r);
					}
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
drag_resize(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	return size_window(lock, wind, widg, true, drag_resize, md);
}
static inline bool
drag_border(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	return size_window(lock, wind, widg, false, drag_border, md);
}

#define MWCTXT_WINDOWS	1
#define MWCTXT_ADVANCED 2
#define MWCTXT_TODESK   3
#define MWCTXT_CLOSE	4
#define MWCTXT_HIDE	5
#define MWCTXT_ICONIFY	6
#define MWCTXT_SHADE	7
#define MWCTXT_MOVE	8
#define MWCTXT_SIZE	9
#define MWCTXT_QUIT	10
#define MWCTXT_KILL	11

#define ADVWC_FLOAT	1
#define ADVWC_SINK	2
#define ADVWC_TOOLBOX	3
#define ADVWC_NOFOCUS	4

#define WCACT_THIS	1
#define WCACT_ALL	2
#define WCACT_OTHERS	3
#define WCACT_RALL	4
#define WCACT_ROTHERS	5

#define WCHIDE_THIS	1
#define WCHIDE_APP	2
#define WCHIDE_OTHER	3
#define WCHIDE_UNHIDEOTH 4

#if 0
static bool
onopen_windows(XA_MENU_ATTACHMENT *at)
{
	return true;
}
#endif
static bool
onopen_advanced(XA_MENU_ATTACHMENT *at)
{
	struct xa_window *wind = at->data;

	if (wind)
	{
		OBJECT *t = at->wt->tree + at->menu;

		setchecked(t + ADVWC_FLOAT,	(wind->window_status & XAWS_FLOAT));
		setchecked(t + ADVWC_SINK,	(wind->window_status & XAWS_SINK));
		setchecked(t + ADVWC_TOOLBOX,	(wind->window_status & XAWS_BINDFOCUS));
		setchecked(t + ADVWC_NOFOCUS,	(wind->window_status & XAWS_NOFOCUS));
	}
	return true;
}
static bool
onopen_close(XA_MENU_ATTACHMENT *at)
{
	struct xa_window *wind = at->data;

	if (wind)
	{
		OBJECT *t = at->wt->tree + at->menu;
		setchecked(t + WCACT_THIS,	false);
		setchecked(t + WCACT_ALL,	false);
		setchecked(t + WCACT_OTHERS,	false);
		setchecked(t + WCACT_RALL,	false);
		disable_object(t + WCACT_RALL,	true);
		setchecked(t + WCACT_ROTHERS,	false);
		disable_object(t + WCACT_ROTHERS, true);
	}
	return true;
}
static bool
onopen_hide(XA_MENU_ATTACHMENT *at)
{
	struct xa_window *wind = at->data;

	if (wind)
	{
		OBJECT *t = at->wt->tree + at->menu;
		setchecked(t + WCHIDE_THIS, false);
		setchecked(t + WCHIDE_APP, false);
		setchecked(t + WCHIDE_OTHER, false);
		setchecked(t + WCHIDE_UNHIDEOTH, false);
	}
	return true;
}

static bool
onopen_iconify(XA_MENU_ATTACHMENT *at)
{
	struct xa_window *wind = at->data;

	if (wind)
	{
		OBJECT *t = at->wt->tree + at->menu;
		setchecked(t + WCACT_THIS, false);
		setchecked(t + WCACT_ALL,  false);
		setchecked(t + WCACT_OTHERS, false);
		setchecked(t + WCACT_RALL, false);
		disable_object(t + WCACT_RALL, false);
		setchecked(t + WCACT_ROTHERS, false);
		disable_object(t + WCACT_ROTHERS, false);
	}
	return true;
}
static bool
onopen_shade(XA_MENU_ATTACHMENT *at)
{
	struct xa_window *wind = at->data;

	if (wind)
	{
		OBJECT *t = at->wt->tree + at->menu;
		setchecked(t + WCACT_THIS, false);
		setchecked(t + WCACT_ALL,  false);
		setchecked(t + WCACT_OTHERS, false);
		setchecked(t + WCACT_RALL, false);
		disable_object(t + WCACT_RALL, false);
		setchecked(t + WCACT_ROTHERS, false);
		disable_object(t + WCACT_ROTHERS, false);
	}
	return true;
}

struct parm { char **start; short num; };

static void *
next_wctxt_entry(short item, void **_data)
{
	struct parm *p = (struct parm *)_data;
	char *ret;

	if (!item)
		p->num = 0;
	ret = p->start[p->num++];

	return ret;
}

static void
CE_shade_action(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		struct xa_window *wind = ce->ptr1;
		if (wind->window_status & XAWS_OPEN)
			shade_action(0, wind);
	}
}

static struct xa_window *Ctxtwin = NULL;

static bool
cwinctxt(struct c_event *ce, long arg)
{
	void **parms = (void **)arg;
	struct xa_window *wind = parms[0];
	struct xa_client *client = parms[1];
	struct xa_window *ctxtwin = ce->ptr1;

	if ((wind && wind == ctxtwin) || (client && ctxtwin && ctxtwin->owner == client)) {
		return true;
	} else {
		return false;
	}
}

void
cancel_winctxt_popup(int lock, struct xa_window *wind, struct xa_client *client)
{
	void *parms[2];

	if (!C.Hlp)
		return;

	parms[0] = wind;
	parms[1] = client;

	cancel_CE(C.Hlp, CE_winctxt, cwinctxt, (long)&parms);

	if (Ctxtwin && ((Ctxtwin == wind) || (client && client == Ctxtwin->owner)))
	{
		Ctxtwin = NULL;
		popout(TAB_LIST_START);
		C.Hlp->usr_evnt = 2;
	}
}

#define WCTXT_POPUPS 4
struct winctxt
{
	struct widget_tree *wt[WCTXT_POPUPS];
	short entries[WCTXT_POPUPS];
	XAMENU xmn[WCTXT_POPUPS];
	char **text[WCTXT_POPUPS];
};

struct windlist_pu
{
	short	num;
	short	an_len;
	short	wn_len;
	XAMENU	xmn;
	char **titles;
	struct xa_window **winds;
};


static struct windlist_pu *
build_windlist_pu(struct xa_client *client, struct moose_data *md)
{
	int an_len = 0, wn_len = 0, max_wn_len = 256, winds = 0, i, j;
	long msize = 0L;
	struct windlist_pu *pu = NULL;
	struct xa_window *wind;

	if( screen.r.g_w < 800 )
	{
		if( md->x > screen.r.g_w / 4 && md->x < screen.r.g_w * 3 / 4 )
			max_wn_len = 12;
		else
			max_wn_len = 32;
	}
	wind = window_list;

	while (wind && wind != root_window)
	{
		if (wind->window_status & XAWS_OPEN)
		{
			char *t;
			long len;

			t = wind->owner->name;
			while (*t == ' ') t++;
			if( *t )
			{
				len = strlen(t) + 1;
				if (len > an_len)
					an_len = len;

				t = wind->wname;
				len = strlen(t) + 1;
				if (len > wn_len)
					wn_len = len;
				if( wn_len > max_wn_len )
					wn_len = max_wn_len;

				winds++;
			}
		}
		wind = wind->next;
	}

	if (winds)
	{
		char *s, *d/*, *d1*/;
		size_t sz;

		msize = (long)(an_len + wn_len + 4) * winds;

		sz = sizeof(*pu) + msize + (sizeof(char *) * winds) + (sizeof(struct xa_window *) * winds);
		pu = kmalloc(sz);

		if (pu)
		{
			i = 0;

			pu->num = winds;
			pu->an_len = an_len;
			pu->wn_len = wn_len;
			pu->titles = (char **)((long)pu + sizeof(*pu));
			pu->winds = (struct xa_window **)((long)pu->titles + (sizeof(char *) * winds));

			d = (char *)((long)pu->winds + (sizeof(struct xa_window *) * winds));

			wind = window_list;
			while (wind && wind != root_window)
			{
				if (wind->window_status & XAWS_OPEN)
				{
					s = wind->owner->name;
					while (*s == ' ') s++;
					if( *s )
					{
						pu->titles[i] = d;
						pu->winds[i] = wind;
						for (j = 0; j < an_len; j++)
						{
							if (*s)
							{
								*d++ = *s++;
							}
							else *d++ = ' ';
						}
						*d++ = ':', *d++ = ' ';

						s = wind->wname;
						for (j = 0; j < wn_len; j++)
						{
							if (*s) *d++ = *s++;
							else *d++ = ' ';
						}

						*d++ = '\0';
						i++;
					}
				}
				wind = wind->next;
			}
		}

		if (pu)
		{
			OBJECT *obtree;
			struct widget_tree *wt;
			struct parm p;

			p.start = pu->titles;
			p.num = 0;
			obtree = create_popup_tree(client, 0, pu->num, 16,16, next_wctxt_entry, (void *)&p);
			if (obtree)
			{
				wt = new_widget_tree(client, obtree);
				if (wt)
				{
					wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
					pu->xmn.wt = wt;
					pu->xmn.menu.mn_tree = wt->tree;
					pu->xmn.menu.mn_menu = ROOT;
					pu->xmn.menu.mn_item = 1;
					pu->xmn.menu.mn_scroll = 1;
					pu->xmn.mn_selected = -1;
				}
				else
				{
					free_object_tree(client, obtree);
					obtree = NULL;
				}
			}

			if (!obtree)
			{
				kfree(pu);
				pu = NULL;
			}
		}
	}
	return pu;
}

void
CE_winctxt(int lock, struct c_event *ce, short cancel)
{
	struct winctxt *wct;
	OBJECT *obtree;
	int i, n_entries;
	short x, y;
	struct xa_window *wind = ce->ptr1;
	struct windlist_pu *wlpu = NULL;
	struct parm p;
	const short *txt;
	XAMENU_RESULT result;
	short obnum;
	short dowhat;

	static short const wctxt_main_txt[] = {
		WCTXT_WINDOWS, WCTXT_ADVANCED, WCTXT_TODESK, WCTXT_CLOSE, WCTXT_HIDE, WCTXT_ICONIFY, WCTXT_SHADE, WCTXT_MOVE, WCTXT_RESIZE, WCTXT_QUIT, WCTXT_KILL, -1,
		WCTXT_KEEPABOVE, WCTXT_KEEPBELOW, WCTXT_TOOLBOX, WCTXT_DENY_KEYBOARD, -1,
		WCTXT_CLOSE_THIS, WCTXT_CLOSE_ALL, WCTXT_CLOSE_OTHERS, WCTXT_RESTORE_ALL, WCTXT_RESTORE_OTHERS, -1,
		WCTXT_HIDE_THIS, WCTXT_HIDE_APP, WCTXT_HIDE_OTHERS, WCTXT_HIDE_SHOW_OTHERS, -1
	};

	if (cancel)
		return;
	if (!wind || wind == root_window)
		return;

	wct = kmalloc(sizeof(*wct));
	if (!wct) return;
	bzero(wct, sizeof(*wct));

	txt = wctxt_main_txt;

	for (i = 0; i < WCTXT_POPUPS; i++)
	{
		struct widget_tree *wt;

		n_entries = 0;
		p.start = wct->text[i] = &xa_strings[*txt];
		while (txt[n_entries] >= 0)
			n_entries++;

		txt += n_entries + 1;
		p.num = wct->entries[i] = n_entries;

		obtree = create_popup_tree(ce->client, 0, n_entries, 16,16, next_wctxt_entry, (void *)&p);

		if (obtree == NULL)
			goto bailout;
		wt = new_widget_tree(ce->client, obtree);

		if (wt == NULL)
		{
			free_object_tree(ce->client, obtree);
			goto bailout;
		}
		wct->wt[i] = wt;

		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		wct->xmn[i].wt = wt;
		wct->xmn[i].menu.mn_tree = wt->tree;
		wct->xmn[i].menu.mn_menu = ROOT;
		wct->xmn[i].menu.mn_item = 1;
		wct->xmn[i].menu.mn_scroll = 1;
		wct->xmn[i].mn_selected = -1;

		if (i == 0)
		{
			obtree->ob_x = ce->md.x;
			obtree->ob_y = ce->md.y;
			obj_offset(wt, aesobj(obtree, 0), &x, &y);
		}
	}

	/* attach advanced menu */
	attach_menu(0, ce->client, wct->wt[0], 2, &wct->xmn[1], onopen_advanced, wind);
	/* attach close menu */
	attach_menu(0, ce->client, wct->wt[0], 4, &wct->xmn[2], onopen_close, wind);
	/* attach hide menu */
	attach_menu(0, ce->client, wct->wt[0], 5, &wct->xmn[3], onopen_hide, wind);
	/* attach iconify menu */
	attach_menu(0, ce->client, wct->wt[0], 6, &wct->xmn[2], onopen_iconify, wind);
	/* attach shade menu */
	attach_menu(0, ce->client, wct->wt[0], 7, &wct->xmn[2], onopen_shade, wind);

	obnum = -1;
	dowhat = -1;

	wlpu = build_windlist_pu(ce->client, &ce->md);
	if (wlpu)
	{
		attach_menu(0, ce->client, wct->wt[0], 1, &wlpu->xmn, NULL, NULL);
	}
	Ctxtwin = wind;
	if (menu_popup(0, wct->wt[0]->owner, &wct->xmn[0], &result, x, y, 2))
	{
		if (result.menu.mn_item > 0)
		{
			if (result.at)
			{
				obnum = result.at->to_item;
				dowhat = result.menu.mn_item;
			}
			else
			{
				obnum = result.menu.mn_item;
			}
		}
	}

	if (TAB_LIST_START && TAB_LIST_START->client == ce->client)
	{
		popout(TAB_LIST_START);
	}

	if (obnum > 0)
	{
		switch (obnum)
		{
		case MWCTXT_WINDOWS:
			if (dowhat > 0)
			{
				struct xa_window *wl;
				if ((wl = wlpu->winds[dowhat - 1]))
				{
					wind = window_list;
					while (wind)
					{
						if (wind == wl)
							break;
						else if (wind == root_window)
						{
							wind = NULL;
							break;
						}
						wind = wind->next;
					}

					if (wind)
					{
#if 0
						S.focus = 0;
						if( wind != TOP_WINDOW )	/* would untop */
#endif
							top_window(0, true, true, wind);
					}
					else
						wlpu->winds[dowhat - 1] = NULL;
				}
			}
			break;

		case MWCTXT_ADVANCED:
			switch (dowhat)
			{
				case ADVWC_FLOAT: /* Keep over others */
					wind->window_status &= ~XAWS_SINK;
					wind->window_status ^= XAWS_FLOAT;
					top_window(0, true, true, wind);
					break;

				case ADVWC_SINK: /* Keep under others */
					wind->window_status &= ~XAWS_FLOAT;
					wind->window_status ^= XAWS_SINK;
					if (is_topped(wind))
						top_window(0, true, true, wind);
					else
						bottom_window(0, true, true, wind);
					break;

				case ADVWC_TOOLBOX:
					wind->window_status ^= XAWS_BINDFOCUS;
					if ((wind->window_status & XAWS_BINDFOCUS))
					{
						if (get_app_infront() != wind->owner)
							hide_toolboxwindows(wind->owner);
						else
							top_window(0, true, false, wind);
					}
					else
						top_window(0, true, true, wind);
					break;

				case ADVWC_NOFOCUS:
					{
						struct xa_window *fw;
						wind->window_status ^= XAWS_NOFOCUS;
						reset_focus(&fw, 0);
						setnew_focus(fw, NULL, true, true, true);
					}
					break;
			}
			break;

		case MWCTXT_TODESK:
			break;

		case MWCTXT_CLOSE:
			switch (dowhat)
			{
			case WCACT_THIS:
				send_closed(0, wind);
				break;

			case WCACT_ALL:
				{
					struct xa_client *cl = wind->owner;
					struct xa_window *nxt;

					wind = window_list;
					while (wind)
					{
						nxt = wind->next;
						if (wind->owner == cl && (wind->window_status & XAWS_OPEN))
							send_closed(0, wind);
						wind = nxt;
					}
				}
				break;
			case WCACT_OTHERS:
				{
					struct xa_client *cl = wind->owner;
					struct xa_window *nxt;

					wind = window_list;
					while (wind)
					{
						nxt = wind->next;
						if (wind->owner != cl && (wind->window_status & XAWS_OPEN))
							send_closed(0, wind);
						wind = nxt;
					}
				}
				break;
			}
			break;

		case MWCTXT_HIDE:
			switch (dowhat)
			{
			case -1:
			case WCHIDE_THIS:
				if ((wind->window_status & XAWS_HIDDEN))
					unhide_window(0, wind, false);
				else
					hide_window(0, wind);
				break;

			case WCHIDE_APP:
				hide_app(0, wind->owner);
				break;

			case WCHIDE_OTHER:
				hide_other(0, wind->owner);
				break;

			case WCHIDE_UNHIDEOTH:
				unhide_all(0, wind->owner);
				break;
			}
			break;

		case MWCTXT_ICONIFY:
			switch (dowhat)
			{
			case -1:
			case WCACT_THIS:
				iconify_action(0, wind, NULL);
				break;

			case WCACT_ALL:
			case WCACT_RALL:
				{
					struct xa_client *cl = wind->owner;
					struct xa_window *nxt;

					wind = window_list;
					while (wind && wind != root_window)
					{
						nxt = wind->next;
						if (dowhat == WCACT_ALL)
						{
							if (wind->owner == cl && !is_iconified(wind))
							{
								iconify_action(0, wind, NULL);
								wind->window_status |= XAWS_ICONIFIED|XAWS_SEMA;
							}
						}
						else
						{
							if (wind->owner == cl && is_iconified(wind))
								iconify_action(0, wind, NULL);
						}
						wind = nxt;
					}
					wind = window_list;
					while (wind && wind != root_window)
					{
						if (wind->owner == cl && (wind->window_status & XAWS_SEMA))
							wind->window_status &= ~(XAWS_ICONIFIED|XAWS_SEMA);
						wind = wind->next;
					}
				}
				break;

			case WCACT_OTHERS:
			case WCACT_ROTHERS:
				{
					struct xa_client *cl = wind->owner;
					struct xa_window *nxt;

					wind = window_list;
					while (wind && wind != root_window)
					{
						nxt = wind->next;
						if (dowhat == WCACT_OTHERS)
						{
							if (wind->owner != cl && !is_iconified(wind))
							{
								iconify_action(0, wind, NULL);
								wind->window_status |= XAWS_ICONIFIED|XAWS_SEMA;
							}
						}
						else
						{
							if (wind->owner != cl && is_iconified(wind))
								iconify_action(0, wind, NULL);
						}
						wind = nxt;
					}
					wind = window_list;
					while (wind && wind != root_window)
					{
						if (wind->owner != cl && (wind->window_status & XAWS_SEMA))
							wind->window_status &= ~(XAWS_ICONIFIED|XAWS_SEMA);
						wind = wind->next;
					}
				}
				break;
			}
			break;

		case MWCTXT_SHADE:
			switch (dowhat)
			{
			case WCACT_THIS: /* Shade window */
				post_cevent(wind->owner, CE_shade_action, wind,NULL, 0,0, NULL,&ce->md);
				break;

			case WCACT_ALL:
			case WCACT_RALL:
				{
					struct xa_client *owner = wind->owner;

					wind = window_list;
					while (wind && wind != root_window)
					{
						if (wind->owner == owner)
						{
							if ( (dowhat == WCACT_ALL && !(wind->window_status & XAWS_SHADED)) ||
							   ( (dowhat == WCACT_RALL && (wind->window_status & XAWS_SHADED))))
								post_cevent(owner, CE_shade_action, wind,NULL, 0,0, NULL, &ce->md);
						}
						wind = wind->next;
					}
				}
				break;

			case WCACT_OTHERS:
			case WCACT_ROTHERS:
				{
					struct xa_client *owner = wind->owner;

					wind = window_list;
					while (wind && wind != root_window)
					{
						if (wind->owner != owner)
						{
							if ( (dowhat == WCACT_OTHERS && !(wind->window_status & XAWS_SHADED)) ||
							   ( (dowhat == WCACT_ROTHERS && (wind->window_status & XAWS_SHADED))))
							{
								post_cevent(wind->owner, CE_shade_action, wind,NULL, 0,0, NULL, &ce->md);
							}
						}
						wind = wind->next;
					}
				}
				break;
			}
			break;

		case MWCTXT_MOVE:
			break;

		case MWCTXT_SIZE:
			break;

		case MWCTXT_QUIT:
			if (wind->owner->type & (APP_AESTHREAD|APP_AESSYS))
			{
				ALERT((xa_strings(AL_TERMAES)/*"Cannot terminate AES system proccesses!"*/));
			}
			else
				send_terminate(lock, wind->owner, AP_TERM);
			break;

		case MWCTXT_KILL:
			if (wind->owner->type & (APP_AESTHREAD|APP_AESSYS))
			{
				ALERT((xa_strings(AL_KILLAES)/*"Not a good idea, I tell you!"*/));
			}
			else
				ikill(wind->owner->p->pid, SIGKILL);
			break;
		}
	}
	Ctxtwin = NULL;

bailout:
	if (wct->wt[0])
	{
		for (i = 1; i < wct->entries[0] + 1; i++)
		{
			if (is_attach(ce->client, wct->wt[0], i, NULL))
				detach_menu(0, ce->client, wct->wt[0], i);
		}
	}

	for (i = 0; i < WCTXT_POPUPS; i++)
	{
		remove_wt(wct->wt[i], false);
	}
	kfree(wct);

	if (wlpu)
	{
		detach_menu(0, ce->client, wct->wt[0], 1);
		remove_wt(wlpu->xmn.wt, false);
		kfree(wlpu);
	}
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
set_widget_repeat(int lock, struct xa_window *wind)
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
click_scroll(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	XA_WIDGET *slider = &(wind->widgets[widg->slider_type]);
	bool reverse = md->state == MBS_RIGHT;
	short mx = md->x;
	short my = md->y;
	short mb = md->state;

	if (!(   widget_active.widg
	      && slider
	      && slider->stuff.sl
	      && slider->stuff.sl->position == (reverse ? widg->xlimit : widg->limit)))
	{
		XA_SLIDER_WIDGET *sl = slider->stuff.sl;
		int dir;
		long pos;
		long w, x, mp;

		if( md->cstate == 1 ) /* mouse-button held down: don't move slider past mouse-position */
		{
			dir = widg == &wind->widgets[XAW_UPPAGE] ||  widg == &wind->widgets[XAW_DNPAGE] ? 1 :
				widg == &wind->widgets[XAW_LFPAGE] || widg == &wind->widgets[XAW_RTPAGE] ? 2 : 0;
			if( dir )
			{
				if( dir == 1 )
				{
					w = slider->ar.g_h;
					x = slider->ar.g_y;
					mp = my;
				}
				else
				{
					w = slider->ar.g_w;
					x = slider->ar.g_x;
					mp = mx;
				}
				pos = w * ((long)sl->position) / 1000L + x;
				w = w * ((long)sl->length) / 1000L;
				dir = mp >= pos - w && mp <= pos + w;	/* mouse-pointer inside slider */
			}
		}
		else
			dir = 0;
		if ((md->kstate & (K_RSHIFT|K_LSHIFT)) || dir)
		{
			int inside;

			/* XXX
			 * Center sliders at the clicked position.
			 * Wait for mousebutton release
			 */
			if ((md->kstate & 3) && mb == md->cstate)
			{
				xa_graf_mouse(XACRS_POINTSLIDE, NULL, NULL, false);
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
				int offs;

				if (widg->slider_type == XAW_VSLIDE)
					offs = bound_sl(pix_to_sl(my - slider->ar.g_y - (sl->r.g_h >> 1), slider->r.g_h - sl->r.g_h));
				else
					offs = bound_sl(pix_to_sl(mx - slider->ar.g_x - (sl->r.g_w >> 1), slider->r.g_w - sl->r.g_w));

				if (offs != sl->position)
				{
					sl->rpos = offs;
					if (wind->send_message)
					{
						wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
								   widg->slider_type == XAW_VSLIDE ? WM_VSLID : WM_HSLID,
								   0, 0, wind->handle, offs, 0, 0, 0);
					}
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
drag_vslide(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;

	if (widget_active.m.cstate)
	{
		short offs;

		if (widget_active.m.cstate & MBS_RIGHT)
		{
			GRECT s, b, d, r;
			rp_2_ap(wind, widg, &widg->ar);

			b = s = widg->ar;

			s.g_x += sl->r.g_x;
			s.g_y += sl->r.g_y;
			s.g_w = sl->r.g_w;
			s.g_h = sl->r.g_h;

			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				lock_screen(wind->owner->p, false);
			xa_graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				unlock_screen(wind->owner->p);

			offs = bound_sl(pix_to_sl(r.g_y - widg->ar.g_y, widg->r.g_h - sl->r.g_h));

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
				xa_graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
				rp_2_ap(wind, widg, &widg->ar);
				widget_active.offs = md->y - (widg->ar.g_y + sl->r.g_y);
				widget_active.y = md->y;
				y = md->sy;
			}
			else
				y = md->y;

			if (widget_active.y != y)
			{
				offs = bound_sl(pix_to_sl((y - widget_active.offs) - widg->ar.g_y, widg->r.g_h - sl->r.g_h));

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
drag_hslide(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;

	if (widget_active.m.cstate)
	{
		short offs;

		if (widget_active.m.cstate & MBS_RIGHT)
		{
			GRECT s, b, d, r;

			rp_2_ap(wind, widg, &widg->ar);

			b = s = widg->ar;

			s.g_x += sl->r.g_x;
			s.g_y += sl->r.g_y;
			s.g_w = sl->r.g_w;
			s.g_h = sl->r.g_h;

			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				lock_screen(wind->owner->p, false);
			xa_graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				unlock_screen(wind->owner->p);

			offs = bound_sl(pix_to_sl(r.g_x - widg->ar.g_x, widg->r.g_w - sl->r.g_w));

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
				xa_graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
				rp_2_ap(wind, widg, &widg->ar);
				widget_active.offs = md->x - (widg->ar.g_x + sl->r.g_x);
				widget_active.x = md->x;
				x = md->sx;
			}
			else
				x = md->x;

			if (widget_active.x != x)
			{

				offs = bound_sl(pix_to_sl((x - widget_active.offs) - widg->ar.g_x, widg->r.g_w - sl->r.g_w));

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
display_object_widget(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	XA_TREE *wt = widg->stuff.wt;
	/* Convert relative coords and window location to absolute screen location */
#if GENERATE_DIAGS
	OBJECT *root = rp_2_ap(wind, widg, NULL);
	DIAG((D_form,wind->owner,"display_object_widget(wind=%d), wt=%lx, e.obj=%d, e.pos=%d, form: %d/%d",
		wind->handle, (unsigned long)wt, edit_item(&wt->e), wt->e.pos, root->ob_x, root->ob_y));
#endif

	if (wind->nolist && (wind->dial & created_for_POPUP))
	{
		/* clip work area */
		(*v->api->set_clip)(v, &wind->wa);
		draw_object_tree(0, wt, NULL, v, aesobj(wt->tree, widg->start), 100, NULL, 0);
		(*v->api->clear_clip)(v);
	}
	else {
		draw_object_tree(0, wt, NULL, v, aesobj(wt->tree, widg->start), 100, NULL, 0);
	}

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
	int thin = wind->thinwork;
	int frame = wind->frame;
	int t_margin, b_margin, l_margin, r_margin;
	short wa_borders = 0;
	bool shaded = wind->window_status & XAWS_SHADED;

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
		wind->wadelta.g_x += frame;
		wind->wadelta.g_y += frame;
		wind->wadelta.g_w += (frame << 1) + wind->x_shadow;
		wind->wadelta.g_h += (frame << 1) + wind->y_shadow;

		wind->outer.g_x += frame;
		wind->outer.g_y += frame;
		wind->outer.g_w -= (frame << 1) + wind->x_shadow;
		wind->outer.g_h -= (frame << 1) + wind->y_shadow;
	}
	if (frame < 0)
		frame = 0;

	wind->inner = f2w(&wind->bd, &wind->outer, true);

	wind->ba.g_x = wind->r.g_x + frame;
	wind->ba.g_y = wind->r.g_y + frame;
	frame <<= 1;
	wind->ba.g_w = wind->r.g_w - frame;
	wind->ba.g_h = wind->r.g_h - frame;

	rp_2_ap_row(wind);

	if (wind->inner.g_y == wind->outer.g_y && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.g_y += t_margin;
		wind->wadelta.g_h += t_margin;
		wa_borders |= WAB_TOP;
	}

	if (wind->inner.g_x == wind->outer.g_x && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.g_x += l_margin;
		wind->wadelta.g_w += l_margin;
		wa_borders |= WAB_LEFT;
	}

	if ((wind->inner.g_y + wind->inner.g_h) == (wind->outer.g_y + wind->outer.g_h) && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.g_h += b_margin;
		wa_borders |= WAB_BOTTOM;
	}

	if ((wind->inner.g_x + wind->inner.g_w) == (wind->outer.g_x + wind->outer.g_w) && wind->frame >= 0 && wind->thinwork && wind->wa_frame)
	{
		wind->wadelta.g_w += r_margin;
		wa_borders |= WAB_RIGHT;
	}

	if (wind->frame >= 0 && !wind->thinwork)
	{
		wind->wadelta.g_x += 2, wind->wadelta.g_y += 2;
		wind->wadelta.g_w += 4, wind->wadelta.g_h += 4;
	}

	wind->wa_borders = wa_borders;
	wind->delta = wind->wadelta;
	wind->wa = f2w(&wind->wadelta, &wind->rc, true);

	if ((wind->active_widgets & TOOLBAR) && !(get_widget(wind, XAW_TOOLBAR)->m.properties & WIP_NOTEXT) && wind->owner->p == get_curproc())
	{
		struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);

		rp_2_ap(wind, widg, &widg->ar);

		wind->delta.g_y += widg->ar.g_h;
		wind->delta.g_h += widg->ar.g_h;
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
			wind->outer.g_x += frame;
			wind->outer.g_y += frame;
			wind->outer.g_w -= (frame << 1) + wind->x_shadow;
			wind->outer.g_h -= (frame << 1) + wind->y_shadow;
		}
		wind->inner = f2w(&wind->rbd, &wind->outer, true);
	}

	/* Add bd to toolbar->r to get window rectangle for create_window
	 * Anyhow, always convenient, especially when snapping the workarea.
	 */
	DIAG((D_widg, wind->owner, "workarea %d: %d/%d,%d/%d", wind->handle, wind->wa.g_x, wind->wa.g_y, wind->wa.g_w, wind->wa.g_h));
}

/*
 * Ozk: Any resources taken by a widget should be released here
 */
void
free_xawidget_resources(struct xa_widget *widg)
{
	DIAGS(("free_xawidget_resources: widg=%lx", (unsigned long)widg));
	if (widg->stuff.wt)
	{
		switch (widg->stufftype)
		{
			case STUFF_IS_WT:
			{
				XA_TREE *wt = widg->stuff.wt;
				DIAGS(("  --- stuff is wt=%lx in widg=%lx",
					(unsigned long)wt, (unsigned long)widg));

				wt->links--;

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
			}
				break;
			default:
				if (widg->flags & XAWF_STUFFKMALLOC)
				{
					DIAGS(("  --- release stuff=%lx in widg=%lx",
						(unsigned long)widg->stuff.ptr, (unsigned long)widg));
					kfree(widg->stuff.ptr);
					widg->stuff.ptr = NULL;
				}
				break;
		}
		widg->flags &= ~XAWF_STUFFKMALLOC;
	} else
	{
		DIAGS(("  --- stuff=%lx not alloced in widg=%lx", (unsigned long)widg->stuff.ptr, (unsigned long)widg));
	}
	widg->stufftype = 0;
	widg->stuff.ptr = NULL;
 	widg->m.r.draw = NULL;
	widg->m.click = NULL;
	widg->m.drag = NULL;
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
/*             defaults              index        mask       status                          rsc          properties  destruct */
/*                                                           mask                                                                        */
stdl_close   = {LT,                  XAW_CLOSE,   CLOSER,    XAWS_ICONIFIED,                 WIDG_CLOSER, WIP_ACTIVE, free_xawidget_resources },
stdl_full    = {RT,                  XAW_FULL,    FULLER,    XAWS_ICONIFIED,                 WIDG_FULL,   WIP_ACTIVE, free_xawidget_resources },
stdl_iconify = {RT,                  XAW_ICONIFY, ICONIFIER, 0,                              WIDG_ICONIFY,WIP_ACTIVE, free_xawidget_resources },
stdl_hide    = {RT,                  XAW_HIDE,    HIDER,     XAWS_ICONIFIED,                 WIDG_HIDE,   WIP_ACTIVE, free_xawidget_resources },
stdl_title   = {LT | R_VARIABLE,     XAW_TITLE,   NAME,      0,                              0,           WIP_ACTIVE, free_xawidget_resources },
stdl_notitle = {LT | R_VARIABLE,     XAW_TITLE,   NAME,      0,                              0,                    0, free_xawidget_resources },
#if 0
stdl_resize  = {RB,                  XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,     WIDG_SIZE,   WIP_ACTIVE, free_xawidget_resources },
#endif
stdl_resize  = {RB,                  XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,     WIDG_SIZE,   WIP_ACTIVE, free_xawidget_resources },
stdl_nresize = {RT,                  XAW_RESIZE,  SIZER,     XAWS_SHADED|XAWS_ICONIFIED,     WIDG_SIZE,            0, free_xawidget_resources },
stdl_uscroll = {LT,                  XAW_UPLN,    UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,     WIDG_UP,     WIP_ACTIVE, free_xawidget_resources },
stdl_upage   = {NO,                  XAW_UPPAGE,  UPARROW,   XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_vslide  = {LT | R_VARIABLE,     XAW_VSLIDE,  VSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_nvslide = {LT | R_VARIABLE,     XAW_VSLIDE,  VSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,     0,                    0, free_xawidget_resources },
stdl_dpage   = {NO,                  XAW_DNPAGE,  DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_dscroll = {RT,                  XAW_DNLN,    DNARROW,   XAWS_SHADED|XAWS_ICONIFIED,     WIDG_DOWN,   WIP_ACTIVE, free_xawidget_resources },
stdl_lscroll = {LB,                  XAW_LFLN,    LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,     WIDG_LEFT,   WIP_ACTIVE, free_xawidget_resources },
stdl_lpage   = {NO,                  XAW_LFPAGE,  LFARROW,   XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_hslide  = {LB | R_VARIABLE,     XAW_HSLIDE,  HSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
#if 0
stdl_nhslide = {LB | R_VARIABLE,     XAW_HSLIDE,  HSLIDE,    XAWS_SHADED|XAWS_ICONIFIED,     0,                    0, free_xawidget_resources },
#endif
stdl_rpage   = {NO,                  XAW_RTPAGE,  RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_rscroll = {RB,                  XAW_RTLN,    RTARROW,   XAWS_SHADED|XAWS_ICONIFIED,     WIDG_RIGHT,  WIP_ACTIVE, free_xawidget_resources },
stdl_info    = {LT | R_VARIABLE,     XAW_INFO,    INFO,      XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_menu    = {LT | R_VARIABLE,     XAW_MENU,    XaMENU,    XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_pop     = {LT,                  XAW_MENU,    XaPOP,     XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources },
stdl_border  = {0,                   XAW_BORDER,  0,         XAWS_SHADED|XAWS_ICONIFIED,     0,           WIP_ACTIVE, free_xawidget_resources }
;

static XA_WIDGET_LOCATION *stdl[] =
{
	&stdl_close,
	&stdl_full,
	&stdl_iconify,
	&stdl_hide,
	&stdl_title,
	&stdl_notitle,
#if 0
	&stdl_resize
#endif
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
#if 0
	&stdl_nhslid
#endif
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
				widg->r.g_y = row->rxy + widg->r.g_h + rxy;
				rxy += widg->r.g_h;
			}
			else
			{
				widg->r.g_y = row->r.g_y + lxy;
				lxy += widg->r.g_h;
			}
			widg->r.g_x = row->r.g_x;
			widg->r.g_w = row->r.g_w;
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
				widg->r.g_x = row->rxy + widg->r.g_w + rxy;
				rxy += widg->r.g_w;
			}
			else
			{
				widg->r.g_x = row->r.g_x + lxy;
				lxy += widg->r.g_w;
			}
			widg->r.g_y = row->r.g_y;
			widg->r.g_h = row->r.g_h;
			widg = widg->next_in_row;
		}
	}
}

static void
rp_2_ap_row(struct xa_window *wind)
{
	struct xa_widget_row *row = wind->widg_rows;
	struct xa_widget *widg;
	GRECT r;
	short v, x2, y2, l;

	r = wind->r;

	if (wind->frame >= 0)
	{
		short frame = wind->frame;

		r.g_x += frame;
		r.g_y += frame;
		frame <<= 1;
		r.g_w -= (frame + wind->x_shadow);
		r.g_h -= (frame + wind->y_shadow);
	}

	x2 = r.g_x + r.g_w;
	y2 = r.g_y + r.g_h;

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
							widg->ar.g_x = r.g_x + widg->r.g_x;
							widg->ar.g_y = r.g_y + widg->r.g_y;
							break;
						}
						case R_RIGHT:
						{
							widg->ar.g_x = x2 - widg->r.g_x;
							widg->ar.g_y = r.g_y + widg->r.g_y;
							if (l > widg->ar.g_x)
								l = widg->ar.g_x;
							break;
						}
						case R_BOTTOM:
						{
							widg->ar.g_x = r.g_x + widg->r.g_x;
							widg->ar.g_y = y2 - widg->r.g_y;
							break;
						}
						case (R_BOTTOM|R_RIGHT):
						{
							widg->ar.g_x = x2 - widg->r.g_x;
							widg->ar.g_y = y2 - widg->r.g_y;
							if (l > widg->ar.g_x)
								l = widg->ar.g_x;
							break;
						}
						default:;
					}
					widg->ar.g_w = widg->r.g_w;
					widg->ar.g_h = widg->r.g_h;
					widg = widg->next_in_row;
				}
				if (v)
				{
					widg = row->start;
					while (widg)
					{
						if (widg->m.r.pos_in_row & R_VARIABLE)
						{
							widg->ar.g_w = widg->r.g_w = (l - widg->ar.g_x);
						}
						if (widg->m.r.xaw_idx == XAW_MENU)
						{
							XA_TREE *wt = widg->stuff.wt;
							if (wt && wt->tree)
							{
								wt->tree->ob_x = widg->ar.g_x;
								wt->tree->ob_y = widg->ar.g_y;
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
					if (widg->m.r.pos_in_row & R_VARIABLE)
						v++;

					switch ((widg->m.r.pos_in_row & (R_RIGHT|R_BOTTOM|R_CENTER)))
					{
						case 0:
						{
							widg->ar.g_x = r.g_x + widg->r.g_x;
							widg->ar.g_y = r.g_y + widg->r.g_y;
							break;
						}
						case R_RIGHT:
						{
							widg->ar.g_x = x2 - widg->r.g_x;
							widg->ar.g_y = r.g_y + widg->r.g_y;
							break;
						}
						case R_BOTTOM:
						{
							widg->ar.g_x = r.g_x + widg->r.g_x;
							widg->ar.g_y = y2 - widg->r.g_y;
							if (l > widg->ar.g_y)
								l = widg->ar.g_y;
							break;
						}
						case (R_BOTTOM|R_RIGHT):
						{
							widg->ar.g_x = x2 - widg->r.g_x;
							widg->ar.g_y = y2 - widg->r.g_y;
							if (l > widg->ar.g_y)
								l = widg->ar.g_y;
							break;
						}
						default:;
					}
					widg->ar.g_w = widg->r.g_w;
					widg->ar.g_h = widg->r.g_h;
					widg = widg->next_in_row;
				}
				if (v)
				{
					widg = row->start;
					while (widg)
					{
						if (widg->m.r.pos_in_row & R_VARIABLE)
						{
							widg->ar.g_h = widg->r.g_h = (l - widg->ar.g_y);
						}
						if (widg->m.r.xaw_idx == XAW_MENU)
						{
							XA_TREE *wt = widg->stuff.wt;
							if (wt && wt->tree)
							{
								wt->tree->ob_x = widg->ar.g_x;
								wt->tree->ob_y = widg->ar.g_y;
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
position_widget(struct xa_window *wind, struct xa_widget *widg, GRECT *offsets)
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

		/*
		 * See if this widget is already in this row
		 */
		while (nxt_w && nxt_w->next_in_row && nxt_w != widg && !(nxt_w->m.r.tp & widg->m.r.tp))
			nxt_w = nxt_w->next_in_row;

		if (nxt_w && (nxt_w != widg))
		{
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
			/*
			 * This is the first widget in this row, and thus we also init its Y
			 */
			rows->start = widg;
			widg->next_in_row = NULL;
			rows->r.g_y = rows->r.g_x = rows->r.g_w = rows->r.g_h = rows->rxy = 0;

			if (!orient)	/* horizontal */
			{
				switch (placement)
				{
					case XAR_START:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.g_y = nxt_r->r.g_y + nxt_r->r.g_h;
						else
						{
							rows->r.g_y = offsets->g_y;
#if 0
							wind->bd.g_y -= offsets->g_y;
							wind->bd.g_h -= offsets->g_h;
#endif
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_START))))
							rows->r.g_x = nxt_r->r.g_x + nxt_r->r.g_w;
						else
							rows->r.g_x = offsets->g_x;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_END))))
							rows->rxy = nxt_r->r.g_x;
						else
							rows->rxy = offsets->g_w;

						wind->ext_borders |= T_BORDER;
						break;
					}
					case XAR_END:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_END))))
							rows->r.g_y = nxt_r->r.g_y;
						else
						{
							rows->r.g_y = offsets->g_h;
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_START))))
							rows->r.g_x = nxt_r->r.g_x + nxt_r->r.g_w;
						else
							rows->r.g_x = offsets->g_w;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT|XAR_END))))
							rows->rxy = nxt_r->r.g_x;
						else
							rows->rxy = offsets->g_w;
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
							rows->r.g_x = nxt_r->r.g_x + nxt_r->r.g_w;
						else
						{
							rows->r.g_x = offsets->g_x;
#if 0
							wind->bd.g_x -= offsets->g_x;
							wind->bd.g_w -= offsets->g_w;
#endif
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.g_y = nxt_r->r.g_y + nxt_r->r.g_h;
						else
							rows->r.g_y = offsets->g_y;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), XAR_END)))
							rows->rxy = nxt_r->r.g_y;
						else
							rows->rxy = offsets->g_h;
						wind->ext_borders |= L_BORDER;
						break;
					}
					case XAR_END:
					{
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_VERT | XAR_END))))
							rows->r.g_x = nxt_r->r.g_x;
						else
						{
							rows->r.g_x = offsets->g_w;
						}
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), (XAR_START))))
							rows->r.g_y = nxt_r->r.g_y + nxt_r->r.g_h;
						else
							rows->r.g_y = offsets->g_y;
						if ((nxt_r = get_prev_row(rows, (XAR_VERT|XAR_PM), XAR_END)))
							rows->rxy = nxt_r->r.g_y;
						else
							rows->rxy = offsets->g_h;
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
			widg->r.g_w = screen.c_max_w;
			widg->r.g_h = screen.c_max_h;
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

			if ((diff = widg->r.g_h - rows->r.g_h) > 0)
			{
				rows->r.g_h = widg->r.g_h;

				switch (placement)
				{
					case XAR_START:
					{
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT|XAR_PM), XAR_START)))
						{
							nxt_r->r.g_y += diff;
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, XAR_VERT)))
						{
							nxt_r->r.g_y += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.g_y += diff;
						wind->bd.g_h += diff;

						break;
					}
					case XAR_END:
					{
						rows->r.g_y += diff;
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), XAR_END)))
						{
							nxt_r->r.g_y += diff;
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, XAR_VERT)))
						{
							nxt_r->rxy += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.g_h += diff;
						break;
					}
				}
			}
			else
				widg->r.g_h = rows->r.g_h;

		}
		else
		{
			XA_RELATIVE rt = m->r.pos_in_row & R_VARIABLE;

			if (m->r.pos_in_row & R_RIGHT)
				rt |= R_BOTTOM;
			if (placement == XAR_END)
				rt |= R_RIGHT;

			m->r.pos_in_row = rt;

			if ((diff = widg->r.g_w - rows->r.g_w) > 0)
			{
				rows->r.g_w = widg->r.g_w;
				switch (placement)
				{
					case XAR_START:
					{
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, XAR_VERT, 0)))
						{
							nxt_r->r.g_x += diff;
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_START))))
						{
							nxt_r->r.g_x += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.g_x += diff;
						wind->bd.g_w += diff;
						break;
					}
					case XAR_END:
					{
						rows->r.g_x += diff;
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT | XAR_PM), (XAR_VERT | XAR_END))))
						{
							nxt_r->r.g_x += diff;
							reloc_widgets_in_row(nxt_r);
						}
						nxt_r = rows;
						while ((nxt_r = get_next_row(nxt_r, (XAR_VERT), 0)))
						{
							nxt_r->rxy += diff;
							reloc_widgets_in_row(nxt_r);
						}
						wind->bd.g_w += diff;
						break;
					}
				}
			}
			else
				widg->r.g_w = rows->r.g_w;
		}
		reloc_widgets_in_row(rows);
	}
}
static XA_WIDGET *
make_widget(struct xa_window *wind,
	    struct xa_widget_methods *m,
	    struct xa_client *owner, GRECT *offsets)
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
		stuff = widg->stuff.ptr;
		st = widg->stufftype;
	}
	else if (widg->m.destruct)
		(*widg->m.destruct)(widg);

	bzero(widg, sizeof(XA_WIDGET));

	widg->m.r.xaw_idx = n;

	if (stuff)
	{
		widg->stuff.ptr = stuff;
		widg->stufftype = st;
	}
}

static struct xa_widget_row *
create_widg_layout(struct xa_window *wind)
{
	XA_WIND_ATTR tp;
	struct nwidget_row *rows;
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
			xa_rows->r = (GRECT){0, 0, 0, 0};
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

static struct xa_widget_methods def_methods[] =
{
/*Properties, statusmask           render_widget click		drag		release	wheel	key		destruct	*/
 {0,0,					{0},	NULL,		NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* Dummy */
 {0,0,					{0},	NULL,		drag_border,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* Border */
 {0,0,					{0},	click_title,	drag_title,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* title */
 {0,XAWS_ICONIFIED,			{0},	click_wcontext,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources },/* wcontext */
 {0,XAWS_ICONIFIED,			{0},	click_wappicn,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources },/* wappicn */
 {0,XAWS_ICONIFIED,			{0},	click_close,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* closer */
 {0,XAWS_ICONIFIED,			{0},	click_full,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* fuller */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_title,	NULL,		NULL,	NULL,	NULL,	free_xawidget_resources	},/* info */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	NULL,		drag_resize,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* resize */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* uparrow */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* uparrow */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* dnarrow */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	NULL,		drag_vslide,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* vslide */
 {0,XAWS_ICONIFIED | XAWS_SHADED,	{0},	click_scroll,	click_scroll,	NULL,	NULL,	NULL,	free_xawidget_resources	},/* lfarrow */
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
init_slider_widget(struct xa_window *wind, struct xa_widget *widg, short slider_idx, GRECT *offsets)
{
	struct xa_widget *w;
	XA_SLIDER_WIDGET *sl;

	/* Ozk: Bad mem leak here. If widget got slider info, we keep this
	 */
	if (widg->stuff.sl == NULL)
	{
		sl = kmalloc(sizeof(*sl));

		assert(sl);
		widg->stuff.sl = sl;
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
			rows->r = (GRECT){0, 0, 0, 0};
			rows = rows->next;
		}
	}
	{
		struct nwidget_row *rows = theme->layout;
		XA_WIND_ATTR rtp, this_tp, *tp_deps;
		struct xa_widget *widg;
		short xaw_idx;
		struct xa_widget_methods dm;
		GRECT *offsets;
		GRECT r_ofs;

		wind->ext_borders = 0;
		if (wind->frame >= 0 && (wind->draw_canvas = theme->draw_canvas))
		{
			offsets = &theme->outer;
			wind->bd.g_x = offsets->g_x + theme->inner.g_x;
			wind->bd.g_y = offsets->g_y + theme->inner.g_y;
			wind->bd.g_w = offsets->g_x + offsets->g_w + theme->inner.g_x + theme->inner.g_w;
			wind->bd.g_h = offsets->g_y + offsets->g_h + theme->inner.g_y + theme->inner.g_h;
			wind->rbd = wind->bd;
		}
		else
		{
			wind->bd = wind->rbd = r_ofs = (GRECT){0,0,0,0};
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
						case XAW_UPLN1:
						case XAW_DNLN:
						{
							widg->slider_type = XAW_VSLIDE;
							if (xaw_idx == XAW_UPLN || xaw_idx == XAW_UPLN1)
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
						case XAW_LFLN1:
						case XAW_RTLN:
						{
							widg->slider_type = XAW_HSLIDE;
							if (xaw_idx == XAW_LFLN || xaw_idx == XAW_LFLN1)
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
display_toolbar(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	XA_TREE *wt = widg->stuff.wt;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, NULL);

	DIAG((D_form,wind->owner,"display_object_widget(wind=%d), wt=%lx, e.obj=%d, e.pos=%d",
		wind->handle, (unsigned long)wt, edit_item(&wt->e), wt->e.pos));

	draw_object_tree(0, wt, NULL, wind->vdi_settings, aesobj(wt->tree, widg->start), 100, NULL, 0);

	return true;
}

/*
 * HR: Direct display of the toolbar widget; HR 260102: over the rectangle list.
 */
void _cdecl
redraw_toolbar(int lock, struct xa_window *wind, short item)
{
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_rect_list *rl;
	GRECT r;

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
set_toolbar_coords(struct xa_window *wind, const GRECT *r)
{
	struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);

	widg->r.g_x = wind->wa.g_x - wind->r.g_x - (wind->frame <= 0 ? 0 : wind->frame);
	widg->r.g_y = wind->wa.g_y - wind->r.g_y - (wind->frame <= 0 ? 0 : wind->frame);
	if (r)
	{
		widg->r.g_w = r->g_w;
		widg->r.g_h = r->g_h;
	}
	else
	{
		XA_TREE *wt = widg->stuff.wt;

		if( wt && (wind->dial & created_for_FMD_START))
		{
			/* use wt-coords for dialog */
			widg->r.g_w = wt->tree->ob_width;
			widg->r.g_h = wt->tree->ob_height;
		}
		else
		{
			widg->r.g_w = wind->wa.g_w;
			widg->r.g_h = wind->wa.g_h;
		}
	}
}

void
set_toolbar_handlers(const struct toolbar_handlers *th, struct xa_window *wind, struct xa_widget *widg, struct widget_tree *wt)
{
	if (widg)
	{
		if (th && th->click)
		{
			if (th->click == (void *)-1L)
				widg->m.click = NULL;
			else
				widg->m.click = th->click;
		}
		else
			widg->m.click = Click_windowed_form_do;

		if (th && th->drag)
		{
			if (th->drag == (void *)-1L)
				widg->m.drag	= NULL;
			else
				widg->m.drag	= th->drag;
		}
		else{
			widg->m.drag	= Click_windowed_form_do;
		}

		if (th && th->draw)
		{
			if (th->draw == (void *)-1L)
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
			if (th->destruct == (void *)-1L)
				widg->m.destruct = NULL;
			else
				widg->m.destruct = th->destruct;
		}
		else
			widg->m.destruct	= free_xawidget_resources;

		if (th && th->release)
		{
			if (th->release == (void *)-1L)
				widg->m.release = NULL;
			else
				widg->m.release	= th->release;
		}
		else
			widg->m.release	= NULL;
	}

	if (wind)
	{
		/* hek:
		 * seems some alerts have no default but only Ok
		 * they can only be handled by mouse then
		 * now all have a keyboard (Key_form_do)
		 */
		if (wt)
		{
			if (th && th->keypress) {
				if (th->keypress == (void *)-1L)
					wind->keypress = NULL;
				else
					wind->keypress = th->keypress;
			} else
				wind->keypress = Key_form_do;
		} else
			wind->keypress = NULL;
	}

	if (wt)
	{
		if (th && th->exitform)
		{
			if (th->exitform == (void *)-1L)
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
set_toolbar_widget(int lock,
		struct xa_window *wind,
		struct xa_client *owner,
		OBJECT *obtree,
		struct xa_aes_object edobj,
		short properties,
		short flags,
		const struct toolbar_handlers *th,
		const GRECT *r)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	XA_TREE *wt;
	XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
	GRECT or,wr;

	DIAG((D_wind, wind->owner, "set_toolbar_widget for %d (%s): obtree %lx, %d",
		wind->handle, wind->owner->name, (unsigned long)obtree, edobj.item));

	if (widg->stuff.wt)
	{
		struct widget_tree *owt = widg->stuff.wt;

		set_toolbar_handlers(NULL, NULL, NULL, owt);
		owt->widg = NULL;
		owt->wind = NULL;
		owt->zen  = false;
		owt->links--;
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
	wt->zen  = flags & STW_ZEN ? true : false;
	wt->links++;

	if ((flags & (STW_COC|STW_GOC)))
	{
		obj_rectangle(wt, aesobj(wt->tree, 0), &or);
		wr = or;
		r = &wr;
	}
	else
	{
		/*
		 * 'or' is not used if flags=0 but form_do does not work if compiled with gcc4
		 * and the assignment is made. widg-wh then comes from or!
		 * This shows up when a windowed classic dialog (form_do without block)
		 * is opend the second time.
		 *
		 * *0 seems to be no problem?
		 */
		if( r )
			or = *r;
	}

	if (flags & (STW_COC))
		center_rect(&or);

	/*
	 * Ozk: if edobj == -2, we want to look for an editable and place the
	 * cursor there. Used by wdlg_create() atm
	 */
	if (edobj.item == -2)
	{
		edobj =  ob_find_any_flst(obtree, OF_EDITABLE, 0, 0, OS_DISABLED/*, 0, 0*/);
	}

	if (!obj_edit(wt, v, ED_INIT, edobj, 0, -1, NULL, false, NULL, NULL, NULL, &edobj))
		obj_edit(wt, v, ED_INIT, edobj, 0, -1, NULL, false, NULL, NULL, NULL, NULL);

	widg->m.properties = properties | WIP_WACLIP | WIP_ACTIVE;
	set_toolbar_handlers(th, wind, widg, wt);

	/* HR 280801: clicks are now put in the widget struct.
	      NB! use this property only when there is very little difference between the 2 */

	widg->state	= OS_NORMAL;
	widg->stuff.wt = wt;
	widg->stufftype	= STUFF_IS_WT;
	widg->start	= 0;
	wind->tool	= widg;

	widg->m.statusmask = XAWS_SHADED;
	widg->m.r.pos_in_row = LT;
	widg->m.r.tp = TOOLBAR;
	widg->m.r.xaw_idx = XAW_TOOLBAR;
	wind->active_widgets |= TOOLBAR;

	if ((flags & STW_SWC))
	{
		or = w2f(&wind->wadelta, &or, false);
		move_window(lock, wind, true, -1, or.g_x, or.g_y, or.g_w, or.g_h);
	}
	set_toolbar_coords(wind, r);

	return wt;
}

void
remove_widget(int lock, struct xa_window *wind, int tool)
{
	XA_WIDGET *widg = get_widget(wind, tool);

	DIAG((D_form, NULL, "remove_widget %d: 0x%lx", tool, (unsigned long)widg->stuff.wt));

	if (widg->m.r.freepriv)
		(*widg->m.r.freepriv)(wind, widg);

	if (widg->m.destruct)
		(*widg->m.destruct)(widg);
	else
	{
		widg->stufftype	= 0;
		widg->stuff.wt = NULL;
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
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;
	short x  = widg->ar.g_x + sl->r.g_x;
	short x2 = x + sl->r.g_w;

	if (click > x && click < x2)
		return 0;

	/* outside slider, arrows must be defined */
	if (w->active_widgets & (LFARROW|LFARROW1))
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
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;
	short y  = widg->ar.g_y + sl->r.g_y;
	short y2 = y + sl->r.g_h;

	if (click > y && click < y2)
		return 0;

 	if (w->active_widgets & (UPARROW|UPARROW1))
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

/*
 * return widget-number+1 if found else 0
 */
short
checkif_do_widgets(int lock, struct xa_window *w, short x, short y, XA_WIDGET **ret)
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
				GRECT r;

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
						return f + 1;
					}
				}
			}
		}
	}
	if (ret)
		*ret = NULL;

	return 0;
}


/* HR 161101: possibility to mask out certain widgets. */
int
do_widgets(int lock, struct xa_window *w, XA_WIND_ATTR mask, const struct moose_data *md)
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
			GRECT r;

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
					bool rtn = false;
					int ax = 0;

					widg->x = md->x - r.g_x; 		/* Mark where the click occurred (relative to the widget) */
					widg->y = md->y - r.g_y;

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
						widg->x = md->x - r.g_x; 			/* Mark where the click occurred (relative to the widget) */
						widg->y = md->y - r.g_y;
						if (widg->m.drag) rtn = widg->m.drag(lock, w, widg, md);	/* we know there is only 1 behaviour for these arrows */
						else		  rtn = true;
					}
					else /* normal widget */
					{
						short b = md->cstate, rx = md->x, ry = md->y;
						XA_TREE *wt = widg->stuff.wt;

						/* We don't auto select & re-display for a menu, info or toolbar widget */
						if (f != XAW_MENU && f != XAW_TOOLBAR && f != XAW_INFO)
						{
							redisplay_widget(lock, w, widg, OS_SELECTED);
						}

						/*
						 * Check if the widget has a dragger function if button still pressed
						*/
						if (b && f == XAW_TOOLBAR && (w->dial & created_for_TOOLBAR) && wt->exit_form )
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
							if (widg->m.click){
								rtn = widg->m.click(lock, w, widg, md);
							}
						}
						else if (b && (widg->m.properties & WIP_NODRAG))
						{
							if (widg->m.click){
								widg->m.click(lock, w, widg, md);
							}
						}
						else if (b && widg->m.drag){
							rtn = widg->m.drag(lock, w, widg, md);
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
						if (f != XAW_MENU && f != XAW_TOOLBAR && f != XAW_INFO)
						{
							redisplay_widget(lock, w, widg, OS_NORMAL); /* Flag the widget as de-selected */
						}
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

static short
redisplay_widget(int lock, struct xa_window *wind, XA_WIDGET *widg, short state)
{
	short ret = widg->state;

	if (wdg_is_inst(widg))
	{
		widg->state = state;
		/* No rectangle list. */
		if ((wind->dial & created_for_SLIST) || (wind->nolist && !wind->rect_list.start))
		{
			if (wind->parent && (wind->dial & created_for_SLIST))
			{
				draw_widget(wind, widg, wind->parent->rect_list.start);
			}
			else
			{
				struct xa_vdi_settings *v = wind->vdi_settings;

				(*v->api->set_clip)(v, &wind->r);
				hidem();
				/* Not on top, but visible */
				widg->m.r.draw(wind, widg, &wind->r);
				showm();
				(*v->api->clear_clip)(v);
			}
		}
		else
		{
			/* Display the selected widget */
			display_widget(lock, wind, widg, NULL);
		}
	}
	return ret;
}

void
do_active_widget(int lock, struct xa_client *client)
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
	struct xa_window *wind;

	if (x == -1 && y == -1)
		check_mouse(NULL, NULL, &x, &y);

	wind = find_window(0, x, y, FNDW_NOLIST|FNDW_NORMAL);
	wind_mshape(wind, x, y);
}

short
wind_mshape(struct xa_window *wind, short x, short y)
{
	short shape = -1;
	struct xa_client *wo;
	WINDOW_STATUS status = wind->window_status;
	XA_WIDGET *widg;
	GRECT r;

	if( !wind )
	{
		return -1;
	}
	if( wind == root_window)
		wo = get_desktop()->owner;
	else
		wo = wind->owner;
	/* C.shutdown, ferr ? */
	if ( !update_locked() || update_locked() == wind->owner->p )
	{
		if (wo)
		{
			if (!(wo->status & CS_EXITING))
			{
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
						xa_graf_mouse(shape, NULL, NULL, true);
				}
				else
				{
					if (C.aesmouse != -1)
						xa_graf_mouse(-1, NULL, NULL, true);
					if (C.mouse_form != wind->owner->mouse_form)
						xa_graf_mouse(wo->mouse, wo->mouse_form, wo, false);
				}
				if (!update_locked() && (cfg.widg_auto_highlight
#if WITH_BBL_HELP
					|| cfg.describe_widgets
#endif
				) )
				{
#if WITH_BBL_HELP
					int bbl_closed = 0;
#endif
					struct xa_window *rwind = NULL;
					struct xa_widget *hwidg, *rwidg = NULL;

					short f = checkif_do_widgets(0, wind, x, y, &hwidg);

					if( f == (XAW_TOOLBAR+1) )
					{
						struct widget_tree *wt = hwidg->stuff.wt;

						if( wt->extra && (wt->flags & WTF_EXTRA_ISLIST) )
						{
							SCROLL_INFO *list = wt->extra;
							while( list )
							{
								if( m_inside(x, y, &list->wi->r) )
								{
									wind = list->wi;
									f = checkif_do_widgets(0, wind, x, y, &hwidg);
									if( f == (XAW_TITLE + 1) )
										hwidg = 0;	/* don't select list-window-title */
									break;
								}
								list = list->next;
							}
						}
					}
					if( (hwidg && !hwidg->owner)
#if WITH_BBL_HELP
						&& wind != bgem_window	/* ? root_window,menu_window ? */
#endif
						)
					{
						if (wind != C.hover_wind || hwidg != C.hover_widg)
						{
							rwind = C.hover_wind;
							rwidg = C.hover_widg;
							if( cfg.widg_auto_highlight )
								redisplay_widget(0, wind, hwidg, OS_SELECTED);
#if WITH_BBL_HELP

							if (cfg.describe_widgets)
							{
								if (rwind)
								{
									xa_bubble( 0, bbl_close_bubble2, 0, 0 );
									bbl_closed = 1;
								}
								bubble_show(xa_strings(f - 1 + BBL_RESIZE));
							}
#endif
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
						if( cfg.widg_auto_highlight )
							redisplay_widget(0, rwind, rwidg, OS_NORMAL);
#if WITH_BBL_HELP
						if( cfg.describe_widgets && !bbl_closed )
						{
							xa_bubble( 0, bbl_close_bubble2, 0, 0 );
						}
#endif
					}
				}
			}
			else
			{
				C.hover_wind = NULL;
				C.hover_widg = NULL;
				xa_graf_mouse(ARROW, NULL, NULL, false);
			}
		}
		else if (C.aesmouse != -1)
		{
			xa_graf_mouse(-1, NULL, NULL, true);
		}
	}

	return shape;
}

