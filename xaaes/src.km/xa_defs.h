/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
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

#ifndef _xa_defs_h
#define _xa_defs_h


/*----------------------------------------------------------------- */
/* COMPILE TIME SWITCHES */
/* - define as 1 to turn on, 0 to turn off */
/*----------------------------------------------------------------- */

#define DISPLAY_LOGO_IN_TITLE	0	/* Display the XaAES logo in the title bar of each window */

#define POINT_TO_TYPE		0	/* if defined focus can be dynamically changed in the cfg (see parde*/

#define ALT_CTRL_APP_OPS	1	/* Some ALT+CTRL+key combo's will perform functions relating to
                       			 * the XaAES system */

#define PRESERVE_DIALOG_BGD	0	/* Preserve the background of dialogs */

#define FILESELECTOR		1	/* Build a XaAES with fileselector */

#define NAES3D			1	/* ??? */


/*----------------------------------------------------------------- */
/* Maximum space for 'nice name' for app's (menu_register) */
#define NICE_NAME		32

#define MAX_FTEXTLEN		256

/* CONFIGURABLE PARAMETERS */
#define NUM_CB			32	/* table of cancel button texts */
#define CB_L			16
#define CASCADE			16	/* Max depth of cascading menus */

/* Maximum number of windows that can be created concurrently */
#define MAX_WINDOWS 		4096	/* should this be enough? */
#define LBITS			32	/* no of bits in a long */

#define MAX_IN_PLACE_TRNFM	128	/* For larger icons a temporary buffer is
					 * allocated to speed up vr_trnfm
					 * 128 = 16*16 icon in 4 planes */

#define DU_RSX_CONV 		8	/* Resource char to pixel coord conversion constants. */
#define DU_RSY_CONV 		16	/* If you are designing your resources in ST-Low res,
					 * change DU_RSY_CONV to 8 */
#define STANDARD_FONT_POINT	10	/* Size of the 'standard' AES font (for window titles, etc) */
#define MEDIUM_FONT_POINT	9	/* The same, but for low resolution screens */
#define SMALL_FONT_POINT	8	/* Size of the 'small' AES font. */
#define STANDARD_AES_FONTID	1	/* Standard GDOS fontid for windows & stuff (set to 1 for no GDOS) */
#define TINY_FONT		6	/* pixelsize of the 6x6 font */

#define ICON_W			0x0010	/* (ex widgets.c) */
#define ICON_H			0x0010

#define GRAF_STEPS		10	/* Number of intermediate steps in a graf_growbox(), etc */

#define PUSH3D_DISTANCE		1	/* Distance text on selected (pushed in) 3D objects will move */
#define SHADOW_OFFSET		2	/* Offset used for shadowing objects */

#define ICONIFIED_W		80	/* Size of an iconified window */
#define ICONIFIED_H		80

#define DOUBLE_CLICK_TIME	50	/* Default double click time in milliseconds (0-200) */

#define RT_SCROLL_STEP		10	/* Step when doing a real-time scroll (0-1000) */

#define MENU_H	(screen.c_max_h + 2)	/* Standard hight od a menu bar */
#define MG	1 /*2*/			/* standard frame size for windows */


/*----------------------------------------------------------------- */
/* Return codes in the clients reply pipe */
#define XA_OK			0L	/* Ok, op-code done */


/*----------------------------------------------------------------- */
/* HR 090701: codes follow same pattern as above */
/* Return codes from op-code service functions */
#define XAC_BLOCK		0L	/* Block the client indefinitely */
#define XAC_DONE		1L	/* Function completed - release client immediately */
#define XAC_TIMER		3L	/* Unclumsify the timer value passing. */


/*----------------------------------------------------------------- */
/* MiNT Memory Allocation Stuff */
#ifndef MX_STRAM
#define MX_STRAM 	0
#define MX_TTRAM	1
#define MX_PREFSTRAM	2
#define MX_PREFTTRAM 	3
#define MX_HEADER	(1 << 3)
#define MX_PRIVATE	((1 << 3) | (1 << 4))
#define MX_GLOBAL	((1 << 3) | (2 << 4))
#define MX_SUPERVISOR	((1 << 3) | (3 << 4))
#define MX_READABLE	((1 << 3) | (4 << 4))
#endif

#endif /* _xa_defs_h */
