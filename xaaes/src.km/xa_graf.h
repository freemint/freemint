/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for MiNT
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

typedef enum
{
	NW,
	N_,
	NE,
	E_,
	SE,
	S_,
	SW,
	W_,
	CDV			/* compass divisions */
} COMPASS;


/* Avoid a clash with the standard function name */
#define graf_mouse xaaes_graf_mouse

int watch_object(enum locks lock, XA_TREE *wt,
		 int ob, int in_state, int out_state);

void graf_mouse(int m_shape, MFORM *mf);

bool rect_changed(const RECT *n, const RECT *o);

void keep_inside(RECT *r, const RECT *o);

const RECT *rect_dist(struct xa_client *client, RECT *r, RECT *d);

RECT widen_rectangle(COMPASS compass,
                short mx, short my,
                RECT start,
                const RECT *dist);

RECT move_rectangle(short mx, short my,
                RECT r,
                const RECT *dist);

void rubber_box(struct xa_client *client, COMPASS compass,
                RECT start, const RECT *dist,
                int minw, int minh,
                int maxw, int maxg,
                RECT *last);

void drag_box(struct xa_client *client, RECT start,
              const RECT *bound,
              const RECT *dist,
              RECT *last);

AES_function
	XA_graf_mouse,
	XA_graf_handle,
	XA_graf_mkstate,
	XA_graf_rubberbox,
	XA_graf_dragbox,
	XA_graf_watchbox,
	XA_graf_growbox,
	XA_graf_shrinkbox,
	XA_graf_movebox,
	XA_graf_slidebox;

#endif /* _xa_graf_h */
