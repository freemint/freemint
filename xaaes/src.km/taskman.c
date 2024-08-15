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

/*
 * Task Manager Dialog
 * Error message dialog
 * System menu handling
 */

#define NEWLOAD 1
#ifndef USE_Suptime
#define USE_Suptime 1
#endif

#if USE_Suptime
#include <mintbind.h>	/* Suptime */
#endif


#include "xa_types.h"
#include "xa_global.h"
#include "keycodes.h"

#include "xaaes.h"

#include "about.h"
#include "app_man.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "form.h"
#include "k_main.h"
#include "k_mouse.h"
#include "draw_obj.h"
#include "render_obj.h"
#include "menuwidg.h"
#include "obtree.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xa_appl.h"
#if WITH_BBL_HELP
#include "xa_bubble.h"
#endif
#include "xa_form.h"
#include "xa_graf.h"
#include "xa_menu.h"
#include "xa_shel.h"
#include "xa_rsrc.h"
#include "xa_fsel.h"
#include "trnfm.h"
#include "util.h"

#include "mint/signal.h"
#include "mint/stat.h"
#include "mint/fcntl.h"
#include "mint/ssystem.h"

#define ADDPROCINFO	0

char XAAESNAME[] = "XaAES";

static void add_kerinfo( char *path, struct scroll_info *list, struct scroll_entry *this, struct scroll_entry *to, struct scroll_content *sc, int maxlen, int startline, int redraw, long *pinfo, bool child, char *out, size_t outl );
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
#define TM_MEMINFO_DATA	(void*)-1;	/* unique "data" for memory-info */
#define TM_UPDATED	0x04L	/* was updated */
#define TM_NOAES		0x08L	/* not an aes-client */
#define TM_PROCINFO	0x10L	/* process-info (if def'd) */
#define TM_HEADER		0x20L	/* header */
#define TM_CLIENT		0x40L	/* aes-client-entry in taskman-list */

/*static*/ struct xa_wtxt_inf norm_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wr_mode   efx   fg       bg        banner x_3dact y_3dact texture */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL},	/* Normal */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_WHITE, G_BLACK, G_WHITE,   0,      0,     NULL},	/* Selected */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL},	/* Highlighted */

};

/*static*/ struct xa_wtxt_inf acc_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wr_mode   efx   fg       bg   banner x_3dact y_3dact texture */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_BLUE,  G_LBLUE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_WHITE, G_LBLUE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

/*static*/ struct xa_wtxt_inf prg_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wr_mode   efx   fg       bg    banner x_3dact y_3dact texture */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_RED,   G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_WHITE, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_BLACK, G_WHITE, G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

/*static*/ struct xa_wtxt_inf sys_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wr_mode   efx   fg        bg        banner x_3dact y_3dact texture */
 {  -1,  -1,  0,   MD_TRANS, 0,    G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1,  0,   MD_TRANS, 0,    G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1,  0,   MD_TRANS, 0,    G_BLACK,  G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

/*static*/ struct xa_wtxt_inf sys_thrd =
{
 WTXT_NOCLIP,
/* id  pnts  flags wr_mode   efx   fg        bg        banner x_3dact y_3dact texture */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_LWHITE, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_LBLACK, G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_BLACK,  G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

};

/*static*/ struct xa_wtxt_inf desk_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags wr_mode   efx   fg        bg    banner x_3dact y_3dact texture */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_LRED,   G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Normal */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_MAGENTA,G_LWHITE, G_WHITE,   0,      0,     NULL  },	/* Selected */
 {  -1,  -1, 0,    MD_TRANS, 0,    G_BLACK,  G_WHITE,  G_WHITE,   0,      0,     NULL  },	/* Highlighted */

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
 * set font-point to pt - if not available select next bigger/smaller to current
 *
 * return new point
 */
short set_xa_fnt( int pt, struct xa_wtxt_inf *wp[], OBJECT *obtree, int objs[], SCROLL_INFO *list, short *neww, short *newh )
{
	short i, w, h, x, vdih;
	short oldh, oldpt, oldw;
	struct xa_wtxt_inf *wpp;

	if( wp )
	{
		wpp = wp[0];
	}
	else
		wpp = &norm_txt;


	if( neww )
		*neww = 0;
	if( newh )
		*newh = 0;

	if( wpp->normal.font_point < 0 )
		wpp->normal.font_point = pt;

	vdih = C.Aes->vdi_settings->handle;
	vst_point(vdih, wpp->normal.font_point, &w, &h, &oldw, &oldh);

	oldpt = wpp->normal.font_point;

	if( wpp->normal.font_point != pt )
	{
		if( pt > wpp->normal.font_point )
			i = 1;
		else
			i = -1;

		wpp->normal.font_point += i;
		for( vst_point(vdih, wpp->normal.font_point, &x, &x, &w, &h);
				wpp->normal.font_point < 65 && wpp->normal.font_point > 0;)
		{
			if( oldh != h || w != oldw )
				break;
			wpp->normal.font_point += i;
			vst_point(vdih, wpp->normal.font_point, &x, &x, &w, &h);
		}

		if( h == oldh && w == oldw )
			return wpp->normal.font_point = oldpt;	/* no change */

		pt = wpp->normal.font_point;
	}
	if( wp )
		for( i = 0; wp[i]; i++ )
		{
			wp[i]->normal.font_point = wp[i]->selected.font_point = wp[i]->highlighted.font_point = pt;
		}

	vst_point(vdih, wpp->normal.font_point, &x, &x, &w, &h);

	/* todo: need smaller/greater icons */
	if( objs && obtree )
		for( i = 0; objs[i]; i++ )
		{
			object_get_spec(obtree + objs[i])->iconblk->ib_hicon = (obtree + objs[i])->ob_height = h;
		}

	if( list )
	{
		list->nesticn_h = h;
		list->char_width = w;
	}

	if( neww )
	{
		*neww = (1000L * (long)w) / (long)oldw;
	}
	if( newh )
	{
		*newh = h;
	}
	return pt;
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
build_tasklist_string( int md, void *app, long pid)
{
#define TX_LEN	256
	char *tx;

	tx = kmalloc(TX_LEN);

	if (tx)
	{
		struct proc *p;
		/*unsigned*/ char *name, c=0, *cp;
		long pinfo[4], utim, ptim, l;
		static char *state_str[] = {"Cur ", "Run ", "Wait", "IO  ", "Zomb", "TSR ", "STOP", "Slct"};

		if( md == AES_CLIENT )
		{
			if( app )
			{
				p = ((struct xa_client *)app)->p;
				name = ((struct xa_client *)app)->name;
				c = name[15];
				name[15] = 0;
			}
			else
			{
				kfree(tx);
				return 0;
			}
		}
		else
		{
			p = pid2proc(pid);
			if( !p )
			{
				BLOG((0,"*build_tasklist_string %ld: not found! (p=%lx)", pid, (unsigned long)p));
				return NULL;
			}
			name = p->name;
		}

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

		l = strlen(p->name);
		cp = p->cmdlin;
		if( !strncmp( cp, p->name, l ) )
			cp += l;
		else
			cp++;	/* length */

		sprintf(tx, TX_LEN-1, "%16s %4d %4d %4d %3d   %c   %s %8ld %11ld %2ld %s",
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
	if( C.shutdown )
		return;
	if (wi && (wi->window_status & XAWS_OPEN)
		&& !( wi->dial
			& ( created_for_SLIST
			| created_for_CALC
			| created_for_POPUP
			| created_for_ALERT
			))
	)
	{
		struct helpthread_data *htd = 0;
		struct scroll_entry *this;
		struct xa_window *wind;
		if( C.Hlp )
		{
			htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
		}

		if (!htd)
			return;
		wind = htd->w_taskman;

		if (wind && wind != wi)
		{
			struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
			OBJECT *obtree = wt->tree;
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
					list->cur = 0;
					if( (this->xstate & OS_OPENED) )
					{
						if( thiswi->prev )
							list->cur = thiswi->prev;
						else
							list->cur = thiswi->next;
					}
					if( list->cur == 0 )
						list->cur = this;
					list->del(list, thiswi, true);
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

						sc.t.text = wi->wname;
						if( !*sc.t.text )
							sc.t.text = (long)title & 1L ? 0 : title;
						if( !sc.t.text || !*sc.t.text )
							return;
						sc.t.strings = 1;
						sc.data = wi;
						sc.usr_flags = TM_WINDOW;
						sc.fnt = &norm_txt;
						list->add(list, this, NULL, &sc, SEADD_CHILD, 0, true);

						if( !(this->xstate & OS_OPENED) )
							list->cur = this;
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
	add_kerinfo( path, list, this, to, sc, PROCINFLEN, 0, NORMREDRAW, NULL, NULL,0 );
}
#endif
void
add_to_tasklist(struct xa_client *client)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	struct xa_window *wind;

	if (!htd || !client)
		return;

	while (!htd->w_taskman)
	{
		yield();
	}
	wind = htd->w_taskman;

	if (wind)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
		OBJECT *obtree = wt->tree;
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		OBJECT *icon;
		char *tx, *cp;
		struct scroll_content sc = {{ 0 }};

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

		tx = build_tasklist_string(AES_CLIENT, client, 0);
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
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
		OBJECT *obtree = wt->tree;
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		struct sesetget_params p = { 0 };

		if (list)
		{
			p.arg.data = client;
			list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
			if (p.e)
			{
#if 0	/* debugging-info: dont delete */
				struct scroll_entry *this = p.e;
				BLOG((0,"remove_from_tasklist:'%s': this=%lx,client=%lx,p=%lx", get_curproc()->name, this, client, client ? client->p : -1 ));
				BLOGif(this,(0,"remove_from_tasklist: flags=%lx,content=%lx", this->usr_flags, this->content ));
				if( this->usr_flags & TM_NOAES )
				{
					struct proc *pr = (struct proc *)client;
					BLOG((0,"remove_from_tasklist: delete: NOAES:%lx:%s:%lx", pr, this->content->c.text.text, p.e ));
				}
				else if( this->usr_flags & TM_CLIENT )
					BLOG((0,"remove_from_tasklist: delete CLIENT:%d:%s(%s):%lx", client->p->pid, client->name, client->p->name, p.e ));
#endif
				if( p.e->prev )
					list->cur = p.e->prev;
				else
					list->cur = p.e->next;
				list->del(list, p.e, true);
				if( list->cur )
					list->set(list, list->cur, SESET_SELECTED, 0, NORMREDRAW);
			}
		}
	}
}

/*
 * md = 1: aes-client
 * md = 0: no aes-client
 */
void
update_tasklist_entry( int md, void *app, struct helpthread_data *htd, long pid, int redraw )
{
	struct xa_window *wind;

	if( !htd )
		htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);
	if (!htd || (md == AES_CLIENT && !app))
		return;
	wind = htd->w_taskman;

	if (wind)
	{
		struct widget_tree *wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
		OBJECT *obtree = wt->tree;
		SCROLL_INFO *list = object_get_slist(obtree + TM_LIST);
		struct sesetget_params p = { 0 };
		struct scroll_content sc = {{ 0 }};

		if (list)
		{
			char *tx = 0;
			struct setcontent_text t = { 0 };

			p.arg.data = (md == AES_CLIENT ? app : (void*)pid);
			list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
			if (p.e)
			{
				struct scroll_entry *this = p.e;
				struct se_content *sec = this->content;

				if( md == AES_CLIENT )
				{
					if( *((uchar*)((struct xa_client *)app)->name+2) <= ' ' )
					{
						remove_from_tasklist( app );
						return;
					}
				}

				if ((tx = build_tasklist_string(md, app, pid)))
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
				if( app == S.app_list.first && TOP_WINDOW != htd->w_taskman )
				{
					if( TOP_WINDOW->owner == app && this->down && (this->xstate & OS_OPENED) )
					{
						/* set cursor on window-entry */
						p.arg.data = TOP_WINDOW;
						p.level.flags = 0;
						p.level.curlevel = 0;
						p.level.maxlevel = 3;
						list->get(list, NULL, SEGET_ENTRYBYDATA, &p);
						if( p.e )
							this = p.e;
					}
					list->cur = this;
					list->set(list, list->cur, SESET_SELECTED, 0, NORMREDRAW);
				}

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
				sc.t.strings = 1;
				sc.data = (void*)pid;
				sc.xflags = 0;
				sc.usr_flags = TM_UPDATED | TM_NOAES;
				if ((tx = build_tasklist_string(md, 0, pid)))
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

bool
isin_namelist(struct cfg_name_list *list, char *name, short nlen, struct cfg_name_list **last, struct cfg_name_list **prev)
{
	bool ret = false;

	if (!nlen)
		nlen = strlen(name);

	DIAGS(("isin_namelist: find '%s'(len=%d) in list=%lx (name='%s', len=%d)",
		name, nlen, (unsigned long)list,
		list ? list->name : "noname",
		list ? list->nlen : -1));

	if (last)
		*last = NULL;
	if (prev)
		*prev = NULL;

	while (list)
	{
		DIAGS((" -- checking list=%lx, name=(%d)'%s'",
			(unsigned long)list, list->nlen, list->name));

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
		ret ? "true":"false", (unsigned long)last, (unsigned long)prev));

	return ret;
}

void
addto_namelist(struct cfg_name_list **list, char *name)
{
	struct cfg_name_list *new, *prev;
	short nlen = strlen(name);

	DIAGS(("addto_namelist: add '%s' to list=%lx(%lx)", name, (unsigned long)*list, (unsigned long)list));

	if (nlen && !isin_namelist(*list, name, 0, NULL, &prev))
	{
		new = kmalloc(sizeof(*new));

		if (new)
		{
			if (nlen > sizeof(new->name) -1)
				nlen = sizeof(new->name) -1;

			bzero(new, sizeof(*new));

			if (prev)
			{
				DIAGS((" -- add new=%lx to prev=%lx", (unsigned long)new, (unsigned long)prev));
				prev->next = new;
			}
			else
			{
				DIAGS((" -- add first=%lx to start=%lx", (unsigned long)new, (unsigned long)list));
				*list = new;
			}
			strncpy(new->name, name, nlen);
			new->name[nlen] = 0;
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
			(unsigned long)l, (unsigned long)*list, (unsigned long)list, l->name));
		kfree(l);
	}
}

static int
taskmanager_destructor(int lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);

	if (htd)
		htd->w_taskman = NULL;
	return true;
}

void
send_terminate(int lock, struct xa_client *client, short reason)
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


void
quit_all_apps(int lock, struct xa_client *except, short reason)
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
	Sema_Up(LOCK_CLIENTS);
	lock |= LOCK_CLIENTS;

	FOREACH_CLIENT(client)
	{
		if (is_client(client) && client != C.Hlp && client != except)
		{
			DIAGS(("shutting down %s", c_owner(client)));
			send_terminate(lock, client, reason);
		}
	}

	Sema_Dn(LOCK_CLIENTS);
}

#if ALT_CTRL_APP_OPS && 1	/* HOTKEYQUIT */
static void
quit_all_clients(int lock, struct cfg_name_list *except_nl, struct xa_client *except_cl, short reason)
{
	struct xa_client *client, *dsk = NULL;

	Sema_Up(LOCK_CLIENTS);
	lock |= LOCK_CLIENTS;

	DIAGS(("quit_all_clients: name_list=%lx, except_client=%lx", (unsigned long)except_nl, (unsigned long)except_cl));
	/*
	 * '_aes_shell' is special. If it is defined, we lookup the pid of
	 * the shell (desktop) loaded by the AES and let it continue to run
	 */
	if (isin_namelist(except_nl, "_aes_shell_", 11, NULL, NULL))
	{
		dsk = pid2client(C.DSKpid);
		DIAGS((" -- _aes_shell_ defined: pid=%d, client=%lx, name=%s",
			C.DSKpid, (unsigned long)dsk, dsk ? dsk->name : "no shell loaded"));
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
	Sema_Dn(LOCK_CLIENTS);
}
void
ce_quit_all_clients(int lock, struct xa_client *client, bool b)
{
	struct cfg_name_list *nl = NULL;


	if ( xaaes_do_form_alert( lock, C.Hlp, 1, xa_strings(ASK_QUITALL_ALERT)) != 2 )
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
				if (m->m[3] == 1)
				{
					C.update_lock = NULL;
					C.updatelock_count--;
					unlock_screen(client->p);
				}
				else if (m->m[3] != 0)
				{
					sprintf(alert, sizeof(alert), /*scrn_snap_serr*/xa_strings(SNAP_ERR1), m->m[3]);
					do_form_alert(0, client, 1, alert, XAAESNAME);
				}
				break;
			}
			default:;
		}
	}
}

#if ALT_CTRL_APP_OPS
void
screen_dump(int lock, struct xa_client *client, short open)
{
	struct xa_client *dest_client;
	UNUSED(open);
	if ((dest_client = get_app_by_procname("xaaesnap")) || cfg.snapper[0] )
	{
		if (update_locked() != client->p && lock_screen(client->p, true))
		{
			short msg[8] = {0x5354, client->p->pid, 0, 0, 0,0,200,200};
			GRECT r, d = {0};
			short b = 0, x, y;
			bool doit = true, snapmsg = false;
			AESPB *a = C.Hlp_pb;
			int pid = -1;

			hidem();
			C.update_lock = client->p;
			C.updatelock_count++;

			do_form_alert(lock, client, 4, xa_strings(SDALERT), XAAESNAME);
			Block(client, 0);
#if WITH_BBL_HELP
			xa_bubble( 0, bbl_tmp_inact, 0, 0 );
#endif

			if (a->intout[0] == 1)
			{
				xa_graf_mouse(THIN_CROSS, NULL,NULL, false);
				while (!b)
					check_mouse(client, &b, &x, &y);

				r.g_x = x;
				r.g_y = y;
				r.g_w = 0;
				r.g_h = 0;
				rubber_box(client, SE, r, &d, 0,0, root_window->r.g_h, root_window->r.g_w, &r);
			}
			else if (a->intout[0] == 2)	/* full */
				r = root_window->r;
			else if (a->intout[0] == 3)	/* top */
			{
				struct xa_window *wind = TOP_WINDOW;

				if ( !dest_client ||
						(wind->r.g_x > 0 && wind->r.g_y > 0 &&
				   (wind->r.g_x + wind->r.g_w) < root_window->r.g_w &&
				   (wind->r.g_y + wind->r.g_h) < root_window->r.g_h) )
					r = wind->r;
				else
				{
					C.updatelock_count--;
					C.update_lock = NULL;
					unlock_screen(client->p);
					do_form_alert(lock, client, 1, xa_strings(SNAP_ERR2), XAAESNAME);
					doit = false;
				}
			}
			else
				doit = false;

			if (doit)
			{
				if (r.g_w > 0 && r.g_h > 0)
				{
					if( dest_client )
					{
						msg[4] = r.g_x;
						msg[5] = r.g_y;
						msg[6] = r.g_w;
						msg[7] = r.g_h;
						send_a_message(lock, dest_client, AMQ_NORM, 0, (union msg_buf *)msg);
						snapmsg = true;
					}
					else
					{
						struct proc *p;
						long sleep_tm = 1;
						char cmdlin[32] = " 1 0 0\0";

						/* <wait> <key> <verbose>
						 * key: 0 -> screen
						 	      2 -> top-window: work-area
						 	      6 -> top-window: whole-area
						 */

						if (a->intout[0] == 3)	/* top window */
							*(cmdlin + 3) = '2';
						else if (a->intout[0] == 1)	/* box */
						{
							int i = sprintf( cmdlin + 6, sizeof(cmdlin)-1, " %d %d %d %d", r.g_x, r.g_y, r.g_w, r.g_h );
							*(cmdlin + 3) = '8';
							*cmdlin = i + 6;
						}
						else
							*(cmdlin + 3) = '0';	/* full screen */

						/* todo: store blit-screenshot to file, snapper converts this */
						pid = create_process(cfg.snapper, cmdlin, NULL, &p, 0, NULL);
						if( pid == 0 && p )
						{
/* todo: if snapper hangs system could hang too */
#if KILL_SNAPPER
							short term = 0;
							long pr = -1;
							short br, xr, yr, xm, ym;
#endif
							pid = p->pid;
							for( sleep_tm *= 1000; sleep_tm; sleep_tm-- )
								nap(1000);
							/* todo: timeout! */
#if KILL_SNAPPER
							check_mouse(client, &br, &xm, &ym);
							xm = xr;
							ym = yr;
							for( term = 0; term < 10000 && !(pr = ikill( pid, 0)); term++ )
							{
								check_mouse(client, &br, &xr, &yr);
								if( term > 3 && xr && (xr != xm || yr != ym) )
									break;
								nap( 1000 );
								xm = xr;
								ym = yr;
							}
							if( pr == 0 )	/* snapper still there */
							{
								char s[128];
								pr = ikill( pid, SIGTERM );
								yield();
								pr = ikill( pid, SIGKILL );
								sprintf( s, sizeof(s)-1, "[1][%s: killed][OK]", cfg.snapper, pid );
								do_form_alert(lock, client, 1, s, XAAESNAME );
							}
#endif
						}
					}
				}
				else
				{
					char s[128];
					sprintf( s, sizeof(s)-1, "[1][could not snap area:w=%d,h=%d][OK]", r.g_w, r.g_h);
					do_form_alert(lock, client, 1, s, XAAESNAME );
				}
			}
			if( snapmsg == false )	/* external process */
			{
				nap(25);
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
#if WITH_BBL_HELP
			xa_bubble( 0, bbl_tmp_inact, 0, 0 );
#endif
			showm();
		}
	}
	else
		do_form_alert(lock, client, 1, /*scrn_snap_notfound*/ xa_strings(SNAP_ERR3), XAAESNAME);
}
#endif

void force_window_top( int lock, struct xa_window *wind )
{
	if( S.focus == wind )
		return;
	if( S.focus )
	{
		struct xa_window *w = S.focus;
		setwin_untopped( lock, w, true );
		w->colours = w->untop_cols;
		send_iredraw(lock, w, 0, NULL);
		S.focus = 0;	/* force focus to new top */
	}
	TOP_WINDOW = 0;
	screen.standard_font_point = wind->owner->options.standard_font_point;
	if (is_hidden(wind))
		unhide_window(lock|LOCK_WINLIST, wind, false);
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

void app_or_acc_in_front( int lock, struct xa_client *client )
{
	/* stolen from k_keybd.c#390: made a function for this */
	if ( client )
	{
		if( C.SingleTaskPid > 0 && client->p->pid != C.SingleTaskPid )
			return;
		app_in_front(lock, client, true, true, true);

		if (client->type & APP_ACCESSORY)
		{
			send_app_message(lock, NULL, client, AMQ_NORM, QMF_CHKDUP,
					 AC_OPEN, 0, 0, 0,
					 client->p->pid, 0, 0, 0);
		}
	}
}

static void kill_client( SCROLL_INFO *list )
{
	struct xa_client *client;

	if( (list->cur->usr_flags & TM_WINDOW) )
		return;

	if( list->cur->usr_flags & TM_NOAES )
	{
		long i, k;
		int pid;

		pid = (int)(long)list->cur->data;
		if( pid <= TM_MINPID || pid >= TM_MAXPID || pid == C.Aes->tp->pid )
			return;

		k = ikill(pid, SIGKILL);
		for( k = i = 0; i < TM_KILLLOOPS && !k; i++ )
		{
			nap( TM_KILLWAIT );
			k = ikill(pid, 0);
		}
		if( k )
		{
			remove_from_tasklist( list->cur->data );
		} else
		{
			BLOG((0,"taskmanager_form_exit: could not kill %d", pid));
		}
		return;
	}

	client = list->cur->data;
	DIAGS(("taskmanager: KILL for %s", c_owner(client)));
	if (client && is_client(client))
	{
		if (client->type & (APP_AESTHREAD|APP_AESSYS))
		{
			return;
		}
		else
		{
			ikill(client->p->pid, SIGKILL);
		}
	}
}

static void stop_cont_client( int lock, SCROLL_INFO *list, int sig )
{
	struct xa_client *client = list->cur->data;
	if( list->cur->usr_flags & TM_NOAES )
	{
		int pid = (int)(long)client;
		if( pid <= TM_MINPID || pid >= TM_MAXPID || pid == C.Aes->tp->pid )
			return;
		ikill(pid, sig);
		update_tasklist_entry( NO_AES_CLIENT, 0, 0, pid, true);
	}
	else if (client && is_client(client))
	{
		DIAGS(("taskmanager: TM_SLEEP for %s", c_owner(client)));
		if (client->type & (APP_AESTHREAD|APP_AESSYS|APP_ACCESSORY))
		{
			return;
		}
		app_in_front(lock, C.Aes, true, true, true);
		ikill(client->p->pid, sig);
		update_tasklist_entry( AES_CLIENT, client, 0, 0, true);
	}
}

static void term_client( int lock, SCROLL_INFO *list )
{
	struct xa_client *client = list->cur->data;
	if( list->cur->usr_flags & TM_NOAES )
	{
		int pid = (int)(long)client;
		if( pid <= TM_MINPID || pid >= TM_MAXPID || pid == C.Aes->tp->pid )
			return;
		ikill(pid, SIGTERM);
		/* todo: update list */
	}
	else{
		if (client && is_client(client))
		{
			DIAGS(("taskmanager: TM_TERM for %s", c_owner(client)));
			if (client->type & (APP_AESTHREAD|APP_AESSYS))
			{
				return;
			}
			else
			{
				send_terminate(lock, client, AP_TERM);
			}
		}
	}
}

/*
 * todo: if fileselector is open during shutdown there's a problem
 */
static void
taskmanager_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	int lock = 0;
	short item = aesobj_item(&fr->obj);
	struct xa_client *client = NULL;
	OBJECT *ob;
	SCROLL_INFO *list;

	Sema_Up(LOCK_CLIENTS);
	lock |= LOCK_CLIENTS;


	wt->which = 0;

	ob = wt->tree + TM_LIST;
	list = object_get_slist(ob);

	if( wind && item != TM_LIST )
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
				if( !client )
				{
					BLOG((0,"taskmanager_form_exit: client = 0!" ));
					return;
				}
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
			kill_client(list);
			break;
		}
		case TM_TERM:
		{
			term_client(lock, list);
			break;
		}
		case TM_SLEEP:
		{
			stop_cont_client( lock, list, SIGSTOP );

		break;
		}
		case TM_WAKE:
		{
			stop_cont_client( lock, list, SIGCONT );
#if 0
			if( list->cur->usr_flags & TM_NOAES )
			{
				int pid = (int)(long)client;
				if( pid <= TM_MINPID || pid >= TM_MAXPID )
					break;
				ikill(pid, SIGCONT);
				update_tasklist_entry( NO_AES_CLIENT, 0, 0, pid, true);
			}
			else if (client && is_client(client))
			{
				if (client->type & (APP_AESTHREAD|APP_AESSYS))
				{
					break;
				}
				ikill(client->p->pid, SIGCONT);
				update_tasklist_entry( AES_CLIENT, client, 0, 0, true);
			}
#endif
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
			dispatch_shutdown(0);
#endif
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
			dispatch_shutdown(REBOOT_SYSTEM);
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
			Sema_Dn(LOCK_CLIENTS);
#if 0
			close_window(lock, wind);
			if ( xaaes_do_form_alert( 0, C.Hlp, 1, xa_strings(ASK_SHUTDOWN_ALERT)) != 2 )
				goto lb_TM_OK;
#endif
			DIAGS(("taskmanager: halt system"));

#if TM_ASK_BEFORE_SHUTDOWN
			post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, HALT_SYSTEM,1, NULL, NULL);
#else
			dispatch_shutdown(HALT_SYSTEM);
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
			dispatch_shutdown(COLDSTART_SYSTEM);
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
						if( !(wi->window_status & XAWS_OPEN) || (wi->window_status & XAWS_BINDFOCUS)
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
			if( fr->no_exit == false )
				close_window(lock, wind);

			break;
		}
		default:
		{
			DIAGS(("taskmanager: unhandled event %i", fr->obj.item));
			break;
		}
	}
	Sema_Dn(LOCK_CLIENTS);
}

/*
 * return true if AES-client AND valid client->name
 */
struct xa_client *is_aes_client( struct proc *p )
{
	struct xa_client *client;
	FOREACH_CLIENT(client)
	{
		if( client->p == p )
		{
			if( *(uchar*)(client->name+2) > ' ' )
				return client;
			else
				return NULL;
		}
	}
	return NULL;
}

#define OLD_MAXLOAD	4095L	/* ?? */
#define MAXLOAD	234L
#define WITH_ACTLD  0x4143544CL /* 'ACTL' */

static bool calc_tm_bar( OBJECT *obtree, short item, short parent, long pinf, long max )
{
	TEDINFO *t = object_get_tedinfo(&obtree[item], NULL);

	if( max == 0L )
		obtree[item].ob_height = 0;
	else
	{
		obtree[item].ob_height = (short)(pinf * (long)obtree[parent].ob_height / max );

		/* if height = 0 a bar is drawn anyway!? */
		if( obtree[item].ob_height <= 0 )
			obtree[item].ob_height = 1;
	}

	if( t )
	{
		OBJC_COLORWORD *c = (OBJC_COLORWORD *)&t->te_color;
		unsigned long v = (unsigned long)obtree[item].ob_height * 100L / (unsigned long)obtree[parent].ob_height;

		/* mark levels with different colors */
		if( v > 100UL )	/* happens sometimes */
		{
			c->fillc = G_MAGENTA;
			obtree[item].ob_height = obtree[parent].ob_height;
		}
		else if( v > 95 )
		{
			c->fillc = G_RED;
		}
		else if( v > 75 )
		{
			c->fillc = G_LRED;
		}
		else if( v > 25 )
		{
			c->fillc = G_CYAN;
		}
		else{
			c->fillc = G_GREEN;
		}
	}

	obtree[item].ob_y = obtree[parent].ob_height - obtree[item].ob_height;

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
	static long systime = 0, old_tim = 0;
	long ud, utim, rsystime = rootproc->systime, new_ld = 0;
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

static unsigned short
tm_slist_key(struct scroll_info *list, unsigned short keycode, unsigned short keystate)
{
	switch( keycode )
	{
	case SC_TAB:
	{
    struct fmd_result fr;
    fr.obj.item = TM_OK;
    fr.no_exit = true;
		taskmanager_form_exit( 0, 0, list->wt, &fr );
		keycode = 0;
	}
	break;
	case SC_DEL:
	{
		if( keystate & (K_RSHIFT|K_LSHIFT) )
		{
			keycode = 0;
			kill_client( list );
		}
		else if( keystate == 0 )
		{
			keycode = 0;
			term_client(LOCK_CLIENTS, list );
		}
	}
	}
	switch( keycode & 0xff )
	{
	case 'S': case 's':
		keycode = 0;
		stop_cont_client(LOCK_CLIENTS, list, SIGSTOP );
	break;
	case 'Q': case 'q':
		keycode = 0;
		stop_cont_client(LOCK_CLIENTS, list, SIGCONT );
	break;
	}
	return keycode;
}

static void do_tm_chart(int lock, XA_TREE *wt, struct proc *rootproc, struct xa_window *wind)
{
	if( !( wt->tree[TM_CHART].ob_flags & OF_HIDETREE) )
	{
		static int has_new_uptime = -1;
		long pinfo[8];
		long u = 0;

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
		/* idle = pinfo[2] * 1000L + pinfo[3]; */
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
#endif
		calc_tm_bar( wt->tree, TM_LOAD1, TM_CPU, pinfo[0], MAXLOAD );
		calc_tm_bar( wt->tree, TM_LOAD2, TM_CPU, pinfo[1], MAXLOAD );
		calc_tm_bar( wt->tree, TM_LOAD3, TM_CPU, pinfo[2], MAXLOAD );
		pinfo[3] = calc_new_ld( rootproc );
		calc_tm_bar( wt->tree, TM_ACTLD, TM_CPU, pinfo[3], MAXLOAD );

		pinfo[0] = 13;
		pinfo[1] = 16;
		pinfo[2]= 19;
		pinfo[3] = 22;
		pinfo[4] = 25;
		pinfo[5]= 28;
		pinfo[6]= 0;

		ker_stat( 0, "meminfo", pinfo);

		calc_tm_bar( wt->tree, TM_MEM, TM_RAM, pinfo[0] - pinfo[1], pinfo[0] );		/* total */
		calc_tm_bar( wt->tree, TM_FAST, TM_RAM, pinfo[2] - pinfo[3], pinfo[2] );	/* fast */
		calc_tm_bar( wt->tree, TM_CORE, TM_RAM, pinfo[4] - pinfo[5], pinfo[4] );	/* core */

		redraw_toolbar( lock, wind, TM_CHART );
	}
}

static void add_meminfo( struct scroll_info *list, struct scroll_entry *this )
{
	struct scroll_content sc = {{ 0 }};
	long uinfo[4];
	struct scroll_entry *to = 0;

	sc.t.strings = 1;
	sc.data = TM_MEMINFO_DATA;
	sc.xflags = 0;
	{
		struct sesetget_params p = { 0 };
		p.arg.data = TM_MEMINFO_DATA;
		if( list->get(list, NULL, SEGET_ENTRYBYDATA, &p) )
		{
			to = p.e;
			if( to && to->next )
			{
				list->del( list, to, NORMREDRAW );	/* meminfo always last */
				to = 0;
			}
		}
	}
	sc.usr_flags = TM_MEMINFO;

	uinfo[0] = 1;
	uinfo[1] = (long)"used: ";
	uinfo[2] = 2;
	uinfo[3] = 0;
	add_kerinfo( "u:/kern/meminfo", list, this, to, &sc, PROCINFLEN, 5, NORMREDRAW, uinfo, false, NULL, 0 );
}

GRECT taskman_r = { 0, 0, 0, 0 };

void
open_taskmanager(int lock, struct xa_client *client, short open)
{
	struct helpthread_data *htd;
	struct xa_window *wind;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;
	GRECT or;
	int redraw = NOREDRAW;

	struct xa_wtxt_inf *wp[] = {&norm_txt, &acc_txt, &prg_txt, &sys_txt, &sys_thrd, &desk_txt, 0};
	int objs[] = {TM_ICN_MENU, TM_ICN_XAAES, 0};

	htd = get_helpthread_data(client);
	if (!htd)
		return;

	if (!htd->w_taskman)
	{
		struct scroll_info *list;
		int tm_ticks[] = {TM_TICK1, 25, 1, TM_TICK2, 50, 2, TM_TICK3, 75, 1, 0, 0, 0};
		short minw, minh;

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, TASK_MANAGER), 0);
		if (!obtree) goto fail;

		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
#if 1
		/* resize window if < 10-point-font */
		if( screen.c_max_h < 14 ){
			short i, rs[4] = {TM_CHART, TM_CPU, TM_RAM, 0};
			short d = 16 / screen.c_max_h, dy;

			obtree->ob_height *= d;

			(obtree+TM_LIST)->ob_height *= d;
			(obtree+TM_LIST)->ob_height += d/2;
			(obtree+TM_LIST)->ob_y += obtree->ob_y + 18;

			for( i = 0; rs[i]; i++ )
			{
				(obtree + rs[i])->ob_height *= d;
			}
			(obtree + TM_CHART)->ob_y = (obtree+TM_LIST)->ob_y + (obtree+TM_LIST)->ob_height;

			dy = obtree->ob_y + obtree->ob_height - ((obtree + TM_OK)->ob_y + (obtree+TM_OK)->ob_height) - 8;
			for( d = TM_QUITAPPS; d < TM_CHART; d++ )
			{
				(obtree+d)->ob_y += dy;
			}
		}
#endif
		obj_rectangle(wt, aesobj(obtree, 0), &or);
		minw = obtree[0].ob_width;
		minh = obtree[0].ob_height;
		if (taskman_r.g_w)
		{
			short dw = taskman_r.g_w - or.g_w - 8;	/* !! */
			short dh = taskman_r.g_h - or.g_h - 30;	/* !! */
			int i;
			obtree[TM_LIST].ob_width += dw;
			obtree[TM_LIST].ob_height += dh;
			for( i = TM_QUITAPPS; i <= TM_RESCHG; i++ )
			{
				obtree[i].ob_x += dw;
				obtree[i].ob_y += dh;
			}
			obtree[XAAES_32].ob_x += dw;
			obtree[TM_CHART].ob_y += dh;
		}
		list = set_slist_object(0, wt, NULL, TM_LIST,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_TREEVIEW|SIF_ICONINDENT|SIF_AUTOOPEN|SIF_AUTOSLIDERS,
				 NULL, NULL, NULL, NULL, NULL, tm_slist_key,
				 NULL, NULL, NULL, NULL,
         xa_strings(RS_APPLST), "      name            pid ppid pgrp pri DOM  STATE       SZ         CPU  % args", NULL, 255);

		if (!list) goto fail;

		obtree[TM_ICONS].ob_flags |= OF_HIDETREE;

		set_xa_fnt( cfg.xaw_point, wp, obtree, objs, list, 0, 0);
		/* Work out sizing */
		if (!taskman_r.g_w)
		{
			center_rect(&or);
			taskman_r = calc_window(lock, client, WC_BORDER,
						BORDER|CLOSER|NAME|TOOLBAR, created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						&or);
		}
		else
		{
			or = taskman_r;
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag, do_formwind_msg,
					client,
					false,
					BACKDROP|BORDER|CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
					created_for_AES,
					C.Aes->options.thinframe,
					C.Aes->options.thinwork,
					&taskman_r, NULL, NULL);

		if (!wind) goto fail;
		/* minimum height for this window */
		wind->min.g_h = minh;
		/* minimum width for this window */
		wind->min.g_w = minw;

		wind->sw = 3;	/* border for moving objects when resizing */
		list->set(list, NULL, SESET_PRNTWIND, (long)wind, NOREDRAW);
		wind->window_status |= XAWS_NODELETE;

		/* Set the window title */
		set_window_title(wind, xa_strings(RS_TASKMANAGER), false);

		wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		wt->exit_form = taskmanager_form_exit;

		wt->focus = aesobj( obtree, TM_LIST );	/*cursor on list */
		init_list_focus( obtree, TM_LIST, 0 );

		make_tm_ticks( obtree, tm_ticks );

		/* Set the window destructor */
		wind->destructor = taskmanager_destructor;

		htd->w_taskman = wind;

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
			struct proc *rootproc = pid2proc(0);
			static int first = 1;	/* do full-redraw for correct slider-size */

			wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;
			list = object_get_slist(wt->tree + TM_LIST);

			if( first == 0 )
				redraw = NORMREDRAW;
			do_tm_chart(lock, wt, rootproc, wind);

			if( list )
			{
				struct scroll_entry *this;
				/* todo: to change fnt-size at runtime font-info for each list-entry would have to be updated */

				for( this = list->start; this; this = this->next )
					this->usr_flags &= ~TM_UPDATED;

				FOREACH_CLIENT(client)
				{				/* */
					update_tasklist_entry( AES_CLIENT, client, htd, 0, redraw);
				}
				/* add non-client-procs */
				{
					long i = d_opendir("u:/kern", 0);
					if (i > 0)
					{
						char nm[128];
						struct proc *pr;
						long pid;

						update_tasklist_entry( NO_AES_CLIENT, 0, htd, 0, redraw );	/* kernel */

						while( d_readdir(127, i, nm) == 0)
						{
							if( isdigit( nm[4] ) )
							{
								pid = atol(nm+4);
								pr = pid2proc( pid );
								if( pr )
								{
									if( !is_aes_client(pr) )
									{
										update_tasklist_entry( NO_AES_CLIENT, 0, htd, pid, redraw );
									}
								}
							}
						}
						d_closedir(i);

						add_meminfo( list, this );

						/* delete exited entries */
						{
						struct scroll_entry *this_next;
						for( this = list->start; this; this = this_next )
						{
							this_next = this->next;
							if( !(this->usr_flags & (TM_MEMINFO|TM_UPDATED|TM_HEADER) ) )
							{
								list->del( list, this, redraw );
							}
						}
						}
					}
				}
				if( first == 1 )
				{
					list->redraw( list, NULL );
					first = 0;
				}
			}	/* /if( list ) */

			if( S.focus != wind )
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

struct xa_window * _cdecl
create_dwind(int lock, XA_WIND_ATTR tp, char *title, struct xa_client *client, struct widget_tree *wt, FormExit(*f), WindowDisplay(*d))
{
	struct xa_window *wind;
	OBJECT *obtree = wt->tree;
        GRECT r, or;

	obj_rectangle(wt, aesobj(obtree, 0), &or);

	center_rect(&or);

	r = calc_window(lock, client, WC_BORDER,
			CLOSER|NAME, created_for_AES,
			client->options.thinframe,
			client->options.thinwork,
			&or);

	/* Create the window */
	wind = create_window(lock,
				do_winmesag, do_formwind_msg,
				client,
				false,
				tp | (title ? NAME : 0) | TOOLBAR | hide_move(&(client->options)),
				created_for_AES,
				client->options.thinframe,
				client->options.thinwork,
				&r, NULL, NULL);

	/* Set the window title */
	if (title)
		set_window_title(wind, title, false);

	wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
	wt->exit_form = f; /* milan_reschg_form_exit; */

	/* Set the window destructor */
	wind->destructor = d;

	return wind;
}

/*
 * client still running dialog
 */
static int
csr_destructor(int lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_csr = NULL;
	return true;
}

static void
csr_form_exit(struct xa_client *Client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr)
{
	int lock = 0;

	Sema_Up(LOCK_CLIENTS);
	lock |= LOCK_CLIENTS;

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
	Sema_Dn(LOCK_CLIENTS);
}

static void
open_csr(int lock, struct xa_client *client, struct xa_client *running)
{
	struct xa_window *wind;
	struct helpthread_data *htd;
	XA_TREE *wt = NULL;
	OBJECT *obtree = NULL;

	if (!(htd = get_helpthread_data(client)))
		return;

	if( running->p->pid == 0 )	/*!??*/
		return;


	if (!htd->w_csr)
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
			uchar *s = (uchar*)running->name;

			while (*s && *s == ' ')
				s++;

			for (; i < 32 && (t->te_ptext[i] = *s++); i++)
				;
		}
		if( i == 0 )
		{
			if( (uchar)running->proc_name[0] > ' ' )
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
CE_open_csr(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		open_csr(0, ce->client, ce->ptr1);
	}
}

void
CE_abort_csr(int lock, struct c_event *ce, short cancel)
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
handle_launcher(int lock, struct fsel_data *fs, const char *path, const char *file)
{
	char parms[128], args[128], *p;
	short len = 0;

	close_fileselector(lock, fs);

	/* provide commandline-args for launched program (fails if prg-name contains blanks) */
	if( (long)fs->data == HL_LAUNCH && (p = strchr( file, ' ' ) ) )
	{
		*p++ = 0;
	}
	else
		p = 0;
	sprintf(parms+1, sizeof(parms)-1, "%s%s", path, file);
	parms[0] = '\0';
	if( p )
	{
		strncpy(args+1, p, sizeof(args)-1);
		len = args[0] = strlen(args+1);
		p = args;
	}
	else
		p = parms;

	DIAGS(("launch: \"%s\"", parms+1));

	switch( (long)fs->data )
	{
		case HL_LAUNCH:
			launch(lock, 0, 0, -len, parms+1, p, C.Aes);
		break;
#if WITH_GRADIENTS
		case HL_LOAD_GRAD:
			load_grd( parms+1 );
		break;
#endif
#if WITH_BKG_IMG
		case HL_LOAD_IMG:
			load_bkg_img( parms + 1 );
		break;
#endif
		case HL_LOAD_PAL:
			load_palette( parms+1 );
		break;
		case HL_LOAD_CFG:
			load_config( parms+1 );
		break;
	}
}

#if FILESELECTOR

static struct fsel_data aes_fsel_data;

void
open_launcher(int lock, struct xa_client *client, int what)
{
	struct fsel_data *fs = &aes_fsel_data;
	char *path, *text;
	char pbuf[PATH_MAX];
	switch( what )
	{
#if WITH_GRADIENTS
		case 0:
			if( !cfg.gradients[0] )
				return;
			path = pbuf;
			text = xa_strings(RS_LDGRAD);
			sprintf( pbuf, sizeof(pbuf), "%s%s", C.Aes->home_path, "gradient\\*.grd" );
		break;
#endif
		case 1:
			path = cfg.launch_path;
			text = xa_strings(RS_LAUNCH);
		break;
#if WITH_BKG_IMG
		case 2:
			path = pbuf;
			make_bkg_img_path( path, sizeof(pbuf)-16 );
			strcat( path, "\\*."BKGIMG_EXT );
			text = xa_strings(RS_LDIMG);
		break;
#endif
		case 3:
			path = pbuf;
			text = xa_strings(RS_LDPAL);
			sprintf( pbuf, sizeof(pbuf), "%s%s", C.Aes->home_path, "pal\\*.pal" );
		break;
		case 4:
			path = pbuf;
			text = xa_strings(RS_LDCNF);
			sprintf( pbuf, sizeof(pbuf), "%s%s", C.Aes->home_path, "*.cnf" );
		break;
		default:
			return;
	}

	/* if lauchpath defined but is no path discard it */
	if (*path)
	{
		struct stat st;
		long r;
		char *p = strrchr( path, '\\' ), c=0;

		if( !p )
			p = strrchr( path, '/' );
		if( p )
		{
			c = *p;
			*p = 0;
		}

		r = f_stat64(0, path, &st);
		if (r != 0 || !S_ISDIR(st.mode) )
			*path = 0;
		else if( p )
			*p = c;
	}
	if (!*path)
	{
		path[0] = letter_from_drive(d_getdrv());
		path[1] = ':';
		path[2] = '\\';
		path[3] = '*';
		path[4] = 0;
	}

	open_fileselector(lock, client, fs,
			  path,
			  NULL, text,
			  handle_launcher, NULL, (void*)(long)what);
}
#endif



/* double click now also available for internal handlers. */
static void
sysalerts_form_exit(struct xa_client *Client,
		    struct xa_window *wind,
		    struct widget_tree *wt,
		    struct fmd_result *fr)
{
	int lock = 0;
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

			p.arg.txt = xa_strings(RS_ALERTS);
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
			break;
		}
	}
}

static int
systemalerts_destructor(int lock, struct xa_window *wind)
{
	struct helpthread_data *htd = lookup_xa_data_byname(&wind->owner->xa_data, HTDNAME);
	if (htd)
		htd->w_sysalrt = NULL;
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
static void kerinfo2line( uchar *in, uchar *out, long maxlen )
{
	uchar *pi = in, *po = out;
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
	*po = 0;
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
	uchar path[256];
	struct file *fp;
	long err;
	int ret = 0;

	if( pid )
		sprintf( (char*)path, sizeof(path), "u:/kern/%d/%s", pid, what );
	else
		sprintf( (char*)path, sizeof(path), "u:/kern/%s",what );
	fp = kernel_open( (char*)path, O_RDONLY, &err, NULL );
	if( !fp )
	{
		return 1;
	}

	err = kernel_read( fp, path, sizeof(path)-1 );
	if( err > 0 && err < sizeof(path) )
	{
		int i, j;
		uchar *p = path;
		path[err] = 0;
		for( j = 0, i = 1; *p && pinfo[j]; i++ )
		{
			for( ; *p && *p <= ' '; p++ );
			if( i == pinfo[j] )
			{
				if( !isdigit( *p ) )
					return 3;
				pinfo[j++] = atol( (char*)p );
			}
			for( ; *p && !(*p <= ' ' || *p == '.'); p++ );
			if( *p )
			{
				p++;
			}
		}
		if( !*p && pinfo[j] )
			ret = 4;
	}
	kernel_close(fp);
	if( err <= 0 )
	{
		return 2;
	}
	return ret;
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
	long features = Ssystem(S_OSFEATURES,0,0);

#if CHECK_STACK
	extern unsigned short stack_align;
	int l = sprintf( s, sizeof(s)-1, xa_strings(RS_MEMPROT), (features & 1) ? xa_strings(RS_ON) : xa_strings(RS_OFF));
	sprintf( s+l, sizeof(s)-1-l, ", Stack: %x", stack_align );
	BLOG((0,s));
#else
	sprintf( s, sizeof(s)-1, xa_strings(RS_MEMPROT), (features & 1) ? xa_strings(RS_ON) : xa_strings[(RS_OFF));
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
	int redraw, long *pinfo, bool child,
	char *out, size_t outl
)
{
	long err;
	struct file *fp;
	fp = kernel_open( path, O_RDONLY, &err, NULL );
	if(fp)
	{
		uchar line[512], sstr[512];

		err = kernel_read( fp, sstr, sizeof(sstr)-1 );
		kernel_close(fp);
		sstr[err-1] = 0;

		if( err > 0 )
		{
			int i, j = 0, p=0, l;
			uchar *sp;
			for( sp = sstr, l = i = 0; l < startline && i < err; i++ )
			{
				if( sstr[i] == '\n' )
				{
					/* grab words from lines before startline */
					if( pinfo && pinfo[p] && pinfo[p] == l )
					{
						uchar *cp, *cpx;
						int k;
						for( cp = sp, k = 0; k < pinfo[p+2]; k++ )
						{
							for( ; *cp > ' '; cp++ );
							for( ; *cp && *cp <= ' '; cp++ );
						}
						for( cpx = cp; *cpx > ' '; cpx++ );
						*cpx = 0;
						if( cp && *cp && sizeof(line) > j + 32)
							j += sprintf( (char*)line+j, sizeof(line)-j-1, "%s%s ", (char *)pinfo[p+1], cp );
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
			sstr[err+i-1] = 0;

			kerinfo2line( sstr+i, line+j, sizeof(line)-j-1 );

			if( to )
			{
				struct setcontent_text t = { 0 };
				struct se_content *sec = to->content;
				t.text = (char*)line;
				/* set only if changed */
				if( !sec || strcmp( t.text, sec->c.text.text ) )
				{
					list->set(list, to, SESET_TEXT, (long)&t, redraw);
				}
			}
			else
			{
				sc->t.text = (char*)line;
				sc->fnt = &norm_txt;
				list->add(list, this, 0, sc, child ? (this ? (SEADD_CHILD) : SEADD_PRIOR) : 0, 0, redraw);
			}
		}
		if( out )
			strncpy( (char*)out, (char*)line, outl );
	}
	else
		BLOG((0,"add_kerinfo:could not open %s err=%ld", path, err ));

}


GRECT systemalerts_r = { 0, 0, 0, 0 };

void
open_systemalerts(int lock, struct xa_client *client, short open)
{
	struct helpthread_data *htd;
	OBJECT *obtree = NULL;
	struct widget_tree *wt = NULL;
	struct xa_window *wind;


	if (!(htd = get_helpthread_data(client)))
		return;

	if (!htd->w_sysalrt)
	{
		struct scroll_info *list;
		GRECT or;
		short minw, minh;
		char *a = xa_strings(RS_ALERTS);
		char *e = xa_strings(RS_ENV);
		char *s = xa_strings(RS_SYSTEM);
		int objs[] = {SALERT_IC1, SALERT_IC2, 0};

		obtree = duplicate_obtree(client, ResourceTree(C.Aes_rsc, SYS_ERROR), 0);
		if (!obtree) goto fail;
		wt = new_widget_tree(client, obtree);
		if (!wt) goto fail;
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;

		obj_rectangle(wt, aesobj(obtree, 0), &or);
		minw = obtree[SYSALERT_LIST].ob_width;
		minh = obtree[SYSALERT_LIST].ob_height;
		if (systemalerts_r.g_w)
		{
			short dw = systemalerts_r.g_w - or.g_w - 8;	/* !! */
			short dh = systemalerts_r.g_h - or.g_h - 30;	/* !! */
			obtree[SYSALERT_LIST].ob_width += dw;
			obtree[SYSALERT_LIST].ob_height += dh;
			obtree[SALERT_CLEAR].ob_x += dw;
			obtree[SALERT_CLEAR].ob_y += dh;
			obtree[SALERT_OK].ob_x += dw;
			obtree[SALERT_OK].ob_y += dh;
		}
		list = set_slist_object(0, wt, NULL, SYSALERT_LIST,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_TREEVIEW|SIF_AUTOOPEN|SIF_AUTOSLIDERS,
				 NULL, NULL, NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, NULL, 255);

		if (!list) goto fail;

		set_xa_fnt( cfg.xaw_point, 0, obtree, objs, list, 0, 0);

		/* todo: set focus into list (?) */
		obj_init_focus(wt, OB_IF_RESET);

		{
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
			struct scroll_entry *this;
			const char **strings = get_raw_env();
			int i;
			char text[255];
			struct sesetget_params p = { 0 };
			struct scroll_content sc = {{ 0 }};
			char sstr[1024];

			p.idx = -1;
			p.arg.txt = e;
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

				sprintf( sstr, sizeof(sstr)-1, xa_strings(RS_VIDEO),/* "Video:%dx%dx%d,%d colours, format: %s" */
					screen.r.g_w, screen.r.g_h, screen.planes, screen.colours, fs );
			}

			sc.t.text = sstr;
			list->add(list, this, 0, &sc, this ? (SEADD_CHILD) : SEADD_PRIOR, 0, true);

			/* cpuinfo */
			add_kerinfo( "u:/kern/cpuinfo", list, this, NULL, &sc, 0, 0, false, NULL, true, text, sizeof(text) );
			BLOG((0,"cpuinfo:%s", text ));
			add_os_features(list, this, &sc);


			this = add_title_string(list, this, "Kernel");

			add_kerinfo( "u:/kern/version", list, this, NULL, &sc, 0, 0, false, NULL, true, text, sizeof(text) );
			BLOG((0,"version:%s", text ));
#if !XAAES_RELEASE
			add_kerinfo( "u:/kern/buildinfo", list, this, NULL, &sc, 0, 1, false, NULL, true, text, sizeof(text) );
			BLOG((0,"buildinfo:%s", text ));
#endif
			init_list_focus( obtree, SYSALERT_LIST, 0 );
		}

		obj_rectangle(wt, aesobj(obtree, 0), &or);

		obtree[SALERT_ICONS].ob_flags |= OF_HIDETREE;

		/* Work out sizing */
		if (!systemalerts_r.g_w)
		{
			center_rect(&or);
			systemalerts_r = calc_window(lock, client, WC_BORDER,
						BORDER|CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
						created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						&or);
		}
		else
		{
			or = systemalerts_r;
		}

		/* Create the window */
		wind = create_window(lock,
					do_winmesag,
					do_formwind_msg,
					client,
					false,
					BACKDROP|BORDER|CLOSER|NAME|TOOLBAR|hide_move(&(C.Aes->options)),
					created_for_AES,
					C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						&systemalerts_r,
					NULL, NULL);
		if (!wind) goto fail;

		/* minimum height for this window */
		wind->min.g_h = minh;
		/* minimum width for this window */
		wind->min.g_w = minw;

		list->set(list, NULL, SESET_PRNTWIND, (long)wind, NOREDRAW);

		/* Set the window title */
		set_window_title(wind, xa_strings(RS_SYS), false);

		wt = set_toolbar_widget(lock, wind, client, obtree, inv_aesobj(), 0, STW_ZEN, NULL, &or);
		wt->exit_form = sysalerts_form_exit;
		wt->focus = aesobj( obtree, SYSALERT_LIST );	/*cursor on list */
		/* Set the window destructor */
		wind->destructor = systemalerts_destructor;

		refresh_systemalerts(obtree);

		if (open)
		{
			open_window(lock, wind, wind->r);
		}

		htd->w_sysalrt = wind;
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
do_system_menu(int lock, int clicked_title, int menu_item)
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
			dispatch_shutdown(0);
			break;

		case SYS_MN_RESTART:
			dispatch_shutdown(RESTART_XAAES);
			break;

		/* Open the "Task Manager" window */
		case SYS_MN_TASKMNG:
			post_cevent(C.Hlp, ceExecfunc, open_taskmanager,NULL, 1,0, NULL,NULL);
			break;
		/* Open system alerts log window filled with environment */
		case SYS_MN_SYSTEM:
		{
			post_cevent(C.Hlp, ceExecfunc, open_systemalerts, NULL, 1,0, NULL,NULL);
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
				ALERT((xa_strings(AL_SHELLRUNS)));
			else if (!*C.desk)
				ALERT((xa_strings(AL_NOSHELL)));
			else
				C.DSKpid = launch(lock, 0,0,0, C.desk, "\0", C.Aes);
			break;
		}
	}
}
