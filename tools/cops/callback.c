/*
 * $Id$
 * 
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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <mint/cookie.h>
#include <mint/mintbind.h>

#include "callback.h"
#include "cops_rsc.h"
#include "cops.h"
#include "cpx_bind.h"
#include "fix_rsc.h"
#include "popup.h"
#include "objmacro.h"

/*----------------------------------------------------------------------------------------*/ 
/* Exportstruktur									*/
/*----------------------------------------------------------------------------------------*/ 
XCPB xctrl_pb =
{
	0,
	0,
	0,
	0,

	get_cpx_list,
	save_header,

	rsh_fix,
	rsh_obfix,

	Popup,

	Sl_size,
	Sl_x,
	Sl_y,
	Sl_arrow,
	Sl_dragx,
	Sl_dragy,

	Xform_do,
	GetFirstRect,
	GetNextRect,
	Set_Evnt_Mask,
			
	XGen_Alert,

	CPX_Save,
	Get_Buffer,
	getcookie,
	1,
	MFsave
};

#define	DEBUG	0

/*----------------------------------------------------------------------------------------*/ 
/* Zeiger auf die Liste der CPXe zurueckgeben						*/
/* Funktionsresultat:	CPX-Liste							*/
/*----------------------------------------------------------------------------------------*/ 
CPX_LIST * _cdecl
get_cpx_list(void)
{
#if DEBUG
	printf("get_cpx_list\n");
#endif

	return cpxlist;
}

/*----------------------------------------------------------------------------------------*/ 
/* CPX-Header speichern									*/
/* Funktionsresultat:	CPX-Liste							*/
/*	cpx_list:	alte CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
short _cdecl
save_header(CPX_LIST *cpx_list)
{
	CPX_DESC *cpx_desc;
	char name[256];
	long handle;
	short result;

#if DEBUG
	printf("save_header\n");
#endif

	cpx_desc = (CPX_DESC *) ((unsigned char *) cpx_list - offsetof(CPX_DESC, old));
	strcpy(name, settings.cpx_path);
	strcat(name, cpx_desc->file_name);
	result = 0;

	handle = Fopen(name, O_WRONLY);
	if (handle > 0)
	{
		if (Fwrite((short) handle, sizeof(CPXHEAD), &cpx_list->header) == sizeof(CPXHEAD))
			result = 1;

		Fclose((short) handle);
	}	
	return result;
}

/*----------------------------------------------------------------------------------------*/ 
/* Resource anpassen, Indizes in Zeiger wandeln, Pixelkoordinaten eintragen		*/
/* Funktionsresultat:	-								*/
/*	num_objs:	Anzahl aller Objekte in rs_object				*/
/*	num_frstr:	Anzahl der freien Strings in rs_frstr				*/
/*	num_frimg:	Anzahl der freien BITBLKs in rs_frimg				*/
/*	num_tree:	Anzahl der Baeume in rs_trindex					*/
/*	rs_object:	Feld mit saemtlichen Resource-OBJECTs				*/
/*	rs_tedinfo:	Feld mit den von OBJECTs angesprochenen TEDINFOs		*/
/*	rs_string:	Feld mit Strings						*/
/*	rs_iconblk:	Feld mit ICONBLKs fuer OBJECTs					*/
/*	rs_bitblk:	Feld mit BITBLKs fuer OBJECTs					*/
/*	rs_frstr:	Feld mit Indizes/Zeigern auf freie Strings			*/
/*	rs_frimg:	Feld mit Indizes/Zeigern auf freie Images			*/
/*	rs_trindex:	Feld mit Indizes/Zeigern auf Objektbaeume			*/
/*	rs_imdope:	Feld mit Zeigern auf Images fuer ICONBLKs			*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
cpx_fix_rsh(CPX_DESC *cpx_desc, short num_objs, short num_frstr, short num_frimg, short num_tree,
	    OBJECT *rs_object, TEDINFO *rs_tedinfo, char *rs_string[], ICONBLK *rs_iconblk, BITBLK *rs_bitblk,
	    long *rs_frstr, long *rs_frimg, long *rs_trindex, struct foobar *rs_imdope)
{
	OBJECT	*obj;

	if (cpx_desc->xctrl_pb.SkipRshFix == 0)
	{
#if DEBUG
		printf("rsh_fix num_objs %d, num_frstr %d, num_frimg %d, num_tree %d\n",
		       num_objs, num_frstr, num_frimg, num_tree);
#endif

		fix_rsc(num_objs, num_frstr, num_frimg, num_tree,
			rs_object, rs_tedinfo, rs_string, rs_iconblk, rs_bitblk,
			rs_frstr, rs_frimg, rs_trindex, rs_imdope);
		
		obj = rs_object;	
	
		while (num_objs > 0)
		{
			cpx_rsh_obfix(cpx_desc, obj, 0);	/* in Pixelkoordinaten wandeln */
			obj++;
			num_objs--;
		}
	
		if (rs_object->ob_width > 512)			/* Grundobjekt zu breit? */
			rs_object->ob_width = 512;
		
		if (rs_object->ob_height > 384)			/* Grundobjekt zu hoch? */
			rs_object->ob_height = 384;
	
		cpx_desc->xctrl_pb.SkipRshFix = 1;		/* RSC wurde schon einmal initialisiert */
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* Zeichenkoordinaten eines Objekts in Pixel umrechnen					*/
/* Funktionsresultat:	-								*/
/*	tree:		Adresse des Objektbaums						*/
/*	ob:		Objektnummer							*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
cpx_rsh_obfix(CPX_DESC *cpx_desc, OBJECT *tree, short ob)
{
	short hpix, ypix;

#if DEBUG_OBFIX
	printf("rsh_obfix ob %d\n", ob);
#endif

	ypix = (tree[ob].ob_y & 0xff00) >> 8;
	tree[ob].ob_y &= 0xff;
	
	hpix = (tree[ob].ob_height & 0xff00) >> 8;
	tree[ob].ob_height &= 0xff;

	rsrc_obfix(tree, ob);					/* von Zeichen- in Pixelkoordinaten umrechnen */
	
	if ((pwchar == 8) && (phchar == 8))			/* unselige Aufl”sung? */
	{
		tree[ob].ob_y += tree[ob].ob_y;			/* H”he und Position korrigieren */
		tree[ob].ob_height += tree[ob].ob_height;
	}

	tree[ob].ob_y += ypix;
	tree[ob].ob_height += hpix;

	if ((ob == 0) && (cpx_desc->obfix_cnt == 0))
	{

		cpx_desc->box_width = tree->ob_width;
		cpx_desc->box_height = tree->ob_height;

		if (cpx_desc->box_width < 32)			/* Grundobjekt zu schmal? */
			cpx_desc->box_width = 32;
		
		if (cpx_desc->box_height < 32)			/* Grundobjekt zu niedrig? */
			cpx_desc->box_height = 32;

		if (cpx_desc->box_width > 512)			/* Grundobjekt zu breit? */
			cpx_desc->box_width = 512;
		
		if (cpx_desc->box_height > 384)			/* Grundobjekt zu hoch? */
			cpx_desc->box_height = 384;
	}
	cpx_desc->obfix_cnt++;

	/* Funktioniert in ram-residenten CPXen aufgrund der
	 * <SkipRshFix>-Abfrage vor der <for>-Schleife...
	 */
	cpx_desc->xctrl_pb.SkipRshFix = 1;			/* RSC wurde schon einmal initialisiert */
}

/*----------------------------------------------------------------------------------------*/ 
/* Popup zeichnen									*/
/* Funktionsresultat:	Index des angewaehlten Eintrags oder -1				*/
/*	strs:		Strings								*/
/*	no_strs:	Anzahl der Strings						*/
/*	slct:		Index des selektierten Eintrags					*/
/*	font:		wird ignoriert							*/
/*	button_rect:	Ausmaže des Grundbuttons					*/
/*	tree_rect:	Ausmaže des CPX						*/
/*----------------------------------------------------------------------------------------*/ 
short _cdecl
Popup(char **strs, short no_strs, short slct, short font, GRECT *button_rect, GRECT *tree_rect)
{
#if DEBUG
	printf("Popup\n");
#endif

	return do_popup(button_rect, strs, no_strs, 0, slct);
}

/*----------------------------------------------------------------------------------------*/ 
/* Objekt zeichnen									*/
/* Funktionsresultat:	-								*/
/*	fnt_dialog:	Zeiger auf die Fontdialog-Struktur				*/
/*	obj:		Nummer des Objekts						*/
/*----------------------------------------------------------------------------------------*/ 

#define my_objc_draw(a,b,c,g) objc_draw(a,b,c,(g)->g_x,(g)->g_y,(g)->g_w,(g)->g_h)

static void
redraw_obj(OBJECT *tree, short obj)
{
	GRECT rect;

	rect = *(GRECT *) &tree->ob_x;				/* Dialog-Rechteck */
	
	wind_update(BEG_UPDATE);				/* Bildschirm sperren */
	my_objc_draw(tree, obj, MAX_DEPTH, &rect);
	wind_update(END_UPDATE);				/* Bildschirm freigeben */
}

/*----------------------------------------------------------------------------------------*/ 
/* Slidergr”že einstellen								*/
/* Funktionsresultat:	-								*/
/* tree:		Zeiger auf den Objektbaum					*/
/*	base:		Hintergrundobjekt fuer den Slider				*/
/*	slider:		Objektnummer des Sliders					*/
/*	num_items:	Anzahl der vorhandenen Elemente					*/
/*	visible:	Anzahl der sichtbaren Elemente					*/
/*	direction:	Richtung (0: vertikal, 1: horizontal)				*/
/*	min_size:	minimale Slidergr”že						*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
Sl_size(OBJECT *tree, short base, short slider, short num_items, short visible, short direction, short min_size)
{
#if DEBUG
	printf("Sl_size base %d, slider %d, num_items %d, visible %d, direction %d, min_size %d\n",
	       base, slider, num_items, visible, direction, min_size);
#endif

	if (direction)						/* horizontaler Slider? */
	{
		tree[slider].ob_x = 0;

		if (visible >= num_items)			/* weniger Elemente als darstellbar? */
			tree[slider].ob_width = tree[base].ob_width;
		else
			tree[slider].ob_width = (short) (((long) tree[base].ob_width * visible) / num_items);
	
		if (tree[slider].ob_width < min_size)
			tree[slider].ob_width = min_size;
	}
	else							/* vertikaler Slider */
	{
		tree[slider].ob_y = 0;

		if (visible >= num_items)			/* weniger Elemente als darstellbar? */
			tree[slider].ob_height = tree[base].ob_height;
		else
			tree[slider].ob_height = (short) (((long) tree[base].ob_height * visible) / num_items);

		if (tree[slider].ob_height < min_size)
			tree[slider].ob_height = min_size;
	}
}

/*----------------------------------------------------------------------------------------*/ 
/*	Slider in horizontaler Richtung positionieren					*/
/* Funktionsresultat:	-								*/
/* tree:		Zeiger auf den Objektbaum					*/
/*	base:		Hintergrundobjekt fuer den Slider				*/
/*	slider:		Objektnummer des Sliders					*/
/*	value:		neuer Wert, den der Slider repraesentieren soll			*/
/*	min:		Minimalwert, den value annehmen darf				*/
/*	max:		Maximalwert, den value annehmen darf				*/
/*	userdef:	benutzerdefinierte Funktion					*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
Sl_x(OBJECT *tree, short base, short slider, short value, short min, short max, void (*userdef)(void))
{
	short w;

#if DEBUG
	printf("Sl_x base %d, slider %d, value %d, min %d, max %d\n",
	       base, slider, value, min, max);
#endif

	w = tree[base].ob_width - tree[slider].ob_width;

	if (min <= max)						/* Slider hat links Minimalwert und rechts Maximalwert? */
	{
		value -= min;
		max -= min;
		
		if (value < 0)
			value = 0;
		if (value > max)
			value = max;

		tree[slider].ob_x = (short)(((long) w * value) / max);
	}
	else							/* Slider hat links Maximalwert und rechts Minimalwert */
	{
		value -= max;
		min -= max;

		if (value < 0)
			value = 0;
		if (value > min)
			value = min;

		tree[slider].ob_x = w - (short)(((long) w * value) / min);
	}
	
	cpx_userdef(userdef);					/* benutzerdefinierte Funktion aufrufen */
}

/*----------------------------------------------------------------------------------------*/ 
/*	Slider in vertikaler Richtung positionieren					*/
/* Funktionsresultat:	-								*/
/* tree:		Zeiger auf den Objektbaum					*/
/*	base:		Hintergrundobjekt fuer den Slider				*/
/*	slider:		Objektnummer des Sliders					*/
/*	value:		neuer Wert, den der Slider repraesentieren soll			*/
/*	min:		Minimalwert, den value annehmen darf				*/
/*	max:		Maximalwert, den value annehmen darf				*/
/*	userdef:	benutzerdefinierte Funktion					*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
Sl_y(OBJECT *tree, short base, short slider, short value, short min, short max, void (*userdef)(void))
{
	short h;

#if DEBUG
	printf("Sl_y base %d, slider %d, value %d, min %d, max %d\n",
	       base, slider, value, min, max);
#endif

	h = tree[base].ob_height - tree[slider].ob_height;

	if (min <= max)						/* Slider hat oben Maximalwert und unten Minimalwert? */
	{
		value -= min;
		max -= min;
		
		if (value < 0)
			value = 0;
		if (value > max)
			value = max;

		tree[slider].ob_y = h - (short)(((long) h * value) / max);
	}
	else							/* Slider hat oben Minimalwert und unten Maximalwert */
	{
		value -= max;
		min -= max;

		if (value < 0)
			value = 0;
		if (value > min)
			value = min;

		tree[slider].ob_y = (short)(((long) h * value) / min);
	}
	
	cpx_userdef(userdef);					/* benutzerdefinierte Funktion aufrufen */
}

/*----------------------------------------------------------------------------------------*/ 
/* Klick auf Scrollpfeil oder Sliderhintergrund bearbeiten				*/
/* Funktionsresultat:	-								*/
/* tree:		Zeiger auf den Objektbaum					*/
/*	base:		Hintergrundobjekt fuer den Slider				*/
/*	slider:		Objektnummer des Sliders					*/
/*	inc:		Anzahl der Einheiten, die addiert oder subtrahiert werden sollen*/
/*	min:		Minimalwert, den value annehmen darf				*/
/*	max:		Maximalwert, den value annehmen darf				*/
/*	value:		neuer Wert, den der Slider repraesentieren soll			*/
/*	direction:	Richtung (0: vertikal, 1: horizontal)				*/
/*	userdef:	benutzerdefinierte Funktion					*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
Sl_arrow(OBJECT *tree, short base, short slider, short obj, short inc,
         short min, short max, short *value, short direction, void (*userdef)(void))
{
	short val;
	
#if DEBUG
	printf("Sl_arrow base %d, slider %d, obj %d, inc %d, min %d, max %, value %d, direction %d\n",
	       base, slider, obj, inc, min, max, *value, direction);
#endif

	val = *value + inc;					/* neue Position */

	if (min <= max)
	{
		if (val < min)
			val = min;
		if (val > max)
			val = max;
	}
	else
	{
		if (val < max)
			val = max;
		if (val > min)
			val = min;
	}

	*value = val;						/* Wert zurueckschreiben */
	
	if (obj >= 0)
	{
		obj_SELECTED(tree, obj);
		redraw_obj(tree, obj);				/* Button selektieren */
	}
	
	if (direction)						/* horizontaler Slider? */
		Sl_x(tree, base, slider, val, min, max, userdef);
	else
		Sl_y(tree, base, slider, val, min, max, userdef);

	redraw_obj(tree, base);					/* Slider neuzeichnen */

	evnt_timer(40);						/* warten, damit das Scrolling nicht zu schnell wird */
	if (obj >= 0)
	{
		obj_DESELECTED(tree, obj);
		redraw_obj(tree, obj);				/* Button wieder normal */
	}
}

/*----------------------------------------------------------------------------------------*/ 
/*	Slider in horizontaler Richtung bewegen						*/
/* Funktionsresultat:	-								*/
/* tree:		Zeiger auf den Objektbaum					*/
/*	base:		Hintergrundobjekt fuer den Slider				*/
/*	slider:		Objektnummer des Sliders					*/
/*	inc:		Anzahl der Einheiten, die addiert oder subtrahiert werden sollen*/
/*	min:		Minimalwert, den value annehmen darf				*/
/*	max:		Maximalwert, den value annehmen darf				*/
/*	value:		neuer Wert, den der Slider repraesentieren soll			*/
/*	userdef:	benutzerdefinierte Funktion					*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
Sl_dragx(OBJECT *tree, short base, short slider, short min, short max,
         short *value, void (*userdef)(void))
{
	short hpos;
	
#if DEBUG
	printf("Sl_dragx base %d, slider %d, min %d, max %d, value %d\n",
	       base, slider, min, max, *value);
#endif

	hpos = graf_slidebox(tree, base, slider,0);
	*value = (short) (((long) (max - min) * hpos) / 1000);
	*value += min;
	Sl_x(tree, base, slider, *value, min, max, userdef);
	redraw_obj(tree, base);					/* Slider neuzeichnen */
}

/*----------------------------------------------------------------------------------------*/ 
/*	Slider in vertikaler Richtung bewegen						*/
/* Funktionsresultat:	-								*/
/* tree:		Zeiger auf den Objektbaum					*/
/*	base:		Hintergrundobjekt fuer den Slider				*/
/*	slider:		Objektnummer des Sliders					*/
/*	inc:		Anzahl der Einheiten, die addiert oder subtrahiert werden sollen*/
/*	min:		Minimalwert, den value annehmen darf				*/
/*	max:		Maximalwert, den value annehmen darf				*/
/*	value:		neuer Wert, den der Slider repraesentieren soll			*/
/*	userdef:	benutzerdefinierte Funktion					*/
/*----------------------------------------------------------------------------------------*/ 
void _cdecl
Sl_dragy(OBJECT *tree, short base, short slider, short min, short max,
         short *value, void (*userdef)(void))
{
	short vpos;
	
#if DEBUG
	printf("Sl_dragy base %d, slider %d, min %d, max %d, value %d\n",
	       base, slider, min, max, *value);
#endif

	vpos = graf_slidebox(tree, base, slider, 1);
	*value = (short)(((long) (max - min) * (1000 - vpos)) / 1000);
	*value += min;
	Sl_y(tree, base, slider, *value, min, max, userdef);
	redraw_obj(tree, base);					/* Slider neuzeichnen */
}

static short
is_edit_obj_hidden(OBJECT *tree, short obj)
{	
	if (is_obj_HIDDEN(tree, obj))
		return 1;
	
	while (tree[obj].ob_next != -1)				/* Wurzel gefunden? */
	{
		short last_obj;
		
		last_obj = obj;
		obj = tree[obj].ob_next;			/* naechstes Objekt oder Vater */
		
		if (last_obj == tree[obj].ob_tail)		/* war das letzte Objekt ein Kind, ist obj der Vater? */
		{
			if (is_obj_HIDDEN(tree, obj))
				return 1;
		}
	}
	return 0;
}

/*----------------------------------------------------------------------------------------*/ 
/* (Xform_do) form_do() fuer CPX-Fenster simulieren					  */
/* Funktionsergebnis:	CPX_DESC des neuen Kontexts					  */
/*	cpx_desc:	CPX-Beschreibung						  */
/*	tree:		Objektbaum des CPX						  */
/*	edit_obj:	Nummer des aktiven Editobjekts oder 0				  */
/*	msg:		Zeiger auf Message-Buffer fuer ausgewaehlte Ereignisse		  */
/*----------------------------------------------------------------------------------------*/ 
CPX_DESC *
cpx_form_do(CPX_DESC *cpx_desc, OBJECT *tree, short edit_obj, short *msg)
{
	DIALOG *dialog;
	short last_edit_obj;
	short cursor;

#if DEBUG
	printf("cpx_form_do\n");
#endif

	dialog = cpx_desc->dialog;

	if (tree != cpx_desc->tree)				/* Objektbaum geaendert? */
	{
		if (cpx_desc->tree == 0L)
			cpx_desc->size.g_y -= desk_grect.g_h;	/* CPX-Rechteck wieder innerhalb des Schirms */

		cpx_desc->tree = tree;
		obj_HIDDEN(tree, ROOT);				/* Baum nicht zeichnen! */
		wdlg_set_tree(dialog, tree);			/* neuen Objektbaum setzen */
		obj_VISIBLE(tree, ROOT);
	}

	last_edit_obj = wdlg_get_edit(dialog, &cursor);		/* Editobjekt erfragen */

	if (last_edit_obj > 0)					/* Editobjekt aktiv? */
	{
		if (is_edit_obj_hidden(tree, last_edit_obj))	/* Editobjekt unsichtbar? */
		{
			obj_HIDDEN(tree, ROOT);			/* Baum nicht zeichnen! */
			wdlg_set_tree(dialog, tree);		/* Objektbaum nochmals setzen, Cursorzustand zuruecksetzen */
			obj_VISIBLE(tree, ROOT);
		}
	} 

	if (edit_obj)
		wdlg_set_edit(dialog, edit_obj);		/* Editobjekt setzen */

	cpx_desc->button = -1;					/* noch kein Button gedrueckt */
	cpx_desc->msg = msg;

	return cpx_main_loop();					/* CPX_DESC des neuen Kontexts zurueckliefern */
}

/*----------------------------------------------------------------------------------------*/ 
/* (GetFirstRect) erstes Redraw-Rechteck fuer CPX-Fenster zurueckliefern		*/
/* Funktionsergebnis:	Redraw-Rechteck oder NULL					*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*	redraw_area:	Maximale Gr”že des insgesamt zu zeichnenden Bereichs		*/
/*----------------------------------------------------------------------------------------*/ 
GRECT *
cpx_get_first_rect(CPX_DESC *cpx_desc, GRECT *redraw_area)
{
	GRECT *w;

	cpx_desc->redraw_area = *redraw_area;			/* fuer Aufruf von cpx_get_next_rect() merken */
	w = &cpx_desc->dirty_area;

#if DEBUG
	printf("cpx_get_first_rect: redraw_area ist x %d y %d w %d h %d\n",
	       redraw_area->g_x, redraw_area->g_y, redraw_area->g_w, redraw_area->g_h);
#endif

	/* erstes Redraw-Rechteck */
	wind_get(cpx_desc->whdl, WF_FIRSTXYWH, &w->g_x, &w->g_y, &w->g_w, &w->g_h);

	while (w->g_w)
	{
		if (rc_intersect(&cpx_desc->redraw_area, w))
		{
#if DEBUG
			printf("cpx_get_first_rect: x %d y %d w %d h %d\n",
			       w->g_x, w->g_y, w->g_w, w->g_h);
#endif
			/* zu zeichnender Bereich */
			return w;
		}

		/* naechstes Redraw-Rechteck */
		wind_get(cpx_desc->whdl, WF_NEXTXYWH, &w->g_x, &w->g_y, &w->g_w, &w->g_h);
	}
	
#if DEBUG
	printf("cpx_get_first_rect: kein weiteres Rechteck\n");
#endif	

	return NULL;

}

/*----------------------------------------------------------------------------------------*/ 
/* (GetNextRect) naechstes Redraw-Rechteck fuer CPX-Fenster zurueckliefern		*/
/* Funktionsergebnis:	Redraw-Rechteck oder NULL						*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
GRECT *
cpx_get_next_rect(CPX_DESC *cpx_desc)
{
	GRECT *w = &cpx_desc->dirty_area;

	/* naechstes Redraw-Rechteck */
	wind_get(cpx_desc->whdl, WF_NEXTXYWH, &w->g_x, &w->g_y, &w->g_w, &w->g_h);

	while (w->g_w)
	{
		if (rc_intersect(&cpx_desc->redraw_area, w))
		{
#if DEBUG
			printf("cpx_get_next_rect: x %d y %d w %d h %d\n",
			       w->g_x, w->g_y, w->g_w, w->g_h);
#endif
			/* zu zeichnender Bereich */
			return w;
		}

		/* naechstes Redraw-Rechteck */
		wind_get(cpx_desc->whdl, WF_NEXTXYWH, &w->g_x, &w->g_y, &w->g_w, &w->g_h);
	}

#if DEBUG
	printf("cpx_get_next_rect: kein weiteres Rechteck\n");
#endif	

	return NULL;
}

/*----------------------------------------------------------------------------------------*/ 
/* (Set_Evnt_Mask) Eventmaske fuer evnt_multi veraendern; ggf. Mausrechtecke, Timer setzen	*/
/* Funktionsergebnis:	0: Fehler 1: alles in Ordnung					*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*	mask:		zusaetzliche Ereignismaske					*/
/*	m1:		erstes Mausrechteck						*/
/*	m2:		zweites Mausrechteck						*/
/*	time:		Timerintervall							*/
/*----------------------------------------------------------------------------------------*/ 
void
cpx_set_evnt_mask(CPX_DESC *cpx_desc, short mask, MOBLK *m1, MOBLK *m2, long time)
{
#if DEBUG
	printf("cpx_set_evnt_mask\n");
#endif

	if (m1)							/* erstes Mausrechteck? */
		cpx_desc->m1 = *m1;
	else
		mask &= ~MU_M1;
			
	if (m2)							/* zweites Mausrechteck? */
		cpx_desc->m2 = *m2;
	else
		mask &= ~MU_M2;

	if (mask & MU_TIMER)					/* Timer? */
		cpx_desc->time = time;												
	else
		cpx_desc->time = 0L;

	cpx_desc->mask = mask;					/* zusaetzliche Ereignismaske */
}

/*----------------------------------------------------------------------------------------*/ 
/* Alertbox fuer CPX aufrufen								*/
/* Funktionsergebnis:	Buttonnummer							*/
/*	alert_id:	Art der Alertbox						*/
/*----------------------------------------------------------------------------------------*/ 
short _cdecl
XGen_Alert(short alert_id)
{
#if DEBUG
	printf("XGen_Alert\n");
#endif

	switch (alert_id)
	{
		/* Voreinstellungen sichern? */
		case 0:
		{
			if (form_alert(1, fstring_addr[SAVE_DFLT_ALERT]) == 1)
				return 1;
		
			break;
		}
		/* nicht genuegend Speicher */
		case 1:
		{
			form_alert(1, fstring_addr[MEM_ERR_ALERT]);
			return 0;
		}
		/* Schreib- oder Lesefehler */
		case 2:
		{
			form_alert(1, fstring_addr[FILE_ERR_ALERT]);
			return 0;
		}
		/* Datei nicht gefunden */
		case 3:
		{
			form_alert(1, fstring_addr[FNF_ERR_ALERT]);
			return 0;
		}
	}
	return 0;
}

/*----------------------------------------------------------------------------------------*/ 
/* (CPX_Save) Daten ab Beginn des Datasegments eines CPX speichern			*/
/* Funktionsresultat:	0: Fehler 1: alles in Ordnung					*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
short
cpx_save_data(CPX_DESC *cpx_desc, void *buf, long no_bytes)
{
	char name[256];
	long handle;
	long offset;
	short result;

#if DEBUG
	printf("CPX_Save\n");
#endif

	strcpy(name, settings.cpx_path);
	strcat(name, cpx_desc->file_name);
	result = 0;

	offset = sizeof(CPXHEAD) + sizeof(PH) + cpx_desc->segm.len_text;
	if (no_bytes > cpx_desc->segm.len_data)			/* laenger als das Data-Segment? */
		no_bytes = cpx_desc->segm.len_data;

	handle = Fopen(name, O_WRONLY);

	if (handle > 0)
	{
		if (Fseek(offset, (short) handle, 0) == offset)
		{
			if (Fwrite((short) handle, no_bytes, buf) == no_bytes)
				result = 1;
		}
		Fclose((short) handle);
	}	
	return result;
}

/*----------------------------------------------------------------------------------------*/ 
/* (Get_Buffer) Zeiger auf temporaeren CPX-Speicher zurueckliefern 			*/
/* Funktionsresultat:	Zeiger auf 64-Bytes						*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
void *
cpx_get_tmp_buffer(CPX_DESC *cpx_desc)
{
#if DEBUG
	printf("cpx_get_tmp_buffert\n");
#endif

	return cpx_desc->old.header.buffer;
}

/*----------------------------------------------------------------------------------------*/ 
/* Cookie suchen									*/
/* Funktionsresultat: 0: nicht vorhanden 1: Cookie gefunden				*/
/*	cookie: Cookietyp								*/
/*	p_value: hier wird der Cookiewert zurueckgeliefert				*/
/*----------------------------------------------------------------------------------------*/ 
short _cdecl
getcookie(long cookie, long *p_value)
{
#if DEBUG
	printf("getcookie\n");
#endif

	return (Getcookie(cookie, p_value) == 0);
}

/*----------------------------------------------------------------------------------------*/ 
/* Mausform setzen oder sichern								*/
/* Funktionsresultat:	-								*/
/*	flag: 0: Mausform setzen 1: Mausform sichern					*/
/*	mf: Zeiger auf Mausform								*/
/*----------------------------------------------------------------------------------------*/ 
#if 0
void _cdecl
MFsave(short flag, MFORM *mf)
{
#if DEBUG
	printf("MFsave\n");
#endif

	if (flag)					/* Mausform sichern? */
	{
		if (_AESversion >= 0x0399)		/* MagiC oder MultiTOS? */
			graf_mouse(258, mf);
		else					/* Mausform ueber LineA auslesen */
			get_mouse_form(mf);
	}
	else
	{
		if (_AESversion >= 0x0399)		/* MagiC oder MultiTOS? */
			graf_mouse(259, mf);
		else
			graf_mouse(USER_DEF, mf);

	}
}
#else
/* contributed by Arnaud */
void _cdecl
MFsave(short flag, MFORM *mf)
{
	static short has_mouse = 0;
	
	if (has_mouse == 0)
	{
		short parm1, dum;
		
		if (appl_getinfo(AES_MOUSE, &parm1, &dum, &dum, &dum) == 1
		    && parm1 == 1)
			has_mouse = 1;
		else
			has_mouse = -1;
	}
	
	if (flag)
	{
		if (has_mouse > 0)
			graf_mouse(M_SAVE, mf);
		else
			/* nothing */ ;
	}
	else
	{
		if (has_mouse > 0)
			graf_mouse(M_RESTORE, mf);
		else
			graf_mouse(ARROW, NULL);
	}
}
#endif

