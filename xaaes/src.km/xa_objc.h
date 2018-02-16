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

#ifndef _xa_objc_h
#define _xa_objc_h

#include "global.h"
#include "xa_types.h"

AES_function
	XA_objc_draw,
	XA_objc_wdraw,		/* MagiC 5.10 */
	XA_objc_offset,
	XA_objc_find,
	XA_objc_change,
	XA_objc_wchange,	/* MagiC 5.10 */
	XA_objc_add,
	XA_objc_delete,
	XA_objc_order,
	XA_objc_edit,
	XA_objc_wedit,		/* MagiC 5.10 */
	XA_objc_sysvar,
	XA_objc_data;		/* XaAES */

#endif /* _xa_objc_h */
