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


#define PROFILING	0

#include "xa_types.h"
#include "xa_global.h"
#include "xversion.h"

#include "xaaes.h"

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


#if XAAES_RELEASE
#define FN_CRED	0
#else
#define FN_CRED	0
#endif

#if 0
const char *const about_lines[] =
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
about_destructor(int lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_about = NULL;
	return true;
}

static SCROLL_INFO *alist = 0;
static SCROLL_ENTRY *athis[1] = {0};
static struct time xah_mtime = {0,0,0};
void reset_about(void)
{
	alist = 0;
	memset( athis, 0, sizeof(athis) );
	memset( &xah_mtime, 0, sizeof(xah_mtime) );
}

void add_keybd_switch(char *k)
{
	char buf[256], lang[4];
	if( !alist || !cfg.keyboards.c )
		return;

	lang_from_akp( lang, 1 );
	sprintf( buf, sizeof(buf), "%c             %s %s(%s)", cfg.keyboards.c, xa_strings(SW_KEYBD), k, lang );
	if( athis[0] )
	{
		struct setcontent_text t = { 0 };
		t.text = buf;
		alist->set( alist, athis[0], SESET_TEXT, (long)&t, true );
	}
	else
	{
		struct sesetget_params p = { 0 };
		struct scroll_content sc = {{ 0 }};
		sc.t.text = buf;
		sc.t.strings = 1;
		sc.fnt = 0;
		alist->add( alist, NULL, NULL, &sc, false, 0, false);

		/* search for keyboard-entry and save in athis[0] */
		p.level.maxlevel = 1;
		p.arg.txt = buf;
		alist->get(alist, 0, SEGET_ENTRYBYTEXT, &p);
		athis[0] = p.e;
	}
}

GRECT about_r = { 0, 0, 0, 0 };
GRECT view_r = { 0, 0, 0, 0 };

bool
wind_exist(int lock, struct xa_window *wind);

static void
about_form_exit(struct xa_client *client,
		struct xa_window *wind,
		struct widget_tree *wt,
		struct fmd_result *fr)
{
	int lock = 0;
	OBJECT *obtree = wt->tree;

	switch (aesobj_item(&fr->obj))
	{
		case ABOUT_OK:
		{
			SCROLL_INFO *list;
			object_deselect(obtree + ABOUT_OK);
			redraw_toolbar(lock, wind, ABOUT_OK);
			close_window(lock, wind);
			list = object_get_slist(obtree + ABOUT_LIST);
			if( list != alist )
			{
				struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
				list->destroy( list );
				view_r = wind->r;
				if( htd )
					htd->w_view = 0;
				if( wind->parent && wind_exist( lock, wind->parent) && (wind->parent->dial & created_for_AES) )
					top_window( lock, true, true, wind->parent );
				delete_window(lock, wind);
			}

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

#if ALT_CTRL_APP_OPS

enum PState
{
	CtrlAlt = 0,
	nil
};

/*
 * p: pointer to cur
 * p1: prev
 * lp: ptr to len of name
 * l: len of name
 * shift: offset for shifted keys or 0
 */
static int IsKeyName( unsigned char **p, unsigned char *p1, long *lp, long l, int shift )
{
	int i;
	for (i = XS_KEY_CURSOR_UP; i <= XS_KEY_UNDO; i++ )
	{
		if (strncmp((const char*)*p, xa_strings(i), l) == 0)
		{
			*p = p1;
			*lp = l + (shift ? 6 : 0);
			if (i == XS_KEY_SPACE)
				i = ' ';
			else if (i == XS_KEY_ESC)
				i = 0x1b;
			else
				i = i + 1 - XS_KEY_CURSOR_UP;
			return i + shift;
		}
	}
	return 0;
}

#define CA_SEP	0x01
static int DStart = 0;

static void enquote( unsigned char *p )
{
	int isquote = 0;
	unsigned char tb[256], *tp = tb, *ep;
	char *in = (char*)p;

	if( !((ep=(unsigned char *)strchr( (const char*)p, '#' )) && strchr( (const char*)ep+1, '#')) )
		return;
	ep = tb + sizeof(tb)-1;
	for( ; *p && tp < ep; p++ )
	{
		if( *p == '\\' )
		{
			if( isquote )
				*tp++ = '\\';
			isquote ^= 1;
			continue;
		}
		else if( *p == '#' && !isquote )
			*tp++ = CA_SEP;
		else
			*tp++ = *p;
		isquote = 0;
	}
	*tp = 0;
	strcpy( in, (const char*)tb );
}
static void unquote( unsigned char *p )
{
	for( ; p && (p=(unsigned char*)strchr( (char*)p, CA_SEP )); )
		*p++ = '#';
}
static int ScanKey( unsigned char **p, long *lp )
{
	unsigned char *p1;	// = *p1+1;
	int i, shift = 0;
	long l;

	p1 = (unsigned char *)strchr( (const char*)*p+1, CA_SEP );
	if( !p1 )
		return 0;
	if( !strncmp( (const char*)*p, "SHIFT+", 6 ) )
	{
		shift = HK_SHIFT;
		*p += 6;
		if( **p <= ' ' )
			return 0;
	}
	l = p1 - *p;
	if( l > 1 )
	{
		return IsKeyName( p, p1, lp, l, shift );
	}
	*lp = shift ? 7 : 1;
	i = **p;
	*p = p1;
	return i + shift;
}

static void PatchLn( unsigned char *ln, long lpo, long lpr )
{
	unsigned char *p;
	unsigned char *px = ln;
	if( !lpo )
		return;
	for( p = ln; lpo && *p; lpo--, p++ )
	;
	for( ; lpr && *p && *p != CA_SEP; lpr-- )
	{
		*ln++ = *p++;
	}
	p++;

	for( *ln++ = ' '; *ln && ln < p; )
		*ln++ = ' ';
	if( DStart == 0 && *ln == ' ' )
	{
		for( ; *ln == ' '; ln++ )
		;
		DStart = ln - px;
	}
	else if( DStart )
	{
		int pos = ln - px;
		pos -= DStart;
		if( pos > 0 )
		{
			for( p = ln - pos; *ln; )
				*p++ = *ln++;
			*p = 0;
		}
	}
}
typedef struct{
	unsigned short c;
}Keys;

int DoCtrlAlt( enum CtrlAltMd md, int orig, int repl )
{
	static Keys CtrlAltKeys[256+256] = {{0}};
	switch( md )
	{
	case Set:
	{
		if( repl <= 0 || orig <= 0 )
			return 1;
		repl--;
		orig--;
		//if( repl > sizeof(CtrlAltKeys) / sizeof(Keys) )
			//return 1;
		CtrlAltKeys[repl].c = orig + 1;
	}
	break;
	case Get:
		if( orig <= 0 )
			return 0;
		if( repl & (K_RSHIFT|K_LSHIFT) )
			orig += HK_SHIFT;
		orig--;
		//if( orig > sizeof(CtrlAltKeys) / sizeof(Keys) )
			//return 0;
		return CtrlAltKeys[orig].c;
	case Reset:
		memset( CtrlAltKeys, 0, sizeof(CtrlAltKeys) );
	break;
	}
	return 0;
}

static void SetCtrlAlt( unsigned char *p )
{
	int org, repl;
	long lpo, lpr;
	unsigned char *p1 = p;

	if( !( org = ScanKey( &p, &lpo ) ) || *p != CA_SEP )
	{
		if( DStart == 0 )
		{
			for( ; *p > ' '; p++ )
			;
			for( p++; *p == ' '; p++ )
			;
			DStart = p - p1;
		}
		return;
	}
	p++;
	if( *p == CA_SEP )	// free shortcut
	{
		DoCtrlAlt( Set, HK_FREE, org );
		*p1 = 0;
		return;
	}
	if( !( repl = ScanKey( &p, &lpr ) ) )
		return;
	PatchLn( p1, lpo + 1, lpr + 1 );
	DoCtrlAlt( Set, org, repl );
}

static enum PState GetPState( unsigned char *p )
{
	static char *Snames[] = { "CtrlAlt", 0};
	int i;
	long l;
	unsigned char *p1 = (unsigned char *)strchr( (char*)p, '>' );

	if( !p1 )
		return nil;	//ERROR
	l = p1 - p;
	for( i = 0; Snames[i]; i++ )
	{
		if( !strncmp( (char*)p, Snames[i], l ) )
			return i;
	}
	return nil;
}
#endif
/*
 * insert lines of a text-file into a scroll-list
 * if first non-white char is '#' line is skipped
 * line-endings must be unix-or dos-style
 *
 *
 */
static void file_to_list( SCROLL_INFO *list, char *fn, bool skip_hash, bool open)
{
	struct scroll_content sc = {{ 0 }};
	struct stat st;
	long r = f_stat64(0, fn, &st);
	XA_FILE *xa_fp;
	unsigned char *p;
	long lineno;
	unsigned char lbuf[256];
#if ALT_CTRL_APP_OPS
	enum PState pstate = nil;
	DStart = 0;
#endif

	if( r || !S_ISREG(st.mode) )
		return;
	xa_fp = xa_fopen(fn, O_RDONLY );
	if( !xa_fp )
		return;

	sc.t.strings = 1;
	//sc.fnt = &norm_txt;
	sc.fnt = 0;

	PRINIT;
	for( lineno = 1; lineno < VIEW_MAX_LINES && xa_readline( (char*)lbuf, sizeof(lbuf), xa_fp ); lineno++ )
	{
		p = lbuf;
		sc.t.text = (char*)p;
		if( skip_hash == true )
		{
			for( ; *p && *p <= ' '; p++ )
			;
			if( *p == '#' )
			{
				switch( *(p+1) )
				{
#if ALT_CTRL_APP_OPS
				case '$':
					switch( *(p+2) )
					{
					case '1':
						if( open && cfg.keyboards.c )
							add_keybd_switch(xa_strings(SW_DEFAULT));
					break;
					}
				continue;

				case '<':
					if( *(p+2) == '/' )
						pstate = nil;
					else
						pstate = GetPState( p+2 );
				continue;
#endif
				default:	// comment
				continue;
				}
			}
		}
#if ALT_CTRL_APP_OPS
		if( pstate == CtrlAlt && *p )
		{
			enquote(p);
			SetCtrlAlt( p );
			if( !*p )
				{BLOG((0,"file_to_list:*p=0(#%ld)", lineno));continue;}
			unquote(p);
		}
#endif
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
open_about(int lock, struct xa_client *client, bool open, char *fn)
{
	struct helpthread_data *htd;
	struct xa_window *wind;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	GRECT or;
	GRECT *rem;
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

	if (view_file)
		rem = &view_r;
	else
		rem = &about_r;

	if (view_file || !htd->w_about)
	{
		short minw, minh, dw = 0, dh = 0;

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, ABOUT_XAAES), 0);
		if (!obtree) goto fail;

		if( view_file )
		{
			int i;
			short h = screen.c_max_h / 2, h0 = obtree->ob_height;
			(obtree + ABOUT_LIST)->ob_y -= h * 6;
			(obtree + ABOUT_LIST)->ob_height = h0 - h * 6;

			(obtree + ABOUT_OK)->ob_y += h * 2;//3 - 2;
			(obtree + ABOUT_OK)->ob_height -= h;
			for( i = ABOUT_VERSION - 1; i <= ABOUT_TARGET; i++ )
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

		obj_rectangle(wt, aesobj(obtree, 0), &or);
		minh = obtree->ob_height;	/* minimum height for this window */
		minw = obtree->ob_width;	/* minimum width for this window */

		obj_init_focus(wt, OB_IF_RESET);

		/* Work out sizing */
		if (!rem->g_w)
		{
			center_rect(&or);
			*rem = calc_window(lock, C.Aes, WC_BORDER,
				BORDER|CLOSER|NAME|TOOLBAR|(C.Aes->options.xa_nomove ? 0 : MOVER),
				created_for_AES,
				C.Aes->options.thinframe,
				C.Aes->options.thinwork, &or);
		}
		else
		{
			//if( !view_file )
			{
				dw = rem->g_w - obtree->ob_width - 4;	/* !! */
				dh = rem->g_h - obtree->ob_height - 32;	/* !! */
			}
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
					rem, 0, NULL);

		if (!wind) goto fail;
		wind->parent = TOP_WINDOW;
		if( view_file )
			htd->w_view = wind;

		wind->min.g_h = minh;
		wind->min.g_w = minw;
#if 1
		obtree[ABOUT_LIST].ob_width += dw;
		obtree[ABOUT_LIST].ob_height += dh;
		obtree[ABOUT_VERSION-1].ob_y += dh;
		obtree[ABOUT_VERSION].ob_y += dh;
		obtree[ABOUT_TARGET-1].ob_y += dh;
		obtree[ABOUT_TARGET].ob_y += dh;
		obtree[ABOUT_DATE-1].ob_y += dh;
		obtree[ABOUT_DATE].ob_y += dh;
		obtree[ABOUT_OK].ob_y += dh;
		obtree[ABOUT_OK].ob_x += dw;
#endif
		set_slist_object(0, wt, wind, ABOUT_LIST, SIF_AUTOSLIDERS | SIF_INLINE_EFFECTS | SIF_AUTOSELECT,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, 255);

		if( !view_file )
		{
			(obtree + ABOUT_VERSION)->ob_spec.free_string = (char *)aes_version_verbose;
			/* Set version date */
#if !XAAES_RELEASE
			(obtree + ABOUT_DATE)->ob_spec.free_string = BDATETIME;
#else
			(obtree + ABOUT_DATE)->ob_flags |= OF_HIDETREE;
			(obtree + ABOUT_DATE-1)->ob_flags |= OF_HIDETREE;
#endif
			(obtree + ABOUT_TARGET)->ob_spec.free_string = arch_target;
		}

		wt = set_toolbar_widget(lock, wind, wind->owner, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		wt->exit_form = about_form_exit;

		/* WM_SIZED resizes list-window (though not open yet!)*/

		wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
				WM_SIZED, 0,0, wind->handle, rem->g_x, rem->g_y, rem->g_w, rem->g_h );
		if( !view_file )
			htd->w_about = wind;
	}
	else{
		if( !view_file )
			wind = htd->w_about;
		wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
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
		sprintf( ebuf, sizeof(ebuf), "%s%s", C.start_path, xa_strings(XA_HELP_FILE));
		set_window_title(wind, xa_strings(RS_ABOUT), true);
		alist = list;
	}

	/* check if help-file has changed and if yes re-read */
	if (list->start)
	{
		struct stat st;
		if( f_stat64( 0, ebuf, &st ) )
		{
			BLOG((0,"about:cannot stat %s", ebuf));
		}
		else
		{
			if( view_file || xah_mtime.time < st.mtime.time )
			{
				list->empty(list, 0, 0);
				list->start = 0;
				xah_mtime.time = st.mtime.time;
				memset( athis, 0, sizeof(athis) );
#if ALT_CTRL_APP_OPS
				if( view_file == false )
					DoCtrlAlt( Reset, 0, 0 );
#endif
			}
		}
	}
#endif
	if (!list->start)
	{
		struct scroll_content sc = {{ 0 }};
		int i;

		sc.t.strings = 1;
		if( view_file == false )
		{
			for (i = ABOUT_LINE0; i <= ABOUT_LINE7; i++)
			{
				if (!FN_CRED && i <= ABOUT_LINE2)
					continue;
				sc.t.text = xa_strings(i);
				list->add(list, NULL, NULL, &sc, false, 0, false);
			}
		}
#if HELPINABOUT
		file_to_list( list, ebuf, view_file == false, open );
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
