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

#ifndef _xa_global_h
#define _xa_global_h

#include "global.h"
#include "xa_types.h"

/*
 * GLOBAL VARIABLES AND DATA STRUCTURES
 */

/* open and closed windows in separate lists. */
struct win_base
{
	struct xa_window *first;
	struct xa_window *last;
};

struct shared
{
	struct win_base open_windows;		/* list of all open windows */
	struct win_base closed_windows;		/* list of all closed windows */
	struct win_base side_windows;		/* list of other special windows like menus popups etc */
	struct win_base deleted_windows;	/* list of windows to be deleted (delayed action) */
	struct win_base nolist_windows;		/* list of open nolist windows - fmd, alerts, etc. */

	LIST_HEAD(xa_client) client_list;
	LIST_HEAD(xa_client) app_list;

	struct xa_client *wait_mouse;	/* This client need mouse events exclusivly */
	struct opt_list *app_options;	/* individual option settings. */

	short wm_count;			/* XXX ??? */
};

/* Area's shared between server and client, subject to locking. */
extern struct shared S;


/* CLIENT list operations */

#define CLIENT_LIST_INIT() \
	LIST_INIT(&(S.client_list))

#define CLIENT_LIST_START \
	LIST_START(&(S.client_list))

#define NEXT_CLIENT(client) \
	LIST_NEXT(client,client_entry)

#define PREV_CLIENT(client) \
	LIST_PREV(client,client_entry)

#define CLIENT_LIST_INSERT_START(client) \
	LIST_INSERT_START(&(S.client_list), client, client_entry);

#define CLIENT_LIST_INSERT_END(client) \
	LIST_INSERT_END(&(S.client_list), client, client_entry, xa_client);

#define CLIENT_LIST_REMOVE(client) \
	LIST_REMOVE(&(S.client_list), client, client_entry)

#define FOREACH_CLIENT(client) \
	LIST_FOREACH(&(S.client_list), client, client_entry)

static inline size_t
client_list_size(void)
{
	struct xa_client *cl;
	size_t length = 0;

	FOREACH_CLIENT(cl)
		length++;

	return length;
}


/* APP list operations */

#define APP_LIST_INIT() \
	LIST_INIT(&(S.app_list))

#define APP_LIST_START \
	LIST_START(&(S.app_list))

#define NEXT_APP(client) \
	LIST_NEXT(client,app_entry)

#define PREV_APP(client) \
	LIST_PREV(client,app_entry)

#define APP_LIST_INSERT_START(client) \
	LIST_INSERT_START(&(S.app_list), client, app_entry);

#define APP_LIST_INSERT_END(client) \
	LIST_INSERT_END(&(S.app_list), client, app_entry, xa_client);

#define APP_LIST_REMOVE(client) \
	LIST_REMOVE(&(S.app_list), client, app_entry)

#define FOREACH_APP(client) \
	LIST_FOREACH(&(S.app_list), client, app_entry)


struct shel_info
{
	short type;
	short reserved;

	char *cmd_tail;
	bool tail_is_heap;

	Path cmd_name;
	Path home_path;
};

struct common
{
	short vh;			/* Virtual workstation handle used by the AES */
	short AESpid;			/* The AES's MiNT process ID */
	short DSKpid;			/* The desktop programs pid, if any */

	short P_handle;			/* Physical workstation handle used by the AES */
	short global_clip[4];
	short prev_clip[4];

	struct xa_client *Aes;		/* */

	long redraws;			/* Counting WM_REDRAWS being sent and dispatched */
	struct xa_client *button_waiter;/* Client which is getting the next moose_data packet, */
					/* most probably a button released one */
	struct xa_client *ce_open_menu;	/* If set, this client has been sent a open_menu client event */

	short shutdown;			/* flags for shutting down xaaes */
#define QUIT_NOW	0x1		/* - enter shutdown the next possible time */
#define HALT_SYSTEM	0x2		/* - halt system after xaaes shutdown */
#define REBOOT_SYSTEM	0x4		/* - reboot system after xaaes shutdown */

	bool mvalidate;

	long alert_pipe;		/* AESSYS: The MiNT Salert() pipe's file handle */
	long KBD_dev;			/* AESSYS: The MiNT keyboard device's file handle */
	struct adif *adi_mouse;

	/* exteneded & generalized (was GeneralCallback & stuff) */
	Tab active_menu[CASCADE];
	Tab *menu_base;

	short menu_nest;			/* current depth of submenus */
	RECT iconify;			/* Positioning information for iconifying windows */
	void *Aes_rsc;			/* Pointer to the XaAES resources */
	char *env;			/* new environment */

	struct xa_window *focus;	/* Only 1 of 2: the top window(window_list) or root_window. */
	struct xa_client *update_lock;
	struct xa_client *mouse_lock;
	struct xa_client *menu_lock;

	short updatelock_count;
	short mouselock_count;
	short menulock_count;
	
	Path desk;			/* Remember the desk path for Launch desk. */
	short mouse;			/* Remember mouse shape */
	MFORM *mouse_form;		/* Remember mouse form */

	short aesmouse;
	MFORM *aesmouse_form;

	short realmouse;
	MFORM *realmouse_form;
};

/* All areas that are common. */
extern struct common C;

struct helpserver
{
	struct helpserver *next;

	char *ext;
	char *name;
	char *path; /* optional */
};

struct config
{
	Path launch_path;		/* Initial path for launcher */
	Path scrap_path;		/* Path to the scrap directory */
	Path acc_path;			/* Path to desk accessory directory */

	Path widg_name;			/* Path to XaAES widget rsc */
	Path rsc_name;			/* Path to XaAES rsc */

	/* display modes of window title */
	short topname;
	short backname;
	
	short next_active;		/* 0 = set previous active client active upon client termination */
					/* 1 = set owner of previous topped (or only) window upon client termination */
	short last_wind;		/* 0 = Put owner of window ontop of window_list infront. */
					/* 1 = Keep client whose last window was closed infront. */

	bool lrmb_swap;			/* Swap left and right mouse-button status bits */
	bool no_xa_fsel;
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

	short widg_w, widg_h;
	short widg_dw, widg_dh;		/* flexible widget object types. */

	short ted_filler;
	short font_id;			/* Font id to use */
	short double_click_time;	/* Double click timing */
	short mouse_packet_timegap;	
	short redraw_timeout;
	short have_wheel_vector;	/* If vex_whlv changed its save_ptr,
					   we have a VDI that supports mouse wheels. */
	short wheel_amount;		/* amount of lines for a wheel click. */
	short standard_font_point;	/* Size for normal text */
	short medium_font_point;	/* The same, but for low resolution screens */
	short small_font_point;		/* Size for small text */
	short popscroll;		/* number of lines of a popup above which it will be made scrollable. */

	short videomode;		/* ID of screen device opened by v_opnwk() */

	struct helpserver *helpservers;	/* configured helpservers */

	/* postponed cnf things */
	char *cnf_shell;		/* SHELL= */
	char *cnf_shell_arg;		/* args for SHELL cnf directive */
	char *cnf_run[32];		/* RUN directives */
	char *cnf_run_arg[32];		/* args for RUN cnf directives */
};

/* Global config data */
extern struct config cfg;

struct xa_screen
{
	RECT r;				/* Screen dimensions */
	XA_DISPLAY display_type;	/* The type of display we are using */
	struct xa_colour_scheme dial_colours;
					/* Colour scheme used for dialogs */

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

/* The screen descriptor */
extern struct xa_screen screen;
#define MONO (screen.colours < 16)

extern struct options default_options;
extern struct options local_options;

extern XA_PENDING_WIDGET widget_active;

extern struct moose_data mainmd;

extern short border_mouse[];

extern const char mnu_clientlistname[];

extern XA_TREE nil_tree;

/* shortcuts */
static inline void hidem(void)  { v_hide_c(C.vh);    }
static inline void showm(void)  { v_show_c(C.vh, 1); }
static inline void forcem(void) { v_show_c(C.vh, 0); }

struct xa_client *pid2client(short pid);

#endif /* _xa_global_h */
