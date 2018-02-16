/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * this is the system update daemon, its only purpose is to call
 * Sync() in regular intervals, so file systems get their sync()
 * function called.
 *
 */

# include "update.h"

long sync_time = 5;

# ifdef SYSUPDATE_DAEMON

# include "mint/proc.h"

# include "dosfile.h"
# include "dossig.h"
# include "filesys.h"
# include "k_kthread.h"

static void
do_sync (long sig)
{
	sys_s_ync ();
}

static void
update (void *arg)
{
	sys_p_signal (SIGALRM, (long) do_sync);
	sys_p_signal (SIGTERM, (long) do_sync);
	sys_p_signal (SIGQUIT, (long) do_sync);
	sys_p_signal (SIGHUP,  (long) do_sync);
	sys_p_signal (SIGTSTP, (long) do_sync);
	sys_p_signal (SIGINT,  (long) do_sync);
	sys_p_signal (SIGABRT, (long) do_sync);
	sys_p_signal (SIGUSR1, (long) do_sync);
	sys_p_signal (SIGUSR2, (long) do_sync);

	for (;;)
	{
		long tsync = sync_time;

		while (tsync > 32)
		{
			sys_f_select (32000, 0L, 0L, 0L);
			tsync -= 32;
		}

		if (tsync > 0)
			sys_f_select (tsync * 1000, 0L, 0L, 0L);

		do_sync (0);
	}

	kthread_exit (0);
	/* not reached */
}

# else

# include "timeout.h"

/* do_sync: sync all filesystems at regular intervals
 */
static void
do_sync (struct proc *p)
{
	sys_s_ync ();

	addroottimeout (1000L * sync_time, do_sync, 0);
}

# endif

void
start_sysupdate (void)
{
# ifndef SYSUPDATE_DAEMON

	addroottimeout (1000L * sync_time, do_sync, 0);

# else
	long r;

	r = kthread_create (NULL, update, NULL, NULL, "update");
	if (r != 0)
		FATAL ("can't create \"update\" kernel thread");
# endif
}
