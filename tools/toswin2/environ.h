#ifndef _tw_environ_h_
#define _tw_environ_h_

extern char *normal_env	(int cols, int rows, int term);
extern char *shell_env	(int cols, int rows, int term, 
									char *homedir, char *shell, bool login);
extern char *share_env	(int cols, int rows, int term, 
									char *prgname, char *arg, char *env);

#endif
