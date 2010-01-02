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
#include "util.h"

#include "init.h"
#include "xa_global.h"
#include "adiload.h"

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
#include "win_draw.h"
#include "version.h"

#include "mint/ssystem.h"
#include "cookie.h"

//long module_exit(void);


short my_global_aes[16];

/* XXX gemlib */
//short	mt_appl_init    (short *global_aes);
//short	mt_appl_exit    (short *global_aes);
//short	mt_graf_handle	(short *Wchar, short *Hchar, short *Wbox, short *Hbox, short *global_aes);


long loader_pid = 0;
long loader_pgrp = 0;

//char version[] = ASCII_VERSION;

static char Aes_display_name[32];

#if 0
static int
imp_msg(void)
{
	long ci;
	
	display(" ---=== IMPORTNAT!! ===---");
	display("");
	display("If you have read the CHANGES.txt, you");
	display("can proceed boot by pressing RETURN.");
	display("If not, please press any other key to");
	display("quit, becaues important changes needs");
	display("your attention!");
	display("");
	display(" This warning will be removed after");
	display(" some time!");
	ci = _c_conin();

	return ((ci & 0xff) == 13) ? 1 : 0;
}
#endif

static void
bootmessage(void)
{
	BLOG((true, "%s", Aes_display_name));
	BLOG((true, "MultiTasking AES for MiNT"));
	BLOG((true, ""));
	BLOG((true, "(c) 1995-1999 Craig Graham, Johan Klockars, Martin Koehling, Thomas Binder"));
	BLOG((true, "              and other assorted dodgy characters from around the world..."));
	BLOG((true, "(c) 1999-2003 Henk Robbers @ Amsterdam"));
	BLOG((true, "    2003-2004 Frank Naumann <fnaumann@freemint.de> and"));
	BLOG((true, "              Odd Skancke <ozk@atari.org>"));
	BLOG((true, ""));
	BLOG((true, "Using Harald Siegmunds NKCC"));
	BLOG((true, ""));
	BLOG((true, "Date: %s, time: %s", __DATE__, __TIME__));
	BLOG((true, "Supports mouse wheels"));
	BLOG((true, "Compile time switches enabled:"));

#if GENERATE_DIAGNOSTICS
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
}

struct kentry *kentry;

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
		display("sysfile_exits: '%s'", buf);
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

static void
make_module_path ( char *result )
{
	char c;
	int plen = strlen ( C.Aes->home_path );
	strcpy ( result, C.Aes->home_path );
	BLOG((false, "Create module path by combining (%d) '%s(%s)' and '%s'", plen, C.Aes->home_path, result, "xam"));
	c = result[plen - 1];
	if ( ! ( c == '/' || c == '\\' ) )
	{
		result[plen++] = '\\';
		result[plen] = '\0';
	}
	strcat ( result, "xam" );
	BLOG((false, "Created module path '%s'", C.Aes->module_path));
}

/*
 *  0 = USA          8 = Ger.Suisse    16 = Hungary    24 = Romania
 *  1 = Germany      9 = Turkey        17 = Poland     25 = Bulgaria
 *  2 = France      10 = Finnland      18 = Lituania   26 = Slovenia
 *  3 = England     11 = Norway        19 = Latvia     27 = Croatia
 *  4 = Spain       12 = Danmark       20 = Estonia    28 = Serbia
 *  5 = Italy       13 = S. Arabia     21 = Bialorus   29 = Montenegro
 *  6 = Sweden      14 = Netherlands   22 = Ukraina    30 = Macedonia
 *  7 = Fr.Suisse   15 = Czech         23 = Slovakia   31 = Greece
 *
 * 32 = Russia      40 = Vietnam       48 = Bangladesh
 * 33 = Israel      41 = India
 * 34 = Sou. Africa 42 = Iran
 * 35 = Portugal    43 = Mongolia
 * 36 = Belgium     44 = Nepal
 * 37 = Japan       45 = Laos
 * 38 = China       46 = Kambodja
 * 39 = Korea       47 = Indonesia
 */
/*
	"us","de","fr","en","es","it","se","fs","gs","tr","fi",
	"no","dk","sa","nl","cz","hu","pl","lt","lv","ee",
	"by","ua","sk","ro","bg","sl","hr","cs","cs","mk",
	"gr","ru","il","za","pt","be","jp","cn","kp","vn",
	"in","ir","mn","np","la","kh","id","bd",
*/
#if 0
#define MaX_COUNTRYCODE 48
static char countrycodes[] = "usdefrenesitsefsgstrfinodksanlczhuplltlveebyuaskrobgslhrcscsmkgrruilzaptbejpcnkpvninirmnnplakhidbd";
struct aes_string {
	int lang;
	union { char c[2]; short w; } ccode;
	int nlen;
	int slen;
	char data[0];
};

static char *
eol(char *b)
{
	char c;

	while ((c = *b++) && c != '\n' || c != '\r')
		;
	if (!c)
		b--;
	else if (c == '\r' && *b == '\n')
		b++;
	return b;
}

static int
cpytochr(char tochr, char *s, char *d)
{
	int chrs = 0;
	char c, *t;

	while (*s) {
		t = to;
		c = *s++;
		while (*t) {
			if (c == *t)
				goto end;
			t++;
		}
		*d++ = c;
		chrs++;
	}
end:
	*d++ = '\0';
	return chrs;
}

static bool
load_text(int cc)
{
	char *fn;
	union { char chr[2]; short code; } c1, c2;

	if (cc > MAX_COUNTRYCODE)
		return false;
	cc <<= 1;
	c1.chr[0] = countrycodes[cc++];
	c1.chr[1] = countrycodes[cc];

	fn = xaaes_sysfile("strings.aes");
	if (fn) {
		bool success = true;
		char *buf = NULL, *s, *d, c;
		struct file *fp;
		long err;
		XATTR x;

		fp = kernel_open(fn, O_RDONLY, &r, &x);
		if (fp) buf = kmalloc(x.size);
		if (fp && buf) {
			r = kernel_read(f, buf, x.size);
			if (r != x.size)
				goto fail;
			kernel_close(fp);
			fp = NULL;

			{
				char nstart[] = "[", nend[] = "]";

			}
		}
fail:		if (buf) kfree(buf);
		if (fp) kernel_close(fp);
		kfree(fn);
		return false;
	}
	return false;
}
#endif
/*
 * Module initialisation
 * - setup internal data
 * - start main kernel thread
 */
static Path start_path;

struct kernel_module *self = NULL;

long
init(struct kentry *k, struct kernel_module *km)
{
	long err = 0L;

	bool first = true;
	
	/* setup kernel entry */
	kentry = k;
	self = km;

	next_res = 0L;

	bzero(&G, sizeof(G));
	C.bootlog_path[0] = '\0';
	get_drive_and_path(start_path, sizeof(start_path));

	setup_xa_api();
again:
	/* zero anything out */
	bzero(&default_options, sizeof(default_options));
	bzero(&cfg, sizeof(cfg));
	bzero(&S, sizeof(S));
	bzero(&C, sizeof(C));
	
	strcpy(C.start_path, start_path);
#if BOOTLOG
	if (!C.bootlog_path[0])
	{
		strcpy(C.bootlog_path, C.start_path);
		strcat(C.bootlog_path, "xa_boot.log");
		display("bootlog '%s'", C.bootlog_path);
	}

	if (first)
		BLOG(( false, "\n~~~~~~~~~~~~ XaAES start up!! ~~~~~~~~~~~~~~~~" ));
	else
		BLOG((false, "\n~~~~~~~~~~~~ XaAES restarting! ~~~~~~~~~~~~~~~"));
#endif	
	if (check_kentry_version())
	{
		err = ENOSYS;
		goto error;
	}

	/* remember loader */
	
	loader_pid = p_getpid();
	loader_pgrp = p_getpgrp();

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
			BLOG((/*00000005*/true, "ERROR: There exist an moose.xdd in your FreeMiNT sysdir."));
			BLOG((/*00000006*/true, "       Please remove it before starting the XaAES kernel module!"));
			err = EINVAL;
			goto error;
		}
		
		/* look is there is an moose.adi
		 * terminate if yes; moose.adi must be in XaAES module directory
		 */
		flag = sysfile_exists(sysdir, "moose_w.adi");
		if (!flag) flag = sysfile_exists(sysdir, "moose.adi");
		if (!flag) flag = sysfile_exists(sysdir, "moose.adm");
		if (!flag) flag = sysfile_exists(sysdir, "moose_w.adm");
		
		if (flag)
		{
			BLOG((true, "ERROR: There exist an moose.adi or moose.adm in your FreeMiNT sysdir."));
			BLOG((true, " sysdir = '%s'", sysdir));
			BLOG((true, "       Please remove it and install it in the XaAES module directory"));
			BLOG((true, "       before starting the XaAES kernel module!"));
			err = EINVAL;
			goto error;
		}
	}

	sprintf(version, sizeof(version), "%i.%i", VER_MAJOR, VER_MINOR);
	sprintf(vversion, sizeof(vversion), "%s %s%s", version,  DEV_STATUS & AES_FDEVSTATUS_STABLE ? "Stable " : "", ASCII_DEV_STATUS);
#if GENERATE_DIAGS
	bzero(&D, sizeof(D));
	D.debug_level = 4;
#if 1 /*LOGDEBUG*/
	/* Set the default debug file */
	strcpy(D.debug_path, "xaaes.log");
	D.debug_file = kernel_open(D.debug_path, O_WRONLY|O_CREAT|O_TRUNC, NULL, NULL);
#else
	D.debug_file = NULL;
#endif
	sprintf(Aes_display_name, sizeof(Aes_display_name), "  XaAES(dbg) v%s", vversion);
#else
	sprintf(Aes_display_name, sizeof(Aes_display_name), "  XaAES v%s", vversion);
#endif
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
// 	cfg.widg_w = ICON_W;
// 	cfg.widg_h = ICON_H;

	cfg.ver_wheel_id = 0;
	cfg.ver_wheel_amount = 1;
	cfg.hor_wheel_id = 1;
	cfg.hor_wheel_amount = 1;

	cfg.icnfy_orient = 3;
	cfg.icnfy_w = 72;
	cfg.icnfy_h = 72;

	cfg.popup_timeout = 10;
	cfg.popout_timeout = 1000;

	cfg.alert_winds = 0xffff;

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

	C.Aes->type = APP_AESSYS;
	C.Aes->cmd_tail = "\0";
	//C.Aes->wt.e.obj = -1;

	strcpy(C.Aes->cmd_name, "XaAES");
	strcpy(C.Aes->name, Aes_display_name);
	strcpy(C.Aes->proc_name,"AESSYS  ");

	strcpy(C.Aes->home_path, self->fpath);
	make_module_path(C.Aes->module_path);
	BLOG((false, "module path: '%s'", C.Aes->module_path));
	

	C.Aes->xdrive = d_getdrv();
	d_getpath(C.Aes->xpath, 0);

	/* 
	 * check if there exist a moose.adi
	 * We dont start without it.
	 */
	if (first)
	{
		bool flag;

		flag = sysfile_exists(C.Aes->module_path, "moose_w.adm");
		if (!flag) flag = sysfile_exists(C.Aes->module_path, "moose.adm");
		
		if (!flag)
		{	
			BLOG((/*00000008*/true, "ERROR: There exist no moose.adm in your XaAES module directory."));
			BLOG((true, " -> '%s'", C.Aes->module_path));
			BLOG((/*00000009*/true, "       Please install it before starting the XaAES kernel module!"));
			err = EINVAL;
			goto error;
		}
	}

	/* clear my_global_aes[0] for old gemlib */
	my_global_aes[0] = 0;
	
	/* requires mint >= 1.15.11 */
	C.mvalidate = true;

	/* Print a text boot message */
	if (first)
		bootmessage();
	BLOG((false, "bootmessage ok!"));

	/* Setup the kernel OS call jump table */
	setup_handler_table();
	
	BLOG((false, "setup_handler_table ok!"));

	/* set bit 3 in conterm, so Bconin returns state of ALT and CTRL in upper 8 bit */
	{
		long helper;

		helper = (s_system(S_GETBVAL, 0x0484, 0)) | 8;
		s_system(S_SETBVAL, 0x0484, (char)helper);
	}
	BLOG((false, "set bit 3 in conterm ok!"));

#if GENERATE_DIAGS
	{ short nkc_vers = nkc_init(); DIAGS(("nkc_init: version %x", nkc_vers)); }
#else
	nkc_init();
#endif
	BLOG((false, "nkc_init ok!"));

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

	/* default to live actions */
	default_options.xa_nomove = true;
	default_options.xa_nohide = true;
	//default_options.xa_objced = true;
	default_options.thinframe = 1;
	default_options.wheel_mode = WHL_AROWWHEEL;

	default_options.clwtna = 1;

	C.Aes->options = default_options;
	
	/* Parse the config file */
	load_config();

	//C.Aes->options = default_options;
	//C.Aes->options.xa_nomove = false;

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

		oldmask = p->p_sigmask;
		p->p_sigmask = 0xffffffffUL;

		while (!(C.shutdown & QUIT_NOW))
			sleep(WAIT_Q, (long)&loader_pid);
		
// 		display("AESSYS kthread exited - C.shutdown = %x", C.shutdown);
		BLOG((false, "AESSYS kthread exited - C.shutdown = %x", C.shutdown));
	
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

	/* succeeded */
	return E_OK;

error:
#if GENERATE_DIAGS
	/* Close the debug output file */
	if (D.debug_file)
		kernel_close(D.debug_file);
#endif
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
