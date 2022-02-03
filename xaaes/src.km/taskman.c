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
#include "app_man.h"
#include "c_window.h"
#include "form.h"
#include "k_main.h"
#include "k_mouse.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "obtree.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_appl.h"
#include "xa_form.h"
#include "xa_graf.h"
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
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL},	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_WHITE, G_BLACK, G_WHITE,   0,      0,     NULL},	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL},	/* Highlighted */

};

static struct xa_wtxt_inf acc_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLUE, G_LBLUE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLUE, G_LBLUE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf prg_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_LRED, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_LRED, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf sys_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1,  0, MD_TRANS, 0, G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1,  0, MD_TRANS, 0, G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1,  0, MD_TRANS, 0, G_BLACK, G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf sys_thrd =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_LWHITE, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK,  G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf desk_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_RED,   G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_RED,   G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static void
delete_htd(void *_htd)
{
	struct helpthread_data *htd = _htd;

	if (htd->w_about)
	{
		close_window(0, htd->w_about);
		delete_window(0, htd->w_about);
	}
	if (htd->w_taskman)
	{
		close_window(0, htd->w_taskman);
		delete_window(0, htd->w_taskman);
	}
	if (htd->w_sysalrt)
	{
		close_window(0, htd->w_sysalrt);
		delete_window(0, htd->w_sysalrt);
	}
	if (htd->w_reschg)
	{
		close_window(0, htd->w_reschg);
		delete_window(0, htd->w_reschg);
	}
	kfree(htd);
}
	
struct helpthread_data *
get_helpthread_data(struct xa_client *client)
{
	struct helpthread_data *htd;

	htd = lookup_xa_data_byname(&client->xa_data, HTDNAME);
	if (!htd)
	{
		if ((htd = kmalloc(sizeof(*htd))))
		{
			bzero(htd, sizeof(*htd));
			add_xa_data(&client->xa_data, htd, 0, HTDNAME, delete_htd);
		}
	}
	return htd;
}

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
            sprintf(tx, tx_len, " %3d/    %s", client->p->pid, client->name);
	}
	return tx;
};

void
add_to_tasklist(struct xa_client *client)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	struct xa_window *wind;

	if (!htd)
		return;
	
	wind = htd->w_taskman;
	
	if (wind)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		OBJECT *obtree = wt->tree; //ResourceTree(C.Aes_rsc, TASK_MANAGER);
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		OBJECT *icon;
		char *tx;
		struct scroll_content sc = {{ 0 }};

// 		display("add2tl: wt = %lx, obtree = %lx, list =%lx", wt, obtree, list);
		if (screen.planes < 4)
			sc.fnt = &norm_txt;
		else if (client->p->pid == C.DSKpid)
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

		tx = build_tasklist_string(client);
		sc.icon = icon;
		sc.t.text = tx ? tx : client->name;
		sc.t.strings = 1;
		sc.data = client;
		list->add(list, NULL, NULL, &sc, false, 0, true);
		if (tx) kfree(tx);
	}
}

void
remove_from_tasklist(struct xa_client *client)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	struct xa_window *wind;

	if (!htd)
		return;
	wind = htd->w_taskman;

	if (wind)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		OBJECT *obtree = wt->tree; //ResourceTree(C.Aes_rsc, TASK_MANAGER);
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
}

void
update_tasklist_entry(struct xa_client *client)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	struct xa_window *wind;

	if (!htd)
		return;
	wind = htd->w_taskman;

	if (wind)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		OBJECT *obtree = wt->tree; //ResourceTree(C.Aes_rsc, TASK_MANAGER);
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		struct sesetget_params p = { 0 };
	
		if (list)
		{
			char *tx;
			p.arg.data = client;
			list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
			if (p.e)
			{
				struct setcontent_text t = { 0 };

				if ((tx = build_tasklist_string(client)))
					t.text = tx;
				else
					t.text = client->name;
			
				list->set(list, p.e, SESET_TEXT, (long)&t, true);
			
				if (tx)
					kfree(tx);
			}
		}
	}
}

// static struct xa_window *task_man_win = NULL;

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

#if INCLUDE_UNUSED
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
#endif
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
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);

	if (htd)
		htd->w_taskman = NULL;
// 	task_man_win = NULL;
	return true;
}

void
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

#if INCLUDE_UNUSED
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
		    client != C.Hlp &&
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
#endif
void
CHlp_aesmsg(struct xa_client *client)
{
	union msg_buf *m;
	char alert[256];

	if (client->waiting_pb && (m = (union msg_buf *)client->waiting_pb->addrin[0]))
	{
		switch (m->m[0])
		{
			case 0x5354:
			{
// 				display("0x5354 gotten");
				if (m->m[3] == 1)
				{
					C.update_lock = NULL;
					C.updatelock_count--;
					unlock_screen(client->p);
// 					do_form_alert(0, client, 1, "[1][Snapper got videodata, screen unlocked][OK]", "XaAES");
				}
				else if (m->m[3] != 0)
				{
					sprintf(alert, sizeof(alert), /*scrn_snap_serr*/"[1][ Snapper could not save snap! | ERROR: %d ][ Ok ]", m->m[3]);
					do_form_alert(0, client, 1, alert, "XaAES");
				}
				break;
			}
			default:;
		}
	}
}


#if INCLUDE_UNUSED
static char sdalert[] = /*scrn_snap_what*/ "[2][What do you want to snap?][Block|Full screen|Top Window|Cancel]";
void
screen_dump(enum locks lock, struct xa_client *client, bool open)
{
	struct xa_client *dest_client;

	if ((dest_client = get_app_by_procname("xaaesnap")))
	{
// 		display("send dump message to %s", dest_client->proc_name);
		if (update_locked() != client->p && lock_screen(client->p, true))
		{
			short msg[8] = {0x5354, client->p->pid, 0, 0, 0,0,200,200};
			RECT r, d = {0};
			short b = 0, x, y;
			bool doit = true;
			AESPB *a = C.Hlp_pb;

			C.update_lock = client->p;
			C.updatelock_count++;

			do_form_alert(lock, client, 4, sdalert, "XaAES");
			Block(client, 0);
			
// 			display("intout %d", C.Hlp_pb->intout[0]);

			if (a->intout[0] == 1) //(open)
			{
				xa_graf_mouse(THIN_CROSS, NULL,NULL, false);
				while (!b)
					check_mouse(client, &b, &x, &y);

				r.x = x; 
				r.y = y;
				r.w = 0;
				r.h = 0;
				rubber_box(client, SE, r, &d, 0,0, root_window->r.h, root_window->r.w, &r);
			}
			else if (a->intout[0] == 2)
				r = root_window->r;
			else if (a->intout[0] == 3)
			{
				struct xa_window *wind = TOP_WINDOW;

				if (wind->r.x > 0 && wind->r.y > 0 &&
				   (wind->r.x + wind->r.w) < root_window->r.w &&
				   (wind->r.y + wind->r.h) < root_window->r.h)
					r = wind->r;
				else
				{
					C.updatelock_count--;
					C.update_lock = NULL;
					unlock_screen(client->p);
					do_form_alert(lock, client, 1, /*scrn_snap_twc*/"[1][Cannot snap topwindow as | parts of it is offscreen!][OK]", "XaAES");
					doit = false;
				}
			}
			else
				doit = false;

			if ((r.w | r.h) && doit) //C.Hlp_pb->intout[0] != 4)
			{
				msg[4] = r.x;
				msg[5] = r.y;
				msg[6] = r.w;
				msg[7] = r.h;
				send_a_message(lock, dest_client, AMQ_NORM, 0, (union msg_buf *)msg);
// 				xa_graf_mouse(ARROW, NULL,NULL, false);
			}
			else
			{
				C.updatelock_count--;
				C.update_lock = NULL;
				unlock_screen(client->p);
			}
			client->usr_evnt = 0;
			client->waiting_for = XAWAIT_MULTI|MU_MESAG;
			client->waiting_pb = a;
		}
	}
	else
		do_form_alert(lock, client, 1, /*scrn_snap_notfound*/"[1]['xaaesnap' process not found.|Start 'xaaesnap.prg' and try again|Later XaAES will start the snapper|according to userconfiguration][OK]", "XaAES");
}
#endif
static void
taskmanager_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	struct xa_vdi_settings *v = wind->vdi_settings;
	
	Sema_Up(clients);
	lock |= clients;
	
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
		case TM_LIST:
		{
			OBJECT *obtree = wt->tree;

			DIAGS(("taskmanager_form_exit: Moved the shit out of form_do() to here!"));
			if ( fr->md && ((aesobj_type(&fr->obj) & 0xff) == G_SLIST))
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, obtree, aesobj_item(&fr->obj), fr->md);
				else
					click_scroll_list(lock, obtree, aesobj_item(&fr->obj), fr->md);
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
					ALERT(/*kill_aes_thread*/("Not a good idea, I tell you!"));
				}
				else
					ikill(client->p->pid, SIGKILL);
			}

			object_deselect(wt->tree + TM_KILL);
			redraw_toolbar(lock, wind, TM_KILL);
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
					ALERT((/*kill_aes_thread*/"Cannot terminate AES system proccesses!"));
				}
				else
					send_terminate(lock, client, AP_TERM);
			}

			object_deselect(wt->tree + TM_TERM);
			redraw_toolbar(lock, wind, TM_TERM);
			break;
		}
		case TM_SLEEP:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = NULL;

			if (list->cur)
				client = list->cur->data;

			DIAGS(("taskmanager: TM_SLEEP for %s", c_owner(client)));
			ALERT(("taskmanager: TM_SLEEP not yet implemented!"));
			if (client && is_client(client) && client != C.Hlp)
			{
			}

			object_deselect(wt->tree + TM_SLEEP);
			redraw_toolbar(lock, wind, TM_SLEEP);
			break;
		}
		case TM_WAKE:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = NULL;

			if (list->cur)
				client = list->cur->data;

			DIAGS(("taskmanager: TM_WAKE for %s", c_owner(client)));
			ALERT(("taskmanager: TM_WAKE not yet implemented!"));
			if (client && is_client(client) && client != C.Hlp)
			{
			}

			object_deselect(wt->tree + TM_WAKE);
			redraw_toolbar(lock, wind, TM_WAKE);
			break;
		}

		/* the bottom action buttons */
		case TM_QUITAPPS:
		{
			DIAGS(("taskmanager: quit all apps"));
			quit_all_apps(lock, NULL, AP_TERM);

			object_deselect(wt->tree + TM_QUITAPPS);
			redraw_toolbar(lock, wind, TM_QUITAPPS);
			break;
		}
		case TM_QUIT:
		{
			DIAGS(("taskmanager: quit XaAES"));
			dispatch_shutdown(0,0);

			object_deselect(wt->tree + TM_QUIT);
			redraw_toolbar(lock, wind, TM_QUIT);
			break;
		}
		case TM_REBOOT:
		{
			DIAGS(("taskmanager: reboot system"));
			dispatch_shutdown(REBOOT_SYSTEM,0);

			object_deselect(wt->tree + TM_REBOOT);
			redraw_toolbar(lock, wind, TM_REBOOT);
			break;
		}
		case TM_HALT:
		{
			DIAGS(("taskmanager: halt system"));
			dispatch_shutdown(HALT_SYSTEM, 0);

			object_deselect(wt->tree + TM_HALT);
			redraw_toolbar(lock, wind, TM_HALT);
			break;
		}
		case TM_COLD:
		{
			DIAGS(("taskmanager: coldstart system"));
			dispatch_shutdown(COLDSTART_SYSTEM, 0);
			object_deselect(wt->tree + TM_COLD);
			redraw_toolbar(lock, wind, TM_COLD);
			break;
		}
		case TM_RESCHG:
		{
			
			if (C.reschange)
				post_cevent(C.Hlp, ceExecfunc, C.reschange,NULL, 1,0, NULL,NULL);
			obj_change(wt, v, fr->obj, -1, aesobj_state(&fr->obj) & ~OS_SELECTED, aesobj_flags(&fr->obj), true, NULL, wind->rect_list.start, 0);
			break;
		}
		case TM_OK:
		{
			object_deselect(wt->tree + TM_OK);
			redraw_toolbar(lock, wind, TM_OK);

			/* and release */
			close_window(lock, wind);
// 			delayed_delete_window(lock, wind);
			break;
		}
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", fr->obj));
			break;
		}
	}
	Sema_Dn(clients);
}

void
open_taskmanager(enum locks lock, struct xa_client *client, bool open)
{
	RECT remember = { 0,0,0,0 };
	struct helpthread_data *htd;
	struct xa_window *wind;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	RECT or;

	htd = get_helpthread_data(client);
	if (!htd)
		return;
	
	if (!htd->w_taskman) //(!task_man_win)
	{
		struct scroll_info *list;
/* ********** */
		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, TASK_MANAGER), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		
		list = set_slist_object(0, wt, NULL, TM_LIST,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_ICONINDENT,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 /*tm_client_apps*/"Client Applications", NULL, NULL, 255);

		if (!list) goto fail;

		obj_init_focus(wt, OB_IF_RESET);
/* ********** */
		obj_rectangle(wt, aesobj(obtree, 0), &or);
		obtree[TM_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, client, WC_BORDER,
						CLOSER|NAME, created_for_AES,
						client->options.thinframe,
						client->options.thinwork,
						*(RECT *)&or);
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag, do_formwind_msg,
					client,
					false,
					CLOSER|NAME|TOOLBAR|hide_move(&(client->options)),
					created_for_AES,
					client->options.thinframe,
					client->options.thinwork,
					remember, NULL, NULL);
		
		if (!wind) goto fail;
		
		list->set(list, NULL, SESET_PRNTWIND, (long)wind, NOREDRAW);
		wind->window_status |= XAWS_NODELETE;
		
		/* Set the window title */
		set_window_title(wind, /*tm_manager*/" Task Manager ", false);

		wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		wt->exit_form = taskmanager_form_exit;

		/* Set the window destructor */
		wind->destructor = taskmanager_destructor;
	
		htd->w_taskman = wind;
		
		if (open)
			open_window(lock, wind, wind->r);	
	}
	else
	{
		wind = htd->w_taskman;
		if (open)
		{
			open_window(lock, wind, wind->r);
			if (wind != TOP_WINDOW)
				top_window(lock, true, false, wind);
		}
	}
	return;
fail:
	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
}

/* ************************************************************ */
/*     Common resolution mode change functions/stuff		*/
/* ************************************************************ */
// static struct xa_window *reschg_win = NULL;
#if 0
static int
reschg_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_reschg = NULL;
// 	reschg_win = NULL;
	return true;
}
#endif
struct xa_window * _cdecl
create_dwind(enum locks lock, XA_WIND_ATTR tp, char *title, struct xa_client *client, struct widget_tree *wt, FormExit(*f), WindowDisplay(*d))
{
	struct xa_window *wind;
	OBJECT *obtree = wt->tree;
        RECT r, or;

	obj_rectangle(wt, aesobj(obtree, 0), &or);

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

	wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
	wt->exit_form = f; //milan_reschg_form_exit;

	/* Set the window destructor */
	wind->destructor = d; //reschg_destructor;

	return wind;
}
/* ************************************************************ */
/*     Atari resolution mode change functions			*/
/* ************************************************************ */
#if 0
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
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);
}

static short
get_resmode_obj(XA_TREE *wt)
{
	struct xa_aes_object o;
	short obj;

	o = obj_get_radio_button(wt, aesobj(wt->tree, RC_MODES), OS_SELECTED);

	obj = aesobj_item(&o);

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
	
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
		case RC_OK:
		{
			DIAGS(("reschange: restart"));

			object_deselect(aesobj_ob(&fr->obj));
			redraw_toolbar(lock, wind, RC_OK);
			next_res = get_resmode_obj(wt);
			next_res |= 0x80000000;
			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			dispatch_shutdown(RESOLUTION_CHANGE/*0*/, next_res);
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
			DIAGS(("taskmanager: unhandled event %i", fr->obj));
			break;
		}
	}
	Sema_Dn(clients);
}

static char t_reschg[] = " Change Resolution ";

void
open_reschange(enum locks lock, struct xa_client *client, bool open)
{
	struct xa_window *wind;
	struct helpthread_data *htd;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;

	htd = get_helpthread_data(client);

	if (!htd->w_reschg) //(!reschg_win)
	{
		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, RES_CHATARI), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;		
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		wind = create_dwind(lock, CLOSER, t_reschg, client, wt, resmode_form_exit, reschg_destructor);
		if (!wind) goto fail;
		
		set_resmode_obj(wt, cfg.videomode);
		if (open)
			open_window(lock, wind, wind->r);
		htd->w_reschg = wind; //reschg_win = wind;
	}
	else
	{
		wind = htd->w_reschg;
		if (open)
		{
			open_window(lock, wind, wind->r);
			if (wind != TOP_WINDOW/*window_list*/)
				top_window(lock, true, false, wind);
		}
	}
	return;
fail:
	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
}
#ifndef ST_ONLY
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
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	obj = RC_COLUMNS + 1 + ((res & (1<<3)) ? 1 : 0);
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	obj = RC_VGA + 1 + ((res & (1<<4)) ? 1 : 0);
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	obj = RC_TVSEL + 1 + ((res & (1<<5)) ? 1 : 0);
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	setsel(obtree + RC_OVERSCAN, (res & (1<<6)));
	setsel(obtree + RC_ILACE, (res & (1<<7)));
	setsel(obtree + RC_BIT15, (res & 0x8000));
}

static unsigned long
get_reschg_obj(XA_TREE *wt)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object obj;
	unsigned long res = 0L;

	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_COLOURS), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= aesobj_item(&obj) - 1 - RC_COLOURS;
	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_COLUMNS), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= (aesobj_item(&obj) - 1 - RC_COLUMNS) << 3;
	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_VGA), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= (aesobj_item(&obj) - 1 - RC_VGA) << 4;
	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_TVSEL), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= (aesobj_item(&obj) - 1 - RC_TVSEL) << 5;

	if (issel(obtree + RC_OVERSCAN)) res |= (1<<6);
	if (issel(obtree + RC_ILACE))	res |= (1<<7);
	if (issel(obtree + RC_BIT15))	res |= 0x8000;

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
	
// 	wt->current = fr->obj;
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
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
			dispatch_shutdown(RESOLUTION_CHANGE, next_res);
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
			DIAGS(("taskmanager: unhandled event %i", fr->obj));
			break;
		}
	}

	Sema_Dn(clients);
}

void
open_falcon_reschange(enum locks lock, struct xa_client *client, bool open)
{
	struct xa_window *wind;
	struct helpthread_data *htd;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;

	htd = get_helpthread_data(client);

	if (!htd->w_reschg) //(!reschg_win)
	{
		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, RES_CHFALC), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;		
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		wind = create_dwind(lock, CLOSER, t_reschg, client, wt, reschg_form_exit, reschg_destructor);
		if (!wind) goto fail;
		
		set_reschg_obj(wt, (unsigned long)cfg.videomode);
		if (open)
			open_window(lock, wind, wind->r);
		htd->w_reschg = wind; //reschg_win = wind;
	}
	else
	{
		wind = htd->w_reschg;
		if (open)
		{
			open_window(lock, wind, wind->r);
			if (wind != TOP_WINDOW/*window_list*/)
				top_window(lock, true, false, wind);
		}
	}
	return;
fail:
	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
}
#endif
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
#ifndef ST_ONLY
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
	
// 	wt->current = fr->obj;
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
		case RCHM_COL:
		{
			int i, o, newres = 1;
			POPINFO *pinf = object_get_popinfo(aesobj_ob(&fr->obj));
			struct widget_tree *pu_wt = NULL;

			for (i = 0, o = pinf->obnum; i < 8 && o >= 0; i++)
			{
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
								if (new[j].x == old->x && new[j].y == old->y)
								{
									newres = j + 1;
									break;
								}
							}
						}
						
						pu_wt = p->col_wt[i];
						new_devid = new[newres - 1].id;
						
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
				obj_set_g_popup(wt, aesobj(wt->tree, RCHM_RES), pinf);
				obj_draw(wt, wind->vdi_settings, aesobj(wt->tree, RCHM_RES), -1, NULL, wind->rect_list.start, 0);
				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_RES:
		{
			POPINFO *pinf = object_get_popinfo(aesobj_ob(&fr->obj));
			struct widget_tree *pu_wt = p->col_wt[p->current[0]];

			if (pu_wt)
			{
				struct resinf *r = p->resinf[p->current[0]];
				
				new_devid = r[pinf->obnum - 1].id;

				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_OK:
		{
			DIAGS(("reschange: restart"));

			object_deselect(wt->tree + RC_OK);
			redraw_toolbar(lock, wind, RC_OK);
			next_res = p->current[1];
			next_res |= 0x80000000;
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			dispatch_shutdown(RESOLUTION_CHANGE, next_res);
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
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", fr->obj));
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
#endif

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
#ifndef ST_ONLY
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

	modes = (struct videodef *)p->misc[2];
	num_modes = p->misc[3];

	while (num_modes > 0)
	{
		if (modes->planes == planes)
		{
			struct resinf *r = p->resinf[p->current[1]];
			
			p->misc[2] = (long)(modes + 1);
			p->misc[3] = num_modes - 1;
			r[item].id = modes->devid;
			r[item].x  = modes->res_x;
			r[item].y  = modes->res_y;
			ret = modes->name;
			break;
		}
		num_modes--;
		modes++;
	}
	return ret;
}
#endif
static int
instchrm_wt(struct xa_client *client, struct widget_tree **wt, OBJECT *obtree)
{
	int ret = 0;
	
	if (obtree)
	{
		*wt = new_widget_tree(client, obtree);
		if (*wt)
		{
			(*wt)->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
			ret = 1;
		}
		else
		{
			free_object_tree(C.Aes, obtree);
		}
	}
	else
		*wt = NULL;

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
#ifndef ST_ONLY
static struct milres_parm *
check_milan_res(struct xa_client *client, short mw)
{
	int i, j;
	short currmode = 0;
	struct milres_parm *p = NULL;
	long num_modes, ret;

	num_modes = mvdi_device(0, 0, DEVICE_GETDEVICE, &ret);

	if (num_modes >= 0)
	{
		currmode = ((struct videodef *)ret)->devid;
	}
	
	num_modes = mvdi_device(0, 0, DEVICE_GETDEVICELIST, &ret);

	if (num_modes > 0)
	{
		struct videodef *modes = (struct videodef *)ret;
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
			union { void **vp; struct milres_parm **mp;} ptrs;
			struct resinf *r;
			
			if (!(p = kmalloc(sizeof(*p) + (sizeof(*r) * devids))))
				goto exit;

			bzero(p, sizeof(*p));

			ptrs.mp = &p;

			p->curr_devid = currmode;
			r = (struct resinf *)((char *)p + sizeof(*p));
			for (i = 0; i < 8; i++)
			{
				if ((p->count[i] = count[i]))
				{
					p->resinf[i] = r;
					r += p->count[i];
				}
			}
			p->num_depths = depths;
			p->current[0] = 0;
			obtree = create_popup_tree(client, 0, depths, mw, 4, &nxt_mdepth, ptrs.vp);
			if (!instchrm_wt(client, &p->depth_wt, obtree))
				goto exit;
			
			p->depth_wt->links++;

			for (i = 0,j = 1; i < 8; i++)
			{
				if (p->count[i])
				{
					p->current[0] = i;
					p->misc[0] = (long)modes;
					p->misc[1] = num_modes;
					obtree = create_popup_tree(client, 0, p->count[i], mw, 4, &nxt_mres, ptrs.vp);
					if (instchrm_wt(client, &p->col_wt[i], obtree))
					{
						p->col_wt[i]->links++;
					}
					else
						goto exit;
					j++;
				}
			}
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
#endif

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
	obj_set_g_popup(wt, aesobj(wt->tree, RCHM_COL), &p->pinf_depth);
	obj_set_g_popup(wt, aesobj(wt->tree, RCHM_RES), &p->pinf_res);
	return found_devid;
}
#ifndef ST_ONLY
void
open_milan_reschange(enum locks lock, struct xa_client *client, bool open)
{
	struct xa_window *wind;
	struct helpthread_data *htd;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	struct milres_parm *p = NULL;

	if (!(htd = get_helpthread_data( client )))
		return;

	if (!htd->w_reschg) //reschg_win)
	{
		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, RES_CHMIL), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;		
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		p = check_milan_res(client, obtree[RCHM_RES].ob_width);
		if (!p) goto fail;
		
		p->current[1] = milan_setdevid(wt, p, p->curr_devid);
			
		wind = create_dwind(lock, CLOSER, t_reschg, client, wt, milan_reschg_form_exit, reschg_destructor);
		if (!wind) goto fail;

		add_xa_data(&wind->xa_data, p, 0, "milres_parm", delete_milres_parm);
		if (open)
			open_window(lock, wind, wind->r);
		htd->w_reschg = wind; //reschg_win = wind;
	}
	else
	{
		wind = htd->w_reschg;
		if (open)
		{
			open_window(lock, wind, wind->r);
			if (wind != TOP_WINDOW/*window_list*/)
				top_window(lock, true, false, wind);
		}
	}
	return;
fail:
	delete_milres_parm(p);

	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
}
#endif
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
	
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
		case RCHM_COL:
		{
			int i, o, newres = 1;
			POPINFO *pinf = object_get_popinfo(aesobj_ob(&fr->obj));
			struct widget_tree *pu_wt = NULL;

			for (i = 0, o = pinf->obnum; i < 8 && o >= 0; i++)
			{
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
								if (new[j].x == old->x && new[j].y == old->y)
								{
									newres = j + 1;
									break;
								}
							}
						}
						
						pu_wt = p->col_wt[i];
						new_devid = new[newres - 1].id;
						
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
				obj_set_g_popup(wt, aesobj(wt->tree, RCHM_RES), pinf);
				obj_draw(wt, wind->vdi_settings, aesobj(wt->tree, RCHM_RES), -1, NULL, wind->rect_list.start, 0);
				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_RES:
		{
			POPINFO *pinf = object_get_popinfo(aesobj_ob(&fr->obj));
			struct widget_tree *pu_wt = p->col_wt[p->current[0]];

			if (pu_wt)
			{
				struct resinf *r = p->resinf[p->current[0]];
				
				new_devid = r[pinf->obnum - 1].id;
				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_OK:
		{
			DIAGS(("reschange: restart"));

			object_deselect(wt->tree + RC_OK);
			redraw_toolbar(lock, wind, RC_OK);
			next_res = p->current[1];
			next_res |= 0x80000000;
			nova_data->next_res = ((struct nova_res *)p->modes)[p->current[1]];
			nova_data->valid = true;
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			kfree(p->modes);
			dispatch_shutdown(RESOLUTION_CHANGE, next_res);
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
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", fr->obj));
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
	char *ret = NULL;

	if (!item)
	{
		p->misc[2] = p->misc[0];
		p->misc[3] = p->misc[1];
		p->current[1] = p->current[0];
	}

	planes = idx2planes[p->current[1]];

	modes = (struct nova_res *)p->misc[2];
	num_modes = p->misc[3];

	while (num_modes > 0)
	{
		if (modes->planes == planes)
		{
			struct resinf *r = p->resinf[p->current[1]];
			
			p->misc[2] = (long)(modes + 1);
			p->misc[3] = num_modes - 1;
			r[item].id = ((long)modes - p->misc[0]) / sizeof(*modes);
			r[item].x = modes->max_x;
			r[item].y = modes->max_y;
			ret = (char *)modes->name;
			ret[32] = '\0';
			break;
		}
		num_modes--;
		modes++;
	}
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
				union { void **vp; struct milres_parm **mp;} ptrs;
				struct resinf *r;
			
				if (!(p = kmalloc(sizeof (*p) + (sizeof(*r) * devids))))
					goto exit;

				bzero(p, sizeof(*p));
				
				ptrs.mp = &p;

				p->modes = modes;
				p->curr_devid = currmode;
				
				r = (struct resinf *)((char *)p + sizeof(*p));

				for (i = 0; i < 8; i++)
				{
					if ((p->count[i] = count[i]))
					{
						p->resinf[i] = r;
						r += p->count[i];
					}
				}
				p->num_depths = depths;
				p->current[0] = 0;
				obtree = create_popup_tree(client, 0, depths, mw, 4, &nxt_mdepth, ptrs.vp);
				if (!instchrm_wt(client, &p->depth_wt, obtree))
					goto exit;
			
				p->depth_wt->links++;

				for (i = 0,j = 1; i < 8; i++)
				{
					if (p->count[i])
					{						
						p->current[0] = i;
						p->misc[0] = (long)modes;
						p->misc[1] = num_modes;
						obtree = create_popup_tree(client, 0, p->count[i], mw, 4, &nxt_novares, ptrs.vp);
						if (instchrm_wt(client, &p->col_wt[i], obtree))
						{
							p->col_wt[i]->links++;
						}
						else
							goto exit;
						j++;
					}
				}
			}	
		}
		else
			kernel_close(fp);
	}
	
	return p;
exit:
	if (modes)
		kfree(modes);
	if (p)
		delete_milres_parm(p);
	
	return NULL;
}

void
open_nova_reschange(enum locks lock, struct xa_client *client, bool open)
{
	struct xa_window *wind;
	struct helpthread_data *htd;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	struct milres_parm *p = NULL;

	if (!(htd = get_helpthread_data(client)))
		return;

	if (!htd->w_reschg) // !reschg_win)
	{
		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, RES_CHMIL), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		p = check_nova_res(client, obtree[RCHM_RES].ob_width);
		if (!p) goto fail;

		p->current[1] = milan_setdevid(wt, p, p->curr_devid);
			
		wind = create_dwind(lock, CLOSER, t_reschg, client, wt, nova_reschg_form_exit, reschg_destructor);
		if (!wind) goto fail;

		add_xa_data(&wind->xa_data, p, 0, "milres_parm", delete_milres_parm);
		if (open)
			open_window(lock, wind, wind->r);
		htd->w_reschg = wind; //reschg_win = wind;
	}
	else
	{
		wind = htd->w_reschg;
		if (open)
		{
			open_window(lock, wind, wind->r);
			if (wind != TOP_WINDOW/*window_list*/)
				top_window(lock, true, false, wind);
		}
	}
	return;
fail:
	delete_milres_parm(p);

	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
	
}
#endif

/*
 * client still running dialog
 */
// static struct xa_window *csr_win = NULL;

static int
csr_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_csr = NULL;
// 	csr_win = NULL;
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
	
// 	wt->current = fr->obj;
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
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
	struct helpthread_data *htd;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;

	if (!(htd = get_helpthread_data(client)))
		return;

	if (!htd->w_csr) // !csr_win)
	{
		TEDINFO *t;
		
		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, KILL_OR_WAIT), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		C.csr_client = running;
		
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
			
		wind = create_dwind(lock, 0, /*txt_shutdown*/" Shutdown ", client, wt, csr_form_exit, csr_destructor);
		if (!wind)
			goto fail;
		htd->w_csr = wind;
		wind->window_status |= XAWS_FLOAT;
		open_window(lock, wind, wind->r);		
	}
	else
	{
		wind = htd->w_csr;
		open_window(lock, wind, wind->r);
		if (wind != TOP_WINDOW/*window_list*/)
			top_window(lock, true, false, wind);
	}
	return;
fail:
	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
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
		struct helpthread_data *htd = lookup_xa_data_byname(&ce->client->xa_data, HTDNAME);
		
		if (htd && htd->w_csr)
		{
			close_window(lock, htd->w_csr);
			delayed_delete_window(lock, htd->w_csr);
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
			  NULL, /*txt_launch_prg*/"Launch Program",
			  handle_launcher, NULL, NULL);
}
#endif



// static struct xa_window *systemalerts_win = NULL;

/* double click now also available for internal handlers. */
static void
sysalerts_form_exit(struct xa_client *Client,
		    struct xa_window *wind,
		    struct widget_tree *wt,
		    struct fmd_result *fr)
{
	enum locks lock = 0;
	short item = aesobj_item(&fr->obj);

	switch (item)
	{
		case SYSALERT_LIST:
		{
			DIAGS(("taskmanager_form_exit: Moved the shit out of form_do() to here!"));
			if ( fr->md && ((aesobj_type(&fr->obj) & 0xff) == G_SLIST))
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, wt->tree, aesobj_item(&fr->obj), fr->md);
				else
					click_scroll_list(lock, wt->tree, aesobj_item(&fr->obj), fr->md);
			}
			break;
		}
		/* Empty the alerts list */
		case SALERT_CLEAR:
		{
			struct scroll_info *list = object_get_slist(wt->tree + SYSALERT_LIST);
			struct sesetget_params p = { 0 };
			
			p.arg.txt = /*txt_alerts*/"Alerts";
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			if (p.e)
				list->empty(list, p.e, 0);
			object_deselect(wt->tree + item);
			redraw_toolbar(lock, wind, SYSALERT_LIST);
			redraw_toolbar(lock, wind, item);
			break;
		}
		case SALERT_OK:
		{
			object_deselect(wt->tree + item);
			redraw_toolbar(lock, wind, item);
			close_window(lock, wind);
// 			delayed_delete_window(lock, wind);	
			break;
		}
	}
}

static int
systemalerts_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_sysalrt = NULL;
// 	systemalerts_win = NULL;
	return true;
}

static void
refresh_systemalerts(OBJECT *form)
{
	OBJECT *sl = form + SYSALERT_LIST;
	SCROLL_INFO *list = object_get_slist(sl);

	list->slider(list, true);
}

void
open_systemalerts(enum locks lock, struct xa_client *client, bool open)
{
	struct helpthread_data *htd;
	OBJECT *obtree = NULL;
	struct widget_tree *wt = NULL;
	struct xa_window *wind;
	RECT remember = { 0, 0, 0, 0 };

	if (!(htd = get_helpthread_data(client)))
		return;

	if (!htd->w_sysalrt) //(!systemalerts_win)
	{
		struct scroll_info *list;
		RECT or;
		
		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, SYS_ERROR), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;

		list = set_slist_object(0, wt, NULL, SYSALERT_LIST,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_TREEVIEW|SIF_AUTOOPEN,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, 255);
		
		if (!list) goto fail;
		
		obj_init_focus(wt, OB_IF_RESET);
		
		{
// 			struct scroll_info *list = object_get_slist(obtree + SYSALERT_LIST);
			struct scroll_content sc = {{ 0 }};
			char a[] = /*txt_alerts*/"Alerts";
			char e[] = /*txt_environment*/"Environment";

			sc.t.text = a;
			sc.t.strings = 1;
			sc.icon = obtree + SALERT_IC1;
			sc.usr_flags = 1;
			sc.xflags = OF_AUTO_OPEN;
			DIAGS(("Add Alerts entry..."));
			list->add(list, NULL, NULL, &sc, 0, SETYP_STATIC, NOREDRAW);
			sc.t.text = e;
			sc.icon = obtree + SALERT_IC2;
			DIAGS(("Add Environment entry..."));
			list->add(list, NULL, NULL, &sc, 0, SETYP_STATIC, NOREDRAW);
		}
		{
// 			struct scroll_info *list = object_get_slist(obtree + SYSALERT_LIST);
			struct scroll_entry *this;
			const char **strings = get_raw_env(); //char * const * const strings = get_raw_env();
			int i;
			struct sesetget_params p = { 0 };
			struct scroll_content sc = {{ 0 }};

			p.idx = -1;
			p.arg.txt = /*txt_environment*/"Environment";
			p.level.flags = 0;
			p.level.curlevel = 0;
			p.level.maxlevel = 0;
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			list->empty(list, p.e, 0);
			this = p.e;
			sc.t.strings = 1;
			for (i = 0; strings[i]; i++)
			{	sc.t.text = strings[i];
				list->add(list, this, NULL, &sc, this ? (SEADD_CHILD|SEADD_PRIOR) : SEADD_PRIOR, 0, true);		
			}
		}

		obj_rectangle(wt, aesobj(obtree, 0), &or);

		obtree[SALERT_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, client, WC_BORDER,
						CLOSER|NAME, created_for_AES,
						client->options.thinframe,
						client->options.thinwork,
						*(RECT *)&or);
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag,
					do_formwind_msg,
					client,
					false,
					CLOSER|NAME|TOOLBAR|hide_move(&(client->options)),
					created_for_AES,
					client->options.thinframe,
					client->options.thinwork,
					remember, NULL, NULL);
		if (!wind) goto fail;
		
		list->set(list, NULL, SESET_PRNTWIND, (long)wind, NOREDRAW);
		
		/* Set the window title */
		set_window_title(wind, /*wint_sysalert*/" System window & Alerts log", false);

		wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		wt->exit_form = sysalerts_form_exit;
		/* Set the window destructor */
		wind->destructor = systemalerts_destructor;

		refresh_systemalerts(obtree);

		if (open)
			open_window(lock, wind, wind->r);

		htd->w_sysalrt = wind; //systemalerts_win = dialog_window;
	}
	else
	{
		wind = htd->w_sysalrt;
		if (open)
		{
			open_window(lock, wind, wind->r);
			if (wind != TOP_WINDOW/*window_list*/)
				top_window(lock, true, false, wind);
		}
	}
	return;
fail:
	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);

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
			post_cevent(C.Hlp, ceExecfunc, open_about,NULL, 1,0, NULL,NULL);
			break;

		/* Quit all applications */
		case SYS_MN_QUITAPP:
			DIAGS(("Quit all Apps"));
			quit_all_apps(lock, NULL, AP_TERM);
			break;

		/* Quit XaAES */
		case SYS_MN_QUIT:
			DIAGS(("Quit XaAES"));
			dispatch_shutdown(0, 0);
			break;

		/* Open the "Task Manager" window */
		case SYS_MN_TASKMNG:
			post_cevent(C.Hlp, ceExecfunc, open_taskmanager,NULL, 1,0, NULL,NULL);
			break;
		/* Open system alerts log window */
		case SYS_MN_SALERT:
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts,NULL, 1,0, NULL,NULL);
			break;
		/* Open system alerts log window filled with environment */
		case SYS_MN_ENV:
		{
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts, NULL, 1,0, NULL,NULL);
#if 0
			OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
			struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
			struct scroll_entry *this;
			const char **strings = get_raw_env(); //char * const * const strings = get_raw_env();
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
				list->add(list, this, NULL, &sc, this ? (SEADD_CHILD|SEADD_PRIOR) : SEADD_PRIOR, 0, true);		
			}
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts,NULL, 1,0, NULL,NULL);
#endif
			break;
		}

#if FILESELECTOR
		/* Launch a new app */
		case SYS_MN_LAUNCH:
		{
			post_cevent(C.Hlp, ceExecfunc, open_launcher,NULL, 1,0, NULL,NULL);
			break;
		}
#endif
		/* Launch desktop. */
		case SYS_MN_DESK:
		{
			if (C.DSKpid >= 0)
				ALERT((/*shell_running*/"XaAES: AES shell already running!"));
			else if (!*C.desk)
				ALERT((/*no_shell*/"XaAES: No AES shell set; See 'shell =' configuration variable in xaaes.cnf"));
			else
				C.DSKpid = launch(lock, 0,0,0, C.desk, "\0", C.Aes);
			break;
		}
		case SYS_MN_RESCHG:
		{
			if (C.reschange)
				post_cevent(C.Hlp, ceExecfunc, C.reschange,NULL, 1,0, NULL,NULL);
		}
	}
}
