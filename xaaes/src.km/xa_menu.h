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

#ifndef _xa_menu_h
#define _xa_menu_h

#include "global.h"
#include "xa_types.h"

void set_menu_width( OBJECT *mnu, XA_TREE *mwt );
int menu_popup(int lock, struct xa_client *client, XAMENU *xmn, XAMENU_RESULT *result, short px, short py, short usr_evnt);

AES_function
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
	XA_form_popup;

#endif /* _xa_menu_h */
