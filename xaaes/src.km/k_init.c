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

#include "k_init.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "desktop.h"
#include "init.h"
#include "nkcc.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"

#include "xa_appl.h"
#include "xa_evnt.h"
#include "xa_form.h"
#include "xa_fsel.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/signal.h"

#include "obtree.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "xa_graf.h"
#include "trnfm.h"

#include "win_draw.h"

#include "xa_xtobj.h"

#include "mvdi.h"

/* kernel header */
#include "mint/ssystem.h"
#include "cookie.h"


static struct xa_module_api xam_api;

// static char *xaaes_sysfile(const char *);

static char *_cdecl
api_xaaes_sysfile(const char *f)
{
	return xaaes_sysfile(f);
}

static RSHDR * _cdecl
api_load_resource(char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal)
{
	return LoadResources(C.Aes, fname, rshdr, designWidth, designHeight, set_pal);
}

static OBJECT * _cdecl
api_resourcetree(RSHDR *tree, long index)
{
	return ResourceTree(tree, index);
}

static struct widget_tree * _cdecl
api_new_widget_tree(OBJECT *obtree)
{
	struct widget_tree *wt = new_widget_tree(C.Aes, obtree);

	if (wt)
	{
		wt->flags |= WTF_AUTOFREE;
	}
	return wt;
}

static struct widget_tree * _cdecl
api_obtree_to_wt(OBJECT *obtree)
{
	return obtree_to_wt(C.Aes, obtree);
}

static void _cdecl
api_init_widget_tree(struct widget_tree *wt, OBJECT *obtree)
{
	init_widget_tree(C.Aes, wt, obtree);
	wt->flags |= WTF_AUTOFREE;
}

static void _cdecl
api_remove_wt(struct widget_tree *wt)
{
	remove_wt(wt, false);
}

static void _cdecl
api_ob_spec_xywh(OBJECT *obtree, short obj, RECT *r)
{
	ob_spec_xywh(obtree, obj, r);
}

static void _cdecl 
api_object_spec_wh(OBJECT *ob, short *w, short *h)
{
	object_spec_wh(ob, w, h);
}

static void * _cdecl
api_rp2ap(struct xa_window *wind, struct xa_widget *widg, RECT *r)
{
	return rp_2_ap(wind, widg, r);
}
static void _cdecl
api_rp2apcs(struct xa_window *wind, struct xa_widget *widg, RECT *r)
{
	rp_2_ap_cs(wind, widg, r);
}

static void * _cdecl
api_kmalloc(long size)
{
	return kmalloc(size);
}

static void * _cdecl
api_umalloc(long size)
{
	return umalloc(size);
}

static void _cdecl
api_kfree(void *addr)
{
	kfree(addr);
}

static void _cdecl
api_ufree(void *addr)
{
	ufree(addr);
}

static void _cdecl
api_bzero(void *addr, unsigned long len)
{
	bzero(addr, len);
}

static void * _cdecl
api_lookup_xa_data(struct xa_data_hdr **l, void *data)
{
	return lookup_xa_data(l, data);
}

static void _cdecl
api_add_xa_data(struct xa_data_hdr **list, void *data, char *name, void _cdecl(*destruct)(void *d))
{
	add_xa_data(list, data, name, destruct);
}
static void _cdecl
api_remove_xa_data(struct xa_data_hdr **list, void *data)
{
	remove_xa_data(list, data);
}
static void _cdecl
api_delete_xa_data(struct xa_data_hdr **list, void *data)
{
	delete_xa_data(list, data);
}

static void _cdecl
api_free_xa_data_list(struct xa_data_hdr **list)
{
	free_xa_data_list(list);
}

static void _cdecl
api_load_img(char *fname, MFDB *img)
{
	load_image(fname, img);
}	

static void
setup_xa_module_api(void)
{
	xam_api.sysfile		= api_xaaes_sysfile;
	xam_api.load_resource	= api_load_resource;
	xam_api.resource_tree	= api_resourcetree;

	xam_api.init_wt		= api_init_widget_tree;
	xam_api.new_wt		= api_new_widget_tree;
	xam_api.obtree_to_wt	= api_obtree_to_wt;
	xam_api.remove_wt	= api_remove_wt;

	xam_api.ob_spec_xywh	= api_ob_spec_xywh;
	xam_api.object_spec_wh	= api_object_spec_wh;

	xam_api.rp2ap		= api_rp2ap;
	xam_api.rp2apcs		= api_rp2apcs;

	xam_api.kmalloc		= api_kmalloc;
	xam_api.umalloc		= api_umalloc;
	xam_api.kfree		= api_kfree;
	xam_api.ufree		= api_ufree;
	xam_api.bclear		= api_bzero;

	xam_api.lookup_xa_data	= api_lookup_xa_data;
	xam_api.add_xa_data	= api_add_xa_data;
	xam_api.remove_xa_data	= api_remove_xa_data;
	xam_api.delete_xa_data	= api_delete_xa_data;
	xam_api.free_xa_data_list = api_free_xa_data_list;

	xam_api.load_img	= api_load_img;
}

/*
 * check if file exist in Aes_home_path
 */
char *
xaaes_sysfile(const char *file)
{
	static char *fn;
	struct file *fp;
	long len = strlen(C.Aes->home_path) + strlen(file) + 2;

	fn = kmalloc(len);

	if (fn)
	{
		strcpy(fn, C.Aes->home_path);
		strcat(fn, file);
		fp = kernel_open(fn, O_RDONLY, NULL, NULL);
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
//static void *widget_resources = NULL;
//static void *xobj_rsc = NULL;
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
#if 0
	*in++ = dev;

	for (i = 1; i < 10; *in++ = 1, i++)
		;

	*in++ = 2;

	for (i = 0; i < (16 - 11); *in++ = 0, i++)
		;
#endif
}

int
k_init(unsigned long vm)
{
	short work_in[16];
	short work_out[58];
	short f;
	char *resource_name;
	struct xa_vdi_settings *v = &global_vdi_settings;
	
	display("\033H");		/* cursor home */
	
	for (f = 0; f < (sizeof(*v) >> 1); f++)
		((short *)v)[f] = -1;

	setup_xa_module_api();

	cfg.videomode = (short)vm;

	global_vdiapi = v->api = init_xavdi_module();	
	{
		short *p;

		if (!(s_system(S_GETCOOKIE, COOKIE_NVDI, (unsigned long)&p)))
		{
			C.nvdi_version = *p;
			DIAGS(("nvdi version = %x", C.nvdi_version));
		}
#if GENERATE_DIAGS
		else
			DIAGS(("could not determine nvdi version"));
#endif
	}

	if (cfg.auto_program)
	{
		short mode = 1;
		long vdo, r;

		r = s_system(S_GETCOOKIE, COOKIE__VDO, (unsigned long)(&vdo));
		if (r != 0)
			vdo = 0;

		if (cfg.videomode)
		{
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
			{
				if (nova_data->valid)
					nova_data->xcb->resolution = cfg.videomode;
// 				display("nova change res to %d - %s", cfg.videomode, nova_data->next_res.name);
				nova_data->valid = false;
				mode = 1;
			}
			else if (vdo == 0x00030000L)
			{
				short nvmode;

				DIAGS(("Falcon video: mode %d(%x)", cfg.videomode, cfg.videomode));

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
			{
				if (cfg.videomode >= 1 && cfg.videomode <= 10)
				{
					mode = cfg.videomode;
				}
				else
				{
					DIAGS(("videomode %d invalid", cfg.videomode));
					DIAGS(("must be between 1 and 10"));
				}
			}
		}
		else
		{
			DIAGS(("Default screenmode"));
		}
// 		display("set mode %x", mode);
		DIAGS(("Screenmode is: %d", mode));


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
		
		DIAGS(("Physical work station opened: %d", C.P_handle));

		if (C.P_handle == 0)
		{
			DIAGS(("v_opnwk failed (%i)!", C.P_handle));
			display("v_opnwk failed (%i)!", C.P_handle);
			return -1;
		}
// 		_b_ubconout(2, ' ');
		/*
		 * We need to get rid of the cursor
		 */
		v_exit_cur(C.P_handle);
		DIAGS(("v_exit_cur ok!"));
		
	}
	else
	{
		/*
		 * The GEM AES has already been started,
		 * so get the physical workstation handle from it
		 */
		short junk;
		C.P_handle = mt_graf_handle(&junk, &junk, &junk, &junk, my_global_aes);
		DIAGS(("graf_handle -> %d", C.P_handle));
	}

	set_defaultpalette(C.P_handle);
	/*
	 * Open us a virtual workstation for XaAES to play with
	 */
	v->handle = C.P_handle;
	set_wrkin(work_in, 1);
	v_opnvwk(work_in, &v->handle, work_out);

	if (v->handle == 0)
	{
		DIAGS(("v_opnvwk failed (%i)!", v->handle));
		return -1;
	}
	DIAGS(("Virtual work station opened: %d", v->handle));

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
	(*global_vdiapi->set_clip)(v, &screen.r);
	
	(*global_vdiapi->f_perimeter)(v, 0);	/* from set_colours; never set to 1 ever */
	v_show_c(C.P_handle, 0);
	hidem();
	graf_mouse(ARROW, NULL, NULL, false);
	showm();
	(*global_vdiapi->t_alignment)(v, 0, 5);
	
	objc_rend.dial_colours =
		MONO ? bw_default_colours : default_colours;

	vq_extnd(v->handle, 1, work_out);	/* Get extended information */
	screen.planes = work_out[4];		/* number of planes in the screen */
	
	if (screen.planes > 8)
		set_defaultpalette(v->handle);
	get_syspalette(C.P_handle, screen.palette);

	screen.pixel_fmt = detect_pixel_format(v);

	DIAGS(("Video info: width(%d/%d), planes :%d, colours %d",
		screen.r.w, screen.r.h, screen.planes, screen.colours));

// 	display("Video info: width(%d/%d), planes :%d, colours %d, pixelfmt = %d",
// 		screen.r.w, screen.r.h, screen.planes, screen.colours, screen.pixel_fmt);

	
	/*
	 * If we are using anything apart from the system font for windows,
	 * better check for GDOS and load the fonts.
	 */
	if (vq_gdos())
		(*global_vdiapi->load_fonts)(v);
	else
		cfg.font_id = 1;

	(*global_vdiapi->t_font)(v, cfg.small_font_point, cfg.font_id);
	screen.standard_font_id  = screen.small_font_id = v->font_rid;
	screen.small_font_height = v->font_h;
	screen.small_font_point  = v->font_rsize;
	screen.c_min_w = v->cell_w;
	screen.c_min_h = v->cell_h;
	(*global_vdiapi->t_font)(v, (screen.r.h <= 280) ? (cfg.standard_font_point = cfg.medium_font_point) : cfg.standard_font_point, -1);
	screen.standard_font_height = v->font_h;
	screen.standard_font_point  = v->font_rsize;
	screen.c_max_w = v->cell_w;
	screen.c_max_h = v->cell_h;

// 	display("stdfont: id = %d, size = %d, cw=%d, ch=%d",
// 		screen.standard_font_id, screen.standard_font_point, screen.c_max_w, screen.c_max_h);
// 	display("smlfont: id = %d, size = %d, cw=%d, ch=%d",
// 		screen.small_font_id, screen.small_font_point, screen.c_min_w, screen.c_min_h);

	/*
	 * Init certain things in the info_tab used by appl_getinfo()
	 */
	init_apgi_infotab();

	/*
	 * Open a diagnostics file? All console output can be considered diagnostics,
	 * so I just redirect the console to the required file/device
	 */
	DIAGS(("Display Device: Phys_handle=%d, Virt_handle=%d", C.P_handle, v->handle));
	DIAGS((" size=[%d,%d], colours=%d, bitplanes=%d", screen.r.w, screen.r.h, screen.colours, screen.planes));

	
// 	get_syspalette(C.P_handle, screen.palette);

	/* Load the system resource files */
	if (!(resource_name = xaaes_sysfile(cfg.rsc_name)))
	{
		display("ERROR: Can't find AESSYS resource file '%s'", cfg.rsc_name);
		return -1;
	}
	else
	{
		C.Aes_rsc = LoadResources(C.Aes,
					  resource_name,
					  NULL,
					  screen.c_max_w, // < 8 ? 8 : screen.c_max_w,
					  screen.c_max_h, // < 16 ? 16 : screen.c_max_h); //DU_RSX_CONV, DU_RSY_CONV);
					  true);
		kfree(resource_name);
		DIAGS(("system resource = %lx (%s)", C.Aes_rsc, cfg.rsc_name));
	}	
	if (!C.Aes_rsc)
	{
		display("ERROR: Can't load system resource file '%s'", cfg.rsc_name);
		return -1;
	}
// 	set_syspalette(C.P_handle, screen.palette);
// 	set_syscolor();

	/*
	 * Version check the aessys resouce
	 */
	{
		OBJECT *about = ResourceTree(C.Aes_rsc, ABOUT_XAAES);
		
		if ((ob_count_objs(about, 0) < RSC_VERSION)   ||
		     about[RSC_VERSION].ob_type != G_TEXT     ||
		    (strcmp(object_get_tedinfo(about + RSC_VERSION)->te_ptext, "0.0.6")))
		{
			display("ERROR: Outdated AESSYS resource file (%s) - update to recent version!", cfg.rsc_name);
// 			display("       also make sure you read CHANGES.txt!!");
			return -1;
		}
	}
	/*
	 *  ---------        prepare the window widgets renderer module  --------------
	 */
	 
	main_xa_theme(&C.Aes->xmwt);

	if (!(C.Aes->wtheme_handle = (*C.Aes->xmwt->init_module)(&xam_api, &screen, (char *)&cfg.widg_name)))
	{
		display("module returned NULL");
		return -1;
	}

	/*
	 * Setup default widget theme
	 */
	init_client_widget_theme(C.Aes);
	
	/*
	 *  ---------        prepare the AES object renderer module  --------------
	 */
	/*
	 * Now load and fix the resource containing the extended AES object icons
	 * This will be done by the object renderer later on...
	 */
	if (!(resource_name = xaaes_sysfile(cfg.xobj_name)))
	{
		display("ERROR: Can't find extended AES objects resource file '%s'", cfg.xobj_name);
		return -1;
	}
	else
	{
		xobj_rshdr = LoadResources(C.Aes,
					 resource_name,
					 NULL,
					 screen.c_max_w, // < 8 ? 8 : screen.c_max_w,
					 screen.c_max_h, //< 16 ? 16 : screen.c_max_h); //DU_RSX_CONV, DU_RSY_CONV);
					 false);
		kfree(resource_name);
		DIAGS(("xobj_rsc = %lx (%s)", xobj_rsc, cfg.widg_name));
	}
	if (!xobj_rshdr)
	{
		display("ERROR: Can't load extended AES objects resource file '%s'", cfg.xobj_name);
		return -1;
	}

	/* get widget object parameters. */
	{
		int i;
		RECT c;
		OBJECT *tree = ResourceTree(xobj_rshdr, EXT_AESOBJ);
		
		ob_spec_xywh(tree, 1, &c);

		for (i = 1; i < EXTOBJ_NAME; i++)
			tree[i].ob_x = tree[i].ob_y = 0;
		
		xobj_rsc = tree;
	}

// 	init_ob_render();

#if FILESELECTOR
	/* Do some itialisation */
	init_fsel();
#endif
	init_client_mdbuff(C.Aes);		/* In xa_appl.c */
	
	/* Create the root (desktop) window
	 * - We don't want messages from it, so make it a NO_MESSAGES window
	 */
	DIAGS(("creating root window"));
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
#if 0
	{
		char *vs = object_get_spec(C.Aes->std_menu->tree + SYS_DESK)->free_string;
		strcpy(vs + strlen(vs) - 3, version + 3);
	}
#endif
	DIAGS(("menu widget set"));
// 	display("menu widget set");

	/* Fix up the file selector menu */
	//fix_menu(C.Aes, ResourceTree(C.Aes_rsc, FSEL_MENU), false);

	/* Fix up the window widget bitmaps and any others we might be using
   	 * (Calls vr_trnfm() for all the bitmaps)
   	 * HR: No, it doesnt! ;-)
   	 */
	DIAGS(("fixing up widgets"));
//	fix_default_widgets(widget_resources);

	/* Set a default desktop */
	DIAGS(("setting default desktop"));
// 	display("setting default desktop");
	{
		OBJECT *ob = get_xa_desktop();
		*(RECT*)&ob->ob_x = root_window->r;
		(ob + DESKTOP_LOGO)->ob_x = (root_window->wa.w - (ob + DESKTOP_LOGO)->ob_width) / 2;
		(ob + DESKTOP_LOGO)->ob_y = (root_window->wa.h - (ob + DESKTOP_LOGO)->ob_height) / 2;
		C.Aes->desktop = new_widget_tree(C.Aes, ob);
		
		set_desktop_widget(root_window, C.Aes->desktop);
		set_desktop(C.Aes, false);
	}
	DIAGS(("display root window"));
// 	display("display root window");
	open_window(NOLOCKING, root_window, screen.r);

	/* Initial iconified window coords */
	C.iconify = iconify_grid(0);

	v_show_c(v->handle, 0); /* 0 = reset */

// 	display("Open taskman -- perhaps");
// 	if (cfg.opentaskman)
// 		open_taskmanager(NOLOCKING);

// 	display("redrawing menu");
	redraw_menu(NOLOCKING);
// 	display("all fine - return 0");
	return 0;
}

void
init_helpthread(enum locks lock, struct xa_client *client)
{

	DIAGS(("setting up task manager"));
// 	display("setting up task manager");
	set_slist_object(0, new_widget_tree(client, ResourceTree(C.Aes_rsc, TASK_MANAGER)), NULL, TM_LIST,
			 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_ICONINDENT,
			 NULL, NULL, NULL, NULL, NULL,
			 NULL, NULL, NULL, NULL,
			 "Client Applications", NULL, NULL, 255);


	DIAGS(("setting up file selector"));

	DIAGS(("setting up System Alert log"));
// 	display("setting up System Alert log");
	set_slist_object(0, new_widget_tree(client, ResourceTree(C.Aes_rsc, SYS_ERROR)), NULL, SYSALERT_LIST,
			 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_TREEVIEW|SIF_AUTOOPEN,
			 NULL, NULL, NULL, NULL, NULL,
			 NULL, NULL, NULL, NULL,
			 NULL, NULL, NULL, 255);
	{
		OBJECT *obtree = ResourceTree(C.Aes_rsc, SYS_ERROR);
		struct scroll_info *list = object_get_slist(obtree/*ResourceTree(C.Aes_rsc, SYS_ERROR)*/ + SYSALERT_LIST);
		struct scroll_content sc = {{ 0 }};
		char a[] = "Alerts";
		char e[] = "Environment";

		sc.t.text = a;
		sc.t.strings = 1;
		sc.icon = obtree + SALERT_IC1;
		sc.usr_flags = 1;
		sc.xflags = OF_AUTO_OPEN;
		DIAGS(("Add Alerts entry..."));
		list->add(list, NULL, NULL, &sc, 0, (SETYP_AMAL|SETYP_STATIC), NOREDRAW);
		sc.t.text = e;
		sc.icon = obtree + SALERT_IC2;
		DIAGS(("Add Environment entry..."));
		list->add(list, NULL, NULL, &sc, 0, (SETYP_AMAL|SETYP_STATIC), NOREDRAW);
	}

	DIAGS(("setting up About text list"));
// 	display("setting up About text list");
	set_slist_object(0, new_widget_tree(client, ResourceTree(C.Aes_rsc, ABOUT_XAAES)), NULL, ABOUT_LIST, 0,
			 NULL, NULL, NULL, NULL, NULL,
			 NULL, NULL, NULL, NULL,
			 NULL, NULL, NULL, 255);

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

	DIAGS(("load_accs: enter (%s)", buf));

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

				DIAGS(("load_accs: launch (%s)", buf));
				launch(NOLOCKING, 3, 0, 0, buf, "", C.Aes);
			}

			r = kernel_readdir(&dirh, name, len);
		}

		kernel_closedir(&dirh);
	}
}
