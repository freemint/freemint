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

#include "util.h"

#include "init.h"
#include "xa_global.h"
#include "adiload.h"

#include "xaaes.h"

#include "c_window.h"
#include "cnf_xaaes.h"
#include "handler.h"
#include "k_init.h"
#include "k_main.h"
#include "k_shutdown.h"
#include "nkcc.h"
#include "draw_obj.h"
#include "semaphores.h"
#include "xa_shel.h"
#include "xa_appl.h"
#include "taskman.h"
#include "scrlobjc.h"
#include "win_draw.h"
#include "xcclib.h"
#include "version.h"

#include "mint/ssystem.h"
#include "cookie.h"
#include "info.h"

#if CHECK_STACK
short check_stack_alignment( long e );
#endif

long loader_pid = 0;
long loader_pgrp = 0;

#if GENERATE_DIAGS
static char const Aes_display_name[] = "  XaAES(dbg) v" AES_ASCII_VERSION_VERBOSE " (" ASCII_ARCH_TARGET ")";
#else
static char const Aes_display_name[] = "  XaAES v" AES_ASCII_VERSION_VERBOSE;
#endif

static void
bootmessage(void)
{
	BLOG((true, "%s (%s, %s) (MultiTasking AES for MiNT)", Aes_display_name+2, arch_target, build_date() ));
#if DISPCREDITS
	BLOG((true, ""));
	BLOG((true, "(c) 1995-1999 Craig Graham, Johan Klockars, Martin Koehling, Thomas Binder"));
	BLOG((true, "              and other assorted dodgy characters from around the world..."));
	BLOG((true, "(c) 1999-2003 Henk Robbers @ Amsterdam"));
	BLOG((true, "    2003-2004 Frank Naumann <fnaumann@freemint.de> and"));
	BLOG((true, "              Odd Skancke <ozk@atari.org>"));
	BLOG((true, ""));
	BLOG((true, "Using Harald Siegmunds NKCC"));
	BLOG((true, ""));
#else
	BLOG((true,"Part of freemint ("FREEMINT_URL")." ));
#endif

#if 0
	BLOG((true, "Supports mouse wheels"));
	BLOG((true, "Compile time switches enabled:"));

#if GENERATE_DIAGS
	BLOG((true, " - Diagnostics"));
#endif

#if DISPLAY_LOGO_IN_TITLE
	BLOG((true, " - Logo in title bar"));
#endif

#if POINT_TO_TYPE
	BLOG((true, " - Point-to-type capability"));
#endif

#if ALT_CTRL_APP_OPS
	BLOG((true, " - CTRL+ALT key-combo's"));
#endif

	if (C.mvalidate)
		BLOG((true, " - Client vector validation"));

	BLOG((true, " - Realtime (live) window scrolling, moving and sizing"));


#if PRESERVE_DIALOG_BGD
	BLOG((true, " - Preserve dialog backgrounds"));
#endif

#if !FILESELECTOR
	BLOG((true, " - Built without file selector"));
#endif

	if (cfg.fsel_cookie)
		display(" - FSEL cookie found");

	BLOG((true, ""));

#endif

}

struct kentry *kentry;

/* similar to xaaes_sysfile in k_init.c */

static bool
sysfile_exists(const char *sd, char *fn)
{
	struct file *check;
	bool flag = false;
	int o = 0;
	char *buf, *tmp;

	buf = kmalloc(strlen(sd)+16);
	if (buf)
	{
		if (sd[0] == '/' || sd[0] == '\\') {
			buf[0] = 'u';
			buf[1] = ':';
			o += 2;
		}
		strcpy(buf + o, sd);
		tmp = buf + strlen(buf) - 1;
		if (*tmp != '/' && *tmp != '\\') {
			tmp[1] = '\\';
			tmp[2] = '\0';
		}
		strcat(buf + o, fn);
		BLOG((0,"sysfile_exists: '%s'", buf));
		check = kernel_open(buf, O_RDONLY, NULL, NULL);
		if (check)
		{
			kernel_close(check);
			flag = true;
		}
		kfree(buf);
	}
	return flag;
}

/*
 *  0 = USA          8 = Ger.Suisse    16 = Hungary    24 = Romania
 *  1 = Germany      9 = Turkey        17 = Poland     25 = Bulgaria
 *  2 = France      10 = Finnland      18 = Lituania   26 = Slovenia
 *  3 = England     11 = Norway        19 = Russia     27 = Croatia
 *  4 = Spain       12 = Danmark       20 = Estonia    28 = Serbia
 *  5 = Italy       13 = S. Arabia     21 = Bialorus   29 = Montenegro
 *  6 = Sweden      14 = Netherlands   22 = Ukraina    30 = Macedonia
 *  7 = Fr.Suisse   15 = Czech         23 = Slovakia   31 = Greece
 *
 * 32 = Latvia      40 = Vietnam       48 = Bangladesh
 * 33 = Israel      41 = India
 * 34 = Sou. Africa 42 = Iran
 * 35 = Portugal    43 = Mongolia
 * 36 = Belgium     44 = Nepal
 * 37 = Japan       45 = Laos
 * 38 = China       46 = Kambodja
 * 39 = Korea       47 = Indonesia
 */
#define MaX_COUNTRYCODE 48
static char const countrycodes[] =
  "endefrukesitsefsgstrfinodksanlczhuplltrueebyuaskrobgslhrcscsmkgrlvilzaptbejpcnkpvninirmnnplakhidbd";
/* 0 1 2 3 4 5 6 7 8 9 1 1 2 3 4 5 6 7 8 9 2 1 2 3 4 5 6 7 8 9 3 1 2 3 4 5 6 7 8 9 4 1 2 3 4 5 6 7 8 */
static short search_code( char *lang )
{
	short i;
	for( i = 0; i < sizeof( countrycodes ); i += 2 )
		if( lang[0] == countrycodes[i] && lang[1] == countrycodes[i+1] )
			return i >> 1;
	return -1;
}

#if CHECK_STACK
/*
 * return alignment-value
 *
 * do not inline!
 */
short check_stack_alignment( long stk )
{
	/* Read the current stack pointer value */

	if( stk & 1L )
		return 1;
	if( stk & 2L )
		return 2;
	return 4;
}

unsigned short stack_align = 0;

#endif
/*
 * md = 0: get lang
 * md = 1: get keyboard-layout-lang
 */
short lang_from_akp( char lang[], int md )
{
	long	li;
	short ret = -1;
	if( md )
		li = gl_kbd;
	else
		li = gl_lang;

	ret = li;
	if( li < MaX_COUNTRYCODE )
	{
		li *= 2;
		lang[0] = countrycodes[li];
		lang[1] = countrycodes[li+1];
	}
	lang[2] = 0;
	return ret;
}

/*
 * Module initialisation
 * - setup internal data
 * - start main kernel thread
 */
static Path start_path;
static const struct kernel_module *self = NULL;

static void configure(void)
{
	C.Aes->type = APP_AESSYS;
	C.Aes->cmd_tail = "\0";

	strcpy(C.Aes->cmd_name, "XaAES");
	strcpy(C.Aes->name, Aes_display_name);
	strcpy(C.Aes->proc_name,"AESSYS  ");

	/* Where were we started? */
	strcpy(C.Aes->home_path, self->path);

	C.Aes->xdrive = d_getdrv();
	d_getpath(C.Aes->xpath, 0);
	/* home_path is no full path - make one */
	if( *(C.Aes->home_path + 2) != ':' )
		sprintf( C.Aes->home_path, sizeof(C.Aes->home_path), "%c:%s\\", letter_from_drive(C.Aes->xdrive), C.Aes->xpath);
	BLOG((true, "home_path: '%s'", C.Aes->home_path));

	/*
	 * default configuration
	 */

	strcpy(cfg.scrap_path, "c:\\clipbrd\\");
	strcpy(cfg.acc_path, "c:\\");
	strcpy(cfg.widg_name, WIDGNAME);
	/*
	 * XXX - REMOVE ME! It is the responsibility of the object renderer to provide
	 *	the extended AES objects.
	 */
	strcpy(cfg.xobj_name, "xa_xtobj.rsc");

	strcpy(cfg.rsc_name, "xaaes.rsc");

	cfg.font_id = STANDARD_AES_FONTID;		/* Font id to use */
	cfg.xaw_point = STANDARD_FONT_POINT;	/* fnt-sz for xaaes-windows */
	cfg.double_click_time = DOUBLE_CLICK_TIME;
	cfg.mouse_packet_timegap = MOUSE_PACKET_TIMEGAP;
	cfg.redraw_timeout = 500;
	cfg.standard_font_point = STANDARD_FONT_POINT;	/* Size for normal text */
	cfg.info_font_point = -1;
	cfg.medium_font_point = MEDIUM_FONT_POINT;	/* The same, but for low resolution screens */
	cfg.small_font_point = SMALL_FONT_POINT;	/* Size for small text */
	cfg.ted_filler = '_';
	cfg.menu_locking = true;
	cfg.back_col = -1;
	cfg.menu_bar = 2;	// always on
	cfg.allow_setexc = -1;	/* dont change */
	cfg.next_active = 0;

	cfg.ver_wheel_id = 0;
	cfg.ver_wheel_amount = 1;
	cfg.hor_wheel_id = 1;
	cfg.hor_wheel_amount = 1;

	cfg.icnfy_orient = 3;
	cfg.icnfy_w = ICONIFIED_W;
	cfg.icnfy_h = ICONIFIED_H;

	cfg.popup_timeout = 10;
	cfg.popout_timeout = 1000;

	cfg.alert_winds = 0xffff;

	/* default to live actions */
	default_options.xa_nomove = true;
	default_options.xa_nohide = true;
	default_options.thinframe = 1;
	default_options.wheel_mode = DEF_WHLMODE;

	default_options.clwtna = 0;
	default_options.alt_shortcuts = 3;
	default_options.submenu_indicator = '>';

	C.Aes->options = default_options;
	/* Parse the config file */
	load_config(0);
}


long _cdecl init_km(struct kentry *k, const struct kernel_module *km) //const char *path)
{
#if CHECK_STACK
	long stk = (long)get_sp();
#endif
	long err = 0L;
	bool first = true;
	struct proc *rootproc;

	/* setup kernel entry */
	kentry = k;
	xmemcpy = memcpy;
	xmemset = memset;

	self = km;
	next_res = 0L;

	if ( !k || !km || check_kentry_version())
	{
		err = ENOSYS;
		goto error;
	}

	C.bootlog_path[0] = '\0';
	get_drive_and_path(start_path, sizeof(start_path));

	setup_xa_module_api();
again:
	/* zero anything out */
	bzero(my_global_aes, sizeof(my_global_aes));
	bzero(&G, sizeof(G));
	bzero(&default_options, sizeof(default_options));
	bzero(&cfg, sizeof(cfg));
	bzero(&S, sizeof(S));
	bzero(&C, sizeof(C));
#if XAAES_RELEASE
	C.loglvl = 0;
#else
	C.loglvl = 1;
#endif
#if CHECK_STACK
	stack_align = 0;
#endif

	root_window = menu_window = 0;
	C.SingleTaskPid = -1;	/* just for sure */

	strcpy(C.start_path, start_path);
#ifdef BOOTLOG
	if (!C.bootlog_path[0])
	{
		strcpy(C.bootlog_path, C.start_path);
		strcat(C.bootlog_path, "xa_boot.log");
	}

	if (first)
		BLOG(( false, "\n~~~~~~~~~~~~ XaAES start up ~~~~~~~~~~~~~~~~" ));
	else
		BLOG((false, "\n~~~~~~~~~~~~ XaAES restarting ~~~~~~~~~~~~~~~"));
#endif

	/* reset single-flags in case of previous fault */
	rootproc = pid2proc(0);
	rootproc->modeflags &= ~(M_SINGLE_TASK|M_DONT_STOP);

	/* remember loader */

	loader_pid = p_getpid();
	//get_curproc()->pgrp = 0;
	loader_pgrp = p_getpgrp();

#if GENERATE_DIAGS
	bzero(&D, sizeof(D));
	D.debug_level = 4;
#if 0 /*LOGDEBUG*/
	/* Set the default debug file */
	strcpy(D.debug_path, "xaaes.log");
	D.debug_file = kernel_open(D.debug_path, O_WRONLY|O_CREAT|O_TRUNC, NULL, NULL);
#else
	D.debug_file = NULL;
#endif
#endif

	/* Print a text boot message */
	if (first)
		bootmessage();

#if CHECK_STACK
	/**** check if stack is sane ****/
	stack_align |= check_stack_alignment(stk);
	if( stack_align == 1 )
	{
		BLOG(( 0,"WARNING: stack is odd:%lx!", stk ));
	}
	else if( stack_align == 4 )
			BLOG(( 0,"stack is long-aligned:%lx", stk ));
	else
			BLOG(( 0,"stack is word-aligned:%lx", stk ));
#endif
	BLOG((0,"PRG: km=%lx, base=%lx, text=%lx -> %lx(%ld), kentry:%d.%d,dos_version=%lx",
		(unsigned long)km,
		(unsigned long)km->b,
		km->b->p_tbase,
		km->b->p_tbase + km->b->p_tlen,
		km->b->p_tlen,
		KENTRY_MAJ_VERSION, KENTRY_MIN_VERSION,
		kentry->dos_version));

#if 0
	/* do some sanity checks of the installation
	 * that are a common source of user problems
	 */
	if (first)
	{
		struct file *check;
		bool flag;

		/* look if there is a moose.xdd
		 * terminate if yes
		 */

		check = kernel_open("u:\\dev\\moose", O_RDONLY, NULL, NULL);
		if (check)
		{
			kernel_close(check);
			flag = true;
		}
		else
			flag = sysfile_exists(sysdir, "moose.xdd");

		if (flag)
		{
			BLOG((true,
"ERROR: There exists a moose.xdd in your FreeMiNT sysdir.\n"
"       Please remove it before starting the XaAES kernel module!"));
			err = EINVAL;
			goto error;
		}

		/* look is there is a moose.adi
		 * terminate if yes; moose.adi must be in XaAES module directory
		 */
		flag = sysfile_exists(sysdir, "moose_w.adi");
		if (!flag)
			flag = sysfile_exists(sysdir, "moose.adi");

		if (flag)
		{
			BLOG((true,
"ERROR: There exists a moose.adi in your FreeMiNT sysdir.\n"
" sysdir = '%s'\n"
"       Please remove it and install it in the XaAES module directory\n"
"       before starting the XaAES kernel module!\n",
			sysdir));
			err = EINVAL;
			goto error;
		}
	}
#endif

	init_env();
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
#if !FILESELECTOR
#error external fileselectors not supported yet!
	cfg.no_xa_fsel = true;
	BLOG((false, "XaAES is compiled without builtin fileselector"));
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
		BLOG((/*00000007*/true, "XaAES ERROR: Can't allocate memory?"));
		err = ENOMEM;
		goto error;
	}

	/* zero out */
	bzero(C.Aes, sizeof(*C.Aes));

	CLIENT_LIST_INIT();
	CLIENT_LIST_INSERT_START(C.Aes);

	APP_LIST_INIT();
	APP_LIST_INSERT_START(C.Aes);

	TAB_LIST_INIT();

	configure();

	/* check if there exist a moose.adi */
	if (first)
	{
		bool flag;

		flag = sysfile_exists(C.Aes->home_path, "moose_w.adi");
		if (!flag)
			BLOG((true, "moose_w.adi not found" ));
		flag |= sysfile_exists(C.Aes->home_path, "moose.adi") << 1;

		if (!flag)
		{
			BLOG((true,
"ERROR: no moose.adi in your XaAES module directory.\n"
"	  -> '%s'"
"   Please install it before starting the XaAES kernel module!",
				C.Aes->home_path));
			err = EINVAL;
			goto error;
		}
		if( flag == 3 )
			BLOG((true, "both moose_w.adi and moose.adi found (better choose one)" ));
	}


	/* Setup the kernel OS call jump table */

	setup_handler_table();

	/* set bit 3 in conterm, so Bconin returns state of ALT and CTRL in upper 8 bit */
	{
		long helper;

		helper = (s_system(S_GETBVAL, 0x0484, 0)) | 8;
		s_system(S_SETBVAL, 0x0484, (char)helper);
	}

#if GENERATE_DIAGS
	{ short nkc_vers = nkc_init(); DIAGS(("nkc_init: version %x", nkc_vers)); }
#else
	nkc_init();
#endif

	{
	short li = 0, p = -1;
	UNUSED(li);
	if( !cfg.lang[0] )
	{
		const char *lang = get_env(0, "LANG=");
		if( lang )
		{
			li = 1;
			strncpy( cfg.lang, lang, 2 );
		}
		else
		{
			li = 2;
			p = lang_from_akp( cfg.lang, 0 );
		}
	}
	if( p == -1 && cfg.lang[0] )
	{
		p = search_code( cfg.lang );
	}
	if( p >= 0 )
	{
		info_tab[3][0] = p;
	}
	BLOG((0,"lang='%s' (from %s) code=%d.",cfg.lang, li == 0 ? "config" : li == 1 ? "Environ" : "AKP", info_tab[3][0] ));
	}

	/* XaAES-windows */
	norm_txt.normal.font_id = cfg.font_id;
	norm_txt.selected.font_id = cfg.font_id;
	norm_txt.highlighted.font_id = cfg.font_id;

	acc_txt.normal.font_id = cfg.font_id;
	acc_txt.selected.font_id = cfg.font_id;
	acc_txt.highlighted.font_id = cfg.font_id;

	prg_txt.normal.font_id = cfg.font_id;
	prg_txt.selected.font_id = cfg.font_id;
	prg_txt.highlighted.font_id = cfg.font_id;

	sys_txt.normal.font_id = cfg.font_id;
	sys_txt.selected.font_id = cfg.font_id;
	sys_txt.highlighted.font_id = cfg.font_id;

	sys_thrd.normal.font_id = cfg.font_id;
	sys_thrd.selected.font_id = cfg.font_id;
	sys_thrd.highlighted.font_id = cfg.font_id;

	desk_txt.normal.font_id = cfg.font_id;
	desk_txt.selected.font_id = cfg.font_id;
	desk_txt.highlighted.font_id = cfg.font_id;

	/* scrlobjc */
	default_fnt.normal.font_id = cfg.font_id;
	default_fnt.selected.font_id = cfg.font_id;
	default_fnt.highlighted.font_id = cfg.font_id;

	C.aesmouse = -1;

	BLOG((false, "load adi modules"));

	if (!G.adi_mouse)
		adi_load(first);

	BLOG((false, "Creating XaAES kernel thread"));
	{
		long r;

		r = kthread_create(NULL, k_main, NULL, &(C.Aes->p), "AESSYS");
		if (r)
			/* XXX todo -> exit gracefully */
			FATAL(/*0000000a*/"can't create XaAES kernel thread");
	}
	{
		struct proc *p = get_curproc();
		ulong oldmask;
		char *sd_str;

		oldmask = p->p_sigmask;
		p->p_sigmask = 0xffffffffUL;

		while (!(C.shutdown & QUIT_NOW))
		{
			sleep(WAIT_Q, (long)&loader_pid);
		}
		switch( C.shutdown & 0x0178)
		{
		case HALT_SYSTEM:
			sd_str = "Halt";
		break;
		case REBOOT_SYSTEM:
			sd_str = "Warmboot";
		break;
		case COLDSTART_SYSTEM:
			sd_str = "Coldboot";
		break;
		case RESTART_XAAES:
			sd_str = "Restart XaAES";
		break;
		default:
			sd_str = "Quit XaAES";
		}
		UNUSED(sd_str);
		BLOG((1,"AESSYS kthread exited - shutdown = %x(%s).", C.shutdown, sd_str));
		if( C.shutdown & (RESTART_XAAES | RESOLUTION_CHANGE) )
		{
			goto again;
		}


// 		display("AESSYS kthread exited - C.shutdown = %x", C.shutdown);

#if GENERATE_DIAGS
		/* Close the debug output file */
		if (D.debug_file)
		{
			kernel_close(D.debug_file);
			D.debug_file = NULL;
		}
#endif
		p->p_sigmask = oldmask;
		if (C.shutdown & HALT_SYSTEM)
			s_hutdown(SHUT_POWER);  /* poweroff or halt if poweroff is not supported */
		else if (C.shutdown & REBOOT_SYSTEM)
			s_hutdown(SHUT_BOOT);  /* warm start */
		else if (C.shutdown & COLDSTART_SYSTEM)
			s_hutdown(SHUT_COLD);
	}

	first = false;
	if ((C.shutdown & (RESOLUTION_CHANGE|HALT_SYSTEM|REBOOT_SYSTEM|COLDSTART_SYSTEM)) == RESOLUTION_CHANGE)
		goto again;

	/* Ozk:
	 * Make sure no processes have any XaAES module attachments
	 * before we exit. If access to 'our' attachment vectors happen
	 * after XaAES module quits, havoc is next on the agenda
	 */
	detach_extension((void *)-1L, XAAES_MAGIC);
	detach_extension((void *)-1L, XAAES_MAGIC_SH);

error:
	/* succeeded */
	//return 0;

#if GENERATE_DIAGS
	/* Close the debug output file */
	if (D.debug_file)
		kernel_close(D.debug_file);
#endif
	if( err == 0 )
		err = ferr;
	BLOG((0,"init:return %ld", err));
	return err;
}

#if INCLUDE_UNUSED
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
#endif
