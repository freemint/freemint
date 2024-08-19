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

#ifndef _xa_rsrc_h
#define _xa_rsrc_h

#include "global.h"
#include "xa_types.h"
//void dump_ra_list(struct xa_rscs *);
/* debug */
void dump_hex( void *data, long len, int bpw, int doit );

RSHDR * _cdecl LoadResources(struct xa_client *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal);
void _cdecl FreeResources(struct xa_client *client, AESPB *pb, struct xa_rscs *rscs);
OBJECT * _cdecl ResourceTree(RSHDR *base, long num);
char *ResourceString(RSHDR *hdr, int num);
void _cdecl obfix(OBJECT *tree, short object, short designwidth, short designheight);
void hide_object_tree( RSHDR *rsc, short tree, short item, int Unhide );

AES_function
	XA_rsrc_load,
	XA_rsrc_free,
	XA_rsrc_gaddr,
	XA_rsrc_obfix,
	XA_rsrc_rcfix;

/* constants for rsc-translation */
#define READ	1
#define WRITE 2

#endif /* _xa_rsrc_h */
