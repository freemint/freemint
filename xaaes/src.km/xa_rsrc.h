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

#ifndef _xa_rsrc_h
#define _xa_rsrc_h

#include "global.h"
#include "xa_types.h"
//void dump_ra_list(struct xa_rscs *);

void *LoadResources(struct xa_client *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal);
void FreeResources(struct xa_client *client, AESPB *pb, struct xa_rscs *rscs);
OBJECT *ResourceTree(RSHDR *base, long num);
void obfix(OBJECT *tree, int object);

AES_function
	XA_rsrc_load,
	XA_rsrc_free,
	XA_rsrc_gaddr,
	XA_rsrc_obfix,
	XA_rsrc_rcfix;

#endif /* _xa_rsrc_h */
