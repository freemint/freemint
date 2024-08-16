/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                 2004 - 2006 F.Naumann & O.Skancke
 *                                 2009 - 2010 P.Wratt
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

/*// GREEN - LIME/apple*/

#include <gem.h>
#include "../gradients.h"
short _start asm("_start") = START;

/* ---------------------------------------------------------------------------------- */
/* -----------  Standard client window colour gradients --------------------------------- */
/* ---------------------------------------------------------------------------------- */
struct xa_gradient otop_vslide_gradient =
{
	(struct xa_data_hdr *)OTOP_VSLIDE_GRADIENT,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{400,500,400},{600,700,600}},
};
struct xa_gradient otop_hslide_gradient =
{
	(struct xa_data_hdr *)OTOP_HSLIDE_GRADIENT,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{400,500,400},{600,700,600}},
};
struct xa_gradient otop_vslider_gradient =
{
	(struct xa_data_hdr *)OTOP_VSLIDER_GRADIENT,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{900,1000,900},{600,700,600}},
};
struct xa_gradient otop_hslider_gradient =
{
	(struct xa_data_hdr *)OTOP_HSLIDER_GRADIENT,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{900,1000,900},{600,700,600}},
};

struct xa_gradient utop_vslide_gradient =
{
	(struct xa_data_hdr *)UTOP_VSLIDE_GRADIENT,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{300,350,300},{500,550,500}},
};
struct xa_gradient utop_hslide_gradient =
{
	(struct xa_data_hdr *)UTOP_HSLIDE_GRADIENT,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{300,350,300},{500,550,500}},
};
struct xa_gradient utop_vslider_gradient =
{
	(struct xa_data_hdr *)UTOP_VSLIDER_GRADIENT,
	-1, 0,
	 0, 16,

	1, 0, {0},
	{{800,850,800},{500,550,500}},
};
struct xa_gradient utop_hslider_gradient =
{
	(struct xa_data_hdr *)UTOP_HSLIDER_GRADIENT,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{800,850,800},{500,550,500}},
};

struct xa_gradient otop_title_gradient =
{
	(struct xa_data_hdr *)OTOP_TITLE_GRADIENT,
	 0, -1,
	16,  0,

	0, 0, {0},
	{{700,900,700},{500,700,500}},
};
struct xa_gradient utop_title_gradient =
{
	(struct xa_data_hdr *)UTOP_TITLE_GRADIENT,
	0, -1,
	16, 0,

	0,0,{0},
	{{600,650,600},{800,850,800}},
};

struct xa_gradient otop_info_gradient =
{
	(struct xa_data_hdr *)OTOP_INFO_GRADIENT,
	0, -1,
	16, 0,

	0,0,{0},
	{{800,850,800},{950,1000,950}},
/*// 	3, 1, {3, 0},*/
/*// 	{{200,200,200},{600,600,600},{900,900,900}},*/
};
struct xa_gradient utop_info_gradient =
{
	(struct xa_data_hdr *)UTOP_INFO_GRADIENT,
	0, -1,
	16, 0,

	0, 0, {0},
	{{600,650,600},{800,850,800}},
};

struct xa_gradient otop_grey_gradient =
{
	(struct xa_data_hdr *)OTOP_GREY_GRADIENT,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{600,700,600},{900,1000,900}},
};

struct xa_gradient utop_grey_gradient =
{
	(struct xa_data_hdr *)UTOP_GREY_GRADIENT,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{500,550,500},{800,850,800}},
/*	{{500,500,500},{900,900,900}}, */
};

/* ---------------------------------------------------------------------------------- */
/* ---------------- Alert window colour gradients --------------------------------------- */
/* ---------------------------------------------------------------------------------- */

struct xa_gradient alert_otop_title_gradient =
{
	(struct xa_data_hdr *)ALERT_OTOP_TITLE_GRADIENT,
	-1, -1,
	0, 0,

	2, 0,{0},
	{{400,250,200},{900,850,800}},
};

struct xa_gradient alert_utop_title_gradient =
{
	(struct xa_data_hdr *)ALERT_UTOP_TITLE_GRADIENT,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{550,500,500},{750,700,700}},
/*	{{500,500,500},{900,900,900}}, */
};

struct xa_gradient alert_utop_grey_gradient =
{
	(struct xa_data_hdr *)ALERT_UTOP_GREY_GRADIENT,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{350,330,300},{750,730,700}},
/*	{{500,500,500},{900,900,900}}, */
};

/* ---------------------------------------------------------------------------------- */
/* ---------------- Scroll List window colour gradients --------------------------------- */
/* ---------------------------------------------------------------------------------- */
struct xa_gradient slist_otop_vslide_gradient =
{
	(struct xa_data_hdr *)SLIST_OTOP_VSLIDE_GRADIENT,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{300,350,300},{500,550,500}},
};
struct xa_gradient slist_otop_hslide_gradient =
{
	(struct xa_data_hdr *)SLIST_OTOP_HSLIDE_GRADIENT,
	0, -1,
	16, 0,

	0, 0, {0},
	{{300,350,300},{500,550,500}},
};
struct xa_gradient slist_otop_vslider_gradient =
{
	(struct xa_data_hdr *)SLIST_OTOP_VSLIDER_GRADIENT,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{800,850,800},{500,550,500}},
};
struct xa_gradient slist_otop_hslider_gradient =
{
	(struct xa_data_hdr *)SLIST_OTOP_HSLIDER_GRADIENT,
	0, -1,
	16, 0,

	0, 0, {0},
	{{800,850,800},{500,550,500}},
};
struct xa_gradient slist_utop_vslide_gradient =
{
	(struct xa_data_hdr *)SLIST_UTOP_VSLIDE_GRADIENT,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{300,350,300},{500,550,500}},
};
struct xa_gradient slist_utop_hslide_gradient =
{
	(struct xa_data_hdr *)SLIST_UTOP_HSLIDE_GRADIENT,
	0, -1,
	16, 0,

	0, 0, {0},
	{{300,350,300},{500,550,500}},
};
struct xa_gradient slist_utop_vslider_gradient =
{
	(struct xa_data_hdr *)SLIST_UTOP_VSLIDER_GRADIENT,
	-1, 0,
	0, 16,

	1, 0, {0},
	{{800,850,800},{500,550,500}},
};
struct xa_gradient slist_utop_hslider_gradient =
{
	(struct xa_data_hdr *)SLIST_UTOP_HSLIDER_GRADIENT,
	0, -1,
	16, 0,

	0, 0, {0},
	{{800,850,800},{500,550,500}},
};

struct xa_gradient slist_otop_title_gradient =
{
	(struct xa_data_hdr *)SLIST_OTOP_TITLE_GRADIENT,
	-1, -1,
	0, 0,

	2, 0,{0},
	{{00,400,00},{600,1000,600}},
};
struct xa_gradient slist_utop_title_gradient =
{
	(struct xa_data_hdr *)SLIST_UTOP_TITLE_GRADIENT,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{500,550,500},{700,750,700}},
/*	{{500,500,500},{900,900,900}}, */
};

struct xa_gradient slist_otop_info_gradient =
{
	(struct xa_data_hdr *)SLIST_OTOP_INFO_GRADIENT,
	0, -1,
	16, 0,

	3, 1, {3, 0},
	{{200,250,200},{600,650,600},{900,950,900}},
};
struct xa_gradient slist_utop_info_gradient =
{
	(struct xa_data_hdr *)SLIST_UTOP_INFO_GRADIENT,
	0, -1,
	16, 0,

	0, 0, {0},
	{{400,450,400},{700,750,700}},
};
struct xa_gradient slist_otop_grey_gradient =
{
	(struct xa_data_hdr *)SLIST_OTOP_GREY_GRADIENT,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{500,550,500},{900,950,900}},
};
struct xa_gradient slist_utop_grey_gradient =
{
	(struct xa_data_hdr *)SLIST_UTOP_GREY_GRADIENT,
	-1, -1,
	0, 0,

	2, 0, {0},
	{{300,350,300},{700,750,700}},
/*	{{500,500,500},{900,900,900}}, */
};

struct xa_gradient menu_gradient =
{
	(struct xa_data_hdr *)MENU_GRADIENT,
	 0, -1,
	16,  0,

	0, 0, {0},
/*// 	{{750,750,750},{900,900,900}},*/
	{{900,950,900},{600,650,600}},
};
struct xa_gradient sel_title_gradient =
{
	(struct xa_data_hdr *)SEL_TITLE_GRADIENT,
	0, -1,
	16, 0,
	0,0,{0},
	{{600,650,600},{950,1000,950}},
};
struct xa_gradient sel_popent_gradient =
{
	(struct xa_data_hdr *)SEL_POPENT_GRADIENT,
	0, -1,
	16, 0,
	0,0,{0},
	{{950,1000,950},{800,1000,800}},
};

struct xa_gradient indbutt_gradient =
{
	(struct xa_data_hdr *)INDBUTT_GRADIENT,
	-1,   0,
	 0,  16,

	4, 1, { -35, 0, },
	{{700,750,700},{900,950,900},{700,750,700}},
};
struct xa_gradient sel_indbutt_gradient =
{
	(struct xa_data_hdr *)SEL_INDBUTT_GRADIENT,
	-1,   0,
	 0,  16,

	4, 1, { -35, 0, },
	{{700,750,700},{500,550,500},{700,750,700}},
};
struct xa_gradient actbutt_gradient =
{
	(struct xa_data_hdr *)ACTBUTT_GRADIENT,
	0, -1,
	16, 0,

	0, 0, { 0 },
	{{900,950,900},{700,750,700}},
};

struct xa_gradient popbkg_gradient =
{
	(struct xa_data_hdr *)POPBKG_GRADIENT,
	0, -1,
	16, 0,

	3, 1, {-40, 0},
	{{800,850,800}, {900,950,900}, {700,750,700}},
};

short stop = STOP;

