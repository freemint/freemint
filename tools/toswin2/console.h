
#ifndef _console_h_
#define _console_h_


extern long con_fd;		/* Handle der Console oder 0 */
extern long con_log_fd;		/* Handle des Console-Log oder 0 */


void	open_console	(void);
bool	log_console	(bool on);
void	handle_console	(char *txt, long len);

bool	is_console	(WINDOW *win);
OBJECT *get_con_icon	(void);


#endif
