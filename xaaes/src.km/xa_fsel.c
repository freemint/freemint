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
 * FILE SELECTOR IMPLEMENTATION
 *
 * This give a non-modal windowed file selector for internal
 * and exported use, with slightly extended pattern matching
 * from the basic GEM spec.
 */

#define PROFILING 0
#define KERNELSTRCMP 0

	/*
	 * has gnu-c builtin strcmp or not?
	 */
#define OWNSTRCMP 1
	/* experentimental */
#define BEAUTY_ROOT_NAME	1

#include "xa_fsel.h"
#include "xa_global.h"

#include "xaaes.h"

#include "c_keybd.h"
#include "c_window.h"
#include "taskman.h"
#include "about.h"
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
#include "keycodes.h"

#include "xa_form.h"
#include "xa_graf.h"
#include "xa_wind.h"
#include "xa_rsrc.h"

#include "mint/pathconf.h"
#include "mint/stat.h"

#define FLAG_FILLED 0x00000001
#define FLAG_DIR	0x00000002
#define FLAG_EXE	0x00000004
#define FLAG_LINK 0x00000008
#define FLAG_SDIR 0x00000010

#define FSLIDX_NAME 0
#define FSLIDX_SIZE 1
#define FSLIDX_TIME 2
#define FSLIDX_DATE 3
#define FSLIDX_FLAG 4
	/*#define FSLIDX_RIGHT	5*/

	/* fcase */
#define FS_FSNOCASE 1 	/* fs is uppercase */
#define FS_PATNOCASE	2 /* pattern is caseinsensitive */

	/* needed for tab-calculation... */
#define MINWIDTH	14
#define MAXWIDTH	60

static char *months[] =
{
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
};
static char *faccess[] =
{
	"---",
	"--x",
	"-w-",
	"-wx",
	"r--",
	"r-x",
	"rw-",
	"rwx",
};

static struct xa_wtxt_inf fs_norm_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags  wr_mode   efx  fg       bg       banner x_3dact y_3dact texture */
 {	-1,  -1,	0,	MD_TRANS, 0,   G_BLACK, G_WHITE, G_WHITE, 0,			0,		 NULL}, /* Normal */
 {	-1,  -1,	0,	MD_TRANS, 0,   G_WHITE, G_BLACK, G_WHITE, 0,			0,		 NULL},/* Selected */
 {	-1,  -1,	0,	MD_TRANS, 0,   G_BLACK, G_WHITE, G_WHITE, 0,			0,		 NULL}, /* Highlighted */
};

static struct xa_wtxt_inf exe_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags  wr_mode   efx  fg        bg       banner x_3dact y_3dact texture */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_RED, 	 G_WHITE, G_WHITE, 0, 		 0, 		NULL},	/* Normal */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_YELLOW, G_WHITE, G_WHITE, 0, 		 0, 		NULL},/* Selected */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_BLACK,  G_WHITE, G_WHITE, 0, 		 0, 		NULL},	/* Highlighted */
};

static struct xa_wtxt_inf dexe_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags  wr_mode   efx  fg        bg       banner x_3dact y_3dact texture */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_RED,	 G_WHITE, G_WHITE, 0,			0,		 NULL },	/* Normal */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_LRED, 	 G_WHITE, G_WHITE, 0,			0,		 NULL },	/* Selected */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_BLACK,  G_WHITE, G_WHITE, 0,			0,		 NULL },	/* Highlighted */
};

static struct xa_wtxt_inf dir_txt =
{
 WTXT_NOCLIP,
/* id  pnts  flags  wr_mode   efx  fg       bg        banner x_3dact y_3dact texture */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_BLUE,  G_WHITE,  G_WHITE, 0,			0,		 NULL },/* Normal */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_CYAN,	G_WHITE,  G_WHITE, 0,			0,		 NULL },	/* Selected */
 {	-1,  -1, 0, 	MD_TRANS, 0,   G_BLACK, G_WHITE,  G_WHITE, 0,			0,		 NULL },	/* Highlighted */
};


enum{
	DOWN  = 0,
	UP
};

struct fs_data fs_data = {0};

static void
refresh_filelist(int lock, struct fsel_data *fs, SCROLL_ENTRY *dir_ent);

static void
add_pattern(const char *pattern)
{
	int i;

	if (pattern && *pattern)
	{
		for (i = 0; ; i++)
		{
			if (fs_data.fs_patterns[i][0])
			{
				if (!strcmp(pattern, fs_data.fs_patterns[i]))
				{
					break;
				}
				if( fs_data.prover && i == FS_NPATTERNS )
				{
					if( fs_data.prover == FS_NPATTERNS )
							fs_data.prover = fs_data.provermin;	/* skip patterns from config */
					i = fs_data.prover;
					fs_data.fs_patterns[i][0] = 0;
					fs_data.prover++;
				}
			}
			else
			{
				strncpy(fs_data.fs_patterns[i], pattern, FS_PATLEN-1);
				fs_data.fs_patterns[i][FS_PATLEN-1] = '\0';
				break;
			}
		}
	}
}

#define SKEWED_TEXT_SCRAMBLED 0
#if SKEWED_TEXT_SCRAMBLED
#undef ITALIC
#define ITALIC	(UNDERLINED|SHADED)
#endif

void
init_fsel(void)
{
	int i;

	add_pattern("*");

	for (i = 0; i < FS_NPATTERNS && cfg.Filters[i][0]; i++)
	{
			add_pattern(cfg.Filters[i]);
	}
	/* todo: free Filters */

	if( i < FS_NPATTERNS -1 )
		fs_data.prover = fs_data.provermin = i + 1;	/* + "*" */
	if (screen.planes < 4)
	{
		exe_txt = dexe_txt = dir_txt = fs_norm_txt;
	}

#if SKEWED_TEXT_SCRAMBLED
	else{
		dir_txt.normal.fg = G_BLACK;
		dir_txt.selected.fg = G_WHITE;
	}
#endif
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
	/* no suffix	*/
	return 0L;
}
static void
strins(char *d, const char *s, long here)
{
	long slen = strlen(s);
	long dlen = strlen(d);
	char t[512];

	if (here > dlen)
		here = dlen;

	strncpy(t, d + here, sizeof(t));
	strncpy(d + here, s, slen );
	strcpy(d + here + slen, t);
}

static struct scroll_entry *
fs_cwd(struct scroll_info *list, char *cwd, short incsel)
{
	struct fsel_data *fs = list->data;
	struct scroll_entry *dir_ent = NULL;
	struct sesetget_params p;

	if (cwd)
		strcpy(cwd, fs->root);

	if (fs->selected_dir)
	{
		struct scroll_entry *this = fs->selected_dir;

		switch (incsel)
		{
			/*
			 * Include fs->selected_dir only when not in treeview
			 */
			case 0:
			{
				if (this && !fs->treeview)
					this = this->up;
				break;
			}
			/*
			 *
			 */
			case 1:
			{
				if (this)
				{
					short xstate;
					list->get(list, this, SEGET_XSTATE, &xstate);
					if (!(xstate & OS_OPENED))
						this = this->up;
				}
				break;
			}
			case 2:
			default:;
		}

		dir_ent = this;

		if (cwd)
		{
			long len = strlen(cwd);
			p.idx = -1;
			while (this)
			{
				if (list->get(list, this, SEGET_TEXTPTR, &p))
				{
					strins(cwd, fs->fslash, len);
					strins(cwd, p.ret.ptr, len);
				}
				this = this->up;
			}
		}
	}
	return dir_ent;
}

static void
set_dir(struct scroll_info *list)
{
	struct fsel_data *fs = list->data;
	fs_cwd(list, fs->path, 1);
	display_widget(list->lock, list->wi, get_widget(list->wi, XAW_TITLE), list->pw ? list->pw->rect_list.start : NULL);
}

static struct scroll_entry *
get_selorboxed(struct scroll_info *list)
{
	struct sesetget_params p;

	p.idx = 0;
	p.arg.state.method = ANYBITS;
	p.arg.state.bits = OS_SELECTED | OS_BOXED;
	p.arg.state.mask = OS_SELECTED | OS_BOXED;
	p.level.flags = 0;
	p.level.maxlevel = -1;
	p.level.curlevel = 0;

	list->get(list, NULL, SEGET_ENTRYBYSTATE, &p);
	return p.e;
}
/*
 * Modify state on multiple scroll-list entries
 */
static void
modify_mult_state(struct scroll_info *list, void *parent, short bits, short mask)
{
	struct sesetget_params p;

	p.idx = 0;
	p.arg.state.method = 0;
	p.arg.state.bits = bits;
	p.arg.state.mask = mask;
	p.level.flags = ENT_ISROOT;
	p.level.maxlevel = -1;
	p.level.curlevel = 0;
	list->set(list, parent, SESET_M_STATE, (long)&p, NORMREDRAW);
}

static void set_tabs( struct scroll_info *list, struct fsel_data *fs, short wd)
{
	struct seset_tab tab;
	int i;
	short x;

	tab.index = FSLIDX_NAME;
	tab.flags = 0;
	tab.r = (GRECT){0,0,0,0};
	if (!fs->treeview && !fs->rtbuild)
		tab.r.g_h = -MINWIDTH;

	list->set(list, NULL, SESET_TAB, (long)&tab, false);

	tab.r.g_w = 0;
	tab.flags |= SETAB_RJUST;
	x = list->tabs[FSLIDX_NAME].r.g_w;
	for( i = FSLIDX_SIZE; i <= FSLIDX_FLAG; i++ )
	{
		if( wd )
		{
			if( wd < 1000 )
			{
				long w;
				w = (long)list->tabs[i].r.g_w * wd;
				list->tabs[i].r.g_w = w / 1000L;
				list->tabs[i].r.g_x = x;
				x += list->tabs[i].r.g_w;
			}
		}
		else
		{
			tab.index = i;
			list->set(list, NULL, SESET_TAB, (long)&tab, false);
		}
	}
}

static struct scroll_entry *
get_selected(struct scroll_info *list)
{
	struct sesetget_params p;

	p.idx = 0;
	p.arg.state.method = EXACTBITS;
	p.arg.state.bits = OS_SELECTED;
	p.arg.state.mask = OS_SELECTED;
	p.level.flags = 0;
	p.level.maxlevel = -1;
	p.level.curlevel = 0;

	list->get(list, NULL, SEGET_ENTRYBYSTATE, &p);
	return p.e;
}

static void
set_file(struct fsel_data *fs, char *fn, bool mark);

static int list_get_match(SCROLL_INFO *list, struct sesetget_params *seget, struct sesetget_params *p, char *pat, int nxt)
{
	int m;
	int nextent = nxt == -1 ? SEGET_PREVENT : SEGET_NEXTENT;
	seget->level.flags = ENT_VISIBLE;
	seget->level.maxlevel = -1;
	seget->level.curlevel = 0;
	p->idx = -1;

	if( nxt )
		list->get(list, seget->e, nextent, seget);
	while (seget->e)
	{
		list->get(list, seget->e, SEGET_TEXTPTR, p);
		if (p->ret.ptr)
		{
			if( (m=match_pattern(p->ret.ptr, pat, true)))
				return m;
		}
		list->get(list, seget->e, nextent, seget);
	}
	return 0;
}

static void
fs_prompt(SCROLL_INFO *list, char *file, bool typing)
{
	struct fsel_data *fs = list->data;
	SCROLL_ENTRY *parent, *s;
	struct sesetget_params p;

	if (!(parent = fs_cwd(list, NULL, 1)))
	{
		parent = NULL;
		s = list->start;
	}
	else
		s = parent->down;

	if( *file && *(file + strlen(file) - 1) != '*' && strchr( file,  fs->fslash[0]) )
	{
		if( *file == fs->fslash[0] || (*(file+1) == ':' && *(file+2) == fs->fslash[0]) )
			strcpy( fs->root, file );
		else
		{
			int l;
			strcpy( fs->root, fs->path );
			l = strlen(fs->root);
			strncpy( fs->root+l, file, sizeof(fs->root)-l-1 );
			fs->file[0] = 0;
		}
		set_file( fs, "", true );
		set_dir(list);
		strcat( fs->root, "*" );
		refresh_filelist(LOCK_FSEL, fs, NULL);
		list->redraw(list, NULL);
		return;
	}
	/* Not if filename empty or list empty */
	if (*file && s && !(fs->rtflags & (FS_CREATE_FOLDER|FS_RENAME_FILE) ))
	{
		int match, l;
		struct sesetget_params seget;
		seget.e = s;
		match = list_get_match( list, &seget, &p, file, 0 );
		s = seget.e;

		l = strlen(file);
		if( !strchr( "|*!?[", *file ) && *(file + l - 1) == '*' )
		{
			*(file + l - 1) = 0;
			l = 1;
		}
		else
			l = 0;
		if (s)
		{
			long uf;

			uf = s->usr_flags;

			list->set(list, s, SESET_CURRENT, 0, NOREDRAW);
			list->set(list, NULL, SESET_MULTISEL, 0, NOREDRAW);

			list->set(list, NULL, SESET_SELECTED, 0, NORMREDRAW);

			if (match == 1)
			{
				if ((uf & FLAG_DIR))
				{
					fs->selected_dir = s->up;
					fs->selected_file = NULL;
					if( l )
						set_file( fs, p.ret.ptr, 0 );
				}
				else
				{
					fs->selected_dir = s->up;
					fs->selected_file = s;
				}
				list->set(list, s, SESET_SELECTED, 0, NORMREDRAW);
			}
			else if (match == 2)
			{
				if ((uf & FLAG_DIR))
				{
					fs->selected_dir = s;
					fs->selected_file = NULL;
				}
				else
				{
					fs->selected_dir = s->up;
					fs->selected_file = s;
				}

				list->set(list, s, SESET_SELECTED, 0, NORMREDRAW);
			}
			list->vis(list, s, NORMREDRAW);
		}
		else
		{
			fs->selected_file = NULL;
			if (parent)
			{
				/*
				 * Clear any boxed or selected settings in this level
				 */
				p.arg.state.mask = ~(OS_BOXED|OS_SELECTED);
				p.arg.state.bits = 0;
				p.level.flags = ENT_ISROOT;
				p.level.maxlevel = -1;
				p.level.curlevel = 0;
				list->set(list, parent, SESET_M_STATE, (long)&p, NORMREDRAW);

				list->set(list, parent, SESET_CURRENT, 0, NOREDRAW);
				fs->selected_dir = parent;
				list->set(list, parent, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NORMREDRAW);
				list->vis(list, parent, NOREDRAW);
			}
			else
			{
				if ((s = get_selorboxed(list)))
				{
					fs->selected_dir = s->up;
					list->set(list, s, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16), NORMREDRAW);
				}
				else if (fs->kbdnav && !typing)
				{
					s = list->start;
					list->set(list, s, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NORMREDRAW);
					list->set(list, s, SESET_CURRENT, 0, NOREDRAW);
				}
				else
					list->set(list, NULL, SESET_CURRENT, 0, NOREDRAW);
			}
		}
	}
	else
	{
		if (!*file)
			fs->tfile = false;

		if (!fs->tfile)
		{
			fs->selected_file = NULL;
			if (parent)
			{
				list->set(list, parent, SESET_CURRENT, 0, NOREDRAW);
				fs->selected_dir = parent;
				list->set(list, NULL, SESET_STATE, ((long)OS_BOXED << 16), NORMREDRAW);
				list->set(list, parent, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NORMREDRAW);
				list->vis(list, parent, NORMREDRAW);
			}
			else
			{
				if ((s = get_selorboxed(list)))
				{
					fs->selected_dir = s->up;
					list->set(list, s, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16), NORMREDRAW);
				}
				else if (fs->kbdnav && !typing)
				{
					s = list->start;
					list->set(list, s, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NORMREDRAW);
					list->set(list, s, SESET_CURRENT, 0, NOREDRAW);
				}
				else
				{
					list->set(list, NULL, SESET_CURRENT, 0, NOREDRAW);
				}
			}
		}
	}
	if( list->flags & SIF_DIRTY )
	{
		list->redraw(list, NULL);
	}
	return;
} /*/fs_prompt*/


static short
sort_by_date(struct xattr *x1, struct xattr *x2)
{
	short tmp;
	/*
	 * Sorting by date.
	 * return -1 if x1 is greater (newer, after) than x2
	 * return  1 if x1 is less (older, before) than x2
	 * return  0 if x1 and x2 has exact same date.
	 */
	if ((tmp = ((x1->mdate >> 9) & 127) - ((x2->mdate >> 9) & 127) )) /* Year */
		return (tmp > 0) ? 1 : -1;
	if ((tmp = ((x1->mdate >> 5) & 15) - ((x2->mdate >> 5) & 15) )) 	/* Month */
		return tmp > 0 ? 1 : -1;
	if ((tmp = (x1->mdate & 31) - (x2->mdate & 31)))			/* day */
		return tmp > 0 ? 1 : -1;
	if ((tmp = ((x1->mtime >> 11) & 31) - ((x2->mtime >> 11) & 31)))	/* hour */
		return tmp > 0 ? 1 : -1;
	if ((tmp = ((x1->mtime >> 5) & 63) - ((x2->mtime >> 5) & 63)))		/* min */
		return tmp > 0 ? 1 : -1;
	if ((tmp = (x1->mtime & 63) - (x2->mtime & 63)))			/* sec */
		return tmp > 0 ? 1 : -1;

	return 0;
}

static short
sort_by_ext(char *n1, char *n2)
{
	short ret = 0;

	/*
	 * return -1 if n1 is greater (newer, after) than n2
	 * return  1 if n1 is less (older, before) than n2
	 * return  0 if n1 and n2 is equal.
	 */
	n1 = getsuf(n1);
	n2 = getsuf(n2);

	if (n1 && n2)
		ret = stricmp(n1, n2);
	else if (!n1 && !n2)
		ret = 0;
	else if (!n1)
		ret = -1;
	else
		ret = 1;

	return ret;
}

#if INCLUDE_CINSENS
static short
sort_by_name(char *n1, char *n2)
{
	return cfg.casesort ? strcmp(n1, n2) : stricmp(n1, n2);
}
#else
static short
sort_by_name(char *n1, char *n2)
{
	short r = strcmp(n1, n2);
	if( fs_data.SortDir == UP )
		r = -r;

	return r;
}
#endif

/*
 * This is called by the slist functions
 */
static bool
sortbydate(struct scroll_info *list, struct scroll_entry *e1, struct scroll_entry *e2)
{
	long uf1, uf2;
	short tmp = 0;
	struct xattr *x1, *x2;
	struct sesetget_params p1, p2;

	list->get(list, e1, SEGET_USRFLAGS, &uf1);
	list->get(list, e2, SEGET_USRFLAGS, &uf2);

	p1.idx = p2.idx = -1;
	list->get(list, e1, SEGET_TEXTPTR, &p1);
	list->get(list, e2, SEGET_TEXTPTR, &p2);

	if ((uf1 & FLAG_SDIR) || (uf2 & FLAG_SDIR))
	{
		if ((uf1 & FLAG_SDIR) && (uf2 & FLAG_SDIR))
		{
			return sort_by_name(p1.ret.ptr, p2.ret.ptr) > 0 ? true : false;
		}
		else if (!(uf2 & FLAG_SDIR))
			return false;
		else
			return true;
	}

	uf1 &= FLAG_DIR;
	uf2 &= FLAG_DIR;
	if (uf1 < uf2)
	{
		/* folders in front */
		return true;
	}

	if (uf1 == uf2)
	{
		list->get(list, e1, SEGET_USRDATA, &x1);
		list->get(list, e2, SEGET_USRDATA, &x2);

		if ((tmp = sort_by_date(x1, x2)))
		{
			if( fs_data.SortDir == UP )
				tmp = -tmp;
			return tmp > 0 ? false : true;
		}

		if ((tmp = sort_by_ext(p1.ret.ptr, p2.ret.ptr)))
			return tmp > 0 ? true : false;

		if ((tmp = sort_by_name(p1.ret.ptr, p2.ret.ptr)))
			return tmp > 0 ? true : false;

		/*
		 * If same date, suffix and name, we let size decide...
		 */
		if (!S_ISDIR(x1->mode) && !S_ISDIR(x2->mode))
		{
			if (x1->size > x2->size)
				return true;
		}
	}
	return false;
}


/*
 * int bstrcmp( char *t1, char *t2 );
 * bstrcmp
 * return true if t1 > t2 else false
 *
 */
static int bstrcmp( char *t1, char *t2 )
{
	for( ; *t1 && *t1 == *t2; t1++, t2++);

	if( *t1 > *t2 ){
		return true;
	}
	return false;
}
#if 0
/*
 * sstrcmp
 * return 1 if t1 > t2 -1 if t1 < t2 else 0
 *
 */
static int sstrcmp( char *t1, char *t2 )
{
	for( ; *t1 && *t1 == *t2; t1++, t2++);

	if( *t1 > *t2 )
		return 1;
	if( *t1 < *t2 )
		return -1;
	return 0;
}
#endif

#ifndef SHRT_MAX
#  define SHRT_MAX	32767
#endif
/*
 * like strncmp
 *
 * if t3 != 0 chars in t3 are considered equal
 *
 * if n = 0 any length (SHRT_MAX)
 *
 */
static int strnccmp( char *t1, char *t2, int n, char *t3 )
{

	int c = 0;
	if( n == 0 )
		n = SHRT_MAX;
	if( !t3 || !*t3 )
		for( ; *t1 && *t2 && *t1 == *t2 && c < n; c++, t1++, t2++);
	else
		for( ; *t1 && *t2 && c < n; c++, t1++, t2++)
			if( !( *t1 == *t2 ) || ( strchr( t3, *t1 ) && strchr( t3, *t2 ) ) )
				break;

	if( c == n )
		return 0;

	if( !*t1 )
		return -1;
	if( !*t2 )
		return 1;

	if( !t3 || !*t3 ){
		if( *t1 > *t2 )
			return 1;
		if( *t1 < *t2 )
			return -1;
	}
	else{
		if( *t1 > *t2 && !((strchr( t3, *t1 ) && strchr( t3, *t2 ) ) ) )
			return 1;
		if( *t1 < *t2 && !((strchr( t3, *t1 ) && strchr( t3, *t2 ) ) ) )
			return -1;
	}
	return 0;
}


bool
sortbyname(struct scroll_info *list, struct scroll_entry *e1, struct scroll_entry *e2)
{
	long uf1, uf2, d1, d2;
#if 0
	short tmp = 0;
	struct xattr *x1, *x2;
#endif
	struct sesetget_params p1, p2;

	PRDEF(sortbyname,get);
	PRDEF(sortbyname,bstrcmp);

	/*list->get(list, e1, SEGET_USRFLAGS, &uf1);
	list->get(list, e2, SEGET_USRFLAGS, &uf2);
	*/
	uf1 = e1->usr_flags;
	uf2 = e2->usr_flags;


	p1.idx = p2.idx = -1; /* get FIRST */

	d1=(uf1 & FLAG_SDIR);
	d2=(uf2 & FLAG_SDIR);
	if ( d1 || d2 )
	{
		if (d1 && d2)
		{

			if( !e1 || !e1->content || !e2 || !e2->content ){
				PROFILE(( "sort_by_name(SDIR): text not first %lx/%lx/%lx/%lx", e1, e1?e1->content:0, e2, e2?e2->content:0 ));
			}
			else{
				if( !(e1->content->type == SECONTENT_TEXT && e2->content->type == SECONTENT_TEXT) ){
					if( e1->content->type == SECONTENT_TEXT )
						PROFILE(( "sort_by_name(SDIR): e2 != TEXT" ));
					else
						PROFILE(( "sort_by_name(SDIR): e1 != TEXT" ));
				}
				else{
					char *t1;
					char *t2;
					t1 = e1->content->c.text.text;
					t2 = e2->content->c.text.text;
					return sort_by_name( t1, t2 ) > 0 ? true : false;
				}
			}
			PROFRECs(list->,get,(list, e1, SEGET_TEXTPTR, &p1));
			PROFRECs(list->,get,(list, e2, SEGET_TEXTPTR, &p2));

			return sort_by_name(p1.ret.ptr, p2.ret.ptr) > 0 ? true : false;
		}
		else if (d1)
			return false;
		else
			return true;
	}

	uf1 &= FLAG_DIR;
	uf2 &= FLAG_DIR;
	if (uf1 < uf2)
	{
		/* folders in front */
		return true;
	}

	if (uf1 == uf2)
	{

		/*if( !e1 || !e1->content || !e2 || !e2->content ){
			PROFILE(( "sort_by_name: text not first %lx/%lx/%lx/%lx", e1, e1?e1->content:0, e2, e2?e2->content:0 ));
		}
		else
		*/

		if( e1->content->type == SECONTENT_TEXT && e2->content->type == SECONTENT_TEXT )
		{
			char *t1 = e1->content->c.text.text;
			char *t2 = e2->content->c.text.text;
#if INCLUDE_CINSENS
			return sort_by_name(t1, t2) > 0 ? true : false;
#else
			if( *t1 == *t2 ){
				if( fs_data.SortDir == UP )
				{
					char *t = t1;
					t1 = t2;
					t2 = t;
				}
				PROFRECr( return, bstrcmp,(t1+1, t2+1 ));
			}
			return fs_data.SortDir == DOWN ? *t1 > *t2 : *t1 < *t2;
#endif
		}
		/* same dir&same name => same file! */
	}
	return false;
}
static bool
sortbyext(struct scroll_info *list, struct scroll_entry *e1, struct scroll_entry *e2)
{
	long uf1, uf2;
	short tmp = 0;
	struct xattr *x1, *x2;
	struct sesetget_params p1, p2;

	list->get(list, e1, SEGET_USRFLAGS, &uf1);
	list->get(list, e2, SEGET_USRFLAGS, &uf2);

	p1.idx = p2.idx = -1;
	list->get(list, e1, SEGET_TEXTPTR, &p1);
	list->get(list, e2, SEGET_TEXTPTR, &p2);

	if ((uf1 & FLAG_SDIR) || (uf2 & FLAG_SDIR))
	{
		if ((uf1 & FLAG_SDIR) && (uf2 & FLAG_SDIR))
		{
			return sort_by_name(p1.ret.ptr, p2.ret.ptr) > 0 ? true : false;
		}
		else if (!(uf2 & FLAG_SDIR))
			return false;
		else
			return true;
	}

	uf1 &= FLAG_DIR;
	uf2 &= FLAG_DIR;
	if (uf1 < uf2)
	{
		/* folders in front */
		return true;
	}

	if (uf1 == uf2)
	{
		list->get(list, e1, SEGET_USRDATA, &x1);
		list->get(list, e2, SEGET_USRDATA, &x2);

		if ((tmp = sort_by_ext(p1.ret.ptr, p2.ret.ptr)))
		{
			if( fs_data.SortDir == UP )
				tmp = -tmp;
			return tmp > 0 ? true : false;
		}

		if ((tmp = sort_by_name(p1.ret.ptr, p2.ret.ptr)))
			return tmp > 0 ? true : false;

		if ((tmp = sort_by_date(x1, x2)))
			return tmp > 0 ? false : true;

		/*
		 * If same date, suffix and name, we let size decide...
		 */
		if (!S_ISDIR(x1->mode) && !S_ISDIR(x2->mode))
		{
			if (x1->size > x2->size)
				return true;
		}
	}
	return false;
}
static bool
sortbysize(struct scroll_info *list, struct scroll_entry *e1, struct scroll_entry *e2)
{
	long uf1, uf2;
	short tmp = 0;
	struct xattr *x1, *x2;
	struct sesetget_params p1, p2;

	list->get(list, e1, SEGET_USRFLAGS, &uf1);
	list->get(list, e2, SEGET_USRFLAGS, &uf2);

	p1.idx = p2.idx = -1;
	list->get(list, e1, SEGET_TEXTPTR, &p1);
	list->get(list, e2, SEGET_TEXTPTR, &p2);

	if ((uf1 & FLAG_SDIR) || (uf2 & FLAG_SDIR))
	{
		if ((uf1 & FLAG_SDIR) && (uf2 & FLAG_SDIR))
		{
			return sort_by_name(p1.ret.ptr, p2.ret.ptr) > 0 ? true : false;
		}
		else if (!(uf2 & FLAG_SDIR))
			return false;
		else
			return true;
	}

	uf1 &= FLAG_DIR;
	uf2 &= FLAG_DIR;
	if (uf1 < uf2)
	{
		/* folders in front */
		return true;
	}

	if (uf1 == uf2)
	{
		list->get(list, e1, SEGET_USRDATA, &x1);
		list->get(list, e2, SEGET_USRDATA, &x2);

		if (!S_ISDIR(x1->mode) && !S_ISDIR(x2->mode))
		{
			if (x1->size > x2->size)
				return fs_data.SortDir == DOWN ? true : false;
			else if (x1->size < x2->size)
				return fs_data.SortDir == DOWN ? false : true;
		}

		if ((tmp = sort_by_name(p1.ret.ptr, p2.ret.ptr)))
		{
			if( fs_data.SortDir == UP )
				tmp = -tmp;
			return tmp > 0 ? true : false;
		}
		if ((tmp = sort_by_ext(p1.ret.ptr, p2.ret.ptr)))
		{
			if( fs_data.SortDir == UP )
				tmp = -tmp;
			return tmp > 0 ? true : false;
		}
		if ((tmp = sort_by_date(x1, x2)))
		{
			if( fs_data.SortDir == UP )
				tmp = -tmp;
			return tmp > 0 ? false : true;
		}
	}
	return false;
}
static bool
sortbynothing(struct scroll_info *list, struct scroll_entry *e1, struct scroll_entry *e2)
{
	long uf1, uf2;
	struct sesetget_params p1, p2;

	list->get(list, e1, SEGET_USRFLAGS, &uf1);
	list->get(list, e2, SEGET_USRFLAGS, &uf2);

	p1.idx = p2.idx = -1;
	list->get(list, e1, SEGET_TEXTPTR, &p1);
	list->get(list, e2, SEGET_TEXTPTR, &p2);

	if ((uf1 & FLAG_SDIR) || (uf2 & FLAG_SDIR))
	{
		if ((uf1 & FLAG_SDIR) && (uf2 & FLAG_SDIR))
		{
			return sort_by_name(p1.ret.ptr, p2.ret.ptr) > 0 ? true : false;
		}
		else if (!(uf2 & FLAG_SDIR))
			return false;
		else
			return true;
	}
	return true;
}

static scrl_compare *sorters[] =
{
	sortbyname,
	sortbydate,
	sortbysize,
	sortbyext,
	sortbynothing,
};

/* yields true if case sensitive */
static bool
inq_xfs(struct fsel_data *fs, char *p, char *dsep)
{
	long c, t;

	c = d_pathconf(p, DP_CASE);
	t = fs->trunc = d_pathconf(p, DP_TRUNC);

	DIAG((D_fsel, NULL, "inq_xfs '%s': case = %ld, trunc = %ld", p, c, t));

	if (dsep)
	{
		* dsep		= fs->fslash[0];
		*(dsep+1) = 0;
	}

	if(c == DP_CASECONV)	/* case always converted */
		fs->fcase |= FS_FSNOCASE;
	else
		fs->fcase &= ~FS_FSNOCASE;

	if( c == DP_CASESENS)	/* case sens */
		fs->fcase &= ~FS_PATNOCASE;
	else
		fs->fcase |= FS_PATNOCASE;

	return !(c == DP_CASECONV && t == DP_DOSTRUNC);
}

static void
add_slash(char *p, char *to)
{
	char *pl = p + strlen(p) - 1;

	if( *pl == '*' )
		*pl = 0;
	else
	if (*pl != '/' && *pl != '\\')
		strcat(pl, to);
}
#if 0
/*
 * remove last item from a path
 */
static int
strip_last(char *p, char *to)
{
	char *pl;

	if( to && p != to )
	{
		strcpy( to, p );
		p = to;
	}
	pl = strmrchr( p, "/\\" );
	if( !pl )
	{
		return 0;
	}

	if (*(pl+1) == 0 )
	{
		*pl = 0;
		pl = strmrchr( p, "/\\" );
	}
	if( pl )
		*pl = 0;
	return 1;
}
#endif

static bool
disabled_exe(char *nam)
{
	char *ext = getsuf(nam);

	return (ext
		/* The mintlib does almost the same :-) */
		&& (	 !stricmp(ext, "prx")
				|| !stricmp(ext, "gtx") || !stricmp(ext, "apx")
				|| !stricmp(ext, "acx")));
}
static void
set_file(struct fsel_data *fs, char *fn, bool mark)
{
	DIAG((D_fsel, NULL, "set_file: fs.file='%s', wh=%d", fn ? fn : fs->file, fs->wind->handle));

	if( strmchr( fs->file, "*!?[" ) )
		fn = fs->file;	/* keep pattern in edit-field */

	/* fixup the cursor edit position */
	if (fn) {
		struct xa_aes_object ob = aesobj(fs->form->tree, FS_FILE);
		obj_edit(fs->form, fs->wind->vdi_settings,
			 ED_MARK, ob,
			0, -1, NULL, false, NULL,NULL, NULL,NULL);
		obj_edit(fs->form, fs->wind->vdi_settings,
			 ED_STRING, ob,
			mark ? 1 : 0, 0, fn, true, NULL, fs->wind->rect_list.start, NULL, NULL);
	}
	else
		redraw_toolbar(0, fs->wind, FS_FILE);

	/* redraw the toolbar file object */
	/* redraw_toolbar(0, fs->wind, FS_FILE); */
}
#define FSIZE_MAX 20


static void
destroy_xattr(void *data)
{
	kfree(data);
}

static void
read_directory(struct fsel_data *fs, SCROLL_INFO *list, SCROLL_ENTRY *dir_ent)
{
	bool csens;
	OBJECT *obtree = fs->form->tree;
	char nm[NAME_MAX+2 + FSIZE_MAX+2	+ 40 + 12], pattern[NAME_MAX*2];
	short xstate;
	long i, rep;

	PRDEF( read_directory,d_xreaddir);
	PRDEF( read_directory,match_pattern);
	PRDEF( read_directory,add);
	PRDEF( read_directory,f_xattr);

	PRINIT;

	if (dir_ent)
		list->get(list, dir_ent, SEGET_XSTATE, &xstate);

	if (dir_ent && (xstate & OS_OPENED))
	{
		struct sesetget_params p;

		p.e = dir_ent;
		p.level.flags = ENT_ISROOT;
		p.level.maxlevel = -1;
		p.level.curlevel = 0;
		p.arg.state.mask = ~(OS_SELECTED|OS_BOXED);
		p.arg.state.bits = 0;
		p.arg.state.method = ANYBITS;
		list->set(list, dir_ent, SESET_M_STATE, (long)&p, NORMREDRAW);

		/*
		 * If this directory is already opened, just close it
		 */
		list->set(list, dir_ent, SESET_OPEN, 0, NORMREDRAW);
		fs->selected_file = NULL;
		set_dir(list);
		dir_ent = NULL;
		return;
	}
	else
	{
		if (dir_ent)
		{
			long here;
			SCROLL_ENTRY *this;
			struct sesetget_params p;

			strcpy(fs->path, fs->root);
			here = strlen(fs->path);
			this = dir_ent;
			p.idx = -1;
			while (this)
			{
				list->get(list, this, SEGET_TEXTPTR, &p);
				strins(fs->path, fs->fslash, here);
				strins(fs->path, p.ret.ptr, here);

				this = this->up;
			}
			/* If realtime build, open entry here */
			if (fs->rtbuild)
				list->set(list, dir_ent, SESET_OPEN, 1, true);

			list->get(list, dir_ent, SEGET_USRFLAGS, &here);
			if (here & 1)
			{
				list->set(list, dir_ent, SESET_OPEN, 1, NORMREDRAW);
				return;
			}
		}
		else
			strcpy(fs->path, fs->root);

		csens = inq_xfs(fs, fs->path, fs->fslash);

		/* if | is first char in pat pattern is caseinsensitive */
		if (*fs->fs_pattern == '|')
		{
			strcpy( pattern, fs->fs_pattern + 1 );
			fs->fcase |= FS_PATNOCASE;
		}
		else
		{
			strcpy( pattern, fs->fs_pattern );
		}
		fs->rtflags = 0;

		if( fs->fcase ){
			strupr( pattern );
		}
		match_pattern( 0, pattern, false);	/* init */

		i = d_opendir(fs->path, 0);

		PROFILE(("fsel:Dopendir %s", fs->path ));

		if (i > 0)
		{
			struct xattr xat, *x;
			int num = 0, max_namlen = 0, max_szlen = 0;

			while (PROFREC(d_xreaddir,(NAME_MAX, i, nm, &xat, &rep) == 0))
			{
				struct scroll_content sc = {{0}};
				char *nam = nm+4;
				bool dir = S_ISDIR(xat.mode);
				bool sln = S_ISLNK(xat.mode);
				bool match = true;
				int l;

				num++;

				if (!(x = kmalloc(sizeof(*x))))
					continue;

				if (!csens)
					strupr(nam);

				if (sln)
				{
					char fulln[NAME_MAX+2];

					strcpy(fulln, fs->path);
					strcat(fulln, nam);

					DIAG((D_fsel, NULL, "Symbolic link: Fxattr on '%s'", fulln));
					PROFRECv(f_xattr,(0, fulln, &xat));
					dir = (xat.mode & 0xf000) == S_IFDIR;

					DIAG((D_fsel, NULL, "After Fxattr dir:%d", dir));
				}
				*x = xat;

				if (!dir){
					if( fs->fcase & FS_PATNOCASE ){
						char upnam[NAME_MAX];
						strncpy( upnam, nam, NAME_MAX-1 );
						upnam[NAME_MAX-1] = 0;
						strupr( upnam );
						PROFRECs(match=, match_pattern,(upnam, pattern, false));
					}
					else
						{PROFRECs(match=, match_pattern,(nam, pattern, false));}
				}
				else
				{
					if ( nam[0] == '.' )
					{
						if( nam[1] == 0 )	/* don't list '.'-entry */
						{
							continue;
						}
						if ( (nam[1] == '.' && nam[2] == 0) )
						{
							if (dir_ent)
								continue;
							sc.usr_flags |= FLAG_SDIR;
						}
					}
					if (fs->treeview)
						sc.xstate |= OS_NESTICON;
					sc.fnt = &dir_txt;
				}

				if (match)
				{
					OBJECT *icon = NULL;
					char *s = nam + (l=strlen(nam)) + 1;

					if( l > max_namlen )
						max_namlen	= l;
					sc.t.text = nam;

					if (dir)
					{
						sc.xflags = OF_OPENABLE;
						sc.usr_flags |= FLAG_DIR;
						icon = obtree + FS_ICN_DIR;
					}
					else
					{
						sc.xflags = 0;

						if (/*executable(nam) ||*/ (xat.mode & S_IXUGO))
						{
							if (sln)
								icon = obtree + FS_ICN_SYMLINK;
							else
								icon = obtree + FS_ICN_PRG;
							sc.fnt = &exe_txt;
						}
						else if (disabled_exe(nam))
						{
							icon = obtree + FS_ICN_PRG;
							sc.fnt = &dexe_txt;
						}
						else
						{
							icon = obtree + FS_ICN_FILE;
							sc.fnt = &fs_norm_txt;
						}
					}
					if (sln)
					{
						sc.fnt->normal.effects |= ITALIC;
						sc.fnt->selected.effects |= ITALIC;
					}
					else
					{
						sc.fnt->normal.effects &= ~ITALIC;
						sc.fnt->selected.effects &= ~ITALIC;
					}

					if (!dir)
					{
						if (xat.size < (1024UL * 1024) )
						{
							l = sprintf(s, 20, "%lu", xat.size);
						}
						else if (xat.size < (1024UL * 1024 * 10))
							l = sprintf(s, 20, "%lu KB", xat.size >> 10);
						else
							l = sprintf(s, 20, "%lu MB", xat.size >> 20);
					}
					else
						l = sprintf(s, 20, "<dir>");

					if( l > max_szlen )
						max_szlen = l;

					s += l + 1;

					l = sprintf(s, 20, " %02d:%02d:%02d", (xat.mtime >> 11) & 31, (xat.mtime >> 5) & 63, xat.mtime & 63);
					s += l + 1;

					{
						unsigned short year, month, day;

						year = ((xat.mdate >> 9) & 127) + 1980;


						month = ((xat.mdate >> 5) & 15) - 1;
						if (month < 0 || month > 11)
							month = 0;

						day = xat.mdate & 31;

						l = sprintf(s, 20, " %02d %s %04d", day, months[month], year);
					}

					s += l + 1;
					sprintf(s, 12, "%s%s%s", faccess[(xat.mode >> 6) & 7], faccess[(xat.mode >> 3) & 7], faccess[xat.mode & 7]);

					if (xat.mode & S_ISUID)
					{
						if (s[2] == 'x')
							s[2] = 's';
						else
							s[2] = 'S';
					}
					if (xat.mode & S_ISGID)
					{
						if (s[5] == 'x')
							s[5] = 's';
						else
							s[5] = 'S';
					}
					if (xat.mode & S_ISVTX)
					{
						if (s[8] == 'x')
							s[8] = 't';
						else
							s[8] = 'T';
					}

					sc.icon = icon;
					sc.t.strings = 5;
					sc.data = x;
					sc.data_destruct = destroy_xattr;

					PROFRECs(list->,add,(list, dir_ent, sorters[fs->sort], &sc,
						dir_ent ? SEADD_PRIOR|SEADD_CHILD : SEADD_PRIOR, 0,
						fs->rtbuild ? NORMREDRAW : NOREDRAW));
				}
			}
			d_closedir(i);
			match_pattern( 0, 0, false);	/* de-init */

			/* this is not elaborated
			 * try to adapt the 1st distance
			 */

			if( max_namlen )
			{
				short h = max_namlen;
				struct seset_tab tab;

				if (fs->treeview)
					h += list->cur->level * 2;
				if( !fs->treeview || fs->curr_longest < h )
				{
					fs->curr_longest = h;
					tab.index = FSLIDX_NAME;
					tab.flags = 0;
					if( h > MAXWIDTH )
						h = MAXWIDTH;

					if( fs->rtbuild )
						h += 10;

					/*list->vdi_settings->api->text_extent(list->vdi_settings, "X", &fs_norm_txt.n, &w, &h);*/

					tab.r = (GRECT){0,0,0, -h};
					list->set(list, NULL, SESET_TAB, (long)&tab, false);
				}
			}
			PROFILE(( "fsel: dir closed %d items", num ));
		}
		if (dir_ent)
		{
			/*
			 * Set bit 0 in user flags to indicate that this directory is read
			 */
			long uf;
			list->get(list, dir_ent, SEGET_USRFLAGS, &uf);
			uf |= FLAG_FILLED;
			list->set(list, dir_ent, SESET_USRFLAGS, uf, 0);
		}
	}

	if (!fs->rtbuild)
	{
		if (dir_ent)
			list->set(list, dir_ent, SESET_OPEN, 1, NORMREDRAW);
		else
		{
		#if 0
			if (!fs->tfile && fs->kbdnav && list->start)
			{
				list->set(list, list->start, SESET_CURRENT, 0, NOREDRAW);
				list->set(list, list->cur, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NOREDRAW);
			}
		#endif
			list->slider(list, NORMREDRAW);
			list->flags |= SIF_DIRTY;
		}
	}
	else if (dir_ent)
	{
		list->set(list, dir_ent, SESET_OPEN, 1, NORMREDRAW);
	}

	PRPRINT;

}

/*
 * Re-load a file_selector listbox
 * HR: version without the big overhead of separate dir/file lists
			 and 2 quicksorts + the moving around of all that.
			 Also no need for the Mintlib. (Mintbind only).
 */

static void
refresh_filelist(int lock, struct fsel_data *fs, SCROLL_ENTRY *dir_ent)
{
	OBJECT *form = fs->form->tree;
	OBJECT *sl;
	SCROLL_INFO *list;
	struct xa_wtxt_inf *wp[] = {&fs_norm_txt, &exe_txt, &dexe_txt, &dir_txt, 0};
	int objs[] = {FS_ICN_EXE, FS_ICN_DIR, FS_ICN_PRG, FS_ICN_FILE, FS_ICN_SYMLINK, 0};
	short wd, h;

	sl = form + FS_LIST;
	DIAG((D_fsel, NULL, "refresh_filelist: fs = %lx, obtree = %lx, sl = %lx", (unsigned long)fs, (unsigned long)fs->form->tree, (unsigned long)sl));
	list = object_get_slist(sl);

	fs->point = set_xa_fnt( fs->point + fs->fntinc, wp, form, objs, list, &wd, &h );

	if( fs->fntinc )
	{
		list->widest = 0;
		if( wd )
		{
			set_tabs( list, fs, wd );
		}
	}

	fs->fntinc = 0;

	if( h > 0 )
	{
		if( h < 12 || h > 22 )
			list->flags |= SIF_NO_ICONS;
		else
			list->flags &= ~SIF_NO_ICONS;
	}

	add_slash(fs->root, fs->fslash);

#ifdef FS_DBAR
	{
		TEDINFO *tx;

		tx = object_get_tedinfo(form + FS_CASE, NULL);
		sprintf(tx->te_ptext, 32, "%ld", fs->fcase&FS_FSNOCASE);

		tx = object_get_tedinfo(form + FS_TRUNC, NULL);
		sprintf(tx->te_ptext, 32, "%ld", fs->trunc);

		redraw_toolbar(lock, fs->wind, FS_DBAR);
	}
#endif
	DIAG((D_fsel, NULL, "refresh_filelist: fs.path='%s',fs_pattern='%s'",
		fs->root, fs->fs_pattern));

	/* Clear out current file list contents */
	if (!dir_ent)
	{
		fs->selected_file = NULL;
		list->empty( list, 0, 0 );
		if (fs->rtbuild)
		{
			list->redraw(list, NULL);
		}
	}
	if (!fs->treeview)
	{
		list->widest = 0;
		list->total_w = 0;
	}

	xa_graf_mouse(HOURGLASS, NULL, NULL, false);
	read_directory(fs, list, dir_ent);

	/* redraw again if rtbuild with correct tab-sizes */
	if( fs->rtbuild )
	{
		list->redraw(list, NULL);
	}
	xa_graf_mouse(ARROW, NULL, NULL, false);
}

static void
CE_refresh_filelist(int lock, struct c_event *ce, short cancel)
{
	if (!cancel)
	{
		struct fsel_data *fs = ce->ptr1;
		struct scroll_info *list = ce->ptr2;

		refresh_filelist(lock, fs, NULL);
		fs_prompt(list, fs->file, false);
	}
}

static int
fsel_drives(OBJECT *m, int drive, long *dmapp)
{
	int drv = 0, drvs = 0;
	int d = FSEL_DRVA;
	unsigned long dmap = b_drvmap();

	if( dmapp )
		*dmapp = dmap;
	while (dmap)
	{
		if (dmap & 1)
		{
			m[d].ob_state &= ~OS_CHECKED;

			if (drv == drive)
				m[d].ob_state |= OS_CHECKED;

			/* fix up the width from adjusting last entry, below */
			m[d].ob_width = m[(FSEL_DRVA + 1)].ob_width;
			/* The " "" is to prevent some stupid program from replacing 2 blanks by "\t" */
			sprintf(m[d].ob_spec.free_string, 32, " "" %c:", letter_from_drive(drv));
			d = m[d].ob_next;
			drvs++;
		}
		dmap >>= 1;
		drv++;
	}

	/*
	 * the menu has 2 columns;
	 * make the last entry the total width if
	 * number of drives is odd
	 */
	if (drvs & 1)
		m[d-1].ob_width = m[FSEL_DRVBOX].ob_width;

	while (d != FSEL_DRVBOX)
	{
		m[d].ob_flags |= OF_HIDETREE;
		/* prevent finding those. */
		*(m[d].ob_spec.free_string + 2) = '~';
		d = m[d].ob_next;
	}

	m[FSEL_DRVBOX].ob_height = ((drvs + 1) / 2) * screen.c_max_h;

	sprintf(m[FSEL_DRV].ob_spec.free_string, 32, " %c:", letter_from_drive(drive));
	return drvs;
}

/*
 * length of the free strings in the popup menu
 * of the resource file,
 * including terminating zero, but not counting the 2
 * leading spaces.
 * Must also match the length of FSEL_FILTER.
 * Note that these strings are shorter than FS_PATLEN.
 */
#define P_free_string_len 16
/*
 * number of leading spaces in those free strings
 */
#define FS_OFFS	2


static void
fsel_filters(OBJECT *m, const char *pattern)
{
	char p[P_free_string_len];
	short box = FSEL_PATBOX;
	short d;
	int i, l;

	add_pattern(pattern);

	if (pattern && *pattern && fs_data.fs_patterns[0][0])
	{
		int maxl;

		for (i = 0, maxl = 0, d = m[box].ob_head; i < FS_NPATTERNS && fs_data.fs_patterns[i][0] && d != box; i++, d = m[d].ob_next)
		{
			m[d].ob_state &= ~OS_CHECKED;
			if (strcmp(pattern, fs_data.fs_patterns[i]) == 0)
			{
				m[d].ob_state |= OS_CHECKED;
			}
			m[d].ob_flags &= ~OF_HIDETREE;	/* may be a new pattern */
			strncpy(p, fs_data.fs_patterns[i], sizeof(p)-1);
			p[sizeof(p)-1] = 0;
			l = (int)strlen(p) + FS_OFFS + 1;
			strcpy(m[d].ob_spec.free_string + FS_OFFS, p);
			l *= screen.c_max_w;
			if (l > maxl)
				maxl = l;
		}

		m[box].ob_height = i * screen.c_max_h;

		while (d != box)
		{
			m[d].ob_flags |= OF_HIDETREE;
			d = m[d].ob_next;
		}
		for (d = m[box].ob_head; d != box; d = m[d].ob_next)
			m[d].ob_width = maxl;
		m[box].ob_width = maxl;
	}
	strncpy(p, pattern, sizeof(p)-1);
	p[sizeof(p)-1] = 0;
	strcpy(m[FSEL_FILTER].ob_spec.free_string + 1, p);
}

/* HR: a little bit more exact. */
/* Dont need form as long as everything is global 8-} */
static void
fs_updir(struct scroll_info *list)
{
	struct fsel_data *fs = list->data;
	int drv;
	bool atroot = false;
	char outof[128];

	outof[0] = '\0';

	if (*fs->root)
	{
		int s = strlen(fs->root) - 1;

		if (fs->root[s] == '/' || fs->root[s] == '\\')
			fs->root[s--] = '\0';

		DIAG((D_fsel, NULL, "fs_updir '%s'", fs->root));

		while (  s
					&& fs->root[s] != ':'
					&& fs->root[s] != '/'
					&& fs->root[s] != '\\')
			s--;

		if (fs->root[s] == ':')
		{
			if (fs->root[s + 1] == '\0')
				atroot = true;

			fs->root[++s] = *fs->fslash;
		}
		if (!atroot)
			strncpy(outof, &fs->root[s + 1], sizeof(outof));

		fs->root[++s] = 0;
		DIAG((D_fsel,NULL," 	--> 	'%s'", fs->root));
	}

	if (!atroot)
	{
		if ((drv = get_drv(fs->root)) >= 0)
			strcpy(fs_data.fs_paths[drv], fs->root);

		/*
		 * Moving root, need to reset relatives..
		 */
		fs->selected_file = NULL;
		if( fs->selected_dir )
			fs->selected_dir = fs->selected_dir->up;
		set_dir(list);
		refresh_filelist(LOCK_FSEL, fs, NULL);
		fs_prompt(list, atroot ? fs->file : outof, false);
	}
}

static void
fs_closer(struct scroll_info *list, bool rdrw)
{
	struct fsel_data *fs = list->data;

	set_file(fs, fs->ofile, false);
	fs_updir(list);
}

static void
fs_enter_dir(struct fsel_data *fs, struct scroll_info *list, struct scroll_entry *dir_ent)
{
	struct sesetget_params p;
	int drv;

	add_slash(fs->root, fs->fslash);

	if (!fs->treeview)
	{
		p.idx = -1;
		list->get(list, dir_ent, SEGET_TEXTPTR, &p);
		strcat(fs->root, p.ret.ptr);
		fs->selected_dir = NULL;
		if( !strncmp( fs->file, p.ret.ptr, strlen( fs->file ) ) )
		{
			fs->file[0] = fs->ofile[0] = 0;
		}
	}

	if ((drv = get_drv(fs->root)) >= 0)
		strcpy(fs_data.fs_paths[drv], fs->root);

#if 0
	/* resolve symlinks (does not work in treeview (->todo)) ->menu */
	if( (long)fs->data == HL_LAUNCH )
	{
		XATTR xat;
		char linkname[PATH_MAX+1];
		long r = f_xattr(1, fs->root, &xat);
		if( !r && S_ISLNK( xat.mode ) )
		{

			if( !_f_readlink( PATH_MAX, linkname, fs->root ) )
			{
				strcpy( fs->root, linkname );
			}
		}
	}
#endif
		set_file(fs, fs->ofile, false);
	set_dir(list);
	fs->selected_file = NULL;
	refresh_filelist(LOCK_FSEL, fs, fs->treeview ? dir_ent : NULL);
	if (fs->treeview && dir_ent)
	{
		short xs;
		list->get(list, dir_ent, SEGET_XSTATE, &xs);
		if (xs & OS_OPENED)
			fs_prompt(list, fs->file, false);
	}
	else
		fs_prompt(list, fs->file, false);
}

static int
fs_item_action(struct scroll_info *list, struct scroll_entry *this, const struct moose_data *md, short flags)
{
	bool fs_done = this ? false : true;
	struct fsel_data *fs = list->data;
	struct sesetget_params p;
	long uf;
	short state, xstate;

	DIAG((D_fsel, NULL, "fs_item_action %lx, fs=%lx", (unsigned long)list->cur, (unsigned long)fs));

	if (this)
	{
		list->get(list, this, SEGET_USRFLAGS, &uf);
		list->get(list, this, SEGET_STATE, &state);
		list->get(list, this, SEGET_XSTATE, &xstate);

		if (!(uf & FLAG_DIR))
		{
			DIAG((D_fsel, NULL, " --- nodir '%s'", this->content->c.text.text));

			/*
			 * If not already selected, or if double-clicked, make selection
			*/
			if (!(state & OS_SELECTED) || !md || md->clicks > 1)
			{
				if (md)
				{
					modify_mult_state(list, this->up, 0, ~(OS_SELECTED|OS_BOXED));
					p.idx = -1;
					p.e = this;
					p.arg.txt = fs->file;
					list->get(list, this, SEGET_TEXTCPY, &p);
					fs->selected_file = this;
					fs->selected_dir = this->up;
					list->set(list, this, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NORMREDRAW);
					set_dir(list);
					set_file(fs, fs->file, true);
					if (md->clicks > 1)
						fs_done = true;
					else
						fs->tfile = true;
					fs->kbdnav = false;
				}
				else if (!flags)
					fs_done = true;
			}
			/*
			 * else unselect
			 */
			else if (state & OS_SELECTED)
			{
				if (md)
					fs->kbdnav = false;

				if (md && md->clicks < 2)
				{
					list->set(list, this, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16), NORMREDRAW);
					fs->selected_file = NULL;
					fs->tfile = false;
					list->set(list, NULL, SESET_CURRENT, 0, NOREDRAW);
					set_file(fs, fs->ofile, false);
				}
				else if (!flags && (!md || md->clicks > 1))
				{
					fs_done = true;
				}
			}
		}
		else
		{
			/* folder entry action */
			DIAG((D_fsel, NULL, " --- folder '%s'", this->content->c.text.text));
			p.idx = -1;
			p.arg.txt = "..";

			if (list->get(list, this, SEGET_TEXTCMP, &p) && !p.ret.ret)
			{
				if (md)
					fs->kbdnav = false;

				set_file(fs, fs->ofile, false);
				fs_updir(list);
				if (!md && !fs->kbdnav)
				{
					fs->tfile = false;
				}
			}
			else
			{
				p.arg.txt = ".";
				if (list->get(list, this, SEGET_TEXTCMP, &p) && p.ret.ret)
				{
					if (!md || (md->state & MBS_RIGHT) || md->clicks > 1)
					{
						fs->selected_dir = this;
						if (!md)
						{
							/*
							 * If return pressed and there is something in input-field
							 * we return, else we close the directory.
							 */
							if ((xstate & OS_OPENED))
							{
								if (flags)
								{
									if (!(flags & 2))
										list->set(list, this, SESET_OPEN, 0, NORMREDRAW);
								}
								else /*if (*fs->file)*/
									fs_done = true;
							}
							else
							{
								if (flags)
								{
									if (!(flags & 2))
									{
										if (!this || !this->up)
											fs_updir(list);
									}
									else
									{
										fs_enter_dir(fs, list, this);
									}
								}
								else
								{
									if (!fs->kbdnav)
										fs->tfile = false;
									fs_enter_dir(fs, list, this);
								}
							}
						}
						else
						{
							fs->kbdnav = false;

							if (xstate & OS_OPENED)
							{
								if ((md->state & MBS_LEFT) && fs->tfile)
									fs_done = true;
							}

							if (!fs_done)
							{
								fs_enter_dir(fs, list, this);
							}
						}
					}
					else
					{
						if (md)
							fs->kbdnav = false;

						if ((state & OS_SELECTED))
						{
							fs->selected_dir = this->up;
							list->set(list, this, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16), NORMREDRAW);
						}
						else
						{
							modify_mult_state(list, this->up, 0, ~(OS_BOXED|OS_SELECTED));
							list->set(list, this, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NORMREDRAW);
							fs->selected_dir = this;
						}
						set_dir(list);

						set_file(fs, fs->ofile, false);
					}
				}
				else
					fs_done = true;
			}
		}
	}

	DIAG((D_fsel, NULL, "fs_item_action: %s%s", fs->root, fs->file));

	if (fs_done)
	{
		fs_cwd(list, fs->path, 2);

		if( this && this->content )
		{
			strncpy( fs->file, this->content->c.text.text, this->content->c.text.tblen);
		}
		if (fs->selected)
		{
			fs->selected(list->lock, fs, fs->path, fs->file);
		}
	}

	return true;
}
static int
fs_click(struct scroll_info *list, struct scroll_entry *this, const struct moose_data *md)
{
	/* since mouse click we should _not_ clear the file field */
	/*fs->clear_on_folder_change = 0;*/

	if (this)
		fs_item_action(list, this, md, 0);
	return true;
}
static int
fs_click_nesticon(struct scroll_info *list, struct scroll_entry *this, const struct moose_data *md)
{
	struct fsel_data *fs = list->data;
	long uf;

	if (this)
	{
		list->get(list, this, SEGET_USRFLAGS, &uf);

		if (uf & FLAG_DIR)
		{
			fs_enter_dir(fs, list, this);
		}
	}
	return true;
}

static void
change_fontsz( char nk, struct scroll_info *list)
{
	struct fsel_data *fs = list->data;
	nk == '+' ? fs->fntinc++ : fs->fntinc--;

	refresh_filelist(LOCK_FSEL, fs, NULL);
	list->redraw(list, NULL);
	fs_prompt( list, fs->file, true );
}

/*
 * search for an unused drive-letter for cancel/Ok-shortcuts
 * and modify the upper byte of ob_state accordingly
 */
static void fs_set_shortcut_letter( OBJECT *o, long dmap )
{
	char *text = o->ob_spec.free_string;
	int b;

	for ( ; *text == ' '; text++ )
		;
	for( b = 0; *text; text++, b++ )
		if( !( ( 1UL << drive_from_letter(*text)) & dmap) )
			break;
	if( !*text )
		o->ob_state &= (~OS_WHITEBAK & 0x80ff);
	else
		o->ob_state |= (b << 8);	/* set unused char */
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
	int lock = 0;
	OBJECT *obtree = wt->tree;
	struct scroll_info *list = object_get_slist(obtree + FS_LIST);
	struct fsel_data *fs = list->data;
#ifdef FS_FILTER
	TEDINFO *filter = object_get_tedinfo(obtree + FS_FILTER, NULL);
#endif

	wt->which = 0;

	switch (aesobj_item(&fr->obj))
	{
	/*
	 *
	 */
	case FS_LIST:
	{
		DIAGS(("filesel_form_exit: Moved the shit out of form_do() to here!"));
		if ( fr->md && ((aesobj_ob(&fr->obj)->ob_type & 0xff) == G_SLIST))
		{
			if (fr->md->clicks > 1)
				dclick_scroll_list(lock, obtree, aesobj_item(&fr->obj), fr->md);
			else
				click_scroll_list(lock, obtree, aesobj_item(&fr->obj), fr->md);
		}
		break;
	}
#ifdef FS_UPDIR
	/* Go up a level in the filesystem */
	case FS_UPDIR:
	{
		fs_updir(list);
		break;
	}
#endif

	case FS_HFNTSIZE:
		change_fontsz( '+', list );
		object_deselect(wt->tree + FS_HFNTSIZE);
		redraw_toolbar(lock, wind, FS_HFNTSIZE);
	break;
	case FS_LFNTSIZE:
		change_fontsz( '-', list );
		object_deselect(wt->tree + FS_LFNTSIZE);
		redraw_toolbar(lock, wind, FS_LFNTSIZE);
	break;

	/* Accept current selection - do the same as a double click */
	case FS_OK:
	{
		object_deselect(wt->tree + FS_OK);
		redraw_toolbar(lock, wind, FS_OK);
		if( fs->rtflags & (FS_CREATE_FOLDER|FS_RENAME_FILE) )
		{
			char pt[PATH_MAX];
			long r;
			struct sesetget_params p;

			sprintf( pt, sizeof(pt)-1, "%s%s", fs->path, fs->file);
			if( fs->rtflags & FS_CREATE_FOLDER )
			{
				_d_create( pt );
			}
			else if( fs->rtflags & FS_RENAME_FILE )
			{
				char pt2[PATH_MAX];
				char *fname = 0;
				struct sesetget_params pa;
				pa.idx = -1;
				if( list->cur )
				{
					pa.arg.txt = fs->file;
					list->get(list, list->cur, SEGET_TEXTCPY, &pa);
					fname = fs->file;
				}
				if( !fname || !*fname )
					fname = fs->ofile;

				if( strmchr( fs->file, "/\\" ) )
					strcpy( pt, fs->file );

				sprintf( pt2, sizeof(pt2)-1, "%s%s", fs->path, fname);
				r = _f_rename( 0, pt2, pt );
				UNUSED( r );
				BLOG((0,"rename '%s' -> '%s' -> %ld", pt2, pt, r ));
			}
			refresh_filelist(LOCK_FSEL, fs, 0);

			p.idx = -1;
			strcpy( pt, fs->path );
			p.arg.txt = pt;
			list->get(list, 0, SEGET_ENTRYBYTEXT, &p);
			fs->selected_dir = p.e;

			set_dir(list);

			list->redraw(list, NULL);
			fs_prompt(list, fs->file, false);
			fs->rtflags &= ~(FS_CREATE_FOLDER|FS_RENAME_FILE);
			list->wt->focus = aesobj(fs->form->tree, FS_LIST);
			break;
		}
#ifdef FS_FILTER
		if (strcmp(filter->te_ptext, fs->fs_pattern) != 0)
		{
			/* changed filter */
			strcpy(fs->fs_pattern, filter->te_ptext);
			refresh_filelist(LOCK_FSEL, fs, NULL);
			fs_prompt(list, fs->file, false);
		}
#else
		if( strmchr( fs->file, "*!?[|" ) )
		{
			strncpy(fs->fs_pattern, fs->file, sizeof(fs->fs_pattern)-1);

			/* copy new pattern into filters */
			fsel_filters(fs->menu->tree, fs->file);
			fs->file[0] = 0;
			set_file( fs, fs->file, true );
			display_widget(lock, fs->wind, get_widget(fs->wind, XAW_MENU), NULL);

			/* if treeview all gets closed! (todo) */
			refresh_filelist(LOCK_FSEL, fs, 0 );
			list->redraw(list, NULL);
		}
		else
#endif
		{
			struct scroll_entry *sel = get_selected(list);

			if (sel)
			{
				fs_item_action(list, sel, NULL, 0);
			}
			else if (fs->selected_dir && !*fs->file)
				fs_item_action(list, fs->selected_dir, NULL, 0);
			else
				fs_item_action(list, NULL, NULL, 0);
#if 0
			/* Just return with current selection */
			if (fs->kbdnav && sel)
				fs_item_action(list, sel/*list->cur*/, NULL, 0);
			else if (!fs->kbdnav)
				fs_item_action(list, fs->selected_dir, NULL, 0);
			else
				fs_item_action(list, fs->selected_dir, NULL, 0);
#endif
		}
		break;
	}
	/* Cancel this selector */
	case FS_CANCEL:
	{
		object_deselect(wt->tree + FS_CANCEL);
		redraw_toolbar(lock, wind, FS_CANCEL);
		fs->selected_file = NULL;
		if (fs->canceled)
			fs->canceled(lock, fs, fs->root, "");
		else
			close_fileselector(lock, fs);
		break;
	}
	}
}


static int
find_drive(int a, struct fsel_data *fs)
{
	XA_TREE *wt = get_widget(fs->wind, XAW_MENU)->stuff.wt;
	OBJECT *m = wt->tree;
	int d = FSEL_DRVA;

	do {
		int x = tolower(*(m[d].ob_spec.free_string + 2));
		if (x == '~')
			break;
		if (x == a)
			return d;
	} while (m[d++].ob_next != FSEL_DRVA - 1);

	/* maybe a shortcut: no ping here */
	/* ping(); */

	return -1;
}

static void
fs_change(int lock, struct fsel_data *fs, OBJECT *m, int p, int title, int d, char *t)
{
	XA_WIDGET *widg = get_widget(fs->wind, XAW_MENU);
	int bx = d - 1, tlen = FS_PATLEN -1;

	do
		m[d].ob_state &= ~OS_CHECKED;
	while (m[d++].ob_next != bx);

	m[p].ob_state |= OS_CHECKED;
	if( title == FSEL_FILTER )
		tlen = 15;
	sprintf(m[title].ob_spec.free_string, tlen, " %s", m[p].ob_spec.free_string + FS_OFFS);
	if( title == FSEL_FILTER )
		m[title].ob_spec.free_string[tlen]  = 0;
	widg->start = 0;
	m[title].ob_state &= ~OS_SELECTED;
	display_widget(lock, fs->wind, widg, NULL);
	if( title == FSEL_FILTER )
	{
		strcpy(t, fs_data.fs_patterns[p-FSEL_PATA]);	/* copy from pattern-list not from menu-text */
	}
	else
		strcpy(t, m[p].ob_spec.free_string + FS_OFFS);
}

static struct scroll_entry *
filename_completion(struct scroll_info *list)
{
	struct sesetget_params p;
	struct scroll_entry *this;
	struct fsel_data *fs = list->data;
	long uf;

	p.idx = 0;
	p.arg.state.method = ANYBITS;
	p.arg.state.mask = p.arg.state.bits = OS_BOXED;
	p.level.flags = 0;
	p.level.maxlevel = -1;
	p.level.curlevel = 0;

	list->get(list, NULL, SEGET_ENTRYBYSTATE, &p);

	if ((this = p.e))
	{
		list->get(list, this, SEGET_USRFLAGS, &uf);
		list->set(list, this, SESET_STATE, ((long)(OS_BOXED|OS_SELECTED) << 16) | OS_SELECTED, NORMREDRAW);
		p.idx = -1;
		p.arg.txt = fs->file;
		list->get(list, this, SEGET_TEXTCPY, &p);
		if (!(uf & FLAG_DIR))
		{
			fs->selected_file = this;
			fs->selected_dir = this->up;
		}
		else
		{
			fs->selected_dir = this;
			fs->selected_file = NULL;
		}
		set_dir(list);
		set_file(fs, fs->file, true); 	/* update filename */
		fs->tfile = false;
	}
	return this;
}
/*
 * This function is called by Form_Keyboard() to let G_SLIST do further keyhandling
 * ks:unused
 */
static unsigned short
fs_slist_key(struct scroll_info *list, unsigned short keycode, unsigned short ks)
{
	SCROLL_ENTRY *was, *this;
	struct fsel_data *fs = list->data;
	long uf;
	unsigned short keyout = keycode;

	switch (keycode)
	{
		case SC_UPARROW:	/* 0x4800 */	/* arrow up */
		case SC_DNARROW:	/* 0x5000 */	/* arrow down */
		case SC_LFARROW:	/* 0x4b00 */	/* arrow left */
		case SC_RTARROW:	/* 0x4d00 */	/* arrow right */
		case SC_INSERT: 	/* 0x5200 */	/* insert */
		{
			short xsb, xsa, kk;

			fs->kbdnav = true;

			if (!(was = filename_completion(list)))
				was = get_selected(list);

			if (was)
				list->get(list, was, SEGET_XSTATE, &xsb);

			list->set(list, was, SESET_CURRENT, 0, NOREDRAW);

			if ((keycode == SC_LFARROW || keycode == SC_RTARROW)
					&& (!fs->treeview || list->start == NULL || scrl_cursor(list, keycode, 0) == 0) )
			{
				this = list->cur;

				if (keycode == SC_RTARROW)
				{
					if (this /*&& (!(was->usr_flags & FLAG_DIR) || was == this ) */)
					{
						if( !(this->usr_flags & FLAG_DIR) )
						{
							fs->kbdnav = false;
							list->wt->focus = aesobj(fs->form->tree, FS_FILE);
							if( *fs->file )
								set_file( fs, fs->file, false );
						}
						else
							fs_item_action(list, was, NULL, 3);
					}
					/*
					else
					{
						set_selected(list, this);
					}
					*/
				}
				else
				{
					if (was && was == this)
					{
						/* If current didnt change, we check if current was opened
						 * before, in which case we dont do anything, because it was
						 * automatically closed.
						 * If current was closed already, we're at root, and this
						 * arrow left should go one dir up
						 */
						list->get(list, this, SEGET_XSTATE, &xsa);
						if (!(xsb & OS_OPENED) && !(xsa & OS_OPENED))
						{
							if( *fs->ofile )
								set_file(fs, fs->ofile, false);
							fs_updir(list);
							if (!get_selected(list))
								scrl_cursor(list, SC_INSERT, 0);
						}
						else
						{
							if( fs->selected_dir && fs->selected_dir != list->cur )
							{
								fs->selected_dir = fs->selected_dir->up;
								set_dir(list);
							}
						}
					}
				}
			}
			else
			{
				list->cur = was;

				kk = (!was
					&& (keycode == SC_DNARROW || keycode == SC_UPARROW)) ?
						SC_SPACE : keycode;

				if (scrl_cursor(list, kk, 0) == 0)
				{
					list->get(list, NULL, SEGET_CURRENT, &this);

					if (this)
					{
						fs->kbdnav = true;
						list->get(list, this, SEGET_USRFLAGS, &uf);

						if (!(uf & FLAG_DIR))
						{
							struct sesetget_params p;

							fs->selected_dir = this->up;
							if( fs->treeview )
								set_dir(list);

							fs->selected_file = this;
							p.idx = -1;
							p.arg.txt = fs->file;
							list->get(list, this, SEGET_TEXTCPY, &p);
							set_file(fs, fs->file, true);
							fs->tfile = false;
						}
						else
						{
							if (this != fs->selected_dir)
							{
								fs->selected_dir = this;
								fs->selected_file = NULL;
								if( fs->treeview )
									set_dir(list);
							}
							if( *fs->ofile )
								set_file(fs, fs->ofile, false);
						}
						if (kk == SC_INSERT)
							fs->kbdnav = true;
					}
					else if (kk == SC_INSERT)
					{
						if (was)
						{
							if (was == fs->selected_dir)
							{
								fs->selected_dir = was->up;
							}
							else if (was == fs->selected_file)
							{
								fs->selected_file = NULL;
							}
							set_file(fs, fs->ofile, false);
						}
						fs->kbdnav = false;
					}
				}
			}
			keyout = 0;
			break;
		}
	#if 0
		case SC_TAB:
		{
			fs->kbdnav = true;
			filename_completion(list);
			keyout = 0;
			break;
		}
	#endif
	}
	return keyout;
}

static void delete_item(struct scroll_info *list, struct fsel_data *fs)
{
	char pt[PATH_MAX];
	long r=0;
	struct sesetget_params p;

	p.idx = -1;

	p.arg.txt = fs->file;
	if ( list->cur && list->get(list, list->cur, SEGET_TEXTCPY, &p))
	{
		long uf;

		list->get(list, list->cur, SEGET_USRFLAGS, &uf);

		if( fs->treeview && (uf & (FLAG_DIR|FLAG_SDIR)) && (uf & FLAG_FILLED) )
			p.arg.txt = "";
		sprintf( pt, sizeof(pt)-1, "%s%s", fs->path, p.arg.txt);

		if( uf & (FLAG_DIR|FLAG_SDIR) )
			r = _d_delete( pt );
		else
			r = _f_delete( pt );

		BLOG((0,"delete '%s' -> %ld", pt, r));
		if( !r )
		{
			SCROLL_ENTRY *c = list->cur;
			fs_slist_key(list, SC_UPARROW, 0);
			list->del( list, c, true );
		}
	}
}

static char *strdel( char *s, int start, int len )
{
	char *p;
	for( p  = s + start; *p; p++ )
		*p = *(p+len);
	*p = 0;
	return s;
}
static void
cnvpath( char *path, char slnew, char slold )
{
	if( slnew == '/' )
	{
		if( *(path+1) == ':' && *(path+2) == '\\' )
		{
			if( *path == 'u' )
				strdel( path, 0, 2 );
			else
			{
				char d = tolower(*path);
				*path = '/';
				*(path+1) = d;
			}
		}
	}
	else
	{
		if( *path == '/' && isalpha(*(path+1)) && *(path+2) == '/' )
		{
			*path = toupper(*(path+1));
			*(path+1) = ':';
		}
		else if( !(*path == 'u' && *(path+1) == ':') )
		{
			strins( path, "u:", 0 );
		}
	}
	for( ; *path; path++ )
		if( *path == slold )
			*path = slnew;
}

static void
fs_msg_handler(
	struct xa_window *wind,
	struct xa_client *to,
	short amq, short qmf,
	short *msg);

#if __GNUC_PREREQ(10,0)
#pragma GCC push_options
#pragma GCC optimize "-O1"
#endif
/*
 * FormKeyInput()
 */
static bool
fs_key_form_do(int lock,
				 struct xa_client *client,
				 struct xa_window *wind,
				 XA_TREE *wt,
				 const struct rawkey *key,

				 struct fmd_result *res_fr)
{
	unsigned short nkcode = key->norm, nk;
	struct scroll_info *list = object_get_slist(get_widget(wind, XAW_TOOLBAR)->stuff.wt->tree + FS_LIST);
	struct fsel_data *fs = list->data;

	wt = get_widget(wind, XAW_TOOLBAR)->stuff.wt;

	if( !fs || !fs->menu )
	{
		BLOG((0,"fs_key_form_do:internal error:menu=0,fs=%lx", (unsigned long)fs));
		return false;
	}

	if (nkcode == 0)
		nkcode = nkc_tconv(key->raw.bcon);
	nk = tolower(nkcode & 0xff);

	/* HR 310501: alt + letter :: select drive */
	if (key->raw.conin.state == K_ALT)
	{
		if (drive_from_letter(nk) >= 0)
		{
			int drive_object_index = find_drive(nk, fs);
			if (drive_object_index >= FSEL_DRVA){
				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
							 MN_SELECTED, 0, 0, FSEL_DRV,
							 drive_object_index, 0, 0, 0);

				return true;
			}
		}
	}

	if ((key->raw.conin.state & K_CTRL) )
	{
	/* ctrl + backspace :: fs_updir() */
		if( (nkcode & NKF_CTRL) && nk == NK_BS)
		{
			set_file(fs, fs->ofile, false);
			fs_updir(list);
		}
#if 0
		else if (nk == '*')
		{
			/* change the filter to '*'
			 * - this should always be the FSEL_PATA entry IIRC
			 */
			fs_change(lock, fs, fs->menu->tree,
					FSEL_PATA, FSEL_FILTER, FSEL_PATA, fs->fs_pattern);
			fs->selected_dir = fs->selected_file = NULL;
			/* apply the change to the filelist */
			refresh_filelist(LOCK_FSEL, fs, NULL);
			fs_prompt(list, fs->file, false);
		}
#endif
		else if( nk == 'i' )	/* toggle casesensitivity */
		{
			int l = strlen( fs->fs_pattern );
			char *cp;

			if( *fs->fs_pattern == '|' )
			{
				for( cp = fs->fs_pattern; *cp; cp++ )
					*cp = *(cp+1);
			}
			else
			{
				for( cp = fs->fs_pattern + l, *(cp+1) = 0; cp > fs->fs_pattern; cp-- )
				{
					*cp = cp[-1];
				}
				*cp = '|';
			}

			fsel_filters(fs->menu->tree, fs->fs_pattern);
			display_widget(lock, fs->wind, get_widget(fs->wind, XAW_MENU), NULL);
			refresh_filelist(LOCK_FSEL, fs, NULL);
			list->redraw(list, NULL);
		}
		else if( nk == 'o' )	/* open menu */
		{
			XA_WIDGET *widg = get_widget(wind, XAW_MENU);
			struct c_event ce;

			ce.ptr1 = wind;
			ce.ptr2 = widg;
			cXA_open_menubykbd( 0, &ce, false);
		}
		else if( nk == 'l' )
		{
			list->redraw(list, NULL);
		}
		else if( nk == '/' )
		{
			char sl = fs->fslash[0] == '\\' ? '/' : '\\';
			cnvpath( fs->path, sl, fs->fslash[0] );
			cnvpath( fs->root, sl, fs->fslash[0] );
			fs->fslash[0] = sl;
			set_dir( list );
			list->redraw(list, NULL);
		}
		else
		{
			OBJECT *obtree;
			int i, k = toupper(nk), l = 0;
			short msg[16];
			char *s, *t, file[sizeof(fs->file)];

			obtree = fs->menu->tree;
			msg[0] = MN_SELECTED;
			msg[3] = FSEL_OPTS;
			for( i = FSM_RTBUILD; i <= FSM_DELETE; i++ )
			{
				s = obtree[i].ob_spec.free_string;
				if( s && *s != '-' )
				{
					t = strchr(s, '^');
					if( !t )
						t = strrchr( s + strlen(s) - 2, ' ' );
					if( t )
					{
						if( *(t+2) <= ' ' )
							l = *(t+1);
						else if( !strnicmp( t+1, xa_strings(XS_KEY_SPACE), 5 ) )
							l = ' ';
						if( l == k )
						{
							msg[4] = i;
							strcpy( file, fs->file);
							fs_msg_handler( wind, client, AMQ_NORM, QMF_NORM, msg );
							strcpy( fs->file, file );
							if( i < FSM_COPY )
								fs_prompt( list, fs->file, true );
							break;
						}
					}
				}
				if( i == FSM_REVSORT )
				{
					msg[3] = FSEL_FILE;
					i = FSM_COPY -1;
				}
			}
		}

	}
	else if( focus_item(wt) != FS_FILE && nk == 0x1f && !(fs->rtflags & (FS_CREATE_FOLDER|FS_RENAME_FILE)) )
	{
		delete_item(list, fs);
	}
	else
	{
		struct fmd_result fr;
		struct rawkey k = *key;
		struct sesetget_params p;
		p.idx = -1;

		/* letter on uppercase fs: make uppercase */
		if( (fs->fcase & FS_FSNOCASE) && (k.aes & 0x00ff) && k.norm >= 'a' && k.norm <= 'z'
				&& !strmchr( fs->file, "*!?[|" ) )
		{
			unsigned char c = k.norm - ('a' - 'A');
			k.aes = (k.aes & 0xff00) | c;
			k.norm = c;
		}

		if( k.aes == SC_TAB )
		{
			struct scroll_entry *e = get_selected(list);
			if( e )
			{
				struct sesetget_params seget;
				int match;

				seget.e = e;
				match = list_get_match( list, &seget, &p, fs->file, (k.raw.conin.state & 3) ? -1 : 1);
				e = seget.e;
				if( match )
					fs_prompt(list, p.ret.ptr , true);
				return true;
			}
		}

		else if( k.aes == SC_RTARROW )
		{
			struct scroll_entry *e = get_selected(list);

			if( e )
			{
				list->get(list, e, SEGET_TEXTPTR, &p);
				if( !(e->usr_flags & FLAG_DIR) )
				{
					if( fs->kbdnav == true && strcmp( fs->file, p.ret.ptr ) )
					{
						/* complete selected */
						strcpy( fs->file, p.ret.ptr );
						set_file( fs, p.ret.ptr , false);
						fs_prompt(list, p.ret.ptr , true);
						fs->kbdnav = false;
						return true;
					}
				}
				else
				{
					/* enter dir */
					fs->kbdnav = true;
					list->wt->focus = aesobj(fs->form->tree, FS_LIST);
				}
			}
		}
		if (Key_form_do(lock, client, wind, wt, &k, &fr))
		{
			/* Ozk:
			 * If key went into the editable, we check if we need to prompt
			 */
			if (!(fs->rtflags & FS_RENAME_FILE) && (fr.flags & FMDF_EDIT))
			{
				fs->tfile = true;
				fs->kbdnav = true;
				if( *fs->file && !strchr( "*[|?", *fs->file ) )
				{
					strcpy( fs->ofile, fs->file );
				}
				fs_prompt(list, fs->file, true);
			}
		}
	}
	return true;
}
#if __GNUC_PREREQ(10,0)
#pragma GCC pop_options
#endif

static void set_edit_width( OBJECT *obtree )
{
	/* temporary: set edit-field-width to max. window-width */
	obtree[FS_FILE ].ob_width = obtree->ob_width - 32;
	if( obtree[FS_FILE ].ob_width > fs_data.fs_file_w )
		obtree[FS_FILE ].ob_width = fs_data.fs_file_w;
}

/* HR: make a start */
/* dest_pid, msg, source_pid, mp3, mp4,  .... 	 */
static void
fs_msg_handler(
	struct xa_window *wind,
	struct xa_client *to,
	short amq, short qmf,
	short *msg)
{
	int lock = 0;
	struct scroll_info *list = object_get_slist(get_widget(wind, XAW_TOOLBAR)->stuff.wt->tree + FS_LIST);
	struct fsel_data *fs = list->data;

	switch (msg[0])
	{
		case MN_SELECTED:
		{
			OBJECT *obtree = fs->menu->tree;
			if (msg[3] == FSEL_OPTS)
			{
				short obj = msg[4], sort;
				if (obj == FSM_RTBUILD)
				{
					if (obtree[FSM_RTBUILD].ob_state & OS_CHECKED)
						fs->rtbuild = false;
					else
						fs->rtbuild = true;
					obtree[FSM_RTBUILD].ob_state ^= OS_CHECKED;
				}
				else if (obj == FSM_TREEVIEW)
				{
					if (obtree[FSM_TREEVIEW].ob_state & OS_CHECKED)
					{
						list->set(list, NULL, SESET_TREEVIEW, 0, NOREDRAW);
						fs->treeview = false;
					}
					else
					{
						list->set(list, NULL, SESET_TREEVIEW, 1, NOREDRAW);
						fs->treeview = true;
					}
					obtree[FSM_TREEVIEW].ob_state ^= OS_CHECKED;
				}
				else
				{
					sort = obj - FSM_SORTBYNAME;
					if (sort >= 0 && sort < 5)
					{
						obtree[FSM_SORTBYNAME + fs->sort].ob_state &= ~OS_CHECKED;
						obtree[FSM_SORTBYNAME + sort		].ob_state |= OS_CHECKED;
						fs->sort = sort;
					}
					else if( sort == 6 )
					{
						fs_data.SortDir = fs_data.SortDir == UP ? DOWN : UP;
						refresh_filelist(LOCK_FSEL, fs, NULL);
						list->redraw(list, NULL);
						fs_prompt( list, fs->file, true );
					}
				}

				obtree[FSEL_OPTS].ob_state &= ~OS_SELECTED;
				display_widget(lock, fs->wind, get_widget(fs->wind, XAW_MENU), NULL);
				fs->selected_dir = NULL;
				fs->selected_file = NULL;
				set_file(fs, fs->ofile, false);
			}
			else if (msg[3] == FSEL_FILTER)
			{
				fs_change(lock, fs, fs->menu->tree, msg[4], FSEL_FILTER, FSEL_PATA, fs->fs_pattern);
				fs->selected_dir = NULL;
				fs->selected_file = NULL;
				/* put pattern into edit-field */
				strcpy( fs->file, fs->fs_pattern );
				set_file(fs, fs->file, false);
			}
			else if (msg[3] == FSEL_DRV)
			{
				int drv;
				fs_change(lock, fs, fs->menu->tree, msg[4], FSEL_DRV, FSEL_DRVA, fs->root);
				inq_xfs(fs, fs->root, fs->fslash);
				add_slash(fs->root, fs->fslash);
				drv = get_drv(fs->root);
				if (fs_data.fs_paths[drv][0])
					strcpy(fs->root, fs_data.fs_paths[drv]);
				else
					strcpy(fs_data.fs_paths[drv], fs->root);
				/* remove the name from the edit field on drive change */
				set_file(fs, fs->ofile, false);
				fs->selected_dir = fs->selected_file = NULL;
			}
			else if (msg[3] == FSEL_FILE)
			{
				struct scroll_entry *e = get_selected(list);
				struct sesetget_params p = {-1};
				switch( msg[4] )
				{
					/* Ctrl-c: copy path/file to clipbrd */
					case FSM_COPY:
						if( *fs->path )
						{
							char t[256];
							int l = strlen( fs->path )-1;
							strcpy( t, fs->path );
							if( !(t[l] == '/' || t[l] == '\\' ) )
								t[l+1] = fs->fslash[0];
							l++;
							if( e )
							{
								list->get(list, e, SEGET_TEXTPTR, &p);
								strcpy( t+l, p.ret.ptr );
							}
							copy_string_to_clipbrd( t );
						}
					break;

					case FSM_RENAME:
						fs->rtflags |= FS_RENAME_FILE;
						strcpy( fs->ofile, fs->file );
						if( fs->selected_file )
							list->cur = fs->selected_file;
						else
							list->cur = fs->selected_dir;
						list->wt->focus = aesobj(fs->form->tree, FS_FILE);

					break;
					case FSM_VIEW:
						if( *fs->file)
						{
							struct xa_client *client = wind->owner;
							char pt[PATH_MAX];

							if( e && !(e->usr_flags & (FLAG_DIR|FLAG_LINK|FLAG_SDIR)) )
							{
								list->get(list, e, SEGET_TEXTPTR, &p);
							}
							else
							{
								break;
								/*
							fs_slist_key(list, SC_UPARROW, 0);
							fs_slist_key(list, SC_DNARROW, 0);
							p.ret.ptr = fs->file;
							*/
							}
							/* this blocks until file is in window! -> post_cevent, speed up add_scroll_entry */
							sprintf( pt, sizeof(pt)-1, "%s%s", fs->path, (char *)p.ret.ptr );
							client->status |= CS_USES_ABOUT;
							open_about( lock, client, true, pt );
							client->status &= ~CS_USES_ABOUT;
						}
					break;

					case FSM_NEW:
						fs->rtflags |= FS_CREATE_FOLDER;
					break;
					case FSM_DELETE:
						delete_item(list, fs);
					break;
				}

				obtree[FSEL_FILE].ob_state &= ~OS_SELECTED;
				display_widget(lock, fs->wind, get_widget(fs->wind, XAW_MENU), NULL);
				return;
			}

			set_dir(list);
			refresh_filelist(lock, fs, NULL);
			fs_prompt(list, fs->file, false);
			list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
			break;
		}
		case WM_CLOSED:
			close_fileselector( lock, fs );
		break;
		case WM_MOVED:
		{
			msg[6] = fs->wind->r.g_w, msg[7] = fs->wind->r.g_h;
			do_formwind_msg(wind, to, amq, qmf, msg);
			break;
			/* fall through */
		}
	/*	case WM_REPOSED: */
		case WM_SIZED:
		{
			short dh, dw;
			OBJECT *obtree = get_widget(wind, XAW_TOOLBAR)->stuff.wt->tree;
			struct xa_window *lwind;

			dw = msg[6] - wind->r.g_w;
			dh = msg[7] - wind->r.g_h;
			obtree->ob_height += dh;
			obtree->ob_width += dw;

			set_edit_width( obtree );

			obtree[FS_LIST ].ob_height += dh;
			obtree[FS_LIST ].ob_width += dw;
			obtree[FS_OK].ob_y += dh;
			obtree[FS_OK].ob_x += dw;
			obtree[FS_CANCEL].ob_y += dh;
			obtree[FS_CANCEL].ob_x += dw;
			obtree[FS_FNTSIZE].ob_y += dh;

			if ((lwind = list->wi) && lwind->send_message)
			{
				lwind->send_message(0, lwind, NULL, amq, QMF_CHKDUP,
					WM_SIZED, 0,0, lwind->handle,
					lwind->r.g_x, lwind->r.g_y, lwind->r.g_w + dw, lwind->r.g_h + dh);
			}
			move_window(0, wind, false, -1, msg[4], msg[5], msg[6], msg[7]);
			set_toolbar_coords(wind, NULL);

			break;
		}
		default:
		{
			do_formwind_msg(wind, to, amq, qmf, msg);
		}
	}
}

static int
fs_destructor(int lock, struct xa_window *wind)
{
	DIAG((D_fsel,NULL,"fsel destructed"));
	return true;
}

static void
fs_init_menu(struct fsel_data *fs)
{
	OBJECT *obtree = fs->menu->tree;

	if (fs_data.treeview)
		obtree[FSM_TREEVIEW].ob_state |= OS_CHECKED;
	else
		obtree[FSM_TREEVIEW].ob_state &= ~OS_CHECKED;
	fs->treeview = fs_data.treeview;

	if (fs_data.rtbuild)
		obtree[FSM_RTBUILD].ob_state |= OS_CHECKED;
	else
		obtree[FSM_RTBUILD].ob_state &= ~OS_CHECKED;

	obtree[FSM_SORTBYNAME + fs->sort].ob_state |= OS_CHECKED;

	fs->rtbuild = fs_data.rtbuild;
	fs_norm_txt.normal.font_id = cfg.font_id;
	fs_norm_txt.selected.font_id = cfg.font_id;
	fs_norm_txt.highlighted.font_id = cfg.font_id;

	exe_txt.normal.font_id = cfg.font_id;
	exe_txt.selected.font_id = cfg.font_id;
	exe_txt.highlighted.font_id = cfg.font_id;

	dexe_txt.normal.font_id = cfg.font_id;
	dexe_txt.selected.font_id = cfg.font_id;
	dexe_txt.highlighted.font_id = cfg.font_id;

	dir_txt.normal.font_id = cfg.font_id;
	dir_txt.selected.font_id = cfg.font_id;
	dir_txt.highlighted.font_id = cfg.font_id;
}

/* ignore updatelocks for fsel (does not work) */
#define NONBLOCK_FSEL 0
#if NONBLOCK_FSEL
/* sav-values */
static struct proc *fs_update_lock;	/* wind_update() locks */
static struct proc *fs_mouse_lock;
#endif
/*
 * char *
 * path
 * fs_einpath 	Name of the default access path (absolute) with appended search
 *							mask; after the call it contains the new pathname
 *
 * char *
 * file
 * fs_einsel		Name of the default file; after the call it contains the newly
 *							selected filename
 *
 * short*
 * intout[1]
 * fs_eexbutton Exit button:
 *
 *						 0 = 'Cancel' was selected
 *						 1 = 'OK' was selected
 * char *
 * title
 * fs_elabel		titelline
 *
 * (see http://toshyp.atari.org/008004.htm)
 */
static bool
open_fileselector1(int lock, struct xa_client *client, struct fsel_data *fs,
			 char *path, const char *file, const char *title,
			 fsel_handler *s, fsel_handler *c, void *data)
{
	bool nolist;
	XA_WIND_ATTR kind;
	OBJECT *form = NULL, *menu = NULL;
	struct xa_window *dialog_window = NULL;
	XA_TREE *wt;
	char *pat=0,*pbt;
	struct scroll_info *list;
	struct xa_vdi_settings *v = client->vdi_settings;
	GRECT remember = {0,0,0,0}, or;
	long dmap = 0;

	DIAG((D_fsel,NULL,"open_fileselector for %s on '%s', fn '%s', '%s', %lx,%lx)",
			c_owner(client), path, file, title, (unsigned long)s, (unsigned long)c));

	for( dmap = 0; dmap < 5 && client->fmd.lock; dmap++ )
	{
		release_blocks(client);
	}
	if( client->fmd.lock )
	{
		BLOG((0,"%s: could not unlock screen for fileselector!", client->name ));
		goto memerr;
	}

	if (fs)
	{
		short dy = 0;
		bzero(fs, sizeof(*fs));

		fs->data = data;
		fs->owner = client;

		form = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, FILE_SELECT), 0);
		if (!form)
			goto memerr;
		fs->form = new_widget_tree(client, form);
		if (!fs->form)
			goto memerr;
		fs->form->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;
		obj_init_focus(fs->form, OB_IF_RESET);

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
			fs->tfile = true;
		}
		else
		{
			fs->file[0] = '\0';
			fs->tfile = false;
		}

		strcpy(fs->ofile, fs->file);


		form[FS_ICONS].ob_flags |= OF_HIDETREE;

		if (path && *path != '\0' && ((*path == '\\' || *path == '/')
			|| (path[1] == ':' && (path[2] == '\\' || path[2] == '/'))))
		{
			char *p, *p1;

			if( *path == '/' ){
				int drv = letter_from_drive(d_getdrv());
				sprintf( fs->root, PATH_MAX, "%c:%s", drv, path );
			}
			else
				strcpy(fs->root, path);

#if BEAUTY_ROOT_NAME
			p = strchr( fs->root, '/' );
			p1 = strchr( fs->root, '\\' );

			if( !p1 || ( p && p < p1) )
			{
				fs->fslash[0] = '/';
				fs->fslash[1] = '\\';
			}
			else{
				fs->fslash[1] = '/';
				fs->fslash[0] = '\\';
			}

			/* beautify root ... */
			for( p = fs->root; (p = strchr(p, fs->fslash[1] )) != NULL; p++ ){
				*p = fs->fslash[0];
			}
			/* get rid of "/../" */
			for( p = fs->root; (p = strstr( p+1, ".." )); )
			{
				if( (*(p-1) == fs->fslash[0]) && (*(p+2) == fs->fslash[0]) )
				{
					p1 = p-2;
					for( ; p1 > fs->root && *p1 != fs->fslash[0]; p1-- );
					if( *p1 == fs->fslash[0] )
					{
						strcpy( p1, p+2 );
						p = p1;
					}
					else
						break;
				}
			}

			fs->fslash[1] = 0;	/* would be handy to keep the other slash but fslash is used as string */
#endif
		}
		else
		{
			int drv = d_getdrv();

			pat = path;

			fs->root[0] = letter_from_drive(drv);
			fs->root[1] = ':';
			d_getcwd(fs->root + 2, drv + 1, sizeof(fs->root));
			if( fs->root[1] == ':' && fs->root[2] == 0 )
			{
				fs->root[2] = '/';
				fs->root[3] = 0;
				fs->fslash[0] = '/';
				fs->fslash[1] = 0;
			}
			else{
				fs->fslash[0] = '\\';
				fs->fslash[1] = 0;
			}
			/* */
		}
		if( !pat )
		{
			pat = strrchr(fs->root, '\\');
			pbt = strrchr(fs->root, '/');
			if (!pat) pat = pbt;
			if( pat )
			{
				if( strmchr( pat, "*!?[" ) )
				{
					*pat++ = 0;
				}
				else
					pat = 0;
			}
		}
		if( !pat || !*pat )
			pat = "*";

#if 0
		if( pat && *pat && !strrchr(pat, '*'))
		{
			if ((chr = fs->root[2]))
			{
				cwdl = strlen(fs->root);
				if (fs->root[cwdl - 1] != chr)
				{
					fs->root[cwdl] = chr;
					fs->root[cwdl+1] = '\0';
				}
			}
			else
				fs->root[2] = fs->fslash[0]/*'\\'*/, fs->root[3] = '\0';
		}
#endif

		/* Strip out the pattern description */
		fs->fs_pattern[0] = '*';
		fs->fs_pattern[1] = '\0';
		fs->fcase = 0;
		if( !pat )
		{
			pat = strrchr(fs->root, '\\');
			pbt = strrchr(fs->root, '/');
			if (!pat) pat = pbt;
			if( pat )
				*pat++ = 0;

		}

		if (pat)
		{
			if( !*pat )
				pat = "*";
			fsel_filters(fs->menu->tree, pat );
			strcpy(fs->fs_pattern, pat);
			strcpy(fs->fs_origpattern, fs->fs_pattern);

			/* if TOS-domain client passes all upper-case pattern make it caseinsensitive */
			if( client->p->domain == 0 )
			{
				bool had_alpha = false;
				if( fs->fs_pattern[0] == '*' && fs->fs_pattern[1] == '.' )
				{
					char *p = fs->fs_pattern + 2;
					if( *p == '[' )
						p++;
					for( ; *p; p++ )
					{
						if( isalpha(*p) )
						{
							had_alpha = true;
							if( !isupper( *p ) )
								break;
						}
					}
					/* pattern is uppercase: make it caseinsensitive */
					if( had_alpha &&  (!*p || *p == ']') )
					{
						fs->fcase = FS_PATNOCASE;
					}
				}
			}
		}


		if( strchr( fs->file, '*' ) ){
			*fs->file = 0;
		}
		else if( !strnccmp( fs->file, fs->root, strlen(fs->file), "/\\" ) )
		{ 	/* in case some buggy app gives "path/file/pattern" in path */
			char *p = strrchr( fs->root, fs->fslash[0] );
			if( p )
			{
				if( !*(p+1) ){
					*p = 0;
					p = strrchr( fs->root, fs->fslash[0] );
				}
				if( p ){
					*p = 0;
					strcpy( fs->file, p+1 );
				}
			}
		}
		{
			int drv = get_drv(fs->root);
			if (drv >= 0)
				strcpy(fs_data.fs_paths[drv], fs->root);
		}

		strcpy(fs->path, fs->root);

		{
			short dh, dw;

			if( fs_data.fs_width == 0 )
			{
				/* first open/not initialized */
				dh = screen.r.g_h - 7 * screen.c_max_h - form->ob_height - 18;
				dw = root_window->wa.g_w - (form->ob_width + (screen.c_max_w * 4));
				if ((dw + form->ob_width) > 560)
					dw = 560 - form->ob_width;

				form->ob_width += dw;
				form->ob_height += dh;
				form[FS_LIST ].ob_height = (form[FS_LIST ].ob_height / screen.c_max_h) * screen.c_max_h - 8;
			}
			else
			{
				dy = screen.c_max_h * fs_data.fs_num;
				if( fs_data.fs_height + fs_data.fs_y + dy > screen.r.g_h )
					fs_data.fs_height = screen.r.g_h - (fs_data.fs_y + dy);

				dw = fs_data.fs_width - form->ob_width;
				dh = fs_data.fs_height - form->ob_height;

				form->ob_width = fs_data.fs_width;
				form->ob_height = fs_data.fs_height;
				form->ob_x = fs_data.fs_x;
				form->ob_y = fs_data.fs_y;
			}
			if( fs_data.fs_file_w == 0 )
				fs_data.fs_file_w = form[FS_FILE ].ob_width;

			set_edit_width( form );

			form[FS_LIST ].ob_height += dh;
			form[FS_LIST ].ob_width += dw;
			form[FS_OK].ob_y += dh;
			form[FS_OK].ob_x += dw;
			form[FS_CANCEL].ob_y += dh;
			form[FS_CANCEL].ob_x += dw;
			form[FS_FNTSIZE].ob_y += dh;
			fs_data.fs = fs;
		}

		obj_rectangle(fs->form, aesobj(form, 0), &or);

		kind = (XaMENU|NAME|TOOLBAR|BORDER);
		/* Work out sizing */
		{
			if( fs_data.fs_x == 0 )
			{
				fs->point = cfg.xaw_point;
				center_rect(&or);
				or.g_y = 64;
			}
			else
			{
				or.g_x = fs_data.fs_x + screen.c_max_w * fs_data.fs_num;
				or.g_y = fs_data.fs_y + dy;
				fs->point = fs_data.fs_point;
			}

			remember =
			calc_window(lock, client, WC_BORDER,
						kind, created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						&or);
			if( fs_data.fs_width )
			{
				short dw = remember.g_w - fs_data.fs_width;
				remember.g_w = fs_data.fs_width;
				form[FS_LIST].ob_width -= dw;
				form[FS_OK].ob_x -= dw;
				form[FS_CANCEL].ob_x -= dw;
			}
			fs->sort = fs_data.fs_sort;
		}

#if !NONBLOCK_FSEL
		if ((C.update_lock == client->p ||
				C.mouse_lock	== client->p))
		{
#else
			fs_update_lock = C.update_lock;
			fs_mouse_lock	= C.mouse_lock;
			fs_data.aes_has_fsel = client;
			C.update_lock = 0;
			C.mouse_lock	= 0;
#endif
#if !NONBLOCK_FSEL
			nolist = true;
			kind |= STORE_BACK;
		}
#endif
#if !NONBLOCK_FSEL
		else
#endif
		{
			kind |= hide_move(&(client->options));
			nolist = false;
		}

		/* Create the window */
		dialog_window = create_window(	lock,
						do_winmesag,
						fs_msg_handler,
						client,
						nolist,
						kind,
						created_for_AES,
						C.Aes->options.thinframe,
						C.Aes->options.thinwork,
						&remember,
						NULL,
						NULL);

		if (!dialog_window)
			goto memerr;

		dialog_window->opts &= ~XAWO_WCOWORK;
		fix_menu(client, fs->menu, dialog_window, false);
		fs_init_menu(fs);

		/* Set the window title */
		/* XXX - pointer into user space, correct here? */
		/*	 ozk: no, absolutely not right here! */
		set_window_title(dialog_window, title, true);

		set_menu_widget(dialog_window, client, fs->menu);

		fs->drives = fsel_drives(fs->menu->tree,
					*(fs->root+1) == ':' ? drive_from_letter(*fs->root) : d_getdrv(), &dmap);

		fs->curr_longest = 0;

		wt = set_toolbar_widget(lock, dialog_window, client, form, aesobj(form, FS_FILE), 0/*WIP_NOTEXT*/, STW_ZEN, NULL, &or);
		obj_edit(fs->form, v, ED_SETPTEXT, aesobj(fs->form->tree, FS_FILE), sizeof(fs->file) - 1, 0, fs->file, false, NULL,NULL, NULL,NULL);
		obj_edit(fs->form, v, ED_MARK, aesobj(fs->form->tree, FS_FILE), 0, -1, NULL, false, NULL,NULL, NULL,NULL);
		obj_edit(fs->form, v, ED_STRING, aesobj(fs->form->tree, FS_FILE), 0, 0, fs->file, false, NULL, NULL, NULL, NULL);

		/* This can be of use for drawing. (keep off border & outline :-) */
		wt->zen = true;
		wt->exit_form = fileselector_form_exit;

		/* HR: We need to do some specific things with the key's,
		 *		 so we supply our own handler,
		 */
		dialog_window->keypress = fs_key_form_do;
		dialog_window->min.g_h = 222;
		dialog_window->min.g_w = fs->menu->area.g_x + fs->menu->area.g_w + 32;

		/* HR: set a scroll list object */
		list = set_slist_object(lock,
				 wt,
				 dialog_window,
				 FS_LIST,
				 SIF_SELECTABLE | SIF_ICONINDENT | (fs->treeview ? SIF_TREEVIEW : 0) | SIF_AUTOSLIDERS | SIF_ICONS_HAVE_NO_TEXT,
				 fs_closer, NULL,
				 /*dclick,							 click		click_nesticon			key */
				 fs_click/*was dclick*/, fs_click, fs_click_nesticon, fs_slist_key,
				 /*add,del,empty,destroy*/
				 NULL, NULL, NULL, NULL/*free_scrollist*/,
				 /*title,info,data,lmax*/
				 fs->path, NULL, fs, 30);

		set_tabs(list, fs, 0);

		/* HR: after set_menu_widget (fs_destructor must cover what is in menu_destructor())
		 *		 Set the window destructor
		 */
		dialog_window->destructor = fs_destructor;

		strcpy(fs->filter, fs->fs_pattern);
		fs->wind = dialog_window;
		open_window(lock, dialog_window, dialog_window->r);
		fs_data.fs_num++;

		/* HR: after set_slist_object() & opwn_window */
		/* refresh_filelist(lock, fs, 0); */
		/* we post this as a client event, so it does not happen before the fsel is drawn... */
		post_cevent(client, CE_refresh_filelist, fs, list, 0, 0, NULL, NULL);

		if( fs->tfile == true ){
			/*
			 * try to set focus to pre-selected item
			 *
			 */

			struct xa_aes_object o = aesobj(list->wt->tree,FS_LIST);

			if( o.ob && o.ob->ob_type == G_SLIST )
				list->wt->focus = o;
		}

		fs_set_shortcut_letter( wt->tree + FS_CANCEL, dmap );
		fs_set_shortcut_letter( wt->tree + FS_OK, dmap );


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

void fs_save(struct fsel_data *fs)
{
	if( !fs )
		fs = fs_data.fs;
	if( !fs )
		return;
	fs_data.rtbuild = fs->rtbuild;
	fs_data.treeview = fs->treeview;
	fs_data.fs_height = fs->form->tree->ob_height;
	fs_data.fs_width = fs->form->tree->ob_width;
	fs_data.fs_x = fs->form->tree->ob_x;
	fs_data.fs_y = fs->form->tree->ob_y;

	fs_data.fs_point = fs->point;
	fs_data.fs_sort = fs->sort;
}
void
close_fileselector(int lock, struct fsel_data *fs)
{
	fs_save(fs);
	if( !fs || !fs->wind )
		return;
	close_window(lock, fs->wind);
	delete_window(lock, fs->wind);

	fs_data.fs_num--;
	fs->wind = NULL;
	fs->menu = NULL;
	fs->form = NULL;
	fs->selected = NULL;
	fs->canceled = NULL;

	if( fs->owner == fs_data.aes_has_fsel )
		fs_data.aes_has_fsel = 0;

	fs_data.fs = 0;
	/* force font-sz-init on next call */

#if NONBLOCK_FSEL
	C.update_lock  = fs_update_lock;
	C.mouse_lock = fs_mouse_lock;
#endif
}

static void
handle_fsel(int lock, struct fsel_data *fs, const char *path, const char *file)
{
	DIAG((D_fsel, NULL, "fsel OK: path%s, file=%s:%s", path, file, fs->file));

	PROFILE(( "fsel: handle_fsel" ));
	close_fileselector(lock, fs);
	fs->ok = 1;
	fs->owner->usr_evnt = 1;
}

static void
cancel_fsel(int lock, struct fsel_data *fs, const char *path, const char *file)
{
	DIAG((D_fsel, NULL, "fsel CANCEL: path=%s, file=%s", path, file));

	close_fileselector(lock, fs);
	fs->ok = 0;
	fs->owner->usr_evnt = 1;
}

void
open_fileselector(int lock, struct xa_client *client, struct fsel_data *fs,
			char *path, const char *file, const char *title,
			fsel_handler *s, fsel_handler *c, void *data)
{
	if (!fs_data.aes_has_fsel)
	{
		if( client == C.Hlp )
			fs_data.aes_has_fsel = C.Hlp;
		open_fileselector1(lock, client, fs, path, file, title, s, c, data);
	}
	else
		top_window( lock, true, true, fs->wind );

}

/*
 * File selector interface routines
 */
static void
do_fsel_exinput(int lock, struct xa_client *client, AESPB *pb, const char *text)
{
	char *path = (char *)(pb->addrin[0]);
	char *file = (char *)(pb->addrin[1]);
	struct fsel_data *fs;

	pb->intout[0] = 0;

#if KERNELSTRCMP
	if( strcmp == 0 )
		strcmp = (*KENTRY->vec_libkern.strcmp);
#endif
	fs = kmalloc(sizeof(*fs));

	if (fs)
	{
		int font_point = client->options.standard_font_point;

		DIAG((D_fsel, NULL, "fsel_(ex)input: title=%s, path=%s, file=%s, fs=%lx",
			text, path, file, (unsigned long)fs));

		client->options.standard_font_point = cfg.standard_font_point;
		if (open_fileselector1( lock|LOCK_FSEL,
					client,
					fs,
					path,
					file,
					text,
					handle_fsel,
					cancel_fsel, NULL))
		{
			client->status |= CS_FSEL_INPUT;
			(*client->block)(client);
			client->status &= ~CS_FSEL_INPUT;
			pb->intout[0] = 1;
			if ((pb->intout[1] = fs->ok))
			{
#if BEAUTY_ROOT_NAME

				char *p;
				int drv;
				/*
				 * if path-separator was changed, revert change
				 */
				if( *fs->path ){
					p = fs->path + strlen(fs->path)-1;
					if( *p == '/' )
						*p = '\\';
				}
				if( *fs->path == '/' && (drv = letter_from_drive(d_getdrv())) != 'U' )
					sprintf( path, PATH_MAX, "/%c%s", drv, fs->path );
				else
#endif
					strcpy(path, fs->path);
				strcat(path, fs->fs_origpattern);
				strcpy(file, fs->file);
			}
		}
		kfree(fs);
		client->options.standard_font_point = font_point;
	}
}

unsigned long
XA_fsel_input(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,2,2)
	short msave;
	MFORM *save;

	msave = client->mouse;
	save = client->mouse_form;
	set_client_mouse(client, SCM_MAIN|0x8000, ARROW, NULL);
	showm();
	do_fsel_exinput(lock, client, pb, "");
	set_client_mouse(client, SCM_MAIN|0x8000, msave, save);
	xa_graf_mouse(client->mouse, client->mouse_form, client, false);

	return XAC_DONE;
}

unsigned long
XA_fsel_exinput(int lock, struct xa_client *client, AESPB *pb)
{
	const char *t = (const char *)(pb->addrin[2]);
	short msave;
	MFORM *save;

	CONTROL(0,2,3)

	msave = client->mouse;
	save = client->mouse_form;
	set_client_mouse(client, SCM_MAIN, ARROW, NULL);

	if (pb->control[N_ADDRIN] <= 2 || t == NULL)
		t = "";

	showm();
	do_fsel_exinput(lock, client, pb, t);

	set_client_mouse(client, SCM_MAIN, msave, save);
	xa_graf_mouse(client->mouse, client->mouse_form, client, false);

	return XAC_DONE;
}
