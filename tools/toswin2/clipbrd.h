
#ifndef _tw_cipbrd_h_
#define _tw_cipbrd_h_


int	select_word	(TEXTWIN *t, short curcol, short currow);
int 	select_text	(TEXTWIN *t, short curcol, short currow, short kshift);
void	unselect	(TEXTWIN *t);

void	copy		(WINDOW *w, char *dest, int length);
void	paste		(WINDOW *w);

char *	read_scrap	(void);


#endif
