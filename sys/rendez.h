/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _rendez_h
# define _rendez_h

# include "mint/mint.h"


long _cdecl sys_p_msg (int mode, long mbid, char *ptr);
long _cdecl sys_p_semaphore (int mode, long id, long timeout);
void free_semaphores (int pid);


# endif /* _rendez_h */
