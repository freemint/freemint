
#ifndef _tw_environ_h_
#define _tw_environ_h_


char *normal_env(int cols, int rows, int term);
char *shell_env	(int cols, int rows, int term, 
		 char *homedir, char *shell, bool login);
char *share_env	(int cols, int rows, int term, 
		 char *prgname, char *arg, char *env);

#endif
