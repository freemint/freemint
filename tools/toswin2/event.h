
#ifndef _tw_event_h_
#define _tw_event_h_


extern bool gl_done;	/* TRUE: Programm beendet sich gerade */

void	event_init	(void);
void	event_loop	(void);
void	menu_help	(int title, int item);


#endif
