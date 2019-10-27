
#ifndef tw_console_h
# define tw_console_h 1

#include <gem.h>

#include "global.h"
#include "window.h"
#include "textwin.h"

extern long con_fd;		/* Handle der Console oder 0 */
extern long con_log_fd;		/* Handle des Console-Log oder 0 */
extern TEXTWIN *con_win;	/* console window */
extern char const console_progname[];


void	open_console	(void);
bool	log_console	(bool on);
bool	out_console	(bool on);
void	handle_console	(char *txt, long len);

bool	is_console	(WINDOW *win);
OBJECT *get_con_icon	(void);


#endif
