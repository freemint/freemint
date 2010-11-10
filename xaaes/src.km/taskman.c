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

/*
 * Task Manager Dialog
 * Error message dialog
 * System menu handling
 */

#include RSCHNAME

#include "xa_types.h"
#include "xa_global.h"

#include "about.h"
#include "app_man.h"
#include "c_window.h"
#include "form.h"
#include "k_main.h"
#include "k_mouse.h"
#include "draw_obj.h"
#include "menuwidg.h"
#include "obtree.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_appl.h"
#include "xa_form.h"
#include "xa_graf.h"
#include "xa_menu.h"
#include "xa_shel.h"
#include "xa_rsrc.h"
#include "xa_fsel.h"
#include "trnfm.h"

#include "mvdi.h"

#include "mint/signal.h"
#include "mint/stat.h"
#include "mint/fcntl.h"
#include "mint/ssystem.h"

#define NEWLOAD 1
#ifndef USE_Suptime
#define USE_Suptime 1
#endif

#define ADDPROCINFO	0

char XAAESNAME[] = "XaAES";

static void add_kerinfo( char *path, struct scroll_info *list, struct scroll_entry *this, struct scroll_entry *to, struct scroll_content *sc, int maxlen, int startline, int redraw, long *pinfo );
static int ker_stat( int pid, char *what, long pinfo[] );


	/* text-len for lists seems to be limited */
#define PROCINFLEN	148

/* wait for non-aes-process to be killed */
#define TM_KILLLOOPS	2000
#define TM_KILLWAIT		15000
/* dont signal these pids */
#define TM_MAXPID		1000
#define TM_MINPID		3

#define TM_WINDOW		0x01L	/* window-entry */
#define TM_MEMINFO	0x02L	/* extra line for memory-info */
#define TM_UPDATED	0x04L	/* was updated */
#define TM_NOAES		0x08L	/* not an aes-client */
#define TM_PROCINFO	0x10L	/* process-info (if def'd) */
#define TM_HEADER		0x20L	/* header */
#define TM_CLIENT		0x40L	/* aes-client-entry in taskman-list */

#define SYSLOGMINH	240	/* syslog cannot have smaller height */

#if USE_Suptime
#ifdef trap_14_w
#undef trap_14_w	/* "redefined" warning */
#endif
#include <mintbind.h>	/* Suptime */
#endif

#if 0
//void ask_and_shutdown( enum locks lock, struct xa_client *client, bool b);

/*void ask_and_shutdown( enum locks lock, struct xa_client *client, bool b)
{
	short r = xaaes_do_form_alert( lock, client, 1, ASK_SHUTDOWN_ALERT);
	if ( r == 2 )
			dispatch_shutdown(0, 0);
}
*/
#endif

/*static*/ struct xa_wtxt_inf norm_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL},	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_WHITE, G_BLACK, G_WHITE,   0,      0,     NULL},	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL},	/* Highlighted */

};

static struct xa_wtxt_inf acc_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLUE, G_LBLUE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_WHITE/*BLUE*/, G_LBLUE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf prg_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_LRED, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_WHITE/*RED*/, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf sys_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1,  0, MD_TRANS, 0, G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1,  0, MD_TRANS, 0, G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1,  0, MD_TRANS, 0, G_BLACK, G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf sys_thrd =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_LWHITE, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK,  G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static struct xa_wtxt_inf desk_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wrm,     efx   fgc      bgc   banner x_3dact y_3dact texture */
 {  -1,  -1, 0, MD_TRANS, 0, G_RED,   G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0, MD_TRANS, 0, G_RED,   G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0, MD_TRANS, 0, G_BLACK, G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

static void
delete_htd(void *_htd)
{
	struct helpthread_data *htd = _htd;

	if (htd->w_about)
	{
		close_window(0, htd->w_about);
		delete_window(0, htd->w_about);
	}
	if (htd->w_taskman)
	{
		close_window(0, htd->w_taskman);
		delete_window(0, htd->w_taskman);
	}
	if (htd->w_sysalrt)
	{
		close_window(0, htd->w_sysalrt);
		delete_window(0, htd->w_sysalrt);
	}
	if (htd->w_reschg)
	{
		close_window(0, htd->w_reschg);
		delete_window(0, htd->w_reschg);
	}
	kfree(htd);
}

/*
 * simulate click on list
 */
static void init_list_focus( OBJECT *obtree, short item, short y )
{
	struct moose_data md = {0};
	md.y = y;
	click_scroll_list(0, obtree, item, &md);
}

/*
 */
short set_xa_fnt( int pt, struct xa_wtxt_inf *wp[], OBJECT *obtree, int objs[], SCROLL_INFO *list )
{
	short i, w, h;
	short oldh, oldpt;
	struct xa_wtxt_inf *wpp;

	if( wp )
	{
		wpp = wp[0];
	}
	else
		wpp = &norm_txt;


	if( pt != wpp->n.p )
	{
		if( wpp->n.p != -1 )
		{
			oldpt = wpp->n.p;
			if( pt > wpp->n.p )
				i = 1;
			else
				i = -1;

			for( C.Aes->vdi_settings->api->text_extent(C.Aes->vdi_settings, "X", &wpp->n, &w, &h), oldh = h;
					oldh == h && wpp->n.p < 66 && wpp->n.p > 0;)
			{
				wpp->n.p += i;
				C.Aes->vdi_settings->api->text_extent(C.Aes->vdi_settings, "X", &wpp->n, &w, &h);
			}

			if( h == oldh )
				return wpp->n.p = oldpt;

			pt = wpp->n.p;
		}
		if( wp )
			for( i = 0; wp[i]; i++ )
			{
				wp[i]->n.p = wp[i]->s.p = wp[i]->h.p = pt;
			}
	}
	C.Aes->vdi_settings->api->text_extent(C.Aes->vdi_settings, "X", &wpp->n, &w, &h);

	/* todo: need smaller/greater icons */
	if( objs && obtree )
		for( i = 0; objs[i]; i++ )
		{
			object_get_spec(obtree + objs[i])->iconblk->ib_hicon = h;
		}

	if( list )
	{
		list->nesticn_h = h;// + 2;
		list->char_width = 0;
	}
	return wpp->n.p;
}

struct helpthread_data *
get_helpthread_data(struct xa_client *client)
{
	struct helpthread_data *htd;

	htd = lookup_xa_data_byname(&client->xa_data, HTDNAME);
	if (!htd)
	{
		if ((htd = kmalloc(sizeof(*htd))))
		{
			bzero(htd, sizeof(*htd));
			add_xa_data(&client->xa_data, htd, 0, HTDNAME, delete_htd);
		}
	}
	return htd;
}

static long uptime = 99;	/* system-uptime */

/*
 * md = 1: aes-client
 * md = 0: no aes-client
 */

static char *
build_tasklist_string( int md, void *app)
{
	long tx_len = 148;
	char *tx;

	tx = kmalloc(tx_len);

	if (tx && app)
	{
		struct proc *p;
		/*unsigned*/ char *name, c=0, *cp;
		long pinfo[4], utim, ptim;
		static char *state_str[] = {"Cur ", "Run ", "Wait", "IO  ", "Zomb", "TSR ", "STOP", "Slct"};

		if( md == AES_CLIENT  )
		{
			p = ((struct xa_client *)app)->p;
			name = ((struct xa_client *)app)->name;
			c = name[15];
			name[15] = 0;
		}
		else
		{
			p = app;
			name = p->name;
		}

		if( !p )
			return NULL;
		if( p->pid )
		{
			pinfo[0] = 23;
			pinfo[1] = 0;

			if( ker_stat( p->pid, "stat", pinfo ) )
				pinfo[0] = 0;
		}
		else
			pinfo[0] = 0;	/* how to get size of kernel? */

		utim = p->systime + p->usrtime;
		if( uptime )
		{
			if( uptime > 1000000L )
			{
				ptim = utim * 10L / (uptime/10L);
			}
			else
				ptim = utim * 100L / uptime;
		}
		else
			ptim = 0L;

		for( cp = p->cmdlin; *cp > ' '; )
			if( *cp++ == 0x7f )
				break;
		for( ; *cp && *cp <= ' '; cp++ )
		;

		sprintf(tx, tx_len, "%16s %4d %4d %4d %3d   %c   %s %8ld %11ld %2ld %s",
			name, p->pid, p->ppid, p->pgrp, p->curpri, p->domain == 0?'T':'M',
			state_str[p->wait_q], pinfo[0],
			utim, ptim,
			cp
		);

		if( md == AES_CLIENT )
			name[15] = c;
	}
	return tx;
}

void add_window_to_tasklist(struct xa_window *wi, const char *title)
{
	if (wi && (wi->window_status & XAWS_OPEN)
		&& !( wi->dial
			& ( created_for_SLIST
			| created_for_CALC
			| created_for_POPUP
			| created_for_ALERT
			))
	)
	{
		struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
		struct scroll_entry *this;
		struct xa_window *wind;

		if (!htd)
			return;
		wind = htd->w_taskman;

		if (wind && wind != wi)
		{
			struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
			OBJECT *obtree = wt->tree; //ResourceTree(C.Aes_rsc, TASK_MANAGER);
			SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
			struct sesetget_params p = { 0 };

			/* search owner in list */
			p.idx = -1;
			p.arg.data = wi->owner;
			p.level.flags = 0;
			p.level.curlevel = 0;
			p.level.maxlevel = 0;
			list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
			this = p.e;

			if( this )
			{
				struct scroll_entry *thiswi = 0;

				p.idx = -1;
				p.arg.data = wi;
				p.level.flags = 0;
				p.level.curlevel = 0;
				p.level.maxlevel = 1;
				list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
				thiswi = p.e;
				if( title == (const char*)-1 )
				{
					if( !thiswi )
						return;
					/* window closed: delete list-entry */
					if( thiswi->prev )
						list->cur = thiswi->prev;
					else
						list->cur = this;
					list->del(list, thiswi, true);
					list->set(list, this, SESET_OPEN, 1, NORMREDRAW);
					list->set(list, list->cur, SESET_SELECTED, 0, NORMREDRAW);
				}
				else
				{
					if( thiswi )
					{
						/* change title */
						struct setcontent_text t = { 0 };

						if( wi->wname[0] )
						{
							t.text = wi->wname;
							list->set(list, thiswi, SESET_TEXT, (long)&t, true);
						}
					}
					else
					{
						/* new window */
						struct scroll_content sc = {{ 0 }};
						bool redraw = (wind->window_status & XAWS_OPEN) ? true : false;

						sc.t.text = wi->wname;
						if( !*sc.t.text )
							sc.t.text = (long)title & 1L ? 0 : title;
						if( !sc.t.text || !*sc.t.text )
							return;
						sc.t.strings = 1;
						sc.data = wi;
						sc.usr_flags = TM_WINDOW;	/* window */
						sc.fnt = &norm_txt;
						list->add(list, this, NULL, &sc, SEADD_CHILD, 0, redraw);

						list->set(list, this, SESET_OPEN, 1, NORMREDRAW);
						list->set(list, list->cur, SESET_SELECTED, 0, NORMREDRAW);
					}
				}
			}
		}
	}
}


#if ADDPROCINFO
static void
add_proc_info( int md, void *app,
	SCROLL_INFO *list,
	struct scroll_entry *this,
	struct scroll_content *sc
)
{
	char path[128];
	//struct scroll_content sc;
	struct scroll_entry *to;
	int pid;
	if( md == AES_CLIENT )
		pid = ((struct xa_client *)app)->p->pid;
	else if( md == NO_AES_CLIENT )
		pid = ((struct proc *)app)->pid;
	else
		return;
	sprintf( path, sizeof(path), "u:/kern/%d/status", pid );
	sc->usr_flags = TM_PROCINFO;
	sc->fnt = &norm_txt;
	to = this->down;
	add_kerinfo( path, list, this, to, sc, PROCINFLEN, 0, NORMREDRAW, NULL );
}
#endif
void
add_to_tasklist(struct xa_client *client)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	struct xa_window *wind;

	if (!htd)
		return;

	wind = htd->w_taskman;

	if (wind)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		OBJECT *obtree = wt->tree; //ResourceTree(C.Aes_rsc, TASK_MANAGER);
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		OBJECT *icon;
		char *tx, *cp;
		struct scroll_content sc = {{ 0 }};

// 		display("add2tl: wt = %lx, obtree = %lx, list =%lx", wt, obtree, list);
		if (screen.planes < 4)
			sc.fnt = &norm_txt;
		else if (client->p->pid == C.DSKpid)
			sc.fnt = &desk_txt;
		else if (client->type & APP_ACCESSORY)
			sc.fnt = &acc_txt;
		else if (client->type & APP_SYSTEM)
			sc.fnt = &sys_txt;
		else if (client->type & APP_AESTHREAD)
			sc.fnt = &sys_thrd;
		else if (client->type & APP_APPLICATION)
			sc.fnt = &prg_txt;
		else
			sc.fnt = &norm_txt;

		if (!list)
			return;

		if (client->type & APP_ACCESSORY)
			icon = obtree + TM_ICN_MENU;
		else
			icon = obtree + TM_ICN_XAAES;

		tx = build_tasklist_string(1, client);
		sc.icon = icon;
		cp = tx ? tx : client->name;
		sc.t.text = cp;
		sc.t.strings = 1;
		sc.data = client;
		sc.xflags = OF_AUTO_OPEN|OF_OPENABLE;
		sc.usr_flags = TM_CLIENT;	/* client */

		/* append at end of this type */

		{
			struct scroll_entry *this = 0, *last;

			for( last = 0, this = list->start; this; this = this->next )
				if( this->usr_flags & TM_CLIENT )
					last = this;

			this = last;
			list->add(list, this, NULL, &sc, false, 0, NORMREDRAW);
			list->set(list, list->cur, SESET_SELECTED, 0, NORMREDRAW);
		}

		if (tx) kfree(tx);

#if ADDPROCINFO
		{
			struct sesetget_params p = { 0 };
			struct scroll_entry *this;
			/* search owner in list */
			p.idx = -1;
			p.arg.data = client;
			p.level.flags = 0;
			p.level.curlevel = 0;
			p.level.maxlevel = 0;
			list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
			this = p.e;

			sc.t.text = "<proc>";
			sc.t.strings = 1;
			sc.data = client;
			sc.usr_flags = TM_PROCINFO;	/* proc-info */
			sc.fnt = &norm_txt;
			list->add(list, this, NULL, &sc, SEADD_CHILD|SEADD_PRIOR, 0, true);

			p.level.maxlevel = 1;
			(const char*)p.arg.txt = sc.t.text;
			list->get(list, this, SEGET_ENTRYBYTEXT, &p);
			this = p.e;
		if( this )
				add_proc_info( 1, client, list, this, &sc );

		}
#endif
	}
}

void
remove_from_tasklist(struct xa_client *client)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	struct xa_window *wind;

	if (!htd)
		return;
	wind = htd->w_taskman;

	if (wind)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		OBJECT *obtree = wt->tree; //ResourceTree(C.Aes_rsc, TASK_MANAGER);
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		struct sesetget_params p = { 0 };

		if (list)
		{
			p.arg.data = client;
			list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
			if (p.e)
			{
#if 0	//debugging-info: dont delete
				struct scroll_entry *this = p.e;
				BLOG((0,"remove_from_tasklist: flags=%lx", this->usr_flags ));
				if( this->usr_flags & TM_NOAES )
				{
					struct proc *pr = (struct proc *)client;
					BLOG((0,"remove_from_tasklist: delete: NOAES:%lx:%s:%lx", pr, this->content->c.text.text, p.e ));
				}
				else if( this->usr_flags == TM_CLIENT )
					BLOG((0,"remove_from_tasklist: delete CLIENT:%d:%s(%s):%lx", client->p->pid, client->name, client->p->name, p.e ));
#endif
				list->del(list, p.e, true);
			}
		}
	}
}

/*
 * md = 1: aes-client
 * md = 0: no aes-client
 */
void
update_tasklist_entry( int md, void *app, int redraw )
{
	struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	struct xa_window *wind;

	if (!htd)
		return;
	wind = htd->w_taskman;

	if (wind && app)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		OBJECT *obtree = wt->tree; //ResourceTree(C.Aes_rsc, TASK_MANAGER);
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		struct sesetget_params p = { 0 };
		struct scroll_content sc = {{ 0 }};

		if (list)
		{
			char *tx = 0;
			struct setcontent_text t = { 0 };

			p.arg.data = app;
			list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
			if (p.e)
			{
				struct scroll_entry *this = p.e;
				struct se_content *sec = this->content;

				if( md == AES_CLIENT )
				{
					if( *(((struct xa_client *)app)->name+2) <= ' ' )
					{
						remove_from_tasklist( app );
						return;
					}
				}

				if ((tx = build_tasklist_string(md, app)))
					t.text = tx;
				else
				{
					if( md == AES_CLIENT )
						t.text = ((struct xa_client *)app)->name;
					else
						t.text = ((struct proc *)app)->name;
				}
				if( !t.text )
					return;

				/* set only if changed */
				if( !sec || strcmp( t.text, sec->c.text.text ) )
				{
					list->set(list, this, SESET_TEXT, (long)&t, NORMREDRAW);
				}
				this->usr_flags |= TM_UPDATED;

#if ADDPROCINFO
				p.level.maxlevel = 1;
				(const char*)p.arg.txt = "<proc>";
				list->get(list, p.e, SEGET_ENTRYBYTEXT, &p);

				if( p.e )
				{
					struct scroll_content sc = {{ 0 }};
					sc.t.strings = 1;
					add_proc_info( md, app, list, p.e, &sc );
				}
#endif
			}
			else if( md == NO_AES_CLIENT )	/* add non-client */
			{
				//p.arg.data = app;

				sc.t.strings = 1;
				sc.data = app;
				sc.xflags = 0;
				sc.usr_flags = TM_UPDATED | TM_NOAES;
				//sc.icon = obtree + TM_ICN_MENU;
				if ((tx = build_tasklist_string(md, app)))
					t.text = tx;
				sc.t.text = t.text;

				sc.fnt = &norm_txt;

				list->add(list, NULL, NULL, &sc, false, 0, NORMREDRAW);
			}
			if (tx)
				kfree(tx);
		}
	}
}

// static struct xa_window *task_man_win = NULL;

bool
isin_namelist(struct cfg_name_list *list, char *name, short nlen, struct cfg_name_list **last, struct cfg_name_list **prev)
{
	bool ret = false;

	if (!nlen)
		nlen = strlen(name);

	DIAGS(("isin_namelist: find '%s'(len=%d) in list=%lx (name='%s', len=%d)",
		name, nlen, list,
		list ? list->name : "noname",
		list ? list->nlen : -1));

	if (last)
		*last = NULL;
	if (prev)
		*prev = NULL;

	while (list)
	{
		DIAGS((" -- checking list=%lx, name=(%d)'%s'",
			list, list->nlen, list->name));

		if (list->nlen == nlen && !strnicmp(list->name, name, nlen))
		{
			ret = true;
			break;
		}
		if (prev)
			*prev = list;

		list = list->next;
	}

	if (last)
		*last = list;

	DIAGS((" -- ret=%s, last=%lx, prev=%lx",
		ret ? "true":"false", last, prev));

	return ret;
}

void
addto_namelist(struct cfg_name_list **list, char *name)
{
	struct cfg_name_list *new, *prev;
	short nlen = strlen(name);

	DIAGS(("addto_namelist: add '%s' to list=%lx(%lx)", name, *list, list));

	if (nlen && !isin_namelist(*list, name, 0, NULL, &prev))
	{
		new = kmalloc(sizeof(*new));

		if (new)
		{
			if (nlen > 32)
				nlen = 32;

			bzero(new, sizeof(*new));

			if (prev)
			{
				DIAGS((" -- add new=%lx to prev=%lx", new, prev));
				prev->next = new;
			}
			else
			{
				DIAGS((" -- add first=%lx to start=%lx", new, list));
				*list = new;
			}
			strcpy(new->name, name);
			new->nlen = nlen;
		}
	}
}

#if INCLUDE_UNUSED
void
removefrom_namelist(struct cfg_name_list **list, char *name, short nlen)
{
	struct cfg_name_list *this, *prev;

	if (isin_namelist(*list, name, nlen, &this, &prev))
	{
		if (prev)
			prev->next = this->next;
		else
			*list = this->next;
		kfree(this);
	}
}
#endif
void
free_namelist(struct cfg_name_list **list)
{
	while (*list)
	{
		struct cfg_name_list *l = *list;
		*list = (*list)->next;
		DIAGS(("free_namelist: freeing %lx, next=%lx(%lx) name='%s'",
			l, *list, list, l->name));
		kfree(l);
	}
}

static int
taskmanager_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);

	if (htd)
		htd->w_taskman = NULL;
// 	task_man_win = NULL;
	return true;
}

void
send_terminate(enum locks lock, struct xa_client *client, short reason)
{
	if (client->type & APP_ACCESSORY)
	{
		DIAGS(("send AC_CLOSE to %s", c_owner(client)));

		/* Due to ambiguities in documentation the pid is filled out
		 * in both msg[3] and msg[4]
		 */
		send_app_message(lock, NULL, client, AMQ_CRITICAL, QMF_CHKDUP,
				 AC_CLOSE,    0, 0, client->p->pid,
				 client->p->pid, 0, 0, 0);
	}

	/* XXX
	 * should we only send if client->swm_newmsg & NM_APTERM is true???
	 */
	DIAGS(("send AP_TERM to %s", c_owner(client)));
	send_app_message(lock, NULL, client, AMQ_CRITICAL, QMF_CHKDUP,
			 AP_TERM,     0,       0, client->p->pid,
			 client->p->pid, reason/*AP_TERM*/, 0, 0);
}

static char ASK_QUITALL_ALERT[] = "[2][Quit All][Cancel|Ok]";

void
quit_all_apps(enum locks lock, struct xa_client *except, short reason)
{
	struct xa_client *client;
#if 0
	bool do_alert = true;

	if( except )
	{
		do_alert = false;
		if( except == (struct xa_client*)-1 )
			except = 0;
	}

	if ( do_alert == true && xaaes_do_form_alert( lock, C.Hlp, 1, ASK_QUITALL_ALERT ) != 2 )
	{
		return;
	}
#endif
	Sema_Up(clients);
	lock |= clients;

	FOREACH_CLIENT(client)
	{
		if (is_client(client) && client != C.Hlp && client != except)
		{
			DIAGS(("shutting down %s", c_owner(client)));
			send_terminate(lock, client, reason);
		}
	}

	Sema_Dn(clients);
}

#if ALT_CTRL_APP_OPS && 1	//HOTKEYQUIT
static void
quit_all_clients(enum locks lock, struct cfg_name_list *except_nl, struct xa_client *except_cl, short reason)
{
	struct xa_client *client, *dsk = NULL;

	Sema_Up(clients);
	lock |= clients;

	DIAGS(("quit_all_clients: name_list=%lx, except_client=%lx", except_nl, except_cl));
	/*
	 * '_aes_shell' is special. If it is defined, we lookup the pid of
	 * the shell (desktop) loaded by the AES and let it continue to run
	 */
	if (isin_namelist(except_nl, "_aes_shell_", 11, NULL, NULL))
	{
		dsk = pid2client(C.DSKpid);
		DIAGS((" -- _aes_shell_ defined: pid=%d, client=%lx, name=%s",
			C.DSKpid, dsk, dsk ? dsk->name : "no shell loaded"));
	}

	FOREACH_CLIENT(client)
	{
		if (is_client(client) &&
		    client != C.Hlp &&
		    client != except_cl &&
		    client != dsk &&
		    !isin_namelist(except_nl, client->proc_name, 8, NULL, NULL))
		{
			DIAGS(("Shutting down %s", c_owner(client)));
			send_terminate(lock, client, reason);
		}
	}
	Sema_Dn(clients);
}
void
ce_quit_all_clients(enum locks lock, struct xa_client *client, bool b)
{
	struct cfg_name_list *nl = NULL;


	if ( xaaes_do_form_alert( lock, C.Hlp, 1, ASK_QUITALL_ALERT ) != 2 )
	{
		return;
	}

	if (b)
		nl = cfg.ctlalta;
	quit_all_clients(lock, nl, NULL, AP_TERM);
}
#endif
void
CHlp_aesmsg(struct xa_client *client)
{
	union msg_buf *m;
	char alert[256];

	if (client->waiting_pb && (m = (union msg_buf *)client->waiting_pb->addrin[0]))
	{
		switch (m->m[0])
		{
			case 0x5354:
			{
// 				display("0x5354 gotten");
				if (m->m[3] == 1)
				{
					C.update_lock = NULL;
					C.updatelock_count--;
					unlock_screen(client->p);
// 					do_form_alert(0, client, 1, "[1][Snapper got videodata, screen unlocked][OK]", XAAESNAME);
				}
				else if (m->m[3] != 0)
				{
					sprintf(alert, sizeof(alert), /*scrn_snap_serr*/"[1][ Snapper could not save snap! | ERROR: %d ][ Ok ]", m->m[3]);
					do_form_alert(0, client, 1, alert, XAAESNAME);
				}
				break;
			}
			default:;
		}
	}
}


#if ALT_CTRL_APP_OPS
static char sdalert[] = /*scrn_snap_what*/ "[2][What do you want to snap?][Block|Full screen|Top Window|Cancel]";
void
screen_dump(enum locks lock, struct xa_client *client, bool open)
{
	struct xa_client *dest_client;

	if ((dest_client = get_app_by_procname("xaaesnap")) || cfg.snapper[0] )
	{
// 		display("send dump message to %s", dest_client->proc_name);
		if (update_locked() != client->p && lock_screen(client->p, true))
		{
			short msg[8] = {0x5354, client->p->pid, 0, 0, 0,0,200,200};
			RECT r, d = {0};
			short b = 0, x, y;
			bool doit = true, snapmsg = false;
			AESPB *a = C.Hlp_pb;

			C.update_lock = client->p;
			C.updatelock_count++;

			do_form_alert(lock, client, 4, sdalert, XAAESNAME);
			Block(client, 0);

// 			display("intout %d", C.Hlp_pb->intout[0]);

			if (a->intout[0] == 1) //(open)
			{
				xa_graf_mouse(THIN_CROSS, NULL,NULL, false);
				while (!b)
					check_mouse(client, &b, &x, &y);

				r.x = x;
				r.y = y;
				r.w = 0;
				r.h = 0;
				rubber_box(client, SE, r, &d, 0,0, root_window->r.h, root_window->r.w, &r);
			}
			else if (a->intout[0] == 2)	/* full */
				r = root_window->r;
			else if (a->intout[0] == 3)	/* top */
			{
				struct xa_window *wind = TOP_WINDOW;

				if ( !dest_client ||
						(wind->r.x > 0 && wind->r.y > 0 &&
				   (wind->r.x + wind->r.w) < root_window->r.w &&
				   (wind->r.y + wind->r.h) < root_window->r.h) )
					r = wind->r;
				else
				{
					C.updatelock_count--;
					C.update_lock = NULL;
					unlock_screen(client->p);
					do_form_alert(lock, client, 1, /*scrn_snap_twc*/"[1][Cannot snap topwindow as | parts of it is offscreen!][OK]", XAAESNAME);
					doit = false;
				}
			}
			else
				doit = false;

			if ((r.w | r.h) && doit) //C.Hlp_pb->intout[0] != 4)
			{
				msg[4] = r.x;
				msg[5] = r.y;
				msg[6] = r.w;
				msg[7] = r.h;
				if( dest_client )
				{
					send_a_message(lock, dest_client, AMQ_NORM, 0, (union msg_buf *)msg);
					snapmsg = true;
				}
				else
				{
					struct proc *p;
					long ret;
					char cmdlin[32] = "60 0 0\0";
					//*cmdlin = 6;
					//*(cmdlin + 6) = 0;
					/* <wait> <key> <verbose>
					 * key: 0 -> screen
					 	      2 -> top-window: work-area
					 	      6 -> top-window: whole-area
					 */
					if (a->intout[0] == 3)	/* top window */
						*(cmdlin + 3) = '2';
					else if (a->intout[0] == 1)	/* box */
					{
						int i = sprintf( cmdlin + 6, sizeof(cmdlin)-1, " %d %d %d %d\0", r.x, r.y, r.w, r.h );
						*(cmdlin + 3) = '8';
						*cmdlin = i + 6;
					}
					else
						*(cmdlin + 3) = '0';	/* full screen */

					ret = create_process(cfg.snapper, cmdlin, NULL, &p, 0, NULL);

				}
// 				xa_graf_mouse(ARROW, NULL,NULL, false);
			}
			if( snapmsg == false )
			{
				C.updatelock_count--;
				C.update_lock = NULL;
				unlock_screen(client->p);
			}
			if( dest_client )
			{
				client->usr_evnt = 0;
				client->waiting_for = XAWAIT_MULTI|MU_MESAG;
				client->waiting_pb = a;
			}
		}
	}
	else
		do_form_alert(lock, client, 1, /*scrn_snap_notfound*/
"[1]['xaaesnap' process not found.|Start 'xaaesnap.prg' and try again|or define snapshot in xaaes.cnf][OK]",
	XAAESNAME);
}
#endif

void force_window_top( enum locks lock, struct xa_window *wind )
{
	if( S.focus )
	{
		setwin_untopped( lock, S.focus, true );
		S.focus->colours = S.focus->untop_cols;
		send_iredraw(lock, S.focus, 0, NULL);
		S.focus = 0;	/* force focus to new top */
	}
	TOP_WINDOW = 0;
	screen.standard_font_point = wind->owner->options.standard_font_point;
	if (is_hidden(wind))
		unhide_window(lock|winlist, wind, false);
	top_window( lock, true, true, wind );
}

/*
 * if client is STOPPED, send SIGCONT
 */
void wakeup_client(struct xa_client *client)
{
	if( client->p->wait_q == STOP_Q )
	{
		ikill( client->p->pid, SIGCONT );
	}
}

void app_or_acc_in_front( enum locks lock, struct xa_client *client )
{
	/* stolen from k_keybd.c#390: made a function for this */
	if ( client )
	{
		if( C.SingleTaskPid > 0 && client->p->pid != C.SingleTaskPid )
			return;
		//if( client->name[1] == '*' )
			//hide_app( lock, client );
		app_in_front(lock, client, true, true, true);

		if (client->type & APP_ACCESSORY)
		{
			//wakeup_client(client);
			send_app_message(lock, NULL, client, AMQ_NORM, QMF_CHKDUP,
					 AC_OPEN, 0, 0, 0,
					 client->p->pid, 0, 0, 0);
		}
	}
}

/******************
static void set_fnts(SCROLL_INFO *list, short pt)
{
	SCROLL_ENTRY *entry;
	for (entry = list->start; entry; entry = entry->next )
	{
		entry->fnt->n.p = entry->fnt->s.p = entry->fnt->h.p = pt;
	}
}

static void clear_list( SCROLL_INFO *list)
{
		while (list->start)
		{
			list->start = list->del(list, list->start, false);
		}
}


****************/

/*
 * todo: if fileselector is open during shutdown there's a problem
 */
static void
taskmanager_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;
	short item = aesobj_item(&fr->obj);
	struct xa_client *client = NULL; //cur_client(list);
	OBJECT *ob;
	SCROLL_INFO *list;

	Sema_Up(clients);
	lock |= clients;


	wt->which = 0;

	ob = wt->tree + TM_LIST;
	list = object_get_slist(ob);

	if( item != TM_LIST )
	{
		object_deselect(wt->tree + item);
		redraw_toolbar(lock, wind, item);
		if( item < XAAES_32 )
		{
			if (list && list->cur)
			{
				client = list->cur->data;
				if( (list->cur->usr_flags & TM_WINDOW) )
					client = ((struct xa_window*)client)->owner;
			}
			else
			{
				return;
			}
		}
	}

	switch (item)
	{
		case TM_LIST:
		{
			OBJECT *obtree = wt->tree;

			DIAGS(("taskmanager_form_exit: Moved the shit out of form_do() to here!"));
			if ( fr->md && ((aesobj_type(&fr->obj) & 0xff) == G_SLIST))
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, obtree, item, fr->md);
				else
					click_scroll_list(lock, obtree, item, fr->md);
			}
			break;
		}
		case TM_KILL:
		{
			if( (list->cur->usr_flags & TM_WINDOW) )
				break;

			if( list->cur->usr_flags & TM_NOAES )
			{
				long i, k = 0;
				int pid = ((struct proc*)client)->pid;
				if( pid <= TM_MINPID || pid >= TM_MAXPID || pid == C.Aes->tp->pid )
					break;
				ikill(pid, SIGKILL);
				for( i = 0; i < TM_KILLLOOPS && !k; i++ )
				{
					nap( TM_KILLWAIT );
					k = ikill(pid, 0);
				}
				if( k )
					remove_from_tasklist( client );
			break;
			}

			DIAGS(("taskmanager: KILL for %s", c_owner(client)));
			if (client && is_client(client))
			{
				if (client->type & (APP_AESTHREAD|APP_AESSYS))
				{
					break;
				}
				else
					ikill(client->p->pid, SIGKILL);
			}

			break;
		}
		case TM_TERM:
		{

			if( list->cur->usr_flags & TM_NOAES )
			{
				int pid = ((struct proc*)client)->pid;
				if( pid <= TM_MINPID || pid >= TM_MAXPID || pid == C.Aes->tp->pid )
					break;
				ikill(pid, SIGTERM);
			}
			else{
				if (client && is_client(client))
				{
					DIAGS(("taskmanager: TM_TERM for %s", c_owner(client)));
					if (client->type & (APP_AESTHREAD|APP_AESSYS))
					{
						break;
					}
					else
					{
						send_terminate(lock, client, AP_TERM);
					}
				}
			}

			/*
			object_deselect(wt->tree + TM_TERM);
			redraw_toolbar(lock, wind, TM_TERM);
			*/
			break;
		}
		case TM_SLEEP:
		{

			if( list->cur->usr_flags & TM_NOAES )
			{
				int pid = ((struct proc*)client)->pid;
				if( pid <= TM_MINPID || pid >= TM_MAXPID || pid == C.Aes->tp->pid )
					break;
				ikill(pid, SIGSTOP);
				update_tasklist_entry( NO_AES_CLIENT, client, true);
			}
			else if (client && is_client(client))
			{
				DIAGS(("taskmanager: TM_SLEEP for %s", c_owner(client)));
				if (client->type & (APP_AESTHREAD|APP_AESSYS|APP_ACCESSORY))
				{
					break;
					//ALERT(/*kill_aes_thread*/("Not a good idea, I tell you!"));
				}
				app_in_front(lock, C.Aes, true, true, true);
				ikill(client->p->pid, SIGSTOP);
				update_tasklist_entry( AES_CLIENT, client, true);
			}
		break;
		}
		case TM_WAKE:
		{
			if( list->cur->usr_flags & TM_NOAES )
			{
				int pid = ((struct proc*)client)->pid;
				if( pid <= TM_MINPID || pid >= TM_MAXPID )
					break;
				ikill(pid, SIGCONT);
				update_tasklist_entry( NO_AES_CLIENT, client, true);
			}
			else if (client && is_client(client))
			{
				if (client->type & (APP_AESTHREAD|APP_AESSYS))
				{
					break;
				}
				ikill(client->p->pid, SIGCONT);
				update_tasklist_entry( AES_CLIENT, client, true);
			}
		break;
		}

		/* the bottom action buttons */
		case TM_QUITAPPS:
		{

		/*
			object_deselect(wt->tree + TM_QUITAPPS);
			redraw_toolbar(lock, wind, TM_QUITAPPS);
			*/
			DIAGS(("taskmanager: quit all apps"));
			quit_all_apps(lock, NULL, AP_TERM);
			force_window_top( lock, wind );
			break;
		}
		case TM_QUIT:
		{
			/*
			object_deselect(wt->tree + TM_QUIT);
			redraw_toolbar(lock, wind, TM_QUIT);
			*/
			DIAGS(("taskmanager: quit XaAES"));
#if TM_ASK_BEFORE_SHUTDOWN
			post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, 0,1, NULL, NULL);
#else
			dispatch_shutdown(0, 0);
#endif
			//ask_and_shutdown( lock, C.Hlp, 0);
			force_window_top( lock, wind );

			break;
		}
		case TM_REBOOT:
		{
			/*
			object_deselect(wt->tree + TM_REBOOT);
			redraw_toolbar(lock, wind, TM_REBOOT);
			*/
			DIAGS(("taskmanager: reboot system"));
#if TM_ASK_BEFORE_SHUTDOWN
			post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, REBOOT_SYSTEM,1, NULL, NULL);
#else
			dispatch_shutdown(REBOOT_SYSTEM, 0);
#endif
			force_window_top( lock, wind );

			break;
		}
		case TM_HALT:
		{
			/*
			object_deselect(wt->tree + TM_HALT);
			redraw_toolbar(lock, wind, TM_HALT);
			*/
			Sema_Dn(clients);
#if 0
			close_window(lock, wind);
			if ( xaaes_do_form_alert( 0, C.Hlp, 1, ASK_SHUTDOWN_ALERT ) != 2 )
				goto lb_TM_OK;//break;
#endif
			DIAGS(("taskmanager: halt system"));

#if TM_ASK_BEFORE_SHUTDOWN
			post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, HALT_SYSTEM,1, NULL, NULL);
#else
			dispatch_shutdown(HALT_SYSTEM, 0);
#endif
			force_window_top( 0, wind );

			break;
		}
		case TM_COLD:
		{
			/*
			object_deselect(wt->tree + TM_COLD);
			redraw_toolbar(lock, wind, TM_COLD);
			*/
			DIAGS(("taskmanager: coldstart system"));
#if TM_ASK_BEFORE_SHUTDOWN
			post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, COLDSTART_SYSTEM,1, NULL, NULL);
#else
			dispatch_shutdown(COLDSTART_SYSTEM, 0);
#endif
			force_window_top( lock, wind );
			break;
		}
#if 1
		case TM_RESCHG:	/* now: toggle load-display */
		{
			OBJECT *obtree = wt->tree;
			obtree[TM_CHART].ob_flags ^= OF_HIDETREE;
			generate_redraws(lock, wind, &wind->r, RDRW_ALL);
			break;
		}
#endif
		case TM_OK:
		{

			if (list->cur && !(list->cur->usr_flags & (TM_MEMINFO | TM_NOAES)) && (long)list->cur->data > 1L )
			{
				TOP_WINDOW = 0;	/* ignore clwtna */
				/* and release */
				close_window(lock, wind);

				if( !(list->cur->usr_flags & TM_WINDOW) )
				{
					client = list->cur->data;
					if( client == C.Hlp )
						client = C.Aes;
					app_or_acc_in_front( lock, client );
				}
				else
				{
					struct xa_window *wi = list->cur->data;
					if( list->cur->data != wind )
					{
						if( !(wi->window_status & XAWS_OPEN)
							|| (wi->dial
								& ( created_for_SLIST
								| created_for_CALC
								| created_for_POPUP
								| created_for_ALERT
								)
							) )
						{
							app_or_acc_in_front( lock, wi->owner );
						}
						else
						{
							wakeup_client(wi->owner);
							force_window_top( lock, wi );
						}
					}
				}
			}
			else
				close_window(lock, wind);
// 			delayed_delete_window(lock, wind);

			break;
		}
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", fr->obj));
			break;
		}
	}
	Sema_Dn(clients);
}

/*
 * return true if AES-client AND valid client->name
 */
static bool is_aes_client( struct proc *p )
{
	struct xa_client *client;
	FOREACH_CLIENT(client)
	{
		if( client->p == p )
		{
			if( *(client->name+2) > ' ' )
				return true;
			else
				return false;
		}
	}
	return false;
}

#define OLD_MAXLOAD	4095L	/* ?? */
#define MAXLOAD	234L
#define WITH_ACTLD  0x4143544CL /* 'ACTL' */

static bool calc_tm_bar( OBJECT *obtree, short item, short ind, long pinfo[] )
{

	TEDINFO *t = object_get_tedinfo(&obtree[item], NULL);
	obtree[item].ob_height = (short)(pinfo[ind] * (long)obtree[TM_CHART].ob_height / MAXLOAD);

	/* if height = 0 a bar is drawn anyway!? */
	if( obtree[item].ob_height <= 0 )
		obtree[item].ob_height = 1;

	obtree[item].ob_y = obtree[TM_CHART].ob_height - obtree[item].ob_height;

	if( t )
	{
		OBJC_COLORWORD *c = (OBJC_COLORWORD *)&t->te_color;
		int v = obtree[item].ob_height * 100 / obtree[TM_CHART].ob_height;
		c->pattern = IP_SOLID;
		/* mark levels with different colors
		  found the color-values by Trial&Error ...
		*/
		if( v > 95 )
		{
			c->borderc = c->fillc = 3;	//magenta
			if( v > 100 )	// happens sometimes
			{
				c->borderc = c->fillc = 8;
				obtree[item].ob_height = obtree[TM_CHART].ob_height;
				//BLOG((0,"calc_tm_bar:%d: v=%d c->borderc=%d type=%dted=%lx color=%x", item, v, c->borderc, obtree[item].ob_type, t, t?t->te_color:0));
			}
		}
		else if( v > 75 )
		{
			c->borderc = c->fillc = 5;	// red
		}
		else if( v > 25 )
		{
			c->borderc = c->fillc = 6;	// blue
			//BLOG((0,"calc_tm_bar: type=%dted=%lx color=%x", obtree[item].ob_type, t, t?t->te_color:0));
		}
		else{

			//c->fillc = c->borderc = 0;	//black
			c->fillc = c->borderc = 7;	//green
		}
	}
	return true;
}

static void make_tm_ticks( OBJECT *obtree, int ticks[] )
{
	int i;
	ulong height = obtree[TM_CHART].ob_height;
	for( i = 0; ticks[i]; i += 3 )
	{
		obtree[ticks[i]].ob_y = (short)(10L * (height * (ulong)ticks[i+1] + 5L) / 1000L);
		obtree[ticks[i]].ob_x = -2;
		if( (obtree[ticks[i]].ob_height = ticks[i+2] ) > 1 )
			obtree[ticks[i]].ob_width = 5;
		else
			obtree[ticks[i]].ob_width = 4;
	}
}
/*
 * calculate actual load by the relation of time-difference and kernel-systime
 *
 * return load with maximum 234
 */
#ifdef trap_14_w
#undef trap_14_w	/* "redefined" warning */
#endif

#include <mintbind.h>	/* Tgettimeofday */

static long calc_new_ld(struct proc *rootproc)
{
	static long systime = 0, old_tim = 0, new_ld=0;
	long ud, utim, rsystime = rootproc->systime;
	struct timeval tv;

	Tgettimeofday( &tv, 0 );
	utim = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
	ud = utim - old_tim;
	old_tim = utim;

	if( ud > 0 )
	{
		new_ld = 5000L * (ud - (rsystime - systime)) / ud;
		if( new_ld > 0 )
		{
			/* maximum new_ld=234 */
			new_ld >>= 4;	 /* 5000/16=312 */
			new_ld -= (new_ld >> 2);  /* 312 - (312/4=78) */
			if( new_ld > 234 )
				new_ld = 234;
		}
		else
			new_ld = 0;
	}
	systime = rsystime;
	return new_ld;
}

void
open_taskmanager(enum locks lock, struct xa_client *client, bool open)
{
	RECT remember = { 0,0,0,0 };
	struct helpthread_data *htd;
	struct xa_window *wind;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	RECT or;
	int redraw = NOREDRAW;

	struct xa_wtxt_inf *wp[] = {&norm_txt, &acc_txt, &prg_txt, &sys_txt, &sys_thrd, &desk_txt, 0};
	int objs[] = {TM_ICN_MENU, TM_ICN_XAAES, 0};

	htd = get_helpthread_data(client);
	if (!htd)
		return;

	if (!htd->w_taskman) //(!task_man_win)
	{
		struct scroll_info *list;
		struct scroll_content sc = {{ 0 }};
		int tm_ticks[] = {TM_TICK1, 25, 1, TM_TICK2, 50, 2, TM_TICK3, 75, 1, 0, 0, 0};



		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, TASK_MANAGER), 0);
		if (!obtree) goto fail;

		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
#if 1
		/* resize window if < 10-point-font */
		if( screen.c_max_h < 16 ){
			short d = 16 / screen.c_max_h, dy;

			obtree->ob_height *= d;
			dy = (obtree+TM_LIST)->ob_height;

			(obtree+TM_LIST)->ob_height *= d;
			(obtree+TM_LIST)->ob_height += d/2;//12;
			(obtree+TM_LIST)->ob_y += obtree->ob_y + 18;

			dy = dy * (d  - 1) + 12;
			(obtree+TM_CHART)->ob_height *= d;
			(obtree+TM_CHART)->ob_y = (obtree+TM_LIST)->ob_y + (obtree+TM_LIST)->ob_height;

			dy = obtree->ob_y + obtree->ob_height - ((obtree + TM_OK)->ob_y + (obtree+TM_OK)->ob_height) - 8;
			for( d = TM_QUITAPPS; d < TM_CHART; d++ )
			{
				(obtree+d)->ob_y += dy;
			}
		}
#endif
		list = set_slist_object(0, wt, NULL, TM_LIST,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_TREEVIEW|SIF_ICONINDENT|SIF_AUTOOPEN|SIF_AUTOSLIDERS,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 /*tm_client_apps*/"Applications", NULL, NULL, 255);

		if (!list) goto fail;


		/*!obj_init_focus(wt, OB_IF_RESET);*/
		obj_rectangle(wt, aesobj(obtree, 0), &or);
		obtree[TM_ICONS].ob_flags |= OF_HIDETREE;

		set_xa_fnt( cfg.xaw_point, wp, obtree, objs, list);
		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, client, WC_BORDER,
						BORDER|CLOSER|NAME|TOOLBAR, created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						*(RECT *)&or);
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag, do_formwind_msg,
					client,//C.Aes,
					false,
					BORDER|CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
					created_for_AES,
					C.Aes->options.thinframe,
					C.Aes->options.thinwork,
					remember, NULL, NULL);

		if (!wind) goto fail;
		wind->min.h = remember.h * 2/3;	/* minimum height for this window */
		wind->min.w = remember.w;	/* minimum width for this window */
		list->set(list, NULL, SESET_PRNTWIND, (long)wind, NOREDRAW);
		wind->window_status |= XAWS_NODELETE;

		/* Set the window title */
		set_window_title(wind, /*tm_manager*/" Task Manager ", false);

		wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		wt->exit_form = taskmanager_form_exit;

		wt->focus = aesobj( obtree, TM_LIST );	/*cursor on list */
		init_list_focus( obtree, TM_LIST, 0 );

		make_tm_ticks( obtree, tm_ticks );

		/* Set the window destructor */
		wind->destructor = taskmanager_destructor;

		htd->w_taskman = wind;

		{
		long uinfo[4];
		sc.t.strings = 1;
		sc.data = (void*)1;
		sc.xflags = 0;
		sc.usr_flags = TM_MEMINFO;
		uinfo[0] = 1;
		uinfo[1] = (long)"used: ";
		uinfo[2] = 2;
		uinfo[3] = 0;
		add_kerinfo( "u:/kern/meminfo", list, NULL, NULL, &sc, PROCINFLEN, 5, NOREDRAW, uinfo );
		}

		sc.data = 0;
		sc.usr_flags = TM_HEADER;
		/* ! no tabs! */
		sc.t.text = "       name          pid  ppid pgrp pri DOM STATE    SZ           CPU  % args";
		sc.fnt = &norm_txt;
		list->add(list, NULL, NULL, &sc, false, 0, NOREDRAW);

		if (open)
		{
			open_window(lock, wind, wind->r);
		}
	}
	else
	{
		wind = htd->w_taskman;
		if (open)
		{
			SCROLL_INFO *list;
			long pinfo[5];
			struct scroll_content sc = {{ 0 }};
			static int has_new_uptime = -1;

			wt = get_widget(wind, XAW_TOOLBAR)->stuff;
			list = object_get_slist(wt->tree + TM_LIST);

			if( TOP_WINDOW == wind )
				redraw = NORMREDRAW;

			if( list )
			{
				struct sesetget_params p = { 0 };
				struct scroll_entry *this;
				struct proc *rootproc = pid2proc(0);
				long u = 0;

				/* todo: to change fnt-size at runtime font-info for each list-entry would have to be updated */
				//set_xa_fnt( cfg.xaw_point + fss, wp, wt->tree, objs, list);
				//set_fnts( list, cfg.xaw_point + fss );

				for( this = list->start; this; this = this->next )
					this->usr_flags &= ~TM_UPDATED;

				sc.t.strings = 1;
				sc.data = (void*)1;
				sc.xflags = 0;
				sc.usr_flags = TM_UPDATED;
				p.arg.data = (void*)1;
				list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
				if (p.e)
				{
					long uinfo[4];
					uinfo[0] = 1;
					uinfo[1] = (long)"used: ";
					uinfo[2] = 2;
					uinfo[3] = 0;
					add_kerinfo( "u:/kern/meminfo", list, NULL, p.e, &sc, PROCINFLEN, 5, redraw, uinfo );
				}

				if( !( wt->tree[TM_CHART].ob_flags & OF_HIDETREE) )
				{
					pinfo[0] = 1;
					pinfo[1] = 2;
					pinfo[2] = 0;
#if !USE_Suptime
					if( ker_stat( 0, "uptime", pinfo )
						|| ((u = pinfo[0] * 1000L + pinfo[1]) < uptime	/* /kern/uptime not always correct! :-*/
						))
					{
						uptime = 0L;
					}
					else
						uptime = u;
					//idle = pinfo[2] * 1000L + pinfo[3];
#else
					if( has_new_uptime == -1 )
					{
						pinfo[3] = WITH_ACTLD;	/* magic */
						pinfo[0] = WITH_ACTLD;
					}
					Suptime( &u, pinfo );	/* this is sometimes all 0 or constant max. on aranym ! :-( */
					if( has_new_uptime == -1 )
					{
						if( pinfo[3] == WITH_ACTLD )
							has_new_uptime = 0;
						else
							has_new_uptime = 1;
					}
					uptime = u * 1000L;
					if( has_new_uptime == 0 )
					{
						int i;
						for( i = 0; i < 3; i++ )
							pinfo[i] = (pinfo[i] * MAXLOAD) / OLD_MAXLOAD;
					}
					calc_tm_bar( wt->tree, TM_LOAD1, 0, pinfo );
					calc_tm_bar( wt->tree, TM_LOAD2, 1, pinfo );
					calc_tm_bar( wt->tree, TM_LOAD3, 2, pinfo );
					pinfo[3] = calc_new_ld( rootproc );
					calc_tm_bar( wt->tree, TM_ACTLD, 3, pinfo );

					redraw_toolbar( lock, wind, TM_CHART );
#endif
				}
				FOREACH_CLIENT(client)
				{				/* */
					update_tasklist_entry( AES_CLIENT, client, redraw);
				}
				/* add non-client-procs */
				{
					long i = d_opendir("u:/kern", 0);
					if (i > 0)
					{
						char nm[128];
						struct proc *pr;
						long pid;

						update_tasklist_entry( NO_AES_CLIENT, rootproc, redraw );	/* kernel */

						while( d_readdir(127, i, nm) == 0)
						{
							if( isdigit( nm[4] ) )
							{
								pid = atol(nm+4);
								/* check if prog really exists */
								if( !ikill( pid, 0 ) )
								{
									pr = pid2proc( pid );
									if( pr )
									{
										if( !is_aes_client(pr) )
										{
											update_tasklist_entry( NO_AES_CLIENT, pr, redraw );
										}
									}
								}
							}
						}
						d_closedir(i);
						for( this = list->start; this; this = this->next )
						{
							if( !(this->usr_flags & (TM_MEMINFO|TM_UPDATED|TM_HEADER) ) )
							{
								list->del( list, this, true );
							}
						}
					}
				}
			}	/* /if( list ) */

			//if( TOP_WINDOW != htd->w_taskman )
			{
				open_window(lock, wind, wind->r);
				force_window_top( lock, wind );
			}
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

/* ************************************************************ */
/*     Common resolution mode change functions/stuff		*/
/* ************************************************************ */
// static struct xa_window *reschg_win = NULL;
#if 0
static int
reschg_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_reschg = NULL;
// 	reschg_win = NULL;
	return true;
}
#endif

struct xa_window * _cdecl
create_dwind(enum locks lock, XA_WIND_ATTR tp, char *title, struct xa_client *client, struct widget_tree *wt, FormExit(*f), WindowDisplay(*d))
{
	struct xa_window *wind;
	OBJECT *obtree = wt->tree;
        RECT r, or;

	obj_rectangle(wt, aesobj(obtree, 0), &or);

	center_rect(&or);

	r = calc_window(lock, client, WC_BORDER,
			CLOSER|NAME, created_for_AES,
			client->options.thinframe,
			client->options.thinwork,
			*(RECT *)&or);

	/* Create the window */
	wind = create_window(lock,
				do_winmesag, do_formwind_msg,
				client,
				false,
				tp | (title ? NAME : 0) | TOOLBAR | hide_move(&(client->options)),
				created_for_AES,
				client->options.thinframe,
				client->options.thinwork,
				r, NULL, NULL);

	/* Set the window title */
	if (title)
		set_window_title(wind, title, false);

	wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
	wt->exit_form = f; //milan_reschg_form_exit;

	/* Set the window destructor */
	wind->destructor = d; //reschg_destructor;

	return wind;
}

/*
 * client still running dialog
 */
// static struct xa_window *csr_win = NULL;

static int
csr_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_csr = NULL;
// 	csr_win = NULL;
	return true;
}

static void
csr_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	enum locks lock = 0;

	Sema_Up(clients);
	lock |= clients;

// 	wt->current = fr->obj;
	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
		case KORW_WAIT:
		{
			if (C.csr_client)
			{
				send_terminate(lock, C.csr_client, (C.shutdown & RESOLUTION_CHANGE) ? AP_RESCHG : AP_TERM);
				set_shutdown_timeout(3000);
				C.csr_client = NULL;
			}
			else
				set_shutdown_timeout(1);

			object_deselect(wt->tree + KORW_WAIT);
			redraw_toolbar(lock, wind, KORW_WAIT);
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			break;
		}
		case KORW_KILL:
		{
			object_deselect(wt->tree + KORW_KILL);
			redraw_toolbar(lock, wind, KORW_KILL);

			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			csr_destructor( lock, wind );	/* window is deleted ... */
			if (C.csr_client)
			{
				C.csr_client->status |= CS_SIGKILLED;
				ikill(C.csr_client->p->pid, SIGKILL);
			}
			C.csr_client = NULL;
			set_shutdown_timeout(0);
			break;
		}
		case KORW_KILLEMALL:
		{
			object_deselect(wt->tree + KORW_KILLEMALL);
			redraw_toolbar(lock, wind, KORW_KILLEMALL);
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			C.shutdown |= KILLEM_ALL;
			set_shutdown_timeout(0);
			C.csr_client = NULL;
			break;
		}
	}
	Sema_Dn(clients);
}

static void
open_csr(enum locks lock, struct xa_client *client, struct xa_client *running)
{
	struct xa_window *wind;
	struct helpthread_data *htd;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;

	if (!(htd = get_helpthread_data(client)))
		return;

	if( running->p->pid == 0 )	/*!??*/
		return;


	if (!htd->w_csr) // !csr_win)
	{
		TEDINFO *t;
		int i = 0;

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, KILL_OR_WAIT), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		C.csr_client = running;

		t = object_get_tedinfo(obtree + KORW_APPNAME, NULL);
		if (running->name[0])
		{
			char *s = running->name;

			while (*s && *s == ' ')
				s++;

			for (; i < 32 && (t->te_ptext[i] = *s++); i++)
				;
		}
		if( i == 0 )
		{
			if( running->proc_name[0] > ' ' )
			{
				strncpy(t->te_ptext, running->proc_name, 8);
			}
			else
				strncpy(t->te_ptext, running->p->name, 8);
			i = 8;
		}
		sprintf( t->te_ptext +i, 32 - i, "(%d)", running->p->pid );

		wind = create_dwind(lock, 0, /*txt_shutdown*/" Shutdown ", client, wt, csr_form_exit, csr_destructor);
		if (!wind)
			goto fail;
		htd->w_csr = wind;
		wind->window_status |= XAWS_FLOAT;
		open_window(lock, wind, wind->r);
	}
	else
	{
		wind = htd->w_csr;
		open_window(lock, wind, wind->r);
	}
	if( wind )
		force_window_top( lock, wind );

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

void
CE_open_csr(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		open_csr(0, ce->client, ce->ptr1);
	}
}

void
CE_abort_csr(enum locks lock, struct c_event *ce, bool cancel)
{
	if (!cancel)
	{
		struct helpthread_data *htd = lookup_xa_data_byname(&ce->client->xa_data, HTDNAME);

		if (htd && htd->w_csr)
		{
			close_window(lock, htd->w_csr);
			delayed_delete_window(lock, htd->w_csr);
			C.csr_client = NULL;
		}
	}
}

static bool
cancelcsr(struct c_event *ce, long arg)
{
	if ((long)C.csr_client == arg)
	{
		C.csr_client = NULL;
		return true;
	}
	return false;
}
void
cancel_csr(struct xa_client *running)
{
	cancel_CE(C.Hlp, CE_open_csr, cancelcsr, (long)running);
}

static void
handle_launcher(enum locks lock, struct fsel_data *fs, const char *path, const char *file)
{
	char parms[200];//, *t;

	sprintf(parms+1, sizeof(parms)-1, "%s%s", path, file);
	parms[0] = '\0';
#if 0
	for(t = parms + 1; *t; t++)
	{
		if (*t == '/')
			*t = '\\';
	}
#endif
	close_fileselector(lock, fs);

	DIAGS(("launch: \"%s\"", parms+1));
	//sprintf(cfg.launch_path, sizeof(cfg.launch_path), "%s%s", path, fs->fs_pattern);
	launch(lock, 0, 0, 0, parms+1, parms, C.Aes);
}

#if FILESELECTOR

static struct fsel_data aes_fsel_data;

void
open_launcher(enum locks lock, struct xa_client *client)
{
	struct fsel_data *fs;

	/* if lauchpath defined but is no path discard it */
	if (*cfg.launch_path)
	{
		struct stat st;
		long r;
		//char *p = strchr( cfg.launch_path, '*' );
		char *p = strrchr( cfg.launch_path, '\\' ), c=0;

		if( !p )
			p = strrchr( cfg.launch_path, '/' );
		if( p )
		{
			c = *p;
			*p = 0;
		}

		r = f_stat64(0, cfg.launch_path, &st);
		if (r != 0 || !S_ISDIR(st.mode) )
			*cfg.launch_path = 0;
		else if( p )
			*p = c;
	}
	if (!*cfg.launch_path)
	{
		cfg.launch_path[0] = d_getdrv() + 'a';
		cfg.launch_path[1] = ':';
		cfg.launch_path[2] = '\\';
		cfg.launch_path[3] = '*';
		cfg.launch_path[4] = 0;
	}

	fs = &aes_fsel_data;
	open_fileselector(lock, client, fs,
			  cfg.launch_path,
			  NULL, /*txt_launch_prg*/"Launch Program",
			  handle_launcher, NULL, NULL);
}
#endif



// static struct xa_window *systemalerts_win = NULL;

/* double click now also available for internal handlers. */
static void
sysalerts_form_exit(struct xa_client *Client,
		    struct xa_window *wind,
		    struct widget_tree *wt,
		    struct fmd_result *fr)
{
	enum locks lock = 0;
	short item = aesobj_item(&fr->obj);

	switch (item)
	{
		case SYSALERT_LIST:
		{
			DIAGS(("taskmanager_form_exit: Moved the shit out of form_do() to here!"));
			if ( fr->md && ((aesobj_type(&fr->obj) & 0xff) == G_SLIST))
			{
				if (fr->md->clicks > 1)
					dclick_scroll_list(lock, wt->tree, aesobj_item(&fr->obj), fr->md);
				else
					click_scroll_list(lock, wt->tree, aesobj_item(&fr->obj), fr->md);
			}
			break;
		}
		/* Empty the alerts list */
		case SALERT_CLEAR:
		{
			struct scroll_info *list = object_get_slist(wt->tree + SYSALERT_LIST);
			struct sesetget_params p = { 0 };

			p.arg.txt = /*txt_alerts*/"Alerts";
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			if (p.e)
				list->empty(list, p.e, 0);
			object_deselect(wt->tree + item);
			redraw_toolbar(lock, wind, SYSALERT_LIST);
			redraw_toolbar(lock, wind, item);
			break;
		}
		case SALERT_OK:
		{
			object_deselect(wt->tree + item);
			redraw_toolbar(lock, wind, item);
			close_window(lock, wind);
// 			delayed_delete_window(lock, wind);
			break;
		}
	}
}

static int
systemalerts_destructor(enum locks lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_sysalrt = NULL;
// 	systemalerts_win = NULL;
	return true;
}

static void
refresh_systemalerts(OBJECT *form)
{
	OBJECT *sl = form + SYSALERT_LIST;
	SCROLL_INFO *list = object_get_slist(sl);

	list->slider(list, true);
}

/*
 * convert a file in /kern into a single line
 * delete tabs
 * replace \n->,
 *
 */
static void kerinfo2line( char *in, char *out, size_t maxlen )
{
	char *pi = in, *po = out;
	size_t i;

	for( i = 0; i < maxlen && *pi; i++, pi++ )
	{
		if( *pi > ' ' /*!= '\t'*/ )
		{
			if( *pi == '\n' )
				*po++ = ',';
			else
				*po++ = *pi;
		}
		else if( !(*(po-1) == ' ' || *(po-1) == ':') )
			*po++ = ' ';
	}
	*--po = 0;
}

/*
 *	stat/: kernget.c:

   info->len = ksprintf (info->buf, len,
    "%d (%s) %c %d %d %d %d %u "
    "%lu %lu %lu %lu %lu %lu %ld %ld %d %d "
     "%ld %ld %ld %ld %lu %lu %lu %lu %lu %lu "
     "%lu %lu %lu %lu %lu %lu %lu\n",

   p->pid,
   p->name,
   state,
   p->ppid,
   p->pgrp,
   get_session_id (p),
   ttypgrp,
   memflags,
   0UL, 0UL, 0UL, 0UL, * Page faults *
   p->systime, p->usrtime, p->chldstime, p->chldutime,
   (int) (p->slices * time_slice * 20),
   (int) -(p->pri),
   (long) timeout / 5,
   p->alarmtim->when / 5,
   (long) p->itimer->timeout->when / 5,
   (long) starttime.tv_sec * 200L + (long) starttime.tv_usec / 5000L,
   (ulong) memused (p),
   (ulong) memused (p),  * rss *
   0x7fffffffUL,   * rlim *
   base + 256UL,
   endcode,
   startcode,
   0UL, 0UL,
   (ulong) (p->sigpending >> 1),
   (ulong) (p->p_sigmask >> 1),
   (ulong) sigign,
   (ulong) sigcgt,
   (ulong) p->wait_q
   );
*/
/*
 * read a file in /kern and grab selected values controlled by pinfo
 * output goes to pinfo
 */
static int ker_stat( int pid, char *what, long pinfo[] )
{
	char path[256];
	struct file *fp;
	long err;
	if( pid )
		sprintf( path, sizeof(path), "u:/kern/%d/%s", pid, what );
	else
		sprintf( path, sizeof(path), "u:/kern/%s",what );
	fp = kernel_open( path, O_RDONLY, &err, NULL );
	if( !fp )
	{
		return 1;
	}

	err = kernel_read( fp, path, sizeof(path) );
	if( err > 0 )
	{
		int i, j;
		char *p = path;
		path[err] = 0;
		for( j = 0, i = 1; *p && pinfo[j]; i++ )
		{
			if( i == pinfo[j] )
			{
				if( !isdigit( *p ) )
					return 3;
				pinfo[j++] = atol( p );
			}
			for( ; *p && !(*p == ' ' || *p == '.'); p++ );
			if( *p )
				p++;
		}
		if( !*p && pinfo[j] )
			return 4;
	}
	kernel_close(fp);
	if( err <= 0 )
	{
		return 2;
	}
	return 0;
}


static struct scroll_entry *add_title_string( struct scroll_info *list, struct scroll_entry *this, char *str)
{
	struct scroll_content sc = {{ 0 }};
	struct sesetget_params p = { 0 };

	sc.t.text = str;
	sc.t.strings = 1;
	sc.xflags = OF_AUTO_OPEN|OF_OPENABLE;

	sc.fnt = &norm_txt;

	if( !list->add(list, this, 0, &sc, SEADD_CHILD, SETYP_STATIC, 0) )
		return 0;

	p.idx = -1;
	p.arg.txt = str;
	p.level.maxlevel = 2;
	list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);

	return p.e;
}

static void add_os_features(struct scroll_info *list, struct scroll_entry *this, struct scroll_content *sc)
{
	char s[128];
	long has_mprot = Ssystem(S_OSFEATURES,0,0);

#if CHECK_STACK
	extern unsigned short stack_align;
	sprintf( s, sizeof(s)-1, "MEMPROT:%s, Stack: %x", has_mprot ? "ON" : "OFF", stack_align );
	BLOG((0,s));
#else
	sprintf( s, sizeof(s)-1, "MEMPROT:%s", has_mprot ? "ON" : "OFF" );
#endif
	sc->t.text = s;

	sc->fnt = &norm_txt;

	list->add(list, this, 0, sc, SEADD_CHILD, 0, 0);
}


/*
 * grab words from a file up to startline into pinfo
 * pinfo[n]: # of word pinfo[n+1]: title pinfo[n+2]: # of words to add
 *
 * convert all from startline into a single line, appended to the above
 * add resulting line to list
 */
static void add_kerinfo(
	char *path,
	struct scroll_info *list,
	struct scroll_entry *this, struct scroll_entry *to,
	struct scroll_content *sc, int maxlen, int startline,
	int redraw, long *pinfo
)
{
	long err;
	struct file *fp;
	fp = kernel_open( path, O_RDONLY, &err, NULL );
	if(fp)
	{
		char line[256], sstr[1024];

		err = kernel_read( fp, sstr, sizeof(sstr)-1 );
		kernel_close(fp);
		if( err >= sizeof(sstr) )
			*(sstr + sizeof(sstr) - 1) = 0;

		if( err > 0 )
		{
			int i, j = 0, p=0, l;
			char *sp;
			for( sp = sstr, l = i = 0; l < startline && i < err; i++ )
			{
				if( sstr[i] == '\n' )
				{
					/* grab words from lines before startline */
					if( pinfo && pinfo[p] && pinfo[p] == l )
					{
						char *cp, *cpx;
						int k;
						for( cp = sp, k = 0; k < pinfo[p+2]; k++ )
						{
							for( ; *cp > ' '; cp++ );
							for( ; *cp && *cp <= ' '; cp++ );
						}
						for( cpx = cp; *cpx > ' '; cpx++ );
						*cpx = 0;
						if( cp && *cp )
							j += sprintf( line+j, sizeof(line)-j-1, "%s%s ", pinfo[p+1], cp );
						p += 3;
						*cpx = ' ';
					}
					l++;
					sp = sstr+i+1;
				}
			}
			if( l < startline )
			{
				BLOG((0,"add_kerinfo: invalid entry: %s.", sstr ));
				return;	/* error */
			}
			err -= i;
			if( maxlen && err > maxlen )
				err = maxlen;
			if( err >= sizeof(line) -1 )
				err = sizeof(line)-1;
			sstr[err+i] = 0;

			kerinfo2line( sstr+i, line+j, sizeof(line)-j-1 );

			if( to )
			{
				struct setcontent_text t = { 0 };
				struct se_content *sec = to->content;
				t.text = line;
				/* set only if changed */
				if( !sec || strcmp( t.text, sec->c.text.text ) )
				{
					list->set(list, to, SESET_TEXT, (long)&t, redraw);
				}
			}
			else
			{
				sc->t.text = line;
				sc->fnt = &norm_txt;
				list->add(list, this, 0, sc, this ? (SEADD_CHILD) : SEADD_PRIOR, 0, redraw);
			}
		}
	}
	else
		BLOG((0,"add_kerinfo:could not open %s err=%ld", path, err ));
}


void
open_systemalerts(enum locks lock, struct xa_client *client, bool open)
{
	struct helpthread_data *htd;
	OBJECT *obtree = NULL;
	struct widget_tree *wt = NULL;
	struct xa_window *wind;
	RECT remember = { 0, 0, 0, 0 };

	if (!(htd = get_helpthread_data(client)))
		return;

	if (!htd->w_sysalrt)
	{
		struct scroll_info *list;
		RECT or;
		char a[] = /*txt_alerts*/"Alerts";
		char e[] = /*txt_environment*/"Environment";
		char s[] = /*txt_environment*/"System";
		int objs[] = {SALERT_IC1, SALERT_IC2, 0};

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, SYS_ERROR), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;

		list = set_slist_object(0, wt, NULL, SYSALERT_LIST,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_TREEVIEW|SIF_AUTOOPEN|SIF_AUTOSLIDERS,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, 255);

		if (!list) goto fail;

		set_xa_fnt( cfg.xaw_point, 0, obtree, objs, list);

		/* todo: set focus into list (?) */
		obj_init_focus(wt, OB_IF_RESET);

		{
// 			struct scroll_info *list = object_get_slist(obtree + SYSALERT_LIST);
			struct scroll_content sc = {{ 0 }};

			sc.t.text = a;
			sc.t.strings = 1;
			sc.icon = obtree + SALERT_IC1;
			sc.usr_flags = 1;
			sc.xflags = OF_AUTO_OPEN|OF_OPENABLE;
			DIAGS(("Add Alerts entry..."));
			sc.fnt = &norm_txt;
			list->add(list, NULL, NULL, &sc, 0, SETYP_STATIC, NOREDRAW);
			sc.t.text = e;
			sc.icon = obtree + SALERT_IC2;
			list->set(list, list->cur, SESET_SELECTED, 0, NORMREDRAW);

			DIAGS(("Add Environment entry..."));
			list->add(list, NULL, NULL, &sc, 0, SETYP_STATIC, NOREDRAW);
			sc.t.text = s;
			sc.icon = obtree + SALERT_IC2;
			DIAGS(("Add System entry..."));
			sc.fnt = &norm_txt;
			list->add(list, NULL, NULL, &sc, 0, SETYP_STATIC, NOREDRAW);
		}
		{
// 			struct scroll_info *list = object_get_slist(obtree + SYSALERT_LIST);
			struct scroll_entry *this;
			const char **strings = get_raw_env(); //char * const * const strings = get_raw_env();
			int i;
			struct sesetget_params p = { 0 };
			struct scroll_content sc = {{ 0 }};
			char sstr[1024];

			p.idx = -1;
			p.arg.txt = e;	// /*txt_environment*/"Environment";
			p.level.flags = 0;
			p.level.curlevel = 0;
			p.level.maxlevel = 0;
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			list->empty(list, p.e, 0);
			this = p.e;
			sc.t.strings = 1;
			sc.fnt = &norm_txt;

			/* todo?: define sort (name/value) */
			sc.t.text = sstr;
			for (i = 0; strings[i]; i++)
			{
				strncpy( sstr, strings[i], sizeof(sstr)-1);
				list->add(list, this, sortbyname, &sc, this ? (SEADD_CHILD|SEADD_PRIOR) : SEADD_PRIOR, 0, true);
			}
			{
				char *pformats[] = {"MOT", "INT", "FMOT", "INT15", "Atari", "Unknown"}, *fs;

				p.arg.txt = s;
				list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
				list->empty(list, p.e, 0);
				this = p.e;
				sc.t.strings = 1;

				/* video */

				if( screen.pixel_fmt <= 3 )
					fs = pformats[screen.pixel_fmt];
				else if( screen.pixel_fmt == 8 )
					fs = pformats[4];
				else
					fs = pformats[5];

				sprintf( sstr, sizeof(sstr)-1, "Video:%dx%dx%d,%d colours, format: %s",
					screen.r.w, screen.r.h, screen.planes, screen.colours, fs );
			}

			sc.t.text = sstr;
			list->add(list, this, 0, &sc, this ? (SEADD_CHILD) : SEADD_PRIOR, 0, true);

			/* cpuinfo */
			add_kerinfo( "u:/kern/cpuinfo", list, this, NULL, &sc, 0, 0, false, NULL );
			BLOG((0,"cpuinfo:%s", sc.t.text ));
			add_os_features(list, this, &sc);


			this = add_title_string(list, this, "Kernel");

			add_kerinfo( "u:/kern/version", list, this, NULL, &sc, 0, 0, false, NULL );
			BLOG((0,"version:%s", sc.t.text ));
#if XAAES_RELEASE > 0
			add_kerinfo( "u:/kern/buildinfo", list, this, NULL, &sc, 0, 1, false, NULL );
			BLOG((0,"buildinfo:%s", sc.t.text ));
#endif
			init_list_focus( obtree, SYSALERT_LIST, 0 );
		}

		obj_rectangle(wt, aesobj(obtree, 0), &or);

		obtree[SALERT_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!remember.w)
		{
			center_rect(&or);
			remember = calc_window(lock, client, WC_BORDER,
						BORDER|CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
						created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						*(RECT *)&or);
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag,
					do_formwind_msg,
					client,
					false,
					BORDER|CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
					created_for_AES,
					C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						remember,
					NULL, NULL);
		if (!wind) goto fail;

		wind->min.h = remember.h * 2/3;	/* minimum height for this window */
		wind->min.w = remember.w;	/* minimum width for this window */
		list->set(list, NULL, SESET_PRNTWIND, (long)wind, NOREDRAW);

		/* Set the window title */
		set_window_title(wind, " System window & Alerts log", false);

		wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		wt->exit_form = sysalerts_form_exit;
		wt->focus = aesobj( obtree, SYSALERT_LIST );	/*cursor on list */
		/* Set the window destructor */
		wind->destructor = systemalerts_destructor;

		refresh_systemalerts(obtree);

		if (open)
			open_window(lock, wind, wind->r);

		htd->w_sysalrt = wind; //systemalerts_win = dialog_window;
	}
	else
	{
		wind = htd->w_sysalrt;
		if (open)
		{
			open_window(lock, wind, wind->r);
			force_window_top( lock, wind );
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

/*
 * Handle clicks on the system default menu
 */
void
do_system_menu(enum locks lock, int clicked_title, int menu_item)
{
	switch (menu_item)
	{
		default:
			DIAGS(("Unhandled system menu item %i", menu_item));
			break;

		/* Open the "About XaAES..." dialog */
		case SYS_MN_ABOUT:
			post_cevent(C.Hlp, ceExecfunc, open_about,NULL, 1,0, NULL,NULL);
			break;

		/* Quit all applications */
		case SYS_MN_QUITAPP:
			DIAGS(("Quit all Apps"));
			quit_all_apps(lock, (struct xa_client*)-1, AP_TERM);
			break;

		/* Quit XaAES */
		case SYS_MN_QUIT:
			DIAGS(("Quit XaAES"));
			dispatch_shutdown(0, 0);
			break;

		/* Open the "Task Manager" window */
		case SYS_MN_TASKMNG:
			post_cevent(C.Hlp, ceExecfunc, open_taskmanager,NULL, 1,0, NULL,NULL);
			break;
		/* Open system alerts log window */
		case SYS_MN_SALERT:
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts,NULL, 1,0, NULL,NULL);
			break;
		/* Open system alerts log window filled with environment */
		case SYS_MN_ENV:
		{
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts, NULL, 1,0, NULL,NULL);
#if 0
			OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
			struct scroll_info *list = object_get_slist(form + SYSALERT_LIST);
			struct scroll_entry *this;
			const char **strings = get_raw_env(); //char * const * const strings = get_raw_env();
			int i;
			struct sesetget_params p = { 0 };
			struct scroll_content sc = {{ 0 }};

			p.idx = -1;
			p.arg.txt = "Environment";
			p.level.flags = 0;
			p.level.curlevel = 0;
			p.level.maxlevel = 0;
			list->get(list, NULL, SEGET_ENTRYBYTEXT, &p);
			list->empty(list, p.e, 0);
			this = p.e;
			sc.t.strings = 1;
			for (i = 0; strings[i]; i++)
			{	sc.t.text = strings[i];
				list->add(list, this, NULL, &sc, this ? (SEADD_CHILD|SEADD_PRIOR) : SEADD_PRIOR, 0, true);
			}
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts,NULL, 1,0, NULL,NULL);
#endif
			break;
		}

#if FILESELECTOR
		/* Launch a new app */
		case SYS_MN_LAUNCH:
		{
			post_cevent(C.Hlp, ceExecfunc, open_launcher,NULL, 1,0, NULL,NULL);
			break;
		}
#endif
		/* Launch desktop. */
		case SYS_MN_DESK:
		{
			if (C.DSKpid >= 0)
				ALERT((/*shell_running*/"XaAES: AES shell already running!"));
			else if (!*C.desk)
				ALERT((/*no_shell*/"XaAES: No AES shell set; See 'shell =' configuration variable in xaaes.cnf"));
			else
				C.DSKpid = launch(lock, 0,0,0, C.desk, "\0", C.Aes);
			break;
		}
		case SYS_MN_RESCHG:
		{
			if (C.reschange)
				post_cevent(C.Hlp, ceExecfunc, C.reschange,NULL, 1,0, NULL,NULL);
		}
	}
}
