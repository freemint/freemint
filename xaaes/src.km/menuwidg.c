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

#include "xaaes.h"
#include "menuwidg.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "k_main.h"
#include "k_mouse.h"
#include "menuwidg.h"
#include "obtree.h"
#include "draw_obj.h"
#include "rectlist.h"
#include "taskman.h"
#include "widgets.h"
#include "messages.h"
#include "xa_rsrc.h"
#include "form.h"
#include "keycodes.h"


static void cancel_pop_timeouts(void);

static TASK menuclick;
static TASK outofpop;
static TASK popup;
static TASK click_desk_popup;
static TASK click_menu_entry;
static TASK menu_bar;


static XAMENU desk_popup;
static XA_TREE desk_wt;
static bool menu_title(int lock, Tab *tab, short item, struct xa_window *wind, XA_WIDGET *widg, int locker, const struct moose_data *md);
static XA_TREE *set_popup_widget(Tab *tab, struct xa_window *wind, int item);


static inline bool
barred(Tab *tab)
{
	return tab->ty == MENU_BAR || tab->ty == ROOT_MENU;
}

static inline unsigned long
menu_colors(void)
{
	return
	     (objc_rend.dial_colours.fg_col << 12)		/* border */
	   | (objc_rend.dial_colours.fg_col << 8)		/* text */
	   |  objc_rend.dial_colours.bg_col		/* inside */
	   | (7 << 4)					/* solid fill */
	   | (1 << 7);					/* transparent text */
}

static void
menu_spec(OBJECT *obtree, int item)
{
	if (object_have_spec(obtree + item))
	{
		object_set_spec(obtree + item, 0xff0000L | menu_colors());
		obtree[item].ob_state |= OS_SHADOWED;
	}
}

static void
change_title(Tab *tab, int state)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = k->m.wt; //k->wt;
	OBJECT *obtree = wt->tree;
	int t = k->m.current;

	if (state)
		state = obtree[t].ob_state | OS_SELECTED;
	else
		state = obtree[t].ob_state & ~OS_SELECTED;

	obtree->ob_x = k->rdx;
	obtree->ob_y = k->rdy;
	wt->rend_flags |= WTR_ROOTMENU;
	obj_change(wt,
		   C.Aes->vdi_settings,
		   aesobj(wt->tree, t), 2,
		   state,
		   obtree[t].ob_flags,
		   true,
		   NULL,
		   tab->wind ? tab->wind->rect_list.start : NULL, 0);
	wt->rend_flags &= ~WTR_ROOTMENU;
}

static void
change_entry(Tab *tab, int state)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = k->p.wt; //k->wt;
	OBJECT *obtree = wt?wt->tree:0;
	short t = k->p.current;

	DIAGS(("change entry: tab=%lx, obtree=%lx, obj=%d, state=%d",
		(unsigned long)tab, (unsigned long)obtree, k->p.current, state));

	if (t < 0 || !obtree) /* || t > ?? */
		return;

	if (state)
		state = obtree[t].ob_state | OS_SELECTED;
	else
		state = obtree[t].ob_state & ~OS_SELECTED;

	obtree->ob_x = k->pdx;
	obtree->ob_y = k->pdy;

	if (k->p.wind)
	{
		obj_change(wt,
			   k->p.wind->vdi_settings,
			   aesobj(wt->tree, t), 1 | 0x8000,
			   state,
			   obtree[t].ob_flags,
			   true,
			   &k->p.wind->wa,
			   k->p.wind->rect_list.start, 0);
	}
	else
	{
		DIAGS(("change_entry: no popw"));
		obj_change(wt, C.Aes->vdi_settings, aesobj(wt->tree, t), 1 | 0x8000, state, obtree[t].ob_flags, true, NULL, NULL, 0);
	}
}

static void
redraw_entry(Tab *tab, short t)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = k->p.wt;
	OBJECT *obtree = wt->tree;

	DIAGS(("change entry: tab=%lx, obtree=%lx, obj=%d",
		(unsigned long)tab, (unsigned long)obtree, k->p.current));

	if (t < 0)
		return;

	obtree->ob_x = k->pdx;
	obtree->ob_y = k->pdy;

	if (k->p.wind)
	{
		obj_draw(wt,
			   k->p.wind->vdi_settings,
			   aesobj(wt->tree, t), 1 | 0x8000,
			   &k->p.wind->wa,
			   k->p.wind->rect_list.start, 0);
	}
	else
	{
		DIAGS(("change_entry: no popw"));
		obj_draw(wt, C.Aes->vdi_settings, aesobj(wt->tree, t), 1 | 0x8000, NULL, NULL, 0);
	}
}
void
wt_menu_area(XA_TREE *wt)
{
	OBJECT *obtree;
	short titles;

	if ((obtree = wt->tree))
	{
		/* shift menubar (todo) */
		//short shift = screen.c_max_w / 2 + 2;
		titles = obtree[obtree[SYSTEM_MENU].ob_head].ob_head;
		//if( wt->owner != C.Aes )
			//aesobj(wt->tree, titles).ob->ob_x -= shift;
		/* additional fix to fit in window */
		obj_area(wt, aesobj(wt->tree, titles), &wt->area);

		DIAG((D_menu, wt->owner, "wt_menu_area: %d/%d/%d/%d for %s",
			wt->area.g_x, wt->area.g_y, wt->area.g_w, wt->area.g_h, wt->owner->name));
	}
}

void
set_rootmenu_area(struct xa_client *client)
{
	DIAG((D_menu, client, "set_rootmenu_area: for %s to %d/%d/%d/%d",
		client->name, client->std_menu->area.g_x, client->std_menu->area.g_y, client->std_menu->area.g_w, client->std_menu->area.g_h));

	if (client->std_menu)
	{
		C.Aes->em.m1 = client->std_menu->area;
		C.Aes->em.flags |= MU_M1;
	}
}

bool
is_attach(struct xa_client *client, XA_TREE *wt, int item, XA_MENU_ATTACHMENT **pat)
{
	XA_MENU_ATTACHMENT *at = client->attach;
#if GENERATE_DIAGS
	OBJECT *attach_to = wt->tree + item;
#endif

	DIAGS(("is_attach: for %s, wt=%lx, obj=%d, obtree=%lx",
		client->name, (unsigned long)wt, item, (unsigned long)wt->tree));

	DIAG((D_menu, client, "is_attach: at=%lx,flags=%x,type=%x, spec=%lx",
	       (unsigned long)at, attach_to->ob_flags, attach_to->ob_type, object_get_spec(attach_to)->index));

	while (at)
	{
		if (at->to == wt && at->to_item == item)
			break;
		at = at->next;
	}

	if (pat)
		*pat = at;

	return at ? true : false;
}

/*
 * return information about a attached submenu.
 */
int
inquire_menu(int lock, struct xa_client *client, XA_TREE *wt, int item, XAMENU *mn)
{
	int ret = 0;

	Sema_Up(LOCK_CLIENTS);

	DIAG((D_menu,NULL,"inquire_menu for %s on %lx + %d",
		c_owner(client), (unsigned long)wt->tree, item));

	/* You can only attach submenu's to normal menu entries */
	if (wt->tree)
	{
		XA_MENU_ATTACHMENT *at;

		if (is_attach(client, wt, item, &at))
		{
			mn->wt = at->wt;
			mn->menu.mn_tree = at->wt->tree;
			mn->menu.mn_menu = at->menu;
			mn->menu.mn_item = at->item;
			mn->menu.mn_scroll = 0;
			mn->menu.mn_keystate = 0;
			ret = 1;
		}
	}

	Sema_Dn(LOCK_CLIENTS);

	return ret;
}

/*
 * Attach a submenu to a menu entry
 */
int
attach_menu(int lock, struct xa_client *client, XA_TREE *wt, int item, XAMENU *mn, on_open_attach *on_open, void *data)
{
	OBJECT *attach_to;
	int ret = 0;

	Sema_Up(LOCK_CLIENTS);

	attach_to = wt->tree + item;

	DIAG((D_menu, NULL, "attach_menu for %s %lx + %d to %lx + %d",
		c_owner(client), (unsigned long)mn->menu.mn_tree, mn->menu.mn_menu, (unsigned long)wt->tree, item));

	/* You can only attach submenu's to normal menu entries */
	if (wt->tree && mn && mn->wt)
	{
		XA_MENU_ATTACHMENT *new;

		new = kmalloc(sizeof(*new));

		if (new)
		{
			bzero(new, sizeof(*new));

			new->next = client->attach;
			if (client->attach)
				client->attach->prev = new;
			client->attach = new;

		/* A menu is attached by replacing ob_spec (the text string)
		 * to point to a structure in the above allocated table.
		 * The text string pointer is moved to the very first location of that
		 * structure. The remainder being additional information.
		 */

			/* OK now we can attach the menu */
			menu_spec(mn->wt->tree, mn->menu.mn_menu);
			attach_to->ob_flags |= OF_SUBMENU;
			new->to = wt;
			new->to_item = item;
			new->menu = mn->menu.mn_menu;	 /* This is the submenu */
			new->item = mn->menu.mn_item;	 /* This is the start item inside the submenu */
			new->wt = mn->wt;
			new->on_open = on_open;
			new->data = data;
			mn->wt->links++;
			ret = 1;
		}
	}
	else
	{
		DIAG((D_menu, NULL, "attach no tree(s)"));
	}

	Sema_Dn(LOCK_CLIENTS);
	DIAGS(("attach_menu exit ok"));
	return ret;
}

int
detach_menu(int lock, struct xa_client *client, XA_TREE *wt, int item)
{
	OBJECT *attach_to = wt->tree + item;
	XA_MENU_ATTACHMENT *xt;
	int ret = 0;

	Sema_Up(LOCK_CLIENTS);

	if (is_attach(client, wt, item, &xt))
	{
		DIAG((D_menu, NULL, "detach_menu %lx + %d for %s %lx + %d",
			(unsigned long)xt->wt->tree, xt->menu, client->name, (unsigned long)wt->tree, item));

		attach_to->ob_flags &= ~OF_SUBMENU;
		xt->to = NULL;
		xt->to_item = 0;

		if (xt->prev)
			xt->prev->next = xt->next;
		else
			client->attach = xt->next;

		if (xt->next)
			xt->next->prev = xt->prev;

		xt->wt->links--;
		kfree(xt);

		ret = 1;
	}

	Sema_Dn(LOCK_CLIENTS);
	return ret;
}

void
free_attachments(struct xa_client *client)
{
	XA_MENU_ATTACHMENT *a;

	while (( a = client->attach))
	{
		client->attach = a->next;
		DIAGS(("freeing remaining attachment %lx", (unsigned long)a));
		a->wt->links--;
		kfree(a);
	}
}

void
remove_attachments(int lock, struct xa_client *client, XA_TREE *wt)
{
	if (wt->tree)
	{
		int f = 0;

		DIAG((D_menu, NULL, "remove_attachments"));
		do {
			detach_menu(lock, client, wt, f);
		}
		while (!(wt->tree[f++].ob_flags & OF_LASTOB));
	}
}

static void
free_popinfo_resources(struct xa_popinfo *pi)
{
	int i;
	OBJECT *obtree = pi->wt->tree;


	if (pi->objs)
	{
		if (pi->scrl_start_row >= 0)
		{
			object_set_spec(obtree + pi->objs[pi->scrl_start_row], (long)pi->save_start_txt);
			object_set_spec(obtree + pi->objs[pi->count - 1], (long)pi->save_end_txt);
		}
		for (i = 0; i < (pi->count - 1); i++)
		{
			obtree[pi->objs[i]].ob_next = pi->objs[i + 1];
		}

		kfree(pi->objs);
		pi->objs = NULL;
	}
	pi->count = 0;
	pi->scrl_start_row = -1;
}

static void
clear_popinfo(struct xa_popinfo *pi)
{
	pi->wind = NULL;
	pi->wt = NULL;
	pi->parent = -1;
	pi->count = 0;
	pi->scrl_start_row = -1;
	pi->objs = NULL;

	pi->current = -1;

	pi->at_up = pi->at_down = NULL;
}

static void
clear_meninfo(struct xa_menuinfo *mi)
{
	mi->wind = NULL;
	mi->wt = NULL;
	mi->titles = -1;
	mi->popups = -1;
	mi->about = -1;
	mi->current = -1;
}

static Tab *
new_menutask(void)
{
	Tab *new;

	new = kmalloc(sizeof(*new));
	if (new)
		bzero(new, sizeof(*new));

	return new;
}

static void
free_menutask(Tab *tab)
{
// 	MENU_TASK *k = &tab->task_data.menu;

	DIAG((D_menu, tab->client, "free_menutask: Tab=%lx, tab->next=%lx, task_prev=%lx for %s",
		(unsigned long)tab, (unsigned long)NEXT_TAB(tab), (unsigned long)PREV_TAB(tab), tab->client->name));

	cancel_pop_timeouts();

	TAB_LIST_REMOVE(tab);

	free_popinfo_resources(&tab->task_data.menu.p);

	kfree(tab);
}

/*
 * Really should get the multi-thread support in here...
 * doesn't work at the moment though...
 *
 * HR: well, some kind of multithreading is now implemented;
 *     a cooperative kind of within the same process.
 */

 /* Next is top -> bottom */
 /* prev is bottom -> top */
Tab *
nest_menutask(Tab *tab)
{
	Tab *new_tab;
	MENU_TASK *new_mt, *mt;

	new_tab = new_menutask();

	DIAG((D_menu, NULL, "nest_menutask: new %lx, old %lx for %s",
		(unsigned long)new_tab, (unsigned long)tab, tab && tab->client ? tab->client->name : "initial"));

	if (new_tab)
	{
		if (TAB_LIST_START)
			new_tab->root = TAB_LIST_START->root;
		else
			new_tab->root = new_tab;

		TAB_LIST_INSERT_START(new_tab);
		new_mt = &new_tab->task_data.menu;
		clear_popinfo(&new_mt->p);
		clear_meninfo(&new_mt->m);
		if (tab)
		{
			new_tab->exit_mb = tab->exit_mb;

			new_tab->usr_evnt = tab->usr_evnt;
			new_tab->data	 = tab->data;

			mt	= &tab->task_data.menu;

			new_tab->ty	= tab->ty;
			new_tab->client	= tab->client;
			new_tab->lock	= tab->lock;
			new_tab->wind	= tab->wind;
			new_tab->widg	= tab->widg;

			new_mt->ty		= POP_UP;
			new_mt->bar		= mt->bar;
			new_mt->m.current	= mt->m.current;
		}
		else
			new_tab->ty = NO_TASK;

#if 0
		new_mt->p.current	= -1;
#endif
	}
	return new_tab;
}


struct appmenu
{
	char name[200];
	struct xa_client *client;
};

static struct appmenu *appmenu = NULL;
static OBJECT *appmenu_ob = NULL;
static size_t appmenusize = 0;

static const OBJECT drop_box =
{
	-1, 1, 2,			/* Object 0  */
	G_BOX, OF_NONE, OS_SHADOWED,
	{ 0x00FF1100L },
	0, 0, 12, 2
};
static const OBJECT drop_choice =	/* Object 1 to n */
{
	0,-1,-1,
	G_STRING, OF_NONE, OS_NORMAL,
	{ (long)"-" },
	0, 1, 12, 1
};

void
free_desk_popup(void)
{
	if (appmenu)
	{
		kfree(appmenu);
		appmenu = NULL;
	}
	if (appmenu_ob)
	{
		kfree(appmenu_ob);
		appmenu_ob = NULL;
	}
	appmenusize = 0;
}

static OBJECT *
built_desk_popup(int lock, short x, short y)
{
	int n, i, xw, obs, split;
	OBJECT *ob;
	struct xa_client *client, *fo;
	size_t maxclients;

	Sema_Up(LOCK_CLIENTS);

	maxclients = client_list_size() + 2; /* how many apps do we have */
	maxclients = (maxclients + 31) & ~31; /* increase by 32 slots */

	if (appmenusize < maxclients)
	{
		struct appmenu *tmp;
		OBJECT *tmp_ob;

		tmp = kmalloc(sizeof(*tmp) * maxclients);
		tmp_ob = kmalloc(sizeof(*tmp_ob) * maxclients);

		if (tmp && tmp_ob)
		{
			free_desk_popup();
// 			if (appmenu) kfree(appmenu);
// 			if (appmenu_ob) kfree(appmenu_ob);

			appmenu = tmp;
			appmenu_ob = tmp_ob;
			appmenusize = maxclients;
		}
		else
		{
			if (tmp) kfree(tmp);
			if (tmp_ob) kfree(tmp_ob);
		}
	}
	assert(appmenu && appmenu_ob);

	/*
	 * go throug all the clients, store pid and name
	 * count in n
	 */

	DIAGS(("built_desk_popup: fetch ACC/AES names.."));

	n = 0;

	FOREACH_CLIENT(client)
	{
		if (client == C.Hlp)
			continue;

		if ((client->type & APP_ACCESSORY) || client == C.Aes)
		{
			if (n < appmenusize)
			{
				DIAGS((" - %s", client->name));

				appmenu[n].client = client;
				if (cfg.menupop_pids)
				{

					sprintf(appmenu[n].name, sizeof(appmenu[n].name),
						"  %d(%d)->%d %s",
						client->p->ppid,
						client->rppid,
						client->p->pid,
						*client->name ? client->name + 2 : client->proc_name);
				}
				else
				{
					sprintf(appmenu[n].name, sizeof(appmenu[n].name),
						"  %s",
						*client->name ? client->name + 2 : client->proc_name);
				}
				n++;
			}
		}
	}

	appmenu[n].client = NULL;
	strcpy(appmenu[n].name, "-");
	split = n++;

	DIAGS(("built_desk_popup: fetch APP names.."));

	FOREACH_CLIENT(client)
	{
		if (client == C.Hlp)
			continue;

		if (!(client->type & APP_ACCESSORY) && client != C.Aes)
		{
			if (n < appmenusize)
			{
				char *name=	*client->name ? client->name + 2 : client->proc_name;
							  /* You can always do a menu_register
							   * in multitasking AES's  :-) */

				if( *name <= ' ' )
					continue;	/* avoid empty entries */
				appmenu[n].client = client;
				if (cfg.menupop_pids)
				{
					sprintf(appmenu[n].name, sizeof(appmenu[n].name),
						"  %d(%d)->%d %s",
						client->p->ppid,
						client->rppid,
						client->p->pid,
						name );
				}
				else
				{
					sprintf(appmenu[n].name, sizeof(appmenu[n].name),
						"  %s", name );
				}
				n++;
			}
		}
	}

	if (split == n - 1)		/* dont want to see split when last */
		n--;

	Sema_Dn(LOCK_CLIENTS);
	DIAGS(("built_desk_popup: building object.."));
	obs = n + 1;
	ob = appmenu_ob;

	ob[0] = drop_box;
	ob[0].ob_tail = obs-1;
	ob[0].ob_height = obs-1;

	xw = 0;
	fo = find_focus(false, NULL, NULL, NULL);
	DIAGS(("built_desk_popup: client with focus = %s", fo->name));

	/* fix the tree */
	for (i = 0; i < n; i++)
	{
		int j = i + (obs - n);
		int m;

		ob[j] = drop_choice;
		if (i == split)
		{
			ob[j].ob_state = OS_DISABLED;
		}
		else
		{
			char *txt = appmenu[i].name;

			if (appmenu[i].client == fo)
				ob[j].ob_state |= OS_CHECKED;
			else
				ob[j].ob_state &= ~OS_CHECKED;

			if (any_hidden(lock, appmenu[i].client, NULL))
				*(txt + 1) = '*';
			else
				*(txt + 1) = ' ';

			//if (appmenu[i].client->type == APP_SYSTEM)
			//	*(txt + 1) = '+';

			ob[j].ob_spec.free_string = txt;
		}

		m = strlen(ob[j].ob_spec.free_string);
		if (m > xw)
			xw = m;

		ob[j].ob_next = j + 1;
		ob[j].ob_y = i;
	}
	ob[obs - 1].ob_flags |= OF_LASTOB;
	ob[obs - 1].ob_next = 0;

	xw++;
	for (i = 0; i < obs; i++)
		ob[i].ob_width = xw;

	if (split < n)
	{
		for (i = 0; i < xw; i++)
			appmenu[split].name[i] = '-';
// 		memset(appmenu[split].name, '-', xw);
		appmenu[split].name[xw] = '\0';
	}

	for (i = 0; i < obs; i++)
		obfix(ob, i, DU_RSX_CONV, DU_RSY_CONV);

	ob[0].ob_x = x;
	ob[0].ob_y = y;

	menu_spec(ob, 0);

	DIAGS(("built_desk_popup: return %lx", (unsigned long)appmenu_ob));
	return appmenu_ob;
}

static bool
desk_menu(Tab *tab)
{
	MENU_TASK *k = &tab->task_data.menu;

	return    tab->ty == ROOT_MENU
	       && !NEXT_TAB(tab)
	       && k->m.current == k->m.wt->tree[k->m.titles].ob_head;
}

static void
title_tnormal(Tab *tab)
{
	MENU_TASK *k = &tab->task_data.menu;

	if (barred(tab))
	{
		if (!NEXT_TAB(tab))
		{
			if (k->m.current > 0)
				change_title(tab, 0);
		}
	}
}

static Tab *
menu_pop(Tab *tab)
{
	struct xa_window *w;
	MENU_TASK *k = &tab->task_data.menu;
	OBJECT *obtree = k->p.wt->tree;
	struct proc *p = get_curproc();

	DIAG((D_menu, tab->client, "menu_pop: tab=%lx for %s", (unsigned long)tab, tab->client->name));

	if( !(tab->ty == ROOT_MENU || p == k->p.wt->owner->p) )
	{
		BLOG((0, "menu_pop: ERROR:(%s,%s)", p->name, k->p.wt->owner->p->name ));
		/* todo: pop delayed? */
		return 0;
	}
	cancel_pop_timeouts();

	k->p.wt->dx = k->p.wt->dy = 0;

	change_entry(tab, 0);

	w = k->p.wind;
	if (w)		/* Windowed popup's */
	{
		close_window(tab->lock, w);
		delete_window(tab->lock, w);
		k->p.wind = NULL;
	}
	else
	{
		hidem();
		(*xa_vdiapi->form_restore)(0, k->drop, &(k->Mpreserve));
		showm();
	}

	if (barred(tab))
	{
		if (!NEXT_TAB(tab))
			obtree[k->p.parent].ob_flags |= OF_HIDETREE;
		if (desk_menu(tab))
			/* remove attached clients popup */
			detach_menu(tab->lock, C.Aes, k->m.wt, k->m.about + 2);
	}

	if (NEXT_TAB(tab))
	{
		Tab *t = tab;
		MENU_TASK *b;

		tab = NEXT_TAB(tab);
		b = &tab->task_data.menu;

		b->x = k->x;		/* pass back current x & y */
		b->y = k->y;

		b->p.at_up = k->p.at_down = NULL;
#if 0
		b->p.attach_wt = NULL;
		b->p.attach_item = -1;
		b->p.attached_to = -1;
#endif
		free_menutask(t);
	}
	return tab;
}


/*
 * This is always called to end a menu navigation session
*/
static void
menu_finish(struct task_administration_block *tab)
{
	struct xa_client *client;
	struct proc *proc;
	bool is_bar;

	/* after !menu_title */
	if (tab)
	{
		MENU_TASK *k = &tab->task_data.menu;
		OBJECT *obtree = k->m.wt->tree;

		cancel_pop_timeouts();

		is_bar = barred(tab);
		client = tab->client;

		DIAG((D_menu, tab->client, "[%d]menu_finish, tab=%lx, ty:%d for %s", tab->dbg2, (unsigned long)tab, tab->ty, tab->client->name));

		if (is_bar && !NEXT_TAB(tab))
			obtree[k->m.popups].ob_flags |= OF_HIDETREE;

		if (tab->widg)
			tab->widg->start = 0;

		k->stage = NO_MENU;

		free_menutask(tab);
		client->status &= ~CS_MENU_NAV;
	}
	else
	{
		DIAG((D_menu, NULL, "[0]menu_finish"));
		client = C.Aes;
	}

	C.Aes->waiting_for = XAWAIT_MENU; /* ready for next menu choice */
	C.Aes->em.flags = MU_M1;
	C.Aes->status &= ~CS_MENU_NAV;

	if ((proc = menustruct_locked()))
	{
		/* If someone wants its menu ontop, it is indicated
		 * by C.next_menu (set in swap_menu, app_man.c)
		 */
		if (C.next_menu)
		{
			set_next_menu(C.next_menu, true, false);
			C.next_menu = NULL;
		}
		unlock_menustruct(proc);
	}
	toggle_menu(0, -2);
}

Tab *
collapse(Tab *tab, Tab *upto)
{
	DIAG((D_menu, tab->client,"collapse tab=%lx, upto=%lx for %s", (unsigned long)tab, (unsigned long)upto, tab->client->name));

	while (tab != upto)
	{
		Tab *x = tab;		/* N.B. tab doesnt change when the last drop is popped */
		tab = menu_pop(tab);	/* menu_pop() never frees the root TAB .. the one at the bottom of the list
					 * It returns the same ptr it gets when ptr is the root TAB
					 */
		if (!tab || tab == x)
			break;
	}
	return tab;
}

void
popout(struct task_administration_block *tab)
{
	Tab *t;

	DIAG((D_menu, NULL, "popout %d", tab->dbg));
	DIAG((D_menu, NULL, "[1]collapse"));

	if( C.update_lock && C.update_lock != get_curproc() )
	{
		BLOG((0,"popout:ERROR:update_lock != get_curproc"));
		return;
	}
	if (tab)
	{
		t = collapse(tab, NULL);
		title_tnormal(t);

		IFDIAG(tab->dbg2 = 1;)

		menu_finish(t);
	}
}

static GRECT
rc_inside(GRECT r, GRECT o)
{
	if (r.g_x < o.g_x || r.g_w > o.g_w)
		r.g_x = o.g_x;
	if (r.g_y < o.g_y || r.g_h > o.g_h)
		r.g_y = o.g_y;

	if (r.g_w < o.g_w && r.g_h < o.g_h)
	{
		if (r.g_x + r.g_w > o.g_x + o.g_w)
			r.g_x = o.g_x + o.g_w - r.g_w;
		if (r.g_y + r.g_h > o.g_y + o.g_h)
			r.g_y = o.g_y + o.g_h - r.g_h;
	}
	return r;
}

static GRECT
popup_inside(Tab *tab, GRECT r)
{
	MENU_TASK *k = &tab->task_data.menu;
	GRECT ir = root_window->wa;
	GRECT rc = rc_inside(r, ir);

	if( rc.g_y < get_menu_height() )
		rc.g_y = get_menu_height();
	if (rc.g_x != r.g_x)
	{
		Tab *tx = NEXT_TAB(tab);

		if (tx && tx->ty == POP_UP)
		{
			MENU_TASK *kx = &tx->task_data.menu;

			rc.g_x = kx->drop.g_x - rc.g_w + 4;
			rc = rc_inside(rc, ir);
		#if 0
			if (m_inside(rc.g_x, rc.g_y, &kx->drop))
			{
				rc.g_x = kx->drop.g_x - rc.g_w + 4;
				rc = rc_inside(rc, ir);
			}
		#endif
		}
	}

	k->pdx = (k->p.wt->tree->ob_x - (r.g_x - rc.g_x));
	k->pdy = (k->p.wt->tree->ob_y - (r.g_y - rc.g_y));

	k->drop = rc;

	return rc;
}

static int
find_menu_object(struct widget_tree *wt, short start, short dx, short dy, short mx, short my, GRECT *c)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object found;

	obtree->ob_x = dx;
	obtree->ob_y = dy;

	if (c && !m_inside(mx, my, c))
		found = inv_aesobj();
	else
		found = obj_find(wt, aesobj(wt->tree, start), MAX_DEPTH, mx, my, c);

	return aesobj_item(&found);
}

#if 0
static short
next_menu_object(MENU_TASK *k)
{
	short nxt;
	OBJECT *obtree = k->wt->tree;

	nxt = k->p.current;
	if (nxt > 1)
	{
		if ((nxt = obtree[nxt].ob_next) != k->item
	}

}
#endif

static void
menu_area(struct widget_tree *wt, int item, short dx, short dy, GRECT *c)
{
	short sx, sy;

	sx = wt->tree->ob_x;
	sy = wt->tree->ob_y;
	wt->tree->ob_x = dx;
	wt->tree->ob_y = dy;
	obj_area(wt, aesobj(wt->tree, item), c);
	wt->tree->ob_x = sx;
	wt->tree->ob_y = sy;
}

static void
display_popup(Tab *tab, short rdx, short rdy)
{
	struct xa_popinfo *pi = &tab->task_data.menu.p;
	struct widget_tree *wt = pi->wt;
	OBJECT *obtree = wt->tree;
	MENU_TASK *k = &tab->task_data.menu;
	struct xa_window *wind;
	XA_WIND_ATTR tp = TOOLBAR;
	GRECT r;
	bool mod_h = false;
	int mg = MONO ? 0 : 0;

	wt->dx = 0;
	wt->dy = 0;

	obtree->ob_x = k->rdx = rdx; /* This is where we want to draw the popup object. */
	obtree->ob_y = k->rdy = rdy;

	obj_rectangle(wt, aesobj(wt->tree, pi->parent), &r);
/* ************ */
	if (tab->scroll)
	{
		if (r.g_h > (tab->scroll/*8*/ * screen.c_max_h))
		{
			mod_h = true;
			r.g_h = tab->scroll/*8*/ * screen.c_max_h;
		}
	}
	if (cfg.popscroll && r.g_h > cfg.popscroll * screen.c_max_h)
	{
		mod_h = true;
		r.g_h = cfg.popscroll * screen.c_max_h;
	}

/* ************ */
	DIAG((D_menu, tab->client, "display_popup: tab=%lx, %d/%d/%d/%d", (unsigned long)tab, r.g_x, r.g_y, r.g_w, r.g_h));

	r = calc_window(tab->lock, tab->client, WC_BORDER, tp, created_for_AES|created_for_POPUP, mg, true, &r);

	DIAG((D_menu, tab->client, "display_popup: rdx/y %d/%d (%d/%d/%d/%d)",
		rdx, rdy, r.g_x, r.g_y, r.g_w, r.g_h));
	DIAG((D_menu, tab->client, " -- scroll=%d, menu_locking=%d",
		tab->scroll, cfg.menu_locking));

	if ((r.g_y + r.g_h) > (root_window->wa.g_y + root_window->wa.g_h))
		r.g_y -= ((r.g_y + r.g_h) - (root_window->wa.g_y + root_window->wa.g_h));

	if (r.g_y < root_window->wa.g_y)
	{
		r.g_y = root_window->wa.g_y;
		if (r.g_h > root_window->wa.g_h - screen.c_max_h)
		{
			mod_h = true;
			r.g_h = root_window->wa.g_h - screen.c_max_h;
		}
	}

	if (r.g_y + r.g_h > root_window->wa.g_y + root_window->wa.g_h)
	{
		mod_h = true;
		r.g_h = root_window->wa.g_h - r.g_y;
	}

	/* if client blocked (e.g. nonwindowed dialog) restore background on exit */
	if( C.update_lock == tab->client->p )
		tp |= STORE_BACK;

	if (mod_h)
	{

		r = calc_window(tab->lock, tab->client, WC_WORK, tp, created_for_AES|created_for_POPUP, mg, true, &r);

		tp |= TOOLBAR|VSLIDE|UPARROW|DNARROW;

		r = calc_window(tab->lock, tab->client, WC_BORDER, tp, created_for_AES|created_for_POPUP, mg, true, &r);
	}

	r = popup_inside(tab, r);
	obtree->ob_x = k->pdx;
	obtree->ob_y = k->pdy;

	wind = create_window(	tab->lock,
				do_winmesag,
				do_formwind_msg,
				tab->client,
				true, //cfg.menu_locking,	/* yields nolist if locking. */
				tp,
				created_for_AES|created_for_POPUP,
				mg, true,
				&r,
				&r, NULL);

	if (wind)
	{
		GRECT or;
		obj_rectangle(wt, aesobj(wt->tree, pi->parent), &or);
		k->drop = wind->wa;

		k->pdx = obtree->ob_x + (k->drop.g_x - or.g_x);
		k->pdy = obtree->ob_y + (k->drop.g_y - or.g_y);
		wt->pdx = k->drop.g_x;
		wt->pdy = k->drop.g_y;

		k->p.wind = wind;

		DIAG((D_menu, tab->client, "drop: %d/%d,%d/%d", r.g_x, r.g_y, r.g_w, r.g_h));
		set_popup_widget(tab, wind, pi->parent);
		/* top menu-owner-window */
		if( tab->wind && tab->wind != TOP_WINDOW && tab->wind != root_window )
			top_window(tab->lock, true, true, tab->wind);
		if (!cfg.menu_locking)
		{
			DIAG((D_menu, tab->client, "nolocking pop window %lx + %d", (unsigned long)obtree, pi->parent));
		}
		else
		{
			DIAG((D_menu, tab->client, "locking pop window %lx + %d", (unsigned long)obtree, pi->parent));
		}
		open_window(tab->lock, wind, r);
	}
}

static void
do_popup(Tab *tab, XA_TREE *wt, short item, short entry, TASK *click, short rdx, short rdy)
{
	OBJECT *root = wt->tree;
	MENU_TASK *k = &tab->task_data.menu;
	//short x, y;

	DIAG((D_menu, tab->client, "do_popup: tab=%lx for %s", (unsigned long)tab, tab->client->name));
	C.boot_focus = 0;
	menu_spec(root, item);
	k->stage = IN_DESK;
	k->p.current = -1;
	k->entry = menuclick;
	k->select = click;

	k->p.wt = wt;
	k->p.parent = item;

	display_popup(tab, rdx, rdy);

	k->em.m2 = k->drop;
	k->em.flags |= MU_M2;
	k->em.m2_flag = 0;			/* into popup */
	k->em.t2 = popup;
	k->outof = NULL;

	if (entry == -2)
	{
		struct xa_aes_object nxt;
		nxt = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), inv_aesobj(),
			0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_VERT|OBFIND_DOWN|OBFIND_HIDDEN|OBFIND_FIRST);
		if (valid_aesobj(&nxt))
			popup(tab, aesobj_item(&nxt));
	}
	else
	{
		if (entry == -1)
		{
#if 0
			check_mouse(wt->owner, NULL, &x, &y);
#else
			//x = rdx, y = rdy;
#endif

			/*if (m_inside(x, y, &k->drop))
			{
				k->x = x;
				k->y = y;
				popup(tab, -1);
			}*/

		}
		else
			popup(tab, entry);
	}
}

void
start_popup_session(Tab *tab, XA_TREE *wt, short item, short entry, TASK *click, short rdx, short rdy)
{
	DIAG((D_menu, tab->client, "start_popup_session: tab=%lx for %s",
		(unsigned long)tab, tab->client->name));

	check_mouse(tab->client, &tab->exit_mb, NULL, NULL);

	tab->client->status |= CS_MENU_NAV;
	do_popup(tab, wt, item, entry, click, rdx, rdy);
}

static struct xa_client *
menu_client(Tab *tab)
{
	if (desk_menu(tab))
		return C.Aes;

	if (tab->widg)
	{
		XA_TREE *wt;
		wt = tab->widg->stuff;
		return wt->owner;
	}
	return tab->client;
}

static void cancel_popout_timeout(void);

static void
do_timeout_popup(Tab *tab)
{
	MENU_TASK *k = &tab->task_data.menu;
	GRECT tra;
	short asel, rdx, rdy;
	TASK *click;
	OBJECT *ob;
	Tab *new;
	XA_TREE *new_wt;
	XA_MENU_ATTACHMENT *at;

	asel = k->attach_select;
	k->attach_select = 0;

	if (!asel)
		asel = -1;

	cancel_popout_timeout();

	DIAG((D_menu, NULL, "do_timeout_popup: tab=%lx", (unsigned long)tab));

	if (tab != TAB_LIST_START)
	{
		collapse(TAB_LIST_START, tab);
	}

	DIAG((D_menu, NULL, " --- is attach"));

	if (!is_attach(menu_client(tab), k->p.wt, k->p.current, &at))
		return;

	if (desk_menu(tab))
		click = click_desk_popup;
	else if (tab->ty == POP_UP)
		click = click_popup_entry;
	else
		click = click_menu_entry;

	new_wt = at->wt;
	ob = new_wt->tree;
	menu_area(k->p.wt, k->p.current, k->pdx, k->pdy, &tra);

	DIAG((D_menu, NULL, " --- attach=%lx, wt=%lx, obtree=%lx",
		(unsigned long)at, (unsigned long)new_wt, (unsigned long)ob));

	ob->ob_x = 0, ob->ob_y = 0;
	obj_offset(new_wt, aesobj(new_wt->tree, at->item), &rdx, &rdy);

	rdx = tra.g_x - rdx;
	rdy = tra.g_y - rdy;

	if (click == click_desk_popup)
		rdy += screen.c_max_h;
	else
		rdx += k->drop.g_w - 4;

	k->p.at_up = at;
	new = nest_menutask(tab);
	new->task_data.menu.p.at_down = at;
	if (at->on_open)
		(*at->on_open)(at);

	do_popup(new, new_wt, at->menu, asel, click, rdx, rdy);
}
static void
do_collapse(Tab *tab)
{
	if (tab != TAB_LIST_START)
	{
		collapse(TAB_LIST_START, tab);
	}
	else
	{
		struct rawkey key;
		key.aes = SC_ESC;	/* close window-menu */
		menu_keyboard(tab, &key);
	}

}

static void
CE_do_popup(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		S.popin_timeout_ce = NULL;
		do_timeout_popup(ce->ptr1);
	}
	else
		cancel_pop_timeouts();
}
static void
CE_do_collapse(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		S.popout_timeout_ce = NULL;
		do_collapse(ce->ptr1);
	}
	else
		cancel_pop_timeouts();
}

void close_window_menu(Tab *tab)
{
	if( tab && tab->ty == MENU_BAR )
		post_cevent(tab->client, CE_do_collapse, tab, NULL, 0,0, NULL,NULL);
}

static bool
CE_cb(struct c_event *ce, long arg)
{
	return true;
}

static void
cancel_CE_do_popup(void)
{
	if (S.popin_timeout_ce)
	{
		DIAGS((" -- cancel_CE_do_popup: funct=%lx", (unsigned long)S.popin_timeout_ce));
		cancel_CE(S.popin_timeout_ce, CE_do_popup, CE_cb, 0);
		S.popin_timeout_ce = NULL;
	}
}
static void
cancel_CE_do_collapse(void)
{
	if (S.popout_timeout_ce)
	{
		DIAGS((" -- cancel_CE_do_collapse: funct=%lx", (unsigned long)S.popout_timeout_ce));
		cancel_CE(S.popout_timeout_ce, CE_do_collapse, CE_cb, 0);
		S.popout_timeout_ce = NULL;
	}
}

static void
do_popup_to(struct proc *p, long arg)
{
	Tab *tab = (Tab *)arg;
	S.popin_timeout = NULL;
	S.popin_timeout_ce = tab->client;

	DIAGS((" -- do_popup_timeout: posting CE_do_popup(%lx) tab=%lx, to %s",
		(unsigned long)CE_do_popup, (unsigned long)tab, tab->client->name));

	post_cevent(tab->client, CE_do_popup, tab, NULL, 0,0, NULL,NULL);
}

static void
do_popout_timeout(struct proc *p, long arg)
{
	Tab *tab = (Tab *)arg;
	S.popout_timeout = NULL;
	S.popout_timeout_ce = tab->client;

	DIAGS((" -- do_popout_timeout: posting CE_do_collapse(%lx) tab=%lx, to %s",
		(unsigned long)CE_do_collapse, (unsigned long)tab, tab->client->name));

	post_cevent(tab->client, CE_do_collapse, tab, NULL, 0,0, NULL,NULL);
}

static void
cancel_popout_timeout(void)
{
	if (S.popout_timeout)
	{
		DIAGS((" -- cancelling popout timeout %lx", (unsigned long)S.popout_timeout));

		cancelroottimeout(S.popout_timeout);
		S.popout_timeout = NULL;
	}
	cancel_CE_do_collapse();
}

static void
set_popout_timeout(Tab *tab, bool instant)
{
	TIMEOUT *t;
	if (!S.popout_timeout && PREV_TAB(tab))
	{
		if (!instant && cfg.popout_timeout)
		{
			t = addroottimeout(cfg.popout_timeout, do_popout_timeout, 1);
			if ((S.popout_timeout = t))
				t->arg = (long)tab;
		}
		else
			do_popout_timeout(NULL, (long)tab);
	}
}

static void
cancel_popin_timeout(void)
{
	TIMEOUT *t;
	if ((t = S.popin_timeout))
	{
		Tab *tab = (void *)t->arg;
		cancelroottimeout(S.popin_timeout);
		S.popin_timeout = NULL;
		tab->task_data.menu.attach_select = 0;
	}
	cancel_CE_do_popup();
}

static void
cancel_pop_timeouts(void)
{
	cancel_popin_timeout();
	cancel_popout_timeout();
}

static Tab *
click_desk_popup(struct task_administration_block *tab, short item)
{
	int lock = tab->lock;
	MENU_TASK *k = &tab->task_data.menu;
	struct xa_client *client;
	struct xa_window *wind = tab->wind;
	int m;

	if (item == -1)
		m = find_menu_object(k->p.wt, k->p.parent, k->pdx, k->pdy, k->x, k->y, &k->drop) - 1;
	else if (item == -2)
		m = -1;
	else
		m = item - 1;

	IFDIAG(tab->dbg = 1;)

	popout(TAB_LIST_START);

	client = NULL;

	if (m >= 0 && m < appmenusize)
		client = appmenu[m].client;

	if (client)
	{
		DIAG((D_menu, NULL, "got client %s", c_owner(client)));

		if (client == C.Aes)
		{
			app_in_front(lock, client, true, true, true);
		}
		else
		{
			unhide_app(lock, client);

			if (client->type & APP_ACCESSORY)
			{
				DIAG((D_menu, NULL, "is an accessory"));
				/* found the reason some acc's wouldnt wake up: msgbuf[4] must receive
				 * the meu_register reply, which in our case is the pid.
				 */
				send_app_message(lock, wind, client, AMQ_NORM | AMQ_ANYCASE, QMF_CHKDUP,
							AC_OPEN,        0, 0, 0,
							client->p->pid, 0, 0, 0);
			}
			else if (client->type & APP_APPLICATION)
			{
				DIAG((D_menu, NULL, "is a real GEM client"));
				app_in_front(lock, client, true, true, true);
			}
		}
	}
	return NULL;
}


static Tab *
menu_bar(struct task_administration_block *tab, short item)
{
	MENU_TASK *k = &tab->task_data.menu;
	short title, dx, dy;


	dx = k->m.wt->dx;
	dy = k->m.wt->dy;
	k->m.wt->dx = k->m.wt->dy = 0;
	if (item == -1)
	{
		title = find_menu_object(k->m.wt, k->m.titles, k->rdx, k->rdy, k->x, k->y, tab->widg->r.g_y == 0 ? &tab->widg->r : &k->bar);	// ??
	}
	else if (item == -2)
		title = -1;
	else
		title = item;

	if (title < 0 || title == k->m.titles || (k->m.wt->tree[title].ob_state & OS_DISABLED))
	{
		k->em.flags |= MU_M1;
		k->em.m1_flag = 0;
		k->em.m1 = k->bar;
		k->em.t1 = menu_bar;
	}
	else if (title == k->m.current)
	{
		k->em.flags |= MU_M1;
		k->em.m1_flag = 1;
		obj_area(k->m.wt, aesobj(k->m.wt->tree, k->m.current), &k->em.m1);
		k->em.t1 = menu_bar;
		k->m.wt->dx = dx;
		k->m.wt->dy = dy;
	}
	else
	{
		if (TAB_LIST_START != tab)
		{
			tab = collapse(TAB_LIST_START, tab);
		}

		tab = menu_pop(tab);
		title_tnormal(tab);
		if (!menu_title(tab->lock, tab, item, tab->wind, tab->widg, tab->locker, NULL))
		{
			//menu_finish(tab);
			tab = NULL;
		}
	}
	return tab;
}

static Tab *
outofpop(Tab *tab, short item)
{
	OBJECT *obtree;
	MENU_TASK *k = &tab->task_data.menu;

	obtree = k->p.wt->tree;

	if (S.popin_timeout && S.popin_timeout->arg == (long)tab)
		cancel_popin_timeout();
	if (S.popout_timeout && S.popout_timeout->arg == (long)tab)
		cancel_popout_timeout();

// 	if (k->p.current > 0 && !(obtree[k->p.current].ob_state & OS_DISABLED) && k->p.attached_to != k->p.current)
	if (k->p.current > 0 && !(obtree[k->p.current].ob_state & OS_DISABLED) && (!k->p.at_up || k->p.at_up->to_item != k->p.current))
	{
		change_entry(tab, 0);
		k->p.current = -1;
	}

// 	if (k->p.attached_to > 0 && k->p.attached_to != k->p.current)
	if (k->p.at_up && k->p.at_up->to_item != k->p.current)
	{
		k->p.current = k->p.at_up->to_item; //k->p.attached_to;
		change_entry(tab, 1);
	}

	k->em.m2 = k->drop;
	k->em.flags |= MU_M2;
	k->em.m2_flag = 0;			/* into popup */
	k->em.t2 = popup;
	k->outof = NULL;

	return tab;
}

static bool
open_attach(Tab *tab, short asel, bool instant)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_MENU_ATTACHMENT *at;
	bool dis, ret = false;

	dis = (k->p.wt->tree[k->p.current].ob_state & OS_DISABLED);


	if (   !dis
	    && is_attach(menu_client(tab), k->p.wt, k->p.current, &at))
	{
// 		if (k->p.attached_to < 0 || (k->p.attached_to > 0 && k->p.attached_to != k->p.current) )
		if (!k->p.at_up || (k->p.at_up != at))
		{
			cancel_pop_timeouts();
			k->attach_select = asel;
			if (!instant && cfg.popup_timeout)
			{
				TIMEOUT *t;

				t = addroottimeout(cfg.popup_timeout, do_popup_to, 1);
				if (t)
				{
					t->arg = (long)tab;
				}
				S.popin_timeout = t;
			}
			else
			{
				do_timeout_popup(tab);
			}
		}
// 		else if (k->p.attached_to == k->p.current)
		else if (k->p.at_up && k->p.at_up == at)
		{
			cancel_pop_timeouts();
		}
		ret = true;
	}
	else if (PREV_TAB(tab))
	{
		cancel_popin_timeout();
		set_popout_timeout(tab, instant);
	}
	return ret;
}
/* This is called by popup menu's as well */
static Tab *
popup(struct task_administration_block *tab, short item)
{
	OBJECT *obtree;
	MENU_TASK *k = &tab->task_data.menu;
	int m;

	k->stage = IN_MENU;

	if (item == -1)
		m = find_menu_object(k->p.wt, k->p.parent, k->pdx, k->pdy, k->x, k->y, &k->drop);
	else if (item == -2)
		m = -1;
	else
		m = item;

	obtree = k->p.wt->tree;

	if (m < 0 || m == k->p.parent || (m > 0 && (obtree[m].ob_state & OS_DISABLED)))
	{
		XA_MENU_ATTACHMENT *at = k->p.at_up;
		short p = k->p.current;
//  		short a = k->p.attached_to, p = k->p.current;

		cancel_popin_timeout();

		if (p > 0 && (!at || (at->to_item != p)) && !(obtree[p].ob_state & OS_DISABLED))
// 		if (p > 0 && p != a && !(obtree[p].ob_state & OS_DISABLED))
		{
			change_entry(tab, 0);
			k->p.current = -1;
			set_popout_timeout(tab, false);
		}
		if (at && at->to_item != p)
// 		if (a > 0 && a != p)
		{
			k->p.current = at->to_item; //k->p.attached_to;
			change_entry(tab, 1);
			cancel_popout_timeout();
		}

		if (m < 0)
		{

			k->em.m2 = k->drop;
			k->em.flags |= MU_M2;
			k->em.m2_flag = 0;	/* into popup */
			k->em.t2 = popup;
			k->outof = NULL;
		}
		else
		{
			k->em.m2.g_x = k->x;
			k->em.m2.g_y = k->y;
			k->em.m2.g_w = 1;
			k->em.m2.g_h = 1;
			k->em.flags |= MU_M2;
			k->em.m2_flag = 1;
			k->em.t2 = popup;
			k->outof = outofpop;
		}
	}
	else
	{
		short off;
		short msg[8] = {WM_ARROWED, 0,0, k->p.wind->handle, WA_UPSCAN, 0, 0,0};
		GRECT r;

		if (m != k->p.current)
		{
			bool dis;

			obtree = k->p.wt->tree;
			dis = obtree[m].ob_state & OS_DISABLED;

			DIAG((D_menu, NULL, "popup: tab=%lx, obj=%d, point_at_menu=%d",
				(unsigned long)tab, m, k->p.current));

			if (   k->p.current > -1
			    && !(obtree[k->p.current].ob_state & OS_DISABLED))
			{
				change_entry(tab, 0);
			}

			k->p.current = m;

			if (!dis)
				change_entry(tab, 1);
			if (item == -1)
			{
				open_attach(tab, -1, false);
			}
		}

		menu_area(k->p.wt, m, k->pdx, k->pdy, &r);

		if (r.g_y + r.g_h > k->drop.g_y + k->drop.g_h)
		{
			off = (r.g_y + r.g_h) - (k->drop.g_y + k->drop.g_h);
			msg[4] = WA_UPSCAN;
			msg[5] = off;
			do_formwind_msg(k->p.wind, tab->client, 0,0, msg);
			menu_area(k->p.wt, m, k->pdx, k->pdy, &r);
		}
		else if (r.g_y < k->drop.g_y)
		{
			msg[4] = WA_DNSCAN;
			msg[5] = k->drop.g_y - r.g_y;
			do_formwind_msg(k->p.wind, tab->client, 0,0, msg);
			menu_area(k->p.wt, m, k->pdx, k->pdy, &r);
		}

		xa_rect_clip(&k->drop, &r, &k->em.m2);
		k->em.flags |= MU_M2;
		k->em.m2_flag = 1;	/* out of entry */
		k->em.t2 = popup;
		k->outof = outofpop;
	}
	return tab;
}

bool
find_pop(short x, short y, Tab **ret)
{
	bool in = false;
	Tab *tab;

	if (ret)
		*ret = NULL;

	FOREACH_TAB(tab)
	{
		GRECT r = tab->task_data.menu.drop;
		in = m_inside(x, y, &r);
		if (in)
		{
			if (ret)
				*ret = tab;
			break;
		}
		if (m_inside(x, y, &tab->task_data.menu.p.wind->r))
		{
			if (ret)
				*ret = tab;
			break;
		}
	}
	return in;
}

static Tab *
click_menu_entry(struct task_administration_block *tab, short item)
{
	MENU_TASK *k= &tab->task_data.menu;
	XA_MENU_ATTACHMENT *at = NULL;

	cancel_pop_timeouts();
	DIAG((D_menu, NULL, "click_menu_entry: stage=%d",k->stage));
	if (k->stage != IN_MENU)
	{
		DIAG((D_menu, NULL, "[3]collapse"));
		tab = collapse(TAB_LIST_START, NULL);
		title_tnormal(tab);
		menu_finish(tab);
		tab = NULL;
	}
	else
	{
		int lock = tab->lock;
		XA_TREE *pop_wt = k->p.wt, *men_wt = k->m.wt;
		struct xa_window *wind = tab->wind;
		int subm = (NEXT_TAB(tab) != NULL);
		short m, d, titles;
		short about, kc, ks;
// 		bool a = false;

		if (pop_wt)
		{
			if (item == -1)
				m = find_menu_object(pop_wt, k->p.parent, k->pdx, k->pdy, k->x, k->y, &k->drop);
			else if (item == -2)
				m = -1;
			else
				m = item;
		}

		if (!pop_wt || m < 0) //(m = (item == -1) ? find_menu_object(pop_wt, k->p.parent, k->pdx, k->pdy, k->x, k->y, &k->drop) : item) < 0)
		{
			popout(TAB_LIST_START);
			return NULL;
		}

		d = pop_wt->tree[m].ob_state & OS_DISABLED;
		if (!d)
			is_attach(menu_client(tab), pop_wt, m, &at);

		if (!at || k->clicks >= 1)
		{
			titles = k->m.titles;
			about = k->m.about;
			ks = k->p.parent;
			kc = k->m.current;

			tab = collapse(TAB_LIST_START, NULL);

			DIAG((D_menu, NULL, "%sABLED,ti=%d,ab=%d,kc=%d,ks=%d,m=%d",d ? "DIS" : "EN",
				titles, about, kc, ks, m));
			/*
			 * Ozk: Found out that one should not deselect the menu-title on behalf of
			 *	a client if a valid menu-entry is clicked on, so we dont :-)
			 */
			if (m > -1 && !d)
			{
				if (wind != root_window)
				{
					OBJECT *rs = pop_wt->tree;

					DIAG((D_menu, NULL, "indirect call"));
					wind->send_message(lock, wind, pop_wt->owner, AMQ_NORM | AMQ_ANYCASE, QMF_CHKDUP,
							   MN_SELECTED, 0, 0, kc,
							   m, (long)rs >> 16, (long)rs, ks);
				}
				else if (subm || (!subm && (kc != men_wt->tree[titles].ob_head || m == about)))
				{
					/* If it's a client program menu, send it a little message to say hi.... */
					if (pop_wt->owner != C.Aes)
					{
						OBJECT *rs = pop_wt->tree;

						DIAG((D_menu, NULL, "Calling send_app_message()"));
						DIAG((D_menu, NULL, "to %s,title=%d, point_at=%d", t_owner(pop_wt), kc, m));

						/* Note the AES4.0 extended message format...... */
						send_app_message(lock, wind, pop_wt->owner, AMQ_NORM | AMQ_ANYCASE, QMF_CHKDUP,
								 MN_SELECTED, 0, 0, kc,
								 m, (long)rs >> 16, (long)rs, ks);
					}
					else
					{
						/* Otherwise, process system menu clicks */
						DIAG((D_menu, NULL, "do_system_menu()"));
						title_tnormal(tab);
						pop_wt->tree[m].ob_state &= ~OS_SELECTED;	/* Deselect the menu entry */
						do_system_menu(lock, kc, m);
					}
				}
			}
			else
			{
				DIAG((D_menu, NULL, "Disabled entry, title_tnormal"));
				title_tnormal(tab);
			}
			menu_finish(tab);
			tab = NULL;
		}
		else
		{
// 			if (a && !d && k->p.attached_to != m)
			if (at && at->to_item != m)
			{
				do_timeout_popup(tab);
			}
		}
	}
	//IFDIAG(tab->dbg2 = 2;)
	return tab;
}

Tab *
click_popup_entry(struct task_administration_block *tab, short item)
{
	MENU_TASK *k = &tab->task_data.menu;
	struct xa_client *client = tab->client;
	struct widget_tree *wt = k->p.wt;
	XAMENU_RESULT *md = tab->data;
	short m;

	if (item == -1)
		m = find_menu_object(wt, k->p.parent, k->pdx, k->pdy, k->x, k->y, &k->drop);
	else if (item == -2)
		m = -1;
	else
		m = item;

	if (md)
	{
		md->menu.mn_tree = wt->tree;
		md->menu.mn_scroll = 0;
		vq_key_s(C.P_handle, &md->menu.mn_keystate);

		md->menu.mn_item = m;

		if (md->menu.mn_item >= 0 && (wt->tree[md->menu.mn_item].ob_state & OS_DISABLED) != 0)
			md->menu.mn_item = -1;

		if (md->menu.mn_item >= 0)
			md->at = k->p.at_down;
		else
			md->at = NULL;

		DIAG((D_menu, NULL, "click_popup_entry %lx + %d", (unsigned long)md->menu.mn_tree, md->menu.mn_item));

		IFDIAG(tab->dbg = 6;)
	}

	m = tab->usr_evnt;
	popout(TAB_LIST_START);			/* incl. screen unlock */
	client->usr_evnt |= m;

	return NULL;
}

Tab *
click_form_popup_entry(struct task_administration_block *tab, short item)
{
	MENU_TASK *k = &tab->task_data.menu;
	struct xa_client *client = tab->client;
	struct widget_tree *wt = k->p.wt;
	short m, ue, *dptr;

	if (item == -1)
		m = find_menu_object(wt, k->p.parent, k->pdx, k->pdy, k->x, k->y, &k->drop);
	else if (item == -2)
		m = -1;
	else
		m = item;

	if (m >= 0 && (wt->tree[m].ob_state & OS_DISABLED) != 0)
		m = -1;

	DIAG((D_menu, NULL, "click_form_popup_entry %lx + %d", (unsigned long)wt->tree, m));

	IFDIAG(tab->dbg = 7;)

	ue = tab->usr_evnt;

	dptr = (short *)tab->data;

	popout(TAB_LIST_START);			/* incl. screen unlock */

	if (dptr)
		*dptr = m;

	client->usr_evnt |= ue;

	return NULL;
}

/*
 * Menu Tree Widget display
 */
/*
 * Ozk: We enter here being in either a thread of wind->owner, or wind->owner itself
 * The menu, however, can be owned by someone else .. we check that.
*/
static void
Display_menu_widg(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	XA_TREE *wt = widg->stuff;
	OBJECT *obtree;

	assert(wt);
	obtree = rp_2_ap(wind, widg, &widg->ar);
	assert(obtree);


	if (wind->dial & created_for_POPUP)
	{
		GRECT r;
		short dx = wt->dx, dy = wt->dy;

		wt->dx = wt->dy = 0;
		obj_rectangle(wt, aesobj(wt->tree, widg->start), &r);
		wt->dx = dx;
		wt->dy = dy;

		obtree->ob_x = obtree->ob_x + (wt->pdx - r.g_x);
		obtree->ob_y = obtree->ob_y + (wt->pdy - r.g_y);

		//if (wind->nolist && (wind->dial & created_for_POPUP))
		//{
			//set_clip(&wind->wa);
			wt->rend_flags |= WTR_POPUP;
			draw_object_tree(0, wt, NULL, wind->vdi_settings, aesobj(wt->tree, widg->start), MAX_DEPTH, NULL, 0);
			wt->rend_flags &= ~WTR_POPUP;
			//clear_clip();
		//}
		//else
		//	draw_object_tree(0, wt, NULL, widg->start, MAX_DEPTH, NULL);
	}
	else //if (wt->menu_line)	/* HR 090501  menu in user window.*/
	{
		obtree->ob_x = widg->ar.g_x; //wt->rdx;
		obtree->ob_y = widg->ar.g_y; //wt->rdy;
		//obtree->ob_height = widg->r.h - 1;
		obtree->ob_width = obtree[obtree[0].ob_head].ob_width = widg->ar.g_w;
		wt->rend_flags |= WTR_ROOTMENU;
		draw_object_tree(0, wt, NULL, wind->vdi_settings, aesobj(wt->tree, 1), MAX_DEPTH, NULL, 0);
		wt->rend_flags &= ~WTR_ROOTMENU;
		(*wt->objcr_api->write_menu_line)(wind->vdi_settings, (GRECT*)&widg->ar); //obtree->ob_x);	/* HR: not in standard menu's object tree */
	}
}

static void
CE_display_menu_widg(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
		Display_menu_widg(ce->ptr1, ce->ptr2, (const GRECT *)&ce->r);
}

static bool
display_menu_widget(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);
	XA_TREE *wt = widg->stuff;

	DIAG((D_menu,wt->owner,"display_menu_widget on %d for %s%s (%lx)",
		wind->handle, t_owner(wt), wt->menu_line ? "; with menu_line" : "", (unsigned long)rc));

	if (!rc)
		rc = C.Aes;

	if (wt->owner == rc || wt->owner == C.Aes)
	{
		DIAG((D_menu, wt->owner, "normal display_menu_widget, wt->owner %s, rc %s",
			wt->owner->name, rc->name));
		Display_menu_widg(wind, widg, clip);
	}
	else
	{
		DIAG((D_menu, wt->owner, "posted display_menu_widget for %s by %s", wt->owner->name, rc->name));
		post_cevent(wt->owner,
			    CE_display_menu_widg,
			    wind,
			    widg,
			    0, 0,
			    clip,
			    NULL);
	}
	return true;
}

/* root function of a menu_bar task */
/* The task stages are driven by mouse rectangle events completely */
/* Called by client events sent by XA_move_event()
   and by do_widgets() only for the menu_bar itself */
/* Ozk: As of now, calling this from other than menu_owners context
 * will return false
 */
static bool
click_menu_widget(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	struct xa_client *client; //, *rc = lookup_extension(NULL, XAAES_MAGIC);
	struct proc *p = get_curproc();
	DIAG((D_menu, NULL, "click_menu_widget"));

	if (C.rdm || md->clicks > 1)
	{
		return false;
	}

	client = ((XA_TREE *)widg->stuff)->owner;

	/*
	 * Make sure we're in the right context
	*/
	if (client == C.Aes)
	{
		if (client->tp != p)
			return false;
	}
	else if (client->p != p)
		return false;

	if ( widg->stuff == get_menu())
	{
		if ( !lock_menustruct(client->p, true) )
		{
			return false;
		}
	}
	((XA_TREE *)widg->stuff)->owner->status |= CS_MENU_NAV;

	if (!menu_title(lock, NULL, -1, wind, widg, client->p->pid, md))
	{
		DIAGS(("menu_title failed?!"));
		//menu_finish(NULL);
	}

	return false;
}

bool
keyboard_menu_widget(int lock, struct xa_window *wind, struct xa_widget *widg)
{
	if (!TAB_LIST_START)
	{
		struct xa_client *client; //, *rc = lookup_extension(NULL, XAAES_MAGIC);
		struct proc *p = get_curproc();

		client = ((XA_TREE *)widg->stuff)->owner;

		/*
		 * Make sure we're in the right context
		*/
		if (client == C.Aes)
		{
			if (client->tp != p)
			{
				return false;
			}
		}
		else if (client->p != p)
		{
			return false;
		}

		if ( widg->stuff == get_menu())
		{
			if ( !lock_menustruct(client->p, true) )
			{
				return false;
			}
		}
		((XA_TREE *)widg->stuff)->owner->status |= CS_MENU_NAV;

		if (!menu_title(lock, NULL, -2, wind, widg, client->p->pid, NULL))
		{
			//menu_finish(NULL);
		}
	}
	else
	{
		if (TAB_LIST_START->task_data.menu.entry)
			(*TAB_LIST_START->task_data.menu.entry)(TAB_LIST_START, -2); //menuclick(tab, k->p.current);
	}
	return false;

}

static bool
menu_title(int lock, Tab *tab, short title, struct xa_window *wind, XA_WIDGET *widg, int locker, const struct moose_data *md)
{
	GRECT r;
	MENU_TASK *k;
	XA_TREE *wt;
	OBJECT *obtree;
	short item = 0;
	short f, n;

	DIAG((D_menu, NULL, "menu_title: tab=%lx, md=%lx", (unsigned long)tab, (unsigned long)md));

	if( widg->wind == root_window && cfg.menu_bar == 0 )
		return false;
	if (!tab)
	{
		tab = nest_menutask(NULL);
		if (!tab){
			return false;
		}
	}
	if (md)
	{
		tab->exit_mb = md->cstate;
	}

	DIAG((D_menu, NULL, " --- tab = %lx", (unsigned long)tab));

	k = &tab->task_data.menu;

	clear_popinfo(&k->p);

	tab->lock = lock;
	tab->locker = locker;
	tab->wind = wind;
	tab->widg = widg;
	wt = widg->stuff;
	tab->client = wt->owner;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &r);

	/* first entry into the menu_bar; initialize task */
	k->m.wt = wt;
	obtree = wt->tree;
	k->m.titles = obtree[obtree[0].ob_head].ob_head;
	k->m.popups  = obtree[0].ob_tail;
	k->m.about  = obtree[obtree[k->m.popups].ob_head].ob_head;

	check_mouse(wind->owner, &f, &k->x, &k->y);

	k->rdx = r.g_x;
	k->rdy = r.g_y;

	if (title == -1)
	{
		k->m.current = find_menu_object(k->m.wt, k->m.titles, k->rdx, k->rdy, k->x, k->y, NULL);
	}
	else if (title == -2)
		k->m.current = obtree[k->m.titles].ob_head;
	else
		k->m.current = title;

	if ( k->m.current > -1
	    && (obtree[k->m.current].ob_type == G_TITLE || obtree[k->m.current].ob_type == G_USERDEF)
	    && !(obtree[k->m.current].ob_state & OS_DISABLED))
	{
		wt->dx = wt->dy = 0;
		tab->ty = k->ty = (wind == root_window ? ROOT_MENU : MENU_BAR);
		k->stage = IN_TITLE;
		obj_area(wt, aesobj(wt->tree, k->m.titles), &k->bar);
		change_title(tab, 1);
		obtree[k->m.popups].ob_flags &= ~OF_HIDETREE;
		n = obtree[k->m.popups].ob_head;
		for (f = obtree[k->m.titles].ob_head; f != k->m.titles; f = obtree[f].ob_next)
		{
			if (f == k->m.current)
				item = n;
			else
				obtree[n].ob_flags |= OF_HIDETREE;

			n = obtree[n].ob_next;
		}

		obtree[item].ob_flags &= ~OF_HIDETREE;	/* Show the actual menu */
		k->entry = click_menu_entry;		/* obeyed by XA_MOUSE.C */
		k->select = NULL;

		if (desk_menu(tab))
		{
			desk_popup.menu.mn_tree = built_desk_popup(tab->lock, 24,24);
			desk_popup.menu.mn_menu = 0;
			desk_popup.menu.mn_item = 0;
			desk_popup.menu.mn_scroll = 0;
			desk_popup.menu.mn_keystate = 0;

			desk_popup.wt = &desk_wt;

			desk_wt.tree = desk_popup.menu.mn_tree;
			desk_wt.owner = C.Aes;
			clear_edit(&desk_wt.e);
			clear_focus(&desk_wt);

			attach_menu(tab->lock, C.Aes, wt, k->m.about + 2, &desk_popup, NULL,NULL);
		}

		k->p.wt = wt;
		k->p.parent = item;
		display_popup(tab, r.g_x, r.g_y);

		k->em.flags = MU_M1;
		k->em.m1_flag = 1;		/* fill out rect event data; out of title */
		obj_area(wt, aesobj(wt->tree, k->m.current), &k->em.m1);
		k->em.t1 = menu_bar;

		if (title == -2 || title > 0)
		{
			struct xa_aes_object o = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), inv_aesobj(),
				0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_VERT|OBFIND_DOWN|OBFIND_HIDDEN|OBFIND_FIRST);

			if (valid_aesobj(&o))
				popup(tab, o.item);
		}
		else
		{
			k->em.m2 = k->drop;
			k->em.flags |= MU_M2;
			k->em.m2_flag = 0;			/* into popup */
			k->em.t2 = popup;
			k->outof = NULL;
		}

		return true;
	}
	else
	{
		menu_finish(tab);
		return false;
	}
}

/*
 * Attach a menu to a window...probably let this be access via wind_set one day
 */
/* HR: Define loc completely outside this function,
       Click behaviour is a parameter (for popup's).
       300801: all XA_TREE instances in struct xa_window (no more calloc's) */

/* XXX - Fixme: What to do when widget_tree resource fetching fails!
 */
void
set_menu_widget(struct xa_window *wind, struct xa_client *owner, XA_TREE *menu)
{
	XA_WIDGET *widg = get_widget(wind, XAW_MENU);
	OBJECT *obtree = menu->tree;

	DIAG((D_widg, wind->owner, "set_menu_widget on %d for %s",
		wind->handle, w_owner(wind)));

	if (widg->stuff)
	{
		((XA_TREE *)widg->stuff)->widg = NULL;
		((XA_TREE *)widg->stuff)->links--;
	}

	if (owner)
		widg->owner = owner;
	else
		widg->owner = wind->owner;

	menu->is_menu   = true;
	menu->menu_line = true; //(wind == root_window);	/* menu in root window.*/
	menu->widg = widg;
	menu->links++;

	/* additional fix to fit in window */
	obtree->ob_width  = obtree[obtree->ob_head].ob_width  = obtree[obtree->ob_tail].ob_width  = wind->wa.g_w;
	obtree->ob_height = obtree[obtree->ob_head].ob_height = obtree[obtree->ob_tail].ob_height = widg->r.g_h - 1;
	obtree[obtree->ob_tail].ob_y = widg->r.g_h;


	widg->m.r.draw = display_menu_widget;
	widg->m.click = click_menu_widget;
	widg->m.drag = NULL /* drag_menu_widget */;
	widg->m.properties |= WIP_INSTALLED|WIP_ACTIVE|WIP_NODRAG;
	widg->state = OS_NORMAL;
	widg->stuff = menu;
	widg->stufftype = STUFF_IS_WT;
	widg->m.destruct = free_xawidget_resources;
	widg->start = 0;

	wind->tool = widg;
	wind->active_widgets |= XaMENU;

	widg->m.statusmask = XAWS_SHADED;
}

/*
 * Attach a popup to a window.
 */
static XA_TREE *
set_popup_widget(Tab *tab, struct xa_window *wind, int obj)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = k->p.wt;
	XA_WIDGET *widg = get_widget(wind, XAW_MENU);
	DrawWidg display_object_widget;
	int frame = wind->frame;

	if ( widg->stuff)
	{
		((XA_TREE *)widg->stuff)->widg = NULL;
		((XA_TREE *)widg->stuff)->links--;
	}


	wt->is_menu = true;
	wt->pop = obj;
	wt->widg = widg;
	wt->links++;
	wt->zen = true;
	wt->objcr_api = tab->client->objcr_api;
	wt->objcr_theme = tab->client->objcr_theme;

	if (frame < 0)
		frame = 0;

	widg->m.properties = WIP_NOTEXT | WIP_INSTALLED;
	widg->m.r.pos_in_row = LT;

	widg->m.r.tp = XaMENU;

	widg->m.r.xaw_idx = XAW_MENU;
 	widg->m.r.draw = display_menu_widget;
	widg->r.g_x = k->rdx - wind->r.g_x - frame;
	widg->r.g_y = k->rdy - wind->r.g_y - frame;
	widg->r.g_w = wt->tree->ob_width;
	widg->r.g_h = wt->tree->ob_height;

	widg->state = OS_NORMAL;
	widg->stuff = wt;
	widg->stufftype = STUFF_IS_WT;
	widg->m.destruct = free_xawidget_resources;
	widg->start = obj;

	wind->tool = widg;

	XA_slider(wind, XAW_HSLIDE, wt->tree[obj].ob_width, wind->wa.g_w, 0);
	XA_slider(wind, XAW_VSLIDE, wt->tree[obj].ob_height, wind->wa.g_h, 0);

	return wt;
}

/*
 * Perform a few fixes on a menu tree prior to installing it
 * (ensure title spacing & items fit into their menus)
 */

/* HR: N.B. Using standard draw_object_tree for the menu bar has many advantages :-)
 *          There is however 1 disadvantage: TOS doesnt do this, so we may expect some
 *          incompatabilities.
 */

void
fix_menu(struct xa_client *client, XA_TREE *menu, struct xa_window *wind, bool do_desk)
{
	int titles, menus, tbar, s_ob, t_ob;
	short h;
	OBJECT *root = menu->tree;

	DIAG((D_menu, NULL, "fixing menu 0x%lx", (unsigned long)root));

	tbar = root[0].ob_head;
	titles = root[tbar].ob_head;
	menus = root[0].ob_tail;

	h = (wind ? get_widget(wind, XAW_MENU)->r.g_h : get_menu_widg()->r.g_h) - 1;

	menu->area.g_w = root->ob_width = root[tbar].ob_width  = root[menus].ob_width = wind ? get_widget(wind, XAW_MENU)->r.g_w : get_menu_widg()->r.g_w; //screen.r.g_w;
	root->ob_height = root[titles].ob_height = root[tbar].ob_height = h;
	root[menus].ob_y = h + 1;
	wt_menu_area(menu);

	object_set_spec(root + tbar, menu_colors());

	/* Hide the actual menus (The big ibox) */
	root[menus].ob_flags |= OF_HIDETREE;

	t_ob = root[titles].ob_head;
	s_ob = root[menus].ob_head;

	while (t_ob != titles)
	{
		/* Hide the actual menu */
		root[s_ob].ob_flags |= OF_HIDETREE;
		menu_spec(root, s_ob);
		root[t_ob].ob_height = h;
		root[s_ob].ob_y = 0;
		t_ob = root[t_ob].ob_next;
		s_ob = root[s_ob].ob_next;
	}

	/* fix desk menu */
	if (do_desk)
	{
		int o;
		s_ob = root[menus].ob_head;
		t_ob = root[s_ob ].ob_head;

		/* LASTOB in next 3 entries - bogus structure? */
		for( o = 0; o < 3; o++ )
		{
			if( (root[t_ob+o].ob_flags & OF_LASTOB) )
			{
				break;
			}
		}
		if( o == 3 )
		{
			t_ob += 3;
			root[s_ob].ob_height = root[t_ob].ob_y - root[s_ob].ob_y;

			/* client->mnu_clientlistname is umalloced area */
			root[t_ob-1].ob_spec.free_string = client->mnu_clientlistname;
			DIAG((D_menu, NULL, "menufix for %s - adding object at %lx", client->name, (unsigned long)client->mnu_clientlistname));
		}
		else
			t_ob = 0;

		while (t_ob != s_ob)
		{
			root[t_ob].ob_flags |= OF_HIDETREE|OS_DISABLED;
			t_ob = root[t_ob].ob_next;
			if( t_ob < 0 || (root[t_ob].ob_flags & OF_LASTOB) )
				break;
		}
		if( t_ob != s_ob )
		{
			BLOG((0,"%s: fix_menu: wrong links? (s=%d,t=%d)", client->name, s_ob, t_ob));
		}
	}

	DIAG((D_menu, NULL, "done fix_menu()"));
}

static void menu_scrld_to(struct proc *p, long arg);
static void menu_scrlu_to(struct proc *p, long arg);

/*
 * return 0 if at end or 1 if more entries can be scrolled
 */
static short
menu_scroll_up(Tab *tab)
{
	struct xa_popinfo *pi = &tab->task_data.menu.p;
	short ret = 0;

	if ((pi->scrl_curr_obj + pi->scrl_height) != pi->count)
	{
		short i, first, last, y, sy, entry_h, scroll_h, flag = 0;
		struct xa_window *wind;
		OBJECT *obtree = pi->wt->tree, *s;

		if (pi->scrl_start_row == pi->scrl_curr_obj)
			flag |= 1;

		s = obtree + pi->objs[pi->scrl_start_row];

		entry_h = obtree[pi->objs[pi->scrl_curr_obj + pi->scrl_height - 2]].ob_height;
		scroll_h = 0;

		pi->scrl_curr_obj++;

		first = pi->scrl_curr_obj + 1;
		last = first + pi->scrl_height - 2;
		y = sy = s->ob_y + s->ob_height;

		if ((pi->scrl_curr_obj + pi->scrl_height) == pi->count)
			flag |= 2;
		else
			ret = 1;

		s->ob_next = pi->objs[first];

		for (i = first; i < last; i++)
		{
			s = obtree + pi->objs[i];
			s->ob_y = y;
			s->ob_next = pi->objs[i + 1];
			y += s->ob_height;
			scroll_h += s->ob_height;
		}
		obtree[pi->objs[last - 1]].ob_next = pi->objs[pi->count - 1];

		if (flag & 1)
		{
			s = obtree + pi->objs[pi->scrl_start_row];
			object_set_spec(s, (long)pi->scrl_start_txt);
// 			sy -= s->ob_height;
		}
		if (flag & 2)
		{
			s = obtree + pi->objs[pi->count - 1];
			object_set_spec(s, (long)pi->save_end_txt);
		}


		wind = pi->wind;

		if ((wind->nolist && nolist_list == wind) || (!wind->nolist && is_topped(wind)))
		{
			GRECT r, from, to;

			obj_rectangle(pi->wt, aesobj(pi->wt->tree, pi->parent), &r);
			r.g_y += sy;
			r.g_h = scroll_h;
			if (xa_rect_clip(&wind->wa, &r, &r))
			{
				if (r.g_h > entry_h)
				{
					from.g_x = to.g_x = r.g_x;
					from.g_y = to.g_y = r.g_y;
					from.g_w = to.g_w = r.g_w;
					from.g_h = to.g_h = r.g_h - entry_h;

					from.g_y += entry_h;

					hidem();
					(*xa_vdiapi->form_copy)(&from, &to);

					r.g_y += r.g_h - entry_h;
					r.g_h = entry_h;

				}
				if (flag & 1)
					redraw_entry(tab, pi->objs[pi->scrl_start_row]);
				if (flag & 2)
					redraw_entry(tab, pi->objs[pi->count - 1]);
				redraw_entry(tab, pi->objs[last - 1]);
				showm();
			}

		}
		else
		{
			generate_redraws(0, pi->wind, &pi->wind->wa, RDRW_WA);
		}

	}
	else
		ret = -1;

	return ret;
}
static short
menu_scroll_down(Tab *tab)
{
	struct xa_popinfo *pi = &tab->task_data.menu.p;
	short ret = 0;

	if (pi->scrl_start_row != pi->scrl_curr_obj)
	{
		short i, first, last, y, sy, entry_h, scroll_h, flag = 0;
		struct xa_window *wind;
		OBJECT *obtree = pi->wt->tree, *s;

		if ((pi->scrl_curr_obj + pi->scrl_height) == pi->count)
			flag |= 2;

		s = obtree + pi->objs[pi->scrl_start_row];

		entry_h = obtree[pi->objs[pi->scrl_curr_obj + 1]].ob_height;
		scroll_h = 0;

		pi->scrl_curr_obj--;

		first = pi->scrl_curr_obj + 1;
		last = first + pi->scrl_height - 2;
		y = sy = s->ob_y + s->ob_height;

		s->ob_next = pi->objs[first];

		if (pi->scrl_start_row == pi->scrl_curr_obj)
		{
			flag |= 1;
		}
		else
			ret = 1;

		for (i = first; i < last; i++)
		{
			s = obtree + pi->objs[i];
			s->ob_y = y;
			s->ob_next = pi->objs[i + 1];
			y += s->ob_height;
			scroll_h += s->ob_height;
		}
		obtree[pi->objs[last - 1]].ob_next = pi->objs[pi->count - 1];

		if (flag & 1)
		{
			s = obtree + pi->objs[pi->scrl_start_row];
			object_set_spec(s, (long)pi->save_start_txt);
		}
		if (flag & 2)
		{
			s = obtree + pi->objs[pi->count - 1];
			object_set_spec(s, (long)pi->scrl_end_txt);
		}

		wind = pi->wind;

		if ((wind->nolist && nolist_list == wind) || (!wind->nolist && is_topped(wind)))
		{
			GRECT r, from, to;

			obj_rectangle(pi->wt, aesobj(pi->wt->tree, pi->parent), &r);
			r.g_y += sy;
			r.g_h = scroll_h;
			if (xa_rect_clip(&wind->wa, &r, &r))
			{
				if (r.g_h > entry_h)
				{
					from.g_x = to.g_x = r.g_x;
					from.g_y = to.g_y = r.g_y;
					from.g_w = to.g_w = r.g_w;
					from.g_h = to.g_h = r.g_h - entry_h;

					to.g_y += entry_h;

					hidem();
					(*xa_vdiapi->form_copy)(&from, &to);

					r.g_y += sy;
					r.g_h = entry_h;

				}
				if (flag & 1)
					redraw_entry(tab, pi->objs[pi->scrl_start_row]);
				if (flag & 2)
					redraw_entry(tab, pi->objs[pi->count - 1]);
				redraw_entry(tab, pi->objs[first]);
				showm();
			}

		}
		else
		{
			generate_redraws(0, pi->wind, &pi->wind->wa, RDRW_WA);
		}
	}
	else
		ret = -1;

	return ret;
}

static void
CE_do_menu_scroll(int lock, struct c_event *ce, short cancel)
{
	Tab *tab = ce->ptr1;
	TIMEOUT *t = NULL;

	if (!cancel)
	{
		short ret, mb;

		check_mouse(ce->client, &mb, NULL,NULL);

		if (mb)
		{
			ret = !ce->d0 ? menu_scroll_down(tab) : menu_scroll_up(tab);

			check_mouse(ce->client, &mb, NULL,NULL);

			if (mb && ret == 1)
			{
				t = addroottimeout(cfg.menu_settings.mn_set.speed, !ce->d0 ? menu_scrld_to : menu_scrlu_to, 1);
				if (t)
					t->arg = (long)tab;
			}
		}
		S.menuscroll_timeout = t;
	}
}

static void
menu_scrld_to(struct proc *p, long arg)
{
	Tab *tab = (Tab *)arg;
	short mb;
	S.menuscroll_timeout = NULL;
	check_mouse(NULL, &mb, NULL,NULL);
	if (mb)
		post_cevent(tab->client, CE_do_menu_scroll, tab, NULL, 0,0, NULL, NULL);
}

static void
menu_scrlu_to(struct proc *p, long arg)
{
	Tab *tab = (Tab *)arg;
	short mb;
	S.menuscroll_timeout = NULL;
	check_mouse(NULL, &mb, NULL,NULL);
	if (mb)
		post_cevent(tab->client, CE_do_menu_scroll, tab, NULL, 1,0, NULL, NULL);
}

static Tab *
menuclick(Tab *tab, short item)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_MENU_ATTACHMENT *at;
	struct xa_popinfo *pi = &k->p;
	short m;

	cancel_pop_timeouts();
	DIAG((D_menu, NULL, "menuclick: tab=%lx", (unsigned long)tab));

	if (item == -1)
		m = find_menu_object(k->p.wt, k->p.parent, k->pdx, k->pdy, k->x, k->y, &k->drop);
	else if (item == -2)
		m = -1;
	else
		m = item;

	if (pi->objs && pi->scrl_start_row >= 0)
	{
		short mb, ret;

		if (m == pi->objs[pi->scrl_start_row])
		{
			ret = menu_scroll_down(tab);
			if (ret == 1)
			{
				check_mouse(tab->client, &mb, NULL,NULL);
				if (mb)
				{
					TIMEOUT *t;
					t = addroottimeout(cfg.menu_settings.mn_set.delay, menu_scrld_to, 1);
					if (t)
						t->arg = (long)tab;
					S.menuscroll_timeout = t;
				}
			}
			if (ret != -1)
				goto done;
		}
		else if (m == pi->objs[pi->count -1])
		{
			ret = menu_scroll_up(tab);

			if (ret == 1)
			{
				check_mouse(tab->client, &mb, NULL,NULL);
				if (mb)
				{
					TIMEOUT *t;
					t = addroottimeout(cfg.menu_settings.mn_set.delay, menu_scrlu_to, 1);
					if (t)
						t->arg = (long)tab;
					S.menuscroll_timeout = t;
				}
			}
			if (ret != -1)
				goto done;
		}
	}
	if (m > 0 && (item != -1 || k->clicks < 2) && is_attach(menu_client(tab), k->p.wt, m, &at))
	{
// 		if (!PREV_TAB(tab) || k->p.attached_to != m)
		if (!PREV_TAB(tab) || at->to_item != m)
			do_timeout_popup(tab);
	}
	else if (k->select)
		tab = k->select(tab, item);

done:
	if (tab && tab->root)
		tab->root->exit_mb = 0;

	return tab;
}

bool
menu_keyboard(Tab *tab, const struct rawkey *key)
{
	if (tab)
	{
		short keycode = key->aes, dir;
		MENU_TASK *k = &tab->task_data.menu;
		struct xa_aes_object nxt;

		nxt = inv_aesobj();

		DIAGS(("process menukey! %x", keycode));
		switch (keycode)
		{
		case SC_UPARROW:	/* 0x4800 */ /* UP arrow */
		{
			if (k->p.current > 0)
				dir = OBFIND_VERT|OBFIND_UP|OBFIND_HIDDEN;
			else
				dir = OBFIND_VERT|OBFIND_LAST;

			nxt = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), aesobj(k->p.wt->tree, k->p.current),
				0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, dir);


			DIAGS(("  up   - was %d, next %d", k->p.current, nxt.item));

			if (!valid_aesobj(&nxt))
				goto last;

			break;
		}
		case SC_DNARROW:	/* 0x5000 */ /* down arrow */
		{
			if (k->p.current > 0)
				dir = OBFIND_VERT|OBFIND_DOWN|OBFIND_HIDDEN;
			else
				dir = OBFIND_VERT|OBFIND_FIRST|OBFIND_HIDDEN;

			nxt = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), aesobj(k->p.wt->tree, k->p.current),
				0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, dir);

			DIAGS(("  down - was %d, next %d", k->p.current, nxt.item));

			if (!valid_aesobj(&nxt))
				goto first;
			break;
		}
		case SC_RTARROW:	/* 0x4d00 */ /* Right arrow */
		{
			bool a = false;

			if (k->p.current > 0)
			{
				nxt = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), aesobj(k->p.wt->tree, k->p.current),
					0, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_HOR|OBFIND_DOWN|OBFIND_HIDDEN|OBFIND_NOWRAP);
				if (!valid_aesobj(&nxt))
				{
					a = open_attach(tab, -2, true);
				}
			}
			if (!a && !valid_aesobj(&nxt))
			{
				tab = tab->root;
				k = &tab->task_data.menu;
				DIAGS(("  right, menu wt = %lx", (unsigned long)k->m.wt));
				if (k->m.wt)
				{
					nxt = ob_find_next_any_flagstate(k->m.wt, aesobj(k->m.wt->tree, k->m.titles), aesobj(k->m.wt->tree, k->m.current),
						0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_HOR|OBFIND_DOWN|OBFIND_HIDDEN);
					if (!valid_aesobj(&nxt))
						nxt = ob_find_next_any_flagstate(k->m.wt, aesobj(k->m.wt->tree, k->m.titles), inv_aesobj(),
							0, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_HOR|OBFIND_FIRST);
					if (valid_aesobj(&nxt))
						menu_bar(tab, nxt.item);
				}
				nxt = inv_aesobj();
			}
			break;
		}
		case SC_LFARROW:	/* 0x4b00 */ /* Left arrow */
		{
			if (k->p.current > 0)
			{
				nxt = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), aesobj(k->p.wt->tree, k->p.current),
					0, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_HOR|OBFIND_UP|OBFIND_HIDDEN|OBFIND_NOWRAP);
			}

			if (!valid_aesobj(&nxt))
			{
				if (NEXT_TAB(tab))
					set_popout_timeout(NEXT_TAB(tab), true);
				else
				{
					tab = tab->root;
					k = &tab->task_data.menu;
					DIAGS(("  left, menu wt = %lx", (unsigned long)k->m.wt));
					if (k->m.wt)
					{
						nxt = ob_find_next_any_flagstate(k->m.wt, aesobj(k->m.wt->tree, k->m.titles), aesobj(k->m.wt->tree, k->m.current),
							0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_HOR|OBFIND_UP|OBFIND_HIDDEN);
						if (!valid_aesobj(&nxt))
							nxt = ob_find_next_any_flagstate(k->m.wt, aesobj(k->m.wt->tree, k->m.titles), aesobj(k->m.wt->tree, k->m.current),
								0, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_HOR|OBFIND_LAST);
						if (valid_aesobj(&nxt))
							menu_bar(tab, nxt.item);
					}
					nxt = inv_aesobj();
				}
			}
			break;
		}
		case SC_SHFT_CLRHOME:	/* 0x4737 */ /* SHIFT+HOME */
		case SC_PGDN:		/* 0x5100 */ /* page down key (Milan &| emulators)   */
		case SC_END:		/* 0x4f00 */ /* END key (Milan &| emus)		*/
		{
last:
			nxt = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), inv_aesobj(),
				0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_VERT|OBFIND_DOWN|OBFIND_HIDDEN|OBFIND_LAST);
			DIAGS(("  found first obj %d, parent %d", nxt.item, k->p.parent));
			break;
		}
		case SC_CLRHOME:	/* 0x4700 */ /* HOME */
		case SC_PGUP:		/* 0x4900 */ /* page up key (Milan &| emulators)    */
		{
first:
			nxt = ob_find_next_any_flagstate(k->p.wt, aesobj(k->p.wt->tree, k->p.parent), inv_aesobj(),
				0, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_VERT|OBFIND_DOWN|OBFIND_HIDDEN|OBFIND_FIRST);
			DIAGS(("  found last obj %d, parent %d", nxt.item, k->p.parent));
			break;

		}
		case SC_RETURN:		/* 0x1c0d */
		case SC_NMPAD_ENTER:	/* 0x720d */
		case SC_SPACE:
		{
			if (k->p.current > 0)
			{
				DIAGS(("  spacekey on %d", k->p.current));
				if (k->entry)
					(*k->entry)(tab, k->p.current);
			}
			break;
		}
		case SC_ESC:	/* 0x011b */ /* escape */
		{
			if (k->entry)
				(*k->entry)(tab, -2);
			break;
		}
		}

		if (valid_aesobj(&nxt) && aesobj_item(&nxt) != k->p.current)
		{
			popup(tab, aesobj_item(&nxt));
		}
		return true;
	}
	return false;
}
