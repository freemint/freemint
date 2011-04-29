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

#ifndef _ucd_h
#define _ucd_h

#include "global.h"
#include "ucd/ucd_defs.h"

extern struct 	ucdif *allucdifs;

struct ucdif *	ucd_name2ucd		(char *aname);
short		ucd_getfreeunit		(char *name);
long		ucd_register		(struct ucdif *u);
long		ucd_unregister		(struct ucdif *u);
long		ucd_close		(struct ucdif *u);
long		ucd_open		(struct ucdif *u);
long		ucd_ioctl		(struct ucdif *u, short cmd, long arg);

#endif /* _ucd_h */
