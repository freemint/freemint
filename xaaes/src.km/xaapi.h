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

#ifndef _XAAPI_H_
#define _XAAPI_H_

#include "xa_types.h"

struct xa_objc_api
{
	RSHDR * _cdecl		(*load_resource)	(struct xa_client *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal);
	void _cdecl		(*free_resource)	(struct xa_client *client, AESPB *pb, RSHDR *thisrsc);
	OBJECT * _cdecl		(*resource_tree)	(RSHDR *rsc, long num);

	void _cdecl		(*obfix)		(OBJECT *obtree, short obj, short designWidth, short desighHeight);

	OBJECT * _cdecl		(*duplicate_obtree)	(struct xa_client *client, OBJECT *tree, short start);
	void _cdecl		(*free_object_tree)	(struct xa_client *cleint, OBJECT *tree);

	void _cdecl			(*init_widget_tree)	(struct xa_client *client, struct widget_tree *wt, OBJECT *obtree);
	struct widget_tree * _cdecl	(*new_widget_tree)	(struct xa_client *client, OBJECT *obtree);
	struct widget_tree * _cdecl	(*obtree_to_wt)		(struct xa_client *client, OBJECT *obtree);
	bool _cdecl			(*remove_wt)		(struct widget_tree *wt, bool force);

	/* OBJECT specific functions */
	OBSPEC *	_cdecl (*object_get_spec)	(OBJECT *ob);
	void 		_cdecl (*object_set_spec)	(OBJECT *ob, unsigned long cl);
	POPINFO *	_cdecl (*object_get_popinfo)	(OBJECT *ob);
	TEDINFO *	_cdecl (*object_get_tedinfo)	(OBJECT *ob, XTEDINFO **x);
	void		_cdecl (*object_spec_wh)	(OBJECT *ob, short *w, short *h);

	void		_cdecl	(*ob_spec_xywh)		(OBJECT *tree, short obj, RECT *r);
	void		_cdecl	(*render_object)	(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_aes_object item, short px, short py);
	CICON *		_cdecl	(*getbest_cicon)	(CICONBLK *ciconblk);
	short		_cdecl	(*obj_offset)		(struct widget_tree *wt, struct xa_aes_object object, short *mx, short *my);
	void		_cdecl	(*obj_rectangle)	(struct widget_tree *wt, struct xa_aes_object object, RECT *r);

	void 		_cdecl	(*obj_set_radio_button)	(struct widget_tree *wt,
							 struct xa_vdi_settings *v,
							 struct xa_aes_object obj,
							 bool redraw,
							 const RECT *clip,
							 struct xa_rect_list *rl);
	struct xa_aes_object _cdecl (*obj_get_radio_button)	(struct widget_tree *wt,
								struct xa_aes_object parent,
								short state);

	void 		_cdecl	(*obj_draw)		(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object obj,
				  			 int transdepth, const RECT *clip, struct xa_rect_list *rl, short flags);

	void 		_cdecl	(*obj_change)		(XA_TREE *wt,
				  			 struct xa_vdi_settings *v,
	  						 struct xa_aes_object obj,
							 int transdepth, short state, short flags,
							 bool redraw,
							 const RECT *clip,
							 struct xa_rect_list *rl, short dflags);
	short 		_cdecl	(*obj_edit)		(XA_TREE *wt,
							 struct xa_vdi_settings *v,
							 short func,
							 struct xa_aes_object obj,
							 short keycode,
							 short pos,	/* -1 sets position to end of text */
							 char *string,
							 bool redraw,
							 const RECT *clip,
							 struct xa_rect_list *rl,
							/* outputs */
							 short *ret_pos,
							 struct xa_aes_object *ret_obj);

	void _cdecl	(*obj_set_g_popup)		(XA_TREE *swt, struct xa_aes_object sobj, POPINFO *pinf);
	void _cdecl	(*obj_unset_g_popup)		(XA_TREE *swt, struct xa_aes_object sobj, char *txt);

	OBJECT * _cdecl	(*create_popup_tree)		(struct xa_client *client, short type, short nobjs,
							 short min_w, short min_h,
							 void *(*cb)(short item, void **data), void **data);

	void *		reserved[64 - 28];
};
struct xa_menu_api
{

	void _cdecl	(*remove_attachments)(enum locks lock, struct xa_client *client, XA_TREE *wt);

	void *		reserved[64 - 1];
};

struct xa_debug_api
{
	void	_cdecl		(*display)		(const char *fmt, ...);
	void	_cdecl		(*ndisplay)		(const char *fmt, ...);
	void	_cdecl		(*bootlog)		(bool disp, const char *fmt, ...);

	void	_cdecl		(*diags)		(const char *t, ...);

	void *			reserved[16 - 4];
};

/*
 * module_register() modes..
 */
#define MODREG_UNREGISTER	0x80000000
#define MODREG_KERNKEY		0x00000001
#define MODREG_SETUPVDI		0x00000002
//#define MODREG_RESCHANGE	0x00000003
struct xa_module_api
{
	long	_cdecl		(*module_register)	(long type, void *data);
	void *			reserved[16 - 1];
};
/*
 * This api provides entrypoints for device drivers.
 * This is the interface to extend for other devices
 * such as keyboard, etc.
 */
struct xa_device_input_api
{
	void (*mouse_move)		(short x, short y);
	void (*mouse_button)		(struct moose_data *md);
	void (*mouse_wheel)		(struct moose_data *md);

	void *				reserved[64 - 1];
};

struct xa_xad_api
{
	struct xad *	(*xad_name2adi)		(char *aname);
	short		(*xad_getfreeunit)	(char *name);
	long		(*xad_register)		(struct xad *a);
	long		(*xad_unregister)	(struct xad *a);
	long		(*xad_open)		(struct xad *a);
	long		(*xad_close)		(struct xad *a);
	long		(*xad_ioctl)		(struct xad *a, short cmd, long arg);

	void *		reserved[32 - 7];
};

struct xa_window_api
{
	struct xa_window * _cdecl (*create) (enum locks lock,
						SendMessage *message_handler,
						DoWinMesag *message_doer,
						struct xa_client *client,
						bool nolist,
						XA_WIND_ATTR tp,
						WINDOW_TYPE dial,
						int frame,
						bool thinwork,
						const RECT R,
						const RECT *max,
						RECT *remember);

	int _cdecl	(*open)			(enum locks lock, struct xa_window *wind, RECT r);
	bool _cdecl	(*close)		(enum locks lock, struct xa_window *wind);
	void _cdecl	(*move)			(enum locks lock, struct xa_window *wind, bool blit, WINDOW_STATUS newstate, short x, short y, short w, short h);
	void _cdecl	(*top)			(enum locks lock, bool snd_untopped, bool snd_ontop, struct xa_window *wind);
	void _cdecl	(*bottom)		(enum locks lock, bool snd_untopped, bool snd_ontop, struct xa_window *wind);

	void _cdecl	(*send_to_bottom)	(enum locks lock, struct xa_window *wind);
	void _cdecl	(*delete)		(enum locks lock, struct xa_window *wind);
	void _cdecl	(*delayed_delete)	(enum locks lock, struct xa_window *wind);

	struct xa_window * _cdecl (*create_dwind)(enum locks lock, XA_WIND_ATTR tp, char *title, struct xa_client *client, struct widget_tree *wt, FormExit(*f), WindowDisplay(*d));

	void _cdecl	(*redraw_toolbar)	(enum locks lock, struct xa_window *wind, short item);

	void * _cdecl	(*rp2ap)		(struct xa_window *wind, struct xa_widget *widg, RECT *r);
	void   _cdecl	(*rp2apcs)		(struct xa_window *wind, struct xa_widget *widg, RECT *r);

	void *		reserved[64 - 13];
};


struct xa_rectangle_api
{
	short _cdecl	(*clip)		(RECT *s, RECT *d, RECT *r);
};


struct xa_lib_api
{
	char * _cdecl	(*sysfile)		(const char *fname);
	void * _cdecl	(*kmalloc)		(size_t size, const char *f);
	void * _cdecl	(*umalloc)		(size_t size, const char *f);
	void _cdecl	(*kfree)		(void *addr, const char *f);
	void _cdecl	(*ufree)		(void *addr, const char *f);

	void _cdecl	(*bclear)		(void *addr, unsigned long len);

	char * _cdecl	(*make_fqfname)		(const char *path, const char *file);

	void *		reserved[64 - 7];
};

struct xa_xadata_api
{
	void * _cdecl	(*lookup)		(struct xa_data_hdr **l,    void *_data);
	void * _cdecl	(*lookup_byid)		(struct xa_data_hdr **l,    long id);
	void * _cdecl	(*lookup_byname)	(struct xa_data_hdr **l,    char *name);
	void * _cdecl	(*lookup_byidname)	(struct xa_data_hdr **l,  long id, char *name);
	void _cdecl	(*add)			(struct xa_data_hdr **list, void *_data, long id, char *name, void _cdecl(*destruct)(void *d));
	void _cdecl	(*remove)		(struct xa_data_hdr **list, void *_data);
	void _cdecl	(*delete)		(struct xa_data_hdr **list, void *_data);
	void _cdecl	(*ref)			(struct xa_data_hdr **list, void *_data, short count);
	long _cdecl	(*unref)		(struct xa_data_hdr **list, void *_data, short flags);
	void _cdecl	(*free_list)		(struct xa_data_hdr **list);

	void *		reserved[16 - 10];
};

struct xa_img_api
{
	void _cdecl	(*load_img)		(char *fname, XAMFDB *result);

	void *		reserved[16 - 1];
};

struct xa_api
{
	unsigned char	version_major;	/* xa_api major version */
	unsigned char	version_minor;	/* xa_api kentry minor version */
	unsigned short	status;		/* XaAES status */

	char name[32];

	struct config *cfg;
	struct common *C;
	struct shared *S;
	struct kentry *k;

	void _cdecl	(*dispatch_shutdown)	(short flags, unsigned long arg);

	struct xa_debug_api		debug;
	struct xa_module_api		module;
	struct xa_device_input_api	device_input;
	struct xa_xad_api		xad;
	struct xa_objc_api		objc;
	struct xa_menu_api		menu;
	struct xa_rectangle_api		rect;
	struct xa_lib_api		lib;
	struct xa_xadata_api		xadata;
	struct xa_window_api		wind;

	struct xa_img_api		img;
};

#define DEFAULTS_xa_objc_api	\
{ \
	LoadResources,		\
	FreeResources,		\
	ResourceTree,		\
	obfix,			\
	duplicate_obtree,	\
	free_object_tree,	\
	init_widget_tree,	\
	new_widget_tree,	\
	obtree_to_wt,		\
	remove_wt,		\
	api_object_get_spec,	\
	api_object_set_spec,	\
	api_object_get_popinfo,	\
	api_object_get_tedinfo,	\
	api_object_spec_wh,	\
	api_ob_spec_xywh,	\
	api_render_object,	\
	api_getbest_cicon,	\
	api_obj_offset,		\
	api_obj_rectangle,	\
	obj_set_radio_button,	\
	obj_get_radio_button,	\
	obj_draw,		\
	obj_change,		\
	obj_edit,		\
	obj_set_g_popup,	\
	obj_unset_g_popup,	\
	create_popup_tree,	\
}

#define DEFAULTS_xa_menu_api	\
{ \
	remove_attachments,	\
}

#define DEFAULTS_xa_debug_api	\
{ \
	display,		\
	ndisplay,		\
	bootlog,		\
	diags,			\
}

#define DEFAULTS_xa_module_api	\
{ \
	module_register,	\
}

#define DEFAULTS_xa_devinput_api \
{				\
	adi_move,		\
	adi_button,		\
	adi_wheel,		\
}

#define DEFAULTS_xa_xad_api 	\
{				\
	xad_name2adi,		\
	xad_getfreeunit,	\
	xad_register,		\
	xad_unregister,		\
	xad_open,		\
	xad_close,		\
	xad_ioctl,		\
}

#define DEFAULTS_xa_window_api	\
{ \
	create_window,		\
	open_window,		\
	close_window,		\
	move_window,		\
	top_window,		\
	bottom_window,		\
	send_wind_to_bottom,	\
	delete_window,		\
	delayed_delete_window,	\
	create_dwind,		\
	redraw_toolbar,		\
	api_rp2ap,		\
	api_rp2apcs,		\
}

#define DEFAULTS_xa_rectangle_api \
{ \
	api_rect_clip,		\
}

#define DEFAULTS_xa_lib_api	\
{ \
	xaaes_sysfile,		\
	xaaes_kmalloc,		\
	xaaes_umalloc,		\
	xaaes_kfree,		\
	xaaes_ufree,		\
	api_bzero,		\
	make_fqfname,		\
}

#define DEFAULTS_xa_xadata_api	\
{ \
	api_lookup_xa_data,		\
	api_lookup_xa_data_byid,	\
	api_lookup_xa_data_byname,	\
	api_lookup_xa_data_byidname,	\
	api_add_xa_data,		\
	api_remove_xa_data,		\
	api_delete_xa_data,		\
	api_ref_xa_data,		\
	api_deref_xa_data,		\
	api_free_xa_data_list,		\
}

#define DEFAULTS_xa_img_api	\
{ \
	api_load_img,		\
}

#define DEFAULTS_xa_api			\
{ \
	0,				\
	1,				\
	'b',				\
	"XaAES API v0.1",		\
	&cfg,				\
	&C,				\
	&S,				\
	NULL, /*kentry*/		\
	\
	dispatch_shutdown,		\
	\
	DEFAULTS_xa_debug_api,		\
	DEFAULTS_xa_module_api,		\
	DEFAULTS_xa_devinput_api,	\
	DEFAULTS_xa_xad_api,		\
	DEFAULTS_xa_objc_api,		\
	DEFAULTS_xa_menu_api,		\
	DEFAULTS_xa_rectangle_api,	\
	DEFAULTS_xa_lib_api,		\
	DEFAULTS_xa_xadata_api,		\
	DEFAULTS_xa_window_api,		\
	DEFAULTS_xa_img_api,		\
}

#endif /* _XAAPI_H_ */
