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

void check_wh_cp(GRECT *c, COMPASS cp, short minw, short minh, short maxw, short maxh);

void xa_graf_mouse(int m_shape, MFORM *mf, struct xa_client *client, bool aesm);
void set_client_mouse(struct xa_client *client, short which, short m_form, MFORM *mf);

//bool rect_changed(const GRECT *n, const GRECT *o);

//void keep_inside(GRECT *r, const GRECT *o);

//const GRECT *rect_dist(struct xa_client *client, GRECT *r, GRECT *d);
const GRECT *rect_dist_xy(struct xa_client *client, short x, short y, GRECT *r, GRECT *d);

GRECT widen_rectangle(COMPASS c,
                short mx, short my,
                GRECT start,
                const GRECT *dist);

GRECT move_rectangle(short mx, short my,
                GRECT r,
                const GRECT *dist);

void rubber_box(struct xa_client *client, COMPASS c,
                GRECT start, const GRECT *dist,
                int minw, int minh,
                int maxw, int maxg,
                GRECT *last);

void drag_box(struct xa_client *client, GRECT start,
              const GRECT *bound,
              const GRECT *dist,
              GRECT *last);

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
