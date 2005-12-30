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
	
// 	wt->current = fr->obj|fr->dblmask;

	switch (aesobj_item(&fr->obj))
	{
		case ABOUT_OK:
		{
			object_deselect(obtree + ABOUT_OK);
			redraw_toolbar(lock, about_window, ABOUT_OK);
			close_window(lock, about_window);
			delayed_delete_window(lock, about_window);
			break;
		}
		case ABOUT_LIST:
		{
			if ( fr->md && (aesobj_ob(&fr->obj)->ob_type & 0xff) == G_SLIST)
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, obtree, aesobj_item(&fr->obj), fr->md);
				else
					click_scroll_list(lock, obtree, aesobj_item(&fr->obj), fr->md);
			}
			break;
		}
	}
}

void
open_about(enum locks lock, struct xa_client *client)
{
	if (!about_window)
	{
		RECT remember = { 0, 0, 0, 0 };

		struct xa_window *dialog_window;
		XA_TREE *wt;
		SCROLL_INFO *list;
		OBJECT *obtree = ResourceTree(C.Aes_rsc, ABOUT_XAAES);
		RECT or;

		wt = obtree_to_wt(client, obtree);
		
		ob_rectangle(obtree, aesobj(obtree, 0), &or);

		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, client, WC_BORDER, CLOSER|NAME, created_for_AES,
						client->options.thinframe,
						client->options.thinwork, *(RECT *)&or);
		}

		/* Create the window */
		dialog_window = create_window(lock,
						do_winmesag,
						do_formwind_msg,
						client,
						false,
						CLOSER|NAME|TOOLBAR|(client->options.xa_nomove ? 0 : MOVER),
						created_for_AES,
						client->options.thinframe,client->options.thinwork,
						remember, 0, NULL); //&remember);

		/* Set the window title */
		set_window_title(dialog_window, "  About  ", true);
		/* set version */
		(obtree + ABOUT_VERSION)->ob_spec.free_string = vversion;
		/* Set version date */
		(obtree + ABOUT_DATE)->ob_spec.free_string = __DATE__;
		(obtree + ABOUT_TARGET)->ob_spec.free_string = arch_target;
#if XAAES_RELEASE
		(obtree + ABOUT_INFOSTR)->ob_spec.free_string = '\0';
#else
		(obtree + ABOUT_INFOSTR)->ob_spec.free_string = info_string;
#endif

		wt = set_toolbar_widget(lock, dialog_window, dialog_window->owner, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, true, NULL, &or);
		wt->exit_form = about_form_exit;
		list = object_get_slist(obtree + ABOUT_LIST);

		/* fill the list if already list */
		if (!list->start)
		{
			char **t = about_lines;
			struct scroll_content sc = {{ 0 }};

			sc.t.strings = 1;
			while (*t)
			{
				sc.t.text = *t;
				list->add(list, NULL, NULL, &sc, false, 0, false);
				t++;
			}
		}
		
		list->slider(list, false);

		/* Set the window destructor */
		dialog_window->destructor = about_destructor;
		open_window(lock, dialog_window, remember);
		about_window = dialog_window;
	}
	else if (about_window != window_list)
	{
		top_window(lock, true, false, about_window, (void *)-1L);
	}
}
