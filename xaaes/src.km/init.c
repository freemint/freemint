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
#include "draw_obj.h"
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


long loader_pid = 0;
long loader_pgrp = 0;

char version[] = ASCII_VERSION;


static char Aes_display_name[32];

static void
bootmessage(void)
{
	display("%s", Aes_display_name);
	display("MultiTasking AES for MiNT");
	display("");
	display("(c) 1995-1999 Craig Graham, Johan Klockars, Martin Koehling, Thomas Binder");
	display("              and other assorted dodgy characters from around the world...");
	display("(c) 1999-2003 Henk Robbers @ Amsterdam");
	display("    2003-2004 Frank Naumann <fnaumann@freemint.de> and");
	display("              Odd Skancke <ozk@atari.org>");
	display("");
	display("Using Harald Siegmunds NKCC");
	display("");
	display("Date: %s, time: %s", __DATE__, __TIME__);
	display("Supports mouse wheels");
	display("Compile time switches enabled:");

#if GENERATE_DIAGNOSTICS
	display(" - Diagnostics");
#endif

#if DISPLAY_LOGO_IN_TITLE
	display(" - Logo in title bar");
#endif

#if POINT_TO_TYPE
	display(" - Point-to-type capability");
#endif

#if ALT_CTRL_APP_OPS
	display(" - CTRL+ALT key-combo's");
#endif

	if (C.mvalidate)
		display(" - Client vector validation");

	display(" - Realtime (live) window scrolling, moving and sizing");

#if PRESERVE_DIALOG_BGD
	display(" - Preserve dialog backgrounds");
#endif

#if !FILESELECTOR
	display(" - Built without file selector");
#endif

	if (cfg.fsel_cookie)
		display(" - FSEL cookie found");

	if (cfg.auto_program)
		display("auto program");

	display("");
}

struct kentry *kentry;

/*
 * Module initialisation
 * - setup internal data
 * - start main kernel thread
 */
long
init(struct kentry *k, const char *path)
{
	/* setup kernel entry */
	kentry = k;

	if (check_kentry_version())
		return ENOSYS;

	/* remember loader */
	loader_pid = p_getpid();
	loader_pgrp = p_getpgrp();

	/* do some sanity checks of the installation
	 * that are a common source of user problems
	 */
	{
		struct file *check;
		char *buf;
		bool flag;

		/* look if there is a moose.xdd
		 * terminate if yes
		 */

		flag = false;

		check = kernel_open("u:\\dev\\moose", O_RDONLY, NULL);
		if (check)
		{
			kernel_close(check);
			flag = true;
		}

		buf = kmalloc(strlen(sysdir)+16);
		if (buf)
		{
			strcpy(buf, sysdir);
			strcat(buf, "moose.xdd");

			check = kernel_open(buf, O_RDONLY, NULL);
			if (check)
			{
				kernel_close(check);
				flag = true;
			}

			kfree(buf);
		}

		if (flag)
		{
			display("ERROR: There exist an moose.xdd in your FreeMiNT sysdir.");
			display("       Please remove it before starting the XaAES kernel module!");
			return EINVAL;
		}


		/* look is there is an moose.adi
		 * terminate if yes; moose.adi must be in XaAES module directory
		 */

		buf = kmalloc(strlen(sysdir)+16);
		if (buf)
		{
			strcpy(buf, sysdir);
			strcat(buf, "moose.adi");

			check = kernel_open(buf, O_RDONLY, NULL);
			if (check)
			{
				kernel_close(check);
				flag = true;
			}

			kfree(buf);
		}

		if (flag)
		{
			display("ERROR: There exist an moose.adi in your FreeMiNT sysdir.");
			display("       Please remove it and install it in the XaAES module directory");
			display("       before starting the XaAES kernel module!");
			return EINVAL;
		}
	}

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
	cfg.mouse_packet_timegap = MOUSE_PACKET_TIMEGAP;
	cfg.redraw_timeout = 500;
	cfg.standard_font_point = STANDARD_FONT_POINT;	/* Size for normal text */
	cfg.medium_font_point = MEDIUM_FONT_POINT;	/* The same, but for low resolution screens */
	cfg.small_font_point = SMALL_FONT_POINT;	/* Size for small text */
	cfg.ted_filler = '_';
	cfg.menu_locking = true;
	cfg.backname = FAINT;
	cfg.next_active = 0;
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
		display("XaAES ERROR: Can't allocate memory?");
		goto error;
	}

	/* zero out */
	bzero(C.Aes, sizeof(*C.Aes));

	CLIENT_LIST_INIT();
	CLIENT_LIST_INSERT_START(C.Aes);

	APP_LIST_INIT();
	APP_LIST_INSERT_START(C.Aes);

	C.Aes->cmd_tail = "\0";
	//C.Aes->wt.e.obj = -1;

	strcpy(C.Aes->cmd_name, "XaAES");
	strcpy(C.Aes->name, Aes_display_name);
	strcpy(C.Aes->proc_name,"AESSYS  ");

	/* Where were we started? */
	strcpy(C.Aes->home_path, path);

	/* strip off last element */
	{
		char *s = C.Aes->home_path, *name = NULL;
		char c;

		do {
			c = *s++;
			if (c == '\\' || c == '/')
				name = s;
		}
		while (c);

		if (name)
			*name = '\0';
	}
	DIAGS(("module path: '%s'", C.Aes->home_path));

	C.Aes->xdrive = d_getdrv();
	d_getpath(C.Aes->xpath, 0);

	/* check if there exist a moose.adi */
	{
		struct file *check;
		char *buf;
		bool flag;

		flag = false;

		buf = kmalloc(strlen(C.Aes->home_path)+16);
		if (buf)
		{
			strcpy(buf, C.Aes->home_path);
			strcat(buf, "moose.adi");

			check = kernel_open(buf, O_RDONLY, NULL);
			if (check)
			{
				kernel_close(check);
				flag = true;
			}

			kfree(buf);
		}

		if (!flag)
		{
			display("ERROR: There exist no moose.adi in your XaAES module directory.");
			display("       Please install it before starting the XaAES kernel module!");
			return EINVAL;
		}
	}


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
	default_options.xa_nomove = true;
	default_options.xa_nohide = true;
	default_options.thinframe = 1;

	C.Aes->options = default_options;

	/* Parse the config file */
	load_config();

	//C.Aes->options = default_options;
	//C.Aes->options.xa_nomove = false;

	C.aesmouse = -1;

	DIAGS(("load adi modules"));
	adi_load();

	DIAGS(("Creating XaAES kernel thread"));
	{
		long r;

		r = kthread_create(NULL, k_main, NULL, &(C.Aes->p), "AESSYS");
		if (r)
			/* XXX todo -> exit gracefully */
			FATAL("can't create XaAES kernel thread");

	}

	{
		struct proc *p = get_curproc();
		ulong oldmask;

		oldmask = p->p_sigmask;
		p->p_sigmask = 0xffffffffUL;

		while (!(C.shutdown & QUIT_NOW))
			sleep(WAIT_Q, (long)&loader_pid);

		p->p_sigmask = oldmask;
	}

	/* succeeded */
	return 0;

error:
#if GENERATE_DIAGS
	/* Close the debug output file */
	if (D.debug_file)
		kernel_close(D.debug_file);
#endif

	return ENOMEM;
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
