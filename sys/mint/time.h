/*
 * $Id$
 * 
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * Copyright (C) 1998 by Guido Flohr.
 */

# ifndef _mint_time_h
# define _mint_time_h

# ifdef __KERNEL__
# include "ktypes.h"
# else
# include <sys/types.h>
# endif

/**
 * This is the representation of a time value.
 * The syscalls expect and deliver timevalues in this representation.
 * 
 */
struct timeval
{
	long	tv_sec;		/**< seconds */
	long	tv_usec;	/**< microseconds */
};


typedef struct time TIME;

/**
 * DONT KNOW YET. PLEASE FILLOUT.
 */
struct time
{
	long	high_time;
	long	time;		/* This has to be signed!  */
	ulong	nanoseconds;
};


# ifdef __KERNEL__

/**
 * Structure defined by POSIX.1b to be like a timeval.
 */
struct timespec
{
	long	tv_sec;		/**< seconds */
	long	tv_nsec;	/**< and nanoseconds */
};

# define TIMEVAL_TO_TIMESPEC(tv, ts)		\
{						\
	(ts)->tv_sec = (tv)->tv_sec;		\
	(ts)->tv_nsec = (tv)->tv_usec * 1000;	\
}
# define TIMESPEC_TO_TIMEVAL(tv, ts)		\
{						\
	(tv)->tv_sec = (ts)->tv_sec;		\
	(tv)->tv_usec = (ts)->tv_nsec / 1000;	\
}


# define CLOCKS_PER_SEC HZ

/**
 * 
 */
struct timezone
{
	long	tz_minuteswest;	/**< Minutes west of Greenwich */
	long	tz_dsttime;		/**< Type of dst correction */
};


typedef void _cdecl to_func (PROC *);
/**
 * Representation of an timeout event.
 * The timeout events are stored in a callout list (single linked).
 * The events are sorted in the order of their occurence. Therefor
 * the time for searching the list is saved. The insertion of he event takes
 * longer, but does't happend too often, compared with the searching, which
 * happens at every clock tick. The time when to trigger the event is given
 * relative to the event in the list before this event. So decreasing the
 * counter for the first event in the list decereses the counter for the other
 * events too.
 */
struct timeout
{
	TIMEOUT	*next;		/**< link to next event in the list.				*/
	PROC	*proc;		/**< This process registerd this timeout event.		*/
	long	when;		/**< Difference to the event before this in the list.*/
	to_func	*func;		/**< Function to call at timeout					*/
	ushort	flags;
	long	arg;		/**< Argument to the function which gets called.	*/
};


# define ITIMER_REAL	0
# define ITIMER_VIRTUAL	1
# define ITIMER_PROF	2

/**
 * DONT KNOW YET. PLEASE FILLOUT.
 */
struct itimervalue
{
	TIMEOUT	*timeout;
	long	interval;
	long	reqtime;
	long	startsystime;
	long	startusrtime;
};

# endif /* __KERNEL__ */

# endif /* _mint_time_h */
