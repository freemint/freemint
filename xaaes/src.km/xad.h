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

#ifndef _xad_h
#define _xad_h

#include "global.h"
#include "xad_defs.h"

struct xad *	xad_name2adi	(char *aname);
short		xad_getfreeunit	(char *name);
long		xad_register	(struct xad *a);
long		xad_unregister	(struct xad *a);
long		xad_close	(struct xad *a);
long		xad_open	(struct xad *a);
long		xad_ioctl	(struct xad *a, short cmd, long arg);

#endif /* _xad_h */
