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
 
/*
 * This file contains the client side of mouse event processing
 */

#include "c_mouse.h"
#include "xa_global.h"
#include "xa_evnt.h"
#include "app_man.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "desktop.h"
#include "init.h"
#include "k_init.h"
#include "k_main.h"
#include "k_mouse.h"
#include "k_shutdown.h"
#include "nkcc.h"
#include "menuwidg.h"
#include "scrlobjc.h"
#include "semaphores.h"
#include "taskman.h"
#include "widgets.h"
#include "obtree.h"

#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/signal.h"


void
cXA_button_event(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_client *client = ce->client;
	struct xa_window *wind = (struct xa_window *)ce->ptr1;
	struct moose_data *md = &ce->md;

	if (cancel)
	{
		if (C.ce_menu_click == client)
			C.ce_menu_click = NULL;
		
		return;
	}

	DIAG((D_button, client, "cXA_button_event for %s - %d/%d, state=%d, clicks=%d - ptr1=%lx, ptr2=%lx, %lx/%lx",
		client->name, md->x, md->y, md->state, md->clicks,
		ce->ptr1, ce->ptr2, wind, md));

	if (!wind)
	{
		Tab *root_tab;

		if ((root_tab = TAB_LIST_START) && md->state && root_tab->client == client)
		{
			Tab *tab;
			
			tab = find_pop(md->x, md->y);
				
			if (tab && !tab->task_data.menu.entry)
				tab = collapse(root_tab, tab);
			else if (!tab)
				tab = root_tab;

			
			DIAG((D_button, client, "cXA_button_event: Menu click"));
			if (tab->ty)
			{
				MENU_TASK *k = &tab->task_data.menu;
				wind = k->popw;

				/* HR 161101: widgets in scrolling popups */
				if (wind)
				{
					if (wind->owner != client)
					{
						DIAG((D_button, client, "cXA_button_event: Wrong client %s, should be %s", wind->owner->name, client->name));
						goto endmenu;
					}
					if (   (wind->dial & created_for_POPUP) != 0
					    && (wind->active_widgets & V_WIDG) != 0
					   )
					{
						if (do_widgets(lock, wind, XaMENU, md))
							goto endmenu;
					}
				}

				if (k->entry)
				{
					if (tab->client != client)
					{
						DIAG((D_button, client, "cXA_button_event: Wrong owner of Tab %s, should be %s", tab->client->name, client->name));
						goto endmenu;
					}
					k->clicks = md->clicks;
					k->x = md->x;
					k->y = md->y;
					k->entry(tab);
					goto endmenu;
				}
			}
		}
endmenu:	C.ce_menu_click = NULL;
		return;
	}
	
	if ( is_topped(wind) || wind == root_window || (!is_topped(wind) && wind->active_widgets & NO_TOPPED) )
	{
		DIAG((D_button, client, "cXA_button_event: Topped win"));
		if (do_widgets(lock, wind, 0, md))
			return;
		button_event(lock, client, md);
		return;
	}

	if (!is_topped(wind))
	{
		DIAG((D_button, client, "cXA_button_event: wind not on top"));
		if (do_widgets(lock, wind, 0, md))
			return;
	}

	if (md->state)
	{
		DIAG((D_button, client, "cXA_button_event: send click"));
		if (wind->send_message)
		{
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP, WM_TOPPED, 0, 0, wind->handle, 0, 0, 0, 0);
		}
		return;
	}
}

void
cXA_deliver_button_event(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_window *wind;
	struct xa_widget *widg = NULL;
	
	if (!cancel)
	{
		wind = ce->ptr1;
		if (wind)
			widg = get_widget(wind, XAW_ICONIFY);

		DIAG((D_button, ce->client, "cXA_deliver_button_event: to %s (wind=%lx, widg=%lx)",
			ce->client->name, wind, widg));

		/*
		 * Double click on a iconified window will uniconify
		 */
		if (wind && (wind->window_status & XAWS_ICONIFIED) && widg && widg->click)
		{
			if (ce->md.clicks > 1)
				widg->click(lock, wind, widg, &ce->md);
			else if (wind->send_message && !is_topped(wind))
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP, WM_TOPPED, 0, 0, wind->handle, 0, 0, 0, 0);
		}
		else
		{
			button_event(lock, ce->client, &ce->md);
		}
	}
}

void
cXA_deliver_rect_event(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_client *client = ce->client;
	AESPB *pb = client->waiting_pb;
	int events = ce->d0;

	if (!cancel)
	{
		if (pb)
		{
			short *out = pb->intout;
			struct mbs mbs;

			get_mbstate(client, &mbs);

			if (client->waiting_for & XAWAIT_MULTI)
			{
				*out++ = events;
				*out++ = mbs.x;
				*out++ = mbs.y;
				*out++ = mbs.b;
				*out++ = mbs.ks;
				*out++ = 0;
				*out   = 0;
			}
			else
			{
				*out++ = 1;
				*out++ = mbs.x;
				*out++ = mbs.y;
				*out++ = mbs.b;
				*out   = mbs.ks;
			}
		}
		client->usr_evnt = 1;
	}
}

void
cXA_form_do(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_client *client = ce->client;

	if (!cancel)
	{
		DIAG((D_mouse, client, "cXA_form_do for %s", client->name));
		client->fmd.mousepress(lock, client, NULL, NULL, &ce->md);
	}
}

void
cXA_open_menu(enum locks lock, struct c_event *ce, bool cancel)
{
	XA_WIDGET *widg = ce->ptr1;
	XA_TREE *menu = ce->ptr2;

	if (!cancel)
	{
		DIAG((D_mouse, ce->client, "cXA_open_menu for %s", ce->client->name));
		if ( menu == get_menu() )
			widg->click(lock, root_window, widg, &ce->md);
#if GENERATE_DIAGS
		else
			DIAG((D_mouse, ce->client, "cXA_open_menu skipped for %s - menu changed before cevent.", ce->client->name));
#endif
	}
	C.ce_open_menu = NULL;
}

void
cXA_menu_move(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		if (TAB_LIST_START->client == ce->client && !C.move_block)
		{
			Tab *tab = TAB_LIST_START;
			MENU_TASK *k;
			short x = ce->md.x;
			short y = ce->md.y;

			DIAG((D_mouse, ce->client, "cXA_menu_move for %s", ce->client->name));
		
			/*
			 * Ozk: Cannot use FOREACH_TAB() here, since there may be additions to the top (start)
			 *      of the list during our wander down towards the bottom of it.
			 */
			while(tab)
			{
				k = &tab->task_data.menu;

				if (k->em.flags & MU_MX)
				{
					/* XaAES internal flag: report any mouse movement. */

					k->em.flags &= ~MU_MX; //0;
					k->x = x;
					k->y = y;
					tab = k->em.t1(tab);	/* call the function */
					break;
				}
				if ((k->em.flags & MU_M1))
				{
					if (is_rect(x, y, k->em.m1_flag & 1, &k->em.m1))
					{
						k->em.flags &= ~MU_M1; //0;
						k->x = x;
						k->y = y;
						tab = k->em.t1(tab);	/* call the function */
						break;
					}
					if (m_inside(x, y, &k->bar))
						break;
				}
				if ((k->em.flags & MU_M2))
				{
					if (is_rect(x, y, k->em.m2_flag & 1, &k->em.m2))
					{
						k->em.flags &= ~MU_M2;
						k->x = x;
						k->y = y;
						tab = k->em.t2(tab);
						break;
					}
					if (m_inside(x, y, &k->drop))
						break;
				}
				tab = tab->tab_entry.next;
			}
			
			if (tab)
				tab = tab->tab_entry.next;
			while (tab)
			{
				k = &tab->task_data.menu;
				if (k->outof)
					k->outof(tab);
				tab = tab->tab_entry.next;
			}
		}
	}
	C.ce_menu_move = NULL;
}

void
cXA_do_widgets(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		DIAG((D_mouse, ce->client, "cXA_do_widgets for %s", ce->client->name));
		do_widgets(lock, (struct xa_window *)ce->ptr1, 0, &ce->md);
	}
}
 
void
cXA_active_widget(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		DIAG((D_mouse, ce->client, "cXA_active_widget for %s", ce->client->name));
		do_active_widget(lock, ce->client);

		/* If active widget action did not generate any WM_MOVED or other WM_REDRAW generating
		 * actions, move_block is still 1 in which case we can release the move_block here
		 */
	}
	if (C.move_block == 1)
		C.move_block = 0;
}

void
cXA_widget_click(enum locks lock, struct c_event *ce, bool cancel)
{
	XA_WIDGET *widg = ce->ptr1;

	if (!cancel)
	{
		DIAG((D_mouse, ce->client, "cXA_widget_click for %s", ce->client->name));
		widg->click(lock, root_window, widg, &ce->md);
	}
}


static void
wheel_arrow(struct xa_window *wind, const struct moose_data *md, XA_WIDGET **wr, short *r, short *r_amnt)
{
	XA_WIDGETS which;
	XA_WIDGET  *widg;
	short orient;
	int fac = wind->owner->options.wheel_page;
	bool rev = wind->owner->options.wheel_reverse ? true : false;

	if (wr)
		*wr = NULL;
	if (r)
		*r = -1;

	if (md->state == cfg.ver_wheel_id)
	{
		if (r_amnt)
			*r_amnt = cfg.ver_wheel_amount;

		if (md->clicks < 0)
		{
			which = rev ? XAW_DNLN : XAW_UPLN;
			orient = rev ? 1 : 0;
		}
		else
		{
			which = rev ? XAW_UPLN : XAW_DNLN;
			orient = rev ? 0 : 1;
		}
	}
	else if (md->state == cfg.hor_wheel_id)
	{
		if (r_amnt)
			*r_amnt = cfg.hor_wheel_amount;

		if (md->clicks < 0)
		{
			which = rev ? XAW_RTLN : XAW_LFLN;
			orient = rev ? 3 : 2;
		}
		else
		{
			which = rev ? XAW_LFLN : XAW_RTLN;
			orient = rev ? 2 : 3;
		}
	}
	else
		return;

	if (r)
		*r = orient;

	if (wind)
	{
		if (fac && abs(md->clicks) > abs(fac))
		{
			switch (which)
			{
				case XAW_UPLN: which = XAW_UPPAGE; break;
				case XAW_DNLN: which = XAW_DNPAGE; break;
				case XAW_LFLN: which = XAW_LFPAGE; break;
				case XAW_RTLN: which = XAW_RTPAGE; break;
				default: /* make gcc happy */ break;
			}
		}

		widg = get_widget(wind, which);
		if (widg)
		{
			if (widg->type && wr)
				*wr = widg;
		}
	}
}
/* If md pointer is NULL, send a standard WM_ARROWED (application dont support wheel)
 * else send extended WM_ARROWED message
 */
static void
whlarrowed(struct xa_window *wind, short WA, short amount, const struct moose_data *md)
{
	if (md)
	{
		wind->send_message(0, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				   WM_ARROWED, 0,0, wind->handle,
				   (amount << 8)|(WA & 7), md->x, md->y, md->kstate);
	}
	else
	{
		while (amount)
		{
			wind->send_message(0, wind, NULL, AMQ_NORM, 0,
					   WM_ARROWED, 0,0, wind->handle,
					   WA, 0, 0, 0);
			amount--;
		}
	}
}

void
cXA_wheel_event(enum locks lock, struct c_event *ce, bool cancel)
{
	struct xa_client *client = ce->client;
	struct xa_window *wind = ce->ptr1;
	struct moose_data *md = &ce->md;
	XA_WIDGET *widg;
	
	if (!cancel)
	{
		if (client->waiting_for & MU_WHEEL)
		{
			struct moose_data *nmd = client->wheel_md;
			short amount = 0;

			if (!nmd)
				nmd = kmalloc(sizeof(*nmd));
			else
				amount = nmd->clicks;
			if (nmd)
			{
				*nmd = *md;
				nmd->clicks += amount;
				client->wheel_md = nmd;
			}
		}
		else if (wind)
		{
			bool slist = false;
			short orient, amount, WA = WA_UPPAGE;
			
			wheel_arrow(wind, md, &widg, &orient, &amount);
			
			if ((md->kstate & K_ALT))
				orient ^= 2;

			if (orient & 2)
				WA = WA_LFPAGE;
			
			if (!(md->kstate & (K_RSHIFT|K_LSHIFT)))
				WA += WA_UPLINE;
			else
				amount = 1;	/* No more than one when paging */

			WA += orient & 1;

			if (is_topped(wind))
			{
				XA_TREE *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
				OBJECT *obtree;
				short obj;
				
				if (wt && (obtree = wt->tree))
				{
					//display("got toolbar - wt %lx", wt);
					//display("got object tree %lx", obtree);
					obj = obj_find(wt, 0, 10, md->x, md->y, NULL);
					//display("found obj %d, type = %d", obj, obtree[obj].ob_type);
					if (obj > 0 && (obtree[obj].ob_type & 0xff) == G_SLIST)
					{
						struct scroll_info *list = object_get_slist(obtree + obj);
						
						//display("object is slist");
						amount *= (md->clicks < 0 ? -md->clicks : md->clicks);
						whlarrowed(list->wi, WA, amount, NULL/*md*/);
						slist = true;
					}
				}
			}
			 
			if (!slist && wind->opts & XAWO_WHEEL)
			{
				switch (wind->wheel_mode)
				{
					case WHL_REALWHEEL:
					{
						wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
								   WM_WHEEL, 0,0, wind->handle,
								   md->x, md->y,
								   md->kstate,
								   ((md->state && 0xf) << 12)|((orient & 0xf) << 8)|(md->clicks & 0xff));
						break;
					}
					case WHL_AROWWHEEL:
					{
						amount *= (md->clicks < 0 ? -md->clicks : md->clicks);
						whlarrowed(wind, WA, amount, md);
						break;
					}
					case WHL_SLDRWHEEL:
					{
						short w = -1, s = 0;

						if (orient & 2)
						{
							if (wind->active_widgets & HSLIDE)
								w = XAW_HSLIDE, s = WM_HSLID;
						}
						else if (wind->active_widgets & VSLIDE)
							w = XAW_VSLIDE, s = WM_VSLID;

						if (w == -1)
							whlarrowed(wind, WA, 1, NULL);

						widg = get_widget(wind, w);

						if (widg)
						{
							XA_SLIDER_WIDGET *sl = widg->stuff;
							short unit, wh;

							if (s == WM_VSLID)
								wh = widg->r.h - sl->r.h;
							else
								wh = widg->r.w - sl->r.w;
					
							unit = pix_to_sl(2L, wh) - pix_to_sl(1L, wh);

							if (md->clicks < 0)
								sl->rpos = bound_sl(sl->rpos - (unit * -md->clicks));
							else
								sl->rpos = bound_sl(sl->rpos + (unit * md->clicks));
					
							{
								wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
										   s, 0,0, wind->handle,
										   sl->rpos, 0,0,0);
							}
						}
						break;
					}
				}
			}
			else if (!slist)
			{
				amount *= (md->clicks < 0 ? -md->clicks : md->clicks);
				whlarrowed(wind, WA, amount, NULL);
			}
		}
	}
}
