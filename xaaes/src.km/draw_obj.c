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

#include WIDGHNAME


#include "xa_types.h"
#include "xa_global.h"

#include "draw_obj.h"
#include "obtree.h"

#include "rectlist.h"
#include "c_window.h"
#include "widgets.h"
#include "scrlobjc.h"
#include "xa_user_things.h"

#include "mint/signal.h"

#define done(x) (*wt->state_mask &= ~(x))


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
	"xaaes slist",
	"popup",
	"resv",
	"edit",
	"shortcut",
	"39",
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
			TEDINFO *ted = object_get_spec(tree + t)->tedinfo;
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
#endif

bool inline d3_any(OBJECT *ob)        { return (ob->ob_flags & OF_FL3DACT) != 0;	}
bool inline d3_indicator(OBJECT *ob)  { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DIND; }
bool inline d3_foreground(OBJECT *ob) { return (ob->ob_flags & OF_FL3DIND) != 0; }
bool inline d3_background(OBJECT *ob) { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DBAK; }
bool inline d3_activator(OBJECT *ob)  { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DACT; }

static void
g2d_box(short b, RECT *r, short colour)
{
	/* inside runs from 3 to 0 */
	if (b > 0)
	{
		if (b >  4) b =  3;
		else        b--;
		l_color(colour);
		while (b >= 0)
			gbox(-b, r), b--;
	}
	/* outside runs from 4 to 1 */
	else if (b < 0)
	{
		if (b < -4) b = -4;
		l_color(colour);
		while (b < 0)
			gbox(-b, r), b++;
	}
}

void
draw_2d_box(short x, short y, short w, short h, short border_thick, short colour)
{
	RECT r;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	g2d_box(border_thick, &r, colour);
}


/*
 * Set clipping to entire screen
 */
void
clear_clip(void)
{
	rtopxy(C.global_clip, &screen.r);
	vs_clip(C.vh, 1, C.global_clip);
}
void
restore_clip(RECT *s)
{
	rtopxy(C.global_clip, s);
	vs_clip(C.vh, 1, C.global_clip);
}
void
save_clip(RECT *s)
{
	*s = *(RECT *)&C.global_clip;
}

void
set_clip(const RECT *r)
{
	if (r->w <= 0 || r->h <= 0)
		rtopxy(C.global_clip, &screen.r);
	else
		rtopxy(C.global_clip, r);

	vs_clip(C.vh, 1, C.global_clip);
}

void
write_disable(RECT *r, short colour)
{
	static short pattern[16] =
	{
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa
	};

	wr_mode(MD_TRANS);
	f_color(colour);
	vsf_udpat(C.vh, pattern, 1);
	f_interior(FIS_USER);
	gbar(0, r);
}

void
wr_mode(short m)
{
	static short mode = -1;

	if (mode != m)
	{
		mode = m;
		vswr_mode(C.vh, mode);
	}
}

void
l_color(short m)
{
	static short mode = -1;

	if (mode != m)
	{
		mode = m;
		vsl_color(C.vh, mode);
	}
}

void
f_color(short m)
{
	static short mode = -1;

	if (mode != m)
	{
		mode = m;
		vsf_color(C.vh, mode);
	}
}

void
t_color(short m)
{
	static short mode = -1;

	if (mode != m)
	{
		mode = m;
		vst_color(C.vh, mode);
	}
}

void t_effect(short m)
{
	static short mode = -1;

	if (mode != m)
	{
		mode = m;
		vst_effects(C.vh, mode);
	}
}

void
t_font(short p, short f)
{
	static short pm = -1;
	static short fm = -1;
	short temp;


	if (fm != f)
	{
		fm = f;
		vst_font(C.vh, f);
		f = 0;
	}
	else
		f = 1;

	/*
	 * Ozk: Hmm... have to reset point size when changing fonts!!??
	 */
	if (!f || pm != p)
	{
		pm = p;
		vst_point(C.vh, p, &temp, &temp, &temp, &temp);
	}
}

void f_interior(short m)
{
	static short mode = -1;

	if (mode != m)
	{
		mode = m;
		vsf_interior(C.vh, mode);
	}
}

void f_style(short m)
{
	static short mode = -1;
	if (m != mode)
		vsf_style(C.vh, mode = m);
}

/* HR: pxy wrapper functions (Beware the (in)famous -1 bug */
/* Also vsf_perimeter only works in XOR writing mode */

void write_menu_line(RECT *cl)
{
	short pnt[4];
	l_color(G_BLACK);

	pnt[0] = cl->x;
	pnt[1] = cl->y + cl->h - 1;
	pnt[2] = cl->x + cl->w - 1;
	pnt[3] = pnt[1];
	v_pline(C.vh, 2, pnt);
#if 0
	pnt[0] = cl->x;
	pnt[1] = cl->y + MENU_H;
	pnt[2] = cl->x + cl->w - 1;
	pnt[3] = pnt[1];
	v_pline(C.vh,2,pnt);
#endif
}

void
r2pxy(short *p, short d, const RECT *r)
{
	*p++ = r->x - d;
	*p++ = r->y - d;
	*p++ = r->x + r->w + d - 1;
	*p   = r->y + r->h + d - 1;
}

void rtopxy(short *p, const RECT *r)
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
	
void ritopxy(short *p, short x, short y, short w, short h)
{
	*p++ = x;
	*p++ = y;
	*p++ = x + w - 1; 
	*p   = y + h - 1;
}

void line(short x, short y, short x1, short y1, short col)
{
	short pxy[4] = { x, y, x1, y1 };

	l_color(col);
	v_pline(C.vh, 2, pxy);
}


void bar(short d,  short x, short y, short w, short h)
{
	short l[4];
	x -= d, y -= d, w += d+d, h += d+d;
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y+h-1;
	v_bar(C.vh, l);
}

void gbar(short d, const RECT *r)		/* for perimeter = 0 */
{
	short l[4];
	l[0] = r->x - d;
	l[1] = r->y - d;
	l[2] = r->x + r->w + d - 1;
	l[3] = r->y + r->h + d - 1;
	v_bar(C.vh, l);
}

void p_bar(short d, short x, short y, short w, short h) /* for perimeter = 1 */
{
	short l[10];
	x -= d, y -= d, w += d+d, h += d+d;
	l[0] = x+1;			/* only the inside */
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
	l[9] = y+1;			/* beware Xor mode :-) */
	v_pline(C.vh,5,l);
}

void p_gbar(short d, const RECT *r)	/* for perimeter = 1 */
{
	short l[10];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;
	l[0] = x+1;			/* only the inside */
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
	l[9] = y+1;			/* beware Xor mode :-) */
	v_pline(C.vh,5,l);
}

void box(short d, short x, short y, short w, short h)
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
	l[9] = y+1;			/* for Xor mode :-) */
	v_pline(C.vh,5,l);
}

void gbox(short d, const RECT *r)
{
	short l[10];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y;
	l[4] = x+w-1;
	l[5] = y+h-1;
	l[6] = x;
	l[7] = y+h-1;
	l[8] = x;
	l[9] = y+1;			/* for Xor mode :-) */
	v_pline(C.vh,5,l);
}

#if NAES3D
#define PW (default_options.naes ? 1 : 0)
#else
#define PW 0
#endif

void
top_line(short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;

	l_color(col);
	pnt[0] = x;
	pnt[1] = y;
	pnt[2] = x + w - 1;
	pnt[3] = y;
	v_pline(C.vh, 2, pnt);
}
void
bottom_line(short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;

	l_color(col);
	pnt[0] = x;
	pnt[1] = y + h - 1;
	pnt[2] = x + w - 1;;
	pnt[3] = pnt[1];
	v_pline(C.vh, 2, pnt);
}
void
left_line(short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short h = r->h + d+d;

	l_color(col);
	pnt[0] = x;
	pnt[1] = y;
	pnt[2] = x;
	pnt[3] = y + h - 1;
	v_pline(C.vh, 2, pnt);
}
void
right_line(short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;

	l_color(col);
	pnt[0] = x + w - 1;
	pnt[1] = y;
	pnt[2] = pnt[0];
	pnt[3] = y + h - 1;
	v_pline(C.vh, 2, pnt);
}

void tl_hook(short d, const RECT *r, short col)
{
	short pnt[6];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;
	l_color(col);
	pnt[0] = x;
	pnt[1] = y + h - 1 - PW;
	pnt[2] = x;
	pnt[3] = y;
	pnt[4] = x + w - 1 - PW;
	pnt[5] = y;
	v_pline(C.vh, 3, pnt);
}

void br_hook(short d, const RECT *r, short col)
{
	short pnt[6];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;
	l_color(col);
	pnt[0] = x + PW;
	pnt[1] = y + h - 1;
	pnt[2] = x + w - 1;
	pnt[3] = y + h - 1;
	pnt[4] = x + w - 1;
	pnt[5] = y + PW;
	v_pline(C.vh, 3, pnt);
}

void adjust_size(short d, RECT *r)
{
	r->x -= d;	/* positive value d means enlarge! :-)   as everywhere. */
	r->y -= d;
	r->w += d+d;
	r->h += d+d;
}

void chiseled_gbox(short d, const RECT *r)
{
	br_hook(d,   r, screen.dial_colours.lit_col);
	tl_hook(d,   r, screen.dial_colours.shadow_col);
	br_hook(d-1, r, screen.dial_colours.shadow_col);
	tl_hook(d-1, r, screen.dial_colours.lit_col);
}

void t_extent(const char *t, short *w, short *h)
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

void
text_extent(const char *t, struct xa_fnt_info *f, short *w, short *h)
{
	t_font(f->p, f->f);
	vst_effects(C.vh, f->e);
	t_extent(t, w, h);
	vst_effects(C.vh, 0);
}

void
wtxt_output(struct xa_wtxt_inf *wtxti, char *txt, short state, const RECT *r, short xoff, short yoff)
{
	struct xa_fnt_info *wtxt;
	bool sel = state & OS_SELECTED;
	short x, y, f = wtxti->flags;
	char t[200];

	if (sel)
		wtxt = &wtxti->s;
	else
		wtxt = &wtxti->n;

	wr_mode(MD_TRANS);
	t_font(wtxt->p, wtxt->f);

	vst_effects(C.vh, wtxt->e);

	if (f & WTXT_NOCLIP)
		strncpy(t, txt, sizeof(t));
	else
		prop_clipped_name(txt, t, r->w - (xoff << 1), &x, &y, 1);
	
	t_extent(t, &x, &y);
	y = yoff + r->y + ((r->h - y) >> 1);
	
	if (f & WTXT_CENTER)
		x = xoff + r->x + ((r->w - x) >> 1);
	else
		x = xoff + r->x;

	if (f & WTXT_DRAW3D)
	{
		if (sel && (f & WTXT_ACT3D))
			x++, y++;
	
		t_color(wtxt->bgc);
		x++;
		y++;
		v_gtext(C.vh, x, y, t);
		x--;
		y--;
	}
	t_color(wtxt->fgc);
	v_gtext(C.vh, x, y, t);

	/* normal */
	vst_effects(C.vh, 0);
}
	
void write_selection(short d, RECT *r)
{
	wr_mode(MD_XOR);
	f_color(G_BLACK);
	f_interior(FIS_SOLID);
	gbar(d, r);
	wr_mode(MD_TRANS);
}

void d3_pushbutton(short d, RECT *r, BFOBSPEC *col, short state, short thick, short mode)
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
			f_color(screen.dial_colours.bg_col);
		/* otherwise set by set_colours() */
		/* inside bar */
		gbar(d, r);
	}

	j = d;
	t = abs(thick);
	outline = j;

#if NAES3D
	if (default_options.naes && !(mode & 2))
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

/* strip leading and trailing spaces. */
void strip_name(char *to, const char *fro)
{
	const char *last = fro + strlen(fro) - 1;

	while (*fro && *fro == ' ')
		fro++;

	if (*fro)
	{
		while (*last == ' ') last--;
		while (*fro &&  fro != last + 1) *to++ = *fro++;
	}

	*to = '\0';
}

/* should become c:\s...ng\foo.bar */
void cramped_name(const void *s, char *t, short w)
{
	char tus[256];
	const char *q = s;
	char *p = t;
	short l, d, h;

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
	{
		strcpy(t, s);
	}
	else
	{
		if (w < 12)		/* 8.3 */
		{
			strcpy(t, q + d); /* only the last ch's */
		}
		else
		{
			h = (w-3)/2;
			strncpy(p, q, h);
			p += h;
			*p++='.';
			*p++='.';
			*p++='.';
			strcpy(p, q+l-h);
		}
	}
}

const char *clipped_name(const void *s, char *t, short w)
{
	const char *q = s;
	short l = strlen(q);

	if (l > w)
	{
		strncpy(t, q, w);
		t[w]=0;
		return t;
	}
	return s;
}

const char *
prop_clipped_name(const char *s, char *d, int w, short *ret_w, short *ret_h, short method)
{
	int swidth = 0;
	short cw, tmp;
	char *dst = d;
	char end[256];

	switch (method)
	{
		/*
		 * cramp string. "This is a long string, yeah?" becomes "This is...tring, yeah?"
		 */
		case 1:
		{
			const char *e = s + strlen(s);

			if (s + 8 < e)
			{
				int i, chars = 0;
				char c;
				bool tog = false;

				i = sizeof(end) - 1;
				end[i--] = '\0';
				while (s != e)
				{
					if (tog) c = *--e;
					else	 c = *s++;

					vqt_width(C.vh, c, &cw, &tmp, &tmp);
					if (cw != -1)
					{
						swidth += cw;
						if (swidth >= w)
						{
							if (tog) e++;
							else     s--;
							break;
						}
						if (tog)
							end[i--] = c, tog = false;
						else
							*dst++ = c, tog = true;
						chars++;
					}
				}
				*dst = '\0';
				i++;
				if (e != s)
				{
					if (chars > 6)
					{
						*(dst - 2) = '.';
						*(dst - 1) = '.';
						end[i] = '.';
					}
					else
					{
						while (end[i++])
							*dst++ = *s++;
						*dst = '\0';
						i--;
					}
				}
				if (end[i])
					strcat(d, &end[i]);
				break;
			}
			/* fall through */
		}
		/*
		 * Clip string. "This is a long string, yeah?" becomes "This is a long..."
		 */
		case 0:
		{
			while (*s)
			{
				vqt_width(C.vh, *s, &cw, &tmp, &tmp);
				if (cw != -1)
				{
					swidth += cw;

					if (swidth >= w)
						break;
					*dst++ = *s++;
				}
			}
			*dst = '\0';		
			if (*s)
			{
				int i = strlen(d);
				if (i > 8)
				{
					for (i = 3; i > 0 && dst != d; i--)
						*--dst = '.';
				}
			}
			break;
		}
	}			
	
	if (ret_w && ret_h)
		t_extent(d, ret_w, ret_h);

	return d;	
}
		
/* HR: 1 (good) set of routines for screen saving */
inline long calc_back(const RECT *r, short planes)
{
	return 2L * planes
		  * ((r->w + 15) / 16)
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
			vro_cpyfm(C.vh, S_ONLY, pnt, &Mscreen, &Mpreserve);
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
			vro_cpyfm(C.vh, S_ONLY, pnt, &Mpreserve, &Mscreen);
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
	vro_cpyfm(C.vh, S_ONLY, pnt, &Mscreen, &Mscreen);
}

void
shadow_object(short d, short state, RECT *rp, short colour, short thick)
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
			br_hook(d, &r, colour);
		}
	}
}

static short menu_dis_col(XA_TREE *wt)		/* Get colours for disabled better. */
{
	short c = G_BLACK;

	if (!MONO)
	{
		if (wt->is_menu)
		{
			OBJECT *ob = wt->tree + wt->current;
			if (ob->ob_state & OS_DISABLED)
			{
				c = screen.dial_colours.shadow_col;
				done(OS_DISABLED);
			}
		}
	}

	return c;
}

static BFOBSPEC button_colours(void)
{
	BFOBSPEC c;

	c.character = 0;
	c.framesize = 0;

	c.framecol = screen.dial_colours.border_col;
	c.textcol  = G_BLACK;
	c.textmode = 1;
	c.fillpattern = IP_HOLLOW;
	c.interiorcol = screen.dial_colours.bg_col;

	return c;
}

static void
ob_text(XA_TREE *wt, RECT *r, RECT *o, BFOBSPEC *c, const char *t, short state, short und)
{
	if (t && *t)
	{
		OBJECT *ob = wt->tree + wt->current;
		bool fits = !o || (o && (o->h >= r->h - (d3_foreground(ob) ? 4 : 0)));

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
				f_color(screen.dial_colours.bg_col);
				wr_mode(MD_REPLACE);
				gbar(0, o ? o : r);
				wr_mode(MD_TRANS);
			}
			else
				wr_mode(c->textmode ? MD_REPLACE : MD_TRANS);
		}

		if (!MONO && (state&OS_DISABLED) != 0)
		{
			done(OS_DISABLED);
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
			short l = strlen(t);
			if (und < l)
			{
				short x = r->x + und*screen.c_max_w,
				    y = r->y + screen.c_max_h - 1;
				line(x, y, x + screen.c_max_w - 1, y, G_RED);
			}
		}
	}
}

static void
g_text(XA_TREE *wt, RECT r, RECT *o, const char *text, short state)
{
	/* only center the text. ;-) */
	r.y += (r.h-screen.c_max_h) / 2;
	if (!MONO && (state&OS_DISABLED))
	{
		t_color(screen.dial_colours.lit_col);
		v_gtext(C.vh, r.x+1, r.y+1, text);
		t_color(screen.dial_colours.shadow_col);
		v_gtext(C.vh, r.x,   r.y,   text);
		done(OS_DISABLED);
	}
	else
	{
		t_color(menu_dis_col(wt));
		ob_text(wt, &r, o, NULL, text, 0, (state & OS_WHITEBAK) ? (state >> 8) & 0x7f : -1);
		if (state & OS_DISABLED)
		{
			write_disable(&wt->r, screen.dial_colours.bg_col);
			done(OS_DISABLED);
		}
	}
}

/* This function doesnt change colourword anymore, but just sets the required color.
 * Neither does it affect writing mode for text (this is handled in ob_text() */
static void
set_colours(OBJECT *ob, BFOBSPEC *colourword)
{
	wr_mode(MD_REPLACE);

	/* 2 */
	f_interior(FIS_PATTERN);

	if (colourword->fillpattern == IP_SOLID)
	{
		/* 2,8  solid fill  colour */
		f_style(8);
		f_color(colourword->interiorcol);
	}
	else
	{
		if (colourword->fillpattern == IP_HOLLOW)	
		{
			/* 2,8 solid fill  white */
			f_style(8);

			/* Object inherits default dialog background colour? */
			if ((colourword->interiorcol == 0) && d3_any(ob))
				f_color(screen.dial_colours.bg_col);
			else
				f_color(G_WHITE);

		}
		else
		{
			f_style(colourword->fillpattern);
			f_color(colourword->interiorcol);
		}
	}

#if SELECT_COLOR
	if (!MONO && (ob->ob_state & OS_SELECTED))
	{
		/* Allow a different colour set for 3d push  */
		if (d3_any(ob))
			f_color(selected3D_colour[colourword->interiorcol]);
		else
			f_color(selected_colour[colourword->interiorcol]);
	}
#endif

	t_color(colourword->textcol);
	l_color(colourword->framecol);
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
format_dialog_text(char *text_out, const char *template, const char *text_in, short edit_pos)
{
	short index = 0, tpos = 0, max = strlen(template);
	/* HR: In case a template ends with '_' and the text is completely
	 * filled, edit_index was indeterminate. :-)
	 */
	short edit_index = max;
	bool aap = *text_in == '@';

	DIAG((D_o, NULL, "format_dialog_text edit_pos %d", edit_pos));

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
	 RECT *gr,
	 RECT *cr,
	 bool formatted,
	 short edit_pos,
	 char *temp_text,
	 BFOBSPEC *colours,
	 short *thick,
	 RECT r)
{
	TEDINFO *ted = object_get_tedinfo(ob);
	RECT cur;
	short w, h, cur_x = 0;

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
		t_font(ted->te_fontsize, ted->te_fontid);
		cur.w = screen.c_max_w;
		cur.h = screen.c_max_h;
		break;
	}
	case TE_STANDARD:		/* Use the standard system font (probably 10 point) */
	{
		t_font(screen.standard_font_point, screen.standard_font_id);
		cur.w = screen.c_max_w;
		cur.h = screen.c_max_h;
		break;
	}
	case TE_SMALL:			/* Use the small system font (probably 8 point) */
	{
		t_font(screen.small_font_point, screen.small_font_id);
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
		cur_x = format_dialog_text(temp_text, ted->te_ptmplt, ted->te_ptext, edit_pos);
	else
		strncpy(temp_text, ted->te_ptext, 255);

	t_extent(temp_text, &w, &h);

	/* HR 290301 & 070202: Dont let a text violate its object space! (Twoinone packer shell!! :-) */
	if (w > r.w)
	{
		short	rw  = r.w / cur.w,
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
		cr->w = 1;
	}

	cur.w = w;
	cur.h = h;
	*gr = cur;
}

static void
rl_xor(RECT *r, struct xa_rect_list *rl)
{
	if (rl)
	{
		short c[4];
		while (rl)
		{
			rtopxy(c, &rl->r);
			vs_clip(C.vh, 1, c);
			write_selection(0, r);
			rl = rl->next;
		}
		vs_clip(C.vh, 1, C.global_clip);
	}
	else
		write_selection(0, r);
}
void
enable_objcursor(struct widget_tree *wt)
{
	if (wt->e.obj > 0)
	{
		set_objcursor(wt);
		wt->e.c_state |= OB_CURS_ENABLED;
	}
}

void
disable_objcursor(struct widget_tree *wt, struct xa_rect_list *rl)
{
	undraw_objcursor(wt, rl);
	wt->e.c_state &= ~OB_CURS_ENABLED;
}

void
eor_objcursor(struct widget_tree *wt, struct xa_rect_list *rl)
{
	RECT r;

	if (wt->e.obj != -1)
	{
		set_objcursor(wt);
		r = wt->e.cr;
		r.x += wt->tree->ob_x;
		r.y += wt->tree->ob_y;

		if (!(wt->tree[wt->e.obj].ob_flags & OF_HIDETREE))
			rl_xor(&r, rl);
	}
}
	
void
draw_objcursor(struct widget_tree *wt, struct xa_rect_list *rl)
{
	if ( (wt->e.c_state & (OB_CURS_ENABLED | OB_CURS_DRAWN)) == OB_CURS_ENABLED )
	{
		RECT r = wt->e.cr;

		r.x += wt->tree->ob_x;
		r.y += wt->tree->ob_y;
		
		if (wt->e.obj >= 0 && !(wt->tree[wt->e.obj].ob_flags & OF_HIDETREE))
		{
			rl_xor(&r, rl); //write_selection(0, &r);
			wt->e.c_state |= OB_CURS_DRAWN;
		}
	}
}
void
undraw_objcursor(struct widget_tree *wt, struct xa_rect_list *rl)
{
	if ( (wt->e.c_state & (OB_CURS_ENABLED | OB_CURS_DRAWN)) == (OB_CURS_ENABLED | OB_CURS_DRAWN) )
	{
		RECT r = wt->e.cr;

		r.x += wt->tree->ob_x;
		r.y += wt->tree->ob_y;
		
		if (wt->e.obj >= 0 && !(wt->tree[wt->e.obj].ob_flags & OF_HIDETREE))
		{
			rl_xor(&r, rl); //write_selection(0, &r);
			wt->e.c_state &= ~OB_CURS_DRAWN;
		}
	}
}

void
set_objcursor(struct widget_tree *wt)
{
	char temp_text[256];
	RECT r; // = wt->r;
	OBJECT *ob;
	RECT gr;
	BFOBSPEC colours;
	short thick;

	if (wt->e.obj <= 0)
		return;

	obj_offset(wt, wt->e.obj, &r.x, &r.y);
	ob = wt->tree + wt->e.obj;
	r.w  = ob->ob_width;
	r.h  = ob->ob_height;
	
	set_text(ob, &gr, &wt->e.cr, true, wt->e.pos, temp_text, &colours, &thick, r);

	wt->e.cr.x -= wt->tree->ob_x;
	wt->e.cr.y -= wt->tree->ob_y;

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
draw_g_box(struct widget_tree *wt, const RECT *clip)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	short thick;

	colours = object_get_spec(ob)->obspec;
	thick = object_thickness(ob);
	set_colours(ob, &colours);

	/* before borders */
	done(OS_SELECTED);

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

		if (ob->ob_state & OS_SELECTED)
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			if (!(wt->current == 0 && wt->zen))
			{
				g2d_box(thick, &r, colours.framecol);
				shadow_object(0, ob->ob_state, &r, colours.framecol, thick);
			}
		}
	}
}

void
d_g_box(enum locks lock, struct widget_tree *wt, const RECT *clip)
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
		draw_g_box(wt, clip);
		object_set_spec(ob, (unsigned long)p);
		ob->ob_type = ty;

		p->wt		= wt;
		p->index	= wt->current;
		p->r		= wt->r;
		p->clip		= *clip;
	#if 0
		p->clip.x	= clip->x;
		p->clip.y	= clip->y;
		p->clip.w	= clip->w - clip->x + 1;
		p->clip.h	= clip->h - clip->y + 1;
	#endif
		if (xa_rect_clip(&wt->r, &p->clip, &p->clip))
		{
			p->clip.w = p->clip.x + p->clip.w - 1;
			p->clip.h = p->clip.y + p->clip.h - 1;
				
			(*p->callout)(p);
		}
	}
	else
	{
		draw_g_box(wt, clip);
	}
}

/*
 * Draw a plain hollow ibox
 */
void
d_g_ibox(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	short thick;

	colours = object_get_spec(ob)->obspec;
	thick = object_thickness(ob);
	set_colours(ob, &colours);

	/* before borders */
	done(OS_SELECTED|OS_DISABLED);

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
		if (ob->ob_state & OS_SELECTED)
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			if (!(wt->current == 0 && wt->zen))
			{
				g2d_box(thick, &r, colours.framecol);
				shadow_object(0, ob->ob_state, &r, colours.framecol, thick);
			}
		}
	}
}

/*
 * Display a boxchar (respecting 3d flags)
 */
void
d_g_boxchar(enum locks lock, struct widget_tree *wt, const RECT *clip)
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
	set_colours(ob, &colours);

	/* Centre the text in the box */
	gr.x = r.x + ((r.w - screen.c_max_w) / 2);
	gr.y = r.y + ((r.h - screen.c_max_h) / 2);
	gr.w = screen.c_max_w;
	gr.h = screen.c_max_h;

	if (d3_foreground(ob))
	{
		d3_pushbutton(0, &r, &colours, ob->ob_state, thick, 1);
		if (ob->ob_state & OS_SELECTED)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}
		wr_mode(colours.textmode ? MD_REPLACE : MD_TRANS);
		t_font(screen.standard_font_point, screen.standard_font_id);
		ob_text(wt, &gr, &r, NULL, temp_text, 0, -1);
	}
	else
	{
		gbar(0, &r);
		t_font(screen.standard_font_point, screen.standard_font_id);
		ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

		if (selected)
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			g2d_box(thick, &r, colours.framecol);
			shadow_object(0, ob->ob_state, &r, colours.framecol, thick);
		}
	}

	done(OS_SELECTED);
}

/*
 * Draw a boxtext object
 */
void
d_g_boxtext(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	short thick = 0;
	ushort selected;
	RECT r = wt->r, gr;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	char temp_text[256];

	selected = ob->ob_state & OS_SELECTED;

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
	}
	else
	{
		gbar(0, &r);
		//t_font(10, 1);
		ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

		if (selected)
			write_selection(0, &r);		/* before border */

		if (thick)	/* Display a border? */
		{
			wr_mode(MD_REPLACE);
			g2d_box(thick, &r, colours.framecol);
			shadow_object(0, ob->ob_state, &r, colours.framecol, thick);
		}
	}

	//t_font(screen.standard_font_point, screen.standard_font_id);
	done(OS_SELECTED);
}
void
d_g_fboxtext(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	char temp_text[256];
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	RECT gr,cr;
	BFOBSPEC colours;
	const bool is_edit = (wt->current == wt->e.obj);
	const unsigned short selected = ob->ob_state & OS_SELECTED;
	short thick;

	set_text(ob, &gr, &cr, true, is_edit ? wt->e.pos : -1, temp_text, &colours, &thick, r);
	set_colours(ob, &colours);

	if (d3_foreground(ob))
	{
		d3_pushbutton(0, &r, &colours, ob->ob_state, thick, 1);
		if (selected)
		{
			gr.x += PUSH3D_DISTANCE;
			gr.y += PUSH3D_DISTANCE;
		}
		//t_font(10, 1);
		ob_text(wt, &gr, &r, &colours, temp_text, 0, -1);
	}
	else
	{
		gbar(0, &r);
		//t_font(10, 1);			
		ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

		if (selected)
			/* before border */
			write_selection(0, &r);

		/* Display a border? */
		if (thick)
		{
			wr_mode(MD_REPLACE);
			g2d_box(thick, &r, colours.framecol);
			shadow_object(0, ob->ob_state, &r, colours.framecol, thick);
		}
	}

#if 0
	/*
	 * Save cursor position
	 */
	if (is_edit)
	{
		wt->e.cr = cr;
		wt->e.cr.x -= wt->tree->ob_x;
		wt->e.cr.y -= wt->tree->ob_y;
	}
#endif

	//t_font(screen.standard_font_point, screen.standard_font_id);
	done(OS_SELECTED);
}

/*
 * Draw a button object
 */
void
d_g_button(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	RECT r = wt->r, gr = r;
	OBJECT *ob = wt->tree + wt->current;
	BFOBSPEC colours;
	short thick = object_thickness(ob); 
	ushort selected = ob->ob_state & OS_SELECTED;
	char *text = object_get_spec(ob)->free_string;

	colours = button_colours();

	t_color(G_BLACK);

	if ((ob->ob_state & OS_WHITEBAK) && (ob->ob_state & 0x8000))
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
				l_color(G_BLACK);
				gbox(0, &rr);
			}
			else
				chiseled_gbox(0, &rr);
			gr.x = r.x + screen.c_max_w;
			gr.y = r.y;
			t_font(screen.standard_font_point, screen.standard_font_id);
			t_extent(text, &gr.w, &gr.h);
			ob_text(wt, &gr, NULL, &colours, text, 0, -1);
		}
		else
		{
			XA_TREE b;

			b.owner = wt->owner; //C.Aes;
			b.tree = get_widgets();
			display_object(	lock, &b, clip,
					  (ob->ob_flags & OF_RBUTTON)
					? (selected ? RADIO_SLCT : RADIO_DESLCT )
					: (selected ? BUT_SLCT   : BUT_DESLCT   ),
					gr.x, gr.y, 11);
			gr.x += ICON_W;
			gr.x += screen.c_max_w;
			wr_mode(MD_TRANS);
			t_font(screen.standard_font_point, screen.standard_font_id);
			ob_text(wt, &gr, &r, NULL, text, 0, und & 0x7f);
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
		t_font(screen.standard_font_point, screen.standard_font_id);
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
			f_color(selected ? G_BLACK : G_WHITE);

			/* display inside bar */		
			gbar(-thick, &r);

			wr_mode(MD_TRANS);
			t_color(selected ? G_WHITE : G_BLACK);
			ob_text(wt, &gr, &r, NULL, text, 0, und);

			/* Display a border? */
			if (thick)
			{
				g2d_box(thick, &r, G_BLACK);
				shadow_object(0, ob->ob_state, &r, colours.framecol, thick);
			}
		}
	}
	done(OS_SELECTED);
}

static void
icon_characters(ICONBLK *iconblk, short state, short obx, short oby, short icx, short icy)
{
	char lc = iconblk->ib_char;
	short tx,ty,pnt[4];

	wr_mode(MD_REPLACE);
	ritopxy(pnt, obx + iconblk->ib_xtext, oby + iconblk->ib_ytext,
		     iconblk->ib_wtext, iconblk->ib_htext);

	t_font(screen.small_font_point, screen.small_font_id);

	/* center the text in a bar given by iconblk->tx, relative to object */
	t_color(G_BLACK);
	if (   iconblk->ib_ptext
	    && *iconblk->ib_ptext
	    && iconblk->ib_wtext
	    && iconblk->ib_htext)
	{
		f_color((state&OS_SELECTED) ? G_BLACK : G_WHITE);
		f_interior(FIS_SOLID);
		v_bar(C.vh,pnt);
	
		tx = obx + iconblk->ib_xtext + ((iconblk->ib_wtext - strlen(iconblk->ib_ptext)*6) / 2);	
		ty = oby + iconblk->ib_ytext + ((iconblk->ib_htext - 6) / 2);

		if (state & OS_SELECTED)
			wr_mode(MD_XOR); if (state & OS_DISABLED)
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

	if (state & OS_SELECTED)
		f_color(G_WHITE);

	//t_font(screen.standard_font_point, screen.standard_font_id);
	t_effect(0);
	wr_mode( MD_TRANS);
}

/*
 * Draw a image
 */
void
d_g_image(enum locks lock, struct widget_tree *wt, const RECT *clip)
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

	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);
}

/* HR 060202: all icons: handle disabled: make icon rectangle and text faint in stead
 *                                        of object rectangle.
 */

/*
 * Draw a mono icon
 */
void
d_g_icon(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	OBJECT *ob = wt->tree + wt->current;
	ICONBLK *iconblk;
	MFDB Mscreen;
	MFDB Micon;
	RECT ic;
	short pxy[8], cols[2], obx, oby, msk_col, icn_col;
	
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
	Micon.fd_wdwidth = (ic.w + 15) / 16;
	Micon.fd_nplanes = 1;
	Micon.fd_stand = 0;
	Mscreen.fd_addr = NULL;
			
	Micon.fd_addr = iconblk->ib_pmask;
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
		
	cols[0] = msk_col;
	cols[1] = icn_col;
	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);
			
	Micon.fd_addr = iconblk->ib_pdata;
	cols[0] = icn_col;
	cols[1] = msk_col;
	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);

	if (ob->ob_state & OS_DISABLED)
		write_disable(&ic, G_WHITE);

	/* should be the same for color & mono */
	icon_characters(iconblk, ob->ob_state & (OS_SELECTED|OS_DISABLED), obx, oby, ic.x, ic.y);

	done(OS_SELECTED|OS_DISABLED);
}

/*
 * Draw a colour icon
 */
void
d_g_cicon(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	OBJECT *ob = wt->tree + wt->current;
	ICONBLK *iconblk;
	//CICONBLK *ciconblk;
	CICON	*best_cicon;
	MFDB Mscreen;
	MFDB Micon;
	bool have_sel;
	RECT ic;
	short pxy[8], cols[2] = {0,1}, obx, oby, blitmode;

	best_cicon = getbest_cicon(object_get_spec(ob)->ciconblk);
#if 0
	ciconblk = object_get_spec(ob)->ciconblk;
	best_cicon = NULL;
	
	DIAG((D_o, wt->owner, "cicon ciconblk 0x%lx", ciconblk));

	c = ciconblk->mainlist;
	while (c)
	{
		//DIAG((D_o, wt->owner, "cicon cicon 0x%lx", c));

		/* Jinnee v<2.5 has misunderstood the next_res NULL rule :( */
		if ( c == (CICON*)-1 ) break;

		if (c->num_planes <= screen.planes
		    && (!best_cicon || (best_cicon && c->num_planes > best_cicon->num_planes)))
		{
			//DIAG((D_o, wt->owner, "cicot best_cicon 0x%lx planes=%d", c, c->num_planes));
			best_cicon = c;
		}

		c = c->next_res;	
	}
#endif
	/* No matching icon, so use the mono one instead */
	if (!best_cicon)
	{
		//DIAG((D_o, wt->owner, "cicon !best_cicon", c));
		d_g_icon(lock, wt, clip);
		return;
	}

	//c = best_cicon;

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

	//have_sel = c->sel_data != NULL;
	have_sel = best_cicon->sel_data ? true : false;

	//DIAG((D_o, wt->owner, "cicon sel_mask 0x%lx col_mask 0x%lx", c->sel_mask, c->col_mask));

	/* check existence of selection. */
	if ((ob->ob_state & OS_SELECTED) && have_sel)
		Micon.fd_addr = best_cicon->sel_mask; //c->sel_mask;
	else
		Micon.fd_addr = best_cicon->col_mask; //c->col_mask;

	blitmode = screen.planes > 8 ? S_AND_D : S_OR_D;

	vrt_cpyfm(C.vh, MD_TRANS, pxy, &Micon, &Mscreen, cols);

	if ((ob->ob_state & OS_SELECTED) && have_sel)
		Micon.fd_addr = best_cicon->sel_data; //c->sel_data;
	else
		Micon.fd_addr = best_cicon->col_data; //c->col_data;

	Micon.fd_nplanes = screen.planes;
	vro_cpyfm(C.vh, blitmode, pxy, &Micon, &Mscreen);

	if ((ob->ob_state & OS_SELECTED) && !have_sel)
	{
		Micon.fd_addr = best_cicon->col_mask; //c->col_mask;
		Micon.fd_nplanes = 1;
		vrt_cpyfm(C.vh, MD_XOR, pxy, &Micon, &Mscreen, cols);
	}

	if (ob->ob_state & OS_DISABLED)
		write_disable(&ic, G_WHITE);

	icon_characters(iconblk, ob->ob_state & (OS_SELECTED|OS_DISABLED), obx, oby, ic.x, ic.y);

	done(OS_SELECTED|OS_DISABLED);
}

/*
 * Draw a text object
 */
void
d_g_text(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	short thick,thin;
	OBJECT *ob = wt->tree + wt->current;
	RECT r = wt->r, gr;
	BFOBSPEC colours;
	char temp_text[256];

	set_text(ob, &gr, NULL, false, -1, temp_text, &colours, &thick, r);
	set_colours(ob, &colours);
	thin = thick > 0 ? thick-1 : thick+1;

	if (d3_foreground(ob))
	{
		d3_pushbutton(thick > 0 ? -thick : 0, &r, &colours, ob->ob_state, thin, 3);
		done(OS_SELECTED);
	}

	//t_font(10, 1);
	ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

	//t_font(screen.standard_font_point, screen.standard_font_id);
}
void
d_g_ftext(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	short thick,thin;
	OBJECT *ob = wt->tree + wt->current;
	RECT r = wt->r, gr, cr;
	BFOBSPEC colours;
	bool is_edit = wt->current == wt->e.obj;
	char temp_text[256];

	set_text(ob, &gr, &cr, true, is_edit ? wt->e.pos : -1, temp_text, &colours, &thick, r);
	set_colours(ob, &colours);
	thin = thick > 0 ? thick-1 : thick+1;

	if (d3_foreground(ob))
	{
		d3_pushbutton(thick > 0 ? -thick : 0, &r, &colours, ob->ob_state, thin, 3);
		done(OS_SELECTED);
	}

	ob_text(wt, &gr, &r, &colours, temp_text, ob->ob_state, -1);

#if 0
	/*
	 * Save cursor position
	 */
	if (is_edit)
	{
		wt->e.cr = cr;
		wt->e.cr.x -= wt->tree->ob_x;
		wt->e.cr.y -= wt->tree->ob_y;
	}
#endif

	//t_font(screen.standard_font_point, screen.standard_font_id);
}

#define userblk(ut) (*(USERBLK **)(ut->userblk_pp))
#define ret(ut)     (     (long *)(ut->ret_p     ))
#define parmblk(ut) (  (PARMBLK *)(ut->parmblk_p ))
void
d_g_progdef(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	struct sigaction oact, act;
	struct xa_client *client = lookup_extension(NULL, XAAES_MAGIC);
	OBJECT *ob = wt->tree + wt->current;
	PARMBLK *p;
	short r[4];

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

	*(RECT *)&(p->pb_xc) = *clip;
#if 0
	p->pb_xc = clip->x; //C.global_clip[0];
	p->pb_yc = clip->y; //C.global_clip[1];
	p->pb_wc = clip->w - clip->x + 1; //C.global_clip[2] - C.global_clip[0] + 1;
	p->pb_hc = clip->h - clip->y + 1; //C.global_clip[3] - C.global_clip[1] + 1;
#endif
	userblk(client->ut) = object_get_spec(ob)->userblk;
	p->pb_parm = userblk(client->ut)->ub_parm;

	wr_mode(MD_TRANS);

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
	*(RECT *)&r = *clip;
	r[2] += (r[0] - 1);
	r[3] += (r[1] - 1);
	vs_clip(C.vh, 1, (short *)&r);
	
	if (*wt->state_mask & OS_DISABLED)
	{
		write_disable(&wt->r, screen.dial_colours.bg_col);
		done(OS_DISABLED);
	}
	wr_mode(MD_REPLACE);
}
#undef userblk
#undef ret
#undef parmblk

#if 0
static void
l_text(short x, short y, char *t, short w, short left)
{
	char ct[256];
	short l = strlen(t);
	w /= screen.c_max_w;
	if (left < l)
	{
		strcpy(ct, t + left);
		ct[w]=0;
		v_gtext(C.vh, x, y, ct);
	}
}
#endif

void
d_g_slist(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	RECT r = wt->r, wa;
	SCROLL_INFO *list;
	SCROLL_ENTRY *this;
	struct xa_window *w;
	//short y, maxy;
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

	t_color(G_BLACK);
	
	if (list->state == 0)
	{
		get_widget(w, XAW_TITLE)->stuff = list->title;
		draw_window(list->lock, w, clip);
		draw_slist(lock, list, NULL, clip);
	}
	done(OS_SELECTED);
}

void
d_g_string(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	ushort state = ob->ob_state;
	char *text = object_get_spec(ob)->free_string;

	/* most AES's allow null string */
	if (text)
	{
		bool tit =    ( state & OS_WHITEBAK )
		           && ( state & 0xff00   ) == 0xff00;

		wr_mode(MD_TRANS);

		if (d3_foreground(ob) && !tit)
			d3_pushbutton(0, &r, NULL, state, 0, 0);

		if (   wt->is_menu
		    && (ob->ob_state & OS_DISABLED)
		    && *text == '-')
		{
			r.x += 1, r.w -= 3;
			r.y += (r.h - 2)/2;
			if (MONO)
			{
				vsl_type(C.vh, 7);
				vsl_udsty(C.vh, 0xaaaa);
				line(r.x, r.y, r.x + r.w, r.y, G_BLACK);
				vsl_udsty(C.vh, 0x5555);
				line(r.x, r.y + 1, r.x + r.w, r.y + 1, G_BLACK);
				vsl_type(C.vh, 0);
			}
			else
			{
				r.x += 2, r.w -= 4;
				line(r.x, r.y,     r.x + r.w, r.y,     screen.dial_colours.fg_col);
				line(r.x, r.y + 1, r.x + r.w, r.y + 1, screen.dial_colours.lit_col);
				l_color(G_BLACK);
			}
			done(OS_DISABLED);
		}
		else
		{
			t_font(screen.standard_font_point, screen.standard_font_id);
			g_text(wt, r, &wt->r, text, ob->ob_state);
		}

		if (tit)
			line(r.x, r.y + r.h, r.x + r.w -1, r.y + r.h, G_BLACK);
	}
}

void
d_g_title(enum locks lock, struct widget_tree *wt, const RECT *clip)
{
	RECT r = wt->r;
	OBJECT *ob = wt->tree + wt->current;
	const char *text = object_get_spec(ob)->free_string;

	wr_mode( MD_TRANS);

	/* menu in user window.*/
	if (!wt->menu_line)
		d3_pushbutton(-2, &r, NULL, ob->ob_state, 1, MONO ? 1 : 3);

	/* most AES's allow null string */
	if (text)
	{
		t_font(screen.standard_font_point, screen.standard_font_id);
		g_text(wt, r, &wt->r, text, ob->ob_state);
	}

	if (ob->ob_state & OS_SELECTED && wt->menu_line)
		/* very special!!! */
		write_selection(0, &r);

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
	objc_jump_table[G_SLIST   ] = d_g_slist;
	objc_jump_table[G_SHORTCUT] = d_g_string;
	objc_jump_table[G_EXTBOX  ] = d_g_box;
}

/*
 * Display a primitive object
 */
void
display_object(enum locks lock, XA_TREE *wt, const RECT *clip, short item, short parent_x, short parent_y, short which)
{
	RECT r, o;
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

	o.x = r.x + o.x;
	o.y = r.y + o.y;
	o.w = r.w - o.w;
	o.h = r.h - o.h;
	
	if (   o.x		> (clip->x + clip->w - 1) //C.global_clip[2]	/* x + w */
	    || o.x + o.w - 1	< clip->x //C.global_clip[0]	/* x     */
	    || o.y		> (clip->y + clip->h - 1) //C.global_clip[3]	/* y + h */
	    || o.y + o.h - 1	< clip->y) //C.global_clip[1])	/* y     */
		return;

	if (t <= G_UNKNOWN)
		/* Get display routine for this type of object from jump table */
		display_routine = objc_jump_table[t];

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
	wr_mode(MD_TRANS);

#if 1
#if GENERATE_DIAGS
	if (wt->tree != get_widgets())
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
	(*display_routine)(lock, wt, clip);

	wr_mode(MD_TRANS);

	if (t != G_PROGDEF)
	{
		/* Handle CHECKED object state: */
		if ((ob->ob_state & state_mask) & OS_CHECKED)
		{
			t_color(G_BLACK);
			/* ASCII 8 = checkmark */
			v_gtext(C.vh, r.x + 2, r.y, "\10");
		}

		/* Handle DISABLED state: */
		if ((ob->ob_state & state_mask) & OS_DISABLED)
			write_disable(&r, G_WHITE);

		/* Handle CROSSED object state: */
		if ((ob->ob_state & state_mask) & OS_CROSSED)
		{
			short p[4];
			l_color(G_BLACK);
			p[0] = r.x;
			p[1] = r.y;
			p[2] = r.x + r.w - 1;
			p[3] = r.y + r.h - 1;
			v_pline(C.vh, 2, p);
			p[0] = r.x + r.w - 1;
			p[2] = r.x;
			v_pline(C.vh, 2, p);
		}

		/* Handle OUTLINED object state: */
		if ((ob->ob_state & state_mask) & OS_OUTLINED)
		{
			/* special handling of root object. */
			if (!wt->zen || item != 0)
			{
				if (!MONO && d3_any(ob))
				{
					tl_hook(1, &r, screen.dial_colours.lit_col);
					br_hook(1, &r, screen.dial_colours.shadow_col);
					tl_hook(2, &r, screen.dial_colours.lit_col);
					br_hook(2, &r, screen.dial_colours.shadow_col);
					gbox(3, &r);
				}
				else
				{
					l_color(G_WHITE);
					gbox(1, &r);
					gbox(2, &r);
					l_color(G_BLACK);
					gbox(3, &r);
				}
			}
		}

		if ((ob->ob_state & state_mask) & OS_SELECTED)
			write_selection(0, &r);
	}

	wr_mode(MD_TRANS);
}

/*
 * Walk an object tree, calling display for each object
 * HR: is_menu is true if a menu.
 */

/* draw_object_tree */
short
draw_object_tree(enum locks lock, XA_TREE *wt, OBJECT *tree, short item, short depth, short *xy) //short which)
{
	XA_TREE this;
	short next;
	short current = 0, rel_depth = 1, head;
	short x, y;
	bool start_drawing = false;
	bool curson = (wt->e.c_state & (OB_CURS_ENABLED | OB_CURS_DRAWN)) == (OB_CURS_ENABLED | OB_CURS_DRAWN) ? true : false;
	RECT clip = *(RECT *)&C.global_clip;
	IFDIAG(short *cl = C.global_clip;)

	clip.w -= (clip.x - 1);
	clip.h -= (clip.y - 1);
	
	if (wt == NULL)
	{
		this = nil_tree;

		wt = &this;
		wt->e.obj = -1;
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
	DIAG((D_objc, wt->owner, "  -   clip: %d.%d/%d.%d    %d/%d,%d/%d",
		cl[0], cl[1], cl[2], cl[3], cl[0], cl[1], cl[2] - cl[0] + 1, cl[3] - cl[1] + 1));

	depth++;

	if (wt->owner->options.xa_objced && curson)
		undraw_objcursor(wt, NULL);

	do {

	//	DIAG((D_objc, NULL, " ~~~ obj=%d(%d/%d), flags=%x, state=%x, head=%d, tail=%d, next=%d, depth=%d, draw=%s",
	//		current, x, y, tree[current].ob_flags, tree[current].ob_state,
	//		tree[current].ob_head, tree[current].ob_tail, tree[current].ob_next,
	//		rel_depth, start_drawing ? "yes":"no"));

		if (current == item)
		{
			start_drawing = true;
			rel_depth = 0;
		}

		if (start_drawing && !(tree[current].ob_flags & OF_HIDETREE))
		{
			/* Display this object */
			display_object(lock, wt, (const RECT *)&clip, current, x, y, 10);
		}

		head = tree[current].ob_head;

		/* Any non-hidden children? */
		if (    head != -1
		    && (tree[current].ob_flags & OF_HIDETREE) == 0
		    && (start_drawing == 0
			|| (start_drawing != 0
			    && rel_depth < depth)))
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
			while (next != -1 && tree[next].ob_tail == current)
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
	while (current != -1 && !(start_drawing && rel_depth < 1));

	if (wt->owner->options.xa_objced && curson)
		draw_objcursor(wt, NULL);

	wr_mode(MD_TRANS);
	f_interior(FIS_SOLID);

	DIAGS(("draw_object_tree exit OK!"));
	return true;
}


#if 1


#endif
