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

#ifndef _menuwidg_h
#define _menuwidg_h

#include "global.h"
#include "xa_global.h"
#include "xa_types.h"

void free_desk_popup(void);

void wt_menu_area(XA_TREE *wt);
void set_rootmenu_area(struct xa_client *client);

TASK	click_form_popup_entry;
TASK	click_popup_entry;

Tab *	nest_menutask(Tab *tab);
void	popout(Tab *tab);
void	start_popup_session(Tab *tab, XA_TREE *wt, short item, short entry, TASK *click, short rdx, short rdy);
bool	is_attach(struct xa_client *client, XA_TREE *wt, int item, XA_MENU_ATTACHMENT **pat);
int	inquire_menu(int lock, struct xa_client *client, XA_TREE *wt, int item, XAMENU *mn);
int	attach_menu(int lock, struct xa_client *client, XA_TREE *wt, int item, XAMENU *mn, on_open_attach *cb_open, void *data);
int	detach_menu(int lock, struct xa_client *client, XA_TREE *wt, int item);
void	free_attachments(struct xa_client *client);
void	remove_attachments(int lock, struct xa_client *client, XA_TREE *wt);
void	set_menu_widget(struct xa_window *wind, struct xa_client *owner, XA_TREE *menu);
void	fix_menu(struct xa_client *client, XA_TREE *root, struct xa_window *w, bool do_desk);

Tab *	collapse(Tab *from, Tab *upto);
bool	find_pop(short x, short y, Tab **ret);


INLINE	struct xa_widget * get_menu_widg(void) { return &root_window->widgets[XAW_MENU]; }
INLINE	XA_TREE *get_menu(void) { return root_window->widgets[XAW_MENU].stuff.wt; }
INLINE	short get_menu_height(void) { return C.Aes->std_menu->tree->ob_height; }
INLINE	struct xa_client *menu_owner(void) { return get_menu()->owner; }

void close_window_menu(Tab *tab);
bool	keyboard_menu_widget(int lock, struct xa_window *wind, struct xa_widget *widg);
bool	menu_keyboard(Tab *tab, const struct rawkey *key);

#endif /* _menuwidg_h */
