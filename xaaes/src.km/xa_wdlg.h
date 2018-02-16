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

#ifndef _xa_wdlg_h
#define _xa_wdlg_h

#include "global.h"
#include "xa_types.h"

#if WDIALOG_WDLG

void  wdialog_redraw(enum locks lock, struct xa_window *wind, struct xa_aes_object start, short depth, RECT *r);
short wdialog_event(enum locks lock, struct xa_client *client, struct wdlg_evnt_parms *wep);


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

#endif /* _xa_wdlg_h */
