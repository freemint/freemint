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

#include "xa_menu.h"
#include "xa_global.h"

#include "xa_appl.h"
#include "xa_bubble.h"
#include "xa_rsrc.h"
#include "k_main.h"
#include "app_man.h"
#include "c_window.h"
#include "menuwidg.h"
#include "obtree.h"
#include "rectlist.h"
#include "taskman.h"
#include "util.h"
#include "widgets.h"

void set_menu_width( OBJECT *mnu, XA_TREE *mwt )
{
	struct xa_widget *widg = get_menu_widg();
	int i = 0, t = 0;
	short mw = 0;
	short xm = 0;
	while ((mnu[i].ob_flags & OF_LASTOB) == 0)
	{
		if( mnu[i].ob_x > xm && (mnu[i].ob_type == G_TITLE || mnu[i].ob_type == G_USERDEF) )
		{
			t = i;
			xm = mnu[i].ob_x;
		}
		i++;
	}

	mw = mnu[t].ob_x + mnu[t].ob_width;
	mwt->lastob = i;
	mnu[0].ob_width = widg->r.g_w = mwt->area.g_w = mw + mwt->area.g_x;

	/* !!! */
	mwt->area.g_h += 2;
}
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
XA_menu_bar(int lock, struct xa_client *client, AESPB *pb)
{
	bool swap = true;
	XA_TREE *menu_bar;
	XA_TREE *menu = client->std_menu;
	OBJECT *mnu = (OBJECT*)pb->addrin[0];
	struct xa_client *top_owner;

	CONTROL(1,1,1)

	menu_bar = get_menu();

	pb->intout[0] = 0;

	DIAG((D_menu, NULL, "menu_bar for %s, %lx", c_owner(client), (unsigned long)mnu));

	switch (pb->intin[0])
	{
	case MENU_INSTL:
		swap = false;
		/* fall through... */

	case MENU_INSTALL:
		DIAG((D_menu,NULL,"MENU_INSTALL"));

		if (validate_obtree(client, mnu, "XA_menu_bar:"))
		{
			XA_TREE *mwt = obtree_to_wt(client, mnu);

			if (!mwt || (mwt && mwt != menu))
			{
				if (!mwt)
					mwt = new_widget_tree(client, mnu);

				assert(mwt);

				/* Do a special fix on the menu  */
				fix_menu(client, mwt, root_window, true);
				DIAG((D_menu,NULL,"fixed menu"));

				/* ->fix menu */
				if( cfg.menu_layout != 0 )
				{
					set_menu_width( mnu, mwt );
				}
				/* HR: std_menu is now a complete widget_tree :-) */
				mwt->is_menu = true;

				if (swap)
				{
					top_owner = get_app_infront();

					if (menu && (client == top_owner || !top_owner->std_menu))
					{
						swap_menu(lock|LOCK_WINLIST, client, mwt, SWAPM_TOPW);
					}
					else
					{
						client->nxt_menu = mwt;
						app_in_front(lock, client, true, true, true);
					}
				}

				pb->intout[0] = 1;
				DIAG((D_menu, NULL, "done display, lastob = %d", mwt->lastob));
			}
			else if (mwt && swap)
			{
				top_owner = get_app_infront();
				if( cfg.menu_layout == 0 )
					wt_menu_area(mwt);
				if (menu && (client == top_owner || !top_owner->std_menu))
				{
					swap_menu(lock|LOCK_WINLIST, client, NULL, SWAPM_TOPW);
				}
				else
				{
					client->nxt_menu = mwt;
					app_in_front(lock, client, true, true, true);
				}
			}
		}
		break;

	case MENU_REMOVE:
		DIAG((D_menu,NULL,"MENU_REMOVE"));

		if (!menu)
			menu = client->nxt_menu;

		if (menu)
		{
			swap_menu(lock, client, NULL, SWAPM_REMOVE);
			remove_attachments(lock|LOCK_WINLIST, client, menu);
			pb->intout[0] = 1;
		}
		break;

	case MENU_INQUIRE:
		DIAG((D_menu, NULL, "MENU_INQUIRE := %d", menu_bar->owner->p->pid));

		pb->intout[0] = menu_bar->owner->p->pid;
		break;

	case MENU_GETMODE:
		DIAG((D_menu, NULL, "MENU_GETMODE"));

		if (cfg.menu_bar != 2)
			pb->intout[0] |= MENU_HIDDEN;
		if (cfg.menu_behave == PUSH)
			pb->intout[0] |= MENU_PULLDOWN;
		pb->intout[0] |= MENU_SHADOWED; /* always ON */
		break;

	case MENU_SETMODE:
		DIAG((D_menu, NULL, "MENU_SETMODE (unimplemented)"));
		break;

	case MENU_UPDATE:
		DIAG((D_menu, NULL, "MENU_UPDATE (unimplemented)"));
		break;
	}

	DIAG((D_menu,NULL,"done menu_bar()"));
	return XAC_DONE;
}

static void
upd_menu(int lock, struct xa_client *client, OBJECT *tree, short item, bool redraw)
{
	XA_TREE *wt;

	wt = obtree_to_wt(client, tree);
	if (wt && (tree[item].ob_type == G_TITLE || tree[item].ob_type == G_USERDEF))
	{
		if( cfg.menu_layout == 0 )
			wt_menu_area(wt);
		if (wt == get_menu())
		{
			set_rootmenu_area(client);
			if (redraw)
			{
				redraw_menu(lock);
			}
		}
	}
}

/*
 * Highlight / un-highlight a menu title
 * - Actually, this isn't really needed as XaAES cancels the highlight itself...
 * ...it's only here for compatibility.
 */
unsigned long
XA_menu_tnormal(int lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *tree = (OBJECT *)pb->addrin[0];
	short i = pb->intin[0], obj, state;

	CONTROL(2,1,1)

	if (!validate_obtree(client, tree, "XA_menu_tnormal:"))
		return XAC_DONE;

	obj = i & ~0x8000;

	state = tree[obj].ob_state;

	if (state & OS_DISABLED)
	{
		pb->intout[0] = 0;
	}
	else
	{
		DIAG((D_menu, client, "menu_tnormal: tree=%lx, obj=%d(%d), state=%d",
			(unsigned long)tree, obj, i, pb->intin[1]));

		if (pb->intin[1])
			state &= ~OS_SELECTED;
		else
			state |= OS_SELECTED;

		if (tree[obj].ob_state != state)
		{
			tree[obj].ob_state = state;
			upd_menu(lock, client, tree, obj, true);
		}
		pb->intout[0] = 1;
	}
	return XAC_DONE;
}

/*
 * Enable/Disable a menu item
 */
unsigned long
XA_menu_ienable(int lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *tree = (OBJECT *)pb->addrin[0];
	short i = pb->intin[0], obj, state;
	bool redraw;

	CONTROL(2,1,1)

	if (!validate_obtree(client, tree, "XA_menu_ienable:"))
		return XAC_DONE;

	redraw = i & 0x8000 ? true : false;
	obj = i & ~0x8000;
	state = tree[obj].ob_state;

	DIAG((D_menu, client, "menu_ienable: tree=%lx, obj=%d(%d), state=%d",
		(unsigned long)tree, obj, i, pb->intin[1]));

	/* Change the disabled status of a menu item */
	if (pb->intin[1])
		state &= ~OS_DISABLED;
	else
		state |= OS_DISABLED;

	if (tree[obj].ob_state != state)
	{
		tree[obj].ob_state = state;
		upd_menu(lock, client, tree, obj, redraw);
	}

	pb->intout[0] = 1;
	return XAC_DONE;
}

/*
 * Check / un-check a menu item
 */
unsigned long
XA_menu_icheck(int lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *tree = (OBJECT *)pb->addrin[0];
	short i = pb->intin[0], obj, state;

	CONTROL(2,1,1)

	if (!validate_obtree(client, tree, "XA_menu_icheck:"))
		return XAC_DONE;

	obj = i & ~0x8000;
	state = tree[obj].ob_state;

	DIAG((D_menu, client, "menu_icheck: tree=%lx, obj=%d(%d), state=%d",
		(unsigned long)tree, obj, i, pb->intin[1]));
	/* Change the disabled status of a menu item */
	if (pb->intin[1])
		state |= OS_CHECKED;
	else
		state &= ~OS_CHECKED;

	if (tree[obj].ob_state != state)
	{
		tree[obj].ob_state = state;
		upd_menu(lock, client, tree, obj, false);
	}

	pb->intout[0] = 1;

	return XAC_DONE;
}

/*
 * Change a menu item's text
 */
unsigned long
XA_menu_text(int lock, struct xa_client *client, AESPB *pb)
{
	const char *text = (const char *)pb->addrin[1];
	OBJECT *tree = (OBJECT *)pb->addrin[0];
	short i = pb->intin[0], obj;

	CONTROL(1,1,2)

	if (!validate_obtree(client, tree, "XA_menu_text:"))
		return XAC_DONE;

	obj = i & ~0x8000;


	strcpy(object_get_spec(&tree[obj])->free_string, text);

	upd_menu(lock, client, tree, obj, false);

	pb->intout[0] = 1;

	return XAC_DONE;
}
/*
 * Register an apps 'pretty' & 'official' names.
 */
unsigned long
XA_menu_register(int lock, struct xa_client *client, AESPB *pb)
{
	const char *n = (const char *)pb->addrin[0];

	CONTROL(1,1,1)

	if (n)
	{
		pb->intout[0] = client->p->pid;

		if (pb->intin[0] != -1)
		{
			client_nicename(client, n, false);
#if WITH_BBL_HELP
			if( cfg.xa_bubble && !strcmp( "  BubbleGEM ", client->name ) )
			{
				xa_bubble( lock, bbl_disable_bubble, 0, 0 );
			}
#endif

			/* refresh the name change in the taskmanager */
			update_tasklist_entry(AES_CLIENT, client, 0, 0, NOREDRAW);

			DIAGS(("menu_register 'nice' for %d: '%s'",
				client->p->pid, client->name));
		}
		else
		{
			int f;

			/* copy over */
			strncpy(client->proc_name, n, 8);
			client->proc_name[8] = '\0';

			/* fill with space */
			for (f = strlen(client->proc_name); f < 8; f++)
				client->proc_name[f] = ' ';

			/* uppercase */
			strnupr(client->proc_name, 8);

			/* refresh the name change in the taskmanager */
			update_tasklist_entry(AES_CLIENT, client, 0, 0, NOREDRAW);
			DIAGS(("menu_register 'proc' for %d: '%s'",
				client->p->pid, client->proc_name));
		}
	}
	else
		pb->intout[0] = -1;

	return XAC_DONE;
}

/* XXX - Ozk: TODO: Things related to mn_selected!
 * mn_scroll	If positive number;
 *			sets the 'row number' in the popup at which to start scroll area.
 *		If Negative number;
 *			Flags a dropdown listbox session.
 *		If Null;
 *			Scrolling turned off
 *
 * mn_item	Sets the object number of the object to be ontop of the scrollable
 *		area.
 */

static void
init_popinfo(XAMENU *mn, struct xa_popinfo *pi)
{
	int i, count, pop_h;
	short this, parent;
	struct widget_tree *wt = mn->wt;
	OBJECT *obtree = wt->tree;

	pop_h = cfg.menu_settings.mn_set.height;

	pi->count = count = ob_count_objs(obtree, mn->menu.mn_menu, 1);
	pi->objs = kmalloc(sizeof(short) * count);

	pi->parent = parent = mn->menu.mn_menu;

	if (pi->objs && mn->menu.mn_scroll > 1 && mn->menu.mn_scroll < (pop_h - 1) && count > pop_h)
	{
		short flag = 0, *objs = pi->objs;

		for (i = 1, this = obtree[pi->parent].ob_head; i <= count; i++)
		{
			if (this == parent || this == -1)
			{
				break;
			}

			*objs++ = this;

			if (mn->menu.mn_scroll == i)
			{
				/*
				 * mn_scroll == row in popup at which scrolling starts
				 */
				flag |= 1;
				if (flag & 2)
				{
					/* If we're asked to start scrolling at an object above
					 * the object at which scrolling starts, we refuse!
					 */
					pi->scrl_start_obj = i - 1;
				}
				pi->scrl_start_row = i - 1;
				pi->scrl_height = pop_h - (i - 1);
			}
			if (mn->menu.mn_item == this)
			{
				/*
				 * mn_item == object number that we want to initiate scroll field with
				 */
				flag |= 2;
				pi->scrl_start_obj = i - 2;
			}
			this = obtree[this].ob_next;
		}
#if 1
		if (flag != ((1<<1)|1) || pi->scrl_height < 3)
		{
noscroll:
			pi->scrl_start_row = -1;
			kfree(pi->objs);
			pi->objs = NULL;
		}
		else
		{
			if (pi->scrl_start_obj < pi->scrl_start_row)
			{
				pi->scrl_start_obj = pi->scrl_start_row;
			}

			if ( (pi->scrl_start_obj + pi->scrl_height) > pi->count)
			{
				short corr = (pi->scrl_start_obj + pi->scrl_height) - pi->count;
				pi->scrl_start_obj -= corr;
				if (pi->scrl_start_obj < pi->scrl_start_row)
				{
					goto noscroll;
				}
			}

			pi->scrl_curr_obj = pi->scrl_start_obj;

			strcpy(pi->scrl_start_txt, "  \1\1\1 ");
			strcpy(pi->scrl_end_txt, "  \2\2\2 ");

			pi->save_start_txt = object_get_spec(obtree + pi->objs[pi->scrl_start_row])->free_string;
			pi->save_end_txt = object_get_spec(obtree + pi->objs[pi->count - 1])->free_string;

			if (pi->scrl_start_row != pi->scrl_curr_obj)
			{
				object_set_spec(obtree + pi->objs[pi->scrl_start_row], (long)pi->scrl_start_txt);
			}

			if ((pi->scrl_start_obj + pi->scrl_height) != pi->count)
			{
				object_set_spec(obtree + pi->objs[pi->count - 1], (long)pi->scrl_end_txt);
			}

			{
				short obj, y, first, last;

				first = pi->scrl_curr_obj + 1;
				last = first + pi->scrl_height - 2;

				obtree[parent].ob_height = screen.c_max_h * pop_h;
				obj = pi->objs[pi->count - 1];
				obtree[obj].ob_y = screen.c_max_h * (pop_h - 1);

				obj = pi->objs[pi->scrl_start_row];
				obtree[obj].ob_next = pi->objs[first];
				y = obtree[obj].ob_y + obtree[obj].ob_height;

				for (i = first; i < last; i++)
				{
					obj = pi->objs[i];
					obtree[obj].ob_next = pi->objs[i + 1];
					obtree[obj].ob_y = y;
					y += obtree[obj].ob_height;
				}

				obtree[pi->objs[last - 1]].ob_next = pi->objs[pi->count - 1];
			}

		}
#endif

	}
	else
	{
		if (pi->objs)
		{
			kfree(pi->objs);
			pi->objs = NULL;
		}
		if (mn->menu.mn_scroll > 0)
			mn->menu.mn_scroll = 1;
	}
}

/*
 * menu_popup expects the following elements of 'mn' to be initialized;
 *	mn->menu
 *	mn->mn_selected
 * All other elements are initialized here.
 */
int
menu_popup(int lock, struct xa_client *client, XAMENU *mn, XAMENU_RESULT *result, short px, short py, short usr_evnt)
{
	if (mn && result)
	{
		Tab *tab;
		short x, y;
		OBJECT *ob;

		if (TAB_LIST_START)
			popout(TAB_LIST_START);

		tab = nest_menutask(NULL);
		ob = mn->menu.mn_tree;

		if (tab && validate_obtree(client, ob, "_menu_popup:"))		/* else already locked */
		{
			XA_TREE *wt;
			short old_x, old_y;

			wt = obtree_to_wt(client, ob);
			if (!wt)
				wt = new_widget_tree(client, ob);
			if (!wt)
			{
				return 0;
			}

			mn->wt = wt;

			init_popinfo(mn, &tab->task_data.menu.p);

			result->menu = mn->menu;
			result->at = NULL;

			DIAG((D_menu,NULL,"_menu_popup %lx + %d", (unsigned long)ob, mn->menu.mn_menu));

			tab->locker = client->p->pid;
			tab->client = client;
			tab->lock = lock;

			old_x = ob->ob_x;
			old_y = ob->ob_y;

			ob->ob_x = ob->ob_y = wt->dx = wt->dy = 0;
			obj_offset(wt, aesobj(&wt->tree[mn->menu.mn_menu], mn->menu.mn_item), &x, &y);
			tab->wind = NULL;
			tab->widg = NULL;
			tab->ty = POP_UP;

			if (mn->menu.mn_scroll == -1)
				tab->scroll = 8;
			else if (mn->menu.mn_scroll == 1)
				tab->scroll = cfg.menu_settings.mn_set.height;
			else
				tab->scroll = 0;

			tab->usr_evnt = usr_evnt;
			tab->data = result;
			result->menu.mn_item = -1;

			start_popup_session(tab, wt, mn->menu.mn_menu, mn->mn_selected,
				 click_popup_entry,
				 px - x,
				 py - y);

			client->status |= CS_BLOCK_MENU_NAV;
			(*client->block)(client);
			client->status &= ~CS_BLOCK_MENU_NAV;

			ob->ob_x = old_x;
			ob->ob_y = old_y;
			return 1;
		}
	}
	return 0;
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
XA_menu_popup(int lock, struct xa_client *client, AESPB *pb)
{

	CONTROL(2,1,2)

	MENU *result = (MENU *)pb->addrin[1];
	XAMENU xmn;
	XAMENU_RESULT tmp;

	tmp.menu = *result;

	xmn.menu = *(MENU *)pb->addrin[0];
	xmn.mn_selected = -1;

	if (menu_popup(lock, client, &xmn, &tmp, pb->intin[0], pb->intin[1], 1))
	{
		*result = tmp.menu;
		pb->intout[0] = result->mn_item < 0 ? 0 : 1;
	}
	else
		pb->intout[0] = -1;

	return XAC_DONE;
}

/*
 * HR 051101: MagiC popup function.
 */
unsigned long
XA_form_popup(int lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *ob = (OBJECT*)pb->addrin[0];
	CONTROL2(2,1,1, 6,2,3)

	pb->intout[0] = -1;
	if (pb->control[2] >= 2 && pb->control[1] >= 6)
		pb->intout[1] = pb->intin[5];

	if (validate_obtree(client, ob, "XA_form_popup:"))
	{
		Tab *tab;

		if (TAB_LIST_START)
			popout(TAB_LIST_START);

		tab = nest_menutask(NULL);

		if (tab)		/* else already locked */
		{
			XA_TREE *wt;
			short x, y, old_x, old_y;

			wt = obtree_to_wt(client, ob);
			if (!wt)
				wt = new_widget_tree(client, ob);
			if (!wt)
				return XAC_DONE;

			DIAG((D_menu,NULL,"form_popup %lx", (unsigned long)ob));

			tab->locker = client->p->pid;
			tab->client = client;
			tab->lock = lock;
			tab->wind = NULL;
			tab->widg = NULL;
			tab->ty = POP_UP;
			tab->scroll = 0;
			tab->data = &pb->intout[0];
			tab->usr_evnt = 1;

			old_x = ob->ob_x;
			old_y = ob->ob_y;

			x = pb->intin[0];
			y = pb->intin[1];
			if (x == 0 && y == 0)
			{
				x = ob->ob_x;
				y = ob->ob_y;
			}
			else
			{
				x -= ob->ob_width/2;
				y -= ob->ob_height/2;
			}
			if (x < 4)
				x = 4;
			if (y < get_menu_widg()->r.g_h)
				y = get_menu_widg()->r.g_h;
			ob->ob_x = 0;
			ob->ob_y = 0;

			start_popup_session(tab, wt, 0, -1,
				 click_form_popup_entry,
				 x,
				 y);

			client->status |= CS_BLOCK_MENU_NAV;
			(*client->block)(client);
			client->status &= ~CS_BLOCK_MENU_NAV;

			ob->ob_x = old_x;
			ob->ob_y = old_y;
			return XAC_DONE;
		}
	}
	return XAC_DONE;
}

static unsigned long
XA_menu_click(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(2,1,0)

	DIAG((D_menu, client, "menu_click setit=%d, click=%d", pb->intin[1], pb->intin[0]));

	if (pb->intin[1] == MN_CHANGE)
	{
		if (pb->intin[0] == 0)
			cfg.menu_behave = PULL;
		else if (pb->intin[0] == 1)
			cfg.menu_behave = PUSH;
	}

	pb->intout[0] = (cfg.menu_behave == PUSH) ? 1 : 0;

	return XAC_DONE;
}

/*
 * Attach, remove or inquire a submenu to a menu item.  HR: march 2000
 * attach with NULL is remove
 */
unsigned long
XA_menu_attach(int lock, struct xa_client *client, AESPB *pb)
{
	short md;
	CONTROL2(2,1,2, 2,1,0)

	if (pb->control[N_ADDRIN] == 0)
	{
		return XA_menu_click(lock, client, pb);
	}

	md = pb->intin[0];

	DIAG((D_menu, client, "menu_attach %d", pb->intin[0]));

	pb->intout[0] = 1;	/* non-error return code by default */

	if( pb->addrin[1] == 0 )	/* mdata NULL? */
	{
		if( md != ME_INQUIRE )
			md = ME_REMOVE;
		else
			pb->intout[0] = 0;
	}

	if (validate_obtree(client, (OBJECT *)pb->addrin[0], "XA_menu_attach:") && pb->intout[0] != 0)
	{
		XA_TREE *wt;
		MENU *mn;
		XAMENU xamn;

		wt = obtree_to_wt(client, (OBJECT *)pb->addrin[0]);
		if (!wt)
			wt = new_widget_tree(client, (OBJECT *)pb->addrin[0]);
		assert(wt);

		switch (md)
		{
		case ME_ATTACH:
		{
			mn = (MENU *)pb->addrin[1];


			if (is_attach(client, wt, pb->intin[1], NULL))
				detach_menu(lock, client, wt, pb->intin[1]);

			if (mn)
			{
				xamn.wt = obtree_to_wt(client, mn->mn_tree);
				if (!xamn.wt)
					xamn.wt = new_widget_tree(client, mn->mn_tree);

				assert(xamn.wt);

				xamn.menu = *mn;

				pb->intout[0] = attach_menu(lock,
							    client,
							    wt,
							    pb->intin[1],
							    &xamn, NULL,NULL);
			}
			break;
		}
		case ME_REMOVE:
		{
			pb->intout[0] = detach_menu(lock,
						    client,
						    wt,
						    pb->intin[1]);
			break;
		}
		case ME_INQUIRE:
		{
			mn = (MENU *)pb->addrin[1];
			pb->intout[0] = inquire_menu(lock,
						     client,
						     wt,
						     pb->intin[1],
						     &xamn);

			*mn = xamn.menu;
			break;
		}
		}
	}
	else
	{
		pb->intout[0] = 0;
	}

 	return XAC_DONE;
}

/*
 * Align a submenu.
 */
#define MIS_GETALIGN    0
#define MIS_SETALIGN    1
unsigned long
XA_menu_istart(int lock, struct xa_client *client, AESPB *pb)
{
	XA_TREE *wt;
	XA_MENU_ATTACHMENT *a;
	CONTROL(3,1,1)

	DIAG((D_menu,client,"menu_istart"));

	pb->intout[0] = 0;

	wt = obtree_to_wt(client, (OBJECT *)pb->addrin[0]);
	assert(wt);

	a = client->attach;
	while (a)
	{
		if ((a->wt == wt) && (pb->intin[1] == a->menu))
			break;

		a = a->next;
	}

	if (a)
	{        
		if (pb->intin[0] == MIS_GETALIGN)
		{
			pb->intout[0] = a->item;
		}
		else if (pb->intin[0] == MIS_SETALIGN)
		{
			pb->intout[0] = a->item = pb->intin[2];
		}
	}

 	return XAC_DONE;
}

/*
 * Influence behaviour.
 * intin[0]	0:	get
 *         	1:	set
 */
unsigned long
XA_menu_settings(int lock, struct xa_client *client, AESPB *pb)
{
	MN_SET *mn = (MN_SET *)pb->addrin[0];
	CONTROL(1,1,1)

	DIAG((D_menu,client,"menu_settings %d", pb->intin[0]));

	/* accepted, no implementation planned */
	pb->intout[0] = 1;
	switch (pb->intin[0])
	{
		case 0:
		{
			*mn = cfg.menu_settings.mn_set;
			break;
		}
		case 1:
		{
			cfg.menu_settings.mn_set = *mn;
			cfg.popup_timeout = cfg.menu_settings.mn_set.display;
			cfg.popout_timeout = cfg.menu_settings.mn_set.drag;
			break;
		}
	}
	return XAC_DONE;
}
