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

#ifndef _xa_types_h
#define _xa_types_h

#include "global.h"
#include "ahcm.h"

#include "xa_aes.h"
#include "xa_defs.h"
#include "display.h"
#include "messages.h"

#include "../../sys/xdd/whlmoose/whlmoose.h"


/* forward declarations */
struct task_administration_block;
struct widget_tree;
struct xa_widget;
struct scroll_entry;

enum menu_behave
{
	PULL,
	PUSH,
	LEAVE
};


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
typedef struct xa_colour_scheme XA_COLOUR_SCHEME;

struct xa_screen
{
	RECT r;				/* Screen dimensions */
	XA_DISPLAY display_type;	/* The type of display we are using */
	XA_COLOUR_SCHEME dial_colours;	/* Colour scheme used for dialogs */

	short colours;			/* Number of colours available */
	short planes;			/* Number of planes in screen */
	short standard_font_height;	/* Needed for appl_getinfo */
	short standard_font_id;
	short standard_font_point;
	short small_font_id;
	short small_font_point;
	short small_font_height;	/* Needed for appl_getinfo */

	short c_max_w, c_max_h;		/* Maximum character dimensions in pixels */
	short c_min_w, c_min_h;		/* Minimum (small font) character dimensions in pixels */
};

/*-----------------------------------------------------------------
 * Configuration and options structures
 *-----------------------------------------------------------------*/

struct config
{
	Path launch_path;		/* Initial path for launcher */
	Path scrap_path;		/* Path to the scrap directory */
	Path acc_path;			/* Path to desk accessory directory */

	/* display modes of window title */
	int topname;
	int backname;

	bool direct_call;
	bool no_xa_fsel;
	bool superprogdef;		/* Run progdefs in supervisor mode */
	bool auto_program;
	bool point_to_type;
	bool fsel_cookie;
	bool usehome;			/* use $HOME in shell_find */
	bool menu_locking;		/* menus run in a window.
	                                 * See lines.app run underneath the pulldown menu. :-) */
	bool opentaskman;		/* open taskmanager at boot. */

	char cancel_buttons[NUM_CB][CB_L];
#if FILESELECTOR
	char Filters[23][16];
#endif

	enum menu_behave menu_behave;	/* pull, push or leave */

	int widg_w, widg_h;
	int widg_dw, widg_dh;		/* flexible widget object types. */

	int ted_filler;
	int font_id;			/* Font id to use */
	int have_wheel_vector;		/* If vex_whlv changed its save_ptr, we have a VDI that supports mouse wheels. */
	int wheel_amount;		/* amount of lines for a wheel click. */
	int standard_font_point;	/* Size for normal text */
	int medium_font_point;		/* The same, but for low resolution screens */
	int small_font_point;		/* Size for small text */
	int popscroll;			/* number of lines of a popup above which it will be made scrollable. */
};

struct options
{
	int windowner;			/* display window owner in title. */
	bool nohide;
	bool xa_nohide;
	bool xa_nomove;
	bool xa_none;			/* No xa_windows at all; classic dialogue handling. */
	bool noleft;			/* Dont move window out left side of screen. */
	bool thinwork;			/* workarea frame a single line. */
	bool live;			/* Live moving and sizing of windows. */
	bool wheel_reverse;		/* Do you want move the slider in the wheel direction,
					 * or rather the window content? */
	bool naes;			/* N.Aes 3d compatability */
	bool naes12;

	int thinframe;			/* -1: keep colour frame thin
					 * +1: make a thicker frame for wasier border grabbing and sizing. */
	int wheel_page;			/* how many wheel clicks for a page */

	long half_screen;
	IFDIAG(enum debug_item point[D_max];)
};

struct opt_list
{
	struct opt_list *next;
	Path name;
	struct options options;
};
typedef struct opt_list OPT_LIST;

struct app_list
{
	struct app_list *next;
	Path name;
};
typedef struct app_list APP_LIST;

/*-----------------------------------------------------------------
 * Client application data structures
 *-----------------------------------------------------------------*/

/* List of Pending AES messages */
struct xa_aesmsg_list
{
	struct xa_aesmsg_list *next;
	union msg_buf message;
};
typedef struct xa_aesmsg_list XA_AESMSG_LIST;

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
typedef union conkey CONKEY;

struct rawkey
{
	CONKEY raw;
	ushort aes;			/* AES keycode */
	ushort norm;			/* mormalized keycode */
};
typedef struct rawkey KEY;

/*
 * Codes for what types of event an app is currently waiting for
 * - I've used the same bit allocation and nomenclature as evnt_multi() here,
 *   but there is
 *   the extra XAWAIT_MULTI bit to indicate we are waiting from evnt_multi()
 *   as opposed to one of the individual routines (as return values are different).
 */
typedef enum
{
	XAWAIT_MULTI	= 0x200,	/* Waiting for an evnt_multi() */
	XAWAIT_WDLG	= 0x400,	/* Waiting for a wdlg_xxx event to occur */
	XAWAIT_CHILD	= 0x800,	/* Waiting for a non-parallel shel_write() child process to exit */
	/* HR: menu rectangle event */
	XAWAIT_MENU	= 0x1000,	/* AES private */
} WAITING_FOR;

typedef void TASK(struct task_administration_block *tab);

struct xa_mouse_rect
{
	short flags;
	RECT m1, m2;
	TASK *t1, *t2;	/* used by tasking */
};
typedef struct xa_mouse_rect XA_MOUSE_RECT;

#define ATTACH_MAX 64	/* This is per menu_bar!!!! */

typedef struct menu_attachments
{
	OBJECT *tree;		/* attached menu tree */
	int item;
	OBJECT *to_tree;	/* attached to */
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
typedef bool WidgetBehaviour(LOCK lock, struct xa_window *wind,
                                        struct xa_widget *widg);

typedef int WindowKeypress (LOCK lock, struct xa_window *wind,
                                       struct widget_tree *wt, ushort keycode, ushort nkcode, KEY key);

typedef void ClassicClick(LOCK lock, struct xa_client *client, struct moose_data *md);

/* Object display function type */
typedef void ObjectDisplay(LOCK lock, struct widget_tree *wt);

/* Object handler function type */
typedef void ObjectHandler(LOCK lock, struct widget_tree *wt);

/* Exit form condition handlers */
typedef void ExitForm(LOCK lock, struct xa_window *wind,
                                 struct xa_widget *widg,
                                 struct widget_tree *wt,
                                 int f, int os, int dbl, int which, KEY *key);


/* Object Tree based widget descriptor */
struct wdlg_info
{
	void *handle;			/* For use as 'wdialog structure' */
	struct xa_window *wind;		/* cross reference to parent window. */
	int code;			/* Data for wdlg_xxx extension. */
	int flag;
	void *user_data;
	void *data;
	EVNT *evnt;
	HNDL_OBJ *exit;
};
typedef struct wdlg_info WDLG_INFO;

struct widget_tree
{
	OBJECT *tree;			/* The object tree */
	int current;			/* current item within above tree. */
	RECT r;				/* Why not do the addition (parent_x+ob_x) once in the caller? */
					/* And set a usefull RECT as soon as possible, ready for use in
					 * all kind of functions. */
	struct xa_client *owner;	/* The tree widget would be owned by a different app to
					 * the actual window (like the menu bar on the root window),
					 * or the desktop toolbar widget. HR 270801: was: int pid; */
	struct xa_widget *widg;		/* Cross pointer to widget. */

	ushort *state_mask;

	short parent_x;			/* Keep both in: dont need to change everything in a single effort */
	short parent_y;

	int edit_obj;			/* Index of the current editable text field (if any) */
	int edit_pos;			/* Cursor position within the text field (if any) */
	int lastob;			/* Can be used to validate item number */
	int which;			/* kind of event for use by WDIAL exit handler. */

	bool is_menu;			/* true when the tree is a menu or part of it. */
	bool menu_line;			/* draw a menu line. */
	bool zen;			/* true when the tree is embedded in a window.
	                                 * Do not shadow and border. */
	short dx, dy;			/* displacement of root from upper left corner of window
					 * for operation by sliders. */

	ExitForm *exit_form;		/* Called if exit condition occurs
					 * while handling a form_do or a toolbar
					 * or anything the like ;-) */

	ObjectHandler *exit_handler;	/* Handler vec for direct handling of exit
					 * condition. The 'aaplication' aspect of the
					 * exit_form */

	void *extra;			/* Extra info if needed (texts for alert) */

#if WDIAL
	WDLG_INFO wdlg;
#endif
};
typedef struct widget_tree XA_TREE;

/* used for linking resources's per client */
struct xa_rscs
{
	struct xa_rscs *next, *prior;
	int     id, handle;
	struct aes_global *globl;	/* Need a global per resource, otherwise rsrc_gaddr would be ambiguous. */
	void *rsc;
};
typedef struct xa_rscs XA_RSCS;

/* Kernal command packet structure */
struct k_cmd_packet
{
	short pid;			/* Client pid */
	ushort cmd;			/* Command code */
	AESPB *pb;			/* Pointer to AES parameter block */
};
typedef struct k_cmd_packet K_CMD_PACKET;

struct app_contrl
{
	char pipe[32];
	short ctrl[5];	
	K_CMD_PACKET packet;
	AESPB pb;
};
typedef struct app_contrl APP_CONTRL;

struct fmd
{
	struct xa_window *wind;		/* Pointer to a window that could be about to become a dialog */
	short state;			/* fmd.r contains a valid rectangle of a form_dial, which is postponed. */
	short lock;			/* Client has locked the screen. */
	XA_WIND_ATTR kind;		/* Window attributes to be used. */
	WindowKeypress *keypress;
	ClassicClick *mousepress;
	RECT r;				/* The rectangle for the postponed dialogue window */
};
typedef struct fmd FMD;

/* Main client application descriptor */
struct xa_client
{
	struct xa_client *next, *prior;
	int pid;			/* clients process identifier number */

	int client_end;			/* File handle's of the clients internal reply pipe */
	int kernel_end;

	bool init;			/* true between appl_init & appl_exit */
	bool killed;			/* true if the clients memory is destroyed. */
	bool secured;			/* true if secure remove_refs applied. */
	bool apterm;			/* true if application understands AP_TERM. */
	bool wa_wheel;			/* The client wants WA_HEEL messages. */

	XA_AESMSG_LIST *msg;		/* Pending AES messages */
	WAITING_FOR waiting_for;	/* What types of event(s) the client is waiting for */
	AESPB *waiting_pb;		/* Parameter block for whatever the client is waiting for */

	int mouse;			/* The cursor to use when this is top application */
	int save_mouse;
	MFORM *mouse_form;
	MFORM *save_mouse_form;

	struct aes_global *globl_ptr;	/* Pointer to the client's globl array (so we can fill in the resource
					 * address field later). */

	RSHDR *rsrc;			/* Pointer to the client's standard GEM-style single resource file */
	RSHDR *rsrc_1;			/* For multiple resources. */
	OBJECT **trees;
	OBJECT **trees_1;
	int rsct;			/* count up/down the loaded resources. Used by XA_alloc, XA_free */

	XA_MENU_ATTACHMENT *attach;	/* submenus */
	XA_TREE std_menu;		/* The client's standard GEM-style menu-bar widget */
	XA_TREE desktop;		/* The clients desktop as a standard toolbar widget */
	XA_TREE wt;			/* Widget tree for everything except form_do(). */

	Path home_path;			/* The directory that the client was started in */
	Path cmd_name;			/* The full filename used when launching the process (if launched by shell_write) */
	char *cmd_tail;			/* The command tail of the process (if launched by shell_write) */
	bool tail_is_heap;		/* If true, cmd_tail is a malloc */

	char name[NICE_NAME+2];		/* The clients 'pretty' name (possibly set by menu_register) */
	char proc_name[10];		/* The clients 'official' (ie. used by appl_find) name. */
	int  parent;			/* Process id of the client's "parent" process. (need not be a AES client!!!!) */

	FMD fmd;			/* Data needed by the XaAES windowing of dialogues. */
	void *temp;			/* Temporary storage */
	int  type;			/* What type of client is this? */

	char *half_screen_buffer;	/* for wind_get WF_SCREEN (people tend to use what is offered,
					 * whether the idee is good or not) */
	long half_screen_size;

	XA_MOUSE_RECT em;		/* Needed. whether threads or not */
	long timer_val;			/* Pass it here, instead of awkward in the return value; */ 
	XA_memory base;			/* Base of the memory allocated for the client. */
	XA_RSCS *resources;		/* link loaded resoures' allocated memory, so it can be freeed.
					 * this also solves the problem that memory allocated for colour icon data
					 * was left orphaned. */

	APP_CONTRL app;			/* Control structure for XA_new_client & XA_client_exit. */
	unsigned long tinybuf;		/* For writing return values to the client. */
	struct xa_client *srchn;	/* first/next for appl_search */
#if GENERATE_DIAGS
	char zen_name[NICE_NAME + 2 + 16];
#endif
	int xdrive;
	Path xpath;
	struct options options;		/* Individual AES options. */
};
typedef struct xa_client XA_CLIENT;

typedef unsigned long AES_function(LOCK lock, XA_CLIENT *client, AESPB *pb);

typedef enum
{
	LOCKSCREEN = 1,			/* lock/unlock screen around AES function. */
	NO_SEMA    = 2			/* Call direct for small AES functions
					 * that do not need the call_direct semaphores. */
} AES_funcflags;

struct xa_function_table
{
	AES_function *f;		/* function pointer */
	bool d;				/* true id call direct */
	AES_funcflags p;		/* flags */
};
typedef struct xa_function_table XA_FTAB;

struct button_data
{
	XA_CLIENT *client;
	short x, y;
	short b, cb, clicks, ks;
	bool got, have, skip;
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
	XAW_TOOLBAR,			/* Extended XaAES widget */
	XAW_BORDER,			/* Extended XaAES widget, used for border sizing. */
	XAW_MENU,			/* Extended XaAES widget, must be drawn last. */

	/* Number of available XA_WIDGET slots in a the window for default/standard widgets */
	XA_MAX_WIDGETS
};
typedef enum xa_widgets XA_WIDGETS;

/* Widget positions are defined as relative locations */
struct xa_widget_location
{
	XA_RELATIVE relative_type;	/* Where is the position relative to? */
	RECT r;				/* Position */
	XA_WIDGETS n;			/* index */
	XA_WIND_ATTR mask;		/* disconnect NAME|SMALLER etc from emumerated type XA_WIDGETS */
	int rsc_index;			/* If a bitmap widget, index in rsc file */
	bool top;			/* does the widget add to the number widgets at the top of the window. */
};
typedef struct xa_widget_location XA_WIDGET_LOCATION;

/* Window Widget */
struct xa_widget
{
	struct xa_widget *next;		/* For future use. */
	XA_WIDGET_LOCATION loc;		/* Location of widget relative to window extents */

	WidgetBehaviour *display;	/* Function pointers to the behaviours of the widget */
	WidgetBehaviour *click;
	WidgetBehaviour *dclick;
	WidgetBehaviour *drag;
	WidgetBehaviour *release;

	OB_STATE state;			/* Current status (selected, etc) */
	XA_WIDGETS type;		/* For convenience, makes it possible to disconnect type from index */
	short x, y;			/* If we are activated because we've been clicked on, this is the
					 * relative location of the click (click_object_widget)*/
	short mx, my;			/* absolute mouse location. */
	short s;			/* we must be able to use the original button and state. */
	short k;
	short clicks;			/* Pass the number of clicks, might be usefull */
	short arrowx;			/* WM_ARROWED msg type */
	short xarrow;			/* reverse action when right clicked. */
	short limit;			/* on which limit to stop */
	short xlimit;			/* for reverse action */
	short slider_type;		/* which slider should move for scroll widget */

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
	short b, cb, clicks, nx, ny;
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
typedef struct xa_rect_list XA_RECT_LIST;

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
typedef int WindowDisplay (LOCK lock, struct xa_window *wind);

/* Window status codes */
typedef enum
{
	XAWS_CLOSED,
	XAWS_OPEN,
	XAWS_ICONIFIED
} XAWS;

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
	RECT r;				/* Current dimensions */
	RECT ro;			/* Original dimemsions when iconified */
	RECT wa;			/* user work area */
	RECT bd;			/* border displacement */
	RECT ba;			/* border area for use by border sizing facility. */
	RECT pr;			/* previous dimensions */
		 
	RECT *remember;			/* Where to store the current position for remembering. */

	XA_CLIENT *owner;

	int handle;			/* Window handle */
	int is_open;			/* Flag - is the window actually open? */
	int window_status;		/* Window status */
	int frame;			/* Size of the frame (0 for windowed listboxes) */

	XA_RECT_LIST *rect_list;	/* The rectangle list for redraws in this window */
	XA_RECT_LIST *rect_user;	/* User (wind_get) rect list current pointer */
	XA_RECT_LIST *rect_start;	/* Start of the rectangle list memory block */
	XA_RECT_LIST *rect_prev;	/* Remember the first rectangle */
	XA_RECT_LIST  prev_rect;

	void *background;		/* Pointer to a buffer containing the saved background */

	WINDOW_TYPE dial;		/* Flag - 1 = this window was created by form_dial,
					 *            so don't destroy it until the form_dial(FMD_FINISH)
					 *      - 2 = created by form_do()
					 *      - 4 = otherwise created on behalf of the AES
					 *      - 8 = or created on behalf of wdlg_xxx extension. */

	WindowDisplay *redraw;		/* Pointer to the window's auto-redraw function (if any) */
	WindowKeypress *keypress;	/* Pointer to the window's keyboard handler function (if any) */
	WindowDisplay *destructor;	/* Pointer to the window's destructor function (auto-called on
					 * window deletion if a fully auto window) */

	/* Pointer to the internal message handler for this window
	 * (to be handled by the creator) */
	void (*send_message)(
		LOCK lock,
		struct xa_window *wind,
		struct xa_client *to,
		short mp0, short mp1, short mp2, short mp3,
		short mp4, short mp5, short mp6, short mp7);

	OBJECT *winob;			/* Tree and index of a sub window (The parent object of the window) */
	int winitem;			/* currently used by list boxes */

#if USER_WIDGETS
	XA_WIDGET_LIST *user_widgets;	/* Pointer to a list of user added widgets */
#endif
	XA_WIDGET *tool;		/* If dialogue or popup: which widget is used. */
	XA_WIDGET widgets[XA_MAX_WIDGETS]; /* The windows standard widget set (array for speed) */

	XA_TREE widg_info;		/* Holds the object tree information for def_widgets. */
	XA_TREE menu_bar;		/*   "         "              "      for a menu bar. */
	XA_TREE toolbar;		/*   "         "              "      for a tool bar. */

	Path name, info;		/* wind_set makes copies. */
};
typedef struct xa_window XA_WINDOW;

/* open and closed windows in separate lists. */
struct win_base
{
	XA_WINDOW *first;
	XA_WINDOW *last;
};
typedef struct win_base WIN_BASE;

XA_WINDOW *get_top(void);
extern XA_WINDOW *root_window;
#define window_list S.open_windows.first

typedef void fsel_handler(LOCK lock, char *path, char *file);
typedef int scrl_click(LOCK lock, OBJECT *form, int objc);
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
	FLAG_PENDING    = 0x008,	/* For system alerts if the update lock is set. */
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


typedef void scrl_widget(struct scroll_info *list);

struct scroll_info
{
	LOCK lock;

	XA_WINDOW *wi;			/* make the scroll box a real window */
	XA_WINDOW *pw;			/* If the listbox is part of a windowed dialogue, we must know that,
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
	XA_WINDOW *popw;
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
	XA_MOUSE_RECT em;
	void *Mpreserve;
	TASK *entry;
	OBJECT *root;
	/* root displacements */
	short rdx, rdy;
};
typedef struct menu_task MENU_TASK;

struct drag_task
{
	RECT in,out,dr;
	short  x,y;
};
typedef struct drag_task DRAG_TASK;

struct task_administration_block
{
	struct task_administration_block *nx;
	struct task_administration_block *pr;	/* different concurrent tasks */
	struct task_administration_block *nest;	/* stages of a recursive task (like sub menu's) */ 

	int reply;
	int locker;

	TASK_TY ty;	/* NO_TASK, ROOT_MENU, MENU_BAR, POP_UP... */

	int timeout;	/* Normally 0, otherwise the frequency that we call active_function with */
	LOCK lock;

	XA_CLIENT *client;
	XA_WINDOW *wind;
	XA_WIDGET *widg;

	TASK *task;	/* general task pointer */
	AESPB *pb;

	bool scroll;
	int dbg, dbg2, dbg3;

	union
	{
		MENU_TASK menu;
		DRAG_TASK drag;
	} task_data;
};
typedef struct task_administration_block Tab;

struct shared
{
	WIN_BASE open_windows;
	WIN_BASE closed_windows;	/* open and closed windows in separate lists */
	WIN_BASE side_windows;		/* list of other special windows like menus popups etc */

	int update_lock;		/* wind_update() locks */
	int mouse_lock;
	int update_cnt;
	int mouse_cnt;

	XA_CLIENT *client_list;		/* The clients database */
	XA_CLIENT *Clients[MAX_PID];	/* for Pid2Client */

	OPT_LIST *app_options;		/* individual option settings. */
};

struct common
{
	short vh;			/* Virtual workstation handles used by the AES */
	short AESpid;			/* The AES's MiNT process ID */
	short DSKpid;			/* The desktop programs pid, if any */
	short home_drv;

	short P_handle;			/* Physical workstation handles used by the AES */
	short global_clip[4];

	Path home;			/* XaAES's home location */
	Path cmd_name;			/* Name of the Aes command pipe */

	XA_CLIENT *Aes;			/* static pid array */
	XA_CLIENT *pending_exit[MAX_CLIENT];
					/* a client didnt call appl_exit, so signal handler
					   wants to remove the client from the AES. */

	int shutdown;
	bool mvalidate;

	long AES_in_pipe;		/* The AES instruction input pipe handle */
	long MOUSE_dev;			/* The /dev/mouse handle */
	long KBD_dev;			/* The MiNT keyboard device's file handle */
	long Salert_pipe;		/* The MiNT Salert() pipe's file handle */

	/* exteneded & generalized (was GeneralCallback & stuff) */
	Tab active_menu[CASCADE];
	Tab active_timeout;
	Tab *menu_base;

	int menu_nest;			/* current depth of submenus */
	RECT iconify;			/* Positioning information for iconifying windows */
	void *Aes_rsc;			/* Pointer to the XaAES resources */
	char *env;			/* new environment */
	char *strings[STRINGS];		/* pointers to environment variables from mint & config file */

	XA_WINDOW *focus;		/* Only 1 of 2: the top window(window_list) or root_window. */

	Path desk;			/* Remember the desk path for Launch desk. */
	int mouse;			/* Remember mouse shape */
	MFORM *mouse_form;		/* Remember mouse form */
};

/*
 * definitions for shutdown in struct common
 */
#define QUIT_NOW      1
#define HALT_SYSTEM   2
#define REBOOT_SYSTEM 4
#define QUIT_XAAES    8


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
