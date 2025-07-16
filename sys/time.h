/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998 Guido Flohr <guido@freemint.de>
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
 * Author: Guido Flohr <guido@freemint.de>
 * Started: 1998-03-30
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _time_h
# define _time_h

# include "mint/mint.h"
# include "mint/time.h"


/* Global variables exported into the kernel. */
extern struct timeval64 xtime64;		/* Current kernel time (UTC). */
extern struct timeval64 boottime;		/* kernel boot time (UTC). */
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

/* GEMDOS */
long _cdecl sys_t_getdate (void);
long _cdecl sys_t_setdate (ushort date);
long _cdecl sys_t_gettime (void);
long _cdecl sys_t_settime (ushort time);

/* New GEMDOS extensions */
long _cdecl sys_t_gettimeofday (struct timeval *tv, struct timezone *tz);
long _cdecl sys_t_settimeofday (struct timeval *tv, struct timezone *tz);
long _cdecl sys_t_adjtime      (const struct timeval *delta, struct timeval *olddelta);
long _cdecl sys_t_gettimeofday64 (struct timeval64 *tv, struct timezone *tz);
long _cdecl sys_t_settimeofday64 (struct timeval64 *tv, struct timezone *tz);

/* XBIOS */
long _cdecl sys_b_gettime (void);
void _cdecl sys_b_settime (ulong datetime);

void xfs_warp_clock(long diff);

# endif /* _time_h */
