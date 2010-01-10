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

#ifndef _xaaes_module_h_
#define _xaaes_module_h_

#include "xad_defs.h"
#include "xaapi.h"

#define dispatch_shutdown	(*XAPI->dispatch_shutdown)

/*********************
 * objc functions.. *
 *********************/
#define load_resource		(*XAPI->objc.load_resource)
#define free_resource		(*XAPI->objc.free_resource)
#define resource_tree		(*XAPI->objc.resource_tree)
#define obfix			(*XAPI->objc.obfix)
#define duplicate_obtree	(*XAPI->objc.duplicate_obtree)
#define free_object_tree	(*XAPI->objc.free_object_tree)
#define init_widget_tree	(*XAPI->objc.init_widget_tree)
#define new_widget_tree		(*XAPI->objc.new_widget_tree)
#define obtree_to_wt		(*XAPI->objc.obtree_to_wt)
#define remove_wt		(*XAPI->objc.remove_wt)
#define object_get_spec		(*XAPI->objc.object_get_spec)
#define object_set_spec		(*XAPI->objc.object_set_spec)
#define object_get_popinfo	(*XAPI->objc.object_get_popinfo)
#define object_get_tedinfo	(*XAPI->objc.object_get_tedinfo)
#define object_spec_wh		(*XAPI->objc.object_spec_wh)
#define ob_spec_xywh		(*XAPI->objc.ob_spec_xywh)
#define render_object		(*XAPI->objc.render_object)
#define getbest_cicon		(*XAPI->objc.getbest_cicon)
#define obj_offset		(*XAPI->objc.obj_offset)
#define obj_rectangle		(*XAPI->objc.obj_rectangle)
#define obj_set_radio_button	(*XAPI->objc.obj_set_radio_button)
#define obj_get_radio_button	(*XAPI->objc.obj_get_radio_button)
#define obj_draw		(*XAPI->objc.obj_draw)
#define obj_change		(*XAPI->objc.obj_change)
#define obj_edit		(*XAPI->objc.obj_edit)
#define obj_set_g_popup		(*XAPI->objc.obj_set_g_popup)
#define obj_unset_g_popup	(*XAPI->objc.obj_unset_g_popup)
#define create_popup_tree	(*XAPI->objc.create_popup_tree)

#define remove_attachments	(*XAPI->menu.remove_attachments)

/*********************
 * Debug functions.. *
 *********************/
#define display			(*XAPI->debug.display)
#define ndisplay		(*XAPI->debug.ndisplay)
#undef BLOG
#define bootlog			(*XAPI->debug.bootlog)
#if BOOTLOG
#define BLOG(x)			bootlog x
#else
#define BLOG(x)
#endif

#define XADIAGS			(*XAPI->debug.diags)
#undef DIAGS
#if GENERATE_DIAGS
#define DIAGS(x)		XADIAGS x
#else
#define DIAGS(x)
#endif

/*********************
 * Module functions.. *
 *********************/
#define module_register		(*XAPI->module.module_register)
/****************************
 * Device Input functions.. *
 ***************************/
#define mouse_move		(*XAPI->device_input.mouse_move)
#define mouse_button		(*XAPI->device_input.mouse_button)
#define mouse_wheel		(*XAPI->device_input.mouse_wheel)
/****************************
 * XAD functions.. *
 ***************************/
#define xad_name2xad		(*XAPI->xad.xad_name2xad)
#define xad_getfreeunit		(*XAPI->xad.xad_getfreeunit)
#define xad_register		(*XAPI->xad.xad_register)
#define xad_unregister		(*XAPI->xad.xad_unregister)
#define xad_open		(*XAPI->xad.xad_open)
#define xad_close		(*XAPI->xad.xad_close)
#define xad_ioctl		(*XAPI->xad.xad_ioctl)
/*********************
 * window functions.. *
 *********************/
#define create_window		(*XAPI->wind.create)
#define open_window		(*XAPI->wind.open)
#define close_window		(*XAPI->wind.close)
#define move_window		(*XAPI->wind.move)
#define top_window		(*XAPI->wind.top)
#define bottom_window		(*XAPI->wind.bottom)
#define send_wind_to_bottom	(*XAPI->wind.send_to_bottom)
#define delete_window		(*XAPI->wind.delete)
#define delayed_delete_window	(*XAPI->wind.delayed_delete)
#define create_dwind		(*XAPI->wind.create_dwind)
#define redraw_toolbar		(*XAPI->wind.redraw_toolbar)
#define relpos2abspos		(*XAPI->wind.rp2ap)
#define relpos2abspos_cs	(*XAPI->wind.rp2apcs)

/*********************
 * rectangle functions.. *
 *********************/
#define rect_clip		(*XAPI->rect.clip)

/*********************
 * lib functions.. *
 *********************/
#define sysfile			(*XAPI->lib.sysfile)

#undef kmalloc
#define XAAES_KMALLOC		(*XAPI->lib.kmalloc)
#define kmalloc(size)		XAAES_KMALLOC(size, FUNCTION)

#undef umalloc
#define XAAES_UMALLOC		(*XAPI->lib.umalloc)
#define umalloc(size)		XAAES_UMALLOC(size, FUNCTION)

#undef kfree
#define XAAES_KFREE		(*XAPI->lib.kfree)
#define kfree(adr)		XAAES_KFREE(adr, FUNCTION)

#undef ufree
#define XAAES_UFREE		(*XAPI->lib.ufree)
#define ufree(adr)		XAAES_UFREE(adr, FUNCTION)

#undef bzero
#define bzero			(*XAPI->lib.bclear)

#define make_fqfname		(*XAPI->lib.make_fqfname)

/*********************
 * xa data object functions.. *
 *********************/
#define lookup_xa_data		(*XAPI->xadata.lookup)
#define lookup_xa_data_byid	(*XAPI->xadata.lookup_byid)
#define lookup_xa_data_byname	(*XAPI->xadata.lookup_byname)
#define lookup_xa_data_byidname	(*XAPI->xadata.lookup_byidname)
#define add_xa_data		(*XAPI->xadata.add)
#define remove_xa_data		(*XAPI->xadata.remove)
#define delete_xa_data		(*XAPI->xadata.delete)
#define ref_xa_data		(*XAPI->xadata.ref)
#define unref_xa_data		(*XAPI->xadata.unref)
#define free_xa_data_list	(*XAPI->xadata.free_list)

/*********************
 * image functions.. *
 *********************/
#define load_img		(*XAPI->img.load_img)

#endif /* _xaaes_module.h */
