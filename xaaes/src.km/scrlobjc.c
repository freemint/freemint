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
#define USEOWNSTRLEN 1
#define USEBUILTINMEMCPY	0
#define LSLINDST_PTS	2

#include "xa_types.h"
#include "xa_global.h"

#include "xa_evnt.h"
#include "xa_graf.h"
#include "k_mouse.h"

#include "scrlobjc.h"
#include "form.h"
#include "rectlist.h"
#include "draw_obj.h"
#include "obtree.h"
#include "widgets.h"
#include "xa_fsel.h"
#include "xa_form.h"
#include "c_window.h"
#include "keycodes.h"

#ifdef __GNUC__
/* using builtin */
# undef memcpy
# if USEBUILTINMEMCPY
# define memcpy __builtin_memcpy
# define _Pmemcpy _P__builtin_memcpy
# else
void* memcpy (void*  __dest, const void* __src, size_t __n);
# endif

#else
typedef void * _cdecl (*memcpy_t)(void *dst, const void *src, unsigned long nbytes);
static memcpy_t memcpy = 0;
#endif

#if USEOWNSTRLEN
static int xa_strlen (const char *p1 )
{
	const char *p = p1;
	if( /*!p1 ||*/ !*p1 )
		return 0;
	for( ; *++p1; );
	return (int)(p1-p);
}
# undef strlen
#define strlen xa_strlen
#endif

#define ONLY_OPENED 1
#define NO_DOWN     2

static void
slist_msg_handler(struct xa_window *wind, struct xa_client *to, short amq, short qmf, short *msg);

static void entry_action(struct scroll_info *list, struct scroll_entry *this, const struct moose_data *md);

/*static*/ struct xa_wtxt_inf default_fnt =
{
 WTXT_NOCLIP,
/* id  pnts flags wr_mode   efx  fg       bg   banner x_3dact y_3dact texture */
 {  1,  10, 0,    MD_TRANS, 0,   G_BLACK, G_WHITE, G_WHITE,   0,      0,    NULL},	/* Normal */
 {  1,  10, 0,    MD_TRANS, 0,   G_WHITE, G_BLACK, G_WHITE,   0,      0,    NULL},	/* Selected */
 {  1,  10, 0,    MD_TRANS, 0,   G_BLACK, G_WHITE, G_WHITE,   0,      0,    NULL},	/* Highlighted */

};
static struct xa_wcol_inf default_col =
{
	WCOL_DRAWBKG, MD_REPLACE,
      /* color    inter     style   boxcol   boxsize   tl       br        texture */
	{G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK,   NULL},
	{G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK,   NULL},
	{G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK,   NULL},
};

static struct se_tab default_setab =
{
	0,0,
	{0,0,0,0},
	{0,0,0,0},
	0,
	0,
};

static SCROLL_ENTRY *
next_entry(SCROLL_ENTRY *this, short flags, short maxlevel, short *level)
{
	short lev;
	bool goabove;

	if (level)
	{
		lev = *level;
		goabove = (flags & ENT_GOABOVE);
	}
	else
	{
		lev = 0;
		goabove = true;
	}

	if (this)
	{
		if (flags & ENT_VISIBLE)
		{
			if (this->down && (this->xstate & OS_OPENED) && (maxlevel == -1 || maxlevel > lev))
				this = this->down, lev++;
			else if (!lev && (flags & ENT_ISROOT))
				this = NULL;
			else if (this->next)
				this = this->next;
			else
			{
				lev--;
				if (flags & ENT_ISROOT)
				{
					if (!lev)
						this = NULL;
					else
					{
						this = this->up;
						while (this)
						{
							if (this->next)
							{
								this = this->next;
								break;
							}
							else
							{
								lev--;
								if (!lev)
									this = NULL;
								else
									this = this->up;
							}
						}
					}
				}
				else
				{
					if (lev < 0 && !goabove)
						this = NULL, lev++;
					else if ((this = this->up))
					{
						while (this)
						{
							if (this->next)
							{
								this = this->next;
								break;
							}
							else
							{
								lev--;
								if (lev < 0 && !goabove)
									this = NULL, lev++;
								else if (!(this = this->up))
									lev++;
							}
						}
					}
					else
						lev++;
				}

			}
		}
		else
		{
			if (this->down && (maxlevel == -1 || maxlevel > lev))
				this = this->down, lev++;
			else if (!lev && (flags & ENT_ISROOT))
				this = NULL;
			else if (this->next)
				this = this->next;
			else
			{
				lev--;
				if (flags & ENT_ISROOT)
				{
					if (!lev)
						this = NULL;
					else
					{
						this = this->up;
						while (this)
						{
							if (this->next)
							{
								this = this->next;
								break;
							}
							else
							{
								lev--;
								if (!lev)
									this = NULL;
								else
									this = this->up;
							}
						}
					}
				}
				else
				{
					if (lev < 0 && !goabove)
						this = NULL, lev++;
					else if ((this = this->up))
					{
						while (this)
						{
							if (this->next)
							{
								this = this->next;
								break;
							}
							else
							{
								lev--;
								if (lev < 0 && !goabove)
									this = NULL, lev++;
								else if (!(this = this->up))
									lev++;
							}
						}
					}
					else
						lev++;
				}
			}
		}
	}
	if (level)
		*level = lev;

	return this;
}

static SCROLL_ENTRY *
prev_entry(SCROLL_ENTRY *this, short flags)
{
	if (this)
	{
		if (this->prev)
		{
			this = this->prev;
			while (this->down && (this->xstate & OS_OPENED))
			{
				this = this->down;
				while (this->next)
					this = this->next;
			}
		}
		else if (this->up)
			this = this->up;
		else
			this = NULL;
	}

	return this;
}

static bool
is_in_list(SCROLL_INFO *list, SCROLL_ENTRY *this)
{
	SCROLL_ENTRY *e = list->start;

	while (e)
	{
		if (e == this)
			return true;
		e = next_entry(e, 0, -1, NULL);
	}
	return false;
}

static void
change_entry(SCROLL_INFO *list, SCROLL_ENTRY *entry, bool select, bool rdrw)
{
	short state = entry->state;

	if (select)
		state |= OS_SELECTED;
	else
		state &= ~OS_SELECTED;

	if (entry->state != state)
	{
		entry->state = state;
		if (rdrw) list->redraw(list, entry);
	}
}

static void
check_movement(SCROLL_INFO *list)
{
	struct xa_window *wind = list->wi;
	OBJECT *obtree = list->wt->tree;
	short x, y;

	x = obtree->ob_x + list->rel_x;
	y = obtree->ob_y + list->rel_y;

	if (x != wind->rc.g_x || y != wind->rc.g_y )
	{
		wind->r.g_x = wind->rc.g_x = x;
		wind->r.g_y = wind->rc.g_y = y;
		calc_work_area(list->wi);
	}
}
static bool
canredraw(SCROLL_INFO *list)
{
	struct xa_window *wind;

	if (!(wind = list->pw))
		wind = list->wt->wind;
	if (wind && (wind->window_status & (XAWS_OPEN|XAWS_SHADED|XAWS_HIDDEN)) == XAWS_OPEN)
	{
		check_movement(list);
		return true;
	}
	return false;
}


static void
get_list_rect(struct scroll_info *list, GRECT *r)
{
	check_movement(list);
	*r = list->wi->wa;
}

static void
recalc_tabs(struct scroll_info *list)
{
	int i, ntabs;
	short listw, x, totalw;
	short w0;
	struct se_tab *tabs;
	GRECT r_list;

	/* negative is percentage */
	/* null is width of content */
	/* positive is static size */

	tabs = list->tabs;
	ntabs = list->num_tabs;
	get_list_rect(list, &r_list);
	listw = r_list.g_w;
	x = 0;
	totalw = 0;

	w0 = 0;
	if( tabs->v.g_h < 0 )
	{
		w0 = (-tabs->v.g_h + 8) * screen.c_max_w;
		if( w0 > listw )
			w0 = listw;
	}

	for (i = 0; i < ntabs; i++, tabs++)
	{
		tabs->r.g_x = x;

		if (tabs->v.g_w < 0)
		{
			short p = -tabs->v.g_w;
			long tmp;

			if (p > 100)
				p = 100;

			tmp = (long)listw * p;
			tmp /= 100;

			if ((listw - tmp) < 0)
			{
				tabs->r.g_w = 4;
			}
			else
			{
				tabs->r.g_w = tmp;
			}
		}
		else if (!tabs->v.g_w)
		{
			listw -= tabs->r.g_w;
			if (listw < 0)
				listw = 0;
		}
		else
		{
			listw -= tabs->v.g_w;
			tabs->r.g_w = tabs->v.g_w;
			if (listw < 0)
				listw = 0;
		}
		tabs->r.g_x = x;
		tabs->r.g_y = 0;

		if( i == 0 && w0 )
			tabs->r.g_w = w0;
		x += tabs->r.g_w;
		totalw += tabs->r.g_w;
	}

	if (list->widest < totalw)
		list->widest = list->total_w = totalw;

}

static short calc_len(char *s)
{
	short r = 0;
	for( ; *s; s++ )
		if( *s == '\t' )
			r = (r / TABSZ + 1) * TABSZ;
		else if( *s == '<' )	/* don't count html-tags */
		{
			short n;
			char *t;
			for( t = s, n = 0; *t && *t != '>' && n < 4; n++, t++ )
				;
			if( *t == '>' )
				s = t;
		}
		else
			r++;

	return r;
}

static bool
calc_entry_wh(SCROLL_INFO *list, SCROLL_ENTRY *this)
{
	bool fullredraw = false;
	struct se_tab *tabs = list->tabs, *tab;
	struct se_content *c = this->content;
	short ew, eh, tw = 0, th = 0;
	char *s;
	PRDEF( calc_entry_wh, text_extent);
	PRDEF( calc_entry_wh, recalc_tabs);

	while (c)
	{
		switch (c->type)
		{
			case SECONTENT_TEXT:
			{
				short l = 0;
				s = c->c.text.text;
				if( s )
				{
					if( (list->flags & SIF_INLINE_EFFECTS) )
					{
						/*
						 * include (inline-)tabs and html-tags
						 */
						l = calc_len(s);
					}
					else
						l = c->c.text.slen;
				}


				if( list->char_width )
				{
					c->c.text.w = list->char_width * l;	/* assuming unprop. font! */
					/* TODO list->char_height, list->const_height */
					if( !c->c.text.h && c->prev && c->prev->c.text.h )
						c->c.text.h = c->prev->c.text.h;
				}
				if( !c->c.text.h || !list->char_width )
				{
					short w;
					struct xa_fnt_info *f = c->fnt ? &c->fnt->normal : &this->fnt->normal;
					PROFRECp(
					*list->vdi_settings->api->,text_extent,(list->vdi_settings, s,
							f, &w, &c->c.text.h));
					if( !c->c.text.w )
						c->c.text.w = w;

					if( l && !list->char_width )
					{
						list->char_width = c->c.text.w / l;
					}
				}

				ew = c->c.text.w + 2;
				eh = c->c.text.h;
				if (c->c.text.icon.icon)
				{
					ew += c->c.text.icon.r.g_w;
					if (c->c.text.icon.r.g_h > eh)
						eh = c->c.text.icon.r.g_h;
				}
				else if( list->char_width && (long)this->data > 1 )
					ew += list->char_width;

				break;
			}
			case SECONTENT_ICON:
			{
				ew = c->c.icon.r.g_w + 2;
				eh = c->c.icon.r.g_h;
				break;
			}
			default:
			{
				ew = 0;
				eh = 0;
			}
		}

		tab = &tabs[c->index];
		if (tab->r.g_w < ew)
		{
			tab->r.g_w = ew + 3;
			if( !(list->flags & SIF_INLINE_EFFECTS) )
				PROFRECv(recalc_tabs,(list));
			fullredraw = true;
		}

		tw += ew;

		if (th < eh)
			th = eh;

		c = c->next;
	}

	if (list->flags & SIF_TREEVIEW)
		tw += list->char_width * 6;	/* indentation ... */
	this->r.g_w = tw;
	if( this->r.g_h == 0 )
		this->r.g_h = th;	/* dont change height */

	return fullredraw;
}


static short
draw_nesticon(struct xa_vdi_settings *v, short width, GRECT *xy, SCROLL_ENTRY *this)
{
	int i;
	GRECT r;
	short x_center, y_center, x;
	short pnt[4];

	(*v->api->wr_mode)(v, MD_REPLACE);

	x_center = (width >> 1);
	y_center = (xy->g_h >> 1);
	if (this->level)
	{
		struct scroll_entry *root = this->up;

		x = xy->g_x + (width * (this->level - 1));

		(*v->api->l_color)(v, G_LBLACK);
		for (i = 0; i < this->level; i++)
		{
			if (root)
			{
				if (root->next)
				{
					pnt[0] = x + x_center;
					pnt[1] = xy->g_y;
					pnt[2] = pnt[0];
					pnt[3] = xy->g_y + xy->g_h - 1;
					v_pline(v->handle, 2, pnt);
				}
				root = root->up;
			}
			x -= width;
		}
	}

	x = xy->g_x + (width * this->level);

	r.g_x = x + x_center - 4;
	r.g_y = xy->g_y + y_center - 4;
	r.g_w = 9;
	if( this->r.g_h < 9 )
		r.g_h = this->r.g_h - 2;
	else
		r.g_h = 9;


	if (this->down || (this->xstate & OS_NESTICON))
	{
		(*v->api->f_interior)(v, FIS_SOLID);
		(*v->api->f_color)(v, G_WHITE);
		(*v->api->gbar)(v, 0, &r);

		(*v->api->l_color)(v, G_BLACK);
		(*v->api->gbox)(v, 0, &r);
		if (this->prev)
		{
			(*v->api->l_color)(v, G_LBLACK);
			pnt[0] = x + x_center;
			pnt[1] = xy->g_y;
			pnt[2] = pnt[0];
			pnt[3] = r.g_y;
			v_pline(v->handle, 2, pnt);
		}
		if (this->next)
		{
			pnt[0] = x + x_center;
			pnt[1] = r.g_y + 9;
			pnt[2] = pnt[0];
			pnt[3] = xy->g_y + xy->g_h - 1;
			v_pline(v->handle, 2, pnt);
		}

		(*v->api->l_color)(v, G_BLACK);
		pnt[0] = r.g_x + 2;
		pnt[1] = r.g_y + 4;
		pnt[2] = pnt[0] + 4;
		pnt[3] = pnt[1];

		v_pline(v->handle, 2, pnt);

		if (!(this->xstate & OS_OPENED))
		{
			pnt[0] = r.g_x + 4;
			pnt[1] = r.g_y + 2;
			pnt[2] = pnt[0];
			pnt[3] = r.g_y + 6;
			v_pline(v->handle, 2, pnt);
		}
	}
	else
	{
		(*v->api->l_color)(v, G_LBLACK);
		if (this->prev || this->up)
		{
			pnt[0] = x + x_center;
			pnt[1] = xy->g_y;
			pnt[2] = pnt[0];
			pnt[3] = xy->g_y + y_center;
			v_pline(v->handle, 2, pnt);
		}
		if (this->next)
		{
			pnt[0] = x + x_center;
			pnt[1] = xy->g_y + y_center;
			pnt[2] = pnt[0];
			pnt[3] = xy->g_y + xy->g_h - 1;
			v_pline(v->handle, 2, pnt);
		}

		pnt[0] = x + x_center;
		pnt[1] = xy->g_y + y_center;
		pnt[2] = x + width;
		pnt[3] = pnt[1];
		v_pline(v->handle, 2, pnt);
	}
	return width;
}

static short ssel;
static struct xa_fnt_info owtxt = {0};
static void
display_list_element(int lock, SCROLL_INFO *list, SCROLL_ENTRY *this,
	struct xa_vdi_settings *v, GRECT *xy, const GRECT *clip, short TOP, bool NO_ICONS)
{
	bool sel = this->state & OS_SELECTED;

	if (this)
	{
		short indent = this->indent;
		struct se_content *c = this->content;
		struct se_tab *tab;
		struct xa_fnt_info *wtxt;
		GRECT r, clp;

		if( TOP != 2 || ssel != sel )
		{
			(*v->api->wr_mode)(v, MD_REPLACE);
			(*v->api->f_color)(v, sel ? G_BLACK : G_WHITE);
			(*v->api->f_interior)(v, FIS_SOLID);
			ssel = sel;
		}
		(*v->api->bar)(v, 0, xy->g_x, xy->g_y, xy->g_w, this->r.g_h);

		if (list->flags & SIF_TREEVIEW)
		{
			r.g_x = xy->g_x;
			r.g_y = xy->g_y;
			r.g_w = 16 * (this->level+1);
			r.g_h = this->r.g_h;
			if ( (TOP || xa_rect_clip(clip, &r, &clp)))
			{
				if( !TOP )
				{
					(*v->api->set_clip)(v, &clp);
				}
				draw_nesticon(v, 16, xy, this);
			}
			indent += 16;
		}

		r.g_h = this->r.g_h;

		while (c)
		{
			tab = &list->tabs[c->index];

			r.g_x = tab->r.g_x;
			r.g_w = tab->r.g_w - 3;

			if (c->index < list->indent_upto)
			{
				r.g_x += indent;
				if (tab->v.g_w < 0)
				{
					if ((r.g_w -= indent) < 0 && r.g_x >= list->tabs[list->indent_upto].r.g_x)
						r.g_w = 0;
				}
			}

			r.g_x += xy->g_x;
			r.g_y = xy->g_y;

			switch (c->type)
			{
				case SECONTENT_ICON:
				{
					if (NO_ICONS == false && (TOP || xa_rect_clip(clip, &r, &clp)))
					{
						short ix, iy;

						if( !TOP )
						{
							(*v->api->set_clip)(v, &clp);
						}
						if (sel)
							c->c.icon.icon->ob_state |= OS_SELECTED;
						else
							c->c.icon.icon->ob_state &= ~OS_SELECTED;

						ix = c->c.icon.r.g_w - c->c.icon.icon->ob_width;
						if (ix > 0)
							ix = r.g_x + (ix >> 1);
						else
							ix = r.g_x;

						iy = c->c.icon.r.g_h - c->c.icon.icon->ob_height;
						if (iy > 0)
							iy = r.g_y + (iy >> 1);
						else
							iy = r.g_y;

						list->nil_wt->tree = c->c.icon.icon;
						list->nil_wt->owner = list->wt->owner;
						list->nil_wt->objcr_api = list->wt->owner->objcr_api;
						list->nil_wt->objcr_theme = list->wt->owner->objcr_theme;
						display_object(lock, list->nil_wt, v, aesobj(list->nil_wt->tree, 0), ix, iy, 0);
					}
					break;
				}
				case SECONTENT_TEXT:
				{
					short x2, dx, dy, tw, th, f;
					char t[512];

					if (TOP || xa_rect_clip(clip, &r, &clp))
					{
						if( !TOP )
						{
							(*v->api->set_clip)(v, &clp);
						}

						dx = r.g_x;
						x2 = dx + r.g_w;
						dy = r.g_y;
						tw = r.g_w;
						if (c->c.text.icon.icon)
						{
							if( NO_ICONS == false )
							{

							short ix, iy;
							if (sel)
								c->c.text.icon.icon->ob_state |= OS_SELECTED;
							else
								c->c.text.icon.icon->ob_state &= ~OS_SELECTED;

							ix = c->c.text.icon.r.g_w - c->c.text.icon.icon->ob_width;
							if (ix > 0)
								ix = dx + (ix >> 1);
							else
								ix = dx;
							iy = c->c.text.icon.r.g_h - c->c.text.icon.icon->ob_height;
							if (iy > 0)
								iy = dy + (iy >> 1);
							else
								iy = dy;

							list->nil_wt->tree = c->c.text.icon.icon;
							list->nil_wt->owner = list->wt->owner;
							list->nil_wt->objcr_api = list->wt->owner->objcr_api;
							list->nil_wt->objcr_theme = list->wt->owner->objcr_theme;
							display_object(lock, list->nil_wt, v, aesobj(list->nil_wt->tree, 0), ix, iy, 0);
							}
							dx += c->c.text.icon.r.g_w;
							if( list->icon_w == 0 )
								list->icon_w = c->c.text.icon.r.g_w;
							tw -= c->c.text.icon.r.g_w;
						}
						else if (!this->up && this->prev && !c->prev /*&& (long)this->data > 1*/ )
						{
							dx += list->icon_w;
							tw -= list->icon_w;
						}
						if (tw > 0)
						{
							int method, w;

							if( c->index == 0/*FSLIDX_NAME*/ )
							{
								if( c->next /*|| !c->prev*/ )
									method = 1;
								else
									method = -1;	/* no ... at end of line */
							}
							else
								method = 0;
							f = this->fnt->flags;

							if (sel)
								wtxt = c->fnt ? &c->fnt->selected : &this->fnt->selected;
							else
								wtxt = c->fnt ? &c->fnt->normal : &this->fnt->normal;

							if( !(list->flags & SIF_ICONS_HAVE_NO_TEXT) || (list->flags & SIF_TREEVIEW) || memcmp( wtxt, &owtxt, sizeof(owtxt) ) )
							{
								(*v->api->wr_mode)(v, wtxt->wr_mode);
								(*v->api->t_font)(v, wtxt->font_point, wtxt->font_id);
								(*v->api->t_effects)(v, wtxt->effects);
								owtxt = *wtxt;
							}

							w = c->c.text.slen * list->char_width;
							if( method == 1 && (list->flags & SIF_TREEVIEW) )
								tw -= (this->level+1) * 16;

							/* opt: const width for prop_... */
							if( method >= 0 && w > tw )
							{
								strncpy( t, c->c.text.text, c->c.text.tblen);
								t[c->c.text.tblen] = 0;
								(*v->api->prop_clipped_name)(v, t, t, tw, &tw, &th, method);
							}
							else
							{
								int l = c->c.text.slen < sizeof(t) ? c->c.text.slen : sizeof(t);
								strncpy( t, c->c.text.text, sizeof(t)-1 );

								t[l] = 0;
								th = c->c.text.h;
								tw = c->c.text.slen * list->char_width;
							}
							/* on some systems the last letter is cut off if italic */
							if( C.fvdi_version > 0 )
							{
								if( (wtxt->effects & ITALIC) && c->c.text.slen < sizeof(t)-1 )
								{
									t[c->c.text.slen] = ' ';
									t[c->c.text.slen+1] = 0;
								}
							}

							{
							char *tp = t;
#if 0
							short flags = tab->flags;
							/* support for inlined centered and right-bound text (not yet used) */
							if( list->flags & SIF_INLINE_EFFECTS )
							{
								bool cmd = false, not = false;

								if( *tp == '<' )
								{
									if( *(tp+1) == '/' )
									{
										not = true;
										tp++;
									}
									switch( *(tp+1) )
									{
									case 'r':
										if( not == true )
											tab->flags &= ~SETAB_RJUST;
										else
											tab->flags |= SETAB_RJUST;
										cmd = true;
									break;
									case 'c':
										if( not == true )
											tab->flags &= ~SETAB_CJUST;
										else
											tab->flags |= SETAB_CJUST;
										cmd = true;
									break;
									}
									if( cmd == true )
									{
										tp += 2;
										if( *tp != '>' )
										else
											tp++;
										tw -= list->char_width * (tp - t);
									}
								}
							}
#endif
							if (tab->flags & SETAB_RJUST)
							{
								dx = x2 - tw;
							}
							else if ( tab->flags & SETAB_CJUST)
							{
								dx += list->char_width * c->c.text.tblen - (tw >> 1);
							}

							dy += ((this->r.g_h - th) >> 1);

							(*v->api->t_color)(v, wtxt->fg);

							if( list->flags & SIF_INLINE_EFFECTS )
							{
								bool cont = true;
								char *tpp = tp, cp = 0;
								short te = wtxt->effects, wd, h, tabsz = TABSZ * list->char_width, dx0 = dx;

								while( cont )
								{
									bool unknown = false;

									for( ; *tp && !(*tp == '\t' || *tp == '<' || (*tp == '\\' && *(tp+1) == '<')); tp++ )
									;

									if( !(cp = *tp) )
										cont = false;

									*tp = 0;
									v_gtext(v->handle, dx, dy, tpp);
									list->vdi_settings->api->t_extent(list->vdi_settings, tpp, &wd, &h);
									dx += wd;

									tpp = tp;
									switch( cp )
									{
									case '\t':
										dx = dx0 + (((dx - dx0) / tabsz) + 1) * tabsz;
										*tp++ = '\t';
										tpp = tp;
									continue;
									case '\\':
										*tp = '\\';
										tp += 2;
										tpp++;
									continue;
									case '<':
										switch( *++tp )
										{
										case 'i':
											te |= ITALIC;
										break;
										case 'u':
											te |= UNDERLINED;
										break;
										case 'b':
											te |= BOLD;
										break;
										case '/':
											switch( *++tp )
											{
											case 'i':
												te &= ~ITALIC;
											break;
											case 'u':
												te &= ~UNDERLINED;
											break;
											case 'b':
												te &= ~BOLD;
											break;
											default:
												unknown = true;
											}
										break;
										default:
											unknown = true;

										}

										if( unknown == false && *++tp == '>' )
										{
											(*v->api->t_effects)(v, te);
											break;
										}
										else
										{
											tp = tpp + 1;	/* reset tp to 1 after the < */
											*tpp = cp;	/* next output-string */
											continue;
										}

									}

									*tpp = cp;
									if( cont )
										tpp = tp + 1;
								}
							}
							else
							{
								if( (wtxt->effects & ITALIC) )
									dx -= list->char_width / 2;	/* looks nicer */

								if (f & WTXT_DRAW3D)
								{
									if (sel && (f & WTXT_ACT3D))
										dx++, dy++;

									(*v->api->t_color)(v, wtxt->bg);
									dx++;
									dy++;
									v_gtext(v->handle, dx, dy, t);
									dx--;
									dy--;
								}
								v_gtext(v->handle, dx, dy, t);
							}
							}

						}	/*/if (tw > 0)*/
					}
					break;
				}
				default:;
			}

			c = c->next;
		}

		r.g_x = xy->g_x;
		r.g_y = xy->g_y;
		r.g_w = list->widest;
		r.g_h = this->r.g_h;
		{
		bool clipped = false;
		if ((this->state & OS_BOXED) && (TOP || xa_rect_clip(clip, &r, &clp)) )
		{
			if( !TOP )
			{
				clipped = true;
				(*v->api->set_clip)(v, &clp);
			}

			(*v->api->box)(v, 0, r.g_x, r.g_y, r.g_w, r.g_h);
		}

		if( clipped == false )
		{
			(*v->api->set_clip)(v, clip);
		}
		/* normal */
		/* (*v->api->t_effects)(v, 0); */
		}
	}
}

void
draw_slist(int lock, SCROLL_INFO *list, SCROLL_ENTRY *entry, const GRECT *clip)
{
	struct xa_window *wind = list->wi;
	struct xa_vdi_settings *v = list->vdi_settings;
	struct scroll_entry *this = list->top;
	GRECT r, xy;

	if (xa_rect_clip(clip, &wind->wa, &r))
	{
		short TOP = 0;
		bool no_icons = false;

		if( TOP_WINDOW == list->pw )
		{
			TOP = 1;
		}

		(*v->api->set_clip)(v, &r);

		(*v->api->wr_mode)(v, MD_TRANS);

		xy.g_x = wind->wa.g_x - list->start_x;
		xy.g_y = wind->wa.g_y - list->off_y;
		xy.g_w = wind->wa.g_w + list->start_x;
		xy.g_h = wind->wa.g_h + list->off_y;

		if( list->flags & SIF_NO_ICONS )
			no_icons = true;

		memset( &owtxt, 0, sizeof(owtxt) );
		ssel = -1;
		v->font_rid = -1;
		while (this && xy.g_y < (wind->wa.g_y + wind->wa.g_h))
		{
			GRECT ar;

			ar.g_x = xy.g_x;
			ar.g_y = xy.g_y;
			ar.g_w = xy.g_w;
			ar.g_h = this->r.g_h;

			if (!entry || entry == this)
			{
				display_list_element(lock, list, this, v, &ar, &r, TOP, no_icons);
				if( TOP == 1 )
					TOP = 2;
			}

			xy.g_y += this->r.g_h;
			xy.g_h -= this->r.g_h;
			this = next_entry(this, ENT_VISIBLE, -1, NULL);
		}
		TOP = 0;

		if (!entry)
		{
			if (xy.g_h > 0)
			{
				(*v->api->f_color)(v, G_WHITE);
				(*v->api->bar)(v, 0, xy.g_x, xy.g_y, xy.g_w, xy.g_h);
			}
			list->flags &= ~SIF_DIRTY;
		}
		(*v->api->set_clip)(v, clip);
	}

}
static short
find_widest(SCROLL_INFO *list, SCROLL_ENTRY *start);

/*
 * Reset listwindow's widgets according to contents
 */
static bool
reset_listwi_widgets(SCROLL_INFO *list, short redraw)
{
	XA_WIND_ATTR tp = list->wi->active_widgets;
	bool rdrw = false;
	bool as = !!(list->flags & SIF_AUTOSLIDERS);

	if (list->wi)
	{
		if (list->widest/*total_w*/ > list->wi->wa.g_w)
		{
			if (as){
				tp |= (LFARROW|RTARROW|HSLIDE);
			}
		}
		else
		{
			if (as){
				tp &= ~(LFARROW|RTARROW|HSLIDE);
			}
			if ((list->start_x | list->off_x))
			{
				list->start_x = 0;
				list->off_x = 0;
				rdrw = true;
			}
		}

		if (list->total_h > list->wi->wa.g_h)
		{
			if (as)
				tp |= (UPARROW|DNARROW|VSLIDE);
		}
		else
		{
			if (as)
				tp &= ~(UPARROW|DNARROW|VSLIDE);
			if ((list->start_y | list->off_y))
			{
				list->start_y = 0;
				list->off_y = 0;
				list->top = list->start;
				rdrw = true;
			}
		}

		if (rdrw || (tp != list->wi->active_widgets))
		{
			change_window_attribs(0, list->wi->owner, list->wi, tp, false, false, 0, list->wi->r, NULL);
			if (list->flags & SIF_TREEVIEW)
			{
				list->widest = find_widest(list, NULL);
				if( list->tabs[1].r.g_w == 0 )
					list->tabs[0].r.g_w = list->widest;
			}
			recalc_tabs(list);
			if (canredraw(list))
			{
				/* no rectlist-clip here! */
				display_window( 0, 13, list->wi, &list->pw->r);

				if (redraw && (list->flags & SIF_TREEVIEW) )
				{
					list->redraw(list, NULL);
				}

			}
			return true;
		}
	}
	return false;
}

static void
sliders(struct scroll_info *list, bool rdrw)
{
	int rm = 0;


	if (XA_slider(list->wi, XAW_VSLIDE,
		  list->total_h,
		  list->wi->wa.g_h,
		  list->start_y))
		rm |= 1;


	if (XA_slider(list->wi, XAW_HSLIDE,
		  list->widest,
		  list->wi->wa.g_w,
		  list->start_x))
		rm |= 2;

	if (rm && rdrw)
	{
		struct xa_window *wind = list->pw;
		if (!wind)
			wind = list->wt->wind;
		if (wind && wind->rect_list.start)
		{
			check_movement(list);
			if (rm & 1)
			{
				display_widget(0, list->wi, get_widget(list->wi, XAW_VSLIDE), wind->rect_list.start);
			}
			if (rm & 2)
			{
				struct xa_widget *info = get_widget(list->wi, XAW_INFO);
				info->xlimit = -list->start_x;
				display_widget(0, list->wi, info, wind->rect_list.start);
				display_widget(0, list->wi, get_widget(list->wi, XAW_HSLIDE), wind->rect_list.start);
			}
		}
	}
}

static struct scroll_entry *
get_last_entry(SCROLL_INFO *list, short flags, short maxlevel, short *level)
{
	struct scroll_entry *this = list->start;
	struct scroll_entry *last = NULL;

	while (this)
	{
		last = this;
		this = next_entry(this, flags, maxlevel, level);
	}
	return last;
}

static struct scroll_entry *
get_next_state(struct scroll_info *list, struct scroll_entry *this, short bits)
{
	this = next_entry(this, 0, -1, NULL);
	while (this)
	{
		if (this->state & bits)
			break;
		this = next_entry(this, 0, -1, NULL);
	}
	return this;
}

static struct scroll_entry *
get_first_state(SCROLL_INFO *list, short bits)
{
	struct scroll_entry *this = list->start;

	while (this)
	{
		if (this->state & bits)
			break;
		this = next_entry(this, 0, -1, NULL);
	}
	return this;
}
#if 0
static struct scroll_entry *
get_next_selected(struct scroll_info *list, struct scroll_entry *this)
{
	this = next_entry(this, 0, -1, NULL);
	while (this)
	{
		if (this->state & OS_SELECTED)
			break;
		this = next_entry(this, 0, -1, NULL);
	}
	return this;
}

static struct scroll_entry *
get_first_selected(SCROLL_INFO *list)
{
	struct scroll_entry *this = list->start;

	while (this)
	{
		if (this->state & OS_SELECTED)
			break;
		this = next_entry(this, 0, -1, NULL);
	}
	return this;
}
#endif

static bool
hidden_entry(SCROLL_ENTRY *this)
{
	bool hidden = false;

	this = this->up;
	while (this)
	{
		if (!(this->xstate & OS_OPENED))
		{
			hidden = true;
			break;
		}
		this = this->up;
	}
	return hidden;
}

static void
unselect_all(SCROLL_INFO *list, short redraw)
{
	SCROLL_ENTRY *this;
	this = get_first_state(list, OS_SELECTED);
	while (this)
	{
		change_entry(list, this, false, redraw ? !hidden_entry(this) : redraw);
		this = get_next_state(list, this, OS_SELECTED);
	}
}

static short
find_widest(SCROLL_INFO *list, SCROLL_ENTRY *start)
{
	short widest = 0;

	if (!start)
		start = list->start;

	while (start)
	{
		if (start->r.g_w > widest)
			widest = start->r.g_w;
		start = next_entry(start, ENT_VISIBLE, -1, NULL);
	}
	return widest;
}


static int
get_entry_lrect(struct scroll_info *l, struct scroll_entry *e, short flags, LRECT *r)
{
	long x = 0, y = 0, w = 0, h = 0;
	struct scroll_entry *this = l->start;
	short level = 0;

	PRDEF( get_entry_lrect, next_entry );

	while (this)
	{
		if (e == this)
			break;
		y += this->r.g_h;
		if( (l->flags & SIF_TREEVIEW) /*|| this->down*/ ){
			PROFRECs( this =, next_entry,(this, ENT_VISIBLE, -1, NULL));
		}
		else
			this = this->next;
	}
	if (this)
	{
		w = this->r.g_w;
		h = this->r.g_h;
		if ((flags & 1))
		{
			while (PROFRECa(this =, next_entry,(this, (ENT_VISIBLE|ENT_ISROOT), -1, &level)))
				w += this->r.g_w, h += this->r.g_h;
#if 0
			this = next_entry(this, (ENT_VISIBLE|ENT_ISROOT), -1, &level);
			if (this && level)
			{
				level = 0;
				while (this)
				{
					w += this->r.g_w, h += this->r.g_h;
					this = next_entry(this, (ENT_VISIBLE|ENT_ISROOT), -1, &level);
				}
			}
#endif
		}
		r->x = x;
		r->y = y;
		r->w = w;
		r->h = h;
		return 1;
	}
	else
	{
		r->w = r->h = 0L;
		return 0;
	}
}
/*
 * If entry == NULL, redraw whole box,
 * else redraw only the given entry.
 * If scroll_list has no parent window, nothing is redrawn,
 * as then we do not have a rectangle list to use.
 */
static void
slist_redraw(SCROLL_INFO *list, SCROLL_ENTRY *entry)
{
	struct xa_window *wind = list->pw;
	struct xa_rect_list *rl;

	if (!wind)
		wind = list->wt->wind;

	if (entry)
	{
		LRECT r;
		get_entry_lrect(list, entry, 0, &r);
		if (!(r.w | r.h) || ((r.y + r.h) < list->start_y) || (r.y > (list->start_y + list->wi->wa.g_h)))
		{
			return;
		}
	}
	else
	{
		reset_listwi_widgets(list, true);
		list->slider(list, true);
	}


	if (wind && ((wind->window_status & (XAWS_OPEN|XAWS_SHADED|XAWS_HIDDEN)) == XAWS_OPEN) && (rl = wind->rect_list.start))
	{
		check_movement(list);

		hidem();
		while (rl)
		{
			draw_slist(0, list, entry, &rl->r);
			rl = rl->next;
		}
		(*wind->vdi_settings->api->clear_clip)(wind->vdi_settings);
		/*if (!entry){
			list->flags &= ~SIF_DIRTY;
		}*/
		showm();
	}
}

static bool
canblit(SCROLL_INFO *list)
{
	bool ret = false;
	struct xa_window *wind = list->pw;
	struct xa_rect_list *rl = NULL;

	if (!wind)
		wind = list->wt->wind;
	if (wind)
	{
		GRECT r;
		rl = wind->rect_list.start;
		while (rl)
		{
			if (xa_rect_chk(&rl->r, &list->wi->wa, &r) == 2)
			{
				ret = true;
				break;
			}
			rl = rl->next;
		}
	}
	return ret;
}

static void
set_font(struct xa_fnt_info *src, struct xa_fnt_info *dst, struct xa_fnt_info *def)
{

	if (src->font_id >= 0)
		dst->font_id = src->font_id;
	else if (def)
		dst->font_id = def->font_id;

	if (src->font_point >= 0)
		dst->font_point = src->font_point;
	else if (def)
		dst->font_point = def->font_point;

	if (src->wr_mode >= 0)
		dst->wr_mode = src->wr_mode;
	else if (def)
		dst->wr_mode = def->wr_mode;

	if (src->effects >= 0)
		dst->effects = src->effects;
	else if (def)
		dst->effects = def->effects;

	if (src->fg >= 0)
		dst->fg = src->fg;
	else if (def)
		dst->fg = def->fg;

	if (src->bg >= 0)
		dst->bg = src->bg;
	else if (def)
		dst->bg = def->bg;
};
/*
 * Check if entry has its own wtxt structure or if its using the global
 * default one. If using default, allocate mem for and copy the default
 * one over. Need this so that changing one entry's fnt info dont affect
 * all others
 */
static void
alloc_entry_wtxt(struct scroll_entry *entry, struct xa_wtxt_inf *newwtxt)
{
	if (!(entry->iflags & SEF_WTXTALLOC))
	{
		struct xa_wtxt_inf *old = entry->fnt;
		entry->fnt = kmalloc(sizeof(struct xa_wtxt_inf));
		if (entry->fnt)
		{
			entry->iflags |= SEF_WTXTALLOC;

			if (newwtxt)
			{
				entry->fnt->flags = newwtxt->flags;
				set_font(&newwtxt->normal, &entry->fnt->normal, &default_fnt.normal);
				set_font(&newwtxt->selected, &entry->fnt->selected, &default_fnt.selected);
				set_font(&newwtxt->highlighted, &entry->fnt->highlighted, &default_fnt.highlighted);

			}
			else if (old)
				*entry->fnt = *old;
			else
				*entry->fnt = *(struct xa_wtxt_inf *)&default_fnt;
		}
		else
		{
			entry->fnt = old;
			return;
		}
	}
	else if (newwtxt)
		*entry->fnt = *newwtxt;
}

/*
 * Look for entrys that have any of 'bits' statebits set,
 * use mask and newbits to set new state
 */
static void
changestate_all(SCROLL_INFO *list, short bits, short mask, short newbits, bool rdrw)
{
	SCROLL_ENTRY *this;
	this = get_first_state(list, bits);
	while (this)
	{
		short state = this->state & ~mask;
		state |= newbits;
		if (this->state != state)
		{
			this->state = state;
			if (rdrw && !hidden_entry(this))
				list->redraw(list, this);
		}
		this = get_next_state(list, this, bits);
	}
}

static void
seset_state(SCROLL_INFO *list, SCROLL_ENTRY *this, short newstate, short mask, bool rdrw)
{
	short chs = 0;
	short state = this->state;

	if (!(list->flags & SIF_MULTISELECT) && (mask & OS_SELECTED) && (newstate & OS_SELECTED) && !(state & OS_SELECTED))
		chs |= OS_SELECTED;
	if (!(list->flags & SIF_MULTIBOXED) && (mask & OS_BOXED) && (newstate & OS_BOXED) && !(state & OS_BOXED))
		chs |= OS_BOXED;

	state &= ~mask;
	state |= newstate;

	if (chs)
	{
		changestate_all(list, chs, chs, 0, rdrw);
	}

	if (this->state != state)
	{
		this->state = state;
		if (rdrw && !hidden_entry(this)) list->redraw(list, this);
	}
}

static struct se_content *
get_entry_setext(struct scroll_entry *this, short idx)
{
	struct se_content *p = NULL;

	if (this)
	{
		p = this->content;
		if (idx == -1)
		{
			while (p && p->type != SECONTENT_TEXT)
				p = p->next;
		}
		else
		{
			while (p && !(p->index == idx && p->type == SECONTENT_TEXT))
				p = p->next;
		}
	}
	return p;
}

static struct se_content *
get_entry_next_setext(struct se_content *this_sc)
{
	this_sc = this_sc->next;
	while (this_sc && this_sc->type != SECONTENT_TEXT)
		this_sc = this_sc->next;
	return this_sc;
}

static void
free_seicon(struct se_content *seicon)
{
}

static void
free_setext(struct se_content *setext)
{
	if (setext->c.text.flags & SETEXT_ALLOC)
	{
		kfree(setext->c.text.text);
	}
}

static void
remove_se_content(struct scroll_entry *this, struct se_content *this_sc, struct se_content **prev, struct se_content **next)
{
	if (prev) *prev = this_sc->prev;
	if (next) *next = this_sc->next;

	if (this_sc->prev)
		this_sc->prev->next = this_sc->next;
	else
		this->content = this_sc->next;

	if (this_sc->next)
		this_sc->next->prev = this_sc->prev;
}

static void
delete_se_content(struct scroll_entry *this, struct se_content *sc, struct se_content **prev, struct se_content **next)
{
	remove_se_content(this, sc, prev, next);
	switch (sc->type)
	{
		case SECONTENT_TEXT:
			free_setext(sc);	break;
		case SECONTENT_ICON:
			free_seicon(sc);	break;
		default:;
	}
	if (sc->fnt)
		kfree(sc->fnt);
	if (sc->col)
		kfree(sc->col);
	if( !next )
		kfree(sc);
}

static void
delete_all_content(struct scroll_entry *this)
{
	struct se_content *this_sc = this->content, *n, *s = this_sc;

	while (this_sc)
	{
		delete_se_content(this, this_sc, NULL, &n);
		this_sc = n;
	}
	kfree(s);
}
#if 0
static void
insert_se_content(struct scroll_entry *this, struct se_content *this_sc, short index)
{
	struct se_content *sc = this->content;
	short last_idx;

	this_sc->index = index;

	if (!sc)
	{
		this->content = this_sc;
		last_idx = index;
	}
	else
	{
		while ((last_idx = sc->index) < index && sc->next)
		{
			sc = sc->next;
		}

		/*
		 * If we got to the index, we insert before
		 */
		if (last_idx >= index)
		{
			if ((this_sc->prev = sc->prev))
			{
				sc->prev->next = this_sc;
			}
			else
			{
				this->content = this_sc;
			}
			this_sc->next = sc;
			sc->prev = this_sc;

			if (last_idx == index)
			{
				this_sc = this_sc->next;
				while (this_sc && this_sc->index == last_idx)
				{
					this_sc->index++;
					last_idx++;
					this_sc = this_sc->next;
				}
			}
		}
		/*
		 * else we insert after
		 */
		else
		{
			this_sc->prev = sc;
			if ((this_sc->next = sc->next))
			{
				sc->next->prev = this_sc;
			}
			sc->next = this_sc;
		}
	}
}
#endif
static void
remove_setext_icon(struct se_content *this_sc)
{
	if (this_sc->c.text.icon.icon)
	{
		this_sc->c.text.icon.icon = NULL;
	}
}

static void
set_setext_icon(struct se_content *this_sc, OBJECT *icon)
{
	this_sc->c.text.icon.icon = icon;
	icon->ob_x = icon->ob_y = 0;
	ob_spec_xywh(icon, 0, &this_sc->c.text.icon.r);
	this_sc->c.text.icon.r.g_w += 2;
	this_sc->c.text.icon.r.g_h += 2;
}

#define PRFSE	1
static struct se_content *
new_setext(const char *t, OBJECT *icon, short type, short *sl, char **tp)
{
	struct se_content *new = NULL;
	short slen, tblen;
#if PRFSE
	PRDEF(new_setext,set_setext_icon);
	PRDEF(new_setext,memcpy);
	PRDEF(new_setext,xaaes_kmalloc);
#if USEOWNSTRLEN
	PRDEF(new_setext,xa_strlen);
#endif
#endif
	/* Ozk:
	 * On second thought, lets always copy the text
	 */
#if PRFSE && USEOWNSTRLEN
	PROFRECs(slen =, xa_strlen,(t));
#else
	slen = strlen(t);
#endif
	tblen = slen < 44 ? 44 : slen;
	if( sl )
		*sl = slen;

	if( tp )
	{
		/* append new item-data to buffer (save malloc) */
		new = (struct se_content *)*tp;
		*tp += sizeof(struct se_content) + tblen + 1;
		*tp += ((long)*tp & 1);	/* force even address! */
	}
	else
	{
#if PRFSE
		PROFRECs(new =, xaaes_kmalloc,(sizeof(*new) + tblen + 1, FUNCTION));
#else
		new = xaaes_kmalloc(sizeof(*new) + tblen + 1, FUNCTION);
#endif
	}
	if (new)
	{
		/*bzero(new, sizeof(*new)); xaaes_kmalloc zeros */
		new->c.text.flags = SETEXT_TXTSTR;
		new->c.text.text = new->c.text.txtstr;
		/*
		 * this could copy more than is allocated?
		 * strcpy(new->c.text.txtstr, t);
		 */
#if PRFSE
		PROFRECv(memcpy,(new->c.text.txtstr, t, slen+1));
#else
		memcpy(new->c.text.txtstr, t, slen+1);
#endif
		if (icon){
#if PRFSE
			PROFRECv(set_setext_icon,(new, icon));
#else
			set_setext_icon(new, icon);
#endif
		}
		new->type = SECONTENT_TEXT;
		new->next = NULL;
		new->c.text.tblen = tblen;
		new->c.text.slen = slen;
	}
	return new;
}

#if 0
/*
 * create a new SECONTENT_ICON content
 */
static struct se_content *
new_seicon(OBJECT *icon)
{
	struct se_content *new;

	if ((new = kmalloc(sizeof(*new))))
	{
		/*bzero(new, sizeof(*new)); xaaes_kmalloc zeros */
		new->c.icon.icon = icon;
		icon->ob_x = icon->ob_y = 0;
		ob_spec_xywh(icon, 0, &new->c.icon.r);
		new->c.icon.r.w += 2;
		new->c.icon.r.h += 2;
		new->type = SECONTENT_ICON;
	}
	return new;
}
#endif
/*
 * Add a scroll entry content, type SECONTENT_TEXT
 */
static struct se_content *
insert_strings(struct scroll_entry *this, struct sc_text *t, OBJECT *icon, short type)
{
	struct se_content *this_sc, *ret = NULL;
	int i, j = t->strings;
	short slen;
	const char *s = t->text;
	char tbuf[2048], *tp = tbuf, *ep = tbuf + sizeof(tbuf);
	size_t sz;
	PRDEF(insert_strings,new_setext);

	memset( tbuf, 0, sizeof(tbuf));
	for (i = 0; i < j && tp < ep; i++)
	{
		PROFRECs(this_sc =, new_setext,(s, icon, type, &slen, &tp));
		if( !this_sc )
			return 0;
		s += slen + 1;
		if (icon)
			icon = NULL;
	}

	sz = tp - tbuf;
	this->content = ret = this_sc = xaaes_kmalloc(sz, FUNCTION);
	memcpy( this_sc, tbuf, sz );

	this_sc->c.text.text = this_sc->c.text.txtstr;
	if( j == 1 )
		this_sc->next = this_sc->prev = 0;
	else
	{
		this_sc = ret;
		this_sc->prev = 0;
		for ( i = 1; i <= j; i++ )
		{
			struct se_content *next = (struct se_content *)((char*)this_sc + this_sc->c.text.tblen + sizeof(struct se_content) + 1);
			next = (struct se_content *)((char*)next + ((long)next & 1));		/* force even address! (see new_setext) */
			this_sc->c.text.text = this_sc->c.text.txtstr;
			this_sc->index = i - 1;
			if( i == j )
			{
				this_sc->next = 0;
				break;
			}
			next->prev = this_sc;
			this_sc->next = next;
			this_sc = next;

		}
	}

	return ret;
}
#if 0
static struct se_content *
insert_icon(struct scroll_entry *this, short index, OBJECT *icon)
{
	struct se_content *this_sc = NULL;

	if (icon)
	{
		this_sc = new_seicon(icon);
		if (this_sc)
		{
			insert_se_content(this, this_sc, index);
		}
	}
	return this_sc;
}
#endif
static int
set(SCROLL_INFO *list,
    SCROLL_ENTRY *entry,
    short what,
    unsigned long arg,
    short rdrw)
{
	bool redrw = false;
	int ret = 0;

	switch (what)
	{
		case SESET_STATE:
		{
			short newstate = arg, mask = arg >> 16;

			if (entry)
				seset_state(list, entry, newstate, mask, rdrw);
			else
			{
				entry = list->start;
				while (entry)
				{
					seset_state(list, entry, newstate, mask, rdrw);
					entry = next_entry(entry, 0, -1, NULL);
				}
			}
			break;
		}
		case SESET_M_STATE:
		{
			struct sesetget_params *p = (void *)arg;
			short state, maxlevel, flags;

			if (p)
			{
				flags = p->level.flags;
				maxlevel = p->level.maxlevel;

				if (!entry)
				{
					if ((entry = list->start))
					{
						if ((flags & ENT_ISROOT) && maxlevel != -1)
						{
							if ((maxlevel -= 1) < 0)
								goto m_state_done;
						}
						flags &= ~(ENT_ISROOT|ENT_INCROOT);
					}
					else
						goto m_state_done;
				}

				if ((flags & (ENT_ISROOT|ENT_INCROOT)) == ENT_ISROOT)
					entry = next_entry(entry, flags, maxlevel, &p->level.curlevel);
				while (entry)
				{
					state = entry->state;
					state &= p->arg.state.mask;
					state |= p->arg.state.bits;
					if (entry->state != state)
					{
						entry->state = state;
						if (rdrw && !hidden_entry(entry))
							list->redraw(list, entry);
					}
					entry = next_entry(entry, flags, maxlevel, &p->level.curlevel);
				}
			}
m_state_done:
			break;
		}
		case SESET_NFNT:
		{
			struct xa_fnt_info *f = (void*)arg;
			alloc_entry_wtxt(entry, NULL);
			if (entry->iflags & SEF_WTXTALLOC)
			{
				set_font(f, &entry->fnt->normal, NULL);
			}
				else ret = 0;
			break;
		}
		case SESET_SFNT:
		{
			struct xa_fnt_info *f = (void*)arg;
			alloc_entry_wtxt(entry, NULL);
			if (entry->iflags & SEF_WTXTALLOC)
			{
				set_font(f, &entry->fnt->selected, NULL);
			}
				else ret = 0;
			break;
		}
		case SESET_HFNT:
		{
			struct xa_fnt_info *f = (void*)arg;
			alloc_entry_wtxt(entry, NULL);
			if (entry->iflags & SEF_WTXTALLOC)
			{
				set_font(f, &entry->fnt->highlighted, NULL);
			}
				else ret = 0;
			break;
		}
		case SESET_SELECTED:
		{
			if (!(list->flags & SIF_MULTISELECT))
			{
				unselect_all(list, rdrw);
			}
			change_entry(list, entry, true, rdrw);
			break;
		}
		case SESET_UNSELECTED:
		{
			if (arg == UNSELECT_ALL)
				unselect_all(list, rdrw);
			else if (arg == UNSELECT_ONE)
				change_entry(list, entry, false, rdrw);
			break;
		}
		case SESET_MULTISEL:
		{
			if (arg)
				list->flags |= SIF_MULTISELECT;
			else
				list->flags &= ~SIF_MULTISELECT;
			break;
		}
		case SESET_OPEN:
		{
			short xstate = entry->xstate;
			LRECT r;
			GRECT s, d;

			if (arg)
			{
				xstate |= OS_OPENED;

				if (xstate != entry->xstate)
				{
					entry->xstate = xstate;

					if (entry->down)
					{
						get_entry_lrect(list, entry, 1, &r);
						if ((r.w || r.h))
						{
							list->total_h += (r.h - entry->r.g_h);
							if (list->flags & SIF_TREEVIEW)
								list->total_w = list->widest = find_widest(list, NULL);
							if (!reset_listwi_widgets(list, rdrw))
							{
								if ((r.y + entry->r.g_h) < list->start_y)
									list->start_y += (r.h - entry->r.g_h);
								else if (rdrw && canredraw(list) && (r.y < (list->start_y + list->wi->wa.g_h)))
								{
									if (canblit(list))
									{
										s = d = list->wi->wa;
										s.g_y += ((r.y + entry->r.g_h) - list->start_y);
										d.g_y += ((r.y + r.h) - list->start_y);
										d.g_h -= ((r.y + r.h) - list->start_y);
										s.g_h = d.g_h;
										hidem();
										if (d.g_h > 4)
										{
											(*list->vdi_settings->api->form_copy)(&s, &d);
											s.g_h = d.g_y - s.g_y;
										}
										else
										{
											s.g_h += (d.g_y - s.g_y);
										}
										draw_slist(0, list, NULL, &s);
										showm();
										(*list->vdi_settings->api->clear_clip)(list->vdi_settings);
									}
									else{
										list->redraw(list, NULL);
									}
								}
							}
						}
						if (rdrw != NOREDRAW)
							list->slider(list, rdrw);
					}
				}
			}
			else
			{
				xstate &= ~OS_OPENED;
				if (xstate != entry->xstate)
				{
					get_entry_lrect(list, entry, 1, &r);
					entry->xstate = xstate;
					if (entry->down)
					{
						if ((r.w || r.h))
						{
							list->total_h -= (r.h - entry->r.g_h);
							list->total_w = list->widest = find_widest(list, NULL);
							if (!reset_listwi_widgets(list, rdrw))
							{
								if ((r.y + r.h) < list->start_y)
									list->start_y -= (r.h - entry->r.g_h);
								else if (rdrw && canredraw(list) && r.y < (list->start_y + list->wi->wa.g_h))
								{
									if (canblit(list))
									{
										s = d = list->wi->wa;
										s.g_y += ((r.y + r.h) - list->start_y);
										s.g_h -= ((r.y + r.h) - list->start_y);
										hidem();
										if (s.g_h > 4)
										{
											d.g_y += ((r.y + entry->r.g_h) - list->start_y);
											d.g_h = s.g_h;
											(*list->vdi_settings->api->form_copy)(&s, &d);
											d.g_h = s.g_y - d.g_y;
											d.g_y += s.g_h;
										}
										else
										{
											d.g_y += ((r.y + entry->r.g_h) - list->start_y);
											d.g_h -= ((r.y + entry->r.g_h) - list->start_y);
										}

										if (d.g_h > 0)
										{
											draw_slist(0, list, NULL, &d);
										}

										showm();
										(*list->vdi_settings->api->clear_clip)(list->vdi_settings);
									}
									else{
										list->redraw(list, NULL);
									}
								}
							}
						}
						list->slider(list, rdrw);
					}
				}
			}
			list->redraw(list, entry);
			break;
		}
		case SESET_TAB:
		{
			struct seset_tab *tab = (void *)arg;
			struct se_tab *tabs = list->tabs;
			short idx = tab->index;

			if (idx < list->num_tabs)
			{
				tabs[idx].flags |= tab->flags;
				tabs[idx].v = tab->r;
				recalc_tabs(list);

			}
			else
				ret = 0;
			break;
		}
		case SESET_USRFLAGS:
		{
			if (entry)
				entry->usr_flags = arg;
			break;
		}
		case SESET_USRDATA:
		{
			if (entry)
				entry->data = (void *)arg;
			break;
		}
		case SESET_TREEVIEW:
		{
			if (arg)
				list->flags |= SIF_TREEVIEW;
			else
				list->flags &= ~SIF_TREEVIEW;
			if (rdrw)
			{
				list->redraw(list, NULL);
			}
			break;
		}
		case SESET_TEXT:
		{
			struct setcontent_text *t = (struct setcontent_text *)arg;

			if (t && t->text && entry)
			{
				bool frdrw = false;
				struct se_content *setext;
				short slen;

				setext = get_entry_setext(entry, t->index);
				if (setext)
				{
					slen = strlen(t->text);
					if (slen <= setext->c.text.tblen || setext->next || setext->prev)
					{
						strncpy(setext->c.text.text, t->text, setext->c.text.tblen);
					}
					else
					{
						struct se_content *new = new_setext(t->text, setext->c.text.icon.icon, 0,0, 0);
						if (new)
						{
							delete_se_content(entry, setext, NULL, NULL);
							entry->content = new;
						}
						else
						{
							strncpy(setext->c.text.text, t->text, setext->c.text.tblen);
						}
					}
					setext->c.text.slen = slen;
				}
				else
				{
					struct se_content *sc = entry->content, *scc;
					struct se_content *new = new_setext(t->text, NULL, 0, 0, 0);
					if (new)
					{
							new->index = t->index;
					}
					if( !sc )
					{
						entry->content = new;
						setext = new;
					}
					else if( new )
					{
						for( scc = 0; sc && sc->index < t->index; sc = sc->next )
							scc = sc;

						if( sc == entry->content )
						{
							new->next = sc;
							sc->prev = new;
							entry->content = new;
						}
						else if( sc )
						{
							sc->prev->next = new;
							new->next = sc;
							new->prev = sc->prev;
							sc->prev = new;
						}
						else if( scc )
						{
							scc->next = new;
							new->prev = scc;
						}
					}
				}
				if (setext)
				{
					if (t->icon)
					{
						if (t->icon == (void *)-1L)
							remove_setext_icon(setext);
						else
						{
							remove_setext_icon(setext);
							set_setext_icon(setext, t->icon);
						}
					}
					if (t->fnt)
					{
						if (!setext->fnt)
							setext->fnt = kmalloc(sizeof(*setext->fnt));
						if (setext->fnt)
							*setext->fnt = *t->fnt;
					}
					frdrw = calc_entry_wh(list, entry);
				}
				if (rdrw)
				{
					list->redraw(list, frdrw ? NULL : entry);
				}
			}
			break;
		}
		case SESET_CURRENT:
		{
			if (entry != list->cur)
			{
				if (!(entry && is_in_list(list, entry)))
					entry = NULL;
				list->cur = entry;
			}
			break;
		}
		case SESET_ACTIVE:
		{
			if (entry != list->cur)
			{
				if (!(entry && is_in_list(list, entry)))
					entry = NULL;
				if ((list->cur = entry))
				{
					entry_action(list, list->cur, NULL);
				}
			}
			break;
		}
		case SESET_PRNTWIND:
		{
			struct xa_window *parent = (struct xa_window *)arg;

			if (parent)
			{
				list->pw = parent;
			}
			if (list->wi)
				list->wi->parent = parent;
			break;
		}
	}

	if (redrw)
	{
		LRECT r;
		get_entry_lrect(list, entry, 0, &r);
		if (((r.y + r.h) > list->start_y) && (r.y < (list->start_y + list->wi->wa.g_h)))
			list->redraw(list, entry);
	}
	return ret;
}

static int
get(SCROLL_INFO *list, SCROLL_ENTRY *entry, short what, void *arg)
{
	long ret = 0;

	if (arg)
	{
		ret = 1;

		switch (what)
		{
			case SEGET_STATE:
			{
				*(short *)arg = entry->state;
				break;
			}
			case SEGET_XSTATE:
			{
				*(short *)arg = entry->xstate;
				break;
			}
			case SEGET_NFNT:
			{
				struct xa_fnt_info *f = arg;
				*f = entry->fnt->normal;
				break;
			}
			case SEGET_SFNT:
			{
				struct xa_fnt_info *f = arg;
				*f = entry->fnt->selected;
				break;
			}
			case SEGET_HFNT:
			{
				struct xa_fnt_info *f = arg;
				*f = entry->fnt->highlighted;
				break;
			}
			case SEGET_WTXT:
			{
				struct xa_wtxt_inf *wtxt = arg;
				*wtxt = *(struct xa_wtxt_inf *)&default_fnt;
				break;
			}
			/*
			 * Return entry at index 'idx', starting at level below 'entry'
			 * If 'entry' is NULL, search start at root-level.
			 * If 'idx' entry is not reached, a NULL is returned as 'e' while
			 * the number of entries are returned in 'idx'
			 * idx starts with 1 to ...
			 */
			case SEGET_ENTRYBYIDX:
			{
				struct sesetget_params *p = arg;
				struct scroll_entry *this = entry;
				int idx = p->idx;

				if (!this)
					this = list->start;
				else
					this = this->down;

				if (this)
				{
					idx--;
					while (idx && this->next)
					{
						this = this->next;
						idx--;
					}
					if (idx)
						this = NULL;
				}
				p->idx -= idx;
				p->e = this;
				break;
			}
			case SEGET_ENTRYBYDATA:
			{
				struct sesetget_params *p = arg;
				struct scroll_entry *this = entry;

				if (!this)
					this = list->start;
				while (this)
				{
					if (this->data == p->arg.data)
						break;
					this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
				}
				if (!(p->e = this))
					ret = 0L;
				break;
			}
			case SEGET_ENTRYBYTEXT:
			{
				struct sesetget_params *p = arg;
				struct se_content *setext;
				struct scroll_entry *this = entry;

				if (!this)
					this = list->start;

				while (this)
				{
					if (p->idx == -1)
					{
						setext = get_entry_setext(this, -1);
						while (setext)
						{
							if (!strcmp(p->arg.txt, setext->c.text.text))
								break;
							setext = get_entry_next_setext(setext);
						}
						if (setext)
							break;
					}
					else
					{
						setext = get_entry_setext(this, p->idx);
						if (setext)
						{
							if (!strcmp(p->arg.txt, setext->c.text.text))
								break;
						}
					}
					this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
				}
				if (!(p->e = this))
					ret = 0L;
				break;
			}
			case SEGET_ENTRYBYSTATE:
			{
				struct sesetget_params *p = arg;
				struct scroll_entry *this = entry;
				short state, bits, mask;

				if (!this)
					this = list->start;
				mask = (short)p->arg.state.mask;
				bits = (short)p->arg.state.bits;
				switch (p->arg.state.method)
				{
					case ANYBITS:
					{
						while (this)
						{
							state = this->state & mask;
							if ((state & bits))
								break;
							this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
						}
						break;
					}
					case EXACTBITS:
					{
						while (this)
						{
							state = this->state & mask;
							if (state == bits)
								break;
							this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
						}
						break;
					}
					default:
						this = NULL;
				}
				if (!(p->e = this))
					ret = 0L;
				break;
			}
			case SEGET_ENTRYBYXSTATE:
			{
				struct sesetget_params *p = arg;
				struct scroll_entry *this = entry;
				short state, bits, mask;

				if (!this)
					this = list->start;
				mask = (short)p->arg.state.mask;
				bits = (short)p->arg.state.bits;
				switch (p->arg.state.method)
				{
					case ANYBITS:
					{
						while (this)
						{
							state = this->xstate & mask;
							if ((state & bits))
								break;
							this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
						}
						break;
					}
					case EXACTBITS:
					{
						while (this)
						{
							state = this->xstate & mask;
							if (state == bits)
								break;
							this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
						}
						break;
					}
					default:
						this = NULL;
				}
				if (!(p->e = this))
					ret = 0L;
				break;
			}
			case SEGET_ENTRYBYUSRFLAGS:
			{
				struct sesetget_params *p = arg;
				struct scroll_entry *this = entry;
				long flags, bits, mask;

				if (!this)
					this = list->start;

				mask = p->arg.usr_flags.mask;
				bits = p->arg.usr_flags.bits;
				switch (p->arg.state.method)
				{
					case ANYBITS:
					{
						while (this)
						{
							flags = this->usr_flags & mask;
							if ((flags & bits))
								break;
							this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
						}
						break;
					}
					case EXACTBITS:
					{
						while (this)
						{
							flags = this->usr_flags & mask;
							if (flags == bits)
								break;
							this = next_entry(this, p->level.flags, p->level.maxlevel, &p->level.curlevel);
						}
						break;
					}
					default:
						this = NULL;
				}
				if (!(p->e = this))
					ret = 0L;
				break;
			}

			case SEGET_LISTXYWH:
			{
				get_list_rect(list, (GRECT *)arg);
				break;
			}
			case SEGET_PRNTWIND:
			{
				struct xa_window **rwind = arg;
				*rwind = list->pw;
				break;
			}
			case SEGET_TAB:
			{
				struct seset_tab *tab = arg;
				struct se_tab *tabs = list->tabs;
				short idx = tab->index;

				if (idx < list->num_tabs)
				{
					tab->flags = tabs[idx].flags;
					tab->r = tabs[idx].v;
				}
				else
					ret = 0;
				break;
			}
			case SEGET_SELECTED:
			{
				if (entry)
					*(struct scroll_entry **)arg = get_next_state(list, entry, OS_SELECTED);
				else
					*(struct scroll_entry **)arg = get_first_state(list, OS_SELECTED);
				break;
			}
			case SEGET_USRFLAGS:
			{
				if (arg && entry)
					*(long *)arg = entry->usr_flags;
				break;
			}
			case SEGET_USRDATA:
			{
				if (arg && entry)
					*(void **)arg = entry->data;
				break;
			}
			case SEGET_NEXTENT:
			{
				struct sesetget_params *p = arg;

				if (entry)
				{
					p->e = next_entry(entry, p->level.flags, p->level.maxlevel, &p->level.curlevel);
				}
				break;
			}
			case SEGET_PREVENT:
			{
				struct sesetget_params *p = arg;

				if (entry)
				{
					if (!p->level.maxlevel)
						p->e = entry->prev;
					else
						p->e = prev_entry(entry, p->level.flags);
				}
				break;
			}
			case SEGET_TEXTCPY:
			{
				struct sesetget_params *p = arg;
				struct se_content *setext;

				if ((setext = get_entry_setext(entry, p->idx)))
				{
					strcpy(p->arg.txt, setext->c.text.text);
				}
				else
					ret = 0;
				break;
			}
			case SEGET_TEXTCMP:
			{
				struct sesetget_params *p = arg;
				struct se_content *setext;

				if ((setext = get_entry_setext(entry, p->idx)))
					p->ret.ret = strcmp(p->arg.txt, setext->c.text.text);
				else
					ret = 0;
				break;
			}
			case SEGET_TEXTPTR:
			{
				struct sesetget_params *p = arg;
				struct se_content *setext;

				if ((setext = get_entry_setext(entry, p->idx)))
					p->ret.ptr = setext->c.text.text;
				else
					ret = 0;
				break;
			}
			case SEGET_CURRENT:
			{
				if (arg)
					*(struct scroll_entry **)arg = list->cur;
				break;
			}
			default:
			{
				ret = 0;
				break;
			}
		}
	}
	return ret;
}

#ifndef FLAG_FILLED	/* cmp xa_fsel.c#67 */
#define FLAG_FILLED 0x00000001
#define FLAG_DIR  0x00000002
#define FLAG_EXE  0x00000004
#define FLAG_LINK 0x00000008
#define FLAG_SDIR 0x00000010
#endif
static void
sort_entry(SCROLL_INFO *list, SCROLL_ENTRY **start, SCROLL_ENTRY *new, scrl_compare *greater)
{

	PRDEF(sort_entry,greater);
	if (*start)
	{
		bool not_is_dir = !(new->usr_flags & (FLAG_SDIR|FLAG_DIR));
		SCROLL_ENTRY *c = *start, *here, *cp=0;

		if( !(list->flags & SIF_TREEVIEW) )
		{
			if( not_is_dir )
			{
				/* 1st check if new is greatest cmp to eolist. TODO flag if dir is sorted */
				for( here = cp = c; cp; here = cp, cp = cp->next )
					;
				if( PROFREC(greater,(list, new, here)) ){
					*start = here;	/* done */
					return;
				}
			}
			else
			{
				c = list->start;	/* if dir start search from list->start (greater should give more information) */
			}
		}

		/* new > c */
		if ( PROFREC(greater,(list, new, c)) )
		{
			here = c;
			c = c->next;

			while (c)
			{
				/* while new > c */
				if (PROFREC(greater,(list, new, c)) )
				{
					here = c;
					c = c->next;
				}
				else /* new < c */
				{
					/* insert before c */
					break;
				}
			}
		}
		else /* new <= c */
		{
			c = c->prev;
			while (c)
			{
				/* while new < c */
				if (PROFREC(greater,(list, c, new)) )
				{
					c = c->prev;
				}
				else /* new > c */
				{
					/* insert after c */
					break;
				}
			}
			here = c;
		}
		*start = here;
	}
	else
		*start = list->start;
}
/* better control over the content of scroll_entry. */
static int
add_scroll_entry(SCROLL_INFO *list,
		 SCROLL_ENTRY *parent,
		 scrl_compare *sort,
		 struct scroll_content *sc, /*OBJECT *icon, void *text,*/
		 short addmode,
		 SCROLL_ENTRY_TYPE type,
		 short redraw)
{
	struct scroll_entry *here, *new;
	LRECT r = {0,0,0,0};
	if (!parent || is_in_list(list, parent))
	{
		bool fullredraw = false;
		PRDEF(add_scroll_entry,insert_strings);
		PRDEF(add_scroll_entry,alloc_entry_wtxt);
		PRDEF(add_scroll_entry,get_entry_lrect);
		PRDEF(add_scroll_entry,reset_listwi_widgets);
		PRDEF(add_scroll_entry,next_entry);
		PRDEF(add_scroll_entry,form_copy);
		PRDEF(add_scroll_entry,redraw);
		PRDEF(add_scroll_entry,slider);
		PRDEF(add_scroll_entry,calc_entry_wh);
		PRDEF(add_scroll_entry,sort_entry);
		PRDEF(add_scroll_entry,canredraw);
		PRDEF(add_scroll_entry,canblit);

#ifndef __GNUC__
		if( memcpy == 0 )
			memcpy = (*KENTRY->vec_libkern.memcpy);
#endif

		/*
		 * mem for the new entry...
		 */
		new = kmalloc(sizeof(*new));

		if (!new)
			return 0;

		PROFRECv(insert_strings,(new, &sc->t, sc->icon, type));

		new->usr_flags = sc->usr_flags;
		new->data = sc->data;
		new->data_destruct = sc->data_destruct;

		new->xstate = sc->xstate;
		new->xflags = sc->xflags;

		if (sc->fnt){
			PROFRECv(alloc_entry_wtxt,(new, sc->fnt));
		}
		else
			new->fnt = &default_fnt;


		if (!(new->col = sc->col))
			new->col = &default_col;

		new->type = type;

		new->indent = list->indent;

		if( list->start )
		{
			((struct se_content *)new->content)->c.text.h = list->start->r.g_h;
		}
		PROFRECv(calc_entry_wh,(list, new));

		if (parent)
		{
			if ((addmode & SEADD_CHILD))
			{
				here = parent->down;
			}
			else
				here = parent;
		}
		else
			here = list->start;

		if (!here)
		{
			if ((addmode & SEADD_CHILD))
			{
				parent->down = new;
				new->up = parent;
			}
			else
				list->start = list->top = new;

			list->cur = here = new;
		}
		else
		{
			if (sort)
			{
				bool usecur = false;

				if( !(list->flags & SIF_TREEVIEW)/* && list->cur*/ ){
					here = list->cur;
					usecur = true;
				}
				PROFRECv(sort_entry,(list, &here, new, sort));

				if( here )
				{
					here->r.g_y += here->r.g_h;
					r.y = (long)here->r.g_y;
				}
				/*else
					r.y = 0L;*/


				if( usecur == true )
					list->cur = new;

				if (!here)
				{
					if (parent)
						here = parent->down;
					else
						here = list->start;
					addmode |= SEADD_PRIOR;
				}
				else
					addmode &= ~SEADD_PRIOR;
			}
			else
			{
				if (parent)
				{
					/*
					 * If we have a parent but 'here' is not equal to it,
					 * 'here' points to 'parent's first child - we now need
					 * to find the last entry at that level and append 'new'
					 * there.
					 */
					if (here != parent && !(addmode & SEADD_PRIOR))
					{
						while (here->next)
							here = here->next;
					}
					/* else if here == parent means that 'here' points to the entry after
					 * which we will insert the 'new' entry.
					 */
				}
				else if (!(addmode & SEADD_PRIOR))
				{
					while (here->next)
						here = here->next;
				}
			}

			new->up = here->up;
			if (addmode & SEADD_PRIOR)
			{
				new->next = here;
				if (!(new->prev = here->prev))
				{
					if (here->up)
						here->up->down = new;
					else
						list->start = new;
				}
				else
					new->prev->next = new;

				here->prev = new;
			}
			else
			{
				if (here->next)
					here->next->prev = new;
				new->next = here->next;
				here->next = new;
				new->prev = here;
			}
		}

		if (new->up)
		{

			if (!new->up->level)
				new->root = new->up;
			else
				new->root = new->up->root;

			new->level = new->up->level + 1;
			new->indent += (new->level * list->nesticn_w);
			if (list->flags & SIF_TREEVIEW)
			{
				if (new->r.g_h < list->nesticn_h)
					new->r.g_h = list->nesticn_h;
				if (new->r.g_w < list->nesticn_w)
					new->r.g_w = list->nesticn_w;
			}
		}

		if ( (list->flags & SIF_TREEVIEW))
		{
			PROFRECv(get_entry_lrect,(list, new, 0, &r));
		}
		else
		{
			/*r.x = new->r.g_x;
			r.w = new->r.g_w;*/
			r.h = new->r.g_h;

			if( !sort )
			{
				if( here )
				{
					/*here->r.g_y += here->r.g_h;
					r.y = (long)here->r.g_y;
					*/
					new->r.g_y = here->r.g_y + r.h;
					r.y = (long)new->r.g_y;
				}
				/*else
					r.y = 0L;*/
			}
		}

		if (new->r.g_w > list->widest)
			list->widest = new->r.g_w;
		if ((r.h /*| r.w*/))
		{
			/* compensate for not calling next_entry in get_entry_lrect...*/
			/*if( !sort )
			{
				new->r.h += LSLINDST_PTS;
			}*/

			if (new->r.g_h > list->highest)
				list->highest = new->r.g_h;
			list->total_h += new->r.g_h;


			if (r.y < (list->start_y - list->off_y))
			{
				list->start_y += r.h;

				if( !redraw )
					return 1;

				PROFRECv(reset_listwi_widgets,(list, redraw));
			}
			else if (r.y <  (list->start_y + list->wi->wa.g_h))
			{
				if (r.y == (list->start_y - list->off_y))
				{
					if (list->off_y)
						list->start_y += r.h;
					else
						list->top = new;
				}

				if( !redraw )
					return 1;

				if( !PROFREC( reset_listwi_widgets,(list, redraw)) && redraw)
				{
					if (PROFREC(canredraw,(list)))
					{
						if (!PROFREC(canblit,(list)) || fullredraw )
						{
							PROFRECs(list->,redraw,(list, NULL));
							fullredraw = false;
						}
						else
						{
							short sy = (r.y - list->start_y);
							if (sy >= 0)
							{
								struct scroll_entry *next;
								hidem();
								PROFRECs(next =, next_entry,(new, ENT_VISIBLE, -1, NULL));
								if (next)
								{
									GRECT d, s = list->wi->wa;

									s.g_y += sy;
									s.g_h -= (sy + r.h);
									d = s;
									d.g_y += r.h;
									if (d.g_h > 0){
										PROFRECp(*list->vdi_settings->api->,form_copy,(&s, &d));
									}
								}
								PROFRECs(list->,redraw,(list, new));
								showm();
							}
						}
					}
					else
						redraw = 0;
				}
			}
			else
			{
				if( !redraw )
					return 1;
				PROFRECv(reset_listwi_widgets,(list, redraw));
			}
#if 0
			if (fullredraw)
			{
				if (!reset_listwi_widgets(list, redraw) && canredraw(list) && redraw)
					list->redraw(list, NULL);
			}
#endif
			if( redraw != NOREDRAW )
				PROFRECs(list->,slider,(list, redraw));
		}

		return 1;
	}
	return 0;
}


static void
free_se(struct scroll_entry *this)
{
	if ((this->iflags & SEF_WTXTALLOC))
	{
		kfree(this->fnt);
	}

	if (this->data_destruct)
		(*this->data_destruct)(this->data);
	kfree(this);
}

/*
 * Delete scroll entry
 */
static SCROLL_ENTRY *
free_scroll_entry(struct scroll_info *list, struct scroll_entry *this, long *height)
{
	struct scroll_entry *stop = this, *next;
	short level = 1;
	long h = 0;

	if (!this)
	{
		return this;
	}

	this = this->down;

	while (this && level)
	{
		if (this->down)
		{
			this = this->down;
			level++;
		}
		else
		{
			next = this->next;
			if ((this->type & SETYP_STATIC))
			{
				if (!next)
				{
					next = this->up;
					level--;
					while (next && level)
					{
						if (next->next)
						{
							next = next->next;
							break;
						}
						next = next->up;
						level--;
					}
				}
			}
			else
			{
				if (next)
					next->prev = this->prev;
				if (this->prev)
					this->prev->next = next;
				else
				{
					if (this->up)
						this->up->down = next;
					else
						list->start = next;
				}
				if (!next)
				{
					next = this->up;
					level--;
				}

				if (this == list->cur)
				{
					list->cur = NULL;
				}
				if (this == list->top)
				{
					list->top = NULL;
				}

				delete_all_content(this);

				h += this->r.g_h;

				free_se(this);
			}
			this = next;
		}
	}

	this = stop;
	next = this->next;
	if (!(this->type & SETYP_STATIC))
	{
		if (next)
			next->prev = this->prev;
		if (this->prev)
			this->prev->next = next;
		else
		{
			if (this->up)
				this->up->down = next;
			else
				list->start = next;
		}
	}
	if (!next)
	{
		next = this->up;
		level--;
		while (next)
		{
			if (next->next)
			{
				next = next->next;
				break;
			}
			next = next->up;
			level--;
		}
	}

	if (!(this->type & SETYP_STATIC))
	{
		if (this == list->cur)
			list->cur = NULL;
		if (this == list->top)
			list->top = NULL;

		delete_all_content(this);

		h += this->r.g_h;
		free_se(this);
	}

	if (height)
		*height = h;

	return next;
}

static struct scroll_entry *
del_scroll_entry(struct scroll_info *list, struct scroll_entry *e, short redraw)
{

	struct scroll_entry *next, *prev;
	short sy = 0, dy = 0;
	long del_h;
	LRECT lr, r;

	get_entry_lrect(list, e, 1, &lr);

	if (!(prev = e->prev))
		prev = e->up;

	next = free_scroll_entry(list, e, &del_h);

	if (lr.y < (list->start_y - list->off_y))
	{
		list->total_h -= lr.h;

		if (list->total_h <= list->wi->wa.g_h)
		{
			list->top = list->start;

			if (!list->start_y && !list->off_y)
			{
				sy = lr.y + lr.h;
				dy = lr.y;
			}
			else
			{
				list->start_y = list->off_y = 0;
				sy = -1L;
			}
		}
		else
		{
			if ((lr.y + lr.h) < (list->start_y - list->off_y))
			{
				list->start_y -= lr.h;
			}
			else
			{
				if (next)
				{
					get_entry_lrect(list, next, 0, &r);
					list->top = next;
					sy = (lr.h + r.y) - list->start_y;
					if (sy > list->start_y + list->wi->wa.g_h)
						sy = -1L;
					dy = 0;
					list->start_y = r.y;
					list->off_y = 0;
				}
				else if (prev)
				{
					get_entry_lrect(list, prev, 0, &r);
					list->top = prev;
					list->start_y = r.y;
					list->off_y = 0;
					sy = -1L;
				}
				/* else the list is empty */
			}
		}
	}
	else
	{
		list->total_h -= lr.h;
		if (list->total_h <= list->wi->wa.g_h)
		{
			list->top = list->start;
			if (!list->start_y && !list->off_y)
			{
				sy = lr.y + lr.h;
				dy = lr.y;
			}
			else
			{
				list->start_y = list->off_y = 0;
				sy = -1L;
			}
		}
		else
		{
			if (lr.y == (list->start_y - list->off_y))
			{
				if (next)
				{
					list->top = next;
					get_entry_lrect(list, next, 0, &r);
					sy = (r.y + lr.h) - list->start_y;
					if (sy > list->wi->wa.g_h)
						sy = -1L;
					dy = 0;
					list->start_y = r.y;
					list->off_y = 0;
				}
				else if (prev)
				{
					list->top = prev;
					get_entry_lrect(list, prev, 0, &r);
					list->start_y = r.y;
					list->off_y = 0;
					sy = -1L;
				}
				/* else list is empty */
			}
			else if (lr.y < (list->start_y + list->wi->wa.g_h))
			{
				dy = lr.y - list->start_y;
				if ((lr.y + lr.h) < (list->start_y + list->wi->wa.g_h))
				{
					if (next)
					{
						get_entry_lrect(list, next, 0, &r);
						sy = (r.y + lr.h) - list->start_y;
						if (sy > list->wi->wa.g_h)
							sy = -1l;
					}
					else
					{
						sy = (lr.y + lr.h) - list->start_y;
						if (sy > list->wi->wa.g_h)
							sy = -1L;
					}
				}
				else
					sy = -1L;
			}
		}
	}
	if (lr.w == list->widest)
	{
		list->widest /*= list->total_w */= find_widest(list, NULL);
	}

	if (!reset_listwi_widgets(list, redraw/*true*/) && redraw && sy)
	{
		short h;
		GRECT s, d;

		hidem();
		if (!canblit(list) || sy == -1L)
		{
			list->redraw(list, NULL);
		}
		else
		{
			s = list->wi->wa;
			h = s.g_h - sy;
			if (h < 8)
			{
				s.g_y += dy;
				s.g_h -= s.g_h - dy;
				draw_slist(0, list, NULL, &s);
			}
			else
			{
				s.g_h = h;
				d = s;
				s.g_y += sy;
				d.g_y += dy;
				(*list->vdi_settings->api->form_copy)(&s, &d);
				d.g_y += (d.g_h - 1);
				d.g_h = list->wi->wa.g_h - (d.g_y - list->wi->wa.g_y) + 1;
				draw_slist(0, list, NULL, &d);
			}
			(*list->vdi_settings->api->clear_clip)(list->vdi_settings);
		}
		showm();
	}

	list->slider(list, redraw);
	return next;
}	/*/del_scroll_entry*/

/* Modified such that a choice can be made. */
static void
empty_scroll_list(SCROLL_INFO *list, SCROLL_ENTRY *this, SCROLL_ENTRY_TYPE type)
{
	if (!this)
	{
		this = list->start;
		while (this)
		{
			if (type == -2)
				this->type &= ~SETYP_STATIC;

			this = free_scroll_entry(list, this, 0);
#if 0
			if (type == -2 || ((this->type & type) || type == -1))
				this = del_scroll_entry(list, this, false);
			else
				this = next_entry(this, ENT_ISROOT, -1, &level);
#endif
		}
		list->total_h =	list->start_y = list->off_y = 0;
		list->widest = 0;
		list->start = list->top = list->cur = 0;
	}
	else
	{
		SCROLL_ENTRY *next = this->next;
		this = this->down;
		while (this && this != next)
			this = del_scroll_entry(list, this, false);
	}
}

#define OS_SLID	0x8000	/* mark that scroll-message came from slider */

static void
scroll_up(SCROLL_INFO *list, long num, bool rdrw)
{
	long n, h;
	struct scroll_entry *this, *next;

	DIAG((D_fnts, NULL, "scroll_up: %ld lines, top = %lx",
		num, (unsigned long)list->top));

	n = list->total_h - (list->start_y + list->wi->wa.g_h);
	if (n <= 0)
		return;

	this = list->top;

	if( n > num )
	{
		n = num;
	}
	if( !(list->flags & OS_SLID) )
	{
		/* always scroll up whole lines */
		h = this->r.g_h;
		n = (n + h/2) / h;
		if( n < 1 )
			n = 1;
		n *= h;
	}
	else
		list->flags &= ~OS_SLID;

	while (n > 0 && (next = next_entry(this, ENT_VISIBLE, -1, NULL)))
	{
		h = this->r.g_h;
		if (list->off_y)
			h -= list->off_y;

		n -= h;

		if (n < 0)
		{
			list->off_y = n + this->r.g_h;
			list->start_y += (n + h);
		}
		else
		{
			list->off_y = 0;
			list->start_y += h;
			this = next;
		}
	}
	list->top = this;

	if (rdrw)
	{
#if 0
		if ( !canblit(list) || max > (list->wi->wa.h - 8)*/)
#endif
			list->redraw(list, NULL);
#if 0
		else
		{
		/*
		 * XXX - Ozk: This assumes there's nothing covering our slist window"
		 */
			GRECT s, d;

			hidem();
			s = list->wi->wa;
			s.h -= max;
			d = s;
			s.y += max;

			(*list->vdi_settings->api->form_copy)(&s, &d);

			d.y += (d.h - 1);
			d.h = max + 1;
			draw_slist(0, list, NULL, &d);
			(*list->vdi_settings->api->clear_clip)(list->vdi_settings);
			showm();
		}
#endif
		list->slider(list, true);
	}
}	/*/scroll_up*/

static void
scroll_down(SCROLL_INFO *list, long num, bool rdrw)
{
	SCROLL_ENTRY *prev;
	long max = num, n, h;

	DIAG((D_fnts, NULL, "scroll_down: %ld lines, top = %lx",
		num, (unsigned long)list->top));

	if (max > list->start_y)
		max = list->start_y;

	n = max;

	if (list->top)
	{
		while (n > 0 && ((prev = prev_entry(list->top, ENT_VISIBLE)) || list->off_y))
		{
			if (list->off_y)
			{
				h = list->off_y;
				n -= h;
				if (n < 0)
				{
					list->off_y -= (n + h);
					list->start_y -= (n + h);
				}
				else
				{
					list->off_y = 0;
					list->start_y -= h;
				}
			}
			else
			{
				list->top = prev;
				h = (list->top->r.g_h - list->off_y);

				n -= h;
				if (n < 0)
				{
					list->off_y = list->top->r.g_h - (n + h);
					list->start_y -= (n + h);
				}
				else
				{
					list->off_y = 0;
					list->start_y -= h;
				}
			}
		}
	}
	if (rdrw)
	{
		if (!canblit(list) || max > (list->wi->wa.g_h - 8))
		{
			list->redraw(list, NULL);
		}
		else
		{
			GRECT s, d;

			s = list->wi->wa;

			s.g_h -= max;
			d = s;
			d.g_y += max;
			hidem();
			(*list->vdi_settings->api->form_copy)(&s, &d);
			s.g_h = max;
			draw_slist(0, list, NULL, &s);
			(*list->vdi_settings->api->clear_clip)(list->vdi_settings);
			showm();
		}
		list->slider(list, true);
	}
}	/*/scroll_down*/

static void
scroll_left(SCROLL_INFO *list, long num, bool rdrw)
{
	long n, max;

	if (list->widest < list->wi->wa.g_w)
		return;

	if ((num + list->start_x + list->wi->wa.g_w) > list->widest)
	{
		max = n = list->widest - (list->start_x + list->wi->wa.g_w);
		if (max < 0)
			return;
	}
	else
		max = n = num;

	list->start_x += n;

	if (rdrw)
	{
		if (!canblit(list) || max > list->wi->wa.g_w - 8)
		{
			list->redraw(list, NULL);
		}
		else
		{
			GRECT s, d;

			s = list->wi->wa;
			s.g_w -= max;
			d = s;
			s.g_x += max;
			hidem();
			(*list->vdi_settings->api->form_copy)(&s, &d);
			d.g_x += d.g_w;
			d.g_w = max;
			draw_slist(0, list, NULL, &d);
			(*list->vdi_settings->api->clear_clip)(list->vdi_settings);
			showm();
		}
		list->slider(list, true);
	}
}	/*/scroll_left*/

static void
scroll_right(SCROLL_INFO *list, long num, bool rdrw)
{
	long n, max;

	if (list->widest < list->wi->wa.g_w)
		return;

	if (num > list->start_x)
		max = n = list->start_x;
	else
		max = n = num;

	list->start_x -= n;

	if (rdrw)
	{
		if ( !canblit(list) || max > list->wi->wa.g_w - 8)
		{
			list->redraw(list, NULL);
		}
		else
		{
			GRECT s, d;

			s = list->wi->wa;
			s.g_w -= max;
			d = s;
			d.g_x += max;

			hidem();
			(*list->vdi_settings->api->form_copy)(&s, &d);
			s.g_w = max;
			draw_slist(0, list, NULL, &s);
			(*list->vdi_settings->api->clear_clip)(list->vdi_settings);
			showm();
		}
		list->slider(list, true);
	}
}	/*/scroll_right*/


static void
visible(SCROLL_INFO *list, SCROLL_ENTRY *s, short redraw)
{
	LRECT r;

	DIAGS(("scrl_visible: start_y=%ld, height=%d, total =%ld",
		list->start_y, list->wi->wa.g_h, list->total_h));

	if (get_entry_lrect(list, s, 0, &r))
	{
		bool rdrw = false;
		if ((r.w | r.h))
		{
			check_movement(list);

			if (redraw == NORMREDRAW)
				rdrw = true;

			if ((r.y + r.h) <= list->start_y || r.y > (list->start_y + list->wi->wa.g_h))
			{
				/*
				 * If the entry to make visible is the next-up, we only scroll enough
				 * to make that visible, i.e., do not scroll so that it is placed
				 * in at the center.
				 */
				if ((r.y + r.h) == list->start_y)
				{
					scroll_down(list, r.h, rdrw);
				}
				else
				{
					/*
					 * We want to place the entry as close as possible to the center
					 * of the list (vertical center, that is)
					 */
					r.y -= list->start_y;
					if (r.y < 0)
						scroll_down(list, -r.y + (list->wi->wa.g_h >> 1), rdrw);
					else
					{
						r.y -= list->wi->wa.g_h;
						scroll_up(list, r.y + (list->wi->wa.g_h >> 1), rdrw);
					}
				}
				rdrw = false;
			}
			else if (r.y < list->start_y)
			{
				/*
				 * The entry is partially visible, scroll just enough to make it the first
				 * visible in the list
				 */
				scroll_down(list, list->off_y, rdrw);
			}
			else if ((r.y + r.h-1) > (list->start_y + list->wi->wa.g_h))
			{
				/*
				 * The entry is partially visible at the end of the list, scroll just
				 * enough to make it visible as the last line in the list
				 *
				 * now: scroll half page
				 */
				scroll_up(list, list->wi->wa.g_h / 2, rdrw);
			}
			if (rdrw){
				list->redraw(list, s);
			}

			if (redraw == FULLREDRAW){
				list->redraw(list, NULL);
			}

			list->slider(list, redraw);
		}
	}
}	/*/visible*/

static SCROLL_ENTRY *
search(SCROLL_INFO *list, SCROLL_ENTRY *start, short mode, void *data)
{
	SCROLL_ENTRY *ret = NULL;

	if (!start)
		start = list->start;

	switch (mode)
	{
		case SEFM_BYDATA:
		{
			/*
			 * Find entry based on data ptr
			 */
			while (start)
			{
				if (start->data == data)
					break;
				start = next_entry(start, 0, -1, NULL);
			}
			ret = start;
			break;
		}
		case SEFM_BYTEXT:
		{
			struct se_content *setext;

			while (start)
			{
				setext = get_entry_setext(start, -1);
				while (setext)
				{
					if (!strcmp(setext->c.text.text, data))
						break;
					setext = get_entry_next_setext(setext);
				}
				if (setext)
					break;
				start = next_entry(start, 0, -1, NULL);
			}
			ret = start;
			break;
		}
		case SEFM_LAST:
		{
			ret = get_last_entry(list, 0, -1, NULL);
			break;
		}
	}
	return ret;
}

static void
entry_action(struct scroll_info *list, struct scroll_entry *this, const struct moose_data *md)
{
	if (!this)
		this = list->cur;

	if (this)
	{
		short ni_x = (list->wi->wa.g_x + this->indent) - list->off_x;
		LRECT r;

		if ((list->flags & SIF_TREEVIEW) && (md && md->x > ni_x && md->x < (ni_x + list->nesticn_w)) )
		{
			if ((this->xflags & OF_AUTO_OPEN))
			{
				if (this->xstate & OS_OPENED)
					list->set(list, this, SESET_OPEN, 0, NORMREDRAW);
				else
					list->set(list, this, SESET_OPEN, 1, NORMREDRAW);
			}
			if (list->click_nesticon)
				list->click_nesticon(list, this, md);
		}
		else
		{
			list->cur = this;
			if (list->flags & SIF_AUTOSELECT)
			{
				if (!(list->flags & SIF_MULTISELECT))
					list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
				list->set(list, this, SESET_SELECTED, 0, NOREDRAW);
				get_entry_lrect(list, this, 0, &r);
				list->vis(list, this, NORMREDRAW);
			}
			if (list->click)			/* Call the new object selected function */
				(*list->click)(list, this, md);
		}
	}
#if 0
	else
	{
		if (list->click)
			list->click(list, this, md);
	}
#endif
}

/* HR 181201: pass all mouse data to these functions using struct moose_data. */
void
click_scroll_list(int lock, OBJECT *form, int item, const struct moose_data *md)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	OBJECT *ob = form + item;
	short cy = md->y;

	list = (SCROLL_INFO *)object_get_spec(ob)->index;

	DIAGS(("click_scroll_list: state=%d, cstate=%d", md->state, md->cstate));
	check_movement(list);
	if (!do_widgets(lock, list->wi, 0, md))
	{
		long y;

		if (md->cstate & MBS_RIGHT)
		{
			short mb, mx, my, px, py;
			S.wm_count++;
			check_mouse(list->wi->owner, &mb, &px, &py);
			if (mb)
			{
				xa_graf_mouse(FLAT_HAND, NULL, NULL, false);

				while (mb)
				{
					long num;
					wait_mouse(list->wi->owner, &mb, &mx, &my);

					if (!mb)
						break;

					if (mx != px)
					{
						if ((num = mx - px) < 0)
							scroll_left(list, -num, true);
						else
							scroll_right(list, num, true);
						px = mx;
					}
					if (my != py)
					{
						if ((num = my - py) < 0)
							scroll_up(list, -num, true);
						else
							scroll_down(list, num, true);
						py = my;
					}
				}
				set_winmouse(-1, -1);
			}
			S.wm_count--;
			return;
		}

		cy -= list->wi->wa.g_y;

		if ((this = list->top))
		{
			y = list->top->r.g_h - list->off_y;

			while (this && y < cy)
			{
				this = next_entry(this, ENT_VISIBLE, -1, NULL);
				if (this) y += this->r.g_h;
			}
		}
		if (this)
			entry_action(list, this, md);
		else if (list->click)
			list->click(list, this, md);
#if 0
		if (this)
		{
			short ni_x = (list->wi->wa.x + this->indent) - list->off_x;
			LRECT r;

			if ((list->flags & SIF_TREEVIEW) && md->x > ni_x && md->x < (ni_x + list->nesticn_w))
			{
				if ((this->xflags & OF_AUTO_OPEN))
				{
					if (this->xstate & OS_OPENED)
						list->set(list, this, SESET_OPEN, 0, NORMREDRAW);
					else
						list->set(list, this, SESET_OPEN, 1, NORMREDRAW);
				}
				if (list->click_nesticon)
					list->click_nesticon(list, this, md);
			}
			else
			{
				list->cur = this;
				if (list->flags & SIF_AUTOSELECT)
				{
					if (!(list->flags & SIF_MULTISELECT))
						list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
					list->set(list, this, SESET_SELECTED, 0, NOREDRAW);
					get_entry_lrect(list, this, 0, &r);
					list->vis(list, this, NORMREDRAW);
				}
				if (list->click)			/* Call the new object selected function */
					(*list->click)(list, this, md);
			}
		}
		else
		{
			if (list->click)
				list->click(list, this, md);
		}
#endif
	}
}

void
dclick_scroll_list(int lock, OBJECT *form, int item, const struct moose_data *md)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	OBJECT *ob = form + item;
	long y;
	short cx = md->x, cy = md->y;

	list = (SCROLL_INFO *)object_get_spec(ob)->index;
	check_movement(list);
	if (!do_widgets(lock, list->wi, 0, md))		/* HR 161101: mask */
	{
		if (!m_inside(cx, cy, &list->wi->wa))
			return;

		cy -= list->wi->wa.g_y;

		if ((this = list->top))
		{
			y = list->top->r.g_h - list->off_y;
			while(this && y < cy)
			{
				this = next_entry(this, ENT_VISIBLE, -1, NULL);
				if (this) y += this->r.g_h;
			}
		}

		if (this)
			list->cur = this;

		if (list->dclick)
			(*list->dclick)(list, this, md);
	}
}

static bool
drag_vslide(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *imd)
{
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;

	DIAGS(("scroll object vslide drag!"));

	if (imd->cstate)
	{
		short offs;
		struct moose_data m = *imd;
		struct moose_data *md = &m;
		short msg[8] = { WM_VSLID, 0, 0, wind->handle, 0, 0,0,0};

		if (md->cstate & MBS_RIGHT)
		{
			GRECT s, b, d, r;

			rp_2_ap(wind, widg, &widg->ar);

			b = s = widg->ar;

			s.g_x += sl->r.g_x;
			s.g_y += sl->r.g_y;
			s.g_w = sl->r.g_w;
			s.g_h = sl->r.g_h;

			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				lock_screen(wind->owner->p, false);
			xa_graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			if (!(wind->owner->status & CS_NO_SCRNLOCK))
				unlock_screen(wind->owner->p);

			offs = bound_sl(pix_to_sl(r.g_y - widg->ar.g_y, widg->r.g_h - sl->r.g_h));

			if (offs != sl->position)
			{
				sl->rpos = msg[4] = offs;
				slist_msg_handler(wind, NULL, 0,0, msg);
			}
		}
		else
		{
			short y, old_y = 0, old_offs = 0, pmx, pmy;
			short cont = 0;

			S.wm_count++;
			while (md->cstate)
			{
				if (!cont)
				{
					cont = true;
					/* Always have a nice consistent sizer when dragging a box */
					xa_graf_mouse(XACRS_VERTSIZER, NULL, NULL, false);
					rp_2_ap(wind, widg, &widg->ar);
					old_offs = md->y - (widg->ar.g_y + sl->r.g_y);
					old_y = md->y;
					y = md->sy;
				}
				else
					y = md->y;

				if (old_y != y)
				{
					offs = bound_sl(pix_to_sl((y - old_offs) - widg->ar.g_y, widg->r.g_h - sl->r.g_h));

					if (offs != sl->position && wind->send_message)
					{
						sl->rpos = msg[4] = offs;
						slist_msg_handler(wind, NULL, 0,0, msg);
					}
					old_y = y;
				}
				pmx = md->x, pmy = md->y;
				while (md->cstate && md->x == pmx && md->y == pmy)
				{
					wait_mouse(wind->owner, &md->cstate, &md->x, &md->y);
				}
			}
			S.wm_count--;
		}
		xa_graf_mouse(wind->owner->mouse, NULL, NULL, false);
	}
	return true;
}
static bool
drag_hslide(int lock, struct xa_window *wind, struct xa_widget *widg, const struct moose_data *imd)
{
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;

	DIAGS(("scroll object vslide drag!"));

	if (imd->cstate)
	{
		short offs;
		struct moose_data m = *imd;
		struct moose_data *md = &m;
		short msg[8] = { WM_HSLID, 0, 0, wind->handle, 0, 0,0,0};

		if (md->cstate & MBS_RIGHT)
		{
			GRECT s, b, d, r;

			rp_2_ap(wind, widg, &widg->ar);

			b = s = widg->ar;

			s.g_x += sl->r.g_x;
			s.g_y += sl->r.g_y;
			s.g_w = sl->r.g_w;
			s.g_h = sl->r.g_h;

			lock_screen(wind->owner->p, false);
			xa_graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
			drag_box(wind->owner, s, &b, rect_dist_xy(wind->owner, md->x, md->y, &s, &d), &r);
			unlock_screen(wind->owner->p);

			offs = bound_sl(pix_to_sl(r.g_x - widg->ar.g_x, widg->r.g_w - sl->r.g_w));

			if (offs != sl->position)
			{
				sl->rpos = msg[4] = offs;
				slist_msg_handler(wind, NULL, 0,0, msg);
			}
		}
		else
		{
			short x, old_x = 0, old_offs = 0, pmx;
			short cont = 0;

			S.wm_count++;
			while (md->cstate)
			{
				if (!cont)
				{
					cont = true;
					/* Always have a nice consistent sizer when dragging a box */
					xa_graf_mouse(XACRS_HORSIZER, NULL, NULL, false);
					rp_2_ap(wind, widg, &widg->ar);
					old_offs = md->x - (widg->ar.g_x + sl->r.g_x);
					old_x = md->x;
					x = md->sx;
				}
				else
					x = md->x;

				if (old_x != x)
				{
					offs = bound_sl(pix_to_sl((x - old_offs) - widg->ar.g_x, widg->r.g_w - sl->r.g_w));

					if (offs != sl->position && wind->send_message)
					{
						sl->rpos = msg[4] = offs;
						slist_msg_handler(wind, NULL, 0,0, msg);
					}
					old_x = x;
				}
				pmx = md->x;
				while (md->cstate && md->x == pmx)
				{
					wait_mouse(wind->owner, &md->cstate, &md->x, &md->y);
				}
			}
			S.wm_count--;
		}
		xa_graf_mouse(wind->owner->mouse, NULL, NULL, false);
	}
	return true;
}

static void
unset_G_SLIST(struct scroll_info *list)
{
	struct scroll_entry *this;
	OBJECT *ob = list->wt->tree + list->item;

	DIAG((D_objc, NULL, "unset_G_SLIST: list=%lx, obtree=%lx, index=%d",
		(unsigned long)list, (unsigned long)list->wt->tree, list->item));
	this = list->start;
	while (this)
	{
		this->type &= ~SETYP_STATIC;
		this = free_scroll_entry(list, this, NULL);
	}

	if (list->wi)
	{
		close_window(0, list->wi);
		delete_window(0, list->wi);
		list->wi = NULL;
	}

	if (list->wt)
	{
		list->wt = NULL;
	}

	if (list->tabs)
	{
		kfree(list->tabs);
	}

	*ob = list->prev_ob;
	if (list->flags & SIF_KMALLOC)
		kfree(list);
	else
		ufree(list);
}

/* preparations for windowed list box widget;
 * most important is to get the drawing smooth and simple.
 * get rid of all those small (confolded) constant value's.
 */
SCROLL_INFO *
set_slist_object(int lock,
		 XA_TREE *wt,
		 struct xa_window *parentwind,
		 short item,
		 short flags,

		 scrl_widget *closer,
		 scrl_widget *fuller,
		 scrl_click *dclick,
		 scrl_click *click,
		 scrl_click *click_nesticon,
		 scrl_keybd *key,

		 scrl_add	*add,
		 scrl_del	*del,
		 scrl_empty	*empty,
		 scrl_list	*destroy,

		 char *title,
		 char *info,
		 void *data,
		 short lmax)	/* Used to determine whether a horizontal slider is needed. */
{
	int i;
	struct scroll_info *list;
	struct se_tab *tabs;
	struct widget_tree *nil_wt;
	GRECT r;
	XA_WIND_ATTR wkind = 0;
	OBJECT *ob = wt->tree + item;

	DIAG((D_rsrc, NULL, "set_slist_object"));

	list = kmalloc(sizeof(*list) + sizeof(*nil_wt));
	tabs = kmalloc(sizeof(*tabs) * 10);

	if (!list || !tabs)
	{
		if (list)
			kfree(list);
		if (tabs)
			kfree(tabs);
		return NULL;
	}
	list->icon_w = list->char_width = 0;

	nil_wt = (struct widget_tree *)((char *)list + sizeof(*list));

	/*bzero(list, sizeof(*list) + sizeof(*nil_wt)); xaaes_kmalloc zeros */

	clear_edit(&wt->e);
	nil_wt->ei = NULL;
	clear_focus(nil_wt);
	list->nil_wt = nil_wt;

	list->tabs = tabs;
	list->num_tabs = 10;
	for (i = 0; i < 10; i++)
		tabs[i] = default_setab;	/* 0 */

	list->flags |= SIF_KMALLOC | SIF_DIRTY;

	/* colours are those for windows */
	list->prev_ob = *ob;

	object_set_spec(wt->tree + item, (unsigned long)list);
	ob->ob_type = G_SLIST;
	ob->ob_flags |= OF_TOUCHEXIT;

	if (flags & SIF_SELECTABLE)
		ob->ob_flags |= OF_SELECTABLE;
	else
		ob->ob_flags &= ~OF_SELECTABLE;

	list->flags |= (flags & ~SIF_KMALLOC);

	list->item = item;

	list->pw = parentwind;

	list->wt = wt;

	list->data = data;

	list->dclick = dclick;			/* Pass the scroll list's double click function */
	list->click = click;			/* Pass the scroll list's click function */
	list->click_nesticon = click_nesticon;
	list->keypress = key ? key : scrl_cursor;

	list->title = title;
	obj_area(wt, aesobj(wt->tree, item), &r);
	list->rel_x = r.g_x - wt->tree->ob_x;
	list->rel_y = r.g_y - wt->tree->ob_y;

	if (!(flags & SIF_AUTOSLIDERS))
	{
		wkind |= UPARROW|VSLIDE|DNARROW;
		if (lmax * screen.c_max_w + ICON_W > r.g_w - 24)
			wkind |= LFARROW|HSLIDE|RTARROW;
	}

	if (title)
		wkind |= NAME;
	if (info)
		wkind |= INFO;
	if (closer)
		wkind |= CLOSER;
	if (fuller)
		wkind |= FULLER;


	wkind |= TOOLBAR;

	list->wi = create_window(lock,
				 do_winmesag,
				 slist_msg_handler,
				 wt->owner,
				 true,			/* nolist */
				 wkind,
				 created_for_AES | created_for_SLIST,
				 0, true/*false*/, &r, NULL, NULL);
	if (list->wi)
	{
		int dh;

		if( wt->extra )
		{
			list->next = wt->extra;
		}
		wt->extra = list;
		wt->flags |= WTF_EXTRA_ISLIST;
		list->vdi_settings = list->wi->vdi_settings;
		list->wi->parent = list->pw;
		if (title)
			set_window_title(list->wi, title, false);
		if (info)
			set_window_info(list->wi, info, false);

		get_widget(list->wi, XAW_VSLIDE)->m.drag = drag_vslide;
		get_widget(list->wi, XAW_HSLIDE)->m.drag = drag_hslide;

		list->wi->winob	= wt->tree;		/* The parent object of the windowed list box */
		list->wi->winitem = item;
		r = list->wi->wa;
		dh = list->wi->wa.g_h - r.g_h;
		ob->ob_height -= dh;
		list->wi->r.g_h -= dh;

		list->slider	= sliders;
		list->closer	= closer;

		list->add	= add ? add		: add_scroll_entry;
		list->del	= del ? del		: del_scroll_entry;
		list->empty	= empty ? empty		: empty_scroll_list;
		list->destroy	= destroy ? destroy	: unset_G_SLIST;

		list->fuller	= fuller;
		list->vis	= visible;
		list->search	= search;
		list->set	= set;
		list->get	= get;
		list->redraw	= slist_redraw;
		list->lock	= lock;

		list->indent_upto = 1;
		list->indent = 2;
		list->nesticn_w = 16;
		/*if( cfg.xaw_point < 10 )
			list->nesticn_h = 10;
		else
		*/
		list->nesticn_h = 16;

		open_window(lock, list->wi, list->wi->r);
	}
	DIAGS((" -- return new slist %lx", (unsigned long)list));
	return list;
}

/*
 * set focus into window
 * if top==true set list->cur to list->top
 */
static void sl_set_selected( SCROLL_INFO *list, bool top, short redraw )
{
	if (list->cur)
	{
		list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ONE, NORMREDRAW);

		if( top == true )
			list->cur = list->top;
		if (list->flags & SIF_KEYBDACT)
			entry_action(list, list->cur, NULL);
		else
		{
			list->set(list, list->cur, SESET_SELECTED, 0, /*key?NOREDRAW:*/NORMREDRAW);
			list->vis(list, list->cur, redraw );
		}
	}
}
#if 0
static long round_to( long nl, long to )
{
	/* force whole lines */
	nl = (nl + to/2) / to;
	if( nl < 1 )
		nl = 1;
	nl *= to;
	return nl;
}
#endif
/* HR: The application point of view of the list box */
/* HR: Immensely cleanup of the code by using a window and it's mechanisms for the list box.
       Including fully functional sliders and all arrows.
*/
static void
slist_msg_handler(
	struct xa_window *wind,
	struct xa_client *to,
	short amq, short qmf,
	short *msg)
{
	SCROLL_INFO *list;
	SCROLL_ENTRY *top, *n;
	OBJECT *ob;
	long p;
	short amount, msgt;

	ob = wind->winob + wind->winitem;
	list = object_get_slist(ob);
	top = list->top;
	check_movement(list);
	switch (msg[0])		/* message number */
	{
	case WM_ARROWED:
	{
		if (top)
		{
			if ((msgt = msg[4] & 7) < WA_LFPAGE)
			{
				p = list->wi->wa.g_h;
				if (!(amount = (msg[4] >> 8) & 0xff))
					amount++;

				if (p < list->total_h)
				{
					switch (msgt)
					{
					case WA_UPLINE:
					{
						if ((n = prev_entry(top, ENT_VISIBLE)) || list->off_y)
						{
							if (list->off_y)
								scroll_down(list, list->off_y, true);
							else
							{
								long h = n->r.g_h;
								amount--;
								while (amount)
								{
									if (!(n = prev_entry(n, ENT_VISIBLE)))
										break;
									h += n->r.g_h;
									amount--;
								}
								scroll_down(list, h, true);
							}
						}
						break;
					}
					case WA_DNLINE:
					{
						if ((n = next_entry(top, ENT_VISIBLE, -1, NULL)))
						{
							if (list->off_y)
								scroll_up(list, top->r.g_h - list->off_y, true);
							else if (n)
							{
								long h = n->r.g_h;
								amount--;
								while (amount)
								{
									if (!(n = next_entry(n, ENT_VISIBLE, -1, NULL)))
										break;
									h += n->r.g_h;
									amount--;
								}
								scroll_up(list, h, true);
							}
						}
						break;
					}
					case WA_UPPAGE:
					{
						scroll_down(list, list->wi->wa.g_h - top->r.g_h, true);
						break;
					}
					case WA_DNPAGE:
					{
						/*
						short nl = list->wi->wa.h - top->r.h;

						nl = round_to( nl, top->r.h );
						scroll_up(list, nl, true);
						*/
						scroll_up(list, list->wi->wa.g_h - top->r.g_h, true);
						break;
					}
					}
				}
			}
			else
			{
				{
					switch (msg[4])
					{
						case WA_LFLINE:
							scroll_right(list, 4, true);
							break;
						case WA_RTLINE:
							scroll_left(list, 4, true);
							break;
						case WA_LFPAGE:
							scroll_right(list, list->wi->wa.g_w - 8, true);
							break;
						case WA_RTPAGE:
							scroll_left(list, list->wi->wa.g_w - 8, true);
					}
				}
			}
		}
		break;
	}
	case WM_VSLID:
	{
		if (top)
		{
			p = list->wi->wa.g_h;
			if (p < list->total_h)
			{
				long new = sl_to_pix(list->total_h - p, msg[4]);

				new -= list->start_y;
				if (new < 0)
					scroll_down(list, -new, true);
				else
				{
					list->flags |= OS_SLID;	/* came from slider */
					scroll_up(list, new, true);
				}
			}
		}
		break;
	}
	case WM_HSLID:
	{
		if (top)
		{
			p = list->wi->wa.g_w;
			if (p < list->widest )
			{
				long new = sl_to_pix(list->widest - p, msg[4]);
				new -= list->start_x;
				if (new < 0)
					scroll_right(list, -new, true);
				else
					scroll_left(list, new, true);
			}
		}
		break;
	}
	case WM_CLOSED:
	{
		if (list->closer)
			list->closer(list, true);
		break;
	}
	case WM_FULLED:
	{
		if (list->fuller)
			list->fuller(list, true);
	}
	case WM_SIZED:
	{
		move_window(0, wind, false, -1, msg[4], msg[5], msg[6], msg[7]);
		reset_listwi_widgets(list, false);
		list->slider(list, false);
		break;
	}
	case WM_UNTOPPED:
	case WM_ONTOP:
	{
		void *colours = (msg[0] == WM_UNTOPPED) ? wind->untop_cols : wind->ontop_cols;

		if( wind->colours != colours )
		{
			wind->colours = colours;
			display_window( 0, 13, wind, &wind->r);
		}

		break;
	}
	default:
	{
		return;
	}
	}	/*/switch (msg[0])		message number */
}


unsigned short
scrl_cursor(SCROLL_INFO *list, unsigned short keycode, unsigned short keystate)
{

	short msg[8] = { WM_ARROWED,0,0,list->wi->handle,0,0,0,0 };

	switch (keycode)
	{
	case SC_SPACE:
	case SC_INSERT:
		if (!get_first_state( list, OS_SELECTED))
		{
			SCROLL_ENTRY *this = list->top;
			list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
			list->cur = this;
			list->set(list, this, SESET_SELECTED, 0, NOREDRAW);
			list->vis(list, this, NORMREDRAW);
		}
		else
		{
			list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
			list->cur = NULL;
		}
		break;

	case SC_UPARROW: /* 0x4800 */		/* up arrow */
	{
		if (!list->cur)
		{
			msg[4] = WA_UPLINE;
			slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		}
		else
		{
			SCROLL_ENTRY *n = prev_entry(list->cur, ENT_VISIBLE);
			if (!n)
			{
				n = get_last_entry(list, ENT_VISIBLE, -1, NULL);
			}
			if (n)
			{
				if (list->flags & SIF_KEYBDACT)
				{
					entry_action(list, n, NULL);
				}
				else
				{
					list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
					list->cur = n;
					list->set(list, n, SESET_SELECTED, 0, NOREDRAW);
					list->vis(list, n, NORMREDRAW);
				}
			}
		}
		break;
	}
	case SC_DNARROW: /* 0x5000 */		/* down arrow */
	{
		if (!list->cur)
		{
			msg[4] = WA_DNLINE;
			slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		}
		else
		{
			SCROLL_ENTRY *n = next_entry(list->cur, ENT_VISIBLE, -1, NULL);
			if (!n)
				n = list->start;
			if (n)
			{
				if (list->flags & SIF_KEYBDACT)
				{
					entry_action(list, n, NULL);
				}
				else
				{
					list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
					list->cur = n;
					list->set(list, n, SESET_SELECTED, 0, NOREDRAW);
					list->vis(list, n, NORMREDRAW);
				}
			}
		}
		break;
	}
	case SC_LFARROW: /* 0x4B00 */			/* left arrow */
	{
		if ( !(list->flags & SIF_TREEVIEW) || !list->cur)
		{
			msg[4] = WA_LFLINE;
			slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		}
		else
		{
			if ((list->cur->xstate & OS_OPENED))
				list->set(list, list->cur, SESET_OPEN, 0, NORMREDRAW);
			else if (list->cur->up)
			{
				SCROLL_ENTRY *n = list->cur->up;
				if (n)
				{
					list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
					list->cur = n;
					list->set(list, n, SESET_SELECTED, 0, NOREDRAW);
					list->vis(list, n, NORMREDRAW);
				}
			}
		}
		break;
	}
#if 0
	case SC_CTRL_LFARROW: /* 0x7300 */	/* CTLR + left arrow */
	{
		if (list->cur)
		{
			struct scroll_entry *n = list->cur->up;
			if (n)
			{
				list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
				list->cur = n;
				list->set(list, n, SESET_SELECTED, 0, NOREDRAW);
				list->vis(list, n, NORMREDRAW);
			}
		}
		break;
	}
#endif
	case SC_RTARROW: /* 0x4D00 */		/* Right arrow */
	{
		struct scroll_entry *cur;
		if ( !(list->flags & SIF_TREEVIEW) || !(cur = list->cur))
		{
			msg[4] = WA_RTLINE;
			slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		}
		else
		{
			short xf = cur->xflags;
			short xs = cur->xstate;
			/*
			 * - If entry has children, autoopen when closed and OF_AUTO_OPEN set.
			 * - If entry has children, but is not OF_AUTO_OPEN, do nothing, so caller can detect
			 *   that entry has children, is closed and isnt auto open by checking that list->cur
			 *   doesnt change.
			 * - If entry is already opened or does not have children, go to next visible entry.
			 */
			if (cur->down && (xf & (OF_OPENABLE|OF_AUTO_OPEN)) == (OF_OPENABLE|OF_AUTO_OPEN) && !(xs & OS_OPENED))
			{
				list->set(list, cur, SESET_OPEN, 1, NORMREDRAW);
			}
			else if (!(xf & OF_OPENABLE) || ((xs & OS_OPENED)))
			{
				SCROLL_ENTRY *n = next_entry(cur, ENT_VISIBLE, -1, NULL);
				if (!n)
					n = list->start;
				if (n)
				{
					if (list->flags & SIF_KEYBDACT)
					{
						entry_action(list, n, NULL);
					}
					else
					{
						list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
						list->cur = n;
						list->set(list, n, SESET_SELECTED, 0, NOREDRAW);
						list->vis(list, n, NORMREDRAW);
					}
				}
			}
		}
		break;
	}
	case SC_PGUP:		/* 0x4900 */	/* page up */
	case SC_SHFT_UPARROW:	/* 0x4838 */	/* shift + up arrow */
	{

		msg[4] = WA_UPPAGE;
		if (list->cur)
			list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);

		slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);

		if (list->cur)
		{
			sl_set_selected( list, true, NORMREDRAW );
		}
		break;
	}
	case SC_SHFT_DNARROW:	/* 0x5032 */ 	/* shift + down arrow */
	case SC_PGDN:		/* 0x5100 */	/* page down */
	{
		msg[4] = WA_DNPAGE;
		if (list->cur)
			list->set(list, NULL, SESET_UNSELECTED, UNSELECT_ALL, NORMREDRAW);
		slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
		if (list->cur)
		{
			sl_set_selected( list, true, NORMREDRAW );
		}
		break;
	}

	case SC_SHFT_RTARROW:		/* page right */
		msg[4] = WA_RTPAGE;
		/* fall through */
	case SC_SHFT_LFARROW:		/* page left */
		if( msg[4] == 0 )
			msg[4] = WA_LFPAGE;
		slist_msg_handler(list->wi, NULL, AMQ_NORM, QMF_CHKDUP, msg);
	break;

	case SC_CLRHOME:	/* 0x4700 */	/* home */
		scroll_down(list, list->start_y, true);

		sl_set_selected( list, true, NORMREDRAW );

		break;

	case SC_SHFT_CLRHOME:	/* 0x4737 */	/* shift + home */
	{
		long n = list->total_h - (list->start_y + list->wi->wa.g_h);
		bool top = true;
		short redraw = NOREDRAW;

		if (n > list->start->r.g_h / 2){
			scroll_up(list, n, true);
		}
		else{
			SCROLL_ENTRY *e = get_last_entry(list, ENT_VISIBLE, -1, NULL);
			if( !e || e == list->cur )
				break;
			list->cur = e;
			top = false;
			redraw = NORMREDRAW;
		}
		sl_set_selected( list, top, redraw );
		break;
	}
	case SC_CTRL_C:
		if( list->cur->content /*&& list->wi*/ )
		{
		/*
		 * copy list-entry to clipboard if text and from AES
		 */
			char *text = list->cur->content->c.text.text;
			if( list->cur->content->type == SECONTENT_TEXT && text )
			{
				copy_string_to_clipbrd( text );
			}
		}
	break;
	default:
		return keycode;
	}
	return 0;
}
