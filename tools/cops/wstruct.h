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

#ifndef _wstruct_h
#define _wstruct_h

typedef struct _window
{
	struct _window *next;		/* Zeiger auf naechste Struktur */

	short	handle;			/* Fensterhandle */
	short	kind;			/* Fensterkomponenten */

	GRECT	border;			/* Fensteraussnflaeche */
	GRECT	workarea;		/* Fensterinnenflaeche */
	GRECT	saved_border;		/* bei Iconify gesicherte Fensteraussnflaeche */

	char	*name;			/* Zeiger auf den Fensternamen oder 0L */
	char	*iconified_name;	/* Fenstername fuer ikonifiziertes Fenster oder 0L */
	
	struct
	{
		unsigned reserved:	7;	/* reserviert */
		unsigned hide_cursor:	1;	/* 0: Cursor nicht ausschalten (wird von der Redraw-Routine erledigt)
						 * 1: Cursor vor Redraw ausschalten */
		unsigned snap_width:	1;	/* Breite auf Vielfache von dx anpassen */
		unsigned snap_height:	1;	/* Hoehe auf Vielfache von dy anpassen */
		unsigned snap_x	:	1;	/* x-Koordinate auf Vielfache von dx anpassen */
		unsigned snap_y	:	1;	/* y-Koordinate auf Vielfache von dy anpassen */
		unsigned smart_size:	1;	/* Smart Redraw fuers Sizen */ 
		unsigned limit_wsize:	1;	/* maximale Fenstergroess begrenzen */
		unsigned fuller:	1;	/* Fenster hat maximale Groess */
		unsigned iconified:	1;	/* Fenster ist ikonifiziert */
	} wflags;

	void	(*redraw)(struct _window *window, GRECT *area); /* Zeiger auf eine Redraw-Routine */
	
	long	interior_flags;		/* Art des Inhalts */
	void	*interior;		/* Zeiger auf den Inhalt */
	
	long	x;			/* x-Koordinate der linken oberen Ecke */
	long	y;			/* y-Koordinate der linken oberen Ecke */
	long	w;			/* Breite der Arbeitsflaeche */
	long	h;			/* Hoehe der Arbeitsflaeche */

	short	dx;			/* Breite einer Scrollspalte in Pixeln */
	short	dy;			/* Hoehe einer Scrollzeile in Pixeln */
	short	snap_dx;		/* Vielfaches fuer horizontale Fensterposition */
	short	snap_dy			/* Vielfaches fuer vertikale Fensterposition */;							
	short	snap_dw;		/* Vielfaches der Fensterbreite */
	short	snap_dh;		/* Vielfaches der Fensterhoehe */
	short	limit_w;		/* maximal anzuzeigende Breite der Arbeitsflaeche oder 0 */
	short	limit_h;		/* maximal anzuzeigende Hoehe der Arbeitsflaeche oder 0 */

	short	hslide;			/* Position des horizontalen Sliders in Promille */
	short	vslide;			/* Position des vertikalen Sliders in Promille */
	short	hslsize;		/* Groess des horizontalen Sliders in Promille */
	short	vslsize;		/* Groess des vertikalen Sliders in Promille */

} WINDOW;

#endif /* _wstruct_h */
