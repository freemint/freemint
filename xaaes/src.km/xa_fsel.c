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
 * FILE SELECTOR IMPLEMENTATION
 * 
 * This give a non-modal windowed file selector for internal
 * and exported use, with slightly extended pattern matching
 * from the basic GEM spec.
 */

#include RSCHNAME

#include "xa_fsel.h"
#include "xa_global.h"

#include "c_window.h"
#include "form.h"
#include "k_main.h"
#include "matchpat.h"
#include "menuwidg.h"
#include "nkcc.h"
#include "obtree.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "util.h"
#include "widgets.h"

#include "xa_graf.h"
#include "xa_rsrc.h"

#include "mint/pathconf.h"
#include "mint/stat.h"



static char fs_paths[DRV_MAX][NAME_MAX+2];

void
init_fsel(void)
{
	int i;
	for (i = 0; i < DRV_MAX; i++)
		fs_paths[i][0] = 0;
}

static char *
getsuf(char *f)
{
	char *p;

	if ((p = strrchr(f, '.')) != 0L)
	{
		if (strchr(p, '/') == 0L
		    && strchr(p, '\\') == 0L)
			/* didnt ran over slash? */
			return p+1;
	}
	/* no suffix  */
	return 0L;
}

/*
 *  find typed characters in the list
 */

/* This one didnt exist.
   It doesnt reinvent the wheel. :-)	*/
static char *
stristr(char *in, char *s)
{
	char xs[32], xin[32];
	char *x;

	if (strlen(s) < 32 && strlen(in) < 32)
	{
		strcpy(xs, s);
		strlwr(xs);
		strcpy(xin, in);
		strlwr(xin);

		x = strstr(xin, xs);
		if (x)
			x = in + (xin - x);
	}
	else
		x = strstr(in, s);

	return x;
}

static bool
fs_prompt(SCROLL_INFO *list)
{
	struct fsel_data *fs = list->data;
	SCROLL_ENTRY *s;

	s = list->start;

	/* Not if filename empty or list empty */
	if (*fs->file && s)
	{
		SCROLL_ENTRY *old = list->cur;

		list->cur = NULL;	/* s1 < s2   ==>   -1 */
		while (s->next && stricmp(s->text, fs->file) < 0)
			s = s->next;

		/* is the text completely at the beginning of a entry ? */
		if (stristr(s->text, fs->file) == s->text)
		{
			list->cur = s;
		}
		else if (s->flag & FLAG_DIR)
		{
			while(s->next && (s->flag & FLAG_DIR))
				s = s->next;
			while (s->next && stricmp(s->text, fs->file) < 0)
				s = s->next;
			if (stristr(s->text, fs->file) == s->text)
				list->cur = s;
		}

		list->vis(list, s, true);

		/* prompt changed the selection... */
		fs->clear_on_folder_change |= old == list->cur;

		return true;
	}

	return false;
}

static bool
fs_prompt_refresh(SCROLL_INFO *list)
{
	struct fsel_data *fs = list->data;
	int clear = fs->clear_on_folder_change;
	bool res = fs_prompt(list);
	fs->clear_on_folder_change = clear;
	return res;
}

typedef bool sort_compare(SCROLL_ENTRY *s1, SCROLL_ENTRY *s2);

static bool
dirflag_name(SCROLL_ENTRY *s1, SCROLL_ENTRY *s2)
{
	unsigned short f1 = s1->flag & FLAG_DIR;
	unsigned short f2 = s2->flag & FLAG_DIR;

	if (f1 < f2)
		/* folders in front */
		return true;

	if (f1 == f2 && stricmp(s1->text, s2->text) > 0)
		return true;

	return false;
}

static void
sort_entry(SCROLL_INFO *list, SCROLL_ENTRY *new, sort_compare *greater)
{
	/* temporary use of ->cur for sorting */
	if (list->start)
	{
		SCROLL_ENTRY *c = list->cur;

		/* new > c */
		if (greater(new, c))
		{
			c = c->next;
			while (c)
			{
				/* while new < c */
				if (greater(new, c))
				{
					c = c->next;
				}
				else /* new < c */
				{
					/* insert before c */
					new->next = c;
					new->prev = c->prev;
					c->prev->next = new;
					c->prev = new;
					break;
				}
			}
			if (!c)
			{
				/* put at end */
				list->last->next = new;
				new->prev = list->last;
				list->last = new;
			}
		}
		else /* new < c */
		{
			c = c->prev;
			while (c)
			{
				/* while new < c */
				if (greater(c, new))
				{
					c = c->prev;
				}
				else /* new > c */
				{
					/* insert after c */
					new->next = c->next;
					new->prev =c;
					c->next->prev = new;
					c->next = new;
					break;
				}
			}
			if (!c)
			{
				/* put in front */
				list->start->prev = new;
				new->next = list->start;
				list->start = new;
			}
		}
	}
	else
		list->start = list->last = new;

	list->cur = new;
}

/* yields true if case sensitive */
static bool
inq_xfs(struct fsel_data *fs, char *p, char *dsep)
{
	
	long c, t;

	c = fs->fcase = d_pathconf(p, DP_CASE);
	t = fs->trunc = d_pathconf(p, DP_TRUNC);

	DIAG((D_fsel, NULL, "inq_xfs '%s': case = %ld, trunc = %ld", p, c, t));

	if (dsep)
	{
		* dsep    = '\\';
		*(dsep+1) = 0;
	}

	return !(c == 1 && t == 2);
}

static void
add_slash(char *p, char *to)
{
	char *pl = p + strlen(p) - 1;

	if (*pl != '/' && *pl != '\\')
		strcat(p, to);
}

static bool
executable(char *nam)
{
	char *ext = getsuf(nam);

	return (ext
		/* The mintlib does almost the same :-) */
		&& (   !stricmp(ext, "ttp") || !stricmp(ext, "prg")
		    || !stricmp(ext, "tos") || !stricmp(ext, "g")
		    || !stricmp(ext, "sh")  || !stricmp(ext, "bat")
		    || !stricmp(ext, "gtp") || !stricmp(ext, "app")
		    || !stricmp(ext, "acc")));
}

static void
set_file(enum locks lock, struct fsel_data *fs, const char *fn)
{
	XA_TREE *wt = get_widget(fs->wind, XAW_TOOLBAR)->stuff;

	DIAG((D_fsel, NULL, "set_file: fs.file='%s', wh=%d", fn, fs->wind->handle));

	/* fixup the cursor edit position */
#if 0
	/* FIXME: the edit_object() doesn't work in case the client
	 * didn't call the form_do().
	 *
	 * We should call something like the below instead
	 * of direct e.pos adjustments.
	 */
	{
		short newpos;
		edit_object(lock, wt->owner, ED_END, wt, wt->tree, FS_FILE, 0, &newpos);
		strcpy(fs.file, fn); /* set the edit field text */
		edit_object(lock, wt->owner, ED_INIT, wt, wt->tree, FS_FILE, 0, &newpos);
	}
#else
	strcpy(fs->file, fn); /* set the edit field text */
	wt->e.pos = strlen(fn);
#endif

	/* redraw the toolbar file object */
	display_toolbar(lock, fs->wind, FS_FILE);
}

/*
 * Re-load a file_selector listbox
 * HR: version without the big overhead of separate dir/file lists
       and 2 quicksorts + the moving around of all that.
       Also no need for the Mintlib. (Mintbind only).
 */
static void
refresh_filelist(enum locks lock, struct fsel_data *fs, int which)
{
	OBJECT *form = fs->form->tree;
	OBJECT *sl;
	SCROLL_INFO *list;
	char nm[NAME_MAX+2];
	int n = 0;
	long i, rep;
	bool csens;

	sl = form + FS_LIST;
	DIAG((D_fsel, NULL, "refresh_filelist: fs = %lx, obtree = %lx, sl = %lx",
		fs, fs->form->tree, sl));
	list = object_get_slist(sl);
	add_slash(fs->path, fs->fslash);
	csens = inq_xfs(fs, fs->path, fs->fslash);

#ifdef FS_DBAR
	{
		TEDINFO *tx;

		tx = ob_spec(form + FS_CASE);
		sprintf(tx->te_ptext, 32, "%ld", fs->fcase);

		tx = ob_spec(form + FS_TRUNC);
		sprintf(tx->te_ptext, 32, "%ld", fs->trunc);

		display_toolbar(lock, fs->wind, FS_DBAR);
	}
#endif

	DIAG((D_fsel, NULL, "[%d]refresh_filelist: fs.path='%s',fs_pattern='%s'",
		which, fs->path, fs->fs_pattern));

	/* Clear out current file list contents */
	free_scrollist(list);

	graf_mouse(HOURGLASS, NULL, NULL, false);

	i = d_opendir(fs->path, 0);
	DIAG((D_fsel, NULL, "Dopendir -> %lx", i));
	if (i > 0)
	{
		struct xattr xat;
		SCROLL_ENTRY *entry;

		while (d_xreaddir(NAME_MAX, i, nm, &xat, &rep) == 0)
		{
			char *nam = nm+4;
			bool dir = S_ISDIR(xat.mode);
			bool sln = S_ISLNK(xat.mode);
			bool match = true;

			if (!csens)
				strupr(nam);

			if (sln) 
			{
				char fulln[NAME_MAX+2];

				strcpy(fulln, fs->path);
				strcat(fulln, nam);

				DIAG((D_fsel, NULL, "Symbolic link: Fxattr on '%s'", fulln));
				f_xattr(0, fulln, &xat);
				dir = (xat.mode & 0xf000) == S_IFDIR;

				DIAG((D_fsel, NULL, "After Fxattr dir:%d", dir));
			}

			if (!dir)
				match = match_pattern(nam, fs->fs_pattern);

			if (match)
			{
				OBJECT *icon = NULL;

				entry = kmalloc(sizeof(*entry) + strlen(nam) + 2);
				assert(entry);

				entry->text = entry->the_text;
				strcpy(entry->text, nam);

				if (sln)
					entry->flag |= FLAG_LINK;

				if (dir)
				{
					icon = form + FS_ICN_DIR;
					/* text part of the entry, so FLAG_MAL is off */
					entry->flag |= FLAG_DIR;
				}
				else if (executable(nam))
					icon = form + FS_ICN_EXE;

				if (icon)
					icon->ob_x = icon->ob_y = 0;

				entry->icon = icon;
				sort_entry(list, entry, dirflag_name);
			}
		}
		d_closedir(i);
		list->cur = NULL;
		entry = list->start;
		while (entry)
		{
			entry->n = ++n;
			if (entry->n == list->s)
				list->bot = entry;
			entry = entry->next;
		}
	}

	DIAG((D_fsel, NULL, "%d entries have been read", n));
	
	list->top = list->cur = list->start;
	list->n = n;
	graf_mouse(ARROW, NULL, NULL, false);
	list->slider(list);
	if (!fs_prompt_refresh(list))
		display_toolbar(lock, fs->wind, FS_LIST);
}

#define DRIVELETTER(i)	(i + ((i < 26) ? 'A' : '1' - 26))
static int
fsel_drives(OBJECT *m, int drive)
{
	int drv = 0, drvs = 0;
	int d = FSEL_DRVA;
	unsigned long dmap = b_drvmap();

	while (dmap)
	{
		if (dmap & 1)
		{
			m[d].ob_state &= ~OS_CHECKED;

			if (drv == drive)
				m[d].ob_state |= OS_CHECKED;

			sprintf(m[d++].ob_spec.free_string, 32, "  %c:", DRIVELETTER(drv));
			drvs++;
		}
		dmap >>= 1;
		drv++;
	}

	if (drvs & 1)
		m[d-1].ob_width = m[FSEL_DRVBOX].ob_width;

	do {
		m[d].ob_flags |= OF_HIDETREE;
		/* prevent finding those. */
		*(m[d].ob_spec.free_string + 2) = '~';
	}
	while (m[d++].ob_next != FSEL_DRVBOX);

	m[FSEL_DRVBOX].ob_height = ((drvs + 1) / 2) * screen.c_max_h;

	sprintf(m[FSEL_DRV].ob_spec.free_string, 32, " %c:", DRIVELETTER(drive));
	return drvs;
}

static void
fsel_filters(OBJECT *m, char *pattern)
{
	char p[16];
	int d = FSEL_PATA;
	int i = 0;

	if (cfg.Filters[0][0])
	{
		while (i < 23 && cfg.Filters[i][0])
		{
			cfg.Filters[i][15] = 0;
			m[d].ob_state &= ~OS_CHECKED;
			if (stricmp(pattern,cfg.Filters[i]) == 0)
				m[d].ob_state|=OS_CHECKED;
			
			sprintf(m[d++].ob_spec.free_string, 128, "  %s",cfg.Filters[i++]);
		}
	
		do {
			m[d].ob_flags |= OF_HIDETREE;
		}
		while (m[d++].ob_next != FSEL_PATBOX);

		m[FSEL_PATBOX].ob_height = i * screen.c_max_h;
	}
	else
	{
		while (i < 23)
		{
			char *s = m[d].ob_spec.free_string;
			if (strstr(s+2,"**") == s+2)	/* check for place holder entry */
				*(s+3) = 0;		/* Keep the 1 '*' */
			else
			{
				int j = strlen(s);
				while (*(s+ --j) == ' ')
					*(s+j) = 0;
			}
			m[d].ob_state&=~OS_CHECKED;
			if (stricmp(pattern,s) == 0)
				m[d].ob_state|=OS_CHECKED;
			d++, i++;
		}
	}
	strncpy(p,pattern,15);
	p[15] = 0;
	sprintf(m[FSEL_FILTER].ob_spec.free_string, 128, " %s", p);
}

/* HR: a little bit more exact. */
/* Dont need form as long as everything is global 8-} */
static void
fs_updir(enum locks lock, struct fsel_data *fs)
{
	int drv;

	if (*fs->path)
	{
		int s = strlen(fs->path) - 1;

		if (fs->path[s] == '/' || fs->path[s] == '\\')
			s--;

		DIAG((D_fsel, NULL, "fs_updir '%s'", fs->path));	

		while(   s
		      && fs->path[s] != ':'
		      && fs->path[s] != '/'
		      && fs->path[s] != '\\')
			s--;

		if (fs->path[s] == ':')
			fs->path[++s] = *fs->fslash;

		fs->path[++s] = 0;
		DIAG((D_fsel,NULL,"   -->   '%s'", fs->path));	
	}

	if ((drv = get_drv(fs->path)) >= 0)
		strcpy(fs_paths[drv], fs->path);

	refresh_filelist(fsel, fs, 1);
}

static void
fs_closer(struct scroll_info *list)
{
	struct fsel_data *fs = list->data;

	if ( fs->clear_on_folder_change )
		set_file(list->lock, fs, "");

	fs_updir(list->lock, fs);
}

static int
fs_item_action(enum locks lock, struct scroll_info *list, OBJECT *form, int objc)
{
	struct fsel_data *fs = list->data;

	DIAG((D_fsel, NULL, "fs_item_action %lx, fs=%lx", list->cur, fs));
	if (list->cur)
	{
		if ( (list->cur->flag & FLAG_DIR) == 0)
		{
			DIAG((D_fsel, NULL, " --- nodir '%s'", list->cur->text));
			/* file entry action */
			strcpy(fs->file, list->cur->text);
		}
		else
		{			
			/* folder entry action */
			DIAG((D_fsel, NULL, " --- folder '%s'", list->cur->text));
			
			if (strcmp(list->cur->text, "..") == 0)
			{
				if ( fs->clear_on_folder_change )
					set_file(lock, fs, "");

				/* cur on double dot folder line */
				fs_updir(lock, fs);
				return true;
			}

			if (strcmp(list->cur->text, ".") != 0)
			{
				/* cur on common folder line (NON dot) */
				int drv;
				add_slash(fs->path, fs->fslash);
				strcat(fs->path, list->cur->text);
				if ((drv = get_drv(fs->path)) >= 0)
					strcpy(fs_paths[drv], fs->path);
				if ( fs->clear_on_folder_change )
					set_file(lock, fs, "");
				refresh_filelist(fsel, fs, 2);
				return true;
			}
		}
	}
	DIAG((D_fsel, NULL, "fs_item_action: %s%s", fs->path, fs->file));

	if (fs->selected)
		fs->selected(lock, fs, fs->path, fs->file);

	return true;
}

static int
fs_dclick(enum locks lock, struct scroll_info *list, OBJECT *form, int objc)
{
	struct fsel_data *fs = list->data;
	/* since mouse click we should _not_ clear the file field */
	fs->clear_on_folder_change = 0;

	return fs_item_action(lock, list, form, objc);
}

/* converted */
static int
fs_click(enum locks lock, struct scroll_info *list, OBJECT *form, int objc)
{
	struct fsel_data *fs = list->data;

	/* since mouse click we should _not_ clear the file field */
	fs->clear_on_folder_change = 0;

	if (list->cur)
	{
		if ( ! (list->cur->flag & FLAG_DIR))
		{
			set_file(lock, fs, list->cur->text);
		}
		else if (strcmp(list->cur->text, ".") == 0)
		{
			set_file(lock, fs, "");
		}
		else
		{
			fs_item_action(lock, list, form, objc);
		}
	}
	return true;
}

/*
 * FormExit()
 */
static void
fileselector_form_exit(struct xa_client *client,
		       struct xa_window *wind,
		       XA_TREE *wt,
		       struct fmd_result *fr)
{
	enum locks lock = 0;
	OBJECT *obtree = wt->tree;
	struct scroll_info *list = object_get_slist(obtree + FS_LIST);
	struct fsel_data *fs = list->data;
#ifdef FS_FILTER
	TEDINFO *filter = object_get_tedinfo(obtree + FS_FILTER);
#endif
#if 0
#ifdef FS_UPDIR
	struct xa_window *wl = list->wi;
#endif
#endif
	
	/* Ozk:
	 * Dont know exactly what 'which' and 'current' does yet...
	 * ... now I do ;-)
	 */
	wt->which = 0;
	wt->current = fr->obj;
	
	switch (fr->obj)
	{
	/*
	 * 
	 */
	case FS_LIST:
	{
		short obj = fr->obj;

		DIAGS(("filesel_form_exit: Moved the shit out of form_do() to here!"));
		if ( fr->md && ((obtree[obj].ob_type & 0xff) == G_SLIST))
		{
			if (fr->md->clicks > 1)
				dclick_scroll_list(lock, obtree, obj, fr->md);
			else
				click_scroll_list(lock, obtree, obj, fr->md);
		}
		break;
	}
#ifdef FS_UPDIR
	/* Go up a level in the filesystem */
	case FS_UPDIR:
	{
		fs_updir(lock, fs);
		break;
	}
#endif
	/* Accept current selection - do the same as a double click */
	case FS_OK:
	{
		object_deselect(wt->tree + FS_OK);
		display_toolbar(lock, wind, FS_OK);
#ifdef FS_FILTER
		if (strcmp(filter->te_ptext, fs->fs_pattern) != 0)
		{
			/* changed filter */
			strcpy(fs->fs_pattern, filter->te_ptext);
			refresh_filelist(fsel, fs, 3);
		}
		else
#endif
		{
			fs_item_action(lock, list, wt->tree, FS_LIST);
		}
		break;
	}
	/* Cancel this selector */
	case FS_CANCEL:
	{
		object_deselect(wt->tree + FS_CANCEL);
		display_toolbar(lock, wind, FS_CANCEL);
		if (fs->canceled)
	 		fs->canceled(lock, fs, fs->path, "");
		else
			close_fileselector(lock, fs);
		break;
	}
	}
}


static int
find_drive(int a, struct fsel_data *fs)
{
	XA_TREE *wt = get_widget(fs->wind, XAW_MENU)->stuff;
	OBJECT *m = wt->tree;
	int d = FSEL_DRVA;

	do {
		int x = tolower(*(m[d].ob_spec.free_string + 2));
		if (x == '~')
			break;
		if (x == a)
			return d;
	} while (m[d++].ob_next != FSEL_DRVA - 1);

	ping();

	return -1;
}

static void
fs_change(enum locks lock, struct fsel_data *fs, OBJECT *m, int p, int title, int d, char *t)
{
	XA_WIDGET *widg = get_widget(fs->wind, XAW_MENU);
	int bx = d - 1;
	
	do
		m[d].ob_state &= ~OS_CHECKED;
	while (m[d++].ob_next != bx);

	m[p].ob_state |= OS_CHECKED;
	sprintf(m[title].ob_spec.free_string, 128, " %s", m[p].ob_spec.free_string + 2);
	widg->start = 0;
	m[title].ob_state &= ~OS_SELECTED;
	display_widget(lock, fs->wind, widg);
	strcpy(t, m[p].ob_spec.free_string + 2);
}

/*
 * FormKeyInput()
 */
static bool
fs_key_form_do(enum locks lock,
	       struct xa_client *client,
	       struct xa_window *wind,
	       XA_TREE *wt,
	       const struct rawkey *key)
{
	unsigned short keycode = key->aes;
	unsigned short nkcode = key->norm;
	struct scroll_info *list = object_get_slist(((XA_TREE *)get_widget(wind, XAW_TOOLBAR)->stuff)->tree + FS_LIST);
	struct fsel_data *fs = list->data;
	SCROLL_ENTRY *old_entry = list->cur;

	/* HR 310501: ctrl|alt + letter :: select drive */
	if ((key->raw.conin.state & (K_CTRL|K_ALT)) != 0)
	{
		ushort nk;

		if (nkcode == 0)
			nkcode = nkc_tconv(key->raw.bcon);

		nk = tolower(nkcode & 0xff);
		if (   (nk >= 'a' && nk <= 'z')
		    || (nk >= '0' && nk <= '9'))
		{
			int drive_object_index = find_drive(nk, fs);
			if (drive_object_index >= FSEL_DRVA)
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
						   MN_SELECTED, 0, 0, FSEL_DRV,
						   drive_object_index, 0, 0, 0);
		}

		/* ctrl + backspace :: fs_updir() */
		if ((nkcode & NKF_CTRL) && nk == NK_BS)
		{
			if (fs->clear_on_folder_change)
				set_file(lock, fs, "");

			fs_updir(lock, fs);
		}
		if ((nkcode & NKF_CTRL) && nk == '*')
		{
			/* change the filter to '*'
			 * - this should always be the FSEL_PATA entry IIRC
			 */
			fs_change(lock, fs, fs->menu->tree,
					FSEL_PATA, FSEL_FILTER, FSEL_PATA, fs->fs_pattern);
			/* apply the change to the filelist */
			refresh_filelist(fsel, fs, 6);
		}
	}
	else
	/*  If anything in the list and it is a cursor key */
	if (list->n && scrl_cursor(list, keycode) != -1)
	{
		if (!(list->cur->flag & FLAG_DIR))
		{
			if (old_entry != list->cur)
				fs->clear_on_folder_change = 1;

			set_file(lock, fs, list->cur->text);
		}
	}
	else
	{
		char old[NAME_MAX+2];
		strcpy(old, fs->file);
		/* HR 290501: if !discontinue */
		if (Key_form_do(lock, client, wind, wt, key))
		{
			/* something typed in there? */
			if (strcmp(old, fs->file) != 0)
			{
				fs_prompt(list);
			}
		}
	}
	return true;
}

/* HR: make a start */
/* dest_pid, msg, source_pid, mp3, mp4,  ....    */
static void
fs_msg_handler(
	struct xa_window *wind,
	struct xa_client *to,
	short amq, short qmf,
	short *msg)
{
	enum locks lock = 0;
	struct scroll_info *list = object_get_slist(((XA_TREE *)get_widget(wind, XAW_TOOLBAR)->stuff)->tree + FS_LIST);
	struct fsel_data *fs = list->data;

	switch (msg[0])
	{
	case MN_SELECTED:
	{
		if (msg[3] == FSEL_FILTER)
			fs_change(lock, fs, fs->menu->tree, msg[4], FSEL_FILTER, FSEL_PATA, fs->fs_pattern);
		else if (msg[3] == FSEL_DRV)
		{
			int drv;
			fs_change(lock, fs, fs->menu->tree, msg[4], FSEL_DRV, FSEL_DRVA, fs->path);
			inq_xfs(fs, fs->path, fs->fslash);
			add_slash(fs->path, fs->fslash);
			drv = get_drv(fs->path);
			if (fs_paths[drv][0])
				strcpy(fs->path, fs_paths[drv]);
			else
				strcpy(fs_paths[drv], fs->path);
			/* remove the name from the edit field on drive change */
			if ( fs->clear_on_folder_change )
				set_file(lock, fs, "");
		}
		refresh_filelist(lock, fs, 4);
		break;
	}
	case WM_MOVED:
	{
		msg[6] = fs->wind->r.w, msg[7] = fs->wind->r.h;
		/* fall through */
	}
	default:
	{
		do_formwind_msg(wind, to, amq, qmf, msg);
	}
	}
}

static int
fs_destructor(enum locks lock, struct xa_window *wind)
{
	DIAG((D_fsel,NULL,"fsel destructed"));
	return true;
}

static bool
open_fileselector1(enum locks lock, struct xa_client *client, struct fsel_data *fs,
		   const char *path, const char *file, const char *title,
		   fsel_handler *s, fsel_handler *c)
{
	int dh;			/* HR variable height for fileselector :-) */
	bool nolist;
	XA_WIND_ATTR kind;
	OBJECT *form = NULL, *menu = NULL;
	struct xa_window *dialog_window = NULL;
	XA_TREE *wt;
	char *pat,*pbt;
	RECT remember = {0,0,0,0};

	DIAG((D_fsel,NULL,"open_fileselector for %s on '%s', fn '%s', '%s', %lx,%lx)",
			c_owner(client), path, file, title, s, c));

	if (fs)
	{
		bzero(fs, sizeof(*fs));

		fs->owner = client;
		
		*fs->fslash = '\\';
		
		form = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, FILE_SELECT), 0);
		if (!form)
			goto memerr;
		fs->form = new_widget_tree(client, form);
		if (!fs->form)
			goto memerr;
		fs->form->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;

		menu = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, FSEL_MENU), 0);
		if (!menu)
			goto memerr;
		fs->menu = new_widget_tree(client, menu);
		if (!fs->menu)
			goto memerr;
		fs->menu->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
				
		fs->selected = s;
		fs->canceled = c;
		if (file && *file)
		{
			strncpy(fs->file, file, sizeof(fs->file) - 1);
			*(fs->file + sizeof(fs->file) - 1) = 0;
		}

		object_get_spec(form + FS_FILE)->tedinfo->te_ptext = fs->file;
		form[FS_ICONS].ob_flags |= OF_HIDETREE;
		
		dh = root_window->wa.h - 7*screen.c_max_h - form->ob_height;
		form->ob_height += dh;
		form[FS_LIST ].ob_height += dh;
		form[FS_UNDER].ob_y += dh;

		/* Work out sizing */
		if (!remember.w)
		{
			form_center(form, 2*ICON_H);
			remember =
			calc_window(lock, client, WC_BORDER,
				    XaMENU|NAME,
				    C.Aes->options.thinframe,
				    C.Aes->options.thinwork,
				    *(RECT*)&form->ob_x);
		}

		strcpy(fs->path, path);

		/* Strip out the pattern description */

		fs->fs_pattern[0] = '*';
		fs->fs_pattern[1] = '\0';
		pat = strrchr(fs->path, '\\');
		pbt = strrchr(fs->path, '/');
		if (!pat) pat = pbt;
		if (pat)
		{
			strcpy(fs->fs_pattern, pat + 1);
			*(pat + 1) = 0;
			if (strcmp(fs->fs_pattern, "*.*") == 0)
				*(fs->fs_pattern + 1) = 0;
		}

		{
			int drv = get_drv(fs->path);
			if (drv >= 0)
				strcpy(fs_paths[drv], fs->path);
		}

		kind = (XaMENU|NAME|TOOLBAR);
		if (C.update_lock == client->p ||
		    C.mouse_lock  == client->p)
		{
			nolist = true;
			kind |= STORE_BACK;
		}
		else
		{
			kind |= hide_move(&(client->options));
			nolist = false;
		}

		/* Create the window */
		dialog_window = create_window(  lock,
						do_winmesag,
						fs_msg_handler,
						client,
						nolist,
						kind,
						created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						remember,
						NULL,
						NULL); //&remember);
		
		if (!dialog_window)
			goto memerr;

		/* Set the window title */
		/* XXX - pointer into user space, correct here? */
		/*   ozk: no, absolutely not right here! */
		set_window_title(dialog_window, title, true);

		set_menu_widget(dialog_window, client, fs->menu);

		fs->drives = fsel_drives(fs->menu->tree,
					*(fs->path+1) == ':' ? tolower(*fs->path) - 'a' : d_getdrv());
		fsel_filters(fs->menu->tree, fs->fs_pattern);

		fs->clear_on_folder_change = 0;
		strcpy(fs->file, file); /* fill in the file edit field */

		wt = set_toolbar_widget(lock, dialog_window, client, form, FS_FILE, WIDG_NOTEXT, NULL);
		/* This can be of use for drawing. (keep off border & outline :-) */
		wt->zen = true;
		wt->exit_form = fileselector_form_exit;

		/* HR: We need to do some specific things with the key's,
		 *     so we supply our own handler,
		 */
		dialog_window->keypress = fs_key_form_do; //fs_key_handler;

		/* HR: set a scroll list object */
		set_slist_object(lock,
				 wt,
				 dialog_window,
				 FS_LIST,
				 SIF_SELECTABLE,
				 fs_closer, NULL,
				 fs_dclick, fs_click,
				 NULL, NULL, NULL, free_scrollist,
				 fs->path, NULL, fs, 30);

		/* HR: after set_menu_widget (fs_destructor must cover what is in menu_destructor())
		 *     Set the window destructor
		 */
		dialog_window->destructor = fs_destructor;

		strcpy(fs->filter, fs->fs_pattern);
		fs->wind = dialog_window;
		open_window(lock, dialog_window, dialog_window->r);

		/* HR: after set_slist_object() & opwn_window */
		refresh_filelist(lock, fs, 5);

		DIAG((D_fsel,NULL,"done."));

		return true;
	}
	else
	{
memerr:
		if (fs)
		{
			if (fs->form)
			{
				remove_wt(fs->form, true);
				fs->form = NULL;
			}
			else if (form)
				free_object_tree(client, form);
			if (fs->menu)
			{
				remove_wt(fs->menu, true);
				fs->menu = NULL;
			}
			else if (menu)
				free_object_tree(client, form);
		}
		return false;
	}
}

void
close_fileselector(enum locks lock, struct fsel_data *fs)
{
	close_window(lock, fs->wind);
	delayed_delete_window(lock, fs->wind);
	fs->wind = NULL;
	fs->menu = NULL;
	fs->form = NULL;
	fs->selected = NULL;
	fs->canceled = NULL;
}

static void
handle_fsel(enum locks lock, struct fsel_data *fs, const char *path, const char *file)
{
	DIAG((D_fsel, NULL, "fsel OK: path=%s, file=%s", path, file));

	close_fileselector(lock, fs);
	fs->owner->usr_evnt = 1;
	fs->ok = 1;
	fs->done = 1;
}

static void
cancel_fsel(enum locks lock, struct fsel_data *fs, const char *path, const char *file)
{
	DIAG((D_fsel, NULL, "fsel CANCEL: path=%s, file=%s", path, file));

	close_fileselector(lock, fs);
	fs->owner->usr_evnt = 1;
	fs->ok = 0;
	fs->done = 1;
}

static int locked = 0;
void
open_fileselector(enum locks lock, struct xa_client *client, struct fsel_data *fs,
		  const char *path, const char *file, const char *title,
		  fsel_handler *s, fsel_handler *c)
{
	if (!locked)
	{
		open_fileselector1(lock, client, fs, path, file, title, s, c);
	}
}

/*
 * File selector interface routines
 */
static void
do_fsel_exinput(enum locks lock, struct xa_client *client, AESPB *pb, const char *text)
{
	char *path = (char *)(pb->addrin[0]);
	char *file = (char *)(pb->addrin[1]);
	struct fsel_data *fs;

	pb->intout[0] = 0;

	fs = kmalloc(sizeof(*fs));
	
	if (fs)
	{
		DIAG((D_fsel, NULL, "fsel_(ex)input: title=%s, path=%s, file=%s, fs=%lx",
			text, path, file, fs));
			
		if (open_fileselector1( lock|fsel,
					client,
					fs,
					path,
					file,
					text,
					handle_fsel,
					cancel_fsel))
		{
			client->status |= CS_FSEL_INPUT;
			Block(client, 21);
			client->status &= ~CS_FSEL_INPUT;
			pb->intout[0] = 1;
			pb->intout[1] = fs->ok;
			strcpy(path, fs->path);
			strcpy(file, fs->file);
		}
		kfree(fs);
	}
}

unsigned long
XA_fsel_input(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,2,2)

	do_fsel_exinput(lock, client, pb, "");

	return XAC_DONE;
}

unsigned long
XA_fsel_exinput(enum locks lock, struct xa_client *client, AESPB *pb)
{
	const char *t = (const char *)(pb->addrin[2]);

	CONTROL(0,2,3)

	if (pb->control[3] <= 2 || t == NULL)
		t = "";

	do_fsel_exinput(lock, client, pb, t);

	return XAC_DONE;
}
