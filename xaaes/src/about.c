/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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
 
#include RSCHNAME

#include "xa_types.h"
#include "xa_global.h"

#include "c_window.h"
#include "objects.h"
#include "scrlobjc.h"
#include "widgets.h"
#include "xa_form.h"
#include "xa_rsrc.h"

#include "about.h"


static char *about_lines[] =
{
  /*          1         2         3         4         5         6
     123456789012345678901234567890123456789012345678901234567890 */
	"Original code by Craig at Data Uncertain.",
	"Additional work by:",
	"Johan K, Martin K, Mario B, T Eero,",
	"Steve S, Evan K, Thomas B, James C.",
	"Using Harald Siegmunds NKCC.",
	"Thanks to all on the MiNT mailing list,",
	"       Joakim H for his 3D test program.",
	"",
	"XaAES is free software. You may use it,",
	"modify it, rip out and renovate it's dark",
	"inner secrets .... as long as you tell",
	"me. You can NOT sell it for profit.",
	0
};

static XA_WINDOW *about_window = 0;

static int
about_destructor(LOCK lock, struct xa_window *wind)
{
	about_window = 0;
	return true;
}

static void
handle_about(LOCK lock, struct widget_tree *wt)
{
	/* The ''form_do'' part */
	if ((wt->current & 0xff) == ABOUT_OK)
	{
		deselect(wt->tree, ABOUT_OK);
		display_toolbar(lock, about_window, ABOUT_OK);
		close_window(lock, about_window);
		delete_window(lock, about_window);
	}
}

void
open_about(LOCK lock)
{
	if (!about_window)
	{
		static RECT remember = { 0, 0, 0, 0 };

		XA_WINDOW *dialog_window;
		XA_TREE *wt;
		SCROLL_INFO *list;
		OBJECT *form = ResourceTree(C.Aes_rsc, ABOUT_XAAES);

		/* Work out sizing */
		if (!remember.w)
		{
			center_form(form, ICON_H);
			remember = calc_window(lock, C.Aes, WC_BORDER, CLOSE|NAME, MG,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork, form->r);
		}

		/* Create the window */
		dialog_window = create_window(lock, 0,
						C.Aes,
						false,
						CLOSE|NAME|
						MOVE|
						TOOLBAR,
						created_for_AES,
						MG,
						C.Aes->options.thinframe,C.Aes->options.thinwork,
						remember, 0, &remember);

		/* Set the window title */
		get_widget(dialog_window, XAW_TITLE)->stuff = "  About  ";
		/* Set version date */
		(form + ABOUT_DATE)->ob_spec.string = __DATE__;

		wt = set_toolbar_widget(lock, dialog_window, form, -1);
		wt->exit_form = XA_form_exit;
		wt->exit_handler = handle_about;

		/* set a scroll list widget */
		list = set_slist_object(lock, wt, form, ABOUT_LIST, 0, 0, 0, 0, 0, 0, 42);

		/* fill the list if already list */
		if (!list->start)
		{
			char **t = about_lines;
			while (*t)
			{
				add_scroll_entry(form, ABOUT_LIST, 0, *t, 0);
				t++;
			}
		}
		
		list->slider(list);

		/* Set the window destructor */
		dialog_window->destructor = about_destructor;
		open_window(lock, dialog_window, remember);
		about_window = dialog_window;
	}
	else if (about_window != window_list)
	{
		C.focus = pull_wind_to_top(lock, about_window);
		after_top(lock, true);
		display_window(lock, 3, about_window, NULL);
	}
}
