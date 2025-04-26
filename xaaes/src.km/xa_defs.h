/*
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


/* absolute value for any type of number */
#ifndef abs
#define	abs(x)		((x)<0?(-(x)):(x))
#endif


/*----------------------------------------------------------------- */
/* COMPILE TIME SWITCHES */
/* - define as 1 to turn on, 0 to turn off */
/*----------------------------------------------------------------- */

/* define 1 if CTRL-ALT-keys shall shutdown system or term apps */
#ifndef HOTKEYQUIT
#define HOTKEYQUIT	1
#endif

/* define 1 you want an alert before exiting XaAES from taskmanager */
#ifndef TM_ASK_BEFORE_SHUTDOWN
#define TM_ASK_BEFORE_SHUTDOWN 0
#endif

/* define 1 if credits shall be displayed in 'about' and at bootup, 0 for only freemint-url */
#define DISPCREDITS	0

/* define 1 if GPL shall be quoted in 'about' 0 if only mentioned */
#define LONG_LICENSE	0

/* define 1 if unused code shall be included */
#define INCLUDE_UNUSED	0

/* define 1 if xa_help.txt shall be displayed in about-window */
#define HELPINABOUT	1

/* define 1 if alerts in syslog shall have a timestamp */
#define ALERTTIME	1

/* define 1 if CPU-caches should be saved/restored for opnwk/clswk */
#define SAVE_CACHE_WK 1

#define DISPLAY_LOGO_IN_TITLE	0	/* Display the XaAES logo in the title bar of each window */

#define POINT_TO_TYPE		0	/* if defined focus can be dynamically changed in the cfg (see parde*/

#define ALT_CTRL_APP_OPS	1	/* Some ALT+CTRL+key combo's will perform functions relating to
                       			 * the XaAES system */

/* set !0 to skip texel-demo-wait-loop */
#define SKIP_TEXEL_INTRO 0

/* set default-widgets */
#ifndef WIDGNAME
#define WIDGNAME "xaaeswdg.rsc"
#endif
/* if 0 don't produce wheel-events from keys */
#ifndef EIFFEL_SUPPORT
#define EIFFEL_SUPPORT 1
#endif

/* enable gradients */
#ifndef WITH_GRADIENTS
#ifdef ST_ONLY
#define WITH_GRADIENTS 0
#else
#define WITH_GRADIENTS 1
#endif
#endif
/* enable background-image */
#ifndef WITH_BKG_IMG
#ifdef ST_ONLY
#define WITH_BKG_IMG 0
#else
#define WITH_BKG_IMG 1
#endif
#endif
#define BKGIMG_EXT "mfd"

/* enable builtin bubble-gem */
#ifndef WITH_BBL_HELP
#define WITH_BBL_HELP 1
#endif

#define PRESERVE_DIALOG_BGD	0	/* Preserve the background of dialogs */

#define FILESELECTOR		1	/* Build a XaAES with fileselector */
#if FILESELECTOR
#define FS_PATLEN	48	/* max. len of each pattern */
#define FS_NPATTERNS	16	/* # of patterns */
#endif


#ifndef WDIALOG_WDLG
#define WDIALOG_WDLG 1	/* wdlg_xx() extensions */
#endif
#ifndef WDIALOG_LBOX
#define WDIALOG_LBOX 1	/* lbox_xx() extensions */
#endif
#ifndef WDIALOG_FNTS
#define WDIALOG_FNTS 1	/* fnts_xx() extensions */
#endif
#ifndef WDIALOG_FSLX
#define WDIALOG_FSLX 0	/* fslx_xx() extensions */
#endif
#ifndef WDIALOG_PDLG
#define WDIALOG_PDLG 1	/* pdlg_xx() extensions */
#endif
#ifndef WDIALOG_EDIT
#define WDIALOG_EDIT 0	/* edit_xx() extensions */
#endif

/* check stack-alignment for all xaaes-threads and display the result in the
 * system-window
 */
#if XAAES_RELEASE
#define CHECK_STACK	0
#else
#define CHECK_STACK	1
#endif

/* do appl_init if an app does AES-calls without precedent appl_init */
#ifndef INSERT_APPL_INIT
#define INSERT_APPL_INIT 1
#endif
/* some debug-code to produce a bus-error */
#ifndef TST_BE
#define TST_BE 0
#endif

#if __GNUC__ > 2
/* set 1 if you wish more inlinig
 * increases size but maybe faster
 */
#define AGGRESSIVE_INLINING	0
#else
/* always 0 on gcc2 and others */
#define AGGRESSIVE_INLINING	0
#endif

/*----------------------------------------------------------------- */

#define FREEMINT_URL	"https://freemint.github.io/"
/* Maximum space for 'nice name' for app's (menu_register) */
#define NICE_NAME		64

#ifdef PATH_MAX
#define FILENAME_MAX PATH_MAX
#else
#define FILENAME_MAX 255
#endif

/* CONFIGURABLE PARAMETERS */
#define NUM_CB			32	/* table of cancel button texts */
#define CB_L			16
#define CASCADE			16	/* Max depth of cascading menus */

#define MAX_KEYBOARDS 4     /* maximum number of keytables to cycle */

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

#define ICONIFIED_W		72	/* Size of an iconified window */
#define ICONIFIED_H		72

#define DOUBLE_CLICK_TIME	(250 / 5)	/* Default double click time in timer ticks (0-200) */
#define MOUSE_PACKET_TIMEGAP	 3	/* Amount of time that have to pass between two packets received from the VDI */
#define RT_SCROLL_STEP		10	/* Step when doing a real-time scroll (0-1000) */

#define MENU_H	(screen.c_max_h + 2)	/* Standard hight of a menu bar */
#define MG	1 /*2*/			/* standard frame size for windows */


#define TABSZ 4	/* tab-size for expanding \t in list-text */

/* special indices for shortcut-replacement-algorithm */
#define HK_SHIFT  256        /* add for SHIFT */
#define HK_FREE   511        /* last index: unused/invalid */

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

/* MiNT-semaphores */
#define XA_SEM 0x58414553L	/*"XAES"*/
#define XA_SCP 0x5F534350L	/*"_SCP"*/
#define SEMCREATE	0
#define SEMDESTROY	1
#define SEMGET	2
#define SEMRELEASE	3

/*
* kernel-code:
*
*  MODE  ACTION
*    0 Create and get a semaphore with the given ID.
*    1 Destroy.
*    2 Get (blocks until it's available or destroyed, or timeout).
*    3 Release.
*
* RETURNS
*
*  CODE  MEANING
*    0 OK.  Created/obtained/released/destroyed, depending on mode.
*  ERROR You asked for a semaphore that you already own.
*  EBADARG That semaphore doesn't exist (modes 1, 2, 3, 4),
*    or out of slots for new semaphores (0).
*  EACCES  That semaphore exists already, so you can't create it (mode 0),
*    or the semaphore is busy (returned from mode 3 if you lose),
*    or you don't own it (modes 1 and 4).
*/

#endif /* _xa_defs_h */
