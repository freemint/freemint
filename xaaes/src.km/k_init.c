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

#include "k_main.h"
#include "messages.h"
#include "form.h"
#include "k_init.h"
#include "xa_global.h"
#include "xa_strings.h"

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
#include "info.h"
#include "xa_evnt.h"
#include "xa_form.h"
#include "xa_fsel.h"
#include "xa_menu.h"
#include "xa_rsrc.h"
#include "xa_shel.h"
#if WITH_BBL_HELP
#include "xa_bubble.h"
#endif

#include "mint/dcntl.h"
#include "mint/fcntl.h"

#include "obtree.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "xa_graf.h"
#include "trnfm.h"

#include "win_draw.h"
#include "render_obj.h"

#include "xa_xtobj.h"

#include "mvdi.h"
#include "mt_gem.h"
/* kernel header */
#include "mint/ssystem.h"
#include "cookie.h"

#ifndef COOKIE_fVDI
# define COOKIE_fVDI	0x66564449L
#endif

struct xa_module_api xam_api;

STATIC char *xaaes_sysfile(const char *);

static OBSPEC * _cdecl
api_object_get_spec(OBJECT *ob)
{
	return object_get_spec(ob);
}

static void _cdecl
api_object_set_spec(OBJECT *ob, unsigned long cl)
{
	object_set_spec(ob, cl);
}

static POPINFO * _cdecl
api_object_get_popinfo(OBJECT *ob)
{
	return object_get_popinfo(ob);
}
static TEDINFO * _cdecl
api_object_get_tedinfo(OBJECT *ob, XTEDINFO **x)
{
	return object_get_tedinfo(ob, x);
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

static void _cdecl
api_render_object(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object item, short px, short py)
{
	display_object(0, wt, v, item, px, py, 0);
}

static CICON * _cdecl
api_getbest_cicon(CICONBLK *ciconblk)
{
	return getbest_cicon(ciconblk);
}

static short _cdecl
api_obj_offset(struct widget_tree *wt, struct xa_aes_object object, short *mx, short *my)
{
	return obj_offset(wt, object, mx, my);
}

static void _cdecl
api_obj_rectangle(struct widget_tree *wt, struct xa_aes_object object, RECT *r)
{
	obj_rectangle(wt, object, r);
}

// static short _cdecl
// api_object_thickness(OBJECT *ob){return object_thickness(ob);}

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

static short _cdecl
api_rect_clip(RECT *s, RECT *d, RECT *r)
{
	return (xa_rect_clip(s, d, r)) ? 1 : 0;
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

static void * _cdecl
api_lookup_xa_data_byid(struct xa_data_hdr **l, long id)
{
	return lookup_xa_data_byid(l, id);
}
static void * _cdecl
api_lookup_xa_data_byname(struct xa_data_hdr **l, char *name)
{
	return lookup_xa_data_byname(l, name);
}
static void * _cdecl
api_lookup_xa_data_byidname(struct xa_data_hdr **l, long id, char *name)
{
	return lookup_xa_data_byidname(l, id, name);
}

static void _cdecl
api_add_xa_data(struct xa_data_hdr **list, void *data, long id, char *name, void _cdecl(*destruct)(void *d))
{
	add_xa_data(list, data, id, name, destruct);
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
api_ref_xa_data(struct xa_data_hdr **list, void *data, short count)
{
	ref_xa_data(list, data, count);
}
static long _cdecl
api_deref_xa_data(struct xa_data_hdr **list, void *data, short flags)
{
	return deref_xa_data(list, data, flags);
}
static void _cdecl
api_free_xa_data_list(struct xa_data_hdr **list)
{
	free_xa_data_list(list);
}

static void _cdecl
api_load_img(char *fname, XAMFDB *img)
{
	load_image(fname, img);
}

static long _cdecl
module_register(long mode, void *_p)
{
	long ret = E_OK;

	switch ((mode & 0xefffffff))
	{
		case MODREG_KERNKEY:
		{
			struct kernkey_entry *list;

			if (mode & MODREG_UNREGISTER)
			{
				struct kernkey_entry *this = _p, *prev = NULL;
				list = C.kernkeys;
				while (list)
				{
					if (list->key == this->key)
					{
						if (list->act == this->act)
						{
							if ((this = list->next_act))
								this->next_key = list->next_key;
							else
								this = list->next_key;

							if (prev)
								prev->next_key = this;
							else
								C.kernkeys = this;

							kfree(list);
							break;
						}
						else
						{
							while (list && list->act != this->act)
							{
								prev = list;
								list = list->next_act;
							}
							if (list)
							{
								prev->next_act = list->next_act;
								kfree(list);
							}
							break;
						}
					}
					prev = list;
					list = list->next_key;
				}
			}
			else
			{
				struct kernkey_entry *new;
				new = kmalloc(sizeof(*new));
				if (new)
				{
					struct kernkey_entry *p = _p;

					new->next_key = NULL;
					new->next_act = NULL;
					new->act = p->act;
					new->key = p->key;

					list = C.kernkeys;
					while (list)
					{
						if (list->key == new->key)
							break;
						list = list->next_key;
					}
					if (!list)
					{
						new->next_key = C.kernkeys;
						C.kernkeys = new;
					}
					else
					{
						new->next_act = list;
						new->next_key = list->next_key;
						list->next_key = NULL;
						if (C.kernkeys == list)
							C.kernkeys = new;
					}
				}
				else
					ret = -1;
			}
			break;
		}
		default:
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

void
setup_xa_module_api(void)
{
	xam_api.cfg		= &cfg;
	xam_api.C		= &C;
	xam_api.S		= &S;
	xam_api.k		= kentry;

	xam_api.display		= display;
	xam_api.ndisplay	= ndisplay;
	xam_api.bootlog		= bootlog;

	xam_api.module_register	= module_register;

	xam_api.sysfile		= xaaes_sysfile;
	xam_api.load_resource	= LoadResources;
	xam_api.resource_tree	= ResourceTree;
	xam_api.obfix		= obfix;

	xam_api.duplicate_obtree = duplicate_obtree;
	xam_api.free_object_tree = free_object_tree;

	xam_api.init_widget_tree = init_widget_tree;
	xam_api.new_widget_tree	= new_widget_tree;
	xam_api.obtree_to_wt	= obtree_to_wt;
	xam_api.remove_wt	= remove_wt;

	xam_api.object_get_spec	= api_object_get_spec;
	xam_api.object_set_spec = api_object_set_spec;
	xam_api.object_get_popinfo = api_object_get_popinfo;
 	xam_api.object_get_tedinfo = api_object_get_tedinfo;
	xam_api.object_spec_wh	= api_object_spec_wh;

	xam_api.ob_spec_xywh	= api_ob_spec_xywh;
	xam_api.getbest_cicon	= api_getbest_cicon;
	xam_api.obj_offset	= api_obj_offset;
	xam_api.obj_rectangle	= api_obj_rectangle;
	xam_api.obj_set_radio_button = obj_set_radio_button;
	xam_api.obj_get_radio_button = obj_get_radio_button;

	xam_api.render_object	= api_render_object;

	xam_api.rp2ap		= api_rp2ap;
	xam_api.rp2apcs		= api_rp2apcs;

	xam_api.rect_clip	= api_rect_clip;

	xam_api.kmalloc		= api_kmalloc;
	xam_api.umalloc		= api_umalloc;
	xam_api.kfree		= api_kfree;
	xam_api.ufree		= api_ufree;
	xam_api.bclear		= api_bzero;

	xam_api.lookup_xa_data		= api_lookup_xa_data;
	xam_api.lookup_xa_data_byid	= api_lookup_xa_data_byid;
	xam_api.lookup_xa_data_byname	= api_lookup_xa_data_byname;
	xam_api.lookup_xa_data_byidname = api_lookup_xa_data_byidname;
	xam_api.add_xa_data		= api_add_xa_data;
	xam_api.remove_xa_data		= api_remove_xa_data;
	xam_api.delete_xa_data		= api_delete_xa_data;
	xam_api.ref_xa_data		= api_ref_xa_data;
	xam_api.deref_xa_data		= api_deref_xa_data;
	xam_api.free_xa_data_list	= api_free_xa_data_list;

	xam_api.load_img	= api_load_img;

	xam_api.create_window	= create_window;
	xam_api.open_window	= open_window;
	xam_api.close_window	= close_window;
	xam_api.move_window	= move_window;
	xam_api.top_window	= top_window;
	xam_api.bottom_window	= bottom_window;
	xam_api.send_wind_to_bottom = send_wind_to_bottom;
	xam_api.delete_window	= delete_window;
	xam_api.delayed_delete_window = delayed_delete_window;

	xam_api.create_dwind	= create_dwind;

	xam_api.redraw_toolbar	= redraw_toolbar;

	xam_api.dispatch_shutdown = dispatch_shutdown;
}

/*
 * check if file exist in Aes_home_path
 */
STATIC char * _cdecl
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
}

static short
calc_average_fontsize(struct xa_vdi_settings *v, short *maxw, short *maxh, short *dist)
{
	short i, j, count = 0, cellw, tmp;
	short d, w = 0;
	short temp[8];
	unsigned long wch = 0, dev = 0;

	for (i = 1; i < 256; i++)
	{
		j = vqt_width(v->handle, i, &cellw, &tmp, &tmp);
		if (j >= 0)
		{
			count++;
			wch += cellw;
			if( w > 0 ){
				d = cellw - w;
				dev += d * d;
			}
			w = cellw;
		}
// 		display("i=%d, j=%d, count=%d, cellw=%d, totalw=%ld", i, j, count, cellw, wch);
	}
	if (count)
	{
		*maxw = (wch+(count/2)) / count;
		dev *= 1000;
		dev /= count;
	}
	else
	{
		BLOG((0,"font has no width, assuming 8!" ));
		*maxw = 8;
	}
	(*v->api->t_extent)(v, "A_", &cellw, &tmp);
	*maxh = tmp;

	if( *maxh <= 0 ){
		BLOG((0,"font has no height, assuming 16!" ));
		*maxh = 16;
	}

	vqt_fontinfo(v->handle, &i, &i, dist, &i, temp);

// 	display("w %d(%d), h %d", *maxw, tmp, *maxh);
// 	display("dists %d, %d, %d, %d, %d - %d, %d", dist[0], dist[1], dist[2], dist[3], dist[4]);
// 	display("effex %d, %d, %d", temp[0], temp[1], temp[2]);

	return dev;
}
int
k_init(unsigned long vm)
{
	short work_in[16];
	short work_out[58];
	short dev1, dev2;
	char *resource_name;
	struct xa_vdi_settings *v = &global_vdi_settings;

// 	display("\033H");		/* cursor home */

	{
		short f, *t;

		for (t = (short *)v, f = 0; f < (sizeof(*v) >> 1); f++)
			*t++ = -1;
	}

	cfg.videomode = (short)vm;
	BLOG((0,"k_init: videomode=%d",cfg.videomode ));

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
		if (!(s_system(S_GETCOOKIE, COOKIE_fVDI, (unsigned long)&p)))
		{
			C.fvdi_version = *p;
			BLOG((false, "fvdi version = %x", C.fvdi_version));
		}
#if BOOTLOG
		else
			BLOG((false, "could not determine fvdi version"));
#endif
	}
	c_conws("\033E\033f");		/* Clear screen, cursor home (for WongCKs falcon) and cursor off */

	/* try to open virtual wk - necessary when physical wk is already open
	 */
	v->handle = 0;
#if 1
	if( C.P_handle > 0 || (cfg.et4000_hack && C.nvdi_version > 0x400 && C.fvdi_version == 0) )
	{
		memset( work_out, 0, sizeof(work_out) );
		set_wrkin(work_in, 1);	//cfg.videomode);
		BLOG((0,"1st v_opnvnwk (P_handle=%d)", C.P_handle ));
		v_opnvwk(work_in, &v->handle, work_out);
		BLOG((0,"->%d, wh=%d/%d %d colors", v->handle, work_out[0], work_out[1], work_out[13]));
		if( !(work_out[0] && work_out[1] && work_out[13]) )
		{
			BLOG((0,"invalid values, trying physical workstation"));
			v->handle = 0;
		}
		else
			C.f_phys = 1;

	}
#endif
	if ( v->handle <= 0 )
	{
		short mode = 1;
		long vdo, r;

		r = s_system(S_GETCOOKIE, COOKIE__VDO, (unsigned long)(&vdo));
		if (r != 0)
			vdo = 0;

		BLOG((0,"k_init:vdo=%lx vm=%lx video=%d", vdo, vm, cfg.videomode ));

		if ( cfg.videomode)
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
				BLOG((false, "Falcon video: videomode %d(%x),mode=%d,nvmode=%x", cfg.videomode, cfg.videomode, mode, nvmode));
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
					BLOG((false, "videomode %d invalid, must be between 1 and 10", cfg.videomode));
				}
			}
		}	/*/if (cfg.videomode)*/
		else
		{
			BLOG((false, "Default screenmode"));
		}
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
#if SAVE_CACHE_WK
			unsigned long sc = 0, cm = 0;

			cm = s_system(S_CTRLCACHE, 0L, -1L);
			sc = s_system(S_CTRLCACHE, -1L, 0L);
			s_system(S_CTRLCACHE, sc & ~3, cm);
#endif
			set_wrkin(work_in, mode);
			BLOG((0,"k_init: v_opnwk() mode=%d", mode ));
			v_opnwk(work_in, &(C.P_handle), work_out);
			BLOG((0,"k_init: v_opnwk() handle=%d", C.P_handle ));
#if SAVE_CACHE_WK
			s_system(S_CTRLCACHE, sc, cm);
#endif
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
		//v_exit_cur(C.P_handle);

		/*
		 * Open us a virtual workstation for XaAES to play with
		 */
		//v->handle = C.P_handle;

		set_wrkin(work_in, C.P_handle);
		v_opnvwk(work_in, &v->handle, work_out);
	}

	if (v->handle == 0)
	{
		BLOG((true, "v_opnvwk failed (%i)!", v->handle));
		return -1;
	}
	BLOG((false, "Virtual work station opened: %d", v->handle));

	/*
	 * Setup the screen parameters
	 */
	if( C.P_handle == 0 && C.f_phys )
		C.P_handle = 1;	// HOPE it's 1 ..
	screen.r.x = screen.r.y = 0;
	screen.r.w = work_out[0] + 1;
	screen.r.h = work_out[1] + 1;
	screen.colours = work_out[13];
	screen.display_type = D_LOCAL;
	v->screen = screen.r;

	vq_extnd(v->handle, 1, work_out);	/* Get extended information */

	if( !(work_out[0] == 4 || work_out[0] == 1) )
	{
		BLOG((0,"invalid screen-type:%d", work_out[0] ));
		return -1;
	}
	screen.planes = work_out[4];		/* number of planes in the screen */

	C.Aes->vdi_settings = v;
	vs_clip(/*C.P_*/v->handle, 1, (short *)&screen.r);
	(*v->api->set_clip)(v, &screen.r);

	(*v->api->f_perimeter)(v, 0);

// 	v_show_c(C.P_handle, 0);
// 	hidem();
// 	xa_graf_mouse(ARROW, NULL, NULL, false);
// 	showm();

	objc_rend.dial_colours = MONO ? bw_default_colours : default_colours;

	BLOG((0,"lookup-support:%d, planes:%d", work_out[5], work_out[4] ));

	/* WARNING: This is halfway nonsense! */
	if( screen.planes > 32 || screen.planes < 1 )
	{
		long b, s = 0;
		BLOG((1,"planes wrong: %d (colors=%d)", screen.planes, screen.colours ));
		for( b = screen.colours; b>1; b >>= 1 )
			s++;
		screen.planes = s;
		BLOG((1,"planes now: %d", screen.planes ));
		if( screen.planes > 32 || screen.planes < 1 )
		{
			BLOG((1,"planes still wrong: %d", screen.planes ));
			return 1;
		}
	}

	screen.pixel_fmt = detect_pixel_format(v);
	BLOG((false, "Video info: width(%d/%d), planes :%d, colours %d pixel-format %d",
		screen.r.w, screen.r.h, screen.planes, screen.colours, screen.pixel_fmt));

// 	display("Video info: width(%d/%d), planes :%d, colours %d, pixelfmt = %d",
// 		screen.r.w, screen.r.h, screen.planes, screen.colours, screen.pixel_fmt);

// 	if (screen.planes > 8)
// 		set_defaultpalette(v->handle);
	if( cfg.palette[0] && !rw_syspalette( READ, screen.palette, C.Aes->home_path, cfg.palette ) )
	{
		set_syspalette(v->handle, screen.palette);
	}
	else
		get_syspalette(v->handle, screen.palette);

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
	screen.standard_font_id  = screen.small_font_id = cfg.font_id;	//v->font_rid;
	screen.small_font_height = v->font_h;
	screen.small_font_point  = v->font_rsize;
	screen.c_min_w = v->cell_w;
	screen.c_min_h = v->cell_h;
	dev1 = calc_average_fontsize(v, &screen.c_min_w, &screen.c_min_h, &screen.c_min_dist[0]);
	(*v->api->t_font)(v, (screen.r.h <= 280) ? (cfg.standard_font_point = cfg.medium_font_point) : cfg.standard_font_point, -1);
	screen.standard_font_height = v->font_h;
	screen.standard_font_point  = v->font_rsize;
	screen.c_max_w = v->cell_w;
	screen.c_max_h = v->cell_h;
	dev2 = calc_average_fontsize(v, &screen.c_max_w, &screen.c_max_h, &screen.c_max_dist[0]);

#if !BOOTLOG
	UNUSED(dev1);
	UNUSED(dev2);
#endif
	BLOG((false, "stdfont: id = %d, size = %d, cw=%d, ch=%d, dev=%d",
 		screen.standard_font_id, screen.standard_font_point, screen.c_max_w, screen.c_max_h, dev2));
	BLOG((false, "smlfont: id = %d, size = %d, cw=%d, ch=%d, dev=%d",
		screen.small_font_id, screen.small_font_point, screen.c_min_w, screen.c_min_h, dev1));

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

	/* Load the system resource files
	 */
	BLOG((false, "Loading system resource file '%s'", cfg.rsc_name));
	if ( !(resource_name = xaaes_sysfile(cfg.rsc_name) ) )
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
		BLOG((true, "system resource = %lx (%s)", C.Aes_rsc, resource_name));
		kfree(resource_name);
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
		char *t = 0;
		OBJECT *about = ResourceTree(C.Aes_rsc, ABOUT_XAAES);
		int gt = 0;
		if( about )
			t = object_get_tedinfo(about + RSC_VERSION, NULL)->te_ptext;

		if ( !t || about[RSC_VERSION].ob_type != G_TEXT )
		{
			display("ERROR: wrong resource file: %s - use version "RSCFILE_VERSION"!", cfg.rsc_name);
			return -1;
		}
		if ( (ob_count_objs(about, 0, -1) < RSC_VERSION) || ( gt = strcmp(t, RSCFILE_VERSION)) )
		{
			char *s = gt > 0 ? "too new" : gt < 0 ? "too old" : "wrong";
			display("ERROR: %s resource file (current:%s)(%s) - use version "RSCFILE_VERSION"!", s, t, cfg.rsc_name);
			return -1;
		}
	}

	/*
	 *  ---------        prepare the window widgets renderer module  --------------
	 */

	main_xa_theme(&C.Aes->xmwt);
	main_object_render(&C.Aes->objcr_module);

#if WITH_GRADIENTS
	if( cfg.gradients[0] == '0' && cfg.gradients[1] == 0 )
		cfg.gradients[0] = 0;
	if (!(*C.Aes->objcr_module->init_module)(&xam_api, &screen, cfg.gradients[0] != 0))
#else
	if (!(*C.Aes->objcr_module->init_module)(&xam_api, &screen, 0 ) )
#endif
	{
		BLOG((true, "object render returned NULL"));
		return -1;
	}
	if (init_client_objcrend(C.Aes))
	{
		BLOG((true, "Opening object theme failed"));
		return -1;
	}

#if WITH_GRADIENTS
	if (!(C.Aes->wtheme_handle = (*C.Aes->xmwt->init_module)(&xam_api, &screen, (char *)&cfg.widg_name, cfg.gradients[0] != 0)))
		if (!(C.Aes->wtheme_handle = (*C.Aes->xmwt->init_module)(&xam_api, &screen, WIDGNAME, cfg.gradients[0] != 0)))
#else
	if (!(C.Aes->wtheme_handle = (*C.Aes->xmwt->init_module)(&xam_api, &screen, (char *)&cfg.widg_name, 0 )))
		if (!(C.Aes->wtheme_handle = (*C.Aes->xmwt->init_module)(&xam_api, &screen, WIDGNAME, 0 )))
#endif
	{
		BLOG((true,"Window widget module returned NULL"));
		return -1;
	}

	/*
	 *  ---------        prepare the AES object renderer module  --------------
	 */

	/*
	 * Setup default widget theme
	 */
	init_client_widget_theme(C.Aes);

#if FILESELECTOR
	/* Do some itialisation */
	init_fsel();
#endif
	init_client_mdbuff(C.Aes);		/* In xa_appl.c */

	/* Create the root (desktop) window
	 * - We don't want messages from it, so make it a NO_MESSAGES window
	 */
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

	if( !root_window )
		return -1;
	strcpy( root_window->wname, "root" );
// 	display("Fixing up root menu");
	/* Tack a menu onto the root_window widget */
	C.Aes->std_menu = new_widget_tree(C.Aes, ResourceTree(C.Aes_rsc, SYSTEM_MENU));
	assert(C.Aes->std_menu);
	wt_menu_area(C.Aes->std_menu);
	set_rootmenu_area(C.Aes);

	C.Aes->mnu_clientlistname = kmalloc(strlen(xa_strings[MNU_CLIENTS]) + 1);
	assert(C.Aes->mnu_clientlistname);
	strcpy(C.Aes->mnu_clientlistname, xa_strings[MNU_CLIENTS]);
	fix_menu(C.Aes, C.Aes->std_menu, root_window, true);
	set_menu_widget(root_window, C.Aes, C.Aes->std_menu);
	set_menu_width( C.Aes->std_menu->tree, C.Aes->std_menu );

	/* Set a default desktop */
	{
		OBJECT *ob = get_xa_desktop();
		*(RECT*)&ob->ob_x = root_window->r;
		(ob + DESKTOP_LOGO)->ob_x = (root_window->wa.w - (ob + DESKTOP_LOGO)->ob_width) / 2;
		(ob + DESKTOP_LOGO)->ob_y = (root_window->wa.h - (ob + DESKTOP_LOGO)->ob_height) / 2;
		if( cfg.back_col != -1 )
			(ob + DESKTOP)->ob_spec.obspec.interiorcol = cfg.back_col;
		C.Aes->desktop = new_widget_tree(C.Aes, ob);

		set_desktop_widget(root_window, C.Aes->desktop);
		set_desktop(C.Aes, false);
	}
	open_window(NOLOCKING, root_window, screen.r);
	S.open_windows.root = root_window;

	if( cfg.menu_bar == 2 )
		cfg.menu_ontop = cfg.menu_layout = 0;
	// ->fix_menu
	if( cfg.menu_ontop )
	{
		RECT r = C.Aes->std_menu->area;

		r.x = 0;
		if( cfg.menu_layout == 0 )
			r.w = screen.r.w;
		else
			r.w += C.Aes->std_menu->area.x;
		menu_window = create_window(0, NULL, NULL, C.Aes, true, 0, created_for_AES|created_for_POPUP|created_for_MENUBAR, false, false, r, 0,0);
		if( !menu_window )
			return -1;
		strcpy( menu_window->wname, "menubar" );
		if( cfg.menu_bar )
			open_window(0, menu_window, r);
	}
//#define WITH_BBL_HELP 0
#if WITH_BBL_HELP
	if( cfg.xa_bubble || cfg.menu_bar != 2 )
	{
		RECT r = {0,0,420,420};
		bool nolist = false;

		bgem_window = create_window(0, 0, 0, C.Aes, nolist, 0, created_for_AES|created_for_POPUP|created_for_MENUBAR, 0, false, r, 0,0);
		if( !bgem_window )
			return -1;
		strcpy( bgem_window->wname, "bgem" );
		bgem_window->x_shadow = bgem_window->y_shadow = 0;
		xa_bubble( 0, bbl_enable_bubble, 0, 0 );
	}
#endif

	/* Initial iconified window coords */
	C.iconify = iconify_grid(0);
// 	v_show_c(v->handle, 1); /* 0 = reset */

// 	display("Open taskman -- perhaps");
// 	if (cfg.opentaskman)
// 		open_taskmanager(NOLOCKING);

	set_standard_point( C.Aes );

	redraw_menu(NOLOCKING);
// 	display("all fine - return 0");

	/*
	 * Setup mn_set for menu_settings()
	 */
	cfg.menu_settings.mn_set.display = cfg.popup_timeout;
	cfg.menu_settings.mn_set.drag = cfg.popout_timeout;
	cfg.menu_settings.mn_set.delay = 250;
	cfg.menu_settings.mn_set.speed = 0;
	cfg.menu_settings.mn_set.height = root_window->wa.h / screen.c_max_h;
	v_show_c(C.P_handle, 0);
	return 0;
}

void
init_helpthread(enum locks lock, struct xa_client *client)
{
	open_taskmanager(0, client, 0);
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
	}
	strcpy(cfg.acc_path, buf);

	set_drive_and_path(cfg.acc_path);

	BLOG((false, "loading accessories from '%s'", buf));

	name = buf + len;
	len = sizeof(buf) - len;


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
