/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * Copyright (C) 1998 by Guido Flohr.
 */

# ifndef _time_h
# define _time_h

# include "mint/mint.h"


# define CLOCKS_PER_SEC HZ

struct timezone
{
	long	tz_minuteswest;	/* minutes west of Greenwich */
	long	tz_dsttime;	/* type of dst correction */
};

# ifdef __KERNEL__

/* Global variables exported into the kernel. */
extern struct timeval xtime;		/* Current kernel time (UTC). */
extern struct timeval boottime;		/* kernel boot time (UTC). */
extern struct timezone sys_tz;		/* Timezone for T[gs]ettimeofday. */
extern long clock_mode;			/* Non-zero if clock is ticking in
					 * local time instead of UTC
					 * (read-only).
					 */

/* Used by filesystems and the kernel, updated once per second.  */
extern ushort timestamp;		/* Local time in TOS format. */
extern ushort datestamp;		/* Local date in TOS format. */

extern long timezone;			/* utc -> locatime offsett */

void init_time (void);			/* Initialize date/time stuff. */
void synch_timers (void);		/* Synchronize all os timers. */
void warp_clock (int mode);		/* Change the kernel's notion of
					 * the time provided by the 
					 * hardware clock according to MODE.
					 */
                                   
long _cdecl do_gettimeofday (struct timeval *tv);
long _cdecl do_settimeofday (struct timeval *tv);

/* GEMDOS */
long _cdecl t_getdate (void);
long _cdecl t_setdate (ushort date);
long _cdecl t_gettime (void);
long _cdecl t_settime (ushort time);

/* New GEMDOS extensions */
long _cdecl t_gettimeofday (struct timeval *tv, struct timezone *tz);
long _cdecl t_settimeofday (struct timeval *tv, struct timezone *tz);

/* XBIOS */
long _cdecl gettime (void);
void _cdecl settime (ulong datetime);

# endif

# define ITIMER_REAL 0
# define ITIMER_VIRTUAL 1
# define ITIMER_PROF 2

# endif /* _time_h */
