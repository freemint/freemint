/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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

/*
 * Kernal Message Handler
 */

#ifndef _kernel_h 
#define _kernel_h


#if VECTOR_VALIDATION
bool xa_invalid(int which, int pid, void *addr, long size, bool allownil);
#endif

void remove_client_handle(XA_CLIENT *client);
void insert_client_handle(XA_CLIENT *client);
void close_client(LOCK lock, XA_CLIENT *client);
void pending_client_exit(XA_CLIENT *client);

void Block(XA_CLIENT *client, int which);
void Unblock(XA_CLIENT *client, unsigned long value, int which);

void get_mouse(short which);

void XaAES(void);
void setup_k_function_table(void);

Tab *new_task(Tab *new);
void free_task(Tab *, int *);


AES_function
	XA_appl_init,
	XA_appl_exit,
	XA_appl_search,
	XA_appl_write,
	XA_appl_yield,
	XA_new_client,
	XA_client_exit,
	XA_appl_control,
	XA_appl_getinfo,
	XA_appl_find,
	XA_evnt_button,
	XA_evnt_keybd,
	XA_evnt_mesag,
	XA_evnt_mouse,
	XA_evnt_multi,
	XA_evnt_timer,
	XA_wind_create,
	XA_wind_open,
	XA_wind_close,
	XA_wind_find,
	XA_wind_set,
	XA_wind_get,
	XA_wind_update,
	XA_wind_delete,
	XA_wind_new,
	XA_wind_calc,
	XA_graf_mouse,
	XA_graf_handle,
	XA_graf_mkstate,
	XA_graf_rubberbox,
	XA_graf_dragbox,
	XA_graf_watchbox,
	XA_graf_growbox,
	XA_graf_shrinkbox,
	XA_graf_movebox,
	XA_graf_slidebox,
	XA_rsrc_load,
	XA_rsrc_free,
	XA_rsrc_gaddr,
	XA_rsrc_obfix,
	XA_rsrc_rcfix,
	XA_objc_draw,
	XA_objc_offset,
	XA_objc_find,
	XA_objc_change,
	XA_objc_add,
	XA_objc_delete,
	XA_objc_order,
	XA_objc_edit,
	XA_objc_sysvar,
	XA_form_center,
	XA_form_dial,
	XA_form_button,
	XA_form_alert,
	XA_form_do,
	XA_form_error,
	XA_form_keybd,
	XA_menu_bar,
	XA_menu_tnormal,
	XA_menu_ienable,
	XA_menu_icheck,
	XA_menu_text,
	XA_menu_register,
	XA_menu_popup,
	XA_menu_attach,
	XA_menu_istart,
	XA_menu_settings,
	XA_shell_write,
	XA_shell_read,
	XA_shell_find,
	XA_shell_envrn,
	XA_appl_pipe,
	XA_scrap_read,
	XA_scrap_write,
	XA_form_popup,
	XA_fsel_input,
	XA_fsel_exinput;

#if WDIAL
AES_function
	XA_wdlg_create,
	XA_wdlg_open,
	XA_wdlg_close,
	XA_wdlg_delete,
	XA_wdlg_get,
	XA_wdlg_set,
	XA_wdlg_event,
	XA_wdlg_redraw;
#endif

#if LBOX
AES_function
	XA_lbox_create,
	XA_lbox_update,
	XA_lbox_do,
	XA_lbox_delete,
	XA_lbox_get,
	XA_lbox_set;
#endif

#endif /* _kernel_h */
