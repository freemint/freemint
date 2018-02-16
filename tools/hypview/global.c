/*
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gem.h>
#include "diallib.h"
#include "defs.h"

/*	Dateien und Pfade	*/
char path_list[512] = "*:\\GEMSYS\\GUIDES\\;*:\\GUIDES\\";
char default_file[DL_PATHMAX];
char catalog_file[DL_PATHMAX];
char file_extensions[DL_PATHMAX]="*.HYP\0*.*\0";

/*	System spezifische Einstellungen	*/
char dir_separator = '\\';

/*	Format spezifische Einstellungen	*/
short binary_columns = 76;
short ascii_tab_size = 3;
short ascii_break_len = 127;

/*	aktuelles Fenster	*/
WINDOW_DATA *Win;
GRECT Area;

/*	Start-Masse des Fensters: x und y in Pixel	*/
short win_x = 0,win_y = 0,win_w = 0,win_h = 0;
short adjust_winsize = FALSE;
short intelligent_fuller = TRUE;

/*	ausgewaehlter Zeichensatz	*/
short sel_font_id, sel_font_pt;
short font_id = 1, font_pt = 13;
short xfont_id = 274,xfont_pt = 13;
short font_h, font_w, font_cw, font_ch;

/*	Darstellung	*/
short background_col= G_WHITE;
short text_col = G_BLACK;
short link_col = G_BLUE;
short link_effect = TF_THICKENED|TF_UNDERLINED;
short transparent_pics = TRUE;

/*	Fuer AV_STARTPROG	*/
char *av_parameter = NULL;

/*	AV_START oeffnet ein neues Fenster	*/
short va_start_newwin = 0;

/*	MiNT "fixes"	*/
char debug_file[DL_NAMEMAX];

short check_time = FALSE;

/*	Skin-Management	*/
char skin_path[DL_PATHMAX] = "Skins";
GlobalArray skin_global;

/*	Clipboard Inhalt in neuem Fenster oeffnen	*/
short clipbrd_new_window = FALSE;

/*	Global window cycling using AVSERVER	*/
short av_window_cycle = FALSE;

/* Marken-Management */
char marken_path[DL_PATHMAX] = "";
short marken_save_ask = TRUE;

/* REF Datenbank-Datei */
char all_ref[DL_PATHMAX] = "";

/* HypFind */
char hypfind_path[DL_PATHMAX] = "";

short refonly = 0;
