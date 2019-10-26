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

#ifndef _c_mouse_h
#define _c_mouse_h

#include "widgets.h"
#include "global.h"
#include "xa_types.h"

void cXA_button_event(int lock, struct c_event *ce, short cancel);
void cXA_deliver_button_event(int lock, struct c_event *ce, short cancel);
void cXA_deliver_rect_event(int lock, struct c_event *ce, short cancel);
void cXA_form_do(int lock, struct c_event *ce, short cancel);
void cXA_open_menu(int lock, struct c_event *ce, short cancel);
void cXA_menu_move(int lock, struct c_event *ce, short cancel);
void cXA_do_widgets(int lock, struct c_event *ce, short cancel);
void cXA_active_widget(int lock, struct c_event *ce, short cancel);
void cXA_widget_click(int lock, struct c_event *ce, short cancel);
void cXA_wheel_event(int lock, struct c_event *ce, short cancel);

#endif /* _c_mouse_h */
