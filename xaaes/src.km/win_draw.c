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

// #include "xa_global.h"

#include "xa_types.h"
#include "draw_obj.h"
#include "obtree.h"
#include "xaaeswdg.h"
#include "win_draw.h"

#define MONO (scrninf->colours < 16)

static const struct xa_module_api *api;
static const struct xa_screen *scrninf;

static struct widget_theme *current_theme;
static struct widget_theme *current_pu_theme;

static short widg_w;
static short widg_h;


struct module
{
	struct widget_tree wwt;
	struct xa_data_hdr *allocs;
};

static	WidgGemColor	get_wcol, set_wcol;

static	DrawWidg	d_unused, d_borders, d_title, d_closer, d_fuller, d_info,
			d_sizer, d_uparrow, d_dnarrow, d_vslide,
			d_lfarrow, d_rtarrow, d_hslide, d_iconifier, d_hider;

static	SetWidgSize	s_title_size, s_closer_size, s_fuller_size, s_info_size,
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
// 	{1, 1, 2, 2},
	{0, 0, 0, 0},
	NULL, //draw_canvas,
	d_waframe,

	{ 0,		NULL,		0,		0,			d_unused, NULL},			/* exterior		*/
	{ BORDER,	border_dep,	XAW_BORDER,	NO,			d_borders, NULL},			/* border		*/
	{ NAME,		name_dep,	XAW_TITLE,	LT | R_VARIABLE,	d_title, s_title_size},			/* title		*/
	{ CLOSER,	NULL,		XAW_CLOSE,	LT,			d_closer, s_closer_size},		/* closer		*/
	{ FULLER,	NULL,		XAW_FULL,	RT,			d_fuller, s_fuller_size},		/* fuller		*/
	{ INFO,		NULL,		XAW_INFO,	LT | R_VARIABLE,	d_info, s_info_size},			/* info line		*/
	{ SIZER,	sizer_dep,	XAW_RESIZE,	RB,			d_sizer, s_sizer_size},			/* resizer		*/
	{ UPARROW,	NULL,		XAW_UPLN,	LT,			d_uparrow, s_uparrow_size},		/* up arrow		*/
	{ DNARROW,	NULL,		XAW_DNLN,	RT,			d_dnarrow, s_dnarrow_size},		/* down arrow		*/
	{ VSLIDE,	vslide_dep,	XAW_VSLIDE,	LT | R_VARIABLE,	d_vslide, s_vslide_size},		/* vertical slider	*/
	{ LFARROW,	NULL,		XAW_LFLN,	LT,			d_lfarrow, s_lfarrow_size},		/* left arrow		*/
	{ RTARROW,	NULL,		XAW_RTLN,	RT,			d_rtarrow, s_rtarrow_size},		/* right arrow		*/
	{ HSLIDE,	hslide_dep,	XAW_HSLIDE,	LT | R_VARIABLE,	d_hslide, s_hslide_size},		/* horizontal slider	*/
	{ ICONIFIER,	NULL,		XAW_ICONIFY,	RT,			d_iconifier, s_iconifier_size},		/* iconifier		*/
	{ HIDER,	NULL,		XAW_HIDE,	RT,			d_hider, s_hider_size},			/* hider		*/
	{ 0,		NULL,		XAW_TOOLBAR,	0,			NULL, NULL},				/* Toolbar		*/
	{ XaMENU,	NULL,		XAW_MENU,	LT | R_VARIABLE,	NULL, s_menu_size },			/* menu bar		*/

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
	&def_theme.uparrow,
	&def_theme.vslide,
	&def_theme.dnarrow,
	&def_theme.sizer,
	NULL,
};
struct render_widget *row5[] =
{
	&def_theme.lfarrow,
	&def_theme.hslide,
	&def_theme.rtarrow,
	NULL,
};

struct nwidget_row def_layout[] =
{
	{(XAR_START),			(NAME | CLOSER | FULLER | ICONIFIER | HIDE), row1},
// 	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER), row4},
	{(XAR_START),			INFO, row2},
	{(XAR_START),			XaMENU, row3},
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER), row4},
	{(XAR_END),			(LFARROW | HSLIDE | RTARROW), row5},
	{0, -1, NULL},
};
/* ------------------------------------------------------------------------- */
/* ----------  Normal client window theme ---------------------------------- */
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
// 	{1, 1, 2, 2},
	{0, 0, 0, 0},

	draw_pu_canvas,
	d_waframe,

	{ 0,		NULL,		0,		0,			d_unused, NULL},			/* exterior		*/
	{ BORDER,	pu_border_dep,	XAW_BORDER,	NO,			d_borders, NULL},			/* border		*/
	{ NAME,		pu_name_dep,	XAW_TITLE,	LT | R_VARIABLE,	d_title, s_title_size},			/* title		*/
	{ CLOSER,	NULL,		XAW_CLOSE,	LT,			d_closer, s_closer_size},		/* closer		*/
	{ FULLER,	NULL,		XAW_FULL,	RT,			d_fuller, s_fuller_size},		/* fuller		*/
	{ INFO,		NULL,		XAW_INFO,	LT | R_VARIABLE,	d_info, s_info_size},			/* info line		*/
	{ SIZER,	pu_sizer_dep,	XAW_RESIZE,	RB,			d_sizer, s_sizer_size},			/* resizer		*/
	{ UPARROW,	NULL,		XAW_UPLN,	LT,			d_uparrow, s_uparrow_size},		/* up arrow		*/
	{ DNARROW,	NULL,		XAW_DNLN,	RT,			d_dnarrow, s_dnarrow_size},		/* down arrow		*/
	{ VSLIDE,	pu_vslide_dep,	XAW_VSLIDE,	LT | R_VARIABLE,	d_vslide, s_vslide_size},		/* vertical slider	*/
	{ LFARROW,	NULL,		XAW_LFLN,	LT,			d_lfarrow, s_lfarrow_size},		/* left arrow		*/
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
	&pu_def_theme.uparrow,
	&pu_def_theme.vslide,
	&pu_def_theme.dnarrow,
	&pu_def_theme.sizer,
	NULL,
};
struct render_widget *pu_row5[] =
{
	&pu_def_theme.lfarrow,
	&pu_def_theme.hslide,
	&pu_def_theme.rtarrow,
	NULL,
};

struct nwidget_row pu_def_layout[] =
{
	{(XAR_START),			(NAME | CLOSER | FULLER | ICONIFIER | HIDE), pu_row1},
// 	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER), pu_row4},
	{(XAR_START),			INFO, pu_row2},
	{(XAR_START),			XaMENU, pu_row3},
	{(XAR_END | XAR_VERT),		(UPARROW | VSLIDE | DNARROW | SIZER), pu_row4},
	{(XAR_END),			(LFARROW | HSLIDE | RTARROW), pu_row5},
	{0, -1, NULL},
};
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
struct window_colours
{
	struct xa_data_hdr	h;
	long			links;
	
	short 			waframe_col;
	short 			frame_col;

	struct xa_wcol_inf	win;
	struct xa_wcol_inf	borders;

	struct xa_wcol_inf	slider;
	struct xa_wcol_inf	slide;

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
	struct xa_wcol_inf	rtarrow;

	struct xa_wtxt_inf	title_txt;
	struct xa_wtxt_inf	info_txt;
};

struct window_colours def_otop_cols =
{
 { 0 },	0L,	/* data header, links */

 G_LBLACK, /* window workarea frame color */
 G_LBLACK, /* window frame color */
/*          flags                        wrmode      color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 { WCOL_DRAWBKG/*|WCOL_BOXED*/, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/
 
  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_CYAN,   FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_CYAN, NULL},	/* Normal */
                                                    {G_CYAN,   FIS_SOLID,   0,   G_BLACK,     1,    G_CYAN, G_WHITE, NULL},		/* Selected */
                                                    {G_BLUE,   FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},	/* Highlighted */
/* Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LBLACK, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LBLACK, FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LBLACK, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   0,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_LBLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Info */
 { WCOL_DRAW3D|WCOL_DRAWBKG,		MD_REPLACE, {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL},	/* Selected */
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL}}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE, 
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */

/* flags	                     fontId  size wrmode    Effect      forground       background	*/
/*								           col	          col		*/
 /* Title text-info */
 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
                                      {0,     10, MD_TRANS, 0,        G_BLACK,	G_WHITE },	/* Normal */
                                      {0,     10, MD_TRANS, 0,        G_BLACK,	G_WHITE },	/* Selected */
                                      {0,     10, MD_TRANS, 0,        G_BLACK,	G_WHITE }},	/* Highlighted */
 /* Info text-info */
 { 0,
                                      {0,     9,  MD_TRANS, 0,        G_BLACK,	G_WHITE },	/* Normal */
                                      {0,     9,  MD_TRANS, 0,        G_BLACK,	G_WHITE },	/* Selected */
                                      {0,     9,  MD_TRANS, 0,        G_BLACK,	G_WHITE }},	/* Highlighted */

};

struct window_colours def_utop_cols =
{
 { 0 },	0L,	/* data header */
 
 G_LBLACK,	/* Window work area frame color */
 G_LBLACK,	/* window frame color */
/*          flags                        wrmode      color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 /* Window color (when unused window exterior is drawn) */
 { WCOL_DRAWBKG/*|WCOL_BOXED*/,                      MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   0,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   0,   G_LBLACK,     1,    G_WHITE, G_LBLACK, NULL},
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}},	/* window areas not covered by a widget/ unused widgets*/
 
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LWHITE, NULL},	/* Normal */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_LWHITE, G_WHITE,  NULL},	/* Selected */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}},	/* Highlighted */
 /* Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}},	/* Highlighted */
 /* Slide */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LBLACK, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LBLACK, FIS_SOLID,   0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LBLACK, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Title */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Info */
 { WCOL_DRAWBKG,			MD_REPLACE, {G_LWHITE, FIS_SOLID,   0,   G_LBLACK,    1,    G_LBLACK,G_BLACK, NULL},	/* Normal */
                                                    {G_LWHITE, FIS_SOLID,   0,   G_LBLACK,    1,    G_LBLACK,G_BLACK, NULL},	/* Selected */
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_LBLACK,G_BLACK, NULL}}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG, MD_REPLACE, {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL},	/* Normal */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_LBLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_LWHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_LBLACK, NULL}}, /* Highlighted */

/* flags	                     fontId  size wrmode    Effect      forground       background	*/
/*								           col	          col		*/
 /* Title text-info */
 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
                                      {0,     10, MD_TRANS, 0,        G_LBLACK,	G_WHITE },	/* Normal */
                                      {0,     10, MD_TRANS, 0,        G_LBLACK,	G_WHITE },	/* Selected */
                                      {0,     10, MD_TRANS, 0,        G_BLACK,	G_WHITE }},	/* Highlighted */
 /* Info text-info */
 { 0,		      		      {0,     9,  MD_TRANS, 0,        G_LBLACK,	G_WHITE },	/* Normal */
                                      {0,     9,  MD_TRANS, 0,        G_LBLACK,	G_WHITE },	/* Selected */
                                      {0,     9,  MD_TRANS, 0,        G_BLACK,	G_WHITE }},	/* Highlighted */
};
/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */
struct window_colours mono_def_otop_cols =
{
 { 0 },	0L,	/* data header, links */

 G_BLACK,	/* window workarea frame color */
 G_BLACK,	/* window frame color */
/*          flags                        wrmode      color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 { WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/
 
  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,   FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,   FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,   FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},	/* Highlighted */
/* Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* Slide */
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
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Selected */
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL}}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE, 
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */

/* flags	                     fontId  size wrmode    Effect      forground       background	*/
/*								           col	          col		*/
 /* Title text-info */
 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
                                      {1,     10, MD_REPLACE, 0,        G_BLACK,	G_WHITE },	/* Normal */
                                      {1,     10, MD_REPLACE, 0,        G_BLACK,	G_WHITE },	/* Selected */
                                      {1,     10, MD_REPLACE, 0,        G_BLACK,	G_WHITE }},	/* Highlighted */
 /* Info text-info */
 { 0,		      		      {1,     9,  MD_REPLACE, 0,        G_BLACK,	G_WHITE },	/* Normal */
                                      {1,     9,  MD_REPLACE, 0,        G_BLACK,	G_WHITE },	/* Selected */
                                      {1,     9,  MD_REPLACE, 0,        G_BLACK,	G_WHITE }},	/* Highlighted */

};

struct window_colours mono_def_utop_cols =
{
 { 0 },	0L,	/* data header, links */

 G_BLACK, /* window workarea frame color */
 G_BLACK, /* window frame color */
/*          flags                        wrmode      color       fill    fill    box       box        tl      br		*/
/*                                                             interior  style  color    thickness  colour  colour		*/
 { WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},
                                                    {G_WHITE, FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* window areas not covered by a widget/ unused widgets*/
 
  { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_BLACK,   FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_BLACK,   FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_BLACK,   FIS_SOLID,   0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}},	/* Highlighted */
/* Slider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_BLACK, G_WHITE,  NULL},	/* Selected */
                                                    {G_WHITE, FIS_SOLID,    0,  G_BLACK,     1,    G_WHITE, G_BLACK,  NULL}}, /* Highlighted */
 /* Slide */
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
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL},	/* Selected */
                                                    {G_WHITE,  FIS_SOLID,   0,   G_BLACK,     1,    G_BLACK,G_BLACK, NULL}}, /* Highlighted */
 /* Closer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Hider */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Iconifier */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Fuller */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* Sizer */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* UP Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE, 
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* DN Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* LF Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */
 /* RT Arrow */
 { WCOL_DRAW3D|WCOL_ACT3D|WCOL_DRAWBKG|WCOL_BOXED, MD_REPLACE,
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL},	/* Normal */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_BLACK, G_WHITE, NULL},	/* Selected */
                                                    {G_WHITE,	FIS_SOLID, 0,   G_BLACK,     1,    G_WHITE, G_BLACK, NULL}}, /* Highlighted */

/* struct xa_wtxt_inf */
/* flags	                     fontId  size   wrmode    Effect           forground       background	*/
/*								                  col	          col		*/
 /* Title text-info */
 { WTXT_DRAW3D|WTXT_ACT3D|WTXT_CENTER,
                                      {1,     10, MD_REPLACE, TXT_LIGHT,        G_BLACK,	G_WHITE },	/* Normal */
                                      {1,     10, MD_REPLACE, TXT_LIGHT,        G_BLACK,	G_WHITE },	/* Selected */
                                      {1,     10, MD_REPLACE, TXT_LIGHT,        G_BLACK,	G_WHITE }},	/* Highlighted */
 /* Info text-info */
 { 0,		      		      {1,     9,  MD_REPLACE, TXT_LIGHT,        G_BLACK,	G_WHITE },	/* Normal */
                                      {1,     9,  MD_REPLACE, TXT_LIGHT,        G_BLACK,	G_WHITE },	/* Selected */
                                      {1,     9,  MD_REPLACE, TXT_LIGHT,        G_BLACK,	G_WHITE }},	/* Highlighted */
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

#if 0
static void
draw_canvas(struct xa_window *wind, RECT *outer, RECT *inner, const RECT *clip)
{
// 	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->borders;
	struct xa_vdi_settings *v = wind->vdi_settings;
	short size;
// 	RECT r;

	if ((outer->w | outer->h | inner->w | outer->h))
	{
		size = -3;

		(*v->api->wr_mode)(v, MD_REPLACE);
		
		(*v->api->br_hook)(v, 0, outer, G_BLACK);
		(*v->api->br_hook)(v, -1, outer, G_LBLACK);
// 		(*v->api->br_hook)(v, -2, outer, G_LWHITE);
// 		(*v->api->br_hook)(v, -3, outer, G_LWHITE);
// 		(*v->api->br_hook)(v, -4, outer, G_LWHITE);
// 		(*v->api->br_hook)(v, -5, outer, G_LWHITE);
		
		
		(*v->api->tl_hook)(v, 0, outer, G_BLACK);
		(*v->api->tl_hook)(v, -1, outer, G_WHITE);
// 		(*v->api->tl_hook)(v, -2, outer, G_LWHITE);
// 		(*v->api->tl_hook)(v, -3, outer, G_LWHITE);
// 		(*v->api->tl_hook)(v, -4, outer, G_LWHITE);
// 		(*v->api->tl_hook)(v, -5, outer, G_LWHITE);
		
// 		(*v->api->tl_hook)(v, 1, inner, G_LBLACK);
// 		(*v->api->tl_hook)(v, 2, inner, G_LBLACK);
// 		(*v->api->tl_hook)(v, 3, inner, G_LWHITE);
// 		(*v->api->tl_hook)(v, 4, inner, G_LWHITE);
// 		(*v->api->tl_hook)(v, 5, inner, G_LWHITE);
// 		(*v->api->tl_hook)(v, 6, inner, G_LWHITE);
		
// 		(*v->api->br_hook)(v, 1, inner, G_WHITE);
// 		(*v->api->br_hook)(v, 2, inner, G_LWHITE);
// 		(*v->api->br_hook)(v, 3, inner, G_LWHITE);
// 		(*v->api->br_hook)(v, 4, inner, G_LWHITE);
// 		(*v->api->br_hook)(v, 5, inner, G_LWHITE);
// 		(*v->api->br_hook)(v, 6, inner, G_LWHITE);

#if 0		
		(*v->api->top_line)(v, 0, outer, G_BLUE);
		(*v->api->left_line)(v, 0, outer, G_BLUE);
		(*v->api->bottom_line)(v, 0, outer, G_LBLUE);
		(*v->api->right_line)(v, 0, outer, G_LBLUE);
	
		(*v->api->top_line)(v, 1, inner, G_GREEN);
		(*v->api->left_line)(v, 1, inner, G_GREEN);
		(*v->api->bottom_line)(v, 1, inner, G_LGREEN);
		(*v->api->right_line)(v, 1, inner, G_LGREEN);
		

		(*v->api->f_color)(v, G_LBLACK);
		r.x = outer->x;
		r.y = outer->y;
		r.w = outer->w;
		r.h = inner->y - outer->y;
		(*v->api->gbar)(v, size, &r);
		
		r.x = outer->x;
		r.w = inner->x - outer->x;
		r.y = inner->y - 2;
		r.h = inner->h + 4;
		(*v->api->gbar)(v, size, &r);

		r.x = outer->x;
		r.y = inner->y + inner->h;
		r.w = outer->w;
		r.h = (outer->y + outer->h) - r.y;
		(*v->api->gbar)(v, size, &r);

		r.x = inner->x + inner->w;
		r.w = (outer->x + outer->w) - r.x;
		r.y = inner->y - 2;
		r.h = inner->h + 4;
		(*v->api->gbar)(v, size, &r);
#endif
	}
}
#endif

static void
draw_pu_canvas(struct xa_window *wind, RECT *outer, RECT *inner, const RECT *clip)
{
// 	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->borders;
	struct xa_vdi_settings *v = wind->vdi_settings;
	short size;
// 	RECT r;

	if ((outer->w | outer->h | inner->w | outer->h))
	{
		(*v->api->wr_mode)(v, MD_REPLACE);
		if (MONO)
		{
			(*v->api->br_hook)(v, 0, outer, G_BLACK);
			(*v->api->br_hook)(v, -1, outer, G_WHITE);
		
			(*v->api->tl_hook)(v, 0, outer, G_BLACK);
			(*v->api->tl_hook)(v, -1, outer, G_WHITE);
		}
		else
		{
		
			size = -3;

		
			(*v->api->br_hook)(v, 0, outer, G_BLACK);
			(*v->api->br_hook)(v, -1, outer, G_LBLACK);
		
			(*v->api->tl_hook)(v, 0, outer, G_BLACK);
			(*v->api->tl_hook)(v, -1, outer, G_WHITE);
		}
	}
}
/*
 * Draw a window widget
 */
static void
draw_widg_box(struct xa_vdi_settings *v, short d, struct xa_wcol_inf *wcoli, short state, RECT *wr, RECT *anch)
{
	struct xa_wcol *wcol;
	struct xa_wtexture *wext;
	bool sel = state & OS_SELECTED;
	short f = wcoli->flags, o = 0;
	RECT r = *wr;

	(*v->api->wr_mode)(v, wcoli->wrm);
	
	if (sel)
		wcol = &wcoli->s;
	else
		wcol = &wcoli->n;

	wext = wcol->texture;

	if (d)
	{
		r.x -= d;
		r.y -= d;
		r.w += d+d;
		r.h += d+d;
	}

	if (f & WCOL_BOXED)
	{
		int i = wcol->box_th;
		(*v->api->l_color)(v, wcol->box_c);
		while (i > 0)
			(*v->api->gbox)(v, o, &r), o--, i--;
	}
		
	if (f & WCOL_DRAW3D)
	{
		if (sel)
		{
			(*v->api->tl_hook)(v, o, &r, wcol->tlc);
			(*v->api->br_hook)(v, o, &r, wcol->brc);
		}
		else
		{
			(*v->api->br_hook)(v, o, &r, wcol->brc);
			(*v->api->tl_hook)(v, o, &r, wcol->tlc);
		}
		o -= 1;
	}

	if (f & WCOL_DRAWBKG)
	{
		(*v->api->f_interior)(v, wcol->i);
		if (wcol->i > 1)
			(*v->api->f_style)(v, wcol->f);
	
		(*v->api->f_color)(v, wcol->c);
		(*v->api->gbar)(v, o, &r);
	}
	
	if (wext)
	{
		r.x -= o;
		r.y -= o;
		r.w += o + o;
		r.h += o + o;
		(*v->api->draw_texture)(v, wext->body, &r, anch);
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
	OBJECT *ob = wt->tree + ind;

	if (widg->state & OS_SELECTED)
		ob->ob_state |= OS_SELECTED;
	else
		ob->ob_state &= ~OS_SELECTED;

	x = widg->ar.x, y = widg->ar.y;

	(*api->object_spec_wh)(ob, &w, &h);
	x += (widg->ar.w - w) >> 1;
	y += (widg->ar.h - h) >> 1;
	display_object(0, wt, v, ind, x, y, 0);
}

static void
d_waframe(struct xa_window *wind, const RECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	RECT *wa = &wind->wa;

	if (wind->thinwork)
	{
		if (wind->wa_frame && wind->wa_borders)
		{
			short waframe_col = ((struct window_colours *)wind->colours)->waframe_col;
			
			if (wind->wa_borders & WAB_LEFT)
				(*v->api->left_line)(v, 1, wa, waframe_col); //wind->colours->frame_col);
			if (wind->wa_borders & WAB_RIGHT)
				(*v->api->right_line)(v, 1, wa, waframe_col); //wind->colours->frame_col);
			if (wind->wa_borders & WAB_TOP)
				(*v->api->top_line)(v, 1, wa, waframe_col); //wind->colours->frame_col);
			if (wind->wa_borders & WAB_BOTTOM)
				(*v->api->bottom_line)(v, 1, wa, waframe_col); //wind->colours->frame_col);
		}
	}
	/*
	 * Dont worry about mono colours, because XaAES only allows
	 * thinwork in mono.
	 */
	else
	{
		short sc = G_LBLACK, lc = G_WHITE;

		(*v->api->br_hook)(v, 2, wa, sc);
		(*v->api->tl_hook)(v, 2, wa, lc);
		(*v->api->br_hook)(v, 1, wa, lc);
		(*v->api->tl_hook)(v, 1, wa, sc);
	}
}

static bool _cdecl
d_unused(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wc = &((struct window_colours *)wind->colours)->win;
	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wc, 0, &widg->ar, &wind->r);
	return true;
}

static bool _cdecl
d_borders(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->borders;
	struct xa_vdi_settings *v = wind->vdi_settings;
	short size = wind->frame;
	RECT r;
	
	if (wind->frame > 0)
	{
		if (wind->frame >= 4)
		{
			/* top-left box */
			r.x = wind->r.x;
			r.y = wind->r.y;
			r.w = size;
			r.h = size;
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);

			/* Left border */
			r.y += size;
			r.h = wind->r.h - (size + size + wind->y_shadow);
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);

			/* bottom-left box */
			r.y = wind->r.y + wind->r.h - (size + wind->y_shadow);
			r.h = size;
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);

			/* Bottom border */
			r.x += size;
			r.w = wind->r.w - (size + size + wind->x_shadow);
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);

			/* right-bottom box */
			r.x = wind->r.x + wind->r.w - (size + wind->x_shadow);
			r.w = size;
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);

			/* right border */
			r.y = wind->r.y + size;
			r.h = wind->r.h - (size + size + wind->y_shadow);
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);

			/* top-right box */
			r.y = wind->r.y;
			r.h = size;
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);

			/* Top border*/
			r.x = wind->r.x + size;
			r.w = wind->r.w - (size + size + wind->x_shadow);
			draw_widg_box(v, 0, wci, 0, &r, &wind->r);
		}
		else
		{
			int i;
			r = wind->r;
			r.w -= wind->x_shadow;
			r.h -= wind->y_shadow;
			(*v->api->l_color)(v, ((struct window_colours *)wind->colours)->frame_col);
			for (i = 0; i < wind->frame; i++)
			{
				(*v->api->gbox)(v, 0, &r);
				r.x++;
				r.y++;
				r.w -= 2;
				r.h -= 2;
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

static bool _cdecl
d_title(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct options *o = &wind->owner->options;
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->title;
	struct xa_wtxt_inf *wti = &((struct window_colours *)wind->colours)->title_txt;
	struct xa_vdi_settings *v = wind->vdi_settings;
	char tn[256];
	bool dial = (wind->dial & (created_for_FORM_DO|created_for_FMD_START)) != 0;

	/* Convert relative coords and window location to absolute screen location */
	(*api->rp2ap)(wind, widg, &widg->ar);
#if 0
	if (MONO)
	{
		(*v->api->f_color)(v, G_WHITE);
		(*v->api->t_color)(v, G_BLACK);
		/* with inside border */
		(*v->api->p_gbar)(v, 0, &widg->ar);
	}
	else
#endif
	{
		draw_widg_box(v, 0, wci, widg->state, &widg->ar, &wind->r);
#if 0
		/* no move, no 3D */
		if (wind->active_widgets & MOVER)
		{
			draw_widg_box(v, 0, wci, widg->state, &widg->ar, &wind->r);
		}
		else
		{
			/* XXX - ozk: hack to make mono look ok! */
			if (MONO)
			{
				(*v->api->l_color)(v, G_BLACK);
				(*v->api->p_gbar)(v, 0, &widg->ar);
				(*v->api->f_color)(v, G_WHITE);
				(*v->api->gbar)(v, -1, &widg->ar);
			}
			else
			{
				(*v->api->l_color)(v, G_BLACK);
				(*v->api->p_gbar)(v, 0, &widg->ar);
				(*v->api->f_color)(v, wci->n.c);
				(*v->api->gbar)(v, -1, &widg->ar);
			}
		}
#endif
	}

// 	(*v->api->wr_mode)(v, MD_TRANS);
	
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
				sprintf(tn, sizeof(tn), "(%s) %s", ns, widg->stuff);
			else
				ow = NULL;
		}

		if (!ow)
			sprintf(tn, sizeof(tn), "(%d) %s", wind->owner->p->pid, widg->stuff);
	}
	else
		strcpy(tn, widg->stuff);

	draw_widget_text(v, widg, wti, tn, 4, 0);
	return true;
}

static bool _cdecl
d_closer(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->closer;

	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_CLOSER);
	return true;
}

static bool _cdecl
d_fuller(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->fuller;
	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_FULL);
	return true;
}

static bool _cdecl
d_info(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->info;
	struct xa_wtxt_inf *wti = &((struct window_colours *)wind->colours)->info_txt;

	/* Convert relative coords and window location to absolute screen location */
	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widget_text(wind->vdi_settings, widg, wti, widg->stuff, 4, 0); 
	return true;
}

static bool _cdecl
d_sizer(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->sizer;

	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_SIZE);
	return true;
}

static bool _cdecl
d_uparrow(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->uparrow;

	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_UP);
	return true;
}
static bool _cdecl
d_dnarrow(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->dnarrow;

	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_DOWN);
	return true;
}
static bool _cdecl
d_lfarrow(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->lfarrow;

	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_LEFT);
	return true;
}
static bool _cdecl
d_rtarrow(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->rtarrow;

	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_RIGHT);
	return true;
}

static bool _cdecl
d_vslide(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	RECT cl;
	struct window_colours *wc = wind->colours;

	sl->flags &= ~SLIDER_UPDATE;

	/* Convert relative coords and window location to absolute screen location */
	(*api->rp2ap)(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.x = sl->r.y = 0;
		sl->r.w = widg->ar.w;
		sl->r.h = widg->ar.h;
		cl = widg->ar;
		draw_widg_box(wind->vdi_settings, 0, &wc->slider, 0, &widg->ar, &widg->ar);
		return true;
	}
	len = sl_2_pix(widg->ar.h, sl->length);
	if (len < widg_h - 3)
		len = widg_h - 3;
	
	offs = widg->ar.y + sl_2_pix(widg->ar.h - len, sl->position);

	if (offs < widg->ar.y)
		offs = widg->ar.y;
	if (offs + len > widg->ar.y + widg->ar.h)
		len = widg->ar.y + widg->ar.h - offs;

	draw_widg_box(wind->vdi_settings, 0, &wc->slide, 0, &widg->ar, &widg->ar);	
	
	sl->r.y = offs - widg->ar.y;
	sl->r.h = len;
	sl->r.w = widg->ar.w;

	cl.x = sl->r.x + widg->ar.x;
	cl.y = sl->r.y + widg->ar.y;
	cl.w = sl->r.w;
	cl.h = sl->r.h;
	
	draw_widg_box(wind->vdi_settings, 0/*-1*/, &wc->slider, widg->state, &cl, &cl);
	return true;
}

static bool _cdecl
d_hslide(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	int len, offs;
	XA_SLIDER_WIDGET *sl = widg->stuff;
	struct window_colours *wc = wind->colours;
	RECT cl;

	sl->flags &= ~SLIDER_UPDATE;

	(*api->rp2ap)(wind, widg, &widg->ar);

	if (sl->length >= SL_RANGE)
	{
		sl->r.x = sl->r.y = 0;
		sl->r.w = widg->ar.w;
		sl->r.h = widg->ar.h;
		draw_widg_box(wind->vdi_settings, 0, &wc->slider, 0, &widg->ar, &widg->ar/*&wind->r*/);
		return true;
	}
	len = sl_2_pix(widg->ar.w, sl->length);
	if (len < widg_w - 3)
		len = widg_w - 3;
	
	offs = widg->ar.x + sl_2_pix(widg->ar.w - len, sl->position);

	if (offs < widg->ar.x)
		offs = widg->ar.x;
	if (offs + len > widg->ar.x + widg->ar.w)
		len = widg->ar.x + widg->ar.w - offs;
	
	draw_widg_box(wind->vdi_settings, 0, &wc->slide, 0, &widg->ar, &widg->ar/*&wind->r*/);	
	
	sl->r.x = offs - widg->ar.x;
	sl->r.w = len;
	sl->r.h = widg->ar.h;

	cl.x = sl->r.x + widg->ar.x;
	cl.y = sl->r.y + widg->ar.y;
	cl.w = sl->r.w;
	cl.h = sl->r.h;
	
	draw_widg_box(wind->vdi_settings, 0/*-1*/, &wc->slider, widg->state, &cl, &cl/*&wind->r*/);
	return true;
}

static bool _cdecl
d_iconifier(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->iconifier;

	(*api->rp2ap)(wind, widg, &widg->ar);
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_ICONIFY); //widg->loc.rsc_index);
	return true;
}

static bool _cdecl
d_hider(struct xa_window *wind, struct xa_widget *widg, const RECT *clip)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->colours)->hider;

	(*api->rp2ap)(wind, widg, &widg->ar);	
	draw_widg_box(wind->vdi_settings, 0, wci, widg->state, &widg->ar, &wind->r);
	draw_widg_icon(wind->vdi_settings, widg, &((struct module *)wind->active_theme->module)->wwt, WIDG_HIDE); //widg->loc.rsc_index);
	return true;
}

static void _cdecl
s_title_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->ontop_cols)->title;
	struct xa_wtxt_inf *wti = &((struct window_colours *)wind->ontop_cols)->title_txt;
	struct xa_wtxt_inf *wtu = &((struct window_colours *)wind->untop_cols)->title_txt;
	struct xa_vdi_settings *v = wind->vdi_settings;
	short w, h;

	widg->r.w = widg_w;

	if (!wti->n.f)
	{
// 		(*v->api->t_font)(v, wti->n.p, 9912);
// 		if (v->font_rid != v->font_sid)
// 			(*v->api->t_font)(v, wti->n.p, 9919);
// 		if (v->font_rid != v->font_sid)
			(*v->api->t_font)(v, wti->n.p, 1);
		wti->n.f = wti->s.f = wti->h.f = wtu->n.f = wtu->s.f = wtu->h.f = v->font_sid;
	}	
	(*v->api->t_font)(v, wti->n.p, wti->n.f);
	(*v->api->t_effects)(v, wti->n.e);
	(*v->api->t_extent)(v, "A", &w, &h);
	(*v->api->t_effects)(v, 0);
	
	if ((wci->flags & (WCOL_DRAW3D|WCOL_BOXED)) || (wti->flags & WTXT_DRAW3D))
		h += 4;
	if ((wci->flags & WCOL_ACT3D) || (wti->flags & WTXT_ACT3D))
		h++;

	widg->r.h = h;
};
static void _cdecl
s_info_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_wcol_inf *wci = &((struct window_colours *)wind->ontop_cols)->info;
	struct xa_wtxt_inf *wti = &((struct window_colours *)wind->ontop_cols)->info_txt;
	struct xa_wtxt_inf *wtu = &((struct window_colours *)wind->untop_cols)->info_txt;
	struct xa_vdi_settings *v = wind->vdi_settings;
	short w, h;

	widg->r.w = widg_w;
		
	if (!wti->n.f)
	{
		(*v->api->t_font)(v, wti->n.p, 1); // 103);
// 		if (v->font_rid != v->font_sid)
// 			(*v->api->t_font)(v, wti->n.p, 13384);
		if (v->font_rid != v->font_sid)
			(*v->api->t_font)(v, wti->n.p, 1);
		wti->n.f = wti->s.f = wti->h.f = wtu->n.f = wtu->s.f = wtu->h.f = v->font_sid;
	}	
	
	(*v->api->t_font)(v, wti->n.p, wti->n.f);
	(*v->api->t_effects)(v, wti->n.e);
	(*v->api->t_extent)(v, "A", &w, &h);
	(*v->api->t_effects)(v, 0);
// 	h += 2;
 	if ((wci->flags & (WCOL_DRAW3D|WCOL_BOXED)) || (wti->flags & WTXT_DRAW3D))
 		h += 4;
 	if ((wci->flags & WCOL_ACT3D) || (wti->flags & WTXT_ACT3D))
 		h++;
// 	if ((wci->flags & WCOL_BOXED))
// 		h += 2;

	widg->r.h = h;
};

static void _cdecl
s_menu_size(struct xa_window *wind, struct xa_widget *widg)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	short w, h;

	(*v->api->t_font)(v, scrninf->standard_font_point, scrninf->standard_font_id);
	(*v->api->t_effects)(v, 0);
	(*v->api->t_extent)(v, "A", &w, &h);
	
	widg->r.h = h + 1 + 1;
	widg->r.w = wind->r.w;
}

static void _cdecl
set_widg_size(struct xa_window *wind, struct xa_widget *widg, struct xa_wcol_inf *wci, short rsc_ind)
{
	short w, h, f;
	OBJECT *ob = ((struct module *)wind->active_theme->module)->wwt.tree + rsc_ind;

	f = wci->flags;

	object_spec_wh(ob, &w, &h);

 	if (f & WCOL_DRAW3D)
 		h += 2, w += 2;
 	if (f & WCOL_BOXED)
 		h += 2, w += 2;

	widg->r.w = w;
	widg->r.h = h;
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

static void
build_bfobspec(struct xa_wcol_inf *wci, struct xa_wtxt_inf *wti, BFOBSPEC *ret)
{
	BFOBSPEC n = (BFOBSPEC){0}, s;
	
	s = n;

	if (wci)
	{
		n.framesize	= wci->n.box_th;
		n.framecol	= wci->n.box_c;
		n.fillpattern	= wci->n.i;
		n.interiorcol	= wci->n.c;
	
		s.framesize	= wci->s.box_th;
		s.framecol	= wci->s.box_c;
		s.fillpattern	= wci->s.i;
		s.interiorcol	= wci->s.c;
	}

	if (wti)
	{
		n.textcol = wti->n.fgc;

		s.textcol = wti->s.fgc;
	}
	else
	{
		n.textcol = s.textcol = G_BLACK;
	}
	n.textmode = s.textmode = MD_TRANS;

	*ret++ = n;
	*ret = s;
}

static void _cdecl
get_wcol(struct xa_window *wind, short gem_widget, BFOBSPEC *ret)
{
	struct window_colours *ontop_cols = wind->ontop_cols;
	struct window_colours *untop_cols = wind->untop_cols;
	BFOBSPEC c[4];
 
 	c[0] = c[1] = c[2] = c[3] = (BFOBSPEC){0};

	switch (gem_widget)
	{
		case W_BOX:
		{
			build_bfobspec(&ontop_cols->borders, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->borders, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_TITLE:
		{
			build_bfobspec(&ontop_cols->title, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->title, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_CLOSER:
		{
			build_bfobspec(&ontop_cols->closer, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->closer, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_NAME:
		{
			build_bfobspec(&ontop_cols->title, &ontop_cols->title_txt, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->title, &untop_cols->title_txt, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_FULLER:
		{
			build_bfobspec(&ontop_cols->fuller, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->fuller, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_INFO:
		{
			build_bfobspec(&ontop_cols->info, &ontop_cols->info_txt, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->info, &untop_cols->info_txt, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_DATA:
		{
			BFOBSPEC ot, ut;
		 
		 	ot = ut = (BFOBSPEC){0};

			ot.framesize = ut.framesize = wind->thinwork ? 1 : 2;
			ot.framecol = ontop_cols->waframe_col;
			ut.framecol = untop_cols->waframe_col;
			break;
		}
		case W_WORK:
		{
			c[0] = c[1] = c[2] = c[3] = (BFOBSPEC){0};
			break;
		}
		case W_SIZER:
		{
			build_bfobspec(&ontop_cols->sizer, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->sizer, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_HBAR:
		case W_VBAR:
		{
			build_bfobspec(&ontop_cols->slide, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->slide, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_UPARROW:
		{
			build_bfobspec(&ontop_cols->uparrow, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->uparrow, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_DNARROW:
		{
			build_bfobspec(&ontop_cols->dnarrow, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->dnarrow, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_HSLIDE:
		case W_VSLIDE:
		{
			build_bfobspec(&ontop_cols->slide, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->slide, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_HELEV:
		case W_VELEV:
		{
			build_bfobspec(&ontop_cols->slider, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->slider, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_LFARROW:
		{
			build_bfobspec(&ontop_cols->lfarrow, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->lfarrow, NULL, (BFOBSPEC *)&c[2]);
			break;
		}
		case W_RTARROW:
		{
			build_bfobspec(&ontop_cols->rtarrow, NULL, (BFOBSPEC *)&c[0]);
			build_bfobspec(&untop_cols->rtarrow, NULL, (BFOBSPEC *)&c[2]);
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
	DIAGS(("widget set at %lx", def_widgets));

	for (i = 1; i < WIDG_CFG; i++)
		fix_widg(def_widgets + i);
}

struct widg_texture
{
	struct xa_data_hdr h;
	struct xa_wtexture t;
	MFDB mfdb;
};

static void
set_texture(struct xa_wcol_inf *wcol, struct xa_wtexture *wtexture)
{
	if (wtexture)
	{
		wcol->flags &= ~(WCOL_DRAWBKG); //|WCOL_BOXED);
		wcol->h.texture = wtexture;
		wcol->n.texture = wtexture;
		wcol->s.texture = wtexture;
	}
	else
	{
		wcol->flags |= WCOL_DRAWBKG; //|WCOL_BOXED;
		wcol->h.texture = NULL;
		wcol->n.texture = NULL;
		wcol->s.texture = NULL;
	}
}
	
static void
cleanup_deftheme(void)
{
#if 0	
	win
	borders
	slider
	slide
	title
	info
	closer
	hider
	iconifier
	fuller
	sizer
	uparrow
	dnarrow
	lfarrow
	rtarrow
#endif
	set_texture(&def_otop_cols.win, NULL);
	set_texture(&def_otop_cols.borders, NULL);
 	set_texture(&def_otop_cols.slider, NULL);
 	set_texture(&def_otop_cols.slide, NULL);
	set_texture(&def_otop_cols.title, NULL);
 	set_texture(&def_otop_cols.info, NULL);
	set_texture(&def_otop_cols.closer, NULL);
	set_texture(&def_otop_cols.hider, NULL);
	set_texture(&def_otop_cols.iconifier, NULL);
	set_texture(&def_otop_cols.fuller, NULL);
	set_texture(&def_otop_cols.sizer, NULL);
	set_texture(&def_otop_cols.uparrow, NULL);
	set_texture(&def_otop_cols.dnarrow, NULL);
	set_texture(&def_otop_cols.lfarrow, NULL);
	set_texture(&def_otop_cols.rtarrow, NULL);

	set_texture(&def_utop_cols.win, NULL);
	set_texture(&def_utop_cols.borders, NULL);
 	set_texture(&def_utop_cols.slider, NULL);
 	set_texture(&def_utop_cols.slide, NULL);
	set_texture(&def_utop_cols.title, NULL);
 	set_texture(&def_utop_cols.info, NULL);
	set_texture(&def_utop_cols.closer, NULL);
	set_texture(&def_utop_cols.hider, NULL);
	set_texture(&def_utop_cols.iconifier, NULL);
	set_texture(&def_utop_cols.fuller, NULL);
	set_texture(&def_utop_cols.sizer, NULL);
	set_texture(&def_utop_cols.uparrow, NULL);
	set_texture(&def_utop_cols.dnarrow, NULL);
	set_texture(&def_utop_cols.lfarrow, NULL);
	set_texture(&def_utop_cols.rtarrow, NULL);
}

static int imgpath_file = 0;
static char imgpath[128];

static void _cdecl
delete_texture(void *_t)
{
	struct widg_texture *t = _t;

	if (t->mfdb.fd_addr)
		kfree(t->mfdb.fd_addr);
	kfree(t);
}

static struct widg_texture *
load_texture(struct module *m, char *fn)
{
	struct widg_texture *t = NULL;

	imgpath[imgpath_file] = '\0';
	strcat(imgpath, fn);
	if ((fn = (*api->sysfile)(imgpath)))
	{
		if ((t = (*api->kmalloc)(sizeof(*t))))
		{
			(*api->load_img)(fn, &t->mfdb);
			if (t->mfdb.fd_addr)
			{
				(*api->add_xa_data)(&m->allocs, t, NULL, delete_texture);
				t->t.flags = 0;
				t->t.anchor = 0;
				t->t.left = t->t.right = t->t.top = t->t.bottom = NULL;
				t->t.body = &t->mfdb;
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

static void
init_sliders(struct module *m)
{
	struct widg_texture *t;

	if ((t = load_texture(m, "slider.img"))) //"rock011.img")))
	{
		set_texture(&def_otop_cols.slider, &t->t);
		set_texture(&def_utop_cols.slider, &t->t);
	}
	if ((t = load_texture(m, "slide.img"))) //"drock028.img")))
	{
		set_texture(&def_otop_cols.slide, &t->t);
		set_texture(&def_utop_cols.slide, &t->t);
	}
}

static void
info_texture(struct module *m)
{
	struct widg_texture *t;

	if ((t = load_texture(m, "info.img"))) //"lgrey008.img")))
	{
		set_texture(&def_otop_cols.info, &t->t);
		set_texture(&def_utop_cols.info, &t->t);
	}
}

static void
test_img_stuff(struct module *m)
{
	struct widg_texture *t;

	init_sliders(m);
	info_texture(m);
	if ((t = load_texture(m, "exterior.img"))) //grey8b.img")))
	{
		set_texture(&def_otop_cols.win, &t->t);
		set_texture(&def_otop_cols.borders, &t->t);
// 		set_texture(&def_otop_cols.slider, &t->t);
// 		set_texture(&def_otop_cols.slide, &t->t);
		set_texture(&def_otop_cols.title, &t->t);
// 		set_texture(&def_otop_cols.info, &t->t);
		set_texture(&def_otop_cols.closer, &t->t);
		set_texture(&def_otop_cols.hider, &t->t);
		set_texture(&def_otop_cols.iconifier, &t->t);
		set_texture(&def_otop_cols.fuller, &t->t);
		set_texture(&def_otop_cols.sizer, &t->t);
		set_texture(&def_otop_cols.uparrow, &t->t);
		set_texture(&def_otop_cols.dnarrow, &t->t);
		set_texture(&def_otop_cols.lfarrow, &t->t);
		set_texture(&def_otop_cols.rtarrow, &t->t);
	
		set_texture(&def_utop_cols.win, &t->t);
		set_texture(&def_utop_cols.borders, &t->t);
// 		set_texture(&def_utop_cols.slider, &t->t);
// 		set_texture(&def_utop_cols.slide, &t->t);
		set_texture(&def_utop_cols.title, &t->t);
// 		set_texture(&def_utop_cols.info, &t->t);
		set_texture(&def_utop_cols.closer, &t->t);
		set_texture(&def_utop_cols.hider, &t->t);
		set_texture(&def_utop_cols.iconifier, &t->t);
		set_texture(&def_utop_cols.fuller, &t->t);
		set_texture(&def_utop_cols.sizer, &t->t);
		set_texture(&def_utop_cols.uparrow, &t->t);
		set_texture(&def_utop_cols.dnarrow, &t->t);
		set_texture(&def_utop_cols.lfarrow, &t->t);
		set_texture(&def_utop_cols.rtarrow, &t->t);
	}
}
#if 0		
// 	char *fn;

	init_sliders(m);
	info_texture(m);


	imgpath[imgpath_file] = '\0';
	strcat(imgpath, "grey8b.img");
	fn = (*api->sysfile)(imgpath); //"img\\8b\\grey8b.img");
	if (fn)
	{
		struct widg_texture *texture;
		
		texture = (*api->kmalloc)(sizeof(*texture));
		if (texture)
		{
			(*api->load_img)(fn, &texture->mfdb);
			if (texture->mfdb.fd_addr)
			{
				(*api->add_xa_data)(&m->allocs, texture, delete_texture);
				
				texture->t.flags = 0;
				texture->t.anchor = 0;
				texture->t.body = &texture->mfdb;
				texture->t.left = texture->t.right = texture->t.top = texture->t.bottom = NULL;
	
				set_texture(&def_otop_cols.win, &texture->t);
				set_texture(&def_otop_cols.borders, &texture->t);
// 				set_texture(&def_otop_cols.slider, &texture->t);
// 				set_texture(&def_otop_cols.slide, &texture->t);
				set_texture(&def_otop_cols.title, &texture->t);
// 				set_texture(&def_otop_cols.info, &texture->t);
				set_texture(&def_otop_cols.closer, &texture->t);
				set_texture(&def_otop_cols.hider, &texture->t);
				set_texture(&def_otop_cols.iconifier, &texture->t);
				set_texture(&def_otop_cols.fuller, &texture->t);
				set_texture(&def_otop_cols.sizer, &texture->t);
				set_texture(&def_otop_cols.uparrow, &texture->t);
				set_texture(&def_otop_cols.dnarrow, &texture->t);
				set_texture(&def_otop_cols.lfarrow, &texture->t);
				set_texture(&def_otop_cols.rtarrow, &texture->t);
			
				set_texture(&def_utop_cols.win, &texture->t);
				set_texture(&def_utop_cols.borders, &texture->t);
// 				set_texture(&def_utop_cols.slider, &texture->t);
// 				set_texture(&def_utop_cols.slide, &texture->t);
				set_texture(&def_utop_cols.title, &texture->t);
// 				set_texture(&def_utop_cols.info, &texture->t);
				set_texture(&def_utop_cols.closer, &texture->t);
				set_texture(&def_utop_cols.hider, &texture->t);
				set_texture(&def_utop_cols.iconifier, &texture->t);
				set_texture(&def_utop_cols.fuller, &texture->t);
				set_texture(&def_utop_cols.sizer, &texture->t);
				set_texture(&def_utop_cols.uparrow, &texture->t);
				set_texture(&def_utop_cols.dnarrow, &texture->t);
				set_texture(&def_utop_cols.lfarrow, &texture->t);
				set_texture(&def_utop_cols.rtarrow, &texture->t);
			}
			else
				(*api->kfree)(texture);
		}
		(*api->kfree)(fn);
	}
}
#endif

/*
 * This function is called by XaAES to have the module initialize itself
 */
static void * _cdecl
init_module(const struct xa_module_api *xmapi, const struct xa_screen *screen, char *widg_name)
{
	char *rscfile;
	RSHDR *rsc;
	struct module *m;

// 	display("wind_draw: initmodule:");

	api	= xmapi;
	scrninf	= screen;

	current_theme = &def_theme;
	current_pu_theme = &pu_def_theme;

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
			rsc = (*api->load_resource)(rscfile, NULL, screen->c_max_w, screen->c_max_h, false);
			DIAGS(("widget_resources = %lx (%s)", rsc, widg_name));
			kfree(rscfile);
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
			RECT c;
			OBJECT *tree = (*api->resource_tree)(rsc, 0);
			(*api->ob_spec_xywh)(tree, 1, &c);
			(*api->init_wt)(&m->wwt, tree);
			
// 			display(" -- init widget_tree=%lx", (long)&m->wwt);
			
			widg_w = c.w;
			widg_h = c.h;

			DIAGS(("widg: %d/%d", widg_w, widg_h));
		}
		fix_default_widgets(rsc);

		
		if (screen->planes >= 8)
		{
			strcpy(imgpath, "img\\");
			strcat(imgpath, screen->planes == 8 ? "8b\\" : "hc\\");
			imgpath_file = strlen(imgpath);
			test_img_stuff(m);
		}
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

// 	display(" -- module init done!");
	return m;
}

static void _cdecl
exit_module(void *_module)
{
	struct module *m = _module;
	
	/*
	 * This will free all allocs done by this module
	 */
	(*api->free_xa_data_list)(&m->allocs);

	cleanup_deftheme();
	/*
	 * for now, the resource loaded is attached to AESSYS
	 * and cannot be freed here..
	 */
	(*api->remove_wt)(&m->wwt);
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
		default:
		{
			new = duplicate_theme(current_theme);
			break;
		}
	}

	if (new)
	{
		new->module = m;
		(*api->add_xa_data)(&m->allocs, new, NULL, delete_theme);
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
new_color_theme(void *_module, void **ontop, void **untop)
{
	struct window_colours *new_ontop, *new_untop;
	struct module *m = _module;
	long ret;

	new_ontop = (*api->kmalloc)(sizeof(*new_ontop));
	new_untop = (*api->kmalloc)(sizeof(*new_untop));
	
	if (new_ontop && new_untop)
	{
		*new_ontop = MONO ? mono_def_otop_cols : def_otop_cols;
		*new_untop = MONO ? mono_def_utop_cols : def_utop_cols;
		
		(*api->add_xa_data)(&m->allocs, new_ontop, NULL, delete_color_theme);
		(*api->add_xa_data)(&m->allocs, new_untop, NULL, delete_color_theme);

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
		(*api->delete_xa_data)(&m->allocs, wc);
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
