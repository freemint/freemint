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
#include "adiload.h"
#include "debug.h"

#include "k_init.h"
#include "xa_global.h"


#include "c_window.h"
#include "desktop.h"
#include "taskman.h"
#include "widgets.h"

#include "xa_appl.h"
#include "xa_fsel.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

#include "obtree.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "trnfm.h"

#include "win_draw.h"
#include "render_obj.h"

/* kernel header */
#include "mint/ssystem.h"
#include "cookie.h"

#include "xa_api.h"

extern struct xa_api xa_api;

/*
 * check if file exist in Aes_home_path
 */
char * _cdecl
xaaes_sysfile(const char *file)
{
	char *fn;
	long err;
	struct file *fp;
	long len = strlen(C.Aes->home_path) + strlen(file) + 2;

	fn = kmalloc(len);

	if (fn)
	{
		strcpy(fn, C.Aes->home_path);
		strcat(fn, file);
		fp = kernel_open(fn, O_RDONLY, &err, NULL);
		BLOG((false, "xaaes_sysfile: check if '%s' exists - return = %li", fn, err));
		if (fp)
		{
			kernel_close(fp);
			return fn;
		}
		kfree(fn);
	}
	return NULL;
}

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
extern void *xobj_rshdr;
extern void *xobj_rsc;

static struct xa_colour_scheme default_colours = { G_LWHITE, G_BLACK, G_LBLACK, G_WHITE, G_BLACK, G_CYAN };
static struct xa_colour_scheme bw_default_colours = { G_WHITE, G_BLACK, G_BLACK, G_WHITE, G_BLACK, G_WHITE };

static void
set_wrkin(short *in, short dev)
{
	int i;

	in[0] = dev;
	for (i = 1; i < 10; i++)
		in[i] = 1;
	in[10] = 2;
	for (i = 11; i < 16; i++)
		in[i] = 0;
}

static void
calc_average_fontsize(struct xa_vdi_settings *v, short *maxw, short *maxh, short *dist)
{
	short i, j, count = 0, cellw, tmp;
	short temp[8];
	unsigned long wch = 0;

	for (i = 0; i < 256; i++)
	{
		j = vqt_width(v->handle, i, &cellw, &tmp, &tmp);
		if (j >= 0)
		{
			count++;
			wch += cellw;
		}
// 		display("i=%d, j=%d, count=%d, cellw=%d, totalw=%ld", i, j, count, cellw, wch);
	}
	if (count)
	{
		*maxw = (wch / count);
	}
	(*v->api->t_extent)(v, "A_", &cellw, &tmp);
	*maxh = tmp;

	vqt_fontinfo(v->handle, &i, &i, dist, &i, temp);

// 	display("w %d(%d), h %d", *maxw, tmp, *maxh);
// 	display("dists %d, %d, %d, %d, %d - %d, %d", dist[0], dist[1], dist[2], dist[3], dist[4]);
// 	display("effex %d, %d, %d", temp[0], temp[1], temp[2]);
}

int
k_init(unsigned long vm)
{
	short work_in[16];
	short work_out[58];
	char *resource_name;
	struct xa_vdi_settings *v = &global_vdi_settings;

// 	display("\033H");		/* cursor home */

	{
		short f, *t;

		for (t = (short *)v, f = 0; f < (sizeof(*v) >> 1); f++)
			*t++ = -1;
	}
	/*
	 * Load AES modules
	 */
	xam_load(true);

	cfg.videomode = (short)vm;

	xa_vdiapi = v->api = init_xavdi_module();
	{
		short *p;

		if (!(s_system(S_GETCOOKIE, COOKIE_NVDI, (unsigned long)&p)))
		{
			C.nvdi_version = *p;
			BLOG((false, "nvdi version = %x", C.nvdi_version));
		}
#if BOOTLOG
		else
			BLOG((false, "could not determine nvdi version"));
#endif
	}
	{
		short mode = 1;

		if (C.set_resolution)
			(*C.set_resolution)(vm, work_in, work_out, &mode);
#if 0
		short mode = 1;
		long vdo, r;

		r = s_system(S_GETCOOKIE, COOKIE__VDO, (unsigned long)(&vdo));
		if (r != 0)
			vdo = 0;

		if (cfg.videomode)
		{
#ifndef ST_ONLY
			if ((vm & 0x80000000) && mvdi_api.dispatch)
			{
// 				long ret;
				/* Ozk:  Resolution Change on the Milan;
				 *
				 * I'm guessing like never before here; I found out that one can select
				 * the resolution by stuffing the device ID in work_out[45] and then
				 * open VDI device 5, just like we do it for the Falcon. However, this
				 * often caused the Milan VDI to freeze up real good. Then I found out
				 * that on _my_ Milan, vcheckmode() not only checks the validity of the
				 * passed device ID, it actually sets this mode to be the one to use
				 * when one opens physical workstation 1! And it works every time.
				 * We could use vsetmode() too, but that sets the resolution immediately,
				 * a thing I didnt like much.
				 * Hopefully this will work on all Milans!
				 * Update: Ofcourse this didnt work on all Milans!
				 */

				/*
				 * First try...
				 */
// 				vcheckmode(vm & 0xffff);	/* Works on my Milan - 040 w/s3 trio */
								/* Didnt work on Vido's Milan - 060 w/rage. Didnt change res*/
				/*
				 * Second try...		 * Works on my Milan - 040 w/s3 trio
				 *				 * Didnt work on Vido's Milan - 060 w/rage. System freeze!
				 */
// 				vsetmode(vm & 0xffff);

				/*
				 * Third try...			 * Works with some resolutio on my Milan - VERY unstable. Whe
				 * This is the same as		 * it freezes, it freezes so good I have to use reset to recover
				 * on the Falcon		 * Not tested on Vido's 060 w/rage Milan
				 */
// 				mode = 5;
// 				work_out[45] = vm & 0xffffL;

				/*
				 * Fourth try...		* This works perfect on _my_ milan. Dont know how it works on
				 *				* other machines yet... didnt work with nvdi 5.03 installed!
				 */
// 				mvdi_device(vm & 0x0000ffff, 0L, DEVICE_SETDEVICE, (long *)&ret);

				/*
				 * Fifth try...
				 * this is the same method as used on the Falcon,
				 * only using devid 7 instead of 5.
				 */
				mode = 7;
				work_out[45] = vcheckmode(vm & 0x0000ffff); //vm & 0x0000ffff;
			}
			else if ((vm & 0x80000000) && nova_data && nova_data->valid)
#endif
#ifdef ST_ONLY
			if (((vm & 0x80000000) && nova_data && nova_data->valid))
#endif
			{
				if (nova_data->valid)
					nova_data->xcb->resolution = cfg.videomode;
// 				display("nova change res to %d - %s", cfg.videomode, nova_data->next_res.name);
				nova_data->valid = false;
				mode = 1;
			}
#ifndef ST_ONLY
			else if (vdo == 0x00030000L)
			{
				short nvmode;

				BLOG((false, "Falcon video: mode %d(%x)", cfg.videomode, cfg.videomode));

				/* Ronald Andersson:
				 * This should be the method for falcon!
				 */
				nvmode = vcheckmode(cfg.videomode);
				if ((nvmode & (1 << 4)) &&	/* VGA_FALCON */
				    (nvmode & 7) == 4)		/* is 16bit */
				{
					nvmode &= ~(1 << 3);		/* Set 320 pixels */
				}

				work_out[45] = nvmode;
				mode = 5;
// 				display("Falcon video: mode %x, %x", cfg.videomode, nvmode);
			}
			else
#endif
			{
				if (cfg.videomode >= 1 && cfg.videomode <= 10)
				{
					mode = cfg.videomode;
				}
				else
				{
					BLOG((false, "videomode %d invalid", cfg.videomode));
					BLOG((false, "must be between 1 and 10"));
				}
			}
		}
		else
		{
			BLOG((false, "Default screenmode"));
		}
#endif
// 		display("set mode %x", mode);
		BLOG((false, "Screenmode is: %d", mode));

#ifndef ST_ONLY
		/*
		 * Ozk: We switch off instruction, data and branch caches (where available)
		 *	while the VDI accesses the hardware. This fixes 'black-screen'
		 *	problems on Hades with Nova VDI.
		 *	Goddamned! The fact that caches needs to be turned off during
		 *	opening the physical on a ct60/3 kept me puzzled for hours!
		 *	Just could not make the thing go into mono mode, and had me
		 *	scared shitless for a long time. Therefore we let this code
		 *	stay enabled at all times!
		 */
		{
			unsigned long sc = 0, cm = 0;

			cm = s_system(S_CTRLCACHE, 0L, -1L);
			sc = s_system(S_CTRLCACHE, -1L, 0L);
			s_system(S_CTRLCACHE, sc & ~3, cm);
			set_wrkin(work_in, mode);
			v_opnwk(work_in, &(C.P_handle), work_out);
			s_system(S_CTRLCACHE, sc, cm);
		}
#else
		set_wrkin(work_in, mode);
		v_opnwk(work_in, &(C.P_handle), work_out);
#endif
		BLOG((false, "Physical work station opened: %d", C.P_handle));

		if (C.P_handle == 0)
		{
			BLOG((/* 00000001 */true, "v_opnwk failed (%i)!", C.P_handle));
			return -1;
		}
// 		_b_ubconout(2, ' ');
		/*
		 * We need to get rid of the cursor
		 */
		v_exit_cur(C.P_handle);
		BLOG((false, "v_exit_cur ok!"));
	}

	get_syspalette(C.P_handle, screen.palette);
// 	set_defaultpalette(C.P_handle);
	/*
	 * Open us a virtual workstation for XaAES to play with
	 */
	v->handle = C.P_handle;
	set_wrkin(work_in, 1);
	v_opnvwk(work_in, &v->handle, work_out);

	if (v->handle == 0)
	{
		BLOG((true, "v_opnvwk failed (%i)!", v->handle));
		return -1;
	}
	BLOG((false, "Virtual work station opened: %d", v->handle));

	/*
	 * Setup the screen parameters
	 */
	screen.r.x = screen.r.y = 0;
	screen.r.w = work_out[0] + 1;
	screen.r.h = work_out[1] + 1;
	screen.colours = work_out[13];
	screen.display_type = D_LOCAL;
	v->screen = screen.r;
	C.Aes->vdi_settings = v;
	vs_clip(C.P_handle, 1, (short *)&screen.r);
	(*v->api->set_clip)(v, &screen.r);

	(*v->api->f_perimeter)(v, 0);

// 	v_show_c(C.P_handle, 0);
// 	hidem();
// 	xa_graf_mouse(ARROW, NULL, NULL, false);
// 	showm();

	objc_rend.dial_colours = MONO ? bw_default_colours : default_colours;

	vq_extnd(v->handle, 1, work_out);	/* Get extended information */
	screen.planes = work_out[4];		/* number of planes in the screen */

// 	if (screen.planes > 8)
// 		set_defaultpalette(v->handle);
// 	get_syspalette(C.P_handle, screen.palette);

	screen.pixel_fmt = detect_pixel_format(v);

	BLOG((false, "Video info: width(%d/%d), planes :%d, colours %d",
		screen.r.w, screen.r.h, screen.planes, screen.colours));

// 	display("Video info: width(%d/%d), planes :%d, colours %d, pixelfmt = %d",
// 		screen.r.w, screen.r.h, screen.planes, screen.colours, screen.pixel_fmt);


	/*
	 * If we are using anything apart from the system font for windows,
	 * better check for GDOS and load the fonts.
	 */
	if ((C.gdos_version = vq_gdos()))
		(*v->api->load_fonts)(v);
	else
		cfg.font_id = 1;

	(*v->api->t_alignment)(v, 0, 5);
	(*v->api->t_font)(v, cfg.small_font_point, cfg.font_id);
	screen.standard_font_id  = screen.small_font_id = v->font_rid;
	screen.small_font_height = v->font_h;
	screen.small_font_point  = v->font_rsize;
	screen.c_min_w = v->cell_w;
	screen.c_min_h = v->cell_h;
	calc_average_fontsize(v, &screen.c_min_w, &screen.c_min_h, &screen.c_min_dist[0]);
	(*v->api->t_font)(v, (screen.r.h <= 280) ? (cfg.standard_font_point = cfg.medium_font_point) : cfg.standard_font_point, -1);
	screen.standard_font_height = v->font_h;
	screen.standard_font_point  = v->font_rsize;
	screen.c_max_w = v->cell_w;
	screen.c_max_h = v->cell_h;
	calc_average_fontsize(v, &screen.c_max_w, &screen.c_max_h, &screen.c_max_dist[0]);

	BLOG((false, "stdfont: id = %d, size = %d, cw=%d, ch=%d",
 		screen.standard_font_id, screen.standard_font_point, screen.c_max_w, screen.c_max_h));
	BLOG((false, "smlfont: id = %d, size = %d, cw=%d, ch=%d",
		screen.small_font_id, screen.small_font_point, screen.c_min_w, screen.c_min_h));

	/*
	 * Init certain things in the info_tab used by appl_getinfo()
	 */
	init_apgi_infotab();

	/*
	 * Open a diagnostics file? All console output can be considered diagnostics,
	 * so I just redirect the console to the required file/device
	 */
	BLOG((false, "Display Device: Phys_handle=%d, Virt_handle=%d", C.P_handle, v->handle));
	BLOG((false, " size=[%d,%d], colours=%d, bitplanes=%d", screen.r.w, screen.r.h, screen.colours, screen.planes));


// 	get_syspalette(C.P_handle, screen.palette);

	/* Load the system resource files */
	BLOG((false, "Loading system resource file '%s'", cfg.rsc_name));
	if (!(resource_name = xaaes_sysfile(cfg.rsc_name)))
	{
		display(/* 00000002 */"ERROR: Can't find AESSYS resource file '%s'", cfg.rsc_name);
		return -1;
	}
	else
	{
		C.Aes_rsc = LoadResources(C.Aes,
					  resource_name,
					  NULL,
					  DU_RSX_CONV, // screen.c_max_w, // < 8 ? 8 : screen.c_max_w,
					  DU_RSY_CONV, //screen.c_max_h, // < 16 ? 16 : screen.c_max_h); //DU_RSX_CONV, DU_RSY_CONV);
					  true);
		kfree(resource_name);
		BLOG((false, "system resource = %lx (%s)", C.Aes_rsc, cfg.rsc_name));
	}
	if (!C.Aes_rsc)
	{
		display(/*00000003*/"ERROR: Can't load system resource file '%s'", cfg.rsc_name);
		return -1;
	}
// 	set_syspalette(C.P_handle, screen.palette);
// 	set_syscolor();

	/*
	 * Version check the aessys resouce
	 */
	{
		OBJECT *about = ResourceTree(C.Aes_rsc, ABOUT_XAAES);

		if ((ob_count_objs(about, 0, -1) < RSC_VERSION)   ||
		     about[RSC_VERSION].ob_type != G_TEXT     ||
		    (strcmp(object_get_tedinfo(about + RSC_VERSION, NULL)->te_ptext, "0.0.8")))
		{
			display(/*00000004*/"ERROR: Outdated AESSYS resource file (%s) - update to recent version!", cfg.rsc_name);
// 			display("       also make sure you read CHANGES.txt!!");
			return -1;
		}
	}
	/*
	 *  ---------        prepare the window widgets renderer module  --------------
	 */

	main_xa_theme(&C.Aes->xmwt);
	main_object_render(&C.Aes->objcr_module);

	BLOG((false, "Attempt to open object render module..."));
	if (!(*C.Aes->objcr_module->init_module)(&xa_api, &screen, cfg.gradients))
	{
		BLOG((false, "object render returned NULL"));
		return -1;
	}
	BLOG((false, "Attempt to open new object theme..."));
	if (init_client_objcrend(C.Aes))
	{	BLOG((false, "Opening object theme failed"));
		return -1;
	}

	BLOG((false, "Attempt to open window widget renderer..."));
	if (!(C.Aes->wtheme_handle = (*C.Aes->xmwt->init_module)(&xa_api, &screen, (char *)&cfg.widg_name, cfg.gradients)))
	{
		display("Window widget module returned NULL");
		return -1;
	}

	/*
	 *  ---------        prepare the AES object renderer module  --------------
	 */

	/*
	 * Setup default widget theme
	 */
	BLOG((false, "Setting up default window widget theme..."));
	init_client_widget_theme(C.Aes);

#if FILESELECTOR
	/* Do some itialisation */
	init_fsel();
#endif
	init_client_mdbuff(C.Aes);		/* In xa_appl.c */

	/* Create the root (desktop) window
	 * - We don't want messages from it, so make it a NO_MESSAGES window
	 */
	BLOG((false, "creating root window"));
// 	display("creating root window");
	root_window = create_window(
				NOLOCKING,
				NULL, //do_winmesag, //0,			/* No messages */
				NULL, //do_rootwind_msg,			/* No 'doer' */
				C.Aes,
				false,
				XaMENU,			/* menu standard widget */
				created_for_AES,
				-1, false, /*false,false,*/
				screen.r,		/* current size */
				&screen.r,		/* maximum size (NB default would be rootwindow->wa) */
				0);			/* need no remembrance */

// 	display("Fixing up root menu");
	/* Tack a menu onto the root_window widget */
	C.Aes->std_menu = new_widget_tree(C.Aes, ResourceTree(C.Aes_rsc, SYSTEM_MENU));
	assert(C.Aes->std_menu);
	wt_menu_area(C.Aes->std_menu);
	set_rootmenu_area(C.Aes);

	C.Aes->mnu_clientlistname = kmalloc(strlen(mnu_clientlistname)+1);
	assert(C.Aes->mnu_clientlistname);
	strcpy(C.Aes->mnu_clientlistname, mnu_clientlistname);
	fix_menu(C.Aes, C.Aes->std_menu, root_window, true);
	set_menu_widget(root_window, C.Aes, C.Aes->std_menu);

	/* Set a default desktop */
	BLOG((false, "setting default desktop"));
	{
		OBJECT *ob = get_xa_desktop();
		*(RECT*)&ob->ob_x = root_window->r;
		(ob + DESKTOP_LOGO)->ob_x = (root_window->wa.w - (ob + DESKTOP_LOGO)->ob_width) / 2;
		(ob + DESKTOP_LOGO)->ob_y = (root_window->wa.h - (ob + DESKTOP_LOGO)->ob_height) / 2;
		C.Aes->desktop = new_widget_tree(C.Aes, ob);

		set_desktop_widget(root_window, C.Aes->desktop);
		set_desktop(C.Aes, false);
	}
 	BLOG((false, "really opening now..."));
 	BLOG((false, "opening root window"));
	open_window(NOLOCKING, root_window, screen.r);
	S.open_windows.root = root_window;
	BLOG((false, "..root window opened"));
	/* Initial iconified window coords */
	C.iconify = iconify_grid(0);
	BLOG((false, "initialized iconify grid"));
// 	v_show_c(v->handle, 1); /* 0 = reset */

// 	display("Open taskman -- perhaps");
// 	if (cfg.opentaskman)
// 		open_taskmanager(NOLOCKING);

// 	display("redrawing menu");
	redraw_menu(NOLOCKING);
	BLOG((false, "Menu redrawn"));
// 	display("all fine - return 0");

	/*
	 * Setup mn_set for menu_settings()
	 */
	cfg.menu_settings.mn_set.display = cfg.popup_timeout;
	cfg.menu_settings.mn_set.drag = cfg.popout_timeout;
	cfg.menu_settings.mn_set.delay = 250;
	cfg.menu_settings.mn_set.speed = 0;
	cfg.menu_settings.mn_set.height = root_window->wa.h / screen.c_max_h;
	BLOG((false, "mn_set for menu_settings setup"));
	v_show_c(C.P_handle, 0);
	BLOG((false, "show mouse, return sucess"));
	return 0;
}

void
init_helpthread(enum locks lock, struct xa_client *client)
{
	BLOG((false, "init_helpthread"));
	open_taskmanager(0, client, 0);
	BLOG((false, "init_helpthread done"));
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

static char accpath[] = "ACCPATH=";

void
load_accs(void)
{
	char buf[256];
	struct dirstruct dirh;
	long len;
	char *name;
	const char *env;
	long r;

	if ((env = get_env(0, accpath)))
		strcpy(buf, env);
	else
		strcpy(buf, cfg.acc_path);

	len = strlen(buf);

	if (len)
	{
		int i = 0;

		while (buf[i])
		{
			if (buf[i] == '/')
				buf[i] = '\\';
			i++;
		}
		if (buf[len - 1] != '\\')
		{
			buf[len++] = '\\';
			buf[len] = '\0';
		}
	}

	else if (!len)
	{
		strcpy(buf, "c:\\");
		len = strlen(buf);
	}

	if (!env)
	{
		char nenv[256];
		strcpy(nenv, accpath);
		strcpy(nenv + strlen(accpath), buf);
		put_env(0, nenv);
		BLOG((false, "ACCPATH= not existing, creating %s", nenv));
	}
	strcpy(cfg.acc_path, buf);

	set_drive_and_path(cfg.acc_path);

	BLOG((false, "loading accessories from '%s'", buf));

	name = buf + len;
	len = sizeof(buf) - len;

	BLOG((false, "load_accs: enter (%s)", buf));

	r = kernel_opendir(&dirh, buf);
	if (r == 0)
	{
		r = kernel_readdir(&dirh, name, len);
		while (r == 0)
		{
			r = strlen (name + 4) - 4;
			if ((r > 0) &&
			    stricmp(name + 4 + r, ".acc") == 0 &&
			    !dont_load(name + 4))
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

				BLOG((false, "load_accs: launch (%s)", buf));
				launch(NOLOCKING, 3, 0, 0, buf, "", C.Aes);
			}

			r = kernel_readdir(&dirh, name, len);
		}
		kernel_closedir(&dirh);
	}
}
