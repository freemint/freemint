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

#include "k_shutdown.h"
#include "xa_global.h"

#include "c_window.h"
#include "init.h"
#include "my_aes.h"
#include "nkcc.h"
#include "objects.h"
#include "xalloc.h"
#include "xa_rsrc.h"

#include "mint/signal.h"


/*
 * Cleanup on exit
 */
void
k_shutdown(void)
{
	DIAGS(("Cleaning up ready to exit...."));

	DIAGS(("Cleaning up clients"));
	{
		struct xa_client *client;

		client = S.client_list;
		while (client)
		{
			if (client != C.Aes)
				ikill(client->p->pid, SIGKILL);

			client = client->next;
		}
	}

	/* wait until the clients are gone */
	DIAGS(("Wait for all clients to exit ..."));
	for (;;)
	{
		struct xa_client *client = S.client_list;
		int flag = 0;

		while (client)
		{
			if (client != C.Aes)
				flag = 1;

			client = client->next;
		}

		if (!flag)
			break;

		yield();
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

	DIAGS(("Freeing Aes environment"));
	if (C.env)
		free(C.env);

	DIAGS(("Freeing Aes resources"));
	/* To demonstrate the working on multiple resources. */
	FreeResources(C.Aes, 0);/* first:  widgets */
	FreeResources(C.Aes, 0);/* then:   big resource */

	DIAGS(("Freeing Options"));
	{
		struct opt_list *op;

		op = S.app_options;
		while (op)
		{
			struct opt_list *next = op->next;

			free(op);
			op = next;
		}
	}

#if GENERATE_DIAGS
	DIAGS(("Reporting memory leaks -> XXX todo"));
#endif

	DIAGS(("Freeing what's left"));
	/* dummy, just report leaked memory */
	free_all();

	nkc_exit();

#if 0
	/* Remove semaphores: */
	destroy_semaphores(log);
#endif

	DIAGS(("Bye!"));
	DIAGS((""));

	if (log)
		kernel_close(log);

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

		// display("\033e\033H");		/* Cursor enable, cursor home */
	}
	else
	{
		v_clsvwk(C.vh);
		mt_appl_exit(my_global_aes);
	}
}
