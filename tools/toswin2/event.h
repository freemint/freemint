
#ifndef tw_event_h
# define tw_event_h 1

#ifndef tw_global_h
# include "global.h"
#endif

extern bool gl_done;	/* TRUE: Programm beendet sich gerade */

short	needs_redraw	(TEXTWIN *t);

void	event_init	(void);
void	event_loop	(void);
void	menu_help	(int title, int item);


#endif
