/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for MiNT
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
#include "objects.h"
#include "rectlist.h"
#include "taskman.h"
#include "widgets.h"
#include "xalloc.h"

#include "xa_rsrc.h"
#include "xa_form.h"


static void popup(struct task_administration_block *tab);
static void where_are_we(struct task_administration_block *tab);
static void click_menu_entry(struct task_administration_block *tab);
static void new_title(struct task_administration_block *tab);

static MENU desk_popup;
static bool menu_title(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, int locker);
static XA_TREE *set_popup_widget(Tab *tab, struct xa_window *wind, OBJECT *form, int item);


static bool
barred(Tab *tab)
{
	return tab->ty == MENU_BAR || tab->ty == ROOT_MENU;
}

static unsigned long
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
menu_spec(OBJECT *root, int item)
{
	if (is_spec(root,item))
	{
		set_ob_spec(root,item, 0xff0000L | menu_colors());
		root[item].ob_state |= OS_SHADOWED;
	}
}

static void
change_title(Tab *tab, int state)
{
	MENU_TASK *k = &tab->task_data.menu;
	OBJECT *ob = k->root;
	int t = k->clicked_title;

	if (state)
		state = ob[t].ob_state | OS_SELECTED;
	else
		state = ob[t].ob_state & ~OS_SELECTED;
	ob->ob_x = k->rdx;
	ob->ob_y = k->rdy;
	change_object(	tab->lock,
			tab->widg->stuff,	/* HR 090501 menu in user window. */
			ob, t, NULL, state, true);
}

static void
change_entry(Tab *tab, int state)
{
	MENU_TASK *k = &tab->task_data.menu;
	OBJECT *ob = k->root;
	int t = k->point_at_menu;

	if (state)
		state = ob[t].ob_state | OS_SELECTED;
	else
		state = ob[t].ob_state & ~OS_SELECTED;
	ob->ob_x = k->rdx;
	ob->ob_y = k->rdy;
	if (k->popw)
	{
		XA_TREE *wt = get_widget(k->popw, XAW_MENU)->stuff;
		RECT wa = k->popw->wa;

		change_object(	tab->lock,
				wt,		/* HR 090501: menu in user window. 300801: repaired inheritance. */
				ob, t,
				&wa,		/* HR 191101: clip the work area */
				state, true);
	}
	else if (tab->widg)
		change_object(tab->lock, tab->widg->stuff, ob, t, NULL, state, true);
	else
		change_object(tab->lock, NULL,              ob, t, NULL, state, true);
}

static bool
is_attach(struct xa_client *client, OBJECT *menu, int item, XA_MENU_ATTACHMENT **pat)
{
	XA_MENU_ATTACHMENT *at = client->attach;
	OBJECT *attach_to = menu + item;
	int i;

	if (pat)
		*pat = NULL;

	if (at == NULL || (attach_to->ob_flags&OF_SUBMENU) == 0)
		return false;

	DIAG((D_menu,client, "is_attach: at=%lx,flags=%x,type=%x, spec=%lx",
	       at,attach_to->ob_flags,attach_to->ob_type,get_ob_spec(attach_to)->index));

	for (i = 0; i < ATTACH_MAX; i++)
	{
		if (   menu == at[i].to_tree
		    && item == at[i].to_item)
		{
			if (pat)
				*pat = at + i;
			return true;
		}
	}
	return false;
}

/*
 * return information about a attached submenu.
 */
int
inquire_menu(enum locks lock, struct xa_client *client, OBJECT *tree, int item, MENU *mn)
{
	XA_MENU_ATTACHMENT *at;
	int ret = 0;

	Sema_Up(clients);

	DIAG((D_menu,NULL,"inquire_menu for %s on %lx + %d",
		c_owner(client), tree, item));

	/* You can only attach submenu's to normal menu entries */
	if (tree)
	{
		if (is_attach(client, tree, item, &at))
		{
			mn->mn_tree = at->tree;
			mn->mn_item = at->item;
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
attach_menu(enum locks lock, struct xa_client *client, OBJECT *tree, int item, MENU *mn)
{
	int ret = 0;
	XA_MENU_ATTACHMENT *at = client->attach;
	OBJECT *attach_to;

	Sema_Up(clients);

	attach_to = tree + item;

	DIAG((D_menu,NULL,"attach_menu for %s %lx + %d to %lx + %d",
		c_owner(client), mn->mn_tree, mn->mn_menu, tree, item));

	/* You can only attach submenu's to normal menu entries */
	if (tree && mn && mn->mn_tree)
	{
		/* Allocate a table for at least ATTACH_MAX attachments */
		if (!at)
		{
			at = xcalloc(ATTACH_MAX,sizeof(XA_MENU_ATTACHMENT),102);
			if (!at)
			{
				Sema_Dn(clients);
				return 0;
			}
		}

		client->attach = at;

		while(at)
		{
			/* find a free place in the table */
			if (at->to_tree)
				at++;
			else
				break;
		}

		/* A menu is attached by replacing ob_spec (the text string)
		 * to point to a structure in the above allocated table.
		 * The text string pointer is moved to the very first location of that
		 * structure. The remainder being additional information.
		 */

		/* room left in table ? */
		if (at < at + ATTACH_MAX)
		{
			char *text;
			/* OK now we can attach the menu */
			menu_spec(mn->mn_tree, mn->mn_menu);
			attach_to->ob_flags |= OF_SUBMENU;
			at->to_tree = tree;
			at->to_item = item;
			at->item = mn->mn_menu;	 /* This is the submenu */
			at->tree = mn->mn_tree;
			if ((attach_to->ob_type&0xff) == G_STRING)
			{
				text = get_ob_spec(attach_to)->free_string;
				text[strlen(text)-1] = mn == &desk_popup ? '\2' : '>';
			}
			ret = 1;
		}
	}
	else
	{
		DIAG((D_menu,NULL,"attach no tree(s)"));
	}

	Sema_Dn(clients);
	return ret;
}

int
detach_menu(enum locks lock, struct xa_client *client, OBJECT *tree, int item)
{
	OBJECT *attach_to = tree + item;
	XA_MENU_ATTACHMENT *xt;
	int ret = 0;

	Sema_Up(clients);

	if (is_attach(client, tree, item, &xt))
	{
		char *text;

		DIAG((D_menu,NULL,"detach_menu %lx + %d for %s %lx + %d",
			xt->tree, xt->item, client->name, tree, item));

		attach_to->ob_flags &= ~OF_SUBMENU;
		xt->to_tree = NULL;
		xt->to_item = 0;
		if ((attach_to->ob_type & 0xff) == G_STRING)
		{
			text = get_ob_spec(attach_to)->free_string;
			text[strlen(text)-1] = ' ';
		}
		ret = 1;
	}

	Sema_Dn(clients);
	return ret;
}

void
remove_attachments(enum locks lock, struct xa_client *client, OBJECT *menu)
{
	if (menu)
	{
		int f = 0;

		DIAG((D_menu,NULL, "remove_attachments"));
		do {
			detach_menu(lock, client, menu, f);
		}
		while (!(menu[f++].ob_flags & OF_LASTOB));	
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


#define MAX_MREG 32

static OBJECT menu_reg[MAX_MREG];
static char menu_regt[MAX_MREG][130];
static struct xa_client *menu_regcl[MAX_MREG];

static OBJECT drop_box =
{
	-1, 1, 2,		/* Object 0  */
	G_BOX, OF_NONE, OS_SHADOWED,
	{ 0x00FF1100L },
	0, 0, 12, 2
};

static OBJECT drop_choice =	/* Object 1 to n */
{
	0,-1,-1,
	G_STRING, OF_NONE, OS_NORMAL,
	{ (long)"-" },
	0, 1, 12, 1
};

static OBJECT *
built_desk_popup(enum locks lock, short x, short y)
{
	int n = 0, i = 0, xw = 0, obs, split = 0;
	OBJECT *ob;
	struct xa_client *client;

	/* go throug all the clients, store pid and name */
	/* count in n */

	Sema_Up(clients);

	client = S.client_list;
	while (client)
	{
		if (client->type == APP_ACCESSORY || client == C.Aes)
		{
			menu_regcl[n] = client;
			sprintf(menu_regt[n], sizeof(menu_regt[n]), "  %d->%d %s",
				client->p->ppid,
				client->p->pid,
				*client->name 
					? client->name + 2
					: client->proc_name);
			n++;
		}
		client = client->next;
	}

	menu_regcl[n] = NULL;
	strcpy(menu_regt[n],"-");
	split = n++;

	client = S.client_list;
	while (client)
	{
		if (client->type != APP_ACCESSORY)
		{
			menu_regcl[n] = client;
			sprintf(menu_regt[n], sizeof(menu_regt[n]), "  %d->%d %s",
				client->p->ppid,
				client->p->pid,
				*client->name 
					? client->name + 2
					  /* You can always do a menu_register in multitasking AES's  :-) */
					: client->proc_name);
			n++;
		}
		client = client->next;
	}

	if (split == n-1)		/* dont want to see split when last */
		n--;

	Sema_Dn(clients);

	obs = n+1;
	ob = menu_reg;

	ob[0] = drop_box;
	ob[0].ob_tail = obs-1;
	ob[0].ob_height = obs-1;
	
	for (i=0; i<n; i++)		/* fix the tree */
	{
		int m;
		int j = i+(obs-n);

		ob[j] = drop_choice;
		if (i == split)
			ob[j].ob_state = OS_DISABLED;
		else
		{
			char *txt = menu_regt[i];

			if (menu_regcl[i] == focus_owner())
				ob[j].ob_state |= OS_CHECKED;
			else
				ob[j].ob_state &= ~OS_CHECKED;

			if (any_hidden(lock, menu_regcl[i]))
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
	for (i=0; i<obs; i++)
		ob[i].ob_width = xw;

	if (split < n)
	{
		memset(menu_regt[split], '-', xw);
		menu_regt[split][xw]=0;
	}

	for (i=0; i<obs; i++)
		obfix(ob,i);

	ob[0].ob_x = x;
	ob[0].ob_y = y;

	menu_spec(ob, 0);

	return menu_reg;
}

static bool
desk_menu(Tab *tab)
{
	MENU_TASK *k = &tab->task_data.menu;

	return    tab->ty == ROOT_MENU
	       && tab->nest == NULL
	       && k->clicked_title == k->root[k->titles].ob_head;
}

static Tab *
menu_pop(Tab *tab)
{
	struct xa_window *w;
	MENU_TASK *b,*k = &tab->task_data.menu;

	if (barred(tab))
	{
		if (!tab->nest)
		{
			if (k->clicked_title > 0)
				change_title(tab, 0);
			k->root[k->pop_item].ob_flags |= OF_HIDETREE;
		}
		if (desk_menu(tab))
			/* remove attached clients popup */
			detach_menu(tab->lock, C.Aes, k->root, k->about + 2);
	}
	w = k->popw;
	if (w)		/* Windowed popup's */
	{	
		close_window(tab->lock, w);
		if (cfg.menu_locking)
		{
			hidem();
			form_restore(k->border, k->drop, k->Mpreserve);
			showm();
		}
		delete_window(tab->lock, w);
		k->popw = NULL;
	}
	else
	{
		hidem();
		form_restore(k->border, k->drop, k->Mpreserve);
		showm();
	}

	k->root[k->point_at_menu].ob_state&=~OS_SELECTED;

	if (tab->nest)
	{
		C.menu_base = tab->nest;
		C.menu_nest--;
		b = &C.menu_base->task_data.menu;
		b->x = k->x;		/* pass back current x & y */
		b->y = k->y;
		tab = C.menu_base;
	}
	return tab;
}

static void
menu_finish(struct task_administration_block *tab)
{
	struct xa_client *client;
	bool is_bar;
	
	/* after !menu_title */
	if (tab)
	{
		MENU_TASK *k = &tab->task_data.menu;

		is_bar = barred(tab);
		client = pid2client(tab->locker);

		DIAG((D_menu, NULL, "[%d]menu_finish, ty:%d", tab->dbg2, tab->ty));

		if (is_bar && !tab->nest)
			k->root[k->menus].ob_flags |= OF_HIDETREE;
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

	/* Ozk: Only unlock screen here if locker is XaAES kernel itself! */
	if (cfg.menu_locking && update_locked() == C.Aes)
	{
		if (is_bar && client)
			unlock_mouse(client, 10);

		unlock_screen(C.Aes, 10);
	}
}

static Tab *
collapse(Tab *tab,Tab *upto)
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

static void
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
	if (r.x < o.x || r.w > o.w) r.x = o.x;
	if (r.y < o.y || r.h > o.h) r.y = o.y;

	if (r.w < o.w && r.h < o.h)
	{
		if (r.x + r.w > o.x + o.w) r.x = o.x + o.w - r.w;
		if (r.y + r.h > o.y + o.h) r.y = o.y + o.h - r.h;
	}
	return r;
}

static RECT
popup_inside(Tab *tab, RECT r)
{
	MENU_TASK *k = &tab->task_data.menu;
	RECT ir = tab->ty == ROOT_MENU ? screen.r : root_window->wa,
	     rc = rc_inside(r, ir);

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

	k->rdx = k->root->ob_x - (r.x - rc.x);
	k->rdy = k->root->ob_y - (r.y - rc.y);

	k->drop = rc;

	return rc;
}

static int
find_menu_object(Tab *tab, int start)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt;
	short dx=0, dy=0;

	if (k->popw)
	{
		wt = get_widget(k->popw, XAW_MENU)->stuff;
		dx = wt->dx;
		dy = wt->dy;
	}

	k->root->ob_x = k->rdx;
	k->root->ob_y = k->rdy;

	return find_object(k->root, start, MAX_DEPTH, k->x, k->y, dx, dy);
}


static void
menu_area(RECT *c, Tab *tab, int item)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt;
	short dx=0, dy=0;

	if (k->popw)
	{
		wt = get_widget(k->popw, XAW_MENU)->stuff;
		dx = wt->dx;
		dy = wt->dy;
	}

	object_area(c, k->root, item, dx, dy);
}

static void
display_popup(Tab *tab, OBJECT *root, int item, short rdx, short rdy)
{
	MENU_TASK *k = &tab->task_data.menu;
	struct xa_window *wind;
	RECT r;
	int wash;

	k->pop_item = item;
	k->border = 0;
	k->root = root;
	root->ob_x = rdx; /* This is where we want to draw the popup object. */
	root->ob_y = rdy;

	DIAG((D_menu, tab->client, "display_popup: rdx/y %d/%d", rdx, rdy));
	object_rectangle(&r, root, item, 0, 0);
	r = popup_inside(tab, r);
	wash = r.h;

	if (tab->scroll && r.h > 8*screen.c_max_h)
		r.h = 8*screen.c_max_h;

	if (r.y + r.h > root_window->wa.y + root_window->wa.h)
		r.h = root_window->wa.h - r.y; 

	if (cfg.popscroll && r.h > cfg.popscroll*screen.c_max_h)
		r.h = cfg.popscroll*screen.c_max_h;

	if (!cfg.menu_locking || r.h < wash)
	{
		if (r.h < wash)
		{
			int mg = MONO ? 2 : 1;
			XA_WIND_ATTR tp = TOOLBAR|VSLIDE|UPARROW|DNARROW;

			tab->scroll = true;

			r = calc_window(tab->lock, C.Aes, WC_BORDER, tp, mg, 1, 1, r);

			wind = create_window(	tab->lock,
						handle_form_window,
						tab->client,
						cfg.menu_locking,	/* yields nolist if locking. */
						tp,
						created_for_AES|created_for_POPUP,
						mg,1,1,
						r,
						&r, NULL);
		}
		else
		{
			object_area(&r, root, item, 0, 0);
			r = popup_inside(tab, r);

			wind = create_window(	tab->lock,
						NULL,
						tab->client,
						cfg.menu_locking,	/* yields nolist if locking. */
						TOOLBAR,
						created_for_AES|created_for_POPUP,
						-1,			/* no margin and no widgets: completely invisible. */
						0,0,
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
		set_popup_widget(tab, wind, root, item);
		if (!cfg.menu_locking)
		{
			DIAG((D_menu, tab->client, "nolocking pop window %lx + %d", root, item));
			open_window(tab->lock, wind, r);
		}
		else
		{
			hidem();
			k->Mpreserve = form_save(k->border, k->drop, NULL);
			DIAG((D_menu, tab->client, "locking pop window %lx + %d", root, item));
			wind->is_open = true;
			display_window(tab->lock, 71, wind, NULL);
			showm();
		}
	}
	else
	{
		XA_TREE wt = nil_tree;
		wt.owner = NULL;
		object_area(&r, root, item, 0, 0);
		r = popup_inside(tab, r);
		hidem();
		k->Mpreserve = form_save(k->border, k->drop, NULL);
		DIAG((D_menu, tab->client, "pop draw %lx + %d", root, item));
		wr_mode(MD_TRANS);
		root->ob_x = k->rdx;
		root->ob_y = k->rdy;
		wt.is_menu = true;
		draw_object_tree(tab->lock, &wt, root, item, 1, 5);
		showm();
	}
}

void
do_popup(Tab *tab, OBJECT *root, int item, TASK *click, short rdx, short rdy)
{
	MENU_TASK *k = &tab->task_data.menu;

	DIAG((D_menu, NULL, "do_popup"));
	menu_spec(root,item);
	k->stage = IN_DESK;
	k->point_at_menu = -1;
	k->entry = click;

	display_popup(tab, root, item, rdx, rdy);

	if (tab->nest)
	{
		TASK where_are_we;
		Tab *nx = tab->nest;
		MENU_TASK *kx = &nx->task_data.menu;

		kx->root->ob_x = kx->rdx;
		kx->root->ob_y = kx->rdy;
		menu_area(&k->em.m1, nx, kx->point_at_menu);
		k->em.flags |= MU_M1|1;	   		/* out of entry */
		k->em.t1 = where_are_we;
	}
	else
	{
		k->em.m1 = k->drop;
		k->em.flags = MU_M1|0;			/* into popup */
		k->em.t1 = popup;
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

	client = menu_regcl[m];
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
				DIAG((D_menu, NULL, "is an accessory"));
				/* found the reason some acc's wouldnt wake up: msgbuf[4] must receive
				 * the meu_register reply, which in our case is the pid.
				 */
				send_app_message(tab->lock, tab->wind, client,
							AC_OPEN,        0, 0, 0,
							client->p->pid, 0, 0, 0);
				break;

			/* Application, swap topped app */
			case APP_APPLICATION:
				DIAG((D_menu, NULL, "is a real GEM client"));
				app_in_front(tab->lock, client);
				break;
			}
		}
	}
}

static struct xa_client *
menu_client(Tab *tab)
{
	if (desk_menu(tab))
		return C.Aes;
	else if (tab->widg)
	{
		XA_TREE *wt;
		wt = tab->widg->stuff;
		return wt->owner;
	}
	else
		return tab->client;
}

/* This is called by popup menu's as well */
static void
popup(struct task_administration_block *tab)
{
	MENU_TASK *k = &tab->task_data.menu;
	int m;

	k->stage = IN_MENU;
/*	k->root->ob_x = k->rdx;
	k->root->ob_y = k->rdy;

	m = find_object(k->root, k->pop_item, 2, k->x, k->y);
*/
	m = find_menu_object(tab, k->pop_item);
	if (m < 0)
	{
		k->stage = IN_DESK;		/* There is no specific rectangle to target */
		k->em.flags = MU_MX;		/* report any mouse movement */
		k->em.t1 = where_are_we;	/* This will sort it out. :-) */
	}
	else if (m == k->pop_item)		/* failsafe  HR 010501: check for pop_item (there could be space between entries) */
	{
		k->em.flags = MU_MX;		/* report any mouse movement */
		k->em.t1 = where_are_we;	/* This will sort it out. :-) */
	}
	else
	{
		bool dis = k->root[m].ob_state&OS_DISABLED;
		XA_MENU_ATTACHMENT *at;

		if (m != k->point_at_menu)
		{
			if (   k->point_at_menu > -1
			    && !(k->root[k->point_at_menu].ob_state & OS_DISABLED))
			{
				change_entry(tab, 0);
			}
			k->point_at_menu = m;
		}
		if (!dis)
			change_entry(tab, 1);

		if (   !dis
		    && is_attach(menu_client(tab), k->root, k->point_at_menu, &at)
		    && C.menu_nest < CASCADE-1)
		{
			RECT tra;
			short rdx,rdy;
			TASK *click;
			OBJECT *ob;
			Tab *new;

			if (desk_menu(tab))
				click = click_desk_popup;
			else if (tab->ty == POP_UP)
				click = click_popup_entry;
			else
				click = click_menu_entry;

			ob = at->tree;
			menu_area(&tra, tab, k->point_at_menu);
			ob->ob_x = 0, ob->ob_y = 0;
			object_offset(ob, at->item, 0, 0, &rdx, &rdy);

			rdx = tra.x - rdx;
			rdy = tra.y - rdy;

			if (click == click_desk_popup)
				rdy += screen.c_max_h;
			else
				rdx += k->drop.w - 4;

			new = nest_menutask(tab);
			do_popup(new, ob, at->item, click, rdx, rdy);
		}
		else
		{
			menu_area(&k->em.m1, tab, m);
			k->em.flags = MU_M1|1;			/* out of entry */
			k->em.t1 = where_are_we;
		}
	}
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

static void
where_are_we(struct task_administration_block *tab)
{
	MENU_TASK *k;
	Tab *any = any_pop(tab);

	if (any && any == tab)
	{
		popup(tab);
	}
	else
	{
		if (any && any != tab)
		{
			DIAG((D_menu, NULL, "[2]collapse"));
			tab = collapse(tab, any);
			k = &tab->task_data.menu;
		}

		k = &tab->task_data.menu;

		if (   barred(tab)
		    && m_inside(k->x,k->y,&k->bar))
		{
			new_title(tab);
		}
		/* in desktop ? */
		else if (   cfg.menu_behave == LEAVE
		 	 && barred(tab)
		 	 && !tab->nest
		 	 && !any
		 	 && !(   k->x >= k->drop.x
		 	      && k->x <  k->drop.x + k->drop.w)) /* do not leave if mouse UNDER popup. */
		{
			DIAG((D_menu, NULL, "popout for leave"));
			IFDIAG(tab->dbg = 2;)
			popout(tab);
		}
		else
		{
			k->stage = IN_DESK;		/* There is no specific rectangle to target */
			k->em.flags = MU_MX;		/* report any mouse movement */
			k->em.t1 = where_are_we;	/* This will sort it out. :-) */
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
		int title;
		MENU_TASK *k = &tab->task_data.menu;
	
		k->stage = IN_TITLE;	
		title = find_menu_object(tab, k->titles);
		if (title == k->clicked_title)		/* no change */
		{
			k->em.flags = MU_M1|1;		/* fill out rect event data; out of title */
			object_area(&k->em.m1, k->root, k->clicked_title, 0, 0);
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
		OBJECT *root = k->root;
		XA_TREE *wt;
		int m,d,titles,subm = tab->nest != NULL,
			  about,
			  kc,
			  ks;

		m = find_menu_object(tab, 0);
		d = root[m].ob_state & OS_DISABLED;

		DIAG((D_menu, NULL, "[4]collapse"));
		tab = collapse(tab,NULL);
		k = &tab->task_data.menu;

		wt = tab->widg->stuff;
		titles = k->titles,
		about = k->about,
		ks = k->pop_item;
		kc = k->clicked_title;

		DIAG((D_menu, NULL, "%sABLED,ti=%d,ab=%d,kc=%d,ks=%d,m=%d",d ? "DIS" : "EN",
			titles,about,kc,ks,m));

		if (m > -1 && !d)
		{	
			root[m].ob_state &= ~OS_SELECTED;	/* Deselect the menu entry */
			if (tab->wind != root_window)
			{
				OBJECT *rs = root;
				DIAG((D_menu,NULL,"indirect call"));
				tab->wind->send_message(tab->lock, tab->wind, wt->owner,
								MN_SELECTED, 0, 0, kc,
								m, (long)rs >> 16, (long)rs, ks);
			}
			else if (subm || (!subm && (kc != root[titles].ob_head || m == about)))
			{
				/* If it's a client program menu, send it a little message to say hi.... */
				if (wt->owner != C.Aes)
				{
					OBJECT *rs = root;

					DIAG((D_menu, NULL, "Calling send_app_message()"));
					DIAG((D_menu, NULL, "to %s,title=%d,point_at=%d", t_owner(wt), kc, m));

					/* Note the AES4.0 extended message format...... */
					send_app_message(tab->lock, tab->wind, wt->owner,
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

	md->mn_tree = k->root;
	md->mn_scroll = 0;
	vq_key_s(C.vh, &md->mn_keystate);

	md->mn_item = find_menu_object(tab, k->pop_item);
	if (md->mn_item >= 0 && (k->root[md->mn_item].ob_state & OS_DISABLED) != 0)
		md->mn_item = -1;
	
	DIAG((D_menu, NULL, "click_popup_entry %lx + %d", md->mn_tree, md->mn_item));

	IFDIAG(tab->dbg = 6;)
	popout(tab);			/* incl. screen unlock */

	pb->intout[0] = md->mn_item < 0 ? 0 : 1;
	Unblock(tab->client, XA_OK, 102);
}

void
click_form_popup_entry(struct task_administration_block *tab)
{
	MENU_TASK *k = &tab->task_data.menu;
	AESPB *pb = tab->pb;
	int item;

	item = find_menu_object(tab, k->pop_item);
	if (item >= 0 && (k->root[item].ob_state & OS_DISABLED) != 0)
		item = -1;

	DIAG((D_menu, NULL, "click_form_popup_entry %lx + %d", k->root, item));

	IFDIAG(tab->dbg = 7;)
	popout(tab);			/* incl. screen unlock */

	pb->intout[0] = item;
	Unblock(tab->client, XA_OK, 2);
}

XA_TREE *
get_menu(void)
{
	return root_window->widgets[XAW_MENU].stuff;
}

struct xa_client *
menu_owner(void)
{
	return get_menu()->owner;
}

/*
 * Menu Tree Widget display
 */
static bool
display_menu_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	XA_TREE *wt = widg->stuff;
	OBJECT *root;

	DIAG((D_menu,wt->owner,"display_menu_widget on %d for %s%s",
		wind->handle, t_owner(wt), wt->menu_line ? "; with menu_line" : ""));

	/* Convert relative coords and window location to absolute screen location */
	root = rp_2_ap(wind, widg, NULL);

	/* clip work area */
	if (wind->nolist && (wind->dial&created_for_POPUP))
	{
		set_clip(&wind->wa);
		draw_object_tree(lock, wt, NULL, widg->start , MAX_DEPTH, 126);
		clear_clip();
	}
	else
		draw_object_tree(lock, wt, NULL, widg->start , MAX_DEPTH, 6);

	if (wt->menu_line)			/* HR 090501  menu in user window.*/
	{
		int titles;

		write_menu_line((RECT*)&root->ob_x);	/* HR: not in standard menu's object tree */

		/* HR: Use the AES's client structure to register the rectangle for the current menu bar. */
		titles = root[root[0].ob_head].ob_head;
		C.Aes->waiting_for = XAWAIT_MENU;
		object_area(&C.Aes->em.m1, root, titles, 0, 0);
		C.Aes->em.flags = MU_M1;	/* into menu bar */
	}
	return true;
}

/* root function of a menu_bar task */
/* The task stages are driven by mouse rectangle events completely */
/* Called by XA_move_event()
   and by do_widgets() only for the menu_bar itself */
static bool
click_menu_widget(enum locks lock, struct xa_window *wind, struct xa_widget *widg)
{
	int pid = C.AESpid;

	DIAG((D_menu, NULL, "click_menu_widget"));

	/* only need locking when the menu is outside a app window. */
	if (cfg.menu_locking && wind == root_window)
	{
		/* Can't bring up menu without locking the screen */
		if (!lock_screen(C.Aes, 0, NULL, 4))
			/* We return false here so the widget display status
			 * stays selected whilst it repeats */
			return false;

		if (!lock_mouse(C.Aes, 0, NULL, 4))
		{
			unlock_screen(C.Aes, 5);
			return false;
		}
	}

	if (!menu_title(lock, wind, widg, pid))
		menu_finish(NULL);

	return false;
}

static bool
menu_title(enum locks lock, struct xa_window *wind, XA_WIDGET *widg, int locker)
{
	RECT r;
	Tab *tab;
	MENU_TASK *k;
	XA_TREE *wt;
	OBJECT *root;
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
	root = wt->tree;
	k->root = root;
	k->titles = root[root[0].ob_head].ob_head;
	k->menus  = root[0].ob_tail;
	k->about = root[root[k->menus].ob_head].ob_head;

	check_mouse(wind->owner, &f, &k->x, &k->y);

	k->rdx = r.x;
	k->rdy = r.y;
	k->clicked_title = find_menu_object(tab, k->titles);
	k->point_at_menu = -1;

	if (   k->clicked_title > -1
	    && root[k->clicked_title].ob_type == G_TITLE
	    && !(root[k->clicked_title].ob_state & OS_DISABLED))
	{
		C.menu_base = tab;			/* OK, can do */
		C.menu_nest = 0;
		tab->ty = k->ty = (wind == root_window ? ROOT_MENU : MENU_BAR);
		k->stage = IN_TITLE;
		object_area(&k->bar, root, k->titles, 0, 0);
		change_title(tab, 1);
		root[k->menus].ob_flags &= ~OF_HIDETREE;
		n = root[k->menus].ob_head;
		for (f = root[k->titles].ob_head; f != k->titles; f = root[f].ob_next)
		{
			if (f == k->clicked_title)
				item = n;
			else
				root[n].ob_flags |= OF_HIDETREE;
				
			n = root[n].ob_next;
		}
	
		root[item].ob_flags &= ~OF_HIDETREE;	/* Show the actual menu */
		k->entry = click_menu_entry;		/* obeyed by XA_MOUSE.C */

		if (desk_menu(tab) && C.menu_nest < CASCADE-1)
		{
			desk_popup.mn_tree = built_desk_popup(tab->lock, 24,24);
			desk_popup.mn_menu = 0;
			desk_popup.mn_item = 0;
			desk_popup.mn_scroll = 0;
			desk_popup.mn_keystate = 0;
			attach_menu(tab->lock, C.Aes, root, k->about + 2, &desk_popup);
		}

		display_popup(tab, root, item, r.x, r.y);

		k->em.flags = MU_M1|1;		/* fill out rect event data; out of title */
		object_area(&k->em.m1, root, k->clicked_title, 0, 0);
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
void
set_menu_widget(struct xa_window *wind, XA_TREE *menu)
{
	XA_TREE *wt = &wind->menu_bar;
	XA_WIDGET *widg = get_widget(wind, XAW_MENU);
	OBJECT *obj = menu->tree;

	DIAG((D_widg, menu->owner, "set_menu_widget on %d for %s",
		wind->handle, w_owner(wind)));

	menu->is_menu   = true;				/* HR 131101: set the flags in the original */
	menu->menu_line = (wind == root_window);	/* HR 090501: menu in root window.*/

	*wt = *menu;

	/* additional fix to fit in window */
	obj->ob_width = obj[obj->ob_head].ob_width = obj[obj->ob_tail].ob_width = wind->wa.w;

	widg->display = display_menu_widget;
	widg->click = click_menu_widget;
	widg->dclick = NULL;
	widg->drag = NULL /* drag_menu_widget */;
	widg->state = OS_NORMAL;
	widg->stuff = wt;
	widg->start = 0;
	wind->tool = widg;
	wind->active_widgets |= XaMENU;
}

/*
 * Attach a popup to a window.
 */
static XA_TREE *
set_popup_widget(Tab *tab, struct xa_window *wind, OBJECT *form, int item)
{
	MENU_TASK *k = &tab->task_data.menu;
	XA_TREE *wt = &wind->toolbar;
	XA_WIDGET *widg = get_widget(wind, XAW_MENU);
	XA_WIDGET_LOCATION loc;
	OBJECT *ob = form +item;
	WidgetBehaviour display_object_widget;
	int frame = wind->frame;

	if (frame < 0)
		frame = 0;

	wt->tree = form;
	wt->owner = wind->owner;

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

	loc.r.w = form->ob_width;
	loc.r.h = form->ob_height;
	loc.mask = XaMENU;

	widg->type = XAW_MENU;
 	widg->display = display_menu_widget;
#if 0
	/* handled by other means (Task administration Tab) */
	widg->click = click_popup_widget;
#endif
	widg->loc = loc;
	widg->state = OS_NORMAL;
	widg->stuff = wt;
	widg->start = item;
	if (tab->nest)
		/* HR testing: Use the window borders. */
		wt->zen = true;
	wt->is_menu = true;
	wind->tool = widg;
	XA_slider(wind, XAW_HSLIDE, ob->ob_width, wind->wa.w, 0);
	XA_slider(wind, XAW_VSLIDE, ob->ob_height, wind->wa.h, 0);

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

/* Called by menu_bar() only */
void
fix_menu(OBJECT *root, bool do_desk)
{
	int titles, menus, tbar, s_ob, t_ob;

	DIAG((D_menu, NULL, "fixing menu 0x%lx", root));

	tbar = root[0].ob_head;
	titles = root[tbar].ob_head;
	menus = root[0].ob_tail;

	root->ob_width = root[tbar].ob_width = root[menus].ob_width = screen.r.w;
	set_ob_spec(root,tbar, menu_colors());

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
		root[t_ob++].ob_spec.free_string = "  Clients \3";
		while (t_ob != s_ob)
		{
			root[t_ob].ob_flags |= OF_HIDETREE|OS_DISABLED;
			t_ob = root[t_ob].ob_next;
		}
	}

	DIAG((D_menu, NULL, "done fix_menu()"));
}
