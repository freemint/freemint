
#ifndef tw_window_h
# define tw_window_h 1

#ifndef tw_global_h
# include "global.h"
#endif

/* win->flags */
#define WFULLED		1
#define WICONIFIED 	2
#define WSHADED 	4
#define WISDIAL		8

typedef struct win WINDOW;
struct win
{
	WINDOW	*next;		/* next window in list */
	int	handle;		/* AES window handle */
	int	kind;		/* window gadgets */
	char 	*title;
	GRECT	work;		/* size of window working area */
	GRECT	full;
	GRECT	prev;
	int	max_w, max_h;	/* max size of window working area */
	int	icon_x, icon_y;	/* WINICON Position, da der Objektbaum mehrfach benutzt wird. */
	int	flags;		/* various window flags */
	int	redraw;		/* redraw flag, true if a AES redraw is in progress */
	int	old_wkind;	/* old window gadgets before iconification */
	void	*extra;		/* Pointer to extra data for subclasses */

	void	(*draw) 	(struct win *win, short x, short y, short w, short h);
	void	(*topped) 	(struct win *win);
  	void    (*ontopped) 	(struct win *win);
	void	(*untopped) 	(struct win *win);
	void	(*bottomed) 	(struct win *win);
	void	(*closed) 	(struct win *win);
	void	(*fulled) 	(struct win *win);
	void	(*sized) 	(struct win *win, short x, short y, short w, short h);
	void	(*moved) 	(struct win *win, short x, short y, short w, short h);
	void	(*iconify) 	(struct win *win, short, short, short, short);
	void	(*uniconify)	(struct win *win, short, short, short, short);
	void	(*shaded) 	(struct win *win, short on);
	void	(*arrowed) 	(struct win *win, short msg);
	void	(*vslid) 	(struct win *win, short vpos);
	bool	(*keyinp) 	(struct win *win, short keycode, short shft );
	bool	(*mouseinp)	(struct win *win, short clicks, short x, short y, short shft, short mbuttons);
	void	(*oldfulled)	(struct win *win);
	bool	(*oldmouse)	(struct win *win, short clicks, short x, short y, short shft, short mbuttons);
	void	(*timer) 	(struct win *win, int top);
};

/* Min/Maximale Fenstergr”že */
#define MINROWS	1
#define MINCOLS	10
#define MAXROWS	128
#define MAXCOLS	220

extern WINDOW 	*gl_topwin;	/* oberstes Fenster */
extern WINDOW 	*gl_winlist;	/* LIFO Liste der offenen Fenster */
extern int	 gl_winanz;	/* Anzahl der offenen Fenster */

WINDOW	*create_window (char *title, short kind, 
			short wx, short wy, short ww, short wh,
			short max_w, short max_h);

void	open_window(WINDOW *v, bool as_icon);

void 	destroy_window(WINDOW *v);

void	redraw_window(WINDOW *v, short xc, short yc, short wc, short hc);

WINDOW 	*which_window(short handle);

void	window_term(void);

void 	force_redraw(WINDOW *v);

WINDOW	*get_window(short handle);
WINDOW 	*find_window(short x, short y);

void 	change_window_gadgets(WINDOW *v, short newkind);

void 	cycle_window(void);

void 	title_window(WINDOW *v, char *title);
void	uniconify_all(void);

void	draw_winicon (WINDOW *win);

void	update_window_lock(void);
void	update_window_unlock(void);

void	get_window_rect(WINDOW *win, short kind, GRECT *rect);

/*
 * Eventverarbeitung.
 */
bool 	window_msg(short *msgbuff);
bool	window_key(short keycode, short shift);
bool	window_click(short clicks, short x, short y, short kshift, short mbutton);
void	window_timer (void);

void	wdial_open(WDIALOG *dial);
void	wdial_close(WDIALOG *dial);
bool	wdial_msg(short *msg);

void 	update_avwin(bool new);

/*
 * Mouse actions.
 */
void	mouse_show(void);
short	mouse_hide_if_needed(GRECT *rect);

#endif
