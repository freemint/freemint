/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _update_h
# define _update_h

# include "mint/mint.h"


extern long sync_time;

void start_sysupdate (void);

# ifdef SYSUPDATE_DAEMON
extern short update_pid;
# endif


# endif /* _update_h */
