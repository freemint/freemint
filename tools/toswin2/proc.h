
#ifndef tw_proc_h
# define tw_proc_h 1

#ifndef tw_textwin_h
# include "textwin.h"
#endif
#ifndef tw_config_h
# include "config.h"
#endif

TEXTWIN	*new_proc(char *progname, char *arg, char *env, char *progdir,
		  WINCFG *cfg, int talkID);

void	term_proc(TEXTWIN *t);

TEXTWIN *new_shell(void);
TEXTWIN *start_prog(void);

void 	fd_input(void);
void	add_fd(int fd);

void	proc_init(void);


#endif
