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

#ifndef _cops_h
#define _cops_h

#include "global.h"
#include "cpx.h"

#define ALPHAFILEVERSION 8

extern _WORD app_id;
extern _WORD aes_handle;
extern _WORD vdi_handle;
extern _WORD pwchar;
extern _WORD phchar;
extern _WORD pwbox;
extern _WORD phbox;

extern _WORD menu_id;
extern _WORD aes_flags;
extern _WORD aes_font;
extern _WORD aes_height;
extern _WORD quit;

extern GRECT desk_grect;

struct alphaheader
{
	long	magic;
	short	version;
	GRECT	mw;
	short	whslide;
	short	wvslide;
	short	booticon;	/* Hauptfenster beim Start oeffnen und ikonifizieren */
	short	clickact;	/* inaktive CPXe werden durch Doppelklick umbenannt und geoeffnet */
	short	term;
	short	after;
	short	count;
	char	cpx_path[128];
	short	sortmode;	/* 0: unsortiert, 1: nach Namen ordnen */
	short	reserved[15];
};
extern struct alphaheader settings;

extern struct cpxlist *cpxlist;
extern CPX_DESC *cpx_desc_list;

void open_cpx_context(CPX_DESC *cpx_desc);
void cpx_main_loop(void);

#endif /* _cops_h */
