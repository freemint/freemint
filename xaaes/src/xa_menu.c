/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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

#include <mint/mintbind.h>

#include "xa_types.h"
#include "xa_global.h"

#include "util.h"
#include "rectlist.h"
#include "objects.h"
#include "c_window.h"
#include "handler.h"
#include "menuwidg.h"
#include "widgets.h"

#include "app_man.h"
#include "taskman.h"


#if GENERATE_DIAGS
bool
check_tree(XA_CLIENT *client, OBJECT *tree, int item)
{
	long ltree = (long)tree;
	XA_TREE *menu_bar = get_menu();
	RSHDR *rs;

	/* set by AES menu_bar() */
	if (tree == menu_bar->tree && menu_bar->lastob)
	{
		if (item < 0 || item > menu_bar->lastob)
		{
			DIAGS(("***** MENU ITEM > LASTOB %d ***** tree %lx, item %d\n",
				menu_bar->lastob, tree, item));

			/* definetely wrong */
			return false;
		}
	}
	else if (item < 0)
	{
		DIAGS(("***** ITEM ERROR ***** tree %lx, item %d\n", tree, item));
		return false;
	}
	else if (ltree < 0 || (ltree >= 0x400000 && ltree < 0x1000000) || ltree > 0x1400000)
	{
		DIAGS(("***** TREE ERROR ***** tree %lx, item %d\n",tree,item));
		return false;
	}
	else if (client)
	{
		rs = client->rsrc;
		if (rs)
		{
			OBJECT **tr, *ob;
			int i; /* Is it a tree of the resource? */

			(long)tr = (long)rs + rs->rsh_trindex;
			for (i = 0; i < rs->rsh_ntree; i++)
			{
				ob = tr[i];
				if (ob == tree)
				{
					int j = 0;

					while ((ob->ob_flags & LASTOB) == 0)
					{
						ob++;
						j++;
					}

					if (item > j)
					{
						DIAGS(("***** TREE ITEM > LASTOB %d ***** tree %lx, item %d\n",
							j, tree, item));
						return false;
					}	
				}
			}
		}
	}

	/* DIAGS(("check_tree ok: %lx, %d\n", tree, item)); */

	/* OK, or (foreign tree && item >= 0, have to accept) */
	return true;
}
#else
#define check_tree(a,b,c) 1
#endif

/*
 * This file provides the interface between XaAES's (somewhat strange) menu
 * system and the standard GEM calls. Most GEM apps will only care about the
 * menu they install onto the desktop, although XaAES can allow seperate menus
 * for each window...
 */

/*
 * Install a menu bar onto the desktop window
 */
unsigned long
XA_menu_bar(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	XA_TREE *menu_bar;
	XA_TREE *menu = &client->std_menu;

	OBJECT *mnu = pb->addrin[0];
	XA_WINDOW *wl;
	XA_CLIENT *top_owner;

	CONTROL(1,1,1)

	menu_bar = get_menu();

	pb->intout[0] = 0;

	DIAG((D_menu, NULL, "menu_bar for %s, %lx\n", c_owner(client), mnu));

	switch (pb->intin[0])
	{
	case MENU_INSTALL:
		DIAG((D_menu,NULL,"MENU_INSTALL\n"));

		if (mnu)
		{
			/* Do a special fix on the menu  */
			fix_menu(mnu,true);
			DIAG((D_menu,NULL,"fixed menu\n"));

			mnu->r.w = mnu[mnu->ob_tail].r.w = mnu[mnu->ob_head].r.w = screen.r.w;
	
#if GENERATE_DIAGS
			{
				int i = 0;
				while ((mnu[i].ob_flags & LASTOB) == 0)
					i++;
				menu_bar->lastob = i;
			}
#endif
			/* HR: std_menu is now a complete widget_tree :-) */
			menu->tree = mnu;
			menu->owner = client;
			menu->is_menu = true;
			menu->menu_line = true;
			swap_menu(lock|winlist, client, false, 6);
			pb->intout[0] = 1;
			DIAG((D_menu, NULL, "done display, lastob = %d\n", menu->lastob));
		}
		break;

	case MENU_REMOVE:
		DIAG((D_menu,NULL,"MENU_REMOVE\n"));
		remove_attachments(lock|winlist, client, menu->tree);
		menu->tree = NULL;

		top_owner = C.Aes;
		wl = window_list;
		while (wl)
		{
			if (   wl->owner != client
			    && wl->owner->std_menu.tree)
			{
				top_owner = wl->owner;
				break;
			}
			wl = wl->next;
		}
		swap_menu(lock|winlist, top_owner, false, 7);
		pb->intout[0] = 1;
		break;
		
	case MENU_INQUIRE:
		DIAG((D_menu, NULL, "MENU_INQUIRE := %d\n", menu_bar->owner->pid));
		pb->intout[0] = menu_bar->owner->pid;
		break;
	}

	DIAG((D_menu,NULL,"done menu_bar()\n"));
	return XAC_DONE;
}

/*
 * Highlight / un-highlight a menu title
 * - Actually, this isn't really needed as XaAES cancels the highlight itself...
 * ...it's only here for compatibility. 
 */
unsigned long
XA_menu_tnormal(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	XA_TREE *menu_bar = get_menu();
	OBJECT *tree = (OBJECT *)pb->addrin[0];
	
	CONTROL(2,1,1)

	/* Change the highlight / normal status of a menu title */
	if (pb->intin[1])
		tree[pb->intin[0]].ob_state &= ~SELECTED;
	else
		tree[pb->intin[0]].ob_state |= SELECTED;

	/* If we just changed the main root window's menu, better redraw it */
	if ((tree == menu_bar->tree) && (tree[pb->intin[0]].ob_type == G_TITLE))
		redraw_menu(lock);

	pb->intout[0] = 1;
	return XAC_DONE;
}

/*
 * Enable/Disable a menu item
 */
unsigned long
XA_menu_ienable(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	XA_TREE *menu_bar = get_menu();
	OBJECT *tree = (OBJECT *)pb->addrin[0];

	CONTROL(2,1,1)

	/* Change the disabled status of a menu item */
	if (pb->intin[1])
		tree[pb->intin[0]].ob_state &= ~DISABLED;
	else
		tree[pb->intin[0]].ob_state |= DISABLED;

	/* If we just changed the main root window's menu, better redraw it */
	if ((tree == menu_bar->tree) && (tree[pb->intin[0]].ob_type == G_TITLE))
		redraw_menu(lock);

	pb->intout[0] = 1;
	return XAC_DONE;
}

/*
 * Check / un-check a menu item
 */
unsigned long
XA_menu_icheck(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	XA_TREE *menu_bar = get_menu();
	OBJECT *tree = (OBJECT *)pb->addrin[0];

	CONTROL(2,1,1)

	/* Change the disabled status of a menu item */
	if (pb->intin[1])
		tree[pb->intin[0]].ob_state |= CHECKED;
	else
		tree[pb->intin[0]].ob_state &= ~CHECKED;

	/* If we just changed the main root window's menu, better redraw it */
	if ((tree == menu_bar->tree) && (tree[pb->intin[0]].ob_type == G_TITLE))
		redraw_menu(lock);

	pb->intout[0] = 1;
	return XAC_DONE;
}

/*
 * Change a menu item's text
 */
unsigned long
XA_menu_text(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	char *text = (char *)pb->addrin[1];

	XA_TREE *menu_bar = get_menu();
	OBJECT *tree = (OBJECT *)pb->addrin[0];

	CONTROL(1,1,2)

	strcpy(get_ob_spec(&tree[pb->intin[0]])->string, text);

	/* If we just changed the main root window's menu, better redraw it */
	if ((tree == menu_bar->tree) && (tree[pb->intin[0]].ob_type == G_TITLE))
		redraw_menu(lock);

	pb->intout[0] = 1;
	return XAC_DONE;
}

/*
 * Register an apps 'pretty' & 'official' names.
 */
unsigned long
XA_menu_register(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	int f; char *n = pb->addrin[0];
	
	CONTROL(1,1,1)

	if (n)
	{
		pb->intout[0] = client->pid;
		if (pb->intin[0] != -1)
		{
			int l = strlen(n);
			if (l >= NICE_NAME)
			{
				strncpy(client->name, n, NICE_NAME-1);
				*(client->name + NICE_NAME - 1) = 0;
			}
			else
				strcpy(client->name, n);
			update_tasklist(lock);

			DIAGS(("menu_register 'nice' for %d: '%s'\n",
				client->pid, client->name));
		}
		else
		{
			strncpy(client->proc_name, n,8);
			client->proc_name[8] = '\0';
			for (f = strlen(client->proc_name); f < 8; f++)
				client->proc_name[f] = ' ';
			strnupr(client->proc_name, 8);
			update_tasklist(lock);

			DIAGS(("menu_register 'proc' for %d: '%s'\n",
				client->pid, client->proc_name));
		}
	}
	else
		pb->intout[0] = -1;

	return XAC_DONE;
}

/*
 * Display and handle a popup menu.  HR: march 2000
 */

/* Ozk 180603: Absolutely hair-raising usage of Psemaphore across
 * different PIDs! I removed the LOCKSCREEN from the Ktab flags,
 * since the lock_screen() call from XA_handler was done under the
 * clients pid, while the unlock_screen() happens under the kernel
 * pid. Guess the results - total LOCKUP until offending client killed!
*/ 
unsigned long
XA_menu_popup(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(2,1,2)

	pb->intout[0] = -1;

	if (   pb->addrin[0]
	    && pb->addrin[1]
	    /*&& lock_screen(client, 0, NULL, 6)*/)
	{
		Tab *tab = C.active_menu;
		MENU *mn = pb->addrin[0], *md = pb->addrin[1];
		short x, y;
		OBJECT *ob = mn->mn_tree;

//		if (cfg.menu_locking)
//			lock_screen(C.Aes, -1, NULL, 0);


		if (tab->ty == NO_TASK)		/* else already locked */
		{	
			*md = *mn;

			DIAG((D_menu,NULL,"menu_popup %lx + %d\n",ob, mn->mn_menu));

			C.menu_base = tab;
			tab->pb = pb;
			tab->locker = client->pid; /*C.Aes->pid;*/ /*client->pid;*/
			tab->client = client;
			tab->lock = lock;
			ob->r.x = 0;
			ob->r.y = 0;
			object_offset(ob, mn->mn_menu, 0, 0, &x, &y);
			tab->wind = NULL;
			tab->widg = NULL;
			tab->ty = POP_UP;
			tab->scroll = mn->mn_scroll != 0;
			bzero(&tab->task_data.menu, sizeof(MENU_TASK));

			do_popup(tab, ob, mn->mn_menu,
				 click_popup_entry,
				 pb->intin[0] - x,
				 pb->intin[1] - y);

			return XAC_BLOCK;
		}
//		if (cfg.menu_locking)
//			unlock_screen(C.Aes, 0);
	}
 	return XAC_DONE;
}

/*
 * HR 051101: MagiC popup function.
 */
unsigned long
XA_form_popup(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	short x, y;
	OBJECT *ob = pb->addrin[0];
	CONTROL(2,1,1)

	pb->intout[0] = -1;

	if (ob) /* && lock_screen(client, 0, NULL,7) ) */
	{
		Tab *tab = C.active_menu;

//		if (cfg.menu_locking)
//			lock_screen(C.Aes, -1, 0, 0);

		if (tab->ty == NO_TASK)		/* else already locked */
		{
			DIAG((D_menu,NULL,"form_popup %lx\n",ob));

			C.menu_base = tab;
			tab->pb = pb;
			tab->locker = client->pid; /*C.Aes->pid;*/	/*client->pid;*/
			tab->client = client;
			tab->lock = lock;
			tab->wind = NULL;
			tab->widg = NULL;
			tab->ty = POP_UP;
			tab->scroll = false;

			x = pb->intin[0];
			y = pb->intin[1];
			if (x == 0 && y == 0)
			{
				x = ob->r.x;
				y = ob->r.y;
			}
			else
			{
				x -= ob->r.w/2;
				y -= ob->r.h/2;
			}
			if (x < 4)
				x = 4;
			if (y < MENU_H)
				y = MENU_H;
			ob->r.x = 0;
			ob->r.y = 0;

			bzero(&tab->task_data.menu, sizeof(MENU_TASK));

			do_popup(tab, ob, 0,
				 click_form_popup_entry,
				 x,
				 y);

			return XAC_BLOCK;
		}
//		if (cfg.menu_locking)
//			unlock_screen(C.Aes, 0);
	}

	return XAC_DONE;
}

/*
 * Attach a submenu to a menu item.  HR: march 2000
 */
unsigned long
XA_menu_attach(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(2,1,2)

	DIAG((D_menu, client, "menu_attach %d\n", pb->intin[0]));

	if (pb->addrin[0] && pb->addrin[1])
	{
		switch (pb->intin[0])
		{
		case ME_ATTACH:
			pb->intout[0] = attach_menu(lock,
						    client,
						    pb->addrin[0],
						    pb->intin[1],
						    pb->addrin[1]);
			break;
		case ME_REMOVE:
			pb->intout[0] = detach_menu(lock,
						    client,
						    pb->addrin[0],
						    pb->intin[1]);
			break;
		case ME_INQUIRE:
			pb->intout[0] = inquire_menu(lock,
						     client,
						     pb->addrin[0],
						     pb->intin[1],
						     pb->addrin[1]);
			break;
		}
	}
	else
		pb->intout[0] = 1;

 	return XAC_DONE;
}

/*
 * Align a submenu.  HR: march 2000
 */
unsigned long
XA_menu_istart(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(1,1,1)

	DIAG((D_menu,client,"menu_istart\n"));

	pb->intout[0] = 0;
 	return XAC_DONE;
}

/*
 * Influence behaviour.  HR: march 2000
 */
unsigned long
XA_menu_settings(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(1,1,1)

	DIAG((D_menu,client,"menu_settings %d\n", pb->intin[0]));

	/* accepted, no implementation planned */
	pb->intout[0] = 1;
	if (pb->intin[0] == 0)
	{
		MN_SET *mn = pb->addrin[0];
		mn->display = 200;
		mn->drag = 10000;
		mn->delay = 250;
		mn->speed = 0;
		mn->height = root_window->wa.h/screen.c_max_h;
	}
	return XAC_DONE;
}
