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

/*
 * Definitions for the standard GEM application data structures
 */

#ifndef _xa_aes_h
#define _xa_aes_h

#include "mt_gem.h"
#include "mt_gemx.h"

typedef struct
{
    unsigned int    version;
    unsigned int    date;
    unsigned int    time;
    unsigned int    flags;
    unsigned long   unused_1;
    unsigned long   unused_2;
} N_AESINFO;

typedef struct
{
   char      *in_dos;                 /* Address of DOS flags       */
   int       *dos_time;               /* Address of DOS time        */
   int       *dos_date;               /* Address of DOS date        */
   long      res1;                    /*                            */
   long      res2;                    /*                            */
   long      res3;                    /* Is 0L                      */
   void      *act_pd;                 /* Running program            */
   long      res4;                    /*                            */
   int       res5;                    /*                            */
   void      *res6;                   /*                            */
   void      *res7;                   /* Internal DOS-memory list   */
   void      (*resv_intmem)();        /* Extend DOS memory          */
   long      (*etv_critic)();         /* etv_critic of GEMDOS       */
   char *    ((*err_to_str)(char e)); /* Conversion code->plaintext */
   long      res8;                    /*                            */
   long      res9;                    /*                            */
   long      res10;                   /*                            */
} MAGX_DOSVARS;

/* os_magic -> */

typedef struct
{
     long magic;                   /* Must be $87654321               */
     void *membot;                 /* End of the AES-variables        */
     void *aes_start;              /* Start address                   */
     long magic2;                  /* Is 'MAGX'                       */
     long date;                    /* Creation date ddmmyyyy          */
     void (*chgres)(int res, int txt);  /* Change resolution          */
     long (**shel_vector)(void);   /* Resident desktop                */
     char *aes_bootdrv;            /* Booting took place from here    */
     int  *vdi_device;             /* VDI-driver used by AES          */
     void *reservd1;
     void *reservd2;
     void *reservd3;
     int  version;                 /* e.g. $0201 is V2.1              */
     int  release;                 /* 0=alpha..3=release              */
} MAGX_AESVARS;

/* Cookie MagX --> */

typedef struct
{
     long		config_status;
     MAGX_DOSVARS	*dosvars;
     MAGX_AESVARS	*aesvars;
     void		*res1;
     void		*hddrv_functions;
     long		status_bits;             /* MagiC 3 from 24.5.95 on      */
} MAGX_COOKIE;

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

struct extbox_parms;
struct extbox_parms
{
	unsigned long	obspec;
	short		type;
	void		(*callout)(struct extbox_parms *p);
	void		*data;
/*
 * part below this line is filled by object-renderer when calling 'callout'
 */
	struct widget_tree *wt;
	short	index;
	RECT r;
	RECT clip;
};

/* The shel_write extensions (used in xa_fork_exec()) */
#define SW_UID 	0x1000	/* Set user id of launched child */
#define	SW_GID	0x2000	/* Set group id of launched child */

/* extended OBJECT types, XaAES only */
#define G_SLIST		39
#define G_EXTBOX	40
#define G_UNKNOWN	41

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
#define VS_WIDGETS (UPARROW|DNARROW|VSLIDE)
#define HS_WIDGETS (RTARROW|LFARROW|HSLIDE)

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
	XACRS_POINTSLIDE	/* ozk: The 'two-arrows pointing inwards' cursor to pinpoint slider position */
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

#endif /* _xa_aes_h */
