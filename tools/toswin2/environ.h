
#ifndef tw_environ_h
# define tw_environ_h 1

#ifndef tw_global_h
# include "global.h"
#endif

char *normal_env(int cols, int rows, int term);
char *shell_env	(int cols, int rows, int term, 
		 char *homedir, char *shell, bool login);
char *share_env	(int cols, int rows, int term, 
		 char *prgname, char *arg, char *env);

#endif
