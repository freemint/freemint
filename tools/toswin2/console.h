#ifndef _console_h_
#define _console_h_

extern int	con_fd;			/* Handle der Console oder 0 */
extern int	con_log_fd;		/* Handle des Console-Log oder 0 */

extern void open_console	(void);
extern bool log_console		(bool on);
extern void handle_console	(char *txt, long len);

extern bool is_console		(WINDOW *win);
extern OBJECT *get_con_icon(void);

#endif
