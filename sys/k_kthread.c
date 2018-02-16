/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
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
 */

# include "k_kthread.h"

# include "libkern/libkern.h"
# include "mint/signal.h"

# include "k_exec.h"
# include "k_exit.h"
# include "k_fork.h"
# include "kmemory.h"
# include "proc.h"


static void
kthread_fatal(void *arg)
{
	FATAL("restoring SYSCALL context from kernel thread???");
}

long _cdecl
kthread_create(struct proc *p, void _cdecl (*func)(void *), void *arg,
	       struct proc **np, const char *fmt, ...)
{
	va_list args;
	long r;

	va_start(args, fmt);
	r = kthread_create_v(p, func, arg, np, fmt, args);
	va_end(args);

	return r;
}

long
kthread_create_v(struct proc *p, void _cdecl (*func)(void *), void *arg,
		 struct proc **np, const char *fmt, va_list args)
{
	struct proc *p2;
	long err;

	if (!p) p = rootproc;

	DEBUG(("kthread_create for pid %i: 0x%p", p->pid, func));

	p2 = fork_proc1(p, FORK_SHAREVM|FORK_SHARECWD|FORK_SHAREFILES|FORK_SHAREEXT, &err);
	if (p2)
	{
		int i;

		/* XXX */
		p2->ppid = 0;
		p2->pgrp = 0;

		/* this blocks SIGKILL for the update process */
		p2->p_flag |= P_FLAG_SYS;

		kvsprintf(p2->fname, sizeof(p2->fname), fmt, args);
		kvsprintf(p2->cmdlin, sizeof(p2->cmdlin), fmt, args);
		kvsprintf(p2->name, sizeof(p2->name), fmt, args);

		/* initialize signals */
		p2->p_sigmask = 0;
		for (i = 0; i < NSIG; i++)
		{
			struct sigaction *sigact = &SIGACTION(p2, i);

			if (sigact->sa_handler != SIG_IGN)
			{
				sigact->sa_handler = SIG_DFL;
				sigact->sa_mask = 0;
				sigact->sa_flags = 0;
			}
		}

		/* zero the user registers, and set the FPU in a "clear" state */
		for (i = 0; i < 15; i++)
		{
			p2->ctxt[CURRENT].regs[i] = 0;
			p2->ctxt[SYSCALL].regs[i] = 0;
		}

		p2->ctxt[CURRENT].sr = 0x2000;	/* kernel threads work in super mode */
		p2->ctxt[CURRENT].fstate.bytes[0] = 0;
		p2->ctxt[CURRENT].pc = (long)func;
		p2->ctxt[CURRENT].usp = p2->sysstack;
		p2->ctxt[CURRENT].term_vec = (long) rts;

		p2->ctxt[SYSCALL].sr = 0x2000;
		p2->ctxt[SYSCALL].fstate.bytes[0] = 0;
		p2->ctxt[SYSCALL].pc = (long)kthread_fatal;
		p2->ctxt[SYSCALL].usp = p2->sysstack;
		p2->ctxt[SYSCALL].term_vec = (long) rts;

		*((long *)(p2->ctxt[CURRENT].usp + 4)) = (long) arg;
		*((long *)(p2->ctxt[SYSCALL].usp + 4)) = (long) arg;

		DEBUG(("kthread_create: p2 0x%p", p2));

		if (np != NULL)
			*np = p2;

		run_next(p2, 3);
		return 0;
	}

	DEBUG(("kthread_create: failed (%li)", err));
	return err;
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void _cdecl
kthread_exit(short code)
{
	struct proc *p = get_curproc();

	if (code != 0)
		ALERT("WARNING: thread `%s' (%d) exits with status %d\n",
			p->name, p->pid, code);

	terminate(p, code, ZOMBIE_Q);

	/* not reached */
	FATAL("terminate returned???");
}
