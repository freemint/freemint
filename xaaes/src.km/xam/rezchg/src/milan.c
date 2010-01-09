/*
 * $Id$
 *
 * This file is a part of the XaAES/FreeMiNT project.
 *
 *
 * Copyright 2007 Odd Skancke
 * All rights reserved.
 *
 *
 * XaAES module: Setup VDI for resolution, and provide
 * a resolution change dialog for Milan VDI.
 *
 */

#define XAAES_MODULE
#define BOOTLOG 1
#define GENERATE_DIAGS 0

/*
 * Odd Skancke:
 *  IMPORTANT; ALWAYS include libkern.h _BEFORE_ xaaes_module.h
 *  because some things in libkern.h is redefined in
 *  xaaes_module.h!
 */
#include "mt_gem.h"
#include "libkern/libkern.h"
#include "mint/stat.h"
#include "xaaes_module.h"
#include "ob_inlines.h"
#include "ssystem.h"
#include "cookie.h"

#include "mvdi.h"

#include "rcmilan.h"

#define RESFILE "rcmilan.res"
#define RSCFILE "rcmilan.rsc"

long _cdecl init(struct kernel_module *km, void *arg, struct device_methods ***r);
static void _cdecl open_milan_reschange(enum locks lock, struct xa_client *client, bool open);

struct xa_api *XAPI = NULL;
struct kentry *KENTRY = NULL;

static struct xa_window *our_wind = NULL;
static RSHDR *our_rshdr = NULL;
static const char *our_path = NULL;
static struct cookie_mvdi *mvdi = NULL;

static char t_reschg[] = " Change Resolution ";
static struct register_kernkey_parms kk = { 'R', 0, open_milan_reschange };

static long _cdecl rcprobe(device_t dev);
static long _cdecl rcattach(device_t dev);
static long _cdecl rcdetach(device_t dev);

static long _cdecl setres_attach(device_t dev);
static long _cdecl setres_detach(device_t dev);

DEVICE_METHODS(devmethods, NULL, rcprobe, rcattach, rcdetach);
DEVICE_METHODS(setresmethods, NULL, rcprobe, setres_attach, setres_detach);

static struct device_methods *ourdms[] =
{
	&devmethods, &setresmethods, NULL
};

static long _cdecl
rcprobe(device_t dev)
{
	unsigned long tmp;

	if (XAPI && !strcmp("XaAES API v0.1", XAPI->name)) {
		if (mvdi)
			return E_OK;
		else if (!(s_system(S_GETCOOKIE, COOKIE__VDI, (unsigned long)&tmp))) {
			mvdi = (struct cookie_mvdi *)tmp;
			our_path = dev->km->fpath;
			return E_OK;
		}
	}
	return ENXIO;
}

static long _cdecl
rcattach(device_t dev)
{
	long err;

	err = module_register(MODREG_KERNKEY, &kk);
	BLOG((true, "rcmilan: rcattach - KERNKEY reg returns %ld", err));
	return err;
}

static long _cdecl
rcdetach(device_t dev)
{
	module_register(MODREG_KERNKEY | MODREG_UNREGISTER, &kk);
	if (our_wind) {
		close_window(0, our_wind);
		delayed_delete_window(0, our_wind);
		free_resource(NULL, NULL, our_rshdr);
		our_rshdr = NULL;
	}
	return E_OK;
}

static long _cdecl setupmvdi(unsigned long vm, short *work_in, short *work_out, short *retmode);

static long _cdecl
setres_attach(device_t dev)
{
	long err;
	err = module_register(MODREG_SETUPVDI, setupmvdi);
	return err;
}

static long _cdecl
setres_detach(device_t dev)
{
	return (module_register(MODREG_SETUPVDI | MODREG_UNREGISTER, setupmvdi));
}

static void
save_res(unsigned long res)
{
	struct file *fp;
	char *f;

	f = make_fqfname(our_path, RESFILE);
	if (f) {
		fp = kernel_open(f, O_WRONLY|O_CREAT,NULL,NULL);
		kfree(f);
		if (fp) {
			kernel_write(fp, &res, sizeof(res));
			kernel_close(fp);
		}
	}
}

static long
load_res(unsigned long *res)
{
	struct file *fp;
	char *f;
	unsigned long r;

	if (res) {
		f = make_fqfname(our_path, RESFILE);
		if (f) {
			fp = kernel_open(f, O_RDONLY, NULL,NULL);
			kfree(f);
			if (fp) {
				long size = kernel_read(fp, &r, sizeof(r));
				kernel_close(fp);
				if (size == sizeof(r)) {
					*res = r;
					return E_OK;
				}
			}
		}
	}
	return EERROR;
}

long _cdecl
init(struct kernel_module *km, void *arg, struct device_methods ***r)
{
	KENTRY = km->kentry;
	if (check_kentry_version())
		return 1;
	XAPI = arg;
	*r = ourdms;
	return E_OK;
}

/* ************************************************************ */
/*     Nova resolution mode change functions			*/
/* ************************************************************ */

static long _cdecl
setupmvdi(unsigned long vm, short *work_in, short *work_out, short *retmode)
{
	long ret = EERROR;

	if (load_res(&vm) == E_OK || (vm & 0x80000000)) {
		*retmode = 7;
		work_out[45] = vcheckmode(vm & 0x0000ffff);
		ret = E_OK;
	}
	return ret;
}

static char *coldepths[] =
{
	"Monochrome",
	"4 colors",
	"16 colors",
	"256 colors",
	"15 bpp (32K)",
	"16 bpp (64K)",
	"24 bpp (16M)",
	"32 bpp (16M)",
};

static char whatthehell[] = "What the hell!";

struct resinf
{
	short id;
	short x;
	short y;
};

struct milres_parm
{
	struct xa_data_hdr h;

	void *modes;
	short curr_devid;

	short current[2];
	long misc[4];
	short count[8];

	short num_depths;
	struct widget_tree *depth_wt;
	struct widget_tree *col_wt[8];
	struct resinf *resinf[8];
// 	short *devids[8];

	POPINFO pinf_depth;
	POPINFO pinf_res;
};

static void _cdecl
delete_milres_parm(void *_p)
{
	struct milres_parm *p = _p;
	int i;

	if (p)
	{
		if (p->depth_wt)
		{
			remove_attachments(0, p->depth_wt->owner, p->depth_wt);
			p->depth_wt->links--;
			remove_wt(p->depth_wt, false);
			p->depth_wt = NULL;
		}
		for (i = 0; i < 8; i++)
		{
			if (p->col_wt[i])
			{
				p->col_wt[i]->links--;
				remove_wt(p->col_wt[i], false);
				p->col_wt[i] = NULL;
			}
		}
		kfree(p);
	}
}

static void
milan_reschg_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	short new_devid = -1;
	struct milres_parm *p = lookup_xa_data_byname(&wind->xa_data, "milres_parm");

	Sema_Up(clients);
	lock |= clients;

// 	wt->current = fr->obj;
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
		case RCHM_COL:
		{
			int i, o, newres = 1;
			POPINFO *pinf = object_get_popinfo(aesobj_ob(&fr->obj));
			struct widget_tree *pu_wt = NULL;

			for (i = 0, o = pinf->obnum; i < 8 && o >= 0; i++)
			{
				if (p->col_wt[i])
				{
					o--;
					if (!o)
					{
						int j;
						struct resinf *old, *new = NULL;

						old = p->resinf[p->current[0]];
						for (j = 0; j < p->count[p->current[0]]; j++, old++)
						{
							if (old->id == p->current[1])
							{
								new = old;
								break;
							}
						}
						if (new)
						{
							old = new;

							new = p->resinf[i];
							for (j = 0; j < p->count[i]; j++)
							{
								if (new[j].x == old->x && new[j].y == old->y)
								{
									newres = j + 1;
									break;
								}
							}
						}

						pu_wt = p->col_wt[i];
						new_devid = new[newres - 1].id;

						p->current[0] = i;
						break;
					}
				}
			}
			if (pu_wt)
			{
				pinf = &p->pinf_res;
				pinf->tree = pu_wt->tree;
				pinf->obnum = newres;
				obj_set_g_popup(wt, aesobj(wt->tree, RCHM_RES), pinf);
				obj_draw(wt, wind->vdi_settings, aesobj(wt->tree, RCHM_RES), -1, NULL, wind->rect_list.start, 0);
				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_RES:
		{
			POPINFO *pinf = object_get_popinfo(aesobj_ob(&fr->obj));
			struct widget_tree *pu_wt = p->col_wt[p->current[0]];

			if (pu_wt)
			{
				struct resinf *r = p->resinf[p->current[0]];

				new_devid = r[pinf->obnum - 1].id;

				p->current[1] = new_devid;
			}
			break;
		}
		case RCHM_OK:
		{
			unsigned long next_res;
			DIAGS(("reschange: restart"));

			object_deselect(wt->tree + RCHM_OK);
			redraw_toolbar(lock, wind, RCHM_OK);
			next_res = p->current[1];
			next_res |= 0x80000000;
			save_res(next_res);
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			dispatch_shutdown(RESOLUTION_CHANGE, next_res);
			break;
		}
		case RCHM_CANCEL:
		{
			object_deselect(wt->tree + RCHM_CANCEL);
			redraw_toolbar(lock, wind, RCHM_CANCEL);

			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			break;
		}
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", fr->obj));
			break;
		}
	}

	Sema_Dn(clients);
}

static void *
nxt_mdepth(short item, void **data)
{
	int i;
	struct milres_parm *p = *data;
	short current = -1;
	void *ret;

	if (!item)
	{
		p->current[1] = p->current[0];
	}

	for (i = p->current[1]; i < 8; i++)
	{
		if (p->count[i])
		{
			current = i;
			break;
		}
	}
	if (current == -1)
	{
		return whatthehell;
	}
	else
	{
		ret = coldepths[current];
		p->current[1] = current + 1;
	}

	return ret;
};

static char idx2planes[] =
{
	1,2,4,8,15,16,24,32
};
static void *
nxt_mres(short item, void **data)
{
	struct milres_parm *p = *data;
	struct videodef *modes;
	short planes;
	long num_modes;
	void *ret = NULL;

	if (!item)
	{
		p->misc[2] = p->misc[0];
		p->misc[3] = p->misc[1];
		p->current[1] = p->current[0];
	}

	planes = idx2planes[p->current[1]];

	modes = (struct videodef *)p->misc[2];
	num_modes = p->misc[3];

	while (num_modes > 0)
	{
		if (modes->planes == planes)
		{
			struct resinf *r = p->resinf[p->current[1]];

			p->misc[2] = (long)(modes + 1);
			p->misc[3] = num_modes - 1;
			r[item].id = modes->devid;
			r[item].x  = modes->res_x;
			r[item].y  = modes->res_y;
			ret = modes->name;
			break;
		}
		num_modes--;
		modes++;
	}
	return ret;
}
static int
instchrm_wt(struct xa_client *client, struct widget_tree **wt, OBJECT *obtree)
{
	int ret = 0;

	if (obtree)
	{
		*wt = new_widget_tree(client, obtree);
		if (*wt)
		{
			(*wt)->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
			ret = 1;
		}
		else
		{
			free_object_tree(XAPI->C->Aes, obtree);
		}
	}
	else
		*wt = NULL;

	return ret;
}

static short
count_milan_res(long num_modes, short planes, struct videodef *modes)
{
	short count = 0;
// 	bool p = false;

	while (num_modes--)
	{
		if (modes->planes == planes)
			count++;
#if 0
		if (count == 1 && !p)
		{
			short i, *m;
			display("Planes %d", planes);
			m = (short *)modes;
			for (i = (66/2); i < (106/2); i++)
				display("%02d, %04x, %05d", i, m[i], m[i]);
			p = true;
		}
#endif
		modes++;
	}
	return count;
}

static struct milres_parm *
check_milan_res(struct xa_client *client, short mw)
{
	int i, j;
	short currmode = 0;
	struct milres_parm *p = NULL;
	long num_modes, ret;

	num_modes = mvdi_device(mvdi, 0, 0, DEVICE_GETDEVICE, &ret);

	if (num_modes >= 0)
	{
		currmode = ((struct videodef *)ret)->devid;
	}

	num_modes = mvdi_device(mvdi, 0, 0, DEVICE_GETDEVICELIST, &ret);

	if (num_modes > 0)
	{
		struct videodef *modes = (struct videodef *)ret;
		short depths = 0, devids = 0;
		OBJECT *obtree;
		short count[8];

		count[0] = count_milan_res(num_modes,  1, modes);
		count[1] = count_milan_res(num_modes,  2, modes);
		count[2] = count_milan_res(num_modes,  4, modes);
		count[3] = count_milan_res(num_modes,  8, modes);
		count[4] = count_milan_res(num_modes, 15, modes);
		count[5] = count_milan_res(num_modes, 16, modes);
		count[6] = count_milan_res(num_modes, 24, modes);
		count[7] = count_milan_res(num_modes, 32, modes);

		for (i = 0; i < 8; i++)
		{
			if (count[i])
			{
				devids += count[i] + 1;
				depths++;
			}
		}

		if (depths)
		{
			union { void **vp; struct milres_parm **mp;} ptrs;
			struct resinf *r;

			if (!(p = kmalloc(sizeof(*p) + (sizeof(*r) * devids))))
				goto exit;

			bzero(p, sizeof(*p));

			ptrs.mp = &p;

			p->curr_devid = currmode;
			r = (struct resinf *)((char *)p + sizeof(*p));
			for (i = 0; i < 8; i++)
			{
				if ((p->count[i] = count[i]))
				{
					p->resinf[i] = r;
					r += p->count[i];
				}
			}
			p->num_depths = depths;
			p->current[0] = 0;
			obtree = create_popup_tree(client, 0, depths, mw, 4, &nxt_mdepth, ptrs.vp);
			if (!instchrm_wt(client, &p->depth_wt, obtree))
				goto exit;

			p->depth_wt->links++;

			for (i = 0,j = 1; i < 8; i++)
			{
				if (p->count[i])
				{
					p->current[0] = i;
					p->misc[0] = (long)modes;
					p->misc[1] = num_modes;
					obtree = create_popup_tree(client, 0, p->count[i], mw, 4, &nxt_mres, ptrs.vp);
					if (instchrm_wt(client, &p->col_wt[i], obtree))
					{
						p->col_wt[i]->links++;
					}
					else
						goto exit;
					j++;
				}
			}
		}
	}
	else
	{
exit:
		delete_milres_parm(p);
		p = NULL;
	}
	return p;
}

static short
milan_setdevid(struct widget_tree *wt, struct milres_parm *p, short devid)
{
	int i, j, depth_idx = 0, res_idx = -1;
	short found_devid = 0, current;
	struct widget_tree *first = NULL, *pu_wt = NULL;

	for (i = 0; i < 8; i++)
	{
		if (p->col_wt[i])
		{
			if (!first)
			{
				first = p->col_wt[i];
				found_devid = (*p->resinf[i]).id;
				current = i;
			}

			for (j = 0; j < p->count[i]; j++)
			{
				struct resinf *r = p->resinf[i];

				if (r[j].id == devid)
				{
					pu_wt = p->col_wt[i];
					res_idx = j + 1;
					found_devid = devid;
					current = i;
				}
			}
			if (res_idx != -1)
				break;

			depth_idx++;
		}
	}
	p->pinf_depth.tree = p->depth_wt->tree;
	p->current[0] = i;
	if (res_idx != -1)
	{
		p->pinf_depth.obnum = depth_idx + 1;
		p->pinf_res.tree = pu_wt->tree;
		p->pinf_res.obnum = res_idx;
	}
	else
	{
		p->pinf_depth.obnum = 1;
		p->pinf_res.tree = first->tree;
		p->pinf_res.obnum = 1;
	}
	obj_set_g_popup(wt, aesobj(wt->tree, RCHM_COL), &p->pinf_depth);
	obj_set_g_popup(wt, aesobj(wt->tree, RCHM_RES), &p->pinf_res);
	return found_devid;
}

static int
reschg_destructor(enum locks lock, struct xa_window *wind)
{
	if (our_wind)
		our_wind = NULL;
	return true;
}

static void
open_milan_reschange(enum locks lock, struct xa_client *client, bool open)
{
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	struct milres_parm *p = NULL;

	if (!our_wind)
	{
		if (!our_rshdr) {
			char *rsc_fn;
			rsc_fn = make_fqfname(our_path, RSCFILE);
			if (!rsc_fn) goto fail;
			BLOG((false, "rcmilan: attempt to load rsc'%s'", rsc_fn));
			our_rshdr = load_resource(client, rsc_fn, NULL, 16,16, false);
			kfree(rsc_fn);
			BLOG((false, "rcmilan: resource at %lx", our_rshdr));
			if (!our_rshdr) goto fail;
		} else
			BLOG((false, "rcmilan: rsc already loaded"));

		obtree = resource_tree(our_rshdr, RES_CHMIL);
		if (!obtree) goto fail;
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		BLOG((false, "rcmilan: obtree %lx, wt %lx", obtree, wt));
		wt->flags |= WTF_AUTOFREE;
		p = check_milan_res(client, obtree[RCHM_RES].ob_width);
		if (!p) goto fail;

		p->current[1] = milan_setdevid(wt, p, p->curr_devid);

		our_wind = create_dwind(lock, CLOSER, t_reschg, client, wt, milan_reschg_form_exit, reschg_destructor);
		if (!our_wind) goto fail;

		add_xa_data(&our_wind->xa_data, p, 0, "milres_parm", delete_milres_parm);
		if (open)
			open_window(lock, our_wind, our_wind->r);
	}
	else
	{
		if (open)
		{
			open_window(lock, our_wind, our_wind->r);
			if (our_wind != TOP_WINDOW/*window_list*/)
				top_window(lock, true, false, our_wind);
		}
	}

	return;
fail:
	delete_milres_parm(p);

	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (our_rshdr) {
		free_resource(NULL, NULL, our_rshdr);
		our_rshdr = NULL;
	}
}
