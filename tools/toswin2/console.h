
#ifndef tw_console_h
# define tw_console_h 1

#include <gem.h>

#ifndef tw_global_h
# include "global.h"
#endif
#ifndef tw_window_h
# include "window.h"
#endif

extern long con_fd;		/* Handle der Console oder 0 */
extern long con_log_fd;		/* Handle des Console-Log oder 0 */


void	open_console	(void);
bool	log_console	(bool on);
void	handle_console	(char *txt, long len);

bool	is_console	(WINDOW *win);
OBJECT *get_con_icon	(void);


#endif
