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

#include RSCHNAME

#include "k_shutdown.h"
#include "xa_global.h"

#include "k_main.h"
#include "k_keybd.h"
#include "c_window.h"
#include "init.h"
#include "nkcc.h"
#include "draw_obj.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_form.h"
#include "xa_rsrc.h"

#include "mint/signal.h"

static const char stillrun[] = " is still running.|Wait again or Kill?][Wait|Kill]";

static bool
kill_or_wait(struct xa_client *client)
{
	char atxt[80] = {"[1]["};
	const char *cn = client->name;

	while (*cn == 0x20)
		cn++;

	strcat(atxt, cn);
	strcat(atxt, stillrun);

	do_form_alert(0, C.Aes, 1, atxt);

	return false;
}

static void
CE_at_terminate(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
		ce->client->tp_term = 1;
}
/*
 * Cleanup on exit
 */
void
k_shutdown(void)
{
	long j = 0;

	DIAGS(("Cleaning up ready to exit...."));

	/* send all applications AP_TERM */
	quit_all_apps(NOLOCKING, NULL);

	/* wait until the clients are gone */
	DIAGS(("Wait for all clients to exit ..."));
	for (;;)
	{
		struct xa_client *client;
		int flag = 1;

		FOREACH_CLIENT(client)
		{
			if (client != C.Aes)
			{
				flag = 0;
				DIAGS(("client '%s' still running", client->name));
				Unblock(client, 1, 1);
			}
		}

		if (flag)
			break;

		//yield();
		/* sleep a second */
		//f_select(5000L, NULL, 0, 0);
		
		nap(50);

		if (j > 1000)
		{
			DIAGS(("Cleaning up clients"));

			FOREACH_CLIENT(client)
			{
				if (client != C.Aes)
				{
					DIAGS(("killing client '%s'", client->name));
					ikill(client->p->pid, SIGKILL);
				}
			}
		}
		j++;
	}
	DIAGS(("all clients have exited"));
#if 0
	DIAGS(("Freeing open windows"));
	{
		struct xa_window *w;

		w = window_list;
		while (w)
		{
			struct xa_window *next = w->next;
			close_window(NOLOCKING, w);
			delete_window(NOLOCKING, w);
			w = next;
		}
	}

	DIAGS(("Freeing closed windows"));
	{
		struct xa_window *w;

		w = S.closed_windows.first;
		while (w)
		{
			struct xa_window *next = w->next;
			delete_window(NOLOCKING, w);
			w = next;
		}
	}
#endif
	DIAGS(("Removing all remaining windows"));
	remove_all_windows(NOLOCKING, NULL);
	DIAGS(("Closing and deleting root window"));
	if (root_window)
	{
		close_window(NOLOCKING, root_window);
		delete_window(NOLOCKING, root_window);
		root_window = NULL;
	}

	DIAGS(("Freeing delayed deleted windows"));
	do_delayed_delete_window(NOLOCKING);

	post_cevent(C.Aes, CE_at_terminate, NULL,NULL, 0,0, NULL,NULL);
	yield();
	
	DIAGS(("Freeing Aes environment"));
	if (C.env)
	{
		kfree(C.env);
		C.env = NULL;
	}

	DIAGS(("Freeing Aes resources"));

	/* empty alert scrollbar */
	{
		OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
		if (form) empty_scroll_list(form, SYSALERT_LIST, -1);
	}

	/* To demonstrate the working on multiple resources. */
	FreeResources(C.Aes, 0);/* first: widgets */
	FreeResources(C.Aes, 0);/* then: big resource */

	/* just to be sure */
	if (C.button_waiter == C.Aes)
		C.button_waiter = NULL;

	cancel_app_aesmsgs(C.Aes);
	cancel_cevents(C.Aes);
	cancel_keyqueue(C.Aes);

	if (C.Aes->attach)
		kfree(C.Aes->attach);

	if (C.Aes->mnu_clientlistname)
		kfree(C.Aes->mnu_clientlistname);

	free_wtlist(C.Aes);
	kfree(C.Aes);

	C.Aes = NULL;

	DIAGS(("Freeing cnf stuff"));
	{
		int i;

		if (cfg.cnf_shell)
			kfree(cfg.cnf_shell);

		if (cfg.cnf_shell_arg)
			kfree(cfg.cnf_shell_arg);

		for (i = 0; i < sizeof(cfg.cnf_run)/sizeof(cfg.cnf_run[0]); i++)
		{
			if (cfg.cnf_run[i])
				kfree(cfg.cnf_run[i]);

			if (cfg.cnf_run_arg[i])
				kfree(cfg.cnf_run_arg[i]);
		}
	}

	DIAGS(("Freeing Options"));
	{
		struct opt_list *op;

		op = S.app_options;
		while (op)
		{
			struct opt_list *next = op->next;

			kfree(op);
			op = next;
		}
	}

	xaaes_kmalloc_leaks();
	nkc_exit();

#if 0
	/* Remove semaphores: */
	destroy_semaphores(log);
#endif

#if GENERATE_DIAGS
	DIAGS(("C.shutdown = 0x%x", C.shutdown));

	if (C.shutdown & HALT_SYSTEM)
		DIAGS(("HALT_SYSTEM flag is set"));

	if (C.shutdown & REBOOT_SYSTEM)
		DIAGS(("REBOOT_SYSTEM flag is set"));
#endif

	DIAGS(("Bye!"));
	DIAGS((""));

#if GENERATE_DIAGS
	/* Close the debug output file */
	if (D.debug_file)
	{
		kernel_close(D.debug_file);
		D.debug_file = NULL;
	}
#endif

	t_color(G_BLACK);
	wr_mode(MD_REPLACE);

	/* Shut down the VDI */
	v_clrwk(C.vh);

	if (cfg.auto_program)
	{
		/* v_clswk bombs with NOVA VDI 2.67 & Memory Protection.
		 * so I moved this to the end of the xaaes_shutdown,
		 * AFTER closing the debugfile.
		 */
		v_clsvwk(C.vh);
		v_enter_cur(C.P_handle);	/* Ozk: Lets enter cursor mode */
		v_clswk(C.P_handle);		/* Auto version must close the physical workstation */

		display("\033e\033H");		/* Cursor enable, cursor home */
	}
	else
	{
		v_clsvwk(C.vh);
		mt_appl_exit(my_global_aes);
	}

	DIAGS(("leaving k_shutdown()"));
}
