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
#include "c_window.h"
#include "init.h"
#include "nkcc.h"
#include "draw_obj.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_rsrc.h"

#include "mint/signal.h"


/*
 * Cleanup on exit
 */
void
k_shutdown(void)
{
	DIAGS(("Cleaning up ready to exit...."));

	/* send all applications AP_TERM */
	quit_all_apps(NOLOCKING, NULL);

	/* wait until the clients are gone */
	DIAGS(("Wait for all clients to exit ..."));
	for (;;)
	{
		struct xa_client *client;
		int flag = 1;

		client = S.client_list;
		while (client)
		{
			if (client != C.Aes)
			{
				flag = 0;
				DIAGS(("client '%s' still running", client->name));
				Unblock(client, 1, 1);
			}
			client = client->next;
		}

		if (flag)
			break;

		/* sleep a second */
		nap(1000);

		DIAGS(("Cleaning up clients"));
		{
			client = S.client_list;
			while (client)
			{
				if (client != C.Aes)
				{
					DIAGS(("killing client '%s'", client->name));
					ikill(client->p->pid, SIGKILL);
				}
				client = client->next;
			}
		}
	}
	DIAGS(("all clients have exited"));

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

	DIAGS(("Freeing delayed deleted windows"));
	do_delayed_delete_window(NOLOCKING);

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

	cancel_aesmsgs(&(C.Aes->rdrw_msg));
	cancel_aesmsgs(&(C.Aes->msg));
	cancel_cevents(C.Aes);

	if (C.Aes->attach)
		kfree(C.Aes->attach);

	if (C.Aes->mnu_clientlistname)
		kfree(C.Aes->mnu_clientlistname);

	free_wtlist(C.Aes);
	kfree(C.Aes);


	C.Aes = NULL;

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
		kernel_close(D.debug_file);
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
}
