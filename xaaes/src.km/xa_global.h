/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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
	struct win_base open_windows;	/* list of all open windows */
	struct win_base closed_windows;	/* list of all closed windows */
	struct win_base side_windows;	/* list of other special windows like menus popups etc */
	struct win_base deleted_windows;/* list of windows to be deleted (delayed action) */

	struct xa_client *client_list;	/* The clients database */
	short		 wm_count;
	struct xa_client *wait_mouse;	/* This client need mouse events exclusivly */
	struct opt_list *app_options;	/* individual option settings. */
};

/* Area's shared between server and client, subject to locking. */
extern struct shared S;

struct shel_info
{
	int type;

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
	short home_drv;

	short P_handle;			/* Physical workstation handle used by the AES */
	short global_clip[4];

	Path home;			/* XaAES's home location */

	struct xa_client *Aes;		/* */

	struct xa_client *buffer_moose;

	int shutdown;
	bool mvalidate;

	long alert_pipe;		/* AESSYS: The MiNT Salert() pipe's file handle */
	long KBD_dev;			/* AESSYS: The MiNT keyboard device's file handle */
	long MOUSE_dev;			/* AESSYS: The /dev/mouse file handle */
	struct file *kmoose;		/* internal, context mouse handle */

	/* exteneded & generalized (was GeneralCallback & stuff) */
	Tab active_menu[CASCADE];
	Tab active_timeout;
	Tab *menu_base;

	int menu_nest;			/* current depth of submenus */
	RECT iconify;			/* Positioning information for iconifying windows */
	void *Aes_rsc;			/* Pointer to the XaAES resources */
	char *env;			/* new environment */
	char *strings[STRINGS];		/* pointers to environment variables from mint & config file */

	struct xa_window *focus;	/* Only 1 of 2: the top window(window_list) or root_window. */

	Path desk;			/* Remember the desk path for Launch desk. */
	int mouse;			/* Remember mouse shape */
	MFORM *mouse_form;		/* Remember mouse form */
};

/* All areas that are common. */
extern struct common C;

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
	int have_wheel_vector;		/* If vex_whlv changed its save_ptr,
					   we have a VDI that supports mouse wheels. */
	int wheel_amount;		/* amount of lines for a wheel click. */
	int standard_font_point;	/* Size for normal text */
	int medium_font_point;		/* The same, but for low resolution screens */
	int small_font_point;		/* Size for small text */
	int popscroll;			/* number of lines of a popup above which it will be made scrollable. */

	/* postponed cnf things */
	Path cnf_shell;			/* SHELL= */
	char *cnf_run[32];		/* RUN= */
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

extern BUTTON mu_button;

extern XA_TREE nil_tree;

/* shortcuts */
static inline void hidem(void)  { v_hide_c(C.vh);    }
static inline void showm(void)  { v_show_c(C.vh, 1); }
static inline void forcem(void) { v_show_c(C.vh, 0); }

struct xa_client *pid2client(short pid);

#endif /* _xa_global_h */
