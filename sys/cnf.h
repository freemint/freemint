/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _cnf_h
# define _cnf_h

# include "mint/mint.h"


extern int init_is_gem;

extern char *init_prg;
extern char *init_env;
extern char init_tail[256];


void	load_config	(void);


# endif /* _cnf_h */
