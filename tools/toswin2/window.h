
#ifndef _tw_window_h_
#define _tw_window_h_

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

/*
 * Eventverarbeitung.
 */
bool 	window_msg(short *msgbuff);
bool	window_key(short keycode, short shift);
bool	window_click(short clicks, short x, short y, short kshift, short mbutton);

void	wdial_open(WDIALOG *dial);
void	wdial_close(WDIALOG *dial);
bool	wdial_msg(short *msg);

void 	update_avwin(bool new);


#endif
