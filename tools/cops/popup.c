/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdio.h>

#include "cops.h"
#include "objmacro.h"
#include "popup.h"


static void _cdecl strs_init(struct POPUP_INIT_args);
static short _cdecl draw_arrow(PARMBLK *parmblock);

/*----------------------------------------------------------------------------------------*/ 
/* BITBLKs fuer Pfeile nach oben und unten */
/*----------------------------------------------------------------------------------------*/ 
static short up_arrow_data[] = { 0x0000, 0x0100, 0x0380, 0x07C0, 0x0FE0, 0x1FF0, 0x3FF8, 0x0000 };
static short dn_arrow_data[] = { 0x0000, 0x3FF8, 0x1FF0, 0x0FE0, 0x07C0, 0x0380, 0x0100, 0x0000 };

static BITBLK up_arrow = { up_arrow_data, 2, 8, 0, 0, 0x0001 };
static BITBLK dn_arrow = { dn_arrow_data, 2, 8, 0, 0, 0x0001 };

static USERBLK up_userdef = { draw_arrow, (long) &up_arrow };
static USERBLK dn_userdef = { draw_arrow, (long) &dn_arrow };

#define	MAX_STRS 20

typedef struct
{
	char **strs;
	short no_strs;
	short max_strs;
	short spaces;
	short slct;
	char *buf_str[MAX_STRS];
} PSTRS;

short
do_obj_popup(OBJECT *parent_tree, short parent_obj, char **strs, short no_strs, short no_spaces, short slct)
{
	GRECT button_rect;

	objc_offset(parent_tree, parent_obj, &button_rect.g_x, &button_rect.g_y);
	button_rect.g_w = parent_tree[parent_obj].ob_width;
	button_rect.g_h = parent_tree[parent_obj].ob_height;

	return do_popup(&button_rect, strs, no_strs, no_spaces, slct);
}

/*----------------------------------------------------------------------------------------*/ 
/* Popup behandeln */
/* Funktionsresultat:	Index des angewaehlten Strings */
/*	button_rect:	Ausmasse des Buttons */
/*	strs:		Feld mit Zeigern auf die Strings */
/*	no_strs:	Anzahl der Strings */
/*	spaces:		Anzahl der vor den Strings einzufuegenden Leerzeichen */
/*	slct:		Index des selektierten Strings */
/*----------------------------------------------------------------------------------------*/ 
short
do_popup(GRECT *button_rect, char **strs, short no_strs, short spaces, short slct)
{
	PSTRS popup_par;
	OBJECT *tree;
	short max_width;
	short max_strs;
	short i;
	long tree_size;

	if (no_strs > MAX_STRS)
		max_strs = MAX_STRS; /* Anzahl der sichtbaren Strings */
	else
		max_strs = no_strs;

	max_width = 0;

	for (i = 0; i < no_strs; i++) /* breitesten String bestimmen */
	{
		long len;
		
		len = strlen(strs[i]);
		
		if (len > max_width)
			max_width = (short) len;
	}

	max_width += spaces; /* maximale Stringbreite in Zeichen */
	tree_size = sizeof(OBJECT) * (1 + max_strs); /* Laenge des Objektbaums */

	tree = (void *)Malloc(tree_size + (max_strs * (max_width + 1)));

	for (i = 0; i < max_strs; i++) /* Zeiger auf die temporaeren String initialisieren */
		popup_par.buf_str[i] = ((char *) tree) + tree_size + (i * (max_width + 1));

	if (tree) /* Speicher vorhanden? */
	{
		short scroll_pos;

		max_width *= pwchar; /* Breite in Pixeln */

		if (max_width < button_rect->g_w) /* zu schmal? */
			max_width = button_rect->g_w;

		tree->ob_next = -1; /* Wurzelobjekt; weisse Hintergrundbox */

		if (max_strs > 0) /* sind Strings vorhanden? */
		{
			tree->ob_head = 1;
			tree->ob_tail = max_strs;
		}
		else /* keine Strings */
		{
			tree->ob_flags += OF_LASTOB;
			tree->ob_head = -1;
			tree->ob_tail = -1;
		}

		tree->ob_type = G_BOX;
		tree->ob_flags = OF_NONE;
		tree->ob_state = OS_NORMAL|OS_SHADOWED;
		tree->ob_spec.index = 0x00ff1000L;
		
		tree->ob_x = button_rect->g_x; /* Buttonposition */
		tree->ob_y = button_rect->g_y;
		tree->ob_width = max_width; /* Breite der Box */
		tree->ob_height = max_strs * phchar; /* Hoehe der Box */

		popup_par.strs = strs;
		popup_par.no_strs = no_strs;
		popup_par.max_strs = max_strs;
		popup_par.spaces = spaces;
		popup_par.slct = slct;

		if (no_strs <= MAX_STRS) /* kein Scrolling? */
			scroll_pos = 0;
		else
		{
			scroll_pos = slct - (MAX_STRS / 2);
			if (scroll_pos < 0)
				scroll_pos = 0;
			if (scroll_pos > (no_strs - MAX_STRS))
				scroll_pos = no_strs - MAX_STRS;
		}

		if ((slct < 0) || (slct >= no_strs))
			slct = 0;

		{
			struct POPUP_INIT_args args = { tree, scroll_pos, no_strs, (void *) &popup_par };
			strs_init(args); /* String besetzen */
		}

		tree->ob_y -= (slct - scroll_pos) * phchar; /* Popup nach oben verschieben */
		slct = xfrm_popup(tree, 0, 0, 1, max_strs, no_strs, strs_init, (void *) &popup_par, &scroll_pos);

		if (slct > 0)
		{
			slct -= 1;
			slct += scroll_pos;
		}
		else
			slct = -1;

		Mfree(tree);
	}

	return slct;
}

/*----------------------------------------------------------------------------------------*/ 
/* Strings in scrollendem Popup besetzen */
/* Funktionsresultat:	1 */
/*	tree:		Objektbaum */
/*	scroll_pos:	augenblickliche Scrollposition */
/*	nlines:		Anzahl aller vorhandenen Zeilen */
/*	param:		Zeiger auf Parameterstruktur */
/*----------------------------------------------------------------------------------------*/ 
static void _cdecl
strs_init(struct POPUP_INIT_args args)
{
	OBJECT *obj;
	PSTRS *popup_par;
	short max_strs;
	short i;

	popup_par = (PSTRS *)args.param; /* Zeiger auf Parameterstruktur */
	max_strs = popup_par->max_strs; /* Anzahl der sichtbaren Eintraege */

	obj = args.tree + 1; /* erster G_STRING */

	for (i = 1; i <= max_strs; i++) /* Objekte fuer die Strings erzeugen */
	{
		char *tmp;
		short j;

		tmp = popup_par->buf_str[i - 1]; /* Zeiger auf den Stringbuffer */

		if (i < max_strs) /* noch weitere G_STRINGs? */
			obj->ob_next = i + 1;
		else
			obj->ob_next = 0;

		obj->ob_head = -1;
		obj->ob_tail = -1;
		obj->ob_type = G_STRING;
		obj->ob_flags = OF_SELECTABLE;

		if (i == max_strs) /* letzter Eintrag? */
			obj->ob_flags |= OF_LASTOB;

		obj->ob_state = OS_NORMAL;
		if ((i + args.scrollpos - 1) == popup_par->slct) /* ausgewaehlter Eintrag? */
			obj->ob_state |= OS_CHECKED;

		obj->ob_spec.free_string = tmp; /* Zeiger auf den String */

		obj->ob_x = 0;
		obj->ob_y = (i - 1) * phchar;
		obj->ob_width = args.tree->ob_width;
		obj->ob_height = phchar;

		for (j = 0; j < popup_par->spaces; j++) /* Anfang mit Leerzeichen auffuellen */
			*tmp++ = ' ';

		strcpy(tmp, popup_par->strs[args.scrollpos + i - 1]); /* String kopieren */

		obj++;
	}

	if (popup_par->no_strs > MAX_STRS) /* scrollendes Popup? */
	{
		if (args.scrollpos < (args.nlines - MAX_STRS)) /* nicht die letzte Zeile? */
		{
			obj = args.tree + MAX_STRS;			/* letzter G_STRING */

			obj->ob_type = G_USERDEF;		/* durch Userdef ersetzen */
			obj->ob_state &= ~OS_CHECKED;		/* kein Haeckchen */
			obj->ob_spec.userblk = &dn_userdef;	/* mit Pfeil nach unten */
		}

		if (args.scrollpos > 0) /* nicht die erste Zeile? */
		{
			obj = args.tree + 1;				/* erster G_STRING */

			obj->ob_type = G_USERDEF;		/* durch Userdef ersetzen */
			obj->ob_state &= ~OS_CHECKED;		/* kein Haeckchen */
			obj->ob_spec.userblk = &up_userdef;	/* mit Pfeil nach oben */
		}
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* USERDEF-Funktion fuer Scrollpfeil im Popup */
/* Funktionsresultat:	nicht aktualisierte Objektstati */
/* parmblock:		Zeiger auf die Parameter-Block-Struktur */
/*----------------------------------------------------------------------------------------*/ 
static short _cdecl
draw_arrow(PARMBLK *parmblock)
{
	BITBLK *image;
	MFDB src;
	MFDB des;
	short clip[4];
	short xy[8];
	short image_colors[2];

	*(GRECT *) clip = *(GRECT *) &parmblock->pb_xc; /* Clipping-Rechteck... */
	clip[2] += clip[0] - 1;
	clip[3] += clip[1] - 1;
	vs_clip(vdi_handle, 1, clip); /* Zeichenoperationen auf gegebenen Bereich beschraenken */

	image = (BITBLK *) parmblock->pb_parm;

	src.fd_addr = image->bi_pdata;
	src.fd_w = image->bi_wb * 8;
	src.fd_h = image->bi_hl;
	src.fd_wdwidth = image->bi_wb / 2;
	src.fd_stand = 0;
	src.fd_nplanes = 1;
	src.fd_r1 = 0;
	src.fd_r2 = 0;
	src.fd_r3 = 0;

	des.fd_addr = 0L;

	xy[0] = 0;
	xy[1] = 0;
	xy[2] = src.fd_w - 1;
	xy[3] = src.fd_h - 1;
	xy[4] = parmblock->pb_x + ((parmblock->pb_w - src.fd_w) / 2);
	xy[5] = parmblock->pb_y + ((parmblock->pb_h - src.fd_h) / 2);
	xy[6] = xy[4] + xy[2];
	xy[7] = xy[5] + xy[3];

	image_colors[0] = 1; /* schwarz als Vordergrundfarbe */
	image_colors[1] = 0; /* Hintergrundfarbe */

	vrt_cpyfm(vdi_handle, MD_REPLACE, xy, &src, &des, image_colors);

	return parmblock->pb_currstate;
}
