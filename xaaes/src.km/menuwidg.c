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

#include "xa_rsrc.h"
#include "form.h"


static Tab *
new_task(Tab *new)
{
	bzero(new, sizeof(*new));
	return new;
}

static void
free_task(Tab *t, short *nester)
{
	if (t)
	{
		if (t->nest && *nester)
		{
			nester--;
			free_task(t->nest, nester);
		}

		t->nest = 0;
		t->ty = NO_TASK;
		t->pr = 0;
		t->nx = 0;
		t->task = 0;
	}
}

static void popup(struct task_administration_block *tab);
static void where_are_we(struct task_administration_block *tab);
static void click_menu_entry(struct task_administration_block *tab);
static void new_title(struct task_administration_block *tab);

static XAMENU desk_popup;
static XA_TREE desk_wt;
static bool menu_title(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, int locker);
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
	     (screen.dial_colours.fg_col << 12)		/* border */
	   | (screen.dial_colours.fg_col << 8)		/* text */
	   |  screen.dial_colours.bg_col		/* inside */
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
	XA_TREE *wt = k->wt;
	OBJECT *obtree = wt->tree;
	int t = k->clicked_title;

	if (state)
		state = obtree[t].ob_state | OS_SELECTED;
	else
		state = obtree[t].ob_state & ~OS_SELECTED;

	obtree->ob_x = k->rdx;
	obtree->ob_y = k->rdy;

	obj_change(wt, //tab->widg->stuff,
		   t,
		   state,
		   obtree[t].ob_flags,
		   true,
		   NULL);
}

static void
change_entry(Tab *tab, int state)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = k->wt;
	OBJECT *obtree = wt->tree;
	short t = k->point_at_menu;

	DIAGS(("change entry: obtree=%lx, obj=%d, state=%d",
		obtree, k->point_at_menu, state));

	if (t < 0)
		return;

	if (state)
		state = obtree[t].ob_state | OS_SELECTED;
	else
		state = obtree[t].ob_state & ~OS_SELECTED;

	obtree->ob_x = k->rdx;
	obtree->ob_y = k->rdy;

	if (k->popw)
	{
		struct xa_rect_list rl;

		DIAGS(("change_entry: got popw"));
		rl.next = NULL;
		rl.r = k->popw->wa;

		obj_change(wt,
			   t,
			   state,
			   obtree[t].ob_flags,
			   true,
			   &rl);
	}
	else
	{
		DIAGS(("change_entry: no popw"));
		obj_change(wt, t, state, obtree[t].ob_flags, true, NULL);
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
		client->name, wt, item, wt->tree));

	DIAG((D_menu, client, "is_attach: at=%lx,flags=%x,type=%x, spec=%lx",
	       at, attach_to->ob_flags, attach_to->ob_type, object_get_spec(attach_to)->index));

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
inquire_menu(enum locks lock, struct xa_client *client, XA_TREE *wt, int item, XAMENU *mn)
{
	int ret = 0;

	Sema_Up(clients);

	DIAG((D_menu,NULL,"inquire_menu for %s on %lx + %d",
		c_owner(client), wt->tree, item));

	/* You can only attach submenu's to normal menu entries */
	if (wt->tree)
	{
		XA_MENU_ATTACHMENT *at;

		if (is_attach(client, wt, item, &at))
		{
			mn->wt = at->wt;
			mn->menu.mn_tree = at->wt->tree;
			mn->menu.mn_item = at->item;
			mn->menu.mn_scroll = 0;
			mn->menu.mn_keystate = 0;
			ret = 1;
		}
	}

	Sema_Dn(clients);

	return ret;
}

/*
 * Attach a submenu to a menu entry
 */
int
attach_menu(enum locks lock, struct xa_client *client, XA_TREE *wt, int item, XAMENU *mn)
{
	//XA_MENU_ATTACHMENT *at = client->attach;
	OBJECT *attach_to;
	int ret = 0;

	Sema_Up(clients);

	attach_to = wt->tree + item;

	DIAG((D_menu, NULL, "attach_menu for %s %lx + %d to %lx + %d",
		c_owner(client), mn->menu.mn_tree, mn->menu.mn_menu, wt->tree, item));

	/* You can only attach submenu's to normal menu entries */
	if (wt->tree && mn && mn->wt)
	{
		XA_MENU_ATTACHMENT *new;
		
		new = kmalloc(sizeof(*new));

		if (new)
		{
			char *text;

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
			new->item = mn->menu.mn_menu;	 /* This is the submenu */
			new->wt = mn->wt;

			if ((attach_to->ob_type & 0xff) == G_STRING)
			{
				text = object_get_spec(attach_to)->free_string;
				text[strlen(text)-1] = mn == &desk_popup ? '\2' : '>';
			}
			ret = 1;
		}
	}
	else
	{
		DIAG((D_menu, NULL, "attach no tree(s)"));
	}

	Sema_Dn(clients);
	DIAGS(("attatch_menu exit ok"));
	return ret;
}

int
detach_menu(enum locks lock, struct xa_client *client, XA_TREE *wt, int item)
{
	OBJECT *attach_to = wt->tree + item;
	XA_MENU_ATTACHMENT *xt;
	int ret = 0;

	Sema_Up(clients);

	if (is_attach(client, wt, item, &xt))
	{
		char *text;

		DIAG((D_menu, NULL, "detach_menu %lx + %d for %s %lx + %d",
			xt->wt->tree, xt->item, client->name, wt->tree, item));

		attach_to->ob_flags &= ~OF_SUBMENU;
		xt->to = NULL;
		xt->to_item = 0;
		if ((attach_to->ob_type & 0xff) == G_STRING)
		{
			text = object_get_spec(attach_to)->free_string;
			text[strlen(text)-1] = ' ';
		}

		if (xt->prev)
			xt->prev->next = xt->next;
		else
			client->attach = xt->next;

		if (xt->next)
			xt->next->prev = xt->prev;

		kfree(xt);

		ret = 1;
	}

	Sema_Dn(clients);
	return ret;
}

void
free_attachments(struct xa_client *client)
{
	XA_MENU_ATTACHMENT *a;

	while (( a = client->attach))
	{
		client->attach = a->next;
		DIAGS(("freeing remaining attachment %lx", a));
		kfree(a);
	}
}
	
void
remove_attachments(enum locks lock, struct xa_client *client, XA_TREE *wt)
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

/*
 * Really should get the multi-thread support in here...
 * doesn't work at the moment though...
 * 
 * HR: well, some kind of multithreading is now implemented;
 *     a cooperative kind of within the same process.
 */

static Tab *
nest_menutask(Tab *tab)
{
	MENU_TASK *b, *k;

	DIAG((D_menu, NULL, "nest_menutask"));
	/* Check the nest maximimum in the caller!!! */
	C.menu_base = &C.active_menu[++C.menu_nest];
	bzero(C.menu_base, sizeof(Tab));

	C.menu_base->nest = tab;
	b = &C.menu_base->task_data.menu,
	k = &tab->task_data.menu;

	C.menu_base->ty = b->ty = POP_UP;
	b->bar = k->bar;
	C.menu_base->ty = tab->ty;
	C.menu_base->client = tab->client;
	C.menu_base->pb = tab->pb;
	C.menu_base->lock = tab->lock;
	C.menu_base->wind = tab->wind;
	C.menu_base->widg = tab->widg;

	b->clicked_title = k->clicked_title;

	return C.menu_base;
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


static OBJECT *
built_desk_popup(enum locks lock, short x, short y)
{
	int n, i, xw, obs, split;
	OBJECT *ob;
	struct xa_client *client, *fo;
	size_t maxclients;

	Sema_Up(clients);

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
			if (appmenu) kfree(appmenu);
			if (appmenu_ob) kfree(appmenu_ob);

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
		if (client->type == APP_ACCESSORY || client == C.Aes)
		{
			if (n < appmenusize)
			{
				DIAGS((" - %s", client->name));

				appmenu[n].client = client;
				sprintf(appmenu[n].name, sizeof(appmenu[n].name),
					"  %d->%d %s",
					client->p->ppid,
					client->p->pid,
					*client->name 
						? client->name + 2
						: client->proc_name);
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
		if (client->type != APP_ACCESSORY && client != C.Aes)
		{
			if (n < appmenusize)
			{
				appmenu[n].client = client;
				sprintf(appmenu[n].name, sizeof(appmenu[n].name),
					"  %d->%d %s",
					client->p->ppid,
					client->p->pid,
					*client->name 
						? client->name + 2
						  /* You can always do a menu_register
						   * in multitasking AES's  :-) */
						: client->proc_name);
				n++;
			}
		}
	}

	if (split == n-1)		/* dont want to see split when last */
		n--;

	Sema_Dn(clients);
	DIAGS(("built_desk_popup: building object.."));
	obs = n+1;
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

			if (any_hidden(lock, appmenu[i].client))
				*(txt + 1) = '*';
			else
				*(txt + 1) = ' ';

			ob[j].ob_spec.free_string = txt;
		}

		m = strlen(ob[j].ob_spec.free_string);
		if (m > xw)
			xw = m;

		ob[j].ob_next = j+1;
		ob[j].ob_y = i;
	}
	ob[obs-1].ob_flags|=OF_LASTOB;
	ob[obs-1].ob_next = 0;

	xw++;
	for (i = 0; i < obs; i++)
		ob[i].ob_width = xw;

	if (split < n)
	{
		memset(appmenu[split].name, '-', xw);
		appmenu[split].name[xw] = '\0';
	}

	for (i = 0; i < obs; i++)
		obfix(ob, i);

	ob[0].ob_x = x;
	ob[0].ob_y = y;

	menu_spec(ob, 0);

	DIAGS(("built_desk_popup: return %lx", appmenu_ob));
	return appmenu_ob;
}

static bool
desk_menu(Tab *tab)
{
	MENU_TASK *k = &tab->task_data.menu;

	return    tab->ty == ROOT_MENU
	       && tab->nest == NULL
	       && k->clicked_title == k->wt->tree[k->titles].ob_head;
}

static Tab *
menu_pop(Tab *tab)
{
	struct xa_window *w;
	MENU_TASK *k = &tab->task_data.menu;
	OBJECT *obtree = k->wt->tree;

	DIAG((D_menu, NULL, "menu_pop"));
	if (barred(tab))
	{
		if (!tab->nest)
		{
			if (k->clicked_title > 0)
				change_title(tab, 0);
			obtree[k->pop_item].ob_flags |= OF_HIDETREE;
		}
		if (desk_menu(tab))
			/* remove attached clients popup */
			detach_menu(tab->lock, C.Aes, k->wt, k->about + 2);
	}
	
	change_entry(tab, 0);

	w = k->popw;
	if (w)		/* Windowed popup's */
	{	
		close_window(tab->lock, w);
#if 0
		if (cfg.menu_locking)
		{
			hidem();
			form_restore(k->border, k->drop, &(k->Mpreserve));
			showm();
		}
#endif
		delete_window(tab->lock, w);
		k->popw = NULL;
	}
	else
	{
		hidem();
		form_restore(k->border, k->drop, &(k->Mpreserve));
		showm();
	}

	k->wt->dx = k->wt->dy = 0;

	if (tab->nest)
	{
		MENU_TASK *b;

		C.menu_base = tab->nest;
		C.menu_nest--;
		b = &C.menu_base->task_data.menu;
		b->x = k->x;		/* pass back current x & y */
		b->y = k->y;
		tab = C.menu_base;
	}
	return tab;
}

/*
 * This is always called to end a menu navigation sesstion
*/
static void
menu_finish(struct task_administration_block *tab)
{
	struct xa_client *client;
	bool is_bar;
	
	/* after !menu_title */
	if (tab)
	{
		MENU_TASK *k = &tab->task_data.menu;
		OBJECT *obtree = k->wt->tree;

		is_bar = barred(tab);
		client = pid2client(tab->locker);

		DIAG((D_menu, NULL, "[%d]menu_finish, ty:%d", tab->dbg2, tab->ty));

		if (is_bar && !tab->nest)
			obtree[k->menus].ob_flags |= OF_HIDETREE;

		if (tab->widg)
			tab->widg->start = 0;

		k->stage = NO_MENU;
	}
	else
	{
		DIAG((D_menu, NULL, "[0]menu_finish"));
		is_bar = true;
		client = C.Aes;
	}

	free_task(C.menu_base, &C.menu_nest);
	C.menu_base = NULL;
	C.Aes->waiting_for = XAWAIT_MENU; /* ready for next menu choice */
	C.Aes->em.flags = MU_M1;

	if (menustruct_locked())
		unlock_menustruct(menustruct_locked());
}

Tab *
collapse(Tab *tab, Tab *upto)
{
	DIAG((D_menu,NULL,"collapse tab:%lx, upto:%lx", tab, upto));

	while (tab != upto)
	{
		Tab *x = tab;		/* N.B. tab doesnt change when the last drop is popped */
		tab = menu_pop(tab);
		if (!x->nest)
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

	t = collapse(tab, NULL);

	IFDIAG(tab->dbg2 = 1;)
	menu_finish(t);
}

static RECT
rc_inside(RECT r, RECT o)
{
	if (r.x < o.x || r.w > o.w)
		r.x = o.x;
	if (r.y < o.y || r.h > o.h)
		r.y = o.y;

	if (r.w < o.w && r.h < o.h)
	{
		if (r.x + r.w > o.x + o.w)
			r.x = o.x + o.w - r.w;
		if (r.y + r.h > o.y + o.h)
			r.y = o.y + o.h - r.h;
	}
	return r;
}

static RECT
popup_inside(Tab *tab, RECT r)
{
	MENU_TASK *k = &tab->task_data.menu;
	RECT ir = (tab->ty == ROOT_MENU) ? screen.r : root_window->wa;
	RECT rc = rc_inside(r, ir);

	if (rc.x != r.x)
	{
		Tab *tx = tab->nest;

		if (tx && tx->ty == POP_UP)
		{
			MENU_TASK *kx = &tx->task_data.menu;

			if (m_inside(rc.x, rc.y, &kx->drop))
			{
				rc.x = kx->drop.x - rc.w + 4;
				rc = rc_inside(rc, ir);
			}
		}
	}

	k->rdx = k->wt->tree->ob_x - (r.x - rc.x);
	k->rdy = k->wt->tree->ob_y - (r.y - rc.y);

	k->drop = rc;

	return rc;
}

static int
find_menu_object(Tab *tab, int start)
{
	MENU_TASK *k = &tab->task_data.menu;
	OBJECT *obtree = k->wt->tree;

	obtree->ob_x = k->rdx;
	obtree->ob_y = k->rdy;

	return obj_find(k->wt, start, MAX_DEPTH, k->x, k->y);
}


static void
menu_area(RECT *c, Tab *tab, int item)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = k->wt;
	short dx = 0;
	short dy = 0;

	if (k->popw)
	{
		dx = wt->dx;
		dy = wt->dy;
	}

	obj_area(wt, item, c);
}

static void
display_popup(Tab *tab, XA_TREE *wt, int item, short rdx, short rdy)
{
	OBJECT *obtree = wt->tree;
	MENU_TASK *k = &tab->task_data.menu;
	struct xa_window *wind;
	RECT r;
	int wash;

	k->pop_item = item;
	k->border = 0;
	k->wt = wt;

	wt->dx = 0;
	wt->dy = 0;

	obtree->ob_x = rdx; /* This is where we want to draw the popup object. */
	obtree->ob_y = rdy;

	obj_rectangle(wt, item, &r);
	//obj_area(wt, item, &r);
	DIAG((D_menu, tab->client, "display_popup: %d/%d/%d/%d", r));
	r = popup_inside(tab, r);
	wash = r.h;
	obtree->ob_x = k->rdx;
	obtree->ob_y = k->rdy;

	DIAG((D_menu, tab->client, "display_popup: rdx/y %d/%d (%d/%d/%d/%d)",
		rdx, rdy, r));
	DIAG((D_menu, tab->client, " -- scroll=%d, menu_locking=%d",
		tab->scroll, cfg.menu_locking));

#if 0
	if (tab->scroll && r.h > 8 * screen.c_max_h)
		r.h = 8 * screen.c_max_h;
#endif

	if (r.y + r.h > root_window->wa.y + root_window->wa.h)
		r.h = root_window->wa.h - r.y; 

	if (cfg.popscroll && r.h > cfg.popscroll * screen.c_max_h)
		r.h = cfg.popscroll * screen.c_max_h;

	if (!cfg.menu_locking || r.h < wash)
	{
		if (r.h < wash)
		{
			//int mg = MONO ? 2 : 1;
			XA_WIND_ATTR tp = TOOLBAR|VSLIDE|UPARROW|DNARROW|STORE_BACK;

			tab->scroll = true;

			r = calc_window(tab->lock, C.Aes, WC_BORDER, tp, 0, 1, 0, r);

			wind = create_window(	tab->lock,
						do_winmesag, //handle_form_window,
						do_formwind_msg,
						tab->client,
						cfg.menu_locking,	/* yields nolist if locking. */
						tp,
						created_for_AES|created_for_POPUP,
						0,1,0,
						r,
						&r, NULL);
		}
		else
		{
			int mg = MONO ? 2 : 1;
			XA_WIND_ATTR tp = TOOLBAR|STORE_BACK;

			r = calc_window(tab->lock, C.Aes, WC_BORDER, tp, mg, 1, 1, r);

			wind = create_window(	tab->lock,
						do_winmesag, //NULL,
						do_formwind_msg, //NULL,
						tab->client,
						cfg.menu_locking,	/* yields nolist if locking. */
						TOOLBAR|STORE_BACK,
						created_for_AES|created_for_POPUP,
						0,			/* no margin and no widgets: completely invisible. */
						1,1,
						r,
						NULL, NULL);
		}
	}
	else
		wind = NULL;

	if (wind)
	{
		k->popw = wind;
		k->drop = r;
		DIAG((D_menu, tab->client, "drop: %d/%d,%d/%d", r));
		set_popup_widget(tab, wind, item);
		if (!cfg.menu_locking)
		{
			DIAG((D_menu, tab->client, "nolocking pop window %lx + %d", obtree, item));
			open_window(tab->lock, wind, r);
		}
		else
		{
			DIAG((D_menu, tab->client, "locking pop window %lx + %d", obtree, item));
			open_window(tab->lock, wind, r);
		}
	}
	else
	{
		DIAGS(("display_popup: wt=%lx, obtree=%lx, item=%d", wt, wt->tree, item));

		obj_area(wt, item, &r);
		r = popup_inside(tab, r);
#if GENERATE_DIAGS
		if (!strnicmp(wt->owner->proc_name, "start", 5))
		{
			yield();
			yield();
			yield();
			yield();
		}
#endif
		hidem();
		form_save(k->border, k->drop, &(k->Mpreserve));
		DIAG((D_menu, tab->client, "pop draw %lx + %d", obtree, item));
		wr_mode(MD_TRANS);
		obtree->ob_x = k->rdx;
		obtree->ob_y = k->rdy;
		wt->is_menu = true;
		draw_object_tree(tab->lock, wt, obtree, item, 1, 5);
		showm();
	}
}

void
do_popup(Tab *tab, XA_TREE *wt, int item, TASK *click, short rdx, short rdy)
{
	OBJECT *root = wt->tree;
	MENU_TASK *k = &tab->task_data.menu;

	DIAG((D_menu, NULL, "do_popup: tab=%lx"));
	menu_spec(root,item);
	k->stage = IN_DESK;
	k->point_at_menu = -1;
	k->entry = click;

	display_popup(tab, wt, item, rdx, rdy);

	if (tab->nest)
	{
		TASK where_are_we;
		Tab *nx = tab->nest;
		MENU_TASK *kx = &nx->task_data.menu;
		
		kx->wt->tree->ob_x = kx->rdx;
		kx->wt->tree->ob_y = kx->rdy;
		menu_area(&k->em.m1, nx, kx->point_at_menu);
		k->em.flags |= MU_M1|1;	   		/* out of entry */
		k->em.t1 = where_are_we;
	}
	else
	{
		short x, y;

		k->em.m1 = k->drop;
		k->em.flags = MU_M1|0;			/* into popup */
		k->em.t1 = popup;

		check_mouse(wt->owner, NULL, &x, &y);
		if (is_rect(x, y, k->em.flags & 1, &k->em.m1))
		{
			k->em.flags = 0;
			k->x = x;
			k->y = y;
			k->em.t1(C.menu_base);
		}
	}
}

static void
click_desk_popup(struct task_administration_block *tab)
{
	MENU_TASK *k = &tab->task_data.menu;
	struct xa_client *client;
	int m;

	m = find_menu_object(tab, k->pop_item) - 1;

	IFDIAG(tab->dbg = 1;)
	popout(tab);

	client = NULL;

	if (m >= 0 && m < appmenusize)
		client = appmenu[m].client;

	if (client)
	{
		DIAG((D_menu, NULL, "got client %s", c_owner(client)));

		if (client == C.Aes)
		{
			app_in_front(tab->lock, client);
		}
		else
		{
			unhide_app(tab->lock, client);

			switch (client->type)
			{
			/* Accessory - send AC_OPEN */
			case APP_ACCESSORY:
			{
				DIAG((D_menu, NULL, "is an accessory"));
				/* found the reason some acc's wouldnt wake up: msgbuf[4] must receive
				 * the meu_register reply, which in our case is the pid.
				 */
				send_app_message(tab->lock, tab->wind, client, AMQ_NORM,
							AC_OPEN,        0, 0, 0,
							client->p->pid, 0, 0, 0);
				break;
			}
			/* Application, swap topped app */
			case APP_APPLICATION:
			{
				DIAG((D_menu, NULL, "is a real GEM client"));
				app_in_front(tab->lock, client);
				break;
			}
			}
		}
	}
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

/* This is called by popup menu's as well */
static void
popup(struct task_administration_block *tab)
{
	MENU_TASK *k = &tab->task_data.menu;
	int m;

	k->stage = IN_MENU;
/*	k->wt->ob_x = k->rdx;
	k->wt->ob_y = k->rdy;

	m = find_object(k->wt, k->pop_item, 2, k->x, k->y);
*/
	m = find_menu_object(tab, k->pop_item);

	if (m < 0)
	{
		if (k->popw)
		{
			k->em.flags = MU_MX;
			k->em.t1 = where_are_we;
		}
		else
		{
			k->stage = IN_DESK;		/* There is no specific rectangle to target */
			k->em.flags = MU_MX;		/* report any mouse movement */
			k->em.t1 = where_are_we;	/* This will sort it out. :-) */
		}
	}
	else if (m == k->pop_item)		/* failsafe  HR 010501: check for pop_item (there could be space between entries) */
	{
		k->em.flags = MU_MX;		/* report any mouse movement */
		k->em.t1 = where_are_we;	/* This will sort it out. :-) */
	}
	else
	{
		OBJECT *obtree = k->wt->tree;
		bool dis = obtree[m].ob_state & OS_DISABLED;
		XA_MENU_ATTACHMENT *at;

		DIAG((D_menu, NULL, "popup: tab=%lx, obj=%d, point_at_menu=%d",
			m, k->point_at_menu));

		if (m != k->point_at_menu)
		{
			if (   k->point_at_menu > -1
			    && !(obtree[k->point_at_menu].ob_state & OS_DISABLED))
			{
				change_entry(tab, 0);
			}
			k->point_at_menu = m;
		}

		if (!dis)
			change_entry(tab, 1);

		if (   !dis
		    && is_attach(menu_client(tab), k->wt, k->point_at_menu, &at)
		    && C.menu_nest < CASCADE-1)
		{
			RECT tra;
			short rdx,rdy;
			TASK *click;
			OBJECT *ob;
			Tab *new;
			XA_TREE *new_wt;

			DIAG((D_menu, NULL, "popup: is attach"));

			if (desk_menu(tab))
				click = click_desk_popup;
			else if (tab->ty == POP_UP)
				click = click_popup_entry;
			else
				click = click_menu_entry;

			new_wt = at->wt;
			ob = new_wt->tree;
			menu_area(&tra, tab, k->point_at_menu);

			DIAG((D_menu, NULL, "popup: attach=%lx, wt=%lx, obtree=%lx",
				at, new_wt, ob));

			ob->ob_x = 0, ob->ob_y = 0;
			obj_offset(new_wt, at->item, &rdx, &rdy);

			rdx = tra.x - rdx;
			rdy = tra.y - rdy;

			if (click == click_desk_popup)
				rdy += screen.c_max_h;
			else
				rdx += k->drop.w - 4;

			new = nest_menutask(tab);
			do_popup(new, new_wt, at->item, click, rdx, rdy);
		}
		else
		{
			menu_area(&k->em.m1, tab, m);
			k->em.flags = MU_M1|1;			/* out of entry */
			k->em.t1 = where_are_we;
		}
	}
}

Tab *
find_pop(Tab *tab, short x, short y)
{
	do {
		RECT r = tab->task_data.menu.drop;
		bool in = m_inside(x ,y , &r);
		if (in)
			break;
		tab = tab->nest;
	}
	while (tab);

	return tab;
}

static Tab *
any_pop(Tab *tab)
{
	MENU_TASK *k = &tab->task_data.menu;

	do {
		RECT r = tab->task_data.menu.drop;
		bool in = m_inside(k->x,k->y,&r);
		if (in)
			break;
		tab = tab->nest;
	}
	while (tab);

	return tab;
}

/*
 * This is installed in em.t1 and handles navigating (moving)
 * around an open menu
*/
static void
where_are_we(struct task_administration_block *tab)
{
	Tab *any = any_pop(tab);

	if (any && any == tab)
	{
		popup(tab);
	}
	else
	{
		MENU_TASK *k;

		if (any && any != tab)
		{
			DIAG((D_menu, NULL, "where_are_we: [2]collapse"));
			tab = collapse(tab, any);
		}

		k = &tab->task_data.menu;

		/*
		 * A menu bar? (and not just a popup?)
		 * and is mouse still on the bar?
		*/
		if (   barred(tab)
		    && m_inside(k->x,k->y,&k->bar))
		{
			new_title(tab);
		}
		/*
		 * Mouse somewhere on desktop? Must also be 'barred'
		 *
		*/
		else if (   cfg.menu_behave == LEAVE
		 	 && barred(tab)
		 	 && !tab->nest
		 	 && !any
		 	 && !(   k->x >= k->drop.x
		 	      && k->x <  k->drop.x + k->drop.w)) /* do not leave if mouse UNDER popup. */
		{
			DIAG((D_menu, NULL, "popout for leave"));
			IFDIAG(tab->dbg = 2;)
			popout(tab);			/* This will end the menu navigation */
		}
		else
		{
			popup(tab);
			
			//k->stage = IN_DESK;		/* There is no specific rectangle to target */
			//k->em.flags = MU_MX;		/* report any mouse movement */
			//k->em.t1 = where_are_we;	/* This will sort it out. :-) */
		}
	}
}

static void
new_title(struct task_administration_block *tab)
{
	if (tab->nest)
	{
		IFDIAG(tab->dbg = 3;)
		popout(tab);
	}
	else
	{
		MENU_TASK *k = &tab->task_data.menu;
		int title;
	
		k->stage = IN_TITLE;	

		title = find_menu_object(tab, k->titles);
		if (title == k->clicked_title)		/* no change */
		{
			k->em.flags = MU_M1|1;		/* fill out rect event data; out of title */
			obj_area(k->wt, k->clicked_title, &k->em.m1);
			k->em.t1 = where_are_we;
		}
		else
		{
			tab = menu_pop(tab);
			if (!menu_title(tab->lock, tab->wind, tab->widg, tab->locker))
			{
				/* No popup has been drawn. */
				IFDIAG(tab->dbg2 = 3;)
				menu_finish(NULL);
			}
		}
	}
}

static void
click_menu_entry(struct task_administration_block *tab)
{
	MENU_TASK *k= &tab->task_data.menu;

	DIAG((D_menu, NULL, "click_menu_entry: stage=%d",k->stage));
	if (k->stage != IN_MENU)
	{
		DIAG((D_menu, NULL, "[3]collapse"));
		tab = collapse(tab, NULL);
	}
	else
	{
		XA_TREE *wt = k->wt;
		OBJECT *obtree = wt->tree;
		int subm = (tab->nest != NULL);
		int m, d, titles;
		int about, kc, ks;

		m = find_menu_object(tab, 0);
		d = obtree[m].ob_state & OS_DISABLED;

		DIAG((D_menu, NULL, "[4]collapse"));
		tab = collapse(tab,NULL);
		k = &tab->task_data.menu;

		wt = k->wt;
		titles = k->titles,
		about = k->about,
		ks = k->pop_item;
		kc = k->clicked_title;

		DIAG((D_menu, NULL, "%sABLED,ti=%d,ab=%d,kc=%d,ks=%d,m=%d",d ? "DIS" : "EN",
			titles, about, kc, ks, m));

		if (m > -1 && !d)
		{	
			obtree[m].ob_state &= ~OS_SELECTED;	/* Deselect the menu entry */
			if (tab->wind != root_window)
			{
				OBJECT *rs = obtree;
				DIAG((D_menu, NULL, "indirect call"));
				tab->wind->send_message(tab->lock, tab->wind, wt->owner, AMQ_NORM,
							MN_SELECTED, 0, 0, kc,
							m, (long)rs >> 16, (long)rs, ks);
			}
			else if (subm || (!subm && (kc != obtree[titles].ob_head || m == about)))
			{
				/* If it's a client program menu, send it a little message to say hi.... */
				if (wt->owner != C.Aes)
				{
					OBJECT *rs = obtree;

					DIAG((D_menu, NULL, "Calling send_app_message()"));
					DIAG((D_menu, NULL, "to %s,title=%d,point_at=%d", t_owner(wt), kc, m));

					/* Note the AES4.0 extended message format...... */
					send_app_message(tab->lock, tab->wind, wt->owner, AMQ_NORM,
							 MN_SELECTED, 0, 0, kc,
							 m, (long)rs >> 16, (long)rs, ks);
				}
				else
				{
					/* Otherwise, process system menu clicks */
					DIAG((D_menu, NULL, "do_system_menu()"));
					do_system_menu(tab->lock, kc, m);
				}
			}
		}
	}
	IFDIAG(tab->dbg2 = 2;)
	/* Oef, a hole was left in the if symmetrie. */
	menu_finish(tab);
}

void
click_popup_entry(struct task_administration_block *tab)
{
	MENU_TASK *k = &tab->task_data.menu;
	AESPB *pb = tab->pb;
	MENU *md = (MENU*)pb->addrin[1];

	md->mn_tree = k->wt->tree;
	md->mn_scroll = 0;
	vq_key_s(C.vh, &md->mn_keystate);

	md->mn_item = find_menu_object(tab, k->pop_item);
	if (md->mn_item >= 0 && (k->wt->tree[md->mn_item].ob_state & OS_DISABLED) != 0)
		md->mn_item = -1;
	
	DIAG((D_menu, NULL, "click_popup_entry %lx + %d", md->mn_tree, md->mn_item));

	IFDIAG(tab->dbg = 6;)
	popout(tab);			/* incl. screen unlock */

	assert(pb);
	pb->intout[0] = md->mn_item < 0 ? 0 : 1;

	tab->client->usr_evnt = 1;
}

void
click_form_popup_entry(struct task_administration_block *tab)
{
	MENU_TASK *k = &tab->task_data.menu;
	AESPB *pb = tab->pb;
	int item;

	item = find_menu_object(tab, k->pop_item);
	if (item >= 0 && (k->wt->tree[item].ob_state & OS_DISABLED) != 0)
		item = -1;

	DIAG((D_menu, NULL, "click_form_popup_entry %lx + %d", k->wt->tree, item));

	IFDIAG(tab->dbg = 7;)
	popout(tab);			/* incl. screen unlock */

	assert(pb);
	pb->intout[0] = item;

	tab->client->usr_evnt = 1;
}

/*
 * Menu Tree Widget display
 */
/*
 * Ozk: We enter here being in either a thread of wind->owner, or wind->owner itself
 * The menu, however, can be owned by someone else .. we check that.
*/
static void
Display_menu_widg(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	XA_TREE *wt = widg->stuff;
	OBJECT *obtree = rp_2_ap(wind, widg, NULL);

	if (wind->nolist && (wind->dial & created_for_POPUP))
	{
		set_clip(&wind->wa);
		draw_object_tree(0, wt, NULL, widg->start, MAX_DEPTH, 6);
		clear_clip();
	}
	else
		draw_object_tree(0, wt, NULL, widg->start, MAX_DEPTH, 6);

	if (wt->menu_line)	/* HR 090501  menu in user window.*/
	{
		short titles;

		write_menu_line((RECT*)&obtree->ob_x);	/* HR: not in standard menu's object tree */

		/* HR: Use the AES's client structure to register the rectangle for the current menu bar. */
		titles = obtree[obtree[0].ob_head].ob_head;
		C.Aes->waiting_for = XAWAIT_MENU;
		obj_area(wt, titles, &C.Aes->em.m1);
		C.Aes->em.flags = MU_M1;	/* into menu bar */
	}
}

static void
CE_display_menu_widg(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
		Display_menu_widg(lock, ce->ptr1, ce->ptr2);
}
	
static bool
display_menu_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_client *rc = lookup_extension(NULL, XAAES_MAGIC);
	XA_TREE *wt = widg->stuff;

	DIAG((D_menu,wt->owner,"display_menu_widget on %d for %s%s (%lx)",
		wind->handle, t_owner(wt), wt->menu_line ? "; with menu_line" : "", rc));

	if (!rc)
		rc = C.Aes;

	if (wt->owner == rc || wt->owner == C.Aes)
	{
		DIAG((D_menu, wt->owner, "normal display_menu_widget, wt->owner %s, rc %s",
			wt->owner->name, rc->name));
		Display_menu_widg(lock, wind, widg);
	}
	else
	{
		DIAG((D_menu, wt->owner, "posted display_menu_widget for %s by %s", wt->owner->name, rc->name));
		post_cevent(wt->owner,
			    CE_display_menu_widg,
			    wind,
			    widg,
			    0, 0,
			    NULL,
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
click_menu_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *md)
{
	struct xa_client *client, *rc = lookup_extension(NULL, XAAES_MAGIC);
	//bool l = false;

	DIAG((D_menu, NULL, "click_menu_widget"));

	client = ((XA_TREE *)widg->stuff)->owner;

	/*
	 * Make sure we're in the right context
	*/
#if 0
	if (!rc)
		rc = C.Aes;

	if (client != rc)
		return false;

	if ( widg->stuff == get_menu())
	{
		if ( !lock_menustruct(client, false) )
			return false;
	}
#endif
	if (!menu_title(lock, wind, widg, client->p->pid))
	{
		menu_finish(NULL);
	}

	return false;
}

/* Changed! */
static bool
menu_title(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, int locker)
{
	RECT r;
	Tab *tab;
	MENU_TASK *k;
	XA_TREE *wt;
	OBJECT *obtree;
	short item = 0;
	short f, n;

	DIAG((D_menu, NULL, "menu_title"));
	tab = new_task(C.active_menu);
	k = &tab->task_data.menu;
	tab->lock = lock;
	tab->locker = locker;
	tab->wind = wind;
	tab->widg = widg;
	wt = widg->stuff;
	tab->client = wt->owner;

	/* Convert relative coords and window location to absolute screen location */
	rp_2_ap(wind, widg, &r);

	/* first entry into the menu_bar; initialize task */
	k->wt = wt;
	obtree = wt->tree;
	k->titles = obtree[obtree[0].ob_head].ob_head;
	k->menus  = obtree[0].ob_tail;
	k->about  = obtree[obtree[k->menus].ob_head].ob_head;

	check_mouse(wind->owner, &f, &k->x, &k->y);

	k->rdx = r.x;
	k->rdy = r.y;
	k->clicked_title = find_menu_object(tab, k->titles);
	k->point_at_menu = -1;

	if (   k->clicked_title > -1
	    && obtree[k->clicked_title].ob_type == G_TITLE
	    && !(obtree[k->clicked_title].ob_state & OS_DISABLED))
	{
		C.menu_base = tab;			/* OK, can do */
		C.menu_nest = 0;
		tab->ty = k->ty = (wind == root_window ? ROOT_MENU : MENU_BAR);
		k->stage = IN_TITLE;
		obj_area(wt, k->titles, &k->bar);
		change_title(tab, 1);
		obtree[k->menus].ob_flags &= ~OF_HIDETREE;
		n = obtree[k->menus].ob_head;
		for (f = obtree[k->titles].ob_head; f != k->titles; f = obtree[f].ob_next)
		{
			if (f == k->clicked_title)
				item = n;
			else
				obtree[n].ob_flags |= OF_HIDETREE;
				
			n = obtree[n].ob_next;
		}

		obtree[item].ob_flags &= ~OF_HIDETREE;	/* Show the actual menu */
		k->entry = click_menu_entry;		/* obeyed by XA_MOUSE.C */

		if (desk_menu(tab) && C.menu_nest < CASCADE-1)
		{
			desk_popup.menu.mn_tree = built_desk_popup(tab->lock, 24,24);
			desk_popup.menu.mn_menu = 0;
			desk_popup.menu.mn_item = 0;
			desk_popup.menu.mn_scroll = 0;
			desk_popup.menu.mn_keystate = 0;

			desk_popup.wt = &desk_wt;
			desk_wt.tree = desk_popup.menu.mn_tree;
			desk_wt.owner = C.Aes;
			
			attach_menu(tab->lock, C.Aes, wt, k->about + 2, &desk_popup);
		}
		display_popup(tab, wt, item, r.x, r.y);

		k->em.flags = MU_M1|1;		/* fill out rect event data; out of title */
		obj_area(wt, k->clicked_title, &k->em.m1);
		k->em.t1 = where_are_we;
		return true;
	}
	else
	{
		C.menu_base = 0;
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
set_menu_widget(struct xa_window *wind, XA_TREE *menu)
{
	XA_WIDGET *widg = get_widget(wind, XAW_MENU);
	OBJECT *obtree = menu->tree;

	DIAG((D_widg, wind->owner, "set_menu_widget on %d for %s",
		wind->handle, w_owner(wind)));

	if (widg->stuff)
		((XA_TREE *)widg->stuff)->widg = NULL;


	menu->is_menu   = true;				/* set the flags in the original */
	menu->menu_line = (wind == root_window);	/* menu in root window.*/
	menu->widg = widg;

	/* additional fix to fit in window */
	obtree->ob_width = obtree[obtree->ob_head].ob_width = obtree[obtree->ob_tail].ob_width = wind->wa.w;

	widg->display = display_menu_widget;
	widg->click = click_menu_widget;
	widg->dclick = NULL;
	widg->drag = NULL /* drag_menu_widget */;
	widg->state = OS_NORMAL;
	widg->stuff = menu;
	widg->stufftype = STUFF_IS_WT;
	widg->destruct = free_xawidget_resources;
	widg->start = 0;

	wind->tool = widg;
	wind->active_widgets |= XaMENU;

	widg->loc.statusmask = XAWS_SHADED;

}

/*
 * Attach a popup to a window.
 */
static XA_TREE *
set_popup_widget(Tab *tab, struct xa_window *wind, int obj)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = k->wt;
	XA_WIDGET *widg = get_widget(wind, XAW_MENU);
	XA_WIDGET_LOCATION loc;
	//OBJECT *ob = wt->tree + obj;
	DisplayWidget display_object_widget;
	int frame = wind->frame;

	if ( widg->stuff)
		((XA_TREE *)widg->stuff)->widg = NULL;

	wt->is_menu = true;
	wt->widg = widg;

	if (frame < 0)
		frame = 0;

	loc.relative_type = LT;

/* HR The whole idee is: what to put in relative location to get the object I want too see (item) on the place I
      have chosen (window work area) The popup may take any place inside a root object. */
/* Derivation:
              (window border compensation  )   - (target       root displacement).
	loc.r.x = (wind->wa.x - wind->r.x - frame) - (wind->wa.x - k->rdx);
	loc.r.y = (wind->wa.y - wind->r.y - frame) - (wind->wa.y - k->rdy);
*/
	loc.r.x = k->rdx - wind->r.x - frame;
	loc.r.y = k->rdy - wind->r.y - frame;
	
	DIAG((D_menu, tab->client, "loc %d/%d", loc.r.x, loc.r.y));

	loc.r.w = wt->tree->ob_width;
	loc.r.h = wt->tree->ob_height;
	loc.mask = XaMENU;

	widg->type = XAW_MENU;
 	widg->display = display_menu_widget;
#if 0
	/* handled by other means (Task administration Tab) */
	widg->click = click_popup_widget;
#endif
	widg->r = loc.r;
	widg->loc = loc;
	widg->state = OS_NORMAL;
	widg->stuff = wt;
	widg->stufftype = STUFF_IS_WT;
	widg->destruct = free_xawidget_resources;
	widg->start = obj;
//	if (tab->nest)
		/* HR testing: Use the window borders. */
		wt->zen = true;

	wind->tool = widg;

	XA_slider(wind, XAW_HSLIDE, wt->tree[obj].ob_width, wind->wa.w, 0);
	XA_slider(wind, XAW_VSLIDE, wt->tree[obj].ob_height, wind->wa.h, 0);

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
fix_menu(struct xa_client *client, OBJECT *root, bool do_desk)
{
	int titles, menus, tbar, s_ob, t_ob;

	DIAG((D_menu, NULL, "fixing menu 0x%lx", root));

	tbar = root[0].ob_head;
	titles = root[tbar].ob_head;
	menus = root[0].ob_tail;

	root->ob_width = root[tbar].ob_width = root[menus].ob_width = screen.r.w;
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
		t_ob = root[t_ob].ob_next;
		s_ob = root[s_ob].ob_next;
	}

	/* fix desk menu */
	if (do_desk)
	{
		s_ob = root[menus].ob_head;
		t_ob = root[s_ob ].ob_head;
		root[s_ob].ob_height = root[t_ob+3].ob_y - root[s_ob].ob_y;
		t_ob += 2;

		/* client->mnu_clientlistname is umalloced area */
		root[t_ob++].ob_spec.free_string = client->mnu_clientlistname;
		DIAG((D_menu, NULL, "Wrong menufix for %s - adding object at %lx",
			client->name, client->mnu_clientlistname));

		while (t_ob != s_ob)
		{
			root[t_ob].ob_flags |= OF_HIDETREE|OS_DISABLED;
			t_ob = root[t_ob].ob_next;
		}
	}

	DIAG((D_menu, NULL, "done fix_menu()"));
}
