/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *
 * A multitasking AES replacement for MiNT
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

/*
 * Definitions for the standard GEM application data structures
 */

#ifndef _xa_aes_h
#define _xa_aes_h

#include "aes_defs.h"
#include "vdi_defs.h"


/* To ensure that both XaAES and clients talk about the same things :-) */
#define SL_RANGE	1000

/* Process id of the screen manager - this is redundant
 * and will probably require a kludge to redirect it to the
 * 'real' desktop (check the AVSERVER variable?) */
#define SCR_MGR		0x0001

#define AP_MSG		0

/* Object library definitions */
#define ROOT		0

/* System foreground and background rules */
#define SYS_FG		0x1100	/* but transparent */
#define WTS_FG		0x11a1	/* Window title selected; using pattern 2 & replace mode text */
#define WTN_FG		0x1100	/* Window title normal   */


/* AES 4.1 extended te_font types */
enum te_defs
{
	TE_GDOS_PROP,		/* Use a proportional SPEEDOGDOS font */
	TE_GDOS_MONO,		/* Use a monospaced SPEEDOGDOS font */
	TE_GDOS_BITM,		/* Use a GDOS bitmap font */
	TE_STANDARD = 3,	/* Use the standard system font (10 point) */
	TE_SMALL = 5		/* Use the small system font (8 point) */
};


/* Graphics library definitions */
#define VERTICAL	0
#define HORIZONTAL	1

/* Scrap library definitions */
#define SCRAP_CSV	0x0001
#define SCRAP_TXT	0x0002
#define SCRAP_GEM	0x0004
#define SCRAP_IMG	0x0008
#define SCRAP_DCA	0x0010
#define SCRAP_USR	0x8000

/* Window library definitions */

typedef unsigned long XA_WIND_ATTR;

#define USE_INTIN5 0x8000
enum wind_xattr
{
	XMENU = 0x0001,		/*  A window with a menu bar */
	XTOOL = 0x0002,		/*      "    with a tool bar */
	XMAX  = 0x0004		/*  Use the create window max rextangle when sizing */
};

/* Extended XaAES Attributes */

/* Visible (settable) to the outside world (WIND_XATTR << 16) */
#define XaMENU		((long) XMENU << 16) /* Have a menu bar */
#define TOOLBAR		((long) XTOOL << 16) /* Have a tool bar */
#define USE_MAX		((long) XMAX  << 16) /* Use max */

/* Internal */
#define XaPOP		0x01000000L /* Special kind of window. */
#define XaTOP		0x02000000L /* Window stays permanently on top (mostly popups) */
#define NO_TOPPED	0x04000000L /* Send MU_BUTTON for a click on workarea, not WM_TOPPED */
#define NO_LIST		0x08000000L /* Dont put this window in a list */
#define STORE_BACK	0x10000000L /* Store the background of a window & don't generate redraw
				     * messages to windows below. HR 250602: Now only used for form_alerts
				     * when the application has locked the screen.
				     * Older AES's restore form_alerts and many apps count on that. */
#define HIDE		0x20000000L /* Have a hider widget */
#define HIDER		HIDE

/* visible widgets */
#define V_WIDG  (CLOSE|NAME|MOVE|ICONIFY|FULL|XaMENU|HIDE|INFO|SIZE|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE)

#define DESK	0

/* redifined AES parameter block */
typedef struct
{
	short *contrl;
	struct aes_global *globl;
	short *intin;
	short *intout;
	void **addrin;
	void **addrout;
} AESPB;


/*------------------------------------------------------------ */
/* SPECIAL N.Aes/XaAES stuff */

/* appl_control */
enum apc_code
{
	APC_HIDE = 10,
	APC_SHOW,
	APC_TOP,
	APC_HIDENOT,
	APC_INFO,
	/* APC_MENU, This is a very bad idea */
	APC_HIGH
};

/* appl_search */
enum client_type				/* compendium */
{
	APP_FIRST,
	APP_NEXT,
	APP_DESK,
	APP_TASKINFO    = 0x100,	/* XaAES extension for taskbar applications. */
	APP_SYSTEM      = 0x01,
	APP_APPLICATION = 0x02,
	APP_ACCESSORY   = 0x04,
	APP_SHELL       = 0x08,
	APP_HIDDEN      = 0x100,	/* XaAES extension for taskbar applications. */
	APP_FOCUS       = 0x200		/* idem  */
};

/*------------------------------------------------------------ */
/* SPECIAL WDIALOG stuff */

/* HR: This is the worst piece of interface design I have ever seen.
 *     How on earth could Andreas Kromke have accepted this clumsy work
 *     of the Behne brothers. They really should have known better.
 *     Given the problems that progdefs have caused in multitasking OS's.
 *
 *     Calling back from the OS into user code is something known to be a bad thing
 *     for half a century now.
 *     
 *     This whole lot of crap has succeeded to spoil my good humour for much too long.
 *     It would have been so much easier to use a few message types in stead.
 */

typedef void *DIALOG;
typedef short _cdecl HNDL_OBJ(void *dialog, EVNT *events, short obj, short clicks, void *data);
typedef HNDL_OBJ *HANDL_OBJ;

typedef enum
{
	/* Function numbers for <obj> with handle_exit(...) */
	HNDL_UNTP =-11,		/* Dialog-window is not active */
	HNDL_TOPW,		/* Dialog-window has been topped */
	HNDL_MOVE,		/* Dialog was moved */
	HNDL_EDCH,		/* Edit-field was changed */
	HNDL_EDDN,		/* Character was entered in edit-field */
	HNDL_EDIT,		/* Test characters for an edit-field */
	HNDL_OPEN,		/* End of dialog initialisation (second  call at end of wdlg_init) */
	HNDL_CLSD =-3,		/* Dialog window was closed */
	HNDL_MESG,		/* Initialise dialog */
	HNDL_INIT,		/* Initialise dialog */

	/* Definitions for <flags> */
	WDLG_BKGD = 1		/* Permit background operation */
} WDLG_OPT;

/****** Listbox definitions ***********************************************/
struct lbox_item
{
	struct lbox_item *next;	/* Pointer to the next entry in the list */
	short selected;		/* Specifies if the object is selected */

	short data1;		/* Data for the program... */
	void *data2;
	void *data3;

};
typedef struct lbox_item LBOX_ITEM;

typedef void *LIST_BOX;

typedef void (_cdecl *SLCT_ITEM)(LIST_BOX *box, OBJECT *tree, struct lbox_item *item,
				 void *user_data, short obj_index, short last_state);

typedef short (_cdecl *SET_ITEM)(LIST_BOX *box, OBJECT *tree, struct lbox_item *item,
				 short obj_index, void *user_data, RECT *rect, short first);

typedef enum
{
	LBOX_VERT     =   1,	/* Listbox with vertical slider */
	LBOX_AUTO     =   2,	/* Auto-scrolling */
	LBOX_AUTOSLCT =   4,	/* Automatic display during auto-scrolling */
	LBOX_REAL     =   8,	/* Real-time slider */
	LBOX_SNGL     =  16,	/* Only a selectable entry */
	LBOX_SHFT     =  32,	/* Multi-selection with Shift */
	LBOX_TOGGLE   =  64,	/* Toggle status of an entry at selection */
	LBOX_2SLDRS   = 128	/* Listbox has a horiz. and a vertical slider */
} LBOX_FLG;


/*------------------------------------------------------------ */
/* SPECIAL XaAES stuff */

/* AES Command types */
typedef enum
{
	AESCMD_STD = 200,	/* Standard blocking AES call (process blocks for reply) */
	AESCMD_TEST,		/* The 'secret' test for installed AES call */
	AESCMD_NOREPLY,		/* Process isn't interested in reply so don't give one */
	AESCMD_NOBLOCK,		/* Don't block for reply, but place in process reply pipe */
				/* - Process is handling its own reply pipe */
	VDICMD_STD = 115	/* The trap was a VDI trap, so call the old vector instead */
} AES_CMD;

/* Extra mouse forms */
typedef enum
{
	XACRS_BUBBLE_DISC = 270,/* The Data Uncertain logo */
	XACRS_RESIZER,		/* The 'resize window' cursors */
	XACRS_SE_SIZER = XACRS_RESIZER,
	XACRS_NE_SIZER,
	XACRS_MOVER,		/* The 'move window' cursor */
	XACRS_VERTSIZER,	/* The 'resize vertically' cursor */
	XACRS_HORSIZER		/* The 'resize horizontally' cursor */
} XA_MOUSE;

#define	CGd		0x01	/* Numeric digit */
#define CGa		0x02	/* Alpha */
#define	CGs		0x04	/* Whitespace */
#define	CGu		0x08	/* Upper case */
#define CGp 		0x10	/* Punctuation character */
#define	CGdt		0x20	/* Dot */
#define	CGw		0x40	/* Wild card */
#define CGxp		0x80	/* Extended punctuation */

#ifndef THICKENED
#define THICKENED	0x01
#define SHADED		0x02
#define SKEWED		0x04
#define UNDERLINED	0x08
#define OUTLINE		0x10
#define SHADOW		0x20
#endif
#ifndef BOLD
#define BOLD		THICKENED
#define FAINT		SHADED
#define ITALIC		SKEWED
#endif

typedef enum
{
	XA_M_CNF = 200,
	XA_M_SCL,
	XA_M_OPT,
	XA_M_GETSYM,
	XA_M_DESK,
	XA_M_EXEC = 250,
	XA_M_OK = 300
} XA_MSGS;

#endif /* _xa_aes_h */
