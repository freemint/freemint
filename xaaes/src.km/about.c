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
 
#include RSCHNAME

#include "xa_types.h"
#include "xa_global.h"

#include "c_window.h"
#include "form.h"
#include "init.h"
#include "messages.h"
#include "obtree.h"
#include "scrlobjc.h"
#include "widgets.h"
#include "xa_form.h"
#include "xa_rsrc.h"
#include "version.h"
#include "about.h"


static char *about_lines[] =
{
  /*          1         2         3         4         5         6
     123456789012345678901234567890123456789012345678901234567890 */
	"",
	"Copyright (c) 1992 - 1998 C.Graham",
	"              1999 - 2003 H.Robbers",
	"                from 2004 F.Naumann & O.Skancke",
	"",
	"Additional work by:",
	"   Johan K, Martin K, Mario B, T Eero,",
	"   Steve S, Evan K, Thomas B, James C.",
	"",
	"Thanks to all on the MiNT mailing list,",
	"to Joakim H for his 3D test program.",
	"Using Harald Siegmunds NKCC.",
	"",
	"XaAES is free software; you can redistribute it",
	"and/or modify it under the terms of the GNU",
	"General Public License as published by the Free",
	"Software Foundation; either version 2 of the",
	"License, or (at your option) any later version.",
	"",
	NULL
};

static struct xa_window *about_window = NULL;

static int
about_destructor(enum locks lock, struct xa_window *wind)
{
	about_window = NULL;
	return true;
}

static void
about_form_exit(struct xa_client *client,
		struct xa_window *wind,
		struct widget_tree *wt,
		struct fmd_result *fr)
{
	enum locks lock = 0;
	OBJECT *obtree = wt->tree;
	
	wt->current = fr->obj|fr->dblmask;

	switch (fr->obj)
	{
		case ABOUT_OK:
		{
			object_deselect(obtree + ABOUT_OK);
			display_toolbar(lock, about_window, ABOUT_OK);
			close_window(lock, about_window);
			delete_window(lock, about_window);
			break;
		}
		case ABOUT_LIST:
		{
			short obj = fr->obj;

			if ( fr->md && ((obtree[obj].ob_type & 0xff) == G_SLIST))
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, obtree, obj, fr->md);
				else
					click_scroll_list(lock, obtree, obj, fr->md);
			}
			break;
		}
	}
}

void
open_about(enum locks lock)
{
	if (!about_window)
	{
		static RECT remember = { 0, 0, 0, 0 };

		struct xa_window *dialog_window;
		XA_TREE *wt;
		SCROLL_INFO *list;
		OBJECT *form = ResourceTree(C.Aes_rsc, ABOUT_XAAES);

		/* Work out sizing */
		if (!remember.w)
		{
			form_center(form, ICON_H);
			remember = calc_window(lock, C.Aes, WC_BORDER, CLOSER|NAME,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork, *(RECT*)&form->ob_x);
		}

		/* Create the window */
		dialog_window = create_window(lock,
						do_winmesag,
						do_formwind_msg,
						C.Aes,
						false,
						CLOSER|NAME|TOOLBAR|(C.Aes->options.xa_nomove ? 0 : MOVER),
						created_for_AES,
						C.Aes->options.thinframe,C.Aes->options.thinwork,
						remember, 0, &remember);

		/* Set the window title */
		get_widget(dialog_window, XAW_TITLE)->stuff = "  About  ";
		/* set version */
		(form + ABOUT_VERSION)->ob_spec.free_string = vversion;
		/* Set version date */
		(form + ABOUT_DATE)->ob_spec.free_string = __DATE__;
		(form + ABOUT_TARGET)->ob_spec.free_string = arch_target;
#if XAAES_RELEASE
		(form + ABOUT_INFOSTR)->ob_spec.free_string = '\0';
#else
		(form + ABOUT_INFOSTR)->ob_spec.free_string = info_string;
#endif

		wt = set_toolbar_widget(lock, dialog_window, dialog_window->owner, form, -1, WIDG_NOTEXT);
		wt->exit_form = about_form_exit;

		/* set a scroll list widget */
		list = set_slist_object(lock, wt, form, dialog_window, ABOUT_LIST, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 42);

		/* fill the list if already list */
		if (!list->start)
		{
			char **t = about_lines;
			while (*t)
			{
				add_scroll_entry(form, ABOUT_LIST, 0, *t, 0, NULL);
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
		top_window(lock, true, about_window, (void *)-1L, NULL);
	}
}
