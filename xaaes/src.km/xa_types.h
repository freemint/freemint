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

/*
 * 'which' parameter for set_client_mouse() in xa_graf.c
 */
#define SCM_MAIN 0
#define SCM_PREV 1
#define SCM_SAVE 2


#define created_for_CLIENT		0x0000
/* All of the following flags indicate at least that a windows workarea is compleyely
   occupied by (part of) the dialoge root object. */
#define created_for_FMD_START	0x0001
#define created_for_FORM_DO		0x0002
#define created_for_POPUP		0x0004
#define created_for_WDIAL		0x0008
#define created_for_TOOLBAR		0x0010
#define created_for_SLIST		0x0020
#define created_for_AES			0x0100
#define created_for_ALERT		0x0200
#define created_for_CALC		0x0400
#define created_for_MENUBAR 	0x0800	/* don't draw anything */
typedef unsigned short WINDOW_TYPE;


enum xa_window_class
{
	WINCLASS_CLIENT = 0,
	WINCLASS_POPUP,
	WINCLASS_ALERT,
	WINCLASS_SLIST,
};
typedef enum xa_window_class WINDOW_CLASS;

#define XAWS_OPEN			0x00000001UL
#define XAWS_ICONIFIED		0x00000002UL
#define XAWS_CHGICONIF		0x00000004UL
#define XAWS_SHADED			0x00000008UL
#define XAWS_ZWSHADED		0x00000010UL
#define XAWS_HIDDEN			0x00000020UL
#define XAWS_FULLED			0x00000040UL
#define XAWS_NODELETE		0x00000080UL
#define XAWS_NOFOCUS		0x00000100UL
#define XAWS_STICKYFOCUS	0x00000200UL
#define XAWS_FLOAT			0x00000400UL
#define XAWS_SINK			0x00000800UL
#define XAWS_BINDFOCUS		0x00001000UL
#define XAWS_BELOWROOT		0x00002000UL
#define XAWS_FIRST			0x00004000UL
#define XAWS_RESIZED		0x00008000UL		/* if WM_SIZED to XaAES-(list-)window, evaluate in draw_object_tree() */
#define XAWS_RM_WDG			0x00010000UL
#define XAWS_SEMA			0x80000000UL
typedef unsigned long WINDOW_STATUS;

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

static inline bool obj_is_3d(OBJECT *ob)         { return (ob->ob_flags & OF_FL3DACT) != 0;	}
static inline bool obj_is_indicator(OBJECT *ob)  { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DIND; }
static inline bool obj_is_foreground(OBJECT *ob) { return (ob->ob_flags & OF_FL3DIND) != 0; }
static inline bool obj_is_background(OBJECT *ob) { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DBAK; }
static inline bool obj_is_activator(OBJECT *ob)  { return (ob->ob_flags & OF_FL3DACT) == OF_FL3DACT; }

static inline bool is_3d(short flags)         { return (flags & OF_FL3DACT) != 0;	}
static inline bool is_indicator(short flags)  { return (flags & OF_FL3DACT) == OF_FL3DIND; }
static inline bool is_foreground(short flags) { return (flags & OF_FL3DIND) != 0; }
static inline bool is_background(short flags) { return (flags & OF_FL3DACT) == OF_FL3DBAK; }
static inline bool is_activator(short flags)  { return (flags & OF_FL3DACT) == OF_FL3DACT; }

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
  short img_w;    /* Pixels per line (= (x + 7) / 8 Bytes)    */
  short img_h;    /* Total number of lines               */
  long  magic;    /* Contains "XIMG" if standard color   */
  short paltype;  /* palette type (0=RGB (short each)) */
};
typedef struct ximg_header XIMG_header;

struct xa_ximg_head
{
  struct ximg_header ximg;
  short *palette;	/* palette etc.                        */
  unsigned char *addr;     /* Address for the depacked bit-planes */
};
typedef struct xa_ximg_head XA_XIMG_HEAD;

typedef struct
{
	short	*control;   /**< aes_control[] array */
	short	*global;    /**< aes_global[]  array */
	short	*intin;     /**< aes_intin[]   array */
	short	*intout;    /**< aes_intout[]  array */
	long	*addrin;    /**< aes_addrin[]  array */
	long	*addrout;   /**< aes_addrout[] array */
	union msg_buf *msg;
} XAESPB;

struct xa_rsc_rgb
{
	short red;
	short green;
	short blue;
	short pen;
};

/* Rectangle List entry */
struct xa_rect_list
{
	struct xa_rect_list *next;
	/* Dimensions of segment */
	GRECT r;
};

struct xa_rectlist_entry
{
	struct xa_rect_list *start;
	struct xa_rect_list *next;
};
/*
 * Describing an AES object
 */
struct xa_aes_object
{
	OBJECT *ob;
	OBJECT *tree;
	short item;
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

struct menu_attachments;
typedef bool on_open_attach(struct menu_attachments *);

typedef struct menu_attachments
{
	struct menu_attachments *prev;
	struct menu_attachments *next;

	on_open_attach *on_open;

	void *data;

	struct widget_tree *wt;
	short menu;
	short item;
	struct widget_tree *to;
	short to_item;

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

struct xamenu
{
	struct widget_tree *wt;
	MENU menu;
	short mn_selected;	/* -1 means no selection,
				 * -2 means select first entry,
				 * positive value means object number to set selected
				 */
};
typedef struct xamenu XAMENU;

struct xamenu_result
{
	MENU menu;
	XA_MENU_ATTACHMENT *at;
};
typedef struct xamenu_result XAMENU_RESULT;

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
struct xa_colour_scheme
{
	short bg_col;			/* Colour used for backgrounds */
	short fg_col;			/* Colour used for foregrounds */
	short shadow_col;		/* Colour used for shadowed 3d edge */
	short lit_col;			/* Colour used for lit 3d edge */
	short border_col;		/* Colour used for edging */
	short highlight_col;		/* Colour used for highlighting */

	short font_fgcol;
	short font_bgcol;
};

struct xa_screen
{
	GRECT r;				/* Screen dimensions */

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
	short c_max_dist[8];
	short c_min_w, c_min_h;		/* Minimum (small font) character dimensions in pixels */
	short c_min_dist[8];

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
	short w, h;
	XAMFDB	*tl_corner;
	XAMFDB	*tr_corner;
	XAMFDB	*bl_corner;
	XAMFDB	*br_corner;
	XAMFDB	*left;
	XAMFDB	*right;
	XAMFDB	*top;
	XAMFDB	*bottom;
	XAMFDB	*body;
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
#if WITH_GRADIENTS
	struct	xa_gradient *gradient;
#endif
};

struct xa_wcol_inf
{
#define WCOL_ACT3D			0x0001	/* Draw 3D action when selected */
#define WCOL_DRAW3D			0x0002	/* Draw 3D borders */
#define WCOL_DRAWBKG		0x0004	/* Draw background */
#define WCOL_DRAWTEXTURE	0x0008	/* Draw Texture */
#define WCOL_ONLYTEXTURE	0x0010	/* Skip drawing background when texture exists */
#define WCOL_BOXED			0x0020	/* Draw box */
#define WCOL_REV3D			0x0040	/* Draw the bottom/left lines before top/right 3D borders */
#define WCOL_BOXBF3D		0x0080	/* Draw the BOX (if WCOL_BOXED) before 3D borders (if WCOL_DRAW3D) */
#define WCOL_GRADIENT		0x0100
#define WCOL_BOXRND			0x0200	/* Draw Rounded BOX */

	short	flags;
	short	wr_mode;	/* wrmode */
	struct xa_wcol normal; /* normal */
	struct xa_wcol selected; /* Selected */
	struct xa_wcol highlighted; /* Highlighted */
};

/*-----------------------------------------------------------------
 * Configuration and options structures
 *-----------------------------------------------------------------*/
#define ALTSC_ALERT	1
#define ALTSC_DIALOG	2
#define ALTSC_ONLY_PREDEF	4

struct options
{
	uchar clwtna;			/* Close Last Window Tops Next App:  */
	uchar alt_shortcuts;	/* add shortcuts 1: alert, 2: dialog, 3: both, 0: none */
	bool windowner;			/* display window owner in title. */
	bool nohide;
	bool xa_nohide;
	bool xa_nomove;
	bool noleft;			/* Dont move window out left side of screen. */
	bool thinwork;			/* workarea frame a single line. */
	bool nolive;			/* Live moving and sizing of windows. */
	bool wheel_reverse;		/* Do you want move the slider in the wheel direction,
					 * or rather the window content? */
	bool naes_ff;			/**/
	bool inhibit_hide;

	short thinframe;		/* -1: keep colour frame thin
					 * +1: make a thicker frame for wasier border grabbing and sizing. */
	short wheel_page;		/* how many wheel clicks for a page */
	short wheel_mode;
	short standard_font_point;	/* if != 0 set standard-font for application */
	short rsc_lang;		/* try to read strings for rsc from <rsc-basename>.rsl */
	short ignore_rsc_size;		/* ignore mismatch in actual rscfile-size and size from header */
#ifndef ST_ONLY
	char *icn_pal_name;       /* remap icons to palette */
#endif
	unsigned long wind_opts;			/* Default window options - see struct xa_window.opts */
	unsigned long app_opts;
	long half_screen;
	short submenu_indicator; /* if != 0 use '>' for attached submenus */

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
	} conin;
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
#define XAWAIT_MULTI	0x00010000UL	/* Waiting for an evnt_multi() */
#define XAWAIT_MOUSE	0x00020000UL	/* XaAES private; exclusive mouse event */
#define XAWAIT_MENU		0x00040000UL	/* XaAES private; menu rectangle event */
#define XAWAIT_NTO		0x00080000UL	/* XaAES private; timeout value NULL */

typedef struct task_administration_block Tab;
typedef Tab *TASK(Tab *tab, short item);

struct xa_mouse_rect
{
	short flags;
	short m1_flag;
	short m2_flag;
	GRECT m1, m2;
	TASK *t1, *t2;	/* used by tasking */
};

/*************************************************************************** */
/* The vdi api
*/

static inline long calc_back(const GRECT *r, short planes)
{
	return 2L * planes * ((r->g_w + 15) >> 4) * r->g_h;
}

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
typedef bool WidgetBehaviour	(int lock,
				 struct xa_window *wind,
				 struct xa_widget *widg,
				 const struct moose_data *md);

typedef bool WidgetKeyInput	(struct xa_window *wind,
				 struct xa_widget *widg,
				 const struct rawkey *key);
#if 0
typedef bool DisplayWidget(int lock, struct xa_window *wind,
			   struct xa_widget *widg);
#endif
typedef bool FormKeyInput(int lock,
			  struct xa_client *client,
			  struct xa_window *window,
			  struct widget_tree *wt,
			  const struct rawkey *key,
			  /* output */
			  struct fmd_result *res_fr);

typedef bool FormMouseInput(int lock,
			    struct xa_client *client,
			    struct xa_window *window,
			    struct widget_tree *wt,
			    const struct moose_data *md);

typedef void FormExit(struct xa_client *client,
		      struct xa_window *wind,
		      struct widget_tree *wt,
		      struct fmd_result *fr);


typedef int WindowKeypress(int lock, struct xa_window *wind,
			   struct widget_tree *wt,
			   unsigned short keycode, unsigned short nkcode, struct rawkey key);

/* Object display function type */
typedef void ObjectDisplay(int lock, struct widget_tree *wt, struct xa_vdi_settings *v);

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
	struct xa_data_hdr	*next;
	unsigned long		id;
	long			links;
	char			name[16];
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
	GRECT ofs;

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
	void  (*redraw)(int lock, struct xa_window *wind, struct xa_aes_object start, short depth, GRECT *r);

	struct xa_aes_object obj;
};

struct xa_usr_prn_settings;
struct xa_usr_prn_settings
{
	struct xa_data_hdr h;
	unsigned long flags;
	PRN_SETTINGS *settings;
};

struct xa_pdlg_drv_info;
struct xa_pdlg_drv_info
{
	struct xa_pdlg_drv_info *next;
	DRV_INFO		drv_info;
};

struct xa_prndrv_info;
struct xa_prndrv_info
{
	struct xa_prndrv_info *next;
#define DRVINFO_KMALLOC		1
	long flags;
	char path[128];
	char fname[32];
	char dname[128];
	DRV_INFO *d;
};

struct xa_pdlg_info
{
	struct xa_data_hdr h;

	void	*handle;
	struct	xa_client *client;
	struct	xa_window *wind;
	struct	widget_tree *mwt;
	struct	widget_tree *dwt;
	struct	scroll_info *list;

	short	min_w, min_h;

	short	exit_button;
	short	flags;
	short	dialog_flags;
	short	option_flags;

	short	n_drivers;
	short	n_printers;

	POPINFO drv_pinf;
	short	drv_obnum;
	struct widget_tree *drv_wt;

	POPINFO mode_pinf;
	short	mode_obnum;
	struct widget_tree *mode_wt;

	POPINFO color_pinf;
	short	color_obnum;
	struct widget_tree *color_wt;

	POPINFO dither_pinf;
	short	dither_obnum;
	struct widget_tree *dither_wt;

	POPINFO type_pinf;
	short	type_obnum;
	struct widget_tree *type_wt;

	POPINFO size_pinf;
	short	size_obnum;
	struct widget_tree *size_wt;

	POPINFO itray_pinf;
	short	itray_obnum;
	struct widget_tree *itray_wt;

	POPINFO otray_pinf;
	short	otray_obnum;
	struct widget_tree *otray_wt;

	POPINFO outfile_pinf;
	short	outfile_obnum;
	short	sel_outfile_obnum;
	struct widget_tree *outfile_wt;

	struct xa_prndrv_info *drivers;

	struct xa_pdrv_info *priv_drivers;

	short	nxt_subdlgid;
	PDLG_SUB *current_subdlg;

	PRN_SETTINGS *settings;

	DRV_INFO *curr_drv;
	PRN_ENTRY *curr_prn;
	PRN_MODE *curr_mode;
	DITHER_MODE *curr_dither;
	MEDIA_TYPE *curr_type;
	MEDIA_SIZE *curr_size;
	PRN_TRAY *curr_itray;
	PRN_TRAY *curr_otray;

	char *outfile;
	char *outpath;

	char *outfiles[(PDLG_OUTFILES + 1) * 2];

	short n_colmodes;
	long colormodes[32];

	PRN_SETTINGS current_settings;
	PRN_SETTINGS default_settings;

	char document_name[256];
	char filesel[256];
};

#define OB_CURS_EOR	1
#define OB_CURS_ENABLED	2
#define OB_CURS_DRAWN	4
struct objc_edit_info
{
	TEDINFO *p_ti;

	struct xa_aes_object o;	/* Index of editable object */

	short pos;	/* Cursor (char) position, relative to edstart */
	short c_state;	/* Cursor state */

	short edstart;	/* Offset into formatted string where first editable char begins */
	short m_start;
	short m_end;
	unsigned short t_offset;
	unsigned short p_offset;
	GRECT cr;	/* Cursor coordinates, relative */
	TEDINFO ti;
};
typedef struct objc_edit_info XTEDINFO;

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
#define WTF_FBDO_SLIST	0x00000040	/* Form_Button() handles SLIST objects */
#define WTF_OBJCEDIT	0x00000080
#define WTF_EXTRA_ISLIST 0x00000100	/* extra points to a list */
	unsigned long	flags;

#define WTR_POPUP	0x00000001
#define WTR_ROOTMENU	0x00000002

	unsigned long	rend_flags;

	struct xa_window *wind;		/* Not of any specific use just yet...*/
	struct xa_client *owner;	/* The tree widget would be owned by a different app to
					 * the actual window (like the menu bar on the root window),
					 * or the desktop toolbar widget. HR 270801: was: int pid; */
	struct xa_widget *widg;		/* Cross pointer to widget. */

	struct object_render_api *objcr_api;
	void *objcr_theme;		/* object renderer private data */
	void *objcr_data;		/* object renderer private ptr */

	OBJECT *tree;			/* The object tree */
	struct xa_aes_object current;
	GRECT r;				/* Why not do the addition (parent_x+ob_x) once in the caller? */
					/* And set a usefull GRECT as soon as possible, ready for use in
					 * all kind of functions. */
	GRECT area;
	ushort *state_mask;

	GRECT r_parent;
#if 0
	short parent_x;			/* Keep both in: dont need to change everything in a single effort */
	short parent_y;
#endif

	struct xa_aes_object focus;

	struct objc_edit_info *ei;
	struct objc_edit_info e;

	short lastob;			/* Can be used to validate item number */
	short which;			/* kind of event for use by WDIAL exit handler. */

	short rdx, rdy;
	short pdx, pdy;
	short pop;
	bool is_menu;			/* true when the tree is a menu or part of it. */
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
#define FMDF_EDIT	1

struct fmd_result
{
	bool no_exit;
	short flags;
	struct xa_aes_object obj;
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
	GRECT r;				/* The rectangle for the postponed dialogue window */
};

#define CEVNT_MOVEMENT	1
#define CEVNT_BUTTON	2
#define CEVNT_MENUSTART 4

struct c_event
{
	struct c_event		*next;
	void			(*funct)(int, struct c_event *, short cancel);
	struct xa_client	*client;
	void			*ptr1;
	void			*ptr2;
	int			d0;
	int			d1;
	GRECT			r;
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

typedef unsigned long AES_function(int lock, struct xa_client *client, AESPB *pb);

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
#define XAR_PM			6	/* Placement mask */
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
	XAW_WCONTEXT,			/* -- new -- */
	XAW_WAPPICN,			/* -- new -- */
	XAW_CLOSE,
	XAW_FULL,
	XAW_INFO,
	XAW_RESIZE,
	XAW_UPLN,			/* 8 (was 6) */
	XAW_UPLN1,			/* -- new -- */
	XAW_DNLN,			/* 10 (was 7) */
	XAW_VSLIDE,
	XAW_LFLN,			/* 12 (was 9) */
	XAW_LFLN1,			/* -- new -- */
	XAW_RTLN,			/* 14 (was 10) */
	XAW_HSLIDE,
	XAW_ICONIFY,
	XAW_HIDE,			/* 17 (was 13) */

 /*
  * The widget types above this comment MUST be context indipendant.
  * The widget types blow this comment are considered context-dependant
  * and must be processed via client events.
  * Furthermore, remember to change XA_MAX_CF_WIDGETS below if you
  * put insert a new context dependant widget before XAW_TOOLBAR!
  */
	XAW_TOOLBAR,			/* 18 ( was 14) Extended XaAES widget */
	XAW_MENU,			/* 19 ( was 15) Extended XaAES widget, must be drawn last. */

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

typedef bool _cdecl DrawWidg(struct xa_window *wind, struct xa_widget *widg, const GRECT *clip);
typedef void _cdecl DrawCanvas(struct xa_window *wind, GRECT *outer, GRECT *inner, const GRECT *clip);
typedef void _cdecl SetWidgSize(struct xa_window *wind, struct xa_widget *widg);
typedef void _cdecl WidgGemColor(struct xa_window *wind, short gem_widget, BFOBSPEC *c);
typedef void _cdecl DrawFrame(struct xa_window *wind, const GRECT *clip);
typedef void _cdecl FreePriv(struct xa_window *wind, struct xa_widget *widg);

struct render_widget
{
	XA_WIND_ATTR	tp;
	XA_WIND_ATTR	*tp_depends;
	short		xaw_idx;
	short		pos_in_row;
	DrawWidg	*draw;		/* Function called to draw the widget */
	SetWidgSize	*setsize;	/* Function called to get widget dimensions */
	FreePriv	*freepriv;	/* Function called to release any widget render resources */
	long		priv[4];	/* wdiget render on a 'per widget' private data */
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
	GRECT r;
	short rxy;
};

struct widget_theme
{
	struct xa_data_hdr	h;
	long			links;

	struct nwidget_row	*layout;

	WidgGemColor		*get_widgcolor;
	WidgGemColor		*set_widgcolor;

	GRECT			outer;
	GRECT			inner;
	DrawCanvas		*draw_canvas;

	DrawFrame		*draw_waframe;

	struct render_widget	exterior;

	struct render_widget	border;
	struct render_widget	title;
	struct render_widget	wcontext;
	struct render_widget	wappicn;
	struct render_widget	closer;
	struct render_widget	fuller;
	struct render_widget	info;
	struct render_widget	sizer;
	struct render_widget	uparrow;
	struct render_widget	uparrow1;
	struct render_widget	dnarrow;
	struct render_widget	vslide;
	struct render_widget	lfarrow;
	struct render_widget	lfarrow1;
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
	struct widget_theme *client;
	struct widget_theme *popup;
	struct widget_theme *alert;
	struct widget_theme *slist;
};

struct xa_module_api;
struct xa_module_widget_theme
{
	char		*sname;
	char		*lname;

	void * _cdecl	(*init_module)(const struct xa_module_api *, const struct xa_screen *screen, char *widg_name, bool grads);
	void _cdecl	(*exit_module)(void *module);

	long _cdecl	(*new_theme)(void *module, short win_type, struct widget_theme **);
	long _cdecl	(*free_theme)(void *module, struct widget_theme **);

	long _cdecl	(*new_color_theme)(void *module, short win_type, void **ontop, void **untop);
	void _cdecl	(*free_color_theme)(void *module, void *ctheme);
};
/* ****************************************************************************************** */
/* ****************************************************************************************** */
/* ****************************************************************************************** */
/* ****************************************************************************************** */
typedef void _cdecl DrawObject(struct widget_tree *wt, struct xa_vdi_settings *v);
struct object_render_api
{
	struct xa_data_hdr	h;

	short _cdecl (*objc_sysvar)	(void *theme, short mode, short what, short *val1, short *val2, short *val3, short *val4);
	void  _cdecl (*obj_offsets)	(struct widget_tree *wt, OBJECT *ob, GRECT *c);
	short _cdecl (*obj_thickness)	(struct widget_tree *wt, OBJECT *ob);
	short _cdecl (*obj_is_transp)	(struct widget_tree *wt, OBJECT *ob, bool progdef_is_trans);

	void _cdecl  (*write_menu_line)	(struct xa_vdi_settings *v, GRECT *r);
	void _cdecl  (*g2d_box)		(struct xa_vdi_settings *v, short b, GRECT *r, short colour);
	void _cdecl  (*draw_2d_box)	(struct xa_vdi_settings *v, short x, short y, short w, short h, short border_thick, short colour);
	void _cdecl  (*write_selection)	(struct xa_vdi_settings *v, short d, GRECT *r);

	void _cdecl  (*set_cursor)	(struct widget_tree *wt, struct xa_vdi_settings *v, struct objc_edit_info *ei);
	void _cdecl  (*eor_cursor)	(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl);
	void _cdecl  (*draw_cursor)	(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl, short flags);
	void _cdecl  (*undraw_cursor)	(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl, short flags);

	DrawObject	*drawers[256];
};

struct xa_module_object_render
{
	char	*sname;
	char	*lname;

	void * _cdecl	(*init_module)(const struct xa_module_api *, const struct xa_screen *, bool grads);
	long   _cdecl	(*exit_module)(void);

	long _cdecl	(*open)	(struct object_render_api **ptr_api);
	long _cdecl	(*close)(struct object_render_api *api);

	long _cdecl	(*new_theme)(void **ptr_theme);
	long _cdecl	(*free_theme)(void *theme);

};


struct xa_widget_methods
{
	short		properties;
	WINDOW_STATUS	statusmask;

	struct render_widget	r;

#define WIP_NOTEXT	0x0001		/* Widget is NOT part of window exterior, will not be automatically redrawn */
#define WIP_WACLIP	0x0002		/* Widget is part of, and will be clipped by, windows work area */
/* WIP_ACTIVE - If this bit is set, widget is clickable, else is just to draw the
 * corresponding part of window exterior and clicks on it is ignored
 */
#define WIP_ACTIVE	0x0004
#define WIP_INSTALLED	0x0008
/* WIP_NODRAG - When set, 'click' will be called immediately.
 * When clear, 'drag' will be called if widget has one, else watch_object()
 * is done and 'click' called upon released button if pointer still inside
 */
#define WIP_NODRAG	0x0010

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

typedef struct xa_slider_widget XA_SLIDER_WIDGET;

/* Window Widget */
struct xa_widget
{
	struct xa_widget	*next_in_row;

	struct xa_window	*wind;		/* Window to which this widget belongs */
	struct xa_client	*owner;
	struct xa_widget_methods m;

	short		state;		/* Current status (selected, etc) */

	GRECT		r;		/* Relative position */
	GRECT		ar;		/* Absolute position */
	GRECT		prevr;		/* Prevsioiu position - free to use for modules */

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
	union {
		XA_TREE *wt;
		XA_SLIDER_WIDGET *sl;
		/* const */ char *name;
		void *ptr;
	} stuff;

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
	GRECT d;				/* distance information */
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
	GRECT r;				/* physical */
};

#define ZT_A	1
#define ZT_B	2
#define ZT_C	3
#define ZT_D	4


#define XAAO_WF_SLIDE	((long)AO0_WF_SLIDE << 16)
#define XAAO_OBJC_EDIT	((long)AO0_OBJC_EDIT << 16)
#define XAAO_SUPPORTED	(XAAO_WF_SLIDE|XAAO_OBJC_EDIT)

/* Callback for a window's auto-redraw function */
typedef int WindowDisplay (int lock, struct xa_window *wind);

typedef void DrawWinElement(struct xa_window *wind);

#define XAWO_WHEEL	((long)WO0_WHEEL << 16)
#define XAWO_FULLREDRAW ((long)WO0_FULLREDRAW << 16)
#define XAWO_NOBLITW	((long)WO0_NOBLITW << 16)
#define XAWO_NOBLITH	((long)WO0_NOBLITH << 16)
#define XAWO_SENDREPOS	((long)WO0_SENDREPOS << 16)
#define XAWO_WCOWORK	((long)WO0_WCOWORK << 16)

#define XAWO_SUPPORTED	(XAWO_WHEEL|XAWO_FULLREDRAW|XAWO_NOBLITW|XAWO_NOBLITH|XAWO_SENDREPOS|XAWO_WCOWORK)

struct xa_wc_cache;
struct xa_wc_cache
{
	struct xa_wc_cache *next;
	short	class;
	XA_WIND_ATTR tp;
	GRECT delta;
	GRECT wadelta;
};

/* Window Descriptor */
struct xa_window
{
	struct xa_window	*next;	/* Window list stuff - next is the window below */
	struct xa_window	*prev;	/*		     - prev is the window above (both NULL terminated) */
	struct xa_window	*parent;
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
	GRECT delta;
	GRECT wadelta;
	GRECT save_delta;
	GRECT save_wadelta;

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

	long rect_lock;
	unsigned long opts;			/* Window options. XAWO_xxx */
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
	GRECT max;			/* Creator dimension's, maximum for sizing */
	GRECT min;
	GRECT r;				/* Current dimensions */
	GRECT rc;			/* Real coordinates when shaded */
	GRECT ro;			/* Original dimemsions when iconified */
	GRECT wa;			/* user work area */
	GRECT rwa;			/* work area minus toolbar, if installed - else same as wa */
	GRECT bd;			/* border displacement */
	GRECT rbd;
	GRECT ba;			/* border area for use by border sizing facility. */
	GRECT pr;			/* previous dimensions */
	GRECT t;				/* Temporary coordinates used internally */

	GRECT outer;
	GRECT inner;

	short sw;			/* define middle of window (default 2 -> 1/2), used for resizing */
	short sh;			/* height to use when SHADED */
	short hx, hy;

	short handle;			/* Window handle */
	short frame;			/* Size of the frame (0 for windowed listboxes) */

	struct xa_rectlist_entry rect_list;
	struct xa_rectlist_entry rect_user;
	struct xa_rectlist_entry rect_opt;
	struct xa_rectlist_entry rect_toolbar;

	bool use_rlc;
	GRECT rl_clip;

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

extern struct xa_window *root_window;
#define window_list S.open_windows.first
#define TOP_WINDOW S.open_windows.top
#define nolist_list S.open_nlwindows.first

struct scroll_info;

/* Directory entry flags */
#define SETYP_STATIC	0x8000
typedef unsigned short SCROLL_ENTRY_TYPE;

#define SEF_WTXTALLOC 0x0001
#define SEF_WCOLALLOC 0x0002
typedef unsigned short SCROLL_ENTRY_FLAGS;

#define SIF_KMALLOC		0x0001
#define SIF_SELECTABLE	0x0002
#define SIF_ICONINDENT	0x0004
#define SIF_MULTISELECT 0x0008
#define SIF_MULTIBOXED  0x0010
#define SIF_AUTOSLIDERS 0x0020
#define SIF_AUTOSELECT  0x0040
#define SIF_TREEVIEW	0x0080
#define SIF_AUTOOPEN	0x0100
#define SIF_KEYBDACT	0x0200
#define SIF_DIRTY		0x0400
#define SIF_INLINE_EFFECTS	0x0800
#define SIF_NO_ICONS	0x1000
#define SIF_ICONS_HAVE_NO_TEXT 0x2000
typedef unsigned short SCROLL_INFO_FLAGS;

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
	GRECT v;

	GRECT r;
	short widest;
	short highest;
};

#define SETEXT_TXTSTR	1
#define SETEXT_ALLOC	2
struct se_icon
{
	GRECT r;
	OBJECT *icon;
};
struct se_text;
struct se_text
{
	short flags;
	unsigned short tblen, slen;
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
typedef unsigned short	scrl_keybd	(struct scroll_info *list, unsigned short kc, unsigned short ks);
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
#define SESET_ACTIVE		15

#define SESET_M_STATE		16

#define SESET_PRNTWIND		17

/* SESET_UNSELECTED arguments */
#define UNSELECT_ONE	0
#define UNSELECT_ALL	1

struct seset_tab
{
	short index;
	short flags;
	GRECT r;
};

/* scrl_get modes */
#define SEGET_STATE		 0
#define SEGET_XSTATE		 1
#define SEGET_NFNT		 2
#define SEGET_SFNT		 3
#define SEGET_HFNT		 4
#define SEGET_WTXT		 5
#define SEGET_SELECTED		 6

#define SEGET_ENTRYBYIDX	10
#define SEGET_ENTRYBYTEXT	11
#define SEGET_ENTRYBYDATA	12
#define SEGET_ENTRYBYUSRFLG	13
#define SEGET_ENTRYBYSTATE	14
#define SEGET_ENTRYBYXSTATE	15
#define SEGET_ENTRYBYUSRFLAGS	16

#define SEGET_LISTXYWH		20
#define SEGET_PRNTWIND		21
#define SEGET_TAB		30

#define SEGET_USRFLAGS		40
#define SEGET_USRDATA		41
#define SEGET_NEXTENT		50
#define SEGET_PREVENT		51
#define SEGET_TEXTCPY		60
#define SEGET_TEXTCMP		61
#define SEGET_TEXTPTR		62
#define SEGET_CURRENT		100


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
	long bits;
	long mask;
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
		struct se_arg_flags usr_flags;
		char *txt;
		void *data;
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
	GRECT r;
	SCROLL_ENTRY_TYPE  type; 	/* type flags */
	SCROLL_ENTRY_FLAGS iflags;	/* internal flags */

	struct xa_wtxt_inf *fnt;
	struct xa_wcol_inf *col;
	struct se_content *content;
	long usr_flags;
	void *data;
	void (*data_destruct)(void *);
};
typedef struct scroll_entry SCROLL_ENTRY;

struct scroll_info
{
	struct scroll_info *next;	/* lists in one window linked */
	int lock;

	struct xa_window *wi;		/* make the scroll box a real window */
	struct xa_window *pw;		/* If the listbox is part of a windowed dialogue, we must know that,
					 * otherwise we couldnt move that window (rp_2_ap). */
	struct xa_vdi_settings *vdi_settings;
	struct widget_tree *nil_wt;

	SCROLL_INFO_FLAGS flags;
	XA_TREE *wt;
	short item;
	OBJECT prev_ob;			/* Original object contents */

	short indent_upto;
	short num_tabs;
	struct se_tab *tabs;
	short rel_x, rel_y;

	SCROLL_ENTRY *start;		/* Pointer to first element */
	SCROLL_ENTRY *cur;		/*            current selected element */
	SCROLL_ENTRY *top;		/*            top-most displayed element */

	char *title;			/* Title of the list */

	short nesticn_w;
	short nesticn_h;
	short icon_w;		/* width of icon: for display list-elements with and without icons */

	short indent;
	short widest, highest;		/* Width and hight of the widgest and highest element; */
	long start_x;
	long off_x;
	long start_y;
	long off_y;
	long total_w;
	long total_h;

	short char_width;		/* remember char-width for this list */

	scrl_click *dclick;		/* Callback function for double click behaviour */
	scrl_click *click;		/* Callback function for single click behaviour */
	scrl_click *click_nesticon;

	scrl_keybd *keypress;

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

	XA_MENU_ATTACHMENT *at_up;
	XA_MENU_ATTACHMENT *at_down;

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

	unsigned long actflags;

	struct xa_menuinfo m;
	struct xa_popinfo  p;

	short attach_select;

	short clicks, x, y;
	GRECT bar, drop;

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

	int lock;
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


/*
 * definitions for form_alert.
 * The object tree was cloned, but NOT the texts.
 */
#define MAX_X 100
#define MAX_B 20
#define ALERT_LINES 6
#define ALERT_BUTTONS 4

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

	struct xa_module_object_render *objcr_module;
	struct object_render_api *objcr_api;
	void *objcr_theme;

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
	struct xa_aesmsg_list *lost_rdrw_msg;
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

#define CS_NO_SCRNLOCK		0x00001000
#define CS_CLIENT_EXIT		0x00002000
#define CS_CALLED_EVNT    0x00004000
#define CS_USES_ABOUT     0x00008000


	long status;

	unsigned long waiting_for;	/* What types of event(s) the client is waiting for */
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
	Path start_path;		/* The directory that the started binary lives */
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

	int xdrive;
	Path xpath;
	struct options options;		/* Individual AES options. */

	char	*mnu_clientlistname;	/* This holds the text of the menu-entry for client-list */
/*
 * This part is for Client event dispatching
*/

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

#define HTDNAME "helptdata"
struct helpthread_data
{
	struct xa_data_hdr h;

	struct xa_window *w_about;
	struct xa_window *w_view;
	struct xa_window *w_taskman;
	struct xa_window *w_sysalrt;
	struct xa_window *w_reschg;
	struct xa_window *w_csr;

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
	GRECT r;
	GRECT clip;
};

struct oblink_spec
{
	struct xa_aes_object to;
	GRECT save_to_r;
	struct xa_aes_object from;
	OBJECT save_obj;
	struct xa_aes_object savestop;
	union
	{
		short	 smisc[10];
		long	 lmisc[5];
		void	*pmisc[5];
	} d;
};

/* ------------------------------------------------  */

inline static bool
is_tree_item_aesobj(struct xa_aes_object *o, OBJECT *tree, short item)
{
	return ((tree + item) == o->ob) ? true : false;
}

inline static bool
same_aesobj(struct xa_aes_object *a, struct xa_aes_object *b)
{
	return (a->ob == b->ob) ? true : false;
}
inline static bool
valid_aesobj(struct xa_aes_object *o)
{
	if (o->item < 0 || !o->tree)
		return false;
	return true;
}

inline static short	aesobj_item	(struct xa_aes_object *o) { return o->item; }
inline static OBJECT *	aesobj_ob	(struct xa_aes_object *o) { return o->ob; }
inline static OBJECT *	aesobj_tree	(struct xa_aes_object *o) { return o->tree; }

inline static void	aesobj_settype	(struct xa_aes_object *o, short t) { o->ob->ob_type = t; }
inline static short	aesobj_type	(struct xa_aes_object *o) { return   o->ob->ob_type; }

inline static void	aesobj_setflags	(struct xa_aes_object *o, short f) { o->ob->ob_flags = f; }
inline static short	aesobj_flags	(struct xa_aes_object *o) { return   o->ob->ob_flags; }

inline static void	aesobj_setstate	(struct xa_aes_object *o, short s) { o->ob->ob_state = s; }
inline static short	aesobj_state	(struct xa_aes_object *o) { return   o->ob->ob_state; }

inline static short	aesobj_head	(struct xa_aes_object *o) { return o->ob->ob_head; }
inline static short	aesobj_tail	(struct xa_aes_object *o) { return o->ob->ob_tail; }
inline static short	aesobj_next	(struct xa_aes_object *o) { return o->ob->ob_next; }

inline static short	aesobj_getx (struct xa_aes_object *o) { return o->ob->ob_x; }
inline static short	aesobj_gety (struct xa_aes_object *o) { return o->ob->ob_y; }
inline static short	aesobj_getw (struct xa_aes_object *o) { return o->ob->ob_width; }
inline static short	aesobj_geth (struct xa_aes_object *o) { return o->ob->ob_height; }
inline static void	aesobj_setx (struct xa_aes_object *o, short c) { o->ob->ob_x = c; }
inline static void	aesobj_sety (struct xa_aes_object *o, short c) { o->ob->ob_y = c; }
inline static void	aesobj_setw (struct xa_aes_object *o, short c) { o->ob->ob_width = c; }
inline static void	aesobj_seth (struct xa_aes_object *o, short c) { o->ob->ob_height = c; }

inline static void	aesobj_sethidden(struct xa_aes_object *o) { o->ob->ob_flags |=  OF_HIDETREE; }
inline static void	aesobj_clrhidden(struct xa_aes_object *o) { o->ob->ob_flags &= ~OF_HIDETREE; }
inline static bool	aesobj_hidden(struct xa_aes_object *o) { return (o->ob->ob_flags & OF_HIDETREE); }

inline static void	aesobj_setedit(struct xa_aes_object *o) { o->ob->ob_flags |=  OF_EDITABLE; }
inline static void	aesobj_clredit(struct xa_aes_object *o) { o->ob->ob_flags &= ~OF_EDITABLE; }
inline static bool	aesobj_edit(struct xa_aes_object *o) { return (o->ob->ob_flags & OF_EDITABLE); }

inline static void	aesobj_setsel(struct xa_aes_object *o) { o->ob->ob_state |=  OS_SELECTED; }
inline static void	aesobj_clrsel(struct xa_aes_object *o) { o->ob->ob_state &= ~OS_SELECTED; }
inline static bool	aesobj_sel(struct xa_aes_object *o) { return (o->ob->ob_state & OS_SELECTED); }

inline static void	aesobj_setdisabled (struct xa_aes_object *o) { o->ob->ob_state |=  OS_DISABLED; }
inline static void	aesobj_clrdisabled (struct xa_aes_object *o) { o->ob->ob_state &= ~OS_DISABLED; }
inline static bool	aesobj_disabled	   (struct xa_aes_object *o) { return (o->ob->ob_state & OS_DISABLED); }

inline static struct xa_aes_object
inv_aesobj(void)
{
	return (struct xa_aes_object){ob: NULL, tree: NULL, item: -1};
}

inline static struct xa_aes_object
aesobj(OBJECT *tree, short item)
{
	return (struct xa_aes_object){ob: tree + item, tree: tree, item: item};
}

inline static bool
is_aesobj(OBJECT *tree, short item, struct xa_aes_object *o)
{
	return ((tree + item) == aesobj_ob(o));
}
/*
 * Focus object
 */
inline static void
clear_focus(struct widget_tree *wt)
{
	wt->focus = inv_aesobj();
}
inline static void
set_focus(struct widget_tree *wt, OBJECT *tree, short item)
{
	wt->focus = aesobj(tree, item);
}

inline static bool
focus_set(struct widget_tree *wt)
{
	return (wt->focus.ob) ? true : false;
}
inline static short
focus_item(struct widget_tree *wt)
{
	return wt->focus.item;
}
inline static OBJECT *
focus_ob(struct widget_tree *wt)
{
	return wt->focus.ob;
}

inline static bool
obj_is_focus(struct widget_tree *wt, struct xa_aes_object *o)
{
	return same_aesobj(&wt->focus, o);
}

inline static struct xa_aes_object
focus(struct widget_tree *wt)
{
	return wt->focus;
}

/*
 * Edit object
 */
inline static void
clear_edit(struct objc_edit_info *ei)
{
	ei->o = inv_aesobj();
}
inline static void
set_edit(struct objc_edit_info *ei, OBJECT *tree, short item)
{
	ei->o = aesobj(tree, item);
}

inline static bool
edit_set(struct objc_edit_info *ei)
{
	return valid_aesobj(&ei->o);
}
inline static short
edit_item(struct objc_edit_info *ei)
{
	return ei->o.item;
}
inline static OBJECT *
edit_ob(struct objc_edit_info *ei)
{
	return ei->o.ob;
}

inline static bool
obj_is_edit(struct objc_edit_info *ei, struct xa_aes_object *o)
{
	return same_aesobj(&ei->o, o);
}

inline static struct xa_aes_object
editfocus(struct objc_edit_info *ei)
{
	return ei->o;
}

typedef void kernkey_action(int lock, struct xa_client *client, short open);

struct kernkey_entry
{
	struct kernkey_entry *next_key;	/* Next registered key */
	struct kernkey_entry *next_act; /* Next registered action for this key */
	short key;			/* Key */
	short state;			/* shiftkey state */
	kernkey_action *act;		/* action */
};

/*
 * GLOBAL VARIABLES AND DATA STRUCTURES
 */

/* open and closed windows in separate lists. */
struct win_base
{
	struct xa_window *first;
	struct xa_window *last;
	struct xa_window *top;
	struct xa_window *root;
};

struct shared
{
	struct win_base open_windows;		/* list of all open windows */
	struct win_base closed_windows;		/* list of all closed windows */
	struct win_base hidden_windows;
	struct win_base deleted_windows;	/* list of windows to be deleted (delayed action) */

	struct win_base open_nlwindows;
	struct win_base closed_nlwindows;
	struct win_base calc_windows;		/* list of open nolist windows - fmd, alerts, etc. */

	struct xa_window *focus;

	LIST_HEAD(xa_client) client_list;
	LIST_HEAD(xa_client) app_list;

	LIST_HEAD(task_administration_block) menu_base;

	TIMEOUT *menuscroll_timeout;

	TIMEOUT *popin_timeout;
	struct xa_client *popin_timeout_ce;
	TIMEOUT *popout_timeout;
	struct xa_client *popout_timeout_ce;

	struct xa_client *wait_mouse;	/* This client need mouse events exclusivly */
	struct opt_list *app_options;	/* individual option settings. */

	short wm_count;			/* XXX ??? */
	short clients_exiting;		/* Increased by exit_client() upon enry and decreased upon exit
					 * used to prevent interrupt-handling during shutdown of a client
					 */
};

struct shel_info
{
	short type;
	short rppid;		/* Real parent pid, that is, the pid of app that called of shel_write() */
	short reserved;
	bool shel_write;

	char *cmd_tail;
	bool tail_is_heap;

	Path cmd_name;
	Path home_path;
};

struct common
{
	Path	start_path;
	unsigned short nvdi_version;
	unsigned short fvdi_version;
	bool f_phys;	/* using physical wk */
	unsigned long gdos_version;

	void (*reschange)(int lock, struct xa_client *client, bool open);
	struct kernkey_entry *kernkeys;

	short AESpid;			/* The AES's MiNT process ID */
	short DSKpid;			/* The desktop programs pid, if any */
	short SingleTaskPid;	/* The pid of the single-task-client, if any */
	short rdm;        /* "dirty" menubar */

	short P_handle;			/* Physical workstation handle used by the AES */
	short global_clip[4];
	short prev_clip[4];

	struct xa_client *Aes;		/* */
	struct xa_client *Hlp;
	void 	*Hlp_pb;
	unsigned long	Aes_waiting_for;

	short move_block;		/* 0 = movement allowed
					 * 1 = internal movement cevent sent to client - no move
					 * 2 = WM_MOVED AES message sent to client - no move
					 * 3 = client did a wind_set(WF_CURRXYWH) and we got WM_REDRAWS
					 *     in the queue. 'move_block' is then reset when all WM_REDRAWS
					 *     are serviced
					 */
	short rect_lock;
	long redraws;			/* Counting WM_REDRAWS being sent and dispatched */
	struct xa_client *button_waiter;/* Client which is getting the next moose_data packet, */
					/* most probably a button released one */
	struct xa_client *ce_open_menu;	/* If set, this client has been sent a open_menu client event */
	struct xa_client *ce_menu_move; /* If set, this client has been sent a menu_move client event */
	struct xa_client *ce_menu_click;

	struct xa_client *next_menu;
	struct widget_tree *next_menu_wt;

	struct xa_client *csr_client;	/* Client current in query by the Kill or Wait dialog */

	short shutdown;			/* flags for shutting down xaaes */
#define SHUTDOWN_STARTED	0x0001
#define SHUTTING_DOWN		0x0002
#define QUIT_NOW		0x0004		/* - enter shutdown the next possible time */
#define HALT_SYSTEM		0x0008		/* - halt system after xaaes shutdown */
#define REBOOT_SYSTEM		0x0010		/* - reboot system after xaaes shutdown */
#define COLDSTART_SYSTEM	0x0020		/* - cold reboot */
#define RESOLUTION_CHANGE	0x0040
#define KILLEM_ALL		0x0080
#define RESTART_XAAES	0x0100
#define RESTART_AFTER_BOMB	0x0400
#define EXIT_MAINLOOP		0x8000
	short shutdown_step;
	struct timeout *sdt;		/* Shutdown Timeout */

	long alert_pipe;		/* AESSYS: The MiNT Salert() pipe's file handle */
	long KBD_dev;			/* AESSYS: The MiNT keyboard device's file handle */

	GRECT iconify;			/* Positioning information for iconifying windows */
	void *Aes_rsc;			/* Pointer to the XaAES resources */
	char *env;			/* new environment */

#ifndef ST_ONLY
	/* icon-palette */
	short is_init_icn_pal;
	struct rgb_1000 *icn_pal;
#endif
	struct xa_window *hover_wind;
	struct xa_widget *hover_widg;

	struct proc *update_lock;
	struct proc *mouse_lock;
	struct proc *menu_lock;

	short updatelock_count;
	short mouselock_count;
	short menulock_count;

	Path desk;			/* Remember the desk path for Launch desk. */
	short mouse;			/* Remember mouse shape */
	MFORM *mouse_form;		/* Remember mouse form */
	struct xa_client *mouse_owner;

	short aesmouse;
	MFORM *aesmouse_form;

	short realmouse;
	MFORM *realmouse_form;
	struct xa_client *realmouse_owner;

	struct xa_client	*do_widget_repeat_client;
	int		 do_widget_repeat_lock;
	struct proc *boot_focus;
	short loglvl;
	char bootlog_path[200];
};

struct aesys_global
{
	struct adif *adi_mouse;
};

struct helpserver
{
	struct helpserver *next;

	char *ext;
	char *name;
	char *path; /* optional */
};

struct cfg_name_list;
struct cfg_name_list
{
	struct cfg_name_list *next;
	short nlen;
	char name[32];
};

struct xa_keyboards
{
	short c, cur;
	char keyboard[MAX_KEYBOARDS+1][12];
};

struct config
{
#if WITH_GRADIENTS
	Path gradients;
#endif
	Path textures;
	Path palette;       /* load palette from file */

	Path launch_path;		/* Initial path for launcher */
	Path scrap_path;		/* Path to the scrap directory */
	Path snapper;				/* if !0 this is launched on C-A-D */
	Path acc_path;			/* Path to desk accessory directory */

	Path widg_name;			/* Path to XaAES widget rsc */
	Path rsc_name;			/* Path to XaAES rsc */
	Path xobj_name;

	char lang[4];				/* language (de,en,es,...) */

	/* display modes of window title (unimplemented)*/
	/*short topname;
	short backname;
	*/

	short next_active;		/* 0 = set previous active client active upon client termination */
					/* 1 = set owner of previous topped (or only) window upon client termination */
#if 0
	short last_wind;		/* 0 = Put owner of window ontop of window_list infront. */
					/* 1 = Keep client whose last window was closed infront. */
#endif

#if WITH_BBL_HELP
	short xa_bubble;	/* use XaAES for bubble-help: 1: tooltip-style, 2: ballon-style */
	short describe_widgets;	/* display widget-name/function */
#endif
	bool lrmb_swap;			/* Swap left and right mouse-button status bits */
	bool widg_auto_highlight;	/* WIDGET Highligh when Mouse Hovers */
	bool leave_top_border;	/* windows may be moved higher than top of screen */
	bool remap_cicons;
	bool set_rscpalette;
	bool textures_cache;
	bool no_xa_fsel;
	bool point_to_type;
	bool fsel_cookie;
	bool save_windows;
#if EIFFEL_SUPPORT
	bool eiffel_support;	/* generate wheel-events on special keys */
#endif
	bool usehome;			/* use $HOME in shell_find */
	bool naes_cookie;		/* If true, install fake nAES cookie */
	bool menupop_pids;		/* If true, add PIDs to clients listed in menupop clientlist */
	bool menu_locking;		/* menus run in a window.
	                                 * See lines.app run underneath the pulldown menu. :-) */
	bool opentaskman;		/* open taskmanager at boot. */

	short alert_winds;		/* If set, alert windows are shown */
	short back_col;       /* if set, use for background */
	short menu_bar;				/* If 0 don't draw main-menubar */
	short menu_layout;		/* short, float .. */
	short menu_ontop;     /* menubar is window */

	struct xa_keyboards keyboards;
	char cancel_buttons[NUM_CB][CB_L];
#if FILESELECTOR
	char Filters[FS_NPATTERNS][FS_PATLEN];
#endif

	enum menu_behave menu_behave;	/* pull, push or leave */

	short ted_filler;
	short font_id;			/* Font id to use (buttons, menus) */
	short xaw_point;	/* Font id to use (XaAES-window-content) */
	short double_click_time;	/* Double click timing */
	short mouse_packet_timegap;
	short redraw_timeout;

	union {
		MN_SET mn_set;
		struct xa_menu_settings	xamn_set;
	} menu_settings;

	short popup_timeout;
	short popout_timeout;

	short have_wheel_vector;	/* If vex_whlv changed its save_ptr,
					   we have a VDI that supports mouse wheels. */
	short ver_wheel_id;		/* ID of horizontal wheel */
	short ver_wheel_amount;
	short hor_wheel_id;		/* ID of vertical wheel */
	short hor_wheel_amount;

	short icnfy_orient;
	short icnfy_l_x;
	short icnfy_r_x;
	short icnfy_t_y;
	short icnfy_b_y;
	short icnfy_w;
	short icnfy_h;
	short icnfy_reorder_to;

	short standard_font_point;	/* Size for normal text */
	short medium_font_point;	/* The same, but for low resolution screens */
	short small_font_point;		/* Size for small text */
	short info_font_point;	/* Size for infoline */
	short popscroll;		/* number of lines of a popup above which it will be made scrollable. */

	short allow_setexc;  /* 0: never, 1: no trap-vectors, 2: all */
	short videomode;		/* Parameter for video mode selection */
	short device; /* ID of screen device opened by v_opnwk() */

	struct helpserver *helpservers;	/* configured helpservers */
	struct cfg_name_list *ctlalta;
	struct cfg_name_list *kwq;	/* Apps listed here are killed without question upon shutdowns */

	/* postponed cnf things */
	char *cnf_shell;		/* SHELL= */
	char *cnf_shell_arg;		/* args for SHELL cnf directive */
	char *cnf_run[32];		/* RUN directives */
	char *cnf_run_arg[32];		/* args for RUN cnf directives */

	char focus[10];		/* first focus after startup */
};
/*
 * module_register() modes..
 */
#define MODREG_UNREGISTER	0x80000000
#define MODREG_KERNKEY		0x00000001


struct xa_module_api
{
	struct config *cfg;
	struct common *C;
	struct shared *S;
	struct kentry *k;

	void	_cdecl		(*display)		(const char *fmt, ...);
	void	_cdecl		(*ndisplay)		(const char *fmt, ...);
	void	_cdecl		(*bootlog)		(short disp, const char *fmt, ...);

	long	_cdecl		(*module_register)	(long type, void *data);

	char * _cdecl		(*sysfile)		(const char *fname);
	RSHDR * _cdecl		(*load_resource)	(struct xa_client *client, char *fname, RSHDR *rshdr, short designWidth, short designHeight, bool set_pal);
	OBJECT * _cdecl		(*resource_tree)	(RSHDR *rsc, long num);
	void _cdecl		(*obfix)		(OBJECT *obtree, short obj, short designWidth, short desighHeight);

	OBJECT * _cdecl		(*duplicate_obtree)	(struct xa_client *client, OBJECT *tree, short start);
	void _cdecl		(*free_object_tree)	(struct xa_client *cleint, OBJECT *tree);

	void _cdecl			(*init_widget_tree)	(struct xa_client *client, struct widget_tree *wt, OBJECT *obtree);
	struct widget_tree * _cdecl	(*new_widget_tree)	(struct xa_client *client, OBJECT *obtree);
	struct widget_tree * _cdecl	(*obtree_to_wt)		(struct xa_client *client, OBJECT *obtree);
	bool _cdecl			(*remove_wt)		(struct widget_tree *wt, bool force);

	/* OBJECT specific functions */
	OBSPEC *	_cdecl (*object_get_spec)	(OBJECT *ob);
	void 		_cdecl (*object_set_spec)	(OBJECT *ob, unsigned long cl);
	POPINFO *	_cdecl (*object_get_popinfo)	(OBJECT *ob);
	SWINFO *	_cdecl (*object_get_swinfo)	(OBJECT *ob);
	TEDINFO *	_cdecl (*object_get_tedinfo)	(OBJECT *ob, XTEDINFO **x);
	void		_cdecl (*object_spec_wh)	(OBJECT *ob, short *w, short *h);

	void		_cdecl	(*ob_spec_xywh)		(OBJECT *tree, short obj, GRECT *r);
	void		_cdecl	(*render_object)	(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_aes_object item, short px, short py);
	CICON *		_cdecl	(*getbest_cicon)	(CICONBLK *ciconblk);
	short		_cdecl	(*obj_offset)		(struct widget_tree *wt, struct xa_aes_object object, short *mx, short *my);
	void		_cdecl	(*obj_rectangle)	(struct widget_tree *wt, struct xa_aes_object object, GRECT *r);

	void _cdecl		(*obj_set_radio_button)	(struct widget_tree *wt,
							 struct xa_vdi_settings *v,
							 struct xa_aes_object obj,
							 bool redraw,
							 const GRECT *clip,
							 struct xa_rect_list *rl);
	struct xa_aes_object _cdecl (*obj_get_radio_button)	(struct widget_tree *wt,
								struct xa_aes_object parent,
								short state);


	void * _cdecl	(*rp2ap)		(struct xa_window *wind, struct xa_widget *widg, GRECT *r);
	void _cdecl	(*rp2apcs)		(struct xa_window *wind, struct xa_widget *widg, GRECT *r);

	short _cdecl	(*rect_clip)		(GRECT *s, GRECT *d, GRECT *r);

	void * _cdecl	(*kmalloc)		(long size);
	void * _cdecl	(*umalloc)		(long size);
	void _cdecl	(*kfree)		(void *addr);
	void _cdecl	(*ufree)		(void *addr);

	void _cdecl	(*bclear)		(void *addr, unsigned long len);

	void * _cdecl	(*lookup_xa_data)	(struct xa_data_hdr **l,    void *_data);
	void * _cdecl	(*lookup_xa_data_byid)	(struct xa_data_hdr **l,    long id);
	void * _cdecl	(*lookup_xa_data_byname)(struct xa_data_hdr **l,    char *name);
	void * _cdecl	(*lookup_xa_data_byidname)(struct xa_data_hdr **l,  long id, char *name);
	void _cdecl	(*add_xa_data)		(struct xa_data_hdr **list, void *_data, long id, char *name, void _cdecl(*destruct)(void *d));
	void _cdecl	(*remove_xa_data)	(struct xa_data_hdr **list, void *_data);
	void _cdecl	(*delete_xa_data)	(struct xa_data_hdr **list, void *_data);
	void _cdecl	(*ref_xa_data)		(struct xa_data_hdr **list, void *_data, short count);
	long _cdecl	(*deref_xa_data)	(struct xa_data_hdr **list, void *_data, short flags);
	void _cdecl	(*free_xa_data_list)	(struct xa_data_hdr **list);

	void _cdecl	(*load_img)		(char *fname, XAMFDB *result);

	struct xa_window * _cdecl (*create_window)	(int lock,
							SendMessage *message_handler,
							DoWinMesag *message_doer,
							struct xa_client *client,
							bool nolist,
							XA_WIND_ATTR tp,
							WINDOW_TYPE dial,
							int frame,
							bool thinwork,
							const GRECT *R,
							const GRECT *max,
							GRECT *remember);

	int _cdecl	(*open_window)		(int lock, struct xa_window *wind, GRECT r);
	bool _cdecl	(*close_window)		(int lock, struct xa_window *wind);
	void _cdecl	(*move_window)		(int lock, struct xa_window *wind, bool blit, WINDOW_STATUS newstate, short x, short y, short w, short h);
	void _cdecl	(*top_window)		(int lock, bool snd_untopped, bool snd_ontop, struct xa_window *wind);
	void _cdecl	(*bottom_window)	(int lock, bool snd_untopped, bool snd_ontop, struct xa_window *wind);

	void _cdecl	(*send_wind_to_bottom)	(int lock, struct xa_window *wind);
	void _cdecl	(*delete_window)	(int lock, struct xa_window *wind);
	void _cdecl	(*delayed_delete_window)(int lock, struct xa_window *wind);




	struct xa_window * _cdecl (*create_dwind)(int lock, XA_WIND_ATTR tp, char *title, struct xa_client *client, struct widget_tree *wt, FormExit(*f), WindowDisplay(*d));

	void _cdecl	(*redraw_toolbar)	(int lock, struct xa_window *wind, short item);

	void _cdecl	(*dispatch_shutdown)	(short flags);
};

#endif /* _xa_types_h */
