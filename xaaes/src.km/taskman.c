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
#include "k_main.h"
#include "objects.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_appl.h"
#include "xa_form.h"
#include "xa_shel.h"
#include "xa_rsrc.h"
#include "xa_fsel.h"

#include "mint/signal.h"


static void
refresh_tasklist(enum locks lock)
{
	OBJECT *form = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	OBJECT *tl = form + TM_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)tl->ob_spec.index;
	struct xa_client *client;

	/* Empty the task list */
	empty_scroll_list(form, TM_LIST, -1);

	Sema_Up(clients);

	/* Add all current tasks to the list */
	client = S.client_list;
	while (client)
	{
		OBJECT *icon;
		char *tx;

#if 0
		if (client->msg)
		{
			icon = form + TM_ICN_MESSAGE;
		}
		else if (   client->pid == S.mouse_lock
		         || client->pid == S.update_lock)
		{
			icon = form + TM_ICN_LOCK;
		}
		else
#endif

		if (client->type == APP_ACCESSORY)
			icon = form + TM_ICN_MENU;
		else
			icon = form + TM_ICN_XAAES;

		tx = kmalloc(128);
		if (tx)
		{
			long prio = p_getpriority(0, client->p->pid);
			if (prio >= 0)
				sprintf(tx, 128, " %d/%ld %s", client->p->pid, prio-20, client->name);
			else
				sprintf(tx, 128, " %d/E%ld %s", client->p->pid, prio, client->name);

			add_scroll_entry(form, TM_LIST, icon, tx, FLAG_MAL);
		}
		else
			add_scroll_entry(form, TM_LIST, icon, client->name, 0);

		client = client->next;
	}

	list->slider(list);
	Sema_Dn(clients);
}

static struct xa_window *task_man_win = NULL;

void
update_tasklist(enum locks lock)
{
	if (task_man_win)
	{
		DIAGS(("update_tasklist"));
		refresh_tasklist(lock);
		display_toolbar(lock, task_man_win, TM_LIST);
	}
}

static int
taskmanager_destructor(enum locks lock, struct xa_window *wind)
{
	OBJECT *ob = ResourceTree(C.Aes_rsc, TASK_MANAGER) + TM_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;

	delayed_delete_window(lock, list->wi);
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
	struct xa_client *client = S.client_list;

	while (this != list->cur)
	{
		this = this->next;
		client = client->next;
	}
	return client;
}

static void
send_terminate(enum locks lock, struct xa_client *client)
{
	if (client->type == APP_ACCESSORY)
	{
		DIAGS(("send AC_CLOSE to %s", c_owner(client)));

		/* Due to ambiguities in documentation the pid is filled out
		 * in both msg[3] and msg[4]
		 */
		send_app_message(lock, NULL, client,
				 AC_CLOSE,    0, 0, client->p->pid,
				 client->p->pid, 0, 0, 0);

		remove_windows(lock, client);
	}

	/* XXX
	 * should we only send if client->apterm is true???
	 */
	DIAGS(("send AP_TERM to %s", c_owner(client)));
	send_app_message(lock, NULL, client,
			 AP_TERM,     0,       0, client->p->pid,
			 client->p->pid, AP_TERM, 0, 0);
}

void
quit_all_apps(enum locks lock, struct xa_client *except)
{
	struct xa_client *client;

	Sema_Up(clients);
	lock |= clients;

	client = S.client_list;
	while (client)
	{
		if (is_client(client) && client != except)
		{
			DIAGS(("shutting down %s", c_owner(client)));
			send_terminate(lock, client);
		}

		client = client->next;
	}

	Sema_Dn(clients);
}

/* double click now also available for internal handlers. */
static void
handle_taskmanager(enum locks lock, struct widget_tree *wt)
{
	wt->current &= 0xff;

	Sema_Up(clients);
	lock |= clients;
	
	switch (wt->current)
	{
		case TM_KILL:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: KILL for %s", c_owner(client)));

			if (is_client(client))
				ikill(client->p->pid, SIGKILL);

			deselect(wt->tree, TM_KILL);
			display_toolbar(lock, task_man_win, TM_KILL);
			break;
		}
		case TM_TERM:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: TM_TERM for %s", c_owner(client)));

			if (is_client(client))
				send_terminate(lock, client);

			deselect(wt->tree, TM_TERM);
			display_toolbar(lock, task_man_win, TM_TERM);
			break;
		}
		case TM_SLEEP:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: TM_SLEEP for %s", c_owner(client)));
			ALERT(("taskmanager: TM_SLEEP not yet implemented!"));
			if (is_client(client))
			{
			}

			deselect(wt->tree, TM_SLEEP);
			display_toolbar(lock, task_man_win, TM_SLEEP);
			break;
		}
		case TM_WAKE:
		{
			OBJECT *ob = wt->tree + TM_LIST;
			SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;
			struct xa_client *client = cur_client(list);

			DIAGS(("taskmanager: TM_WAKE for %s", c_owner(client)));
			ALERT(("taskmanager: TM_WAKE not yet implemented!"));
			if (is_client(client))
			{
			}

			deselect(wt->tree, TM_WAKE);
			display_toolbar(lock, task_man_win, TM_WAKE);
			break;
		}

		/* the bottom action buttons */
		case TM_QUITAPPS:
		{
			DIAGS(("taskmanager: quit all apps"));
			quit_all_apps(lock, NULL);

			deselect(wt->tree, TM_QUITAPPS);
			display_toolbar(lock, task_man_win, TM_QUITAPPS);
			break;
		}
		case TM_QUIT:
		{
			DIAGS(("taskmanager: quit XaAES"));
			dispatch_shutdown(0);

			deselect(wt->tree, TM_QUIT);
			display_toolbar(lock, task_man_win, TM_QUIT);
			break;
		}
		case TM_REBOOT:
		{
			DIAGS(("taskmanager: reboot system"));
			dispatch_shutdown(REBOOT_SYSTEM);

			deselect(wt->tree, TM_REBOOT);
			display_toolbar(lock, task_man_win, TM_REBOOT);
			break;
		}
		case TM_HALT:
		{
			DIAGS(("taskmanager: halt system"));
			dispatch_shutdown(HALT_SYSTEM);

			deselect(wt->tree, TM_HALT);
			display_toolbar(lock, task_man_win, TM_HALT);
			break;
		}
		case TM_OK:
		{
			deselect(wt->tree, TM_OK);
			display_toolbar(lock, task_man_win, TM_OK);

			/* and release */
			close_window(lock, task_man_win);
			delete_window(lock, task_man_win);
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
	OBJECT *form = ResourceTree(C.Aes_rsc, TASK_MANAGER);


	if (!task_man_win)
	{
		form[TM_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			center_form(form, ICON_H);
			remember = calc_window(lock, C.Aes, WC_BORDER,
						CLOSER|NAME,
						MG,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						*(RECT*)&form->ob_x);
		}

		/* Create the window */
		dialog_window = create_window(lock, NULL,
						C.Aes,
						false,
						CLOSER|NAME|TOOLBAR|hide_move(&default_options),
						created_for_AES,
						MG,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						remember, NULL, &remember);

		/* Set the window title */
		get_widget(dialog_window, XAW_TITLE)->stuff = " Task Manager";

		wt = set_toolbar_widget(lock, dialog_window, form, -1);
		wt->exit_form = XA_form_exit;
		wt->exit_handler = handle_taskmanager;

		/* set a scroll list widget */
		set_slist_object(lock, wt, form, TM_LIST, NULL, NULL, NULL, NULL, "Client Applications", NULL, NICE_NAME);

		/* Set the window destructor */
		dialog_window->destructor = taskmanager_destructor;
	
		/* better position (to get sliders correct initially) */
		refresh_tasklist(lock);
		open_window(lock, dialog_window, dialog_window->r);
		task_man_win = dialog_window;
	}
	else if (task_man_win != window_list)
	{
		C.focus = pull_wind_to_top(lock, task_man_win);
		after_top(lock, true);
		display_window(lock, 100, task_man_win, NULL);
	}
}

static void
handle_launcher(enum locks lock, const char *path, const char *file)
{
	extern char fs_pattern[];
	char parms[200], *t;
	
	sprintf(parms+1, sizeof(parms)-1, "%s%s", path, file);
	parms[0] = '\0';

	for(t = parms + 1; *t; t++)
	{
		if (*t == '/')
			*t = '\\';
	}		

	close_fileselector(lock);

	DIAGS(("launch: \"%s\"", parms+1));
	sprintf(cfg.launch_path, sizeof(cfg.launch_path), "%s%s", path, fs_pattern);
	launch(lock, 0, 0, 0, parms+1, parms, C.Aes);
}

#if FILESELECTOR
static void
open_launcher(enum locks lock)
{
	if (!*cfg.launch_path)
	{
		cfg.launch_path[0] = d_getdrv() + 'a';
		cfg.launch_path[1] = ':';
		cfg.launch_path[2] = '\\';
		cfg.launch_path[3] = '*';
		cfg.launch_path[4] = 0;
	}

	open_fileselector(lock, C.Aes,
			  cfg.launch_path,
			  NULL, "Launch Program",
			  handle_launcher, NULL);
}
#endif

static struct xa_window *systemalerts_win = NULL;

/* double click now also available for internal handlers. */
static void
handle_systemalerts(enum locks lock, struct widget_tree *wt)
{
	OBJECT *form = wt->tree;
	int item = wt->current & 0xff;

	switch (item)
	{
		/* Empty the task list */
		case SALERT_CLEAR:
		{
			empty_scroll_list(form, SYSALERT_LIST, -1);
			deselect(wt->tree, item);
			display_toolbar(lock, systemalerts_win, SYSALERT_LIST);
			display_toolbar(lock, systemalerts_win, item);
			break;
		}
		case SALERT_OK:
		{
			deselect(wt->tree, item);
			display_toolbar(lock, systemalerts_win, item);
			close_window(lock, systemalerts_win);
			delayed_delete_window(lock, systemalerts_win);	
			break;
		}
	}
}

static int
systemalerts_destructor(enum locks lock, struct xa_window *wind)
{
	OBJECT *ob = ResourceTree(C.Aes_rsc, SYS_ERROR) + SYSALERT_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;

	delayed_delete_window(lock, list->wi);
	systemalerts_win = NULL;
	return true;
}

static void
refresh_systemalerts(OBJECT *form)
{
	OBJECT *sl = form + SYSALERT_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)sl->ob_spec.index;

	list->slider(list);
}

static void
open_systemalerts(enum locks lock)
{
	struct xa_window *dialog_window;
	XA_TREE *wt;
	OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
	static RECT remember = {0,0,0,0};
	
	if (!systemalerts_win)
	{
		form[SALERT_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			center_form(form, ICON_H);
			remember = calc_window(lock, C.Aes, WC_BORDER,
						CLOSER|NAME,
						MG,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						*(RECT*)&form->ob_x);
		}

		/* Create the window */
		dialog_window = create_window(lock, NULL,
						C.Aes,
						false,
						CLOSER|NAME|TOOLBAR|hide_move(&default_options),
						created_for_AES,
						MG,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						remember, NULL, &remember);

		/* Set the window title */
		get_widget(dialog_window, XAW_TITLE)->stuff = " System window & Alerts Log";
		wt = set_toolbar_widget(lock, dialog_window, form, -1);
		wt->exit_form = XA_form_exit;
		wt->exit_handler = handle_systemalerts;

		/* HR: set a scroll list widget */
		set_slist_object(lock, wt, form, SYSALERT_LIST, NULL, NULL, NULL, NULL, NULL, NULL, 256);

		/* Set the window destructor */
		dialog_window->destructor = systemalerts_destructor;

		refresh_systemalerts(form);

		open_window(lock, dialog_window, dialog_window->r);
		systemalerts_win = dialog_window;
	}
	else if (systemalerts_win != window_list)
	{
		C.focus = pull_wind_to_top(lock, systemalerts_win);
		after_top(lock, true);
		display_window(lock, 101, systemalerts_win, NULL);
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
			char * const * const strings = get_raw_env();
			int i;

			empty_scroll_list(form, SYSALERT_LIST, FLAG_ENV);

			for (i = 0; strings[i]; i++)
				add_scroll_entry(form, SYSALERT_LIST, NULL, strings[i], FLAG_ENV);

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
