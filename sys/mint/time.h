/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * Copyright (C) 1998 by Guido Flohr.
 */

# ifndef _mint_time_h
# define _mint_time_h

# include "ktypes.h"


struct timeval
{
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* microseconds */
};

typedef struct time TIME;
struct time
{
	long	high_time;
	long	time;		/* This has to be signed!  */
	ulong	nanoseconds;
};

/*
 * Structure defined by POSIX.1b to be like a timeval.
 */
struct timespec
{
	long	tv_sec;		/* seconds */
	long	tv_nsec;	/* and nanoseconds */
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

struct timezone
{
	long	tz_minuteswest;	/* minutes west of Greenwich */
	long	tz_dsttime;	/* type of dst correction */
};


/*
 * Timeout events are stored in a list; the "when" field in the event
 * specifies the number of milliseconds *after* the last entry in the
 * list that the timeout should occur, so routines that manipulate
 * the list only need to check the first entry.
 */

typedef void _cdecl to_func (PROC *);

struct timeout
{
	TIMEOUT	*next;
	PROC	*proc;
	long	when;
	to_func	*func;	/* function to call at timeout */
	ushort	flags;
	long	arg;
};


# define ITIMER_REAL	0
# define ITIMER_VIRTUAL	1
# define ITIMER_PROF	2

struct itimervalue
{
	TIMEOUT	*timeout;
	long	interval;
	long	reqtime;
	long	startsystime;
	long	startusrtime;
};


# endif /* _mint_time_h */
