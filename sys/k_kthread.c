/*
 * $Id$
 *
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
 * Started: 2001-04-24
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "k_kthread.h"

# include "libkern/libkern.h"
# include "mint/proc.h"

# include "k_exec.h"
# include "k_exit.h"
# include "k_fork.h"
# include "kmemory.h"
# include "proc.h"


long
kthread_create (void (*func)(void *), void *arg, struct proc **np, const char *fmt, ...)
{
	struct proc *p2;
	long err;

	DEBUG (("kthread_create: 0x%lx", func));

	p2 = fork_proc1 (rootproc, FORK_SHAREVM|FORK_SHARECWD|FORK_SHAREFILES|FORK_SHARESIGS, &err);
	if (p2)
	{
		va_list args;
		int i;

		va_start (args, fmt);
		vsprintf (p2->fname, sizeof (p2->fname), fmt, args);
		vsprintf (p2->cmdlin, sizeof (p2->cmdlin), fmt, args);
		vsprintf (p2->name, sizeof (p2->name), fmt, args);
		va_end (args);

		/* kernel threads don't have a basepage */
		p2->base = NULL;

		/* zero the user registers, and set the FPU in a "clear" state */
		for (i = 0; i < 15; i++)
			p2->ctxt[CURRENT].regs[i] = 0;

		p2->ctxt[CURRENT].sr = 0x2000;	/* kernel threads work in super mode */
		p2->ctxt[CURRENT].fstate[0] = 0;
		p2->ctxt[CURRENT].pc = (long)func;
		p2->ctxt[CURRENT].usp = p2->sysstack;
		p2->ctxt[CURRENT].term_vec = (long) rts;

		p2->ctxt[SYSCALL].sr = 0x2000;	/* kernel threads work in super mode */
		p2->ctxt[SYSCALL].fstate[0] = 0;
		p2->ctxt[SYSCALL].pc = (long)func;
		p2->ctxt[SYSCALL].usp = p2->sysstack;
		p2->ctxt[SYSCALL].term_vec = (long) rts;

		*((long *)(p2->ctxt[CURRENT].usp + 4)) = (long) arg;
		*((long *)(p2->ctxt[SYSCALL].usp + 4)) = (long) arg;

		DEBUG (("kthread_create: p2 0x%lx", p2));

		if (np != NULL)
			*np = p2;

		run_next (p2, 3);

		return 0;
	}

	DEBUG (("kthread_create: failed (%li)", err));
	return err;
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void
kthread_exit (short code)
{
	struct proc *p = curproc;

	if (code != 0)
		ALERT ("WARNING: thread `%s' (%d) exits with status %d\n",
			p->name, p->pid, code);

	terminate (p, code, ZOMBIE_Q);

	/* not reached */
	for (;;) ;
}
