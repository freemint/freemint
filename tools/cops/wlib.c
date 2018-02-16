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

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cops.h"
#include "list.h"
#include "wlib.h"

/*----------------------------------------------------------------------------------------*/
/* Defines                                                                                */
/*----------------------------------------------------------------------------------------*/
#define	WWORK window->workarea

typedef enum { FALSE = (0 == 1), TRUE  = (1 == 1) } boolean;

/*----------------------------------------------------------------------------------------*/
/* Globale Variablen                                                                      */
/*----------------------------------------------------------------------------------------*/
short app_id;
short vdi_handle;
static short work_out[57];
			
static WINDOW *window_list = NULL;

/*----------------------------------------------------------------------------------------*/
static void scroll_horizontal(WINDOW *window, short dx);
static void scroll_vertical(WINDOW *window, short dy);

/*----------------------------------------------------------------------------------------*/
/* Library initialisieren */
/* Funktionsergebnis:	0: Fehler 1: alles in Ordnung */
/*	id:		AES-Programm-ID */
/*----------------------------------------------------------------------------------------*/
short
init_wlib(short id)
{
	short work_in[11];
	short i;
			
	app_id = id; /* AES-Programm-ID */
	window_list = NULL;

	for (i = 1; i < 10 ; i++)
		work_in[i] = 1;

	work_in[0] = Getrez() + 2; /* Aufloesung */
	work_in[10] = 2; /* Rasterkoordinaten benutzen */

	vdi_handle = graf_handle(&i, &i, &i, &i);
	v_opnvwk(work_in, &vdi_handle, work_out);

	if (vdi_handle > 0)
		return 1;

	return 0;
}

/*----------------------------------------------------------------------------------------*/
/* Library zuruecksetzen */
/* Funktionsergebnis:	0: Fehler 1: alles in Ordnung */
/*----------------------------------------------------------------------------------------*/
short
reset_wlib(void)
{
	while (window_list)
		delete_window(window_list->handle); /* Fenster loeschen */

	v_clsvwk(vdi_handle); /* Workstation schliessen */

	return 1;
}

/*----------------------------------------------------------------------------------------*/
/* Zeiger auf die Fensterliste zurueckliefern */
/* Funktionsergebnis:	Zeiger auf das erste Fenster oder NULL */
/*----------------------------------------------------------------------------------------*/
WINDOW *
get_window_list(void)
{
	return window_list; /* Zeiger auf das erste Element der Fensterliste */
}

/*----------------------------------------------------------------------------------------*/
/* Fenster beim AES anmelden, aber nicht zeichnen                                        */
/* Funktionsresultat:	Zeiger auf eine WINDOW-Struktur oder NULL bei Fehlern */
/*	kind:		Fenster-Elemente */
/*	border:		Zeiger auf ein GRECT mit den maximalen aesseren Fenstermassen */
/*	handle:		Zeiger auf das Fensterhandle (bei Fehler < 0) */
/*	name:		Zeiger auf den Fensternamen oder NULL */
/*----------------------------------------------------------------------------------------*/
WINDOW *
create_window(short kind, GRECT *border, short *handle, char *name, char *iconified_name)
{
	WINDOW *new = NULL;

	*handle = wind_create_grect(kind, border);
	if (*handle >= 0)
	{
		GRECT workarea;
		unsigned long size;

		wind_calc_grect(WC_WORK, kind, border, &workarea);

		if (name) /* Fensternamen vorhanden? */
		{
			size = (strlen(name) + 1) & 0xfffe;
			if (size < 128) /* mindestens 128 Bytes fuer Fensternamen */
				size = 128;

			if (iconified_name) /* 16 Bytes fuer Namen des ikonifizierten Fensters */
				size += 16;

			size += sizeof(WINDOW);
		}
		else
			size = sizeof(WINDOW);
			
		new = malloc(size);
		assert(new);

		new->handle = *handle;		/* Fensterhandle */
		new->kind = kind;		/* Fensterattribute */
		new->border = *border; 		/* Fensteraussenflaeche */
		new->workarea = workarea;	/* Fensterinnenflaeche */

		if (name) /* Fensternamen vorhanden? */
		{
			new->name = (char *)new + sizeof(WINDOW);
			strcpy(new->name, name);
			wind_set_str(new->handle, WF_NAME, new->name);

			if (iconified_name)
			{
				new->iconified_name = (char *)new + size - 16;
				strncpy(new->iconified_name, iconified_name, 15);
				new->iconified_name[15] = 0;
			}
		}
		else
		{
			new->name = NULL; /* kein Fenstername */
			new->iconified_name = NULL;
		}

		new->wflags.hide_cursor = 1;	/* Cursor vor dem Redraw immer ausschalten */
		new->wflags.snap_width = 0;	/* Breite nicht auf Vielfache von dx anpassen */
		new->wflags.snap_height = 0;	/* Hoehe nicht auf Vielfache von dy anpassen */
		new->wflags.snap_x = 0;		/* x-Koordinate nicht auf Vielfache von dx anpassen */
		new->wflags.snap_y = 0;		/* y-Koordinate nicht auf Vielfache von dy anpassen */
		new->wflags.smart_size = 1;	/* Smart Redraw fuers Sizen */
		new->wflags.limit_wsize = 0;	/* maximale Fenstergroesse nicht begrenzen */
		new->wflags.fuller = 0;		/* Fuller wurde noch nicht betaetigt */
		new->wflags.iconified = 0;	/* Iconifier wurde noch nicht betaetigt */
	
		new->redraw = 0L;

		new->interior_flags = 0L;
		new->interior = 0L;

		new->x = 0;			/* x-Koordinate der linken oberen Ecke */
		new->y = 0;			/* y-Koordinate der linken oberen Ecke */
		new->w = 0;			/* Breite der Arbeitsflaeche */
		new->h = 0;			/* Hoehe der Arbeitsflaeche */
		new->dx = 0;			/* Breite einer Scrollspalte in Pixeln */
		new->dy = 0;			/* Hoehe einer Scrollzeile in Pixeln */
		new->snap_dx = 1;		/* Vielfaches fuer horizontale Fensterposition */
		new->snap_dy = 1;		/* Vielfaches fuer vertikale Fensterposition */
		new->snap_dw = 1;		/* Vielfaches der Fensterbreite */
		new->snap_dh = 1;		/* Vielfaches der Fensterhoehe */
		new->limit_w = 0;		/* Arbeitsflaeche nicht begrenzen */
		new->limit_h = 0;		/* Arbeitsflaeche nicht begrenzen */

		new->hslide = 0;		/* Position des hor. Sliders in Promille */
		new->vslide = 0;		/* Position des ver. Sliders in Promille */
		new->hslsize = 0;		/* Groesse des hor. Sliders in Promille */
		new->vslsize = 0;		/* Groesse des ver. Sliders in Promille */

		new->next = 0L;			/* keine Folgestruktur vorhanden */
	
		list_insert((void **) &window_list, new, offsetof(WINDOW, next));
	}

	return new;
}

/*----------------------------------------------------------------------------------------*/
/* Redraw eines Fensters */
/* Funktionsergebnis:	- */
/* handle:		Fensterhandle */
/* area:		Zeiger auf ein GRECT mit den Ausmassen des zu zeichnenden Bereichs */
/*----------------------------------------------------------------------------------------*/
void
redraw_window(short handle, GRECT *area)
{
	GRECT box, desk;
	WINDOW *window;

	window = search_struct(handle);
	if (window)
	{
		wind_update(BEG_UPDATE); /* Bildschirm fuer andere Apps sperren */
		if (window->wflags.hide_cursor)
			graf_mouse(M_OFF, 0L); /* Maus ausschalten */
		
		wind_get_grect(0, WF_WORKXYWH, &desk); /* Groesse des Hintergrunds erfragen */
		wind_get_grect(window->handle, WF_FIRSTXYWH, &box); /* erstes Element der Rechteckliste erfragen */
		
		while (box.g_w && box.g_h) /* noch gueltige Rechtecke vorhanden? */
		{
			if (rc_intersect(&desk, &box)) /* sichtbar? */
			{
				if (rc_intersect(area, &box)) /* innerhalb des zu zeichnenden Bereichs? */
					window->redraw(window, &box); /* Bereich box zeichnen */
			}
			wind_get_grect(window->handle, WF_NEXTXYWH, &box); /* naechstes Element der Rechteckliste holen */
		}

		if (window->wflags.hide_cursor)
			graf_mouse(M_ON, 0L); /* Maus an */
		wind_update(END_UPDATE); /* Bildschirm freigeben */
	}
}

/*----------------------------------------------------------------------------------------*/
/* Fuer ein Fensterhandle die WINDOW-Struktur suchen */
/* Funktionsergebnis:	Zeiger auf die WINDOW-Struktur oder 0L */
/*	handle:		Fensterhandle */
/*----------------------------------------------------------------------------------------*/
WINDOW *
search_struct(short handle)
{
	WINDOW *search;

	search = window_list;
	while (search != 0L)
	{
		if (search->handle == handle)
			return search;

		search = search->next;
	}

	return NULL;
}

/*----------------------------------------------------------------------------------------*/
/* Fenster beim AES abmelden und Speicher fuer WINDOW-Struktur freigeben */
/* Funktionsergebnis:	- */
/* handle:		Fensterhandle */
/*----------------------------------------------------------------------------------------*/
void
delete_window(short handle)
{
	WINDOW *window;

	window = search_struct(handle);
	if (window)	
	{
		list_remove((void **) &window_list, window, offsetof(WINDOW, next));

		wind_close(window->handle);
		wind_delete(window->handle);
		free(window);
	}
}

/*----------------------------------------------------------------------------------------*/
/* Fenster bewegen */
/* Funktionsergebnis:	- */
/* handle:		Fensterhandle */
/* area:		Fenstergroesse */
/*----------------------------------------------------------------------------------------*/
void
move_window(short handle, GRECT *area)
{
	WINDOW *window;
	GRECT desk;

	wind_update(BEG_UPDATE); /* Bildschirm sperren */

	window = search_struct(handle);
	if (window)
	{
		short kind;

		if (window->wflags.iconified)
			kind = NAME + MOVER;
		else
			kind = window->kind;

		wind_get_grect(0, WF_WORKXYWH, &desk); /* Groesse des Hintergrunds erfragen */

		wind_calc_grect(WC_WORK, kind, area, &window->workarea);

		if (window->wflags.snap_x)				
		{
			WWORK.g_x += window->snap_dx - 2;	
			if (WWORK.g_x > (desk.g_x + desk.g_w - 32))
				WWORK.g_x = desk.g_x + desk.g_w - 32;
			WWORK.g_x -= WWORK.g_x % window->snap_dx;
		}

		if (window->wflags.snap_y)
		{
			WWORK.g_y += window->snap_dy - 1;
			if (WWORK.g_y > (desk.g_y + desk.g_h - 2))
				WWORK.g_y = desk.g_y + desk.g_h - 2;
			WWORK.g_y -= WWORK.g_y % window->snap_dy;
		}

		wind_calc_grect(WC_BORDER, kind, &window->workarea, &window->border);
		wind_set_grect(handle, WF_CURRXYWH, &window->border);
	}

	wind_update(END_UPDATE); /* Bildschirm freigeben */
}

/*----------------------------------------------------------------------------------------*/
/* Scrolling */
/* Funktionsergebnis:	- */
/* handle:		Fensterhandle */
/* command:		Gibt an, auf welche Art zu scrollen ist */
/*----------------------------------------------------------------------------------------*/
void
arr_window(short handle, short command)
{
	WINDOW *window;

	wind_update(BEG_UPDATE); /* Bildschirm freigeben */

	window = search_struct(handle);
	if (window)
	{
		switch (command)
		{
		case WA_UPPAGE: up_window(window, window->workarea.g_h); break;
		case WA_DNPAGE: dn_window(window, window->workarea.g_h); break;
		case WA_UPLINE: up_window(window, window->dy); break;
		case WA_DNLINE: dn_window(window, window->dy); break; 
		case WA_LFPAGE: lf_window(window, window->workarea.g_w); break;
		case WA_RTPAGE: rt_window(window, window->workarea.g_w); break;
		case WA_LFLINE: lf_window(window, window->dx); break;
		case WA_RTLINE: rt_window(window, window->dx); break;
		}
	}

	wind_update(END_UPDATE); /* Bildschirm freigeben */
}

/*----------------------------------------------------------------------------------------*/
/* Um dy Zeilen nach oben Scrollen */
/* Funktionsergebnis:	- */
/* window:		Zeiger auf Fensterstruktur */
/*	dy:		Anzahl der Zeilen */
/*----------------------------------------------------------------------------------------*/
void
up_window(WINDOW *window, long dy)
{
	if (window->y > 0) /* Slider schon oben? */
	{
		wind_update(BEG_UPDATE); /* Bildschirm fuer andere Apps sperren */
		graf_mouse(M_OFF, 0L); /* Maus ausschalten */

		if (window->y < dy)
			dy = window->y;

		window->y -= dy; /* dy Pixelzeilen weiter nach oben */

		scroll_vertical(window, -dy); /* nach links scrollen */

		graf_mouse(M_ON, 0L); /* Maus einschalten */
		wind_update(END_UPDATE); /* Bildschirm freigeben */
	}
}

/*----------------------------------------------------------------------------------------*/
/* Um dy Zeilen nach unten Scrollen */
/* Funktionsergebnis:	- */
/* window:		Zeiger auf Fensterstruktur */
/*	dy:		Anzahl der Zeilen */
/*----------------------------------------------------------------------------------------*/
void
dn_window(WINDOW *window, long dy)
{
	if ((window->y + window->workarea.g_h) < window->h) /* Slider bereits unten? */
	{
		wind_update(BEG_UPDATE); /* Bildschirm fuer andere Apps sperren */
		graf_mouse(M_OFF, 0L); /* Maus ausschalten */

		if ((window->y + window->workarea.g_h + dy) > window->h)
			dy = window->h - window->y - window->workarea.g_h;

		window->y += dy;
		scroll_vertical(window, dy); /* nach unten scrollen */

		graf_mouse(M_ON, 0L); /* Maus einschalten */
		wind_update(END_UPDATE); /* Bildschirm freigeben */
	}
}

/*----------------------------------------------------------------------------------------*/
/* Um dx Spalten nach links Scrollen */
/* Funktionsergebnis:	- */
/* window:		Zeiger auf Fensterstruktur */
/*	dx:		Anzahl der Spalten */
/*----------------------------------------------------------------------------------------*/
void
lf_window(WINDOW *window, long dx)
{
	if (window->x > 0) /* Slider schon linnks? */
	{
		wind_update(BEG_UPDATE); /* Bildschirm fuer andere Apps sperren */
		graf_mouse(M_OFF, 0L); /* Maus ausschalten */

		if (window->x < dx) /* kann um dx Pixelspalten gescrollt werden? */
			dx = window->x;

		window->x -= dx; /* dx Pixelzeilen weiter nach oben */
		scroll_horizontal(window, -dx); /* nach links scrollen */

		graf_mouse(M_ON, 0L); /* Maus einschalten */
		wind_update(END_UPDATE); /* Bildschirm freigeben */
	}
}

/*----------------------------------------------------------------------------------------*/
/* Um dx Spalten nach rechts Scrollen */
/* Funktionsergebnis:	- */
/* window:		Zeiger auf Fensterstruktur */
/*	dx:		Anzahl der Spalten */
/*----------------------------------------------------------------------------------------*/
void
rt_window(WINDOW *window, long dx)
{
	if ((window->x + window->workarea.g_w) < window->w) /* Slider bereits rechts? */
	{
		wind_update(BEG_UPDATE); /* Bildschirm fuer andere Apps sperren */
		graf_mouse(M_OFF, 0L); /* Maus ausschalten */

		if ((window->x + window->workarea.g_w + dx) > window->w)
			dx = window->w - window->x - window->workarea.g_w;

		window->x += dx;
		scroll_horizontal(window, dx); /* nach rechts scrollen */

		graf_mouse(M_ON, 0L); /* Maus einschalten */
		wind_update(END_UPDATE);/* Bildschirm freigeben */
	}
}

/*----------------------------------------------------------------------------------------*/
/* Horizontale Sliderposition setzen */
/* Funktionsergebnis:	- */
/* handle:		Fensterhandle */
/* hslid:		neue Sliderposition in Promille */
/*----------------------------------------------------------------------------------------*/
void
hlsid_window(short handle, short hslid)
{
	WINDOW *window;

	wind_update(BEG_UPDATE); /* Bildschirm sperren */
	graf_mouse(M_OFF, 0L); /* Maus ausschalten */

	window = search_struct(handle);
	if (window)
	{
		long old_x = window->x;

		if ((WWORK.g_w + window->x) > window->w) /* Fenster breiter als der sichtbare Inhalt? */
		{
			window->x = hslid * window->x / 1000;
			set_slsize(window);
		}
		else
			window->x = hslid * (window->w - window->workarea.g_w) / 1000;

		if (window->wflags.snap_width)
			window->x -= window->x % window->snap_dw;

		if (old_x != window->x) /* Sliderposition veraendert? */
			scroll_horizontal(window, window->x - old_x);
	}

	graf_mouse(M_ON, 0L); /* Maus einschalten */
	wind_update(END_UPDATE); /* Bildschirm freigeben */
}

/*----------------------------------------------------------------------------------------*/
/* Vertikale Sliderposition setzen */
/* Funktionsergebnis:	- */
/* handle:		Fensterhandle */
/* vslid:		neue Sliderposition in Promille */
/*----------------------------------------------------------------------------------------*/
void
vslid_window(short handle, short vslid)
{
	WINDOW *window;
	
	wind_update(BEG_UPDATE); /* Bildschirm fuer andere Apps sperren */
	graf_mouse(M_OFF, 0L); /* Maus ausschalten */

	window = search_struct(handle);
	if (window)
	{
		long	old_y;
		
		old_y = window->y;
		
		if ((WWORK.g_h + window->y) > window->h) /* Fenster hoeher als der sichtbare Inhalt? */
		{
			window->y = vslid * window->y / 1000;
			set_slsize(window);
		}
		else
			window->y =  vslid * (window->h - window->workarea.g_h) / 1000;
	
		if (window->wflags.snap_height)
			window->y -= window->y % window->snap_dh;

		if (old_y != window->y) /* Sliderposition veraendert? */
			scroll_vertical(window, window->y - old_y);
	}

	graf_mouse(M_ON, 0L); /* Maus einschalten */
	wind_update(END_UPDATE); /* Bildschirm freigeben */
}

/*----------------------------------------------------------------------------------------*/
/* Horizontales Scrolling (BEG_UPDATE muss bereits gesetzt sein) */
/* Funktionsergebnis:	- */
/*	window:		Fensterstruktur */
/*	dx:		horizontale Verschiebung (< 0: nach links > 0: nach rechts) */
/*----------------------------------------------------------------------------------------*/
static void
scroll_horizontal(WINDOW *window, short dx)
{
	GRECT box;
	GRECT desk;
	MFDB screen;
	RECT16 rect[2];

	if (dx)
	{
		wind_get_grect(0, WF_WORKXYWH, &desk); /* Groesse des Hintergrunds erfragen */
		wind_get_grect(window->handle, WF_FIRSTXYWH, &box); /* erstes Element der Rechteckliste erfragen */

		while (box.g_w && box.g_h) /* Ende der Rechteckliste? */
		{
			if (rc_intersect(&desk, &box)) /* Rechteck innerhalb des Schirms? */
			{			
				if (rc_intersect(&window->workarea, &box)) /* Rechteck innerhalb des Fensters? */
				{			
					if (ABS(dx) < box.g_w) /* kann verschoben werden? */
					{
						screen.fd_addr = 0L; /* Bildschirm */
					
						*(GRECT *)rect = box;
						rect[0].x2 += box.g_x - 1;
						rect[0].y2 += box.g_y - 1;
		
						vs_clip(vdi_handle, 1, (short *) rect);	/* Clipping-Rechteck setzen */
					
						rect[1] = rect[0];
						
						if (dx >= 0)
						{
							rect[0].x1 += dx;
							rect[1].x2 -= dx;
						
						   box.g_x += box.g_w - dx;
						}
						else
						{
							rect[0].x2 += dx;
							rect[1].x1 -= dx;
						}
				  		box.g_w = ABS(dx);
					
						vro_cpyfm(vdi_handle, S_ONLY, (short *) rect, &screen, &screen);
					   
					}
					window->redraw(window, &box); /* Bereich box zeichnen */
				}	
			}	
			wind_get_grect(window->handle, WF_NEXTXYWH, &box); /* naechstes Element der Rechteckliste holen */
		}		

		if ((WWORK.g_w + window->x) > window->w) /* Fenster breiter als der sichtbare Inhalt? */
			set_slsize(window);
		else
			set_slpos(window);
	}
}

/*----------------------------------------------------------------------------------------*/
/* Vertikales Scrolling (BEG_UPDATE muss bereits gesetzt sein) */
/* Funktionsergebnis:	- */
/*	window:		Fensterstruktur */
/*	dy:		vertikale Verschiebung (< 0: nach oben > 0: nach unten) */
/*----------------------------------------------------------------------------------------*/
static void
scroll_vertical(WINDOW *window, short dy)
{
	GRECT box;
	GRECT desk;
	MFDB screen;
	RECT16 rect[2];

	if (dy) /* Sliderposition veraendert? */
	{
		wind_get_grect(0, WF_WORKXYWH, &desk); /* Groesse des Hintergrunds erfragen */
		wind_get_grect(window->handle, WF_FIRSTXYWH, &box); /* erstes Element der Rechteckliste erfragen */

		while (box.g_w && box.g_h) /* Ende der Rechteckliste? */
		{
			if (rc_intersect(&desk, &box)) /* Rechteck innerhalb des Schirms? */
			{			
				if (rc_intersect(&window->workarea, &box)) /* Rechteck innerhalb des Fensters? */
				{			
					if (ABS(dy) < box.g_h) /* kann verschoben werden? */
					{
						screen.fd_addr = 0L; /* Bildschirm */

						*(GRECT *)rect = box;
						rect[0].x2 += box.g_x - 1;
						rect[0].y2 += box.g_y - 1;

						vs_clip(vdi_handle, 1, (short *) rect);	/* Clipping-Rechteck setzen */

						rect[1] = rect[0];

						if (dy >= 0) /* nach unten scrollen? */
						{
							rect[0].y1 += dy;
							rect[1].y2 -= dy;

							box.g_y += box.g_h - dy;
						}
						else /* nach oben scrollen */
						{
							rect[0].y2 += dy;
							rect[1].y1 -= dy;
						}

				  		box.g_h = ABS(dy);

						vro_cpyfm(vdi_handle, S_ONLY, (short *) rect, &screen, &screen);
					}
					window->redraw(window, &box); /* Bereich box zeichnen */
				}	
			}	
			wind_get_grect(window->handle, WF_NEXTXYWH, &box); /* naechstes Element der Rechteckliste holen */
		}		

		if ((WWORK.g_h + window->y) > window->h) /* Fenster hoeher als der sichtbare Inhalt? */
			set_slsize(window);
		else
			set_slpos(window);
	}
}


/*----------------------------------------------------------------------------------------*/
/* Fenstergroesse veraendern */
/* Funktionsergebnis:	- */
/* handle:		Fensterhandle */
/* size:		Zeiger auf GRECT mit Fensterausmassen */
/*----------------------------------------------------------------------------------------*/
void
size_window(short handle, GRECT *size)
{
	WINDOW *window;
	short buf[8];
	GRECT desk;
	
	window = search_struct(handle);
	if (window)
	{
		boolean	full_redraw;
		
		full_redraw = FALSE;
		
		if (window->wflags.smart_size == 0) /* gesamtes Fenster neuzeichnen? */
			full_redraw = TRUE;	

		wind_update(BEG_UPDATE); /* Bildschirm sperren */

		wind_get_grect(0, WF_WORKXYWH, &desk); /* Groesse des Hintergrunds erfragen */

		window->border = *size; /* Groesse von Fensteraussen und -Innenflaeche bestimmen */
		wind_calc_grect(WC_WORK, window->kind, &window->border, &window->workarea);
	
		if (window->wflags.snap_x) /* x-Koordinate snappen? */
		{
			WWORK.g_x += window->snap_dx - 2;	
			if (WWORK.g_x > (desk.g_x + desk.g_w - 32))
				WWORK.g_x = desk.g_x + desk.g_w - 32;
			WWORK.g_x -= WWORK.g_x % window->snap_dx;
		}
		
		if (window->wflags.snap_y) /* y-Koordinate snappen? */
		{
			WWORK.g_y += window->snap_dy - 1;
			if (WWORK.g_y > (desk.g_y + desk.g_h - 2))
				WWORK.g_y = desk.g_y + desk.g_h - 2;
			WWORK.g_y -= WWORK.g_y % window->snap_dy;
		}
		
		if (window->wflags.snap_width) /* Breite snappen? */
		{
			WWORK.g_w += window->snap_dw - 1;
			if (WWORK.g_w > desk.g_w)
				WWORK.g_h = desk.g_w;
			WWORK.g_w -= WWORK.g_w % window->snap_dw;
		}
		
		if (window->wflags.snap_height) /* Hoehe snappen? */
		{
			WWORK.g_h += window->snap_dh - 1;
			if (WWORK.g_h > desk.g_h)
				WWORK.g_h = desk.g_h;
			WWORK.g_h -= WWORK.g_h % window->snap_dh;
		}
		
		window->wflags.fuller = 0; /* das Fenster hat nicht mehr die volle Groesse */
		if (window->wflags.limit_wsize) 	/* Groesse begrenzt? */
		{
			short max_w, max_h;
			
			if (window->limit_w) /* sichtbare Breite zusaetzlich begrenzt? */
				max_w = window->limit_w;
			else
				max_w = (short) window->w;
				
			if (window->limit_h) /* sichtbare Hoehe zusaetzlich begrenzt? */
				max_h = window->limit_h;
			else
				max_h = (short) window->h;
			
			if (WWORK.g_w >= max_w)
			{
				window->border.g_w -= WWORK.g_w - max_w;
				WWORK.g_w = max_w;
			}
			if (WWORK.g_h >= max_h)
			{
				window->border.g_h -= WWORK.g_h - max_h;
				WWORK.g_h = max_h;
			}

			if (window->x + WWORK.g_w > window->w)
			{
				window->x = window->w - WWORK.g_w;
				full_redraw = TRUE; /* gesamtes Fenster neuzeichnen */
			}

			if (window->y + WWORK.g_h > window->h)
			{
				window->y = window->h - WWORK.g_h;
				full_redraw = TRUE; /* gesamtes Fenster neuzeichnen */
			}
		}

		wind_calc_grect(WC_BORDER, window->kind, &window->workarea, &window->border);
		wind_set_grect(handle, WF_CURRXYWH, &window->border);
		
		set_slsize(window); /* Slidergroesse setzen */
		set_slpos(window); /* Sliderposition setzen */

		if (full_redraw) /* gesamtes Fenster neuzeichnen? */
		{
			buf[0] = WM_REDRAW;		/* Nachrichtennummer */
			buf[1] = app_id;		/* Absender der Nachricht */
		 	buf[2] = 0;			/* Ueberlaenge in Bytes */
			buf[3] = handle;		/* Fensternummer */
			*(GRECT *)&buf[4] = *size;	/* Fensterkoordinaten */
			appl_write(app_id, 16, buf);	/* Redraw-Meldung an sich selber schicken */
		}
		
		wind_update(END_UPDATE); /* Bildschirm freigeben */
	}
}

/*----------------------------------------------------------------------------------------*/
/* Sliderposition setzen */
/* Funktionsergebnis:	- */
/* window:		Zeiger auf Fensterdaten */
/*----------------------------------------------------------------------------------------*/
void
set_slpos(WINDOW *window)
{
	if (window->kind & HSLIDE) /* horizontaler Slider vorhanden? */
	{
		short old_pos = window->hslide;

		if ((WWORK.g_w + window->x) >= window->w) /* Fenster breiter als der sichtbare Inhalt? */
			window->hslide = 1000;
		else
			window->hslide = (short)(window->x * 1000 / (window->w - window->workarea.g_w));

		if (old_pos != window->hslide) /* Sliderposition veraendert? */
			wind_set(window->handle, WF_HSLIDE, window->hslide, 0, 0, 0);
	}
	
	if (window->kind & VSLIDE) /* vertikaler Slider vorhanden? */
	{
		short old_pos = window->vslide;

		if ((WWORK.g_h + window->y) >= window->h) /* Fenster hoeher als der sichtbare Inhalt? */
			window->vslide = 1000;
		else
			window->vslide = (short)(window->y * 1000 / (window->h - window->workarea.g_h));

		if (old_pos != window->vslide) /* Sliderposition veraendert? */
			wind_set(window->handle, WF_VSLIDE, window->vslide, 0, 0, 0);
	}
}

/*----------------------------------------------------------------------------------------*/
/* Slidergroesse setzen */
/* Funktionsergebnis:	- */
/* window:		Zeiger auf Fensterdaten */
/*----------------------------------------------------------------------------------------*/
void
set_slsize(WINDOW *window)
{
	if (window->kind & HSLIDE) /* horizontaler Slider vorhanden? */
	{
		short old_size = window->hslsize;

		if ((WWORK.g_w + window->x) > window->w) /* Fenster breiter als der sichtbare Inhalt? */
			window->hslsize = (short) ((long) WWORK.g_w * 1000 / (WWORK.g_w + window->x));
		else
			window->hslsize = (short) ((long) window->workarea.g_w * 1000 / window->w);
		
		if (window->hslsize == 0)
			window->hslsize = -1;
		if (window->hslsize > 1000)
			window->hslsize = 1000;
			
		if (old_size != window->hslsize) /* Slidergroesse veraendert? */
			wind_set(window->handle, WF_HSLSIZE, window->hslsize, 0, 0, 0);
	}
	
	if (window->kind & VSLIDE) /* vertikaler Slider vorhanden? */
	{
		short old_size = window->vslsize;

		if ((WWORK.g_h + window->y) > window->h) /* Fenster hoeher als der sichtbare Inhalt? */
			window->vslsize = (short) ((long) WWORK.g_h * 1000 / (WWORK.g_h + window->y));
		else
			window->vslsize = (short) ((long) window->workarea.g_h * 1000 / window->h);
		
		if (window->vslsize == 0)
			window->vslsize = -1;
		if (window->vslsize > 1000)
			window->vslsize = 1000;
			
		if (old_size != window->vslsize) /* Slidergroesse veraendert? */
			wind_set(window->handle, WF_VSLSIZE, window->vslsize, 0, 0, 0);
	}
}

/*----------------------------------------------------------------------------------------*/
/* Fenster auf maximale Groesse */
/* Funktionsergebnis:	- */
/*	handle:		Fensterhandle */
/*	max_width:	0: ignorieren >0: maximale Breite, Position nicht veraendern */
/*	max_height:	0: ignorieren >0: maximale Hoehe, Position nicht veraendern */
/*----------------------------------------------------------------------------------------*/
void
full_window(short handle, short max_width, short max_height)
{
	WINDOW *window;
	GRECT area;
	GRECT desk;
	
	window = search_struct(handle);
	if (window)
	{
		if (window->wflags.fuller) /* bereits maximale Groesse? */
		{
			wind_get_grect(handle, WF_PREVXYWH, &area); 
			size_window(handle, &area);
			window->wflags.fuller = 0;
		}
		else
		{
			wind_get_grect(0, WF_WORKXYWH, &desk); /* Groesse des Desktops */
			wind_calc_grect(WC_WORK, window->kind, &desk, &desk);	/* Groesse der Fensterinnenflaeche */
			
			area = desk;

			area.g_x = window->workarea.g_x; /* die Position nicht veraendern */
			area.g_y = window->workarea.g_y;

			if (window->wflags.limit_wsize) /* Fenstergroesse begrenzt? */
			{
				long max_w;
				long max_h;

				if (window->limit_w) /* sichtbare Breite zusaetzlich begrenzt? */
					max_w = window->limit_w;
				else
					max_w = (short) window->w;
					
				if (window->limit_h) /* sichtbare Hoehe zusaetzlich begrenzt? */
					max_h = window->limit_h;
				else
					max_h = window->h;

				if (area.g_w > max_w)
					area.g_w = max_w;
				if (area.g_h > max_h)
					area.g_h = max_h;
			}

			if (max_width || max_height) /* Fenstergroesse nur so gross wie noetig machen? */
			{
				if (area.g_w > max_width)
					area.g_w = max_width;
				if (area.g_h > max_height)
					area.g_h = max_height;
			}
			
			if ((area.g_x + area.g_w) > (desk.g_x + desk.g_w))
				area.g_x = desk.g_x + desk.g_w - area.g_w;
			
			if (area.g_x < desk.g_x)
				area.g_x = desk.g_x;

			if ((area.g_y + area.g_h) > (desk.g_y + desk.g_h))
				area.g_y = desk.g_y + desk.g_h - area.g_h;
			
			if (area.g_y < desk.g_y)
				area.g_y = desk.g_y;
			
			wind_calc_grect(WC_BORDER, window->kind, &area, &area);
			size_window(handle, &area);
			window->wflags.fuller = 1;
		}
	}
}

/*----------------------------------------------------------------------------------------*/
/* Fenster ikonifizieren */
/* Funktionsergebnis:	- */
/*	handle:		Fensterhandle */
/*	size:		neue Fenstergroesse oder NULL */
/*----------------------------------------------------------------------------------------*/
void
iconify_window(short handle, GRECT *size)
{
	WINDOW *window;

	window = search_struct(handle);
	if (window)
	{
		GRECT area;
	
		window->saved_border = window->border;
	
		if ((size == 0L) || (size->g_w < 1) || (size->g_h < 1))
		{
			GRECT unknown = { -1, -1, -1, -1 };
	
			wind_close(window->handle);
			wind_set_grect(window->handle, WF_ICONIFY, &unknown);
			wind_get_grect(window->handle, WF_CURRXYWH, &area); 
			size = &area;
	
			wind_open_grect(window->handle, &area);
		}
		else
		{
			graf_shrinkbox_grect(size , &window->border);
			wind_set_grect(window->handle, WF_ICONIFY, size);
		}
	
		window->border = *size;
		wind_get_grect(window->handle, WF_WORKXYWH, &window->workarea);

		/* spezieller Fenstertitel? */
		if (window->iconified_name)
			wind_set_str(window->handle, WF_NAME, window->iconified_name);

		wind_set(window->handle, WF_BOTTOM, 0, 0, 0, 0); /* nach hinten legen */
		window->wflags.iconified = 1;
	}
}

/*----------------------------------------------------------------------------------------*/
/* Fenster auf normale Groesse bringen */
/* Funktionsergebnis:	- */
/*	handle:		Fensterhandle */
/*	size:		neue Fenstergroesse oder 0L */
/*----------------------------------------------------------------------------------------*/
void
uniconify_window(short handle, GRECT *size)
{
	WINDOW *window;

	window = search_struct(handle);
	if (window)
	{
		if ((size == 0L) || (size->g_w < 1) || (size->g_h < 1))
			size = &window->saved_border;

		graf_growbox_grect(&window->border, size);
		wind_set_grect(window->handle, WF_UNICONIFY, size);
		window->border = *size;
		wind_get_grect(window->handle, WF_WORKXYWH, &window->workarea);

		/* Fenstertitel? */
		if (window->name)
			wind_set_str(window->handle, WF_NAME, window->name);

		wind_set(window->handle, WF_TOP, 0, 0, 0, 0);
		window->wflags.iconified = 0;
	}
}

/*----------------------------------------------------------------------------------------*/
/* Das naechste Fenster in den Vordergrund bringen */
/* Funktionsergebnis:	- */
/*----------------------------------------------------------------------------------------*/
void
switch_window(void)
{
	short handle, buf[8];
	WINDOW *change;

	wind_get(0, WF_TOP, &handle, 0, 0, 0);

	/* liegt ein Fenster des eigenen Programms vorne? */
	change = search_struct(handle);
	if (change)
	{
		if (change->next)
			change = change->next;
		else
			change = window_list;

		buf[0] = WM_TOPPED;		/* Nachrichtennummer */
		buf[1] = app_id;		/* Absender der Nachricht */
		buf[2] = 0;			/* Ueberlaenge in Bytes */
		buf[3] = change->handle;	/* Fensternummer */
		appl_write(app_id, 16, buf);	/* Redraw-Meldung an sich selber schicken */
	}
}
