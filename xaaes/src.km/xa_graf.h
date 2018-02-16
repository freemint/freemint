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

#ifndef _xa_graf_h
#define _xa_graf_h

#include "global.h"
#include "xa_types.h"

/* Avoid a clash with the standard function name */
#define xa_graf_mouse xaaes_xa_graf_mouse

void check_wh_cp(RECT *c, COMPASS cp, short minw, short minh, short maxw, short maxh);

void xa_graf_mouse(int m_shape, MFORM *mf, struct xa_client *client, bool aesm);
void set_client_mouse(struct xa_client *client, short which, short m_form, MFORM *mf);

//bool rect_changed(const RECT *n, const RECT *o);

//void keep_inside(RECT *r, const RECT *o);

//const RECT *rect_dist(struct xa_client *client, RECT *r, RECT *d);
const RECT *rect_dist_xy(struct xa_client *client, short x, short y, RECT *r, RECT *d);

RECT widen_rectangle(COMPASS c,
                short mx, short my,
                RECT start,
                const RECT *dist);

RECT move_rectangle(short mx, short my,
                RECT r,
                const RECT *dist);

void rubber_box(struct xa_client *client, COMPASS c,
                RECT start, const RECT *dist,
                int minw, int minh,
                int maxw, int maxg,
                RECT *last);

void drag_box(struct xa_client *client, RECT start,
              const RECT *bound,
              const RECT *dist,
              RECT *last);

AES_function
	XA_xa_graf_mouse,
	XA_graf_handle,
	XA_graf_mkstate,
	XA_graf_rubberbox,
	XA_graf_dragbox,
	XA_graf_watchbox,
	XA_graf_wwatchbox,	/* MagiC 5.10 */
	XA_graf_growbox,
	XA_graf_shrinkbox,
	XA_graf_movebox,
	XA_graf_slidebox;

#endif /* _xa_graf_h */
