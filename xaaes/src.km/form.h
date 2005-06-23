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

#ifndef _form_h
#define _form_h

#include "xa_types.h"
#include "mt_gem.h"

void	Set_form_do	(struct xa_client *client,
			 OBJECT *obtree,
			 short edobj);

struct xa_window *
	create_fmd_wind	(enum locks lock,
			 struct xa_client *client,
			 XA_WIND_ATTR kind,
			 WINDOW_TYPE dial,
			 RECT *r);

bool	Setup_form_do	(struct xa_client *client,
			 OBJECT *obtree,
			 short edobj,
			 /* Output */
			 struct xa_window **ret_wind,
			 short *ret_edobj);

void	form_center	(OBJECT *obtree, short barsizes);
void	form_center_r	(OBJECT *obtree, short barsizes, RECT *r);
void	center_rect	(RECT *r);

#define FBF_REDRAW	1
#define FBF_DO_SLIST	2

bool	form_button	(XA_TREE *wt,
			 struct xa_vdi_settings *v,
			 short obj,
			 const struct moose_data *md,
			 unsigned long flags, //bool redraw,
			 struct xa_rect_list *rl,
			 /* Outputs */
			 short *newstate,
			 short *nxtob,
			 short *clickmsk);

short	form_cursor	(XA_TREE *wt,
			 struct xa_vdi_settings *v,
			 ushort keycode,
			 short obj,
			 bool redraw,
			 struct xa_rect_list *rl);

bool	form_keyboard	(XA_TREE *wt,
			 struct xa_vdi_settings *v,
			 short obj,
			 const struct rawkey *key,
			 bool redraw,
			 struct xa_rect_list *rl,
			 /* outputs */
			 short *nxtobj,
			 short *newstate,
			 short *nxtkey);

void	Exit_form_do	(struct xa_client *client,
			 struct xa_window *wind,
			 XA_TREE *wt,
			 struct fmd_result *fr);

WidgetBehaviour	Click_windowed_form_do;
FormMouseInput	Click_form_do;
FormKeyInput	Key_form_do;
//SendMessage	handle_form_window;
DoWinMesag	do_formwind_msg;

#endif /* _form_h */
