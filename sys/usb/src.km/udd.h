/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _udd_h
#define _udd_h

#include "global.h"
#include "udd/udd_defs.h"

extern struct 	uddif *alluddifs;

struct uddif *	udd_name2udd		(char *aname);
short		udd_getfreeunit		(char *name);
long		udd_register		(struct uddif *u, struct usb_driver *);
long		udd_unregister		(struct uddif *u, struct usb_driver *);
long		udd_close		(struct uddif *u);
long		udd_open		(struct uddif *u);
long		udd_ioctl		(struct uddif *u, short cmd, long arg);

#endif /* _udd_h */
