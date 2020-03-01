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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gem.h>

#include <fcntl.h>
#include <mint/cookie.h>
#include <mint/mintbind.h>

#include "cops_rsc.h"
#include "adaptrsc.h"
#include "callback.h"
#include "cops.h"
#include "cpx_bind.h"
#include "fix_rsc.h"
#include "popup.h"
#include "objmacro.h"

#undef min
#undef max

#define DEBUG_CALLBACK(cpx) DEBUG((DEBUG_FMT "(%s)\n", DEBUG_ARGS, cpx ? cpx->file_name : "not found"))


static struct cpxlist *_cdecl get_cpx_list(void) { return cpxlist; }

static void    _cdecl rsh_fix(struct rsh_fix_args);
static void    _cdecl rsh_obfix(struct rsh_obfix_args);
static short   _cdecl Popup(struct Popup_args);
static void    _cdecl Sl_size(struct Sl_size_args);
static void    _cdecl Sl_x(struct Sl_xy_args);
static void    _cdecl Sl_y(struct Sl_xy_args);
static void    _cdecl Sl_arrow(struct Sl_arrow_args);
static void    _cdecl Sl_dragx(struct Sl_dragxy_args);
static void    _cdecl Sl_dragy(struct Sl_dragxy_args);
static GRECT * _cdecl GetFirstRect(GRECT *prect);
static GRECT * _cdecl GetNextRect(void *args);
static void    _cdecl Set_Evnt_Mask(struct Set_Evnt_Mask_args args);
static short   _cdecl XGen_Alert(struct XGen_Alert_args);
static short   _cdecl CPX_Save(void *ptr, long bytes);
static void *  _cdecl Get_Buffer(void *args);
static void    _cdecl MFsave(struct MFsave_args);


/*----------------------------------------------------------------------------------------*/ 
/* Cookie suchen									*/
/* Funktionsresultat: 0: nicht vorhanden 1: Cookie gefunden				*/
/*	cookie: Cookietyp								*/
/*	p_value: hier wird der Cookiewert zurueckgeliefert				*/
/*----------------------------------------------------------------------------------------*/ 
#if defined(__PUREC__) || defined(__AHCC__) || defined(__FASTCALL__)
static short _cdecl getcookie(long cookie, long *p_value)
{
	DEBUG(("getcookie(0x%lx, %p)\n", cookie, p_value));

	return get_cookie(cookie, p_value);
}
#endif

/* 
 * exported cpx support functions
 */
struct xcpb xctrl_pb =
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
	(GRECT * _cdecl (*)(void))GetNextRect,
	Set_Evnt_Mask,
			
	XGen_Alert,

	CPX_Save,
	(void *  _cdecl (*)(void))Get_Buffer,
#if defined(__PUREC__) || defined(__AHCC__) || defined(__FASTCALL__)
	getcookie,
#else
	get_cookie,
#endif

#if defined(GERMAN)
	1,
#elif defined(FRENCH)
	2,
#else
	0,
#endif

	MFsave
};


/*
 * search cpx
 */
CPX_DESC *
find_cpx_by_addr(void *pc)
{
	const void *addr = pc;
	CPX_DESC *cpx;

	DEBUG(("find_cpx_by_addr(%p)\n", addr));

	cpx = cpx_desc_list;
	while (cpx)
	{
		if (cpx->start_of_cpx && cpx->end_of_cpx)
		{
			DEBUG(("find_cpx_by_addr: [%s] start %p end %p\n",
				cpx->file_name, cpx->start_of_cpx, cpx->end_of_cpx));

			if (addr >= cpx->start_of_cpx && addr < cpx->end_of_cpx)
				break;
		}

		cpx = cpx->next;
	}

	return cpx;
}

/*----------------------------------------------------------------------------------------*/ 
/* CPX-Header speichern									*/
/* Funktionsresultat:	CPX-Liste							*/
/*	cpx_list:	alte CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
short _cdecl
save_header(struct cpxlist *cpx_list)
{
	char name[256];
	CPX_DESC *cpx_desc;
	short handle;
	short result = 0;

	DEBUG(("save_header\n"));

	cpx_desc = (CPX_DESC *)((unsigned char *)cpx_list - offsetof(CPX_DESC, old));
	strcpy(name, settings.cpx_path);
	strcat(name, cpx_desc->file_name);

	handle = (short)Fopen(name, O_WRONLY);
	if (handle > 0)
	{
		long ret;

		ret = Fwrite(handle, sizeof(cpx_list->header), &(cpx_list->header));
		if (ret == sizeof(cpx_list->header))
			result = 1;

		Fclose(handle);
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

static void cpx_rsh_obfix(CPX_DESC *cpx_desc, OBJECT *tree, short ob);

static void cpx_rsh_fix(CPX_DESC *cpx_desc, const struct rsh_fix_args *args)
{
	OBJECT *obj;

	DEBUG_CALLBACK(cpx_desc);

	if (cpx_desc->xctrl_pb.SkipRshFix == 0)
	{
		short num_objs = args->num_objs;

		DEBUG(("rsh_fix num_objs %d, num_frstr %d, num_frimg %d, num_tree %d\n",
		       args->num_objs, args->num_frstr, args->num_frimg, args->num_tree));

		fix_rsc(args->num_objs, args->num_frstr, args->num_frimg, args->num_tree,
			args->rs_object, args->rs_tedinfo, args->rs_strings, args->rs_iconblk, args->rs_bitblk,
			args->rs_frstr, args->rs_frimg, args->rs_trindex, args->rs_imdope);

		obj = args->rs_object;	

		while (num_objs > 0)
		{
			cpx_rsh_obfix(cpx_desc, obj, 0);	/* in Pixelkoordinaten wandeln */
			obj++;
			num_objs--;
		}

		if (args->rs_object->ob_width > 512)		/* Grundobjekt zu breit? */
			args->rs_object->ob_width = 512;

		if (args->rs_object->ob_height > 384)		/* Grundobjekt zu hoch? */
			args->rs_object->ob_height = 384;

		cpx_desc->xctrl_pb.SkipRshFix = 1;		/* RSC wurde schon einmal initialisiert */
	}
}

static void _cdecl
rsh_fix(struct rsh_fix_args args)
{
	CPX_DESC *cpx;
	void *pc = ((void **)&args)[-1];

	cpx = find_cpx_by_addr(pc);
	DEBUG_CALLBACK(cpx);

	if (cpx)
	{
		cpx_rsh_fix(cpx, &args);
	}
}


/*----------------------------------------------------------------------------------------*/ 
/* Zeichenkoordinaten eines Objekts in Pixel umrechnen					*/
/* Funktionsresultat:	-								*/
/*	tree:		Adresse des Objektbaums						*/
/*	ob:		Objektnummer							*/
/*----------------------------------------------------------------------------------------*/ 
static void
cpx_rsh_obfix(CPX_DESC *cpx_desc, OBJECT *tree, short ob)
{
	short hpix, ypix;

	DEBUG_CALLBACK(cpx_desc);
	DEBUG(("rsh_obfix ob %d\n", (int)ob));

	ypix = (tree[ob].ob_y & 0xff00) >> 8;
	tree[ob].ob_y &= 0xff;

	hpix = (tree[ob].ob_height & 0xff00) >> 8;
	tree[ob].ob_height &= 0xff;

	rsrc_obfix(tree, ob);					/* von Zeichen- in Pixelkoordinaten umrechnen */

	if ((pwchar == 8) && (phchar == 8))			/* unselige Aufloesung? */
	{
		tree[ob].ob_y += tree[ob].ob_y;			/* Hoehe und Position korrigieren */
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

static void _cdecl
rsh_obfix(struct rsh_obfix_args args)
{
	CPX_DESC *cpx;
	void *pc = ((void **)&args)[-1];
	
	cpx = find_cpx_by_addr(pc);
	DEBUG_CALLBACK(cpx);

	if (cpx)
	{
		cpx_rsh_obfix(cpx, args.tree, args.ob);
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* Popup zeichnen									*/
/* Funktionsresultat:	Index des angewaehlten Eintrags oder -1				*/
/*	strs:		Strings								*/
/*	no_strs:	Anzahl der Strings						*/
/*	slct:		Index des selektierten Eintrags					*/
/*	font:		wird ignoriert							*/
/*	button_rect:	Ausmasse des Grundbuttons					*/
/*	tree_rect:	Ausmasse des CPX						*/
/*----------------------------------------------------------------------------------------*/ 
static short _cdecl
Popup(struct Popup_args args)
{
	DEBUG(("Popup\n"));

	return do_popup(args.up, args.items, args.no_items, 0, args.slct);
}

/*----------------------------------------------------------------------------------------*/ 
/* Objekt zeichnen									*/
/* Funktionsresultat:	-								*/
/*	fnt_dialog:	Zeiger auf die Fontdialog-Struktur				*/
/*	obj:		Nummer des Objekts						*/
/*----------------------------------------------------------------------------------------*/ 
static void
redraw_obj(OBJECT *tree, short obj)
{
	GRECT *r = (GRECT *) &tree->ob_x; /* Dialog-Rechteck */

	wind_update(BEG_UPDATE);
	objc_draw_grect(tree, obj, MAX_DEPTH, r);
	wind_update(END_UPDATE);
}

/*----------------------------------------------------------------------------------------*/ 
/* Slidergroesse einstellen								*/
/* Funktionsresultat:	-								*/
/* tree:		Zeiger auf den Objektbaum					*/
/*	base:		Hintergrundobjekt fuer den Slider				*/
/*	slider:		Objektnummer des Sliders					*/
/*	num_items:	Anzahl der vorhandenen Elemente					*/
/*	visible:	Anzahl der sichtbaren Elemente					*/
/*	direction:	Richtung (0: vertikal, 1: horizontal)				*/
/*	min_size:	minimale Slidergroesse						*/
/*----------------------------------------------------------------------------------------*/ 
static void _cdecl
Sl_size(struct Sl_size_args args)
#define tree      args.tree
#define base      args.base
#define slider    args.slider
#define num_items args.num_items
#define visible   args.visible
#define direction args.direction
#define min_size  args.min_size
{
	DEBUG(("Sl_size base %d, slider %d, num_items %d, visible %d, direction %d, min_size %d\n",
	       base, slider, num_items, visible, direction, min_size));

	if (direction)
	{
		/* horizontaler Slider */

		tree[slider].ob_x = 0;

		if (visible >= num_items) /* weniger Elemente als darstellbar? */
			tree[slider].ob_width = tree[base].ob_width;
		else
			tree[slider].ob_width = (short) (((long) tree[base].ob_width * visible) / num_items);
	
		if (tree[slider].ob_width < min_size)
			tree[slider].ob_width = min_size;
	}
	else
	{
		/* vertikaler Slider */

		tree[slider].ob_y = 0;

		if (visible >= num_items) /* weniger Elemente als darstellbar? */
			tree[slider].ob_height = tree[base].ob_height;
		else
			tree[slider].ob_height = (short) (((long) tree[base].ob_height * visible) / num_items);

		if (tree[slider].ob_height < min_size)
			tree[slider].ob_height = min_size;
	}
}
#undef min_size
#undef direction
#undef visible
#undef num_items
#undef slider
#undef base
#undef tree

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
static void
cpx_Sl_x(OBJECT *tree, short base, short slider, short value,
	 short min, short max, void __CDECL (*userdef)(void))
{
	short w;

	DEBUG(("Sl_x base %d, slider %d, value %d, min %d, max %d\n",
	       base, slider, value, min, max));

	w = tree[base].ob_width - tree[slider].ob_width;

	if (min <= max) /* Slider hat links Minimalwert und rechts Maximalwert? */
	{
		value -= min;
		max -= min;
		
		if (value < 0)
			value = 0;
		if (value > max)
			value = max;

		tree[slider].ob_x = max == 0 ? 0 : (short)(((long) w * value) / max);
	}
	else /* Slider hat links Maximalwert und rechts Minimalwert */
	{
		value -= max;
		min -= max;

		if (value < 0)
			value = 0;
		if (value > min)
			value = min;

		tree[slider].ob_x = w - (short)(((long) w * value) / min);
	}

	/* benutzerdefinierte Funktion aufrufen */
	cpx_userdef(userdef);
}
static void _cdecl
Sl_x(struct Sl_xy_args args)
{
	cpx_Sl_x(args.tree, args.base, args.slider, args.value,
		 args.min, args.max, args.userdef);
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
static void
cpx_Sl_y(OBJECT *tree, short base, short slider, short value,
	 short min, short max, void __CDECL (*userdef)(void))
{
	short h;

	DEBUG(("Sl_y base %d, slider %d, value %d, min %d, max %d\n",
	       base, slider, value, min, max));

	h = tree[base].ob_height - tree[slider].ob_height;

	if (min <= max) /* Slider hat oben Maximalwert und unten Minimalwert? */
	{
		value -= min;
		max -= min;
		
		if (value < 0)
			value = 0;
		if (value > max)
			value = max;

		tree[slider].ob_y = h - (max == 0 ? 0 : (short)(((long) h * value) / max));
	}
	else /* Slider hat oben Minimalwert und unten Maximalwert */
	{
		value -= max;
		min -= max;

		if (value < 0)
			value = 0;
		if (value > min)
			value = min;

		tree[slider].ob_y = (short)(((long) h * value) / min);
	}

	/* benutzerdefinierte Funktion aufrufen */
	cpx_userdef(userdef);
}
static void _cdecl
Sl_y(struct Sl_xy_args args)
{
	cpx_Sl_y(args.tree, args.base, args.slider, args.value,
		 args.min, args.max, args.userdef);
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
static void _cdecl
Sl_arrow(struct Sl_arrow_args args)
{
#define tree      args.tree
#define base      args.base
#define slider    args.slider
#define obj       args.obj
#define inc       args.inc
#define min       args.min
#define max       args.max
#define value     args.value
#define direction args.direction
#define userdef   args.userdef

	short val;

	DEBUG(("Sl_arrow base %d, slider %d, obj %d, inc %d, min %d, max %d, value %d, direction %d\n",
	       base, slider, obj, inc, min, max, *value, direction));

	val = *value + inc;			/* neue Position */

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

	*value = val;				/* Wert zurueckschreiben */
	
	if (obj >= 0)
	{
		obj_SELECTED(tree, obj);
		redraw_obj(tree, obj);		/* Button selektieren */
	}
	
	if (direction)				/* horizontaler Slider? */
		cpx_Sl_x(tree, base, slider, val, min, max, userdef);
	else
		cpx_Sl_y(tree, base, slider, val, min, max, userdef);

	redraw_obj(tree, base);			/* Slider neuzeichnen */

	evnt_timer(40);				/* warten, damit das Scrolling nicht zu schnell wird */
	if (obj >= 0)
	{
		obj_DESELECTED(tree, obj);
		redraw_obj(tree, obj);		/* Button wieder normal */
	}
#undef tree
#undef base
#undef slider
#undef obj
#undef inc
#undef min
#undef max
#undef value
#undef direction
#undef userdef
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
static void _cdecl
Sl_dragx(struct Sl_dragxy_args args)
#define tree    args.tree
#define base    args.base
#define slider  args.slider
#define min     args.min
#define max     args.max
#define value   args.value
#define userdef args.userdef
{
	short hpos;

	DEBUG(("Sl_dragx base %d, slider %d, min %d, max %d, value %d\n",
	       base, slider, min, max, *value));

	hpos = graf_slidebox(tree, base, slider,0);
	*value = (short)(((long)(max - min) * hpos) / 1000);
	*value += min;
	cpx_Sl_x(tree, base, slider, *value, min, max, userdef);

	/* Slider neuzeichnen */
	redraw_obj(tree, base);
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
static void _cdecl
Sl_dragy(struct Sl_dragxy_args args)
{
	short vpos;

	DEBUG(("Sl_dragy base %d, slider %d, min %d, max %d, value %d\n",
	       base, slider, min, max, *value));

	vpos = graf_slidebox(tree, base, slider, 1);
	*value = (short)(((long)(max - min) * (1000 - vpos)) / 1000);
	*value += min;
	cpx_Sl_y(tree, base, slider, *value, min, max, userdef);

	/* Slider neuzeichnen */
	redraw_obj(tree, base);
}
#undef userdef
#undef value
#undef max
#undef min
#undef slider
#undef base
#undef tree

static short
is_edit_obj_hidden(OBJECT *tree, short obj)
{	
	if (is_obj_HIDDEN(tree, obj))
		return 1;
	
	while (tree[obj].ob_next != -1)			/* Wurzel gefunden? */
	{
		short last_obj;
		
		last_obj = obj;
		obj = tree[obj].ob_next;		/* naechstes Objekt oder Vater */
		
		if (last_obj == tree[obj].ob_tail)	/* war das letzte Objekt ein Kind, ist obj der Vater? */
		{
			if (is_obj_HIDDEN(tree, obj))
				return 1;
		}
	}
	return 0;
}


_WORD find_obj(OBJECT *tree, _WORD startob, DIRS direction)
{
	_WORD obj;
	_WORD searchflag;
	_WORD curflag;
	_WORD inc;

	obj = 0;
	searchflag = OF_EDITABLE;
	inc = 1;

	switch (direction)
	{
	case BACKWARD:
		inc = -1;
		obj = startob + inc;
		break;

	case FORWARD:
		if (!(tree[startob].ob_flags & OF_LASTOB))
			obj = startob + inc;
		else
			obj = -1;
		break;

	case DEFAULTDIR:
		searchflag = OF_DEFAULT;
		break;
	case NODIR:
		return startob;
	}

	while (obj >= 0)
	{
		curflag = tree[obj].ob_flags;

		if (searchflag & curflag)
			return obj;

		if (curflag & OF_LASTOB)
			obj = -1;
		else
			obj += inc;
	}
	return startob;
}



/*----------------------------------------------------------------------------------------*/ 
/* (Xform_do) form_do() fuer CPX-Fenster simulieren					  */
/* Funktionsergebnis:	CPX_DESC des neuen Kontexts					  */
/*	cpx_desc:	CPX-Beschreibung						  */
/*	tree:		Objektbaum des CPX						  */
/*	edit_obj:	Nummer des aktiven Editobjekts oder 0				  */
/*	msg:		Zeiger auf Message-Buffer fuer ausgewaehlte Ereignisse		  */
/*----------------------------------------------------------------------------------------*/ 
void
cpx_form_do(CPX_DESC *cpx_desc, OBJECT *tree, _WORD edit_obj, _WORD *msg)
{
	DIALOG *dialog;
	_WORD last_edit_obj;

	DEBUG_CALLBACK(cpx_desc);

	dialog = cpx_desc->dialog;

	if (tree != cpx_desc->tree)				/* Objektbaum geaendert? */
	{
		if (cpx_desc->tree == NULL)
			cpx_desc->size.g_y -= desk_grect.g_h;	/* CPX-Rechteck wieder innerhalb des Schirms */

		cpx_desc->tree = tree;
		obj_HIDDEN(tree, ROOT);				/* Baum nicht zeichnen! */
		if (dialog)
			wdlg_set_tree(dialog, tree);			/* neuen Objektbaum setzen */
		tree[ROOT].ob_x = cpx_desc->size.g_x;
		tree[ROOT].ob_y = cpx_desc->size.g_y;
		obj_VISIBLE(tree, ROOT);

		if (edit_obj == 0)
			edit_obj = find_obj(tree, 0, FORWARD);
	
		cpx_desc->edit_obj = edit_obj;
		cpx_desc->cursor_idx = -1;
		cpx_desc->cursor = FALSE;
	}

	if (dialog)
	{
		last_edit_obj = wdlg_get_edit(dialog, &cpx_desc->cursor_idx);		/* Editobjekt erfragen */
	} else
	{
		last_edit_obj = 0;
	}

	if (last_edit_obj > 0)					/* Editobjekt aktiv? */
	{
		if (dialog)
		{
			if (is_edit_obj_hidden(tree, last_edit_obj))	/* Editobjekt unsichtbar? */
			{
				obj_HIDDEN(tree, ROOT);			/* Baum nicht zeichnen! */
				wdlg_set_tree(dialog, tree);		/* Objektbaum nochmals setzen, Cursorzustand zuruecksetzen */
				obj_VISIBLE(tree, ROOT);
			}
		}
	} 

	if (edit_obj)
	{
		if (dialog)
		{
			wdlg_set_edit(dialog, edit_obj);		/* Editobjekt setzen */
		} else
		{
		}
	}

	cpx_desc->button = -1;					/* noch kein Button gedrueckt */
	cpx_desc->msg = msg;

	cpx_main_loop();
}

/*----------------------------------------------------------------------------------------*/ 
/* (GetFirstRect) erstes Redraw-Rechteck fuer CPX-Fenster zurueckliefern		*/
/* Funktionsergebnis:	Redraw-Rechteck oder NULL					*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*	redraw_area:	Maximale Groesse des insgesamt zu zeichnenden Bereichs		*/
/*----------------------------------------------------------------------------------------*/ 
static GRECT *cpx_get_first_rect(CPX_DESC *cpx_desc, GRECT *redraw_area)
{
	GRECT *w;

	DEBUG_CALLBACK(cpx_desc);

	cpx_desc->redraw_area = *redraw_area; /* fuer Aufruf von cpx_get_next_rect() merken */
	w = &cpx_desc->dirty_area;

	DEBUG(("cpx_get_first_rect: redraw_area is x %d y %d w %d h %d\n",
	       redraw_area->g_x, redraw_area->g_y, redraw_area->g_w, redraw_area->g_h));

	/* first redraw rectangle */
	wind_get_grect(cpx_desc->whdl, WF_FIRSTXYWH, w);

	while (w->g_w)
	{
		if (rc_intersect(&cpx_desc->redraw_area, w))
		{
			DEBUG(("cpx_get_first_rect: x %d y %d w %d h %d\n",
			       w->g_x, w->g_y, w->g_w, w->g_h));

			/* zu zeichnender Bereich */
			return w;
		}

		/* naechstes Redraw-Rechteck */
		wind_get_grect(cpx_desc->whdl, WF_NEXTXYWH, w);
	}

	DEBUG(("cpx_get_first_rect: no more rectangles\n"));

	return NULL;

}

static GRECT * _cdecl GetFirstRect(GRECT *prect)
{
	CPX_DESC *cpx;
	void *pc = ((void **)&prect)[-1];

	cpx = find_cpx_by_addr(pc);
	DEBUG_CALLBACK(cpx);

	if (cpx)
	{
		return cpx_get_first_rect(cpx, prect);
	}

	return NULL;
}

/*----------------------------------------------------------------------------------------*/ 
/* (GetNextRect) naechstes Redraw-Rechteck fuer CPX-Fenster zurueckliefern		*/
/* Funktionsergebnis:	Redraw-Rechteck oder NULL						*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
static GRECT *cpx_get_next_rect(CPX_DESC *cpx_desc)
{
	GRECT *w = &(cpx_desc->dirty_area);

	DEBUG_CALLBACK(cpx_desc);

	/* naechstes Redraw-Rechteck */
	wind_get_grect(cpx_desc->whdl, WF_NEXTXYWH, w);

	while (w->g_w)
	{
		if (rc_intersect(&cpx_desc->redraw_area, w))
		{
			DEBUG(("cpx_get_next_rect: x %d y %d w %d h %d\n",
			       w->g_x, w->g_y, w->g_w, w->g_h));

			/* zu zeichnender Bereich */
			return w;
		}

		/* naechstes Redraw-Rechteck */
		wind_get_grect(cpx_desc->whdl, WF_NEXTXYWH, w);
	}

	DEBUG(("cpx_get_next_rect: no more rectangles\n"));
	return NULL;
}

static GRECT * _cdecl GetNextRect(void *args)
{
	CPX_DESC *cpx;
	void *pc = (&args)[-1];

	cpx = find_cpx_by_addr(pc);
	DEBUG_CALLBACK(cpx);

	if (cpx)
		return cpx_get_next_rect(cpx);

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
static void
cpx_set_evnt_mask(CPX_DESC *cpx_desc, short mask, MOBLK *m1, MOBLK *m2, long timeout)
{
	DEBUG_CALLBACK(cpx_desc);

	if (m1)				/* erstes Mausrechteck? */
		cpx_desc->m1 = *m1;
	else
		mask &= ~MU_M1;
			
	if (m2)				/* zweites Mausrechteck? */
		cpx_desc->m2 = *m2;
	else
		mask &= ~MU_M2;

	if (mask & MU_TIMER)		/* Timer? */
		cpx_desc->time = timeout;
	else
		cpx_desc->time = 0;

	cpx_desc->mask = mask;		/* zusaetzliche Ereignismaske */
}

static void _cdecl Set_Evnt_Mask(struct Set_Evnt_Mask_args args)
{
	CPX_DESC *cpx;
	void *pc = ((void **)&args)[-1];

	cpx = find_cpx_by_addr(pc);
	DEBUG_CALLBACK(cpx);

	if (cpx)
	{
		cpx_set_evnt_mask(cpx, args.mask, args.m1, args.m2, args.evtime);
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* Alertbox fuer CPX aufrufen								*/
/* Funktionsergebnis:	Buttonnummer							*/
/*	alert_id:	Art der Alertbox						*/
/*----------------------------------------------------------------------------------------*/ 
static short _cdecl
XGen_Alert(struct XGen_Alert_args args)
{
	DEBUG(("XGen_Alert\n"));

	switch (args.alert_id)
	{
	/* Voreinstellungen sichern? */
	case XAL_SAVE_DEFAULTS:
		if (form_alert(1, fstring_addr[SAVE_DFLT_ALERT]) == 1)
			return 1;
		break;

	/* nicht genuegend Speicher */
	case XAL_MEM_ERR:
		form_alert(1, fstring_addr[MEM_ERR_ALERT]);
		return 0;

	/* Schreib- oder Lesefehler */
	case XAL_FILE_ERR:
		form_alert(1, fstring_addr[FILE_ERR_ALERT]);
		return 0;

	/* Datei nicht gefunden */
	case XAL_FILE_NOT_FOUND:
		form_alert(1, fstring_addr[FNF_ERR_ALERT]);
		return 0;

	case XAL_NO_SOUND_DMA:
		form_alert(1, fstring_addr[AL_NO_SOUND_DMA]);
		return 0;
	}

	return 0;
}

/*----------------------------------------------------------------------------------------*/ 
/* (CPX_Save) Daten ab Beginn des Datasegments eines CPX speichern			*/
/* Funktionsresultat:	0: Fehler 1: alles in Ordnung					*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
static short cpx_save_data(CPX_DESC *cpx_desc, void *buf, long bytes)
{
	char name[256];
	long handle, offset;
	short result = 0;

	DEBUG_CALLBACK(cpx_desc);

	strcpy(name, settings.cpx_path);
	strcat(name, cpx_desc->file_name);

	offset = sizeof(struct cpxhead) + sizeof(struct program_header) + cpx_desc->segm.len_text;
	if (bytes > cpx_desc->segm.len_data) /* laenger als das Data-Segment? */
		bytes = cpx_desc->segm.len_data;

	handle = Fopen(name, O_WRONLY);
	if (handle > 0)
	{
		if (Fseek(offset, (short)handle, 0) == offset)
		{
			if (Fwrite((short)handle, bytes, buf) == bytes)
				result = 1;
		}
		Fclose((short)handle);
	}	

	return result;
}

static short _cdecl
CPX_Save(void *ptr, long bytes)
{
	CPX_DESC *cpx;
	void *pc = ((void **)&ptr)[-1];

	cpx = find_cpx_by_addr(pc);
	DEBUG_CALLBACK(cpx);

	if (cpx)
	{
		return cpx_save_data(cpx, ptr, bytes);
	}

	return 0;
}

/*----------------------------------------------------------------------------------------*/ 
/* (Get_Buffer) Zeiger auf temporaeren CPX-Speicher zurueckliefern 			*/
/* Funktionsresultat:	Zeiger auf 64-Bytes						*/
/*	cpx_desc:	CPX-Beschreibung						*/
/*----------------------------------------------------------------------------------------*/ 
static void * _cdecl Get_Buffer(void *args)
{
	CPX_DESC *cpx;
	void *pc = (&args)[-1];

	cpx = find_cpx_by_addr(pc);
	DEBUG_CALLBACK(cpx);

	if (cpx)
		return cpx->old.header.buffer;

	return NULL;
}

/*----------------------------------------------------------------------------------------*/ 
/* Mausform setzen oder sichern								*/
/* Funktionsresultat:	-								*/
/*	flag: 0: Mausform setzen 1: Mausform sichern					*/
/*	mf: Zeiger auf Mausform								*/
/*----------------------------------------------------------------------------------------*/ 
static void _cdecl MFsave(struct MFsave_args args) /* contributed by Arnaud */
{
	DEBUG(("MFsave\n"));

	if (args.flag)
	{
		if (aes_flags & GAI_MOUSE)
			graf_mouse(M_SAVE, args.mf);
		else
			/* nothing */ ;
	}
	else
	{
		if (aes_flags & GAI_MOUSE)
			graf_mouse(M_RESTORE, args.mf);
		else
			graf_mouse(ARROW, NULL);
	}
}
