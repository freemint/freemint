/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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

#include "op_names.h"

#if GENERATE_DIAGS
const char *op_code_names[] = 
{
	"",					/*  0 */
	"",					/*  1 */
	"",					/*  2 */
	"",					/*  3 */
	"",					/*  4 */
	"",					/*  5 */
	"",					/*  6 */
	"",					/*  7 */
	"",					/*  8 */
	"",					/*  9 */
	"appl_init",				/* 10 */
	"appl_read",				/* 11 */
	"appl_write",				/* 12 */
	"appl_find",				/* 13 */
	"appl_tplay",				/* 14 */
	"appl_trecord",				/* 15 */
	"",					/* 16 */
	"",					/* 17 */
	"appl_search",				/* 18 */
	"appl_exit",				/* 19 */
	"evnt_keybd",				/* 20 */
	"evnt_button",				/* 21 */
	"evnt_mouse",				/* 22 */
	"evnt_mesag",				/* 23 */
	"evnt_timer",				/* 24 */
	"evnt_multi",				/* 25 */
	"evnt_dclick",				/* 26 */
	"",					/* 27 */
	"",					/* 28 */
	"",					/* 29 */
	"menu_bar",				/* 30 */
	"menu_icheck",				/* 31 */
	"menu_ienable",				/* 32 */
	"menu_tnormal",				/* 33 */
	"menu_text",				/* 34 */
	"menu_register",			/* 35 */
	"menu_popup",				/* 36 */
	"menu_attach",				/* 37 */
	"menu_istart",				/* 38 */
	"menu_settings",			/* 39 */
	"objc_add",				/* 40 */
	"objc_delete",				/* 41 */
	"objc_draw",				/* 42 */
	"objc_find",				/* 43 */
	"objc_offset",				/* 44 */
	"objc_order",				/* 45 */
	"objc_edit",				/* 46 */
	"objc_change",				/* 47 */
	"objc_sysvar",				/* 48 */
	"", 					/* 49 */
	"form_do",				/* 50 */
	"form_dial",				/* 51 */
	"form_alert",				/* 52 */
	"form_error",				/* 53 */
	"form_center",				/* 54 */
	"form_keybd",				/* 55 */
	"form_button",				/* 56 */
	"",					/* 57 */
	"",					/* 58 */
	"",					/* 59 */
	"",					/* 60 */
	"",					/* 61 */
	"",					/* 62 */
	"",					/* 63 */
	"",					/* 64 */
	"",					/* 65 */
	"",					/* 66 */
	"",					/* 67 */
	"",					/* 68 */
	"",					/* 69 */
	"graf_rubberbox",			/* 70 */
	"graf_dragbox",				/* 71 */
	"graf_movebox",				/* 72 */
	"graf_growbox",				/* 73 */
	"graf_shrinkbox",			/* 74 */
	"graf_watchbox",			/* 75 */
	"graf_slidebox",			/* 76 */
	"graf_handle",				/* 77 */
	"graf_mouse",				/* 78 */
	"graf_mkstate",				/* 79 */
	"scrap_read",				/* 80 */
	"scrap_write",				/* 81 */
	"",					/* 82 */
	"",					/* 83 */
	"",					/* 84 */
	"",					/* 85 */
	"",					/* 86 */
	"",					/* 87 */
	"",					/* 88 */
	"",					/* 89 */
	"fsel_input",				/* 90 */
	"fsel_exinput",				/* 91 */
	"",					/* 92 */
	"",					/* 93 */
	"",					/* 94 */
	"",					/* 95 */
	"",					/* 96 */
	"",					/* 97 */
	"",					/* 98 */
	"",					/* 99 */
	"wind_create",				/* 100 */
	"wind_open",				/* 101 */
	"wind_close",				/* 102 */
	"wind_delete",				/* 103 */
	"wind_get",				/* 104 */
	"wind_set",				/* 105 */
	"wind_find",				/* 106 */
	"wind_update",				/* 107 */
	"wind_calc",				/* 108 */
	"wind_new",				/* 109 */
	"rsrc_load",				/* 110 */
	"rsrc_free",				/* 111 */
	"rsrc_gaddr",				/* 112 */
	"rsrc_saddr",				/* 113 */
	"rsrc_obfix",				/* 114 */
	"rsrc_rcfix",				/* 115 */
	"",					/* 116 */
	"",					/* 117 */
	"",					/* 118 */
	"", 					/* 119 */
	"shel_read",				/* 120 */
	"shel_write",				/* 121 */
	"shel_get",				/* 122 */
	"shel_put",				/* 123 */
	"shel_find",				/* 124 */
	"shel_envrn",				/* 125 */
	"",					/* 126 */
	"",					/* 127 */
	"",					/* 128 */
	"",					/* 129 */
	"appl_getinfo"				/* 130 */
};
#endif
