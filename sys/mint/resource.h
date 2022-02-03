/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-10-31
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_resource_h
# define _mint_resource_h

# include "kcompiler.h"


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


# define RLIMIT_CPU	0		/* cpu time (milliseconds) */
# define RLIMIT_CORE	1		/* core memory */
# define RLIMIT_DATA	2		/* amount of malloc'd memory */
# define RLIMIT_FSIZE	3		/* maximum file size */
# define RLIMIT_STACK	4		/* stack size */
# define RLIMIT_NPROC	5		/* number of processes */
# define RLIMIT_NOFILE	6		/* number of open files */

# define RLIM_NLIMITS	7		/* number of resource limits */

# define RLIM_INFINITY	(~((u_int64_t) 1 << 63)) /* no limit */
# define RLIM_SAVED_MAX	RLIM_INFINITY	/* unrepresentable hard limit */
# define RLIM_SAVED_CUR	RLIM_INFINITY	/* unrepresentable soft limit */

struct rlimit
{
	llong	rlim_cur;		/* current (soft) limit */
	llong	rlim_max;		/* maximum value for rlim_cur */
};

/* process limits */
struct plimit
{
	struct rlimit	limits[RLIM_NLIMITS];
	ulong		flags;
	long		links;		/* number of references */
};


# endif /* _mint_resource_h */
