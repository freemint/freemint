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

#define PROFILING	0

#include "xa_types.h"
#include "xa_global.h"
#include "trnfm.h"

static void _cdecl
r2pxy(short *p, short d, const RECT *r)
{
	*p++ = r->x - d;
	*p++ = r->y - d;
	*p++ = r->x + r->w + d - 1;
	*p   = r->y + r->h + d - 1;
}

static void _cdecl
rtopxy(short *p, const RECT *r)
{
	*p++ = r->x;
	*p++ = r->y;
	*p++ = r->x + r->w - 1;
	*p   = r->y + r->h - 1;
}

static void _cdecl
ri2pxy(short *p, short d, short x, short y, short w, short h)
{
	*p++ = x - d;
	*p++ = y - d;
	*p++ = x + w + d - 1;
	*p   = y + h + d - 1;
}

static void _cdecl
ritopxy(short *p, short x, short y, short w, short h)
{
	*p++ = x;
	*p++ = y;
	*p++ = x + w - 1;
	*p   = y + h - 1;
}

static void _cdecl
xa_load_fonts(struct xa_vdi_settings *v)
{
	if (v->fonts_loaded ==  -1)
		v->fonts_loaded = C.gdos_version ? vst_load_fonts(v->handle, 0) : 0;
}
static void _cdecl
xa_unload_fonts(struct xa_vdi_settings *v)
{
	if (v->fonts_loaded)
	{
		if (C.gdos_version)
			vst_unload_fonts(v->handle, 0);
		v->fonts_loaded = -1;
	}
}

static void _cdecl
xa_clear_clip(struct xa_vdi_settings *v)
{
	short r[4];

	rtopxy(r, (RECT *)&v->screen);
	vs_clip(v->handle, 1, r);
};

static void _cdecl
xa_restore_clip(struct xa_vdi_settings *v, const RECT *s)
{
	short r[4];

	v->clip = *s;
	rtopxy(r, s);
	vs_clip(v->handle, 1, r);
};

static void _cdecl
xa_save_clip(struct xa_vdi_settings *v, RECT *s)
{
	*s = v->clip;
}

static void _cdecl
xa_set_clip(struct xa_vdi_settings *v, const RECT *clip)
{
	short r[4];

	if (!clip)
		clip = &v->clip;

	if (clip->w > 0 && clip->h > 0)
	{
		if (clip != &v->clip) v->clip = *clip;
	}
	else
		v->clip = v->screen;

	rtopxy(r, &v->clip);
	vs_clip(v->handle, 1, r);
}

static void _cdecl
xa_wr_mode(struct xa_vdi_settings *v, short m)
{
	if (v->wr_mode != m)
	{
		v->wr_mode = m;
		vswr_mode(v->handle, m);
	}
}

static void _cdecl xa_f_color(struct xa_vdi_settings *, short);
static void _cdecl xa_f_interior(struct xa_vdi_settings *, short);
static void _cdecl xa_gbar(struct xa_vdi_settings *v, short d, const RECT *r);		/* for perimeter = 0 */

static void _cdecl
xa_write_disable(struct xa_vdi_settings *v, RECT *r, short colour)
{
	static short pattern[16] =
	{
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa
	};
	xa_wr_mode(v, MD_TRANS);
	xa_f_color(v, colour);
	vsf_udpat(v->handle, pattern, 1);
	xa_f_interior(v, FIS_USER);
	xa_gbar(v, 0, r);
}

static void _cdecl
xa_l_color(struct xa_vdi_settings *v, short col)
{
	if (v->line_color != col)
	{
		v->line_color = col;
		vsl_color(v->handle, col);
	}
}

static void _cdecl
xa_line(struct xa_vdi_settings *v, short x, short y, short x1, short y1, short col)
{
	short pxy[4] = { x, y, x1, y1 };

	xa_l_color(v, col);
	v_pline(v->handle, 2, pxy);
}

static void _cdecl
xa_l_type(struct xa_vdi_settings *v, short type)
{
	if (v->line_style != type)
	{
		v->line_style = type;
		vsl_type(v->handle, type);
	}
}
static void _cdecl
xa_l_udsty(struct xa_vdi_settings *v, unsigned short style)
{
	v->line_udsty = style;
	vsl_udsty(v->handle, style);
}

static void _cdecl
xa_l_ends(struct xa_vdi_settings *v, short beg, short end)
{
	if (v->line_beg != beg || v->line_end != end)
	{
		v->line_beg = beg;
		v->line_end = end;
		vsl_ends(v->handle, beg, end);
	}
}

static void _cdecl
xa_l_width(struct xa_vdi_settings *v, short width)
{
	if (v->line_width != width)
	{
		v->line_width = width;
		vsl_width(v->handle, width);
	}
}

static void _cdecl
xa_f_color(struct xa_vdi_settings *v, short col)
{
	if (v->fill_color != col)
	{
		v->fill_color = col;
		vsf_color(v->handle, col);
	}
}

static void _cdecl
xa_t_color(struct xa_vdi_settings *v, short col)
{
	if (v->text_color != col)
	{
		v->text_color = col;
		vst_color(v->handle, col);
	}
}

static void _cdecl
xa_t_effects(struct xa_vdi_settings *v, short efx)
{
	if (v->text_effects != efx)
	{
		v->text_effects = efx;
		vst_effects(v->handle, efx);
	}
}

static void _cdecl
xa_t_font(struct xa_vdi_settings *v, short point, short id)
{
	if (id != -1 && v->font_rid != id)
	{
		v->font_rid = id;
		v->font_sid = vst_font(v->handle, id);
		id = 0;
		if (point == -1)
			point = v->font_rsize;
	}
	else
		id = 1;

	/*
	 * Ozk: Hmm... have to reset point size when changing fonts!!??
	 */
	if (point != -1 )	//&& (!id || v->font_rsize != point))
	{
		v->font_rsize = point;
		v->font_ssize = vst_point(v->handle, point, &v->font_w, &v->font_h, &v->cell_w, &v->cell_h);
		id = 0;
	}
	if (!id)
	{
		vqt_fontinfo(v->handle, &v->first_ade, &v->last_ade, &v->dists[0], &v->max_w, &v->efx[0]);
		v->dists[5] = 0;
	}
}

static void _cdecl
xa_t_alignment(struct xa_vdi_settings *v, short halign, short valign)
{
	if (v->halign != halign || v->valign != valign)
	{
		vst_alignment(v->handle, halign, valign, &v->halign, &v->valign);
	}
}

static void _cdecl
xa_t_extent(struct xa_vdi_settings *v, const char *t, short *w, short *h)
{
	short e[8];

	vqt_extent(v->handle, t, e);

	*w = e[4] - e[6];	/* oberen rechten minus oberen linken x coord */
	*h = -(e[1] - e[7]);	/* unteren linken minus oberen linken y coord
				 * vqt_extent uses a true Cartesion coordinate system
				 * How about that? :-)
				 * A mathematicion lost among nerds!
				 */
}

static void _cdecl
xa_text_extent(struct xa_vdi_settings *v, const char *t, struct xa_fnt_info *f, short *w, short *h)
{
	PRDEF( xa_text_extent, xa_t_font );
	PRDEF( xa_text_extent, effects );
	PRDEF( xa_text_extent, t_extent);

	PROFRECv( xa_t_font,(v, f->p, f->f));

	if( f->e ){
		PROFRECv( vst_effects,(v->handle, f->e));
	}

	PROFRECv( xa_t_extent,(v, t, w, h));

	if( f->e ){
		PROFRECv( vst_effects,(v->handle, 0));
	}
}

static const char * _cdecl
xa_prop_clipped_name(struct xa_vdi_settings *v, const char *s, char *d, int w, short *ret_w, short *ret_h, short method)
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

					/* fvdi-bug #nn: if high-bit in c is set, vqt_width returns random values for cw */
					if( C.fvdi_version > 0 && (c & 0x80) )
						tmp = vqt_width(v->handle, 'a', &cw, &tmp, &tmp);
					else
						tmp = vqt_width(v->handle, c, &cw, &tmp, &tmp);

					if (tmp != -1)
					{
						swidth += cw;
						if (swidth > w)
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
		case 2:
		case 0:
		{
			for(; *s; s++)
			{
				tmp = vqt_width(v->handle, *s, &cw, &tmp, &tmp);
				if (tmp != -1)
				{
					swidth += cw;

					if (swidth > w)
						break;
					*dst++ = *s;
				}
			}
			*dst = '\0';
			if (method == 0 && *s)
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
		case -1:
			strcpy( d, s );
		break;
	}

	if (ret_w && ret_h)
		xa_t_extent(v, d, ret_w, ret_h);

	return d;
}

static void _cdecl
xa_wtxt_output(struct xa_vdi_settings *v, struct xa_wtxt_inf *wtxti, char *txt, short state, const RECT *r, short xoff, short yoff)
{
	struct xa_fnt_info *wtxt;
	bool sel = state & OS_SELECTED;
	short x, y, f = wtxti->flags;
	char t[200];

	if (sel)
		wtxt = &wtxti->s;
	else
		wtxt = &wtxti->n;

	xa_wr_mode(v, wtxt->wrm); //MD_TRANS);
	xa_t_font(v, wtxt->p, wtxt->f);

	xa_t_effects(v, wtxt->e); //vst_effects(v->handle, wtxt->e);

	if (f & WTXT_NOCLIP)
		strncpy(t, txt, sizeof(t));
	else
		xa_prop_clipped_name(v, txt, t, r->w - (xoff << 1), &x, &y, 1);

	xa_t_extent(v, t, &x, &y);
	y = yoff + r->y + ((r->h - y) >> 1);

	if (f & WTXT_CENTER)
	{
		x = r->x + ((r->w - x) >> 1);
		if (x < (r->x + xoff))
			x += ((r->x + xoff) - x);
	}
	else
		x = xoff + r->x;

	if (f & WTXT_DRAW3D)
	{
		if (sel && (f & WTXT_ACT3D))
			x++, y++;

		xa_t_color(v, wtxt->bgc);
		x++;
		y++;
		v_gtext(v->handle, x, y, t);
		x--;
		y--;
	}
	xa_t_color(v, wtxt->fgc);
	v_gtext(v->handle, x, y, t);

	/* normal */
	xa_t_effects(v, 0);
}

static void _cdecl
xa_f_interior(struct xa_vdi_settings *v, short m)
{
	if (v->fill_interior != m)
	{
		v->fill_interior = m;
		vsf_interior(v->handle, m);
	}
}

static void _cdecl
xa_f_style(struct xa_vdi_settings *v, short m)
{
	if (v->fill_style != m)
	{
		v->fill_style = m;
		vsf_style(v->handle, m);
	}
}
static void _cdecl
xa_f_perimeter(struct xa_vdi_settings *v, short m)
{
	if (v->fill_perimeter != m)
	{
		v->fill_perimeter = m;
		vsf_perimeter(v->handle, m);
	}
}

static void _cdecl
xa_gbox(struct xa_vdi_settings *v, short d, const RECT *r)
{
#if 0
	XVDIPB *pb = create_vdipb();
	if (pb) {
		pb->ptsin[0] = pb->ptsin[6] = pb->ptsin[8] = r->x - d;
		pb->ptsin[1] = pb->ptsin[3] = r->y - d;
		pb->ptsin[2] = pb->ptsin[4] = pb->ptsin[0] + r->w + d+d;
// 		pb->ptsin[3] = pb->ptsin[1];
// 		pb->ptsin[4] = pb->ptsin[2];
		pb->ptsin[5] = pb->ptsin[7] = pb->ptsin[1] + r->h + d+d;
// 		pb->ptsin[6] = pb->ptsin[0];
// 		pb->ptsin[7] = pb->ptsin[5];
// 		pb->ptsin[8] = pb->ptsin[0];
		pb->ptsin[9] = pb->ptsin[1] + 1;
		VDI(pb, 6, 5, 0, 0, v->handle);
	}
#else
	short l[10];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;
	l[0] = x;
	l[1] = y;
	l[2] = x + w - 1;
	l[3] = y;
	l[4] = x + w - 1;
	l[5] = y + h - 1;
	l[6] = x;
	l[7] = y + h - 1;
	l[8] = x;
	l[9] = y + 1;			/* for Xor mode :-) */
	v_pline(v->handle, 5, l);
#endif
}

static void _cdecl
xa_rgbox(struct xa_vdi_settings *v, short d, short rnd, const RECT *r)
{
	short l[10];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;

	l[0] = x + rnd;
	l[1] = l[3] = y;
	l[2] = x + w - 1 - rnd;
	v_pline(v->handle, 2, l);
	l[1] += (h - 1);
	l[3] = l[1];
	v_pline(v->handle, 2, l);

	l[0] = l[2] = x;
	l[1] = y + rnd;
	l[3] = y + h - 1 - rnd;
	v_pline(v->handle, 2, l);
	l[0] += (w - 1);
	l[2] = l[0];
	v_pline(v->handle, 2, l);
}
static void _cdecl
xa_box(struct xa_vdi_settings *v, short d, short x, short y, short w, short h)
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
	v_pline(v->handle,5,l);
}

static void _cdecl
xa_gbar(struct xa_vdi_settings *v, short d, const RECT *r)		/* for perimeter = 0 */
{
	short l[4];
	l[0] = r->x - d;
	l[1] = r->y - d;
	l[2] = r->x + r->w + d - 1;
	l[3] = r->y + r->h + d - 1;
	v_bar(v->handle, l);
}

static void _cdecl
xa_bar(struct xa_vdi_settings *v, short d,  short x, short y, short w, short h)
{
	short l[4];
	x -= d, y -= d, w += d+d, h += d+d;
	l[0] = x;
	l[1] = y;
	l[2] = x+w-1;
	l[3] = y+h-1;
	v_bar(v->handle, l);
}

static void _cdecl
xa_p_gbar(struct xa_vdi_settings *v, short d, const RECT *r)	/* for perimeter = 1 */
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
//	BLOG((0,"xa_p_gbar:%d/%d/%d/%d", l[0], l[1], l[2], l[3] ));
 	v_bar(v->handle, l);
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
	v_pline(v->handle,5,l);
}

static void _cdecl
xa_top_line(struct xa_vdi_settings *v, short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d + d;

	xa_l_color(v, col);
	pnt[0] = x;
	pnt[1] = y;
	pnt[2] = x + w - 1;
	pnt[3] = y;
	v_pline(v->handle, 2, pnt);
}

static void _cdecl
xa_bottom_line(struct xa_vdi_settings *v, short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;

	xa_l_color(v, col);
	pnt[0] = x;
	pnt[1] = y + h - 1;
	pnt[2] = x + w - 1;;
	pnt[3] = pnt[1];
	v_pline(v->handle, 2, pnt);
}

static void _cdecl
xa_left_line(struct xa_vdi_settings *v, short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short h = r->h + d+d;

	xa_l_color(v, col);
	pnt[0] = x;
	pnt[1] = y;
	pnt[2] = x;
	pnt[3] = y + h - 1;
	v_pline(v->handle, 2, pnt);
}

static void _cdecl
xa_right_line(struct xa_vdi_settings *v, short d, const RECT *r, short col)
{
	short pnt[4];

	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + d+d;
	short h = r->h + d+d;

	xa_l_color(v, col);
	pnt[0] = x + w - 1;
	pnt[1] = y;
	pnt[2] = pnt[0];
	pnt[3] = y + h - 1;
	v_pline(v->handle, 2, pnt);
}

static void _cdecl
xa_tl_hook(struct xa_vdi_settings *v, short d, const RECT *r, short col)
{
	short pnt[6];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + (d+d);
	short h = r->h + (d+d);
	xa_l_color(v, col);
	pnt[0] = x;
	pnt[1] = y + h - 1; // - PW;
	pnt[2] = x;
	pnt[3] = y;
	pnt[4] = x + w - 1; // - PW;
	pnt[5] = y;
	v_pline(v->handle, 3, pnt);
}

static void _cdecl
xa_br_hook(struct xa_vdi_settings *v, short d, const RECT *r, short col)
{
	short pnt[6];
	short x = r->x - d;
	short y = r->y - d;
	short w = r->w + (d+d);
	short h = r->h + (d+d);
	xa_l_color(v, col);
	pnt[0] = x; // + PW;
	pnt[1] = y + h - 1;
	pnt[2] = x + w - 1;
	pnt[3] = y + h - 1;
	pnt[4] = x + w - 1;
	pnt[5] = y; // + PW;
	v_pline(v->handle, 3, pnt);
}

static void _cdecl
xa_draw_texture(struct xa_vdi_settings *v, XAMFDB *msrc, RECT *r, RECT *anch)
{
	short pnt[8];
	short x, y, w, h, sy, sh, dy, dh, width, height;
	MFDB mscreen;

	mscreen.fd_addr = NULL;

	x = (long)(r->x - anch->x) % msrc->d_w;
	w = msrc->d_w - x;
	sy = (long)(r->y - anch->y) % msrc->d_h;
	sh = msrc->d_h - sy;
	dy = r->y;
	dh = r->h;

	width = r->w;
	while (width > 0)
	{
		pnt[0] = x;
		pnt[4] = r->x;

		width -= w;
		if (width <= 0)
			w += width;
		else
		{
			r->x += w;
			x = 0;
		}
		pnt[2] = pnt[0] + w - 1;
		pnt[6] = pnt[4] + w - 1;

		y = sy;
		h = sh;
		r->y = dy;
		r->h = dh;
		height = r->h;
		while (height > 0)
		{
			//pnt[0] = x;
			pnt[1] = y;
			//pnt[4] = r.x;
			pnt[5] = r->y;

			height -= h;
			if (height <= 0)
				h += height;
			else
			{
				r->y += h;
				y = 0;
			}

			pnt[3] = pnt[1] + h - 1;

			pnt[7] = pnt[5] + h - 1;

			vro_cpyfm(v->handle, S_ONLY, pnt, &msrc->mfdb, &mscreen);

			h = msrc->d_h;
		}
		w = msrc->d_w;
	}
}
#if 0
/* HR: 1 (good) set of routines for screen saving */
inline long
calc_back(const RECT *r, short planes)
{
	return 2L * planes
		  * ((r->w + 15) >> 4)
		  * r->h;
}
#endif

static void _cdecl
xa_form_save(short d, RECT r, void **area)
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
		if( screen.planes == 8 )
		{
			short xd = r.x & 0x000f;
			r.x &= ~0x000f;		/* set x to word-boundary (fVDI)*/
			r.w += xd;
		}

		rtopxy(pnt, &r);
		ritopxy(pnt + 4, 0, 0, r.w, r.h);

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
		//else
	}
}

static void _cdecl
xa_form_restore(short d, RECT r, void **area)
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
			if( screen.planes == 8 )
			{
				short xd = r.x & 0x000f;
				r.x &= ~0x000f;		/* set x to word-boundary (fVDI) */
				r.w += xd;
			}

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

static void _cdecl
xa_form_copy(const RECT *fr, const RECT *to)
{
	MFDB Mscreen = { 0 };
	short pnt[8];
	rtopxy(pnt, fr);
	rtopxy(pnt + 4, to);
	vro_cpyfm(C.P_handle, S_ONLY, pnt, &Mscreen, &Mscreen);
}

static struct xa_vdi_api vdiapi =
{
	xa_wr_mode,
	xa_load_fonts,
	xa_unload_fonts,

	xa_set_clip,
	xa_clear_clip,
	xa_restore_clip,
	xa_save_clip,

	xa_line,
	xa_l_color,
	xa_l_type,
	xa_l_udsty,
	xa_l_ends,
	xa_l_width,

	xa_t_color,
	xa_t_effects,
	xa_t_font,
	xa_t_alignment,
	xa_t_extent,
	xa_text_extent,

	xa_f_color,
	xa_f_interior,
	xa_f_style,
	xa_f_perimeter,
	xa_draw_texture,

	xa_box,
	xa_gbox,
	xa_rgbox,
	xa_bar,
	xa_gbar,
	xa_p_gbar,

	xa_top_line,
	xa_bottom_line,
	xa_left_line,
	xa_right_line,
	xa_tl_hook,
	xa_br_hook,

	xa_write_disable,

	xa_prop_clipped_name,
	xa_wtxt_output,

	xa_form_save,
	xa_form_restore,
	xa_form_copy,

	r2pxy,
	rtopxy,
	ri2pxy,
	ritopxy,
#if WITH_GRADIENTS
	create_gradient,
#endif
};

struct xa_vdi_api *
init_xavdi_module(void)
{
	return &vdiapi;
}

