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
 
/*
 * Boot up code
 */

#define DEBUG_BOOT 1
#define SCL_HOOKS  0		/* Probe the files xa_user1.scl and xa_user2.scl */

#include RSCHNAME
#include WIDGHNAME

#include <mint/mintbind.h>
#include <mint/dcntl.h>
#include <mint/ssystem.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "xa_types.h"
#include "xa_global.h"

#include "bootup.h"
#include "config.h"
#include "c_window.h"
#include "desktop.h"
#include "handler.h"
#include "ipff.h"
#include "menuwidg.h"
#include "my_aes.h"
#include "nkcc.h"
#include "objects.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "semaphores.h"
#include "signals.h"
#include "taskman.h"
#include "widgets.h"
#include "xalloc.h"
#include "xa_clnt.h"
#include "xa_form.h"
#include "xa_fsel.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

#include "version.h"


#if DEBUG_BOOT
#define BTRACE(n) fdisplay(loghandle, false, "-= %d =-\n",n)
#else
#define BTRACE(n)
#endif

#if GENERATE_DIAGS
static void punit(XA_memory *base, XA_block *blk, XA_unit *unit, char *txt);
# endif

#if GENERATE_DIAGS
static char *scls_name = "xa_debug.scl";
static char *old_name  = "xaaesdbg.cnf";
#else
static char *scls_name = "xa_setup.scl";
static char *old_name  = "xaaes.cnf";
#endif
static char *scle_name = "xa_exec.scl";
#if SCL_HOOKS
static char *scl1_name = "xa_user1.scl";
static char *scl2_name = "xa_user2.scl";
#endif
#if SEPARATE_SCL
static char *sclp_name = "xa_scl.prg";
#endif

static char Aes_display_name[32];
Path Aes_home_path;

long loghandle;

int err = 0;

/* Global config data */
struct config cfg;

/* The screen descriptor */
struct xa_screen screen;

/* All areas that are common. */
struct common C;

static char version[] = ASCII_VERSION;

/* Pointer to the widget resource (icons) */
static void *widget_resources;

/* current no of env strings. */
static int envs = 0;

static XA_COLOUR_SCHEME default_colours = {G_LWHITE, G_BLACK, G_LBLACK, G_WHITE, G_BLACK, G_CYAN};
static XA_COLOUR_SCHEME bw_default_colours = {G_WHITE, G_BLACK, G_BLACK, G_WHITE, G_BLACK, G_WHITE};

static vdi_vec *svmotv = 0;
static vdi_vec *svbutv = 0;
static vdi_vec *svwhlv = 0;
/* static vdi_vec *svtimv = 0; */

char moose_name[] = "u:\\dev\\moose";

static long protection_flags = 0;

static void
bootmessage(unsigned long mint)
{
	if (mint)
	{
		fdisplay(loghandle, true, "It's FreeMiNT v%ld.%ld.%ld%c%c  (%lx)\n",
			 (mint >> 24),
			 (mint >> 16) & 255,
			 (mint >>  8) & 255,
			 (mint & 255) ? '.' : ' ',
			 (mint & 255) ? mint & 255 : ' ', 
			 (mint));

		fdisplay(loghandle, true, " and ");
	}

	fdisplay(loghandle, true, "%s\n", Aes_display_name);
	fdisplay(loghandle, true, "MultiTasking AES for MiNT\n");
	fdisplay(loghandle, true, "(w)1995,96,97,98,99 Craig Graham, Johan Klockars, Martin Koehling, Thomas Binder\n");
	fdisplay(loghandle, true, "     and other assorted dodgy characters from around the world...\n");
	fdisplay(loghandle, true, "   Using Harald Siegmunds NKCC\n");
	fdisplay(loghandle, true, "   1999-2002 Henk Robbers @ Amsterdam\n");
	fdisplay(loghandle, true, "Date: %s, time: %s\n", __DATE__,__TIME__);
	fdisplay(loghandle, true, "Supports mouse wheels\n");
	fdisplay(loghandle, true, "Compile time switches enabled:\n");

	IFDIAG(fdisplay(loghandle, true, " - Diagnostics\n");)

#if DISPLAY_LOGO_IN_TITLE_BAR
	fdisplay(loghandle, true, " - Logo in title bar\n");
#endif

#if POINT_TO_TYPE
	fdisplay(loghandle, true, " - Point-to-type capability\n");
#endif

#if ALT_CTRL_APP_OPS
	fdisplay(loghandle, true, " - CTRL+ALT key-combo's\n");
#endif

#if MEMORY_PROTECTION
	fdisplay(loghandle, true, " - OS_SPECIAL memory access\n");
	fdisplay(loghandle, true, " -     protection_flags: 0x%lx\n",protection_flags);
#endif

#if VECTOR_VALIDATION
	if (C.mvalidate)
		fdisplay(loghandle, true, " - Client vector validation\n");
#endif

	fdisplay(loghandle, true, " - Realtime (live) window scrolling, moving and sizing\n");

#if PRESERVE_DIALOG_BGD
	fdisplay(loghandle, true, " - Preserve dialog backgrounds\n");
#endif

#if !FILESELECTOR
	fdisplay(loghandle, true, " - Built without file selector\n");
#endif
#if !TEAR_OFF
	fdisplay(loghandle, true, " - Menu's can *not* be teared off\n");
#endif

	if (cfg.superprogdef)
		fdisplay(loghandle, true, "progdefs are executed in supervisor mode\n");

	if (cfg.fsel_cookie)
		fdisplay(loghandle, true, " - FSEL cookie found\n");

	if (lcfg.falcon == 0x0030000L)
		fdisplay(loghandle, true, " - Falcon video handled\n");

	if (cfg.auto_program)
	{
		if (lcfg.havemode)
			fdisplay(loghandle, true, " - video mode %d(x%x)\n", lcfg.modecode, lcfg.modecode);

		fdisplay(loghandle, true, "auto program\n");
	}
}

static void
load_accs(void)
{
	char search_path[200];
	char acc[200];
	int fnd;
	_DTA *my_dta = Fgetdta();

	sdisplay(search_path,"%s*.ACC", cfg.acc_path);

	if (*(cfg.acc_path + 1) == ':')
	{
		Dsetdrv(tolower(*cfg.acc_path) - 'a');
		Dsetpath(cfg.acc_path + 2);
	}
	else
		Dsetpath(cfg.acc_path);

	fnd = Fsfirst(search_path, 0xff);
	while (!fnd)
	{
		sdisplay(acc, "%s%s", cfg.acc_path, my_dta->dta_name);
		DIAGS(("Launch accessory: '%s'\n", acc));

		launch(NOLOCKING, 3, 0, 0, acc, "", C.Aes);

		fnd = Fsnext();
	}

	Dsetdrv(C.home_drv);
	Dsetpath(C.home);
}

/*
 * Cleanup on exit
 */
static void
cleanup(void)
{
	XA_CLIENT *client;
	XA_WINDOW *w;

	DIAGS(("Cleaning up ready to exit....\n"));

	if (svmotv)
	{
		void *m, *b, *h;

		vex_motv(C.P_handle, svmotv, &m);
		vex_butv(C.P_handle, svbutv, &b);

		if (svwhlv)
			vex_wheelv(C.P_handle, svwhlv, &h);
	}

	Psignal(SIGCHLD, 0L);

	DIAGS(("Cleaning up clients\n"));
	client = S.client_list;
	while (client)
	{
		XA_CLIENT *next = client->next;

		DIAGS((" - %d,%d\n", client->pid, client->client_end));
		if (is_client(client))
		{
			DIAGS(("Cleaning %s\n", c_owner(client)));
			remove_refs(client, true);
			XA_client_exit(NOLOCKS, client, 0);
		}
		client = next;
	}

	DIAGS(("Removing clients\n"));

	client = S.client_list;
	while (client)
	{
		XA_CLIENT *next = client->next;

		DIAGS((" - %d,%d  %d\n", client->pid, client->client_end, client->killed));
		if (client->pid == C.AESpid)
		{
			clear_clip();
			redraw_desktop(NOLOCKING, root_window);
		}
		else if (client->killed)
		{
			int pid = client->pid;
			DIAGS(("Removing %s\n", c_owner(client)));
			close_client(NOLOCKING, client);
			(void) Pkill(pid, SIGKILL);
		}
		client = next;
	}

#if 0
	DIAGS(("Killing clients\n"));

	client = S.client_list;
	while (client)
	{
		XA_CLIENT *next = client->next;

		DIAGS((" - %d,%d\n", client->pid, client->client_end));
		if (is_client(client))
		{
			long ex_con;

			/* If client ignored SIGTERM, send SIGKILL */
			ex_con = Pwaitpid(client->pid, 1, NULL);
			if (ex_con == 0)
			{
				DIAGS(("Killing %s\n", c_owner(client)));
				Pkill(client->pid, SIGKILL);
			}
		}
		client = next;
	}
#endif


	DIAGS(("Freeing open windows\n"));

	w = window_list;
	while (w)
	{
		XA_WINDOW *nx = w->next;
		close_window(NOLOCKING, w);
		delete_window(NOLOCKING, w);
		w = nx;
	}
	DIAGS(("Freeing closed windows\n"));
	w = S.closed_windows.first;
	while (w)
	{
		XA_WINDOW *nx = w->next;
		delete_window(NOLOCKING, w);
		w = nx;
	}

	DIAGS(("Freeing Aes environment\n"));
	if (C.env)
		free(C.env);

	DIAGS(("Freeing Aes resources\n"));
	/* To demonstrate the working on multiple resources. */
	FreeResources(C.Aes, 0);		/* first:  widgets */
	FreeResources(C.Aes, 0);		/* then:   big resource */

	DIAGS(("Freeing Options\n"));
	{
		OPT_LIST *nop, *op = S.app_options;
		while (op)
		{
			nop = op->next;
			free(op);
			op = nop;
		}
	}

	DIAGS(("Freeing Symbols\n"));
	
	free_sym();

#if GENERATE_DIAGS
	DIAGS(("Reporting memory leaks\n"));
	XA_leaked(0, -1, -1, punit);
#endif

	DIAGS(("Freeing what's left\n"));
	_FreeAll();		/* Free (default) global base */

	unhook_from_vector();

	if (C.MOUSE_dev > 0)
		Fclose(C.MOUSE_dev);

	nkc_exit();

	if (C.KBD_dev > 0)
		Fclose(C.KBD_dev);

	if (C.AES_in_pipe > 0)
		Fclose(C.AES_in_pipe);

	if (C.Salert_pipe > 0)
		Fclose(C.Salert_pipe);

	/* Remove semaphores: */
	destroy_semaphores(loghandle);

	DIAGS(("Bye!\n"));
	DIAGS(("\n"));

#if GENERATE_DIAGS
	/* Close the debug output file */
	if (D.debug_file > 0)
		Fclose(D.debug_file);
#endif

	t_color(G_BLACK);
	wr_mode(MD_REPLACE);

	/* Shut down the VDI */
	v_clrwk(C.vh);

	if (cfg.auto_program)
	{
#if 0
		int i;

		for (i=512; i>=C.vh; i--)
		{
			char p[20];
			sdisplay(p, "-= vh %d =- ", i);
			v_gtext(C.vh, 0, 64, p);
			v_clsvwk(i);
		}
#else
		v_clsvwk(C.vh);
#endif
/* HR 130302: v_clswk bombs with NOVA VDI 2.67 & Memory Protection.
              so I moved this to the end of the cleanup, AFTER closing the debugfile. */

		v_enter_cur(C.P_handle);	/* Ozk: Lets enter cursor mode */
		v_clswk(C.P_handle);		/* Auto version must close the physical workstation */

		display("\033e\033H");			/* HR 100302: Cursor enable, cursor home */
	}
	else
	{
		v_clsvwk(C.vh);
		mt_appl_exit(my_global_aes);
	}
}

/*
 * (Re)initialise the mouse device /dev/moose
 */
static bool
init_moose(void)
{
	struct fs_info info;
	long major, minor;
	struct moose_vecsbuf vecs;
	unsigned short dclick_time;

	if (!C.MOUSE_dev)
	{
		C.MOUSE_dev = Fopen(moose_name, O_RDWR);
		if (C.MOUSE_dev < 0)
		{
			err = fdisplay(loghandle, true, "Can't open %s\n", moose_name);
			return false;
		}
	}

	/* first check the xdd version */
	if (Fcntl(C.MOUSE_dev, &info, FS_INFO) != 0)
	{
		err = fdisplay(loghandle, true, "Fcntl(FS_INFO) failed, do you use the right xdd?\n");
		return false;
	}

	major = info.version >> 16;
	minor = info.version & 0xffffL;
	if (major != 0 || minor < 4)
	{
		err = fdisplay(loghandle, true, ", do you use the right xdd?\n");
		return false;
	}

	if (Fcntl(C.MOUSE_dev, &vecs, MOOSE_READVECS) != 0)
	{
		err = fdisplay(loghandle, true, "Moose set dclick time failed\n");
		return false;
	}

	if (vecs.motv)
	{
		vex_motv(C.P_handle, vecs.motv, (void **)(&svmotv));
		vex_butv(C.P_handle, vecs.butv, (void **)(&svbutv));

		if (vecs.whlv)
		{
			vex_wheelv(C.P_handle, vecs.whlv, (void **)(&svwhlv));
			fdisplay(loghandle, true, "Wheel support present\n");
		}
		else
			fdisplay(loghandle, true, "No wheel support!!\n");
	}

	dclick_time = lcfg.double_click_time;
	if (Fcntl(C.MOUSE_dev, &dclick_time, MOOSE_DCLICK) != 0)
		err = fdisplay(loghandle, true, "Moose set dclick time failed\n");

	return true;
}

void
reopen_moose(void)
{
#if 1
	unsigned short dclick_time = 50;
#else
	unsigned short dclick_time = lcfg.double_click_time;
#endif

	C.MOUSE_dev = Fopen(moose_name, O_RDWR);

	if (C.MOUSE_dev >= 0)
	{
		if (Fcntl(C.MOUSE_dev, &dclick_time, MOOSE_DCLICK) != 0)
		{
			DIAG((D_mouse, 0, "moose set dclick time failed\n"));
		}
	}
}


XA_WINDOW *root_window;

static char aessys_name[32] = "AESSYS";

/*
 * Startup & Initialisation...
 * - Spawn off any extra programs we need (mouse server, etc).
 * - Open physical & virtual workstations.
 * - Install our trap handler.
 * - Run the xaaes.cnf startup script.
 */
int
xaaes_init(int argc, char **argv, char **env)
{
	LOCK lock = NOLOCKS;
	short work_in[12];
	short work_out[58];
	short f;
	char *resource_name, *full;
	long old_sp = 0;
	long helper = 0;

	XA_set_base(0, 32768, -1, 0);

	/* open the log. */
	loghandle = Fcreate("xa_setup.log", 0);

	lcfg.booting = true;
	bzero(&default_options, sizeof(default_options));
	bzero(&cfg, sizeof(cfg));
	bzero(&S, sizeof(S));
	bzero(&C, sizeof(C));
	strcpy(C.cmd_name, "u:\\pipe\\XaAES.cmd");

	
#if GENERATE_DIAGS
#if DEBUG_BOOT
	bzero(&D, sizeof(D));
	D.debug_level = 4;
	D.debug_file = (int)Fcreate("xaaes.log", 0);
	Fforce(1, D.debug_file);
#endif
	sdisplay(Aes_display_name, "  XaAES(dbg) %s", version);
BTRACE(1);
#else
	sdisplay(Aes_display_name, "  XaAES %s", version);
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
	DIAGS(("XaAES is compiled without builtin fileselector\n"));
#endif

	{
		char **p = env;
BTRACE(2);
		while (*p && (p - env < STRINGS))
			C.strings[envs++] = *p++;
BTRACE(3);
		C.env = C.strings[0];
BTRACE(4);
	}

	/* Check that MiNT is actually installed */
	if (Ssystem(-1, 0, 0) != 0)
	{
BTRACE(5);
		bootmessage(0);
		fdisplay(loghandle, true, "Sorry, XaAES requires MiNT >= 1.15.0 to run.");

#if GENERATE_DIAGS
		if (!D.debug_file)
		{
			display(">");
			Bconin(2);
		}
#endif
		Fclose(loghandle);
		Pterm(1);
	}

	/* Let's get our MiNT process id being as MiNT is loaded... */
BTRACE(6);
	C.AESpid = Pgetpid();

BTRACE(7);
	(void) Pnice(-1);
	(void) Pdomain(1);	/* HR The kernel runs in the MiNT domain. */

BTRACE(8);
	get_procname(C.AESpid, aessys_name, sizeof(aessys_name));
	argv[0] = aessys_name;

BTRACE(9);
	/* Ulrichs advice */
	sdisplay(cfg.scrap_path, "u:\\proc\\%s.%d", argv[0], C.AESpid);
	sdisplay(cfg.acc_path,   "u:\\proc\\AESSYS.%d",      C.AESpid);
	fdisplay(loghandle, true, "rename: '%s' to '%s'\n", cfg.scrap_path, cfg.acc_path);

BTRACE(10);
	(void) Frename(0, cfg.scrap_path, cfg.acc_path);

BTRACE(11);
	/* 'FSEL' */
	cfg.fsel_cookie = Ssystem(S_GETCOOKIE, 0x4653454cL, 0) != 0xffffffff;

	/* dynamic window handle allocation. */
	clear_wind_handles();

BTRACE(12);
	/* Initialise the object tree display routines */
	init_objects();

	/* So we can speak to and about ourself. */
	C.Aes = NewClient(C.AESpid);

BTRACE(13);
	if (C.Aes == 0)
	{
		err = fdisplay(loghandle, true, "XaAES ERROR: Can't allocate memory ?\n");
		cleanup() ;
		return -1;
	}
	strcpy(C.Aes->cmd_name,"XaAES");
	C.Aes->parent = Pgetppid();
	strcpy(C.Aes->name, Aes_display_name);
	strcpy(C.Aes->proc_name,"AESSYS  ");

BTRACE(14);
	/* Change the protection mode to OS_SPECIAL to allow operation with
	 * memory protection (doesn't work yet, what's wrong?)
	 */
	/* Craig's Note: I think this causes a massive memory leak in the MiNT kernal, so
	 * I've disabled it
	 */
#if MEMORY_PROTECTION
	{
#ifndef F_PROT_S
#define F_PROT_S	0x20
#endif
#ifndef F_OS_SPECIAL
#define F_OS_SPECIAL	0x8000
#endif
		short proc_handle;
		long protection;

BTRACE(15);
		/* Opening "u:\proc\.-1" opens the current process... */

		proc_handle = Fopen("u:\\proc\\.-1", O_RDONLY);
		if (proc_handle >= 0)
		{
BTRACE(16);
			/* get process memory flags */
			Fcntl(proc_handle, (long) &protection, PGETFLAGS);

			protection_flags = protection;

			/* delete protection mode bits */
			protection &= 0xffffff0fUL;

			/* super access, OS special */
			protection |= F_OS_SPECIAL | F_PROT_S;
BTRACE(17);
			/* set process memory flags */
			Fcntl(proc_handle, (long)&protection, PSETFLAGS);
BTRACE(18);
			Fclose(proc_handle);
BTRACE(19);
		}
		else
			; /* XXX warn at least about that */
	}
#endif

	/* Where were we started? */

	C.home_drv = Dgetdrv();
	DIAGS(("Home drive: %d\n", C.home_drv));

	Dgetpath(C.home, 0);
	DIAGS(("Home directory: '%s'\n", C.home));

	Dgetcwd(C.home, C.home_drv + 1, sizeof(C.home) - 1);

	DIAGS(("current working directory: '%s'\n", C.home));
	DIAGS(("argv[0]: '%s'\n", argv[0]));

	sdisplay(Aes_home_path, "%c:%s", C.home_drv + 'A', C.home);

	C.Aes->xdrive = C.home_drv;
	strcpy(C.Aes->xpath, C.home);

	/* Are we an auto/mint.cnf launched program? */

BTRACE(20);
	/* clear my_global_aes[0] for old gemlib */
	my_global_aes[0] = 0;
	mt_appl_init(my_global_aes);
BTRACE(21);
	cfg.auto_program = (my_global_aes[0] == 0);

	for (f = 0; f < 10; f++)
		work_in[f] = 1;

	work_in[10] = 2;

	if (cfg.auto_program)
	{
#if 0
		/* Any shift key and low rez */
		if ((Kbshift(-1) & 3) && (Getrez() == 0))
			/* Switch to medium rez */
			Setscreen((void*)-1L, (void*)-1L, 1, 0);
		else
#endif
		{
			/* Set video mode from command line parameter? */			
			if (argc > 2)
			{
				lcfg.havemode = true;
				ipff_in(argv[2]);
				sk();
				switch (tolower(*argv[2]))
				{
				case 'o':  lcfg.modecode = (skc(), oct());  break;
				case 'x':  lcfg.modecode = (skc(), hex());  break;
				case 'b':  lcfg.modecode = (skc(), bin());  break;
				default :  lcfg.modecode =         idec();  break;
				}

				/* Falcon Video mode switch */
				if (stricmp("-fvideo", argv[1]) == 0)
				{
					/* '_VDO' */
					Ssystem(S_GETCOOKIE, 0x5f56444fL, (long)&lcfg.falcon);
					/* ´SCPN´ - Screenblaster present? */
					helper = Ssystem(S_GETCOOKIE, 0x5343504e, 0L);
					helper = (helper == -1) ? 0 : 1;

					if (lcfg.falcon == 0x00030000L)
					{
						work_out[45] = lcfg.modecode;
						/* Ronald Andersson: This should be the mehod for falcon. */
						work_in[0] = 5;
						DIAGS(("Falcon: mode %d(%x)\n", lcfg.modecode, lcfg.modecode));
					}
					else
					{
						DIAGS(("-fvideo: No, or incorrect _VDO cookie: %lx\n", lcfg.falcon));
					}

				}
				else
				{
					/* Video mode switch */
					if (stricmp("-video", argv[1]) == 0)
					{
						work_in[0] = lcfg.modecode;
						DIAGS(("Standard: mode %d(%x)\n", lcfg.modecode, lcfg.modecode));
					}
					else
					{
#if 0
						lcfg.modecode = Getrez();
						work_in[0] = lcfg.modecode + 2;
						DIAGS(("Standard: Getrez() = %d(%x)\n", lcfg.modecode, lcfg.modecode));
#else
						DIAGS(("Invalid agument: '%s'\n", argv[1]));
#endif
					}
				}
			}
			else
			{
				DIAGS(("Default screenmode.\n"));
			}

#if GENERATE_DIAGS
			/* Before screen is cleared :-) */
			if (!D.debug_file)
			{
				display(">");
				Bconin(2);
			}
#endif


			/* switch to supervisor-mode                    */
			/* this is done because screenblaster-software  */
			/* tries to write the new screen-adress to $44E */

			if (helper) old_sp = Super(0L);
			
			v_opnwk(work_in, &C.P_handle, work_out);

			if (helper) Super(old_sp);

			fdisplay(loghandle, true, "\033HPhysical work station opened: %d\n",C.P_handle);

			v_exit_cur(C.P_handle);		/* Ozk: We need to get rid of the cursor */
		}
	}
	else
	{
		/* The GEM AES has already been started,
		 * so get the physical workstation handle from it
		 */
		short junk;
		C.P_handle = mt_graf_handle(&junk, &junk, &junk, &junk, my_global_aes);
	}

	/* Setup the kernel OS call jump table */
	
BTRACE(22);
	setup_k_function_table();


BTRACE(23);

	/* Create a whole wodge of semphores */
	if (create_semaphores(loghandle))
		return 1;

BTRACE(24);
	lcfg.mint = Ssystem(S_OSVERSION, 0, 0);

BTRACE(124);
#if MEMORY_PROTECTION
	/* mint >= 1.15.11 */
	if (lcfg.mint >= 0x10f0b00)
		C.mvalidate = true;
#endif

BTRACE(224);
	/* Print a text boot message */
	bootmessage(lcfg.mint);


	/* set bit 3 in conterm, so Bconin returns state of ALT and CTRL in upper 8 bit */
	(char)helper = (Ssystem(S_GETBVAL, 0x0484, NULL)) | 8;
	Ssystem(S_SETBVAL, 0x0484, (char)helper);

BTRACE(25);


	/* Patch the AES trap vector to use our OS. */
	hook_into_vector();
BTRACE(26);

	/* Open the MiNT Salert() pipe to be polite about system errors */
	C.Salert_pipe = Fopen("u:\\pipe\\alert", O_CREAT|O_RDWR);

	/* Open the u:/dev/console device to get keyboard input */

	C.KBD_dev = Fopen("u:\\dev\\console", O_DENYRW|O_RDONLY);
	if (C.KBD_dev < 0)
	{
		err = fdisplay(loghandle, true, "XaAES ERROR: Can't open /dev/console ?\n");
		cleanup() ;
		return -1;
	}

#if GENERATE_DIAGS
	{ short nkc_vers = nkc_init(); DIAGS(("nkc_init: version %x\n", nkc_vers)); }
#else
	nkc_init();
#endif

	/* Open us a virtual workstation for XaAES to play with */

BTRACE(27);
	C.vh = C.P_handle;
	v_opnvwk(work_in, &C.vh, work_out);
	vsf_perimeter(C.vh, 0);		/* HR 051002: from set_colours; never set to 1 ever */

BTRACE(28);
	graf_mouse(ARROW, 0);
	v_hide_c(C.vh);
	wr_mode(MD_TRANS); /* We run in TRANSPARENT mode for all AES ops (unless otherwise requested) */
	vst_alignment(C.vh, 0, 5, &f, &f); /* YESss !!! */

	DIAGS(("Virtual work station opened: %d\n",C.vh));

	/* Setup the screen parameters */

BTRACE(29);
	screen.r.x = screen.r.y = 0;
	screen.r.w = work_out[0] + 1;	/* HR + 1 */
	screen.r.h = work_out[1] + 1;	/* HR + 1 */
	clear_clip();
BTRACE(30);
	screen.colours = work_out[13];
	screen.display_type = D_LOCAL;

	screen.dial_colours =
		MONO ? bw_default_colours : default_colours;

BTRACE(31);
	vq_extnd(C.vh, 1, work_out);	/* Get extended information */
	screen.planes = work_out[4];	/* number of planes in the screen */

	DIAGS(("Video info: width(%d/%d), planes :%d, colours %d\n",
				screen.r.w, screen.r.h, screen.planes, screen.colours));

	/* Set the default clipboard */
	strcpy(cfg.scrap_path, "c:\\clipbrd\\");

	/* Set the default accessory path */
	strcpy(cfg.acc_path, "c:\\");

	/* By default debugging output goes to the file "./debug.lst"*/
	IFDIAG (strcpy(D.debug_path, "xaaes.log");)

	strcpy(C.Aes->home_path, Aes_home_path);

#if GENERATE_DIAGS
	if (!D.debug_file)
	{
		display(">");
		Bconin(2);
	}
#endif

	/* Create the XaAES.cmd introduction pipe */

	C.AES_in_pipe = Fopen(C.cmd_name, O_CREAT|O_RDWR);
	fdisplay(loghandle, true, "Open '%s' to %ld\n", C.cmd_name, C.AES_in_pipe);
	if (C.AES_in_pipe < 0)
	{
		err = fdisplay(loghandle, true, "XaAES ERROR: Can't open '%s' :: %ld\n",
			       C.cmd_name, C.AES_in_pipe);
		cleanup() ;
		return -1;
	}

	/* Parse the standard startup file.
	 * This can redirect the debugging output to another file/device
	 */
	full = xa_find(scls_name);
	if (full)
	{
		SCL(NOLOCKING, 0, scls_name, full, 0);
	}
	else /* transition time */
	{
		SCL(NOLOCKING, 2, old_name, 0, 0);
		lcfg.oldcnf = true;
	}

#if SCL_HOOKS
	full = xa_find(scl1_name);
	if (full)
		SCL(NOLOCKING, 0, scl1_name, full, 0);
#endif

BTRACE(32);
	C.Aes->options = default_options;
BTRACE(33);

	/* Open /dev/moose (040201: after xa_setup.scl for mouse configuration) */
	
	if (!init_moose())
	{
BTRACE(34);
		cleanup();
		return -1;
	}

	/* If we are using anything apart from the system font for windows,
	 * better check for GDOS and load the fonts.
	 */

	if (cfg.font_id != 1)
	{
BTRACE(35);
		if (vq_gdos())	/* Yeah, I know, this is assuming the old-style vq_gdos() binding */
		{
BTRACE(36);
			vst_load_fonts(C.vh, 0);
BTRACE(37);
		}
		else
		{
BTRACE(38);
			cfg.font_id = 1;
		}
	}


	/* Set standard AES font */

BTRACE(39);
	screen.standard_font_id = screen.small_font_id =
		vst_font(C.vh, cfg.font_id);


	/* 110202: Use the ability of vst_point to return the character cell measures. */
	/* Select Small font */

BTRACE(40);
	screen.small_font_point = vst_point(C.vh,
					cfg.small_font_point,
					&f,
					&screen.small_font_height,
					&screen.c_min_w,
					&screen.c_min_h);


	/* Select standard font */

BTRACE(41);
	screen.standard_font_point = vst_point(C.vh,
 		(screen.r.h <= 280) ? cfg.medium_font_point : cfg.standard_font_point,
		&f,
		&screen.standard_font_height,
		&screen.c_max_w,
		&screen.c_max_h);

	/* Open a diagnostics file? All console output can be considered diagnostics,
	 * so I just redirect the console to the required file/device
	 */

	DIAGS(("Display Device: Phys_handle=%d, Virt_handle=%d\n", C.P_handle, C.vh));
	DIAGS((" size=[%d,%d], colours=%d, bitplanes=%d\n", screen.r.w, screen.r.h, screen.colours, screen.planes));

	/* Load the system resource files */
	resource_name = xa_find(lcfg.rsc_name);
	if (resource_name)
	{
		C.Aes_rsc = LoadResources(C.Aes, resource_name, 0, DU_RSX_CONV, DU_RSY_CONV);
		fdisplay(loghandle, true, "system resource = %lx (%s)\n",
			 C.Aes_rsc, lcfg.rsc_name);
	}	
	if (!resource_name || !C.Aes_rsc)
	{
		err = fdisplay(loghandle, true, "ERROR: Can't find/load system resource file '%s'\n", lcfg.rsc_name);
		cleanup();
		return -1;
	}

	widget_resources = NULL;
	resource_name = xa_find(lcfg.widg_name);
	if (resource_name)
	{
		widget_resources = LoadResources(C.Aes, resource_name, 0, DU_RSX_CONV, DU_RSY_CONV);
		fdisplay(loghandle, true, "widget_resources = %lx (%s)\n",
			 widget_resources, lcfg.widg_name);
	}
	if (!resource_name || !widget_resources)
	{
		err = fdisplay(loghandle, true, "ERROR: Can't find/load widget resource file '%s'\n", lcfg.widg_name);
		cleanup();
		return -1;
	}

	/* 251201: get widget object parameters. */
	{
		RECT c;
		OBJECT *tree = ResourceTree(widget_resources, 0);
		object_area(&c, tree, 1, 0, 0);
		cfg.widg_w = c.w;
		cfg.widg_h = c.h;
		cfg.widg_dw = (tree[1].ob_width - c.w)/2;
		cfg.widg_dh = (tree[1].ob_height - c.h)/2;

		fdisplay(loghandle, true, "cfg.widg: %d/%d   %d/%d\n", cfg.widg_w, cfg.widg_h, cfg.widg_dw, cfg.widg_dh);
	}
#if FILESELECTOR
	/* Do some itialisation */
BTRACE(42);
	init_fsel();
BTRACE(43);
#endif

	/* Create the root (desktop) window
	 * - We don't want messages from it, so make it a NO_MESSAGES window
	 */
	
	DIAGS(("creating root window\n"));

	root_window = create_window(
				NOLOCKING,
				0,			/* No messages */
				C.Aes,
				false,
				XaMENU,			/* HR: menu standard widget */
				created_for_AES,
				-1,			/* HR: -1, no frame, overscan window */
				false,false,
				screen.r,		/* current size */
				&screen.r,		/* maximum size (NB default would be rootwindow->wa) */
				0);			/* need no remembrance */

	/* Tack a menu onto the root_window widget */
	C.Aes->std_menu.tree = ResourceTree(C.Aes_rsc, SYSTEM_MENU);
	C.Aes->std_menu.owner = C.Aes;
#if 0
	set_ob_spec(C.Aes->std_menu.tree, SYS_DESK, (long)Aes_display_name);
	(C.Aes->std_menu.tree + SYS_DESK)->r.w = strlen(Aes_display_name) * screen.c_max_w;
#endif
BTRACE(44);
	fix_menu(C.Aes->std_menu.tree, true);
BTRACE(45);
	set_menu_widget(root_window, &C.Aes->std_menu);

BTRACE(46);
	{
		char *vs = get_ob_spec(C.Aes->std_menu.tree + SYS_DESK)->free_string;
		strcpy(vs + strlen(vs) - 3, version + 3);
	}

	DIAGS(("menu widget set\n"));

	/* Fix up the file selector menu */
BTRACE(47);
	fix_menu(ResourceTree(C.Aes_rsc, FSEL_MENU),false);

	/* Fix up the window widget bitmaps and any others we might be using
   	 * (Calls vr_trnfm() for all the bitmaps)
   	 * HR: No, it doesnt! ;-)
   	 */
	 
	DIAGS(("fixing up widgets\n"));
	fix_default_widgets(widget_resources);

	/* Set a default desktop */

	DIAGS(("setting default desktop\n"));
	
	{
		OBJECT *ob = get_xa_desktop();
		*(RECT*)&ob->ob_x = root_window->r;
		(ob + DESKTOP_LOGO)->ob_x = (root_window->wa.w - (ob + DESKTOP_LOGO)->ob_width) / 2;
		(ob + DESKTOP_LOGO)->ob_y = (root_window->wa.h - (ob + DESKTOP_LOGO)->ob_height) / 2;
		C.Aes->desktop.tree = ob;
		C.Aes->desktop.owner = C.Aes;
		
		set_desktop_widget(root_window, &C.Aes->desktop);
		set_desktop(&C.Aes->desktop);
	}

	DIAGS(("setting up task manager\n"));
	
/* HR moved the title to set_slist_object() */
	/* Setup the task manager */
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, TASK_MANAGER), TM_LIST);
BTRACE(48);

	/* Setup the file selector */
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, FILE_SELECT), FS_LIST);
BTRACE(49);

	/* Setup the System Alert log */
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, SYS_ERROR), SYSALERT_LIST);
BTRACE(50);

	/* Setup the About text list */
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, ABOUT_XAAES), ABOUT_LIST);
BTRACE(51);

	/* Display the root window */
	
	DIAGS(("About to display root window\n"));

	open_window(NOLOCKING, root_window, screen.r);
	
	/* Initial iconified window coords */

BTRACE(52);
	C.iconify = iconify_grid(0);

	/* Set our process priority
	 * - a high negative value here improves performance a lot, 
	 * so I set it to -20 in XA_DEFS.H. This doesn't hurt the rest
	 * of the systems performance, as there are no busy wait loops
	 * in the server, and it spends most of its time blocked
	 * waiting for commands.
	 */


	/* Catch SIGCHLD to spot dead children */
	
BTRACE(53);
	Psignal(SIGCHLD, (long)HandleSIGCHLD);

	/* Catch SIGINT and SIGQUIT so we can shutdown with CTRL+ALT+C */

#if 1
	Psignal(SIGINT, (long)HandleSIGINT);
	Psignal(SIGQUIT, (long)HandleSIGQUIT);
#endif

	v_show_c(C.vh, 0); /* 0 = reset */		

	if (cfg.opentaskman)
		open_taskmanager(NOLOCKING);

	/* Load Accessories */
	DIAGS(("loading accs\n"));
	load_accs();
	DIAGS(("loading accs done!\n"));

	/* Execute startup
	 * This can run programs
	 */

#if SCL_HOOKS
	full = xa_find(scl2_name);
	if (full)
		SCL(NOLOCKING, 1, scl2_name, full, 0);
#endif

	if (!lcfg.oldcnf)
	{
#if !SEPARATE_SCL
		full = xa_find(scle_name);
		if (full)
			SCL(NOLOCKING, 1, scle_name, full, 0);
#else
	/* 130402: xa_scl */
		char par[256];
		char *p, *n;

		n = xa_find(scle_name);
		if (n)
		{
			par[0] = sdisplay(par + 1, "%s", n); /* Grrrrrrr!!!!! */
			p = xa_find(sclp_name);
			if (p)
				launch(NOLOCKING, 0, 0, 0, p, par, C.Aes);
		}
#endif
	}

	/* I just like to have the unlocks at the same level as the locks. */

	/* Unlock the semaphores...we're ready to go */
#if 0
	Sema_Dn(trap);
#endif
	Sema_Dn(desk);
	Sema_Dn(lck_update);
	Sema_Dn(mouse);
	Sema_Dn(fsel);

	Sema_Dn(envstr);
	Sema_Dn(pending);

	Sema_Dn(clients);
	Sema_Dn(appl);

	DIAGS(("Semaphores Unlocked!!!\n"));

	fdisplay(loghandle, false, "*** End of successfull setup ***\n");
	// don't close the log handle, useful for debugging
	// Fclose(loghandle);

	lcfg.booting = false;


	DIAGS(("------------- Going to the kernel -------------\n"));

	/* Call the main command interpreter kernel */
	XaAES();

	/* Closedown & exit */
	cleanup();


	if (C.shutdown & HALT_SYSTEM)
	{
		Shutdown(0);  /* halt */
	}
	else if (C.shutdown & REBOOT_SYSTEM)
	{
		Shutdown(1);  /* warm start */
	}
	
	
	return 0;	
	
}

#if GENERATE_DIAGS
static void
punit(XA_memory *base, XA_block *blk, XA_unit *unit, char *txt)
{
	display("**** %s: ", txt);
	
	if (unit)
	{
		XA_unit *prior = unit->prior;
		XA_unit *next = unit->next;
		
		display(" -%d- %ld :: %ld, p:%ld :: %ld, n:%ld :: %ld, block %ld :: %ld\n",
			unit->key,
			unit, unit->size,
			prior, prior?prior->size:-1,
			next, next?next->size:-1,
			blk, blk->size);
	}
	else
		display("0\n");
}
#endif
