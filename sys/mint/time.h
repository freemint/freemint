/*
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
# include "arch/timer.h"
# else
# include <sys/param.h>
# include <sys/types.h>
# endif

/**
 * This is the representation of a time value.
 * The syscalls expect and deliver timevalues in this representation.
 * 
 */
struct timeval
{
	time32_t	tv_sec;		/**< seconds */
	long		tv_usec;	/**< microseconds */
};
struct timeval64
{
	time64_t	tv_sec;		/**< seconds */
	long		tv_usec;	/**< microseconds */
};


typedef struct time TIME;

/**
 * Structure used to store file related timestamps of struct stat
 */
struct time
{
	union {
		struct {
			long	high_time;
			time32_t	time;		/* This has to be signed!  */
		};
		time64_t time64;
	};
	ulong	nanoseconds;
};

# ifdef __KERNEL__

/*
 * Macros used to get/set the time & date fields of
 * struct xattr.
 * Note that, although these are broken down into two shorts,
 * most of the time they just contain a long value of a unix
 * timestamp (however in local time, not UTC)
 */
#define SET_XATTR_TD(a,x,ut)			\
{						\
	(a)->__CONCAT(x,time).time32 = ut;	\
}

#define GET_XATTR_TD(a,x) ((a)->__CONCAT(x,time).time32)

#define dta_UTC_local_dos(dta,xattr,x)					\
{									\
	union { ushort s[2]; ulong l;} data;				\
									\
	/* UTC -> localtime -> DOS style */				\
	data.l = GET_XATTR_TD(&xattr, x);			\
	data.l = dostime(data.l - timezone);			\
	dta->dta_time	= data.s[0];					\
	dta->dta_date	= data.s[1];					\
}

#define xtime_to_local_dos(a,x)				\
{							\
	u_int32_t data;		\
	data = GET_XATTR_TD(a, x);		\
	data = dostime(data - timezone);		\
	SET_XATTR_TD(a, x, data);		\
}

/**
 * Structure defined by POSIX.1b to be like a timeval.
 */
struct timespec
{
	time32_t	tv_sec;		/**< seconds */
	long		tv_nsec;	/**< and nanoseconds */
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


typedef void _cdecl to_func (PROC *, long arg);
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
