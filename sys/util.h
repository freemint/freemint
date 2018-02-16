/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _util_h
# define _util_h

# include "mint/mint.h"


struct proc *	pid2proc	(int pid);
int		newpid		(void);
void		set_pid_1	(void);


# endif /* _util_h */
