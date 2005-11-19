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
 * Task Manager Dialog
 * Error message dialog
 * System menu handling
 */

#include RSCHNAME

#include "xa_types.h"
#include "xa_global.h"

#include "about.h"
#include "c_window.h"
#include "form.h"
#include "k_main.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "obtree.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_appl.h"
#include "xa_form.h"
#include "xa_menu.h"
#include "xa_shel.h"
#include "xa_rsrc.h"
#include "xa_fsel.h"
#include "trnfm.h"

#include "mvdi.h"

#include "mint/signal.h"
#include "mint/stat.h"
#include "mint/fcntl.h"

static struct xa_wtxt_inf norm_txt =
{
 WTXT_NOCLIP,
/* id  pnts efx   fgc      bgc */
 {  -1,  -1,   MD_TRANS, 0, G_BLACK, G_WHITE },	/* Normal */
 {  -1,  -1,   MD_TRANS, 0, G_WHITE, G_BLACK },	/* Selected */
 {  -1,  -1,   MD_TRANS, 0, G_BLACK, G_WHITE },	/* Highlighted */

};

static struct xa_wtxt_inf acc_txt =
{
 WTXT_NOCLIP,
/* id  pnts efx   fgc      bgc */
 {  -1,  -1,   MD_TRANS, 0, G_BLUE, G_LBLUE },	/* Normal */
 {  -1,  -1,   MD_TRANS, 0, G_BLUE, G_LBLUE },	/* Selected */
 {  -1,  -1,   MD_TRANS, 0, G_BLACK, G_WHITE },	/* Highlighted */

};

static struct xa_wtxt_inf prg_txt =
{
 WTXT_NOCLIP,
/* id  pnts efx   fgc      bgc */
 {  -1,  -1,   MD_TRANS, 0, G_LRED, G_WHITE },	/* Normal */
 {  -1,  -1,   MD_TRANS, 0, G_LRED, G_WHITE },	/* Selected */
 {  -1,  -1,   MD_TRANS, 0, G_BLACK, G_WHITE },	/* Highlighted */

};

static struct xa_wtxt_inf sys_txt =
{
 WTXT_NOCLIP,
/* id  pnts efx   fgc      bgc */
 {  -1,  -1,   MD_TRANS, 0, G_LBLACK, G_LWHITE },	/* Normal */
 {  -1,  -1,   MD_TRANS, 0, G_LBLACK, G_LWHITE },	/* Selected */
 {  -1,  -1,   MD_TRANS, 0, G_BLACK, G_WHITE },	/* Highlighted */

};

static struct xa_wtxt_inf sys_thrd =
{
 WTXT_NOCLIP,
/* id  pnts efx   fgc      bgc */
 {  -1,  -1,   MD_TRANS, 0, G_LWHITE, G_LWHITE },	/* Normal */
 {  -1,  -1,   MD_TRANS, 0, G_LBLACK, G_LWHITE },	/* Selected */
 {  -1,  -1,   MD_TRANS, 0, G_BLACK, G_WHITE },	/* Highlighted */

};

static struct xa_wtxt_inf desk_txt =
{
 WTXT_NOCLIP,
/* id  pnts efx   fgc      bgc */
 {  -1,  -1,   MD_TRANS, 0, G_RED, G_LWHITE },	/* Normal */
 {  -1,  -1,   MD_TRANS, 0, G_RED, G_LWHITE },	/* Selected */
 {  -1,  -1,   MD_TRANS, 0, G_BLACK, G_WHITE },	/* Highlighted */

};

static char *
build_tasklist_string(struct xa_client *client)
{
	long tx_len = 128;
	char *tx;
 
	tx = kmalloc(tx_len);

	if (tx)
	{
		long prio = p_getpriority(0, client->p->pid);

		if (prio >= 0)
		{
			prio -= 20;

			if (prio < 0 && prio > -10)
				sprintf(tx, tx_len, " %3d/ %2ld%s", client->p->pid, prio, client->name);
			else
				sprintf(tx, tx_len, " %3d/%3ld%s", client->p->pid, prio, client->name);
		}
		else
			sprintf(tx, tx_len, " %3d/    %s", client->p->pid, 0, client->name);
	}
	return tx;
};

void
add_to_tasklist(struct xa_client *client)
{
	OBJECT *obtree = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
	OBJECT *icon;
	char *tx;
	struct scroll_content sc = {{ 0 }};

	if (client->p->pid == C.DSKpid)
		sc.fnt = &desk_txt;
	else if (client->type & APP_ACCESSORY)
		sc.fnt = &acc_txt;
	else if (client->type & APP_SYSTEM)
		sc.fnt = &sys_txt;
	else if (client->type & APP_AESTHREAD)
		sc.fnt = &sys_thrd;
	else if (client->type & APP_APPLICATION)
		sc.fnt = &prg_txt;
	else
		sc.fnt = &norm_txt;

	if (!list)
		return;

	if (client->type & APP_ACCESSORY)
		icon = obtree + TM_ICN_MENU;
	else
		icon = obtree + TM_ICN_XAAES;

	if ((tx = build_tasklist_string(client)))
	{
		sc.icon = icon;
		sc.t.text = tx;
		sc.t.strings = 1;
		sc.data = client;
		list->add(list, NULL, NULL, &sc, false, SETYP_MAL, true);
	}
	else
	{
		sc.icon = icon;
		sc.t.text = client->name;
		sc.t.strings = 1;
		sc.data = client;
		list->add(list, NULL, NULL, &sc, false, 0, true);
	}
}
void
remove_from_tasklist(struct xa_client *client)
{
	OBJECT *obtree = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
	struct sesetget_params p = { 0 };

	if (list)
	{			
		p.arg.data = client;
		list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
		if (p.e)
			list->del(list, p.e, true);
	}
}
void
update_tasklist_entry(struct xa_client *client)
{
	OBJECT *obtree = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
	struct sesetget_params p = { 0 };
	
	if (list)
	{
		char *tx;
		p.arg.data = client;
		list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
		if (p.e)
		{
			struct setcontent_text t = { 0 }; //sc_text t;

			if ((tx = build_tasklist_string(client)))
				t.text = tx;
			else
				t.text = client->name;
			
// 			t.index = 0;
			list->set(list, p.e, SESET_TEXT, (long)&t, true);
			
			if (tx)
				kfree(tx);
		}
	}
}

static struct xa_window *task_man_win = NULL;

bool
isin_namelist(struct cfg_name_list *list, char *name, short nlen, struct cfg_name_list **last, struct cfg_name_list **prev)
{
	bool ret = false;

	if (!nlen)
		nlen = strlen(name);

	DIAGS(("isin_namelist: find '%s'(len=%d) in list=%lx (name='%s', len=%d)",
		name, nlen, list,
		list ? list->name : "noname",
		list ? list->nlen : -1));

	if (last)
		*last = NULL;
	if (prev)
		*prev = NULL;
	
	while (list)
	{
		DIAGS((" -- checking list=%lx, name=(%d)'%s'",
			list, list->nlen, list->name));

		if (list->nlen == nlen && !strnicmp(list->name, name, nlen))
		{
			ret = true;
			break;
		}
		if (prev)
			*prev = list;
		
		list = list->next;
	}

	if (last)
		*last = list;
	
	DIAGS((" -- ret=%s, last=%lx, prev=%lx",
		ret ? "true":"false", last, prev));

	return ret;
}
			
void
addto_namelist(struct cfg_name_list **list, char *name)
{
	struct cfg_name_list *new, *prev;
	short nlen = strlen(name);

	DIAGS(("addto_namelist: add '%s' to list=%lx(%lx)", name, *list, list));
	
	if (nlen && !isin_namelist(*list, name, 0, NULL, &prev))
	{
		new = kmalloc(sizeof(*new));
	
		if (new)
		{
			if (nlen > 32)
				nlen = 32;

			bzero(new, sizeof(*new));

			if (prev)
			{
				DIAGS((" -- add new=%lx to prev=%lx", new, prev));
				prev->next = new;
			}
			else
			{
				DIAGS((" -- add first=%lx to start=%lx", new, list));
				*list = new;
			}
			strcpy(new->name, name);
			new->nlen = nlen;
		}
	}
}

void
removefrom_namelist(struct cfg_name_list **list, char *name, short nlen)
{
	struct cfg_name_list *this, *prev;

	if (isin_namelist(*list, name, nlen, &this, &prev))
	{
		if (prev)
			prev->next = this->next;
		else
			*list = this->next;
		kfree(this);
	}
}

void
free_namelist(struct cfg_name_list **list)
{
	while (*list)
	{
		struct cfg_name_list *l = *list;
		*list = (*list)->next;
		DIAGS(("free_namelist: freeing %lx, next=%lx(%lx) name='%s'",
			l, *list, list, l->name));
		kfree(l);
	}
}

static int
taskmanager_destructor(enum locks lock, struct xa_window *wind)
{
	task_man_win = NULL;
	return true;
}

static void
send_terminate(enum locks lock, struct xa_client *client, short reason)
{
	if (client->type & APP_ACCESSORY)
	{
		DIAGS(("send AC_CLOSE to %s", c_owner(client)));

		/* Due to ambiguities in documentation the pid is filled out
		 * in both msg[3] and msg[4]
		 */
		send_app_message(lock, NULL, client, AMQ_CRITICAL, QMF_CHKDUP,
				 AC_CLOSE,    0, 0, client->p->pid,
				 client->p->pid, 0, 0, 0);
	}

	/* XXX
	 * should we only send if client->swm_newmsg & NM_APTERM is true???
	 */
	DIAGS(("send AP_TERM to %s", c_owner(client)));
	send_app_message(lock, NULL, client, AMQ_CRITICAL, QMF_CHKDUP,
			 AP_TERM,     0,       0, client->p->pid,
			 client->p->pid, reason/*AP_TERM*/, 0, 0);
}

void
quit_all_apps(enum locks lock, struct xa_client *except, short reason)
{
	struct xa_client *client;

	Sema_Up(clients);
	lock |= clients;

	FOREACH_CLIENT(client)
	{
		if (is_client(client) && client != C.Hlp && client != except)
		{
			DIAGS(("shutting down %s", c_owner(client)));
			send_terminate(lock, client, reason);
		}
	}

	Sema_Dn(clients);
}

void
quit_all_clients(enum locks lock, struct cfg_name_list *except_nl, struct xa_client *except_cl, short reason)
{
	struct xa_client *client, *dsk = NULL;

	Sema_Up(clients);
	lock |= clients;

	DIAGS(("quit_all_clients: name_list=%lx, except_client=%lx", except_nl, except_cl));
	/*
	 * '_aes_shell' is special. If it is defined, we lookup the pid of
	 * the shell (desktop) loaded by the AES and let it continue to run
	 */
	if (isin_namelist(except_nl, "_aes_shell_", 11, NULL, NULL))
	{
		dsk = pid2client(C.DSKpid);
		DIAGS((" -- _aes_shell_ defined: pid=%d, client=%lx, name=%s",
			C.DSKpid, dsk, dsk ? dsk->name : "no shell loaded"));
	}
	
	FOREACH_CLIENT(client)
	{
		if (is_client(client) &&
		    client != except_cl &&
		    client != dsk &&
		    !isin_namelist(except_nl, client->proc_name, 8, NULL, NULL))
		{
			DIAGS(("Shutting down %s", c_owner(client)));
			send_terminate(lock, client, reason);
		}
	}
	Sema_Dn(clients);
}

static void
taskmanager_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	struct xa_vdi_settings *v = wind->vdi_settings;
	short obj = fr->obj;
	
	Sema_Up(clients);
	lock |= clients;
	
	wt->current = fr->obj;
	wt->which = 0;

	switch (fr->obj)
	{
		case TM_LIST:
		{
// 			short obj = fr->obj;
			OBJECT *obtree = wt->tree;

			DIAGS(("taskmanager_form_exit: Moved the shit out of form_do() to here!"));
			if ( fr->md && ((obtree[obj].ob_type & 0xff) == G_SLIST))
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, obtree, obj, fr->md);
				else
					click_scroll_list(lock, obtree, obj, fr->md);
			}
			break;
		}
		case TM_KILL:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = NULL; //= cur_client(list);

			if (list->cur)
				client = list->cur->data;
			DIAGS(("taskmanager: KILL for %s", c_owner(client)));
			if (client && is_client(client))
			{
				if (client->type & (APP_AESTHREAD|APP_AESSYS))
				{
					ALERT(("Not a good idea, I tell you!"));
				}
				else
					ikill(client->p->pid, SIGKILL);
			}

			object_deselect(wt->tree + TM_KILL);
			redraw_toolbar(lock, task_man_win, TM_KILL);
			break;
		}
		case TM_TERM:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = NULL; //cur_client(list);

			DIAGS(("taskmanager: TM_TERM for %s", c_owner(client)));

			if (list->cur)
				client = list->cur->data;

			if (client && is_client(client))
			{
				if (client->type & (APP_AESTHREAD|APP_AESSYS))
				{
					ALERT(("Cannot terminate AES system proccesses!"));
				}
				else
					send_terminate(lock, client, AP_TERM);
			}

			object_deselect(wt->tree + TM_TERM);
			redraw_toolbar(lock, task_man_win, TM_TERM);
			break;
		}
		case TM_SLEEP:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = NULL; //cur_client(list);

			if (list->cur)
				client = list->cur->data;

			DIAGS(("taskmanager: TM_SLEEP for %s", c_owner(client)));
			ALERT(("taskmanager: TM_SLEEP not yet implemented!"));
			if (client && is_client(client))
			{
			}

			object_deselect(wt->tree + TM_SLEEP);
			redraw_toolbar(lock, task_man_win, TM_SLEEP);
			break;
		}
		case TM_WAKE:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = NULL; //cur_client(list);

			if (list->cur)
				client = list->cur->data;

			DIAGS(("taskmanager: TM_WAKE for %s", c_owner(client)));
			ALERT(("taskmanager: TM_WAKE not yet implemented!"));
			if (client && is_client(client))
			{
			}

			object_deselect(wt->tree + TM_WAKE);
			redraw_toolbar(lock, task_man_win, TM_WAKE);
			break;
		}

		/* the bottom action buttons */
		case TM_QUITAPPS:
		{
			DIAGS(("taskmanager: quit all apps"));
			quit_all_apps(lock, NULL, AP_TERM);

			object_deselect(wt->tree + TM_QUITAPPS);
			redraw_toolbar(lock, task_man_win, TM_QUITAPPS);
			break;
		}
		case TM_QUIT:
		{
			DIAGS(("taskmanager: quit XaAES"));
			dispatch_shutdown(0);

			object_deselect(wt->tree + TM_QUIT);
			redraw_toolbar(lock, task_man_win, TM_QUIT);
			break;
		}
		case TM_REBOOT:
		{
			DIAGS(("taskmanager: reboot system"));
			dispatch_shutdown(REBOOT_SYSTEM);

			object_deselect(wt->tree + TM_REBOOT);
			redraw_toolbar(lock, task_man_win, TM_REBOOT);
			break;
		}
		case TM_HALT:
		{
			DIAGS(("taskmanager: halt system"));
			dispatch_shutdown(HALT_SYSTEM);

			object_deselect(wt->tree + TM_HALT);
			redraw_toolbar(lock, task_man_win, TM_HALT);
			break;
		}
		case TM_COLD:
		{
			DIAGS(("taskmanager: coldstart system"));
			dispatch_shutdown(COLDSTART_SYSTEM);
			object_deselect(wt->tree + TM_COLD);
			redraw_toolbar(lock, task_man_win, TM_COLD);
			break;
		}
		case TM_RESCHG:
		{
			
			if (C.reschange)
				post_cevent(C.Hlp, ceExecfunc, C.reschange,NULL, 0,0, NULL,NULL);
			obj_change(wt, v, obj, -1, wt->tree[obj].ob_state & ~OS_SELECTED, wt->tree[obj].ob_flags, true, NULL, wind->rect_list.start, 0);
			break;
		}
		case TM_OK:
		{
			object_deselect(wt->tree + TM_OK);
			redraw_toolbar(lock, task_man_win, TM_OK);

			/* and release */
			close_window(lock, task_man_win);
			delayed_delete_window(lock, task_man_win);
			break;
		}
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", wt->current));
			break;
		}
	}
	Sema_Dn(clients);
}

void
open_taskmanager(enum locks lock, struct xa_client *client)
{
	RECT remember = { 0,0,0,0 };
	struct xa_window *wind;
	XA_TREE *wt;
	OBJECT *obtree = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	RECT or;

	ob_rectangle(obtree, 0, &or);

	wt = obtree_to_wt(client, obtree);

	if (!task_man_win)
	{
		obtree[TM_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or); //form_center(obtree, ICON_H);
			remember = calc_window(lock, client, WC_BORDER,
						CLOSER|NAME, created_for_AES,
						client->options.thinframe,
						client->options.thinwork,
						*(RECT *)&or); //*(RECT*)&obtree->ob_x);
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag, do_formwind_msg,
					client,
					false, /*false,*/
					CLOSER|NAME|TOOLBAR|hide_move(&(client->options)),
					created_for_AES,
					client->options.thinframe,
					client->options.thinwork,
					remember, NULL, NULL);

		/* Set the window title */
		set_window_title(wind, " Task Manager ", false);

		wt = set_toolbar_widget(lock, wind, client, obtree, -1, 0/*WIP_NOTEXT*/, true, NULL, &or);
		wt->exit_form = taskmanager_form_exit;

		/* Set the window destructor */
		wind->destructor = taskmanager_destructor;
	
		/* better position (to get sliders correct initially) */
// 		refresh_tasklist(lock);
#if 0		
		{
			struct xa_client *cl;
			FOREACH_CLIENT(cl)
			{
				if (cl != C.Hlp)
					add_to_tasklist(cl);
			}
		}
#endif
		open_window(lock, wind, wind->r);
		task_man_win = wind;
	}
	else if (task_man_win != window_list)
	{
		top_window(lock, true, false, task_man_win, (void *)-1L);
	}
}

/* ************************************************************ */
/*     Common resolution mode change functions/stuff		*/
/* ************************************************************ */
static struct xa_window *reschg_win = NULL;
static int
reschg_destructor(enum locks lock, struct xa_window *wind)
{
	reschg_win = NULL;
	return true;
}

static struct xa_window *
create_dwind(enum locks lock, XA_WIND_ATTR tp, char *title, struct xa_client *client, struct widget_tree *wt, FormExit(*f), WindowDisplay(*d))
{
	struct xa_window *wind;
	OBJECT *obtree = wt->tree;
        RECT r, or;

	ob_rectangle(obtree, 0, &or);

	center_rect(&or);

	r = calc_window(lock, client, WC_BORDER,
			CLOSER|NAME, created_for_AES,
			client->options.thinframe,
			client->options.thinwork,
			*(RECT *)&or);

	/* Create the window */
	wind = create_window(lock,
				do_winmesag, do_formwind_msg,
				client,
				false,
				tp | (title ? NAME : 0) | TOOLBAR | hide_move(&(client->options)),
				created_for_AES,
				client->options.thinframe,
				client->options.thinwork,
				r, NULL, NULL);

	/* Set the window title */
	if (title)
		set_window_title(wind, title, false);

	wt = set_toolbar_widget(lock, wind, client, obtree, -1, 0/*WIP_NOTEXT*/, true, NULL, &or);
	wt->exit_form = f; //milan_reschg_form_exit;

	/* Set the window destructor */
	wind->destructor = d; //reschg_destructor;

	return wind;
}

static void
setsel(OBJECT *obtree, short obj, bool sel)
{
	short state;

	state = obtree[obj].ob_state & ~OS_SELECTED;
	if (sel)
		state |= OS_SELECTED;
	obtree[obj].ob_state = state;
}
/* ************************************************************ */
/*     Atari resolution mode change functions			*/
/* ************************************************************ */
static void
set_resmode_obj(XA_TREE *wt, short res)
{
	short obj;
	struct xa_vdi_settings *v = wt->owner->vdi_settings;

	if (res < 1)
		res = 1;
	if (res > 10)
		res = 9;

	obj = RC_MODES + res;
	obj_set_radio_button(wt, v, obj, false, NULL, NULL);
}

static short
get_resmode_obj(XA_TREE *wt)
{
	short obj;

	obj = obj_get_radio_button(wt, RC_MODES, OS_SELECTED);

	if (obj > 0)
		obj -= RC_MODES;
	else
		obj = 1;

	return obj;
}
	
static void
resmode_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	Sema_Up(clients);
	lock |= clients;
	
	wt->current = fr->obj;
	wt->which = 0;

	switch (fr->obj)
	{
		case RC_OK:
		{
			DIAGS(("reschange: restart"));

			object_deselect(wt->tree + RC_OK);
			redraw_toolbar(lock, wind, RC_OK);
			next_res = get_resmode_obj(wt);
			next_res |= 0x80000000;
			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			dispatch_shutdown(RESOLUTION_CHANGE/*0*/);
			break;
		}
		case RC_CANCEL:
		{
			object_deselect(wt->tree + RC_CANCEL);
			redraw_toolbar(lock, wind, RC_CANCEL);

			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			break;
		}
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", wt->current));
			break;
		}
	}
	Sema_Dn(clients);
}

static char t_reschg[] = " Change Resolution ";

void
open_reschange(enum locks lock, struct xa_client *client)
{
	struct xa_window *wind;
	XA_TREE *wt;
	OBJECT *obtree;

	if (!reschg_win)
	{
		obtree = ResourceTree(C.Aes_rsc, RES_CHATARI);
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		if (wt)
		{
			wind = create_dwind(lock, CLOSER, t_reschg, client, wt, resmode_form_exit, reschg_destructor);
			if (wind)
			{
				set_resmode_obj(wt, cfg.videomode);
				open_window(lock, wind, wind->r);
				reschg_win = wind;
			}
		}
	}
	else if (reschg_win != window_list)
	{
		top_window(lock, true, false, reschg_win, (void *)-1L);
	}
}
/* ************************************************************ */
/*     Falcon resolution mode change functions			*/
/* ************************************************************ */
static void
set_reschg_obj(XA_TREE *wt, unsigned long res)
{
	OBJECT *obtree = wt->tree;
	short obj;
	struct xa_vdi_settings *v = wt->owner->vdi_settings;

	obj = res & 0x7;
	if (obj > 4)
		obj = 4;
	obj += RC_COLOURS + 1;
	obj_set_radio_button(wt, v, obj, false, NULL, NULL);

	obj = RC_COLUMNS + 1 + ((res & (1<<3)) ? 1 : 0);
	obj_set_radio_button(wt, v, obj, false, NULL, NULL);

	obj = RC_VGA + 1 + ((res & (1<<4)) ? 1 : 0);
	obj_set_radio_button(wt, v, obj, false, NULL, NULL);

	obj = RC_TVSEL + 1 + ((res & (1<<5)) ? 1 : 0);
	obj_set_radio_button(wt, v, obj, false, NULL, NULL);

	setsel(obtree, RC_OVERSCAN, (res & (1<<6)));
	setsel(obtree, RC_ILACE, (res & (1<<7)));
	setsel(obtree, RC_BIT15, (res & 0x8000));

// 	ob_set_children_sf(obtree, RC_MODES, ~(OS_SELECTED|OS_DISABLED), OS_DISABLED, -1, 0, true);
}

inline static bool
issel(OBJECT *obtree, short obj)
{	return (obtree[obj].ob_state & OS_SELECTED); }

static unsigned long
get_reschg_obj(XA_TREE *wt)
{
	OBJECT *obtree = wt->tree;
	short obj;
	unsigned long res = 0L;

	if ((obj = obj_get_radio_button(wt, RC_COLOURS, OS_SELECTED)) != -1)
		res |= obj - 1 - RC_COLOURS;
	if ((obj = obj_get_radio_button(wt, RC_COLUMNS, OS_SELECTED)) != -1)
		res |= (obj - 1 - RC_COLUMNS) << 3;
	if ((obj = obj_get_radio_button(wt, RC_VGA, OS_SELECTED)) != -1)
		res |= (obj - 1 - RC_VGA) << 4;
	if ((obj = obj_get_radio_button(wt, RC_TVSEL, OS_SELECTED)) != -1)
		res |= (obj - 1 - RC_TVSEL) << 5;

	if (issel(obtree, RC_OVERSCAN)) res |= (1<<6);
	if (issel(obtree, RC_ILACE))	res |= (1<<7);
	if (issel(obtree, RC_BIT15))	res |= 0x8000;

	return res;
}

static void
reschg_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	Sema_Up(clients);
	lock |= clients;
	
	wt->current = fr->obj;
	wt->which = 0;

	switch (fr->obj)
	{
		case RC_OK:
		{
			DIAGS(("reschange: restart"));

			object_deselect(wt->tree + RC_OK);
			redraw_toolbar(lock, wind, RC_OK);
			next_res = get_reschg_obj(wt);
			next_res |= 0x80000000;
			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			dispatch_shutdown(RESOLUTION_CHANGE/*0*/);
			break;
		}
		case RC_CANCEL:
		{
			object_deselect(wt->tree + RC_CANCEL);
			redraw_toolbar(lock, wind, RC_CANCEL);

			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			break;
		}
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", wt->current));
			break;
		}
	}

	Sema_Dn(clients);
}

void
open_falcon_reschange(enum locks lock, struct xa_client *client)
{
	struct xa_window *wind;
	XA_TREE *wt;
	OBJECT *obtree;

	if (!reschg_win)
	{
		obtree = ResourceTree(C.Aes_rsc, RES_CHFALC);
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		if (wt)
		{
			wind = create_dwind(lock, CLOSER, t_reschg, client, wt, reschg_form_exit, reschg_destructor);
			if (wind)
			{
				set_reschg_obj(wt, (unsigned long)cfg.videomode);
				open_window(lock, wind, wind->r);
				reschg_win = wind;
			}
		}
	}
	else if (reschg_win != window_list)
	{
		top_window(lock, true, false, reschg_win, (void *)-1L);
	}
}
/* ************************************************************ */
/*     Milan resolution mode change functions			*/
/* ************************************************************ */
static char *coldepths[] =
{
	"Monochrome",
	"4 colors",
	"16 colors",
	"256 colors",
	"15 bpp (32K)",
	"16 bpp (64K)",
	"24 bpp (16M)",
	"32 bpp (16M)",
};

static char whatthehell[] = "What the hell!";

struct resinf
{
	short id;
	short x;
	short y;
};

struct milres_parm
{
	struct xa_data_hdr h;
	
	void *modes;
	short curr_devid;
	
	short current[2];
	long misc[4];
	short count[8];
	
	short num_depths;
	struct widget_tree *depth_wt;
	struct widget_tree *col_wt[8];
	struct resinf *resinf[8];
// 	short *devids[8];

	POPINFO pinf_depth;
	POPINFO pinf_res;
};
static void
milan_reschg_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	short new_devid = -1;
	struct milres_parm *p = lookup_xa_data_byname(&wind->xa_data, "milres_parm");

	Sema_Up(clients);
	lock |= clients;
	
	wt->current = fr->obj;
	wt->which = 0;

	switch (fr->obj)
	{
		case RCHM_COL:
		{
			int i, o, newres = 1;
			POPINFO *pinf = object_get_popinfo(wt->tree + fr->obj);
			struct widget_tree *pu_wt = NULL;

			for (i = 0, o = pinf->obnum; i < 8 && o >= 0; i++)
			{
// 				display("o = %d, i = %d, colwt = %lx", o, i, p->col_wt[i]);
				if (p->col_wt[i])
				{
					o--;
					if (!o)
					{
						int j;
						struct resinf *old, *new = NULL;
						
						old = p->resinf[p->current[0]];
						for (j = 0; j < p->count[p->current[0]]; j++, old++)
						{
							if (old->id == p->current[1])
							{
								new = old;
								break;
							}
						}
						if (new)
						{
							old = new;
						
							new = p->resinf[i];
							for (j = 0; j < p->count[i]; j++)
							{
// 								display("check %x (%d/%d) against %x (%d/%d)",
// 									new[j].id, new[j].x, new[j].y,
// 									old->id, old->x, old->y);

								if (new[j].x == old->x && new[j].y == old->y)
								{
									newres = j + 1;
									break;
								}
							}
						}
						
						pu_wt = p->col_wt[i];
// 						new_devid = *p->devids[i];
						new_devid = new[newres - 1].id; //(*p->resinf[i]).id;
						
						p->current[0] = i;
						break;
					}
				}
			}
			if (pu_wt)
			{
				pinf = &p->pinf_res;
				pinf->tree = pu_wt->tree;
				pinf->obnum = newres;
				obj_set_g_popup(wt, RCHM_RES, pinf);
				obj_draw(wt, wind->vdi_settings, RCHM_RES, -1, NULL, wind->rect_list.start, 0);
// 				display("new devid = %x", new_devid);
				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_RES:
		{
			POPINFO *pinf = object_get_popinfo(wt->tree + fr->obj);
			struct widget_tree *pu_wt = p->col_wt[p->current[0]];

			if (pu_wt)
			{
				struct resinf *r = p->resinf[p->current[0]];
				
				new_devid = r[pinf->obnum - 1].id;

// 				new_devid = *(p->devids[p->current[0]] + (pinf->obnum - 1));
				p->current[1] = new_devid;
			}
// 			display("new devid = %x", new_devid);
			break;
		}
#if 1
		case RCHM_OK:
		{
			DIAGS(("reschange: restart"));

			object_deselect(wt->tree + RC_OK);
			redraw_toolbar(lock, wind, RC_OK);
			next_res = p->current[1]; //get_reschg_obj(wt);
			next_res |= 0x80000000;
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			dispatch_shutdown(RESOLUTION_CHANGE/*0*/);
			break;
		}
		case RCHM_CANCEL:
		{
			object_deselect(wt->tree + RC_CANCEL);
			redraw_toolbar(lock, wind, RC_CANCEL);

			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			break;
		}
#endif
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", wt->current));
			break;
		}
	}

	Sema_Dn(clients);
}

static short
count_milan_res(long num_modes, short planes, struct videodef *modes)
{
	short count = 0;
// 	bool p = false;

	while (num_modes--)
	{
		if (modes->planes == planes)
			count++;
#if 0
		if (count == 1 && !p)
		{
			short i, *m;
			display("Planes %d", planes);
			m = (short *)modes;
			for (i = (66/2); i < (106/2); i++)
				display("%02d, %04x, %05d", i, m[i], m[i]);
			p = true;
		}
#endif
		modes++;
	}
	return count;
}

static void *
nxt_mdepth(short item, void **data)
{
	int i;
	struct milres_parm *p = *data;
	short current = -1;
	void *ret;

	if (!item)
	{
		p->current[1] = p->current[0];
	}
	
	for (i = p->current[1]; i < 8; i++)
	{
		if (p->count[i])
		{
			current = i;
			break;
		}
	}
	if (current == -1)
	{
		return whatthehell;
	}
	else
	{	
		ret = coldepths[current];
		p->current[1] = current + 1;
	}
	
	return ret;
};
static char idx2planes[] =
{
	1,2,4,8,15,16,24,32
};

static void *
nxt_mres(short item, void **data)
{
	struct milres_parm *p = *data;
	struct videodef *modes;
	short planes;
	long num_modes;
	void *ret = NULL;

	if (!item)
	{
		p->misc[2] = p->misc[0];
		p->misc[3] = p->misc[1];
		p->current[1] = p->current[0];
	}

	planes = idx2planes[p->current[1]];
// 	ndisplay("nxt_mres: planes = %d", planes);

	(long)modes = p->misc[2];
	num_modes = p->misc[3];

	while (num_modes > 0)
	{
		if (modes->planes == planes)
		{
			struct resinf *r = p->resinf[p->current[1]];
			
			(struct milres_parm *)p->misc[2] = modes + 1;
			p->misc[3] = num_modes - 1;
			r[item].id = modes->devid;
			r[item].x  = modes->res_x;
			r[item].y  = modes->res_y;
			ret = modes->name;
#if 0
			(struct milres_parm *)p->misc[2] = modes + 1;
			p->misc[3] = num_modes - 1;
			*(p->devids[p->current[1]] + item) = modes->devid;
			ret = modes->name;
#endif
// 			ndisplay(", name '%s' - %x", ret, modes->devid);
			break;
		}
		num_modes--;
		modes++;
	}
// 	display(" ..return %lx (%s)", ret, ret);
	return ret;
}

static int
instchrm_wt(struct xa_client *client, struct widget_tree **wt, OBJECT *obtree)
{
	int ret = 0;
	
	if (obtree)
	{
		*wt = new_widget_tree(client, obtree);
		if (*wt)
		{
// 			display("new wt = %lx", *wt);
			(*wt)->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
			ret = 1;
		}
		else
		{
// 			display("no new wt!!");
			free_object_tree(C.Aes, obtree);
		}
	}
	else
		*wt = NULL;

// 	display("return %d", ret);
	return ret;
}

static void _cdecl
delete_milres_parm(void *_p)
{
	struct milres_parm *p = _p;
	int i;

	if (p)
	{
		if (p->depth_wt)
		{
			remove_attachments(0, p->depth_wt->owner, p->depth_wt);
			p->depth_wt->links--;
			remove_wt(p->depth_wt, false);
			p->depth_wt = NULL;
		}
		for (i = 0; i < 8; i++)
		{
			if (p->col_wt[i])
			{
				p->col_wt[i]->links--;
				remove_wt(p->col_wt[i], false);
				p->col_wt[i] = NULL;
			}
		}
		kfree(p);
	}
}

static struct milres_parm *
check_milan_res(struct xa_client *client, short mw)
{
	int i, j;
	short currmode = 0;
	struct videodef *modes;
	struct milres_parm *p = NULL;
	long num_modes;

	num_modes = mvdi_device(0, 0, DEVICE_GETDEVICE, (long *)&modes);
	
	if (num_modes >= 0)
	{
		currmode = modes->devid;
	}
	
	num_modes = mvdi_device(0, 0, DEVICE_GETDEVICELIST, (long *)&modes);

	if (num_modes > 0)
	{
		short depths = 0, devids = 0;
		OBJECT *obtree;
		short count[8];

		count[0] = count_milan_res(num_modes,  1, modes);
		count[1] = count_milan_res(num_modes,  2, modes);
		count[2] = count_milan_res(num_modes,  4, modes);
		count[3] = count_milan_res(num_modes,  8, modes);
		count[4] = count_milan_res(num_modes, 15, modes);
		count[5] = count_milan_res(num_modes, 16, modes);
		count[6] = count_milan_res(num_modes, 24, modes);
		count[7] = count_milan_res(num_modes, 32, modes);
	
		for (i = 0; i < 8; i++)
		{
			if (count[i])
			{	
				devids += count[i] + 1;
				depths++;
			}
		}
		
		if (depths)
		{
			struct resinf *r;
// 			short *di;
			
			if (!(p = kmalloc(sizeof(*p) + (sizeof(*r) * devids)))) //if (!(p = kmalloc(sizeof(*p) + (devids << 1))))
				goto exit;

			bzero(p, sizeof(*p));

			p->curr_devid = currmode;
// 			(long)di = (long)p + sizeof(*p);
			(long)r = (long)p + sizeof(*p);

			for (i = 0; i < 8; i++)
			{
				if ((p->count[i] = count[i]))
				{
					p->resinf[i] = r;
					r += p->count[i];
				}
			}
#if 0			
			for (i = 0; i < 8; i++)
			{
				p->count[i] = count[i];

				if (p->count[i])
				{
					p->devids[i] = di;
					di += p->count[i];
				}
			}
#endif
			p->num_depths = depths;
			p->current[0] = 0;
// 			display("color depths %d", depths);
			obtree = create_popup_tree(client, 0, depths, mw, 4, &nxt_mdepth, (void **)&p);
			if (!instchrm_wt(client, &p->depth_wt, obtree))
				goto exit;
			
			p->depth_wt->links++;

// 			display("1");
			for (i = 0,j = 1; i < 8; i++)
			{
				if (p->count[i])
				{
					p->current[0] = i;
					p->misc[0] = (long)modes;
					p->misc[1] = num_modes;
					obtree = create_popup_tree(client, 0, p->count[i], mw, 4, &nxt_mres, (void **)&p);
					if (instchrm_wt(client, &p->col_wt[i], obtree))
					{
						p->col_wt[i]->links++;
					}
					else
						goto exit;
					j++;
// 					display("2 - %d %d", i, j);
				}
			}
// 			display("3");
		}
	}
	else
	{
exit:
		delete_milres_parm(p);
		p = NULL;
	}	
	return p;
}

static short
milan_setdevid(struct widget_tree *wt, struct milres_parm *p, short devid)
{
	int i, j, depth_idx = 0, res_idx = -1;
	short found_devid = 0, current;
	struct widget_tree *first = NULL, *pu_wt = NULL;

	for (i = 0; i < 8; i++)
	{
		if (p->col_wt[i])
		{
			if (!first)
			{
				first = p->col_wt[i];
// 				found_devid = *p->devids[i];
				found_devid = (*p->resinf[i]).id;
				current = i;
			}
			
			for (j = 0; j < p->count[i]; j++)
			{
				struct resinf *r = p->resinf[i];
				
				if (r[j].id == devid)
				{
					pu_wt = p->col_wt[i];
					res_idx = j + 1;
					found_devid = devid;
					current = i;
				}
			#if 0
				if (*(p->devids[i] + j) == devid)
				{
					pu_wt = p->col_wt[i];
					res_idx = j + 1;
					found_devid = devid;
					current = i;
					break;
				}
			#endif
			}
			if (res_idx != -1)
				break;
			
			depth_idx++;
		}
	}
	p->pinf_depth.tree = p->depth_wt->tree;
	p->current[0] = i;
	if (res_idx != -1)
	{
		p->pinf_depth.obnum = depth_idx + 1;
		p->pinf_res.tree = pu_wt->tree;
		p->pinf_res.obnum = res_idx;
	}
	else
	{
		p->pinf_depth.obnum = 1;
		p->pinf_res.tree = first->tree;
		p->pinf_res.obnum = 1;
	}
	obj_set_g_popup(wt, RCHM_COL, &p->pinf_depth);
	obj_set_g_popup(wt, RCHM_RES, &p->pinf_res);
// 	display("init to devid %x", found_devid);
	return found_devid;
}

void
open_milan_reschange(enum locks lock, struct xa_client *client)
{
	struct xa_window *wind;
	XA_TREE *wt;
	OBJECT *obtree;
	struct milres_parm *p;

	if (!reschg_win)
	{
		obtree = ResourceTree(C.Aes_rsc, RES_CHMIL);
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);

		if (wt && (p = check_milan_res(client, obtree[RCHM_RES].ob_width)))
		{

			p->current[1] = milan_setdevid(wt, p, p->curr_devid);
			
			wind = create_dwind(lock, CLOSER, t_reschg, client, wt, milan_reschg_form_exit, reschg_destructor);
			if (wind)
			{
				add_xa_data(&wind->xa_data, p, "milres_parm", delete_milres_parm);
				open_window(lock, wind, wind->r);
				reschg_win = wind;
			}
			else
				goto failure;
		}
	}
	else if (reschg_win != window_list)
	{
		top_window(lock, true, false, reschg_win, (void *)-1L);
	}
	return;
failure:
	delete_milres_parm(p);
}
/* ************************************************************ */
/*     Nova resolution mode change functions			*/
/* ************************************************************ */
static char fn_novabib[] = "c:\\auto\\sta_vdi.bib\0";
static void
nova_reschg_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	short new_devid = -1;
	struct milres_parm *p = lookup_xa_data_byname(&wind->xa_data, "milres_parm");

	Sema_Up(clients);
	lock |= clients;
	
	wt->current = fr->obj;
	wt->which = 0;

	switch (fr->obj)
	{
		case RCHM_COL:
		{
			int i, o, newres = 1;
			POPINFO *pinf = object_get_popinfo(wt->tree + fr->obj);
			struct widget_tree *pu_wt = NULL;

			for (i = 0, o = pinf->obnum; i < 8 && o >= 0; i++)
			{
// 				display("o = %d, i = %d, colwt = %lx", o, i, p->col_wt[i]);
				if (p->col_wt[i])
				{
					o--;
					if (!o)
					{
						int j;
						struct resinf *old, *new = NULL;
						
						old = p->resinf[p->current[0]];
						for (j = 0; j < p->count[p->current[0]]; j++, old++)
						{
							if (old->id == p->current[1])
							{
								new = old;
								break;
							}
						}
						if (new)
						{
							old = new;
						
							new = p->resinf[i];
							for (j = 0; j < p->count[i]; j++)
							{
// 								display("check %x (%d/%d) against %x (%d/%d)",
// 									new[j].id, new[j].x, new[j].y,
// 									old->id, old->x, old->y);

								if (new[j].x == old->x && new[j].y == old->y)
								{
									newres = j + 1;
									break;
								}
							}
						}
						
						pu_wt = p->col_wt[i];
// 						new_devid = *p->devids[i];
						new_devid = new[newres - 1].id; //(*p->resinf[i]).id;
						
						p->current[0] = i;
						break;
					}
				}
			}
			if (pu_wt)
			{
				pinf = &p->pinf_res;
				pinf->tree = pu_wt->tree;
				pinf->obnum = newres;
				obj_set_g_popup(wt, RCHM_RES, pinf);
				obj_draw(wt, wind->vdi_settings, RCHM_RES, -1, NULL, wind->rect_list.start, 0);
// 				display("new devid = %x", new_devid);
				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_RES:
		{
			POPINFO *pinf = object_get_popinfo(wt->tree + fr->obj);
			struct widget_tree *pu_wt = p->col_wt[p->current[0]];

			if (pu_wt)
			{
				struct resinf *r = p->resinf[p->current[0]];
				
				new_devid = r[pinf->obnum - 1].id;
				p->current[1] = new_devid;
			#if 0
				new_devid = *(p->devids[p->current[0]] + (pinf->obnum - 1));
				p->current[1] = new_devid;
			#endif
			}
// 			display("new devid = %x", new_devid);
			break;
		}
#if 1
		case RCHM_OK:
		{
			DIAGS(("reschange: restart"));

			object_deselect(wt->tree + RC_OK);
			redraw_toolbar(lock, wind, RC_OK);
			next_res = p->current[1]; //get_reschg_obj(wt);
			next_res |= 0x80000000;
			nova_data->next_res = ((struct nova_res *)p->modes)[p->current[1]];
			nova_data->valid = true;
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			kfree(p->modes);
			dispatch_shutdown(RESOLUTION_CHANGE/*0*/);
			break;
		}
		case RCHM_CANCEL:
		{
			object_deselect(wt->tree + RC_CANCEL);
			redraw_toolbar(lock, wind, RC_CANCEL);

			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			kfree(p->modes);
			break;
		}
#endif
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", wt->current));
			break;
		}
	}

	Sema_Dn(clients);
}

static void *
nxt_novares(short item, void **data)
{
	struct milres_parm *p = *data;
	struct nova_res *modes;
	short planes;
	long num_modes;
	void *ret = NULL;

	if (!item)
	{
		p->misc[2] = p->misc[0];
		p->misc[3] = p->misc[1];
		p->current[1] = p->current[0];
	}

	planes = idx2planes[p->current[1]];
// 	ndisplay("nxt_mres: planes = %d", planes);

	(long)modes = p->misc[2];
	num_modes = p->misc[3];

	while (num_modes > 0)
	{
		if (modes->planes == planes)
		{
			struct resinf *r = p->resinf[p->current[1]];
			
			(struct nova_res *)p->misc[2] = modes + 1;
			p->misc[3] = num_modes - 1;
			r[item].id = ((long)modes - p->misc[0]) / sizeof(*modes);
			r[item].x = modes->max_x;
			r[item].y = modes->max_y;
			ret = modes->name;
			((char *)ret)[32] = '\0';
		#if 0
			(struct nova_res *)p->misc[2] = modes + 1;
			p->misc[3] = num_modes - 1;
			*(p->devids[p->current[1]] + item) = ((long)modes - p->misc[0]) / sizeof(*modes);
			ret = modes->name;
			((char *)ret)[32] = '\0';
		#endif
			break;
		}
		num_modes--;
		modes++;
	}
// 	display(" ..return %lx (%s)", ret, ret);
	return ret;
}

static short
count_nova_res(long num_modes, short planes, struct nova_res *modes)
{
	short count = 0;

	while (num_modes--)
	{
		if (modes->planes == planes)
			count++;
		modes++;
	}
	return count;
}
static struct milres_parm *
check_nova_res(struct xa_client *client, short mw)
{
	int i, j;
	unsigned short currmode = 0;
	struct nova_res *modes;
	struct milres_parm *p = NULL;
	long num_modes;
	struct file *fp;
	XATTR x;

	currmode = nova_data->xcb->resolution;
	
	fp = kernel_open(fn_novabib, O_RDONLY, NULL, &x);
	if (fp)
	{
// 		display("found sta_vdi.bib");
		modes = kmalloc(x.size);
		if (modes)
		{
			OBJECT *obtree;
			short depths = 0, devids = 0;
			short count[8];

			num_modes = kernel_read(fp, modes, x.size);
			kernel_close(fp);
			if (num_modes != x.size)
				goto exit;
			
			num_modes = x.size / sizeof(struct nova_res);
			
			count[0] = count_nova_res(num_modes,  1, modes);
			count[1] = count_nova_res(num_modes,  2, modes);
			count[2] = count_nova_res(num_modes,  4, modes);
			count[3] = count_nova_res(num_modes,  8, modes);
			count[4] = count_nova_res(num_modes, 15, modes);
			count[5] = count_nova_res(num_modes, 16, modes);
			count[6] = count_nova_res(num_modes, 24, modes);
			count[7] = count_nova_res(num_modes, 32, modes);

			for (i = 0; i < 8; i++)
			{
				if (count[i])
				{	
					devids += count[i] + 1;
					depths++;
				}
			}
		
			if (depths)
			{
				struct resinf *r;
// 				short *di;
			
				if (!(p = kmalloc(sizeof (*p) + (sizeof(*r) * devids)))) //if (!(p = kmalloc(sizeof(*p) + (devids << 1))))
					goto exit;

				bzero(p, sizeof(*p));

				p->modes = modes;
				p->curr_devid = currmode;
				
// 				(long)di = (long)p + sizeof(*p);
				(long)r = (long)p + sizeof(*p);

				for (i = 0; i < 8; i++)
				{
					if ((p->count[i] = count[i]))
					{
						p->resinf[i] = r;
						r += p->count[i];
					}
				}
			#if 0
				for (i = 0; i < 8; i++)
				{
					p->count[i] = count[i];

					if (p->count[i])
					{
						p->devids[i] = di;
						di += p->count[i];
					}
				}
			#endif
				p->num_depths = depths;
				p->current[0] = 0;
// 				display("color depths %d", depths);
				obtree = create_popup_tree(client, 0, depths, mw, 4, &nxt_mdepth, (void **)&p);
				if (!instchrm_wt(client, &p->depth_wt, obtree))
					goto exit;
			
				p->depth_wt->links++;

// 				display("1");
				for (i = 0,j = 1; i < 8; i++)
				{
					if (p->count[i])
					{
						p->current[0] = i;
						p->misc[0] = (long)modes;
						p->misc[1] = num_modes;
						obtree = create_popup_tree(client, 0, p->count[i], mw, 4, &nxt_novares, (void **)&p);
						if (instchrm_wt(client, &p->col_wt[i], obtree))
						{
							p->col_wt[i]->links++;
						}
						else
							goto exit;
						j++;
// 						display("2 - %d %d", i, j);
					}
				}
// 			display("3");
			}	
		}
	}
// 	else
// 		display("sta_vdi.bib not found");

	return p;
exit:
	if (modes)
		kfree(modes);
	if (p)
		delete_milres_parm(p);
	
	return NULL;
}

void
open_nova_reschange(enum locks lock, struct xa_client *client)
{
	struct xa_window *wind;
	XA_TREE *wt;
	OBJECT *obtree;
	struct milres_parm *p;

	if (!reschg_win)
	{
		obtree = ResourceTree(C.Aes_rsc, RES_CHMIL);
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		if (wt && (p = check_nova_res(client, obtree[RCHM_RES].ob_width)))
		{
			p->current[1] = milan_setdevid(wt, p, p->curr_devid);
			
			wind = create_dwind(lock, CLOSER, t_reschg, client, wt, nova_reschg_form_exit, reschg_destructor);
			if (wind)
			{
				add_xa_data(&wind->xa_data, p, "milres_parm", delete_milres_parm);
				open_window(lock, wind, wind->r);
				reschg_win = wind;
			}
			else
				goto failure;
		}
	}
	else if (reschg_win != window_list)
	{
		top_window(lock, true, false, reschg_win, (void *)-1L);
	}
	return;
failure:
	delete_milres_parm(p);
	
}

/*
 * client still running dialog
 */
static struct xa_window *csr_win = NULL;

static int
csr_destructor(enum locks lock, struct xa_window *wind)
{
	csr_win = NULL;
	return true;
}

static void
csr_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;

	Sema_Up(clients);
	lock |= clients;
	
	wt->current = fr->obj;
	wt->which = 0;

	switch (fr->obj)
	{
		case KORW_WAIT:
		{
			if (C.csr_client)
			{
				send_terminate(lock, C.csr_client, (C.shutdown & RESOLUTION_CHANGE) ? AP_RESCHG : AP_TERM);
				set_shutdown_timeout(3000);
				C.csr_client = NULL;
			}
			else
				set_shutdown_timeout(1);

			object_deselect(wt->tree + KORW_WAIT);
			redraw_toolbar(lock, wind, KORW_WAIT);
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			break;
		}
		case KORW_KILL:
		{
			object_deselect(wt->tree + KORW_KILL);
			redraw_toolbar(lock, wind, KORW_KILL);

			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			if (C.csr_client)
			{
				C.csr_client->status |= CS_SIGKILLED;
				ikill(C.csr_client->p->pid, SIGKILL);
			}
			C.csr_client = NULL;
			set_shutdown_timeout(0);
			break;
		}
		case KORW_KILLEMALL:
		{
			object_deselect(wt->tree + KORW_KILLEMALL);
			redraw_toolbar(lock, wind, KORW_KILLEMALL);
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			C.shutdown |= KILLEM_ALL;
			set_shutdown_timeout(0);
			C.csr_client = NULL;
			break;
		}
	}
	Sema_Dn(clients);
}

static void
open_csr(enum locks lock, struct xa_client *client, struct xa_client *running)
{
	struct xa_window *wind;
	XA_TREE *wt;
	OBJECT *obtree;

	if (!csr_win)
	{
		TEDINFO *t;
		
		C.csr_client = running;
		obtree = ResourceTree(C.Aes_rsc, KILL_OR_WAIT);

		t = object_get_tedinfo(obtree + KORW_APPNAME, NULL);
		if (running->name[0])
		{
			int i;
			char *s = running->name;

			while (*s && *s == ' ')
				s++;

			for (i = 0; i < 32 && (t->te_ptext[i] = *s++); i++)
				;
		}
		else
		{
			strncpy(t->te_ptext, running->proc_name, 8);
			t->te_ptext[8] = '\0';
		}
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		if (wt)
		{
			wind = create_dwind(lock, 0, " Shutdown ", client, wt, csr_form_exit, csr_destructor);
			if (wind)
			{
				open_window(lock, wind, wind->r);
				csr_win = wind;
			}
		}
	}
	else if (csr_win != window_list)
	{
		top_window(lock, true, false, csr_win, (void *)-1L);
	}
}

void
CE_open_csr(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		open_csr(0, ce->client, ce->ptr1);
	}
}

void
CE_abort_csr(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		if (csr_win)
		{
			close_window(lock, csr_win);
			delayed_delete_window(lock, csr_win);
			C.csr_client = NULL;
		}
	}
}

static bool
cancelcsr(struct c_event *ce, long arg)
{
	if ((long)C.csr_client == arg)
	{
		C.csr_client = NULL;
		return true;
	}
	return false;
}
void
cancel_csr(struct xa_client *running)
{
	cancel_CE(C.Hlp, CE_open_csr, cancelcsr, (long)running);
}

static void
handle_launcher(enum locks lock, struct fsel_data *fs, const char *path, const char *file)
{
	char parms[200], *t;

	sprintf(parms+1, sizeof(parms)-1, "%s%s", path, file);
	parms[0] = '\0';

	for(t = parms + 1; *t; t++)
	{
		if (*t == '/')
			*t = '\\';
	}		

	close_fileselector(lock, fs);

	DIAGS(("launch: \"%s\"", parms+1));
	sprintf(cfg.launch_path, sizeof(cfg.launch_path), "%s%s", path, fs->fs_pattern);
	launch(lock, 0, 0, 0, parms+1, parms, C.Aes);
}

#if FILESELECTOR

static struct fsel_data aes_fsel_data;

static void
open_launcher(enum locks lock, struct xa_client *client)
{
	struct fsel_data *fs;

	if (!*cfg.launch_path)
	{
		cfg.launch_path[0] = d_getdrv() + 'a';
		cfg.launch_path[1] = ':';
		cfg.launch_path[2] = '\\';
		cfg.launch_path[3] = '*';
		cfg.launch_path[4] = 0;
	}

	fs = &aes_fsel_data;
	open_fileselector(lock, client, fs,
			  cfg.launch_path,
			  NULL, "Launch Program",
			  handle_launcher, NULL);
}
#endif



static struct xa_window *systemalerts_win = NULL;

/* double click now also available for internal handlers. */
static void
sysalerts_form_exit(struct xa_client *Client,
		    struct xa_window *wind,
		    struct widget_tree *wt,
		    struct fmd_result *fr)
{
	enum locks lock = 0;
	OBJECT *form = wt->tree;
	short item = fr->obj;

	switch (item)
	{
		case SYSALERT_LIST:
		{
			short obj = fr->obj;
			OBJECT *obtree = wt->tree;

			DIAGS(("taskmanager_form_exit: Moved the shit out of form_do() to here!"));
			if ( fr->md && ((obtree[obj].ob_type & 0xff) == G_SLIST))
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, obtree, obj, fr->md);
				else
					click_scroll_list(lock, obtree, obj, fr->md);
			}
			break;
		}
		/* Empty the alerts list */
		case SALERT_CLEAR:
		{
			struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
			struct sesetget_params p = { 0 };
			
			p.arg.txt = "Alerts";
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			if (p.e)
				list->empty(list, p.e, 0);
// 			display("emptied the alert list");
			object_deselect(wt->tree + item);
			redraw_toolbar(lock, systemalerts_win, SYSALERT_LIST);
			redraw_toolbar(lock, systemalerts_win, item);
			break;
		}
		case SALERT_OK:
		{
			object_deselect(wt->tree + item);
			redraw_toolbar(lock, systemalerts_win, item);
			close_window(lock, systemalerts_win);
			delayed_delete_window(lock, systemalerts_win);	
			break;
		}
	}
}

static int
systemalerts_destructor(enum locks lock, struct xa_window *wind)
{
	systemalerts_win = NULL;
	return true;
}

static void
refresh_systemalerts(OBJECT *form)
{
	OBJECT *sl = form + SYSALERT_LIST;
	SCROLL_INFO *list = object_get_slist(sl);

	list->slider(list, true);
}

static void
open_systemalerts(enum locks lock, struct xa_client *client)
{
	RECT remember = { 0, 0, 0, 0 };

	if (!systemalerts_win)
	{
		OBJECT *obtree = ResourceTree(C.Aes_rsc, SYS_ERROR);
		struct xa_window *dialog_window;
		XA_TREE *wt;
		RECT or;

		ob_rectangle(obtree, 0, &or);

		wt = obtree_to_wt(client, obtree);

		obtree[SALERT_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, client, WC_BORDER,
						CLOSER|NAME, created_for_AES,
						client->options.thinframe,
						client->options.thinwork,
						*(RECT *)&or); //*(RECT*)&obtree->ob_x);
		}

		/* Create the window */
		dialog_window = create_window(lock,
						do_winmesag,
						do_formwind_msg,
						client,
						false,
						CLOSER|NAME|TOOLBAR|hide_move(&(client->options)),
						created_for_AES,
						client->options.thinframe,
						client->options.thinwork,
						remember, NULL, NULL);

		/* Set the window title */
		set_window_title(dialog_window, " System window & Alerts log", false);

		wt = set_toolbar_widget(lock, dialog_window, client, obtree, -1, 0/*WIP_NOTEXT*/, true, NULL, &or);
		wt->exit_form = sysalerts_form_exit;
		/* Set the window destructor */
		dialog_window->destructor = systemalerts_destructor;

		refresh_systemalerts(obtree);

		open_window(lock, dialog_window, dialog_window->r);
		systemalerts_win = dialog_window;
	}
	else if (systemalerts_win != window_list)
	{
		top_window(lock, true, false, systemalerts_win, (void *)-1L);
	}
}


/*
 * Handle clicks on the system default menu
 */
void
do_system_menu(enum locks lock, int clicked_title, int menu_item)
{
	switch (menu_item)
	{
		default:
			DIAGS(("Unhandled system menu item %i", menu_item));
			break;

		/* Open the "About XaAES..." dialog */
		case SYS_MN_ABOUT:
			post_cevent(C.Hlp, ceExecfunc, open_about,NULL, 0,0, NULL,NULL);
			break;

		/* Quit all applications */
		case SYS_MN_QUITAPP:
			DIAGS(("Quit all Apps"));
			quit_all_apps(lock, NULL, AP_TERM);
			break;

		/* Quit XaAES */
		case SYS_MN_QUIT:
			DIAGS(("Quit XaAES"));
			dispatch_shutdown(0);
			break;

		/* Open the "Task Manager" window */
		case SYS_MN_TASKMNG:
			post_cevent(C.Hlp, ceExecfunc, open_taskmanager,NULL, 0,0, NULL,NULL);
			break;
		/* Open system alerts log window */
		case SYS_MN_SALERT:
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts,NULL, 0,0, NULL,NULL);
			break;
		/* Open system alerts log window filled with environment */
		case SYS_MN_ENV:
		{
			OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
			struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
			struct scroll_entry *this;
			char * const * const strings = get_raw_env();
			int i;
			struct sesetget_params p = { 0 };
			struct scroll_content sc = {{ 0 }};

			p.idx = -1;
			p.arg.txt = "Environment";
			p.level.flags = 0;
			p.level.curlevel = 0;
			p.level.maxlevel = 0;
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			list->empty(list, p.e, 0);
			this = p.e;
			sc.t.strings = 1;
			for (i = 0; strings[i]; i++)
			{	sc.t.text = strings[i];
				list->add(list, this, NULL, &sc, this ? (SEADD_CHILD|SEADD_PRIOR) : SEADD_PRIOR, SETYP_AMAL, true);		
			}
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts,NULL, 0,0, NULL,NULL);
			break;
		}

#if FILESELECTOR
		/* Launch a new app */
		case SYS_MN_LAUNCH:
		{
			post_cevent(C.Hlp, ceExecfunc, open_launcher,NULL, 0,0, NULL,NULL);
			break;
		}
#endif
		/* Launch desktop. */
		case SYS_MN_DESK:
		{
			if (C.DSKpid >= 0)
				ALERT(("XaAES: AES shell already running!"));
			else if (!*C.desk)
				ALERT(("XaAES: No AES shell set; See 'shell =' configuration variable in xaaes.cnf"));
			else
				C.DSKpid = launch(lock, 0,0,0, C.desk, "\0", C.Aes);
			break;
	#if 0
			if (*C.desk)
			{
				if (C.DSKpid >= 0);
					ALERT("AES shell already running!");
				else if (!*C.desk)
					ALERT("No AES shell defined");
				else
					C.DSKpid = launch(lock, 0, 0, 0, C.desk, "\0", C.Aes);
			}
			break;
	#endif
		}
		case SYS_MN_RESCHG:
		{
			if (C.reschange)
				post_cevent(C.Hlp, ceExecfunc, C.reschange,NULL, 0,0, NULL,NULL);
		}
	}
}
