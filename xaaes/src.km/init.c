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
#include WIDGHNAME

#include "init.h"
#include "xa_global.h"
#include "adiload.h"

#include "c_window.h"
#include "cnf_xaaes.h"
#include "handler.h"
#include "k_main.h"
#include "k_shutdown.h"
#include "nkcc.h"
#include "objects.h"
#include "semaphores.h"
#include "xa_shel.h"

#include "version.h"

#include "mint/ssystem.h"
#include "cookie.h"


short my_global_aes[16];

/* XXX gemlib */
//short	mt_appl_init    (short *global_aes);
//short	mt_appl_exit    (short *global_aes);
//short	mt_graf_handle	(short *Wchar, short *Hchar, short *Wbox, short *Hbox, short *global_aes);


#if GENERATE_DIAGS
static const char *cnf_name = "xaaesdbg.cnf";
#else
static const char *cnf_name = "xaaes.cnf";
#endif

static char Aes_display_name[32];
Path Aes_home_path;

long loader_pid = -1;
struct file *log = NULL;

char version[] = ASCII_VERSION;

static void
bootmessage(void)
{
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

	if (C.mvalidate)
		fdisplay(log, " - Client vector validation");

	fdisplay(log, " - Realtime (live) window scrolling, moving and sizing");

#if PRESERVE_DIALOG_BGD
	fdisplay(log, " - Preserve dialog backgrounds");
#endif

#if !FILESELECTOR
	fdisplay(log, " - Built without file selector");
#endif

	if (cfg.fsel_cookie)
		fdisplay(log, " - FSEL cookie found");

	if (cfg.auto_program)
		fdisplay(log, "auto program");
}

struct kentry *kentry;

/*
 * Module initialisation
 * - setup internal data
 * - start main kernel thread
 */
void *
init(struct kentry *k)
{
	/* setup kernel entry */
	kentry = k;

	if (check_kentry_version())
		return NULL;

	loader_pid = p_getpid();

	/* open the log
	 * if failed nothing is written; fdisplay is failsafe
	 */
	log = kernel_open("xa_setup.log", O_WRONLY|O_CREAT|O_TRUNC, NULL);

	/* zero anything out */
	bzero(&default_options, sizeof(default_options));
	bzero(&cfg, sizeof(cfg));
	bzero(&S, sizeof(S));
	bzero(&C, sizeof(C));

#if GENERATE_DIAGS
	bzero(&D, sizeof(D));
	D.debug_level = 4;
	/* Set the default debug file */
	strcpy(D.debug_path, "xaaes.log");
	D.debug_file = kernel_open(D.debug_path, O_WRONLY|O_CREAT|O_TRUNC, NULL);
	sprintf(Aes_display_name, sizeof(Aes_display_name), "  XaAES(dbg) %s", version);
#else
	sprintf(Aes_display_name, sizeof(Aes_display_name), "  XaAES %s", version);
#endif

	/*
	 * default configuration
	 */

	strcpy(cfg.scrap_path, "c:\\clipbrd\\");
	strcpy(cfg.acc_path, "c:\\");
	strcpy(cfg.widg_name, WIDGNAME);
	strcpy(cfg.rsc_name, RSCNAME);

	cfg.font_id = STANDARD_AES_FONTID;		/* Font id to use */
	cfg.double_click_time = DOUBLE_CLICK_TIME;
	cfg.standard_font_point = STANDARD_FONT_POINT;	/* Size for normal text */
	cfg.medium_font_point = MEDIUM_FONT_POINT;	/* The same, but for low resolution screens */
	cfg.small_font_point = SMALL_FONT_POINT;	/* Size for small text */
	cfg.ted_filler = '_';
	cfg.menu_locking = true;
	cfg.backname = FAINT;
	cfg.widg_w = ICON_W;
	cfg.widg_h = ICON_H;

#if !FILESELECTOR
#error external fileselectors not supported yet!
	cfg.no_xa_fsel = true;
	DIAGS(("XaAES is compiled without builtin fileselector"));
#endif
	cfg.fsel_cookie = s_system(S_GETCOOKIE, COOKIE_FSEL, 0) != 0xffffffff;
	if (cfg.fsel_cookie)
	{
		/* we need to kill the cookie for now
		 * as we don't support external fileslectors yet
		 */
		s_system(S_DELCOOKIE, COOKIE_FSEL, 0);
		s_system(S_DELCOOKIE, COOKIE_HBFS, 0);
	}

	/* dynamic window handle allocation. */
	clear_wind_handles();

	/* Initialise the object tree display routines */
	init_objects();

	/* So we can speak to and about ourself. */
	C.Aes = kmalloc(sizeof(*C.Aes));
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

	/* requires mint >= 1.15.11 */
	C.mvalidate = true;

	/* Print a text boot message */
	bootmessage();
	DIAGS(("bootmessage ok!"));

	/* Setup the kernel OS call jump table */
	setup_handler_table();
	DIAGS(("setup_handler_table ok!"));

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

	/* copy over environment from loader */
	{
		struct proc *p = get_curproc();
		const char *env_str = p->p_mem->base->p_env;

		if (env_str)
		{
			while (*env_str)
			{
				put_env(NOLOCKING, env_str);

				while (*env_str)
					env_str++;

				env_str++;
			}
		}
	}

	/* default to live actions */
	default_options.live = true;

	/* Parse the config file */
	load_config(cnf_name);

	C.Aes->options = default_options;

	DIAGS(("call load_adi"));
	adi_load();

	fdisplay(log, "*** End of successfull setup ***");

	DEBUG(("Creating XaAES kernel thread"));
	{
		long r;

		r = kthread_create(NULL, k_main, NULL, &(C.Aes->p), "AESSYS");
		if (r)
			/* XXX todo -> exit gracefully */
			FATAL(("can't create XaAES kernel thread"));

	}

	return ((void *) -1L);

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
