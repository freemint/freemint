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

#ifndef _adi_h
#define _adi_h

#include "global.h"
#include "adi/adidefs.h"

struct adif *	adi_name2adi	(char *aname);
short		adi_getfreeunit	(char *name);
long		adi_register	(struct adif *a);
long		adi_unregister	(struct adif *a);
long		adi_close	(struct adif *a);
long		adi_open	(struct adif *a);
long		adi_ioctl	(struct adif *a, short cmd, long arg);

#endif /* _adi_h */
