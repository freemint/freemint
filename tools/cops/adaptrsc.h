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

#ifndef _adaptrsc_h
#define _adaptrsc_h

#include <gem.h>
#include "global.h"

#define	GAI_WDLG	0x0001	/* wdlg_xx()-Funktionen vorhanden */
#define	GAI_LBOX	0x0002	/* lbox_xx()-Funktionen vorhanden */
#define	GAI_FNTS	0x0004	/* fnts_xx()-Funktionen vorhanden */
#define	GAI_FSEL	0x0008	/* neue Dateiauswahl vorhanden */

#define	GAI_MAGIC	0x0100	/* MagiC-AES vorhanden */
#define	GAI_INFO	0x0200	/* appl_getinfo() vorhanden */
#define	GAI_3D		0x0400	/* 3D-Look vorhanden */
#define	GAI_CICN	0x0800	/* Color-Icons vorhanden */
#define	GAI_APTERM	0x1000	/* AP_TERM wird unterst�tzt */
#define	GAI_GSHORTCUT	0x2000	/* Objekttyp G_SHORTCUT wird unterst�tzt */

short get_aes_info(short *font_id, short *font_height, short *hor_3d, short *ver_3d);
void adapt3d_rsrc(OBJECT *objs, unsigned short no_objs, short hor_3d, short ver_3d);
void no3d_rsrc(OBJECT *objs, unsigned short no_objs, short ftext_to_fboxtext);
void substitute_objects(OBJECT *objs, unsigned short no_objs, short aes_flags, OBJECT *rslct, OBJECT *rdeslct);
void substitute_free(void);
char *is_userdef_title(OBJECT *obj);

#endif /* _adaptrsc_h */
