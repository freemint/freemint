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

#include RSCHNAME

#include "k_init.h"
#include "xa_global.h"
#include "xa_codes.h"

#include "app_man.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "desktop.h"
#include "init.h"
#include "op_names.h"
#include "nkcc.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xalloc.h"

#include "xa_evnt.h"
#include "xa_form.h"
#include "xa_fsel.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/signal.h"

#include "objects.h"
#include "menuwidg.h"
#include "my_aes.h"
#include "xa_graf.h"

/*
 * global data
 */

/* The screen descriptor */
struct xa_screen screen;

struct xa_window *root_window;

/*
 * private data
 */

/* Pointer to the widget resource (icons) */
static void *widget_resources;

/* current no of env strings. */
static int envs = 0;

static struct xa_colour_scheme default_colours = { G_LWHITE, G_BLACK, G_LBLACK, G_WHITE, G_BLACK, G_CYAN };
static struct xa_colour_scheme bw_default_colours = { G_WHITE, G_BLACK, G_BLACK, G_WHITE, G_BLACK, G_WHITE };

int
k_init(void)
{
	short work_in[16];
	short work_out[58];
	short f;
	char *resource_name;

	/*
	 * setup work_in
	 */
	for (f = 0; f < sizeof(work_in)/sizeof(work_in[0]); f++)
		work_in[f] = 0;

	for (f = 0; f < 10; f++)
		work_in[f] = 1;

	work_in[10] = 2;


	if (cfg.auto_program)
	{
#if 0
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
				s_system(S_GETCOOKIE, 0x5f56444fL, (long)&lcfg.falcon);
				/* ´SCPN´ - Screenblaster present? */
				helper = s_system(S_GETCOOKIE, 0x5343504e, 0L);
				helper = (helper == -1) ? 0 : 1;

				if (lcfg.falcon == 0x00030000L)
				{
					work_out[45] = lcfg.modecode;
					/* Ronald Andersson: This should be the method for falcon. */
					work_in[0] = 5;
					DIAGS(("Falcon: mode %d(%x)", lcfg.modecode, lcfg.modecode));
				}
				else
				{
					DIAGS(("-fvideo: No, or incorrect _VDO cookie: %lx", lcfg.falcon));
				}

			}
			else
			{
				/* Video mode switch */
				if (stricmp("-video", argv[1]) == 0)
				{
					work_in[0] = lcfg.modecode;
					DIAGS(("Standard: mode %d(%x)", lcfg.modecode, lcfg.modecode));
				}
				else
				{
					DIAGS(("Invalid agument: '%s'", argv[1]));
				}
			}
		}
		else
#endif
		{
			DIAGS(("Default screenmode."));
		}

		v_opnwk(work_in, &(C.P_handle), work_out);
		DIAGS(("Physical work station opened: %d", C.P_handle));

		if (C.P_handle == 0)
		{
			DIAGS(("v_opnwk failed (%i)!", C.P_handle));
			return -1;
		}

		/* We need to get rid of the cursor */
		v_exit_cur(C.P_handle);
		DIAGS(("v_exit_cur ok!"));
	}
	else
	{
		/* The GEM AES has already been started,
		 * so get the physical workstation handle from it
		 */
		short junk;
		C.P_handle = mt_graf_handle(&junk, &junk, &junk, &junk, my_global_aes);
		DIAGS(("graf_handle -> %d", C.P_handle));
	}

	/* Open us a virtual workstation for XaAES to play with */
	C.vh = C.P_handle;
	v_opnvwk(work_in, &C.vh, work_out);

	if (C.vh == 0)
	{
		DIAGS(("v_opnvwk failed (%i)!", C.vh));
		return -1;
	}

	vsf_perimeter(C.vh, 0);		/* from set_colours; never set to 1 ever */

	graf_mouse(ARROW, 0);
	v_hide_c(C.vh);
	wr_mode(MD_TRANS); /* We run in TRANSPARENT mode for all AES ops (unless otherwise requested) */
	vst_alignment(C.vh, 0, 5, &f, &f); /* YESss !!! */

	DIAGS(("Virtual work station opened: %d", C.vh));

	/* Setup the screen parameters */
	screen.r.x = screen.r.y = 0;
	screen.r.w = work_out[0] + 1;	/* HR + 1 */
	screen.r.h = work_out[1] + 1;	/* HR + 1 */
	clear_clip();
	screen.colours = work_out[13];
	screen.display_type = D_LOCAL;

	screen.dial_colours =
		MONO ? bw_default_colours : default_colours;

	vq_extnd(C.vh, 1, work_out);	/* Get extended information */
	screen.planes = work_out[4];	/* number of planes in the screen */

	DIAGS(("Video info: width(%d/%d), planes :%d, colours %d",
		screen.r.w, screen.r.h, screen.planes, screen.colours));

	/* If we are using anything apart from the system font for windows,
	 * better check for GDOS and load the fonts.
	 */
	if (cfg.font_id != 1)
	{
		if (vq_gdos())	/* Yeah, I know, this is assuming the old-style vq_gdos() binding */
			vst_load_fonts(C.vh, 0);
		else
			cfg.font_id = 1;
	}

	/* Set standard AES font */
	screen.standard_font_id = screen.small_font_id =
		vst_font(C.vh, cfg.font_id);

	/*
	 * Use the ability of vst_point to return the character cell measures.
	 */

	/* Select Small font */
	screen.small_font_point = vst_point(C.vh,
					cfg.small_font_point,
					&f,
					&screen.small_font_height,
					&screen.c_min_w,
					&screen.c_min_h);

	/* Select standard font */
	screen.standard_font_point = vst_point(C.vh,
 		(screen.r.h <= 280) ? cfg.medium_font_point : cfg.standard_font_point,
		&f,
		&screen.standard_font_height,
		&screen.c_max_w,
		&screen.c_max_h);

	/* Open a diagnostics file? All console output can be considered diagnostics,
	 * so I just redirect the console to the required file/device
	 */
	DIAGS(("Display Device: Phys_handle=%d, Virt_handle=%d", C.P_handle, C.vh));
	DIAGS((" size=[%d,%d], colours=%d, bitplanes=%d", screen.r.w, screen.r.h, screen.colours, screen.planes));

	/* Load the system resource files */
	resource_name = xa_find(lcfg.rsc_name);
	if (resource_name)
	{
		C.Aes_rsc = LoadResources(C.Aes, resource_name, 0, DU_RSX_CONV, DU_RSY_CONV);
		fdisplay(log, "system resource = %lx (%s)",
			 C.Aes_rsc, lcfg.rsc_name);
	}	
	if (!resource_name || !C.Aes_rsc)
	{
		fdisplay(log, "ERROR: Can't find/load system resource file '%s'", lcfg.rsc_name);
		return -1;
	}

	widget_resources = NULL;
	resource_name = xa_find(lcfg.widg_name);
	if (resource_name)
	{
		widget_resources = LoadResources(C.Aes, resource_name, 0, DU_RSX_CONV, DU_RSY_CONV);
		fdisplay(log, "widget_resources = %lx (%s)",
			 widget_resources, lcfg.widg_name);
	}
	if (!resource_name || !widget_resources)
	{
		fdisplay(log, "ERROR: Can't find/load widget resource file '%s'", lcfg.widg_name);
		return -1;
	}

	/* get widget object parameters. */
	{
		RECT c;
		OBJECT *tree = ResourceTree(widget_resources, 0);
		object_area(&c, tree, 1, 0, 0);
		cfg.widg_w = c.w;
		cfg.widg_h = c.h;
		cfg.widg_dw = (tree[1].ob_width - c.w)/2;
		cfg.widg_dh = (tree[1].ob_height - c.h)/2;

		fdisplay(log, "cfg.widg: %d/%d   %d/%d", cfg.widg_w, cfg.widg_h, cfg.widg_dw, cfg.widg_dh);
	}

#if FILESELECTOR
	/* Do some itialisation */
	init_fsel();
#endif

	/* Create the root (desktop) window
	 * - We don't want messages from it, so make it a NO_MESSAGES window
	 */
	DIAGS(("creating root window"));
	root_window = create_window(
				NOLOCKING,
				0,			/* No messages */
				C.Aes,
				false,
				XaMENU,			/* menu standard widget */
				created_for_AES,
				-1,			/* -1, no frame, overscan window */
				false,false,
				screen.r,		/* current size */
				&screen.r,		/* maximum size (NB default would be rootwindow->wa) */
				0);			/* need no remembrance */

	/* Tack a menu onto the root_window widget */
	C.Aes->std_menu.tree = ResourceTree(C.Aes_rsc, SYSTEM_MENU);
	C.Aes->std_menu.owner = C.Aes;
	strncpy((struct xa_client *)&C.Aes->mnu_clientlistname, "  Clients \3", 14);
	fix_menu(C.Aes, C.Aes->std_menu.tree, true);
	set_menu_widget(root_window, &C.Aes->std_menu);
	{
		char *vs = get_ob_spec(C.Aes->std_menu.tree + SYS_DESK)->free_string;
		strcpy(vs + strlen(vs) - 3, version + 3);
	}
	DIAGS(("menu widget set"));

	/* Fix up the file selector menu */
	fix_menu(C.Aes, ResourceTree(C.Aes_rsc, FSEL_MENU), false);

	/* Fix up the window widget bitmaps and any others we might be using
   	 * (Calls vr_trnfm() for all the bitmaps)
   	 * HR: No, it doesnt! ;-)
   	 */
	DIAGS(("fixing up widgets"));
	fix_default_widgets(widget_resources);

	/* Set a default desktop */
	DIAGS(("setting default desktop"));
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

	DIAGS(("setting up task manager"));
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, TASK_MANAGER), TM_LIST);

	DIAGS(("setting up file selector"));
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, FILE_SELECT), FS_LIST);

	DIAGS(("setting up System Alert log"));
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, SYS_ERROR), SYSALERT_LIST);

	DIAGS(("setting up About text list"));
	set_scroll(C.Aes, ResourceTree(C.Aes_rsc, ABOUT_XAAES), ABOUT_LIST);

	DIAGS(("display root window"));
	open_window(NOLOCKING, root_window, screen.r);

	/* Initial iconified window coords */
	C.iconify = iconify_grid(0);

	v_show_c(C.vh, 0); /* 0 = reset */		

	if (cfg.opentaskman)
		open_taskmanager(NOLOCKING);

	return 0;
}

static const char *dont_load_list[] =
{
};

static int
dont_load(const char *name)
{
	int i;

	for (i = 0; i < (sizeof(dont_load_list) / sizeof(*dont_load_list)); i++)
		if (stricmp(dont_load_list[i], name) == 0)
			return 1;

	return 0;
}

void
load_accs(void)
{
	char buf[256];
	struct dirstruct dirh;
	long len;
	char *name;
	long r;

	strcpy(buf, cfg.acc_path);
	len = strlen(buf);

	buf[len++] = '\\';
	buf[len] = '\0';

	name = buf + len;
	len = sizeof(buf) - len;

	DEBUG(("load_accs: enter (%s)", buf));

	r = kernel_opendir(&dirh, buf);
	if (r == 0)
	{
		r = kernel_readdir(&dirh, name, len);
		while (r == 0)
		{
			r = strlen (name+4) - 4;
			if ((r > 0) &&
			    stricmp(name+4 + r, ".acc") == 0 &&
			    !dont_load(name+4))
			{
				char *ptr1 = name;
				char *ptr2 = name+4;
				long len2 = len - 1;

				while (*ptr2)
				{
					*ptr1++ = *ptr2++;
					len2--; assert(len2);
				}

				*ptr1 = '\0';

				DEBUG(("load_accs: launch (%s)", buf));
				launch(NOLOCKING, 3, 0, 0, buf, "", C.Aes);
			}

			r = kernel_readdir(&dirh, name, len);
		}

		kernel_closedir(&dirh);
	}
}
