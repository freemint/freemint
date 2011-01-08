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
#include "taskman.h"
#include "init.h"
#include "messages.h"
#include "obtree.h"
#include "scrlobjc.h"
#include "widgets.h"
#include "xa_form.h"
#include "xa_rsrc.h"
#include "version.h"
#include "about.h"
#include "mint/stat.h"


#define XA_HELP_FILE	"xa_help.txt"

#if XAAES_RELEASE
#define FN_CRED	1
#else
#define FN_CRED	0
#endif

static char *about_lines[] =
{
  /*          1         2         3         4         5         6
     123456789012345678901234567890123456789012345678901234567890 */
#if FN_CRED
	"",
	"           <u>Dedicated to <i>Frank Naumann </i></u>\xbb",
	"<u>                                                                  </u>",
#endif
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
	"The terms of the <b>GPL version 2</b> or later apply.",
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
#include "util.h"
/*
 * insert lines of a text-file into a scroll-list
 * if first non-white char is '#' line is skipped
 * line-endings must be unix-or dos-style
 *
 * currently used only in about-window
 *
  */
static void file_to_list( SCROLL_INFO *list, char *fn)
{
	struct scroll_content sc = {{ 0 }};
	XA_FILE *xa_fp = xa_fopen(fn, O_RDONLY );
	char *p;

	if( !xa_fp )
		return;

	sc.t.strings = 1;
	sc.fnt = &norm_txt;

	for( ; (p = xa_readline( 0, 0, xa_fp )); )
	{
		sc.t.text = p;
		for( ; *p && *p <= ' '; p++ )
		;
		if( *p != '#' )
			list->add(list, NULL, NULL, &sc, false, 0, false);
	}
	xa_fclose(xa_fp);

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
	SCROLL_INFO *list;
	char ebuf[196];

	htd = get_helpthread_data(client);
	if (!htd)
		return;

	if (!htd->w_about)
	{
		RECT remember = { 0, 0, 0, 0 };

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, ABOUT_XAAES), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;

		set_slist_object(0, wt, NULL, ABOUT_LIST, SIF_AUTOSLIDERS | SIF_INLINE_EFFECTS,
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
		if( screen.c_max_h < 16 ){
			short d = 16 / screen.c_max_h;
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					WM_SIZED, 0,0, wind->handle, wind->r.x, wind->r.y, wind->r.w, wind->r.h * d );
		}
	}
	else{
		wind = htd->w_about;
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;

	}

	list = object_get_slist(wt->tree + ABOUT_LIST);
	set_xa_fnt( cfg.xaw_point, 0, 0, 0, list);

#if HELPINABOUT
	sprintf( ebuf, sizeof(ebuf), "%s\\%s", C.start_path, XA_HELP_FILE );
	/* check if help-file has changed and if yes re-read */
	if (list->start)
	{
		static struct time xah_mtime = {0,0,0};
		struct stat st;
		if( f_stat64( 0, ebuf, &st ) )
		{
			BLOG((0,"about:cannot stat %s", ebuf));
		}
		else
		{
			if( xah_mtime.time != st.mtime.time )
			{
				list->empty(list, 0, 0);
				list->start = 0;
				xah_mtime.time = st.mtime.time;
			}
		}
	}
#endif
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
		file_to_list( list, ebuf );
#endif
	}

	list->slider(list, false);

	/* Set the window destructor */
	wind->destructor = about_destructor;
	htd->w_about = wind;
	if (open)
	{
		open_window(lock, wind, wind->r); //remember);
		force_window_top( lock, wind );
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
