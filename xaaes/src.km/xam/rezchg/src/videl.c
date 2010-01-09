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
 * a resolution change dialog for plain Videl based
 * setups, such as Falcons.
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
#include "xaaes_module.h"

#include "ob_inlines.h"
#include "ssystem.h"
#include "cookie.h"

#include "reschg.h"

#define RESFILE "rcvidel.res"
#define RSCFILE "rcvidel.rsc"

long _cdecl init(struct kernel_module *km, void *arg, struct device_methods ***r);

struct xa_api *XAPI = NULL;
struct kentry *KENTRY = NULL;

static struct xa_window *our_wind = NULL;
static RSHDR *our_rshdr = NULL;
static const char *our_path = NULL;

static char t_reschg[] = " Change Resolution ";

//static char nofalc[] = "Need Videl (Falcon 030)";
static void _cdecl open_falcon_reschange(enum locks lock, struct xa_client *client, bool open);

static long _cdecl setupvdi(unsigned long vm, short *work_in, short *work_out, short *retmode);

static long _cdecl probe(device_t dev);
static long _cdecl attach(device_t dev);
static long _cdecl detach(device_t dev);
static long _cdecl sr_attach(device_t dev);
static long _cdecl sr_detach(device_t dev);

DEVICE_METHODS(devmethods, NULL, probe, attach, detach);
DEVICE_METHODS(setres_methods, NULL, probe, sr_attach, sr_detach);

static struct device_methods *ourdms[] =
{
	&devmethods, &setres_methods, NULL
};
/*
 * Called by the the kernel module framework. This is where the
 * module should check if it can run on the current hardware
 * and return appropriate error code. The kernel will unload
 * the module on any error.
 *
 * For this module, we check if the machine has a videl...
 */
static long _cdecl
probe(device_t dev)
{
	unsigned long tmp;

	if (XAPI && !strcmp("XaAES API v0.1", XAPI->name)) {
		if (!(s_system(S_GETCOOKIE, COOKIE__VDO, (unsigned long)(&tmp)))) {
			long t = (tmp >> 16);
			if (t == 3) {
				if (!our_path)
					our_path = dev->km->fpath;
				return E_OK;
			}
		}
	}
	return ENXIO;
}

/* XaAES specific
 * Structure used to bind an action to a key.
 */
static struct register_kernkey_parms kk =
{ 'R', 0, open_falcon_reschange	};


/*
 * This is called when the respective module framework
 * wants the module to attach itself.
 * For this module, this would be XaAES's module framework.
 */
static long _cdecl
attach(device_t dev)
{
	long err = ENXIO;
	/*
	 * Bind an action to a kernel key.
	 * This tells XaAES to call open_falcon_reschange when
	 * user press ctrl-alt-r
	 */
	err = module_register(MODREG_KERNKEY, &kk);
	return err;
}
/*
 * detach is called when the module framework wants the
 * module to detach. This normally happens before a module
 * unload
 */
static long _cdecl
detach(device_t dev)
{
	/*
	 * Unregister with XaAES's module framework.
	 */
	module_register(MODREG_KERNKEY | MODREG_UNREGISTER, &kk);
	/*
	 * Close window if we have one open
	 */
	if (our_wind) {
		close_window(0, our_wind);
		delayed_delete_window(0, our_wind);
		free_resource(NULL, NULL, our_rshdr);
		our_rshdr = NULL;
	}
	our_path = NULL;
	return E_OK;
}

static long _cdecl
sr_attach(device_t dev)
{
	return module_register(MODREG_SETUPVDI, setupvdi);
}
static long _cdecl
sr_detach(device_t dev)
{
	module_register(MODREG_SETUPVDI | MODREG_UNREGISTER, setupvdi);
	our_path = NULL;
	return E_OK;
}
/*
 * The entrypoint called by the AES during module load
 */
long _cdecl
init(struct kernel_module *km, void *arg, struct device_methods ***r)
{
	kentry = km->kentry;
	if (check_kentry_version())
		return 1;
	XAPI = arg;
	*r = ourdms;
	return E_OK;
}

static void
save_res(unsigned long res)
{
	struct file *fp;
	char *f;

	f = make_fqfname(our_path, RESFILE);
	if (f) {
		BLOG((false, "rcvidel: save settigns to '%s'", f));
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
	unsigned long r;

	if (res) {
		char *f = make_fqfname(our_path, RESFILE);
		if (f) {
			BLOG((false, "rcvidel: load settings from '%s'", f));
			fp = kernel_open(RESFILE, O_RDONLY, NULL,NULL);
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

static short
vcheckmode(short mode)
{
	short ret;

	__asm__ volatile
	(
		"move.w		%1,-(sp)\n\t"		\
		"move.w		#0x5f,-(sp)\n\t"	\
		"trap		#14\n\t"		\
		"addq.w		#4,sp\n\t"		\
		"move.w		d0,%0\n\t"		\
		: "=d"(ret)
		: "d"(mode)
		: "d0","d1","d2","a0","a1","a2","memory"
	);
	return ret;
}

/* ************************************************************ */
/*     Falcon resolution mode change functions			*/
/* ************************************************************ */
static long _cdecl
setupvdi(unsigned long vm, short *work_in, short *work_out, short *retmode)
{
	short nvmode;

	if (load_res(&vm) == E_OK || (vm)) {
		nvmode = (short)vm;

		/* Ronald Andersson:
		 * This should be the method for falcon!
		 */
		nvmode = vcheckmode(vm);
		if ((nvmode & (1 << 4)) &&	/* VGA_FALCON */
		    (nvmode & 7) == 4)		/* is 16bit */
		{
			nvmode &= ~(1 << 3);		/* Set 320 pixels */
		}

		work_out[45] = nvmode;
		*retmode = 5;
	}
	return E_OK;
}

static void
set_reschg_obj(XA_TREE *wt, unsigned long res)
{
	OBJECT *obtree = wt->tree;
	short obj;
	struct xa_vdi_settings *v = wt->owner->vdi_settings;

	obj = res & 0x7;
	if (obj > 4)
		obj = 4;
	obj += RC_COLOURS + 1;
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	obj = RC_COLUMNS + 1 + ((res & (1<<3)) ? 1 : 0);
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	obj = RC_VGA + 1 + ((res & (1<<4)) ? 1 : 0);
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	obj = RC_TVSEL + 1 + ((res & (1<<5)) ? 1 : 0);
	obj_set_radio_button(wt, v, aesobj(wt->tree, obj), false, NULL, NULL);

	setsel(obtree + RC_OVERSCAN, (res & (1<<6)));
	setsel(obtree + RC_ILACE, (res & (1<<7)));
	setsel(obtree + RC_BIT15, (res & 0x8000));
}

static unsigned long
get_reschg_obj(XA_TREE *wt)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object obj;
	unsigned long res = 0L;

	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_COLOURS), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= aesobj_item(&obj) - 1 - RC_COLOURS;
	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_COLUMNS), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= (aesobj_item(&obj) - 1 - RC_COLUMNS) << 3;
	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_VGA), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= (aesobj_item(&obj) - 1 - RC_VGA) << 4;
	obj = obj_get_radio_button(wt, aesobj(wt->tree, RC_TVSEL), OS_SELECTED);
	if (valid_aesobj(&obj))
		res |= (aesobj_item(&obj) - 1 - RC_TVSEL) << 5;

	if (issel(obtree + RC_OVERSCAN)) res |= (1<<6);
	if (issel(obtree + RC_ILACE))	res |= (1<<7);
	if (issel(obtree + RC_BIT15))	res |= 0x8000;

	return res;
}

static void
reschg_form_exit(struct xa_client *Client,
		 struct xa_window *wind,
		 struct widget_tree *wt,
		 struct fmd_result *fr)
{
	enum locks lock = 0;
	Sema_Up(clients);
	lock |= clients;

	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
		case RC_OK:
		{
			unsigned long next_res;
			DIAGS(("rcvidel: reschange: restart"));

			object_deselect(wt->tree + RC_OK);
			redraw_toolbar(lock, wind, RC_OK);
			next_res = get_reschg_obj(wt);
			next_res |= 0x80000000;
			save_res(next_res);
			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			dispatch_shutdown(/*RESOLUTION_CHANGE*/ 0x0040, next_res);
			break;
		}
		case RC_CANCEL:
		{
			object_deselect(wt->tree + RC_CANCEL);
			redraw_toolbar(lock, wind, RC_CANCEL);

			/* and release */
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			break;
		}
		default:
		{
			DIAGS(("rcvidel: unhandled event %i", fr->obj));
			break;
		}
	}

	Sema_Dn(clients);
}

static int
reschg_destructor(enum locks lock, struct xa_window *wind)
{
	if (our_wind)
		our_wind = NULL;
	return true;
}

/*
 * This is what XaAES calls to open the resolution change dialog.
 */
static void _cdecl
open_falcon_reschange(enum locks lock, struct xa_client *client, bool open)
{
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	unsigned long res;

	if (!our_wind)
	{
		if (!our_rshdr) {
		char *rsc_fn;

			rsc_fn = make_fqfname(our_path, RSCFILE);
			if (!rsc_fn) {
				BLOG((true, "rcvidel: Could not locate 'rcvidel.rsc'!"));
				goto fail;
			}
			BLOG((false, "rcvidel: loading '%s'", rsc_fn));

			our_rshdr = load_resource(client, rsc_fn, NULL, 16, 16, false);
			kfree(rsc_fn);
			BLOG((false, "rcvidel: our_rshdr = %lx, client = %lx", our_rshdr, client));
			BLOG((false, "rcvidel: client = %s", client ? client->name : "What the hell?!"));
			if (!our_rshdr) goto fail;
		}
		obtree = duplicate_obtree(client, resource_tree(our_rshdr, RES_CHFALC), 0);
		BLOG((false, "rcvidel: obtree = %lx", obtree));
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		BLOG((false, "rcvidel: wt = %lx", wt));
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		BLOG((false, "rcvidel: creating wind..."));
		our_wind = create_dwind(lock, CLOSER, t_reschg, client, wt, reschg_form_exit, reschg_destructor);
		BLOG((false, "rcvidel: wind = %lx", our_wind));
		if (!our_wind) goto fail;

		BLOG((false, "rcvidel: set_reschg_obj"));

		if (load_res(&res) != E_OK)
			res = (unsigned long)XAPI->cfg->videomode;
		set_reschg_obj(wt, res);
		BLOG((false, "rcvidel: open = %d", open));
		if (open)
			open_window(lock, our_wind, our_wind->r);
		BLOG((false, "rcvidel: done!"));
	}
	else
	{
		if (open)
		{
			open_window(lock, our_wind, our_wind->r);
			if (!(our_wind->window_status & XAWS_FIRST)) // != TOP_WINDOW/*window_list*/)
				top_window(lock, true, false, our_wind);
		}
	}
	return;
fail:
	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
	if (our_rshdr) {
		free_resource(NULL,NULL, our_rshdr);
		our_rshdr = NULL;
	}
}
