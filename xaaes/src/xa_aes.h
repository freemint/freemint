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

#include "mt_gem.h"
#include "mt_gemx.h"

/* TODO: Usage of RECT should be eliminated somehow
         and replaced by the gemlib's GRECT.
         It is quite a lot of work however. :( */
typedef struct { short x, y, w, h; } RECT;

/*
 * XaAES/oAESis Extended Shell Write structure 
 * Extra fields for UID/GID setting of spawned clients.
 * Different naming.
 */
struct xshelw
{
	char *newcmd;
	long psetlimit;
	long prenice;
	char *defdir;
	char *env;
	short uid;			/* New child's UID */
	short gid;			/* New child's GID */
};
typedef struct xshelw XSHELW;

/* The shel_write extensions (used in xa_fork_exec()) */
#define SW_UID 	0x1000	/* Set user id of launched child */
#define	SW_GID	0x2000	/* Set group id of launched child */

/* extended OBJECT types, XaAES only */
#define G_SLIST		39
#define G_UNKNOWN	40

/* To ensure that both XaAES and clients talk about the same things :-) */
#define SL_RANGE	1000


/* AES 4.1 extended te_font types */
enum te_defs
{
	TE_GDOS_PROP,		/* Use a proportional SPEEDOGDOS font */
	TE_GDOS_MONO,		/* Use a monospaced SPEEDOGDOS font */
	TE_GDOS_BITM,		/* Use a GDOS bitmap font */
	TE_STANDARD = 3,	/* Use the standard system font (10 point) */
	TE_SMALL = 5		/* Use the small system font (8 point) */
};

/* Window library definitions */

typedef unsigned long XA_WIND_ATTR;

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
#define V_WIDG  (CLOSER|NAME|MOVER|ICONIFIER|FULLER|XaMENU|HIDE|INFO|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE)

/* At last give in to the fact that it is a struct, NOT an array */
struct aes_global
{
	short	version;
	short	count;
	short	id;
	union private *pprivate;	/*  3,  4 */
	void	*ptree;			/*  5,  6 */
	void	*rshdr;			/*  7,  8 */
	short	lmem;			/*  9     */
	short	nplanes;		/* 10     */
	short	res1, client_end;	/* 11, 12 */
	short	c_max_h, bvhard;	/* 13, 14 */
};

/* appl_search */
enum client_type
{
	APP_TASKINFO    = 0x100,	/* XaAES extension for taskbar applications. */
	APP_HIDDEN      = 0x100,	/* XaAES extension for taskbar applications. */
	APP_FOCUS       = 0x200		/* idem  */
};


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
	XACRS_HORSIZER,		/* The 'resize horizontally' cursor */
	XACRS_POINTSLIDE	/* ozk: The 'two-arrows pointing inwards' cursor to pinpoint slider position */
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
