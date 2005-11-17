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

//#include WIDGHNAME
#include "xa_xtobj.h"

#include "xa_types.h"
#include "xa_global.h"

#include "draw_obj.h"
#include "obtree.h"
#include "k_init.h"
#include "trnfm.h"
#include "rectlist.h"
#include "c_window.h"
#include "widgets.h"
#include "scrlobjc.h"
#include "xa_user_things.h"

#include "mint/signal.h"

#define done(x) (*wt->state_mask &= ~(x))

struct xa_texture
{
	struct xa_data_hdr h;
	MFDB mfdb;
	struct xa_wtexture t;
};

void *xobj_rshdr = NULL;
void *xobj_rsc = NULL;
static struct xa_texture *rootmenu_texture = NULL;


#if GENERATE_DIAGS

static char *pstates[] =
{
	"SEL",
	"CROSS",
	"\10",
	"DIS",
	"OUTL",
	"SHA",
	"WBAK",
	"D3D",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"15"
};

static char *pflags[] =
{
	"S",
	"DEF",
	"EXIT",
	"ED",
	"RBUT",
	"LAST",
	"TOUCH",
	"HID",
	">>",
	"INDCT",
	"BACKGR",
	"SUBM",
	"12",
	"13",
	"14",
	"15"
};

static char *ob_types[] =
{
	"box",
	"text",
	"boxtext",
	"image",
	"progdef",
	"ibox",
	"button",
	"boxchar",
	"string",
	"ftext",
	"fboxtext",
	"icon",
	"title",
	"cicon",
	"Magic swbutton",	/* 34 */
	"Magic popup",	/* 35 */
	"Magic wintitle",	/* 36 */
	"Magic edit",		/* 37 */
	"Magic shortcut",	/* 38 */
	"xaaes slist",	/* 39 */
	"40"
};

static char *
object_txt(OBJECT *tree, short t)			/* HR: I want to know the culprit in a glance */
{
    	static char nother[160];
	int ty = tree[t].ob_type;

	*nother = 0;

	switch (ty & 0xff)
	{
		case G_FTEXT:
		case G_TEXT:
		case G_BOXTEXT:
		case G_FBOXTEXT:
		{
			TEDINFO *ted = object_get_tedinfo(tree + t, NULL);
			sprintf(nother, sizeof(nother), " (%lx)'%s'", (long)ted->te_ptext, ted->te_ptext);
			break;
		}
		case G_BUTTON:
		case G_TITLE:
		case G_STRING:
		case G_SHORTCUT:
			sprintf(nother, sizeof(nother), " '%s'", object_get_spec(tree + t)->free_string);
			break;
	}	
	return nother;
}

static char *
object_type(OBJECT *tree, short t)
{
	static char other[80];

	unsigned int ty = tree[t].ob_type;
	unsigned int tx;

	if (ty >= G_BOX && ty < G_BOX+sizeof(ob_types)/sizeof(*ob_types))
		return ob_types[ty-G_BOX];

	tx = ty & 0xff;
	if (tx >= G_BOX && tx < G_BOX+sizeof(ob_types)/sizeof(*ob_types))
		sprintf(other, sizeof(other), "ext: 0x%x + %s", ty >> 8, ob_types[tx-G_BOX]);
	else
		sprintf(other, sizeof(other), "unknown: 0x%x,%d", ty, ty);

	return other;
}
#else
#if 0
static char *ob_types[] =
{
	"box",
	"text",
	"boxtext",
	"image",
	"progdef",
	"ibox",
	"button",
	"boxchar",
	"string",
	"ftext",
	"fboxtext",
	"icon",
	"title",
	"cicon",
	"Magic swbutton",	/* 34 */
	"Magic popup",	/* 35 */
	"Magic wintitle",	/* 36 */
	"Magic edit",		/* 37 */
	"Magic shortcut",	/* 38 */
	"xaaes slist",	/* 39 */
	"40"
};
#endif
#endif

bool inline d3_any(OBJECT *ob)        { return (ob->ob_flags & OF_FL3DACT) != 0;	}
bool inline d3_indicator(OBJECT *ob)  { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DIND; }
bool inline d3_foreground(OBJECT *ob) { return (ob->ob_flags & OF_FL3DIND) != 0; }
bool inline d3_background(OBJECT *ob) { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DBAK; }
bool inline d3_activator(OBJECT *ob)  { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DACT; }

static void
g2d_box(struct xa_vdi_settings *v, short b, RECT *r, short colour)
{
	/* inside runs from 3 to 0 */
	if (b > 0)
	{
		if (b >  4) b =  3;
		else        b--;
		(*v->api->l_color)(v, colour);
		while (b >= 0)
			(*v->api->gbox)(v, -b, r), b--;
	}
	/* outside runs from 4 to 1 */
	else if (b < 0)
	{
		if (b < -4) b = -4;
		(*v->api->l_color)(v, colour);
		while (b < 0)
			(*v->api->gbox)(v, -b, r), b++;
	}
}

void
draw_2d_box(struct xa_vdi_settings *v, short x, short y, short w, short h, short border_thick, short colour)
{
	RECT r;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	g2d_box(v, border_thick, &r, colour);
}

/* HR: pxy wrapper functions (Beware the (in)famous -1 bug */
/* Also vsf_perimeter only works in XOR writing mode */

void
write_menu_line(struct xa_vdi_settings *v, RECT *cl)
{
	short pnt[4];
	(*v->api->l_color)(v, G_BLACK);

	pnt[0] = cl->x;
	pnt[1] = cl->y + cl->h - 1;
	pnt[2] = cl->x + cl->w - 1;
	pnt[3] = pnt[1];
	v_pline(v->handle, 2, pnt);
}

void
r2pxy(short *p, short d, const RECT *r)
{
	*p++ = r->x - d;
	*p++ = r->y - d;
	*p++ = r->x + r->w + d - 1;
	*p   = r->y + r->h + d - 1;
}

void
rtopxy(short *p, const RECT *r)
{
	*p++ = r->x;
	*p++ = r->y;
	*p++ = r->x + r->w - 1; 
	*p   = r->y + r->h - 1;
}

void
ri2pxy(short *p, short d, short x, short y, short w, short h)
{
	*p++ = x - d;
	*p++ = y - d;
	*p++ = x + w + d - 1;
	*p   = y + h + d - 1;
} 
	
void
ritopxy(short *p, short x, short y, short w, short h)
{
	*p++ = x;
	*p++ = y;
	*p++ = x + w - 1; 
	*p   = y + h - 1;
}

#if NAES3D
#define PW (default_options.naes ? 1 : 0)
#else
#define PW 0
#endif

void
adjust_size(short d, RECT *r)
{
	r->x -= d;	/* positive value d means enlarge! :-)   as everywhere. */
	r->y -= d;
	r->w += d+d;
	r->h += d+d;
}

static void
chiseled_gbox(struct xa_vdi_settings *v, short d, const RECT *r)
{
	(*v->api->br_hook)(v, d,   r, objc_rend.dial_colours.lit_col);
	(*v->api->tl_hook)(v, d,   r, objc_rend.dial_colours.shadow_col);
	(*v->api->br_hook)(v, d-1, r, objc_rend.dial_colours.shadow_col);
	(*v->api->tl_hook)(v, d-1, r, objc_rend.dial_colours.lit_col);
}
	
static void
write_selection(struct xa_vdi_settings *v, short d, RECT *r)
{
	(*v->api->wr_mode)(v, MD_XOR);
	(*v->api->f_color)(v, G_BLACK);
	(*v->api->f_interior)(v, FIS_SOLID);
	(*v->api->gbar)(v, d, r);
	(*v->api->wr_mode)(v, MD_TRANS);
}

static void
d3_bottom_line(struct xa_vdi_settings *v, RECT *r, bool l3d, short fgc, short bgc)
{
	short x, y, w;
			 
	 if (l3d)
	 {
	 	x = r->x;
	 	y = r->y + r->h - 1;
	 	w = r->w - 1;
	 }
	 else
	 {
	 	x = r->x;
	 	y = r->y + r->h;
	 	w = r->w;
	 }
	(*v->api->line)(v, x, y, x + w, y, fgc);
	if (l3d)
	{
		y++;
		(*v->api->line)(v, x + 1, y, x + w + 1, y, bgc);
	}
}

static void
d3_pushbutton(struct xa_vdi_settings *v, short d, RECT *r, BFOBSPEC *col, short state, short thick, short mode)
{
	const unsigned short selected = state & OS_SELECTED;
	short t, j, outline;

	thick = -thick;		/* make thick same direction as d (positive value --> LARGER!) */

	if (thick > 0)		/* outside thickness */
		d += thick;
	d += 2;

	if (mode & 1)		/* fill ? */
	{
		if (col == NULL)
			(*v->api->f_color)(v, objc_rend.dial_colours.bg_col);
		/* otherwise set by set_colours() */
		/* inside bar */
		(*v->api->gbar)(v, d, r);
	}

	j = d;
	t = abs(thick);
	outline = j;

#if NAES3D
	if (default_options.naes && !(mode & 2))
	{	
		(*v->api->l_color)(v, objc_rend.dial_colours.fg_col);
		
		while (t > 0)
		{
			/* outside box */
			(*v->api->gbox)(v, j, r);
			t--, j--;
		}
	
		(*v->api->br_hook)(v, j, r, selected ? objc_rend.dial_colours.lit_col : objc_rend.dial_colours.shadow_col);
		(*v->api->tl_hook)(v, j, r, selected ? objc_rend.dial_colours.shadow_col : objc_rend.dial_colours.lit_col);
	}
	else
#endif
	{
		if (mode & 0x8000)
		{
			j--;
			do {
				(*v->api->br_hook)(v, j, r, selected ? objc_rend.dial_colours.lit_col : objc_rend.dial_colours.shadow_col);
				(*v->api->tl_hook)(v, j, r, selected ? objc_rend.dial_colours.shadow_col : objc_rend.dial_colours.lit_col);
				t--, j--;
			}
			while (t > 0);

			/* full outline ? */
			if (thick && !(mode & 2))
			{
				(*v->api->l_color)(v, objc_rend.dial_colours.fg_col);
				/* outside box */
			
				if (thick > 2)
					(*v->api->gbox)(v, outline - 1, r);
				(*v->api->rgbox)(v, outline, 1, r);
			}
		}
		else
		{
			do {
				(*v->api->br_hook)(v, j, r, selected ? objc_rend.dial_colours.lit_col : objc_rend.dial_colours.shadow_col);
				(*v->api->tl_hook)(v, j, r, selected ? objc_rend.dial_colours.shadow_col : objc_rend.dial_colours.lit_col);
				t--, j--;
			}
			while (t >= 0);

			/* full outline ? */
			if (thick && !(mode & 2))
			{
				(*v->api->l_color)(v, objc_rend.dial_colours.fg_col);
				/* outside box */
				(*v->api->gbox)(v, outline, r);
			}
		}
	}

	shadow_object(v, outline, state, r, objc_rend.dial_colours.border_col, thick);

	(*v->api->l_color)(v, objc_rend.dial_colours.border_col);
}
		
/* HR: 1 (good) set of routines for screen saving */
inline long calc_back(const RECT *r, short planes)
{
	return 2L * planes
		  * ((r->w + 15) >> 4) // / 16)
		  * r->h;
}

void form_save(short d, RECT r, void **area)
{
	MFDB Mscreen = { 0 };
	MFDB Mpreserve;
	short pnt[8];

	r.x -= d;
	r.y -= d;
	r.w += d * 2;
	r.h += d * 2;

	if (r.x < 0)
	{
		r.w += r.x;
		r.x = 0;
	}
	if (r.y < 0)
	{
		r.h += r.y;
		r.y = 0;
	}
	
	if (r.w > 0 && r.h > 0)
	{
		rtopxy(pnt, &r);
		ritopxy(pnt+4,0,0,r.w,r.h);

		DIAG((D_menu, NULL, "form_save %d/%d,%d/%d", r.x, r.y, r.w, r.h));

		Mpreserve.fd_w = r.w;
		Mpreserve.fd_h = r.h;
		Mpreserve.fd_wdwidth = (r.w + 15) / 16;
		Mpreserve.fd_nplanes = screen.planes;
		Mpreserve.fd_stand = 0;

		/* if something is allocated free it */
		if (*area)
			kfree(*area);

		*area = kmalloc(calc_back(&r,screen.planes));
		if (*area)
		{
			DIAG((D_menu, NULL, "form_save: to %lx", *area));
			Mpreserve.fd_addr = *area;
			hidem();
			vro_cpyfm(C.P_handle, S_ONLY, pnt, &Mscreen, &Mpreserve);
			showm();
		}
	}
}

void form_restore(short d, RECT r, void **area)
{
	if (*area)
	{
		MFDB Mscreen = { 0 };
		MFDB Mpreserve;
		short pnt[8];

		r.x -= d;
		r.y -= d;
		r.w += d * 2;
		r.h += d * 2;

		if (r.x < 0)
		{
			r.w += r.x;
			r.x = 0;
		}
		if (r.y < 0)
		{
			r.h += r.y;
			r.y = 0;
		}

		if (r.w > 0 && r.h > 0)
		{
			rtopxy(pnt+4, &r);
			ritopxy(pnt,0,0,r.w,r.h);

			DIAG((D_menu, NULL, "form_restore %d/%d,%d/%d from %lx", r.x, r.y, r.w, r.h, *area));

			Mpreserve.fd_w = r.w;
			Mpreserve.fd_h = r.h;
			Mpreserve.fd_wdwidth = (r.w + 15) / 16;
			Mpreserve.fd_nplanes = screen.planes;
			Mpreserve.fd_stand = 0;
			Mpreserve.fd_addr = *area;
			hidem();
			vro_cpyfm(C.P_handle, S_ONLY, pnt, &Mpreserve, &Mscreen);
			showm();

			kfree(*area);
			*area = NULL;
		}
	}
}

void
form_copy(const RECT *fr, const RECT *to)
{
	MFDB Mscreen = { 0 };
	short pnt[8];
	rtopxy(pnt,fr);
	rtopxy(pnt+4,to);
	vro_cpyfm(C.P_handle, S_ONLY, pnt, &Mscreen, &Mscreen);
}

void
shadow_object(struct xa_vdi_settings *v, short d, short state, RECT *rp, short colour, short thick)
{
	RECT r = *rp;
	short offset, increase;

	/* Are we shadowing this object? (Borderless objects aren't shadowed!) */
	if (thick && (state & OS_SHADOWED))
	{
		short i;

		if (thick < -4) thick = -4;
		else
		if (thick >  4) thick =  4;

		offset = thick > 0 ? thick : 0;
		increase = -thick;

		r.x += offset;
		r.y += offset;
		r.w += increase;
		r.h += increase;

		for (i = 0; i < abs(thick)*2; i++)
		{
			r.w++, r.h++;
			(*v->api->br_hook)(v, d, &r, colour);
		}
	}
}

void
shadow_area(struct xa_vdi_settings *v, short d, short state, RECT *rp, short colour, short x_thick, short y_thick)
{
	RECT r;
	short offset, inc;

	/* Are we shadowing this object? (Borderless objects aren't shadowed!) */
	if ((x_thick | y_thick) && (state & OS_SHADOWED))
	{
		short i;

		if (x_thick)
		{
			r = *rp;
			offset	= x_thick > 0 ? x_thick : 0;
			inc	= -x_thick;

			r.y += offset;
			r.h -= offset;
			r.w += inc;

			for (i = x_thick < 0 ? -x_thick : x_thick; i > 0; i--)
			{
				r.w++;
				(*v->api->right_line)(v, d, &r, colour);
			}
		}
		if (y_thick)
		{
			r = *rp;
			offset	= y_thick > 0 ? y_thick : 0;
			inc	= -y_thick;

			r.x += offset;
			r.w -= offset;
			r.h += inc;

			for (i = y_thick < 0 ? -y_thick : y_thick; i > 0; i--) //(i = 0; i < abs(y_thick); i++)
			{
				r.h++;
				(*v->api->bottom_line)(v, d, &r, colour);
			}
		}
	}
}
#if 0
static short
menu_dis_col(XA_TREE *wt)		/* Get colours for disabled better. */
{
	short c = G_BLACK;

	if (!MONO)
	{
		if (wt->is_menu)
		{
			OBJECT *ob = wt->tree + wt->current;
			if (ob->ob_state & OS_DISABLED)
			{
				c = objc_rend.dial_colours.shadow_col;
				done(OS_DISABLED);
			}
		}
	}

	return c;
}
#endif
static BFOBSPEC
button_colours(void)
{
	BFOBSPEC c;

	c.character = 0;
	c.framesize = 0;

	c.framecol = objc_rend.dial_colours.border_col;
	c.textcol  = G_BLACK;
	c.textmode = 1;
	c.fillpattern = IP_HOLLOW;
	c.interiorcol = objc_rend.dial_colours.bg_col;

	return c;
}

static void
ob_text(XA_TREE *wt,
	struct xa_vdi_settings *v,
	struct objc_edit_info *ei,
	RECT *r, RECT *o,
	BFOBSPEC *c,
	char *t,
	short state,
	short flags,
	short und, short undcol)
{
	if (t && *t)
	{
		OBJECT *ob = wt->tree + wt->current;
		bool fits = true; // !o || ((o->h >= r->h - (d3_foreground(ob) ? 4 : 0)));

		/* set according to circumstances. */
		if (c)
		{
			/* more restrictions	*/
			if (    c->textmode
			    && !MONO
			    && d3_any(ob)
			    && (     c->fillpattern == IP_HOLLOW
			         || (c->fillpattern == IP_SOLID && c->interiorcol == G_WHITE)))
			{
				(*v->api->f_color)(v, objc_rend.dial_colours.bg_col);
				(*v->api->wr_mode)(v, MD_REPLACE);
				(*v->api->gbar)(v, 0, o ? o : r);
				(*v->api->wr_mode)(v, MD_TRANS);
			}
			else
				(*v->api->wr_mode)(v, c->textmode ? MD_REPLACE : MD_TRANS);
		}

		if (!MONO && (state & OS_DISABLED))
		{
			done(OS_DISABLED);
			if (fits)
			{
				(*v->api->t_color)(v, objc_rend.dial_colours.lit_col);
				v_gtext(v->handle, r->x + 1, r->y + 1 - v->dists[5], t);
				(*v->api->t_color)(v, objc_rend.dial_colours.shadow_col);
			}
		}
		
		if (ei && !(state & OS_DISABLED) && ei->obj == wt->current && ei->m_end > ei->m_start)
		{
			int sl = strlen(t) + 1;
			short tc, x = r->x, y = r->y - v->dists[5], w, h;
			short start, end;
			char s[256];
			RECT br = o ? *o : *r;
			
			start = ei->m_start + ei->edstart;
			end = ei->m_end + ei->edstart;
			tc = v->text_color;
			if (start)
			{
				strncpy(s, t, start);
				s[start] = '\0';
				v_gtext(v->handle, x, y, s);
				(*v->api->t_extent)(v, s, &w, &h);
				x += w;
			}
			strncpy(s, t + start, end - start);
			s[end - start] = '\0';
			(*v->api->t_extent)(v, s, &w, &h);
			br.x = x, br.w = w;
			(*v->api->f_color)(v, G_BLUE);
			(*v->api->wr_mode)(v, MD_REPLACE);
			(*v->api->gbar)(v, 0, &br);
			(*v->api->wr_mode)(v, MD_TRANS);
			
			(*v->api->t_color)(v, G_WHITE);

			v_gtext(v->handle, x, y, s);
			(*v->api->t_color)(v, tc);
			if (sl > end)
			{
// 				(*v->api->t_extent)(v, s, &w, &h);
				x += w;
				strncpy(s, t + end, sl - end);
				s[sl - end] = '\0';
				v_gtext(v->handle, x, y, s);
			}
		}
		else
		{
			if (fits)
			{
				if (!MONO && (flags & OF_FL3DIND) && !(state & OS_DISABLED))
				{
					short tc = v->text_color;
					(*v->api->t_color)(v, objc_rend.dial_colours.lit_col);
					v_gtext(v->handle, r->x + 1, r->y + 1 - v->dists[5], t);
					(*v->api->t_color)(v, tc);
				}
				v_gtext(v->handle, r->x, r->y - v->dists[5], t);
			}
		}
		/* Now underline the shortcut character, if any. */
		/* Ozk: Prepared for proportional fonts! */
		if (und >= 0)
		{
			short l = strlen(t);
			if (und < l)
			{
				char sc;
				short x, y, w, h;
				
				y = r->y;
				
				if (!und)
					x = 0;
				else
				{
					sc = t[und];
					t[und] = '\0';
					(*v->api->t_extent)(v, t, &w, &h);
					x = w;
					t[und] = sc;
				}
				sc = t[und + 1];
				t[und + 1] = '\0';
				(*v->api->t_extent)(v, t, &w, &h);
				t[und + 1] = sc;
				w -= x;
				y += (h - v->dists[0] + 1);
				x += r->x;
				(*v->api->line)(v, x, y, x + w, y, undcol);
			}
		}
		else if (und == -2)
		{
			short w, h;
			RECT nr = *r;
			(*v->api->t_extent)(v, t, &w, &h);
			nr.w = w;
			d3_bottom_line(v, &nr, (flags & OF_FL3DBAK), G_BLACK, G_WHITE);
		}
	}
}
#if 0
static void
g_text(XA_TREE *wt, struct xa_vdi_settings *v, RECT r, RECT *o, const char *text, short state)
{
	char t[256];
	/* only center the text. ;-) */

	strcpy(t, text);

	r.y += (r.h - screen.c_max_h) / 2;
	if (!MONO && (state & OS_DISABLED))
	{
		(*v->api->t_color)(v, objc_rend.dial_colours.lit_col);
		v_gtext(v->handle, r.x + 1, r.y + 1, text);
		(*v->api->t_color)(v, objc_rend.dial_colours.shadow_col);
		v_gtext(v->handle, r.x,   r.y,   t);
		done(OS_DISABLED);
	}
	else
	{
		(*v->api->t_color)(v, menu_dis_col(wt));
		ob_text(wt, v, NULL, &r, o, NULL, t, 0, 0, (state & OS_WHITEBAK) ? (state >> 8) & 0x7f : -1, G_RED);
		if (state & OS_DISABLED)
		{
			(*v->api->write_disable)(v, &wt->r, objc_rend.dial_colours.bg_col);
			done(OS_DISABLED);
		}
	}
}
#endif

/* This function doesnt change colourword anymore, but just sets the required color.
 * Neither does it affect writing mode for text (this is handled in ob_text() */
static void
set_colours(OBJECT *ob, struct xa_vdi_settings *v, BFOBSPEC *colourword)
{
	(*v->api->wr_mode)(v, MD_REPLACE);

	/* 2 */
	(*v->api->f_interior)(v, FIS_PATTERN);

	if (colourword->fillpattern == IP_SOLID)
	{
		/* 2,8  solid fill  colour */
		(*v->api->f_style)(v, 8);
		(*v->api->f_color)(v, colourword->interiorcol);
	}
	else
	{
		if (colourword->fillpattern == IP_HOLLOW)	
		{
			short c;

			/* 2,8 solid fill  white */
			(v->api->f_style)(v, 8);

			/* Object inherits default dialog background colour? */
			if ((colourword->interiorcol == 0) && d3_any(ob))
				c = objc_rend.dial_colours.bg_col;
			else
				c = G_WHITE;
			
			(*v->api->f_color)(v, c);

		}
		else
		{
			(*v->api->f_style)(v, colourword->fillpattern);
			(*v->api->f_color)(v, colourword->interiorcol);
		}
	}

#if SELECT_COLOR
	if (!MONO && (ob->ob_state & OS_SELECTED))
	{
		/* Allow a different colour set for 3d push  */
		if (d3_any(ob))
			(*v->api->f_color)(v, selected3D_colour[colourword->interiorcol]);
		else
			(*v->api->f_color)(v, selected_colour[colourword->interiorcol]);
	}
#endif

	(*v->api->t_color)(v, colourword->textcol);
	(*v->api->l_color)(v, colourword->framecol);
}

/*
 * Format a G_FTEXT type text string from its template,
 * and return the real position of the text cursor.
 * 
 * HR: WRONG_LEN
 * It is a very confusing here, edit_pos is always (=1) passed as tedinfo->te_tmplen.
 * which means that this only works if that field is wrongly used as the text corsor position.
 * A very bad thing, because it is supposed to be a constant describing the amount of memory
 * allocated to the template string.
 * 
 * 28 jan 2001
 * OK, edit_pos is not anymore the te_tmplen field.
 */
static short
format_dialog_text(char *text_out, const char *template, const char *text_in, short edit_pos, short *ret_startpos)
{
	short index = 0, start_tpos = -1, tpos = 0, max = strlen(template);
	/* HR: In case a template ends with '_' and the text is completely
	 * filled, edit_index was indeterminate. :-)
	 */
	short edit_index = max;
// 	char *to = text_out;
	bool aap = *text_in == '@';

	DIAG((D_o, NULL, "format_dialog_text edit_pos %d", edit_pos));
// 	display("t  in '%s'", text_in);
// 	display("tmplt '%s'", template);
	while (*template)
	{
		if (*template != '_')
		{
			*text_out++ = *template;
		}
		else
		{
			if (start_tpos == -1)
				start_tpos = index;

			/* Found text field */

			if (tpos == edit_pos)
				edit_index = index;
				
			if (*text_in)
			{
				if (aap)
					*text_out++ = '_';
				else
					*text_out++ = *text_in++;
			}
			else
				*text_out++ = cfg.ted_filler;

			tpos++;		
		}
		template++;
		index++;
	}

	*text_out = '\0';
	
	if (edit_index > (start_tpos + tpos))
		edit_index = start_tpos + tpos;

	if (ret_startpos)
		*ret_startpos = start_tpos;

	/* keep visible at end */
	if (edit_index > max)
		edit_index--;

// 	display("t out '%s'", to);
	return edit_index;
}

/* implement wt->x,y
 * Modifications: set_text(...  &gr, &cr, temp_text ...)
 * cursor alignment in centered text.
 * Justification now integrated in set_text (v_gtext isolated).
 * Preparations for positioning cursor in proportional fonts.
 * Moved a few calls.
 * pbox(hl,ext[0]+x,ext[1]+y,ext[4]+x-1,ext[5]+y-1);
 */
static void
set_text(OBJECT *ob,
	 struct xa_vdi_settings *v,
	 RECT *gr,
	 RECT *cr,
	 bool formatted,
	 short edit_pos,
	 char *temp_text,
	 BFOBSPEC *colours,
	 short *thick,
	 short *ret_edstart,
	 struct objc_edit_info **ret_ei,
	 RECT r)
{
	TEDINFO *ted;
	XTEDINFO *xted = NULL;
	RECT cur;
	short w, h, cur_x = 0, start_tpos = 0;

	(unsigned long)ted = object_get_spec(ob)->index;
	
	if ((long)ted->te_ptext == 0xffffffffL)
	{
// 		ndisplay("ted %lx, just %d", ted, ted->te_just);
		(long)xted = (long)ted->te_ptmplt;
		ted = &xted->ti;
// 		display("ted %lx, just %d", ted, ted->te_just);
// 		display("xted %lx, te_ptext %lx, text '%s'", xted, ted->te_ptext, ted->te_ptext);
		if (ret_ei)
			*ret_ei = xted;
	}
	else if (ret_ei)
		*ret_ei = NULL;

	*thick = (char)ted->te_thickness;

	*colours = *(BFOBSPEC*)&ted->te_just;
        // FIXME: gemlib problem: hacked a bit need only "ted->te_color" word;
	//	  -> cleaning the information that would not be taken if
	//	     properly used:
	colours->character = 0;
	colours->framesize = 0;

	/* Set the correct text size & font */
	switch (ted->te_font)
	{
	case TE_GDOS_PROP:		/* Use a proportional SPEEDOGDOS font (AES4.1 style) */
	case TE_GDOS_MONO:		/* Use a monospaced SPEEDOGDOS font (AES4.1 style) */
	case TE_GDOS_BITM:		/* Use a GDOS bitmap font (AES4.1 style) */
	{
		(*v->api->t_font)(v, ted->te_fontsize, ted->te_fontid);
		cur.w = screen.c_max_w;
		cur.h = screen.c_max_h;
		break;
	}
	case TE_STANDARD:		/* Use the standard system font (probably 10 point) */
	{
		(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
		cur.w = screen.c_max_w;
		cur.h = screen.c_max_h;
		break;
	}
	case TE_SMALL:			/* Use the small system font (probably 8 point) */
	{
		(*v->api->t_font)(v, screen.small_font_point, screen.small_font_id);
		cur.w = screen.c_min_w;
		cur.h = screen.c_min_h;
		break;
	}
	}

	/* HR: use vqt_extent to obtain x, because it is almost impossible, or
	 *	at least very unnecessary tedious to position the cursor.
	 *	vst_alignment is not needed anymore.
	 */
	/* after vst_font & vst_point */

	if (formatted)
	{
		cur_x = format_dialog_text(temp_text, ted->te_ptmplt, ted->te_ptext, edit_pos, &start_tpos);
	}
	else
		strncpy(temp_text, ted->te_ptext, 255);

	(*v->api->t_extent)(v, temp_text, &w, &h);

	/* HR 290301 & 070202: Dont let a text violate its object space! (Twoinone packer shell!! :-) */
	if (w > r.w)
	{
		short	rw  = r.w / cur.w,
			dif = (w - r.w + cur.w - 1) / cur.w,
			h1dif, h2dif;

		switch(ted->te_just)
		{
			case TE_LEFT:
				*(temp_text + rw) = '\0';
				break;
			case TE_RIGHT:
				strcpy (temp_text, temp_text + dif);
				break;
			case TE_CNTR:
				h1dif = dif/2;
				h2dif = (dif+1)/2;
				*(temp_text + strlen(temp_text) - h2dif) = 0;
				strcpy (temp_text, temp_text + h1dif);
				break;
		}
		(*v->api->t_extent)(v, temp_text, &w, &h);
	}

	switch (ted->te_just)		/* Set text alignment - why on earth did */
	{
						/* Atari use a different horizontal alignment */
		case TE_LEFT:			/* code for GEM to the one the VDI uses? */
			cur.x = r.x;
			break;
		case TE_RIGHT:
			cur.x = r.x + r.w - w;
			break;
		case TE_CNTR:
			cur.x = r.x + (r.w - w) / 2;
			break;
	}

	cur.y = r.y + (r.h - h) / 2;

	if (cr)
	{
		short tw, th;
		char sc;
		 
		sc = temp_text[cur_x];
		temp_text[cur_x] = '\0';
		(*v->api->t_extent)(v, temp_text, &tw, &th);
		temp_text[cur_x] = sc;
		
		*cr = cur;		
		cr->x += tw; //cur_x * cur.w;	/* non prop font only */
		cr->w = 1;
	}

	cur.w = w;
	cur.h = h;
	*gr = cur;

// 	display(" -- '%s'", temp_text);
	if (ret_edstart)
		*ret_edstart = start_tpos;
}

static void
rl_xor(struct xa_vdi_settings *v, RECT *r, struct xa_rect_list *rl)
{
	if (rl)
	{
		short c[4];
		while (rl)
		{
			rtopxy(c, &rl->r);
			vs_clip(v->handle, 1, c);
			write_selection(v, 0, r);
			rl = rl->next;
		}
		rtopxy(c, &v->clip);
		vs_clip(v->handle, 1, c);
	}
	else
		write_selection(v, 0, r);
}
void
enable_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v)
{
	struct objc_edit_info *ei = wt->ei;

	if (!ei)
		return; //ei = &wt->e;

	if (ei->obj > 0)
	{
		set_objcursor(wt, v, ei);
		ei->c_state |= OB_CURS_ENABLED;
	}
}

void
disable_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl)
{
	struct objc_edit_info *ei = wt->ei;
	if (!ei)
		return; //ei = &wt->e;
	undraw_objcursor(wt, v, rl);
	ei->c_state &= ~OB_CURS_ENABLED;
}

void
eor_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl)
{
	struct objc_edit_info *ei = wt->ei;
	RECT r;

	if (!ei)
		ei = &wt->e;

	if (ei->obj != -1)
	{
		set_objcursor(wt, v, ei);
		r = ei->cr;
		r.x += wt->tree->ob_x;
		r.y += wt->tree->ob_y;

		if (!(wt->tree[ei->obj].ob_flags & OF_HIDETREE))
			rl_xor(v, &r, rl);
	}
}
	
void
draw_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl)
{
	struct objc_edit_info *ei = wt->ei;
	
	if (!ei)
		return; //ei = &wt->e;

	if ( (ei->c_state & (OB_CURS_ENABLED | OB_CURS_DRAWN)) == OB_CURS_ENABLED )
	{
		RECT r = ei->cr;

		r.x += wt->tree->ob_x;
		r.y += wt->tree->ob_y;
		
		if (ei->obj >= 0 && !(wt->tree[ei->obj].ob_flags & OF_HIDETREE))
		{
			rl_xor(v, &r, rl);
			ei->c_state |= OB_CURS_DRAWN;
		}
	}
}
void
undraw_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl)
{
	struct objc_edit_info *ei = wt->ei;

	if (!ei)
		return; //ei = &wt->e;

	if ( (ei->c_state & (OB_CURS_ENABLED | OB_CURS_DRAWN)) == (OB_CURS_ENABLED | OB_CURS_DRAWN) )
	{
		RECT r = ei->cr;

		r.x += wt->tree->ob_x;
		r.y += wt->tree->ob_y;
		
		if (ei->obj >= 0 && !(wt->tree[ei->obj].ob_flags & OF_HIDETREE))
		{
			rl_xor(v, &r, rl); //write_selection(0, &r);
			ei->c_state &= ~OB_CURS_DRAWN;
		}
	}
}
/* 780e000 */
void
set_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct objc_edit_info *ei)
{
	char temp_text[256];
	RECT r;
	OBJECT *ob;
	RECT gr;
	BFOBSPEC colours;
	short thick;

	if (!ei)
		ei = &wt->e;

	if (ei->obj <= 0)
		return;

	obj_offset(wt, ei->obj, &r.x, &r.y);
	ob = wt->tree + ei->obj;
	r.w  = ob->ob_width;
	r.h  = ob->ob_height;
	
	set_text(ob, v, &gr, &ei->cr, true, ei->pos, temp_text, &colours, &thick, &ei->edstart, NULL, r);

	ei->cr.x -= wt->tree->ob_x;
	ei->cr.y -= wt->tree->ob_y;

	//t_font(screen.standard_font_point, screen.standard_font_id);
}

#if SELECT_COLOR
                                      /*  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
static const short selected_colour[]   = {1, 0,13,15,14,10,12,11, 8, 9, 5, 7, 6, 2, 4, 3};
static const short selected3D_colour[] = {1, 0,13,15,14,10,12,11, 9, 8, 5, 7, 6, 2, 4, 3};
#endif

/* HR: implement wt->x,y in all ObjectDisplay functions */
/* HR 290101: OBSPEC union & BFOBSPEC structure now fully implemented
 *            in xa_aes.h.
 *            Accordingly replaced ALL nasty casting & assemblyish approaches
 *            by simple straightforward C programming.
 */

/*
 * Draw a box (respecting 3d flags)
 */
static void
draw_g_box(struct widget_tree *wt, struct xa_vdi_settings *v)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	short thick;

	colours = object_get_spec(ob)->obspec;
	thick = object_thickness(ob);
	set_colours(ob, v, &colours);

	/* before borders */
	done(OS_SELECTED);

	/* plain box is a tiny little bit special. :-) */
	if (d3_foreground(ob)
	    && !(wt->current == 0 && thick < 0))	/* root box with inside border */
	{
		d3_pushbutton( v, 0,
		               &r,
		               &colours,
		               ob->ob_state,
		               thick,
		               1);
	}
	else
	{
// 		if (wt->rend_flags & WTR_ROOTMENU && rootmenu_texture)
// 		{
// 			RECT tr = r;
// 			(*v->api->wr_mode)(v, MD_REPLACE);
// 			(*v->api->draw_texture)(v, rootmenu_texture->t.body, &tr, &tr);
// 		}
// 		else
// 		{
			/* display inside */
			(*v->api->gbar)(v, 0, &r);
// 		}

		if (ob->ob_state & OS_SELECTED)
			write_selection(v, 0, &r);

		/* Display a border? */
		if (thick)
		{
			if (!(wt->current == 0 && wt->zen))
			{
				g2d_box(v, thick, &r, colours.framecol);
				shadow_object(v, 0, ob->ob_state, &r, colours.framecol, thick);
			}
		}
	}
}

void
d_g_box(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	//RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	//BFOBSPEC colours;
	//short thick;

	if ((ob->ob_type & 0xff) == G_EXTBOX)
	{
		struct extbox_parms *p = (struct extbox_parms *)object_get_spec(ob)->index;
		short ty = ob->ob_type;
		
		ob->ob_type = p->type;
		object_set_spec(ob, (unsigned long)p->obspec);
		draw_g_box(wt, v);
		object_set_spec(ob, (unsigned long)p);
		ob->ob_type = ty;

		p->wt		= wt;
		p->index	= wt->current;
		p->r		= wt->r;
		if (xa_rect_clip(&wt->r, &v->clip, &p->clip))
		{
			p->clip.w = p->clip.x + p->clip.w - 1;
			p->clip.h = p->clip.y + p->clip.h - 1;
				
			(*p->callout)(p);
		}
// 		(*v->api->set_clip)(v, &v->clip);
	}
	else
	{
		draw_g_box(wt, v);
	}
}

/*
 * Draw a plain hollow ibox
 */
void
d_g_ibox(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	short thick;

	colours = object_get_spec(ob)->obspec;
	thick = object_thickness(ob);
	set_colours(ob, v, &colours);

	/* before borders */
	done(OS_SELECTED|OS_DISABLED);

#if NAES3D
	if (default_options.naes && thick < 0)
		(*v->api->gbox)(v, d3_foreground(ob) ? 2 : 0, &r);
#endif

	/* plain box is a tiny little bit special. :-) */
	if (d3_foreground(ob)
	    && !(wt->current == 0 && thick < 0)) /* root box with inside border */
	{
		d3_pushbutton( v, 0,
		               &r,
		               &colours,
		               ob->ob_state,
		               thick,
		               0);
	}
	else
	{
		if (ob->ob_state & OS_SELECTED)
			write_selection(v, 0, &r);

		/* Display a border? */
		if (thick)
		{
			if (!(wt->current == 0 && wt->zen))
			{
				g2d_box(v, thick, &r, colours.framecol);
				shadow_object(v, 0, ob->ob_state, &r, colours.framecol, thick);
			}
		}
	}
}

/*
 * Display a boxchar (respecting 3d flags)
 */
void
d_g_boxchar(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	RECT r = wt->r, gr = r;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	ushort selected = ob->ob_state & OS_SELECTED;
	short thick;
	char temp_text[2];

	colours = object_get_spec(ob)->obspec;
	
	temp_text[0] = object_get_spec(ob)->obspec.character;
	temp_text[1] = '\0';

	thick = object_thickness(ob);

	/* leaves MD_REPLACE */
	set_colours(ob, v, &colours);

	(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
	(*v->api->t_effects)(v, 0);
	/* Centre the text in the box */
	(*v->api->t_extent)(v, temp_text, &gr.w, &gr.h);
	gr.x = r.x + ((r.w - gr.w) / 2);
	gr.y = r.y + ((r.h - gr.h) / 2);
// 	gr.w = screen.c_max_w;
// 	gr.h = screen.c_max_h;

	if (d3_foreground(ob))
	{
		d3_pushbutton(v, 0, &r, &colours, ob->ob_state, thick, 1);
		if (ob->ob_state & OS_SELECTED)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}
		(*v->api->wr_mode)(v, colours.textmode ? MD_REPLACE : MD_TRANS);
// 		(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
// 		(*v->api->t_effects)(v, 0);
		ob_text(wt, v, NULL, &gr, &r, NULL, temp_text, 0, 0, -1, G_BLACK);
	}
	else
	{
		(*v->api->gbar)(v, 0, &r);
// 		(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
// 		(*v->api->t_effects)(v, 0);
		ob_text(wt, v, NULL, &gr, &r, &colours, temp_text, ob->ob_state, 0, -1, G_BLACK);

		if (selected)
			write_selection(v, 0, &r);

		/* Display a border? */
		if (thick)
		{
			g2d_box(v, thick, &r, colours.framecol);
			shadow_object(v, 0, ob->ob_state, &r, colours.framecol, thick);
		}
	}

	done(OS_SELECTED);
}

/*
 * Draw a boxtext object
 */
void
d_g_boxtext(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	short thick = 0;
	ushort selected;
	RECT r = wt->r, gr;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	struct objc_edit_info *ei;
	char temp_text[256];

	selected = ob->ob_state & OS_SELECTED;

	(*v->api->t_effects)(v, 0);
	set_text(ob, v, &gr, NULL, false, -1, temp_text, &colours, &thick, NULL, &ei, r);
	set_colours(ob, v, &colours);

	if (d3_foreground(ob))		/* indicator or avtivator */
	{
		d3_pushbutton(v, 0, &r, &colours, ob->ob_state, thick, 1);

		if (selected)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}
		ob_text(wt, v, ei, &gr, &r, &colours, temp_text, ob->ob_state, 0, -1, G_BLACK);
	}
	else
	{
		(*v->api->gbar)(v, 0, &r);
		
		ob_text(wt, v, ei, &gr, &r, &colours, temp_text, ob->ob_state, 0, -1, G_BLACK);

		if (selected)
			write_selection(v, 0, &r);		/* before border */

		if (thick)	/* Display a border? */
		{
			(*v->api->wr_mode)(v, MD_REPLACE);
			g2d_box(v, thick, &r, colours.framecol);
			shadow_object(v, 0, ob->ob_state, &r, colours.framecol, thick);
		}
	}

	//t_font(screen.standard_font_point, screen.standard_font_id);
	done(OS_SELECTED);
}
void
d_g_fboxtext(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	char temp_text[256];
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	RECT gr,cr;
	BFOBSPEC colours;
	struct objc_edit_info *ei;
	const unsigned short selected = ob->ob_state & OS_SELECTED;
	short thick;

	ei = wt->ei ? wt->ei : &wt->e;
	if (ei->obj != wt->current)
		ei = NULL;

	(*v->api->t_effects)(v, 0);
	set_text(ob, v, &gr, &cr, true, ei ? ei->pos : -1, temp_text, &colours, &thick, NULL, &ei, r);
	set_colours(ob, v, &colours);

	if (d3_foreground(ob))
	{
		d3_pushbutton(v, 0, &r, &colours, ob->ob_state, thick, 1);
		if (selected)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}
		ob_text(wt, v, ei, &gr, &r, &colours, temp_text, ob->ob_state, 0, -1, G_BLACK);
	}
	else
	{
		(*v->api->gbar)(v, 0, &r);
		ob_text(wt, v, ei, &gr, &r, &colours, temp_text, ob->ob_state, 0, -1, G_BLACK);

		if (selected)
			/* before border */
			write_selection(v, 0, &r);

		/* Display a border? */
		if (thick)
		{
			(*v->api->wr_mode)(v, MD_REPLACE);
			g2d_box(v, thick, &r, colours.framecol);
			shadow_object(v, 0, ob->ob_state, &r, colours.framecol, thick);
		}
	}

	//t_font(screen.standard_font_point, screen.standard_font_id);
	done(OS_SELECTED);
}

/*
 * Draw a button object
 */
void
d_g_button(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	RECT r = wt->r, gr = r;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	short thick = object_thickness(ob); 
	ushort selected = ob->ob_state & OS_SELECTED;
	char *text = NULL; // = object_get_spec(ob)->free_string;
	
	if (ob->ob_type == G_POPUP)
	{
		POPINFO *pi = object_get_popinfo(ob);
		if (pi->obnum > 0)
			text = object_get_spec(pi->tree + pi->obnum)->free_string + 2;
	}
	else
		text = object_get_spec(ob)->free_string;

	colours = button_colours();
// 	(*v->api->t_color)(v, G_BLACK);

	if ((ob->ob_state & OS_WHITEBAK) && (ob->ob_state & 0x8000))
	{
		short und = (short)ob->ob_state >> 8;
		(*v->api->wr_mode)(v, MD_REPLACE);
		/* group frame */
		if (und == -2)
		{
			RECT rr = r;
			if (text && *text)
			{
				(*v->api->t_color)(v, G_BLACK);
				(*v->api->t_effects)(v, 0);
				if (ob->ob_state & OS_SHADOWED)
					(*v->api->t_font)(v, screen.small_font_point, screen.small_font_id);
				else
					(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
				
				(*v->api->t_extent)(v, text, &gr.w, &gr.h);
				rr.y += gr.h / 2;
				rr.h -= gr.h / 2;
			}
			if (MONO || !(ob->ob_flags & OF_FL3DBAK))
			{
				(*v->api->l_color)(v, MONO ? G_BLACK : G_LBLACK);
				(*v->api->gbox)(v, 0, &rr);
			}
			else
				chiseled_gbox(v, 0, &rr);

			if (text && *text)
			{
				gr.x = r.x + screen.c_max_w;
				gr.y = r.y;
				
				(*v->api->f_color)(v, objc_rend.dial_colours.bg_col);
				(*v->api->wr_mode)(v, MD_REPLACE);
				(*v->api->gbar)(v, 0, &gr);
				(*v->api->wr_mode)(v, MD_TRANS);
				ob_text(wt, v, NULL, &gr, NULL, NULL, text, ob->ob_state, ob->ob_flags, -1, G_BLACK);
			}
		}
		else
		{
			short xobj, w, h;
			XA_TREE b;

			b.owner = wt->owner;
			b.tree = xobj_rsc;
			xobj = (ob->ob_flags & OF_RBUTTON) ? (selected ? XOBJ_R_SEL : XOBJ_R_DSEL) : (selected ? XOBJ_B_SEL : XOBJ_B_DSEL);
			object_spec_wh((OBJECT *)xobj_rsc + xobj, &w, &h);
			if (gr.h > h)
				gr.y += ((gr.h - h) >> 1);
			display_object(	lock, &b, v, xobj, gr.x, gr.y, 11);
			if (text)
			{
				short undcol;
				(*v->api->t_color)(v, G_BLACK);
				r.x += (w + 2);
				(*v->api->wr_mode)(v, MD_TRANS);
				(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
				(*v->api->t_effects)(v, 0);
				
				switch ((ob->ob_flags & FL3DMASK) >> 9)
				{
					case 1:	 undcol = G_GREEN; break;
					case 2:  undcol = G_BLUE; break;
					case 3:  undcol = G_RED;   break;
					default: undcol = G_BLACK; break;
				}
				ob_text(wt, v, NULL, &r, &wt->r, NULL, text, ob->ob_state, 0, und & 0x7f, undcol);
			}
		}
	}
	else
	{
		short und, tw, th;

		und = (ob->ob_state & OS_WHITEBAK) ? (ob->ob_state >> 8) & 0x7f : -1;

		/* Use extent, NOT vst_alinment. Makes x and y to v_gtext
		 * real values that can be used for other stuff (like shortcut
		 * underlines :-)
		 */
		if (text)
		{
			(*v->api->t_color)(v, G_BLACK);
			(*v->api->t_effects)(v, 0);
			(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
			(*v->api->t_extent)(v, text, &tw, &th);
			gr.y += (r.h - th) / 2;
			gr.x += (r.w - tw) / 2;
		}

		if (d3_foreground(ob))
		{
			d3_pushbutton(v, 0, &r, NULL, ob->ob_state, thick, 0x8001);
			if (selected)
			{
				gr.x += PUSH3D_DISTANCE;
				gr.y += PUSH3D_DISTANCE;
			}
			if (text)
			{
				(*v->api->wr_mode)(v, MD_TRANS);
				ob_text(wt, v, NULL, &gr, &r, NULL, text, ob->ob_state, 0, und, G_BLACK);
			}
		}
		else
		{
			(*v->api->wr_mode)(v, MD_REPLACE);
			(*v->api->f_interior)(v, FIS_SOLID);
			(*v->api->f_color)(v, selected ? G_BLACK : G_WHITE);

			/* display inside bar */		
			(*v->api->gbar)(v, -thick, &r);

			(*v->api->wr_mode)(v, MD_TRANS);
			(*v->api->t_color)(v, selected ? G_WHITE : G_BLACK);
			if (text)
			{
				ob_text(wt, v, NULL, &gr, &r, NULL, text, ob->ob_state, 0, und, G_BLACK);
			}
			/* Display a border? */
			if (thick)
			{
				g2d_box(v, thick, &r, G_BLACK);
				shadow_object(v, 0, ob->ob_state, &r, colours.framecol, thick);
			}
		}
	}
	done(OS_SELECTED);
}

static void
icon_characters(struct xa_vdi_settings *v, ICONBLK *iconblk, short state, short obx, short oby, short icx, short icy)
{
	char lc = iconblk->ib_char;
	short tx,ty,pnt[4];

	(*v->api->wr_mode)(v, MD_REPLACE);
	ritopxy(pnt, obx + iconblk->ib_xtext, oby + iconblk->ib_ytext,
		     iconblk->ib_wtext, iconblk->ib_htext);

	(*v->api->t_font)(v, screen.small_font_point, screen.small_font_id);

	/* center the text in a bar given by iconblk->tx, relative to object */
	(*v->api->t_color)(v, G_BLACK);
	if (   iconblk->ib_ptext
	    && *iconblk->ib_ptext
	    && iconblk->ib_wtext
	    && iconblk->ib_htext)
	{
		tx = obx + iconblk->ib_xtext + ((iconblk->ib_wtext - strlen(iconblk->ib_ptext)*6) / 2);	
		ty = oby + iconblk->ib_ytext + ((iconblk->ib_htext - 6) / 2);
		
		if (state & OS_STATE08)
		{
			short col;
			if (state & OS_SELECTED)
				col = G_WHITE;
			else
				col = G_BLACK;
			(*v->api->wr_mode)(v, MD_TRANS);
			(*v->api->t_color)(v, col);
// 			display("disp icontext without banner");
		}
		else
		{
			(*v->api->f_color)(v, (state & OS_SELECTED) ? G_BLACK : G_WHITE);
			(*v->api->f_interior)(v, FIS_SOLID);
			v_bar(v->handle, pnt);
	
			if (state & OS_SELECTED)
				(*v->api->wr_mode)(v, MD_XOR);
			if (state & OS_DISABLED)
				(*v->api->t_effects)(v, FAINT);
		}
		v_gtext(v->handle, tx, ty, iconblk->ib_ptext);
#if 0		
		(*v->api->f_color)(v, (state & OS_SELECTED) ? G_BLACK : G_WHITE);
		(*v->api->f_interior)(v, FIS_SOLID);
		v_bar(v->handle, pnt);
	
		tx = obx + iconblk->ib_xtext + ((iconblk->ib_wtext - strlen(iconblk->ib_ptext)*6) / 2);	
		ty = oby + iconblk->ib_ytext + ((iconblk->ib_htext - 6) / 2);

		if (state & OS_SELECTED)
			(*v->api->wr_mode)(v, MD_XOR);
		if (state & OS_DISABLED)
			(*v->api->t_effects)(v, FAINT);

		v_gtext(v->handle, tx, ty, iconblk->ib_ptext);
#endif
	}

	if (lc != 0 && lc != ' ')
	{
		char ch[2];
		ch[0] = lc;
		ch[1] = 0;
		v_gtext(v->handle, icx + iconblk->ib_xchar, icy + iconblk->ib_ychar, ch);
		/* Seemingly the ch is supposed to be relative to the image */
	}

	if (state & OS_SELECTED)
		(*v->api->f_color)(v, G_WHITE);

	//t_font(screen.standard_font_point, screen.standard_font_id);
	(*v->api->t_effects)(v, 0);
	(*v->api->wr_mode)(v, MD_TRANS);
}

/*
 * Draw a image
 */
void
d_g_image(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	OBJECT *ob = wt->tree + wt->current;
	BITBLK *bitblk;
	MFDB Mscreen;
	MFDB Micon;
	short pxy[8], cols[2], icx, icy;

	bitblk = object_get_spec(ob)->bitblk;

	icx = wt->r.x;
	icy = wt->r.y;

	Mscreen.fd_addr = NULL;
			
	Micon.fd_w = (bitblk->bi_wb & -2) << 3;
	Micon.fd_h = bitblk->bi_hl;
	Micon.fd_wdwidth = (Micon.fd_w + 15) >> 4;
	Micon.fd_nplanes = 1;
	Micon.fd_stand = 0;
	Micon.fd_addr = bitblk->bi_pdata;

	cols[0] = bitblk->bi_color;	/* HR 100202: simply 1 foreground color index. */
	cols[1] = 0;			/* background color; not used in mode MD_TRANS. */

	pxy[0] = bitblk->bi_x;
	pxy[1] = bitblk->bi_y;
	pxy[2] = Micon.fd_w - 1 - pxy[0];
	pxy[3] = Micon.fd_h - 1 - pxy[1];
	pxy[4] = icx;
	pxy[5] = icy;
	pxy[6] = icx + pxy[2];
	pxy[7] = icy + pxy[3];

	vrt_cpyfm(v->handle, MD_TRANS, pxy, &Micon, &Mscreen, cols);
}

/* HR 060202: all icons: handle disabled: make icon rectangle and text faint in stead
 *                                        of object rectangle.
 */

static unsigned short dismskb[10 * 160];
static MFDB Mddm =
{
	dismskb,
	160, 160,
	10,
	1,
	1,
	0,0,0
};
/*
 * Draw a mono icon
 */
void
d_g_icon(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	OBJECT *ob = wt->tree + wt->current;
	ICONBLK *iconblk;
	MFDB Mscreen;
	MFDB Micon;
	RECT ic;
	short pxy[8], cols[2], obx, oby, msk_col, icn_col, blitmode;
	
	iconblk = object_get_spec(ob)->iconblk;
	obx = wt->r.x;
	oby = wt->r.y;

	ic = *(RECT*)&iconblk->ib_xicon;
	ic.x += obx;
	ic.y += oby;

	ritopxy(pxy,     0, 0, ic.w, ic.h);
	rtopxy (pxy + 4, &ic);

	Micon.fd_w = ic.w;
	Micon.fd_h = ic.h;
	Micon.fd_wdwidth = (ic.w + 15) >> 4;
	Micon.fd_nplanes = 1;
	Micon.fd_stand = 0;
	Mscreen.fd_addr = NULL;

	if (ob->ob_state & OS_SELECTED)
	{
		icn_col = ((iconblk->ib_char) >> 8) & 0xf;
		msk_col = ((iconblk->ib_char) >> 12) & 0xf;
	}
	else
	{
		icn_col = ((iconblk->ib_char) >> 12) & 0xf;
		msk_col = ((iconblk->ib_char) >> 8) & 0xf;
	}
	/* Ozk:
	 * Dont draw mask when WHITEBAK set
	 */
	if (!(ob->ob_state & OS_WHITEBAK))
	{
		Micon.fd_addr = iconblk->ib_pmask;
		cols[0] = msk_col;
		cols[1] = icn_col;
		vrt_cpyfm(v->handle, MD_TRANS, pxy, &Micon, &Mscreen, cols);
		blitmode = MD_TRANS;
	}
	else
		blitmode = MD_REPLACE;
		
	Micon.fd_addr = iconblk->ib_pdata;
	cols[0] = icn_col;
	cols[1] = msk_col;
	vrt_cpyfm(v->handle, blitmode, pxy, &Micon, &Mscreen, cols);
		
	if (ob->ob_state & OS_DISABLED)
	{
		int i, j, sc;			
		unsigned short *s, *d, m;
		unsigned long sm = 0xAAAA5555L;

		s = iconblk->ib_pmask; //Micon.fd_addr;
		d = Mddm.fd_addr;
		sc = Mddm.fd_wdwidth - Micon.fd_wdwidth;

		for (i = 0; i < Micon.fd_h; i++)
		{
			m = sm;
			for (j = 0; j < Micon.fd_wdwidth; j++)
			{
				*d++ = *s++ & m;
			}
			sm = (sm << 16) | (sm >> 16);
			d += sc;
		}

		if (screen.planes > 3)
			cols[0] = G_LWHITE;
		else
			cols[0] = G_WHITE;
		
		cols[1] = G_WHITE;
		vrt_cpyfm(v->handle, MD_TRANS, pxy, &Mddm, &Mscreen, cols);
	}

	/* should be the same for color & mono */
	icon_characters(v, iconblk, ob->ob_state & (OS_SELECTED|OS_DISABLED), obx, oby, ic.x, ic.y);

	done(OS_SELECTED|OS_DISABLED);
}

/*
 * Draw a colour icon
 */
void
d_g_cicon(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	OBJECT *ob = wt->tree + wt->current;
	ICONBLK *iconblk;
	CICON	*best_cicon;
	MFDB Mscreen;
	MFDB Micon, Mmask;
	bool have_sel;
	RECT ic;
	short pxy[8], cols[2] = {0,1}, obx, oby, blitmode;

	best_cicon = getbest_cicon(object_get_spec(ob)->ciconblk);
	/* No matching icon, so use the mono one instead */
	if (!best_cicon)
	{
		//DIAG((D_o, wt->owner, "cicon !best_cicon", c));
		d_g_icon(lock, wt, v);
		return;
	}

	iconblk = object_get_spec(ob)->iconblk;
	obx = wt->r.x;
	oby = wt->r.y;

	ic = *(RECT*)&iconblk->ib_xicon;

	ic.x += obx;
	ic.y += oby;

	ritopxy(pxy,     0, 0, ic.w, ic.h);
	rtopxy (pxy + 4, &ic);
			
	Micon.fd_w = ic.w;
	Micon.fd_h = ic.h;
	Micon.fd_wdwidth = (ic.w + 15) >> 4;
	Micon.fd_nplanes = 1;
	Micon.fd_stand = 0;
	Mscreen.fd_addr = NULL;

	Mmask = Micon;
	//have_sel = c->sel_data != NULL;
	have_sel = best_cicon->sel_data ? true : false;

	//DIAG((D_o, wt->owner, "cicon sel_mask 0x%lx col_mask 0x%lx", c->sel_mask, c->col_mask));

	
	/* check existence of selection. */
	if ((ob->ob_state & OS_SELECTED) && have_sel)
	{
		Micon.fd_addr = best_cicon->sel_data;
		Mmask.fd_addr = best_cicon->sel_mask;
	}
	else
	{
		Micon.fd_addr = best_cicon->col_data;
		Mmask.fd_addr = best_cicon->col_mask;
	}
	
	if (!(ob->ob_state & OS_WHITEBAK))
	{
		vrt_cpyfm(v->handle, MD_TRANS, pxy, &Mmask, &Mscreen, cols);
		blitmode = screen.planes > 8 ? S_AND_D : S_OR_D;
	}
	else
		blitmode = S_ONLY;

	Micon.fd_nplanes = screen.planes;
	
	vro_cpyfm(v->handle, blitmode, pxy, &Micon, &Mscreen);
	icon_characters(v, iconblk, ob->ob_state & (OS_SELECTED|OS_DISABLED), obx, oby, ic.x, ic.y);

	if ((ob->ob_state & OS_DISABLED) || ((ob->ob_state & OS_SELECTED) && !have_sel))
	{
		int i, j, sc;			
		unsigned short *s, *d, m;
		unsigned long sm = 0xAAAA5555L;

		s = Mmask.fd_addr;
		d = Mddm.fd_addr;
		sc = Mddm.fd_wdwidth - Mmask.fd_wdwidth;

		for (i = 0; i < Micon.fd_h; i++)
		{
			m = sm;
			for (j = 0; j < Micon.fd_wdwidth; j++)
			{
				*d++ = *s++ & m;
			}
			sm = (sm << 16) | (sm >> 16);
			d += sc;
		}

		if (ob->ob_state & OS_DISABLED)
			cols[0] = G_LWHITE;
		else
			cols[0] = G_BLACK;

		cols[1] = G_WHITE;				
		vrt_cpyfm(v->handle, MD_TRANS, pxy, &Mddm, &Mscreen, cols);
	}
	done(OS_SELECTED|OS_DISABLED);
}

/*
 * Draw a text object
 */
void
d_g_text(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	short thick,thin;
	OBJECT *ob = wt->tree + wt->current;
	RECT r = wt->r, gr;
	BFOBSPEC colours;
	struct objc_edit_info *ei;
	char temp_text[256];

	(*v->api->t_effects)(v, 0);
	set_text(ob, v, &gr, NULL, false, -1, temp_text, &colours, &thick, NULL, &ei, r);
	set_colours(ob, v, &colours);
	thin = thick > 0 ? thick-1 : thick+1;

	if (d3_foreground(ob))
	{
		d3_pushbutton(v, thick > 0 ? -thick : 0, &r, &colours, ob->ob_state, thin, 3);
		done(OS_SELECTED);
	}

	ob_text(wt, v, ei, &gr, &r, &colours, temp_text, ob->ob_state, 0, -1, G_BLACK);
}
void
d_g_ftext(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	short thick,thin;
	OBJECT *ob = wt->tree + wt->current;
	RECT r = wt->r, gr, cr;
	BFOBSPEC colours;
	struct objc_edit_info *ei;
	char temp_text[256];

	ei = wt->ei ? wt->ei : &wt->e;
	if (ei->obj != wt->current)
		ei = NULL;

	(*v->api->t_effects)(v, 0);
	set_text(ob, v, &gr, &cr, true, ei ? ei->pos : -1, temp_text, &colours, &thick, NULL, &ei, r);
	set_colours(ob, v, &colours);
	thin = thick > 0 ? thick-1 : thick+1;

	if (d3_foreground(ob))
	{
		d3_pushbutton(v, thick > 0 ? -thick : 0, &r, &colours, ob->ob_state, thin, 3);
		done(OS_SELECTED);
	}

	ob_text(wt, v, ei, &gr, &r, &colours, temp_text, ob->ob_state, 0, -1, G_BLACK);
}

#define userblk(ut) (*(USERBLK **)(ut->userblk_pp))
#define ret(ut)     (     (long *)(ut->ret_p     ))
#define parmblk(ut) (  (PARMBLK *)(ut->parmblk_p ))
void
d_g_progdef(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	struct sigaction oact, act;
	struct xa_client *client = lookup_extension(NULL, XAAES_MAGIC);
	OBJECT *ob = wt->tree + wt->current;
	PARMBLK *p;
	//short r[4];
	RECT save_clip;

#if GENERATE_DIAGS
	struct proc *curproc = get_curproc();

	DIAGS(("XaAES d_g_progdef: curproc - pid %i, name %s", curproc->pid, curproc->name));
	DIAGS(("XaAES d_g_progdef: client  - pid %i, name %s", client->p->pid, client->name));
#endif

//	KERNEL_DEBUG("ut = 0x%lx", ut);
//	KERNEL_DEBUG("ut->progdef_p = 0x%lx", ut->progdef_p);
//	KERNEL_DEBUG("ut->userblk_p = 0x%lx", ut->userblk_p);
//	KERNEL_DEBUG("ut->ret_p     = 0x%lx", ut->ret_p    );
//	KERNEL_DEBUG("ut->parmblk_p = 0x%lx", ut->parmblk_p);

	if ((client->status & CS_EXITING))
		return;
		
	p = parmblk(client->ut);
	p->pb_tree = wt->tree;
	p->pb_obj = wt->current;

	p->pb_prevstate = p->pb_currstate = ob->ob_state;

	*(RECT *)&(p->pb_x) = wt->r;

	*(RECT *)&(p->pb_xc) = save_clip = v->clip; //*clip;
	userblk(client->ut) = object_get_spec(ob)->userblk;
	p->pb_parm = userblk(client->ut)->ub_parm;

	(*v->api->wr_mode)(v, MD_TRANS);

#if GENERATE_DIAGS
	{
		char statestr[128];

		show_bits(*wt->state_mask & ob->ob_state, "state ", pstates, statestr);
		DIAG((D_o, wt->owner, "progdef before %s %04x", statestr, *wt->state_mask & ob->ob_state));
	}
#endif
	act.sa_handler = client->ut->progdef_p;
	act.sa_mask = 0xffffffff;
	act.sa_flags = SA_RESET;

	/* set new signal handler; remember old */
	p_sigaction(SIGUSR2, &act, &oact);

	DIAGS(("raise(SIGUSR2)"));
	raise(SIGUSR2);
	DIAGS(("handled SIGUSR2 progdef callout"));

	/* restore old handler */
	p_sigaction(SIGUSR2, &oact, NULL);
	
	/* The PROGDEF function returns the ob_state bits that
	 * remain to be handled by the AES:
	 */
	*wt->state_mask = *ret(client->ut);

	/* BUG: OS_SELECTED bit only handled in non-color mode!!!
	 * (Not too serious I believe... <mk>)
	 * HR: Yes I would call that correct, cause in color mode
	 *     selected appearance is object specific.
	 *     I even think that it shouldnt be done at ALL.
	 * 		
	 * The whole state_mask returning mechanism of progdefs should
         * be taken into account.
         * So we actually dont need to do anything special here.
         * But for progdef'd menu separators it is nicer to check here
         * and use bg_col.	              
	 */

#if GENERATE_DIAGS
	{
		char statestr[128];

		show_bits(*wt->state_mask, "state ", pstates, statestr);
		DIAG((D_o, wt->owner, "progdef remaining %s %04x", statestr, *wt->state_mask));
	}
#endif
	/*
	 * Ozk: Since it is possible that taskswitches happens during the callout of
	 *	progdef's, we need to restore the clip-rect used by this 'thread'
	 */

// 	*(RECT *)&r = v->clip; //*clip;
// 	r[2] += (r[0] - 1);
// 	r[3] += (r[1] - 1);
// 	vs_clip(v->handle, 1, (short *)&r);
	(*v->api->set_clip)(v, &save_clip);
	
	if (*wt->state_mask & OS_DISABLED)
	{
		(*v->api->write_disable)(v, &wt->r, objc_rend.dial_colours.bg_col);
		done(OS_DISABLED);
	}
	(*v->api->wr_mode)(v, MD_REPLACE);
}
#undef userblk
#undef ret
#undef parmblk

void
d_g_slist(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	RECT r = wt->r, wa;
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	struct xa_window *w;
	OBJECT *ob = wt->tree + wt->current;

	/* list = object_get_spec(ob)->listbox; */
	list = (SCROLL_INFO*)object_get_spec(ob)->index;
	w = list->wi;

	w->r.x = w->rc.x = r.x;
	w->r.y = w->rc.y = r.y;

	/* for after moving */
	calc_work_area(w);

	wa = w->wa;
	//y = wa.y;
	//maxy = y + wa.h - screen.c_max_h;
	this = list->top;

	(*v->api->t_color)(v, G_BLACK);
	
	if (list->state == 0)
	{
		get_widget(w, XAW_TITLE)->stuff = list->title;
		r = v->clip;
		draw_window(list->lock, w, &r);
		draw_slist(lock, list, NULL, &r);
	}
	done(OS_SELECTED);
}

void
d_g_string(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	ushort state = ob->ob_state;
	char *t, text[256];

	t = object_get_spec(ob)->free_string;

	/* most AES's allow null string */
	if (t)
	{
		bool tit = ( state & OS_WHITEBAK )
		           && ( state & 0xff00   ) == 0xff00;
			
		strncpy(text, t, 254);
		text[255] = '\0';
		
		(*v->api->wr_mode)(v, MD_TRANS);

		if (d3_foreground(ob) && !tit)
			d3_pushbutton(v, 0, &r, NULL, state, 0, 0);

		if (   wt->is_menu
		    && (ob->ob_state & OS_DISABLED)
		    && *text == '-')
		{
			r.x += 1, r.w -= 3;
			r.y += (r.h - 2)/2;
			if (MONO)
			{
				(*v->api->l_type)(v, 7);
				(*v->api->l_udsty)(v, 0xaaaa);
				(*v->api->line)(v, r.x, r.y, r.x + r.w, r.y, G_BLACK);
				(*v->api->l_udsty)(v, 0x5555);
				(*v->api->line)(v, r.x, r.y + 1, r.x + r.w, r.y + 1, G_BLACK);
				(*v->api->l_type)(v, 0);
			}
			else
			{
				r.x += 2, r.w -= 4;
				(*v->api->line)(v, r.x, r.y,     r.x + r.w, r.y,     objc_rend.dial_colours.fg_col);
				(*v->api->line)(v, r.x, r.y + 1, r.x + r.w, r.y + 1, objc_rend.dial_colours.lit_col);
				(*v->api->l_color)(v, G_BLACK);
			}
			done(OS_DISABLED);
		}
		else
		{
			short und = -1, flags = 0;
			if ((state & OS_WHITEBAK))
			{
				short ns = state >> 8;
				if (!(ns & 0x80))
				{
					und = ns & 0x7f;
				}
				else if (ns == 0xfe)
				{
					und = -2;
					flags = ob->ob_flags;
				}
				else if (ns == 0xff)
					flags = ob->ob_flags;
			}
			(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
			(*v->api->t_color)(v, G_BLACK);
			(*v->api->t_effects)(v, 0);
			ob_text(wt, v, NULL, &r, &wt->r, NULL, t, state, flags, und, G_BLACK);
// 			g_text(wt, v, r, &wt->r, text, ob->ob_state);
		}

		if (tit)
			d3_bottom_line(v, &r, (ob->ob_flags & OF_FL3DBAK), G_BLACK, G_WHITE);
	
	}
}

void
d_g_title(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v)
{
	OBJECT *ob = wt->tree + wt->current;
	char *t = object_get_spec(ob)->free_string;

	if (t)
	{
		short und = -1, state = ob->ob_state;
		RECT r = wt->r;
		char text[256];
		strncpy(text, t, 254);
		text[255] = '\0';

		(*v->api->wr_mode)(v, MD_TRANS);

		/* menu in user window.*/
		if (!wt->menu_line)
			d3_pushbutton(v, -2, &r, NULL, ob->ob_state, 1, MONO ? 1 : 3);

		/* most AES's allow null string */
		(*v->api->t_color)(v, G_BLACK);
		(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
		(*v->api->t_effects)(v, 0);
		
		if (state & 0x8000)
			und = (state >> 8) & 0x7f;
		
		ob_text(wt, v, NULL, &r, &wt->r, NULL, text, ob->ob_state, OF_FL3DBAK, und, G_BLACK);
// 		g_text(wt, v, r, &wt->r, text, ob->ob_state);

		if (ob->ob_state & OS_SELECTED && wt->menu_line)
			/* very special!!! */
			write_selection(v, 0, &r);
	}
	done(OS_SELECTED);
}

XA_TREE nil_tree = { 0 };
static ObjectDisplay *objc_jump_table[G_UNKNOWN];

/*
 * Initialise the object display jump table
 */
void
init_objects(void)
{
	short f;

	nil_tree.e.obj = -1;
	nil_tree.focus = -1;

	for (f = 0; f <= G_UNKNOWN; f++)
		/* Anything with a NULL pointer won't get called */
		objc_jump_table[f] = NULL;

	objc_jump_table[G_BOX     ] = d_g_box;
	objc_jump_table[G_TEXT    ] = d_g_text;
	objc_jump_table[G_BOXTEXT ] = d_g_boxtext;
	objc_jump_table[G_IMAGE   ] = d_g_image;
	objc_jump_table[G_PROGDEF ] = d_g_progdef;
	objc_jump_table[G_IBOX    ] = d_g_ibox;
	objc_jump_table[G_BUTTON  ] = d_g_button;
	objc_jump_table[G_BOXCHAR ] = d_g_boxchar;
	objc_jump_table[G_STRING  ] = d_g_string;
	objc_jump_table[G_FTEXT   ] = d_g_ftext;
	objc_jump_table[G_FBOXTEXT] = d_g_fboxtext;
	objc_jump_table[G_ICON    ] = d_g_icon;
	objc_jump_table[G_TITLE   ] = d_g_title;
	objc_jump_table[G_CICON   ] = d_g_cicon;

// 	objc_jump_table[G_SWBUTTON] = d_g_swbutton;
	objc_jump_table[G_POPUP   ] = d_g_button;
// 	objc_jump_table[G_WINTITLE] = d_g_wintitle;
// 	objc_jump_table[G_EDIT    ] = d_g_edit;

	objc_jump_table[G_SHORTCUT] = d_g_string;
	objc_jump_table[G_SLIST   ] = d_g_slist;
	objc_jump_table[G_EXTBOX  ] = d_g_box;

}

void
init_ob_render(void)
{
	rootmenu_texture = NULL;
#if 0
	struct xa_texture *t;
	char *fn;

	if ((t = kmalloc(sizeof(*t))))
	{
		t->mfdb.fd_addr = NULL;
		if ((fn = xaaes_sysfile("img\\ob.img")))
		{
			load_image(fn, &t->mfdb);
			if (t->mfdb.fd_addr)
			{
				t->t.anchor = 0;
				t->t.flags = 0;
				t->t.tl_corner = t->t.bl_corner = t->t.tr_corner = t->t.br_corner = NULL;
				t->t.top = t->t.bottom = t->t.left = t->t.right = NULL;
				t->t.body = &t->mfdb;
				
				rootmenu_texture = t;
			}
		}
		kfree(fn);
		if (!t->mfdb.fd_addr)
			kfree(t);
	}
#endif
}			

void
exit_ob_render(void)
{
	if (rootmenu_texture)
	{
		kfree(rootmenu_texture->mfdb.fd_addr);
		kfree(rootmenu_texture);
		rootmenu_texture = NULL;
	}
}

/* object types */
#define G_BOX			20
#define G_TEXT			21
#define G_BOXTEXT		22
#define G_IMAGE			23
#define G_USERDEF		24
#define G_PROGDEF		G_USERDEF
#define G_IBOX			25
#define G_BUTTON		26
#define G_BOXCHAR		27
#define G_STRING		28
#define G_FTEXT			29
#define G_FBOXTEXT		30
#define G_ICON			31
#define G_TITLE			32
#define G_CICON			33

/* extended object types, MagiC only */
#define G_SWBUTTON		34
#define G_POPUP			35
#define G_WINTITLE		36
#define G_EDIT			37
#define G_SHORTCUT		38

/*
 * Display a primitive object
 */
void
display_object(enum locks lock, XA_TREE *wt, struct xa_vdi_settings *v, short item, short parent_x, short parent_y, short which)
{
	RECT r, o, sr;
	OBJECT *ob = wt->tree + item;
	ObjectDisplay *display_routine = NULL;

	/* HR: state_mask is for G_PROGDEF originally.
	 * But it means that other objects must unflag what they
	 * can do themselves in the same manner.
	 * The best thing (no confusion) is to generalize the concept.
	 * Which I did. :-)
	 */
	unsigned short state_mask = (OS_SELECTED|OS_CROSSED|OS_CHECKED|OS_DISABLED|OS_OUTLINED);
	unsigned short t = ob->ob_type & 0xff;

	object_offsets(ob, &o);

	r.x = parent_x + ob->ob_x;
	r.y = parent_y + ob->ob_y;
	r.w = ob->ob_width; 
	r.h = ob->ob_height;
	sr = r;

	o.x = r.x + o.x;
	o.y = r.y + o.y;
	o.w = r.w - o.w;
	o.h = r.h - o.h;
	
	if (   o.x		> (v->clip.x + v->clip.w - 1)
	    || o.x + o.w - 1	< v->clip.x
	    || o.y		> (v->clip.y + v->clip.h - 1)
	    || o.y + o.h - 1	< v->clip.y)
		return;

	if (t <= G_UNKNOWN)
		/* Get display routine for this type of object from jump table */
		display_routine = objc_jump_table[t];

//	if (t >= G_SWBUTTON && t <= G_SHORTCUT)
//		display("display %s(%d) for %s", ob_types[t], t, wt->owner->name);

	if (display_routine == NULL)
	{
		DIAG((D_objc,wt->owner,"no display_routine! ob_type: %d(0x%x)", t, ob->ob_type));
		/* dont attempt doing what could be indeterminate!!! */
		return;
	}

	/* Fill in the object display parameter structure */			
	wt->current = item;
	wt->parent_x = parent_x;
	wt->parent_y = parent_y;
	/* absolute RECT, ready for use everywhere. */
	wt->r = r;
	wt->state_mask = &state_mask;

	/* Better do this before AND after (fail safe) */
	(*v->api->wr_mode)(v, MD_TRANS);

#if 1
#if GENERATE_DIAGS
	if (wt->tree != xobj_rsc) //get_widgets())
	{
		char flagstr[128];
		char statestr[128];

		show_bits(ob->ob_flags, "flg=", pflags, flagstr);
		show_bits(ob->ob_state, "st=", pstates, statestr);

		DIAG((D_o, wt->owner, "ob=%d, %d/%d,%d/%d [%d: 0x%lx]; %s%s (%x)%s (%x)%s",
			 item,
			 r.x, r.y, r.w, r.h,
			 t, display_routine,
			 object_type(wt->tree, item),
			 object_txt(wt->tree, item),
			 ob->ob_flags,flagstr,
			 ob->ob_state,statestr));
	}
#endif
#endif

	/* Call the appropriate display routine */
	(*display_routine)(lock, wt, v);

	(*v->api->wr_mode)(v, MD_TRANS);

	if (t != G_PROGDEF)
	{
		/* Handle CHECKED object state: */
		if ((ob->ob_state & state_mask) & OS_CHECKED)
		{
			(*v->api->t_color)(v, G_BLACK);
			/* ASCII 8 = checkmark */
			v_gtext(v->handle, r.x + 2, r.y, "\10");
		}

		/* Handle DISABLED state: */
		if ((ob->ob_state & state_mask) & OS_DISABLED)
			(*v->api->write_disable)(v, &r, G_WHITE);

		/* Handle CROSSED object state: */
		if ((ob->ob_state & state_mask) & OS_CROSSED)
		{
			short p[4];
			(*v->api->l_color)(v, G_BLACK);
			p[0] = r.x;
			p[1] = r.y;
			p[2] = r.x + r.w - 1;
			p[3] = r.y + r.h - 1;
			v_pline(v->handle, 2, p);
			p[0] = r.x + r.w - 1;
			p[2] = r.x;
			v_pline(v->handle, 2, p);
		}

		/* Handle OUTLINED object state: */
		if ((ob->ob_state & state_mask) & OS_OUTLINED)
		{
			/* special handling of root object. */
			if (!wt->zen || item != 0)
			{
				if (!MONO && d3_any(ob))
				{
					(*v->api->tl_hook)(v, 1, &r, objc_rend.dial_colours.lit_col);
					(*v->api->br_hook)(v, 1, &r, objc_rend.dial_colours.shadow_col);
					(*v->api->tl_hook)(v, 2, &r, objc_rend.dial_colours.lit_col);
					(*v->api->br_hook)(v, 2, &r, objc_rend.dial_colours.shadow_col);
					(*v->api->gbox)(v, 3, &r);
				}
				else
				{
					(*v->api->l_color)(v, G_WHITE);
					(*v->api->gbox)(v, 1, &r);
					(*v->api->gbox)(v, 2, &r);
					(*v->api->l_color)(v, G_BLACK);
					(*v->api->gbox)(v, 3, &r);
				}
			}
		}

		if ((ob->ob_state & state_mask) & OS_SELECTED)
			write_selection(v, 0, &r);
	}

	if (wt->focus == item)
	{
		(*v->api->wr_mode)(v, MD_TRANS);
		(*v->api->l_color)(v, G_RED);
		(*v->api->l_type)(v, 7);
		(*v->api->l_udsty)(v, 0xaaaa); //0xe0e0); //0xaaaa);

		(*v->api->gbox)(v, 0, &sr);
		(*v->api->l_type)(v, 0);
	}

	(*v->api->wr_mode)(v, MD_TRANS);
}

/*
 * Walk an object tree, calling display for each object
 * HR: is_menu is true if a menu.
 */

/* draw_object_tree */
short
draw_object_tree(enum locks lock, XA_TREE *wt, OBJECT *tree, struct xa_vdi_settings *v, short item, short depth, short *xy, short flags)
{
	XA_TREE this;
	short next;
	short current = 0, stop = -1, rel_depth = 1, head;
	short x, y;
	struct objc_edit_info *ei = wt->ei;
	bool start_drawing = false;
	bool curson;
	bool docurs = ((flags & DRW_CURSOR));

	if (wt == NULL)
	{
		this = nil_tree;

		wt = &this;
		wt->e.obj = -1;
		wt->ei = NULL;
		wt->owner = C.Aes;
	}

	if (tree == NULL)
		tree = wt->tree;
	else
		wt->tree = tree;

	if (!wt->owner)
		wt->owner = C.Aes;

	/* dx,dy are provided by sliders if present. */
	x = -wt->dx;
	y = -wt->dy;
	DIAG((D_objc, wt->owner, "dx = %d, dy = %d", x, y));
	DIAG((D_objc, wt->owner, "draw_object_tree for %s to %d/%d,%d/%d; %lx + %d depth:%d",
		t_owner(wt), x + tree->ob_x, y + tree->ob_y,
		tree->ob_width, tree->ob_height, tree, item, depth));
	DIAG((D_objc, wt->owner, "  -   (%d)%s%s",
		wt->is_menu, obtree_is_menu(tree) ? "menu" : "object", wt->zen ? " with zen" : ""));
// 	DIAG((D_objc, wt->owner, "  -   clip: %d.%d/%d.%d    %d/%d,%d/%d",
// 		cl[0], cl[1], cl[2], cl[3], cl[0], cl[1], cl[2] - cl[0] + 1, cl[3] - cl[1] + 1));

	depth++;

	if (ei)
	{
		curson = ((ei->c_state & (OB_CURS_ENABLED | OB_CURS_DRAWN)) == (OB_CURS_ENABLED | OB_CURS_DRAWN)) ? true : false;
		if (curson) undraw_objcursor(wt, v, NULL);
	}
	else
		curson = false;

	do {

	//	DIAG((D_objc, NULL, " ~~~ obj=%d(%d/%d), flags=%x, state=%x, head=%d, tail=%d, next=%d, depth=%d, draw=%s",
	//		current, x, y, tree[current].ob_flags, tree[current].ob_state,
	//		tree[current].ob_head, tree[current].ob_tail, tree[current].ob_next,
	//		rel_depth, start_drawing ? "yes":"no"));

		if (current == item)
		{
			stop = item;
			start_drawing = true;
			rel_depth = 0;
		}

		if (start_drawing && !(tree[current].ob_flags & OF_HIDETREE))
		{
			display_object(lock, wt, v, current, x, y, 10);
			/* Display this object */
			if (!ei && docurs && current == wt->e.obj)
			{
// 				display("redrawing cursor for obj %d", current);
				if ((tree[current].ob_type & 0xff) != G_USERDEF)
					eor_objcursor(wt, v, NULL); // display("redrawing cursor for obj %d", current);
// 				else
// 					display("skipping progdef curs");
				docurs = false;	
			}
		}

		head = tree[current].ob_head;

		/* Any non-hidden children? */
		if (    head != -1
		    && !(tree[current].ob_flags & OF_HIDETREE)
		    &&  (!start_drawing || (start_drawing && rel_depth < depth)))
		{
			x += tree[current].ob_x;
			y += tree[current].ob_y;

			rel_depth++;

			current = head;
		}
		else
		{
			/* Try for a sibling */
			next = tree[current].ob_next;

			/* Trace back up tree if no more siblings */
			while (next != stop && next != -1/*-1*/ && tree[next].ob_tail == current)
			{
				current = next;
				x -= tree[current].ob_x;
				y -= tree[current].ob_y;
				next = tree[current].ob_next;
				rel_depth--;
			}
			current = next;
		}
	}
	while (current != stop && current != -1 && rel_depth > 0); // !(start_drawing && rel_depth < 1));

	if (curson)
		draw_objcursor(wt, v, NULL);

	(*v->api->wr_mode)(v, MD_TRANS);
	(*v->api->f_interior)(v, FIS_SOLID);

	DIAGS(("draw_object_tree exit OK!"));
	return true;
}


#if 1


#endif
