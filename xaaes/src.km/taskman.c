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
#include "obtree.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_appl.h"
#include "xa_form.h"
#include "xa_shel.h"
#include "xa_rsrc.h"
#include "xa_fsel.h"
#include "trnfm.h"

#include "mint/signal.h"


static void
refresh_tasklist(enum locks lock)
{
	OBJECT *form = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	OBJECT *tl = form + TM_LIST;
	SCROLL_INFO *list = object_get_slist(tl);
	struct xa_client *client;

	/* Empty the task list */
	list->empty(list, NULL, -1);

	Sema_Up(clients);

	/* Add all current tasks to the list */
	FOREACH_CLIENT(client)
	{
		const size_t tx_len = 128;
		OBJECT *icon;
		char *tx;
		struct scroll_content sc = { 0 };

		/* default icon */
		icon = form + TM_ICN_XAAES;

		if (client->type & APP_ACCESSORY)
			icon = form + TM_ICN_MENU;

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
				sprintf(tx, tx_len, " %3d/   %s", client->p->pid, 0, client->name);

			sc.icon = icon;
			sc.text = tx;
			sc.n_strings = 1;
			list->add(list, NULL, NULL, &sc, false, FLAG_MAL, true);
		}
		else
		{
			sc.icon = icon;
			sc.text = client->name;
			sc.n_strings = 1;
			list->add(list, NULL, NULL, &sc, false, 0, true);
		}
	}

	list->slider(list, true);
	Sema_Dn(clients);
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

void
update_tasklist(enum locks lock)
{
	if (task_man_win)
	{
		DIAGS(("update_tasklist"));
		refresh_tasklist(lock);
		redraw_toolbar(lock, task_man_win, TM_LIST);
	}
}

static int
taskmanager_destructor(enum locks lock, struct xa_window *wind)
{
	OBJECT *form = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	OBJECT *ob = form + TM_LIST;
	SCROLL_INFO *list = object_get_slist(ob); //(SCROLL_INFO *)ob->ob_spec.index;

	list->empty(list, NULL, -1);
	
	task_man_win = NULL;

	return true;
}

/*
 * as long update_tasklist() is called for every *modification* of
 * the global client list this routine work correct
 */
static struct xa_client *
cur_client(SCROLL_INFO *list)
{
	SCROLL_ENTRY *this = list->start;
	struct xa_client *client = CLIENT_LIST_START;

	while (this != list->cur)
	{
		this = this->next;
		client = NEXT_CLIENT(client);
	}

	return client;
}

static void
send_terminate(enum locks lock, struct xa_client *client)
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
			 client->p->pid, AP_TERM, 0, 0);
}

void
quit_all_apps(enum locks lock, struct xa_client *except)
{
	struct xa_client *client;

	Sema_Up(clients);
	lock |= clients;

	FOREACH_CLIENT(client)
	{
		if (is_client(client) && client != except)
		{
			DIAGS(("shutting down %s", c_owner(client)));
			send_terminate(lock, client);
		}
	}

	Sema_Dn(clients);
}

void
quit_all_clients(enum locks lock, struct cfg_name_list *except_nl, struct xa_client *except_cl)
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
			send_terminate(lock, client);
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
	Sema_Up(clients);
	lock |= clients;
	
	wt->current = fr->obj;
	wt->which = 0;

	switch (fr->obj)
	{
		case TM_LIST:
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
		case TM_KILL:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: KILL for %s", c_owner(client)));

			if (is_client(client))
				ikill(client->p->pid, SIGKILL);

			object_deselect(wt->tree + TM_KILL);
			redraw_toolbar(lock, task_man_win, TM_KILL);
			break;
		}
		case TM_TERM:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: TM_TERM for %s", c_owner(client)));

			if (is_client(client))
				send_terminate(lock, client);

			object_deselect(wt->tree + TM_TERM);
			redraw_toolbar(lock, task_man_win, TM_TERM);
			break;
		}
		case TM_SLEEP:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = object_get_slist(ob);
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: TM_SLEEP for %s", c_owner(client)));
			ALERT(("taskmanager: TM_SLEEP not yet implemented!"));
			if (is_client(client))
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
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: TM_WAKE for %s", c_owner(client)));
			ALERT(("taskmanager: TM_WAKE not yet implemented!"));
			if (is_client(client))
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
			quit_all_apps(lock, NULL);

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
open_taskmanager(enum locks lock)
{
	static RECT remember = { 0,0,0,0 };

	struct xa_window *dialog_window;
	XA_TREE *wt;
	OBJECT *obtree = ResourceTree(C.Aes_rsc, TASK_MANAGER);

	wt = obtree_to_wt(C.Aes, obtree);

	if (!task_man_win)
	{
		obtree[TM_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			form_center(obtree, ICON_H);
			remember = calc_window(lock, C.Aes, WC_BORDER,
						CLOSER|NAME,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						*(RECT*)&obtree->ob_x);
		}

		/* Create the window */
		dialog_window = create_window(lock,
						do_winmesag, do_formwind_msg,
						C.Aes,
						false,
						CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
						created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						remember, NULL, NULL);

		/* Set the window title */
		set_window_title(dialog_window, " Task Manager ", false);

		wt = set_toolbar_widget(lock, dialog_window, C.Aes, obtree, -1, WIP_NOTEXT, NULL);
		wt->exit_form = taskmanager_form_exit;


		/* Set the window destructor */
		dialog_window->destructor = taskmanager_destructor;
	
		/* better position (to get sliders correct initially) */
		refresh_tasklist(lock);
		open_window(lock, dialog_window, dialog_window->r);
		task_man_win = dialog_window;
	}
	else if (task_man_win != window_list)
	{
		top_window(lock, true, task_man_win, (void *)-1L, NULL);
	}
}

static void
handle_launcher(enum locks lock, struct fsel_data *fs, const char *path, const char *file)
{
	char parms[200], *t;

	display(" '%s' '%s'", path, file);
	
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
open_launcher(enum locks lock)
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
	open_fileselector(lock, C.Aes, fs,
			  cfg.launch_path,
			  NULL, "Launch Program",
			  handle_launcher, NULL);
}
#endif

static struct xa_wtexture test_texture =
{
	0, 0, NULL,
};
static MFDB tmfdb;

static char imgpath[200] = { 0 };
extern struct xa_window_colours def_otop_wc;

#define TESTMASK (WCOL_DRAWBKG|WCOL_BOXED)

static void
handle_imgload(enum locks lock, struct fsel_data *fs, const char *path, const char *file)
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
	sprintf(imgpath, sizeof(imgpath), "%s%s", path, fs->fs_pattern);
	display("selected imgfile = '%s' in '%s'", file, path);
	
	load_image(parms+1, &tmfdb);
	
	if (tmfdb.fd_addr)
	{
		test_texture.mfdb = &tmfdb;
		def_otop_wc.title.n.texture = &test_texture;
		def_otop_wc.title.s.texture = &test_texture;
		def_otop_wc.title.flags &= ~TESTMASK;

		def_otop_wc.closer.n.texture = &test_texture;
		def_otop_wc.closer.s.texture = &test_texture;
		def_otop_wc.closer.flags &= ~TESTMASK;
		
		def_otop_wc.hider.n.texture = &test_texture;
		def_otop_wc.hider.s.texture = &test_texture;
		def_otop_wc.hider.flags &= ~TESTMASK;
		
		def_otop_wc.iconifier.n.texture = &test_texture;
		def_otop_wc.iconifier.s.texture = &test_texture;
		def_otop_wc.iconifier.flags &= ~TESTMASK;
		
		def_otop_wc.fuller.n.texture = &test_texture;
		def_otop_wc.fuller.s.texture = &test_texture;
		def_otop_wc.fuller.flags &= ~TESTMASK;
		
		def_otop_wc.sizer.n.texture = &test_texture;
		def_otop_wc.sizer.s.texture = &test_texture;
		def_otop_wc.sizer.flags &= ~TESTMASK;
		
		def_otop_wc.slider.n.texture = &test_texture;
		def_otop_wc.slider.s.texture = &test_texture;
		def_otop_wc.slider.flags &= ~(WCOL_DRAWBKG|WCOL_BOXED);
	
	}
	else
	{
		def_otop_wc.title.n.texture = NULL;
		def_otop_wc.title.s.texture = NULL;
		def_otop_wc.closer.n.texture = NULL;
		def_otop_wc.closer.s.texture = NULL;
		def_otop_wc.hider.n.texture = NULL;
		def_otop_wc.hider.s.texture = NULL;
		def_otop_wc.iconifier.n.texture = NULL;
		def_otop_wc.iconifier.s.texture = NULL;
		def_otop_wc.fuller.n.texture = NULL;
		def_otop_wc.fuller.s.texture = NULL;
		def_otop_wc.sizer.n.texture = NULL;
		def_otop_wc.sizer.s.texture = NULL;
	
		def_otop_wc.slider.n.texture = NULL;
		def_otop_wc.slider.s.texture = NULL;
	}
}

void
open_imgload(enum locks lock)
{
	struct fsel_data *fs;
	char *path = imgpath;

	display("open path '%s'", path);
	
	if (!*path)
	{
		path[0] = 'u'; //d_getdrv() + 'a';
		path[1] = ':'; //':';
		path[2] = '\\';
		path[3] = '*';
		path[4] = 0;
	}

	fs = &aes_fsel_data;
	open_fileselector(lock, C.Aes, fs,
			  path,
			  NULL, "Select image",
			  handle_imgload, NULL);
	
}


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
		/* Empty the task list */
		case SALERT_CLEAR:
		{
			struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
			list->empty(list, NULL, -1);
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
open_systemalerts(enum locks lock)
{
	static RECT remember = { 0, 0, 0, 0 };

	if (!systemalerts_win)
	{
		OBJECT *obtree = ResourceTree(C.Aes_rsc, SYS_ERROR);
		struct xa_window *dialog_window;
		XA_TREE *wt;

		wt = obtree_to_wt(C.Aes, obtree);

		obtree[SALERT_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			form_center(obtree, ICON_H);
			remember = calc_window(lock, C.Aes, WC_BORDER,
						CLOSER|NAME,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						*(RECT*)&obtree->ob_x);
		}

		/* Create the window */
		dialog_window = create_window(lock,
						do_winmesag,
						do_formwind_msg,
						C.Aes,
						false,
						CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
						created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						remember, NULL, NULL);

		/* Set the window title */
		set_window_title(dialog_window, " System window & Alerts log", false);

		wt = set_toolbar_widget(lock, dialog_window, C.Aes, obtree, -1, WIP_NOTEXT, NULL);
		wt->exit_form = sysalerts_form_exit;
		/* Set the window destructor */
		dialog_window->destructor = systemalerts_destructor;

		refresh_systemalerts(obtree);

		open_window(lock, dialog_window, dialog_window->r);
		systemalerts_win = dialog_window;
	}
	else if (systemalerts_win != window_list)
	{
		top_window(lock, true, systemalerts_win, (void *)-1L, NULL);
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
			open_about(lock);
			break;

		/* Quit all applications */
		case SYS_MN_QUITAPP:
			DIAGS(("Quit all Apps"));
			quit_all_apps(lock, NULL);
			break;

		/* Quit XaAES */
		case SYS_MN_QUIT:
			DIAGS(("Quit XaAES"));
			dispatch_shutdown(0);
			break;

		/* Open the "Task Manager" window */
		case SYS_MN_TASKMNG:
			open_taskmanager(lock);
			break;

		/* Open system alerts log window */
		case SYS_MN_SALERT:
			open_systemalerts(lock);
			break;

		/* Open system alerts log window filled with environment */
		case SYS_MN_ENV:
		{
			OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
			struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
			struct scroll_entry *this;
			char * const * const strings = get_raw_env();
			int i;
			struct seget_entrybyarg p = { 0 };
			struct scroll_content sc = { 0 };

			p.arg.txt = "Environment";
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			list->empty(list, p.e, 0);
			this = p.e;
			sc.n_strings = 1;
			for (i = 0; strings[i]; i++)
			{	sc.text = strings[i];
				list->add(list, this, NULL, &sc, this ? (SEADD_CHILD|SEADD_PRIOR) : SEADD_PRIOR, FLAG_AMAL, true);
			}

			open_systemalerts(lock);
			break;
		}

#if FILESELECTOR
		/* Launch a new app */
		case SYS_MN_LAUNCH:
			open_launcher(lock);
			break;
#endif

		/* Launch desktop. */
		case SYS_MN_DESK:
			if (*C.desk)
				C.DSKpid = launch(lock, 0, 0, 0, C.desk, "\0", C.Aes);
			break;
	}
}
