/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *
 * A multitasking AES replacement for MiNT
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

#include <mint/osbind.h>

#include "xa_types.h"
#include "xa_global.h"

#include WIDGHNAME

#include "xalloc.h"
#include "rectlist.h"
#include "objects.h"
/* HR: for windowed list boxes */
#include "c_window.h"
#include "widgets.h"


#define done(x) (*wt->state_mask &= ~(x))

OBSPEC *
get_ob_spec(OBJECT *ob)
{
	return (ob->ob_flags & INDIRECT) ?
			ob->ob_spec.indirect : &ob->ob_spec;
}

bool
is_spec(OBJECT *tree, int item)
{
	switch (tree[item].ob_type & 0xff)
	{
		case G_BOX:
		case G_IBOX:
		case G_BOXCHAR:
		return true;
	}
	return false;
}

bool d3_any(OBJECT *ob)        { return (ob->ob_flags & FLD3DANY) != 0;	}
bool d3_indicator(OBJECT *ob)  { return (ob->ob_flags & FLD3DANY) == FLD3DIND; }
bool d3_foreground(OBJECT *ob) { return (ob->ob_flags & FLD3DIND) != 0; }
bool d3_background(OBJECT *ob) { return (ob->ob_flags & FLD3DANY) == FLD3DBAK; }
bool d3_activator(OBJECT *ob)  { return (ob->ob_flags & FLD3DANY) == FLD3DACT; }

void wr_mode(int m)
{
	static int mode = -1;

	if (mode != m)
	{
		mode = m;
		vswr_mode(C.vh, mode);
	}
}

void l_color(int m)
{
	static int mode = -1;

	if (mode != m)
	{
		mode = m;
		vsl_color(C.vh, mode);
	}
}

void f_color(int m)
{
	static int mode = -1;

	if (mode != m)
	{
		mode = m;
		vsf_color(C.vh, mode);
	}
}

void t_color(int m)
{
	static int mode = -1;

	if (mode != m)
	{
		mode = m;
		vst_color(C.vh, mode);
	}
}

void t_effect(int m)
{
	static int mode = -1;

	if (mode != m)
	{
		mode = m;
		vst_effects(C.vh, mode);
	}
}

void t_font(int p, int f)
{
	static int pm = -1;
	static int fm = -1;
	short temp;

	if (pm != p)
	{
		pm = p;
		vst_point(C.vh, pm, &temp, &temp, &temp, &temp);
	}

	if (fm != f)
	{
		fm = f;
		vst_font(C.vh, fm);
	}
}

void f_interior(int m)
{
	static int mode = -1;

	if (mode != m)
	{
		mode = m;
		vsf_interior(C.vh, mode);
	}
}

void f_style(int m)
{
	static int mode = -1;
	if (m != mode)
		vsf_style(C.vh, mode = m);
}

#ifndef __GNUC__
void hidem(void)  { v_hide_c(C.vh);    }
void showm(void)  { v_show_c(C.vh, 1); }
void forcem(void) { v_show_c(C.vh, 0); }
#endif

void deselect(OBJECT *tree, int item)
{
	(tree + item)->ob_state &= ~SELECTED;
}

/* HR: pxy wrapper functions (Beware the (in)famous -1 bug */
/* Also vsf_perimeter only works in XOR writing mode */

void write_menu_line(RECT *cl)
{
	short pnt[4];
	l_color(BLACK);
	pnt[0] = cl->x;
	pnt[1] = cl->y + MENU_H;
	pnt[2] = cl->x + cl->w - 1;
	pnt[3] = pnt[1];
	v_pline(C.vh,2,pnt);
}

void rtopxy(short *p, const RECT *r)
{
	*p++ = r->x;
	*p++ = r->y;
	*p++ = r->x + r->w-1; 
	*p   = r->y + r->h-1;
}

void ritopxy(short *p, short x, short y, short w, short h)
{
	*p++ = x;
	*p++ = y;
	*p++ = x + w-1; 
	*p   = y + h-1;
}

void line(short x, short y, short x1, short y1, int col)
{
	short pxy[4];

	pxy[0]=x;pxy[2]=x1;
	pxy[1]=y;pxy[3]=y1;
	l_color(col);
	v_pline(C.vh,2,pxy);
}

void bar(int d,  short x, short y, short w, short h)
{
	short l[4];
	x -= d, y -= d, w += d+d, h += d+d;
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y+h-1;
	v_bar(C.vh, l);
}

void gbar(int d, RECT *r)	/* for perimeter = 0 */
{
	short l[4];
	l[0] = r->x - d;
	l[1] = r->y - d;
	l[2] = r->x + r->w + d - 1;
	l[3] = r->y + r->h + d - 1;
	v_bar(C.vh, l);
}

void p_bar(int d, short x, short y, short w, short h)	/* for perimeter = 1 */
{
	short l[10];
	x -= d, y -= d, w += d+d, h += d+d;
	l[0] = x+1;	/* only the inside */
	l[1] = y+1;
	l[2] = x+w-2;
	l[3] = y+h-2;
	v_bar(C.vh, l);
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y;
	l[4] = x+w-1;
	l[5] = y+h-1;
	l[6] = x;
	l[7] = y+h-1;
	l[8] = x;
	l[9] = y+1;		/* beware Xor mode :-) */
	v_pline(C.vh,5,l);
}

void p_gbar(int d, RECT *r)	/* for perimeter = 1 */
{
	short l[10],
	    x = r->x - d,
	    y = r->y - d,
	    w = r->w + d+d,
	    h = r->h + d+d;
	l[0] = x+1;	/* only the inside */
	l[1] = y+1;
	l[2] = x+w-2;
	l[3] = y+h-2;
	v_bar(C.vh, l);
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y;
	l[4] = x+w-1;
	l[5] = y+h-1;
	l[6] = x;
	l[7] = y+h-1;
	l[8] = x;
	l[9] = y+1;		/* beware Xor mode :-) */
	v_pline(C.vh,5,l);
}

void box(int d, short x, short y, short w, short h)
{
	short l[10];
	x -= d, y -= d, w += d+d, h += d+d;
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y;
	l[4] = x+w-1;
	l[5] = y+h-1;
	l[6] = x;
	l[7] = y+h-1;
	l[8] = x;
	l[9] = y+1;		/* for Xor mode :-) */
	v_pline(C.vh,5,l);
}

void gbox(int d, RECT *r)
{
	short l[10],
	    x = r->x - d,
	    y = r->y - d,
	    w = r->w + d+d,
	    h = r->h + d+d;
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y;
	l[4] = x+w-1;
	l[5] = y+h-1;
	l[6] = x;
	l[7] = y+h-1;
	l[8] = x;
	l[9] = y+1;		/* for Xor mode :-) */
	v_pline(C.vh,5,l);
}

#if NAES3D
#define PW (default_options.naes ? 1 : 0)
#else
#define PW 0
#endif

void tl_hook(int d, RECT *r, int col)
{
	short pnt[6],
	    x = r->x - d,
	    y = r->y - d,
	    w = r->w + d+d,
	    h = r->h + d+d;
	l_color(col);
	pnt[0] = x;
	pnt[1] = y + h - 1 - PW;
	pnt[2] = x;
	pnt[3] = y;
	pnt[4] = x + w - 1 - PW;
	pnt[5] = y;
	v_pline(C.vh, 3, pnt);
}

void br_hook(int d, RECT *r, int col)
{
	short pnt[6],
	    x = r->x - d,
	    y = r->y - d,
	    w = r->w + d+d,
	    h = r->h + d+d;
	l_color(col);
	pnt[0] = x + PW;
	pnt[1] = y + h - 1;
	pnt[2] = x + w - 1;
	pnt[3] = y + h - 1;
	pnt[4] = x + w - 1;
	pnt[5] = y + PW;
	v_pline(C.vh, 3, pnt);
}

void adjust_size(int d, RECT *r)
{
	r->x -= d;	/* positive value d means enlarge! :-)   as everywhere. */
	r->y -= d;
	r->w += d+d;
	r->h += d+d;
}

void chiseled_gbox(int d, RECT *r)
{
	br_hook(d,   r, screen.dial_colours.lit_col);
	tl_hook(d,   r, screen.dial_colours.shadow_col);
	br_hook(d-1, r, screen.dial_colours.shadow_col);
	tl_hook(d-1, r, screen.dial_colours.lit_col);
}

void t_extent(char *t, short *w, short *h)
{
	short e[8];
	vqt_extent(C.vh, t, e);

	*w = e[4] - e[6];	/* oberen rechten minus oberen linken x coord */
	*h = -(e[1] - e[7]);	/* unteren linken minus oberen linken y coord
				 * vqt_extent uses a true Cartesion coordinate system
				 * How about that? :-)
				 * A mathematicion lost among nerds!
				 */
}

void write_selection(int d, RECT *r)
{
	wr_mode(MD_XOR);
	f_color(BLACK);
	f_interior(FIS_SOLID);
	gbar(d, r);
	wr_mode(MD_TRANS);
}

void d3_pushbutton(int d, RECT *r, OBJC_COLOURS *col, int state, int thick, int mode)
{
	ushort selected = state&SELECTED;
	int t, j, outline;

	thick = -thick;		/* make thick same direction as d (positive value --> LARGER!) */

	if (thick > 0)		/* outside thickness */
		d += thick;
	d += 2;
	
	if (mode & 1)		/* fill ? */
	{
		if (col == NULL)
			f_color(screen.dial_colours.bg_col);
		/* otherwise set by set_colours() */
		/* inside bar */
		gbar(d, r);
	}

	j = d;
	t = abs(thick);
	outline = j;

#if NAES3D
	if (default_options.naes && !(mode&2))
	{	
		l_color(screen.dial_colours.fg_col);
		
		while (t > 0)
		{
			/* outside box */
			gbox(j, r);
			t--, j--;
		}
	
		br_hook(j, r, selected ? screen.dial_colours.lit_col : screen.dial_colours.shadow_col);
		tl_hook(j, r, selected ? screen.dial_colours.shadow_col : screen.dial_colours.lit_col);
	}
	else
#endif
	{
		do {
			br_hook(j, r, selected ? screen.dial_colours.lit_col : screen.dial_colours.shadow_col);
			tl_hook(j, r, selected ? screen.dial_colours.shadow_col : screen.dial_colours.lit_col);
			t--, j--;
		}
		while (t >= 0);

		/* full outline ? */
		if (thick && !(mode & 2))
		{
			l_color(screen.dial_colours.fg_col);
			/* outside box */
			gbox(outline, r);
		}
	}

	shadow_object(outline, state, r, screen.dial_colours.border_col, thick);

	l_color(screen.dial_colours.border_col);
}

/* HR 220401: strip leading and trailing spaces. */
void strip_name(char *to, char *fro)
{
	char *last = fro + strlen(fro) - 1;
	while (*fro && *fro == ' ')  fro++;
	if (*fro)
	{
		while (*last == ' ') last--;
		while (*fro &&  fro != last + 1) *to++ = *fro++;
	}
	*to = 0;
}

/* should become c:\s...ng\foo.bar */
void cramped_name(void *s, char *t, int w)
{
	char *q=s,*p=t, tus[256];
	int l, d, h;

	l = strlen(q);
	d = l - w;

	if (d > 0)
	{
		strip_name(tus, s);
		q = tus;
		l = strlen(tus);
		d = l - w;
	}

	if (d <= 0)
		strcpy(t,s);
	else
	{
		if (w < 12)		/* 8.3 */
			strcpy(t, q+d); /* only the last ch's */
		else
		{
			h = (w-3)/2;
			strncpy(p,q,h);
			p+=h;
			*p++='.';
			*p++='.';
			*p++='.';
			strcpy(p,q+l-h);
		}
	}
}

char *clipped_name(void *s, char *t, int w)
{
	char *q=s;
	int l = strlen(q);

	if (l > w)
	{
		strncpy(t,q,w);
		t[w]=0;
		return t;
	}
	return s;
}

/* HR: 1 (good) set of routines for screen saving */
inline long calc_back(RECT *r, int planes)
{
	return 2L * planes
		  * ((r->w + 15) / 16)
		  * r->h;
}

void *form_save(int d, RECT r, void *area)
{
	MFDB Mscreen={0};
	MFDB Mpreserve;
	short pnt[8];
	
	r.x-=d, r.y-=d, r.w+=2*d, r.h+=2*d;

	rtopxy(pnt, &r);
	ritopxy(pnt+4,0,0,r.w,r.h);

	DIAG((D_menu, NULL, "form_save %d/%d,%d/%d\n", r.x, r.y, r.w, r.h));

	Mpreserve.fd_w = r.w;
	Mpreserve.fd_h = r.h;
	Mpreserve.fd_wdwidth = (r.w + 15) / 16;
	Mpreserve.fd_nplanes = screen.planes;
	Mpreserve.fd_stand = 0;

	if (area == NULL)
		area = xmalloc(calc_back(&r,screen.planes),210);

	if (area)
	{
		Mpreserve.fd_addr = area;
		vro_cpyfm(C.vh, S_ONLY, pnt, &Mscreen, &Mpreserve);
	}
	return area;
}

void form_restore(int d, RECT r, void *area)
{
	MFDB Mscreen={0};
	MFDB Mpreserve;
	short pnt[8];
	
	if (!area)
		return;

	r.x-=d, r.y-=d, r.w+=2*d, r.h+=2*d;

	rtopxy(pnt+4, &r);
	ritopxy(pnt,0,0,r.w,r.h);

	DIAG((D_menu, NULL, "form_restore %d/%d,%d/%d\n", r.x, r.y, r.w, r.h));

	Mpreserve.fd_w = r.w;
	Mpreserve.fd_h = r.h;
	Mpreserve.fd_wdwidth = (r.w + 15) / 16;
	Mpreserve.fd_nplanes = screen.planes;
	Mpreserve.fd_stand = 0;
	Mpreserve.fd_addr = area;
	vro_cpyfm(C.vh, S_ONLY, pnt, &Mpreserve, &Mscreen);
	free(area);
}

void form_copy(RECT *fr, RECT *to)
{
	short pnt[8];
	MFDB Mscreen={0};
	rtopxy(pnt,fr);
	rtopxy(pnt+4,to);
	hidem();
	vro_cpyfm(C.vh, S_ONLY, pnt, &Mscreen, &Mscreen);
	showm();
}

void shadow_object(int d, int state, RECT *rp, int colour, int thick)
{
	RECT r = *rp;
	int offset, increase;

	/* Are we shadowing this object? (Borderless objects aren't shadowed!) */
	if (thick && (state & SHADOWED))
	{
		int i;

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
			br_hook(d, &r, colour);
		}
	}
}

static int menu_dis_col(XA_TREE *wt)		/* Get colours for disabled better. */
{
	int c = BLACK;
	if (!MONO)
		if (wt->is_menu)
		{
			OBJECT *ob = wt->tree + wt->current;
			if (ob->ob_state&DISABLED)
			{
				c = screen.dial_colours.shadow_col;
				done(DISABLED);
			}
		}
	return c;
}

static OBJC_COLOURS button_colours(void)
{
	OBJC_COLOURS c;
	c.borderc = screen.dial_colours.border_col;
	c.textc   = BLACK;
	c.opaque  = 1;
	c.pattern = IP_HOLLOW;
	c.fillc   = screen.dial_colours.bg_col;
	return c;
}

static void ob_text(XA_TREE *wt, RECT *r, RECT *o, OBJC_COLOURS *c, char *t, int state, int und)
{
	if (t && *t)
	{
		OBJECT *ob = wt->tree + wt->current;
		bool fits = !o || (o && (o->h >= r->h - (d3_foreground(ob) ? 4 : 0)));

		/* set according to circumstances. */
		if (c)
		{
			/* more restrictions	*/
			if (    c->opaque
			    && !MONO
			    && d3_any(ob)
			    && (     c->pattern == IP_HOLLOW
			         || (c->pattern == IP_SOLID && c->fillc == WHITE)))
			{
				f_color(screen.dial_colours.bg_col);
				wr_mode(MD_REPLACE);
				gbar(0, o ? o : r);
				wr_mode(MD_TRANS);
			}
			else
				wr_mode(c->opaque ? MD_REPLACE : MD_TRANS);
		}

		if (!MONO && (state&DISABLED) != 0)
		{
			done(DISABLED);
			if (fits)
			{
				t_color(screen.dial_colours.lit_col);
				v_gtext(C.vh, r->x+1, r->y+1, t);
				t_color(screen.dial_colours.shadow_col);
			}
		}

		if (fits)
			v_gtext(C.vh, r->x, r->y, t);

		/* Now underline the shortcut character, if any. */
		if (und >= 0)
		{
			int l = strlen(t);
			if (und < l)
			{
				short x = r->x + und*screen.c_max_w,
				    y = r->y + screen.c_max_h - 1;
				line(x, y, x + screen.c_max_w - 1, y, RED);
			}
		}
	}
}

static void g_text(XA_TREE *wt, RECT r, RECT *o, char *text, int state)
{
	/* HR: only center the text. ;-) */
	r.y += (r.h-screen.c_max_h) / 2;
	if (!MONO && (state&DISABLED))
	{
		t_color(screen.dial_colours.lit_col);
		v_gtext(C.vh, r.x+1, r.y+1, text);
		t_color(screen.dial_colours.shadow_col);
		v_gtext(C.vh, r.x,   r.y,   text);
		done(DISABLED);
	}
	else
	{
		t_color(menu_dis_col(wt));
		ob_text(wt, &r, o, NULL, text, 0, (state & WHITEBAK) ? (state >> 8) & 0x7f : -1);
		if (state & DISABLED)
		{
			write_disable(&wt->r, screen.dial_colours.bg_col);
			done(DISABLED);
		}
	}
}

/* HR 051002: This function doesnt change colourword anymore, but just sets the required color.
              Neither does it affect writing mode for text (this is handled in ob_text() */
static void
set_colours(OBJECT *ob, OBJC_COLOURS *colourword)
{
	wr_mode(MD_REPLACE);

	/* 2 */
	f_interior(FIS_PATTERN);

	if (colourword->pattern == IP_SOLID)
	{
		/* 2,8  solid fill  colour */
		f_style(8);
		f_color(colourword->fillc);
	}
	else
	{
		if (colourword->pattern == IP_HOLLOW)	
		{
			/* 2,8 solid fill  white */
			f_style(8);

			/* Object inherits default dialog background colour? */
			if ((colourword->fillc == 0) && d3_any(ob))
				f_color(screen.dial_colours.bg_col);
			else
				f_color(WHITE);

		}
		else
		{
			f_style(colourword->pattern);
			f_color(colourword->fillc);
		}
	}

#if SELECT_COLOR
	if (!MONO && (ob->ob_state & SELECTED))
	{
		/* Allow a different colour set for 3d push  */
		if (d3_any(ob))
			f_color(selected3D_colour[colourword->fillc]);
		else
			f_color(selected_colour[colourword->fillc]);
	}
#endif

	t_color(colourword->textc);
	l_color(colourword->borderc);
}

/*
 * Format a G_FTEXT type text string from its template,
 * and return the real position of the text cursor.
 */
/* HR: WRONG_LEN
   It is a very confusing here, edit_pos is always (=1) passed as tedinfo->te_tmplen.
   which means that this only works if that field is wrongly used as the text corsor position.
   A very bad thing, because it is supposed to be a constant describing the amount of memory
   allocated to the template string.
   
   28 jan 2001
   OK, edit_pos is not anymore the te_tmplen field.
*/

static int
format_dialog_text(char *text_out, char *template, char *text_in, int edit_pos)
{
	int index = 0, tpos = 0, max = strlen(template);
	/* HR: In case a template ends with '_' and the text is completely
	 * filled, edit_index was indeterminate. :-)
	 */
	int edit_index = max;
	bool aap = *text_in == '@';

	while (*template)
	{
		if (*template != '_')
		{
			*text_out++ = *template;
		}
		else
		{
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
	/* keep visible at end */
	if (edit_index >= max)
		edit_index--;

	return edit_index;
}

/* HR: implement wt->x,y */
/* HR: Modifications: set_text(...  &gr, &cr, temp_text ...)
 *     cursor alignment in centered text.
 *     Justification now integrated in set_text (v_gtext isolated).
 *     Preparations for positioning cursor in proportional fonts.
 *     Moved a few calls.
	pbox(hl,ext[0]+x,ext[1]+y,ext[4]+x-1,ext[5]+y-1);
 */
static void
set_text(OBJECT *ob,
	 RECT *gr,
	 RECT *cr,
	 bool formatted,
	 int edit_pos,
	 char *temp_text,
	 OBJC_COLOURS *colours,
	 int *thick,
	 RECT r)
{
	TEDINFO *ted = get_ob_spec(ob)->tedinfo;
	RECT cur;
	short w, h, cur_x = 0;

	*colours = ted->te_color;
	*thick = (char)ted->te_thickness;

	/* Set the correct text size & font */
	switch (ted->te_font)
	{
	case TE_GDOS_PROP:		/* Use a proportional SPEEDOGDOS font (AES4.1 style) */
	case TE_GDOS_MONO:		/* Use a monospaced SPEEDOGDOS font (AES4.1 style) */
	case TE_GDOS_BITM:		/* Use a GDOS bitmap font (AES4.1 style) */
		t_font(ted->te_fontsize, ted->te_fontid);
		cur.w = screen.c_max_w;
		cur.h = screen.c_max_h;
		break;
	case TE_STANDARD:		/* Use the standard system font (probably 10 point) */
		cur.w = screen.c_max_w;
		cur.h = screen.c_max_h;
		break;
	case TE_SMALL:			/* Use the small system font (probably 8 point) */
		t_font(screen.small_font_point, screen.small_font_id);
		cur.w = screen.c_min_w;
		cur.h = screen.c_min_h;
		break;
	}

	/* HR: use vqt_extent to obtain x, because it is almost impossible, or
	 *	at least very unnecessary tedious to position the cursor.
	 *	vst_alignment is not needed anymore.
	 */
	/* after vst_font & vst_point */

	if (formatted)
		cur_x = format_dialog_text(temp_text, ted->te_ptmplt, ted->te_ptext, edit_pos);
	else
		strncpy(temp_text, ted->te_ptext, 255);

	t_extent(temp_text, &w, &h);

	/* HR 290301 & 070202: Dont let a text violate its object space! (Twoinone packer shell!! :-) */
	if (w > r.w)
	{
		short rw  = r.w/cur.w,
		    dif = (w - r.w + cur.w - 1)/cur.w,
		    h1dif,h2dif;
		switch(ted->te_just)
		{
		case TE_LEFT:
			*(temp_text + rw) = 0;
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

		t_extent(temp_text, &w, &h);
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
		*cr = cur;
		cr->x += cur_x * cur.w;	/* non prop font only */
	}

	cur.w = w;
	cur.h = h;
	*gr = cur;
}

#if SELECT_COLOR
static const short
	/*                     0  1  2   3   4   5   6   7   8  9 10 11 12 13 14 15	*/
	selected_colour[]   = {1, 0, 13, 15, 14, 10, 12, 11, 8, 9, 5, 7, 6, 2, 4, 3},
	selected3D_colour[] = {1, 0, 13, 15, 14, 10, 12, 11, 9, 8, 5, 7, 6, 2, 4, 3};
#endif

/* HR: implement wt->x,y in all ObjectDisplay functions */
/* HR 290101: OBSPEC union & OBJC_COLOURS structure now fully implemented
 *            in xa_aes.h.
 *            Accordingly replaced ALL nasty casting & assemblyish approaches
 *            by simple straightforward C programming.
 */


/*
 * Draw a box (respecting 3d flags)
 */
void
d_g_box(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	OBJC_COLOURS colours;
	int thick;

	colours = get_ob_spec(ob)->this.colours;
	thick = thickness(ob);
	set_colours(ob, &colours);

	/* before borders */
	done(SELECTED);

	/* plain box is a tiny little bit special. :-) */
	if (d3_foreground(ob)
	    && !(wt->current == 0 && thick < 0))	/* root box with inside border */
	{
		d3_pushbutton( 0,
		               &r,
		               &colours,
		               ob->ob_state,
		               thick,
		               1);
	}
	else
	{
		/* display inside */
		gbar(0, &r);

		if (ob->ob_state & SELECTED)
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			if (!(wt->current == 0 && wt->zen))
			{
				g2d_box(thick, &r, colours.borderc);
				shadow_object(0, ob->ob_state, &r, colours.borderc, thick);
			}
		}
	}
}

/*
 * Draw a plain hollow ibox
 */
void
d_g_ibox(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	OBJC_COLOURS colours;
	int thick;

	colours = get_ob_spec(ob)->this.colours;
	thick = thickness(ob);
	set_colours(ob, &colours);

	/* before borders */
	done(SELECTED|DISABLED);

#if NAES3D
	if (default_options.naes && thick < 0)
		gbox(d3_foreground(ob) ? 2 : 0, &r);
#endif

	/* plain box is a tiny little bit special. :-) */
	if (d3_foreground(ob)
	    && !(wt->current == 0 && thick < 0)) /* root box with inside border */
	{
		d3_pushbutton( 0,
		               &r,
		               &colours,
		               ob->ob_state,
		               thick,
		               0);
	}
	else
	{
		if (ob->ob_state & SELECTED)
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			if (!(wt->current == 0 && wt->zen))
			{
				g2d_box(thick, &r, colours.borderc);
				shadow_object(0, ob->ob_state, &r, colours.borderc, thick);
			}
		}
	}
}

/*
 * Display a boxchar (respecting 3d flags)
 */
void
d_g_boxchar(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r, gr = r;
	OBJECT *ob = wt->tree + wt->current;
	OBJC_COLOURS colours;
	ushort selected = ob->ob_state & SELECTED;
	int thick;
	char temp_text[2];

	colours = get_ob_spec(ob)->this.colours;
	
	temp_text[0] = get_ob_spec(ob)->this.character;
	temp_text[1] = '\0';

	thick = thickness(ob);

	/* leaves MD_REPLACE */
	set_colours(ob, &colours);

	/* Centre the text in the box */
	gr.x = r.x + ((r.w - screen.c_max_w) / 2);
	gr.y = r.y + ((r.h - screen.c_max_h) / 2);
	gr.w = screen.c_max_w;
	gr.h = screen.c_max_h;

	if (d3_foreground(ob))
	{
		d3_pushbutton(0, &r, &colours, ob->ob_state, thick, 1);
		if (ob->ob_state & SELECTED)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}
		wr_mode(colours.opaque ? MD_REPLACE : MD_TRANS);
		ob_text(wt, &gr, &r, NULL, temp_text, 0, -1);
	}
	else
	{
		gbar(0, &r);
		ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

		if (selected)
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			g2d_box(thick, &r, colours.borderc);
			shadow_object(0, ob->ob_state, &r, colours.borderc, thick);
		}
	}

	done(SELECTED);
}

/*
 * Draw a boxtext object
 */
void
d_g_boxtext(LOCK lock, struct widget_tree *wt)
{
	int thick = 0;
	ushort selected;
	RECT r = wt->r, gr;
	OBJECT *ob = wt->tree + wt->current;
	OBJC_COLOURS colours;
	char temp_text[256];

	selected = ob->ob_state & SELECTED;

	set_text(ob, &gr, NULL, false, -1, temp_text, &colours, &thick, r);
	set_colours(ob, &colours);

	if (d3_foreground(ob))		/* indicator or avtivator */
	{
		d3_pushbutton(0, &r, &colours, ob->ob_state, thick, 1);

		if (selected)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}
		ob_text(wt, &gr, &r, &colours, temp_text, 0, -1);
	} else
	{
		gbar(0, &r);
		ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

		if (selected)
			write_selection(0, &r);		/* before border */

		if (thick)	/* Display a border? */
		{
			wr_mode(MD_REPLACE);
			g2d_box(thick, &r, colours.borderc);
			shadow_object(0, ob->ob_state, &r, colours.borderc, thick);
		}
	}

	t_font(screen.standard_font_point, screen.standard_font_id);
	done(SELECTED);
}


void
d_g_fboxtext(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	RECT gr,cr;
	OBJC_COLOURS colours;
	bool is_edit = wt->current == wt->edit_obj;
	ushort selected=ob->ob_state & SELECTED;
	int thick;
	char temp_text[256];

	set_text(ob, &gr, &cr, true, is_edit ? wt->edit_pos : -1, temp_text, &colours, &thick, r);
	set_colours(ob, &colours);

	if (d3_foreground(ob))
	{
		d3_pushbutton(0, &r, &colours, ob->ob_state, thick, 1);
		if (selected)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}

		ob_text(wt, &gr, &r, &colours, temp_text, 0, -1);
	}
	else
	{
		gbar(0, &r);				
		ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

		if (selected)
			/* before border */
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			wr_mode(MD_REPLACE);
			g2d_box(thick, &r, colours.borderc);
			shadow_object(0, ob->ob_state, &r, colours.borderc, thick);
		}
	}


	if (is_edit)
		/* write cursor */
		write_selection(0, &cr);

	t_font(screen.standard_font_point, screen.standard_font_id);
	done(SELECTED);
}

/*
 * Draw a button object
 */

void
d_g_button(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r, gr = r;
	OBJECT *ob = wt->tree + wt->current;
	OBJC_COLOURS colours;
	int thick = thickness(ob); 
	ushort selected = ob->ob_state & SELECTED;
	char *text = get_ob_spec(ob)->string;

	colours = button_colours();

	t_color(BLACK);

	if ((ob->ob_state & WHITEBAK) && (ob->ob_state & 0x8000))
	{
		short und = (short)ob->ob_state >> 8;
		wr_mode(MD_REPLACE);
		/* group frame */
		if (und == -2)
		{
			RECT rr = r;
			rr.y += screen.c_max_h/2;
			rr.h -= screen.c_max_h/2;
			if (MONO || !d3_any(ob))
			{
				l_color(BLACK);
				gbox(0, &rr);
			}
			else
				chiseled_gbox(0, &rr);
			gr.x = r.x + screen.c_max_w;
			gr.y = r.y;
			t_extent(text, &gr.w, &gr.h);
			ob_text(wt, &gr, NULL, &colours, text, 0, -1);
		}
		else
		{
			XA_TREE b;
			b.tree = get_widgets();
			display_object(	lock, &b,
					  (ob->ob_flags & RBUTTON)
					? (selected ? RADIO_SLCT : RADIO_DESLCT )
					: (selected ? BUT_SLCT   : BUT_DESLCT   ),
					gr.x, gr.y, 11);
			gr.x += ICON_W;
			gr.x += screen.c_max_w;
			wr_mode(MD_TRANS);
			ob_text(wt, &gr, &r, NULL, text, 0, und & 0x7f);
		}
	}
	else
	{
		short und, tw, th;

		und = (ob->ob_state & WHITEBAK) ? (ob->ob_state >> 8) & 0x7f : -1;

		/* Use extent, NOT vst_alinment. Makes x and y to v_gtext
		 * real values that can be used for other stuff (like shortcut
		 * underlines :-)
		 */

		t_extent(text, &tw, &th);
		gr.y += (r.h - th) / 2;
		gr.x += (r.w - tw) / 2;

		if (d3_foreground(ob))
		{
			d3_pushbutton(0, &r, NULL, ob->ob_state, thick, 1);
			if (selected)
			{
				gr.x += PUSH3D_DISTANCE;
				gr.y += PUSH3D_DISTANCE;
			}
	
			wr_mode(MD_TRANS);
			ob_text(wt, &gr, &r, NULL, text, 0, und);
		}
		else
		{
			wr_mode(MD_REPLACE);
			f_interior(FIS_SOLID);
			f_color(selected ? BLACK : WHITE);

			/* display inside bar */		
			gbar(-thick, &r);

			wr_mode(MD_TRANS);
			t_color(selected ? WHITE : BLACK);
			ob_text(wt, &gr, &r, NULL, text, 0, und);

			/* Display a border? */
			if (thick)
			{
				g2d_box(thick, &r, BLACK);
				shadow_object(0, ob->ob_state, &r, colours.borderc, thick);
			}
		}
	}
	done(SELECTED);
}

static void
icon_characters(ICONBLK *iconblk, int state, short obx, short oby, short icx, short icy)
{
	char lc = iconblk->ib_char;
	short tx,ty,pnt[4];

	wr_mode(MD_REPLACE);
	ritopxy(pnt, obx + iconblk->tx.x, oby + iconblk->tx.y, iconblk->tx.w, iconblk->tx.h);

	t_font(screen.small_font_point, screen.small_font_id);

	/* center the text in a bar given by iconblk->tx, relative to object */
	t_color(BLACK);
	if (   iconblk->ib_ptext
	    && *iconblk->ib_ptext
	    && iconblk->tx.w
	    && iconblk->tx.h)
	{
		f_color((state&SELECTED) ? BLACK : WHITE);
		f_interior(FIS_SOLID);
		v_bar(C.vh,pnt);
	
		tx = obx + iconblk->tx.x + ((iconblk->tx.w - strlen(iconblk->ib_ptext)*6) / 2);	
		ty = oby + iconblk->tx.y + ((iconblk->tx.h - 6) / 2);

		if (state & SELECTED)
			wr_mode(MD_XOR);
		if (state & DISABLED)
			t_effect(FAINT);

		v_gtext(C.vh, tx, ty, iconblk->ib_ptext);
	}

	if (lc != 0 && lc != ' ')
	{
		char ch[2];
		ch[0] = lc;
		ch[1] = 0;
		v_gtext(C.vh, icx + iconblk->ib_xchar, icy + iconblk->ib_ychar, ch);
		/* Seemingly the ch is supposed to be relative to the image */
	}

	if (state & SELECTED)
		f_color(WHITE);

	t_font(screen.standard_font_point, screen.standard_font_id);
	t_effect(0);
	wr_mode( MD_TRANS);
}

/*
 * Draw a image
 */
void
d_g_image(LOCK lock, struct widget_tree *wt)
{
	OBJECT *ob = wt->tree + wt->current;
	BITBLK *bitblk;
	MFDB Mscreen;
	MFDB Micon;
	short pxy[8], cols[2], icx, icy;

	bitblk = get_ob_spec(ob)->bitblk;

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

	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);
}

/* HR 060202: all icons: handle disabled: make icon rectangle and text faint in stead
 *                                        of object rectangle.
 */

/*
 * Draw a mono icon
 */
void
d_g_icon(LOCK lock, struct widget_tree *wt)
{
	OBJECT *ob = wt->tree + wt->current;
	ICONBLK *iconblk;
	MFDB Mscreen;
	MFDB Micon;
	RECT ic;
	short pxy[8], cols[2], obx, oby, msk_col, icn_col;
	
	iconblk = get_ob_spec(ob)->iconblk;
	obx = wt->r.x;
	oby = wt->r.y;

	ic = iconblk->ic;
	ic.x += obx;
	ic.y += oby;

	ritopxy(pxy,     0, 0, ic.w, ic.h);
	rtopxy (pxy + 4, &ic);

	Micon.fd_w = ic.w;
	Micon.fd_h = ic.h;
	Micon.fd_wdwidth = (ic.w + 15) / 16;
	Micon.fd_nplanes = 1;
	Micon.fd_stand = 0;
	Mscreen.fd_addr = NULL;
			
	Micon.fd_addr = iconblk->ib_pmask;
	if (ob->ob_state & SELECTED)
	{
		icn_col = ((iconblk->ib_char) >> 8) & 0xf;
		msk_col = ((iconblk->ib_char) >> 12) & 0xf;
	}
	else
	{
		icn_col = ((iconblk->ib_char) >> 12) & 0xf;
		msk_col = ((iconblk->ib_char) >> 8) & 0xf;
	}
		
	cols[0] = msk_col;
	cols[1] = icn_col;
	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);
			
	Micon.fd_addr = iconblk->ib_pdata;
	cols[0] = icn_col;
	cols[1] = msk_col;
	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);

	if (ob->ob_state & DISABLED)
		write_disable(&ic, WHITE);

	/* should be the same for color & mono */
	icon_characters(iconblk, ob->ob_state & (SELECTED|DISABLED), obx, oby, ic.x, ic.y);

	done(SELECTED|DISABLED);
}

/*
 * Draw a colour icon
 */
void
d_g_cicon(LOCK lock, struct widget_tree *wt)
{
	OBJECT *ob = wt->tree + wt->current;
	ICONBLK *iconblk;
	CICONBLK *ciconblk;
	CICON	*c, *best_cicon;
	MFDB Mscreen;
	MFDB Micon;
	bool have_sel;
	RECT ic;
	short pxy[8], cols[2] = {0,1}, obx, oby, blitmode;

	ciconblk = get_ob_spec(ob)->ciconblk;
	best_cicon = NULL;
	

	c = ciconblk->mainlist;
	while (c)
	{
		if (c->num_planes <= screen.planes
		    && (!best_cicon || (best_cicon && c->num_planes > best_cicon->num_planes)))
		{
			best_cicon = c;
		}

		c = c->next_res;	
	}

	/* No matching icon, so use the mono one instead */
	if (!best_cicon)
	{
		d_g_icon(lock, wt);
		return;
	}

	c = best_cicon;

	iconblk = get_ob_spec(ob)->iconblk;
	obx = wt->r.x;
	oby = wt->r.y;

	ic = iconblk->ic;

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

	have_sel = c->sel_data != NULL;

	/* check existence of selection. */			
	if ((ob->ob_state & SELECTED) && have_sel)
		Micon.fd_addr = c->sel_mask;
	else
		Micon.fd_addr = c->col_mask;

	blitmode = screen.planes > 8 ? S_AND_D : S_OR_D;

	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);

	if ((ob->ob_state & SELECTED) && have_sel)
		Micon.fd_addr = c->sel_data;
	else
		Micon.fd_addr = c->col_data;

	Micon.fd_nplanes = screen.planes;
	vro_cpyfm(C.vh, blitmode, pxy, &Micon, &Mscreen);

	if ((ob->ob_state & SELECTED) && !have_sel)
	{
		Micon.fd_addr = c->col_mask;
		Micon.fd_nplanes = 1;
		vrt_cpyfm(C.vh, MD_XOR, pxy, &Micon, &Mscreen, cols);
	}

	if (ob->ob_state & DISABLED)
		write_disable(&ic, WHITE);

	icon_characters(iconblk, ob->ob_state & (SELECTED|DISABLED), obx, oby, ic.x, ic.y);

	done(SELECTED|DISABLED);
}

/*
 * Draw a text object
 */

void
d_g_text(LOCK lock, struct widget_tree *wt)
{
	int thick,thin;
	OBJECT *ob = wt->tree + wt->current;
	RECT r = wt->r, gr;
	OBJC_COLOURS colours;
	char temp_text[256];

	set_text(ob, &gr, NULL, false, -1, temp_text, &colours, &thick, r);
	set_colours(ob, &colours);
	thin = thick > 0 ? thick-1 : thick+1;

	if (d3_foreground(ob))
	{
		d3_pushbutton(thick > 0 ? -thick : 0, &r, &colours, ob->ob_state, thin, 3);
		done(SELECTED);
	}

	ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

	t_font(screen.standard_font_point, screen.standard_font_id);
}

void
d_g_ftext(LOCK lock, struct widget_tree *wt)
{
	int thick,thin;
	OBJECT *ob = wt->tree + wt->current;
	RECT r = wt->r, gr, cr;
	OBJC_COLOURS colours;
	bool is_edit = wt->current == wt->edit_obj;
	char temp_text[256];

	set_text(ob, &gr, &cr, true, is_edit ? wt->edit_pos : -1, temp_text, &colours, &thick, r);
	set_colours(ob, &colours);
	thin = thick > 0 ? thick-1 : thick+1;

	if (d3_foreground(ob))
	{
		d3_pushbutton(thick > 0 ? -thick : 0, &r, &colours, ob->ob_state, thin, 3);
		done(SELECTED);
	}

	/* unwrite cursor */
	if (is_edit && !colours.opaque)
		write_selection(0, &cr);

	ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

	if (is_edit)
		/* write cursor */
		write_selection(0, &cr);

	t_font(screen.standard_font_point, screen.standard_font_id);
}

#if PUREC
/* saves registerized local variables */
int call_pdef(void *, PARMBLK *);
#endif

#ifdef LATTICE
int call_pdef(int (*_cdecl)(PARMBLK *), PARMBLK *);
#pragma inline d0 = call_pdef(a1,) {register d2, a2; "4e91";}
#endif

void
d_g_progdef(LOCK lock, struct widget_tree *wt)
{
	OBJECT *ob = wt->tree + wt->current;
	APPLBLK *ab;
	PARMBLK p;
	long old_ssp = 0;
	short smd = 0;

	if (cfg.superprogdef)
		smd = (short)Super(1L);

	ab = get_ob_spec(ob)->appblk;
	p.pb_tree = wt->tree;
	p.pb_obj = wt->current;

	p.pb_prevstate = p.pb_currstate = ob->ob_state;
				
	p.r = wt->r;

	p.c.x = C.global_clip[0];
	p.c.y = C.global_clip[1];
	p.c.w = C.global_clip[2] - C.global_clip[0] + 1;
	p.c.h = C.global_clip[3] - C.global_clip[1] + 1;

	p.pb_parm = ab->ab_parm;

	/* ++cg[25/2/97]: We run PROGDEF's in supervisor mode because
	 * Lattice C 5.52's progdef-in-menu type submenus will cause
	 * a privilege violation if we don't
	 */

	wr_mode(MD_TRANS);

#if GENERATE_DIAGS
	{
		char statestr[128];
		extern char *pstates[];

		show_bits(*wt->state_mask&ob->ob_state, "state ", pstates, statestr);
		DIAG((D_o, wt->owner, "progdef before %s %04x\n", statestr, *wt->state_mask&ob->ob_state));
	}
#endif
	if (cfg.superprogdef)
	{
		if (!smd)
			/* Enter Supervisor mode*/
			old_ssp = Super(0);
	}

	/* The PROGDEF function returns the ob_state bits that
	 * remain to be handled by the AES:
	 */

#ifdef __GNUC__
	*wt->state_mask = (ushort)(*(ab->ab_code))(&p);
#else
	*wt->state_mask = (ushort)call_pdef(ab->ab_code, &p);
#endif

	if (cfg.superprogdef)
	{
		if (!smd)
			/* Back to User Mode */
			Super(old_ssp);
	}

	/* BUG: SELECTED bit only handled in non-color mode!!!
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
		extern char *pstates[];

		show_bits(*wt->state_mask, "state ", pstates, statestr);
		DIAG((D_o, wt->owner, "progdef remaining %s %04x\n", statestr, *wt->state_mask));
	}
#endif

	if (*wt->state_mask&DISABLED)
	{
		write_disable(&wt->r, screen.dial_colours.bg_col);
		done(DISABLED);
	}

	wr_mode(MD_REPLACE);
}

static void
l_text(short x, short y, char *t, short w, int left)
{
	char ct[256];
	int l = strlen(t);
	w /= screen.c_max_w;
	if (left < l)
	{
		strcpy(ct, t + left);
		ct[w]=0;
		v_gtext(C.vh, x, y, ct);
	}
}

static void
display_list_element(LOCK lock, SCROLL_ENTRY *this, int left, short x, short y, short w, int sel)
{
	XA_TREE tr = nil_tree;
	short xt = x + ICON_W;

	if (this)
	{
		f_color(sel ? BLACK : WHITE);
		bar(0, xt, y, w - ICON_W, screen.c_max_h);
		if (sel)
		{
			t_color(WHITE);
			l_text(xt, y, this->text, w - ICON_W, left);
			t_color(BLACK);
		} else
			l_text(xt, y, this->text, w - ICON_W, left);
	
		f_color(WHITE);
		bar(0, x, y, ICON_W, screen.c_max_h);

		if (this->icon)
		{
			if (sel)
				this->icon->ob_state |= SELECTED;
			else
				this->icon->ob_state &= ~SELECTED;
			tr.tree = this->icon;
			tr.owner = C.Aes;
			display_object(lock, &tr, 0, x, y, 12);
		}
	}
	else /* filler line */
	{
		f_color(WHITE);
		bar(0, x, y, w, screen.c_max_h);
	}
}

void
d_g_slist(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r, wa;
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	XA_WINDOW *w;
	short y, maxy;
	OBJECT *ob = wt->tree + wt->current;

	list = get_ob_spec(ob)->listbox;
	w = list->wi;

	w->r.x = r.x;
	w->r.y = r.y;
	/* for after moving */
	calc_work_area(w);

	wa = w->wa;
	y = wa.y;
	maxy = wa.y + wa.h - screen.c_max_h;
	this = list->top;

	t_color(BLACK);

	if (list->state == 0)
	{
		get_widget(w, XAW_TITLE)->stuff = list->title;
		if (MONO)
			l_color(screen.dial_colours.border_col),
			gbox(2,&r);
		else
			br_hook(2,&r,screen.dial_colours.shadow_col),
			tl_hook(2,&r,screen.dial_colours.lit_col);
		display_window(list->lock, 73, w, NULL);
		for (; y <= maxy; y += screen.c_max_h)
		{
			/* can handle nil this */
			display_list_element(lock, this, list->left, wa.x, y, wa.w, this == list->cur);
			if (this)
			{
				list->bot = this;
				this = this->next;
			}
		}
	}
	else /* arrives only here if there are more elements than fit the window */
	{
		RECT fr = wa, to;
		fr.h -= screen.c_max_h;
		to = fr;
		if (list->state == SCRLSTAT_UP)
		{
			to.y += screen.c_max_h;
			form_copy(&fr,&to);
			list->bot = list->bot->prev;
		}
		else if (list->state == SCRLSTAT_DOWN)
		{				
			fr.y += screen.c_max_h;
			form_copy(&fr,&to);
			for (; this && y < maxy; this = this->next, y += screen.c_max_h);
			list->bot = this;
		}
		/* scroll list windows are not on top, but are visible! */
		display_vslide(list->lock, w, get_widget(w, XAW_VSLIDE) );
		display_list_element(lock, this, list->left, wa.x, y, wa.w, this == list->cur);
		list->state = 0;
	}
	done(SELECTED);
}

void
d_g_string(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	ushort state = ob->ob_state;
	char *text = get_ob_spec(ob)->string;

	/* most AES's allow null string */
	if (text)
	{
		bool tit =    ( state & WHITEBAK )
		           && ( state & 0xff00   ) == 0xff00;

		wr_mode(MD_TRANS);

		if (d3_foreground(ob) && !tit)
			d3_pushbutton(0, &r, NULL, state, 0, 0);

		if (   wt->is_menu
		    && (ob->ob_state & DISABLED)
		    && *text == '-')
		{
			r.x += 1, r.w -= 3;
			r.y += (r.h - 2)/2;
			if (MONO)
			{
				vsl_type(C.vh, 7);
				vsl_udsty(C.vh, 0xaaaa);
				line(r.x, r.y, r.x + r.w, r.y, BLACK);
				vsl_udsty(C.vh, 0x5555);
				line(r.x, r.y + 1, r.x + r.w, r.y + 1, BLACK);
				vsl_type(C.vh, 0);
			}
			else
			{
				r.x += 2, r.w -= 4;
				line(r.x, r.y,     r.x + r.w, r.y,     screen.dial_colours.fg_col);
				line(r.x, r.y + 1, r.x + r.w, r.y + 1, screen.dial_colours.lit_col);
				l_color(BLACK);
			}
			done(DISABLED);
		}
		else
			g_text(wt, r, &wt->r, text, ob->ob_state);

		if (tit)
			line(r.x, r.y + r.h, r.x + r.w -1, r.y + r.h, BLACK);
	}
}

void
d_g_title(LOCK lock, struct widget_tree *wt)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	char *text = get_ob_spec(ob)->string;

	wr_mode( MD_TRANS);

	/* menu in user window.*/
	if (!wt->menu_line)
		d3_pushbutton(-2, &r, NULL, ob->ob_state, 1, MONO ? 1 : 3);

	/* most AES's allow null string */
	if (text)
		g_text(wt, r, &wt->r, text, ob->ob_state);

	if (ob->ob_state & SELECTED && wt->menu_line)
		/* very special!!! */
		write_selection(-1, &r);

	done(SELECTED);
}
