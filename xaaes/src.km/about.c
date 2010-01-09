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

static int
about_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_about = NULL;
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

	switch (aesobj_item(&fr->obj))
	{
		case ABOUT_OK:
		{
			object_deselect(obtree + ABOUT_OK);
			redraw_toolbar(lock, wind, ABOUT_OK);
			close_window(lock, wind);
			break;
		}
		case ABOUT_LIST:
		{
			if ( fr->md && (aesobj_type(&fr->obj) & 0xff) == G_SLIST)
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
open_about(enum locks lock, struct xa_client *client, bool open)
{
	struct helpthread_data *htd;
	struct xa_window *wind;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	RECT or;

	htd = get_helpthread_data(client);
	if (!htd)
		return;

	if (!htd->w_about)
	{
		RECT remember = { 0, 0, 0, 0 };
		SCROLL_INFO *list;

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, ABOUT_XAAES), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;

		set_slist_object(0, wt, NULL, ABOUT_LIST, 0,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, 255);
		obj_init_focus(wt, OB_IF_RESET);

		obj_rectangle(wt, aesobj(obtree, 0), &or);

		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, client, WC_BORDER, CLOSER|NAME, created_for_AES,
						client->options.thinframe,
						client->options.thinwork, *(RECT *)&or);
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag,
					do_formwind_msg,
					client,
					false,
					CLOSER|NAME|TOOLBAR|(client->options.xa_nomove ? 0 : MOVER),
					created_for_AES,
					client->options.thinframe,client->options.thinwork,
					remember, 0, NULL);

		if (!wind) goto fail;

		/* Set the window title */
		set_window_title(wind, "  About  ", true);
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

		wt = set_toolbar_widget(lock, wind, wind->owner, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
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
		wind->destructor = about_destructor;
		htd->w_about = wind;
		if (open)
			open_window(lock, wind, wind->r);
	}
	else
	{
		wind = htd->w_about;
		if (open)
		{
			open_window(lock, wind, wind->r);
			if (wind != TOP_WINDOW)
				top_window(lock, true, false, wind);
		}
	}
	return;
fail:
	if (wt)
	{
		remove_wt(wt, false);
		obtree = NULL;
	}
	if (obtree)
		free_object_tree(client, obtree);
}
