
#ifndef tw_clipbrd_h
# define tw_clipbrd_h 1

#ifndef tw_textwin_h
# include "textwin.h"
#endif
#ifndef tw_window_h
# include "window.h"
#endif

int	select_word	(TEXTWIN *t, short curcol, short currow);
int 	select_text	(TEXTWIN *t, short curcol, short currow, short kshift);
void	unselect	(TEXTWIN *t);

void	copy		(WINDOW *w, char *dest, int length);
void	paste		(WINDOW *w);

char *	read_scrap	(void);


#endif
