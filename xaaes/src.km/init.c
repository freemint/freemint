/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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
#include WIDGHNAME

#include "init.h"
#include "xa_global.h"

#include "c_window.h"
#include "cnf_xaaes.h"
#include "handler.h"
#include "k_main.h"
#include "k_shutdown.h"
#include "my_aes.h"
#include "nkcc.h"
#include "objects.h"
#include "semaphores.h"
#include "xalloc.h"

#include "version.h"

#include "mint/ssystem.h"
#include "cookie.h"


static char *scls_name = "xaaes.cnf";

static char Aes_display_name[32];
Path Aes_home_path;

struct file *log = NULL;


/*
 * private data
 */

char version[] = ASCII_VERSION;

static void
bootmessage(unsigned long mint)
{
	fdisplay(log, "It's FreeMiNT v%ld.%ld.%ld%c%c  (0x%lx)",
		 (mint >> 24),
		 (mint >> 16) & 255,
		 (mint >>  8) & 255,
		 (mint & 255) ? '.' : ' ',
		 (mint & 255) ? mint & 255 : ' ', 
		 (mint));

	fdisplay(log, " and ");

	fdisplay(log, "%s", Aes_display_name);
	fdisplay(log, "MultiTasking AES for MiNT");
	fdisplay(log, "(w)1995,96,97,98,99 Craig Graham, ");
	fdisplay(log, "Johan Klockars, Martin Koehling, ");
	fdisplay(log, "Thomas Binder");
	fdisplay(log, "     and other assorted dodgy characters from around the world...");
	fdisplay(log, "   Using Harald Siegmunds NKCC");
	fdisplay(log, "   1999-2003 Henk Robbers @ Amsterdam");
	fdisplay(log, "        2004 Frank Naumann <fnaumann@freemint.de>");
	fdisplay(log, "Date: %s, time: %s", __DATE__, __TIME__);
	fdisplay(log, "Supports mouse wheels");
	fdisplay(log, "Compile time switches enabled:");

#if GENERATE_DIAGNOSTICS
	fdisplay(log, " - Diagnostics");
#endif

#if DISPLAY_LOGO_IN_TITLE
	fdisplay(log, " - Logo in title bar");
#endif

#if POINT_TO_TYPE
	fdisplay(log, " - Point-to-type capability");
#endif

#if ALT_CTRL_APP_OPS
	fdisplay(log, " - CTRL+ALT key-combo's");
#endif

#if VECTOR_VALIDATION
	if (C.mvalidate)
		fdisplay(log, " - Client vector validation");
#endif

	fdisplay(log, " - Realtime (live) window scrolling, moving and sizing");

#if PRESERVE_DIALOG_BGD
	fdisplay(log, " - Preserve dialog backgrounds");
#endif

#if !FILESELECTOR
	fdisplay(log, " - Built without file selector");
#endif

	if (cfg.fsel_cookie)
		fdisplay(log, " - FSEL cookie found");

	if (lcfg.falcon == 0x0030000L)
		fdisplay(log, " - Falcon video handled");

	if (cfg.auto_program)
	{
		if (lcfg.havemode)
			fdisplay(log, " - video mode %d(x%x)", lcfg.modecode, lcfg.modecode);

		fdisplay(log, "auto program");
	}
}

struct kentry *kentry;

/*
 * Startup & Initialisation...
 * - Spawn off any extra programs we need (mouse server, etc).
 * - Open physical & virtual workstations.
 * - Install our trap handler.
 * - Run the xaaes.cnf startup script.
 */
void *
init(struct kentry *k)
{
	char *full;

	/* setup kernel entry */
	kentry = k;

	if (check_kentry_version())
		return NULL;

	/* open the log
	 * if failed nothing is written; fdisplay is failsafe
	 */
	log = kernel_open("xa_setup.log", O_WRONLY|O_CREAT|O_TRUNC, NULL);

	lcfg.booting = true;
	bzero(&default_options, sizeof(default_options));
	bzero(&cfg, sizeof(cfg));
	bzero(&S, sizeof(S));
	bzero(&C, sizeof(C));

#if GENERATE_DIAGS
	bzero(&D, sizeof(D));
	D.debug_level = 4;
	D.debug_file = kernel_open("xaaes.log", O_WRONLY|O_CREAT|O_TRUNC, NULL);
	sprintf(Aes_display_name, sizeof(Aes_display_name), "  XaAES(dbg) %s", version);
#else
	sprintf(Aes_display_name, sizeof(Aes_display_name), "  XaAES %s", version);
#endif

	cfg.font_id = STANDARD_AES_FONTID;		/* Font id to use */
	cfg.standard_font_point = STANDARD_FONT_POINT;	/* Size for normal text */
	cfg.medium_font_point = MEDIUM_FONT_POINT;	/* The same, but for low resolution screens */
	cfg.small_font_point = SMALL_FONT_POINT;	/* Size for small text */
	cfg.ted_filler = '_';
	cfg.menu_locking = true;
	cfg.backname = FAINT;
	cfg.widg_w = ICON_W;
	cfg.widg_h = ICON_H;

#if !FILESELECTOR
	cfg.no_xa_fsel = true;
	DIAGS(("XaAES is compiled without builtin fileselector"));
#endif
	cfg.fsel_cookie = s_system(S_GETCOOKIE, COOKIE_FSEL, 0) != 0xffffffff;

	/* dynamic window handle allocation. */
	clear_wind_handles();

	/* Initialise the object tree display routines */
	init_objects();

	/* So we can speak to and about ourself. */
	C.Aes = xmalloc(sizeof(*C.Aes), 110);
	if (!C.Aes)
	{
		fdisplay(log, "XaAES ERROR: Can't allocate memory?");
		goto error;
	}
	/* zero out */
	bzero(C.Aes, sizeof(*C.Aes));
	S.client_list = C.Aes;

	C.Aes->cmd_tail = "\0";
	C.Aes->wt.edit_obj = -1;
	C.Aes->options = default_options;
	
	strcpy(C.Aes->cmd_name, "XaAES");
	strcpy(C.Aes->name, Aes_display_name);
	strcpy(C.Aes->proc_name,"AESSYS  ");

	/* Where were we started? */
	C.home_drv = d_getdrv();
	DIAGS(("Home drive: %d", C.home_drv));

	d_getpath(C.home, 0);
	DIAGS(("Home directory: '%s'", C.home));

	d_getcwd(C.home, C.home_drv + 1, sizeof(C.home) - 1);
	DIAGS(("current working directory: '%s'", C.home));

	sprintf(Aes_home_path, sizeof(Aes_home_path), "%c:%s", C.home_drv + 'A', C.home);
	strcpy(C.Aes->home_path, Aes_home_path);

	C.Aes->xdrive = C.home_drv;
	strcpy(C.Aes->xpath, C.home);


	/* Are we an auto/mint.cnf launched program? */

	/* clear my_global_aes[0] for old gemlib */
	my_global_aes[0] = 0;
	mt_appl_init(my_global_aes);
	DIAGS(("appl_init -> %i", my_global_aes[0]));

	cfg.auto_program = (my_global_aes[0] == 0);

	lcfg.mint = s_system(S_OSVERSION, 0, 0);
	DIAGS(("s_system(S_OSVERSION) ok (%lx)!", lcfg.mint));

	/* requires mint >= 1.15.11 */
	C.mvalidate = true;

	/* Print a text boot message */
	bootmessage(lcfg.mint);
	DIAGS(("bootmessage ok!"));

	/* Setup the kernel OS call jump table */
	setup_k_function_table();
	DIAGS(("setup_k_function_table ok!"));

#if 0
	/* Create a whole wodge of semphores */
	if (create_semaphores(log))
		return NULL;
#endif

	/* set bit 3 in conterm, so Bconin returns state of ALT and CTRL in upper 8 bit */
	{
		long helper;

		helper = (s_system(S_GETBVAL, 0x0484, 0)) | 8;
		s_system(S_SETBVAL, 0x0484, (char)helper);
	}
	DIAGS(("set bit 3 in conterm ok!"));

#if GENERATE_DIAGS
	{ short nkc_vers = nkc_init(); DIAGS(("nkc_init: version %x", nkc_vers)); }
#else
	nkc_init();
#endif
	DIAGS(("nkc_init ok!"));

	/* Set the default clipboard */
	strcpy(cfg.scrap_path, "c:\\clipbrd\\");

	/* Set the default accessory path */
	strcpy(cfg.acc_path, "c:\\");

	/* Set the default debug file */
	IFDIAG(strcpy(D.debug_path, "xaaes.log");)


	/* Parse the standard startup file.
	 */
	full = xa_find(scls_name);
	if (full)
	{
		SCL(NOLOCKING, 0, scls_name, full, 0);
	}

	C.Aes->options = default_options;

	/* semaphores no longer needed */
#if 0
	/* I just like to have the unlocks at the same level as the locks. */
	/* Unlock the semaphores...we're ready to go */
	Sema_Dn(desk);
	Sema_Dn(lck_update);
	Sema_Dn(mouse);
	Sema_Dn(fsel);
	Sema_Dn(envstr);
	Sema_Dn(pending);
	Sema_Dn(clients);
	Sema_Dn(appl);
	DIAGS(("Semaphores Unlocked!!!"));
#endif

	fdisplay(log, "*** End of successfull setup ***");
	lcfg.booting = false;

	DEBUG(("Creating XaAES kernel thread"));
	{
		long r;

		r = kthread_create(k_main, NULL, &(C.Aes->p), "AESSYS");
		if (r == 0)
			/* this blocks SIGKILL */
			C.Aes->p->p_flag |= P_FLAG_SYS;
		else
			/* XXX todo -> exit gracefully */
			FATAL(("can't create \"XaAES\" kernel thread"));

	}

	return ((DEVDRV *) -1L);

error:
	if (log)
		kernel_close(log);

#if GENERATE_DIAGS
	/* Close the debug output file */
	if (D.debug_file)
		kernel_close(D.debug_file);
#endif

	return NULL;
}

long
module_exit(void)
{
	/* Closedown & exit */
	k_shutdown();

	if (C.shutdown & HALT_SYSTEM)
		s_hutdown(0);  /* halt */
	else if (C.shutdown & REBOOT_SYSTEM)
		s_hutdown(1);  /* warm start */

	return 0;
}
