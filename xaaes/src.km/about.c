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
void file_to_list( SCROLL_INFO *list, char *path, char *fn);

/*
#undef DISPCREDITS
#undef LONG_LICENSE
#define DISPCREDITS	1
#define LONG_LICENSE	1
*/
static char *about_lines[] =
{
  /*          1         2         3         4         5         6
     123456789012345678901234567890123456789012345678901234567890 */
	"",
#if DISPCREDITS
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
#else
	"Part of freemint ("SPAREMINT_URL").",
#endif
#if LONG_LICENSE
	"XaAES is free software; you can redistribute it",
	"and/or modify it under the terms of the GNU",
	"General Public License as published by the Free",
	"Software Foundation; either version 2 of the",
	"License, or (at your option) any later version.",
#else
	"",
	"The terms of the GPL version 2 or later apply.",
#endif
	"",
	NULL
};

// static struct xa_window *about_window = NULL;

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

// 	wt->current = fr->obj|fr->dblmask;

	switch (aesobj_item(&fr->obj))
	{
		case ABOUT_OK:
		{
			object_deselect(obtree + ABOUT_OK);
			redraw_toolbar(lock, wind, ABOUT_OK);
			close_window(lock, wind);
// 			delayed_delete_window(lock, wind);
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

#if HELPINABOUT
/*
 * insert lines of a text-file into a list
 * if first non-white char is '#' line is skipped
 * line-endings must be unix-style
 *
 * currently used only in about-window
 */
void file_to_list( SCROLL_INFO *list, char *path, char *fn)
{
	char ebuf[196], *p, *p1, *p2;
	struct file *fp;
	struct scroll_content sc = {{ 0 }};
	long err;
	int state = 0, offs = 0;

	sprintf( ebuf, sizeof(ebuf), "%s\\%s", path, fn );
	fp = kernel_open( ebuf, O_RDONLY, &err, NULL );
	if( fp )
	{
		sc.t.strings = 1;
		p2 = 0;
		state = 0;
		for( ;; )
		{
			if( offs > 0 )
				memcpy( ebuf, p2 + 1, offs );
			err = kernel_read( fp, ebuf+offs, sizeof(ebuf)-offs-1 );

			if( err <= 0 )
				break;
			ebuf[err+offs] = 0;
			if( offs == 0 && ebuf[0] == '#' )
				state = 1;
			for( p2 = 0, p1 = ebuf, p = ebuf+offs; *p; p++ )
			{
				if( state == 2 && *p > ' ' )
				{
					if( *p == '#' )
						state = 1;
					else
						state = 0;
				}
				if( *p == '\n' )
				{
					*p = 0;
					if( state != 1 )
					{
						sc.t.text = p1;
						list->add(list, NULL, NULL, &sc, false, 0, false);
					}
					p2 = p;
					*p = '\n';
					p1 = p+1;
					if( *p1 == '#' )
						state = 1;
					else if( *p1 <= ' ' )
						state = 2;
					else
						state = 0;
				}
			}
			if( p2 && p > p2 )
				offs = p - p2 - 1;
			else
				offs = 0;
		}
		kernel_close(fp);
	}
}
#endif

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

		set_slist_object(0, wt, NULL, ABOUT_LIST, SIF_AUTOSLIDERS,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, 255);
		obj_init_focus(wt, OB_IF_RESET);

		obj_rectangle(wt, aesobj(obtree, 0), &or);


		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, C.Aes, WC_BORDER,
				BORDER|CLOSER|NAME|TOOLBAR|(C.Aes->options.xa_nomove ? 0 : MOVER),
				created_for_AES,
				C.Aes->options.thinframe,
				C.Aes->options.thinwork, *(RECT *)&or);
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag,
					do_formwind_msg,
					client,//C.Aes,
					false,
					BORDER|CLOSER|NAME|TOOLBAR|(C.Aes->options.xa_nomove ? 0 : MOVER),
					created_for_AES,
					C.Aes->options.thinframe, C.Aes->options.thinwork,
					remember, 0, NULL);

		if (!wind) goto fail;

		wind->min.h = wind->r.h;//MINOBJMVH * 3;	/* minimum height for this window */
		wind->min.w = wind->r.w;//MINOBJMVH;	/* minimum width for this window */
		/* Set the window title */
		set_window_title(wind, "  About  ", true);

		(obtree + ABOUT_INFOSTR)->ob_spec.free_string = "\0";
#if XAAES_RELEASE
		/* set version */
		(obtree + ABOUT_VERSION)->ob_spec.free_string = vversion;
		/* Set version date */
		(obtree + ABOUT_DATE)->ob_spec.free_string = __DATE__;
		(obtree + ABOUT_TARGET)->ob_spec.free_string = arch_target;
#else
		(obtree + ABOUT_VERSION)->ob_spec.free_string = vversion;
		/* Set version date */
		(obtree + ABOUT_DATE)->ob_spec.free_string = info_string;
		(obtree + ABOUT_TARGET)->ob_spec.free_string = arch_target;
		//(obtree + ABOUT_INFOSTR)->ob_spec.free_string = info_string;
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
#if HELPINABOUT
			file_to_list( list, C.start_path, "xa_help.txt" );
#endif
		}

		list->slider(list, false);

		/* Set the window destructor */
		wind->destructor = about_destructor;
		htd->w_about = wind;
		if (open)
			open_window(lock, wind, wind->r); //remember);
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
