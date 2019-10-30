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

/*
 * Definitions for the standard GEM application data structures
 */

#ifndef _xa_aes_h
#define _xa_aes_h

#include "mt_gem.h"
#include "mt_gemx.h"

/*
 * constants that might be missing from current gemlib headers.
 * Should be removed, once they are known to be available.
 */
#ifndef FNTS_DISPLAY
#define FNTS_DISPLAY	2
#endif
#ifndef PDLG_OUTFILES
#define PDLG_OUTFILES		5
#endif
#ifndef PLANE_MASK
#define PLANE_MASK	(PLANE_BLACK|PLANE_YELLOW|PLANE_MAGENTA|PLANE_CYAN)
#endif
#ifndef APC_SYSTEM
#define APC_SYSTEM			 2
#endif
#ifndef AES_FUNCTIONS
#define AES_FUNCTIONS			98
#endif
#ifndef AES_AOPTS
#define AES_AOPTS		99
#endif
#ifndef WO0_WCOWORK
#define WO0_WCOWORK		0x0020
#endif

/* appl_getinfo(AES_VERSION) return values */
#ifndef AES_ARCH_M68000
#define AES_DEVSTATUS_ALPHA	0
#define AES_DEVSTATUS_BETA	1
#define AES_DEVSTATUS_RELEASE	2
#define AES_FDEVSTATUS_STABLE	0x100

#define AES_ARCH_M68000	    0
#define AES_ARCH_M68010     1
#define AES_ARCH_M68020     2
#define AES_ARCH_M68030    	3
#define AES_ARCH_M68040    	4
#define AES_ARCH_M68060  	5
#define AES_ARCH_M6802060	6
#define AES_ARCH_COLDFILRE	7
#endif

/* appl_getinfo(AES_FUNCTIONS) return values */
#ifndef AGI_WFORM
#define AGI_WFORM			1
#define AGI_AOPTS			2
#endif
#ifndef AGI_OBJCDATA
#define AGI_OBJCDATA		4
#endif

/* appl_options(AES_AOPTS) return values and appl_options() settings */
#ifndef AO0_WF_SLIDE
#define AO0_WF_SLIDE		1
#define AO0_OBJC_EDIT		2
#endif

#ifndef APP_AESSYS
#define APP_AESSYS			0x10
#endif
#ifndef APP_AESTHREAD
#define APP_AESTHREAD		0x20
#endif

#ifndef WHEEL_SLIDER
#define WHEEL_MESAG			0	/**< AES will send #WM_WHEEL messages */
#define WHEEL_ARROWED		1   /**< AES will send #WM_ARROWED messages */
#define WHEEL_SLIDER		2   /**< AES will convert mouse wheel events to slider events */
#endif

#ifndef WA_RTSCAN
#define WA_UPSCAN		8	/* XaAES only */
#define WA_DNSCAN		9	/* XaAES only */
#define WA_LFSCAN		10	/* XaAES only */
#define WA_RTSCAN		11	/* XaAES only */
#endif

#ifndef X_MSET_SHAPE
#define X_MRESET      1000	/* geneva */
#define X_MGET        1001	/* geneva */
#define X_MSET_SHAPE  1100	/* geneva */
#endif

#ifndef OF_FL3DMASK
#define OF_FL3DMASK		(OF_FL3DIND|OF_FL3DBAK|OF_FL3DACT)
#endif

#ifndef ED_CRSROFF
/* XaAES extensions to objc_edit() */
#define ED_DISABLE		5
#define ED_ENABLE		6
#define ED_CRSRON		7
#define ED_CRSROFF		8

#define ED_MARK			9
#define ED_STRING		10
#define ED_SETPTEXT		11
#define ED_SETPTMPLT	12
#define ED_SETPVALID	13
#define ED_GETPTEXT		14
#define ED_GETPTMPLT	15
#define ED_GETPVALID	16
#endif

#ifndef ED_REDRAW
#define ED_REDRAW		200		/* XaAES only */
#define ED_XINIT		201
#define ED_XCHAR		202
#define ED_XEND			203		/* Redraw cursor, XaAES only */
#define ED_CHGTEXTPTR	204
#define ED_CHGTMPLPTR	205
#endif


#ifndef __RSXHDR
#define __RSXHDR
typedef struct rsxhdr
{
	unsigned short	rsh_vrsn;     /* should be 3                                 */
	unsigned short	rsh_extvrsn;  /* not used, initialised to 'IN' for Interface */

	unsigned long	rsh_object;
	unsigned long	rsh_tedinfo;
	unsigned long	rsh_iconblk;  /* list of ICONBLKS                            */
	unsigned long	rsh_bitblk;
	unsigned long	rsh_frstr;
	unsigned long	rsh_string;
	unsigned long	rsh_imdata;   /* image data                                  */
	unsigned long	rsh_frimg;
	unsigned long	rsh_trindex;

	unsigned long	rsh_nobs;     /* counts of various structs                   */
	unsigned long	rsh_ntree;
	unsigned long	rsh_nted;
	unsigned long	rsh_nib;
	unsigned long	rsh_nbb;
	unsigned long	rsh_nstring;
	unsigned long	rsh_nimages;
	unsigned long	rsh_rssize;   /* total bytes in resource                     */
} RSXHDR;
#endif


/*
 * end of missing constants
 */

/* conflicts with mint/mem.h */
#undef ROUND

#define MAX_WHLMODE	WHEEL_SLIDER
#define DEF_WHLMODE	WHEEL_ARROWED


/* bubble-gem */
typedef struct
{
	long   magic;   /* 'BGEM'                                   */
	long   size;    /* Größe dieser Struktur, derzeit 18        */
	short  release; /* derzeit 7, nie kleiner als 5             */
	short  active;  /* <>0, wenn gerade eine Hilfe angezeigt wird;  0  sonst                               */
	struct mouse_form *mhelp;   /* Zeiger auf Hilfe-Mausform                */
	short  dtimer;  /* Dämon-Timer; Default 200ms; ab Release 6 */
} BGEM;

typedef struct
{
#define BGC_FONTCHANGED 0x0001
#define BGC_NOWINSTYLE  0x0002
#define BGC_SENDKEY     0x0004
#define BGC_DEMONACTIVE 0x0008
#define BGC_TOPONLY     0x0010
	short flags;
	short display_time;	/* ms */
} BHLP;

#define C_BGEM 	0x4247454dL

#define BGS7_USRHIDE 0x0001
#define BGS7_USRHIDE2 0x0002	/* XaAES */
#define BGS7_MOUSE   0x0004
#define BGS7_DISPCL  0x0080	/* display client: XaAES-special */

#define BUBBLEGEM_REQUEST  0xBABA
#define BUBBLEGEM_SHOW     0xBABB
#define BUBBLEGEM_ACK      0xBABC
#define BUBBLEGEM_ASKFONT  0xBABD
#define BUBBLEGEM_FONT     0xBABE
#define BUBBLEGEM_HIDE     0xBABF

typedef struct
{
    unsigned int    version;
    unsigned int    date;
    unsigned int    time;
    unsigned int    flags;
    unsigned long   unused_1;
    unsigned long   unused_2;
} N_AESINFO;

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
	const char *newcmd;
	long psetlimit;
	long prenice;
	const char *defdir;
	char *env;
	short uid;			/* New child's UID */
	short gid;			/* New child's GID */
};


/* The shel_write extensions (used in xa_fork_exec()) */
#define SW_UID 	0x1000	/* Set user id of launched child */
#define	SW_GID	0x2000	/* Set group id of launched child */

/* extended OBJECT types, XaAES only */
#define G_SLIST		39
#define G_EXTBOX	40
#define G_OBLINK	41
#define G_UNKNOWN	42

/* To ensure that both XaAES and clients talk about the same things :-) */
#define SL_RANGE	1000


#define N_INTIN 1
#define N_INTOUT 2
#define N_ADDRIN 3
#define N_ADDROUT 4

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
	XMENU		= 0x0001,		/*  A window with a menu bar */
	XTOOL		= 0x0002,		/*      "    with a tool bar */
	XMAX 		= 0x0004,		/*  Use the create window max rextangle when sizing */
	XWCONTEXT	= 0x0008,
	XWAPPICN	= 0x0010,
	XUPARROW1	= 0x0020,
	XLFARROW1	= 0x0040,
};

/* Extended XaAES Attributes */

/* Visible (settable) to the outside world (WIND_XATTR << 16) */
#define XaMENU		((long) XMENU << 16) /* Have a menu bar */
#define TOOLBAR		((long) XTOOL << 16) /* Have a tool bar */
#define USE_MAX		((long) XMAX  << 16) /* Use max */
#define WCONTEXT	((long) XWCONTEXT << 16)
#define WAPPICN		((long) XWAPPICN << 16)
#define UPARROW1	((long) XUPARROW1 << 16)
#define LFARROW1	((long) XLFARROW1 << 16)

/* Internal */
// #define XaPOP		0x01000000L /* Special kind of window. */
// #define XaTOP		0x02000000L /* Window stays permanently on top (mostly popups) */
#define NO_TOPPED	0x04000000L /* Send MU_BUTTON for a click on workarea, not WM_TOPPED */
// #define NO_LIST		0x08000000L /* Dont put this window in a list */
#define STORE_BACK	0x10000000L /* Store the background of a window & don't generate redraw
				     * messages to windows below. HR 250602: Now only used for form_alerts
				     * when the application has locked the screen.
				     * Older AES's restore form_alerts and many apps count on that. */
#define HIDE		0x20000000L /* Have a hider widget */
#define HIDER		HIDE

/* visible widgets */
#define VS_WIDGETS (UPARROW|DNARROW|VSLIDE|UPARROW1)
#define HS_WIDGETS (RTARROW|LFARROW|HSLIDE|LFARROW1)

#define V_WIDG  (CLOSER|NAME|MOVER|ICONIFIER|FULLER|XaMENU|HIDE|INFO|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|BORDER)
/* stdwidgets that can be changed with standard_widgets() */
#define STD_WIDGETS   (CLOSER|NAME|MOVER|ICONIFIER|FULLER|HIDE|INFO|SIZER|VS_WIDGETS|HS_WIDGETS|BORDER|XaMENU)
/* Widgets that are changable via themes */
#define THEME_WIDGETS (CLOSER|NAME|      ICONIFIER|FULLER|HIDE|INFO|SIZER|VS_WIDGETS|HS_WIDGETS|BORDER|XaMENU)
//#define         (NAME|CLOSER|FULLER|MOVER|INFO|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|MENUBAR|BACKDROP|SMALLER|BORDER|ICONIFIER)

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
	short	res1, res2;		/* 11, 12 */
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

/* Extra mouse forms */
enum xa_mouse
{
	XACRS_BUBBLE_DISC = 270,/* The Data Uncertain logo */
	XACRS_RESIZER,		/* The 'resize window' cursors */
	XACRS_SE_SIZER = XACRS_RESIZER,
	XACRS_NE_SIZER,
	XACRS_MOVER,		/* The 'move window' cursor */
	XACRS_VERTSIZER,	/* The 'resize vertically' cursor */
	XACRS_HORSIZER,		/* The 'resize horizontally' cursor */
	XACRS_POINTSLIDE,	/* ozk: The 'two-arrows pointing inwards' cursor to pinpoint slider position */

	X_LFTRT = 9,			/* Horizontal arrows (N.AES) */
	X_UPDOWN = 10			/* Vertical arrows (N.AES) */
};

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

#define XA_M_DESK	204
#define XA_M_OK		300

#define XMU_KEYBD	0x80000000
#define XMU_BUTTON	0x40000000
#define XMU_M1		0x20000000
#define XMU_M2		0x10000000
#define XMU_MX		0x08000000
#define XMU_MESAG	0x04000000
#define XMU_TIMER	0x02000000
#define XMU_FSELECT	0x01000000
#define XMU_PMSG	0x00800000

/* XaAES new xevent_multi definitions */
struct xevnt_mask
{
  unsigned long ev_0;
  unsigned long ev_1;
  unsigned long ev_2;
  unsigned long ev_3;
};

struct xevnts
{
  struct evnt_mu_keyboard  *e_kbd;
  struct evnt_mu_button    *e_but;
  struct evnt_mu_mr        *e_mu1;
  struct evnt_mu_mr        *e_mu2;
  struct evnt_mu_mx        *e_mx;
  struct evnt_mu_mesag     *e_mesag;
  struct evnt_mu_timer     *e_timer;
  struct evnt_mu_fselect   *e_fselect;
  struct evnt_mu_pmsg      *e_pmsg;
  long reserved[(32*4)-9];
};

struct evnt_mu_keyboard
{
  unsigned short scan;
  unsigned short ascii;
  unsigned short aes;
  unsigned short norm;
  unsigned short kstate;
};

struct evnt_mu_button
{
  short clicks;
  short mask;
  short state;
  short kstate;
  short x, y;
};

struct evnt_mu_mr
{
  short x, y, w, h;
  short flag;
};

struct evnt_mu_mx
{
  short reserved;
};

struct evnt_mu_mesag
{
  short type;
  short id_sender;
  short len;
  short data[16-3];
};

struct evnt_mu_timer
{
  long delta;
};

struct evnt_mu_pmsg
{
  long userlong1;
  long userlong2;
  short pid;
  short reserved[16-5];
};

struct evnt_mu_fselect
{
  long timeout;
  unsigned long rfds;
  unsigned long wfds;
  unsigned long ret;
  long reserved[8-4];
};

struct object;
typedef struct
{
	struct object	*tree;
	short		obnum;
} POPINFO;

#endif /* _xa_aes_h */
