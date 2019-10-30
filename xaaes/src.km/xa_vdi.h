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

#ifndef _xa_vdi_h
#define _xa_vdi_h

#include "xa_types.h"

struct xa_vdi_settings
{
	struct	xa_vdi_api *api;

	short	handle;

	short	wr_mode;

	RECT	clip;
	RECT	screen;

	short	line_color;
	short	line_style;
	short	line_udsty;
	short	line_beg;
	short	line_end;
	short	line_width;

	short	fill_color;
	short	fill_interior;
	short	fill_style;
	short	fill_perimeter;

	short	fonts_loaded;
	short	text_color;
	short	text_effects;
	short	font_rid, font_sid;		/* Requested and real font ID */
	short	font_rsize, font_ssize;		/* Requested and real font size */
	short	font_w, font_h;
	short	cell_w, cell_h;
	short	first_ade, last_ade;
	short	max_w;
	short	dists[6];
	short	efx[3];
};

struct xa_mfdb
{
	short d_w;
	short d_h;
	MFDB mfdb;
};
typedef struct xa_mfdb XAMFDB;

struct xa_fnt_info
{
	long		f;	/* Font ID */
	short		p;	/* Point size */
	unsigned short	flags;
	short		wrm;
	short		e;	/* Effects */
	short		fgc;	/* Foreground colour */
	short		bgc;	/* Background colour (used to obtain 3-d effect) */
	short		bannercol;

	short		x_3dact;
	short		y_3dact;
	struct		xa_wtexture *texture;
	void		*misc;
};

#define WTXT_ACT3D	0x0001
#define WTXT_DRAW3D	0x0002
#define WTXT_CENTER	0x0004
#define WTXT_NOCLIP	0x0008
#define WTXT_DRAWBNR	0x0010
struct xa_wtxt_inf
{

	short	flags;	/* Flags */
	struct xa_fnt_info n;	/* normal */
	struct xa_fnt_info s;	/* selected */
	struct xa_fnt_info h;	/* highlighted? */
};

struct xa_vdi_api
{
	void _cdecl (*wr_mode)		(struct xa_vdi_settings *v, short m);
	void _cdecl (*load_fonts)	(struct xa_vdi_settings *v);
	void _cdecl (*unload_fonts)	(struct xa_vdi_settings *v);

	void _cdecl (*set_clip)		(struct xa_vdi_settings *v, const RECT *clip);
	void _cdecl (*clear_clip)	(struct xa_vdi_settings *v);
	void _cdecl (*restore_clip)	(struct xa_vdi_settings *v, const RECT *s);
	void _cdecl (*save_clip)	(struct xa_vdi_settings *v, RECT *s);

	void _cdecl (*line)			(struct xa_vdi_settings *v, short x1, short y1, short x2, short y2, short col);
	void _cdecl (*l_color)		(struct xa_vdi_settings *v, short col);
	void _cdecl (*l_type)		(struct xa_vdi_settings *v, short type);
	void _cdecl (*l_udsty)		(struct xa_vdi_settings *v, unsigned short ty);
	void _cdecl (*l_ends)		(struct xa_vdi_settings *v, short s, short e);
	void _cdecl (*l_width)		(struct xa_vdi_settings *v, short w);

	void _cdecl (*t_color)		(struct xa_vdi_settings *v, short col);
	void _cdecl (*t_effects)	(struct xa_vdi_settings *v, short efx);
	void _cdecl (*t_font)		(struct xa_vdi_settings *v, short point, short id);
	void _cdecl (*t_alignment)	(struct xa_vdi_settings *v, short halign, short valign);
	void _cdecl (*t_extent)		(struct xa_vdi_settings *v, const char *t, short *w, short *h);
	void _cdecl (*text_extent)	(struct xa_vdi_settings *v, const char *t, struct xa_fnt_info *f, short *w, short *h);

	void _cdecl (*f_color)		(struct xa_vdi_settings *v, short col);
	void _cdecl (*f_interior)	(struct xa_vdi_settings *v, short m);
	void _cdecl (*f_style)		(struct xa_vdi_settings *v, short m);
	void _cdecl (*f_perimeter)	(struct xa_vdi_settings *v, short m);
	void _cdecl (*draw_texture)	(struct xa_vdi_settings *v, struct xa_mfdb *texture, RECT *r, RECT *anchor);

	void _cdecl (*box)			(struct xa_vdi_settings *v, short d, short x, short y, short w, short h);
	void _cdecl (*gbox)			(struct xa_vdi_settings *v, short d, const RECT *r);
	void _cdecl (*rgbox)		(struct xa_vdi_settings *v, short d, short rnd, const RECT *r);
	void _cdecl (*bar)			(struct xa_vdi_settings *v, short d, short x, short y, short w, short h);
	void _cdecl (*gbar)			(struct xa_vdi_settings *v, short d, const RECT *r);
	void _cdecl (*p_gbar)		(struct xa_vdi_settings *v, short d, const RECT *r);

	void _cdecl (*top_line)		(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*bottom_line)	(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*left_line)	(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*right_line)	(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*tl_hook)		(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*br_hook)		(struct xa_vdi_settings *v, short d, const RECT *r, short col);

	void _cdecl (*write_disable)	(struct xa_vdi_settings *v, RECT *r, short colour);

	const char * _cdecl	(*prop_clipped_name)	(struct xa_vdi_settings *v, const char *s, char *d, int w, short *ret_w, short *ret_h, short method);
	void _cdecl		(*wtxt_output)		(struct xa_vdi_settings *v, struct xa_wtxt_inf *wtxti, char *txt, short state, const RECT *r, short xoff, short yoff);

	void _cdecl (*form_save)	(short d, RECT r, void **area);
	void _cdecl (*form_restore)	(short d, RECT r, void **area);
	void _cdecl (*form_copy)	(const RECT *from, const RECT *to);

	void _cdecl (*r2pxy)		(short *p, short d, const RECT *r);
	void _cdecl (*rtopxy)		(short *p, const RECT *r);
	void _cdecl (*ri2pxy)		(short *p, short d, short x, short y, short w, short h);
	void _cdecl (*ritopxy)		(short *p, short x, short y, short w, short h);
#if WITH_GRADIENTS
	void _cdecl (*create_gradient)	(struct xa_mfdb *pm, struct rgb_1000 *c, short method, short n_steps, short *steps, short w, short h );
#endif
};

struct xa_vdi_api * init_xavdi_module(void);

#endif /* _xa_vdi_h */
