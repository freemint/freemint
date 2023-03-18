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

#ifndef _scrlobjc_h
#define _scrlobjc_h

#include "global.h"

void	click_scroll_list(int lock, OBJECT *form, int objc, const struct moose_data *md);
void	dclick_scroll_list(int lock, OBJECT *form, int objc, const struct moose_data *md);
void	draw_slist(int lock, SCROLL_INFO *list, SCROLL_ENTRY *entry, const GRECT *clip);

SCROLL_INFO *set_slist_object(
		int lock,
		XA_TREE *wt,
		struct xa_window *parentwind,
		short item,
		short flags,

		scrl_widget *closer,
		scrl_widget *fuller,
		scrl_click *dclick,
		scrl_click *click,
		scrl_click *click_nesticon,
		scrl_keybd *key,

		scrl_add	*add,
		scrl_del	*del,
		scrl_empty	*empty,
		scrl_list	*destroy,

		char *title, char *info,
		void *data,
		short line_max);

unsigned short scrl_cursor(SCROLL_INFO *list, unsigned short kc, unsigned short ks);
void free_scrollist(SCROLL_INFO *list);

extern struct xa_wtxt_inf default_fnt;

#endif /* _scrlobjc_h */
