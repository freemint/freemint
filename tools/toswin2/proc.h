#ifndef _tw_proc_h_
#define _tw_proc_h_

extern TEXTWIN	*new_proc(char *progname, char *arg, char *env, char *progdir,
								 WINCFG *cfg, int talkID);

extern void		term_proc(TEXTWIN *t);

extern TEXTWIN *new_shell(void);
extern TEXTWIN *start_prog(void);

extern void 	fd_input(void);
extern void		add_fd(int fd);

extern void		proc_init(void);

#endif
