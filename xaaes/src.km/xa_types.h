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

#ifndef _xa_types_h
#define _xa_types_h

#include "global.h"

#include "xa_aes.h"
#include "xa_vdi.h"
#include "xa_defs.h"

#include "debug.h"
#include "messages.h"

#include "mint/proc.h"
#include "adi/moose.h"

#include "xa_list.h"


#define MAX_WINDOW_NAME 200
#define MAX_WINDOW_INFO 200

#define WM_WHEEL  345

#define RDRW_WA		1
#define RDRW_EXT	2
#define RDRW_ALL	(RDRW_WA|RDRW_EXT)


/* forward declarations */
struct task_administration_block;
struct widget_tree;
struct xa_widget;
struct scroll_entry;
struct fmd_result;

struct xa_lbox_info;
struct lbox_slide;

struct menu_attachements;

struct keyqueue;

#define XIMG      0x58494D47
struct img_header
{
  short version;  /* Img file format version (1) */
  short length;   /* Header length in words  (8) */
  short planes;   /* Number of bit-planes    (1) */
  short pat_len;  /* length of Patterns      (2) */
  short pix_w;    /* Pixel width in 1/1000 mmm  (372)    */
  short pix_h;    /* Pixel height in 1/1000 mmm (372)    */
  short img_w;    /* Pixels per line (=(x+7)/8 Bytes)    */
  short img_h;    /* Total number of lines               */
};
typedef struct img_header IMG_header;

/* Header of GEM Image Files   */
struct ximg_header
{
  short version;  /* Img file format version (1) */
  short length;   /* Header length in words  (8) */
  short planes;   /* Number of bit-planes    (1) */
  short pat_len;  /* length of Patterns      (2) */
  short pix_w;    /* Pixel width in 1/1000 mmm  (372)    */
  short pix_h;    /* Pixel height in 1/1000 mmm (372)    */
  short img_w;    /* Pixels per line (=(x+7)/8 Bytes)    */
  short img_h;    /* Total number of lines               */
  long  magic;    /* Contains "XIMG" if standard color   */
  short paltype;  /* palette type (0=RGB (short each)) */
};
typedef struct ximg_header XIMG_header;

struct xa_ximg_head
{
  struct ximg_header ximg;
  short *palette;	/* palette etc.                        */
  char *addr;     /* Address for the depacked bit-planes */
};
typedef struct xa_ximg_head XA_XIMG_HEAD;

struct xa_rsc_rgb
{
	short red;
	short green;
	short blue;
	short pen;
};

struct lrect
{
	long x, y, w, h;
};
typedef struct lrect LRECT;

enum menu_behave
{
	PULL,
	PUSH,
	LEAVE
};

struct xamenu
{
	struct widget_tree *wt;
	MENU menu;
};
typedef struct xamenu XAMENU;

typedef enum
{
	NW,
	N_,
	NE,
	E_,
	SE,
	S_,
	SW,
	W_,
	CDV			/* compass divisions */
} COMPASS;

/*-----------------------------------------------------------------
 * Display descriptions
 *-----------------------------------------------------------------*/
struct xa_vdi_settings
{
	struct	xa_vdi_api *api;

	short	handle;

	short	wr_mode;
	
	RECT	clip;
	RECT	screen;

	short	line_color;
	short	line_style;
	short	line_udsty;
	short	line_beg;
	short	line_end;
	short	line_width;

	short	fill_color;
	short	fill_interior;
	short	fill_style;
	short	fill_perimeter;

	short	fonts_loaded;
	short	text_color;
	short	text_effects;
	short	text_halign;
	short	text_valign;
	short	font_rid, font_sid;		/* Requested and real font ID */
	short	font_rsize, font_ssize;		/* Requested and real font size */
	short	halign, valign;
	short	font_w, font_h;
	short	cell_w, cell_h;
};

typedef enum { D_LOCAL = 0, D_TCP = 1 } XA_DISPLAY;

struct xa_colour_scheme
{
	short bg_col;			/* Colour used for backgrounds */
	short fg_col;			/* Colour used for foregrounds */
	short shadow_col;		/* Colour used for shadowed 3d edge */
	short lit_col;			/* Colour used for lit 3d edge */
	short border_col;		/* Colour used for edging */
	short highlight_col;		/* Colour used for highlighting */
};

struct xa_screen
{
	RECT r;				/* Screen dimensions */
	XA_DISPLAY display_type;	/* The type of display we are using */

	short colours;			/* Number of colours available */
	short planes;			/* Number of planes in screen */
	short pixel_fmt;
	short standard_font_height;	/* Needed for appl_getinfo */
	short standard_font_id;
	short standard_font_point;
	short small_font_id;
	short small_font_point;
	short small_font_height;	/* Needed for appl_getinfo */

	short c_max_w, c_max_h;		/* Maximum character dimensions in pixels */
	short c_min_w, c_min_h;		/* Minimum (small font) character dimensions in pixels */

	struct rgb_1000 palette[256];
};

struct xa_objc_render
{
	struct xa_colour_scheme dial_colours;
};

struct xa_wtexture
{
	short anchor;
	short flags;
	MFDB	*tl_corner;
	MFDB	*tr_corner;
	MFDB	*bl_corner;
	MFDB	*br_corner;
	MFDB	*left;
	MFDB	*right;
	MFDB	*top;
	MFDB	*bottom;
	MFDB	*body;
};
	
struct xa_wcol
{
	short	c;	/* color */
	short	i;	/* Interior style */
	short	f;	/* Fill Style */
	short	box_c;  /* box color if boxed */
	short	box_th; /* box thickness */
	short	tlc;	/* Top-Left color for 3-d effect */
	short	brc;	/* Bottom-Right color for 3-d effect */
	struct	xa_wtexture *texture;
};

struct xa_wcol_inf
{

#define WCOL_ACT3D	1
#define WCOL_DRAW3D	2
#define WCOL_DRAWBKG	4
#define WCOL_BOXED	8

	short	flags;	/* */
	short	wrm;	/* wrmode */
	struct xa_wcol n; /* normal */
	struct xa_wcol s; /* Selected */
	struct xa_wcol h; /* Highlighted */
};

struct xa_fnt_info
{
	long	f;	/* Font ID */
	short	p;	/* Point size */
	short	wrm;
	short	e;	/* Effects */
	short	fgc;	/* Foreground colour */
	short	bgc;	/* Background colour (used to obtain 3-d effect) */
};
	
struct xa_wtxt_inf
{

#define WTXT_ACT3D	1
#define WTXT_DRAW3D	2
#define WTXT_CENTER	4
#define WTXT_NOCLIP	8

	short	flags;	/* Flags */
	struct xa_fnt_info n;
	struct xa_fnt_info s;
	struct xa_fnt_info h;
};

/*-----------------------------------------------------------------
 * Configuration and options structures
 *-----------------------------------------------------------------*/
struct widget_theme;

struct options
{
	short clwtna;			/* Close Last Window Tops Next App */
	bool windowner;			/* display window owner in title. */
	bool nohide;
	bool xa_nohide;
	bool xa_nomove;
	bool xa_objced;
	bool xa_none;			/* No xa_windows at all; classic dialogue handling. */
	bool noleft;			/* Dont move window out left side of screen. */
	bool thinwork;			/* workarea frame a single line. */
	bool nolive;			/* Live moving and sizing of windows. */
	bool wheel_reverse;		/* Do you want move the slider in the wheel direction,
					 * or rather the window content? */
	bool naes;			/* N.Aes 3d compatability */
	bool naes12;
	bool naes_ff;			/**/
	bool inhibit_hide;

	short thinframe;		/* -1: keep colour frame thin
					 * +1: make a thicker frame for wasier border grabbing and sizing. */
	short wheel_page;		/* how many wheel clicks for a page */
	short wheel_mode;
	long wind_opts;			/* Default window options - see struct xa_window.opts */
	long app_opts;
	long half_screen;

#if GENERATE_DIAGS
	enum debug_item point[D_max];
#endif
};

struct opt_list
{
	struct opt_list *next;
	Path name;
	struct options options;
};

struct app_list
{
	struct app_list *next;
	Path name;
};

/*-----------------------------------------------------------------
 * Client application data structures
 *-----------------------------------------------------------------*/

/* List of Pending AES messages */
struct xa_aesmsg_list
{
	struct xa_aesmsg_list *next;
	union msg_buf message;
};

union conkey
{
	long bcon;			/* returned by Bconin */
	struct
	{
		unsigned char state;	/* Ensure conterm bit 3 is set, so we get the keyboard status in here. */
		unsigned char scan;	/* scancode */
		unsigned char dum;
		unsigned char code;	/* ascii (if applicable */
	}
	conin;
};

struct rawkey
{
	union conkey raw;
	unsigned short aes;		/* AES keycode */
	unsigned short norm;		/* mormalized keycode */
};

/*
 *
 */
struct mbs
{
	short b;
	short c;
	short x, y;
	short ks;

	bool empty;
};

/*
 * Codes for what types of event an app is currently waiting for
 * - I've used the same bit allocation and nomenclature as evnt_multi() here,
 *   but there is
 *   the extra XAWAIT_MULTI bit to indicate we are waiting from evnt_multi()
 *   as opposed to one of the individual routines (as return values are different).
 */
enum waiting_for
{
	XAWAIT_MULTI	= 0x200,	/* Waiting for an evnt_multi() */
	XAWAIT_WDLG	= 0x400,	/* Waiting for a wdlg_xxx event to occur */
	XAWAIT_MOUSE	= 0x800,	/* XaAES private; exclusive mouse event */
	XAWAIT_MENU	= 0x1000,	/* XaAES private; menu rectangle event */
};

typedef struct task_administration_block * TASK(struct task_administration_block *tab, short item);

struct xa_mouse_rect
{
	short flags;
	short m1_flag;
	short m2_flag;
	RECT m1, m2;
	TASK *t1, *t2;	/* used by tasking */
};

#define ATTACH_MAX 64	/* This is per menu_bar!!!! */

typedef struct menu_attachments
{
	struct menu_attachments *prev;
	struct menu_attachments *next;

	struct widget_tree *wt;
	int item;
	struct widget_tree *to;
	int to_item;
} XA_MENU_ATTACHMENT;

struct xa_menu_settings
{
	/* from here ... */
	long  display;		/* submenu display delay in milliseconds */
	long  drag;		/* submenu drag delay in milliseconds */
	long  delay;		/* single-click scroll delay in milliseconds */
	long  speed;		/* continuous scroll delay in milliseconds */
	short height; 		/* menu scroll height (in items) */
	/* .. to here is identical to MN_SET */

	short	behave;
};

/*************************************************************************** */
/* The vdi api
*/
struct xa_vdi_api
{
	void _cdecl (*wr_mode)		(struct xa_vdi_settings *v, short m);
	void _cdecl (*load_fonts)	(struct xa_vdi_settings *v);
	void _cdecl (*unload_fonts)	(struct xa_vdi_settings *v);
	
	void _cdecl (*set_clip)		(struct xa_vdi_settings *v, const RECT *clip);
	void _cdecl (*clear_clip)	(struct xa_vdi_settings *v);
	void _cdecl (*restore_clip)	(struct xa_vdi_settings *v, const RECT *s);
	void _cdecl (*save_clip)	(struct xa_vdi_settings *v, RECT *s);

	void _cdecl (*line)		(struct xa_vdi_settings *v, short x1, short y1, short x2, short y2, short col);
	void _cdecl (*l_color)		(struct xa_vdi_settings *v, short col);
	void _cdecl (*l_type)		(struct xa_vdi_settings *v, short type);
	void _cdecl (*l_udsty)		(struct xa_vdi_settings *v, unsigned short ty);
	void _cdecl (*l_ends)		(struct xa_vdi_settings *v, short s, short e);
	void _cdecl (*l_width)		(struct xa_vdi_settings *v, short w);
	
	void _cdecl (*t_color)		(struct xa_vdi_settings *v, short col);
	void _cdecl (*t_effects)	(struct xa_vdi_settings *v, short efx);
	void _cdecl (*t_font)		(struct xa_vdi_settings *v, short point, short id);
	void _cdecl (*t_alignment)	(struct xa_vdi_settings *v, short halign, short valign);
	void _cdecl (*t_extent)		(struct xa_vdi_settings *v, const char *t, short *w, short *h);
	void _cdecl (*text_extent)	(struct xa_vdi_settings *v, const char *t, struct xa_fnt_info *f, short *w, short *h);

	void _cdecl (*f_color)		(struct xa_vdi_settings *v, short col);
	void _cdecl (*f_interior)	(struct xa_vdi_settings *v, short m);
	void _cdecl (*f_style)		(struct xa_vdi_settings *v, short m);
	void _cdecl (*f_perimeter)	(struct xa_vdi_settings *v, short m);
	void _cdecl (*draw_texture)	(struct xa_vdi_settings *v, MFDB *texture, RECT *r, RECT *anchor);

	void _cdecl (*box)		(struct xa_vdi_settings *v, short d, short x, short y, short w, short h);	
	void _cdecl (*gbox)		(struct xa_vdi_settings *v, short d, const RECT *r);
	void _cdecl (*bar)		(struct xa_vdi_settings *v, short d, short x, short y, short w, short h);
	void _cdecl (*gbar)		(struct xa_vdi_settings *v, short d, const RECT *r);
	void _cdecl (*p_gbar)		(struct xa_vdi_settings *v, short d, const RECT *r);
	
	void _cdecl (*top_line)		(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*bottom_line)	(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*left_line)	(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*right_line)	(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*tl_hook)		(struct xa_vdi_settings *v, short d, const RECT *r, short col);
	void _cdecl (*br_hook)		(struct xa_vdi_settings *v, short d, const RECT *r, short col);

	void _cdecl (*write_disable)	(struct xa_vdi_settings *v, RECT *r, short colour);

	const char * _cdecl	(*prop_clipped_name)	(struct xa_vdi_settings *v, const char *s, char *d, int w, short *ret_w, short *ret_h, short method);
	void _cdecl		(*wtxt_output)		(struct xa_vdi_settings *v, struct xa_wtxt_inf *wtxti, char *txt, short state, const RECT *r, short xoff, short yoff);

};

/*-----------------------------------------------------------------
 * Object Handler Structures
 *-----------------------------------------------------------------*/

/*
 * Object display callback parameter
 * - This only exists because Lattice 5.52 has a bug that crashes the compiler
 *   if you have:
 *		typedef void (*ObjectDisplay)(OBJECT *ob, int parent_x, int parent_y);
 */
/* HR: Well, it appeared to be handy as well. */
/* HR: 070101  completely combined with XA_TREE. */

/* A function of the type used for widget behaviours is a 
   'WidgetBehaviour'. */
typedef bool WidgetBehaviour	(enum locks lock,
				 struct xa_window *wind,
				 struct xa_widget *widg,
				 const struct moose_data *md);

typedef bool WidgetKeyInput	(struct xa_window *wind,
				 struct xa_widget *widg,
				 const struct rawkey *key);
#if 0
typedef bool DisplayWidget(enum locks lock, struct xa_window *wind,
			   struct xa_widget *widg);
#endif

typedef bool FormKeyInput(enum locks lock,
			  struct xa_client *client,
			  struct xa_window *window,
			  struct widget_tree *wt,
			  const struct rawkey *key);

typedef bool FormMouseInput(enum locks lock,
			    struct xa_client *client,
			    struct xa_window *window,
			    struct widget_tree *wt,
			    const struct moose_data *md);

typedef void FormExit(struct xa_client *client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr);


typedef int WindowKeypress(enum locks lock, struct xa_window *wind,
			   struct widget_tree *wt,
			   unsigned short keycode, unsigned short nkcode, struct rawkey key);

/* Object display function type */
typedef void ObjectDisplay(enum locks lock, struct widget_tree *wt, struct xa_vdi_settings *v);

/* Object handler function type */
// typedef void ObjectHandler(enum locks lock, struct widget_tree *wt);

#if 0
struct widget_behaviour
{
	WidgetBehaviour *click;
	WidgetBehaviour *drag;
};
#endif

typedef short _cdecl wdlg_exit  (void *dialog,
				EVNT *evnt,
				short obj,
				short clicks,
				void *data);

struct xa_data_hdr;
struct xa_data_hdr
{
	unsigned long		id;
	char			name[16];
	struct xa_data_hdr	*next;
	void			(*destruct)(void *data);
};

/* Object Tree based widget descriptor */
struct wdlg_info
{
	struct xa_data_hdr	h;

	void *handle;			/* For use as 'wdialog structure' */
	struct xa_window *wind;		/* cross reference to parent window. */
	short code;			/* Data for wdlg_xxx extension. */
	short flag;
	void *user_data;
	void *data;
	EVNT *evnt;
	wdlg_exit *exit;

	short ify_obj;
	struct widget_tree *std_wt;
	struct widget_tree *ify_wt;
	char std_name[MAX_WINDOW_NAME];
	char ify_name[40];
};
#if 0
typedef	void  _cdecl lbox_select(LIST_BOX *box,
				OBJECT *tree,
				struct lbox_item *item,
				void *user_data,
				short obj_index,
				short last_state);
typedef	short _cdecl lbox_set	(LIST_BOX *box,
				OBJECT *tree,
				struct lbox_item *item,
				short obj_index,
				void *user_data,
				GRECT *rect,
				short first);
#endif
typedef bool _cdecl lbox_scroll	(struct xa_lbox_info *lbox,
				 struct lbox_slide *s,
				 short n);

struct lbox_slide
{
	short ul;
	short dr;
	short bkg;
	short sld;

	short num_visible;
	short first_visible;
	short entries;
	short pause;
	short flags;
	RECT ofs;

	lbox_scroll *dr_scroll;
	lbox_scroll *ul_scroll;
};
	
struct xa_lbox_info
{
	struct xa_data_hdr	h;
	struct xa_vdi_settings	*vdi_settings;

	struct xa_lbox_info *next;	/* Next lbox attached to this widget tree */
	void  *wdlg_handle;		/* wdlg handle */
	void  *lbox_handle;		/* lbox handle */
	short flags;			/* Flags */
	short parent;			/* LBOX parent object */
	struct lbox_item *items;	/* Pointer to first item */
	short *objs;			/* Pointer to array of objects making up elements in LBOX */
	struct lbox_slide aslide;	/* Slider A object indexes + info */
	struct lbox_slide bslide;	/* Slider B object indexes + info */
	struct widget_tree *wt;		/* Pointer to the widget tree this LBOX is attached to */
	void *user_data;		/* User data */
	long slct;			/* Select callback function */
	long set;			/* Set callback function */
};

struct xa_fnts_item;
struct xa_fnts_item
{
	struct xa_fnts_item *link;
	struct xa_fnts_item *nxt_family;
	struct xa_fnts_item *nxt_kin;

	FNTS_ITEM f;
	char	font_name[64];
	char	family_name[64];
	char	style_name[64];
	char	sizes[64];
};

#define XAFNTS_DISPLAY	1

struct xa_fnts_info
{
	struct xa_data_hdr h;
	struct xa_vdi_settings *vdi_settings;

	void	*handle;
	struct	xa_window *wind;
	struct	widget_tree *wt;
	
	short	vdi_handle;
	short	fnts_loaded;

	short	num_fonts;
	short	font_flags;
	short	dialog_flags;
	short	button_flags;

	struct	xa_fnts_item *fnts_ring;
	struct	xa_fnts_item *fnts_list;
	
	struct	xa_fnts_item *fnts_selected;
	short	exit_button;
	short	resrvd;
	long	fnt_id;
	long	fnt_pt;
	long	fnt_ratio;

	char	*sample_text;
	char	*opt_button;
	
};

struct wdlg_evnt_parms
{
	struct xa_window *wind;
	struct widget_tree *wt;
	EVNT *ev;
	struct wdlg_info *wdlg;
	short (*callout)(struct xa_client *client, struct wdlg_info *wdlg, void *ev, short nxtobj, short mclicks, void *udata, void *feedback);
	void  (*redraw)(enum locks lock, struct xa_window *wind, short start, short depth, RECT *r);

	short obj;
};

struct xa_pdlg_drv_info;
struct xa_pdlg_drv_info
{
	struct xa_pdlg_drv_info *next;
	DRV_INFO		drv_info;
};

struct xa_pdlg_info
{
	struct xa_data_hdr h;

	void	*handle;
	struct	xa_window *wind;
	struct	widget_tree *mwt;
	struct	widget_tree *dwt;
	struct	scroll_info *list;

	short	exit_button;
	short	flags;
	short	dialog_flags;
	short	option_flags;

	struct xa_pdrv_info *priv_drivers;
	
	PRN_SETTINGS *settings;
	PRN_SETTINGS current_settings;
	char document_name[256];
};

#define OB_CURS_ENABLED	1
#define OB_CURS_DRAWN	2
struct objc_edit_info
{
	short obj;	/* Index of editable object */
	short pos;	/* Cursor (char) position */
	short c_state;	/* Cursor state */
	RECT cr;	/* Cursor coordinates, relative */
};
	
struct widget_tree
{
	struct widget_tree *next;	/* Next widget tree */
	short	links;

#define WTF_ALLOC	0x00000001
#define WTF_XTRA_ALLOC	0x00000002
#define WTF_TREE_ALLOC	0x00000004
#define WTF_TREE_CALLOC 0x00000008
#define WTF_STATIC	0x00000010
#define WTF_AUTOFREE	0x00000020
#define WTF_FBDO_SLIST	0x00000040	/* form_button() handles SLIST objects */
	unsigned long	flags;

#define WTR_POPUP	0x00000001
#define WTR_ROOTMENU	0x00000002

	unsigned long	rend_flags;

	struct xa_window *wind;		/* Not of any specific use just yet...*/
	struct xa_client *owner;	/* The tree widget would be owned by a different app to
					 * the actual window (like the menu bar on the root window),
					 * or the desktop toolbar widget. HR 270801: was: int pid; */
	struct xa_widget *widg;		/* Cross pointer to widget. */
	
	OBJECT *tree;			/* The object tree */
	short current;			/* current item within above tree. */
	RECT r;				/* Why not do the addition (parent_x+ob_x) once in the caller? */
					/* And set a usefull RECT as soon as possible, ready for use in
					 * all kind of functions. */
	RECT area;
	ushort *state_mask;

	short parent_x;			/* Keep both in: dont need to change everything in a single effort */
	short parent_y;

	struct objc_edit_info e;

	short lastob;			/* Can be used to validate item number */
	short which;			/* kind of event for use by WDIAL exit handler. */

	short rdx, rdy;
	short puobj;
	short pdx, pdy;

	bool is_menu;			/* true when the tree is a menu or part of it. */
	bool menu_line;			/* draw a menu line. */
	bool zen;			/* true when the tree is embedded in a window.
	                                 * Do not shadow and border. */
	short dx, dy;			/* displacement of root from upper left corner of window
					 * for operation by sliders. */

	short ox, oy;			/* Root offset from upper left corner of window
					 * caused by root-object borders
					 */

	FormExit *exit_form;		/* Called if exit condition occurs
					 * while handling a form_do or a toolbar
					 * or anything the like ;-) */
	void *extra;			/* Extra info if needed (texts for alert) */
	struct xa_lbox_info *lbox;
};
typedef struct widget_tree XA_TREE;

/* used for linking resources's per client */

#define RSRC_ALLOC 1
struct remember_alloc;
struct remember_alloc
{
	struct remember_alloc *next;
	void *addr;
};
	
struct xa_rscs
{
	struct xa_rscs	*next, *prior;
	short 		id;
	short 		handle;
	short 		flags;
	struct aes_global *globl;	/* Need a global per resource, otherwise rsrc_gaddr would be ambiguous. */
	struct remember_alloc *ra;
	void		*rsc;
	struct rgb_1000	*palette;
};

/*
 * Structure used to pass form_do/dial results to FormExit functions()
 */
struct fmd_result
{
	bool no_exit;
	short obj;
	short obj_state;
	short dblmask;
	ushort aeskey;
	ushort normkey;
	short ks;
	const struct moose_data *md;
	const struct rawkey *key;
};

struct fmd
{
	struct xa_window *wind;		/* Pointer to a window that could be about to become a dialog */
	struct widget_tree *wt;
	short state;			/* fmd.r contains a valid rectangle of a form_dial, which is postponed. */
#define SCREEN_UPD	1
#define MOUSE_UPD	2
	short lock;			/* Client has locked the screen. */
	XA_WIND_ATTR kind;		/* Window attributes to be used. */
	FormKeyInput *keypress;
	FormMouseInput *mousepress;
	RECT r;				/* The rectangle for the postponed dialogue window */
};

#define CEVNT_MOVEMENT	1
#define CEVNT_BUTTON	2
#define CEVNT_MENUSTART 4

struct c_event
{
	struct c_event		*next;
	void			(*funct)(enum locks, struct c_event *, bool cancel);
	struct xa_client	*client;
	void			*ptr1;
	void			*ptr2;
	int			d0;
	int			d1;
	RECT			r;
	struct	moose_data	md;
};

struct keyqueue
{
	struct keyqueue *next;
	struct rawkey	key;
};

struct fselect_result
{
	bool inuse;
	long ret;
	long rfds;
	long wfds;
	long xfds;
};

#define XAPP_XT_WF_SLIDE	0x00000001	/**/


typedef unsigned long AES_function(enum locks lock, struct xa_client *client, AESPB *pb);

/*-----------------------------------------------------------------
 * Windows & Widgets
 *----------------------------------------------------------------- */

/* Relative coordinate types */
typedef enum
{
	/* Bit 0 - 0 = top,	1 = bottom	*/
	/* bit 1 - 0 = left,	1 = right	*/
	/* bit 2 - 0 = no Center 1 = center	*/
	
	R_BOTTOM	= 0x0001,
	R_RIGHT		= 0x0002,
	R_CENTER	= 0x0004,
	R_VERTICAL	= 0x0008,
	R_VARIABLE	= 0x0010,
	R_NONE		= 0x8000,
	
	LT = 0,		/* 0000 */	/* Top right */
	LB,		/* 0001 */	/* Bottom right */
	RT,		/* 0010 */	/* Top left */
	RB,		/* 0011 */	/* Bottom left */
	
	CT,		/* 0100 */	/* Top centred */
	CB,		/* 0101 */	/* Bottom Centered */
	
	CR,		/* 0110 */	/* Right centered */
	CL,		/* 0111 */	/* Left centered */
	
	HLT,		/* 0000 */	/* Top right */
	HLB,		/* 0001 */	/* Bottom right */
	HRT,		/* 0010 */	/* Top left */
	HRB,		/* 0011 */	/* Bottom left */
	HCT,		/* 0100 */	/* Top centred */
	HCB,		/* 0101 */	/* Bottom Centered */
	
	HCR,		/* 0110 */	/* Right centered */
	HCL,		/* 0111 */	/* Left centered */

	
	NO = 0x8000,	/* */

} XA_RELATIVE;

 /* bit 0 - orientation;	0 = vertical, 1 = horizontal */
 /* bit 1&2 - placement:	0 = top, 1 = midle, 2 = end */

#define XAR_VERT		1
#define XAR_PM			6
#define XAR_START		(1 << 1)
#define XAR_MIDDLE		(2 << 1)
#define XAR_END			(3 << 1)
#define XAR_NO			0x8000


/* Widget Index's */

enum xa_widgets
{
 /*
  * These widgets must be processable totally indipendant of
  * process context.
  */
	XAW_BORDER = 0,			/* Extended XaAES widget, used for border sizing. */
	XAW_TITLE,
	XAW_CLOSE,
	XAW_FULL,
	XAW_INFO,
	XAW_RESIZE,
	XAW_UPLN,			/* 6 */
	XAW_DNLN,			/* 7 */
	XAW_VSLIDE,
	XAW_LFLN,			/* 9 */
	XAW_RTLN,			/* 10 */
	XAW_HSLIDE,
	XAW_ICONIFY,
	XAW_HIDE,			/* 13 */
 
 /*
  * The widget types above this comment MUST be context indipendant.
  * The widget types blow this comment are considered context-dependant
  * and must be processed via client events.
  * Furthermore, remember to change XA_MAX_CF_WIDGETS below if you
  * put insert a new context dependant widget before XAW_TOOLBAR!
 */
	XAW_TOOLBAR,			/* 14 Extended XaAES widget */
	XAW_MENU,			/* 15 Extended XaAES widget, must be drawn last. */
	
	XAW_MOVER,			/* Not actually used like the others */
	XAW_UPPAGE,
	XAW_DNPAGE,
	XAW_LFPAGE,
	XAW_RTPAGE,
	/* Number of available XA_WIDGET slots in a the window for default/standard widgets */
	XA_MAX_WIDGETS
};
typedef enum xa_widgets XA_WIDGETS;

#define XA_MAX_CF_WIDGETS	XAW_TOOLBAR

struct xa_widget;

typedef bool _cdecl DrawWidg(struct xa_window *wind, struct xa_widget *widg, const RECT *clip);
typedef void _cdecl DrawCanvas(struct xa_window *wind, RECT *outer, RECT *inner, const RECT *clip);
typedef void _cdecl SetWidgSize(struct xa_window *wind, struct xa_widget *widg);
typedef void _cdecl WidgGemColor(struct xa_window *wind, short gem_widget, BFOBSPEC *c);
typedef void _cdecl DrawFrame(struct xa_window *wind, const RECT *clip);

struct render_widget
{
	XA_WIND_ATTR	tp;
	XA_WIND_ATTR	*tp_depends;
	short		xaw_idx;
	short		pos_in_row;
	DrawWidg	*draw;
	SetWidgSize	*setsize;
	void		*priv;
};

struct nwidget_row
{
	short		rel;
	XA_WIND_ATTR	tp_mask;
	struct render_widget **w;
};
	
struct xa_widget_row;
struct xa_widget_row
{
	struct xa_widget_row	*prev;
	struct xa_widget_row	*next;

	XA_RELATIVE		rel;
	XA_WIND_ATTR		tp_mask;
	struct xa_widget 	*start;
	short	rownr;
	RECT r;
	short rxy;
};

struct widget_theme
{
	struct xa_data_hdr	h;
	long			links;


	struct nwidget_row	*layout;

	WidgGemColor		*get_widgcolor;
	WidgGemColor		*set_widgcolor;

	RECT			outer;
	RECT			inner;
	DrawCanvas		*draw_canvas;

	DrawFrame		*draw_waframe;

	struct render_widget	exterior;
	
	struct render_widget	border;
	struct render_widget	title;
	struct render_widget	closer;
	struct render_widget	fuller;
	struct render_widget	info;
	struct render_widget	sizer;
	struct render_widget	uparrow;
	struct render_widget	dnarrow;
	struct render_widget	vslide;
	struct render_widget	lfarrow;
	struct render_widget	rtarrow;
	struct render_widget	hslide;
	struct render_widget	iconifier;
	struct render_widget	hider;
	struct render_widget	toolbar;
	struct render_widget	menu;

	void			*module;
};

struct xa_widget_theme
{
// 	struct widget_theme *active;
	struct widget_theme *client;
	struct widget_theme *popup;
	struct widget_theme *alert;
};

struct xa_module_api
{
	char * _cdecl	(*sysfile)		(const char *fname);
	RSHDR * _cdecl	(*load_resource)	(char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal);
	OBJECT * _cdecl	(*resource_tree)	(RSHDR *rsc, long num);

	void _cdecl			(*init_wt)	(struct widget_tree *wt, OBJECT *obtree);
	struct widget_tree * _cdecl	(*new_wt)	(OBJECT *obtree);
	struct widget_tree * _cdecl	(*obtree_to_wt)	(OBJECT *obtree);
	void _cdecl			(*remove_wt)	(struct widget_tree *wt);

	/* OBJECT specific functions */
	void _cdecl	(*ob_spec_xywh)		(OBJECT *tree, short obj, RECT *r);
	void _cdecl	(*object_spec_wh)	(OBJECT *ob, short *w, short *h);

	void * _cdecl	(*rp2ap)	(struct xa_window *wind, struct xa_widget *widg, RECT *r);
	void _cdecl	(*rp2apcs)	(struct xa_window *wind, struct xa_widget *widg, RECT *r);

	void * _cdecl	(*kmalloc)		(long size);
	void * _cdecl	(*umalloc)		(long size);
	void _cdecl	(*kfree)		(void *addr);
	void _cdecl	(*ufree)		(void *addr);

	void _cdecl	(*bclear)		(void *addr, unsigned long len);

	void * _cdecl	(*lookup_xa_data)	(struct xa_data_hdr **l,    void *_data);
	void _cdecl	(*add_xa_data)		(struct xa_data_hdr **list, void *_data, char *name, void _cdecl(*destruct)(void *d));
	void _cdecl	(*remove_xa_data)	(struct xa_data_hdr **list, void *_data);
	void _cdecl	(*delete_xa_data)	(struct xa_data_hdr **list, void *_data);
	void _cdecl	(*free_xa_data_list)	(struct xa_data_hdr **list);

	void _cdecl	(*load_img)		(char *fname, MFDB *result);	
};

struct xa_module_widget_theme
{
	char		*sname;
	char		*lname;

	void * _cdecl	(*init_module)(const struct xa_module_api *, const struct xa_screen *screen, char *widg_name);
	void _cdecl	(*exit_module)(void *module);

	long _cdecl	(*new_theme)(void *module, short win_type, struct widget_theme **);
	long _cdecl	(*free_theme)(void *module, struct widget_theme **);

	long _cdecl	(*new_color_theme)(void *module, void **ontop, void **untop);
	void _cdecl	(*free_color_theme)(void *module, void *ctheme);
};

struct xa_widget_methods
{
	short		properties;
	short		statusmask;
	
	struct render_widget	r;

#define WIP_NOTEXT	0x0001		/* Widget is NOT part of window exterior, will not be automatically redrawn */
#define WIP_WACLIP	0x0002		/* Widget is part of, and will be clipped by, windows work area */
#define WIP_ACTIVE	0x0004		/* If this bit is set, widget is clickable, else is just to draw the
					 * corresponding part of window exterior and clicks on it is ignored
					 */
#define WIP_INSTALLED	0x0008

	WidgetBehaviour	*click;
	WidgetBehaviour *drag;
	WidgetBehaviour *release;
	WidgetBehaviour *wheel;
	
	WidgetKeyInput	*key;

	void (*destruct)(struct xa_widget *w);
};

/*
 * Parameter block used to communicate parameters to set_toolbar_widget();
 * A pointer value of NULL (0L) means "Use standard handler".
 * A pointer value of -1L makes the corresponding handler pointer to be disabled, (a NULL pointer is filled in)
 * Any other pointer values are considered pointers to a function.
 */

struct toolbar_handlers
{	
	FormExit	*exitform;
	FormKeyInput	*keypress;

	DrawWidg	*draw;
	WidgetBehaviour	*click;
	WidgetBehaviour	*drag;
	WidgetBehaviour	*release;
	void (*destruct)(struct xa_widget *w);
};

/* Window Widget */
struct xa_widget
{
	struct xa_widget	*next_in_row;

	struct xa_window	*wind;		/* Window to which this widget belongs */
	struct xa_client	*owner;
	struct xa_widget_methods m;

	short		state;		/* Current status (selected, etc) */
	
	RECT		r;		/* Relative position */
	RECT		ar;		/* Absolute position */

#define XAWF_ALLOC		1
#define XAWF_STUFFKMALLOC	2
	short flags;

	short x, y;			/* If we are activated because we've been clicked on, this is the
					 * relative location of the click (click_object_widget)*/
	short arrowx;			/* WM_ARROWED msg type */
	short xarrow;			/* reverse action when right clicked. */
	short limit;			/* on which limit to stop */
	short xlimit;			/* for reverse action */
	short slider_type;		/* which slider should move for scroll widget */

#define STUFF_IS_WT 1

	short stufftype;		/* type of widget dependant pointer */
	void *stuff;			/* Pointer to widget dependant context data, if any */

	short  start;			/* If stuff is a OBJECT tree, we want start drawing here */
};
typedef struct xa_widget XA_WIDGET;

/* Pending action from a widget */
struct xa_pending_widget
{
	XA_WIDGET *widg;		/* Pointer to widget for which this action is pending */
	struct xa_window *wind;		/* Window to which the widget is connected */
	WidgetBehaviour *action;	/* Function to call */
	short x, y;
	struct moose_data m;
	int offs;			/* slider information */
	RECT d;				/* distance information */
	int xy;				/* compass when border sizing */
	bool cont;
};
typedef struct xa_pending_widget XA_PENDING_WIDGET;

/* Extra details for a slider widget */
#define SLIDER_UPDATE	1
struct xa_slider_widget
{
	short flags;
	short position;			/* Actual position of the slider (0-1000(SL_RANGE)) */
	short length;			/* Length (0-1000(SL_RANGE)) */
	short rpos;
	RECT r;				/* physical */
};
typedef struct xa_slider_widget XA_SLIDER_WIDGET;

#define ZT_A	1
#define ZT_B	2
#define ZT_C	3
#define ZT_D	4

/* Rectangle List entry */
struct xa_rect_list
{
	struct xa_rect_list *next;
	/* Dimensions of segment */
	RECT r;
};

struct xa_rectlist_entry
{
	struct xa_rect_list *start;
	struct xa_rect_list *next;
};

enum window_type
{
	created_for_CLIENT	= 0x0000,
/* All of the following flags indicate at least that a windows workarea is compleyely
   occupied by (part of) the dialoge root object. */
	created_for_FMD_START	= 0x0001,
	created_for_FORM_DO	= 0x0002,
	created_for_POPUP	= 0x0004,
	created_for_WDIAL	= 0x0008,
	created_for_TOOLBAR	= 0x0010,
	created_for_SLIST	= 0x0020,
	created_for_AES		= 0x0100,
	created_for_ALERT	= 0x0200,
	created_for_CALC	= 0x0400,
};
typedef enum window_type WINDOW_TYPE;

enum xa_window_class
{
	WINCLASS_CLIENT = 0,
	WINCLASS_POPUP,
	WINCLASS_ALERT,
	WINCLASS_SLIST,
};
typedef enum xa_window_class WINDOW_CLASS;

/* Callback for a window's auto-redraw function */
typedef int WindowDisplay (enum locks lock, struct xa_window *wind);

typedef void DrawWinElement(struct xa_window *wind);

#define XAWO_WHEEL	((long)WO0_WHEEL << 16)
#define XAWO_FULLREDRAW ((long)WO0_FULLREDRAW << 16)
#define XAWO_NOBLITW	((long)WO0_NOBLITW << 16)
#define XAWO_NOBLITH	((long)WO0_NOBLITH << 16)
#define XAWO_SENDREPOS	((long)WO0_SENDREPOS << 16)
#define XAWO_WCOWORK	((long)WO0_WCOWORK << 16)

#define XAWO_SUPPORTED	(XAWO_WHEEL|XAWO_FULLREDRAW|XAWO_NOBLITW|XAWO_NOBLITH|XAWO_SENDREPOS|XAWO_WCOWORK)

enum window_status
{
	XAWS_OPEN	= 0x0001,
	XAWS_ICONIFIED	= 0x0002,
	XAWS_SHADED	= 0x0004,
	XAWS_ZWSHADED	= 0x0008,
	XAWS_HIDDEN	= 0x0010,
	XAWS_FULLED	= 0x0020,
};
typedef enum window_status WINDOW_STATUS;

struct xa_wc_cache;
struct xa_wc_cache
{
	struct xa_wc_cache *next;
// 	void  *wtheme_handle;
	short	class;
	XA_WIND_ATTR tp;
	RECT delta;
	RECT wadelta;
};

/* Window Descriptor */
struct xa_window
{
	struct xa_window	*next;	/* Window list stuff - next is the window below */
	struct xa_window	*prev;	/*		     - prev is the window above (both NULL terminated) */
	struct xa_client	*owner;
	
	WINDOW_TYPE	dial;		/* Flag - 1 = this window was created by form_dial,
					 *            so don't destroy it until the form_dial(FMD_FINISH)
					 *      - 2 = created by form_do()
					 *      - 4 = otherwise created on behalf of the AES
					 *      - 8 = or created on behalf of wdlg_xxx extension. */

	short	class;

	WINDOW_STATUS window_status;	/* Window status */
	
	XA_WIND_ATTR	requested_widgets;
	XA_WIND_ATTR	active_widgets;	/* Summary of the current standard widgets for the window */
	XA_WIND_ATTR	save_widgets;	/* Remember active_widgets if iconified */
	XA_WIND_ATTR	save_requested_widgets;
	RECT delta;
	RECT wadelta;
	RECT save_delta;
	RECT save_wadelta;

	/*
	 * These are pointers to structures holding widget color information for the
	 * widget theme module. XaAES will update the 'colors' member whenever the
	 * window goes on/off top. Theme modules should then fetch its color data
	 * from 'colours' when rendering widgets.
	 */
	void *colours;
	void *ontop_cols;
	void *untop_cols;
	struct xa_vdi_settings *vdi_settings;
	struct widget_theme *active_theme;
// 	struct xa_widget_theme *widget_theme;

	long rect_lock;
	long opts;			/* Window options. XAWO_xxx */
	short wheel_mode;		/* mouse wheel mode */
	bool nolist;			/* If true, dont put in the window_list. For embedded listboxes mainly. */
	bool thinwork;			/* if true and colour then work := single line box */
	bool dial_followed;		/* false immediate after opening a dial window.
	                	       	 * true after first objc_draw. */
	bool wa_frame;

#define WAB_LEFT	1
#define WAB_RIGHT	2
#define WAB_TOP		4
#define WAB_BOTTOM	8

#define L_BORDER	1
#define R_BORDER	2
#define T_BORDER	4
#define B_BORDER	8

	short wa_borders;
	short ext_borders;
	short x_shadow;
	short y_shadow;
	RECT max;			/* Creator dimension's, maximum for sizing */
	RECT min;
	RECT r;				/* Current dimensions */
	RECT rc;			/* Real coordinates when shaded */
	RECT ro;			/* Original dimemsions when iconified */
	RECT wa;			/* user work area */
	RECT rwa;			/* work area minus toolbar, if installed - else same as wa */
	RECT bd;			/* border displacement */
	RECT rbd;
	RECT ba;			/* border area for use by border sizing facility. */
	RECT pr;			/* previous dimensions */
	RECT t;				/* Temporary coordinates used internally */
	
	RECT outer;
	RECT inner;

	short sw, sh;			/* width(not used) and height to use when SHADED */
	short hx, hy;

	short handle;			/* Window handle */
	short frame;			/* Size of the frame (0 for windowed listboxes) */

	struct xa_rectlist_entry rect_list;
	struct xa_rectlist_entry rect_user;
	struct xa_rectlist_entry rect_opt;
	struct xa_rectlist_entry rect_toolbar;
	
	bool use_rlc;
	RECT rl_clip;
	
	void *background;		/* Pointer to a buffer containing the saved background */

	WindowDisplay *redraw;		/* Pointer to the window's auto-redraw function (if any) */
	FormKeyInput *keypress;		/* Pointer to the window's keyboard handler function (if any) */
	WindowDisplay *destructor;	/* Pointer to the window's destructor function (auto-called on
					 * window deletion if a fully auto window) */

	/* Pointer to the internal message handler for this window
	 * (to be handled by the creator) */
	SendMessage		*send_message;
	DoWinMesag		*do_message;

	OBJECT			*winob;		/* Tree and index of a sub window (The parent object of the window) */
	int			winitem;	/* currently used by list boxes */

#if USER_WIDGETS
	XA_WIDGET_LIST		*user_widgets;	/* Pointer to a list of user added widgets */
#endif
	XA_WIDGET 		*tool;		/* If dialogue or popup: which widget is used. */
	struct xa_widget_row	*widg_rows;
	DrawFrame		*draw_waframe;
	DrawCanvas		*draw_canvas;
	XA_WIDGET		widgets[XA_MAX_WIDGETS]; /* The windows standard widget set (array for speed) */

	char			wname[MAX_WINDOW_NAME];	/* window name line (copy) */
	char			winfo[MAX_WINDOW_INFO];	/* window info line (copy) */

	struct xa_data_hdr *xa_data;
	struct wdlg_info *wdlg;
};

struct xa_window *get_top(void);
extern struct xa_window *root_window;
#define window_list S.open_windows.first
#define nolist_list S.open_nlwindows.first

struct scroll_info;

/* Directory entry flags */
enum scroll_entry_type
{
	SETYP_AMAL	= 0x0001,
	SETYP_MAL	= 0x0002,

	SETYP_STATIC	= 0x8000,

};
typedef enum scroll_entry_type SCROLL_ENTRY_TYPE;

enum scroll_entry_flags
{
	SEF_WTXTALLOC = 0x0001,
	SEF_WCOLALLOC = 0x0002,
};
typedef enum scroll_entry_flags SCROLL_ENTRY_FLAGS;

enum scroll_info_flags
{
	SIF_KMALLOC	= 0x0001,
	SIF_SELECTABLE	= 0x0002,
	SIF_ICONINDENT	= 0x0004,
	SIF_MULTISELECT = 0x0008,
	SIF_MULTIBOXED  = 0x0010,
	SIF_AUTOSLIDERS = 0x0020,
	SIF_AUTOSELECT  = 0x0040,
	SIF_TREEVIEW	= 0x0080,
	SIF_AUTOOPEN	= 0x0100,

};
typedef enum scroll_info_flags SCROLL_INFO_FLAGS;

#define SETAB_RJUST		0x0001
#define SETAB_CJUST		0x0002
#define SETAB_REL		0x0004
#define SETAB_RORIG		0x0008
#define SETAB_END		0x8000
struct se_tab;
struct se_tab
{
	short flags;
	short uflags;
	RECT v;
	
	RECT r;
	short widest;
	short highest;
};
#if 0
struct se_text_tabulator
{
	short flags;
	RECT r;
	short widest;
	short highest;
};
#endif

#define SETEXT_TXTSTR	1
#define SETEXT_ALLOC	2
struct se_icon
{
	RECT r;
	OBJECT *icon;
};
struct se_text;
struct se_text
{
// 	struct se_text *next;
	short flags;
	unsigned short tblen;
	char *text;
	short w;
	short h;
	struct se_icon icon;
	struct xa_wtxt_inf *fnt;
	char txtstr[0];
};

/*
 * if f.id == 0, fnt info is ignored

 * If icon == NULL, icon is left unchanged
 * If icon == -1, icon is removed if already present
 */
struct setcontent_text
{
	short index;
	struct xa_wtxt_inf *fnt;
	struct xa_wcol_inf *col;
	OBJECT *icon;
	char *text;
// 	struct xa_fnt_info f;
};
	
struct sc_text
{
	short index;
	short strings;
	const char *text;
};

/*
 * Structure used to pass contents of a 'SETYPE_TEXT' scroll entry to the
 * scroll-list 'add' function
 */
struct scroll_content
{
	struct sc_text t;
	OBJECT *icon;
	short xstate;
	short xflags;
	long usr_flags;
	void *data;
	void (*data_destruct)(void *);
	struct xa_wtxt_inf *fnt;
	struct xa_wcol_inf *col;
};

#if 0
struct scroll_content_text
{
	short n_strings;
	struct se_text *text;
	struct xa_wtxt_inf *fnt;
};
#endif

#define SECONTENT_TEXT	0
#define SECONTENT_ICON  1

struct se_content;
struct se_content
{
	struct se_content *prev;
	struct se_content *next;
	short type;
	short index;
	struct xa_wtxt_inf *fnt;
	struct xa_wcol_inf *col;
	union
	{
		struct se_text text;
		struct se_icon icon;
	}c;
};

#if 0
struct scroll_entry_content
{
	short type;
	long usr_flags;
	void *data;
	OBJECT *icon;
	struct xa_wcol_inf *col;
	union
	{
		struct scroll_content_text text;
	}td;
};
#endif

typedef	bool	scrl_compare	(struct scroll_info *list, struct scroll_entry *new_ent, struct scroll_entry *cur_ent);
typedef int 	scrl_click	(struct scroll_info *list, struct scroll_entry *e, const struct moose_data *md);
typedef void	scrl_list	(struct scroll_info *list);
typedef void	scrl_widget	(struct scroll_info *list, bool rdrw);
typedef void	scrl_empty	(struct scroll_info *list, struct scroll_entry *s, SCROLL_ENTRY_TYPE flags);
typedef int	scrl_add	(struct scroll_info *list, struct scroll_entry *parent, scrl_compare *sort, struct scroll_content *sc, short where, SCROLL_ENTRY_TYPE flags, short redraw);
typedef struct scroll_entry *	scrl_del	(struct scroll_info *list, struct scroll_entry *s, short redraw);
typedef void	scrl_vis	(struct scroll_info *list, struct scroll_entry *s, short redraw);
typedef int	scrl_set	(struct scroll_info *list, struct scroll_entry *s, short what, unsigned long data, short rdrw);
typedef int	scrl_get	(struct scroll_info *list, struct scroll_entry *s, short what, void *arg);
typedef void	scrl_redraw	(struct scroll_info *list, struct scroll_entry *s);
typedef struct scroll_entry * scrl_search(struct scroll_info *list, struct scroll_entry *start, short mode, void *data);

/* HR: The FS_LIST box is the place holder and the
 *     entrypoint via its TOUCHEXIT flag.
 *     The list box itself is turned into a full fledged window.
 */
#define SEADD_PRIOR		1
#define SEADD_CHILD		2

#define NOREDRAW	0
#define NORMREDRAW	1
#define FULLREDRAW	2

/* scrl_search modes */
#define SEFM_BYDATA	0
#define SEFM_BYTEXT	1
#define SEFM_LAST	2

/* scrl_set modes */
#define SESET_STATE		 0
#define SESET_NFNT		 1
#define SESET_SFNT		 2
#define SESET_HFNT		 3
#define SESET_WTXT		 4
#define SESET_SELECTED		 5
#define SESET_UNSELECTED	 6
#define SESET_MULTISEL		 7
#define SESET_OPEN		 8
#define SESET_TAB		 9
#define SESET_USRFLAGS		10
#define SESET_USRDATA		11
#define SESET_TEXT		12
#define SESET_TREEVIEW		13
#define SESET_CURRENT		14

#define SESET_M_STATE		15

/* SESET_UNSELECTED arguments */
#define UNSELECT_ONE	0
#define UNSELECT_ALL	1

struct seset_tab
{
	short index;
	short flags;
	RECT r;
};

/* scrl_get modes */
#define SEGET_STATE		 0
#define SEGET_XSTATE		 1
#define SEGET_NFNT		 2
#define SEGET_SFNT		 3
#define SEGET_HFNT		 4
#define SEGET_WTXT		 5
#define SEGET_SELECTED		 6
#define SEGET_ENTRYBYIDX	 7
#define SEGET_ENTRYBYTEXT	 8
#define SEGET_ENTRYBYDATA	 9
#define SEGET_ENTRYBYUSRFLG	10
#define SEGET_ENTRYBYSTATE	11
#define SEGET_ENTRYBYXSTATE	12
#define SEGET_LISTXYWH		13
#define SEGET_TAB		14
#define SEGET_USRFLAGS		15
#define SEGET_USRDATA		16
#define SEGET_NEXTENT		17
#define SEGET_PREVENT		18
#define SEGET_TEXTCPY		19
#define SEGET_TEXTCMP		20
#define SEGET_TEXTPTR		21
#define SEGET_CURRENT		22


/*
 * Structure used to control walking the scroll-list entries.
 */
#define ENT_VISIBLE	1	/* Only visible entries */
#define ENT_ISROOT	2	/* Start entry is 'root' */
#define ENT_INCROOT	4	/* If ENT_ISROOT, consider it too or just its 'children'? */
#define ENT_GOABOVE	8	/* Allow going up above level 0 */

struct se_arg_level
{
	short flags;		/* Flags - ENT_xxx, see above */
	short maxlevel;		/* Maximum descent level. If set to -1 descent is unlimited */
	short curlevel;		/* Set to start level, usually 0. */
};
/*
 * Structure used to control flags, as in an entries state, flags, xstate, xflag words.
 */
#define ANYBITS		0	/* Any bits will generate hit */
#define EXACTBITS	1	/* Only exact match generate hit */
struct se_arg_flags
{
	short method;
	short bits;
	short mask;
};

/*
 * structure used to pass parameters with get(SEGET_ENTRYBYxxx)
 */
#if 0
struct seget_entrybyarg
{
	int idx;
	struct scroll_entry *e;

	struct
	{
		union
		{
			struct se_arg_flags state;
			struct se_arg_flags xstate;
			char *txt;
			void *data;
			long usr_flag;
		}typ;
		struct se_arg_level level;
	}arg;
	
	union
	{
		long	ret;
		void	*ptr;
	}ret;
};
#endif
struct sesetget_params
{
	int idx;
	struct scroll_entry *e;
	struct scroll_entry *e1;
	
	struct se_arg_level level;

	union
	{
		struct se_arg_flags state;
		struct se_arg_flags xstate;
		char *txt;
		void *data;
		long usr_flag;
	}arg;
	
	union
	{
		long ret;
		void *ptr;
	}ret;
};

#define OS_OPENED	1
#define OS_NESTICON	2

#define OS_BOXED	OS_STATE08

#define OF_OPENABLE	1
#define OF_AUTO_OPEN	2

struct scroll_entry
{
	struct scroll_entry *list;	/* list we belong to */
	
	struct scroll_entry *root;
	struct scroll_entry *next;	/* Next element */
	struct scroll_entry *prev;	/* Previous element */
	struct scroll_entry *up;	/* level up entry (parent) */
	struct scroll_entry *down;	/* level down entry (childen) */
	short xstate;
	short xflags;
	short state;
	short level;
	short indent;
	short num_content;
// 	short icon_w, icon_h;
	RECT r;
	SCROLL_ENTRY_TYPE  type; 	/* type flags */
	SCROLL_ENTRY_FLAGS iflags;	/* internal flags */

	struct xa_wtxt_inf *fnt;
	struct xa_wcol_inf *col;
	struct se_content *content;
	long usr_flags;
	void *data;
	void (*data_destruct)(void *);
	
// 	struct scroll_entry_content c;
};
typedef struct scroll_entry SCROLL_ENTRY;

struct scroll_info
{
	enum locks lock;

	struct xa_window *wi;		/* make the scroll box a real window */
	struct xa_window *pw;		/* If the listbox is part of a windowed dialogue, we must know that,
					 * otherwise we couldnt move that window (rp_2_ap). */
	struct xa_vdi_settings *vdi_settings;

	SCROLL_INFO_FLAGS flags;
	XA_TREE *wt;
	short item;
	OBJECT prev_ob;			/* Original object contents */
	
	short indent_upto;
	short num_tabs;
	struct se_tab *tabs;
// 	struct se_text_tabulator *tabs;
	short rel_x, rel_y;

	SCROLL_ENTRY *start;		/* Pointer to first element */
	SCROLL_ENTRY *cur;		/*            current selected element */
	SCROLL_ENTRY *top;		/*            top-most displayed element */
	
	char *title;			/* Title of the list */

	short nesticn_w;
	short nesticn_h;

	short indent;
// 	short icon_w, icon_h;
	short widest, highest;		/* Width and hight of the widgest and highest element; */
	long start_x;
	long off_x;
	long start_y;
	long off_y;
	long total_w;
	long total_h;

	short state;			/* Extended status info for scroll list */

	scrl_click *dclick;		/* Callback function for double click behaviour */
	scrl_click *click;		/* Callback function for single click behaviour */
	scrl_click *click_nesticon;

	scrl_widget *slider;		/* slider calc function */
	scrl_widget *closer;		/* closer function */
	scrl_widget *fuller;		/* fuller function */

	scrl_redraw	*redraw;
	scrl_vis	*vis;			/* check visibility */
	scrl_search	*search;
	scrl_set	*set;
	scrl_get	*get;
	scrl_add	*add;
	scrl_del	*del;
	scrl_empty	*empty;
	scrl_list	*destroy;
	
	void	*data;

};
typedef struct scroll_info SCROLL_INFO;

/* SCROLL_INFO .status bits */
#define SCRLSTAT_UP	1		/* Scroll up is clicked on */
#define SCRLSTAT_DOWN	2		/* Scroll down is clicked on */
#define SCRLSTAT_SCROLL	4		/* Scroller drag is clicked on */
#define SCRLSTAT_RDB	8		/* Redraw buttons only */

typedef enum
{
	NO_MENU,
	IN_TITLE,
	IN_MENU,
	IN_DESK
} TASK_STAGE;

typedef enum
{
	NO_TASK,
	ROOT_MENU,
	MENU_BAR,
	POP_UP
} TASK_TY;

struct xa_popinfo
{
	struct xa_window *wind;
	XA_TREE *wt;
	
	short	parent;			/* Object index of popup parent */
	short	count;			/* Number of objects in popup */
	short	scrl_start_row;		/* Row in popup at which scrolling starts */
	short	scrl_start_obj;		/* Top scroll object (first object of scroll area) */
	short	scrl_curr_obj;
	short	scrl_height;
	short	*objs;

	short	current;		/* Currently selected, or pointed to, object */

	short	attach_parent;		/* Object number of object in parent popup that has 'this' popup attached */
	
	XA_TREE	*attach_wt;
	short	attach_item;
	short	attached_to;

	char	*save_start_txt;
	char	*save_end_txt;
	char	scrl_start_txt[16];
	char	scrl_end_txt[16];
};

struct xa_menuinfo
{
	struct xa_window *wind;
	XA_TREE	*wt;
	
	short	titles;
	short	popups;
	short	about;

	short	current;
};
	
struct menu_task
{
	TASK_STAGE stage;
	short ty;

	struct xa_menuinfo m;
	struct xa_popinfo  p;

	short clicks, x, y;
	RECT bar, drop;
	
	struct xa_mouse_rect em;
	void *Mpreserve;
	TASK *outof;
	TASK *entry;
	TASK *select;
	
	/* root displacements */
	short rdx, rdy;
	short pdx, pdy;
	short dx, dy;
};
typedef struct menu_task MENU_TASK;

struct task_administration_block
{
	LIST_ENTRY(task_administration_block) tab_entry;
	
	struct task_administration_block	*root;
	
	TASK_TY ty;	/* NO_TASK, ROOT_MENU, MENU_BAR, POP_UP... */

	enum locks lock;
	int locker;

	struct xa_client *client;
	struct xa_window *wind;
	XA_WIDGET *widg;

	TASK *task;	/* general task pointer */

	short exit_mb;
	short scroll;
	
	int usr_evnt;
	void *data;

	union
	{
		MENU_TASK menu;
	} task_data;

#if GENERATE_DIAGS
	int dbg;
	int dbg2;
	int dbg3;
#endif
};
typedef struct task_administration_block Tab;


/* 
 * definitions for form_alert.
 * The object tree was cloned, but NOT the texts.
 */
#define MAX_X 100
#define MAX_B 20
#define ALERT_LINES 6
#define ALERT_BUTTONS 3

struct alertxt
{
	char text  [ALERT_LINES  ][MAX_X+1];	/* Texts to display in the alert */
	char button[ALERT_BUTTONS][MAX_X+1];	/* Text for buttons (note: use MAX_X to get coercible row type) */
};
typedef struct alertxt ALERTXT;


/* Main client application descriptor */
struct xa_client
{
	LIST_ENTRY(xa_client) client_entry;
	LIST_ENTRY(xa_client) app_entry;

	struct xa_vdi_settings *vdi_settings;
	struct xa_module_widget_theme *xmwt;	/* Widget theme module */
	void *wtheme_handle;			/* Widget theme handle */
	struct xa_widget_theme *widget_theme;

#if GENERATE_DIAGS
	short enter;
#endif
	short  rppid;			/* Pid of our 'real' parent, the client that called shel_write() to give birth*/

	struct proc *p;			/* context back ptr */
	struct xa_user_things *ut;	/* trampoline code for user callbacks */
	struct proc *tp;		/* Thread */
	struct xa_wc_cache *wcc;	/* window_calc cache */

	short	swm_newmsg;

	bool forced_init_client;
	bool pexit;

	struct xa_aesmsg_list *msg;		/* Pending AES messages */
	struct xa_aesmsg_list *rdrw_msg;	/* WM_REDRAW messages */
	struct xa_aesmsg_list *crit_msg;	/* Critical AES messages - these are prioritized */
	struct xa_aesmsg_list *irdrw_msg;	/* Internal redraw messages */

	void	(*block)(struct xa_client *client, int which);

#define CS_LAGGING		0x00000001
#define CS_CE_REDRAW_SENT 	0x00000002
#define CS_FORM_ALERT		0x00000004
#define CS_FORM_DO		0x00000008
#define CS_WAIT_MENU		0x00000010
#define CS_FSEL_INPUT		0x00000020
#define CS_MISS_RDRW		0x00000040
#define CS_MENU_NAV		0x00000080
#define CS_BLOCK_MENU_NAV	0x00000100

#define CS_EXITING		0x00000200
#define CS_BLOCK_CE		0x00000400
#define CS_SIGKILLED		0x00000800

	long status;

	enum waiting_for waiting_for;	/* What types of event(s) the client is waiting for */
	AESPB *waiting_pb;		/* Parameter block for whatever the client is waiting for */
	short *waiting_short;		/* */

	short mouse;			/* The cursor to use when this is top application */
	short save_mouse;		/* The cursor saved by M_SAVE */
	short prev_mouse;		/* The cursor previous to any change - used by M_LAST/M_PREVIOUS */
	MFORM *mouse_form;
	MFORM *save_mouse_form;
	MFORM *prev_mouse_form;
	MFORM user_def;

	struct aes_global *globl_ptr;	/* Pointer to the client's globl array (so we can fill in the resource
					 * address field later). */

	RSHDR *rsrc;			/* Pointer to the client's standard GEM-style single resource file */
	OBJECT **trees;
	int rsct;			/* count up/down the loaded resources. Used by XA_alloc, XA_free */

	XA_TREE *wtlist;

	XA_TREE *std_menu;
	XA_TREE *nxt_menu;
	XA_TREE *desktop;

	XA_MENU_ATTACHMENT *attach;	/* submenus */

	Path home_path;			/* The directory that the client was started in */
	Path cmd_name;			/* The full filename used when launching the process (if launched by shell_write) */
	char *cmd_tail;			/* The command tail of the process (if launched by shell_write) */
	bool tail_is_heap;		/* If true, cmd_tail is a malloc */

	char name[NICE_NAME+2];		/* The clients 'pretty' name (possibly set by menu_register) */
	char proc_name[10];		/* The clients 'official' (ie. used by appl_find) name. */

	struct xa_window *alert;

	struct fmd fmd;			/* Data needed by the XaAES windowing of dialogues. */
	struct xa_client *nextclient;	/* Use for appl_find(APP_FIRST/APP_NEXT) */
	int  type;			/* What type of client is this? */

	char *half_screen_buffer;	/* for wind_get WF_SCREEN (people tend to use what is offered,
					 * whether the idee is good or not) */
	long half_screen_size;

	struct xa_mouse_rect em;	/* Needed. whether threads or not */
	long timer_val;			/* Pass it here, instead of awkward in the return value; */
	struct timeout *timeout;	/* remember timeout magic */
	struct xa_rscs *resources;	/* link loaded resoures' allocated memory, so it can be freeed.
					 * this also solves the problem that memory allocated for colour icon data
					 * was left orphaned. */

#if GENERATE_DIAGS
	char zen_name[NICE_NAME + 2 + 16];
#endif
	int xdrive;
	Path xpath;
	struct options options;		/* Individual AES options. */

	char	*mnu_clientlistname;	/* This holds the text of the menu-entry for client-list */
/*
 * This part is for Client event dispatching
*/
// 	struct	xa_rectlist_entry *lost_redraws;

#define MAX_KEYQUEUE	4
	int	kq_count;
	struct keyqueue *kq_head;
	struct keyqueue	*kq_tail;

#define CLIENT_MD_BUFFERS	10

	struct moose_data *md_head;
	struct moose_data *md_tail;
	struct moose_data *md_end;
	struct moose_data mdb[CLIENT_MD_BUFFERS+1];

	struct moose_data *wheel_md;

#define MAX_CEVENTS 15	/* Also used to mask ce_head/ce_tail */

#define XABT_NONE   0
#define XABT_SLEEP  1
#define XABT_SELECT 2
	int	blocktype;
	int	sleepqueue;
	long	sleeplock;
	
	int	usr_evnt;

	short	cevnt_count;
	struct	c_event *cevnt_head;
	struct	c_event *cevnt_tail;

	short	tp_term;
	short	tpcevnt_count;
	struct	c_event *tpcevnt_head;
	struct	c_event *tpcevnt_tail;

	struct	xa_data_hdr *xa_data;
};

#endif /* _xa_types_h */
