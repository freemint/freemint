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
#include <mt_gemx.h>

#include "cops_rsc.h"
#include "cops.h"
#include "objmacro.h"
#include "adaptrsc.h"
#include "popup.h"

#define graf_mkstate_mrets(mk) \
	graf_mkstate(&(mk)->x,&(mk)->y,&(mk)->buttons,&(mk)->kstate)

#define inside(xx, yy, x, y, w, h) \
	((xx) >= (x) && (xx) < ((x) + (w)) && \
	 (yy) >= (y) && (yy) < ((y) + (h)))

#define rc_inside(xx, yy, gr) \
	inside(xx, yy, (gr)->g_x, (gr)->g_y, (gr)->g_w, (gr)->g_h)

/*----------------------------------------------------------------------------------------*/ 
/* BITBLKs fuer Pfeile nach oben und unten */
/*----------------------------------------------------------------------------------------*/ 
static _WORD up_arrow_data[] = { 0x0000, 0x0100, 0x0380, 0x07C0, 0x0FE0, 0x1FF0, 0x3FF8, 0x0000 };
static _WORD dn_arrow_data[] = { 0x0000, 0x3FF8, 0x1FF0, 0x0FE0, 0x07C0, 0x0380, 0x0100, 0x0000 };

static BITBLK up_arrow = { up_arrow_data, 2, 8, 0, 0, 0x0001 };
static BITBLK dn_arrow = { dn_arrow_data, 2, 8, 0, 0, 0x0001 };

#define	MAX_STRS 20

typedef struct
{
	char **strs;
	short no_strs;		/* total number of strings */
	short max_strs;		/* number of visible strings in popup */
	short spaces;
	short slct;
	short max_width;
	char *buf_str[MAX_STRS];
} PSTRS;

/*----------------------------------------------------------------------------------------*/ 
/* USERDEF-Funktion fuer Scrollpfeil im Popup */
/* Funktionsresultat:	nicht aktualisierte Objektstati */
/* parmblock:		Zeiger auf die Parameter-Block-Struktur */
/*----------------------------------------------------------------------------------------*/ 
static _WORD _cdecl
draw_arrow(PARMBLK *parmblock)
{
	BITBLK *image;
	MFDB src;
	MFDB des;
	_WORD clip[4];
	_WORD xy[8];
	_WORD image_colors[2];

	/* clipping rectangle... */
	clip[0] = parmblock->pb_xc;
	clip[1] = parmblock->pb_yc;
	clip[2] = parmblock->pb_xc + parmblock->pb_wc - 1;
	clip[3] = parmblock->pb_yc + parmblock->pb_hc - 1;
	udef_vs_clip(vdi_handle, 1, clip); /* Zeichenoperationen auf gegebenen Bereich beschraenken */

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

	udef_vrt_cpyfm(vdi_handle, MD_REPLACE, xy, &src, &des, image_colors);

	return parmblock->pb_currstate;
}

static USERBLK up_userdef = { draw_arrow, (long) &up_arrow };
static USERBLK dn_userdef = { draw_arrow, (long) &dn_arrow };


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
	const char *str;

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
		obj->ob_type = aes_flags & GAI_GSHORTCUT ? G_SHORTCUT : G_STRING;
		obj->ob_flags = OF_SELECTABLE;

		obj->ob_state = OS_NORMAL;
		if ((i + args.scrollpos - 1) == popup_par->slct) /* ausgewaehlter Eintrag? */
			obj->ob_state |= OS_CHECKED;

		obj->ob_spec.free_string = tmp; /* Zeiger auf den String */

		obj->ob_x = 0;
		obj->ob_y = (i - 1) * phchar;
		obj->ob_width = args.tree->ob_width;
		obj->ob_height = phchar;

		str = popup_par->strs[args.scrollpos + i - 1];
		if (*str == '-')
		{
			for (j = 0; j < popup_par->max_width; j++)
				*tmp++ = '-';
			*tmp = '\0';
			obj->ob_state |= OS_DISABLED;
		} else
		{
			for (j = 0; j < popup_par->spaces; j++) /* Anfang mit Leerzeichen auffuellen */
				*tmp++ = ' ';

			strcpy(tmp, str); /* String kopieren */
		}
		obj++;
	}
	obj[-1].ob_flags |= OF_LASTOB;

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
/* Popup behandeln */
/* Funktionsresultat:	Index des angewaehlten Strings */
/*	button_rect:	Ausmasse des Buttons */
/*	strs:		Feld mit Zeigern auf die Strings */
/*	no_strs:	Anzahl der Strings */
/*	spaces:		Anzahl der vor den Strings einzufuegenden Leerzeichen */
/*	slct:		Index des selektierten Strings */
/*----------------------------------------------------------------------------------------*/ 
_WORD
do_popup(GRECT *button_rect, char **strs, _WORD no_strs, _WORD spaces, _WORD slct)
{
	PSTRS popup_par;
	OBJECT *tree;
	_WORD max_width;
	_WORD max_strs;
	_WORD i;
	long tree_size;

	if (no_strs > MAX_STRS)
		max_strs = MAX_STRS; /* Anzahl der sichtbaren Strings */
	else
		max_strs = no_strs;

	max_width = 0;

	for (i = 0; i < no_strs; i++) /* breitesten String bestimmen */
	{
		_WORD len;
		
		len = (_WORD)strlen(strs[i]);
		
		if (len > max_width)
			max_width = len;
	}

	max_width += spaces; /* maximale Stringbreite in Zeichen */
	popup_par.max_width = max_width;
	tree_size = sizeof(OBJECT) * (1 + max_strs); /* Laenge des Objektbaums */

	tree = (void *)Malloc(tree_size + (max_strs * (max_width + 1)));

	for (i = 0; i < max_strs; i++) /* Zeiger auf die temporaeren String initialisieren */
		popup_par.buf_str[i] = ((char *) tree) + tree_size + (i * (max_width + 1));

	if (tree) /* Speicher vorhanden? */
	{
		_WORD scroll_pos;

		max_width *= pwchar; /* Breite in Pixeln */

		if (max_width < button_rect->g_w) /* zu schmal? */
			max_width = button_rect->g_w;

		tree->ob_next = -1; /* Wurzelobjekt; weisse Hintergrundbox */
		tree->ob_flags = OF_NONE;

		if (max_strs > 0) /* sind Strings vorhanden? */
		{
			tree->ob_head = 1;
			tree->ob_tail = max_strs;
		}
		else /* keine Strings */
		{
			tree->ob_flags |= OF_LASTOB;
			tree->ob_head = -1;
			tree->ob_tail = -1;
		}

		tree->ob_type = G_BOX;
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
		{
			scroll_pos = 0;
		} else
		{
			scroll_pos = slct - (MAX_STRS / 2);
			if (scroll_pos < 0)
				scroll_pos = 0;
			if (scroll_pos > (no_strs - MAX_STRS))
				scroll_pos = no_strs - MAX_STRS;
		}

		if (slct < 0 || slct >= no_strs)
			slct = 0;

		tree->ob_y -= (slct - scroll_pos) * phchar; /* Popup nach oben verschieben */

		/*
		 * call the init function explicitly here, in case
		 * xfrm_popup does not support scrollable popups
		 * (like in XaAES)
		 */
		 {
			struct POPUP_INIT_args args;
			args.tree = tree;
			args.scrollpos = scroll_pos;
			args.nlines = no_strs;
			args.param = (void *) &popup_par;
#ifdef __PUREC__
#pragma warn -stv
#endif
			strs_init(args); /* String besetzen */
#ifdef __PUREC__
#pragma warn +stv
#endif
		}

		if (no_strs > MAX_STRS)
		{
			if (aes_flags & GAI_SCROLLPOPUP)
				slct = xfrm_popup(tree, 0, 0, 1, max_strs, no_strs, strs_init, (void *) &popup_par, &scroll_pos);
			else
				slct = do_form_popup(tree, 0, 0, 1, max_strs, no_strs, strs_init, (void *) &popup_par, &scroll_pos);
		} else if (aes_flags & GAI_POPUP)
		{
			slct = form_popup(tree, 0, 0);
		} else
		{
			slct = do_form_popup(tree, 0, 0, 0, 0, 0, 0, 0, &scroll_pos);
		}
		evnt_timer(0);

		if (slct > 0)
		{
			slct -= 1;
			slct += scroll_pos;
		} else
		{
			slct = -1;
		}

		Mfree(tree);
	}

	return slct;
}

static void pop_position(OBJECT *tree, GRECT *work)
{
	_WORD x, y;
	GRECT gr;
	_WORD xdiff, ydiff;
	
	x = tree[ROOT].ob_x;
	y = tree[ROOT].ob_y;

	form_center_grect(tree, &gr);
	xdiff = tree[ROOT].ob_x - gr.g_x + 1;
	ydiff = tree[ROOT].ob_y - gr.g_y + 1;
	x -= xdiff;
	y -= ydiff;
	work->g_w = gr.g_w + 3;
	work->g_h = gr.g_h + 3;
	
	if ((y + work->g_h) > (desk_grect.g_y + desk_grect.g_h))
		y = desk_grect.g_y + desk_grect.g_h - work->g_h;
	if (y < desk_grect.g_y)
		y = desk_grect.g_y;

	if ((x + work->g_w) > (desk_grect.g_x + desk_grect.g_w))
		x = desk_grect.g_x + desk_grect.g_w - work->g_w;
	if (x < desk_grect.g_x)
		x = desk_grect.g_x;

	work->g_x = x;
	work->g_y = y;
	tree[ROOT].ob_x = x + xdiff;
	tree[ROOT].ob_y = y + ydiff;
}


static void objc_draw_obj(OBJECT *tree, _WORD object)
{
	GRECT r;
	
	objc_offset(tree, object, &r.g_x, &r.g_y);
	r.g_w = tree[object].ob_width;
	r.g_h = tree[object].ob_height;
	objc_draw_grect(tree, ROOT, MAX_DEPTH, &r);
}


static BOOLEAN pop_arrow(OBJECT *tree, _WORD obj, _WORD *offset, _WORD no_strs, void *param)
{
	PSTRS *popup_par = (PSTRS *)param;
	BOOLEAN draw = FALSE;

	/* Up Arrow Selected AND not at the top */
	if (obj == tree[ROOT].ob_head && *offset > 0)
	{
		*offset -= 1;
		draw = TRUE;
	}

	/* DOWN Arrow and not at the bottom */
	if (obj == tree[ROOT].ob_tail && (*offset + popup_par->max_strs + 1) <= popup_par->no_strs)
	{
		*offset += 1;
		draw = TRUE;
	}

	if (draw)
	{
		struct POPUP_INIT_args args;
		args.tree = tree;
		args.scrollpos = *offset;
		args.nlines = no_strs;
		args.param = param;
#ifdef __PUREC__
#pragma warn -stv
#endif
		strs_init(args); /* String besetzen */
#ifdef __PUREC__
#pragma warn +stv
#endif
		tree[obj].ob_state |= OS_SELECTED;
		objc_draw_obj(tree, ROOT);
	}
	return draw;
}


_WORD do_form_popup(OBJECT *tree,
	_WORD x, _WORD y,
	_WORD firstscrlob, _WORD lastscrlob, _WORD nlines,
	void __CDECL (*init)(struct POPUP_INIT_args),
	void *param,
	_WORD *offset)
{
	EVNT events;
	MOBLK m1;
	_WORD obj;
	_WORD oldobj;						/* old object           */
	BOOLEAN done = FALSE;				/* Done flag to return..    */

	_WORD exit_state;					/* looking for up or down button */
	GRECT work;
	
	if (x != 0 || y != 0)
		form_center_grect(tree, &work);
	pop_position(tree, &work);
	objc_draw_grect(tree, ROOT, MAX_DEPTH, &work);

	evnt_timer(25);

	wind_update(BEG_UPDATE);
	wind_update(BEG_MCTRL);
	form_dial_grect(FMD_START, &work, &work);

	graf_mkstate(&events.mx, &events.my, &events.mbutton, &events.kstate);
	m1.m_x = events.mx;
	m1.m_y = events.my;
	m1.m_w = 1;
	m1.m_h = 1;
	m1.m_out = 1;
	exit_state = events.mbutton ^ 1;

	oldobj = -1;
	obj = objc_find(tree, ROOT, MAX_DEPTH, events.mx, events.my);
	if (obj <= 0 || (tree[obj].ob_state & OS_DISABLED))
		obj = -1;

	do
	{
		if (obj != oldobj)
		{
			if (oldobj > 0)
			{
				tree[oldobj].ob_state &= ~OS_SELECTED;
				objc_draw_obj(tree, oldobj);
			}
			if (obj > 0)
			{
				tree[obj].ob_state |= OS_SELECTED;
				objc_draw_obj(tree, obj);
			}
		}

		oldobj = obj;

		{
			EVNT_multi(MU_BUTTON | MU_M1,
				0x01, 0x01, exit_state,
				&m1, NULL,
				0,
				&events);
			events.mwhich &= MU_BUTTON;
			m1.m_x = events.mx;
			m1.m_y = events.my;
		}

		obj = objc_find(tree, ROOT, MAX_DEPTH, events.mx, events.my);
		if (obj <= 0 || (tree[obj].ob_state & OS_DISABLED))
			obj = -1;
		
		if (events.mwhich && init && (obj == firstscrlob || obj == lastscrlob))
		{
			if (pop_arrow(tree, obj, offset, nlines, param))
			{
				exit_state = 1;
				events.mwhich = 0;
				evnt_timer(125);
			}
		}

		if (events.mwhich)
			done = TRUE;

	} while (!done);

	/* Wait for up Button! */
	while (events.mbutton)
	{
		graf_mkstate(&events.mx, &events.my, &events.mbutton, &events.kstate);
	}

	if (obj <= 0)
		obj = -1;
	else
		tree[obj].ob_state &= ~OS_SELECTED;

	form_dial_grect(FMD_FINISH, &work, &work);

	wind_update(END_MCTRL);
	wind_update(END_UPDATE);

	return obj;
}
