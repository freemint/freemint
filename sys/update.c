/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/* uk: this is the system update daemon, its only purpose is to call
 *     Sync() in regular intervals, so file systems get their sync()
 *     function called.
 */

# include "update.h"


long sync_time = 5;


# ifdef SYSUPDATE_DAEMON

# include "mint/basepage.h"
# include "mint/signal.h"

# include "dosmem.h"
# include "k_exec.h"

# include <mintbind.h>


# define UPDATE_STKSIZE		6144

# ifdef SYSUPDATE_DAEMON
short update_pid;
# endif

static void
do_sync (long sig)
{
	UNUSED (sig);
	
	Sync ();
}

static void
update (BASEPAGE *bp)
{
	(void) Pdomain (1);
	(void) Pnice (-5);
	
	Psignal (SIGALRM, do_sync);
	Psignal (SIGTERM, do_sync);
	Psignal (SIGQUIT, do_sync);
	Psignal (SIGHUP,  do_sync);
	Psignal (SIGTSTP, do_sync);
	Psignal (SIGINT,  do_sync);
	Psignal (SIGABRT, do_sync);
	Psignal (SIGUSR1, do_sync);
	Psignal (SIGUSR2, do_sync);
	
	for (;;)
	{
		long tsync = sync_time;
		
		while (tsync > 32)
		{
			(void) Fselect (32000, 0L, 0L, 0L);
			tsync -= 32;
		}
		
		if (tsync > 0)
			(void) Fselect ((int) tsync * 1000, 0L, 0L, 0L);
		
		do_sync (0);
	}
	
	Pterm (0);
	
	/* not reached */
}

# else

/* do_sync: sync all filesystems at regular intervals
 */
static void
do_sync (PROC *p)
{
	s_ync ();
	
	addroottimeout (1000L * sync_time, do_sync, 0);
}

# endif

void
start_sysupdate (void)
{
# ifndef SYSUPDATE_DAEMON
	
	addroottimeout (1000L * sync_time, do_sync, 0);
	
# else
	
	BASEPAGE *b;
	long r;
	
	/* start a new asynchronous process
	 */
	
	/* create basepage */
	b = (BASEPAGE *) sys_pexec (PE_CBASEPAGE, 0L, "", 0L); 
	r = m_shrink (0, (virtaddr) b, UPDATE_STKSIZE + 256L);
	assert (r >= 0L);
	
	b->p_tbase = (long) update;
	b->p_hitpa = (long) b + UPDATE_STKSIZE + 256L;
	
	update_pid = sys_pexec (PE_ASYNC_GO, "update", b, 0L);
# endif
}
