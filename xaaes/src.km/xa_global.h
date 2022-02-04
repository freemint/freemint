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
#include "mvdi.h"
#include "xcb.h"

# define SHUT_POWER	0
# define SHUT_BOOT	1
# define SHUT_COLD	2
# define SHUT_HALT	3

extern struct nova_data *nova_data;
extern struct cookie_mvdi mvdi_api;

extern unsigned long next_res;

extern char version[32];
extern char vversion[128];

extern char arch_target[];
extern char long_name[];
extern char aes_id[];

extern char info_string[256];

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

/* task administration block list */
#define TAB_LIST_INIT() \
	LIST_INIT(&(S.menu_base))

#define TAB_LIST_START \
	LIST_START(&(S.menu_base))

#define NEXT_TAB(tab) \
	LIST_NEXT(tab,tab_entry)

#define PREV_TAB(tab) \
	LIST_PREV(tab,tab_entry)

#define TAB_LIST_INSERT_START(tab) \
	LIST_INSERT_START(&(S.menu_base), tab, tab_entry);

#define TAB_LIST_INSERT_END(tab) \
	LIST_INSERT_END(&(S.menu_base), tab, tab_entry, task_administration_block);

#define TAB_LIST_REMOVE(tab) \
	LIST_REMOVE(&(S.menu_base), tab, tab_entry)

#define FOREACH_TAB(tab) \
	LIST_FOREACH(&(S.menu_base), tab, tab_entry)

static inline size_t
tab_list_size(void)
{
	struct task_administration_block *tab;
	size_t length = 0;

	FOREACH_TAB(tab)
		length++;

	return length;
}

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
	unsigned long gdos_version;

	void (*reschange)(enum locks lock, struct xa_client *client, bool open);
	struct kernkey_entry *kernkeys;
	
	short AESpid;			/* The AES's MiNT process ID */
	short DSKpid;			/* The desktop programs pid, if any */

	short P_handle;			/* Physical workstation handle used by the AES */
	short global_clip[4];
	short prev_clip[4];

	struct xa_client *Aes;		/* */
    struct xa_client *volatile Hlp;
	void 	*Hlp_pb;
// 	enum waiting_for Aes_waiting_for;
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
#define EXIT_MAINLOOP		0x8000
	short shutdown_step;
	struct timeout *sdt;		/* Shutdown Timeout */

	bool mvalidate;

	long alert_pipe;		/* AESSYS: The MiNT Salert() pipe's file handle */
	long KBD_dev;			/* AESSYS: The MiNT keyboard device's file handle */
	
	RECT iconify;			/* Positioning information for iconifying windows */
	void *Aes_rsc;			/* Pointer to the XaAES resources */
	char *env;			/* new environment */

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
	enum locks		 do_widget_repeat_lock;
	char bootlog_path[200];
};
/* All areas that are common. */
extern struct common C;

struct aesys_global
{
	struct adif *adi_mouse;
};
extern struct aesys_global G;

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
	
struct config
{
	short gradients;

	Path launch_path;		/* Initial path for launcher */
	Path scrap_path;		/* Path to the scrap directory */
	Path acc_path;			/* Path to desk accessory directory */

	Path widg_name;			/* Path to XaAES widget rsc */
	Path rsc_name;			/* Path to XaAES rsc */
	Path xobj_name;

	/* display modes of window title */
	short topname;
	short backname;
	
	short next_active;		/* 0 = set previous active client active upon client termination */
					/* 1 = set owner of previous topped (or only) window upon client termination */
//	short last_wind;		/* 0 = Put owner of window ontop of window_list infront. */
//					/* 1 = Keep client whose last window was closed infront. */

	bool lrmb_swap;			/* Swap left and right mouse-button status bits */
	bool widg_auto_highlight;	/* WIDGET Highligh when Mouse Hovers */
	bool remap_cicons;
	bool set_rscpalette;
	bool no_xa_fsel;
	bool point_to_type;
	bool fsel_cookie;
	bool usehome;			/* use $HOME in shell_find */
	bool naes_cookie;		/* If true, install fake nAES cookie */
	bool menupop_pids;		/* If true, add PIDs to clients listed in menupop clientlist */
	bool menu_locking;		/* menus run in a window.
	                                 * See lines.app run underneath the pulldown menu. :-) */
	bool opentaskman;		/* open taskmanager at boot. */
	
	short alert_winds;		/* If set, alert windows are shown */

	char cancel_buttons[NUM_CB][CB_L];
#if FILESELECTOR
	char Filters[23][16];
#endif

	enum menu_behave menu_behave;	/* pull, push or leave */

//	short widg_w, widg_h;
//	short widg_dw, widg_dh;		/* flexible widget object types. */

	short ted_filler;
	short font_id;			/* Font id to use */
	short double_click_time;	/* Double click timing */
	short mouse_packet_timegap;	
	short redraw_timeout;

	struct xa_menu_settings	mn_set;
	short popup_timeout;
	short popout_timeout;

	short have_wheel_vector;	/* If vex_whlv changed its save_ptr,
					   we have a VDI that supports mouse wheels. */
	short ver_wheel_id;		/* ID of horizontal wheel */
	short ver_wheel_amount;
	short hor_wheel_id;		/* ID of vertical wheel */
	short hor_wheel_amount;
	//short wheel_amount;		/* amount of lines for a wheel click. */

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
	short popscroll;		/* number of lines of a popup above which it will be made scrollable. */

	short videomode;		/* ID of screen device opened by v_opnwk() */

	struct helpserver *helpservers;	/* configured helpservers */
	struct cfg_name_list *ctlalta;
	struct cfg_name_list *kwq;	/* Apps listed here are killed without question upon shutdowns */

	/* postponed cnf things */
	char *cnf_shell;		/* SHELL= */
	char *cnf_shell_arg;		/* args for SHELL cnf directive */
	char *cnf_run[32];		/* RUN directives */
	char *cnf_run_arg[32];		/* args for RUN cnf directives */
};

/* Global config data */
extern struct config cfg;

/* The screen descriptor */
extern struct xa_screen screen;
extern struct xa_objc_render objc_rend;
extern struct xa_vdi_settings global_vdi_settings;
extern struct xa_vdi_api *xa_vdiapi;

#define MONO (screen.colours < 16)

//extern struct xa_widget_theme default_widget_theme;
extern struct options default_options;
extern struct options local_options;

extern XA_PENDING_WIDGET widget_active;

extern struct moose_data mainmd;

extern short border_mouse[];

extern const char mnu_clientlistname[];

// extern XA_TREE nil_tree;

/* shortcuts */
static inline void hidem(void)  { v_hide_c(C.P_handle);    }
static inline void showm(void)  { v_show_c(C.P_handle, 1); }
static inline void forcem(void) { v_show_c(C.P_handle, 0); }

struct xa_client *pid2client(short pid);
struct xa_client *proc2client(struct proc *p);

void *	lookup_xa_data		(struct xa_data_hdr **l,    void *_data);
void *	lookup_xa_data_byname	(struct xa_data_hdr **list, char *name);
void *	lookup_xa_data_byid	(struct xa_data_hdr **list, long id);
void *	lookup_xa_data_byidname	(struct xa_data_hdr **list, long id, char *name);
void	add_xa_data		(struct xa_data_hdr **list, void *_data, long id, char *name, void _cdecl(*destruct)(void *d));
void	remove_xa_data		(struct xa_data_hdr **list, void *_data);
void	delete_xa_data		(struct xa_data_hdr **list, void *_data);
void	ref_xa_data		(struct xa_data_hdr **list, void *_data, short count);
long	deref_xa_data		(struct xa_data_hdr **list, void *_data, short flags);
void	free_xa_data_list	(struct xa_data_hdr **list);

/* Global VDI calls */
XVDIPB *	create_vdipb(void);
void		get_vdistr(char *d, short *s, short len);
void		xvst_font(XVDIPB *vpb, short handle, short id);
XFNT_INFO *	xvqt_xfntinfo(XVDIPB *vpb, short handle, short flags, short id, short index);
short		xvst_point(XVDIPB *vpb, short handle, short point);

static inline void
do_vdi_trap (XVDIPB * vpb)
{
	__asm__ volatile
	(
		"move.l		%0,d1\n\t" 		\
		"move.w		#115,d0\n\t"		\
		"trap		#2\n\t"			\
		:
		: "g"(vpb)			
		: "a0", "a1", "a2", "d0", "d1", "a2", "memory"
	);
}

static inline void
VDI(XVDIPB *vpb, short c0, short c1, short c3, short c5, short c6)
{
	vpb->control[V_OPCODE   ] = c0;
	vpb->control[V_N_PTSIN  ] = c1;
	vpb->control[V_N_INTIN  ] = c3;
	vpb->control[V_SUBOPCODE] = c5;
	vpb->control[V_HANDLE   ] = c6;

	do_vdi_trap(vpb);
}

#endif /* _xa_global_h */
