/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * Copyright (C) 1998 by Guido Flohr.
 */

# ifndef _resource_h
# define _resource_h

# include "mint/mint.h"


# ifndef __MSHORT__
# error This file is not 32-bit clean.
# endif

# ifndef MIN_NICE
# define MIN_NICE -20
# endif
# ifndef MAX_NICE
# define MAX_NICE 20
# endif

# define PRIO_MIN MIN_NICE   /* The smallest valid priority value. */
# define PRIO_MAX MAX_NICE   /* The highest valid priority value. */

/* Values for the first argument to `getpriority' and `setpriority'.
 * Read or set the priority of one process.  Second argument is
 * a process ID.
 */
# define PRIO_PROCESS 0

/* Read or set the priority of one process group.  Second argument is
 * a process group ID.
 */
# define PRIO_PGRP 1

/* Read or set the priority of one user's processes.  Second argument is
 * a user ID.
 */
# define PRIO_USER 2


# ifdef __KERNEL__

/* Return the highest priority of any process specified by WHICH and WHO
 * (see above); if WHO is zero, the current process, process group, or user
 * (as specified by WHO) is used. A lower priority number means higher
 * priority. Priorities range from PRIO_MIN to PRIO_MAX (above).
 */
long _cdecl p_getpriority (int which, int who);

/* Set the priority of all processes specified by WHICH and WHO (see above)
 * to PRIO.  Returns 0 on success, -1 on errors.
 */
long _cdecl p_setpriority (int which, int who, int prio);

/* Provided for backward compatibility.  */
long _cdecl p_renice (int pid, int increment);
long _cdecl p_nice (int increment);

# endif /* __KERNEL__ */


# endif	/* _resource_h  */
