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

/*
 * Kernal Message Handler
 */

#ifndef _k_mouse_h
#define _k_mouse_h

#include "global.h"
#include "adi.h"
#include "xa_types.h"

bool is_bevent(int gotbut, int gotcl, const short *o, int which);
//bool add_md(struct moose_data *md);

void button_event(enum locks lock, struct xa_client *client, const struct moose_data *md);

//void XA_button_event  (enum locks lock, const struct moose_data *md, bool widgets);
//int  XA_move_event    (enum locks lock, const struct moose_data *md);
//void XA_wheel_event   (enum locks lock, const struct moose_data *md);

void kick_mousemove_timeout(void);

/* block until mouse data are available */
void wait_mouse(struct xa_client *client, short *br, short *xr, short *yr);
/* non-blocking, context free */
void check_mouse(struct xa_client *client, short *br, short *xr, short *yr);

void adi_move(struct adif *a, short x, short y);
void adi_button(struct adif *a, struct moose_data *md);
void adi_wheel(struct adif *a, struct moose_data *md);

bool eiffel_wheel(unsigned short scan);

#endif /* _k_mouse_h */
