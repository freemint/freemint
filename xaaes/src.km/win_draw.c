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

#include "xa_types.h"
#include "xaaeswdg.h"
#include "win_draw.h"
#include "rectlist.h"
#include "gradients.h"

extern struct config cfg;


#define MONO (scrninf->colours < 16)

static const struct xa_module_api *api;
static const struct xa_screen *scrninf;

static struct widget_theme *current_theme;
static struct widget_theme *current_pu_theme;
static struct widget_theme *current_slist_theme;

static short widg_w;
static short widg_h;

struct widg_texture
{
	struct xa_data_hdr h;
	struct xa_wtexture t;
	XAMFDB xamfdb;
};

struct module
{
	struct widget_tree wwt;
	struct xa_data_hdr *allocs;
};

static bool use_gradients = true;

static FreePriv free_priv_gradients;

static	WidgGemColor	get_wcol, set_wcol;

static	DrawWidg	d_unused, d_borders, d_title, d_wcontext, d_wappicn, d_closer, d_fuller, d_info,
			d_sizer, d_uparrow, d_dnarrow, d_vslide,
			d_lfarrow, d_rtarrow, d_hslide, d_iconifier, d_hider;

static	SetWidgSize	s_title_size, s_wcontext_size, s_wappicn_size, s_closer_size, s_fuller_size, s_info_size,
			s_sizer_size, s_uparrow_size, s_dnarrow_size, s_vslide_size,
			s_lfarrow_size, s_rtarrow_size, s_hslide_size,
			s_iconifier_size, s_hider_size, s_menu_size;

static DrawCanvas	/*draw_canvas,*/ draw_pu_canvas;
static DrawFrame	d_waframe;


/* ------------------------------------------------------------------------- */
/* ----------  Normal client window theme ---------------------------------- */
/* ------------------------------------------------------------------------- */
struct nwidget_row def_layout[];

static XA_WIND_ATTR name_dep[] =
{
	(CLOSER|FULLER|ICONIFIER|HIDE), -1,
	0
};
static XA_WIND_ATTR sizer_dep[] =
{
	(UPARROW|DNARROW|VSLIDE), (LFARROW|RTARROW|HSLIDE),
	0
};
static XA_WIND_ATTR vslide_dep[] =
{
	(UPARROW|DNARROW|SIZER), -1,
	0
};
static XA_WIND_ATTR hslide_dep[] =
{
	(LFARROW|RTARROW), -1,
	0
};

static XA_WIND_ATTR border_dep[] =
{
	(SIZER|MOVER), -1,
	0
};

static struct widget_theme def_theme =
{
	{ 0 },		/* xa_data_hdr */
	0L,		/* Links */

	&def_layout[0],

	get_wcol,
	set_wcol,

	{2, 2, 2, 2},	/* min */
	{0, 0, 0, 0},
	NULL, /* draw_canvas, */
	d_waframe,

	{ 0,		NULL,		0,		0,			d_unused, NULL,NULL,{ 0 }},			/* exterior		*/
	{ BORDER,	border_dep,	XAW_BORDER,	NO,			d_borders, NULL,NULL,{ 0 }},			/* border		*/
	{ NAME,		name_dep,	XAW_TITLE,	LT | R_VARIABLE,	d_title, s_title_size,free_priv_gradients,{ 0 }},	/* title		*/
	{ WCONTEXT,	NULL,		XAW_WCONTEXT,	LT,			d_wcontext, s_wcontext_size,free_priv_gradients,{ 0 }},	/* closer		*/
	{ WAPPICN,	NULL,		XAW_WAPPICN,	LT,			d_wappicn, s_wappicn_size,free_priv_gradients,{ 0 }},	/* closer		*/
	{ CLOSER,	NULL,		XAW_CLOSE,	LT,			d_closer, s_closer_size,free_priv_gradients,{ 0 }},	/* closer		*/
	{ FULLER,	NULL,		XAW_FULL,	RT,			d_fuller, s_fuller_size,free_priv_gradients,{ 0 }},	/* fuller		*/
	{ INFO,		NULL,		XAW_INFO,	LT | R_VARIABLE,	d_info, s_info_size,free_priv_gradients,{ 0 }},		/* info line		*/
	{ SIZER,	sizer_dep,	XAW_RESIZE,	RB,			d_sizer, s_sizer_size,free_priv_gradients,{ 0 }},	/* resizer		*/
	{ UPARROW,	NULL,		XAW_UPLN,	RT,			d_uparrow, s_uparrow_size,free_priv_gradients,{ 0 }},	/* up arrow		*/
	{ UPARROW1,	NULL,		XAW_UPLN1,	LT,			d_uparrow, s_uparrow_size,free_priv_gradients,{ 0 }},	/* up arrow		*/
	{ DNARROW,	NULL,		XAW_DNLN,	RT,			d_dnarrow, s_dnarrow_size,free_priv_gradients,{ 0 }},	/* down arrow		*/
	{ VSLIDE,	vslide_dep,	XAW_VSLIDE,	LT | R_VARIABLE,	d_vslide, s_vslide_size,free_priv_gradients,{ 0 }},	/* vertical slider	*/
	{ LFARROW,	NULL,		XAW_LFLN,	RT,			d_lfarrow, s_lfarrow_size,free_priv_gradients,{ 0 }},	/* left arrow		*/
	{ LFARROW1,	NULL,		XAW_LFLN1,	LT,			d_lfarrow, s_lfarrow_size,free_priv_gradients,{ 0 }},	/* left arrow		*/
	{ RTARROW,	NULL,		XAW_RTLN,	RT,			d_rtarrow, s_rtarrow_size,free_priv_gradients,{ 0 }},	/* right arrow		*/
	{ HSLIDE,	hslide_dep,	XAW_HSLIDE,	LT | R_VARIABLE,	d_hslide, s_hslide_size,free_priv_gradients,{ 0 }},	/* horizontal slider	*/
	{ ICONIFIER,	NULL,		XAW_ICONIFY,	RT,			d_iconifier, s_iconifier_size,free_priv_gradients,{ 0 }},/* iconifier		*/
	{ HIDER,	NULL,		XAW_HIDE,	RT,			d_hider, s_hider_size,free_priv_gradients,{ 0 }},	/* hider		*/
	{ 0,		NULL,		XAW_TOOLBAR,	0,			NULL, NULL,NULL,{ 0 }},					/* Toolbar		*/
	{ XaMENU,	NULL,		XAW_MENU,	LT | R_VARIABLE,	NULL, s_menu_size,NULL,{ 0 }},				/* menu bar		*/

	NULL,								/* handle (module ptr)	*/
};

struct render_widget *row1[] =
{
	&def_theme.closer,
	&def_theme.title,
	&def_theme.hider,
	&def_theme.iconifier,
	&def_theme.fuller,
	NULL,
};
struct render_widget *row2[] =
{
	&def_theme.info,
	NULL,
};
struct render_widget *row3[] =
{
	&def_theme.menu,
	NULL,
};
struct render_widget *row4[] =
{
	&def_theme.uparrow1,
	&def_theme.vslide,
	&def_theme.dnarrow,
	&def_theme.uparrow,
	&def_theme.sizer,
	NULL,
};
struct render_widget *row5[] =
{
	&def_theme.lfarrow1,
	&def_theme.hslide,
	&def_theme.lfarrow,
	&def_theme.rtarrow,
	NULL,
};

struct nwidget_row def_layout[] =
{
	{(XAR_START),			(NAME | CLOSER | FULLER | ICONIFIER | HIDE), row1},
#if 0
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER), row4},
#endif
	{(XAR_START),			INFO, row2},
	{(XAR_START),			XaMENU, row3},
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | UPARROW1 | DNARROW | SIZER), row4},
	{(XAR_END),			(LFARROW | HSLIDE | LFARROW1 | RTARROW), row5},
	{0, -1, NULL},
};
/* ------------------------------------------------------------------------- */
/* ----------  Popup window theme ---------------------------------- */
/* ------------------------------------------------------------------------- */
struct nwidget_row pu_def_layout[];

static XA_WIND_ATTR pu_name_dep[] =
{
	(CLOSER|FULLER|ICONIFIER|HIDE), -1,
	0
};
static XA_WIND_ATTR pu_sizer_dep[] =
{
	(UPARROW|DNARROW|VSLIDE), (LFARROW|RTARROW|HSLIDE),
	0
};
static XA_WIND_ATTR pu_vslide_dep[] =
{
	(UPARROW|DNARROW|SIZER), -1,
	0
};
static XA_WIND_ATTR pu_hslide_dep[] =
{
	(LFARROW|RTARROW), -1,
	0
};

static XA_WIND_ATTR pu_border_dep[] =
{
	(SIZER|MOVER), -1,
	0
};

static struct widget_theme pu_def_theme =
{
	{ 0 },		/* xa_data_hdr */
	0L,		/* Links */

	&pu_def_layout[0],

	get_wcol,
	set_wcol,

	{2, 2, 2, 2},	/* min */
#if 0
	{1, 1, 2, 2},
#endif
	{0, 0, 0, 0},

	draw_pu_canvas,
	d_waframe,

	{ 0,		NULL,		0,		0,			d_unused, NULL},			/* exterior		*/
	{ BORDER,	pu_border_dep,	XAW_BORDER,	NO,			d_borders, NULL},			/* border		*/
	{ NAME,		pu_name_dep,	XAW_TITLE,	LT | R_VARIABLE,	d_title, s_title_size},			/* title		*/
	{ WCONTEXT,	NULL,		XAW_WCONTEXT,	LT,			d_wcontext, s_wcontext_size},	/* closer		*/
	{ WAPPICN,	NULL,		XAW_WAPPICN,	LT,			d_wappicn, s_wappicn_size},	/* closer		*/
	{ CLOSER,	NULL,		XAW_CLOSE,	LT,			d_closer, s_closer_size},		/* closer		*/
	{ FULLER,	NULL,		XAW_FULL,	RT,			d_fuller, s_fuller_size},		/* fuller		*/
	{ INFO,		NULL,		XAW_INFO,	LT | R_VARIABLE,	d_info, s_info_size},			/* info line		*/
	{ SIZER,	pu_sizer_dep,	XAW_RESIZE,	RB,			d_sizer, s_sizer_size},			/* resizer		*/
	{ UPARROW,	NULL,		XAW_UPLN,	RT,			d_uparrow, s_uparrow_size},		/* up arrow		*/
	{ UPARROW1,	NULL,		XAW_UPLN1,	LT,			d_uparrow, s_uparrow_size},
	{ DNARROW,	NULL,		XAW_DNLN,	RT,			d_dnarrow, s_dnarrow_size},		/* down arrow		*/
	{ VSLIDE,	pu_vslide_dep,	XAW_VSLIDE,	LT | R_VARIABLE,	d_vslide, s_vslide_size},		/* vertical slider	*/
	{ LFARROW,	NULL,		XAW_LFLN,	RT,			d_lfarrow, s_lfarrow_size},		/* left arrow		*/
	{ LFARROW1,	NULL,		XAW_LFLN1,	LT,			d_lfarrow, s_lfarrow_size},
	{ RTARROW,	NULL,		XAW_RTLN,	RT,			d_rtarrow, s_rtarrow_size},		/* right arrow		*/
	{ HSLIDE,	pu_hslide_dep,	XAW_HSLIDE,	LT | R_VARIABLE,	d_hslide, s_hslide_size},		/* horizontal slider	*/
	{ ICONIFIER,	NULL,		XAW_ICONIFY,	RT,			d_iconifier, s_iconifier_size},		/* iconifier		*/
	{ HIDER,	NULL,		XAW_HIDE,	RT,			d_hider, s_hider_size},			/* hider		*/
	{ 0,		NULL,		XAW_TOOLBAR,	0,			NULL, NULL},				/* Toolbar		*/
	{ XaMENU,	NULL,		XAW_MENU,	LT | R_VARIABLE,	NULL, s_menu_size },			/* menu bar		*/

	NULL,								/* handle (module ptr)	*/
};

struct render_widget *pu_row1[] =
{
	&pu_def_theme.closer,
	&pu_def_theme.title,
	&pu_def_theme.hider,
	&pu_def_theme.iconifier,
	&pu_def_theme.fuller,
	NULL,
};
struct render_widget *pu_row2[] =
{
	&pu_def_theme.info,
	NULL,
};
struct render_widget *pu_row3[] =
{
	&pu_def_theme.menu,
	NULL,
};
struct render_widget *pu_row4[] =
{
	&pu_def_theme.uparrow1,
	&pu_def_theme.vslide,
	&pu_def_theme.dnarrow,
	&pu_def_theme.uparrow,
	&pu_def_theme.sizer,
	NULL,
};
struct render_widget *pu_row5[] =
{
	&pu_def_theme.lfarrow1,
	&pu_def_theme.hslide,
	&pu_def_theme.lfarrow,
	&pu_def_theme.rtarrow,
	NULL,
};

struct nwidget_row pu_def_layout[] =
{
	{(XAR_START),			(NAME | CLOSER | FULLER | ICONIFIER | HIDE), pu_row1},
#if 0
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER), pu_row4},
#endif
	{(XAR_START),			INFO, pu_row2},
	{(XAR_START),			XaMENU, pu_row3},
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | UPARROW1 | DNARROW | SIZER), pu_row4},
	{(XAR_END),			(LFARROW | HSLIDE | LFARROW1 | RTARROW), pu_row5},
	{0, -1, NULL},
};
/* ------------------------------------------------------------------------- */
/* ----------  Scroll-list window theme ---------------------------------- */
/* ------------------------------------------------------------------------- */
struct nwidget_row sl_def_layout[];

static XA_WIND_ATTR sl_name_dep[] =
{
	(CLOSER|FULLER|ICONIFIER|HIDE), -1,
	0
};
static XA_WIND_ATTR sl_sizer_dep[] =
{
	(UPARROW|DNARROW|VSLIDE), (LFARROW|RTARROW|HSLIDE),
	0
};
static XA_WIND_ATTR sl_vslide_dep[] =
{
	(UPARROW|DNARROW|SIZER), -1,
	0
};
static XA_WIND_ATTR sl_hslide_dep[] =
{
	(LFARROW|RTARROW), -1,
	0
};
static XA_WIND_ATTR sl_border_dep[] =
{
	(SIZER|MOVER), -1,
	0
};

static struct widget_theme sl_def_theme =
{
	{ 0 },		/* xa_data_hdr */
	0L,		/* Links */

	&sl_def_layout[0],

	get_wcol,
	set_wcol,

	{2, 2, 2, 2},	/* min */
#if 0
	{1, 1, 2, 2},
#endif
	{0, 0, 0, 0},
	NULL, /* draw_canvas */
	d_waframe,

	{ 0,		NULL,		0,		0,			d_unused, NULL,NULL,{ 0 }},		/* exterior		*/
	{ BORDER,	sl_border_dep,	XAW_BORDER,	NO,			d_borders, NULL,NULL,{ 0 }},		/* border		*/
	{ NAME,		sl_name_dep,	XAW_TITLE,	LT | R_VARIABLE,	d_title, s_title_size,free_priv_gradients,{ 0 }},	/* title		*/
	{ WCONTEXT,	NULL,		XAW_WCONTEXT,	LT,			d_wcontext, s_wcontext_size},	/* closer		*/
	{ WAPPICN,	NULL,		XAW_WAPPICN,	LT,			d_wappicn, s_wappicn_size},	/* closer		*/
	{ CLOSER,	NULL,		XAW_CLOSE,	LT,			d_closer, s_closer_size,free_priv_gradients,{ 0 }},	/* closer		*/
	{ FULLER,	NULL,		XAW_FULL,	RT,			d_fuller, s_fuller_size,free_priv_gradients,{ 0 }},	/* fuller		*/
	{ INFO,		NULL,		XAW_INFO,	LT | R_VARIABLE,	d_info, s_info_size,free_priv_gradients,{ 0 }},	/* info line		*/
	{ SIZER,	sl_sizer_dep,	XAW_RESIZE,	RB,			d_sizer, s_sizer_size,free_priv_gradients,{ 0 }},	/* resizer		*/
	{ UPARROW,	NULL,		XAW_UPLN,	RT,			d_uparrow, s_uparrow_size,free_priv_gradients,{ 0 }},	/* up arrow		*/
	{ UPARROW1,	NULL,		XAW_UPLN1,	LT,			d_uparrow, s_uparrow_size,free_priv_gradients,{ 0 }},	/* up arrow		*/
	{ DNARROW,	NULL,		XAW_DNLN,	RT,			d_dnarrow, s_dnarrow_size,free_priv_gradients,{ 0 }},	/* down arrow		*/
	{ VSLIDE,	sl_vslide_dep,	XAW_VSLIDE,	LT | R_VARIABLE,	d_vslide, s_vslide_size,free_priv_gradients,{ 0 }},	/* vertical slider	*/
	{ LFARROW,	NULL,		XAW_LFLN,	RT,			d_lfarrow, s_lfarrow_size,free_priv_gradients,{ 0 }},	/* left arrow		*/
	{ LFARROW1,	NULL,		XAW_LFLN1,	LT,			d_lfarrow, s_lfarrow_size,free_priv_gradients,{ 0 }},	/* left arrow		*/
	{ RTARROW,	NULL,		XAW_RTLN,	RT,			d_rtarrow, s_rtarrow_size,free_priv_gradients,{ 0 }},	/* right arrow		*/
	{ HSLIDE,	sl_hslide_dep,	XAW_HSLIDE,	LT | R_VARIABLE,	d_hslide, s_hslide_size,free_priv_gradients,{ 0 }},	/* horizontal slider	*/
	{ ICONIFIER,	NULL,		XAW_ICONIFY,	RT,			d_iconifier, s_iconifier_size,free_priv_gradients,{ 0 }},/* iconifier		*/
	{ HIDER,	NULL,		XAW_HIDE,	RT,			d_hider, s_hider_size,free_priv_gradients,{ 0 }},	/* hider		*/
	{ 0,		NULL,		XAW_TOOLBAR,	0,			NULL, NULL,NULL,{ 0 }},			/* Toolbar		*/
	{ XaMENU,	NULL,		XAW_MENU,	LT | R_VARIABLE,	NULL, s_menu_size,NULL,{ 0 }},		/* menu bar		*/

	NULL,								/* handle (module ptr)	*/
};

struct render_widget *sl_row1[] =
{
	&sl_def_theme.closer,
	&sl_def_theme.title,
	&sl_def_theme.hider,
	&sl_def_theme.iconifier,
	&sl_def_theme.fuller,
	NULL,
};
struct render_widget *sl_row2[] =
{
	&sl_def_theme.info,
	NULL,
};
struct render_widget *sl_row3[] =
{
	&sl_def_theme.menu,
	NULL,
};
struct render_widget *sl_row4[] =
{
	&sl_def_theme.uparrow1,
	&sl_def_theme.vslide,
	&sl_def_theme.dnarrow,
	&sl_def_theme.uparrow,
	&sl_def_theme.sizer,
	NULL,
};
struct render_widget *sl_row5[] =
{
	&sl_def_theme.lfarrow1,
	&sl_def_theme.hslide,
	&sl_def_theme.lfarrow,
	&sl_def_theme.rtarrow,
	NULL,
};

struct nwidget_row sl_def_layout[] =
{
	{(XAR_START),			(NAME | CLOSER | FULLER | ICONIFIER | HIDE), sl_row1},
#if 0
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER), row4},
#endif
	{(XAR_START),			INFO, sl_row2},
	{(XAR_START),			XaMENU, sl_row3},
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | UPARROW1 | DNARROW | SIZER), sl_row4},
	{(XAR_END),			(LFARROW | HSLIDE | LFARROW1 | RTARROW), sl_row5},
	{0, -1, NULL},
};
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
#ifndef ST_ONLY
static void set_texture(struct module *m, struct xa_wcol_inf *wcol, struct widg_texture *t);
#if WITH_GRADIENTS
static struct xa_wtexture * find_gradient(struct xa_vdi_settings *, struct xa_wcol *, bool, struct xa_data_hdr **, short w, short h);
#endif
#endif
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static struct widget_theme *
duplicate_theme(struct widget_theme *theme)
{
	struct widget_theme *new_theme;
	struct nwidget_row *rows, *new_rows;
	struct render_widget **wr, **new_wr;
	long n_rows = 1, n_widgets = 0, len;

	/*
	 * Count rows and widgets
	 */
	rows = theme->layout;

	while (rows->tp_mask != -1)
	{
		if ((wr = rows->w))
		{
			while (*wr)
			{
				n_widgets++;
				wr++;
			}
		}
		n_rows++;
		rows++;
	}

	n_widgets += n_rows;	/* Each array of render_widget's have a NULL terminator */

	len = ((sizeof(wr) * n_widgets)) + sizeof(*new_theme) + ((sizeof(*rows) * n_rows));

	new_theme = (*api->kmalloc)(len);

	if (new_theme)
	{
		*new_theme = *theme;
		new_rows 	= (struct nwidget_row *)    ((long)new_theme + sizeof(struct widget_theme));
		new_wr 		= (struct render_widget **) ((long)new_rows  + (sizeof(struct nwidget_row) * n_rows));

		new_theme->layout = new_rows;
		rows = theme->layout;

		while (rows->tp_mask != -1)
		{
			XA_WIND_ATTR utp, ntp;

			*new_rows = *rows;

			wr = rows->w;
			new_rows->w = new_wr;
			utp = rows->tp_mask;
			ntp = 0;
			while (utp && *wr)
			{
				if (utp & (*wr)->tp)
				{
					*new_wr = (struct render_widget *)((long)new_theme + ((long)*wr - (long)theme));
					utp &= ~((*wr)->tp);
					ntp |= (*wr)->tp;
					new_wr++;
				}
				wr++;
			}
			new_rows->tp_mask = ntp;
			*new_wr++ = NULL;
			rows++;
			new_rows++;
		}
		*new_rows = *rows;
	}
	return new_theme;
}

/*
 * Called by delete_xa_data()
 */
static void _cdecl
delete_theme(void *_theme)
{
	(*api->kfree)(_theme);
}


/*
 * Private structure holding color information. This structure is accessible
 * to the widget-drawer functions via the 'colours', 'ontop_cols' and 'untop_cols'
 * elements of xa_window structure.
 */
#define WCF_TOP		0x00000001

struct window_colours
{
	struct xa_data_hdr	h;

	unsigned long		flags;

	short 			waframe_col;
	short 			frame_col;

	struct xa_wcol_inf	win;		/* Must be first */
	struct xa_wcol_inf	borders;

	struct xa_wcol_inf	hslider;
	struct xa_wcol_inf	hslide;
	struct xa_wcol_inf	vslider;
	struct xa_wcol_inf	vslide;

	struct xa_wcol_inf	title;

	struct xa_wcol_inf	info;

	struct xa_wcol_inf	closer;
	struct xa_wcol_inf	hider;
	struct xa_wcol_inf	iconifier;
	struct xa_wcol_inf	fuller;
	struct xa_wcol_inf	sizer;
	struct xa_wcol_inf	uparrow;
	struct xa_wcol_inf	dnarrow;
	struct xa_wcol_inf	lfarrow;
	struct xa_wcol_inf	rtarrow;	/* Must be last */

	struct xa_wtxt_inf	title_txt;
	struct xa_wtxt_inf	info_txt;
};

/* ---------------------------------------------------------------------------------- */
/* -----------  Standard client window colour theme --------------------------------- */
/* ---------------------------------------------------------------------------------- */
#if WITH_GRADIENTS
struct xa_gradient otop_vslide_gradient =
{
	NULL,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{400,400,500},{600,600,700}},
};
struct xa_gradient otop_hslide_gradient =
{
	NULL,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{400,400,500},{600,600,700}},
};
struct xa_gradient otop_vslider_gradient =
{
	NULL,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{900,900,1000},{600,600,700}},
};
struct xa_gradient otop_hslider_gradient =
{
	NULL,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{900,900,1000},{600,600,700}},
};

struct xa_gradient utop_vslide_gradient =
{
	NULL,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{300,300,300},{500,500,500}},
};
struct xa_gradient utop_hslide_gradient =
{
	NULL,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{300,300,300},{500,500,500}},
};
struct xa_gradient utop_vslider_gradient =
{
	NULL,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{800,800,800},{500,500,500}},
};
struct xa_gradient utop_hslider_gradient =
{
	NULL,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{800,800,800},{500,500,500}},
};

struct xa_gradient otop_title_gradient =
{
	NULL,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{700,700,900},{500,500,700}},
};
struct xa_gradient utop_title_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0,0,{0},
	{{600,600,600},{800,800,800}},
};

struct xa_gradient otop_info_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0,0,{0},
	{{800,800,850},{950,950,1000}},
#if 0
	3, 1, {3, 0},
	{{200,200,200},{600,600,600},{900,900,900}},
#endif
};
struct xa_gradient utop_info_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0, 0, {0},
	{{600,600,600},{800,800,800}},
};

struct xa_gradient otop_grey_gradient =
{
	NULL,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{600,600,700},{900,900,1000}},
};
struct xa_gradient utop_grey_gradient =
{
	NULL,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{500,500,500},{800,800,800}},
/*	{{500,500,500},{900,900,900}}, */
};
#endif

struct window_colours def_otop_cols =
{
 { 0 },	/* data header */
 WCF_TOP,

 G_LBLACK, /* window workarea frame color */
 G_LBLACK, /* window frame color */
/*          flags                        wr_mode     color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
  { WCOL_DRAWBKG/*|WCOL_BOXED*/, MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/
  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
        {G_CYAN,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_CYAN, NULL},	/* Normal */
        {G_CYAN,   FIS_SOLID,   8,   G_BLACK,     1,    G_CYAN, G_WHITE, NULL},		/* Selected */
        {G_BLUE,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},	/* Highlighted */
/* Horizontal Slider */
  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_hslider_gradient) },	/* Normal */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&otop_hslider_gradient) },	/* Selected */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&otop_hslider_gradient) }}, /* Highlighted */
 /* Horizontal Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_hslide_gradient) },	/* Normal */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&otop_hslide_gradient) },	/* Selected */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_hslide_gradient) }}, /* Highlighted */
 /* Vertical Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_vslider_gradient) },	/* Normal */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&otop_vslider_gradient) },	/* Selected */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&otop_vslider_gradient) }}, /* Highlighted */
 /* Vertical Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_vslide_gradient) },	/* Normal */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&otop_vslide_gradient) },	/* Selected */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_vslide_gradient) }}, /* Highlighted */
/* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED|WCOL_GRADIENT, MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) },	/* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&otop_title_gradient) },	/* Selected */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,      1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) }}, /* Highlighted */
 /* Info */
 { WCOL_DRAW3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&otop_info_gradient) },	/* Normal */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&otop_info_gradient) },	/* Selected */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&otop_info_gradient) }}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_title_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) }}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_title_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) }}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_title_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) }}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_title_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_title_gradient) }}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) }}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) }}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) }}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) }}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&otop_grey_gradient) }}, /* Highlighted */
 /* Title text-info */
	 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
     /* id  pnts                 flags wr_mode   efx     fg       bg       banner   x_3dact y_3dact texture */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_WHITE, G_BLUE,  G_WHITE, 1,      1,  NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_WHITE, G_BLUE,  G_WHITE, 1,      1,  NULL},  /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_WHITE, G_WHITE, G_WHITE, 1,      1,  NULL}}, /* Highlighted */
     /* Info text-info */
     { 0,
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK, G_WHITE, G_WHITE, 1,      1,  NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK, G_WHITE, G_WHITE, 1,      1,  NULL}, /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK, G_WHITE, G_WHITE, 1,      1,  NULL}}, /* Highlighted */
};

struct window_colours def_utop_cols =
{
 { 0 },	/* data header */
 0,

 G_LBLACK,	/* Window work area frame color */
 G_LBLACK,	/* window frame color */
/*          flags                        wr_mode      color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 /* Window color (when unused window exterior is drawn) */
 { WCOL_DRAWBKG/*|WCOL_BOXED*/,                      MD_REPLACE,
          {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
          {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
          {G_LWHITE, FIS_SOLID,   8,   G_BLACK,      1,    G_WHITE, G_LBLACK, NULL}},	/* window areas not covered by a widget/ unused widgets*/
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
	{G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LWHITE, NULL},	/* Normal */
	{G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_LWHITE, G_WHITE,  NULL},	/* Selected */
	{G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}},	/* Highlighted */
 /* H Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_hslider_gradient) },	/* Normal */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&utop_hslider_gradient) },	/* Selected */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&utop_hslider_gradient) }},	/* Highlighted */
 /* H Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_hslide_gradient) },	/* Normal */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_hslide_gradient) },	/* Selected */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_hslide_gradient) }}, /* Highlighted */
  /* V Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_vslider_gradient) },	/* Normal */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&utop_vslider_gradient) },	/* Selected */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&utop_vslider_gradient) }},	/* Highlighted */
 /* V Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_vslide_gradient) },	/* Normal */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_vslide_gradient) },	/* Selected */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_vslide_gradient) }}, /* Highlighted */
 /* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_GRADIENT, MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_title_gradient) },	/* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_title_gradient) },	/* Selected */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_title_gradient) }}, /* Highlighted */
 /* Info */
 { WCOL_DRAWBKG|WCOL_GRADIENT,		MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,    1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&utop_info_gradient) },	/* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,    1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&utop_info_gradient) },	/* Selected */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&utop_info_gradient) }}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&utop_grey_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&utop_grey_gradient) }}, /* Highlighted */

 /* Title text-info */
     { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
     /* id  pnts                 flags wr_mode   efx     fg        bg       banner  x_3dact y_3dact texture */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL},    /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL},    /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL}},   /* Highlighted */
 /* Info text-info */
     { 0,
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL}, /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL}}, /* Highlighted */
};

/* ---------------------------------------------------------------------------------- */
/* ---------------- Alert window colour theme --------------------------------------- */
/* ---------------------------------------------------------------------------------- */
#if WITH_GRADIENTS
struct xa_gradient alert_otop_title_gradient =
{
    NULL,
    -1, -1,
    0, 0,

    2, 0,{0},
    {{500,200,200},{1000,800,800}},
};
struct xa_gradient alert_utop_title_gradient =
{
    NULL,
    -1, -1,
    0, 0,

    2, 0, {0},
    {{500,500,500},{700,700,700}},
/*  {{500,500,500},{900,900,900}}, */
};

struct xa_gradient alert_utop_grey_gradient =
{
    NULL,
    -1, -1,
    0, 0,

    2, 0, {0},
    {{300,300,300},{700,700,700}},
/*  {{500,500,500},{900,900,900}}, */
};
#endif

struct window_colours alert_def_otop_cols =
{
 { 0 }, /* data header */
 WCF_TOP,

 G_LBLACK, /* window workarea frame color */
 G_LBLACK, /* window frame color */
/*          flags                        wr_mode     color       fill    fill    box       box        tl      br        */
/*                                                             interior  style  color    thickness  colour  colour      */
 { WCOL_DRAWBKG/*|WCOL_BOXED*/, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/

  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_CYAN,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_CYAN, NULL}, /* Normal */
                                                    {G_CYAN,   FIS_SOLID,   8,   G_BLACK,     1,    G_CYAN, G_WHITE, NULL},     /* Selected */
                                                    {G_BLUE,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},   /* Highlighted */
/* Horizontal Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},   /* Selected */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* Horizontal Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},    /* Selected */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */

 /* Vertical Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},   /* Selected */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* Vertical Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},    /* Selected */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
/* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED|WCOL_GRADIENT, MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_otop_title_gradient) }, /* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&alert_otop_title_gradient) }, /* Selected */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,      1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_otop_title_gradient) }}, /* Highlighted */
 /* Info */
 { WCOL_DRAW3D|WCOL_DRAWBKG|WCOL_GRADIENT, MD_REPLACE,
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(NULL) }, /* Normal */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(NULL) }, /* Selected */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_otop_title_gradient) },    /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&alert_otop_title_gradient) },    /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_otop_title_gradient) }}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },    /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },    /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
              {G_LWHITE,    FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */

/* flags                         fontId  size flags wr_mode   Effect      forground       background x_3dact y_3dact    */
/*                                              col           col       */
 /* Title text-info */
 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
     /* id  pnts                 flags wr_mode   efx       fg       bg       banner    x_3dact y_3dact texture */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,        G_RED,   G_WHITE, G_WHITE,  1,       1,     NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,        G_RED,   G_WHITE, G_WHITE,  1,       1,     NULL},    /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,        G_RED,   G_WHITE, G_WHITE,  1,       1,     NULL}},   /* Highlighted */
 /* Info text-info */
 { 0,
        {0, MEDIUM_FONT_POINT,   0,    MD_TRANS, 0,        G_BLACK, G_WHITE, G_WHITE,  1,       1,     NULL},   /* Normal */
        {0, MEDIUM_FONT_POINT,   0,    MD_TRANS, 0,        G_BLACK, G_WHITE, G_WHITE,  1,       1,     NULL},   /* Selected */
        {0, MEDIUM_FONT_POINT,   0,    MD_TRANS, 0,        G_BLACK, G_WHITE, G_WHITE,  1,       1,     NULL}},  /* Highlighted */
};

struct window_colours alert_def_utop_cols =
{
 { 0 }, /* data header */
 0,

 G_LBLACK,  /* Window work area frame color */
 G_LBLACK,  /* window frame color */
/*          flags                        wr_mode     color       fill    fill    box       box        tl      br        */
/*                                                             interior  style  color    thickness  colour  colour      */
 /* Window color (when unused window exterior is drawn) */
 { WCOL_DRAWBKG/*|WCOL_BOXED*/,                      MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}},  /* window areas not covered by a widget/ unused widgets*/

 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LWHITE, NULL},   /* Normal */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_LWHITE, G_WHITE,  NULL},  /* Selected */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}},  /* Highlighted */
 /* H Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},   /* Selected */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}},  /* Highlighted */
 /* H Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},   /* Selected */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
  /* V Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},   /* Selected */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}},  /* Highlighted */
 /* V Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},   /* Normal */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},   /* Selected */
                                                    {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_GRADIENT, MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_utop_title_gradient) },  /* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&alert_utop_title_gradient) },  /* Selected */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_utop_title_gradient) }}, /* Highlighted */
 /* Info */
 { WCOL_DRAWBKG|WCOL_GRADIENT,      MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,    1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(NULL) }, /* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,    1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(NULL) }, /* Selected */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_utop_grey_gradient) }, /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&alert_utop_grey_gradient) }, /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&alert_utop_grey_gradient) }}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) },  /* Normal */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(NULL) },  /* Selected */
             {G_LWHITE, FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(NULL) }}, /* Highlighted */

/* flags                         fontId  size wr_mode    Effect      forground       background */
/*                                         col            col       */
 /* Title text-info */
     { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
     /* id  pnts                 flags wr_mode   efx     fg        bg       banner   x_3dact y_3dact texture */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL}, /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL}}, /* Highlighted */
 /* Info text-info */
     { 0,
        {0, MEDIUM_FONT_POINT,   0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL},   /* Normal */
        {0, MEDIUM_FONT_POINT,   0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL},   /* Selected */
        {0, MEDIUM_FONT_POINT,   0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL}}, /* Highlighted */
};
/* ---------------------------------------------------------------------------------- */
/* ---------------- Scroll List window colour theme --------------------------------- */
/* ---------------------------------------------------------------------------------- */
#if WITH_GRADIENTS
struct xa_gradient slist_otop_vslide_gradient =
{
	NULL,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{300,300,300},{500,500,500}},
};
struct xa_gradient slist_otop_hslide_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0, 0, {0},
	{{300,300,300},{500,500,500}},
};
struct xa_gradient slist_otop_vslider_gradient =
{
	NULL,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{800,800,800},{500,500,500}},
};
struct xa_gradient slist_otop_hslider_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0, 0, {0},
	{{800,800,800},{500,500,500}},
};
struct xa_gradient slist_utop_vslide_gradient =
{
	NULL,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{300,300,300},{500,500,500}},
};
struct xa_gradient slist_utop_hslide_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0, 0, {0},
	{{300,300,300},{500,500,500}},
};
struct xa_gradient slist_utop_vslider_gradient =
{
	NULL,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{800,800,800},{500,500,500}},
};
struct xa_gradient slist_utop_hslider_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0, 0, {0},
	{{800,800,800},{500,500,500}},
};

struct xa_gradient slist_otop_title_gradient =
{
	NULL,
	-1, -1,
	0, 0,

	2, 0,{0},
	{{00,00,400},{600,600,1000}},
};
struct xa_gradient slist_utop_title_gradient =
{
	NULL,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{500,500,500},{700,700,700}},
};

struct xa_gradient slist_otop_info_gradient =
{
	NULL,
	0, -1,
	16, 0,

	3, 1, {3, 0},
	{{200,200,200},{600,600,600},{900,900,900}},
};
struct xa_gradient slist_utop_info_gradient =
{
	NULL,
	0, -1,
	16, 0,

	0, 0, {0},
	{{400,400,400},{700,700,700}},
};
struct xa_gradient slist_otop_grey_gradient =
{
	NULL,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{500,500,500},{900,900,900}},
};
struct xa_gradient slist_utop_grey_gradient =
{
	NULL,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{300,300,300},{700,700,700}},
/*	{{500,500,500},{900,900,900}}, */
};
#endif

#ifndef ST_ONLY
struct window_colours slist_def_otop_cols =
{
 { 0 },	/* data header */
 WCF_TOP,

 G_LBLACK, /* window workarea frame color */
 G_LBLACK, /* window frame color */
/*          flags                        wr_mode     color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 { WCOL_DRAWBKG/*|WCOL_BOXED*/, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/

  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_CYAN,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_CYAN, NULL},	/* Normal */
                                                    {G_CYAN,   FIS_SOLID,   8,   G_BLACK,     1,    G_CYAN, G_WHITE, NULL},		/* Selected */
                                                    {G_BLUE,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},	/* Highlighted */
/* Horizontal Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_hslider_gradient) },	/* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&slist_otop_hslider_gradient) },	/* Selected */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&slist_otop_hslider_gradient) }}, /* Highlighted */
 /* Horizontal Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
             {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_hslide_gradient) },	/* Normal */
             {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&slist_otop_hslide_gradient) },	/* Selected */
             {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_hslide_gradient) }}, /* Highlighted */
 /* Vertical Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_vslider_gradient) },	/* Normal */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&slist_otop_vslider_gradient) },	/* Selected */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&slist_otop_vslider_gradient) }}, /* Highlighted */
 /* Vertical Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
             {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_vslide_gradient) },	/* Normal */
             {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_vslide_gradient) },	/* Selected */
             {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_vslide_gradient) }}, /* Highlighted */
/* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED|WCOL_GRADIENT, MD_REPLACE,
             {G_BLUE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Normal */
             {G_LBLUE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Selected */
             {G_LWHITE, FIS_SOLID,   8,   G_BLACK,      1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) }}, /* Highlighted */
 /* Info */
 { WCOL_DRAW3D|WCOL_DRAWBKG|WCOL_GRADIENT, MD_REPLACE,
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&slist_otop_info_gradient) },	/* Normal */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&slist_otop_info_gradient) },	/* Selected */
             {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL XA_GRADIENT(&slist_otop_info_gradient) }}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Normal */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Selected */
             {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) }}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) }}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) }}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_title_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_title_gradient) }}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) }}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) }}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) }}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) }}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Normal */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_otop_grey_gradient) },	/* Selected */
              {G_LWHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_otop_grey_gradient) }}, /* Highlighted */

   /* Title text-info */
    { /*WTXT_DRAW3D|WTXT_ACT3D|*/WTXT_CENTER,
     /* id  pnts                 flags wr_mode   efx     fg       bg        banner    x_3dact y_3dact texture */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_WHITE, G_LWHITE, G_WHITE,  1,       1,     NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_WHITE, G_LWHITE, G_WHITE,  1,       1,     NULL},  /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_WHITE, G_LWHITE, G_WHITE,  1,       1,     NULL}}, /* Highlighted */
   /* Info text-info */
    { 0,
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK, G_WHITE,  G_WHITE,  1,       1,     NULL},   /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK, G_WHITE,  G_WHITE,  1,       1,     NULL},   /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK, G_WHITE,  G_WHITE,  1,       1,     NULL}},  /* Highlighted */
};

struct window_colours slist_def_utop_cols =
{
 { 0 },	/* data header */
 0,

 G_LBLACK,	/* Window work area frame color */
 G_LBLACK,	/* window frame color */
/*          flags                        wr_mode     color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 /* Window color (when unused window exterior is drawn) */
 { WCOL_DRAWBKG/*|WCOL_BOXED*/,                      MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}},	/* window areas not covered by a widget/ unused widgets*/

 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LWHITE, NULL},	/* Normal */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_LWHITE, G_WHITE,  NULL},	/* Selected */
                                                    {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}},	/* Highlighted */
 /* H Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_hslider_gradient) },	/* Normal */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&slist_utop_hslider_gradient) },	/* Selected */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&slist_utop_hslider_gradient) }},	/* Highlighted */
 /* H Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_hslide_gradient) },	/* Normal */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_hslide_gradient) },	/* Selected */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_hslide_gradient) }}, /* Highlighted */
  /* V Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_vslider_gradient) },	/* Normal */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL XA_GRADIENT(&slist_utop_vslider_gradient) },	/* Selected */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL XA_GRADIENT(&slist_utop_vslider_gradient) }},	/* Highlighted */
 /* V Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_vslide_gradient) },	/* Normal */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_vslide_gradient) },	/* Selected */
        {G_LBLACK, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_vslide_gradient) }}, /* Highlighted */
 /* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_GRADIENT, MD_REPLACE,
        {G_BLUE,   FIS_SOLID,   8,   G_LBLACK,    1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_title_gradient) },	/* Normal */
        {G_LBLUE,  FIS_SOLID,   8,   G_LBLACK,    1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_title_gradient) },	/* Selected */
        {G_LWHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_title_gradient) }}, /* Highlighted */
 /* Info */
 { WCOL_DRAWBKG|WCOL_GRADIENT,		MD_REPLACE,
        {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,    1,    G_LBLACK, G_BLACK, NULL XA_GRADIENT(&slist_utop_info_gradient) },	/* Normal */
        {G_LWHITE, FIS_SOLID,   8,   G_LBLACK,    1,    G_LBLACK, G_BLACK, NULL XA_GRADIENT(&slist_utop_info_gradient) },	/* Selected */
        {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_LBLACK, G_BLACK, NULL XA_GRADIENT(&slist_utop_info_gradient) }}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE,
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Normal */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL XA_GRADIENT(&slist_utop_grey_gradient) },	/* Selected */
        {G_LWHITE,	FIS_SOLID,  8,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL XA_GRADIENT(&slist_utop_grey_gradient) }}, /* Highlighted */

/* Title text-info */
     { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
     /* id  pnts                 flags wr_mode   efx     fg        bg       banner  x_3dact y_3dact texture */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_LBLACK, G_WHITE, G_WHITE, 1,      1,       NULL}, /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL}}, /* Highlighted */
/* Info text-info */
     { 0,
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL}, /* Normal */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL},  /* Selected */
        {0, STANDARD_FONT_POINT, 0,    MD_TRANS, 0,      G_BLACK,  G_WHITE, G_WHITE, 1,      1,       NULL}}, /* Highlighted */
};
#endif

/* ---------------------------------------------------------------------------------- */
/* ---------------- Monochrome color theme used by all window classes --------------- */
/* ---------------------------------------------------------------------------------- */
struct window_colours mono_def_otop_cols =
{
 { 0 },	/* data header, links */
 WCF_TOP,

 G_BLACK,	/* window workarea frame color */
 G_BLACK,	/* window frame color */
/*          flags                        wr_mode     color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 { WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/

  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,   FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},	/* Highlighted */
/* H Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* H Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
/* V Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* V Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK, FIS_PATTERN,   2,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK, FIS_PATTERN,   2,  G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK, FIS_PATTERN,   2,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Info */
 { /*WCOL_DRAW3D|*/WCOL_DRAWBKG|WCOL_BOXED,	MD_REPLACE,
                                                    {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Selected */
                                                    {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL}}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */

 /* Title text-info */
 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
     /* id  pnts                 flags wr_mode     efx    fg       bg       banner  x_3dact y_3dact texture */
        {1, STANDARD_FONT_POINT, 0,    MD_REPLACE, 0,     G_BLACK, G_WHITE, G_WHITE,  1,      1,      NULL},    /* Normal */
        {1, STANDARD_FONT_POINT, 0,    MD_REPLACE, 0,     G_BLACK, G_WHITE, G_WHITE,  1,      1,      NULL},    /* Selected */
        {1, STANDARD_FONT_POINT, 0,    MD_REPLACE, 0,     G_BLACK, G_WHITE, G_WHITE,  1,      1,      NULL}},   /* Highlighted */
 /* Info text-info */
 { 0,
        {1, MEDIUM_FONT_POINT,   0,    MD_REPLACE, 0,     G_BLACK, G_WHITE, G_WHITE,  1,      1,      NULL}, /* Normal */
        {1, MEDIUM_FONT_POINT,   0,    MD_REPLACE, 0,     G_BLACK, G_WHITE, G_WHITE,  1,      1,      NULL}, /* Selected */
        {1, MEDIUM_FONT_POINT,   0,    MD_REPLACE, 0,     G_BLACK, G_WHITE, G_WHITE,  1,      1,      NULL}},    /* Highlighted */
};

struct window_colours mono_def_utop_cols =
{
 { 0 },	/* data header, links */
 0,

 G_BLACK, /* window workarea frame color */
 G_BLACK, /* window frame color */
/*          flags                        wr_mode     color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 { WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/

  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK,   FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK,   FIS_SOLID,   8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},	/* Highlighted */
/* H Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* H Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
/* V Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* V Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK, FIS_PATTERN,  4,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK, FIS_PATTERN,   2,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK, FIS_PATTERN,   2,  G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK, FIS_PATTERN,   2,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Info */
 { /*WCOL_DRAW3D|*/WCOL_DRAWBKG|WCOL_BOXED,	MD_REPLACE,
                                                    {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Selected */
                                                    {G_WHITE,  FIS_SOLID,   8,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL}}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 8,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */

 /* Title text-info */
 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
     /* id  pnts                 flags wr_mode     efx        fg       bg       banner  x_3dact y_3dact texture */
        {1, STANDARD_FONT_POINT, 0,    MD_REPLACE, TXT_LIGHT, G_BLACK, G_WHITE, G_WHITE, 1,       1,    NULL}, /* Normal */
        {1, STANDARD_FONT_POINT, 0,    MD_REPLACE, TXT_LIGHT, G_BLACK, G_WHITE, G_WHITE, 1,       1,    NULL}, /* Selected */
        {1, STANDARD_FONT_POINT, 0,    MD_REPLACE, TXT_LIGHT, G_BLACK, G_WHITE, G_WHITE, 1,       1,    NULL}},    /* Highlighted */
 /* Info text-info */
 { 0,
        {1, MEDIUM_FONT_POINT,   0,    MD_REPLACE, TXT_LIGHT, G_BLACK, G_WHITE, G_WHITE, 1,       1,    NULL},  /* Normal */
        {1, MEDIUM_FONT_POINT,   0,    MD_REPLACE, TXT_LIGHT, G_BLACK, G_WHITE, G_WHITE, 1,       1,    NULL},  /* Selected */
        {1, MEDIUM_FONT_POINT,   0,    MD_REPLACE, TXT_LIGHT, G_BLACK, G_WHITE, G_WHITE, 1,       1,    NULL}}, /* Highlighted */
};

static inline long
pix_2_sl(long p, long s)
{
	/*
	 * Ozk:
	 *      Hmmm.... divide by Zero not handled correctly, at least
	 *	not on my Milan 040 where this crashed if 's' is 0
	 */
	if (s)
		return (p * SL_RANGE) / s;
	else
		return 0L;
}

static inline long
sl_2_pix(long s, long p)
{
	return ((s * p) + (SL_RANGE >> 2)) / SL_RANGE;
}

static void
draw_pu_canvas(struct xa_window *wind, GRECT *outer, GRECT *inner, const GRECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_vdi_api *vapi = v->api;

	if (outer->g_w || outer->g_h)
	{
		(*vapi->wr_mode)(v, MD_REPLACE);
		if (MONO)
		{
			(*vapi->br_hook)(v, 0, outer, G_BLACK);
			(*vapi->br_hook)(v, -1, outer, G_WHITE);

			(*vapi->tl_hook)(v, 0, outer, G_BLACK);
			(*vapi->tl_hook)(v, -1, outer, G_WHITE);
		}
		else
		{
			(*vapi->br_hook)(v, 0, outer, G_BLACK);
			(*vapi->br_hook)(v, -1, outer, G_LBLACK);

			(*vapi->tl_hook)(v, 0, outer, G_BLACK);
			(*vapi->tl_hook)(v, -1, outer, G_WHITE);
		}
	}
}
/*
 * Draw a window widget
 */
static void
draw_widg_box(struct xa_vdi_settings *v, short d, struct xa_wcol_inf *wcoli, struct xa_wtexture *t, short state, GRECT *wr, GRECT *anch)
{
	struct xa_wcol *wcol;
	struct xa_wtexture *wext;
	bool sel = state & OS_SELECTED;
	short f = wcoli->flags, o = 0;
	GRECT r = *wr;
	struct xa_vdi_api *vapi = v->api;

	(*vapi->wr_mode)(v, wcoli->wr_mode);

	if (sel)
		wcol = &wcoli->selected;
	else
		wcol = &wcoli->normal;

	wext = t ? t : wcol->texture;

	if (d)
	{
		r.g_x -= d;
		r.g_y -= d;
		r.g_w += d+d;
		r.g_h += d+d;
	}

	if (f & WCOL_BOXED)
	{
		int i = wcol->box_th;
		(*vapi->l_color)(v, wcol->box_c);
		while (i > 0)
			(*vapi->gbox)(v, o, &r), o--, i--;
	}

	if (f & WCOL_DRAW3D)
	{
		if (sel)
		{
			(*vapi->tl_hook)(v, o, &r, wcol->tlc);
			(*vapi->br_hook)(v, o, &r, wcol->brc);
		}
		else
		{
			(*vapi->br_hook)(v, o, &r, wcol->brc);
			(*vapi->tl_hook)(v, o, &r, wcol->tlc);
		}
		o -= 1;
	}

	if (!t && (f & WCOL_DRAWBKG))
	{
		(*vapi->f_interior)(v, wcol->i);
		if (wcol->i > 1)
			(*vapi->f_style)(v, wcol->f);

		(*vapi->f_color)(v, wcol->c);
		(*vapi->gbar)(v, o, &r);
	}

	if (wext)
	{
		r.g_x -= o;
		r.g_y -= o;
		r.g_w += o + o;
		r.g_h += o + o;
		(*vapi->draw_texture)(v, wext->body, &r, anch);
	}
}

static void
draw_widget_text(struct xa_vdi_settings *v, struct xa_widget *widg, struct xa_wtxt_inf *wtxti, char *txt, short xoff, short yoff)
{
	(*v->api->wtxt_output)(v, wtxti, txt, widg->state, &widg->ar, xoff, yoff);
}

static void
draw_widg_icon(struct xa_vdi_settings *v, struct xa_widget *widg, XA_TREE *wt, short ind)
{
	short x, y, w, h;
	struct xa_aes_object ob;

	ob = aesobj(wt->tree, ind);

	if (widg->state & OS_SELECTED)
		aesobj_setsel(&ob);
	else
		aesobj_clrsel(&ob);

	x = widg->ar.g_x, y = widg->ar.g_y;

	(*api->object_spec_wh)(aesobj_ob(&ob), &w, &h);
	x += (widg->ar.g_w - w) >> 1;
	y += (widg->ar.g_h - h) >> 1;
	(*api->render_object)(wt, v, ob, x, y);
}

#include "widgets.h"

static void
d_waframe(struct xa_window *wind, const GRECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_vdi_api *vapi = v->api;
	GRECT wa = (wind->dial & created_for_TOOLBAR) ? wind->rwa : wind->wa;	/*(?)*/

	if (wind->thinwork)
	{
		if (wind->wa_frame && wind->wa_borders)
		{
			short waframe_col = ((struct window_colours *)wind->colours)->waframe_col;

			if (wind->wa_borders & WAB_LEFT)
				(*vapi->left_line)(v, 1, &wa, waframe_col);
			if (wind->wa_borders & WAB_RIGHT)
				(*vapi->right_line)(v, 1, &wa, waframe_col);
			if (wind->wa_borders & WAB_TOP)
				(*vapi->top_line)(v, 1, &wa, waframe_col);
			if (wind->wa_borders & WAB_BOTTOM)
				(*vapi->bottom_line)(v, 1, &wa, waframe_col);
		}
	}
	/*
	 * Dont worry about mono colours, because XaAES only allows
	 * thinwork in mono.
	 */
	else
	{
		short sc = G_LBLACK, lc = G_WHITE;

		(*vapi->tl_hook)(v, 2, &wa, sc);
		(*vapi->tl_hook)(v, 1, &wa, lc);

		(*vapi->br_hook)(v, 2, &wa, sc);
		(*vapi->br_hook)(v, 1, &wa, lc);

		/* fill gap between upper toolbar and wa */
		if( (wind->active_widgets & TOOLBAR) )
		{
			if( (wind->active_widgets & VSLIDE) )
			{
				XA_WIDGET *widg = get_widget(wind, XAW_VSLIDE);
				wa.g_w += widg->ar.g_w;
			}

			wa.g_y = wind->wa.g_y;
			wa.g_h = wind->wa.g_h;

			(*vapi->br_hook)(v, 2, &wa, sc);
			(*vapi->tl_hook)(v, 2, &wa, lc);
			(*vapi->br_hook)(v, 1, &wa, lc);
			(*vapi->tl_hook)(v, 1, &wa, sc);
		}
	}
}

static bool _cdecl
d_unused(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct xa_wcol_inf *wc = &((struct window_colours *)wind->colours)->win;
	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wc, NULL, 0, &widg->ar, &wind->r);
	return true;
}

static bool _cdecl
d_borders(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->borders;
	struct xa_vdi_settings *v = wind->vdi_settings;
	short size = wind->frame;
	GRECT r;

	if (wind->frame > 0)
	{
		if (wind->frame >= 4)
		{
			/* top-left box */
			r.g_x = wind->r.g_x;
			r.g_y = wind->r.g_y;
			r.g_w = size;
			r.g_h = size;
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);

			/* Left border */
			r.g_y += size;
			r.g_h = wind->r.g_h - (size + size + wind->y_shadow);
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);

			/* bottom-left box */
			r.g_y = wind->r.g_y + wind->r.g_h - (size + wind->y_shadow);
			r.g_h = size;
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);

			/* Bottom border */
			r.g_x += size;
			r.g_w = wind->r.g_w - (size + size + wind->x_shadow);
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);

			/* right-bottom box */
			r.g_x = wind->r.g_x + wind->r.g_w - (size + wind->x_shadow);
			r.g_w = size;
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);

			/* right border */
			r.g_y = wind->r.g_y + size;
			r.g_h = wind->r.g_h - (size + size + wind->y_shadow);
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);

			/* top-right box */
			r.g_y = wind->r.g_y;
			r.g_h = size;
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);

			/* Top border*/
			r.g_x = wind->r.g_x + size;
			r.g_w = wind->r.g_w - (size + size + wind->x_shadow);
			draw_widg_box(v, 0, wci, NULL, 0, &r, &wind->r);
		}
		else
		{
			int i;
			r = wind->r;
			r.g_w -= wind->x_shadow;
			r.g_h -= wind->y_shadow;
			(*v->api->l_color)(v, ((struct window_colours *)wind->colours)->frame_col);
			for (i = 0; i < wind->frame; i++)
			{
				(*v->api->gbox)(v, 0, &r);
				r.g_x++;
				r.g_y++;
				r.g_w -= 2;
				r.g_h -= 2;
			}
		}
	}
	return true;
}

/* strip leading and trailing spaces. */
static void
strip_name(char *to, const char *fro)
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

#if 1	/* WITH_GRADIENTS */
static void _cdecl
free_priv_gradients(struct xa_window *wind, struct xa_widget *widg)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (widg->m.r.priv[i])
			(*api->free_xa_data_list)((struct xa_data_hdr **)&widg->m.r.priv[i]);
	}
}
#else
static void _cdecl
free_priv_gradients(struct xa_window *wind, struct xa_widget *widg){}
#endif

static bool _cdecl
d_title(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct options *o = &wind->owner->options;
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->title;
	struct xa_wtxt_inf *wti = &wc->title_txt;
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_wtexture *t = NULL;
	char tn[256];
	bool dial = (wind->dial & (created_for_FORM_DO|created_for_FMD_START)) != 0;

	/* Convert relative coords and window location to absolute screen location */
	(*api->rp2ap)(wind, widg, &widg->ar);
	{
#if WITH_GRADIENTS
		if (scrninf->planes > 8)
		{
			struct xa_data_hdr **allocs;
			struct xa_wcol *wcol;

			if (wc->flags & WCF_TOP)
			{
				wcol = &wci->normal;
				allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
			}
			else
			{
				wcol = &wci->selected;
				allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
			}
			t = find_gradient(v, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
		}
#endif
		widg->prevr = widg->ar;

		draw_widg_box(v, 0, wci, t, widg->state, &widg->ar, &widg->ar);
	}

	if (o->windowner && wind->owner && !wind->winob && !dial)
	{
		char *ow = NULL;

		if (o->windowner == true)
			ow = wind->owner->proc_name;
		/* XXX this can't happen
		else if (o->windowner == 2)
			ow = wind->owner->name; */

		if  (ow)
		{
			char ns[32];
			strip_name(ns, ow);
			if (*ns)
				sprintf(tn, sizeof(tn), "(%s) %s", ns, widg->stuff.name);
			else
				ow = NULL;
		}

		if (!ow)
			sprintf(tn, sizeof(tn), "(%d) %s", wind->owner->p->pid, widg->stuff.name);
	}
	else
		strcpy(tn, widg->stuff.name);

	draw_widget_text(v, widg, wti, tn, 4, 0);
	return true;
}

static bool _cdecl
d_wcontext(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->closer;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_CLOSER);
	return true;
}

static bool _cdecl
d_wappicn(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->closer;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_CLOSER);
	return true;
}

static bool _cdecl
d_closer(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->closer;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_CLOSER);
	return true;
}

static bool _cdecl
d_fuller(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->fuller;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_FULL);
	return true;
}

static bool _cdecl
d_info(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->info;
	struct xa_wtxt_inf *wti = &wc->info_txt;
	struct xa_wtexture *t = NULL;
	struct xa_vdi_settings *v = wind->vdi_settings;
	GRECT dr = v->clip;


	/* Convert relative coords and window location to absolute screen location */
	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;


	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);

	if (wti->flags & WTXT_NOCLIP)
	{
		GRECT r;
		if (xa_rect_clip(&widg->ar, &dr, &r))
			(*v->api->set_clip)(wind->vdi_settings, &r);
		else
			return true;
	}
	draw_widget_text(wind->vdi_settings, widg, wti, widg->stuff.name, 4 + widg->xlimit, 0);
	/* restore clip */
	if (wti->flags & WTXT_NOCLIP)
		(*v->api->set_clip)(wind->vdi_settings, &dr);
	return true;
}

static bool _cdecl
d_sizer(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->sizer;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_SIZE);
	return true;
}

static bool _cdecl
d_uparrow(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->uparrow;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_UP);
	return true;
}
static bool _cdecl
d_dnarrow(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->dnarrow;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_DOWN);
	return true;
}
static bool _cdecl
d_lfarrow(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->lfarrow;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_LEFT);
	return true;
}
static bool _cdecl
d_rtarrow(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->rtarrow;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_RIGHT);
	return true;
}

#if WITH_GRADIENTS
static inline struct xa_wtexture *
get_widg_gradient(struct xa_vdi_settings *v, struct xa_widget *widg, struct window_colours *wc, struct xa_wcol_inf *wci, short i, short w, short h)
{
	struct xa_wtexture *t = NULL;
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[i];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[i + 1];
		}
		t = find_gradient(v, wcol, true, allocs, w, h);
	}
	return t;
}
#endif

static bool _cdecl
d_vslide(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;
	GRECT cl;
	struct window_colours *wc = wind->colours;
	struct xa_wtexture *t = 0;

	sl->flags &= ~SLIDER_UPDATE;

	/* Convert relative coords and window location to absolute screen location */
	(*api->rp2ap)(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.g_x = sl->r.g_y = 0;
		sl->r.g_w = widg->ar.g_w;
		sl->r.g_h = widg->ar.g_h;
		cl = widg->ar;
#if WITH_GRADIENTS
		t = get_widg_gradient(wind->vdi_settings, widg, wc, &wc->vslider, 2, widg->ar.g_w, widg->ar.g_h);
#endif
		draw_widg_box(wind->vdi_settings, 0, &wc->vslider, t, 0, &widg->ar, &widg->ar);
		return true;
	}
	len = sl_2_pix(widg->ar.g_h, sl->length);
	if (len < widg_h - 3)
		len = widg_h - 3;

	offs = widg->ar.g_y + sl_2_pix(widg->ar.g_h - len, sl->position);

	if (offs < widg->ar.g_y)
		offs = widg->ar.g_y;
	if (offs + len > widg->ar.g_y + widg->ar.g_h)
		len = widg->ar.g_y + widg->ar.g_h - offs;

#if WITH_GRADIENTS
	t = get_widg_gradient(wind->vdi_settings, widg, wc, &wc->vslide, 0, widg->ar.g_w, widg->ar.g_h);
#endif
	draw_widg_box(wind->vdi_settings, 0, &wc->vslide, t, 0, &widg->ar, &widg->ar);

	sl->r.g_y = offs - widg->ar.g_y;
	sl->r.g_h = len;
	sl->r.g_w = widg->ar.g_w;

	cl.g_x = sl->r.g_x + widg->ar.g_x;
	cl.g_y = sl->r.g_y + widg->ar.g_y;
	cl.g_w = sl->r.g_w;
	cl.g_h = sl->r.g_h;

#if WITH_GRADIENTS
	t = get_widg_gradient(wind->vdi_settings, widg, wc, &wc->vslider, 2, cl.g_w, cl.g_h);
#endif
	draw_widg_box(wind->vdi_settings, 0/*-1*/, &wc->vslider, t, widg->state, &cl, &cl);
	return true;
}

static bool _cdecl
d_hslide(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff.sl;
	struct window_colours *wc = wind->colours;
	struct xa_wtexture *t = 0;
	GRECT cl;

	sl->flags &= ~SLIDER_UPDATE;

	(*api->rp2ap)(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.g_x = sl->r.g_y = 0;
		sl->r.g_w = widg->ar.g_w;
		sl->r.g_h = widg->ar.g_h;
#if WITH_GRADIENTS
		t = get_widg_gradient(wind->vdi_settings, widg, wc, &wc->hslider, 2, widg->ar.g_w, widg->ar.g_h);
#endif
		draw_widg_box(wind->vdi_settings, 0, &wc->hslider, t, 0, &widg->ar, &widg->ar/*&wind->r*/);
		return true;
	}
	len = sl_2_pix(widg->ar.g_w, sl->length);
	if (len < widg_w - 3)
		len = widg_w - 3;

	offs = widg->ar.g_x + sl_2_pix(widg->ar.g_w - len, sl->position);

	if (offs < widg->ar.g_x)
		offs = widg->ar.g_x;
	if (offs + len > widg->ar.g_x + widg->ar.g_w)
		len = widg->ar.g_x + widg->ar.g_w - offs;

#if WITH_GRADIENTS
	t = get_widg_gradient(wind->vdi_settings, widg, wc, &wc->hslide, 0, widg->ar.g_w, widg->ar.g_h);
#endif
	draw_widg_box(wind->vdi_settings, 0, &wc->hslide, t, 0, &widg->ar, &widg->ar/*&wind->r*/);

	sl->r.g_x = offs - widg->ar.g_x;
	sl->r.g_w = len;
	sl->r.g_h = widg->ar.g_h;

	cl.g_x = sl->r.g_x + widg->ar.g_x;
	cl.g_y = sl->r.g_y + widg->ar.g_y;
	cl.g_w = sl->r.g_w;
	cl.g_h = sl->r.g_h;

#if WITH_GRADIENTS
	t = get_widg_gradient(wind->vdi_settings, widg, wc, &wc->hslider, 2, cl.g_w, cl.g_h);
#endif
	draw_widg_box(wind->vdi_settings, 0/*-1*/, &wc->hslider, t, widg->state, &cl, &cl/*&wind->r*/);
	return true;
}

static bool _cdecl
d_iconifier(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->iconifier;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_ICONIFY);
	return true;
}

static bool _cdecl
d_hider(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip)
{
	struct window_colours *wc = wind->colours;
	struct xa_wcol_inf *wci = &wc->hider;
	struct xa_wtexture *t = NULL;

	(*api->rp2ap)(wind, widg, &widg->ar);
#if WITH_GRADIENTS
	if (scrninf->planes > 8)
	{
		struct xa_data_hdr **allocs;
		struct xa_wcol *wcol;

		if (wc->flags & WCF_TOP)
		{
			wcol = &wci->normal;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[0];
		}
		else
		{
			wcol = &wci->selected;
			allocs = (struct xa_data_hdr **)&widg->m.r.priv[1];
		}
		t = find_gradient(wind->vdi_settings, wcol, true, allocs, widg->ar.g_w, widg->ar.g_h);
	}
#endif
	widg->prevr = widg->ar;
	draw_widg_box(wind->vdi_settings, 0, wci, t, widg->state, &widg->ar, t ? &widg->ar : &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_HIDE);
	return true;
}

static void _cdecl
s_title_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->ontop_cols)->title;
	struct xa_wtxt_inf *wti = &((struct window_colours *)wind->ontop_cols)->title_txt;
	struct xa_wtxt_inf *wtu = &((struct window_colours *)wind->untop_cols)->title_txt;
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_vdi_api *vapi = v->api;
	short w, h;

	widg->r.g_w = widg_w;

	if (!wti->normal.font_id)
	{
		wti->normal.font_id =
		wti->selected.font_id =
		wti->highlighted.font_id =
		wtu->normal.font_id =
		wtu->selected.font_id =
		wtu->highlighted.font_id =
		cfg.font_id;
	}
	(*vapi->t_font)(v, wti->normal.font_point, wti->normal.font_id);
	(*vapi->t_effects)(v, wti->normal.effects);
	(*vapi->t_extent)(v, "A", &w, &h);
	(*vapi->t_effects)(v, 0);

	if ((wci->flags & (WCOL_DRAW3D|WCOL_BOXED)) || (wti->flags & WTXT_DRAW3D))
		h += 4;
	if ((wci->flags & WCOL_ACT3D) || (wti->flags & WTXT_ACT3D))
		h++;

	widg->r.g_h = h;
};
static void _cdecl
s_info_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wtxt_inf *wti = &((struct window_colours *)wind->ontop_cols)->info_txt;
	struct xa_wtxt_inf *wtu = &((struct window_colours *)wind->untop_cols)->info_txt;
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_vdi_api *vapi = v->api;
	short w, h;

	widg->r.g_w = widg_w;

	if (!wti->normal.font_id)
	{
		wti->normal.font_id =
		wti->selected.font_id =
		wti->highlighted.font_id =
		wtu->normal.font_id =
		wtu->selected.font_id =
		wtu->highlighted.font_id = cfg.font_id;
	}

	(*vapi->t_font)(v, wti->normal.font_point, wti->normal.font_id);
	(*vapi->t_effects)(v, wti->normal.effects);
	(*vapi->text_extent)(v, "X", &wti->normal, &w, &h);
	(*vapi->t_effects)(v, 0);
 	h += 2;
	widg->r.g_h = h;
}

static void _cdecl
s_menu_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	struct xa_vdi_api *vapi = v->api;
	short w, h;

	(*vapi->t_font)(v, wind->owner->options.standard_font_point, scrninf->standard_font_id);
	(*vapi->t_effects)(v, 0);
	(*vapi->t_extent)(v, "A", &w, &h);

	widg->r.g_h = h + 1 + 1;
	widg->r.g_w = wind->r.g_w;
}

static void _cdecl
set_widg_size(struct xa_window *wind, struct xa_widget *widg, struct xa_wcol_inf *wci, short rsc_ind)
{
	short w, h, f;
	OBJECT *ob = ((struct module *)wind->active_theme->module)->wwt.tree + rsc_ind;

	f = wci->flags;

	(*api->object_spec_wh)(ob, &w, &h);
 	if (f & WCOL_DRAW3D)
 		h += 2, w += 2;
 	if (f & WCOL_BOXED)
 		h += 2, w += 2;

	widg->r.g_w = w;
	widg->r.g_h = h;
}

static void _cdecl
s_wcontext_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->closer, WIDG_CLOSER);
}
static void _cdecl
s_wappicn_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->closer, WIDG_CLOSER);
}
static void _cdecl
s_closer_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->closer, WIDG_CLOSER);
}
static void _cdecl
s_hider_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->hider, WIDG_SIZE);
}
static void _cdecl
s_iconifier_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->iconifier, WIDG_ICONIFY);
}
static void _cdecl
s_fuller_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->fuller, WIDG_FULL);
}
static void _cdecl
s_uparrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->uparrow, WIDG_UP);
}
static void _cdecl
s_dnarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->dnarrow, WIDG_DOWN);
}
static void _cdecl
s_lfarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->lfarrow, WIDG_LEFT);
}
static void _cdecl
s_rtarrow_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->rtarrow, WIDG_RIGHT);
}
static void _cdecl
s_sizer_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->sizer, WIDG_SIZE);
}
/*
 * The vertical slider must get its dimentions from the arrows
 */
static void _cdecl
s_vslide_size(struct xa_window *wind, struct xa_widget *widg)
{
	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->uparrow, WIDG_UP);
}
static void _cdecl
s_hslide_size(struct xa_window *wind, struct xa_widget *widg)
{
 	set_widg_size(wind, widg, &((struct window_colours *)wind->ontop_cols)->rtarrow, WIDG_RIGHT);
}

/*
 * Build two BFOBSPEC fields from info found in xa_wcol_inf and xa_wtxt_inf.
 * first BFOBSEPC is for normal widget while the second is for selected widget
 */
static void
build_bfobspec(struct xa_wcol_inf *wci, struct xa_wtxt_inf *wti, BFOBSPEC *ret)
{
	union { unsigned long l[2]; BFOBSPEC o[2]; } b;

	b.l[0] = b.l[1] = 0L;

	if (wci)
	{
		b.o[0].framesize	= wci->normal.box_th;
		b.o[0].framecol		= wci->normal.box_c;
		b.o[0].fillpattern	= (wci->normal.i == FIS_PATTERN && wci->normal.f < 9) ? wci->normal.f : 7;
		b.o[0].interiorcol	= wci->normal.c;

		b.o[1].framesize	= wci->selected.box_th;
		b.o[1].framecol		= wci->selected.box_c;
		b.o[1].fillpattern	= (wci->selected.i == FIS_PATTERN && wci->selected.f < 9) ? wci->selected.f : 7;
		b.o[1].interiorcol	= wci->selected.c;
	}

	if (wti)
	{
		b.o[0].textcol = wti->normal.fg;
		b.o[1].textcol = wti->selected.fg;
	}
	else
	{
		b.o[0].textcol = b.o[1].textcol = G_BLACK;
	}
	b.o[0].textmode = b.o[1].textmode = 0;

	*ret++ = b.o[0];
	*ret = b.o[1];
}

static void _cdecl
get_wcol(struct xa_window *wind, short gem_widget, BFOBSPEC *ret)
{
	struct window_colours *ontop_cols = wind->ontop_cols;
	struct window_colours *untop_cols = wind->untop_cols;
	BFOBSPEC c[4];

 	/*
 	 * Builds 4 BFOBSPECs,
 	 *  0 = colors for normal   widget when window ontop
 	 *  1 = colors for selected widget when window ontop
 	 *  2 = colors for normal   widget when window untopped
 	 *  3 = colors for selected widget when window untopped
 	 */
 	c[0] = c[1] = c[2] = c[3] = (BFOBSPEC){0};

	switch (gem_widget)
	{
		case W_BOX:
		{
			build_bfobspec(&ontop_cols->borders, NULL, &c[0]);
			build_bfobspec(&untop_cols->borders, NULL, &c[2]);
			break;
		}
		case W_TITLE:
		{
			build_bfobspec(&ontop_cols->title, NULL, &c[0]);
			build_bfobspec(&untop_cols->title, NULL, &c[2]);
			break;
		}
		case W_CLOSER:
		{
			build_bfobspec(&ontop_cols->closer, NULL, &c[0]);
			build_bfobspec(&untop_cols->closer, NULL, &c[2]);
			break;
		}
		case W_NAME:
		{
			build_bfobspec(&ontop_cols->title, &ontop_cols->title_txt, &c[0]);
			build_bfobspec(&untop_cols->title, &untop_cols->title_txt, &c[2]);
			break;
		}
		case W_FULLER:
		{
			build_bfobspec(&ontop_cols->fuller, NULL, &c[0]);
			build_bfobspec(&untop_cols->fuller, NULL, &c[2]);
			break;
		}
		case W_INFO:
		{
			build_bfobspec(&ontop_cols->info, &ontop_cols->info_txt, &c[0]);
			build_bfobspec(&untop_cols->info, &untop_cols->info_txt, &c[2]);
			break;
		}
		case W_DATA:
		{
#if 0
			BFOBSPEC ot, ut;

		 	ot = ut = (BFOBSPEC){0};

			ot.framesize = ut.framesize = wind->thinwork ? 1 : 2;
			ot.framecol = ontop_cols->waframe_col;
			UNUSED(ot);
			ut.framecol = untop_cols->waframe_col;
#endif
			break;
		}
		case W_WORK:
		{
			c[0] = c[1] = c[2] = c[3] = (BFOBSPEC){0};
			break;
		}
		case W_SIZER:
		{
			build_bfobspec(&ontop_cols->sizer, NULL, &c[0]);
			build_bfobspec(&untop_cols->sizer, NULL, &c[2]);
			break;
		}
		case W_HBAR:
		case W_VBAR:
		{
			build_bfobspec(&ontop_cols->vslide, NULL, &c[0]);
			build_bfobspec(&untop_cols->vslide, NULL, &c[2]);
			break;
		}
		case W_UPARROW:
		{
			build_bfobspec(&ontop_cols->uparrow, NULL, &c[0]);
			build_bfobspec(&untop_cols->uparrow, NULL, &c[2]);
			break;
		}
		case W_DNARROW:
		{
			build_bfobspec(&ontop_cols->dnarrow, NULL, &c[0]);
			build_bfobspec(&untop_cols->dnarrow, NULL, &c[2]);
			break;
		}
		case W_HSLIDE:
		case W_VSLIDE:
		{
			build_bfobspec(&ontop_cols->vslide, NULL, &c[0]);
			build_bfobspec(&untop_cols->vslide, NULL, &c[2]);
			break;
		}
		case W_HELEV:
		case W_VELEV:
		{
			build_bfobspec(&ontop_cols->vslider, NULL, &c[0]);
			build_bfobspec(&untop_cols->vslider, NULL, &c[2]);
			break;
		}
		case W_LFARROW:
		{
			build_bfobspec(&ontop_cols->lfarrow, NULL, &c[0]);
			build_bfobspec(&untop_cols->lfarrow, NULL, &c[2]);
			break;
		}
		case W_RTARROW:
		{
			build_bfobspec(&ontop_cols->rtarrow, NULL, &c[0]);
			build_bfobspec(&untop_cols->rtarrow, NULL, &c[2]);
			break;
		}
	}
	if (ret)
	{
		*ret++ = c[0];
		*ret++ = c[1];
		*ret++ = c[2];
		*ret   = c[3];
	}
}

static void _cdecl
set_wcol(struct xa_window *wind, short gem_widget, BFOBSPEC *c)
{
	get_wcol(wind, gem_widget, c);
}

static inline void
fix_widg(OBJECT *widg)
{
	widg->ob_x = 0;
	widg->ob_y = 0;
}

/* Standard Widget Set from widget resource file */
static void
fix_default_widgets(void *rsc)
{
	OBJECT *def_widgets;
	int i;

	def_widgets = (*api->resource_tree)(rsc, WIDGETS);
	DIAGS(("widget set at %lx", (unsigned long)def_widgets));

	for (i = 1; i < WIDG_CFG; i++)
		fix_widg(def_widgets + i);
}


static void
set_texture(struct module *m, struct xa_wcol_inf *wcol, struct widg_texture *t)
{
	if (t)
	{
		wcol->flags &= ~(WCOL_DRAWBKG);
		wcol->highlighted.texture = &t->t;
		wcol->normal.texture = &t->t;
		wcol->selected.texture = &t->t;
		(*api->ref_xa_data)(&m->allocs, t, 3);
	}
	else
	{
		wcol->flags |= WCOL_DRAWBKG;
		wcol->highlighted.texture = NULL;
		wcol->normal.texture = NULL;
		wcol->selected.texture = NULL;
	}
}
static void
ref_colortheme_resources(struct module *m, struct window_colours *wc)
{
	int i, wcols;
	struct xa_wcol_inf *wci;
	struct widg_texture *t;
	struct xa_wtexture *wtexture;

	wcols = ( (long)&wc->rtarrow - (long)&wc->win) / sizeof(struct  xa_wcol_inf);
	wci = &wc->win;

	for (i = 0; i <= wcols; i++)
	{
		if ((wtexture = wci[i].highlighted.texture))
		{
			t = (*api->lookup_xa_data_byid)(&m->allocs, (long)wtexture);
			if (t) (*api->ref_xa_data)(&m->allocs, t, 1);
			else BLOG((1,"ref_colortheme_resources(h):t=0(wtexure=%lx,m=%lx,i=%d)", (unsigned long)wtexture, (unsigned long)m, i));
		}
		if ((wtexture = wci[i].normal.texture))
		{
			t = (*api->lookup_xa_data_byid)(&m->allocs, (long)wtexture);
			if (t) (*api->ref_xa_data)(&m->allocs, t, 1);
			else BLOG((1,"ref_colortheme_resources(n):t=0(wtexure=%lx,m=%lx,i=%d)", (unsigned long)wtexture, (unsigned long)m, i));
		}
		if ((wtexture = wci[i].selected.texture))
		{
			t = (*api->lookup_xa_data_byid)(&m->allocs, (long)wtexture);
			if (t) (*api->ref_xa_data)(&m->allocs, t, 1);
			else BLOG((1,"ref_colortheme_resources(s):t=0(wtexure=%lx,m=%lx,i=%d)", (unsigned long)wtexture, (unsigned long)m, i));
		}
	}
}
static void
deref_colortheme_resources(struct module *m, struct window_colours *wc)
{
	int i, wcols;
	struct xa_wcol_inf *wci;
	struct widg_texture *t;
	struct xa_wtexture *wtexture;

	wcols = ((long)&wc->rtarrow - (long)&wc->win) / sizeof(struct xa_wcol_inf);
	wci = &wc->win;

	for (i = 0; i <= wcols; i++)
	{
		if ((wtexture = wci[i].highlighted.texture))
		{
			t = (*api->lookup_xa_data_byid)(&m->allocs, (long)wtexture);
			if (t)
			{
				(*api->deref_xa_data)(&m->allocs, t, 1);
			}
			else BLOG((1,"deref_colortheme_resources(h):t=0(wtexure=%lx,m=%lx,i=%d)", (unsigned long)wtexture, (unsigned long)m, i));
		}
		if ((wtexture = wci[i].normal.texture))
		{
			t = (*api->lookup_xa_data_byid)(&m->allocs, (long)wtexture);
			if (t)
			{
				(*api->deref_xa_data)(&m->allocs, t, 1);
			}
			else BLOG((1,"deref_colortheme_resources(n):t=0(wtexure=%lx,m=%lx,i=%d)", (unsigned long)wtexture, (unsigned long)m, i));
		}
		if ((wtexture = wci[i].selected.texture))
		{
			t = (*api->lookup_xa_data_byid)(&m->allocs, (long)wtexture);
			if (t)
			{
				(*api->deref_xa_data)(&m->allocs, t, 1);
			}
			else BLOG((1,"deref_colortheme_resources(s):t=0(wtexure=%lx,m=%lx,i=%d)", (unsigned long)wtexture, (unsigned long)m, i));
		}
	}
}

static void
foreach_widget(struct module *m, struct window_colours *wc, void(*f)(struct module *, struct xa_wcol_inf *, void *), void *parms)
{
	int i, wcols;
	struct xa_wcol_inf *wci;

	wcols = ((long)&wc->rtarrow - (long)&wc->win) / sizeof(struct xa_wcol_inf);
	wci = &wc->win;

	for (i = 0; i <= wcols; i++)
	{
		(*f)(m, &wci[i], parms);
	}
}

static void
cleanup_colortheme(struct module *m, struct window_colours *wc, char *txt)
{
	int i, wcols;
	struct xa_wcol_inf *wci;

	deref_colortheme_resources(m, wc);

	wcols = ((long)&wc->rtarrow - (long)&wc->win) / sizeof(struct xa_wcol_inf);
	wci = &wc->win;

	for (i = 0; i <= wcols; i++)
	{
		set_texture(m, &wci[i], NULL);
	}
}

/* render_obj.c */
extern int imgpath_file;
extern char imgpath[128];

static void _cdecl
delete_texture(void *_t)
{
	struct widg_texture *t = _t;

	if (t->xamfdb.mfdb.fd_addr)
		(*api->kfree)(t->xamfdb.mfdb.fd_addr);
	(*api->kfree)(t);
}

static struct widg_texture *
load_texture(struct module *m, char *fn)
{
	struct widg_texture *t = NULL;

	if( imgpath_file == -1 )
		return NULL;

	imgpath[imgpath_file] = '\0';
	strcat(imgpath, fn);
	if ((fn = (*api->sysfile)(imgpath)))
	{
		if ((t = (*api->kmalloc)(sizeof(*t))))
		{
			(*api->bclear)(t, sizeof(*t));

			(*api->load_img)(fn, &t->xamfdb);
			if (t->xamfdb.mfdb.fd_addr)
			{
				(*api->add_xa_data)(&m->allocs, t, (long)&t->t, NULL, delete_texture);
				t->t.flags = 0;
				t->t.anchor = 0;
				t->t.left = t->t.right = t->t.top = t->t.bottom = NULL;
				t->t.body = &t->xamfdb;
			}
			else
			{
				(*api->kfree)(t);
				t = NULL;
			}
		}
		(*api->kfree)(fn);
	}
	return t;
}


#if WITH_GRADIENTS
static void free_grad( struct xa_window *w )
{
	int i, n;
	struct xa_data_hdr **a;
	for( n = 0; n < XA_MAX_WIDGETS; n++ )
	{
		for( i = 0; i < 4; i++ )
		{
			a = (struct xa_data_hdr **)&w->widgets[n].m.r.priv[i];
			(*api->free_xa_data_list)(a);
		}
	}
}
void free_widg_grad(const struct xa_module_api *_api)
{
	struct xa_window *wp[] = {_api->S->open_windows.first, _api->S->closed_windows.first, 0};
	struct xa_window *w;
	int i;
	for( i = 0; wp[i]; i++ )
		for( w = wp[i]; w; w = w->next )
		{
			if( w != root_window )
			{
				free_grad( w );
				if( w->winob )
					free_grad( (struct xa_window *)w->winob );
			}
		}
}

static void _cdecl
delete_pmap(void *_t)
{
	struct widg_texture *t = _t;

	if (t->xamfdb.mfdb.fd_addr)
		(*api->kfree)(t->xamfdb.mfdb.fd_addr);
	(*api->kfree)(t);
}
static struct xa_wtexture *
find_gradient(struct xa_vdi_settings *v, struct xa_wcol *wcol, bool free, struct xa_data_hdr **allocs, short w, short h)
{
	struct widg_texture *t;
	struct xa_wtexture *ret = NULL;
	struct xa_gradient *g = wcol->gradient;

	if (g && use_gradients && g->n_steps >= 0)
	{
		if (!allocs)
			allocs = &g->allocs;

		w &= g->wmask;
		w |= g->w;
		h &= g->hmask;
		h |= g->h;
		t =  (*api->lookup_xa_data_byid)(allocs, (((long)w << 16) | h) );
		if (!t)
		{
			if (free)
			{
				(*api->free_xa_data_list)(allocs);
			}

			t = kmalloc(sizeof(*t));
			if (t)
			{
				(*api->bclear)(t, sizeof(*t));

				(*v->api->create_gradient)(&t->xamfdb, g->c, g->method, g->n_steps, g->steps, w, h);

				if (t->xamfdb.mfdb.fd_addr)
				{
					(*api->add_xa_data)(allocs, t, (((long)w << 16) | h), NULL, delete_pmap);
					t->t.flags = 0;
					t->t.anchor = 0;
					t->t.left = t->t.right = t->t.top = t->t.bottom = NULL;
					t->t.body = &t->xamfdb;
					ret = &t->t;
				}
				else
				{
					(*api->kfree)(t);
					t = NULL;
				}
			}
		}
		else
		{
			ret = &t->t;
		}
	}
	return ret;
}
#endif
static void
init_sliders(struct module *m)
{
	struct widg_texture *t;

 /* slide and slider (scrollbars) */
	if ((t = load_texture(m, "slider.img")))
	{
		set_texture(m, &def_otop_cols.vslider, t);
		set_texture(m, &def_utop_cols.vslider, t);
		set_texture(m, &def_otop_cols.hslider, t);
		set_texture(m, &def_utop_cols.hslider, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_otop_cols.vslider, t);
		set_texture(m, &slist_def_utop_cols.vslider, t);
		set_texture(m, &slist_def_otop_cols.hslider, t);
		set_texture(m, &slist_def_utop_cols.hslider, t);
#endif
	}

	if ((t = load_texture(m, "slide.img")))
	{
		set_texture(m, &def_otop_cols.vslide, t);
		set_texture(m, &def_utop_cols.vslide, t);
		set_texture(m, &def_otop_cols.hslide, t);
		set_texture(m, &def_utop_cols.hslide, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_otop_cols.vslide, t);
		set_texture(m, &slist_def_utop_cols.vslide, t);
		set_texture(m, &slist_def_otop_cols.hslide, t);
		set_texture(m, &slist_def_utop_cols.hslide, t);
#endif
	}

 /* allow seperate slider textures */
	if ((t = load_texture(m, "vslider.img")))
	{
		set_texture(m, &def_otop_cols.vslider, t);
		set_texture(m, &def_utop_cols.vslider, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_otop_cols.vslider, t);
		set_texture(m, &slist_def_utop_cols.vslider, t);
#endif
	}

	if ((t = load_texture(m, "vslide.img")))
	{
		set_texture(m, &def_otop_cols.vslide, t);
		set_texture(m, &def_utop_cols.vslide, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_otop_cols.vslide, t);
		set_texture(m, &slist_def_utop_cols.vslide, t);
#endif
	}

 /* allow seperate unfocused textures */
	if ((t = load_texture(m, "slideru.img")))
	{
		set_texture(m, &def_utop_cols.vslider, t);
		set_texture(m, &def_utop_cols.hslider, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_utop_cols.vslider, t);
		set_texture(m, &slist_def_utop_cols.hslider, t);
#endif
	}

	if ((t = load_texture(m, "slideu.img")))
	{
		set_texture(m, &def_utop_cols.vslide, t);
		set_texture(m, &def_utop_cols.hslide, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_utop_cols.vslide, t);
		set_texture(m, &slist_def_utop_cols.hslide, t);
#endif
	}

 /* allow seperate unfocused slider textures */
	if ((t = load_texture(m, "vslideru.img")))
	{
		set_texture(m, &def_utop_cols.vslider, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_utop_cols.vslider, t);
#endif
	}

	if ((t = load_texture(m, "vslideu.img")))
	{
		set_texture(m, &def_utop_cols.vslide, t);
#ifndef ST_ONLY
		set_texture(m, &slist_def_utop_cols.vslide, t);
#endif
	}
}

static void
info_texture(struct module *m)
{
	struct widg_texture *t;

	/* info bar */
	if ((t = load_texture(m, "info.img")))
	{
		set_texture(m, &def_otop_cols.info, t);
		set_texture(m, &def_utop_cols.info, t);
	}

	/* unfocused info bar */
	if ((t = load_texture(m, "infou.img")))
	{
		set_texture(m, &def_utop_cols.info, t);
	}
}

static void
title_texture(struct module *m)
{
	struct widg_texture *t;

	t = load_texture(m, "wtitle.img");

	 /* window title bar */
	if (t)
	{
		set_texture(m, &def_otop_cols.title, t);
		set_texture(m, &def_utop_cols.title, t);
	}

	/* unfocused window title bar */
	t = load_texture(m, "wtitleu.img");
	if (t)
	{
		set_texture(m, &def_utop_cols.title, t);
	}

#ifndef ST_ONLY
 /* scroll list title (fileselector) bar */
	t = load_texture(m, "slwtitle.img");
	if (t)
	{
		set_texture(m, &slist_def_otop_cols.title, t);
		set_texture(m, &slist_def_utop_cols.title, t);
	}

 /* unfocused scroll list title (fileselector) bar */
	t = load_texture(m, "ulwtitle.img");
	if (t)
	{
		set_texture(m, &slist_def_utop_cols.title, t);
	}
#endif
}

static void
installtexture(struct module *m, struct xa_wcol_inf *wci, void *_t)
{
	struct widg_texture *t = _t;
	if (!wci->normal.texture && !wci->selected.texture && !wci->highlighted.texture)
		set_texture(m, wci, t);
}

static void
test_img_stuff(struct module *m)
{
	struct widg_texture *t;

	init_sliders(m);
	info_texture(m);
	title_texture(m);

	/* background texture */
	if ((t = load_texture(m, "exterior.img")))
	{
		foreach_widget(m, &def_otop_cols, installtexture, t);
		foreach_widget(m, &def_utop_cols, installtexture, t);
#ifndef ST_ONLY
		foreach_widget(m, &slist_def_otop_cols, installtexture, t);
		foreach_widget(m, &slist_def_utop_cols, installtexture, t);
#endif
	}

 /* unfocused background texture */
	if ((t = load_texture(m, "uxterior.img")))
	{
		foreach_widget(m, &def_utop_cols, installtexture, t);
#ifndef ST_ONLY
		foreach_widget(m, &slist_def_utop_cols, installtexture, t);
#endif
	}

}

/*
 * This function is called by XaAES to have the module initialize itself
 */
static void * _cdecl
init_module(const struct xa_module_api *xmapi, const struct xa_screen *screen, char *widg_name, bool grads)
{
	char *rscfile;
	RSHDR *rsc;
	struct module *m;

	use_gradients = grads;

	api	= xmapi;
	scrninf	= screen;

	current_theme = &def_theme;
	current_pu_theme = &pu_def_theme;
	current_slist_theme = &sl_def_theme;

	m = (*api->kmalloc)(sizeof(*m));
	if (m)
	{
		(*api->bclear)(m, sizeof(*m));

		/* Load the widget resource files */
		if (!(rscfile = (*api->sysfile)(widg_name)))
		{
			display("ERROR: Can't find widget resource file '%s'", widg_name);
			goto error;
		}
		else
		{
			rsc = (*api->load_resource)(NULL, rscfile, NULL, DU_RSX_CONV, DU_RSY_CONV, false);
			DIAGS(("widget_resources = %lx (%s)", (unsigned long)rsc, widg_name));
			(*api->kfree)(rscfile);
		}
		if (!rsc)
		{
			display("ERROR: Can't find/load widget resource file '%s'", widg_name);
			goto error;
			(*api->kfree)(m);
			return NULL;
		}

		/* get widget object parameters. */
		{
			GRECT c;
			OBJECT *tree = (*api->resource_tree)(rsc, 0);
			(*api->ob_spec_xywh)(tree, 1, &c);
			(*api->init_widget_tree)(NULL, &m->wwt, tree);

			widg_w = c.g_w;
			widg_h = c.g_h;

			DIAGS(("widg: %d/%d", widg_w, widg_h));
		}
		fix_default_widgets(rsc);

		if (screen->planes >= 8 && imgpath_file != -1)
		{
			strcpy(imgpath, "img\\");
			strcat(imgpath, screen->planes == 8 ? "8b\\" : "hc\\");
			imgpath_file = strlen(imgpath);
			test_img_stuff(m);
		}

		if (screen->planes > 8 && use_gradients)
		{
			def_otop_cols.title_txt.normal.fg = G_WHITE;
			def_otop_cols.title_txt.normal.bg = G_BLUE;
			def_otop_cols.title_txt.selected.fg = G_WHITE;
			def_otop_cols.title_txt.selected.bg = G_BLUE;
		}
		else
		{
			def_otop_cols.title_txt.normal.fg = G_BLACK;
			def_otop_cols.title_txt.normal.bg = G_WHITE;
			def_otop_cols.title_txt.selected.fg = G_BLACK;
			def_otop_cols.title_txt.selected.bg = G_WHITE;
		}

		if (scrninf->r.g_h <= 280)
		{
			def_otop_cols.title_txt.normal.font_point = MEDIUM_FONT_POINT;
			def_otop_cols.title_txt.selected.font_point = MEDIUM_FONT_POINT;
			def_otop_cols.title_txt.highlighted.font_point = MEDIUM_FONT_POINT;
			def_utop_cols.title_txt.normal.font_point = MEDIUM_FONT_POINT;
			def_utop_cols.title_txt.selected.font_point = MEDIUM_FONT_POINT;
			def_utop_cols.title_txt.highlighted.font_point = MEDIUM_FONT_POINT;
		}

		/* set window-title and info-font-id */
		def_otop_cols.title_txt.normal.font_id = cfg.font_id;
		def_otop_cols.title_txt.selected.font_id = cfg.font_id;
		def_otop_cols.title_txt.highlighted.font_id = cfg.font_id;
		def_utop_cols.title_txt.normal.font_id = cfg.font_id;
		def_utop_cols.title_txt.selected.font_id = cfg.font_id;
		def_utop_cols.title_txt.highlighted.font_id = cfg.font_id;

		def_otop_cols.info_txt.normal.font_id = cfg.font_id;
		def_otop_cols.info_txt.selected.font_id = cfg.font_id;
		def_otop_cols.info_txt.highlighted.font_id = cfg.font_id;
		def_utop_cols.info_txt.normal.font_id = cfg.font_id;
		def_utop_cols.info_txt.selected.font_id = cfg.font_id;
		def_utop_cols.info_txt.highlighted.font_id = cfg.font_id;

		/* set window-info-font-pt */
		def_otop_cols.info_txt.normal.font_point = cfg.info_font_point;
		def_otop_cols.info_txt.selected.font_point = cfg.info_font_point;
		def_otop_cols.info_txt.highlighted.font_point = cfg.info_font_point;

		def_utop_cols.info_txt.normal.font_point = cfg.info_font_point;
		def_utop_cols.info_txt.selected.font_point = cfg.info_font_point;
		def_utop_cols.info_txt.highlighted.font_point = cfg.info_font_point;

		mono_def_otop_cols.info_txt.normal.font_point = cfg.info_font_point;
		mono_def_otop_cols.info_txt.selected.font_point = cfg.info_font_point;
		mono_def_otop_cols.info_txt.highlighted.font_point = cfg.info_font_point;

		mono_def_utop_cols.info_txt.normal.font_point = cfg.info_font_point;
		mono_def_utop_cols.info_txt.selected.font_point = cfg.info_font_point;
		mono_def_utop_cols.info_txt.highlighted.font_point = cfg.info_font_point;


		mono_def_otop_cols.title_txt.normal.font_id = cfg.font_id;
		mono_def_otop_cols.title_txt.selected.font_id = cfg.font_id;
		mono_def_otop_cols.title_txt.highlighted.font_id = cfg.font_id;
		mono_def_utop_cols.title_txt.normal.font_id = cfg.font_id;
		mono_def_utop_cols.title_txt.selected.font_id = cfg.font_id;
		mono_def_utop_cols.title_txt.highlighted.font_id = cfg.font_id;

		mono_def_otop_cols.info_txt.normal.font_id = cfg.font_id;
		mono_def_otop_cols.info_txt.selected.font_id = cfg.font_id;
		mono_def_otop_cols.info_txt.highlighted.font_id = cfg.font_id;
		mono_def_utop_cols.info_txt.normal.font_id = cfg.font_id;
		mono_def_utop_cols.info_txt.selected.font_id = cfg.font_id;
		mono_def_utop_cols.info_txt.highlighted.font_id = cfg.font_id;


#ifndef ST_ONLY
		/* set slist-window-title and info font-id */
		slist_def_otop_cols.title_txt.normal.font_id = cfg.font_id;
		slist_def_otop_cols.title_txt.selected.font_id= cfg.font_id;
		slist_def_otop_cols.title_txt.highlighted.font_id= cfg.font_id;

		slist_def_utop_cols.title_txt.normal.font_id = cfg.font_id;
		slist_def_utop_cols.title_txt.selected.font_id = cfg.font_id;
		slist_def_utop_cols.title_txt.highlighted.font_id = cfg.font_id;

		slist_def_otop_cols.info_txt.normal.font_id = cfg.font_id;
		slist_def_otop_cols.info_txt.selected.font_id = cfg.font_id;
		slist_def_otop_cols.info_txt.highlighted.font_id = cfg.font_id;

		slist_def_utop_cols.info_txt.normal.font_id = cfg.font_id;
		slist_def_utop_cols.info_txt.selected.font_id = cfg.font_id;
		slist_def_utop_cols.info_txt.highlighted.font_id = cfg.font_id;


		/* set infoline-point */
		slist_def_otop_cols.info_txt.normal.font_point = cfg.xaw_point;
		slist_def_otop_cols.info_txt.selected.font_point = cfg.xaw_point;
		slist_def_otop_cols.info_txt.highlighted.font_point = cfg.xaw_point;

		slist_def_utop_cols.info_txt.normal.font_point = cfg.xaw_point;
		slist_def_utop_cols.info_txt.selected.font_point = cfg.xaw_point;
		slist_def_utop_cols.info_txt.highlighted.font_point = cfg.xaw_point;
#endif

	}
	else
	{
error:
		if (m)
		{
			(*api->kfree)(m);
			m = NULL;
		}
		api = NULL;
	}

	return m;
}

static void _cdecl
exit_module(void *_module)
{
	struct module *m = _module;

	/*
	 * This will free all allocs done by this module
	 */
#if 0
	cleanup_deftheme(m);
#endif
	cleanup_colortheme(m, &def_otop_cols, "default ontop");
	cleanup_colortheme(m, &def_utop_cols, "default untop");
#ifndef ST_ONLY
	cleanup_colortheme(m, &alert_def_otop_cols, "alert default ontop");
	cleanup_colortheme(m, &alert_def_utop_cols, "alert default untop");
	cleanup_colortheme(m, &slist_def_otop_cols, "slist default ontop");
	cleanup_colortheme(m, &slist_def_utop_cols, "slist default untop");
#endif
	(*api->free_xa_data_list)(&m->allocs);
	/*
	 * for now, the resource loaded is attached to AESSYS
	 * and cannot be freed here..
	 */
	(*api->remove_wt)(&m->wwt, true);
	(*api->kfree)(m);
}

/*
 * This is called by XaAES whenever a new widget_theme structure needs
 * filling in
 */
static long _cdecl
new_theme(void *_module, short win_type, struct widget_theme **ret_theme)
{
	struct widget_theme *new = NULL;
	struct module *m = _module;
	long ret = 0L;

	switch (win_type)
	{
		case WINCLASS_CLIENT:
			new = duplicate_theme(current_theme);
			break;
		case WINCLASS_POPUP:
			new = duplicate_theme(current_pu_theme);
			break;
		case WINCLASS_ALERT:
			new = duplicate_theme(current_theme);
			break;
		case WINCLASS_SLIST:
			new = duplicate_theme(current_slist_theme);
			break;
		default:
		{
			new = duplicate_theme(current_theme);
			break;
		}
	}

	if (new)
	{
		new->module = m;
		(*api->add_xa_data)(&m->allocs, new, 0, NULL, delete_theme);
		*ret_theme = new;
	}
	else
	{
		*ret_theme = NULL;
		ret = ENOMEM;
	}
	return ret;
}

static long _cdecl
free_theme(void *_module, struct widget_theme **theme)
{
	struct module *m = _module;
	struct widget_theme *t;
	long ret = -1;

	if ((t = (*api->lookup_xa_data)(&m->allocs, *theme)))
	{
		(*api->delete_xa_data)(&m->allocs, t);
		ret = 0;
	}
	return ret;
}


static void _cdecl
delete_color_theme(void *_ctheme)
{
	(*api->kfree)(_ctheme);
}


static long _cdecl
new_color_theme(void *_module, short win_class, void **ontop, void **untop)
{
	struct window_colours *new_ontop, *new_untop;
	struct module *m = _module;
	long ret;

	new_ontop = (*api->kmalloc)(sizeof(*new_ontop));
	new_untop = (*api->kmalloc)(sizeof(*new_untop));

	if (new_ontop && new_untop)
	{
		switch (win_class)
		{
			case WINCLASS_CLIENT:
				*new_ontop = MONO ? mono_def_otop_cols : def_otop_cols;
				*new_untop = MONO ? mono_def_utop_cols : def_utop_cols;
				break;
			case WINCLASS_POPUP:
				*new_ontop = MONO ? mono_def_otop_cols : def_otop_cols;
				*new_untop = MONO ? mono_def_utop_cols : def_utop_cols;
				break;
			case WINCLASS_ALERT:
#ifndef ST_ONLY
				*new_ontop = MONO ? mono_def_otop_cols : alert_def_otop_cols;
				*new_untop = MONO ? mono_def_utop_cols : alert_def_utop_cols;
#else
				*new_ontop = MONO ? mono_def_otop_cols : def_otop_cols;
				*new_untop = MONO ? mono_def_utop_cols : def_utop_cols;
#endif
				break;
			case WINCLASS_SLIST:
#ifndef ST_ONLY
				*new_ontop = MONO ? mono_def_otop_cols : slist_def_otop_cols;
				*new_untop = MONO ? mono_def_utop_cols : slist_def_utop_cols;
#else
				*new_ontop = MONO ? mono_def_otop_cols : def_otop_cols;
				*new_untop = MONO ? mono_def_utop_cols : def_utop_cols;
#endif
				/* !! */
				new_untop->info_txt.flags |= WTXT_NOCLIP;
				new_ontop->info_txt.flags |= WTXT_NOCLIP;

				break;
			default:
 			{
				*new_ontop = MONO ? mono_def_otop_cols : def_otop_cols;
				*new_untop = MONO ? mono_def_utop_cols : def_utop_cols;
				break;
			}
		}

		ref_colortheme_resources(m, new_ontop);
		ref_colortheme_resources(m, new_untop);

		(*api->add_xa_data)(&m->allocs, new_ontop, 0, NULL, delete_color_theme);
		(*api->add_xa_data)(&m->allocs, new_untop, 0, NULL, delete_color_theme);

		ret = 1L;
	}
	else
	{
		if (new_ontop)
			(*api->kfree)(new_ontop);
		if (new_untop)
			(*api->kfree)(new_untop);
		new_ontop = new_untop = NULL;
		ret = 0L;
	}
	*ontop = new_ontop;
	*untop = new_untop;
	return ret;
}


static void _cdecl
free_color_theme(void *_module, void *ctheme)
{
	struct module *m = _module;
	struct window_colours *wc;

	if ((wc = (*api->lookup_xa_data)(&m->allocs, ctheme)))
	{
		deref_colortheme_resources(m, wc);
		(*api->delete_xa_data)(&m->allocs, wc);
	}
}

/*
 *
 */
static char short_name[] = "XaAES standard.";
static char long_name[]  = "XaAES standard widget-module.";

static struct xa_module_widget_theme module =
{
	short_name,
	long_name,

	init_module,
	exit_module,

	new_theme,
	free_theme,

	new_color_theme,
	free_color_theme,
};

/*
 * The very first entry XaAES will do into the module is to its 'main'
 * function, which should return the address of its module_theme api.
 */
void
main_xa_theme(struct xa_module_widget_theme **ret)
{
	*ret = &module;
}
