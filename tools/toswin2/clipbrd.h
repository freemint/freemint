#ifndef _tw_cipbrd_h_
#define _tw_cipbrd_h_

extern int	select_word	(TEXTWIN *t, int curcol, int currow);
extern int 	select_text	(TEXTWIN *t, int curcol, int currow, int kshift);
extern void	unselect		(TEXTWIN *t);

extern void copy			(WINDOW *w, char *dest, int length);
extern void paste			(WINDOW *w);

extern char *read_scrap	(void);

#endif
