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

#include "k_shutdown.h"
#include "xa_global.h"

#include "xaaes.h"

#include "app_man.h"
#include "cnf_xaaes.h"
#include "k_main.h"
#include "k_keybd.h"
#include "c_window.h"
#include "init.h"
#include "nkcc.h"
#include "draw_obj.h"
#include "render_obj.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "menuwidg.h"
#include "obtree.h"
#include "xa_rsrc.h"

#include "mint/signal.h"
#include "mint/ssystem.h"


static void
CE_at_terminate(int lock, struct c_event *ce, short cancel)
{
// 	display("at_terminate %d, %lx", cancel, ce->client);
	if (!cancel)
		ce->client->tp_term = 3;
}

/*
 * Cleanup on exit
 */
void
k_shutdown(void)
{
	struct xa_vdi_settings *v = C.Aes->vdi_settings;

	if( cfg.save_windows )
		write_inf();
	_s_ync();
	if( !(C.shutdown & (HALT_SYSTEM | REBOOT_SYSTEM | COLDSTART_SYSTEM)) )
	{
		BLOG((false, "wait for AES help thread to terminate....(ferr=%d)", ferr));
		cancel_reiconify_timeout();

		if (!ferr && C.Hlp)
		{
			const volatile struct xa_client **h = (const volatile struct xa_client **)(const void**)&C.Hlp;
			long l = 0;
			post_cevent(C.Hlp, CE_at_terminate, NULL, NULL, 0,0, NULL, NULL);
			while (*h && l++ < 1500)
			{
				Unblock(C.Hlp, 0);
				nap( 1500 );
				//yield();
			}
			if( *h )
				BLOG((0,"failed!" ));
		}
		/* To demonstrate the working on multiple resources. */
		while (C.Aes->resources)
			FreeResources(C.Aes, NULL, NULL);

		BLOG((false, "Removing all remaining windows"));
		remove_all_windows(NOLOCKING, NULL);
		BLOG((false, "Freeing delayed deleted windows"));
		do_delayed_delete_window(NOLOCKING);

#if WITH_BBL_HELP
		if (bgem_window)
		{
			close_window(NOLOCKING, bgem_window);
			delete_window(NOLOCKING, bgem_window);
			bgem_window = NULL;
		}
#endif
		if (menu_window)
		{
			close_window(NOLOCKING, menu_window);
			delete_window(NOLOCKING, menu_window);
			menu_window = NULL;
		}
		if (root_window)
		{
			BLOG((false, "Closing and deleting root window"));
			close_window(NOLOCKING, root_window);
			delete_window(NOLOCKING, root_window);
			root_window = NULL;
			S.open_windows.root = NULL;
		}
#if WITH_BKG_IMG
		do_bkg_img( C.Aes, 2, 0 );
#endif
		BLOG((false, "shutting down aes thread .. tp=%lx", (unsigned long)C.Aes->tp));

		if (C.Aes->tp)
		{
			const volatile struct proc **h = (const volatile struct proc **)(const void**)&C.Aes->tp;
			post_cevent(C.Aes, CE_at_terminate, NULL,NULL, 0,0, NULL,NULL);
			while (*h)
			{
				Unblock(C.Aes, 0);
				yield();
			}
		}

		BLOG((false, "Freeing Aes environment"));
		if (C.env)
		{
			kfree(C.env);
			C.env = NULL;
		}

		/* just to be sure */
		if (C.button_waiter == C.Aes)
			C.button_waiter = NULL;

		BLOG((false, "cancel aesmsgs"));
		cancel_app_aesmsgs(C.Aes);
		BLOG((false, "cancel cevents"));
		cancel_cevents(C.Aes);
		BLOG((false, "cancel keyqueue"));
		cancel_keyqueue(C.Aes);

		BLOG((false, "freeing attachements"));
		if (C.Aes->attach)
			kfree(C.Aes->attach);

		BLOG((false, "free clientlistname"));
		if (C.Aes->mnu_clientlistname)
			kfree(C.Aes->mnu_clientlistname);

		/*
		 * Exit the widget theme module
		 */
		if (C.Aes->xmwt && C.Aes->wtheme_handle)
		{
			BLOG((false, "Exit widget theme module"));
			exit_client_widget_theme(C.Aes);
			(*C.Aes->xmwt->exit_module)(C.Aes->wtheme_handle);
			C.Aes->wtheme_handle = NULL;
		}

		/*
		 * Free the wind_calc() cache
		 */
		BLOG((false, "Freeing wind_calc cache"));
		delete_wc_cache(&C.Aes->wcc);
		/*
		 * Freeing the WT list is the last thing to do. Modules may attach
		 * widget_tree's to C.Aes
		 */
		BLOG((false, "freeing attachments"));
		free_attachments(C.Aes);

		/*
		 * Exit the object render module
		 */
		BLOG((false, "Exit object render module"));
		if (C.Aes->objcr_module)
		{
			exit_client_objcrend( C.Aes );
			(*C.Aes->objcr_module->exit_module)();
		}

		BLOG((false, "freeing wtlist"));
		free_wtlist(C.Aes);
		BLOG((false, "Free main XaAES client structure"));
		kfree(C.Aes);
		C.Aes = NULL;

		free_desk_popup();

		BLOG((false, "Freeing cnf stuff"));
		{
			int i;

			free_namelist(&cfg.ctlalta);
			free_namelist(&cfg.kwq);

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

		BLOG((false, "Freeing Options"));
		{
		struct opt_list *op;

		op = S.app_options;
		while (op)
		{
			struct opt_list *next = op->next;

			kfree(op);
			op = next;
		}

		xaaes_kmalloc_leaks();

		nkc_exit();
		}

		BLOG((false, "C.shutdown = 0x%x", C.shutdown));
		BLOGif(C.shutdown & HALT_SYSTEM, (false, "HALT_SYSTEM flag is set"));
		BLOGif(C.shutdown & REBOOT_SYSTEM, (false, "REBOOT_SYSTEM flag is set"));
		BLOGif(C.shutdown & RESOLUTION_CHANGE, (false, "RESOLUTION_CHANGE flag is set"));

		/*
		 * Close the virtual used by XaAES
		 */
		if (v && v->handle)
		{
			BLOG((false, "Closing down vwk %d", v->handle));
			v_clsvwk(v->handle);
		}
		/*
		 * Close the physical
		 */
		if (C.P_handle > 0 && C.f_phys == 0 )
		{
			vst_color(C.P_handle, G_BLACK);
			vswr_mode(C.P_handle, MD_REPLACE);

			/* Shut down the VDI */
			v_clrwk(C.P_handle);

			/* v_clswk bombs with NOVA VDI 2.67 & Memory Protection.
			 * so I moved this to the end of the xaaes_shutdown,
			 * AFTER closing the debugfile.
			 */

			{
#ifndef ST_ONLY
#if SAVE_CACHE_WK
				unsigned long sc = 0, cm = 0;
#endif
				int odbl;

				BLOG((false, "Closing down physical vdi workstation %d", C.P_handle));
			/*
			 * Ozk: We switch off instruction, data and branch caches (where available)
			 *	while the VDI accesses the hardware. This fixes 'black-screen'
			 *	problems on Hades with Nova VDI.
			 */
#if SAVE_CACHE_WK
				cm = s_system(S_CTRLCACHE, 0L, -1L);
				sc = s_system(S_CTRLCACHE, -1L, 0L);
				BLOG((false, "Get current cpu cache settings... cm = %lx, sc = %lx", cm, sc));
				s_system(S_CTRLCACHE, sc & ~7, cm);
#endif
				//v_enter_cur(C.P_handle);	/* Ozk: Lets enter cursor mode */
				BLOG((false, "Closing VDI workstation %d", C.P_handle));
				odbl = DEBUG_LEVEL;
				DEBUG_LEVEL = 4;
				v_clswk(C.P_handle);		/* Auto version must close the physical workstation */
				BLOG((false, "VDI workstation closed"));
#if SAVE_CACHE_WK
				BLOG((0,"Restore CPU caches:%lx,%lx",sc,cm));
				s_system(S_CTRLCACHE, sc, cm);
#endif
				DEBUG_LEVEL = odbl;
				BLOG((false, "Done shutting down VDI"));
#else
				v_enter_cur(C.P_handle);
				v_clswk(C.P_handle);
#endif
			}
			if( !ferr )
				c_conws("\033e\033E");		/* Cursor enable, cursor home */
		}
	}
	BLOG((false, "leaving k_shutdown()"));
}
