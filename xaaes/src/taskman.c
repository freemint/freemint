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

#include RSCHNAME

#include <mint/mintbind.h>
#include <signal.h>

#include "xa_types.h"
#include "xa_global.h"

#include "about.h"
#include "c_window.h"
#include "objects.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xalloc.h"
#include "xa_form.h"
#include "xa_shel.h"
#include "xa_rsrc.h"
#include "xa_fsel.h"
#include "xa_clnt.h"


/*
 * General system dialogs
 * (Error Log and Task Manager)
 */

void
refresh_tasklist(LOCK lock)
{
	OBJECT *form = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	XA_CLIENT *client;
	OBJECT *icon;
	OBJECT *tl = form + TM_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)tl->ob_spec.index;
	char *tx;
	int counter=0;

	/* Empty the task list */
	empty_scroll_list(form, TM_LIST, -1);

	Sema_Up(clients);

	/* Add all current tasks to the list */
	client = S.client_list;
	while (client)
	{
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

#if 1
		tx = xmalloc(128,11);
		if (tx)
		{
			long prio = Pgetpriority(0, client->pid);
			if (prio >= 0)
				sdisplay(tx, " %d/%ld %s", client->pid, prio-20, client->name);
			else
				sdisplay(tx, " %d/E%ld %s", client->pid, prio, client->name);
				
			add_scroll_entry(form, TM_LIST, icon, tx, FLAG_MAL);
		}
		else
#endif
			add_scroll_entry(form, TM_LIST, icon, client->name, 0);
		client = client->next;
		counter++;
	}

	list->slider(list);
	if ( counter==1 && C.shutdown)  /* now all programs are quitted */
		C.shutdown |= QUIT_NOW;

	Sema_Dn(clients);
}

XA_WINDOW *task_man_win = NULL;

static int
taskmanager_destructor(LOCK lock, struct xa_window *wind)
{
	OBJECT *ob = ResourceTree(C.Aes_rsc, TASK_MANAGER) + TM_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;

	delete_window(lock, list->wi);
	task_man_win = NULL;

	return true;
}

static XA_CLIENT *
cur_client(SCROLL_INFO *list)
{
	SCROLL_ENTRY *this = list->start;
	XA_CLIENT *client = S.client_list;
		
	while(this != list->cur)
	{
		this = this->next;
		client = client->next;
	}
	return client;
}

static void
send_terminate(LOCK lock, XA_CLIENT *client)
{
	if (is_client(client))
	{
		if (client->type == APP_ACCESSORY)
		{
			/* Due to ambiguities in documentation the pid is filled out in both msg[3] and msg[4] */
			DIAGS(("   --   AC_CLOSE\n"));
			send_app_message(lock, NULL, client,
					 AC_CLOSE,    0, 0, client->pid,
					 client->pid, 0, 0, 0);
			remove_windows(lock, client);
		}
		DIAGS(("   --   AP_TERM\n"));
		send_app_message(lock, NULL, client,
				 AP_TERM,     0,       0, client->pid,
				 client->pid, AP_TERM, 0, 0);
	}
}

/* HR 300101: double click now also available for internal handlers. */
static void
handle_taskmanager(LOCK lock, struct widget_tree *wt)
{
	OBJECT *ob = wt->tree + TM_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;
	XA_CLIENT *client;
	
	wt->current &= 0xff;

	Sema_Up(clients);

	lock |= clients;
	
	switch(wt->current)
	{
		case TM_TERM:
			client = cur_client(list);
			DIAGS(("TM_TERM for %s\n", c_owner(client)));
			send_terminate(lock, client);
			deselect(wt->tree, TM_TERM);
			display_toolbar(lock, task_man_win, TM_TERM);
			break;
		case TM_SHUT:
			DIAGS(("shutdown by taskmanager\n"));
			deselect(wt->tree, TM_SHUT);
			shutdown(lock);
			break;
		case TM_KILL:
			client = cur_client(list);		
			if (is_client(client))
			{
				(void) Pkill(client->pid, SIGKILL);
				(void) Fselect(200, NULL, NULL, NULL);
				refresh_tasklist(lock);
				display_toolbar(lock, task_man_win, TM_LIST);
			}
			deselect(wt->tree, TM_KILL);
			display_toolbar(lock, task_man_win, TM_KILL);
			break;
		case TM_OK:
			deselect(wt->tree, TM_OK);
			display_toolbar(lock, task_man_win, TM_OK);
			close_window(lock, task_man_win);
			delete_window(lock, task_man_win);
			break;
		case TM_QUIT:
			deselect(wt->tree, TM_QUIT);
			C.shutdown = QUIT_NOW;  /* quit now */
			break;
		case TM_HALT:
			deselect(wt->tree, TM_HALT);
			shutdown(lock);
			C.shutdown |= HALT_SYSTEM;
			refresh_tasklist(lock);
			break;
		case TM_REBOOT:
			deselect(wt->tree, TM_REBOOT);
			C.shutdown |= REBOOT_SYSTEM;
			shutdown(lock);
			refresh_tasklist(lock);
			break;
	}

	Sema_Dn(clients);
}

void
open_taskmanager(LOCK lock)
{
	XA_WINDOW *dialog_window;
	XA_TREE *wt;
	OBJECT *form = ResourceTree(C.Aes_rsc, TASK_MANAGER);
	static RECT remember = { 0,0,0,0 };


/*	i don´t see the use for this...
	if (shutdown) 
		form[TM_QUIT].ob_flags &= ~HIDETREE;
	else
		form[TM_QUIT].ob_flags |=  HIDETREE;
*/

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
handle_launcher(LOCK lock, char *path, char *file)
{
	extern char fs_pattern[];
	char parms[200], *t;
	
	sdisplay(parms + 1, "%s%s", path, file);
	parms[0] = '\0';
	for(t = parms + 1; *t; t++)
	{
		if (*t == '/')
			*t = '\\';
	}		

	close_fileselector(lock);

	DIAGS(("launch: \"%s\"\n", parms + 1));

	sdisplay(cfg.launch_path, "%s%s", path, fs_pattern);

	launch(lock, 0, 0, 0, parms + 1, parms, C.Aes);
}

#if FILESELECTOR
static void
open_launcher(LOCK lock)
{
	if (!*cfg.launch_path)
	{
		cfg.launch_path[0] = Dgetdrv() + 'a';
		cfg.launch_path[1] = ':';
		cfg.launch_path[2] = '\\';
		cfg.launch_path[3] = '*';
		cfg.launch_path[4] = 0;
	}

	open_fileselector(lock, C.Aes, cfg.launch_path, NULL, "Launch Program", handle_launcher, NULL);
}
#endif

static XA_WINDOW *systemalerts_win = NULL;

/* HR 300101: double click now also available for internal handlers. */
static void
handle_systemalerts(LOCK lock, struct widget_tree *wt)
{
	OBJECT *form = wt->tree;
	int item = wt->current & 0xff;

	switch (item)
	{
	/* Empty the task list */
	case SALERT_CLEAR:
		empty_scroll_list(form, SYSALERT_LIST, -1);
		deselect(wt->tree, item);
		display_toolbar(lock, systemalerts_win, SYSALERT_LIST);
		display_toolbar(lock, systemalerts_win, item);
		break;
	case SALERT_OK:
		deselect(wt->tree, item);
		display_toolbar(lock, systemalerts_win, item);
		close_window(lock, systemalerts_win);
		delete_window(lock, systemalerts_win);	
	}
}

static int
systemalerts_destructor(LOCK lock, struct xa_window *wind)
{
	OBJECT *ob = ResourceTree(C.Aes_rsc, SYS_ERROR) + SYSALERT_LIST;
	SCROLL_INFO *list = (SCROLL_INFO *)ob->ob_spec.index;
	delete_window(lock, list->wi);
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
open_systemalerts(LOCK lock)
{
	XA_WINDOW *dialog_window;
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
do_system_menu(LOCK lock, int clicked_title, int menu_item)
{
	switch(menu_item)
	{
		case SYS_MN_ABOUT: /* Open the "About XaAES..." dialog */
			open_about(lock);
			break;
	
		case SYS_MN_SHUTDOWN: /* Shutdown the system */
			DIAGS(("shutdown by menu\n"));
			shutdown(lock);
			break;
		case SYS_MN_QUIT: /* Quit XaAES */
			C.shutdown = QUIT_NOW;
			break;
		
		case SYS_MN_TASKMNG: /* Open the "Task Manager" window */
			open_taskmanager(lock);
			break;
		case SYS_MN_SALERT: /* Open system alerts log window */
			open_systemalerts(lock);
			break;
#if MN_ENV
		case SYS_MN_ENV:
		{
			OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
			int i;

			empty_scroll_list(form, SYSALERT_LIST, FLAG_ENV);

			for (i = 0; C.strings[i]; i++)
				add_scroll_entry(form, SYSALERT_LIST, NULL, C.strings[i], FLAG_ENV);

			open_systemalerts(lock);
		}
		break;
#endif
#if FILESELECTOR
		case SYS_MN_LAUNCH: /* Launch a new app */
			open_launcher(lock);
			break;
#endif
		case SYS_MN_DESK: /* Launch desktop. */
			if (*C.desk)
				C.DSKpid = launch(lock, 0, 0, 0, C.desk, "\0", C.Aes);
			break;
	}
}

/* find a pending alert */
void *
pendig_alerts(OBJECT *form, int item)
{
	OBJECT *ob = form + item;
	SCROLL_INFO *list = (SCROLL_INFO *)get_ob_spec(ob)->index;
	SCROLL_ENTRY *cur;

	cur = list->start;
	while (cur)
	{
		if (cur->flag & FLAG_PENDING)
		{
			cur->flag &= ~FLAG_PENDING;
			return cur->text;
		}
		cur = cur->next;
	}
	return NULL;
}

/* simple but mostly effective shutdown using the taskmanager. */
void
shutdown(LOCK lock)
{
	XA_CLIENT *client;

	Sema_Up(clients);

	lock |= clients;

	open_taskmanager(lock);

	client = S.client_list;
	while (client)
	{
		DIAGS(("shutting down %s\n", c_owner(client)));
		send_terminate(lock, client);
		client = client->next;
	}

	Sema_Dn(clients);
}

void
update_tasklist(LOCK lock)
{
	if (task_man_win)
	{
		DIAGS(("update_tasklist\n"));
		refresh_tasklist(lock);
		display_toolbar(lock, task_man_win, TM_LIST);
	}
}
