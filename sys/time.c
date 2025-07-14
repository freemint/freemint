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
 *
 * Kernel time functions.
 *
 */

# include "time.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/arch/mfp.h"
# include "arch/syscall.h"
# include "arch/timer.h"
# include "arch/tosbind.h"

# include "k_prot.h"
# include "pipefs.h"
# include "procfs.h"
# include "shmfs.h"

# include "proc.h"


ushort timestamp;
ushort datestamp;
long clock_mode = 0;

/* Last value read at _hz_200. */
static ulong sys_lastticks;

# define SECS_PER_YEAR (364L * 24L * 3600L)
/* Of course this only works because CLOCKS_PER_SEC < 1000000. */
# define MICROSECONDS_PER_CLOCK (1000000L / CLOCKS_PER_SEC)

/* The current time in UTC. */
struct timeval xtime = { 0, 0 };

/* The boot time in UTC. */
struct timeval boottime = { 0, 0 };

/* The timezone that we're living in. */
struct timezone sys_tz = { 0, 0 };

/* Seconds west of GMT. */
long timezone = 0;

/* Correction to apply to our kernel time to get a valid setting
 * for the hardware clock.  If the hardware clock is ticking
 * in UTC it is zero.  If it is ticking in local time it is
 * minus timezone.
 */
static long sys2tos = 0;

/* Communications variables between quick_synch and do_gettimeofday. */
static ulong timerc = 0;
static ulong last_timerc = 0xffffffffL;

static ulong hardtime = 0;

static void quick_synch (void);

# if 0
/* Time of day in representation of the IKBD-Chip.  If somebody can
 * fix the routine that reads the values of the IKBD-clock we
 * will be able to retrieve the system time with an accuracy of
 * one second at system start-up.
 */
struct
{
	char year;	/* Year in packed BCD-format. */
	char month;	/* Month in packed BCD-format. */
	char day;	/* Day of month in packed BCD-format. */
	char hour;	/* Hour of day in packed BCD-format. */
	char minute;	/* Minute of hour in packed BCD-format. */
	char second;	/* Second of minute in packed BCD-format. */
} ikbd_time;

/* Holds address of old clock vector.  */
long oldcvec;
/* Tells if an IKBD packet has come */
volatile short packet_came = 0;		/* volatile, f**k */
# endif

/*
 * get/set time and date original functions.  Their use is deprecated
 * now.  Use Tgettimeofday () and Tsettimeofday () instead.
 */
long _cdecl
sys_t_getdate (void)
{
	TRACE (("Tgetdate ()"));
	return datestamp;
}

long _cdecl
sys_t_gettime (void)
{
	TRACE (("Tgettime ()"));
	return timestamp;
}

long _cdecl
sys_t_setdate (ushort date)
{
	struct timeval tv = { 0, 0 };

	if (!suser (get_curproc()->p_cred->ucr))
	{
		DEBUG (("Tsetdate: attempt to change time by unprivileged user"));
		return EPERM;
	}

	/*
	 * Disallow dates after >= 1.1.2098,
	 * since that would overflow the year value of a DOSTIME
	 */
	if (date >= ((128 - 10) << 9))
	{
		DEBUG (("Tsetdate: date overflow"));
		return EOVERFLOW;
	}
	tv.tv_sec = unixtime (timestamp, date) + timezone;
	return do_settimeofday (&tv);
}

long _cdecl
sys_t_settime (ushort time)
{
	struct timeval tv = { 0, 0 };

	if (!suser (get_curproc()->p_cred->ucr))
	{
		DEBUG (("Tsettime: attempt to change time by unprivileged user"));
		return EPERM;
	}

	tv.tv_sec = unixtime (time, datestamp) + timezone;
	return do_settimeofday (&tv);
}

/* This function is like t_gettimeofday except that it doesn't
 * use the struct timezone.  The caller has to take care that
 * the argument TV is a valid pointer.
 *
 * The time value will be calculated from the number of
 * 200-Hz-timer-interrupts.  This will give an accuracy of
 * 5000 ms.  The microseconds inbetween are interpolated from
 * the value found in the timer-c data register.  This counter
 * down from 192 to 1 once per timer interrupt period.  This
 * provides an accuracy of little more than 26 us.
 *
 * Some programs will get confused if the time is standing still
 * for a little amount of time.  We keep track of the last timeval
 * reported plus the additional microseconds we added to the
 * calculated timevals.  In case the last timeval and the current
 * timeval are equal we add MICRO_ADJUSTMENT microseconds and
 * increment this value.  This gives us time for about 26 us.
 * If this isn't enough we have to tell the truth however.
 */
static struct timeval last_tv = { 0, 0 };
static long micro_adjustment = 0;

long _cdecl
do_gettimeofday (struct timeval* tv)
{
	quick_synch ();

	*tv = xtime;

	tv->tv_usec += (192L - timerc) * MICROSECONDS_PER_CLOCK / 192L;

	if (last_tv.tv_sec == tv->tv_sec && last_tv.tv_usec == tv->tv_usec)
	{
		if (micro_adjustment < 25)
			micro_adjustment++;
	}
	else
	{
		micro_adjustment = 0;
	}

	last_tv = *tv;

	tv->tv_usec += micro_adjustment;

	if (tv->tv_usec >= 1000000L)
	{
		tv->tv_usec -= 1000000L;
		tv->tv_sec++;
	}

	return E_OK;
}

long _cdecl
sys_t_gettimeofday (struct timeval *tv, struct timezone *tz)
{
	TRACE (("Tgettimeofday (tv = 0x%p, tz = 0x%p)", tv, tz));

	if (tz != NULL)
		*tz = sys_tz;

	if (tv != NULL)
	{
		if (xtime.tv_sec >= 2147483647L)
			return EOVERFLOW;
		return do_gettimeofday (tv);
	}

	return E_OK;
}

/* Common function for setting the kernel time.  The caller has
 * to check for appropriate privileges and valid pointer.
 */
long _cdecl
do_settimeofday (struct timeval* tv)
{
	ulong tos_combined;

	TRACE (("do_settimeofday %ld.%ld s", tv->tv_sec, 1000000L * tv->tv_usec));

	/* We don't allow dates before Jan 2nd, 1980.  This avoids all
	 * headaches about timezones.
	 */
# define JAN_2_1980 (10L * 365L + 2L) * 24L * 60L * 60L
	if (tv->tv_sec < JAN_2_1980)
	{
		DEBUG (("do_settimeofday: attempt to rewind time to before 1980"));
		return EBADARG;
	}
	/*
	 * check if tv.sec + sys2tos would overflow the range of signed 32bit
	 */
	if (tv->tv_sec > (sys2tos < 0 ? 2147483647L + sys2tos : 2147483647L - sys2tos) ||
		(timezone < 0 && tv->tv_sec > 2147483647L + timezone))
	{
		DEBUG (("do_settimeofday: attempt to set time to after year 2038"));
		return EOVERFLOW;
	}

	/* The timeval we got is always in UTC */
	xtime = *tv;

	/* Now calculate timestamp and datestamp from that */
	tos_combined = unix2xbios (xtime.tv_sec - timezone);
	datestamp = (tos_combined >> 16) & 0xffff;
	timestamp = tos_combined & 0xffff;

	hardtime = unix2xbios (xtime.tv_sec + sys2tos);

	ROM_Settime (hardtime);

	return E_OK;
}

long _cdecl
sys_t_settimeofday (struct timeval *tv, struct timezone *tz)
{
	TRACE (("Tsettimeofday (tv = 0x%p, tz = 0x%p)", tv, tz));

	if (!suser (get_curproc()->p_cred->ucr))
	{
		DEBUG (("t_settimeofday: attempt to change time by unprivileged user"));
		return EPERM;
	}

	if (tz != NULL)
	{
		long old_timezone = timezone;

		sys_tz = *tz;
		timezone = sys_tz.tz_minuteswest * 60L;

		/* We have to distinguish now if the clock is ticking in UTC
		 * or local time.
		 */
		if (clock_mode == 0)
		{
			/* UTC */
			sys2tos = 0;
		}
		else
		{
			sys2tos = -timezone;

			/* If the timezone has really changed we have to
			 * correct the kernel's UTC time.  If the user has
			 * supplied a time this will be overwritten in an
			 * instant below but that doesn't hurt.  If a time
			 * was supplied it was really in UTC.
			 */
			xtime.tv_sec += (old_timezone - timezone);
		}

		/* Update timestamp and datestamp */
		synch_timers ();
	}

	if (tv != NULL)
	{
		long retval = do_settimeofday (tv);
		hardtime = 0;
		if (retval < 0)
			return retval;
	}

	return E_OK;
}

/* Adjust the current time of day by the amount in DELTA.
 * If OLDDELTA is not NULL, it is filled in with the amount
 * of time adjustment remaining to be done from the last
 * Tadjtime() call.
 */
long _cdecl
sys_t_adjtime(const struct timeval *delta, struct timeval *olddelta)
{
	TRACE (("Tadjtime (delta = 0x%p, olddelta = 0x%p)", delta, olddelta));

	if (!suser (get_curproc()->p_cred->ucr))
	{
		DEBUG (("t_adjtime: attempt to change time by unprivileged user"));
		return EPERM;
	}

	if (delta != NULL) {
		struct timeval tv;
		long retval = do_gettimeofday(&tv);
		if (retval < 0)
			return retval;

		tv.tv_usec += delta->tv_usec;
		if (tv.tv_usec >= 1000000L)
		{
			tv.tv_usec -= 1000000L;
			tv.tv_sec++;
		}
		tv.tv_sec += delta->tv_sec;
		
		retval = do_settimeofday (&tv);
		if (retval < 0)
			return retval;
	}

	return E_OK;
}

/* Most programs assume that the values returned by Tgettime()
 * and Tgetdate() are in local time.  We won't disappoint them.
 * However it is favorable to have at least the high-resolution
 * kernel time running in UTC instead.
 *
 * This means that Tgettime(), Tsettime(), Tgetdate() and
 * Tsetdate() keep on handling local times.  All other time-
 * related new system calls (Tgettimeofday, Tsettimeofday
 * and Tadjtimex) handle UTC instead.
 */
void
init_time (void)
{
	long value;

	if (machine == machine_unknown)
		value = 0;
	else
		value = _mfpregs->tbdr;

# if 0
	/* See, a piece of code is a function, not just a long integer */
	extern long _cdecl newcvec();  /* In intr.spp */
	KBDVEC *kvecs;
	/* Opcode for getting ikbd clock. */
	uchar ikbd_clock_get = 0x1c;
	register short count;
# endif

	/* Check if we are already initialized.  */
	if (xtime.tv_sec != 0)
		return;

	/* Interpolate. */
	value &= 0x000000ffL;
	value -= 192L;
	if (value < 0)
		value = 0;

# if 0
	/* Get the current setting of the hardware clock. Since we only bend
	 * the vector once we don't bother about xbra stuff. We can't go
	 * the fine MiNT way (using syskey, cf. mint.h) because this routine
	 * is called before the interrupts are initialized.
	 */

	/* Draco's explanation on why this doesn't work:
	 *
	 * 1. because of a bug - this caused bombs on startup (fixed).
	 * 2. because documentation lies - the Ikbdws() writes `len' chars,
	 *    not `len+1' (also fixed).
	 * 3. because documentation lies - the pointer passed to the
	 *    clock handler doesn't point to the packet header,
	 *    but to the packet itself (fixed as well).
	 * 4. finally - because the IKBD clock is not set,
	 *    at least on Falcon030 TOS. So the returned packet
	 *    actually consists of zeros only (not fixed).
	 *
	 * Notice: even if it worked as (previously) exspected, kernel's
	 * time package might work incorrectly when no keyboard is present.
	 *
	 */

	do {
		kvecs = (KBDVEC *) TRAP_Kbdvbase ();
	}
	while (kvecs->drvstat);

	oldcvec = kvecs->clockvec;
	kvecs->clockvec = (long) &newcvec;
	TRAP_Ikbdws (1, &ikbd_clock_get);
	for (count = 0; count < 9999 && packet_came == 0; count++);
	kvecs->clockvec = oldcvec;
# endif

	/* initialize datestamp & timestamp */
	{
		ulong tostime;

		tostime = TRAP_Gettime ();

		datestamp = (tostime >> 16) & 0xffff;
		timestamp = tostime & 0xffff;
	}

	sys_lastticks = *hz_200;

	xtime.tv_sec = unixtime (timestamp, datestamp);
	xtime.tv_usec = (sys_lastticks % CLOCKS_PER_SEC) * MICROSECONDS_PER_CLOCK
		+ value * MICROSECONDS_PER_CLOCK / 192L;

	/* set booting time */
	boottime = xtime;
}

static void
quick_synch (void)
{
	ulong current_ticks;
	long elapsed;	/* Microseconds elapsed since last quick_synch.
			 * Because this routine is guaranteed to be
			 * called at least once per second we don't have
			 * to care to much about overflows.
			 */

	if (machine == machine_unknown)
		timerc = 0;
	else
		timerc = _mfpregs->tbdr;

	current_ticks = *hz_200;

	/* Make sure that the clock runs monotonic.  */
	timerc &= 0x000000ffL;
	if (timerc > 192)
		timerc = 192;

	if (current_ticks <= sys_lastticks)
	{
		if (timerc > last_timerc)
			timerc = last_timerc;
	}

	last_timerc = timerc;

	if (current_ticks < sys_lastticks)
	{
		/* We had an overflow in _hz_200. */
		ulong uelapsed = 0xffffffffL - sys_lastticks;
		uelapsed += current_ticks;

		elapsed = uelapsed * MICROSECONDS_PER_CLOCK;
		TRACE (("quick_synch: 200-Hz-Timer overflow"));
	}
	else
	{
		elapsed = (current_ticks - sys_lastticks) * MICROSECONDS_PER_CLOCK;
	}

	sys_lastticks = current_ticks;

	xtime.tv_usec += elapsed;
	if (xtime.tv_usec >= 1000000L)
	{
		xtime.tv_sec += (xtime.tv_usec / 1000000L);
		xtime.tv_usec = xtime.tv_usec % 1000000L;
	}
}

/* Calculate timestamp and datestamp.  */
void _cdecl
synch_timers (void)
{
	ulong tos_combined;

	quick_synch ();

	/* Now adjust timestamp and datestamp to be in local time.  */
	tos_combined = unix2xbios (xtime.tv_sec - timezone);
	datestamp = (tos_combined >> 16) & 0xffff;
	timestamp = tos_combined & 0xffff;

	if (hardtime && get_curproc()->pid == 0)
	{
		/* Hm, if Tsetdate or Tsettime was called by the user our
		 * changes get lost. That's why this strange piece of code
		 * is here.
		 */
		hardtime = unix2xbios (xtime.tv_sec + sys2tos);
		ROM_Settime (hardtime);
		hardtime = 0;
	}
}

/* Change the kernel's notion of the system time.  If the kernel clock
 * (in UTC!) actually changes by the clock warp we have to change a lot
 * of timestamps too.
 */
void _cdecl
warp_clock (int mode)
{
	long diff;

	if ((mode == 0 && clock_mode == 0) || (mode != 0 && clock_mode != 0))
		return;

	if (clock_mode == 0)
	{
		/* Change it from UTC to local time.  */
		diff = timezone;
		xtime.tv_sec += diff;
		sys2tos = -diff;
		clock_mode = 1;
	}
	else
	{
		/* Change it back from local time to UTC.  */
		diff = -timezone;
		xtime.tv_sec += diff;
		sys2tos = 0;
		clock_mode = 0;
	}

	if (diff != 0)
	{
		PROC *p;
		struct fifo *fifo;
		struct shmfile *shm;

		procfs_stmp.tv_sec += diff;
		for (p = proclist; p != NULL; p = p->gl_next)
			p->started.tv_sec += diff;

		pipestamp.tv_sec += diff;
		for (fifo = piperoot; fifo != NULL; fifo = fifo->next)
		{
			fifo->mtime.tv_sec += diff;
			fifo->ctime.tv_sec += diff;
		}

		shmfs_stmp.tv_sec += diff;
		for (shm = shmroot; shm != NULL; shm = shm->next)
		{
			shm->mtime.tv_sec += diff;
			shm->ctime.tv_sec += diff;
		}

		/* The timestamps of the bios devices are a mess anyway.  */
	}

	/* Set timestamp and datestamp correctly. */
	synch_timers ();
}

long _cdecl
sys_b_gettime (void)
{
	TRACE (("gettime ()"));

	if (!xtime.tv_sec)
	{
		/* We're not initialized */
		init_time ();
	}

	return (((long) datestamp << 16) | (timestamp & 0xffff));
}

void _cdecl
sys_b_settime (ulong datetime)
{
	if (hardtime)
	{
		TRACE (("settime (%li) -> Settime (%li)", datetime, hardtime));

		/* Called from the kernel.  */
		ROM_Settime (hardtime);
	}
	else
	{
		struct timeval tv = { 0, 0 };
		ushort time, date;

		TRACE (("settime (%li) -> do_settimeofday", datetime));

		if (!suser (get_curproc()->p_cred->ucr))
		{
			DEBUG (("Settime: attempt to change time by unprivileged user"));
			return;
		}

		time = datetime & 0xffff;
		date = (datetime >> 16) & 0xffff;

		tv.tv_sec = unixtime (time, date) + timezone;

		do_settimeofday (&tv);
	}
}
