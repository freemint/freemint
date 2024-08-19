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

#include "k_main.h"
#include "messages.h"
#include "form.h"
#include "k_init.h"
#include "xa_global.h"

#include "xaaes.h"

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
#include "mint/falcon.h"

#include "obtree.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "xa_graf.h"
#include "trnfm.h"

#include "win_draw.h"
#include "render_obj.h"

#include "xa_aes.h"
/* kernel header */
#include "mint/ssystem.h"
#include "cookie.h"

struct xa_module_api xam_api;

static char *xaaes_sysfile(const char *);

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
static SWINFO * _cdecl
api_object_get_swinfo(OBJECT *ob)
{
	return object_get_swinfo(ob);
}
static TEDINFO * _cdecl
api_object_get_tedinfo(OBJECT *ob, XTEDINFO **x)
{
	return object_get_tedinfo(ob, x);
}

static void _cdecl
api_ob_spec_xywh(OBJECT *obtree, short obj, GRECT *r)
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
api_obj_rectangle(struct widget_tree *wt, struct xa_aes_object object, GRECT *r)
{
	obj_rectangle(wt, object, r);
}

static void * _cdecl
api_rp2ap(struct xa_window *wind, struct xa_widget *widg, GRECT *r)
{
	return rp_2_ap(wind, widg, r);
}
static void _cdecl
api_rp2apcs(struct xa_window *wind, struct xa_widget *widg, GRECT *r)
{
	rp_2_ap_cs(wind, widg, r);
}

static short _cdecl
api_rect_clip(GRECT *s, GRECT *d, GRECT *r)
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
	xam_api.object_get_swinfo  = api_object_get_swinfo;
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
 * Workaround for buggy TOS3/4 VDI. We have to set the flag explicitly
 * because we already are in kernel so enter_vdi() wouldn't be called.
 * See sys_m_xalloc() in dosmem.c for further explanation.
 */
static void v_opnvwk_patched(short work_in[], VdiHdl *handle, short work_out[])
{
	struct proc *p = get_curproc();
	p->in_vdi = 1;
	v_opnvwk(work_in, handle, work_out);
	p->in_vdi = 0;
}

/*
 * check if file exist in Aes_home_path
 */
static char * _cdecl
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
}

int
k_init(unsigned short dev, unsigned short mc)
{
	static short work_in[16];
	static short work_out[58];
	char *resource_name;
	struct xa_vdi_settings *v = &global_vdi_settings;
	struct xa_client *client;
	
	{
		short f, *t;

		for (t = (short *)v, f = 0; f < (sizeof(*v) >> 1); f++)
			*t++ = -1;
	}

	cfg.videomode = mc;
	cfg.device    = dev;

	BLOG((0,"k_init: videomode=%d",cfg.videomode ));

	xa_vdiapi = v->api = init_xavdi_module();
	{
		/*
		 * This is actually a pointer to struct which happens to have
		 * its first 16-bit record the BCD version. Applies to both
		 * NVDI and fVDI.
		 */
		short *p;

		if (s_system(S_GETCOOKIE, COOKIE_NVDI, (unsigned long)&p) == E_OK)
		{
			C.nvdi_version = *p;
			BLOG((false, "nvdi version = %x", C.nvdi_version));
		} else
		{
			BLOG((false, "could not determine nvdi version"));
		}
		if (s_system(S_GETCOOKIE, COOKIE_fVDI, (unsigned long)&p) == E_OK)
		{
			C.fvdi_version = *p;
			BLOG((false, "fvdi version = %x", C.fvdi_version));
		} else
		{
			BLOG((false, "could not determine fvdi version"));
		}
	}
	c_conws("\033E\033f");		/* Clear screen, cursor home (for WongCKs falcon) and cursor off */

	/* try to open virtual wk - necessary when physical wk is already open
	 */
	v->handle = 0;

	if( C.P_handle > 0 )
	{
		memset( work_out, 0, sizeof(work_out) );
		set_wrkin(work_in, 1);
		BLOG((0,"1st v_opnvwk (P_handle=%d)", C.P_handle ));
		v_opnvwk_patched(work_in, &v->handle, work_out);
		BLOG((0,"->%d, wh=%d/%d %d colors", v->handle, work_out[0], work_out[1], work_out[13]));
		if( !(work_out[0] && work_out[1] && work_out[13]) )
		{
			BLOG((0,"invalid values, trying physical workstation"));
			v->handle = 0;
		}
		else
			C.f_phys = 1;
	}

	if ( v->handle <= 0 )
	{
		short device = cfg.device;
		short modecode = cfg.videomode;

		if(!device)
		{
			BLOG((false, "Auto detecting screen device"));

			if(nova_data)
			{
				BLOG((false, "Assume NOVA VDI compatibility"));
				device = 1;
			}
			else
			{
				unsigned short tmp[2];

				if (s_system(S_GETCOOKIE, COOKIE__VDI, (unsigned long)tmp) == E_OK)
				{
					device = 7;
					BLOG((false, "Assume Milan VDI Compatibility"));
				}
				else
				{
					if(s_system(S_GETCOOKIE, COOKIE__VDO, (unsigned long)tmp) != E_OK)
						tmp[0] = 0;

					switch(tmp[0])
					{
						case 3:
						case 4:
							BLOG((false, "Assume Falcon VDI compatibility"));
							device = 5;
							break;
						default:
							BLOG((false, "Assume generic VDI compatibility"));
							device = cfg.videomode;
							modecode = 0;
							break;
					}
				}
			}
		}

		if(device < 1 || device > 10)
		{
			BLOG((false, "Screen device %d invalid, must be between 1 and 10. Forcing device = 1", device));
			device = 1;
			modecode = 0;
		}
		else if ((device == 5 || device == 7) && !modecode)
		{
			long dummy;

			if (device == 5 &&
				s_system(S_GETCOOKIE, COOKIE_CT60, (unsigned long)&dummy) == E_OK &&
				s_system(S_GETCOOKIE, COOKIE__PCI, (unsigned long)&dummy) == E_OK)
			{
				/*
				 * CTPCI VDI is yet another beast: it doesn't accept device == 1
				 * and reinitializes screen using Vsetscreen(0, 0, 3, work_out[45]),
				 * if it differs from VsetMode(-1).
				 */
				 BLOG((false, "Video mode not set in CTPCI VDI. Keeping device = 5"));
				 modecode = VsetMode(-1);
			}
			else
			{
				BLOG((false, "Video mode not set in Falcon/Milan VDI. Forcing device = 1"));
				device = 1;
			}
		}

		BLOG((false, "Screen device is: %d", device));

		if(modecode)
		{
			if(nova_data)
			{
				if(nova_data->valid)
				{
					nova_data->xcb->resolution = modecode;
					nova_data->valid = false;
					BLOG((false, "Nova mode is: %x", modecode));
				}
			}
			else
			{
				work_out[45] = Validmode(modecode);
				BLOG((false, "Modecode is: %x (%x)", modecode, work_out[45]));
			}
		}

		/*
		 * When opening the physical workstation, NVDI/ET4000 (and perhaps other
		 * drivers, too) tries to load some files from disk needed for the correct
		 * settings of the respective graphics card. NVDI loads these files from the
		 * current drive, expecting it to be the boot drive. However, this drive is
		 * always set to U: in xaloader's loader_init().
		 */
		d_setdrv(sysdrv);

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
			set_wrkin(work_in, device);
			BLOG((0,"k_init: v_opnwk() device=%d", device ));
			v_opnwk(work_in, &(C.P_handle), work_out);
			BLOG((0,"k_init: v_opnwk() handle=%d", C.P_handle ));
#if SAVE_CACHE_WK
			s_system(S_CTRLCACHE, sc, cm);
#endif
		}
#else
		set_wrkin(work_in, device);
		v_opnwk(work_in, &(C.P_handle), work_out);
#endif
		BLOG((false, "Physical work station opened: %d", C.P_handle));
		
		/* set back to U: */
		d_setdrv('u' - 'a');

		if (C.P_handle == 0)
		{
			BLOG((/* 00000001 */true, "v_opnwk failed (%i)!", C.P_handle));
			return -1;
		}

		/*
		 * We need to get rid of the cursor
		 */
#if 0
		v_exit_cur(C.P_handle);
#endif

		/*
		 * Open us a virtual workstation for XaAES to play with
		 */
		v->handle = C.P_handle;

		set_wrkin(work_in, 1);
		v_opnvwk_patched(work_in, &v->handle, work_out);
	}	/* /if ( v->handle <= 0 ) */

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
	{
		BLOG((false, "Guessing phys-handle is 1"));
		C.P_handle = 1;	/* HOPE it's 1 .. */
	}
	screen.r.g_x = screen.r.g_y = 0;
	screen.r.g_w = work_out[0] + 1;
	screen.r.g_h = work_out[1] + 1;
	screen.colours = work_out[13];
	v->screen = screen.r;

	vq_extnd(v->handle, 1, work_out);	/* Get extended information */

	BLOG((0,"screen-type:%d", work_out[0] ));

	screen.planes = work_out[4];		/* number of planes in the screen */

	client = C.Aes;
	client->vdi_settings = v;
	vs_clip(/*C.P_*/v->handle, 1, (short *)&screen.r);
	(*v->api->set_clip)(v, &screen.r);

	(*v->api->f_perimeter)(v, 0);

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
		screen.r.g_w, screen.r.g_h, screen.planes, screen.colours, screen.pixel_fmt));

	if( cfg.palette[0] && !rw_syspalette( READ, screen.palette, client->home_path, cfg.palette ) )
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
		cfg.font_id = STANDARD_AES_FONTID;

	(*v->api->t_alignment)(v, 0, 5);
	(*v->api->t_font)(v, cfg.small_font_point, cfg.font_id);
	screen.standard_font_id  = screen.small_font_id = cfg.font_id;
	screen.small_font_height = v->font_h;
	screen.small_font_point  = v->font_rsize;
	screen.c_min_w = v->cell_w;
	screen.c_min_h = v->cell_h;
	calc_average_fontsize(v, &screen.c_min_w, &screen.c_min_h, &screen.c_min_dist[0]);
	if (screen.r.g_h <= 280)
	{
		cfg.standard_font_point = cfg.medium_font_point;
		if (cfg.xaw_point == STANDARD_FONT_POINT)
			cfg.xaw_point = cfg.medium_font_point;
	}
	(*v->api->t_font)(v, cfg.standard_font_point, -1);
	screen.standard_font_height = v->font_h;
	screen.standard_font_point  = v->font_rsize;
	screen.c_max_w = v->cell_w;
	screen.c_max_h = v->cell_h;
	calc_average_fontsize(v, &screen.c_max_w, &screen.c_max_h, &screen.c_max_dist[0]);

	default_fnt.normal.font_point = cfg.xaw_point;
	default_fnt.selected.font_point = cfg.xaw_point;
	default_fnt.highlighted.font_point = cfg.xaw_point;

	client->options.standard_font_point = cfg.standard_font_point;

	if( cfg.info_font_point == -1 )
		cfg.info_font_point = cfg.standard_font_point;

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
	BLOG((false, " size=[%d,%d], colours=%d, bitplanes=%d", screen.r.g_w, screen.r.g_h, screen.colours, screen.planes));


	/* Load the system resource files
	 */
	xa_strings = NULL;
	BLOG((false, "Loading system resource file '%s' lang='%s'(%d) trans=%d", cfg.rsc_name, cfg.lang, info_tab[3][0], client->options.rsc_lang));
	if ( !(resource_name = xaaes_sysfile(cfg.rsc_name) ) )
	{
		display(/* 00000002 */"ERROR: Can't find AESSYS resource file '%s'", cfg.rsc_name);
		return -1;
	}
	else
	{
		C.Aes_rsc = LoadResources(client,
					  resource_name,
					  NULL,
					  DU_RSX_CONV,
					  DU_RSY_CONV,
					  true);

		BLOG((0, "system resource = %lx (%s)", (unsigned long)C.Aes_rsc, resource_name));
		kfree(resource_name);
	}
	client->options.rsc_lang = 0;	/* dont translate other rsc-files */
	if (!C.Aes_rsc)
	{
		display(/*00000003*/"ERROR: Can't load system resource file '%s'", cfg.rsc_name);
		return -1;
	}

	/*
	 * Version check the aessys resource
	 */
	{
		const char *rsm_crc;

		rsm_crc = ResourceString(C.Aes_rsc, _RSM_CRC_);
		if (rsm_crc == NULL || strcmp(rsm_crc, _RSM_CRC_STRING_) != 0)
		{
			display("ERROR: %s: resource version mismatch!", cfg.rsc_name);
			return -1;
		}
	}

	/*
	 *  ---------        prepare the window widgets renderer module  --------------
	 */

	main_xa_theme(&client->xmwt);
	main_object_render(&client->objcr_module);

	if (cfg.textures[0] == '0' && cfg.textures[1] == '\0')
		cfg.textures[0] = 0;

#if WITH_GRADIENTS
	if (cfg.gradients[0] == '0' && cfg.gradients[1] == '\0')
		cfg.gradients[0] = 0;
	if (!(*client->objcr_module->init_module)(&xam_api, &screen, cfg.gradients[0] != 0))
#else
	if (!(*client->objcr_module->init_module)(&xam_api, &screen, 0 ) )
#endif
	{
		BLOG((true, "object render returned NULL"));
		return -1;
	}
	if (init_client_objcrend(client))
	{
		BLOG((true, "Opening object theme failed"));
		return -1;
	}

#if WITH_GRADIENTS
	if (!(client->wtheme_handle = (*client->xmwt->init_module)(&xam_api, &screen, cfg.widg_name, cfg.gradients[0] != 0)))
		if (!(client->wtheme_handle = (*client->xmwt->init_module)(&xam_api, &screen, WIDGNAME, cfg.gradients[0] != 0)))
#else
	if (!(client->wtheme_handle = (*client->xmwt->init_module)(&xam_api, &screen, cfg.widg_name, 0 )))
		if (!(client->wtheme_handle = (*client->xmwt->init_module)(&xam_api, &screen, WIDGNAME, 0 )))
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
	init_client_widget_theme(client);

#if FILESELECTOR
	/* Do some itialisation */
	init_fsel();
#endif
	init_client_mdbuff(client);		/* In xa_appl.c */

	/* Create the root (desktop) window
	 * - We don't want messages from it, so make it a NO_MESSAGES window
	 */

	root_window = create_window(
				NOLOCKING,
				NULL, /* No messages */
				NULL, /* No 'doer' */
				client,
				false,
				XaMENU,			/* menu standard widget */
				created_for_AES,
				-1, false, /*false,false,*/
				&screen.r,		/* current size */
				&screen.r,		/* maximum size (NB default would be rootwindow->wa) */
				0);			/* need no remembrance */

	if( !root_window )
		return -1;
	strcpy( root_window->wname, "root" );
	/* Tack a menu onto the root_window widget */
	client->std_menu = new_widget_tree(client, ResourceTree(C.Aes_rsc, SYSTEM_MENU));
	assert(client->std_menu);
	wt_menu_area(client->std_menu);
	set_rootmenu_area(client);

	client->mnu_clientlistname = kmalloc(strlen(xa_strings(MNU_CLIENTS)) + 1);
	assert(client->mnu_clientlistname);
	strcpy(client->mnu_clientlistname, xa_strings(MNU_CLIENTS));
	fix_menu(client, client->std_menu, root_window, true);
	set_menu_widget(root_window, client, client->std_menu);
	set_menu_width( client->std_menu->tree, client->std_menu );

	/* Set a default desktop */
	{
		OBJECT *ob = get_xa_desktop();
		*(GRECT*)&ob->ob_x = root_window->r;
		(ob + DESKTOP_LOGO)->ob_x = (root_window->wa.g_w - (ob + DESKTOP_LOGO)->ob_width) / 2;
		(ob + DESKTOP_LOGO)->ob_y = (root_window->wa.g_h - (ob + DESKTOP_LOGO)->ob_height) / 2;
		if( cfg.back_col != -1 )
			(ob + DESKTOP)->ob_spec.obspec.interiorcol = cfg.back_col;
		client->desktop = new_widget_tree(client, ob);

		set_desktop_widget(root_window, client->desktop);
		set_desktop(client, false);
	}
	open_window(NOLOCKING, root_window, screen.r);
	S.open_windows.root = root_window;

	if( cfg.menu_bar == 2 )
		cfg.menu_ontop = cfg.menu_layout = 0;

	if( cfg.menu_ontop )
	{
		GRECT r = client->std_menu->area;

		r.g_x = 0;
		if( cfg.menu_layout == 0 )
			r.g_w = screen.r.g_w;
		else
			r.g_w += client->std_menu->area.g_x;
		menu_window = create_window(0, NULL, NULL, client, true, 0, created_for_AES|created_for_POPUP|created_for_MENUBAR, false, false, &r, 0,0);
		if( !menu_window )
			return -1;
		strcpy( menu_window->wname, "menubar" );
		if( cfg.menu_bar )
			open_window(0, menu_window, r);
	}
#if WITH_BBL_HELP
	if( cfg.xa_bubble || cfg.menu_bar != 2 )
	{
		GRECT r = {0,0,420,420};
		bool nolist = false;

		bgem_window = create_window(0, 0, 0, client, nolist, 0, created_for_AES|created_for_POPUP|created_for_MENUBAR, 0, false, &r, 0,0);
		if( !bgem_window )
			return -1;
		strcpy( bgem_window->wname, "bgem" );
		bgem_window->x_shadow = bgem_window->y_shadow = 0;
		xa_bubble( 0, bbl_enable_bubble, 0, 0 );
	}
#endif

	/* Initial iconified window coords */
	C.iconify = iconify_grid(0);

	set_standard_point(client);

	redraw_menu(NOLOCKING);

	/*
	 * Setup mn_set for menu_settings()
	 */
	cfg.menu_settings.mn_set.display = cfg.popup_timeout;
	cfg.menu_settings.mn_set.drag = cfg.popout_timeout;
	cfg.menu_settings.mn_set.delay = 250;
	cfg.menu_settings.mn_set.speed = 0;
	cfg.menu_settings.mn_set.height = root_window->wa.g_h / screen.c_max_h;
	v_show_c(C.P_handle, 0);
	return 0;
}

void
init_helpthread(int lock, struct xa_client *client)
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

static char const accpath[] = "ACCPATH=";

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
