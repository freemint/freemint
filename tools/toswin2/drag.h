
#ifndef tw_drag_h
# define tw_drag_h 1

#ifndef tw_textwin_h
# include "textwin.h"
#endif

void	handle_dragdrop	(short *msg);
void	handle_avdd	(short win_id, char *data);

void	drag_selection	(TEXTWIN *t);


#endif
