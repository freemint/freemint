#include "xa_types.h"

#include "xa_global.h"

#include "util.h"

#include "k_init.h"
#include "k_main.h"
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

#include "obtree.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "xa_graf.h"
#include "trnfm.h"

#include "win_draw.h"
#include "render_obj.h"
#include "xa_xtobj.h"

#include "xaapi.h"

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
	bool unregister;

	unregister = (mode & 0x80000000) ? true : false;
	mode &= 0x7fffffff;

	BLOG((false, "module_register: %s mode = %lx, _p = %lx", unregister ? "unregister" : "register", mode, _p));

	switch (mode)
	{
		case MODREG_KERNKEY:
		{
			struct kernkey_entry *list;

			if (unregister)
			{
				struct kernkey_entry *this, *prev = NULL;
				struct register_kernkey_parms *p = _p;
				BLOG((false, "module_register: unregister KERNKEY"));
				list = C.kernkeys;
				while (list)
				{
					if (list->key == p->key && list->state == p->state) //if (list->key == this->key)
					{
						if (list->act == p->act) //if (list->act == this->act)
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
							while (list && list->act != p->act)
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
				BLOG((false, "module_register: register KERNKEY"));
				new = kmalloc(sizeof(*new));
				if (new)
				{
// 					struct kernkey_entry *p = _p;
					struct register_kernkey_parms *p = _p;

					new->next_key = NULL;
					new->next_act = NULL;
					new->act = p->act;
					new->key = p->key;
					new->state = p->state;

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
					ret = EERROR;
			}
			break;
		}
		case MODREG_SETUPVDI:
		{
			if (unregister) {
				BLOG((false, "module_register: unregister SETUPVDI (this %lx, current %lx)", _p, C.set_resolution));
				if (C.set_resolution == _p)
					C.set_resolution = NULL;
			} else {
				BLOG((false, "module_register: register SETUPVDI (old %lx, new %lx)", C.set_resolution, _p));
				C.set_resolution = _p;
			}
			break;
		}
#if 0
		case MODREG_RESCHANGE:
		{
			if (unregister) {
				if (C.reschange == _p)
					C.reschange = NULL;
			} else {
// 				if (C.reschange)
// 					ret = EERROR;
// 				else
					C.reschange = _p;
			}
			break;
		}
#endif
		default:
		{
			ret = EERROR;
			break;
		}
	}
	BLOG((false, "module_register: returning %ld", ret));
	return ret;
}

struct xa_api xa_api = DEFAULTS_xa_api;

void
setup_xa_api(void)
{
	xa_api.k = kentry;
}
