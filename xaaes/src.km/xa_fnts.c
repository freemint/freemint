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

#include "xa_fnts.h"
#include "xa_wdlg.h"
#include "xa_global.h"

#include "k_main.h"
#include "k_mouse.h"

#include "app_man.h"
#include "form.h"
#include "obtree.h"
#include "draw_obj.h"
#include "c_window.h"
#include "rectlist.h"
#include "widgets.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_form.h"
#include "scrlobjc.h"

#include "xa_user_things.h"
#include "nkcc.h"
#include "mint/signal.h"


/*
 * WDIALOG FUNCTIONS (fnts)
 *
 * documentation about this can be found in:
 *
 * - the GEMLIB documentation:
 *   http://arnaud.bercegeay.free.fr/gemlib/
 *
 * - the MagiC documentation project:
 *   http://www.bygjohn.fsnet.co.uk/atari/mdp/
 */

#if WDIALOG_FNTS

struct spd_trnfmd
{
	short	y_pos;
	short	x_scale;
	short	y_scale;
};
typedef struct spd_trnfmd SPD_TRNFMD;

struct speedo_header
{
	char	fmver[8];
	long	font_size;
	long	min_fontbuff_size;
	short	min_charbuff_size;
	short	header_size;
	short	src_font_id;
	short	src_font_version;
	char	src_font_name[70];
	char	mdate[10];
	char	layout_name[70];
	char	copyright[78];
	short	layout_size;
	short	nchars;
	short	first_char;
	short	kern_tracks;
	short	kern_pairs;
	char	flags;
	char	class_flags;
	char	family_flags;
	char	fontform_class_flags;
	char	short_fontname[32];
	char	short_facename[16];
	char	font_form[14];
	short	italic_angle;
	short	oru_per_em;
	short	width_wordspace;
	short	width_enspace;
	short	width_thinspace;
	short	width_figspace;
	short	fontwide_minx;
	short	fontwide_miny;
	short	fontwide_maxx;
	short	fontwide_maxy;
	short	uline_pos;
	short	uline_thickness;
	
	SPD_TRNFMD	small_caps;
	SPD_TRNFMD	display_sups;
	SPD_TRNFMD	footnote_sups;
	SPD_TRNFMD	alpha_sups;
	SPD_TRNFMD	chemical_infs;
	SPD_TRNFMD	small_nums;
	SPD_TRNFMD	small_denoms;
	SPD_TRNFMD	medium_nums;
	SPD_TRNFMD	medium_denoms;
	SPD_TRNFMD	large_nums;
	SPD_TRNFMD	large_denoms;
};

struct co_display_parms
{
	short	x, y;
	RECT	*clip;
	long	id, pt, ratio;
	char	*txt;
};

static short
xvst_load_fonts(XVDIPB *vpb, short select, short handle)
{
	vpb->intin[0] = select;
	VDI(vpb, 119, 0, 1, 0, handle);
	return vpb->intout[0];
}
//callout_display(short x, short y, RECT *clip, long id, long pt, long ratio, char *txt)
static void
callout_display(struct xa_fnts_item *f, short vdih, long pt, long ratio, RECT *clip, RECT *area, char *txt)
{
	if (f)
	{
		if (f->f.display)
		{
			struct sigaction oact, act;
			struct xa_callout_head *u;

			u = umalloc(xa_callout_user.len + sizeof(struct co_display_parms) + sizeof(*clip) + strlen(txt) + 1);
			if (u)
			{
				struct xa_callout_parms *p;
				struct co_display_parms *wp;

				bcopy(&xa_callout_user, u, xa_callout_user.len);
			
				u->sighand_p	+= (long)u;
				u->parm_p	 = (void *)((char *)u->parm_p + (long)u);

				p	= u->parm_p;
				p->func	= (long)f->f.display;
				p->plen	= sizeof(struct co_display_parms) >> 1;
			
				wp	= (struct co_display_parms *)(&p->parms);

				wp->clip = (RECT *)((long)wp + sizeof(*wp));
				*wp->clip = *clip;
				wp->txt = (char *)((long)wp + sizeof(*wp) + sizeof(*clip));
				strcpy(wp->txt, txt);
				
				wp->x		= area->x;
				wp->y		= area->y + area->h - 2;
				wp->id		= f->f.id;
				wp->pt		= pt;
				wp->ratio	= ratio;
				
				cpushi(u, xa_callout_user.len);

				act.sa_handler	= u->sighand_p;
				act.sa_mask	= 0xffffffff;
				act.sa_flags	= SA_RESET;

				p_sigaction(SIGUSR2, &act, &oact);
				DIAGS(("raise(SIGUSR2)"));
				raise(SIGUSR2);
				DIAGS(("handled SIGUSR2 fnts->display callout"));
				/* restore old handler */
				p_sigaction(SIGUSR2, &oact, NULL);

				ufree(u);
			}
		}
		else
		{
			short x, y, w, h;
			short c[8];

			vst_color(vdih, 1);
			vs_clip(vdih, 1, (short *)clip);
			vst_alignment(vdih, 0, 5, &x, &x);
			vst_font(vdih, f->f.id);
			vst_point(vdih, pt >> 16, &x, &x, &x, &x);

			vqt_extent(vdih, txt, c);

			w = c[4] - c[6];
			h = -(c[1] - c[7]);

			x = area->x + (area->w >> 1);
			y = area->y + (area->h >> 1);
			
			y -= (h >> 1);
			x -= (w >> 1);

			if (x < area->x)
				x = area->x;
			if (y < area->y)
				y = area->y;
			
			v_gtext(vdih, x, y, txt);
		}
	}
}

static void
fnts_extb_callout(struct extbox_parms *p)
{
	struct xa_fnts_info *fnts = p->data;
	struct xa_fnts_item *f = fnts->fnts_selected;

	if (f)
	{
		callout_display(f, fnts->vdi_handle, fnts->fnt_pt, fnts->fnt_ratio, &p->clip, &p->r, fnts->sample_text);
	}
}

static struct speedo_header *
xvqt_fontheader(XVDIPB *vpb, short handle)
{
	struct speedo_header *hdr;

	hdr = kmalloc(sizeof (*hdr));
	if (hdr)
	{
		*(struct speedo_header **)&vpb->intin[0] = hdr;
		VDI(vpb, 232, 0, 2, 0, handle);
	}
	return hdr;
}

static struct xa_fnts_info *
new_fnts(void)
{
	struct xa_fnts_info *new;

	new = kmalloc(sizeof(*new));
	if (new)
	{
		bzero(new, sizeof(*new));
	}
	return new;
}
/*
 * This function is pointed to by xa_data_hdr->destruct, and is called
 * by delete_xa_data() and free_xa_data_list() (in global.c).
 */
static void _cdecl
delete_fnts_info(void *_fnts)
{
	struct xa_fnts_info *fnts = _fnts;

	if (fnts)
	{
		struct xa_fnts_item *f = fnts->fnts_ring;
		DIAG((D_fnts, NULL, "delete_fnts_info: fnts=%lx", fnts));

		/*
		 * Release all fnt items...
		 */
		while (f)
		{
			struct xa_fnts_item *n = f->link;
			DIAG((D_fnts, NULL, " --- deleting fnts_item=%lx (%s)", f, f->f.full_name));
			kfree(f);
			f = n;
		}
		fnts->fnts_list = NULL;
		fnts->fnts_ring = NULL;
		fnts->fnts_selected = NULL;
		if (fnts->sample_text)
		{
			DIAG((D_fnts, NULL, " --- deleting sample_text '%s'", fnts->sample_text));
			kfree(fnts->sample_text);
		}
		if (fnts->opt_button)
		{
			DIAG((D_fnts, NULL, " --- deleting opt_button '%s'", fnts->opt_button));
			kfree(fnts->opt_button);
		}

		if (fnts->wt)
		{
			fnts->wt->links--;
			remove_wt(fnts->wt, false);
		}

		kfree(fnts);
	}
}

/*
 * Lookup window with fnts_ptr in wind->data
 */

static struct xa_window *
get_fnts_wind(struct xa_client *client, void *fnts_ptr)
{
	struct xa_fnts_info *fnts = lookup_xa_data(&client->xa_data, fnts_ptr);
	
	if (fnts)
		return fnts->wind;
	else
		return NULL;
}


static short pt_sizes[] = 
{
	4,   5,  6,  7,  8,  9, 10, 11, 12, 13,
	14, 15, 16, 18, 20, 24, 28, 36, 48, 72
};

static void
add_point(struct xa_fnts_item *f, short point)
{
	int i;
	bool dup = false;

	for (i = 0; i < f->f.npts; i++)
	{
		if (f->f.pts[i] == point)
		{
			dup = true;
			break;
		}
	}
	if (!dup)
	{
		f->f.pts[f->f.npts++] = point;
	}
}

/*
 * Fill 'fitem' with info about 'index' font
 */
static void
xvqt_name(struct xa_fnts_info *fnts, XVDIPB *vpb, short index, struct xa_fnts_item *fitem)
{
	int i;
	
	/*
	 * vqt_name()
	 */
	vpb->intin[0] = index;
	VDI(vpb, 130, 0, 2, 1, fnts->vdi_handle);

	fitem->f.id	= vpb->intout[0];
	fitem->f.index	= index;

	get_vdistr(fitem->f.full_name, &vpb->intout[1], 32);
	*fitem->f.family_name = '\0';
	*fitem->f.style_name = '\0';
	
	if (vpb->control[4] <= 33)
	{
		fitem->f.outline = 0;
		fitem->f.mono = 1;
	}
	else if (vpb->control[4] == 34)
	{
		struct speedo_header *hdr;

		fitem->f.outline = vpb->intout[33] ? 1 : 0;
		fitem->f.mono = 1;

		xvst_font(vpb, fnts->vdi_handle, fitem->f.id);
		
		if ((hdr = xvqt_fontheader(vpb, fnts->vdi_handle)))
		{
			fitem->f.mono = hdr->flags & 1 ? 1 : 0;
			kfree(hdr);
		}
	}
	else
	{
		fitem->f.outline = ((vpb->intout[34] & 0xff) != 1)  ? 1 : 0;
		fitem->f.mono = (vpb->intout[34] & 0xff00) ? 1 : 0;	
	}

	/*
	 * Scan for available point sizes
	 */
	fitem->f.npts = 0;
	xvst_font(vpb, fnts->vdi_handle, fitem->f.id);
	for (i = 0; i < sizeof(pt_sizes); i++)
	{
		add_point(fitem, xvst_point(vpb, fnts->vdi_handle, pt_sizes[i]));
	}

	/*
	 * Get family/style names -- only possible when vqt_xfntinfo() is available.
	 */
	if (C.nvdi_version >= 0x0302)
	{
		XFNT_INFO *x;

		if ((x = xvqt_xfntinfo(vpb, fnts->vdi_handle, 0xffff/*((1<<1)|(1<<2)|(1<<8))*/, fitem->f.id, 0)))
		{
			strcpy(fitem->f.family_name, x->family_name);
			strcpy(fitem->f.style_name, x->style_name);
			kfree(x);
		}
	}
	/*
	 * Sort the point sizes..
	 */
	{
		int j, n = fitem->f.npts;
		char tmp, *a = fitem->f.pts;

		if (n > 1)
		{
			for (i = 0; i < n - 1; i++)
			{
				for (j = 0; j < n - 1 - i; j++)
				{
					if (a[j + 1] < a[j])
					{
						tmp = a[j];
						a[j] = a[j+1];
						a[j+1] = tmp;
					}
				}
			}
		}
	}
}

static struct xa_fnts_item *
new_fnts_item(void)
{
	struct xa_fnts_item *new;

	if ((new = kmalloc(sizeof(*new))))
	{
		bzero(new, sizeof(*new));
		new->f.full_name   = new->font_name;
		new->f.family_name = new->family_name;
		new->f.style_name  = new->style_name;
		new->f.pts	   = new->sizes;
	}
	return new;
}

static void
link_fnts_item(struct xa_fnts_item **list, struct xa_fnts_item *f)
{
	struct xa_fnts_item *fl = *list;

	while (fl && fl->link)
		fl = fl->link;

	if (fl)
		fl->link = f;
	else
		*list = f;
}

static void
unlink_fnts_item(struct xa_fnts_item **list, struct xa_fnts_item *f)
{
	while (*list)
	{
		if (*list == f)
		{
			*list = f->link;
			DIAG((D_fnts, NULL, "unlink_fnts_item: freeing %lx (%s)", f, f->f.full_name));
			kfree(f);
			break;
		}
		list = &((*list)->link);
	}
}

static void
add_fnts_item(struct xa_fnts_item **list, struct xa_fnts_item *f, short flags)
{
	struct xa_fnts_item *fl = *list, *last = NULL;
	
	f->nxt_family = f->nxt_kin = NULL;
	DIAG((D_fnts, NULL, "add_fnts_item: list=%lx, f=%lx, flags=%x", *list, f, flags));
	
	if ((!(flags & FNTS_BTMP) && !f->f.outline) ||
	    (!(flags & FNTS_OUTL) && f->f.outline) ||
	    (!(flags & FNTS_MONO) && !f->f.mono) ||
	    (!(flags & FNTS_PROP) && f->f.mono))
	{
		DIAG((D_fnts, NULL, " --- ignoring - outline=%s, mono=%s",
			f->f.outline ? "Yes":"No", f->f.mono ? "Yes":"No"));

		return;
	}

	if (*f->f.family_name == '\0')
	{
		struct xa_fnts_item **l;
		l = list;
		while (*l)
		{
			l = &((*l)->nxt_family);
		}
		*l = f;
		return;
	}

	while (fl)
	{
		if (*fl->f.family_name != '\0' && !strcmp(fl->f.family_name, f->f.family_name))
		{
			struct xa_fnts_item *l = fl;
			while (l)
			{
				fl = l;
				l = l->nxt_kin;
			}
			fl->nxt_kin = f;
			break;
		}
		last = fl;
		fl = fl->nxt_family;
	}
	
	if (!fl)
	{
		if (last)
			last->nxt_family = f;
		else
			*list = f;
	}
}

static struct xa_fnts_item *
get_font_items(struct xa_fnts_info *fnts)
{
	short i;
	XVDIPB *vpb;
	struct xa_fnts_item *fitem = NULL, *last = NULL, *new;
	
	DIAGS(("get_font_items:"));
	vpb = create_vdipb();
	DIAGS(("get_font_items: create vdipb=%lx", vpb));
	
	if (fnts->vdi_handle && !fnts->fnts_loaded)
	{
		DIAGS(("get_font_items: gdos version = %lx, calling vst_load_fonts(%d, 0)", C.gdos_version, fnts->vdi_handle));
		fnts->fnts_loaded = 1;
		if (C.gdos_version)
		{
			fnts->fnts_loaded += xvst_load_fonts(vpb, 0, fnts->vdi_handle);
// 			fnts->fnts_loaded += vst_load_fonts(fnts->vdi_handle, 0);
		}
		DIAGS(("get_font_items: loaded %d fonts", fnts->fnts_loaded));
	}

	for (i = 1; i < (fnts->fnts_loaded); i++)
	{
		if (!(new = new_fnts_item()))
			break;

		xvqt_name(fnts, vpb, i, new);
		
		if (!fitem)
			fitem = last = new;
		else
		{
			last->link = new;
			last = new;
		}
		
		DIAGS(("get_font_items: ID=%ld, index=%x, %s, %s font", new->f.id, new->f.index,
			new->f.mono ? "Monospaced":"Proportional",
			new->f.outline ? "Outline":"Bitmapped"));

		DIAGS(("get_font_items: name='%s', family='%s', style='%s'",
			new->f.full_name, new->f.family_name, new->f.style_name));
		
	}

	DIAGS(("get_font_items: free vdipb at %lx, returning fitem=%lx", vpb, fitem));

	kfree(vpb);
	
	return fitem;
}

static void
select_edsize(struct xa_fnts_info *fnts)
{
	struct scroll_info *list;
	struct scroll_entry *ent;
	OBJECT *obtree = fnts->wt->tree;
	short redraw;

	list = object_get_slist(obtree + FNTS_POINTS);
	ent = list->search(list, NULL, SEFM_BYTEXT, object_get_tedinfo(obtree + FNTS_EDSIZE, NULL)->te_ptext);
	
	if (list->cur)
	{
		list->set(list, list->cur, SESET_UNSELECTED, 0, true);
		redraw = NORMREDRAW;
	}
	else
		redraw = FULLREDRAW;

	if (ent)
	{
		list->cur = ent;
		list->set(list, ent, SESET_SELECTED, 0, NOREDRAW);
		list->vis(list, ent, redraw);
	}
	else
	{
		list->cur = NULL;
		list->slider(list, true);
		list->redraw(list, NULL);
	}
}

static void
set_points_list(struct xa_fnts_info *fnts, struct xa_fnts_item *f)
{
	int i;
	OBJECT *obtree = fnts->wt->tree;
	struct scroll_info *list = object_get_slist(obtree + FNTS_POINTS);
	char b[8];
	struct scroll_content sc = {{ 0 }};

	list->empty(list, NULL, -1);

	if (f)
	{
		sc.t.strings = 1;
		for (i = 0; i < f->f.npts; i++)
		{
			sc.t.text = b;
			sc.data = f;
			sprintf(b, sizeof(b), "%d", f->f.pts[i]);
			DIAGS(("set_point_list: add '%s'", b));
			list->add(list, NULL, NULL, &sc, false, 0, false);
		}
	}
	
	fnts->fnts_selected = f;
	select_edsize(fnts);

	//list->redraw(list, NULL);

	//fnts_redraw(0, fnts->wind, FNTS_SHOW, 1, NULL);
	wdialog_redraw(0, fnts->wind, aesobj(obtree, FNTS_SHOW), 1, NULL);
}

static void
set_name_list(struct xa_fnts_info *fnts, struct xa_fnts_item *selstyle)
{
	struct xa_fnts_item *f;
	OBJECT *obtree = fnts->wt->tree;
	SCROLL_INFO *list = object_get_slist(obtree + FNTS_FNTLIST);
	SCROLL_INFO *list_type = object_get_slist(obtree + FNTS_TYPE);
	struct scroll_content sc = {{ 0 }};
	struct xa_wtxt_inf wtxt;

	list_type->empty(list_type, NULL, -1);

	if (list->cur)
	{
		list_type->get(list_type, NULL, SEGET_WTXT, &wtxt);
		if (fnts->dialog_flags & FNTS_DISPLAY)
			sc.fnt = &wtxt;
		
		f = list->cur->data;
		if (*f->f.style_name != '\0')
		{
			sc.t.strings = 1;
			while (f)
			{
				wtxt.n.f = wtxt.s.f = wtxt.h.f = f->f.id;
				sc.t.text = f->f.style_name;
				sc.data = f;
				list_type->add(list_type, NULL, NULL, &sc, false, 0, false);
				f = f->nxt_kin;
			}
		}
	}
	
	if (list_type->top)
	{
		list_type->cur = NULL;
		if (selstyle)
		{
			SCROLL_ENTRY *s = list_type->search(list_type, NULL, SEFM_BYDATA, selstyle);
			
			if (s)
			{
				list_type->set(list_type, s, SESET_SELECTED, 0, false);
				list_type->vis(list_type, s, NORMREDRAW);
				list_type->cur = s;
			}
//			else
//				list_type->slider(list_type, true);
		}
		if (!list_type->cur)
			list_type->cur = list_type->top;

		f = list_type->cur->data;
	}
	else if (list->cur)
		f = list->cur->data;
	else
		f = NULL;

	set_points_list(list->data, f);

	list_type->slider(list_type, true);
	list_type->redraw(list_type, NULL);	
}	
static bool
sort_names(struct scroll_info *list, struct scroll_entry *new, struct scroll_entry *this)
{
	char *s1, *s2;
	struct sesetget_params p;

	p.idx = -1;
	list->get(list, new, SEGET_TEXTPTR, &p);
	s1 = p.ret.ptr;
	list->get(list, this, SEGET_TEXTPTR, &p);
	s2 = p.ret.ptr;
	
	return (stricmp(s1, s2) > 0) ? true : false; // stricmp(new->content->c.text.text, this->content->c.text.text) > 0) ? true : false;
}

static void
update_slists(struct xa_fnts_info *fnts)
{
	OBJECT *obtree = fnts->wt->tree;
	struct xa_fnts_item *f;
	SCROLL_INFO *list_name, *list_style;
	struct scroll_content sc = {{ 0 }};
	struct xa_wtxt_inf wtxt;

	list_name  = object_get_slist(obtree + FNTS_FNTLIST);
	list_style = object_get_slist(obtree + FNTS_TYPE);

	list_name->empty(list_name, NULL, -1);
	list_style->empty(list_style, NULL, -1);
	list_name->redraw(list_name, NULL);

	/*
	 * Rebuild font item list according to flags
	 */
	fnts->fnts_list = NULL;
	f = fnts->fnts_ring;
	while (f)
	{
		add_fnts_item(&fnts->fnts_list, f, fnts->font_flags);
		f = f->link;
	}

	list_name->get(list_name, NULL, SEGET_WTXT, &wtxt);

	f = fnts->fnts_list;
	if (fnts->dialog_flags & FNTS_DISPLAY)
		sc.fnt = &wtxt;

	sc.t.strings = 1;
	while (f)
	{
		wtxt.n.f = wtxt.s.f = wtxt.h.f = f->f.id;
		
		sc.t.text = *f->f.family_name != '\0' ? f->f.family_name : f->f.full_name;
		sc.data = f;
		list_name->add(list_name, NULL, sort_names, &sc, false, 0, NORMREDRAW);
		f = f->nxt_family;
	}

	list_name->slider(list_name, true);
	list_name->redraw(list_name, NULL);

	set_name_list(fnts, NULL);

}

/*
 * Called by scroll_object handlers when SLIST object is clicked
 */
static int
click_name(SCROLL_INFO *list, SCROLL_ENTRY *this, const struct moose_data *md)
{
	set_name_list(list->data, NULL);
	return 0;
}

static int
click_style(SCROLL_INFO *list, SCROLL_ENTRY *this, const struct moose_data *md)
{
	struct xa_fnts_item *f;

	if (list->cur)
		f = list->cur->data;
	else
		f = NULL;

	set_points_list(list->data, f);

	return 0;
}

static unsigned long
get_edpoint(struct xa_fnts_info *fnts)
{
	unsigned long val = atol(object_get_tedinfo(fnts->wt->tree + FNTS_EDSIZE, NULL)->te_ptext);
	return (val << 16);
}

	
static int
click_size(SCROLL_INFO *list, SCROLL_ENTRY *this, const struct moose_data *md)
{
	if (list->cur)
	{
		OBJECT *obtree = list->wt->tree;
		struct xa_fnts_info *fnts = list->data;
		TEDINFO *ted = object_get_tedinfo(obtree + FNTS_EDSIZE, NULL);
		struct sesetget_params p; //seget_entrybyarg p;
		struct xa_aes_object object;

		object = aesobj(fnts->wt->tree, FNTS_EDSIZE);
		p.idx = -1;
		p.arg.txt = ted->te_ptext;
		list->get(list, list->cur, SEGET_TEXTCPY, &p);
		
		obj_edit(fnts->wt, fnts->vdi_settings,
		         ED_STRING,
		         object,
		         1,
		         0,
		         ted->te_ptext,
		         true,
		         NULL,
		         ((struct xa_fnts_info *)list->data)->wind->rect_list.start, NULL,NULL);

		fnts->fnt_pt = get_edpoint(fnts);
		
		wdialog_redraw(0, ((struct xa_fnts_info *)list->data)->wind, aesobj(fnts->wt->tree, FNTS_SHOW), 1, NULL);
	}
	return 0;
}

static XA_TREE *
duplicate_fnts_obtree(struct xa_client *client)
{
	OBJECT *obtree = NULL;
	XA_TREE *wt = NULL;

	if ((obtree = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, WDLG_FNTS), 0)))
	{
		wt = new_widget_tree(client, obtree);
		if (wt)
		{
			/*
			 * We let delete_fnts_info free the widget_tree
			 */
			wt->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
			obtree->ob_state &= ~OS_OUTLINED;
			form_center(obtree, ICON_H);
			obj_init_focus(wt, OB_IF_RESET);
		}
		else
			free_object_tree(C.Aes, obtree);
	}
	return wt;
}

static struct xa_fnts_info *
create_new_fnts(enum locks lock,
		struct xa_client *client,
		struct xa_window *wind,
		XA_TREE *wt,
		short vdih,
		short num_fonts,
		short font_flags,
		short dialog_flags,
		char *sample,
		char *opt)
{
	struct xa_fnts_info *fnts;
	char *d;

	if ((fnts = new_fnts()))
	{
		/*
		 * Add fnts structure to the clients xa_data list.
		 * All structures attached here must have xa_data_hdr structure
		 * as its first element for the xa_data_xx() functions to work on.
		 */
		add_xa_data(&client->xa_data, fnts, 0, NULL, delete_fnts_info);

		fnts->wind		= wind;
		fnts->vdi_settings	= wind->vdi_settings;
		fnts->handle		= (void *)((unsigned long)fnts >> 16 | (unsigned long)fnts << 16);
		fnts->wt		= wt;
		wt->links++;
		fnts->vdi_handle	= vdih;
		fnts->num_fonts		= num_fonts;
		fnts->font_flags	= font_flags;
		fnts->dialog_flags	= dialog_flags;
		
		d = NULL;
		if (sample)
		{
			long slen = strlen(sample) + 1;
			d = kmalloc(slen);
			if (d) strcpy(d, sample);
		}
		fnts->sample_text = d;
		d = NULL;
		if (opt)
		{
			long slen = strlen(opt) + 1;
			d = kmalloc(slen);
			if (d) strcpy(d, opt);
		}
		fnts->opt_button = d;
		
		fnts->fnts_ring = get_font_items(fnts);

		/* HR: set a scroll list object */
		set_slist_object(lock,
				 wt,
				 wind,
				 FNTS_FNTLIST,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_KEYBDACT,
				 NULL, NULL,			/* scrl_widget closer, fuller*/
				 NULL, click_name, NULL, NULL,	/* scrl_click dclick, click, click_nesticon, key */
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, fnts, 1);

		set_slist_object(lock,
				 wt,
				 wind,
				 FNTS_TYPE,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_KEYBDACT,
				 NULL, NULL,
				 NULL, click_style, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, fnts, 1);
		
		set_slist_object(lock,
				 wt,
				 wind,
				 FNTS_POINTS,
				 SIF_SELECTABLE|SIF_AUTOSELECT|SIF_KEYBDACT,
				 NULL, NULL,
				 NULL, click_size, NULL, NULL,
				 NULL, NULL, NULL, NULL,
				 NULL, NULL, fnts, 1);

		/* XXX
		 * ozk: Using ob_type == 40 for this! Dont know if that type is
		 *	used by other apps, but if it is, it most probably will
		 *	crash badly!
		 */ 
		{
			struct extbox_parms *p;
			OBJECT *obtree = wt->tree;

			if ((p = kmalloc(sizeof(struct extbox_parms))))
			{
				p->callout = fnts_extb_callout;
				p->data = fnts;
				p->obspec = object_get_spec(obtree + FNTS_SHOW)->index;
				object_set_spec(obtree + FNTS_SHOW, (unsigned long)p);
				p->type = obtree[FNTS_SHOW].ob_type;
				obtree[FNTS_SHOW].ob_type = G_EXTBOX;
			}
		}
	}
	return fnts;
}

extern struct toolbar_handlers wdlg_th;

unsigned long
XA_fnts_create(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts = NULL;
	struct xa_window *wind = NULL;
	OBJECT *obtree = NULL;
	XA_TREE *wt = NULL;
	XA_WIND_ATTR tp = MOVER|NAME;
	RECT r, or;

	DIAG((D_fnts, client, "XA_fnts_create"));
	pb->intout[0] = 0;
	pb->addrout[0] = 0L;

	if ((wt = duplicate_fnts_obtree(client)))
	{
		obtree = wt->tree;
		ob_rectangle(obtree, aesobj(obtree, 0), &or);
		
		r = calc_window(lock, client, WC_BORDER,
				tp, created_for_WDIAL,
				client->options.thinframe,
				client->options.thinwork,
				*(RECT *)&or);

		if (!(wind = create_window(lock,
				     send_app_message,
				     NULL,
				     client,
				     false,
				     tp,
				     created_for_WDIAL,
				     client->options.thinframe,
				     client->options.thinwork,
				     r, NULL, NULL)))
			goto memerr;


		if (!(fnts = create_new_fnts(lock, client, wind, wt, pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3], (char *)pb->addrin[0], (char *)pb->addrin[1])))
			goto memerr;

		wt = set_toolbar_widget(lock, wind, client, obtree, aesobj(obtree, -2), 0, true, &wdlg_th, &or);

		update_slists(fnts);
		
		pb->addrout[0] = (long)fnts->handle;

		pb->intout[0] = 1;
	}
	else
	{
memerr:
		if (wt)
			remove_wt(wt, true);
		else if (obtree)
			free_object_tree(C.Aes, obtree);

		if (wind)
			delete_window(lock, wind);
		
		if (fnts)
			kfree(fnts);
	}
	return XAC_DONE;
}

unsigned long
XA_fnts_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	
	DIAG((D_fnts, client, "XA_fnts_delete"));

	pb->intout[0] = 0;

	fnts = (struct xa_fnts_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		
		if (wind->window_status & XAWS_OPEN)
			close_window(lock, wind);

		delayed_delete_window(lock, wind);
		delete_xa_data(&client->xa_data, fnts);

		pb->intout[0] = 1;
	}
	return XAC_DONE;
}

static void
set_but(OBJECT *obtree, short ind, bool en)
{
	if (en)
	{
		obtree[ind].ob_flags |= OF_SELECTABLE;
		obtree[ind].ob_state &= ~OS_DISABLED;
	}
	else
	{
		obtree[ind].ob_flags &= ~OF_SELECTABLE;
		obtree[ind].ob_state |= OS_DISABLED;
	}
}
static void
set_obj(OBJECT *obtree, short ind, bool sel, bool hide)
{
	if (sel)
		obtree[ind].ob_state |= OS_CROSSED;
	else
		obtree[ind].ob_state &= ~OS_CROSSED;
	if (hide)
		obtree[ind].ob_flags |= OF_HIDETREE;
	else
		obtree[ind].ob_flags &= ~OF_HIDETREE;
}

static void
update(struct xa_fnts_info *fnts, short bf)
{
	OBJECT *obtree = fnts->wt->tree;

	set_obj(obtree, FNTS_CBNAMES, (bf & FNTS_SNAME), (bf & FNTS_CHNAME));
	set_obj(obtree, FNTS_CBSTYLES, (bf & FNTS_SSTYLE), (bf & FNTS_CHSTYLE));
	set_obj(obtree, FNTS_CBSIZES, (bf & FNTS_SSIZE), (bf & FNTS_CHSIZE));
	set_obj(obtree, FNTS_CBRATIO, (bf & FNTS_SRATIO), (bf & FNTS_CHRATIO));
		
	set_but(obtree, FNTS_XSET, (bf & FNTS_BSET));
	set_but(obtree, FNTS_XMARK, (bf & FNTS_BMARK));

	//fnts_redraw(0, fnts->wind, 0, 10, NULL);
	wdialog_redraw(0, fnts->wind, aesobj(obtree, 0), 10, NULL);
}

static void
find_fitembyid(struct xa_fnts_info *fnts, unsigned long id, struct xa_fnts_item **ret_family, struct xa_fnts_item **ret_style)
{
	struct xa_fnts_item *fl, *family = NULL, *style = NULL;

	fl = fnts->fnts_list;

	while (fl && !family && !style)
	{
		if (fl->f.id == id)
			family = fl;

		if (fl->nxt_kin)
		{
			style = fl;
			while (style)
			{
				if (style->f.id == id)
				{
					family = fl;
					break;
				}
				style = style->nxt_kin;
			}
		}
		fl = fl->nxt_family;
	}

	if (ret_family)
		*ret_family = family;
	if (ret_style)
		*ret_style = style;
}

static void
init_fnts(struct xa_fnts_info *fnts)
{
	OBJECT *obtree = fnts->wt->tree;

	{
		char pt[16];
		TEDINFO *ted;
		struct xa_aes_object size_obj, ratio_obj;
		
		size_obj  = aesobj(fnts->wt->tree, FNTS_EDSIZE);
		ratio_obj = aesobj(fnts->wt->tree, FNTS_EDRATIO);

		/*
		 * set ratio edit field..
		 */ 
		ted = object_get_tedinfo(aesobj_ob(&ratio_obj), NULL);
		sprintf(pt, sizeof(pt), "%d", (unsigned short)(fnts->fnt_ratio >> 16));
		sprintf(pt + strlen(pt), sizeof(pt) - strlen(pt), ".%d", (short)(fnts->fnt_ratio));
		strcpy(ted->te_ptext, pt);
		obj_edit(fnts->wt, fnts->vdi_settings, ED_INIT, ratio_obj, 0, -1, NULL, false, NULL, NULL, NULL, NULL);
	
		/*
		 * Set sizes edit field...
		 */
		ted = object_get_tedinfo(aesobj_ob(&size_obj), NULL);

		if (fnts->fnt_pt)
		{
			sprintf(pt, sizeof(pt), "%ld", (fnts->fnt_pt >> 16));
			strcpy(ted->te_ptext, pt);
		}
		else
			strcpy(ted->te_ptext, "10");
	
		obj_edit(fnts->wt, fnts->vdi_settings, ED_INIT, size_obj, 0, -1, NULL, false, NULL, NULL, NULL, NULL);

	}
	DIAG((D_fnts, NULL, " --- fnt_id = %ld, fnt_pt = %lx, fnt_ratio = %lx",
		fnts->fnt_id, fnts->fnt_pt, fnts->fnt_ratio));

/*
 * Highlight selected ID, family and point
 */
 	{
		struct xa_fnts_item *family, *style;
		struct scroll_info *list;
		struct scroll_entry *ent;
	
		find_fitembyid(fnts, fnts->fnt_id, &family, &style);
	
		DIAG((D_fnts, NULL, " --- set family=%lx (%s)", family, family ? family->f.full_name : "Not found"));
		DIAG((D_fnts, NULL, " --- set style =%lx (%s)", style,  style  ? style->f.full_name  : "Not found"));
	
		if (family)
		{
			list = object_get_slist(obtree + FNTS_FNTLIST);	
			ent = list->search(list, NULL, SEFM_BYDATA, family);
			DIAG((D_fnts, NULL, " --- search found family entry %lx", ent));
			if (ent)
			{
				list->cur = ent;
				list->set(list, ent, SESET_SELECTED, 0, NOREDRAW);
				set_name_list(fnts, style);
				list->vis(list, ent, FULLREDRAW);
			}
		}
	}
	select_edsize(fnts);
}

unsigned long
XA_fnts_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	
	DIAG((D_fnts, client, "XA_fnts_open"));

	pb->intout[0] = 0;

	fnts = (struct xa_fnts_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		if (!(wind->window_status & XAWS_OPEN))
		{
			struct widget_tree *wt = fnts->wt;
			struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);
			RECT r = wind->wa;
			XA_WIND_ATTR tp = wind->active_widgets | MOVER|NAME;
		
			widg->m.properties |= WIP_NOTEXT;
			set_toolbar_handlers(&wdlg_th, wind, widg, widg->stuff);
		
			obj_init_focus(wt, OB_IF_RESET);
	
			if (pb->intin[1] == -1 || pb->intin[2] == -1)
			{
				r.x = (root_window->wa.w - r.w) / 2;
				r.y = (root_window->wa.h - r.h) / 2;
			}
			else
			{
				r.x = pb->intin[1];
				r.y = pb->intin[2];
			}
			{
				RECT or;

				obj_rectangle(wt, aesobj(wt->tree, 0), &or);
				or.x = r.x;
				or.y = r.y;
				change_window_attribs(lock, client, wind, tp, true, true, 2, or, NULL);
			}

			fnts->button_flags = pb->intin[0];
			update(fnts, fnts->button_flags);

			fnts->fnt_id	= (long)pb->intin[3] << 16 | pb->intin[4];
			fnts->fnt_pt	= (long)pb->intin[5] << 16 | pb->intin[6];
			fnts->fnt_ratio	= (long)pb->intin[7] << 16 | pb->intin[8];

			init_fnts(fnts);
			
			wt->tree->ob_x = wind->wa.x;
			wt->tree->ob_y = wind->wa.y;
			if (!wt->zen)
			{
				wt->tree->ob_x += wt->ox;
				wt->tree->ob_y += wt->oy;
			}
			open_window(lock, wind, wind->rc);
		}
		pb->intout[0] = wind->handle;
	}	
	return XAC_DONE;
}

unsigned long
XA_fnts_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	
	DIAG((D_fnts, client, "XA_fnts_close"));

	pb->intout[0] = 0;
	fnts = (struct xa_fnts_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		pb->intout[0] = 1;
		pb->intout[1] = fnts->wt->tree->ob_x;
		pb->intout[2] = fnts->wt->tree->ob_y;
		close_window(lock, wind);
	}
	return XAC_DONE;
}

unsigned long
XA_fnts_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	
	DIAG((D_fnts, client, "XA_fnts_get"));
	
	pb->intout[0] = 0;
	fnts = (struct xa_fnts_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		switch (pb->intin[0])
		{
			case 0: /* Get Number of styles */
			{
				struct xa_fnts_item *family;
				short count = 0;
				
				find_fitembyid(fnts, *(const unsigned long *)&pb->intin[1], &family, NULL);
				if (family)
				{
					count = 1;
					while (family->nxt_kin)
					{
						family = family->nxt_kin;
						count++;
					}
				}
				pb->intout[0] = count;	
				break;
			}
			case 1: /* Get style ID */
			{
				struct xa_fnts_item *family;
				short index = pb->intin[3];
				unsigned long id = 0L;

				find_fitembyid(fnts, *(const unsigned long *)&pb->intin[1], &family, NULL);
				for (; index > 0 && family; index--, family = family->nxt_kin)
					id = family->f.id;

				*(long *)&pb->intout[0] = id;	
				break;
			}
			case 2: /* Get font name */
			{
				struct xa_fnts_item *family, *style;
				char *d;
				
				find_fitembyid(fnts, *(const unsigned long *)&pb->intin[1], &family, &style);
				
				if (style)
					family = style;
				
				if (family)
				{
					if ((d = (char *)pb->addrin[1]))
						strcpy(d, family->f.full_name);
					if ((d = (char *)pb->addrin[2]))
						strcpy(d, family->f.family_name);
					if ((d = (char *)pb->addrin[3]))
						strcpy(d, family->f.style_name);

					pb->intout[0] = 1;
				}
				break;
			}
			case 3: /*Get font info */
			{
				struct xa_fnts_item *family, *style;
				
				find_fitembyid(fnts, *(const unsigned long *)&pb->intin[1], &family, &style);
				
				if (style)
					family = style;
				
				if (family)
				{
					if ((pb->intout[0] = family->f.index))
					{
						pb->intout[1] = family->f.mono;
						pb->intout[2] = family->f.outline;
					}
				}
				break;
			}
		}
	}
	
	return XAC_DONE;
}

unsigned long
XA_fnts_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	
	DIAG((D_fnts, client, "XA_fnts_set: funct = %d, fnts=%lx",
		pb->intin[0], ((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16)));

	pb->intout[0] = 0;
	fnts = (struct xa_fnts_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		switch (pb->intin[0])
		{
			case 0: /* Add user fonts */
			{
				FNTS_ITEM *f = (FNTS_ITEM*)pb->addrin[1];
				struct xa_fnts_item *xf;
				int i;
				
				DIAG((D_fnts, client, " --- add user fonts"));
				
				while (f)
				{
					xf = new_fnts_item();
					if (xf)
					{
						xf->f.display	= f->display;
						xf->f.id	= f->id;
						xf->f.index	= 0;
						xf->f.mono	= f->mono;
						xf->f.outline	= f->outline;
						xf->f.npts	= f->npts;
						strcpy(xf->f.full_name, f->full_name);
						strcpy(xf->f.family_name, f->family_name);
						strcpy(xf->f.style_name, f->style_name);
						for (i = 0; i < f->npts; i++)
							xf->f.pts[i] = f->pts[i];
						
						link_fnts_item(&fnts->fnts_ring, xf);
						add_fnts_item(&fnts->fnts_list, xf, fnts->font_flags);
						DIAG((D_fnts, client, " --- copy %lx into %lx", f, xf));
					}
					f = f->next;
				}
				update_slists(fnts);
				break;
			}
			case 1: /* Remove user fonts */
			{
				struct xa_fnts_item *f = fnts->fnts_ring, *n;
				bool upd = false;

				DIAG((D_fnts, client, " --- remove user fonts"));
				while (f)
				{
					n = f->link;
					if (!f->f.index)
					{
						upd = true;
						unlink_fnts_item(&fnts->fnts_ring, f);
					}
					f = n;
				}
				if (upd)
					update_slists(fnts);
				
				break;
			}
			case 2: /* update window */
			{
				DIAG((D_fnts, client, " --- update window"));
				fnts->button_flags = pb->intin[2];
				update(fnts, pb->intin[2]);
				break;
			}
		}
	}
	return XAC_DONE;
}

static int
check_internal_objects(struct xa_fnts_info *fnts, struct xa_aes_object obj)
{
	long id;
	int ret = 0;

	if (aesobj_item(&obj) == FNTS_XDISPLAY)
	{
		//fnts->dialog_flags ^= FNTS_DISPLAY;
		
		if (aesobj_ob(&obj)->ob_state & OS_SELECTED)
			fnts->dialog_flags |= FNTS_DISPLAY;
		else
			fnts->dialog_flags &= ~FNTS_DISPLAY;
		
		id = fnts->fnts_selected ? fnts->fnts_selected->f.id : 0;
		update_slists(fnts);
		if (id)
		{
			fnts->fnt_id = id;
			init_fnts(fnts);
		}
		ret = 1;
	}
	return ret;
}

static short
get_cbstatus(OBJECT *obtree)
{
	int i;
	short objs[4] = { FNTS_CBNAMES, FNTS_CBSTYLES, FNTS_CBSIZES, FNTS_CBRATIO};
	short obit[4] = {   FNTS_SNAME,   FNTS_SSTYLE,   FNTS_SSIZE,  FNTS_SRATIO};
	short state = 0;

	for (i = 0; i < 4; i++)
	{
		if (obtree[objs[i]].ob_state & OS_SELECTED)
			state |= obit[i];
	}
	return state;
}

unsigned long
XA_fnts_evnt(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	short ret = 1;
	
	DIAG((D_fnts, client, "XA_fnts_evnt: funct = %d, fnts=%lx",
		pb->intin[0], ((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16)));

	fnts = (struct xa_fnts_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
// 		OBJECT *obtree = fnts->wt->tree;
		long val;
		struct wdlg_evnt_parms wep;

		wep.wind	= wind;
		wep.wt		= get_widget(wind, XAW_TOOLBAR)->stuff;
		wep.ev		= (EVNT *)pb->addrin[1];
		wep.wdlg	= NULL;
		wep.callout	= NULL;
		wep.redraw	= wdialog_redraw;
		wep.obj		= inv_aesobj();

		ret = wdialog_event(lock, client, &wep);
		
		if (check_internal_objects(fnts, wep.obj))
		{
			wep.obj = inv_aesobj();
			ret = 1;
		}
		else
		{
			if (valid_aesobj(&wep.obj) && (aesobj_ob(&wep.obj)->ob_state & OS_SELECTED))
				obj_change(fnts->wt, wind->vdi_settings, wep.obj, -1, aesobj_ob(&wep.obj)->ob_state & ~OS_SELECTED, aesobj_ob(&wep.obj)->ob_flags, true, &wind->wa, wind->rect_list.start, 0);
		
			val = get_edpoint(fnts);
			if (val != fnts->fnt_pt)
			{
				fnts->fnt_pt = val;
				wdialog_redraw(0, wind, aesobj(fnts->wt->tree, FNTS_SHOW), 1, NULL);
				select_edsize(fnts);
			}
			fnts->fnt_pt = val;
		
			*(long *)&pb->intout[3] = fnts->fnts_selected->f.id;
			*(long *)&pb->intout[5] = fnts->fnt_pt;
			*(long *)&pb->intout[7] = fnts->fnt_ratio;
		}
		pb->intout[1] = valid_aesobj(&wep.obj) ? aesobj_item(&wep.obj) : 0;
		pb->intout[2] = get_cbstatus(wep.wt->tree);
	}
	pb->intout[0] = ret;

	DIAG((D_fnts, client, " --- return %d, obj=%d, cbstat=%x, id=%ld, pt=%ld, ratio=%ld",
		pb->intout[0], pb->intout[1], pb->intout[2], *(long *)&pb->intout[3], *(long *)&pb->intout[5], *(long *)&pb->intout[7]));

	return XAC_DONE;
}
/*
 * WidgetBehaviour()
 * Return is used by do_widgets() to check if state of obj is
 * to be reset or not
 */
//static WidgetBehaviour fnts_th_click;
//static FormMouseInput	fntsClick_form_do;
static FormKeyInput	Keypress;
static FormExit		Formexit;

static bool
Keypress(enum locks lock,
	    struct xa_client *client,
	    struct xa_window *wind,
	    struct widget_tree *wt,
	    const struct rawkey *key,
	    struct fmd_result *res_fr)
{
	bool no_exit;

	if ((no_exit = Key_form_do(lock, client, wind, wt, key, NULL)))
	{
		struct scroll_info *list;
		struct xa_fnts_info *fnts;
		unsigned long val;
		
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		list = object_get_slist(wt->tree + FNTS_FNTLIST);
		fnts = list->data;
		val = get_edpoint(fnts);

		if (val != fnts->fnt_pt)
		{
			fnts->fnt_pt = val;
			//fnts_redraw(0, wind, FNTS_SHOW, 1, NULL);
			wdialog_redraw(0, wind, aesobj(fnts->wt->tree, FNTS_SHOW), 1, NULL);

			select_edsize(fnts);
		}
	}
	return no_exit;	
}

static void
Formexit( struct xa_client *client,
	      struct xa_window *wind,
	      XA_TREE *wt,
	      struct fmd_result *fr)
{
	struct xa_fnts_info *fnts = (struct xa_fnts_info *)(object_get_slist(wt->tree + FNTS_FNTLIST))->data;
	
	if (!check_internal_objects(fnts, fr->obj))
	{
		fnts->exit_button = valid_aesobj(&fr->obj) ? aesobj_item(&fr->obj) : 0;
		client->usr_evnt = 1;
	}
}

static struct toolbar_handlers fnts_th =
{
	Formexit,		/* FormExit		*exitform;	*/
	Keypress,		/* FormKeyInput		*keypress;	*/

	NULL,			/* DisplayWidget	*display;	*/
	NULL, /*fnts_th_click,*/	/* WidgetBehaviour	*click;		*/
	NULL, /*fnts_th_click,*/	/* WidgetBehaviour	*drag;		*/
	NULL,			/* WidgetBehaviour	*release;	*/
	NULL,			/* void (*destruct)(struct xa_widget *w); */
};

/* XA_fnts_do
 * Ozk:
 *  Todo: Cursor isnt automatically drawn.
 */ 
unsigned long
XA_fnts_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	
	DIAG((D_fnts, client, "XA_fnts_do"));

	pb->intout[0] = 0;

	fnts = (struct xa_fnts_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		XA_TREE *wt = fnts->wt;
		XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
		OBJECT *obtree = wt->tree;
		RECT or;
		XA_WIND_ATTR tp = wind->active_widgets & ~STD_WIDGETS;

		obj_init_focus(wt, OB_IF_RESET);

		ob_rectangle(obtree, aesobj(obtree, 0), &or);
		center_rect(&or);

		change_window_attribs(lock, client, wind, tp, true, true, 2, or, NULL);
		
		widg->m.properties &= ~WIP_NOTEXT;
		set_toolbar_handlers(&fnts_th, wind, widg, widg->stuff);
		wt->flags |= WTF_FBDO_SLIST;

		fnts->button_flags = pb->intin[0];
		update(fnts, fnts->button_flags);

		fnts->fnt_id	= *(const long *)&pb->intin[1];
		fnts->fnt_pt	= *(const long *)&pb->intin[3];
		fnts->fnt_ratio	= *(const long *)&pb->intin[5];

		init_fnts(fnts);
			
		open_window(lock, wind, wind->rc);

		client->status |= CS_FORM_DO;
		Block(client, 0);
		client->status &= ~CS_FORM_DO;
		wt->flags &= ~WTF_FBDO_SLIST;
		close_window(lock, wind);
		
		pb->intout[0] = fnts->exit_button;
		pb->intout[1] = get_cbstatus(fnts->wt->tree);
		*(long *)&pb->intout[2] = fnts->fnts_selected->f.id;
		*(long *)&pb->intout[4] = fnts->fnt_pt;
		*(long *)&pb->intout[6] = fnts->fnt_ratio;
	}
	return XAC_DONE;
}

#endif /* WDIALOG_FNTS */
