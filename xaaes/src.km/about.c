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


#define PROFILING	0
#include RSCHNAME

#include "xa_types.h"
#include "xa_global.h"
#include "xa_strings.h"

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


#if 1
extern char *about_lines[];
#else
char *about_lines[] =
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
	AB_MINT,
#endif
#if LONG_LICENSE
	"XaAES is free software; you can redistribute it",
	"and/or modify it under the terms of the GNU",
	"General Public License as published by the Free",
	"Software Foundation; either version 2 of the",
	"License, or (at your option) any later version.",
#else
	"",
	AB_GPL,
#endif

	"",
	NULL
};
#endif


static int
about_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_about = NULL;
	return true;
}

static RECT wsiz = { 0, 0, 0, 0 };

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
			SCROLL_INFO *list;
			object_deselect(obtree + ABOUT_OK);
			redraw_toolbar(lock, wind, ABOUT_OK);
			close_window(lock, wind);
			wsiz = wind->r;
			list = object_get_slist(obtree + ABOUT_LIST);
			list->destroy( list );
			delete_window(lock, wind);

			if( wind->parent )
				top_window( lock, true, true, wind->parent );
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

#define VIEW_MAX_LINES 1024
/*
 * insert lines of a text-file into a scroll-list
 * if first non-white char is '#' line is skipped
 * line-endings must be unix-or dos-style
 *
 *
 */
static void file_to_list( SCROLL_INFO *list, char *fn, bool skip_hash)
{
	struct scroll_content sc = {{ 0 }};
	struct stat st;
	long r = f_stat64(0, fn, &st);
	XA_FILE *xa_fp;
	char *p;
	long lineno;

	if( r || !S_ISREG(st.mode) )
		return;
	xa_fp = xa_fopen(fn, O_RDONLY );
	if( !xa_fp )
		return;

	sc.t.strings = 1;
	//sc.fnt = &norm_txt;
	sc.fnt = 0;


	PRINIT;
	for( lineno = 1; lineno < VIEW_MAX_LINES && (p = xa_readline( 0, 0, xa_fp )); lineno++ )
	{
		sc.t.text = p;
		if( skip_hash == true )
		{
			for( ; *p && *p <= ' '; p++ )
			;
			if( *p == '#' )
				continue;
		}
		list->add(list, NULL, NULL, &sc, false, 0, false);
	}


	if( lineno >= VIEW_MAX_LINES )
	{
		sc.t.text = "...";
		list->add(list, NULL, NULL, &sc, false, 0, false);
	}
	xa_fclose(xa_fp);
	PRPRINT;
	list->set(list, list->start, SESET_SELECTED, 0, NOREDRAW);

}
#endif

void
open_about(enum locks lock, struct xa_client *client, bool open, char *fn)
{
	struct helpthread_data *htd;
	struct xa_window *wind;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	RECT or;
	SCROLL_INFO *list;
	char ebuf[196];
	bool view_file = false;

	if( (fn && (client->status & CS_USES_ABOUT)) )
	{
		view_file = true;
	}
	client = C.Hlp;

	htd = get_helpthread_data(client);
	if (!htd)
		return;

	if (view_file || !htd->w_about)
	{
		RECT remember = { 0, 0, 0, 0 };

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, ABOUT_XAAES), 0);
		if (!obtree) goto fail;

		if( view_file )
		{
			int i;
			short h = screen.c_max_h / 2, h0 = obtree->ob_height;
			(obtree + ABOUT_LIST)->ob_y -= h * 6;
			(obtree + ABOUT_LIST)->ob_height = h0 - h * 6;

			(obtree + ABOUT_OK)->ob_y += h * 3 - 2;
			(obtree + ABOUT_OK)->ob_height -= h;
			for( i = ABOUT_VERSION - 1; i <= RSC_VERSION; i++ )
			{
				if( i != ABOUT_OK )
					(obtree + i)->ob_flags |= OF_HIDETREE;
			}
		}
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
		if( client != C.Aes )
			wt->flags |= WTF_TREE_CALLOC;

		obj_init_focus(wt, OB_IF_RESET);

		obj_rectangle(wt, aesobj(obtree, 0), &or);

		/* Work out sizing */
		//if (!remember.w)
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
					BACKDROP|BORDER|CLOSER|NAME|TOOLBAR|(C.Aes->options.xa_nomove ? 0 : MOVER),
					created_for_AES,
					C.Aes->options.thinframe, C.Aes->options.thinwork,
					remember, 0, NULL);

		wind->parent = TOP_WINDOW;
		if (!wind) goto fail;

		wind->min.h = wind->r.h;	/* minimum height for this window */
		wind->min.w = wind->r.w;	/* minimum width for this window */

		set_slist_object(0, wt, wind, ABOUT_LIST, SIF_AUTOSLIDERS | SIF_INLINE_EFFECTS | SIF_AUTOSELECT,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, 255);

		if( !view_file )
		{
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
#endif
		}

		wt = set_toolbar_widget(lock, wind, wind->owner, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		wt->exit_form = about_form_exit;
		//if( screen.c_max_h < 16 )
		{
			short d = 16 / screen.c_max_h;
			if( wsiz.w == 0 )
			{
				wsiz.x = wind->r.x;
				wsiz.y = wind->r.y - ( wind->r.h * ( d - 1 ) ) / 2;
				wsiz.w = wind->r.w;
				wsiz.h = wind->r.h * d;
			}
			wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
					WM_SIZED, 0,0, wind->handle, wsiz.x, wsiz.y, wsiz.w, wsiz.h );
		}
	}
	else{
		if( !view_file )
			wind = htd->w_about;
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
	}

	list = object_get_slist(wt->tree + ABOUT_LIST);
	set_xa_fnt( cfg.xaw_point, 0, 0, 0, list, 0, 0);

#if HELPINABOUT
	if(	view_file == true )
	{
		strcpy( ebuf, fn );
		/* Set the window title */
		set_window_title(wind, fn, true);
	}
	else
	{
		sprintf( ebuf, sizeof(ebuf), "%s%s", C.start_path, xa_strings[XA_HELP_FILE] );
		set_window_title(wind, xa_strings[RS_ABOUT], true);
	}


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
			if( view_file || xah_mtime.time != st.mtime.time )
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
		if( view_file == false )
		{
			while (*t)
			{
				sc.t.text = *t;
				list->add(list, NULL, NULL, &sc, false, 0, false);
				t++;
			}
		}
#if HELPINABOUT
		file_to_list( list, ebuf, view_file == false );
#endif
	}


	/* Set the window destructor */
	wind->destructor = about_destructor;
	if( !view_file )
		htd->w_about = wind;
	if (open)
	{
		open_window(lock, wind, wind->r);
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
