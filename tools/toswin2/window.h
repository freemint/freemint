#ifndef _tw_window_h_
#define _tw_window_h_

extern WINDOW 	*gl_topwin;		/* oberstes Fenster */
extern WINDOW 	*gl_winlist;	/* LIFO Liste der offenen Fenster */
extern int		gl_winanz;		/* Anzahl der offenen Fenster */

extern WINDOW	*create_window(const char *title, int kind, 
										int wx, int wy, int ww, int wh,
										int max_w, int max_h);

extern void		open_window(WINDOW *v, bool as_icon);

extern void 	destroy_window(WINDOW *v);

extern void		redraw_window(WINDOW *v, int xc, int yc, int wc, int hc);

extern WINDOW 	*which_window(int handle);

extern void		window_term(void);

extern void 	force_redraw(WINDOW *v);

extern WINDOW	*get_window(int handle);
extern WINDOW 	*find_window(int x, int y);

extern void 	change_window_gadgets(WINDOW *v, int newkind);

extern void 	cycle_window(void);

extern void 	title_window(WINDOW *v, char *title);
extern void		uniconify_all(void);

extern bool 	window_msg(int *msgbuff);
extern bool		window_key(int keycode, int shift);
extern bool		window_click(int clicks, int x, int y, int kshift, int mbutton);
/*
 * Eventverarbeitung.
*/

extern void		wdial_open(WDIALOG *dial);
extern void		wdial_close(WDIALOG *dial);
extern bool		wdial_msg(int *msg);

extern void 	update_avwin(bool new);

#endif
