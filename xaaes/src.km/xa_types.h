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
#include "xa_defs.h"

#include "debug.h"
#include "messages.h"

#include "mint/proc.h"
#include "adi/moose.h"

#include "xa_list.h"


#define MAX_WINDOW_NAME 200
#define MAX_WINDOW_INFO 200


/* forward declarations */
struct task_administration_block;
struct widget_tree;
struct xa_widget;
struct scroll_entry;
struct fmd_result;

struct xa_lbox_info;
struct lbox_slide;

struct menu_attachements;

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

typedef enum { D_LOCAL = 0, D_TCP = 1 } XA_DISPLAY;

struct xa_colour_scheme
{
	int bg_col;			/* Colour used for backgrounds */
	int fg_col;			/* Colour used for foregrounds */
	int shadow_col;			/* Colour used for shadowed 3d edge */
	int lit_col;			/* Colour used for lit 3d edge */
	int border_col;			/* Colour used for edging */
	int highlight_col;		/* Colour used for highlighting */
};

/*-----------------------------------------------------------------
 * Configuration and options structures
 *-----------------------------------------------------------------*/

struct options
{
	bool windowner;			/* display window owner in title. */
	bool nohide;
	bool xa_nohide;
	bool xa_nomove;
	bool xa_none;			/* No xa_windows at all; classic dialogue handling. */
	bool noleft;			/* Dont move window out left side of screen. */
	bool thinwork;			/* workarea frame a single line. */
	bool nolive;			/* Live moving and sizing of windows. */
	bool wheel_reverse;		/* Do you want move the slider in the wheel direction,
					 * or rather the window content? */
	bool naes;			/* N.Aes 3d compatability */
	bool naes12;

	short thinframe;		/* -1: keep colour frame thin
					 * +1: make a thicker frame for wasier border grabbing and sizing. */
	short wheel_page;		/* how many wheel clicks for a page */

	long half_screen;
	IFDIAG(enum debug_item point[D_max];)
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

typedef void TASK(struct task_administration_block *tab);

struct xa_mouse_rect
{
	short flags;
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

typedef bool DisplayWidget(enum locks lock, struct xa_window *wind,
			   struct xa_widget *widg);

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
#if 0
typedef void ClassicClick(enum locks lock, struct xa_client *client,
			  const struct moose_data *md);
#endif

/* Object display function type */
typedef void ObjectDisplay(enum locks lock, struct widget_tree *wt);

/* Object handler function type */
typedef void ObjectHandler(enum locks lock, struct widget_tree *wt);

#if 0
/* Exit form condition handlers */
typedef void ExitForm(enum locks lock, struct xa_window *wind,
                                 struct xa_widget *widg,
                                 struct widget_tree *wt,
                                 int f, int os, int dbl, int which, struct rawkey *key);
#endif

typedef short _cdecl wdlg_exit  (void *dialog,
				EVNT *evnt,
				short obj,
				short clicks,
				void *data);

/* Object Tree based widget descriptor */
struct wdlg_info
{
	void *handle;			/* For use as 'wdialog structure' */
	struct xa_window *wind;		/* cross reference to parent window. */
	short code;			/* Data for wdlg_xxx extension. */
	short flag;
	void *user_data;
	void *data;
	EVNT *evnt;
	wdlg_exit *exit; //HNDL_OBJ exit;

	short ify_obj;
	struct widget_tree *std_wt;
	struct widget_tree *ify_wt;
	char std_name[MAX_WINDOW_NAME];
	char ify_name[40];
};

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
	lbox_select	*slct;		/* Select callback function */
	lbox_set	*set;		/* Set callback funtion */
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

#define WTF_ALLOC	1
#define WTF_XTRA_ALLOC	2
#define WTF_TREE_ALLOC	4
#define WTF_STATIC	8

	ulong	flags;

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
	ushort *state_mask;

	short parent_x;			/* Keep both in: dont need to change everything in a single effort */
	short parent_y;

	struct objc_edit_info e;


	short lastob;			/* Can be used to validate item number */
	short which;			/* kind of event for use by WDIAL exit handler. */

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
	struct xa_rscs *next, *prior;
	short id;
	short handle;
	short flags;
	struct aes_global *globl;	/* Need a global per resource, otherwise rsrc_gaddr would be ambiguous. */
	struct remember_alloc *ra;
	void *rsc;
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

struct xac_moose_data
{
	struct moose_data *next;
	struct moose_data md;
};

/* Main client application descriptor */
struct xa_client
{
	LIST_ENTRY(xa_client) client_entry;
	LIST_ENTRY(xa_client) app_entry;

	struct proc *p;			/* context back ptr */
	struct xa_user_things *ut;	/* trampoline code for user callbacks */

	bool apterm;			/* true if application understands AP_TERM. */
	bool wa_wheel;			/* The client wants WA_HEEL messages. */

	struct xa_aesmsg_list *msg;	 /* Pending AES messages */
	struct xa_aesmsg_list *rdrw_msg; /* WM_REDRAW messages */
	struct xa_aesmsg_list *crit_msg; /* Critical AES messages - these are prioritized */

#define CS_LAGGING		0x0001
#define CS_CE_REDRAW_SENT 	0x0002
#define CS_FORM_ALERT		0x0004
#define CS_FORM_DO		0x0008
#define CS_WAIT_MENU		0x0100

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

	struct aes_global *globl_ptr;	/* Pointer to the client's globl array (so we can fill in the resource
					 * address field later). */

	RSHDR *rsrc;			/* Pointer to the client's standard GEM-style single resource file */
	RSHDR *rsrc_1;			/* For multiple resources. */
	OBJECT **trees;
	OBJECT **trees_1;
	int rsct;			/* count up/down the loaded resources. Used by XA_alloc, XA_free */

	XA_TREE *wtlist;

	XA_TREE *std_menu;
	XA_TREE *desktop;

	XA_MENU_ATTACHMENT *attach;	/* submenus */
#if 0
	XA_TREE std_menu;		/* The client's standard GEM-style menu-bar widget */
	XA_TREE desktop;		/* The clients desktop as a standard toolbar widget */
	XA_TREE wt;			/* Widget tree for everything except form_do(). */
#endif

	Path home_path;			/* The directory that the client was started in */
	Path cmd_name;			/* The full filename used when launching the process (if launched by shell_write) */
	char *cmd_tail;			/* The command tail of the process (if launched by shell_write) */
	bool tail_is_heap;		/* If true, cmd_tail is a malloc */

	char name[NICE_NAME+2];		/* The clients 'pretty' name (possibly set by menu_register) */
	char proc_name[10];		/* The clients 'official' (ie. used by appl_find) name. */

	struct fmd fmd;			/* Data needed by the XaAES windowing of dialogues. */
	void *temp;			/* Temporary storage */
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

	struct xa_client *srchn;	/* first/next for appl_search */
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
#define CLIENT_MD_BUFFERS	10

	struct moose_data *md_head;
	struct moose_data *md_tail;
	struct moose_data *md_end;

	struct moose_data mdb[CLIENT_MD_BUFFERS+1];

	//bool	md_valid;
	//struct moose_data md;

#define MAX_CEVENTS 15	/* Also used to mask ce_head/ce_tail */
	int	sleepqueue;
	long	sleeplock;

	bool	inblock;
	int	usr_evnt;

	short	cevnt_count;
	struct	c_event *cevnt_head;
	struct	c_event *cevnt_tail;
};

typedef unsigned long AES_function(enum locks lock, struct xa_client *client, AESPB *pb);

struct button_data
{
	struct xa_client *client;
	short x, y;
	short b, cb, clicks, ks;
	//bool got, have, skip;
	short newc, newr;
};
typedef struct button_data BUTTON;

/*-----------------------------------------------------------------
 * Windows & Widgets
 *----------------------------------------------------------------- */


/* Relative coordinate types */
typedef enum
{
	RT = 0,				/* Top right */
	RB,				/* Bottom right */
	LT,				/* Top left */
	LB,				/* Bottom left */
	CT,				/* Top centred */
	CR,				/* right centered */
	CB,				/* Bottom centred */
	CL				/* Left centered */
} XA_RELATIVE;

/* Widget Index's */

enum xa_widgets
{
 /*
  * These widgets must be processable totally indipendant of
  * process context.
  */
	XAW_TITLE = 0,
	XAW_CLOSE,
	XAW_FULL,
	XAW_MOVER,			/* Not actually used like the others */
	XAW_INFO,
	XAW_RESIZE,
	XAW_UPLN,			/* 6 */
	XAW_DNLN,			/* 7 */
	XAW_VSLIDE,
	XAW_UPPAGE,
	XAW_DNPAGE,
	XAW_LFLN,			/* 11 */
	XAW_RTLN,			/* 12 */
	XAW_HSLIDE,
	XAW_LFPAGE,
	XAW_RTPAGE,
	XAW_ICONIFY,
	XAW_HIDE,
	XAW_BORDER,			/* Extended XaAES widget, used for border sizing. */

 /*
  * The widget types above this comment MUST be context indipendant.
  * The widget types blow this comment are considered context-dependant
  * and must be processed via client events.
  * Furthermore, remember to change XA_MAX_CF_WIDGETS below if you
  * put insert a new context dependant widget before XAW_TOOLBAR!
 */
	XAW_TOOLBAR,			/* Extended XaAES widget */
	XAW_MENU,			/* Extended XaAES widget, must be drawn last. */

	/* Number of available XA_WIDGET slots in a the window for default/standard widgets */
	XA_MAX_WIDGETS
};
typedef enum xa_widgets XA_WIDGETS;

#define XA_MAX_CF_WIDGETS	XAW_TOOLBAR

struct xa_widget;

/* Widget positions are defined as relative locations */
struct xa_widget_location
{
	XA_RELATIVE relative_type;	/* Where is the position relative to? */
	RECT r;				/* Position */
	XA_WIDGETS n;			/* index */
	XA_WIND_ATTR mask;		/* disconnect NAME|SMALLER etc from emumerated type XA_WIDGETS */
	short statusmask;
	int rsc_index;			/* If a bitmap widget, index in rsc file */
	bool top;			/* does the widget add to the number widgets at the top of the window. */
	void (*destruct)(struct xa_widget *widg);
};
typedef struct xa_widget_location XA_WIDGET_LOCATION;

/* Window Widget */
struct xa_widget
{
	struct xa_widget *next;		/* For future use. */

	XA_WIDGET_LOCATION loc;		/* Location of widget relative to window extents */
	RECT r;

	DisplayWidget *display;		/* Function pointers to the behaviours of the widget */
	WidgetBehaviour *click;
	WidgetBehaviour *dclick;
	WidgetBehaviour *drag;
	WidgetBehaviour *release;

#define XAWF_ALLOC 1
#define XAWF_STUFFKMALLOC 2
	long flags;
	void (*destruct)(struct xa_widget *w);

	short state;			/* Current status (selected, etc) */
	XA_WIDGETS type;		/* For convenience, makes it possible to disconnect type from index */
	short x, y;			/* If we are activated because we've been clicked on, this is the
					 * relative location of the click (click_object_widget)*/
	short mx, my;			/* absolute mouse location. */
	short s;			/* we must be able to use the original button and state. */
	short cs;
	short k;
	short clicks;			/* Pass the number of clicks, might be usefull */
	short arrowx;			/* WM_ARROWED msg type */
	short xarrow;			/* reverse action when right clicked. */
	short limit;			/* on which limit to stop */
	short xlimit;			/* for reverse action */
	short slider_type;		/* which slider should move for scroll widget */

#define STUFF_IS_WT 1

	short stufftype;		/* type of widget dependant pointer */
	void *stuff;			/* Pointer to widget dependant context data, if any */

	int  start;			/* If stuff is a OBJECT tree, we want start drawing here */
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
struct xa_slider_widget
{
	int position;			/* Actual position of the slider (0-1000(SL_RANGE)) */
	int length;			/* Length (0-1000(SL_RANGE)) */
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

enum window_type
{
	created_for_CLIENT    = 0,
/* All of the following flags indicate at least that a windows workarea is compleyely
   occupied by (part of) the dialoge root object. */
	created_for_FMD_START = 1,
	created_for_FORM_DO   = 2,
	created_for_POPUP     = 4,
	created_for_WDIAL     = 8,
	created_for_TOOLBAR   = 16,
	created_for_AES       = 0x100
};
typedef enum window_type WINDOW_TYPE;

/* Callback for a window's auto-redraw function */
typedef int WindowDisplay (enum locks lock, struct xa_window *wind);

#define XAWS_OPEN	1
#define XAWS_ICONIFIED	2
#define XAWS_SHADED	4
#define XAWS_ZWSHADED	8

/* Window Descriptor */
struct xa_window
{
	struct xa_window *next;		/* Window list stuff - next is the window below */
	struct xa_window *prev;		/*		     - prev is the window above (both NULL terminated) */

	XA_WIND_ATTR active_widgets;	/* Summary of the current standard widgets for the window */
	XA_WIND_ATTR save_widgets;	/* Remember active_widgets if iconified */

	bool nolist;			/* If true, dont put in the window_list. For embedded listboxes mainly. */
	bool thinwork;			/* if true and colour then work := single line box */
	bool wa_wheel;			/* Client wants to receive WA_WHEEL */
	bool outline_adjust;		/* For outlined root object put ny XaAES in a window:
					 * let the window draw the 3 pixel space that emanates from construction. */
	bool dial_followed;		/* false immediate after opening a dial window.
	                        	 * true after first objc_draw. */

	RECT max;			/* Creator dimension's, maximum for sizing */
	RECT min;
	RECT r;				/* Current dimensions */
	RECT rc;
	RECT ro;			/* Original dimemsions when iconified */
	RECT wa;			/* user work area */
	RECT bd;			/* border displacement */
	RECT ba;			/* border area for use by border sizing facility. */
	RECT pr;			/* previous dimensions */
	RECT t;				/* Temporary coordinates used internally */

	short sw, sh;			/* width(not used) and height to use when SHADED */

	RECT *remember;			/* Where to store the current position for remembering. */

	struct xa_client *owner;

	short handle;			/* Window handle */
	short window_status;		/* Window status */
	short frame;			/* Size of the frame (0 for windowed listboxes) */

	struct xa_rect_list *rect_list;	/* The rectangle list for redraws in this window */
	struct xa_rect_list *rect_user;	/* User (wind_get) rect list current pointer */
	struct xa_rect_list *rect_start;/* Start of the rectangle list memory block */
	struct xa_rect_list *rect_wastart;
	struct xa_rect_list *rect_prev;	/* Remember the first rectangle */
	struct xa_rect_list  prev_rect;

	void *background;		/* Pointer to a buffer containing the saved background */

	WINDOW_TYPE dial;		/* Flag - 1 = this window was created by form_dial,
					 *            so don't destroy it until the form_dial(FMD_FINISH)
					 *      - 2 = created by form_do()
					 *      - 4 = otherwise created on behalf of the AES
					 *      - 8 = or created on behalf of wdlg_xxx extension. */

	WindowDisplay *redraw;		/* Pointer to the window's auto-redraw function (if any) */
	//WindowKeypress *keypress;	/* Pointer to the window's keyboard handler function (if any) */
	FormKeyInput *keypress;		/* Pointer to the window's keyboard handler function (if any) */
	WindowDisplay *destructor;	/* Pointer to the window's destructor function (auto-called on
					 * window deletion if a fully auto window) */

	/* Pointer to the internal message handler for this window
	 * (to be handled by the creator) */
	SendMessage	*send_message;
	DoWinMesag	*do_message;

	OBJECT *winob;			/* Tree and index of a sub window (The parent object of the window) */
	int winitem;			/* currently used by list boxes */

#if USER_WIDGETS
	XA_WIDGET_LIST *user_widgets;	/* Pointer to a list of user added widgets */
#endif
	XA_WIDGET *tool;		/* If dialogue or popup: which widget is used. */
	XA_WIDGET widgets[XA_MAX_WIDGETS]; /* The windows standard widget set (array for speed) */

	XA_TREE widg_info;		/* Holds the object tree information for def_widgets. */

	char wname[MAX_WINDOW_NAME];	/* window name line (copy) */
	char winfo[MAX_WINDOW_INFO];	/* window info line (copy) */

	struct wdlg_info *wdlg;
};

struct xa_window *get_top(void);
extern struct xa_window *root_window;
#define window_list S.open_windows.first

struct scroll_info;

typedef int scrl_click(enum locks lock, OBJECT *form, int objc);
typedef void scrl_widget(struct scroll_info *list);
typedef void scrl_vis(struct scroll_info *list, struct scroll_entry *s);


/* HR: The FS_LIST box is the place holder and the
 *     entrypoint via its TOUCHEXIT flag.
 *     The list box itself is turned into a full fledged window.
 */

/* Directory entry flags */
enum scroll_entry_type
{
	FLAG_DIR        = 0x001,
	FLAG_EXECUTABLE = 0x002,
	FLAG_LINK       = 0x004,
	FLAG_MAL        = 0x100,	/* text part is malloc'd and must be freed. */
	FLAG_ENV        = 0x200
};
typedef enum scroll_entry_type SCROLL_ENTRY_TYPE;

struct scroll_entry
{
	char *text;			/* Text to display */
	OBJECT *icon;			/* Icon/object to display to the left of text (if any) */
	struct scroll_entry *next;	/* Next element */
	struct scroll_entry *prev;	/* Previous element */

	SCROLL_ENTRY_TYPE flag;

	/* This simple number makes everything so easy
	 */
	short n;			/* element number in the list */

	char the_text[0];		/* if text included in malloc it is here.
					 * Let entry->text point to here.
					 * FLAG_MAL is off */
};
typedef struct scroll_entry SCROLL_ENTRY;


struct scroll_info
{
	enum locks lock;

	struct xa_window *wi;			/* make the scroll box a real window */
	struct xa_window *pw;			/* If the listbox is part of a windowed dialogue, we must know that,
					 * otherwise we couldnt move that window (rp_2_ap). */
	XA_TREE *wt;

	OBJECT *tree;			/* originating object */

	short item;
	SCROLL_ENTRY *start;		/* Pointer to first element */
	SCROLL_ENTRY *cur;		/*            current selected element */
	SCROLL_ENTRY *top;		/*            top-most displayed element */
	SCROLL_ENTRY *bot;		/*            bottom_most displayed element */
	SCROLL_ENTRY *last;		/*            last element */
	char *title;			/* Title of the list */

	/* The following simple numbers make everything so easy
	 */
	short n;			/* Number of elements in the list */
	short s;			/* Number of elements that fits in the box */
	short v;			/* number of characters that fit the width of the box */
	short left;			/* Portion of the line that is left of the visible part */
	short max;			/* Maximum length of line allowed */
	short state;			/* Extended status info for scroll list */

	scrl_click *dclick;		/* Callback function for double click behaviour */
	scrl_click *click;		/* Callback function for single click behaviour */

	scrl_widget *slider;		/* slider calc function */
	scrl_widget *closer;		/* closer function */
	scrl_widget *fuller;		/* fuller function */

	scrl_vis *vis;			/* check visibility */
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

struct menu_task
{
	TASK_STAGE stage;
	struct xa_window *popw;
	short ty;
	short titles;
	short menus;
	short about;
	short border;
	short clicked_title;
	short pop_item;
	short point_at_menu;
	short clients;
	short exit_mb;
	short omx, omy;
	short x, y;
	RECT bar, drop;
	struct xa_mouse_rect em;
	void *Mpreserve;
	TASK *entry;
	//OBJECT *root;
	XA_TREE *wt;
	/* root displacements */
	short rdx, rdy;
};
typedef struct menu_task MENU_TASK;

struct task_administration_block
{
	struct task_administration_block *nx;
	struct task_administration_block *pr;	/* different concurrent tasks */
	struct task_administration_block *nest;	/* stages of a recursive task (like sub menu's) */ 

	TASK_TY ty;	/* NO_TASK, ROOT_MENU, MENU_BAR, POP_UP... */

	enum locks lock;
	int locker;

	struct xa_client *client;
	struct xa_window *wind;
	XA_WIDGET *widg;

	TASK *task;	/* general task pointer */
	AESPB *pb;

	bool scroll;

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

#endif /* _xa_types_h */
